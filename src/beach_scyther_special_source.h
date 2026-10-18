#ifndef ISLE_BEACH_SCYTHER_SPECIAL_SOURCE_H
#define ISLE_BEACH_SCYTHER_SPECIAL_SOURCE_H
#include "actor_owner_source.h"
#ifdef __cplusplus
extern "C" {
#endif
void func_beach_802C8D74(int32_t, uint16_t, IsleActorFactoryBlock*, IsleActorFactoryBlock*, IsleActorFactorySpawn*);
void func_beach_802C8DB0(IsleGameObject*);
void func_beach_802C8DE8(IsleGameObject*);
void func_beach_802C8E20(IsleGameObject*);
void func_beach_802C8E90(IsleGameObject*);
void func_beach_802C8F48(IsleGameObject*);
void func_beach_802C907C(IsleGameObject*);
void func_beach_802C90E0(IsleGameObject*);
void func_beach_802C914C(IsleGameObject*);
void func_beach_802C91CC(int32_t, uint16_t, IsleActorFactoryBlock*, IsleActorFactoryBlock*, IsleActorFactorySpawn*);
void func_beach_802C9204(int32_t, uint16_t, IsleActorFactoryBlock*, IsleActorFactoryBlock*, IsleActorFactorySpawn*);
void func_beach_802CA0A0(IsleGameObject*);
void func_beach_802CA0F4(IsleGameObject*);
void func_beach_802CA148(IsleGameObject*);
void func_beach_802CA198(IsleGameObject*);
void func_beach_802CA1BC(IsleGameObject*);
void func_beach_802CA23C(IsleGameObject*);
void func_beach_802CA2BC(int32_t, uint16_t, IsleActorFactoryBlock*, IsleActorFactoryBlock*, IsleActorFactorySpawn*);
void func_beach_802CBCE0(IsleGameObject*);
void func_beach_802CBD04(IsleGameObject*);
void func_beach_802CBDA0(int32_t, uint16_t, IsleActorFactoryBlock*, IsleActorFactoryBlock*, IsleActorFactorySpawn*);
void func_beach_802CBDD8(IsleGameObject*);
/* Reuse the existing ActorOwner's ordinary spawn followed by its real DL4
 * transfer; the initialization identity selects the actual resource record. */
IsleGameObject* isle_actor_owner_spawn_dl4(int32_t, uint16_t, IsleActorFactoryBlock*, IsleActorFactoryBlock*, IsleActorFactorySpawn*, const IsleActorFactoryInitialization*);
/* A returning native leaf: no wait/fault may cross Environment ownership. */
IsleGameResult isle_actor_owner_environment_sound(IsleActorSource*, uint8_t, int32_t);
void isle_beach_scyther_special_play_sound(IsleGameObject*, uint8_t, int32_t);
#ifdef __cplusplus
}
#endif
#endif
