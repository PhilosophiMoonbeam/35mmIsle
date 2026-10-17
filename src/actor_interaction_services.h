#ifndef ISLE_ACTOR_INTERACTION_SERVICES_H
#define ISLE_ACTOR_INTERACTION_SERVICES_H

#include "actor_source.h"
#include "item_source.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Returning, nonwaiting service leaves for the privately compiled GAME C
 * interpreters. No leaf invokes a behavior, faults, waits or ends a process.
 * Pointer outputs clear on every error; value outputs publish only on success.
 * Caller retains the canonical Source, bound context and its dependencies,
 * graph rows/count, native identities/models and published focus identities.
 * All mutation and destruction require actual native/command and independent
 * graphics/preparation/readback exclusion. A native join is not a GPU fence. */
/* NonNULL graphs admit every executable row, even later/ineligible rows, plus
 * declared callback/graph/weighted dependencies and the actual compiled source
 * resource/service closure. Caller-local rows are rechecked on each call; NULL
 * graphs return NULL successfully without requiring a bound context. */
IsleGameResult isle_actor_interaction_rows(IsleActorSource*, const IsleActorInteraction**);
/* auxiliary=1 requires real RT/RTS AUX; auxiliary=0 reads root-local position
 * even when AUX is enabled. Neither path evaluates or updates a pose cache. */
IsleGameResult isle_actor_interaction_position(IsleActorSource*, IsleGameObject* target,
    int auxiliary, IsleGameVec3*);
/* Uses the actual shared Items ascending twenty-entry cursor. No private
 * iterator, item-family filter or nearest-item implementation is retained. */
IsleGameResult isle_actor_interaction_item_iterator(IsleActorSource*);
IsleGameResult isle_actor_interaction_item_next(IsleActorSource*, IsleGameObject**);
IsleGameResult isle_actor_interaction_item_source(IsleActorSource*, IsleGameObject*, IsleItemSource**);
IsleGameResult isle_actor_interaction_flute(IsleActorSource*, int32_t*);
/* Pure spatial math stays live-only. Only a Services-proven registered, joined,
 * identity-held second sender projects to the source's absent-model FLT_MAX. */
IsleGameResult isle_actor_interaction_distance(IsleActorSource*, IsleGameObject* second, float*);
IsleGameResult isle_actor_interaction_neighbors(IsleActorSource*, IsleGameVec3*, float*);
IsleGameResult isle_actor_interaction_player(IsleActorSource*, IsleGameObject**);
IsleGameResult isle_actor_interaction_focus(IsleActorSource*, uint32_t* timer, IsleGameObject** current);
IsleGameResult isle_actor_interaction_direction(IsleActorSource*, int32_t*);

#ifdef __cplusplus
}
#endif
#endif
