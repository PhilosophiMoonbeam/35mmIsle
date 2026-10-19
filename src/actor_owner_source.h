#ifndef ISLE_ACTOR_OWNER_SOURCE_H
#define ISLE_ACTOR_OWNER_SOURCE_H
#include "actor_source.h"
#include "source_command_scope.h"
#include <stdbool.h>
typedef struct IsleActorCommandData {
    int32_t cmd;
    IsleGameObject* source;
} IsleActorCommandData;
typedef struct IsleActorFocusState {
    uint32_t timer;
    IsleGameObject* current;
} IsleActorFocusState;
/* Native semantic values: references borrow the retained game/model arena.
 * Read views expire at the next publication, wait, relocation or retirement. */
enum IsleActorAuxMask {
    ISLE_ACTOR_PX=1u<<0, ISLE_ACTOR_PY=1u<<1, ISLE_ACTOR_PZ=1u<<2,
    ISLE_ACTOR_RX=1u<<3, ISLE_ACTOR_RY=1u<<4, ISLE_ACTOR_RZ=1u<<5,
    ISLE_ACTOR_SX=1u<<6, ISLE_ACTOR_SY=1u<<7, ISLE_ACTOR_SZ=1u<<8
};
typedef enum IsleActorPositionSpace {
    ISLE_ACTOR_POSITION_ROOT, ISLE_ACTOR_POSITION_AUX
} IsleActorPositionSpace;
typedef struct IsleActorPositionRef {
    IsleGameModelRef model;
    IsleActorPositionSpace space;
} IsleActorPositionRef;
typedef struct IsleActorNodeRef { IsleGameModelRef model; size_t node; } IsleActorNodeRef;
typedef struct IsleActorPathInfo { int32_t kind; float duration; } IsleActorPathInfo;
typedef struct IsleActorScaleInitialization {
    IsleGameVec3 scale, collision_center;
    float radius;
} IsleActorScaleInitialization;
/* Own GAME app_level.h115-124 movement bits. */
enum IsleActorMovementFlags {
    MOVEMENT_FLAG_ON_GROUND=0x01,
    MOVEMENT_FLAG_UPDATE_TARGET_POS=0x02,
    MOVEMENT_FLAG_STOP_WHEN_FLUTE_STOPPED_PLAYING=0x04,
    MOVEMENT_FLAG_STOP_WHEN_TURN_COMPLETED=0x08,
    MOVEMENT_FLAG_FIXED_HEIGHT=0x10,
    MOVEMENT_FLAG_TURN_TO_PLAYER=0x20,
    MOVEMENT_FLAG_TURN_AWAY=0x40,
    MOVEMENT_FLAG_TURN_GRADUALLY=0x80
};
#ifdef __cplusplus
extern "C" {
#endif
/* Nonwaiting leaves return before the C bridge reports native execution faults.
 * owner in the canonical record points directly to stable ActorOwner slot data. */
IsleGameResult isle_actor_owner_animation(IsleActorSource*, uint32_t header_identity, float start, int force);
void isle_actor_source_set_animation(IsleGameObject*, uint32_t header_identity, float start, int force);
/* Masked native object flags only; Pokemon tangible/flags are unchanged. */
void isle_actor_source_object_flags(IsleGameObject*, uint32_t mask, uint32_t value);
/* AUX stores consume only mask-selected inputs and merge against the latest
 * canonical RT/RTS placement. Scale components require RTS. Captured position
 * references preserve both model incarnation and selected ROOT/AUX space;
 * they never retarget to a replacement model or a joined-target fallback. */
IsleGameResult isle_actor_owner_self_model(IsleActorSource*, IsleGameModelRef*);
IsleGameResult isle_actor_owner_aux_read(IsleActorSource*, const IsleGameModelRef*, const IsleGameModelTransform**);
IsleGameResult isle_actor_owner_aux_store(IsleActorSource*, const IsleGameModelRef*, uint32_t, const IsleGameModelTransform*);
IsleGameResult isle_actor_owner_root_position_store(IsleActorSource*, const IsleGameModelRef*, const IsleGameVec3*);
IsleGameResult isle_actor_owner_block_indices(IsleActorSource*, int32_t* current, int32_t* base);
IsleGameResult isle_actor_owner_schedule_cleanup(IsleActorSource*);
IsleGameResult isle_actor_owner_step_walk(IsleActorSource*, float, float, uint32_t, int32_t*);
IsleGameResult isle_actor_owner_scale_initialization(IsleActorSource*, IsleActorScaleInitialization*);
IsleGameResult isle_actor_owner_target_position_ref(IsleActorSource*, IsleGameObject*, IsleActorPositionRef*);
IsleGameResult isle_actor_owner_position_read(IsleActorSource*, const IsleActorPositionRef*, const IsleGameVec3**);
IsleGameResult isle_actor_owner_node_world_ref(IsleActorSource*, const IsleActorNodeRef*, IsleGameVec3*, IsleGameVec3*);
/* Native retained alternatives, admitted cold for this exact initialized family.
 * Index is the Original misc slot, not a universal pointer interpretation.
 * Object get clears output on failure; references retain actual incarnations.
 * Path save/restore retains immutable actual World provenance, including NULL;
 * restore before save or after retirement/reuse is rejected. */
IsleGameResult isle_actor_owner_misc_object_get(IsleActorSource*, uint32_t, IsleGameObject**);
IsleGameResult isle_actor_owner_misc_object_set(IsleActorSource*, uint32_t, IsleGameObject*);
IsleGameResult isle_actor_owner_saved_path_save(IsleActorSource*, uint32_t);
IsleGameResult isle_actor_owner_saved_path_restore(IsleActorSource*, uint32_t);
IsleGameResult isle_actor_owner_path_info(IsleActorSource*, const void*, IsleActorPathInfo*);
IsleGameResult isle_actor_owner_path_position(IsleActorSource*, const void*, float, IsleGameVec3*);
IsleGameResult isle_actor_owner_path_velocity(IsleActorSource*, const void*, float, IsleGameVec3*);
IsleGameResult isle_actor_owner_current_world_position(IsleActorSource*, IsleGameVec3*);
IsleGameModelRef isle_actor_source_self_model(IsleGameObject*);
const IsleGameModelTransform* isle_actor_source_aux(IsleGameObject*, const IsleGameModelRef*);
void isle_actor_source_aux_store(IsleGameObject*, const IsleGameModelRef*, uint32_t, const IsleGameModelTransform*);
int32_t isle_actor_source_step_walk(IsleGameObject*, float, float, uint32_t);
IsleActorScaleInitialization isle_actor_source_scale_initialization(IsleGameObject*);
IsleActorPositionRef isle_actor_source_target_position_ref(IsleGameObject*, IsleGameObject*);
const IsleGameVec3* isle_actor_source_position_read(IsleGameObject*, const IsleActorPositionRef*);
void isle_actor_source_node_world_ref(IsleGameObject*, const IsleActorNodeRef*, IsleGameVec3*, IsleGameVec3*);
IsleActorPathInfo isle_actor_source_path_info(IsleGameObject*, const void*);
void isle_actor_source_path_position(IsleGameObject*, const void*, float, IsleGameVec3*);
void isle_actor_source_path_velocity(IsleGameObject*, const void*, float, IsleGameVec3*);
IsleGameVec3 isle_actor_source_current_world_position(IsleGameObject*);
IsleGameResult isle_actor_owner_step(IsleActorSource*);
IsleGameResult isle_actor_owner_interaction_context(IsleActorSource*, void**);
/* Resolve retained native graph storage only after complete family admission. */
IsleGameResult isle_actor_owner_graph(IsleActorSource*, uint32_t, const IsleActorInteractionGraph**);
const IsleActorInteractionGraph* isle_actor_source_graph(IsleGameObject*, uint32_t);
IsleGameResult isle_actor_owner_cleanup(IsleActorSource*);
IsleGameResult isle_actor_owner_commands_begin(IsleActorSource*, IsleSourceCommandScope*);
IsleGameResult isle_actor_owner_commands_end(IsleActorSource*, IsleSourceCommandScope*, IsleGameResult, int);
IsleGameResult isle_actor_owner_command_status(IsleActorSource*);
IsleGameResult isle_actor_owner_execution_enabled(IsleActorSource*, int32_t*);
IsleGameResult isle_actor_source_install_update(IsleGame*, IsleGameObject*);
IsleGameResult isle_actor_owner_reference_assign(IsleActorSource*, IsleGameObject** field,
    IsleGameObject* sender);
/* Settles only the current persistent command token; preserves FIFO suffix.
 * Call after canonical-field adoption and before a nonreturning C transfer. */
IsleGameResult isle_actor_owner_command_before_transfer(IsleActorSource*);
IsleGameResult isle_actor_owner_distance(IsleActorSource*);
IsleGameResult isle_actor_owner_local_subtract_y(IsleActorSource*, float);
IsleGameResult isle_actor_owner_visibility(IsleActorSource*, int hidden);
IsleGameResult isle_actor_owner_environment_sound(IsleActorSource*, uint8_t category, int32_t sound);
IsleGameResult isle_actor_owner_trigger_position(IsleActorSource*, IsleGameVec3);
float isle_actor_owner_animation_time(IsleActorSource*);
int isle_actor_owner_paused(IsleActorSource*);
IsleGameResult isle_actor_source_pause_object(IsleActorSource*, int paused);
/* Typed native factory arguments, never console structures or guest callbacks. */
typedef struct IsleActorFactoryBlock { void* owner; void* block; void* result; } IsleActorFactoryBlock;
typedef struct IsleActorFactorySpawn {
    const void* spawn;
    size_t definition;
    /* Synchronous borrow for authored Meowth/Valley behavior reads, or the
     * exact native automatic-spawn portability repair before Valley's branch. */
    const int32_t* behavior;
    size_t automatic_definition;
} IsleActorFactorySpawn;
typedef struct IsleActorFactoryInitialization { uint32_t identity; } IsleActorFactoryInitialization;
int32_t isle_actor_factory_behavior(const IsleActorFactorySpawn*);
/* Actual native Game/domain fault plus abandoned-update STOPPED state. */
IsleGameResult isle_actor_source_execution_status(IsleGame*);
/* Detachment only, after the course owner's genuine native join fences. Allows
 * ONLY its acknowledged returning scene-exit stop; active execution remains
 * STOPPED and unrelated nonreturning stops/cancellation remain terminal. */
IsleGameResult isle_actor_source_retirement_status(IsleGame*);
/* Original Pokemon_RemovePokemons, shared by course providers. The returning
 * C fence requires the actual current Actor to remain in the terminated list;
 * cleanup therefore cannot discard its owning native service call. */
void isle_actor_remove_pokemons(IsleGameObject*, const uint16_t*);
IsleGameResult isle_actor_source_remove_pokemons_call(IsleGame*,
    IsleGameObject*, const uint16_t*, size_t count);
typedef IsleGameObject* (*IsleActorReturningFactory)(int32_t, uint16_t,
    IsleActorFactoryBlock*, IsleActorFactoryBlock*, IsleActorFactorySpawn*);
typedef void (*IsleActorVoidFactory)(int32_t, uint16_t,
    IsleActorFactoryBlock*, IsleActorFactoryBlock*, IsleActorFactorySpawn*);
/* Returning C-only fence. Source faults never cross the C++ caller. */
IsleGameResult isle_actor_source_factory_call(IsleGame*, IsleActorReturningFactory,
    IsleActorVoidFactory, int32_t, uint16_t, IsleActorFactoryBlock*,
    IsleActorFactoryBlock*, IsleActorFactorySpawn*, IsleGameObject**);
/* Exact synchronous native ticket capability, never an object/guest cast.
 * Clears output on failure; valid tickets identify Game even if Source context
 * admission returned no Source. Native leaf returns before C checked transfers.
 * Non-OK checked requires a live issued ticket and Player C factory fence;
 * invalid/unissued tickets fail closed, never fault through a guessed Game. */
IsleGameResult isle_actor_owner_factory_game(const IsleActorFactoryBlock*,
    const IsleActorFactoryBlock*, const IsleActorFactorySpawn*, IsleGame**);
void isle_actor_source_factory_checked(const IsleActorFactoryBlock*,
    const IsleActorFactoryBlock*, const IsleActorFactorySpawn*, IsleGameResult);
IsleGameObject* isle_actor_owner_spawn_ground_dl4(int32_t object_id, uint16_t id,
    IsleActorFactoryBlock*, IsleActorFactoryBlock*, IsleActorFactorySpawn*,
    const IsleActorFactoryInitialization*);
IsleGameObject* isle_actor_owner_spawn_ground(int32_t object_id, uint16_t id,
    IsleActorFactoryBlock*, IsleActorFactoryBlock*, IsleActorFactorySpawn*,
    const IsleActorFactoryInitialization*);
/* Complete ground factory, then the original ordinary late AUX-Y store. */
IsleGameObject* isle_actor_owner_spawn(int32_t object_id, uint16_t id,
    IsleActorFactoryBlock*, IsleActorFactoryBlock*, IsleActorFactorySpawn*,
    const IsleActorFactoryInitialization*);
/* Ordinary factory, followed by its genuine Source DL4 publication. */
IsleGameObject* isle_actor_owner_spawn_dl4(int32_t object_id, uint16_t id,
    IsleActorFactoryBlock*, IsleActorFactoryBlock*, IsleActorFactorySpawn*,
    const IsleActorFactoryInitialization*);
IsleGameObject* func_beach_802CAC48(int32_t object_id, uint16_t id,
    IsleActorFactoryBlock*, IsleActorFactoryBlock*, IsleActorFactorySpawn*);
void func_beach_802CAB50(IsleGameObject*);
void func_beach_802CAC08(IsleGameObject*);
void func_8035E298_4FE6A8(IsleGameObject*);
void func_8035E174_4FE584(IsleGameObject*, IsleGameVec3*);
IsleGameObject* func_8035E1D4_4FE5E4(IsleGameObject*);
IsleGameObject* func_8035E238_4FE648(IsleGameObject*);
typedef struct IsleActorGeoDefinition { uint32_t identity; } IsleActorGeoDefinition;
typedef struct IsleActorGeoSpawn {
    int32_t id;
    IsleGameVec3 translation, euler, scale;
    const void* path;
    int32_t behavior;
} IsleActorGeoSpawn;
IsleGameObject* Pokemon_AddAtGeo(IsleGameObject*, uint16_t, const IsleActorGeoDefinition*);
IsleGameResult isle_actor_owner_magikarp_active(IsleActorSource*, int);
IsleGameResult isle_actor_owner_ground_node(IsleActorSource*, size_t*);
IsleGameResult isle_actor_owner_node_world(IsleActorSource*, size_t, IsleGameVec3*, IsleGameVec3*);
/* Native success and source terrain presence are independent. Missing terrain
 * fills {-9000,{0,1,0},UINT32_MAX} and returns found=0 without a native fault. */
IsleGameResult isle_actor_owner_ground(IsleActorSource*, float, float, IsleActorGround*, int32_t* found);
float Pokemon_GetGroundAt(IsleGameObject*, float, float);
float func_803623F4_502804(IsleGameObject*, float, float);
bool Pokemon_Turn(IsleGameObject*, IsleGameModelRef, float, float);
void Pokemon_TurnToModelNode(IsleGameObject*, IsleActorNodeRef, float, uint32_t);
void func_80361FBC_5023CC(IsleGameObject*, IsleGameVec3*, const void*, float);
float Pokemon_WrapAngleSum(float, float);
float Pokemon_HeadingBetweenPoints(const IsleGameVec3*, const IsleGameVec3*);
float Pokemon_GetHeadingVector(const IsleGameVec3*, const IsleGameVec3*, IsleGameVec3*, float);
void Pokemon_SetScale(IsleGameObject*, float);
void Pokemon_MoveXZ(IsleGameObject*);
void Pokemon_Jump(IsleGameObject*, float, float, float, float);
void Pokemon_JumpToHeight(IsleGameObject*, float, float, float, float);
void Pokemon_Fall(IsleGameObject*, float, float, float, float);
void Pokemon_FallToHeight(IsleGameObject*, float, float, float, float);
void Pokemon_FallDownOnGround(IsleGameObject*, float, int32_t);
int32_t Pokemon_StepWalkInDirection(IsleGameObject*, float, uint32_t);
bool Pokemon_StepWalkInDirectionFacing(IsleGameObject*, uint32_t);
void Pokemon_RunInCircles(IsleGameObject*, float, float, int32_t);
void Pokemon_RunInCirclesDefault(IsleGameObject*, float);
int32_t Pokemon_HearsPokeFlute(IsleGameObject*);
void Pokemon_TurnToTarget(IsleGameObject*, float, uint32_t);
void func_80361720_501B30(IsleGameObject*);
void Pokemon_RunToTarget(IsleGameObject*, float, float, uint32_t);
void Pokemon_RunAwayFromTarget(IsleGameObject*, float, float, uint32_t);
void Pokemon_RunAwayDefault(IsleGameObject*, float);
void Pokemon_SetTargetPos(IsleGameObject*, float, float);
int32_t Pokemon_StepToTargetPos(IsleGameObject*, float, uint32_t);
void Pokemon_RunToTargetPos(IsleGameObject*, float);
void Pokemon_RunToTargetDefault(IsleGameObject*, float);
void Pokemon_TurnToPokeFlute(IsleGameObject*, float);
void Pokemon_ResetPathPos(IsleGameObject*);
void Pokemon_FollowPath(IsleGameObject*, float, float, float, float, uint32_t);
/* A live canonical caller may observe NULL before the first World block. */
IsleGameResult isle_actor_owner_current_block(IsleActorSource*, void**);
IsleGameResult isle_actor_owner_add_one(IsleActorSource*, void*, void*, const IsleActorGeoSpawn*,
    const IsleActorGeoDefinition*, IsleGameObject**);
IsleGameResult isle_actor_owner_copy_position(IsleActorSource*, IsleGameObject*);
IsleGameResult isle_actor_source_id_in_use_link(IsleGame*, uint8_t link, uint32_t, int*);
/* Checked, nonwaiting native link3 cursor. NULL previous starts at the head;
 * otherwise previous must still be a live same-game link3 object. Each returned
 * object has its identical canonical bound record and actual attached model.
 * End and errors clear every supplied output; unbound nodes are never skipped.
 * Caller retains all lifetimes and excludes list/binding/model mutation for the
 * traversal. Expired/recycled raw previous handles are not supported. */
IsleGameResult isle_actor_source_link3_next(IsleGame*, IsleGameObject* previous,
    IsleGameObject** out_object, IsleActorSource** out_source, IsleGameModel** out_model);
void isle_actor_source_magikarp_active(IsleGameObject*, int);
size_t isle_actor_source_ground_node(IsleGameObject*);
void isle_actor_source_node_world(IsleGameObject*, size_t, IsleGameVec3*, IsleGameVec3*);
int32_t isle_actor_source_ground(IsleGameObject*, float, float, IsleActorGround*);
void* isle_actor_source_current_block(IsleGameObject*);
IsleGameObject* isle_actor_source_add_one(IsleGameObject*, void*, void*, const IsleActorGeoSpawn*,
    const IsleActorGeoDefinition*);
void isle_actor_source_copy_position(IsleGameObject*, IsleGameObject*);
void isle_actor_source_publish_position(IsleGameObject*, IsleGameVec3);
IsleGameObject* func_8035E0D4_4FE4E4(int32_t object_id, uint16_t id,
    IsleActorFactoryBlock*, IsleActorFactoryBlock*, IsleActorFactorySpawn*);
void isle_actor_source_animation_process(IsleGameObject*);
void isle_actor_source_distance_process(IsleGameObject*);
void Pokemon_Update(IsleGameObject*);
void Pokemon_Cleanup(IsleGameObject*);
void Pokemon_RunCleanup(IsleGameObject*);
void Pokemon_WaitForFlag(IsleGameObject*, int32_t);
void Pokemon_RunInteractions(IsleGameObject*);
void Pokemon_HandleCommand(IsleActorCommandData);
IsleGameObject* Pokemon_GetClosestItem(IsleGameObject*, float*);
const IsleActorInteraction* isle_actor_source_interaction_rows(IsleGameObject*);
IsleGameVec3 isle_actor_source_position(IsleGameObject*, IsleGameObject*, int auxiliary);
void isle_actor_source_item_iterator(IsleGameObject*);
IsleGameObject* isle_actor_source_item_next(IsleGameObject*);
struct IsleItemSource* isle_actor_source_item(IsleGameObject*, IsleGameObject*);
/* Original Pokemon_EatApple is retained by the Item importer; the native
 * nonwaiting Item Owner runs that body, including its real deletion path. */
void isle_actor_source_eat_apple(IsleGameObject*);
/* Commands enter the shared checked Services queues. Link broadcasts retain
 * actual native update-list order and sender identity lifetime accounting. */
void isle_actor_source_send_command(IsleGameObject*, int32_t, IsleGameObject*);
/* Requester supplies the admitted Actor context; destination may be the real
 * Player or another Actor. Destination never masquerades as the requester. */
void isle_actor_source_send_addressed(IsleGameObject* requester, IsleGameObject* destination,
    int32_t command, IsleGameObject* sender);
void isle_actor_source_send_to_link(IsleGameObject*, uint8_t, int32_t, IsleGameObject*);
int32_t isle_actor_source_flute(IsleGameObject*);
float isle_actor_source_distance(IsleGameObject*, IsleGameObject*);
float isle_actor_source_neighbors(IsleGameObject*, IsleGameVec3*);
IsleGameObject* isle_actor_source_player(IsleGameObject*);
IsleActorFocusState isle_actor_source_focus(IsleGameObject*);
int32_t isle_actor_source_direction(IsleGameObject*);
void isle_actor_source_level_progress(IsleGameObject*, int32_t* block_count, float* block_part);
int32_t isle_actor_source_execution_enabled(IsleGameObject*);
void isle_actor_source_pause_all(IsleGameObject*);
void isle_actor_source_reference_assign(IsleGameObject*, IsleGameObject**, IsleGameObject*);
void func_8035DF20_4FE330(IsleGameObject*);
void func_8035DFB0_4FE3C0(IsleGameObject*);
void func_8035E034_4FE444(IsleGameObject*);
IsleGameResult func_8035E264_4FE674(IsleGameObject*, IsleGameVec3*);
/* Exact source trigger signal body; position publication precedes signal. */
void isle_actor_source_common1003_trigger(IsleGameObject*, IsleGameVec3);
#ifdef __cplusplus
}
#endif
#endif
