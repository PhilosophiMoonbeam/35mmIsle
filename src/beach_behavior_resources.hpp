#ifndef ISLE_BEACH_BEHAVIOR_RESOURCES_HPP
#define ISLE_BEACH_BEHAVIOR_RESOURCES_HPP

#include "rom_snapshot.hpp"
#include "rom_view.hpp"

#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace isle::beach {
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
struct SceneChoice {
    rom::GuestView wire;
    std::uint8_t weight = 0;
    // NULL is a weighted no-spawn choice, not a terminator.
    std::uint32_t function_identity = 0;
};
struct SceneChooser {
    rom::GuestView wire;
    std::array<SceneChoice, 2> entries;
    std::int32_t total_weight = 0;
};
// Constructor-only metadata. Retain the original, unmoved Snapshot until this
// owner and every borrowed view/reference are finished, as with ActorResources.
// Moving the Snapshot invalidates access; destroying it first violates this
// borrowed lifetime contract. No executable installation or interpreter exists.
class BehaviorResources final {
public:
    explicit BehaviorResources(const rom::Snapshot&);
    BehaviorResources(rom::Snapshot&&) = delete;
    BehaviorResources(const BehaviorResources&) = delete;
    BehaviorResources& operator=(const BehaviorResources&) = delete;
    BehaviorResources(BehaviorResources&&) = delete;
    BehaviorResources& operator=(BehaviorResources&&) = delete;

    std::span<const InteractionGraph> graphs() const;
    std::span<const WeightedStates> weighted_states() const;
    const SceneChooser& scene_chooser() const;
    // Exact admitted identities only: NULL, interior records and other guest
    // addresses throw out_of_range. Repeated lookups return the same object.
    const InteractionGraph& graph(std::uint32_t identity) const;
    const WeightedStates& weighted_states(std::uint32_t identity) const;
    const SceneChooser& scene_chooser(std::uint32_t identity) const;
    void require_live() const;
private:
    const rom::Snapshot* snapshot_;
    const std::byte* source_data_;
    std::vector<InteractionGraph> graphs_;
    std::vector<WeightedStates> weighted_states_;
    SceneChooser scene_chooser_;
};
} // namespace isle::beach
#endif
