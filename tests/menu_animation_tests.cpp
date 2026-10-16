#include "menu_animation.hpp"

#include <bit>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>

namespace m = isle::menu;
namespace a = isle::menu::animation;
namespace {
int failures = 0;
void check(bool condition, std::string_view meaning) {
    if (!condition) { ++failures; std::cerr << "FAIL: " << meaning << '\n'; }
}
std::uint32_t f(float value) { return std::bit_cast<std::uint32_t>(value); }
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
    a::CameraState initial_camera{{0,0,100},{0,0,0},0,45,true,true,0,{}};
    Fixture() {
        models[0].nodes.resize(1);
        models[0].nodes[0].position = {99,7,8};
        models[0].nodes[0].scale = {1,1,1};
        models[0].nodes[0].animation = 0;
    }
    a::CreateResult create(std::size_t camera = m::no_index) {
        return a::Runtime::create({models,materials,animations,camera},initial_camera);
    }
};
void persistent_linear_and_end() {
    Stream s;
    s.cmd(11,16,0,{f(2)}); s.cmd(3,16,2,{f(6)}); s.cmd(0,0,0);
    Fixture fixture;
    fixture.animations.push_back(s.view(m::AnimationKind::model,0x1000));
    auto made = fixture.create(); check(bool(made),"linear stream admitted"); if (!made) return;
    auto& r = *made.runtime;
    const auto* stable = r.model(0).nodes.data();
    for (unsigned frame = 0; frame <= 4; ++frame) {
        check(bool(r.step_model(0)),"persistent update succeeds");
        check(r.model(0).nodes[0].transform.position.x == 2.0f+static_cast<float>(frame),"time0 then consecutive binary32 half-frame linear progress");
        check(r.model(0).nodes.data() == stable,"frame storage never reallocates");
    }
    check(r.model(0).nodes[0].clock.phase == a::Phase::disabled,"END applies final value then disables");
    const auto revision = stable[0].revision;
    check(bool(r.step_model(0)) && stable[0].transform.position.x == 6 && stable[0].revision == revision,"disabled track neither advances nor dirties");
    check(bool(r.reset()) && stable[0].transform.position.x == 99,"fresh reentry restores bind pose rather than frozen pose0");
    check(bool(r.step_model(0)) && stable[0].transform.position.x == 2,"reset restarts source time0");
    check(bool(r.reset(0.5f)) && bool(r.step_model(0)) && stable[0].transform.position.x == 3,"skipFrames applies source negative timeLeft before update");
}
void cubic_wait_flags_and_zero_duration() {
    Stream s;
    s.cmd(2,0,1); s.cmd(15,3,0); s.cmd(11,16,0,{f(2)});
    s.cmd(8,16,2,{f(6)}); s.cmd(0,0,0);
    Fixture fixture; fixture.animations.push_back(s.view(m::AnimationKind::model,0x2000));
    auto made = fixture.create(); check(bool(made),"wait/cubic stream admitted"); if (!made) return;
    auto& r = *made.runtime;
    check(bool(r.step_model(0)) && r.model(0).nodes[0].transform.position.x == 99,"WAIT does not overwrite bind fields");
    check(bool(r.step_model(0)) && r.model(0).nodes[0].flags == 0,"WAIT positive timeLeft avoids parsing next flags");
    check(bool(r.step_model(0)) && r.model(0).nodes[0].transform.position.x == 2 && r.model(0).nodes[0].flags == 3,"exact wait boundary executes zero-duration commands in source order");
    check(bool(r.step_model(0)) && r.model(0).nodes[0].transform.position.x == 2.625f,"source grouped cubic at t=.5");
    check(bool(r.set_object_flags(0,2)) && bool(r.step_model(0)) && r.model(0).nodes[0].transform.position.x == 2.625f,"GOBJ_FLAG_2 advances time while suppressing model writes");
    check(bool(r.set_object_flags(0,0)) && bool(r.step_model(0)) && r.model(0).nodes[0].transform.position.x == 5.375f,"unsuppress publishes advanced track, not paused pose");

    Stream z; z.cmd(11,16,0,{f(2)}); z.cmd(3,16,2,{f(6)});
    z.cmd(4,16,0,{f(10)}); z.cmd(2,0,1); z.cmd(0,0,0);
    Fixture other; other.animations.push_back(z.view(m::AnimationKind::model,0x2800));
    auto zero = other.create(); check(bool(zero),"zero-duration linear stream admitted"); if (!zero) return;
    for (unsigned i = 0; i < 5; ++i) check(bool(zero.runtime->step_model(0)),"zero-duration boundary update");
    check(zero.runtime->model(0).nodes[0].transform.position.x == 6,"duration zero preserves previous rate, not snap-to-target fallback");
    check(bool(zero.runtime->step_model(0)) && zero.runtime->model(0).nodes[0].transform.position.x == 7,"retained rate progresses after zero-duration command");
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
    check(!r.model(0).nodes[0].callback_receiver && r.model(0).nodes[1].callback_receiver && !r.model(0).nodes[2].callback_receiver,"receiver is first non-null animated node, not root");
    check(bool(r.step_model(0)) && r.events().size() == 4,"no callback coalescing across nodes");
    if (r.events().size() == 4) {
        check(r.events()[0].node == 1 && r.events()[1].node == 1 && r.events()[2].node == 2 && r.events()[3].node == 2,"native depth-first callback node ordering");
        check(r.events()[0].event_id == 0 && r.events()[0].argument == 1 && r.events()[1].argument == 4,"opcode16 high bits eventID and low byte float argument");
        check(r.events()[0].command_address == 0x3000 && r.events()[3].source_order == 3,"callback provenance and exact global source order");
    }
    const auto generation = r.generation();
    const auto* events = r.events().data();
    check(r.step_model(99).error == a::Error::extent && r.generation() == generation && r.events().data() == events && r.events().size() == 4,"invalid update fails before any frame/event publication");
    check(r.reset(std::numeric_limits<float>::quiet_NaN()).error == a::Error::malformed && r.generation() == generation,"invalid reset is failure atomic");
    check(bool(r.step_model(0)) && r.events().empty(),"callbacks are not replayed by resampling");
    check(bool(r.step_model(0)) && r.events().size() == 1 && r.events()[0].event_id == -1 && r.events()[0].node == 1,"only designated receiver emits END callback");
    made.runtime.reset(); // End the immutable borrowed resource lease before editing fixture.
    fixture.models[0].receives_effect_callback = false;
    auto silent = fixture.create(); check(bool(silent),"sourceNULL callback graph admitted");
    if (silent) check(bool(silent.runtime->step_model(0)) && silent.runtime->events().empty(),"sourceNULL opcode16 consumed without dispatch");
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
    check(r.model(0).materials[0].parameters[4] == 2 && r.model(0).materials[0].parameters[5] == 4,"MObj initial source14/scaleS copied to24/28");
    check(r.model(0).materials[0].lod_level == 128.0f/255.0f,"lod seed is byte54, not packed word54");
    check(bool(r.step_model(0)) && r.model(0).materials[0].color_and_trailing_words[0] == 0x10203040,"color target uses raw RGBA word, not float numeric conversion");
    check(bool(r.step_model(0)) && r.model(0).materials[0].color_and_trailing_words[0] == 0x30405060,"source8.8 color interpolation changes consecutive frame");
    check(bool(r.step_camera()) && r.camera().fov_y == 40,"camera first update time0");
    check(bool(r.step_camera()) && r.camera().fov_y == 45 && !r.camera().view_dirty && r.camera().projection_dirty,"FOV-only change invalidates projection, not kind8 fixedYup view");
    check(r.camera().eye.z == 100 && r.camera().written_channels == (1u<<9),"unwritten camera channels preserve caller initial fields");
    check(bool(r.step_camera(2)) && r.camera().fov_y == 45 && !r.camera().projection_dirty,"camera object flag2 advances without publishing");
    check(bool(r.step_camera()) && r.camera().fov_y == 55,"camera resumes from actual elapsed clock");
    r.retire(); check(r.step_model(0).error == a::Error::retired && r.step_camera().error == a::Error::retired && r.events().empty(),"retirement stops all publication");
}
void rates_and_source_restart() {
    Stream s;
    s.cmd(11,16,0,{f(2)}); s.cmd(7,16,0,{f(2)});
    s.cmd(5,16,2,{f(6),f(0)}); s.cmd(0,0,0);
    Fixture fixture; fixture.animations.push_back(s.view(m::AnimationKind::model,0x6800));
    auto made = fixture.create(); check(bool(made),"explicit target-rate cubic stream admitted"); if (!made) return;
    auto& r = *made.runtime;
    check(bool(r.step_model(0)) && bool(r.step_model(0)) &&
          r.model(0).nodes[0].transform.position.x == 3.1875f,"cubic inherits previous target-rate as initial tangent");
    const auto preserved = r.model(0).nodes[0].transform.position.x;
    check(bool(r.restart_model(0)) && r.model(0).nodes[0].clock.phase == a::Phase::changed &&
          r.model(0).nodes[0].transform.position.x == preserved,"source animSet keeps fields until next evaluation");
    check(bool(r.step_model(0)) && r.model(0).nodes[0].transform.position.x == 2,"source restart processes time0 with retained allocated tracks");
    check(bool(r.reset()) && r.model(0).nodes[0].transform.position.x == 99,"fresh reset differs from source restart field preservation");
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
    const auto* nodes = r.model(0).nodes.data();
    check(bool(r.step_model(0)) && bool(r.step_model(0)) && nodes[1].transform.position.x == 3,
          "original model advances before switching");
    const auto revision = nodes[1].revision;
    const std::array<std::size_t,2> next{m::no_index,1};
    check(bool(r.bind_model(0,next,0.5f,a::ModelBindPose::restore)) &&
          nodes[0].clock.phase == a::Phase::disabled && !nodes[0].callback_receiver &&
          nodes[0].clock.time_passed == 0.5f && nodes[0].flags == 7 &&
          nodes[1].callback_receiver && nodes[1].clock.phase == a::Phase::changed &&
          nodes[1].transform.position.x == 99 && nodes[1].revision > revision && r.events().empty(),
          "model null disables retaining clock/flags; bind helper restores pose and reselects receiver without evaluation");
    check(bool(r.step_model(0)) && nodes[1].transform.position.x == 7 &&
          nodes[1].clock.time_passed == 0.5f && nodes[0].transform.position.x == 99,
          "different stream inherits previous target and zero-duration rate, applies skip, leaves disabled node alone");
    check(bool(r.set_model_speed(0,-0.5f)) && bool(r.step_model(0)) &&
          nodes[1].transform.position.x == 6 && nodes[1].clock.time_passed == 0,
          "signed model speed seeks the newly bound stream backwards");
    check(r.model(1).nodes[1].transform.position.x == 99 && r.model(1).nodes[1].clock.speed == 0.5f &&
          bool(r.step_model(1)) && r.model(1).nodes[1].transform.position.x == 2 &&
          nodes[1].transform.position.x == 6,"same immutable programs have independent instance bindings, tracks and speed");
    const std::array<std::size_t,2> retained{1,m::no_index};
    check(bool(r.bind_model(0,retained,-0.5f)) && nodes[0].transform.position.x == 99 &&
          nodes[1].transform.position.x == 6 && bool(r.step_model(0)) &&
          nodes[0].transform.position.x == 5 && nodes[0].clock.time_passed == -0.5f,
          "preserve-pose bind keeps fields until evaluation; negative skip retains source track timing");
    check(r.model(0).nodes.data() == nodes && bool(r.reset()) &&
          nodes[0].clock.speed == 0.5f && nodes[1].transform.position.x == 99 &&
          nodes[0].callback_receiver && !nodes[1].callback_receiver &&
          bool(r.step_model(0)) && nodes[1].transform.position.x == 2,
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
    check(bool(r.bind_model(0,binding,2)) && r.events().empty() && bool(r.step_model(0)) &&
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
    const auto revision = r.model(0).nodes[0].revision;
    const auto time = r.model(0).animation_time;
    const std::array<std::size_t,3> bad_kind{0,0,2}, bad_index{0,0,999};
    const std::array<std::size_t,1> bad_extent{0};
    check(r.bind_model(0,bad_kind,0,a::ModelBindPose::restore).error == a::Error::malformed &&
          r.bind_model(0,bad_index).error == a::Error::malformed &&
          r.bind_model(0,bad_extent).error == a::Error::extent &&
          r.bind_model(0,binding,std::numeric_limits<float>::infinity()).error == a::Error::malformed &&
          r.bind_model(99,binding).error == a::Error::extent &&
          r.generation() == generation && r.events().data() == emitted.data() && r.events().size() == 7 &&
          r.events()[3].event_id == -1 && r.model(0).nodes[0].revision == revision &&
          r.model(0).nodes[1].callback_receiver && r.model(0).animation_time == time,
          "late invalid program/type/extent/skip reject atomically, preserving event and frame publication");
    check(bool(r.step_model(0)) && r.events().empty(),"switched END is not replayed on next update");
    r.retire();
    check(r.bind_model(0,binding).error == a::Error::retired &&
          r.bind_materials(0,{}).error == a::Error::retired &&
          r.reset_materials(0).error == a::Error::retired && r.model(0).nodes.empty() && r.events().empty(),
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
    const auto* materials = r.model(0).materials.data();
    check(bool(r.step_model(0)) && bool(r.step_model(0)) &&
          materials[0].image_index == 3 && materials[0].palette_index == 3,"original texture tracks advance");
    const std::array<std::size_t,2> next{1,m::no_index};
    check(bool(r.bind_materials(0,next,0.5f)) && materials[0].image_index == 3 &&
          materials[0].clock.phase == a::Phase::changed && materials[1].clock.phase == a::Phase::running &&
          materials[1].clock.time_passed == 0.5f && r.model(0).animation_time == 0.5f &&
          bool(r.step_model(0)) && materials[0].image_index == 7 && materials[1].image_index == 4 &&
          materials[0].color_and_trailing_words[0] == 0x60708090 && materials[0].palette_index == 3,
          "new texture stream retains scalar rate/color targets/inverse duration; null entry continues old stream");
    check(bool(r.set_texture_speed(0,-0.5f)) && bool(r.step_model(0)) &&
          materials[0].image_index == 6 && materials[1].image_index == 3,
          "signed texture speed acts independently on switched and retained programs");
    check(r.model(1).materials[0].image_index == 0 && r.model(1).materials[0].clock.speed == 0.5f &&
          bool(r.step_model(1)) && r.model(1).materials[0].image_index == 2 &&
          materials[0].image_index == 6 && materials[0].clock.speed == -0.5f,
          "shared texture resources/programs do not share switched bindings, tracks or signed clocks between instances");
    const auto time = materials[0].clock.time_passed;
    check(bool(r.bind_materials(0,{},-2)) && materials[0].clock.time_passed == time &&
          materials[0].clock.phase == a::Phase::running && r.model(0).animation_time == -2,
          "null outer texture array changes only object animation time, not material clocks/bindings");
    const auto generation = r.generation(), revision = materials[0].revision;
    const std::array<std::size_t,2> bad_kind{1,2}, bad_resource{2,999};
    const std::array<std::size_t,1> bad_extent{1};
    check(r.bind_materials(0,bad_kind).error == a::Error::malformed &&
          r.bind_materials(0,bad_extent).error == a::Error::extent &&
          r.bind_materials(0,next,std::numeric_limits<float>::quiet_NaN()).error == a::Error::malformed &&
          r.reset_materials(0,bad_resource).error == a::Error::extent &&
          r.reset_materials(0,bad_extent).error == a::Error::extent &&
          r.generation() == generation && materials[0].revision == revision &&
          materials[0].image_index == 6 && materials[0].clock.time_passed == time &&
          r.model(0).animation_time == -2,"late texture/reset rejection cannot publish a valid prefix");
    const std::array<std::size_t,2> reset_one{2,m::no_index};
    check(bool(r.reset_materials(0,reset_one)) && materials[0].resource == 2 &&
          materials[0].parameters[0] == 99 && materials[0].parameters[4] == 99 &&
          materials[0].parameters[5] == 4 && materials[0].palette_index == 3 &&
          materials[0].lod_level == 128.0f/255.0f && materials[0].image_index == 0 &&
          materials[0].next_image_index == 0 && materials[0].clock.phase == a::Phase::disabled &&
          materials[0].clock.time_passed == 0 && materials[0].clock.speed == 1 &&
          materials[0].revision > revision && materials[1].image_index == 3 &&
          materials[1].clock.speed == -0.5f && r.model(0).animation_time == -2,
          "initial texture reset clears track/list/clock, restores resource fields, retains palette, skips null attachment");
    check(bool(r.bind_materials(0,next)) && bool(r.step_model(0)) &&
          materials[0].image_index == 0 && materials[0].color_and_trailing_words[0] == 0,
          "rebinding after texture reset starts fresh track values/rates instead of retained pre-reset tracks");
    check(bool(r.reset_materials(0)) && materials[0].resource == 0 && materials[1].resource == 1 &&
          materials[0].palette_index == 3 && materials[1].clock.phase == a::Phase::disabled &&
          materials[1].clock.speed == 1 && r.model(0).materials.data() == materials,
          "default texture reset restores original attachment resources at stable addresses");
    check(bool(r.reset()) && materials[0].palette_index == 0 && materials[0].clock.speed == 0.5f &&
          bool(r.step_model(0)) && materials[0].image_index == 2 && materials[1].image_index == 2,
          "scene reset restores original texture bindings and new-instance palette state");
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
    switched_callbacks_and_atomicity(); material_binding_null_and_initial_reset(); malformed_streams_reject_unpublished();
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
