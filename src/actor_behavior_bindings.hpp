#ifndef ISLE_ACTOR_BEHAVIOR_BINDINGS_HPP
#define ISLE_ACTOR_BEHAVIOR_BINDINGS_HPP

#include "actor_source.h"
#include "beach_behavior_resources.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace isle::actor {
// Exact compiled own-C callback identities, never converted guest addresses.
// Dependency spans are consumed during construction only. They describe native
// callback/graph/list references; factory/assets/services require Owner admission
// in addition to this callable-data closure before any process can execute.
struct BehaviorCallbackBinding {
    std::uint32_t identity = 0;
    IsleGameSourceCallback function = nullptr;
    std::span<const std::uint32_t> callbacks, graphs, weighted_states;
};
struct BoundWeightedStates {
    std::uint32_t source_identity = 0;
    std::span<const IsleActorRandomState> rows; // Includes native NULL terminal.
    std::int32_t total_weight = 0;
};
// Constructor-only immutable translation of a complete requested callable-data
// closure. Unresolved later/ineligible executable rows reject the whole owner.
// Native pointers, row bits/order and dependency cycles remain distinct from
// the decoder's retained guest provenance. No allocation or lookup occurs in C
// interpretation. Retain resources and its unmoved Snapshot through all users.
class BehaviorBindings final {
public:
    BehaviorBindings(const beach::BehaviorResources&,
        std::span<const BehaviorCallbackBinding>,
        std::span<const std::uint32_t> graphs,
        std::span<const std::uint32_t> weighted_states = {},
        std::span<const std::uint32_t> callbacks = {});
    BehaviorBindings(const BehaviorBindings&) = delete;
    BehaviorBindings& operator=(const BehaviorBindings&) = delete;
    BehaviorBindings(BehaviorBindings&&) = delete;
    BehaviorBindings& operator=(BehaviorBindings&&) = delete;

    const IsleActorInteractionGraph& graph(std::uint32_t identity) const;
    const BoundWeightedStates& weighted_states(std::uint32_t identity) const;
    IsleGameSourceCallback callback(std::uint32_t identity) const;
    bool contains(IsleGameSourceCallback) const;
    void require_live() const;
private:
    struct Graph {
        std::vector<IsleActorInteraction> rows;
        IsleActorInteractionGraph view{};
    };
    struct Weighted {
        std::vector<IsleActorRandomState> rows;
        BoundWeightedStates view;
    };
    struct Callback { std::uint32_t identity; IsleGameSourceCallback function; };
    const beach::BehaviorResources* resources_;
    std::vector<Graph> graphs_;
    std::vector<Weighted> weighted_;
    std::vector<Callback> callbacks_;
};
} // namespace isle::actor
#endif
