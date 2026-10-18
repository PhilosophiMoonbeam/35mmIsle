#!/usr/bin/env python3
"""Privately materialize all14 Beach setup/end and all7 original intro C bodies."""
import argparse
import hashlib
import json
import re
from pathlib import Path
from prepare_engine import git, publish

REVISION = 'df8815e75f3d79618466e01e0827c6165b93d11c'
VERSION = 'beach-lifecycle-complete14-plus7-v2'
SOURCE_INVENTORY_SHA256 = '1bc39ea13382512d4581d6ab44a957fccaddeaeb5505d54f3dd3b32c0d39f8df'
SELECTION_SHA256 = '60eba2a984bde2174935e7424461883b4cd13f79688b0b109e904cd92bf8e735'
RECIPE_SHA256 = '46d2e241ba11085a4498198dbea51447663cac33ffcb4e7a5dcbf63a2266cce0'
OUTPUT_SHA256 = '8aec92a1c56229bff0b235e8d96d1603fbc8b5a206a6796646574c8a7c88161d'
ROM_SHA1 = 'edc7c49cc568c045fe48be0d18011c30f393cbaf'
SUPPORT = (
    'src/world_module.hpp',
    'src/beach_lifecycle_source.h', 'src/beach_lifecycle_source.c',
    'src/beach_lifecycle.hpp', 'src/beach_lifecycle.cpp',
    'src/application_photos.hpp', 'src/application_photos.cpp',
    'src/application_photos_source.h',
    'src/actor_photo_scene.hpp', 'src/actor_photo_scene.cpp',
    'src/player_root.hpp', 'src/player_root.cpp',
    'src/beach_music.hpp', 'src/beach_music.cpp', 'src/menu_scene_bridge.h',
    'src/actor_source.h', 'src/player_source.h', 'src/player_root_source.h',
    'src/player_cart_source.h', 'src/player_camera_animation_clip.h',
    'src/beach_music_source.h', 'src/engine_internal.h',
    'include/35mmisle/engine.h', 'include/35mmisle/continuation.h',
    'include/35mmisle/rng.h', 'tools/prepare_engine.py',
)
PREAMBLE = '''/* Complete pinned own-GAME Beach setup/end and intro; no SDK layouts. */
#include "engine_internal.h"
#include "beach_lifecycle_source.h"
#define A_BUTTON 0x8000u
#define START_BUTTON 0x1000u
'''


def digest(data):
    return hashlib.sha256(data).hexdigest()


def canonical_digest(value):
    return digest(json.dumps(value, sort_keys=True, separators=(',', ':')).encode())


def rewrite(original, unit):
    body = original
    for change in unit['changes']:
        count = change['occurrences']
        if count < 1 or body.count(change['find']) != count:
            raise ValueError('Beach lifecycle counted rewrite mismatch: ' + unit['symbol'] + ': ' + change['find'])
        body = body.replace(change['find'], change['replace'])
    if digest(body.encode()) != unit['native_sha256']:
        raise ValueError('Beach lifecycle transformed definition mismatch: ' + unit['symbol'])
    if body[:body.index('{')].strip() != unit['native_declaration']:
        raise ValueError('Beach lifecycle typed declaration mismatch: ' + unit['symbol'])
    return body


def prepare(repository, manifest_path, output_path, evidence_path):
    manifest_bytes = manifest_path.read_bytes()
    manifest = json.loads(manifest_bytes)
    root = Path(__file__).resolve().parent.parent
    if (manifest['schema_version'] != 1 or manifest['revision'] != REVISION or
            manifest['transformation_version'] != VERSION):
        raise ValueError('Beach lifecycle authority/version mismatch')
    output, evidence = output_path.resolve(), evidence_path.resolve()
    parent = output.parent
    protected = (repository.resolve(), *(root / name for name in
                 ('src', 'include', 'tests', 'tools', 'cmake', '.git', 'graft')))
    if (output.name != 'game.c' or evidence.name != 'evidence.json' or
            output == evidence or parent != evidence.parent or
            parent.name != 'isle-beach-lifecycle-private' or parent.parent.name != '_deps' or
            any(parent == base or base in parent.parents for base in protected)):
        raise ValueError('Beach lifecycle requires exact private build output paths outside source trees')
    if manifest['generator_sha256'] != digest(Path(__file__).read_bytes()):
        raise ValueError('Beach lifecycle generator identity mismatch')
    if canonical_digest(manifest['source_files']) != SOURCE_INVENTORY_SHA256:
        raise ValueError('Beach lifecycle source inventory mismatch')
    recipe = {key: manifest[key] for key in
              ('definitions', 'constant_names', 'semantic_origins', 'rom_facts')}
    if canonical_digest(recipe) != RECIPE_SHA256:
        raise ValueError('Beach lifecycle complete recipe/ROM fact identity mismatch')
    if set(manifest['native_support']) != set(SUPPORT):
        raise ValueError('Beach lifecycle native support allowlist mismatch')
    if git(repository, 'cat-file', '-t', REVISION).strip() != b'commit':
        raise ValueError('Beach lifecycle authority is not the pinned commit')
    sources = {}
    for path, identity in manifest['source_files'].items():
        raw = git(repository, 'show', REVISION + ':' + path)
        blob = hashlib.sha1(b'blob ' + str(len(raw)).encode() + b'\0' + raw).hexdigest()
        if (blob != identity['git_blob'] or digest(raw) != identity['sha256'] or
                len(raw.splitlines()) != identity['line_count']):
            raise ValueError('Beach lifecycle pinned source identity mismatch: ' + path)
        sources[path] = raw.decode()
    for path, identity in manifest['native_support'].items():
        selected = (root / path).resolve()
        if root not in selected.parents or digest(selected.read_bytes()) != identity['sha256']:
            raise ValueError('Beach lifecycle native support identity mismatch: ' + path)
    # ROM facts are a locked authoring receipt, not generated-code inputs.
    # Actual data hydration receives the identity-admitted Snapshot at runtime;
    # public builds must never require a ROM just to compile these C bodies.
    facts = manifest['rom_facts']
    if facts['rom_sha1'] != ROM_SHA1 or facts['rom_bytes'] != 0x1000000:
        raise ValueError('Beach lifecycle runtime ROM authority mismatch')
    for extent in facts['ranges']:
        first, length = int(extent['rom_offset'], 0), extent['bytes']
        if length <= 0 or first < 0 or first + length > facts['rom_bytes']:
            raise ValueError('Beach lifecycle runtime ROM extent is invalid: ' + extent['label'])
    for origin in manifest['semantic_origins']:
        selected = ''.join(sources[origin['path']].splitlines(keepends=True)[origin['first'] - 1:origin['last']])
        if digest(selected.encode()) != origin['source_sha256']:
            raise ValueError('Beach lifecycle semantic source origin mismatch: ' + origin['label'])
    units = manifest['definitions']
    selection = [[unit[key] for key in ('path', 'symbol', 'first', 'last', 'owner')] for unit in units]
    if (digest(json.dumps(selection, separators=(',', ':')).encode()) != SELECTION_SHA256 or
            len(units) != 21 or len({unit['symbol'] for unit in units}) != 21 or
            {owner: sum(unit['owner'] == owner for unit in units) for owner in ('setup', 'intro')} !=
            {'setup': 14, 'intro': 7} or
            {unit['path'] for unit in units} != {'src/beach/55C110.c', 'src/beach/55D1C0.c'}):
        raise ValueError('Beach lifecycle requires complete unique14-plus7 source ownership')
    constants = {}
    for path in ('include/enums.h', 'src/world/world.h'):
        for block in re.findall(r'enum\s*\w*\s*\{(.*?)\};', sources[path], re.S):
            value = -1
            for part in re.sub(r'//[^\n]*', '', block).split(','):
                match = re.fullmatch(r'\s*(\w+)(?:\s*=\s*(0x[\dA-Fa-f]+|\d+))?\s*', part)
                if match:
                    value = int(match[2], 0) if match[2] else value + 1
                    if match[1] in manifest['constant_names']:
                        constants[match[1]] = value
    if set(constants) != set(manifest['constant_names']):
        raise ValueError('Beach lifecycle constant source coverage mismatch')
    result = [PREAMBLE, 'enum {\n' + ''.join('    %s = %d,\n' % pair
              for pair in sorted(constants.items())) + '};\n']
    for unit in units:
        original = ''.join(sources[unit['path']].splitlines(keepends=True)[unit['first'] - 1:unit['last']])
        if digest(original.encode()) != unit['source_sha256']:
            raise ValueError('Beach lifecycle complete original definition mismatch: ' + unit['symbol'])
        result.extend(['#line %d "snap@%s/%s"\n' % (unit['first'], REVISION, unit['path']),
                       rewrite(original, unit), '\n'])
    generated = ''.join(result)
    if digest(generated.encode()) != OUTPUT_SHA256 or manifest['output_sha256'] != OUTPUT_SHA256:
        raise ValueError('Beach lifecycle complete generated output identity mismatch')
    receipt = dict(manifest, manifest_sha256=digest(manifest_bytes),
        source_inventory_sha256=SOURCE_INVENTORY_SHA256, selection_sha256=SELECTION_SHA256,
        recipe_sha256=RECIPE_SHA256, ownership_counts={'setup': 14, 'intro': 7},
        scope='COMPLETE21_PINNED_ORIGINAL_C_BEACH_LIFECYCLE_WITH_TYPED_NATIVE_OWNER',
        rom_data_admission='ROM-independent build; locked authoring facts, actual admitted-Snapshot hydration at runtime',
        generated_source_sha256=OUTPUT_SHA256)
    publish(output, generated)
    publish(evidence, json.dumps(receipt, indent=2) + '\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('repository', 'manifest', 'output', 'evidence'):
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    prepare(args.repository, args.manifest, args.output, args.evidence)


if __name__ == '__main__':
    main()
