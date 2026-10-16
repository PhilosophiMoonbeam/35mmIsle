#include "menu_animation.hpp"

#include <bit>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

// Opt-in private-input smoke. No ROM, decompressed bank, extracted stream or
// expected pose table is embedded. Caller supplies one admitted original ROM.
// This program is source-only in the worker handoff; parent owns execution.
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
void run(const char* path) {
    const auto snapshot = isle::rom::read_snapshot(path);
    auto catalog = m::hydrate(snapshot);
    require(bool(catalog),"private menu resource hydration failed");
    const a::CameraState initial_camera{{0,0,100},{0,0,0},0,45,true,true,0,{}};
    std::vector<a::SlotCapacity> capacities;
    for (const auto& model : catalog.catalog->models()) capacities.push_back(a::slot_capacity(model));
    auto made = a::Runtime::create(*catalog.catalog,capacities,initial_camera);
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
    bool prior_transform_changed = false, consecutive_transform_changes = false;
    a::CameraState prior_camera = runtime.camera();
    a::Transform replay_pose{};
    a::CameraState replay_camera{};
    for (unsigned frame = 0; frame <= 3721; ++frame) {
        bool frame_transform_changed = false;
        // Deliberately explicit caller schedule; production uses actual OM order.
        // No runtime-wide implicit catalog-order tick is provided.
        for (std::size_t i = 0; i < runtime.slot_count(); ++i) {
            require(bool(runtime.step_model(instances[i])),"actual persistent model update failed");
            const auto state = runtime.model(instances[i]);
            for (std::size_t n = 0; n < state.nodes.size(); ++n) {
                const auto now = state.nodes[n].transform;
                const auto old = prior[i][n];
                if (different(now.position,old.position) || different(now.rotation,old.rotation) || different(now.scale,old.scale)) {
                    ++transform_changes; frame_transform_changed = true;
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
        consecutive_transform_changes |= frame_transform_changed && prior_transform_changed;
        prior_transform_changed = frame_transform_changed;
    }
    require(event_index == std::size(expected),"actual installed callback events missing");
    require(transform_changes > 292 && consecutive_transform_changes && material_changes > 10 && camera_changes > 10,
            "actual consecutive model/material/camera progression absent");
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
    std::cout << "menu animation private ROM: 292 nodes, 116 streams; " << transform_changes << " transform, "
              << material_changes << " material, " << camera_changes << " camera changes; 4 ordered installed callbacks; reentry reproduced\n";
}
} // namespace
int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "one admitted private ROM argument required\n"; return EXIT_FAILURE; }
    try { run(argv[1]); return EXIT_SUCCESS; }
    catch (const std::exception& e) { std::cerr << e.what() << '\n'; return EXIT_FAILURE; }
}
