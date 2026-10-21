#ifndef ISLE_PRIVATE_NATIVE_BEACH_PESTER_PHOTO_HPP
#define ISLE_PRIVATE_NATIVE_BEACH_PESTER_PHOTO_HPP

#include "native_progression_puzzles.hpp"
#include <array>
#include <cstddef>
#include <cstdint>

namespace isle::application { class Photos; }
namespace isle::private_campaign {
using BeachPesterIncarnation = std::array<std::uint64_t,3>;
int choose_beach_pester_photo_species(application::Photos&);
enum class BeachPesterPhase { observe, unzoom, aim, press, release, flight, reaction, capture, finished };
enum class BeachPesterDisposition {
    course_not_active, player_not_ready, pester_unavailable, film_reserved,
    target_absent, target_not_ready, unsafe_geometry, item_outstanding,
    press_timeout, release_timeout, item_ambiguous, item_lost, flight_timeout,
    reaction_unattributed, reaction_timeout, capture_pending, window_closed,
    actor_lost, cleanup, progress_cleanup, steering_limit, recovery_limit,
    request_limit, overall_limit, capacity, course_ended, source_error, qualified, none
};
struct BeachPesterForecast {
    bool accepted=false, ground=false;
    int surface=-1, static_object=-1;
    std::uint32_t updates=0;
    float actor_distance=0;
    IsleGameVec3 contact{};
};
struct BeachPesterFacts {
    int required_species=0;
    bool available=false, complete=false, exhausted=false, owns_pad=false;
    // Parent must skip old Beach puzzles while this actual window is live.
    bool capture_pending=false;
    bool player_ready=false, pester_available=false, target_present=false, target_ready=false;
    bool hidden_seen=false, item_present=false, physical_seen=false, sender_seen=false;
    bool reaction_seen=false, reveal_seen=false, eligible=false, focused=false, capture_observed=false;
    bool cleanup_seen=false;
    BeachPesterPhase phase=BeachPesterPhase::observe;
    BeachPesterDisposition disposition=BeachPesterDisposition::course_not_active;
    int current_block=-1, target_behavior=-1, film=0, qualified=0, captured_slot=-1;
    int focus_species=0, focus_context=-1, taken_context=-1, raw_primary=0, taken_species=0;
    int item_kind=0, item_state=0, source_status=0;
    // Genuine post-course evaluator is owned by the parent; never inferred here.
    int evaluated_species=-1;
    std::uint32_t target_graph=0, target_state=0, target_flags=0, target_object_flags=0;
    std::uint32_t target_object_id=0, process_flags=0, model_animation=0, focus_flags=0;
    std::uint32_t material_animation=0;
    std::uint32_t item_flags=0, item_object_flags=0, item_object_id=0;
    float collision_radius=0, animation_time=0, progress=0, accepted_distance=0;
    IsleGameVec3 root{}, collision{}, global_root{}, motion{}, input_target{}, surface_target{}, item_position{};
    BeachPesterForecast ideal_forecast{}, actual_forecast{};
    BeachPesterIncarnation target{}, item{}, accepted_item{}, interaction_sender{}, focus_actor{}, taken_actor{}, captured_actor{};
    std::uint64_t initial_update=0, last_update=0, hidden_update=0, request_update=0;
    std::uint64_t consumed_update=0, release_update=0, physical_update=0, sender_update=0;
    std::uint64_t reaction_update=0, reveal_update=0, capture_update=0, cleanup_update=0;
    std::uint64_t captured_slots=0, requests=0, pressed=0, consumed=0, releases=0;
    std::uint64_t captures=0, capture_mismatches=0, full_copy_mismatches=0;
    std::uint64_t steering_updates=0, recovery_updates=0, flight_updates=0, forecast_rejections=0;
};
namespace beach_pester_detail {
struct Actor {
    BeachPesterIncarnation identity{}, interaction{};
    IsleGameVec3 root{}, collision{};
    float radius=0, animation_time=0, distance=0, interaction_distance=0;
    std::uint32_t graph=0, state=0, flags=0, object_flags=0, object_id=0, process_flags=0, model_animation=0;
    std::uint32_t material_animation=0;
    int behavior=0;
    bool ready=false, hidden=false, reaction=false, eligible=false, focused=false;
};
struct Sphere { BeachPesterIncarnation identity{}; IsleGameVec3 center{}; float radius=0; };
struct Item {
    BeachPesterIncarnation identity{};
    IsleGameVec3 position{}, previous{};
    int kind=0, state=0;
    std::uint32_t flags=0, object_flags=0, object_id=0;
    float rest_timer=0;
};
}
// Physical Pester only; no A ownership, Report writes or synthetic reveal.
class BeachPesterPhotoPuzzle final {
public:
    BeachPesterPhotoPuzzle(private_post_course::Controller, Aim, int required_species);
    BeachPesterPhotoPuzzle(const BeachPesterPhotoPuzzle&)=delete;
    BeachPesterPhotoPuzzle& operator=(const BeachPesterPhotoPuzzle&)=delete;
    void observe(const application::RuntimeView&, std::uint64_t actual_completed_updates);
    bool update(const application::RuntimeView&, std::uint64_t actual_completed_updates);
    const BeachPesterFacts& facts() const noexcept { return facts_; }
private:
    void publish(std::uint16_t=0, std::int8_t=0, std::int8_t=0) noexcept;
    void stop(BeachPesterDisposition) noexcept;
    private_post_course::Controller controller_;
    Aim aim_;
    BeachPesterFacts facts_{};
    std::array<beach_pester_detail::Actor,16> actors_{};
    std::array<beach_pester_detail::Sphere,256> spheres_{};
    std::array<beach_pester_detail::Item,20> items_{};
    std::array<BeachPesterIncarnation,20> prethrow_items_{};
    std::size_t actor_count_=0, sphere_count_=0, item_count_=0, prethrow_count_=0;
    bool sampled_=false, control_sampled_=false, active_=false, started_=false, engaged_=false;
    bool pending_press_=false, pending_release_=false, tracking_=false, capture_armed_=false;
    bool neutral_pending_=false, zoom_out_requested_=false, previous_hidden_=false, previous_reaction_=false;
    bool previous_ground_=false;
    std::uint64_t observation_update_=0, control_update_=0, release_start_=0;
    std::uint64_t steering_=0, recovery_=0;
    int observed_roll_=-1, observed_qualified_=0, armed_slot_=-1, armed_qualified_=0;
    BeachPesterIncarnation previous_target_{}, previous_item_{}, armed_subject_{};
    IsleGameVec3 previous_global_root_{};
};
}
#endif
