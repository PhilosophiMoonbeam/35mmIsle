#!/usr/bin/env python3
"""Materialize complete pinned Valley actor Original C in a build-private directory."""
import argparse
import hashlib
import json
from pathlib import Path
from prepare_engine import git, publish
REVISION = 'df8815e75f3d79618466e01e0827c6165b93d11c'
PROVIDER = 'valley-actor'
VERSION = 'valley-complete309-typed-v2'
SOURCE_INVENTORY_SHA256 = '8acfd1257e923f92a0b43fac185699ea7bf713f625fcab9af994e0b9ae63eb6a'
SELECTION_SHA256 = 'e575ff4bc2a4358bd7bb2edb62ede5ff8c2945062da3a07ad1073fd333d2e50c'
RECIPE_SHA256 = '0bdf9b45db9aee49e7eefdee2de71ca988c38c93bc9ef0eff73d852b7a08582a'
OUTPUT_SHA256 = '473abd80a7626a8f6fde7a8e5ec757ef91b7850786acbddac6a8ff9b918aa6ba'
SUPPORT = ('src/valley_actor_source.h', 'src/valley_actor_source.c', 'src/valley_actor.hpp', 'src/valley_actor.cpp', 'src/valley_lifecycle_source.h', 'src/valley_lifecycle_source.c', 'src/course_actor_source.h', 'src/course_actor_source.c', 'src/actor_source.h', 'src/actor_owner_source.h', 'src/actor_source_bridge.c', 'src/actor_source_owner_bridge.c', 'src/actor_owner.cpp', 'src/actor_behavior_bindings.hpp', 'src/actor_behavior_bindings.cpp', 'src/valley_actor_catalog.cpp', 'src/engine_internal.h', 'src/engine_math.h', 'src/item_source.h', 'include/35mmisle/engine.h', 'include/35mmisle/continuation.h', 'include/35mmisle/rng.h', 'tools/prepare_engine.py')
DEFINITION_COUNT = 290
RECIPE_KEYS = ('definitions', 'preamble', 'field_ownership', 'native_services',
               'native_lifetime', 'no_emitted_file_local_helpers',
               'extra_signed_root', 'counted_four_options', 'semantic_origins')

def digest(value):
    return hashlib.sha256(value).hexdigest()

def canonical(value):
    return digest(json.dumps(value,sort_keys=True,separators=(',',':')).encode())

def prepare(repository,manifest_path,output_path,evidence_path):
    manifest_bytes=manifest_path.read_bytes()
    manifest=json.loads(manifest_bytes)
    root=Path(__file__).resolve().parent.parent
    if (manifest['schema_version']!=1 or manifest['revision']!=REVISION or
            manifest['provider']!=PROVIDER or manifest['transformation_version']!=VERSION):
        raise ValueError('Valley actor authority/version mismatch')
    output,evidence=output_path.resolve(),evidence_path.resolve()
    parent=output.parent
    protected=(repository.resolve(),*(root/name for name in ('src','include','tests','tools','cmake','.git','graft')))
    if (output.name!='game.c' or evidence.name!='evidence.json' or output==evidence or
            parent!=evidence.parent or parent.name!='isle-valley-actor-private' or
            parent.parent.name!='_deps' or any(parent==base or base in parent.parents for base in protected)):
        raise ValueError('Valley actor requires exact private build output paths')
    if manifest['generator_sha256']!=digest(Path(__file__).read_bytes()):
        raise ValueError('Valley actor generator identity mismatch')
    if canonical(manifest['source_files'])!=SOURCE_INVENTORY_SHA256:
        raise ValueError('Valley actor complete pinned input inventory mismatch')
    if canonical({key:manifest[key] for key in RECIPE_KEYS})!=RECIPE_SHA256:
        raise ValueError('Valley actor typed transformation/ownership recipe mismatch')
    if set(manifest['native_support'])!=set(SUPPORT):
        raise ValueError('Valley actor native support allowlist mismatch')
    if git(repository,'cat-file','-t',REVISION).strip()!=b'commit':
        raise ValueError('Valley authority is not the pinned commit')
    sources={}
    for path,identity in manifest['source_files'].items():
        raw=git(repository,'show',REVISION+':'+path)
        blob=hashlib.sha1(b'blob '+str(len(raw)).encode()+b'\0'+raw).hexdigest()
        if (blob!=identity['git_blob'] or digest(raw)!=identity['sha256'] or
                len(raw.splitlines())!=identity['line_count']):
            raise ValueError('Valley actor pinned source identity mismatch: '+path)
        sources[path]=raw
    for path,identity in manifest['native_support'].items():
        selected=(root/path).resolve()
        if root not in selected.parents or digest(selected.read_bytes())!=identity['sha256']:
            raise ValueError('Valley actor native support identity mismatch: '+path)
    units=manifest['definitions']
    selection=[[u[key] for key in ('path','symbol','native_symbol','first','last','source_byte_start','source_byte_end_exclusive')] for u in units]
    if (len(units)!=DEFINITION_COUNT or len({u['native_symbol'] for u in units})!=DEFINITION_COUNT or
            canonical(selection)!=SELECTION_SHA256):
        raise ValueError('Valley actor requires its complete unique exact selector set')
    generated=[manifest['preamble']]
    for unit in units:
        raw=sources[unit['path']]
        first,last=unit['source_byte_start'],unit['source_byte_end_exclusive']
        if not (0<=first<last<=len(raw)):
            raise ValueError('Valley invalid exact Source extent: '+unit['symbol'])
        original=raw[first:last].decode()
        if digest(original.encode())!=unit['source_sha256']:
            raise ValueError('Valley actor original definition mismatch: '+unit['symbol'])
        position=0
        chunks=[]
        for change in unit['rewrites']:
            begin,end=change['first_codepoint'],change['last_codepoint_exclusive']
            if not (position<=begin<=end<=len(original)):
                raise ValueError('Valley overlapping/out-of-range typed rewrite: '+unit['symbol'])
            chunks.extend((original[position:begin],change['replace']))
            position=end
        chunks.append(original[position:])
        body=''.join(chunks)
        if (digest(body.encode())!=unit['native_sha256'] or
                body[:body.index('{')].strip()!=unit['native_declaration']):
            raise ValueError('Valley actor transformed definition/declaration mismatch: '+unit['symbol'])
        generated.append('#line %d "snap@%s/%s"\n%s\n'%(unit['first'],REVISION,unit['path'],body))
    # Preamble is followed directly by the first #line; only definitions have
    # the one-separator convention locked by the authoring output hash.
    text=manifest['preamble']+'\n'.join(generated[1:])
    if digest(text.encode())!=OUTPUT_SHA256 or manifest['output_sha256']!=OUTPUT_SHA256:
        raise ValueError('Valley actor complete generated output identity mismatch')
    receipt=dict(manifest,manifest_sha256=digest(manifest_bytes),
        source_inventory_sha256=SOURCE_INVENTORY_SHA256,selection_sha256=SELECTION_SHA256,
        recipe_sha256=RECIPE_SHA256,generated_source_sha256=OUTPUT_SHA256,
        scope='COMPLETE_PINNED_ORIGINAL_C_VALLEY_actor_TYPED_NATIVE_CONTRACT',
        native_owner_execution='NOT_CLAIMED: parent-owned real native host/services and acceptance required')
    publish(output,text)
    publish(evidence,json.dumps(receipt,indent=2)+'\n')

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('repository','manifest','output','evidence'):
        parser.add_argument('--'+name,required=True,type=Path)
    args=parser.parse_args()
    prepare(args.repository,args.manifest,args.output,args.evidence)

if __name__=='__main__':
    main()
