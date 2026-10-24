#!/usr/bin/env python3
"""Linux x86-64 syscall-metadata custody gate for an already blocked tracee.

Only PTRACE_GET_SYSCALL_INFO, event IDs, proc status/maps/personality and FD stat metadata
are read. No PEEK, GETREGS, process_vm_readv, argument-pointer dereference,
register modification, syscall substitution or Source state access is used.
The initial sealed loader/application/auditor bootstrap is admitted separately
by the supervisor before this gate attaches at the auditor activation barrier.
READ_IMPLIES_EXEC and unsupported syscall ABIs reject before execution. Exiting
tasks retain final-wait ownership but cannot be used as live mapping witnesses.
The required isolated descendant owner supplies the sole wait dispatcher and
rejected-tree retirement; this gate is not a process sandbox.
"""
import ctypes
import errno
import json
import os
from pathlib import Path
import signal
import stat
import time


class SyscallCustodyError(RuntimeError):
    pass


_SEIZE = 0x4206
_INTERRUPT = 0x4207
_GET_EVENT = 0x4201
_GET_SYSCALL = 0x420E
_SYSCALL = 24
_CONT = 7
_OPTIONS = 0x1 | 0x2 | 0x4 | 0x8 | 0x10 | 0x40 | 0x100000
_ARCH = 0xC000003E
_X32_SYSCALL_BIT = 0x40000000
_READ_IMPLIES_EXEC = 0x0400000
_CLONE_UNTRACED = 0x00800000
_SPAWN = frozenset((56, 57, 58, 435))
_EXEC = frozenset((59, 322))
_EVENT_SPAWN = frozenset((1, 2, 3))
_STOP_SIGNALS = frozenset((signal.SIGSTOP, signal.SIGTSTP, signal.SIGTTIN, signal.SIGTTOU))
_MAX_ADDRESS = (1 << 64) - 1

_libc = ctypes.CDLL(None, use_errno=True)
_ptrace = _libc.ptrace
_ptrace.restype = ctypes.c_long
_ptrace.argtypes = (ctypes.c_uint, ctypes.c_int, ctypes.c_void_p, ctypes.c_void_p)


def _call(request, tid, address=0, data=0):
    ctypes.set_errno(0)
    result = _ptrace(request, tid, address, data)
    error = ctypes.get_errno()
    if result == -1 and error:
        raise OSError(error, os.strerror(error))
    return result


class _Entry(ctypes.Structure):
    _fields_ = [('number', ctypes.c_uint64), ('arguments', ctypes.c_uint64 * 6)]


class _Exit(ctypes.Structure):
    _fields_ = [('result', ctypes.c_int64), ('is_error', ctypes.c_uint8)]


class _Seccomp(ctypes.Structure):
    _fields_ = [('number', ctypes.c_uint64), ('arguments', ctypes.c_uint64 * 6),
                ('return_data', ctypes.c_uint32)]


class _Details(ctypes.Union):
    _fields_ = [('entry', _Entry), ('exit', _Exit), ('seccomp', _Seccomp)]


class _Info(ctypes.Structure):
    _fields_ = [('operation', ctypes.c_uint8), ('reserved', ctypes.c_uint8),
                ('flags', ctypes.c_uint16), ('architecture', ctypes.c_uint32),
                ('instruction_pointer', ctypes.c_uint64), ('stack_pointer', ctypes.c_uint64),
                ('details', _Details)]


def _info(tid):
    value = _Info()
    count = _call(_GET_SYSCALL, tid, ctypes.sizeof(value), ctypes.byref(value))
    if value.reserved or value.flags or value.architecture != _ARCH:
        raise SyscallCustodyError('Unsupported syscall metadata ABI')
    if value.operation == 1 and count >= 80:
        number = int(value.details.entry.number)
        if number & _X32_SYSCALL_BIT:
            raise SyscallCustodyError('Unsupported x32 syscall metadata ABI')
        return ('entry', number, tuple(value.details.entry.arguments))
    if value.operation == 2 and count >= 33:
        return ('exit', int(value.details.exit.result), bool(value.details.exit.is_error))
    raise SyscallCustodyError('Missing structured syscall entry/exit metadata')


def _tgid(tid, *, require_live=False):
    group, state = None, None
    with open(f'/proc/{tid}/status', encoding='ascii') as stream:
        for line in stream:
            if line.startswith('Tgid:'):
                group = int(line.split()[1])
            elif line.startswith('State:'):
                state = line.split()[1]
    if group is None or state is None:
        raise SyscallCustodyError('Tracee thread-group identity is unavailable')
    return group if not require_live or state not in ('Z', 'X', 'x') else None


def _identity(value):
    return (os.major(value.st_dev), os.minor(value.st_dev), value.st_ino)


def _maps(pid):
    result = []
    with open(f'/proc/{pid}/maps', encoding='utf-8', errors='strict') as stream:
        for line in stream:
            fields = line.split(None, 5)
            if len(fields) < 5:
                raise SyscallCustodyError('Malformed kernel mapping metadata')
            first, last = (int(part, 16) for part in fields[0].split('-'))
            major, minor = (int(part, 16) for part in fields[3].split(':'))
            result.append((first, last, fields[1], (major, minor, int(fields[4]))))
    return result


class _Thread:
    def __init__(self, tid):
        self.tid = tid
        self.stopped = False
        self.packet = None
        self.number = None
        self.arguments = ()
        self.spawn_pid = None
        self.interrupt_requested = False
        self.exiting = False


class SyscallAdmission:
    """One owned native thread group; unadmitted new processes/execs reject.

    Call attach while the native audit ACTIVATE callback is awaiting its ACK,
    then poll independently of audit socket service. The required isolated
    owner's dispatcher consumes all child statuses and retains descendant
    retirement evidence. root_status is the raw final wait status; the supervisor
    must not independently reap the root. Retained image descriptors remain
    owned by the calling supervisor.
    """
    def __init__(self, pid, executable_identities, emit=None, stop_timeout=15.0, *, owner):
        self.pid = int(pid)
        self.owner = owner
        self.identities = frozenset(tuple(value) for value in executable_identities)
        self.emit = emit
        self.stop_timeout = float(stop_timeout)
        self.threads = {}
        self.root_status = None
        self.attached = False
        self.freezing = False
        self.event_count = 0
        self.retiring = False
        self.syscall_stops = 0
        self.failure = None
        self.wakeup_fd = owner.wakeup_fd

    def drain_notifications(self):
        self.owner.drain_notifications()

    def close_notifications(self):
        if self.attached and not self.complete:
            raise SyscallCustodyError('Cannot release notifications before owned tracees cease')
        self.wakeup_fd = None

    def _record(self, kind, **values):
        event = {'kind': kind, 'monotonic_ns': time.monotonic_ns(), **values}
        self.event_count += 1
        if self.emit is not None:
            self.emit(event)

    def attach(self):
        if self.attached or not self.identities or self.stop_timeout <= 0:
            raise SyscallCustodyError('Invalid or repeated syscall-admission attach')
        tids = {int(path.name) for path in Path(f'/proc/{self.pid}/task').iterdir()}
        if tids != {self.pid}:
            raise SyscallCustodyError('Activation occurred after unobserved thread creation')
        _call(_SEIZE, self.pid, 0, _OPTIONS)
        self.attached = True
        self.threads[self.pid] = _Thread(self.pid)
        self.owner.register_trace(self.pid, self._ingest)
        _call(_INTERRUPT, self.pid)
        deadline = time.monotonic() + self.stop_timeout
        while True:
            thread = self.threads.get(self.pid)
            if thread is None or thread.exiting:
                raise SyscallCustodyError('Activation tracee ceased before admission')
            if thread.stopped:
                break
            if not self._collect_once():
                if time.monotonic() >= deadline:
                    raise SyscallCustodyError('Activation trace-stop deadline expired')
                time.sleep(.0005)
        with open(f'/proc/{self.pid}/personality', encoding='ascii') as stream:
            personality = int(stream.read().strip(), 16)
        if personality & _READ_IMPLIES_EXEC:
            raise SyscallCustodyError('READ_IMPLIES_EXEC is enabled at sealed activation')
        self._check_executable_maps()
        self._record('attached', pid=self.pid, options=_OPTIONS,
                     bootstrap='independent sealed-image audit/map admission',
                     process_memory_read=False, personality=personality,
                     read_implies_exec=False)
        self._dispatch(self.threads[self.pid])

    def _resume(self, thread, delivered_signal=0):
        _call(_SYSCALL, thread.tid, 0, delivered_signal)
        thread.stopped = False
        thread.packet = None
        thread.interrupt_requested = False

    def _collect_once(self):
        progress = self.owner.collect(stop_timeout=self.stop_timeout)
        if self.owner.rejections and not self.retiring:
            raise SyscallCustodyError('Unregistered descendant wait evidence: ' +
                                      json.dumps(self.owner.rejections, sort_keys=True))
        return progress

    def _ingest(self, tid, status):
        if self.retiring:
            self._ingest_retirement(tid, status)
            return
        thread = self.threads[tid]
        if os.WIFEXITED(status) or os.WIFSIGNALED(status):
            unresolved_spawn = thread.number in _SPAWN
            if tid == self.pid:
                self.root_status = status
            del self.threads[tid]
            self._record('ceased', tid=tid, wait_status=status,
                         returncode=os.waitstatus_to_exitcode(status))
            if unresolved_spawn:
                raise SyscallCustodyError('Tracee exited with an unresolved process-creation syscall')
            return
        if not os.WIFSTOPPED(status):
            raise SyscallCustodyError('Unexpected tracee wait state')
        thread.stopped = True
        stopped_signal = os.WSTOPSIG(status)
        event = status >> 16
        if stopped_signal == (signal.SIGTRAP | 0x80):
            self.syscall_stops += 1
            thread.packet = ('syscall', _info(tid))
        elif event in _EVENT_SPAWN:
            child = ctypes.c_ulong()
            _call(_GET_EVENT, tid, 0, ctypes.byref(child))
            child_pid = int(child.value)
            if thread.number not in _SPAWN or thread.spawn_pid is not None:
                raise SyscallCustodyError('Unmatched kernel process-creation event')
            thread.spawn_pid = child_pid
            self.threads[child_pid] = _Thread(child_pid)
            self.owner.register_trace(child_pid, self._ingest)
            group = _tgid(child_pid)
            self._record('created', parent_tid=tid, tid=child_pid, tgid=group, event=event)
            if group != self.pid:
                raise SyscallCustodyError('Unadmitted new process in the scoped native tree')
            thread.packet = ('event', event)
        elif event == 4:
            raise SyscallCustodyError('Unadmitted exec transition after sealed activation')
        elif event == 6:
            # EXIT is irrevocable, not a runnable task or a final wait witness.
            # Continue kernel exit now even while other threads are quiesced.
            thread.exiting = True
            self._record('exiting', tid=tid)
            _call(_CONT, tid)
            thread.stopped = False
            thread.packet = None
            thread.interrupt_requested = False
        elif event == 128:
            thread.packet = ('event', event)
        elif event:
            raise SyscallCustodyError('Unsupported kernel trace event')
        else:
            if stopped_signal in _STOP_SIGNALS:
                raise SyscallCustodyError('External group-stop interrupts complete execution evidence')
            thread.packet = ('signal', stopped_signal)

    @staticmethod
    def _critical(info):
        if info[0] != 'entry':
            return False
        number, arguments = info[1:]
        return ((number == 9 and not (arguments[3] & 0x20)) or
                (number in (10, 329) and arguments[2] & 4))

    def _dispatch(self, thread):
        packet = thread.packet
        if packet is None:
            raise SyscallCustodyError('Stopped thread lacks a classified event')
        if packet[0] == 'syscall':
            info = packet[1]
            if info[0] == 'entry':
                number, arguments = info[1:]
                if thread.number is not None:
                    raise SyscallCustodyError('Unresolved syscall before a new entry')
                if number == 135:
                    personality = arguments[0] & 0xffffffff
                    if personality != 0xffffffff and personality & _READ_IMPLIES_EXEC:
                        raise SyscallCustodyError('READ_IMPLIES_EXEC personality transition is unadmitted')
                if number == 56 and arguments[0] & _CLONE_UNTRACED:
                    raise SyscallCustodyError('CLONE_UNTRACED creation is unadmitted')
                if number in _EXEC or number == 134 or (number == 30 and arguments[2] & 0x8000):
                    raise SyscallCustodyError('Unadmitted executable-image syscall')
                thread.number, thread.arguments, thread.spawn_pid = number, arguments, None
                if self._critical(info):
                    self._gate(thread)
                    return
            else:
                self._finish_syscall(thread, info)
            self._resume(thread)
        elif packet[0] == 'signal':
            self._resume(thread, packet[1])
        else:
            self._resume(thread)

    def _finish_syscall(self, thread, info):
        result, is_error = info[1:]
        if thread.number in _SPAWN:
            if not is_error and result > 0 and thread.spawn_pid != result:
                raise SyscallCustodyError('Untraced or mismatched process-creation result')
            if is_error and thread.spawn_pid is not None:
                raise SyscallCustodyError('Kernel spawn event conflicts with failed syscall')
        thread.number = None
        thread.arguments = ()
        thread.spawn_pid = None

    def _check_executable_maps(self, first=None, last=None):
        observations = []
        tid = self.live_tid
        if tid is None:
            raise SyscallCustodyError('No live traced task supplies mapping evidence')
        for begin, end, permissions, identity in _maps(tid):
            overlap = first is None or begin < last and first < end
            if identity[2] and ((first is None and 'x' in permissions) or overlap and first is not None):
                if identity not in self.identities:
                    raise SyscallCustodyError('Unadmitted file-backed executable mapping')
                observations.append({'begin': begin, 'end': end, 'identity': identity})
        return observations

    def _freeze_others(self, current):
        for thread in tuple(self.threads.values()):
            if thread.tid == current.tid or thread.stopped or thread.exiting:
                continue
            try:
                _call(_INTERRUPT, thread.tid)
                thread.interrupt_requested = True
            except OSError as error:
                if error.errno not in (errno.EIO, errno.ESRCH):
                    raise
        deadline = time.monotonic() + self.stop_timeout
        while (self.owner.pending or
               any(not thread.stopped and not thread.exiting for thread in self.threads.values())):
            if not self._collect_once():
                if time.monotonic() >= deadline:
                    raise SyscallCustodyError('Thread-quiescence metadata deadline expired')
                time.sleep(.0005)
            for thread in tuple(self.threads.values()):
                if not thread.stopped and not thread.exiting and not thread.interrupt_requested:
                    try:
                        _call(_INTERRUPT, thread.tid)
                        thread.interrupt_requested = True
                    except OSError as error:
                        if error.errno not in (errno.EIO, errno.ESRCH):
                            raise
        if current.tid not in self.threads or current.exiting:
            raise SyscallCustodyError('Critical syscall owner ceased before admission')
        tid = self.live_tid
        if tid is None:
            raise SyscallCustodyError('No live traced task supplies the task census')
        actual = {int(path.name) for path in Path(f'/proc/{tid}/task').iterdir()}
        live = {thread.tid for thread in self.threads.values() if not thread.exiting}
        if not live <= actual <= set(self.threads):
            raise SyscallCustodyError('Kernel task census differs from traced thread ownership')

    def _admit_operation(self, thread):
        number, arguments = thread.number, thread.arguments
        if number == 9:
            descriptor = ctypes.c_int32(arguments[4] & 0xffffffff).value
            if descriptor < 0:
                raise SyscallCustodyError('Invalid file-backed mmap descriptor')
            metadata = os.stat(f'/proc/{thread.tid}/fd/{descriptor}')
            identity = _identity(metadata)
            executable = bool(arguments[2] & 4)
            if executable and (not stat.S_ISREG(metadata.st_mode) or identity not in self.identities):
                raise SyscallCustodyError('Unadmitted file-backed executable mmap')
            self._record('file_mmap_entry', tid=thread.tid, descriptor=descriptor,
                         size=arguments[1], protection=arguments[2], flags=arguments[3],
                         offset=arguments[5], identity=identity, executable=executable,
                         custody='sealed_image' if identity in self.identities else 'unprotected_data')
        else:
            first, size, protection = arguments[:3]
            if not size or first > _MAX_ADDRESS - size:
                raise SyscallCustodyError('Invalid executable-protection interval')
            mappings = self._check_executable_maps(first, first + size)
            self._record('execute_protection_entry', tid=thread.tid, begin=first, size=size,
                         protection=protection, admitted_file_mappings=mappings,
                         anonymous_memory='outside file-image custody claim')
        self._check_executable_maps()

    def _gate(self, initial):
        if self.freezing:
            raise SyscallCustodyError('Nested syscall-admission gate')
        self.freezing = True
        try:
            self._freeze_others(initial)
            while True:
                pending = next((thread for thread in self.threads.values()
                                if not thread.exiting and thread.packet and thread.packet[0] == 'syscall' and
                                self._critical(thread.packet[1])), None)
                if pending is None:
                    break
                info = pending.packet[1]
                if pending.number is None:
                    pending.number, pending.arguments, pending.spawn_pid = info[1], info[2], None
                self._admit_operation(pending)
                number = pending.number
                self._resume(pending)
                deadline = time.monotonic() + self.stop_timeout
                while not pending.stopped:
                    if not self._collect_once():
                        if pending.tid not in self.threads:
                            raise SyscallCustodyError('Critical syscall owner ceased without an exit witness')
                        if time.monotonic() >= deadline:
                            raise SyscallCustodyError('Critical syscall completion deadline expired')
                        time.sleep(.0005)
                packet = pending.packet
                if packet[0] != 'syscall' or packet[1][0] != 'exit':
                    raise SyscallCustodyError('Critical syscall lacks its matching exit metadata')
                self._record('file_operation_exit', tid=pending.tid, number=number,
                             result=packet[1][1], is_error=packet[1][2])
                self._finish_syscall(pending, packet[1])
                pending.packet = ('event', 128)
        finally:
            self.freezing = False
        for thread in tuple(self.threads.values()):
            if thread.stopped and not thread.exiting:
                self._dispatch(thread)

    def poll(self, limit=256):
        """Service kernel stops through the sole configured wait consumer."""
        if not self.attached:
            raise SyscallCustodyError('Syscall admission has not attached')
        try:
            for _ in range(limit):
                if not self._collect_once():
                    break
                for thread in tuple(self.threads.values()):
                    if thread.stopped and not thread.exiting:
                        self._dispatch(thread)
        except (OSError, SyscallCustodyError) as error:
            self.failure = str(error)
            self._record('rejected', reason=self.failure)
            raise
        return self.root_status

    def _retire_stopped(self):
        """Release consumed ptrace stops into kernel termination, never userspace."""
        self.retiring = True
        sent = False
        for thread in tuple(self.threads.values()):
            if not thread.stopped:
                continue
            try:
                # PTRACE_CONT's signal argument is ignored at non-delivery
                # stops. Queue real SIGKILL before releasing a kernel event.
                os.kill(thread.tid, signal.SIGKILL)
                _call(_CONT, thread.tid, 0, signal.SIGKILL)
                sent = True
                thread.stopped = False
                thread.packet = None
                thread.interrupt_requested = False
            except OSError as error:
                if error.errno != errno.ESRCH:
                    raise
        return sent

    def _ingest_retirement(self, tid, status):
        thread = self.threads[tid]
        if os.WIFEXITED(status) or os.WIFSIGNALED(status):
            if tid == self.pid:
                self.root_status = status
            del self.threads[tid]
            self._record('ceased', tid=tid, wait_status=status,
                         returncode=os.waitstatus_to_exitcode(status), cleanup=True)
            return
        if not os.WIFSTOPPED(status):
            raise SyscallCustodyError('Unexpected owned task state during teardown')
        thread.stopped = True
        event = status >> 16
        if event in _EVENT_SPAWN:
            child = ctypes.c_ulong()
            _call(_GET_EVENT, tid, 0, ctypes.byref(child))
            child_pid = int(child.value)
            if child_pid in self.threads:
                raise SyscallCustodyError('Duplicate traced child during teardown')
            self.threads[child_pid] = _Thread(child_pid)
            self.owner.register_trace(child_pid, self._ingest)
            try:
                os.kill(child_pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
        elif event == 6:
            thread.exiting = True
        try:
            os.kill(tid, signal.SIGKILL)
            _call(_CONT, tid, 0, signal.SIGKILL)
            thread.stopped = False
            thread.packet = None
            thread.interrupt_requested = False
        except OSError as error:
            if error.errno != errno.ESRCH:
                raise

    def terminate(self):
        """Retire the configured owner boundary without releasing retained images."""
        retired = self.owner.retire(self, timeout=self.stop_timeout)
        if self.owner.root_status is not None:
            self.root_status = self.owner.root_status
        if not retired or not self.complete:
            raise SyscallCustodyError('Owned descendants did not cease; retained FDs must remain open')
        return self.owner.sent_sigkill

    @property
    def live_tid(self):
        """A currently live member of the traced group, never a root fallback."""
        for thread in self.threads.values():
            if thread.exiting:
                continue
            try:
                group = _tgid(thread.tid, require_live=True)
            except (FileNotFoundError, ProcessLookupError):
                continue
            if group is None:
                continue
            if group != self.pid:
                raise SyscallCustodyError('Live mapping witness left the admitted thread group')
            return thread.tid
        return None

    @property
    def complete(self):
        return (self.root_status is not None and not self.threads and
                self.owner.complete)

    def summary(self):
        return {'scope': 'post-activation native syscall mapping and task metadata',
                'attached': self.attached, 'complete': self.complete,
                'root_wait_status': self.root_status, 'failure': self.failure,
                'syscall_stops': self.syscall_stops, 'event_count': self.event_count,
                'remaining_tids': sorted(self.threads),
                'owner_retirement': self.owner.metadata(),
                'process_memory_or_register_blob_reads': False,
                'syscall_or_register_modification': False,
                'anonymous_COW_JIT_memory_identity': 'not claimed',
                'nonexecuting_data_mapping_immutability': 'not claimed'}
