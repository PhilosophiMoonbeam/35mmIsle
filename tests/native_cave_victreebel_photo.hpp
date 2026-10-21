#ifndef ISLE_PRIVATE_NATIVE_CAVE_VICTREEBEL_PHOTO_HPP
#define ISLE_PRIVATE_NATIVE_CAVE_VICTREEBEL_PHOTO_HPP

#include "native_progression_puzzles.hpp"
#include <array>
#include <cstddef>
#include <cstdint>

namespace isle::private_campaign {
using CaveVictreebelIncarnation = std::array<std::uint64_t,3>;
enum class CaveVictreebelPhase { observe, unzoom, aim, press, release, flight, knock, handshake, reveal, capture, finished };
enum class CaveVictreebelDisposition {
    course_not_active, player_not_ready, pester_unavailable, item_ui_not_ready, film_reserved,
    target_absent, target_not_ready, hidden_subject_absent, unsafe_geometry, item_outstanding,
    press_timeout, release_timeout, flight_timeout, hit_unattributed, heading_unattributed,
    wrong_landing, handshake_unattributed, actor_lost, observation_gap, scene_changed,
    reaction_timeout, capture_timeout, capture_pending, qualified, request_limit, steering_limit,
    recovery_limit, overall_limit, course_ended, source_error, capacity, none
};
enum class CaveVictreebelForecastDisposition {
    unobserved, inverse_rejected, invalid_launch, bounds, ground_unavailable, ground_contact,
    ceiling_contact, static_hit, competing_actor, target_missed, forbidden_ground_unavailable,
    knock_limit, wrong_surface, receiver_distance, accepted
};
struct CaveVictreebelForecast {
    CaveVictreebelForecastDisposition disposition=CaveVictreebelForecastDisposition::unobserved;
    IsleGameVec3 target{}, aimed{}, position{}, incoming_velocity{}, hit_root{}, landing{}, blocker_center{};
    CaveVictreebelIncarnation blocker{};
    float entry=2, target_entry=2, blocker_radius=0, flight=0, landing_distance=0, ground_height=0;
    int static_object=-1, ground_surface=-1;
    unsigned flight_step=0, knock_steps=0;
};
struct CaveVictreebelFacts {
    int required_species=71;
    bool available=false, complete=false, exhausted=false, owns_pad=false, capture_pending=false;
    bool player_ready=false, pester_available=false, target_present=false, target_ready=false;
    bool subject_present=false, subject_hidden=false, subject_visible=false, subject_flag4=false;
    bool subject_eligible=false, subject_focused=false, preplaced_seen=false, item_present=false;
    bool item_hidden=false, interaction_matches=false, hit_seen=false, heading_seen=false;
    bool landing_seen=false, command29_seen=false, command30_seen=false, command31_seen=false;
    bool reveal_seen=false, parent_cleanup_seen=false, target_death_cleared=false, capture_observed=false;
    CaveVictreebelPhase phase=CaveVictreebelPhase::observe;
    CaveVictreebelDisposition disposition=CaveVictreebelDisposition::course_not_active;
    int current_block=-1, film=0, qualified=0, raw_primary=0, taken_species=0, focus_species=0;
    int focus_context=-1, taken_context=-1, evaluator_slot=-1, captured_slot=-1, captured_evaluator_slot=-1;
    int item_kind=0, item_state=0, source_status=0, host_status=0, actual_surface=-1;
    std::uint32_t target_graph=0, target_state=0, target_path=0, target_flags=0, target_object_flags=0;
    std::uint32_t target_process_flags=0, target_object_id=0, subject_graph=0, subject_state=0, subject_path=0;
    std::uint32_t subject_flags=0, subject_object_flags=0, subject_object_id=0, item_flags=0, item_object_flags=0;
    std::uint32_t item_object_id=0, focus_flags=0;
    float collision_radius=0, actual_distance=0, incoming_yaw=0, facing_yaw=0, heading_error=0, hspeed=0, jump_velocity=0;
    IsleGameVec3 root{}, global_root{}, collision{}, subject_root{}, item_position{}, collision_velocity{}, motion{}, input_target{};
    IsleGameVec3 actual_landing{};
    CaveVictreebelIncarnation parent{}, subject{}, item{}, accepted_item{}, sender29{}, sender30{}, sender31{};
    CaveVictreebelIncarnation focus_actor{}, taken_actor{}, captured_actor{};
    std::uintptr_t parent_object=0, subject_object=0, item_object=0;
    std::uint64_t initial_update=0, last_update=0, request_update=0, consumed_update=0, release_update=0;
    std::uint64_t preplaced_update=0, hit_update=0, landing_update=0, command29_update=0, command30_update=0;
    std::uint64_t command31_update=0, reveal_update=0, cleanup_update=0, flag4_update=0, capture_update=0;
    std::uint64_t captured_slots=0, requests=0, pressed=0, consumed=0, releases=0, accepted_hits=0;
    std::uint64_t captures=0, capture_mismatches=0, mismatches=0, captured_identity_hash=0;
    std::uint64_t steering_updates=0, recovery_updates=0, flight_updates=0, arc_rejections=0;
    unsigned plan_count=0;
    std::array<CaveVictreebelForecast,18> plan_forecasts{};
    CaveVictreebelForecast actual_forecast{};
};
namespace cave_victreebel_detail {
struct Actor {
    CaveVictreebelIncarnation identity{};
    IsleGameObject* object=nullptr;
    IsleGameObject* interaction=nullptr;
    IsleGameVec3 root{}, global_root{}, collision{}, global_collision{};
    float radius=0, facing=0, hspeed=0, jump=0, distance=0, ground_height=0;
    std::uint32_t graph=0, state=0, path=0, flags=0, object_flags=0, object_id=0, process_flags=0, surface=0;
    bool present=false, ready=false, tangible=false, eligible=false, focused=false, empty_forbidden=false;
};
struct Item {
    CaveVictreebelIncarnation identity{};
    IsleGameObject* object=nullptr;
    IsleGameVec3 position{}, collision_velocity{};
    std::uint32_t flags=0, object_flags=0, object_id=0;
    int kind=0, state=0;
    float rest_timer=0;
};
struct Sphere { CaveVictreebelIncarnation identity{}; IsleGameVec3 center{}; float radius=0; };
struct Snapshot {
    Actor parent{}, subject{};
    std::array<Actor,16> subjects{};
    std::array<Item,20> items{};
    std::array<Sphere,256> spheres{};
    std::size_t subject_count=0, item_count=0, sphere_count=0;
    bool capacity=true, copied_context=false;
    int focus_species=0, focus_context=-1, taken_species=0, evaluator_slot=-1;
    std::uint32_t focus_flags=0;
    CaveVictreebelIncarnation focus_actor{}, taken_actor{};
};
}
// Only B/Z/stick. Observe every completed update, including generic shutter ownership.
class CaveVictreebelPhotoPuzzle final {
public:
    CaveVictreebelPhotoPuzzle(private_post_course::Controller, Aim);
    CaveVictreebelPhotoPuzzle(const CaveVictreebelPhotoPuzzle&)=delete;
    CaveVictreebelPhotoPuzzle& operator=(const CaveVictreebelPhotoPuzzle&)=delete;
    void observe(const application::RuntimeView&,std::uint64_t completed_updates);
    bool update(const application::RuntimeView&,std::uint64_t completed_updates);
    const CaveVictreebelFacts& facts() const noexcept { return facts_; }
private:
    void publish(std::uint16_t=0,std::int8_t=0,std::int8_t=0) noexcept;
    void stop(CaveVictreebelDisposition) noexcept;
    private_post_course::Controller controller_;
    Aim aim_;
    CaveVictreebelFacts facts_{};
    cave_victreebel_detail::Snapshot snapshot_{};
    IsleGame* scene_=nullptr;
    IsleGameObject* parent_object_=nullptr;
    IsleGameObject* subject_object_=nullptr;
    bool sampled_=false, control_sampled_=false, observation_active_=false, started_=false;
    bool pending_press_=false, pending_release_=false, tracking_=false, capture_armed_=false;
    bool neutral_due_=false, pad_published_=false, zoom_out_requested_=false, previous_knock_=false;
    std::uint64_t previous_update_=0, control_update_=0, release_start_=0, attempt_start_=0, capture_start_=0;
    std::uint64_t steering_=0, recovery_=0;
    int observed_roll_=-1, observed_qualified_=0, armed_slot_=-1, armed_qualified_=0;
    CaveVictreebelIncarnation previous_parent_{}, armed_subject_{};
    IsleGameVec3 previous_global_collision_{};
    std::array<CaveVictreebelIncarnation,20> prethrow_items_{};
    std::size_t prethrow_count_=0;
};
}
#endif
