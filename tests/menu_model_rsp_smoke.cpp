// Explicit private-ROM, CPU Vulkan canonical RSP->RDP proof. No fixture, Original
// collector, extracted public payload, or SDK producer implementation is used.
#include "menu_model_producer.hpp"
#include "contextual_model_math.h"
#include "graphics_task.hpp"
#include "rsp_render.hpp"
#include <algorithm>
#include <array>
#include <exception>
#include <cmath>
#include <cstring>
#include <limits>
#include <filesystem>
#include <iostream>
#include <span>
#include <stdexcept>
#include <vector>

extern "C" void hal_perspective_fast_f(float [4][4], std::uint16_t*, float, float, float, float, float);
extern "C" void hal_mtx_f2l(float [4][4], IsleGameFixedMatrix*);

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
void placement_world(const isle::menu::model::WorldMatrix& local,
                     const isle::menu::model::WorldMatrix& placed,
                     const IsleGameModelTransform& t, bool scaled) {
    // Independent row-vector quarter-turn oracle, not a fixed-matrix decode.
    const float c = std::cos(t.rotation.y), s = std::sin(t.rotation.y);
    const float x = scaled ? t.scale.x : 1, y = scaled ? t.scale.y : 1, z = scaled ? t.scale.z : 1;
    for (unsigned row = 0; row < 4; ++row) {
        const float w = local.m[row][3];
        const std::array<float,4> expected{
            (local.m[row][0] * x * c + local.m[row][2] * z * s) + w * t.position.x,
            local.m[row][1] * y + w * t.position.y,
            (-local.m[row][0] * x * s + local.m[row][2] * z * c) + w * t.position.z, w};
        for (unsigned col = 0; col < 4; ++col)
            require(std::abs(placed.m[row][col] - expected[col]) <=
                0.002f + std::abs(expected[col]) * 0.00001f,
                "attachment world is not local x placement (or child x placed parent)");
    }
}
// Predict matrix use from source visibility/payload semantics, independently of
// the emitted command stream and the producer's deferred-prefix traversal.
std::array<unsigned,292> live_matrix_lanes(const isle::menu::Model& source,
                                         const isle::menu::animation::ModelState& state) {
    std::array<unsigned,292> lanes{};
    std::array<bool,292> reachable{};
    require(state.nodes.size() == source.nodes.size() && state.nodes.size() <= lanes.size(),
        "live hierarchy exceeds source-index matrix-use oracle");
    if (state.object_flags & 1u) return lanes;
    const bool deferred = source.render_type == isle::menu::RenderType::d_fogged ||
        source.render_type == isle::menu::RenderType::j_fogged;
    for (std::size_t n = 0; n < state.nodes.size(); ++n) {
        const auto& node = state.nodes[n];
        require(node.parent == isle::menu::no_index || node.parent < n,
            "matrix-use oracle requires admitted parent-before-child source indices");
        reachable[n] = !(node.flags & 2u) &&
            (node.parent == isle::menu::no_index || reachable[node.parent]);
        if (!reachable[n]) continue;
        // B/I prepare immediately even on geometry-less or hide-self nodes.
        if (!deferred) lanes[n] = 1u;
        if (!deferred || (node.flags & 1u)) continue;
        for (const auto& payload : source.nodes[n].payloads) {
            if (!payload.draw_list) continue;
            require(payload.display_list_id < 4,"source payload selects an invalid lane");
            for (auto parent = n; parent != isle::menu::no_index; parent = state.nodes[parent].parent)
                lanes[parent] |= 1u << payload.display_list_id;
        }
    }
    return lanes;
}
void placement_lists(std::span<const std::uint32_t> words, const isle::menu::model::Frame& frame,
                     const isle::menu::Catalog& catalog,
                     const isle::menu::animation::Runtime& animation,
                     std::span<const menu::animation::InstanceHandle> handles, bool require_all_models = false) {
    // Consume the actual producer's flat lists. Payload calls remain opaque;
    // this checks logical modelview scope, not incidental command-word counts.
    constexpr std::uint32_t local_base = 0x110000 + 128;
    constexpr std::uint32_t placement_base = local_base + 292 * 64;
    std::array<std::array<unsigned,4>,5> uses{};
    std::array<std::uint32_t,5> roots{};
    auto root = local_base;
    for (std::size_t m = 0; m < roots.size(); ++m) {
        roots[m] = root;
        root += static_cast<std::uint32_t>(catalog.models()[m].nodes.size() * 64);
    }
    for (unsigned lane_id = 0; lane_id < frame.lanes.size(); ++lane_id) {
        const auto lane = frame.lanes[lane_id];
        unsigned stack_depth = 0;
        std::size_t pending_model = uses.size();
        for (std::size_t i = 0; i < lane.commands; ++i) {
            const auto a = words[lane.address / 4 + i * 2];
            const auto b = words[lane.address / 4 + i * 2 + 1];
            if ((a >> 24) == 0xda) {
                if (!(a & 1u)) ++stack_depth;
                if (b >= placement_base && b < placement_base + 5 * 64) {
                    require((b - placement_base) % 64 == 0,"placement payload address is not matrix-aligned");
                    const auto m = (b - placement_base) / 64;
                    ++uses[m][lane_id];
                    require(!(a & 1u), "placement did not perform the logical root PUSH");
                    pending_model = m;
                } else {
                    if (pending_model != uses.size()) {
                        require((a & 1u) && stack_depth == 1 && b == roots[pending_model],
                            "placement prefix did not prepare its own ordinary root in the same scope");
                        pending_model = uses.size();
                    }
                }
            }
            if ((a >> 24) == 0xd8) {
                require(stack_depth != 0, "deferred lane popped a scope it never flushed");
                --stack_depth;
            }
        }
        require(stack_depth == 0 && pending_model == uses.size(),
            "model leaked an incomplete root prefix or stack scope into its sibling object/lane");
    }
    for (std::size_t m = 0; m < uses.size(); ++m) {
        const auto expected = live_matrix_lanes(catalog.models()[m],animation.model(handles[m]))[0];
        require(!require_all_models || expected != 0,"all-visible source pose lost an actual model's matrix-use coverage");
        for (unsigned lane = 0; lane < frame.lanes.size(); ++lane)
            require(uses[m][lane] == ((expected >> lane) & 1u),
                "placement presence/count does not match the live model's actual used lanes");
    }
}
void kind44_inheritance(std::span<const std::uint32_t> words, const isle::menu::model::Frame& frame,
                        const isle::menu::Catalog& catalog, const isle::menu::animation::Runtime& animation,
                        std::span<const menu::animation::InstanceHandle> handles,
                        const IsleGamePerspective& perspective, float actor_x, bool require_all_nodes = false) {
    float projection[4][4];
    std::uint16_t ignored = 0;
    hal_perspective_fast_f(projection,&ignored,perspective.fovy,perspective.aspect,
        perspective.near_plane,perspective.far_plane,perspective.scale);
    std::uint32_t base = 0x110000 + 128;
    unsigned reached = 0;
    for (std::size_t m = 0; m < catalog.models().size(); ++m) {
        const auto poses = animation.model(handles[m]).nodes;
        const auto used = live_matrix_lanes(catalog.models()[m],animation.model(handles[m]));
        std::array<float,292> inherited_x{};
        for (std::size_t n = 0; n < poses.size(); ++n)
            inherited_x[n] = (poses[n].parent == isle::menu::no_index ? actor_x :
                inherited_x[poses[n].parent]) * poses[n].transform.scale.x;
        for (std::size_t n = 0; n < poses.size(); ++n) {
            if (!(catalog.models()[m].nodes[n].depth_flags & 0xf000u)) continue;
            const float parent_x = poses[n].parent == isle::menu::no_index ?
                actor_x : inherited_x[poses[n].parent];
            const auto& scale = poses[n].transform.scale;
            float expected[4][4]{};
            expected[0][0] = projection[0][0] * (parent_x * scale.x);
            expected[1][1] = projection[1][1] * (scale.y * parent_x);
            expected[2][2] = projection[2][2] * (parent_x * scale.x);
            expected[2][3] = projection[2][3] * (parent_x * scale.x);
            IsleGameFixedMatrix fixed;
            hal_mtx_f2l(expected,&fixed);
            unsigned found_lanes = 0;
            for (unsigned lane_id = 0; lane_id < frame.lanes.size(); ++lane_id) for (
                std::size_t i = 0; i + 13 < frame.lanes[lane_id].commands; ++i) {
                const auto lane = frame.lanes[lane_id];
                const auto at = lane.address / 4 + i * 2;
                if ((words[at] >> 24) != 0xda || words[at + 1] != base + n * 64) continue;
                require(words[at + 2] == 0xd5000001,"kind44 omitted MVP recomputation before row replacement");
                for (unsigned row = 0; row < 3; ++row) for (unsigned pair = 0; pair < 2; ++pair) {
                    const auto integral = at + 4 + (row * 2 + pair) * 2;
                    const auto fractional = integral + 12;
                    require(words[integral] == (0xdb000000u | (row * 8 + pair * 4)) &&
                        words[integral + 1] == fixed.m[row / 2][(row % 2) * 2 + pair] &&
                        words[fractional] == (0xdb000020u | (row * 8 + pair * 4)) &&
                        words[fractional + 1] == fixed.m[2 + row / 2][(row % 2) * 2 + pair],
                        "kind44 lost RTS inherited X or incorrectly used inherited Y for its Y row");
                }
                require(!(found_lanes & (1u << lane_id)),"kind44 prefix was duplicated in one lane");
                found_lanes |= 1u << lane_id;
            }
            require(found_lanes == used[n],"kind44 matrix presence differs from its live reachable payload lanes");
            require(!require_all_nodes || used[n] != 0,"all-visible source pose lost required kind44 coverage");
            ++reached;
        }
        base += static_cast<std::uint32_t>(poses.size() * 64);
    }
    require(reached == 3,"actual title no longer exercises all three source kind44 nodes");
}
int run(const std::filesystem::path& input) {
    const auto snapshot = rom::read_snapshot(input);
    auto hydrated = menu::hydrate(snapshot);
    require(bool(hydrated),"private resource hydration rejected");
    auto& catalog = *hydrated.catalog;
    World world;
    std::array<menu::animation::SlotCapacity,5> capacities;
    for (std::size_t m = 0; m < capacities.size(); ++m) capacities[m] = menu::animation::slot_capacity(catalog.models()[m]);
    auto animated = menu::animation::Runtime::create(
        catalog, capacities, {{1500,0,0},{0,0,0},0,30,true,true,0,{}},{});
    require(bool(animated),"animation creation rejected");
    auto& animation = *animated.runtime;
    std::array<menu::animation::InstanceHandle,5> handles;
    std::array<menu::model::Binding,5> bindings;
    menu::model::CameraBasisState camera_basis;
    std::vector<IsleGameModelNodeInit> attachment_scratch(226);
    for (std::size_t m = 0; m < handles.size(); ++m) {
        const auto acquired = animation.acquire(m,m,catalog.models()[m].receives_effect_callback);
        require(bool(acquired),"title instance acquisition rejected"); handles[m] = acquired.handle;
        IsleGameModel* model = nullptr;
        require(bool(menu::model::attach_title_model(world.game,world.objects[m],catalog.models()[m],
            animation.model(handles[m]),attachment_scratch,&model)),"title native attachment rejected");
        bindings[m] = {handles[m],world.objects[m],model};
    }
    const std::array<menu::RamRange,5> reserved{{{top,0x10000},{stack,0x10000},
        {0x200000,0x40000},{depth,target_bytes},{guard,256}}};
    {
        auto tiny_layout = menu::model::Layout{};
        tiny_layout.lists.size = 64;
        auto tiny = menu::model::Producer::create(world.game,world.camera,camera_basis,catalog,animation,
            bindings,tiny_layout,reserved);
        require(bool(tiny),"bounded producer construction rejected before capacity exercise");
        const IsleGameCameraMatrix* matrix = nullptr;
        native(isle_game_camera_matrix(world.game,world.camera,0,&matrix));
        const auto revision = matrix->revision;
        const IsleGamePerspective changed{31,4.0f / 3.0f,47,12800,1};
        native(isle_game_camera_set_perspective(world.game,world.camera,&changed));
        std::vector<std::uint32_t> ram(menu::rdram_size / 4,0x11223344);
        const auto retained_before = camera_basis.value;
        const auto rejected = tiny.producer->prepare(ram,1);
        require(!rejected && rejected.status.error == menu::model::Error::extent &&
            matrix->revision == revision && tiny.producer->world_nodes(handles[0]).empty() &&
            std::all_of(ram.begin(),ram.end(),[](auto v) { return v == 0x11223344; }) &&
            std::memcmp(&retained_before,&camera_basis.value,sizeof(retained_before)) == 0,
            "capacity exhaustion published RAM/native camera/world state");
        const IsleGamePerspective restored{30,4.0f / 3.0f,47,12800,1};
        native(isle_game_camera_set_perspective(world.game,world.camera,&restored));
    }
    {
        auto exact_layout = menu::model::Layout{};
        exact_layout.payloads.size = 128 + 292 * 64;
        auto exact = menu::model::Producer::create(world.game,world.camera,camera_basis,catalog,animation,
            bindings,exact_layout,reserved);
        require(bool(exact),"disabled exact-capacity producer construction failed");
        std::vector<std::uint32_t> ram(menu::rdram_size / 4,0x11223344);
        require(bool(exact.producer->prepare(ram,1)),"disabled exact-capacity prepare failed");
        const auto baseline = ram;
        const auto old_world = exact.producer->world_nodes(handles[0])[0];
        const auto old_local = exact.producer->native_nodes(handles[0])[0];
        IsleGameModelTransform t{{3,4,5},{0,0.4f,0},{1,1,1}};
        auto* handle = exact.producer->native_model(handles[0]);
        native(isle_game_model_set_placement(world.game,handle,ISLE_GAME_MODEL_PLACEMENT_RT,&t));
        const IsleGameModelPlacement* placement = nullptr;
        native(isle_game_model_placement(world.game,handle,&placement));
        const auto accepted = *placement;
        require(!exact.producer->prepare(ram,1) && ram == baseline &&
            std::memcmp(&old_world,&exact.producer->world_nodes(handles[0])[0],sizeof(old_world)) == 0 &&
            exact.producer->native_nodes(handles[0])[0].revision == old_local.revision &&
            placement->revision == accepted.revision && placement->mode == ISLE_GAME_MODEL_PLACEMENT_RT,
            "payload rejection changed RAM/world/local or rolled back accepted native placement");
        native(isle_game_model_set_placement(world.game,handle,ISLE_GAME_MODEL_PLACEMENT_DISABLED,nullptr));
        require(bool(exact.producer->prepare(ram,1)) && ram == baseline,
            "disable did not restore exact-capacity admission/old local payload addresses");
    }
    auto created = menu::model::Producer::create(world.game,world.camera,camera_basis,catalog,animation,bindings,{},reserved);
    require(bool(created),"model producer creation rejected");
    auto& producer = *created.producer;
    {
        // Before source flag commands run, require coverage of every actual
        // model and all three kind44 nodes, not merely those visible when frozen.
        IsleGameModelTransform placed{{70,-25,35},{0,1.57079637f,0},{-0.8f,1.2f,0.7f}};
        for (std::size_t m = 0; m < 5; ++m)
            native(isle_game_model_set_placement(world.game,producer.native_model(handles[m]),
                ISLE_GAME_MODEL_PLACEMENT_RTS,&placed));
        std::vector<std::uint32_t> source_pose(menu::rdram_size / 4,0x11223344);
        const auto frame = producer.prepare(source_pose,16);
        require(bool(frame),"all-visible source-pose placement preparation failed");
        placement_lists(source_pose,frame.frame,catalog,animation,handles,true);
        const IsleGameCameraParameters* parameters = nullptr;
        native(isle_game_camera_parameters(world.game,world.camera,&parameters));
        kind44_inheritance(source_pose,frame.frame,catalog,animation,handles,parameters->perspective,
            placed.scale.x,true);
        for (std::size_t m = 0; m < 5; ++m)
            native(isle_game_model_set_placement(world.game,producer.native_model(handles[m]),
                ISLE_GAME_MODEL_PLACEMENT_DISABLED,nullptr));
    }
    rsp::CanonicalExecutor executor(true,{12000000,4 * 1024 * 1024});
    const auto arena_used = world.arena.cursor;
    auto step = [&] {
        for (const auto handle : handles) require(bool(animation.step_model(handle)),"model animation rejected");
        require(bool(animation.step_camera()),"camera animation rejected");
        camera_pose(world,animation.camera());
    };
    auto execute = [&](bool cold, bool expect_all_hidden = false,
                       rsp::CanonicalExecutor* alternate = nullptr, std::uint64_t epoch = 1) {
        auto& target = alternate ? *alternate : executor;
        auto preparation = target.begin_preparation();
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
        const auto nodes_before = producer.native_nodes(handles[0]);
        const auto first_revision = nodes_before.empty() ? 0 : nodes_before[0].revision;
        const auto invalid = producer.prepare(words.first(words.size() - 1),1);
        require(!invalid && std::equal(words.begin(),words.end(),before.begin()),"invalid request changed RAM");
        if (!nodes_before.empty()) require(nodes_before[0].revision == first_revision,"invalid request changed native cache");
        const auto frame = producer.prepare(words,epoch);
        require(bool(frame),"native model prepare rejected");
        const IsleGameCameraParameters* context_camera = nullptr;
        native(isle_game_camera_parameters(world.game,world.camera,&context_camera));
        menu::model::WorldMatrix expected_basis;
        native(isle_model_source48_basis(context_camera,camera_basis.value.perspective.m,1,expected_basis.m));
        require(std::memcmp(&expected_basis,&camera_basis.value.basis48,sizeof(expected_basis)) == 0,
            "native Title camera preparation did not publish its source-derived retained basis");
        unsigned active_placements = 0;
        float placement_x = 1;
        for (std::size_t m = 0; m < 5; ++m) {
            const IsleGameModelPlacement* placement = nullptr;
            native(isle_game_model_placement(world.game,producer.native_model(handles[m]),&placement));
            if (placement->mode == ISLE_GAME_MODEL_PLACEMENT_DISABLED) continue;
            const auto x = placement->mode == ISLE_GAME_MODEL_PLACEMENT_RTS ? placement->transform.scale.x : 1.0f;
            if (active_placements++) require(x == placement_x,"GPU proof phase has unequal placement X inputs");
            placement_x = x;
        }
        if (active_placements) {
            require(active_placements == 5,"GPU proof phase must exercise all five accepted placements");
            placement_lists(words,frame.frame,catalog,animation,handles);
            const IsleGameCameraParameters* parameters = nullptr;
            native(isle_game_camera_parameters(world.game,world.camera,&parameters));
            kind44_inheritance(words,frame.frame,catalog,animation,handles,parameters->perspective,placement_x);
        }
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
        auto completed = target.execute(std::move(preparation),prepared.task,prepared.boot);
        require(!target.quiescent(),"result borrow did not retain execution ownership");
        bool rejected = false;
        try { auto forbidden = target.begin_preparation(); static_cast<void>(forbidden); }
        catch (const std::exception&) { rejected = true; }
        require(rejected,"outstanding GPU result allowed RAM reuse");
        for (unsigned i = 0; i < 256; ++i)
            require(completed.rdram()[guard + i] == std::byte((i % 4 == 0) ? 0xdf :
                (i % 4 == 1) ? 0x9b : (i % 4 == 2) ? 0x57 : 0x13),"actual GPU changed out-of-scope guard");
        auto result = capture(completed);
        completed.release(); require(target.quiescent(),"result release did not retire GPU borrow");
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
        visible_flags[m] = animation.model(handles[m]).object_flags;
        require(bool(animation.set_object_flags(handles[m],visible_flags[m] | 1u)),"hide rejected");
    }
    require(animation.generation() == visible_generation,"visibility edit advanced animation");
    const auto hidden = execute(false,true); // Same RAM/epoch, no step/synchronize.
    require(hidden.triangles.empty(),"hidden objects still emitted actual RSP triangles");
    for (std::size_t i = 0; i < target_bytes; ++i)
        require(hidden.color[i] == std::byte((i & 1) ? 1 : 0) &&
            hidden.depth[i] == std::byte((i & 1) ? 0xfc : 0xff),
            "hidden model payloads still changed actual RDP color/depth");
    for (std::size_t m = 0; m < visible_flags.size(); ++m)
        require(bool(animation.set_object_flags(handles[m],visible_flags[m])),"show rejected");
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
    for (std::size_t m = 0; m < handles.size(); ++m) {
        const std::vector<std::size_t> disabled(catalog.models()[m].nodes.size(),menu::no_index);
        require(bool(animation.bind_model(handles[m],disabled)),"disable real model bindings rejected");
        require(bool(animation.reset_materials(handles[m])),"restore real material resources rejected");
    }
    const auto frozen = execute(false);
    for (unsigned frame = 0; frame < 180; ++frame)
        for (const auto handle : handles)
            require(bool(animation.step_model(handle)),"disabled real model step rejected");
    const auto still_frozen = execute(false);
    require(frozen.triangles == still_frozen.triangles && frozen.color == still_frozen.color &&
        frozen.depth == still_frozen.depth,"disabled real bindings changed completed geometry/color/depth");
    const auto placement_generation = animation.generation();
    std::array<std::vector<IsleGameModelNode>,5> local_before;
    std::array<std::vector<menu::model::WorldMatrix>,5> worlds_before;
    for (std::size_t m = 0; m < 5; ++m) {
        const auto nodes = producer.native_nodes(handles[m]);
        local_before[m].assign(nodes.begin(),nodes.end());
        const auto worlds = producer.world_nodes(handles[m]);
        worlds_before[m].assign(worlds.begin(),worlds.end());
    }
    IsleGameModelTransform actor{{70,-25,35},{0,1.57079637f,0},
        {std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity(),0}};
    auto set_placement = [&](IsleGameModelPlacementMode mode) {
        for (std::size_t m = 0; m < 5; ++m)
            native(isle_game_model_set_placement(world.game,producer.native_model(handles[m]),mode,
                mode == ISLE_GAME_MODEL_PLACEMENT_DISABLED ? nullptr : &actor));
    };
    auto check_locals = [&] {
        require(animation.generation() == placement_generation,"placement advanced animation generation");
        for (std::size_t m = 0; m < 5; ++m) {
            const auto nodes = producer.native_nodes(handles[m]);
            require(nodes.size() == local_before[m].size(),"placement inserted a synthetic source node");
            for (std::size_t n = 0; n < nodes.size(); ++n)
                require(nodes[n].revision == local_before[m][n].revision &&
                    std::memcmp(&nodes[n].fixed,&local_before[m][n].fixed,sizeof(nodes[n].fixed)) == 0,
                    "placement-only edit changed ordinary native local matrix/revision");
        }
    };
    set_placement(ISLE_GAME_MODEL_PLACEMENT_RT);
    require(bool(producer.synchronize()),"placement-only attachment synchronization failed");
    check_locals();
    for (std::size_t m = 0; m < 5; ++m)
        for (std::size_t n = 0; n < worlds_before[m].size(); ++n)
            placement_world(worlds_before[m][n],producer.world_nodes(handles[m])[n],actor,false);
    const auto rt_placed = execute(false);
    require(rt_placed.triangles != still_frozen.triangles && rt_placed.color != still_frozen.color &&
        rt_placed.depth != still_frozen.depth,"frozen animation RT placement did not change actual geometry/color/depth");
    actor.scale = {-0.8f,1.2f,0.7f};
    set_placement(ISLE_GAME_MODEL_PLACEMENT_RTS);
    require(bool(producer.synchronize()),"RTS attachment synchronization failed");
    check_locals();
    for (std::size_t m = 0; m < 5; ++m)
        for (std::size_t n = 0; n < worlds_before[m].size(); ++n)
            placement_world(worlds_before[m][n],producer.world_nodes(handles[m])[n],actor,true);
    const auto rts_placed = execute(false);
    require(rts_placed.triangles != rt_placed.triangles && rts_placed.color != rt_placed.color &&
        rts_placed.depth != rt_placed.depth,"negative/nonuniform RTS placement did not change completed GPU outputs");
    {
        std::vector<std::uint32_t> fresh(menu::rdram_size / 4,0x89abcdef);
        auto frame = producer.prepare(fresh,17);
        require(bool(frame),"fresh pointer/epoch active-placement preparation failed");
        placement_lists(fresh,frame.frame,catalog,animation,handles);
        const auto installed = fresh;
        for (const auto range : producer.reservations())
            std::fill(fresh.begin() + static_cast<std::ptrdiff_t>(range.offset / 4),
                fresh.begin() + static_cast<std::ptrdiff_t>((range.offset + range.size) / 4),0x76543210);
        frame = producer.prepare(fresh,18);
        require(bool(frame),"same pointer/new epoch placement preparation failed");
        const IsleGameCameraParameters* camera = nullptr;
        native(isle_game_camera_parameters(world.game,world.camera,&camera));
        kind44_inheritance(fresh,frame.frame,catalog,animation,handles,camera->perspective,actor.scale.x);
        constexpr auto matrix_end = (0x110000 + 128 + (292 + 5) * 64) / 4;
        require(std::equal(fresh.begin() + 0x110000 / 4,fresh.begin() + matrix_end,
            installed.begin() + 0x110000 / 4),"new epoch failed to republish active placement/local payloads");
        const auto stable_ram = fresh;
        const IsleGameModelPlacement* old = nullptr;
        native(isle_game_model_placement(world.game,producer.native_model(handles[0]),&old));
        const auto revision = old->revision;
        set_placement(ISLE_GAME_MODEL_PLACEMENT_RTS);
        require(bool(producer.prepare(fresh,18)) && fresh == stable_ram && old->revision == revision,
            "identical placement input invalidated resident command/matrix caches");
        auto distinctive = actor;
        distinctive.position.x += 3;
        native(isle_game_model_set_placement(world.game,producer.native_model(handles[1]),
            ISLE_GAME_MODEL_PLACEMENT_RTS,&distinctive));
        require(bool(producer.prepare(fresh,18)),"distinctive placement preparation failed");
        const IsleGameModelPlacement* second = nullptr;
        native(isle_game_model_placement(world.game,producer.native_model(handles[2]),&second));
        const auto unchanged_revision = second->revision;
        native(isle_game_model_set_placement(world.game,producer.native_model(handles[0]),
            ISLE_GAME_MODEL_PLACEMENT_DISABLED,nullptr));
        require(bool(producer.prepare(fresh,18)),"active placement compaction failed");
        constexpr auto placement_base = 0x110000 + 128 + 292 * 64;
        for (std::size_t m = 1; m < 5; ++m) {
            const IsleGameModelPlacement* active = nullptr;
            native(isle_game_model_placement(world.game,producer.native_model(handles[m]),&active));
            for (unsigned row = 0; row < 4; ++row) for (unsigned col = 0; col < 4; ++col)
                require(fresh[(placement_base + (m - 1) * 64) / 4 + row * 4 + col] ==
                    active->fixed.m[row][col],"compacted address reused stale equal-revision placement payload");
        }
        IsleGameCounts live{};
        native(isle_game_counts(world.game,&live));
        require(live.model_nodes == 292 && live.matrices == 298 && second->revision == unchanged_revision,
            "compaction changed native indices/revisions or live matrix accounting");
        set_placement(ISLE_GAME_MODEL_PLACEMENT_RTS);
        require(bool(producer.prepare(fresh,18)),"placement re-enable after compaction failed");
        const auto ram_before_failure = fresh;
        const auto world_before_failure = producer.world_nodes(handles[0])[0];
        const auto camera_before_failure = *camera;
        const IsleGameCameraMatrix* camera_matrix = nullptr;
        native(isle_game_camera_matrix(world.game,world.camera,1,&camera_matrix));
        const auto camera_revision = camera_matrix->revision;
        distinctive = actor; distinctive.position.x += 1;
        native(isle_game_model_set_placement(world.game,producer.native_model(handles[0]),
            ISLE_GAME_MODEL_PLACEMENT_RTS,&distinctive));
        const IsleGameLookAt invalid_fixed_view{{40000,0,0},{40000,0,-1},{0,1,0}};
        native(isle_game_camera_set_look_at(world.game,world.camera,&invalid_fixed_view));
        require(!producer.prepare(fresh,18) && fresh == ram_before_failure &&
            std::memcmp(&world_before_failure,&producer.world_nodes(handles[0])[0],sizeof(world_before_failure)) == 0 &&
            camera_matrix->revision == camera_revision && old->transform.position.x == distinctive.position.x,
            "camera rejection partially published worlds/RAM/cache or rolled back accepted placement");
        native(isle_game_camera_set_look_at(world.game,world.camera,&camera_before_failure.look_at));
        set_placement(ISLE_GAME_MODEL_PLACEMENT_RTS);
        require(bool(producer.prepare(fresh,18)),"placement/camera rejection recovery failed");
    }
    {
        rsp::CanonicalExecutor fresh_executor(true,{12000000,4 * 1024 * 1024});
        const auto fresh_gpu = execute(true,false,&fresh_executor,19);
        require(fresh_gpu.triangles == rts_placed.triangles && fresh_gpu.color == rts_placed.color &&
            fresh_gpu.depth == rts_placed.depth,"fresh canonical memory/epoch lost active placement");
    }
    set_placement(ISLE_GAME_MODEL_PLACEMENT_DISABLED);
    const auto placement_restored = execute(false);
    check_locals();
    require(placement_restored.triangles == still_frozen.triangles &&
        placement_restored.color == still_frozen.color && placement_restored.depth == still_frozen.depth,
        "disabled placement failed exact completed geometry/color/depth restoration");
    for (std::size_t m = 0; m < 5; ++m)
        require(std::memcmp(worlds_before[m].data(),producer.world_nodes(handles[m]).data(),
            worlds_before[m].size() * sizeof(menu::model::WorldMatrix)) == 0,
            "disabled placement did not exactly restore attachment worlds");
    for (std::size_t m = 0; m < handles.size(); ++m) {
        const auto& source = catalog.models()[m];
        std::vector<std::size_t> models,materials;
        models.reserve(source.nodes.size());
        for (const auto& node : source.nodes) {
            models.push_back(node.animation);
            for (std::size_t k = 0; k < node.materials.size(); ++k)
                materials.push_back(k < node.material_animations.size() ?
                    node.material_animations[k] : menu::no_index);
        }
        require(bool(animation.bind_model(handles[m],models,0,menu::animation::ModelBindPose::restore)),
                "real model rebind rejected");
        require(bool(animation.bind_materials(handles[m],materials)),"real material rebind rejected");
        require(bool(animation.set_model_speed(handles[m],source.animation_speed)) &&
                bool(animation.set_texture_speed(handles[m],source.animation_speed)),"real rebound speed rejected");
        require(bool(animation.step_model(handles[m])),"real rebound immediate evaluation rejected");
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
    std::cout << "menu_models_rsp_rdp=passed models=5 nodes=292 frames=initial,advanced,frozen,RT,RTS,rebound,restored "
        "bindings=disabled_stable_and_rebound placement=frozen_animation_RT_RTS_disabled_exact_restore "
        "fresh_pointer_epoch=republished visibility=hide_show_without_step guard=preserved leases=exclusive "
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
