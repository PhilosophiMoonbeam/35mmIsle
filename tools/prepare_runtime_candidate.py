#!/usr/bin/env python3
"""Seal explicit, ROM-free preview8 Runtime candidates; never publish a release.

Inputs are one checked Runtime ZIP/TGZ, its independently extracted root, and a
clean public checkout at a full immutable commit. No build tree is copied.
"""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import shutil
import stat
import subprocess
import tarfile
import zipfile

from runtime_composition import CompositionError, commit, digest, load, payload_inventory, relative, require, selected_sdk_notices

MANIFEST = 'share/35mmisle/runtime-composition.json'
PRIVATE_SUFFIXES = {'.z64', '.v64', '.n64', '.rom', '.otr', '.o2r', '.sav', '.sra',
                    '.eep', '.fla', '.png', '.jpg', '.jpeg', '.wav', '.pcm', '.mp4',
                    '.pdb', '.obj', '.o', '.lib', '.a', '.log', '.suo', '.dmp'}
SDK_NOTICE_PREFIX = 'share/licenses/35mmisle/dependencies/sdk/'
SDK_NONNOTICE_SUFFIXES = {'.c', '.cc', '.cpp', '.cxx', '.h', '.hh', '.hpp', '.hxx',
                          '.s', '.sh', '.bash', '.py', '.pl', '.rb', '.ps1', '.bat',
                          '.cmd', '.js', '.ts', '.cmake', '.cfg', '.conf', '.ini',
                          '.json', '.yaml', '.yml', '.toml', '.dll', '.exe', '.so',
                          '.dylib', '.bin', '.zip', '.gz', '.xz', '.tar', '.7z'}
SECRET = re.compile(rb'-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----|'
                    rb'gh[pousr]_[A-Za-z0-9]{36,}|github_pat_[A-Za-z0-9_]{40,}')


def public_name(name, runtime=True, sdk_notices=None):
    relative(name)
    path = PurePosixPath(name)
    require(path.suffix.lower() not in PRIVATE_SUFFIXES,
            'Private payload/file type is not a candidate input')
    selected_notice = (runtime and sdk_notices is not None and
                       name.startswith(SDK_NOTICE_PREFIX) and name in sdk_notices)
    if selected_notice:
        require(path.suffix.lower() not in SDK_NONNOTICE_SUFFIXES and
                not re.search(r'\.so(?:\.[0-9]+)*$', path.name, re.IGNORECASE),
                'Producer SDK notice selection contains code or binary material')
    require('sdk' not in {part.lower() for part in path.parts} or selected_notice,
            'Unselected SDK material is not a candidate input')
    require(not any(part.lower() in {'.git', 'profiles', 'captures', 'private-kits',
                                    'private-runtime-relink-kit', 'roms'}
                    for part in path.parts), 'Private path is not a candidate input')
    if runtime:
        require(name.startswith(('bin/', 'lib/35mmisle/', 'share/35mmisle/',
                                 'share/licenses/35mmisle/',
                                 'share/licenses/35mmisle-rom-tool/')),
                'Runtime path is outside the bounded install composition')
        require(not re.search(r'(^|/)(lightning|mman)(/|$)', name, re.IGNORECASE),
                'Obsolete backend has entered the Runtime candidate')
        if name.startswith('bin/'):
            require(path.name in ('35mmisle', '35mmisle-rom', '35mmisle.exe',
                                  '35mmisle-rom.exe') or path.suffix.lower() == '.dll',
                    'Unexpected executable/tool in Runtime candidate')


def hash_stream(stream):
    value = hashlib.sha256()
    size = 0
    overlap = b''
    for block in iter(lambda: stream.read(1024 * 1024), b''):
        size += len(block)
        require(size <= 1024 ** 3, 'Candidate member exceeds bounded size')
        require(not SECRET.search(overlap + block), 'Secret material in candidate input')
        overlap = block[-256:]
        value.update(block)
    return value.hexdigest(), size


def inspect_archive(archive, root_name, expected, sdk_notice_records=()):
    """Compare archive contents with the extracted exact inventory, without extraction."""
    seen = set()
    selected = selected_sdk_notices(list(sdk_notice_records), expected)
    sdk_notices = {item['path']: item['sha256'] for item in selected}
    # Directory entries are structural only: validate their actual leaf payloads first.
    for name in expected:
        public_name(name, sdk_notices=sdk_notices)

    def check(name, stream=None, target=None, directory=False):
        normalized = name.rstrip('/')
        relative(normalized)
        require(normalized == root_name or normalized.startswith(root_name + '/'),
                'Archive member escapes its single product directory')
        if normalized == root_name:
            require(directory, 'Archive root is not a directory')
            return
        name = normalized[len(root_name) + 1:]
        if directory:
            require(any(key.startswith(name + '/') for key in expected),
                    'Unexpected empty archive directory')
            return
        public_name(name, sdk_notices=sdk_notices)
        require(name in expected and name not in seen, 'Unexpected or repeated archive payload')
        seen.add(name)
        item = expected[name]
        if target is not None:
            require(item['kind'] == 'symlink' and target == item['target'],
                    'Archive symlink differs from extracted inventory')
        else:
            require(item['kind'] == 'file' and stream is not None,
                    'Archive member type differs from inventory')
            actual_hash, actual_size = hash_stream(stream)
            require(actual_hash == item['sha256'] and actual_size == item['bytes'],
                    'Archive bytes differ from extracted exact inventory')

    if archive.name.endswith('.zip'):
        with zipfile.ZipFile(archive) as source:
            for member in source.infolist():
                mode = member.external_attr >> 16
                require(not stat.S_ISLNK(mode), 'Runtime ZIP must not encode symlinks')
                require(not member.is_dir() or member.file_size == 0,
                        'Archive directory contains unexpected payload bytes')
                with source.open(member) as stream:
                    check(member.filename, stream, directory=member.is_dir())
    else:
        require(archive.name.endswith('.tar.gz'), 'Only Runtime ZIP/TGZ candidates are supported')
        with tarfile.open(archive, 'r:gz') as source:
            for member in source:
                require(member.isfile() or member.isdir() or member.issym(),
                        'Special/hardlinked archive member is excluded')
                require(member.isfile() or member.size == 0,
                        'Archive directory/link contains unexpected payload bytes')
                if member.isfile():
                    with source.extractfile(member) as stream:
                        check(member.name, stream)
                else:
                    check(member.name, target=member.linkname if member.issym() else None,
                          directory=member.isdir())
    require(seen == set(expected), 'Archive is missing exact Runtime payload members')


def seal(args):
    source = Path(args.source).resolve(strict=True)
    archive = Path(args.archive).resolve(strict=True)
    extracted = Path(args.extracted).resolve(strict=True)
    output = Path(args.output).absolute()
    revision = commit(args.revision)
    require(archive.is_file() and extracted.is_dir(), 'Explicit archive/extracted root is missing')
    require(not output.exists(), 'Candidate destination must be new and empty')
    require(subprocess.check_output(['git', '-C', str(source), 'rev-parse', 'HEAD'],
                                    text=True).strip() == revision,
            'Candidate source does not match the requested full revision')
    require(not subprocess.check_output(['git', '-C', str(source), 'diff', '--no-ext-diff',
                                          '--no-textconv', '--binary', 'HEAD', '--']),
            'Candidate checkout has tracked changes')
    manifest = load(extracted / MANIFEST)
    project = manifest['selected_sources']['project']
    require(project['commit'] == revision and not project['tracked_changes'],
            'Installed candidate was not composed from the clean immutable revision')
    llvm = manifest['selected_sources']['llvm']
    require(llvm['version'] == '21.1.8' and
            llvm['commit'] == '2078da43e25a4623cab2d0d60decddf709aaea28',
            'Candidate is not the selected production LLVM composition')
    actual = payload_inventory(extracted, MANIFEST)
    require(actual == manifest['payload'], 'Extracted Runtime differs from its exact composition')
    expected = {item['path']: item for item in actual}
    require(len(expected) == len(actual), 'Repeated Runtime inventory path')
    expected[MANIFEST] = {'kind': 'file', 'bytes': (extracted / MANIFEST).stat().st_size,
                          'sha256': digest(extracted / MANIFEST)}
    sdk_notice_records = selected_sdk_notices(
        manifest.get('producer_selected_sdk_notices', []), expected)
    sdk_notices = {item['path']: item['sha256'] for item in sdk_notice_records}
    for name in expected:
        public_name(name, sdk_notices=sdk_notices)
    require(any(name.startswith('share/licenses/35mmisle/dependencies/llvm/') for name in expected),
            'LLVM notice payload is missing')
    archive_sha256 = digest(archive)
    inspect_archive(archive, extracted.name, expected, sdk_notice_records)
    # Git archive exports only committed public source, never untracked local inputs.
    names = subprocess.check_output(['git', '-C', str(source), 'ls-tree', '-r',
                                     '--name-only', '-z', revision]).split(b'\0')
    for raw in names:
        if raw:
            public_name(raw.decode('utf-8'), runtime=False)
    output.mkdir(parents=True)
    try:
        source_archive = output / ('35mmisle-source-' + revision + '.tar.gz')
        subprocess.run(['git', '-C', str(source), 'archive', '--format=tar.gz',
                        '--prefix=35mmisle-source-' + revision + '/',
                        '--output=' + str(source_archive), revision], check=True)
        with tarfile.open(source_archive, 'r:gz') as files:
            for member in files:
                require(member.isfile() or member.isdir(), 'Nonregular public source archive member')
                if member.isfile():
                    with files.extractfile(member) as stream:
                        hash_stream(stream)
        shutil.copyfile(archive, output / archive.name)
        shutil.copyfile(extracted / MANIFEST, output / 'runtime-composition.json')
        require(digest(output / archive.name) == archive_sha256 and
                digest(output / 'runtime-composition.json') == expected[MANIFEST]['sha256'],
                'Candidate archive/composition changed during sealing')
        record = {'schema_version': 1, 'purpose': 'preview8-maintainer-download-candidate',
                  'source_revision': revision, 'system': manifest['build']['system'],
                  'source_scope': 'Complete committed project tree; pinned external build inputs identified in runtime-composition.json and fetched by retained recipes',
                  'payload_verification': 'Every archive member matches independently extracted install-time composition; no unlisted files',
                  'publication': 'No release created; maintainer must download, inspect and smoke the exact hashes before final publication',
                  'qualification': 'Not Windows 11, full campaign, device or fidelity qualification',
                  'files': {path.name: {'sha256': digest(path), 'bytes': path.stat().st_size}
                            for path in sorted(output.iterdir())}}
        (output / 'candidate.json').write_text(json.dumps(record, indent=2, sort_keys=True) + '\n', encoding='utf-8')
        (output / 'SHA256SUMS').write_text(''.join(digest(path) + '  ' + path.name + '\n'
                                                 for path in sorted(output.iterdir())), encoding='utf-8')
    except BaseException:
        shutil.rmtree(output)
        raise


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('archive', 'extracted', 'source', 'revision', 'output'):
        parser.add_argument('--' + name, required=True)
    try:
        seal(parser.parse_args())
    except (CompositionError, OSError, ValueError, KeyError, subprocess.SubprocessError,
            tarfile.TarError, zipfile.BadZipFile) as error:
        parser.exit(1, 'Candidate sealing failed closed: ' +
                    (str(error) if isinstance(error, CompositionError) else type(error).__name__) + '\n')


if __name__ == '__main__':
    main()
