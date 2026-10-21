#ifndef ISLE_PRIVATE_NATIVE_VOLCANO_MAGMAR_PAIR_HPP
#define ISLE_PRIVATE_NATIVE_VOLCANO_MAGMAR_PAIR_HPP

#include "native_progression_puzzles.hpp"
#include <array>

namespace isle::private_campaign {
using MagmarPairKey = std::array<std::uint64_t,3>;
enum class MagmarPairPhase { dormant, aiming, throw_receipt, convergence, framing, shutter_receipt, release, closed };
enum class MagmarPairEnd {
    none, complete, prerequisites, activation_missing, pair_invalid, actor_lost, retired,
    preempted, plan_exhausted, aim_timeout, geometry_lost, throw_mismatch,
    throw_release_timeout, food_lost, unsafe_settlement, wrong_partner,
    interaction_history, interaction_lost, convergence_timeout, framing_timeout,
    film_reserved, capture_mismatch, release_timeout
};
struct MagmarPairSubject {
    int pokemon = 0, animation = -1, pose = 0, special = 0;
    float time = 0, yaw = 0;
    IsleGameVec3 position{};
};
struct MagmarPairFacts {
    MagmarPairPhase phase = MagmarPairPhase::dormant;
    MagmarPairEnd end = MagmarPairEnd::none;
    bool checked = false, prior_reports = false, owns_pad = false, pair_present = false;
    bool capacity = true, moltres_appeared = false, settled = false, overlap = false;
    bool accepted_interaction = false, special11 = false, coherent = false, complete = false;
    bool flying_ground_contact = false, prospective_overlap = false;
    std::array<MagmarPairKey,2> actor{}, apple{}, food_target{}, interaction{};
    MagmarPairKey moltres{}, item{}, aggressor{}, partner{}, taken_actor{};
    std::array<bool,2> present{}, wait_seen{}, activated{}, adopted{}, running_seen{}, accepting26{};
    std::array<bool,2> accepted26{};
    std::array<std::uint32_t,2> acceptance_graph{};
    std::array<std::uint64_t,2> acceptance_update{};
    std::array<float,2> acceptance_world_time{};
    float taken_world_time = 0;
    std::array<std::uint32_t,2> graph{}, state{}, path{}, model_anims{}, process_flags{};
    std::array<std::uint64_t,2> wait_update{}, activation_update{}, adoption_update{}, running_update{};
    std::array<IsleGameVec3,2> position{}, collision{}, predicted_stop{};
    std::array<float,2> radius{}, yaw{};
    std::array<unsigned,2> predicted_steps{};
    std::array<MagmarPairSubject,2> live{};
    MagmarPairSubject taken{};
    unsigned pair_count = 0, valid_items = 0, plans = 0, candidates = 0;
    unsigned ground_rejections = 0, arc_rejections = 0, corridor_rejections = 0;
    unsigned race_rejections = 0, portrait_rejections = 0, predicted_bounces = 0;
    unsigned predicted_arrival_delta = 0, throw_requests = 0, throw_presses = 0, throws_consumed = 0;
    unsigned shutter_requests = 0, shutter_presses = 0, releases = 0, mismatches = 0;
    std::uint64_t phase_started = 0, deadline = 0, convergence_deadline = 0, controls = 0, aligned_updates = 0;
    std::uint64_t moltres_update = 0, throw_request = 0, throw_consumed = 0, throw_release = 0;
    std::uint64_t settled_update = 0, overlap_update = 0, interaction_update = 0;
    std::uint64_t flying_ground_contact_update = 0, shutter_request = 0, capture_update = 0, shutter_release = 0;
    int aggressor_index = -1, film_before = 0, qualified_before = 0;
    int slot = -1, decision_context = -1, taken_context = -1, surface = -1;
    float predicted_overlap_margin = 0;
    IsleGameVec3 candidate_global{}, predicted_contact{}, actual_settlement{};
};
class MagmarPairPhoto final {
public:
    MagmarPairPhoto(private_post_course::Controller, Aim);
    // Call every completed update, including unavailable updates: passive
    // WaitMoltres lineage observation must precede the optional input window.
    // False availability never publishes neutral over a higher-priority owner.
    bool update(const application::RuntimeView&, const game::State&, std::uint64_t, bool available);
    const MagmarPairFacts& facts() const noexcept { return facts_; }
private:
    void publish(std::uint16_t = 0, std::int8_t = 0, std::int8_t = 0) noexcept;
    private_post_course::Controller controller_;
    Aim aim_;
    MagmarPairFacts facts_{};
    bool sampled_ = false;
    std::uint64_t last_update_ = 0;
    bool observed_ = false;
    std::uint64_t last_observation_ = 0;
    // Owned captured Source values, not retained context/scene pointers. The
    // taken context may differ from the decision context after render lag.
    std::array<std::array<MagmarPairSubject,2>,2> request_context_{};
    std::array<std::array<bool,2>,2> request_context_present_{};
    MagmarPairSubject request_live_{};
};
} // namespace isle::private_campaign
#endif
