#!/usr/bin/env python3
"""Privately admit all five complete pinned own-GAME bump detector definitions."""
import argparse
import hashlib
import json
import re
from pathlib import Path
from prepare_engine import git, publish

REVISION = 'df8815e75f3d79618466e01e0827c6165b93d11c'
VERSION = 'player-bump-complete-v1'
SOURCE = 'src/app_level/bump_detect.c'
FILES = (SOURCE, 'include/enums.h', 'src/app_level/app_level.h')
DEFINITIONS = (
    ('BumpDetector_Update', 12, 45),
    ('BumpDetector_Init', 47, 52),
    ('BumpDetector_Disable', 54, 57),
    ('BumpDetector_Enable', 59, 62),
    ('BumpDetector_Delete', 64, 69),
)
ORIGINS = (
    (SOURCE, 'canonical_proximity_enabled_and_threshold_initializers', 4, 6),
    (SOURCE, 'canonical_detector_object', 10, 10),
    ('include/enums.h', 'actual_native_update_links', 248, 258),
    ('include/enums.h', 'actual_bump_object_identity', 283, 301),
    ('include/enums.h', 'actual_actor_tangible_flag', 378, 390),
    ('src/app_level/app_level.h', 'actual_player_bump_command', 104, 113),
)
CONSTANTS = (
    ('include/enums.h', 'LINK_0', 0),
    ('include/enums.h', 'LINK_POKEMON', 3),
    ('include/enums.h', 'OBJID_BUMP_DETECTOR', 8),
    ('include/enums.h', 'POKEMON_FLAG_20', 0x20),
    ('src/app_level/app_level.h', 'PLAYER_CMD_BUMP', 4),
)
GLOBAL_COUNTS = {
    'BumpDetector_Update': {'BumpDetector_ProximityLevel': 6, 'BumpDetector_Thresholds': 4},
    'BumpDetector_Init': {'BumpDetector_ProximityLevel': 1, 'BumpDetector_IsEnabled': 1, 'BumpDetector_Object': 2},
    'BumpDetector_Disable': {'BumpDetector_IsEnabled': 1, 'BumpDetector_Object': 1},
    'BumpDetector_Enable': {'BumpDetector_IsEnabled': 1, 'BumpDetector_Object': 1},
    'BumpDetector_Delete': {'BumpDetector_Object': 3},
}
SUPPORT = ('src/player_bump_source.h', 'src/player_bump_source.c',
           'src/player_bump.hpp', 'src/player_bump.cpp')
PREAMBLE = '''/* Complete pinned own-GAME C bodies; native semantic types only. */
#include "engine_internal.h"
#include "player_bump_source.h"
#include <float.h>
typedef float f32;
#define FLOAT_MAX FLT_MAX
'''


def digest(data):
    return hashlib.sha256(data).hexdigest()


def transform(symbol, original):
    text = original
    changes = []

    def replace(old, new, count=1):
        nonlocal text
        actual = text.count(old)
        if actual != count:
            raise ValueError('bump rewrite count mismatch for %s: expected %d, got %d: %r' %
                             (symbol, count, actual, old))
        text = text.replace(old, new)
        changes.append(dict(find_sha256=digest(old.encode()),
                            replace_sha256=digest(new.encode()), count=count,
                            find=old, replace=new))

    # Exact original globals become the sole retained source record. This does
    # not mirror Actor fields: the original loop reads bound ActorSource fields
    # directly from the actual native omGObjListHead[LINK_POKEMON] list.
    for name, count in GLOBAL_COUNTS[symbol].items():
        replace(name, 'bump->' + name, count)
    if symbol == 'BumpDetector_Update':
        replace('void BumpDetector_Update(GObj* arg0) {',
                'void isle_player_bump_BumpDetector_Update(GObj* arg0) {\n'
                '    IslePlayerBumpSource* bump = isle_player_bump_source_current(arg0);')
        replace('Pokemon* pokemon = GET_POKEMON(pokemonObj);',
                'IsleActorSource* pokemon = isle_player_bump_source_actor(bump, pokemonObj);')
        replace('cmdSendCommand(gObjPlayer, PLAYER_CMD_BUMP, arg0);',
                'isle_player_bump_source_record(bump, isle_player_bump_native_command(\n'
                '                bump, bump->player->object, PLAYER_CMD_BUMP, arg0));\n'
                '            isle_player_bump_source_checked(bump);')
        replace('ohWait(1);', 'isle_player_bump_source_wait(bump, 1);')
    else:
        arguments = 's32 arg0' if symbol == 'BumpDetector_Delete' else 'void'
        native_arguments = 'IslePlayerBumpSource* bump'
        if symbol == 'BumpDetector_Delete':
            native_arguments += ', s32 arg0'
        guard = ('\n    if (!bump || !bump->player || bump->player->bump != bump || !bump->owner)\n'
                 '        return ISLE_GAME_INVALID_ARGUMENT;')
        if symbol != 'BumpDetector_Delete':
            guard += '\n    if (bump->status != ISLE_GAME_OK) return bump->status;'
        replace('void %s(%s) {' % (symbol, arguments),
                'IsleGameResult isle_player_bump_%s(%s) {' % (symbol, native_arguments) + guard)
        if symbol == 'BumpDetector_Init':
            replace('omAddGObj(OBJID_BUMP_DETECTOR, ohUpdateDefault, LINK_0, 0x80000000);',
                    'isle_player_bump_source_object(bump, OBJID_BUMP_DETECTOR, LINK_0, 0x80000000);\n'
                    '    if (bump->status != ISLE_GAME_OK) return bump->status;')
            replace('omCreateProcess(bump->BumpDetector_Object, BumpDetector_Update, 0, 9);',
                    'isle_player_bump_source_process(bump, bump->BumpDetector_Object,\n'
                    '        isle_player_bump_BumpDetector_Update, 0, 9);\n'
                    '    if (bump->status != ISLE_GAME_OK) return bump->status;')
        elif symbol == 'BumpDetector_Disable':
            replace('ohPauseObjectProcesses(bump->BumpDetector_Object);',
                    'isle_player_bump_source_pause(bump, bump->BumpDetector_Object, 1);')
        elif symbol == 'BumpDetector_Enable':
            replace('ohResumeObjectProcesses(bump->BumpDetector_Object);',
                    'isle_player_bump_source_pause(bump, bump->BumpDetector_Object, 0);')
        elif symbol == 'BumpDetector_Delete':
            replace('omDeleteGObj(bump->BumpDetector_Object);',
                    'isle_player_bump_source_record(bump,\n'
                    '            isle_player_bump_native_delete(bump, bump->BumpDetector_Object));')
            # Native self-deletion may only be scheduled at this point. Move the
            # original clear's ownership to the actual Services joined observer;
            # no early clear, surrogate join, or second scheduler is admitted.
            replace('        bump->BumpDetector_Object = NULL;',
                    '        /* Canonical Object clears only at the actual native joined callback. */')
        replace('\n}\n', '\n    return bump->status;\n}\n')
    return text, changes


def prepare(repository, manifest_path, output_path, evidence_path):
    raw = manifest_path.read_bytes()
    manifest = json.loads(raw)
    if (manifest['schema_version'] != 1 or manifest['revision'] != REVISION or
            manifest['transformation_version'] != VERSION):
        raise ValueError('bump authority/version mismatch')
    if set(manifest['source_files']) != set(FILES):
        raise ValueError('bump source allowlist mismatch')
    if git(repository, 'cat-file', '-t', REVISION).strip() != b'commit':
        raise ValueError('bump authority is not the pinned commit')
    sources = {}
    for path, identity in manifest['source_files'].items():
        data = git(repository, 'show', REVISION + ':' + path)
        blob = hashlib.sha1(b'blob ' + str(len(data)).encode() + b'\0' + data).hexdigest()
        if identity['git_blob'] != blob or identity['sha256'] != digest(data):
            raise ValueError('bump Git object identity mismatch: ' + path)
        sources[path] = data.decode().splitlines(keepends=True)
    if len(sources[SOURCE]) != 69:
        raise ValueError('bump complete source length mismatch')
    if manifest['generator_sha256'] != digest(Path(__file__).read_bytes()):
        raise ValueError('bump generator identity mismatch')
    if set(manifest['native_support']) != set(SUPPORT):
        raise ValueError('bump native support allowlist mismatch')
    root = Path(__file__).resolve().parent.parent
    for path, identity in manifest['native_support'].items():
        if identity['sha256'] != digest((root / path).read_bytes()):
            raise ValueError('bump native support identity mismatch: ' + path)
    if len(manifest['semantic_origins']) != len(ORIGINS):
        raise ValueError('bump semantic inventory mismatch')
    for origin, (path, label, first, last) in zip(manifest['semantic_origins'], ORIGINS):
        if (origin['path'], origin['label'], origin['first'], origin['last']) != (path, label, first, last):
            raise ValueError('bump semantic inventory mismatch')
        if origin['source_sha256'] != digest(''.join(sources[path][first-1:last]).encode()):
            raise ValueError('bump semantic source mismatch: ' + label)
    constants = []
    for path, name, expected in CONSTANTS:
        pattern = r'^\s*' + re.escape(name) + r'\s*=\s*(0[xX][0-9a-fA-F]+|[0-9]+)\s*,?\s*$'
        values = re.findall(pattern, ''.join(sources[path]), re.MULTILINE)
        if len(values) != 1 or int(values[0], 0) != expected:
            raise ValueError('bump canonical enum mismatch: ' + name)
        constants.append('    %s = %s' % (name, values[0]))
    if len(manifest['definitions']) != len(DEFINITIONS):
        raise ValueError('bump complete definition inventory mismatch')
    output = [PREAMBLE, 'enum {\n' + ',\n'.join(constants) + '\n};\n\n']
    admitted = []
    for unit, (symbol, first, last) in zip(manifest['definitions'], DEFINITIONS):
        if (unit['path'], unit['symbol'], unit['first'], unit['last']) != (SOURCE, symbol, first, last):
            raise ValueError('bump complete definition inventory mismatch')
        selected = ''.join(sources[SOURCE][first-1:last])
        if unit['source_sha256'] != digest(selected.encode()):
            raise ValueError('bump complete body identity mismatch: ' + symbol)
        native, changes = transform(symbol, selected)
        output.extend(('#line %d "snap@%s/%s"\n' % (first, REVISION, SOURCE), native, '\n'))
        admitted.append(dict(unit, native_sha256=digest(native.encode()), changes=changes))
    result = ''.join(output)
    publish(output_path, result)
    publish(evidence_path, json.dumps(dict(manifest,
        manifest_sha256=digest(raw), transformed_definitions=admitted,
        source_sha256=digest(result.encode())), indent=2) + '\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('repository', 'manifest', 'output', 'evidence'):
        parser.add_argument('--' + name, type=Path, required=True)
    arguments = parser.parse_args()
    prepare(arguments.repository, arguments.manifest, arguments.output, arguments.evidence)


if __name__ == '__main__':
    main()
