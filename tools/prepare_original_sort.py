#!/usr/bin/env python3
"""Once-only pinned Original C sort/name comparator provider, private output."""
import argparse
import hashlib
import json
import re
from pathlib import Path
from prepare_engine import git, publish
REVISION='df8815e75f3d79618466e01e0827c6165b93d11c'
def sha(value):
    return hashlib.sha256(value if isinstance(value,bytes) else value.encode()).hexdigest()
def rewrite(body,name):
    records=[]
    def change(old,new,count=1):
        nonlocal body
        if body.count(old)!=count:
            raise ValueError((name,old,body.count(old),count))
        body=body.replace(old,new)
        records.append({'find':old,'replace':new,'count':count})
    if name=='swap':
        change('void swap(', 'static void isle_original_swap(')
    elif name=='qsortInternal':
        change('void qsortInternal(', 'static void isle_original_qsort_internal(IsleOriginalSortCall* sort, ')
        change('qsortInternal(', 'isle_original_qsort_internal(sort, ',2)
        change('swap(', 'isle_original_swap(',2)
        change('qsortCompareFunc(', 'sort->compare(sort->context, ',2)
        change('qsortItemSize','sort->item_size',10)
        change('            last = s1 - sort->item_size;', '            if (s1 == first) break;\n            last = s1 - sort->item_size;')
        change('            isle_original_qsort_internal(sort, first, s1 - sort->item_size);', '            if (s1 > first) isle_original_qsort_internal(sort, first, s1 - sort->item_size);')
    elif name=='qsort2':
        change('void qsort2(void* _base, u32 count, u32 itemSize, s32 (*compare)(const void*, const void*)) {', 'void isle_original_source_qsort2(void* context, void* _base, uint32_t count, uint32_t itemSize, IsleOriginalSourceCompare compare) {\n    IsleOriginalSortCall call = {context, itemSize, compare};\n    IsleOriginalSortCall* sort = &call;\n    if (count < 2) return;')
        change('compare(curr, next)', 'compare(context, curr, next)')
        change('qsortItemSize','sort->item_size',2)
        change('qsortCompareFunc','sort->compare')
        change('qsortInternal(', 'isle_original_qsort_internal(sort, ')
    elif name=='func_803750CC_84887C':
        change('s32 func_803750CC_84887C(char* arg0, char* arg1)', 'int32_t isle_original_source_name_compare(const char* arg0, const char* arg1)')
    else:
        raise ValueError(('unadmitted sort definition',name))
    body=re.sub(r'\bs32\b','int32_t',body)
    body=re.sub(r'\bu8\b','uint8_t',body)
    body=re.sub(r'\bu32\b','uint32_t',body)
    return body,records

def main():
    p=argparse.ArgumentParser()
    p.add_argument('--source-repo',required=True,type=Path)
    p.add_argument('--output',required=True,type=Path)
    p.add_argument('--manifest',type=Path,default=Path(__file__).resolve().parents[1]/'cmake/original-sort-inputs.json')
    p.add_argument('--manifest-sha256',required=True)
    a=p.parse_args(); raw_recipe=a.manifest.read_bytes(); recipe=json.loads(raw_recipe)
    if not re.fullmatch(r'[0-9a-f]{64}',a.manifest_sha256) or sha(raw_recipe)!=a.manifest_sha256:
        raise ValueError('sort externally admitted recipe fingerprint')
    if recipe['revision']!=REVISION or [d['name'] for d in recipe['definitions']]!=['swap','qsortInternal','qsort2','func_803750CC_84887C']:
        raise ValueError('unadmitted sort pin/closure')
    root=Path(__file__).resolve().parents[1]
    if not recipe['native_support']: raise ValueError('missing sort native guards')
    for path,identity in recipe['native_support'].items():
        if sha((root/path).read_bytes())!=identity['sha256']: raise ValueError(('sort support guard',path))
    texts={}
    for path,identity in recipe['source_files'].items():
        raw=git(a.source_repo,'show',REVISION+':'+path)
        blob=hashlib.sha1(b'blob '+str(len(raw)).encode()+b'\0'+raw).hexdigest()
        if blob!=identity['blob'] or sha(raw)!=identity['sha256']: raise ValueError(('sort Source identity',path))
        texts[path]=raw.decode()
    bodies=[]; evidence=[]
    for unit in recipe['definitions']:
        body=''.join(texts[unit['source']].splitlines(keepends=True)[unit['first']-1:unit['last']])
        if sha(body)!=unit['sha256']: raise ValueError(('sort source selector',unit['name']))
        body,records=rewrite(body,unit['name']); bodies.append(body)
        evidence.append({**unit,'rewrites':records,'native_sha256':sha(body)})
    body='\n'.join(bodies)
    native_unit=(root/'src/original_sort_source.c').read_text()
    outputs={'game.c':sha(native_unit),'original_sort_original.inc':sha(body)}
    if evidence!=recipe['native_definitions']:
        raise ValueError('sort native definition/rewrite fingerprint')
    if outputs!=recipe['native_outputs']:
        raise ValueError('sort whole-output fingerprint')
    a.output.parent.mkdir(parents=True,exist_ok=True)
    publish(a.output,native_unit)
    publish(a.output.with_name('original_sort_original.inc'),body)
    publish(a.output.with_name('evidence.json'),json.dumps({'provider':recipe['provider'],'revision':REVISION,'recipe_sha256':sha(raw_recipe),'generator_sha256':sha(Path(__file__).read_bytes()),'source_files':recipe['source_files'],'native_support':recipe['native_support'],'definitions':evidence,'outputs':outputs,'compiled_ownership':'once-only low-level C; PhotoCheck/Report C thunks only'},indent=2)+'\n')
if __name__=='__main__': main()
