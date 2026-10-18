#!/usr/bin/env python3
"""Materialize complete audited own-GAME Item C in build-private output only."""
import argparse
import hashlib
import json
import re
from pathlib import Path
from prepare_engine import git, publish

REVISION = "df8815e75f3d79618466e01e0827c6165b93d11c"
ITEMS = "src/app_level/items.c"
VECTOR = "src/sys/vector.c"
POKEMON = "src/app_level/pokemon.c"
SPATIAL = "src/app_level/503C10.c"
OH = "src/sys/oh.c"
VERSION = "item-owner-v2-context-collision"
SUPPORT = (
    "src/item_owner.hpp", "src/item_owner.cpp", "src/item_owner_source.h",
    "src/item_source_bridge.c", "src/item_source.h", "src/item_source_bind.c",
    "src/borrowed_model_producer.hpp", "src/item_flute.hpp", "src/item_flute.cpp",
    "src/engine.c", "src/engine_internal.h", "src/source_factory_resources.h", "include/35mmisle/engine.h",
    "src/menu_animation.hpp", "src/menu_animation.cpp",
    "src/scene_services.hpp", "src/scene_services.cpp",
    "src/world_runtime.hpp", "src/world_runtime.cpp",
    "src/actor_owner.hpp", "src/actor_owner.cpp", "src/actor_owner_source.h",
    "src/actor_source_owner_bridge.c", "src/beach_magikarp_scene.hpp",
    "src/beach_magikarp_scene.cpp", "tools/prepare_engine.py",
    "src/actor_course_context.hpp", "src/rainbow_actor_source.h",
    "src/rainbow_actor_source.c", "src/rainbow_actor.hpp", "src/rainbow_actor.cpp",
    "src/rainbow_lifecycle_source.h", "src/player_root.hpp",
)
# Full definition inventory, not a selected physics cohort. All service changes
# below are recorded, ordered and byte-hashed in the authority manifest.
DEFINITIONS = (
    (ITEMS, "Items_DotProduct", 65, 67),
    (ITEMS, "Items_EndProcessByFunction", 69, 85),
    (ITEMS, "Items_Init", 87, 109),
    (ITEMS, "Items_InitIterator", 111, 114),
    (ITEMS, "Items_NextValidItem", 116, 124),
    (ITEMS, "Items_func_80359900", 126, 134),
    (ITEMS, "Items_CheckObjectExists", 136, 145),
    (ITEMS, "Items_LinkObject", 147, 168),
    (ITEMS, "Items_UnlinkObject", 170, 192),
    (ITEMS, "Items_GetFreeObjectID", 194, 214),
    (ITEMS, "Items_ShowDelayed", 225, 229),
    (ITEMS, "Items_InitItem", 231, 260),
    (ITEMS, "Items_ProcessCommand", 262, 266),
    (ITEMS, "Items_UpdateObject", 268, 270),
    (ITEMS, "Items_DeleteFume", 272, 274),
    (ITEMS, "Items_UpdateFume", 276, 291),
    (ITEMS, "Items_CreateSplash", 293, 322),
    (ITEMS, "Items_CreateWaterSplash", 324, 326),
    (ITEMS, "Items_CreateLavaSplash", 328, 330),
    (ITEMS, "Items_DeleteItemImmediately", 332, 335),
    (ITEMS, "Items_PlaySound", 337, 353),
    (ITEMS, "Items_CollideWithGround", 355, 508),
    (ITEMS, "Items_CollideWithCeiling", 510, 633),
    (ITEMS, "Items_NotifyItemPosition", 635, 652),
    (ITEMS, "Items_BounceFromCeiling", 654, 715),
    (ITEMS, "Items_BounceFromGround", 717, 769),
    (ITEMS, "Items_BounceFromCeilingAndGround", 771, 853),
    (ITEMS, "Items_UpdateItemMovement", 855, 1095),
    (ITEMS, "Items_RemovePesterBall", 1097, 1110),
    (ITEMS, "Items_UpdateNonMovingItem", 1112, 1132),
    (ITEMS, "Items_SpawnPesterBall", 1134, 1159),
    (ITEMS, "Items_SpawnApple", 1161, 1186),
    (ITEMS, "Items_PlayPokeFlute", 1188, 1201),
    (ITEMS, "Items_StopPokeFlute", 1203, 1211),
    (ITEMS, "Items_GetPokeFluteCmd", 1213, 1219),
    (ITEMS, "Items_GetPokeFluteState", 1221, 1227),
    (ITEMS, "Items_DoRemoveItem", 1229, 1233),
    (ITEMS, "Items_DeleteItem", 1235, 1258),
    (ITEMS, "Items_SetCustomFunctions", 1260, 1271),
    (ITEMS, "Items_func_8035CA1C", 1273, 1275),
    (ITEMS, "Items_Pause", 1277, 1285),
    (ITEMS, "Items_UnPause", 1287, 1295),
    (ITEMS, "Items_RemoveFlyingItems", 1297, 1305),
    (ITEMS, "Items_DisplaceAllItems", 1307, 1326),
    (VECTOR, "Vec3fNormalize", 5, 16),
    (VECTOR, "Vec3fAdd", 22, 28),
    (VECTOR, "Vec3fDirection", 46, 52),
    (VECTOR, "Vec3fScale", 54, 60),
    (VECTOR, "Vec3fAddScaled", 72, 78),
    (VECTOR, "Vec3fReflect", 250, 258),
    (POKEMON, "Pokemon_EatApple", 879, 885),
    (POKEMON, "Pokemon_KickItem", 902, 917),
    (SPATIAL, "func_80363848_503C58", 43, 52),
)
ORIGINS = (
    (ITEMS, "flags_and_logical_storage", 6, 63),
    (ITEMS, "callback_and_flute_globals", 216, 223),
    (POKEMON, "cache_entry", 23, 26),
    (POKEMON, "cache_extent", 69, 71),
    (POKEMON, "cache_init_allocate_free", 211, 250),
    (VECTOR, "dot_macro", 3, 3),
    (OH, "default_update", 100, 102),
    ("src/app_level/app_level.h", "item_ids_states", 25, 35),
    ("src/app_level/app_level.h", "command_and_record", 134, 148),
    ("src/world/world.h", "surface_types", 172, 193),
    ("include/enums.h", "native_links", 249, 254),
)
GLOBALS = {
    "Items_ListEntryArray": "entries", "Items_AllocatedObjectListHead": "allocated_head",
    "Items_AllocatedObjectListTail": "allocated_tail", "Items_FreeObjectListHead": "free_head",
    "D_803AF0B4_54F4C4": "iterator", "D_803AF0B8_54F4C8": "iterator_unused",
    "Items_TotalItemCount": "total", "Items_PesterBallCount": "balls", "Items_AppleCount": "apples",
    "Items_FnUpdateKind": "update_kind", "Items_FnUpdate": "update",
    "Items_FnCollide": "collide", "D_80382EBC_5232CC": "extra", "Items_ObjectCounter": "object_counter",
}
EXPLICIT_GLOBALS = {
    "Items_Init", "Items_InitIterator", "Items_NextValidItem", "Items_func_80359900",
    "Items_CheckObjectExists", "Items_GetFreeObjectID", "Items_SetCustomFunctions",
    "Items_func_8035CA1C", "Items_Pause", "Items_UnPause", "Items_RemoveFlyingItems",
    "Items_DisplaceAllItems", "Items_PlaySound", "Pokemon_EatApple", "Pokemon_KickItem",
    "func_80363848_503C58",
}
FLUTE = {"Items_PlayPokeFlute": (0, "void"), "Items_StopPokeFlute": (1, "void"),
         "Items_GetPokeFluteCmd": (2, "s32"), "Items_GetPokeFluteState": (3, "s32")}
PREAMBLE = '''/* Private transformed complete own-GAME Item bodies. No SDK inputs. */
#include "engine_internal.h"
#include "item_owner_source.h"
#define SQ(x) ((x) * (x))
#define VEC_DOT(v, w) (((v)->x * (w)->x) + ((v)->y * (w)->y) + ((v)->z * (w)->z))
#define ABS(x) ((x) < 0 ? -(x) : (x))
#define BASE_ITEM_OBJID 300
#define GET_ITEM(obj) isle_item_source_nullable(g, obj)
#define GET_POKEMON(obj) isle_item_source_actor(g, obj)
typedef IsleItemListEntry ItemObjectListEntry;
typedef IsleItemSource Item;
typedef IsleActorSource Pokemon;
typedef IsleActorGround GroundResult;
static float item_normalize(IsleGameVec3*);
static IsleGameVec3* item_add(IsleGameVec3*, IsleGameVec3*);
static float item_direction(IsleGameVec3*, IsleGameVec3*, IsleGameVec3*);
static IsleGameVec3* item_scale(IsleGameVec3*, float);
static IsleGameVec3* item_add_scaled(IsleGameVec3*, IsleGameVec3*, float);
static IsleGameVec3* item_reflect(IsleGameVec3*, IsleGameVec3*);
static float Items_DotProduct(IsleGameVec3*, IsleGameVec3*);
void Items_ProcessCommand(IsleGameObject*, int32_t);
'''
VECTOR_NAMES = {"Vec3fNormalize": "item_normalize", "Vec3fAdd": "item_add",
                "Vec3fDirection": "item_direction", "Vec3fScale": "item_scale",
                "Vec3fAddScaled": "item_add_scaled", "Vec3fReflect": "item_reflect"}

def transform(symbol, original):
    text = original
    changes = []
    def change(old, new, expected_count=None):
        nonlocal text
        count = text.count(old)
        if not count:
            raise ValueError("missing reviewed transformation in " + symbol + ": " + old)
        if expected_count is not None and count != expected_count:
            raise ValueError("reviewed transformation count mismatch in " + symbol + ": " + old)
        # Every occurrence is independently located in the pinned selected body.
        changes.append({"find_sha256": hashlib.sha256(old.encode()).hexdigest(),
                        "replace_sha256": hashlib.sha256(new.encode()).hexdigest(), "count": count})
        text = text.replace(old, new)
    if symbol in FLUTE:
        action, result = FLUTE[symbol]
        # Shared Flute is the ONLY executable implementation of these four
        # simple source bodies. Their complete originals remain hash-authority.
        new = result + " " + symbol + "(IsleItemGlobals* g) {\n    int32_t value = 0;\n    isle_item_source_checked(g, isle_item_owner_flute(g, " + str(action) + ", &value));\n"
        new += "    return value;\n" if result != "void" else ""
        change(text, new + "}\n")
        return text, changes
    if symbol == "Items_ProcessCommand":
        change("void Items_ProcessCommand(GObjCmdData cmdData)", "void Items_ProcessCommand(IsleGameObject* receiver, int32_t command)")
        change("omCurrentObject", "receiver")
        change("cmdData.cmd", "command")
    elif symbol in ("Items_NextValidItem", "Items_func_80359900"):
        # The original dereference assumes a valid canonical payload. Native
        # admission failures must return status at the public query boundary,
        # not dereference NULL or fault through an owning C++ iterator caller.
        change("        if (obj != NULL && GET_ITEM(obj)->state ",
               "        Item* item = NULL;\n        if (obj != NULL) {\n            IsleGameResult result = isle_item_source_get(g->game, obj, &item);\n            if (result != ISLE_GAME_OK) { g->status = result; return NULL; }\n        }\n        if (obj != NULL && item->state ")
    elif symbol == "Items_UpdateObject":
        change("    cmdProcessCommands(Items_ProcessCommand);", "    isle_item_source_checked(g, isle_item_owner_commands(g, obj));")
    elif symbol == "Items_InitItem":
        change("void Items_InitItem(GObj* obj, Vec3f* pos, Vec3f* extraVelocity)", "IsleGameResult Items_InitItem(IsleItemGlobals* g, GObj* obj, Vec3f* pos, Vec3f* extraVelocity)")
        change("    Vec3f velocity;", "    Vec3f velocity;\n    Vec3f target;\n    IsleGameResult result = isle_item_owner_target(g, &target);\n    if (result != ISLE_GAME_OK) return result;")
        change("    model = obj->data.dobj;", "    result = isle_item_owner_root(g, obj, &model);\n    if (result != ISLE_GAME_OK) return result;")
        for axis in "XYZ":
            change("gCamTarget" + axis, "target." + axis.lower())
        change("    item = obj->userData;", "    item = GET_ITEM(obj);\n    if (item == NULL) return g->status == ISLE_GAME_OK ? ISLE_GAME_INVALID_STATE : g->status;\n    item->prevPos = *pos;\n    item->collisionVelocity = (IsleGameVec3){0};")
        change("    omCreateProcess(obj, Items_ShowDelayed, 0, 7);", "    omCreateProcess(obj, Items_ShowDelayed, 0, 7);\n    return g->status;")
    elif symbol == "Items_CreateSplash":
        change("UnkEC64Arg3* treeDef, Texture*** textures, AnimCmd** modelAnim, AnimCmd*** texturesAnim", "int family")
        change("    GObj* fumeObj;", "    GObj* fumeObj;\n    IsleGameModelTransform* fumeModel;")
        change("    animSetTextureAnimationSpeed(pesterBallObj, 0.0f);",
               "    isle_item_source_checked(g, isle_item_owner_prepare(g, family, NULL));\n    if (g->status != ISLE_GAME_OK) return;\n    animSetTextureAnimationSpeed(pesterBallObj, 0.0f);")
        first = text.index("    fumeObj = omAddGObj(")
        last = text.index("    getGroundAt(", first)
        change(text[first:last], "    isle_item_source_checked(g, isle_item_owner_create(g, Items_GetFreeObjectID(g), family, &fumeObj));\n    fumeModel = isle_item_source_root(g, fumeObj);\n")
        change("    fumeObj->userData = NULL;\n", "")
        change("    omCreateProcess(fumeObj, Items_UpdateFume, 0, 7);",
               "    omCreateProcess(fumeObj, Items_UpdateFume, 0, 7);\n    isle_item_source_checked(g, isle_item_owner_finish(g));\n    if (g->status != ISLE_GAME_OK) return;")
    elif symbol in ("Items_CreateWaterSplash", "Items_CreateLavaSplash"):
        args = ("D_800EB430, D_800EB510, D_800EAFB0, D_800EB0C0" if symbol.endswith("WaterSplash")
                else "D_800EDAB0, D_800EDB90, D_800ED5B0, D_800ED6B0")
        change(args, "2" if symbol.endswith("WaterSplash") else "3")
    elif symbol == "Items_PlaySound":
        first = text.index("    playerPos.x =")
        last = text.index("    dist =", first)
        change(text[first:last], "    isle_item_source_checked(g, isle_item_owner_player_position(g, &playerPos));\n")
        change("auPlaySoundWithVolume(soundID,", "isle_item_source_sound(g, soundID,")
    elif symbol in ("Items_SpawnPesterBall", "Items_SpawnApple"):
        ball = symbol.endswith("PesterBall")
        obj, payload, family = ("ballObj", "ball", "0") if ball else ("appleObj", "apple", "1")
        change("void " + symbol + "(Vec3f* pos, Vec3f* extraVelocity)", "IsleGameResult " + symbol + "(IsleItemGlobals* g, Vec3f* pos, Vec3f* extraVelocity, GObj** output)")
        change("    Item* " + payload + ";", "    Item* " + payload + ";\n    IsleGameResult result;\n    *output = NULL;")
        count = "Items_PesterBallCount" if ball else "Items_AppleCount"
        predicate = count + " >= 5 && Items_AllocatedObjectListHead != NULL"
        change("    if (" + predicate + ") {\n        Items_DeleteItem(Items_AllocatedObjectListHead->obj);\n    }",
               "    GObj* eviction = (" + predicate + ") ? Items_AllocatedObjectListHead->obj : NULL;\n"
               "    result = isle_item_owner_prepare(g, " + family + ", eviction);\n"
               "    if (result != ISLE_GAME_OK) return result;\n"
               "    if (eviction != NULL) {\n        Items_DeleteItem(eviction);\n"
               "        if (g->status != ISLE_GAME_OK) return g->status;\n    }")
        first = text.index("    " + obj + " = omAddGObj(")
        last = text.index("    " + payload + "->itemID", first)
        change(text[first:last], "    result = isle_item_owner_create(g, Items_GetFreeObjectID(g), " + family + ", &" + obj + ");\n    if (result != ISLE_GAME_OK) return result;\n    " + payload + " = GET_ITEM(" + obj + ");\n    if (" + payload + " == NULL) return g->status == ISLE_GAME_OK ? ISLE_GAME_INVALID_STATE : g->status;\n")
        change("    Items_InitItem(" + obj + ", pos, extraVelocity);", "    result = Items_InitItem(g, " + obj + ", pos, extraVelocity);\n    if (result != ISLE_GAME_OK) return result;\n    result = isle_item_owner_finish(g);\n    if (result != ISLE_GAME_OK) return result;\n    *output = " + obj + ";\n    return g->status;")
    elif symbol == "Items_UpdateItemMovement":
        # Original gravity local is named g; keep its exact float operations
        # while avoiding collision with the explicit native Globals parameter.
        change("    f32 g = 1.2f;", "    f32 gravity = 1.2f;")
        change("g * 0.5f", "gravity * 0.5f")
    elif symbol == "Items_func_8035CA1C":
        change("void Items_func_8035CA1C(s32 (*arg0)(Vec3f*, Vec3f*, Vec3f*, Vec3f*))",
               "IsleGameResult Items_func_8035CA1C(IsleGame* binding_game, IsleItemCollisionProvider provider, void* context)", expected_count=1)
        change("    D_80382EBC_5232CC = arg0;",
               "    if (g == NULL || binding_game == NULL || (provider == NULL) != (context == NULL))\n"
               "        return ISLE_GAME_INVALID_ARGUMENT;\n"
               "    if (binding_game != g->game || g->extra_active != 0)\n"
               "        return ISLE_GAME_INVALID_STATE;\n"
               "    D_80382EBC_5232CC = provider;\n"
               "    g->extra_context = context;\n"
               "    return ISLE_GAME_OK;", expected_count=1)
    elif symbol == "Items_DeleteItem":
        change("    Items_UnlinkObject(obj);",
               "    Items_UnlinkObject(obj);\n    if (g->status != ISLE_GAME_OK) return;")
        change("    item = GET_ITEM(obj);",
               "    item = GET_ITEM(obj);\n    if (item == NULL) {\n        isle_item_source_checked(g, ISLE_GAME_INVALID_STATE);\n        return;\n    }")
    elif symbol == "Items_DoRemoveItem":
        change("    Pokemon_SendDieCommand(obj);", "    isle_item_source_checked(g, isle_item_owner_die(g, obj));")
        change("    Pokemon_FreeItem(obj->userData);", "    isle_item_source_checked(g, isle_item_owner_free_payload(g, obj));")
    elif symbol == "Pokemon_KickItem":
        change("    DObj* model = obj->data.dobj;\n", "")
        change("GET_TRANSFORM(model)->rot.v.y", "isle_item_source_actor_yaw(g, obj)")
    elif symbol == "func_80363848_503C58":
        change("func_80363848_503C58", "isle_item_source_sphere")
    if symbol in EXPLICIT_GLOBALS:
        opener = text[:text.index(" {")]
        args_at = opener.index("(") + 1
        args = opener[args_at:-1]
        change(opener, opener[:args_at] + "IsleItemGlobals* g" + (", " + args if args != "void" else "") + ")")
    elif symbol not in {"Items_DotProduct", "Items_EndProcessByFunction", "Items_ProcessCommand", "Items_InitItem", "Items_SpawnPesterBall", "Items_SpawnApple", "Items_CreateWaterSplash", "Items_CreateLavaSplash", "Items_DeleteItemImmediately"} and symbol not in VECTOR_NAMES:
        arg = "arg0" if symbol == "Items_RemovePesterBall" else "pesterBallObj" if symbol == "Items_CreateSplash" else "obj"
        opener = text.splitlines(keepends=True)[0]
        change(opener, opener + "    IsleItemGlobals* g = isle_item_source_globals(" + arg + ");\n")
    if "    static s32 Items_ObjectCounter = 0;\n" in text:
        change("    static s32 Items_ObjectCounter = 0;\n", "")
    for old, field in GLOBALS.items():
        if re.search(r"\b" + old + r"\b", text):
            rewritten = re.sub(r"\b" + old + r"\b", "(g->" + field + ")", text)
            change(text, rewritten)
    replacements = {
        "DObj*": "IsleGameModelTransform*",
        "model->position.v": "model->position", "model->scale.v": "model->scale",
        "obj->data.dobj": "isle_item_source_root(g, obj)",
        "pesterBallObj->data.dobj": "isle_item_source_root(g, pesterBallObj)",
        "ptr->data.dobj": "isle_item_source_root(g, ptr)",
        "ballModel->position.v": "ballModel->position",
        "fumeObj->data.dobj->position.v": "fumeModel->position",
        "fumeObj->data.dobj->scale.v": "fumeModel->scale",
        "isle_item_source_root(g, obj)->position.v": "isle_item_source_root(g, obj)->position",
        "animSetTextureAnimationSpeed(": "isle_item_source_texture(g, ",
        "getGroundAt(": "isle_item_source_ground(g, ", "getCeilingAt(": "isle_item_source_ceiling(g, ",
        "StaticObject_Collide(": "isle_item_source_collide(g, ",
        "func_80363848_503C58(": "isle_item_source_sphere(g, ",
        "cmdSendCommandToLink(LINK_POKEMON, ": "isle_item_source_broadcast(g, ",
        "cmdSendCommand(": "isle_item_source_send(g, ",
        "omCreateProcess(": "isle_item_source_create_process(g, ",
        "omDeleteGObj(": "isle_item_source_delete_native(g, ",
        "ohPauseObjectProcesses(ptr)": "isle_item_source_pause(g, ptr, 1)",
        "ohResumeObjectProcesses(ptr)": "isle_item_source_pause(g, ptr, 0)",
        "obj->animationTime": "isle_item_source_animation_time(g, obj)",
        "Items_PlaySound(model, ": "Items_PlaySound(g, model, ",
        "Items_CheckObjectExists(pokemon->apple)": "Items_CheckObjectExists(g, pokemon->apple)",
        "auPlaySound(SOUND_ID_8)": "isle_item_source_sound(g, 8, 0x7fff)",
        "auPlaySound(SOUND_ID_9)": "isle_item_source_sound(g, 9, 0x7fff)",
    }
    for old, new in replacements.items():
        if old in text:
            change(old, new)
    rewritten = re.sub(r"\b(?:" + "|".join(map(re.escape, VECTOR_NAMES)) + r")\b",
                       lambda match: VECTOR_NAMES[match.group()], text)
    if rewritten != text:
        change(text, rewritten)
    new = re.sub(r"\bVec3f\b", "IsleGameVec3", text)
    if new != text: change(text, new)
    # These are source enum names with explicit numeric suffixes, not inferred
    # binary layout values. Header blobs are independently pinned in manifest.
    for pattern, replace in ((r"\bSURFACE_TYPE_([0-9A-F]+)\b", lambda m: "0x" + m[1]),
                             (r"\b(?:SOUND_ID_|POKEMON_CMD_|PokemonID_)([0-9]+)\b", lambda m: m[1])):
        new = re.sub(pattern, replace, text)
        if new != text: change(text, new)
    for old, new in {"ITEM_ID_PESTER_BALL": "ISLE_ITEM_PESTER_BALL", "ITEM_ID_APPLE": "ISLE_ITEM_APPLE",
                     "ITEM_STATE_INVALID": "ISLE_ITEM_INVALID", "ITEM_STATE_FLYING": "ISLE_ITEM_FLYING",
                     "ITEM_STATE_STILL": "ISLE_ITEM_STILL", "ITEM_FLAG_BOUNCED": "ISLE_ITEM_BOUNCED",
                     "ITEM_FLAG_TOUCHED_GROUND": "ISLE_ITEM_TOUCHED_GROUND", "ITEM_FLAG_DELETED": "ISLE_ITEM_DELETED",
                     "ITEM_CMD_REMOVE": "100", "POKEMON_FLAG_2": "2"}.items():
        if old in text: change(old, new)
    # Publish each consecutive scalar-write group before the next source action.
    # No observer, wait, end, callback or query sees staging-only source writes.
    # Arithmetic/assignment order remains verbatim; no clock/track is reset.
    lines = text.splitlines(keepends=True)
    def mutation_target(line):
        if re.search(r"(?:model|ballModel|fumeModel)->(?:position|scale)\.[xyz]\s*[+*\-]?=", line):
            if "ballModel->" in line: return "pesterBallObj"
            if "fumeModel->" in line: return "fumeObj"
            return "ptr" if symbol == "Items_DisplaceAllItems" else "obj"
        if "item_add_scaled(&model->position," in line: return "obj"
        return None
    for index, line in enumerate(lines):
        target = mutation_target(line)
        if target and (index + 1 == len(lines) or mutation_target(lines[index + 1]) != target):
            indent = line[:len(line)-len(line.lstrip())]
            lines[index] += indent + "isle_item_source_publish(g, " + target + ");\n"
    new = "".join(lines)
    if new != text: change(text, new)
    # Native link traversal remains the actual shared OM list and order, checked
    # for canonical Actor records rather than a private owner slot scan.
    if "omGObjListHead[LINK_POKEMON]" in text:
        change("omGObjListHead[LINK_POKEMON]", "isle_item_source_link_next(g, NULL, 3)")
        change("pokemonObjPtr = pokemonObjPtr->next", "pokemonObjPtr = isle_item_source_link_next(g, pokemonObjPtr, 3)")
    if "omGObjListHead[LINK_ITEM]" in text:
        change("omGObjListHead[LINK_ITEM]", "isle_item_source_link_next(g, NULL, 4)")
        change("ptr = ptr->next", "ptr = isle_item_source_link_next(g, ptr, 4)")
    if symbol == "Items_UpdateItemMovement":
        # Keep the original callback point and hit branch. A nonwaiting failure
        # must return before either hit effects or the no-collision path.
        change("    if ((g->extra) != NULL && (g->extra)(&model->position, &vel2, &outPos, &outVel) >= 0) {",
               "    if ((g->extra) != NULL) {\n"
               "        somethingID = isle_item_source_extra(g, obj, &model->position, &vel2, &outPos, &outVel);\n"
               "        if (g->status != ISLE_GAME_OK) return;\n"
               "    } else {\n"
               "        somethingID = -1;\n"
               "    }\n"
               "    if (somethingID >= 0) {", expected_count=1)
    # Typed FX is created/configured by one real shared effect leaf. Preserve
    # exhaustion as a normal NULL result and preserve its position-before-send.
    text_before_fx = text
    text = re.sub(r"(?m)^([ \t]*)Effect\* fx;\n", "", text)
    text = re.sub(r"(?m)^([ \t]*)(?:Effect\* )?fx = fx_createEffect\(3, 0\);\n\1if \(fx != NULL\) \{\n(?:\1    fx->pos\.[xyz] = [^\n]+;\n){3}\1\}", r"\1isle_item_source_effect(g, obj);", text)
    if text != text_before_fx:
        changes.append({"find_sha256": hashlib.sha256(text_before_fx.encode()).hexdigest(),
                        "replace_sha256": hashlib.sha256(text.encode()).hexdigest(), "count": 1})
    for object_name in ("obj", "ballObj", "appleObj"):
        old = object_name + "->flags |= GOBJ_FLAG_HIDDEN;"
        if old in text: change(old, "isle_item_source_hidden(g, " + object_name + ", 1);")
    if "obj->flags &= ~GOBJ_FLAG_HIDDEN;" in text:
        change("obj->flags &= ~GOBJ_FLAG_HIDDEN;", "isle_item_source_hidden(g, obj, 0);")
    # Public factories run inside a returning C++ status boundary. Unlike a
    # running C continuation, that boundary accumulates rather than faults, so
    # stop at the first failed leaf before the next irreversible Source effect.
    returns = {
        "Items_InitItem": "return g->status;",
        "Items_SpawnPesterBall": "return g->status;",
        "Items_SpawnApple": "return g->status;",
        "Items_CreateSplash": "return;",
        "Items_UnlinkObject": "return;",
        "Items_DeleteItem": "return;",
    }
    if symbol in returns:
        before_guards = text
        lines = text.splitlines(keepends=True)
        for index, line in enumerate(lines):
            if ("isle_item_source_" in line and "isle_item_source_globals(" not in line and
                line.rstrip().endswith(";") and not re.search(r"\breturn\b", line) and
                (index+1==len(lines) or not lines[index+1].lstrip().startswith(
                    ("if (g->status != ISLE_GAME_OK)", "if (result != ISLE_GAME_OK)")))):
                indent = line[:len(line)-len(line.lstrip())]
                lines[index] += indent + "if (g->status != ISLE_GAME_OK) " + returns[symbol] + "\n"
        text = "".join(lines)
        if text != before_guards:
            changes.append({"find_sha256": hashlib.sha256(before_guards.encode()).hexdigest(),
                            "replace_sha256": hashlib.sha256(text.encode()).hexdigest(), "count": 1})
    if symbol in VECTOR_NAMES or symbol == "Items_DotProduct":
        change(text.splitlines()[0], "static " + text.splitlines()[0])
    return text, changes

def prepare(repository, manifest_path, output_path, evidence_path):
    raw = manifest_path.read_bytes()
    manifest = json.loads(raw)
    if manifest["schema_version"] != 1 or manifest["revision"] != REVISION or manifest["transformation_version"] != VERSION:
        raise ValueError("unsupported Item authority/transformation")
    allowed = {ITEMS, VECTOR, POKEMON, SPATIAL, OH, "include/enums.h", "src/app_level/app_level.h", "src/world/world.h"}
    if set(manifest["source_files"]) != allowed:
        raise ValueError("Item source allowlist mismatch")
    if git(repository, "cat-file", "-t", REVISION).strip() != b"commit":
        raise ValueError("Item authority is not the exact commit")
    root = Path(__file__).resolve().parent.parent
    if set(manifest["native_support"]) != set(SUPPORT):
        raise ValueError("Item native support allowlist mismatch")
    for path, identity in manifest["native_support"].items():
        if hashlib.sha256((root / path).read_bytes()).hexdigest() != identity["sha256"]:
            raise ValueError("Item native support identity mismatch: " + path)
    sources = {}
    for path, identity in manifest["source_files"].items():
        data = git(repository, "show", REVISION + ":" + path)
        blob = git(repository, "rev-parse", REVISION + ":" + path).decode("ascii").strip()
        if (identity["git_blob"] != blob or identity["sha256"] != hashlib.sha256(data).hexdigest() or
                hashlib.sha1(b"blob " + str(len(data)).encode() + b"\0" + data).hexdigest() != blob):
            raise ValueError("Item source identity mismatch: " + path)
        sources[path] = data.decode().splitlines(keepends=True)
    if len(manifest["semantic_origins"]) != len(ORIGINS):
        raise ValueError("Item semantic origin allowlist mismatch")
    for origin, (path, label, first, last) in zip(manifest["semantic_origins"], ORIGINS):
        if (origin["path"], origin["label"], origin["first"], origin["last"]) != (path, label, first, last):
            raise ValueError("Item semantic origin allowlist mismatch")
        selected = "".join(sources[path][first-1:last])
        if hashlib.sha256(selected.encode()).hexdigest() != origin["source_sha256"]:
            raise ValueError("Item semantic origin identity mismatch")
    if len(manifest["definitions"]) != len(DEFINITIONS):
        raise ValueError("Item complete definition allowlist mismatch")
    if manifest["generator_sha256"] != hashlib.sha256(Path(__file__).read_bytes()).hexdigest():
        raise ValueError("unadmitted Item transformation implementation")
    admitted = []
    output = [PREAMBLE]
    for unit, (path, symbol, first, last) in zip(manifest["definitions"], DEFINITIONS):
        if (unit["path"], unit["symbol"], unit["first"], unit["last"]) != (path, symbol, first, last):
            raise ValueError("Item definition allowlist mismatch")
        selected = "".join(sources[path][first-1:last])
        native, changes = transform(symbol, selected)
        if unit["source_sha256"] != hashlib.sha256(selected.encode()).hexdigest():
            raise ValueError("unadmitted Item source body: " + symbol)
        admitted.append(dict(unit, native_sha256=hashlib.sha256(native.encode()).hexdigest(), changes=changes))
        output.extend(('#line %d "snap@%s/%s"\n' % (first, REVISION, path), native, "\n"))
    result = "".join(output)
    if hashlib.sha256(result.encode()).hexdigest() != manifest["transformed_source_sha256"]:
        raise ValueError("unadmitted Item transformed output")
    evidence = dict(manifest, manifest_sha256=hashlib.sha256(raw).hexdigest(),
                    transformed_definitions=admitted,
                    source_sha256=hashlib.sha256(result.encode()).hexdigest())
    if output_path.resolve() == evidence_path.resolve():
        raise ValueError("Item source and evidence paths must differ")
    publish(output_path, result)
    publish(evidence_path, json.dumps(evidence, indent=2) + "\n")

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repository", type=Path, required=True)
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--evidence", type=Path, required=True)
    args = parser.parse_args()
    prepare(args.repository, args.manifest, args.output, args.evidence)

if __name__ == "__main__":
    main()
