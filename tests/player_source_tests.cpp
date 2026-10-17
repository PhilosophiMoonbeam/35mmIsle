#include "player_source_fixture.h"
#include "actor_source.h"
#include "item_source.h"
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
void ok(IsleGameResult result, std::source_location location=std::source_location::current()) {
    if (result != ISLE_GAME_OK)
        throw std::runtime_error(std::string("native Player context operation rejected at ") +
            location.file_name() + ":" + std::to_string(location.line()) + " result=" +
            std::to_string(static_cast<int>(result)));
}
class World final {
public:
    World() : bytes(65536) {
        require(isle_arena_init(&arena,bytes.data(),bytes.size()) == ISLE_OK,"arena initialization failed");
        ok(isle_game_create(&arena,&game));
        ok(isle_player_source_prepare(game,&source,&rng,this,10));
        ok(isle_player_source_activate(&source));
    }
    ~World() {
        for (std::size_t i=0;i<object_count;++i)
            if (isle_game_delete_object(game,objects[i]) != ISLE_GAME_OK) std::abort();
        if (isle_player_source_deactivate(&source) != ISLE_GAME_OK ||
            isle_game_destroy(game) != ISLE_GAME_OK) std::abort();
    }
    World(const World&) = delete;
    World& operator=(const World&) = delete;
    IsleGameObject* object(std::uint32_t id) {
        require(object_count < objects.size(),"fixture object capacity exceeded");
        IsleGameObject* object=nullptr;
        ok(isle_game_add_object(game,id,9,0x80000000u,nullptr,nullptr,&object));
        objects[object_count++]=object;
        return object;
    }
    IsleGameCounts counts() {
        IsleGameCounts result{}; ok(isle_game_counts(game,&result)); return result;
    }
    isle_rng rng{1};
    IslePlayerSource source{};
    std::vector<unsigned char> bytes;
    isle_arena arena{};
    IsleGame* game=nullptr;
    std::array<IsleGameObject*,8> objects{};
    std::size_t object_count=0;
};

void canonical_root_cannot_alias_actor_item_foreign_or_second_context() {
    IsleActorSource actor{};
    IsleItemSource item{};
    World world;
    auto* root=world.object(7);
    ok(isle_player_source_bind(world.game,root,&world.source));
    auto* actor_object=world.object(8);
    auto* item_object=world.object(9);
    ok(isle_actor_source_bind(world.game,actor_object,&actor,&world.rng,&world));
    ok(isle_item_source_bind(world.game,item_object,&item,&world));
    IslePlayerSource* player_out=&world.source;
    for (auto* object : {actor_object,item_object}) {
        require(isle_player_source_get(world.game,object,&player_out) == ISLE_GAME_INVALID_STATE && !player_out,
                "Actor or Item payload was reinterpreted as canonical Player state");
        player_out=&world.source;
    }
    IsleActorSource* actor_out=&actor;
    IsleItemSource* item_out=&item;
    require(isle_actor_source_get(world.game,root,&actor_out) == ISLE_GAME_INVALID_STATE && !actor_out &&
            isle_item_source_get(world.game,root,&item_out) == ISLE_GAME_INVALID_STATE && !item_out,
            "canonical Player payload was reinterpreted as Actor or Item");
    require(isle_actor_source_bind(world.game,root,&actor,&world.rng,&world) == ISLE_GAME_INVALID_STATE &&
            isle_item_source_bind(world.game,root,&item,&world) == ISLE_GAME_INVALID_STATE &&
            isle_player_source_bind(world.game,world.object(10),&world.source) == ISLE_GAME_INVALID_STATE,
            "canonical source ownership was overwritten or rebound");
    IslePlayerSource second{};
    ok(isle_player_source_prepare(world.game,&second,&world.rng,&second,10));
    require(isle_player_source_activate(&second) == ISLE_GAME_INVALID_STATE,
            "a second active Player context replaced the canonical per-game context");
    // Native OM permits one live game. A real foreign thread exercises its
    // owner boundary without inventing a second concurrent game or stale handle.
    IsleGameResult foreign_result=ISLE_GAME_OK;
    player_out=&world.source;
    std::thread foreign([&] { foreign_result=isle_player_source_get(world.game,root,&player_out); });
    foreign.join();
    require(foreign_result == ISLE_GAME_WRONG_OWNER && !player_out,
            "foreign thread acquired canonical Player state");
    ok(isle_player_source_get(world.game,root,&player_out));
    require(player_out == &world.source && player_out->object == root,
            "rejected ownership conversions changed the actual Player binding");
    const auto before=world.counts();
    require(isle_game_destroy(world.game) == ISLE_GAME_INVALID_STATE,
            "game destruction invalidated an active retained Player context");
    const auto after=world.counts();
    require(after.objects == before.objects && after.processes == before.processes,
            "rejected game destruction partially retired canonical Player objects");
}

void helper_uses_game_context_across_real_wait_without_payload_cast() {
    int unrelated_userdata=17;
    World world;
    auto* root=world.object(7);
    ok(isle_player_source_bind(world.game,root,&world.source));
    auto* helper=world.object(8);
    player_source_helper_userdata(helper,&unrelated_userdata);
    ok(isle_player_source_call(&world.source,player_source_cold_entry,helper));
    require(world.source.ThrowItemTimeout == 77 && world.source.nonwaiting_call_frame == nullptr &&
            world.counts().processes == 0 && unrelated_userdata == 17,
            "original cold callback required a fake process, userdata cast or lingering execution context");
    IsleGameProcess* process=nullptr;
    ok(isle_game_add_source_process(world.game,helper,player_source_helper_wait,ISLE_GAME_STACKFUL,9,&process));
    ok(isle_game_tick(world.game));
    require(world.source.ThrowItemTimeout == 41 && world.counts().processes == 1,
            "helper did not suspend with the canonical Player state retained");
    ok(isle_game_tick(world.game));
    require(world.source.ThrowItemTimeout == 42 && world.counts().processes == 0 && unrelated_userdata == 17,
            "C helper lost its context, resumed a self-ended suffix, or cast unrelated helper userdata");
    IslePlayerSource* output=&world.source;
    require(isle_player_source_get(world.game,helper,&output) == ISLE_GAME_INVALID_STATE && !output,
            "context lookup manufactured a root Player payload for a helper");
}

void nonwaiting_c_fences_restore_ownership_and_preserve_native_fault_status() {
    for (auto callback : {player_source_nested_failure,player_source_forbidden_wait}) {
        World world;
        bool destroyed=false;
        struct Owner { bool& destroyed; ~Owner() { destroyed=true; } };
        {
            Owner owner{destroyed};
            const auto expected=callback == player_source_nested_failure ? ISLE_GAME_INVALID_ARGUMENT : ISLE_GAME_INVALID_STATE;
            require(isle_player_source_call(&world.source,callback,nullptr) == expected &&
                    world.source.status == expected && world.source.ThrowItemTimeout == 41 &&
                    world.source.nonwaiting_call_frame == nullptr && !destroyed,
                    "local C failure skipped a fence, executed a rejected suffix, or crossed its owning C++ caller");
            require(isle_game_status(world.game) == ISLE_GAME_OK && world.counts().processes == 0,
                    "nonwaiting admission failure fabricated an OM fault or continuation");
            require(isle_player_source_bind(world.game,world.object(7),&world.source) == expected &&
                    world.source.object == nullptr,
                    "failed Player context published a new root binding");
            require(isle_player_source_call(&world.source,player_source_nested_failure,nullptr) == expected &&
                    world.source.ThrowItemTimeout == 41,
                    "a sticky source failure reentered C execution or changed its original status");
        }
        require(destroyed,"returned C failure bypassed C++ owner destruction");
    }
}
}
int main() {
    try {
        canonical_root_cannot_alias_actor_item_foreign_or_second_context();
        helper_uses_game_context_across_real_wait_without_payload_cast();
        nonwaiting_c_fences_restore_ownership_and_preserve_native_fault_status();
        std::cout << "Player context ownership, C fences and real continuation boundaries passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n'; return 1;
    }
}
