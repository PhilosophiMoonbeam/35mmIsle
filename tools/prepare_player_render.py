#!/usr/bin/env python3
"""Materialize the complete pinned camera tail and light setter, not Root106."""
import argparse
import hashlib
import json
from pathlib import Path
from prepare_engine import git, publish

REVISION = 'df8815e75f3d79618466e01e0827c6165b93d11c'
VERSION = "player-render-complete-v1"
SOURCES = {'src/app_level/player.c': {'git_blob': 'bdca3b88bad38d67936f5956288ae22bd88add95', 'sha256': '229ba02d3a49c41300cb31023847ddcb86fa9ae11ba6f201fce48ead3368298b', 'line_count': 2998}, 'src/app_render/4D400.c': {'git_blob': 'f2ab62d614dffc86bbd413f87150ea68ff473fd3', 'sha256': '67b399176bfd9884cfa3ab929ed3b8565e6503ec88cb5b540f572d3f490927ca', 'line_count': 102}, 'src/app_level/app_level.h': {'git_blob': '953e57fd6e9d76a3962068fb5aefa93d9f55b3d4', 'sha256': 'b95ed8883bc54021ff82df89a6846b16c487e781215eb71f241dbaff194f60e4', 'line_count': 324}, 'include/enums.h': {'git_blob': '18d9da14c9daf13f5bb6a4ec35d25eeefbbdca07', 'sha256': '02b0bfbbc6d010f41039073612a7a031ee2112bdf07fd78cd4b18a17b302e096', 'line_count': 422}, 'src/sys/vector.c': {'git_blob': '99b21e6c073cdbce135abfc10533defc56a05026', 'sha256': 'f36b6f36e6e1434bd498d41df4b03e63775d8f765dc6337f76a7ccfbb5c9fa57', 'line_count': 347}}
SELECTION = [['src/app_level/player.c', 'setMainCameraViewport', 2825, 2843, 'renderer'], ['src/app_level/player.c', 'mainCameraSetScissor', 2845, 2851, 'renderer'], ['src/app_level/player.c', 'mainCameraPreRender', 2853, 2855, 'renderer'], ['src/app_level/player.c', 'fillBorderBlack', 2857, 2870, 'renderer'], ['src/app_level/player.c', 'func_80357FD4_4F83E4', 2872, 2885, 'renderer'], ['src/app_level/player.c', 'mainCameraHandleCmd', 2887, 2891, 'commands'], ['src/app_level/player.c', 'mainCameraUpdate', 2893, 2895, 'commands'], ['src/app_level/player.c', 'mainCameraRender', 2897, 2934, 'renderer'], ['src/app_level/player.c', 'createMainCameras', 2936, 2986, 'renderer'], ['src/app_level/player.c', 'removeMainCamera', 2988, 2990, 'renderer'], ['src/app_level/player.c', 'getMainCameraObj', 2992, 2994, 'renderer'], ['src/app_level/player.c', 'getMainCamera', 2996, 2998, 'renderer'], ['src/app_render/4D400.c', 'func_800A1E6C', 98, 102, 'renderer']]
SELECTION_SHA256 = '86209f6abe58424fceddf91cefc65eaa66c7ee0ac21613bc765e64651c6bf4f6'
RECIPE_SHA256 = '4be2ad71608bb350980336419e36ad00b9052bec1e93888acf2671ab473196d2'
OUTPUT_SHA256 = {'renderer': 'a887453f2d8a97016c69105a261ccf42c4ecac47f0349962bfb014ec78a33739', 'commands': 'e07dde7805141ef8dbba3ae4830594cf5f98b1f85225e46e92867d84ded87b98', 'header': 'ab794165b3df6b2212d6b50afc99e600bee1e34bd2f4db3ba1a3b7a135101f0f'}
SUPPORT = ('src/player_source.h', 'src/player_render_source.h', 'src/player_render_source_bridge.c', 'src/player_commands_source.h', 'src/player_commands_source.c', 'src/player_camera_viewport.h')
RENDER_PREAMBLE = '/* Complete pinned own-GAME Player camera renderer component, not full Root106. */\n#include "player_render_source.h"\n#include <math.h>\n#define SQ(value) ((value) * (value))\n'
COMMAND_PREAMBLE = '/* Canonical complete pinned own-GAME camera commands; C-only, no GPU dependency. */\n#include "player_commands_source.h"\n'
HEADER_PREAMBLE = '#ifndef ISLE_PLAYER_RENDER_ORIGINAL_H\n#define ISLE_PLAYER_RENDER_ORIGINAL_H\n#include "player_render_source.h"\n#ifdef __cplusplus\nextern "C" {\n#endif\n'
HEADER_END = '#ifdef __cplusplus\n}\n#endif\n#endif\n'


def digest(data):
    return hashlib.sha256(data).hexdigest()


def prepare(repository, manifest_path, directory):
    root = Path(__file__).resolve().parent.parent
    directory = directory.resolve()
    if directory.name != "isle-player-render-private" or directory.parent.name != "_deps":
        raise ValueError("renderer requires build _deps/isle-player-render-private")
    protected = (repository.resolve(), *(root / name for name in
                 ("src", "include", "tests", "tools", "cmake", ".git", "graft")))
    if any(directory == base or base in directory.parents for base in protected):
        raise ValueError("renderer outputs must remain outside source trees")
    manifest_bytes = manifest_path.read_bytes()
    manifest = json.loads(manifest_bytes)
    if (manifest["schema_version"] != 1 or manifest["revision"] != REVISION or
            manifest["transformation_version"] != VERSION or
            manifest["scope"] != "complete-camera-render-component-not-root106" or
            manifest["source_files"] != SOURCES):
        raise ValueError("renderer authority/version/scope/source inventory mismatch")
    if manifest["generator_sha256"] != digest(Path(__file__).read_bytes()):
        raise ValueError("renderer generator identity mismatch")
    if set(manifest["native_support"]) != set(SUPPORT):
        raise ValueError("renderer native support allowlist mismatch")
    for path, identity in manifest["native_support"].items():
        if digest((root / path).read_bytes()) != identity["sha256"]:
            raise ValueError("renderer native support identity mismatch: " + path)
    if git(repository, "cat-file", "-t", REVISION).strip() != b"commit":
        raise ValueError("renderer authority is not the pinned commit")
    sources = {}
    for path, identity in SOURCES.items():
        raw = git(repository, "show", REVISION + ":" + path)
        blob = hashlib.sha1(b"blob " + str(len(raw)).encode() + b"\0" + raw).hexdigest()
        lines = raw.decode("utf-8").splitlines(keepends=True)
        if (blob != identity["git_blob"] or digest(raw) != identity["sha256"] or
                len(lines) != identity["line_count"]):
            raise ValueError("renderer immutable source identity mismatch: " + path)
        sources[path] = lines
    units = manifest["definitions"]
    selection = [[u[k] for k in ("path", "symbol", "first", "last", "owner")] for u in units]
    if (len(units) != 13 or selection != SELECTION or
            digest(json.dumps(selection, separators=(",", ":")).encode()) != SELECTION_SHA256 or
            digest(json.dumps(units, sort_keys=True, separators=(",", ":")).encode()) != RECIPE_SHA256):
        raise ValueError("renderer requires all13 exact complete definition recipes")
    output = {"renderer": RENDER_PREAMBLE, "commands": COMMAND_PREAMBLE, "header": HEADER_PREAMBLE}
    admitted = []
    for unit in units:
        original = "".join(sources[unit["path"]][unit["first"] - 1:unit["last"]])
        if (digest(original.encode()) != unit["source_sha256"] or
                original[:original.index("{")].strip() != unit["source_declaration"] or
                not original.endswith("}\n")):
            raise ValueError("renderer complete original definition mismatch: " + unit["symbol"])
        native = original
        changes = []
        for change in unit["changes"]:
            old, new, count = change["find"], change["replace"], change["occurrences"]
            if (not isinstance(count, int) or isinstance(count, bool) or count < 1 or
                    not old or native.count(old) != count):
                raise ValueError("renderer counted rewrite mismatch: " + unit["symbol"] + ": " + old)
            native = native.replace(old, new)
            changes.append(dict(change, find_sha256=digest(old.encode()), replace_sha256=digest(new.encode())))
        signature = native[:native.index("{")].strip()
        if digest(native.encode()) != unit["native_sha256"] or signature != unit["native_declaration"]:
            raise ValueError("renderer complete native definition mismatch: " + unit["symbol"])
        output[unit["owner"]] += '#line %d "snap@%s/%s"\n' % (unit["first"], REVISION, unit["path"]) + native + "\n"
        output["header"] += signature + ";\n"
        admitted.append(dict(unit, changes=changes))
    output["header"] += HEADER_END
    if manifest["output_sha256"] != OUTPUT_SHA256:
        raise ValueError("renderer output inventory mismatch")
    for owner, text in output.items():
        if digest(text.encode()) != OUTPUT_SHA256[owner]:
            raise ValueError("renderer complete output identity mismatch: " + owner)
    receipt = dict(manifest, manifest_sha256=digest(manifest_bytes),
                   selection_sha256=SELECTION_SHA256, recipe_sha256=RECIPE_SHA256,
                   transformed_definitions=admitted)
    # Nothing is published until the entire13-function recipe has been admitted.
    for owner, filename in (("renderer", "player_render_original.c"),
                            ("commands", "player_camera_commands_original.c"),
                            ("header", "player_render_original.h")):
        publish(directory / filename, output[owner])
    publish(directory / "evidence.json", json.dumps(receipt, indent=2) + "\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repository", type=Path, required=True)
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    prepare(args.repository, args.manifest, args.output)


if __name__ == "__main__":
    main()
