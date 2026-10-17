#ifndef ISLE_TEST_ACTOR_INTERACTION_PROBE_H
#define ISLE_TEST_ACTOR_INTERACTION_PROBE_H
#include "actor_owner_source.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Test-owned C entry only. It never implements a selector or a behavior and is
 * deliberately absent from BehaviorBindings' callable registry. */
typedef struct ActorInteractionProbe {
    IsleGame* game;
    IsleGameObject* object;
    IsleGameObject* second;
    IsleGameProcess* process;
    int mode; /* 0: RunInteractions; 1: source distance; 2: SetState(NULL). */
    int entered, returned, failures, observed_kind;
    float distance;
} ActorInteractionProbe;
IsleGameResult actor_interaction_probe_start(ActorInteractionProbe*, IsleActorSource*,
    IsleGameProcessKind, int mode, int owns_state);
void actor_interaction_probe_entry(IsleGameObject*);
/* Read-only observations of native records, never fabricated callback effects. */
int actor_interaction_native_update(const IsleGameObject*);
int actor_interaction_process_alive(const IsleGameProcess*);
int actor_interaction_process_paused(const IsleGameProcess*);
size_t actor_interaction_function_count(const IsleGameObject*, IsleGameSourceCallback, int priority);
size_t actor_interaction_paused_count(const IsleGameObject*);
size_t actor_interaction_process_count(const IsleGameObject*);
int actor_interaction_aux_before_state(const IsleGameObject*, IsleGameSourceCallback, IsleGameSourceCallback);
int actor_interaction_link_precedes(const IsleGameObject*, const IsleGameObject*);
int actor_interaction_model_alive(const IsleGameModel*);
int actor_interaction_object_alive(const IsleGameObject*);
#ifdef __cplusplus
}
#endif
#endif
