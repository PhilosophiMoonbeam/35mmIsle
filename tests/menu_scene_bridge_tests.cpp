#include "menu_scene_bridge.h"
#include "source_factory_resources.h"
#include "owned_command_fixture.h"

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
void check_native(int source_line, std::string_view meaning) {
    if (source_line == 0) return;
    ++failures;
    std::cerr << "FAIL: " << meaning << " (native C fixture line " << source_line << ")\n";
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
    IsleGameCommandPool pool{&f.arena, nullptr, 0};
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
void frozen_native_capacity_rejects_growth_and_recycles() {
    Fixture f;
    check(isle_menu_reserve_pools(f.game,&f.arena,1,0,0,0,0)==ISLE_GAME_OK,
          "reserve explicit physical object envelope");
    IsleGameCommandPool pool{&f.arena,nullptr,0};
    check(isle_menu_reserve_native_capacity(f.game,&pool,2,nullptr,0)==ISLE_GAME_OK,
          "freeze seeded physical pools with two command nodes");
    const auto reserved=f.arena.cursor;
    auto* object=f.object();
    IsleGameObject* overflow=nullptr;
    check(isle_game_add_object(f.game,15,0,0,nullptr,nullptr,&overflow)==ISLE_GAME_EXHAUSTED &&
          overflow==nullptr && f.arena.cursor==reserved,
          "frozen object exhaustion never falls back to available arena space");
    IsleGameCommandQueue queue{};
    check(isle_game_command_send(&pool,&queue,-4,nullptr)==ISLE_GAME_OK &&
          isle_game_command_send(&pool,&queue,-5,nullptr)==ISLE_GAME_OK,
          "reserved command capacity accepts its exact boundary");
    const auto* head=queue.head;
    const auto* tail=queue.tail;
    check(isle_game_command_send(&pool,&queue,19,nullptr)==ISLE_GAME_EXHAUSTED &&
          queue.count==2 && queue.head==head && queue.tail==tail && f.arena.cursor==reserved,
          "command exhaustion preserves the complete queued prefix and arena");
    isle_game_commands_release(&pool,&queue);
    check(isle_game_command_send(&pool,&queue,-4,nullptr)==ISLE_GAME_OK && f.arena.cursor==reserved,
          "released command storage remains reusable after freezing");
    int erase=0;
    check(isle_game_commands(f.game,object,&pool,&queue,nullptr,nullptr,&erase)==ISLE_GAME_OK &&
          isle_menu_object_hidden(f.game,object) && queue.count==0,
          "recycled frozen command executes its actual builtin effect");
    check(isle_game_delete_object(f.game,object)==ISLE_GAME_OK && f.object()==object &&
          f.arena.cursor==reserved,"frozen object retirement reuses physical storage");
    check(isle_menu_reserve_pools(f.game,&f.arena,1,0,0,0,0)==ISLE_GAME_INVALID_STATE &&
          isle_menu_reserve_native_capacity(f.game,&pool,1,nullptr,0)==ISLE_GAME_INVALID_STATE &&
          f.arena.cursor==reserved,"frozen pools cannot be expanded or refrozen");
}
void frozen_models_require_reserved_native_storage() {
    Fixture f;
    check(isle_menu_reserve_pools(f.game,&f.arena,1,0,0,0,0)==ISLE_GAME_OK,
          "seed the model owner's physical object");
    IsleGameCommandPool pool{&f.arena,nullptr,0};
    check(isle_menu_reserve_native_capacity(f.game,&pool,0,nullptr,0)==ISLE_GAME_OK,
          "freeze with no reserved native models");
    auto* object=f.object();
    const IsleGameModelNodeInit root{UINT32_MAX,{{},{},{1,1,1}},ISLE_GAME_MODEL_TRANSLATE};
    IsleGameModel* model=nullptr;
    const auto reserved=f.arena.cursor;
    check(isle_game_attach_model(f.game,object,&root,1,&model)==ISLE_GAME_EXHAUSTED &&
          model==nullptr && f.arena.cursor==reserved,
          "an unreserved model cannot grow a frozen native arena");
}
void frozen_model_shapes_recycle_without_growth() {
    Fixture f;
    check(isle_menu_reserve_pools(f.game,&f.arena,4,0,0,0,0)==ISLE_GAME_OK,
          "seed model and unpublished-factory owner slots");
    IsleGameCommandPool pool{&f.arena,nullptr,0};
    const IsleGameModelCapacity capacities[]{{1,1},{3,1}};
    check(isle_menu_reserve_native_capacity(f.game,&pool,1,capacities,2)==ISLE_GAME_OK,
          "reserve distinct native model shapes before freezing");
    const auto reserved=f.arena.cursor;
    auto* a=f.object();
    auto* b=f.object();
    auto* c=f.object();
    const IsleGameModelNodeInit nodes[]{
        {UINT32_MAX,{{},{},{1,1,1}},ISLE_GAME_MODEL_TRANSLATE},
        {0,{{1,2,3},{},{1,1,1}},ISLE_GAME_MODEL_TRANSLATE},
        {1,{{4,5,6},{},{1,1,1}},ISLE_GAME_MODEL_TRANSLATE}
    };
    IsleGameModel* small=nullptr;
    IsleGameModel* large=nullptr;
    IsleGameModel* overflow=nullptr;
    check(isle_game_attach_model(f.game,a,nodes,1,&small)==ISLE_GAME_OK &&
          isle_game_attach_model(f.game,b,nodes,3,&large)==ISLE_GAME_OK &&
          f.arena.cursor==reserved,"best-fit reserves preserve both native shapes");
    check(isle_game_attach_model(f.game,c,nodes,1,&overflow)==ISLE_GAME_EXHAUSTED &&
          overflow==nullptr && f.arena.cursor==reserved,
          "live model depletion cannot allocate from the remaining arena");
    check(isle_game_remove_model(f.game,small)==ISLE_GAME_OK,
          "release the small model without deleting its actual owner");
    IsleSourceFactoryResources factory{};
    check(isle_source_factory_prepare(f.game,1,nullptr,0,&factory)==ISLE_GAME_OK &&
          f.arena.cursor==reserved,
          "actual unpublished Source factory reserves recycled frozen model storage");
    check(isle_source_factory_cancel(&factory)==ISLE_GAME_OK && f.arena.cursor==reserved,
          "factory cancellation returns frozen model/object storage");
    check(isle_game_remove_model(f.game,large)==ISLE_GAME_OK &&
          isle_game_attach_model(f.game,c,nodes,2,&overflow)==ISLE_GAME_OK &&
          overflow==large && f.arena.cursor==reserved,
          "released larger model storage serves a smaller live shape without growth");
}
void failed_model_reservation_keeps_old_storage_and_growth_policy() {
    Fixture f;
    auto* object=f.object();
    const IsleGameModelNodeInit nodes[]{
        {UINT32_MAX,{{},{},{1,1,1}},ISLE_GAME_MODEL_TRANSLATE},
        {0,{{},{},{1,1,1}},ISLE_GAME_MODEL_TRANSLATE}
    };
    IsleGameModel* old=nullptr;
    check(isle_game_attach_model(f.game,object,nodes,1,&old)==ISLE_GAME_OK &&
          isle_game_remove_model(f.game,old)==ISLE_GAME_OK,"seed preexisting free model");
    IsleGameCommandPool pool{&f.arena,nullptr,0};
    const auto before=f.arena.cursor;
    const IsleGameModelCapacity impossible[]{{1,1},{3,std::numeric_limits<std::size_t>::max()}};
    check(isle_menu_reserve_native_capacity(f.game,&pool,2,impossible,2)==ISLE_GAME_EXHAUSTED &&
          f.arena.cursor==before && pool.free==nullptr && !pool.fixed_capacity,
          "partial model exhaustion publishes neither commands nor model classes nor freeze");
    IsleGameModel* recycled=nullptr;
    check(isle_game_attach_model(f.game,object,nodes,1,&recycled)==ISLE_GAME_OK &&
          recycled==old && f.arena.cursor==before &&
          isle_game_remove_model(f.game,recycled)==ISLE_GAME_OK,
          "failed reservation leaves the preexisting released model intact");
    check(isle_game_attach_model(f.game,object,nodes,2,&recycled)==ISLE_GAME_OK &&
          f.arena.cursor>before,"failed model reservation preserves ordinary growth");
}
void failed_or_late_capacity_reservation_does_not_freeze() {
    {
        Fixture f;
        IsleGameCommandPool pool{&f.arena,nullptr,0};
        const auto before=f.arena.cursor;
        check(isle_menu_reserve_native_capacity(f.game,&pool,std::numeric_limits<std::size_t>::max(),nullptr,0)==
              ISLE_GAME_EXHAUSTED && f.arena.cursor==before,
              "overflowed command reservation publishes no partial capacity");
        check(f.object()!=nullptr && f.arena.cursor>before,
              "failed reservation leaves the previous growth policy usable");
    }
    {
        Fixture f;
        check(isle_game_tick(f.game)==ISLE_GAME_OK,"run an actual first native update");
        IsleGameCommandPool pool{&f.arena,nullptr,0};
        const auto before=f.arena.cursor;
        check(isle_menu_reserve_native_capacity(f.game,&pool,1,nullptr,0)==ISLE_GAME_INVALID_STATE &&
              f.arena.cursor==before,"capacity cannot be frozen after execution starts");
        check(f.object()!=nullptr && f.arena.cursor>before,
              "late rejection leaves the previous growth policy intact");
    }
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
    IsleGameCommandPool pool{&f.arena,nullptr,0};
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
    frozen_native_capacity_rejects_growth_and_recycles();
    frozen_models_require_reserved_native_storage();
    frozen_model_shapes_recycle_without_growth();
    failed_model_reservation_keeps_old_storage_and_growth_policy();
    failed_or_late_capacity_reservation_does_not_freeze();
    self_end_is_a_real_continuation_fence();
    prefix_reentry_preserves_native_process_order();
    custom_commands_share_the_builtin_fifo();
    deferred_retirement_joins_before_userdata_recycling();
    check_native(owned_command_admission_regression(),
                 "custom admission preserves rejected credits and removes only an earlier consumed prefix");
    check_native(owned_command_completion_regression(),
                 "handler effects are consumed before status failure; later builtin/custom effects do not run");
    check_native(owned_command_append_regression(),
                 "late append runs in the same pass without recycling retained/countable nodes early");
    check_native(owned_command_delete_regression(),
                 "builtin deletion bypasses invalid custom admission and cannot replay its retained batch");
    check_native(owned_command_token_regression(),
                 "early or NULL token finish cannot close the callback, unlock recursion, tick or identity release");
    check_native(owned_command_cancel_regression(),
                 "cancellation retains only the active callback credit until actual handler return");
    check_native(owned_command_fault_regression(),
                 "native C fault disarms and returns from consumed-prefix cleanup before leaving its tick fence");
    check_native(owned_command_scope_regression(),
                 "owned scope validates actual current receiver/context and null-current end preserves FIFO continuation");
    check_native(owned_command_legacy_wait_regression(),
                 "legacy parked callback retains depth and credits across actual native continuation waits");
    check_native(owned_command_target_retirement_regression(),
                 "target retirement cannot free an external parked handler's credit before actual return");
    check_native(owned_command_caller_join_regression(),
                 "actual caller join abandons only consumed work and preserves a live target's suffix/observer depth");
    check_native(owned_command_legacy_fault_regression(),
                 "an unowned legacy native fault does not blanket-abort a callback before its caller joins");
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
