#!/usr/bin/env python3
"""Materialize the complete immutable own-GAME score_Render producer privately."""
import argparse
import hashlib
import json
from pathlib import Path
from prepare_engine import git, publish

REVISION = "df8815e75f3d79618466e01e0827c6165b93d11c"
SOURCE = "src/app_render/score.c"
BLOB = "17a0d973c0050536cf4af4265a0bb106f294c82e"
SOURCE_SHA256 = "ae38f419085fa0506c799d06347b4f6e0772ac06819021f07f838735e24cb60c"
VERSION = "photo-score-render-complete-v1"
BODY_SHA256 = "037d3260494361e6b863a306907723a8821b07fe9a7e3d3218d0c8d493992300"
RECIPE_SHA256 = "f74ec9db0b5b4d8562c8559a13e4d47876357f8137d10fb993f9907a808e8d3a"
OUTPUT_SHA256 = "592b3d1386faef15f22ea865e3880be38198622055a02cbfb1894509622a3f47"
SUPPORT = ("src/photo_score_source.h", "src/photo_score_source.c")
PREAMBLE = '/* Complete own-GAME score_Render; no SDK types or implementation. */\n#include "photo_score_source.h"\n'


def digest(data):
    return hashlib.sha256(data).hexdigest()


def prepare(repository, manifest_path, directory):
    root = Path(__file__).resolve().parent.parent
    directory = directory.resolve()
    if directory.name != "isle-photo-score-render-private" or directory.parent.name != "_deps":
        raise ValueError("score renderer requires exact private build _deps output")
    protected = (repository.resolve(), *(root / name for name in
        ("src", "include", "tests", "tools", "cmake", ".git", "graft")))
    if any(directory == base or base in directory.parents for base in protected):
        raise ValueError("score renderer output must remain outside source trees")
    manifest_bytes = manifest_path.read_bytes()
    manifest = json.loads(manifest_bytes)
    if (manifest["schema_version"] != 1 or manifest["revision"] != REVISION or
            manifest["source_path"] != SOURCE or manifest["transformation_version"] != VERSION or
            manifest["source_blob"] != BLOB or manifest["source_sha256"] != SOURCE_SHA256 or
            manifest["generator_sha256"] != digest(Path(__file__).read_bytes()) or
            set(manifest["native_support"]) != set(SUPPORT)):
        raise ValueError("score renderer immutable authority/recipe identity mismatch")
    for path, identity in manifest["native_support"].items():
        if digest((root / path).read_bytes()) != identity["sha256"]:
            raise ValueError("score renderer native support identity mismatch: " + path)
    if git(repository, "cat-file", "-t", REVISION).strip() != b"commit":
        raise ValueError("score renderer authority is not the pinned commit")
    raw = git(repository, "show", REVISION + ":" + SOURCE)
    blob = hashlib.sha1(b"blob " + str(len(raw)).encode() + b"\0" + raw).hexdigest()
    if blob != BLOB or digest(raw) != SOURCE_SHA256:
        raise ValueError("score renderer pinned source identity mismatch")
    unit = manifest["definition"]
    if (unit["symbol"] != "score_Render" or unit["first"] != 154 or unit["last"] != 228 or
            unit["source_declaration"] != "void score_Render(GObj* camObj)"):
        raise ValueError("score renderer requires its one complete original definition")
    original = "".join(raw.decode().splitlines(keepends=True)[153:228])
    if digest(original.encode()) != BODY_SHA256 or unit["source_sha256"] != BODY_SHA256 or not original.endswith("}\n"):
        raise ValueError("score renderer complete source body identity mismatch")
    native = original
    for change in unit["changes"]:
        count = change["occurrences"]
        if not isinstance(count, int) or isinstance(count, bool) or count < 1 or native.count(change["find"]) != count:
            raise ValueError("score renderer counted rewrite mismatch: " + change["find"])
        native = native.replace(change["find"], change["replace"])
    recipe = json.dumps(unit, sort_keys=True, separators=(",", ":")).encode()
    if (digest(recipe) != RECIPE_SHA256 or manifest["recipe_sha256"] != RECIPE_SHA256 or
            digest(native.encode()) != unit["native_sha256"] or
            native[:native.index("{")].strip() != unit["native_declaration"]):
        raise ValueError("score renderer complete native body/recipe identity mismatch")
    output = PREAMBLE + '#line 154 "snap@' + REVISION + '/' + SOURCE + '"\n' + native
    if digest(output.encode()) != OUTPUT_SHA256 or manifest["output_sha256"] != OUTPUT_SHA256:
        raise ValueError("score renderer private output identity mismatch")
    publish(directory / "photo_score_render_original.c", output)
    publish(directory / "evidence.json", json.dumps(dict(manifest, manifest_sha256=digest(manifest_bytes)), indent=2) + "\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repository", type=Path, required=True)
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    prepare(args.repository, args.manifest, args.output)


if __name__ == "__main__":
    main()
