#!/usr/bin/env python3
"""Bounded install-time candidate identities, not redistribution approval.

The private CMake input file contains local paths; the output deliberately does
not. Only public selected dependency trees are fingerprinted. No GAME source,
ROM, profile or state is read. capture-sdk binds installed DSOs to an actual
public source build; capture-deb-sdk binds selected extracted DSOs to exact
retained Debian archive members without claiming source-build verification.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import stat
import subprocess
import sys
import tarfile


class CompositionError(Exception):
    pass


def require(condition, message):
    if not condition:
        raise CompositionError(message)


def digest(path):
    require(Path(path).is_file(), 'Required identity file is missing')
    h = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            h.update(block)
    return h.hexdigest()


def hash_bytes(data):
    return hashlib.sha256(data).hexdigest()


def text(value, label):
    require(isinstance(value, str) and value and len(value) <= 256 and
            not re.search(r'[\x00-\x1f\x7f/\\]', value),
            'Missing or unsafe ' + label)
    return value


def sha(value):
    require(isinstance(value, str) and re.fullmatch(r'[0-9a-f]{64}', value),
            'Required SHA256 identity is missing or invalid')
    return value


def commit(value):
    require(isinstance(value, str) and re.fullmatch(r'[0-9a-f]{40}', value),
            'Required git commit identity is missing or invalid')
    return value


def relative(value):
    require(isinstance(value, str) and value and '\\' not in value and
            not re.search(r'[\x00-\x1f\x7f:]', value), 'Unsafe relative payload path')
    path = PurePosixPath(value)
    require(not path.is_absolute() and '..' not in path.parts and
            path.as_posix() == value, 'Noncanonical relative payload path')
    return value


def repository(value):
    require(isinstance(value, str) and re.fullmatch(
        r'https://github\.com/[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+', value),
        'Source repository must be an uncredentialed public GitHub identity')
    return value


def load(path):
    require(Path(path).is_file(), 'Required composition input is missing')
    return json.loads(Path(path).read_text(encoding='utf-8'))


def run(arguments, binary=False):
    result = subprocess.run([str(arg) for arg in arguments], stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE, check=False,
                            env=dict(os.environ, LC_ALL='C'))
    # Never propagate subprocess errors containing private command paths.
    require(result.returncode == 0, 'Required identity metadata command failed')
    return result.stdout if binary else result.stdout.decode('utf-8').strip()


def git_identity(git, root, expected=None, include_tree=False):
    root = Path(root)
    head = commit(run([git, '-C', root, 'rev-parse', 'HEAD']))
    if expected:
        require(head == expected, 'Actual selected public dependency commit differs from pin')
    # --no-ext-diff/--no-textconv avoid executing repository-defined diff helpers.
    delta = run([git, '-C', root, 'diff', '--no-ext-diff', '--no-textconv',
                 '--binary', 'HEAD', '--'], binary=True)
    result = {'commit': head, 'tracked_changes': bool(delta),
              'tracked_diff_sha256': hash_bytes(delta),
              'untracked_files': 'not included; selected generated inputs recorded separately'}
    if include_tree:
        names = run([git, '-C', root, 'ls-files', '-z'], binary=True)
        paths = [name.decode('utf-8') for name in names.split(b'\0') if name]
        result['selected_tree'] = tree_identity(root, paths)
    return result


def tree_identity(root, names=None):
    root = Path(root).resolve()
    require(root.is_dir(), 'Required selected public source tree is missing')
    if names is None:
        names = []
        for directory, directories, files in os.walk(root, followlinks=False):
            directories[:] = sorted(name for name in directories
                                    if name not in ('.git', '__pycache__'))
            require(not any((Path(directory) / name).is_symlink() for name in directories),
                    'Symlink directory in selected public source tree')
            names.extend((Path(directory) / name).relative_to(root).as_posix() for name in files)
    records = []
    total = 0
    for name in sorted(set(names)):
        relative(name)
        path = root / name
        require(not path.is_symlink(), 'Symlink in selected public source identity')
        require(root in path.resolve().parents or path.resolve() == root,
                'Selected public source input escapes its bounded root')
        if path.is_dir():
            continue
        require(path.is_file(), 'Missing tracked selected public dependency input')
        total += path.stat().st_size
        require(len(records) < 100000 and total <= 2 * 1024 ** 3,
                'Selected public source identity exceeds bounded scope')
        records.append([name, digest(path)])
    require(records, 'Empty selected public source identity')
    encoded = json.dumps(records, ensure_ascii=True, separators=(',', ':')).encode('ascii')
    return {'algorithm': 'sha256 of sorted JSON [relative-path,file-sha256] pairs',
            'sha256': hash_bytes(encoded), 'files': len(records), 'bytes': total}


def executable_identity(path, role):
    require(path and Path(path).is_file(), 'Required ' + role + ' executable is missing')
    return {'sha256': digest(path), 'identity_scope': 'selected executable bytes, not full tool installation'}


def debian_owner(path, required=True):
    path = Path(path).resolve()
    candidates = [str(path)]
    if str(path).startswith('/usr/lib/') or str(path).startswith('/usr/lib64/'):
        candidates.append(str(path)[4:])
    owner = None
    for candidate in candidates:
        result = subprocess.run(['dpkg-query', '--search', candidate], stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE, env=dict(os.environ, LC_ALL='C'))
        if result.returncode:
            continue
        lines = result.stdout.decode('utf-8').strip().splitlines()
        if len(lines) == 1:
            package, separator, owned = lines[0].partition(': ')
            if separator and owned == candidate and re.fullmatch(r'[a-z0-9][a-z0-9+.-]*(?::[a-z0-9-]+)?', package):
                owner = package
                break
    if not owner:
        require(not required, 'Selected system input has no unambiguous Debian owner')
        return None
    fields = run(['dpkg-query', '--show', '--showformat=${binary:Package}\n${Version}\n'
                  '${source:Package}\n${source:Version}\n${Architecture}\n${db:Status-Status}', owner]).splitlines()
    require(len(fields) == 6 and fields[-1] == 'installed', 'Incomplete installed Debian identity')
    return dict(kind='debian-installed-package', **{key: text(value, key) for key, value in zip(
        ('binary_package', 'binary_version', 'source_package', 'source_version', 'architecture'), fields[:5])})


def prepared_identity(inputs, kind):
    root = Path(inputs[kind + '_root'])
    source_root = Path(inputs['source_root'])
    receipt = load(root / 'provenance.json')
    manifest = source_root / 'cmake' / (kind + '-dependencies.json')
    generator = source_root / 'tools' / ('prepare_' + kind + '.py')
    require(receipt.get('manifest_sha256') == digest(manifest) and
            receipt.get('generator_sha256') == digest(generator),
            'Prepared public dependency receipt does not match selected recipe')
    require(receipt.get('dependencies') == load(manifest)['dependencies'],
            'Prepared dependency set does not match selected manifest')
    if kind == 'rsp':
        require(receipt.get('helper_sha256') == digest(source_root / 'tools/prepare_rdp.py'),
                'RSP preparation helper identity changed')
    dependencies = []
    for item in receipt['dependencies']:
        dependencies.append({'name': text(item['name'], 'prepared dependency name'),
                             'repository': repository('https://github.com/' + item['repository']),
                             'commit': commit(item['commit']),
                             'archive_sha256': sha(item['archive_sha256'])})
    adaptation_root = root / ('source' if kind == 'rsp' else 'compiler')
    adaptations = []
    for item in receipt.get('adaptations', []):
        name = relative(item['path'])
        require(digest(adaptation_root / name) == item['adapted_sha256'],
                'Actual prepared source differs from its adaptation receipt')
        adaptations.append({'path': name, 'original_sha256': sha(item['original_sha256']),
                            'adapted_sha256': sha(item['adapted_sha256'])})
    require(adaptations, 'Required source adaptations have no actual receipt')
    trees = ('source',) if kind == 'rsp' else ('renderer', 'compiler')
    result = {'manifest_sha256': digest(manifest), 'recipe_sha256': digest(generator),
              'receipt_sha256': digest(root / 'provenance.json'), 'dependencies': dependencies,
              'adaptations': adaptations,
              'selected_trees': {name: tree_identity(root / name) for name in trees}}
    if kind == 'rdp':
        result['generated_shader_sha256'] = digest(root / 'renderer/parallel-rdp/shaders/slangmosh.hpp')
        result['compiler_scope'] = 'build-only shader compiler; not a claim all compiler inputs are runtime-linked'
    return result


def source_identities(inputs):
    root = Path(inputs['source_root'])
    manifest = load(root / 'cmake/dependencies.json')
    git = inputs['git']
    lus = Path(inputs['lus_source'])
    result = {'project': git_identity(git, root),
              'dependency_manifest_sha256': digest(root / 'cmake/dependencies.json')}
    lus_pin = manifest['lus']['commit']
    lus_identity = git_identity(git, inputs['lus_input'], lus_pin)
    require(not lus_identity['tracked_changes'], 'Pinned LUS input changed since configuration')
    names = Path(inputs['lus_inventory']).read_text(encoding='utf-8').splitlines()
    lus_identity.update({'selected_prepared_tree': tree_identity(lus, names),
                         'archive_sha256': digest(inputs['lus_archive']),
                         'host_patch_sha256': digest(root / 'cmake/patches/lus-host-lifetime.patch'),
                         'imgui_patch_sha256': digest(lus / 'cmake/dependencies/patches/imgui-fixes-and-config.patch'),
                         'preparation_recipe_sha256': digest(root / 'cmake/Libultraship.cmake')})
    result['libultraship'] = lus_identity
    pins = {entry['name'].lower(): entry for entry in manifest['enabled_host_common_inputs']}
    fetched = {}
    for selected in inputs['fetched']:
        name = selected['name']
        fetched[name] = git_identity(git, selected['source'], pins[name.lower()]['resolved_commit'], True)
        require(name == 'ImGui' or not fetched[name]['tracked_changes'],
                'Unexpected changes in unpatched fetched dependency')
    require(len(fetched) == 4, 'Incomplete selected FetchContent dependency identities')
    result['fetched'] = fetched
    stb = pins['stb_image.h']
    header = Path(inputs['build_root']) / '_deps/stb/stb_image.h'
    require(digest(header) == stb['content_sha256'],
            'Actual selected stb header identity is missing or changed')
    result['stb_image'] = {'commit': commit(stb['resolved_commit']),
                           'sha256': sha(stb['content_sha256']),
                           'generated_implementation_sha256': digest(header.with_name('stb_impl.c'))}
    result['parallel_rsp'] = prepared_identity(inputs, 'rsp')
    result['parallel_rdp'] = prepared_identity(inputs, 'rdp')
    # Authority metadata only: never open or export private GAME materializations.
    game_manifest = root / 'cmake/snap-engine-inputs.json'
    game = load(game_manifest)
    result['game_authority'] = {'repository': repository(game['repository']),
                              'revision': commit(game['revision']),
                              'manifest_sha256': digest(game_manifest),
                              'scope': 'private-development authority metadata only; no redistribution grant'}
    return result


def parse_control(content):
    records = []
    current = {}
    for line in content.splitlines() + ['']:
        if not line:
            if current:
                records.append(current)
                current = {}
        elif line[0].isspace():
            continue
        else:
            key, separator, value = line.partition(': ')
            require(separator, 'Invalid installed package status metadata')
            current[key] = value
    return records


def windows_sdk_root(selected_root):
    if selected_root:
        return Path(selected_root)
    # A VS-generator configure need not inherit a developer-shell environment.
    # Query only the public SDK installation root; never export registry paths.
    import winreg
    for view in (winreg.KEY_WOW64_32KEY, winreg.KEY_WOW64_64KEY):
        try:
            with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE,
                                r'SOFTWARE\Microsoft\Windows Kits\Installed Roots',
                                0, winreg.KEY_READ | view) as key:
                value, _ = winreg.QueryValueEx(key, 'KitsRoot10')
                if value:
                    return Path(value)
        except OSError:
            continue
    raise CompositionError('Actual selected Windows SDK installation is missing')


def windows_identity(inputs):
    version = inputs['windows_sdk_version'] or inputs['windows_sdk_environment_version'].rstrip('\\/')
    require(re.fullmatch(r'10\.0\.[0-9]+\.[0-9]+', version or ''),
            'Actual selected Windows SDK version is required')
    sdk_root = windows_sdk_root(inputs['windows_sdk_root'])
    sdk = {'version': version, 'identity_scope': 'selected SDK version and boundary files, not complete Windows OS',
           'boundary_sha256': {name: digest(sdk_root / name) for name in (
               'Include/' + version + '/um/Windows.h',
               'Include/' + version + '/ucrt/corecrt.h',
               'Lib/' + version + '/um/x64/kernel32.lib',
               'Lib/' + version + '/ucrt/x64/libucrt.lib')}}
    require(inputs['msvc_runtime'] in ('MultiThreaded', 'MultiThreadedDebug'),
            'Windows candidate must record the actual selected static MSVC runtime')
    root = Path(inputs['vcpkg_root'])
    require(inputs['vcpkg_root'] and inputs['vcpkg_installed'], 'Actual vcpkg roots are required privately')
    manifest = load(Path(inputs['source_root']) / 'cmake/dependencies.json')['platform_package_inputs']['windows']
    source = git_identity(inputs['git'], root, manifest['source_and_registry_commit'])
    require(not source['tracked_changes'], 'Selected vcpkg tracked source differs from frozen pin')
    tool_hash = digest(root / 'vcpkg.exe')
    require(tool_hash == manifest['tool']['sha256'], 'Actual selected vcpkg executable differs from enforced tool')
    status = Path(inputs['vcpkg_installed']) / 'vcpkg/status'
    triplet = text(inputs['vcpkg_triplet'], 'vcpkg target triplet')
    installed = [record for record in parse_control(status.read_text(encoding='utf-8'))
                 if record.get('Status') == 'install ok installed']
    versions = {(record['Package'], record['Architecture']): record['Version']
                for record in installed if 'Version' in record}
    ports = []
    for record in installed:
        port = {key.lower(): text(record[key], 'installed vcpkg ' + key)
                for key in ('Package', 'Architecture')}
        # Feature paragraphs inherit the version of their actually installed
        # base package; vcpkg does not repeat Version in feature paragraphs.
        port['version'] = text(versions[(record['Package'], record['Architecture'])],
                               'installed vcpkg version')
        for key in ('Port-Version', 'Feature', 'Abi'):
            if key in record:
                port[key.lower()] = text(record[key], 'installed vcpkg ' + key)
        ports.append(port)
    selected = {port['package'] for port in ports if port['architecture'] == triplet}
    require({'sdl3', 'libzip', 'nlohmann-json', 'tinyxml2', 'spdlog', 'fmt', 'glew'} <= selected,
            'Required actually installed vcpkg package/version records are missing')
    return {'windows_sdk': sdk, 'msvc_toolset': text(inputs['msvc_toolset'], 'MSVC toolset'),
            'msvc_runtime': inputs['msvc_runtime'],
            'vcpkg': {'source': source, 'tool_sha256': tool_hash, 'target_triplet': triplet,
                      'installed_status_sha256': digest(status), 'installed_packages': ports,
                      'scope': 'actual installed ports/features, including build helpers; not a static-link object census'}}


def selected_packages(inputs):
    result = []
    for item in inputs['packages']:
        entry = {'name': text(item['name'], 'selected package name')}
        if item['version']:
            entry['cmake_reported_version'] = text(item['version'], 'CMake selected package version')
        directory = item['config_directory']
        files = []
        if directory and not directory.endswith('-NOTFOUND'):
            files = sorted(path for path in Path(directory).glob('*.cmake')
                           if 'config' in path.name.lower())
        entry['selected_config_files'] = []
        for path in files:
            record = {'name': text(path.name, 'package config basename'), 'sha256': digest(path)}
            if inputs['system'] == 'Linux' and str(path.resolve()).startswith(('/usr/', '/lib/')):
                record['owner'] = debian_owner(path)
            entry['selected_config_files'].append(record)
        require(entry.get('cmake_reported_version') or entry['selected_config_files'] or
                inputs['system'] == 'Windows', 'Selected package has no version/config identity')
        result.append(entry)
    return result


SDK_SCHEMA = 2
SDK_RECORD_KIND = 'captured-runtime-sdk-identities'
DEB_SOURCE_SCOPE = ('Package-declared source identity only; source archives, distro patches, '
                    'build environment and binary-to-source correspondence not verified')
DEB_AUTH_SCOPE = ('Archive bytes identified by SHA256; APT origin, signatures and repository '
                  'authentication not verified by this capture')


def checked_member(value):
    require(isinstance(value, str), 'Missing Debian archive member path')
    relative(value[2:] if value.startswith('./') else value)
    return value


def positive_size(value, label):
    require(type(value) is int and value > 0, 'Missing or invalid ' + label)
    return value


def checked_sdk_identity(identity, original_sha256):
    kind = identity.get('kind')
    if kind == 'debian-binary-package':
        require(identity.get('dso_sha256') == original_sha256,
                'Debian SDK receipt DSO hash differs from its lookup key')
        package = {}
        for key in ('binary_package', 'binary_version', 'source_package', 'source_version', 'architecture'):
            package[key] = text(identity[key], 'Debian SDK ' + key)
        for key in ('binary_package', 'source_package'):
            require(re.fullmatch(r'[a-z0-9][a-z0-9+.-]*', package[key]),
                    'Invalid Debian SDK package name')
        require(type(identity['source_field_present']) is bool,
                'Debian Source field presence must be recorded')
        require(identity['source_verification'] == DEB_SOURCE_SCOPE and
                identity['archive_authentication'] == DEB_AUTH_SCOPE,
                'Unsupported Debian SDK verification claim')
        return dict(package, kind=kind,
                    archive={'sha256': sha(identity['archive']['sha256']),
                             'bytes': positive_size(identity['archive']['bytes'], 'Debian archive size')},
                    member_path=checked_member(identity['member_path']),
                    dso_sha256=sha(identity['dso_sha256']),
                    dso_bytes=positive_size(identity['dso_bytes'], 'Debian member size'),
                    source_field_present=identity['source_field_present'],
                    source_verification=DEB_SOURCE_SCOPE,
                    archive_authentication=DEB_AUTH_SCOPE)
    require(kind == 'captured-public-source-sdk', 'Unsupported captured SDK identity kind')
    source = identity['source']
    build = identity['build']
    require(isinstance(source['tracked_changes'], bool),
            'SDK tracked-change status must be captured explicitly')
    return {
        'kind': kind, 'name': text(identity['name'], 'SDK project name'),
        'version': text(identity['version'], 'SDK project version'),
        'source': {'repository': repository(source['repository']),
                   'commit': commit(source['commit']),
                   'tracked_changes': source['tracked_changes'],
                   'tracked_diff_sha256': sha(source['tracked_diff_sha256']),
                   'selected_tree': checked_tree(source['selected_tree'])},
        'build': {'cache_sha256': sha(build['cache_sha256']),
                  'cmake_sha256': sha(build['cmake_sha256']),
                  'cmake_version': text(build['cmake_version'], 'SDK CMake version'),
                  'compiler': {'sha256': sha(build['compiler']['sha256']),
                               'id': text(build['compiler']['id'], 'SDK compiler ID'),
                               'version': text(build['compiler']['version'], 'SDK compiler version')},
                  'build_type': text(build['build_type'], 'SDK build type'),
                  'recipe_sha256': sha(build['recipe_sha256'])}}


def sdk_receipt(path):
    receipt = load(path)
    require(receipt.get('schema_version') == SDK_SCHEMA and
            receipt.get('record_kind') == SDK_RECORD_KIND,
            'Unsupported SDK identity schema; recapture with this generator')
    sha(receipt['generator_sha256'])
    require(isinstance(receipt['libraries'], dict) and 0 < len(receipt['libraries']) <= 256,
            'Missing or excessive SDK library identities')
    return {sha(key): checked_sdk_identity(value, key)
            for key, value in receipt['libraries'].items()}


def sdk_libraries(inputs, libraries):
    sdk = None
    for library in libraries:
        relative(library['path'])
        sha(library['original_sha256'])
        sha(library['staged_sha256'])
        sha(library['original_rpath_sha256'])
        sha(library['rewrite_tool_sha256'])
        require(library['staged_rpath'] == '$ORIGIN', 'Staged DSO lacks relative RUNPATH')
        text(library['soname'], 'DSO SONAME')
        owner = library['owner']
        if owner.get('kind') == 'debian-installed-package':
            for key in ('binary_package', 'binary_version', 'source_package', 'source_version', 'architecture'):
                text(owner[key], 'DSO Debian ' + key)
        else:
            require(owner.get('kind') == 'external-sdk-requires-exact-identity',
                    'Unexpected non-dpkg Runtime owner classification')
            if sdk is None:
                require(inputs['sdk_identities'], 'Reached non-dpkg DSO requires captured SDK identities')
                sdk = sdk_receipt(inputs['sdk_identities'])
            identity = sdk.get(library['original_sha256'])
            require(identity, 'Reached original SDK DSO SHA256 has no exact captured identity')
            library['owner'] = identity
    return libraries


def checked_tree(tree):
    require(isinstance(tree['files'], int) and tree['files'] > 0 and
            isinstance(tree['bytes'], int) and tree['bytes'] >= 0,
            'SDK selected tree census is missing')
    return {'algorithm': 'sha256 of sorted JSON [relative-path,file-sha256] pairs',
            'sha256': sha(tree['sha256']), 'files': tree['files'], 'bytes': tree['bytes']}


def payload_inventory(stage, manifest_name):
    result = []
    for directory, directories, files in os.walk(stage, followlinks=False):
        directories.sort()
        # Include symlink directories as links, never walk through them.
        links = [name for name in directories if (Path(directory) / name).is_symlink()]
        directories[:] = [name for name in directories if name not in links]
        for name in sorted(files + links):
            path = Path(directory) / name
            name = relative(path.relative_to(stage).as_posix())
            if name == manifest_name:
                continue
            mode = path.lstat().st_mode
            if stat.S_ISLNK(mode):
                target = os.readlink(path)
                require(not os.path.isabs(target) and '\\' not in target,
                        'Absolute/nonportable Runtime symlink target')
                resolved = path.resolve(strict=True)
                require(stage in resolved.parents, 'Runtime symlink escapes payload root')
                result.append({'path': name, 'kind': 'symlink', 'target': target,
                               'sha256': digest(resolved), 'hash_scope': 'resolved target file bytes'})
            else:
                require(stat.S_ISREG(mode), 'Nonregular Runtime payload entry')
                result.append({'path': name, 'kind': 'file', 'bytes': path.stat().st_size,
                               'sha256': digest(path)})
    require(result, 'Cannot record empty Runtime payload')
    return sorted(result, key=lambda item: item['path'])


def prerequisites(system):
    common = {'identity_scope': 'bounded candidate inputs; no fully locked OS or driver closure claim',
              'graphics': ['Working OpenGL implementation for the LUS frontend',
                           'Vulkan loader and compatible ICD/device driver for parallel-RDP; dynamically loaded'],
              'audio': 'SDL audio backend and usable OS output/device permissions; physical hearing not established',
              'devices': 'OS display/input/controller/audio services and permissions remain external',
              'user_rom': 'Supported user-supplied ROM remains external; no ROM identity or content recorded',
              'runtime_qualification': 'Not established by composition generation'}
    if system == 'Linux':
        common['platform'] = ['x86-64 ELF loader and compatible glibc/libm',
                              'Compatible libstdc++/libgcc runtime and required symbol versions',
                              'X11/Wayland/display, graphics/audio/device backend libraries used by SDL remain platform inputs']
    else:
        common['platform'] = ['Windows x64 OS and system DLL APIs used by the product',
                              'System OpenGL/D3DCompiler services and graphics driver',
                              'vulkan-1.dll loader plus compatible ICD when parallel-RDP is used']
    return common


def generate(inputs_path, stage_path, libraries_json):
    inputs = load(inputs_path)
    system = inputs['system']
    require(system in ('Linux', 'Windows') and inputs['pointer_bytes'] == '8',
            'Only actual Linux/Windows x64 candidate composition is supported')
    stage = Path(stage_path).resolve()
    require(stage.is_dir(), 'Runtime staging root is missing')
    manifest_name = relative(inputs['manifest_relative'])
    output = stage / manifest_name
    require(stage in output.parent.resolve().parents or output.parent.resolve() == stage,
            'Manifest directory escapes Runtime staging root')
    # Do not let a failed reinstall leave an old apparently current record.
    if output.exists() or output.is_symlink():
        require(not output.is_symlink(), 'Manifest path must not be a symlink')
        output.unlink()
    configuration = inputs['configuration']
    if not configuration:
        require(inputs['multi_configuration'] == 'false',
                'Actual multi-config build configuration is missing')
        configuration = 'unspecified single-config build'
    build = {'system': system, 'processor': text(inputs['processor'], 'target processor'),
             'configuration': text(configuration, 'actual build configuration'),
             'generator': text(inputs['generator'], 'CMake generator'),
             'cmake': dict(executable_identity(inputs['cmake'], 'CMake'),
                           version=text(inputs['cmake_version'], 'CMake version'))}
    for language in ('c', 'cxx'):
        compiler = executable_identity(inputs[language + '_compiler'], language + ' compiler')
        compiler.update(id=text(inputs[language + '_id'], 'compiler ID'),
                        version=text(inputs[language + '_version'], 'compiler version'),
                        selected_flags_sha256=hash_bytes(inputs[language.upper() + '_flags'].encode('utf-8')))
        compiler['target'] = (text(inputs[language + '_target'], 'compiler target')
                              if inputs[language + '_target'] else 'native compiler default')
        compiler['architecture_id'] = (text(inputs[language + '_architecture'], 'compiler architecture')
                                       if inputs[language + '_architecture'] else 'not reported by CMake')
        build[language + '_compiler'] = compiler
    for role in ('linker', 'archiver'):
        build[role] = executable_identity(inputs[role], role)
    build['toolchain'] = ({'kind': 'selected-CMake-toolchain-file', 'sha256': digest(inputs['toolchain'])}
                          if inputs['toolchain'] else {'kind': 'native compiler discovery; no CMake toolchain file selected'})
    if system == 'Windows':
        build.update(windows_identity(inputs))
    else:
        build['windows_sdk'] = {'state': 'not-applicable-to-Linux'}
        build['tool_owning_packages'] = {role: debian_owner(inputs[role], required=False)
                                       for role in ('c_compiler', 'cxx_compiler', 'linker', 'archiver', 'cmake')}
        # Non-dpkg tools have exact executable hashes, not fabricated package identities.
        build['tool_owning_packages'] = {role: owner if owner else {'state': 'not-owned-by-dpkg; executable hash recorded'}
                                       for role, owner in build['tool_owning_packages'].items()}
    libraries = json.loads(libraries_json)
    require(isinstance(libraries, list) and (system != 'Linux' or libraries),
            'Actual Linux library staging metadata is required')
    libraries = sdk_libraries(inputs, libraries)
    inventory = payload_inventory(stage, manifest_name)
    indexed = {entry['path']: entry for entry in inventory}
    for key in ('product_relative', 'checker_relative'):
        require(relative(inputs[key]) in indexed, 'Required installed Runtime executable is missing')
    notice_prefix = relative(inputs['notice_relative']) + '/'
    require(any(name.startswith(notice_prefix) for name in indexed), 'Runtime notices have not been staged')
    for library in libraries:
        require(library['path'] in indexed and indexed[library['path']]['sha256'] == library['staged_sha256'],
                'Final staged DSO bytes differ from Linux rewrite receipt')
    root = Path(inputs['source_root'])
    record = {'schema_version': 2, 'record_kind': 'actual-install-time-Runtime-candidate-composition',
              'project': {'name': text(inputs['project_name'], 'project name'),
                          'version': text(inputs['project_version'], 'project version')},
              'scope': 'Observed selected public source/preparation/build metadata and final staged bytes; not an object-level reproducible-build attestation',
              'release_gates': {'r1_redistribution_approval': 'NOT APPROVED by this record',
                                'r2_complete_compliance_approval': 'NOT APPROVED by this record',
                                'publication': 'HOLD pending separate rights, source/relink, notice and runtime gates'},
              'build': build, 'selected_sources': source_identities(inputs),
              'selected_packages': selected_packages(inputs), 'linux_bundled_libraries': libraries,
              'platform_prerequisites': prerequisites(system),
              'generator_sha256': digest(Path(__file__)),
              'cmake_recipe_sha256': {name: digest(root / name) for name in (
                  'CMakeLists.txt', 'cmake/RuntimeComposition.cmake',
                  'cmake/RuntimeComposition.cmake.in', 'cmake/LinuxRuntime.cmake.in')},
              'inventory_scope': 'Every final staged file/symlink, including notices/resources; excludes only this manifest itself',
              'payload': inventory}
    output.parent.mkdir(parents=True, exist_ok=True)
    require(stage in output.parent.resolve().parents or output.parent.resolve() == stage,
            'Manifest directory escapes Runtime staging root')
    with output.open('x', encoding='utf-8') as stream:
        json.dump(record, stream, indent=2, sort_keys=True, ensure_ascii=True)
        stream.write('\n')


def cache_entries(path):
    result = {}
    for line in Path(path).read_text(encoding='utf-8').splitlines():
        match = re.match(r'^([^#/:][^:=]*):[^=]+=(.*)$', line)
        if match:
            result[match[1]] = match[2]
    return result


def capture_sdk(args):
    source = Path(args.source).resolve()
    build = Path(args.build).resolve()
    cache_file = build / 'CMakeCache.txt'
    cache = cache_entries(cache_file)
    require(Path(cache['CMAKE_HOME_DIRECTORY']).resolve() == source,
            'SDK build cache does not select the supplied source checkout')
    git = args.git
    repo = repository(args.repository)
    actual_repo = run([git, '-C', source, 'remote', 'get-url', 'origin'])
    require(re.sub(r'\.git$', '', actual_repo) == re.sub(r'\.git$', '', repo),
            'SDK public source repository does not match actual selected checkout')
    identity = git_identity(git, source, include_tree=True)
    identity['repository'] = repo
    compiler_files = list(build.glob('CMakeFiles/*/CMakeCCompiler.cmake'))
    require(len(compiler_files) == 1, 'SDK selected compiler metadata is missing or ambiguous')
    metadata = compiler_files[0].read_text(encoding='utf-8')
    compiler = executable_identity(cache['CMAKE_C_COMPILER'], 'SDK C compiler')
    for key, cmake_key in (('id', 'CMAKE_C_COMPILER_ID'), ('version', 'CMAKE_C_COMPILER_VERSION')):
        match = re.search(r'set\(' + cmake_key + r' "([^"\n]+)"\)', metadata)
        require(match, 'SDK actual compiler version/ID metadata is missing')
        compiler[key] = text(match[1], 'SDK compiler ' + key)
    cmake = cache['CMAKE_COMMAND']
    version_output = run([cmake, '--version']).splitlines()[0]
    require(version_output.startswith('cmake version '), 'SDK CMake executable identity is unreadable')
    recipe = Path(args.recipe)
    build_identity = {'cache_sha256': digest(cache_file), 'compiler': compiler,
                      'cmake_sha256': digest(cmake),
                      'cmake_version': text(version_output[len('cmake version '):], 'SDK CMake version'),
                      'build_type': text(cache['CMAKE_BUILD_TYPE'], 'SDK build type'),
                      'recipe_sha256': digest(recipe)}
    project = text(cache['CMAKE_PROJECT_NAME'], 'SDK project name')
    version = text(cache['CMAKE_PROJECT_VERSION'], 'SDK project version')
    libraries = {}
    built_hashes = set()
    for path in args.built_library:
        artifact = Path(path).resolve(strict=True)
        require(build in artifact.parents, 'SDK artifact must be in the selected build tree')
        built_hashes.add(digest(artifact))
    install_manifest = build / 'install_manifest.txt'
    installed_paths = {Path(path).resolve() for path in
                       install_manifest.read_text(encoding='utf-8').splitlines() if path}
    for path in args.library:
        library = Path(path).resolve(strict=True)
        require(library.is_file(), 'Actual installed SDK library is missing')
        require(library in installed_paths and digest(library) in built_hashes,
                'Installed SDK DSO does not match actual build/install artifact bytes')
        libraries[digest(library)] = {'kind': 'captured-public-source-sdk',
                                      'name': project, 'version': version,
                                      'source': identity, 'build': build_identity}
    write_sdk_receipt(args, libraries)


def write_sdk_receipt(args, libraries):
    libraries = {sha(key): checked_sdk_identity(value, key) for key, value in libraries.items()}
    for path in args.merge:
        for key, identity in sdk_receipt(path).items():
            require(key not in libraries or libraries[key] == identity,
                    'Conflicting SDK receipts for identical DSO bytes')
            libraries[key] = identity
    require(0 < len(libraries) <= 256, 'SDK capture requires 1 to 256 selected libraries')
    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps({'schema_version': SDK_SCHEMA, 'record_kind': SDK_RECORD_KIND,
                                 'generator_sha256': digest(Path(__file__)), 'libraries': libraries},
                                indent=2, sort_keys=True) + '\n', encoding='utf-8')


def deb_metadata(tool, archive):
    values = {key: run([tool, '--field', archive, key])
              for key in ('Package', 'Version', 'Source', 'Architecture')}
    package = text(values['Package'], 'Debian Package')
    version = text(values['Version'], 'Debian Version')
    architecture = text(values['Architecture'], 'Debian Architecture')
    require(re.fullmatch(r'[a-z0-9][a-z0-9+.-]*', package) and
            re.fullmatch(r'[a-z0-9][a-z0-9-]*', architecture),
            'Invalid Debian package or architecture')
    source_package, source_version = package, version
    if values['Source']:
        match = re.fullmatch(r'([a-z0-9][a-z0-9+.-]*)(?: \(([^()\s]+)\))?', values['Source'])
        require(match, 'Invalid Debian Source field')
        source_package = match[1]
        source_version = text(match[2] or version, 'Debian source version')
    return {'binary_package': package, 'binary_version': version,
            'source_package': source_package, 'source_version': source_version,
            'architecture': architecture, 'source_field_present': bool(values['Source']),
            'source_verification': DEB_SOURCE_SCOPE, 'archive_authentication': DEB_AUTH_SCOPE}


class BoundedDebStream:
    """Limit decompressed tar bytes, including padding and extended headers."""
    def __init__(self, stream):
        self.stream = stream
        self.remaining = 1024 ** 3

    def read(self, size):
        require(0 <= size <= self.remaining, 'Debian data archive exceeds bounded scope')
        data = self.stream.read(size)
        self.remaining -= len(data)
        return data


def deb_members(tool, archive, selected):
    matches = {}
    process = subprocess.Popen([str(tool), '--fsys-tarfile', str(archive)],
                               stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
                               env=dict(os.environ, LC_ALL='C'))
    reader = BoundedDebStream(process.stdout)
    try:
        with tarfile.open(fileobj=reader, mode='r|') as members:
            count = 0
            total = 0
            for member in members:
                count += 1
                total += member.size
                require(count <= 100000 and 0 <= member.size and total <= 1024 ** 3,
                        'Debian data archive exceeds bounded member scope')
                name = member.name[2:] if member.name.startswith('./') else member.name
                if name not in selected:
                    continue
                checked_member(member.name)
                require(name not in matches and member.isfile() and
                        0 < member.size <= 256 * 1024 ** 2,
                        'Selected Debian DSO member is ambiguous, nonregular or excessive')
                expected_sha256, expected_bytes = selected[name]
                require(member.size == expected_bytes, 'Selected SDK DSO differs from archive member size')
                h = hashlib.sha256()
                with members.extractfile(member) as stream:
                    for block in iter(lambda: stream.read(1024 * 1024), b''):
                        h.update(block)
                require(h.hexdigest() == expected_sha256,
                        'Selected SDK DSO differs from exact Debian archive member bytes')
                matches[name] = member.name
        # Consume any trailing tar padding through the same bounded reader.
        # Closing a pipe before dpkg-deb finishes would otherwise hide failures.
        while reader.read(10240):
            pass
        require(process.wait() == 0, 'Debian data archive metadata command failed')
    finally:
        process.stdout.close()
        if process.poll() is None:
            process.terminate()
            process.wait()
    return matches


def capture_deb_sdk(args):
    prefix = Path(args.prefix).resolve(strict=True)
    require(prefix.is_dir(), 'Selected extracted Debian SDK prefix is missing')
    require(0 < len(args.deb) <= 64 and 0 < len(args.library) <= 256,
            'Debian SDK capture requires bounded explicit archives and DSO paths')
    selected = {}
    for value in args.library:
        name = relative(value)
        library = (prefix / name).resolve(strict=True)
        require(prefix in library.parents and library.is_file(),
                'Selected SDK DSO escapes explicit prefix or is missing')
        # A selected SONAME symlink resolves to its real regular archive member.
        name = relative(library.relative_to(prefix).as_posix())
        require(0 < library.stat().st_size <= 256 * 1024 ** 2,
                'Selected SDK DSO size exceeds bounded scope')
        with library.open('rb') as stream:
            require(stream.read(4) == b'\x7fELF', 'Selected SDK DSO is not an ELF input')
        selected[name] = (digest(library), library.stat().st_size)
    libraries = {}
    matched = set()
    archives = set()
    for value in args.deb:
        archive = Path(value).resolve(strict=True)
        size = archive.stat().st_size
        require(archive.is_file() and 0 < size <= 256 * 1024 ** 2,
                'Selected Debian archive is missing or exceeds bounded scope')
        archive_sha256 = digest(archive)
        require(archive_sha256 not in archives, 'Repeated Debian SDK archive')
        archives.add(archive_sha256)
        metadata = deb_metadata(args.dpkg_deb, archive)
        matches = deb_members(args.dpkg_deb, archive, selected)
        require(matches, 'Supplied Debian archive contains no selected SDK DSO member')
        for name, member_path in matches.items():
            require(name not in matched, 'Selected SDK DSO occurs in multiple supplied archives')
            matched.add(name)
            dso_sha256, dso_bytes = selected[name]
            require(dso_sha256 not in libraries, 'Ambiguous equal-byte selected Debian DSO identities')
            libraries[dso_sha256] = dict(metadata, kind='debian-binary-package',
                                        archive={'sha256': archive_sha256, 'bytes': size},
                                        member_path=member_path, dso_sha256=dso_sha256,
                                        dso_bytes=dso_bytes)
        require(digest(archive) == archive_sha256 and archive.stat().st_size == size,
                'Debian archive changed during capture')
    require(matched == set(selected), 'Selected SDK DSO has no exact retained Debian archive member')
    for name, (expected_sha256, _) in selected.items():
        require(digest(prefix / name) == expected_sha256, 'Selected SDK DSO changed during capture')
    write_sdk_receipt(args, libraries)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--inputs')
    parser.add_argument('--stage')
    parser.add_argument('--linux-libraries', default='[]')
    subparsers = parser.add_subparsers(dest='command')
    sdk = subparsers.add_parser('capture-sdk', help='Capture actual public-source SDK/installed DSO identities')
    for name in ('source', 'build', 'repository', 'recipe', 'output'):
        sdk.add_argument('--' + name, required=True)
    sdk.add_argument('--library', action='append', required=True)
    sdk.add_argument('--built-library', action='append', required=True)
    sdk.add_argument('--git', default='git')
    sdk.add_argument('--merge', action='append', default=[],
                     help='Explicit schema-2 receipt to combine with this capture; repeatable')
    deb = subparsers.add_parser('capture-deb-sdk',
                               help='Bind selected extracted SDK DSOs to retained exact Debian members')
    for name in ('prefix', 'output'):
        deb.add_argument('--' + name, required=True)
    deb.add_argument('--deb', action='append', required=True,
                     help='Exact retained .deb; every supplied archive must own a selected DSO')
    deb.add_argument('--library', action='append', required=True,
                     help='Selected original DSO path relative to --prefix; repeatable')
    deb.add_argument('--dpkg-deb', default='dpkg-deb')
    deb.add_argument('--merge', action='append', default=[],
                     help='Explicit schema-2 receipt to combine with this capture; repeatable')
    args = parser.parse_args()
    try:
        if args.command == 'capture-sdk':
            capture_sdk(args)
        elif args.command == 'capture-deb-sdk':
            capture_deb_sdk(args)
        else:
            require(args.inputs and args.stage, 'Install-time inputs and staging root are required')
            generate(args.inputs, args.stage, args.linux_libraries)
    except (CompositionError, OSError, ValueError, KeyError, TypeError, tarfile.TarError) as error:
        # Errors from foreign metadata/filesystem operations may contain paths.
        message = str(error) if isinstance(error, CompositionError) else type(error).__name__
        parser.exit(1, 'Runtime composition failed closed: ' + message + '\n')


if __name__ == '__main__':
    main()
