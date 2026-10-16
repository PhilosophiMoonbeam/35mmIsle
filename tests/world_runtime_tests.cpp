#include "world_movement.hpp"
#include "menu_scene_bridge.h"
#include <array>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <vector>

// Native own-GAME wait boundary, also exercised by the compiled-C engine tests.
// The callback below retains no owning C++ automatic across this failure fence.
extern "C" void ohWait(std::int32_t);

namespace {
namespace w = isle::world;
namespace m = isle::menu;
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
void ok(IsleGameResult result) { require(result == ISLE_GAME_OK,"native operation failed"); }
struct Native {
    std::vector<unsigned char> bytes = std::vector<unsigned char>(1024*1024);
    isle_arena arena{};
    IsleGame* game = nullptr;
    Native() { require(isle_arena_init(&arena,bytes.data(),bytes.size()) == ISLE_OK,"arena"); ok(isle_game_create(&arena,&game)); }
    ~Native() { if (isle_game_destroy(game) != ISLE_GAME_OK) std::abort(); }
};
struct Event { char kind; std::size_t block, reference; };
struct Trace {
    std::vector<Event> events;
    std::vector<w::Vec3> camera, effects;
    std::size_t command_calls = 0, exits = 0, updates = 0;
    Trace() { events.reserve(128); camera.reserve(16); effects.reserve(16); }
    static void add(void* p,w::Block& b,w::Block& r) noexcept { static_cast<Trace*>(p)->events.push_back({'a',b.index,r.index}); }
    static void remove(void* p,w::Block& b) noexcept { static_cast<Trace*>(p)->events.push_back({'d',b.index,0}); }
    static void change(void* p,w::Block& b,w::Block& r) noexcept { static_cast<Trace*>(p)->events.push_back({'c',b.index,r.index}); }
    static void shift_camera(void* p,w::Vec3 d) noexcept { static_cast<Trace*>(p)->camera.push_back(d); }
    static void shift_effects(void* p,w::Vec3 d) noexcept { static_cast<Trace*>(p)->effects.push_back(d); }
    static void commands(void* p,IsleGameObject*) noexcept { ++static_cast<Trace*>(p)->command_calls; }
    static void exit(void* p,w::Block&) noexcept { ++static_cast<Trace*>(p)->exits; }
    static void update(void* p,std::size_t,float) noexcept { ++static_cast<Trace*>(p)->updates; }
    w::Callbacks callbacks() { return {this,add,remove,change,shift_camera,shift_effects,commands}; }
};
struct Input {
    isle::beach::WorldMetadata setup{};
    isle::beach::SkyMetadata sky{};
    std::array<w::BlockResource,4> blocks{};
    std::array<m::Model,7> models;
    Input() {
        setup.sky_animation_speed = 0.4f;
        for (std::size_t i = 0; i < blocks.size(); ++i) {
            auto& b = blocks[i]; b.model = i; b.metadata.render_callback_guest = 0x800a1530;
            b.metadata.world_position = {float(i)*10,float(i)*2,-float(i)*3};
            models[i].render_type = m::RenderType::a_fogged;
            m::Node root; root.scale = {1,1,1}; models[i].nodes.push_back(root);
            if (i < 3) {
                b.road_model = 4+i; b.metadata.control_line_count = 2;
                b.metadata.road_guest = 4; b.metadata.movement_animation_guest = 8;
                b.metadata.movement_animation_duration = 100;
                auto& road = models[4+i]; road.render_type = m::RenderType::b;
                road.nodes.push_back(root);
                m::Node cp; cp.parent = 0; cp.position = {10+float(i),20,30};
                cp.rotation = {0.1f,0.2f,0.3f}; cp.scale = {0.25f+float(i)*0.25f,1,1};
                road.nodes.push_back(cp);
            }
        }
    }
    w::ResourceView view() { return {&setup,&sky,blocks,3,models,{},{},{},{},nullptr,{}}; }
};
void event(const Trace& trace,std::size_t i,char kind,std::size_t block,std::size_t reference = 0) {
    require(i < trace.events.size() && trace.events[i].kind == kind && trace.events[i].block == block &&
        trace.events[i].reference == reference,"world callback source order differs");
}
void transitions_and_retirement() {
    Native n; Input input; Trace trace;
    w::World world(n.game,input.view(),trace.callbacks());
    IsleGameCounts counts{}; ok(isle_game_counts(n.game,&counts));
    require(counts.objects == 8 && counts.models == 7 && counts.processes == 1,"real sky/block/road owners missing");
    require(!world.current() && world.blocks()[3].previous == nullptr && world.blocks()[3].next == nullptr,"secondary blocks joined road");
    world.enter_first(0);
    event(trace,0,'a',0,0); event(trace,1,'a',1,0);
    require(trace.events.size() == 2,"first block invented previous/secondary add");
    require(world.enter_next() == &world.blocks()[1],"first transition failed");
    event(trace,2,'a',2,1); event(trace,3,'c',0,1); event(trace,4,'c',1,1);
    require(trace.camera.size() == 1 && trace.effects.size() == 1 && trace.camera[0].x == -1000 &&
        trace.camera[0].y == -200 && trace.camera[0].z == 300 && trace.effects[0].x == trace.camera[0].x,
        "required camera/effect callbacks lost source100x shift");
    const IsleGameModelNode* nodes = nullptr; std::size_t count = 0;
    ok(isle_game_model_nodes(n.game,world.blocks()[0].model,&nodes,&count));
    require(nodes[0].transform.position.x == -1000 && nodes[0].transform.position.y == -200 && nodes[0].transform.position.z == 300,
        "native model not shifted relative to actual current block");
    world.enter_next(); event(trace,5,'d',0); event(trace,6,'c',0,2); event(trace,7,'c',1,2); event(trace,8,'c',2,2);
    require(world.enter_next() == nullptr && trace.events.size() == 9,"terminal block invented loopback or callbacks");
    world.set_visibility(5);
    for (std::size_t i = 0; i < world.blocks().size(); ++i) {
        IsleGameObjectMetadata metadata{}; ok(isle_game_object_metadata(n.game,world.blocks()[i].object,&metadata));
        require(bool(metadata.flags&1u) == !(5u&(1u<<i)),"visibility source native bit consumption differs");
    }
    ok(isle_game_object_set_flags(n.game,world.blocks()[1].object,0x81));
    world.set_visibility(15);
    IsleGameObjectMetadata flags{}; ok(isle_game_object_metadata(n.game,world.blocks()[1].object,&flags));
    require(flags.flags == 0x80,"immediate world visibility failed to clear hidden while retaining other native flags");
    ok(isle_game_object_set_flags(n.game,world.blocks()[1].object,0));
    ok(isle_game_tick(n.game)); require(trace.command_calls == 8 && std::fabs(world.global_time()-0.4f)<0.00001f,"native commands/timer not scheduled");
    world.retire(); world.retire();
    ok(isle_game_counts(n.game,&counts));
    require(counts.objects == 0 && counts.models == 0 && counts.processes == 0 && counts.model_nodes == 0,
        "world retirement leaked native objects/processes/models");
    require(trace.events.size() == 13,"retirement did not invoke each source remove exactly once");
    for (std::size_t i = 0; i < 4; ++i) event(trace,9+i,'d',i);
    ok(isle_game_tick(n.game)); require(trace.command_calls == 8,"retired callback userdata reused");
}
void movement_boundary_rotation_and_stop() {
    Native n; Input input; Trace trace;
    input.blocks[0].metadata.yaw = 1.5707963267948966f;
    input.blocks[0].metadata.reversed = true;
    w::World world(n.game,input.view(),trace.callbacks());
    w::MovementState state;
    w::Movement movement(world,state,{&trace,Trace::exit,Trace::update});
    require(movement.initialize(0),"movement init");
    require(std::fabs(state.position.x-30)<0.0001f && std::fabs(state.position.z+10)<0.0001f && state.position.y == 20,
        "world movement yaw transform differs");
    require(std::fabs(state.rotation.y-(0.2f+1.5707963267948966f+3.1415927f))<0.00001f && state.move_speed == 0.25f,
        "reversed yaw or control-node speed lost");
    state.move_time = 0.75f; state.move_speed = 0.25f; movement.update();
    require(state.block->index == 0 && state.move_time == 1 && trace.exits == 0,"strict greater-than boundary changed");
    state.move_speed = 0.25f; movement.update();
    require(state.block->index == 1 && state.move_time == 0.25f && trace.exits == 1,"movement carry/exit/transition order differs");
    state.block = world.enter_first(2); state.move_time = 0.9f; state.move_speed = 0.2f;
    movement.update();
    require(state.block->index == 2 && state.move_time == 0.9f && state.move_speed == 0 && trace.exits == 2,
        "last block must retain source time, notify exit, and stop");
    state.speed_multiplier = -1; require(!movement.initialize(0),"negative movement multiplier admitted");
}
void position_sampling_preserves_movement_boundary() {
    Native n; Input input; Trace trace;
    // Authored CP animation, not a ROM table: position.x advances 10 to110 over
    // 100 road frames. Raw CP speeds also permit a carry-driven transition.
    constexpr std::uint32_t guest = 0x80300000;
    std::array<std::byte,20> bytes{};
    const auto word = [&](std::size_t offset,std::uint32_t value) {
        for (unsigned i = 0; i < 4; ++i) bytes[offset+i] = std::byte((value>>(24-8*i))&255);
    };
    word(0,(11u<<25)|(16u<<15)); word(4,std::bit_cast<std::uint32_t>(10.0f));
    word(8,(3u<<25)|(16u<<15)|100u); word(12,std::bit_cast<std::uint32_t>(110.0f));
    word(16,0);
    const auto wire = [&](std::size_t offset,std::size_t size) {
        return isle::rom::GuestView{guest+static_cast<std::uint32_t>(offset),offset,std::span(bytes).subspan(offset,size)};
    };
    m::Animation program; program.wire = wire(0,20); program.kind = m::AnimationKind::model;
    program.commands = {{wire(0,8),11,16,0},{wire(8,8),3,16,100},{wire(16,4),0,0,0}};
    auto& cp = input.models[4].nodes[1]; cp.animation = 0; cp.scale.x = 1;
    input.models[5].nodes[1].scale.x = 2;
    auto resources = input.view(); resources.animations = std::span(&program,1);
    w::World world(n.game,resources,trace.callbacks());
    w::MovementState state; w::Movement movement(world,state,{});
    require(movement.initialize(0),"position-sampling movement admission");
    auto* block = state.block; state.move_time = 0.75f;
    movement.update_position(*block,0.5f,state.control_time);
    require(world.current() == block && state.block == block && state.move_time == 0.75f,
        "position sampling performed a carry-driven block transition or advanced movement time");
    require(state.position.x == 60 && state.position.y == 20 && state.position.z == 30 &&
        state.rotation.x == 0.1f && state.rotation.y == 0.2f && state.rotation.z == 0.3f && state.move_speed == 1.5f,
        "position sampling lost animated control pose or next-block speed interpolation");
    require(world.animation_model(block->road_instance).animation_time == 50,"road sample lost duration times moveTime");
    const IsleGameModelNode* nodes = nullptr; std::size_t count = 0;
    ok(isle_game_model_nodes(n.game,block->road_model,&nodes,&count));
    require(count == 2 && nodes[1].transform.position.x == state.position.x,"sampled CP pose did not reach the native model");
    movement.update_position(*block,0.25f,state.control_time);
    require(world.current() == block && state.block == block && state.move_time == 0.75f &&
        state.position.x == 35 && state.move_speed == 1.25f &&
        world.animation_model(block->road_instance).animation_time == 25 && nodes[1].transform.position.x == 35,
        "signed road resampling changed the movement boundary or lost native pose synchronization");
}
void required_consumers_and_sky_follow() {
    Native n; Input input; Trace trace;
    auto callbacks = trace.callbacks(); callbacks.shift_effects = nullptr;
    bool rejected = false; try { w::World world(n.game,input.view(),callbacks); } catch (const std::invalid_argument&) { rejected = true; }
    require(rejected,"required effect consumer silently replaced by noop");
    input.sky.display_list_guest = 0x8011cc28; input.sky.render_callback_guest = 0x800e1ca4;
    w::World world(n.game,input.view(),trace.callbacks());
    IsleGameObject* object = nullptr; IsleGameCamera* camera = nullptr;
    ok(isle_game_add_object(n.game,1,0,0,nullptr,nullptr,&object)); ok(isle_game_attach_camera(n.game,object,&camera));
    IsleGameLookAt look{{10,20,30},{0,0,0},{0,1,0}}; ok(isle_game_camera_set_look_at(n.game,camera,&look));
    world.follow_camera(camera); ok(isle_game_tick(n.game));
    const auto sky = world.render_objects().front(); const IsleGameModelNode* nodes = nullptr; std::size_t count = 0;
    ok(isle_game_model_nodes(n.game,sky.model,&nodes,&count));
    require(nodes[0].transform.position.x == 10 && nodes[0].transform.position.y == 20 && nodes[0].transform.position.z == 30,
        "sky follow is not an actual native priority0 process");
    world.retire(); ok(isle_game_delete_object(n.game,object));
}
void path_tracks_callbacks_and_boundaries() {
    m::AnimationPath path;
    path.kind = 2; path.segment_point_count = 2; path.duration = 10;
    path.points = {{0,0,0},{10,0,0},{20,0,0},{30,0,0}};
    path.timestamps = {0,1}; path.speed_polynomials = {0,0,0,0,100};
    require(std::fabs(m::animation::path_position(path,0).x-10)<0.0001f &&
        std::fabs(m::animation::path_position(path,0.5f).x-15)<0.0001f &&
        std::fabs(m::animation::path_position(path,1).x-20)<0.0001f,
        "source Simpson parameterization/B-spline endpoint law differs");
    require(std::fabs(m::animation::path_velocity(path,0.5f).x-10)<0.0001f,"source spline derivative differs");
    bool rejected = false;
    try { m::animation::path_velocity(path,1); } catch (const std::invalid_argument&) { rejected = true; }
    require(rejected,"terminal source velocity read exceeded admitted spline guard points");
    constexpr std::uint32_t guest = 0x80300000;
    std::array<std::byte,28> bytes{};
    const auto word = [&](std::size_t offset,std::uint32_t value) {
        for (unsigned i = 0; i < 4; ++i) bytes[offset+i] = std::byte((value>>(24-8*i))&255);
    };
    word(0,(13u<<25)|(8u<<15)); word(4,0x80310000);
    word(8,(11u<<25)|(8u<<15)); word(12,std::bit_cast<std::uint32_t>(0.5f));
    word(16,(17u<<25)|(1u<<15)|1u); word(20,std::bit_cast<std::uint32_t>(3.0f)); word(24,0);
    const auto view = [&](std::size_t offset,std::size_t size) {
        return isle::rom::GuestView{guest+static_cast<std::uint32_t>(offset),offset,std::span(bytes).subspan(offset,size)};
    };
    m::Animation program; program.wire = view(0,28); program.kind = m::AnimationKind::model; program.callback_installed = true;
    program.commands = {{view(0,8),13,8,0,0},{view(8,8),11,8,0},{view(16,8),17,1,1},{view(24,4),0,0,0}};
    m::Model model; model.receives_effect_callback = true;
    m::Node node; node.scale = {1,1,1}; node.animation = 0; model.nodes.push_back(node);
    const std::array capacities{m::animation::slot_capacity(model)};
    auto result = m::animation::Runtime::create({std::span(&model,1),{},std::span(&program,1),m::no_index,std::span(&path,1)},capacities,{});
    require(bool(result),"source opcode13/17 native program admission failed");
    const auto acquired = result.runtime->acquire(0,0,true);
    require(bool(acquired),"path instance acquisition failed");
    require(bool(result.runtime->step_model(acquired.handle)),"path animation update failed");
    const auto state = result.runtime->model(acquired.handle);
    require(std::fabs(state.nodes[0].transform.position.x-15)<0.0001f,"opcode13 did not drive actual shared node transform");
    const auto events = result.runtime->events();
    require(events.size()==1 && events[0].event_id==4 && events[0].argument==3,
        "opcode17 must deliver real typed callback argument, not a noop");
    require(bool(result.runtime->set_model_speed(acquired.handle,-0.25f)),"signed source road seek speed rejected");
}
void source_control_line_limits() {
    Native n; Input input; Trace trace;
    input.blocks[1].metadata.control_line_count = 4;
    input.blocks[2].metadata.control_line_count = 1;
    w::World world(n.game,input.view(),trace.callbacks());
    require(!world.blocks()[1].control_points && world.blocks()[1].road_model &&
        world.blocks()[2].control_points && !world.blocks()[2].road_model,
        "source control slice and UV object availability were conflated");
    require(world.sample(world.blocks()[1],0.5f,0.5f).speed == 0,
        "numControlLines4 must have no source control-point sample");
    w::MovementState state; w::Movement movement(world,state,{&trace,Trace::exit,Trace::update});
    require(movement.initialize(0),"control-limit movement init");
    state.move_time = 1; state.move_speed = 0.1f; movement.update();
    require(state.block->index == 0 && state.move_speed == 0,"null control slice did not stop source transition");
    state.block = world.enter_first(1); state.move_time = 1; state.move_speed = 0.1f; movement.update();
    require(state.block->index == 2 && !state.block->road_model,
        "source nonnull control slice must permit entry even when no UV model exists");
}
void same_resource_world_owners_and_clear() {
    Native n; Input input; Trace trace;
    input.blocks[1].road_model = input.blocks[0].road_model;
    w::World world(n.game,input.view(),trace.callbacks());
    auto& first = world.blocks()[0];
    auto& second = world.blocks()[1];
    const auto first_handle = first.road_instance, second_handle = second.road_instance;
    const auto* second_nodes = world.animation_model(second_handle).nodes.data();
    require(first_handle != second_handle &&
            world.animation_model(first_handle).resource_model == world.animation_model(second_handle).resource_model,
            "World conflated two real native owners of the same immutable road resource");
    const auto native_cursor = n.arena.cursor;
    world.reset_road(first,12);
    world.reset_road(second,34);
    require(world.animation_model(first_handle).animation_time == 12 &&
            world.animation_model(second_handle).animation_time == 34,
            "World road clock/reset crossed same-resource owners");
    world.clear_block(first);
    require(!world.animation_model(first_handle) &&
            world.animation_model(second_handle).nodes.data() == second_nodes &&
            world.animation_model(second_handle).animation_time == 34,
            "World clear failed to retire its handle or invalidated another resource-sharing owner");
    world.reset_road(second,56);
    require(world.animation_model(second_handle).animation_time == 56,
            "surviving same-resource World owner stopped after neighboring retirement");
    require(n.arena.cursor == native_cursor,
            "World road seeks/retirement advanced native allocation cursor (not an actor-spawn allocation claim)");
}
struct FaultRetirementContext {
    IsleGame* game = nullptr;
    w::World* world = nullptr;
    m::animation::InstanceHandle instance;
    const m::animation::NodeState* nodes = nullptr;
    std::array<IsleGameProcess*,3> parked{};
    unsigned entered = 0, resumed = 0, fault_returned = 0, retired_calls = 0;
    unsigned live_at_retirement = 99;
    m::animation::Error runtime_error_at_retirement = m::animation::Error::none;
    float retained_position = -1;
    bool world_retired_at_callback = true;
    static void sleep(IsleGameObject*, void* user) noexcept {
        auto& c = *static_cast<FaultRetirementContext*>(user);
        ++c.entered;
        if (isle_game_wait(c.game,1000) == ISLE_GAME_OK) ++c.resumed;
    }
    static void fault(IsleGameObject*, void* user) noexcept {
        auto& c = *static_cast<FaultRetirementContext*>(user);
        ohWait(-1);
        ++c.fault_returned;
    }
    static void retired(IsleGameObject*, void* user) noexcept {
        auto& c = *static_cast<FaultRetirementContext*>(user);
        ++c.retired_calls;
        c.live_at_retirement = 0;
        for (auto* process : c.parked) {
            IsleGameSourceCallback source = nullptr;
            if (isle_game_process_source_function(c.game,process,&source) == ISLE_GAME_OK)
                ++c.live_at_retirement;
        }
        c.world_retired_at_callback = c.world->retired();
        if (!c.world_retired_at_callback) {
            c.runtime_error_at_retirement = c.world->animation_model(c.instance).status.error;
            c.retained_position = c.nodes[1].transform.position.x;
        }
    }
};
void faulted_owner_retirement_joins_before_context_release() {
    Native n; Input input; Trace trace;
    w::World world(n.game,input.view(),trace.callbacks());
    auto& block = world.blocks()[0];
    FaultRetirementContext context;
    context.game = n.game; context.world = &world; context.instance = block.road_instance;
    context.nodes = world.animation_model(context.instance).nodes.data();
    ok(isle_game_object_set_retire_callback(n.game,block.road_object,FaultRetirementContext::retired,&context));
    for (std::size_t i = 0; i < context.parked.size(); ++i)
        ok(isle_game_add_process(n.game,block.road_object,FaultRetirementContext::sleep,&context,
                                ISLE_GAME_STACKFUL,static_cast<std::uint32_t>(i),&context.parked[i]));
    ok(isle_game_tick(n.game));
    require(context.entered == 3 && context.resumed == 0,"three real continuations did not park before fault");
    IsleGameObject* fault_owner = nullptr;
    IsleGameProcess* fault_process = nullptr;
    ok(isle_game_add_object(n.game,900,0,0,nullptr,nullptr,&fault_owner));
    ok(isle_game_add_process(n.game,fault_owner,FaultRetirementContext::fault,&context,
                            ISLE_GAME_SYNCHRONOUS,11,&fault_process));
    require(isle_game_tick(n.game) == ISLE_GAME_EXECUTION_FAILURE && context.fault_returned == 0,
            "illegal source wait did not latch a terminal native fault");
    require(isle_menu_end_processes(n.game,block.road_object) == ISLE_GAME_EXECUTION_FAILURE,
            "faulted end_processes unexpectedly concealed the latched failure");
    unsigned survivors = 0;
    for (auto* process : context.parked) {
        IsleGameSourceCallback source = nullptr;
        if (isle_game_process_source_function(n.game,process,&source) == ISLE_GAME_OK) ++survivors;
    }
    require(survivors == 2 && context.retired_calls == 0,
            "faulted helper did not end exactly one process while retaining its owner/context");
    // No graphics have been submitted by this native fixture. World owns its
    // Runtime and keeps it, callback userdata and retiring storage through erase.
    world.retire();
    require(context.retired_calls == 1 && context.live_at_retirement == 0 && context.resumed == 0 &&
            !context.world_retired_at_callback && context.runtime_error_at_retirement == m::animation::Error::handle &&
            context.retained_position == 10 && world.retired() && !world.error().empty(),
            "whole-object fault cleanup released context/animator storage before all real parked processes joined");
    require(isle_game_delete_object(n.game,fault_owner) == ISLE_GAME_EXECUTION_FAILURE,
            "fault owner deletion must preserve the terminal failure, not reset/retry it");
    IsleGameCounts counts{}; ok(isle_game_counts(n.game,&counts));
    require(counts.objects == 0 && counts.processes == 0 && counts.models == 0,
            "latched-fault retirement left an actual native owner or continuation alive");
}
}
int main() {
    try { transitions_and_retirement(); movement_boundary_rotation_and_stop(); position_sampling_preserves_movement_boundary(); required_consumers_and_sky_follow(); path_tracks_callbacks_and_boundaries(); source_control_line_limits();
        same_resource_world_owners_and_clear();
        faulted_owner_retirement_joins_before_context_release();
        std::cout << "native world lifecycle/movement tests passed\n"; return 0; }
    catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
