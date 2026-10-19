#ifndef ISLE_PRIVATE_NATIVE_VALLEY_DASH_PUZZLE_HPP
#define ISLE_PRIVATE_NATIVE_VALLEY_DASH_PUZZLE_HPP

#include "native_progression_puzzles.hpp"

namespace isle::private_campaign {
enum class ValleyDashDependency {
    course_not_active, player_not_ready, pester_reward, item_ui_not_ready,
    final_block_actors, squirtle_proximity, squirtle_not_ready,
    mountain_mankey_not_ready, hidden_mankey_not_ready, switch_not_ready,
    endpoint_not_ready, projectile_out_of_reach, item_cooldown,
    squirtle_slide, squirtle_slide_missed, mankey_reveal,
    mankey_safe_side, mankey_failed_side, switch_sequence,
    endpoint_animation, dialogue_not_ready, dash_already_owned, launch_alignment, none
};
struct ValleyDashActorFacts {
    bool present = false, tangible = false, model_ready = false;
    std::int32_t behavior = -1;
    std::uint32_t graph = 0, process_flags = 0;
    float player_distance = 0, yaw = 0, initial_yaw = 0, collision_radius = 0;
    IsleGameVec3 position{}, collision{};
};
struct ValleyDashFacts {
    ValleyDashDependency dependency = ValleyDashDependency::course_not_active;
    bool owns_pad = false, player_ready = false, pester_available = false;
    bool dash_preexisting = false, dash_observed = false, notice_observed = false;
    bool squirtle_activated = false, squirtle_hit = false, squirtle_sliding = false;
    bool slide_item_direction_observed = false, slide_geometry_observed = false, slide_snap_observed = false;
    IsleGameVec3 slide_item_direction{}, slide_item_position{};
    float slide_item_yaw = 0, slide_target_yaw = 0;
    bool launch_alignment_ready = false;
    float launch_yaw = 0, launch_target_yaw = 0, launch_uncertainty = 0;
    bool mountain_mankey_hit = false, mankey_revealed = false;
    bool mankey_safe_side = false, mankey_hit = false, mankey_failed = false;
    bool trigger_transition = false, switch_triggered = false;
    bool endpoint_entered = false, endpoint_animation_complete = false;
    bool dialogue_ready = false, endpoint_requested = false, complete = false;
    std::int32_t current_block = -1;
    float block_progress = 0;
    // Actor records are current observations. Outcome booleans above latch
    // actual transitions; mankey_safe_side and readiness describe this update.
    ValleyDashActorFacts squirtle{}, mountain_mankey{}, mankey{}, switch_actor{}, endpoint{};
    IsleGameVec3 input_target{};
    std::uint32_t squirtle_throw_requests = 0, mankey_throw_requests = 0;
    std::uint32_t throws_consumed = 0, throw_presses_rejected = 0;
    std::uint32_t dialogue_requests = 0, dialogue_presses_consumed = 0;
};
class ValleyDashPuzzle final {
public:
    ValleyDashPuzzle(private_post_course::Controller, Aim);
    ValleyDashPuzzle(const ValleyDashPuzzle&) = delete;
    ValleyDashPuzzle& operator=(const ValleyDashPuzzle&) = delete;
    // One instance per actual Valley course epoch, serialized outside callbacks.
    // True owns this pad observation; false permits normal photography/travel.
    // Duplicate ordinals do not publish/advance; regressions are rejected.
    bool update(const application::RuntimeView&, const game::State&,
                std::uint64_t completed_updates);
    const ValleyDashFacts& facts() const noexcept { return facts_; }
private:
    enum class Press { none, squirtle, mankey, dialogue };
    void publish(std::uint16_t = 0, std::int8_t = 0, std::int8_t = 0) noexcept;
    private_post_course::Controller controller_;
    Aim aim_;
    ValleyDashFacts facts_{};
    std::uint64_t last_update_ = 0;
    bool sampled_ = false, baseline_ = false, acquired_ = false;
    bool release_pending_ = false;
    Press pending_ = Press::none;
};
} // namespace isle::private_campaign
#endif
