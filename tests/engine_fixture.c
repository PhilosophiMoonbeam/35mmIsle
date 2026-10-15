#include "engine_fixture.h"

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
