#!/usr/bin/env python3
"""Materialize complete pinned own-GAME Beach ending privately; no SDK bodies."""
import argparse
import hashlib
import json
from pathlib import Path
from prepare_engine import git, publish

REVISION = 'df8815e75f3d79618466e01e0827c6165b93d11c'
VERSION = 'beach-ending-complete10-v2-shared-keep-list'
# Filled by the authoring receipt; changes require reviewing source ownership.
SOURCE_INVENTORY_SHA256 = '2620887b1a138b92618f82856479d088bda93c9f7a8dc7c3068783ea88645e58'
RECIPE_SHA256 = 'e44a4a08ef69723e54c3191ac3ae39b57a7feb453c40e49d0dc1ee2ab596b2a5'
OUTPUT_SHA256 = 'fe5117ccbef91ec91d36f3149c572b8d7d0ce3f316873a463a2a310a7499eb99'
SUPPORT = (
    'src/beach_ending_source.h', 'src/beach_ending_source.c',
    'src/beach_ending.hpp', 'src/beach_ending.cpp',
    'src/actor_owner.hpp', 'src/actor_owner.cpp', 'src/actor_owner_source.h',
    'src/actor_interaction_services.hpp', 'src/actor_interaction_services.cpp',
    'src/beach_initial_species.cpp', 'src/actor_source.h',
    'src/player_cart_source.h', 'src/player_root_source.h',
    'src/menu_scene_bridge.h', 'src/engine_internal.h', 'tools/prepare_engine.py',
    'src/actor_course_context.hpp', 'src/rainbow_actor_source.h',
    'src/rainbow_actor_source.c', 'src/rainbow_actor.hpp', 'src/rainbow_actor.cpp',
    'src/rainbow_lifecycle_source.h', 'src/player_root.hpp',
)
PREAMBLE = '''/* Complete own-GAME ending closure. No sign1004 duplicate, SDK bodies or layouts. */
#include "engine_internal.h"
#include "menu_scene_bridge.h"
#include "beach_ending_source.h"
typedef IsleActorSource Pokemon;
typedef IsleActorFactoryBlock WorldBlock;
typedef IsleActorFactorySpawn ObjectSpawn;
#define UNUSED
#define GET_POKEMON(object) isle_actor_source_require(object)
#define Pokemon_SpawnDlLink4 isle_actor_owner_spawn_dl4
static const IsleActorFactoryInitialization D_beach_802CDA10 = { UINT32_C(0x802cda10) };
'''


def digest(data):
    return hashlib.sha256(data).hexdigest()


def canonical_digest(value):
    return digest(json.dumps(value, sort_keys=True, separators=(',', ':')).encode())


def prepare(repository, manifest_path, output_path, evidence_path):
    manifest_bytes = manifest_path.read_bytes()
    manifest = json.loads(manifest_bytes)
    root = Path(__file__).resolve().parent.parent
    if (manifest['schema_version'] != 1 or manifest['revision'] != REVISION or
            manifest['transformation_version'] != VERSION):
        raise ValueError('Beach ending authority/version mismatch')
    output, evidence = output_path.resolve(), evidence_path.resolve()
    parent = output.parent
    protected = (repository.resolve(), *(root / name for name in
                 ('src', 'include', 'tests', 'tools', 'cmake', '.git', 'graft')))
    if (output.name != 'game.c' or evidence.name != 'evidence.json' or output == evidence or
            parent != evidence.parent or parent.name != 'isle-beach-ending-private' or
            parent.parent.name != '_deps' or
            any(parent == base or base in parent.parents for base in protected)):
        raise ValueError('Beach ending requires exact private build output paths')
    if manifest['generator_sha256'] != digest(Path(__file__).read_bytes()):
        raise ValueError('Beach ending generator identity mismatch')
    if canonical_digest(manifest['source_files']) != SOURCE_INVENTORY_SHA256:
        raise ValueError('Beach ending source inventory mismatch')
    recipe = {key: manifest[key] for key in ('definitions', 'semantic_origins', 'rom_facts', 'ownership')}
    if canonical_digest(recipe) != RECIPE_SHA256:
        raise ValueError('Beach ending complete recipe/ROM/source ownership mismatch')
    if set(manifest['native_support']) != set(SUPPORT):
        raise ValueError('Beach ending native support allowlist mismatch')
    if git(repository, 'cat-file', '-t', REVISION).strip() != b'commit':
        raise ValueError('Beach ending authority is not pinned commit')
    sources = {}
    for path, identity in manifest['source_files'].items():
        raw = git(repository, 'show', REVISION + ':' + path)
        blob = hashlib.sha1(b'blob ' + str(len(raw)).encode() + b'\0' + raw).hexdigest()
        if (blob != identity['git_blob'] or digest(raw) != identity['sha256'] or
                len(raw.splitlines()) != identity['line_count']):
            raise ValueError('Beach ending pinned source identity mismatch: ' + path)
        sources[path] = raw.decode()
    for path, identity in manifest['native_support'].items():
        selected = (root / path).resolve()
        if root not in selected.parents or digest(selected.read_bytes()) != identity['sha256']:
            raise ValueError('Beach ending native support identity mismatch: ' + path)
    units = manifest['definitions']
    expected = ('func_beach_802CA450', 'func_beach_802CA474', 'func_beach_802CA4CC',
                'func_beach_802CA5A8', 'func_beach_802CA5D4', 'func_beach_802CA5DC',
                'func_beach_802CA8C4', 'func_beach_802CA950', 'func_beach_802CAB14',
                'Pokemon_RemovePokemons')
    if tuple(unit['symbol'] for unit in units) != expected:
        raise ValueError('Beach ending requires complete unique10 definition closure')
    for origin in manifest['semantic_origins']:
        text = ''.join(sources[origin['path']].splitlines(keepends=True)[origin['first']-1:origin['last']])
        if digest(text.encode()) != origin['source_sha256']:
            raise ValueError('Beach ending semantic authority mismatch: ' + origin['label'])
    bodies = []
    for unit in units:
        body = ''.join(sources[unit['path']].splitlines(keepends=True)[unit['first']-1:unit['last']])
        if digest(body.encode()) != unit['source_sha256']:
            raise ValueError('Beach ending original definition mismatch: ' + unit['symbol'])
        for change in unit['changes']:
            if body.count(change['find']) != change['occurrences'] or change['occurrences'] < 1:
                raise ValueError('Beach ending counted rewrite mismatch: ' + unit['symbol'] + ': ' + change['find'])
            body = body.replace(change['find'], change['replace'])
        if (digest(body.encode()) != unit['native_sha256'] or
                body[:body.index('{')].strip() != unit['native_declaration']):
            raise ValueError('Beach ending transformed definition mismatch: ' + unit['symbol'])
        bodies.append(body)
    text = PREAMBLE + '\n'.join(bodies)
    if digest(text.encode()) != OUTPUT_SHA256 or manifest['output_sha256'] != OUTPUT_SHA256:
        raise ValueError('Beach ending complete generated output identity mismatch')
    receipt = {'schema_version': 1, 'revision': REVISION, 'transformation_version': VERSION,
               'manifest_sha256': digest(manifest_bytes), 'generator_sha256': manifest['generator_sha256'],
               'output_sha256': OUTPUT_SHA256, 'definition_count': len(units),
               'definitions': [{key: unit[key] for key in ('path', 'symbol', 'first', 'last', 'source_sha256', 'native_sha256')} for unit in units],
               'native_support': manifest['native_support'], 'ownership': manifest['ownership'],
               'rom_facts': manifest['rom_facts']}
    publish(output, text)
    publish(evidence, json.dumps(receipt, indent=2) + '\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repository', required=True, type=Path)
    parser.add_argument('--manifest', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--evidence', required=True, type=Path)
    args = parser.parse_args()
    prepare(args.repository, args.manifest, args.output, args.evidence)


if __name__ == '__main__':
    main()
