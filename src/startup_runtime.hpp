#ifndef ISLE_STARTUP_RUNTIME_HPP
#define ISLE_STARTUP_RUNTIME_HPP

#include "menu_ui_scene.hpp"
#include "scene_storage.hpp"

namespace isle::startup {
enum class Error { none, invalid, allocation, native, input, audio, profile, retired };
enum class Phase { logo, title_menu, scene_exit, retired, failed };
enum class Step { updated, scene_changed, scene_exit, failed };
struct Exit {
    menu::scene::SourceScene scene = menu::scene::SourceScene::invalid;
    std::uint32_t entry_flags = 0;
};
// A preparation-only view, never an asynchronous graphics lease. The parent
// must complete/retire every RSP/RDP access and CPU readback before tick(), a
// scene transition or destruction. Optional presentation owns copied pixels.
struct View {
    menu::Scene resources = menu::Scene::logo;
    IsleGame* game = nullptr;
    IsleGameCamera* sprite_camera = nullptr;
    IsleGameSprite* logo_sprite = nullptr;
    scene::Services* services = nullptr;
    menu::scene::Title* title = nullptr;
    std::uint32_t background_rgba = 0;
    save::ScreenOffsets screen_offsets{};
};
struct CreateResult;
// Complete source AA0650 logo and A08E30/A0BE70 title/menu scene owner. External
// scene IDs/entry flags are returned to the game scene manager, not converted
// into fake gameplay or process completion. Catalog, input, audio, persistent
// State and application Storage are borrowed stable owners. Only boot logo's
// continuation initializes State after its first real wait. Runtime retirement
// joins scene content but never frees Storage backing or closes the profile.
class Runtime final {
public:
    // Storage must have no live game: retire prior scene content first. State
    // and Storage outlive Runtime and every preparation/GPU borrow it exposes.
    static CreateResult create(const menu::Catalog&, game_input::Service&,
        game_audio::Session&, game::State&, scene::Storage&,
        std::uint32_t raw_game_time) noexcept;
    // Explicit source title reentry: requires successfully initialized State,
    // reuses the same profile/RNG/latches, and never runs boot/logo/open again.
    static CreateResult create_title(const menu::Catalog&, game_input::Service&,
        game_audio::Session&, game::State&, scene::Storage&,
        std::uint32_t raw_game_time) noexcept;
    ~Runtime();
    Runtime(const Runtime&) = delete;
    Runtime& operator=(const Runtime&) = delete;
    // Owner-thread only; controller publication and scheduler/audio retraces
    // are explicit external services. No input acquisition/presentation pacing
    // is hidden here. Main-menu source process priority0 invalidates its menu
    // cache AFTER priority1 controls; the host must not invalidate it again.
    Step tick(std::uint32_t raw_game_time) noexcept;
    Phase phase() const noexcept;
    Error error() const noexcept;
    const char* message() const noexcept;
    std::uint64_t updates() const noexcept;
    std::uint32_t logo_counter() const noexcept;
    int controller() const noexcept;
    const menu::scene::Ui* menu_ui() const noexcept; // Owner-phase observation.
    View view() noexcept;
    Exit exit() const noexcept;
    // Invalid/recovery profiles retain their actual checked Store evidence and
    // lock. Missing-only defaults never imply a durable write.
    const save::GameOpenResult* save_initialization() const noexcept;
    save::GameSession* save() noexcept;
    bool retire() noexcept; // After the preparation-view/actual GPU fence.
private:
    struct Impl;
    explicit Runtime(std::unique_ptr<Impl>) noexcept;
    std::unique_ptr<Impl> impl_;
};
struct CreateResult {
    Error error = Error::none;
    std::unique_ptr<Runtime> runtime;
    std::array<char, 512> message{};
    explicit operator bool() const noexcept { return error == Error::none && bool(runtime); }
};
} // namespace isle::startup
#endif
