#!/usr/bin/env python3
"""Materialize complete pinned Volcano Original C selectors outside source trees."""
import argparse
import hashlib
import json
import re
from pathlib import Path
from prepare_engine import git, publish
REVISION='df8815e75f3d79618466e01e0827c6165b93d11c'
PROVIDER='volcano-actor'
VERSION='volcano-complete312-typed-v1'
RECIPE_SHA256='ca057591bc36fb460f4beaad94bf39f93134965d275af2921d7daff766c7ed22'
OUTPUT_SHA256='bdfdea0dd4dc061229308bedc6680f3ca82fb1b236874ba6467e7e477af809c2'
def digest(raw):return hashlib.sha256(raw).hexdigest()
def canonical(value):return digest(json.dumps(value,sort_keys=True,separators=(',',':')).encode())
def rewrite(body,changes):
    for c in changes:
        if 'edits' in c:
            last=len(body)
            for e in reversed(c['edits']):
                if e['end']>last or body[e['start']:e['end']]!=e['find']:raise ValueError('counted span rewrite source mismatch')
                last=e['start'];body=body[:e['start']]+e['replace']+body[e['end']:]
        elif 'token' in c:
            pattern=r'\b'+re.escape(c['token'])+r'\b'
            if len(re.findall(pattern,body))!=c['occurrences']:raise ValueError('counted token rewrite mismatch')
            body=re.sub(pattern,lambda _:c['replace'],body)
        else:
            if c['occurrences']<1 or body.count(c['find'])!=c['occurrences']:raise ValueError('counted exact rewrite mismatch')
            body=body.replace(c['find'],c['replace'])
    return body

def prepare(repository,manifest_path,output_path,evidence_path):
    raw_manifest=manifest_path.read_bytes();m=json.loads(raw_manifest)
    root=Path(__file__).resolve().parent.parent
    if m['schema_version']!=1 or m['revision']!=REVISION or m['provider']!=PROVIDER or m['transformation_version']!=VERSION:raise ValueError('Volcano authority/provider/version mismatch')
    out,receipt=output_path.resolve(),evidence_path.resolve();parent=out.parent
    protected=(repository.resolve(),*(root/n for n in ('src','tools','include','tests','cmake','.git')))
    if out.name!='game.c' or receipt.name!='evidence.json' or parent!=receipt.parent or parent.name!='isle-'+PROVIDER+'-private' or parent.parent.name!='_deps' or any(parent==p or p in parent.parents for p in protected):raise ValueError('Volcano outputs must be private build game.c/evidence.json siblings')
    if m['generator_sha256']!=digest(Path(__file__).read_bytes()):raise ValueError('Volcano generator identity mismatch')
    recipe={k:m[k] for k in ('source_files','definitions','semantic_origins','declarations','preamble','epilogue','ownership','typed_contract','numeric_evidence','exclusions')}
    if canonical(recipe)!=RECIPE_SHA256:raise ValueError('Volcano closed complete recipe mismatch')
    if git(repository,'cat-file','-t',REVISION).strip()!=b'commit':raise ValueError('Volcano authority is not pinned commit')
    sources={}
    for path,record in m['source_files'].items():
        raw=git(repository,'show',REVISION+':'+path)
        blob=hashlib.sha1(b'blob '+str(len(raw)).encode()+b'\0'+raw).hexdigest()
        if digest(raw)!=record['sha256'] or blob!=record['git_blob'] or len(raw.splitlines())!=record['line_count']:raise ValueError('Volcano pinned source identity mismatch: '+path)
        sources[path]=raw.decode()
    for path,record in m['native_support'].items():
        native=(root/path).resolve()
        if root not in native.parents or digest(native.read_bytes())!=record['sha256']:raise ValueError('Volcano native support identity mismatch: '+path)
    for d in m['semantic_origins']:
        text=''.join(sources[d['path']].splitlines(keepends=True)[d['first']-1:d['last']])
        if digest(text.encode())!=d['source_sha256']:raise ValueError('Volcano Source datum provenance mismatch: '+d['symbol'])
    result=[m['preamble']]
    for d in m['declarations']:
        if digest(d['native'].encode())!=d['native_sha256']:raise ValueError('Volcano typed data rewrite mismatch')
        result.extend([d['native'],'\n'])
    for d in m['definitions']:
        body=''.join(sources[d['path']].splitlines(keepends=True)[d['first']-1:d['last']]).rstrip('\n')
        if digest(body.encode())!=d['source_sha256']:raise ValueError('Volcano Original function identity mismatch: '+d['symbol'])
        native=rewrite(body,d['changes'])
        if digest(native.encode())!=d['native_sha256'] or native[:native.index('{')].strip()!=d['native_declaration']:raise ValueError('Volcano complete typed function rewrite mismatch: '+d['symbol'])
        result.extend(['#line '+str(d['first'])+' "snap@'+REVISION+'/'+d['path']+'"\n',native,'\n'])
    result.append(m['epilogue']);generated=''.join(result)
    if digest(generated.encode())!=OUTPUT_SHA256 or m['output_sha256']!=OUTPUT_SHA256:raise ValueError('Volcano complete generated Source identity mismatch')
    evidence={'schema_version':1,'revision':REVISION,'provider':PROVIDER,'manifest_sha256':digest(raw_manifest),'generator_sha256':m['generator_sha256'],'recipe_sha256':RECIPE_SHA256,'output_sha256':OUTPUT_SHA256,'definitions':[{k:d[k] for k in ('path','symbol','first','last','source_sha256','native_sha256','source_declaration','native_declaration')} for d in m['definitions']],'ownership':m['ownership'],'typed_contract':m['typed_contract'],'numeric_evidence':m['numeric_evidence'],'exclusions':m['exclusions'],'scope':'COMPLETE_PINNED_ORIGINAL_C_TYPED_PROVIDER_MATERIALIZATION_ONLY','native_execution':'NOT_CLAIMED; parent must bind actual native owners/services and exercise acceptance'}
    publish(out,generated);publish(receipt,json.dumps(evidence,indent=2)+'\n')

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for n in ('repository','manifest','output','evidence'):p.add_argument('--'+n,type=Path,required=True)
    a=p.parse_args();prepare(a.repository,a.manifest,a.output,a.evidence)
if __name__=='__main__':main()
