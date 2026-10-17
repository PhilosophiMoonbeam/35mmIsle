#ifndef ISLE_ACTOR_OWNER_SOURCE_H
#define ISLE_ACTOR_OWNER_SOURCE_H
#include "actor_source.h"
#include "source_command_scope.h"
typedef struct IsleActorCommandData {
    int32_t cmd;
    IsleGameObject* source;
} IsleActorCommandData;
typedef struct IsleActorFocusState {
    uint32_t timer;
    IsleGameObject* current;
} IsleActorFocusState;
#ifdef __cplusplus
extern "C" {
#endif
/* Nonwaiting leaves return before the C bridge reports native execution faults.
 * owner in the canonical record points directly to stable ActorOwner slot data. */
IsleGameResult isle_actor_owner_animation(IsleActorSource*, uint32_t header_identity, float start, int force);
IsleGameResult isle_actor_owner_step(IsleActorSource*);
IsleGameResult isle_actor_owner_interaction_context(IsleActorSource*, void**);
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
IsleGameResult isle_actor_owner_trigger_position(IsleActorSource*, IsleGameVec3);
float isle_actor_owner_animation_time(IsleActorSource*);
int isle_actor_owner_paused(IsleActorSource*);
IsleGameResult isle_actor_source_pause_object(IsleActorSource*, int paused);
/* Typed native factory arguments, never console structures or guest callbacks. */
typedef struct IsleActorFactoryBlock { void* owner; void* block; void* result; } IsleActorFactoryBlock;
typedef struct IsleActorFactorySpawn { const void* spawn; size_t definition; } IsleActorFactorySpawn;
typedef struct IsleActorFactoryInitialization { uint32_t identity; } IsleActorFactoryInitialization;
IsleGameObject* isle_actor_owner_spawn_ground_dl4(int32_t object_id, uint16_t id,
    IsleActorFactoryBlock*, IsleActorFactoryBlock*, IsleActorFactorySpawn*,
    const IsleActorFactoryInitialization*);
IsleGameObject* isle_actor_owner_spawn_ground(int32_t object_id, uint16_t id,
    IsleActorFactoryBlock*, IsleActorFactoryBlock*, IsleActorFactorySpawn*,
    const IsleActorFactoryInitialization*);
IsleGameObject* func_beach_802CAC48(int32_t object_id, uint16_t id,
    IsleActorFactoryBlock*, IsleActorFactoryBlock*, IsleActorFactorySpawn*);
void func_beach_802CAB50(IsleGameObject*);
void func_beach_802CAC08(IsleGameObject*);
void func_8035E298_4FE6A8(IsleGameObject*);
void func_8035E174_4FE584(IsleGameObject*, IsleGameVec3*);
typedef struct IsleActorGeoDefinition { uint32_t identity; } IsleActorGeoDefinition;
typedef struct IsleActorGeoSpawn {
    int32_t id;
    IsleGameVec3 translation, euler, scale;
    const void* path;
    int32_t behavior;
} IsleActorGeoSpawn;
IsleGameObject* Pokemon_AddAtGeo(IsleGameObject*, uint16_t, const IsleActorGeoDefinition*);
IsleGameResult isle_actor_owner_yaw(IsleActorSource*, float);
IsleGameResult isle_actor_owner_magikarp_active(IsleActorSource*, int);
IsleGameResult isle_actor_owner_ground_node(IsleActorSource*, size_t*);
IsleGameResult isle_actor_owner_node_world(IsleActorSource*, size_t, IsleGameVec3*, IsleGameVec3*);
IsleGameResult isle_actor_owner_ground(IsleActorSource*, float, float, IsleActorGround*);
/* A live canonical caller may observe NULL before the first World block. */
IsleGameResult isle_actor_owner_current_block(IsleActorSource*, void**);
IsleGameResult isle_actor_owner_add_one(IsleActorSource*, void*, void*, const IsleActorGeoSpawn*,
    const IsleActorGeoDefinition*, IsleGameObject**);
IsleGameResult isle_actor_owner_copy_position(IsleActorSource*, IsleGameObject*);
IsleGameResult isle_actor_source_id_in_use(IsleGame*, uint32_t, int*);
/* Checked, nonwaiting native link3 cursor. NULL previous starts at the head;
 * otherwise previous must still be a live same-game link3 object. Each returned
 * object has its identical canonical bound record and actual attached model.
 * End and errors clear every supplied output; unbound nodes are never skipped.
 * Caller retains all lifetimes and excludes list/binding/model mutation for the
 * traversal. Expired/recycled raw previous handles are not supported. */
IsleGameResult isle_actor_source_link3_next(IsleGame*, IsleGameObject* previous,
    IsleGameObject** out_object, IsleActorSource** out_source, IsleGameModel** out_model);
void isle_actor_source_yaw(IsleGameObject*, float);
void isle_actor_source_magikarp_active(IsleGameObject*, int);
size_t isle_actor_source_ground_node(IsleGameObject*);
void isle_actor_source_node_world(IsleGameObject*, size_t, IsleGameVec3*, IsleGameVec3*);
void isle_actor_source_ground(IsleGameObject*, float, float, IsleActorGround*);
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
int32_t isle_actor_source_flute(IsleGameObject*);
float isle_actor_source_distance(IsleGameObject*, IsleGameObject*);
float isle_actor_source_neighbors(IsleGameObject*, IsleGameVec3*);
IsleGameObject* isle_actor_source_player(IsleGameObject*);
IsleActorFocusState isle_actor_source_focus(IsleGameObject*);
int32_t isle_actor_source_direction(IsleGameObject*);
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
