#!/usr/bin/env python3
"""Materialize the complete admitted own-GAME transition definitions privately."""
import argparse
import hashlib
import json
from pathlib import Path
from prepare_engine import git, publish
REVISION = "df8815e75f3d79618466e01e0827c6165b93d11c"
SOURCE = "src/app_render/52DE0.c"
DEFINITIONS = (
    ("7430",23,25), ("7460",27,29), ("7470",31,35), ("748C",37,39),
    ("750C",41,58), ("7660",60,86), ("7760",88,113), ("7860",115,132),
    ("7918",134,137), ("7948",139,158), ("7A58",160,177), ("7BAC",179,211),
    ("7CC4",213,257), ("7F40",259,262), ("7F68",264,290),
)

def prepare(repository, manifest_path, output_path, evidence_path):
    manifest_bytes = manifest_path.read_bytes()
    m = json.loads(manifest_bytes)
    if m["revision"] != REVISION or m["source_path"] != SOURCE or m["transformation_version"] != "screen-transition-v1":
        raise ValueError("unsupported screen transition authority")
    raw = git(repository, "show", REVISION + ":" + SOURCE)
    blob = hashlib.sha1(b"blob " + str(len(raw)).encode() + b"\0" + raw).hexdigest()
    if blob != m["git_blob"] or hashlib.sha256(raw).hexdigest() != m["source_sha256"]:
        raise ValueError("screen transition source identity mismatch")
    if m["git_blob"] != "69f85403fc02060c7d5b2aa0c4428ed4fd1a4aaa":
        raise ValueError("unadmitted transition blob")
    if [(u["symbol"],u["first"],u["last"]) for u in m["definitions"]] != [
            ("func_800A"+name,first,last) for name,first,last in DEFINITIONS]:
        raise ValueError("incomplete transition definition inventory")
    viewport = m["viewport_definition"]
    if (viewport["path"], viewport["symbol"], viewport["first"], viewport["last"],
            viewport["git_blob"]) != ("src/app_render/53AD0.c", "func_800A844C", 72, 138,
                                     "aa25770661121586cb00b1927a96e147878a3255"):
        raise ValueError("unadmitted viewport helper")
    viewport_raw = git(repository, "show", REVISION + ":" + viewport["path"])
    if (hashlib.sha256(viewport_raw).hexdigest() != viewport["file_sha256"] or
            hashlib.sha1(b"blob " + str(len(viewport_raw)).encode() + b"\0" + viewport_raw).hexdigest() != viewport["git_blob"]):
        raise ValueError("viewport source identity mismatch")
    lines = raw.decode().splitlines(keepends=True)
    output = ['/* Private transformed own-GAME definitions. */\n#include "screen_transition_source.h"\n#include <math.h>\n#include <stdbool.h>\n']
    for unit in [viewport, *m["definitions"]]:
        origin = viewport_raw.decode().splitlines(keepends=True) if unit is viewport else lines
        selected = ''.join(origin[unit["first"]-1:unit["last"]])
        if hashlib.sha256(selected.encode()).hexdigest() != unit["source_sha256"]:
            raise ValueError("screen transition body identity mismatch")
        native = selected
        for change in unit["changes"]:
            if change["occurrences"] < 1 or native.count(change["find"]) != change["occurrences"]:
                raise ValueError("screen transition rewrite count mismatch: " + unit["symbol"])
            native = native.replace(change["find"], change["replace"])
        if hashlib.sha256(native.encode()).hexdigest() != unit["native_sha256"]:
            raise ValueError("screen transition native body identity mismatch")
        output.extend(['#line %d "snap@%s/%s"\n' % (unit["first"], REVISION, unit.get("path", SOURCE)), native, '\n'])
    text = ''.join(output)
    evidence = dict(m, manifest_sha256=hashlib.sha256(manifest_bytes).hexdigest(),
                    generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                    output_sha256=hashlib.sha256(text.encode()).hexdigest())
    if output_path.resolve() == evidence_path.resolve():
        raise ValueError("source and evidence must be distinct")
    publish(output_path, text)
    publish(evidence_path, json.dumps(evidence, indent=2) + '\n')

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('repository', 'manifest', 'output', 'evidence'):
        parser.add_argument('--' + name, type=Path, required=True)
    a = parser.parse_args()
    prepare(a.repository, a.manifest, a.output, a.evidence)
if __name__ == '__main__':
    main()
