#include "actor_control_fixture.h"
#include "engine_internal.h"

static ActorControlFixture* fixture(IsleGameObject* object) {
    return isle_actor_source_require(object)->owner;
}
static void record(ActorControlFixture* f, int value) {
    if (f->count < ARRAY_COUNT(f->events)) f->events[f->count++] = value;
    else ++f->failures;
}
static void expect(ActorControlFixture* f, int condition) {
    if (!condition) ++f->failures;
}
static void current_process(ActorControlFixture* f, int priority) {
    expect(f, omCurrentProcess != NULL && omCurrentProcess->kind == 0 &&
        omCurrentProcess->priority == priority && omCurrentProcess->object == f->actor.object);
}
int actor_control_joined(const IsleGameProcess* process) {
    return process != NULL && !process->alive && process->continuation == NULL;
}
int actor_control_payload_empty(const IsleGameObject* object) { return object->userData == NULL; }
void* actor_control_update_context(const IsleGameObject* object) { return object->updateUser; }
void actor_control_foreign_payload(IsleGameObject* object, void* payload) { object->userData = payload; }

void actor_control_replace_self(IsleGameObject* object) {
    ActorControlFixture* f = fixture(object);
    current_process(f, 7);
    record(f, 10);
    f->old = f->actor.stateProc;
    Pokemon_SetState(object, actor_control_new_state);
    ++f->after_end;
}
void actor_control_new_state(IsleGameObject* object) {
    ActorControlFixture* f = fixture(object);
    current_process(f, 7);
    expect(f, f->actor.stateProc == omCurrentProcess && f->actor.transitionGraph == NULL);
    expect(f, actor_control_joined(f->old));
    record(f, 11);
    Pokemon_SetState(object, NULL);
    ++f->after_end;
}
void actor_control_sleep_state(IsleGameObject* object) {
    ActorControlFixture* f = fixture(object);
    record(f, 20);
    ohWait(50);
    ++f->old_resumed;
}
void actor_control_cross_replace(IsleGameObject* object, void* user) {
    ActorControlFixture* f = user;
    (void)object;
    record(f, 21);
    f->old = f->actor.stateProc;
    Pokemon_SetState(f->actor.object, actor_control_new_state);
    expect(f, actor_control_joined(f->old));
    expect(f, f->actor.stateProc != f->old && f->actor.transitionGraph == NULL);
    record(f, 22);
    omEndProcess(NULL);
}

static void aux(IsleGameObject* object) {
    ActorControlFixture* f = fixture(object);
    current_process(f, 6);
    expect(f, !(f->actor.processFlags & POKEMON_PROCESS_FLAG_AUX_ENDED));
    record(f, 32);
    Pokemon_StopAuxProc(object);
    ++f->after_end;
}
void actor_control_path_old(IsleGameObject* object) {
    ActorControlFixture* f = fixture(object);
    current_process(f, 5);
    expect(f, f->actor.pathProc == omCurrentProcess);
    record(f, 33);
    ohWait(50);
    ++f->old_resumed;
}
void actor_control_aux_path(IsleGameObject* object) {
    ActorControlFixture* f = fixture(object);
    current_process(f, 7);
    record(f, 30);
    Pokemon_StartAuxProc(object, aux);
    Pokemon_StartPathProc(object, actor_control_path_old);
    expect(f, f->actor.processFlags == UINT32_C(0x100));
    record(f, 31);
    ohWait(1);
    expect(f, f->actor.processFlags == (UINT32_C(0x100) | POKEMON_PROCESS_FLAG_AUX_ENDED));
    record(f, 34);
    f->old_path = f->actor.pathProc;
    f->actor.processFlags |= POKEMON_PROCESS_FLAG_PATH_ENDED | POKEMON_PROCESS_TARGET_REACHED |
        POKEMON_PROCESS_FLAG_MOVEMENT_PAUSED;
    Pokemon_StartPathProc(object, NULL);
    expect(f, f->actor.pathProc == NULL && actor_control_joined(f->old_path));
    expect(f, f->actor.processFlags == UINT32_C(0x13a));
    Pokemon_SetState(object, NULL);
    ++f->after_end;
}
static void path_new(IsleGameObject* object) {
    ActorControlFixture* f = fixture(object);
    current_process(f, 5);
    expect(f, f->actor.pathProc == omCurrentProcess && actor_control_joined(f->old_path));
    record(f, 35);
    ohWait(50);
    ++f->old_resumed;
}
void actor_control_replace_path(IsleGameObject* object, void* user) {
    ActorControlFixture* f = user;
    (void)object;
    f->old_path = f->actor.pathProc;
    Pokemon_StartPathProc(f->actor.object, path_new);
    expect(f, actor_control_joined(f->old_path));
    expect(f, f->actor.processFlags == UINT32_C(0x108));
    record(f, 36);
    omEndProcess(NULL);
}
void actor_control_null_path(IsleGameObject* object, void* user) {
    ActorControlFixture* f = user;
    (void)object;
    f->old_path = f->actor.pathProc;
    Pokemon_StartPathProc(f->actor.object, NULL);
    expect(f, f->actor.pathProc == NULL && actor_control_joined(f->old_path));
    record(f, 37);
    omEndProcess(NULL);
}
void actor_control_random(IsleGameObject* object) {
    ActorControlFixture* f = fixture(object);
    f->old = f->actor.stateProc;
    Pokemon_SetStateRandom(object, f->random);
    ++f->after_end;
}
static void chosen(IsleGameObject* object, int choice) {
    ActorControlFixture* f = fixture(object);
    current_process(f, 7);
    expect(f, actor_control_joined(f->old) && f->actor.stateProc == omCurrentProcess);
    expect(f, f->actor.transitionGraph == NULL);
    f->choice = choice;
    record(f, 40 + choice);
    Pokemon_SetState(object, NULL);
    ++f->after_end;
}
void actor_control_choice_a(IsleGameObject* object) { chosen(object, 0); }
void actor_control_choice_b(IsleGameObject* object) { chosen(object, 1); }
void actor_control_forbidden_choice(IsleGameObject* object) { ++fixture(object)->failures; }

void actor_control_wait(IsleGameObject* object) {
    ActorControlFixture* f = fixture(object);
    record(f, 50);
    Pokemon_WaitForFlagNoInteraction(object, f->mask);
    f->wait_return_update = f->updates;
    record(f, 51);
    Pokemon_SetState(object, NULL);
    ++f->after_end;
}
void actor_control_update(IsleGameObject* object, void* user) {
    ActorControlFixture* f = user;
    IsleActorSource* actor = NULL;
    expect(f, isle_actor_source_get(f->actor.game, object, &actor) == ISLE_GAME_OK && actor == &f->actor);
    ++f->updates;
    if (f->updates == f->signal_at) actor->processFlags |= f->signal;
}
void actor_control_creation_failure(IsleGameObject* object) {
    ActorControlFixture* f = fixture(object);
    record(f, 60);
    Pokemon_SetState(object, actor_control_new_state);
    ++f->after_end;
}
void actor_control_invalid_wait(IsleGameObject* object) {
    ActorControlFixture* f = fixture(object);
    record(f, 61);
    Pokemon_WaitForFlagNoInteraction(object, f->mask);
    ++f->after_end;
}
