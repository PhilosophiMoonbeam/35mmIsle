#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
#include "game_host.hpp"
#include "graphics_task.hpp"
#include "35mmisle/rsp.hpp"
#include "35mmisle/continuation.h"

#include <SDL3/SDL.h>
#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <fast/Fast3dGui.h>
#include <fast/Fast3dWindow.h>
#include <fast/backends/gfx_rendering_api.h>
#include <libultraship/bridge/UltraBridge.h>
#include <libultraship/controller/controldeck/ControlDeck.h>
#include <libultraship/libultra/gbi.h>
#include <ship/config/Config.h>
#include <ship/config/ConsoleVariable.h>
#include <ship/controller/controldevice/controller/Controller.h>
#include <ship/core/Context.h>
#include <ship/debug/Console.h>
#include <ship/resource/ResourceManager.h>
#include <ship/thread/ThreadPool.h>

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <unordered_map>

#if !defined(F3DEX_GBI_2) || defined(GBI_FLOATS)
#error "The game host requires F3DEX_GBI_2 and integer vertices"
#endif

namespace isle::game_host {
namespace {
using Output = audio::SdlAudioOutput;
using NativeOutput = game_audio::Output;
constexpr std::uint32_t audio_boot = 0x107000;
constexpr std::uint32_t audio_text = 0x3d980, audio_data = 0x44ba0;
constexpr std::size_t tile_width = 64, tile_height = 16;
constexpr std::size_t tile_bytes = tile_width * tile_height * 4;
constexpr std::size_t commands_per_tile = 16, fixed_commands = 18;
// Keep the existing 64-byte backing alignment with exact per-extent ownership;
// no dependency on a vector allocator's lesser byte-element alignment.
struct alignas(64) Tile { std::array<std::byte, tile_bytes> bytes{}; };
static_assert(sizeof(Tile) == tile_bytes);

template<std::size_t N> void text(std::array<char, N>& to, const char* from) noexcept {
    const auto n = from ? std::min(N - 1, std::strlen(from)) : 0;
    if (n) std::memcpy(to.data(), from, n);
    to[n] = '\0';
}
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
// The actual host executables embed activeCodePage=UTF-8. At the pinned LUS
// narrow file/stream boundary use UTF-8 explicitly, never path::string()'s
// locale-dependent UTF-16 conversion or optional 8.3 aliases.
std::string lus_path(const std::filesystem::path& path) {
    const auto encoded = path.u8string();
    return {reinterpret_cast<const char*>(encoded.data()), encoded.size()};
}
class GameGui final : public Fast::Fast3dGui {
public:
    GameGui(bool input, const std::filesystem::path& data)
        : input_(input), ini_(lus_path(data / "imgui.ini")), log_(lus_path(data / "imgui_log.txt")) {}
    void release() {
        Ship::Gui::ReleaseHostReferences();
        mWindow.reset(); mResourceManager.reset(); mConsoleVariables.reset();
    }
protected:
    void ImGuiWMInit() override {
        Fast::Fast3dGui::ImGuiWMInit();
        platform_ = ImGui::GetIO().BackendPlatformUserData != nullptr;
        require(platform_, "ImGui SDL3 initialization failed");
        ImGui::GetIO().IniFilename = ini_.c_str();
        ImGui::GetIO().LogFilename = log_.c_str();
        // Must precede the first ImGui NewFrame: AutoFirst enumerates even when
        // gamepad navigation is not enabled in ImGui's config flags.
        if (!input_) ImGui_ImplSDL3_SetGamepadMode(ImGui_ImplSDL3_GamepadMode_Manual, nullptr, 0);
    }
    void ImGuiBackendInit() override {
        Fast::Fast3dGui::ImGuiBackendInit();
        renderer_ = ImGui::GetIO().BackendRendererUserData != nullptr;
        require(renderer_, "ImGui OpenGL renderer initialization failed");
    }
    void ImGuiWMShutdown() override {
        if (platform_) { Fast::Fast3dGui::ImGuiWMShutdown(); platform_ = false; }
    }
    void ImGuiBackendShutdown() override {
        if (renderer_) { Fast::Fast3dGui::ImGuiBackendShutdown(); renderer_ = false; }
    }
private:
    bool input_, platform_ = false, renderer_ = false;
    const std::string ini_, log_;
};
} // namespace

struct Host::Impl {
#ifdef _WIN32
    using GlFinish = void (__stdcall*)();
#else
    using GlFinish = void (*)();
#endif
    Config options;
    Error error = Error::none;
    std::array<char, 512> message{};
    std::filesystem::path configuration_path;
    std::shared_ptr<Ship::Context> context;
    std::shared_ptr<Ship::Config> config;
    std::shared_ptr<Ship::ConsoleVariable> variables;
    std::shared_ptr<Ship::ThreadPool> pool;
    std::shared_ptr<Ship::ResourceManager> resources;
    std::shared_ptr<Ship::Console> console;
    std::shared_ptr<GameGui> gui;
    std::shared_ptr<Fast::Fast3dWindow> window;
    std::shared_ptr<LUS::ControlDeck> deck;
    std::shared_ptr<LUS::UltraBridge> bridge;
    GlFinish finish_gpu = nullptr;
    std::array<std::array<OSContPad, game_input::port_count>, 2> pads{};
    std::uint8_t controller_bits = 0;
    std::uint32_t present_width = width, present_height = height;
    std::size_t tile_columns = width / tile_width, tile_rows = height / tile_height;
    std::unique_ptr<Tile[]> tiles = std::make_unique<Tile[]>(tile_columns * tile_rows);
    std::unique_ptr<Gfx[]> commands =
        std::make_unique<Gfx[]>(fixed_commands + tile_columns * tile_rows * commands_per_tile);
    const std::unordered_map<Mtx*, MtxF> replacements;
    bool pending = false, sdl_owned = false, window_ready = false, config_ready = false, stopped = false;
    SDL_Window* native_window = nullptr;
    SDL_WindowID native_window_id = 0;
    std::atomic<std::uint32_t> station_actions{0};
    bool watching_events = false;
    Shutdown closure{};
    Output output;
    std::unique_ptr<game_audio::Session> audio_session;
    // Only initialization borrows Snapshot. Real execution uses owned boot bytes
    // and the admitted image already installed below AudioService producer RAM.
    const rom::Snapshot* initializing_snapshot = nullptr;
    std::array<std::byte, graphics::boot_bytes> boot{};
    std::unique_ptr<rsp::Machine> machine;
    std::span<std::uint32_t> borrowed_audio_ram;

    explicit Impl(Config c) : options(std::move(c)) {}
    ~Impl() { shutdown(); }
    bool fail(Error kind, const char* reason) noexcept {
        if (error == Error::none) { error = kind; text(message, reason); }
        return false;
    }
    static bool SDLCALL watch_event(void* raw, SDL_Event* event) noexcept {
        auto& self = *static_cast<Impl*>(raw);
        if (event->type == SDL_EVENT_KEY_DOWN && !event->key.repeat &&
            event->key.key == SDLK_F9 && event->key.windowID == self.native_window_id)
            self.station_actions.fetch_add(1,std::memory_order_relaxed);
        return true;
    }
    bool audio_failure(const char* reason) noexcept {
        const auto& e = output.error();
        return fail(Error::audio, e.kind == Output::ErrorKind::none ? reason : e.message.data());
    }
    static NativeOutput::State output_state(Output::State state) noexcept {
        switch (state) {
        case Output::State::ready: return NativeOutput::State::ready;
        case Output::State::draining: return NativeOutput::State::draining;
        case Output::State::stream_drained: return NativeOutput::State::drained;
        default: return NativeOutput::State::failed;
        }
    }
    struct OutputInspection {
        Impl& host;
        NativeOutput::Info result{};
    };
    static void inspect_on_owner(void* payload) noexcept {
        auto& request = *static_cast<OutputInspection*>(payload);
        if (!SDL_IsMainThread()) {
            request.host.fail(Error::audio,"Audio inspection did not reach the actual SDL owner");
            return;
        }
        request.result = {output_state(request.host.output.state()),
                         static_cast<std::uint32_t>(request.host.output.info().source.freq)};
    }
    static NativeOutput::Info inspect_output(void* owner) noexcept {
        auto& self = *static_cast<Impl*>(owner);
        OutputInspection request{self,{}};
        if (SDL_IsMainThread()) inspect_on_owner(&request);
        else if (isle_continuation_on_owner(inspect_on_owner,&request) != ISLE_CONT_OK)
            self.fail(Error::audio,"GAME audio endpoint inspection owner handoff failed");
        return request.result;
    }
    static NativeOutput::Acceptance submit_output(void* owner, std::span<const std::int16_t> samples) noexcept {
        auto& self = *static_cast<Impl*>(owner);
        const auto result = self.output.try_submit(samples);
        if (result == Output::Submit::accepted)
            return {NativeOutput::Submit::accepted, static_cast<std::uint32_t>(samples.size() / 2)};
        if (result == Output::Submit::would_block) return {NativeOutput::Submit::would_block, 0};
        self.audio_failure("SDL PCM submission failed");
        return {};
    }
    static bool finish_output(void* owner) noexcept {
        auto& self = *static_cast<Impl*>(owner);
        return self.output.finish_input() || self.audio_failure("SDL final audio flush failed");
    }
    static NativeOutput::State poll_output(void* owner) noexcept {
        auto& self = *static_cast<Impl*>(owner);
        const auto result = self.output.poll_drain();
        if (result == Output::Drain::error) self.audio_failure("SDL audio stream drain failed");
        return output_state(self.output.state());
    }
    static bool reject_dp(void*, std::span<const std::uint32_t>) noexcept { return false; }
    static bool initialize_executor(void* owner, std::span<std::uint32_t> words) noexcept {
        auto& self = *static_cast<Impl*>(owner);
        try {
            require(self.initializing_snapshot && !self.machine, "Audio executor initialization has no admitted ROM");
            const auto& snapshot = *self.initializing_snapshot;
            graphics::validate_cold_inputs(words, snapshot, snapshot.canonical().data(), audio_boot);
            graphics::install_cold_inputs(words, snapshot, audio_boot);
            std::copy_n(snapshot.canonical().begin() + graphics::boot_rom, self.boot.size(), self.boot.begin());
            self.machine = std::make_unique<rsp::Machine>(words, reject_dp, nullptr);
            self.borrowed_audio_ram = words;
            return true;
        } catch (const std::exception& e) {
            self.fail(Error::audio, e.what());
        } catch (...) { self.fail(Error::audio, "Audio RSP initialization failed"); }
        self.machine.reset(); self.borrowed_audio_ram = {};
        return false;
    }
    static bool executor_ready(void* owner, std::span<std::uint32_t> words) noexcept {
        const auto& self = *static_cast<Impl*>(owner);
        return self.machine && self.borrowed_audio_ram.data() == words.data() && self.borrowed_audio_ram.size() == words.size();
    }
    static void retire_executor(void* owner) noexcept {
        auto& self = *static_cast<Impl*>(owner);
        self.machine.reset(); self.borrowed_audio_ram = {};
    }
    void load_configuration() {
        config = std::make_shared<Ship::Config>(lus_path(configuration_path));
    }
    void save_configuration() {
        window->SaveWindowToConfig();
        variables->Save();
        config->Save();
    }
    void init(const rom::Snapshot& snapshot) {
#ifdef _WIN32
        require(GetACP() == CP_UTF8, "Game host requires its Windows UTF-8 activeCodePage manifest");
#endif
        require(options.max_queued_audio_frames >= audio::AudioService::max_frames,
                "Audio queue must hold the largest whole native source block");
        require(SDL_WasInit(0) == 0 && ImGui::GetCurrentContext() == nullptr,
                "Game host requires exclusive SDL/ImGui acquisition ownership");
        const bool input = options.input == InputMode::control_deck;
        // SDL can partially acquire requested subsystems before returning false.
        // We had exclusive ownership, so even that failure must balance them.
        sdl_owned = true;
        if (!SDL_Init(SDL_INIT_VIDEO | (input ? SDL_INIT_GAMEPAD : 0u))) throw std::runtime_error(SDL_GetError());
        if (options.data_directory.empty()) {
            char* pref = SDL_GetPrefPath("35mmIsle", "35mmIsle");
            require(pref != nullptr, SDL_GetError());
            try { options.data_directory = std::filesystem::path(pref); }
            catch (...) { SDL_free(pref); throw; }
            SDL_free(pref);
        }
        std::filesystem::create_directories(options.data_directory);
        configuration_path = options.data_directory / "35mmIsle.json";
        if (options.renderer_resources.empty()) {
            const char* base=SDL_GetBasePath();
            require(base!=nullptr,SDL_GetError());
            const auto installed=std::filesystem::path(base)/ISLE_HOST_INSTALLED_RESOURCES;
            options.renderer_resources=std::filesystem::is_regular_file(installed/"shaders/opengl/default.shader.glsl")?
                installed:std::filesystem::path(ISLE_HOST_RENDERER_RESOURCES);
        }
        require(std::filesystem::is_regular_file(options.renderer_resources / "shaders/opengl/default.shader.glsl"),
                "Pinned LUS OpenGL shader resources are missing");
        context = Ship::Context::CreateInstance("35mmIsle", "35mmIsle");
        context->Init();
        load_configuration();
        context->GetChildren().Add(config);
        variables = std::make_shared<Ship::ConsoleVariable>(config);
        context->GetChildren().Add(variables);
        config->SetInt("Window.Backend.Id", Fast::FAST3D_SDL_OPENGL);
        if (!config->Contains("Window.Width")) config->SetInt("Window.Width", 640);
        if (!config->Contains("Window.Height")) config->SetInt("Window.Height", 480);
        variables->SetInteger(CVAR_ENABLE_MULTI_VIEWPORTS, 0);
        pool = std::make_shared<Ship::ThreadPool>(1);
        context->GetChildren().Add(pool);
        resources = std::make_shared<Ship::ResourceManager>(pool);
        context->GetChildren().Add(resources);
        resources->Init(nlohmann::json{{"archivePaths", std::vector<std::string>{lus_path(options.renderer_resources)}},
                                       {"validHashes", std::vector<std::uint32_t>{}}});
        console = std::make_shared<Ship::Console>();
        context->GetChildren().Add(console); console->Init();
        gui = std::make_shared<GameGui>(input, options.data_directory);
        window = std::make_shared<Fast::Fast3dWindow>(gui, config, variables);
        context->GetChildren().Add(window);
        if (input) { deck = std::make_shared<LUS::ControlDeck>(window, variables); context->GetChildren().Add(deck); }
        bridge = std::make_shared<LUS::UltraBridge>();
        context->GetChildren().Add(bridge);
        window->Init();
        window_ready = true;
        SDL_SetEventEnabled(SDL_EVENT_DROP_FILE, false);
        if (deck) deck->Init(&controller_bits);
        else for (const auto event : {SDL_EVENT_MOUSE_BUTTON_DOWN, SDL_EVENT_MOUSE_BUTTON_UP})
            SDL_SetEventEnabled(event, false);
        bridge->UpdateCaches(context);
        window->SetRendererUCode(ucode_f3dex2);
        window->SetTargetFps(60);
        window->SetMaximumFrameLatency(1);
        const auto interpreter = window->GetInterpreterWeak().lock();
        require(interpreter && interpreter->GetCurrentRenderingAPI() && SDL_GL_GetCurrentContext() && window->IsRunning(),
                "LUS did not establish its real SDL3/OpenGL window and renderer");
        native_window = SDL_GL_GetCurrentWindow();
        require(native_window != nullptr, SDL_GetError());
        native_window_id = SDL_GetWindowID(native_window);
        require(native_window_id != 0, SDL_GetError());
        require(SDL_AddEventWatch(watch_event,this), SDL_GetError());
        watching_events = true;
        finish_gpu = reinterpret_cast<GlFinish>(SDL_GL_GetProcAddress("glFinish"));
        require(finish_gpu != nullptr, "OpenGL completion barrier is unavailable");
        interpreter->SetNativeDimensions(float(width), float(height));
        config_ready = true;
        require(output.open({static_cast<int>(game_audio::source_rate), options.max_queued_audio_frames, options.physical_audio_device}),
                output.error().message.data());
        auto hydrated = game_audio::hydrate(snapshot);
        require(bool(hydrated), "GAME audio resource hydration failed");
        std::array<audio::effects::Section, 8> sections{};
        audio::effects::Config effects;
        require(game_audio::native_startup_effects(hydrated.resources, sections, effects) == audio::Error::none,
                "GAME default eight-section effects derivation failed");
        initializing_snapshot = &snapshot;
        auto made = game_audio::Session::create(std::move(hydrated.resources), effects,
            {this, inspect_output, submit_output, finish_output, poll_output},
            {this, initialize_executor, executor_ready, retire_executor});
        initializing_snapshot = nullptr;
        require(bool(made), "GAME audio/player/output/RSP initialization failed");
        audio_session = std::move(made.session);
        require(audio_session->initialization_ready() && executor_ready(this, audio_session->service().rdram_words()),
                "GAME audio Session is not actually ready");
    }
    void invalidate_tiles() {
        finish_gpu();
        const auto interpreter = window->GetInterpreterWeak().lock();
        require(bool(interpreter), "Presentation lost its LUS interpreter");
        for (std::size_t i = 0; i < tile_columns * tile_rows; ++i)
            interpreter->TextureCacheDelete(reinterpret_cast<const std::uint8_t*>(tiles[i].bytes.data()));
    }
    void build_commands(save::ScreenOffsets offsets) {
        Gfx* out = commands.get();
        gDPPipeSync(out++);
        gDPSetScissor(out++, G_SC_NON_INTERLACE, 0, 0, present_width, present_height);
        gDPSetColorImage(out++, G_IM_FMT_RGBA, G_IM_SIZ_16b, present_width, 0x00100000);
        gDPSetDepthImage(out++, 0x00200000);
        gDPSetCycleType(out++, G_CYC_FILL);
        gDPSetFillColor(out++, 0x00010001);
        gDPFillRectangle(out++, 0, 0, present_width - 1, present_height - 1);
        gDPPipeSync(out++);
        gDPSetCycleType(out++, G_CYC_1CYCLE);
        gDPSetTexturePersp(out++, G_TP_NONE);
        gDPSetTextureLOD(out++, G_TL_TILE);
        gDPSetTextureLUT(out++, G_TT_NONE);
        gDPSetTextureFilter(out++, G_TF_POINT);
        gDPSetAlphaCompare(out++, G_AC_NONE);
        gDPSetRenderMode(out++, G_RM_OPA_SURF, G_RM_OPA_SURF2);
        gDPSetCombineMode(out++, G_CC_DECALRGBA, G_CC_DECALRGBA);
        // GAME sched.c rounds both vertical edges DOWN to even half-lines.
        // Source clocks/half-lines describe the same full VI surface at either
        // extent: 320 uses half-pixel offsets, 640 uses whole-pixel offsets.
        const int vertical = offsets.vertical - ((offsets.vertical % 2 + 2) % 2);
        const int horizontal_scale = int(present_width / width);
        const int vertical_scale = int(present_height / height);
        for (std::size_t row = 0; row < tile_rows; ++row) for (std::size_t column = 0; column < tile_columns; ++column) {
            const int x = int(column * tile_width * 4) + offsets.horizontal * 2 * horizontal_scale;
            const int y = int(row * tile_height * 4) + vertical * 2 * vertical_scale;
            const int left = std::max(0, x), top = std::max(0, y);
            const int right = std::min(int(present_width * 4), x + int(tile_width * 4));
            const int bottom = std::min(int(present_height * 4), y + int(tile_height * 4));
            if (left >= right || top >= bottom) continue;
            const auto* source = tiles[row * tile_columns + column].bytes.data();
            gDPLoadTextureTile(out++, source, G_IM_FMT_RGBA, G_IM_SIZ_32b, tile_width, tile_height,
                0, 0, tile_width - 1, tile_height - 1, 0,
                G_TX_CLAMP, G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
            gSPTextureRectangle(out++, left, top, right, bottom, G_TX_RENDERTILE,
                (left - x) * 8, (top - y) * 8, 1024, 1024);
            gDPPipeSync(out++);
        }
        gDPFullSync(out++);
        gSPEndDisplayList(out++);
    }
    bool execute_audio(const audio::TaskView& built) noexcept {
        bool success = false;
        try {
            require(machine && borrowed_audio_ram.data() == audio_session->service().rdram_words().data(),
                    "Audio task lost its actual RSP borrower");
            const rsp::Task task{2, 0, audio_boot, graphics::boot_bytes, audio_text, 0x1000, audio_data, 0x800,
                0, 0, 0, 0, built.command_address, built.command_bytes, 0, 0};
            const auto actual = machine->execute(task, boot, {10000000, 0});
            success = bool(actual) && actual.sp_status == 0x203 && actual.packets == 0 &&
                actual.packet_words == 0 && actual.full_syncs == 0;
            if (!success) fail(Error::audio, "Audio RSP failed SP0x203/no-DPC/budget terminal validation");
        } catch (const std::exception& e) { fail(Error::audio, e.what()); }
        catch (...) { fail(Error::audio, "Audio RSP execution failed"); }
        // Every published task receives its actual terminal, including exception,
        // unexpected DPC, budget and malformed status paths. No PCM on failure.
        const auto completed = audio_session->complete(built.token, success);
        if (completed != audio::Error::none) return audio_failure("Audio actual-terminal completion failed");
        return success;
    }
    bool pump_audio() noexcept {
        if (!audio_session || stopped || error != Error::none) return false;
        if (output.state() == Output::State::failed) return audio_failure("Audio endpoint failed");
        if (audio_session->submit() != audio::Error::none) return audio_failure("Audio PCM acceptance failed");
        audio::TaskView built;
        const auto result = audio_session->build(built);
        if (result != audio::Error::none) {
            if (built.token.serial) (void)audio_session->complete(built.token, false);
            return audio_failure("Audio source task build failed");
        }
        if (built.token.serial && !execute_audio(built)) return false;
        if (audio_session->submit() != audio::Error::none) return audio_failure("Audio source-gated PCM acceptance failed");
        return true;
    }
    Shutdown shutdown() noexcept {
        if (stopped) return closure;
        if (watching_events) {
            SDL_RemoveEventWatch(watch_event,this);
            watching_events = false;
        }
        // No asynchronous audio execution exists in this owner. Complete/retire
        // PCM before Session retires Machine, and Machine before service RAM.
        if (audio_session) {
            if (audio_session->service().has_lease() && audio_session->abandon_pcm() != audio::Error::none)
                fail(Error::audio, "Audio abort could not retire its completed PCM lease");
            audio_session.reset();
        }
        machine.reset(); borrowed_audio_ram = {};
        const auto audio_closed = output.close();
        closure.audio = audio_closed.disposition;
        if (audio_closed.failed) audio_failure("Checked audio closure failed");
        const std::weak_ptr<Fast::Fast3dWindow> window_witness = window;
        const std::weak_ptr<GameGui> gui_witness = gui;
        const std::weak_ptr<LUS::ControlDeck> deck_witness = deck;
        const std::weak_ptr<Ship::ResourceManager> resource_witness = resources;
        const std::weak_ptr<Ship::ThreadPool> pool_witness = pool;
        try {
            // Init can throw after SDL acquired a real context but before the
            // complete Window/renderer-ready boundary. Fence/clear those partial
            // graphics owners too, before the adapted backend destroys context.
            if (window && SDL_GL_GetCurrentContext()) {
                if (!finish_gpu) finish_gpu = reinterpret_cast<GlFinish>(SDL_GL_GetProcAddress("glFinish"));
                if (finish_gpu) finish_gpu();
                else fail(Error::graphics, "Partial OpenGL acquisition has no completion barrier");
                if (auto interpreter = window->GetInterpreterWeak().lock()) interpreter->TextureCacheClear();
            }
            pending = false;
            if (config_ready) {
                try { save_configuration(); }
                catch (const std::exception& e) { fail(Error::platform, e.what()); }
            }
            if (bridge) bridge->ClearCaches();
            if (window_ready) window->Close();
            if (window) window->GetMouseStateManager()->SetWindow(nullptr);
            if (gui) { gui->ShutDownImGui(window.get()); gui->release(); }
            if (deck) deck->ReleaseHostReferences();
            if (context) context->GetChildren().Remove(true);
            deck.reset(); window.reset(); gui.reset();
            if (resources) resources->ReleaseHostReferences();
            resources.reset(); pool.reset(); console.reset(); bridge.reset();
            variables.reset(); context.reset(); config.reset();
        } catch (const std::exception& e) { fail(Error::platform, e.what()); }
        catch (...) { fail(Error::platform, "Platform ownership retirement failed"); }
        // LUS's adapted backend destroys its context/window and calls SDL_Quit.
        // This also covers failures before a backend was acquired at all.
        if (sdl_owned) { SDL_Quit(); sdl_owned = false; }
        stopped = true;
        native_window = nullptr; native_window_id = 0;
        station_actions.store(0,std::memory_order_relaxed);
        closure.owners_released = window_witness.expired() && gui_witness.expired() && deck_witness.expired() &&
            resource_witness.expired() && pool_witness.expired();
        if (!closure.owners_released) fail(Error::platform, "Platform shutdown retained a LUS ownership cycle");
        closure.failed = error != Error::none;
        return closure;
    }
};

Host::Host(std::unique_ptr<Impl> impl) noexcept : impl_(std::move(impl)) {}
Host::~Host() = default;
CreateResult Host::create(const rom::Snapshot& snapshot, Config config) noexcept {
    CreateResult result;
    std::unique_ptr<Impl> impl;
    try {
        impl = std::make_unique<Impl>(std::move(config));
        impl->init(snapshot);
        result.host.reset(new Host(std::move(impl)));
        return result;
    } catch (const std::bad_alloc&) {
        if (impl) impl->fail(Error::allocation, "Platform allocation failed");
        result.error = impl ? impl->error : Error::allocation;
        text(result.message, impl ? impl->message.data() : "Platform allocation failed");
    } catch (const std::exception& e) {
        if (impl) impl->fail(Error::platform, e.what());
        result.error = impl ? impl->error : Error::platform;
        text(result.message, impl ? impl->message.data() : e.what());
    } catch (...) {
        if (impl) impl->fail(Error::platform, "Platform acquisition failed");
        result.error = impl ? impl->error : Error::platform;
        text(result.message, impl ? impl->message.data() : "Platform acquisition failed");
    }
    return result;
}
bool Host::running() const noexcept { return !impl_->stopped && impl_->error == Error::none && impl_->window && impl_->window->IsRunning(); }
Error Host::error() const noexcept { return impl_->error; }
const char* Host::message() const noexcept { return impl_->message.data(); }
const std::filesystem::path& Host::data_directory() const noexcept { return impl_->options.data_directory; }
game_audio::Session* Host::audio() noexcept { return impl_->audio_session.get(); }
const Output::Info& Host::audio_info() const noexcept { return impl_->output.info(); }
bool Host::pump_events() noexcept {
    if (impl_->stopped || impl_->error != Error::none) return false;
    try { impl_->window->HandleEvents(); return impl_->window->IsRunning(); }
    catch (const std::exception& e) { return impl_->fail(Error::platform, e.what()); }
    catch (...) { return impl_->fail(Error::platform, "LUS event processing failed"); }
}
bool Host::station_action_pending() const noexcept {
    return impl_->station_actions.load(std::memory_order_relaxed) != 0;
}
bool Host::take_station_action() noexcept {
    auto pending=impl_->station_actions.load(std::memory_order_relaxed);
    while (pending) {
        if (impl_->station_actions.compare_exchange_weak(pending,pending-1,std::memory_order_relaxed))
            return true;
    }
    return false;
}
bool Host::station_feedback(const char* status) noexcept {
    if (impl_->stopped || impl_->error != Error::none || !impl_->native_window || !status) return false;
    std::array<char,768> caption{};
    std::snprintf(caption.data(),caption.size(),"35mmIsle - Snap Station: %s",status);
    return SDL_SetWindowTitle(impl_->native_window,caption.data()) ||
        impl_->fail(Error::platform,SDL_GetError());
}
Input Host::acquire_input() noexcept {
    Input result;
    if (impl_->stopped || impl_->error != Error::none) return result;
    if (!impl_->deck) { result.completed = true; return result; }
    try {
        // Alternate actual acquisition destinations. If ControlDeck blocks and
        // returns before sampling, its GetPads pointer cannot match this buffer.
        auto& pads = impl_->pads[impl_->deck->GetPads() == impl_->pads[0].data() ? 1 : 0];
        pads = {};
        impl_->deck->WriteToPad(pads.data());
        if (impl_->deck->GetPads() != pads.data()) return result;
        // Optional HID discovery may set SDL error text while keyboard sampling
        // succeeds. GetPads is the completion signal; preserve actual per-port
        // errors below, rather than treating SDL's diagnostic string as status.
        result.controller_bits = impl_->controller_bits;
        for (std::size_t port = 0; port < game_input::port_count; ++port) {
            const auto controller = impl_->deck->GetControllerByPort(static_cast<std::uint8_t>(port));
            auto& sample = result.ports[port];
            sample.error = pads[port].err_no;
            if (controller && (impl_->controller_bits & (1u << port))) {
                // LUS aggregates keyboard/mouse/multiple gamepads into this
                // logical acquisition owner. Its actual address, NOT port+1 or
                // a fabricated physical SDL instance, is the endpoint identity.
                sample.connection = game_input::Connection::present;
                sample.identity = static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(controller.get()));
                sample.buttons = static_cast<std::uint16_t>(pads[port].button);
                sample.stick_x = pads[port].stick_x; sample.stick_y = pads[port].stick_y;
            }
        }
        result.completed = true;
    } catch (const std::exception& e) { impl_->fail(Error::input, e.what()); }
    catch (...) { impl_->fail(Error::input, "ControlDeck acquisition failed"); }
    return result;
}
bool Host::notify_retrace(std::uint64_t ordinal, std::uint64_t ns) noexcept {
    if (impl_->stopped || impl_->error != Error::none || !impl_->audio_session) return false;
    if (impl_->audio_session->retrace(ordinal, ns) != audio::Error::none) return impl_->audio_failure("Source audio retrace rejected");
    return true;
}
bool Host::pump_audio() noexcept { return impl_->pump_audio(); }
bool Host::stop_audio() noexcept {
    if (impl_->stopped || impl_->error != Error::none || !impl_->audio_session) return false;
    return impl_->audio_session->stop() == audio::Error::none || impl_->audio_failure("Source audio stop failed");
}
Drain Host::poll_audio_drain() noexcept {
    if (impl_->stopped || impl_->error != Error::none || !impl_->audio_session) return Drain::failed;
    const auto state = impl_->audio_session->state();
    if (state == game_audio::Session::State::drained) return Drain::drained;
    if (state == game_audio::Session::State::failed) { impl_->audio_failure("Source audio drain failed"); return Drain::failed; }
    if (state != game_audio::Session::State::draining) return Drain::pending;
    if (impl_->audio_session->poll_drain() != audio::Error::none) { impl_->audio_failure("Source audio stream drain failed"); return Drain::failed; }
    return impl_->audio_session->state() == game_audio::Session::State::drained ? Drain::drained : Drain::pending;
}
bool Host::configure_present(std::uint32_t new_width, std::uint32_t new_height) noexcept {
    if (impl_->stopped || impl_->error != Error::none || impl_->pending) return false;
    if (!((new_width == 320 && new_height == 240) || (new_width == 640 && new_height == 480)))
        return impl_->fail(Error::invalid, "Unsupported presentation extent");
    if (new_width == impl_->present_width && new_height == impl_->present_height) return true;
    try {
        const auto columns = new_width / tile_width, rows = new_height / tile_height;
        auto tiles = std::make_unique<Tile[]>(columns * rows);
        auto commands = std::make_unique<Gfx[]>(fixed_commands + columns * rows * commands_per_tile);
        const auto interpreter = impl_->window->GetInterpreterWeak().lock();
        require(bool(interpreter), "Presentation lost its LUS interpreter");
        // Fence and delete using OLD addresses/count before changing backing.
        impl_->invalidate_tiles();
        interpreter->SetNativeDimensions(float(new_width), float(new_height));
        impl_->tiles = std::move(tiles);
        impl_->commands = std::move(commands);
        impl_->present_width = new_width;
        impl_->present_height = new_height;
        impl_->tile_columns = columns;
        impl_->tile_rows = rows;
        return true;
    } catch (const std::bad_alloc&) { return impl_->fail(Error::allocation, "Presentation extent allocation failed"); }
    catch (const std::exception& e) { return impl_->fail(Error::graphics, e.what()); }
    catch (...) { return impl_->fail(Error::graphics, "Presentation extent configuration failed"); }
}
bool Host::queue_present(std::span<const std::byte> rgba, save::ScreenOffsets offsets) noexcept {
    return queue_present(rgba, impl_->present_width, impl_->present_height, offsets);
}
bool Host::queue_present(std::span<const std::byte> rgba, std::uint32_t surface_width,
                         std::uint32_t surface_height, save::ScreenOffsets offsets) noexcept {
    if (impl_->stopped || impl_->error != Error::none || impl_->pending) return false;
    if (surface_width != impl_->present_width || surface_height != impl_->present_height ||
        rgba.size() != std::size_t(impl_->present_width) * impl_->present_height * 4 ||
        offsets.horizontal < -76 || offsets.horizontal > 24 ||
        offsets.vertical < -36 || offsets.vertical > 14) return impl_->fail(Error::invalid, "Invalid native RGBA8 surface or VI offsets");
    try {
        impl_->invalidate_tiles();
        for (std::size_t row = 0; row < impl_->tile_rows; ++row)
            for (std::size_t column = 0; column < impl_->tile_columns; ++column)
                for (std::size_t y = 0; y < tile_height; ++y)
                    std::memcpy(impl_->tiles[row * impl_->tile_columns + column].bytes.data() + y * tile_width * 4,
                        rgba.data() + (row * tile_height + y) * impl_->present_width * 4 + column * tile_width * 4,
                        tile_width * 4);
        impl_->build_commands(offsets);
        impl_->pending = true;
        return true;
    } catch (const std::exception& e) { return impl_->fail(Error::graphics, e.what()); }
    catch (...) { return impl_->fail(Error::graphics, "RGBA8 presentation preparation failed"); }
}
bool Host::presentation_pending() const noexcept { return impl_->pending; }
Present Host::try_present() noexcept {
    if (impl_->stopped || impl_->error != Error::none) return Present::failed;
    if (!impl_->pending) return Present::idle;
    try {
        if (!impl_->window->IsRunning()) return cancel_present() ? Present::declined : Present::failed;
        if (!impl_->window->DrawAndRunGraphicsCommands(impl_->commands.get(), impl_->replacements)) return Present::declined;
        impl_->finish_gpu(); // LUS OpenGL FinishRender is not a completion fence.
        impl_->pending = false;
        return Present::completed;
    } catch (const std::exception& e) { impl_->fail(Error::graphics, e.what()); }
    catch (...) { impl_->fail(Error::graphics, "LUS presentation failed"); }
    return Present::failed;
}
bool Host::cancel_present() noexcept {
    if (impl_->stopped) return !impl_->pending;
    try { impl_->invalidate_tiles(); impl_->pending = false; return true; }
    catch (const std::exception& e) { return impl_->fail(Error::graphics, e.what()); }
    catch (...) { return impl_->fail(Error::graphics, "Presentation cancellation failed"); }
}
Shutdown Host::shutdown() noexcept { return impl_->shutdown(); }
} // namespace isle::game_host
