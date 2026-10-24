"""Dedicated Linux/x86-64 subreaper and sole wait consumer for one custody launch.

Embedding callers must use runtime_custody.launch(), which creates a private CLI
worker. Only an already isolated, single-threaded executable may explicitly call
declare_isolated_owner(); it must not create unrelated children afterwards.
"""
import ctypes
import errno
import os
from pathlib import Path
import signal
import sys
import time


class OwnershipError(RuntimeError):
    pass


_WALL = 0x40000000
_SET_CHILD_SUBREAPER = 36
_GET_CHILD_SUBREAPER = 37
_GET_EVENT = 0x4201
_CONT = 7
_EVENT_SPAWN = frozenset((1, 2, 3))
_libc = ctypes.CDLL(None, use_errno=True)
_libc.prctl.restype = ctypes.c_int
_libc.ptrace.argtypes = (ctypes.c_uint, ctypes.c_int, ctypes.c_void_p, ctypes.c_void_p)
_libc.ptrace.restype = ctypes.c_long
_libc.signalfd.argtypes = (ctypes.c_int, ctypes.c_void_p, ctypes.c_int)
_libc.signalfd.restype = ctypes.c_int
_owner = None


def _native(result):
    if result == -1:
        error = ctypes.get_errno()
        raise OSError(error, os.strerror(error))
    return result


class IsolatedOwner:
    """An explicit process-wide ownership declaration, never an embedding shim."""
    def __init__(self):
        if sys.platform != 'linux' or os.uname().machine != 'x86_64':
            raise OwnershipError('Isolated custody ownership requires Linux x86-64')
        self.pid = os.getpid()
        self._check_isolated(empty=True)
        # Installing SIG_DFL clears an inherited SIG_IGN/SA_NOCLDWAIT disposition;
        # a default SIGCHLD, unlike explicit SIG_IGN, retains waitable zombies.
        signal.signal(signal.SIGCHLD, signal.SIG_DFL)
        self.previous_signal_mask = signal.pthread_sigmask(signal.SIG_BLOCK, {signal.SIGCHLD})
        mask = (ctypes.c_ulong * 16)()
        mask[0] = 1 << (signal.SIGCHLD - 1)
        self.wakeup_fd = None
        try:
            self.wakeup_fd = _native(_libc.signalfd(-1, ctypes.byref(mask), os.O_NONBLOCK | os.O_CLOEXEC))
            _native(_libc.prctl(_SET_CHILD_SUBREAPER, ctypes.c_ulong(1), 0, 0, 0))
            enabled = ctypes.c_int()
            _native(_libc.prctl(_GET_CHILD_SUBREAPER, ctypes.byref(enabled), 0, 0, 0))
            if enabled.value != 1:
                raise OwnershipError('Kernel did not establish dedicated subreaper ownership')
        except BaseException:
            if self.wakeup_fd is not None:
                os.close(self.wakeup_fd)
            signal.pthread_sigmask(signal.SIG_SETMASK, self.previous_signal_mask)
            raise
        self.root_pid = None
        self.root_status = None
        self.callbacks = {}
        self.pending = []
        self.initial_stop_deadlines = {}
        self.rejections = []
        self.reaped_tasks = []
        self.observed_children = set()
        self.wait_echild = False
        self.retiring = False
        self.sent_sigkill = False
        self.retirement_deadline_exceeded = False
        self.launch_started = False
        self._collecting = False

    def _direct_children(self):
        # Only this dedicated single-threaded process can reap these children.
        # No wait occurs between this census and the retirement signals below.
        text = Path(f'/proc/self/task/{self.pid}/children').read_text(encoding='ascii')
        return {int(value) for value in text.split()}

    def _check_isolated(self, empty=False):
        if os.getpid() != self.pid:
            raise OwnershipError('Isolated owner cannot be inherited across fork')
        tids = {int(path.name) for path in Path('/proc/self/task').iterdir()}
        if tids != {self.pid}:
            raise OwnershipError('Custody owner must remain single-threaded')
        children = self._direct_children()
        if empty and children:
            raise OwnershipError('Dedicated owner has unrelated or pending children: ' + str(sorted(children)))
        return children

    def begin_launch(self):
        if self.launch_started and not self.complete:
            raise OwnershipError('Previous owned tree has not retired')
        self._check_isolated(empty=True)
        self.drain_notifications()
        self.root_pid = self.root_status = None
        self.callbacks.clear()
        self.pending.clear()
        self.initial_stop_deadlines.clear()
        self.rejections.clear()
        self.reaped_tasks.clear()
        self.observed_children.clear()
        self.wait_echild = False
        self.retiring = self.sent_sigkill = False
        self.retirement_deadline_exceeded = False
        self.launch_started = True
        return self

    def ready_to_spawn(self):
        # Trusted synchronous ELF inspection has finished and reaped its helpers.
        self._check_isolated(empty=True)
        if self.root_pid is not None:
            raise OwnershipError('Dedicated owner already launched its target')

    def set_root(self, pid):
        if self.root_pid is not None or pid <= 0:
            raise OwnershipError('Invalid or repeated owned root registration')
        self.root_pid = int(pid)
        self.observed_children.add(self.root_pid)
        self.wait_echild = False

    def restore_child_signals(self):
        signal.pthread_sigmask(signal.SIG_SETMASK, self.previous_signal_mask)

    def register_trace(self, tid, callback):
        tid = int(tid)
        if tid <= 0 or tid in self.callbacks:
            raise OwnershipError('Invalid or repeated trace registration')
        self.callbacks[tid] = callback
        self.initial_stop_deadlines.pop(tid, None)
        self.wait_echild = False

    def drain_notifications(self):
        while True:
            try:
                packet = os.read(self.wakeup_fd, 4096)
            except BlockingIOError:
                return
            if not packet or len(packet) % 128:
                raise OwnershipError('Malformed dedicated owner SIGCHLD notification')

    def _reject(self, tid, reason, **metadata):
        if not any(row['tid'] == tid and row['reason'] == reason for row in self.rejections):
            self.rejections.append({'tid': tid, 'reason': reason,
                                    'monotonic_ns': time.monotonic_ns(), **metadata})

    def _census(self):
        children = self._check_isolated()
        self.observed_children.update(children)
        for tid in children:
            if tid != self.root_pid and tid not in self.callbacks:
                self._reject(tid, 'Unregistered direct or adopted child', source='proc-task-children')
        return children

    def _retire_unregistered_stop(self, tid, status):
        # This status itself is fresh ptrace ownership evidence, not an unmatched
        # numeric clone result. Service spawn events before killing the stop.
        if status >> 16 in _EVENT_SPAWN:
            child = ctypes.c_ulong()
            _native(_libc.ptrace(_GET_EVENT, tid, 0, ctypes.byref(child)))
            self._reject(int(child.value), 'Unregistered ptrace creation event', parent_tid=tid)
        try:
            # Signal injection on PTRACE_EVENT stops is not delivery.
            os.kill(tid, signal.SIGKILL)
            _native(_libc.ptrace(_CONT, tid, 0, signal.SIGKILL))
            self.sent_sigkill = True
        except OSError as error:
            if error.errno != errno.ESRCH:
                raise

    def _dispatch_pending(self, stop_timeout):
        progress = False
        # A seized child's initial EVENT_STOP may precede the parent's creation
        # event even across collect calls. Only the parent's kernel event may
        # register it; until then retain the stop without issuing any restart.
        # The syscall gate supplies its existing stop deadline, not a new budget.
        while True:
            known = next((index for index, (tid, _) in enumerate(self.pending)
                          if tid in self.callbacks), None)
            if known is None:
                break
            tid, status = self.pending.pop(known)
            terminal = os.WIFEXITED(status) or os.WIFSIGNALED(status)
            try:
                self.callbacks[tid](tid, status)
            except Exception as error:
                if not self.retiring:
                    raise
                self._reject(tid, 'Registered retirement callback failed',
                             failure=type(error).__name__ + ': ' + str(error), wait_status=status)
                if os.WIFSTOPPED(status):
                    self._retire_unregistered_stop(tid, status)
            finally:
                if terminal:
                    self.callbacks.pop(tid, None)
            progress = True
        for tid, status in tuple(self.pending):
            if tid == self.root_pid and not os.WIFSTOPPED(status):
                self.pending.remove((tid, status))
                progress = True
                continue
            if (not self.retiring and not self.wait_echild and self.callbacks and
                    stop_timeout > 0 and os.WIFSTOPPED(status) and
                    status >> 16 == 128 and os.WSTOPSIG(status) == signal.SIGTRAP):
                deadline = self.initial_stop_deadlines.setdefault(tid, time.monotonic() + stop_timeout)
                if time.monotonic() < deadline:
                    continue
            self._reject(tid, 'Unregistered owned task wait status', wait_status=status,
                         source='waitpid-wall')
            if os.WIFEXITED(status) or os.WIFSIGNALED(status):
                self.pending.remove((tid, status))
                self.initial_stop_deadlines.pop(tid, None)
                progress = True
            elif self.retiring and os.WIFSTOPPED(status):
                self._retire_unregistered_stop(tid, status)
                self.pending.remove((tid, status))
                self.initial_stop_deadlines.pop(tid, None)
                progress = True
        return progress

    def collect(self, *, stop_timeout=0.0):
        if self._collecting:
            raise OwnershipError('Dedicated owner wait dispatch cannot recurse')
        self._collecting = True
        progress = False
        try:
            self._census()
            while True:
                try:
                    tid, status = os.waitpid(-1, os.WNOHANG | _WALL)
                except ChildProcessError:
                    self.wait_echild = True
                    break
                except InterruptedError:
                    continue
                self.wait_echild = False
                if not tid:
                    break
                progress = True
                if os.WIFEXITED(status) or os.WIFSIGNALED(status):
                    self.reaped_tasks.append({'tid': tid, 'wait_status': status,
                                              'returncode': os.waitstatus_to_exitcode(status)})
                    if tid == self.root_pid:
                        self.root_status = status
                self.pending.append((tid, status))
            progress |= self._dispatch_pending(stop_timeout)
            # Callback delivery can expose an additional initial/final stop on
            # the next collect. ECHILD is never enough with a nonempty ledger.
            return progress
        finally:
            self._collecting = False

    @property
    def complete(self):
        return (self.wait_echild and not self.callbacks and not self.pending
                and not self.initial_stop_deadlines
                and (self.root_pid is None or self.root_status is not None))

    def retire(self, kernel=None, timeout=5.0):
        self.retiring = True
        if kernel is not None:
            kernel.retiring = True
            try:
                self.sent_sigkill = kernel._retire_stopped() or self.sent_sigkill
            except Exception as error:
                self._reject(self.root_pid or self.pid, 'Could not resume an owned retirement stop',
                             failure=type(error).__name__ + ': ' + str(error))
        deadline = time.monotonic() + timeout
        while True:
            # A direct child's PID cannot be reused until this sole owner reaps
            # it. Census-to-signal is serialized, with no intervening wait.
            children = None
            try:
                children = self._census()
                for tid in children:
                    try:
                        os.kill(tid, signal.SIGKILL)
                        self.sent_sigkill = True
                    except ProcessLookupError:
                        pass
                progress = self.collect()
            except Exception as error:
                self._reject(self.root_pid or self.pid, 'Retirement ownership service failed',
                             failure=type(error).__name__ + ': ' + str(error))
                progress = False
            trace_ledger_retired = kernel is None or not kernel.attached or kernel.complete
            if self.complete and trace_ledger_retired and children is not None and not children:
                # An empty direct-child census followed by ECHILD and an empty
                # trace ledger leaves no owned ancestor able to fork/adopt.
                return True
            if time.monotonic() >= deadline:
                self.retirement_deadline_exceeded = True
                return False
            if not progress:
                time.sleep(.001)

    def metadata(self):
        return {'dedicated_subreaper': True, 'owner_pid': self.pid,
                'owned_tree_retired': self.complete, 'wait_echild': self.wait_echild,
                'root_pid': self.root_pid, 'root_wait_status': self.root_status,
                'root_status_accounted': self.root_pid is None or self.root_status is not None,
                'remaining_trace_tids': sorted(self.callbacks),
                'pending_wait_statuses': [{'tid': tid, 'wait_status': status} for tid, status in self.pending],
                'observed_direct_children': sorted(self.observed_children),
                'reaped_tasks': list(self.reaped_tasks), 'rejected_tasks': list(self.rejections),
                'retirement_deadline_exceeded': self.retirement_deadline_exceeded,
                'supervisor_sent_sigkill': self.sent_sigkill}


def declare_isolated_owner():
    """Declare this executable dedicated; call before preparing images/children.

    This changes process-wide SIGCHLD/subreaper state. It is intentionally not
    automatic in Supervisor or public launch(), and cannot be used after fork
    to inherit a declaration made by another process.
    """
    global _owner
    if _owner is None:
        _owner = IsolatedOwner()
    else:
        _owner._check_isolated()
    return _owner


def require_isolated_owner():
    if _owner is None or _owner.pid != os.getpid():
        raise OwnershipError('Supervisor requires an explicit isolated owner; embedding callers must use launch()')
    _owner._check_isolated()
    return _owner
