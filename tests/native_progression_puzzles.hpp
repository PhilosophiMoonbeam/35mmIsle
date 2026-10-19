#ifndef ISLE_PRIVATE_NATIVE_PROGRESSION_PUZZLES_HPP
#define ISLE_PRIVATE_NATIVE_PROGRESSION_PUZZLES_HPP

#include "post_course_scene_probe.hpp"
#include "35mmisle/engine.h"
#include <cstdint>

struct IslePlayerSource;
namespace isle::application { struct RuntimeView; }
namespace isle::game { class State; }

namespace isle::private_campaign {
enum class Puzzle { tunnel1014, river1019 };
// Non-owning bridge to Native Probe's existing pure camera geometry. This may
// inspect the actual Player/camera but must not publish input or mutate Source.
struct Aim {
    void* owner = nullptr;
    void (*point)(void*, const IslePlayerSource&, const IsleGameVec3&, bool zoomed,
                  std::int8_t& x, std::int8_t& y) = nullptr;
};
// Original item launch/half-step gravity, inverted into a real camera aim point.
// Read-only; false means no reachable low arc. The Item still resolves impacts.
[[nodiscard]] bool projectile_target(const IslePlayerSource&,const IsleGameVec3&,IsleGameVec3& aimed);
// Copy these from the canonical GameSnapshot, not the ridden course argument.
// Original progress_get reads selected_course; progress_set writes unlocked_course.
struct Progress { int known_course = -1, unlocked_course = -1; };
enum class PuzzleDependency {
    course_not_active, player_not_ready, target_not_present, target_not_ready, endpoint_not_ready,
    apple_reward, pester_reward, item_ui_not_ready, river_block4,
    river_trigger_surface, projectile_out_of_reach, none
};
struct PuzzleFacts {
    PuzzleDependency dependency = PuzzleDependency::course_not_active;
    bool player_ready = false, ability_available = false;
    bool target_present = false, target_ready = false, endpoint_ready = false;
    bool trigger_surface = false, owns_pad = false;
    bool trigger_transition = false, endpoint_entered = false;
    bool progress_observed = false, notice_observed = false;
    bool endpoint_animation_complete = false, endpoint_requested = false;
    // Requires an actual observed endpoint AND its actual progress/notice.
    // Does not assert physical scene retirement, Save durability or Oak unveiling.
    bool complete = false;
    std::uint32_t throw_requests = 0, throws_consumed = 0;
    std::uint32_t target_graph = 0;
    std::int32_t target_behavior = 0, current_block = -1;
    IsleGameVec3 target_position{}, input_target{};
};
class ProgressionPuzzles final {
public:
    ProgressionPuzzles(Puzzle, private_post_course::Controller, Aim);
    ProgressionPuzzles(const ProgressionPuzzles&) = delete;
    ProgressionPuzzles& operator=(const ProgressionPuzzles&) = delete;
    // Call serialized outside Source/native callbacks, once per actual completed
    // update. Repeated ordinals do not advance input. True reserves the pad;
    // false permits the parent's ordinary photography/travel driver. Parent owns
    // entry activation, all update/failure bounds, neutral teardown and routing.
    bool update(const application::RuntimeView&, const game::State&, Progress,
                std::uint64_t completed_updates);
    const PuzzleFacts& facts() const noexcept { return facts_; }
private:
    void publish(std::uint16_t = 0, std::int8_t = 0, std::int8_t = 0) noexcept;
    Puzzle puzzle_;
    private_post_course::Controller controller_;
    Aim aim_;
    PuzzleFacts facts_{};
    std::uint64_t last_update_ = 0;
    bool sampled_ = false, pending_throw_ = false, release_pending_ = false;
    bool acquired_ = false, baseline_ = false;
    int initial_unlocked_ = -1;
    bool surface_cached_ = false;
    IsleGameVec3 global_surface_{};
};
} // namespace isle::private_campaign
#endif
