#!/usr/bin/env python3
"""Materialize the complete pinned Rainbow own-GAME actor closure privately."""
import argparse
import hashlib
import json
from pathlib import Path
from prepare_engine import git, publish

REVISION = 'df8815e75f3d79618466e01e0827c6165b93d11c'
VERSION = 'rainbow-actor-complete77-v2'
SOURCE_INVENTORY_SHA256 = '21de3ed1f242d77dab16747ef83695782695cf580a625fe35e49c03176a3c68b'
RECIPE_SHA256 = 'a13e709c1c1a03c627f3ed0edaa1a36170ce1a1eacc95e6474772ec255eb9633'
OUTPUT_SHA256 = '78130cd819b8feee8ba67385ff8244c768fb6d71081d17496732c4f3b828041e'
SUPPORT = ('src/rainbow_actor_source.h', 'src/rainbow_actor_source.c', 'src/rainbow_actor.hpp', 'src/rainbow_actor.cpp', 'src/actor_source.h', 'src/actor_owner_source.h', 'src/actor_source_bridge.c', 'src/actor_source_owner_bridge.c', 'src/actor_owner.hpp', 'src/actor_owner.cpp', 'src/actor_interaction_services.h', 'src/actor_interaction_services.hpp', 'src/actor_interaction_services.cpp', 'src/actor_behavior_bindings.hpp', 'src/actor_behavior_bindings.cpp', 'src/actor_resources.hpp', 'src/actor_resources.cpp', 'src/actor_behavior_resources.hpp', 'src/actor_behavior_resources.cpp', 'src/actor_resource_catalog.hpp', 'src/actor_resource_catalog.cpp', 'src/rainbow_actor_catalog.cpp', 'src/engine_internal.h', 'tools/prepare_engine.py', 'src/actor_course_context.hpp', 'src/rainbow_lifecycle_source.h', 'src/player_root.hpp')
EXPECTED = (('src/rainbow/main.c', 'func_803466C0_825E30', 49, 85), ('src/rainbow/main.c', 'func_803467A4_825F14', 87, 124), ('src/rainbow/main.c', 'func_80346898_826008', 126, 159), ('src/rainbow/mew/code.c', 'func_80346FA0_826710', 169, 192), ('src/rainbow/mew/code.c', 'func_803470CC_82683C', 194, 206), ('src/rainbow/mew/code.c', 'func_80347188_8268F8', 208, 213), ('src/rainbow/mew/code.c', 'func_803471D4_826944', 215, 236), ('src/rainbow/mew/code.c', 'func_80347334_826AA4', 238, 245), ('src/rainbow/mew/code.c', 'func_80347398_826B08', 247, 268), ('src/rainbow/mew/code.c', 'func_80347574_826CE4', 270, 276), ('src/rainbow/mew/code.c', 'func_803475D0_826D40', 278, 301), ('src/rainbow/mew/code.c', 'func_80347724_826E94', 303, 334), ('src/rainbow/mew/code.c', 'func_80347900_827070', 336, 339), ('src/rainbow/mew/code.c', 'func_80347940_8270B0', 341, 347), ('src/rainbow/mew/code.c', 'func_8034799C_82710C', 349, 369), ('src/rainbow/mew/code.c', 'func_80347B88_8272F8', 371, 392), ('src/rainbow/mew/code.c', 'func_80347CC8_827438', 394, 409), ('src/rainbow/mew/code.c', 'func_80347E0C_82757C', 411, 414), ('src/rainbow/mew/code.c', 'func_80347E44_8275B4', 416, 445), ('src/rainbow/mew/code.c', 'func_8034800C_82777C', 447, 460), ('src/rainbow/mew/code.c', 'func_803480C0_827830', 462, 474), ('src/rainbow/mew/code.c', 'func_80348208_827978', 476, 495), ('src/rainbow/mew/code.c', 'func_803482EC_827A5C', 497, 539), ('src/rainbow/mew/code.c', 'func_803484F0_827C60', 541, 549), ('src/rainbow/mew/code.c', 'func_80348540_827CB0', 551, 595), ('src/rainbow/mew/code.c', 'func_8034877C_827EEC', 597, 602), ('src/rainbow/mew/code.c', 'func_803487CC_827F3C', 604, 613), ('src/rainbow/mew/code.c', 'func_80348850_827FC0', 615, 631), ('src/rainbow/mew/code.c', 'func_80348938_8280A8', 657, 659), ('src/rainbow/gate/code.c', 'func_80348970_8280E0', 30, 32), ('src/rainbow/gate/code.c', 'func_80348994_828104', 34, 41), ('src/rainbow/gate/code.c', 'func_80348A04_828174', 43, 70), ('src/rainbow/gate/code.c', 'func_80348B34_8282A4', 72, 80), ('src/rainbow/gate/code.c', 'func_80348BDC_82834C', 82, 86), ('src/rainbow/gate/code.c', 'func_80348C08_828378', 88, 107), ('src/rainbow/gate/code.c', 'func_80348D48_8284B8', 109, 122), ('src/rainbow/gate/code.c', 'func_80348DD4_828544', 124, 169), ('src/rainbow/gate/code.c', 'func_80348FB8_828728', 171, 173), ('src/rainbow/gate/code.c', 'func_80348FF0_828760', 175, 191), ('src/rainbow/gate/code.c', 'func_80349084_8287F4', 193, 224), ('src/rainbow/pkmn_1037/code.c', 'func_80349180_8288F0', 48, 53), ('src/rainbow/pkmn_1037/code.c', 'func_803491D4_828944', 55, 71), ('src/rainbow/pkmn_1037/code.c', 'func_8034930C_828A7C', 73, 78), ('src/rainbow/pkmn_1037/code.c', 'func_8034935C_828ACC', 80, 92), ('src/rainbow/pkmn_1037/code.c', 'func_803493F4_828B64', 94, 104), ('src/rainbow/pkmn_1037/code.c', 'func_80349480_828BF0', 106, 116), ('src/rainbow/pkmn_1037/code.c', 'func_80349514_828C84', 118, 123), ('src/rainbow/pkmn_1037/code.c', 'func_8034956C_828CDC', 125, 134), ('src/rainbow/pkmn_1037/code.c', 'func_803495FC_828D6C', 136, 141), ('src/rainbow/pkmn_1037/code.c', 'func_80349654_828DC4', 143, 151), ('src/rainbow/pkmn_1037/code.c', 'func_803496BC_828E2C', 153, 158), ('src/rainbow/pkmn_1037/code.c', 'func_80349714_828E84', 160, 166), ('src/rainbow/pkmn_1037/code.c', 'func_8034977C_828EEC', 168, 179), ('src/rainbow/pkmn_1037/code.c', 'func_80349814_828F84', 181, 191), ('src/rainbow/pkmn_1037/code.c', 'func_803498A0_829010', 193, 203), ('src/rainbow/pkmn_1037/code.c', 'func_80349934_8290A4', 205, 210), ('src/rainbow/pkmn_1037/code.c', 'func_8034998C_8290FC', 212, 221), ('src/rainbow/pkmn_1037/code.c', 'func_80349A1C_82918C', 223, 228), ('src/rainbow/pkmn_1037/code.c', 'func_80349A74_8291E4', 230, 238), ('src/rainbow/pkmn_1037/code.c', 'func_80349ADC_82924C', 240, 245), ('src/rainbow/pkmn_1037/code.c', 'func_80349B34_8292A4', 247, 253), ('src/rainbow/pkmn_1037/code.c', 'func_80349B98_829308', 255, 262), ('src/rainbow/pkmn_1037/code.c', 'func_80349C18_829388', 264, 288), ('src/rainbow/pkmn_1037/code.c', 'func_80349D40_8294B0', 290, 296), ('src/rainbow/pkmn_1037/code.c', 'func_80349DB0_829520', 298, 301), ('src/rainbow/pkmn_1037/code.c', 'func_80349DE8_829558', 303, 306), ('src/rainbow/pkmn_1037/code.c', 'func_80349E28_829598', 311, 320), ('src/rainbow/pkmn_1037/code.c', 'func_80349EB8_829628', 322, 324), ('src/rainbow/evolution_controller/code.c', 'func_80349F00_829670', 9, 18), ('src/rainbow/evolution_controller/code.c', 'func_80349F88_8296F8', 20, 31), ('src/rainbow/evolution_controller/code.c', 'func_8034A064_8297D4', 33, 35), ('src/rainbow/evolution_controller/code.c', 'func_8034A09C_82980C', 37, 65), ('src/rainbow/pkmn_1038/code.c', 'func_8034A190_829900', 9, 22), ('src/rainbow/pkmn_1038/code.c', 'func_8034A27C_8299EC', 24, 33), ('src/rainbow/pkmn_1038/code.c', 'func_8034A334_829AA4', 35, 42), ('src/rainbow/pkmn_1038/code.c', 'func_8034A3A0_829B10', 44, 61), ('src/rainbow/pkmn_1038/code.c', 'func_8034A4E8_829C58', 63, 65))
PREAMBLE = '/* Complete pinned Rainbow Source C closure. No SDK bodies/layouts or assets. */\n#include "engine_internal.h"\n#include "rainbow_actor_source.h"\n#include <math.h>\n#include <string.h>\ntypedef IsleActorSource Pokemon;\ntypedef IsleActorFactoryBlock WorldBlock;\ntypedef IsleActorFactorySpawn ObjectSpawn;\ntypedef IsleGameVec3 Vec3f;\n#define omCreateProcess isle_actor_source_create_process\n#define Pokemon_SpawnOnGround isle_actor_owner_spawn_ground\n#define Pokemon_SpawnDlLink4 isle_actor_owner_spawn_dl4\n#define POKEMON_FLAG_8 0x008\n/* Same qualified native numeric policy as the common actor provider; no\n * uninspected SDK macro or libm bit-parity claim. Array lengths are own C. */\n#define PI 3.14159265358979323846\n#define TAU (2.0 * PI)\n#define ABS(value) (((value) < 0) ? -(value) : (value))\n#define ARRAY_COUNT(array) (sizeof(array) / sizeof((array)[0]))\nenum { PokemonID_MEW=151, PokemonID_GATE=1001,\n    PokemonID_EVOLUTION_CONTROLLER=1002, PokemonID_1037=1037, PokemonID_1038=1038,\n    POKEMON_CMD_9=9, POKEMON_CMD_13=13,\n    RAINBOW_CMD_28=28, RAINBOW_CMD_29=29, RAINBOW_CMD_30=30, RAINBOW_CMD_31=31,\n    RAINBOW_CMD_32=32, RAINBOW_CMD_33=33, RAINBOW_CMD_34=34, RAINBOW_CMD_35=35,\n    PLAYER_CMD_SHAKE_CAMERA=3, PLAYER_CMD_FINISH=6, PLAYER_CMD_7=7, PLAYER_CMD_9=9,\n    BGM_PLAYER_MAIN=0, BGM_PLAYER_AUX=1, SONG_ID_28=28,\n    SOUND_ID_38=38, SOUND_ID_39=39, SOUND_ID_40=40, SOUND_ID_41=41 };\nIsleActorSource* isle_rainbow_actor_source(IsleRainbowActorSource*, IsleGameObject*);\nIsleGameModelRef isle_rainbow_actor_model(IsleRainbowActorSource*, IsleGameObject*);\nconst IsleGameModelTransform* isle_rainbow_actor_aux(IsleRainbowActorSource*, const IsleGameModelRef*);\nvoid isle_rainbow_actor_store(IsleRainbowActorSource*, const IsleGameModelRef*, uint32_t, float);\nvoid isle_rainbow_actor_reference(IsleRainbowActorSource*, IsleGameObject**, IsleGameObject*);\nvoid isle_rainbow_actor_visibility(IsleRainbowActorSource*, IsleActorSource*, IsleGameObject*, int);\nIsleActorNodeRef isle_rainbow_actor_mew_node(IsleRainbowActorSource*);\nvoid isle_rainbow_actor_node_world(IsleRainbowActorSource*, const IsleActorNodeRef*, IsleGameVec3*, IsleGameVec3*);\nvoid* isle_rainbow_actor_current_block(IsleRainbowActorSource*);\nIsleGameObject* isle_rainbow_actor_add_one(IsleRainbowActorSource*, void*, void*, const IsleRainbowAutomaticSpawn*, const IsleRainbowDefinition*);\nint32_t isle_rainbow_actor_sound(IsleRainbowActorSource*, int32_t);\nint32_t isle_rainbow_actor_sound_active(IsleRainbowActorSource*, int32_t);\nIsleGameObject* isle_rainbow_actor_main_camera(IsleRainbowActorSource*);\nIsleGameObject* isle_rainbow_actor_player_model(IsleRainbowActorSource*);\nint32_t isle_rainbow_actor_collide(IsleRainbowActorSource*, IsleGameVec3*, IsleGameVec3*, IsleGameVec3*, IsleGameVec3*, const IsleGameModelRef*);\n'
PREAMBLE += 'void isle_rainbow_actor_enter(IsleRainbowActorSource*);\n'

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
            manifest["provider"] != "rainbow-actor" or
            manifest["transformation_version"] != VERSION):
        raise ValueError("Rainbow actor authority/version mismatch")
    output, evidence = output_path.resolve(), evidence_path.resolve()
    parent = output.parent
    protected = (repository.resolve(), *(root / name for name in
                 ("src", "include", "tests", "tools", "cmake", ".git", "graft")))
    if (output.name != "game.c" or evidence.name != "evidence.json" or output == evidence or
            parent != evidence.parent or parent.name != "isle-rainbow-actor-private" or
            parent.parent.name != "_deps" or
            any(parent == base or base in parent.parents for base in protected)):
        raise ValueError("Rainbow actor requires exact private build output paths")
    if manifest["generator_sha256"] != digest(Path(__file__).read_bytes()):
        raise ValueError("Rainbow actor generator identity mismatch")
    if canonical_digest(manifest["source_files"]) != SOURCE_INVENTORY_SHA256:
        raise ValueError("Rainbow actor closed source inventory mismatch")
    recipe = {key: manifest[key] for key in
              ("definitions", "semantic_origins", "rom_facts", "ownership")}
    if canonical_digest(recipe) != RECIPE_SHA256:
        raise ValueError("Rainbow actor complete recipe/ROM/ownership mismatch")
    if set(manifest["native_support"]) != set(SUPPORT):
        raise ValueError("Rainbow actor closed native support allowlist mismatch")
    if git(repository, "cat-file", "-t", REVISION).strip() != b"commit":
        raise ValueError("Rainbow actor authority is not the pinned commit")
    sources = {}
    for path, identity in manifest["source_files"].items():
        raw = git(repository, "show", REVISION + ":" + path)
        blob = hashlib.sha1(b"blob " + str(len(raw)).encode() + b"\0" + raw).hexdigest()
        if (blob != identity["git_blob"] or digest(raw) != identity["sha256"] or
                len(raw.splitlines()) != identity["line_count"]):
            raise ValueError("Rainbow actor pinned source identity mismatch: " + path)
        sources[path] = raw.decode()
    for path, identity in manifest["native_support"].items():
        selected = (root / path).resolve()
        if root not in selected.parents or digest(selected.read_bytes()) != identity["sha256"]:
            raise ValueError("Rainbow actor native support identity mismatch: " + path)
    units = manifest["definitions"]
    if tuple((u["path"], u["symbol"], u["first"], u["last"]) for u in units) != EXPECTED:
        raise ValueError("Rainbow actor requires the complete unique77 definition closure")
    for origin in manifest["semantic_origins"]:
        text = "".join(sources[origin["path"]].splitlines(keepends=True)
                       [origin["first"]-1:origin["last"]])
        if digest(text.encode()) != origin["source_sha256"]:
            raise ValueError("Rainbow actor semantic authority mismatch: " + origin["label"])
    bodies = []
    for unit in units:
        body = "".join(sources[unit["path"]].splitlines(keepends=True)
                       [unit["first"]-1:unit["last"]])
        if digest(body.encode()) != unit["source_sha256"]:
            raise ValueError("Rainbow actor original definition mismatch: " + unit["symbol"])
        for change in unit["changes"]:
            if change["occurrences"] < 1 or body.count(change["find"]) != change["occurrences"]:
                raise ValueError("Rainbow actor counted typed rewrite mismatch: " +
                                 unit["symbol"] + ": " + change["find"])
            body = body.replace(change["find"], change["replace"])
        if (digest(body.encode()) != unit["native_sha256"] or
                body[:body.index("{")].strip() != unit["native_declaration"]):
            raise ValueError("Rainbow actor transformed definition mismatch: " + unit["symbol"])
        bodies.append(body)
    text = PREAMBLE + "\n".join(bodies)
    if digest(text.encode()) != OUTPUT_SHA256 or manifest["output_sha256"] != OUTPUT_SHA256:
        raise ValueError("Rainbow actor complete generated output identity mismatch")
    receipt = {"schema_version": 1, "revision": REVISION, "transformation_version": VERSION,
               "manifest_sha256": digest(manifest_bytes),
               "generator_sha256": manifest["generator_sha256"], "output_sha256": OUTPUT_SHA256,
               "definition_count": len(units),
               "rewrite_count": sum(len(u["changes"]) for u in units),
               "definitions": [{key: u[key] for key in
                   ("path", "symbol", "first", "last", "source_sha256", "native_sha256")}
                   for u in units],
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
