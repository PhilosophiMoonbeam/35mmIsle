// Original host integration; procedural by default, with an explicit bounded private-ROM experiment.
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
#include <ship/core/Context.h>
#include <ship/debug/Console.h>
#include <ship/resource/ResourceManager.h>
#include <ship/thread/ThreadPool.h>

#ifdef ISLE_HAS_GAME_ENGINE
#include "intro_probe.h"
#else
class IntroProbe;
#endif
#if defined(ISLE_HAS_GAME_ENGINE) || defined(ISLE_HAS_AUDIO_PROBE)
#include "rom_snapshot.hpp"
#endif
#ifdef ISLE_HAS_AUDIO_PROBE
#include "audio_host_probe.hpp"
#endif
#ifdef ISLE_HAS_CANONICAL_RDP
#include "rsp_render.hpp"
#endif

#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>

#if !defined(F3DEX_GBI_2) || defined(GBI_FLOATS)
#error "The procedural host fixture requires F3DEX_GBI_2 and integer vertices"
#endif

namespace {
// The base GUI and the Fast3d specialization cache distinct Window references.
// Clear both only after the final frame and after ImGui has released GPU objects.
class ProbeGui final : public Fast::Fast3dGui {
  public:
    explicit ProbeGui(bool enableInput) : inputEnabled(enableInput) {}
    void ReleaseReferences() {
        Ship::Gui::ReleaseHostReferences();
        mWindow.reset();
        mResourceManager.reset();
        mConsoleVariables.reset();
    }

  protected:
    void ImGuiWMInit() override {
        Fast::Fast3dGui::ImGuiWMInit();
        if (!inputEnabled)
            ImGui_ImplSDL3_SetGamepadMode(ImGui_ImplSDL3_GamepadMode_Manual, nullptr, 0);
        platformReady = true;
    }
    void ImGuiBackendInit() override {
        Fast::Fast3dGui::ImGuiBackendInit();
        // The pinned wrapper discards ImGui_ImplOpenGL3_Init's boolean result.
        // Failed loader initialization has no renderer data and must not be shut down.
        if (ImGui::GetIO().BackendRendererUserData == nullptr) {
            throw std::runtime_error("ImGui OpenGL renderer initialization failed");
        }
        rendererReady = true;
    }
    void ImGuiWMShutdown() override {
        if (platformReady) {
            Fast::Fast3dGui::ImGuiWMShutdown();
            platformReady = false;
        }
    }
    void ImGuiBackendShutdown() override {
        if (rendererReady) {
            Fast::Fast3dGui::ImGuiBackendShutdown();
            rendererReady = false;
        }
    }

  private:
    const bool inputEnabled;
    bool platformReady = false;
    bool rendererReady = false;
};

class SdlLifetime final {
  public:
    explicit SdlLifetime(bool enableInput) {
        if (!SDL_Init(SDL_INIT_VIDEO | (enableInput ? SDL_INIT_GAMEPAD : 0u))) {
            throw std::runtime_error(std::string("SDL3 requested subsystems unavailable: ") + SDL_GetError());
        }
        // Early diagnostic only: the private adaptation separately rejects failures
        // when LUS creates its actual SDL window and OpenGL context.
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_Window* check = SDL_CreateWindow("35mmIsle OpenGL qualification", 64, 64,
                                             SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
        if (check == nullptr) {
            const std::string error = SDL_GetError();
            SDL_Quit();
            throw std::runtime_error("SDL3/OpenGL window unavailable: " + error);
        }
        SDL_GLContext gl = SDL_GL_CreateContext(check);
        const bool usable = gl != nullptr && SDL_GL_MakeCurrent(check, gl) &&
                            SDL_GL_GetProcAddress("glCreateShader") != nullptr;
        const std::string error = SDL_GetError();
        if (gl != nullptr) {
            SDL_GL_DestroyContext(gl);
        }
        SDL_DestroyWindow(check);
        if (!usable) {
            SDL_Quit();
            throw std::runtime_error("A real shader-capable OpenGL context is unavailable: " + error);
        }
    }
    ~SdlLifetime() { SDL_Quit(); }
    SdlLifetime(const SdlLifetime&) = delete;
    SdlLifetime& operator=(const SdlLifetime&) = delete;
};

class ProbeHost final {
  public:
    ProbeHost() = default;
    ProbeHost(const ProbeHost&) = delete;
    ProbeHost& operator=(const ProbeHost&) = delete;

    void Init(bool introMode, bool enableInput) {
        context = Ship::Context::CreateInstance("35mmIsleHostProbe", "35mmIsle nonplayable host probe");
        context->Init();
        const std::filesystem::path data =
            Ship::Context::GetAppDirectoryPath("35mmIsleHostProbe");
        std::filesystem::create_directories(data);
        const std::filesystem::path rendererResources = ISLE_HOST_RENDERER_RESOURCES;
        if (!std::filesystem::is_regular_file(rendererResources / "shaders/opengl/default.shader.glsl")) {
            throw std::runtime_error("Pinned LUS OpenGL renderer shader resource is missing");
        }
        config = std::make_shared<Ship::Config>((data / "host-probe.json").string());
        context->GetChildren().Add(config);
        config->SetInt("Window.Backend.Id", Fast::FAST3D_SDL_OPENGL);
        config->SetInt("Window.Width", 640);
        config->SetInt("Window.Height", 480);
        config->SetBool("Window.Fullscreen.Enabled", false);
        variables = std::make_shared<Ship::ConsoleVariable>(config);
        context->GetChildren().Add(variables);
        variables->SetInteger(CVAR_ENABLE_MULTI_VIEWPORTS, 0);
        pool = std::make_shared<Ship::ThreadPool>(1);
        context->GetChildren().Add(pool);
        resources = std::make_shared<Ship::ResourceManager>(pool);
        context->GetChildren().Add(resources);
        // Real pinned LUS renderer support resources, never game assets.
        resources->Init(nlohmann::json{
            {"archivePaths", std::vector<std::string>{rendererResources.string()}},
            {"validHashes", std::vector<std::uint32_t>{}}});
        console = std::make_shared<Ship::Console>();
        context->GetChildren().Add(console);
        console->Init();
        gui = std::make_shared<ProbeGui>(enableInput);
        window = std::make_shared<Fast::Fast3dWindow>(gui, config, variables);
        context->GetChildren().Add(window);
        if (enableInput) {
            deck = std::make_shared<LUS::ControlDeck>(window, variables);
            context->GetChildren().Add(deck);
        }
        bridge = std::make_shared<LUS::UltraBridge>();
        context->GetChildren().Add(bridge);
        window->Init();
        windowReady = true;
        // This probe does not import files; the SDL backend's FileDrop is absent.
        SDL_SetEventEnabled(SDL_EVENT_DROP_FILE, false);
        if (deck) deck->Init(&controllerBits);
        else {
            // Fast3dWindow installs handlers requiring ControlDeck. Disable only
            // those event types; window, resize and quit processing remain live.
            for (const auto type : {SDL_EVENT_KEY_DOWN, SDL_EVENT_KEY_UP,
                                    SDL_EVENT_MOUSE_BUTTON_DOWN, SDL_EVENT_MOUSE_BUTTON_UP})
                SDL_SetEventEnabled(type, false);
            std::cout << "INPUT_ENUMERATION_DISABLED: explicit input-disabled diagnostic; "
                         "no SDL gamepad subsystem, ControlDeck initialization, input callbacks or ImGui auto-enumeration.\n";
        }
        bridge->UpdateCaches(context);
        window->SetRendererUCode(ucode_f3dex2);
        window->SetTargetFps(60);
        window->SetMaximumFrameLatency(1);
        auto interpreter = window->GetInterpreterWeak().lock();
        if (!interpreter || interpreter->GetCurrentRenderingAPI() == nullptr ||
            SDL_GL_GetCurrentContext() == nullptr || !window->IsRunning()) {
            throw std::runtime_error("LUS did not establish a running SDL3/OpenGL renderer");
        }
        // The pinned OpenGL FinishRender() is empty. A real GL completion
        // barrier is required before borrowed intro texels may be released.
        finishGpu = reinterpret_cast<GlFinish>(SDL_GL_GetProcAddress("glFinish"));
        if (finishGpu == nullptr) {
            throw std::runtime_error("OpenGL completion barrier is unavailable");
        }
        interpreter->SetNativeDimensions(320.0f, 240.0f);
        std::cout << "NONPLAYABLE " << (introMode ? "bounded intro-asset" : "procedural host")
                  << " probe: real LUS 759b13401cd6903daa67a00a753066f791622e78; backend="
                  << interpreter->GetCurrentRenderingAPI()->GetName()
                  << "; F3DEX_GBI_2 / ucode_f3dex2; pinned renderer shaders; "
                  << (introMode ? "explicit private-ROM sprite/C-fade scope only, not a full scene\n"
                                : "procedural graphics, not a game scheduler\n");
    }
#ifdef ISLE_HAS_AUDIO_PROBE
    void EnableAudio(const isle::rom::Snapshot& snapshot) {
        audio = std::make_unique<AudioHostProbe>(snapshot);
    }
    void FinishAudio() {
        if (audio) audio->Finish();
    }
#endif

#ifdef ISLE_HAS_CANONICAL_RDP
    void EnableRequiredFeedback(bool requireCpu, bool decline) {
        requiredExecutor.emplace(requireCpu);
        declinePresentations = decline;
        std::cout << "REQUIRED_FEEDBACK_ENABLED: one persistent canonical executor; "
                     "fixed required slot independent of optional visual leases.\n";
        if (decline)
            std::cout << "OPTIONAL_PRESENTATIONS_DECLINED: explicit diagnostic policy retains both leases; "
                         "not a claim that LUS returned false.\n";
    }
#endif
#ifdef ISLE_HAS_GAME_ENGINE
    void RunIntro(std::uint32_t frameLimit, IntroProbe& intro) {
        std::array<std::optional<IntroProbe::Frame>, 2> pending;
        const std::unordered_map<Mtx*, MtxF> replacements;
        const bool feedbackDiagnostic = intro.FeedbackDiagnostic();
        const auto deadline = std::chrono::steady_clock::now() +
                              std::chrono::seconds(20 + frameLimit / 10);
        auto retire = [&intro](std::optional<IntroProbe::Frame>& frame, isle::host::Terminal terminal) {
            if (intro.CompleteFrame(frame->token, terminal) != isle::host::ClockError::none)
                throw std::logic_error("Intro consumer terminal rejected");
            frame.reset();
        };
        auto quiesce = [&] {
            intro.RequestStop();
            finishGpu(); // Do not retire a borrowed list while GL may still access it.
            for (auto& frame : pending) if (frame) retire(frame, isle::host::Terminal::cancelled);
            intro.StopGame(); // Stops C users on their creator, not this SDL thread.
            #ifdef ISLE_HAS_CANONICAL_RDP
            RetireRequired(intro); // CPU accesses ceased; release main lease before capacity.
            if (!intro.RequiredQuiescent() || requiredMemory || requiredToken)
                throw std::logic_error("Required feedback did not quiesce before renderer shutdown");
            #endif
        };
        std::uint32_t submitted = 0;
        intro.Start(); // Asset admission and SDL/audio initialization precede VI time.
        try {
            while (feedbackDiagnostic ? intro.CompletedFeedbackCycles() < frameLimit : submitted < frameLimit) {
                window->HandleEvents();
                if (!window->IsRunning()) throw std::runtime_error("Window closed before intro completed");
                if (std::chrono::steady_clock::now() >= deadline)
                    throw std::runtime_error("Intro renderer exceeded bounded deadline");
                intro.Check();
#ifdef ISLE_HAS_CANONICAL_RDP
                ServiceRequired(intro); // Required work never queues behind optional presentation.
#endif
#ifdef ISLE_HAS_AUDIO_PROBE
                if (audio) audio->Tick();
#endif
                if (deck) deck->WriteToPad(pads.data());
                IntroProbe::Frame frame;
                while (intro.TakeFrame(frame)) {
                    auto& slot = pending[frame.token.context];
                    if (slot) throw std::logic_error("Intro context reused before terminal");
                    slot = frame;
                }
                // Context indices are reusable storage, not submission order.
                std::optional<IntroProbe::Frame>* next = pending[0] ? &pending[0] : nullptr;
                if (pending[1] && (next == nullptr || pending[1]->token.serial < (*next)->token.serial))
                    next = &pending[1];
                if (!declinePresentations && next && window->DrawAndRunGraphicsCommands((*next)->commands, replacements)) {
                    finishGpu(); // Pinned LUS FinishRender is empty; FullSync is not a GPU fence.
                    retire(*next, isle::host::Terminal::completed);
                    ++submitted;
                    if (submitted == 1)
                        std::cout << "FIRST_SUCCESSFUL_RENDERER_FRAME: immutable intro list/context rendered; "
                                     "GL completion acknowledged, not pixel/fidelity proof.\n" << std::flush;
                } else {
                    // A declined presentation retains its exact lease and bytes.
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }
        } catch (...) {
            quiesce();
            throw;
        }
        quiesce();
        const auto stats = intro.Stats();
        std::cout << "GTL_NATIVE_CLOCK: documented NTSC progressive 2250000/37609 Hz; "
                  << "retraces=" << stats.retraces << "; dropped=" << stats.dropped_retraces
                  << "; updates=" << stats.updates << "; admissions=" << stats.submissions
                  << "; terminals=" << stats.terminals << "; skipped=" << stats.skipped_draws << '\n'
                  << "Submitted " << submitted << " real-renderer intro-asset frames; "
                  << intro.Ticks() << " independently clocked C updates.\n"
                     "Bounded native GTL/asset experiment; not scene boot, physical timing or full fidelity.\n";
        #ifdef ISLE_HAS_CANONICAL_RDP
        if (feedbackDiagnostic) {
            const auto requiredStats = intro.RequiredStats();
            if (submitted != 0 || stats.submissions != 2 || stats.terminals != 2 ||
                requiredStats.completed != frameLimit || requiredStats.submissions != frameLimit ||
                requiredStats.retirements != frameLimit || requiredStats.failed != 0 ||
                requiredStats.cancelled != 0 || !intro.RequiredQuiescent())
                throw std::logic_error("Required-feedback diagnostic violated its end-to-end acceptance contract");
            std::cout << "REQUIRED_FEEDBACK_COMPLETE: cpu_consumed=" << intro.CompletedFeedbackCycles()
                      << "; gpu_ready=" << requiredStats.gpu_ready << "; cpu_borrows=" << requiredStats.cpu_borrows
                      << "; real_backend_leases_retired=" << requiredStats.retirements
                      << "; visual_leases_retained=2; visual_submissions=0; required_quiescent=true.\n";
        }
        #endif
    }
#endif
    void Run(std::uint32_t frameLimit, [[maybe_unused]] IntroProbe* intro = nullptr) {
#ifdef ISLE_HAS_GAME_ENGINE
        if (intro != nullptr) {
            RunIntro(frameLimit, *intro);
            return;
        }
#endif
        std::array<Gfx, 32> commands{};
        const std::unordered_map<Mtx*, MtxF> replacements;
        const auto deadline = std::chrono::steady_clock::now() +
                              std::chrono::seconds(20 + frameLimit / 10);
        std::uint32_t submitted = 0;
        while (submitted < frameLimit) {
            window->HandleEvents();
            if (!window->IsRunning()) {
                throw std::runtime_error("Window closed before the requested frame count completed");
            }
            if (std::chrono::steady_clock::now() >= deadline) {
                throw std::runtime_error("Renderer did not submit the requested frames before the bounded deadline");
            }
#ifdef ISLE_HAS_AUDIO_PROBE
            if (audio) audio->Tick();
#endif
            if (deck) deck->WriteToPad(pads.data());
            Gfx* out = commands.data();
            gDPPipeSync(out++);
            gDPSetScissor(out++, G_SC_NON_INTERLACE, 0, 0, 320, 240);
            gDPSetCycleType(out++, G_CYC_FILL);
            gDPSetColorImage(out++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 320, 0x00100000);
            gDPSetDepthImage(out++, 0x00200000);
            auto rectangle = [&out](std::uint16_t rgba, int left, int top, int right, int bottom) {
                const std::uint32_t fill = (static_cast<std::uint32_t>(rgba) << 16) | rgba;
                gDPSetFillColor(out++, fill);
                gDPFillRectangle(out++, left, top, right, bottom);
                gDPPipeSync(out++);
            };
            // Asymmetric original geometry, not a black placeholder or game image.
            rectangle(0x084b, 0, 0, 319, 239);       // dark blue background
            rectangle(0xf801, 16, 16, 91, 63);       // red upper left
            rectangle(0x07c1, 228, 25, 299, 104);     // green upper right
            rectangle(0xffc1, 40, 158, 127, 215);     // yellow lower left
            const int x = 136 + static_cast<int>(submitted % 72);
            rectangle(0x07ff, x, 112, x + 39, 143);  // moving cyan rectangle
            gDPFullSync(out++);
            gSPEndDisplayList(out++);
            if (window->DrawAndRunGraphicsCommands(commands.data(), replacements)) {
                ++submitted;
                if (submitted == 1) {
                    std::cout << "FIRST_SUCCESSFUL_RENDERER_FRAME: procedural display list submitted; "
                                 "observe colored regions and moving cyan, not a game.\n" << std::flush;
                }
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
        std::cout << "Submitted " << submitted
                  << " real-renderer frames; observe red/green/yellow geometry and moving cyan on dark blue.\n"
                     "This is host integration evidence only, not game/render-fidelity evidence.\n";
    }

    void Shutdown() {
        if (stopped) {
            return;
        }
#ifdef ISLE_HAS_CANONICAL_RDP
        if (requiredMemory || requiredToken || (requiredExecutor && !requiredExecutor->quiescent()))
            throw std::logic_error("Canonical executor still has a required memory lease during shutdown");
        requiredExecutor.reset();
#endif
        bool audioOk = true;
#ifdef ISLE_HAS_AUDIO_PROBE
        if (audio) {
            audioOk = audio->Abort();
            audio.reset();
        }
#endif
        const std::weak_ptr<Fast::Fast3dWindow> windowWitness = window;
        const std::weak_ptr<ProbeGui> guiWitness = gui;
        const std::weak_ptr<LUS::ControlDeck> deckWitness = deck;
        const std::weak_ptr<Ship::ResourceManager> resourcesWitness = resources;
        const std::weak_ptr<Ship::ThreadPool> poolWitness = pool;
        // Shader loads use synchronous LoadResource(...).get(); no resource work
        // or host submission remains outstanding once Run has returned.
        // Keep owners alive until all caches and backend references are unbound.
        if (finishGpu != nullptr && SDL_GL_GetCurrentContext() != nullptr) {
            finishGpu();
        }
        if (windowReady) {
            if (auto interpreter = window->GetInterpreterWeak().lock()) {
                interpreter->TextureCacheClear();
            }
        }
        if (bridge) {
            bridge->ClearCaches();
        }
        if (windowReady) {
            window->SaveWindowToConfig();
            window->Close();
        }
        if (window) {
            window->GetMouseStateManager()->SetWindow(nullptr);
        }
        if (gui) {
            gui->ShutDownImGui(window.get());
            gui->ReleaseReferences();
        }
        if (deck) {
            deck->ReleaseHostReferences();
        }
        if (context) {
            context->GetChildren().Remove(true);
        }
        deck.reset();
        window.reset();
        gui.reset();
        if (resources) {
            resources->ReleaseHostReferences();
        }
        resources.reset();
        pool.reset();
        console.reset();
        bridge.reset();
        variables.reset();
        context.reset();
        config.reset();
        stopped = true;
        if (!windowWitness.expired() || !guiWitness.expired() || !deckWitness.expired() ||
            !resourcesWitness.expired() || !poolWitness.expired()) {
            throw std::runtime_error("Host shutdown retained a window, GUI, deck, resource manager or worker-pool owner");
        }
        if (!audioOk) throw std::runtime_error("Checked audio aborted with an observed endpoint or lease error");
        std::cout << "HOST_OWNERS_RELEASED: window, GUI, deck, resources and worker pool destroyed before process exit.\n"
                  << std::flush;
    }

    ~ProbeHost() {
        try {
            Shutdown();
        } catch (const std::exception& error) {
            std::cerr << "Host unwind cleanup failed: " << error.what() << '\n';
        }
    }

  private:
#ifdef ISLE_HAS_CANONICAL_RDP
    void RetireRequired(IntroProbe& intro) {
        isle::host::RequiredRetirement retirement;
        while (intro.TakeRequiredRetirement(retirement)) {
            if (requiredToken && *requiredToken != retirement.token)
                throw std::logic_error("Required retirement has a foreign backend lease");
            if (static_cast<bool>(requiredMemory) != requiredToken.has_value() ||
                (!requiredToken && retirement.outcome == isle::host::Terminal::completed))
                throw std::logic_error("Required retirement has no matching actual memory ownership");
            requiredMemory.release();
            requiredToken.reset();
            if (intro.FinishRequiredRetirement(retirement.token) != isle::host::ClockError::none)
                throw std::logic_error("Required retirement rejected after actual lease release");
        }
    }
    void ServiceRequired(IntroProbe& intro) {
        RetireRequired(intro);
        isle::host::RequiredRequest request;
        if (!intro.TakeRequired(request)) return;
        if (!requiredExecutor || requiredMemory || requiredToken)
            throw std::logic_error("Required executor missing or reused before actual retirement");
        try {
            requiredMemory = requiredExecutor->execute(*request.capture);
            requiredToken = request.token;
        } catch (...) {
            // execute drains/resets failed backend access before publishing failure.
            if (intro.FailRequired(request.token) != isle::host::ClockError::none) std::abort();
            throw;
        }
        if (intro.PublishRequired(request.token, {requiredMemory.rdram(), requiredMemory.hidden()}) !=
            isle::host::ClockError::none)
            throw std::logic_error("Canonical memory readiness publication rejected");
    }
#endif
#ifdef _WIN32
    using GlFinish = void (__stdcall*)();
#else
    using GlFinish = void (*)();
#endif
    GlFinish finishGpu = nullptr;
    std::shared_ptr<Ship::Context> context;
    std::shared_ptr<Ship::Config> config;
    std::shared_ptr<Ship::ConsoleVariable> variables;
    std::shared_ptr<Ship::ThreadPool> pool;
    std::shared_ptr<Ship::ResourceManager> resources;
    std::shared_ptr<Ship::Console> console;
    std::shared_ptr<ProbeGui> gui;
    std::shared_ptr<Fast::Fast3dWindow> window;
    std::shared_ptr<LUS::ControlDeck> deck;
    std::shared_ptr<LUS::UltraBridge> bridge;
#ifdef ISLE_HAS_AUDIO_PROBE
    std::unique_ptr<AudioHostProbe> audio;
#endif
#ifdef ISLE_HAS_CANONICAL_RDP
    std::optional<isle::rsp::probe::CanonicalCaptureExecutor> requiredExecutor;
    isle::rsp::probe::CanonicalCaptureExecutor::MemoryLease requiredMemory;
    std::optional<isle::host::RequiredToken> requiredToken;
#endif
    bool declinePresentations = false;
    std::array<OSContPad, MAXCONTROLLERS> pads{};
    std::uint8_t controllerBits = 0;
    bool windowReady = false;
    bool stopped = false;
};

#ifdef _WIN32
using Argument = wchar_t;
constexpr std::wstring_view framesArgument = L"--frames";
constexpr std::wstring_view romArgument = L"--rom";
constexpr std::wstring_view audioArgument = L"--audio";
constexpr std::wstring_view noInputArgument = L"--no-input";
constexpr std::wstring_view requiredArgument = L"--required-feedback-probe";
constexpr std::wstring_view declineArgument = L"--decline-presentations";
constexpr std::wstring_view cpuArgument = L"--cpu-device";
constexpr std::wstring_view helpArgument = L"--help";
#else
using Argument = char;
constexpr std::string_view framesArgument = "--frames";
constexpr std::string_view romArgument = "--rom";
constexpr std::string_view audioArgument = "--audio";
constexpr std::string_view noInputArgument = "--no-input";
constexpr std::string_view requiredArgument = "--required-feedback-probe";
constexpr std::string_view declineArgument = "--decline-presentations";
constexpr std::string_view cpuArgument = "--cpu-device";
constexpr std::string_view helpArgument = "--help";
#endif

struct Options {
    std::uint32_t frames = 120;
    std::optional<std::filesystem::path> rom;
    bool audio = false;
    bool noInput = false;
    bool requiredFeedback = false, declinePresentations = false, requireCpu = false;
};
void Usage(std::ostream& output) {
    output << "35mmisle-host-probe [--frames 1..3600] [--rom PRIVATE_ROM] [--audio] [--no-input]\n"
              "  [--required-feedback-probe --decline-presentations [--cpu-device]]\n"
              "--rom: one admitted z64/v64/n64 snapshot for compiled private consumers.\n"
              "--no-input: disable controller enumeration and keyboard/button callbacks;\n"
              "real presentation remains enabled. Audio and graphics drivers are independent.\n"
              "ISLE_BUILD_GAME_ENGINE enables the bounded original intro sprite/C fade.\n"
              "ISLE_BUILD_AUDIO_PROBE + --audio enables independently queue-paced native SFX14.\n"
              "--audio requires --rom; no Ship::Audio/legacy-bridge/AI/VI/cue fidelity claim.\n"
              "Checked SDL stream drain is not physical-device/hearing completion.\n"
              "Required-feedback mode needs compiled native engine/RSP/canonical RDP and --rom;\n"
              "--frames counts owner-domain CPU consumptions, not optional presentations.\n"
              "It deliberately retains two visual leases and disables physical controller enumeration;\n"
              "actual canonical mapped pixels/hidden memory feed dependent diagnostic operations,\n"
              "not GAME photo scoring, scene boot, a prerecorded image or a Null backend.\n"
              "Neither mode is full native game boot. No implicit ROM search or Null fallback.\n";
}

Options ParseOptions(int argc, Argument* argv[]) {
    Options options;
    bool haveFrames = false;
    for (int i = 1; i < argc; ++i) {
        const std::basic_string_view<Argument> argument(argv[i]);
        if (argument == framesArgument && !haveFrames && i + 1 < argc) {
            const std::basic_string_view<Argument> text(argv[++i]);
            std::uint32_t count = 0;
            if (text.empty()) {
                throw std::invalid_argument("--frames must be an integer in 1..3600");
            }
            for (const Argument digit : text) {
                if (digit < '0' || digit > '9' || count > 3600) {
                    throw std::invalid_argument("--frames must be an integer in 1..3600");
                }
                count = count * 10 + static_cast<std::uint32_t>(digit - '0');
            }
            if (count == 0 || count > 3600) {
                throw std::invalid_argument("--frames must be an integer in 1..3600");
            }
            options.frames = count;
            haveFrames = true;
        } else if (argument == romArgument && !options.rom && i + 1 < argc) {
#if defined(ISLE_HAS_GAME_ENGINE) || defined(ISLE_HAS_AUDIO_PROBE)
            const std::basic_string_view<Argument> text(argv[++i]);
            if (text.empty() || (text.size() >= 2 && text[0] == '-' && text[1] == '-'))
                throw std::invalid_argument("--rom requires a native path, not another flag");
            options.rom.emplace(text);
#else
            throw std::invalid_argument("--rom requires a compiled private intro or audio consumer");
#endif
        } else if (argument == audioArgument && !options.audio) {
#ifdef ISLE_HAS_AUDIO_PROBE
            options.audio = true;
#else
            throw std::invalid_argument("--audio requires ISLE_BUILD_AUDIO_PROBE");
#endif
        } else if (argument == noInputArgument && !options.noInput) {
            options.noInput = true;
        } else if (argument == requiredArgument && !options.requiredFeedback) {
#ifdef ISLE_HAS_CANONICAL_RDP
            options.requiredFeedback = true;
#else
            throw std::invalid_argument("--required-feedback-probe requires compiled native engine/RSP/canonical RDP");
#endif
        } else if (argument == declineArgument && !options.declinePresentations) {
            options.declinePresentations = true;
        } else if (argument == cpuArgument && !options.requireCpu) {
            options.requireCpu = true;
        } else {
            throw std::invalid_argument("Unknown, repeated or incomplete host argument; use --help");
        }
    }
    if (options.audio && !options.rom) throw std::invalid_argument("--audio requires --rom");
    if (options.requiredFeedback && (!options.rom || !options.declinePresentations))
        throw std::invalid_argument("--required-feedback-probe requires --rom and --decline-presentations");
    if ((options.declinePresentations || options.requireCpu) && !options.requiredFeedback)
        throw std::invalid_argument("--decline-presentations/--cpu-device require --required-feedback-probe");
#if !defined(ISLE_HAS_GAME_ENGINE) && defined(ISLE_HAS_AUDIO_PROBE)
    if (options.rom && !options.audio) throw std::invalid_argument("This composition requires --audio with --rom");
#endif
    return options;
}
} // namespace

int Run(int argc, Argument* argv[]) {
    try {
        if (argc == 2 && std::basic_string_view<Argument>(argv[1]) == helpArgument) {
            Usage(std::cout);
            std::cout.flush();
            return std::cout ? 0 : 1;
        }
        const Options options = ParseOptions(argc, argv);
        const bool inputEnabled = !options.noInput && !options.requiredFeedback;
        SdlLifetime sdl(inputEnabled);
#if defined(ISLE_HAS_GAME_ENGINE) || defined(ISLE_HAS_AUDIO_PROBE)
        std::optional<isle::rom::Snapshot> snapshot;
        if (options.rom) snapshot.emplace(isle::rom::read_snapshot(*options.rom));
#endif
#ifdef ISLE_HAS_GAME_ENGINE
        // Declared before the host: unwind destroys the renderer before these
        // borrowed CPU owners, even if frame submission or shutdown throws.
        std::unique_ptr<IntroProbe> intro;
        if (options.rom) {
            intro = std::make_unique<IntroProbe>(*snapshot, options.requiredFeedback ? options.frames : 0u);
        }
#endif
        ProbeHost host;
#ifdef ISLE_HAS_GAME_ENGINE
        host.Init(intro != nullptr, inputEnabled);
#else
        host.Init(false, inputEnabled);
#endif
#ifdef ISLE_HAS_CANONICAL_RDP
        if (options.requiredFeedback)
            host.EnableRequiredFeedback(options.requireCpu, options.declinePresentations);
#endif
#ifdef ISLE_HAS_AUDIO_PROBE
        if (options.audio) host.EnableAudio(*snapshot);
#endif
#if defined(ISLE_HAS_GAME_ENGINE) || defined(ISLE_HAS_AUDIO_PROBE)
        if (snapshot) {
            snapshot.reset();
            std::cout << "PRIVATE_ROM_SNAPSHOT_RELEASED: one admission/normalization; typed consumers own their resources.\n"
                      << std::flush;
        }
#endif
        try {
#ifdef ISLE_HAS_GAME_ENGINE
            host.Run(options.frames, intro.get());
#else
            host.Run(options.frames);
#endif
#ifdef ISLE_HAS_AUDIO_PROBE
            host.FinishAudio();
#endif
        } catch (...) {
#ifdef ISLE_HAS_GAME_ENGINE
            if (intro) {
                intro->StopGame();
            }
#endif
            throw;
        }
#ifdef ISLE_HAS_GAME_ENGINE
        if (intro) {
            intro->StopGame();
        }
#endif
        host.Shutdown();
#ifdef ISLE_HAS_GAME_ENGINE
        if (intro) {
            intro.reset();
            std::cout << "INTRO_ASSET_OWNERS_RELEASED: C users joined, GL completion barrier passed, "
                         "renderer/cache destroyed, then arena/bank/pixels/display lists released.\n" << std::flush;
        }
#endif
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Nonplayable host probe failed: " << error.what() << '\n';
        return 1;
    }
}

#ifdef _WIN32
int wmain(int argc, wchar_t* argv[]) {
#else
int main(int argc, char* argv[]) {
#endif
    return Run(argc, argv);
}
