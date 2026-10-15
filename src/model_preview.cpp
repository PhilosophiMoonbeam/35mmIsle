// SPDX-License-Identifier: MIT
#include "model_preview.hpp"
#include "photo_lighting.hpp"
#include "pidgey_producer.hpp"
#include "rsp_render.hpp"

#include <array>
#include <cstddef>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace {
void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
void Check(IsleGameResult result) {
    Require(result == ISLE_GAME_OK, "Native model preview C engine operation failed");
}
constexpr std::uint32_t color_address = 0x300000;
constexpr std::uint32_t depth_address = color_address + 0x1800;
constexpr std::size_t row_bytes = 64 * 4;
constexpr std::size_t tile_rows = 16;
}

struct ModelPreview::Impl {
    alignas(64) std::array<unsigned char, 131072> storage;
    isle_arena arena{};
    IsleGame* game = nullptr;
    IsleGameCamera* camera = nullptr;
    const isle::pidgey::HdResources resources;
    const isle::photo::LightingInputs lighting;
    isle::rsp::CanonicalExecutor executor;
    std::unique_ptr<isle::pidgey::PidgeyProducer> producer;
    std::array<std::uint8_t, 64 * 48 * 4> pixels;
    std::array<Gfx, 128> commands;
    const void* allocation = nullptr;
    std::size_t steady_arena = 0;
    std::uint32_t frames = 0;
    std::uint64_t first_digest = 0, last_digest = 0, changed_frames = 0;
    bool installed = false;

    Impl(const isle::rom::Snapshot& snapshot, bool require_cpu)
        : resources(snapshot), lighting(snapshot), executor(require_cpu) {
        Require(isle_arena_init(&arena, storage.data(), storage.size()) == ISLE_OK,
                "Native model preview arena initialization failed");
        Check(isle_game_create(&arena, &game));
        try {
            IsleGameObject* object = nullptr;
            Check(isle_game_add_object(game, 0xca, 0, 0, nullptr, nullptr, &object));
            Check(isle_game_attach_camera(game, object, &camera));
            Check(isle_game_camera_add_matrix(game, camera, ISLE_GAME_CAMERA_PERSPECTIVE_FAST));
            Check(isle_game_camera_add_matrix(game, camera, ISLE_GAME_CAMERA_LOOK_AT));
            const IsleGameLookAt look{{0, 30, 180}, {0, 30, 0}, {0, 1, 0}};
            const IsleGameViewport viewport{{128, 96, 511, 0}, {128, 96, 511, 0}};
            const IsleGamePerspective perspective{30, 4.0f / 3.0f, 10, 25600, 1};
            Check(isle_game_camera_set_look_at(game, camera, &look));
            Check(isle_game_camera_set_viewport(game, camera, &viewport));
            Check(isle_game_camera_set_perspective(game, camera, &perspective));
            producer = std::make_unique<isle::pidgey::PidgeyProducer>(game, camera, snapshot, resources, 1);
            producer->set_photo_pose(0, 0, {0, 0, 0}, 0);
            steady_arena = arena.cursor;
            BuildCommands();
        } catch (...) {
            producer.reset();
            if (isle_game_destroy(game) != ISLE_GAME_OK) std::terminate();
            game = nullptr;
            throw;
        }
        std::cout << "NATIVE_MODEL_PREVIEW_ENABLED: real pose-0 HD Pidgey, GAME camera/light/material, "
                     "persistent actual RSP/RDP; 64x48 point presentation through LUS. "
                     "Authored 0..10 pose sampling, not original scene/timeline or gameplay.\n" << std::flush;
    }
    ~Impl() {
        producer.reset();
        if (game && isle_game_destroy(game) != ISLE_GAME_OK) std::terminate();
    }

    void BuildCommands() {
        Gfx* out = commands.data();
        gDPPipeSync(out++);
        gDPSetScissor(out++, G_SC_NON_INTERLACE, 0, 0, 320, 240);
        gDPSetColorImage(out++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 320, 0x00100000);
        gDPSetDepthImage(out++, 0x00200000);
        gDPSetCycleType(out++, G_CYC_1CYCLE);
        gDPSetTexturePersp(out++, G_TP_NONE);
        gDPSetTextureLOD(out++, G_TL_TILE);
        gDPSetTextureLUT(out++, G_TT_NONE);
        gDPSetTextureFilter(out++, G_TF_POINT);
        gDPSetAlphaCompare(out++, G_AC_NONE);
        gDPSetRenderMode(out++, G_RM_OPA_SURF, G_RM_OPA_SURF2);
        gDPSetCombineMode(out++, G_CC_DECALRGBA, G_CC_DECALRGBA);
        for (std::size_t tile = 0; tile < 3; ++tile) {
            const auto* source = pixels.data() + tile * tile_rows * row_bytes;
            // One 64x16 RGBA32 tile is 4 KiB total, split across TMEM banks.
            // Clamp/point avoids filtering across the three independent tiles.
            gDPLoadTextureTile(out++, source, G_IM_FMT_RGBA, G_IM_SIZ_32b,
                               64, 16, 0, 0, 63, 15, 0,
                               G_TX_CLAMP, G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK,
                               G_TX_NOLOD, G_TX_NOLOD);
            const int top = static_cast<int>(tile * tile_rows * 5);
            gSPTextureRectangle(out++, 0, top * 4, 320 * 4, (top + 80) * 4,
                                G_TX_RENDERTILE, 0, 0, 205, 205);
            gDPPipeSync(out++);
        }
        gDPFullSync(out++);
        gSPEndDisplayList(out++);
        Require(out <= commands.data() + commands.size(), "Native model presentation command capacity exceeded");
    }

    void Prepare(std::uint32_t frame) {
        const auto phase = frame % 20;
        const float sample = static_cast<float>(phase <= 10 ? phase : 20 - phase);
        producer->set_photo_pose(0, sample, {0, 0, 0}, 0);
        Require(arena.cursor == steady_arena, "Native model preview update unexpectedly grew arena");
        auto preparation = executor.begin_preparation();
        auto words = preparation.words();
        if (!installed) {
            allocation = words.data();
            producer->install_inputs(words, 1);
            words[0x7fffc0 / 4] = 0x92173ab5;
            installed = true;
        }
        Require(words.data() == allocation && words[0x7fffc0 / 4] == 0x92173ab5,
                "Native model preview canonical residency changed");
        const auto prepared = producer->prepare_preview(words, 1, 0,
            {color_address, depth_address, 64, 48}, lighting);
        auto completed = executor.execute(std::move(preparation), prepared.task, prepared.boot);
        Require(bool(completed.rsp_result()) && completed.rsp_result().sp_status == 0x203 &&
                completed.rsp_result().full_syncs == 1, "Native model preview RSP task did not complete");
        const auto* memory = reinterpret_cast<const std::uint32_t*>(completed.rdram().data());
        Require(memory == allocation && memory[0x7fffc0 / 4] == 0x92173ab5,
                "Native model preview canonical canary changed");
        std::uint64_t digest = 14695981039346656037ull;
        std::size_t colored = 0;
        for (std::size_t i = 0; i < 64 * 48; ++i) {
            const auto address = color_address + static_cast<std::uint32_t>(i * 2);
            const auto value = static_cast<std::uint16_t>(memory[address / 4] >> ((address & 2) ? 0 : 16));
            colored += value != 1;
            const auto expand = [](unsigned v) { return static_cast<std::uint8_t>((v << 3) | (v >> 2)); };
            pixels[i * 4] = expand((value >> 11) & 31);
            pixels[i * 4 + 1] = expand((value >> 6) & 31);
            pixels[i * 4 + 2] = expand((value >> 1) & 31);
            pixels[i * 4 + 3] = (value & 1) ? 255 : 0;
            digest = (digest ^ value) * 1099511628211ull;
        }
        completed.release();
        Require(executor.quiescent(), "Native model preview backend lease did not retire");
        if (frames == 0) {
            Require(colored != 0, "Native model preview produced only the clear image");
            first_digest = digest;
            std::cout << "NATIVE_MODEL_FIRST_FRAME: actual_RSP_RDP=true; colored_pixels=" << colored
                      << "; pose_time=" << sample << "; real_memory_lease_retired=true.\n" << std::flush;
        } else if (digest != last_digest) {
            ++changed_frames;
        }
        last_digest = digest;
        ++frames;
    }
};

ModelPreview::ModelPreview(const isle::rom::Snapshot& snapshot, bool require_cpu_device)
    : impl_(std::make_unique<Impl>(snapshot, require_cpu_device)) {}
ModelPreview::~ModelPreview() = default;
void ModelPreview::PrepareFrame(std::uint32_t frame) { impl_->Prepare(frame); }
Gfx* ModelPreview::Commands() noexcept { return impl_->commands.data(); }
std::array<const std::uint8_t*, 3> ModelPreview::TextureBases() const noexcept {
    return {impl_->pixels.data(), impl_->pixels.data() + tile_rows * row_bytes,
            impl_->pixels.data() + 2 * tile_rows * row_bytes};
}
void ModelPreview::Report() const {
    std::cout << "NATIVE_MODEL_PREVIEW_COMPLETE: actual_tasks=" << impl_->frames
              << "; changed_adjacent_frames=" << impl_->changed_frames
              << "; first_digest=" << impl_->first_digest << "; last_digest=" << impl_->last_digest
              << "; one_persistent_allocation=true; arena_growth=0; all_canonical_leases_retired=true.\n"
              << std::flush;
}
