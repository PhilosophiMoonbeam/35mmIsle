#!/usr/bin/env python3
"""Private Linux/x86-64 prospective sealed ELF file-image custody.

Independent sealed-loader bootstrap witnesses precede post-activation native
syscall gates for file-backed executable mappings. Unknown processes and execs
reject; an isolated subreaper retains all images until the entire owned tree
has ceased. This is not a sandbox, whole-port qualification, or an immutability
claim for nonexecuting data, anonymous COW contents, or JIT code. The manifest
is private input; no runtime host search result is an admission authority.
"""
import argparse
import ctypes
from dataclasses import dataclass
import fcntl
import hashlib
import json
import math
import os
from pathlib import Path
import re
import resource
import selectors
import signal
import socket
import stat
import subprocess
import struct
import sys
import tempfile
import time

try:
    from .runtime_relink import digest, elf_identity
    from .runtime_custody_syscalls import SyscallAdmission, SyscallCustodyError
    from .runtime_custody_owner import declare_isolated_owner, require_isolated_owner, OwnershipError
except ImportError:
    from runtime_relink import digest, elf_identity
    from runtime_custody_syscalls import SyscallAdmission, SyscallCustodyError
    from runtime_custody_owner import declare_isolated_owner, require_isolated_owner, OwnershipError

VERSION = 1
CONFIG_MAGIC = 0x31474643454c5349
EVENT_MAGIC = 0x31545645454c5349
ACK_MAGIC = 0x314b4341454c5349
NO_IMAGE = NO_NAMESPACE = (1 << 64) - 1
CONFIG_FD, CONTROL_FD = 198, 199
CONFIG = struct.Struct('<12Q')
IMAGE = struct.Struct('<8Q')
ALIAS = struct.Struct('<3Q')
EVENT = struct.Struct('<12Q')
ACK = struct.Struct('<4Q')
ROLES = {'application': 1, 'loader': 2, 'auditor': 3, 'provider': 4}
KINDS = {1: 'activate', 2: 'search', 3: 'open', 4: 'activity',
         5: 'preinit', 6: 'close', 7: 'fatal'}
PAGE = os.sysconf('SC_PAGE_SIZE')
SEALS = 15
# Linux UAPI commands; Python need not export the build host's optional macros.
F_ADD_SEALS, F_GET_SEALS = 1033, 1034
_native_memfd_create = ctypes.CDLL(None, use_errno=True).memfd_create
_native_memfd_create.argtypes = (ctypes.c_char_p, ctypes.c_uint)
_native_memfd_create.restype = ctypes.c_int
SCOPE = {
    'coverage': 'sealed ELF loader checkpoints and post-activation file-backed execution gates',
    'continuous_mapping_qualification': False,
    'continuous_file_backed_execution_gate': True,
    'cohort_qualified': False,
    'not_qualified': ['helpers', 'compilers', 'data', 'COW contents', 'JIT code'],
    'remaining_coverage_owner': 'parent',
}


class CustodyError(ValueError):
    pass


def require(condition, message):
    if not condition:
        raise CustodyError(message)


def down(value):
    return value & -PAGE


def up(value):
    return (value + PAGE - 1) & -PAGE


def identity(info):
    return (os.major(info.st_dev), os.minor(info.st_dev), info.st_ino)


def hash_fd(fd):
    result = hashlib.sha256()
    offset = 0
    while True:
        block = os.pread(fd, 1024 * 1024, offset)
        if not block:
            return result.hexdigest()
        result.update(block)
        offset += len(block)


def write_all(fd, data):
    view = memoryview(data)
    while view:
        count = os.write(fd, view)
        require(count > 0, 'short sealed-file write')
        view = view[count:]


def durable_json(path, value):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, temporary = tempfile.mkstemp(prefix=path.name + '.', dir=path.parent)
    try:
        with os.fdopen(fd, 'w', encoding='utf-8') as stream:
            json.dump(value, stream, indent=2, ensure_ascii=True)
            stream.write('\n')
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(temporary, path)
        directory = os.open(path.parent, os.O_RDONLY | os.O_DIRECTORY)
        try:
            os.fsync(directory)
        finally:
            os.close(directory)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)


def high_fd(fd):
    try:
        return fcntl.fcntl(fd, fcntl.F_DUPFD_CLOEXEC, 200)
    finally:
        os.close(fd)


def memfd(name):
    # Call the real glibc ABI even when Python was built without os.memfd_create.
    # MFD_CLOEXEC=1, MFD_ALLOW_SEALING=2, MFD_EXEC=16; never retry weaker flags.
    ctypes.set_errno(0)
    descriptor = _native_memfd_create(os.fsencode(name), 1 | 2 | 16)
    if descriptor < 0:
        error = ctypes.get_errno()
        raise OSError(error, os.strerror(error))
    return high_fd(descriptor)


@dataclass
class StagedImage:
    index: int
    role: str
    original: str
    aliases: list
    fd: int
    path: str
    key: tuple
    size: int
    sha256: str
    elf: dict

    def receipt(self):
        return {'index': self.index, 'role': self.role, 'original_path': self.original,
                'aliases': self.aliases, 'supervisor_fd': self.fd,
                'sealed_path': self.path, 'device_major': self.key[0],
                'device_minor': self.key[1], 'inode': self.key[2], 'size': self.size,
                'sha256': self.sha256, 'seals': fcntl.fcntl(self.fd, F_GET_SEALS),
                'elf': self.elf}


@dataclass(frozen=True)
class Mapping:
    start: int
    end: int
    permissions: str
    offset: int
    key: tuple
    name: str


def parse_maps(text):
    result = []
    for line in text.splitlines():
        fields = line.split(None, 5)
        require(len(fields) >= 5, 'malformed proc maps record')
        bounds, permissions, offset, device, inode = fields[:5]
        start, end = (int(value, 16) for value in bounds.split('-'))
        major, minor = (int(value, 16) for value in device.split(':'))
        require(start < end and len(permissions) == 4, 'invalid proc maps range')
        result.append(Mapping(start, end, permissions, int(offset, 16),
                              (major, minor, int(inode)), fields[5] if len(fields) > 5 else ''))
    require(result, 'empty proc maps')
    return result


def covered(start, end, intervals):
    cursor = start
    for left, right in sorted(intervals):
        if left > cursor:
            break
        cursor = max(cursor, right)
        if cursor >= end:
            return True
    return cursor >= end


def permissions_fit(mapping, segment):
    flags = segment['flags']
    return all(permission not in mapping.permissions or flag in flags
               for permission, flag in (('r', 'R'), ('w', 'W'), ('x', 'E')))


def file_intervals(image, mapping, bias):
    intervals = []
    for segment in image.elf['segments']:
        if segment['type'] != 'LOAD' or not segment['filesz']:
            continue
        left = bias + down(segment['vaddr'])
        right = bias + up(segment['vaddr'] + segment['filesz'])
        if (mapping.start - mapping.offset != left - down(segment['offset'])
                or not permissions_fit(mapping, segment)):
            continue
        intervals.append((max(mapping.start, left), min(mapping.end, right)))
    return [(left, right) for left, right in intervals if left < right]


def row_biases(image, mapping):
    candidates = set()
    for segment in image.elf['segments']:
        if segment['type'] != 'LOAD' or not segment['filesz']:
            continue
        bias = mapping.start - mapping.offset - down(segment['vaddr']) + down(segment['offset'])
        if image.elf['type'] == 'EXEC' and bias != 0:
            continue
        if bias >= 0 and covered(mapping.start, mapping.end, file_intervals(image, mapping, bias)):
            candidates.add(bias)
    return candidates


def complete_image(image, bias, mappings):
    for segment in image.elf['segments']:
        if segment['type'] != 'LOAD' or not segment['memsz']:
            continue
        start = bias + down(segment['vaddr'])
        file_end = bias + up(segment['vaddr'] + segment['filesz']) if segment['filesz'] else start
        end = bias + up(segment['vaddr'] + segment['memsz'])
        intervals = []
        for mapping in mappings:
            if mapping.key == image.key:
                intervals.extend(file_intervals(image, mapping, bias))
            elif not mapping.key[2] and permissions_fit(mapping, segment):
                intervals.append((max(file_end, mapping.start), min(end, mapping.end)))
        if not covered(start, end, [(left, right) for left, right in intervals if left < right]):
            return False
    return True


class Supervisor:
    """Retain sealed images until a dedicated owner's entire tree is retired.

    Direct use requires declare_isolated_owner() in an already dedicated
    executable. Embedding callers use launch(), which isolates the CLI worker.
    prepare()/run() remain split for native sealed-image substitution tests.
    """
    def __init__(self, manifest_path, record_path, timeout, arguments, audit_module=None):
        self.owner = require_isolated_owner()
        self.manifest_path = Path(manifest_path).absolute()
        self.record_path = Path(record_path).absolute()
        require(math.isfinite(timeout) and timeout > 0, 'timeout must be finite and positive')
        self.timeout = timeout
        self.arguments = list(arguments)
        self.audit_module = audit_module
        self.images = []
        self.aliases = {}
        self.owned_fds = set()
        self.config_fd = None
        self.parent_socket = self.child_socket = None
        self.pid = None
        self.reaped = False
        self.wait_status = None
        self.prepared = False
        self.activated = self.preinit = False
        self.serial = self.object_serial = 0
        self.objects = {}
        self.counts = {value: 0 for value in KINDS.values()}
        self.receipt = {'schema_version': 1, 'private_only': True, 'scope': dict(SCOPE),
                        'status': 'preparing', 'admitted': False,
                        'supervisor_pid': os.getpid(), 'manifest': str(self.manifest_path),
                        'arguments': self.arguments, 'timeout_seconds': timeout,
                        'started_unix_ns': time.time_ns(), 'retained_fds_closed': False,
                        'supervisor_sent_sigkill': False, 'owned_tree_retired': False,
                        'retirement_incomplete': False}
        self.journal = None
        self.syscalls = None
        self.syscall_journal = None
        self.syscall_journal_hash = hashlib.sha256()
        self.streams = {}
        self.retirement_failure = None
        self.owner.begin_launch()
        self.receipt['ownership'] = self.owner.metadata()

    def __enter__(self):
        return self

    def __exit__(self, kind, value, traceback):
        self.close()

    def own(self, fd):
        self.owned_fds.add(fd)
        return fd

    def validate_elf(self, image):
        metadata = image.elf
        require(metadata['class'] == 'ELF64' and "little endian" in metadata['data']
                and metadata['machine'] == 'Advanced Micro Devices X86-64',
                'image is not native little-endian ELF64 x86-64: ' + image.original)
        require(metadata['type'] in ('DYN', 'EXEC'), 'unsupported ELF type')
        require(metadata['type'] == 'DYN' or image.role == 'application',
                'non-application image must be ET_DYN')
        loads = [segment for segment in metadata['segments'] if segment['type'] == 'LOAD']
        require(loads and any('E' in segment['flags'] for segment in loads), 'missing executable PT_LOAD')
        for segment in loads:
            require(segment['filesz'] <= segment['memsz']
                    and segment['offset'] + segment['filesz'] <= image.size
                    and segment['vaddr'] + segment['memsz'] < 1 << 64
                    and segment['offset'] % PAGE == segment['vaddr'] % PAGE,
                    'invalid PT_LOAD extent/alignment')
            alignment = segment['align']
            require(alignment in (0, 1) or (alignment & (alignment - 1) == 0
                    and segment['vaddr'] % alignment == segment['offset'] % alignment),
                    'invalid PT_LOAD p_align')
            require(not ('W' in segment['flags'] and 'E' in segment['flags']),
                    'writable executable PT_LOAD is unsupported')
        if image.role in ('application', 'loader'):
            require(any('E' in segment['flags'] and segment['vaddr'] <= metadata['entry']
                        < segment['vaddr'] + segment['filesz'] for segment in loads),
                    'entry point is outside an executable file-backed PT_LOAD')
        require(not any(segment['type'] == 'GNU_STACK' and 'E' in segment['flags']
                        for segment in metadata['segments']), 'executable stack is unsupported')
        supported_tags = {
            'NEEDED', 'PLTRELSZ', 'PLTGOT', 'HASH', 'GNU_HASH', 'STRTAB', 'SYMTAB',
            'RELA', 'RELASZ', 'RELAENT', 'STRSZ', 'SYMENT', 'INIT', 'FINI', 'SONAME',
            'RPATH', 'RUNPATH', 'SYMBOLIC', 'REL', 'RELSZ', 'RELENT', 'PLTREL',
            'DEBUG', 'JMPREL', 'BIND_NOW', 'INIT_ARRAY', 'FINI_ARRAY', 'INIT_ARRAYSZ',
            'FINI_ARRAYSZ', 'PREINIT_ARRAY', 'PREINIT_ARRAYSZ', 'FLAGS', 'FLAGS_1',
            'SYMTAB_SHNDX', 'RELRSZ', 'RELR', 'RELRENT', 'VERDEF', 'VERDEFNUM',
            'VERNEED', 'VERNEEDNUM', 'VERSYM', 'RELACOUNT', 'RELCOUNT', 'NULL',
            'TLSDESC_PLT', 'TLSDESC_GOT',
            # x86-64 psABI PLT addresses/sizes; these do not select providers.
            'DT_X86_64_PLT', 'DT_X86_64_PLTSZ', 'DT_X86_64_PLTENT',
        }
        # Audit/filter/config/group/unknown extensions are not implicitly
        # supported merely because the host loader happens to implement them.
        for entry in metadata['dynamic_tags']:
            require(entry['tag'] in supported_tags, 'unsupported loader directive DT_' + entry['tag'])
            if entry['tag'] == 'FLAGS_1':
                flags = entry['value'].removeprefix('Flags:').split()
                require(set(flags) <= {'NOW', 'ORIGIN', 'NODELETE', 'NODEFLIB', 'PIE'},
                        'unsupported DT_FLAGS_1: ' + entry['value'])
            if entry['tag'] == 'FLAGS':
                flags = entry['value'].split()
                require(set(flags) <= {'ORIGIN', 'SYMBOLIC', 'BIND_NOW', 'STATIC_TLS'},
                        'unsupported DT_FLAGS: ' + entry['value'])
        # PT_INTERP governs exec, not DSO loading. prepare() binds the
        # application's interpreter to the independently admitted loader.
        if image.role == 'auditor':
            require(not metadata['dynamic']['needed'], 'audit module must have no DT_NEEDED')

    def prepare(self):
        require(not self.prepared and not self.images, 'supervisor already prepared')
        require(sys.platform == 'linux' and os.uname().machine == 'x86_64',
                'custody requires native Linux x86-64')
        manifest_bytes = self.manifest_path.read_bytes()
        manifest = json.loads(manifest_bytes.decode('utf-8'))
        require(isinstance(manifest, dict) and type(manifest.get('schema_version')) is int
                and manifest['schema_version'] == 1,
                'unsupported private manifest schema')
        require(set(manifest) == {'schema_version', 'images', 'environment', 'cwd'},
                'manifest fields must be schema_version/images/environment/cwd')
        images = manifest['images']
        require(isinstance(images, list) and 3 <= len(images) <= 256, 'invalid image count')
        environment = manifest['environment']
        require(isinstance(environment, dict) and all(isinstance(key, str) and key and '=' not in key
                and '\0' not in key and isinstance(value, str) and '\0' not in value
                for key, value in environment.items()), 'environment must be an explicit string dictionary')
        # Do not inherit the launcher's shell. GUI selection (DISPLAY, WAYLAND_DISPLAY,
        # XAUTHORITY, XDG_RUNTIME_DIR, etc.) survives only when explicitly declared.
        removed = sorted(key for key in environment if key.startswith('LD_') or key in (
            'GLIBC_TUNABLES', 'GCONV_PATH', 'LOCPATH', 'MALLOC_TRACE'))
        self.environment = {key: value for key, value in environment.items() if key not in removed}
        self.cwd = manifest['cwd']
        require(isinstance(self.cwd, str) and Path(self.cwd).is_absolute()
                and Path(self.cwd).is_dir() and '\0' not in self.cwd, 'cwd must be an existing absolute directory')
        require(self.arguments and all(isinstance(arg, str) and '\0' not in arg for arg in self.arguments),
                'actual target argv is required')
        roles = {role: [] for role in ROLES}
        originals = set()
        for index, entry in enumerate(images):
            require(isinstance(entry, dict) and set(entry) == {
                'role', 'path', 'expected_sha256', 'size', 'aliases'}, 'invalid image manifest fields')
            role, path, expected, size, aliases = (entry[key] for key in (
                'role', 'path', 'expected_sha256', 'size', 'aliases'))
            require(isinstance(role, str) and role in ROLES, 'unknown image role')
            require(isinstance(path, str) and Path(path).is_absolute() and '\0' not in path,
                    'image path must be absolute')
            require(path not in originals, 'image path has multiple roles/entries')
            originals.add(path)
            require(isinstance(expected, str) and re.fullmatch('[0-9a-f]{64}', expected),
                    'invalid expected_sha256')
            require(type(size) is int and size > 0, 'invalid expected image size')
            require(isinstance(aliases, list)
                    and all(isinstance(alias, str) and alias and '\0' not in alias
                            and len(os.fsencode(alias)) <= 4096 for alias in aliases)
                    and len(aliases) == len(set(aliases)), 'invalid exact aliases')
            fd = self.own(memfd('isle-custody-' + role))
            source = os.open(path, os.O_RDONLY | os.O_CLOEXEC | os.O_NONBLOCK)
            try:
                before = os.fstat(source)
                require(stat.S_ISREG(before.st_mode) and before.st_size == size,
                        'source is not the declared whole regular file')
                offset = 0
                while offset < size:
                    block = os.pread(source, min(1024 * 1024, size - offset), offset)
                    require(block, 'source truncated during staging')
                    write_all(fd, block)
                    offset += len(block)
                require(not os.pread(source, 1, size), 'source grew during staging')
            finally:
                os.close(source)
            fcntl.fcntl(fd, F_ADD_SEALS, SEALS)
            info = os.fstat(fd)
            actual = hash_fd(fd)
            require(info.st_size == size and actual == expected
                    and fcntl.fcntl(fd, F_GET_SEALS) == SEALS,
                    'sealed whole-file digest/size/seals mismatch: ' + path)
            fd_path = '/proc/' + str(os.getpid()) + '/fd/' + str(fd)
            image = StagedImage(index, role, path, list(aliases), fd, fd_path,
                                identity(info), size, actual, elf_identity(fd_path))
            self.validate_elf(image)
            self.images.append(image)
            roles[role].append(index)
            for alias in [*aliases, fd_path]:
                require(alias not in self.aliases, 'ambiguous exact alias: ' + alias)
                self.aliases[alias] = index
        require(all(len(roles[role]) == 1 for role in ('application', 'loader', 'auditor')),
                'exactly one application, loader and auditor are required')
        self.application, self.loader, self.auditor = (self.images[roles[role][0]]
                                                      for role in ('application', 'loader', 'auditor'))
        require(self.arguments[0] == self.application.original, 'argv[0] must equal the manifest application path')
        if self.audit_module is not None:
            require(str(Path(self.audit_module).absolute()) == self.auditor.original,
                    '--audit-module differs from the manifest auditor')
        require(self.application.elf['interpreters'] == [self.loader.original],
                'application PT_INTERP must be the declared loader path')
        require(self.aliases.get(self.loader.original) == self.loader.index,
                'loader PT_INTERP must be an explicitly declared exact alias')
        for image in self.images:
            for needed in image.elf['dynamic']['needed']:
                require(needed in self.aliases, 'missing transitive exact alias: ' + needed)
                require(self.images[self.aliases[needed]].role in ('provider', 'loader'),
                        'DT_NEEDED alias selects a non-provider bootstrap image')
        self.receipt.update({'manifest_sha256': hashlib.sha256(manifest_bytes).hexdigest(),
                             'cwd': self.cwd, 'environment': self.environment,
                             'removed_loader_environment': removed,
                             'images': [image.receipt() for image in self.images]})
        self.make_config()
        self.parent_socket, self.child_socket = socket.socketpair(socket.AF_UNIX, socket.SOCK_SEQPACKET)
        child_fd = high_fd(self.child_socket.detach())
        self.child_socket = socket.socket(fileno=child_fd)
        self.record_path.parent.mkdir(parents=True, exist_ok=True)
        self.journal_path = Path(str(self.record_path) + '.events.jsonl')
        self.journal = self.journal_path.open('wb')
        self.prepared = True
        self.receipt['status'] = 'prepared'
        durable_json(self.record_path, self.receipt)
        return self

    def make_config(self):
        require(len(self.aliases) <= 2048, 'too many aliases')
        sorted_aliases = sorted(self.aliases, key=os.fsencode)
        images_offset = CONFIG.size
        aliases_offset = images_offset + IMAGE.size * len(self.images)
        strings_offset = aliases_offset + ALIAS.size * len(sorted_aliases)
        strings = bytearray()
        def string(value):
            encoded = os.fsencode(value)
            result = strings_offset + len(strings), len(encoded)
            strings.extend(encoded + b'\0')
            return result
        image_table = bytearray()
        for image in self.images:
            offset, size = string(image.path)
            image_table.extend(IMAGE.pack(offset, size, ROLES[image.role], image.fd,
                                          *image.key, image.size))
        alias_table = bytearray()
        for alias in sorted_aliases:
            offset, size = string(alias)
            alias_table.extend(ALIAS.pack(offset, size, self.aliases[alias]))
        total = strings_offset + len(strings)
        require(total <= 1048576, 'configuration exceeds protocol limit')
        data = CONFIG.pack(CONFIG_MAGIC, VERSION, total, os.getpid(), len(self.images),
                           len(sorted_aliases), images_offset, aliases_offset, strings_offset,
                           self.application.index, self.loader.index, self.auditor.index)
        self.config_fd = self.own(memfd('isle-custody-config'))
        write_all(self.config_fd, data + image_table + alias_table + strings)
        fcntl.fcntl(self.config_fd, F_ADD_SEALS, SEALS)
        # Hand the child an O_RDONLY open description, not the staging write FD.
        readonly = self.own(high_fd(os.open('/proc/self/fd/' + str(self.config_fd), os.O_RDONLY | os.O_CLOEXEC)))
        self.owned_fds.remove(self.config_fd)
        os.close(self.config_fd)
        self.config_fd = readonly
        self.config_key = identity(os.fstat(readonly))
        self.config_size = total
        self.receipt['config'] = {'device_major': self.config_key[0], 'device_minor': self.config_key[1],
                                  'inode': self.config_key[2], 'size': total,
                                  'sha256': hash_fd(readonly), 'seals': fcntl.fcntl(readonly, F_GET_SEALS),
                                  'child_fd': CONFIG_FD, 'access': 'read-only'}

    def snapshot(self, full=False, opened=None, vdso_bias=None):
        live_tid = self.syscalls.live_tid if self.syscalls is not None and self.syscalls.attached else self.pid
        require(live_tid is not None, 'no verified live target task for bootstrap/checkpoint metadata')
        proc = '/proc/' + str(live_tid)
        require(identity(os.stat(proc + '/exe')) == self.loader.key,
                'actual /proc/live_tid/exe is not the sealed loader inode')
        text = Path(proc + '/maps').read_text(encoding='utf-8')
        mappings = parse_maps(text)
        known = {image.key: image for image in self.images}
        present = set()
        biases = {image.index: set() for image in self.images}
        config_intervals = []
        witnessed_mappings = []
        unprotected_data_mappings = []
        anonymous_executable_mappings = []
        for mapping in mappings:
            if mapping.key == self.config_key:
                require(mapping.permissions == 'r--s' and mapping.offset + mapping.end - mapping.start <= up(self.config_size),
                        'configuration is not an exact read-only sealed mapping')
                config_intervals.append((mapping.offset, mapping.offset + mapping.end - mapping.start))
            elif mapping.key[2]:
                if mapping.key not in known:
                    require('x' not in mapping.permissions,
                            'unadmitted executable mapped inode: ' + mapping.name)
                    unprotected_data_mappings.append({
                        'device_major': mapping.key[0], 'device_minor': mapping.key[1],
                        'inode': mapping.key[2], 'start': mapping.start, 'end': mapping.end,
                        'offset': mapping.offset, 'permissions': mapping.permissions,
                        'name': mapping.name, 'immutable_custody': False})
                    continue
                image = known[mapping.key]
                candidates = row_biases(image, mapping)
                require(candidates, 'mapping dev/inode/offset/permissions not covered by PT_LOAD: ' + mapping.name)
                present.add(image.index)
                biases[image.index].update(candidates)
                witnessed_mappings.append({
                    'image_index': image.index, 'device_major': mapping.key[0],
                    'device_minor': mapping.key[1], 'inode': mapping.key[2],
                    'start': mapping.start, 'end': mapping.end, 'offset': mapping.offset,
                    'permissions': mapping.permissions, 'candidate_load_biases': sorted(candidates)})
            elif 'x' in mapping.permissions:
                anonymous_executable_mappings.append({
                    'start': mapping.start, 'end': mapping.end,
                    'permissions': mapping.permissions, 'name': mapping.name,
                    'immutable_custody': False,
                    'scope': 'kernel mapping' if mapping.name in ('[vdso]', '[vsyscall]')
                             else 'anonymous COW/JIT outside file-image custody'})
        complete = {image.index: sorted(bias for bias in biases[image.index]
                                        if complete_image(image, bias, mappings)) for image in self.images}
        if vdso_bias is not None:
            require(any(mapping.name == '[vdso]' and mapping.key[2] == 0
                        and mapping.start == vdso_bias and 'r' in mapping.permissions
                        and 'x' in mapping.permissions and 'w' not in mapping.permissions
                        for mapping in mappings), 'OPEN vDSO bias lacks a kernel mapping witness')
        require(complete[self.loader.index] and complete[self.auditor.index]
                and covered(0, up(self.config_size), config_intervals),
                'missing independent loader/auditor/config bootstrap mapping witness')
        if full:
            require(complete[self.application.index], 'missing independent application PT_LOAD witness')
        if opened is not None:
            image_index, bias = opened
            require(bias in complete[image_index], 'OPEN load bias lacks complete actual PT_LOAD witness')
        return {'maps_sha256': hashlib.sha256(text.encode()).hexdigest(),
                'metadata_live_tid': live_tid,
                'mapped_image_indices': sorted(present),
                'mapped_segments': witnessed_mappings,
                'unprotected_data_mappings': unprotected_data_mappings,
                'anonymous_executable_mappings': anonymous_executable_mappings,
                'complete_load_biases': {str(index): values for index, values in complete.items() if values},
                'loader_exe_identity': list(self.loader.key), 'config_readonly_witness': True,
                'application_witness': bool(complete[self.application.index])}

    def syscall_event(self, record):
        packet = (json.dumps(record, ensure_ascii=True, sort_keys=True) + '\n').encode()
        self.syscall_journal.write(packet)
        self.syscall_journal.flush()
        os.fsync(self.syscall_journal.fileno())
        self.syscall_journal_hash.update(packet)

    def event(self, packet):
        require(len(packet) >= EVENT.size, 'short control event')
        values = EVENT.unpack_from(packet)
        magic, version, kind, serial, pid, tid, namespace, index, bias, flags, cookie, length = values
        require(magic == EVENT_MAGIC and version == VERSION and kind in KINDS,
                'invalid event magic/version/kind')
        require(serial == self.serial + 1, 'event serial gap/overflow')
        live_tid = self.syscalls.live_tid if self.syscalls is not None and self.syscalls.attached else self.pid
        require(pid == self.pid and live_tid is not None and tid > 0 and (kind == 7
                or Path('/proc/' + str(live_tid) + '/task/' + str(tid)).exists()),
                'event process/thread identity mismatch')
        require(length <= 4096 and len(packet) == EVENT.size + length and b'\0' not in packet[EVENT.size:],
                'event name length/record mismatch')
        name = os.fsdecode(packet[EVENT.size:])
        require(index == NO_IMAGE or index < len(self.images), 'event image index out of range')
        require(kind == 7 or ((kind == 1) == (not self.activated)), 'missing/repeated activation')
        record = {'serial': serial, 'kind': KINDS[kind], 'pid': pid, 'tid': tid,
                  'namespace_id': namespace, 'image_index': index, 'load_bias': bias,
                  'flags': flags, 'cookie': cookie, 'name': name}
        if kind == 7:
            raise CustodyError('native audit fatal: ' + name + ' (reason ' + str(flags) + ')')
        opened = None
        if kind == 1:
            require(index == self.auditor.index and namespace == NO_NAMESPACE and cookie == 0
                    and bias == 0 and not name and flags in (1, 2), 'malformed activation')
        elif kind == 2:
            require(namespace == NO_NAMESPACE and bias == 0 and self.aliases.get(name) == index
                    and index != NO_IMAGE and flags > 0 and not (flags & ~207),
                    'SEARCH does not name an exact approved alias')
        elif kind == 3:
            require(namespace != NO_NAMESPACE and cookie >> 9 == self.object_serial + 1 and flags == 0,
                    'invalid OPEN namespace/object serial')
            if index == NO_IMAGE:
                require(name == 'linux-vdso.so.1' and cookie & 511 == 0, 'invalid kernel-vdso object')
            else:
                require(cookie & 511 == index + 1 and (self.aliases.get(name) == index
                        or (not name and index == self.application.index and namespace == 0)),
                        'OPEN name/cookie does not select the admitted image')
                opened = index, bias
            require(cookie not in self.objects, 'duplicate live object cookie')
        elif kind == 4:
            require(namespace == NO_NAMESPACE and bias == 0 and index == NO_IMAGE
                    and not name and flags in (0, 1, 2), 'invalid loader activity')
        elif kind == 5:
            require(namespace == NO_NAMESPACE and bias == 0 and index == NO_IMAGE
                    and not name and flags == 0 and not self.preinit,
                    'invalid/repeated preinit')
            require(any(value[0] == self.application.index and value[1] == 0
                        for value in self.objects.values()), 'preinit lacks base application OPEN')
        elif kind == 6:
            require(namespace == NO_NAMESPACE and bias == 0 and not name and flags == 0
                    and cookie in self.objects and self.objects[cookie][0] == index,
                    'CLOSE unknown/mismatched object cookie')
        record['checkpoint'] = self.snapshot(full=kind == 5 or self.preinit, opened=opened,
                                             vdso_bias=bias if kind == 3 and index == NO_IMAGE else None)
        if kind == 1:
            self.syscall_journal_path = Path(str(self.record_path) + '.syscalls.jsonl')
            self.syscall_journal = self.syscall_journal_path.open('wb')
            self.syscalls = SyscallAdmission(self.pid, [image.key for image in self.images],
                                            emit=self.syscall_event, owner=self.owner)
            self.syscalls.attach()
        record['accepted'] = True
        self.journal.write((json.dumps(record, ensure_ascii=True, sort_keys=True) + '\n').encode())
        self.journal.flush()
        os.fsync(self.journal.fileno())
        # Publish the durable checkpoint before allowing the loader to proceed.
        self.parent_socket.sendall(ACK.pack(ACK_MAGIC, VERSION, serial, 1))
        self.serial = serial
        self.counts[KINDS[kind]] += 1
        if kind == 1:
            self.activated = True
        elif kind == 3:
            self.object_serial += 1
            self.objects[cookie] = (index, namespace, bias)
        elif kind == 5:
            self.preinit = True
        elif kind == 6:
            del self.objects[cookie]

    def spawn(self):
        self.owner.ready_to_spawn()
        pipes = {}
        for name in ('stdout', 'stderr', 'handoff'):
            read_fd, write_fd = os.pipe2(os.O_CLOEXEC)
            pipes[name] = self.own(high_fd(read_fd)), self.own(high_fd(write_fd))
        null = self.own(high_fd(os.open(os.devnull, os.O_RDONLY | os.O_CLOEXEC)))
        argv = [self.loader.path, '--inhibit-cache', '--audit', self.auditor.path,
                self.application.path, *self.arguments[1:]]
        self.receipt['loader_argv'] = argv
        # All sources are >=200, so dup2 cannot trample either reserved target FD.
        # Never reserve/replace 198/199 in the supervisor's shared descriptor table.
        close_limit = resource.getrlimit(resource.RLIMIT_NOFILE)[0]
        require(close_limit != resource.RLIM_INFINITY, 'finite FD limit required for safe handoff')
        pid = os.fork()
        if pid == 0:
            try:
                self.owner.restore_child_signals()
                os.setsid()
                os.chdir(self.cwd)
                os.dup2(null, 0)
                os.dup2(pipes['stdout'][1], 1)
                os.dup2(pipes['stderr'][1], 2)
                os.dup2(self.config_fd, CONFIG_FD)
                os.dup2(self.child_socket.fileno(), CONTROL_FD)
                error_fd = pipes['handoff'][1]
                # Keep just the CLOEXEC error channel in addition to stdio/config/control.
                for left, right in ((3, CONFIG_FD), (CONTROL_FD + 1, error_fd),
                                    (error_fd + 1, int(close_limit))):
                    if left < right:
                        os.closerange(left, right)
                os.execve(self.loader.path, argv, self.environment)
            except BaseException as error:
                try:
                    os.write(pipes['handoff'][1], (type(error).__name__ + ': ' + str(error)).encode()[:4096])
                finally:
                    os._exit(126)
        self.pid = pid
        self.owner.set_root(pid)
        self.child_socket.close()
        self.child_socket = None
        for read_fd, write_fd in pipes.values():
            os.close(write_fd)
            self.owned_fds.remove(write_fd)
            os.set_blocking(read_fd, False)
        os.close(null)
        self.owned_fds.remove(null)
        self.receipt.update({'pid': pid, 'status': 'running'})
        durable_json(self.record_path, self.receipt)
        return pipes

    def reap(self, blocking=False):
        while True:
            if self.syscalls is not None and self.syscalls.attached:
                self.syscalls.poll()
            else:
                self.owner.collect()
            if self.owner.root_status is not None:
                self.reaped = True
                self.wait_status = self.owner.root_status
            require(not self.owner.rejections, 'unregistered task in the owned native tree')
            if not blocking or self.reaped or not self.pid:
                return
            time.sleep(.001)

    def stop(self, grace=0):
        deadline = time.monotonic() + grace
        while not self.owner.complete and time.monotonic() < deadline:
            try:
                self.reap()
            except (CustodyError, SyscallCustodyError, OwnershipError, OSError):
                break
            time.sleep(.0005)
        while not self.owner.retire(self.syscalls, timeout=5.0):
            # A deadline is evidence of rejection, never permission to release
            # seals or exit this owner while a descendant can still execute.
            self.retirement_failure = 'Owned-tree retirement deadline exceeded; custody retained until cessation'
            self.receipt.update({'status': 'rejected', 'admitted': False,
                                 'failure': self.receipt.get('failure') or self.retirement_failure,
                                 'owned_tree_retired': False, 'retirement_incomplete': True,
                                 'retained_fds_closed': False, 'ownership': self.owner.metadata()})
            try:
                durable_json(self.record_path, self.receipt)
            except OSError as error:
                # Failed receipt I/O must not turn into premature owner exit.
                self.receipt['retirement_receipt_error'] = str(error)
                print('runtime custody: cannot publish incomplete retirement receipt; '
                      'retaining custody: ' + str(error), file=sys.stderr)
        if self.owner.root_status is not None:
            self.reaped = True
            self.wait_status = self.owner.root_status
        self.receipt.update({'owned_tree_retired': True, 'retirement_incomplete': False,
                             'ownership': self.owner.metadata()})
        self.receipt['supervisor_sent_sigkill'] |= self.owner.sent_sigkill

    def run(self):
        failure = None
        selector = selectors.DefaultSelector()
        handoff = bytearray()
        channel_eof = None
        rejected_event = False
        pipes = {}
        syscall_registered = False
        deadline = time.monotonic() + self.timeout
        try:
            if not self.prepared:
                self.prepare()
            pipes = self.spawn()
            for name, (read_fd, _) in pipes.items():
                selector.register(read_fd, selectors.EVENT_READ, name)
                if name != 'handoff':
                    path = Path(str(self.record_path) + '.' + name)
                    self.streams[name] = {'stream': path.open('wb', buffering=0), 'path': path, 'size': 0,
                                          'hash': hashlib.sha256(), 'preview': bytearray(), 'fully_drained': False}
            selector.register(self.parent_socket, selectors.EVENT_READ, 'control')
            while True:
                self.reap()
                if self.syscalls is not None and self.syscalls.attached:
                    if self.syscalls.complete and syscall_registered:
                        selector.unregister(self.syscalls.wakeup_fd)
                        syscall_registered = False
                    elif not self.syscalls.complete and not syscall_registered:
                        selector.register(self.syscalls.wakeup_fd, selectors.EVENT_READ, 'syscalls')
                        syscall_registered = True
                now = time.monotonic()
                require(now < deadline, 'target timeout')
                if channel_eof is not None and not self.reaped:
                    require(now - channel_eof < 0.25, 'source control channel closed while target remained alive')
                if self.reaped and not selector.get_map():
                    break
                for key, _ in selector.select(min(0.05, max(0, deadline - now))):
                    if key.data == 'control':
                        packet, ancillary, flags, address = self.parent_socket.recvmsg(EVENT.size + 4096, 0)
                        require(not ancillary and not flags & (socket.MSG_TRUNC | socket.MSG_CTRUNC),
                                'truncated/ancillary control packet')
                        if not packet:
                            selector.unregister(key.fileobj)
                            channel_eof = time.monotonic()
                            require(self.activated and self.preinit, 'source channel EOF before activation/preinit')
                        else:
                            try:
                                self.event(packet)
                            except (CustodyError, SyscallCustodyError, OSError) as error:
                                rejected_event = True
                                self.journal.write((json.dumps({
                                    'accepted': False, 'failure': str(error),
                                    'source_packet_hex': packet.hex()}, sort_keys=True) + '\n').encode())
                                self.journal.flush()
                                os.fsync(self.journal.fileno())
                                if len(packet) >= EVENT.size:
                                    serial = EVENT.unpack_from(packet)[3]
                                    try:
                                        self.parent_socket.sendall(ACK.pack(ACK_MAGIC, VERSION, serial, 0))
                                    except OSError:
                                        pass
                                raise error
                    elif key.data == 'syscalls':
                        self.syscalls.drain_notifications()
                        self.reap()
                    else:
                        block = os.read(key.fd, 65536)
                        if not block:
                            selector.unregister(key.fd)
                            os.close(key.fd)
                            self.owned_fds.remove(key.fd)
                            if key.data in self.streams:
                                self.streams[key.data]['fully_drained'] = True
                        elif key.data == 'handoff':
                            handoff.extend(block)
                            require(len(handoff) <= 4096, 'handoff diagnostics overflow')
                            raise CustodyError('native exec handoff failed: ' + handoff.decode(errors='replace'))
                        else:
                            self.capture(key.data, block)
                # A reaped child cannot legitimately leave inherited channels live.
                if self.reaped and selector.get_map() and time.monotonic() + 0.1 >= deadline:
                    raise CustodyError('channels retained after target reaping')
            require(self.activated and self.preinit, 'target exited without activation/preinit admission')
            require(self.syscalls is not None and self.syscalls.complete
                    and self.syscalls.failure is None, 'incomplete kernel execution custody')
            require(os.WIFEXITED(self.wait_status), 'target terminated by signal')
            require(os.WEXITSTATUS(self.wait_status) != 126, 'native fail-closed exit 126')
        except (Exception, KeyboardInterrupt) as error:
            failure = type(error).__name__ + ': ' + str(error)
        finally:
            selector.close()
            self.stop(grace=0.25 if rejected_event else 0)
            failure = failure or self.retirement_failure
            if self.owner.rejections:
                failure = failure or 'Unregistered or rejected owned task: ' + self.owner.rejections[0]['reason']
            # On rejection/timeout, capture all queued output after reaping too.
            # If an inherited descendant channel has not reached EOF, say so.
            for name, (read_fd, _) in pipes.items():
                if name not in self.streams or read_fd not in self.owned_fds:
                    continue
                while True:
                    try:
                        block = os.read(read_fd, 65536)
                    except BlockingIOError:
                        break
                    if not block:
                        self.streams[name]['fully_drained'] = True
                        break
                    self.capture(name, block)
            self.receipt.update({'status': 'rejected' if failure else 'completed',
                                 'failure': failure, 'activated': self.activated, 'preinit': self.preinit,
                                 'event_count': self.serial, 'event_counts': self.counts,
                                 'admitted': failure is None and self.activated and self.preinit,
                                 'target_reaped': self.reaped,
                                 'control_channel_eof': channel_eof is not None,
                                 'handoff_error': handoff.decode(errors='replace') or None,
                                 'completed_unix_ns': time.time_ns()})
            if self.wait_status is not None:
                self.receipt['target_exit_code'] = os.waitstatus_to_exitcode(self.wait_status)
            if self.syscalls is not None:
                self.receipt['syscall_custody'] = self.syscalls.summary()
            self.finish_streams()
            self.close()
            durable_json(self.record_path, self.receipt)
        return self.receipt

    def capture(self, name, block):
        info = self.streams[name]
        write_all(info['stream'].fileno(), block)
        info['hash'].update(block)
        info['size'] += len(block)
        info['preview'].extend(block[:max(0, 8192 - len(info['preview']))])

    def finish_streams(self):
        outputs = {}
        for name, info in self.streams.items():
            stream = info['stream']
            if not stream.closed:
                stream.flush()
                os.fsync(stream.fileno())
                stream.close()
            outputs[name] = {'path': str(info['path']), 'size': info['size'],
                             'sha256': info['hash'].hexdigest(),
                             'fully_drained': info['fully_drained'],
                             'preview_utf8': info['preview'].decode(errors='replace'),
                             'preview_truncated': info['size'] > len(info['preview'])}
        self.receipt['streams'] = outputs
        if self.journal is not None:
            self.journal.flush()
            os.fsync(self.journal.fileno())
            self.journal.close()
            self.receipt['events'] = {'path': str(self.journal_path),
                                      'sha256': digest(self.journal_path)}
            self.journal = None

    def close(self):
        self.stop()
        if self.syscalls is not None:
            self.syscalls.close_notifications()
        if self.syscall_journal is not None:
            self.syscall_journal.flush()
            os.fsync(self.syscall_journal.fileno())
            self.receipt['syscall_journal'] = {
                'path': str(self.syscall_journal_path),
                'size': self.syscall_journal.tell(),
                'sha256': self.syscall_journal_hash.hexdigest()}
            self.syscall_journal.close()
            self.syscall_journal = None
        for value in (self.parent_socket, self.child_socket):
            if value is not None:
                value.close()
        self.parent_socket = self.child_socket = None
        if self.journal is not None:
            self.journal.close()
            self.journal = None
        for info in self.streams.values():
            if not info['stream'].closed:
                info['stream'].close()
        for fd in self.owned_fds:
            os.close(fd)
        self.owned_fds.clear()
        self.receipt['retained_fds_closed'] = True
        self.receipt['retained_through_reaping'] = self.owner.complete and (not self.pid or self.reaped)


def launch(manifest_path, record_path, timeout, arguments, audit_module=None):
    """Run one private CLI owner without changing or reaping the caller's tree."""
    command = [sys.executable, str(Path(__file__).resolve()), 'launch',
               '--manifest', str(Path(manifest_path).absolute()),
               '--record', str(Path(record_path).absolute()), '--timeout', str(timeout)]
    if audit_module is not None:
        command.extend(('--audit-module', str(Path(audit_module).absolute())))
    command.extend(('--', *arguments))
    started = time.time_ns()
    process = subprocess.Popen(command)
    process.wait()
    path = Path(record_path)
    require(path.is_file(), 'isolated custody worker produced no durable receipt (exit '
            + str(process.returncode) + ')')
    receipt = json.loads(path.read_text(encoding='utf-8'))
    require(receipt.get('supervisor_pid') == process.pid
            and receipt.get('started_unix_ns', 0) >= started
            and receipt.get('retained_fds_closed') is True
            and receipt.get('owned_tree_retired') is True,
            'isolated worker did not establish complete owned-tree retirement')
    return receipt


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest='command', required=True)
    command = commands.add_parser('launch')
    command.add_argument('--manifest', required=True)
    command.add_argument('--record', required=True)
    command.add_argument('--timeout', required=True, type=float)
    command.add_argument('--audit-module', help='must match the manifest auditor; never an unpinned override')
    command.add_argument('arguments', nargs=argparse.REMAINDER)
    options = parser.parse_args(argv)
    arguments = options.arguments
    if arguments[:1] == ['--']:
        arguments = arguments[1:]
    try:
        declare_isolated_owner()
        with Supervisor(options.manifest, options.record, options.timeout,
                        arguments, options.audit_module) as supervisor:
            receipt = supervisor.run()
    except (CustodyError, OwnershipError, OSError, ValueError) as error:
        print('runtime custody: ' + str(error), file=sys.stderr)
        return 126
    if not receipt['admitted']:
        print('runtime custody: ' + str(receipt['failure']), file=sys.stderr)
        return 126
    code = receipt['target_exit_code']
    return code if code >= 0 else 128 - code


if __name__ == '__main__':
    sys.exit(main())
