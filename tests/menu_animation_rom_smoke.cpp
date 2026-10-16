#include "menu_animation.hpp"

#include <array>
#include <bit>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <utility>

// Opt-in private-input smoke. No ROM, decompressed bank, extracted stream or
// expected pose table is embedded. Caller supplies one admitted original ROM.
namespace m = isle::menu;
namespace a = isle::menu::animation;
namespace {
void require(bool ok, const char* meaning) { if (!ok) throw std::runtime_error(meaning); }
bool different(m::Vec3 x, m::Vec3 y) {
    return std::bit_cast<std::uint32_t>(x.x) != std::bit_cast<std::uint32_t>(y.x) ||
        std::bit_cast<std::uint32_t>(x.y) != std::bit_cast<std::uint32_t>(y.y) ||
        std::bit_cast<std::uint32_t>(x.z) != std::bit_cast<std::uint32_t>(y.z);
}
struct ExpectedEvent {
    unsigned frame;
    std::size_t model, node;
    std::int32_t id;
    float arg;
    std::uint32_t address;
};
// Independent metadata-only source timeline acquisition: command durations,
// receiver selection, callback installation. Not a private asset payload table.
constexpr ExpectedEvent expected[] = {
    {2574,2,4,0,1,0x80359cbc}, {2576,2,4,0,1,0x80359cc0},
    {3720,2,0,-1,0,0x80359c5c}, {3720,3,0,-1,0,0x8038fe08}
};
void actual_cyclic_wire(const isle::rom::Snapshot& snapshot) {
    // Narrow public-API witness: an authored one-node hierarchy borrowing the
    // actual initial Snorlax root stream, not actor model/Gfx/factory admission.
    // Exact initialized mapping: beach_assets ROMa74e0 -> guest8011b050.
    constexpr std::uint32_t entry = 0x8017ebdc;
    constexpr std::size_t offset = 0xa74e0 + (entry - 0x8011b050);
    const auto bytes = snapshot.canonical().subspan(offset,16);
    m::Animation program{{entry,offset,bytes},m::AnimationKind::model,true,{}};
    for (const auto [start,size] : {std::pair<std::size_t,std::size_t>{0,4},{4,4},{8,8}}) {
        const isle::rom::GuestView wire{entry+static_cast<std::uint32_t>(start),offset+start,
            bytes.subspan(start,size)};
        const auto word = wire.u32(0);
        program.commands.push_back({wire,static_cast<std::uint8_t>(word >> 25),
            static_cast<std::uint16_t>((word >> 15)&1023),static_cast<std::uint16_t>(word&32767)});
    }
    m::Model model;
    model.nodes.resize(1);
    model.nodes[0].scale = {1,1,1};
    model.nodes[0].animation = 0;
    const a::Resources resources{{&model,1},{},{&program,1}};
    const std::array<a::SlotCapacity,1> slots{{{1,0}}};
    auto made = a::Runtime::create(resources,slots,{}, {32,8});
    require(bool(made),"actual cyclic wire admission");
    auto& runtime = *made.runtime;
    const auto acquired = runtime.acquire(0,0,true);
    require(bool(acquired),"actual cyclic wire instance");
    const auto instance = acquired.handle;
    require(bool(runtime.step_model(instance)) && runtime.events().empty() &&
            runtime.model(instance).nodes[0].clock.time_left == 6,
            "actual cyclic wire initial WAIT schedule");
    require(bool(runtime.set_model_speed(instance,75)) && bool(runtime.step_model(instance)),
            "actual cyclic wire three-cycle overshoot");
    const auto state = runtime.model(instance);
    require(state.nodes[0].clock.time_passed == 0 && state.nodes[0].clock.time_left == 6 &&
            state.animation_time == 0 && runtime.events().size() == 6,
            "actual 25-frame cycle preserves overshoot and resets node/object clocks");
    for (std::size_t i = 0; i < 6; ++i) {
        const auto& event = runtime.events()[i];
        const bool completion = i%2 != 0;
        require(event.instance == instance && event.node == 0 && event.source_order == i &&
                event.event_id == (completion ? -2 : 0) &&
                event.argument == (completion ? 0 : 1) &&
                event.command_address == entry + (completion ? 8u : 4u),
                "actual callback and loop completion source order/identity/arguments");
    }
    auto bounded = a::Runtime::create(resources,slots,{}, {8,8});
    require(bool(bounded),"actual cyclic bounded consumer admission");
    auto& limited = *bounded.runtime;
    const auto small = limited.acquire(0,0,true);
    require(bool(small) && bool(limited.step_model(small.handle)) &&
            bool(limited.set_model_speed(small.handle,50)) && bool(limited.step_model(small.handle)) &&
            limited.events().size() == 4 && bool(limited.set_model_speed(small.handle,75)),
            "actual bounded consumer publishes a two-cycle event batch");
    const auto generation = limited.generation();
    const auto* events = limited.events().data();
    require(limited.step_model(small.handle).error == a::Error::capacity &&
            limited.generation() == generation && limited.events().data() == events &&
            limited.events().size() == 4 && limited.events().front().source_order == 0 &&
            limited.events().back().source_order == 3 &&
            limited.model(small.handle).nodes[0].clock.time_passed == 0 &&
            limited.model(small.handle).nodes[0].clock.time_left == 6 &&
            limited.model(small.handle).animation_time == 0,
            "actual late instruction-budget failure preserves clocks, generation and prior events");
    std::cout << "actual_cyclic_wire=passed source=initial_Snorlax_root period=25 "
                 "crossings=3 ordered_events=6 budget_rejection=atomic "
                 "actor_geometry_factory_admission=not_claimed\n";
}
void run(const char* path) {
    const auto snapshot = isle::rom::read_snapshot(path);
    auto catalog = m::hydrate(snapshot);
    require(bool(catalog),"private menu resource hydration failed");
    const a::CameraState initial_camera{{0,0,100},{0,0,0},0,45,true,true,0,{}};
    std::vector<a::SlotCapacity> capacities;
    for (const auto& model : catalog.catalog->models()) capacities.push_back(a::slot_capacity(model));
    auto made = a::Runtime::create(*catalog.catalog,capacities,initial_camera,{});
    require(bool(made),"actual menu animation graph rejected");
    auto& runtime = *made.runtime;
    std::vector<a::InstanceHandle> instances;
    for (std::size_t i = 0; i < capacities.size(); ++i) {
        const auto acquired = runtime.acquire(i,i,catalog.catalog->models()[i].receives_effect_callback);
        require(bool(acquired),"actual title instance acquisition");
        instances.push_back(acquired.handle);
    }
    require(runtime.slot_count() == 5 && catalog.catalog->animations().size() == 116,"full actual model/116-stream resource boundary");
    std::size_t nodes = 0, materials = 0;
    for (std::size_t i = 0; i < runtime.slot_count(); ++i) {
        nodes += runtime.model(instances[i]).nodes.size(); materials += runtime.model(instances[i]).materials.size();
    }
    require(nodes == 292 && materials == 10,"all292 nodes and10 material attachments required");
    std::vector<std::vector<a::Transform>> prior(runtime.slot_count());
    std::vector<std::vector<std::uint64_t>> material_revision(runtime.slot_count());
    for (std::size_t i = 0; i < runtime.slot_count(); ++i) {
        for (const auto& node : runtime.model(instances[i]).nodes) prior[i].push_back(node.transform);
        for (const auto& material : runtime.model(instances[i]).materials) material_revision[i].push_back(material.revision);
    }
    std::size_t event_index = 0, transform_changes = 0, material_changes = 0, camera_changes = 0;
    a::CameraState prior_camera = runtime.camera();
    a::Transform replay_pose{};
    a::CameraState replay_camera{};
    for (unsigned frame = 0; frame <= 3721; ++frame) {
        // Deliberately explicit caller schedule; production uses actual OM order.
        // No runtime-wide implicit catalog-order tick is provided.
        for (std::size_t i = 0; i < runtime.slot_count(); ++i) {
            require(bool(runtime.step_model(instances[i])),"actual persistent model update failed");
            const auto state = runtime.model(instances[i]);
            for (std::size_t n = 0; n < state.nodes.size(); ++n) {
                const auto now = state.nodes[n].transform;
                const auto old = prior[i][n];
                if (different(now.position,old.position) || different(now.rotation,old.rotation) || different(now.scale,old.scale)) {
                    ++transform_changes;
                }
                require(state.nodes[n].parent == m::no_index || state.nodes[n].parent < n,"actual native parent-before-child matrix composition ordering");
                prior[i][n] = now;
            }
            for (std::size_t j = 0; j < state.materials.size(); ++j) {
                if (state.materials[j].revision != material_revision[i][j]) ++material_changes;
                material_revision[i][j] = state.materials[j].revision;
            }
            for (const auto& event : runtime.events()) {
                require(event_index < std::size(expected),"unexpected real callback (including sourceNULL stream leakage)");
                const auto& e = expected[event_index];
                require(frame == e.frame && event.instance == instances[e.model] && event.node == e.node &&
                        event.event_id == e.id && event.argument == e.arg && event.command_address == e.address &&
                        event.source_order == event_index,"actual callback source order/time/receiver/encoding mismatch");
                ++event_index;
            }
        }
        require(bool(runtime.step_camera()),"actual persistent camera update failed");
        const auto& camera = runtime.camera();
        if (different(camera.eye,prior_camera.eye) || different(camera.at,prior_camera.at) ||
            camera.roll != prior_camera.roll || camera.fov_y != prior_camera.fov_y) ++camera_changes;
        prior_camera = camera;
        if (frame == 60) { replay_pose = runtime.model(instances[0]).nodes[1].transform; replay_camera = camera; }
    }
    require(event_index == std::size(expected),"actual installed callback events missing");
    require(bool(runtime.reset()),"actual fresh reentry reset failed");
    for (unsigned frame = 0; frame <= 60; ++frame) {
        for (std::size_t i = 0; i < runtime.slot_count(); ++i) require(bool(runtime.step_model(instances[i])),"reentry model update failed");
        require(bool(runtime.step_camera()),"reentry camera update failed");
    }
    const auto replayed = runtime.model(instances[0]).nodes[1].transform;
    require(!different(replayed.position,replay_pose.position) && !different(replayed.rotation,replay_pose.rotation) &&
            !different(replayed.scale,replay_pose.scale) && !different(runtime.camera().eye,replay_camera.eye) &&
            !different(runtime.camera().at,replay_camera.at) && runtime.camera().roll == replay_camera.roll &&
            runtime.camera().fov_y == replay_camera.fov_y,"actual reentry must reproduce persistent source frame60");
    actual_cyclic_wire(snapshot);
    std::cout << "menu animation private ROM: 292 nodes, 116 streams; " << transform_changes << " transform, "
              << material_changes << " material, " << camera_changes << " camera changes; 4 ordered installed callbacks; reentry reproduced\n";
}
} // namespace
int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "one admitted private ROM argument required\n"; return EXIT_FAILURE; }
    try { run(argv[1]); return EXIT_SUCCESS; }
    catch (const std::exception& e) { std::cerr << e.what() << '\n'; return EXIT_FAILURE; }
}
