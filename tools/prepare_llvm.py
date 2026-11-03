#!/usr/bin/env python3
"""Prepare verified LLVM 21.1.8 source and a native static ORC build (never compile)."""

import argparse
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import platform
import shutil
import subprocess
import tarfile
import tempfile
import urllib.request

from prepare_rdp import copy_file, publish_tree

VERSION = '21.1.8'
COMMIT = '2078da43e25a4623cab2d0d60decddf709aaea28'
ARCHIVE = 'llvm-project-21.1.8.src.tar.xz'
ARCHIVE_SHA256 = '4633a23617fa31a3ea51242586ea7fb1da7140e426bd62fc164261fe036aa142'
URL = 'https://github.com/llvm/llvm-project/releases/download/llvmorg-21.1.8/' + ARCHIVE
# Inventory covers every regular file in the selected unmodified release closure.
SOURCE_INVENTORY_SHA256 = '973581c4b66fc85e4c0db3e3172f06d73c043e063f06c6945d8b34d6b0de0809'
MAX_BYTES = 20 * 1024 ** 3
OPTIONS = {
    'CMAKE_BUILD_TYPE': 'Release', 'BUILD_SHARED_LIBS': 'OFF',
    'LLVM_TARGETS_TO_BUILD': 'X86', 'LLVM_ENABLE_PROJECTS': '',
    'LLVM_ENABLE_RUNTIMES': '', 'LLVM_BUILD_LLVM_DYLIB': 'OFF',
    'LLVM_LINK_LLVM_DYLIB': 'OFF', 'LLVM_INCLUDE_TESTS': 'OFF',
    'LLVM_INCLUDE_EXAMPLES': 'OFF', 'LLVM_INCLUDE_BENCHMARKS': 'OFF',
    'LLVM_BUILD_TOOLS': 'OFF', 'LLVM_INCLUDE_UTILS': 'OFF',
    'LLVM_ENABLE_BINDINGS': 'OFF', 'LLVM_ENABLE_ASSERTIONS': 'ON',
    'LLVM_ENABLE_EH': 'ON', 'LLVM_ENABLE_RTTI': 'ON',
    'LLVM_ENABLE_ZLIB': 'OFF', 'LLVM_ENABLE_ZSTD': 'OFF',
    'LLVM_ENABLE_LIBXML2': 'OFF', 'LLVM_ENABLE_LIBEDIT': 'OFF',
    'LLVM_ENABLE_CURL': 'OFF',
    'LLVM_PARALLEL_COMPILE_JOBS': '2', 'LLVM_PARALLEL_LINK_JOBS': '1',
    'LLVM_ENABLE_LTO': 'OFF',
}


def digest_file(path):
    value = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            value.update(block)
    return value.hexdigest()


def json_bytes(value):
    return (json.dumps(value, sort_keys=True, indent=2) + '\n').encode('utf-8')


def save(path, value):
    data = json_bytes(value)
    if not path.is_file() or path.read_bytes() != data:
        path.write_bytes(data)


def budget(root):
    size = sum(path.stat().st_size for path in root.rglob('*') if path.is_file())
    if size > MAX_BYTES:
        raise ValueError('LLVM dependency root exceeds the 20 GiB retention cap')


def register(root):
    root.mkdir(parents=True, exist_ok=True)
    if root.is_symlink():
        raise ValueError('LLVM dependency root must not be a symlink')
    manifest = root / 'dependency-retention.json'
    if not manifest.exists():
        save(manifest, {'schema_version': 1, 'owner_issue': '3pr.2',
             'purpose': 'Public verified LLVM native X86 ORC static dependency',
             'max_bytes': MAX_BYTES, 'max_compile_jobs': 2,
             'retained': ['archives', 'source', 'notices', 'source-inventory.json',
                          'provenance.json', 'build-artifacts.json', 'build', 'dependency-retention.json'],
             'excluded': ['ROM', 'SDK', 'private assets', 'profiles', 'captures'],
             'cleanup': 'Retain verified archive/source/notices and compatible build until parent acceptance; remove owned staging/partial files on failed acquisition only. Whole-root deletion requires parent authorization.'})
    budget(root)


def acquire(root):
    cache = root / 'archives'
    cache.mkdir(exist_ok=True)
    archive = cache / ARCHIVE
    if not archive.exists():
        partial = cache / (ARCHIVE + '.partial')
        try:
            with urllib.request.urlopen(URL, timeout=120) as source, partial.open('wb') as target:
                shutil.copyfileobj(source, target, 1024 * 1024)
            if digest_file(partial) != ARCHIVE_SHA256:
                raise ValueError('official LLVM source archive SHA-256 mismatch')
            partial.replace(archive)
        finally:
            partial.unlink(missing_ok=True)
    if digest_file(archive) != ARCHIVE_SHA256:
        raise ValueError('cached LLVM source archive SHA-256 mismatch')
    return archive


def collect_notices(source, notices, inventory):
    licenses = []
    for relative in inventory:
        path = source / relative
        leaf = path.name.lower()
        if leaf.startswith(('license', 'copying', 'copyright', 'notice')) or leaf.endswith('.license'):
            copy_file(path, notices / relative)
            licenses.append(relative)
        elif relative.startswith(('llvm/lib/', 'llvm/include/')):
            # LLVM's umbrella license explicitly requires retaining independent
            # per-file third-party terms. Preserve the complete leading comment
            # region, not implementation bodies, as installable plain text.
            text = path.read_bytes()[:65536].decode('utf-8', errors='replace')
            end = 0
            while end < len(text):
                if text[end].isspace():
                    end += 1
                elif text.startswith('//', end):
                    newline = text.find('\n', end)
                    end = len(text) if newline < 0 else newline + 1
                elif text.startswith('/*', end):
                    closer = text.find('*/', end + 2)
                    if closer < 0:
                        break
                    end = closer + 2
                else:
                    break
            header = text[:end].strip()
            if any(term in header.lower() for term in (
                    'redistribution and use', 'permission is hereby granted',
                    'copyright (c)', 'copyright ©', 'public domain',
                    'permission to use, copy')):
                target = notices / (relative + '.txt')
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_text('Embedded source notice: ' + relative + '\n'
                                  'Source SHA256: ' + inventory[relative] + '\n\n'
                                  + header + '\n', encoding='utf-8')
                licenses.append(relative)
    return licenses


def prepare(root):
    register(root)  # Retention and resource ownership precede acquisition.
    if (root / 'source-inventory.json').is_file():
        verify_source(root)
        refresh_notices(root)
        print('Reused verified LLVM source inventory SHA256: ' + SOURCE_INVENTORY_SHA256)
        return
    archive = acquire(root)
    with tempfile.TemporaryDirectory(prefix='staging-', dir=root) as temporary:
        staging = Path(temporary)
        source = staging / 'source'
        source.mkdir()
        # Reuse prepare_rdp's regular-file materialization policy. The official
        # release also has three MLGO script links, admitted as byte-for-byte
        # copies of their regular-file archive targets, never filesystem links.
        with tarfile.open(archive, 'r:xz') as package:
            for member in package:
                path = PurePosixPath(member.name)
                if path.is_absolute() or '..' in path.parts or not path.parts:
                    raise ValueError('unsafe LLVM archive member: ' + member.name)
                relative = PurePosixPath(*path.parts[1:])
                if not relative.parts or relative.parts[0] not in ('llvm', 'cmake', 'third-party'):
                    continue
                target = source.joinpath(*relative.parts)
                if member.isdir():
                    target.mkdir(parents=True, exist_ok=True)
                elif member.isfile():
                    target.parent.mkdir(parents=True, exist_ok=True)
                    with package.extractfile(member) as original, target.open('wb') as output:
                        shutil.copyfileobj(original, output)
                elif member.issym() and relative.parent.as_posix() == 'llvm/utils/mlgo-utils':
                    link = PurePosixPath(member.linkname)
                    if link.is_absolute() or '..' in link.parts:
                        raise ValueError('unsafe LLVM source link')
                    original_member = package.getmember(str(path.parent / link))
                    if not original_member.isfile():
                        raise ValueError('LLVM script link target must be a regular file')
                    target.parent.mkdir(parents=True, exist_ok=True)
                    with package.extractfile(original_member) as original, target.open('wb') as output:
                        shutil.copyfileobj(original, output)
                else:
                    raise ValueError('unsupported LLVM source link/special member: ' + member.name)
        inventory = {path.relative_to(source).as_posix(): digest_file(path)
                     for path in sorted(source.rglob('*')) if path.is_file()}
        inventory_hash = hashlib.sha256(json_bytes(inventory)).hexdigest()
        if SOURCE_INVENTORY_SHA256 and inventory_hash != SOURCE_INVENTORY_SHA256:
            raise ValueError('LLVM selected release inventory mismatch')
        notices = staging / 'notices'
        license_files = collect_notices(source, notices, inventory)
        publish_tree(source, root / 'source')
        publish_tree(notices, root / 'notices')
        save(root / 'source-inventory.json', inventory)
        save(root / 'provenance.json', {
            'schema_version': 1, 'name': 'llvm', 'version': VERSION,
            'repository': 'llvm/llvm-project', 'commit': COMMIT,
            'tag': 'llvmorg-' + VERSION, 'archive_sha256': ARCHIVE_SHA256,
            'archive_url': URL, 'source_inventory_sha256': inventory_hash,
            'license': 'Apache-2.0 WITH LLVM-exception; preserve selected third-party notices',
            'license_files': license_files,
            'composition': 'Unmodified llvm, cmake, third-party release sources; X86 native ORC static libraries; no Clang or legacy RSP debug CPU',
        })
    budget(root)
    print('Prepared LLVM source inventory SHA256: ' + inventory_hash)


def verify_source(root):
    inventory_path = root / 'source-inventory.json'
    if not SOURCE_INVENTORY_SHA256 or digest_file(inventory_path) != SOURCE_INVENTORY_SHA256:
        raise ValueError('LLVM source inventory is not the pinned release inventory; run --prepare')
    inventory = json.loads(inventory_path.read_text(encoding='utf-8'))
    source = root / 'source'
    actual = set()
    for path in source.rglob('*'):
        if path.is_symlink():
            raise ValueError('LLVM source may not contain symlinks')
        if path.is_file():
            relative = path.relative_to(source).as_posix()
            actual.add(relative)
            if relative not in inventory or digest_file(path) != inventory[relative]:
                raise ValueError('LLVM source differs from pinned inventory: ' + relative)
    if actual != set(inventory):
        raise ValueError('LLVM source closure is incomplete')
    budget(root)
    return inventory


def refresh_notices(root):
    inventory = json.loads((root / 'source-inventory.json').read_text(encoding='utf-8'))
    with tempfile.TemporaryDirectory(prefix='notices-', dir=root) as temporary:
        notices = Path(temporary)
        license_files = collect_notices(root / 'source', notices, inventory)
        publish_tree(notices, root / 'notices')
    provenance = json.loads((root / 'provenance.json').read_text(encoding='utf-8'))
    provenance['license_files'] = license_files
    save(root / 'provenance.json', provenance)


def cache_values(root):
    result = {}
    for line in (root / 'build/CMakeCache.txt').read_text(encoding='utf-8').splitlines():
        if line and not line.startswith(('#', '//')) and '=' in line and ':' in line.split('=', 1)[0]:
            key, value = line.split('=', 1)
            result[key.split(':', 1)[0]] = value
    return result


def configure(root, cc, cxx, generator, architecture, build_type):
    verify_source(root)
    if platform.machine().lower() not in ('x86_64', 'amd64') or platform.system() not in ('Linux', 'Windows'):
        raise ValueError('LLVM dependency requires a native Linux/Windows x64 host')
    command = ['cmake', '-S', str(root / 'source/llvm'), '-B', str(root / 'build'), '-G', generator]
    options = dict(OPTIONS)
    options['CMAKE_BUILD_TYPE'] = build_type
    if generator.startswith('Visual Studio'):
        if os.name != 'nt' or architecture != 'x64':
            raise ValueError('Visual Studio LLVM builds require native Windows x64')
        command += ['-A', architecture, '-T', 'host=x64']
        options['CMAKE_CONFIGURATION_TYPES'] = build_type
    else:
        options.update({'CMAKE_C_COMPILER': str(Path(shutil.which(cc) or cc).resolve()),
                        'CMAKE_CXX_COMPILER': str(Path(shutil.which(cxx) or cxx).resolve())})
    if os.name == 'nt':
        # Keep Debug CRT/iterator/assertion semantics, but do not duplicate
        # dependency PDBs inside the bounded reusable source/build root.
        options.update({'CMAKE_MSVC_RUNTIME_LIBRARY': 'MultiThreaded$<$<CONFIG:Debug>:Debug>',
                        'LLVM_USE_CRT_RELEASE': 'MT', 'LLVM_USE_CRT_DEBUG': 'MTd',
                        'CMAKE_MSVC_DEBUG_INFORMATION_FORMAT': ''})
    if (root / 'build/CMakeCache.txt').exists():
        previous = cache_values(root)
        for key, expected in options.items():
            if previous.get(key) != expected:
                raise ValueError('incompatible existing LLVM cache: ' + key + '; use another owned root')
        verify_build(root)
    command += ['-D' + key + '=' + value for key, value in options.items()]
    subprocess.run(command, check=True)
    verify_build(root, configured=True)


def verify_build(root, configured=False, require_seal=False):
    verify_source(root)
    values = cache_values(root)
    expected_options = dict(OPTIONS)
    build_type = values.get('CMAKE_BUILD_TYPE')
    if build_type not in ('Debug', 'Release'):
        raise ValueError('LLVM build type must be Debug or Release')
    expected_options['CMAKE_BUILD_TYPE'] = build_type
    for key, expected in expected_options.items():
        if values.get(key) != expected:
            raise ValueError('incompatible LLVM configuration: ' + key)
    for key, flags in values.items():
        if key != 'CMAKE_CXX_FLAGS' and not key.startswith('CMAKE_CXX_FLAGS_'):
            continue
        if any(flag in flags for flag in (
                '-flto', '-fabi-version', '_GLIBCXX_USE_CXX11_ABI',
                '_GLIBCXX_DEBUG', '_GLIBCXX_PROFILE', '-stdlib=', '/GL')):
            raise ValueError('unsupported LLVM ABI/LTO override in ' + key)
    if Path(values.get('CMAKE_HOME_DIRECTORY', '')).resolve() != root / 'source/llvm':
        raise ValueError('LLVM build is not from the verified source root')
    if values.get('CMAKE_CROSSCOMPILING', 'FALSE') not in ('FALSE', 'OFF', ''):
        raise ValueError('cross-compiled LLVM is not a native dependency')
    if os.name == 'nt' and (values.get('CMAKE_MSVC_RUNTIME_LIBRARY') != 'MultiThreaded$<$<CONFIG:Debug>:Debug>' or values.get('LLVM_USE_CRT_RELEASE') != 'MT' or values.get('LLVM_USE_CRT_DEBUG') != 'MTd'):
        raise ValueError('Windows LLVM must use native MSVC /MT')
    if os.name == 'nt' and values.get('CMAKE_MSVC_DEBUG_INFORMATION_FORMAT') != '':
        raise ValueError('LLVM dependency debug symbols must remain disabled within the 20 GiB resource cap')
    # Compiler facts are generated by CMake, never supplied as compiler-ID overrides.
    facts = sorted((root / 'build/CMakeFiles').glob('*/CMakeCXXCompiler.cmake'))
    if len(facts) != 1:
        raise ValueError('ambiguous or absent LLVM compiler facts')
    compiler_facts = facts[0].read_text(encoding='utf-8')
    def fact(name):
        import re
        match = re.search(r'set\(' + name + r' "?([^"\)\n]*)"?\)', compiler_facts)
        return match.group(1) if match else ''
    compiler = Path(fact('CMAKE_CXX_COMPILER') or values['CMAKE_CXX_COMPILER'])
    implicit_libraries = fact('CMAKE_CXX_IMPLICIT_LINK_LIBRARIES').split(';')
    standard_library = ('msvc' if os.name == 'nt' else
                        'libstdc++' if 'stdc++' in implicit_libraries else
                        'libc++' if 'c++' in implicit_libraries else '')
    if not standard_library:
        raise ValueError('LLVM standard library ABI could not be identified')
    build = {'cache_sha256': digest_file(root / 'build/CMakeCache.txt'),
             'compiler_facts_sha256': digest_file(facts[0]),
             'compiler_id': fact('CMAKE_CXX_COMPILER_ID'),
             'compiler_version': fact('CMAKE_CXX_COMPILER_VERSION'),
             'compiler_sha256': digest_file(compiler), 'system': platform.system(),
             'processor': platform.machine().lower(), 'pointer_size': fact('CMAKE_CXX_SIZEOF_DATA_PTR'),
             'build_type': build_type, 'targets': 'X86', 'shared': False,
             'msvc_runtime': ('MultiThreadedDebug' if build_type == 'Debug' else 'MultiThreaded') if os.name == 'nt' else '',
             'source_inventory_sha256': SOURCE_INVENTORY_SHA256}
    artifact_path = root / 'build-artifacts.json'
    if artifact_path.is_file():
        artifacts = json.loads(artifact_path.read_text(encoding='utf-8'))
        if (artifacts.get('schema_version') != 1 or
                artifacts.get('source_inventory_sha256') != SOURCE_INVENTORY_SHA256 or
                artifacts.get('cache_sha256') != build['cache_sha256']):
            raise ValueError('LLVM compiled artifact receipt does not match its verified source/configuration')
        actual = artifact_files(root)
        if artifacts.get('files') != actual:
            raise ValueError('LLVM static archive/generated-header/package content changed since sealing')
        build['artifact_inventory_sha256'] = digest_file(artifact_path)
    elif require_seal:
        raise ValueError('LLVM compiled dependency is unsealed; run --seal-build after successful target build')
    build['standard_library'] = standard_library
    if build['pointer_size'] != '8' or (os.name == 'nt' and build['compiler_id'] != 'MSVC'):
        raise ValueError('LLVM compiler is not the supported native x64 compiler')
    provenance = json.loads((root / 'provenance.json').read_text(encoding='utf-8'))
    if (provenance.get('archive_sha256') != ARCHIVE_SHA256 or
            provenance.get('commit') != COMMIT or
            provenance.get('version') != VERSION or
            provenance.get('source_inventory_sha256') != SOURCE_INVENTORY_SHA256):
        raise ValueError('LLVM provenance is not the supported release')
    previous = provenance.get('build')
    if require_seal and not (previous or {}).get('artifact_inventory_sha256'):
        raise ValueError('LLVM configure receipt does not contain the compiled artifact seal')
    if not configured and not previous:
        raise ValueError('LLVM build has no source-backed configure receipt; run --configure')
    if previous:
        for key, value in previous.items():
            if key == 'cache_sha256' and configured:
                continue
            if build.get(key) != value:
                raise ValueError('LLVM cache/compiler facts changed since configuration: ' + key)
    provenance['build'] = build
    save(root / 'provenance.json', provenance)
    refresh_notices(root)


def artifact_files(root):
    build = root / 'build'
    selected = set()
    archives = []
    values = cache_values(root)
    library_directories = [build / 'lib']
    if values.get('CMAKE_CONFIGURATION_TYPES'):
        configuration = values.get('CMAKE_BUILD_TYPE')
        if (configuration not in ('Debug', 'Release') or
                configuration not in values['CMAKE_CONFIGURATION_TYPES'].split(';')):
            raise ValueError('LLVM selected multi-config configuration is invalid')
        # Visual Studio emits component libraries here, not under build/lib.
        library_directories.append(build / configuration / 'lib')
    for directory in library_directories:
        if directory.is_symlink():
            raise ValueError('LLVM compiled artifact tree must not contain symlinks')
        for path in directory.rglob('*'):
            if path.is_symlink():
                raise ValueError('LLVM compiled artifact tree must not contain symlinks')
            if path.is_file() and path.suffix.lower() in ('.a', '.lib'):
                archives.append(path)
                selected.add(path)
    # These are the actual CMake component target names requested by the
    # provisioning contract, not a guessed transitive linker library list.
    names = {path.stem.removeprefix('lib') for path in archives}
    required = {'LLVMOrcJIT', 'LLVMExecutionEngine', 'LLVMRuntimeDyld',
                'LLVMObject', 'LLVMX86CodeGen', 'LLVMX86Desc', 'LLVMX86Info',
                'LLVMCore', 'LLVMSupport', 'LLVMPasses'}
    if not required.issubset(names):
        raise ValueError('LLVM ORC/native/pass component build is incomplete')
    for directory in (build / 'include', build / 'lib/cmake/llvm'):
        if not directory.is_dir():
            raise ValueError('LLVM generated headers/package are absent')
        for path in directory.rglob('*'):
            if path.is_symlink():
                raise ValueError('LLVM generated artifact may not be a symlink')
            if path.is_file():
                selected.add(path)
    return {path.relative_to(build).as_posix(): digest_file(path)
            for path in sorted(selected)}


def seal_build(root):
    # Seal only after the caller has successfully built the declared component
    # targets. This command never configures or invokes a compiler.
    verify_build(root)
    record = {'schema_version': 1,
              'source_inventory_sha256': SOURCE_INVENTORY_SHA256,
              'cache_sha256': digest_file(root / 'build/CMakeCache.txt'),
              'files': artifact_files(root)}
    save(root / 'build-artifacts.json', record)
    verify_build(root)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', required=True, type=Path)
    action = parser.add_mutually_exclusive_group(required=True)
    action.add_argument('--prepare', action='store_true')
    action.add_argument('--configure', action='store_true')
    action.add_argument('--verify', action='store_true')
    action.add_argument('--verify-sealed', action='store_true')
    action.add_argument('--seal-build', action='store_true')
    parser.add_argument('--cc', default='cl' if os.name == 'nt' else 'cc')
    parser.add_argument('--cxx', default='cl' if os.name == 'nt' else 'c++')
    parser.add_argument('--generator', default='Visual Studio 17 2022' if os.name == 'nt' else 'Ninja')
    parser.add_argument('--architecture', default='x64')
    parser.add_argument('--build-type', choices=('Debug', 'Release'), default='Release')
    args = parser.parse_args()
    root = args.root.resolve()
    try:
        if args.prepare:
            prepare(root)
        elif args.configure:
            configure(root, args.cc, args.cxx, args.generator, args.architecture, args.build_type)
        elif args.seal_build:
            seal_build(root)
        else:
            verify_build(root, require_seal=args.verify_sealed)
    except (OSError, ValueError, KeyError, tarfile.TarError, subprocess.CalledProcessError) as error:
        parser.exit(1, 'Pinned LLVM preparation failed: %s\n' % error)


if __name__ == '__main__':
    main()
