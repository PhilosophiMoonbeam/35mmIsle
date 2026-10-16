// Explicit private-ROM, CPU Vulkan canonical RSP->RDP proof. No fixture, Original
// collector, extracted public payload, or SDK producer implementation is used.
#include "menu_model_producer.hpp"
#include "graphics_task.hpp"
#include "rsp_render.hpp"
#include <algorithm>
#include <array>
#include <exception>
#include <filesystem>
#include <iostream>
#include <span>
#include <stdexcept>
#include <vector>

namespace {
using namespace isle;
void require(bool ok, const char* text) { if (!ok) throw std::runtime_error(text); }
void native(IsleGameResult result) { require(result == ISLE_GAME_OK,"native scene operation rejected"); }
constexpr std::uint32_t top = 0x100000, identity = 0x10ff00, viewport = 0x10ff80;
constexpr std::uint32_t stack = 0x140000, yield = 0x144000, task = 0x146000, boot = 0x147000;
constexpr std::uint32_t color = 0x3b5000, depth = 0x600000, guard = 0x7f0000;
constexpr std::size_t target_bytes = 320 * 240 * 2;
struct World {
    std::vector<std::byte> storage = std::vector<std::byte>(2 * 1024 * 1024);
    isle_arena arena{};
    IsleGame* game = nullptr;
    IsleGameCamera* camera = nullptr;
    std::array<IsleGameObject*,5> objects{};
    World() {
        require(isle_arena_init(&arena,storage.data(),storage.size()) == ISLE_OK,"arena initialization");
        native(isle_game_create(&arena,&game));
        IsleGameObject* camera_object = nullptr;
        native(isle_game_add_object(game,1,0,0,nullptr,nullptr,&camera_object));
        native(isle_game_attach_camera(game,camera_object,&camera));
        native(isle_game_camera_add_matrix(game,camera,ISLE_GAME_CAMERA_PERSPECTIVE_FAST));
        native(isle_game_camera_add_matrix(game,camera,ISLE_GAME_CAMERA_LOOK_AT_ROLL));
        const IsleGamePerspective perspective{30,4.0f / 3.0f,47,12800,1};
        native(isle_game_camera_set_perspective(game,camera,&perspective));
        for (auto& object : objects)
            native(isle_game_add_object(game,0xe,0,0x80000000,nullptr,nullptr,&object));
    }
    ~World() { if (game && isle_game_destroy(game) != ISLE_GAME_OK) std::terminate(); }
};
void camera_pose(World& world, const menu::animation::CameraState& state) {
    const IsleGameCameraParameters* old = nullptr;
    native(isle_game_camera_parameters(world.game,world.camera,&old));
    auto perspective = old->perspective; perspective.fovy = state.fov_y;
    const IsleGameLookAt view{{state.eye.x,state.eye.y,state.eye.z},
        {state.at.x,state.at.y,state.at.z},{0,1,0}};
    native(isle_game_camera_set_perspective(world.game,world.camera,&perspective));
    native(isle_game_camera_set_look_at(world.game,world.camera,&view));
    native(isle_game_camera_set_roll(world.game,world.camera,state.roll));
}
void top_list(std::span<std::uint32_t> words, const menu::Catalog& catalog,
              const menu::model::Frame& frame) {
    graphics::Memory memory(words);
    std::uint32_t cursor = top;
    auto cmd = [&](std::uint32_t a, std::uint32_t b) {
        require(cursor + 8 <= identity,"top command capacity");
        memory.word(cursor,a); memory.word(cursor + 4,b); cursor += 8;
    };
    const IsleGameFixedMatrix one{{{0x10000,0,1,0},{0,0x10000,0,1},{0,0,0,0},{0,0,0,0}}};
    memory.matrix(identity,one);
    const std::array<std::int16_t,8> vp{640,480,511,0,640,480,511,0};
    for (unsigned i = 0; i < vp.size(); ++i) memory.half(viewport + i * 2,std::uint16_t(vp[i]));
    cmd(0xdb060000,0); cmd(0xdb06003c,color);
    cmd(0xe7000000,0); cmd(0xed000000,(1280u << 12) | 960u);
    cmd(0xef300000,0); cmd(0xff10013f,depth); cmd(0xf7000000,0xfffcfffc);
    cmd(0xf64fc3bc,0); cmd(0xe7000000,0);
    cmd(0xff10013f,color); cmd(0xf7000000,0x00010001); cmd(0xf64fc3bc,0);
    cmd(0xe7000000,0); cmd(0xfe000000,depth);
    cmd(0xef080cf0,0x00552078); cmd(0xd9000000,0x00a20405);
    cmd(0xdb040004,1); cmd(0xdb04000c,1); cmd(0xdb040014,0xffff); cmd(0xdb04001c,0xffff);
    cmd(0xd7000000,0); cmd(0xfcffffff,0xfffe793c);
    cmd(0xdc080008,viewport);
    cmd(0xde000000,catalog.pre_render(menu::Scene::title_menu).display_list.guest_address);
    for (const auto lane : frame.lanes) {
        cmd(0xda380003,identity); cmd(0xde000000,frame.camera.address);
        cmd(0xde000000,lane.address);
    }
    cmd(0xe9000000,0); cmd(0xdf000000,0);
}
struct Capture {
    std::vector<std::uint32_t> triangles;
    std::vector<std::byte> color, depth;
};
Capture capture(const rsp::CanonicalExecutor::MemoryLease& lease) {
    Capture result;
    const auto& packets = lease.packets();
    for (const auto packet : packets.packets()) {
        const auto words = packets.words().subspan(packet.offset,packet.count);
        const auto opcode = (words[0] >> 24) & 63;
        if (opcode >= 8 && opcode <= 15)
            result.triangles.insert(result.triangles.end(),words.begin(),words.end());
    }
    auto copy_target = [&](std::uint32_t address, std::vector<std::byte>& output) {
        output.resize(target_bytes);
        for (std::size_t i = 0; i < target_bytes; ++i) output[i] = lease.rdram()[(address + i) ^ 3];
    };
    copy_target(color,result.color); copy_target(depth,result.depth);
    return result;
}
int run(const std::filesystem::path& input) {
    const auto snapshot = rom::read_snapshot(input);
    auto hydrated = menu::hydrate(snapshot);
    require(bool(hydrated),"private resource hydration rejected");
    auto& catalog = *hydrated.catalog;
    World world;
    auto animated = menu::animation::Runtime::create(
        catalog, {{1500,0,0},{0,0,0},0,30,true,true,0,{}});
    require(bool(animated),"animation creation rejected");
    auto& animation = *animated.runtime;
    const std::array<menu::RamRange,5> reserved{{{top,0x10000},{stack,0x10000},
        {0x200000,0x40000},{depth,target_bytes},{guard,256}}};
    {
        auto tiny_layout = menu::model::Layout{};
        tiny_layout.lists.size = 64;
        auto tiny = menu::model::Producer::create(world.game,world.camera,catalog,animation,
            world.objects,tiny_layout,reserved);
        require(bool(tiny),"bounded producer construction rejected before capacity exercise");
        const IsleGameCameraMatrix* matrix = nullptr;
        native(isle_game_camera_matrix(world.game,world.camera,0,&matrix));
        const auto revision = matrix->revision;
        const IsleGamePerspective changed{31,4.0f / 3.0f,47,12800,1};
        native(isle_game_camera_set_perspective(world.game,world.camera,&changed));
        std::vector<std::uint32_t> ram(menu::rdram_size / 4,0x11223344);
        const auto rejected = tiny.producer->prepare(ram,1);
        require(!rejected && rejected.status.error == menu::model::Error::extent &&
            matrix->revision == revision && tiny.producer->world_nodes(0).empty() &&
            std::all_of(ram.begin(),ram.end(),[](auto v) { return v == 0x11223344; }),
            "capacity exhaustion published RAM/native camera/world state");
        const IsleGamePerspective restored{30,4.0f / 3.0f,47,12800,1};
        native(isle_game_camera_set_perspective(world.game,world.camera,&restored));
    }
    auto created = menu::model::Producer::create(world.game,world.camera,catalog,animation,world.objects,{},reserved);
    require(bool(created),"model producer creation rejected");
    auto& producer = *created.producer;
    rsp::CanonicalExecutor executor(true,{12000000,4 * 1024 * 1024});
    const auto arena_used = world.arena.cursor;
    auto step = [&] {
        for (std::size_t m = 0; m < 5; ++m) require(bool(animation.step_model(m)),"model animation rejected");
        require(bool(animation.step_camera()),"camera animation rejected");
        camera_pose(world,animation.camera());
    };
    auto execute = [&](bool cold, bool expect_all_hidden = false) {
        auto preparation = executor.begin_preparation();
        auto words = preparation.words();
        if (cold) {
            graphics::validate_cold_inputs(words,snapshot,snapshot.canonical().data(),boot);
            const auto plan = catalog.plan_install(menu::Scene::title_menu,reserved);
            require(bool(plan.status),"private smoke resource collision");
            graphics::install_cold_inputs(words,snapshot,boot);
            require(bool(catalog.install(menu::Scene::title_menu,std::as_writable_bytes(words),reserved)),"resource install rejected");
            for (unsigned i = 0; i < 64; ++i) words[guard / 4 + i] = 0x13579bdf;
        }
        const auto before = std::vector<std::uint32_t>(words.begin(),words.end());
        const auto nodes_before = producer.native_nodes(0);
        const auto first_revision = nodes_before.empty() ? 0 : nodes_before[0].revision;
        const auto invalid = producer.prepare(words.first(words.size() - 1),1);
        require(!invalid && std::equal(words.begin(),words.end(),before.begin()),"invalid request changed RAM");
        if (!nodes_before.empty()) require(nodes_before[0].revision == first_revision,"invalid request changed native cache");
        const auto frame = producer.prepare(words,1);
        require(bool(frame),"native model prepare rejected");
        if (expect_all_hidden) for (const auto lane : frame.frame.lanes)
            require(lane.commands == 1 && words[lane.address / 4] == 0xdf000000 &&
                words[lane.address / 4 + 1] == 0,
                "hidden objects retained payload/fog commands in a published draw lane");
        // CPU writes must remain in exactly the producer's three owned ranges.
        for (std::size_t i = 0; i < words.size(); ++i) {
            const auto at = i * 4;
            const auto owned = std::any_of(frame.frame.reservations.begin(),frame.frame.reservations.end(),
                [&](menu::RamRange r) { return at >= r.offset && at < r.offset + r.size; });
            if (!owned) require(words[i] == before[i],"producer changed an unowned RAM word");
        }
        top_list(words,catalog,frame.frame);
        const auto prepared = graphics::finish_task(words,snapshot,boot,stack,yield,task,
            0x200000,0x240000,top);
        auto completed = executor.execute(std::move(preparation),prepared.task,prepared.boot);
        require(!executor.quiescent(),"result borrow did not retain execution ownership");
        bool rejected = false;
        try { auto forbidden = executor.begin_preparation(); static_cast<void>(forbidden); }
        catch (const std::exception&) { rejected = true; }
        require(rejected,"outstanding GPU result allowed RAM reuse");
        for (unsigned i = 0; i < 256; ++i)
            require(completed.rdram()[guard + i] == std::byte((i % 4 == 0) ? 0xdf :
                (i % 4 == 1) ? 0x9b : (i % 4 == 2) ? 0x57 : 0x13),"actual GPU changed out-of-scope guard");
        auto result = capture(completed);
        completed.release(); require(executor.quiescent(),"result release did not retire GPU borrow");
        require(world.arena.cursor == arena_used,"per-frame native allocation");
        return result;
    };
    step();
    const auto initial = execute(true);
    require(!initial.triangles.empty(),"actual initial model task emitted no geometry");
    require(std::any_of(initial.color.begin(),initial.color.end(),[](std::byte b) {
        return b != std::byte{0} && b != std::byte{1}; }),"actual initial RDP color remained clear");
    bool z_written = false;
    for (std::size_t i = 0; i < initial.depth.size(); ++i)
        z_written |= initial.depth[i] != std::byte((i & 1) ? 0xfc : 0xff);
    require(z_written,"actual initial RDP depth remained clear");
    const auto visible_generation = animation.generation();
    std::array<std::uint32_t,5> visible_flags{};
    for (std::size_t m = 0; m < visible_flags.size(); ++m) {
        visible_flags[m] = animation.model(m).object_flags;
        require(bool(animation.set_object_flags(m,visible_flags[m] | 1u)),"hide rejected");
    }
    require(animation.generation() == visible_generation,"visibility edit advanced animation");
    const auto hidden = execute(false,true); // Same RAM/epoch, no step/synchronize.
    require(hidden.triangles.empty(),"hidden objects still emitted actual RSP triangles");
    for (std::size_t i = 0; i < target_bytes; ++i)
        require(hidden.color[i] == std::byte((i & 1) ? 1 : 0) &&
            hidden.depth[i] == std::byte((i & 1) ? 0xfc : 0xff),
            "hidden model payloads still changed actual RDP color/depth");
    for (std::size_t m = 0; m < visible_flags.size(); ++m)
        require(bool(animation.set_object_flags(m,visible_flags[m])),"show rejected");
    require(animation.generation() == visible_generation,"visibility restore advanced animation");
    const auto shown = execute(false); // Flags alone must invalidate every cached lane.
    require(initial.triangles == shown.triangles && initial.color == shown.color && initial.depth == shown.depth,
        "show without animation step failed to restore actual geometry/color/depth");
    for (unsigned i = 0; i < 180; ++i) step();
    const auto advanced = execute(false);
    require(initial.triangles != advanced.triangles && initial.color != advanced.color && initial.depth != advanced.depth,
        "actual animated model task did not change geometry/color/depth");
    // Consume the gameplay binding APIs over real immutable ROM programs.
    // Keep the camera fixed: disabled model/material tracks must not move or
    // recolor a completed frame merely because the source clock is stepped.
    for (std::size_t m = 0; m < animation.model_count(); ++m) {
        const std::vector<std::size_t> disabled(catalog.models()[m].nodes.size(),menu::no_index);
        require(bool(animation.bind_model(m,disabled)),"disable real model bindings rejected");
        require(bool(animation.reset_materials(m)),"restore real material resources rejected");
    }
    const auto frozen = execute(false);
    for (unsigned frame = 0; frame < 180; ++frame)
        for (std::size_t m = 0; m < animation.model_count(); ++m)
            require(bool(animation.step_model(m)),"disabled real model step rejected");
    const auto still_frozen = execute(false);
    require(frozen.triangles == still_frozen.triangles && frozen.color == still_frozen.color &&
        frozen.depth == still_frozen.depth,"disabled real bindings changed completed geometry/color/depth");
    for (std::size_t m = 0; m < animation.model_count(); ++m) {
        const auto& source = catalog.models()[m];
        std::vector<std::size_t> models,materials;
        models.reserve(source.nodes.size());
        for (const auto& node : source.nodes) {
            models.push_back(node.animation);
            for (std::size_t k = 0; k < node.materials.size(); ++k)
                materials.push_back(k < node.material_animations.size() ?
                    node.material_animations[k] : menu::no_index);
        }
        require(bool(animation.bind_model(m,models,0,menu::animation::ModelBindPose::restore)),
                "real model rebind rejected");
        require(bool(animation.bind_materials(m,materials)),"real material rebind rejected");
        require(bool(animation.set_model_speed(m,source.animation_speed)) &&
                bool(animation.set_texture_speed(m,source.animation_speed)),"real rebound speed rejected");
        require(bool(animation.step_model(m)),"real rebound immediate evaluation rejected");
    }
    const auto rebound = execute(false);
    require(frozen.triangles != rebound.triangles && frozen.color != rebound.color &&
        frozen.depth != rebound.depth,"real rebound programs did not republish changed geometry/color/depth");
    require(bool(animation.reset()),"animation restore rejected"); step();
    const auto restored = execute(false);
    require(initial.triangles == restored.triangles && initial.color == restored.color && initial.depth == restored.depth,
        "restored actual animated task did not reproduce initial geometry/color/depth");
    producer.retire();
    std::vector<std::uint32_t> rejected_ram(menu::rdram_size / 4,0xabcddcba);
    require(!producer.prepare(rejected_ram,2) &&
        std::all_of(rejected_ram.begin(),rejected_ram.end(),[](auto v) { return v == 0xabcddcba; }),
        "retired scene published a new task");
    std::cout << "menu_models_rsp_rdp=passed models=5 nodes=292 frames=initial,advanced,frozen,rebound,restored "
        "bindings=disabled_stable_and_rebound geometry_color_depth=changed_and_restored "
        "visibility=hide_show_without_step guard=preserved leases=exclusive "
        "Original_scene_parity=not_claimed\n";
    return 0;
}
}
#ifdef _WIN32
int wmain(int argc, wchar_t** argv) {
#else
int main(int argc, char** argv) {
#endif
    try { require(argc == 2,"expected one explicit private ROM path"); return run(std::filesystem::path(argv[1])); }
    catch (const std::filesystem::filesystem_error&) { std::cerr << "private model smoke filesystem failure\n"; }
    catch (const std::exception& e) { std::cerr << "private model smoke: " << e.what() << '\n'; }
    return 1;
}
