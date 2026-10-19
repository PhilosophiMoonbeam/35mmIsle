#ifndef ISLE_RAINBOW_LIFECYCLE_HPP
#define ISLE_RAINBOW_LIFECYCLE_HPP
#include "application_manager.hpp"
#include "application_photos.hpp"
#include "rainbow_lifecycle_source.h"
#include "scene_graphics.hpp"
#include "world_module.hpp"
#include <array>
#include <cstdint>
#include <memory>
#include <optional>

namespace isle::rom { class Snapshot; }
namespace isle::application { class Manager; class Photos; }
namespace isle::game { class State; }
namespace isle::game_input { class Service; }
namespace isle::game_audio { class Session; }
namespace isle::host { class GtlClock; }
namespace isle::rainbow {
struct LifecycleAccess;
enum class LifecycleStatus { ready, setup_pending, running, retiring, completed, cancelled, failed, retired };
struct LifecycleCreateResult;
// One fixed-address original Rainbow entry and Actor/BGM record outside native
// scene Storage. The application retains its Manager EntryBorrow until every
// Source/native/semantic/model/graphics/Photo/audio borrower actually joins.
// Source chooses scene results; this owner adds no gameplay FSM or clock.
class Lifecycle final {
    friend struct LifecycleAccess;
public:
    static LifecycleCreateResult create(const rom::Snapshot&, application::Manager&,
        game::State&, game_input::Service&, game_audio::Session&, host::GtlClock&,
        application::Photos&, world::ModuleState&, std::int32_t entry_argument,
        bool require_cpu_renderer=false) noexcept;
    ~Lifecycle();
    Lifecycle(const Lifecycle&)=delete;
    Lifecycle& operator=(const Lifecycle&)=delete;
    Lifecycle(Lifecycle&&)=delete;
    Lifecycle& operator=(Lifecycle&&)=delete;
    // Independent Source only: first setup intent, then actual original route
    // after a separately witnessed complete native scene retirement.
    IsleGameResult resume() noexcept;
    const IsleRainbowLifecycleSceneSetup* pending_setup() const noexcept;
    IsleGameResult initialize_scene() noexcept;
    IsleGameResult update(std::uint32_t raw_game_time) noexcept;
    // A healthy returning scene exit truncated OM; never a completed GTL update.
    bool update_abandoned() const noexcept;
    bool draw(scene::graphics::FrameContext = {}) noexcept;
    bool graphics_busy() const noexcept;
    scene::graphics::Poll poll() noexcept;
    scene::graphics::Completed completed_frame() const noexcept;
    // Explicit completed Photo cancellation/teardown; ordinary presentation
    // release does not retire the Source CopyInfo/shutter consumer's image.
    IsleGameResult release_completed() noexcept;
    // Nonblocking real joins. False retains live owners for a later host retry.
    bool retire_scene() noexcept;
    IsleGameResult cancel() noexcept;
    IsleGameResult retire() noexcept;
    LifecycleStatus status() const noexcept;
    IsleGameResult error() const noexcept;
    const char* message() const noexcept;
    std::optional<std::int32_t> result() const noexcept;
    const IsleRainbowLifecycleSource& source() const noexcept;
    const IsleRainbowActorSource& actors_source() const noexcept;
private:
    struct Impl;
    explicit Lifecycle(std::unique_ptr<Impl>) noexcept;
    std::unique_ptr<Impl> impl_;
};
struct LifecycleCreateResult {
    IsleGameResult error=ISLE_GAME_OK;
    std::unique_ptr<Lifecycle> lifecycle;
    std::array<char,512> message{};
    explicit operator bool() const noexcept { return error==ISLE_GAME_OK && bool(lifecycle); }
};
} // namespace isle::rainbow
#endif
