#!/usr/bin/env python3
"""Materialize the complete pinned River Original C river-lifecycle closure privately."""
import argparse
import hashlib
import json
from pathlib import Path
from prepare_engine import git, publish
REVISION = 'df8815e75f3d79618466e01e0827c6165b93d11c'
PROVIDER = 'river-lifecycle'
VERSION = 'river-lifecycle-complete20-v1'
SOURCE_INVENTORY_SHA256 = 'afc2f4d9c3c4156279964fab53e4dcb57c6d6e9eb3eb43d995ae23ad23112167'
RECIPE_SHA256 = 'df1caf35355d5cece94f3b2f9cfb7dd20a12c5f16ea1ad6f7e1a619353900b74'
OUTPUT_SHA256 = 'dfd0567cc425981be1d01c1f7eb0636a69d0448c0a172c82711483f2ffcb15aa'
EXPECTED = (('src/river/6C05E0.c', 'func_802D8800_6C05E0', 25, 27), ('src/river/6C05E0.c', 'func_802D8824_6C0604', 29, 31), ('src/river/6C05E0.c', 'func_802D8848_6C0628', 33, 35), ('src/river/6C05E0.c', 'func_802D886C_6C064C', 37, 42), ('src/river/6C05E0.c', 'func_802D88F8_6C06D8', 44, 55), ('src/river/6C05E0.c', 'func_802D895C_6C073C', 57, 64), ('src/river/6C05E0.c', 'func_802D89AC_6C078C', 66, 67), ('src/river/6C05E0.c', 'func_802D89B4_6C0794', 69, 83), ('src/river/6C05E0.c', 'func_802D8FCC_6C0DAC', 263, 280), ('src/river/6C05E0.c', 'func_802D90B8_6C0E98', 282, 305), ('src/river/6C05E0.c', 'func_802D9208_6C0FE8', 307, 308), ('src/river/6C05E0.c', 'func_802D9210_6C0FF0', 310, 327), ('src/river/6C1090.c', 'func_802D92B0_6C1090', 6, 49), ('src/river/6C9EB0.c', 'func_802E20D0_6C9EB0', 20, 24), ('src/river/6C9EB0.c', 'func_802E2100_6C9EE0', 26, 28), ('src/river/6C9EB0.c', 'func_802E2120_6C9F00', 30, 46), ('src/river/6C9EB0.c', 'func_802E2194_6C9F74', 48, 59), ('src/river/6C9EB0.c', 'func_802E222C_6CA00C', 61, 74), ('src/river/6C9EB0.c', 'func_802E22E4_6CA0C4', 76, 140), ('src/river/6C9EB0.c', 'func_802E25C4_6CA3A4', 142, 151))
RECIPE_KEYS = ('definitions', 'adapters', 'semantic_origins', 'ownership', 'source_selector_partition', 'typed_services', 'evaluated_heap_symbols', 'preamble', 'postamble')
SUPPORT = ('cmake/actor-control-inputs.json', 'cmake/player-root-inputs.json', 'include/35mmisle/continuation.h', 'include/35mmisle/engine.h', 'include/35mmisle/rng.h', 'src/actor_owner_source.h', 'src/actor_source.h', 'src/course_actor_source.c', 'src/course_actor_source.h', 'src/engine_internal.h', 'src/player_root_source.c', 'src/player_root_source.h', 'src/player_source.h', 'src/river_actor.cpp', 'src/river_actor.hpp', 'src/river_actor_source.c', 'src/river_actor_source.h', 'src/river_lifecycle_source.c', 'src/river_lifecycle_source.h', 'src/source_command_scope.h', 'tools/prepare_engine.py')

def digest(data):
    return hashlib.sha256(data).hexdigest()

def canonical(value):
    return digest(json.dumps(value, sort_keys=True, separators=(",", ":")).encode())

def prepare(repository, manifest_path, output_path, evidence_path):
    raw_manifest = manifest_path.read_bytes()
    manifest = json.loads(raw_manifest)
    root = Path(__file__).resolve().parent.parent
    output, evidence = output_path.resolve(), evidence_path.resolve()
    parent = output.parent
    protected = (repository.resolve(), *(root / name for name in ("src", "include", "tools", "cmake", "tests", ".git", "graft")))
    if (output.name != "game.c" or evidence.name != "evidence.json" or output == evidence or
            parent != evidence.parent or parent.name != "isle-" + PROVIDER + "-private" or
            parent.parent.name != "_deps" or any(parent == p or p in parent.parents for p in protected)):
        raise ValueError("River requires exact build-private output paths")
    if (manifest["schema_version"] != 1 or manifest["revision"] != REVISION or
            manifest["provider"] != PROVIDER or manifest["transformation_version"] != VERSION):
        raise ValueError("River authority/version mismatch")
    if manifest["generator_sha256"] != digest(Path(__file__).read_bytes()):
        raise ValueError("River materializer identity mismatch")
    if canonical(manifest["source_files"]) != SOURCE_INVENTORY_SHA256:
        raise ValueError("River complete pinned source inventory mismatch")
    if canonical({k: manifest[k] for k in RECIPE_KEYS}) != RECIPE_SHA256:
        raise ValueError("River complete typed transformation recipe mismatch")
    if set(manifest["native_support"]) != set(SUPPORT):
        raise ValueError("River native support allowlist mismatch")
    if git(repository, "cat-file", "-t", REVISION).strip() != b"commit":
        raise ValueError("River authority is not the pinned commit")
    sources = {}
    for path, identity in manifest["source_files"].items():
        raw = git(repository, "show", REVISION + ":" + path)
        blob = hashlib.sha1(b"blob " + str(len(raw)).encode() + b"\0" + raw).hexdigest()
        if (blob != identity["git_blob"] or digest(raw) != identity["sha256"] or
                len(raw.splitlines()) != identity["line_count"]):
            raise ValueError("River pinned source identity mismatch: " + path)
        sources[path] = raw.decode()
    for path, identity in manifest["native_support"].items():
        selected = (root / path).resolve()
        if root not in selected.parents or digest(selected.read_bytes()) != identity["sha256"]:
            raise ValueError("River native support identity mismatch: " + path)
    definitions = manifest["definitions"]
    if tuple((u["path"], u["symbol"], u["first"], u["last"]) for u in definitions) != EXPECTED:
        raise ValueError("River complete unique selector closure mismatch")
    for origin in manifest["semantic_origins"]:
        text = "".join(sources[origin["path"]].splitlines(keepends=True)[origin["first"]-1:origin["last"]])
        if digest(text.encode()) != origin["source_sha256"]:
            raise ValueError("River semantic declaration authority mismatch: " + origin["path"])
    bodies = []
    for unit in definitions:
        body = "".join(sources[unit["path"]].splitlines(keepends=True)[unit["first"]-1:unit["last"]]).rstrip("\n")
        if digest(body.encode()) != unit["source_sha256"]:
            raise ValueError("River original selector mismatch: " + unit["symbol"])
        for change in unit["changes"]:
            if "offset" in change:
                first = change["offset"]
                last = first + len(change["find"])
                if body[first:last] != change["find"]:
                    raise ValueError("River positional typed rewrite mismatch: " + unit["symbol"])
                body = body[:first] + change["replace"] + body[last:]
            else:
                if change["occurrences"] < 1 or body.count(change["find"]) != change["occurrences"]:
                    raise ValueError("River counted typed rewrite mismatch: " + unit["symbol"])
                body = body.replace(change["find"], change["replace"])
        if (digest(body.encode()) != unit["native_sha256"] or
                body[:body.index("{")].strip() != unit["native_declaration"]):
            raise ValueError("River transformed Source definition mismatch: " + unit["symbol"])
        bodies.append(body)
    text = manifest["preamble"] + "\n\n".join(bodies) + manifest["postamble"]
    if digest(text.encode()) != OUTPUT_SHA256 or manifest["output_sha256"] != OUTPUT_SHA256:
        raise ValueError("River complete generated output identity mismatch")
    receipt = {"schema_version": 1, "provider": PROVIDER, "revision": REVISION,
        "transformation_version": VERSION, "manifest_sha256": digest(raw_manifest),
        "generator_sha256": manifest["generator_sha256"], "output_sha256": OUTPUT_SHA256,
        "definition_count": len(definitions), "native_adapter_count": len(manifest["adapters"]),
        "rewrite_count": sum(len(u["changes"]) for u in definitions),
        "definitions": [{k: u[k] for k in ("path", "symbol", "source_symbol", "first", "last", "source_sha256", "native_sha256", "source_abi", "native_abi")} for u in definitions],
        "source_selector_partition": manifest["source_selector_partition"],
        "native_support": manifest["native_support"], "ownership": manifest["ownership"],
        "typed_services": manifest["typed_services"], "adapters": manifest["adapters"]}
    publish(output, text)
    publish(evidence, json.dumps(receipt, indent=2) + "\n")

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repository", required=True, type=Path)
    parser.add_argument("--manifest", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--evidence", required=True, type=Path)
    args = parser.parse_args()
    prepare(args.repository, args.manifest, args.output, args.evidence)

if __name__ == "__main__":
    main()
