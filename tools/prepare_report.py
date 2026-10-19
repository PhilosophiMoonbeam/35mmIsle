#!/usr/bin/env python3
"""Materialize pinned Report14 into private storage; public files contain no GAME bodies."""
import argparse
import ast
import hashlib
import json
import re
from pathlib import Path
from prepare_engine import git, publish

REVISION = 'df8815e75f3d79618466e01e0827c6165b93d11c'
PRIMARY = {'src/pokemon_report/9D3230.c', 'src/pokemon_report/9D3660.c', 'src/pokemon_report/9D91C0.c'}
TOKENS = re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|//[^\n]*|/\*.*?\*/', re.S)
EXPRESSION = r'[A-Za-z_]\w*(?:(?:\[[^]\n]+\])|(?:\.\w+))*'

def sha(value):
    return hashlib.sha256(value if isinstance(value, bytes) else value.encode()).hexdigest()

def code_only(text, transform):
    parts = []; at = 0
    for token in TOKENS.finditer(text):
        parts.extend((transform(text[at:token.start()]), token[0])); at = token.end()
    parts.append(transform(text[at:]))
    return ''.join(parts)

def encode(text):
    def literal(m):
        token = m[0]
        if token.startswith('"'):
            value = ast.literal_eval(token).encode('euc_jp')
            return '"' + ''.join('\\x%02x' % b for b in value) + '"'
        if token.startswith("'"):
            value = ast.literal_eval(token).encode('euc_jp')
            if len(value) > 1:
                return str(int.from_bytes(value, 'big'))
        return token
    return TOKENS.sub(literal, text)

def rewrite(original, name, recipe, names):
    records = []
    def sub(pattern, replacement, text, label):
        result, count = re.subn(pattern, replacement, text)
        if count:
            records.append({'operation': label, 'count': count})
        return result
    def exact(old, new, count=1):
        nonlocal original
        actual = original.count(old)
        if actual != count:
            raise ValueError(('counted source replacement', name, old, actual, count))
        original = original.replace(old, new)
        records.append({'operation': 'exact', 'find': old, 'replace': new, 'count': count})
    if name == recipe['entry']:
        return None, None, [{'operation': 'independent C entry', 'implementation': 'isle_report_source_entry', 'notice_after_setup_return': 0x200000}]
    if name == 'func_801DC8C0_9D3230':
        original = 'void func_801DC8C0_9D3230(IsleReportGraphics** arg0) {\n    rp_check(source, source->native->render_bind(source, arg0));\n}\n'
        records.append({'operation': 'native actual display-list append', 'origin': '9D3230.c76-78'})
    if name == 'func_801DC904_9D3274':
        original = 'void func_801DC904_9D3274(int32_t arg0, int32_t arg1) {\n    rp_check(source, source->native->render_state(source, arg0, arg1));\n}\n'
        records.append({'operation': 'native actual render state', 'origin': '9D3230.c80-93'})
    if name == 'func_801DCA48_9D33B8':
        original = 'void func_801DCA48_9D33B8(void) {\n    rp_camera(source);\n}\n'
        records.append({'operation': 'returning native camera; exact C idle registration', 'origin': '9D3230.c103-125'})
    if name == 'func_801DCBF4_9D3564':
        exact('UIMem_CreateHeap(D_80203C10_9FA580, sizeof(D_80203C10_9FA580));', 'rp_check(source, source->native->ui_heap(source, 0x2D000));')
    if name == 'func_801DF4AC_9D5E1C':
        exact('u8* src;', 'const u8* src;')
        exact('dest = (u8*) UIMem_Allocate(0xFD8);', 'dest = rp_thumbnail(source, arg0, sobj);')
        exact('sobj->sprite.bitmap->buf = D_80230DC8_A27738[arg0].unk_08 = dest;', 'D_80230DC8_A27738[arg0].unk_08 = dest;')
        exact('    return gobj;\n}', '    rp_check(source, isle_report_native_thumbnail_publish(source, arg0));\n    return gobj;\n}')
        records.append({'operation': 'native owned bitmap backing; original exact 0xFD8 C copy loop retained'})
    if name == 'func_801DF884_9D61F4':
        exact('sobj->sprite = *D_80202DD0_9F9740[arg0];', 'rp_rebind(source, sobj, D_80202DD0_9F9740[arg0]);')
        exact('sobj->sprite.bitmap->buf = D_80202DE8_9F9758[arg0];', 'rp_bitmap(source, sobj, arg0);')
    if name == 'func_801E0C3C_9D75AC':
        exact('ARRAY_COUNT(D_80202E8C_9F97FC)', '6')
    def handles(code):
        code = sub(r'\bUNUSED\s+[^;]+;', '', code, 'matching-only stack padding')
        code = sub(r'('+EXPRESSION+r')->sprite\s*=\s*(D_[A-F0-9_]+);', lambda m: 'rp_rebind(source, '+m[1]+', UINT32_C(0x'+m[2].split('_')[1]+'));', code, 'prototype rebind')
        code = sub(r'&('+EXPRESSION+r')->sprite', r'\1', code, 'preview takes actual native child')
        code = sub(r'('+EXPRESSION+r')->data\.sobj->sprite', r'(*rp_sprite(source, rp_root(source, \1)))', code, 'actual root sprite view')
        code = sub(r'('+EXPRESSION+r')->data\.sobj', r'rp_root(source, \1)', code, 'actual root handle')
        code = sub(r'('+EXPRESSION+r')->sprite', r'(*rp_sprite(source, \1))', code, 'actual mutable sprite view')
        code = sub(r'('+EXPRESSION+r')->unk_58\s*=\s*NULL;', r'rp_detach(source, \1, 0);', code, 'actual child detach')
        code = sub(r'('+EXPRESSION+r')->unk_5C\s*=\s*NULL;', r'rp_detach(source, \1, 1);', code, 'actual parent detach')
        code = sub(r'('+EXPRESSION+r')->unk_58', r'rp_link(source, \1, 1)', code, 'actual first child')
        code = sub(r'('+EXPRESSION+r')->next', r'rp_link(source, \1, 0)', code, 'actual sibling')
        code = sub(r'D_80230E14_A27784\[([^]\n]+)\]\.id', r'rp_slot(source, \1)', code, 'canonical live Layout selector')
        for asset, address in recipe['assets'].items():
            code = sub(r'&'+asset+r'\b', 'UINT32_C(0x%08x)' % address, code, 'prototype '+asset)
        return code
    original = code_only(original, handles)
    head, inner = original.split('{', 1)
    ret, args = head.split(name+'(', 1)
    args = args.rsplit(')', 1)[0].strip()
    public = name in recipe['callbacks']
    signature = ('' if public else 'static ') + ret.strip() + ' isle_report_' + name + '('
    signature += args if public else 'Context* source' + (', '+args if args not in ('', 'void') else '')
    signature += ')'
    if public:
        argument = args.split()[-1].lstrip('*')
        inner = '\n    Context* source=(Context*)isle_post_course_source_require('+argument+',14);' + inner
    substitutions = {**recipe['bridge'], **recipe['constants'], **recipe['read_only'],
                     **{field:'source->'+field for field in recipe['context_fields']},
                     **{n:'isle_report_'+n for n in names},
                     's32':'int32_t','u32':'uint32_t','s16':'int16_t','u16':'uint16_t',
                     's8':'int8_t','u8':'uint8_t','f32':'float'}
    substitutions['func_803750CC_84887C'] = 'isle_original_source_name_compare'
    token = re.compile(r'\b('+'|'.join(map(re.escape, sorted(substitutions,key=len,reverse=True)))+r')\b')
    def tokens(code):
        code = token.sub(lambda m: substitutions[m[0]], code)
        for function in list(recipe['bridge'].values())+['isle_report_'+n for n in names if n not in recipe['callbacks'] and n != recipe['entry']]:
            code = sub(r'\b'+function+r'\(\s*(?!Context\b)', function+'(source, ', code, 'explicit C context '+function)
            code = code.replace(function+'(source, )', function+'(source)')
        code = code.replace('source->D_80202D2C_9F969C(', 'source->D_80202D2C_9F969C(source, ')
        code = code.replace('char**', 'const char* const*').replace('UIButton*', 'const UIButton*')
        return code
    signature = code_only(signature, tokens).replace('(source, Context* source', '(Context* source')
    result = encode(signature+' {'+code_only(inner,tokens))
    # No SDK layouts/heap emulation/host sort may escape the checked rewrite.
    forbidden = r'\b(?:Gfx|OMCamera|Bitmap|Sprite|UIMem_Allocate|UIMem_CreateHeap|qsort2|ohCreateCamera|omSetupScene|gMainGfxPos)\b|->(?:sprite|data\.sobj|unk_58|unk_5C|next)\b'
    if re.search(forbidden, code_only(result, lambda s:s)):
        raise ValueError(('untranslated SDK operation', name))
    return result, encode(signature)+';\n', records

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--source-repo', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--manifest', type=Path, default=Path(__file__).resolve().parents[1]/'cmake/report-inputs.json')
    parser.add_argument('--manifest-sha256', required=True)
    args = parser.parse_args()
    recipe_raw = args.manifest.read_bytes(); recipe = json.loads(recipe_raw)
    if not re.fullmatch(r'[0-9a-f]{64}', args.manifest_sha256) or sha(recipe_raw) != args.manifest_sha256:
        raise ValueError('Report externally admitted recipe fingerprint')
    if recipe['revision'] != REVISION or recipe['primary_body_count'] != 73:
        raise ValueError('unadmitted Report pin/count')
    root = Path(__file__).resolve().parents[1]
    for path, identity in recipe['native_support'].items():
        if sha((root/path).read_bytes()) != identity['sha256']:
            raise ValueError(('native support guard', path))
    if not recipe['native_support']:
        raise ValueError('missing native support guards')
    texts = {}
    for path, identity in recipe['source_files'].items():
        raw = git(args.source_repo, 'show', REVISION+':'+path)
        blob = hashlib.sha1(b'blob '+str(len(raw)).encode()+b'\0'+raw).hexdigest()
        if sha(raw) != identity['sha256'] or blob != identity['blob']:
            raise ValueError(('immutable Source identity', path))
        texts[path] = raw.decode()
    selected = recipe['definitions']; names = [d['name'] for d in selected]
    if len(names) != len(set(names)) or sum(d['source'] in PRIMARY for d in selected) != 73:
        raise ValueError('nonunique/incomplete Report selectors')
    signature = r'^(?:void|s32|GObj\*|UIElement\*|UIButton\*|void\*) +(func_\w+)\([^\n]*\) \{'
    actual = {(path,name) for path in PRIMARY for name in re.findall(signature,texts[path],re.M)}
    admitted = {(d['source'],d['name']) for d in selected if d['source'] in PRIMARY}
    if len(actual)!=73 or actual!=admitted:
        raise ValueError('Original primary inventory/selector closure mismatch')
    definitions=[]; declarations=[]; evidence=[]
    for unit in selected:
        if unit['source'] not in PRIMARY and unit['name'] not in recipe['support_allowlist'].get(unit['source'], []):
            raise ValueError(('unadmitted support body', unit['name']))
        original=''.join(texts[unit['source']].splitlines(keepends=True)[unit['first']-1:unit['last']])
        if sha(original) != unit['sha256']:
            raise ValueError(('positional Source guard', unit['name']))
        native, declaration, rewrites = rewrite(original, unit['name'], recipe, names)
        if native is not None:
            definitions.append(native); declarations.append(declaration)
        evidence.append({**unit,'native_sha256':sha(native) if native else None,'rewrites':rewrites})
    strings = '\n'.join(re.findall(r'^const char otherText\d*\[\] = .*?;', texts['src/pokemon_report/9D3660.c'], re.M))+'\n'
    if len(re.findall(r'^const char otherText', strings, re.M)) != 23:
        raise ValueError('caption literal declaration extent')
    body = encode(strings)+'\n'+'\n'.join(definitions)
    declarations = recipe['window_enum_declarations']+'\n'+''.join(declarations)
    native_unit = (root/'src/report_source.c').read_text()
    outputs = {'game.c':sha(native_unit),'report_original.inc':sha(body),'report_declarations.inc':sha(declarations)}
    if evidence != recipe['native_definitions']:
        raise ValueError('Report native definition/rewrite fingerprint')
    if outputs != recipe['native_outputs']:
        raise ValueError('Report whole-output fingerprint')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    publish(args.output, native_unit)
    publish(args.output.with_name('report_original.inc'), body)
    publish(args.output.with_name('report_declarations.inc'), declarations)
    publish(args.output.with_name('evidence.json'), json.dumps({
        'provider':recipe['provider'],'revision':REVISION,'recipe_sha256':sha(recipe_raw),
        'generator_sha256':sha(Path(__file__).read_bytes()),'source_files':recipe['source_files'],
        'native_support':recipe['native_support'],'definitions':evidence,
        'outputs':outputs,
        'caption_literal_count':23,'primary_body_count':73,
        'shared_dependencies':['isle_original_source_qsort2','isle_original_source_name_compare']},indent=2)+'\n')

if __name__ == '__main__':
    main()
