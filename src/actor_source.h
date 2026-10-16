#ifndef ISLE_ACTOR_SOURCE_H
#define ISLE_ACTOR_SOURCE_H

#include "35mmisle/engine.h"
#include "35mmisle/rng.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Native semantic data, not a console Pokemon layout. Exactly one record is
 * bound to the real GObj.userData; updateUser remains the engine's context.
 * Owner, RNG and record must survive all source processes and actual joined
 * object retirement. Serialization is the native game's owner/worker handoff.
 * After joined retirement a caller may zero/reuse the record; stale raw object
 * and process handles never become valid again merely because an address is
 * recycled. No automatic record reclamation or GPU completion is implied. */
typedef struct IsleActorSource {
    IsleGameProcess* stateProc;
    IsleGameProcess* pathProc;
    uint32_t processFlags;
    int32_t id;
    uint16_t flags;                /* Actual Pokemon semantics, not GObj flags. */
    const void* transitionGraph;
    const void* animHeader;
    uint32_t modelAnims, matAnims;
    int32_t loopCount, pokemonLoopTarget, counter;
    float lastAnimationFrame;
    int32_t tangible;
    const void* initData;
    const void* animSetup;
    void* baseBlock;
    const void* path;
    const void* forbiddenGround;
    IsleGameVec3 pos1, targetPos, euler, collisionOffset;
    float collisionRadius, playerDist, interactionDist;
    int32_t behavior;
    struct { int32_t field1; } miscVars[7];
    IsleGameObject* apple;
    IsleGameObject* interactionTarget;
    int32_t unk_18;
    uint8_t unk_E4;
    float jumpVel, hSpeed, facingYaw, pathParam;
    const void* eggGeo;
    uint16_t poseID, specialPoseID;
    isle_rng* rng;                 /* The application's actual State RNG. */
    void* owner;                   /* The actual native actor owner. */
    IsleGame* game;
    IsleGameObject* object;
} IsleActorSource;

typedef struct IsleActorRandomState {
    int32_t weight;
    IsleGameSourceCallback func;
} IsleActorRandomState;

enum {
    POKEMON_PROCESS_FLAG_ANIMATION_ENDED = 0x001,
    POKEMON_PROCESS_WAIT_ENDED = 0x004,
    POKEMON_PROCESS_FLAG_PATH_ENDED = 0x002,
    POKEMON_PROCESS_FLAG_AUX_ENDED = 0x008,
    POKEMON_PROCESS_TARGET_REACHED = 0x010,
    POKEMON_PROCESS_FLAG_MOVEMENT_PAUSED = 0x020
};

/* Bind once to an empty live payload and a zero-initialized caller-owned
 * record. Does not replace native update context, observers or model payload.
 * get() clears output on failure and rejects retired/unbound objects, including
 * an unbound recycled native slot. Raw stale handles remain forbidden.
 * Do not free the record from an enclosing source frame or retirement hook. */
IsleGameResult isle_actor_source_bind(IsleGame*, IsleGameObject*, IsleActorSource*, isle_rng*, void* owner);
IsleGameResult isle_actor_source_get(IsleGame*, IsleGameObject*, IsleActorSource**);

/* Exact privately generated GAME C definitions. These are ONLY for active
 * native C callbacks, not owner-side startup calls. Stackful self replacement,
 * stop, wait and failure abandon their C stack. No C++ automatic ownership may
 * straddle these calls. Owner startup uses isle_game_add_source_process and
 * checks its returned status; bind/get likewise report status without waiting.
 * Random lists retain source semantics: native callable identities, NULL func
 * sentinel, nonnegative weights and positive sum <=32768 (the existing RNG's
 * nonnegative product range). No guest address may be cast to a callback. Callers
 * clear their own completed state/path handles as the GAME behaviors do. */
void Pokemon_SetState(IsleGameObject*, IsleGameSourceCallback);
void Pokemon_SetStateRandom(IsleGameObject*, IsleActorRandomState*);
void Pokemon_StartAuxProc(IsleGameObject*, IsleGameSourceCallback);
void Pokemon_StopAuxProc(IsleGameObject*);
void Pokemon_StartPathProc(IsleGameObject*, IsleGameSourceCallback);
void Pokemon_WaitForFlagNoInteraction(IsleGameObject*, uint32_t flags);

/* C-only translation seams consumed by the private selected definitions. */
IsleActorSource* isle_actor_source_require(IsleGameObject*);
IsleGameProcess* isle_actor_source_create_process(IsleGameObject*, IsleGameSourceCallback, int32_t kind, int32_t priority);
int32_t isle_actor_source_rand_range(IsleGameObject*, int32_t range);

#ifdef __cplusplus
}
#endif
#endif
