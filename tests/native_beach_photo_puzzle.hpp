#ifndef ISLE_PRIVATE_NATIVE_BEACH_PHOTO_PUZZLE_HPP
#define ISLE_PRIVATE_NATIVE_BEACH_PHOTO_PUZZLE_HPP

#include "native_progression_puzzles.hpp"

namespace isle::private_campaign {
enum class BeachPhotoDependency {
    course_not_active, player_not_ready, apple_reward, prior_report, film_reserved,
    target_not_present, target_not_ready, target_out_of_range, item_ui_not_ready,
    projectile_out_of_reach, reveal_in_progress, none
};
struct BeachPhotoFacts {
    BeachPhotoDependency dependency = BeachPhotoDependency::course_not_active;
    bool owns_pad = false, player_ready = false, apple_available = false;
    bool prior_report_checked = false, prior_report = false;
    bool target_present = false, target_tangible = false, target_visible = false;
    bool initial_flag8_seen = false, reveal_state_seen = false;
    bool graph_changed_after_consumption = false, flag8_cleared = false;
    bool photo_eligible = false, published_focus = false, zoom_settled = false;
    bool capture_observed = false, qualified_capture = false, captured_chansey = false;
    // Completion is a real qualified roll exposure containing Chansey, NOT an
    // Oak score, new Report, Save commit or cold-reopen witness.
    bool complete = false;
    std::uint16_t target_flags = 0, focus_flags = 0;
    std::uint32_t target_graph = 0, target_state = 0, target_process_flags = 0;
    std::int32_t target_behavior = 0, report_index = -1, focus_id = 0;
    float target_distance = 0, collision_radius = 0;
    IsleGameVec3 target_position{}, target_collision{}, target_scale{}, input_target{};
    std::uint32_t throw_requests = 0, throws_pressed = 0, throws_consumed = 0;
    std::uint32_t shutter_requests = 0, shutters_pressed = 0, captures_consumed = 0;
    std::int32_t film = 0, qualified = 0, capture_slot = -1, captured_subject_zero = 0;
    std::uint64_t initial_update = 0, throw_request_update = 0, throw_consumed_update = 0;
    std::uint64_t throw_release_update = 0, graph_change_update = 0;
    std::uint64_t reveal_update = 0, flag8_clear_update = 0;
    std::uint64_t shutter_request_update = 0, capture_update = 0;
};
class BeachPhotoPuzzle final {
public:
    BeachPhotoPuzzle(private_post_course::Controller, Aim);
    BeachPhotoPuzzle(const BeachPhotoPuzzle&) = delete;
    BeachPhotoPuzzle& operator=(const BeachPhotoPuzzle&) = delete;
    bool update(const application::RuntimeView&, const game::State&,
                std::uint64_t completed_updates);
    const BeachPhotoFacts& facts() const noexcept { return facts_; }
private:
    void publish(std::uint16_t = 0, std::int8_t = 0, std::int8_t = 0) noexcept;
    private_post_course::Controller controller_;
    Aim aim_;
    BeachPhotoFacts facts_{};
    std::uint64_t last_update_ = 0;
    bool sampled_ = false, pending_throw_ = false, release_pending_ = false;
    bool pending_shutter_ = false, shutter_press_seen_ = false;
    std::int32_t pending_slot_ = -1, pending_qualified_ = 0;
};
} // namespace isle::private_campaign
#endif
