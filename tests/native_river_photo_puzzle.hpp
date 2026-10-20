#ifndef ISLE_PRIVATE_NATIVE_RIVER_PHOTO_PUZZLE_HPP
#define ISLE_PRIVATE_NATIVE_RIVER_PHOTO_PUZZLE_HPP

#include "native_progression_puzzles.hpp"
#include "player_photo_source.h"
#include <array>
#include <cstddef>

namespace isle::private_campaign {
using RiverIncarnation = std::array<std::uint64_t, 3>;
enum class RiverPhotoPhase { observe, unzoom, aim, press, release, flight, approach, evolution, capture, finished };
enum class RiverPhotoDisposition {
    course_not_active, player_not_ready, apple_unavailable, preserve_slowpoke,
    film_reserved, target_absent, target_not_ready, endpoint_not_ready,
    item_ui_not_ready, item_outstanding, unsafe_geometry, steering_limit,
    recovery_limit, request_limit, press_timeout, release_timeout,
    item_ambiguous, item_lost, item_unsafe, approach_timeout, actor_lost,
    incarnation_limit, evolution_unattributed, evolution_timeout,
    spawn_unattributed, capture_pending, overall_limit, course_ended, qualified, none
};
struct RiverPhotoFacts {
    bool available=false, complete=false, owns_pad=false, exhausted=false;
    bool player_ready=false, apple_available=false, prior_slowpoke_report=false;
    bool slowpoke_captured=false, target_present=false, endpoint_ready=false;
    bool item_present=false, item_still=false, item_safe=false, item_closest=false;
    bool different_apple=false, apple_reference_matches=false, interaction_reference_matches=false;
    bool approach_seen=false, endpoint_progress=false, command28_seen=false;
    bool evolution_seen=false, parent_cleanup_seen=false, spawn_seen=false;
    bool child_present=false, child_eligible=false, child_focused=false, capture_observed=false;
    RiverPhotoPhase phase=RiverPhotoPhase::observe;
    RiverPhotoDisposition disposition=RiverPhotoDisposition::course_not_active;
    std::int32_t slowbro_slot=-1, current_block=-1, target_behavior=-1;
    std::int32_t film=0, qualified=0, item_state=0, item_kind=0, raw_primary=0;
    std::int32_t focus_species=0, focus_context=-1, taken_context=-1;
    std::uint32_t target_graph=0, target_state=0, process_flags=0, model_animation=0;
    std::uint32_t target_flags=0, child_flags=0, item_flags=0, focus_flags=0;
    std::uint32_t target_object_id=0, child_object_id=0, item_object_id=0;
    std::int32_t counter=0, ground_surface=-1, predicted_surface=-1;
    float animation_time=0, root_endpoint_squared=0, initial_endpoint_squared=0;
    float best_endpoint_squared=0, item_root_distance=0, collision_radius=0;
    float target_yaw=0, child_yaw=0, spawn_yaw_error=0, spawn_distance_squared=0;
    IsleGameVec3 root{}, endpoint{}, collision{}, global_root{}, child_root{};
    IsleGameVec3 item_position{}, candidate{}, predicted_landing{}, input_target{};
    RiverIncarnation parent{}, child{}, item{};
    std::uint64_t initial_update=0, last_update=0, request_update=0, consumed_update=0;
    std::uint64_t release_update=0, approach_update=0, evolution_update=0;
    std::uint64_t spawn_update=0, capture_update=0;
    std::uint64_t captured_slots=0; // Coherent exposures of this same child; roll capacity is sixty.
    std::uint64_t requests=0, pressed=0, consumed=0, releases=0;
    std::uint64_t steering_updates=0, recovery_updates=0, flight_updates=0;
    std::uint64_t planning_updates=0;
    std::uint64_t approach_updates=0, evolution_updates=0, spawns=0, captures=0;
    std::uint64_t mismatches=0, lost_items=0, ambiguous_items=0, blocked_items=0;
    std::uint64_t candidates=0, ground_rejections=0, corridor_rejections=0;
    std::uint64_t arc_rejections=0, actual_ray_rejections=0, incarnation_count=0;
    std::uint64_t capture_focus_updates=0, capture_mismatches=0;
};
namespace river_detail {
struct Actor {
    bool present=false, eligible=false, focused=false, ready=false, approaching=false, evolving=false;
    bool endpoint_ready=false, ground_list_valid=true, explicit_ground=false;
    RiverIncarnation identity{}, apple{}, interaction{};
    std::uint32_t object_id=0, graph=0, state=0, process_flags=0, flags=0, model_animation=0;
    int behavior=0, counter=0;
    float radius=0, yaw=0, animation_time=0, endpoint_distance=0;
    IsleGameVec3 root{}, global_root{}, collision{}, endpoint{};
    std::array<float, 32> forbidden{};
    std::size_t forbidden_count=0;
};
struct Sphere { IsleGameVec3 center{}; float radius=0; };
struct Item {
    RiverIncarnation identity{};
    int kind=0, state=0;
    unsigned flags=0, object_flags=0;
    std::uint32_t object_id=0;
    IsleGameVec3 position{};
};
struct Planning {
    Actor parent{};
    std::array<Sphere, 256> spheres{};
    std::array<Item, 20> items{};
    std::size_t sphere_count=0, item_count=0;
    bool capacity_ok=true;
    std::array<Actor, 16> children{};
    std::size_t child_count=0;
    bool other_evolving=false, multiple_other_evolving=false;
    IsleGameVec3 other_global{};
    bool captured_child_context=false;
    IslePlayerPhotoFocus focus{}; // Scalar fields only; object is never stored.
};
} // namespace river_detail
// Lure-only controller. Generic photography retains all shutter/film/retake
// authority. All references retained here are scalar native incarnation keys.
class RiverPhotoPuzzle final {
public:
    RiverPhotoPuzzle(private_post_course::Controller, Aim);
    RiverPhotoPuzzle(const RiverPhotoPuzzle&)=delete;
    RiverPhotoPuzzle& operator=(const RiverPhotoPuzzle&)=delete;
    // Call on every completed update, including ordinary shutter/release.
    // Idempotent per ordinal; NEVER publishes pad input or requests an item.
    void observe(const application::RuntimeView&, std::uint64_t completed_updates);
    bool update(const application::RuntimeView&, std::uint64_t completed_updates);
    const RiverPhotoFacts& facts() const noexcept { return facts_; }
    std::uint64_t requests() const noexcept { return facts_.requests; }
private:
    void publish(std::uint16_t=0, std::int8_t=0, std::int8_t=0) noexcept;
    void stop(RiverPhotoDisposition) noexcept;
    void abandon_parent(RiverPhotoDisposition) noexcept;
    private_post_course::Controller controller_;
    Aim aim_;
    RiverPhotoFacts facts_{};
    bool sampled_=false, started_=false, reports_sampled_=false;
    bool pending_press_=false, pending_release_=false, tracking_item_=false;
    bool attempt_approach_=false, evolution_entered_=false;
    bool previous_parent_evolving_=false, capture_armed_=false;
    bool zoom_out_requested_=false;
    std::uint64_t previous_update_=0, attempt_start_=0, release_start_=0;
    std::uint64_t actor_steering_=0, actor_recovery_=0, actor_planning_=0;
    int observed_roll_=-1, observed_qualified_=0, armed_slot_=-1, armed_qualified_=0;
    RiverIncarnation armed_child_{};
    std::array<RiverIncarnation, 2> attempted_parents_{};
    std::array<RiverIncarnation, 20> prethrow_items_{};
    std::size_t prethrow_count_=0;
    bool control_sampled_=false, observation_active_=false;
    std::uint64_t control_update_=0;
    river_detail::Planning planning_{};
    RiverIncarnation prethrow_actor_apple_{};
    std::array<RiverIncarnation, 16> previous_children_{};
    std::size_t previous_child_count_=0;
    IsleGameVec3 previous_parent_global_{};
    float previous_parent_yaw_=0;
    bool previous_other_evolving_=false, previous_other_ambiguous_=false;
    IsleGameVec3 previous_other_global_{};
};
} // namespace isle::private_campaign
#endif
