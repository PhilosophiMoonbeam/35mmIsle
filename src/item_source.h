#ifndef ISLE_ITEM_SOURCE_H
#define ISLE_ITEM_SOURCE_H

#include "actor_source.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    ISLE_ITEM_POKEFLUTE = 161,
    ISLE_ITEM_PESTER_BALL = 162,
    ISLE_ITEM_APPLE = 163,
    ISLE_ITEM_INVALID = 0,
    ISLE_ITEM_FLYING = 1,
    ISLE_ITEM_STILL = 2,
    ISLE_ITEM_BOUNCED = 1,
    ISLE_ITEM_TOUCHED_GROUND = 2,
    ISLE_ITEM_DELETED = 4
};
/* Source-semantic fields, NOT a console Item layout. Actual source u8 widths
 * are retained; native ownership follows them. Bind one canonical record to
 * the real GObj. No Actor payload, copied pose or renderer cache is an Item.
 * Keep this record through native join, all registered source borrows and the
 * independent preparation/GPU/readback fence before physical storage reuse. */
typedef struct IsleItemSource {
    uint8_t itemID, state, entryIndex, flags;
    float restTimer;
    IsleGameVec3 velocity, collisionVelocity, prevPos;
    void* owner;
    IsleGame* game;
    IsleGameObject* object;
} IsleItemSource;

/* Actual typed C callbacks; no guest function word or process adapter identity.
 * A collision callback may mutate the result. The four-vector provider may
 * mutate its inputs even on a negative return; source preserves those effects. */
typedef void (*IsleItemCollisionCallback)(IsleGameObject*, IsleActorGround*);
typedef int32_t (*IsleItemCollisionProvider)(IsleGameVec3*, IsleGameVec3*,
    IsleGameVec3*, IsleGameVec3*);

/* Owner-side bind/get return status and never fault or wait. get clears output
 * on error and rejects wrong-kind/unbound/retired/wrong-game objects. Bind
 * starts defined native zero state; Owner initialization supplies prevPos from
 * the actual initial root and collisionVelocity zero. This deliberately does
 * not reproduce source unwritten/recycled payload bytes. */
IsleGameResult isle_item_source_bind(IsleGame*, IsleGameObject*, IsleItemSource*, void* owner);
IsleGameResult isle_item_source_get(IsleGame*, IsleGameObject*, IsleItemSource**);
/* C callback only. Reports failure through the existing C execution fence,
 * never through an owning C++ frame. Owner startup uses bind/get instead. */
IsleItemSource* isle_item_source_require(IsleGameObject*);

#ifdef __cplusplus
}
#endif
#endif
