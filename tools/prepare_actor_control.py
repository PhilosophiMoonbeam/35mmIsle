#!/usr/bin/env python3
"""Materialize audited pinned GAME C actor bodies in private build output only."""
import argparse
import hashlib
import json
from pathlib import Path
from prepare_engine import git, publish

REVISION = "df8815e75f3d79618466e01e0827c6165b93d11c"
POKEMON = "src/app_level/pokemon.c"
COMMON = "src/app_level/4FE330.c"
# Ordered, exact, single-occurrence substitutions; native identities are never guest casts.
DEFINITIONS = (
    (POKEMON, "Pokemon_SetState", 252, 263, ()),
    (POKEMON, "Pokemon_SetStateRandom", 265, 288, (("randRange(sumWeight)", "isle_actor_source_rand_range(obj, sumWeight)"),)),
    (POKEMON, "Pokemon_StartAuxProc", 291, 296, ()),
    (POKEMON, "Pokemon_StopAuxProc", 298, 303, ()),
    (POKEMON, "Pokemon_StartPathProc", 305, 318, ()),
    (POKEMON, "Pokemon_WaitForFlagNoInteraction", 736, 745, ()),
    (POKEMON, "Pokemon_RunInteractions", 630, 720, (("    s32 i;\n    s32 continueSearch = true;\n    Vec3f dirToNeighbors;\n    f32 interactionDist;\n    GObj* interactionTarget;\n    GObjFunc stateProc = NULL;\n    GObjFunc auxProc = NULL;\n    InteractionHandler* transitionGraph = pokemon->transitionGraph;", "    const void* transitionGraph = pokemon->transitionGraph;"),)),
    (POKEMON, "Pokemon_WaitForFlag", 723, 734, ()),
    (POKEMON, "Pokemon_RunCleanup", 771, 773, ()),
    (POKEMON, "Pokemon_Update", 2066, 2089, (("getIsPaused()", "isle_actor_owner_paused(pokemon)"), ("obj->animationTime", "isle_actor_owner_animation_time(pokemon)"), ("cmdProcessCommands(Pokemon_HandleCommand);", "isle_actor_source_commands(obj);"))),
    (COMMON, "func_8035DF20_4FE330", 63, 77, (("Pokemon_SetAnimation(obj, &D_80388310_528720);", "isle_actor_source_set_animation(obj, UINT32_C(0x80388310), 0);"),)),
    (COMMON, "func_8035DFB0_4FE3C0", 79, 93, (("    DObj* model = obj->data.dobj;\n", ""), ("model->position.v.y -= 0.2f;", "isle_actor_source_subtract_y(obj, 0.2f);"))),
    (COMMON, "func_8035E034_4FE444", 95, 110, (("HIDE_POKEMON();", "isle_actor_source_visibility(obj, 1);"), ("SHOW_POKEMON();", "isle_actor_source_visibility(obj, 0);"), ("Pokemon_ForceAnimation(obj, &D_80388310_528720);", "isle_actor_source_set_animation(obj, UINT32_C(0x80388310), 1);"))),
    (COMMON, "func_8035E0D4_4FE4E4", 112, 114, ()),
    (COMMON, "func_8035E264_4FE674", 164, 173, (
        ("void func_8035E264_4FE674", "IsleGameResult func_8035E264_4FE674"),
        ("Vec3f* arg1", "IsleGameVec3* arg1"),
        ("    PokemonTransform* sp2C;\n", ""),
        ("    Pokemon* pokemon = GET_POKEMON(obj);", "    Pokemon* pokemon = NULL;\n    IsleGameResult result = isle_actor_source_get(obj->game, obj, &pokemon);\n    if (result != ISLE_GAME_OK) return result;"),
        ("    sp2C = GET_TRANSFORM(obj->data.dobj);\n    sp2C->pos.v.x = arg1->x;\n    sp2C->pos.v.y = arg1->y;\n    sp2C->pos.v.z = arg1->z;", "    result = isle_actor_owner_trigger_position(pokemon, *arg1);\n    if (result != ISLE_GAME_OK) return result;"),
        ("    pokemon->miscVars[0].field1 = 1;", "    pokemon->miscVars[0].field1 = 1;\n    return ISLE_GAME_OK;"),
    )),
)
CONTROL_SYMBOLS = frozenset(unit[1] for unit in DEFINITIONS[:6])
CONTROL_PREAMBLE = '''/* Private selected own-GAME core actor control. */
#include "engine_internal.h"
#include "actor_source.h"
typedef IsleActorSource Pokemon;
typedef IsleActorRandomState RandomState;
#define GET_POKEMON(object) isle_actor_source_require(object)
#define omCreateProcess isle_actor_source_create_process
'''
PREAMBLE = '''/* Private transformed own-GAME bodies, not byte-identical source. */
#include "engine_internal.h"
#include "actor_owner_source.h"
typedef IsleActorSource Pokemon;
typedef IsleActorRandomState RandomState;
typedef IsleActorFactoryBlock WorldBlock;
typedef IsleActorFactorySpawn ObjectSpawn;
typedef IsleActorFactoryInitialization PokemonInitData;
static const PokemonInitData D_80388348_528758 = { UINT32_C(0x80388348) };
#define Pokemon_SpawnOnGroundDlLink4 isle_actor_owner_spawn_ground_dl4
#define GET_POKEMON(object) isle_actor_source_require(object)
#define omCreateProcess isle_actor_source_create_process
void isle_actor_source_set_animation(IsleGameObject*, uint32_t, int);
void isle_actor_source_subtract_y(IsleGameObject*, float);
void isle_actor_source_visibility(IsleGameObject*, int);
void isle_actor_source_commands(IsleGameObject*);
void isle_actor_source_interactions(IsleGameObject*);
'''

def reviewed_changes(path, symbol, first, last, changes, sources):
    result = list(changes)
    if symbol == "Pokemon_RunInteractions":
        # Preserve the genuine NULL-graph early return verbatim. The unadmitted
        # nonNULL graph interpreter must fail owner admission, never be a no-op.
        tail = "".join(sources[path][644:720])
        result.append((tail, "    isle_actor_source_interactions(obj);\n}\n"))
    return [{"find": old, "replace": new} for old, new in result]

def prepare(repository, manifest_path, output_path, owner_output_path, evidence_path):
    manifest_bytes = manifest_path.read_bytes()
    manifest = json.loads(manifest_bytes)
    if (manifest["schema_version"] != 1 or manifest["revision"] != REVISION or
            manifest["transformation_version"] != "actor-control-v2"):
        raise ValueError("unsupported actor authority/transformation")
    if git(repository, "cat-file", "-t", REVISION).strip() != b"commit":
        raise ValueError("actor authority is not a commit")
    if set(manifest["source_files"]) != {POKEMON, COMMON, "include/enums.h", "include/common_structs.h"}:
        raise ValueError("actor source allowlist mismatch")
    sources = {}
    for path, identity in manifest["source_files"].items():
        data = git(repository, "show", REVISION + ":" + path)
        blob = git(repository, "rev-parse", REVISION + ":" + path).decode("ascii").strip()
        if (blob != identity["git_blob"] or hashlib.sha256(data).hexdigest() != identity["sha256"] or
                hashlib.sha1(b"blob " + str(len(data)).encode() + b"\0" + data).hexdigest() != blob):
            raise ValueError("actor source identity mismatch: " + path)
        sources[path] = data.decode().splitlines(keepends=True)
    root = Path(__file__).resolve().parent.parent
    support_paths = {"src/actor_source.h", "src/actor_source_bridge.c", "src/actor_source_owner_bridge.c",
                     "src/actor_owner_source.h", "src/engine.c", "src/engine_internal.h",
                     "include/35mmisle/engine.h", "include/35mmisle/rng.h", "src/rng.c", "tools/prepare_engine.py"}
    if set(manifest["native_support"]) != support_paths:
        raise ValueError("actor native support allowlist mismatch")
    for path, identity in manifest["native_support"].items():
        if hashlib.sha256((root / path).read_bytes()).hexdigest() != identity["sha256"]:
            raise ValueError("actor native support identity mismatch: " + path)
    if len(manifest["definitions"]) != len(DEFINITIONS):
        raise ValueError("actor definition allowlist mismatch")
    control_output = [CONTROL_PREAMBLE]
    owner_output = [PREAMBLE]
    for unit, (path, symbol, first, last, changes) in zip(manifest["definitions"], DEFINITIONS):
        if (unit["path"], unit["symbol"], unit["first"], unit["last"]) != (path, symbol, first, last):
            raise ValueError("actor definition allowlist mismatch")
        selected = "".join(sources[path][first - 1:last])
        if hashlib.sha256(selected.encode()).hexdigest() != unit["source_sha256"]:
            raise ValueError("actor selected source identity mismatch")
        expected = reviewed_changes(path, symbol, first, last, changes, sources)
        if unit["changes"] != expected:
            raise ValueError("unadmitted actor transformation")
        native = selected
        for change in expected:
            if native.count(change["find"]) != 1:
                raise ValueError("actor transformation must match exactly once")
            native = native.replace(change["find"], change["replace"], 1)
        if hashlib.sha256(native.encode()).hexdigest() != unit["native_sha256"]:
            raise ValueError("actor transformed identity mismatch")
        output = control_output if symbol in CONTROL_SYMBOLS else owner_output
        output.extend(('#line %d "snap@%s/%s"\n' % (first, REVISION, path), native, "\n"))
    for origin in manifest["semantic_origins"]:
        selected = "".join(sources[origin["path"]][origin["first"] - 1:origin["last"]])
        if hashlib.sha256(selected.encode()).hexdigest() != origin["source_sha256"]:
            raise ValueError("actor semantic origin identity mismatch")
    control_source = "".join(control_output)
    owner_source = "".join(owner_output)
    evidence = dict(manifest)
    evidence.update(manifest_sha256=hashlib.sha256(manifest_bytes).hexdigest(),
                    generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                    control_source_sha256=hashlib.sha256(control_source.encode()).hexdigest(),
                    owner_source_sha256=hashlib.sha256(owner_source.encode()).hexdigest())
    if len({output_path.resolve(), owner_output_path.resolve(), evidence_path.resolve()}) != 3:
        raise ValueError("control, owner and evidence paths must differ")
    publish(output_path, control_source)
    publish(owner_output_path, owner_source)
    publish(evidence_path, json.dumps(evidence, indent=2) + "\n")

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repository", type=Path, required=True)
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--owner-output", type=Path, required=True)
    parser.add_argument("--evidence", type=Path, required=True)
    args = parser.parse_args()
    prepare(args.repository, args.manifest, args.output, args.owner_output, args.evidence)

if __name__ == "__main__":
    main()
