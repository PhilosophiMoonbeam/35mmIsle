#!/usr/bin/env python3
"""Materialize exact106 Player ownership: 94 Root/support, 10 renderer, 2 commands."""
import argparse
import hashlib
import json
import re
from pathlib import Path
from prepare_engine import git, publish

REVISION = "df8815e75f3d79618466e01e0827c6165b93d11c"
SOURCE = "src/app_level/player.c"
SOURCE_BLOB = "bdca3b88bad38d67936f5956288ae22bd88add95"
SOURCE_SHA256 = "229ba02d3a49c41300cb31023847ddcb86fa9ae11ba6f201fce48ead3368298b"
SELECTION_SHA256 = "8bb75b6e1a0d436854c9d0787ee9667de4cd628d4190b8ff21ac9e567d716a84"
VERSION = "player-root-complete106-v1"
SOURCE_INVENTORY_SHA256 = "162f71bade9b13002b64faa26c1689ce055e4601f6912d84704dee1b32e22b55"
RECIPE_SHA256 = "b20092986b6fa45acb126bd879f8f91652c1e7a5ff5ec24458e49efc56e0ee41"
OUTPUT_SHA256 = {
    "root": "203c3d0a3b70c8607b3535196c182dcebc24fc09e5d860f3cc84d5e864571b84",
    "support": "28e2dd821d5ee132ea11c87146547ef746cce5b932030d946a98d9eef9b25f50",
    "header": "0b041c5f39e3253d93ef762f7c13c5f6067cfc8e384124d972ccf92a2700eb22",
}
SUPPORT = (
    "src/player_root_source.h", "src/player_root_source.c", "src/player_source.h",
    "src/player_source_bridge.c", "src/engine_internal.h", "src/engine_math.h",
    "src/actor_source.h", "src/player_ui_source.h", "src/player_photo_source.h",
    "src/player_bump_source.h", "src/player_render_source.h", "src/player_commands_source.h",
    "src/world_movement_state.h", "src/player_camera_animation_clip.h",
    "include/35mmisle/sprite.h", "include/35mmisle/engine.h", "cmake/player-render-inputs.json",
    "src/player_root.hpp", "src/player_root.cpp",
    "src/scene_services.hpp", "src/scene_services.cpp",
    "src/game_state.hpp", "src/game_state.cpp",
    "src/application_source.h",
    "src/game_audio.hpp", "src/game_audio.cpp",
    "src/player_ui_resources.hpp", "src/player_ui_resources.cpp",
    "include/35mmisle/game_save.hpp", "src/game_save.cpp",
    "src/screen_transition_source.h", "tests/player_root_rsp_smoke.cpp",
    "tests/source_camera_contract.hpp",
)

PREAMBLE = '''/* Private complete own-GAME Player Root bodies; no SDK implementation. */
#include "engine_internal.h"
#include "actor_source.h"
#include "player_root_source.h"
#include "player_root_original.h"
#include "player_ui_source.h"
#include "player_photo_source.h"
#include "player_bump_source.h"
#include "player_render_source.h"
#include <stdio.h>
#define ABS(x) ((x) < 0 ? -(x) : (x))
#define SIGN(x) ((x) >= 0 ? 1 : -1)
#define SQ(x) ((x) * (x))
#define PI 3.14159265358979323846
#define TAU (2.0 * PI)
#define PI_2 (PI / 2.0)
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define A_BUTTON 0x8000u
#define B_BUTTON 0x4000u
#define START_BUTTON 0x1000u
#define CONT_START START_BUTTON
#define Z_TRIG 0x2000u
#define R_TRIG 0x0010u
#define U_CBUTTONS 0x0008u
#define D_CBUTTONS 0x0004u
#define L_CBUTTONS 0x0002u
#define R_CBUTTONS 0x0001u
#define CAMERA_CMD_BLINK 1
#define GOBJ_FLAG_HIDDEN 1u
'''


def digest(data):
    return hashlib.sha256(data).hexdigest()


def rewrite(original, unit):
    native = original
    for change in unit["changes"]:
        count = change["occurrences"]
        if count < 1 or native.count(change["find"]) != count:
            raise ValueError("Player rewrite count mismatch: " + unit["symbol"] + ": " + change["find"])
        native = native.replace(change["find"], change["replace"])
    if digest(native.encode()) != unit["native_sha256"]:
        raise ValueError("Player transformed identity mismatch: " + unit["symbol"])
    return native


def prepare(repository, manifest_path, output_path, header_path, support_path, evidence_path):
    manifest_bytes = manifest_path.read_bytes()
    manifest = json.loads(manifest_bytes)
    root = Path(__file__).resolve().parent.parent
    if (manifest["revision"] != REVISION or manifest["source_path"] != SOURCE or
            manifest["transformation_version"] != VERSION or manifest["schema_version"] != 1):
        raise ValueError("unadmitted Player Root authority")
    paths = [path.resolve() for path in (output_path, header_path, support_path, evidence_path)]
    parent = paths[0].parent
    if (len(set(paths)) != 4 or any(path.parent != parent for path in paths) or
            parent.name != "isle-player-root-private" or parent.parent.name != "_deps" or
            tuple(path.name for path in paths) != ("player_root_original.c", "player_root_original.h",
                "player_root_support.c", "evidence.json")):
        raise ValueError("Player Root outputs require exact private _deps paths")
    protected = (repository.resolve(), *(root / name for name in
                 ("src", "include", "tests", "tools", "cmake", ".git", "graft")))
    if any(parent == base or base in parent.parents for base in protected):
        raise ValueError("Player Root outputs must remain outside source trees")
    if digest(Path(__file__).read_bytes()) != manifest["generator_sha256"]:
        raise ValueError("Player Root generator identity mismatch")
    if digest(json.dumps(manifest["source_files"], sort_keys=True,
            separators=(",", ":")).encode()) != SOURCE_INVENTORY_SHA256:
        raise ValueError("Player Root source inventory mismatch")
    recipe = {key: manifest[key] for key in ("definitions", "immutable_data", "constant_names")}
    if digest(json.dumps(recipe, sort_keys=True, separators=(",", ":")).encode()) != RECIPE_SHA256:
        raise ValueError("Player Root complete counted recipe identity mismatch")
    if set(manifest["native_support"]) != set(SUPPORT):
        raise ValueError("Player Root native support allowlist mismatch")
    if git(repository, "cat-file", "-t", REVISION).strip() != b"commit":
        raise ValueError("Player Root authority is not the pinned commit")
    files = {}
    for path, identity in manifest["source_files"].items():
        raw = git(repository, "show", REVISION + ":" + path)
        blob = hashlib.sha1(b"blob " + str(len(raw)).encode() + b"\0" + raw).hexdigest()
        if blob != identity["git_blob"] or digest(raw) != identity["sha256"] or len(raw.splitlines()) != identity["line_count"]:
            raise ValueError("Player pinned source identity mismatch: " + path)
        files[path] = raw.decode()
    if (manifest["source_files"][SOURCE]["git_blob"] != SOURCE_BLOB or
            manifest["source_files"][SOURCE]["sha256"] != SOURCE_SHA256):
        raise ValueError("Player source pin mismatch")
    units = manifest["definitions"]
    selection = [[unit["symbol"], unit["first"], unit["last"]] for unit in units]
    selection_digest = digest(json.dumps(selection, separators=(",", ":")).encode())
    if len(units) != 106 or len({u["symbol"] for u in units}) != 106 or selection_digest != SELECTION_SHA256:
        raise ValueError("Player requires complete unique 106-definition coverage")
    counts = {owner: sum(u["owner"] == owner for u in units) for owner in ("root", "support", "renderer", "commands")}
    if counts != {"root": 92, "support": 2, "renderer": 10, "commands": 2}:
        raise ValueError("Player canonical ownership partition mismatch")
    for path, identity in manifest["native_support"].items():
        selected = (root / path).resolve()
        if root not in selected.parents or selected.suffix not in (".h", ".c", ".hpp", ".cpp", ".json"):
            raise ValueError("unadmitted Player native support path: " + path)
        if digest(selected.read_bytes()) != identity["sha256"]:
            raise ValueError("Player native support identity mismatch: " + path)
    renderer = json.loads((root / "cmake/player-render-inputs.json").read_bytes())
    external = {u["symbol"]: u for u in renderer["definitions"] if u["path"] == SOURCE}
    declarations = ['#ifndef ISLE_PLAYER_ROOT_ORIGINAL_H\n#define ISLE_PLAYER_ROOT_ORIGINAL_H\n',
                    '#include "player_root_source.h"\n#ifdef __cplusplus\nextern "C" {\n#endif\n']
    core = [PREAMBLE]
    support = ['/* Canonical pure Player support; independent of Root native domains. */\n',
               '#include "player_root_source.h"\n#include <stdbool.h>\n']
    evidence = []
    lines = files[SOURCE].splitlines(keepends=True)
    for unit in units:
        original = "".join(lines[unit["first"] - 1:unit["last"]])
        if digest(original.encode()) != unit["source_sha256"]:
            raise ValueError("Player complete source body mismatch: " + unit["symbol"])
        if unit["owner"] in ("renderer", "commands"):
            other = external.get(unit["symbol"])
            if other is None or any(unit[key] != other[key] for key in
                    ("first", "last", "owner", "source_sha256", "native_sha256")):
                raise ValueError("Player external canonical owner mismatch: " + unit["symbol"])
            evidence.append(unit)
            continue
        native = rewrite(original, unit)
        signature = native[:native.index("{")].strip()
        if signature != unit["native_declaration"]:
            raise ValueError("Player native declaration mismatch: " + unit["symbol"])
        if not signature.startswith("static "):
            declarations.append(signature.replace("Mtx3f m", "float m[3][3]") + ";\n")
        target = support if unit["owner"] == "support" else core
        target.extend(['#line %d "snap@%s/%s"\n' % (unit["first"], REVISION, SOURCE), native, "\n"])
        evidence.append(unit)
    immutable = []
    constants = {}
    for path in ("include/enums.h", "src/app_level/app_level.h", "include/sys/audio.h", SOURCE):
        for block in re.findall(r"enum\s+\w+\s*\{(.*?)\};", files[path], re.S):
            value = -1
            for part in re.sub(r"//[^\n]*", "", block).split(","):
                match = re.fullmatch(r"\s*(\w+)(?:\s*=\s*(0x[\dA-Fa-f]+|\d+))?\s*", part)
                if match:
                    value = int(match[2], 0) if match[2] else value + 1
                    if match[1] in manifest["constant_names"]:
                        constants[match[1]] = value
    if len(constants) != len(manifest["constant_names"]):
        raise ValueError("Player constant source coverage mismatch")
    data = ['enum {\n' + ''.join('    %s = %d,\n' % pair for pair in sorted(constants.items())) + '};\n']
    for unit in manifest["immutable_data"]:
        original = "".join(files[unit["path"]].splitlines(keepends=True)[unit["first"] - 1:unit["last"]])
        if digest(original.encode()) != unit["source_sha256"]:
            raise ValueError("Player immutable data source mismatch: " + unit["symbol"])
        native = rewrite(original, unit)
        data.extend(['#line %d "snap@%s/%s"\n' % (unit["first"], REVISION, unit["path"]), native, "\n"])
        immutable.append(unit)
    core.insert(1, ''.join(data))
    declarations.append('#ifdef __cplusplus\n}\n#endif\n#endif\n')
    generated = {"root": ''.join(core), "header": ''.join(declarations), "support": ''.join(support)}
    hashes = {key: digest(value.encode()) for key, value in generated.items()}
    if hashes != OUTPUT_SHA256 or manifest["output_sha256"] != OUTPUT_SHA256:
        raise ValueError("Player Root complete output identity mismatch")
    receipt = dict(revision=REVISION, scope="COMPLETE106_SOURCE_OWNERSHIP_NOT_NATIVE_ROOT_RUNTIME_QUALIFICATION",
        source_files=manifest["source_files"], manifest_sha256=digest(manifest_bytes),
        generator_sha256=digest(Path(__file__).read_bytes()), selection_sha256=selection_digest,
        recipe_sha256=RECIPE_SHA256, source_inventory_sha256=SOURCE_INVENTORY_SHA256,
        ownership_counts=counts, native_support=manifest["native_support"],
        definitions=evidence, immutable_data=immutable, output_sha256=hashes,
        prior_authoring_tool=manifest["prior_authoring_tool"])
    for path, key in ((output_path, "root"), (header_path, "header"), (support_path, "support")):
        publish(path, generated[key])
    publish(evidence_path, json.dumps(receipt, indent=2) + "\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("repository", "manifest", "output", "header", "support", "evidence"):
        parser.add_argument("--" + name, type=Path, required=True)
    args = parser.parse_args()
    prepare(args.repository, args.manifest, args.output, args.header, args.support, args.evidence)


if __name__ == "__main__":
    main()
