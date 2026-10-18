#!/usr/bin/env python3
"""Materialize complete pinned own-GAME Eevee/Chansey privately; no SDK bodies."""
import argparse
import hashlib
import json
from pathlib import Path
from prepare_engine import git, publish

REVISION = "df8815e75f3d79618466e01e0827c6165b93d11c"
VERSION = "beach-eevee-chansey-complete38-v1"
SOURCE_INVENTORY_SHA256 = "db7900b35798aa1c8ca83c5e81a479a12a5ace661b6ed5cbc7d5719d63652f73"
RECIPE_SHA256 = "83080b7f94de5da685e0fcc567786a6df93f613caece748f2ccece20862f052e"
OUTPUT_SHA256 = "bdddb05c81bc93902b13f5d135c9ab4efde6345a0f676d6a7c0305ea07f3900b"
SUPPORT = ('src/beach_eevee_chansey_source.h', 'src/beach_eevee_chansey_source.c', 'src/beach_eevee_chansey.hpp', 'src/beach_eevee_chansey.cpp', 'src/actor_source.h', 'src/actor_source_bridge.c', 'src/actor_source_owner_bridge.c', 'src/actor_owner_source.h', 'src/actor_owner.hpp', 'src/actor_owner.cpp', 'src/actor_interaction_services.h', 'src/actor_interaction_services.hpp', 'src/actor_interaction_services.cpp', 'src/actor_behavior_bindings.hpp', 'src/actor_behavior_bindings.cpp', 'src/beach_initial_species_source.h', 'src/actor_resources.hpp', 'src/actor_resource_catalog.hpp', 'src/beach_actor_scene.hpp', 'src/actor_resources.cpp', 'src/actor_resource_catalog.cpp', 'src/beach_actor_catalog.cpp', 'src/rainbow_actor_catalog.cpp', 'src/beach_actor_scene.cpp', 'src/actor_behavior_resources.hpp', 'src/actor_behavior_resources.cpp', 'src/source_command_scope.h', 'src/engine.c', 'src/engine_internal.h', 'include/35mmisle/engine.h', 'include/35mmisle/rng.h', 'src/rng.c', 'tools/prepare_engine.py', 'src/actor_course_context.hpp', 'src/rainbow_actor_source.h', 'src/rainbow_actor_source.c', 'src/rainbow_actor.hpp', 'src/rainbow_actor.cpp', 'src/rainbow_lifecycle_source.h', 'src/player_root.hpp')
SYMBOLS = ('func_beach_802C72E0', 'func_beach_802C737C', 'func_beach_802C73EC', 'func_beach_802C7490', 'func_beach_802C74EC', 'func_beach_802C755C', 'func_beach_802C75D0', 'func_beach_802C762C', 'func_beach_802C76C4', 'func_beach_802C7748', 'func_beach_802C77CC', 'func_beach_802C77F0', 'func_beach_802C7864', 'func_beach_802C78B0', 'func_beach_802C78E8', 'func_beach_802C796C', 'func_beach_802C79DC', 'func_beach_802C7A60', 'func_beach_802C7AD0', 'func_beach_802C9240', 'func_beach_802C9274', 'func_beach_802C92BC', 'func_beach_802C9348', 'func_beach_802C93B8', 'func_beach_802C943C', 'func_beach_802C94F0', 'func_beach_802C9580', 'func_beach_802C95F0', 'func_beach_802C9694', 'func_beach_802C96F0', 'func_beach_802C9760', 'func_beach_802C97D4', 'func_beach_802C9830', 'func_beach_802C98F4', 'func_beach_802C9978', 'func_beach_802C999C', 'func_beach_802C9A10', 'func_beach_802C9A7C')
PREAMBLE = '/* Complete pinned own-GAME Eevee133/Chansey113; no SDK bodies or layouts. */\n#include "engine_internal.h"\n#include "beach_eevee_chansey_source.h"\n#include "beach_initial_species_source.h"\ntypedef IsleActorSource Pokemon;\ntypedef IsleActorInteraction InteractionHandler;\ntypedef IsleActorFactoryBlock WorldBlock;\ntypedef IsleActorFactorySpawn ObjectSpawn;\n#define UNUSED\n#define GET_POKEMON(object) isle_actor_source_require(object)\n#define Pokemon_SpawnOnGround isle_actor_owner_spawn_ground\n#define LINK_POKEMON 3\n#define POKEMON_CMD_33 33\n#define POKEMON_CMD_34 34\n#define POKEMON_FLAG_8 0x008\n#define POKEMON_FLAG_200 0x200\n#define POKEMON_FLAG_800 0x800\n#define cmdSendCommandToLink(link, command, sender) isle_actor_source_send_to_link(sender, link, command, sender)\nstatic const IsleActorFactoryInitialization D_beach_802CCCEC = { UINT32_C(0x802cccec) };\nstatic const IsleActorFactoryInitialization D_beach_802CD55C = { UINT32_C(0x802cd55c) };\n'

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
        raise ValueError("Beach Eevee/Chansey authority/version mismatch")
    output, evidence = output_path.resolve(), evidence_path.resolve()
    parent = output.parent
    protected = (repository.resolve(), *(root / name for name in
                 ("src", "include", "tests", "tools", "cmake", ".git", "graft")))
    if (output.name != "game.c" or evidence.name != "evidence.json" or output == evidence or
            parent != evidence.parent or parent.name != "isle-beach-eevee-chansey-private" or
            parent.parent.name != "_deps" or
            any(parent == base or base in parent.parents for base in protected)):
        raise ValueError("Beach Eevee/Chansey requires exact private build output paths")
    if manifest["generator_sha256"] != digest(Path(__file__).read_bytes()):
        raise ValueError("Beach Eevee/Chansey generator identity mismatch")
    if canonical_digest(manifest["source_files"]) != SOURCE_INVENTORY_SHA256:
        raise ValueError("Beach Eevee/Chansey source inventory mismatch")
    recipe = {key: manifest[key] for key in
              ("definitions", "semantic_origins", "rom_facts", "ownership", "common_requirements")}
    if canonical_digest(recipe) != RECIPE_SHA256:
        raise ValueError("Beach Eevee/Chansey complete recipe/ROM/source ownership mismatch")
    if set(manifest["native_support"]) != set(SUPPORT):
        raise ValueError("Beach Eevee/Chansey native support allowlist mismatch")
    if git(repository, "cat-file", "-t", REVISION).strip() != b"commit":
        raise ValueError("Beach Eevee/Chansey authority is not pinned commit")
    sources = {}
    for path, identity in manifest["source_files"].items():
        raw = git(repository, "show", REVISION + ":" + path)
        blob = hashlib.sha1(b"blob " + str(len(raw)).encode() + b"\0" + raw).hexdigest()
        if (blob != identity["git_blob"] or digest(raw) != identity["sha256"] or
                len(raw.splitlines()) != identity["line_count"]):
            raise ValueError("Beach Eevee/Chansey pinned source identity mismatch: " + path)
        sources[path] = raw.decode()
    for path, identity in manifest["native_support"].items():
        selected = (root / path).resolve()
        if root not in selected.parents or digest(selected.read_bytes()) != identity["sha256"]:
            raise ValueError("Beach Eevee/Chansey native support identity mismatch: " + path)
    units = manifest["definitions"]
    if tuple(unit["symbol"] for unit in units) != SYMBOLS:
        raise ValueError("Beach Eevee/Chansey requires all complete unique38 definitions")
    for origin in manifest["semantic_origins"]:
        text = "".join(sources[origin["path"]].splitlines(keepends=True)[origin["first"]-1:origin["last"]])
        if digest(text.encode()) != origin["source_sha256"]:
            raise ValueError("Beach Eevee/Chansey semantic authority mismatch: " + origin["label"])
    bodies = []
    for unit in units:
        body = "".join(sources[unit["path"]].splitlines(keepends=True)[unit["first"]-1:unit["last"]])
        if digest(body.encode()) != unit["source_sha256"]:
            raise ValueError("Beach Eevee/Chansey original definition mismatch: " + unit["symbol"])
        for change in unit["changes"]:
            if change["occurrences"] < 1 or body.count(change["find"]) != change["occurrences"]:
                raise ValueError("Beach Eevee/Chansey counted rewrite mismatch: " + unit["symbol"] + ": " + change["find"])
            body = body.replace(change["find"], change["replace"])
        if (digest(body.encode()) != unit["native_sha256"] or
                body[:body.index("{")].strip() != unit["native_declaration"]):
            raise ValueError("Beach Eevee/Chansey transformed definition mismatch: " + unit["symbol"])
        bodies.append(body)
    text = PREAMBLE + "\n".join(bodies)
    if digest(text.encode()) != OUTPUT_SHA256 or manifest["output_sha256"] != OUTPUT_SHA256:
        raise ValueError("Beach Eevee/Chansey complete output identity mismatch")
    receipt = {
        "schema_version": 1, "revision": REVISION, "transformation_version": VERSION,
        "manifest_sha256": digest(manifest_bytes), "generator_sha256": manifest["generator_sha256"],
        "output_sha256": OUTPUT_SHA256, "definition_count": len(units),
        "definitions": [{key: unit[key] for key in
                         ("path", "symbol", "first", "last", "source_sha256", "native_sha256")}
                        for unit in units],
        "native_support": manifest["native_support"], "ownership": manifest["ownership"],
        "rom_facts": manifest["rom_facts"], "common_requirements": manifest["common_requirements"],
    }
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
