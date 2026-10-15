#include "engine_fixture.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
void ok(IsleGameResult result) { require(result == ISLE_GAME_OK, "native game operation rejected"); }

class World {
public:
    explicit World(std::size_t size = 65536) : bytes(size) {
        require(reinterpret_cast<std::uintptr_t>(bytes.data()) > UINT32_MAX,
                "test needs actual high-address backing storage");
        require(isle_arena_init(&arena, bytes.data(), bytes.size()) == ISLE_OK, "arena init");
        ok(isle_game_create(&arena, &game));
    }
    ~World() {
        if (game != nullptr && isle_game_destroy(game) != ISLE_GAME_OK) std::abort();
    }
    void reset() {
        ok(isle_game_destroy(game));
        game = nullptr;
        require(isle_arena_reset(&arena) == ISLE_OK, "quiescent arena reset");
        ok(isle_game_create(&arena, &game));
    }
    World(const World&) = delete;
    World& operator=(const World&) = delete;
    std::vector<unsigned char> bytes;
    isle_arena arena{};
    IsleGame* game = nullptr;
};

IsleGameObject* object(World& world, std::uint32_t id, std::uint8_t link = 0,
                       std::uint32_t priority = 0, IsleGameCallback update = nullptr,
                       void* user = nullptr) {
    IsleGameObject* result = nullptr;
    ok(isle_game_add_object(world.game, id, link, priority, update, user, &result));
    return result;
}
IsleGameProcess* process(World& world, IsleGameObject* owner, IsleGameCallback entry,
                         void* user, IsleGameProcessKind kind, std::uint32_t priority) {
    IsleGameProcess* result = nullptr;
    ok(isle_game_add_process(world.game, owner, entry, user, kind, priority, &result));
    return result;
}
IsleGameCounts counts(World& world) {
    IsleGameCounts result{};
    ok(isle_game_counts(world.game, &result));
    return result;
}
void trace(const EngineFixture& fixture, std::initializer_list<int> expected) {
    require(fixture.failures == 0, "compiled-C callback contract failed");
    require(fixture.count == expected.size(), "authoritative dispatch trace extent differs");
    std::size_t index = 0;
    for (int value : expected) require(fixture.events[index++] == value, "authoritative dispatch order differs");
}

void update_and_process_order() {
    World world;
    EngineFixture fixture{}; fixture.game = world.game;
    auto* a = object(world, 10, 0, 20, engine_record_object, &fixture);
    auto* b = object(world, 11, 0, 10, engine_record_object, &fixture);
    auto* c = object(world, 12, 1, UINT32_MAX, engine_record_object, &fixture);
    auto* d = object(world, 13, 0, 20, engine_record_object, &fixture);
    std::array<EngineRecord, 4> records{{{&fixture, 130}, {&fixture, 100}, {&fixture, 120}, {&fixture, 110}}};
    process(world, d, engine_record_process, &records[0], ISLE_GAME_SYNCHRONOUS, 5);
    process(world, a, engine_record_process, &records[1], ISLE_GAME_SYNCHRONOUS, 5);
    process(world, c, engine_record_process, &records[2], ISLE_GAME_SYNCHRONOUS, 11);
    process(world, b, engine_record_process, &records[3], ISLE_GAME_SYNCHRONOUS, 0);
    ok(isle_game_tick(world.game));
    ok(isle_game_tick(world.game));
    trace(fixture, {10, 13, 11, 12, 120, 100, 130, 110,
                    10, 13, 11, 12, 120, 100, 130, 110});
}

void pause_preserves_nested_waits() {
    World world;
    EngineFixture fixture{}; fixture.game = world.game;
    auto* a = object(world, 1);
    auto* p = process(world, a, engine_nested_process, &fixture, ISLE_GAME_STACKFUL, 5);
    ok(isle_game_tick(world.game)); trace(fixture, {200, 201});
    ok(isle_game_tick(world.game)); trace(fixture, {200, 201, 202, 300});
    ok(isle_game_pause_process(world.game, p, 1));
    ok(isle_game_tick(world.game)); ok(isle_game_tick(world.game));
    trace(fixture, {200, 201, 202, 300});
    ok(isle_game_pause_process(world.game, p, 0));
    ok(isle_game_tick(world.game)); trace(fixture, {200, 201, 202, 300});
    ok(isle_game_tick(world.game)); trace(fixture, {200, 201, 202, 300, 301, 203});
    require(fixture.local_result == UINT64_C(0x1234567887654332), "nested C stack lost native-width local");
    require(counts(world).processes == 0, "normal return did not retire C process");
}

void callback_lifecycle_and_same_tick_child() {
    World world;
    EngineFixture fixture{}; fixture.game = world.game;
    auto* a = object(world, 1);
    fixture.victim = process(world, a, engine_victim, &fixture, ISLE_GAME_STACKFUL, 8);
    process(world, a, engine_parent, &fixture, ISLE_GAME_STACKFUL, 5);
    ok(isle_game_tick(world.game)); trace(fixture, {700, 800, 801, 900});
    require(fixture.retired_continued == 0 && fixture.after_finish == 0,
            "retired C callback resumed after cancellation/self-end");
    ok(isle_game_tick(world.game)); trace(fixture, {700, 800, 801, 900, 901});
    require(counts(world).processes == 0, "callback-created child was not owner-dispatched and retired");
}

void synchronous_end_and_update_pause() {
    World world;
    EngineFixture fixture{}; fixture.game = world.game;
    auto* a = object(world, 1, 0, 0, engine_record_object, &fixture);
    EngineRecord survivor{&fixture, 22};
    process(world, a, engine_end_synchronous, &fixture, ISLE_GAME_SYNCHRONOUS, 5);
    process(world, a, engine_record_process, &survivor, ISLE_GAME_SYNCHRONOUS, 4);
    ok(isle_game_tick(world.game)); trace(fixture, {1, 20, 21, 22});
    ok(isle_game_pause_updates(world.game, a, 1));
    ok(isle_game_tick(world.game)); trace(fixture, {1, 20, 21, 22, 22});
    ok(isle_game_pause_updates(world.game, a, 0));
    ok(isle_game_tick(world.game)); trace(fixture, {1, 20, 21, 22, 22, 1, 22});
    require(counts(world).processes == 1, "kind-1 return/end semantics differ");
}

void deferred_object_deletion() {
    for (int mode : {0, 1, 2}) {
        World world;
        EngineFixture fixture{}; fixture.game = world.game; fixture.mode = mode;
        auto* a = object(world, 1, 0, 1, mode == 2 ? engine_delete_self : nullptr,
                         mode == 2 ? &fixture : nullptr);
        auto* b = object(world, 2);
        EngineRecord forbidden{&fixture, 32}, survivor{&fixture, 40};
        if (mode != 2) process(world, a, engine_delete_self, &fixture,
                              mode == 0 ? ISLE_GAME_STACKFUL : ISLE_GAME_SYNCHRONOUS, 5);
        process(world, a, engine_record_process, &forbidden, ISLE_GAME_SYNCHRONOUS, 5);
        process(world, b, engine_record_process, &survivor, ISLE_GAME_SYNCHRONOUS, 5);
        ok(isle_game_tick(world.game)); trace(fixture, {30, 31, 40});
        require(fixture.retired_continued == 0, "self-deleted stackful C object resumed");
        auto live = counts(world);
        require(live.objects == 1 && live.processes == 1, "deferred deletion missed sibling process/object");
        ok(isle_game_tick(world.game)); trace(fixture, {30, 31, 40, 40});
    }
}

void actual_intro_fade_and_cold_reset() {
    World world;
    EngineFixture retired{}; retired.game = world.game;
    auto* a = object(world, 1);
    IsleSprite initial{};
    initial.red = 200; initial.green = 100; initial.blue = 50; initial.alpha = 77;
    IsleGameSprite* sprite = nullptr;
    ok(isle_game_add_sprite(world.game, a, &initial, &sprite));
    IsleSprite* view = nullptr;
    ok(isle_game_sprite_view(world.game, sprite, &view));
    IsleGameProcess* fade = nullptr;
    ok(isle_game_add_intro_fade(world.game, a, sprite, 4, 5, &fade));
    for (std::uint8_t expected : {std::uint8_t{191}, std::uint8_t{127}, std::uint8_t{63}, std::uint8_t{0}, std::uint8_t{0}}) {
        ok(isle_game_tick(world.game));
        require(view->red == expected && view->green == expected && view->blue == expected && view->alpha == 77,
                "actual authoritative C fade or native sprite field layout differs");
    }
    require(counts(world).processes == 0, "completed intro helper retained a process");
    process(world, a, engine_victim, &retired, ISLE_GAME_STACKFUL, 5);
    ok(isle_game_tick(world.game)); trace(retired, {700});
    world.reset();
    EngineFixture fresh{}; fresh.game = world.game;
    auto* b = object(world, 9, 0, 0, engine_record_object, &fresh);
    IsleGameSprite* fresh_sprite = nullptr;
    ok(isle_game_add_sprite(world.game, b, &initial, &fresh_sprite));
    ok(isle_game_sprite_view(world.game, fresh_sprite, &view));
    require(view->red == 200 && view->green == 100 && view->blue == 50, "cold native state inherited mutated tint");
    ok(isle_game_tick(world.game)); trace(fresh, {9});
    require(retired.retired_continued == 0 && retired.count == 1,
            "old world callback accessed storage after quiescent arena reuse");
}

void bounded_pool_reuse_and_failure_fence() {
    {
        World world(4096);
        EngineFixture fixture{}; fixture.game = world.game;
        std::vector<IsleGameObject*> handles;
        IsleGameResult result = ISLE_GAME_OK;
        for (std::uint32_t id = 1; result == ISLE_GAME_OK && id < 512; ++id) {
            IsleGameObject* added = nullptr;
            result = isle_game_add_object(world.game, id, 0, 0, engine_record_object, &fixture, &added);
            if (result == ISLE_GAME_OK) handles.push_back(added);
            else require(added == nullptr, "failed allocation published an object");
        }
        require(result == ISLE_GAME_EXHAUSTED, "bounded arena did not reject exhausted pool");
        ok(isle_game_tick(world.game));
        require(fixture.failures == 0 && fixture.count == handles.size(), "allocation rejection corrupted live object chain");
        for (auto* handle : handles) ok(isle_game_delete_object(world.game, handle));
        fixture.count = 0;
        object(world, 499, 0, 0, engine_record_object, &fixture);
        ok(isle_game_tick(world.game)); trace(fixture, {499});
    }
    {
        World world;
        EngineFixture fixture{}; fixture.game = world.game;
        auto* a = object(world, 1);
        process(world, a, engine_invalid_legacy_wait, &fixture, ISLE_GAME_SYNCHRONOUS, 5);
        require(isle_game_tick(world.game) == ISLE_GAME_EXECUTION_FAILURE, "invalid legacy C wait reported tick success");
        trace(fixture, {60});
        require(fixture.retired_continued == 0, "invalid C callback continued after its failure fence");
        require(isle_game_tick(world.game) == ISLE_GAME_EXECUTION_FAILURE, "failed world resumed callbacks");
        trace(fixture, {60});
    }
}
} // namespace

int main() {
    try {
        update_and_process_order();
        pause_preserves_nested_waits();
        callback_lifecycle_and_same_tick_child();
        synchronous_end_and_update_pause();
        deferred_object_deletion();
        actual_intro_fade_and_cold_reset();
        bounded_pool_reuse_and_failure_fence();
    } catch (const std::exception& error) {
        std::cerr << "native C engine checks failed: " << error.what() << '\n';
        return 1;
    }
    std::cout << "native C engine checks passed; actual dispatcher/fade, not full scene boot\n";
    return 0;
}
