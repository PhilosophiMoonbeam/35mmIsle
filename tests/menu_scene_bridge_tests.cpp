#include "menu_scene_bridge.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>

namespace {
int failures = 0;
void check(bool condition, std::string_view meaning) {
    if (!condition) { ++failures; std::cerr << "FAIL: " << meaning << '\n'; }
}
struct Fixture {
    std::array<std::byte, 128 * 1024> storage{};
    isle_arena arena{};
    IsleGame* game = nullptr;
    Fixture() {
        check(isle_arena_init(&arena, storage.data(), storage.size()) == ISLE_OK, "arena initialization");
        check(isle_game_create(&arena, &game) == ISLE_GAME_OK, "native world initialization");
    }
    ~Fixture() { if (game != nullptr) check(isle_game_destroy(game) == ISLE_GAME_OK, "native world cessation"); }
    IsleGameObject* object() {
        IsleGameObject* result = nullptr;
        check(isle_game_add_object(game, 14, 0, 0x80000000, nullptr, nullptr, &result) == ISLE_GAME_OK, "native object creation");
        return result;
    }
};
void count(IsleGameObject*, void* context) { ++*static_cast<unsigned*>(context); }
void ordered_commands_and_release() {
    Fixture f;
    auto* object = f.object();
    unsigned calls = 0;
    IsleGameProcess* process = nullptr;
    check(isle_game_add_process(f.game, object, count, &calls, ISLE_GAME_SYNCHRONOUS, 1, &process) == ISLE_GAME_OK, "command target process");
    IsleGameCommandPool pool{&f.arena, nullptr};
    IsleGameCommandQueue queue{};
    int erase = 0;
    check(isle_game_command_send(&pool, &queue, -2, object) == ISLE_GAME_OK && queue.count == 1, "enqueue actual pause command");
    check(isle_game_commands(f.game,object,&pool,&queue,nullptr,nullptr,&erase) == ISLE_GAME_OK && !erase && queue.count == 0, "pause drains the queue");
    check(isle_game_tick(f.game) == ISLE_GAME_OK && calls == 0, "paused process consumes no update");
    const auto recycled_cursor = f.arena.cursor;
    check(isle_game_command_send(&pool, &queue, -3, object) == ISLE_GAME_OK && f.arena.cursor == recycled_cursor, "released command nodes recycle");
    check(isle_game_commands(f.game,object,&pool,&queue,nullptr,nullptr,&erase) == ISLE_GAME_OK, "resume drains the queue");
    check(isle_game_tick(f.game) == ISLE_GAME_OK && calls == 1, "resumed process executes in the next actual native update");
    check(isle_game_command_send(&pool, &queue, -4, object) == ISLE_GAME_OK &&
          isle_game_command_send(&pool, &queue, -5, object) == ISLE_GAME_OK &&
          isle_game_command_send(&pool, &queue, 72, object) == ISLE_GAME_OK, "ordered hide/show/custom command chain");
    check(isle_game_commands(f.game,object,&pool,&queue,nullptr,nullptr,&erase) == ISLE_GAME_OK &&
          !isle_menu_object_hidden(f.game, object) && queue.count == 0, "show wins over earlier hide; NULL custom handler drains unknown commands");
    IsleGameObjectMetadata metadata{};
    check(isle_game_object_set_flags(f.game,object,0x101) == ISLE_GAME_OK &&
          isle_menu_object_hidden(f.game,object) &&
          isle_game_object_metadata(f.game,object,&metadata) == ISLE_GAME_OK && metadata.flags == 0x101 &&
          isle_game_object_set_flags(f.game,object,metadata.flags & ~1u) == ISLE_GAME_OK &&
          !isle_menu_object_hidden(f.game,object),"immediate native visibility assignments preserve unrelated source flags");
    check(isle_game_command_send(&pool, &queue, -4, object) == ISLE_GAME_OK &&
          isle_game_command_send(&pool, &queue, -1, object) == ISLE_GAME_OK &&
          isle_game_command_send(&pool, &queue, -5, object) == ISLE_GAME_OK, "delete in middle of command chain");
    check(isle_game_commands(f.game,object,&pool,&queue,nullptr,nullptr,&erase) == ISLE_GAME_OK && erase &&
          isle_menu_object_hidden(f.game, object), "delete stops before later show; caller owns whole-object deletion");
    check(isle_game_object_metadata(f.game,object,&metadata) == ISLE_GAME_OK && metadata.flags == 0x101,
          "queued hide and stop-on-delete preserve unrelated immediate source flags");
    isle_game_commands_release(&pool, &queue);
    check(isle_game_delete_object(f.game, object) == ISLE_GAME_OK, "whole-object delete retires all target processes");
    IsleGameCounts counts{};
    check(isle_game_counts(f.game, &counts) == ISLE_GAME_OK && counts.objects == 0 && counts.processes == 0,
          "delete leaves no object or process borrower");
}
void initial_pool_overflow_recycling_and_rejection() {
    Fixture f;
    check(isle_menu_reserve_pools(f.game, &f.arena, 1, 1, 1, 1, 2) == ISLE_GAME_OK, "reserve native initial pools");
    const auto reserved = f.arena.cursor;
    auto* first = f.object();
    check(f.arena.cursor == reserved, "first object comes from initial native pool");
    auto* overflow = f.object();
    check(overflow != nullptr && f.arena.cursor > reserved, "initial object count is not a maximum");
    check(isle_game_delete_object(f.game, first) == ISLE_GAME_OK, "release first pool object");
    const auto grown = f.arena.cursor;
    check(f.object() == first && f.arena.cursor == grown, "whole-object deletion recycles actual native storage");
    check(isle_menu_reserve_pools(f.game, &f.arena, std::numeric_limits<std::size_t>::max(), 1, 0, 0, 0) == ISLE_GAME_EXHAUSTED &&
          f.arena.cursor == grown, "oversized pool reservation publishes no cursor change");
    check(isle_menu_end_processes(f.game, overflow) == ISLE_GAME_OK, "empty process chain ends without removing object");
}
struct SelfEnd {
    IsleGame* game = nullptr;
    unsigned calls = 0;
    bool entered = false, returned = false;
};
void end_self_and_siblings(IsleGameObject* object, void* context) {
    auto& state = *static_cast<SelfEnd*>(context);
    state.entered = true;
    isle_menu_end_processes(state.game, object);
    state.returned = true;
}
void self_end_is_a_real_continuation_fence() {
    Fixture f;
    auto* object = f.object();
    SelfEnd state{f.game};
    IsleGameProcess* process = nullptr;
    check(isle_game_add_process(f.game, object, end_self_and_siblings, &state, ISLE_GAME_STACKFUL, 1, &process) == ISLE_GAME_OK,
          "create actual self-ending stackful process");
    check(isle_game_add_process(f.game, object, count, &state.calls, ISLE_GAME_SYNCHRONOUS, 1, &process) == ISLE_GAME_OK,
          "create sibling following self-ending process");
    check(isle_game_tick(f.game) == ISLE_GAME_OK && state.entered && !state.returned && state.calls == 0,
          "end-all joins siblings before nonreturning current-process end");
    IsleGameCounts counts{};
    check(isle_game_counts(f.game, &counts) == ISLE_GAME_OK && counts.objects == 1 && counts.processes == 0,
          "self-end retains owner but no process continuation");
}
struct Order { unsigned clock = 0, first = 0, second = 0; };
void first(IsleGameObject*, void* context) {
    auto& order = *static_cast<Order*>(context);
    order.first = ++order.clock;
}
void second(IsleGameObject*, void* context) {
    auto& order = *static_cast<Order*>(context);
    order.second = ++order.clock;
}
void prefix_reentry_preserves_native_process_order() {
    Fixture f;
    auto* effects = f.object();
    auto* control = f.object();
    auto* camera = f.object();
    check(isle_menu_place_before(f.game, camera, effects) == ISLE_GAME_OK,
          "fresh reentry camera reinserted before surviving source FX/UI prefix");
    IsleGameProcess* process = nullptr;
    Order order;
    check(isle_game_add_process(f.game, control, second, &order, ISLE_GAME_SYNCHRONOUS, 1, &process) == ISLE_GAME_OK &&
          isle_game_add_process(f.game, camera, first, &order, ISLE_GAME_SYNCHRONOUS, 1, &process) == ISLE_GAME_OK,
          "camera process created after UI control, in actual initializer order");
    check(isle_menu_place_before(f.game, control, effects) == ISLE_GAME_INVALID_ARGUMENT,
          "reordering a live process owner rejects instead of corrupting process-list order");
    check(isle_game_tick(f.game) == ISLE_GAME_OK && order.first == 1 && order.second == 2,
          "source object order, not process creation order, runs camera before UI control");
}
struct Custom {
    IsleGame* game = nullptr;
    IsleGameCommandPool* pool = nullptr;
    IsleGameCommandQueue* queue = nullptr;
    std::array<std::int32_t,8> values{};
    unsigned count = 0;
    IsleGameObject* source = nullptr;
};
void custom(IsleGameObject* object, std::int32_t command, IsleGameObject* source, void* opaque) {
    auto& state = *static_cast<Custom*>(opaque);
    state.values[state.count++] = command;
    state.source = source;
    if (command == 7) check(isle_game_command_send(state.pool,state.queue,9,object) == ISLE_GAME_OK,
                            "custom handler appends to the same live source FIFO");
}
void custom_commands_share_the_builtin_fifo() {
    Fixture f;
    auto* object = f.object();
    auto* source = f.object();
    IsleGameCommandPool pool{&f.arena,nullptr};
    IsleGameCommandQueue queue{};
    Custom state{f.game,&pool,&queue};
    int erase = 0;
    for (auto command : {7,-4,8,-5})
        check(isle_game_command_send(&pool,&queue,command,source) == ISLE_GAME_OK,"enqueue mixed source custom/builtins");
    check(isle_game_commands(f.game,object,&pool,&queue,custom,&state,&erase) == ISLE_GAME_OK &&
          !erase && state.count == 3 && state.values[0] == 7 && state.values[1] == 8 && state.values[2] == 9 &&
          state.source == object && !isle_menu_object_hidden(f.game,object) && queue.count == 0,
          "custom commands retain FIFO/source identity including handler-appended command; builtins bypass handler");
    state.count = 0;
    for (auto command : {11,-1,12})
        check(isle_game_command_send(&pool,&queue,command,source) == ISLE_GAME_OK,"enqueue custom/delete/custom");
    check(isle_game_commands(f.game,object,&pool,&queue,custom,&state,&erase) == ISLE_GAME_OK &&
          erase && state.count == 1 && state.values[0] == 11 && queue.count == 3,
          "builtin deletion stops before custom suffix and retains nodes until whole-object retirement");
    isle_game_commands_release(&pool,&queue);
}
struct Retirement {
    IsleGame* game = nullptr;
    bool entered = false, retired = false, continued = false;
    unsigned siblings = 0;
};
void retired(IsleGameObject*, void* opaque) {
    auto& state = *static_cast<Retirement*>(opaque);
    IsleGameCounts counts{};
    check(isle_game_counts(state.game,&counts) == ISLE_GAME_OK && counts.processes == 0 && counts.sprites == 0,
          "retirement callback observes all process and payload borrowers joined/removed");
    state.retired = true;
}
void delete_self(IsleGameObject* object, void* opaque) {
    auto& state = *static_cast<Retirement*>(opaque);
    state.entered = true;
    check(isle_game_delete_object(state.game,object) == ISLE_GAME_OK && !state.retired,
          "self-delete retains callback userdata before actual native traversal retirement");
    state.continued = true;
    isle_game_wait(state.game,1);
    check(false,"retired self-delete continuation never resumes");
}
void deferred_retirement_joins_before_userdata_recycling() {
    Fixture f;
    auto* object = f.object();
    Retirement state{f.game};
    IsleGameProcess* process = nullptr;
    IsleGameSprite* sprite = nullptr;
    check(isle_game_object_set_retire_callback(f.game,object,retired,&state) == ISLE_GAME_OK &&
          isle_game_add_sprite(f.game,object,nullptr,&sprite) == ISLE_GAME_OK &&
          isle_game_add_process(f.game,object,delete_self,&state,ISLE_GAME_STACKFUL,1,&process) == ISLE_GAME_OK &&
          isle_game_add_process(f.game,object,count,&state.siblings,ISLE_GAME_SYNCHRONOUS,1,&process) == ISLE_GAME_OK,
          "create real deferred deletion callback, payload and sibling process");
    check(isle_game_tick(f.game) == ISLE_GAME_OK && state.entered && state.continued && state.retired && state.siblings == 0,
          "retirement runs once at real self-delete fence, before skipped sibling can execute");
    auto* reused = f.object();
    check(reused == object && isle_game_delete_object(f.game,reused) == ISLE_GAME_OK,
          "native node can recycle only after retired callback cleared");
}
} // namespace
int main() {
    ordered_commands_and_release();
    initial_pool_overflow_recycling_and_rejection();
    self_end_is_a_real_continuation_fence();
    prefix_reentry_preserves_native_process_order();
    custom_commands_share_the_builtin_fifo();
    deferred_retirement_joins_before_userdata_recycling();
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
