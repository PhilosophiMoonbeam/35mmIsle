#!/usr/bin/env python3
"""Privately materialize exact pinned Rainbow main13 and complete intro8 C bodies."""
import argparse
import hashlib
import json
import re
from pathlib import Path
from prepare_engine import git, publish

REVISION = 'df8815e75f3d79618466e01e0827c6165b93d11c'
VERSION = 'rainbow-lifecycle-complete13-plus8-v1'
SOURCE_INVENTORY_SHA256 = 'e844e3445fc547b572af18e754b17206a05ea257cffcaff915d6b5e7bee325ee'
SELECTION_SHA256 = 'c713e90d33349c160d6df821e4509f68235f77a7d7053f9f520f90702ba506cf'
RECIPE_SHA256 = '2b3857649064b615f9bc8816c4a263d2bbaac862e36ae4d55f7986a065637591'
OUTPUT_SHA256 = '8d1d31805f2cc0a20fd41af2c14272a257d5b289ac60358429ff1e46a25c36c8'
ROM_SHA1 = 'edc7c49cc568c045fe48be0d18011c30f393cbaf'
SUPPORT = (
    'src/rainbow_lifecycle_source.h', 'src/rainbow_lifecycle_source.c',
    'src/rainbow_lifecycle.hpp', 'src/rainbow_lifecycle.cpp',
    'src/rainbow_actor_source.h', 'src/rainbow_actor_source.c',
    'src/actor_owner_source.h', 'src/actor_source.h',
    'src/player_source.h', 'src/player_root_source.h', 'src/player_root_source.c',
    'src/player_cart_source.h', 'src/player_cart_source.c',
    'src/player_camera_animation_clip.h', 'src/world_movement_state.h',
    'src/menu_scene_bridge.h', 'src/engine_internal.h',
    'include/35mmisle/engine.h', 'include/35mmisle/continuation.h',
    'include/35mmisle/rng.h', 'tools/prepare_engine.py',
)
PREAMBLE = '''/* Complete pinned own-GAME Rainbow main13 and intro8; no SDK layouts. */
#include "engine_internal.h"
#include "rainbow_lifecycle_source.h"
#define A_BUTTON 0x8000u
#define START_BUTTON 0x1000u
'''
RECIPE_KEYS = ('definitions', 'constant_names', 'semantic_origins', 'rom_facts',
               'callback_dependencies')
CONSTANT_SOURCES = ('include/enums.h', 'src/world/world.h',
                    'src/app_level/app_level.h', 'include/sys/audio.h')


def digest(data):
    return hashlib.sha256(data).hexdigest()


def canonical_digest(value):
    return digest(json.dumps(value, sort_keys=True, separators=(',', ':')).encode())


def rewrite(original, unit):
    body = original
    for change in unit['changes']:
        count = change['occurrences']
        if count < 1 or body.count(change['find']) != count:
            raise ValueError('Rainbow lifecycle counted rewrite mismatch: ' +
                             unit['symbol'] + ': ' + change['find'])
        body = body.replace(change['find'], change['replace'])
    if digest(body.encode()) != unit['native_sha256']:
        raise ValueError('Rainbow lifecycle complete transformed definition mismatch: ' + unit['symbol'])
    if body[:body.index('{')].strip() != unit['native_declaration']:
        raise ValueError('Rainbow lifecycle typed declaration mismatch: ' + unit['symbol'])
    return body


def prepare(repository, manifest_path, output_path, evidence_path):
    manifest_bytes = manifest_path.read_bytes()
    manifest = json.loads(manifest_bytes)
    root = Path(__file__).resolve().parent.parent
    if (manifest['schema_version'] != 1 or manifest['revision'] != REVISION or
            manifest['transformation_version'] != VERSION):
        raise ValueError('Rainbow lifecycle source authority/version mismatch')
    output, evidence = output_path.resolve(), evidence_path.resolve()
    parent = output.parent
    protected = (repository.resolve(), *(root / name for name in
                 ('src', 'include', 'tests', 'tools', 'cmake', '.git', 'graft')))
    if (output.name != 'game.c' or evidence.name != 'evidence.json' or
            output == evidence or parent != evidence.parent or
            parent.name != 'isle-rainbow-lifecycle-private' or parent.parent.name != '_deps' or
            any(parent == base or base in parent.parents for base in protected)):
        raise ValueError('Rainbow lifecycle requires exact private build paths outside protected source trees')
    if manifest['generator_sha256'] != digest(Path(__file__).read_bytes()):
        raise ValueError('Rainbow lifecycle generator identity mismatch')
    if canonical_digest(manifest['source_files']) != SOURCE_INVENTORY_SHA256:
        raise ValueError('Rainbow lifecycle source inventory mismatch')
    if canonical_digest({key: manifest[key] for key in RECIPE_KEYS}) != RECIPE_SHA256:
        raise ValueError('Rainbow lifecycle complete recipe/ROM/callback identity mismatch')
    if set(manifest['native_support']) != set(SUPPORT):
        raise ValueError('Rainbow lifecycle closed native support allowlist mismatch')
    if git(repository, 'cat-file', '-t', REVISION).strip() != b'commit':
        raise ValueError('Rainbow lifecycle authority is not the pinned commit')
    sources = {}
    for path, identity in manifest['source_files'].items():
        raw = git(repository, 'show', REVISION + ':' + path)
        blob = hashlib.sha1(b'blob ' + str(len(raw)).encode() + b'\0' + raw).hexdigest()
        if (blob != identity['git_blob'] or digest(raw) != identity['sha256'] or
                len(raw.splitlines()) != identity['line_count']):
            raise ValueError('Rainbow lifecycle complete pinned source identity mismatch: ' + path)
        sources[path] = raw.decode()
    for path, identity in manifest['native_support'].items():
        selected = (root / path).resolve()
        if root not in selected.parents or digest(selected.read_bytes()) != identity['sha256']:
            raise ValueError('Rainbow lifecycle native support identity mismatch: ' + path)
    # Locked authoring receipt only. Builds never read a ROM; actual runtime C
    # wire hydration receives the identity-admitted canonical Snapshot.
    facts = manifest['rom_facts']
    if facts['rom_sha1'] != ROM_SHA1 or facts['rom_bytes'] != 0x1000000:
        raise ValueError('Rainbow lifecycle runtime ROM authority mismatch')
    for extent in facts['ranges']:
        first, length = int(extent['rom_offset'], 0), extent['bytes']
        if length <= 0 or first < 0 or first + length > facts['rom_bytes']:
            raise ValueError('Rainbow lifecycle invalid ROM numeric extent: ' + extent['label'])
    for origin in manifest['semantic_origins']:
        selected = ''.join(sources[origin['path']].splitlines(keepends=True)
                           [origin['first'] - 1:origin['last']])
        if digest(selected.encode()) != origin['source_sha256']:
            raise ValueError('Rainbow lifecycle semantic source origin mismatch: ' + origin['label'])
    units = manifest['definitions']
    selection = [[unit[key] for key in ('path', 'symbol', 'first', 'last', 'owner')]
                 for unit in units]
    if (digest(json.dumps(selection, separators=(',', ':')).encode()) != SELECTION_SHA256 or
            len(units) != 21 or len({unit['symbol'] for unit in units}) != 21 or
            {owner: sum(unit['owner'] == owner for unit in units) for owner in ('setup', 'intro')} !=
            {'setup': 13, 'intro': 8} or
            {unit['path'] for unit in units} != {'src/rainbow/main.c', 'src/rainbow/intro.c'} or
            set(manifest['callback_dependencies']) != {unit['symbol'] for unit in units}):
        raise ValueError('Rainbow lifecycle requires complete unique13-plus8 source ownership and callback closure')
    constants = {}
    for path in CONSTANT_SOURCES:
        for block in re.findall(r'enum\s*\w*\s*\{(.*?)\};', sources[path], re.S):
            value = -1
            for part in re.sub(r'//[^\n]*', '', block).split(','):
                part = re.sub(r'/\*.*?\*/', '', part, flags=re.S)
                match = re.fullmatch(r'\s*(\w+)(?:\s*=\s*(0x[\dA-Fa-f]+|\d+))?\s*', part)
                if match:
                    value = int(match[2], 0) if match[2] else value + 1
                    if match[1] in manifest['constant_names']:
                        constants[match[1]] = value
    if set(constants) != set(manifest['constant_names']):
        raise ValueError('Rainbow lifecycle source constant coverage mismatch')
    result = [PREAMBLE, 'enum {\n' + ''.join('    %s = %d,\n' % pair
              for pair in sorted(constants.items())) + '};\n']
    for unit in units:
        original = ''.join(sources[unit['path']].splitlines(keepends=True)
                           [unit['first'] - 1:unit['last']])
        if digest(original.encode()) != unit['source_sha256']:
            raise ValueError('Rainbow lifecycle complete original definition mismatch: ' + unit['symbol'])
        result.extend(['#line %d "snap@%s/%s"\n' % (unit['first'], REVISION, unit['path']),
                       rewrite(original, unit), '\n'])
    generated = ''.join(result)
    if digest(generated.encode()) != OUTPUT_SHA256 or manifest['output_sha256'] != OUTPUT_SHA256:
        raise ValueError('Rainbow lifecycle complete generated output identity mismatch')
    receipt = dict(manifest, manifest_sha256=digest(manifest_bytes),
        source_inventory_sha256=SOURCE_INVENTORY_SHA256, selection_sha256=SELECTION_SHA256,
        recipe_sha256=RECIPE_SHA256, ownership_counts={'setup': 13, 'intro': 8},
        scope='COMPLETE21_PINNED_ORIGINAL_C_RAINBOW_LIFECYCLE_TYPED_NATIVE_CONTRACTS_ONLY',
        native_owner_execution='NOT_CLAIMED: requires concrete native owners and actual acceptance',
        rom_data_admission='ROM-independent build; numeric authoring receipt; actual admitted-Snapshot runtime hydration',
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
