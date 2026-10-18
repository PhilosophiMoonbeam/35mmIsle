#!/usr/bin/env python3
"""Privately materialize the complete pinned Beach movement-music C callback."""
import argparse
import hashlib
import json
from pathlib import Path
from prepare_engine import git, publish

REVISION = 'df8815e75f3d79618466e01e0827c6165b93d11c'
VERSION = 'beach-music-complete-v1'
SOURCE = 'src/beach/55C870.c'
IDENTITIES = {
    SOURCE: ('2dd46c1a0bc0a360adc91e578b654e818483e794', '688c1b4bc4a71d0fdf8648427f3d44814ae1ac7edad1cc1d230767eef61da266'),
    'src/beach/55C110.c': ('213859dc00eab1f498c4f76d15fa3aba7bda6656', 'e6e79a9cae04506d0f35d9ddbafa14593e565c50463174c6ebaa2cab915d7d77'),
    'src/beach/beach.h': ('0aad5edbe49cd130449874b9b3b55941d3b73042', 'a6ac77fd76c0368f28990771f0b8050f7a4c34c113b8d27497d699eda8601edc'),
    'splat.yaml': ('d2690bd3d2cb0aeac5c648db20666c5a42e5ba45', '24a476315ea9e2de573c11b5bc4662480d23109180a4ed29c01141c882ceadcd'),
    'src/sys/audio.c': ('a617f2ee04ad0851e0db46d31a6cf034e1eb0355', '99e092ee4faf2707b312740d235257fad16616d062b350cacdc9924348777dd8'),
}
GLOBAL_COUNTS = {
    'D_beach_802CC0D0': 6, 'D_beach_802CC0D4': 4, 'D_beach_802CC0D8': 4,
    'D_beach_80347578': 7, 'D_beach_80347579': 7, 'D_beach_8034757A': 3,
    'D_beach_8034757B': 5, 'D_beach_8034757C': 9,
}
SUPPORT = (
    'src/beach_music.hpp', 'src/beach_music.cpp',
    'src/beach_music_source.h', 'src/beach_music_source.c',
    'src/rom_snapshot.hpp', 'src/rom_view.hpp', 'src/scene_services.hpp',
    'src/game_audio.hpp', 'src/player_source.h', 'src/player_source_bridge.c',
    'src/player_root_source.h', 'cmake/player-root-inputs.json',
)
ORIGINS = (
    (SOURCE, 'course_sound_handle_and_volume_BSS', 4, 10),
    ('src/beach/beach.h', 'initialized_signed_music_stage_words', 25, 27),
    ('splat.yaml', 'actual_Beach_overlay_mapping_and_symbol_name_format', 5944, 5957),
    ('src/beach/55C110.c', 'original_movement_callback_and_late_detector_stage', 158, 165),
    ('src/sys/audio.c', 'actual_parameterized_sound_request', 1318, 1335),
    ('src/sys/audio.c', 'actual_sound_volume_request', 1359, 1374),
    ('src/sys/audio.c', 'actual_sound_stop_request', 1418, 1427),
)
PREAMBLE = '''/* Complete pinned own-GAME Beach movement music; no SDK implementation. */
#include "engine_internal.h"
#include "beach_music_source.h"
#include "player_root_source.h"

'''
ORIGINAL_SHA256 = 'a28a4324eebccdb9d2afed58c341c3990df1ef6666d245c33a98b368387ac080'
NATIVE_SHA256 = 'a4840bfa593ee0f4c20b20c10782157fae7fadc786792c04b7c2b37fef99509e'
OUTPUT_SHA256 = 'c9cfbf003f5d36c10edc7d4398092eb5e1f400f0c1900b2788644c74f26b57b9'


def digest(data):
    return hashlib.sha256(data).hexdigest()


def transform(original):
    body = original
    changes = []

    def replace(old, new, count):
        nonlocal body
        if body.count(old) != count:
            raise ValueError('Beach music counted rewrite mismatch: ' + old)
        body = body.replace(old, new)
        changes.append(dict(find=old, replace=new, occurrences=count))

    replace('void func_beach_802C4800(s32 arg0, f32 arg1) {',
            'void isle_beach_music_func_802C4800(IsleBeachMusicSource* music, s32 arg0, f32 arg1) {', 1)
    for name, count in GLOBAL_COUNTS.items():
        replace(name, 'music->' + name, count)
    for name, native, count in (
            ('auPlaySoundWithParams', 'isle_beach_music_play_sound', 4),
            ('auSetSoundVolume', 'isle_beach_music_volume', 17),
            ('auStopSound', 'isle_beach_music_stop', 1)):
        replace(name + '(', native + '(music, ', count)
    replace('setBackgroundMusic(0);', 'isle_player_setBackgroundMusic(music->player, 0);', 1)
    if digest(body.encode()) != NATIVE_SHA256:
        raise ValueError('Beach music complete native body mismatch')
    return body, changes


def prepare(repository, manifest_path, output, evidence):
    raw = manifest_path.read_bytes()
    manifest = json.loads(raw)
    root = Path(__file__).resolve().parent.parent
    if (manifest['schema_version'] != 1 or manifest['revision'] != REVISION or
            manifest['transformation_version'] != VERSION):
        raise ValueError('Beach music authority/version mismatch')
    output = output.resolve()
    evidence = evidence.resolve()
    protected = (repository.resolve(), *(root / name for name in
                 ('src', 'include', 'tests', 'tools', 'cmake', '.git', 'graft')))
    if (output.name != 'game.c' or output.parent.name != 'isle-beach-music-private' or
            output.parent.parent.name != '_deps' or output == evidence or
            any(base == parent or base in parent.parents for base in protected
                for parent in (output.parent, evidence.parent))):
        raise ValueError('Beach music requires private build outputs outside source trees')
    if manifest['generator_sha256'] != digest(Path(__file__).read_bytes()):
        raise ValueError('Beach music generator identity mismatch')
    if set(manifest['source_files']) != set(IDENTITIES) or set(manifest['native_support']) != set(SUPPORT):
        raise ValueError('Beach music source/support allowlist mismatch')
    if git(repository, 'cat-file', '-t', REVISION).strip() != b'commit':
        raise ValueError('Beach music authority is not the pinned commit')
    sources = {}
    for path, (blob, sha) in IDENTITIES.items():
        data = git(repository, 'show', REVISION + ':' + path)
        identity = manifest['source_files'][path]
        actual_blob = hashlib.sha1(b'blob ' + str(len(data)).encode() + b'\0' + data).hexdigest()
        if (actual_blob != blob or digest(data) != sha or identity['git_blob'] != blob or
                identity['sha256'] != sha or identity['line_count'] != len(data.splitlines())):
            raise ValueError('Beach music pinned source identity mismatch: ' + path)
        sources[path] = data.decode().splitlines(keepends=True)
    for path, identity in manifest['native_support'].items():
        if identity['sha256'] != digest((root / path).read_bytes()):
            raise ValueError('Beach music native support identity mismatch: ' + path)
    if len(manifest['semantic_origins']) != len(ORIGINS):
        raise ValueError('Beach music semantic origin inventory mismatch')
    for origin, (path, label, first, last) in zip(manifest['semantic_origins'], ORIGINS):
        if ((origin['path'], origin['label'], origin['first'], origin['last']) != (path, label, first, last) or
                origin['source_sha256'] != digest(''.join(sources[path][first-1:last]).encode())):
            raise ValueError('Beach music semantic origin mismatch: ' + label)
    original = ''.join(sources[SOURCE][11:113])
    if (manifest['definition'] != dict(path=SOURCE, symbol='func_beach_802C4800', first=12, last=113,
            source_sha256=ORIGINAL_SHA256, native_sha256=NATIVE_SHA256) or digest(original.encode()) != ORIGINAL_SHA256):
        raise ValueError('Beach music complete definition inventory mismatch')
    body, changes = transform(original)
    result = PREAMBLE + '#line 12 "snap@' + REVISION + '/' + SOURCE + '"\n' + body
    if digest(result.encode()) != OUTPUT_SHA256 or manifest['output_sha256'] != OUTPUT_SHA256:
        raise ValueError('Beach music complete output identity mismatch')
    publish(output, result)
    publish(evidence, json.dumps(dict(manifest, manifest_sha256=digest(raw),
        changes=changes, generated_source_sha256=digest(result.encode())), indent=2) + '\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('repository', 'manifest', 'output', 'evidence'):
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    prepare(args.repository, args.manifest, args.output, args.evidence)


if __name__ == '__main__':
    main()
