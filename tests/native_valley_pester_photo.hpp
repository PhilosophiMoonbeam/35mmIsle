#ifndef ISLE_PRIVATE_NATIVE_VALLEY_PESTER_PHOTO_HPP
#define ISLE_PRIVATE_NATIVE_VALLEY_PESTER_PHOTO_HPP
#include "native_progression_puzzles.hpp"
#include <array>
#include <cstddef>
#include <cstdint>

namespace isle::application { class Photos; }
namespace isle::private_campaign {
using ValleyPesterIncarnation=std::array<std::uint64_t,3>;
int choose_valley_pester_photo_species(application::Photos&);
enum class ValleyPesterPhase { observe, early_lure, early_leap, swimmer, shell, unzoom, aim, press, release, flight, reaction, reveal, capture, finished };
enum class ValleyPesterDisposition {
    course_not_active, player_not_ready, pester_unavailable, item_ui_not_ready, film_reserved,
    target_absent, target_not_ready, unsafe_geometry, item_outstanding, fish_latch_busy,
    press_timeout, release_timeout, flight_timeout, reaction_timeout, reveal_timeout,
    early_window_missed, mankey_latched, hit_unattributed, stage_unattributed, spawn_unattributed,
    reveal_unattributed, actor_lost, capture_pending, qualified, request_limit, steering_limit,
    recovery_limit, overall_limit, course_ended, source_error, capacity, none
};
struct ValleyPesterFacts {
    int required_species=0;
    bool available=false, complete=false, exhausted=false, owns_pad=false, capture_pending=false;
    bool player_ready=false, pester_available=false, target_present=false, target_ready=false;
    bool early_hit=false, early_leap=false, early_latch=false, mankey_latched=false;
    bool swimmer_hit=false, source_leap=false, fish_latch_busy=false, physical_ground=false;
    bool item_present=false, item_hidden=false, interaction_matches=false, reveal_seen=false;
    bool subject_present=false, subject_visible=false, subject_eligible=false, subject_focused=false;
    bool capture_observed=false, full_context_copied=false;
    ValleyPesterPhase phase=ValleyPesterPhase::observe;
    ValleyPesterDisposition disposition=ValleyPesterDisposition::course_not_active;
    int current_block=-1, target_behavior=-1, shell_stage=0, film=0, qualified=0;
    int focus_species=0, taken_species=0, raw_primary=0, focus_context=-1, taken_context=-1;
    int evaluator_slot=-1, captured_slot=-1, item_kind=0, item_state=0, source_status=0, host_status=0;
    std::uint32_t target_graph=0, target_state=0, target_animation=0, target_flags=0, target_object_flags=0;
    std::uint32_t subject_state=0, subject_animation=0, subject_flags=0, focus_flags=0;
    std::uint32_t material=0, predicted_material=0, item_flags=0, item_object_flags=0;
    float block_progress=0, player_distance=0, interaction_distance=0, animation_time=0;
    float predicted_flight=0, ground_player_distance=0, ground_target_distance=0, subject_path=0;
    IsleGameVec3 root{}, collision{}, motion{}, item_position{}, ground_position{}, input_target{};
    ValleyPesterIncarnation target{}, early_actor{}, swimmer{}, shell{}, subject{}, item{}, accepted_item{}, accepted_sender{};
    ValleyPesterIncarnation focus_actor{}, taken_actor{}, captured_actor{}, fish_latch{};
    ValleyPesterIncarnation early_item{}, swimmer_item{};
    std::array<ValleyPesterIncarnation,4> shell_items{};
    std::array<std::uint64_t,4> shell_hit_updates{}, shell_ready_updates{};
    std::array<std::uint32_t,4> shell_animations{};
    std::uint64_t initial_update=0, last_update=0, request_update=0, consumed_update=0, release_update=0;
    std::uint64_t physical_update=0, early_hit_update=0, early_leap_update=0, early_latch_update=0;
    std::uint64_t swimmer_hit_update=0, reveal_update=0, capture_update=0;
    std::uint64_t captured_slots=0, requests=0, consumed=0, releases=0, pressed=0;
    std::uint64_t accepted_hits=0, physical_landings=0, fish_attempts=0, captures=0, capture_mismatches=0;
    std::uint64_t steering_updates=0, recovery_updates=0, flight_updates=0, arc_rejections=0;
};
namespace valley_pester_detail {
struct Actor {
    ValleyPesterIncarnation identity{}, item_sender{}, actor_sender{};
    IsleGameObject* object=nullptr;
    IsleGameObject* interaction=nullptr;
    IsleGameVec3 root{}, collision{}, global_collision{};
    std::uint32_t graph=0, state=0, animation=0, flags=0, object_flags=0;
    float radius=0, player_distance=0, interaction_distance=0, animation_time=0, path=0;
    int species=0, behavior=0;
    bool visible=false, eligible=false, focused=false, tangible=false;
};
struct Item {
    ValleyPesterIncarnation identity{};
    IsleGameVec3 position{};
    std::uint32_t flags=0, object_flags=0, material=0;
    int kind=0, state=0;
    float rest_timer=0;
};
struct Sphere { ValleyPesterIncarnation identity{}; IsleGameVec3 center{}; float radius=0; };
struct Snapshot {
    IsleGameVec3 player_root{};
    std::array<Actor,64> actors{};
    std::array<Item,20> items{};
    std::array<Sphere,256> spheres{};
    std::size_t actor_count=0, item_count=0, sphere_count=0;
    bool capacity=true, copied=false;
    int focus_species=0, focus_context=-1, taken_species=0, evaluator_slot=-1;
    std::uint32_t focus_flags=0;
    ValleyPesterIncarnation focus_actor{}, taken_actor{};
};
}
// B/Z/stick only; generic photography retains all shutter/film ownership.
// Observe every completed actual OM update, even while another owner runs.
// Bound physical-plan targets retain neutral ownership through finite recovery
// and re-planning; absent, unready and unbound unsafe objectives still yield.
class ValleyPesterPhotoPuzzle final {
public:
    ValleyPesterPhotoPuzzle(private_post_course::Controller,Aim,int required_species);
    ValleyPesterPhotoPuzzle(const ValleyPesterPhotoPuzzle&)=delete;
    ValleyPesterPhotoPuzzle& operator=(const ValleyPesterPhotoPuzzle&)=delete;
    void observe(const application::RuntimeView&,std::uint64_t completed_updates);
    bool update(const application::RuntimeView&,std::uint64_t completed_updates);
    const ValleyPesterFacts& facts() const noexcept { return facts_; }
private:
    void publish(std::uint16_t=0,std::int8_t=0,std::int8_t=0) noexcept;
    bool relinquish() noexcept;
    void stop(ValleyPesterDisposition) noexcept;
    private_post_course::Controller controller_;
    Aim aim_;
    ValleyPesterFacts facts_{};
    valley_pester_detail::Snapshot snapshot_{};
    bool sampled_=false, control_sampled_=false, active_=false, started_=false, neutral_due_=false;
    bool pending_press_=false, pending_release_=false, tracking_=false, accepted_attempt_=false;
    bool ground_seen_=false, waiting_reaction_=false, capture_armed_=false, zoom_out_requested_=false;
    bool previous_latch_=false, previous_subject_hidden_=false, pad_published_=false;
    std::uint64_t previous_update_=0, control_update_=0, attempt_start_=0, reaction_start_=0;
    std::uint64_t release_start_=0, acquisition_updates_=0, recovery_=0;
    int observed_roll_=-1, observed_qualified_=0, armed_slot_=-1, armed_qualified_=0;
    ValleyPesterIncarnation armed_subject_{}, previous_target_{}, previous_subject_{};
    std::uint32_t previous_target_graph_=0;
    IsleGameObject* swimmer_object_=nullptr;
    IsleGameVec3 previous_global_collision_{};
    std::array<ValleyPesterIncarnation,20> prethrow_items_{};
    std::array<ValleyPesterIncarnation,64> prethrow_actors_{};
    std::size_t prethrow_item_count_=0, prethrow_actor_count_=0;
};
}
#endif
