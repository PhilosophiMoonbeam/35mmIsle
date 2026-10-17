#ifndef ISLE_ITEM_OWNER_SOURCE_H
#define ISLE_ITEM_OWNER_SOURCE_H
#include "item_source.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Native semantic state, never an N64 allocation/layout. The caller retains
 * this across wrapper reconstruction. Init deliberately resets only the source
 * logical structures and total count, not cursor, family counts or hooks. */
typedef struct IsleItemListEntry {
    struct IsleItemListEntry *prev, *next;
    IsleGameObject* obj;
    int32_t index;
} IsleItemListEntry;
typedef struct IsleItemGlobals {
    IsleItemListEntry entries[20];
    IsleItemListEntry *allocated_head, *allocated_tail, *free_head;
    int32_t iterator, iterator_unused, total, balls, apples, object_counter;
    uint8_t update_kind;
    IsleGameSourceCallback update;
    IsleItemCollisionCallback collide;
    IsleItemCollisionProvider extra;
    size_t payload_cache;
    void* owner;
    IsleGame* game;
    IsleGameResult status;
    unsigned nonwaiting;
} IsleItemGlobals;
/* All owner leaves return without waiting, ending a process or faulting. */
IsleGameResult isle_item_owner_root(IsleItemGlobals*, IsleGameObject*, IsleGameModelTransform**);
IsleGameResult isle_item_owner_publish(IsleItemGlobals*, IsleGameObject*);
/* Single-flight real unpublished admission. C chooses the Original oldest
 * object and ID; these nonwaiting leaves never run Source or own a C wait. */
IsleGameResult isle_item_owner_prepare(IsleItemGlobals*, int family, IsleGameObject* eviction);
IsleGameResult isle_item_owner_process(IsleItemGlobals*, IsleGameObject*, IsleGameSourceCallback,
    uint8_t kind, uint32_t priority, IsleGameProcess**);
IsleGameResult isle_item_owner_finish(IsleItemGlobals*);
IsleGameResult isle_item_owner_create(IsleItemGlobals*, uint32_t, int family, IsleGameObject**);
IsleGameResult isle_item_owner_texture(IsleItemGlobals*, IsleGameObject*, float);
IsleGameResult isle_item_owner_animation_time(IsleItemGlobals*, IsleGameObject*, float*);
IsleGameResult isle_item_owner_step(IsleItemGlobals*, IsleGameObject*);
IsleGameResult isle_item_owner_commands(IsleItemGlobals*, IsleGameObject*);
IsleGameResult isle_item_owner_send(IsleItemGlobals*, IsleGameObject*, int32_t, IsleGameObject*);
IsleGameResult isle_item_owner_broadcast(IsleItemGlobals*, int32_t, IsleGameObject*);
IsleGameResult isle_item_owner_ground(IsleItemGlobals*, float, float, IsleActorGround*);
IsleGameResult isle_item_owner_ceiling(IsleItemGlobals*, float, float, IsleActorGround*, int*);
IsleGameResult isle_item_owner_collide(IsleItemGlobals*, IsleGameVec3*, IsleGameVec3*, IsleGameVec3*, IsleGameVec3*, int32_t*);
IsleGameResult isle_item_owner_effect(IsleItemGlobals*, IsleGameObject*);
IsleGameResult isle_item_owner_player_position(IsleItemGlobals*, IsleGameVec3*);
IsleGameResult isle_item_owner_target(IsleItemGlobals*, IsleGameVec3*);
IsleGameResult isle_item_owner_sound(IsleItemGlobals*, int32_t, int32_t);
IsleGameResult isle_item_owner_die(IsleItemGlobals*, IsleGameObject*);
IsleGameResult isle_item_owner_free_payload(IsleItemGlobals*, IsleGameObject*);
IsleGameResult isle_item_owner_beach(IsleItemGlobals*, IsleGameObject*, IsleActorGround*);
IsleItemGlobals* isle_item_owner_globals(IsleGameObject*);
IsleGameResult isle_item_owner_actor_yaw(IsleItemGlobals*, IsleGameObject*, float*);
IsleGameResult isle_item_owner_flute(IsleItemGlobals*, int action, int32_t*);
/* C adapters alone may fault, after the nonwaiting C++ leaf has returned.
 * During public nonwaiting entrypoints status accumulates instead of faulting. */
IsleItemGlobals* isle_item_source_globals(IsleGameObject*);
IsleGameModelTransform* isle_item_source_root(IsleItemGlobals*, IsleGameObject*);
void isle_item_source_publish(IsleItemGlobals*, IsleGameObject*);
void isle_item_source_checked(IsleItemGlobals*, IsleGameResult);
void isle_item_source_texture(IsleItemGlobals*, IsleGameObject*, float);
float isle_item_source_animation_time(IsleItemGlobals*, IsleGameObject*);
void isle_item_source_animation_process(IsleGameObject*);
void isle_item_source_update(IsleGameObject*, void*);
void isle_item_source_default_update(IsleGameObject*, void*);
void isle_item_source_command(IsleGameObject*, int32_t, IsleGameObject*, void*);
void isle_item_source_beach(IsleGameObject*, IsleActorGround*);
void isle_item_source_ground(IsleItemGlobals*, float, float, IsleActorGround*);
int isle_item_source_ceiling(IsleItemGlobals*, float, float, IsleActorGround*);
int32_t isle_item_source_collide(IsleItemGlobals*, IsleGameVec3*, IsleGameVec3*, IsleGameVec3*, IsleGameVec3*);
int32_t isle_item_source_extra(IsleItemGlobals*, IsleGameObject*, IsleGameVec3*, IsleGameVec3*, IsleGameVec3*, IsleGameVec3*);
void isle_item_source_effect(IsleItemGlobals*, IsleGameObject*);
void isle_item_source_send(IsleItemGlobals*, IsleGameObject*, int32_t, IsleGameObject*);
void isle_item_source_broadcast(IsleItemGlobals*, int32_t, IsleGameObject*);
void isle_item_source_sound(IsleItemGlobals*, int32_t, int32_t);
void isle_item_source_delete_native(IsleItemGlobals*, IsleGameObject*);
void isle_item_source_pause(IsleItemGlobals*, IsleGameObject*, int);
IsleGameObject* isle_item_source_link_next(IsleItemGlobals*, IsleGameObject*, uint8_t);
IsleActorSource* isle_item_source_actor(IsleItemGlobals*, IsleGameObject*);
float isle_item_source_actor_yaw(IsleItemGlobals*, IsleGameObject*);
void Pokemon_EatApple(IsleItemGlobals*, IsleGameObject*);
void Pokemon_KickItem(IsleItemGlobals*, IsleGameObject*, float);
float isle_item_source_sphere(IsleItemGlobals*, IsleGameObject*, IsleGameVec3*);
void isle_item_source_create_process(IsleItemGlobals*, IsleGameObject*, IsleGameSourceCallback, uint8_t, uint32_t);
void isle_item_source_hidden(IsleItemGlobals*, IsleGameObject*, int);
IsleItemSource* isle_item_source_nullable(IsleItemGlobals*, IsleGameObject*);
/* Complete private own-GAME Item algorithms materialized by prepare_items.py. */
void Items_Init(IsleItemGlobals*);
void Items_InitIterator(IsleItemGlobals*);
IsleGameObject* Items_NextValidItem(IsleItemGlobals*);
IsleGameObject* Items_func_80359900(IsleItemGlobals*);
IsleGameObject* Items_CheckObjectExists(IsleItemGlobals*, IsleGameObject*);
void Items_LinkObject(IsleGameObject*);
void Items_UnlinkObject(IsleGameObject*);
int32_t Items_GetFreeObjectID(IsleItemGlobals*);
void Items_EndProcessByFunction(IsleGameObject*, IsleGameSourceCallback);
void Items_ShowDelayed(IsleGameObject*);
IsleGameResult Items_InitItem(IsleItemGlobals*, IsleGameObject*, IsleGameVec3*, IsleGameVec3*);
void Items_UpdateObject(IsleGameObject*);
void Items_ProcessCommand(IsleGameObject*, int32_t);
void Items_DeleteFume(IsleGameObject*);
void Items_UpdateFume(IsleGameObject*);
void Items_CreateSplash(IsleGameObject*, int);
void Items_CreateWaterSplash(IsleGameObject*);
void Items_CreateLavaSplash(IsleGameObject*);
void Items_DeleteItemImmediately(IsleGameObject*);
void Items_PlaySound(IsleItemGlobals*, IsleGameModelTransform*, int32_t);
void Items_CollideWithGround(IsleGameObject*, IsleActorGround*);
void Items_CollideWithCeiling(IsleGameObject*, IsleActorGround*);
void Items_NotifyItemPosition(IsleGameObject*);
void Items_BounceFromCeiling(IsleGameObject*, IsleActorGround*);
void Items_BounceFromGround(IsleGameObject*, IsleActorGround*);
void Items_BounceFromCeilingAndGround(IsleGameObject*, IsleActorGround*);
void Items_UpdateItemMovement(IsleGameObject*);
void Items_RemovePesterBall(IsleGameObject*);
void Items_UpdateNonMovingItem(IsleGameObject*);
IsleGameResult Items_SpawnPesterBall(IsleItemGlobals*, IsleGameVec3*, IsleGameVec3*, IsleGameObject**);
IsleGameResult Items_SpawnApple(IsleItemGlobals*, IsleGameVec3*, IsleGameVec3*, IsleGameObject**);
void Items_DoRemoveItem(IsleGameObject*);
void Items_DeleteItem(IsleGameObject*);
void Items_SetCustomFunctions(IsleItemGlobals*, IsleGameSourceCallback, uint8_t, IsleItemCollisionCallback);
void Items_func_8035CA1C(IsleItemGlobals*, IsleItemCollisionProvider);
void Items_Pause(IsleItemGlobals*);
void Items_UnPause(IsleItemGlobals*);
void Items_RemoveFlyingItems(IsleItemGlobals*);
void Items_DisplaceAllItems(IsleItemGlobals*, float, float, float);
void Items_PlayPokeFlute(IsleItemGlobals*);
void Items_StopPokeFlute(IsleItemGlobals*);
int32_t Items_GetPokeFluteCmd(IsleItemGlobals*);
int32_t Items_GetPokeFluteState(IsleItemGlobals*);
#ifdef __cplusplus
}
#endif
#endif
