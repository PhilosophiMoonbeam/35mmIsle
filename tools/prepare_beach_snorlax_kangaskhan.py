#!/usr/bin/env python3
"""Materialize complete pinned own-GAME Snorlax/Kangaskhan in private build output."""
import argparse
import hashlib
import json
from pathlib import Path
from prepare_engine import git, publish

REVISION = 'df8815e75f3d79618466e01e0827c6165b93d11c'
VERSION = 'beach-snorlax-kangaskhan-complete32-v1'
SOURCE_INVENTORY_SHA256 = 'a9553dced8ed1bf1d2774ba7784c16618846f9f70a15bdf3ad4e2b84c46cff92'
RECIPE_SHA256 = '6c5570b956de9c8ea139a2a2998939b57ed24c75968352432d354bae29b1a258'
OUTPUT_SHA256 = '71c3441e9ae3c806ffcb3626daef1318239c7b87f0dbf97857369f11d478335f'
SUPPORT = ('src/beach_snorlax_kangaskhan_source.h', 'src/beach_snorlax_kangaskhan_source.c', 'src/beach_snorlax_kangaskhan.hpp', 'src/beach_snorlax_kangaskhan.cpp', 'src/actor_source.h', 'src/actor_owner_source.h', 'src/actor_source_bridge.c', 'src/actor_source_owner_bridge.c', 'src/actor_owner.hpp', 'src/actor_owner.cpp', 'src/actor_interaction_services.h', 'src/actor_interaction_services.cpp', 'src/actor_behavior_bindings.hpp', 'src/actor_behavior_bindings.cpp', 'src/actor_resources.hpp', 'src/actor_resource_catalog.hpp', 'src/beach_actor_scene.hpp', 'src/actor_resources.cpp', 'src/actor_resource_catalog.cpp', 'src/beach_actor_catalog.cpp', 'src/rainbow_actor_catalog.cpp', 'src/beach_actor_scene.cpp', 'src/actor_behavior_resources.hpp', 'src/actor_behavior_resources.cpp', 'src/engine_internal.h', 'tools/prepare_engine.py', 'src/actor_course_context.hpp', 'src/rainbow_actor_source.h', 'src/rainbow_actor_source.c', 'src/rainbow_actor.hpp', 'src/rainbow_actor.cpp', 'src/rainbow_lifecycle_source.h', 'src/player_root.hpp')
EXPECTED_SYMBOLS = ('func_beach_802C5620', 'func_beach_802C56C8', 'func_beach_802C5700', 'func_beach_802C5768', 'func_beach_802C5828', 'func_beach_802C5884', 'func_beach_802C58B8', 'func_beach_802C58EC', 'func_beach_802C594C', 'func_beach_802C59C0', 'func_beach_802C5A20', 'func_beach_802C5A94', 'func_beach_802C5AF4', 'func_beach_802C5B68', 'func_beach_802C5BD4', 'func_beach_802C5C28', 'func_beach_802C9AC0', 'func_beach_802C9AE4', 'func_beach_802C9B40', 'func_beach_802C9B9C', 'func_beach_802C9BF8', 'func_beach_802C9C88', 'func_beach_802C9CF8', 'func_beach_802C9D68', 'func_beach_802C9D70', 'func_beach_802C9DCC', 'func_beach_802C9E70', 'func_beach_802C9EBC', 'func_beach_802C9F30', 'func_beach_802C9F9C', 'func_beach_802CA010', 'func_beach_802CA05C')
PREAMBLE = '/* Complete pinned own-GAME Snorlax/Kangaskhan; no SDK bodies or layouts. */\n#include "engine_internal.h"\n#include "beach_snorlax_kangaskhan_source.h"\ntypedef IsleActorSource Pokemon;\ntypedef IsleActorFactoryBlock WorldBlock;\ntypedef IsleActorFactorySpawn ObjectSpawn;\n#define UNUSED\n#define GET_POKEMON(object) isle_actor_source_require(object)\n#define POKEMON_FLAG_8 0x008\n#define Pokemon_SpawnOnGround isle_actor_owner_spawn_ground\n'


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
        raise ValueError('Snorlax/Kangaskhan authority/version mismatch')
    output, evidence = output_path.resolve(), evidence_path.resolve()
    parent = output.parent
    protected = (repository.resolve(), *(root / name for name in
                 ('src', 'include', 'tests', 'tools', 'cmake', '.git', 'graft')))
    if (output.name != 'game.c' or evidence.name != 'evidence.json' or output == evidence or
            parent != evidence.parent or parent.name != 'isle-beach-snorlax-kangaskhan-private' or
            parent.parent.name != '_deps' or
            any(parent == base or base in parent.parents for base in protected)):
        raise ValueError('Snorlax/Kangaskhan requires exact private build output paths')
    if manifest['generator_sha256'] != digest(Path(__file__).read_bytes()):
        raise ValueError('Snorlax/Kangaskhan generator identity mismatch')
    if canonical_digest(manifest['source_files']) != SOURCE_INVENTORY_SHA256:
        raise ValueError('Snorlax/Kangaskhan source inventory mismatch')
    recipe = {key: manifest[key] for key in ('definitions', 'semantic_origins', 'rom_facts', 'ownership')}
    if canonical_digest(recipe) != RECIPE_SHA256:
        raise ValueError('Snorlax/Kangaskhan complete recipe/ROM/source ownership mismatch')
    if set(manifest['native_support']) != set(SUPPORT):
        raise ValueError('Snorlax/Kangaskhan native support allowlist mismatch')
    if git(repository, 'cat-file', '-t', REVISION).strip() != b'commit':
        raise ValueError('Snorlax/Kangaskhan authority is not pinned commit')
    sources = {}
    for path, identity in manifest['source_files'].items():
        raw = git(repository, 'show', REVISION + ':' + path)
        blob = hashlib.sha1(b'blob ' + str(len(raw)).encode() + b'\0' + raw).hexdigest()
        if (blob != identity['git_blob'] or digest(raw) != identity['sha256'] or
                len(raw.splitlines()) != identity['line_count']):
            raise ValueError('Snorlax/Kangaskhan pinned source identity mismatch: ' + path)
        sources[path] = raw.decode()
    for path, identity in manifest['native_support'].items():
        selected = (root / path).resolve()
        if root not in selected.parents or digest(selected.read_bytes()) != identity['sha256']:
            raise ValueError('Snorlax/Kangaskhan native support identity mismatch: ' + path)
    units = manifest['definitions']
    if tuple(unit['symbol'] for unit in units) != EXPECTED_SYMBOLS:
        raise ValueError('Snorlax/Kangaskhan requires complete unique32 definition closure')
    for origin in manifest['semantic_origins']:
        text = ''.join(sources[origin['path']].splitlines(keepends=True)[origin['first']-1:origin['last']])
        if digest(text.encode()) != origin['source_sha256']:
            raise ValueError('Snorlax/Kangaskhan semantic authority mismatch: ' + origin['label'])
    bodies = []
    for unit in units:
        body = ''.join(sources[unit['path']].splitlines(keepends=True)[unit['first']-1:unit['last']])
        if digest(body.encode()) != unit['source_sha256']:
            raise ValueError('Snorlax/Kangaskhan original definition mismatch: ' + unit['symbol'])
        for change in unit['changes']:
            if body.count(change['find']) != change['occurrences'] or change['occurrences'] < 1:
                raise ValueError('Snorlax/Kangaskhan counted rewrite mismatch: ' + unit['symbol'])
            body = body.replace(change['find'], change['replace'])
        if (digest(body.encode()) != unit['native_sha256'] or
                body[:body.index('{')].strip() != unit['native_declaration']):
            raise ValueError('Snorlax/Kangaskhan transformed definition mismatch: ' + unit['symbol'])
        bodies.append(body)
    text = PREAMBLE + '\n'.join(bodies)
    if digest(text.encode()) != OUTPUT_SHA256 or manifest['output_sha256'] != OUTPUT_SHA256:
        raise ValueError('Snorlax/Kangaskhan complete generated output identity mismatch')
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
