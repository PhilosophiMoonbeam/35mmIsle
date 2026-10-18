#!/usr/bin/env python3
"""Privately materialize the complete pinned application manager and ten-body closure."""
import argparse
import hashlib
import json
import re
from pathlib import Path
from prepare_engine import git, publish

REVISION = 'df8815e75f3d79618466e01e0827c6165b93d11c'
VERSION = 'application-manager-complete10-v1'
SOURCE = 'src/app_render/46270.c'
SOURCE_INVENTORY_SHA256 = '6959af81f0549841f8b92a9e7574b6d12ff2f04140d80886c21941184b1e6f93'
RECIPE_SHA256 = 'c1a51911e6271522b3c13bed1defb5ddf8b963608fd996980715b51d2fc09b9d'
SELECTION_SHA256 = '2a56572187e7145fd93ffe71838120ca38abb4ab4f57c189bbe2614ed5d2cd48'
OUTPUT_SHA256 = '891fbaef498644816241985f716ad31fd4ece9e657c080e23e6bf0e6fd083ee6'
SUPPORT = (
    'src/application_source.h', 'src/application_source.c', 'src/player_source.h',
    'include/35mmisle/continuation.h', 'include/35mmisle/engine.h',
    'include/35mmisle/game_save.hpp', 'src/game_save.cpp',
    'cmake/game-save-inputs.json', 'tools/prepare_engine.py',
    'src/application_manager.hpp', 'src/application_manager.cpp',
    'src/game_state.hpp', 'src/game_state.cpp',
    'src/game_audio.hpp', 'src/game_audio.cpp',
    'src/host_clock.hpp', 'src/host_clock.cpp',
    'include/35mmisle/game_input.hpp', 'src/game_input.cpp',
    'src/menu_resources.hpp', 'src/menu_resources.cpp',
)
RECIPE_KEYS = ('definitions', 'semantic_origins', 'constant_names',
               'resource_identities', 'state_origins')
PREAMBLE = '''/* Complete pinned own-GAME application manager/helpers; no SDK implementation. */
#include <stdbool.h>

'''


def digest(data):
    return hashlib.sha256(data).hexdigest()


def compact(value):
    return json.dumps(value, sort_keys=True, separators=(',', ':')).encode()


def rewrite(original, unit):
    native = original
    for change in unit['changes']:
        expected = change['occurrences']
        if expected < 1 or len(re.findall(change['find'], native)) != expected:
            raise ValueError('Application counted rewrite mismatch: ' + unit['symbol'])
        native, count = re.subn(change['find'], lambda match: change['replace'], native)
        if count != expected:
            raise ValueError('Application replacement count changed: ' + unit['symbol'])
    if digest(native.encode()) != unit['native_sha256']:
        raise ValueError('Application transformed complete body mismatch: ' + unit['symbol'])
    if native[:native.index('{')].strip() != unit['native_declaration']:
        raise ValueError('Application C declaration mismatch: ' + unit['symbol'])
    return native


def prepare(repository, manifest_path, output, evidence):
    raw_manifest = manifest_path.read_bytes()
    manifest = json.loads(raw_manifest)
    root = Path(__file__).resolve().parent.parent
    if (manifest['schema_version'] != 1 or manifest['revision'] != REVISION or
            manifest['source_path'] != SOURCE or manifest['transformation_version'] != VERSION):
        raise ValueError('Unadmitted application authority/version')
    output, evidence = output.resolve(), evidence.resolve()
    parent = output.parent
    protected = (repository.resolve(), *(root / name for name in
                 ('src', 'include', 'tests', 'tools', 'cmake', '.git', 'graft')))
    if (output.name != 'application_original.inc' or evidence.name != 'evidence.json' or
            output == evidence or parent != evidence.parent or
            parent.name != 'isle-application-private' or parent.parent.name != '_deps' or
            any(parent == base or base in parent.parents for base in protected)):
        raise ValueError('Application outputs require exact private build paths outside source trees')
    if manifest['generator_sha256'] != digest(Path(__file__).read_bytes()):
        raise ValueError('Application generator identity mismatch')
    if digest(compact(manifest['source_files'])) != SOURCE_INVENTORY_SHA256:
        raise ValueError('Application pinned source inventory mismatch')
    recipe = {key: manifest[key] for key in RECIPE_KEYS}
    if digest(compact(recipe)) != RECIPE_SHA256:
        raise ValueError('Application complete counted recipe mismatch')
    if set(manifest['native_support']) != set(SUPPORT):
        raise ValueError('Application native support allowlist mismatch')
    if git(repository, 'cat-file', '-t', REVISION).strip() != b'commit':
        raise ValueError('Application authority is not the pinned commit')
    sources = {}
    for path, identity in manifest['source_files'].items():
        data = git(repository, 'show', REVISION + ':' + path)
        blob = hashlib.sha1(b'blob ' + str(len(data)).encode() + b'\0' + data).hexdigest()
        if (blob != identity['git_blob'] or digest(data) != identity['sha256'] or
                len(data.splitlines()) != identity['line_count']):
            raise ValueError('Application pinned Git-object identity mismatch: ' + path)
        sources[path] = data.decode().splitlines(keepends=True)
    for path, identity in manifest['native_support'].items():
        selected = (root / path).resolve()
        if root not in selected.parents or digest(selected.read_bytes()) != identity['sha256']:
            raise ValueError('Application native support identity mismatch: ' + path)
    units = manifest['definitions']
    selection = [[u['path'], u['symbol'], u['first'], u['last']] for u in units]
    selection_digest = digest(json.dumps(selection, separators=(',', ':')).encode())
    counts = dict(total=len(units), manager_file=sum(u['path'] == SOURCE for u in units),
                  mixer_helper=sum(u['path'] == 'src/app_render/568D0.c' for u in units))
    if (counts != dict(total=10, manager_file=9, mixer_helper=1) or
            len({(u['path'], u['symbol']) for u in units}) != 10 or
            any(u['owner'] != 'application_source' for u in units) or
            selection_digest != SELECTION_SHA256 or
            sum(u['last'] - u['first'] + 1 for u in units) != 343 or
            sum(c['occurrences'] for u in units for c in u['changes']) != 236):
        raise ValueError('Application requires complete unique ten-definition ownership')
    for origin in manifest['semantic_origins']:
        original = ''.join(sources[origin['path']][origin['first'] - 1:origin['last']])
        if digest(original.encode()) != origin['source_sha256']:
            raise ValueError('Application semantic origin identity mismatch: ' + origin['label'])
    constants = {}
    names = set(manifest['constant_names'])
    for path in ('include/enums.h', 'include/sys/audio.h'):
        for block in re.findall(r'enum\s+\w+\s*\{(.*?)\};', ''.join(sources[path]), re.S):
            value = -1
            for part in re.sub(r'//[^\n]*', '', block).split(','):
                match = re.fullmatch(r'\s*(\w+)(?:\s*=\s*(0x[\dA-Fa-f]+|\d+))?\s*', part)
                if match:
                    value = int(match[2], 0) if match[2] else value + 1
                    if match[1] in names:
                        constants[match[1]] = value
    if set(constants) != names or len(constants) != 28:
        raise ValueError('Application original scene/flag/audio constant coverage mismatch')
    generated = [PREAMBLE, 'enum {\n',
                 ''.join('    %s = %d,\n' % item for item in sorted(constants.items())), '};\n\n']
    for unit in units:
        original = ''.join(sources[unit['path']][unit['first'] - 1:unit['last']])
        if digest(original.encode()) != unit['source_sha256']:
            raise ValueError('Application complete Source body mismatch: ' + unit['symbol'])
        native = rewrite(original, unit)
        generated.extend(['#line %d "snap@%s/%s"\n' %
                          (unit['first'], REVISION, unit['path']), native, '\n'])
    generated = ''.join(generated)
    if digest(generated.encode()) != OUTPUT_SHA256 or manifest['output_sha256'] != OUTPUT_SHA256:
        raise ValueError('Application complete private output identity mismatch')
    receipt = dict(revision=REVISION,
        scope='COMPLETE10_SOURCE_OWNERSHIP_NOT_NATIVE_APPLICATION_RUNTIME_QUALIFICATION',
        source_files=manifest['source_files'], source_inventory_sha256=SOURCE_INVENTORY_SHA256,
        manifest_sha256=digest(raw_manifest), generator_sha256=digest(Path(__file__).read_bytes()),
        recipe_sha256=RECIPE_SHA256, selection_sha256=selection_digest,
        ownership_counts=counts, original_body_line_count=343, counted_rewrite_occurrences=236,
        native_support=manifest['native_support'], definitions=units,
        semantic_origins=manifest['semantic_origins'], state_origins=manifest['state_origins'],
        resource_identities=manifest['resource_identities'], constants=constants,
        output_sha256=OUTPUT_SHA256,
        native_runtime_qualification='Not exercised; native leaves are declaration-only interfaces.')
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
