#ifndef ISLE_PRIVATE_NATIVE_SIGN_PUZZLES_HPP
#define ISLE_PRIVATE_NATIVE_SIGN_PUZZLES_HPP

#include "native_progression_puzzles.hpp"
#include "native_sign_geometry.hpp"
#include <array>
#include <span>
#include <string_view>

namespace isle::private_campaign {
// Private solver ID order. Canonical Report slots are63+evidence.course.
inline constexpr std::array<int,6> sign_ids{1004,1010,1018,1022,1028,1035};
struct SignEvidence {
    int id, course;
    std::uint32_t model_identity;
    std::string_view source, producer, capture;
};
[[nodiscard]] std::span<const SignEvidence,6> sign_evidence() noexcept;
enum class SignDependency {
    none, already_reported, course_not_active, player_not_ready, dash_reward,
    apple_reward, pester_reward, flute_reward, item_ui_not_ready,
    sign_not_present, producer_not_present, producer_not_revealed,
    producer_not_ready, flute_out_of_range, pikachu_egg_separation,
    lure_ground, volcano_lava_surface, projectile_out_of_reach,
    source_visibility_window, focus_membership, camera_turning,
    camera_zoom, shutter_cooldown, physical_release, film_reserve, geometry_not_ready
};
enum class SignProducerState {
    absent, waiting, apple_pursuit, flute_charge, egg_hatching,
    zapdos_flight, powered, vileplume_reveal, smoke_spawn, smoke_face
};
struct SignFacts {
    int id = 0, course = -1, initial_photo_count = -1, photo_count = -1;
    int capture_slot = -1, recorded_primary = 0, producer_id = 0;
    SignDependency dependency = SignDependency::course_not_active;
    SignProducerState producer_state = SignProducerState::absent;
    bool owns_pad = false, player_ready = false;
    bool dash = false, apple = false, pester = false, flute = false;
    bool sign_present = false, sign_capture_flag = false;
    bool producer_present = false, producer_revealed = false;
    bool powered = false, egg_hatching = false, smoke_cleared = false;
    bool trigger_surface = false, flute_playing = false;
    bool focus_authorized = false, capture_observed = false;
    bool geometry_ready = false;
    unsigned geometry_kind = 0, geometry_rejection = 0;
    std::uint64_t geometry_frame = 0;
    std::uint16_t sign_flags = 0;
    std::uint32_t sign_graph = 0, producer_graph = 0;
    std::uint32_t shutter_requests = 0, shutters_consumed = 0;
    std::uint32_t item_requests = 0, items_consumed = 0;
    float producer_player_distance = 0, pikachu_egg_distance = 0;
    IsleGameVec3 sign_position{}, producer_position{}, input_target{};
};
// One instance per trip. Only missing signs reserve the pad. Parent owns all
// update/failure bounds, canonical slot/digest receipts, Camera/Photo/Report,
// course completion and teardown. A successful request is never capture proof.
// The parent's Aim bridge physically acquires observed geometry hints; real Item
// arcs must clear intervening terrain/Actors after stick quantization, and live
// input/render cadence must reach each finite Source visibility/reveal window.
// Dependencies and capture facts, not elapsed updates, adjudicate outcomes.
class SignPuzzles final {
public:
    SignPuzzles(int actual_course, std::array<bool,6> already_reported,
                private_post_course::Controller, Aim);
    SignPuzzles(const SignPuzzles&) = delete;
    SignPuzzles& operator=(const SignPuzzles&) = delete;
    // Only while an actual completed canonical Frame prevents further OM work.
    void frame(const application::RuntimeView&, std::uint64_t ordinal);
    void retire() noexcept;
    bool update(const application::RuntimeView&, const game::State&,
                std::uint64_t completed_updates);
    const SignFacts& facts() const noexcept { return facts_; }
private:
    enum class Press { none, apple, pester, flute, shutter };
    void publish(std::uint16_t = 0, std::int8_t = 0, std::int8_t = 0) noexcept;
    bool own(SignDependency, std::uint16_t = 0, std::int8_t = 0, std::int8_t = 0) noexcept;
    bool zoom(const IslePlayerSource&, bool desired);
    bool throw_item(const IslePlayerSource&, const IsleGameVec3&, Press);
    bool play_flute(const IslePlayerSource&);
    int index_ = -1;
    bool reported_ = false, sampled_ = false, baseline_ = false;
    std::uint64_t last_update_ = 0;
    private_post_course::Controller controller_;
    Aim aim_;
    SignFacts facts_{};
    Press pending_ = Press::none;
    std::uint16_t release_button_ = 0;
    int requested_photo_count_ = -1;
    bool lava_cached_ = false;
    IsleGameVec3 global_lava_{};
    test::SignGeometry geometry_;
    std::uint64_t geometry_entry_ = 0;
    IsleGameVec3 geometry_global_{};
};
} // namespace isle::private_campaign
#endif
