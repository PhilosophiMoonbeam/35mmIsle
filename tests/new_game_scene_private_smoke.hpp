#ifndef ISLE_NEW_GAME_SCENE_PRIVATE_SMOKE_HPP
#define ISLE_NEW_GAME_SCENE_PRIVATE_SMOKE_HPP
#include "new_game_scene.hpp"
#include <string>
namespace isle::new_game::private_smoke {
enum class Scenario { empty_default, seven_and_backspace, abort_while_selecting };
struct Frame {
    std::span<const std::byte> rgba;
    std::uint64_t actual_terminal_ordinal = 0;
};
struct Hooks {
    void* owner = nullptr;
    // Main's real scheduler must publish the supplied software-owned controller0
    // sample through normal input acquisition, perform one OM update, service
    // independent audio, and draw only when the source GTL cadence requires it.
    // A nondraw update returns ordinal0/empty RGBA. A draw executes render_input
    // through canonical RSP/RDP and completes GPU/readback/producer borrowers
    // BEFORE returning its real nonzero terminal ordinal and RGBA.
    // No host/device acquisition or source-state setters are used by this hook.
    bool (*advance)(void*, const game_input::HostFrame&, Frame&) noexcept = nullptr;
    // Required for the intentional raster-admission error regression: resolve
    // any real failed/held graphics task and readback before native retirement.
    bool (*fence)(void*) noexcept = nullptr;
    // Optional forwarding of the actual once-consumed Source result to the
    // application-manager integration caller; never a fabricated scene ID.
    std::int32_t* completed_result = nullptr;
};
// Source-only opt-in consumer. Requires a genuinely fresh RAM new-game session,
// actual private Resources and Main's real scheduler/render/audio owners. It
// never creates a fake renderer/audio executor or forces a scene state/result.
bool exercise(Scene&, game::State&, Hooks, Scenario, unsigned maximum_updates, std::string& error);
// Main constructs the actual Scene with Config.raster_end==raster_begin. The
// real native opening process must reject its first Element allocation, retain
// a first error and retire without name/PFID16/result/durable side effects.
bool reject_raster_extent(Scene&, game::State&, Hooks, unsigned maximum_updates, std::string& error);
} // namespace isle::new_game::private_smoke
#endif
