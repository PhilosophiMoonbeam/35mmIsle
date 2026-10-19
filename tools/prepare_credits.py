#!/usr/bin/env python3
"""Admit exact Original Credits into private native build storage only.

All 54 overlay definitions, five credit lists and the actual randTRange body
are selected uniquely from immutable Git objects. No SDK, worktree, ROM or
capture is a source authority. A recipe mismatch fails before publication.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

REVISION = 'df8815e75f3d79618466e01e0827c6165b93d11c'


def sha(data):
    return hashlib.sha256(data).hexdigest()


def definition(text, name):
    pattern = r'(?m)^([\w* ]+?)\s+' + re.escape(name) + r'\(([^\n]*)\)\s*\{'
    matches = list(re.finditer(pattern, text))
    if len(matches) != 1:
        raise ValueError(('definition selector cardinality', name, len(matches)))
    match = matches[0]
    end = match.end()
    depth, state, escaped = 1, 'code', False
    while depth:
        if end >= len(text):
            raise ValueError(('unterminated definition', name))
        ch = text[end]
        following = text[end:end + 2]
        end += 1
        if state in ('string', 'char'):
            if escaped:
                escaped = False
            elif ch == '\\':
                escaped = True
            elif ch == ('"' if state == 'string' else "'"):
                state = 'code'
        elif state == 'line':
            if ch == '\n':
                state = 'code'
        elif state == 'block':
            if following == '*/':
                state = 'code'
                end += 1
        elif following == '//':
            state = 'line'
            end += 1
        elif following == '/*':
            state = 'block'
            end += 1
        elif ch == '"':
            state = 'string'
        elif ch == "'":
            state = 'char'
        elif ch == '{':
            depth += 1
        elif ch == '}':
            depth -= 1
    return text[match.start():end], text.count('\n', 0, match.start()) + 1


def rewrite(text, item):
    for change in item['rewrites']:
        if change['kind'] == 'literal':
            count = text.count(change['old'])
            if count != change['count']:
                raise ValueError(('literal rewrite count', item['name'], change, count))
            text = text.replace(change['old'], change['new'])
        elif change['kind'] == 'regex':
            text, count = re.subn(change['old'], change['new'], text)
            if count != change['count']:
                raise ValueError(('regex rewrite count', item['name'], change, count))
        elif change['kind'] == 'span':
            first, last = change['first'], change['last']
            if first < 0 or last < first or last > len(text) or sha(text[first:last].encode()) != change['sha256']:
                raise ValueError(('positional rewrite fingerprint', item['name']))
            text = text[:first] + change['new'] + text[last:]
        elif change['kind'] == 'source_strings':
            pattern = r'"(?:\\.|[^"\\])*"'
            if len(re.findall(pattern, text)) != change['count']:
                raise ValueError(('Source string count', item['name']))
            def encode(match):
                return ''.join(ch if ord(ch) < 128 else ''.join('\\%03o' % b for b in ch.encode('euc_jp')) for ch in match.group())
            text = re.sub(pattern, encode, text)
        else:
            raise ValueError(('unknown rewrite kind', change['kind']))
    if sha(text.encode()) != item['native_sha256']:
        raise ValueError(('native definition fingerprint', item['name']))
    return text


def checked_read(repository, record):
    result = subprocess.run(['git', '-C', str(repository), 'show',
                             REVISION + ':' + record['path']],
                            check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    raw = result.stdout
    blob = hashlib.sha1(b'blob ' + str(len(raw)).encode() + b'\0' + raw).hexdigest()
    if blob != record['git_blob'] or sha(raw) != record['sha256'] or len(raw) != record['bytes']:
        raise ValueError(('immutable input fingerprint', record['path']))
    return raw.decode('utf-8')


def publish(path, data):
    # One exact output, no stale partial evidence after a rejected recipe.
    path.parent.mkdir(parents=True, exist_ok=True)
    staging = path.with_name(path.name + '.new')
    staging.write_bytes(data)
    staging.replace(path)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-repo', type=Path, required=True)
    parser.add_argument('--build-dir', type=Path)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--manifest', type=Path, default=Path(__file__).resolve().parents[1] / 'cmake/credits-inputs.json')
    parser.add_argument('--manifest-sha256', required=True)
    parser.add_argument('--native-root', type=Path, default=Path(__file__).resolve().parents[1])
    args = parser.parse_args()
    if (args.build_dir is None) == (args.output is None):
        parser.error('specify exactly one of --build-dir or --output')
    destination = args.output if args.output is not None else args.build_dir / '_deps/isle-credits-private'
    manifest_raw = args.manifest.read_bytes()
    if not re.fullmatch(r'[0-9a-f]{64}', args.manifest_sha256) or sha(manifest_raw) != args.manifest_sha256:
        raise ValueError('Credits externally admitted recipe fingerprint')
    recipe = json.loads(manifest_raw)
    if recipe['schema'] != 1 or recipe['commit'] != REVISION or recipe['overlay_body_count'] != 54:
        raise ValueError('unadmitted Credits recipe')
    for path, expected in recipe['native_support'].items():
        if sha((args.native_root / path).read_bytes()) != expected['sha256']:
            raise ValueError(('native support fingerprint', path))
    sources = {record['path']: checked_read(args.source_repo, record) for record in recipe['inputs']}
    found = []
    for path in recipe['overlay_c_files']:
        found.extend((path, match.group(1)) for match in re.finditer(
            r'(?m)^[\w* ]+?\s+(func_credits_\w+)\([^\n]*\)\s*\{', sources[path]))
    selected = [(item['path'], item['name']) for item in recipe['definitions'] if item['path'] in recipe['overlay_c_files']]
    if len(found) != 54 or len(set(found)) != 54 or selected != found:
        raise ValueError('Credits exact complete selector closure')
    dependencies = [(item['path'], item['name']) for item in recipe['definitions'] if item['path'] not in recipe['overlay_c_files']]
    if dependencies != [('src/sys/misc.c', 'randTRange')] or len(recipe['data']) != 6:
        raise ValueError('Credits exact RNG and five-list/table closure')
    bodies, prototypes, evidence = [], [], []
    for item in recipe['definitions']:
        original, first = definition(sources[item['path']], item['name'])
        if first != item['first'] or sha(original.encode()) != item['sha256']:
            raise ValueError(('Original definition fingerprint', item['name']))
        native = rewrite(original, item)
        bodies.append(native)
        prototypes.append(native[:native.index('{')].rstrip() + ';')
        evidence.append({key: item[key] for key in ('path', 'name', 'first', 'sha256', 'native_sha256')})
    data = []
    for item in recipe['data']:
        text = sources[item['path']]
        matches = list(re.finditer(item['selector'], text, re.S | re.M))
        if len(matches) != 1 or sha(matches[0].group().encode()) != item['sha256']:
            raise ValueError(('Original initialized data selector', item['name']))
        data.append(rewrite(matches[0].group(), item))
    generated = ('/* Private pinned Credits Source; never publish. */\n' +
                 '\n'.join(prototypes) + '\n\n' + '\n\n'.join(data) + '\n\n' +
                 '\n\n'.join(bodies) + '\n').encode()
    game = ('/* Private Credits compilation unit. */\n#include "credits_source.c"\n').encode()
    if sha(generated) != recipe['output_sha256'] or sha(game) != recipe['game_sha256']:
        raise ValueError('Credits whole-output fingerprint')
    report = {'schema': 1, 'authority': recipe['repository'] + '@' + REVISION,
              'overlay_body_count': 54, 'dependency_body_count': 1,
              'credit_lists': 5, 'definitions': evidence,
              'output_sha256': sha(generated), 'game_sha256': sha(game),
              'recipe_sha256': sha(manifest_raw), 'generator_sha256': sha(Path(__file__).read_bytes()),
              'native_support': recipe['native_support']}
    publish(destination / 'credits_source.inc', generated)
    publish(destination / 'game.c', game)
    publish(destination / 'evidence.json', (json.dumps(report, indent=2) + '\n').encode())


if __name__ == '__main__':
    main()
