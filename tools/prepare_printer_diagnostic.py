#!/usr/bin/env python3
"""Admit the pinned private printer diagnostic translation, never working-tree C."""
import argparse
import hashlib
import json
import re
import subprocess
from pathlib import Path
REVISION='df8815e75f3d79618466e01e0827c6165b93d11c'
SOURCE='src/AA18E0.c'
BLOB='c49dd1573ace8cd1c58940bbd60def2149125164'
SHA256='650df306cb91d47858371a0ce7e5e10de4145465d7f4b8ef0cb23543c48443a3'
NAMES=['func_801DC8A0_AA18E0','func_801DC930_AA1970','func_801DC9BC_AA19FC','func_801DC9D0_AA1A10','func_801DCCD4_AA1D14','func_801DCCDC_AA1D1C','func_801DD010_AA2050']
TOKENS=re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|//[^\n]*|/\*.*?\*/',re.S)
def sha(data): return hashlib.sha256(data).hexdigest()
def definitions(text):
    masked=TOKENS.sub(lambda m:' '*len(m[0]),text); result={}
    for m in re.finditer(r'(?m)^(?:void|s32)\s+(\w+)\([^;{}]*\)\s*\{',masked):
        depth=1; end=m.end()
        while depth:
            if end>=len(masked): raise ValueError('unterminated definition')
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
        text=text.replace(old,new); records.append({'selector_sha256':sha(old.encode()),'replacement_sha256':sha(new.encode()),'count':count})
    if name==NAMES[1]:
        text='static void isle_printer_diagnostic_'+name+'(Context* source,OMCamera* cam,PhotoData* photoData) {\n    pd_check(source,source->requiredNativeTable->presentation_begin(source,cam,photoData));\n}\n'
        return text,[{'mechanical':'native presentation initializes actual photo and pauses both original process lists'}]
    if name==NAMES[5]:
        return 'static void isle_printer_diagnostic_'+name+'(Context* source) {\n    pd_check(source,source->requiredNativeTable->camera_setup(source));\n    for (int i=0;i<4;i++) source->saved[i]=NULL;\n}\n',[{'mechanical':'typed native exact camera and sprite setup; original saved pointer clearing'}]
    if name==NAMES[6]:
        return 'void isle_printer_diagnostic_'+name+'(void* user) { isle_printer_diagnostic_source_entry(user); }\n',[{'mechanical':'independent C entry continuation retains actual retirement result'}]
    if name==NAMES[3]:
        change('    sp2C = arg0->data.cam;','    sp2C = source->camera;')
        change('D_801E55DC_AAA61C->flags &= ~1;','pd_check(source,source->requiredNativeTable->hidden(source,source->sprite_camera,0));')
        change('D_801E55DC_AAA61C->flags |= 1;','pd_check(source,source->requiredNativeTable->hidden(source,source->sprite_camera,1));')
        change('scSetPostProcessFunc(func_801DC9BC_AA19FC);','pd_check(source,source->requiredNativeTable->postprocess(source,1));')
        change('scRemovePostProcessFunc();','pd_check(source,source->requiredNativeTable->postprocess(source,0));')
        change('func_8009FB50(D_801E54BC_AAA4FC, D_801E54C0_AAA500, D_801E54C4_AAA504);','pd_check(source,source->requiredNativeTable->replay_initialize(source,source->photo_link,source->actor_link,source->item_link));')
    substitutions={'PANIC()':'pd_check(source,source->requiredNativeTable->panic(source))','func_8009FBC4()':'pd_cleanup(source)','func_800067DC()':'pd_terminal(source)','contPrinterGetStatus()':'pd_status(source)','contPrinterSendCommand(':'pd_command(source,','ohWait(':'pd_wait(source,','func_800BF574_5C414(':'pd_saved(source,','gContInputPressedButtons':'pd_pressed(source)'}
    for old,new in substitutions.items():
        count=text.count(old)
        if count: change(old,new,count)
    fields={'D_801E54C8_AAA508':'result_words','D_801E55D8_AAA618':'argument','D_801E550C_AAA54C':'render_complete','D_801E55E0_AAA620':'saved'}
    for old,new in fields.items():
        count=text.count(old)
        if count: change(old,'source->'+new,count)
    for called in (NAMES[0],NAMES[1]):
        count=text.count(called+'(')
        # Include the defining signature separately below.
        if called!=name and count: change(called+'(','isle_printer_diagnostic_'+called+'(source,',count)
    if name==NAMES[3]:
        change('void '+name+'(GObj* arg0) {','void isle_printer_diagnostic_'+name+'(GObj* arg0) {\n    Context* source=(Context*)isle_post_course_source_require(arg0,-2);')
    elif name==NAMES[2]:
        change('void '+name+'(void* unused) {','void isle_printer_diagnostic_'+name+'(void* unused) {\n    Context* source=(Context*)unused;')
    else:
        change('void '+name+'(s32 '+('arg0' if name==NAMES[0] else 'unused')+') {','static void isle_printer_diagnostic_'+name+'(Context* source,s32 '+('arg0' if name==NAMES[0] else 'unused')+') {')
    return text,records

def materialize(raw,root):
    units=definitions(raw.decode()); output=[]; declarations=[]; evidence=[]
    for name,original in units.items():
        native,records=rewrite(name,original); output.append(native)
        declarations.append(native[:native.index('{')].strip()+';\n')
        evidence.append({'name':name,'source_sha256':sha(original.encode()),'native_sha256':sha(native.encode()),'rewrites':records})
    return {'printer_diagnostic_original.inc':'\n'.join(output).encode(),'printer_diagnostic_declarations.inc':''.join(declarations).encode(),'game.c':(root/'src/printer_diagnostic_source.c').read_bytes()},evidence

def main():
    ap=argparse.ArgumentParser()
    for flag in ('decomp','output','recipe','manifest-sha256','support-root'): ap.add_argument('--'+flag,required=True)
    a=ap.parse_args(); recipe_raw=Path(a.recipe).read_bytes()
    if not re.fullmatch(r'[0-9a-f]{64}',a.manifest_sha256) or sha(recipe_raw)!=a.manifest_sha256: raise ValueError('externally admitted manifest fingerprint')
    recipe=json.loads(recipe_raw); root=Path(a.support_root)
    if (recipe['revision'],recipe['source'],recipe['definition_count'])!=(REVISION,SOURCE,7): raise ValueError('source selector')
    def git(*args): return subprocess.check_output(['git','-C',a.decomp,*args])
    blob=git('rev-parse',REVISION+':'+SOURCE).decode().strip()
    if blob!=BLOB or recipe['blob']!=BLOB: raise ValueError('git blob guard')
    raw=git('cat-file','blob',blob)
    if sha(raw)!=SHA256 or recipe['sha256']!=SHA256: raise ValueError('source guard')
    for path,expected in recipe['support_sha256'].items():
        if sha((root/path).read_bytes())!=expected: raise ValueError('support guard '+path)
    outputs,evidence=materialize(raw,root)
    fingerprints={name:sha(data) for name,data in outputs.items()}
    if evidence!=recipe['units'] or len(evidence)!=7: raise ValueError('unit and rewrite count guard')
    if fingerprints!=recipe['native_outputs'] or fingerprints['printer_diagnostic_original.inc']!=recipe['output_sha256']: raise ValueError('whole output guard')
    dest=Path(a.output); dest.mkdir(parents=True,exist_ok=True)
    for name,data in outputs.items(): (dest/name).write_bytes(data)
    (dest/'evidence.json').write_text(json.dumps({'revision':REVISION,'source':SOURCE,'blob':blob,'sha256':sha(raw),'units':evidence,'native_outputs':fingerprints,'output_sha256':fingerprints['printer_diagnostic_original.inc'],'recipe_sha256':sha(recipe_raw),'generator_sha256':sha(Path(__file__).read_bytes())},indent=2)+'\n')
if __name__=='__main__': main()
