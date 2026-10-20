#ifndef ISLE_BEACH_LIFECYCLE_HPP
#define ISLE_BEACH_LIFECYCLE_HPP
#include "application_manager.hpp"
#include "application_photos.hpp"
#include "actor_behavior_bindings.hpp"
#include "beach_lifecycle_source.h"
#include "beach_ending_source.h"
#include "scene_graphics.hpp"
#include "world_module.hpp"
#include <memory>
#include <optional>
namespace isle::actor { class Owner; class Producer; class Resources; }
namespace isle::world { class World; }
namespace isle::items { class Owner; }
namespace isle::beach {
struct LifecycleAccess;
enum class LifecycleStatus { ready, setup_pending, running, retiring, completed, cancelled, failed, retired };
struct LifecycleCreateResult;
// Fixed-address original C invocation. The host owns input/retrace/audio/GtlClock
// and keeps its Manager EntryBorrow until this owner/domain and all consumers
// retire. Neither callbacks nor resume() construct/drive another continuation.
class Lifecycle final {
    friend struct LifecycleAccess;
public:
    struct Behaviors {
        std::span<const actor::BehaviorCallbackBinding> callbacks;
        std::span<const std::uint32_t> graphs, roots;
    };
    static LifecycleCreateResult create(const rom::Snapshot&, application::Manager&,
        game::State&, game_input::Service&, game_audio::Session&, host::GtlClock&,
        application::Photos&, world::ModuleState&, std::int32_t entry_argument, Behaviors,
        bool require_cpu_renderer=false) noexcept;
    ~Lifecycle();
    Lifecycle(const Lifecycle&)=delete;
    Lifecycle& operator=(const Lifecycle&)=delete;
    Lifecycle(Lifecycle&&)=delete;
    Lifecycle& operator=(Lifecycle&&)=delete;
    // Independent Source continuation only. First resume publishes setup intent;
    // post-retirement resume executes the actual original signed scene route.
    IsleGameResult resume() noexcept;
    const IsleBeachLifecycleSceneSetup* pending_setup() const noexcept;
    // Host-only, after resume returns. Creates a cold native scene then invokes
    // the complete returning original init fence exactly once.
    IsleGameResult initialize_scene() noexcept;
    // Input Service::begin_update is the application's responsibility. Update
    // rejects GPU/readback work but retains completed CPU Photo/history for
    // original CopyInfo and shutter consumers; draw retires it before reuse.
    IsleGameResult update(std::uint32_t raw_game_time) noexcept;
    // True only after a healthy Source exit stopped traversal at its OM epilogue.
    // The application must abandon, not finish, this update's GTL claim.
    bool update_abandoned() const noexcept;
    bool draw(scene::graphics::FrameContext = {}) noexcept;
    bool graphics_busy() const noexcept;
    scene::graphics::Poll poll() noexcept;
    scene::graphics::Completed completed_frame() const noexcept;
    // Explicit completed Photo retirement for cancellation/teardown, not the
    // application's ordinary presentation-image release.
    IsleGameResult release_completed() noexcept;
    // Nonblocking real scene retirement. false retains all unclosed owners;
    // caller continues GPU polling/releases and retries at host boundaries.
    bool retire_scene() noexcept;
    IsleGameResult cancel() noexcept;
    IsleGameResult retire() noexcept;
    LifecycleStatus status() const noexcept;
    IsleGameResult error() const noexcept;
    const char* message() const noexcept;
    std::optional<std::int32_t> result() const noexcept;
    const IsleBeachLifecycleSource& source() const noexcept;
    // Canonical live ending state, NULL after actual Actor owner retirement.
    const IsleBeachEndingSource* ending_source() const noexcept;
    // Scoped read-only observations, like NativeHost. Native pointers become
    // NULL when their real owners retire; immutable resources live with Entry.
    const actor::Resources& actor_resources() const noexcept;
    const actor::Owner* actors() const noexcept;
    const actor::Producer* actor_packet_producer() const noexcept;
    const world::World* world_owner() const noexcept;
    const items::Owner* items_owner() const noexcept;
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
}
#endif
