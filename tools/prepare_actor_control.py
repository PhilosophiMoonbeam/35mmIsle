#!/usr/bin/env python3
"""Materialize selected pinned GAME actor control C only in caller build output."""

import argparse
import hashlib
import json
from pathlib import Path
import subprocess

from prepare_engine import git, publish


REVISION = "df8815e75f3d79618466e01e0827c6165b93d11c"
DEFINITIONS = (
    ("Pokemon_SetState", 252, 263),
    ("Pokemon_SetStateRandom", 265, 288),
    ("Pokemon_StartAuxProc", 291, 296),
    ("Pokemon_StopAuxProc", 298, 303),
    ("Pokemon_StartPathProc", 305, 318),
    ("Pokemon_WaitForFlagNoInteraction", 736, 745),
)
PREAMBLE = (
    '/* Build-private selected GAME actor control; see adjacent evidence. */\n'
    '#include "engine_internal.h"\n#include "actor_source.h"\n\n'
    'typedef IsleActorSource Pokemon;\n'
    'typedef IsleActorRandomState RandomState;\n'
    '#define GET_POKEMON(object) isle_actor_source_require(object)\n'
    '#define omCreateProcess isle_actor_source_create_process\n\n'
)


def prepare(repository, manifest_path, output_path, evidence_path):
    manifest_bytes = manifest_path.read_bytes()
    manifest = json.loads(manifest_bytes)
    if (manifest["schema_version"] != 1 or manifest["revision"] != REVISION or
            manifest["transformation_version"] != "actor-control-v1"):
        raise ValueError("unsupported actor authority/transformation")
    if git(repository, "cat-file", "-t", REVISION).strip() != b"commit":
        raise ValueError("actor authority is not a commit")
    source_paths = {"src/app_level/pokemon.c", "include/enums.h", "include/common_structs.h"}
    if set(manifest["source_files"]) != source_paths:
        raise ValueError("actor source allowlist mismatch")
    sources = {}
    for path, identity in manifest["source_files"].items():
        data = git(repository, "show", REVISION + ":" + path)
        blob = git(repository, "rev-parse", REVISION + ":" + path).decode("ascii").strip()
        if (blob != identity["git_blob"] or hashlib.sha256(data).hexdigest() != identity["sha256"] or
                hashlib.sha1(b"blob " + str(len(data)).encode("ascii") + b"\0" + data).hexdigest() != blob):
            raise ValueError("actor source identity mismatch: " + path)
        sources[path] = data.decode("utf-8").splitlines(keepends=True)
    root = Path(__file__).resolve().parent.parent
    support_paths = {"src/actor_source.h", "src/actor_source_bridge.c", "src/engine.c",
                     "src/engine_internal.h", "include/35mmisle/engine.h",
                     "include/35mmisle/rng.h", "src/rng.c", "tools/prepare_engine.py"}
    if set(manifest["native_support"]) != support_paths:
        raise ValueError("actor native support allowlist mismatch")
    for path, identity in manifest["native_support"].items():
        if hashlib.sha256((root / path).read_bytes()).hexdigest() != identity["sha256"]:
            raise ValueError("actor native support identity mismatch: " + path)
    expected = list(DEFINITIONS)
    actual = [(unit["symbol"], unit["first"], unit["last"]) for unit in manifest["definitions"]]
    if actual != expected:
        raise ValueError("actor definition allowlist mismatch")
    output = [PREAMBLE]
    identities = []
    for unit in manifest["definitions"]:
        if unit["path"] != "src/app_level/pokemon.c":
            raise ValueError("unadmitted actor definition path")
        selected = "".join(sources[unit["path"]][unit["first"] - 1:unit["last"]])
        if hashlib.sha256(selected.encode()).hexdigest() != unit["source_sha256"]:
            raise ValueError("actor definition identity mismatch: " + unit["symbol"])
        expected_changes = ([{"find": "randRange(sumWeight)",
                              "replace": "isle_actor_source_rand_range(obj, sumWeight)"}]
                            if unit["symbol"] == "Pokemon_SetStateRandom" else [])
        if unit["changes"] != expected_changes:
            raise ValueError("unadmitted actor transformation")
        native = selected
        for change in expected_changes:
            if native.count(change["find"]) != 1:
                raise ValueError("actor transformation does not match exactly once")
            native = native.replace(change["find"], change["replace"], 1)
        if hashlib.sha256(native.encode()).hexdigest() != unit["native_sha256"]:
            raise ValueError("actor native definition identity mismatch")
        output.append('#line %d "snap@%s/%s"\n' % (unit["first"], REVISION, unit["path"]))
        output.extend((native, "\n"))
        identities.append({key: unit[key] for key in
                           ("symbol", "path", "first", "last", "source_sha256", "native_sha256")})
    for origin in manifest["semantic_origins"]:
        selected = "".join(sources[origin["path"]][origin["first"] - 1:origin["last"]])
        if hashlib.sha256(selected.encode()).hexdigest() != origin["source_sha256"]:
            raise ValueError("actor semantic origin identity mismatch")
    source = "".join(output)
    evidence = {
        "revision": REVISION,
        "transformation_version": manifest["transformation_version"],
        "manifest_sha256": hashlib.sha256(manifest_bytes).hexdigest(),
        "generator_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        "generated_source_sha256": hashlib.sha256(source.encode()).hexdigest(),
        "source_files": manifest["source_files"], "definitions": identities,
        "semantic_origins": manifest["semantic_origins"],
        "native_support": manifest["native_support"],
        "transformations": manifest["transformations"],
        "scope": "Private selected GAME actor C control; no SDK, ROM, interaction, animation or factory input",
    }
    if output_path.resolve() == evidence_path.resolve():
        raise ValueError("source and evidence paths must differ")
    publish(output_path, source)
    publish(evidence_path, json.dumps(evidence, indent=2) + "\n")
    print("Prepared %d reviewed native actor control definitions at %s" % (len(identities), REVISION))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repository", type=Path, required=True)
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--evidence", type=Path, required=True)
    args = parser.parse_args()
    try:
        prepare(args.repository, args.manifest, args.output, args.evidence)
    except (OSError, ValueError, KeyError, TypeError, subprocess.CalledProcessError) as error:
        parser.exit(1, "Private native actor preparation failed: %s\n" % error)


if __name__ == "__main__":
    main()
