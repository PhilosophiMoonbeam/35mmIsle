#include "actor_interaction_probe.h"
#include "engine_internal.h"

/* There is one admitted native game and one outstanding test entry. The caller
 * retains this stable context until the process has ended, including faults. */
static ActorInteractionProbe* active;

IsleGameResult actor_interaction_probe_start(ActorInteractionProbe* probe,
    IsleActorSource* source, IsleGameProcessKind kind, int mode, int owns_state) {
    IsleGameResult result;
    if (probe == NULL || source == NULL || mode < 0 || mode > 2)
        return ISLE_GAME_INVALID_ARGUMENT;
    probe->game = source->game;
    probe->object = source->object;
    probe->mode = mode;
    probe->entered = probe->returned = probe->failures = 0;
    probe->observed_kind = -1;
    probe->distance = -1.0f;
    active = probe;
    result = isle_game_add_source_process(source->game, source->object,
        actor_interaction_probe_entry, kind, 7, &probe->process);
    if (result == ISLE_GAME_OK && owns_state) source->stateProc = probe->process;
    return result;
}
void actor_interaction_probe_entry(IsleGameObject* object) {
    ActorInteractionProbe* probe = active;
    IsleActorSource* source;
    if (probe == NULL || probe->object != object || omCurrentObject != object ||
        omCurrentProcess == NULL || omCurrentProcess->object != object ||
        omCurrentProcess->function != actor_interaction_probe_entry) {
        isle_native_execution_fault(object->game, "interaction probe has no real OM process context");
    }
    source = isle_actor_source_require(object);
    ++probe->entered;
    probe->observed_kind = omCurrentProcess->kind;
    if (omCurrentProcess->priority != 7 || probe->game != object->game) ++probe->failures;
    if (probe->mode == 0) Pokemon_RunInteractions(object);
    else if (probe->mode == 1) probe->distance = isle_actor_source_distance(object, probe->second);
    else Pokemon_SetState(object, NULL);
    ++probe->returned;
    if (source->stateProc == omCurrentProcess) source->stateProc = NULL;
    omEndProcess(NULL);
}
int actor_interaction_native_update(const IsleGameObject* object) {
    return object != NULL && object->alive && object->fnUpdate == Pokemon_Update && object->update == NULL;
}
int actor_interaction_process_alive(const IsleGameProcess* process) { return process != NULL && process->alive; }
int actor_interaction_process_paused(const IsleGameProcess* process) { return process != NULL && process->alive && process->paused; }
size_t actor_interaction_function_count(const IsleGameObject* object, IsleGameSourceCallback function, int priority) {
    size_t count = 0;
    const GObjProcess* process;
    for (process = object->processListHead; process != NULL; process = process->next)
        if (process->alive && process->function == function && process->priority == priority) ++count;
    return count;
}
size_t actor_interaction_paused_count(const IsleGameObject* object) {
    size_t count = 0;
    const GObjProcess* process;
    for (process = object->processListHead; process != NULL; process = process->next)
        if (process->alive && process->paused) ++count;
    return count;
}
size_t actor_interaction_process_count(const IsleGameObject* object) {
    size_t count = 0;
    const GObjProcess* process;
    for (process = object->processListHead; process != NULL; process = process->next)
        if (process->alive) ++count;
    return count;
}
int actor_interaction_aux_before_state(const IsleGameObject* object,
    IsleGameSourceCallback auxiliary, IsleGameSourceCallback state) {
    const GObjProcess* process;
    int auxiliary_seen = 0;
    for (process = object->processListHead; process != NULL; process = process->next) {
        if (process->alive && process->function == auxiliary && process->priority == 6) auxiliary_seen = 1;
        if (process->alive && process->function == state && process->priority == 7) return auxiliary_seen;
    }
    return 0;
}
int actor_interaction_link_precedes(const IsleGameObject* first,const IsleGameObject* second) {
    const GObj* object;
    if (first == NULL || second == NULL || !first->alive || !second->alive ||
        first->game != second->game || first->link != second->link) return 0;
    for (object = omGObjListHead[first->link]; object != NULL; object = object->next) {
        if (object == first) return 1;
        if (object == second) return 0;
    }
    return 0;
}
int actor_interaction_model_alive(const IsleGameModel* model) { return model != NULL && model->alive; }
int actor_interaction_object_alive(const IsleGameObject* object) { return object != NULL && object->alive; }
