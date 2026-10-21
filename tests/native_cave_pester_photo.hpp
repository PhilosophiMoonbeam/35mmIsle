#ifndef ISLE_PRIVATE_NATIVE_CAVE_PESTER_PHOTO_HPP
#define ISLE_PRIVATE_NATIVE_CAVE_PESTER_PHOTO_HPP

#include "native_progression_puzzles.hpp"
#include <array>
#include <cstddef>
#include <cstdint>

namespace isle::application { class Photos; }
namespace isle::private_campaign {
using CavePesterIncarnation = std::array<std::uint64_t, 3>;
int choose_cave_pester_photo_species(application::Photos&);
enum class CavePesterPhase { observe, unzoom, aim, press, release, flight, hit_animation, evolution, capture, finished };
enum class CavePesterDisposition {
    course_not_active, player_not_ready, pester_unavailable, item_ui_not_ready,
    film_reserved, target_absent, target_not_ready, unsafe_geometry, item_outstanding,
    press_timeout, release_timeout, flight_timeout, hit_unattributed, counter_unattributed,
    actor_lost, spawn_unattributed, evolution_timeout, capture_pending, qualified,
    request_limit, steering_limit, recovery_limit, overall_limit, course_ended, source_error, capacity, none
};
enum class CavePesterForecastDisposition {
    unobserved, inverse_rejected, invalid_launch, ground_unavailable, ground_contact,
    ceiling_contact, static_hit, competing_actor, target_hit, flight_limit, actor_motion_unobserved
};
struct CavePesterForecast {
    CavePesterForecastDisposition disposition=CavePesterForecastDisposition::unobserved;
    IsleGameVec3 target{}, aimed{}, position{}, blocker_center{}, blocker_motion{};
    CavePesterIncarnation blocker{};
    float blocker_radius=0, entry=2, target_entry=2, ground_height=0;
    int static_object=-1, ground_surface=-1, blocker_species=0;
    unsigned step=0;
    // Every sphere uses its last consecutive global collision-center delta.
    // Constant motion is a prospective assumption, never a collision guarantee.
    bool aimed_valid=false, constant_motion=false;
};
enum class CavePesterLaunchGate {
    unobserved, steering, b_held, actual_ray_rejected, clear_ray, outstanding_item, requested,
    cooldown, impact_spacing, impact_window, flight_unqualified, item_capacity, throws_committed
};
// Next OM's reticle-object update consumes this completed camera/reticle, not
// the already-published cam_target. This is a prediction, never a hit receipt.
struct CavePesterConsumeRay {
    IsleGameCameraParameters camera{};
    IsleGameVec3 ray{};
    std::int32_t reticle_x=0, reticle_y=0, direction=0;
    float input_x=0, input_y=0, stick_x_value=0, stick_y_value=0;
    float stick_x_accum=0, stick_y_accum=0, view_yaw=0, view_pitch=0;
    bool valid=false;
};
struct CavePesterThrow {
    CavePesterIncarnation item{}, sender{}, receiver{};
    CavePesterForecast launch_forecast{}, remaining_forecast{};
    IsleGameVec3 launch_eye{}, launch_ray{}, launch_player_velocity{}, created_position{}, created_velocity{};
    IsleGameVec3 consumed_eye{}, consumed_ray{}, consumed_player_velocity{};
    IsleGameVec3 position{}, velocity{};
    std::uint64_t request_update=0, consumed_update=0, release_update=0, created_update=0;
    std::uint64_t observed_update=0, hit_update=0, counter_update=0, retired_update=0, forecast_update=0;
    std::uint64_t launch_predicted_impact=0, predicted_impact=0, preceding_impact=0, admission_after=0, admission_before=0;
    std::uint32_t object_id=0, flags=0, object_flags=0, receiver_graph=0, receiver_state=0;
    std::uint32_t hit_process_flags=0, hit_model_animation=0, counter_graph=0, counter_state=0;
    int kind=0, state=0, counter_before=0, counter_after=0, counter_window=0;
    float rest_timer=0, hit_animation_time=0;
    std::array<CavePesterIncarnation,20> prethrow_items{};
    std::size_t prethrow_count=0;
    std::uint32_t consumed_buttons=0, pressed_buttons=0;
    int consumed_timeout=0;
    bool created=false, present=false, hidden=false, accepted=false, counter_incremented=false, retired=false;
    CavePesterConsumeRay consume_prediction{};
};
struct CavePesterFacts {
    int required_species=0;
    bool available=false, complete=false, owns_pad=false, exhausted=false;
    bool player_ready=false, pester_available=false, target_present=false, target_ready=false;
    bool item_present=false, item_hidden=false, interaction_matches=false, hit_seen=false;
    bool evolution_seen=false, spawn_seen=false, parent_cleanup_seen=false;
    bool child_present=false, child_eligible=false, child_focused=false, capture_observed=false;
    CavePesterPhase phase=CavePesterPhase::observe;
    CavePesterDisposition disposition=CavePesterDisposition::course_not_active;
    int current_block=-1, target_behavior=-1, hit_counter=0, hit_window=0;
    int film=0, qualified=0, raw_primary=0, taken_species=0, focus_species=0;
    int focus_context=-1, taken_context=-1, captured_slot=-1, item_kind=0, item_state=0;
    int source_status=0, host_status=0;
    std::uint32_t target_graph=0, target_state=0, target_flags=0, target_object_flags=0;
    std::uint32_t process_flags=0, model_animation=0, target_object_id=0, child_object_id=0;
    std::uint32_t child_flags=0, item_flags=0, item_object_flags=0, item_object_id=0, focus_flags=0;
    float animation_time=0, collision_radius=0, item_distance=0, spawn_distance_squared=0, spawn_yaw_error=0;
    float predicted_flight=0;
    IsleGameVec3 root{}, global_root{}, collision{}, child_root{}, item_position{}, input_target{}, motion{};
    CavePesterIncarnation parent{}, child{}, item{}, accepted_item{};
    CavePesterIncarnation focus_actor{}, taken_actor{}, captured_actor{};
    std::uint64_t initial_update=0, last_update=0, request_update=0, consumed_update=0, release_update=0;
    std::uint64_t hit_update=0, evolution_update=0, spawn_update=0, cleanup_update=0, capture_update=0;
    std::uint64_t captured_slots=0, requests=0, pressed=0, consumed=0, releases=0;
    std::uint64_t accepted_hits=0, counter_increments=0, captures=0, capture_mismatches=0, mismatches=0;
    std::uint64_t steering_updates=0, recovery_updates=0, flight_updates=0, arc_rejections=0;
    std::array<CavePesterForecast,4> plan_forecasts{};
    CavePesterForecast actual_forecast{};
    IsleGameVec3 planning_eye{}, planning_velocity{}, planning_ray{}, planning_motion{};
    float planning_radius=0;
    std::uint64_t planning_update=0;
    unsigned plan_count=0;
    std::uint64_t launch_update=0;
    CavePesterLaunchGate launch_gate=CavePesterLaunchGate::unobserved;
    int launch_aim_x=0, launch_aim_y=0;
    float launch_reticle_x=0, launch_reticle_y=0, blocking_rest_timer=0;
    CavePesterIncarnation blocking_item{};
    int blocking_kind=0, blocking_state=0;
    std::uint32_t blocking_flags=0, blocking_object_flags=0;
    std::array<CavePesterThrow,3> throws{};
    std::size_t throw_count=0;
    std::uint32_t reaction_header=0, reaction_receiver=0;
    float reaction_speed=0;
    unsigned reaction_wait=0, reaction_callback_wait=0, reaction_advances=0;
    std::uint64_t predicted_impact=0, admission_after=0, admission_before=0;
    int live_pester_items=0, throw_cooldown=0;
    CavePesterConsumeRay consume_prediction{};
};
namespace cave_pester_detail {
struct Actor {
    CavePesterIncarnation identity{}, interaction{};
    IsleGameVec3 root{}, global_root{}, collision{};
    float radius=0, yaw=0, animation_time=0, distance=0;
    std::uint32_t graph=0, state=0, flags=0, object_flags=0, object_id=0, process_flags=0, model_animation=0;
    int behavior=0, counter=0, window=0;
    bool present=false, ready=false, tangible=false, eligible=false, focused=false, hit=false, evolving=false;
};
struct Item {
    CavePesterIncarnation identity{};
    IsleGameVec3 position{};
    IsleGameVec3 velocity{};
    std::uint32_t flags=0, object_flags=0, object_id=0;
    int kind=0, state=0;
    float rest_timer=0;
};
struct Sphere {
    CavePesterIncarnation identity{};
    IsleGameVec3 center{}, global_center{}, motion{};
    float radius=0;
    int species=0;
    bool motion_valid=false;
};
struct CollisionSample { CavePesterIncarnation identity{}; IsleGameVec3 global_center{}; };
struct Snapshot {
    Actor parent{};
    std::array<Actor,16> children{};
    std::array<Item,20> items{};
    std::array<Sphere,256> spheres{};
    std::size_t child_count=0, item_count=0, sphere_count=0;
    bool capacity=true, captured_subject_context=false, other_evolving_near=false;
    int focus_species=0, focus_context=-1, taken_species=0;
    std::uint32_t focus_flags=0;
    CavePesterIncarnation focus_actor{}, taken_actor{};
};
}
// Pester/reveal only. Generic photography owns every shutter and film decision.
// Observe every actual completed update, even while another controller owns pad.
class CavePesterPhotoPuzzle final {
public:
    CavePesterPhotoPuzzle(private_post_course::Controller, Aim, int required_species);
    CavePesterPhotoPuzzle(const CavePesterPhotoPuzzle&)=delete;
    CavePesterPhotoPuzzle& operator=(const CavePesterPhotoPuzzle&)=delete;
    void observe(const application::RuntimeView&, std::uint64_t completed_updates);
    bool update(const application::RuntimeView&, std::uint64_t completed_updates);
    const CavePesterFacts& facts() const noexcept { return facts_; }
private:
    void publish(std::uint16_t=0, std::int8_t=0, std::int8_t=0) noexcept;
    void stop(CavePesterDisposition) noexcept;
    private_post_course::Controller controller_;
    Aim aim_;
    CavePesterFacts facts_{};
    cave_pester_detail::Snapshot snapshot_{};
    bool sampled_=false, control_sampled_=false, observation_active_=false, started_=false;
    bool pending_press_=false, pending_release_=false, tracking_=false, accepted_attempt_=false;
    bool waiting_counter_=false, zoom_out_requested_=false, capture_armed_=false, neutral_due_=false;
    bool previous_parent_present_=false, previous_evolving_=false, previous_hit_=false;
    bool previous_other_evolving_=false;
    std::uint64_t previous_update_=0, control_update_=0, attempt_start_=0, release_start_=0;
    std::uint64_t steering_=0, recovery_=0;
    int observed_roll_=-1, observed_qualified_=0, armed_slot_=-1, armed_qualified_=0, counter_before_=0;
    CavePesterIncarnation armed_subject_{}, previous_parent_{};
    IsleGameVec3 previous_global_root_{};
    float previous_yaw_=0;
    std::array<CavePesterIncarnation,20> prethrow_items_{};
    std::size_t prethrow_count_=0;
    std::size_t pending_throw_=3, counter_throw_=3;
    std::array<CavePesterIncarnation,16> previous_children_{};
    std::size_t previous_child_count_=0;
    std::array<cave_pester_detail::CollisionSample,256> previous_spheres_{};
    std::size_t previous_sphere_count_=0;
    std::uint64_t previous_sphere_update_=0;
};
}
#endif
