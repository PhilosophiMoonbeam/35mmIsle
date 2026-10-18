#ifndef ISLE_BEACH_EEVEE_CHANSEY_SOURCE_H
#define ISLE_BEACH_EEVEE_CHANSEY_SOURCE_H
#include "actor_owner_source.h"
#ifdef __cplusplus
extern "C" {
#endif
void func_beach_802C72E0(IsleGameObject*);
void func_beach_802C737C(IsleGameObject*);
void func_beach_802C73EC(IsleGameObject*);
void func_beach_802C7490(IsleGameObject*);
void func_beach_802C74EC(IsleGameObject*);
void func_beach_802C755C(IsleGameObject*);
void func_beach_802C75D0(IsleGameObject*);
void func_beach_802C762C(IsleGameObject*);
void func_beach_802C76C4(IsleGameObject*);
void func_beach_802C7748(IsleGameObject*);
void func_beach_802C77CC(IsleGameObject*);
void func_beach_802C77F0(IsleGameObject*);
void func_beach_802C7864(IsleGameObject*);
void func_beach_802C78B0(IsleGameObject*);
void func_beach_802C78E8(IsleGameObject*);
void func_beach_802C796C(IsleGameObject*);
void func_beach_802C79DC(IsleGameObject*);
void func_beach_802C7A60(IsleGameObject*);
void func_beach_802C7AD0(int32_t, uint16_t, IsleActorFactoryBlock*, IsleActorFactoryBlock*, IsleActorFactorySpawn*);
void func_beach_802C9240(IsleGameObject*);
void func_beach_802C9274(IsleGameObject*);
void func_beach_802C92BC(IsleGameObject*);
void func_beach_802C9348(IsleGameObject*);
void func_beach_802C93B8(IsleGameObject*);
void func_beach_802C943C(IsleGameObject*);
void func_beach_802C94F0(IsleGameObject*);
void func_beach_802C9580(IsleGameObject*);
void func_beach_802C95F0(IsleGameObject*);
void func_beach_802C9694(IsleGameObject*);
void func_beach_802C96F0(IsleGameObject*);
void func_beach_802C9760(IsleGameObject*);
void func_beach_802C97D4(IsleGameObject*);
void func_beach_802C9830(IsleGameObject*);
void func_beach_802C98F4(IsleGameObject*);
void func_beach_802C9978(IsleGameObject*);
void func_beach_802C999C(IsleGameObject*);
void func_beach_802C9A10(IsleGameObject*);
void func_beach_802C9A7C(int32_t, uint16_t, IsleActorFactoryBlock*, IsleActorFactoryBlock*, IsleActorFactorySpawn*);
/* Returning common leaf: owns no global, waits, behavior call or fault. The
 * scene retains this one canonical native path through both species' joins. */
IsleGameResult isle_actor_interaction_scene_path(IsleActorSource*, uint32_t, const void***);
const void** isle_beach_eevee_chansey_scene_path(IsleGameObject*);
/* Preserve Chansey's original four-row automatic array, including its terminal.
 * Caller owns both the copied rows and the returned descriptor across the wait. */
IsleActorInteractionGraph isle_beach_eevee_chansey_local_graph(IsleGameObject*, IsleActorInteraction rows[4]);
#ifdef __cplusplus
}
#endif
#endif
