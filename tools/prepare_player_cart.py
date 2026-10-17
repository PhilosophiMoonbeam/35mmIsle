#!/usr/bin/env python3
"""Materialize all four complete own-GAME cart bodies into private build storage."""
import argparse
import hashlib
import json
from pathlib import Path
from prepare_engine import git, publish

REVISION = "df8815e75f3d79618466e01e0827c6165b93d11c"
VERSION = "player-cart-owner-complete-v1"
SOURCE = "src/app_level/render_player.c"
SOURCE_BLOB = "a87c007297022d577f629d5850a8ec76ac69a845"
SOURCE_SHA256 = "eab269e561e60f85d7bf3c4f6f76c468e89aa6e8666c4a3f2351d9c97b23ffe8"
SELECTION_SHA256 = "355ad636640e9acb82843296a9ea3cc8c36d335cafad60392f87d1fd17e890d9"
DEFINITIONS = (
    ("PlayerModel_Init", 6, 18),
    ("PlayerModel_SetAnimation", 20, 33),
    ("PlayerModel_ChangeAnimation", 35, 46),
    ("PlayerModel_SetAnimationSpeed", 48, 55),
)
SUPPORT = (
    "src/player_cart_source.h", "src/player_cart_source.c", "src/player_source.h",
    "src/player_cart.hpp", "src/player_cart.cpp",
    "src/player_cart_producer.hpp", "src/player_cart_producer.cpp",
    "src/player_cart_render_binding.hpp", "cmake/SnapPlayerServices.cmake",
)
PREAMBLE = ('/* Four complete pinned own-GAME cart bodies; private build storage only. */\n'
            '#include "player_cart_original.h"\n')
HEADER_PREAMBLE = ('#ifndef ISLE_PLAYER_CART_ORIGINAL_H\n#define ISLE_PLAYER_CART_ORIGINAL_H\n'
                   '#include "player_cart_source.h"\n'
                   '#ifdef __cplusplus\nextern "C" {\n#endif\n')
HEADER_END = '#ifdef __cplusplus\n}\n#endif\n#endif\n'


def digest(data):
    return hashlib.sha256(data).hexdigest()


def prepare(repository, manifest_path, output_path, header_path, evidence_path):
    manifest_bytes = manifest_path.read_bytes()
    manifest = json.loads(manifest_bytes)
    if (manifest["schema_version"] != 1 or manifest["revision"] != REVISION or
            manifest["transformation_version"] != VERSION):
        raise ValueError("cart authority/version mismatch")
    if (manifest["source_path"] != SOURCE or manifest["source_git_blob"] != SOURCE_BLOB or
            manifest["source_sha256"] != SOURCE_SHA256):
        raise ValueError("cart own-GAME source identity mismatch")
    if len({output_path.resolve(), header_path.resolve(), evidence_path.resolve()}) != 3:
        raise ValueError("cart bodies, declarations and evidence must be distinct")
    if git(repository, "cat-file", "-t", REVISION).strip() != b"commit":
        raise ValueError("cart authority is not the pinned commit")
    raw = git(repository, "show", REVISION + ":" + SOURCE)
    blob = hashlib.sha1(b"blob " + str(len(raw)).encode() + b"\0" + raw).hexdigest()
    if blob != SOURCE_BLOB or digest(raw) != SOURCE_SHA256:
        raise ValueError("cart own-GAME Git object identity mismatch")
    lines = raw.decode("utf-8").splitlines(keepends=True)
    if len(lines) != 55:
        raise ValueError("cart complete source length mismatch")
    units = manifest["definitions"]
    selection = [[unit["symbol"], unit["first"], unit["last"]] for unit in units]
    selection_digest = digest(json.dumps(selection, separators=(",", ":")).encode())
    if (len(units) != 4 or selection_digest != SELECTION_SHA256 or
            selection != [list(unit) for unit in DEFINITIONS]):
        raise ValueError("cart requires all four complete original definitions")
    if manifest["generator_sha256"] != digest(Path(__file__).read_bytes()):
        raise ValueError("cart generator identity mismatch")
    if set(manifest["native_support"]) != set(SUPPORT):
        raise ValueError("cart native support allowlist mismatch")
    root = Path(__file__).resolve().parent.parent
    for path, identity in manifest["native_support"].items():
        if digest((root / path).read_bytes()) != identity["sha256"]:
            raise ValueError("cart native support identity mismatch: " + path)
    output = [PREAMBLE]
    declarations = [HEADER_PREAMBLE]
    admitted = []
    for unit in units:
        if unit["path"] != SOURCE:
            raise ValueError("cart definition source mismatch: " + unit["symbol"])
        original = "".join(lines[unit["first"] - 1:unit["last"]])
        if digest(original.encode()) != unit["source_sha256"]:
            raise ValueError("cart complete body identity mismatch: " + unit["symbol"])
        if original[:original.index("{")].strip() != unit["source_declaration"]:
            raise ValueError("cart original declaration mismatch: " + unit["symbol"])
        native = original
        changes = []
        for change in unit["changes"]:
            old, new, count = change["find"], change["replace"], change["occurrences"]
            if not isinstance(count, int) or isinstance(count, bool) or count < 1:
                raise ValueError("cart rewrite requires a positive exact count")
            if not old or native.count(old) != count:
                raise ValueError("cart counted rewrite mismatch: " + unit["symbol"] + ": " + old)
            native = native.replace(old, new)
            changes.append(dict(change, find_sha256=digest(old.encode()),
                                replace_sha256=digest(new.encode())))
        native_digest = digest(native.encode())
        if native_digest != unit["native_sha256"]:
            raise ValueError("cart transformed complete body mismatch: " + unit["symbol"])
        signature = native[:native.index("{")].strip()
        if signature != unit["native_declaration"]:
            raise ValueError("cart native declaration mismatch: " + unit["symbol"])
        output.extend(('#line %d "snap@%s/%s"\n' % (unit["first"], REVISION, SOURCE),
                       native, "\n"))
        declarations.append(signature + ";\n")
        admitted.append(dict(unit, changes=changes, native_sha256=native_digest))
    declarations.append(HEADER_END)
    generated, header = "".join(output), "".join(declarations)
    if (digest(generated.encode()) != manifest["output_sha256"] or
            digest(header.encode()) != manifest["header_sha256"]):
        raise ValueError("cart complete generated output/declaration identity mismatch")
    receipt = dict(manifest, manifest_sha256=digest(manifest_bytes),
                   selection_sha256=selection_digest, transformed_definitions=admitted,
                   output_sha256=digest(generated.encode()), header_sha256=digest(header.encode()))
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
