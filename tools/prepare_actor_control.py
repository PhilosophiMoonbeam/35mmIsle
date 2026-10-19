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
PLAYER = "src/app_level/player.c"
# Ordered, exact, single-occurrence substitutions; native identities are never guest casts.
DEFINITIONS = (
    (POKEMON, "Pokemon_SetState", 252, 263, ()),
    (POKEMON, "Pokemon_SetStateRandom", 265, 288, (
        ("RandomState* stateList", "const RandomState* stateList"),
        ("randRange(sumWeight)", "isle_actor_source_rand_range(obj, sumWeight)"),
    )),
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
    (COMMON, "func_8035DF20_4FE330", 63, 77, (("Pokemon_SetAnimation(obj, &D_80388310_528720);", "isle_actor_source_set_animation(obj, UINT32_C(0x80388310), 0.0f, 0);"),)),
    (COMMON, "func_8035DFB0_4FE3C0", 79, 93, (("    DObj* model = obj->data.dobj;\n", ""), ("model->position.v.y -= 0.2f;", "isle_actor_source_subtract_y(obj, 0.2f);"))),
    (COMMON, "func_8035E034_4FE444", 95, 110, (("HIDE_POKEMON();", "isle_actor_source_visibility(obj, 1);"), ("SHOW_POKEMON();", "isle_actor_source_visibility(obj, 0);"), ("Pokemon_ForceAnimation(obj, &D_80388310_528720);", "isle_actor_source_set_animation(obj, UINT32_C(0x80388310), 0.0f, 1);"))),
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
        ("Pokemon_SetAnimation(obj, &D_beach_802CDAB4);", "isle_actor_source_set_animation(obj, UINT32_C(0x802cdab4), 0.0f, 0);"),
        ("rotation->v.y = randRange(360) * PI / 180.0f;", "{\n        IsleGameModelRef model = isle_actor_source_self_model(obj);\n        IsleGameModelTransform values;\n        values.rotation.y = isle_actor_source_rand_range(obj, 360) * PI / 180.0f;\n        isle_actor_source_aux_store(obj, &model, ISLE_ACTOR_RY, &values);\n    }"),
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
    (COMMON, "func_8035E1D4_4FE5E4", 143, 154, (
        ("    PokemonTransform* sp2C;", "    IsleGameVec3 sp2C;"),
        ("    sp2C = GET_TRANSFORM(sp30->data.dobj);", "    sp2C = isle_actor_source_position(sp30, sp30, 1);"),
        ("    getGroundAt(sp2C->pos.v.x, sp2C->pos.v.z, &sp18);", "    isle_actor_source_ground(sp30, sp2C.x, sp2C.z, &sp18);"),
        ("    sp2C->pos.v.y = sp18.height;", "    sp2C.y = sp18.height;\n    isle_actor_source_publish_position(sp30, sp2C);"),
    )),
    (COMMON, "func_8035E238_4FE648", 156, 162, ()),
    (POKEMON, "Pokemon_AddAtGeo", 793, 826, (
        ("PokemonDef* def", "const PokemonDef* def"),
        ("    ObjectSpawn spawn;", "    IsleActorGeoSpawn spawn;"),
        ("    WorldBlock* block;", "    void* block;"),
        ("getCurrentWorldBlock()", "isle_actor_source_current_block(gobj)"),
        ("pokemonAddOne(block, block, &spawn, def)", "isle_actor_source_add_one(gobj, block, block, &spawn, def)"),
        ("        PokemonTransformBase* transform2 = GET_TRANSFORM_BASE(gobj->data.dobj);\n        PokemonTransformBase* transform1 = GET_TRANSFORM_BASE(pokemonObj->data.dobj);\n\n        transform1->xform.pos.v.x = transform2->xform.pos.v.x;\n        transform1->xform.pos.v.y = transform2->xform.pos.v.y;\n        transform1->xform.pos.v.z = transform2->xform.pos.v.z;", "        isle_actor_source_copy_position(gobj, pokemonObj);"),
    )),
)
MOVEMENT_DEFINITIONS = (
    (POKEMON, "Pokemon_GetGroundAt", 425, 432, (
        ("Pokemon_GetGroundAt(f32 x, f32 z)", "Pokemon_GetGroundAt(GObj* obj, f32 x, f32 z)"),
        ("getGroundAt(x, z, &result)", "isle_actor_source_ground(obj, x, z, &result)"),
    )),
    (POKEMON, "Pokemon_WrapAngleSum", 434, 442, ()),
    (POKEMON, "Pokemon_HeadingBetweenPoints", 535, 558, (
        ("Vec3f* pointTo, Vec3f* pointFrom", "const IsleGameVec3* pointTo, const IsleGameVec3* pointFrom"),
    )),
    (POKEMON, "Pokemon_GetHeadingVector", 560, 588, (
        ("Vec3f* pointTo, Vec3f* pointFrom, Vec3f* outVec", "const IsleGameVec3* pointTo, const IsleGameVec3* pointFrom, IsleGameVec3* outVec"),
    )),
    (POKEMON, "Pokemon_SetScale", 887, 900, (
        ("    DObj* model = obj->data.dobj;", "    IsleGameModelRef model = isle_actor_source_self_model(obj);"),
        ("    PokemonInitData* initData = pokemon->initData;", "    IsleActorScaleInitialization initData = isle_actor_source_scale_initialization(obj);\n    IsleGameModelTransform values;"),
        ("    GET_TRANSFORM(model)->scale.v.x = initData->scale.x * 0.1f * newScale;\n    GET_TRANSFORM(model)->scale.v.y = initData->scale.y * 0.1f * newScale;\n    GET_TRANSFORM(model)->scale.v.z = initData->scale.z * 0.1f * newScale;", "    values.scale.x = initData.scale.x * 0.1f * newScale;\n    values.scale.y = initData.scale.y * 0.1f * newScale;\n    values.scale.z = initData.scale.z * 0.1f * newScale;\n    isle_actor_source_aux_store(obj, &model, ISLE_ACTOR_SX | ISLE_ACTOR_SY | ISLE_ACTOR_SZ, &values);"),
        ("initData->collisionCenter", "initData.collision_center", 3),
        ("initData->scale", "initData.scale", 4),
        ("initData->radius", "initData.radius"),
    )),
    (POKEMON, "Pokemon_MoveXZ", 919, 929, (
        ("    DObj* model = obj->data.dobj;", "    IsleGameModelRef model = isle_actor_source_self_model(obj);"),
        ("GET_TRANSFORM(model)->rot.v.y", "isle_actor_source_aux(obj, &model)->rotation.y"),
        ("    GET_TRANSFORM(model)->pos.v.x += speedX;\n    GET_TRANSFORM(model)->pos.v.z += speedZ;", "    IsleGameModelTransform values;\n    values.position.x = isle_actor_source_aux(obj, &model)->position.x;\n    values.position.x += speedX;\n    values.position.z = isle_actor_source_aux(obj, &model)->position.z;\n    values.position.z += speedZ;\n    isle_actor_source_aux_store(obj, &model, ISLE_ACTOR_PX | ISLE_ACTOR_PZ, &values);"),
    )),
    (POKEMON, "Pokemon_Jump", 931, 966, (
        ("    DObj* model = obj->data.dobj;\n    PokemonTransform* transform = GET_TRANSFORM(model);", "    IsleGameModelRef model = isle_actor_source_self_model(obj);"),
        ("            transform->pos.v.y += jumpVel;", "            IsleGameModelTransform values;\n            values.position.y = isle_actor_source_aux(obj, &model)->position.y;\n            values.position.y += jumpVel;\n            isle_actor_source_aux_store(obj, &model, ISLE_ACTOR_PY, &values);"),
    )),
    (POKEMON, "Pokemon_JumpToHeight", 968, 999, (
        ("    DObj* model = obj->data.dobj;\n    PokemonTransform* transform = GET_TRANSFORM(model);", "    IsleGameModelRef model = isle_actor_source_self_model(obj);"),
        ("            transform->pos.v.y += jumpVel;", "            IsleGameModelTransform values;\n            values.position.y = isle_actor_source_aux(obj, &model)->position.y;\n            values.position.y += jumpVel;\n            isle_actor_source_aux_store(obj, &model, ISLE_ACTOR_PY, &values);"),
        ("transform->pos.v.y >= maxHeight", "isle_actor_source_aux(obj, &model)->position.y >= maxHeight"),
    )),
    (POKEMON, "Pokemon_Fall", 1001, 1035, (
        ("    DObj* model = obj->data.dobj;\n    PokemonTransform* transform = GET_TRANSFORM(model);", "    IsleGameModelRef model = isle_actor_source_self_model(obj);"),
        ("            transform->pos.v.y -= fallSpeed;", "            IsleGameModelTransform values;\n            values.position.y = isle_actor_source_aux(obj, &model)->position.y;\n            values.position.y -= fallSpeed;\n            isle_actor_source_aux_store(obj, &model, ISLE_ACTOR_PY, &values);"),
    )),
    (POKEMON, "Pokemon_FallToHeight", 1037, 1068, (
        ("    DObj* model = obj->data.dobj;\n    PokemonTransform* transform = GET_TRANSFORM(model);", "    IsleGameModelRef model = isle_actor_source_self_model(obj);"),
        ("            transform->pos.v.y -= fallSpeed;", "            IsleGameModelTransform values;\n            values.position.y = isle_actor_source_aux(obj, &model)->position.y;\n            values.position.y -= fallSpeed;\n            isle_actor_source_aux_store(obj, &model, ISLE_ACTOR_PY, &values);"),
        ("transform->pos.v.y <= minHeight", "isle_actor_source_aux(obj, &model)->position.y <= minHeight"),
    )),
    (POKEMON, "Pokemon_JumpAndBounceFromGround", 1070, 1119, (
        ("    DObj* model = obj->data.dobj;", "    IsleGameModelRef model = isle_actor_source_self_model(obj);"),
        ("GET_TRANSFORM(model)->rot.v.y", "isle_actor_source_aux(obj, &model)->rotation.y"),
        ("randFloat()", "isle_actor_source_rand_float(obj)"),
        ("getGroundAt(GET_TRANSFORM(model)->pos.v.x, GET_TRANSFORM(model)->pos.v.z, &pokemon->currGround)", "isle_actor_source_ground(obj, isle_actor_source_aux(obj, &model)->position.x, isle_actor_source_aux(obj, &model)->position.z, &pokemon->currGround)"),
        ("GET_TRANSFORM(model)->pos.v.y < pokemon->currGround.height", "isle_actor_source_aux(obj, &model)->position.y < pokemon->currGround.height"),
        ("cmdSendCommand(gObjPlayer, PLAYER_CMD_SHAKE_CAMERA, obj)", "isle_actor_source_send_addressed(obj, isle_actor_source_player(obj), 3, obj)"),
        ("                GET_TRANSFORM(model)->pos.v.y = pokemon->currGround.height;", "                IsleGameModelTransform values;\n                values.position.y = pokemon->currGround.height;\n                isle_actor_source_aux_store(obj, &model, ISLE_ACTOR_PY, &values);"),
        ("        GET_TRANSFORM(model)->pos.v.x += velX;", "        IsleGameModelTransform valuesX;\n        valuesX.position.x = isle_actor_source_aux(obj, &model)->position.x;\n        valuesX.position.x += velX;\n        isle_actor_source_aux_store(obj, &model, ISLE_ACTOR_PX, &valuesX);"),
        ("        GET_TRANSFORM(model)->pos.v.y += velY;", "        IsleGameModelTransform valuesY;\n        valuesY.position.y = isle_actor_source_aux(obj, &model)->position.y;\n        valuesY.position.y += velY;\n        isle_actor_source_aux_store(obj, &model, ISLE_ACTOR_PY, &valuesY);"),
        ("        GET_TRANSFORM(model)->pos.v.z += velZ;", "        IsleGameModelTransform valuesZ;\n        valuesZ.position.z = isle_actor_source_aux(obj, &model)->position.z;\n        valuesZ.position.z += velZ;\n        isle_actor_source_aux_store(obj, &model, ISLE_ACTOR_PZ, &valuesZ);"),
    )),
    (POKEMON, "Pokemon_FallDownOnGround", 1121, 1144, (
        ("    DObj* model = obj->data.dobj;\n    PokemonTransform* transform = GET_TRANSFORM(model);", "    IsleGameModelRef model = isle_actor_source_self_model(obj);"),
        ("getGroundAt(transform->pos.v.x, transform->pos.v.z, &pokemon->currGround)", "isle_actor_source_ground(obj, isle_actor_source_aux(obj, &model)->position.x, isle_actor_source_aux(obj, &model)->position.z, &pokemon->currGround)"),
        ("        transform->pos.v.y += jumpVel;", "        IsleGameModelTransform values;\n        values.position.y = isle_actor_source_aux(obj, &model)->position.y;\n        values.position.y += jumpVel;\n        isle_actor_source_aux_store(obj, &model, ISLE_ACTOR_PY, &values);"),
        ("transform->pos.v.y < pokemon->currGround.height", "isle_actor_source_aux(obj, &model)->position.y < pokemon->currGround.height"),
        ("    transform->pos.v.y = pokemon->currGround.height;", "    IsleGameModelTransform values;\n    values.position.y = pokemon->currGround.height;\n    isle_actor_source_aux_store(obj, &model, ISLE_ACTOR_PY, &values);"),
    )),
    (POKEMON, "Pokemon_StepWalkInDirection", 1212, 1226, (
        ("    DObj* model = obj->data.dobj;\n", ""),
        ("Pokemon_StepWalk(obj, velX, velZ, flags)", "isle_actor_source_step_walk(obj, velX, velZ, flags)"),
    )),
    (POKEMON, "Pokemon_StepWalkInDirectionFacing", 1228, 1243, (
        ("    DObj* model = obj->data.dobj;", "    IsleGameModelRef model = isle_actor_source_self_model(obj);"),
        ("GET_TRANSFORM(model)->rot.v.y", "isle_actor_source_aux(obj, &model)->rotation.y"),
        ("Pokemon_StepWalk(obj, velX, velZ, flags)", "isle_actor_source_step_walk(obj, velX, velZ, flags)"),
    )),
    (POKEMON, "Pokemon_Turn", 1245, 1269, (
        ("Pokemon_Turn(DObj* model, f32 targetYaw, f32 turnSpeed)", "Pokemon_Turn(GObj* obj, IsleGameModelRef model, f32 targetYaw, f32 turnSpeed)"),
        ("    yaw = GET_TRANSFORM(model)->rot.v.y;", "    yaw = isle_actor_source_aux(obj, &model)->rotation.y;"),
        ("        GET_TRANSFORM(model)->rot.v.y = yaw + sign * turnSpeed;", "        IsleGameModelTransform values;\n        values.rotation.y = yaw + sign * turnSpeed;\n        isle_actor_source_aux_store(obj, &model, ISLE_ACTOR_RY, &values);"),
        ("        GET_TRANSFORM(model)->rot.v.y = targetYaw;", "        IsleGameModelTransform values;\n        values.rotation.y = targetYaw;\n        isle_actor_source_aux_store(obj, &model, ISLE_ACTOR_RY, &values);"),
    )),
    (POKEMON, "Pokemon_RunInCircles", 1271, 1332, (
        ("    DObj* model = obj->data.dobj;", "    IsleGameModelRef model = isle_actor_source_self_model(obj);"),
        ("    Vec3f centerPos;", "    IsleGameVec3 centerPos;"),
        ("randRange(300)", "isle_actor_source_rand_range(obj, 300)"),
        ("GET_TRANSFORM(model)->rot.v.y", "isle_actor_source_aux(obj, &model)->rotation.y", 6),
        ("GET_TRANSFORM(model)->pos.v.x", "isle_actor_source_aux(obj, &model)->position.x", 2),
        ("GET_TRANSFORM(model)->pos.v.z", "isle_actor_source_aux(obj, &model)->position.z", 2),
        ("Pokemon_StepWalk(obj, deltaX, deltaZ, MOVEMENT_FLAG_ON_GROUND)", "isle_actor_source_step_walk(obj, deltaX, deltaZ, MOVEMENT_FLAG_ON_GROUND)"),
        ("            isle_actor_source_aux(obj, &model)->rotation.y = Pokemon_WrapAngleSum(currentYaw, sign * maxModelTurnSpeed);", "            IsleGameModelTransform values;\n            values.rotation.y = Pokemon_WrapAngleSum(currentYaw, sign * maxModelTurnSpeed);\n            isle_actor_source_aux_store(obj, &model, ISLE_ACTOR_RY, &values);"),
        ("            isle_actor_source_aux(obj, &model)->rotation.y = nextYaw;", "            IsleGameModelTransform values;\n            values.rotation.y = nextYaw;\n            isle_actor_source_aux_store(obj, &model, ISLE_ACTOR_RY, &values);"),
    )),
    (POKEMON, "Pokemon_RunInCirclesDefault", 1334, 1336, ()),
    (POKEMON, "Pokemon_HearsPokeFlute", 1338, 1346, ()),
    (POKEMON, "Pokemon_TurnToTarget", 1348, 1391, (
        ("    DObj* model = obj->data.dobj;", "    IsleGameModelRef model = isle_actor_source_self_model(obj);"),
        ("    Vec3f* targetPos;\n    Vec3f* currentPos;\n    DObj* targetModel;", "    const IsleGameVec3* targetPos;\n    const IsleGameVec3* currentPos;\n    IsleActorPositionRef targetPosition;\n    IsleGameObject* targetObject;"),
        ("targetModel = gPlayerDObj;", "targetObject = isle_actor_source_player(obj);"),
        ("targetModel = pokemon->interactionTarget->data.dobj;", "targetObject = pokemon->interactionTarget;"),
        ("    if (GET_TRANSFORM_BASE(targetModel) != NULL) {\n        targetPos = &GET_TRANSFORM(targetModel)->pos.v;\n    } else {\n        targetPos = &targetModel->position.v;\n    }\n    currentPos = &GET_TRANSFORM(model)->pos.v;", "    targetPosition = isle_actor_source_target_position_ref(obj, targetObject);"),
        ("            if (flags & MOVEMENT_FLAG_TURN_AWAY) {", "            targetPos = isle_actor_source_position_read(obj, &targetPosition);\n            currentPos = &isle_actor_source_aux(obj, &model)->position;\n            if (flags & MOVEMENT_FLAG_TURN_AWAY) {"),
        ("Pokemon_Turn(model, targetDir, turnSpeed)", "Pokemon_Turn(obj, model, targetDir, turnSpeed)"),
    )),
    (POKEMON, "Pokemon_TurnToModelNode", 1393, 1425, (
        ("DObj* modelNode", "IsleActorNodeRef modelNode"),
        ("    DObj* model = obj->data.dobj;", "    IsleGameModelRef model = isle_actor_source_self_model(obj);"),
        ("    Vec3f* currentPos;\n    Vec3f targetPos;\n    Vec3f vel = { 0, 0, 0 };", "    const IsleGameVec3* currentPos;\n    IsleGameVec3 targetPos;\n    IsleGameVec3 vel = { 0, 0, 0 };"),
        ("    currentPos = &GET_TRANSFORM(model)->pos.v;\n", ""),
        ("fx_getPosVelDObj(&targetPos, &vel, modelNode)", "isle_actor_source_node_world_ref(obj, &modelNode, &targetPos, &vel)"),
        ("            if (flags & MOVEMENT_FLAG_TURN_AWAY) {", "            currentPos = &isle_actor_source_aux(obj, &model)->position;\n            if (flags & MOVEMENT_FLAG_TURN_AWAY) {"),
        ("Pokemon_Turn(model, targetDir, turnSpeed)", "Pokemon_Turn(obj, model, targetDir, turnSpeed)"),
    )),
    (POKEMON, "func_80361720_501B30", 1427, 1431, ()),
    (POKEMON, "Pokemon_RunToTarget", 1433, 1483, (
        ("    DObj* model = obj->data.dobj;", "    IsleGameModelRef model = isle_actor_source_self_model(obj);"),
        ("    DObj* targetModel;\n    Vec3f* targetPos;", "    IsleActorPositionRef targetPosition;\n    const IsleGameVec3* targetPos;"),
        ("    Vec3f* currentPos;\n    Vec3f walkDir;", "    const IsleGameVec3* currentPos;\n    IsleGameVec3 walkDir;"),
        ("            targetModel = pokemon->interactionTarget->data.dobj;\n            if (GET_TRANSFORM_BASE(targetModel) != NULL) {\n                targetPos = &GET_TRANSFORM(targetModel)->pos.v;\n                pokemon->targetPos.x = GET_TRANSFORM(targetModel)->pos.v.x;\n                pokemon->targetPos.y = GET_TRANSFORM(targetModel)->pos.v.y;\n                pokemon->targetPos.z = GET_TRANSFORM(targetModel)->pos.v.z;\n            } else {\n                targetPos = &targetModel->position.v;\n                pokemon->targetPos.x = targetModel->position.v.x;\n                pokemon->targetPos.y = targetModel->position.v.y;\n                pokemon->targetPos.z = targetModel->position.v.z;\n            }\n            currentPos = &GET_TRANSFORM(model)->pos.v;", "            targetPosition = isle_actor_source_target_position_ref(obj, pokemon->interactionTarget);\n            targetPos = isle_actor_source_position_read(obj, &targetPosition);\n            pokemon->targetPos.x = targetPos->x;\n            pokemon->targetPos.y = targetPos->y;\n            pokemon->targetPos.z = targetPos->z;\n            currentPos = &isle_actor_source_aux(obj, &model)->position;"),
        ("Pokemon_StepWalk(obj, walkDir.x, walkDir.z, flags | MOVEMENT_FLAG_ON_GROUND)", "isle_actor_source_step_walk(obj, walkDir.x, walkDir.z, flags | MOVEMENT_FLAG_ON_GROUND)"),
        ("        Pokemon_Turn(model, targetYaw, turnSpeed);", "        Pokemon_Turn(obj, model, targetYaw, turnSpeed);\n        currentPos = &isle_actor_source_aux(obj, &model)->position;"),
    )),
    (POKEMON, "Pokemon_RunAwayFromTarget", 1485, 1534, (
        ("    DObj* model = obj->data.dobj;", "    IsleGameModelRef model = isle_actor_source_self_model(obj);"),
        ("    DObj* targetModel;\n    Vec3f* targetPos;", "    IsleActorPositionRef targetPosition;\n    const IsleGameVec3* targetPos;"),
        ("    Vec3f* currentPos;\n    Vec3f walkDir;", "    const IsleGameVec3* currentPos;\n    IsleGameVec3 walkDir;"),
        ("            targetModel = pokemon->interactionTarget->data.dobj;\n            if (GET_TRANSFORM_BASE(targetModel) != NULL) {\n                targetPos = &GET_TRANSFORM(targetModel)->pos.v;\n                pokemon->targetPos.x = GET_TRANSFORM(targetModel)->pos.v.x;\n                pokemon->targetPos.y = GET_TRANSFORM(targetModel)->pos.v.y;\n                pokemon->targetPos.z = GET_TRANSFORM(targetModel)->pos.v.z;\n            } else {\n                targetPos = &targetModel->position.v;\n                pokemon->targetPos.x = targetModel->position.v.x;\n                pokemon->targetPos.y = targetModel->position.v.y;\n                pokemon->targetPos.z = targetModel->position.v.z;\n            }\n            currentPos = &GET_TRANSFORM(model)->pos.v;", "            targetPosition = isle_actor_source_target_position_ref(obj, pokemon->interactionTarget);\n            targetPos = isle_actor_source_position_read(obj, &targetPosition);\n            pokemon->targetPos.x = targetPos->x;\n            pokemon->targetPos.y = targetPos->y;\n            pokemon->targetPos.z = targetPos->z;\n            currentPos = &isle_actor_source_aux(obj, &model)->position;"),
        ("Pokemon_StepWalk(obj, walkDir.x, walkDir.z, flags | MOVEMENT_FLAG_ON_GROUND)", "isle_actor_source_step_walk(obj, walkDir.x, walkDir.z, flags | MOVEMENT_FLAG_ON_GROUND)"),
        ("        Pokemon_Turn(model, targetYaw, turnSpeed);", "        Pokemon_Turn(obj, model, targetYaw, turnSpeed);\n        currentPos = &isle_actor_source_aux(obj, &model)->position;"),
    )),
    (POKEMON, "Pokemon_RunAwayDefault", 1536, 1538, ()),
    (POKEMON, "Pokemon_SetTargetPos", 1540, 1545, ()),
    (POKEMON, "Pokemon_StepToTargetPos", 1547, 1608, (
        ("    DObj* model = obj->data.dobj;", "    IsleGameModelRef model = isle_actor_source_self_model(obj);"),
        ("GET_TRANSFORM(model)->pos.v.x", "isle_actor_source_aux(obj, &model)->position.x", 2),
        ("GET_TRANSFORM(model)->pos.v.z", "isle_actor_source_aux(obj, &model)->position.z", 2),
        ("Pokemon_StepWalk(obj, velX, velZ, flags)", "isle_actor_source_step_walk(obj, velX, velZ, flags)"),
        ("    currentYaw = GET_TRANSFORM(model)->rot.v.y;", "    currentYaw = isle_actor_source_aux(obj, &model)->rotation.y;"),
        ("        GET_TRANSFORM(model)->rot.v.y = currentYaw + sign * turnSpeed;", "        IsleGameModelTransform values;\n        values.rotation.y = currentYaw + sign * turnSpeed;\n        isle_actor_source_aux_store(obj, &model, ISLE_ACTOR_RY, &values);"),
        ("        GET_TRANSFORM(model)->rot.v.y = targetYaw;", "        IsleGameModelTransform values;\n        values.rotation.y = targetYaw;\n        isle_actor_source_aux_store(obj, &model, ISLE_ACTOR_RY, &values);"),
    )),
    (POKEMON, "Pokemon_RunToTargetPos", 1610, 1614, ()),
    (POKEMON, "Pokemon_RunToTargetDefault", 1616, 1618, ()),
    (POKEMON, "Pokemon_TurnToPokeFlute", 1620, 1647, (
        ("    DObj* model = obj->data.dobj;", "    IsleGameModelRef model = isle_actor_source_self_model(obj);"),
        ("    DObj* targetModel;\n    Vec3f* targetPos;\n    Vec3f* currentPos;", "    IsleActorPositionRef targetPosition;\n    const IsleGameVec3* targetPos;\n    const IsleGameVec3* currentPos;"),
        ("    targetModel = pokemon->interactionTarget->data.dobj;\n    if (GET_TRANSFORM_BASE(targetModel) != NULL) {\n        targetPos = &GET_TRANSFORM(targetModel)->pos.v;\n    } else {\n        targetPos = &targetModel->position.v;\n    }\n    currentPos = &GET_TRANSFORM(model)->pos.v;", "    targetPosition = isle_actor_source_target_position_ref(obj, pokemon->interactionTarget);"),
        ("        Pokemon_Turn(model, Pokemon_HeadingBetweenPoints(targetPos, currentPos), turnSpeed);", "        targetPos = isle_actor_source_position_read(obj, &targetPosition);\n        currentPos = &isle_actor_source_aux(obj, &model)->position;\n        Pokemon_Turn(obj, model, Pokemon_HeadingBetweenPoints(targetPos, currentPos), turnSpeed);"),
    )),
    (POKEMON, "func_80361FBC_5023CC", 1649, 1664, (
        ("func_80361FBC_5023CC(Vec3f* pos, InterpData* path, f32 arg2)", "func_80361FBC_5023CC(GObj* obj, IsleGameVec3* pos, const void* path, f32 arg2)"),
        ("switch (path->type)", "switch (isle_actor_source_path_info(obj, path).kind)"),
        ("HAL_INTERP_BEZIER_S3", "ISLE_ACTOR_PATH_BEZIER_S3"),
        ("HAL_INTERP_BEZIER", "ISLE_ACTOR_PATH_BEZIER"),
        ("HAL_INTERP_CATROM", "ISLE_ACTOR_PATH_CATROM"),
        ("HAL_INTERP_LINEAR", "ISLE_ACTOR_PATH_LINEAR"),
        ("GetInterpolatedPosition(pos, path, arg2)", "isle_actor_source_path_position(obj, path, arg2, pos)", 4),
    )),
    (POKEMON, "Pokemon_ResetPathPos", 1666, 1674, (
        ("    DObj* model = obj->data.dobj;", "    IsleGameModelRef model = isle_actor_source_self_model(obj);"),
        ("    WorldBlock* block = getCurrentWorldBlock();", "    IsleGameVec3 worldPosition = isle_actor_source_current_world_position(obj);\n    IsleGameModelTransform values;"),
        ("    GET_TRANSFORM(model)->pos.v.x = -(block->descriptor->worldPos.x * 100.0f);\n    GET_TRANSFORM(model)->pos.v.y = -(block->descriptor->worldPos.y * 100.0f);\n    GET_TRANSFORM(model)->pos.v.z = -(block->descriptor->worldPos.z * 100.0f);", "    values.position.x = -(worldPosition.x * 100.0f);\n    values.position.y = -(worldPosition.y * 100.0f);\n    values.position.z = -(worldPosition.z * 100.0f);\n    isle_actor_source_aux_store(obj, &model, ISLE_ACTOR_PX | ISLE_ACTOR_PY | ISLE_ACTOR_PZ, &values);"),
    )),
    (POKEMON, "Pokemon_FollowPath", 1676, 1754, (
        ("    DObj* model = obj->data.dobj;", "    IsleGameModelRef model = isle_actor_source_self_model(obj);"),
        ("    Vec3f worldPos = { 0, 0, 0 };", "    IsleGameVec3 worldPos = { 0, 0, 0 };"),
        ("    Vec3f vel;", "    IsleGameVec3 vel;"),
        ("pokemon->path->duration", "isle_actor_source_path_info(obj, pokemon->path).duration"),
        ("            GET_TRANSFORM(model)->pos.v.x -= worldPos.x * 100.0f;\n            GET_TRANSFORM(model)->pos.v.z -= worldPos.z * 100.0f;", "            IsleGameModelTransform values;\n            values.position.x = isle_actor_source_aux(obj, &model)->position.x;\n            values.position.x -= worldPos.x * 100.0f;\n            values.position.z = isle_actor_source_aux(obj, &model)->position.z;\n            values.position.z -= worldPos.z * 100.0f;\n            isle_actor_source_aux_store(obj, &model, ISLE_ACTOR_PX | ISLE_ACTOR_PZ, &values);"),
        ("                    GET_TRANSFORM(model)->pos.v.y -= worldPos.y;", "                    values.position.y = isle_actor_source_aux(obj, &model)->position.y;\n                    values.position.y -= worldPos.y;\n                    isle_actor_source_aux_store(obj, &model, ISLE_ACTOR_PY, &values);"),
        ("                    GET_TRANSFORM(model)->pos.v.y -= worldPos.y * 100.0f;", "                    values.position.y = isle_actor_source_aux(obj, &model)->position.y;\n                    values.position.y -= worldPos.y * 100.0f;\n                    isle_actor_source_aux_store(obj, &model, ISLE_ACTOR_PY, &values);"),
        ("GetInterpolatedVelocity(&vel, pokemon->path, pokemon->pathParam)", "isle_actor_source_path_velocity(obj, pokemon->path, pokemon->pathParam, &vel)"),
        ("Pokemon_Turn(model, heading, turnSpeed)", "Pokemon_Turn(obj, model, heading, turnSpeed)"),
        ("                    GET_TRANSFORM(model)->rot.v.y = heading;", "                    values.rotation.y = heading;\n                    isle_actor_source_aux_store(obj, &model, ISLE_ACTOR_RY, &values);"),
        ("func_80361FBC_5023CC(&worldPos, pokemon->path, pokemon->pathParam)", "func_80361FBC_5023CC(obj, &worldPos, pokemon->path, pokemon->pathParam)"),
        ("            GET_TRANSFORM(model)->pos.v.x += worldPos.x * 100.0f;\n            GET_TRANSFORM(model)->pos.v.z += worldPos.z * 100.0f;", "            values.position.x = isle_actor_source_aux(obj, &model)->position.x;\n            values.position.x += worldPos.x * 100.0f;\n            values.position.z = isle_actor_source_aux(obj, &model)->position.z;\n            values.position.z += worldPos.z * 100.0f;\n            isle_actor_source_aux_store(obj, &model, ISLE_ACTOR_PX | ISLE_ACTOR_PZ, &values);"),
        ("Pokemon_GetGroundAt(GET_TRANSFORM(model)->pos.v.x, GET_TRANSFORM(model)->pos.v.z)", "Pokemon_GetGroundAt(obj, isle_actor_source_aux(obj, &model)->position.x, isle_actor_source_aux(obj, &model)->position.z)"),
        ("                    GET_TRANSFORM(model)->pos.v.y += worldPos.y;", "                    values.position.y = isle_actor_source_aux(obj, &model)->position.y;\n                    values.position.y += worldPos.y;\n                    isle_actor_source_aux_store(obj, &model, ISLE_ACTOR_PY, &values);"),
        ("                    GET_TRANSFORM(model)->pos.v.y += worldPos.y * 100.0f;", "                    values.position.y = isle_actor_source_aux(obj, &model)->position.y;\n                    values.position.y += worldPos.y * 100.0f;\n                    isle_actor_source_aux_store(obj, &model, ISLE_ACTOR_PY, &values);"),
    )),
    (POKEMON, "func_803623F4_502804", 1756, 1758, (
        ("func_803623F4_502804(f32 x, f32 z)", "func_803623F4_502804(GObj* obj, f32 x, f32 z)"),
        ("Pokemon_GetGroundAt(x, z)", "Pokemon_GetGroundAt(obj, x, z)"),
    )),
)
INITIAL_SPECIES_FILES = ("src/beach/55FB80.c", "src/beach/560320.c", "src/beach/560620.c", "src/beach/563000.c", "src/beach/562370.c")
INITIAL_SPECIES_UNITS = (
    (0, "802C7B10", 4, 21), (0, "802C7BB8", 24, 37),
    (0, "802C7C7C", 39, 48), (0, "802C7CF0", 50, 63),
    (0, "802C7DA0", 65, 72), (0, "802C7DEC", 74, 81),
    (0, "802C7E38", 83, 85), (1, "802C82B0", 4, 17),
    (1, "802C8354", 19, 30), (1, "802C83D0", 32, 41),
    (1, "802C8438", 43, 52), (1, "802C84A4", 54, 71),
    (1, "802C8570", 73, 75),
)

def initial_species_changes(identity):
    changes = []
    animations = {
        "802C7B10": (("obj", "802CCD24", "isle_actor_source_rand_float(obj) * 100.0f"),),
        "802C7BB8": (("arg0", "802CCD24", None), ("arg0", "802CCD38", None)),
        "802C7CF0": (("obj", "802CCD4C", None), ("obj", "802CCD24", None)),
        "802C8354": (("obj", "802CCF00", None),),
        "802C83D0": (("obj", "802CCEB0", None),),
        "802C84A4": (("obj", "802CCEC4", None), ("obj", "802CCED8", None), ("obj", "802CCEEC", None)),
    }
    for obj, header, start in animations.get(identity, ()):
        old = (f"Pokemon_SetAnimationAtTime({obj}, &D_beach_{header}, randFloat() * 100.0f)"
               if start else f"Pokemon_SetAnimation({obj}, &D_beach_{header})")
        changes.append((old, f"isle_actor_source_set_animation({obj}, UINT32_C(0x{header.lower()}), {start or '0.0f'}, 0)"))
    if identity == "802C7B10":
        changes += [
            ("    DObj* model = obj->data.dobj;", "    IsleGameModelRef model = isle_actor_source_self_model(obj);"),
            ("    Mtx3Float* position = &GET_TRANSFORM(model)->pos;", "    const IsleGameVec3* position = &isle_actor_source_aux(obj, &model)->position;"),
            ("position->v.y", "position->y"),
        ]
    if identity == "802C7BB8":
        changes.append(("randRange(600)", "isle_actor_source_rand_range(arg0, 600)"))
    if identity == "802C7C7C":
        changes.append(("randFloat()", "isle_actor_source_rand_float(obj)"))
    graphs = {"802C7BB8": ("802CCD60", 2), "802C7CF0": ("802CCD60", 1),
              "802C82B0": ("802CCF44", 1), "802C8354": ("802CCF14", 1),
              "802C83D0": ("802CCF14", 1)}
    if identity in graphs:
        graph, count = graphs[identity]
        changes.append((f"D_beach_{graph}", f"isle_actor_source_graph(obj, UINT32_C(0x{graph.lower()}))" if identity != "802C7BB8" else f"isle_actor_source_graph(arg0, UINT32_C(0x{graph.lower()}))", count))
    if identity == "802C82B0":
        changes.append(("obj->flags |= GOBJ_FLAG_HIDDEN;", "isle_actor_source_object_flags(obj, UINT32_C(1), UINT32_C(1));"))
    if identity == "802C8354":
        changes.append(("obj->flags = 0;", "isle_actor_source_object_flags(obj, UINT32_MAX, 0);"))
    return tuple(changes)

DEFINITIONS += tuple((INITIAL_SPECIES_FILES[file], "func_beach_" + identity, first, last,
                      initial_species_changes(identity))
                     for file, identity, first, last in INITIAL_SPECIES_UNITS)

# Full ordinary25/16/131 executable closure; dead local helpers are excluded.
ENTRY_SPECIES_UNITS = (
    ('src/beach/55FB80.c', '802C7E70', 89, 106),
    ('src/beach/55FB80.c', '802C7F1C', 108, 116),
    ('src/beach/55FB80.c', '802C7F74', 118, 130),
    ('src/beach/55FB80.c', '802C8028', 132, 148),
    ('src/beach/55FB80.c', '802C80BC', 150, 153),
    ('src/beach/55FB80.c', '802C80F0', 155, 158),
    ('src/beach/55FB80.c', '802C8124', 160, 164),
    ('src/beach/55FB80.c', '802C816C', 166, 176),
    ('src/beach/55FB80.c', '802C81C4', 178, 188),
    ('src/beach/55FB80.c', '802C821C', 190, 200),
    ('src/beach/55FB80.c', '802C8274', 202, 204),
    ('src/beach/560620.c', '802C85B0', 9, 12),
    ('src/beach/560620.c', '802C85E0', 15, 33),
    ('src/beach/560620.c', '802C86A8', 35, 58),
    ('src/beach/560620.c', '802C87BC', 60, 70),
    ('src/beach/560620.c', '802C8828', 72, 82),
    ('src/beach/560620.c', '802C8894', 84, 98),
    ('src/beach/560620.c', '802C8900', 100, 115),
    ('src/beach/560620.c', '802C8980', 117, 119),
    ('src/beach/560620.c', '802C89A4', 124, 150),
    ('src/beach/560620.c', '802C8AAC', 152, 168),
    ('src/beach/560620.c', '802C8B60', 170, 180),
    ('src/beach/560620.c', '802C8BC4', 182, 194),
    ('src/beach/560620.c', '802C8C30', 196, 200),
    ('src/beach/560620.c', '802C8C58', 202, 206),
    ('src/beach/560620.c', '802C8C7C', 208, 215),
    ('src/beach/560620.c', '802C8CC0', 217, 220),
    ('src/beach/560620.c', '802C8CFC', 222, 228),
    ('src/beach/560620.c', '802C8D3C', 230, 232),
    ('src/beach/562BC0.c', '802CAC80', 36, 45),
    ('src/beach/562BC0.c', '802CACD4', 47, 60),
    ('src/beach/562BC0.c', '802CAD58', 62, 64),
    ('src/beach/562BC0.c', '802CAD7C', 66, 73),
    ('src/beach/562BC0.c', '802CADCC', 75, 81),
    ('src/beach/562BC0.c', '802CAE14', 83, 96),
    ('src/beach/562BC0.c', '802CAEB0', 98, 107),
    ('src/beach/562BC0.c', '802CAF1C', 109, 119),
    ('src/beach/563000.c', '802CAF90', 9, 17),
    ('src/beach/563000.c', '802CAFEC', 19, 28),
    ('src/beach/563000.c', '802CB054', 30, 33),
    ('src/beach/563000.c', '802CB08C', 35, 48),
    ('src/beach/563000.c', '802CB128', 50, 65),
    ('src/beach/563000.c', '802CB194', 69, 90),
    ('src/beach/563000.c', '802CB2D8', 92, 104),
    ('src/beach/563000.c', '802CB36C', 106, 115),
    ('src/beach/563000.c', '802CB3DC', 117, 126),
    ('src/beach/563000.c', '802CB44C', 128, 137),
    ('src/beach/563000.c', '802CB520', 153, 162),
    ('src/beach/563000.c', '802CB594', 165, 189),
    ('src/beach/563000.c', '802CB6B4', 191, 200),
    ('src/beach/563000.c', '802CB710', 202, 212),
    ('src/beach/563000.c', '802CB784', 214, 226),
    ('src/beach/563000.c', '802CB814', 228, 235),
    ('src/beach/563000.c', '802CB874', 238, 266),
    ('src/beach/563000.c', '802CB9E0', 268, 277),
    ('src/beach/563000.c', '802CBA48', 279, 297),
    ('src/beach/563000.c', '802CBB34', 299, 308),
    ('src/beach/563000.c', '802CBB90', 310, 324),
    ('src/beach/563000.c', '802CBC4C', 326, 334),
    ('src/beach/563000.c', '802CBCA8', 336, 338),
)
ENTRY_ANIMATIONS = {
    '802C7F74': (('Force', '802CCE08'), ('Force', '802CCDE8')),
    '802C86A8': (('Set', '802CCFC0'), ('Set', '802CCFD4'), ('Set', '802CCFE8'), ('Set', '802CCFFC')),
    '802C89A4': (('Set', '802CCFC0'), ('Set', '802CCFD4'), ('Set', '802CCFE8'), ('Set', '802CCFFC')),
    '802C8AAC': (('Set', '802CCFD4'),),
    '802CAE14': (('Set', '802CDBAC'), ('Set', '802CDBFC')),
    '802CAF1C': (('Set', '802CDBC0'),),
    '802CAF90': (('Force', '802CDBD4'),),
    '802CAFEC': (('Set', '802CDBC0'),),
    '802CB08C': (('Force', '802CDB84'),),
    '802CB2D8': (('Set', '802CDB5C'),),
    '802CB36C': (('Set', '802CDB5C'),),
    '802CB3DC': (('Force', '802CDB70'),),
    '802CB44C': (('Force', '802CDBC0'),),
    '802CB520': (('Set', '802CDBD4'),),
    '802CB594': (('Set', '802CDBAC'), ('Set', '802CDC10')),
    '802CB784': (('Force', '802CDB84'),),
    '802CB874': (('Force', '802CDB70'), ('Set', '802CDBE8'), ('Set', '802CDBAC'), ('Set', '802CDBD4'), ('Set', '802CDBFC')),
    '802CB9E0': (('Force', '802CDBC0'),),
    '802CBB90': (('Force', '802CDBD4'), ('Set', '802CDBFC'), ('Force', '802CDBFC')),
    '802CBC4C': (('Force', '802CDBC0'),),
}
ENTRY_GRAPHS = {
    '802C7F1C': (('802CCE1C', 1),),
    '802C85E0': (('802CD040', 1),),
    '802C86A8': (('802CD010', 4),),
    '802C8828': (('802CD010', 1),),
    '802C89A4': (('802CD064', 4),),
    '802C8C7C': (('802CD0B4', 1),),
    '802C8CFC': (('802CD0E4', 1),),
    '802CAC80': (('802CDC60', 1),),
    '802CAD7C': (('802CDC80', 1),),
    '802CAE14': (('802CDCA0', 2),),
    '802CAF90': (('802CDCD0', 1),),
    '802CB08C': (('802CDD40', 1),),
    '802CB2D8': (('802CDD80', 1),),
    '802CB36C': (('802CDD80', 1),),
    '802CB3DC': (('802CDD80', 1),),
    '802CB44C': (('802CDE48', 1),),
    '802CB520': (('802CDE78', 1),),
    '802CB594': (('802CDEF8', 3),),
    '802CB874': (('802CDF98', 3), ('802CDFD8', 3)),
    '802CBB90': (('802CDFD8', 3),),
    '802CBC4C': (('802CDFD8', 1),),
}
def entry_species_changes(identity):
    changes = []
    for mode, header in ENTRY_ANIMATIONS.get(identity, ()):
        changes.append((f"Pokemon_{mode}Animation(obj, &D_beach_{header})",
                        f"isle_actor_source_set_animation(obj, UINT32_C(0x{header.lower()}), 0.0f, {int(mode == 'Force')})"))
    for graph, count in ENTRY_GRAPHS.get(identity, ()):
        object_name = "arg0" if identity == "802C85E0" else "obj"
        changes.append((f"D_beach_{graph}", f"isle_actor_source_graph({object_name}, UINT32_C(0x{graph.lower()}))", count))
    if identity in ("802C7E70", "802C7F74", "802CAD7C"):
        changes.append(("HIDE_POKEMON();", "isle_actor_source_visibility(obj, 1);"))
    if identity in ("802C7F74", "802CADCC", "802CAE14"):
        changes.append(("SHOW_POKEMON();", "isle_actor_source_visibility(obj, 0);"))
    if identity == "802C7F74":
        changes.append(("randRange(600)", "isle_actor_source_rand_range(obj, 600)"))
    for address in {"802C80BC": ("802CCDFC",), "802C80F0": ("802CCE00",),
                    "802C8124": ("802CCE00", "802CCE04"), "802C816C": ("802CCDFC",),
                    "802C81C4": ("802CCE00",), "802C821C": ("802CCE04",),
                    "802C8AAC": ("802CD060",)}.get(identity, ()):
        changes.append((f"D_beach_{address}", f"(*isle_actor_source_scene_counter(obj, UINT32_C(0x{address.lower()})))"))
    if identity == "802C85B0":
        changes.append(("arg0->flags |= 1;", "isle_actor_source_object_flags(arg0, UINT32_C(1), UINT32_C(1));"))
    if identity in ("802C86A8", "802C89A4", "802C8C7C"):
        changes.append(("obj->flags = 0;", "isle_actor_source_object_flags(obj, UINT32_MAX, 0);"))
    if identity in ("802CB2D8", "802CB36C", "802CB3DC"):
        changes.append(("D_beach_802CDE30", "isle_actor_source_weighted(obj, UINT32_C(0x802cde30))"))
    if identity == "802CB594":
        changes.append(("Pokemon_EatApple(obj)", "isle_actor_source_eat_apple(obj)"))
    if identity == "802CB194":
        changes += [
            ("    Vec3f sp2C = D_beach_802CE018;", "    IsleGameVec3 sp2C = isle_actor_source_pikachu_temporary(obj);"),
            ("    WorldBlock* block;", "    IsleGameVec3 worldPosition;"),
            ("    DObj* model = obj->data.dobj;", "    IsleGameModelRef model = isle_actor_source_self_model(obj);"),
            ("    block = getCurrentWorldBlock();", "    worldPosition = isle_actor_source_current_world_position(obj);"),
            ("block->descriptor->worldPos.x", "worldPosition.x"),
            ("block->descriptor->worldPos.y", "worldPosition.y"),
            ("block->descriptor->worldPos.z", "worldPosition.z"),
            ("GetInterpolatedPosition(&sp2C, pokemon->path, 0.99999f)", "isle_actor_source_path_position(obj, pokemon->path, 0.99999f, &sp2C)"),
            ("GET_TRANSFORM(model)->pos.v.x", "isle_actor_source_aux(obj, &model)->position.x"),
            ("GET_TRANSFORM(model)->pos.v.z", "isle_actor_source_aux(obj, &model)->position.z"),
        ]
    if identity == "802CBA48":
        changes += [
            ("    DObj* model;", "    IsleGameModelRef model;"),
            ("    Vec3f sp34;", "    IsleGameVec3 sp34;"),
            ("    model = obj->data.dobj;", "    model = isle_actor_source_self_model(obj);"),
            ("GetInterpolatedVelocity(&sp34, pokemon->path, 0.99999f)", "isle_actor_source_path_velocity(obj, pokemon->path, 0.99999f, &sp34)"),
            ("Pokemon_Turn(model, temp_f20, 0.13962634f)", "Pokemon_Turn(obj, model, temp_f20, 0.13962634f)"),
        ]
    if identity == "802CA300":
        changes += [
            ("obj->flags |= GOBJ_FLAG_2;", "isle_actor_source_object_flags(obj, UINT32_C(2), UINT32_C(2));"),
            ("getLevelProgress(", "isle_actor_source_level_progress(obj, ", 2),
        ]
    return tuple(changes)

DEFINITIONS += tuple((path, "func_beach_" + identity, first, last, entry_species_changes(identity))
                     for path, identity, first, last in ENTRY_SPECIES_UNITS)
DEFINITIONS += tuple(("src/beach/562370.c", "func_beach_" + identity, first, last,
                      entry_species_changes(identity))
                     for identity, first, last in (("802CA300", 10, 50), ("802CA418", 52, 54)))
DEFINITIONS += MOVEMENT_DEFINITIONS
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
#include "beach_initial_species_source.h"
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
#define LINK_POKEMON 3
#define cmdSendCommandToLink(link, command, sender) isle_actor_source_send_to_link(sender, link, command, sender)
#define cmdSendCommand(object, command, sender) isle_actor_source_send_command(object, command, sender)
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
static const PokemonInitData D_beach_802CCDA4 = { UINT32_C(0x802ccda4) };
static const PokemonInitData D_beach_802CCF78 = { UINT32_C(0x802ccf78) };
static const PokemonInitData D_beach_802CCE50 = { UINT32_C(0x802cce50) };
static const PokemonInitData D_beach_802CD118 = { UINT32_C(0x802cd118) };
static const PokemonInitData D_beach_802CE038 = { UINT32_C(0x802ce038) };
static const PokemonInitData D_beach_802CD948 = { UINT32_C(0x802cd948) };
#define POKEMON_FLAG_4 0x004
#define POKEMON_FLAG_200 0x200
#define POKEMON_FLAG_800 0x800
static const IsleGameVec3 D_8038838C_52879C = { 0.0f, 0.0f, 0.0f };
#define PokemonID_1003 1003
/* Pinned ultralib/include/macros.h numeric tokens and comparison semantics. */
#define PI 3.1415927f
#define TAU 6.2831855f
#define PI_2 1.5707964f
/* ISO C does not require math.h to expose the pinned M_PI double constant. */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define M_PI_F ((f32) M_PI)
#define SIGN(value) (((value) < 0) ? -1 : 1)
#define SQ(value) ((value) * (value))
#define ABS(value) (((value) < 0) ? -(value) : (value))
/* Native World AnimationPath kind values, not an InterpData layout. */
enum {
    ISLE_ACTOR_PATH_LINEAR = 0, ISLE_ACTOR_PATH_BEZIER = 1,
    ISLE_ACTOR_PATH_BEZIER_S3 = 2, ISLE_ACTOR_PATH_CATROM = 3
};
#define UNUSED
#define Pokemon_SpawnOnGround isle_actor_owner_spawn_ground
#define Pokemon_SpawnOnGroundDlLink4 isle_actor_owner_spawn_ground_dl4
#define Pokemon_Spawn isle_actor_owner_spawn
#define GET_POKEMON(object) isle_actor_source_require(object)
#define omCreateProcess isle_actor_source_create_process
void isle_actor_source_set_animation(IsleGameObject*, uint32_t, float, int);
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
            manifest["transformation_version"] != "actor-control-v10"):
        raise ValueError("unsupported actor authority/transformation")
    if git(repository, "cat-file", "-t", REVISION).strip() != b"commit":
        raise ValueError("actor authority is not a commit")
    if set(manifest["source_files"]) != {POKEMON, COMMON, MAGIKARP, PLAYER, *INITIAL_SPECIES_FILES, "src/app_level/app_level.h", "include/enums.h", "include/common_structs.h"}:
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
                          "src/source_command_scope.h", "src/source_factory_resources.h",
                          "src/beach_initial_species_source.h"})
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
