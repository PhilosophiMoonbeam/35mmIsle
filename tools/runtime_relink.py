#!/usr/bin/env python3
"""PRIVATE candidate-specific static GNU lightning source/application relink kit.

No install/upload action and no redistribution grant. Capture is called only by
an opted-in actual product link; assemble is an explicit build target. The kit
uses ordinary native link inputs, never a copy of the existing executable.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import struct
import subprocess
import sys
import tempfile
from datetime import datetime, timezone

SCHEMA = 1
MARKER = 'ISLE_LIGHTNING_MODIFIED_LIBRARY_PROOF_659hv'
SYSTEM_LINUX = ('/lib', '/lib64', '/usr/lib', '/usr/lib64')
SYSTEM_NAMES = {'c', 'm', 'dl', 'pthread', 'rt', 'stdc++', 'gcc', 'gcc_s', 'atomic', 'util', 'resolv'}


def digest(path):
    value = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            value.update(block)
    return value.hexdigest()


def load(path):
    return json.loads(Path(path).read_text(encoding='utf-8-sig'))


def save(path, value):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(path.suffix + '.new')
    temporary.write_text(json.dumps(value, indent=2, ensure_ascii=False) + '\n', encoding='utf-8')
    temporary.replace(path)


def fail(message):
    raise ValueError(message)


def absolute(value, cwd):
    path = Path(value)
    return (path if path.is_absolute() else Path(cwd) / path).resolve()


def within(path, root):
    try:
        Path(path).resolve().relative_to(Path(root).resolve())
        return True
    except ValueError:
        return False


def windows_split(text):
    """MS C command/response-file quote and backslash rules, without a shell."""
    args = []
    index = 0
    while index < len(text):
        while index < len(text) and text[index].isspace():
            index += 1
        if index == len(text):
            break
        word = []
        quoted = False
        while index < len(text) and (quoted or not text[index].isspace()):
            slashes = 0
            while index < len(text) and text[index] == '\\':
                slashes += 1
                index += 1
            if index < len(text) and text[index] == '"':
                word.extend('\\' * (slashes // 2))
                if slashes % 2:
                    word.append('"')
                elif quoted and index + 1 < len(text) and text[index + 1] == '"':
                    word.append('"')
                    index += 1
                else:
                    quoted = not quoted
                index += 1
            else:
                word.extend('\\' * slashes)
                if index < len(text) and (quoted or not text[index].isspace()):
                    word.append(text[index])
                    index += 1
        if quoted:
            fail('unterminated Windows response-file quote')
        args.append(''.join(word))
    return args


def split(text, windows):
    return windows_split(text) if windows else shlex.split(text, posix=True)


def expand_response(arguments, cwd, windows, records=None, active=None):
    records = records if records is not None else []
    active = active if active is not None else set()
    result = []
    for arg in arguments:
        if not windows and arg.startswith('-Wl,'):
            values = expand_response(arg[4:].split(','), cwd, False, records, active)
            for value in values:
                result.extend(['-Xlinker', value])
            continue
        if not arg.startswith('@'):
            result.append(arg)
            continue
        path = absolute(arg[1:], cwd)
        if path in active:
            fail('recursive response-file cycle: ' + path.name)
        data = path.read_bytes()
        text = data.decode('utf-16') if data.startswith((b'\xff\xfe', b'\xfe\xff')) else data.decode('utf-8-sig')
        records.append({'sha256': hashlib.sha256(data).hexdigest(), 'size': len(data)})
        # Both GNU ld and LINK resolve nested @ paths against link cwd, not
        # the containing response-file directory. Preserve that exact rule.
        result.extend(expand_response(split(text, windows), cwd, windows, records, active | {path}))
    return result


def native_objects(path, windows):
    """Reject thin archives, LLVM bitcode, GCC LTO, and MSVC /GL-only input."""
    data = Path(path).read_bytes()
    if data.startswith(b'!<thin>\n'):
        fail('thin archive needs external members; no usable kit: ' + Path(path).name)
    members = []
    if data.startswith(b'!<arch>\n'):
        offset = 8
        while offset < len(data):
            header = data[offset:offset + 60]
            if len(header) != 60 or header[58:60] != b'`\n':
                fail('malformed archive: ' + Path(path).name)
            size = int(header[48:58].strip())
            name = header[:16].strip()
            body = data[offset + 60:offset + 60 + size]
            if len(body) != size:
                fail('truncated archive member')
            if name not in (b'/', b'//', b'/SYM64/'):
                if name.startswith(b'#1/'):
                    body = body[int(name[3:]):]
                members.append(body)
            offset += 60 + size + (size & 1)
    else:
        members.append(data)
    directives = []
    for body in members:
        if body.startswith((b'BC\xc0\xde', b'\xde\xc0\x17\x0b')) or b'.gnu.lto_' in body or b'.llvmbc' in body:
            fail('LTO-only material is not admitted; produce separately qualified ordinary objects: ' + Path(path).name)
        if not windows or len(body) < 20:
            continue
        machine, sections = struct.unpack_from('<HH', body)
        if machine == 0 and sections == 0xffff:
            version = struct.unpack_from('<H', body, 4)[0]
            if version == 0:  # Short COFF import object: external DLL, not /GL.
                continue
            bigobj = bytes.fromhex('c7a1bad1eebaa94baf20faf66aa4dcb8')
            if version < 2 or body[12:28] != bigobj or len(body) < 56:
                fail('MSVC anonymous/LTCG object is not an ordinary COFF object: ' + Path(path).name)
            sections = struct.unpack_from('<I', body, 44)[0]
            section_start = 56
        elif machine in (0x8664, 0x14c, 0xaa64):
            section_start = 20 + struct.unpack_from('<H', body, 16)[0]
        else:
            continue  # .res, linker import metadata, or non-object input.
        for index in range(sections):
            start = section_start + index * 40
            section = body[start:start + 40]
            if len(section) != 40:
                fail('truncated COFF section table')
            if section[:8].rstrip(b'\0') == b'.drectve':
                size, offset = struct.unpack_from('<II', section, 16)
                directives.extend(windows_split(body[offset:offset + size].decode('ascii').strip('\0 ')))
    return directives


def reject_implicit_link_environment(windows):
    if os.environ.get('LINK') or os.environ.get('_LINK_'):
        fail('implicit LINK/_LINK_ options are forbidden for capture and recipient relinking')
    if not windows and any(os.environ.get(key) for key in (
            'GCC_EXEC_PREFIX', 'COMPILER_PATH', 'LIBRARY_PATH',
            'LD_LIBRARY_PATH', 'LD_RUN_PATH', 'LD_PRELOAD')):
        fail('implicit compiler/linker/loader search or injection environment is forbidden')


def reject_alternate_linker_selection(arguments, windows):
    if windows:
        return  # MSBuild invokes the captured native link.exe directly.
    selectors = ('-fuse-ld', '--ld-path', '-ld-path', '-B', '-specs', '--specs',
                 '-wrapper', '--wrapper', '--gcc-toolchain', '-gcc-toolchain',
                 '--gcc-install-dir', '-gcc-install-dir', '--target', '-target')
    linker_operand = False
    for argument in arguments:
        if linker_operand:
            linker_operand = False
            continue
        if not isinstance(argument, str):
            continue
        if argument == '-Xlinker':
            linker_operand = True
            continue
        if any(argument == selector or argument.startswith(selector + '=')
               for selector in selectors) or (
                argument.startswith('-B') and argument not in ('-Bstatic', '-Bdynamic')):
            fail('alternate driver/toolchain/linker selection is not admitted: ' + argument.split('=')[0])


def native_driver_linker(driver):
    name = subprocess.check_output([str(driver), '-print-prog-name=ld'], text=True).strip()
    path = Path(shutil.which(name) or name).resolve()
    if not path.is_file():
        fail('native driver-selected default linker cannot be identified')
    return path


def tool_receipt(settings, driver):
    """Do not copy general-purpose compiler/SDK binaries into the private kit."""
    driver = Path(shutil.which(str(driver)) or driver).resolve()
    paths = {'driver': driver, 'c_compiler': Path(settings['c_compiler']).resolve(),
             'cxx_compiler': Path(settings['cxx_compiler']).resolve(),
             'linker': (Path(settings['linker']).resolve() if settings['system'] == 'Windows'
                        else native_driver_linker(driver))}
    if settings['system'] == 'Windows':
        for pattern in ('c1.dll', 'c1xx.dll', 'c2.dll', 'mspdb*.dll', 'msobj*.dll'):
            for path in driver.parent.glob(pattern):
                paths['sibling:' + path.name] = path
    else:
        if paths['linker'] != Path(settings['linker']).resolve():
            fail('native driver default linker differs from CMake; no unverified alternate linker route')
        for label, option in (('lto_wrapper', '-print-prog-name=lto-wrapper'),
                              ('lto_plugin', '-print-file-name=liblto_plugin.so')):
            name = subprocess.check_output([str(driver), option], text=True).strip()
            path = Path(name)
            if path.is_file():
                paths[label] = path.resolve()
    return {key: {'name': path.name, 'sha256': digest(path)} for key, path in paths.items()}


class LinkPlan:
    def __init__(self, settings, args, cwd, trace=()):
        self.settings = settings
        self.windows = settings['system'] == 'Windows'
        self.cwd = Path(cwd)
        self.args = args
        self.trace = [Path(path).resolve() for path in trace]
        self.inputs = {}
        self.system_inputs = {}
        self.recipe = []
        self.search = []
        self.defaultlibs = []
        self.resource = False
        self.lightning_slots = 0
        self.output = None
        self.static = False
        self.selection_stack = []
        self.system_roots = []
        if self.windows:
            for key in ('VCToolsInstallDir', 'WindowsSdkDir', 'UniversalCRTSdkDir'):
                if os.environ.get(key):
                    self.system_roots.append(Path(os.environ[key]).resolve())
            if not self.system_roots:
                fail('MSBuild capture requires the real VS developer environment (VC/Windows SDK roots)')
        else:
            self.system_roots = [Path(x).resolve() for x in SYSTEM_LINUX]
        search_args = [arg for arg in args if arg != '-Xlinker'] if not self.windows else args
        for index, arg in enumerate(search_args):
            if self.windows and arg.lower().startswith('/libpath:'):
                self.search.append(absolute(arg.split(':', 1)[1], cwd))
            elif not self.windows and arg.startswith('-L'):
                value = arg[2:] if len(arg) > 2 else search_args[index + 1]
                self.search.append(absolute(value, cwd))
        if self.windows:
            # LINK searches explicit /LIBPATH entries before the developer LIB.
            self.search.extend(absolute(x, cwd) for x in os.environ.get('LIB', '').split(';') if x)

    def system(self, path):
        if not any(within(path, root) for root in self.system_roots):
            return False
        if self.windows:
            return True
        # A /usr/lib location is not itself the LGPL System Library exception:
        # SDL, libzip and other application libraries must still be delivered.
        name = Path(path).name
        return (bool(re.fullmatch(r'(?:[A-Za-z]*crt[^/]*|crtbegin[^/]*|crtend[^/]*)\.o', name)) or
                bool(re.fullmatch(r'ld-linux[^/]*\.so(?:\.[0-9]+)*', name)) or
                any(re.fullmatch(r'lib' + re.escape(library) + r'(?:_nonshared)?\.(?:a|so(?:\.[0-9]+)*)', name)
                    for library in SYSTEM_NAMES))

    def input(self, value, defaultlib=False):
        path = absolute(value, self.cwd)
        if not path.is_file():
            fail('missing actual link input: ' + path.name)
        if self.system(path):
            self.system_inputs[str(path)] = {'name': path.name, 'sha256': digest(path)}
            # Explicit absolute System Libraries are acquired from the matching
            # recipient toolchain, never referred to by a producer path.
            return {'system': path.name}
        lightning = path == Path(self.settings['lightning']).resolve()
        if lightning:
            self.lightning_slots += 1
            return {'lightning': True}
        key = str(path)
        if key not in self.inputs:
            directives = native_objects(path, self.windows) if path.suffix.lower() in ('.o', '.obj', '.a', '.lib', '.res') else []
            self.inputs[key] = {'origin': key, 'name': path.name, 'sha256': digest(path),
                                'size': path.stat().st_size, 'defaultlib': defaultlib}
            if path.suffix.lower() == '.res':
                self.resource = True
            for directive in directives:
                if directive.lower().startswith(('/defaultlib:', '-defaultlib:')):
                    value = directive.split(':', 1)[1]
                    if Path(value).is_absolute() or '/' in value or '\\' in value:
                        fail('embedded absolute/path DEFAULTLIB cannot be relocated safely: ' + path.name)
                    self.defaultlibs.append(value)
                elif re.search(r'[A-Za-z]:[\\/]', directive):
                    fail('private absolute path in a COFF linker directive: ' + path.name)
        elif defaultlib:
            self.inputs[key]['defaultlib'] = True
        return {'input': key}

    def library(self, name, defaultlib=False):
        if self.windows:
            filename = name if name.lower().endswith('.lib') else name + '.lib'
            matches = [directory / filename for directory in self.search if (directory / filename).is_file()]
            if not matches:
                fail('unresolved MSVC library (including embedded DEFAULTLIB): ' + filename)
            return self.input(str(matches[0]), defaultlib)
        exact = name.startswith(':')
        names = [name[1:]] if exact else (['lib' + name + '.a'] if self.static else ['lib' + name + '.so', 'lib' + name + '.a'])
        for directory in self.search:
            for filename in names:
                path = directory / filename
                if path.is_file():
                    return self.input(str(path))
        # The trace is from the actual successful link, not a guessed library
        # family. Canonical symlink names may differ from -l names.
        matches = [path for path in self.trace if path.name in names]
        if len(set(matches)) == 1:
            return self.input(str(matches[0]))
        for filename in names:
            value = subprocess.check_output([self.settings['cxx_compiler'], '-print-file-name=' + filename], text=True).strip()
            if Path(value).is_file():
                return self.input(value)
        if not exact and name in SYSTEM_NAMES:
            return '-l' + name
        fail('unresolved actual Linux -l input: ' + name)

    def link_tokens(self, values):
        """Normalize GNU linker-only arguments, preserving ordering/groups."""
        result = []
        index = 0
        while index < len(values):
            token = values[index]
            if token.startswith('-L'):
                if token == '-L':
                    index += 1
                    if index == len(values):
                        fail('missing linker library-directory argument')
                index += 1
                continue
            if token == '--dependency-file' or token.startswith('--dependency-file='):
                if token == '--dependency-file':
                    index += 1
                    if index == len(values):
                        fail('missing linker dependency-file output')
                    value = values[index]
                else:
                    value = token.split('=', 1)[1]
                name = Path(value).name
                if not name or name in ('.', '..'):
                    fail('invalid linker dependency-file output')
                result.append({'side_output': '--dependency-file=', 'name': name})
                index += 1
                continue
            if token in ('-rpath', '--rpath', '-rpath-link', '--rpath-link'):
                if index + 1 == len(values):
                    fail('missing linker path argument')
                value = '$ORIGIN/lib' if 'rpath-link' not in token else {'runtime_dir': True}
                result.extend([token, value])
                index += 2
                continue
            if token.startswith(('-rpath=', '--rpath=')):
                result.extend(['-rpath', '$ORIGIN/lib'])
            elif token.startswith(('-rpath-link=', '--rpath-link=')):
                result.extend(['-rpath-link', {'runtime_dir': True}])
            elif token == '--push-state':
                self.selection_stack.append(self.static)
                result.append(token)
            elif token == '--pop-state':
                if not self.selection_stack:
                    fail('unmatched linker --pop-state')
                self.static = self.selection_stack.pop()
                result.append(token)
            elif token in ('-Bstatic', '--Bstatic'):
                self.static = True
                result.append(token)
            elif token in ('-Bdynamic', '--Bdynamic'):
                self.static = False
                result.append(token)
            elif token.startswith('-l'):
                result.append(self.library(token[2:]))
            elif token in ('--version-script', '--dynamic-list', '-T', '--script', '--retain-symbols-file'):
                if index + 1 == len(values):
                    fail('missing linker control input')
                result.extend([token, self.input(values[index + 1])])
                index += 2
                continue
            elif token.startswith(('--version-script=', '--dynamic-list=', '--script=', '--retain-symbols-file=')):
                key, value = token.split('=', 1)
                result.extend([key, self.input(value)])
            elif Path(token).is_absolute() or (not token.startswith('-') and absolute(token, self.cwd).is_file()):
                result.append(self.input(token))
            else:
                self.clean_option(token)
                result.append(token)
            index += 1
        return result

    def clean_option(self, token):
        roots = [self.settings[key] for key in ('project', 'build', 'source')]
        if any(root and root.replace('\\', '/') in token.replace('\\', '/') for root in roots):
            fail('unhandled producer path in link option; no-kit failure: ' + token.split('=')[0])
        if re.search(r'[A-Za-z]:[\\/]', token) or re.search(r'(?:=|,)/[^$]', token):
            fail('unhandled absolute path in link option: ' + token.split('=')[0])

    def build(self):
        index = 0
        while index < len(self.args):
            arg = self.args[index]
            lower = arg.lower()
            if self.windows:
                if lower.startswith('/out:'):
                    self.output = absolute(arg.split(':', 1)[1], self.cwd)
                    self.recipe.append({'output_option': '/OUT:'})
                elif lower.startswith(('/pdb:', '/implib:', '/ilk:', '/map:')):
                    self.recipe.append({'side_output': arg.split(':', 1)[0] + ':',
                                        'name': Path(arg.split(':', 1)[1]).name})
                elif lower.startswith('/libpath:'):
                    pass  # Every non-system lookup is copied; SDK lookup uses recipient LIB.
                elif lower.startswith(('/defaultlib:', '/wholearchive:')):
                    option, value = arg.split(':', 1)
                    item = self.library(value) if '/' not in value and '\\' not in value else self.input(value)
                    self.recipe.append({'prefix': option + ':', 'value': item})
                elif lower.startswith('/ltcg') and lower not in ('/ltcg:off',):
                    fail('MSVC /LTCG is not admitted by the ordinary-object relink route')
                elif lower.startswith('/manifestinput:') or lower.startswith('/def:'):
                    option, value = arg.split(':', 1)
                    self.recipe.append({'prefix': option + ':', 'value': self.input(value)})
                elif not arg.startswith(('/', '-')):
                    self.recipe.append(self.input(arg) if ('/' in arg or '\\' in arg or absolute(arg, self.cwd).is_file()) else self.library(arg))
                else:
                    self.clean_option(arg)
                    self.recipe.append(arg)
            else:
                if arg == '-o':
                    index += 1
                    self.output = absolute(self.args[index], self.cwd)
                    self.recipe.extend(['-o', {'output': True}])
                elif arg.startswith('-L'):
                    if arg == '-L':
                        index += 1
                elif arg.startswith('-l'):
                    self.recipe.append(self.library(arg[2:]))
                elif arg.startswith('-Wl,'):
                    for value in self.link_tokens(arg[4:].split(',')):
                        self.recipe.extend(['-Xlinker', value])
                elif arg == '-Xlinker':
                    # Gather consecutive -Xlinker pairs so options with operands
                    # can be rewritten together, without changing their order.
                    tokens = []
                    while index < len(self.args) and self.args[index] == '-Xlinker':
                        if index + 1 == len(self.args):
                            fail('missing -Xlinker argument')
                        tokens.append(self.args[index + 1])
                        index += 2
                    for value in self.link_tokens(tokens):
                        self.recipe.extend(['-Xlinker', value])
                    continue
                elif arg in ('-Bstatic', '-static'):
                    self.static = True
                    self.recipe.append(arg)
                elif arg == '-Bdynamic':
                    self.static = False
                    self.recipe.append(arg)
                elif not arg.startswith('-'):
                    self.recipe.append(self.input(arg))
                elif arg.startswith(('-flto', '-fuse-linker-plugin')):
                    fail('LTO link mode is not admitted by the ordinary-object relink route')
                else:
                    self.clean_option(arg)
                    self.recipe.append(arg)
            index += 1
        if self.output != Path(self.settings['product']).resolve():
            fail('captured command is not the actual 35mmisle final product link')
        if not self.lightning_slots:
            fail('actual final product link has no replaceable lightning archive slot')
        # Discover libraries referenced by COFF .drectve, recursively. They are
        # kept as default libraries (not forced into the ordered explicit list).
        processed = set()
        while self.defaultlibs:
            name = self.defaultlibs.pop(0)
            if name.lower() not in processed:
                processed.add(name.lower())
                self.library(name, defaultlib=True)
        for path in self.trace:
            if path.is_file():
                if path == Path(self.settings['lightning']).resolve():
                    continue
                self.input(str(path))
        if self.windows and not self.resource:
            fail('actual Windows product capture omitted its compiled .res input')
        if self.windows and '/manifest:no' not in [x.lower() for x in self.recipe if isinstance(x, str)]:
            fail('actual Windows resource-embedded manifest /MANIFEST:NO treatment is missing')
        return self


def capture(settings_path, command):
    settings = load(settings_path)
    windows = settings['system'] == 'Windows'
    if not command:
        fail('capture requires the actual linker/driver command')
    cwd = Path.cwd()
    responses = []
    arguments = expand_response(command[1:], cwd, windows, responses)
    capture_path = Path(settings['capture'])
    # A failed replacement link must never leave a previous successful receipt.
    capture_path.unlink(missing_ok=True)
    reject_implicit_link_environment(windows)
    reject_alternate_linker_selection(arguments, windows)
    if not windows:
        # --trace changes diagnostics only, not generated code or runtime ABI.
        # It closes implicit driver/startfile/linker-script library discovery.
        actual = command + ['-Wl,--trace']
        process = subprocess.run(actual, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        sys.stdout.buffer.write(process.stdout)
        sys.stdout.buffer.flush()
        trace = []
        for line in process.stdout.decode(errors='replace').splitlines():
            value = line.strip()
            if value.startswith('(') and ')' in value:
                value = value[1:value.index(')')]
            path = absolute(value, cwd)
            if path.is_file():
                trace.append(str(path))
    else:
        process = subprocess.run(command)
        trace = []
    if process.returncode:
        return process.returncode
    plan = LinkPlan(settings, arguments, cwd, trace).build()
    runtime_closure = None if windows else linux_runtime_closure(settings, plan)
    record = {'schema_version': SCHEMA, 'settings_sha256': digest(settings_path),
              'product_sha256': digest(settings['product']), 'lightning_sha256': digest(settings['lightning']),
              'source_provenance_sha256': digest(settings['provenance']),
              'capture_helper_sha256': digest(Path(__file__)),
              'captured_at_utc': datetime.now(timezone.utc).isoformat(),
              'original_command': command, 'expanded_arguments': arguments, 'working_directory': str(cwd),
              'effective_command': command if windows else actual,
              'captured_invocation_sha256': hashlib.sha256(json.dumps(
                  {'command': command, 'expanded_arguments': arguments}, ensure_ascii=False).encode()).hexdigest(),
              'responses': responses, 'recipe': plan.recipe, 'inputs': list(plan.inputs.values()),
              'system_inputs': list(plan.system_inputs.values()),
              'runtime_closure': runtime_closure,
              'toolchain_files': tool_receipt(settings, command[0]),
              'diagnostic_trace': not windows, 'driver_name': Path(command[0]).name}
    save(capture_path, record)
    return 0


def file_api(settings, capture_record):
    reply = Path(settings['build']) / '.cmake/api/v1/reply'
    # CMake specifies the lexicographically largest index name as current.
    indexes = sorted(reply.glob('index-*.json'))
    if not indexes:
        fail('CMake File API reply missing; configure with the private relink hook before building')
    index = load(indexes[-1])
    # cmake_file_api(QUERY) produces shared stateless replies, not client replies.
    model_response = index.get('reply', {}).get('codemodel-v2', {})
    if (model_response.get('kind') != 'codemodel' or
            model_response.get('version', {}).get('major') != 2 or not model_response.get('jsonFile')):
        fail('CMake File API shared codemodel-v2 response missing or incompatible')
    model = load(reply / model_response['jsonFile'])
    configurations = [item for item in model['configurations'] if item['name'] == settings['configuration']]
    if len(configurations) != 1:
        fail('captured configuration does not match CMake File API')
    targets = {item['id']: load(reply / item['jsonFile']) for item in configurations[0]['targets']}
    products = [item for item in targets.values() if item['name'] == '35mmisle']
    lightnings = [item for item in targets.values() if item['name'] == 'isle-rsp-lightning']
    if len(products) != 1 or len(lightnings) != 1:
        fail('actual product/lightning targets absent from File API')
    product, lightning = products[0], lightnings[0]
    observed = {Path(item['origin']).resolve() for item in capture_record['inputs']}
    observed.add(Path(settings['lightning']).resolve())
    dependencies = []
    visited = set()

    def visit(target):
        if target['id'] in visited:
            return
        visited.add(target['id'])
        dependencies.append({'name': target['name'], 'type': target['type']})
        if target['type'] == 'STATIC_LIBRARY':
            for artifact in target.get('artifacts', []):
                if absolute(artifact['path'], settings['build']) not in observed:
                    fail('actual captured closure omitted File API static target: ' + target['name'])
        for dependency in target.get('dependencies', []):
            if dependency['id'] in targets:
                visit(targets[dependency['id']])

    visit(product)
    names = {Path(item['origin']).name.lower() for item in capture_record['inputs']}
    direct_sources = []
    for source in product.get('sources', []):
        if 'compileGroupIndex' not in source:
            continue
        filename = Path(source['path']).name
        if source['path'].lower().endswith('.rc'):
            candidates = {Path(filename).stem.lower() + '.res', filename.lower() + '.res'}
        elif settings['system'] == 'Windows':
            candidates = {Path(filename).stem.lower() + '.obj', filename.lower() + '.obj'}
        else:
            candidates = {filename.lower() + '.o'}
        if not names.intersection(candidates):
            fail('actual product object/resource absent from capture: ' + filename)
        # No application source paths/bodies are exported.
        direct_sources.append({'kind': 'resource' if source['path'].lower().endswith('.rc') else 'compiled-object'})
    if not direct_sources:
        fail('File API did not identify the actual product compilation inputs')
    groups = lightning.get('compileGroups', [])
    if not groups or any(group['language'] != 'C' for group in groups):
        fail('unexpected lightning compilation language/metadata')
    compile_options = []
    definitions = []
    for group in groups:
        options = []
        for fragment in group.get('compileCommandFragments', []):
            options.extend(split(fragment['fragment'], settings['system'] == 'Windows'))
        if compile_options and options != compile_options:
            fail('lightning source-specific compile options require a distinct standalone build recipe')
        compile_options = options
        definitions.extend(item['define'] for item in group.get('defines', []))
        for include in group.get('includes', []):
            path = Path(include['path']).resolve()
            if not within(path, settings['source']):
                fail('standalone lightning would require an external non-toolchain include path')
    for value in compile_options + definitions:
        if (settings['project'].replace('\\', '/') in value.replace('\\', '/') or
                settings['build'].replace('\\', '/') in value.replace('\\', '/') or
                re.search(r'[A-Za-z]:[\\/]', value)):
            fail('producer-path compile flag cannot enter standalone library recipe')
        if value.startswith(('-flto', '/GL')):
            fail('ordinary-object kit cannot rebuild lightning with LTO')
    return {'dependencies': dependencies, 'product_compilation_inputs': direct_sources,
            'compile_options': compile_options, 'definitions': sorted(set(definitions)),
            'codemodel_version': model['version']}


def copy_regular_tree(source, destination, excluded):
    source, destination = Path(source), Path(destination)
    if not source.is_dir():
        fail('required exact library source directory missing: ' + source.name)
    for path in sorted(source.rglob('*')):
        relative = path.relative_to(source)
        if path.is_symlink():
            fail('selected library source contains a symlink')
        if path.is_dir():
            continue
        if relative.as_posix() in excluded:
            continue
        if any(part in ('.git', '.vs', '__pycache__', 'CMakeFiles') for part in relative.parts):
            fail('workspace residue in selected library source; not a source kit')
        if not path.is_file():
            fail('selected library source contains a non-regular input')
        target = destination / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(path, target)


def library_source(settings, output, capture_record, api):
    import tarfile
    source = Path(settings['source'])
    project = Path(settings['project'])
    provenance = load(settings['provenance'])
    manifest_path = project / 'cmake/rsp-dependencies.json'
    manifest = load(manifest_path)
    if digest(manifest_path) != provenance['manifest_sha256'] or manifest['dependencies'] != provenance['dependencies']:
        fail('prepared-source pin/manifest provenance mismatch')
    for name, field in (('prepare_rsp.py', 'generator_sha256'), ('prepare_rdp.py', 'helper_sha256')):
        if digest(project / 'tools' / name) != provenance[field]:
            fail('prepared-source helper differs from production provenance: ' + name)
    if digest(settings['provenance']) != capture_record['source_provenance_sha256']:
        fail('source preparation receipt changed after actual final link')
    record = manifest['dependencies'][0]
    archive_path = source.parent / 'archives' / (record['name'] + '-' + record['commit'] + '.tar.gz')
    if digest(archive_path) != record['archive_sha256']:
        fail('exact retained upstream archive missing or mismatched')
    adaptations = {item['path']: item for item in provenance['adaptations']}
    prefixes = ('lightning/', 'win32/mman/')
    expected = {}
    with tarfile.open(archive_path, 'r:gz') as archive:
        for member in archive:
            parts = Path(member.name).parts
            relative = '/'.join(parts[1:])
            if not relative.startswith(prefixes) or not member.isfile():
                continue
            stream = archive.extractfile(member)
            if stream is None:
                fail('unreadable pinned source archive member')
            original = hashlib.sha256(stream.read()).hexdigest()
            if relative in adaptations:
                if original != adaptations[relative]['original_sha256']:
                    fail('patch ancestry differs from retained upstream archive')
                prepared = adaptations[relative]['adapted_sha256']
            else:
                prepared = original
            if digest(source / relative) != prepared:
                fail('prepared source differs from exact pinned/modified inputs: ' + relative)
            expected[relative] = {'original_sha256': original, 'prepared_sha256': prepared}
    actual = {path.relative_to(source).as_posix()
              for prefix in ('lightning', 'win32/mman') for path in (source / prefix).rglob('*') if path.is_file()}
    if actual != set(expected):
        fail('prepared lightning/mman tree has extra or missing source/build inputs')
    # This exact pinned upstream member is personal IDE state, not build control.
    excluded_workspace = {name: value for name, value in expected.items()
                          if name == 'win32/mman/sys/.vs/mman/v14/.suo'}
    library = output / 'library'
    copy_regular_tree(source / 'lightning', library / 'lightning', set())
    copy_regular_tree(source / 'win32/mman', library / 'win32/mman',
                      {name[len('win32/mman/'):] for name in excluded_workspace})
    for name in ('COPYING', 'COPYING.LESSER', 'COPYING.DOC'):
        if not (library / 'lightning' / name).is_file():
            fail('required GNU lightning license text missing: ' + name)
    provenance_dir = library / 'provenance'
    provenance_dir.mkdir()
    shutil.copyfile(settings['provenance'], provenance_dir / 'prepared-source.json')
    shutil.copyfile(manifest_path, provenance_dir / 'rsp-dependencies.json')
    for name in ('prepare_rsp.py', 'prepare_rdp.py'):
        shutil.copyfile(project / 'tools' / name, provenance_dir / name)
    save(provenance_dir / 'source-files.json', expected)
    save(provenance_dir / 'excluded-upstream-ide-cache.json', {
        'reason': 'Pinned upstream personal Visual Studio state, not source or compilation/installation control; all build inputs retained.',
        'files': excluded_workspace})
    save(provenance_dir / 'modifications.json', {
        'upstream': record, 'patch_identity': provenance['patch_identity'],
        'prepared_modifications': [
            item | {'observed_prepared_file_mtime_utc': datetime.fromtimestamp(
                (source / item['path']).stat().st_mtime, timezone.utc).isoformat()}
            for item in provenance['adaptations'] if item['path'] in expected],
        'recorded_at_utc': capture_record['captured_at_utc'],
        'preparation_date_limit': 'Prepared-file filesystem modification times are observed metadata, not invented authorship dates; capture date also retained.',
        'tool_hashes': {key: provenance[key] for key in ('generator_sha256', 'helper_sha256', 'manifest_sha256')},
        'mman_disposition': 'Exact vendored ancestry and README license declaration retained; complete applicable notice/grant disposition unresolved. R1 HOLD.'})
    shutil.copyfile(project / 'cmake/RuntimeRelinkLibrary.cmake.in', library / 'CMakeLists.txt')
    shutil.copyfile(project / 'cmake/RuntimeRelinkJitProof.c', library / 'jit-proof.c')
    # Bracket quoting keeps literal flags, quotes, backslashes and semicolons
    # independent of the producer's shell and of the extracted kit location.
    def cmake_quote(value):
        if ']==]' in value:
            fail('unrepresentable CMake option')
        return '[==[' + value + ']==]'
    flags = ['# Actual candidate lightning compilation flags; no producer include paths.',
             'if(NOT "${CMAKE_BUILD_TYPE}" STREQUAL "" AND NOT "${CMAKE_BUILD_TYPE}" STREQUAL ' +
             cmake_quote(settings['configuration']) + ')',
             '  message(FATAL_ERROR "Use the candidate configuration recorded in kit-manifest.json")', 'endif()']
    flags.extend([
        'if(CMAKE_CONFIGURATION_TYPES AND NOT "${CMAKE_CONFIGURATION_TYPES}" STREQUAL ' +
        cmake_quote(settings['configuration']) + ')',
        '  message(FATAL_ERROR "Configure only the recorded candidate configuration")', 'endif()',
        'file(SHA256 "${CMAKE_C_COMPILER}" _isle_candidate_compiler_sha256)',
        'if(NOT _isle_candidate_compiler_sha256 STREQUAL ' +
        cmake_quote(capture_record['toolchain_files']['c_compiler']['sha256']) + ')',
        '  message(FATAL_ERROR "Use the exact recorded candidate C compiler")', 'endif()'])
    if api['compile_options']:
        flags.append('target_compile_options(isle-rsp-lightning PRIVATE ' +
                     ' '.join(cmake_quote(value) for value in api['compile_options']) + ')')
    if api['definitions']:
        flags.append('target_compile_definitions(isle-rsp-lightning PRIVATE ' +
                     ' '.join(cmake_quote(value) for value in api['definitions']) + ')')
    (library / 'candidate-flags.cmake').write_text('\n'.join(flags) + '\n', encoding='utf-8')


def elf_dynamic(path):
    """Inspect ELF metadata, never load or execute an application/DSO."""
    with Path(path).open('rb') as stream:
        magic = stream.read(4)
    if magic != b'\x7fELF':
        return None
    output = subprocess.check_output(['readelf', '-d', str(path)], text=True)
    soname = re.findall(r'\(SONAME\).*\[([^\]]+)\]', output)
    needed = re.findall(r'\(NEEDED\).*\[([^\]]+)\]', output)
    if any('/' in name or '\\' in name for name in needed):
        fail('non-system DSO has a non-relocatable DT_NEEDED path: ' + Path(path).name)
    return {'soname': soname[0] if soname else None, 'needed': needed,
            'runpath': re.findall(r'\(RUNPATH\).*\[([^\]]*)\]', output),
            'rpath': re.findall(r'\(RPATH\).*\[([^\]]*)\]', output)}


def elf_identity(path):
    """Read native ELF identity/program headers with binutils, without loading it.

    This is deliberately separate from elf_dynamic's established return contract.
    Consumers must supply their own admission policy for dynamic directives.
    """
    dynamic = elf_dynamic(path)
    if dynamic is None:
        fail('not an ELF image: ' + str(path))
    environment = dict(os.environ, LC_ALL='C')
    output = subprocess.check_output(
        ['readelf', '--wide', '--file-header', '--program-headers', '--dynamic', str(path)],
        text=True, env=environment, stderr=subprocess.PIPE)
    def field(name):
        match = re.search(r'^\s*' + re.escape(name) + r':\s*(.+)$', output, re.MULTILINE)
        if not match:
            fail('missing ELF header field ' + name)
        return match.group(1).strip()
    segments = []
    for match in re.finditer(
            r'^\s*(\S+)\s+(0x[0-9a-fA-F]+)\s+(0x[0-9a-fA-F]+)\s+'
            r'(0x[0-9a-fA-F]+)\s+(0x[0-9a-fA-F]+)\s+(0x[0-9a-fA-F]+)'
            r'\s+([RWE ]*?)\s+(0x[0-9a-fA-F]+)\s*$', output, re.MULTILINE):
        kind, offset, address, physical, filesz, memsz, flags, alignment = match.groups()
        segments.append({'type': kind, 'offset': int(offset, 16),
                         'vaddr': int(address, 16), 'paddr': int(physical, 16),
                         'filesz': int(filesz, 16), 'memsz': int(memsz, 16),
                         'flags': flags.replace(' ', ''), 'align': int(alignment, 16)})
    if not segments:
        fail('ELF has no readable program headers: ' + str(path))
    interpreters = re.findall(r'\[Requesting program interpreter:\s*([^\]]+)\]', output)
    tags = re.findall(r'^\s*0x[0-9a-fA-F]+\s+\(([^)]+)\)\s*(.*)$',
                      output, re.MULTILINE)
    return {'class': field('Class'), 'data': field('Data'),
            'machine': field('Machine'), 'type': field('Type').split()[0],
            'entry': int(field('Entry point address'), 16),
            'segments': segments, 'interpreters': interpreters,
            'dynamic_tags': [{'tag': name, 'value': value} for name, value in tags],
            'dynamic': dynamic}


def linux_runtime_closure(settings, plan):
    """Resolve real ELF DT_NEEDED recursively, without executing any product.

    Native CMake applies ELF RPATH/RUNPATH and ldconfig rules, then the bounded
    parent directories of explicit DSO link inputs for extracted SDK siblings.
    No arbitrary SDK/host scan or guessed directory is permitted.
    """
    cmake = Path(settings['cmake']).resolve()
    objdump = Path(settings['objdump'] or shutil.which('objdump') or '').resolve()
    if not cmake.is_file() or not objdump.is_file():
        fail('native CMake/objdump runtime dependency resolver is unavailable')
    product = Path(settings['product']).resolve()
    product_metadata = elf_dynamic(product)
    if product_metadata is None:
        fail('actual Linux product is not ELF')
    libraries = []
    for item in plan.inputs.values():
        path = Path(item['origin'])
        if '.so' in path.name:
            if elf_dynamic(path) is None:
                fail('non-system DSO link input is not independently relocatable ELF: ' + path.name)
            libraries.append(path)
    library_directories = list(dict.fromkeys(path.parent for path in libraries))
    def quote(path):
        value = str(path)
        if any(part in value for part in (']==]', ';', '\n', '\r')):
            fail('runtime resolver cannot represent the selected native path')
        return '[==[' + value + ']==]'
    with tempfile.TemporaryDirectory(prefix='runtime-closure-', dir=Path(settings['capture']).parent) as temporary:
        temporary = Path(temporary)
        resolved_path = temporary / 'resolved.txt'
        script = temporary / 'resolve.cmake'
        lines = [
            'cmake_minimum_required(VERSION 3.24)',
            'set(CMAKE_GET_RUNTIME_DEPENDENCIES_PLATFORM "linux+elf")',
            'set(CMAKE_GET_RUNTIME_DEPENDENCIES_TOOL "objdump")',
            'set(CMAKE_GET_RUNTIME_DEPENDENCIES_COMMAND ' + quote(objdump) + ')',
            'file(GET_RUNTIME_DEPENDENCIES',
            '  EXECUTABLES ' + quote(product)]
        if libraries:
            lines.append('  LIBRARIES ' + ' '.join(quote(path) for path in libraries))
            lines.append('  DIRECTORIES ' + ' '.join(quote(path) for path in library_directories))
        lines.extend([
            '  RESOLVED_DEPENDENCIES_VAR _resolved',
            '  UNRESOLVED_DEPENDENCIES_VAR _unresolved',
            '  CONFLICTING_DEPENDENCIES_PREFIX _conflicts)',
            'if(_unresolved OR _conflicts_FILENAMES)',
            '  message(FATAL_ERROR "Private relink runtime closure unresolved: ${_unresolved}; conflicting: ${_conflicts_FILENAMES}")',
            'endif()',
            'file(WRITE ' + quote(resolved_path) + ' "")',
            'foreach(_path IN LISTS _resolved)',
            '  file(APPEND ' + quote(resolved_path) + ' "${_path}\\n")',
            'endforeach()'])
        script.write_text('\n'.join(lines) + '\n', encoding='utf-8')
        subprocess.run([str(cmake), '-P', str(script)], check=True)
        resolved = [Path(value) for value in resolved_path.read_text(encoding='utf-8').splitlines()]
    nodes = {}
    aliases = {}
    for selected in libraries + resolved:
        origin = selected.resolve()
        if not origin.is_file():
            fail('resolved runtime dependency is missing: ' + selected.name)
        key = str(origin)
        if key not in nodes:
            metadata = elf_dynamic(origin)
            if metadata is None:
                fail('resolved DT_NEEDED input is not ELF: ' + origin.name)
            nodes[key] = {'origin': key, 'name': origin.name, 'sha256': digest(origin),
                          'size': origin.stat().st_size, 'elf': metadata,
                          'system': plan.system(origin), 'aliases': []}
        node = nodes[key]
        for name in {selected.name, origin.name, node['elf']['soname']} - {None}:
            if name in ('', '.', '..') or Path(name).name != name:
                fail('non-relative runtime dependency SONAME')
            if name in aliases and aliases[name]['sha256'] != node['sha256']:
                fail('runtime DT_NEEDED identity/SONAME conflict: ' + name)
            aliases[name] = node
            if name not in node['aliases']:
                node['aliases'].append(name)
    for metadata in [product_metadata] + [node['elf'] for node in nodes.values()]:
        for needed in metadata['needed']:
            if needed not in aliases:
                fail('runtime DT_NEEDED closure is incomplete: ' + needed)
    # Bind every original DSO identity at the successful final-link boundary.
    # Assembly copies only these exact hashes, never whatever later resolves.
    for node in nodes.values():
        plan.input(node['origin'])
        node['aliases'].sort()
    return {'product_needed': product_metadata['needed'],
            'nodes': [nodes[key] for key in sorted(nodes)],
            'resolver': {'method': 'CMake file(GET_RUNTIME_DEPENDENCIES), linux+elf/objdump; complete recursive graph with explicit DSO-input parent directories',
                         'explicit_dso_directory_count': len(library_directories),
                         'cmake_version': settings['cmake_version'],
                         'cmake_sha256': digest(cmake), 'objdump_sha256': digest(objdump)}}


def assemble(settings_path, output_path):
    settings = load(settings_path)
    record = load(settings['capture'])
    if record.get('schema_version') != SCHEMA or record['settings_sha256'] != digest(settings_path):
        fail('successful final-link capture is absent/stale for this configure')
    if record['capture_helper_sha256'] != digest(Path(__file__)):
        fail('capture helper changed; force an actual instrumented final product relink')
    if digest(settings['product']) != record['product_sha256'] or digest(settings['lightning']) != record['lightning_sha256']:
        fail('actual product/library changed after captured final link; rebuild the instrumented product')
    api = file_api(settings, record)
    output = Path(output_path).resolve()
    if output.exists():
        fail('kit output already exists; retain it as evidence or explicitly remove it before generating a new kit')
    if within(output, settings['source']) or within(output, settings['project']) and not within(output, settings['build']):
        fail('private kit must not be generated into the source tree')
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='.private-relink-', dir=output.parent) as temporary:
        staging = Path(temporary) / 'kit'
        staging.mkdir()
        mapping = {}
        public_inputs = []
        default_names = {}
        runtime = {}
        closure = record['runtime_closure']
        closure_nodes = {node['origin']: node for node in closure['nodes']} if closure is not None else {}
        for item in record['inputs']:
            origin = Path(item['origin'])
            if not origin.is_file() or digest(origin) != item['sha256']:
                fail('captured application input is missing/changed: ' + item['name'])
            relative = Path('inputs') / item['sha256'][:20] / item['name']
            target = staging / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(origin, target)
            mapping[item['origin']] = relative.as_posix()
            public_inputs.append({key: item[key] for key in ('sha256', 'size', 'name', 'defaultlib')} |
                                 {'path': relative.as_posix()})
            if item['defaultlib']:
                key = item['name'].lower()
                if key in default_names and default_names[key] != item['sha256']:
                    fail('non-system DEFAULTLIB basename collision')
                default_names[key] = item['sha256']
                default = staging / 'inputs/defaultlibs' / item['name']
                default.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(origin, default)
            if settings['system'] == 'Linux' and (item['origin'] in closure_nodes or '.so' in origin.name):
                if item['origin'] not in closure_nodes:
                    fail('non-system DSO is absent from the captured complete runtime closure')
                node = closure_nodes[item['origin']]
                if node['sha256'] != item['sha256']:
                    fail('runtime DSO differs from captured dependency identity')
                metadata = node['elf']
                for name in node['aliases']:
                    if name in ('', '.', '..') or Path(name).name != name:
                        fail('unsafe ELF SONAME')
                    if name in runtime and runtime[name]['sha256'] != item['sha256']:
                        fail('runtime DSO basename/SONAME collision')
                    runtime[name] = {
                        'sha256': item['sha256'], 'size': item['size'], 'needed': metadata['needed'],
                        'original_name': node['name'], 'original_input': relative.as_posix(),
                        'original_runpath_sha256': hashlib.sha256(json.dumps(metadata['runpath']).encode()).hexdigest(),
                        'original_rpath_sha256': hashlib.sha256(json.dumps(metadata['rpath']).encode()).hexdigest()}
                    target = staging / 'runtime/lib' / name
                    target.parent.mkdir(parents=True, exist_ok=True)
                    shutil.copyfile(origin, target)
        def relocate(value):
            if isinstance(value, list):
                return [relocate(item) for item in value]
            if isinstance(value, dict):
                if 'input' in value:
                    return {'input': mapping[value['input']]}
                return {key: relocate(item) for key, item in value.items()}
            return value
        recipe = relocate(record['recipe'])
        if runtime and '$ORIGIN/lib' not in recipe:
            # Direct dependencies need runtime lookup even if capture had no RPATH.
            recipe.extend(['-Xlinker', '-rpath', '-Xlinker', '$ORIGIN/lib'])
        if runtime and not any(isinstance(value, dict) and value.get('runtime_dir') for value in recipe):
            # Replacing producer RPATHs must not lose ld's indirect-DSO search.
            recipe.extend(['-Xlinker', '-rpath-link', '-Xlinker', {'runtime_dir': True}])
        public_closure = None
        if closure is not None:
            public_closure = {
                'product_needed': closure['product_needed'], 'resolver': closure['resolver'],
                'nodes': [
                    {key: node[key] for key in ('name', 'sha256', 'size', 'system', 'aliases')} |
                    {'soname': node['elf']['soname'], 'needed': node['elf']['needed']} |
                    ({} if node['system'] else {'input': mapping[node['origin']]})
                    for node in closure['nodes']]}
        library_source(settings, staging, record, api)
        shutil.copyfile(Path(__file__), staging / 'runtime_relink.py')
        instructions = (Path(settings['project']) / 'cmake/RuntimeRelinkInstructions.txt.in').read_text(encoding='utf-8')
        instructions = instructions.replace('@CONFIGURATION@', settings['configuration']).replace('@MARKER@', MARKER)
        (staging / 'RELINK-INSTRUCTIONS.txt').write_text(instructions, encoding='utf-8')
        manifest = {
            'schema_version': SCHEMA, 'private_only': True, 'redistribution': 'R1 HOLD; no application/source/object rights granted or inferred',
            'r1_publication_approved': False,
            'candidate_product_sha256': record['product_sha256'],
            'candidate_lightning_sha256': record['lightning_sha256'],
            'source_provenance_sha256': record['source_provenance_sha256'],
            'captured_invocation_sha256': record['captured_invocation_sha256'],
            'captured_at_utc': record['captured_at_utc'],
            'toolchain': {key: settings[key] for key in ('system', 'processor', 'configuration', 'generator',
                'cmake_version', 'compiler_id', 'compiler_version', 'msvc_toolset', 'windows_sdk', 'crt')},
            'toolchain_files': record['toolchain_files'], 'driver_name': record['driver_name'],
            'object_boundary': 'Ordinary native objects only; detected GCC/LLVM/MSVC LTO and thin archives fail closed. Exact captured driver/linker/compiler hashes remain required, not a generic portability claim.',
            'file_api_crosscheck': api, 'recursive_response_inputs': record['responses'],
            'ordered_recipe': recipe, 'application_inputs': public_inputs,
            'system_inputs': record['system_inputs'], 'runtime_dsos': runtime,
            'runtime_dependency_graph': public_closure,
            'postlink': {'windows': 'Preserve captured /MANIFEST:NO and actual compiled .res; no mt.exe reinjection/signing invented.',
                         'linux': 'Relink uses executable-relative $ORIGIN/lib; copied reached non-system DSOs accompany it. Existing Runtime install remains separate.',
                         'assets': 'Use the matching private Runtime resource/notices bundle. No ROM, state, GAME source, whole build tree, or original executable is in this kit.'}}
        manifest['kit_files'] = {path.relative_to(staging).as_posix(): {'sha256': digest(path), 'size': path.stat().st_size}
                                 for path in sorted(staging.rglob('*')) if path.is_file()}
        # No producer path is permitted in generated JSON, response or build
        # recipe metadata. Opaque objects are intentionally private, not a
        # claim that embedded compiler debug records have been redacted.
        serialized = json.dumps(manifest, ensure_ascii=False).replace('\\', '/')
        for key in ('project', 'build', 'source'):
            if settings[key].replace('\\', '/') in serialized:
                fail('producer path escaped into kit metadata')
        save(staging / 'kit-manifest.json', manifest)
        staging.rename(output)
    print('PRIVATE whole-product relink kit created. Distribution remains R1 HOLD: ' + str(output))


def recipient_system_file(name, manifest, driver):
    if name in ('', '.', '..') or Path(name).name != name:
        fail('invalid System Library name in kit')
    if manifest['toolchain']['system'] == 'Windows':
        for directory in os.environ.get('LIB', '').split(';'):
            path = Path(directory) / name
            if directory and path.is_file():
                return path.resolve()
    else:
        value = subprocess.check_output([str(driver), '-print-file-name=' + name], text=True).strip()
        if Path(value).is_file():
            return Path(value).resolve()
    fail('matching external toolchain/System Library is unavailable: ' + name)


def verify_toolchain(manifest, driver):
    driver = Path(shutil.which(str(driver)) or driver).resolve()
    if not driver.is_file():
        fail('actual compiler/linker driver is unavailable')
    windows = manifest['toolchain']['system'] == 'Windows'
    if windows != (os.name == 'nt'):
        fail('relink must run on the captured native platform')
    for label, item in manifest['toolchain_files'].items():
        if label == 'driver':
            path = driver
        elif windows:
            path = driver.parent / item['name']
        elif label in ('lto_wrapper', 'lto_plugin'):
            option = '-print-prog-name=lto-wrapper' if label == 'lto_wrapper' else '-print-file-name=liblto_plugin.so'
            path = Path(subprocess.check_output([str(driver), option], text=True).strip())
        elif label == 'linker':
            path = native_driver_linker(driver)
        else:
            path = Path(shutil.which(item['name']) or '')
        if not path.is_file() or digest(path) != item['sha256']:
            fail('exact captured compiler/linker component required: ' + item['name'])
    system = {}
    for item in manifest['system_inputs']:
        path = recipient_system_file(item['name'], manifest, driver)
        if digest(path) != item['sha256']:
            fail('external System Library differs from the captured candidate: ' + item['name'])
        system[item['name']] = path
    return driver, system


def relink(kit_path, lightning_path, output_path, driver_path=None):
    kit = Path(kit_path).resolve()
    manifest = load(kit / 'kit-manifest.json')
    if manifest.get('schema_version') != SCHEMA or not manifest.get('private_only'):
        fail('not a supported private relink kit')
    windows = manifest['toolchain']['system'] == 'Windows'
    reject_implicit_link_environment(windows)
    reject_alternate_linker_selection(manifest['ordered_recipe'], windows)
    driver, system = verify_toolchain(manifest, driver_path or manifest['driver_name'])
    lightning = Path(lightning_path).resolve()
    output = Path(output_path).resolve()
    if not lightning.is_file() or lightning.suffix.lower() not in ('.a', '.lib'):
        fail('replacement slot requires a real independently built static lightning archive')
    native_objects(lightning, manifest['toolchain']['system'] == 'Windows')
    if output.exists():
        fail('relink output already exists; choose a new output so proof cannot reuse an old binary')
    if within(output, kit / 'inputs') or within(output, kit / 'library'):
        fail('relink must not overwrite the supplied application/library inputs')
    for item in manifest['application_inputs']:
        path = kit / item['path']
        if not within(path, kit / 'inputs') or digest(path) != item['sha256']:
            fail('application input is absent/changed: ' + item['name'])
    for relative, item in manifest['kit_files'].items():
        if relative.startswith(('runtime/', 'inputs/defaultlibs/')):
            path = kit / relative
            if not within(path, kit) or digest(path) != item['sha256']:
                fail('runtime/default-library companion input is absent/changed')
    output.parent.mkdir(parents=True, exist_ok=True)
    runtime_dir = output.parent / 'lib'
    if manifest['runtime_dsos']:
        if not shutil.which('patchelf'):
            fail('patchelf is required to stage relocatable copies of the non-system Runtime DSOs')
        runtime_dir.mkdir(exist_ok=True)
        for name, item in manifest['runtime_dsos'].items():
            source = kit / 'runtime/lib' / name
            if digest(source) != item['sha256']:
                fail('runtime DSO input hash mismatch')
            target = runtime_dir / name
            if target.exists():
                fail('refusing to overwrite an existing runtime DSO: ' + name)
            shutil.copyfile(source, target)
            subprocess.run(['patchelf', '--set-rpath', '$ORIGIN', str(target)], check=True)
    def render(value):
        if isinstance(value, str):
            return value
        if 'input' in value:
            return str(kit / value['input'])
        if 'system' in value:
            name = value['system']
            return str(system.get(name) or recipient_system_file(name, manifest, driver))
        if 'lightning' in value:
            return str(lightning)
        if 'output' in value:
            return str(output)
        if 'output_option' in value:
            return value['output_option'] + str(output)
        if 'side_output' in value:
            return value['side_output'] + str(output.parent / value['name'])
        if 'runtime_dir' in value:
            return str(runtime_dir)
        if 'prefix' in value:
            return value['prefix'] + render(value['value'])
        fail('unknown link recipe token')
    arguments = [render(value) for value in manifest['ordered_recipe']]
    if manifest['toolchain']['system'] == 'Windows':
        # Embedded plain-name DEFAULTLIB directives keep their linker semantics.
        arguments.insert(0, '/LIBPATH:' + str(kit / 'inputs/defaultlibs'))
        response = subprocess.list2cmdline(arguments)
        encoding = 'utf-16'
    else:
        response = '\n'.join(shlex.quote(value) for value in arguments) + '\n'
        encoding = 'utf-8'
    response_path = output.with_name(output.name + '.relink.rsp')
    response_path.write_text(response, encoding=encoding)
    # Deliberately no shell, original source/build cwd, old EXE, or ROM.
    subprocess.run([str(driver), '@' + str(response_path)], cwd=kit, check=True)
    if not output.is_file():
        fail('actual final linker did not produce the whole product')
    receipt = {
        'schema_version': SCHEMA, 'candidate_product_sha256': manifest['candidate_product_sha256'],
        'relinked_product_sha256': digest(output), 'replacement_lightning_sha256': digest(lightning),
        'candidate_lightning_sha256': manifest['candidate_lightning_sha256'],
        'replacement_differs_from_original_archive': digest(lightning) != manifest['candidate_lightning_sha256'],
        'ordered_recipe_sha256': hashlib.sha256(json.dumps(manifest['ordered_recipe'], sort_keys=True).encode()).hexdigest(),
        'link_response_sha256': digest(response_path), 'toolchain_verified': True,
        'runtime_dsos': {name: {'original_sha256': item['sha256'], 'staged_sha256': digest(runtime_dir / name),
                                'staged_runpath': '$ORIGIN'} for name, item in manifest['runtime_dsos'].items()},
        'proof_limit': 'Actual whole-product link only. Execute the synthetic JIT and genuine product separately; this receipt is not runtime/R1 approval.'}
    save(output.with_name(output.name + '.relink-receipt.json'), receipt)
    print('Linked actual whole product with the supplied lightning archive: ' + str(output))


def mark_proof(library_path):
    path = Path(library_path).resolve() / 'lightning/lib/lightning.c'
    text = path.read_text(encoding='utf-8')
    if MARKER in text:
        fail('proof modification already present; do not confuse repeated edits with an independent rebuild')
    needle = 'init_jit(const char *progname)\n{'
    if text.count(needle) != 1:
        fail('exact delivered init_jit hook is absent; no proof modification applied')
    before = digest(path)
    text = '#include <stdio.h>\n' + text.replace(
        needle, needle + '\n    fputs("' + MARKER + '\\n", stderr);\n    fflush(stderr);', 1)
    path.write_text(text, encoding='utf-8')
    save(Path(library_path) / 'proof-modification.json', {
        'path': 'lightning/lib/lightning.c', 'before_sha256': before, 'after_sha256': digest(path),
        'marker': MARKER, 'purpose': 'Real interface-compatible library modification, visible from actual init_jit callers; not a synthetic replacement result.'})
    print('Modified actual delivered library source. Rebuild it before both JIT and whole-product proofs.')


def prove(program, arguments, evidence_path):
    """Observe a real process; never inject the library marker into its output."""
    program = Path(program).resolve()
    process = subprocess.run([str(program)] + arguments, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    sys.stdout.buffer.write(process.stdout)
    sys.stderr.buffer.write(process.stderr)
    found = MARKER.encode('ascii') in process.stderr
    save(evidence_path, {'schema_version': SCHEMA, 'program_sha256': digest(program),
                         'returncode': process.returncode, 'actual_stderr_marker_observed': found,
                         'stdout_sha256': hashlib.sha256(process.stdout).hexdigest(),
                         'stderr_sha256': hashlib.sha256(process.stderr).hexdigest(),
                         'proof_limit': 'Observed this real process only; product behavior, admitted inputs and platform qualification remain parent-owned.'})
    if process.returncode or not found:
        fail('real process failed or did not execute the modified library init_jit marker')


def main():
    # Redirected Windows streams otherwise use a locale code page for Unicode paths.
    sys.stdout.reconfigure(encoding='utf-8')
    sys.stderr.reconfigure(encoding='utf-8')
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest='command', required=True)
    capture_parser = commands.add_parser('capture')
    capture_parser.add_argument('--settings', required=True, type=Path)
    capture_parser.add_argument('link_command', nargs=argparse.REMAINDER)
    assemble_parser = commands.add_parser('assemble')
    assemble_parser.add_argument('--settings', required=True, type=Path)
    assemble_parser.add_argument('--output', required=True, type=Path)
    relink_parser = commands.add_parser('relink')
    relink_parser.add_argument('--kit', default=Path(__file__).resolve().parent, type=Path)
    relink_parser.add_argument('--lightning', required=True, type=Path)
    relink_parser.add_argument('--output', required=True, type=Path)
    relink_parser.add_argument('--driver', type=Path)
    mark_parser = commands.add_parser('mark-proof')
    mark_parser.add_argument('--library', required=True, type=Path)
    proof_parser = commands.add_parser('prove')
    proof_parser.add_argument('--program', required=True, type=Path)
    proof_parser.add_argument('--evidence', required=True, type=Path)
    proof_parser.add_argument('program_arguments', nargs=argparse.REMAINDER)
    args = parser.parse_args()
    try:
        if args.command == 'capture':
            command = args.link_command
            return capture(args.settings, command[1:] if command[:1] == ['--'] else command)
        if args.command == 'assemble':
            assemble(args.settings, args.output)
        elif args.command == 'relink':
            relink(args.kit, args.lightning, args.output, args.driver)
        elif args.command == 'mark-proof':
            mark_proof(args.library)
        else:
            arguments = args.program_arguments
            prove(args.program, arguments[1:] if arguments[:1] == ['--'] else arguments, args.evidence)
        return 0
    except (OSError, ValueError, KeyError, TypeError, IndexError, struct.error, subprocess.CalledProcessError) as error:
        parser.exit(1, 'Private runtime relink failed closed: %s\n' % error)


if __name__ == '__main__':
    sys.exit(main())
