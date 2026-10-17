#!/usr/bin/env python3
"""Admit only pinned own GAME numeric C; never consume SDK/ROM/checkout code."""
import argparse
import hashlib
import json
from pathlib import Path
from prepare_engine import git, publish

REVISION = 'df8815e75f3d79618466e01e0827c6165b93d11c'
VERSION = 'cart-numeric-reference-v1'
SOURCES = ('src/sys/anim.c', 'src/1F960.c')
SYMBOLS = ('func_8001ED60', 'func_8001F030', 'func_8001F2C4',
           'func_8001F4E4', 'get_speed_approx', 'interp_integrate', 'get_uniform_param')
SUPPORT = ('tests/cart_animation_reference.hpp', 'tests/cart_animation_reference.cpp',
           'tests/cart_animation_reference_numeric.h')
PREAMBLE = '''/* Private generated own-GAME numeric reference; no SDK ABI. */
#include "cart_animation_reference_numeric.h"
#include <math.h>
#include <stdbool.h>
typedef float f32;
typedef int s32;
typedef short s16;
typedef IsleReferencePoint Vec3f;
typedef IsleReferenceSpeed FittingParams;
typedef IsleReferencePath InterpData;
#define SQ(x) ((x) * (x))
#define ABS(x) ((x) < 0 ? -(x) : (x))
enum { HAL_INTERP_LINEAR=0, HAL_INTERP_BEZIER_S3=1,
       HAL_INTERP_BEZIER=2, HAL_INTERP_CATROM=3 };
'''


def digest(data):
    return hashlib.sha256(data).hexdigest()


def definition(text, name):
    lines = text.splitlines(keepends=True)
    starts = [i for i, line in enumerate(lines)
              if line.startswith(('f32 ' + name + '(', 'void ' + name + '('))]
    if len(starts) != 1:
        raise ValueError('numeric definition inventory: ' + name)
    first = starts[0]
    for last in range(first + 1, len(lines)):
        if lines[last] == '}\n':
            return ''.join(lines[first:last+1]), first+1, last+1
    raise ValueError('unterminated numeric definition: ' + name)


def select(sources):
    records = []
    camera, first, last = definition(sources[SOURCES[0]], 'animGetAObjInterpValue')
    records.append((SOURCES[0], 'animGetAObjInterpValue', first, last, camera))
    model = ''.join(sources[SOURCES[0]].splitlines(keepends=True)[599:610])
    records.append((SOURCES[0], 'model_cubic_expression', 600, 610, model))
    for symbol in SYMBOLS:
        body, first, last = definition(sources[SOURCES[1]], symbol)
        records.append((SOURCES[1], symbol, first, last, body))
    return records


def adapt(records):
    changes = []
    bodies = []
    for path, symbol, first, last, original in records:
        body = original
        def replace(old, new, count):
            nonlocal body
            if body.count(old) != count:
                raise ValueError('numeric bounded adaptation mismatch: ' + symbol + ': ' + old)
            body = body.replace(old, new)
            changes.append(dict(symbol=symbol, find=old, replace=new, count=count))
        if symbol == 'animGetAObjInterpValue':
            replace(symbol, 'isle_reference_camera', 1)
        elif symbol == 'model_cubic_expression':
            fields = {'invDuration': 'invDuration', 'time': 'time',
                      'initialValue': 'initialValue', 'targetValue': 'targetValue',
                      'rate': 'initialRate', 'targetRate': 'targetRate'}
            for field, scalar in fields.items():
                token = 'aobj->' + field
                replace(token, scalar, original.count(token))
            replace('value =', 'return', 1)
            body = ('f32 isle_reference_model(f32 invDuration, f32 time, f32 initialValue, '
                    'f32 targetValue, f32 initialRate, f32 targetRate) {\n' + body + '}\n')
        elif symbol == 'func_8001F4E4':
            replace(symbol, 'isle_reference_position', 1)
        elif symbol == 'get_uniform_param':
            replace(symbol, 'isle_reference_map', 1)
        else:
            declaration = 'f32 ' if body.startswith('f32 ') else 'void '
            replace(declaration + symbol + '(', 'static ' + declaration + symbol + '(', 1)
        bodies.append(body)
    return PREAMBLE + '\n'.join(bodies), changes


def prepare(repository, manifest_path, output, evidence):
    manifest = json.loads(manifest_path.read_bytes())
    if manifest['revision'] != REVISION or manifest['transformation_version'] != VERSION:
        raise ValueError('numeric reference authority mismatch')
    if set(manifest['source_files']) != set(SOURCES):
        raise ValueError('numeric source allowlist mismatch')
    if git(repository, 'cat-file', '-t', REVISION).strip() != b'commit':
        raise ValueError('numeric reference requires committed authority')
    sources = {}
    for path in SOURCES:
        data = git(repository, 'show', REVISION + ':' + path)
        blob = hashlib.sha1(b'blob ' + str(len(data)).encode() + b'\0' + data).hexdigest()
        if manifest['source_files'][path] != dict(git_blob=blob, sha256=digest(data)):
            raise ValueError('numeric source identity mismatch: ' + path)
        sources[path] = data.decode()
    records = select(sources)
    inventory = [dict(path=p, symbol=s, first=f, last=l, sha256=digest(b.encode()))
                 for p, s, f, l, b in records]
    generated, changes = adapt(records)
    root = Path(__file__).resolve().parent.parent
    if manifest['definitions'] != inventory or manifest['adaptations'] != changes:
        raise ValueError('numeric source/body/adaptation inventory mismatch')
    if manifest['generated_sha256'] != digest(generated.encode()):
        raise ValueError('numeric generated source identity mismatch')
    if manifest['generator_sha256'] != digest(Path(__file__).read_bytes()):
        raise ValueError('numeric generator identity mismatch')
    if set(manifest['native_support']) != set(SUPPORT):
        raise ValueError('numeric support allowlist mismatch')
    for path in SUPPORT:
        if manifest['native_support'][path] != dict(sha256=digest((root / path).read_bytes())):
            raise ValueError('numeric support identity mismatch: ' + path)
    publish(output, generated)
    publish(evidence, json.dumps(dict(schema_version=1, revision=REVISION,
        manifest_sha256=digest(manifest_path.read_bytes()),
        generated_sha256=digest(generated.encode()), definitions=inventory,
        adaptations=changes), indent=2) + '\n')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repository', type=Path, required=True)
    parser.add_argument('--manifest', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--evidence', type=Path, required=True)
    args = parser.parse_args()
    prepare(args.repository, args.manifest, args.output, args.evidence)
