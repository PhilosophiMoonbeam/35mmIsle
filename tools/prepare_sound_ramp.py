#!/usr/bin/env python3
"""Materialize the eight pinned original app-render sound-ramp C definitions."""
import argparse
import hashlib
import json
from pathlib import Path
from prepare_engine import git, publish

REVISION = "df8815e75f3d79618466e01e0827c6165b93d11c"
SOURCE = "src/app_render/568D0.c"
BLOB = "fed2e286e73ef66a946ebd7b614f2d2b7b1fca6c"
FILE_SHA256 = "ab288404b621d0b12a51c846f00ae7335905931595ea58ca005f073c3e12b2b3"
DEFINITIONS = (
    ("func_800AB240", 72, 100), ("func_800AB41C", 102, 130),
    ("func_800AB518", 132, 162), ("func_800AB700", 164, 192),
    ("func_800AB7FC", 194, 208), ("func_800AB874", 210, 221),
    ("func_800AB918", 223, 236), ("func_800AB9A4", 238, 253),
)


def prepare(repository, manifest_path, output_path, evidence_path):
    manifest_bytes = manifest_path.read_bytes()
    manifest = json.loads(manifest_bytes)
    if (manifest["schema_version"] != 1 or manifest["revision"] != REVISION or
            manifest["source_path"] != SOURCE or manifest["git_blob"] != BLOB or
            manifest["source_sha256"] != FILE_SHA256 or
            manifest["transformation_version"] != "sound-ramp-v1"):
        raise ValueError("unsupported sound-ramp authority")
    inventory = tuple((unit["symbol"], unit["first"], unit["last"])
                      for unit in manifest["definitions"])
    if inventory != DEFINITIONS:
        raise ValueError("sound-ramp definition inventory differs from the eight admitted bodies")
    support_paths = manifest["native_support_paths"]
    if not support_paths or set(support_paths) != set(manifest["native_support"]):
        raise ValueError("incomplete sound-ramp native support inventory")
    native_support = {}
    native_root = Path(__file__).resolve().parents[1]
    for relative in support_paths:
        observed = hashlib.sha256((native_root / relative).read_bytes()).hexdigest()
        if observed != manifest["native_support"][relative]:
            raise ValueError("sound-ramp native support identity mismatch: " + relative)
        native_support[relative] = observed
    raw = git(repository, "show", REVISION + ":" + SOURCE)
    blob = hashlib.sha1(b"blob " + str(len(raw)).encode() + b"\0" + raw).hexdigest()
    if blob != BLOB or hashlib.sha256(raw).hexdigest() != FILE_SHA256:
        raise ValueError("sound-ramp pinned source identity mismatch")
    lines = raw.decode("utf-8").splitlines(keepends=True)
    output = ["/* Build-private complete original sound-ramp C definitions. */\n",
              '#include "sound_ramp_source.h"\n#include <stdbool.h>\n\n']
    definitions = []
    for unit in manifest["definitions"]:
        first, last = unit["first"], unit["last"]
        if not 1 <= first <= last <= len(lines):
            raise ValueError("sound-ramp source span outside authority")
        selected = "".join(lines[first - 1:last])
        if hashlib.sha256(selected.encode()).hexdigest() != unit["source_sha256"]:
            raise ValueError("sound-ramp selected body identity mismatch: " + unit["symbol"])
        native = selected
        for change in unit["changes"]:
            count = change["occurrences"]
            if (not change["find"] or type(count) is not int or count <= 0 or
                    native.count(change["find"]) != count):
                raise ValueError("sound-ramp positive rewrite count mismatch: " + unit["symbol"])
            native = native.replace(change["find"], change["replace"])
        if hashlib.sha256(native.encode()).hexdigest() != unit["native_sha256"]:
            raise ValueError("sound-ramp transformed body identity mismatch: " + unit["symbol"])
        output.extend(['#line %d "snap@%s/%s"\n' % (first, REVISION, SOURCE), native, "\n"])
        definitions.append({key: unit[key] for key in
                            ("symbol", "first", "last", "source_sha256", "native_sha256")})
    text = "".join(output)
    if hashlib.sha256(text.encode()).hexdigest() != manifest["generated_source_sha256"]:
        raise ValueError("sound-ramp generated translation-unit identity mismatch")
    if output_path.resolve() == evidence_path.resolve():
        raise ValueError("sound-ramp source and evidence must be distinct")
    evidence = {
        "revision": REVISION, "source_path": SOURCE, "git_blob": BLOB,
        "source_sha256": FILE_SHA256,
        "manifest_sha256": hashlib.sha256(manifest_bytes).hexdigest(),
        "generator_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        "generated_source_sha256": hashlib.sha256(text.encode()).hexdigest(),
        "definitions": definitions, "numeric_policy": manifest["numeric_policy"],
        "native_support": native_support,
    }
    publish(output_path, text)
    publish(evidence_path, json.dumps(evidence, indent=2) + "\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("repository", "manifest", "output", "evidence"):
        parser.add_argument("--" + name, type=Path, required=True)
    args = parser.parse_args()
    prepare(args.repository, args.manifest, args.output, args.evidence)


if __name__ == "__main__":
    main()
