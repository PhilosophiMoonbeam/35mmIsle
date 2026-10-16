#ifndef ISLE_ACTOR_OWNER_SOURCE_H
#define ISLE_ACTOR_OWNER_SOURCE_H
#include "actor_source.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Nonwaiting leaves return before the C bridge reports native execution faults.
 * owner in the canonical record points directly to stable ActorOwner slot data. */
IsleGameResult isle_actor_owner_animation(IsleActorSource*, uint32_t header_identity, float start, int force);
IsleGameResult isle_actor_owner_step(IsleActorSource*);
IsleGameResult isle_actor_owner_interactions(IsleActorSource*);
IsleGameResult isle_actor_owner_cleanup(IsleActorSource*);
IsleGameResult isle_actor_owner_commands(IsleActorSource*);
IsleGameResult isle_actor_owner_distance(IsleActorSource*);
IsleGameResult isle_actor_owner_local_subtract_y(IsleActorSource*, float);
IsleGameResult isle_actor_owner_visibility(IsleActorSource*, int hidden);
IsleGameResult isle_actor_owner_trigger_position(IsleActorSource*, IsleGameVec3);
float isle_actor_owner_animation_time(IsleActorSource*);
int isle_actor_owner_paused(IsleActorSource*);
IsleGameResult isle_actor_source_pause_object(IsleActorSource*, int paused);
IsleGameResult isle_actor_source_validate_model(IsleGame*, IsleGameObject*, IsleGameModel*);
/* Typed native factory arguments, never console structures or guest callbacks. */
typedef struct IsleActorFactoryBlock { void* owner; void* block; void* result; } IsleActorFactoryBlock;
typedef struct IsleActorFactorySpawn { const void* spawn; size_t definition; } IsleActorFactorySpawn;
typedef struct IsleActorFactoryInitialization { uint32_t identity; } IsleActorFactoryInitialization;
IsleGameObject* isle_actor_owner_spawn_ground_dl4(int32_t object_id, uint16_t id,
    IsleActorFactoryBlock*, IsleActorFactoryBlock*, IsleActorFactorySpawn*,
    const IsleActorFactoryInitialization*);
IsleGameObject* func_8035E0D4_4FE4E4(int32_t object_id, uint16_t id,
    IsleActorFactoryBlock*, IsleActorFactoryBlock*, IsleActorFactorySpawn*);
void isle_actor_source_animation_process(IsleGameObject*);
void isle_actor_source_distance_process(IsleGameObject*);
void Pokemon_Update(IsleGameObject*);
void Pokemon_Cleanup(IsleGameObject*);
void Pokemon_RunCleanup(IsleGameObject*);
void Pokemon_WaitForFlag(IsleGameObject*, int32_t);
void Pokemon_RunInteractions(IsleGameObject*);
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
