#!/usr/bin/env python3
"""Materialize complete pinned own-GAME Scyther/special bodies privately."""
import argparse
import hashlib
import json
from pathlib import Path
from prepare_engine import git, publish

REVISION = "df8815e75f3d79618466e01e0827c6165b93d11c"
VERSION = "beach-scyther-special-complete22-v1"
SOURCE_INVENTORY_SHA256 = 'c94995a72bbc95b0c9f406316d2c611d02a37de3be65f0cc48639aa9071f37bb'
RECIPE_SHA256 = 'f40702f183001741fdbfc67ac5bce5ae3d69db9a3cc5f4b547892b6599a7cb70'
OUTPUT_SHA256 = '2cdd62d247d7cd9c6868e8475da88d24fbdedee48cb16aebac52f8e8917ad1bb'
SUPPORT = ('src/beach_scyther_special_source.h', 'src/beach_scyther_special_source.c', 'src/beach_scyther_special.hpp', 'src/beach_scyther_special.cpp', 'src/actor_owner.hpp', 'src/actor_owner.cpp', 'src/actor_owner_source.h', 'src/actor_source.h', 'src/actor_source_owner_bridge.c', 'src/actor_interaction_services.hpp', 'src/actor_interaction_services.cpp', 'src/actor_interaction_services.h', 'src/actor_behavior_bindings.hpp', 'src/actor_behavior_bindings.cpp', 'src/beach_initial_species_source.h', 'src/actor_resources.hpp', 'src/actor_resource_catalog.hpp', 'src/beach_actor_scene.hpp', 'src/actor_resources.cpp', 'src/actor_resource_catalog.cpp', 'src/beach_actor_catalog.cpp', 'src/rainbow_actor_catalog.cpp', 'src/beach_actor_scene.cpp', 'src/actor_behavior_resources.hpp', 'src/actor_behavior_resources.cpp', 'src/environment_sound.hpp', 'src/environment_sound.cpp', 'src/engine_internal.h', 'tools/prepare_engine.py', 'src/actor_course_context.hpp', 'src/rainbow_actor_source.h', 'src/rainbow_actor_source.c', 'src/rainbow_actor.hpp', 'src/rainbow_actor.cpp', 'src/rainbow_lifecycle_source.h', 'src/player_root.hpp')
EXPECTED = (('src/beach/560620.c', 'func_beach_802C8D74', 234, 236), ('src/beach/560E20.c', 'func_beach_802C8DB0', 4, 11), ('src/beach/560E20.c', 'func_beach_802C8DE8', 13, 16), ('src/beach/560E20.c', 'func_beach_802C8E20', 18, 27), ('src/beach/560E20.c', 'func_beach_802C8E90', 30, 42), ('src/beach/560E20.c', 'func_beach_802C8F48', 44, 68), ('src/beach/560E20.c', 'func_beach_802C907C', 70, 80), ('src/beach/560E20.c', 'func_beach_802C90E0', 82, 96), ('src/beach/560E20.c', 'func_beach_802C914C', 98, 112), ('src/beach/560E20.c', 'func_beach_802C91CC', 114, 116), ('src/beach/560E20.c', 'func_beach_802C9204', 118, 120), ('src/beach/562110.c', 'func_beach_802CA0A0', 4, 10), ('src/beach/562110.c', 'func_beach_802CA0F4', 12, 18), ('src/beach/562110.c', 'func_beach_802CA148', 20, 27), ('src/beach/562110.c', 'func_beach_802CA198', 29, 31), ('src/beach/562110.c', 'func_beach_802CA1BC', 33, 43), ('src/beach/562110.c', 'func_beach_802CA23C', 45, 55), ('src/beach/562110.c', 'func_beach_802CA2BC', 57, 59), ('src/beach/563000.c', 'func_beach_802CBCE0', 340, 342), ('src/beach/563000.c', 'func_beach_802CBD04', 344, 357), ('src/beach/563000.c', 'func_beach_802CBDA0', 359, 361), ('src/beach/563000.c', 'func_beach_802CBDD8', 363, 368))
PREAMBLE = '/* Complete pinned own-GAME Scyther/special closure. No SDK bodies or layouts. */\n#include "engine_internal.h"\n#include "beach_scyther_special_source.h"\n#include "beach_initial_species_source.h"\ntypedef IsleActorSource Pokemon;\ntypedef IsleActorFactoryBlock WorldBlock;\ntypedef IsleActorFactorySpawn ObjectSpawn;\ntypedef IsleActorGeoDefinition PokemonDef;\n#define UNUSED\n#define GET_POKEMON(object) isle_actor_source_require(object)\n#define omCreateProcess isle_actor_source_create_process\n#define Pokemon_Spawn isle_actor_owner_spawn\n#define Pokemon_SpawnDlLink4 isle_actor_owner_spawn_dl4\n#define LINK_POKEMON 3\n#define cmdSendCommandToLink(link, command, sender) isle_actor_source_send_to_link(sender, link, command, sender)\nenum { PokemonID_PIDGEY = 16, PokemonID_SCYTHER = 123 };\nenum { POKEMON_CMD_30 = 30, POKEMON_CMD_37 = 37, POKEMON_CMD_38 = 38 };\nstatic const IsleActorFactoryInitialization D_beach_802CD118 = { UINT32_C(0x802cd118) };\nstatic const IsleActorFactoryInitialization D_beach_802CD258 = { UINT32_C(0x802cd258) };\nstatic const IsleActorFactoryInitialization D_beach_802CD8E4 = { UINT32_C(0x802cd8e4) };\nstatic const IsleActorFactoryInitialization D_beach_802CE0A8 = { UINT32_C(0x802ce0a8) };\nstatic const PokemonDef D_beach_802CD838 = { UINT32_C(0x802cd838) };\nstatic const PokemonDef D_beach_802CD848 = { UINT32_C(0x802cd848) };\nstatic const PokemonDef D_beach_802CE0DC = { UINT32_C(0x802ce0dc) };\nvoid isle_actor_source_visibility(IsleGameObject*, int);\n'

SUPPORT += ('src/tunnel_actor_catalog.cpp', 'src/volcano_actor_catalog.cpp',
            'src/river_actor_catalog.cpp',
            'src/cave_actor_catalog.cpp', 'src/valley_actor_catalog.cpp')


def digest(data):
    return hashlib.sha256(data).hexdigest()


def canonical_digest(value):
    return digest(json.dumps(value, sort_keys=True, separators=(",", ":")).encode())


def prepare(repository, manifest_path, output_path, evidence_path):
    manifest_bytes = manifest_path.read_bytes()
    manifest = json.loads(manifest_bytes)
    root = Path(__file__).resolve().parent.parent
    if (manifest["schema_version"] != 1 or manifest["revision"] != REVISION or
            manifest["transformation_version"] != VERSION):
        raise ValueError("Scyther/special authority/version mismatch")
    output, evidence = output_path.resolve(), evidence_path.resolve()
    parent = output.parent
    protected = (repository.resolve(), *(root / name for name in
                 ("src", "include", "tests", "tools", "cmake", ".git", "graft")))
    if (output.name != "game.c" or evidence.name != "evidence.json" or output == evidence or
            parent != evidence.parent or parent.name != "isle-beach-scyther-special-private" or
            parent.parent.name != "_deps" or
            any(parent == base or base in parent.parents for base in protected)):
        raise ValueError("Scyther/special requires exact private build output paths")
    if manifest["generator_sha256"] != digest(Path(__file__).read_bytes()):
        raise ValueError("Scyther/special generator identity mismatch")
    if canonical_digest(manifest["source_files"]) != SOURCE_INVENTORY_SHA256:
        raise ValueError("Scyther/special source inventory mismatch")
    recipe = {key: manifest[key] for key in ("definitions", "semantic_origins", "rom_facts", "ownership")}
    if canonical_digest(recipe) != RECIPE_SHA256:
        raise ValueError("Scyther/special complete recipe/ROM/ownership mismatch")
    if set(manifest["native_support"]) != set(SUPPORT):
        raise ValueError("Scyther/special native support allowlist mismatch")
    if git(repository, "cat-file", "-t", REVISION).strip() != b"commit":
        raise ValueError("Scyther/special authority is not the pinned commit")
    sources = {}
    for path, identity in manifest["source_files"].items():
        raw = git(repository, "show", REVISION + ":" + path)
        blob = hashlib.sha1(b"blob " + str(len(raw)).encode() + b"\0" + raw).hexdigest()
        if (blob != identity["git_blob"] or digest(raw) != identity["sha256"] or
                len(raw.splitlines()) != identity["line_count"]):
            raise ValueError("Scyther/special pinned source identity mismatch: " + path)
        sources[path] = raw.decode()
    for path, identity in manifest["native_support"].items():
        selected = (root / path).resolve()
        if root not in selected.parents or digest(selected.read_bytes()) != identity["sha256"]:
            raise ValueError("Scyther/special native support identity mismatch: " + path)
    units = manifest["definitions"]
    if tuple((unit["path"], unit["symbol"], unit["first"], unit["last"]) for unit in units) != EXPECTED:
        raise ValueError("Scyther/special requires complete unique22 definition closure")
    for origin in manifest["semantic_origins"]:
        text = "".join(sources[origin["path"]].splitlines(keepends=True)[origin["first"]-1:origin["last"]])
        if digest(text.encode()) != origin["source_sha256"]:
            raise ValueError("Scyther/special semantic authority mismatch: " + origin["label"])
    bodies = []
    for unit in units:
        body = "".join(sources[unit["path"]].splitlines(keepends=True)[unit["first"]-1:unit["last"]])
        if digest(body.encode()) != unit["source_sha256"]:
            raise ValueError("Scyther/special original definition mismatch: " + unit["symbol"])
        for change in unit["changes"]:
            if change["occurrences"] < 1 or body.count(change["find"]) != change["occurrences"]:
                raise ValueError("Scyther/special counted rewrite mismatch: " + unit["symbol"] + ": " + change["find"])
            body = body.replace(change["find"], change["replace"])
        if (digest(body.encode()) != unit["native_sha256"] or
                body[:body.index("{")].strip() != unit["native_declaration"]):
            raise ValueError("Scyther/special transformed definition mismatch: " + unit["symbol"])
        bodies.append(body)
    text = PREAMBLE + "\n".join(bodies)
    if digest(text.encode()) != OUTPUT_SHA256 or manifest["output_sha256"] != OUTPUT_SHA256:
        raise ValueError("Scyther/special complete generated output identity mismatch")
    receipt = {"schema_version": 1, "revision": REVISION, "transformation_version": VERSION,
               "manifest_sha256": digest(manifest_bytes), "generator_sha256": manifest["generator_sha256"],
               "output_sha256": OUTPUT_SHA256, "definition_count": len(units),
               "definitions": [{key: unit[key] for key in ("path", "symbol", "first", "last", "source_sha256", "native_sha256")} for unit in units],
               "native_support": manifest["native_support"], "ownership": manifest["ownership"],
               "rom_facts": manifest["rom_facts"]}
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
