#include "engine_fixture.h"
#include "../src/engine_internal.h"
#include "../src/menu_scene_bridge.h"

/* The selected game ABI uses this wait entry. It must reject synchronous use
 * without returning into the invalid callback or crossing a C++ exception. */
void ohWait(int32_t updates);

static void record(EngineFixture* fixture, int value) {
    if (fixture->count >= sizeof(fixture->events) / sizeof(fixture->events[0])) {
        ++fixture->failures;
        return;
    }
    fixture->events[fixture->count++] = value;
}

static void require_ok(EngineFixture* fixture, IsleGameResult result) {
    if (result != ISLE_GAME_OK) ++fixture->failures;
}

void engine_record_object(IsleGameObject* object, void* user) {
    EngineFixture* fixture = user;
    uint32_t id = 0;
    (void) object;
    if (isle_game_object_id(fixture->game, NULL, &id) != ISLE_GAME_OK) {
        ++fixture->failures;
        return;
    }
    record(fixture, (int) id);
}

void engine_record_process(IsleGameObject* object, void* user) {
    EngineRecord* entry = user;
    (void) object;
    record(entry->fixture, entry->value);
}

static void nested_wait(EngineFixture* fixture, uint64_t value) {
    uint64_t nested = value + UINT64_C(17);
    record(fixture, 300);
    require_ok(fixture, isle_game_wait(fixture->game, 2));
    fixture->local_result = nested;
    record(fixture, 301);
}

void engine_nested_process(IsleGameObject* object, void* user) {
    EngineFixture* fixture = user;
    uint64_t local = UINT64_C(0x1234567887654321);
    (void) object;
    record(fixture, 200);
    require_ok(fixture, isle_game_wait(fixture->game, 0));
    record(fixture, 201);
    require_ok(fixture, isle_game_wait(fixture->game, 1));
    record(fixture, 202);
    nested_wait(fixture, local);
    record(fixture, 203);
}

void engine_victim(IsleGameObject* object, void* user) {
    EngineFixture* fixture = user;
    (void) object;
    record(fixture, 700);
    require_ok(fixture, isle_game_wait(fixture->game, 100));
    ++fixture->retired_continued;
}

static void child_entry(IsleGameObject* object, void* user) {
    EngineFixture* fixture = user;
    (void) object;
    if (fixture->shared_value != 99) ++fixture->failures;
    record(fixture, 900);
    require_ok(fixture, isle_game_wait(fixture->game, 1));
    record(fixture, 901);
}

void engine_parent(IsleGameObject* object, void* user) {
    EngineFixture* fixture = user;
    record(fixture, 800);
    require_ok(fixture, isle_game_end_process(fixture->game, fixture->victim));
    fixture->victim = NULL;
    fixture->shared_value = 99;
    record(fixture, 801);
    require_ok(fixture, isle_game_add_process(fixture->game, object, child_entry, fixture,
                                            ISLE_GAME_STACKFUL, 5, &fixture->child));
    require_ok(fixture, isle_game_end_process(fixture->game, NULL));
    ++fixture->after_finish;
}

void engine_end_synchronous(IsleGameObject* object, void* user) {
    EngineFixture* fixture = user;
    (void) object;
    record(fixture, 20);
    require_ok(fixture, isle_game_end_process(fixture->game, NULL));
    record(fixture, 21);
}

void engine_delete_self(IsleGameObject* object, void* user) {
    EngineFixture* fixture = user;
    (void) object;
    record(fixture, 30);
    require_ok(fixture, isle_game_delete_object(fixture->game, NULL));
    record(fixture, 31);
    if (fixture->mode == 0) {
        require_ok(fixture, isle_game_wait(fixture->game, 1));
        ++fixture->retired_continued;
    }
}

void engine_invalid_legacy_wait(IsleGameObject* object, void* user) {
    EngineFixture* fixture = user;
    (void) object;
    record(fixture, 60);
    ohWait(1);
    ++fixture->retired_continued;
}

IsleGameResult engine_attach_model_with_invalid_kind(IsleGame* game, IsleGameObject* owner,
                                                     const IsleGameModelNodeInit nodes[2],
                                                     IsleGameModel** model) {
    IsleGameModelNodeInit invalid[2] = {nodes[0], nodes[1]};
    /* An integer outside the named enum values is a valid C ABI input.
     * Construct it here: the equivalent C++ out-of-range enum cast is not. */
    invalid[1].matrix_kind = (IsleGameModelMatrixKind) 99;
    return isle_game_attach_model(game, owner, invalid, 2, model);
}

IsleGameResult engine_add_camera_matrix_with_invalid_kind(IsleGame* game, IsleGameCamera* camera) {
    /* Exercise an unknown integer at the C ABI, not an out-of-range C++ enum. */
    return isle_game_camera_add_matrix(game, camera, (IsleGameCameraMatrixKind) 99);
}

void engine_bind_actor_payload(IsleGameObject* object, int* payload) {
    object->userData = payload;
}

void engine_actor_context_update(IsleGameObject* object, void* user) {
    ++*(int*) user;
    int* payload = object->userData;
    *payload += 10;
    if (*payload == 120) omDeleteGObj(object);
}

IsleGameResult engine_replace_actor_update(IsleGame* game, IsleGameObject* object, int* context) {
    return isle_menu_set_update(game, object, engine_actor_context_update, context);
}

void engine_bind_source_payload(IsleGameObject* object, EngineFixture* fixture) {
    object->userData = fixture;
}

void engine_source_a(IsleGameObject* object) {
    EngineFixture* fixture = object->userData;
    IsleGameSourceCallback function = NULL;
    uint64_t local = (uint64_t) fixture->shared_value + UINT64_C(17);
    require_ok(fixture, isle_game_process_source_function(fixture->game, NULL, &function));
    if (function != engine_source_a) ++fixture->failures;
    record(fixture, 410);
    ohWait(fixture->mode == 1 ? 100 : 1);
    if (fixture->mode == 1) {
        ++fixture->retired_continued;
        return;
    }
    fixture->local_result = local;
    record(fixture, 411);
    require_ok(fixture, isle_game_add_source_process(fixture->game, NULL, engine_source_b,
        ISLE_GAME_STACKFUL, 5, &fixture->child));
    require_ok(fixture, isle_game_end_source_processes(fixture->game, NULL, engine_source_a));
    ++fixture->after_finish;
}

void engine_source_b(IsleGameObject* object) {
    EngineFixture* fixture = object->userData;
    IsleGameSourceCallback function = NULL;
    require_ok(fixture, isle_game_process_source_function(fixture->game, NULL, &function));
    if (function != engine_source_b) ++fixture->failures;
    record(fixture, 420);
    if (fixture->mode == 1) return;
    if (fixture->mode == 3) {
        require_ok(fixture, isle_game_end_source_processes(fixture->game, NULL, engine_source_b));
        record(fixture, 422);
        return;
    }
    ohWait(1);
    record(fixture, 421);
    require_ok(fixture, isle_game_delete_object(fixture->game, NULL));
    record(fixture, 422);
    ohWait(1);
    ++fixture->retired_continued;
}

void engine_source_retired(IsleGameObject* object, void* user) {
    EngineFixture* fixture = user;
    IsleGameCounts counts = {0};
    if (object->userData != fixture) ++fixture->failures;
    require_ok(fixture, isle_game_counts(fixture->game, &counts));
    if (counts.processes != 0) ++fixture->failures;
    ++fixture->retire_calls;
    record(fixture, 450);
}
