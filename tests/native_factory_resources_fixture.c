#include "native_factory_resources_fixture.h"
#include "engine_internal.h"

static void record(NativeFactoryFixture* fixture, int value) {
    if (fixture->count == ARRAY_COUNT(fixture->events)) ++fixture->failures;
    else fixture->events[fixture->count++] = value;
}

void native_factory_bind(IsleGameObject* object, NativeFactoryFixture* fixture) {
    object->userData = fixture;
}

void native_factory_update(IsleGameObject* object, void* user) {
    record(user, (int) object->id);
}

void native_factory_animation(IsleGameObject* object) {
    record(object->userData, (int) object->id * 10 + 1);
}

void native_factory_show(IsleGameObject* object) {
    NativeFactoryFixture* fixture = object->userData;
    record(fixture, (int) object->id * 10 + 2);
    if (isle_game_wait(fixture->game, 1) != ISLE_GAME_OK) ++fixture->failures;
    ++fixture->resumed;
    record(fixture, (int) object->id * 10 + 3);
}

void native_factory_remove(IsleGameObject* object) {
    record(object->userData, (int) object->id * 10 + 4);
}

void native_factory_nested_prepare(IsleGameObject* object) {
    NativeFactoryFixture* fixture = object->userData;
    IsleSourceFactoryProcess descriptor = {NULL, native_factory_show, ISLE_GAME_STACKFUL, 7};
    fixture->nested_result = isle_source_factory_prepare(fixture->game, 1, &descriptor, 1,
        fixture->nested_resources);
    if (fixture->nested_result == ISLE_GAME_OK)
        fixture->nested_result = isle_source_factory_cancel(fixture->nested_resources);
    record(fixture, 900);
}

void native_factory_illegal_wait(IsleGameObject* object) {
    NativeFactoryFixture* fixture = object->userData;
    ohWait(1);
    ++fixture->resumed;
}

void native_factory_topology(IsleGame* game, NativeFactoryTopology* output) {
    *output = (NativeFactoryTopology){0};
    output->public_identity_credits = game->held_identities;
    for (size_t link = 0; link < ARRAY_COUNT(omGObjListHead); ++link) {
        GObj* previous = NULL;
        for (GObj* object = omGObjListHead[link]; object != NULL; object = object->next) {
            size_t i = output->object_count++;
            if (i >= ARRAY_COUNT(output->objects)) { ++output->overflow; break; }
            if (object->prev != previous || !object->alive || object->game != game)
                ++output->overflow;
            output->objects[i] = object;
            output->ids[i] = object->id;
            output->object_priorities[i] = object->priority;
            output->links[i] = object->link;
            previous = object;
        }
        if (previous != omGObjListTail[link]) ++output->overflow;
    }
    for (size_t priority = ARRAY_COUNT(omGProcessList); priority-- > 0;) {
        GObjProcess* previous = NULL;
        for (GObjProcess* process = omGProcessList[priority]; process != NULL;
             process = process->nextInPriorityList) {
            size_t i = output->process_count++;
            if (i >= ARRAY_COUNT(output->processes)) { ++output->overflow; break; }
            if (process->prevInPriorityList != previous || !process->alive || process->game != game)
                ++output->overflow;
            output->processes[i] = process;
            output->process_objects[i] = process->object;
            output->functions[i] = process->function;
            output->process_priorities[i] = (uint32_t) process->priority;
            output->kinds[i] = process->kind;
            previous = process;
        }
    }
}

size_t native_factory_ready_workers(IsleSourceFactoryResources* resources) {
    size_t ready = 0;
    for (size_t i = 0; i < resources->process_count; ++i) {
        GObjProcess* process = resources->processes[i];
        IsleContinuationState state;
        if (process != NULL && process->continuation != NULL &&
            isle_continuation_state(process->continuation, &state) == ISLE_CONT_OK &&
            state == ISLE_CONT_READY) ++ready;
    }
    return ready;
}

IsleGameResult native_factory_invalid_kind(IsleGame* game, IsleSourceFactoryResources* resources) {
    IsleSourceFactoryProcess descriptor = {NULL, native_factory_show, (IsleGameProcessKind) 99, 7};
    return isle_source_factory_prepare(game, 1, &descriptor, 1, resources);
}
