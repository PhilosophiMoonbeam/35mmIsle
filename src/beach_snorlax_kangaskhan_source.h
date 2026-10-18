#ifndef ISLE_BEACH_SNORLAX_KANGASKHAN_SOURCE_H
#define ISLE_BEACH_SNORLAX_KANGASKHAN_SOURCE_H
#include "actor_owner_source.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Canonical native descriptors; the admitted ActorResources owns all actual
 * initialization/model/animation data. These are identities, not guest layouts. */
extern const IsleActorFactoryInitialization D_beach_802CC2F0;
extern const IsleActorFactoryInitialization D_beach_802CD7A8;
void func_beach_802C5620(IsleGameObject*);
void func_beach_802C56C8(IsleGameObject*);
void func_beach_802C5700(IsleGameObject*);
void func_beach_802C5768(IsleGameObject*);
void func_beach_802C5828(IsleGameObject*);
void func_beach_802C5884(IsleGameObject*);
void func_beach_802C58B8(IsleGameObject*);
void func_beach_802C58EC(IsleGameObject*);
void func_beach_802C594C(IsleGameObject*);
void func_beach_802C59C0(IsleGameObject*);
void func_beach_802C5A20(IsleGameObject*);
void func_beach_802C5A94(IsleGameObject*);
void func_beach_802C5AF4(IsleGameObject*);
void func_beach_802C5B68(IsleGameObject*);
void func_beach_802C5BD4(IsleGameObject*);
void func_beach_802C9AC0(IsleGameObject*);
void func_beach_802C9AE4(IsleGameObject*);
void func_beach_802C9B40(IsleGameObject*);
void func_beach_802C9B9C(IsleGameObject*);
void func_beach_802C9BF8(IsleGameObject*);
void func_beach_802C9C88(IsleGameObject*);
void func_beach_802C9CF8(IsleGameObject*);
void func_beach_802C9D70(IsleGameObject*);
void func_beach_802C9DCC(IsleGameObject*);
void func_beach_802C9E70(IsleGameObject*);
void func_beach_802C9EBC(IsleGameObject*);
void func_beach_802C9F30(IsleGameObject*);
void func_beach_802C9F9C(IsleGameObject*);
void func_beach_802CA010(IsleGameObject*);
void func_beach_802C9D68(void); /* Complete original empty, unreferenced helper. */
void func_beach_802C5C28(int32_t, uint16_t, IsleActorFactoryBlock*, IsleActorFactoryBlock*, IsleActorFactorySpawn*);
void func_beach_802CA05C(int32_t, uint16_t, IsleActorFactoryBlock*, IsleActorFactoryBlock*, IsleActorFactorySpawn*);
/* Common Source weighted resolver, implemented by the existing actor bridge. */
const IsleActorRandomState* isle_actor_source_weighted(IsleGameObject*, uint32_t);
#ifdef __cplusplus
}
#endif
#endif
