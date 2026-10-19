#!/usr/bin/env python3
"""Materialize the complete pinned Tunnel main12 and intro7 privately."""
import argparse
import hashlib
import json
from pathlib import Path
from prepare_engine import git, publish

REVISION = 'df8815e75f3d79618466e01e0827c6165b93d11c'
VERSION = 'tunnel-lifecycle-complete12-plus7-v1'
ROM_SHA1 = 'edc7c49cc568c045fe48be0d18011c30f393cbaf'
SOURCE_INVENTORY_SHA256 = '5b6cf59171416678c52f2de29b9e47fccfa53e768b48e664b98ef2ebd8bfae4f'
SELECTION_SHA256 = 'b5c7fc04b0dbd63331ce272f19d7b44931e9893bdc435130bc075174220e1e84'
RECIPE_SHA256 = 'e8fda7f8fc9e05d77f420a64ba5e8fc5ea0102b94aab4492e9f7d2a5df841500'
OUTPUT_SHA256 = '4d27a8007366c1dc75eff64a9ac560a5879a9ddde32bcd7f97a0869431f37ef3'
SUPPORT = ('src/tunnel_lifecycle_source.h', 'src/tunnel_lifecycle_source.c')
RECIPE_KEYS = ('definitions', 'constants', 'semantic_origins', 'rom_facts',
               'linker_facts', 'callback_dependencies', 'actor_bridges')
PREAMBLE = '/* Complete own-GAME pinned Tunnel main12/intro7; no SDK layouts. */\n#include "engine_internal.h"\n#include "tunnel_lifecycle_source.h"\n#define A_BUTTON 0x8000u\n#define START_BUTTON 0x1000u\n'


def digest(data):
    return hashlib.sha256(data).hexdigest()


def canonical_digest(value):
    return digest(json.dumps(value, sort_keys=True, separators=(',', ':')).encode())


def rewrite(original, unit):
    body = original
    for change in unit['changes']:
        count = change['occurrences']
        if count < 1 or body.count(change['find']) != count:
            raise ValueError('Tunnel lifecycle counted rewrite mismatch: ' +
                             unit['symbol'] + ': ' + change['find'])
        body = body.replace(change['find'], change['replace'])
    if digest(body.encode()) != unit['native_sha256']:
        raise ValueError('Tunnel lifecycle transformed definition mismatch: ' + unit['symbol'])
    if body[:body.index('{')].strip() != unit['native_declaration']:
        raise ValueError('Tunnel lifecycle typed declaration mismatch: ' + unit['symbol'])
    return body


def prepare(repository, manifest_path, output_path, evidence_path):
    manifest_bytes = manifest_path.read_bytes()
    manifest = json.loads(manifest_bytes)
    root = Path(__file__).resolve().parent.parent
    if (manifest['schema_version'] != 1 or manifest['revision'] != REVISION or
            manifest['transformation_version'] != VERSION):
        raise ValueError('Tunnel lifecycle source authority/version mismatch')
    output, evidence = output_path.resolve(), evidence_path.resolve()
    parent = output.parent
    protected = (repository.resolve(), *(root / name for name in
                 ('src', 'include', 'tests', 'tools', 'cmake', '.git', 'graft')))
    if (output.name != 'game.c' or evidence.name != 'evidence.json' or
            output == evidence or parent != evidence.parent or
            parent.name != 'isle-tunnel-lifecycle-private' or parent.parent.name != '_deps' or
            any(parent == base or base in parent.parents for base in protected)):
        raise ValueError('Tunnel lifecycle requires private build paths outside protected trees')
    if manifest['generator_sha256'] != digest(Path(__file__).read_bytes()):
        raise ValueError('Tunnel lifecycle generator identity mismatch')
    if canonical_digest(manifest['source_files']) != SOURCE_INVENTORY_SHA256:
        raise ValueError('Tunnel lifecycle source inventory mismatch')
    if canonical_digest({key: manifest[key] for key in RECIPE_KEYS}) != RECIPE_SHA256:
        raise ValueError('Tunnel lifecycle complete recipe identity mismatch')
    if set(manifest['native_support']) != set(SUPPORT):
        raise ValueError('Tunnel lifecycle native support allowlist mismatch')
    if git(repository, 'cat-file', '-t', REVISION).strip() != b'commit':
        raise ValueError('Tunnel lifecycle authority is not the pinned commit')
    sources = {}
    for path, identity in manifest['source_files'].items():
        raw = git(repository, 'show', REVISION + ':' + path)
        blob = hashlib.sha1(b'blob ' + str(len(raw)).encode() + b'\0' + raw).hexdigest()
        if (blob != identity['git_blob'] or digest(raw) != identity['sha256'] or
                len(raw.splitlines()) != identity['line_count']):
            raise ValueError('Tunnel lifecycle pinned source identity mismatch: ' + path)
        sources[path] = raw
    for path, identity in manifest['native_support'].items():
        selected = (root / path).resolve()
        if root not in selected.parents or digest(selected.read_bytes()) != identity['sha256']:
            raise ValueError('Tunnel lifecycle native support identity mismatch: ' + path)
    facts = manifest['rom_facts']
    if facts['rom_sha1'] != ROM_SHA1 or facts['rom_bytes'] != 0x1000000:
        raise ValueError('Tunnel lifecycle runtime ROM authority mismatch')
    for extent in facts['ranges']:
        first, length = int(extent['rom_offset'], 0), extent['bytes']
        if length <= 0 or first < 0 or first + length > facts['rom_bytes']:
            raise ValueError('Tunnel lifecycle invalid numeric extent: ' + extent['label'])
    for origin in manifest['semantic_origins']:
        selected = b''.join(sources[origin['path']].splitlines(keepends=True)
                            [origin['first'] - 1:origin['last']])
        if digest(selected) != origin['source_sha256']:
            raise ValueError('Tunnel lifecycle semantic Source origin mismatch: ' + origin['label'])
    units = manifest['definitions']
    selection = [[unit[key] for key in ('path', 'symbol', 'first', 'last', 'owner')]
                 for unit in units]
    if (digest(json.dumps(selection, separators=(',', ':')).encode()) != SELECTION_SHA256 or
            len(units) != 19 or len({unit['symbol'] for unit in units}) != 19 or
            {owner: sum(unit['owner'] == owner for unit in units) for owner in ('setup', 'intro')} !=
            {'setup': 12, 'intro': 7} or
            {unit['path'] for unit in units} != {'src/tunnel/main.c', 'src/tunnel/intro.c'} or
            set(manifest['callback_dependencies']) != {unit['symbol'] for unit in units}):
        raise ValueError('Tunnel lifecycle requires complete unique12-plus7 Source ownership')
    result = [PREAMBLE, 'enum {\n' + ''.join('    %s = %d,\n' % pair
              for pair in sorted(manifest['constants'].items())) + '};\n']
    for unit in units:
        raw = sources[unit['path']]
        first, end = unit['byte_start'], unit['byte_end_exclusive']
        original = raw[first:end].decode()
        if (digest(original.encode()) != unit['source_sha256'] or
                raw[:first].count(b'\n') + 1 != unit['first'] or
                raw[:end].count(b'\n') + 1 != unit['last'] or
                original[:original.index('{')].strip() != unit['source_signature']):
            raise ValueError('Tunnel lifecycle original definition/signature mismatch: ' + unit['symbol'])
        result.extend(['#line %d "snap@%s/%s"\n' % (unit['first'], REVISION, unit['path']),
                       rewrite(original, unit), '\n'])
    generated = ''.join(result)
    if digest(generated.encode()) != OUTPUT_SHA256 or manifest['output_sha256'] != OUTPUT_SHA256:
        raise ValueError('Tunnel lifecycle generated output identity mismatch')
    receipt = dict(manifest, manifest_sha256=digest(manifest_bytes),
        source_inventory_sha256=SOURCE_INVENTORY_SHA256, selection_sha256=SELECTION_SHA256,
        recipe_sha256=RECIPE_SHA256, ownership_counts={'setup': 12, 'intro': 7},
        scope='COMPLETE19_PINNED_ORIGINAL_C_TUNNEL_LIFECYCLE_TYPED_NATIVE_CONTRACTS',
        native_owner_execution='NOT_CLAIMED: requires parent native owners and acceptance',
        rom_data_admission='ROM-independent build; actual identity-admitted Snapshot hydration at runtime',
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
