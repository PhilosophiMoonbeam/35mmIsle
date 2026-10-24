#!/usr/bin/env python3
"""Admit pinned CameraCheck Source into private SDK-free generated includes.

No ROM or GAME bodies are vendored. The manifest freezes every selected body,
source file, counted semantic correction, and support input. This program
reads exact pinned Git objects; no worktree HEAD or SDK input is authoritative.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

REVISION = 'df8815e75f3d79618466e01e0827c6165b93d11c'


def digest(data):
    return hashlib.sha256(data).hexdigest()


def definition(text, name):
    match = re.search(r'^([\w* ]+?)\s+' + re.escape(name) + r'\(([^\n]*)\)\s*\{', text, re.M)
    if not match:
        raise ValueError('missing definition: ' + name)
    end = match.end()
    depth, quoted, escaped = 1, False, False
    while depth:
        ch = text[end]
        end += 1
        if quoted:
            if escaped:
                escaped = False
            elif ch == '\\':
                escaped = True
            elif ch == '"':
                quoted = False
        elif ch == '"':
            quoted = True
        elif ch == '{':
            depth += 1
        elif ch == '}':
            depth -= 1
    return text[match.start():end], match.group(1).strip(), match.group(2)


def strings(text):
    # Original strings are Source EUC-JP bytes. Escapes/embedded NUL survive.
    def convert(match):
        out = []
        for ch in match.group():
            if ord(ch) < 128:
                out.append(ch)
            else:
                out.extend('\\%03o' % byte for byte in ch.encode('euc_jp'))
        return ''.join(out)
    return re.sub(r'"(?:\\.|[^"\\])*"', convert, text)


def replace(text, old, new, count):
    actual = text.count(old)
    if actual != count:
        raise ValueError(f'counted rewrite {old!r}: {actual} != {count}')
    return text.replace(old, new)


def member_access(text, member, function):
    # Locate a complete postfix operand backwards (including calls, indices
    # and -> chains), then replace only the admitted native attachment access.
    needle = '->' + member
    while needle in text:
        end = text.index(needle)
        start = end - 1
        depth = 0
        while start >= 0:
            ch = text[start]
            if ch in ')]':
                depth += 1
            elif ch in '([':
                if depth == 0:
                    break
                depth -= 1
            elif depth == 0 and not (ch.isalnum() or ch in '_.->'):
                break
            start -= 1
        start += 1
        operand = text[start:end]
        if not operand:
            raise ValueError('missing native operand: ' + member)
        text = text[:start] + function + '(ctx, ' + operand + ')' + text[end + len(needle):]
    return text


def translate(body, item, manifest):
    for patch in item.get('rewrites', []):
        body = replace(body, patch['old'], patch['new'], patch['count'])
    # Mutable guest projections are replaced by live, checked native views.
    body = member_access(body, 'data.sobj', 'cc_root')
    body = member_access(body, 'sprite', 'cc_sprite_deref')
    body = body.replace('cc_sprite_deref(', '(*cc_sprite(')
    # cc_sprite_deref(a,b) became (*cc_sprite(a,b)); close via balanced scan.
    cursor = 0
    while True:
        p = body.find('(*cc_sprite(', cursor)
        if p < 0:
            break
        q, depth = p + len('(*cc_sprite('), 1
        while depth:
            if body[q] == '(':
                depth += 1
            elif body[q] == ')':
                depth -= 1
            q += 1
        body = body[:q] + ')' + body[q:]
        cursor = q + 1
    # Accessors take SObj identity, not a transient scalar Sprite pointer.
    body = re.sub(r'&\(\*cc_sprite\(ctx, (.*?)\)\)', r'\1', body)
    for name, target in manifest['services'].items():
        body = re.sub(r'\b' + re.escape(name) + r'\(\s*\)', target + '(ctx)', body)
        body = re.sub(r'\b' + re.escape(name) + r'\(', target + '(ctx, ', body)
    body = re.sub(r'\bcc_(\w+)\(ctx, ctx\)', r'cc_\1(ctx)', body)
    for name in manifest['source_functions']:
        target = name + '_body' if name in manifest['callbacks'] else name
        body = re.sub(r'\b' + re.escape(name) + r'\(\s*(?:void)?\s*\)', target + '(ctx)', body)
        body = re.sub(r'\b' + re.escape(name) + r'\(', target + '(ctx, ', body)
        body = body.replace('(ctx, ctx)', '(ctx)')
    body = body.replace('D_camera_check_802089F4(', 'D_camera_check_802089F4(ctx, ')
    body = body.replace('D_camera_check_80249AA0(', 'D_camera_check_80249AA0(ctx, ')
    body = body.replace('arg0(i);', 'arg0(ctx, i);')
    body = re.sub(r'\b(?:static )?(?:f32|s32) D_camera_check_802089(?:44|48|4C|50|E4|E8|EC) = [^;]+;', '', body)
    for name in manifest['globals']:
        body = re.sub(r'\b' + name + r'\b', 'ctx->' + name, body)
    for name, field in manifest['music_globals'].items():
        body = body.replace(name, 'ctx->music->' + field)
    body = body.replace('PhotoData*', 'const PhotoData*')
    body = body.replace('Unk803A6C18*', 'const Unk803A6C18*')
    body = body.replace('UnkStruct800BEDF8*', 'const UnkStruct800BEDF8*')
    body = body.replace('char*', 'const char*')
    body = body.replace('__cosf(', 'cosf(').replace('__sinf(', 'sinf(')
    # Source signed-bit shifts mean masks of flags, not host negative-shift UB.
    body = re.sub(r'(lhs|rhs)->(unk_18_0x\w+) <<', r'(uint32_t) \1->\2 <<', body)
    body = body.replace('(s32) arg0 == 1', '(uintptr_t) arg0 == 1')
    body = body.replace('void (*arg0)(s32)', 'void (*arg0)(IsleCameraCheckSource*, s32)')
    # The matched function signature alone gains explicit context. Calls above
    # were already threaded; declarations below are derived from this signature.
    p = body.index('(')
    close = body.index(')', p)
    args = body[p + 1:close]
    if args.startswith('ctx, '):
        args = args[5:]
    elif args == 'ctx':
        args = ''
    if args == 'void':
        args = ''
    body = body[:p + 1] + 'IsleCameraCheckSource* ctx' + (', ' + args if args else '') + body[close:]
    if 'native_body' in item:
        body = body[:body.index('{')] + item['native_body']
    if item['name'] != 'func_camera_check_801DCACC':
        body = 'static ' + body
    return strings(body)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-repo', required=True, type=Path)
    parser.add_argument('--manifest', type=Path, default=Path(__file__).resolve().parents[1] / 'cmake/camera-check-inputs.json')
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    manifest = json.loads(args.manifest.read_text())
    if manifest['commit'] != REVISION:
        raise ValueError('unadmitted GAME revision')
    root = Path(__file__).resolve().parents[1]
    if not manifest['native_support']:
        raise ValueError('missing CameraCheck native support guards')
    for path, identity in manifest['native_support'].items():
        if digest((root / path).read_bytes()) != identity['sha256']:
            raise ValueError('unadmitted native support: ' + path)
    source = {}
    for item in manifest['inputs']:
        raw = subprocess.run(['git', '-C', str(args.source_repo), 'show',
                              REVISION + ':' + item['path']],
                             check=True, capture_output=True).stdout
        blob = hashlib.sha1(b'blob ' + str(len(raw)).encode() + b'\0' + raw).hexdigest()
        if blob != item['git_blob']:
            raise ValueError('pinned Git blob mismatch: ' + item['path'])
        if digest(raw) != item['sha256']:
            raise ValueError('pinned input mismatch: ' + item['path'])
        source[item['path']] = raw.decode('utf-8')
    definitions = []
    for item in manifest['definitions']:
        body, _, _ = definition(source[item['path']], item['name'])
        if digest(body.encode()) != item['sha256']:
            raise ValueError('pinned definition mismatch: ' + item['name'])
        definitions.append(translate(body, item, manifest))
    # The complete native typed function prototypes follow transformed bodies,
    # including callbacks/comparators passed by address before their definition.
    declarations = []
    for body in definitions:
        declarations.append(body[:body.index('{')].rstrip() + ';')
    constants = manifest['constants']
    constants_text = '\n'.join('#define ' + name + ' ' + str(value) for name, value in constants.items()) + '\n'
    # Initialized button strings are imported from exact Source declarations.
    tables = []
    for name in manifest['button_tables']:
        text = source['src/camera_check/87D1A0.c']
        match = re.search(r'UIButton ' + name + r'\[\] = \{.*?\n\};', text, re.S)
        if not match:
            raise ValueError('missing initialized table: ' + name)
        tables.append(strings(match.group().replace('UIButton ', 'static const UIButton cc_initial_')))
    output = ('/* Private pinned CameraCheck translation; do not publish. */\n' + '\n'.join(declarations) + '\n' +
              '\n'.join(tables) + '\n' + '\n\n'.join(definitions) + '\n')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.with_name('camera_check_constants.inc').write_text(constants_text)
    args.output.write_text(output)
    report = {'commit': manifest['commit'], 'source_sha256': digest(output.encode()),
              'constants_sha256': digest(constants_text.encode()), 'definitions': len(definitions),
              'manifest_sha256': digest(args.manifest.read_bytes())}
    Path(str(args.output) + '.manifest.json').write_text(json.dumps(report, indent=2) + '\n')


if __name__ == '__main__':
    main()
