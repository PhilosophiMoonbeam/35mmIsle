#!/usr/bin/env python3
"""Emit an exact hosted LLVM cache key; this does not admit or verify the cache."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess


def digest(path):
    value = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            value.update(block)
    return value.hexdigest()


def identity(root, configuration):
    source = Path(__file__).resolve().parents[1]
    system = platform.system()
    if system == 'Windows':
        vswhere = Path(os.environ['ProgramFiles(x86)']) / 'Microsoft Visual Studio/Installer/vswhere.exe'
        installation = subprocess.check_output([
            str(vswhere), '-latest', '-products', '*', '-requires',
            'Microsoft.VisualStudio.Component.VC.Tools.x86.x64',
            '-property', 'installationPath'], text=True).strip()
        if not installation or '\n' in installation:
            raise ValueError('Expected one selected native Visual Studio installation')
        installation = Path(installation)
        version = (installation / 'VC/Auxiliary/Build/Microsoft.VCToolsVersion.default.txt').read_text().strip()
        compiler = installation / 'VC/Tools/MSVC' / version / 'bin/Hostx64/x64/cl.exe'
    elif system == 'Linux':
        compiler = Path(shutil.which('g++') or '').resolve(strict=True)
    else:
        raise ValueError('Unsupported hosted cache platform')
    image = os.environ.get('ImageVersion', '')
    if not image:
        raise ValueError('Hosted image identity is required for reusable LLVM caches')
    values = {'system': system, 'processor': platform.machine().lower(),
              'configuration': configuration, 'image': image,
              'compiler_sha256': digest(compiler),
              'cmake': subprocess.check_output(['cmake', '--version'], text=True).splitlines()[0],
              'absolute_dependency_root': str(Path(root).resolve()),
              'recipes': {name: digest(source / name) for name in (
                  'cmake/llvm-dependencies.json', 'tools/prepare_llvm.py', 'tools/prepare_rdp.py')}}
    encoded = json.dumps(values, sort_keys=True, separators=(',', ':')).encode('utf-8')
    return 'llvm-native-v1-' + system + '-' + configuration + '-' + hashlib.sha256(encoded).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', required=True)
    parser.add_argument('--configuration', required=True, choices=('Debug', 'Release'))
    args = parser.parse_args()
    try:
        print('key=' + identity(args.root, args.configuration))
    except (OSError, ValueError, KeyError, subprocess.SubprocessError) as error:
        parser.exit(1, 'LLVM cache identity unavailable: ' + type(error).__name__ + '\n')


if __name__ == '__main__':
    main()
