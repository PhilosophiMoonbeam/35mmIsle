#ifndef ISLE_PRIVATE_NATIVE_VOLCANO_FOOD_PHOTO_HPP
#define ISLE_PRIVATE_NATIVE_VOLCANO_FOOD_PHOTO_HPP

#include "native_progression_puzzles.hpp"
#include <array>

namespace isle::private_campaign {
enum class VolcanoFoodPhase { dormant, aiming, throw_receipt, approach, framing, shutter_receipt, release, closed };
enum class VolcanoFoodEnd { none, complete, preempted, actor_lost, aim_timeout, throw_mismatch,
    food_lost, approach_timeout, framing_timeout, film_reserved, capture_mismatch, release_timeout };
struct VolcanoFoodFacts {
    VolcanoFoodPhase phase = VolcanoFoodPhase::dormant;
    VolcanoFoodEnd end = VolcanoFoodEnd::none;
    bool checked = false, prior_reports = false, owns_pad = false, present = false;
    bool settled = false, adopted = false, approached = false, with_apple = false;
    bool coherent = false, complete = false, flying_ground_contact = false;
    std::array<std::uint64_t, 3> actor{}, item{}, apple{}, interaction{};
    std::uint32_t graph = 0, state = 0, path = 0, valid_items = 0;
    unsigned plans = 0, candidates = 0, ground_rejections = 0, arc_rejections = 0, corridor_rejections = 0;
    unsigned portrait_rejections = 0;
    unsigned predicted_bounces = 0;
    unsigned throw_requests = 0, throw_presses = 0, throws_consumed = 0;
    unsigned shutter_requests = 0, shutter_presses = 0, releases = 0, mismatches = 0;
    std::uint64_t phase_started = 0, deadline = 0, controls = 0, aligned_updates = 0;
    std::uint64_t throw_request = 0, throw_consumed = 0, throw_release = 0;
    std::uint64_t settled_update = 0, adopted_update = 0, approached_update = 0, with_apple_update = 0;
    std::uint64_t flying_ground_contact_update = 0;
    std::uint64_t shutter_request = 0, capture_update = 0, shutter_release = 0;
    int film_before = 0, qualified_before = 0, slot = -1, decision_context = -1, taken_context = -1;
    int surface = -1;
    float yaw = 0, facing_error = 0;
    IsleGameVec3 position{}, collision{}, candidate_global{}, predicted_contact{}, predicted_stop{};
};
class VolcanoFoodPhoto final {
public:
    VolcanoFoodPhoto(private_post_course::Controller, Aim);
    // Baseline arbitration passes false without transferring neutral input.
    // An already-spent attempt closes rather than resuming after preemption.
    bool update(const application::RuntimeView&, const game::State&, std::uint64_t, bool available);
    const VolcanoFoodFacts& facts() const noexcept { return facts_; }
private:
    void publish(std::uint16_t = 0, std::int8_t = 0, std::int8_t = 0) noexcept;
    private_post_course::Controller controller_;
    Aim aim_;
    VolcanoFoodFacts facts_{};
    bool sampled_ = false;
    std::uint64_t last_update_ = 0;
};
} // namespace isle::private_campaign
#endif
