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
MAGIKARP = "src/beach/562BC0.c"
# Ordered, exact, single-occurrence substitutions; native identities are never guest casts.
DEFINITIONS = (
    (POKEMON, "Pokemon_SetState", 252, 263, ()),
    (POKEMON, "Pokemon_SetStateRandom", 265, 288, (("randRange(sumWeight)", "isle_actor_source_rand_range(obj, sumWeight)"),)),
    (POKEMON, "Pokemon_StartAuxProc", 291, 296, ()),
    (POKEMON, "Pokemon_StopAuxProc", 298, 303, ()),
    (POKEMON, "Pokemon_StartPathProc", 305, 318, ()),
    (POKEMON, "Pokemon_WaitForFlagNoInteraction", 736, 745, ()),
    (POKEMON, "Pokemon_GetClosestItem", 509, 533, (
        ("    DObj* model = obj->data.dobj;", "    IsleGameVec3 source_position = isle_actor_source_position(obj, obj, 1);"),
        ("    x = GET_TRANSFORM(model)->pos.v.x;\n    y = GET_TRANSFORM(model)->pos.v.y;\n    z = GET_TRANSFORM(model)->pos.v.z;", "    x = source_position.x;\n    y = source_position.y;\n    z = source_position.z;"),
        ("Items_InitIterator()", "isle_actor_source_item_iterator(obj)"),
        ("Items_NextValidItem()", "isle_actor_source_item_next(obj)"),
        ("        Mtx3Float* position = &currItem->data.dobj->position;\n        f32 currDist2 = SQ(x - position->v.x) + SQ(y - position->v.y) + SQ(z - position->v.z);", "        IsleGameVec3 position = isle_actor_source_position(obj, currItem, 0);\n        f32 currDist2 = (x-position.x)*(x-position.x) + (y-position.y)*(y-position.y) + (z-position.z)*(z-position.z);"),
    )),
    (POKEMON, "Pokemon_RunInteractions", 630, 720, (
        ("    s32 continueSearch = true;", "    s32 continueSearch = true;\n    IsleActorFocusState source_focus;"),
        ("    Vec3f dirToNeighbors;", "    IsleGameVec3 dirToNeighbors;"),
        ("    InteractionHandler* transitionGraph = pokemon->transitionGraph;", "    const InteractionHandler* transitionGraph = isle_actor_source_interaction_rows(obj);"),
        ("Item* item = GET_ITEM(interactionTarget);", "IsleItemSource* item = isle_actor_source_item(obj, interactionTarget);"),
        ("pokemon->apple = interactionTarget;", "isle_actor_source_reference_assign(obj, &pokemon->apple, interactionTarget);"),
        ("pokemon->interactionTarget = interactionTarget;", "isle_actor_source_reference_assign(obj, &pokemon->interactionTarget, interactionTarget);"),
        ("pokemon->interactionTarget = gObjPlayer;", "isle_actor_source_reference_assign(obj, &pokemon->interactionTarget, gObjPlayer);", 2),
        ("pokemon->interactionDist = interactionDist; // BUG: interactionDist undefined here, must use 'playerDist'", "pokemon->interactionDist = pokemon->playerDist; /* Defined native BUG16 correction. */"),
        ("            case POKEMON_CMD_23:\n                if (sTimerInFocus > transitionGraph[i].value && obj == gPokemonInFocus) {", "            case POKEMON_CMD_23:\n                source_focus = isle_actor_source_focus(obj);\n                if (source_focus.timer > transitionGraph[i].value && obj == source_focus.current) {"),
    )),
    (POKEMON, "Pokemon_HandleCommand", 1806, 2064, (
        ("    InteractionHandler* transitionGraph;", "    const InteractionHandler* transitionGraph;"),
        ("    transitionGraph = pokemon->transitionGraph;", "    transitionGraph = pokemon->transitionGraph != NULL ? pokemon->transitionGraph->rows : NULL;"),
        ("pokemon->apple = cmdData.source;", "isle_actor_source_reference_assign(omCurrentObject, &pokemon->apple, cmdData.source);", 2),
        ("pokemon->interactionTarget = cmdData.source;", "isle_actor_source_reference_assign(omCurrentObject, &pokemon->interactionTarget, cmdData.source);", 11),
        ("pokemon->apple = NULL;", "isle_actor_source_reference_assign(omCurrentObject, &pokemon->apple, NULL);"),
        ("pokemon->interactionTarget = NULL;", "isle_actor_source_reference_assign(omCurrentObject, &pokemon->interactionTarget, NULL);"),
    )),
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
    (MAGIKARP, "func_beach_802CAB50", 4, 18, (
        ("    DObj* model = obj->data.dobj;\n    Mtx4Float* rotation = &GET_TRANSFORM(model)->rot;\n", ""),
        ("Pokemon_SetAnimation(obj, &D_beach_802CDAB4);", "isle_actor_source_set_animation(obj, UINT32_C(0x802cdab4), 0);"),
        ("rotation->v.y = randRange(360) * PI / 180.0f;", "isle_actor_source_yaw(obj, isle_actor_source_rand_range(obj, 360) * PI / 180.0f);"),
        ("D_beach_802CC014 = false;", "isle_actor_source_magikarp_active(obj, false);"),
    )),
    (MAGIKARP, "func_beach_802CAC08", 20, 28, ()),
    (MAGIKARP, "func_beach_802CAC48", 30, 32, (
        ("void func_beach_802CAC48", "GObj* func_beach_802CAC48"),
        ("    Pokemon_SpawnOnGround(", "    return Pokemon_SpawnOnGround("),
    )),
    (COMMON, "func_8035E298_4FE6A8", 175, 195, (
        ("    Vec3f pos;", "    IsleGameVec3 pos;"),
        ("    Vec3f vel;", "    IsleGameVec3 vel;"),
        ("    DObj* s1;", "    size_t s1;"),
        ("s1 = obj->data.dobj->firstChild->firstChild->next;", "s1 = isle_actor_source_ground_node(obj);"),
        ("fx_getPosVelDObj(&pos, &vel, s1);", "isle_actor_source_node_world(obj, s1, &pos, &vel);"),
        ("getGroundAt(pos.x, pos.z, &sp28);", "isle_actor_source_ground(obj, pos.x, pos.z, &sp28);"),
    )),
    (COMMON, "func_8035E174_4FE584", 130, 141, (
        ("Vec3f* arg1", "IsleGameVec3* arg1"),
        ("    PokemonTransform* sp2C;\n", ""),
        ("    sp2C = GET_TRANSFORM(sp30->data.dobj);\n    sp2C->pos.v.x = arg1->x;\n    sp2C->pos.v.y = arg1->y;\n    sp2C->pos.v.z = arg1->z;", "    isle_actor_source_publish_position(sp30, *arg1);"),
    )),
    (POKEMON, "Pokemon_AddAtGeo", 793, 826, (
        ("PokemonDef* def", "const PokemonDef* def"),
        ("    ObjectSpawn spawn;", "    IsleActorGeoSpawn spawn;"),
        ("    WorldBlock* block;", "    void* block;"),
        ("getCurrentWorldBlock()", "isle_actor_source_current_block(gobj)"),
        ("pokemonAddOne(block, block, &spawn, def)", "isle_actor_source_add_one(gobj, block, block, &spawn, def)"),
        ("        PokemonTransformBase* transform2 = GET_TRANSFORM_BASE(gobj->data.dobj);\n        PokemonTransformBase* transform1 = GET_TRANSFORM_BASE(pokemonObj->data.dobj);\n\n        transform1->xform.pos.v.x = transform2->xform.pos.v.x;\n        transform1->xform.pos.v.y = transform2->xform.pos.v.y;\n        transform1->xform.pos.v.z = transform2->xform.pos.v.z;", "        isle_actor_source_copy_position(gobj, pokemonObj);"),
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
#include "item_source.h"
#include <float.h>
typedef IsleActorInteraction InteractionHandler;
typedef IsleActorCommandData GObjCmdData;
#define FLOAT_MAX FLT_MAX
#define ITEM_ID_APPLE ISLE_ITEM_APPLE
#define ITEM_STATE_STILL ISLE_ITEM_STILL
#define Pokemon_GetDistance isle_actor_source_distance
#define Pokemon_GetDirToNeighbours isle_actor_source_neighbors
#define Items_GetPokeFluteCmd() isle_actor_source_flute(omCurrentObject)
#define gObjPlayer isle_actor_source_player(omCurrentObject)
#define gDirectionIndex isle_actor_source_direction(omCurrentObject)
#define getCurrentWorldBlock() isle_actor_source_current_block(omCurrentObject)
#define func_80353D68_4F4178() isle_actor_source_execution_enabled(omCurrentObject)
#define ohPauseObjectProcesses isle_actor_source_pause_all
typedef IsleActorSource Pokemon;
typedef IsleActorRandomState RandomState;
typedef IsleActorFactoryBlock WorldBlock;
typedef IsleActorFactorySpawn ObjectSpawn;
typedef IsleActorFactoryInitialization PokemonInitData;
static const PokemonInitData D_80388348_528758 = { UINT32_C(0x80388348) };
typedef IsleActorGround GroundResult;
typedef IsleActorGeoDefinition PokemonDef;
static const PokemonInitData D_beach_802CDAEC = { UINT32_C(0x802cdaec) };
static const PokemonDef D_8038837C_52878C = { UINT32_C(0x8038837c) };
static const IsleGameVec3 D_8038838C_52879C = { 0.0f, 0.0f, 0.0f };
#define PokemonID_1003 1003
/* Native numeric policy: NIST DLMF3.12.E1 pi, unsuffixed C double.
 * Original SDK macro spelling/type equivalence is unproven; no SDK import. */
#define PI 3.14159265358979323846
#define UNUSED
#define Pokemon_SpawnOnGround isle_actor_owner_spawn_ground
#define Pokemon_SpawnOnGroundDlLink4 isle_actor_owner_spawn_ground_dl4
#define GET_POKEMON(object) isle_actor_source_require(object)
#define omCreateProcess isle_actor_source_create_process
void isle_actor_source_set_animation(IsleGameObject*, uint32_t, int);
void isle_actor_source_subtract_y(IsleGameObject*, float);
void isle_actor_source_visibility(IsleGameObject*, int);
void isle_actor_source_commands(IsleGameObject*);
''' + chr(10).join("enum { POKEMON_CMD_%d = %d };" % (value, value)
    for value in (5, 6, 7, 8, 9, 10, *range(12, 59))) + chr(10) + '''
'''

def reviewed_changes(path, symbol, first, last, changes, sources):
    result = []
    for change in changes:
        old, new = change[:2]
        entry = {"find": old, "replace": new}
        if len(change) == 3:
            entry["occurrences"] = change[2]
        result.append(entry)
    return result

def prepare(repository, manifest_path, output_path, owner_output_path, evidence_path):
    manifest_bytes = manifest_path.read_bytes()
    manifest = json.loads(manifest_bytes)
    if (manifest["schema_version"] != 1 or manifest["revision"] != REVISION or
            manifest["transformation_version"] != "actor-control-v5"):
        raise ValueError("unsupported actor authority/transformation")
    if git(repository, "cat-file", "-t", REVISION).strip() != b"commit":
        raise ValueError("actor authority is not a commit")
    if set(manifest["source_files"]) != {POKEMON, COMMON, MAGIKARP, "src/app_level/app_level.h", "include/enums.h", "include/common_structs.h"}:
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
    support_paths.update({"src/actor_interaction_services.h", "src/item_source.h",
                          "src/source_command_scope.h", "src/source_factory_resources.h"})
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
            occurrences = change.get("occurrences", 1)
            if occurrences < 1 or native.count(change["find"]) != occurrences:
                raise ValueError("actor transformation occurrence count differs from its exact authority")
            native = native.replace(change["find"], change["replace"], occurrences)
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
