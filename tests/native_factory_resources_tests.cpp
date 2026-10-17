#include "native_factory_resources_fixture.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <new>
#include <stdexcept>
#include <thread>

namespace allocation_fixture {
std::atomic<std::size_t> live{0};
thread_local long allowance = -1;
}

// Executable-local allocation failure, not a production injection seam. These
// are the actual continuation/Process/std::thread allocations and deletions.
void* operator new(std::size_t bytes) {
    if (allocation_fixture::allowance >= 0) {
        if (allocation_fixture::allowance == 0) throw std::bad_alloc{};
        --allocation_fixture::allowance;
    }
    void* memory = std::malloc(bytes == 0 ? 1 : bytes);
    if (memory == nullptr) throw std::bad_alloc{};
    allocation_fixture::live.fetch_add(1, std::memory_order_relaxed);
    return memory;
}
void operator delete(void* memory) noexcept {
    if (memory != nullptr) {
        allocation_fixture::live.fetch_sub(1, std::memory_order_relaxed);
        std::free(memory);
    }
}
void* operator new[](std::size_t bytes) { return ::operator new(bytes); }
void operator delete[](void* memory) noexcept { ::operator delete(memory); }
void operator delete(void* memory, std::size_t) noexcept { ::operator delete(memory); }
void operator delete[](void* memory, std::size_t) noexcept { ::operator delete(memory); }

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
void ok(IsleGameResult result) { require(result == ISLE_GAME_OK, "native resource operation rejected"); }

class World {
public:
    World() {
        require(isle_arena_init(&arena, bytes.data(), bytes.size()) == ISLE_OK, "arena initialization");
        ok(isle_game_create(&arena, &game));
    }
    ~World() { if (isle_game_destroy(game) != ISLE_GAME_OK) std::abort(); }
    World(const World&) = delete;
    World& operator=(const World&) = delete;
    void exhaust() {
        isle_span remainder{};
        const auto available = arena.storage.size - arena.cursor;
        if (available != 0)
            require(isle_arena_allocate(&arena, available, 1, &remainder) == ISLE_OK, "arena exhaustion fixture");
    }
    alignas(std::max_align_t) std::array<unsigned char, 65536> bytes{};
    isle_arena arena{};
    IsleGame* game = nullptr;
};

std::array<IsleGameModelNodeInit, 3> initial_nodes() {
    std::array<IsleGameModelNodeInit, 3> nodes{};
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        nodes[i].parent = i == 0 ? UINT32_MAX : 0;
        nodes[i].transform.position = {static_cast<float>(i + 2), 4, 6};
        nodes[i].transform.scale = {1, 1, 1};
        nodes[i].matrix_kind = ISLE_GAME_MODEL_TRANSLATE;
    }
    return nodes;
}
IsleGameCounts counts(World& world) {
    IsleGameCounts value{};
    ok(isle_game_counts(world.game, &value));
    return value;
}
bool same_counts(IsleGameCounts a, IsleGameCounts b) {
    return a.objects == b.objects && a.processes == b.processes && a.sprites == b.sprites &&
        a.cameras == b.cameras && a.matrices == b.matrices && a.models == b.models &&
        a.model_nodes == b.model_nodes;
}
NativeFactoryTopology topology(World& world) {
    NativeFactoryTopology value{};
    native_factory_topology(world.game, &value);
    require(value.overflow == 0, "native OM bidirectional topology is invalid");
    return value;
}
bool same_topology(const NativeFactoryTopology& a, const NativeFactoryTopology& b) {
    if (a.object_count != b.object_count || a.process_count != b.process_count ||
        a.public_identity_credits != b.public_identity_credits) return false;
    for (std::size_t i = 0; i < a.object_count; ++i)
        if (a.objects[i] != b.objects[i] || a.ids[i] != b.ids[i] ||
            a.object_priorities[i] != b.object_priorities[i] || a.links[i] != b.links[i]) return false;
    for (std::size_t i = 0; i < a.process_count; ++i)
        if (a.processes[i] != b.processes[i] || a.process_objects[i] != b.process_objects[i] ||
            a.functions[i] != b.functions[i] || a.process_priorities[i] != b.process_priorities[i] ||
            a.kinds[i] != b.kinds[i]) return false;
    return true;
}
IsleGameObject* object(World& world, NativeFactoryFixture& fixture, std::uint32_t id,
    std::uint8_t link = 0, std::uint32_t priority = 20) {
    IsleGameObject* result = nullptr;
    ok(isle_game_add_object(world.game, id, link, priority, native_factory_update, &fixture, &result));
    native_factory_bind(result, &fixture);
    return result;
}
IsleGameProcess* process(World& world, IsleGameObject* owner, IsleGameSourceCallback function,
    IsleGameProcessKind kind, std::uint32_t priority) {
    IsleGameProcess* result = nullptr;
    ok(isle_game_add_source_process(world.game, owner, function, kind, priority, &result));
    return result;
}
IsleGameObject* publish(World& world, NativeFactoryFixture& fixture,
    IsleSourceFactoryResources& resources, std::uint32_t id = 13) {
    IsleGameObject* result = nullptr;
    ok(isle_source_factory_publish_object(&resources, id, 0, 20, native_factory_update, &fixture, &result));
    native_factory_bind(result, &fixture);
    require(result != nullptr && counts(world).objects != 0, "publication did not expose the actual object");
    return result;
}
IsleGameModel* attach(IsleSourceFactoryResources& resources, IsleGameObject* owner) {
    auto nodes = initial_nodes();
    IsleGameModel* model = nullptr;
    ok(isle_source_factory_attach_model(&resources, owner, nodes.data(), nodes.size(), &model));
    return model;
}
void events(const NativeFactoryFixture& fixture, std::initializer_list<int> expected) {
    require(fixture.failures == 0 && fixture.count == expected.size(), "compiled C callback extent/failure differs");
    std::size_t i = 0;
    for (int value : expected) require(fixture.events[i++] == value, "compiled C callback order differs");
}

void unpublished_resources_are_real_and_invisible() {
    World world;
    NativeFactoryFixture fixture{}; fixture.game = world.game;
    auto* old = object(world, fixture, 10);
    process(world, old, native_factory_animation, ISLE_GAME_SYNCHRONOUS, 7);
    const auto before_counts = counts(world);
    const auto before_topology = topology(world);
    const auto heap_before = allocation_fixture::live.load();
    IsleSourceFactoryResources resources{};
    const IsleSourceFactoryProcess descriptors[] = {
        {nullptr, native_factory_show, ISLE_GAME_STACKFUL, 7},
        {old, native_factory_remove, ISLE_GAME_STACKFUL, 4}
    };
    ok(isle_source_factory_prepare(world.game, 3, descriptors, 2, &resources));
    require(native_factory_ready_workers(&resources) == 2 && fixture.count == 0,
        "prepare did not park both actual workers before their C entries");
    require(same_counts(before_counts, counts(world)) && same_topology(before_topology, topology(world)),
        "prepare changed OM publication, metadata, identity credits or live counts");
    IsleGameObjectMetadata metadata{};
    require(isle_game_object_metadata(world.game, resources.object, &metadata) == ISLE_GAME_INVALID_ARGUMENT,
        "raw prepared object was publicly live");
    const IsleGameModelNode* nodes = nullptr;
    std::size_t count = 99;
    require(isle_game_model_peek_nodes(world.game, resources.model, &nodes, &count) == ISLE_GAME_INVALID_ARGUMENT &&
        nodes == nullptr && count == 0, "raw prepared model was publicly attached");
    IsleGameSourceCallback function = nullptr;
    require(isle_game_process_source_function(world.game, resources.processes[0], &function) == ISLE_GAME_INVALID_ARGUMENT &&
        function == nullptr, "raw prepared process was publicly live");
    require(isle_game_destroy(world.game) == ISLE_GAME_INVALID_STATE &&
        same_topology(before_topology, topology(world)), "destroy partially tore down an outstanding admission");
    std::array<IsleGameResult, 5> foreign{};
    IsleGameObject* foreign_object = old;
    IsleGameModel* foreign_model = resources.model;
    IsleGameProcess* foreign_process = resources.processes[1];
    const auto initial = initial_nodes();
    std::thread stranger([&] {
        foreign[0] = isle_source_factory_publish_object(&resources, 99, 0, 0, nullptr, nullptr, &foreign_object);
        foreign[1] = isle_source_factory_attach_model(&resources, old, initial.data(), initial.size(), &foreign_model);
        foreign[2] = isle_source_factory_publish_process(&resources, old, native_factory_remove,
            ISLE_GAME_STACKFUL, 4, &foreign_process);
        foreign[3] = isle_source_factory_cancel(&resources);
        foreign[4] = isle_source_factory_finish(&resources);
    });
    stranger.join();
    for (auto result : foreign) require(result == ISLE_GAME_WRONG_OWNER, "foreign thread consumed an active factory ticket");
    require(foreign_object == nullptr && foreign_model == nullptr && foreign_process == nullptr &&
        native_factory_ready_workers(&resources) == 2 && same_topology(before_topology, topology(world)),
        "wrong-owner factory consume exposed output or mutated reserved resources");
    auto copied = resources;
    require(isle_source_factory_cancel(&copied) == ISLE_GAME_INVALID_STATE &&
        isle_source_factory_finish(&copied) == ISLE_GAME_INVALID_STATE,
        "copied ticket acquired ownership of parked native resources");
    ok(isle_source_factory_cancel(&resources));
    require(!resources.active && allocation_fixture::live.load() == heap_before &&
        same_counts(before_counts, counts(world)) && same_topology(before_topology, topology(world)),
        "cancellation failed to join and reclaim actual unpublished resources");
    require(isle_source_factory_cancel(&resources) == ISLE_GAME_INVALID_STATE,
        "double cancellation reused an expired ticket");
    ok(isle_game_tick(world.game));
    events(fixture, {10, 101});
    const auto cursor = world.arena.cursor;
    ok(isle_source_factory_prepare(world.game, 3, descriptors, 2, &resources));
    require(world.arena.cursor == cursor, "cancelled raw storage was not returned to existing pools");
    ok(isle_source_factory_cancel(&resources));
}

struct Observation {
    NativeFactoryTopology topology{};
    std::array<std::uint32_t, 16> process_ids{};
    std::array<int, 128> events{};
    std::size_t event_count = 0;
};
Observation ordered_construction(bool prepared, IsleGameProcessKind update_kind) {
    World world;
    NativeFactoryFixture fixture{}; fixture.game = world.game;
    auto* a = object(world, fixture, 10);
    auto* b = object(world, fixture, 11, 0, 10);
    auto* c = object(world, fixture, 12, 1, 0);
    process(world, c, native_factory_animation, ISLE_GAME_SYNCHRONOUS, 7);
    process(world, a, native_factory_animation, ISLE_GAME_SYNCHRONOUS, 7);
    IsleSourceFactoryResources resources{};
    const IsleSourceFactoryProcess descriptors[] = {
        {nullptr, native_factory_animation, ISLE_GAME_SYNCHRONOUS, 3},
        {nullptr, native_factory_remove, update_kind, 7},
        {nullptr, native_factory_show, ISLE_GAME_STACKFUL, 7},
        {b, native_factory_remove, ISLE_GAME_SYNCHRONOUS, 4}
    };
    std::size_t admitted_cursor = 0;
    if (prepared) {
        const auto before = topology(world);
        ok(isle_source_factory_prepare(world.game, 3, descriptors, 4, &resources));
        admitted_cursor = world.arena.cursor;
        require(same_topology(before, topology(world)) &&
            native_factory_ready_workers(&resources) == (update_kind == ISLE_GAME_STACKFUL ? 2u : 1u),
            "four-process admission exposed future state or failed to park the configured kind");
    }
    IsleGameProcess* removed = nullptr;
    if (prepared) {
        ok(isle_source_factory_publish_process(&resources, b, native_factory_remove,
            ISLE_GAME_SYNCHRONOUS, 4, &removed));
        require(topology(world).object_count == 3 && counts(world).processes == 3 &&
            counts(world).models == 0, "old removal consumed future object/model/process hooks early");
    } else removed = process(world, b, native_factory_remove, ISLE_GAME_SYNCHRONOUS, 4);
    auto* future = prepared ? publish(world, fixture, resources) : object(world, fixture, 13);
    auto* following = object(world, fixture, 14);
    (void) following;
    const auto consume_cursor = world.arena.cursor;
    IsleGameModel* model = nullptr;
    if (prepared) model = attach(resources, future);
    else {
        auto nodes = initial_nodes();
        ok(isle_game_attach_model(world.game, future, nodes.data(), nodes.size(), &model));
    }
    for (std::size_t i = 0; i < 3; ++i) {
        if (prepared) {
            IsleGameProcess* result = nullptr;
            ok(isle_source_factory_publish_process(&resources, future, descriptors[i].function,
                descriptors[i].kind, descriptors[i].priority, &result));
            require(result == resources.processes[i], "consume replaced the prepared native process identity");
        } else process(world, future, descriptors[i].function, descriptors[i].kind, descriptors[i].priority);
    }
    if (prepared) {
        // Only the deliberately interleaved ordinary following object may grow
        // the arena after admission; model/process consume cannot allocate.
        require(consume_cursor > admitted_cursor && world.arena.cursor == consume_cursor,
            "prepared model/process consume allocated after its actual Source publication hook");
        const auto cursor = world.arena.cursor;
        ok(isle_source_factory_finish(&resources));
        require(!resources.active && world.arena.cursor == cursor, "finish retained an admission or allocated");
    }
    const IsleGameModelNode* nodes = nullptr;
    std::size_t node_count = 0;
    ok(isle_game_model_nodes(world.game, model, &nodes, &node_count));
    require(node_count == 3 && nodes[0].first_child == 1 && nodes[1].next_sibling == 2 &&
        nodes[2].parent == 0 && nodes[0].revision == 1 && nodes[0].fixed.m[1][2] == 0x00020004u &&
        nodes[0].fixed.m[1][3] == 0x00060001u,
        "prepared model did not use the real native hierarchy/translate matrix initialization");
    Observation result{};
    result.topology = topology(world);
    require(result.topology.object_count == 5 && result.topology.ids[0] == 10 &&
        result.topology.ids[1] == 13 && result.topology.ids[2] == 14 &&
        result.topology.ids[3] == 11 && result.topology.ids[4] == 12,
        "prepared object did not link at its current equal-priority insertion point");
    for (std::size_t i = 0; i < result.topology.process_count; ++i)
        ok(isle_game_object_id(world.game, result.topology.process_objects[i], &result.process_ids[i]));
    require(fixture.count == 0, "a prepared callback ran before its scheduler publication");
    ok(isle_game_tick(world.game));
    ok(isle_game_tick(world.game));
    require(fixture.failures == 0 && fixture.resumed == 1, "published C continuation did not wait/resume exactly once");
    result.event_count = fixture.count;
    for (std::size_t i = 0; i < fixture.count; ++i) result.events[i] = fixture.events[i];
    return result;
}
void publication_preserves_normal_om_order_and_model_recipe() {
    for (auto kind : {ISLE_GAME_SYNCHRONOUS, ISLE_GAME_STACKFUL}) {
        const auto normal = ordered_construction(false, kind);
        const auto prepared = ordered_construction(true, kind);
        require(normal.topology.process_count == prepared.topology.process_count &&
            normal.event_count == prepared.event_count, "prepared scheduling extent differs from normal OM");
        for (std::size_t i = 0; i < normal.topology.process_count; ++i)
            require(normal.process_ids[i] == prepared.process_ids[i] &&
                normal.topology.functions[i] == prepared.topology.functions[i] &&
                normal.topology.kinds[i] == prepared.topology.kinds[i] &&
                normal.topology.process_priorities[i] == prepared.topology.process_priorities[i],
                "prepared process changed exact normal OM order, Source identity, kind or priority");
        for (std::size_t i = 0; i < normal.event_count; ++i)
            require(normal.events[i] == prepared.events[i], "prepared C execution trace differs from ordinary native publication");
    }
}

void model_reservation_uses_actual_best_fit_released_blocks() {
    World world;
    NativeFactoryFixture fixture{}; fixture.game = world.game;
    auto* large_owner = object(world, fixture, 15);
    auto* small_owner = object(world, fixture, 16);
    auto nodes = initial_nodes();
    IsleGameModel *large = nullptr, *small = nullptr;
    ok(isle_game_attach_model(world.game, large_owner, nodes.data(), 3, &large));
    ok(isle_game_attach_model(world.game, small_owner, nodes.data(), 1, &small));
    ok(isle_game_delete_object(world.game, large_owner));
    ok(isle_game_delete_object(world.game, small_owner));
    const auto cursor = world.arena.cursor;
    IsleSourceFactoryResources resources{};
    ok(isle_source_factory_prepare(world.game, 1, nullptr, 0, &resources));
    require(resources.model == small && world.arena.cursor == cursor && counts(world).models == 0,
        "one-node prepare did not privately reserve the actual best-fit model block");
    ok(isle_source_factory_cancel(&resources));
    ok(isle_source_factory_prepare(world.game, 2, nullptr, 0, &resources));
    require(resources.model == large && world.arena.cursor == cursor,
        "two-node prepare reused an undersized block or bypassed the existing released block");
    IsleGameObject* owner = publish(world, fixture, resources, 17);
    IsleGameModel* model = nullptr;
    ok(isle_source_factory_attach_model(&resources, owner, nodes.data(), 2, &model));
    ok(isle_source_factory_finish(&resources));
    require(model == large && counts(world).model_nodes == 2 && world.arena.cursor == cursor,
        "zero-process cohort failed to consume its exact model/object resources without growth");
    ok(isle_game_tick(world.game)); events(fixture, {17});
}

void raw_allocation_boundaries_are_failure_atomic() {
    for (unsigned stage = 0; stage < 4; ++stage) {
        World world;
        NativeFactoryFixture fixture{}; fixture.game = world.game;
        auto* live = object(world, fixture, 20);
        process(world, live, native_factory_animation, ISLE_GAME_SYNCHRONOUS, 7);
        if (stage != 0) {
            auto* spare = object(world, fixture, 21);
            if (stage >= 2) {
                const auto nodes = initial_nodes();
                IsleGameModel* model = nullptr;
                ok(isle_game_attach_model(world.game, spare, nodes.data(), nodes.size(), &model));
                if (stage == 3) {
                    auto* record = process(world, spare, native_factory_animation, ISLE_GAME_SYNCHRONOUS, 3);
                    ok(isle_game_end_process(world.game, record));
                }
            }
            ok(isle_game_delete_object(world.game, spare));
        }
        world.exhaust();
        const auto cursor = world.arena.cursor;
        const auto before_counts = counts(world);
        const auto before_topology = topology(world);
        const auto heap_before = allocation_fixture::live.load();
        IsleSourceFactoryResources resources{};
        const IsleSourceFactoryProcess descriptors[] = {
            {live, native_factory_show, ISLE_GAME_STACKFUL, 7},
            {nullptr, native_factory_show, ISLE_GAME_STACKFUL, 7}
        };
        require(isle_source_factory_prepare(world.game, 3, descriptors, 2, &resources) == ISLE_GAME_EXHAUSTED,
            "actual object/model/process boundary did not reject with EXHAUSTED");
        require(!resources.active && world.arena.cursor == cursor &&
            allocation_fixture::live.load() == heap_before &&
            same_counts(before_counts, counts(world)) && same_topology(before_topology, topology(world)),
            "late raw-allocation failure leaked workers/pins or changed published native state");
        ok(isle_game_tick(world.game));
        events(fixture, {20, 201});
    }
}

void continuation_allocation_and_worker_start_failures_are_atomic() {
    World world;
    NativeFactoryFixture fixture{}; fixture.game = world.game;
    auto* old = object(world, fixture, 30);
    const IsleSourceFactoryProcess descriptors[] = {
        {old, native_factory_show, ISLE_GAME_STACKFUL, 7},
        {nullptr, native_factory_show, ISLE_GAME_STACKFUL, 7}
    };
    IsleSourceFactoryResources resources{};
    ok(isle_source_factory_prepare(world.game, 3, descriptors, 2, &resources));
    ok(isle_source_factory_cancel(&resources));
    const auto cursor = world.arena.cursor;
    const auto before_counts = counts(world);
    const auto before_topology = topology(world);
    const auto heap_before = allocation_fixture::live.load();
    bool succeeded = false;
    for (long allowance = 0; allowance < 32; ++allowance) {
        allocation_fixture::allowance = allowance;
        const auto result = isle_source_factory_prepare(world.game, 3, descriptors, 2, &resources);
        allocation_fixture::allowance = -1;
        if (result == ISLE_GAME_OK) {
            require(native_factory_ready_workers(&resources) == 2,
                "prepared source worker cohort was not parked");
            ok(isle_source_factory_cancel(&resources));
            succeeded = true;
        } else {
            require(result == ISLE_GAME_EXHAUSTED && !resources.active,
                "actual continuation allocation failure lost status or retained a joinable ticket");
        }
        require(isle_game_status(world.game) == ISLE_GAME_OK && world.arena.cursor == cursor &&
            allocation_fixture::live.load() == heap_before &&
            same_counts(before_counts, counts(world)) && same_topology(before_topology, topology(world)) &&
            fixture.count == 0 && fixture.resumed == 0,
            "recoverable worker-start admission poisoned execution, retained ownership or published a resource");
        if (succeeded) break;
    }
    require(succeeded, "bounded failure sweep never admitted both actual parked workers");
    ok(isle_game_delete_object(world.game, old));
    auto* reused = object(world, fixture, 31);
    require(reused == old, "worker-failure cleanup retained an old-object admission pin");
}

void preparation_validation_and_foreign_owner_are_nonsticky() {
    World world;
    NativeFactoryFixture fixture{}; fixture.game = world.game;
    auto* old = object(world, fixture, 40);
    auto* dead = object(world, fixture, 41);
    ok(isle_game_delete_object(world.game, dead));
    IsleSourceFactoryResources resources{};
    IsleSourceFactoryProcess descriptor{nullptr, native_factory_show, ISLE_GAME_STACKFUL, 7};
    const auto cursor = world.arena.cursor;
    const auto before = topology(world);
    require(isle_source_factory_prepare(world.game, 0, &descriptor, 1, &resources) == ISLE_GAME_INVALID_ARGUMENT &&
        isle_source_factory_prepare(world.game, 3, nullptr, 1, &resources) == ISLE_GAME_INVALID_ARGUMENT &&
        isle_source_factory_prepare(world.game, 3, &descriptor, 5, &resources) == ISLE_GAME_INVALID_ARGUMENT &&
        native_factory_invalid_kind(world.game, &resources) == ISLE_GAME_INVALID_ARGUMENT,
        "invalid count/kind/descriptor admission was accepted");
    descriptor.function = nullptr;
    require(isle_source_factory_prepare(world.game, 3, &descriptor, 1, &resources) == ISLE_GAME_INVALID_ARGUMENT,
        "NULL Source callback was admitted");
    descriptor.function = native_factory_show; descriptor.priority = 12;
    require(isle_source_factory_prepare(world.game, 3, &descriptor, 1, &resources) == ISLE_GAME_INVALID_ARGUMENT,
        "out-of-range Source priority was admitted");
    descriptor.priority = 7; descriptor.object = dead;
    require(isle_source_factory_prepare(world.game, 3, &descriptor, 1, &resources) == ISLE_GAME_INVALID_ARGUMENT,
        "dead explicit old object was admitted");
    descriptor.object = old;
    IsleGameResult foreign = ISLE_GAME_OK;
    std::thread other([&] { foreign = isle_source_factory_prepare(world.game, 3, &descriptor, 1, &resources); });
    other.join();
    require(foreign == ISLE_GAME_WRONG_OWNER && !resources.active &&
        world.arena.cursor == cursor && same_topology(before, topology(world)),
        "rejected admission mutated the arena or exposed state");
    ok(isle_game_tick(world.game)); events(fixture, {40});
    ok(isle_source_factory_prepare(world.game, 3, &descriptor, 1, &resources));
    require(isle_source_factory_finish(&resources) == ISLE_GAME_INVALID_STATE,
        "finish silently discarded unconsumed prepared storage");
    IsleGameObject* output = old;
    require(isle_source_factory_publish_object(&resources, 42, 32, 0, nullptr, nullptr, &output) ==
        ISLE_GAME_INVALID_ARGUMENT && output == nullptr, "invalid pre-publication link did not preserve exact error/null output");
    fixture.count = 0;
    ok(isle_game_tick(world.game)); events(fixture, {40});
    ok(isle_source_factory_cancel(&resources));
}

void duplicate_processes_consume_first_matching_resources() {
    World world;
    NativeFactoryFixture fixture{}; fixture.game = world.game;
    IsleSourceFactoryResources resources{};
    const IsleSourceFactoryProcess descriptors[] = {
        {nullptr, native_factory_animation, ISLE_GAME_SYNCHRONOUS, 7},
        {nullptr, native_factory_animation, ISLE_GAME_SYNCHRONOUS, 7}
    };
    ok(isle_source_factory_prepare(world.game, 3, descriptors, 2, &resources));
    auto* first = resources.processes[0];
    auto* second = resources.processes[1];
    auto* owner = publish(world, fixture, resources, 50);
    attach(resources, owner);
    IsleGameProcess* consumed = nullptr;
    ok(isle_source_factory_publish_process(&resources, owner, native_factory_animation, ISLE_GAME_SYNCHRONOUS, 7, &consumed));
    require(consumed == first && counts(world).processes == 1, "first identical descriptor did not consume first resource only");
    ok(isle_source_factory_publish_process(&resources, owner, native_factory_animation, ISLE_GAME_SYNCHRONOUS, 7, &consumed));
    require(consumed == second && counts(world).processes == 2, "second identical descriptor did not consume its remaining resource");
    ok(isle_source_factory_finish(&resources));
    require(isle_source_factory_publish_process(&resources, owner, native_factory_animation,
        ISLE_GAME_SYNCHRONOUS, 7, &consumed) == ISLE_GAME_INVALID_STATE && consumed == nullptr,
        "finished ticket consumed an expired process descriptor");
    ok(isle_game_tick(world.game)); events(fixture, {50, 501, 501});
}

void cancelled_ticket_ignores_independently_reused_published_storage() {
    World world;
    NativeFactoryFixture fixture{}; fixture.game = world.game;
    IsleSourceFactoryResources resources{};
    const IsleSourceFactoryProcess descriptors[] = {
        {nullptr, native_factory_animation, ISLE_GAME_SYNCHRONOUS, 7},
        {nullptr, native_factory_show, ISLE_GAME_STACKFUL, 7}
    };
    ok(isle_source_factory_prepare(world.game, 3, descriptors, 2, &resources));
    auto* owner = publish(world, fixture, resources, 55);
    auto* model = attach(resources, owner);
    IsleGameProcess* consumed = nullptr;
    ok(isle_source_factory_publish_process(&resources, owner, native_factory_animation,
        ISLE_GAME_SYNCHRONOUS, 7, &consumed));
    ok(isle_game_end_process(world.game, consumed));
    ok(isle_game_remove_model(world.game, model));
    auto* other = object(world, fixture, 56);
    auto nodes = initial_nodes();
    IsleGameModel* replacement_model = nullptr;
    ok(isle_game_attach_model(world.game, other, nodes.data(), nodes.size(), &replacement_model));
    auto* replacement_process = process(world, other, native_factory_remove, ISLE_GAME_SYNCHRONOUS, 4);
    require(replacement_model == model && replacement_process == consumed,
        "fixture did not actually recycle independently retired published resources");
    const auto before_counts = counts(world);
    const auto before_topology = topology(world);
    ok(isle_source_factory_cancel(&resources));
    require(same_counts(before_counts, counts(world)) && same_topology(before_topology, topology(world)),
        "ticket cancellation reclaimed storage already transferred to normal native owners");
    ok(isle_game_tick(world.game)); events(fixture, {55, 56, 564});
}

void published_resources_are_not_cancelled_and_fault_is_sticky() {
    World world;
    NativeFactoryFixture fixture{}; fixture.game = world.game;
    auto* old = object(world, fixture, 60);
    IsleSourceFactoryResources resources{};
    const IsleSourceFactoryProcess descriptors[] = {
        {old, native_factory_show, ISLE_GAME_STACKFUL, 7},
        {nullptr, native_factory_show, ISLE_GAME_STACKFUL, 7}
    };
    ok(isle_source_factory_prepare(world.game, 3, descriptors, 2, &resources));
    IsleGameProcess* published = nullptr;
    ok(isle_source_factory_publish_process(&resources, old, native_factory_show, ISLE_GAME_STACKFUL, 7, &published));
    auto* future = publish(world, fixture, resources, 61);
    auto* model = attach(resources, future);
    const auto before_counts = counts(world);
    const auto before_topology = topology(world);
    IsleGameObject* duplicate = old;
    require(isle_source_factory_publish_object(&resources, 99, 1, 0, nullptr, nullptr, &duplicate) ==
        ISLE_GAME_INVALID_STATE && duplicate == nullptr,
        "double object consume overwrote a live native object");
    ok(isle_source_factory_cancel(&resources));
    require(same_counts(before_counts, counts(world)) && same_topology(before_topology, topology(world)),
        "cancel erased published object/model/process resources");
    require(isle_game_tick(world.game) == ISLE_GAME_EXECUTION_FAILURE && fixture.count == 0,
        "publication invariant failure did not remain terminal without executing a callback");
    require(isle_game_delete_object(world.game, future) == ISLE_GAME_EXECUTION_FAILURE &&
        counts(world).objects == 1 && counts(world).models == 0,
        "sticky public delete lost actual published model/object retirement");
    const IsleGameModelNode* nodes = nullptr; std::size_t count = 0;
    require(isle_game_model_nodes(world.game, model, &nodes, &count) == ISLE_GAME_INVALID_ARGUMENT,
        "cancelled ticket retained a model after real object retirement");
    require(isle_game_delete_object(world.game, old) == ISLE_GAME_EXECUTION_FAILURE &&
        counts(world).objects == 0 && counts(world).processes == 0 && fixture.resumed == 0,
        "sticky public delete did not actually join the published parked worker");
}

void malformed_postpublication_model_and_process_are_terminal() {
    for (bool model_failure : {false, true}) {
        World world;
        NativeFactoryFixture fixture{}; fixture.game = world.game;
        IsleSourceFactoryResources resources{};
        const IsleSourceFactoryProcess descriptor{nullptr, native_factory_show, ISLE_GAME_STACKFUL, 7};
        ok(isle_source_factory_prepare(world.game, 3, &descriptor, 1, &resources));
        auto* owner = publish(world, fixture, resources, 70);
        const auto before = counts(world);
        if (model_failure) {
            auto nodes = initial_nodes(); nodes[1].parent = 2;
            IsleGameModel* output = resources.model;
            require(isle_source_factory_attach_model(&resources, owner, nodes.data(), nodes.size(), &output) ==
                ISLE_GAME_INVALID_ARGUMENT && output == nullptr, "malformed prepared hierarchy lost its actual native status");
        } else {
            IsleGameProcess* output = resources.processes[0];
            require(isle_source_factory_publish_process(&resources, owner, native_factory_animation,
                ISLE_GAME_STACKFUL, 7, &output) == ISLE_GAME_INVALID_STATE && output == nullptr,
                "mismatched Source callback consumed an unrelated prepared process");
        }
        require(same_counts(before, counts(world)), "invalid prepared consume changed live resource counts");
        ok(isle_source_factory_cancel(&resources));
        require(isle_game_tick(world.game) == ISLE_GAME_EXECUTION_FAILURE && fixture.count == 0,
            "malformed post-publication consume was reported as recoverable");
        require(isle_game_delete_object(world.game, owner) == ISLE_GAME_EXECUTION_FAILURE,
            "cleanup cleared the sticky prepared-publication fault");
    }
}

void retired_old_and_future_identities_cannot_be_reused_under_a_ticket() {
    World world;
    NativeFactoryFixture fixture{}; fixture.game = world.game;
    auto* old = object(world, fixture, 80);
    IsleSourceFactoryResources resources{};
    const IsleSourceFactoryProcess descriptor{old, native_factory_show, ISLE_GAME_STACKFUL, 7};
    ok(isle_source_factory_prepare(world.game, 3, &descriptor, 1, &resources));
    ok(isle_game_delete_object(world.game, old));
    auto* other = object(world, fixture, 81);
    require(other != old, "prepared old-object descriptor silently retargeted a reused physical object");
    auto* future = publish(world, fixture, resources, 82);
    attach(resources, future);
    ok(isle_game_delete_object(world.game, future));
    auto* following = object(world, fixture, 83);
    require(following != future, "published ticket root was reused before reservation ownership ended");
    IsleGameProcess* output = nullptr;
    require(isle_source_factory_publish_process(&resources, old, native_factory_show,
        ISLE_GAME_STACKFUL, 7, &output) == ISLE_GAME_INVALID_ARGUMENT && output == nullptr,
        "retired old target accepted a delayed prepared process");
    ok(isle_source_factory_cancel(&resources));
    require(counts(world).objects == 2 && counts(world).models == 0 && counts(world).processes == 0,
        "cancel on retired roots changed unrelated published resources");
    require(isle_game_tick(world.game) == ISLE_GAME_EXECUTION_FAILURE,
        "stale post-publication target did not remain terminal");
}

void native_fault_cleanup_and_running_c_admission_use_real_owner_rendezvous() {
    {
        World world;
        NativeFactoryFixture fixture{}; fixture.game = world.game;
        IsleSourceFactoryResources resources{};
        fixture.nested_resources = &resources;
        auto* owner = object(world, fixture, 90);
        process(world, owner, native_factory_nested_prepare, ISLE_GAME_STACKFUL, 7);
        const auto before_heap = allocation_fixture::live.load();
        ok(isle_game_tick(world.game));
        events(fixture, {90, 900});
        require(fixture.nested_result == ISLE_GAME_OK && !resources.active &&
            allocation_fixture::live.load() < before_heap && counts(world).processes == 0,
            "running C prepare/cancel failed its actual owner rendezvous or retained parked workers");
    }
    {
        World world;
        NativeFactoryFixture fixture{}; fixture.game = world.game;
        auto* owner = object(world, fixture, 91);
        process(world, owner, native_factory_illegal_wait, ISLE_GAME_SYNCHRONOUS, 7);
        IsleSourceFactoryResources resources{};
        const IsleSourceFactoryProcess descriptor{nullptr, native_factory_show, ISLE_GAME_STACKFUL, 7};
        const auto before_heap = allocation_fixture::live.load();
        ok(isle_source_factory_prepare(world.game, 3, &descriptor, 1, &resources));
        require(isle_game_tick(world.game) == ISLE_GAME_EXECUTION_FAILURE && fixture.resumed == 0,
            "compiled C fault did not leave the prepared worker unentered");
        ok(isle_source_factory_cancel(&resources));
        require(allocation_fixture::live.load() == before_heap &&
            isle_game_status(world.game) == ISLE_GAME_EXECUTION_FAILURE &&
            isle_game_tick(world.game) == ISLE_GAME_EXECUTION_FAILURE,
            "faulted-game cancellation failed a real join, cleared native fault or resumed source");
        require(isle_source_factory_prepare(world.game, 3, &descriptor, 1, &resources) == ISLE_GAME_EXECUTION_FAILURE &&
            !resources.active, "faulted game admitted new unpublished work");
    }
}
} // namespace

int main() {
    try {
        unpublished_resources_are_real_and_invisible();
        publication_preserves_normal_om_order_and_model_recipe();
        model_reservation_uses_actual_best_fit_released_blocks();
        raw_allocation_boundaries_are_failure_atomic();
        continuation_allocation_and_worker_start_failures_are_atomic();
        preparation_validation_and_foreign_owner_are_nonsticky();
        duplicate_processes_consume_first_matching_resources();
        cancelled_ticket_ignores_independently_reused_published_storage();
        published_resources_are_not_cancelled_and_fault_is_sticky();
        malformed_postpublication_model_and_process_are_terminal();
        retired_old_and_future_identities_cannot_be_reused_under_a_ticket();
        native_fault_cleanup_and_running_c_admission_use_real_owner_rendezvous();
        std::cout << "native factory resource tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        allocation_fixture::allowance = -1;
        std::cerr << "native factory resource tests: " << error.what() << '\n';
        return 1;
    }
}
