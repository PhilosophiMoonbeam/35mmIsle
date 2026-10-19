#ifndef ISLE_PRIVATE_NATIVE_TUNNEL_PHOTO_PUZZLES_HPP
#define ISLE_PRIVATE_NATIVE_TUNNEL_PHOTO_PUZZLES_HPP

#include "native_progression_puzzles.hpp"
#include <array>

namespace isle::private_campaign {
enum class TunnelPhotoPhase { idle, diglett, dugtrio, magnemite, apple, magneton, release };
struct TunnelPhotoFacts {
    TunnelPhotoPhase phase = TunnelPhotoPhase::idle;
    bool owns_pad = false, player_ready = false, apple_available = false;
    // Source IDs in array order: Diglett50, Dugtrio51, Magnemite81, Magneton82.
    // Saved presence is canonical Report-photo presence, never score inference.
    std::array<bool, 4> saved_reports{};
    std::array<std::uint32_t, 4> shutter_requests{}, shutters_consumed{}, captures{};
    // captures counts canonical raw-primary species candidates in qualifying
    // Tunnel slots, not requested species, positive scores or saved Reports.
    std::array<std::uint32_t, 4> requested_exposures{};
    std::uint32_t unknown_primary_exposures = 0, requested_primary_mismatches = 0;
    int last_capture_slot = -1, last_capture_raw_subject_zero = 0, last_capture_requested_id = 0;
    std::array<bool, 3> rise_accepted{}, magnemite_lure_seen{};
    std::array<std::uint32_t, 3> rise_shutter_requests{};
    int diglett_stage = 0, active_diglett_role = 0, num_connected = 0, max_connected = 0;
    unsigned stage_values_seen = 0;
    std::uint32_t diglett_stage_restarts = 0, zoom_recovery_updates = 0;
    bool zoom_recovery_active = false, zoom_recovery_exhausted = false;
    std::array<std::uint32_t, 3> rise_visible_updates{}, rise_ready_updates{},
        rise_focus_updates{}, rise_cooldown_updates{};
    unsigned diglett_roles_seen = 0, magnemite_ready_mask = 0, magnemite_zoom_mask = 0,
        magnemite_luring_mask = 0, magnemite_arrived_mask = 0, magnemite_attracted_mask = 0;
    float nearest_magnemite_distance = -1;
    bool apple_item_present = false, apple_flight_active = false, apple_flight_exhausted = false;
    int apple_item_state = 0;
    unsigned apple_item_flags = 0, apple_near_mask = 0, apple_plan_near_mask = 0;
    std::uint32_t apple_item_id = 0, apple_flight_updates = 0, apple_landings_seen = 0,
        apple_lost_flights = 0, apple_arc_rejections = 0;
    std::uint64_t apple_item_pool = 0, apple_item_incarnation = 0;
    std::uint32_t apple_item_slot = UINT32_MAX;
    // Per-observation planning result: 0 not attempted, 1 insufficient roots,
    // 2 no lure region, 3 no clear landing arc, 4 ready. Never Source credit.
    int apple_plan_result = 0;
    std::uint32_t apple_plan_candidates = 0, apple_plan_arc_rejections = 0;
    std::array<float, 3> apple_root_distances{{-1, -1, -1}};
    IsleGameVec3 apple_position{}, apple_predicted_landing{};
    bool diglett_photo_taken = false, dugtrio_photo_taken = false;
    bool dugtrio_rise_seen = false, magneton_spawn_seen = false;
    bool group_transition_seen = false, magnemite_recoil_seen = false;
    bool target_present = false, target_eligible = false, focus_eligible = false;
    int target_id = 0, target_role = 0;
    std::uint32_t target_graph = 0, apple_requests = 0, apples_consumed = 0;
    IsleGameVec3 target_position{}, apple_ground{};
};
class TunnelPhotoPuzzles final {
public:
    TunnelPhotoPuzzles(private_post_course::Controller, Aim);
    TunnelPhotoPuzzles(const TunnelPhotoPuzzles&) = delete;
    TunnelPhotoPuzzles& operator=(const TunnelPhotoPuzzles&) = delete;
    bool update(const application::RuntimeView&, const game::State&, std::uint64_t completed_updates);
    const TunnelPhotoFacts& facts() const noexcept { return facts_; }
private:
    void publish(std::uint16_t = 0, std::int8_t = 0, std::int8_t = 0) noexcept;
    private_post_course::Controller controller_;
    Aim aim_;
    TunnelPhotoFacts facts_{};
    std::uint64_t last_update_ = 0;
    bool sampled_ = false, reports_sampled_ = false, pending_apple_ = false;
    bool release_pending_ = false, release_zoomed_ = false;
    bool apple_recovery_ = false;
    bool apple_tracking_ = false, apple_landed_ = false;
    // Identity-only scalar comparisons; never passed back as native handles.
    std::array<std::uint64_t, 3> apple_tracked_identity_{};
    std::array<std::array<std::uint64_t, 3>, 20> apple_prethrow_identities_{};
    std::size_t apple_prethrow_count_ = 0;
    std::uint32_t apple_flight_updates_ = 0, apple_response_updates_ = 0;
    int observed_roll_ = 0, pending_shot_ = -1, pending_slot_ = -1;
};
} // namespace isle::private_campaign
#endif
