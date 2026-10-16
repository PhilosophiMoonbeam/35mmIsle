// Private-ROM camera diagnostics, not Original title/Beach scene or hardware parity.
// Public wire reference: https://hack64.net/wiki/doku.php?id=f3dex2
// Ambient-only light-count contract:
// https://ultra64.ca/files/documentation/online-manuals/functions_reference_manual_2.0i/gsp/gSPNumLights.html
// Run: engine-camera-rsp-smoke /absolute/path/to/private/pokemonsnap.z64
// Requires ISLE_BUILD_GAME_ENGINE/RSP_PROBE/RDP_PROBE; uses CPU Vulkan only.
#include "35mmisle/engine.h"
#include "rom_snapshot.hpp"
#include "rsp_fixture.hpp"
#include "rsp_render.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <iostream>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {
namespace probe = isle::rsp_probe;
constexpr std::uint32_t projection = 0x101000, view = 0x101040, model = 0x101080, lookat = 0x101100;
constexpr std::uint32_t diffuse = 0x101120, ambient = 0x101140;
constexpr std::uint32_t texture = 0x101200, vertices = 0x103000;
constexpr std::uint32_t depth = 0x310000;
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
void ok(IsleGameResult result) { require(result == ISLE_GAME_OK, "native camera rejected operation"); }
struct World {
    std::array<std::byte, 65536> storage{};
    isle_arena arena{};
    IsleGame* game = nullptr;
    IsleGameCamera* camera = nullptr;
    explicit World(IsleGameCameraMatrixKind kind = ISLE_GAME_CAMERA_LOOK_AT_REFLECT_ROLL) {
        require(isle_arena_init(&arena, storage.data(), storage.size()) == ISLE_OK, "arena init");
        ok(isle_game_create(&arena, &game));
        IsleGameObject* object = nullptr;
        ok(isle_game_add_object(game, 1, 0, 0, nullptr, nullptr, &object));
        ok(isle_game_attach_camera(game, object, &camera));
        ok(isle_game_camera_add_matrix(game, camera, ISLE_GAME_CAMERA_PERSPECTIVE_FAST));
        ok(isle_game_camera_add_matrix(game, camera, kind));
        const IsleGamePerspective p{60, 4.0f / 3, 1, 100, 1};
        const IsleGameLookAt pose{{0, 0, 5}, {0, 0, 0}, {0, 1, 0}};
        ok(isle_game_camera_set_perspective(game, camera, &p));
        ok(isle_game_camera_set_look_at(game, camera, &pose));
    }
    ~World() { if (game && isle_game_destroy(game) != ISLE_GAME_OK) std::terminate(); }
};
struct Wire {
    std::span<std::uint32_t> memory;
    std::uint32_t next = probe::commands_address;
    void word(std::uint32_t at, std::uint32_t value) { memory[at / 4] = value; }
    void byte(std::uint32_t at, std::uint8_t value) {
        const auto shift = (3 - (at & 3)) * 8;
        auto& target = memory[at / 4];
        target = (target & ~(255u << shift)) | (std::uint32_t(value) << shift);
    }
    void half(std::uint32_t at, std::uint16_t value) {
        byte(at, static_cast<std::uint8_t>(value >> 8)); byte(at + 1, static_cast<std::uint8_t>(value));
    }
    void matrix(std::uint32_t at, const IsleGameFixedMatrix& matrix) {
        for (unsigned r = 0; r < 4; ++r)
            for (unsigned c = 0; c < 4; ++c) word(at + (r * 4 + c) * 4, matrix.m[r][c]);
    }
    void reflection(const IsleGameCameraReflection& reflection) {
        for (unsigned axis = 0; axis < 2; ++axis) {
            for (unsigned i = 0; i < 16; ++i) byte(lookat + axis * 16 + i, 0);
            for (unsigned c = 0; c < 3; ++c) {
                byte(lookat + axis * 16 + c, reflection.color[axis][c]);
                byte(lookat + axis * 16 + 4 + c, reflection.color_copy[axis][c]);
                byte(lookat + axis * 16 + 8 + c,
                     static_cast<std::uint8_t>(reflection.direction[axis][c]));
            }
        }
    }
    void cmd(std::uint32_t a, std::uint32_t b) {
        require(next + 8 <= probe::commands_address + 0x1000, "camera display list exceeds fixture workspace");
        word(next, a); word(next + 4, b); next += 8;
    }
    void fill(std::uint32_t target, std::uint32_t color) {
        cmd(0xe7000000, 0); cmd(0xff10003f, target); cmd(0xed000000, (256u << 12) | 192u);
        cmd(0xef300000, 0); cmd(0xf7000000, color); cmd(0xf60fc0bc, 0);
    }
    void prepare(std::uint16_t norm) {
        fill(depth, 0xfffcfffc); fill(probe::color_address, 0x00010001);
        cmd(0xe7000000, 0); cmd(0xfe000000, depth);
        cmd(0xef080cf0, 0x00552078); // Perspective texture, point filter, opaque Z compare/update.
        cmd(0xfc000000 | (8u << 20) | (16u << 15) | (7u << 12) |
                (7u << 9) | (8u << 5) | 16u,
            (8u << 28) | (8u << 24) | (7u << 21) | (7u << 18) |
                (1u << 15) | (7u << 12) | (1u << 9) | (1u << 6) | (7u << 3) | 1u);
        cmd(0xdb060000, 0); cmd(0xd9000000, 0x00a60005); // Z/shade/smooth/clipping/light/texgen.
        cmd(0xdc080008, 0x102000); cmd(0xda380007, projection); cmd(0xdb0e0000, norm);
        cmd(0xda380005, view); // GAME kind14: view multiplies the projection.
        const IsleGameFixedMatrix identity{{{0x00010000,0,1,0},{0,0x00010000,0,1},{0,0,0,0},{0,0,0,0}}};
        matrix(0x101080, identity); cmd(0xda380003, 0x101080);
        cmd(0xd5000001, 0); // GAME SPECIAL_1 combined-MVP recalculation interface.
        cmd(0xdc08000a, lookat); cmd(0xdc08030a, lookat + 16); // LookAt X/Y light slots, 24-byte stride.
        // Ambient-only is one BLACK directional light followed by white ambient,
        // not a raw zero light-count word. Keep both LookAt slots separate.
        for (unsigned i = 0; i < 16; ++i) {
            byte(diffuse + i, 0); byte(ambient + i, 0);
        }
        byte(diffuse + 10, 127);
        for (unsigned c = 0; c < 3; ++c) {
            byte(ambient + c, 255); byte(ambient + 4 + c, 255);
        }
        cmd(0xdc08060a, diffuse); cmd(0xdc08090a, ambient);
        cmd(0xdb020000, 24); // One directional slot, reset AFTER loading lights.
        cmd(0xfd100003, texture); cmd(0xf5100000, 0x07000000);
        cmd(0xe6000000, 0); cmd(0xf3000000, 0x0700f800); // Sixteen RGBA16 texels, dxt=2048.
        cmd(0xe7000000, 0); cmd(0xf5100200, 0x00008020); // Render tile, line=1, S/T wrap mask=2.
        cmd(0xf2000000, 0x0000c00c); cmd(0xd7000002, 0x20002000);
        for (unsigned y = 0; y < 4; ++y)
            for (unsigned x = 0; x < 4; ++x)
                half(texture + (y * 4 + x) * 2,
                     static_cast<std::uint16_t>(((x * 10) << 11) | ((y * 10) << 6) | 1));
        const std::array<std::array<std::int16_t,3>,3> positions{{{-2,-1,0},{2,-1,0},{-2,2,0}}};
        const std::array<std::array<std::uint8_t,3>,3> normals{{{127,0,0},{0,127,0},{0,0,127}}};
        for (unsigned i = 0; i < 3; ++i) {
            for (unsigned c = 0; c < 3; ++c) {
                half(vertices + i * 16 + c * 2, static_cast<std::uint16_t>(positions[i][c]));
                byte(vertices + i * 16 + 12 + c, normals[i][c]);
            }
            half(vertices + i * 16 + 6, 0); word(vertices + i * 16 + 8, 0); byte(vertices + i * 16 + 15, 255);
        }
        cmd(0x01003006, vertices); cmd(0x05000204, 0); cmd(0xe9000000, 0); cmd(0xdf000000, 0);
    }
    void prepare_roll(std::uint16_t norm) {
        fill(depth, 0xfffcfffc); fill(probe::color_address, 0x00010001);
        cmd(0xe7000000, 0); cmd(0xfe000000, depth);
        cmd(0xef000cf0, 0x00552078); // One-cycle shade with opaque Z compare/update.
        cmd(0xfc887f10, 0x88fe793c); // Both combiner cycles select vertex shade.
        cmd(0xdb060000, 0); cmd(0xd9000000, 0x00a00005); // Z/shade/smooth/clipping, NO light/texgen.
        cmd(0xd7000000, 0); cmd(0xdc080008, 0x102000);
        // Exact GAME df8815e75f3d79618466e01e0827c6165b93d11c render.c:
        // 2262-2268,2360-2363: kind3 projection LOAD + normalization.
        // 2294-2300: kind8 look-at-roll with fixed (0,1,0) up.
        // 2373-2377: kind8 projection MUL, NO gSPLookAt (unlike kind14).
        // Source blob 38e4e802c306971624149d83902767b59eca3214, SHA256
        // fafaa7eb5b5c81bb7fc5e5b2b687375c5088b0edcc52e56317f4e374418eb9f2.
        cmd(0xda380007, projection); cmd(0xdb0e0000, norm);
        cmd(0xda380005, view);
        // A genuine native translate-only model, not the reflection test's
        // identity modelview. This fixture loads its complete root local.
        cmd(0xda380003, model); cmd(0xd5000001, 0);
        const std::array<std::array<std::int16_t, 3>, 3> positions{{{-1,-1,0},{1,-1,0},{-1,1,0}}};
        const std::array<std::array<std::uint8_t, 3>, 3> colors{{{255,32,16},{16,255,32},{32,16,255}}};
        for (unsigned i = 0; i < 3; ++i) {
            for (unsigned c = 0; c < 3; ++c) {
                half(vertices + i * 16 + c * 2, static_cast<std::uint16_t>(positions[i][c]));
                byte(vertices + i * 16 + 12 + c, colors[i][c]);
            }
            half(vertices + i * 16 + 6, 0); word(vertices + i * 16 + 8, 0);
            byte(vertices + i * 16 + 15, 255);
        }
        cmd(0x01003006, vertices); cmd(0x05000204, 0);
        cmd(0xe9000000, 0); cmd(0xdf000000, 0);
    }
};
// Hash only our synthetic task inputs, never private ROM/microcode bytes.
void report_inputs(const Wire& wire, bool reflection = true) {
    const auto report = [&](const char* name, std::uint32_t begin, std::uint32_t end) {
        std::uint64_t hash = 14695981039346656037ull;
        for (std::uint32_t at = begin; at < end; ++at) {
            hash ^= (wire.memory[at / 4] >> ((3 - (at & 3)) * 8)) & 255u;
            hash *= 1099511628211ull;
        }
        std::cout << "input=" << name << " begin=0x" << std::hex << begin
                  << " end=0x" << end << " fnv1a64=0x" << hash << std::dec << '\n';
    };
    report("commands", probe::commands_address, wire.next);
    report("projection", projection, projection + 64);
    report("view", view, view + 64);
    report(reflection ? "identity" : "native-model", model, model + 64);
    if (reflection) {
        report("lookat", lookat, lookat + 32);
        report("diffuse", diffuse, diffuse + 16);
        report("ambient", ambient, ambient + 16);
        report("texture", texture, texture + 32);
    }
    report("viewport", 0x102000, 0x102010);
    report("vertices", vertices, vertices + 48);
}
struct Capture { std::vector<std::uint32_t> triangle; std::vector<std::byte> color, z; };
Capture capture(isle::rsp::CanonicalExecutor::MemoryLease& completed, bool textured = true) {
    require(bool(completed.rsp_result()) && completed.rsp_result().full_syncs == 1,
            "actual RSP task did not complete");
    Capture result;
    for (const auto& packet : completed.packets().packets()) {
        const auto words = completed.packets().words().subspan(packet.offset, packet.count);
        if (((words[0] >> 24) & 0x3f) == (textured ? 0x0fu : 0x0du)) {
            require(result.triangle.empty() && words.size() == (textured ? 44u : 28u),
                    "expected one shaded Z triangle with the selected texture state");
            result.triangle.assign(words.begin(), words.end());
        }
    }
    require(result.triangle.size() == (textured ? 44u : 28u), "native camera task did not emit expected geometry");
    const auto ram = completed.rdram();
    result.color.assign(ram.begin() + probe::color_address, ram.begin() + probe::color_address + 64 * 48 * 2);
    result.z.assign(ram.begin() + depth, ram.begin() + depth + 64 * 48 * 2);
    return result;
}
void reflection_diagnostic(const isle::rom::Snapshot& snapshot) {
    World world;
    const IsleGameCameraMatrix* matrix = nullptr;
    const IsleGameCameraReflection* reflection = nullptr;
    ok(isle_game_camera_matrix(world.game, world.camera, 0, &matrix)); const auto p = matrix->fixed;
    ok(isle_game_camera_matrix(world.game, world.camera, 1, &matrix)); const auto v = matrix->fixed;
    ok(isle_game_camera_reflection(world.game, world.camera, 1, &reflection)); const auto r0 = *reflection;
    ok(isle_game_camera_set_roll(world.game, world.camera, 1.57079632679489661923f));
    ok(isle_game_camera_reflection(world.game, world.camera, 1, &reflection)); const auto r90 = *reflection;
    ok(isle_game_camera_matrix(world.game, world.camera, 1, &matrix));
    const auto v90 = matrix->fixed;
    std::uint16_t norm = 0; ok(isle_game_camera_perspective_normalization(world.game, world.camera, &norm));
    isle::rsp::CanonicalExecutor executor(true, {1000000, probe::capture_words});
    auto task = [&](const char* phase, const IsleGameFixedMatrix& view_matrix,
                    const IsleGameCameraReflection& directions, bool cold) {
        auto preparation = executor.begin_preparation();
        if (cold) probe::initialize_fixture_memory(preparation.words(), snapshot.canonical());
        probe::Fixture fixture(preparation.words(), probe::Case::matrix);
        Wire wire{preparation.words()}; wire.matrix(projection, p); wire.matrix(view, view_matrix);
        wire.reflection(directions); wire.prepare(norm);
        std::cout << "phase=" << phase << " stage=prepared cold=" << cold
                  << " commands=" << (wire.next - probe::commands_address) / 8
                  << " light_count_wire=24 normalization=" << norm
                  << " boot_rom_begin=0x" << std::hex << probe::boot_rom_begin
                  << " boot_bytes=0x" << probe::boot_bytes
                  << " text_begin=0x" << fixture.task.text_address
                  << " text_bytes=0x" << fixture.task.text_size
                  << " data_begin=0x" << fixture.task.data_address
                  << " data_bytes=0x" << fixture.task.data_size
                  << " output_begin=0x" << fixture.task.output_address
                  << " output_end=0x" << fixture.task.output_end << std::dec << '\n';
        report_inputs(wire);
        std::cout << "phase=" << phase << " stage=execute\n" << std::flush;
        auto completed = executor.execute(std::move(preparation), fixture.task,
            snapshot.canonical().subspan(probe::boot_rom_begin, probe::boot_bytes));
        const auto& result = completed.rsp_result();
        std::cout << "phase=" << phase << " stage=completed sp_status=" << result.sp_status
                  << " dma_reads=" << result.dma_reads << " dma_writes=" << result.dma_writes
                  << " imem_writes=" << result.imem_writes << " packets=" << result.packets
                  << " packet_words=" << result.packet_words << " full_syncs=" << result.full_syncs
                  << '\n' << std::flush;
        auto output = capture(completed);
        std::cout << "phase=" << phase << " stage=captured triangle_words=" << output.triangle.size()
                  << " color_bytes=" << output.color.size() << " depth_bytes=" << output.z.size()
                  << '\n' << std::flush;
        return output;
    };
    const auto initial = task("initial", v, r0, true);
    bool depth_written = false;
    for (std::size_t i = 0; i < initial.z.size(); ++i)
        if (initial.z[i] != std::byte((i & 1) ? 0xff : 0xfc)) depth_written = true;
    require(depth_written, "actual RDP diagnostic did not write native-camera triangle depth");
    const auto isolated = task("reflection-only", v, r90, false); // ONLY reflection input bytes differ.
    require(std::equal(initial.triangle.begin(), initial.triangle.begin() + 24, isolated.triangle.begin()) &&
            std::equal(initial.triangle.begin() + 40, initial.triangle.end(), isolated.triangle.begin() + 40) &&
            initial.z == isolated.z, "reflection-only diagnostic changed geometry/shade/depth");
    require(!std::equal(initial.triangle.begin() + 24, initial.triangle.begin() + 40, isolated.triangle.begin() + 24) &&
            initial.color != isolated.color, "actual RSP LookAt did not change generated UV and actual RDP color");
    const auto rolled = task("rolled-matrix", v90, r90, false);
    require(!std::equal(initial.triangle.begin(), initial.triangle.begin() + 8, rolled.triangle.begin()) &&
            rolled.color != initial.color, "native reflection-roll matrix did not change geometry and render");
    ok(isle_game_camera_set_roll(world.game, world.camera, 0));
    ok(isle_game_camera_matrix(world.game, world.camera, 1, &matrix)); const auto restored_view = matrix->fixed;
    ok(isle_game_camera_reflection(world.game, world.camera, 1, &reflection));
    const auto restored = task("restored", restored_view, *reflection, false);
    require(restored.triangle == initial.triangle && restored.color == initial.color && restored.z == initial.z,
            "restored camera inputs did not reproduce real RSP/RDP output");
    std::cout << "reflection_camera_rsp_rdp=passed tasks=4 reflection_only_uv_color_change=yes "
                 "reflection_only_geometry_depth_unchanged=yes restored=yes Original_scene_parity=not_claimed\n";
}

void roll_diagnostic(const isle::rom::Snapshot& snapshot) {
    World world(ISLE_GAME_CAMERA_LOOK_AT_ROLL);
    IsleGameObject* owner = nullptr;
    ok(isle_game_add_object(world.game, 2, 0, 0, nullptr, nullptr, &owner));
    const IsleGameModelNodeInit initial{UINT32_MAX,
        {{0.375f, 0.25f, 0.5f}, {0, 0, 0}, {1, 1, 1}}, ISLE_GAME_MODEL_TRANSLATE};
    IsleGameModel* native_model = nullptr;
    ok(isle_game_attach_model(world.game, owner, &initial, 1, &native_model));
    const IsleGameModelNode* nodes = nullptr;
    std::size_t node_count = 0;
    ok(isle_game_model_nodes(world.game, native_model, &nodes, &node_count));
    require(node_count == 1, "kind8 diagnostic requires one native translated model");
    const auto local = nodes[0].fixed;
    ok(isle_game_camera_compute_all(world.game, world.camera));
    const IsleGameCameraMatrix* matrix = nullptr;
    ok(isle_game_camera_matrix(world.game, world.camera, 0, &matrix)); const auto p = matrix->fixed;
    ok(isle_game_camera_matrix(world.game, world.camera, 1, &matrix)); const auto unrolled = matrix->fixed;
    const IsleGameCameraReflection* absent = nullptr;
    require(isle_game_camera_reflection(world.game, world.camera, 1, &absent) ==
                ISLE_GAME_INVALID_STATE && absent == nullptr, "kind8 fabricated reflection values");
    std::uint16_t norm = 0;
    ok(isle_game_camera_perspective_normalization(world.game, world.camera, &norm));
    const auto used = world.arena.cursor;
    isle::rsp::CanonicalExecutor executor(true, {1000000, probe::capture_words});
    const auto task = [&](const char* phase, const IsleGameFixedMatrix& camera_view, bool cold) {
        auto preparation = executor.begin_preparation();
        if (cold) probe::initialize_fixture_memory(preparation.words(), snapshot.canonical());
        probe::Fixture fixture(preparation.words(), probe::Case::matrix);
        Wire wire{preparation.words()};
        wire.matrix(projection, p); wire.matrix(view, camera_view); wire.matrix(model, local);
        wire.prepare_roll(norm);
        std::cout << "phase=" << phase << " kind=8 stage=prepared cold=" << cold
                  << " commands=" << (wire.next - probe::commands_address) / 8
                  << " normalization=" << norm
                  << " upload=projection-load,normalize,view-projection-multiply,native-modelview-load"
                     " reflection_upload=no\n";
        report_inputs(wire, false);
        std::cout << "phase=" << phase << " kind=8 stage=execute\n" << std::flush;
        auto completed = executor.execute(std::move(preparation), fixture.task,
            snapshot.canonical().subspan(probe::boot_rom_begin, probe::boot_bytes));
        const auto& result = completed.rsp_result();
        std::cout << "phase=" << phase << " kind=8 stage=completed sp_status=" << result.sp_status
                  << " dma_reads=" << result.dma_reads << " dma_writes=" << result.dma_writes
                  << " packets=" << result.packets << " packet_words=" << result.packet_words
                  << " full_syncs=" << result.full_syncs << '\n' << std::flush;
        return capture(completed, false);
    };
    const auto first = task("kind8-initial", unrolled, true);
    bool depth_written = false;
    for (std::size_t i = 0; i < first.z.size(); ++i)
        if (first.z[i] != std::byte((i & 1) ? 0xff : 0xfc)) depth_written = true;
    require(depth_written, "actual kind8 RDP task did not write geometry depth");
    ok(isle_game_camera_set_roll(world.game, world.camera, 0.61f));
    ok(isle_game_camera_compute_all(world.game, world.camera));
    ok(isle_game_camera_matrix(world.game, world.camera, 1, &matrix));
    const auto rolled = task("kind8-rolled", matrix->fixed, false);
    require(!std::equal(first.triangle.begin(), first.triangle.begin() + 8, rolled.triangle.begin()) &&
            first.color != rolled.color && first.z != rolled.z,
            "kind8 source roll did not change actual RSP geometry and RDP color/depth coverage");
    ok(isle_game_camera_set_roll(world.game, world.camera, 0));
    ok(isle_game_camera_compute_all(world.game, world.camera));
    ok(isle_game_camera_matrix(world.game, world.camera, 1, &matrix));
    const auto restored = task("kind8-restored", matrix->fixed, false);
    require(first.triangle == restored.triangle && first.color == restored.color && first.z == restored.z &&
            world.arena.cursor == used, "kind8 restoration lost actual output or allocated native updates");
    std::cout << "roll_camera_rsp_rdp=passed kind=8 tasks=3 source_projection_multiply=yes "
                 "reflection_upload=no nonidentity_native_model=yes geometry_color_depth_change=yes "
                 "restored=yes Original_scene_parity=not_claimed\n";
}

int run(const std::filesystem::path& rom) {
    const auto snapshot = isle::rom::read_snapshot(rom);
    reflection_diagnostic(snapshot);
    roll_diagnostic(snapshot);
    return 0;
}
}
#ifdef _WIN32
int wmain(int argc, wchar_t** argv) {
#else
int main(int argc, char** argv) {
#endif
    try {
        require(argc == 2, "expected one explicit private ROM path");
        return run(std::filesystem::path(argv[1]));
    } catch (const std::filesystem::filesystem_error&) {
        std::cerr << "camera diagnostic: private filesystem operation failed\n";
    } catch (const std::runtime_error& error) {
        std::cerr << "camera diagnostic: " << error.what() << '\n';
    } catch (...) { std::cerr << "camera diagnostic failed\n"; }
    return 1;
}
