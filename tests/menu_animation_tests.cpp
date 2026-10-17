#include "menu_animation.hpp"

#include <bit>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>
#include <new>
#include <utility>

namespace allocation_probe { bool enabled = false; std::size_t count = 0; }
void* operator new(std::size_t bytes) {
    if (allocation_probe::enabled) ++allocation_probe::count;
    if (auto* p = std::malloc(bytes ? bytes : 1)) return p;
    throw std::bad_alloc();
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }

namespace m = isle::menu;
namespace a = isle::menu::animation;
namespace {
int failures = 0;
void check(bool condition, std::string_view meaning) {
    if (!condition) { ++failures; std::cerr << "FAIL: " << meaning << '\n'; }
}
std::uint32_t f(float value) { return std::bit_cast<std::uint32_t>(value); }
bool same_transform(const a::Transform& x, const a::Transform& y) {
    const auto same_vec = [](m::Vec3 v, m::Vec3 w) {
        return f(v.x) == f(w.x) && f(v.y) == f(w.y) && f(v.z) == f(w.z);
    };
    return same_vec(x.position,y.position) && same_vec(x.rotation,y.rotation) && same_vec(x.scale,y.scale);
}
bool same_clock(const a::Clock& x, const a::Clock& y) {
    return x.phase == y.phase && f(x.time_left) == f(y.time_left) &&
        f(x.time_passed) == f(y.time_passed) && f(x.speed) == f(y.speed);
}
bool same_node(const a::NodeState& x, const a::NodeState& y) {
    return same_transform(x.transform,y.transform) && x.parent == y.parent &&
        x.first_material == y.first_material && x.material_count == y.material_count &&
        x.flags == y.flags && x.revision == y.revision &&
        x.callback_receiver == y.callback_receiver && same_clock(x.clock,y.clock);
}
bool same_material(const a::MaterialState& x, const a::MaterialState& y) {
    if (x.resource != y.resource || x.image_index != y.image_index || x.next_image_index != y.next_image_index ||
        f(x.palette_index) != f(y.palette_index) || f(x.lod_level) != f(y.lod_level) ||
        x.color_and_trailing_words != y.color_and_trailing_words || x.word_4c != y.word_4c ||
        x.revision != y.revision || !same_clock(x.clock,y.clock)) return false;
    for (std::size_t i = 0; i < x.parameters.size(); ++i)
        if (f(x.parameters[i]) != f(y.parameters[i])) return false;
    return true;
}
bool same_event(const a::CallbackEvent& x, const a::CallbackEvent& y) {
    return x.instance == y.instance && x.node == y.node && x.event_id == y.event_id &&
        f(x.argument) == f(y.argument) && x.command_address == y.command_address && x.source_order == y.source_order;
}
struct Stream {
    std::vector<std::byte> bytes;
    std::vector<std::pair<std::size_t,std::size_t>> extents;
    void cmd(unsigned op, unsigned mask, unsigned duration, std::initializer_list<std::uint32_t> operands = {}) {
        const auto begin = bytes.size();
        auto word = [&](std::uint32_t w) {
            for (int shift = 24; shift >= 0; shift -= 8) bytes.push_back(std::byte((w >> shift)&255u));
        };
        word((op << 25) | (mask << 15) | duration);
        for (auto w : operands) word(w);
        extents.emplace_back(begin,bytes.size()-begin);
    }
    m::Animation view(m::AnimationKind kind, std::uint32_t address) const {
        m::Animation result;
        result.kind = kind;
        result.wire = {address,0,bytes};
        for (auto [offset,size] : extents) {
            isle::rom::GuestView wire{address+static_cast<std::uint32_t>(offset),offset,std::span(bytes).subspan(offset,size)};
            const auto word = wire.u32(0);
            result.commands.push_back({wire,static_cast<std::uint8_t>(word >> 25),
                static_cast<std::uint16_t>((word >> 15)&0x3ff),static_cast<std::uint16_t>(word&0x7fff)});
        }
        return result;
    }
};
struct Fixture {
    std::vector<m::Model> models{1};
    std::vector<m::Material> materials;
    std::vector<m::Animation> animations;
    std::vector<a::InstanceHandle> instances;
    a::CameraState initial_camera{{0,0,100},{0,0,0},0,45,true,true,0,{}};
    Fixture() {
        models[0].nodes.resize(1);
        models[0].nodes[0].position = {99,7,8};
        models[0].nodes[0].scale = {1,1,1};
        models[0].nodes[0].animation = 0;
    }
    a::CreateResult create(std::size_t camera = m::no_index, a::ExecutionCapacity execution = {}) {
        std::vector<a::SlotCapacity> capacities;
        for (const auto& model : models) capacities.push_back(a::slot_capacity(model));
        auto made = a::Runtime::create({models,materials,animations,camera},capacities,initial_camera,execution);
        instances.clear();
        if (made) for (std::size_t i = 0; i < models.size(); ++i) {
            const auto acquired = made.runtime->acquire(i,i,models[i].receives_effect_callback);
            check(bool(acquired),"fixture explicit instance acquisition");
            instances.push_back(acquired.handle);
        }
        return made;
    }
};
void persistent_linear_and_end() {
    Stream s;
    s.cmd(11,16,0,{f(2)}); s.cmd(3,16,2,{f(6)}); s.cmd(0,0,0);
    Fixture fixture;
    fixture.animations.push_back(s.view(m::AnimationKind::model,0x1000));
    auto made = fixture.create(); check(bool(made),"linear stream admitted"); if (!made) return;
    auto& r = *made.runtime;
    const auto* stable = r.model(fixture.instances[0]).nodes.data();
    for (unsigned frame = 0; frame <= 4; ++frame) {
        check(bool(r.step_model(fixture.instances[0])),"persistent update succeeds");
        check(r.model(fixture.instances[0]).nodes[0].transform.position.x == 2.0f+static_cast<float>(frame),"time0 then consecutive binary32 half-frame linear progress");
        check(r.model(fixture.instances[0]).nodes.data() == stable,"frame storage never reallocates");
    }
    check(r.model(fixture.instances[0]).nodes[0].clock.phase == a::Phase::disabled,"END applies final value then disables");
    const auto revision = stable[0].revision;
    check(bool(r.step_model(fixture.instances[0])) && stable[0].transform.position.x == 6 && stable[0].revision == revision,"disabled track neither advances nor dirties");
    check(bool(r.reset()) && stable[0].transform.position.x == 99,"fresh reentry restores bind pose rather than frozen pose0");
    check(bool(r.step_model(fixture.instances[0])) && stable[0].transform.position.x == 2,"reset restarts source time0");
    check(bool(r.reset(0.5f)) && bool(r.step_model(fixture.instances[0])) && stable[0].transform.position.x == 3,"skipFrames applies source negative timeLeft before update");
}
void cubic_wait_flags_and_zero_duration() {
    Stream s;
    s.cmd(2,0,1); s.cmd(15,3,0); s.cmd(11,16,0,{f(2)});
    s.cmd(8,16,2,{f(6)}); s.cmd(0,0,0);
    Fixture fixture; fixture.animations.push_back(s.view(m::AnimationKind::model,0x2000));
    auto made = fixture.create(); check(bool(made),"wait/cubic stream admitted"); if (!made) return;
    auto& r = *made.runtime;
    check(bool(r.step_model(fixture.instances[0])) && r.model(fixture.instances[0]).nodes[0].transform.position.x == 99,"WAIT does not overwrite bind fields");
    check(bool(r.step_model(fixture.instances[0])) && r.model(fixture.instances[0]).nodes[0].flags == 0,"WAIT positive timeLeft avoids parsing next flags");
    check(bool(r.step_model(fixture.instances[0])) && r.model(fixture.instances[0]).nodes[0].transform.position.x == 2 && r.model(fixture.instances[0]).nodes[0].flags == 3,"exact wait boundary executes zero-duration commands in source order");
    check(bool(r.step_model(fixture.instances[0])) && r.model(fixture.instances[0]).nodes[0].transform.position.x == 2.625f,"source grouped cubic at t=.5");
    check(bool(r.set_object_flags(fixture.instances[0], 2)) && bool(r.step_model(fixture.instances[0])) && r.model(fixture.instances[0]).nodes[0].transform.position.x == 2.625f,"GOBJ_FLAG_2 advances time while suppressing model writes");
    check(bool(r.set_object_flags(fixture.instances[0], 0)) && bool(r.step_model(fixture.instances[0])) && r.model(fixture.instances[0]).nodes[0].transform.position.x == 5.375f,"unsuppress publishes advanced track, not paused pose");

    Stream z; z.cmd(11,16,0,{f(2)}); z.cmd(3,16,2,{f(6)});
    z.cmd(4,16,0,{f(10)}); z.cmd(2,0,1); z.cmd(0,0,0);
    Fixture other; other.animations.push_back(z.view(m::AnimationKind::model,0x2800));
    auto zero = other.create(); check(bool(zero),"zero-duration linear stream admitted"); if (!zero) return;
    for (unsigned i = 0; i < 5; ++i) check(bool(zero.runtime->step_model(other.instances[0])),"zero-duration boundary update");
    check(zero.runtime->model(other.instances[0]).nodes[0].transform.position.x == 6,"duration zero preserves previous rate, not snap-to-target fallback");
    check(bool(zero.runtime->step_model(other.instances[0])) && zero.runtime->model(other.instances[0]).nodes[0].transform.position.x == 7,"retained rate progresses after zero-duration command");
}
void callbacks_receivers_and_atomic_errors() {
    Stream s; s.cmd(16,1,0); s.cmd(16,4,0); s.cmd(2,0,1); s.cmd(0,0,0);
    Fixture fixture; fixture.models[0].receives_effect_callback = true;
    fixture.models[0].nodes.resize(3,fixture.models[0].nodes[0]);
    fixture.models[0].nodes[0].animation = m::no_index;
    fixture.models[0].nodes[1].parent = 0; fixture.models[0].nodes[2].parent = 0;
    fixture.animations.push_back(s.view(m::AnimationKind::model,0x3000));
    auto made = fixture.create(); check(bool(made),"callback graph admitted"); if (!made) return;
    auto& r = *made.runtime;
    check(!r.model(fixture.instances[0]).nodes[0].callback_receiver && r.model(fixture.instances[0]).nodes[1].callback_receiver && !r.model(fixture.instances[0]).nodes[2].callback_receiver,"receiver is first non-null animated node, not root");
    check(bool(r.step_model(fixture.instances[0])) && r.events().size() == 4,"no callback coalescing across nodes");
    if (r.events().size() == 4) {
        check(r.events()[0].node == 1 && r.events()[1].node == 1 && r.events()[2].node == 2 && r.events()[3].node == 2,"native depth-first callback node ordering");
        check(r.events()[0].event_id == 0 && r.events()[0].argument == 1 && r.events()[1].argument == 4,"opcode16 high bits eventID and low byte float argument");
        check(r.events()[0].command_address == 0x3000 && r.events()[3].source_order == 3,"callback provenance and exact global source order");
    }
    const auto generation = r.generation();
    const auto* events = r.events().data();
    check(r.step_model({}).error == a::Error::handle && r.generation() == generation && r.events().data() == events && r.events().size() == 4,"invalid update fails before any frame/event publication");
    check(r.reset(std::numeric_limits<float>::quiet_NaN()).error == a::Error::malformed && r.generation() == generation,"invalid reset is failure atomic");
    check(bool(r.step_model(fixture.instances[0])) && r.events().empty(),"callbacks are not replayed by resampling");
    check(bool(r.step_model(fixture.instances[0])) && r.events().size() == 1 && r.events()[0].event_id == -1 && r.events()[0].node == 1,"only designated receiver emits END callback");
    made.runtime.reset(); // End the immutable borrowed resource lease before editing fixture.
    fixture.models[0].receives_effect_callback = false;
    auto silent = fixture.create(); check(bool(silent),"sourceNULL callback graph admitted");
    if (silent) check(bool(silent.runtime->step_model(fixture.instances[0])) && silent.runtime->events().empty(),"sourceNULL opcode16 consumed without dispatch");
}
void texture_track_time_offset() {
    Stream shifted;
    shifted.cmd(4,2,4,{f(4)}); shifted.cmd(12,2,1);
    shifted.cmd(2,0,4); shifted.cmd(0,0,0);
    Stream baseline;
    baseline.cmd(4,2,4,{f(4)}); baseline.cmd(2,0,4); baseline.cmd(0,0,0);
    std::array<std::byte,0x78> material_bytes{};
    Fixture fixture;
    fixture.models[0].animation_speed = 1;
    fixture.models[0].nodes[0].animation = m::no_index;
    fixture.models[0].nodes[0].materials = {0,0};
    fixture.models[0].nodes[0].material_animations = {0,1};
    fixture.materials.resize(1);
    fixture.materials[0].wire = {0x3c00,0,material_bytes};
    fixture.animations = {shifted.view(m::AnimationKind::texture,0x3a00),
                          baseline.view(m::AnimationKind::texture,0x3b00)};
    auto made = fixture.create();
    check(bool(made),"texture opcode12 must compile through the shared animator");
    if (!made) return;
    auto& runtime = *made.runtime;
    const auto handle = fixture.instances[0];
    check(bool(runtime.step_model(handle)) &&
          runtime.model(handle).materials[0].parameters[0] == 1 &&
          runtime.model(handle).materials[1].parameters[0] == 0 &&
          runtime.model(handle).materials[0].clock.time_left == 4 &&
          runtime.model(handle).materials[1].clock.time_left == 4,
          "texture opcode12 advances only selected track time, not the source wait");
    check(bool(runtime.step_model(handle)) &&
          runtime.model(handle).materials[0].parameters[0] == 2 &&
          runtime.model(handle).materials[1].parameters[0] == 1,
          "one-tick texture track offset persists during later interpolation");
}

void materials_and_camera() {
    Stream color;
    color.cmd(19,1,0,{0x10203040}); color.cmd(20,1,2,{0x90a0b0c0}); color.cmd(0,0,0);
    Stream camera;
    camera.cmd(11,1u<<9,0,{f(40)}); camera.cmd(3,1u<<9,2,{f(60)}); camera.cmd(0,0,0);
    std::vector<std::byte> material_bytes(0x78);
    Fixture fixture; fixture.models[0].nodes[0].animation = m::no_index;
    fixture.models[0].nodes[0].materials = {0}; fixture.models[0].nodes[0].material_animations = {0};
    fixture.materials.resize(1); fixture.materials[0].wire = {0x4000,0,material_bytes};
    fixture.materials[0].parameters = {2,3,4,5,6,7,8,9,10};
    fixture.materials[0].color_and_trailing_words[1] = 0x80010203;
    fixture.animations.push_back(color.view(m::AnimationKind::texture,0x5000));
    fixture.animations.push_back(camera.view(m::AnimationKind::camera,0x6000));
    auto made = fixture.create(1); check(bool(made),"material/color and camera stream graph admitted"); if (!made) return;
    auto& r = *made.runtime;
    check(r.model(fixture.instances[0]).materials[0].parameters[4] == 2 && r.model(fixture.instances[0]).materials[0].parameters[5] == 4,"MObj initial source14/scaleS copied to24/28");
    check(r.model(fixture.instances[0]).materials[0].lod_level == 128.0f/255.0f,"lod seed is byte54, not packed word54");
    check(bool(r.step_model(fixture.instances[0])) && r.model(fixture.instances[0]).materials[0].color_and_trailing_words[0] == 0x10203040,"color target uses raw RGBA word, not float numeric conversion");
    check(bool(r.step_model(fixture.instances[0])) && r.model(fixture.instances[0]).materials[0].color_and_trailing_words[0] == 0x30405060,"source8.8 color interpolation changes consecutive frame");
    check(bool(r.step_camera()) && r.camera().fov_y == 40,"camera first update time0");
    check(bool(r.step_camera()) && r.camera().fov_y == 45 && !r.camera().view_dirty && r.camera().projection_dirty,"FOV-only change invalidates projection, not kind8 fixedYup view");
    check(r.camera().eye.z == 100 && r.camera().written_channels == (1u<<9),"unwritten camera channels preserve caller initial fields");
    check(bool(r.step_camera(2)) && r.camera().fov_y == 45 && !r.camera().projection_dirty,"camera object flag2 advances without publishing");
    check(bool(r.step_camera()) && r.camera().fov_y == 55,"camera resumes from actual elapsed clock");
    r.retire(); check(r.step_model(fixture.instances[0]).error == a::Error::retired && r.step_camera().error == a::Error::retired && r.events().empty(),"retirement stops all publication");
}
void rates_and_source_restart() {
    Stream s;
    s.cmd(11,16,0,{f(2)}); s.cmd(7,16,0,{f(2)});
    s.cmd(5,16,2,{f(6),f(0)}); s.cmd(0,0,0);
    Fixture fixture; fixture.animations.push_back(s.view(m::AnimationKind::model,0x6800));
    auto made = fixture.create(); check(bool(made),"explicit target-rate cubic stream admitted"); if (!made) return;
    auto& r = *made.runtime;
    check(bool(r.step_model(fixture.instances[0])) && bool(r.step_model(fixture.instances[0])) &&
          r.model(fixture.instances[0]).nodes[0].transform.position.x == 3.1875f,"cubic inherits previous target-rate as initial tangent");
    const auto preserved = r.model(fixture.instances[0]).nodes[0].transform.position.x;
    check(bool(r.restart_model(fixture.instances[0])) && r.model(fixture.instances[0]).nodes[0].clock.phase == a::Phase::changed &&
          r.model(fixture.instances[0]).nodes[0].transform.position.x == preserved,"source animSet keeps fields until next evaluation");
    check(bool(r.step_model(fixture.instances[0])) && r.model(fixture.instances[0]).nodes[0].transform.position.x == 2,"source restart processes time0 with retained allocated tracks");
    check(bool(r.reset()) && r.model(fixture.instances[0]).nodes[0].transform.position.x == 99,"fresh reset differs from source restart field preservation");
}
void local_pose_authority_and_waits() {
    Stream child;
    child.cmd(15,7,0); child.cmd(16,0x103,0); child.cmd(2,0,64); child.cmd(0,0,0);
    Stream texture;
    texture.cmd(11,2,0,{f(2)}); texture.cmd(3,2,64,{f(130)}); texture.cmd(0,0,0);
    std::array<std::byte,0x78> material_bytes{};
    Fixture fixture;
    fixture.models[0].nodes[0].animation = m::no_index;
    auto descendant = fixture.models[0].nodes[0];
    descendant.parent = 0;
    descendant.animation = 0;
    fixture.models[0].nodes.push_back(descendant);
    fixture.models[0].nodes[0].materials = {0};
    fixture.models[0].nodes[0].material_animations = {1};
    fixture.materials.resize(1);
    fixture.materials[0].wire = {0x7200,0,material_bytes};
    fixture.animations = {child.view(m::AnimationKind::model,0x7300),
                          texture.view(m::AnimationKind::texture,0x7400)};
    const auto capacity = a::slot_capacity(fixture.models[0]);
    const std::array<a::SlotCapacity,2> capacities{capacity,capacity};
    auto made = a::Runtime::create({fixture.models,fixture.materials,fixture.animations},
                                   capacities,fixture.initial_camera,{});
    check(bool(made),"local-pose same-resource instance pool admitted"); if (!made) return;
    auto& r = *made.runtime;
    const auto acquired = r.acquire(0,0,true), other = r.acquire(1,0,true);
    check(bool(acquired) && bool(other),"local-pose independent owners acquired"); if (!acquired || !other) return;
    const auto h = acquired.handle, peer = other.handle;
    check(bool(r.step_model(h)) && bool(r.set_object_flags(h,1)) && r.events().size() == 1,
          "local-pose source wait leaves one borrowed child callback");
    if (r.events().size() != 1) return;
    const auto view = r.model(h);
    const auto* nodes = view.nodes.data();
    const auto* materials = view.materials.data();
    const auto child_before = nodes[1], peer_root = r.model(peer).nodes[0], peer_child = r.model(peer).nodes[1];
    const auto material_before = materials[0], peer_material = r.model(peer).materials[0];
    const auto emitted = r.events();
    const auto event = emitted[0];
    const auto generation = r.generation();
    a::Transform pose{{99,7,8},{0.25f,-0.5f,0.75f},{0,-2,1}};
    auto expected_root = nodes[0];
    expected_root.transform = pose;
    ++expected_root.revision;
    check(bool(r.set_node_transform(h,0,pose)) && same_node(nodes[0],expected_root) &&
          same_node(nodes[1],child_before) && same_material(materials[0],material_before) &&
          r.model(h).nodes.data() == nodes && r.model(h).materials.data() == materials &&
          r.generation() == generation+1 && r.model(h).animation_time == view.animation_time &&
          r.model(h).object_flags == 1 && r.events().data() == emitted.data() &&
          r.events().size() == 1 && same_event(r.events()[0],event),
          "local pose publishes in authoritative storage, dirtying only its node without consuming borrowed events");
    const auto unchanged_generation = r.generation();
    check(bool(r.set_node_transform(h,0,pose)) && r.generation() == unchanged_generation &&
          same_node(nodes[0],expected_root) && r.events().size() == 1 && same_event(r.events()[0],event),
          "bitwise-identical local pose leaves revisions, generation and pending callbacks unchanged");
    bool hot_ok = true;
    allocation_probe::count = 0;
    allocation_probe::enabled = true;
    // GAME common1003 changes root-local Y once before each of 59 source waits.
    for (unsigned wait = 0; wait < 59; ++wait) {
        pose.position.y -= 0.2f;
        const auto revision = nodes[0].revision;
        hot_ok &= bool(r.set_node_transform(h,0,pose)) && same_transform(nodes[0].transform,pose) &&
                  nodes[0].revision == revision+1;
        hot_ok &= bool(r.step_model(h)) && same_transform(nodes[0].transform,pose) &&
                  nodes[0].revision == revision+1;
    }
    allocation_probe::enabled = false;
    const auto allocations = allocation_probe::count;
    check(hot_ok && nodes[0].clock.phase == a::Phase::disabled &&
          nodes[1].clock.time_passed == 29.5f && nodes[1].clock.time_left == 34.5f &&
          nodes[1].flags == 7 && nodes[1].callback_receiver && materials[0].parameters[0] == 61 &&
          r.model(h).animation_time == 29.5f && r.model(h).object_flags == 1,
          "59 source root-local decrements survive animation waits while child clocks and material tracks continue");
    check(allocations == 0,"repeated local-pose mutation and source wait evaluation allocate no heap storage");
    const auto root_before_child_write = nodes[0];
    auto changed_child = nodes[1];
    changed_child.transform.position = {3,4,5};
    ++changed_child.revision;
    const auto material_before_child_write = materials[0];
    const auto before_child_generation = r.generation();
    check(bool(r.set_node_transform(h,1,changed_child.transform)) && same_node(nodes[1],changed_child) &&
          same_node(nodes[0],root_before_child_write) && same_material(materials[0],material_before_child_write) &&
          r.generation() == before_child_generation+1,
          "non-root local mutation selects exactly the requested node, preserving its active wait, flags and receiver");
    check(same_node(r.model(peer).nodes[0],peer_root) && same_node(r.model(peer).nodes[1],peer_child) &&
          same_material(r.model(peer).materials[0],peer_material) && r.model(peer).animation_time == 0 &&
          r.model(peer).object_flags == 0,
          "local writes and waiting animation never alter a second instance of the same immutable resource");
}

void local_pose_animation_overwrite_and_null_binding() {
    Stream original;
    original.cmd(15,7,0); original.cmd(11,16|32,0,{f(2),f(10)});
    original.cmd(3,16|32,2,{f(6),f(18)}); original.cmd(0,0,0);
    Stream retained;
    retained.cmd(4,16,0,{f(40)}); retained.cmd(2,0,4); retained.cmd(0,0,0);
    Fixture fixture;
    fixture.models[0].animation_speed = 0.25f;
    auto child = fixture.models[0].nodes[0];
    child.parent = 0;
    child.animation = m::no_index;
    fixture.models[0].nodes.push_back(child);
    fixture.animations = {original.view(m::AnimationKind::model,0x7500),
                          retained.view(m::AnimationKind::model,0x7600)};
    auto made = fixture.create();
    check(bool(made),"local-pose animated override streams admitted"); if (!made) return;
    auto& r = *made.runtime;
    const auto h = fixture.instances[0];
    check(bool(r.step_model(h)) && bool(r.step_model(h)) &&
          r.model(h).nodes[0].transform.position.x == 2.5f &&
          r.model(h).nodes[0].transform.position.y == 11,
          "authored tracks advance on their original quarter-frame source clock");
    const auto* nodes = r.model(h).nodes.data();
    const auto child_before = nodes[1];
    a::Transform pose{{400,-100,88},{2,-3,4},{-1,0,2}};
    auto expected = nodes[0];
    expected.transform = pose;
    ++expected.revision;
    const auto generation = r.generation();
    const auto animation_time = r.model(h).animation_time;
    check(bool(r.set_node_transform(h,0,pose)) && same_node(nodes[0],expected) &&
          same_node(nodes[1],child_before) && r.generation() == generation+1 &&
          r.model(h).animation_time == animation_time,
          "animated local override preserves tracks' clocks, speed, receiver, flags and sibling state");
    auto animated_pose = pose;
    animated_pose.position.x = 3;
    animated_pose.position.y = 12;
    check(bool(r.step_model(h)) && same_transform(nodes[0].transform,animated_pose) &&
          nodes[0].clock.time_passed == 0.5f && nodes[0].clock.time_left == 1.5f &&
          nodes[0].clock.speed == 0.25f && nodes[0].revision == expected.revision+1,
          "next animation publication overwrites only authored channels using retained targets, rates and elapsed clock");
    const std::array<std::size_t,2> nulls{m::no_index,m::no_index};
    check(bool(r.bind_model(h,nulls,3.5f)) && nodes[0].clock.phase == a::Phase::disabled &&
          nodes[0].clock.time_passed == 0.5f && nodes[0].clock.time_left == 1.5f,
          "source null binding disables animation without clearing the retained stream and tracks");
    pose.position = {500,-200,89};
    expected = nodes[0];
    expected.transform = pose;
    ++expected.revision;
    check(bool(r.set_node_transform(h,0,pose)) && same_node(nodes[0],expected) &&
          bool(r.step_model(h)) && bool(r.step_model(h)) && same_node(nodes[0],expected) &&
          r.model(h).animation_time == 3.5f && same_node(nodes[1],child_before),
          "null-bound local override persists across updates without advancing disabled clocks or restoring a hidden pose");
    const std::array<std::size_t,2> rebound{1,m::no_index};
    animated_pose = pose;
    animated_pose.position.x = 7;
    check(bool(r.bind_model(h,rebound,0.5f)) && bool(r.step_model(h)) &&
          same_transform(nodes[0].transform,animated_pose) && nodes[0].clock.speed == 0.25f &&
          nodes[0].clock.time_passed == 0.5f && nodes[0].callback_receiver && nodes[0].flags == 7,
          "rebind after local/null writes inherits original target6 and rate2, not the overridden position or fresh tracks");
    animated_pose.position.x = 7.5f;
    check(bool(r.step_model(h)) && same_transform(nodes[0].transform,animated_pose) &&
          nodes[0].clock.time_passed == 0.75f,
          "zero-duration retained-rate animation continues normally after overriding and null-binding");
}

void local_pose_admission_atomicity_and_retirement() {
    Stream stream;
    stream.cmd(16,0x109,0); stream.cmd(2,0,1);
    stream.cmd(16,0x20a,0); stream.cmd(2,0,1); stream.cmd(0,0,0);
    std::array<std::byte,0x78> material_bytes{};
    Fixture fixture;
    fixture.models[0].receives_effect_callback = true;
    auto child = fixture.models[0].nodes[0];
    child.parent = 0;
    child.animation = m::no_index;
    fixture.models[0].nodes.push_back(child);
    fixture.models[0].nodes[0].materials = {0};
    fixture.materials.resize(1);
    fixture.materials[0].wire = {0x7700,0,material_bytes};
    fixture.materials[0].parameters = {2,3,4,5,6,7,8,9,10};
    fixture.animations = {stream.view(m::AnimationKind::model,0x7800)};
    auto made = fixture.create();
    const std::array<a::SlotCapacity,1> capacity{a::slot_capacity(fixture.models[0])};
    auto foreign = a::Runtime::create({fixture.models,fixture.materials,fixture.animations},capacity,{},{});
    check(bool(made) && bool(foreign),"local-pose rejection pools admitted"); if (!made || !foreign) return;
    auto& r = *made.runtime;
    const auto h = fixture.instances[0];
    const auto foreign_instance = foreign.runtime->acquire(0,0,true);
    check(bool(foreign_instance),"local-pose wrong-pool owner acquired"); if (!foreign_instance) return;
    check(bool(r.step_model(h)) && bool(r.set_object_flags(h,1)) && r.events().size() == 1,
          "local-pose atomicity begins with live borrowed events");
    if (r.events().size() != 1) return;
    const auto* nodes = r.model(h).nodes.data();
    const auto* materials = r.model(h).materials.data();
    const auto maximum = std::numeric_limits<float>::max();
    a::Transform finite{{maximum,-maximum,std::numeric_limits<float>::denorm_min()},
                        {maximum,-maximum,1000000},{0,-2,-0.0f}};
    const auto initial_generation = r.generation(), initial_revision = nodes[0].revision;
    check(bool(r.set_node_transform(h,0,finite)) && same_transform(nodes[0].transform,finite) &&
          nodes[0].revision == initial_revision+1 && r.generation() == initial_generation+1,
          "source-state admission accepts full finite position/RPY and zero/negative scale without renderer quantization limits");
    finite.scale.x = -0.0f;
    check(bool(r.set_node_transform(h,0,finite)) && f(nodes[0].transform.scale.x) == f(-0.0f) &&
          nodes[0].revision == initial_revision+2 && r.generation() == initial_generation+2,
          "binary32 signed-zero changes are published and dirty the authoritative pose");
    const auto generation = r.generation();
    const std::array<a::NodeState,2> before{nodes[0],nodes[1]};
    const auto material_before = materials[0];
    const auto view = r.model(h);
    const auto emitted = r.events();
    const auto event = emitted[0];
    const auto unchanged = [&] {
        const auto current = r.model(h);
        return r.generation() == generation && current.nodes.data() == nodes &&
            current.materials.data() == materials && same_node(nodes[0],before[0]) &&
            same_node(nodes[1],before[1]) && same_material(materials[0],material_before) &&
            f(current.animation_time) == f(view.animation_time) && current.object_flags == view.object_flags &&
            current.instance == h && current.resource_model == view.resource_model &&
            r.events().data() == emitted.data() && r.events().size() == 1 && same_event(r.events()[0],event);
    };
    check(bool(r.set_node_transform(h,0,finite)) && unchanged(),
          "same signed-zero pose is a no-op including frame generation and borrowed event storage");
    a::Transform bad{{11,12,13},{14,15,16},{17,18,19}};
    const std::array<float*,9> scalars{&bad.position.x,&bad.position.y,&bad.position.z,
        &bad.rotation.x,&bad.rotation.y,&bad.rotation.z,&bad.scale.x,&bad.scale.y,&bad.scale.z};
    bool atomic = true;
    for (const auto invalid : {std::numeric_limits<float>::quiet_NaN(),
                              std::numeric_limits<float>::infinity(),-std::numeric_limits<float>::infinity()})
        for (auto* scalar : scalars) {
            const auto value = *scalar;
            *scalar = invalid;
            atomic &= r.set_node_transform(h,0,bad).error == a::Error::malformed && unchanged();
            *scalar = value;
        }
    bad.scale.z = std::numeric_limits<float>::quiet_NaN();
    auto wrong_incarnation = h; ++wrong_incarnation.incarnation;
    auto wrong_slot = h; wrong_slot.slot = 1;
    atomic &= r.set_node_transform(h,2,bad).error == a::Error::extent && unchanged();
    atomic &= r.set_node_transform(h,m::no_index,bad).error == a::Error::extent && unchanged();
    atomic &= r.set_node_transform({},m::no_index,bad).error == a::Error::handle && unchanged();
    atomic &= r.set_node_transform(foreign_instance.handle,0,bad).error == a::Error::handle && unchanged();
    atomic &= r.set_node_transform(wrong_incarnation,0,bad).error == a::Error::handle && unchanged();
    atomic &= r.set_node_transform(wrong_slot,0,bad).error == a::Error::handle && unchanged();
    check(atomic,"all nine NaN/infinite scalars, late invalid input, extent, pool, slot and incarnation reject atomically with existing precedence");
    check(bool(r.step_model(h)) && bool(r.step_model(h)) && r.events().size() == 1 &&
          r.events()[0].source_order == event.source_order+1 && r.events()[0].event_id == 2 &&
          r.events()[0].argument == 10 && same_transform(nodes[0].transform,finite),
          "accepted/no-op/rejected local writes neither advance command cursors nor consume callback source order");
    const auto retained_node = nodes[0];
    check(bool(r.begin_retire(h)),"local-pose owner begins retirement");
    const auto retiring_generation = r.generation();
    check(r.set_node_transform(h,0,bad).error == a::Error::handle &&
          r.set_node_transform(h,m::no_index,finite).error == a::Error::handle &&
          r.generation() == retiring_generation && !r.model(h) && r.events().empty() &&
          same_node(nodes[0],retained_node),
          "retiring instance rejects local writes without touching retained owner storage");
    // This pure animator fixture has no native, graphics or effect borrows.
    check(bool(r.release_retired(h)),"local-pose pure owner releases retired storage");
    const auto free_generation = r.generation();
    check(r.set_node_transform(h,0,finite).error == a::Error::handle &&
          r.generation() == free_generation && !r.model(h),
          "released free slot rejects the old incarnation before any local publication");
    const auto reacquired = r.acquire(0,0,true);
    check(bool(reacquired),"local-pose slot reacquired after release"); if (!reacquired) return;
    const auto fresh = r.model(reacquired.handle).nodes[0];
    const auto reused_generation = r.generation();
    check(r.set_node_transform(h,0,finite).error == a::Error::handle &&
          r.generation() == reused_generation && same_node(r.model(reacquired.handle).nodes[0],fresh) &&
          r.set_node_transform(reacquired.handle,0,fresh.transform) && r.generation() == reused_generation,
          "stale handle cannot overwrite a reused slot, whose fresh identical pose remains clean");
    r.retire();
    const auto retired_generation = r.generation();
    check(r.set_node_transform(reacquired.handle,0,finite).error == a::Error::retired &&
          r.set_node_transform({},m::no_index,bad).error == a::Error::retired &&
          r.generation() == retired_generation && r.events().empty(),
          "whole-runtime retirement takes precedence and rejects local writes without publication");
}

void model_binding_switches_and_instances() {
    Stream original;
    original.cmd(15,7,0); original.cmd(11,16,0,{f(2)});
    original.cmd(3,16,2,{f(6)}); original.cmd(0,0,0);
    Stream replacement;
    replacement.cmd(4,16,0,{f(20)}); replacement.cmd(2,0,4); replacement.cmd(0,0,0);
    Fixture fixture;
    fixture.animations = {original.view(m::AnimationKind::model,0x8000),
                          replacement.view(m::AnimationKind::model,0x8100)};
    fixture.models[0].nodes.resize(2,fixture.models[0].nodes[0]);
    fixture.models[0].nodes[1].parent = 0;
    fixture.models.push_back(fixture.models[0]);
    auto made = fixture.create();
    check(bool(made),"switchable model instances admitted"); if (!made) return;
    auto& r = *made.runtime;
    const auto* nodes = r.model(fixture.instances[0]).nodes.data();
    check(bool(r.step_model(fixture.instances[0])) && bool(r.step_model(fixture.instances[0])) && nodes[1].transform.position.x == 3,
          "original model advances before switching");
    const auto revision = nodes[1].revision;
    const std::array<std::size_t,2> next{m::no_index,1};
    check(bool(r.bind_model(fixture.instances[0], next,0.5f,a::ModelBindPose::restore)) &&
          nodes[0].clock.phase == a::Phase::disabled && !nodes[0].callback_receiver &&
          nodes[0].clock.time_passed == 0.5f && nodes[0].flags == 7 &&
          nodes[1].callback_receiver && nodes[1].clock.phase == a::Phase::changed &&
          nodes[1].transform.position.x == 99 && nodes[1].revision > revision && r.events().empty(),
          "model null disables retaining clock/flags; bind helper restores pose and reselects receiver without evaluation");
    check(bool(r.step_model(fixture.instances[0])) && nodes[1].transform.position.x == 7 &&
          nodes[1].clock.time_passed == 0.5f && nodes[0].transform.position.x == 99,
          "different stream inherits previous target and zero-duration rate, applies skip, leaves disabled node alone");
    check(bool(r.set_model_speed(fixture.instances[0], -0.5f)) && bool(r.step_model(fixture.instances[0])) &&
          nodes[1].transform.position.x == 6 && nodes[1].clock.time_passed == 0,
          "signed model speed seeks the newly bound stream backwards");
    check(r.model(fixture.instances[1]).nodes[1].transform.position.x == 99 && r.model(fixture.instances[1]).nodes[1].clock.speed == 0.5f &&
          bool(r.step_model(fixture.instances[1])) && r.model(fixture.instances[1]).nodes[1].transform.position.x == 2 &&
          nodes[1].transform.position.x == 6,"same immutable programs have independent instance bindings, tracks and speed");
    const std::array<std::size_t,2> retained{1,m::no_index};
    check(bool(r.bind_model(fixture.instances[0], retained,-0.5f)) && nodes[0].transform.position.x == 99 &&
          nodes[1].transform.position.x == 6 && bool(r.step_model(fixture.instances[0])) &&
          nodes[0].transform.position.x == 5 && nodes[0].clock.time_passed == -0.5f,
          "preserve-pose bind keeps fields until evaluation; negative skip retains source track timing");
    check(r.model(fixture.instances[0]).nodes.data() == nodes && bool(r.reset()) &&
          nodes[0].clock.speed == 0.5f && nodes[1].transform.position.x == 99 &&
          nodes[0].callback_receiver && !nodes[1].callback_receiver &&
          bool(r.step_model(fixture.instances[0])) && nodes[1].transform.position.x == 2,
          "full reset restores original bindings, pose, receiver and speed at stable native addresses");
}

void switched_callbacks_and_atomicity() {
    Stream quiet; quiet.cmd(2,0,5); quiet.cmd(0,0,0);
    Stream events;
    events.cmd(16,0x102,0); events.cmd(17,5,0,{f(7),f(9)});
    events.cmd(2,0,1); events.cmd(0,0,0);
    Stream texture; texture.cmd(2,0,1); texture.cmd(0,0,0);
    Fixture fixture;
    fixture.animations = {quiet.view(m::AnimationKind::model,0x8200),
                          events.view(m::AnimationKind::model,0x8300),
                          texture.view(m::AnimationKind::texture,0x8400)};
    fixture.models[0].receives_effect_callback = true;
    fixture.models[0].nodes.resize(3,fixture.models[0].nodes[0]);
    fixture.models[0].nodes[1].parent = 0; fixture.models[0].nodes[2].parent = 0;
    auto made = fixture.create();
    check(bool(made),"initially quiet callback model admitted"); if (!made) return;
    auto& r = *made.runtime;
    const std::array<std::size_t,3> binding{m::no_index,1,1};
    check(bool(r.bind_model(fixture.instances[0], binding,2)) && r.events().empty() && bool(r.step_model(fixture.instances[0])) &&
          r.events().size() == 7,"rebind reserves for new callback-rich programs and immediate skip crosses events/END");
    if (r.events().size() != 7) return;
    const auto emitted = r.events();
    check(emitted[0].node == 1 && emitted[0].event_id == 1 && emitted[0].argument == 2 &&
          emitted[1].event_id == 4 && emitted[1].argument == 7 &&
          emitted[2].event_id == 6 && emitted[2].argument == 9 &&
          emitted[3].node == 1 && emitted[3].event_id == -1 &&
          emitted[4].node == 2 && emitted[6].event_id == 6,
          "new receiver alone ends; opcode16/17 and per-node source order are routed without coalescing");
    for (std::size_t i = 1; i < emitted.size(); ++i)
        check(emitted[i].source_order == emitted[i-1].source_order+1,"switched event sequence remains ordered");
    const auto generation = r.generation();
    const auto revision = r.model(fixture.instances[0]).nodes[0].revision;
    const auto time = r.model(fixture.instances[0]).animation_time;
    const std::array<std::size_t,3> bad_kind{0,0,2}, bad_index{0,0,999};
    const std::array<std::size_t,1> bad_extent{0};
    check(r.bind_model(fixture.instances[0], bad_kind,0,a::ModelBindPose::restore).error == a::Error::malformed &&
          r.bind_model(fixture.instances[0], bad_index).error == a::Error::malformed &&
          r.bind_model(fixture.instances[0], bad_extent).error == a::Error::extent &&
          r.bind_model(fixture.instances[0], binding,std::numeric_limits<float>::infinity()).error == a::Error::malformed &&
          r.bind_model({},binding).error == a::Error::handle &&
          r.generation() == generation && r.events().data() == emitted.data() && r.events().size() == 7 &&
          r.events()[3].event_id == -1 && r.model(fixture.instances[0]).nodes[0].revision == revision &&
          r.model(fixture.instances[0]).nodes[1].callback_receiver && r.model(fixture.instances[0]).animation_time == time,
          "late invalid program/type/extent/skip reject atomically, preserving event and frame publication");
    check(bool(r.step_model(fixture.instances[0])) && r.events().empty(),"switched END is not replayed on next update");
    r.retire();
    check(r.bind_model(fixture.instances[0], binding).error == a::Error::retired &&
          r.bind_materials(fixture.instances[0], {}).error == a::Error::retired &&
          r.reset_materials(fixture.instances[0]).error == a::Error::retired && r.model(fixture.instances[0]).nodes.empty() && r.events().empty(),
          "retirement rejects every new binding/reset entry point");
}

void material_binding_null_and_initial_reset() {
    Stream original;
    original.cmd(11,1|256|512,0,{f(2),f(2),f(3)});
    original.cmd(19,1,0,{0x10203040}); original.cmd(21,1,2,{0x50607080});
    original.cmd(3,1,2,{f(6)}); original.cmd(0,0,0);
    Stream replacement;
    replacement.cmd(4,1,0,{f(20)}); replacement.cmd(21,1,0,{0x90a0b0c0});
    replacement.cmd(2,0,4); replacement.cmd(0,0,0);
    Stream model; model.cmd(2,0,1); model.cmd(0,0,0);
    std::vector<std::byte> material_bytes(0x78);
    Fixture fixture;
    fixture.models[0].nodes[0].animation = m::no_index;
    fixture.models[0].nodes[0].materials = {0,1};
    fixture.models[0].nodes[0].material_animations = {0,0};
    fixture.materials.resize(3);
    for (auto& material : fixture.materials) {
        material.wire = {0x8500,0,material_bytes};
        material.parameters = {2,3,4,5,6,7,8,9,10};
        material.color_and_trailing_words[1] = 0x80010203;
    }
    fixture.materials[2].parameters[0] = 99;
    fixture.models.push_back(fixture.models[0]);
    fixture.animations = {original.view(m::AnimationKind::texture,0x8600),
                          replacement.view(m::AnimationKind::texture,0x8700),
                          model.view(m::AnimationKind::model,0x8800)};
    auto made = fixture.create();
    check(bool(made),"switchable attached materials admitted"); if (!made) return;
    auto& r = *made.runtime;
    const auto* materials = r.model(fixture.instances[0]).materials.data();
    check(bool(r.step_model(fixture.instances[0])) && bool(r.step_model(fixture.instances[0])) &&
          materials[0].image_index == 3 && materials[0].palette_index == 3,"original texture tracks advance");
    const std::array<std::size_t,2> next{1,m::no_index};
    check(bool(r.bind_materials(fixture.instances[0], next,0.5f)) && materials[0].image_index == 3 &&
          materials[0].clock.phase == a::Phase::changed && materials[1].clock.phase == a::Phase::running &&
          materials[1].clock.time_passed == 0.5f && r.model(fixture.instances[0]).animation_time == 0.5f &&
          bool(r.step_model(fixture.instances[0])) && materials[0].image_index == 7 && materials[1].image_index == 4 &&
          materials[0].color_and_trailing_words[0] == 0x60708090 && materials[0].palette_index == 3,
          "new texture stream retains scalar rate/color targets/inverse duration; null entry continues old stream");
    check(bool(r.set_texture_speed(fixture.instances[0], -0.5f)) && bool(r.step_model(fixture.instances[0])) &&
          materials[0].image_index == 6 && materials[1].image_index == 3,
          "signed texture speed acts independently on switched and retained programs");
    check(r.model(fixture.instances[1]).materials[0].image_index == 0 && r.model(fixture.instances[1]).materials[0].clock.speed == 0.5f &&
          bool(r.step_model(fixture.instances[1])) && r.model(fixture.instances[1]).materials[0].image_index == 2 &&
          materials[0].image_index == 6 && materials[0].clock.speed == -0.5f,
          "shared texture resources/programs do not share switched bindings, tracks or signed clocks between instances");
    const auto time = materials[0].clock.time_passed;
    check(bool(r.bind_materials(fixture.instances[0], {},-2)) && materials[0].clock.time_passed == time &&
          materials[0].clock.phase == a::Phase::running && r.model(fixture.instances[0]).animation_time == -2,
          "null outer texture array changes only object animation time, not material clocks/bindings");
    const auto generation = r.generation(), revision = materials[0].revision;
    const std::array<std::size_t,2> bad_kind{1,2}, bad_resource{2,999};
    const std::array<std::size_t,1> bad_extent{1};
    check(r.bind_materials(fixture.instances[0], bad_kind).error == a::Error::malformed &&
          r.bind_materials(fixture.instances[0], bad_extent).error == a::Error::extent &&
          r.bind_materials(fixture.instances[0], next,std::numeric_limits<float>::quiet_NaN()).error == a::Error::malformed &&
          r.reset_materials(fixture.instances[0], bad_resource).error == a::Error::extent &&
          r.reset_materials(fixture.instances[0], bad_extent).error == a::Error::extent &&
          r.generation() == generation && materials[0].revision == revision &&
          materials[0].image_index == 6 && materials[0].clock.time_passed == time &&
          r.model(fixture.instances[0]).animation_time == -2,"late texture/reset rejection cannot publish a valid prefix");
    const std::array<std::size_t,2> reset_one{2,m::no_index};
    check(bool(r.reset_materials(fixture.instances[0], reset_one)) && materials[0].resource == 2 &&
          materials[0].parameters[0] == 99 && materials[0].parameters[4] == 99 &&
          materials[0].parameters[5] == 4 && materials[0].palette_index == 3 &&
          materials[0].lod_level == 128.0f/255.0f && materials[0].image_index == 0 &&
          materials[0].next_image_index == 0 && materials[0].clock.phase == a::Phase::disabled &&
          materials[0].clock.time_passed == 0 && materials[0].clock.speed == 1 &&
          materials[0].revision > revision && materials[1].image_index == 3 &&
          materials[1].clock.speed == -0.5f && r.model(fixture.instances[0]).animation_time == -2,
          "initial texture reset clears track/list/clock, restores resource fields, retains palette, skips null attachment");
    check(bool(r.bind_materials(fixture.instances[0], next)) && bool(r.step_model(fixture.instances[0])) &&
          materials[0].image_index == 0 && materials[0].color_and_trailing_words[0] == 0,
          "rebinding after texture reset starts fresh track values/rates instead of retained pre-reset tracks");
    check(bool(r.reset_materials(fixture.instances[0])) && materials[0].resource == 0 && materials[1].resource == 1 &&
          materials[0].palette_index == 3 && materials[1].clock.phase == a::Phase::disabled &&
          materials[1].clock.speed == 1 && r.model(fixture.instances[0]).materials.data() == materials,
          "default texture reset restores original attachment resources at stable addresses");
    check(bool(r.reset()) && materials[0].palette_index == 0 && materials[0].clock.speed == 0.5f &&
          bool(r.step_model(fixture.instances[0])) && materials[0].image_index == 2 && materials[1].image_index == 2,
          "scene reset restores original texture bindings and new-instance palette state");
}

void explicit_pool_reuse_and_retirement() {
    Stream model;
    model.cmd(15,7,0); model.cmd(16,1,0); model.cmd(11,16,0,{f(2)});
    model.cmd(3,16,2,{f(6)}); model.cmd(0,0,0);
    Stream retained;
    retained.cmd(4,16,0,{f(20)}); retained.cmd(2,0,4); retained.cmd(0,0,0);
    Stream texture;
    texture.cmd(11,1|512,0,{f(2),f(3)}); texture.cmd(2,0,4); texture.cmd(0,0,0);
    std::array<std::byte,0x78> material_bytes{};
    Fixture fixture;
    fixture.animations = {model.view(m::AnimationKind::model,0x9000),
                          retained.view(m::AnimationKind::model,0x9100),
                          texture.view(m::AnimationKind::texture,0x9200)};
    fixture.materials.resize(1);
    fixture.materials[0].wire = {0x9300,0,material_bytes};
    fixture.materials[0].parameters[0] = 9;
    fixture.models[0].nodes.resize(3,fixture.models[0].nodes[0]);
    fixture.models[0].nodes[1].parent = 0;
    fixture.models[0].nodes[2].parent = 0;
    fixture.models[0].nodes[0].materials = {0,0};
    fixture.models[0].nodes[0].material_animations = {2,2};
    fixture.models[0].receives_effect_callback = false;
    auto small = fixture.models[0];
    small.animation_speed = 1.5f;
    small.nodes.resize(1);
    small.nodes[0].animation = 1;
    small.nodes[0].position.x = 123;
    small.nodes[0].materials.clear();
    small.nodes[0].material_animations.clear();
    fixture.models.push_back(std::move(small));
    auto oversized = fixture.models[0];
    oversized.nodes.push_back(oversized.nodes.back());
    fixture.models.push_back(std::move(oversized));
    const std::array<a::SlotCapacity,4> capacities{{{3,2},{3,2},{3,1},{0,0}}};
    const a::Resources resources{fixture.models,fixture.materials,fixture.animations};
    auto made = a::Runtime::create(resources,capacities,{},{});
    auto foreign = a::Runtime::create(resources,capacities,{},{});
    check(bool(made) && bool(foreign),"fixed pool creation with explicit capacities");
    if (!made || !foreign) return;
    auto& r = *made.runtime;
    check(r.slot_count() == 4 && resources.models.size() == 3,"instance slot plan is independent of immutable model count");
    const auto first = r.acquire(0,0,true), second = r.acquire(1,0,false);
    const auto other_pool = foreign.runtime->acquire(0,0,true);
    check(bool(first) && bool(second) && bool(other_pool),"two instances share one immutable resource");
    if (!first || !second || !other_pool) return;
    const auto h = first.handle, peer = second.handle;
    const auto* nodes = r.model(h).nodes.data();
    const auto* materials = r.model(h).materials.data();
    const std::array<std::size_t,3> nulls{m::no_index,m::no_index,m::no_index};
    const std::array<std::size_t,3> switched{1,1,1};
    bool hot_ok = true;
    allocation_probe::count = 0;
    allocation_probe::enabled = true;
    hot_ok &= bool(r.step_model(h)) && r.events().size() == 3;
    const auto first_event = r.events().empty() ? a::CallbackEvent{} : r.events().front();
    hot_ok &= first_event.instance == h && first_event.node == 0 && first_event.source_order == 0;
    hot_ok &= bool(r.set_model_speed(h,-0.5f)) && bool(r.set_texture_speed(h,-1));
    hot_ok &= bool(r.set_object_flags(h,3)) && bool(r.bind_model(h,nulls));
    hot_ok &= r.model(h).nodes[0].flags == 7 && r.model(h).nodes[0].clock.speed == -0.5f;
    hot_ok &= bool(r.reset_materials(h)) && r.model(h).materials[0].palette_index == 3;
    hot_ok &= r.model(peer).nodes[0].transform.position.x == 99 &&
              r.model(peer).nodes[0].clock.speed == 0.5f && r.model(peer).object_flags == 0 &&
              r.model(peer).materials[0].palette_index == 0;
    hot_ok &= bool(r.step_model(peer)) && r.events().empty();
    hot_ok &= bool(r.set_callback_enabled(peer,true)) && bool(r.restart_model(peer));
    hot_ok &= bool(r.step_model(peer)) && r.events().size() == 3 && r.events()[0].instance == peer;
    const auto generation = r.generation();
    const auto event_count = r.events().size();
    const auto event_order = r.events().empty() ? UINT64_MAX : r.events().front().source_order;
    hot_ok &= r.acquire(0,1,true).status.error == a::Error::occupied;
    hot_ok &= r.acquire(2,0,true).status.error == a::Error::capacity;
    hot_ok &= r.acquire(2,99,true).status.error == a::Error::extent;
    hot_ok &= r.acquire(3,1,true).status.error == a::Error::capacity &&
              r.acquire(4,1,true).status.error == a::Error::extent;
    hot_ok &= r.step_model(other_pool.handle).error == a::Error::handle && !r.model(other_pool.handle);
    auto wrong_incarnation = h; ++wrong_incarnation.incarnation;
    hot_ok &= r.bind_model(wrong_incarnation,switched).error == a::Error::handle &&
              r.set_callback_enabled(wrong_incarnation,false).error == a::Error::handle;
    hot_ok &= r.generation() == generation && r.events().size() == event_count && !r.events().empty() &&
              r.events().front().source_order == event_order && r.model(h).object_flags == 3;
    hot_ok &= r.release_retired(h).error == a::Error::handle && r.generation() == generation &&
              r.events().size() == event_count;
    hot_ok &= bool(r.begin_retire(h)) && !r.model(h) &&
              r.step_model(h).error == a::Error::handle && nodes[0].flags == 7;
    hot_ok &= bool(r.reset()) && !r.model(h) && nodes[0].flags == 7 &&
              nodes[0].clock.speed == -0.5f && materials[0].palette_index == 3;
    hot_ok &= r.acquire(0,1,false).status.error == a::Error::occupied && nodes[0].flags == 7;
    // This synthetic pure-animator owner has no native, graphics or effect borrows.
    hot_ok &= bool(r.release_retired(h));
    const auto before_capacity_error = r.generation();
    hot_ok &= r.acquire(0,2,true).status.error == a::Error::capacity &&
              r.generation() == before_capacity_error;
    const auto small_instance = r.acquire(0,1,false);
    if (!small_instance) {
        allocation_probe::enabled = false;
        check(false,"small resource reacquisition after explicit owner release");
        return;
    }
    const auto small_handle = small_instance.handle;
    const auto small_view = r.model(small_handle);
    hot_ok &= small_handle.incarnation == h.incarnation+1 && small_view.resource_model == 1 &&
              small_view.instance == small_handle && small_view.nodes.data() == nodes &&
              small_view.nodes.size() == 1 && small_view.materials.empty() &&
              small_view.object_flags == 0 && small_view.animation_time == 0 &&
              small_view.nodes[0].revision == 1 &&
              small_view.nodes[0].transform.position.x == 123 &&
              small_view.nodes[0].flags == 0 && small_view.nodes[0].first_material == 0 &&
              small_view.nodes[0].material_count == 0 && small_view.nodes[0].clock.speed == 1.5f &&
              small_view.nodes[0].clock.time_passed == 0 && small_view.nodes[0].clock.time_left == 0 &&
              small_view.nodes[0].clock.phase == a::Phase::changed && small_view.nodes[0].callback_receiver;
    hot_ok &= !r.model(h) && r.restart_model(h).error == a::Error::handle;
    hot_ok &= bool(r.step_model(small_handle)) && r.events().empty() &&
              r.model(small_handle).nodes[0].transform.position.x == 0;
    hot_ok &= bool(r.begin_retire(small_handle)) && bool(r.release_retired(small_handle));
    const auto large_again = r.acquire(0,0,true);
    if (!large_again) {
        allocation_probe::enabled = false;
        check(false,"large resource reacquisition after explicit owner release");
        return;
    }
    const auto fresh = r.model(large_again.handle);
    hot_ok &= fresh.nodes.data() == nodes && fresh.materials.data() == materials &&
              fresh.nodes.size() == 3 && fresh.materials.size() == 2 &&
              fresh.resource_model == 0 && fresh.object_flags == 0 && fresh.animation_time == 0 &&
              fresh.nodes[0].revision == 1 && fresh.nodes[2].revision == 1 && fresh.materials[0].revision == 1 &&
              fresh.nodes[0].transform.position.x == 99 && fresh.nodes[2].parent == 0 &&
              fresh.nodes[2].flags == 0 && fresh.nodes[2].clock.speed == 0.5f &&
              fresh.nodes[0].callback_receiver && !fresh.nodes[2].callback_receiver &&
              fresh.nodes[0].first_material == 0 && fresh.nodes[0].material_count == 2 &&
              fresh.nodes[2].first_material == 2 && fresh.nodes[2].material_count == 0 &&
              fresh.materials[1].palette_index == 0 && fresh.materials[1].image_index == 0 &&
              fresh.materials[1].clock.speed == 0.5f && fresh.materials[1].revision == 1 &&
              fresh.materials[0].resource == 0 && fresh.materials[0].image_index == 0 &&
              fresh.materials[0].next_image_index == 0 && fresh.materials[0].palette_index == 0 &&
              fresh.materials[0].parameters[0] == 9 && fresh.materials[0].clock.speed == 0.5f &&
              fresh.materials[0].clock.phase == a::Phase::changed;
    hot_ok &= bool(r.step_model(large_again.handle)) && r.events().size() == 3 &&
              r.events()[0].instance == large_again.handle && fresh.nodes[0].transform.position.x == 2 &&
              fresh.materials[0].palette_index == 3 && fresh.materials[0].image_index == 2;
    hot_ok &= !r.model(small_handle) && r.release_retired(h).error == a::Error::handle;
    hot_ok &= bool(r.reset()) && r.events().empty() &&
              r.model(large_again.handle).nodes[0].transform.position.x == 99 &&
              r.model(large_again.handle).materials[0].palette_index == 0;
    hot_ok &= bool(r.step_model(peer)) && r.events().size() == 3 && r.events()[0].instance == peer;
    allocation_probe::enabled = false;
    const auto allocations = allocation_probe::count;
    check(hot_ok,"instance isolation, atomic rejection, fresh large-small-large state and owner-fenced incarnation reuse");
    check(allocations == 0,"acquire/bind/step/reset/retire/release/reuse perform no heap allocation");
    // All borrows above have ended; no native/graphics/effect owners exist in
    // this fixture. A destroyed pool's identity must never name a later owner.
    r.retire();
    made.runtime.reset();
    auto replacement = a::Runtime::create(resources,capacities,{},{});
    check(bool(replacement),"replacement pool creation");
    if (!replacement) return;
    const auto replacement_instance = replacement.runtime->acquire(0,0,true);
    check(bool(replacement_instance) && replacement_instance.handle.pool != h.pool &&
          !replacement.runtime->model(h) &&
          replacement.runtime->step_model(h).error == a::Error::handle,
          "destroy/recreate must not recycle an Impl-address identity into a stale handle");
}

void loop_timers_tracks_receivers_and_reuse() {
    // The Y track is outside the cycle. SET_ANIMATION must not clear it, even
    // while the cycle's zero-duration X command retains its old linear rate.
    Stream stream;
    stream.cmd(11,16|32,0,{f(0),f(0)});
    stream.cmd(4,32,2,{f(4)});
    stream.cmd(3,16,1,{f(2)});
    stream.cmd(16,0x107,1);
    stream.cmd(4,16,0,{f(6)});
    stream.cmd(2,0,1);
    stream.cmd(14,0,0,{0xa01c}); // callback16, after the initial 28 bytes.
    Fixture fixture; fixture.models[0].receives_effect_callback = true;
    fixture.models[0].nodes.resize(2,fixture.models[0].nodes[0]);
    fixture.models[0].nodes[1].parent = 0;
    fixture.models.push_back(fixture.models[0]);
    fixture.animations = {stream.view(m::AnimationKind::model,0xa000)};
    auto made = fixture.create(m::no_index,{14,5}); // Entire two-node prefix, richest callback boundary.
    check(bool(made),"prefix plus positive interior SET_ANIMATION cycle admitted"); if (!made) return;
    auto& r = *made.runtime;
    const auto h = fixture.instances[0], peer = fixture.instances[1];
    const auto* nodes = r.model(h).nodes.data();
    bool ok = true;
    allocation_probe::count = 0; allocation_probe::enabled = true;
    for (unsigned frame = 0; frame <= 6; ++frame) ok &= bool(r.step_model(h));
    // At time3: receiver emits -2 then callback16; node1 emits callback16
    // without receiving -2. Y is still the unmodified linear track at time3.
    ok &= r.events().size() == 3 && nodes[0].transform.position.y == 6 &&
          nodes[1].transform.position.y == 6 && nodes[0].transform.position.x == 4 &&
          nodes[0].clock.time_passed == 0 &&
          nodes[0].clock.time_left == 1 && r.model(h).animation_time == 0 &&
          nodes[0].callback_receiver && !nodes[1].callback_receiver;
    if (r.events().size() == 3) {
        ok &= r.events()[0].event_id == -2 && r.events()[0].node == 0 &&
              r.events()[0].command_address == 0xa02c &&
              r.events()[1].event_id == 1 && r.events()[1].argument == 7 &&
              r.events()[2].event_id == 1 && r.events()[2].node == 1 &&
              r.events()[1].source_order == r.events()[0].source_order+1 &&
              r.events()[2].source_order == r.events()[1].source_order+1;
    }
    ok &= bool(r.set_model_speed(h,-0.5f)) && bool(r.step_model(h)) &&
          nodes[0].clock.time_passed == -0.5f && nodes[0].clock.time_left == 1.5f &&
          nodes[0].transform.position.y == 5 && nodes[0].transform.position.x == 3 && r.events().empty();
    ok &= r.model(peer).nodes[0].transform.position.y == 7 &&
          r.model(peer).nodes[0].clock.phase == a::Phase::changed &&
          bool(r.step_model(peer)) && r.model(peer).nodes[0].transform.position.y == 0;
    const std::array<std::size_t,2> nulls{m::no_index,m::no_index}, bindings{0,0};
    ok &= bool(r.bind_model(h,nulls)) && bool(r.step_model(h)) &&
          nodes[0].transform.position.y == 5 && !nodes[0].callback_receiver;
    ok &= bool(r.bind_model(h,bindings)) && nodes[0].callback_receiver &&
          bool(r.set_model_speed(h,0.5f)) && bool(r.step_model(h)) &&
          nodes[0].transform.position.y == 0;
    ok &= bool(r.begin_retire(h)) && bool(r.release_retired(h));
    const auto fresh = r.acquire(0,0,false);
    ok &= bool(fresh) && fresh.handle != h && r.model(fresh.handle).nodes.data() == nodes &&
          nodes[0].clock.time_passed == 0 && nodes[0].transform.position.y == 7 &&
          !r.model(h) && bool(r.step_model(fresh.handle)) && r.events().empty();
    allocation_probe::enabled = false;
    check(ok,"loop clocks/retained tracks/source -2 receiver/order/signed speed/null binding and fresh slot reuse");
    check(allocation_probe::count == 0,"cyclic hot updates/binds/reuse allocate nothing");
}

void loop_overshoot_and_atomic_capacity() {
    Stream stream;
    stream.cmd(16,0x103,0); stream.cmd(3,16,2,{f(4)}); stream.cmd(14,0,0,{0xb000});
    auto configure = [&](Fixture& fixture) {
        fixture.models[0].receives_effect_callback = true;
        fixture.animations = {stream.view(m::AnimationKind::model,0xb000)};
    };
    Fixture fixture; configure(fixture);
    auto made = fixture.create(m::no_index,{12,8}); // Exact speed7 maximum: four cycle crossings.
    check(bool(made),"explicit multi-loop command/event envelope admitted"); if (!made) return;
    auto& r = *made.runtime; const auto h = fixture.instances[0];
    allocation_probe::count = 0; allocation_probe::enabled = true;
    const bool overshoot_ok = bool(r.step_model(h)) && r.events().size() == 1 &&
        bool(r.set_model_speed(h,7)) && bool(r.step_model(h)) &&
        r.events().size() == 6 && r.model(h).nodes[0].clock.time_left == 1 &&
        r.model(h).nodes[0].clock.time_passed == 1 && r.model(h).animation_time == 1 &&
        r.model(h).nodes[0].transform.position.x == 4;
    allocation_probe::enabled = false;
    check(overshoot_ok && allocation_probe::count == 0,
          "one allocation-free speed7 step crosses three source loops, retaining targets and exact residual clocks");
    if (r.events().size() == 6) for (std::size_t i = 0; i < 6; ++i)
        check(r.events()[i].event_id == (i%2 == 0 ? -2 : 1) &&
              r.events()[i].source_order == i+1,"overshoot emits every -2 and typed callback in source order");
    allocation_probe::count = 0; allocation_probe::enabled = true;
    const bool repeated_ok = bool(r.step_model(h)) && r.events().size() == 8 &&
        r.model(h).nodes[0].clock.time_left == 2 && r.model(h).animation_time == 0 &&
        bool(r.step_model(h)) && r.events().size() == 6 &&
        r.model(h).nodes[0].clock.time_left == 1 && r.model(h).animation_time == 1;
    allocation_probe::enabled = false;
    check(repeated_ok && allocation_probe::count == 0,
          "repeated speed7 overshoots alternate four/three complete cycles without truncation, drift or allocation");
    check(bool(r.restart_model(h,7)) && bool(r.step_model(h)) &&
          r.events().size() == 7 && r.events()[0].event_id == 1 &&
          r.model(h).nodes[0].clock.time_left == 1 && r.model(h).animation_time == 1,
          "nonzero start skip uses eleven commands/seven events, not modulo restart or event dropping");
    check(bool(r.set_model_speed(h,0.25f)) && bool(r.step_model(h)) &&
          r.model(h).animation_time == 1.25f && bool(r.set_model_speed(h,-0.5f)) &&
          bool(r.step_model(h)) && r.model(h).nodes[0].clock.time_passed == 0.75f &&
          r.model(h).nodes[0].clock.time_left == 1.25f && r.events().empty(),
          "fractional overshoot and subsequent signed speed preserve binary32 residual timing");
    for (const auto capacity : {a::ExecutionCapacity{8,7},a::ExecutionCapacity{11,5}}) {
        Fixture bounded; configure(bounded);
        auto limited = bounded.create(m::no_index,capacity);
        check(bool(limited),"bounded cyclic instance admitted"); if (!limited) continue;
        auto& runtime = *limited.runtime; const auto handle = bounded.instances[0];
        check(bool(runtime.step_model(handle)) && bool(runtime.set_model_speed(handle,7)),
              "bounded cycle initial publication");
        const auto before = runtime.model(handle);
        const auto clock = before.nodes[0].clock;
        const auto transform = before.nodes[0].transform;
        const auto revision = before.nodes[0].revision, generation = runtime.generation();
        const auto event = runtime.events()[0];
        const auto* event_data = runtime.events().data();
        allocation_probe::count = 0; allocation_probe::enabled = true;
        const auto status = runtime.step_model(handle);
        allocation_probe::enabled = false;
        check(status.error == a::Error::capacity && runtime.generation() == generation &&
              runtime.events().size() == 1 && runtime.events().data() == event_data &&
              runtime.events()[0].source_order == event.source_order &&
              before.nodes[0].clock.time_left == clock.time_left &&
              before.nodes[0].clock.time_passed == clock.time_passed &&
              before.nodes[0].clock.phase == clock.phase &&
              before.nodes[0].transform.position.x == transform.position.x &&
              before.nodes[0].revision == revision && runtime.model(handle).animation_time == before.animation_time &&
              allocation_probe::count == 0,
              "command/event bound rejection is allocation-free and preserves clocks/pose/revision/generation/borrowed events");
        check(bool(runtime.set_model_speed(handle,1)) && bool(runtime.step_model(handle)) &&
              runtime.model(handle).nodes[0].clock.time_left == 1 &&
              runtime.model(handle).nodes[0].transform.position.x == 2,
              "rejected preflight did not advance hidden cursor/tracks");
    }
    Fixture large; configure(large);
    auto large_runtime = large.create(m::no_index,{11,7});
    check(bool(large_runtime),"large finite-input boundary admitted");
    if (large_runtime) {
        auto& runtime = *large_runtime.runtime; const auto handle = large.instances[0];
        check(bool(runtime.restart_model(handle,std::numeric_limits<float>::max())),
              "finite-large skip accepted as binding metadata");
        const auto generation = runtime.generation();
        check(runtime.step_model(handle).error == a::Error::capacity && runtime.generation() == generation &&
              runtime.model(handle).nodes[0].clock.phase == a::Phase::changed && runtime.events().empty(),
              "binary32 cycle stagnation rejects finite-large skip before first mutation");
        check(bool(runtime.restart_model(handle)) && bool(runtime.step_model(handle)) &&
              bool(runtime.set_model_speed(handle,std::numeric_limits<float>::max())),
              "finite-large speed accepted as source speed metadata");
        const auto event_order = runtime.events()[0].source_order;
        const auto published = runtime.generation();
        check(runtime.step_model(handle).error == a::Error::capacity &&
              runtime.generation() == published && runtime.events().size() == 1 &&
              runtime.events()[0].source_order == event_order &&
              runtime.model(handle).nodes[0].clock.time_left == 2,
              "finite-large speed stagnation preserves prior event borrow and committed state");
    }
    Fixture two; configure(two);
    two.models[0].nodes.resize(2,two.models[0].nodes[0]); two.models[0].nodes[1].parent = 0;
    auto together = two.create(m::no_index,{17,12});
    check(bool(together),"two-node shared budget admitted");
    if (together) {
        auto& runtime = *together.runtime; const auto handle = two.instances[0];
        check(bool(runtime.step_model(handle)) && bool(runtime.set_model_speed(handle,7)),"two-node initial state");
        const auto generation = runtime.generation();
        check(runtime.step_model(handle).error == a::Error::capacity && runtime.generation() == generation &&
              runtime.events().size() == 2 && runtime.model(handle).nodes[0].clock.time_left == 2 &&
              runtime.model(handle).nodes[1].clock.time_left == 2,
              "late-node overflow rejects the entire model, not just the failing player");
    }
    Fixture automatic; configure(automatic);
    const auto finite_only = automatic.create();
    const auto mixed = automatic.create(m::no_index,{11,m::no_index});
    check(!finite_only && finite_only.status.error == a::Error::capacity &&
          !mixed && mixed.status.error == a::Error::malformed,
          "cyclic automatic finite certificate and mixed default/explicit capacity reject at construction");
}

void material_loop_isolation_and_aggregate_work() {
    Stream model; model.cmd(16,1,0); model.cmd(2,0,2); model.cmd(0,0,0);
    Stream material; material.cmd(3,2,2,{f(4)}); material.cmd(14,0,0,{0xc100});
    std::vector<std::byte> material_bytes(0x78);
    Fixture fixture;
    fixture.models[0].receives_effect_callback = true;
    fixture.models[0].nodes[0].materials = {0};
    fixture.models[0].nodes[0].material_animations = {1};
    fixture.materials.resize(1); fixture.materials[0].wire = {0xc200,0,material_bytes};
    fixture.animations = {model.view(m::AnimationKind::model,0xc000),
                          material.view(m::AnimationKind::texture,0xc100)};
    auto made = fixture.create(m::no_index,{7,1}); // END1 + three (14,LAST) pairs.
    check(bool(made),"material loop and finite model admitted together"); if (!made) return;
    auto& runtime = *made.runtime; const auto handle = fixture.instances[0];
    check(bool(runtime.step_model(handle)) && runtime.events().size() == 1 &&
          bool(runtime.set_model_speed(handle,7)) && bool(runtime.set_texture_speed(handle,7)) &&
          bool(runtime.step_model(handle)) && runtime.events().size() == 1 &&
          runtime.events()[0].event_id == -1 && runtime.events()[0].command_address == 0xc008 &&
          runtime.model(handle).animation_time == 7 &&
          runtime.model(handle).materials[0].clock.time_passed == 1 &&
          runtime.model(handle).materials[0].clock.time_left == 1 &&
          runtime.model(handle).materials[0].parameters[0] == 4,
          "material SET_ANIMATION resets only its own clock and never emits model -2 or resets object animation_time");
    check(bool(runtime.restart_model(handle)) && bool(runtime.set_model_speed(handle,0.5f)) &&
          bool(runtime.set_texture_speed(handle,0.5f)) && bool(runtime.step_model(handle)),
          "aggregate-work rejection setup retains a prior model callback");
    check(bool(runtime.set_model_speed(handle,7)) && bool(runtime.set_texture_speed(handle,9)),
          "aggregate-work rejection source speeds");
    const auto generation = runtime.generation();
    check(runtime.step_model(handle).error == a::Error::capacity && runtime.generation() == generation &&
          runtime.events().size() == 1 && runtime.events()[0].event_id == 0 &&
          runtime.model(handle).nodes[0].clock.phase == a::Phase::running &&
          runtime.model(handle).nodes[0].clock.time_left == 2 &&
          runtime.model(handle).materials[0].clock.time_left == 2,
          "late material overflow rejects model END and material writes atomically with one total instruction budget");
    check(bool(runtime.set_callback_enabled(handle,false)) &&
          bool(runtime.set_model_speed(handle,0.5f)) && bool(runtime.set_texture_speed(handle,0.5f)),
          "silent callbacks preserve source schedule");
    const std::array<std::size_t,1> null_binding{m::no_index};
    check(bool(runtime.bind_model(handle,null_binding)) && bool(runtime.bind_materials(handle,null_binding)) &&
          bool(runtime.step_model(handle)) && runtime.events().empty(),
          "null model disables independently while null material binding retains its cyclic player");
    Fixture silent;
    silent.models[0].nodes[0].animation = m::no_index;
    silent.models[0].nodes[0].materials = {0};
    silent.models[0].nodes[0].material_animations = {0};
    silent.materials = fixture.materials;
    silent.animations = {material.view(m::AnimationKind::texture,0xc100)};
    auto no_events = silent.create(m::no_index,{2,0});
    check(bool(no_events) && bool(no_events.runtime->step_model(silent.instances[0])) &&
          bool(no_events.runtime->set_texture_speed(silent.instances[0],2)) &&
          bool(no_events.runtime->step_model(silent.instances[0])) && no_events.runtime->events().empty(),
          "explicit zero event storage is valid for callback-free material loops");
    Fixture zero;
    zero.animations = {model.view(m::AnimationKind::model,0xc000)};
    auto none = zero.create(m::no_index,{0,0});
    check(bool(none),"explicit zero command/event budget is not the automatic finite certificate");
    if (none) {
        auto& r = *none.runtime; const auto h = zero.instances[0];
        const auto zero_generation = r.generation();
        check(r.step_model(h).error == a::Error::capacity && r.generation() == zero_generation &&
              r.model(h).nodes[0].clock.phase == a::Phase::changed &&
              bool(r.bind_model(h,null_binding)) && bool(r.step_model(h)) && r.events().empty(),
              "zero budget atomically rejects active work but permits genuinely disabled zero-work calls");
    }
}

void loop_grammar_rejects_unpublished() {
    auto rejects = [](const Stream& stream, m::AnimationKind kind, a::Error error) {
        Fixture fixture;
        fixture.models[0].nodes[0].animation = m::no_index;
        fixture.animations = {stream.view(kind,0x9000)};
        const auto made = fixture.create();
        return !made && !made.runtime && made.status.error == error;
    };
    Stream foreign; foreign.cmd(2,0,1); foreign.cmd(14,0,0,{0x9100});
    Stream operand; operand.cmd(3,16,1,{f(2)}); operand.cmd(14,0,0,{0x9004});
    Stream unaligned; unaligned.cmd(2,0,1); unaligned.cmd(14,0,0,{0x9001});
    Stream no_operand; no_operand.cmd(2,0,1); no_operand.cmd(14,0,0);
    Stream zero; zero.cmd(4,16,12,{f(2)}); zero.cmd(12,16,12); zero.cmd(14,0,0,{0x9000});
    Stream prefix_only; prefix_only.cmd(2,0,1); prefix_only.cmd(14,0,0,{0x9004});
    Stream trailing; trailing.cmd(2,0,1); trailing.cmd(14,0,0,{0x9000}); trailing.cmd(0,0,0);
    Stream camera; camera.cmd(2,0,1); camera.cmd(14,0,0,{0x9000});
    Stream missing_end; missing_end.cmd(2,0,1);
    check(rejects(foreign,m::AnimationKind::model,a::Error::malformed) &&
          rejects(operand,m::AnimationKind::model,a::Error::malformed) &&
          rejects(unaligned,m::AnimationKind::model,a::Error::malformed) &&
          rejects(no_operand,m::AnimationKind::model,a::Error::malformed),
          "SET_ANIMATION raw target must name a same-program instruction boundary with exact operand framing");
    check(rejects(zero,m::AnimationKind::model,a::Error::malformed) &&
          rejects(prefix_only,m::AnimationKind::model,a::Error::malformed),
          "non-LAST duration and track-time shifts do not certify positive cycle wait");
    check(rejects(trailing,m::AnimationKind::model,a::Error::malformed) &&
          rejects(missing_end,m::AnimationKind::model,a::Error::malformed) &&
          rejects(camera,m::AnimationKind::camera,a::Error::unsupported),
          "single contiguous terminal grammar remains strict and does not invent camera graph admission");
    Stream raw; raw.cmd(2,0,1); raw.cmd(14,0,0,{0xff800000});
    Fixture fixture; fixture.animations = {raw.view(m::AnimationKind::model,0xff800000)};
    auto made = fixture.create(m::no_index,{2,1});
    check(bool(made),"raw SET_ANIMATION address is not reinterpreted as a nonfinite float");
}

void prepared_acquisition_is_private_and_cancelable() {
    Stream stream;
    stream.cmd(16,0x105,2); stream.cmd(0,0,0);
    Fixture fixture;
    fixture.animations = {stream.view(m::AnimationKind::model,0x7800)};
    const auto capacity = a::slot_capacity(fixture.models[0]);
    const std::array<a::SlotCapacity,3> capacities{capacity,capacity,a::SlotCapacity{}};
    auto made = a::Runtime::create({fixture.models,fixture.materials,fixture.animations},capacities,
                                   fixture.initial_camera,{});
    check(bool(made),"prepared acquisition fixture admitted"); if (!made) return;
    auto& runtime = *made.runtime;
    const auto first = runtime.acquire(0,0,true);
    check(bool(first) && bool(runtime.step_model(first.handle)) && runtime.events().size() == 1,
          "active peer emits a real borrowed callback before admission");
    if (!first || runtime.events().size() != 1) return;
    const auto peer = runtime.model(first.handle);
    const auto node = peer.nodes[0];
    const auto event = runtime.events()[0];
    const auto* events = runtime.events().data();
    const auto generation = runtime.generation();
    const auto unchanged = [&] {
        return runtime.generation() == generation && runtime.events().data() == events &&
            runtime.events().size() == 1 && same_event(runtime.events()[0],event) &&
            same_node(runtime.model(first.handle).nodes[0],node);
    };
    auto unpublished = first.handle; unpublished.slot = 1;
    check(runtime.prepare_acquire(0,0).error == a::Error::occupied &&
          runtime.prepare_acquire(2,0).error == a::Error::capacity &&
          runtime.prepare_acquire(3,0).error == a::Error::extent &&
          runtime.prepare_acquire(1,1).error == a::Error::extent && unchanged(),
          "rejected admission preserves active state, callback credit and generation");
    check(bool(runtime.prepare_acquire(1,0,true)) && !runtime.model(unpublished) &&
          runtime.acquire(1,0,false).status.error == a::Error::occupied &&
          runtime.prepare_acquire(1,0).error == a::Error::occupied && unchanged(),
          "reserved slot is neither publicly fetchable nor independently acquirable");
    allocation_probe::count = 0;
    allocation_probe::enabled = true;
    bool cancelled = bool(runtime.cancel_prepared(1));
    for (unsigned i = 0; i < 32; ++i)
        cancelled &= bool(runtime.prepare_acquire(1,0,true)) && bool(runtime.cancel_prepared(1));
    allocation_probe::enabled = false;
    check(cancelled && allocation_probe::count == 0 && unchanged() &&
          runtime.cancel_prepared(0).error == a::Error::handle &&
          runtime.cancel_prepared(1).error == a::Error::handle,
          "repeated preparation cancellation allocates nothing and never consumes active peer events");
    check(bool(runtime.prepare_acquire(1,0,true)),"reserve slot again for actual consumption");
    const auto acquired = runtime.acquire_prepared(1);
    check(bool(acquired) && acquired.handle.incarnation == 1 && runtime.events().empty() &&
          runtime.generation() == generation+1 && runtime.model(acquired.handle).nodes[0].revision == 1,
          "only consumption publishes one fresh incarnation and normal acquisition event boundary");
    if (!acquired) return;
    const auto active_generation = runtime.generation();
    check(runtime.cancel_prepared(1).error == a::Error::handle &&
          runtime.acquire_prepared(1).status.error == a::Error::handle &&
          runtime.generation() == active_generation && bool(runtime.model(acquired.handle)) &&
          bool(runtime.step_model(acquired.handle)) && runtime.events().size() == 1 &&
          runtime.events()[0].instance == acquired.handle &&
          runtime.events()[0].source_order == event.source_order+1,
          "double consumption/cancellation cannot retire active ownership or reset callback source order");
    check(bool(runtime.begin_retire(acquired.handle)) &&
          runtime.cancel_prepared(1).error == a::Error::handle &&
          bool(runtime.release_retired(acquired.handle)) &&
          bool(runtime.prepare_acquire(1,0,true)) && bool(runtime.cancel_prepared(1)) &&
          bool(runtime.prepare_acquire(1,0,true)),
          "retiring storage is not a reservation; actual release restores admission");
    const auto reused = runtime.acquire_prepared(1);
    check(bool(reused) && reused.handle.incarnation == 2 && !runtime.model(acquired.handle),
          "cancelled admission never spends an incarnation or revives an old handle");
    if (!reused) return;
    check(bool(runtime.begin_retire(reused.handle)) && bool(runtime.release_retired(reused.handle)) &&
          bool(runtime.prepare_acquire(1,0)),"prepare outstanding slot before whole-runtime retirement");
    runtime.retire();
    const auto retired_generation = runtime.generation();
    check(runtime.acquire_prepared(1).status.error == a::Error::retired &&
          bool(runtime.cancel_prepared(1)) && runtime.generation() == retired_generation &&
          runtime.events().empty(),
          "whole-runtime retirement still permits private cancellation without scene recovery");
}

void malformed_streams_reject_unpublished() {
    Stream valid; valid.cmd(2,0,1); valid.cmd(0,0,0);
    Stream jump; jump.cmd(1,0,0,{0x12345678}); jump.cmd(0,0,0);
    Fixture fixture; fixture.animations = {valid.view(m::AnimationKind::model,0x7000),jump.view(m::AnimationKind::model,0x7100)};
    auto rejected = fixture.create(); check(!rejected && !rejected.runtime && rejected.status.error == a::Error::unsupported,"late invalid stream rejects entire runtime, never publishes valid prefix");
    fixture.animations = {valid.view(m::AnimationKind::model,0x7000)};
    fixture.animations[0].commands[0].duration = 2;
    rejected = fixture.create(); check(!rejected && rejected.status.error == a::Error::malformed,"metadata/wire disagreement rejected before execution");
}
} // namespace
int main() {
    persistent_linear_and_end(); cubic_wait_flags_and_zero_duration(); callbacks_receivers_and_atomic_errors();
    materials_and_camera(); rates_and_source_restart(); model_binding_switches_and_instances();
    texture_track_time_offset();
    local_pose_authority_and_waits(); local_pose_animation_overwrite_and_null_binding();
    local_pose_admission_atomicity_and_retirement();
    switched_callbacks_and_atomicity(); material_binding_null_and_initial_reset(); malformed_streams_reject_unpublished();
    explicit_pool_reuse_and_retirement();
    prepared_acquisition_is_private_and_cancelable();
    loop_timers_tracks_receivers_and_reuse();
    loop_grammar_rejects_unpublished();
    loop_overshoot_and_atomic_capacity(); material_loop_isolation_and_aggregate_work();
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
