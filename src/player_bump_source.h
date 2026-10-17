#ifndef ISLE_PLAYER_BUMP_SOURCE_H
#define ISLE_PLAYER_BUMP_SOURCE_H

#include "player_source.h"
#include "actor_source.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Sole original bump globals for one canonical Player context. This is native
 * semantic storage, not a console layout or a collision-field mirror. The
 * fixed-address BumpOwner binds player->bump before original Init and retains
 * this record through real native joins AND every queued/stored sender borrow.
 * The original unused Update local remains on its actual C continuation stack. */
struct IslePlayerBumpSource {
    IslePlayerSource* player;
    void* owner;
    IsleGameResult status;
    int32_t BumpDetector_ProximityLevel;
    int8_t BumpDetector_IsEnabled;
    float BumpDetector_Thresholds[4];
    IsleGameObject* BumpDetector_Object;
};

/* Complete privately materialized own-GAME definitions. Update is the actual
 * one-argument kind0/priority9 C callback; all other calls return without waits
 * and receive the canonical context explicitly. Parent C callsites use
 * isle_player_source_checked(player, result) AFTER these calls return. Delete's
 * unused argument stays unused. IsEnabled is not cleared by original Delete.
 * Read original proximity directly as player->bump->BumpDetector_ProximityLevel;
 * there is no second query clock, collision calculation, or cached result. */
void isle_player_bump_BumpDetector_Update(IsleGameObject*);
IsleGameResult isle_player_bump_BumpDetector_Init(IslePlayerBumpSource*);
IsleGameResult isle_player_bump_BumpDetector_Disable(IslePlayerBumpSource*);
IsleGameResult isle_player_bump_BumpDetector_Enable(IslePlayerBumpSource*);
IsleGameResult isle_player_bump_BumpDetector_Delete(IslePlayerBumpSource*, int32_t);

/* Returning C seams. bind publishes no object/process; it initializes the exact
 * 0/false/90/60/30/5/NULL source defaults and binds player->bump once. */
IsleGameResult isle_player_bump_source_bind(IslePlayerSource*, IslePlayerBumpSource*, void* owner);
void isle_player_bump_source_record(IslePlayerBumpSource*, IsleGameResult);
IsleGameObject* isle_player_bump_source_object(IslePlayerBumpSource*, uint32_t id,
    uint8_t link, uint32_t priority);
void isle_player_bump_source_process(IslePlayerBumpSource*, IsleGameObject*,
    IsleGameSourceCallback, int32_t kind, uint32_t priority);
void isle_player_bump_source_pause(IslePlayerBumpSource*, IsleGameObject*, int paused);

/* C-only checked boundaries: no owning C++ frame may straddle failure/wait.
 * Actor lookup requires EVERY actual LINK_POKEMON payload to be a live bound
 * ActorSource; a foreign helper is an admission error, never silently skipped. */
IslePlayerBumpSource* isle_player_bump_source_current(IsleGameObject*);
IsleActorSource* isle_player_bump_source_actor(IslePlayerBumpSource*, IsleGameObject*);
void isle_player_bump_source_checked(IslePlayerBumpSource*);
void isle_player_bump_source_wait(IslePlayerBumpSource*, int32_t updates);

/* Native returning leaves only. Services keeps the real default command drain;
 * lifecycle observers compose with its hooks. Canonical Object is cleared ONLY
 * by its actual joined observer, never merely when self-deletion is requested. */
IsleGameResult isle_player_bump_native_object(IslePlayerBumpSource*, uint32_t id,
    uint8_t link, uint32_t priority, IsleGameObject**);
IsleGameResult isle_player_bump_native_command(IslePlayerBumpSource*, IsleGameObject* receiver,
    int32_t command, IsleGameObject* sender);
IsleGameResult isle_player_bump_native_delete(IslePlayerBumpSource*, IsleGameObject*);

#ifdef __cplusplus
}
#endif
#endif
