#ifndef ISLE_ACTOR_CONTROL_FIXTURE_H
#define ISLE_ACTOR_CONTROL_FIXTURE_H
#include "actor_source.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct ActorControlFixture {
    IsleActorSource actor;
    int events[128];
    size_t count;
    int failures;
    int after_end;
    int old_resumed;
    int updates;
    int signal_at;
    uint32_t signal;
    uint32_t mask;
    int wait_return_update;
    int choice;
    IsleGameProcess* old;
    IsleGameProcess* old_path;
    IsleActorRandomState* random;
} ActorControlFixture;

void actor_control_replace_self(IsleGameObject*);
void actor_control_new_state(IsleGameObject*);
void actor_control_sleep_state(IsleGameObject*);
void actor_control_cross_replace(IsleGameObject*, void*);
void actor_control_aux_path(IsleGameObject*);
void actor_control_path_old(IsleGameObject*);
void actor_control_replace_path(IsleGameObject*, void*);
void actor_control_null_path(IsleGameObject*, void*);
void actor_control_random(IsleGameObject*);
void actor_control_choice_a(IsleGameObject*);
void actor_control_choice_b(IsleGameObject*);
void actor_control_forbidden_choice(IsleGameObject*);
void actor_control_wait(IsleGameObject*);
void actor_control_update(IsleGameObject*, void*);
void actor_control_creation_failure(IsleGameObject*);
void actor_control_invalid_wait(IsleGameObject*);
int actor_control_joined(const IsleGameProcess*);
int actor_control_payload_empty(const IsleGameObject*);
void* actor_control_update_context(const IsleGameObject*);
void actor_control_foreign_payload(IsleGameObject*, void*);

#ifdef __cplusplus
}
#endif
#endif
