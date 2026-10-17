#include "item_source_fixture.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
void ok(IsleGameResult result) { require(result == ISLE_GAME_OK,"native Item operation rejected"); }
class World {
public:
    World() : bytes(65536) {
        require(isle_arena_init(&arena,bytes.data(),bytes.size()) == ISLE_OK,"arena init");
        ok(isle_game_create(&arena,&game));
    }
    ~World() { if (game && isle_game_destroy(game) != ISLE_GAME_OK) std::abort(); }
    World(const World&) = delete;
    World& operator=(const World&) = delete;
    IsleGameObject* object(std::uint32_t id) {
        IsleGameObject* result = nullptr;
        ok(isle_game_add_object(game,id,4,0,nullptr,nullptr,&result));
        return result;
    }
    std::vector<unsigned char> bytes;
    isle_arena arena{};
    IsleGame* game = nullptr;
};

void synchronous_end_publishes_source_suffix_but_stackful_end_does_not() {
    for (const auto kind : {ISLE_GAME_SYNCHRONOUS,ISLE_GAME_STACKFUL}) {
        World world;
        IsleItemSource item{};
        auto* object = world.object(300);
        ok(isle_item_source_bind(world.game,object,&item,&world));
        item.itemID = ISLE_ITEM_APPLE;
        item.state = ISLE_ITEM_FLYING;
        IsleGameProcess* process = nullptr;
        ok(isle_game_add_source_process(world.game,object,item_source_end_and_publish_suffix,kind,7,&process));
        ok(isle_game_tick(world.game));
        IsleItemSource* observed = nullptr;
        ok(isle_item_source_get(world.game,object,&observed));
        const bool synchronous = kind == ISLE_GAME_SYNCHRONOUS;
        require(observed == &item && observed->restTimer == (synchronous ? 1.0f : 0.5f) &&
                observed->flags == (synchronous ? ISLE_ITEM_BOUNCED|ISLE_ITEM_TOUCHED_GROUND : ISLE_ITEM_BOUNCED),
                "source self-end lost its kind-specific continuation and canonical side effects");
        ok(isle_game_tick(world.game));
        require(observed->restTimer == (synchronous ? 1.0f : 0.5f),
                "ended Item movement resumed on a later native frame");
        IsleGameCounts counts{};
        ok(isle_game_counts(world.game,&counts));
        require(counts.processes == 0,"self-ended Item process remained scheduled");
    }
}

void actor_and_item_payloads_cannot_be_reinterpreted_or_overwritten() {
    World world;
    IsleItemSource item{}, unused_item{};
    IsleActorSource actor{}, unused_actor{};
    isle_rng rng{1};
    auto* item_object = world.object(300);
    auto* actor_object = world.object(301);
    ok(isle_item_source_bind(world.game,item_object,&item,&world));
    ok(isle_actor_source_bind(world.game,actor_object,&actor,&rng,&world));
    item.restTimer = 12.5f;
    IsleItemSource* item_output = &unused_item;
    IsleActorSource* actor_output = &unused_actor;
    require(isle_item_source_get(world.game,actor_object,&item_output) == ISLE_GAME_INVALID_STATE && !item_output &&
            isle_actor_source_get(world.game,item_object,&actor_output) == ISLE_GAME_INVALID_STATE && !actor_output,
            "native source kind allowed an Item/Actor payload reinterpretation");
    require(isle_item_source_bind(world.game,actor_object,&unused_item,&world) == ISLE_GAME_INVALID_STATE &&
            isle_actor_source_bind(world.game,item_object,&unused_actor,&rng,&world) == ISLE_GAME_INVALID_STATE,
            "second source owner overwrote a different canonical payload kind");
    ok(isle_item_source_get(world.game,item_object,&item_output));
    ok(isle_actor_source_get(world.game,actor_object,&actor_output));
    require(item_output == &item && item_output->restTimer == 12.5f && actor_output == &actor,
            "rejected source-kind conversion changed live canonical ownership or Item state");
    auto* empty = world.object(302);
    require(isle_item_source_bind(world.game,empty,&item,&world) == ISLE_GAME_INVALID_STATE,
            "one canonical Item record was rebound to a second live object");
    item_output = &unused_item;
    require(isle_item_source_get(world.game,empty,&item_output) == ISLE_GAME_INVALID_STATE && !item_output,
            "failed canonical record rebind published an unowned Item");
}

void joined_held_identity_is_not_a_live_item_and_new_binding_is_kind_safe() {
    World world;
    IsleItemSource retired{}, replacement{};
    auto* old = world.object(300);
    ok(isle_item_source_bind(world.game,old,&retired,&world));
    ok(isle_game_object_retain_identity(world.game,old));
    ok(isle_game_delete_object(world.game,old));
    IsleItemSource* output = &retired;
    require(isle_item_source_get(world.game,old,&output) != ISLE_GAME_OK && !output,
            "a joined identity hold manufactured a live Item source payload");
    auto* while_held = world.object(301);
    require(while_held != old,"native identity hold allowed Item sender ABA reuse");
    ok(isle_game_object_release_identity(world.game,old));
    auto* reused = world.object(302);
    // 'reused' is a fresh live handle. The expired old handle is never queried.
    require(reused == old,"fixture did not exercise native source-kind storage reuse");
    ok(isle_item_source_bind(world.game,reused,&replacement,&world));
    ok(isle_item_source_get(world.game,reused,&output));
    require(output == &replacement && output != &retired,
            "fresh native incarnation exposed the retired Item's canonical record");
}
} // namespace

int main() {
    try {
        synchronous_end_publishes_source_suffix_but_stackful_end_does_not();
        actor_and_item_payloads_cannot_be_reinterpreted_or_overwritten();
        joined_held_identity_is_not_a_live_item_and_new_binding_is_kind_safe();
        std::cout << "Item canonical kind/lifetime and real C self-end semantics passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
