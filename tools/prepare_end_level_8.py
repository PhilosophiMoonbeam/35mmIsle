#!/usr/bin/env python3
"""Strict private EndLevel8 provider. Never reads a checkout's working bytes."""
import argparse
import ast
import hashlib
import json
import re
import subprocess
from pathlib import Path
REVISION='df8815e75f3d79618466e01e0827c6165b93d11c'
SOURCE='src/unk_end_level_8/A084B0.c'
BLOB='541be49786d466806c1221b2264276c1767a6249'
SHA256='c558ee02de8dc640db9fd3238550b531b7f2c25bd2edf5358f8622703ecf9e3e'
NAMES=['func_801DC8C0_A084B0','func_801DC9E8_A085D8','func_801DCE2C_A08A1C','func_801DCE64_A08A54','func_801DCFE0_A08BD0','func_801DD070_A08C60','func_801DD09C_A08C8C']
TOKENS=re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|//[^\n]*|/\*.*?\*/',re.S)
def sha(data): return hashlib.sha256(data).hexdigest()
def definitions(text):
    masked=TOKENS.sub(lambda m:' '*len(m[0]),text)
    result={}
    for m in re.finditer(r'(?m)^(?:void|s32)\s+(\w+)\([^;{}]*\)\s*\{',masked):
        depth=1; end=m.end()
        while depth:
            depth+=(masked[end]=='{')-(masked[end]=='}'); end+=1
        if m[1] in result: raise ValueError('duplicate definition')
        result[m[1]]=text[m.start():end]+'\n'
    if list(result)!=NAMES: raise ValueError('definition closure differs')
    return result

def rewrite(name,text):
    records=[]
    def change(old,new,count=1):
        nonlocal text
        if text.count(old)!=count: raise ValueError((name,old,text.count(old),count))
        text=text.replace(old,new); records.append({'old':old,'new':new,'count':count})
    if name==NAMES[0]:
        change('    s32 unk0[2];\n    UIFrame* sp2C;\n    UIElement* sp28;\n','')
        change('sp2C = UIFrame_Create();','e8_check(source,isle_post_window_frame_create(source->common.window,&source->frame));')
        change('    D_801DD254_A08E44 = sp2C;\n','')
        change('UIFrame_GetBackgroundColor(sp2C, &sp24);','e8_check(source,isle_post_window_frame_color(source->common.window,source->frame,&sp24));')
        change('sp28 = UIElement_Create(104, 173, 192, 47, 0);','e8_check(source,isle_post_window_element_create(source->common.window,104,173,192,47,0,&source->element));')
        change('    D_801DD258_A08E48 = sp28;\n','')
    if name==NAMES[1]:
        change('D_801DD240_A08E30->flags &= ~1;','e8_hidden(source,0);',2)
        change('D_801DD240_A08E30->flags |= 1;','e8_hidden(source,1);',2)
        change('initObjectsOnPhoto(D_801DD244_A08E34, func_800BF574_5C414(D_801DD24C_A08E3C));','e8_photo(source,source->D_801DD24C_A08E3C);',2)
        change('func_8009FBC4();','e8_replay_cleanup(source);')
        change('gtlWaitAllGfxTasksDone();','e8_join_graphics(source);')
    if name==NAMES[2]: change('func_800A85E8(func_801DC9E8_A085D8, LINK_6, DL_LINK_0, NULL);','e8_process(source,6,0,NULL);')
    if name==NAMES[3]:
        text=text[:text.index('{')]+'{\n    e8_camera(source);\n}\n'; records.append({'mechanical':'camera fields replaced by exact typed native camera contract'})
    if name==NAMES[4]:
        change('D_801DD130_A08D20.zBuffer = gtlMalloc(0x25800, 0x40);\n    viApplyScreenSettings(&D_801DD130_A08D20);','e8_check(source,source->requiredNativeTable->depth_screen(source,0x25800,0x40));')
        change('UIMem_CreateHeap(D_801DD260_A08E50, sizeof(D_801DD260_A08E50));','e8_check(source,source->requiredNativeTable->ui_heap(source,0x32000));')
    if name==NAMES[6]:
        return 'void isle_end_level_8_'+name+'(void* user) { isle_end_level_8_source_entry(user); }\n',[{'mechanical':'independent C entry in end_level_8_source.c preserves prelatch, setup and actual retirement result'}]
    native={'func_8009FB50':'replay_initialize','func_8009FBC4':'replay_cleanup','func_800A7860':'fade','func_800AAE28':'input_initialize','auPlaySound':'sound','auSetBGMVolume':'volume','func_800A7F68':'aux_camera','func_800AADF0':'scene_result','UIMem_Deallocate':'deallocate','gtlWaitAllGfxTasksDone':'join_graphics'}
    # Calls with no nested parentheses are the exact remaining native leaf calls.
    for old,field in native.items():
        pattern=r'\b'+old+r'\(([^()]*)\);'
        def leaf(m): return 'e8_check(source,source->requiredNativeTable->'+field+'(source'+(','+m[1] if m[1].strip() else '')+'));'
        text,n=re.subn(pattern,leaf,text)
        if n: records.append({'leaf':old,'count':n})
    direct={'UIFrame_SetStyle','UIFrame_SetPos','UIFrame_Show','UIFrame_ShowBackground','UIElement_SetState','UIElement_SetFont'}
    for old in sorted(direct):
        text,n=re.subn(r'\b'+old+r'\((?:sp2C|sp28|D_801DD254_A08E44|D_801DD258_A08E48),\s*', 'e8_'+old+'(source,',text)
        if n: records.append({'window':old,'count':n})
    text=re.sub(r'UIElement_SetColor\(sp28,\s*','e8_color(source,',text)
    text=text.replace('UIElement_DrawBackground(sp28);','e8_check(source,isle_post_window_element_background(source->common.window,source->element));')
    text=text.replace('UIElement_PrintText(\n        sp28,','e8_print(source,')
    text=text.replace('UIFrame_FadeIn(D_801DD254_A08E44, FRAME_STYLE_1);','e8_check(source,isle_post_window_fade_in(source->common.window,source->frame,FRAME_STYLE_1));')
    for old,field in {'UIText_Initialize':'initialize','UIText_SetShadowOffset':'shadow_offset','UIText_SetSpacing':'spacing','UIText_SetPrintDelay':'delay'}.items():
        text=re.sub(old+r'\(([^()]*)\);',lambda m:'e8_check(source,isle_post_window_text_'+field+'(source->common.window'+(','+m[1] if m[1] else '')+'));',text)
    text=text.replace('func_800A7460()','e8_fade_pending(source)').replace('func_800AA38C(0)','e8_input(source,0)').replace('ohWait(','e8_wait(source,')
    fields=['D_801DD24C_A08E3C','D_801DD250_A08E40','D_801DD25C_A08E4C','D_801DD12C_A08D1C']
    def code(part):
        for field in fields: part=re.sub(r'(?<!->)\b'+field+r'\b','source->'+field,part)
        for called in NAMES:
            part=re.sub(r'\b'+called+r'\(\)', 'isle_end_level_8_'+called+'(source)',part)
        return part
    chunks=[]; pos=0
    for token in TOKENS.finditer(text):
        chunks.append(code(text[pos:token.start()])); literal=token[0]
        if literal.startswith('"'): literal='"'+''.join('\\x%02x'%b for b in ast.literal_eval(literal).encode('euc_jp'))+'"'
        chunks.append(literal);pos=token.end()
    text=''.join(chunks)+code(text[pos:])
    if name==NAMES[1]:
        text=text.replace('void '+name+'(GObj* arg0) {','void isle_end_level_8_'+name+'(GObj* arg0) {\n    Context* source=(Context*)isle_post_course_source_require(arg0,24);')
    else:
        text=re.sub(r'void '+name+r'\((?:void|UNK_PTR arg0)\)', 'static void isle_end_level_8_'+name+'(Context* source'+(',void* arg0' if name==NAMES[5] else '')+')',text)
    return text,records

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--decomp',required=True);ap.add_argument('--output',required=True);ap.add_argument('--recipe',required=True);ap.add_argument('--manifest-sha256',required=True);ap.add_argument('--support-root',required=True);a=ap.parse_args()
    recipe_raw=Path(a.recipe).read_bytes()
    if not re.fullmatch(r'[0-9a-f]{64}',a.manifest_sha256) or sha(recipe_raw)!=a.manifest_sha256:
        raise ValueError('End8 externally admitted recipe fingerprint')
    recipe=json.loads(recipe_raw); root=Path(a.support_root)
    initial=recipe['initialized_snapshot']
    words=initial['words']
    raw_initial=b''.join(int(word,16).to_bytes(4,'big') for word in words)
    if (initial['rom'],initial['guest'],initial['byte_count'])!=('0xa08d10','0x801dd120',288):
        raise ValueError('initialized region selector')
    if len(raw_initial)!=288 or sha(raw_initial)!='35bd648e5e9ad6b015d1e000046aa9002f908674c730b71279ad663e2083f730':
        raise ValueError('canonical initialized bank guard')
    initial_include='static const uint32_t e8_initial_words[72]={\n'+',\n'.join('UINT32_C('+word+')' for word in words)+'\n};\n'
    initial_include+='static const IsleEndLevel8InitialData e8_initial_data={UINT32_C(0xa08d10),UINT32_C(0x801dd120),288,e8_initial_words,"35bd648e5e9ad6b015d1e000046aa9002f908674c730b71279ad663e2083f730"};\n'
    if sha(initial_include.encode())!=recipe['initialized_include_sha256']:
        raise ValueError('initialized native descriptor guard')
    for path,expected in recipe['support_sha256'].items():
        if sha((root/path).read_bytes())!=expected: raise ValueError('support guard '+path)
    def git(*args): return subprocess.check_output(['git','-C',a.decomp,*args])
    if git('rev-parse',REVISION+':'+SOURCE).decode().strip()!=BLOB: raise ValueError('blob pin')
    raw=git('cat-file','blob',BLOB)
    if sha(raw)!=SHA256: raise ValueError('source pin')
    units=definitions(raw.decode()); output=[]; declarations=[]; evidence=[]
    for name,original in units.items():
        native,records=rewrite(name,original)
        if native:
            declarations.append(native[:native.index('{')].strip()+';\n');output.append(native)
        evidence.append({'name':name,'source_sha256':sha(original.encode()),'native_sha256':sha(native.encode()),'rewrites':records})
    generated='\n'.join(output)
    declaration=''.join(declarations)
    game=(root/'src/end_level_8_source.c').read_bytes()
    outputs={'end_level_8_original.inc':sha(generated.encode()),'end_level_8_declarations.inc':sha(declaration.encode()),'end_level_8_initial_data.inc':sha(initial_include.encode()),'game.c':sha(game)}
    if outputs!=recipe['native_outputs']: raise ValueError('End8 whole-output fingerprint')
    if sha(generated.encode())!=recipe['output_sha256']: raise ValueError('native output guard')
    if evidence!=recipe['units']: raise ValueError('unit provenance guard')
    dest=Path(a.output);dest.mkdir(parents=True,exist_ok=True)
    (dest/'end_level_8_original.inc').write_text(generated)
    (dest/'end_level_8_declarations.inc').write_text(declaration)
    (dest/'end_level_8_initial_data.inc').write_text(initial_include)
    (dest/'game.c').write_bytes(game)
    (dest/'evidence.json').write_text(json.dumps({'revision':REVISION,'blob':BLOB,'sha256':SHA256,'units':evidence,'output_sha256':sha(generated.encode()),'native_outputs':outputs,'initialized_snapshot_sha256':sha(raw_initial),'initialized_include_sha256':sha(initial_include.encode()),'initialized_authority':recipe['initialized_authority'],'generator_sha256':sha(Path(__file__).read_bytes()),'recipe_sha256':sha(recipe_raw)},indent=2)+'\n')
if __name__=='__main__': main()
