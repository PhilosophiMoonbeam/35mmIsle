#include "engine_fixture.h"
#include "contextual_model_math.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cfenv>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <stdexcept>
#include <limits>
#include <thread>
#include <utility>
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

void actor_payload_and_update_context_are_independent() {
    World world;
    int initial_context=0,replacement_context=0,payload=100;
    auto* owner=object(world,17,0,0,engine_actor_context_update,&initial_context);
    engine_bind_actor_payload(owner,&payload);
    ok(isle_game_tick(world.game));
    require(initial_context==1&&payload==110,
            "GAME actor payload overwrote the native update callback context");
    ok(engine_replace_actor_update(world.game,owner,&replacement_context));
    ok(isle_game_tick(world.game));
    require(initial_context==1&&replacement_context==1&&payload==120&&counts(world).objects==0,
            "update replacement must preserve actor payload and source self-deletion");
    int reused_context=0,reused_payload=200;
    auto* reused=object(world,18,0,0,engine_actor_context_update,&reused_context);
    engine_bind_actor_payload(reused,&reused_payload);
    ok(isle_game_tick(world.game));
    require(reused_context==1&&reused_payload==210&&payload==120&&replacement_context==1,
            "retired actor payload or context leaked into the next native object");
    ok(isle_game_delete_object(world.game,reused));
    ok(isle_game_tick(world.game));
    require(counts(world).objects==0&&reused_context==1&&reused_payload==210,
            "explicit actor retirement dispatched stale context or payload");
}

void source_function_identity_and_reuse() {
    World world;
    EngineFixture fixture{}; fixture.game = world.game; fixture.mode = 1;
    EngineFixture other{}; other.game = world.game; other.mode = 1;
    auto* owner = object(world, 1);
    auto* other_owner = object(world, 2);
    engine_bind_source_payload(owner, &fixture);
    engine_bind_source_payload(other_owner, &other);
    IsleGameProcess *a = nullptr, *duplicate = nullptr, *b = nullptr, *other_a = nullptr;
    ok(isle_game_add_source_process(world.game, owner, engine_source_a, ISLE_GAME_STACKFUL, 8, &a));
    ok(isle_game_add_source_process(world.game, owner, engine_source_a, ISLE_GAME_STACKFUL, 8, &duplicate));
    ok(isle_game_add_source_process(world.game, owner, engine_source_b, ISLE_GAME_SYNCHRONOUS, 5, &b));
    EngineRecord ordinary{&fixture, 430}, recycled{&fixture, 431};
    auto* native = process(world, owner, engine_record_process, &ordinary, ISLE_GAME_SYNCHRONOUS, 5);
    ok(isle_game_add_source_process(world.game, other_owner, engine_source_a, ISLE_GAME_STACKFUL, 8, &other_a));
    IsleGameSourceCallback function = nullptr;
    for (auto* entry : {a, duplicate, other_a}) {
        ok(isle_game_process_source_function(world.game, entry, &function));
        require(function == engine_source_a, "source identity must be the original callable function");
    }
    ok(isle_game_process_source_function(world.game, b, &function));
    require(function == engine_source_b, "distinct source entries acquired the same adapter identity");
    ok(isle_game_process_source_function(world.game, native, &function));
    require(function == nullptr, "ordinary native callback acquired a source identity");
    ok(isle_game_tick(world.game));
    trace(fixture, {410, 410, 420, 430}); trace(other, {410});
    ok(isle_game_end_source_processes(world.game, owner, engine_source_a));
    require(counts(world).processes == 3, "identity termination missed duplicates or terminated unrelated entries");
    ok(isle_game_tick(world.game));
    trace(fixture, {410, 410, 420, 430, 420, 430}); trace(other, {410});
    require(fixture.retired_continued == 0, "joined source process resumed after exact-identity cancellation");
    ok(isle_game_end_source_processes(world.game, owner, engine_source_b));
    ok(isle_game_end_source_processes(world.game, other_owner, engine_source_a));
    require(counts(world).processes == 1 && other.retired_continued == 0,
            "identity termination crossed object ownership or retained sleeping source entries");
    const auto cursor = world.arena.cursor;
    auto* reused_native = process(world, owner, engine_record_process, &recycled, ISLE_GAME_SYNCHRONOUS, 5);
    ok(isle_game_process_source_function(world.game, reused_native, &function));
    require(function == nullptr, "recycled source slot retained identity in a native callback");
    ok(isle_game_add_source_process(world.game, owner, engine_source_b, ISLE_GAME_SYNCHRONOUS, 5, &b));
    ok(isle_game_process_source_function(world.game, b, &function));
    require(function == engine_source_b && world.arena.cursor == cursor,
            "source replacement failed to reuse joined arena storage with its new typed identity");
    ok(isle_game_end_source_processes(world.game, owner, engine_source_a));
    require(counts(world).processes == 3, "stale source identity terminated a replacement");
    fixture.count = 0;
    ok(isle_game_tick(world.game)); trace(fixture, {430, 431, 420});
    ok(isle_game_end_source_processes(world.game, owner, engine_source_b));
    ok(isle_game_end_process(world.game, native));
    ok(isle_game_end_process(world.game, reused_native));
    fixture.mode = 3; fixture.count = 0;
    ok(isle_game_add_source_process(world.game, owner, engine_source_b, ISLE_GAME_SYNCHRONOUS, 5, &b));
    ok(isle_game_add_source_process(world.game, owner, engine_source_b, ISLE_GAME_SYNCHRONOUS, 5, &duplicate));
    ok(isle_game_tick(world.game)); trace(fixture, {420, 422});
    require(counts(world).processes == 0, "synchronous identity self-end did not return and terminate later matches");
}

void source_function_pause_matches_actual_callables() {
    EngineFixture fixture{}, other{};
    World world;
    fixture.game = other.game = world.game;
    fixture.mode = 1;
    auto* owner = object(world, 1);
    auto* other_owner = object(world, 2);
    engine_bind_source_payload(owner, &fixture);
    engine_bind_source_payload(other_owner, &other);
    IsleGameProcess *first = nullptr, *duplicate = nullptr, *stackful = nullptr, *different = nullptr;
    IsleGameProcess* other_process = nullptr;
    ok(isle_game_add_source_process(world.game, owner, engine_source_pause_pulse,
                                   ISLE_GAME_SYNCHRONOUS, 5, &first));
    ok(isle_game_add_source_process(world.game, owner, engine_source_b,
                                   ISLE_GAME_SYNCHRONOUS, 3, &different));
    process(world, owner, engine_native_source_pause_pulse, &fixture, ISLE_GAME_SYNCHRONOUS, 5);
    ok(isle_game_add_source_process(world.game, owner, engine_source_pause_pulse,
                                   ISLE_GAME_SYNCHRONOUS, 4, &duplicate));
    ok(isle_game_add_source_process(world.game, owner, engine_source_pause_pulse,
                                   ISLE_GAME_STACKFUL, 6, &stackful));
    ok(isle_game_add_source_process(world.game, other_owner, engine_source_pause_pulse,
                                   ISLE_GAME_SYNCHRONOUS, 5, &other_process));
    const auto used = world.arena.cursor;
    ok(isle_game_pause_source_processes(world.game, owner, engine_source_pause_pulse, 1));
    ok(isle_game_pause_source_processes(world.game, owner, engine_source_pause_pulse, -3));
    ok(isle_game_tick(world.game));
    trace(fixture, {461, 460, 420}); trace(other, {460});
    require(counts(world).processes == 6 && engine_process_alive(stackful),
            "pausing matching mixed-kind entries ended an unstarted continuation");
    fixture.count = other.count = 0;
    ok(isle_game_pause_source_processes(world.game, owner, engine_source_pause_wait, 1));
    ok(isle_game_pause_source_processes(world.game, owner, engine_source_pause_pulse, 0));
    ok(isle_game_pause_source_processes(world.game, owner, engine_source_pause_pulse, 0));
    ok(isle_game_tick(world.game));
    trace(fixture, {460, 460, 461, 460, 460, 420}); trace(other, {460});
    require(counts(world).processes == 5,
            "resuming exact source identity lost source kind or normal-return retirement");
    fixture.count = other.count = 0;
    ok(isle_game_pause_source_processes(world.game, owner, engine_source_pause_pulse, 1));
    ok(isle_game_tick(world.game));
    trace(fixture, {461, 460, 420}); trace(other, {460});
    require(world.arena.cursor == used,
            "pause/resume allocated storage instead of preserving actual native process lists");
}

void source_function_pause_preserves_real_waits() {
    EngineFixture fixture{}, other{};
    World world;
    fixture.game = other.game = world.game;
    fixture.shared_value = 37;
    other.shared_value = 5;
    auto* owner = object(world, 1);
    auto* other_owner = object(world, 2);
    engine_bind_source_payload(owner, &fixture);
    engine_bind_source_payload(other_owner, &other);
    IsleGameProcess *first = nullptr, *duplicate = nullptr, *pulse = nullptr, *other_wait = nullptr;
    ok(isle_game_add_source_process(world.game, owner, engine_source_pause_wait,
                                   ISLE_GAME_STACKFUL, 8, &first));
    ok(isle_game_add_source_process(world.game, owner, engine_source_pause_wait,
                                   ISLE_GAME_STACKFUL, 8, &duplicate));
    ok(isle_game_add_source_process(world.game, owner, engine_source_pause_pulse,
                                   ISLE_GAME_SYNCHRONOUS, 1, &pulse));
    ok(isle_game_add_source_process(world.game, other_owner, engine_source_pause_wait,
                                   ISLE_GAME_STACKFUL, 8, &other_wait));
    ok(isle_game_tick(world.game));
    trace(fixture, {470, 470, 460}); trace(other, {470});
    ok(isle_game_pause_source_processes(world.game, owner, engine_source_pause_wait, 1));
    fixture.shared_value = 99;
    for (int tick = 0; tick < 3; ++tick) {
        fixture.count = 0;
        ok(isle_game_tick(world.game));
        trace(fixture, {460});
        require(engine_process_waiting(first) && engine_process_waiting(duplicate),
                "paused real source continuations consumed their pending waits");
    }
    trace(other, {470, 471, 472});
    require(other.after_finish == 1 && other.local_result == 22 && fixture.after_finish == 0,
            "function pause crossed object ownership or lost the other source continuation");
    fixture.count = 0;
    ok(isle_game_pause_source_processes(world.game, owner, engine_source_pause_wait, 0));
    ok(isle_game_tick(world.game)); trace(fixture, {460});
    fixture.count = 0;
    ok(isle_game_tick(world.game)); trace(fixture, {471, 471, 460});
    require(fixture.local_result == 54 && fixture.after_finish == 0,
            "resumed source waits restarted callbacks or lost saved C locals");
    fixture.count = 0;
    ok(isle_game_tick(world.game)); trace(fixture, {472, 472, 460});
    require(fixture.after_finish == 2 && counts(world).processes == 1,
            "resumed duplicate source functions did not finish their actual second waits");
}

void source_function_pause_current_callback_rules() {
    {
        EngineFixture fixture{}, other{};
        World world;
        fixture.game = other.game = world.game;
        fixture.shared_value = 37;
        fixture.mode = 1;
        auto* owner = object(world, 1, 0, 0, engine_source_pause_update, &fixture);
        auto* other_owner = object(world, 2);
        engine_bind_source_payload(owner, &fixture);
        engine_bind_source_payload(other_owner, &other);
        IsleGameProcess *self = nullptr, *pulse = nullptr, *duplicate = nullptr, *other_pulse = nullptr;
        ok(isle_game_add_source_process(world.game, owner, engine_source_pause_self,
                                       ISLE_GAME_STACKFUL, 5, &self));
        ok(isle_game_add_source_process(world.game, owner, engine_source_pause_pulse,
                                       ISLE_GAME_SYNCHRONOUS, 3, &pulse));
        ok(isle_game_add_source_process(world.game, owner, engine_source_pause_pulse,
                                       ISLE_GAME_SYNCHRONOUS, 3, &duplicate));
        ok(isle_game_add_source_process(world.game, other_owner, engine_source_pause_pulse,
                                       ISLE_GAME_SYNCHRONOUS, 3, &other_pulse));
        ok(isle_game_tick(world.game)); trace(fixture, {490, 480, 481});
        fixture.count = 0;
        ok(isle_game_tick(world.game)); ok(isle_game_tick(world.game));
        trace(fixture, {}); trace(other, {460, 460, 460});
        require(engine_process_waiting(self) && fixture.after_finish == 0,
                "source self-pause suspended inside the API, ended, or consumed waits");
        fixture.mode = 2;
        ok(isle_game_tick(world.game)); trace(fixture, {491, 460, 460});
        fixture.count = 0;
        fixture.shared_value = 99;
        ok(isle_game_pause_source_processes(world.game, owner, engine_source_pause_self, 0));
        ok(isle_game_tick(world.game)); trace(fixture, {460, 460});
        fixture.count = 0;
        ok(isle_game_tick(world.game)); trace(fixture, {482, 460, 460});
        require(fixture.after_finish == 1 && fixture.local_result == 60,
                "self-paused source callback failed its real resume or retained C-local contract");
    }
    {
        EngineFixture fixture{};
        World world;
        fixture.game = world.game;
        auto* owner = object(world, 1);
        engine_bind_source_payload(owner, &fixture);
        IsleGameProcess *first = nullptr, *duplicate = nullptr, *pulse = nullptr;
        ok(isle_game_add_source_process(world.game, owner, engine_source_pause_self_synchronous,
                                       ISLE_GAME_SYNCHRONOUS, 5, &first));
        ok(isle_game_add_source_process(world.game, owner, engine_source_pause_self_synchronous,
                                       ISLE_GAME_SYNCHRONOUS, 5, &duplicate));
        ok(isle_game_add_source_process(world.game, owner, engine_source_pause_pulse,
                                       ISLE_GAME_SYNCHRONOUS, 3, &pulse));
        ok(isle_game_tick(world.game)); trace(fixture, {483, 484, 460});
        fixture.count = 0;
        ok(isle_game_tick(world.game)); trace(fixture, {460});
        fixture.count = 0;
        ok(isle_game_pause_source_processes(world.game, owner, engine_source_pause_self_synchronous, 0));
        ok(isle_game_tick(world.game)); trace(fixture, {483, 484, 483, 484, 460});
        require(fixture.shared_value == 3 && counts(world).processes == 3,
                "synchronous current-source pause returned early, missed later matches, or ended a process");
    }
}

void source_function_pause_rejects_invalid_owners_unchanged() {
    World retired_world;
    auto* foreign_object = object(retired_world, 99);
    auto* retired_game = retired_world.game;
    ok(isle_game_destroy(retired_world.game));
    retired_world.game = nullptr;
    EngineFixture fixture{};
    World world;
    fixture.game = world.game;
    fixture.mode = 1;
    auto* owner = object(world, 1);
    engine_bind_source_payload(owner, &fixture);
    IsleGameProcess *pulse = nullptr, *different = nullptr;
    ok(isle_game_add_source_process(world.game, owner, engine_source_pause_pulse,
                                   ISLE_GAME_SYNCHRONOUS, 5, &pulse));
    process(world, owner, engine_native_source_pause_pulse, &fixture, ISLE_GAME_SYNCHRONOUS, 5);
    ok(isle_game_add_source_process(world.game, owner, engine_source_b,
                                   ISLE_GAME_SYNCHRONOUS, 3, &different));
    auto* stale = object(world, 2);
    ok(isle_game_delete_object(world.game, stale));
    ok(isle_game_pause_source_processes(world.game, owner, engine_source_pause_pulse, 1));
    const auto used = world.arena.cursor;
    for (int paused : {0, 1}) {
        require(isle_game_pause_source_processes(nullptr, owner, engine_source_pause_pulse, paused) ==
                    ISLE_GAME_INVALID_ARGUMENT &&
                isle_game_pause_source_processes(retired_game, owner, engine_source_pause_pulse, paused) ==
                    ISLE_GAME_INVALID_STATE &&
                isle_game_pause_source_processes(world.game, nullptr, engine_source_pause_pulse, paused) ==
                    ISLE_GAME_INVALID_ARGUMENT &&
                isle_game_pause_source_processes(world.game, owner, nullptr, paused) ==
                    ISLE_GAME_INVALID_ARGUMENT &&
                isle_game_pause_source_processes(world.game, stale, engine_source_pause_pulse, paused) ==
                    ISLE_GAME_INVALID_ARGUMENT &&
                isle_game_pause_source_processes(world.game, foreign_object, engine_source_pause_pulse, paused) ==
                    ISLE_GAME_INVALID_ARGUMENT,
                "source function pause admitted invalid game, absent current object, NULL identity or dead/foreign object");
    }
    bool wrong_owner = false;
    std::thread foreign([&] {
        wrong_owner = isle_game_pause_source_processes(world.game, owner, engine_source_pause_pulse, 0) ==
                          ISLE_GAME_WRONG_OWNER &&
                      isle_game_pause_source_processes(world.game, nullptr, nullptr, 1) ==
                          ISLE_GAME_WRONG_OWNER;
    });
    foreign.join();
    require(wrong_owner, "source function pause bypassed native gameplay owner validation");
    ok(isle_game_tick(world.game)); trace(fixture, {461, 460, 420});
    fixture.count = 0;
    ok(isle_game_pause_source_processes(world.game, owner, engine_source_pause_pulse, 0));
    ok(isle_game_tick(world.game)); trace(fixture, {460, 461, 460, 420});
    require(world.arena.cursor == used && counts(world).processes == 3,
            "rejected source function pauses changed storage or process membership");
}

void source_function_pause_joins_and_reuses_real_continuations() {
    for (bool cold_reset : {false, true}) {
        EngineFixture retired{}, fresh{};
        World world;
        retired.game = world.game;
        auto* owner = object(world, 1);
        engine_bind_source_payload(owner, &retired);
        ok(isle_game_object_set_retire_callback(world.game, owner, engine_source_retired, &retired));
        IsleGameProcess *first = nullptr, *duplicate = nullptr;
        ok(isle_game_add_source_process(world.game, owner, engine_source_pause_wait,
                                       ISLE_GAME_STACKFUL, 5, &first));
        ok(isle_game_add_source_process(world.game, owner, engine_source_pause_wait,
                                       ISLE_GAME_STACKFUL, 5, &duplicate));
        ok(isle_game_tick(world.game)); trace(retired, {470, 470});
        ok(isle_game_pause_source_processes(world.game, owner, engine_source_pause_wait, 1));
        require(engine_process_waiting(first) && engine_process_waiting(duplicate),
                "pause replaced or ended native sleeping continuations");
        const auto used = world.arena.cursor;
        if (cold_reset) world.reset();
        else ok(isle_game_delete_object(world.game, owner));
        trace(retired, {470, 470, 450});
        require(retired.retire_calls == 1 && retired.after_finish == 0 &&
                engine_process_joined(first) && engine_process_joined(duplicate),
                "paused source continuations did not actually join before native retirement/reset");
        if (!cold_reset) {
            require(isle_game_pause_source_processes(world.game, owner, engine_source_pause_wait, 0) ==
                        ISLE_GAME_INVALID_ARGUMENT,
                    "retired object accepted source resume before slot reuse");
        }
        fresh.game = world.game;
        auto* reused = object(world, 2);
        engine_bind_source_payload(reused, &fresh);
        auto* native = process(world, reused, engine_native_source_pause_pulse, &fresh,
                               ISLE_GAME_SYNCHRONOUS, 5);
        IsleGameProcess* source = nullptr;
        ok(isle_game_add_source_process(world.game, reused, engine_source_pause_pulse,
                                       ISLE_GAME_SYNCHRONOUS, 5, &source));
        require(reused == owner && (native == first || native == duplicate) &&
                (source == first || source == duplicate) && source != native &&
                world.arena.cursor == used,
                "pause lifetime regression failed to exercise actual object/process arena reuse");
        ok(isle_game_pause_source_processes(world.game, reused, engine_source_pause_wait, 1));
        ok(isle_game_tick(world.game)); trace(fresh, {461, 460, 460});
        fresh.count = 0;
        ok(isle_game_pause_source_processes(world.game, reused, engine_source_pause_pulse, 1));
        ok(isle_game_tick(world.game)); trace(fresh, {461, 460});
        fresh.count = 0;
        ok(isle_game_pause_source_processes(world.game, reused, engine_source_pause_pulse, 0));
        ok(isle_game_tick(world.game)); trace(fresh, {461, 460, 460});
        trace(retired, {470, 470, 450});
        require(retired.after_finish == 0 && retired.retire_calls == 1 && fresh.failures == 0,
                "reused source/native slots retained paused identity or resumed retired C frames");
    }
}

void source_wait_replace_and_retirement() {
    {
        World world;
        EngineFixture fixture{}; fixture.game = world.game; fixture.shared_value = 37;
        auto* owner = object(world, 1);
        engine_bind_source_payload(owner, &fixture);
        ok(isle_game_object_set_retire_callback(world.game, owner, engine_source_retired, &fixture));
        EngineRecord high{&fixture, 440}, low{&fixture, 430};
        process(world, owner, engine_record_process, &high, ISLE_GAME_SYNCHRONOUS, 11);
        process(world, owner, engine_record_process, &low, ISLE_GAME_SYNCHRONOUS, 0);
        IsleGameProcess* source = nullptr;
        ok(isle_game_add_source_process(world.game, owner, engine_source_a, ISLE_GAME_STACKFUL, 5, &source));
        ok(isle_game_tick(world.game)); trace(fixture, {440, 410, 430});
        fixture.shared_value = 99;
        ok(isle_game_tick(world.game)); trace(fixture, {440, 410, 430, 440, 411, 420, 430});
        require(fixture.local_result == 54 && fixture.after_finish == 0,
                "source wait lost its C local or returned after identity-aware stackful self-end");
        require(counts(world).processes == 3, "same-tick source replacement did not retire its parent");
        ok(isle_game_tick(world.game));
        trace(fixture, {440, 410, 430, 440, 411, 420, 430, 440, 421, 422, 450});
        require(counts(world).objects == 0 && counts(world).processes == 0 &&
                fixture.retire_calls == 1 && fixture.retired_continued == 0,
                "source self-deletion failed its deferred joined retirement fence");
        ok(isle_game_tick(world.game));
        require(fixture.count == 11 && fixture.retire_calls == 1,
                "source object dispatched again after retirement");
    }
    {
        World world;
        EngineFixture fixture{}; fixture.game = world.game; fixture.mode = 2;
        auto* owner = object(world, 1);
        engine_bind_source_payload(owner, &fixture);
        IsleGameProcess* source = nullptr;
        ok(isle_game_add_source_process(world.game, owner, engine_source_b, ISLE_GAME_SYNCHRONOUS, 5, &source));
        require(isle_game_tick(world.game) == ISLE_GAME_EXECUTION_FAILURE,
                "synchronous source ohWait escaped the C failure fence");
        trace(fixture, {420});
        require(fixture.retired_continued == 0 && fixture.retire_calls == 0,
                "illegal source wait returned into source code");
    }
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
IsleGameCamera* camera(World& world, IsleGameObject* owner, bool matrices = true) {
    IsleGameCamera* result = nullptr;
    ok(isle_game_attach_camera(world.game, owner, &result));
    if (matrices) {
        ok(isle_game_camera_add_matrix(world.game, result, ISLE_GAME_CAMERA_PERSPECTIVE_FAST));
        ok(isle_game_camera_add_matrix(world.game, result, ISLE_GAME_CAMERA_LOOK_AT));
    }
    return result;
}

const IsleGameCameraMatrix* matrix(World& world, IsleGameCamera* owner, std::size_t index) {
    const IsleGameCameraMatrix* result = nullptr;
    ok(isle_game_camera_matrix(world.game, owner, index, &result));
    return result;
}

std::array<float, 4> transform(const float m[4][4], std::array<float, 4> point) {
    std::array<float, 4> result{};
    for (std::size_t col = 0; col < 4; ++col)
        for (std::size_t row = 0; row < 4; ++row) result[col] += point[row] * m[row][col];
    return result;
}

void close_to(float value, float expected, float epsilon = 0.0001f) {
    require(std::isfinite(value) && std::abs(value - expected) <= epsilon, "camera geometry differs");
}

void camera_geometry_and_cache() {
    World world;
    auto* owner = object(world, 1);
    auto* cam = camera(world, owner);
    auto* projection = matrix(world, cam, 0);
    auto* view = matrix(world, cam, 1);
    const IsleGameCameraParameters* parameters = nullptr;
    ok(isle_game_camera_parameters(world.game, cam, &parameters));
    auto eye = transform(view->floating, {parameters->look_at.eye[0], parameters->look_at.eye[1],
                                         parameters->look_at.eye[2], 1});
    close_to(eye[0], 0); close_to(eye[1], 0); close_to(eye[2], 0); close_to(eye[3], 1);
    auto at = transform(view->floating, {0, 0, 0, 1});
    require(at[2] < 0, "default camera target is not in front of the eye");
    for (const auto [distance, expected] :
         {std::pair{parameters->perspective.near_plane, -1.0f},
          std::pair{parameters->perspective.far_plane, 1.0f}}) {
        const auto point = transform(projection->floating, {0, 0, -distance, 1});
        close_to(point[2] / point[3], expected);
    }
    const auto original_projection_revision = projection->revision, original_view_revision = view->revision;
    const auto arena_used = world.arena.cursor;
    auto p = parameters->perspective;
    p.near_plane = 10; p.far_plane = 25600;
    ok(isle_game_camera_set_perspective(world.game, cam, &p));
    projection = matrix(world, cam, 0);
    std::uint16_t normalization = 0;
    ok(isle_game_camera_perspective_normalization(world.game, cam, &normalization));
    require(normalization == static_cast<std::uint16_t>(131072.0f / (10.0f + 25600.0f)),
            "photo perspective normalization differs");
    const auto narrow = projection->floating[1][1];
    const auto photo_revision = projection->revision;
    for (const auto [distance, expected] : {std::pair{10.0f, -1.0f}, std::pair{25600.0f, 1.0f}}) {
        const auto point = transform(projection->floating, {0, 0, -distance, 1});
        close_to(point[2] / point[3], expected);
    }
    p.fovy = 60;
    ok(isle_game_camera_set_perspective(world.game, cam, &p));
    const IsleGameCameraMatrix* pending_projection = nullptr;
    ok(isle_game_camera_peek_matrix(world.game, cam, 0, &pending_projection));
    require(pending_projection == projection && pending_projection->kind == ISLE_GAME_CAMERA_PERSPECTIVE_FAST &&
            projection->revision == photo_revision && projection->floating[1][1] == narrow &&
            world.arena.cursor == arena_used,
            "pure dirty-camera preflight computed, replaced or allocated a published cache");
    require(matrix(world, cam, 0) == projection && projection->revision == photo_revision + 1,
            "projection update changed stable address or missed cache invalidation");
    require(narrow > projection->floating[1][1] * 2.0f, "wide photo FOV did not widen frustum");
    const float edge = 100.0f * std::tan(p.fovy * 3.14159265358979323846f / 360.0f);
    const auto edge_point = transform(projection->floating, {edge * p.aspect, edge, -100, 1});
    close_to(edge_point[0] / edge_point[3], 1, 0.01f);
    close_to(edge_point[1] / edge_point[3], 1, 0.01f);
    require(view == matrix(world, cam, 1) && view->revision == original_view_revision,
            "perspective changes invalidated unrelated view matrix");
    const auto revision = projection->revision;
    ok(isle_game_camera_set_perspective(world.game, cam, &p));
    auto viewport = parameters->viewport;
    viewport.scale[0] = viewport.translate[0] = 128;
    viewport.scale[1] = viewport.translate[1] = 96;
    ok(isle_game_camera_set_viewport(world.game, cam, &viewport));
    require(matrix(world, cam, 0)->revision == revision && matrix(world, cam, 1)->revision == original_view_revision,
            "identical settings or viewport dirtied computed matrices");
    IsleGameLookAt pose{{10, 20, 30}, {10, 20, 29}, {0, 1, 0}};
    ok(isle_game_camera_set_look_at(world.game, cam, &pose));
    require(matrix(world, cam, 1) == view && view->revision == original_view_revision + 1,
            "look-at update lost stable storage or cached old pose");
    eye = transform(view->floating, {10, 20, 30, 1});
    at = transform(view->floating, {10, 20, 29, 1});
    close_to(eye[0], 0); close_to(eye[1], 0); close_to(eye[2], 0);
    close_to(at[0], 0); close_to(at[1], 0); close_to(at[2], -1);
    const auto up = transform(view->floating, {10, 21, 30, 1});
    close_to(up[1], 1);
    ok(isle_game_camera_set_look_at(world.game, cam, &pose));
    require(matrix(world, cam, 1)->revision == original_view_revision + 1 &&
            matrix(world, cam, 0)->revision == revision && world.arena.cursor == arena_used &&
            photo_revision == original_projection_revision + 1, "cache updates allocated or recomputed unchanged data");
    for (auto* computed : {projection, view}) {
        for (std::size_t row = 0; row < 4; ++row) {
            for (std::size_t col = 0; col < 4; ++col) {
                const auto pair = row * 2 + col / 2;
                const auto integral = computed->fixed.m[pair / 4][pair % 4];
                const auto fraction = computed->fixed.m[2 + pair / 4][pair % 4];
                const auto shift = col % 2 == 0 ? 16u : 0u;
                const auto bits = (((integral >> shift) & 0xffffu) << 16) | ((fraction >> shift) & 0xffffu);
                std::int32_t signed_value;
                std::memcpy(&signed_value, &bits, sizeof(bits));
                close_to(static_cast<float>(signed_value) / 65536.0f, computed->floating[row][col], 1.0f / 65536.0f);
            }
        }
    }
    pose = {{12, -8, 4}, {-2, 3, -5}, {0, 1, 0}};
    ok(isle_game_camera_set_look_at(world.game, cam, &pose));
    view = matrix(world, cam, 1);
    eye = transform(view->floating, {12, -8, 4, 1});
    at = transform(view->floating, {-2, 3, -5, 1});
    close_to(eye[0], 0); close_to(eye[1], 0); close_to(eye[2], 0);
    close_to(at[0], 0); close_to(at[1], 0);
    close_to(at[2], -std::sqrt(14.0f * 14.0f + 11.0f * 11.0f + 9.0f * 9.0f));
    for (std::size_t a = 0; a < 3; ++a) {
        for (std::size_t b = 0; b < 3; ++b) {
            float dot = 0;
            for (std::size_t i = 0; i < 3; ++i) dot += view->floating[i][a] * view->floating[i][b];
            close_to(dot, a == b ? 1.0f : 0.0f);
        }
    }
}

const IsleGameCameraReflection* reflection(World& world, IsleGameCamera* cam, std::size_t index) {
    const IsleGameCameraReflection* result = nullptr;
    ok(isle_game_camera_reflection(world.game, cam, index, &result));
    return result;
}

bool same_camera_values(const IsleGameCameraMatrix& a, const IsleGameCameraMatrix& b) {
    return a.kind == b.kind && a.revision == b.revision &&
        std::memcmp(a.floating, b.floating, sizeof(a.floating)) == 0 &&
        std::memcmp(a.fixed.m, b.fixed.m, sizeof(a.fixed.m)) == 0;
}
bool same_reflection_values(const IsleGameCameraReflection& a, const IsleGameCameraReflection& b) {
    return a.revision == b.revision &&
        std::memcmp(a.direction, b.direction, sizeof(a.direction)) == 0 &&
        std::memcmp(a.color, b.color, sizeof(a.color)) == 0 &&
        std::memcmp(a.color_copy, b.color_copy, sizeof(a.color_copy)) == 0;
}

void camera_roll_geometry_and_cache() {
    World world;
    auto* cam = camera(world, object(world, 1), false);
    ok(isle_game_camera_add_matrix(world.game, cam, ISLE_GAME_CAMERA_LOOK_AT));
    ok(isle_game_camera_add_matrix(world.game, cam, ISLE_GAME_CAMERA_LOOK_AT_ROLL));
    IsleGameLookAt pose{{10, 20, 30}, {10, 20, 29}, {0, 1, 0}};
    ok(isle_game_camera_set_look_at(world.game, cam, &pose));
    ok(isle_game_camera_compute_all(world.game, cam));
    const auto* basic = matrix(world, cam, 0);
    const auto* rolled = matrix(world, cam, 1);
    const auto basic_revision = basic->revision;
    const auto initial = *rolled;
    const auto used = world.arena.cursor;
    constexpr float half_pi = 1.57079632679489661923f;
    for (float roll : {half_pi, -half_pi, 0.47f, -2.3f, 0.0f}) {
        const auto revision = rolled->revision;
        ok(isle_game_camera_set_roll(world.game, cam, roll));
        float stored = 99;
        ok(isle_game_camera_roll(world.game, cam, &stored));
        require(stored == roll, "kind8 roll was not retained as native radians");
        ok(isle_game_camera_compute_all(world.game, cam));
        require(matrix(world, cam, 1) == rolled && rolled->revision == revision + 1 &&
                rolled->kind == ISLE_GAME_CAMERA_LOOK_AT_ROLL &&
                matrix(world, cam, 0) == basic && basic->revision == basic_revision,
                "kind8 roll changed stable borrows or dirtied roll-independent kind6");
        close_to(rolled->floating[0][0], std::cos(roll));
        close_to(rolled->floating[1][0], std::sin(roll));
        close_to(rolled->floating[0][1], -std::sin(roll));
        close_to(rolled->floating[1][1], std::cos(roll));
        const auto eye = transform(rolled->floating, {10, 20, 30, 1});
        const auto at = transform(rolled->floating, {10, 20, 29, 1});
        close_to(eye[0], 0); close_to(eye[1], 0); close_to(eye[2], 0);
        close_to(at[0], 0); close_to(at[1], 0); close_to(at[2], -1);
        ok(isle_game_camera_set_roll(world.game, cam, roll));
        ok(isle_game_camera_compute_all(world.game, cam));
        require(rolled->revision == revision + 1 && world.arena.cursor == used,
                "identical kind8 roll recomputed or allocated");
    }
    for (std::size_t r = 0; r < 4; ++r)
        for (std::size_t c = 0; c < 4; ++c)
            close_to(rolled->floating[r][c], initial.floating[r][c]);
    require(std::memcmp(rolled->fixed.m, initial.fixed.m, sizeof(initial.fixed.m)) == 0,
            "kind8 zero-roll restoration changed fixed numeric words");
    auto revision = rolled->revision;
    pose.up[0] = 0.3f; pose.up[2] = 0.4f;
    ok(isle_game_camera_set_look_at(world.game, cam, &pose));
    ok(isle_game_camera_compute_all(world.game, cam));
    require(rolled->revision == revision && basic->revision == basic_revision + 1,
            "kind8 used mutable up instead of source fixed Y-up");
    const IsleGamePerspective perspective{60, 1, 1, 100, 1};
    const IsleGameViewport viewport{{128, 96, 511, 0}, {128, 96, 511, 0}};
    ok(isle_game_camera_set_perspective(world.game, cam, &perspective));
    ok(isle_game_camera_set_viewport(world.game, cam, &viewport));
    ok(isle_game_camera_compute_all(world.game, cam));
    require(rolled->revision == revision && world.arena.cursor == used,
            "projection/viewport mutation dirtied or allocated kind8");
    for (const IsleGameLookAt arbitrary : {
            IsleGameLookAt{{5, 0, 0}, {0, 0, 0}, {0, 1, 0}},
            IsleGameLookAt{{-5, 0, 0}, {0, 0, 0}, {0, 1, 0}},
            IsleGameLookAt{{12, -8, 4}, {-2, 3, -5}, {0, 1, 0}}}) {
        ok(isle_game_camera_set_look_at(world.game, cam, &arbitrary));
        const auto unrolled = *matrix(world, cam, 0);
        ok(isle_game_camera_set_roll(world.game, cam, 0.61f));
        const auto before = rolled->revision;
        require(matrix(world, cam, 1) == rolled && rolled->revision == before + 1,
                "kind8 arbitrary-axis mutation missed stable lazy cache");
        for (std::size_t c = 0; c < 3; ++c) {
            close_to(rolled->floating[c][0], std::cos(0.61f) * unrolled.floating[c][0] +
                     std::sin(0.61f) * unrolled.floating[c][1]);
            close_to(rolled->floating[c][1], -std::sin(0.61f) * unrolled.floating[c][0] +
                     std::cos(0.61f) * unrolled.floating[c][1]);
            close_to(rolled->floating[c][2], unrolled.floating[c][2]);
        }
    }
    auto* paired = camera(world, object(world, 2), false);
    ok(isle_game_camera_add_matrix(world.game, paired, ISLE_GAME_CAMERA_LOOK_AT_ROLL));
    ok(isle_game_camera_add_matrix(world.game, paired, ISLE_GAME_CAMERA_LOOK_AT_REFLECT_ROLL));
    pose = {{0, 0, 5}, {0, 0, 0}, {0, 1, 0}};
    ok(isle_game_camera_set_look_at(world.game, paired, &pose));
    ok(isle_game_camera_compute_all(world.game, paired));
    const auto* kind8 = matrix(world, paired, 0);
    const auto* kind14 = matrix(world, paired, 1);
    const auto* reflected = reflection(world, paired, 1);
    const auto first_revision = kind8->revision, second_revision = kind14->revision;
    ok(isle_game_camera_set_roll(world.game, paired, 0.31f));
    ok(isle_game_camera_compute_all(world.game, paired));
    require(kind8->revision == first_revision + 1 && kind14->revision == second_revision + 1 &&
            reflected->revision == kind14->revision,
            "shared roll did not dirty both kind8 and kind14");
    pose.up[0] = 0.25f;
    ok(isle_game_camera_set_look_at(world.game, paired, &pose));
    ok(isle_game_camera_compute_all(world.game, paired));
    require(kind8->revision == first_revision + 1 && kind14->revision == second_revision + 2 &&
            reflected->revision == kind14->revision,
            "up-only edit dirtied kind8 or missed kind14 reflection");
    const IsleGameCameraReflection* absent = reflected;
    require(isle_game_camera_reflection(world.game, paired, 0, &absent) == ISLE_GAME_INVALID_STATE &&
            absent == nullptr, "kind8 fabricated reflection output");
}

void camera_roll_ignores_unused_up() {
    World world;
    auto* fixed_y = camera(world, object(world, 1), false);
    ok(isle_game_camera_add_matrix(world.game, fixed_y, ISLE_GAME_CAMERA_LOOK_AT_ROLL));
    const IsleGameLookAt source_pose{{0, 0, 5}, {0, 0, 0}, {0, 0, 1}};
    require(isle_game_camera_set_look_at(world.game, fixed_y, &source_pose) == ISLE_GAME_OK,
            "kind8 rejected valid fixed-Y view because its unused supplied up was parallel");
    const auto* view = matrix(world, fixed_y, 0);
    const auto stable = *view;
    auto unused_up = source_pose;
    unused_up.up[2] = 0;
    ok(isle_game_camera_set_look_at(world.game, fixed_y, &unused_up));
    require(same_camera_values(stable, *matrix(world, fixed_y, 0)),
            "unused finite kind8 up changed its view or revision");
    const auto used = world.arena.cursor;
    const auto matrix_count = counts(world).matrices;
    require(isle_game_camera_add_matrix(world.game, fixed_y, ISLE_GAME_CAMERA_LOOK_AT) ==
                ISLE_GAME_INVALID_ARGUMENT && world.arena.cursor == used &&
                counts(world).matrices == matrix_count,
            "basic-view attachment admitted a degenerate consumed up or mutated its pool on rejection");
    auto* mixed = camera(world, object(world, 2), false);
    ok(isle_game_camera_add_matrix(world.game, mixed, ISLE_GAME_CAMERA_LOOK_AT));
    ok(isle_game_camera_add_matrix(world.game, mixed, ISLE_GAME_CAMERA_LOOK_AT_ROLL));
    ok(isle_game_camera_compute_all(world.game, mixed));
    const auto basic = *matrix(world, mixed, 0);
    const auto rolled = *matrix(world, mixed, 1);
    require(isle_game_camera_set_look_at(world.game, mixed, &source_pose) == ISLE_GAME_INVALID_ARGUMENT &&
            same_camera_values(basic, *matrix(world, mixed, 0)) &&
            same_camera_values(rolled, *matrix(world, mixed, 1)),
            "mixed basic/kind8 view ignored a consumed up or changed matrices on rejected mutation");
}

void camera_roll_boundaries_and_reuse() {
    World world;
    auto* owner = object(world, 1);
    auto* cam = camera(world, owner, false);
    const IsleGameLookAt vertical{{0, 1, 0}, {0, 0, 0}, {0, 0, 1}};
    ok(isle_game_camera_set_look_at(world.game, cam, &vertical));
    const auto before_add = world.arena.cursor;
    require(isle_game_camera_add_matrix(world.game, cam, ISLE_GAME_CAMERA_LOOK_AT_ROLL) ==
                ISLE_GAME_INVALID_ARGUMENT && world.arena.cursor == before_add &&
                counts(world).matrices == 0, "kind8 admitted degenerate source Y-up basis or allocated on failure");
    const IsleGameLookAt pose{{0, 0, 0}, {0, 0, -1}, {0, 1, 0}};
    ok(isle_game_camera_set_look_at(world.game, cam, &pose));
    ok(isle_game_camera_add_matrix(world.game, cam, ISLE_GAME_CAMERA_LOOK_AT_ROLL));
    const auto* borrowed = matrix(world, cam, 0);
    const auto used = world.arena.cursor;
    const auto initial = *borrowed;
    for (float invalid : {std::numeric_limits<float>::infinity(),
                          -std::numeric_limits<float>::infinity(),
                          std::numeric_limits<float>::quiet_NaN()})
        require(isle_game_camera_set_roll(world.game, cam, invalid) == ISLE_GAME_INVALID_ARGUMENT,
                "kind8 accepted nonfinite radians");
    std::feclearexcept(FE_ALL_EXCEPT);
    require(isle_game_camera_set_look_at(world.game, cam, &vertical) == ISLE_GAME_INVALID_ARGUMENT &&
            same_camera_values(initial, *matrix(world, cam, 0)) &&
            (std::fetestexcept(FE_INVALID | FE_DIVBYZERO)) == 0,
            "rejected kind8 vertical pose dirtied cache or divided by zero");
    for (float extreme : {std::numeric_limits<float>::max(), -std::numeric_limits<float>::max()}) {
        ok(isle_game_camera_set_roll(world.game, cam, extreme));
        const auto* computed = matrix(world, cam, 0);
        for (const auto& row : computed->floating)
            for (float lane : row) require(std::isfinite(lane), "finite extreme radians produced nonfinite kind8");
    }
    ok(isle_game_camera_set_roll(world.game, cam, 0));
    auto boundary = pose;
    boundary.eye[0] = boundary.at[0] = 32768;
    ok(isle_game_camera_set_look_at(world.game, cam, &boundary));
    require(matrix(world, cam, 0) == borrowed && borrowed->floating[3][0] == -32768,
            "kind8 rejected representable negative 16.16 boundary");
    const auto stable = *borrowed;
    for (float invalid : {std::nextafter(32768.0f, std::numeric_limits<float>::infinity()), -32768.0f}) {
        auto out_of_range = boundary;
        out_of_range.eye[0] = out_of_range.at[0] = invalid;
        ok(isle_game_camera_set_look_at(world.game, cam, &out_of_range));
        const IsleGameCameraMatrix* failed = borrowed;
        require(isle_game_camera_matrix(world.game, cam, 0, &failed) == ISLE_GAME_INVALID_ARGUMENT &&
                failed == nullptr && same_camera_values(stable, *borrowed),
                "kind8 range failure wrapped or published partial stable matrix");
    }
    ok(isle_game_camera_set_look_at(world.game, cam, &pose));
    require(matrix(world, cam, 0) == borrowed && borrowed->revision == stable.revision + 1 &&
            world.arena.cursor == used, "kind8 range recovery lost stable borrow/revision or allocated");
    bool wrong_owner = false;
    std::thread foreign([&] {
        const IsleGameCameraMatrix* output = borrowed;
        const IsleGameCameraReflection* absent = nullptr;
        wrong_owner = isle_game_camera_compute_all(world.game, cam) == ISLE_GAME_WRONG_OWNER &&
            isle_game_camera_matrix(world.game, cam, 0, &output) == ISLE_GAME_WRONG_OWNER &&
            output == nullptr && isle_game_camera_reflection(world.game, cam, 0, &absent) ==
                ISLE_GAME_WRONG_OWNER && absent == nullptr;
    });
    foreign.join();
    require(wrong_owner, "kind8 admitted a foreign gameplay owner");
    ok(isle_game_remove_camera(world.game, cam));
    const IsleGameCameraMatrix* expired = borrowed;
    require(isle_game_camera_matrix(world.game, cam, 0, &expired) == ISLE_GAME_INVALID_ARGUMENT && !expired,
            "removed kind8 retained a live matrix handle");
    cam = camera(world, owner, false);
    ok(isle_game_camera_add_matrix(world.game, cam, ISLE_GAME_CAMERA_LOOK_AT_ROLL));
    float roll = 99;
    ok(isle_game_camera_roll(world.game, cam, &roll));
    require(roll == 0 && matrix(world, cam, 0)->revision == 1 && world.arena.cursor == used,
            "recycled kind8 camera inherited roll/cache or failed bounded pool reuse");
}

void camera_roll_batch_failure_is_atomic() {
    for (bool view_first : {false, true}) {
        World world;
        auto* cam = camera(world, object(world, 1), false);
        ok(isle_game_camera_add_matrix(world.game, cam,
            view_first ? ISLE_GAME_CAMERA_LOOK_AT_ROLL : ISLE_GAME_CAMERA_PERSPECTIVE_FAST));
        ok(isle_game_camera_add_matrix(world.game, cam,
            view_first ? ISLE_GAME_CAMERA_PERSPECTIVE_FAST : ISLE_GAME_CAMERA_LOOK_AT_ROLL));
        const std::size_t pi = view_first ? 1 : 0, vi = view_first ? 0 : 1;
        IsleGamePerspective p{60, 4.0f / 3, 10, 25600, 1};
        const IsleGameLookAt pose{{0, 0, 0}, {0, 0, -1}, {0, 1, 0}};
        ok(isle_game_camera_set_perspective(world.game, cam, &p));
        ok(isle_game_camera_set_look_at(world.game, cam, &pose));
        ok(isle_game_camera_compute_all(world.game, cam));
        const auto* projection = matrix(world, cam, pi);
        const auto* view = matrix(world, cam, vi);
        auto old_projection = *projection, old_view = *view;
        const auto projection_revision = projection->revision, view_revision = view->revision;
        ok(isle_game_camera_set_roll(world.game, cam, 0.47f));
        ok(isle_game_camera_compute_all(world.game, cam));
        require(projection->revision == projection_revision && view->revision == view_revision + 1,
                "roll-only kind8 update dirtied the projection or missed the view");
        ok(isle_game_camera_set_roll(world.game, cam, 0));
        ok(isle_game_camera_compute_all(world.game, cam));
        old_projection = *projection; old_view = *view;
        const auto used = world.arena.cursor;
        p.fovy = 45;
        ok(isle_game_camera_set_perspective(world.game, cam, &p));
        auto far = pose; far.eye[0] = far.at[0] = 40000;
        ok(isle_game_camera_set_look_at(world.game, cam, &far));
        require(isle_game_camera_compute_all(world.game, cam) == ISLE_GAME_INVALID_ARGUMENT &&
                same_camera_values(old_projection, *projection) && same_camera_values(old_view, *view),
                "failed kind8 batch published staged projection/view or revision");
        ok(isle_game_camera_set_look_at(world.game, cam, &pose));
        ok(isle_game_camera_compute_all(world.game, cam));
        require(matrix(world, cam, pi) == projection && matrix(world, cam, vi) == view &&
                projection->revision == old_projection.revision + 1 &&
                view->revision == old_view.revision + 1,
                "kind8 batch recovery moved borrows or lost dirty inputs");
        old_projection = *projection; old_view = *view;
        p.scale = 40000;
        ok(isle_game_camera_set_perspective(world.game, cam, &p));
        ok(isle_game_camera_set_roll(world.game, cam, 0.31f));
        require(isle_game_camera_compute_all(world.game, cam) == ISLE_GAME_INVALID_ARGUMENT &&
                same_camera_values(old_projection, *projection) && same_camera_values(old_view, *view),
                "failed projection published an earlier kind8 roll computation");
        p.scale = 1;
        ok(isle_game_camera_set_perspective(world.game, cam, &p));
        ok(isle_game_camera_compute_all(world.game, cam));
        require(projection->revision == old_projection.revision + 1 && view->revision == old_view.revision + 1,
                "kind8/projection atomic failure was not recoverable");
        std::uint16_t norm = 0;
        ok(isle_game_camera_perspective_normalization(world.game, cam, &norm));
        require(norm == static_cast<std::uint16_t>(131072.0f / (p.near_plane + p.far_plane)),
                "kind8 batch recovery published incorrect projection normalization");
        old_projection = *projection; old_view = *view;
        ok(isle_game_camera_compute_all(world.game, cam));
        require(same_camera_values(old_projection, *projection) && same_camera_values(old_view, *view) &&
                world.arena.cursor == used, "cached kind8 batch recomputed or allocated");
    }
}

void camera_reflection_roll_geometry_and_state() {
    World world;
    auto* owner = object(world, 1);
    auto* cam = camera(world, owner, false);
    ok(isle_game_camera_add_matrix(world.game, cam, ISLE_GAME_CAMERA_LOOK_AT));
    ok(isle_game_camera_add_matrix(world.game, cam, ISLE_GAME_CAMERA_LOOK_AT_REFLECT_ROLL));
    IsleGameLookAt pose{{10, 20, 30}, {10, 20, 29}, {0, 1, 0}};
    ok(isle_game_camera_set_look_at(world.game, cam, &pose));
    auto* basic = matrix(world, cam, 0);
    auto* rolled = matrix(world, cam, 1);
    auto* reflected = reflection(world, cam, 1);
    require(reflected->direction[0][0] == 127 && reflected->direction[1][1] == 127 &&
            reflected->direction[0][1] == 0 && reflected->direction[0][2] == 0 &&
            reflected->direction[1][0] == 0 && reflected->direction[1][2] == 0,
            "axis-aligned reflection directions are not actual signed 1.7 vectors");
    for (std::size_t axis = 0; axis < 2; ++axis)
        for (std::size_t component = 0; component < 3; ++component)
            require(reflected->color[axis][component] == (axis == 1 && component == 1 ? 128 : 0) &&
                    reflected->color_copy[axis][component] == reflected->color[axis][component],
                    "GAME reflection colors were fabricated or omitted");
    const auto initial = *rolled;
    const auto initial_reflection = *reflected;
    const auto basic_revision = basic->revision;
    const auto used = world.arena.cursor;
    constexpr float half_pi = 1.57079632679489661923f;
    for (const float roll : {half_pi, -half_pi, 0.47f, -2.3f, 0.0f}) {
        ok(isle_game_camera_set_roll(world.game, cam, roll));
        float stored = 99;
        ok(isle_game_camera_roll(world.game, cam, &stored));
        require(stored == roll, "roll property did not retain finite radians");
        const auto old_revision = rolled->revision;
        require(reflection(world, cam, 1) == reflected && matrix(world, cam, 1) == rolled &&
                rolled->revision == old_revision + 1 && reflected->revision == rolled->revision,
                "matrix/reflection borrows did not share one stable lazy cache revision");
        close_to(rolled->floating[0][0], std::cos(roll));
        close_to(rolled->floating[1][0], std::sin(roll));
        close_to(rolled->floating[0][1], -std::sin(roll));
        close_to(rolled->floating[1][1], std::cos(roll));
        const auto eye = transform(rolled->floating, {10, 20, 30, 1});
        const auto at = transform(rolled->floating, {10, 20, 29, 1});
        close_to(eye[0], 0); close_to(eye[1], 0); close_to(eye[2], 0);
        close_to(at[0], 0); close_to(at[1], 0); close_to(at[2], -1);
        for (std::size_t axis = 0; axis < 2; ++axis)
            for (std::size_t component = 0; component < 3; ++component) {
                const float scaled = rolled->floating[component][axis] * 128;
                const auto expected = static_cast<int>(std::min(scaled, 127.0f));
                require(reflected->direction[axis][component] == expected &&
                        expected >= -128 && expected <= 127,
                        "reflection quantization/sign is not derived from rolled camera basis");
            }
        if (roll == half_pi)
            require(reflected->direction[0][1] == 127 && reflected->direction[1][0] == -128,
                    "positive roll reflected directions have wrong handedness/endpoints");
        ok(isle_game_camera_set_roll(world.game, cam, roll));
        require(matrix(world, cam, 0)->revision == basic_revision &&
                matrix(world, cam, 1)->revision == old_revision + 1 && world.arena.cursor == used,
                "roll recomputed basic/unchanged matrices or allocated");
    }
    require(std::memcmp(initial.floating, rolled->floating, sizeof(initial.floating)) == 0 &&
            std::memcmp(&initial.fixed, &rolled->fixed, sizeof(initial.fixed)) == 0 &&
            std::memcmp(initial_reflection.direction, reflected->direction,
                        sizeof(initial_reflection.direction)) == 0,
            "restoring zero roll did not restore camera and reflection values");
    for (const IsleGameLookAt arbitrary : {
            IsleGameLookAt{{5, 0, 0}, {0, 0, 0}, {0, 1, 0}},
            IsleGameLookAt{{-5, 0, 0}, {0, 0, 0}, {0, 1, 0}},
            IsleGameLookAt{{12, -8, 4}, {-2, 3, -5}, {0.2f, 2, 0.4f}}}) {
        ok(isle_game_camera_set_look_at(world.game, cam, &arbitrary));
        const auto unrolled = *matrix(world, cam, 0);
        ok(isle_game_camera_set_roll(world.game, cam, 0.61f));
        rolled = matrix(world, cam, 1);
        for (std::size_t c = 0; c < 3; ++c) {
            close_to(rolled->floating[c][0], std::cos(0.61f) * unrolled.floating[c][0] +
                     std::sin(0.61f) * unrolled.floating[c][1]);
            close_to(rolled->floating[c][1], -std::sin(0.61f) * unrolled.floating[c][0] +
                     std::cos(0.61f) * unrolled.floating[c][1]);
            close_to(rolled->floating[c][2], unrolled.floating[c][2]);
        }
        require(reflection(world, cam, 1)->revision == rolled->revision,
                "arbitrary-axis pose change did not invalidate reflection cache");
    }
    ok(isle_game_camera_set_look_at(world.game, cam, &pose));
    std::fenv_t environment;
    require(std::fegetenv(&environment) == 0, "cannot save camera test floating environment");
    for (int rounding : {FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO, FE_TONEAREST}) {
        require(std::fesetround(rounding) == 0, "cannot select camera test rounding");
        std::feclearexcept(FE_ALL_EXCEPT);
        ok(isle_game_camera_set_roll(world.game, cam, static_cast<float>(rounding) * 0.0001f + 0.7f));
        const auto* checked = reflection(world, cam, 1);
        require(checked->revision == rolled->revision && std::fegetround() == rounding &&
                (std::fetestexcept(FE_INVALID | FE_DIVBYZERO | FE_OVERFLOW)) == 0,
                "reflection computation changed rounding or raised invalid/divide/overflow exceptions");
    }
    require(std::fesetenv(&environment) == 0, "cannot restore camera test floating environment");
    ok(isle_game_camera_set_roll(world.game, cam, 0));
    reflection(world, cam, 1);
    const auto revision = rolled->revision;
    const auto stable = *rolled;
    const auto stable_reflection = *reflected;
    std::feclearexcept(FE_ALL_EXCEPT);
    for (float invalid : {std::numeric_limits<float>::infinity(),
                          -std::numeric_limits<float>::infinity(),
                          std::numeric_limits<float>::quiet_NaN()})
        require(isle_game_camera_set_roll(world.game, cam, invalid) == ISLE_GAME_INVALID_ARGUMENT,
                "nonfinite roll mutated a camera");
    auto bad = pose;
    bad.at[2] = bad.eye[2];
    require(isle_game_camera_set_look_at(world.game, cam, &bad) == ISLE_GAME_INVALID_ARGUMENT,
            "reflection camera accepted zero-length look vector");
    require((std::fetestexcept(FE_INVALID | FE_DIVBYZERO)) == 0 &&
            matrix(world, cam, 1)->revision == revision,
            "rejected mutation performed invalid floating math or dirtied cache");
    auto far = pose; far.eye[0] = far.at[0] = 40000;
    ok(isle_game_camera_set_look_at(world.game, cam, &far));
    const IsleGameCameraReflection* failed = reflected;
    require(isle_game_camera_reflection(world.game, cam, 1, &failed) == ISLE_GAME_INVALID_ARGUMENT &&
            failed == nullptr && same_camera_values(stable, *rolled) &&
            same_reflection_values(stable_reflection, *reflected),
            "failed fixed computation exposed partial matrix/reflection cache contents");
    ok(isle_game_camera_set_look_at(world.game, cam, &pose));
    require(reflection(world, cam, 1)->revision == revision + 1,
            "pose restoration failed to refresh reflection cache");
    failed = reflected;
    require(isle_game_camera_reflection(world.game, cam, 0, &failed) == ISLE_GAME_INVALID_STATE && !failed,
            "basic camera fabricated reflection output");
    failed = reflected;
    require(isle_game_camera_reflection(world.game, cam, 2, &failed) == ISLE_GAME_INVALID_ARGUMENT && !failed,
            "reflection query accepted out-of-range matrix index");
    require(isle_game_camera_reflection(world.game, cam, 1, nullptr) == ISLE_GAME_INVALID_ARGUMENT &&
            isle_game_camera_roll(world.game, cam, nullptr) == ISLE_GAME_INVALID_ARGUMENT,
            "reflection/roll query accepted null output");
    bool rejected = false;
    std::thread foreign([&] {
        const IsleGameCameraReflection* output = reflected;
        float roll = 99;
        rejected = isle_game_camera_reflection(world.game, cam, 1, &output) == ISLE_GAME_WRONG_OWNER &&
            output == nullptr && isle_game_camera_set_roll(world.game, cam, 1) == ISLE_GAME_WRONG_OWNER &&
            isle_game_camera_roll(world.game, cam, &roll) == ISLE_GAME_WRONG_OWNER && roll == 0;
    });
    foreign.join();
    require(rejected, "reflection camera admitted foreign owner access");
    ok(isle_game_delete_object(world.game, owner));
    failed = reflected;
    require(isle_game_camera_reflection(world.game, cam, 1, &failed) == ISLE_GAME_INVALID_ARGUMENT &&
            !failed && counts(world).matrices == 0 && counts(world).cameras == 0,
            "deleted camera retained a live reflection borrow");
    auto* replacement = object(world, 2);
    cam = camera(world, replacement, false);
    ok(isle_game_camera_add_matrix(world.game, cam, ISLE_GAME_CAMERA_LOOK_AT_REFLECT_ROLL));
    float roll = 99;
    ok(isle_game_camera_roll(world.game, cam, &roll));
    require(roll == 0 && reflection(world, cam, 0)->revision == 1 && world.arena.cursor == used,
            "recycled reflection camera inherited roll/cache or failed actual-size arena reuse");
}

void camera_batch_failure_is_atomic() {
    World world;
    auto* cam = camera(world, object(world, 1), false);
    ok(isle_game_camera_add_matrix(world.game, cam, ISLE_GAME_CAMERA_PERSPECTIVE_FAST));
    ok(isle_game_camera_add_matrix(world.game, cam, ISLE_GAME_CAMERA_LOOK_AT_REFLECT_ROLL));
    IsleGamePerspective perspective{60, 4.0f / 3, 10, 25600, 1};
    const IsleGameLookAt pose{{0, 0, 0}, {0, 0, -1}, {0, 1, 0}};
    ok(isle_game_camera_set_perspective(world.game, cam, &perspective));
    ok(isle_game_camera_set_look_at(world.game, cam, &pose));
    ok(isle_game_camera_compute_all(world.game, cam));
    const auto* projection = matrix(world, cam, 0);
    const auto* view = matrix(world, cam, 1);
    const auto* directions = reflection(world, cam, 1);
    const auto previous_projection = *projection, previous_view = *view;
    const auto previous_directions = *directions;
    const auto used = world.arena.cursor;

    perspective.fovy = 45;
    ok(isle_game_camera_set_perspective(world.game, cam, &perspective));
    auto unrepresentable = pose;
    unrepresentable.eye[0] = unrepresentable.at[0] = 40000;
    ok(isle_game_camera_set_look_at(world.game, cam, &unrepresentable));
    require(isle_game_camera_compute_all(world.game, cam) == ISLE_GAME_INVALID_ARGUMENT &&
            same_camera_values(previous_projection, *projection) &&
            same_camera_values(previous_view, *view) &&
            same_reflection_values(previous_directions, *directions) && world.arena.cursor == used,
            "failed second camera computation published the first payload or changed stable ownership");

    ok(isle_game_camera_set_look_at(world.game, cam, &pose));
    ok(isle_game_camera_compute_all(world.game, cam));
    require(projection->revision == previous_projection.revision + 1 &&
            view->revision == previous_view.revision + 1 &&
            directions->revision == previous_directions.revision + 1 &&
            std::memcmp(projection->fixed.m, previous_projection.fixed.m, sizeof(projection->fixed.m)) != 0 &&
            std::memcmp(view->fixed.m, previous_view.fixed.m, sizeof(view->fixed.m)) == 0,
            "camera batch recovery lost dirty inputs, double-published, or failed to restore source view");
    const auto recovered_projection = *projection, recovered_view = *view;
    const auto recovered_directions = *directions;
    ok(isle_game_camera_compute_all(world.game, cam));
    require(same_camera_values(recovered_projection, *projection) &&
            same_camera_values(recovered_view, *view) &&
            same_reflection_values(recovered_directions, *directions) && world.arena.cursor == used,
            "unchanged camera batch recomputed, replaced or allocated cached output");
}


void camera_rejection_and_typed_cleanup() {
    World world;
    auto* owner = object(world, 1);
    auto* sprite_owner = object(world, 2);
    IsleGameSprite* sprite = nullptr;
    ok(isle_game_add_sprite(world.game, sprite_owner, nullptr, &sprite));
    auto* cam = camera(world, owner, false);
    std::size_t extent = 99;
    ok(isle_game_camera_matrix_count(world.game, cam, &extent));
    require(extent == 0, "new camera unexpectedly owns matrix nodes");
    const IsleGameCameraMatrix* borrowed = nullptr;
    require(isle_game_camera_matrix(world.game, cam, 0, &borrowed) == ISLE_GAME_INVALID_ARGUMENT && !borrowed,
            "missing matrix was borrowed");
    std::uint16_t norm = 99;
    require(isle_game_camera_perspective_normalization(world.game, cam, &norm) == ISLE_GAME_INVALID_STATE && norm == 0,
            "normalization fabricated without a projection");
    IsleGameCamera* rejected = cam;
    require(isle_game_attach_camera(world.game, owner, &rejected) == ISLE_GAME_INVALID_STATE && !rejected,
            "attach overwrote existing camera");
    rejected = cam;
    require(isle_game_attach_camera(world.game, sprite_owner, &rejected) == ISLE_GAME_INVALID_STATE && !rejected,
            "attach overwrote sprite payload");
    auto* rejected_sprite = sprite;
    require(isle_game_add_sprite(world.game, owner, nullptr, &rejected_sprite) == ISLE_GAME_INVALID_ARGUMENT && !rejected_sprite,
            "sprite attach interpreted camera as sprite list");
    ok(isle_game_camera_add_matrix(world.game, cam, ISLE_GAME_CAMERA_PERSPECTIVE_FAST));
    require(engine_add_camera_matrix_with_invalid_kind(world.game, cam) ==
                ISLE_GAME_INVALID_ARGUMENT, "unsupported matrix kind was silently accepted");
    ok(isle_game_camera_add_matrix(world.game, cam, ISLE_GAME_CAMERA_LOOK_AT));
    require(isle_game_camera_add_matrix(world.game, cam, ISLE_GAME_CAMERA_LOOK_AT) == ISLE_GAME_EXHAUSTED &&
            counts(world).matrices == 2, "third camera matrix corrupted bounded ownership");
    auto* stable = matrix(world, cam, 0);
    borrowed = stable;
    require(isle_game_camera_matrix(world.game, cam, 2, &borrowed) == ISLE_GAME_INVALID_ARGUMENT && !borrowed,
            "out-of-range matrix query leaked pointer output");
    IsleGamePerspective invalid{30, 0, 10, 25600, 1};
    require(isle_game_camera_set_perspective(world.game, cam, &invalid) == ISLE_GAME_INVALID_ARGUMENT,
            "zero aspect accepted");
    invalid = {std::numeric_limits<float>::quiet_NaN(), 1, 10, 25600, 1};
    require(isle_game_camera_set_perspective(world.game, cam, &invalid) == ISLE_GAME_INVALID_ARGUMENT,
            "nonfinite FOV accepted");
    IsleGameLookAt degenerate{{1, 2, 3}, {1, 2, 3}, {0, 1, 0}};
    require(isle_game_camera_set_look_at(world.game, cam, &degenerate) == ISLE_GAME_INVALID_ARGUMENT,
            "coincident eye and target accepted");
    degenerate = {{0, 0, 1}, {0, 0, 0}, {0, 0, 2}};
    require(isle_game_camera_set_look_at(world.game, cam, &degenerate) == ISLE_GAME_INVALID_ARGUMENT,
            "parallel up/look accepted");
    const auto stable_revision = stable->revision;
    require(matrix(world, cam, 0)->revision == stable_revision, "rejected input dirtied projection");
    IsleGameLookAt too_far{{40000, 0, 0}, {0, 0, 0}, {0, 0, 1}};
    ok(isle_game_camera_set_look_at(world.game, cam, &too_far));
    borrowed = stable;
    require(isle_game_camera_matrix(world.game, cam, 1, &borrowed) == ISLE_GAME_INVALID_ARGUMENT && !borrowed,
            "unrepresentable fixed translation wrapped");
    bool wrong_owner = false;
    std::thread other([&] {
        const IsleGameCameraMatrix* matrix_output = stable;
        const IsleGameCameraParameters* parameter_output = nullptr;
        IsleGameCamera* camera_output = cam;
        wrong_owner = isle_game_camera_matrix(world.game, cam, 0, &matrix_output) == ISLE_GAME_WRONG_OWNER &&
            matrix_output == nullptr &&
            isle_game_camera_parameters(world.game, cam, &parameter_output) == ISLE_GAME_WRONG_OWNER &&
            parameter_output == nullptr &&
            isle_game_attach_camera(world.game, owner, &camera_output) == ISLE_GAME_WRONG_OWNER &&
            camera_output == nullptr && isle_game_remove_camera(world.game, cam) == ISLE_GAME_WRONG_OWNER;
    });
    other.join();
    require(wrong_owner, "camera ownership admitted foreign-thread access");
    ok(isle_game_delete_object(world.game, owner));
    auto live = counts(world);
    require(live.cameras == 0 && live.matrices == 0 && live.sprites == 1 && live.objects == 1,
            "typed camera deletion freed sprite or leaked matrices");
    IsleSprite* sprite_view = nullptr;
    ok(isle_game_sprite_view(world.game, sprite, &sprite_view));
    auto* replacement = object(world, 3);
    cam = camera(world, replacement);
    require(counts(world).cameras == 1 && counts(world).matrices == 2, "recycled camera counts differ");
    ok(isle_game_remove_camera(world.game, cam));
    const auto recycled_cursor = world.arena.cursor;
    cam = camera(world, replacement, false);
    ok(isle_game_camera_add_matrix(world.game, cam, ISLE_GAME_CAMERA_PERSPECTIVE_FAST));
    ok(isle_game_camera_add_matrix(world.game, cam, ISLE_GAME_CAMERA_PERSPECTIVE_FAST));
    require(isle_game_camera_add_matrix(world.game, cam, ISLE_GAME_CAMERA_LOOK_AT) == ISLE_GAME_EXHAUSTED &&
            counts(world).matrices == 2 && world.arena.cursor == recycled_cursor,
            "two same-kind GAME matrices failed bounded pool reuse");
    ok(isle_game_remove_camera(world.game, cam));
    ok(isle_game_add_sprite(world.game, replacement, nullptr, &rejected_sprite));
    ok(isle_game_delete_object(world.game, sprite_owner));
    require(counts(world).sprites == 1 && counts(world).cameras == 0, "sprite cleanup dispatched as camera");
}

void camera_exhaustion_and_reuse() {
    for (bool allocate_matrices : {false, true}) {
        World world(8192);
        auto* a = object(world, 1);
        auto* b = object(world, 2);
        auto* cam = camera(world, a, allocate_matrices);
        auto* first = allocate_matrices ? matrix(world, cam, 0) : nullptr;
        // Exhaust actual bytes, not merely the capacity for another GObj:
        // native camera/matrix sizes are independent of object ABI and padding.
        const auto remaining=world.arena.storage.size-world.arena.cursor;
        isle_span tail{};
        if (remaining) require(isle_arena_allocate(&world.arena,remaining,1,&tail)==ISLE_OK,
                               "could not reserve the remaining arena bytes");
        IsleGameCamera* failed = cam;
        require(isle_game_attach_camera(world.game, b, &failed) == ISLE_GAME_EXHAUSTED && !failed,
                "exhausted camera allocation published an owner");
        if (!allocate_matrices)
            require(isle_game_camera_add_matrix(world.game, cam, ISLE_GAME_CAMERA_PERSPECTIVE_FAST) == ISLE_GAME_EXHAUSTED &&
                    counts(world).matrices == 0, "failed matrix allocation published partial ownership");
        const auto full = world.arena.cursor;
        ok(isle_game_remove_camera(world.game, cam));
        cam = camera(world, b, allocate_matrices);
        require(world.arena.cursor == full, "camera/matrix pool reuse allocated in exhausted arena");
        if (allocate_matrices) {
            auto* reused_projection = matrix(world, cam, 0);
            auto* reused_view = matrix(world, cam, 1);
            require(first == reused_projection || first == reused_view, "matrix free list did not reuse storage");
            require(reused_projection->revision == 1, "recycled projection inherited a computed cache");
        }
        ok(isle_game_delete_object(world.game, b));
        require(counts(world).cameras == 0 && counts(world).matrices == 0, "reused camera cleanup leaked nodes");
    }
}

IsleGameModelTransform identity_model_transform() {
    return {{0, 0, 0}, {0, 0, 0}, {1, 1, 1}};
}

std::int32_t fixed_lane(const IsleGameFixedMatrix& m, std::size_t row, std::size_t col) {
    const auto pair = row * 2 + col / 2;
    const auto shift = col % 2 == 0 ? 16u : 0u;
    const auto bits = (((m.m[pair / 4][pair % 4] >> shift) & 0xffffu) << 16) |
        ((m.m[2 + pair / 4][pair % 4] >> shift) & 0xffffu);
    std::int32_t value;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

void model_contextual_pose_admission_and_cache() {
    World world;
    auto* owner = object(world, 1);
    auto pose = identity_model_transform();
    const float largest = std::numeric_limits<float>::max();
    pose.position = {largest, -largest, largest};
    pose.rotation = {-largest, largest, -largest};
    pose.scale = {largest, 0, -largest};
    std::array<IsleGameModelNodeInit, 2> initial{{
        {UINT32_MAX, pose, ISLE_GAME_MODEL_CONTEXTUAL_RPY},
        {0, identity_model_transform(), ISLE_GAME_MODEL_ROTATE_RPY_TRANSLATE_SCALE}
    }};
    IsleGameModel* model = nullptr;
    ok(isle_game_attach_model(world.game, owner, initial.data(), initial.size(), &model));
    const auto used = world.arena.cursor;
    const IsleGameModelNode* nodes = nullptr;
    std::size_t count = 0;
    ok(isle_game_model_peek_nodes(world.game, model, &nodes, &count));
    const auto* stable = nodes;
    const IsleGameFixedMatrix zero{};
    require(count == 2 && nodes[0].revision == 0 && nodes[0].first_child == 1 &&
        nodes[1].parent == 0 && nodes[0].matrix_kind == ISLE_GAME_MODEL_CONTEXTUAL_RPY &&
        std::memcmp(&nodes[0].fixed, &zero, sizeof(zero)) == 0,
        "contextual attachment fabricated fixed payload, lost kind or topology");
    std::feclearexcept(FE_ALL_EXCEPT);
    ok(isle_game_model_nodes(world.game, model, &nodes, &count));
    require(nodes == stable && nodes[0].revision == 1 && nodes[1].revision == 1 &&
        std::memcmp(&nodes[0].fixed, &zero, sizeof(zero)) == 0 &&
        std::fetestexcept(FE_INVALID | FE_DIVBYZERO | FE_OVERFLOW) == 0,
        "contextual getter evaluated old kind28/identity or rejected full finite pose");
    ok(isle_game_model_set_transform(world.game, model, 0, &pose));
    ok(isle_game_model_set_flags(world.game, model, 0, 3));
    ok(isle_game_model_nodes(world.game, model, &nodes, &count));
    require(nodes[0].revision == 1 && nodes[0].flags == 3,
        "identical contextual pose or flags dirtied accepted-pose revision");
    const auto accepted = nodes[0];
    for (auto channel : {&IsleGameModelTransform::position, &IsleGameModelTransform::rotation,
                         &IsleGameModelTransform::scale}) {
        for (auto component : {&IsleGameVec3::x, &IsleGameVec3::y, &IsleGameVec3::z}) {
            auto bad = pose;
            (bad.*channel).*component = std::numeric_limits<float>::quiet_NaN();
            require(isle_game_model_validate_transform(world.game, model, 0, &bad) ==
                    ISLE_GAME_INVALID_ARGUMENT &&
                isle_game_model_set_transform(world.game, model, 0, &bad) == ISLE_GAME_INVALID_ARGUMENT &&
                nodes[0].revision == accepted.revision &&
                std::memcmp(&nodes[0].transform, &accepted.transform, sizeof(pose)) == 0,
                "nonfinite contextual scalar was accepted or partially published");
        }
    }
    pose.position.x = 40000; pose.rotation.y = 1.0e20f; pose.scale.x = 0.999999f;
    ok(isle_game_model_validate_transform(world.game, model, 0, &pose));
    ok(isle_game_model_set_transform(world.game, model, 0, &pose));
    ok(isle_game_model_peek_nodes(world.game, model, &nodes, &count));
    require(nodes[0].revision == 1 && nodes[0].transform.position.x == 40000,
        "contextual peek evaluated pose or applied kind28 output limits");
    ok(isle_game_model_nodes(world.game, model, &nodes, &count));
    require(nodes == stable && nodes[0].revision == 2 && nodes[1].revision == 1 &&
        std::memcmp(&nodes[0].fixed, &zero, sizeof(zero)) == 0 && world.arena.cursor == used,
        "contextual evaluation allocated, fabricated fixed, or dirtied ordinary child");
    ok(isle_game_remove_model(world.game, model));
    ok(isle_game_attach_model(world.game, owner, initial.data(), 1, &model));
    ok(isle_game_model_nodes(world.game, model, &nodes, &count));
    require(nodes[0].revision == 1 && nodes[0].flags == 0 &&
        std::memcmp(&nodes[0].fixed, &zero, sizeof(zero)) == 0 && world.arena.cursor == used,
        "contextual block reuse inherited flags/revision/fixed or allocated");
}

void model_source54_math_and_atomicity() {
    auto pose = identity_model_transform();
    pose.position = {1.5f, -2.25f, 3.125f};
    pose.scale = {0.999999f, -0.2f, 1.25f};
    float inherited[3]{2, 3, 5};
    IsleGameFixedMatrix output{};
    ok(isle_model_source54_matrix(&pose, inherited, &output));
    require(fixed_lane(output, 0, 0) == 65535 && fixed_lane(output, 1, 1) == -13107 &&
        fixed_lane(output, 2, 2) == 81920 &&
        fixed_lane(output, 3, 0) == 98304 && fixed_lane(output, 3, 1) == -147456 &&
        fixed_lane(output, 3, 2) == 204800,
        "source54 quantized local scale or multiplied translation by inherited XYZ");
    pose.rotation = {0.37f, -0.63f, 0.91f};
    ok(isle_model_source54_matrix(&pose, inherited, &output));
    // Independent double RPY basis and coordinate-scale conjugation oracle;
    // conversion is binary32/truncation, not claimed original trig bit parity.
    const double sx = std::sin(double(pose.rotation.x)), cx = std::cos(double(pose.rotation.x));
    const double sy = std::sin(double(pose.rotation.y)), cy = std::cos(double(pose.rotation.y));
    const double sz = std::sin(double(pose.rotation.z)), cz = std::cos(double(pose.rotation.z));
    const double rotation[3][3]{{cy*cz, cy*sz, -sy},
        {sx*sy*cz-cx*sz, sx*sy*sz+cx*cz, sx*cy},
        {cx*sy*cz+sx*sz, cx*sy*sz-sx*cz, cx*cy}};
    const float scale[3]{pose.scale.x, pose.scale.y, pose.scale.z};
    for (std::size_t r = 0; r < 3; ++r)
        for (std::size_t c = 0; c < 3; ++c) {
            const double expected = rotation[r][c] * scale[r] * inherited[r] / inherited[c];
            require(std::abs(double(fixed_lane(output, r, c)) - expected * 65536.0) < 1.1,
                "source54 lost RPY order or unequal inherited-axis conjugation");
        }
    // Check the actual reciprocal/multiply order at a sensitive off-diagonal.
    const float exact01 = std::cos(pose.rotation.y) * std::sin(pose.rotation.z) *
        pose.scale.x * inherited[0] * (1.0f / inherited[1]);
    require(fixed_lane(output, 0, 1) == static_cast<std::int32_t>(exact01 * 65536.0f),
        "source54 reassociated reciprocal/expression evaluation");
    for (std::size_t row = 0; row < 4; ++row)
        require(fixed_lane(output, row, 3) == (row == 3 ? 65536 : 0),
            "source54 did not preserve exact fixed-W column");
    const auto valid = output;
    for (unsigned error = 0; error < 8; ++error) {
        auto bad = pose;
        float scales[3]{2, 3, 5};
        if (error == 0) scales[0] = 0;
        if (error == 1) scales[1] = std::numeric_limits<float>::denorm_min();
        if (error == 2) scales[2] = std::numeric_limits<float>::infinity();
        if (error == 3) bad.position.z = 32768;
        if (error == 4) bad.rotation.x = std::numeric_limits<float>::quiet_NaN();
        if (error == 5) bad.scale.y = std::numeric_limits<float>::max();
        const auto result = isle_model_source54_matrix(error == 6 ? nullptr : &bad,
            error == 7 ? nullptr : scales, &output);
        require(result == ISLE_GAME_INVALID_ARGUMENT &&
            std::memcmp(&output, &valid, sizeof(output)) == 0,
            "source54 rejection changed output or repaired invalid inherited scale");
    }
    require(isle_model_source54_matrix(&pose, inherited, nullptr) == ISLE_GAME_INVALID_ARGUMENT,
        "source54 accepted absent output");
    pose = identity_model_transform();
    pose.rotation = {std::numeric_limits<float>::max(), -1.0e20f, 1.0e10f};
    ok(isle_model_source54_matrix(&pose, inherited, &output));
    require(fixed_lane(output, 3, 3) == 65536,
        "source54 imposed kind28 angle-cast limits on finite libm angles");
    pose = identity_model_transform(); pose.scale = {0, 0, 0};
    ok(isle_model_source54_matrix(&pose, inherited, &output));
    require(fixed_lane(output, 0, 0) == 0 && fixed_lane(output, 3, 3) == 65536,
        "source54 rejected source-valid local zero scale");
    inherited[1] = 0;
    require(isle_model_source54_matrix(&pose, inherited, &output) == ISLE_GAME_INVALID_ARGUMENT,
        "local zero incorrectly rescued source54 zero inherited scale");
    inherited[1] = -3; pose = identity_model_transform();
    pose.position.x = -32768;
    ok(isle_model_source54_matrix(&pose, inherited, &output));
    require(fixed_lane(output, 3, 0) == INT32_MIN, "source54 rejected signed fixed lower endpoint");
}

void model_source48_basis_and_atomicity() {
    IsleGameCameraParameters camera{};
    camera.look_at = {{0, 0, 5}, {0, 0, 0}, {0, 1, 0}};
    float perspective[4][4]{{2, 0, 0, 0}, {0, 3, 0, 0},
        {0, 0, -1.25f, -1}, {0, 0, -7, 0}};
    float basis[4][4]{};
    ok(isle_model_source48_basis(&camera, perspective, 1, basis));
    for (std::size_t r = 0; r < 3; ++r)
        for (std::size_t c = 0; c < 4; ++c)
            require(basis[r][c] == perspective[r][c], "source48 did not concatenate synthetic look-at");
    require(basis[3][2] == -0.75f && basis[3][3] == 5,
        "source48 synthetic eyeZ or row-vector multiplication differs");
    camera.look_at = {{0, 3, 4}, {0, 0, 0}, {0, 0, 0}};
    ok(isle_model_source48_basis(&camera, perspective, 1, basis));
    close_to(basis[1][1], 2.4f); close_to(basis[1][2], -0.75f);
    close_to(basis[2][1], -1.8f); close_to(basis[2][2], -1.0f);
    float other_axis[4][4]{};
    ok(isle_model_source48_basis(&camera, perspective, 2, other_axis));
    close_to(other_axis[1][1], 1.8f); close_to(other_axis[2][1], -2.4f);
    float aliased[4][4]; std::memcpy(aliased, perspective, sizeof(aliased));
    ok(isle_model_source48_basis(&camera, aliased, 1, aliased));
    require(std::memcmp(aliased, basis, sizeof(basis)) == 0,
        "source48 perspective/output alias changed result");
    float saved[4][4]; std::memcpy(saved, basis, sizeof(saved));
    for (unsigned error = 0; error < 6; ++error) {
        auto bad = camera;
        float projection[4][4]; std::memcpy(projection, perspective, sizeof(projection));
        if (error == 0) bad.look_at.eye[0] = std::numeric_limits<float>::infinity();
        if (error == 1) bad.look_at.eye[1] = std::numeric_limits<float>::quiet_NaN();
        if (error == 2) projection[2][3] = std::numeric_limits<float>::max();
        if (error == 2) projection[1][3] = std::numeric_limits<float>::max();
        const auto result = isle_model_source48_basis(error == 3 ? nullptr : &bad,
            error == 4 ? nullptr : projection, error == 5 ? 0u : 1u, basis);
        require(result == ISLE_GAME_INVALID_ARGUMENT &&
            std::memcmp(basis, saved, sizeof(basis)) == 0,
            "source48 invalid consumed math changed output");
    }
    require(isle_model_source48_basis(&camera, perspective, 3, basis) == ISLE_GAME_INVALID_ARGUMENT &&
        isle_model_source48_basis(&camera, perspective, 1, nullptr) == ISLE_GAME_INVALID_ARGUMENT,
        "source48 accepted unknown axis or absent output");
    // Degenerate source branch bypasses perspective and vertical pose entirely.
    camera.look_at.eye[0] = camera.look_at.at[0] = 0;
    camera.look_at.eye[2] = 0; camera.look_at.at[2] = 0.00001f;
    camera.look_at.eye[1] = std::numeric_limits<float>::quiet_NaN();
    perspective[0][0] = std::numeric_limits<float>::quiet_NaN();
    ok(isle_model_source48_basis(&camera, perspective, 1, basis));
    for (std::size_t r = 0; r < 4; ++r)
        for (std::size_t c = 0; c < 4; ++c)
            require(basis[r][c] == (r == 3 && c == 3 ? 1.0f : 0.0f),
                "source48 degenerate basis concatenated perspective or fabricated identity");
    // Row3 is not consumed by recipe48, even if its concat overflows.
    const float largest = std::numeric_limits<float>::max();
    camera.look_at = {{0, largest, 4}, {0, largest, 0}, {0, 1, 0}};
    float diagonal[4][4]{{1,0,0,0},{0,2,0,0},{0,0,1,0},{0,0,0,1}};
    ok(isle_model_source48_basis(&camera, diagonal, 1, basis));
    require(basis[0][0] == 1 && basis[1][1] == 2 && std::isinf(basis[3][1]),
        "source48 invented finite restrictions for unused basis row3");
}

void model_hierarchy_math_and_cache() {
    World world;
    auto* owner = object(world, 1);
    std::array<IsleGameModelNodeInit, 5> initial{};
    for (auto& node : initial) {
        node.transform = identity_model_transform();
        node.matrix_kind = ISLE_GAME_MODEL_ROTATE_RPY_TRANSLATE_SCALE;
    }
    initial[0].parent = UINT32_MAX;
    initial[1].parent = 0; initial[2].parent = 1;
    initial[3].parent = 0; initial[4].parent = 3;
    initial[2].transform.scale = {0.999999f, -0.2f, 1.25f};
    initial[2].transform.position = {1.5f, -2.25f, 3.125f};
    IsleGameModel* model = nullptr;
    ok(isle_game_attach_model(world.game, owner, initial.data(), initial.size(), &model));
    auto* cam_owner = object(world, 2);
    camera(world, cam_owner);
    const auto live = counts(world);
    require(live.models == 1 && live.model_nodes == 5 && live.matrices == 7,
            "model and camera matrix ownership was omitted or double counted");
    const IsleGameModelNode* nodes = nullptr;
    std::size_t count = 0;
    ok(isle_game_model_nodes(world.game, model, &nodes, &count));
    require(count == 5 && nodes[0].parent == UINT32_MAX && nodes[0].first_child == 1 &&
        nodes[1].next_sibling == 3 && nodes[1].first_child == 2 && nodes[3].first_child == 4 &&
        nodes[2].next_sibling == UINT32_MAX && nodes[4].first_child == UINT32_MAX,
        "parent-before-child hierarchy lost root, children or sibling order");
    require(fixed_lane(nodes[2].fixed, 0, 0) == 65280 &&
        fixed_lane(nodes[2].fixed, 1, 1) == -13056 &&
        fixed_lane(nodes[2].fixed, 2, 2) == 81920 &&
        fixed_lane(nodes[2].fixed, 3, 0) == 98304 &&
        fixed_lane(nodes[2].fixed, 3, 1) == -147456 &&
        fixed_lane(nodes[2].fixed, 3, 2) == 204800,
        "kind28 rounded scale to one or used float-to-fixed TRS instead of integer scales");
    const auto used = world.arena.cursor;
    const auto original = nodes[2].fixed;
    for (std::size_t i = 0; i < count; ++i) {
        ok(isle_game_model_set_transform(world.game, model, i, &initial[i].transform));
        ok(isle_game_model_set_flags(world.game, model, i, 3));
    }
    const IsleGameModelNode* again = nullptr;
    ok(isle_game_model_nodes(world.game, model, &again, &count));
    require(again == nodes && world.arena.cursor == used && nodes[2].revision == 1 &&
        std::memcmp(&nodes[2].fixed, &original, sizeof(original)) == 0,
        "unchanged transforms or flags allocated/recomputed/moved model storage");
    auto root = initial[0].transform;
    root.position.x = 9;
    ok(isle_game_model_set_transform(world.game, model, 0, &root));
    ok(isle_game_model_nodes(world.game, model, &again, &count));
    require(nodes[0].revision == 2 && nodes[1].revision == 1 && nodes[2].revision == 1,
        "root update recomputed child local matrices");
    auto turn = identity_model_transform();
    turn.rotation.y = 1.57079637f;
    turn.scale.x = 0.999999f;
    ok(isle_game_model_set_transform(world.game, model, 1, &turn));
    ok(isle_game_model_nodes(world.game, model, &again, &count));
    require(fixed_lane(nodes[1].fixed, 0, 2) == -65280 &&
        fixed_lane(nodes[1].fixed, 2, 0) == 65536 &&
        fixed_lane(nodes[1].fixed, 0, 0) == 0,
        "GAME table-angle quantization or signed scale arithmetic differs");
    turn = identity_model_transform();
    // ROM-observed binary32 MUL.S gives table index 14 here; evaluating the
    // unsuffixed mathematical PI ratio in double incorrectly gives index 15.
    turn.rotation.y = 0x1.78fdbap-6f;
    ok(isle_game_model_set_transform(world.game, model, 1, &turn));
    ok(isle_game_model_nodes(world.game, model, &again, &count));
    require(fixed_lane(nodes[1].fixed, 0, 2) == -1406 &&
        fixed_lane(nodes[1].fixed, 2, 0) == 1406,
        "kind28 angle quantization did not retain actual GAME binary32 multiplication");
    turn = identity_model_transform();
    turn.scale.x = 128; // GAME MULTU/MFLO + SRA retains the low product word.
    ok(isle_game_model_set_transform(world.game, model, 1, &turn));
    ok(isle_game_model_nodes(world.game, model, &again, &count));
    require(fixed_lane(nodes[1].fixed, 0, 0) == -8388608,
        "kind28 lost defined MIPS low-word multiplication semantics");
    require(world.arena.cursor == used, "model mutations allocated");
}

void model_placement_atomicity_and_lifecycle() {
    World world;
    auto* owner = object(world, 1);
    auto* replacement = object(world, 2);
    std::array<IsleGameModelNodeInit, 4> initial{};
    for (std::size_t i = 0; i < initial.size(); ++i)
        initial[i] = {i ? 0u : UINT32_MAX, identity_model_transform(),
            ISLE_GAME_MODEL_ROTATE_RPY_TRANSLATE_SCALE};
    initial[0].transform.position = {7, -3, 2};
    initial[0].transform.rotation = {0.3f, -0.7f, 0.2f};
    IsleGameModel* model = nullptr;
    ok(isle_game_attach_model(world.game, owner, initial.data(), initial.size(), &model));
    const IsleGameModelNode* nodes = nullptr;
    std::size_t count = 0;
    ok(isle_game_model_nodes(world.game, model, &nodes, &count));
    const std::vector<IsleGameModelNode> original(nodes, nodes + count);
    const IsleGameModelPlacement* view = nullptr;
    ok(isle_game_model_placement(world.game, model, &view));
    const auto* stable = view;
    const auto used = world.arena.cursor;
    require(view->mode == ISLE_GAME_MODEL_PLACEMENT_DISABLED && view->revision == 0 &&
        view->transform.scale.x == 1 && counts(world).matrices == 4,
        "new model did not start placement-disabled");
    ok(isle_game_model_set_placement(world.game, model, ISLE_GAME_MODEL_PLACEMENT_DISABLED, nullptr));
    require(view->revision == 0, "disabled no-op advanced placement revision");
    auto t = identity_model_transform();
    t.position = {11, -13, 17};
    t.rotation.y = 1.57079637f;
    t.scale = {std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::infinity(), std::numeric_limits<float>::max()};
    ok(isle_game_model_validate_placement(world.game, model, ISLE_GAME_MODEL_PLACEMENT_RT, &t));
    require(view->revision == 0 && counts(world).matrices == 4 && world.arena.cursor == used,
        "placement preflight published state or allocated");
    ok(isle_game_model_set_placement(world.game, model, ISLE_GAME_MODEL_PLACEMENT_RT, &t));
    require(view == stable && view->revision == 1 && counts(world).matrices == 5 &&
        view->transform.scale.x == 1 && view->transform.scale.y == 1 && view->transform.scale.z == 1 &&
        fixed_lane(view->fixed, 0, 2) == -65536 && fixed_lane(view->fixed, 2, 0) == 65536 &&
        fixed_lane(view->fixed, 3, 0) == 11 * 65536,
        "RT consumed unused scale, lost direct RT arithmetic, moved borrow or miscounted");
    const auto rt = *view;
    t.scale = {-33, 0, 8388608};
    ok(isle_game_model_set_placement(world.game, model, ISLE_GAME_MODEL_PLACEMENT_RT, &t));
    require(view->revision == rt.revision &&
        std::memcmp(&view->fixed, &rt.fixed, sizeof(rt.fixed)) == 0,
        "RT unused scale invalidated placement");
    t.scale = {-2, 3.5f, 0};
    ok(isle_game_model_set_placement(world.game, model, ISLE_GAME_MODEL_PLACEMENT_RTS, &t));
    require(view->revision == 2 && counts(world).matrices == 5 &&
        fixed_lane(view->fixed, 0, 2) == 131072 &&
        fixed_lane(view->fixed, 1, 1) == 229376 && fixed_lane(view->fixed, 2, 0) == 0,
        "RTS negative/nonuniform/zero scales or enabled-mode count differ");
    const auto accepted = *view;
    for (unsigned channel = 0; channel < 6; ++channel) {
        auto bad = t;
        if (channel == 0) bad.position.x = std::numeric_limits<float>::quiet_NaN();
        if (channel == 1) bad.position.y = 32768;
        if (channel == 2) bad.rotation.z = std::numeric_limits<float>::infinity();
        if (channel == 3) bad.rotation.x = std::numeric_limits<float>::max();
        if (channel == 4) bad.scale.x = 8388608;
        if (channel == 5) bad.scale.z = std::numeric_limits<float>::quiet_NaN();
        require(isle_game_model_validate_placement(world.game, model, ISLE_GAME_MODEL_PLACEMENT_RTS, &bad) ==
            ISLE_GAME_INVALID_ARGUMENT &&
            isle_game_model_set_placement(world.game, model, ISLE_GAME_MODEL_PLACEMENT_RTS, &bad) ==
            ISLE_GAME_INVALID_ARGUMENT, "invalid placement reached GAME casts");
        require(view->revision == accepted.revision && view->mode == accepted.mode &&
            std::memcmp(&view->fixed, &accepted.fixed, sizeof(accepted.fixed)) == 0 &&
            std::memcmp(&view->transform, &accepted.transform, sizeof(accepted.transform)) == 0 &&
            counts(world).matrices == 5 && world.arena.cursor == used,
            "rejected placement partially published accepted state/counts");
    }
    require(engine_set_invalid_placement(world.game, model) == ISLE_GAME_INVALID_ARGUMENT &&
        isle_game_model_set_placement(world.game, model, ISLE_GAME_MODEL_PLACEMENT_RT, nullptr) ==
        ISLE_GAME_INVALID_ARGUMENT && isle_game_model_placement(world.game, model, nullptr) ==
        ISLE_GAME_INVALID_ARGUMENT, "unknown mode or null consumed input/output accepted");
    bool rejected_owner = false;
    std::thread foreign([&] {
        const IsleGameModelPlacement* output = stable;
        rejected_owner = isle_game_model_placement(world.game, model, &output) == ISLE_GAME_WRONG_OWNER &&
            output == nullptr &&
            isle_game_model_validate_placement(world.game, model, ISLE_GAME_MODEL_PLACEMENT_RTS, &t) ==
                ISLE_GAME_WRONG_OWNER &&
            isle_game_model_set_placement(world.game, model, ISLE_GAME_MODEL_PLACEMENT_DISABLED, nullptr) ==
                ISLE_GAME_WRONG_OWNER;
    });
    foreign.join();
    require(rejected_owner && view->revision == accepted.revision && counts(world).matrices == 5,
        "foreign placement access mutated state");
    ok(isle_game_model_nodes(world.game, model, &nodes, &count));
    for (std::size_t n = 0; n < count; ++n)
        require(nodes[n].revision == original[n].revision &&
            std::memcmp(&nodes[n].fixed, &original[n].fixed, sizeof(nodes[n].fixed)) == 0 &&
            nodes[n].parent == original[n].parent && nodes[n].first_child == original[n].first_child &&
            nodes[n].next_sibling == original[n].next_sibling,
            "placement changed ordinary local caches or source indices");
    auto local = initial[0].transform;
    local.position.x += 1;
    ok(isle_game_model_set_transform(world.game, model, 0, &local));
    ok(isle_game_model_nodes(world.game, model, &nodes, &count));
    require(view->revision == accepted.revision && nodes[0].revision == original[0].revision + 1,
        "ordinary local update dirtied placement");
    ok(isle_game_model_set_placement(world.game, model, ISLE_GAME_MODEL_PLACEMENT_DISABLED, nullptr));
    require(view->revision == 3 && view->transform.position.x == 0 && view->transform.rotation.y == 0 &&
        view->transform.scale.x == 1 && counts(world).matrices == 4,
        "disabled placement did not restore neutral metadata/count");
    const IsleGameFixedMatrix zero{};
    require(std::memcmp(&view->fixed, &zero, sizeof(zero)) == 0,
        "disabled placement computed/stored an identity payload");
    ok(isle_game_model_set_placement(world.game, model, ISLE_GAME_MODEL_PLACEMENT_RTS, &t));
    ok(isle_game_remove_model(world.game, model));
    require(counts(world).matrices == 0 && counts(world).model_nodes == 0,
        "active placement removal leaked/double-subtracted a matrix");
    view = stable;
    require(isle_game_model_placement(world.game, model, &view) == ISLE_GAME_INVALID_ARGUMENT && !view,
        "removed placement borrow remained accessible");
    IsleGameModel* reused = nullptr;
    ok(isle_game_attach_model(world.game, replacement, initial.data(), 1, &reused));
    ok(isle_game_model_placement(world.game, reused, &view));
    require(reused == model && view == stable && view->revision == 0 &&
        view->mode == ISLE_GAME_MODEL_PLACEMENT_DISABLED && counts(world).matrices == 1 &&
        world.arena.cursor == used, "smaller best-fit reuse inherited placement or allocated");
    ok(isle_game_model_nodes(world.game, reused, &nodes, &count));
    require(count == 1 && nodes[0].revision == 1, "capacity-tail dirty storage lost on smaller reuse");
    ok(isle_game_model_set_placement(world.game, reused, ISLE_GAME_MODEL_PLACEMENT_RT, &t));
    ok(isle_game_delete_object(world.game, replacement));
    require(counts(world).matrices == 0, "reused active placement retirement leaked");
}

void model_translate_selection_and_cache() {
    World world;
    auto* owner = object(world, 1);
    std::array<IsleGameModelNodeInit, 2> initial{{
        {UINT32_MAX, identity_model_transform(), ISLE_GAME_MODEL_TRANSLATE},
        {0, identity_model_transform(), ISLE_GAME_MODEL_ROTATE_RPY_TRANSLATE_SCALE}
    }};
    initial[0].transform.position = {1.5f, -2.25f, 3.125f};
    initial[0].transform.rotation = {0.25f, -0.75f, 1.5f};
    initial[0].transform.scale = {-2, 3, 0.5f};
    auto scalar = [](IsleGameModelTransform& pose, unsigned channel) -> float& {
        IsleGameVec3& vector = channel < 3 ? pose.position : channel < 6 ? pose.rotation : pose.scale;
        return channel % 3 == 0 ? vector.x : channel % 3 == 1 ? vector.y : vector.z;
    };
    const auto arena_before = world.arena.cursor;
    IsleGameModel* model = nullptr;
    auto rejected = initial;
    require(engine_attach_model_with_invalid_kind(world.game, owner, initial.data(), &model) ==
            ISLE_GAME_INVALID_ARGUMENT && !model && world.arena.cursor == arena_before &&
            counts(world).models == 0, "unknown matrix kind mutated model ownership/arena");
    for (unsigned channel = 0; channel < 18; ++channel) {
        rejected = initial;
        scalar(rejected[0].transform, channel % 9) = channel < 9 ?
            std::numeric_limits<float>::quiet_NaN() : std::numeric_limits<float>::infinity();
        require(isle_game_attach_model(world.game, owner, rejected.data(), rejected.size(), &model) ==
                ISLE_GAME_INVALID_ARGUMENT && !model && world.arena.cursor == arena_before &&
                counts(world).models == 0, "invalid translate attachment changed ownership/arena");
    }
    rejected = initial;
    rejected[0].transform.position.x = 32768;
    require(isle_game_attach_model(world.game, owner, rejected.data(), rejected.size(), &model) ==
            ISLE_GAME_INVALID_ARGUMENT && !model && world.arena.cursor == arena_before &&
            counts(world).models == 0, "translate attachment overflow changed ownership/arena");
    ok(isle_game_attach_model(world.game, owner, initial.data(), initial.size(), &model));
    const IsleGameModelNode* nodes = nullptr;
    std::size_t extent = 0;
    ok(isle_game_model_nodes(world.game, model, &nodes, &extent));
    require(extent == 2 && nodes[0].matrix_kind == ISLE_GAME_MODEL_TRANSLATE &&
            nodes[1].matrix_kind == ISLE_GAME_MODEL_ROTATE_RPY_TRANSLATE_SCALE,
            "mixed native model matrix selections were lost");
    const auto* stable = nodes;
    const auto used = world.arena.cursor;
    const auto child_revision = nodes[1].revision;
    const auto child_before = nodes[1];
    require(std::memcmp(&nodes[0].transform, &initial[0].transform, sizeof(initial[0].transform)) == 0,
            "translate attachment discarded canonical rotation/scale");
    const std::array<std::int32_t, 3> expected_translation{98304, -147456, 204800};
    for (std::size_t row = 0; row < 4; ++row)
        for (std::size_t col = 0; col < 4; ++col)
            require(fixed_lane(nodes[0].fixed, row, col) ==
                    (row == 3 && col < 3 ? expected_translation[col] : (row == col ? 65536 : 0)),
                    "translate matrix is not exact identity plus source FTOFIX32 translation");
    const auto initial_fixed = nodes[0].fixed;
    const auto initial_revision = nodes[0].revision;
    ok(isle_game_model_set_transform(world.game, model, 0, &initial[0].transform));
    ok(isle_game_model_set_flags(world.game, model, 0, 1));
    ok(isle_game_model_nodes(world.game, model, &nodes, &extent));
    require(nodes == stable && nodes[0].revision == initial_revision && nodes[1].revision == child_revision &&
            world.arena.cursor == used, "unchanged translate/flags invalidated cache or replaced native borrow");
    for (unsigned channel = 0; channel < 19; ++channel) {
        auto invalid = initial[0].transform;
        if (channel < 18)
            scalar(invalid, channel % 9) = channel < 9 ?
                std::numeric_limits<float>::quiet_NaN() : std::numeric_limits<float>::infinity();
        else invalid.position.x = 32768;
        require(isle_game_model_validate_transform(world.game, model, 0, &invalid) == ISLE_GAME_INVALID_ARGUMENT,
                "translate preflight accepted nonfinite/overflow pose");
        require(isle_game_model_set_transform(world.game, model, 0, &invalid) == ISLE_GAME_INVALID_ARGUMENT,
                "invalid translate mutation accepted");
        ok(isle_game_model_nodes(world.game, model, &nodes, &extent));
        require(nodes == stable && nodes[0].revision == initial_revision &&
                std::memcmp(&nodes[0].fixed, &initial_fixed, sizeof(initial_fixed)) == 0 &&
                std::memcmp(&nodes[0].transform, &initial[0].transform, sizeof(initial[0].transform)) == 0 &&
                std::memcmp(&nodes[1], &child_before, sizeof(child_before)) == 0 && world.arena.cursor == used,
                "rejected translate mutation dirtied state/cache or allocated");
    }
    auto boundary = initial[0].transform;
    boundary.position = {-32768.0f, std::nextafter(32768.0f, 0.0f), -0.5f / 65536.0f};
    ok(isle_game_model_set_transform(world.game, model, 0, &boundary));
    ok(isle_game_model_nodes(world.game, model, &nodes, &extent));
    require(nodes == stable && nodes[0].revision == initial_revision + 1 &&
            nodes[1].revision == child_revision && world.arena.cursor == used &&
            fixed_lane(nodes[0].fixed, 3, 0) == std::numeric_limits<std::int32_t>::min() &&
            fixed_lane(nodes[0].fixed, 3, 1) == 2147483520 &&
            fixed_lane(nodes[0].fixed, 3, 2) == 0,
            "translate boundary/truncate/cache/selective invalidation semantics differ from source");
    ok(isle_game_model_set_transform(world.game, model, 0, &initial[0].transform));
    ok(isle_game_model_nodes(world.game, model, &nodes, &extent));
    require(nodes[0].revision == initial_revision + 2 && nodes[1].revision == child_revision &&
            std::memcmp(&nodes[0].fixed, &initial_fixed, sizeof(initial_fixed)) == 0,
            "restored translate input failed exact fixed repeat");
    auto canonical = initial[0].transform;
    canonical.rotation = {-std::numeric_limits<float>::max(), 0.125f, std::numeric_limits<float>::max()};
    canonical.scale = {std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), 0};
    const auto before_edit_revision = nodes[0].revision;
    for (unsigned channel = 3; channel < 9; ++channel) {
        auto edited = nodes[0].transform;
        scalar(edited, channel) = scalar(canonical, channel);
        const auto revision = nodes[0].revision;
        ok(isle_game_model_validate_transform(world.game, model, 0, &edited));
        ok(isle_game_model_set_transform(world.game, model, 0, &edited));
        require(nodes[0].revision == revision &&
                std::memcmp(&nodes[0].fixed, &initial_fixed, sizeof(initial_fixed)) == 0,
                "rotation/scale edit eagerly evaluated translation cache");
        ok(isle_game_model_peek_nodes(world.game, model, &nodes, &extent));
        require(nodes == stable && nodes[0].revision == revision &&
                std::memcmp(&nodes[0].transform, &edited, sizeof(edited)) == 0,
                "pure peek lost canonical pose or evaluated revision");
        ok(isle_game_model_nodes(world.game, model, &nodes, &extent));
        require(nodes[0].revision == revision + 1 &&
                std::memcmp(&nodes[0].fixed, &initial_fixed, sizeof(initial_fixed)) == 0 &&
                std::memcmp(&nodes[1], &child_before, sizeof(child_before)) == 0 &&
                world.arena.cursor == used,
                "rotation/scale-only edit changed translation bytes, child, arena, or lazy revision");
        ok(isle_game_model_nodes(world.game, model, &nodes, &extent));
        require(nodes[0].revision == revision + 1, "repeat cache read reevaluated canonical pose");
    }
    require(nodes[0].revision == before_edit_revision + 6 &&
            std::memcmp(&nodes[0].transform, &canonical, sizeof(canonical)) == 0,
            "finite huge rotation/scale was narrowed or discarded");
    ok(isle_game_delete_object(world.game, owner));
    require(counts(world).models == 0 && counts(world).model_nodes == 0 && counts(world).matrices == 0,
            "mixed-kind model ownership did not retire");
}

void model_photo_bake() {
    auto root = identity_model_transform();
    root.position = {10, 20, 30};
    root.rotation = {0.25f, 0.5f, -0.75f};
    root.scale = {0.999999f, 2, -3};
    const IsleGameVec3 world{100, 200, 300};
    IsleGameModelTransform baked{}, repeated{};
    ok(isle_game_model_bake_photo_transform(&root, &world, 1.57079637f, 2, &baked));
    close_to(baked.position.x, 106);
    close_to(baked.position.y, 204);
    close_to(baked.position.z, 298);
    close_to(baked.rotation.x, root.rotation.x);
    close_to(baked.rotation.y, root.rotation.y + 1.57079637f);
    close_to(baked.rotation.z, root.rotation.z);
    require(baked.scale.x == root.scale.x * 0.2f && baked.scale.y == 0.4f &&
        baked.scale.z == -0.6f, "photo Pidgey scale did not use 2*0.1 binary32 order");
    ok(isle_game_model_bake_photo_transform(&root, &world, 1.57079637f, 2, &repeated));
    require(std::memcmp(&baked, &repeated, sizeof(baked)) == 0,
        "repeated unbaked photo input accumulated the world transform");
    auto aliased = root;
    ok(isle_game_model_bake_photo_transform(&aliased, &world, 1.57079637f, 2, &aliased));
    require(std::memcmp(&baked, &aliased, sizeof(baked)) == 0, "photo output/root alias changed the result");
    auto invalid = root;
    invalid.scale.x = std::numeric_limits<float>::infinity();
    require(isle_game_model_bake_photo_transform(&invalid, &world, 0, 2, &repeated) ==
        ISLE_GAME_INVALID_ARGUMENT && repeated.scale.x == 0 && repeated.position.x == 0,
        "invalid photo root leaked output");
}

void model_rejection_atomicity_and_reuse() {
    World world(8192);
    auto* owner = object(world, 1);
    auto* other = object(world, 2);
    IsleGameModelNodeInit initial{UINT32_MAX, identity_model_transform(), ISLE_GAME_MODEL_ROTATE_RPY_TRANSLATE_SCALE};
    IsleGameModel* model = nullptr;
    const auto before = world.arena.cursor;
    auto invalid = initial;
    invalid.parent = 0;
    require(isle_game_attach_model(world.game, owner, &invalid, 1, &model) ==
        ISLE_GAME_INVALID_ARGUMENT && !model && world.arena.cursor == before,
        "invalid root attachment changed arena or published a partial model");
    std::array<IsleGameModelNodeInit, 2> bad_tree{initial, initial};
    require(isle_game_attach_model(world.game, owner, bad_tree.data(), bad_tree.size(), &model) ==
        ISLE_GAME_INVALID_ARGUMENT && !model && world.arena.cursor == before,
        "multiple roots accepted");
    bad_tree[1].parent = 2;
    require(isle_game_attach_model(world.game, owner, bad_tree.data(), bad_tree.size(), &model) ==
        ISLE_GAME_INVALID_ARGUMENT && !model, "forward/cyclic parent accepted");
    ok(isle_game_attach_model(world.game, owner, &initial, 1, &model));
    IsleGameCamera* rejected_camera = nullptr;
    require(isle_game_attach_camera(world.game, owner, &rejected_camera) == ISLE_GAME_INVALID_STATE &&
        !rejected_camera, "camera overwrote model payload");
    IsleGameSprite* rejected_sprite = nullptr;
    require(isle_game_add_sprite(world.game, owner, nullptr, &rejected_sprite) == ISLE_GAME_INVALID_ARGUMENT &&
        !rejected_sprite, "sprite interpreted native model as sprite list");
    IsleGameModel* rejected = model;
    require(isle_game_attach_model(world.game, owner, &initial, 1, &rejected) ==
        ISLE_GAME_INVALID_STATE && !rejected, "second model overwrote object");
    const IsleGameModelNode* nodes = nullptr;
    std::size_t extent = 0;
    ok(isle_game_model_nodes(world.game, model, &nodes, &extent));
    const auto original = nodes[0].fixed;
    for (auto value : {std::numeric_limits<float>::quiet_NaN(),
                       std::numeric_limits<float>::infinity(), 32768.0f}) {
        auto t = initial.transform;
        t.position.x = value;
        require(isle_game_model_set_transform(world.game, model, 0, &t) == ISLE_GAME_INVALID_ARGUMENT,
            "nonfinite or unrepresentable position reached fixed cast");
    }
    auto t = initial.transform;
    t.rotation.y = std::numeric_limits<float>::max();
    require(isle_game_model_set_transform(world.game, model, 0, &t) == ISLE_GAME_INVALID_ARGUMENT,
        "angle product outside signed word reached cast");
    t = initial.transform; t.scale.z = 8388608.0f;
    require(isle_game_model_set_transform(world.game, model, 0, &t) == ISLE_GAME_INVALID_ARGUMENT,
        "scale product outside signed word reached cast");
    t = initial.transform; t.position.x = -32768;
    ok(isle_game_model_set_transform(world.game, model, 0, &t));
    ok(isle_game_model_nodes(world.game, model, &nodes, &extent));
    require(fixed_lane(nodes[0].fixed, 3, 0) == INT32_MIN, "legal negative fixed boundary rejected");
    ok(isle_game_model_set_transform(world.game, model, 0, &initial.transform));
    ok(isle_game_model_nodes(world.game, model, &nodes, &extent));
    require(std::memcmp(&nodes[0].fixed, &original, sizeof(original)) == 0,
        "restoring original model input did not restore fixed bytes");
    bool wrong_owner = false;
    std::thread foreign([&] {
        const IsleGameModelNode* output = nodes;
        std::size_t count = 7;
        IsleGameModel* attached = model;
        wrong_owner = isle_game_model_nodes(world.game, model, &output, &count) == ISLE_GAME_WRONG_OWNER &&
            !output && count == 0 &&
            isle_game_attach_model(world.game, other, &initial, 1, &attached) == ISLE_GAME_WRONG_OWNER &&
            !attached && isle_game_remove_model(world.game, model) == ISLE_GAME_WRONG_OWNER &&
            isle_game_model_set_flags(world.game, model, 0, 1) == ISLE_GAME_WRONG_OWNER &&
            isle_game_model_set_transform(world.game, model, 0, &initial.transform) == ISLE_GAME_WRONG_OWNER;
    });
    foreign.join();
    require(wrong_owner, "native model admitted foreign creator-domain access");
    std::vector<IsleGameObject*> fillers;
    for (;;) {
        IsleGameObject* extra = nullptr;
        auto result = isle_game_add_object(world.game, 3, 0, 0, nullptr, nullptr, &extra);
        if (result == ISLE_GAME_EXHAUSTED) break;
        ok(result); fillers.push_back(extra);
    }
    const auto exhausted = world.arena.cursor;
    rejected = model;
    require(isle_game_attach_model(world.game, other, &initial, 1, &rejected) == ISLE_GAME_EXHAUSTED &&
        !rejected && counts(world).models == 1 && counts(world).model_nodes == 1 &&
        world.arena.cursor == exhausted, "exhausted attachment published state or consumed arena");
    ok(isle_game_remove_model(world.game, model));
    nodes = reinterpret_cast<const IsleGameModelNode*>(std::uintptr_t{1}); extent = 9;
    require(isle_game_model_nodes(world.game, model, &nodes, &extent) == ISLE_GAME_INVALID_ARGUMENT &&
        !nodes && extent == 0, "removed model remained borrowable");
    IsleGameModel* reused = nullptr;
    ok(isle_game_attach_model(world.game, other, &initial, 1, &reused));
    require(reused == model && world.arena.cursor == exhausted, "released model block was not reused");
    ok(isle_game_model_nodes(world.game, reused, &nodes, &extent));
    require(nodes[0].revision == 1 && nodes[0].flags == 0, "reused model inherited cache/flags");
    ok(isle_game_delete_object(world.game, other));
    require(counts(world).models == 0 && counts(world).model_nodes == 0 && counts(world).matrices == 0,
        "typed object deletion leaked model ownership");
}

void observe_model_retirement(World& world, IsleGameObject* owner, EngineFixture& fixture) {
    fixture.game = world.game;
    engine_bind_source_payload(owner, &fixture);
    IsleGameModelNodeInit initial{UINT32_MAX, identity_model_transform(), ISLE_GAME_MODEL_CONTEXTUAL_RPY};
    initial.transform.position = {7, -3, 11};
    ok(isle_game_attach_model(world.game, owner, &initial, 1, &fixture.model));
    ok(isle_game_object_set_pre_retire_callback(world.game, owner, engine_observe_retiring, &fixture));
    ok(isle_game_object_set_retire_callback(world.game, owner, engine_observe_joined, &fixture));
}

void native_identity_holds_preserve_only_joined_addresses() {
    for (bool self_delete : {false, true}) {
        EngineIdentityFixture identity{};
        EngineFixture unrelated{}, fresh{};
        World world;
        auto& retired = identity.retirement;
        retired.game = unrelated.game = world.game;
        auto* owner = object(world, 1);
        identity.held = owner;
        observe_model_retirement(world, owner, retired);
        ok(isle_game_object_set_pre_retire_callback(world.game, owner, engine_identity_retiring, &identity));
        ok(isle_game_object_set_retire_callback(world.game, owner, engine_identity_joined, &identity));
        retired.observed_processes[0] = process(world, owner, engine_victim, &retired, ISLE_GAME_STACKFUL, 11);
        ok(isle_game_add_source_process(world.game, owner,
            self_delete ? engine_source_delete : engine_source_a, ISLE_GAME_STACKFUL, 5,
            &retired.observed_processes[1]));
        retired.observed_process_count = retired.waiting_processes = 2;
        auto* receiver = object(world, 9);
        identity.pool.arena = &world.arena;
        IsleGameCommandQueue queued{};
        ok(isle_game_command_send(&identity.pool, &queued, 22, owner));
        const auto held_cursor = world.arena.cursor;
        ok(isle_game_object_retain_identity(world.game, owner));
        ok(isle_game_object_retain_identity(world.game, owner));
        require(engine_identity_retain_at_limit(world.game, owner) == ISLE_GAME_EXHAUSTED &&
                world.arena.cursor == held_cursor,
                "counted identity retain allocated or accepted bounded hold overflow");
        ok(isle_game_tick(world.game));
        if (!self_delete) ok(isle_game_delete_object(world.game, owner));
        if (self_delete) trace(retired, {700, 30, 31, 451, 452});
        else trace(retired, {700, 410, 451, 452});
        require(identity.release_rejections == 2 && !engine_object_alive(owner) &&
                !engine_model_alive(retired.model) && engine_process_joined(retired.observed_processes[0]) &&
                engine_process_joined(retired.observed_processes[1]) &&
                retired.retired_continued == 0 && counts(world).objects == 1 &&
                counts(world).processes == 0 && counts(world).models == 0,
                "identity retention delayed real native joins, typed retirement or lifecycle callbacks");
        std::uint32_t id = 0;
        const IsleGameModelNode* expired_nodes = nullptr;
        std::size_t expired_count = 0;
        require(isle_game_object_id(world.game, owner, &id) == ISLE_GAME_INVALID_ARGUMENT &&
                isle_game_model_nodes(world.game, retired.model, &expired_nodes, &expired_count) ==
                    ISLE_GAME_INVALID_ARGUMENT &&
                isle_game_object_retain_identity(world.game, owner) == ISLE_GAME_INVALID_ARGUMENT,
                "held physical identity manufactured live object/model access or accepted late retain");

        auto* c = object(world, 3, 0, 0, engine_record_object, &unrelated);
        require(c != owner, "ordinary unrelated native allocation recycled a held dead GObj");
        IsleGameModelNodeInit initial{UINT32_MAX, identity_model_transform(), ISLE_GAME_MODEL_CONTEXTUAL_RPY};
        IsleGameModel* c_model = nullptr;
        ok(isle_game_attach_model(world.game, c, &initial, 1, &c_model));
        require(c_model == retired.model,
                "identity hold incorrectly retained retired model payload instead of only the GObj");
        identity.target = c;
        int delete_requested = 0;
        ok(isle_game_commands(world.game, receiver, &identity.pool, &queued,
                              engine_identity_clear_target, &identity, &delete_requested));
        require(!delete_requested && identity.deliveries == 1 && identity.target == c &&
                retired.failures == 0,
                "queued retired sender aliased or cleared an unrelated newly allocated native target");
        require(isle_game_destroy(world.game) == ISLE_GAME_INVALID_STATE &&
                engine_object_alive(c) && engine_model_alive(c_model),
                "destroy accepted dead held identities or partially tore down unrelated native state");
        const auto released_cursor = world.arena.cursor;
        ok(isle_game_object_release_identity(world.game, owner));
        require(world.arena.cursor == released_cursor, "nested identity release allocated storage");
        auto* d = object(world, 4);
        require(d != owner, "one nested release published an identity still held by another borrow");
        const auto final_cursor = world.arena.cursor;
        ok(isle_game_object_release_identity(world.game, owner));
        require(isle_game_object_release_identity(world.game, owner) == ISLE_GAME_INVALID_STATE,
                "dead identity double release republished the native free-list node");
        auto* reused = object(world, 5);
        require(reused == owner && world.arena.cursor == final_cursor,
                "final dead identity release did not publish the genuinely joined native pool slot");
        auto* distinct = object(world, 6);
        require(distinct != reused, "final dead release published the same physical GObj twice");
        ok(isle_game_tick(world.game));
        trace(unrelated, {3});
        ok(isle_game_object_id(world.game, reused, &id));
        require(id == 5 && retired.retired_continued == 0 && retired.pre_retire_calls == 1 &&
                retired.retire_calls == 1,
                "address reuse resurrected joined callbacks or corrupted an unrelated object");
        world.reset();
        fresh.game = world.game;
        auto* reset = object(world, 7, 0, 0, engine_record_object, &fresh);
        require(reset == owner, "cold reset did not exercise the formerly held dead GObj arena address");
        ok(isle_game_object_retain_identity(world.game, reset));
        ok(isle_game_object_release_identity(world.game, reset));
        ok(isle_game_tick(world.game));
        trace(fresh, {7}); trace(unrelated, {3});
        require(retired.pre_retire_calls == 1 && retired.retire_calls == 1 &&
                retired.retired_continued == 0,
                "held-identity cold reset inherited prior holds, callbacks or queued sender effects");
    }
}

void native_identity_access_rejections_preserve_live_holds() {
    World old_world;
    auto* foreign_object = object(old_world, 90);
    auto* old_game = old_world.game;
    ok(isle_game_destroy(old_world.game));
    old_world.game = nullptr;
    World world;
    auto* held = object(world, 1);
    auto* unheld = object(world, 2);
    ok(isle_game_object_retain_identity(world.game, held));
    const auto cursor = world.arena.cursor;
    require(isle_game_object_retain_identity(nullptr, held) == ISLE_GAME_INVALID_ARGUMENT &&
            isle_game_object_release_identity(nullptr, held) == ISLE_GAME_INVALID_ARGUMENT &&
            isle_game_object_retain_identity(old_game, held) == ISLE_GAME_INVALID_STATE &&
            isle_game_object_release_identity(old_game, held) == ISLE_GAME_INVALID_STATE &&
            isle_game_object_retain_identity(world.game, nullptr) == ISLE_GAME_INVALID_ARGUMENT &&
            isle_game_object_release_identity(world.game, nullptr) == ISLE_GAME_INVALID_ARGUMENT &&
            isle_game_object_retain_identity(world.game, foreign_object) == ISLE_GAME_INVALID_ARGUMENT &&
            isle_game_object_release_identity(world.game, foreign_object) == ISLE_GAME_INVALID_ARGUMENT &&
            isle_game_object_release_identity(world.game, unheld) == ISLE_GAME_INVALID_STATE,
            "identity operations admitted invalid games, absent/foreign objects or unheld release");
    bool wrong_owner = false;
    std::thread foreign([&] {
        wrong_owner = isle_game_object_retain_identity(world.game, held) == ISLE_GAME_WRONG_OWNER &&
                      isle_game_object_release_identity(world.game, held) == ISLE_GAME_WRONG_OWNER &&
                      isle_game_object_retain_identity(world.game, nullptr) == ISLE_GAME_WRONG_OWNER &&
                      isle_game_object_release_identity(world.game, nullptr) == ISLE_GAME_WRONG_OWNER;
    });
    foreign.join();
    require(wrong_owner && world.arena.cursor == cursor &&
            isle_game_destroy(world.game) == ISLE_GAME_INVALID_STATE,
            "wrong-thread/rejected operations mutated a real live hold or allocated storage");
    ok(isle_game_object_release_identity(world.game, held));
    require(isle_game_object_release_identity(world.game, held) == ISLE_GAME_INVALID_STATE &&
            engine_object_alive(held), "live release deleted an object or accepted a double release");
    std::uint32_t id = 0;
    ok(isle_game_object_id(world.game, held, &id));
    require(id == 1 && object(world, 3) != held, "live final release published an active native GObj");
    ok(isle_game_delete_object(world.game, held));
    require(object(world, 4) == held, "live hold release prevented later ordinary joined pool reuse");
}

void native_identity_destroy_rejection_and_cold_reset() {
    EngineFixture retired{}, updates{}, fresh{};
    World world;
    updates.game = world.game;
    auto* first = object(world, 1);
    observe_model_retirement(world, first, retired);
    retired.observed_processes[0] = process(world, first, engine_victim, &retired, ISLE_GAME_STACKFUL, 5);
    retired.observed_process_count = retired.waiting_processes = 1;
    auto* held = object(world, 2, 1, 0, engine_record_object, &updates);
    ok(isle_game_object_retain_identity(world.game, held));
    ok(isle_game_object_retain_identity(world.game, held));
    ok(isle_game_tick(world.game));
    require(isle_game_destroy(world.game) == ISLE_GAME_INVALID_STATE &&
            engine_object_alive(first) && engine_model_alive(retired.model) &&
            engine_process_waiting(retired.observed_processes[0]) &&
            retired.pre_retire_calls == 0 && retired.retire_calls == 0,
            "destroy discovered a later held object only after partially deleting earlier native state");
    ok(isle_game_tick(world.game));
    trace(updates, {2, 2}); trace(retired, {700});
    ok(isle_game_object_release_identity(world.game, held));
    require(isle_game_destroy(world.game) == ISLE_GAME_INVALID_STATE &&
            engine_object_alive(first) && engine_process_waiting(retired.observed_processes[0]),
            "destroy ignored the remaining nested live identity hold");
    ok(isle_game_object_release_identity(world.game, held));
    world.reset();
    trace(retired, {700, 451, 452});
    require(retired.retired_continued == 0,
            "released identities blocked actual destroy joins or resumed a retired callback");
    fresh.game = world.game;
    auto* reset = object(world, 7, 0, 0, engine_record_object, &fresh);
    require(reset == first, "identity cold-reset case did not exercise the same arena-backed GObj address");
    ok(isle_game_object_retain_identity(world.game, reset));
    ok(isle_game_object_release_identity(world.game, reset));
    ok(isle_game_tick(world.game));
    trace(fresh, {7}); trace(updates, {2, 2});
    require(retired.pre_retire_calls == 1 && retired.retire_calls == 1,
            "cold reset inherited an identity hold, observer or old callback");
}

void native_identity_release_rejects_real_callback_nesting() {
    EngineIdentityFixture identity{};
    World world;
    identity.retirement.game = world.game;
    identity.pool.arena = &world.arena;
    identity.held = object(world, 1);
    ok(isle_game_object_retain_identity(world.game, identity.held));
    ok(isle_game_delete_object(world.game, identity.held));
    auto* receiver = object(world, 2, 0, 0, engine_identity_release_callback, &identity);
    identity.target = receiver;
    process(world, receiver, engine_identity_release_callback, &identity, ISLE_GAME_SYNCHRONOUS, 5);
    process(world, receiver, engine_identity_release_callback, &identity, ISLE_GAME_STACKFUL, 4);
    ok(isle_game_tick(world.game));
    IsleGameCommandQueue outer{};
    ok(isle_game_command_send(&identity.pool, &outer, 1, receiver));
    ok(isle_game_command_send(&identity.pool, &identity.nested, 2, receiver));
    int delete_requested = 0;
    ok(isle_game_commands(world.game, receiver, &identity.pool, &outer,
                         engine_identity_nested_command, &identity, &delete_requested));
    ok(engine_identity_notify_queued(receiver, &identity));
    require(identity.retirement.failures == 0 && identity.release_rejections == 7 && !delete_requested,
            "real update/process/manual nested command/queue observers admitted active identity release");
    ok(isle_game_object_retain_identity(world.game, receiver));
    ok(isle_game_object_release_identity(world.game, receiver));
    require(object(world, 3) != identity.held,
            "a rejected callback release published a held retired native identity");
    ok(isle_game_object_release_identity(world.game, identity.held));
    require(object(world, 4) == identity.held,
            "returned nested callbacks left a permanent guard or failed real dead pool publication");
}

void native_identity_callback_wait_restores_active_exclusion() {
    EngineIdentityFixture identity{};
    World world;
    identity.retirement.game = world.game;
    identity.pool.arena = &world.arena;
    identity.held = object(world, 1);
    ok(isle_game_object_retain_identity(world.game, identity.held));
    ok(isle_game_delete_object(world.game, identity.held));
    auto* receiver = object(world, 2);
    auto* callback = process(world, receiver, engine_identity_wait_process, &identity,
                             ISLE_GAME_STACKFUL, 5);
    ok(isle_game_command_send(&identity.pool, &identity.nested, 1, receiver));
    ok(isle_game_tick(world.game));
    require(identity.retirement.failures == 0 && identity.release_rejections == 1 &&
            identity.retirement.shared_value == 1 && identity.retirement.after_finish == 0 &&
            engine_process_waiting(callback),
            "real command-handler wait did not preserve its compiled-C continuation");
    ok(isle_game_object_retain_identity(world.game, receiver));
    ok(isle_game_object_release_identity(world.game, receiver));
    require(object(world, 3) != identity.held,
            "waiting command frame lost its held sender's physical identity");
    ok(isle_game_tick(world.game));
    require(identity.retirement.failures == 0 && identity.release_rejections == 2 &&
            identity.retirement.after_finish == 1 && engine_process_joined(callback),
            "real command-handler resume lost its active release exclusion or failed to join normally");
    ok(isle_game_object_release_identity(world.game, identity.held));
    require(object(world, 4) == identity.held,
            "returned waiting command handler left callback exclusion permanently active");
}

void native_identity_retains_from_admitted_stackful_entries() {
    EngineIdentityFixture identity{};
    World world;
    identity.retirement.game = world.game;
    identity.held = object(world, 1);
    auto* callback = process(world, identity.held, engine_identity_retain_process, &identity,
                             ISLE_GAME_STACKFUL, 5);
    ok(isle_game_tick(world.game));
    trace(identity.retirement, {710});
    require(identity.retain_successes == 1 && identity.release_rejections == 1 &&
            engine_process_waiting(callback),
            "identity retention rejected genuine serialized native continuation factory access");
    ok(isle_game_delete_object(world.game, identity.held));
    require(engine_process_joined(callback) && object(world, 2) != identity.held,
            "callback-created hold delayed joining or allowed unrelated native address reuse");
    ok(isle_game_object_release_identity(world.game, identity.held));
    require(object(world, 3) == identity.held && identity.retirement.retired_continued == 0,
            "admitted callback retention did not release its actual joined physical identity");
}

void native_identity_fault_unwinds_callback_exclusion() {
    for (bool stackful : {false, true}) {
        EngineIdentityFixture identity{};
        World world;
        identity.retirement.game = world.game;
        identity.retirement.mode = stackful ? 1 : 0;
        identity.pool.arena = &world.arena;
        identity.held = object(world, 1);
        ok(isle_game_object_retain_identity(world.game, identity.held));
        ok(isle_game_delete_object(world.game, identity.held));
        auto* receiver = object(world, 2, 0, 0, stackful ? nullptr : engine_identity_fault_update,
                                 stackful ? nullptr : &identity);
        if (stackful) process(world, receiver, engine_identity_fault_update, &identity, ISLE_GAME_STACKFUL, 5);
        ok(isle_game_command_send(&identity.pool, &identity.nested, 1, receiver));
        require(isle_game_tick(world.game) == ISLE_GAME_EXECUTION_FAILURE &&
                identity.release_rejections == 1 && identity.retirement.failures == 0,
                "actual owner/worker C fault failed to reject active release or resumed its discarded callback");
        ok(isle_game_object_release_identity(world.game, identity.held));
        ok(isle_game_destroy(world.game));
        world.game = nullptr;
    }
}

void native_retirement_observes_real_joins_and_reuse() {
    for (bool direct : {false, true}) {
        EngineFixture fixture{}, replacement{}, rejected{};
        World world;
        fixture.mode = 1;
        auto* owner = object(world, 1);
        observe_model_retirement(world, owner, fixture);
        fixture.observed_processes[0] = process(world, owner, engine_victim, &fixture, ISLE_GAME_STACKFUL, 8);
        ok(isle_game_add_source_process(world.game, owner, engine_source_a, ISLE_GAME_STACKFUL, 5,
                                       &fixture.observed_processes[1]));
        fixture.observed_process_count = fixture.waiting_processes = 2;
        ok(isle_game_tick(world.game));
        trace(fixture, {700, 410});
        require(fixture.pre_retire_calls == 0 && fixture.retire_calls == 0,
                "parking source/native continuations prematurely notified retirement");
        bool wrong_owner = false;
        std::thread foreign([&] {
            wrong_owner = isle_game_object_set_pre_retire_callback(
                world.game, owner, engine_observe_retiring, &rejected) == ISLE_GAME_WRONG_OWNER;
        });
        foreign.join();
        require(wrong_owner && isle_game_object_set_pre_retire_callback(
                    world.game, nullptr, nullptr, nullptr) == ISLE_GAME_INVALID_ARGUMENT,
                "pre-retirement setter lost owner/NULL-current validation precedence");
        const auto used = world.arena.cursor;
        if (direct) engine_direct_delete(owner);
        else ok(isle_game_delete_object(world.game, owner));
        trace(fixture, {700, 410, 451, 452});
        require(fixture.pre_retire_calls == 1 && fixture.retire_calls == 1 &&
                fixture.retired_continued == 0 && counts(world).objects == 0 &&
                counts(world).processes == 0 && counts(world).models == 0 &&
                rejected.pre_retire_calls == 0,
                "native deletion did not observe intact processes/model then actual joined removal");
        require(isle_game_delete_object(world.game, owner) == ISLE_GAME_INVALID_ARGUMENT &&
                isle_game_object_set_pre_retire_callback(world.game, owner, nullptr, nullptr) ==
                    ISLE_GAME_INVALID_ARGUMENT,
                "dead native object accepted a new retirement observer or repeated deletion");
        auto* reused = object(world, 2);
        require(reused == owner, "native object pool did not exercise the retired slot");
        observe_model_retirement(world, reused, replacement);
        require(replacement.model == fixture.model, "native model reuse did not exercise the retired payload");
        ok(isle_game_delete_object(world.game, reused));
        trace(replacement, {451, 452});
        require(fixture.pre_retire_calls == 1 && fixture.retire_calls == 1 && world.arena.cursor == used,
                "reuse retained observer state or allocated instead of reusing joined storage");
        reused = object(world, 3);
        observe_model_retirement(world, reused, rejected);
        ok(isle_game_object_set_pre_retire_callback(world.game, reused, nullptr, nullptr));
        ok(isle_game_object_set_retire_callback(world.game, reused, nullptr, nullptr));
        ok(isle_game_delete_object(world.game, reused));
        require(rejected.pre_retire_calls == 0 && rejected.retire_calls == 0,
                "cleared native lifecycle observers still ran");
    }
}

void native_retirement_rejects_corrupt_payload_before_notification() {
    for (int mode : {0, 1, 2, 3, 4}) {
        EngineFixture fixture{};
        World world;
        auto* owner = object(world, 1);
        observe_model_retirement(world, owner, fixture);
        fixture.observed_processes[0] = process(world, owner, engine_victim, &fixture, ISLE_GAME_STACKFUL, 5);
        fixture.observed_process_count = fixture.waiting_processes = 1;
        ok(isle_game_tick(world.game));
        require(engine_delete_corrupt_payload(owner, mode) == ISLE_GAME_EXECUTION_FAILURE &&
                fixture.failures == 0 && fixture.pre_retire_calls == 0 && fixture.retire_calls == 0 &&
                counts(world).objects == 1 && counts(world).processes == 1 && counts(world).models == 1,
                "corrupt payload changed lifecycle state, joined a process or removed storage");
        ok(isle_game_destroy(world.game));
        world.game = nullptr;
        trace(fixture, {700, 451, 452});
        require(fixture.retired_continued == 0, "fault destruction resumed a parked continuation");
    }
}

void native_fault_destruction_notifies_only_at_actual_teardown() {
    for (auto kind : {ISLE_GAME_SYNCHRONOUS, ISLE_GAME_STACKFUL}) {
        EngineFixture fixture{};
        World world;
        fixture.mode = kind == ISLE_GAME_STACKFUL ? 1 : 0;
        auto* owner = object(world, 1);
        observe_model_retirement(world, owner, fixture);
        fixture.observed_processes[0] = process(world, owner, engine_victim, &fixture, ISLE_GAME_STACKFUL, 11);
        fixture.observed_processes[1] = process(world, owner, engine_invalid_legacy_wait, &fixture, kind, 5);
        fixture.observed_process_count = 2;
        fixture.waiting_processes = 1;
        require(isle_game_tick(world.game) == ISLE_GAME_EXECUTION_FAILURE &&
                fixture.pre_retire_calls == 0 && fixture.retire_calls == 0 &&
                counts(world).models == 1 && counts(world).processes == 2,
                "callback fault synthesized retirement before the actual destruction boundary");
        ok(isle_game_destroy(world.game));
        world.game = nullptr;
        trace(fixture, {700, 60, 451, 452});
        require(fixture.retired_continued == 0 && fixture.pre_retire_calls == 1 && fixture.retire_calls == 1,
                "fault teardown did not join all native continuations exactly once");
    }
}

void model_deferred_delete_and_destroy() {
    for (int mode : {0, 1, 2, 3}) {
        EngineFixture fixture{};
        World world;
        fixture.game = world.game; fixture.mode = mode;
        auto* owner = object(world, 1, 0, 0, mode == 2 ? engine_delete_self : nullptr,
                             mode == 2 ? &fixture : nullptr);
        IsleGameModelNodeInit initial{UINT32_MAX, identity_model_transform(), ISLE_GAME_MODEL_ROTATE_RPY_TRANSLATE_SCALE};
        IsleGameModel* model = nullptr;
        ok(isle_game_attach_model(world.game, owner, &initial, 1, &model));
        fixture.model = model;
        engine_bind_source_payload(owner, &fixture);
        ok(isle_game_object_set_pre_retire_callback(world.game, owner, engine_observe_retiring, &fixture));
        ok(isle_game_object_set_retire_callback(world.game, owner, engine_observe_joined, &fixture));
        auto placement = identity_model_transform();
        placement.position = {3, 4, 5};
        ok(isle_game_model_set_placement(world.game, model, ISLE_GAME_MODEL_PLACEMENT_RT, &placement));
        require(counts(world).matrices == 2, "deferred owner omitted active placement");
        fixture.observed_processes[0] = process(world, owner, engine_victim, &fixture, ISLE_GAME_STACKFUL, 11);
        fixture.observed_process_count = 1;
        if (mode != 2) {
            if (mode == 3) {
                ok(isle_game_add_source_process(world.game, owner, engine_source_delete,
                    ISLE_GAME_STACKFUL, 5, &fixture.observed_processes[1]));
            } else {
                fixture.observed_processes[1] = process(world, owner, engine_delete_self, &fixture,
                    mode == 0 ? ISLE_GAME_STACKFUL : ISLE_GAME_SYNCHRONOUS, 5);
            }
            fixture.observed_process_count = 2;
            fixture.waiting_processes = mode == 1 ? 1 : 2;
        }
        ok(isle_game_tick(world.game));
        auto live = counts(world);
        require(live.objects == 0 && live.processes == 0 && live.models == 0 &&
            live.model_nodes == 0 && live.matrices == 0 && fixture.retired_continued == 0,
            "deferred self-deletion failed to retire native type1 model");
        if (mode == 2) trace(fixture, {30, 31, 451, 452});
        else trace(fixture, {700, 30, 31, 451, 452});
        require(fixture.pre_retire_calls == 1 && fixture.retire_calls == 1,
                "deferred self-delete notified early, repeated or missed actual teardown");
    }
    EngineFixture fixture{};
    World world;
    auto* owner = object(world, 1);
    IsleGameModelNodeInit initial{UINT32_MAX, identity_model_transform(), ISLE_GAME_MODEL_ROTATE_RPY_TRANSLATE_SCALE};
    IsleGameModel* model = nullptr;
    ok(isle_game_attach_model(world.game, owner, &initial, 1, &model));
    fixture.game = world.game;
    fixture.model = model;
    engine_bind_source_payload(owner, &fixture);
    ok(isle_game_object_set_pre_retire_callback(world.game, owner, engine_observe_retiring, &fixture));
    ok(isle_game_object_set_retire_callback(world.game, owner, engine_observe_joined, &fixture));
    fixture.observed_processes[0] = process(world, owner, engine_victim, &fixture, ISLE_GAME_STACKFUL, 5);
    fixture.observed_process_count = fixture.waiting_processes = 1;
    ok(isle_game_tick(world.game));
    const auto placement = identity_model_transform();
    ok(isle_game_model_set_placement(world.game, model, ISLE_GAME_MODEL_PLACEMENT_RTS, &placement));
    world.reset();
    auto live = counts(world);
    require(live.objects == 0 && live.models == 0 && live.model_nodes == 0 && live.matrices == 0,
        "destroy/cold reset retained native type1 payload");
    trace(fixture, {700, 451, 452});
    ok(isle_game_tick(world.game));
    require(fixture.pre_retire_calls == 1 && fixture.retire_calls == 1 && fixture.retired_continued == 0,
            "native context destruction must join before arena reuse, without resurrecting old observers");
}
} // namespace

int main() {
    try {
        update_and_process_order();
        actor_payload_and_update_context_are_independent();
        source_function_identity_and_reuse();
        source_function_pause_matches_actual_callables();
        source_function_pause_preserves_real_waits();
        source_function_pause_current_callback_rules();
        source_function_pause_rejects_invalid_owners_unchanged();
        source_function_pause_joins_and_reuses_real_continuations();
        source_wait_replace_and_retirement();
        pause_preserves_nested_waits();
        callback_lifecycle_and_same_tick_child();
        synchronous_end_and_update_pause();
        deferred_object_deletion();
        actual_intro_fade_and_cold_reset();
        bounded_pool_reuse_and_failure_fence();
        camera_geometry_and_cache();
        camera_roll_geometry_and_cache();
        camera_roll_ignores_unused_up();
        camera_roll_boundaries_and_reuse();
        camera_roll_batch_failure_is_atomic();
        camera_reflection_roll_geometry_and_state();
        camera_batch_failure_is_atomic();
        camera_rejection_and_typed_cleanup();
        camera_exhaustion_and_reuse();
        model_hierarchy_math_and_cache();
        model_contextual_pose_admission_and_cache();
        model_source54_math_and_atomicity();
        model_source48_basis_and_atomicity();
        model_placement_atomicity_and_lifecycle();
        model_translate_selection_and_cache();
        model_photo_bake();
        model_rejection_atomicity_and_reuse();
        native_retirement_observes_real_joins_and_reuse();
        native_identity_holds_preserve_only_joined_addresses();
        native_identity_access_rejections_preserve_live_holds();
        native_identity_destroy_rejection_and_cold_reset();
        native_identity_release_rejects_real_callback_nesting();
        native_identity_callback_wait_restores_active_exclusion();
        native_identity_retains_from_admitted_stackful_entries();
        native_identity_fault_unwinds_callback_exclusion();
        native_retirement_rejects_corrupt_payload_before_notification();
        native_fault_destruction_notifies_only_at_actual_teardown();
        model_deferred_delete_and_destroy();
    } catch (const std::exception& error) {
        std::cerr << "native C engine checks failed: " << error.what() << '\n';
        return 1;
    }
    std::cout << "native C engine checks passed; actual dispatcher/fade, not full scene boot\n";
    return 0;
}
