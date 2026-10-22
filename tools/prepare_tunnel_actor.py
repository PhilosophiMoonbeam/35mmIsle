#!/usr/bin/env python3
"""Materialize all pinned Tunnel actor Original C importing the once-only Egg provider privately."""
import argparse
import hashlib
import json
from pathlib import Path
from prepare_engine import git, publish
from prepare_common_egg import require_common_provider

REVISION = "df8815e75f3d79618466e01e0827c6165b93d11c"
VERSION = "tunnel-actor-original276-common-egg6-v3"
SOURCE_SHA256 = "c41560260c5c211721425fa47e742eb9881ef1d1c01a496a8dcab94452c1299f"
SELECTOR_SHA256 = "9053ac53944de3170ec43b99ee8122a353d81b2816eac60d972853bdd37143ae"
RECIPE_SHA256 = "da477de58cab92f0289c2ecd696d8975803688fc8aa4632a53ace7e35b3c6c8c"
OUTPUT_SHA256 = "1f4937141d0319a080b0a6a2d36b270d408e58f0b667ddcff85508249ac624c7"
OWN_SUPPORT = ("src/tunnel_actor_source.h", "src/tunnel_actor_source.c",
               "src/tunnel_actor.hpp", "src/tunnel_actor.cpp",
               "tools/prepare_common_egg.py")
COMMON_SUPPORT = ("src/egg_source.h", "src/egg_source.c",
                  "src/course_actor_source.h", "src/course_actor_source.c",
                  "src/actor_owner_source.h", "src/actor_source.h",
                  "src/engine_internal.h", "include/35mmisle/engine.h",
                  "include/35mmisle/rng.h")
RECIPE_KEYS = ("definitions", "semantic_origins", "preamble", "ownership",
               "common_egg_provider")


def digest(raw):
    return hashlib.sha256(raw).hexdigest()


def canonical(value):
    return digest(json.dumps(value, sort_keys=True, separators=(",", ":")).encode())


def source_span(source, first, last):
    return "".join(source.splitlines(keepends=True)[first - 1:last]).rstrip("\r\n")


def prepare(repository, manifest_path, output_path, evidence_path, native_root=None):
    own_root = Path(__file__).resolve().parent.parent
    native_root = native_root.resolve() if native_root else own_root
    manifest_bytes = manifest_path.read_bytes()
    manifest = json.loads(manifest_bytes)
    if (manifest["schema_version"] != 1 or manifest["revision"] != REVISION or
            manifest["provider"] != "tunnel-actor" or
            manifest["transformation_version"] != VERSION):
        raise ValueError("Tunnel actor authority/version mismatch")
    output, evidence = output_path.resolve(), evidence_path.resolve()
    parent = output.parent
    protected = (repository.resolve(), *(base / name for base in
                 (own_root, native_root) for name in
                 ("src", "include", "tools", "cmake", "tests", ".git", "graft")))
    if (output.name != "game.c" or evidence.name != "evidence.json" or
            output == evidence or parent != evidence.parent or
            parent.name != "isle-tunnel-actor-private" or parent.parent.name != "_deps" or
            any(parent == base or base in parent.parents for base in protected)):
        raise ValueError("Tunnel actor requires exact private build output paths")
    if manifest["generator_sha256"] != digest(Path(__file__).read_bytes()):
        raise ValueError("Tunnel actor generator identity mismatch")
    if canonical(manifest["source_files"]) != SOURCE_SHA256:
        raise ValueError("Tunnel actor pinned source inventory mismatch")
    recipe = {key: manifest[key] for key in RECIPE_KEYS}
    if canonical(recipe) != RECIPE_SHA256:
        raise ValueError("Tunnel actor complete counted rewrite recipe mismatch")
    units = manifest["definitions"]
    selectors = [(u["path"], u["symbol"], u["first"], u["last"]) for u in units]
    if len(units) != 276 or canonical(selectors) != SELECTOR_SHA256:
        raise ValueError("Tunnel actor requires all276 local selectors and no compiled Egg copies")
    require_common_provider(own_root, manifest["common_egg_provider"])
    if git(repository, "cat-file", "-t", REVISION).strip() != b"commit":
        raise ValueError("Tunnel actor authority is not the pinned commit")
    sources = {}
    for path, expected in manifest["source_files"].items():
        raw = git(repository, "show", REVISION + ":" + path)
        blob = hashlib.sha1(b"blob " + str(len(raw)).encode() + b"\0" + raw).hexdigest()
        if (blob != expected["git_blob"] or digest(raw) != expected["sha256"] or
                len(raw.splitlines()) != expected["line_count"]):
            raise ValueError("Tunnel actor pinned file identity mismatch: " + path)
        sources[path] = raw.decode()
    for key, allowed, root in (("native_support", OWN_SUPPORT, own_root),
                               ("common_native_support", COMMON_SUPPORT, native_root)):
        if set(manifest[key]) != set(allowed):
            raise ValueError("Tunnel actor native support allowlist mismatch: " + key)
        for path, expected in manifest[key].items():
            selected = (root / path).resolve()
            if root not in selected.parents or digest(selected.read_bytes()) != expected["sha256"]:
                raise ValueError("Tunnel actor native support identity mismatch: " + path)
    for origin in manifest["semantic_origins"]:
        raw = source_span(sources[origin["path"]], origin["first"], origin["last"])
        if digest(raw.encode()) != origin["source_sha256"]:
            raise ValueError("Tunnel actor semantic Source mismatch: " + origin["label"])
    bodies = []
    for unit in units:
        body = source_span(sources[unit["path"]], unit["first"], unit["last"])
        if digest(body.encode()) != unit["source_sha256"]:
            raise ValueError("Tunnel actor original definition mismatch: " + unit["symbol"])
        for change in unit["changes"]:
            if change["occurrences"] < 1 or body.count(change["find"]) != change["occurrences"]:
                raise ValueError("Tunnel actor counted typed rewrite mismatch: " + unit["symbol"])
            body = body.replace(change["find"], change["replace"])
        if (digest(body.encode()) != unit["native_sha256"] or
                body[:body.index("{")].strip() != unit["native_declaration"]):
            raise ValueError("Tunnel actor transformed definition mismatch: " + unit["symbol"])
        bodies.append(body)
    text = manifest["preamble"] + "\n\n".join(bodies) + "\n"
    if digest(text.encode()) != OUTPUT_SHA256 or manifest["output_sha256"] != OUTPUT_SHA256:
        raise ValueError("Tunnel actor complete output identity mismatch")
    receipt = {"schema_version": 1, "revision": REVISION,
               "transformation_version": VERSION,
               "manifest_sha256": digest(manifest_bytes),
               "generator_sha256": manifest["generator_sha256"],
               "output_sha256": OUTPUT_SHA256,
               "definition_count": len(units), "local_definition_count": 276,
               "shared_definition_count": 0,
               "common_egg_provider": manifest["common_egg_provider"],
               "rewrite_count": sum(len(u["changes"]) for u in units),
               "execution_qualified": False,
               "qualification_blockers": manifest["qualification_blockers"],
               "definitions": [{key: u[key] for key in
                                ("path", "symbol", "first", "last", "source_sha256",
                                 "native_sha256", "source_abi", "native_declaration")}
                               for u in units],
               "native_support": manifest["native_support"],
               "common_native_support": manifest["common_native_support"],
               "ownership": manifest["ownership"]}
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
