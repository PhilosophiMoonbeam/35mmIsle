#ifndef ISLE_ACTOR_BEHAVIOR_RESOURCES_HPP
#define ISLE_ACTOR_BEHAVIOR_RESOURCES_HPP

#include "rom_snapshot.hpp"
#include "rom_view.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace isle::actor {
// Guest function identities are immutable data, never native callback bindings.
struct Interaction {
    rom::GuestView wire;
    std::int32_t command = 0;
    std::uint32_t state_identity = 0;
    float value = 0;
    std::uint32_t aux_identity = 0;
};
struct InteractionGraph {
    // Original contiguous initialized records, including the cmd58 record.
    rom::GuestView wire, terminator;
    std::vector<Interaction> entries;
};
struct WeightedState {
    rom::GuestView wire;
    std::int32_t weight = 0;
    std::uint32_t state_identity = 0;
};
struct WeightedStates {
    // The NULL-state terminator's weight is not consumed or interpreted.
    rom::GuestView wire, terminator;
    std::vector<WeightedState> entries;
    std::int32_t total_weight = 0;
};
namespace detail { struct BehaviorCatalog; }

// Constructor-only immutable data for a closed course catalog (Beach 0 or
// Rainbow 6). Retain the original, unmoved Snapshot until this owner and every
// borrowed view/reference are finished. Moving the Snapshot invalidates access;
// destroying it first violates the borrowed lifetime contract. Data decoding
// grants no factory, callback, interpreter or live ActorAI admission. Beach's
// scene chooser is separate course-specific data, not constructed here.
class BehaviorResources final {
public:
    BehaviorResources(const rom::Snapshot&, std::int32_t course_id);
    BehaviorResources(rom::Snapshot&&, std::int32_t) = delete;
    BehaviorResources(const BehaviorResources&) = delete;
    BehaviorResources& operator=(const BehaviorResources&) = delete;
    BehaviorResources(BehaviorResources&&) = delete;
    BehaviorResources& operator=(BehaviorResources&&) = delete;

    std::int32_t course_id() const noexcept;
    std::span<const InteractionGraph> graphs() const;
    std::span<const WeightedStates> weighted_states() const;
    // Exact admitted roots only: NULL, interior records and other guest
    // addresses throw out_of_range. Repeated lookups return the same object.
    const InteractionGraph& graph(std::uint32_t identity) const;
    const WeightedStates& weighted_states(std::uint32_t identity) const;
    void require_live() const;
private:
    const rom::Snapshot* snapshot_;
    const std::byte* source_data_;
    const detail::BehaviorCatalog* catalog_;
    std::vector<InteractionGraph> graphs_;
    std::vector<WeightedStates> weighted_states_;
};
} // namespace isle::actor
#endif
