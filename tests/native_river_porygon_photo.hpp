#ifndef ISLE_PRIVATE_NATIVE_RIVER_PORYGON_PHOTO_HPP
#define ISLE_PRIVATE_NATIVE_RIVER_PORYGON_PHOTO_HPP

#include "native_progression_puzzles.hpp"
#include <array>
#include <cstddef>
#include <cstdint>

namespace isle::application { class Photos; }
namespace isle::private_campaign {
using PorygonIncarnation = std::array<std::uint64_t, 3>;
bool needs_river_porygon_photo(application::Photos&);
enum class RiverPorygonPhase { observe, unzoom, aim, press, release, flight, reveal, capture, finished };
enum class RiverPorygonDisposition {
    course_not_active, player_not_ready, pester_unavailable, film_reserved,
    target_absent, target_not_ready, item_ui_not_ready, item_outstanding,
    surface_unavailable, projectile_out_of_reach, steering_limit, recovery_limit,
    request_limit, press_timeout, release_timeout, flight_timeout, reveal_timeout,
    item_ambiguous, item_lost, actor_lost, capacity_limit, reveal_unattributed,
    capture_pending, qualified, overall_limit, course_ended, none
};
// Predictions gate input only; they never establish an actual notification.
enum class RiverPorygonForecastDisposition {
    unobserved, qualifying_ground, invalid_launch, outside_world, ground_unavailable,
    ceiling_obstruction, static_obstruction, wrong_surface, sender_out_of_range, flight_limit
};
struct RiverPorygonForecast {
    RiverPorygonForecastDisposition disposition=RiverPorygonForecastDisposition::unobserved;
    int first_surface=-1, static_object=-1;
    IsleGameVec3 first_position{};
    float actor_distance=0;
    std::uint32_t flight_updates=0;
};
struct RiverPorygonFacts {
    int required_species=137;
    bool available=false, complete=false, owns_pad=false, exhausted=false;
    bool player_ready=false, pester_available=false, target_present=false;
    bool hidden_seen=false, trigger_surface=false, item_present=false;
    bool notification_observed=false, sender_seen=false, reveal_seen=false;
    bool revealed_eligible=false, revealed_focused=false, capture_observed=false;
    bool first_ground_observed=false, first_ground_queried=false, first_ground_consecutive=false;
    RiverPorygonPhase phase=RiverPorygonPhase::observe;
    RiverPorygonDisposition disposition=RiverPorygonDisposition::course_not_active;
    int current_block=-1, target_behavior=-1, item_kind=0, item_state=0;
    int film=0, qualified=0, captured_slot=-1, raw_primary=0, evaluated_species=-1;
    // Evaluation belongs to the parent's genuine post-course replay; -1 is
    // explicitly unobserved here, never an inferred species or scorer result.
    int focus_species=0, focus_context=-1, taken_context=-1, ground_surface=-1;
    std::uint32_t target_graph=0, target_state=0, target_flags=0, target_object_flags=0;
    std::uint32_t target_object_id=0, process_flags=0, model_animation=0;
    std::uint32_t item_flags=0, item_object_flags=0, item_object_id=0, focus_flags=0;
    std::uint32_t first_ground_flags=0, first_ground_object_flags=0;
    float animation_time=0, item_actor_distance=0, notification_actor_distance=0;
    float first_ground_actor_distance=-1;
    IsleGameVec3 target_position{}, collision{}, input_target{}, surface_target{};
    IsleGameVec3 item_position{}, notification_position{};
    IsleGameVec3 first_ground_position{};
    RiverPorygonForecast ideal_forecast{}, actual_forecast{}, rejected_forecast{};
    PorygonIncarnation target{}, item{}, interaction_sender{}, taken_actor{};
    std::uint64_t initial_update=0, last_update=0, hidden_update=0, request_update=0;
    std::uint64_t consumed_update=0, release_update=0, notification_update=0;
    std::uint64_t sender_update=0, reveal_update=0, capture_update=0;
    std::uint64_t first_ground_update=0;
    std::uint64_t captured_slots=0, requests=0, pressed=0, consumed=0, releases=0;
    std::uint64_t steering_updates=0, recovery_updates=0, flight_updates=0;
    std::uint64_t captures=0, capture_mismatches=0, lost_items=0;
    std::uint64_t forecast_candidates=0, forecast_rejections=0, actual_forecast_rejections=0;
};
namespace porygon_detail {
struct Actor {
    bool present=false, hidden=false, revealing=false, eligible=false, focused=false;
    PorygonIncarnation identity{}, interaction{};
    std::uint32_t graph=0, state=0, flags=0, object_flags=0, object_id=0;
    std::uint32_t process_flags=0, model_animation=0;
    int behavior=0;
    float animation_time=0;
    IsleGameVec3 position{}, collision{};
};
struct Item {
    PorygonIncarnation identity{};
    int kind=0, state=0;
    std::uint32_t flags=0, object_flags=0, object_id=0;
    IsleGameVec3 position{}, previous_position{};
};
}
// Pester/reveal only. Generic photography retains every shutter/film decision.
// All retained identities are copied incarnation keys, never native borrows.
class RiverPorygonPhotoPuzzle final {
public:
    RiverPorygonPhotoPuzzle(private_post_course::Controller, Aim);
    RiverPorygonPhotoPuzzle(const RiverPorygonPhotoPuzzle&)=delete;
    RiverPorygonPhotoPuzzle& operator=(const RiverPorygonPhotoPuzzle&)=delete;
    // Passive, idempotent per completed OM update, including another owner's pad.
    void observe(const application::RuntimeView&, std::uint64_t completed_updates);
    bool update(const application::RuntimeView&, std::uint64_t completed_updates);
    const RiverPorygonFacts& facts() const noexcept { return facts_; }
private:
    void publish(std::uint16_t=0, std::int8_t=0, std::int8_t=0) noexcept;
    void stop(RiverPorygonDisposition) noexcept;
    private_post_course::Controller controller_;
    Aim aim_;
    RiverPorygonFacts facts_{};
    std::array<porygon_detail::Actor, 8> actors_{};
    std::array<porygon_detail::Item, 20> items_{};
    std::array<PorygonIncarnation, 20> prethrow_items_{};
    std::size_t actor_count_=0, item_count_=0, prethrow_count_=0;
    bool sampled_=false, started_=false, active_=false, control_sampled_=false;
    bool pending_press_=false, pending_release_=false, tracking_item_=false;
    bool capture_armed_=false, zoom_out_requested_=false, neutral_pending_=false;
    bool ground_before_=false, surface_cached_=false;
    int observed_roll_=-1, observed_qualified_=0, armed_slot_=-1, armed_qualified_=0;
    std::uint64_t observation_update_=0, control_update_=0, release_start_=0;
    std::uint64_t steering_=0, recovery_=0;
    PorygonIncarnation previous_target_{}, previous_item_{}, armed_subject_{};
    IsleGameVec3 previous_global_target_{}, global_surface_{};
};
} // namespace isle::private_campaign
#endif
