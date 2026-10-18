#ifndef ISLE_BEACH_MEOWTH_SOURCE_H
#define ISLE_BEACH_MEOWTH_SOURCE_H
#include "actor_owner_source.h"
#ifdef __cplusplus
extern "C" {
#endif
void func_beach_802C5C60(IsleGameObject* obj);
void func_beach_802C5DAC(IsleGameObject* obj);
void func_beach_802C5E88(IsleGameObject* obj);
void func_beach_802C5EF8(IsleGameObject* obj);
void func_beach_802C5F9C(IsleGameObject* obj);
void func_beach_802C5FF8(IsleGameObject* obj);
void func_beach_802C60E8(IsleGameObject* obj);
void func_beach_802C619C(IsleGameObject* obj);
void func_beach_802C6210(IsleGameObject* obj);
void func_beach_802C6288(IsleGameObject* obj);
void func_beach_802C62E4(IsleGameObject* obj);
void func_beach_802C63F4(IsleGameObject* obj);
void func_beach_802C6478(IsleGameObject* obj);
void func_beach_802C649C(IsleGameObject* obj);
void func_beach_802C6514(IsleGameObject* obj);
void func_beach_802C6580(IsleGameObject* obj);
void func_beach_802C660C(IsleGameObject* obj);
void func_beach_802C6658(IsleGameObject* obj);
void func_beach_802C667C(IsleGameObject* obj);
void func_beach_802C67F8(void);
void func_beach_802C6700(IsleGameObject* obj);
void func_beach_802C67E0(IsleGameObject* obj);
void func_beach_802C6854(IsleGameObject* obj);
void func_beach_802C68D8(IsleGameObject* obj);
void func_beach_802C6944(IsleGameObject* arg0);
void func_beach_802C69D4(IsleGameObject* obj);
void func_beach_802C6A3C(IsleGameObject* obj);
void func_beach_802C6AA0(IsleGameObject* obj);
void func_beach_802C6B28(IsleGameObject* obj);
void func_beach_802C6B8C(IsleGameObject* obj);
void func_beach_802C6C10(IsleGameObject* obj);
void func_beach_802C6C60(IsleGameObject* obj);
void func_beach_802C6CC4(IsleGameObject* obj);
void func_beach_802C6D20(IsleGameObject* obj);
void func_beach_802C6D44(IsleGameObject* obj);
void func_beach_802C6D9C(IsleGameObject* obj);
void func_beach_802C6DD4(IsleGameObject* obj);
void func_beach_802C6E40(IsleGameObject* obj);
void func_beach_802C6EC0(IsleGameObject* obj);
void func_beach_802C6F6C(IsleGameObject* obj);
void func_beach_802C6FD4(IsleGameObject* obj);
void func_beach_802C6FF8(IsleGameObject* obj);
void func_beach_802C7050(IsleGameObject* obj);
void func_beach_802C70C0(IsleGameObject* obj);
void func_beach_802C7130(IsleGameObject* obj);
void func_beach_802C71E8(IsleGameObject* arg0, float arg1, float arg2, float arg3, float arg4, float arg5, float arg6);
void func_beach_802C7270(int32_t gObjID, uint16_t id, IsleActorFactoryBlock* roomA, IsleActorFactoryBlock* roomB, IsleActorFactorySpawn* spawn);
/* These complete original common GAME helpers are uniquely imported here. */
void pokemonChangeBlock(IsleGameObject*, float, float, float, float, float, float);
void pokemonChangeBlockOnGround(IsleGameObject*, float, float, float, float, float, float);
float func_80363D8C_50419C(IsleGameObject*, float, float);
void Pokemon_ChangeBlockAndRemove(IsleGameObject*, float, float, float, float, float, float);
/* ground: 0 ordinary, 1 ground, -1 Meowth behavior, 2 Eevee removal. */
IsleGameResult isle_meowth_source_call_change_block(IsleGameObject*, float, float,
    float, float, float, float, int ground);
/* Native semantic AUX has one translation channel, never a console layout.
 * Its presence comes from the actual captured native placement mode. */
typedef struct IsleBeachBlockModel {
    IsleGameModelRef model;
    uint32_t translation_slots;
    int auxiliary;
} IsleBeachBlockModel;
IsleActorSource* isle_beach_block_source(IsleGameObject*);
IsleBeachBlockModel isle_beach_block_model(IsleGameObject*, int require_auxiliary);
IsleGameVec3 isle_beach_block_position(IsleGameObject*, const IsleBeachBlockModel*, int auxiliary);
void isle_beach_block_store(IsleGameObject*, const IsleBeachBlockModel*, int auxiliary, IsleGameVec3);
int32_t isle_beach_block_ground(IsleGameObject*, float, float, IsleActorGround*);
int32_t isle_beach_block_passed(IsleGameObject*);
void isle_beach_block_run_cleanup(IsleGameObject*);
IsleGameResult isle_actor_owner_block_indices(IsleActorSource*, int32_t*, int32_t*);
IsleGameResult isle_actor_owner_schedule_cleanup(IsleActorSource*);
IsleGameResult isle_actor_owner_root_position_store(IsleActorSource*,
    const IsleGameModelRef*, const IsleGameVec3*);
void func_beach_802CBDD8(IsleGameObject*);
const IsleActorRandomState* isle_actor_source_weighted(IsleGameObject*, uint32_t);
void isle_actor_source_visibility(IsleGameObject*, int);
/* Reacquire the captured native model incarnation after each original wait. */
IsleGameVec3 isle_beach_meowth_scale(IsleGameObject*, const IsleGameModelRef*);
#ifdef __cplusplus
}
#endif
#endif
