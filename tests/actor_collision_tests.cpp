#include "actor_collision.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <initializer_list>
#include <limits>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

namespace {
namespace a = isle::actor;
namespace c = isle::actor::collision;
namespace w = isle::world;
namespace m = isle::menu;
constexpr std::uint32_t allowed = 0x123456;
constexpr std::uint32_t prohibited = 0x007f66;
constexpr float nan = std::numeric_limits<float>::quiet_NaN();
constexpr float infinity = std::numeric_limits<float>::infinity();
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
void native(IsleGameResult result) { require(result == ISLE_GAME_OK,"native operation failed"); }
void success(a::OwnerStatus status) { require(bool(status),status.detail); }
bool same(IsleGameVec3 x, IsleGameVec3 y) { return x.x == y.x && x.y == y.y && x.z == y.z; }
bool same(const IsleGameModelTransform& x, const IsleGameModelTransform& y) {
    return same(x.position,y.position) && same(x.rotation,y.rotation) && same(x.scale,y.scale);
}
bool same(const IsleActorGround& x, const IsleActorGround& y) {
    return x.height == y.height && same(x.normal,y.normal) && x.surfaceType == y.surfaceType;
}
IsleActorGround marker() { return {876,{2,3,4},0xabcdef}; }
bool near(float x, float y) { return std::fabs(x-y) < 0.00001f; }
struct Native {
    std::vector<std::byte> bytes = std::vector<std::byte>(1024*1024);
    isle_arena arena{};
    IsleGame* game = nullptr;
    Native() {
        require(isle_arena_init(&arena,bytes.data(),bytes.size()) == ISLE_OK,"arena init");
        native(isle_game_create(&arena,&game));
    }
    ~Native() { if (isle_game_destroy(game) != ISLE_GAME_OK) std::abort(); }
};
w::HeightMap plane(std::uint32_t surface = allowed, float height = 1.0f) {
    w::HeightMap map;
    map.patches = {{0,0,1,-height,surface<<8}};
    w::HeightNode node{}; node.left_patch = 0; node.right_patch = 0;
    map.nodes = {node};
    return map;
}
w::HeightMap split(float boundary, std::uint32_t left = prohibited,
                   std::uint32_t right = allowed, float left_height = 2, float right_height = 1) {
    w::HeightMap map;
    map.patches = {{0,0,1,-left_height,left<<8},{0,0,1,-right_height,right<<8}};
    w::HeightNode node{}; node.a = 1; node.c = -boundary;
    node.left_patch = 0; node.right_patch = 1; map.nodes = {node};
    return map;
}
struct Input {
    isle::beach::WorldMetadata setup{};
    isle::beach::SkyMetadata sky{};
    std::array<w::BlockResource,1> blocks{};
    std::array<m::Model,1> models{};
    w::HeightMap map;
    bool with_map;
    Input(w::HeightMap ground, w::Vec3 origin, bool present): map(std::move(ground)),with_map(present) {
        blocks[0].model = 0;
        blocks[0].metadata.render_callback_guest = 0x800a1530;
        blocks[0].metadata.world_position = {origin.x,origin.y,origin.z};
        models[0].render_type = m::RenderType::a_fogged;
        m::Node root; root.scale = {1,1,1}; models[0].nodes.push_back(root);
    }
    w::ResourceView view() { return {&setup,&sky,blocks,1,models,{},{},{},{},with_map ? &map : nullptr,{}}; }
};
struct Trace {
    w::Vec3 camera{},effects{};
    std::size_t commands = 0;
    static void shift_camera(void* p, w::Vec3 d) noexcept { static_cast<Trace*>(p)->camera = d; }
    static void shift_effects(void* p, w::Vec3 d) noexcept { static_cast<Trace*>(p)->effects = d; }
    static void process_commands(void* p, IsleGameObject*) noexcept { ++static_cast<Trace*>(p)->commands; }
    w::Callbacks callbacks() { return {this,nullptr,nullptr,nullptr,shift_camera,shift_effects,process_commands}; }
};
struct Actor {
    IsleActorSource source{};
    isle_rng rng{1};
    IsleGameObject* object = nullptr;
    IsleGameModel* model = nullptr;
    void create(IsleGame* game, std::uint32_t id = 700) {
        native(isle_game_add_object(game,id,3,0,nullptr,nullptr,&object));
        // Real contextual root-local animation is deliberately unrelated to AUX.
        const IsleGameModelNodeInit root{UINT32_MAX,{{111,222,333},{0.7f,0.8f,0.9f},{4,5,6}},
                                        ISLE_GAME_MODEL_CONTEXTUAL_RPY};
        native(isle_game_attach_model(game,object,&root,1,&model));
        native(isle_actor_source_bind(game,object,&source,&rng,this));
        source.collisionRadius = 1;
        source.pos1 = {31,32,33}; source.targetPos = {41,42,43}; source.euler = {0.4f,0.5f,0.6f};
        source.collPosition = {51,52,53}; source.collisionOffset = {3,4,5};
        source.currGround = marker();
        source.flags = 0x200; source.processFlags = 0x80;
        const IsleGameModelTransform placement{{0,17,0},{0.1f,0.2f,0.3f},{-2,3,0.5f}};
        native(isle_game_model_set_placement(game,model,ISLE_GAME_MODEL_PLACEMENT_RTS,&placement));
    }
};
struct Fixture {
    Native native_game;
    Input input;
    Trace trace;
    w::World world;
    Actor actor;
    Fixture(w::HeightMap map = plane(), w::Vec3 origin = {}, bool enter = true, bool present = true)
        : input(std::move(map),origin,present),world(native_game.game,input.view(),trace.callbacks()) {
        if (enter) world.enter_first(0);
        actor.create(native_game.game);
    }
    IsleGame* game() { return native_game.game; }
    IsleGameModelPlacement placement() {
        const IsleGameModelPlacement* result = nullptr;
        native(isle_game_model_placement(game(),actor.model,&result)); return *result;
    }
    IsleGameModelNode root() {
        const IsleGameModelNode* nodes = nullptr; std::size_t count = 0;
        native(isle_game_model_peek_nodes(game(),actor.model,&nodes,&count));
        require(count == 1,"real native root extent changed"); return nodes[0];
    }
    void position(IsleGameVec3 value, IsleGameModelPlacementMode mode = ISLE_GAME_MODEL_PLACEMENT_RTS) {
        auto next = placement().transform; next.position = value;
        native(isle_game_model_set_placement(game(),actor.model,mode,&next));
    }
    a::OwnerStatus walk(float x, float z, std::uint32_t flags, bool& blocked) {
        return c::step_walk(world,actor.source,actor.model,x,z,flags,blocked);
    }
};
void fallback_and_full_ground() {
    {
        Fixture f(plane(),{},false);
        auto ground = marker(); success(c::ground_query(f.world,7,8,ground));
        require(same(ground,{-9000,{0,1,0},UINT32_MAX}),"no-current fallback lost complete source record");
        bool blocked = true; success(f.walk(2,3,1,blocked));
        require(!blocked && same(f.actor.source.currGround,ground) &&
                same(f.placement().transform.position,{2,-9000,3}),"no current became synthetic blocking ground");
    }
    {
        Fixture f(plane(),{},true,false);
        auto ground = marker(); success(c::ground_query(f.world,7,8,ground));
        require(same(ground,{-9000,{0,1,0},UINT32_MAX}),"absent map did not preserve unsigned fallback");
        bool blocked = true; success(f.walk(2,3,0,blocked));
        require(!blocked && same(f.placement().transform.position,{2,17,3}),"absent map blocked ordinary airborne walk");
    }
    {
        Fixture f;
        const auto* descriptor = f.world.blocks()[0].resource;
        f.world.blocks()[0].resource = nullptr;
        auto ground = marker(); const auto status = c::ground_query(f.world,7,8,ground);
        f.world.blocks()[0].resource = descriptor;
        success(status);
        require(same(ground,{-9000,{0,1,0},UINT32_MAX}),"absent descriptor did not use source fallback");
    }
    {
        auto map = plane(); map.patches[0] = {1,2,2,-8,0x12345678};
        Fixture f(std::move(map),{1,1,-2});
        auto ground = marker(); success(c::ground_query(f.world,200,300,ground));
        require(ground.height == 50 && near(ground.normal.x,1.0f/3) &&
                near(ground.normal.y,2.0f/3) && near(ground.normal.z,2.0f/3) && ground.surfaceType == allowed,
                "actual translated/sloped World result lost normal, elevation, axis swap or unsigned surface");
    }
    {
        auto map = plane(); map.patches[0] = {0,0,0,-8,allowed<<8};
        Fixture f(std::move(map));
        auto ground = marker(); success(c::ground_query(f.world,0,0,ground));
        require(ground.height == 0 && same(ground.normal,{0,0,0}),
                "defined zero plane C/normal invented a unit normal or a rejection");
        bool blocked = true; success(f.walk(1,0,1,blocked));
        require(!blocked && f.placement().transform.position.y == 0,
                "defined zero-normal native ground did not remain walkable");
    }
    {
        Fixture f;
        f.world.retire(); const auto prior = marker(); auto ground = prior;
        const auto status = c::ground_query(f.world,0,0,ground);
        require(status.error == a::OwnerError::lifetime && same(ground,prior),"retired World became absent-map success");
        bool blocked = true; const auto walk_status = f.walk(1,0,0,blocked);
        require(walk_status.error == a::OwnerError::lifetime && blocked && same(f.actor.source.currGround,prior),
                "retired World changed walk output or canonical ground");
    }
}
void malformed_and_consumed_map_paths() {
    for (unsigned failure = 0; failure < 4; ++failure) {
        auto map = plane();
        if (failure == 0) map.nodes.clear();
        if (failure == 1) map.nodes[0].right_patch = 99;
        if (failure == 2) map.nodes[0].right_patch = w::no_index;
        if (failure == 3) { map.nodes[0].right_patch = w::no_index; map.nodes[0].right_child = 0; }
        Fixture f(std::move(map));
        const auto prior = marker(); auto ground = prior;
        const auto status = c::ground_query(f.world,0,0,ground);
        require(status.error == a::OwnerError::admission && same(ground,prior),"malformed/uncovered map became fallback success");
        bool blocked = false; const auto walk_status = f.walk(0,0,0,blocked);
        require(walk_status.error == a::OwnerError::admission && !blocked && same(f.actor.source.currGround,prior),
                "failed initial actual ground lookup changed canonical record or blocked output");
    }
    {
        auto map = plane(); map.nodes[0].left_patch = w::no_index; map.nodes[0].left_child = 0;
        Fixture f(std::move(map));
        auto ground = marker(); success(c::ground_query(f.world,0,0,ground));
        require(ground.height == 100 && ground.surfaceType == allowed,"unused malformed sibling invalidated consumed valid branch");
        const auto prior = ground;
        require(c::ground_query(f.world,infinity,0,ground).error == a::OwnerError::admission && same(ground,prior),
                "nonfinite consumed ground coordinates published output");
    }
    {
        Fixture f(plane(),{},false);
        auto ground = marker(); success(c::ground_query(f.world,nan,infinity,ground));
        require(ground.height == -9000,"fallback started validating unconsumed coordinates");
    }
}
void default_and_custom_surfaces() {
    constexpr std::array<std::uint32_t,9> defaults{
        0x007f66,0x00ff00,0x337fb2,0x4ccccc,0x7f6633,0x7f667f,0xff0000,0xff4c19,0xff7fb2};
    for (const auto surface : defaults) {
        Fixture f(plane(surface));
        f.actor.source.forbiddenGroundCount = std::numeric_limits<std::size_t>::max();
        f.actor.source.collisionRadius = nan; // Destination rejection does not consume radius.
        const auto before = f.placement(); bool blocked = false;
        success(f.walk(2,3,1,blocked));
        require(blocked && same(f.placement().transform,before.transform) && f.placement().revision == before.revision &&
                f.actor.source.currGround.height == 100 && f.actor.source.currGround.surfaceType == surface,
                "default-nine rejection/order/destination mutation differs");
    }
    {
        Fixture f(plane(prohibited));
        const std::array list{0.0f,static_cast<float>(prohibited)};
        f.actor.source.forbiddenGround = list.data(); f.actor.source.forbiddenGroundCount = list.size();
        bool blocked = true; success(f.walk(1,0,0,blocked));
        require(!blocked && f.placement().transform.position.x == 1,"custom empty list failed to replace default policy");
    }
    {
        Fixture f;
        const std::array list{static_cast<float>(allowed)}; // No later terminator is required after a match.
        f.actor.source.forbiddenGround = list.data(); f.actor.source.forbiddenGroundCount = list.size();
        bool blocked = false; success(f.walk(1,0,0,blocked));
        require(blocked,"bounded early match demanded an unreadable terminator");
    }
    {
        Fixture f;
        const std::array list{nan,infinity,-7.0f,-0.0f,static_cast<float>(allowed)};
        f.actor.source.forbiddenGround = list.data(); f.actor.source.forbiddenGroundCount = list.size();
        bool blocked = true; success(f.walk(1,0,0,blocked));
        require(!blocked,"float-list NaN/Inf/nonmatch or signed-zero sentinel changed source comparison semantics");
    }
    {
        Fixture f;
        const std::array list{static_cast<float>(allowed),nan,infinity};
        f.actor.source.forbiddenGround = list.data(); f.actor.source.forbiddenGroundCount = 1;
        bool blocked = false; success(f.walk(1,0,0,blocked));
        require(blocked,"first match consumed ignored float-list tail");
    }
    {
        Fixture f;
        const std::array list{static_cast<float>(allowed)+0.5f,0.0f};
        f.actor.source.forbiddenGround = list.data(); f.actor.source.forbiddenGroundCount = list.size();
        bool blocked = true; success(f.walk(1,0,0,blocked));
        require(!blocked,"custom float entry was truncated to an integer surface before comparison");
    }
    for (const std::size_t count : {std::size_t{0},std::size_t{1}}) {
        Fixture f;
        const std::array list{77.0f};
        f.actor.source.forbiddenGround = list.data(); f.actor.source.forbiddenGroundCount = count;
        const auto before = f.placement(); bool blocked = true;
        const auto status = f.walk(1,0,0,blocked);
        require(status.error == a::OwnerError::admission && blocked &&
                same(f.placement().transform,before.transform) && f.actor.source.currGround.height == 100 &&
                f.actor.source.currGround.surfaceType == allowed,
                "consumed custom extent error rolled back actual destination query or became ordinary blocking");
    }
}
void unsigned_sentinel_comparison() {
    {
        Fixture f(plane(),{},true,false);
        const std::array list{-1.0f,0.0f};
        f.actor.source.forbiddenGround = list.data(); f.actor.source.forbiddenGroundCount = list.size();
        bool blocked = true; success(f.walk(1,0,0,blocked));
        require(!blocked && f.actor.source.currGround.surfaceType == UINT32_MAX,"unsigned sentinel compared as signed -1f");
    }
    {
        Fixture f(plane(),{},true,false);
        const std::array list{4294967296.0f};
        f.actor.source.forbiddenGround = list.data(); f.actor.source.forbiddenGroundCount = 1;
        bool blocked = false; success(f.walk(1,0,0,blocked));
        require(blocked,"uint32 sentinel was not converted to binary32 for custom comparison");
    }
}
void probe_order_and_destination_sideeffects() {
    {
        Fixture f(split(0.015f)); // Destination x=1 allowed; old+radius x=2 prohibited.
        f.actor.source.collisionRadius = 2;
        const auto before = f.placement(); bool blocked = false;
        success(f.walk(1,0,1,blocked));
        require(blocked && same(f.placement().transform,before.transform) && f.actor.source.currGround.height == 100 &&
                f.actor.source.currGround.surfaceType == allowed,
                "radius probe rejection overwrote canonical destination ground or published move");
    }
    {
        Fixture f(split(0.015f));
        f.actor.source.collisionRadius = 1;
        bool blocked = true; success(f.walk(1,0,1,blocked));
        require(!blocked && same(f.placement().transform.position,{1,100,0}),
                "radius probe used next position instead of old AUX or added root-local translation");
    }
    {
        Fixture f(split(0.015f));
        f.actor.source.collisionRadius = 5;
        bool blocked = false; success(f.walk(0.3f,0.4f,0,blocked));
        require(blocked && f.actor.source.currGround.surfaceType == allowed,
                "second query did not normalize velocity before applying radius");
    }
    {
        Fixture f(split(0.015f,allowed,allowed,2,1));
        const std::array list{static_cast<float>(allowed)};
        // Destination match exits immediately; the second query and its NaN
        // radius are unconsumed.
        f.actor.source.forbiddenGround = list.data(); f.actor.source.forbiddenGroundCount = 1;
        f.actor.source.collisionRadius = nan;
        bool blocked = false; success(f.walk(1,0,0,blocked));
        require(blocked && f.actor.source.currGround.height == 100,"destination match lost source early return");
    }
    {
        Fixture f(split(0.015f,allowed,prohibited));
        const std::array list{static_cast<float>(allowed)};
        f.actor.source.forbiddenGround = list.data(); f.actor.source.forbiddenGroundCount = 1;
        // Destination is not in the custom list and has no terminator: error
        // precedes even a would-be matching probe.
        f.actor.source.collisionRadius = 2;
        bool blocked = false; const auto status = f.walk(1,0,0,blocked);
        require(status.error == a::OwnerError::admission && !blocked &&
                f.actor.source.currGround.surfaceType == prohibited,"destination-list exhaustion incorrectly reached probe");
    }
    {
        auto map = split(0.015f); map.nodes[0].left_patch = 99;
        Fixture f(std::move(map)); f.actor.source.collisionRadius = 2;
        bool blocked = false; const auto before = f.placement(); const auto status = f.walk(1,0,0,blocked);
        require(status.error == a::OwnerError::admission && !blocked &&
                f.actor.source.currGround.height == 100 && same(f.placement().transform,before.transform),
                "probe map error failed to preserve earlier actual destination mutation and blocked output");
    }
    {
        Fixture f(split(0.015f));
        const std::array list{123.0f,0.0f};
        f.actor.source.forbiddenGround = list.data(); f.actor.source.forbiddenGroundCount = list.size();
        f.actor.source.collisionRadius = 2;
        bool blocked = true; success(f.walk(1,0,1,blocked));
        require(!blocked && f.placement().transform.position.y == 100 && f.actor.source.currGround.height == 100,
                "ON_GROUND snapped to probe height rather than destination ground");
    }
}
void probe_custom_extent_and_match() {
    {
        Fixture f(split(0.015f));
        const std::array list{static_cast<float>(prohibited)};
        // Destination allowed is not matched: give it a zero terminator, then
        // use the same actual list for probe. Both traversals remain bounded.
        const std::array bounded{list[0],0.0f};
        f.actor.source.forbiddenGround = bounded.data(); f.actor.source.forbiddenGroundCount = bounded.size();
        f.actor.source.collisionRadius = 2;
        bool blocked = false; success(f.walk(1,0,0,blocked));
        require(blocked && f.actor.source.currGround.surfaceType == allowed,"custom list was not used for second radius query");
    }
    {
        Fixture f(split(0.015f));
        // Destination matches this one-entry list, so the unconsumed radius is
        // invalid but must not turn an ordinary rejection into an error.
        const float list = static_cast<float>(allowed);
        f.actor.source.forbiddenGround = &list; f.actor.source.forbiddenGroundCount = 1;
        f.actor.source.collisionRadius = infinity;
        bool blocked = false; success(f.walk(1,0,0,blocked)); require(blocked,"early custom match consumed radius");
    }
}
void exact_movement_flags() {
    {
        Fixture f; f.position({0,100,0});
        bool blocked = true; success(f.walk(1,2,0x11,blocked));
        require(!blocked && same(f.placement().transform.position,{1,100,2}),"exact fixed-height equality rejected");
    }
    {
        Fixture f; f.position({0,std::nextafter(100.0f,infinity),0});
        const auto before = f.placement(); bool blocked = false; success(f.walk(1,2,0x11,blocked));
        require(blocked && same(f.placement().transform,before.transform) && f.actor.source.currGround.height == 100,
                "fixed-height bit10 used tolerance or ON_GROUND before exact test");
    }
    {
        Fixture f;
        bool blocked = true; success(f.walk(1,2,0x02,blocked));
        require(!blocked && same(f.placement().transform.position,{1,17,2}) &&
                same(f.actor.source.targetPos,{41,42,43}),"unrelated movement flag was mistaken for fixedheight/target publication");
    }
    {
        Fixture f; bool blocked = true; success(f.walk(1,2,1,blocked));
        require(!blocked && same(f.placement().transform.position,{1,100,2}),"ON_GROUND bit1 did not assign accepted destination height");
    }
}
void signed_zero_and_subnormal_math() {
    {
        Fixture f(split(0,allowed,prohibited)); f.position({1,17,0}); f.actor.source.collisionRadius = -2;
        bool blocked = false; success(f.walk(1,0,0,blocked));
        require(blocked && f.actor.source.currGround.surfaceType == allowed,"finite negative radius was rejected or made nonnegative");
    }
    {
        Fixture f(split(0,allowed,prohibited)); f.position({1,17,0}); f.actor.source.collisionRadius = 0;
        bool blocked = true; success(f.walk(1,0,0,blocked));
        require(!blocked && f.placement().transform.position.x == 2,"zero radius did not probe unchanged old AUX");
    }
    {
        Fixture f; f.actor.source.collisionRadius = -7;
        const auto before = f.placement(); bool blocked = true; success(f.walk(0,-0.0f,0,blocked));
        require(!blocked && same(f.placement().transform,before.transform) && f.placement().revision == before.revision,
                "zero velocity invented a direction or dirtied identical native placement");
    }
    {
        Fixture f(split(0,allowed,prohibited)); f.position({1,17,0}); f.actor.source.collisionRadius = -0.0f;
        bool blocked = true; success(f.walk(1,0,0,blocked)); require(!blocked,"negative zero radius became invalid");
    }
    constexpr float tiny = std::numeric_limits<float>::denorm_min();
    for (const bool should_block : {true,false}) {
        Fixture f(split(should_block ? 1.0e-9f : 1.0e-8f));
        f.actor.source.collisionRadius = std::numeric_limits<float>::max();
        bool blocked = !should_block; success(f.walk(tiny,0,0,blocked));
        require(blocked == should_block && f.actor.source.currGround.surfaceType == allowed,
                "squared-underflow normalization did not retain original nonzero component before radius scale");
        if (!blocked) require(f.placement().transform.position.x == tiny,"subnormal accepted native movement was flushed or rounded away");
    }
    {
        Fixture f; f.actor.source.collisionRadius = -std::numeric_limits<float>::max();
        bool blocked = true; success(f.walk(1,0,0,blocked));
        require(!blocked,"finite signed large radius invented a nonnegative/range policy");
    }
    {
        Fixture f; bool blocked = true;
        const auto status = f.walk(1.0e20f,0,0,blocked);
        require(status.error == a::OwnerError::native && blocked && f.actor.source.currGround.height == 100,
                "defined squared-magnitude overflow was rejected before actual native placement domain");
    }
}
void actual_pose_and_ownership_errors() {
    for (const auto mode : {ISLE_GAME_MODEL_PLACEMENT_RT,ISLE_GAME_MODEL_PLACEMENT_RTS}) {
        Fixture f; f.position({0,17,0},mode);
        const auto before = f.placement(); const auto root = f.root();
        const auto source = f.actor.source;
        bool blocked = true; success(f.walk(3,4,1,blocked));
        const auto after = f.placement(); const auto after_root = f.root();
        require(!blocked && after.mode == mode && same(after.transform.rotation,before.transform.rotation) &&
                same(after.transform.scale,before.transform.scale) && same(after.transform.position,{3,100,4}) &&
                after.revision == before.revision+1 && same(root.transform,after_root.transform) &&
                root.revision == after_root.revision && root.flags == after_root.flags &&
                same(f.actor.source.pos1,source.pos1) && same(f.actor.source.targetPos,source.targetPos) &&
                same(f.actor.source.euler,source.euler) && same(f.actor.source.collPosition,source.collPosition) &&
                same(f.actor.source.collisionOffset,source.collisionOffset) &&
                f.actor.source.flags == source.flags && f.actor.source.processFlags == source.processFlags,
                "walk changed placement mode/TRS, root-local animation or independent canonical actor fields");
    }
    {
        Fixture f; Actor other; other.create(f.game(),701);
        bool blocked = false; const auto prior = f.actor.source.currGround;
        const auto status = c::step_walk(f.world,f.actor.source,other.model,1,0,0,blocked);
        require(status.error == a::OwnerError::native && !blocked && same(f.actor.source.currGround,prior),
                "same-game foreign attached model passed canonical ownership validation");
        auto* object = f.actor.source.object;
        f.actor.source.object = other.object;
        const auto mismatch = f.walk(1,0,0,blocked);
        f.actor.source.object = object;
        require(mismatch.error == a::OwnerError::native && !blocked && same(f.actor.source.currGround,prior),
                "mismatched actual source binding passed validation");
    }
    {
        Fixture f; bool blocked = true; const auto prior = f.actor.source.currGround;
        auto* owner = f.actor.source.owner;
        f.actor.source.owner = nullptr;
        const auto missing_owner = f.walk(1,0,0,blocked);
        f.actor.source.owner = owner;
        require(missing_owner.error == a::OwnerError::native && blocked && same(f.actor.source.currGround,prior),
                "canonical binding without its live native owner was accepted");
        auto* game = f.actor.source.game;
        f.actor.source.game = nullptr;
        const auto missing_game = f.walk(1,0,0,blocked);
        f.actor.source.game = game;
        require(missing_game.error == a::OwnerError::native && blocked && same(f.actor.source.currGround,prior),
                "canonical source without its actual native game was accepted");
        require(c::step_walk(f.world,f.actor.source,nullptr,1,0,0,blocked).error == a::OwnerError::native &&
                blocked && same(f.actor.source.currGround,prior),"NULL model passed actual attached-model validation");
    }
    {
        Fixture f; bool blocked = true;
        native(isle_game_model_set_placement(f.game(),f.actor.model,ISLE_GAME_MODEL_PLACEMENT_DISABLED,nullptr));
        const auto prior = f.actor.source.currGround;
        require(f.walk(1,0,0,blocked).error == a::OwnerError::native && blocked && same(f.actor.source.currGround,prior),
                "disabled AUX placement silently consumed/ignored native movement");
    }
    {
        Fixture f; bool blocked = false; const auto prior = f.actor.source.currGround;
        native(isle_game_remove_model(f.game(),f.actor.model));
        require(f.walk(1,0,0,blocked).error == a::OwnerError::native && !blocked && same(f.actor.source.currGround,prior),
                "removed native model became ordinary blocking or changed canonical ground");
    }
    {
        Fixture f; bool blocked = true; const auto prior = f.actor.source.currGround;
        native(isle_game_delete_object(f.game(),f.actor.object));
        require(f.walk(1,0,0,blocked).error == a::OwnerError::native && blocked && same(f.actor.source.currGround,prior),
                "retired native source/model identity was accepted");
    }
    {
        Fixture f; bool blocked = false; a::OwnerStatus status;
        const auto before = f.placement(); const auto ground = f.actor.source.currGround;
        std::thread wrong_owner([&] { status = f.walk(1,0,0,blocked); }); wrong_owner.join();
        require(status.error == a::OwnerError::native && !blocked && same(f.actor.source.currGround,ground) &&
                same(f.placement().transform,before.transform),"unadmitted native thread queried/published canonical actor movement");
    }
}
void numeric_errors_preserve_ordered_effects() {
    for (const float velocity : {nan,infinity,-infinity}) {
        Fixture f; bool blocked = true; const auto prior = f.actor.source.currGround;
        require(f.walk(velocity,0,0,blocked).error == a::OwnerError::admission && blocked && same(f.actor.source.currGround,prior),
                "nonfinite prospective coordinates mutated ground or blocked output");
    }
    for (const float radius : {nan,infinity,-infinity}) {
        Fixture f; f.actor.source.collisionRadius = radius;
        const auto before = f.placement(); bool blocked = false;
        require(f.walk(1,0,0,blocked).error == a::OwnerError::admission && !blocked &&
                f.actor.source.currGround.height == 100 && same(f.placement().transform,before.transform),
                "consumed unsafe radius probe rolled back destination ground or published output");
    }
    {
        Fixture f(plane(allowed,1000)); // Height100000 cannot enter native fixed placement, but is real ground.
        const auto before = f.placement(); bool blocked = true;
        require(f.walk(1,0,1,blocked).error == a::OwnerError::native && blocked &&
                f.actor.source.currGround.height == 100000 && same(f.placement().transform,before.transform),
                "native setter failure lost earlier actual ground mutation or changed output/placement");
    }
    {
        Fixture f(plane(allowed,1000)); bool blocked = true; success(f.walk(1,0,0,blocked));
        require(!blocked && f.placement().transform.position.y == 17 && f.actor.source.currGround.height == 100000,
                "unconsumed ground-height native range was rejected for airborne walk");
    }
}
} // namespace

int main() {
    try {
        fallback_and_full_ground();
        malformed_and_consumed_map_paths();
        default_and_custom_surfaces();
        unsigned_sentinel_comparison();
        probe_order_and_destination_sideeffects();
        probe_custom_extent_and_match();
        exact_movement_flags();
        signed_zero_and_subnormal_math();
        actual_pose_and_ownership_errors();
        numeric_errors_preserve_ordered_effects();
        std::cout << "native actor ground/walk contracts passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "native actor collision failure: " << error.what() << '\n';
        return 1;
    }
}
