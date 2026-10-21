#ifndef ISLE_PRIVATE_NATIVE_VALLEY_GROUND_REVEAL_PHOTO_HPP
#define ISLE_PRIVATE_NATIVE_VALLEY_GROUND_REVEAL_PHOTO_HPP

#include "native_progression_puzzles.hpp"
#include "player_photo_source.h"
#include <array>
#include <cstddef>
#include <cstdint>

namespace isle::private_campaign {
using ValleyGroundRevealIncarnation=std::array<std::uint64_t,3>;
enum class ValleyGroundRevealPhase { observe, unzoom, aim, press, release, flight, bounce, handshake, capture, finished };
enum class ValleyGroundRevealDisposition {
    course_not_active, player_not_ready, pester_unavailable, item_ui_not_ready, film_reserved,
    target_absent, target_not_ready, hidden_subject_absent, receiver_not_ready, unsafe_geometry,
    item_outstanding, press_timeout, release_timeout, flight_timeout, hit_unattributed,
    bounce_unattributed, handshake_unattributed, window_ended, actor_lost, observation_gap,
    scene_changed, reaction_timeout, capture_timeout, request_limit, steering_limit,
    recovery_limit, overall_limit, course_ended, source_error, capacity, capture_pending, qualified, none
};
enum class ValleyGroundRevealForecastDisposition {
    unobserved, inverse_rejected, invalid_launch, bounds, ground_unavailable, ground_contact,
    ceiling_contact, static_hit, competing_actor, target_missed, bounce_limit, receiver_distance, accepted
};
struct ValleyGroundRevealForecast {
    ValleyGroundRevealForecastDisposition disposition=ValleyGroundRevealForecastDisposition::unobserved;
    IsleGameVec3 aimed{}, hit_root{}, landing{}, normal{}, blocker_center{};
    ValleyGroundRevealIncarnation blocker{};
    float flight=0, receiver_distance=0, yaw=0;
    unsigned bounce_steps=0, reflections=0;
    int surface=-1, static_object=-1;
};
struct ValleyGroundRevealThrowReceipt {
    ValleyGroundRevealIncarnation actor{}, item{}, accepted_item{};
    bool item_seen=false, item_present=false, item_hidden=false, item_removed=false;
    bool interaction_matches=false, hit_seen=false, bounce_seen=false, landing_seen=false;
    std::uint64_t request_update=0, consumed_update=0, release_update=0, hit_update=0, bounce_update=0, landing_update=0;
    std::uint32_t ready_graph=0, hit_state=0, hit_path=0, landing_state=0, landing_path=0;
    std::uint32_t item_flags=0, item_object_flags=0;
    int item_kind=0, item_state=0, behavior=0, actual_surface=-1;
    IsleGameVec3 hit_root{}, bounce_root{}, landing{}, item_position{};
    float bounce_yaw=0, receiver_distance=0;
    ValleyGroundRevealForecast forecast{}, actual_forecast{};
};
struct ValleyGroundRevealFacts {
    int required_species=0;
    bool available=false, complete=false, exhausted=false, owns_pad=false, capture_pending=false;
    bool player_ready=false, pester_available=false, target_present=false, target_ready=false;
    bool subject_present=false, subject_hidden=false, subject_visible=false, subject_eligible=false, subject_focused=false;
    bool first38_accepted=false, second38_accepted=false, graveler_bounce_seen=false, graveler_landing_seen=false;
    bool receiver_acceptance_seen=false, reveal_seen=false, capture_observed=false, full_context_copied=false;
    // Emissions are Source-derived, NOT directly observed command-bus receipts.
    bool command20_source_derived=false, command38_source_derived=false, command39_source_derived=false;
    ValleyGroundRevealPhase phase=ValleyGroundRevealPhase::observe;
    ValleyGroundRevealDisposition disposition=ValleyGroundRevealDisposition::course_not_active;
    int current_block=-1, film=0, qualified=0, source_status=0, host_status=0;
    int focus_species=0, focus_context=-1, taken_species=0, taken_context=-1, evaluator_slot=-1;
    int raw_primary=0, captured_slot=-1, captured_evaluator_slot=-1;
    float block_progress=0, actual_receiver_distance=0;
    std::uint32_t target_graph=0, target_state=0, target_path=0, target_flags=0, target_object_flags=0;
    std::uint32_t graveler_graph=0, graveler_state=0, graveler_path=0, subject_graph=0, subject_state=0, subject_flags=0, subject_object_flags=0;
    std::uint32_t first38_before_graph=0, first38_after_graph=0, second38_before_graph=0, second38_after_state=0, second38_after_path=0;
    std::uint32_t receiver_before_graph=0, receiver_after_state=0;
    int graveler_landing_surface=-1;
    float graveler_bounce_yaw=0;
    ValleyGroundRevealIncarnation subject{}, offered_subject{}, graveler{}, registered_graveler{}, sender20{}, sender38_first{}, sender38_second{}, sender39{};
    ValleyGroundRevealIncarnation focus_actor{}, taken_actor{}, captured_actor{};
    std::uintptr_t offered_subject_object=0;
    IsleGameVec3 root{}, subject_root{}, graveler_root{}, graveler_landing{}, input_target{};
    IsleGameVec3 first38_root{}, second38_root{}, graveler_bounce_root{}, receiver_sender_root{}, receiver_root{};
    ValleyGroundRevealForecast graveler_forecast{};
    std::uint64_t initial_update=0, last_update=0, first38_update=0, second38_update=0, graveler_bounce_update=0;
    std::uint64_t graveler_landing_update=0, reveal_update=0, capture_update=0;
    std::uint64_t requests=0, pressed=0, consumed=0, releases=0, captured_slots=0, captures=0;
    std::uint64_t capture_mismatches=0, captured_identity_hash=0, steering_updates=0, recovery_updates=0, flight_updates=0, arc_rejections=0;
    unsigned active_throw=0;
    std::array<ValleyGroundRevealThrowReceipt,2> throws{};
    // Frozen only on coherent exposure; raw Source context and full captured image.
    IslePlayerPhotoContext captured_context{};
    IslePlayerPhotoData captured_image{};
};
namespace valley_ground_reveal_detail {
struct Actor {
    ValleyGroundRevealIncarnation identity{}, actor_sender{}, item_sender{};
    IsleGameObject* object=nullptr;
    IsleGameVec3 root{}, collision{};
    float yaw=0, radius=0, ground=0;
    int species=0, behavior=0, surface=-1;
    std::uint32_t graph=0, state=0, path=0, flags=0, object_flags=0, process_flags=0;
    bool tangible=false, ready=false, eligible=false, focused=false;
};
struct Item {
    ValleyGroundRevealIncarnation identity{};
    IsleGameVec3 position{};
    std::uint32_t flags=0, object_flags=0;
    int kind=0, state=0;
    float rest_timer=0;
};
struct Sphere { ValleyGroundRevealIncarnation identity{}; IsleGameVec3 center{}; float radius=0; };
struct Snapshot {
    std::array<Actor,10> actors{}; // 5 Geodudes, 1 registered Graveler, 3 Sandshrews, 1 Sandslash.
    std::array<Item,20> items{};
    std::array<Sphere,256> spheres{};
    std::size_t actor_count=0, item_count=0, sphere_count=0;
    bool capacity=true, copied=false;
    int focus_species=0, focus_context=-1, taken_species=0, evaluator_slot=-1;
    std::uint32_t focus_flags=0;
    ValleyGroundRevealIncarnation focus_actor{}, taken_actor{};
};
}
// B/Z/stick only. Parent observes every completed OM update, even during generic A ownership.
class ValleyGroundRevealPhotoPuzzle final {
public:
    ValleyGroundRevealPhotoPuzzle(private_post_course::Controller, Aim, int required_species);
    ValleyGroundRevealPhotoPuzzle(const ValleyGroundRevealPhotoPuzzle&)=delete;
    ValleyGroundRevealPhotoPuzzle& operator=(const ValleyGroundRevealPhotoPuzzle&)=delete;
    void observe(const application::RuntimeView&,std::uint64_t completed_updates);
    bool update(const application::RuntimeView&,std::uint64_t completed_updates);
    const ValleyGroundRevealFacts& facts() const noexcept { return facts_; }
private:
    void publish(std::uint16_t=0,std::int8_t=0,std::int8_t=0) noexcept;
    void stop(ValleyGroundRevealDisposition) noexcept;
    private_post_course::Controller controller_;
    Aim aim_;
    ValleyGroundRevealFacts facts_{};
    valley_ground_reveal_detail::Snapshot snapshot_{};
    IsleGame* scene_=nullptr;
    bool sampled_=false, control_sampled_=false, active_=false, started_=false;
    bool pending_press_=false, pending_release_=false, neutral_due_=false, pad_published_=false;
    bool capture_armed_=false, zoom_out_requested_=false;
    std::uint64_t previous_update_=0, control_update_=0, release_start_=0, capture_start_=0;
    std::uint32_t previous_subject_graph_=0, previous_graveler_graph_=0;
    std::array<std::uint32_t,2> previous_target_graph_{};
    int observed_roll_=-1, observed_qualified_=0, armed_slot_=-1, armed_qualified_=0;
    ValleyGroundRevealIncarnation armed_subject_{};
    std::array<ValleyGroundRevealIncarnation,20> prethrow_items_{};
    std::size_t prethrow_count_=0;
};
}
#endif
