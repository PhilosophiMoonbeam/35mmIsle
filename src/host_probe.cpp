// Original procedural host integration; deliberately not a Pokémon Snap executable.
#include <SDL3/SDL.h>
#include <imgui.h>
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

#include <array>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
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
    void ReleaseReferences() {
        Ship::Gui::ReleaseHostReferences();
        mWindow.reset();
        mResourceManager.reset();
        mConsoleVariables.reset();
    }

  protected:
    void ImGuiWMInit() override {
        Fast::Fast3dGui::ImGuiWMInit();
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
    bool platformReady = false;
    bool rendererReady = false;
};

class SdlLifetime final {
  public:
    SdlLifetime() {
        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
            throw std::runtime_error(std::string("SDL3 video/gamepad unavailable: ") + SDL_GetError());
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

    void Init() {
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
        gui = std::make_shared<ProbeGui>();
        window = std::make_shared<Fast::Fast3dWindow>(gui, config, variables);
        context->GetChildren().Add(window);
        deck = std::make_shared<LUS::ControlDeck>(window, variables);
        context->GetChildren().Add(deck);
        bridge = std::make_shared<LUS::UltraBridge>();
        context->GetChildren().Add(bridge);
        window->Init();
        windowReady = true;
        // This probe does not import files; the SDL backend's FileDrop is absent.
        SDL_SetEventEnabled(SDL_EVENT_DROP_FILE, false);
        deck->Init(&controllerBits);
        bridge->UpdateCaches(context);
        window->SetRendererUCode(ucode_f3dex2);
        window->SetTargetFps(60);
        window->SetMaximumFrameLatency(1);
        auto interpreter = window->GetInterpreterWeak().lock();
        if (!interpreter || interpreter->GetCurrentRenderingAPI() == nullptr ||
            SDL_GL_GetCurrentContext() == nullptr || !window->IsRunning()) {
            throw std::runtime_error("LUS did not establish a running SDL3/OpenGL renderer");
        }
        interpreter->SetNativeDimensions(320.0f, 240.0f);
        std::cout << "NONPLAYABLE procedural host probe: real LUS "
                     "759b13401cd6903daa67a00a753066f791622e78; backend="
                  << interpreter->GetCurrentRenderingAPI()->GetName()
                  << "; F3DEX_GBI_2 / ucode_f3dex2; pinned renderer shaders only; no game assets, audio or game scheduler\n";
    }

    void Run(std::uint32_t frameLimit) {
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
            deck->WriteToPad(pads.data());
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
        const std::weak_ptr<Fast::Fast3dWindow> windowWitness = window;
        const std::weak_ptr<ProbeGui> guiWitness = gui;
        const std::weak_ptr<LUS::ControlDeck> deckWitness = deck;
        const std::weak_ptr<Ship::ResourceManager> resourcesWitness = resources;
        const std::weak_ptr<Ship::ThreadPool> poolWitness = pool;
        // Shader loads use synchronous LoadResource(...).get(); no resource work
        // or host submission remains outstanding once Run has returned.
        // Keep owners alive until all caches and backend references are unbound.
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
    std::array<OSContPad, MAXCONTROLLERS> pads{};
    std::uint8_t controllerBits = 0;
    bool windowReady = false;
    bool stopped = false;
};

std::uint32_t ParseFrames(int argc, char** argv) {
    if (argc == 1) {
        return 120;
    }
    if (argc != 3 || std::string_view(argv[1]) != "--frames") {
        throw std::invalid_argument("Usage: 35mmisle-host-probe [--frames 1..3600]");
    }
    const std::string_view text(argv[2]);
    std::uint32_t count = 0;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), count);
    if (result.ec != std::errc{} || result.ptr != text.data() + text.size() || count == 0 || count > 3600) {
        throw std::invalid_argument("--frames must be an integer in 1..3600");
    }
    return count;
}
} // namespace

int main(int argc, char** argv) {
    try {
        const std::uint32_t frames = ParseFrames(argc, argv);
        SdlLifetime sdl;
        ProbeHost host;
        host.Init();
        host.Run(frames);
        host.Shutdown();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Nonplayable host probe failed: " << error.what() << '\n';
        return 1;
    }
}
