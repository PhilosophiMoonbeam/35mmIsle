#!/usr/bin/env python3
"""Materialize complete pinned own-GAME Meowth privately, without SDK bodies."""
import argparse
import hashlib
import json
from pathlib import Path
from prepare_engine import git, publish

REVISION = 'df8815e75f3d79618466e01e0827c6165b93d11c'
VERSION = 'beach-meowth-complete51-v2-borrowed-behavior'
SOURCE_INVENTORY_SHA256 = '64193dd3d6c57fa1cac8db905791e8e18c7637cd6dea00cd366a82c6b46c5ac6'
RECIPE_SHA256 = 'e1963c130f8fe91336de74a95d37caf3c787bdd4a68d263dca7e3fde83968961'
OUTPUT_SHA256 = '3cac7f62115e636d5081977656c3e55f0b8f6e71ad296ead2217cc83aca8bee3'
SUPPORT = ('src/beach_meowth_source.h', 'src/beach_meowth_source.c', 'src/beach_meowth.hpp', 'src/beach_meowth.cpp', 'src/actor_owner_source.h', 'src/actor_source.h', 'src/actor_owner.hpp', 'src/actor_owner.cpp', 'src/actor_source_owner_bridge.c', 'src/actor_behavior_bindings.hpp', 'src/actor_behavior_bindings.cpp', 'src/actor_interaction_services.h', 'src/actor_interaction_services.cpp', 'src/actor_resources.hpp', 'src/actor_resource_catalog.hpp', 'src/beach_actor_scene.hpp', 'src/actor_resources.cpp', 'src/actor_resource_catalog.cpp', 'src/beach_actor_catalog.cpp', 'src/rainbow_actor_catalog.cpp', 'src/beach_actor_scene.cpp', 'src/menu_scene_bridge.h', 'src/engine_internal.h', 'tools/prepare_engine.py', 'src/actor_course_context.hpp', 'src/rainbow_actor_source.h', 'src/rainbow_actor_source.c', 'src/rainbow_actor.hpp', 'src/rainbow_actor.cpp', 'src/rainbow_lifecycle_source.h', 'src/player_root.hpp')
EXPECTED = ('func_beach_802C5C60', 'func_beach_802C5DAC', 'func_beach_802C5E88', 'func_beach_802C5EF8', 'func_beach_802C5F9C', 'func_beach_802C5FF8', 'func_beach_802C60E8', 'func_beach_802C619C', 'func_beach_802C6210', 'func_beach_802C6288', 'func_beach_802C62E4', 'func_beach_802C63F4', 'func_beach_802C6478', 'func_beach_802C649C', 'func_beach_802C6514', 'func_beach_802C6580', 'func_beach_802C660C', 'func_beach_802C6658', 'func_beach_802C667C', 'func_beach_802C67F8', 'func_beach_802C6700', 'func_beach_802C67E0', 'func_beach_802C6854', 'func_beach_802C68D8', 'func_beach_802C6944', 'func_beach_802C69D4', 'func_beach_802C6A3C', 'func_beach_802C6AA0', 'func_beach_802C6B28', 'func_beach_802C6B8C', 'func_beach_802C6C10', 'func_beach_802C6C60', 'func_beach_802C6CC4', 'func_beach_802C6D20', 'func_beach_802C6D44', 'func_beach_802C6D9C', 'func_beach_802C6DD4', 'func_beach_802C6E40', 'func_beach_802C6EC0', 'func_beach_802C6F6C', 'func_beach_802C6FD4', 'func_beach_802C6FF8', 'func_beach_802C7050', 'func_beach_802C70C0', 'func_beach_802C7130', 'func_beach_802C71E8', 'func_beach_802C7270')
EXPECTED += ('func_80363D8C_50419C', 'pokemonChangeBlock', 'pokemonChangeBlockOnGround', 'Pokemon_ChangeBlockAndRemove')
PREAMBLE = '/* Complete pinned own-GAME Meowth closure; no SDK bodies or layouts. */\n#include "engine_internal.h"\n#include "menu_scene_bridge.h"\n#include "beach_meowth_source.h"\ntypedef IsleActorSource Pokemon;\ntypedef IsleActorGround GroundResult;\ntypedef IsleActorFactoryBlock WorldBlock;\ntypedef IsleActorFactorySpawn ObjectSpawn;\n#define UNUSED\n#define GET_POKEMON(object) isle_actor_source_require(object)\n#define Pokemon_Spawn isle_actor_owner_spawn\n#define Pokemon_SpawnOnGround isle_actor_owner_spawn_ground\nstatic const IsleActorFactoryInitialization D_beach_802CC968 = { UINT32_C(0x802cc968) };\n'

SUPPORT += ('src/tunnel_actor_catalog.cpp', 'src/volcano_actor_catalog.cpp',
            'src/river_actor_catalog.cpp',
            'src/cave_actor_catalog.cpp', 'src/valley_actor_catalog.cpp')


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
        raise ValueError('Meowth pinned authority/version mismatch')
    output, evidence = output_path.resolve(), evidence_path.resolve()
    parent = output.parent
    protected = (repository.resolve(), *(root / name for name in
                 ('src', 'include', 'tests', 'tools', 'cmake', '.git', 'graft')))
    if (output.name != 'game.c' or evidence.name != 'evidence.json' or output == evidence or
            parent != evidence.parent or parent.name != 'isle-beach-meowth-private' or
            parent.parent.name != '_deps' or
            any(parent == base or base in parent.parents for base in protected)):
        raise ValueError('Meowth requires exact private build output paths')
    if manifest['generator_sha256'] != digest(Path(__file__).read_bytes()):
        raise ValueError('Meowth generator identity mismatch')
    if canonical_digest(manifest['source_files']) != SOURCE_INVENTORY_SHA256:
        raise ValueError('Meowth source inventory mismatch')
    recipe = {key: manifest[key] for key in
              ('definitions', 'semantic_origins', 'rom_facts', 'ownership')}
    if canonical_digest(recipe) != RECIPE_SHA256:
        raise ValueError('Meowth complete recipe/ROM/source ownership mismatch')
    if set(manifest['native_support']) != set(SUPPORT):
        raise ValueError('Meowth native support allowlist mismatch')
    if git(repository, 'cat-file', '-t', REVISION).strip() != b'commit':
        raise ValueError('Meowth authority is not pinned commit')
    sources = {}
    for path, identity in manifest['source_files'].items():
        raw = git(repository, 'show', REVISION + ':' + path)
        blob = hashlib.sha1(b'blob ' + str(len(raw)).encode() + b'\0' + raw).hexdigest()
        if (blob != identity['git_blob'] or digest(raw) != identity['sha256'] or
                len(raw.splitlines()) != identity['line_count']):
            raise ValueError('Meowth pinned source identity mismatch: ' + path)
        sources[path] = raw.decode()
    for path, identity in manifest['native_support'].items():
        selected = (root / path).resolve()
        if root not in selected.parents or digest(selected.read_bytes()) != identity['sha256']:
            raise ValueError('Meowth native support identity mismatch: ' + path)
    units = manifest['definitions']
    if tuple(unit['symbol'] for unit in units) != EXPECTED:
        raise ValueError('Meowth requires all complete unique51 original definitions')
    for origin in manifest['semantic_origins']:
        text = ''.join(sources[origin['path']].splitlines(keepends=True)
                       [origin['first']-1:origin['last']])
        if digest(text.encode()) != origin['source_sha256']:
            raise ValueError('Meowth semantic authority mismatch: ' + origin['label'])
    bodies = []
    for unit in units:
        body = ''.join(sources[unit['path']].splitlines(keepends=True)
                       [unit['first']-1:unit['last']])
        if digest(body.encode()) != unit['source_sha256']:
            raise ValueError('Meowth original definition mismatch: ' + unit['symbol'])
        for change in unit['changes']:
            if change['occurrences'] < 1 or body.count(change['find']) != change['occurrences']:
                raise ValueError('Meowth counted rewrite mismatch: ' + unit['symbol'] + ': ' + change['find'])
            body = body.replace(change['find'], change['replace'])
        if (digest(body.encode()) != unit['native_sha256'] or
                body[:body.index('{')].strip() != unit['native_declaration']):
            raise ValueError('Meowth transformed definition mismatch: ' + unit['symbol'])
        bodies.append(body)
    text = PREAMBLE + '\n'.join(bodies)
    if digest(text.encode()) != OUTPUT_SHA256 or manifest['output_sha256'] != OUTPUT_SHA256:
        raise ValueError('Meowth complete generated output identity mismatch')
    receipt = {
        'schema_version': 1, 'revision': REVISION, 'transformation_version': VERSION,
        'manifest_sha256': digest(manifest_bytes), 'generator_sha256': manifest['generator_sha256'],
        'output_sha256': OUTPUT_SHA256, 'definition_count': len(units),
        'definitions': [{key: unit[key] for key in
                         ('path', 'symbol', 'first', 'last', 'source_sha256', 'native_sha256')}
                        for unit in units],
        'native_support': manifest['native_support'], 'ownership': manifest['ownership'],
        'rom_facts': manifest['rom_facts'],
    }
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
