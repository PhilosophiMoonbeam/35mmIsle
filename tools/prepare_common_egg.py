#!/usr/bin/env python3
"""Materialize the single pinned six-function Original Egg C provider privately."""
import argparse
import hashlib
import json
from pathlib import Path
from prepare_engine import git, publish

REVISION = "df8815e75f3d79618466e01e0827c6165b93d11c"
VERSION = "common-egg-original6-v1"
SOURCE_SHA256 = "545fccf2508168cfff9aef749d2ca271073cd5e97c152cf3c84fbf934a8f5ad2"
SELECTOR_SHA256 = "6da8618545bf018238170fc9e98bb8e260688d844359b3ffb58fb4c8b711dba1"
RECIPE_SHA256 = "a5b8f6ba080a7de76672aa7f1b690914bccf0b13132432e2b3a240dcc3ca38d3"
OUTPUT_SHA256 = "98f8a8746d8b0aa6369dba16542a1e9e1522430850359925e9178d1f93b8b4cb"
NATIVE_SUPPORT = ("src/egg_source.h", "src/egg_source.c", "src/actor_source.h",
                  "src/course_actor_source.h", "src/engine_internal.h",
                  "include/35mmisle/engine.h")
RECIPE_KEYS = ("definitions", "semantic_origins", "preamble", "ownership")


def digest(raw):
    return hashlib.sha256(raw).hexdigest()


def canonical(value):
    return digest(json.dumps(value, sort_keys=True, separators=(",", ":")).encode())


def source_span(source, first, last):
    return "".join(source.splitlines(keepends=True)[first - 1:last]).rstrip("\r\n")


def require_common_provider(root, reference):
    """Check that a course imports the exact once-only provider, not private copies."""
    manifest = json.loads((root / "cmake/common-egg-inputs.json").read_bytes())
    actual = {key: manifest[key] for key in
              ("provider", "transformation_version", "definition_count", "output_sha256")}
    actual["manifest"] = "cmake/common-egg-inputs.json"
    actual["definitions"] = [{key: unit[key] for key in
        ("symbol", "native_symbol", "native_declaration", "source_sha256", "native_sha256")}
        for unit in manifest["definitions"]]
    generator = root / "tools/prepare_common_egg.py"
    if (actual != reference or manifest["revision"] != REVISION or
            manifest["generator_sha256"] != digest(generator.read_bytes()) or
            manifest["output_sha256"] != OUTPUT_SHA256):
        raise ValueError("Course requires the exact common Original Egg six provider")


def prepare(repository, manifest_path, output_path, evidence_path, native_root=None):
    own_root = Path(__file__).resolve().parent.parent
    native_root = native_root.resolve() if native_root else own_root
    manifest_bytes = manifest_path.read_bytes()
    manifest = json.loads(manifest_bytes)
    if (manifest["schema_version"] != 1 or manifest["revision"] != REVISION or
            manifest["provider"] != "common-egg" or
            manifest["transformation_version"] != VERSION):
        raise ValueError("Common Egg authority/version mismatch")
    output, evidence = output_path.resolve(), evidence_path.resolve()
    parent = output.parent
    protected = (repository.resolve(), *(base / name for base in
                 (own_root, native_root) for name in
                 ("src", "include", "tools", "cmake", "tests", ".git", "graft")))
    if (output.name != "game.c" or evidence.name != "evidence.json" or
            output == evidence or parent != evidence.parent or
            parent.name != "isle-common-egg-private" or parent.parent.name != "_deps" or
            any(parent == base or base in parent.parents for base in protected)):
        raise ValueError("Common Egg requires exact private build output paths")
    if manifest["generator_sha256"] != digest(Path(__file__).read_bytes()):
        raise ValueError("Common Egg generator identity mismatch")
    if canonical(manifest["source_files"]) != SOURCE_SHA256:
        raise ValueError("Common Egg complete pinned source inventory mismatch")
    recipe = {key: manifest[key] for key in RECIPE_KEYS}
    if canonical(recipe) != RECIPE_SHA256:
        raise ValueError("Common Egg complete positive-count rewrite recipe mismatch")
    units = manifest["definitions"]
    selectors = [(u["path"], u["symbol"], u["first"], u["last"]) for u in units]
    if (len(units) != 6 or manifest["definition_count"] != 6 or
            canonical(selectors) != SELECTOR_SHA256):
        raise ValueError("Common Egg requires exactly the six Original definitions")
    if set(manifest["native_support"]) != set(NATIVE_SUPPORT):
        raise ValueError("Common Egg native support allowlist mismatch")
    for path, expected in manifest["native_support"].items():
        selected = (native_root / path).resolve()
        if native_root not in selected.parents or digest(selected.read_bytes()) != expected["sha256"]:
            raise ValueError("Common Egg native support identity mismatch: " + path)
    if git(repository, "cat-file", "-t", REVISION).strip() != b"commit":
        raise ValueError("Common Egg authority is not the pinned commit")
    sources = {}
    for path, expected in manifest["source_files"].items():
        raw = git(repository, "show", REVISION + ":" + path)
        blob = hashlib.sha1(b"blob " + str(len(raw)).encode() + b"\0" + raw).hexdigest()
        if (blob != expected["git_blob"] or digest(raw) != expected["sha256"] or
                len(raw) != expected["bytes"] or len(raw.splitlines()) != expected["line_count"]):
            raise ValueError("Common Egg pinned file identity mismatch: " + path)
        sources[path] = raw.decode()
    for origin in manifest["semantic_origins"]:
        raw = source_span(sources[origin["path"]], origin["first"], origin["last"])
        if digest(raw.encode()) != origin["source_sha256"]:
            raise ValueError("Common Egg semantic Source mismatch: " + origin["label"])
    bodies = []
    for unit in units:
        body = source_span(sources[unit["path"]], unit["first"], unit["last"])
        if digest(body.encode()) != unit["source_sha256"]:
            raise ValueError("Common Egg Original definition mismatch: " + unit["symbol"])
        if not unit["changes"]:
            raise ValueError("Common Egg requires positive counted rewrites: " + unit["symbol"])
        for change in unit["changes"]:
            if change["occurrences"] < 1 or body.count(change["find"]) != change["occurrences"]:
                raise ValueError("Common Egg counted typed rewrite mismatch: " + unit["symbol"])
            body = body.replace(change["find"], change["replace"])
        if (digest(body.encode()) != unit["native_sha256"] or
                body[:body.index("{")].strip() != unit["native_declaration"]):
            raise ValueError("Common Egg transformed definition mismatch: " + unit["symbol"])
        bodies.append(body)
    text = manifest["preamble"] + "\n\n".join(bodies) + "\n"
    if digest(text.encode()) != OUTPUT_SHA256 or manifest["output_sha256"] != OUTPUT_SHA256:
        raise ValueError("Common Egg complete output identity mismatch")
    receipt = {"schema_version": 1, "provider": "common-egg", "revision": REVISION,
               "transformation_version": VERSION, "manifest_sha256": digest(manifest_bytes),
               "generator_sha256": manifest["generator_sha256"], "output_sha256": OUTPUT_SHA256,
               "definition_count": 6, "rewrite_count": sum(len(u["changes"]) for u in units),
               "definitions": units, "native_support": manifest["native_support"],
               "ownership": manifest["ownership"], "execution_qualified": False}
    publish(output, text)
    publish(evidence, json.dumps(receipt, indent=2) + "\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repository", required=True, type=Path)
    parser.add_argument("--manifest", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--evidence", required=True, type=Path)
    parser.add_argument("--native-root", type=Path)
    args = parser.parse_args()
    prepare(args.repository, args.manifest, args.output, args.evidence, args.native_root)


if __name__ == "__main__":
    main()
