#!/usr/bin/env python3
"""Prepare one complete pinned own-GAME empty command witness for private tests."""
import argparse
import hashlib
import json
from pathlib import Path
from prepare_engine import git, publish

REVISION = "df8815e75f3d79618466e01e0827c6165b93d11c"
VERSION = "player-command-reference-complete-v2"
SOURCE = "src/app_level/player.c"
SOURCE_BLOB = "bdca3b88bad38d67936f5956288ae22bd88add95"
SOURCE_SHA256 = "229ba02d3a49c41300cb31023847ddcb86fa9ae11ba6f201fce48ead3368298b"
SELECTION_SHA256 = "a0e34ba57f6c6524c2bf8860d1abfb5f58a3e5afcf051828080cde50d005bb51"
RECIPE_SHA256 = "6f729069fbe953b0c14e1690bcce6c0fdf375139a46d6339d30fb3785e2c9068"
OUTPUT_SHA256 = "aee9e4c6a8635f24ccba443c663c1308ed7e88a250138fc071869a6f03a0f9dd"
HEADER_SHA256 = "22076a0d19efcb5d9faa866930a10fe8f8162307aef2481cc1b476567ca87557"
DEFINITIONS = (
    ("func_80356118_4F6528", 2282, 2283),
)
SUPPORT = (
    "src/player_commands_source.h", "src/player_commands_source.c",
    "src/player_source.h", "src/player_source_bridge.c",
    "src/player_commands.hpp", "src/player_commands.cpp",
    "src/source_command_scope.h",
    "src/scene_services.hpp", "src/scene_services.cpp",
    "src/menu_scene_bridge.h", "src/menu_scene_bridge.c", "src/engine.c",
    "src/engine_internal.h", "include/35mmisle/engine.h",
    "src/world_movement_state.h", "cmake/SnapPlayerServices.cmake",
    "tools/prepare_engine.py",
)
PREAMBLE = ('/* One complete pinned own-GAME empty command witness; test-private only. */\n'
            '#include "player_command_reference.h"\n')
HEADER_PREAMBLE = ('#ifndef ISLE_PLAYER_COMMAND_REFERENCE_H\n'
                   '#define ISLE_PLAYER_COMMAND_REFERENCE_H\n'
                   '#include "player_commands_source.h"\n'
                   '#ifdef __cplusplus\nextern "C" {\n#endif\n')
HEADER_END = '#ifdef __cplusplus\n}\n#endif\n#endif\n'


def digest(data):
    return hashlib.sha256(data).hexdigest()


def read_source(repository, path, blob, sha256, line_count):
    raw = git(repository, "show", REVISION + ":" + path)
    actual_blob = hashlib.sha1(b"blob " + str(len(raw)).encode() + b"\0" + raw).hexdigest()
    if actual_blob != blob or digest(raw) != sha256:
        raise ValueError("command witness own-GAME object mismatch: " + path)
    lines = raw.decode("utf-8").splitlines(keepends=True)
    if len(lines) != line_count:
        raise ValueError("command witness complete source length mismatch: " + path)
    return lines


def private_outputs(root, repository, output_path, header_path, evidence_path):
    paths = tuple(path.resolve() for path in (output_path, header_path, evidence_path))
    if len(set(paths)) != 3 or len({path.parent for path in paths}) != 1:
        raise ValueError("command witness outputs require three distinct files in one private directory")
    parent = paths[0].parent
    if (parent.name != "isle-player-command-reference-private" or parent.parent.name != "_deps" or
            tuple(path.name for path in paths) !=
            ("player_command_reference.inc", "player_command_reference.h", "evidence.json")):
        raise ValueError("command witness outputs require the private build _deps directory and filenames")
    protected = (repository.resolve(), *(root / name for name in
                 ("src", "include", "tests", "tools", "cmake", ".git", "graft")))
    if any(path == base or base in path.parents for path in paths for base in protected):
        raise ValueError("command witnesses must not be published into a source tree")


def prepare(repository, manifest_path, output_path, header_path, evidence_path):
    root = Path(__file__).resolve().parent.parent
    private_outputs(root, repository, output_path, header_path, evidence_path)
    manifest_bytes = manifest_path.read_bytes()
    manifest = json.loads(manifest_bytes)
    if (manifest["schema_version"] != 1 or manifest["revision"] != REVISION or
            manifest["transformation_version"] != VERSION or
            manifest["scope"] != "private-test-only"):
        raise ValueError("command witness authority/version/scope mismatch")
    expected_sources = {
        SOURCE: {"git_blob": SOURCE_BLOB, "sha256": SOURCE_SHA256, "line_count": 2998},
    }
    if manifest["source_files"] != expected_sources:
        raise ValueError("command witness exact own-GAME source inventory mismatch")
    if git(repository, "cat-file", "-t", REVISION).strip() != b"commit":
        raise ValueError("command witness authority is not the pinned commit")
    sources = {path: read_source(repository, path, identity["git_blob"], identity["sha256"],
                                 identity["line_count"])
               for path, identity in expected_sources.items()}
    if manifest["generator_sha256"] != digest(Path(__file__).read_bytes()):
        raise ValueError("command witness generator identity mismatch")
    if set(manifest["native_support"]) != set(SUPPORT):
        raise ValueError("command witness native support allowlist mismatch")
    for path, identity in manifest["native_support"].items():
        if digest((root / path).read_bytes()) != identity["sha256"]:
            raise ValueError("command witness native support identity mismatch: " + path)
    units = manifest["definitions"]
    selection = [[unit["symbol"], unit["first"], unit["last"]] for unit in units]
    selection_digest = digest(json.dumps(selection, separators=(",", ":")).encode())
    if (len(units) != 1 or selection != [list(unit) for unit in DEFINITIONS] or
            selection_digest != SELECTION_SHA256 or
            digest(json.dumps(units, sort_keys=True, separators=(",", ":")).encode()) != RECIPE_SHA256):
        raise ValueError("command witness requires the one complete frozen empty-handler recipe")
    output = [PREAMBLE]
    declarations = [HEADER_PREAMBLE]
    admitted = []
    for unit in units:
        if unit["path"] != SOURCE:
            raise ValueError("command witness function source mismatch: " + unit["symbol"])
        original = "".join(sources[SOURCE][unit["first"] - 1:unit["last"]])
        if (digest(original.encode()) != unit["source_sha256"] or
                original[:original.index("{")].strip() != unit["source_declaration"] or
                not original.endswith("}\n")):
            raise ValueError("command witness complete original body/declaration mismatch: " + unit["symbol"])
        native = original
        changes = []
        for change in unit["changes"]:
            old, new, count = change["find"], change["replace"], change["occurrences"]
            if not isinstance(count, int) or isinstance(count, bool) or count < 1:
                raise ValueError("command witness rewrite requires a positive exact count")
            if not old or native.count(old) != count:
                raise ValueError("command witness counted rewrite mismatch: " + unit["symbol"] + ": " + old)
            native = native.replace(old, new)
            changes.append(dict(change, find_sha256=digest(old.encode()),
                                replace_sha256=digest(new.encode())))
        signature = native[:native.index("{")].strip()
        native_digest = digest(native.encode())
        if native_digest != unit["native_sha256"] or signature != unit["native_declaration"]:
            raise ValueError("command witness transformed complete body/declaration mismatch: " + unit["symbol"])
        output.extend(('#line %d "snap@%s/%s"\n' % (unit["first"], REVISION, SOURCE), native, "\n"))
        declarations.append(signature + ";\n")
        admitted.append(dict(unit, changes=changes, native_sha256=native_digest))
    declarations.append(HEADER_END)
    generated, header = "".join(output), "".join(declarations)
    if (digest(generated.encode()) != OUTPUT_SHA256 or manifest["output_sha256"] != OUTPUT_SHA256 or
            digest(header.encode()) != HEADER_SHA256 or manifest["header_sha256"] != HEADER_SHA256):
        raise ValueError("command witness complete generated source/header identity mismatch")
    receipt = dict(manifest, manifest_sha256=digest(manifest_bytes),
                   selection_sha256=selection_digest, recipe_sha256=RECIPE_SHA256,
                   transformed_definitions=admitted, output_sha256=OUTPUT_SHA256,
                   header_sha256=HEADER_SHA256)
    # Validate the entire selection before replacing any previously admitted output.
    publish(output_path, generated)
    publish(header_path, header)
    publish(evidence_path, json.dumps(receipt, indent=2) + "\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("repository", "manifest", "output", "header", "evidence"):
        parser.add_argument("--" + name, type=Path, required=True)
    args = parser.parse_args()
    prepare(args.repository, args.manifest, args.output, args.header, args.evidence)


if __name__ == "__main__":
    main()
