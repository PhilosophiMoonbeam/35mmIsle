#!/usr/bin/env python3
"""Prepare the exact MIT-selected parallel-rsp helper closure for LLVM ORC."""
#
# Original tooling: MIT, copyright 2026 35mmIsle contributors.
# MIT-selected parallel-rsp excerpts: copyright (c) 2020 Hans-Kristian
# Arntzen; permission and warranty terms are retained in LICENSE.MIT.

import argparse
import hashlib
import io
import json
from pathlib import Path, PurePosixPath
import tarfile
import tempfile

from prepare_rdp import acquire, publish_tree, copy_file

PIN = ('parallel-rsp', 'libretro/parallel-rsp',
       '9ab776a2ce28899769c4f911403234cac62fc799',
       'e03ad7e3277a9f763aab0aff86da350cf93fecb66ef7f616298d84755fa6275d')
INPUT_HASHES = {
    'state.hpp': '40728a07b19bd0d6693acaa59e1616780906c0ef5199c69e19f830aee26bf0f8',
    'arch/simd/rsp/rsp_core.cpp': '43cacdf5c8170afa6d55563b1012508740061159fe348eaa2447173dfb98b32d',
}


def digest(data):
    return hashlib.sha256(data).hexdigest()


def replace_once(text, before, after):
    if text.count(before) != 1:
        raise ValueError('exact private patch anchor is missing or ambiguous: ' + before[:80])
    return text.replace(before, after, 1)


def adapt(relative, text):
    if relative == 'state.hpp':
        text = replace_once(text, '\tCP0 cp0;\n', '''\tCP0 cp0;
	// Private host-service and bounded-basic-block integration. Not emulated cycles.
	uint32_t block_budget = 0;
	uint32_t service_register = 0;
	uint32_t service_value = 0;
''')
        text = replace_once(text, '\tMODE_CHECK_FLAGS = 4\n', '''\tMODE_CHECK_FLAGS = 4,
	MODE_HOST_SERVICE = 5,
	MODE_BUDGET = 6,
	MODE_HOST_ERROR = 7
''')
    elif relative == 'arch/simd/rsp/rsp_core.cpp':
        text = replace_once(text, '#include <string.h>\n', '''#include <string.h>
#if defined(_MSC_VER)
#include <intrin.h>
#define __builtin_bswap64(value) _byteswap_uint64(value)
#endif
''')
        # An output-only GNU asm constraint supplied arbitrary initial bits.
        # These cases overwrite every subsequently observed lane; zero has the
        # same result and uses the native MSVC intrinsic instead of GNU asm.
        before = '\t\t__asm__("" : "=x"(v)); /* Do not remove. */'
        if text.count(before) != 2:
            raise ValueError('unexpected exact SIMD initializer anchors')
        text = text.replace(before, '''#if defined(_MSC_VER)
        v = _mm_setzero_si128();
#else
''' + before + '\n#endif')
        text = replace_once(text, '''#ifdef NDEBUG
	__builtin_unreachable();
#else
	__builtin_trap();
#endif''', '''#if defined(_MSC_VER)
    __debugbreak();
    __assume(0);
#elif defined(NDEBUG)
	__builtin_unreachable();
#else
	__builtin_trap();
#endif''')
    return text


def prepare(manifest_path, output):
    manifest_bytes = manifest_path.read_bytes()
    manifest = json.loads(manifest_bytes)
    records = manifest['dependencies']
    if manifest['schema_version'] != 2 or len(records) != 1:
        raise ValueError('unsupported RSP dependency manifest')
    record = records[0]
    if tuple(record[key] for key in ('name', 'repository', 'commit', 'archive_sha256')) != PIN:
        raise ValueError('RSP identity differs from the exact supported pin')
    output.mkdir(parents=True, exist_ok=True)
    cache = output / 'archives'
    cache.mkdir(exist_ok=True)
    adaptations = []
    with tempfile.TemporaryDirectory(prefix='staging-', dir=output) as temporary:
        staging = Path(temporary)
        source = staging / 'source'
        source.mkdir()
        # Same path/link policy as prepare_rdp.unpack, selecting the admitted
        # files before writing anything. Obsolete providers are never extracted.
        selected = {'state.hpp', 'rsp_op.hpp', 'cpu_state.hpp', 'LICENSE',
                    'LICENSE.MIT', 'CREDITS.txt', 'rsp/vfunctions.cpp',
                    'rsp/ls.cpp', 'rsp/reciprocal.cpp', 'rsp/reciprocal.h',
                    'rsp/cp2.cpp'}
        with tarfile.open(fileobj=io.BytesIO(acquire(record, cache)), mode='r:gz') as archive:
            roots = set()
            for member in archive:
                path = PurePosixPath(member.name)
                if path.is_absolute() or '..' in path.parts or not path.parts:
                    raise ValueError('unsafe RSP archive member: ' + member.name)
                roots.add(path.parts[0])
                relative = PurePosixPath(*path.parts[1:])
                if relative.as_posix() not in selected and not (
                        relative.parent.as_posix() == 'arch/simd/rsp' and
                        relative.name != 'SSE2NEON.h'):
                    continue
                if member.isdir():
                    continue
                if not member.isfile():
                    raise ValueError('unsupported RSP archive link/special member')
                target = source.joinpath(*relative.parts)
                target.parent.mkdir(parents=True, exist_ok=True)
                with archive.extractfile(member) as original, target.open('wb') as output_file:
                    output_file.write(original.read())
            if len(roots) != 1:
                raise ValueError('RSP archive must have exactly one source root')
        if any(not (source / relative).is_file() for relative in selected):
            raise ValueError('RSP selected source closure is incomplete')
        notices = staging / 'notices'
        for relative in ('LICENSE', 'LICENSE.MIT', 'CREDITS.txt'):
            copy_file(source / relative, notices / relative)
        for relative, expected in INPUT_HASHES.items():
            path = source / relative
            original = path.read_bytes()
            if digest(original) != expected:
                raise ValueError('unexpected exact patch input: ' + relative)
            adapted = adapt(relative, original.decode('utf-8')).encode('utf-8')
            path.write_bytes(adapted)
            adaptations.append({'path': relative, 'original_sha256': expected,
                                'adapted_sha256': digest(adapted)})
        publish_tree(source, output / 'source')
        publish_tree(notices, output / 'notices')
    evidence = {'schema_version': 1, 'manifest_sha256': digest(manifest_bytes),
                'generator_sha256': digest(Path(__file__).read_bytes()),
                'helper_sha256': digest(Path(__file__).with_name('prepare_rdp.py').read_bytes()),
                'dependencies': records, 'adaptations': adaptations,
                'scope': manifest['scope'],
                'patch_identity': 'llvm-mit-helper-state-msvc-v3',
                'source_files': {path.relative_to(output / 'source').as_posix():
                                 digest(path.read_bytes())
                                 for path in sorted((output / 'source').rglob('*')) if path.is_file()},
                'composition': 'MIT-selected SIMD/vector/LS/CP2 helper subset and host CP0; LLVM ORC backend supplied by project. No upstream RSP JIT, LLVM debug CPU, allocator, Lightning or Windows mman source is materialized.',
                'license_notice': 'Upstream dual-license selection is MIT for this helper closure; LICENSE.MIT and embedded CEN64 notices retained. Unmodified complete upstream archive is retained separately as source provenance, not linked production composition.'}
    (output / 'provenance.json').write_text(json.dumps(evidence, indent=2) + '\n', encoding='utf-8')
    print('Prepared exact MIT-selected RSP helpers and notices at ' + str(output))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--manifest', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    try:
        prepare(args.manifest, args.output)
    except (OSError, ValueError, KeyError, TypeError, tarfile.TarError) as error:
        parser.exit(1, 'Pinned RSP preparation failed: %s\n' % error)


if __name__ == '__main__':
    main()
