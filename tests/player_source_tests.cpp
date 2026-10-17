#include "player_source_fixture.h"
#include "actor_source.h"
#include "item_source.h"
#include "player_camera_viewport.h"
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
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
    IsleGameObject* object(std::uint32_t id, std::uint8_t link=9,
        std::uint32_t priority=0x80000000u, IsleGameCallback update=nullptr, void* user=nullptr) {
        require(object_count < objects.size(),"fixture object capacity exceeded");
        IsleGameObject* object=nullptr;
        ok(isle_game_add_object(game,id,link,priority,update,user,&object));
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

void count_native_callback(IsleGameObject*, void* context) {
    ++*static_cast<unsigned*>(context);
}
void object_process_controls_preserve_real_waits_updaters_and_current_context() {
    World world;
    auto* root=world.object(7);
    ok(isle_player_source_bind(world.game,root,&world.source));
    unsigned updates=0,synchronous=0;
    auto* helper=world.object(8,9,0x80000000u,count_native_callback,&updates);
    IsleGameProcess *waiting=nullptr,*ordinary=nullptr,*self=nullptr;
    ok(isle_game_add_source_process(world.game,helper,player_source_helper_wait,
        ISLE_GAME_STACKFUL,9,&waiting));
    ok(isle_game_add_process(world.game,helper,count_native_callback,&synchronous,
        ISLE_GAME_SYNCHRONOUS,8,&ordinary));
    ok(isle_game_tick(world.game));
    require(world.source.ThrowItemTimeout==41 && synchronous==1 && updates==1,
        "actual mixed native processes did not enter their first update/wait");
    PlayerObjectControl control{helper,1,0,0,nullptr,0};
    ok(isle_player_source_call(&world.source,player_source_pause_all,&control));
    for (unsigned i=0;i<3;++i) ok(isle_game_tick(world.game));
    require(world.source.ThrowItemTimeout==41 && synchronous==1 && updates==4 && control.completed==1,
        "object-wide pause advanced waits, missed a process kind, or paused fnUpdate");
    control.paused=0;
    ok(isle_player_source_call(&world.source,player_source_pause_all,&control));
    ok(isle_game_tick(world.game));
    require(world.source.ThrowItemTimeout==42 && synchronous==2 && updates==5 &&
        world.counts().processes==1 && control.completed==2,
        "object-wide resume lost remaining wait, self-end, or synchronous execution");
    auto* other=world.object(9);
    ok(isle_game_add_source_process(world.game,other,player_source_pause_current,
        ISLE_GAME_SYNCHRONOUS,7,&self));
    ok(isle_game_tick(world.game));
    require(world.source.ThrowItemTimeout==44,
        "NULL object failed to resolve actual current callback or stopped its current tail");
    ok(isle_game_tick(world.game));
    require(world.source.ThrowItemTimeout==44 && synchronous==4 && updates==7,
        "self-pause selected canonical Root or unrelated helper instead of current object");
    control.object=other;
    ok(isle_player_source_call(&world.source,player_source_pause_all,&control));
    ok(isle_game_tick(world.game));
    require(world.source.ThrowItemTimeout==46 && synchronous==5 && updates==8,
        "resumed actual current-object process failed its same source continuation");
}

void ordered_source_lookup_preserves_link_priority_absence_and_checked_boundaries() {
    {
        World world;
        world.object(42,9,10);
        auto* high=world.object(42,9,20);
        auto* other_link=world.object(42,8,30);
        PlayerObjectControl query{nullptr,0,9,42,nullptr,0};
        ok(isle_player_source_call(&world.source,player_source_find_object,&query));
        require(query.found==high && query.completed==1,
            "source lookup ignored actual native link/priority order");
        query.link=8;
        ok(isle_player_source_call(&world.source,player_source_find_object,&query));
        require(query.found==other_link && query.completed==2,
            "source lookup searched a separate owner cache or the wrong update link");
        query.id=99;
        ok(isle_player_source_call(&world.source,player_source_find_object,&query));
        require(!query.found && query.completed==3 && world.source.status==ISLE_GAME_OK,
            "legal missing source object fabricated a handle or fault");
        auto* main_camera=world.object(UINT32_MAX,0,0x80000000u);
        query.link=0; query.id=UINT32_MAX;
        ok(isle_player_source_call(&world.source,player_source_find_object,&query));
        require(query.found==main_camera && query.completed==4,
            "source lookup lost the main camera's unsigned representation of object ID -1");
    }
    for (int link : {-1,32}) {
        World world;
        auto* object=world.object(42);
        PlayerObjectControl query{nullptr,0,link,42,object,0};
        require(isle_player_source_call(&world.source,player_source_find_object,&query)==ISLE_GAME_INVALID_ARGUMENT &&
            world.source.status==ISLE_GAME_INVALID_ARGUMENT && query.found==object && !query.completed &&
            !world.source.nonwaiting_call_frame && isle_game_status(world.game)==ISLE_GAME_OK,
            "invalid source link escaped its C fence, changed output or poisoned native Game");
    }
    {
        World world;
        world.object(42);
        PlayerObjectControl control{nullptr,1,0,0,nullptr,0};
        require(isle_player_source_call(&world.source,player_source_pause_all,&control)==ISLE_GAME_INVALID_ARGUMENT &&
            !control.completed && !world.source.nonwaiting_call_frame && isle_game_status(world.game)==ISLE_GAME_OK,
            "cold NULL process target manufactured a current object outside native dispatch");
    }
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

void sprite_borrows_reacquire_actual_membership_after_a_real_wait() {
    int unrelated_userdata = 17;
    World world;
    auto* root = world.object(7);
    ok(isle_player_source_bind(world.game,root,&world.source));
    auto* helper = world.object(8);
    player_source_helper_userdata(helper,&unrelated_userdata);
    auto* target = world.object(9);
    auto* distractor = world.object(10);
    world.source.D_80382C38_523048 = target;
    IsleSprite initial{};
    initial.attr = 0x8141; initial.alpha = 80;
    IsleGameSprite *first = nullptr, *survivor = nullptr;
    ok(isle_game_add_sprite(world.game,target,&initial,&first));
    initial.attr |= ISLE_SP_HIDDEN | ISLE_SP_SCALE;
    ok(isle_game_add_sprite(world.game,target,&initial,&survivor));
    IsleSprite *first_view = nullptr, *survivor_view = nullptr;
    ok(isle_game_sprite_view(world.game,first,&first_view));
    ok(isle_game_sprite_view(world.game,survivor,&survivor_view));
    IsleGameProcess* process = nullptr;
    ok(isle_game_add_source_process(world.game,helper,player_source_sprite_wait,
        ISLE_GAME_STACKFUL,9,&process));
    const auto used = world.arena.cursor;
    ok(isle_game_tick(world.game));
    require(first_view->x == -32768 && first_view->y == -1 && first_view->alpha == 254 &&
                first_view->attr == 0x8155 && first_view->scalex == 0.5f &&
                survivor_view->alpha == 80 && world.source.ThrowItemTimeout == 41 &&
                world.counts().processes == 1,
            "C sprite operations did not target the actual head before suspension");
    ok(isle_game_remove_sprite(world.game,first));
    // The released storage now belongs to another object. The waiting body
    // must reacquire target's surviving member, not revive its expired borrow.
    IsleGameSprite* recycled = nullptr;
    ok(isle_game_add_sprite(world.game,distractor,&initial,&recycled));
    IsleSprite* distractor_view = nullptr;
    ok(isle_game_sprite_view(world.game,recycled,&distractor_view));
    ok(isle_game_tick(world.game));
    require(survivor_view->x == 0 && survivor_view->y == -1 && survivor_view->alpha == 17 &&
                survivor_view->red == 4 && survivor_view->green == 5 && survivor_view->blue == 6 &&
                survivor_view->attr == 0x8141 && survivor_view->scalex == 3.0f &&
                survivor_view->scaley == 0.25f && distractor_view->alpha == 80 &&
                distractor_view->attr == 0x8155 && world.source.ThrowItemTimeout == 42 &&
                world.counts().processes == 0 && world.arena.cursor == used && unrelated_userdata == 17,
            "resumed C helper cached/copied a sprite, enabled scale on raw writes, or cast helper payload");
    PlayerSpriteBorrow request{target,nullptr,nullptr,0,0,0};
    bool wrong_owner = false;
    std::thread foreign([&] {
        wrong_owner = isle_player_source_call(&world.source,player_source_sprite_borrow,&request) ==
            ISLE_GAME_WRONG_OWNER;
    });
    foreign.join();
    require(wrong_owner && request.entered == 0 && world.source.nonwaiting_call_frame == nullptr,
            "foreign nonwaiting call entered a sprite borrow or installed a C fence");
    ok(isle_player_source_call(&world.source,player_source_sprite_borrow,&request));
    require(request.entered == 1 && request.completed == 1 && survivor_view->alpha == 132 &&
                survivor_view->red == 33 && survivor_view->attr == 0x8141 &&
                world.source.nonwaiting_call_frame == nullptr && world.arena.cursor == used,
            "cold C sprite borrow used a fake OM context or failed to modify the live attachment");
    IsleGameSprite* selected = nullptr;
    ok(isle_game_object_sprite(world.game,target,&selected));
    IsleGameProcess* fade = nullptr;
    ok(isle_game_add_intro_fade(world.game,target,selected,4,5,&fade));
    ok(isle_game_tick(world.game));
    require(survivor_view->red == 191 && survivor_view->green == 191 &&
                survivor_view->blue == 191 && survivor_view->alpha == 132 &&
                distractor_view->alpha == 80,
            "the authoritative C sprite consumer did not observe the checked native borrow");
}

void sprite_borrow_failures_use_only_the_canonical_nested_c_fence() {
    for (int failure : {0,1,2,3,4}) {
        World world;
        auto* target = world.object(7);
        IsleGameSprite* sprite = nullptr;
        IsleSprite initial{};
        initial.red = 11; initial.alpha = 22; initial.attr = ISLE_SP_HIDDEN;
        if (failure >= 1 && failure <= 3)
            ok(isle_game_add_sprite(world.game,target,&initial,&sprite));
        if (failure == 1) ok(isle_game_remove_sprite(world.game,sprite));
        if (failure == 4) {
            IsleGameCamera* camera = nullptr;
            ok(isle_game_attach_camera(world.game,target,&camera));
        }
        IslePlayerSource second{};
        ok(isle_player_source_prepare(world.game,&second,&world.rng,&second,10));
        PlayerSpriteBorrow request{target,sprite,
            failure == 2 || failure == 3 ? &second : nullptr,
            failure == 1 || failure == 3,0,0};
        const auto expected = failure == 2 || failure == 3 ?
            ISLE_GAME_INVALID_STATE : ISLE_GAME_INVALID_ARGUMENT;
        const auto used = world.arena.cursor;
        const auto before = world.counts();
        bool destroyed = false;
        struct Owner { bool& destroyed; ~Owner() { destroyed = true; } };
        {
            Owner owner{destroyed};
            require(isle_player_source_call(&world.source,player_source_sprite_nested_borrow,&request) ==
                        expected && world.source.status == expected && request.entered == 1 &&
                        request.completed == 0 && world.source.ThrowItemTimeout == 41 &&
                        world.source.nonwaiting_call_frame == nullptr && !destroyed,
                    "rejected sprite borrow executed its suffix, lost error status or crossed owning C++");
        }
        require(destroyed && second.status == ISLE_GAME_OK && second.nonwaiting_call_frame == nullptr &&
                    isle_game_status(world.game) == ISLE_GAME_OK && world.arena.cursor == used &&
                    world.counts().sprites == before.sprites && world.counts().processes == before.processes,
                "sprite failure trusted a noncanonical fence, mutated another Source or faulted native cleanup");
        if (failure == 2 || failure == 3) {
            IsleSprite* view = nullptr;
            ok(isle_game_sprite_view(world.game,sprite,&view));
            require(view->red == 11 && view->alpha == 22 && view->attr == ISLE_SP_HIDDEN,
                    "rejected noncanonical Source changed the real sprite before admission");
        }
        require(isle_player_source_call(&world.source,player_source_sprite_borrow,&request) == expected &&
                    request.entered == 1 && request.completed == 0,
                "sticky sprite admission failure reentered C or changed its original result");
    }
}

void sprite_self_delete_borrows_end_at_actual_native_retirement() {
    World world;
    auto* root = world.object(7);
    ok(isle_player_source_bind(world.game,root,&world.source));
    // This native object is retired by its own C body; the fixture's retained
    // object list must not try to delete the already-expired handle.
    IsleGameObject* self = nullptr;
    ok(isle_game_add_object(world.game,8,9,0,nullptr,nullptr,&self));
    IsleGameSprite* sprite = nullptr;
    ok(isle_game_add_sprite(world.game,self,nullptr,&sprite));
    IsleGameProcess* process = nullptr;
    ok(isle_game_add_source_process(world.game,self,player_source_sprite_delete_self,
        ISLE_GAME_STACKFUL,9,&process));
    ok(isle_game_tick(world.game));
    IsleSprite* view = nullptr;
    require(world.source.status == ISLE_GAME_OK && world.source.ThrowItemTimeout == 41 &&
                world.counts().objects == 1 && world.counts().sprites == 0 &&
                world.counts().processes == 0 &&
                isle_game_sprite_view(world.game,sprite,&view) == ISLE_GAME_INVALID_ARGUMENT && !view,
            "checked sprite borrow ended before deferred self-delete or survived joined native retirement");
    ok(isle_game_tick(world.game));
    require(world.source.ThrowItemTimeout == 41,
            "a self-deleted sprite continuation resumed an expired native view");
}

void sprite_borrow_without_active_source_faults_only_inside_the_native_c_guard() {
    std::vector<unsigned char> bytes(65536);
    isle_arena arena{};
    require(isle_arena_init(&arena,bytes.data(),bytes.size()) == ISLE_OK,"arena initialization failed");
    IsleGame* game = nullptr;
    ok(isle_game_create(&arena,&game));
    isle_rng rng{1};
    IslePlayerSource source{};
    ok(isle_player_source_prepare(game,&source,&rng,&source,10));
    IsleGameObject* owner = nullptr;
    ok(isle_game_add_object(game,7,9,0,nullptr,nullptr,&owner));
    IsleGameSprite* sprite = nullptr;
    ok(isle_game_add_sprite(game,owner,nullptr,&sprite));
    IsleGameProcess* process = nullptr;
    ok(isle_game_add_process(game,owner,player_source_sprite_explicit_context,&source,
        ISLE_GAME_SYNCHRONOUS,9,&process));
    bool destroyed = false;
    struct Owner { bool& destroyed; ~Owner() { destroyed = true; } };
    {
        Owner cpp_owner{destroyed};
        require(isle_game_tick(game) == ISLE_GAME_EXECUTION_FAILURE &&
                    source.status == ISLE_GAME_INVALID_STATE && source.ThrowItemTimeout != 99 &&
                    source.nonwaiting_call_frame == nullptr && !destroyed,
                "a prepared but inactive Source authorized a native sprite borrow or crossed C++");
    }
    require(destroyed,"native C sprite fault skipped the returning C++ owner's destructor");
    require(isle_game_delete_object(game,owner) == ISLE_GAME_EXECUTION_FAILURE,
            "inactive Source sprite rejection changed the native latched fault");
    IsleGameCounts remaining{};
    ok(isle_game_counts(game,&remaining));
    require(remaining.objects == 0 && remaining.processes == 0 && remaining.sprites == 0,
            "inactive Source sprite fault prevented joined cleanup");
    ok(isle_game_destroy(game));
}
void source_viewport_conversion_retains_quarterpixels_and_signed_low_lanes() {
    World world;
    IsleGameCamera* camera=nullptr;
    ok(isle_game_attach_camera(world.game,world.object(7),&camera));
    // Non-source depth makes the required reset observable, not a default echo.
    IsleGameViewport seed{{120,80,100,0},{400,300,200,0}};
    ok(isle_game_camera_set_viewport(world.game,camera,&seed));
    struct Case {
        float ulx,uly,lrx,lry;
        std::array<std::int16_t,2> scale,translate;
    };
    const Case cases[] = {
        {14,12,305,233,{582,442},{638,490}},
        {0.3125f,-5.9375f,3.0625f,-1.3125f,{5,9},{6,-14}},
        {-3.125f,-5.875f,-0.375f,-1.125f,{5,9},{-7,-14}},
        {8190.25f,-8195.125f,8193.75f,-8191.375f,{7,7},{-32768,32763}},
        // Conversion wraps before native admission, rather than clamping or
        // rejecting a rectangle solely for its source width or endpoint order.
        {0,0,32770,10,{4,20},{4,20}},
        {32766,0,0,10,{4,20},{-4,20}}
    };
    for (const auto& test : cases) {
        ok(isle_player_camera_set_viewport_rect(world.game,camera,
            test.ulx,test.uly,test.lrx,test.lry));
        const IsleGameCameraParameters* parameters=nullptr;
        ok(isle_game_camera_parameters(world.game,camera,&parameters));
        const auto& viewport=parameters->viewport;
        for (std::size_t axis=0;axis<2;++axis)
            require(viewport.scale[axis] == test.scale[axis] &&
                    viewport.translate[axis] == test.translate[axis],
                    "source viewport lost midpoint/truncation or signed low-sixteen-bit semantics");
        require(viewport.scale[2] == 511 && viewport.translate[2] == 511 &&
                viewport.scale[3] == seed.scale[3] && viewport.translate[3] == seed.translate[3],
                "source viewport failed to reset depth or preserve unwritten lanes");
    }
}

void source_viewport_rejections_preserve_native_camera_and_owner_errors() {
    World world;
    IsleGameCamera* camera=nullptr;
    ok(isle_game_attach_camera(world.game,world.object(7),&camera));
    ok(isle_player_camera_set_viewport_rect(world.game,camera,-9.125f,1.375f,31.625f,27.875f));
    const IsleGameCameraParameters* parameters=nullptr;
    ok(isle_game_camera_parameters(world.game,camera,&parameters));
    const IsleGameViewport before=parameters->viewport;
    const auto unchanged=[&] {
        const IsleGameCameraParameters* current=nullptr;
        ok(isle_game_camera_parameters(world.game,camera,&current));
        for (std::size_t axis=0;axis<4;++axis)
            require(current->viewport.scale[axis] == before.scale[axis] &&
                    current->viewport.translate[axis] == before.translate[axis],
                    "rejected viewport conversion partially mutated the native camera");
    };
    const float inf=std::numeric_limits<float>::infinity();
    const float nan=std::numeric_limits<float>::quiet_NaN();
    const float largest=std::numeric_limits<float>::max();
    const std::array<float,4> rejected[] = {
        {nan,0,320,240}, {0,inf,320,240}, {0,0,-inf,240}, {0,0,320,nan},
        {largest,0,largest,240}, // finite inputs, overflowing source midpoint
        {0,0,largest,240},      // finite midpoint, overflowing quarterpixel scale
        {536870912.0f,0,536870912.0f,240}, // +2^31 must not reach a signed cast
        {-536870976.0f,0,-536870976.0f,240},
        {0,0,0.25f,240},        // truncated zero scale rejected by native setter
        {0,0,20000,240},        // low-sixteen-bit scale is negative
        {10,0,0,240}            // negative scale is not silently repaired
    };
    for (const auto& rect : rejected) {
        require(isle_player_camera_set_viewport_rect(world.game,camera,
                    rect[0],rect[1],rect[2],rect[3]) == ISLE_GAME_INVALID_ARGUMENT,
                "invalid source viewport was accepted or its native error was replaced");
        unchanged();
    }
    // Exercise native setter admission independently: failure must remain a
    // returning, nonmutating operation rather than a source-side clamp.
    for (int lane : {0,2,3}) {
        auto invalid=before;
        if (lane == 0) invalid.scale[0]=0;
        else if (lane == 2) invalid.translate[2]=510;
        else invalid.scale[3]=1;
        require(isle_game_camera_set_viewport(world.game,camera,&invalid) == ISLE_GAME_INVALID_ARGUMENT,
                "native viewport setter accepted invalid scale, depth or reserved lane");
        unchanged();
    }
    require(isle_player_camera_set_viewport_rect(world.game,nullptr,0,0,320,240) ==
                ISLE_GAME_INVALID_ARGUMENT,
            "source viewport manufactured a camera for a missing owner");
    bool wrong_owner=false;
    std::thread foreign([&] {
        wrong_owner=isle_player_camera_set_viewport_rect(world.game,camera,nan,0,320,240) ==
                ISLE_GAME_WRONG_OWNER &&
            isle_player_camera_set_viewport_rect(world.game,nullptr,0,0,320,240) ==
                ISLE_GAME_WRONG_OWNER;
    });
    foreign.join();
    require(wrong_owner,"source viewport validated coordinates or camera before native owner access");
    unchanged();
    require(isle_game_status(world.game) == ISLE_GAME_OK,
            "returning viewport rejection poisoned native execution status");
}
}
int main() {
    try {
        canonical_root_cannot_alias_actor_item_foreign_or_second_context();
        helper_uses_game_context_across_real_wait_without_payload_cast();
        object_process_controls_preserve_real_waits_updaters_and_current_context();
        ordered_source_lookup_preserves_link_priority_absence_and_checked_boundaries();
        nonwaiting_c_fences_restore_ownership_and_preserve_native_fault_status();
        sprite_borrows_reacquire_actual_membership_after_a_real_wait();
        sprite_borrow_failures_use_only_the_canonical_nested_c_fence();
        sprite_self_delete_borrows_end_at_actual_native_retirement();
        sprite_borrow_without_active_source_faults_only_inside_the_native_c_guard();
        source_viewport_conversion_retains_quarterpixels_and_signed_low_lanes();
        source_viewport_rejections_preserve_native_camera_and_owner_errors();
        std::cout << "Player context ownership, C fences, continuations, sprite and viewport boundaries passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n'; return 1;
    }
}
