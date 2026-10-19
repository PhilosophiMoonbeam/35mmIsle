#ifndef ISLE_ACTOR_BEHAVIOR_BINDINGS_HPP
#define ISLE_ACTOR_BEHAVIOR_BINDINGS_HPP

#include "actor_source.h"
#include "actor_behavior_resources.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace isle::actor {
// Exact compiled own-C callback identities, never converted guest addresses.
// Dependency spans are consumed during construction only. They describe native
// callback/graph/list references; factory/assets/services require Owner admission
// in addition to this callable-data closure before any process can execute.
// For course 6 the compiled Rainbow provider catalog retains numeric utility,
// animation, collision and factory dependencies in their original typed seams;
// they are deliberately not admitted as GObj callbacks by this registry.
struct BehaviorCallbackBinding {
    std::uint32_t identity = 0;
    IsleGameSourceCallback function = nullptr;
    std::span<const std::uint32_t> callbacks, graphs, weighted_states;
};
// Closed provider views retain each course's actual typed callback registry.
std::span<const BehaviorCallbackBinding> course_callbacks(std::int32_t) noexcept;
std::span<const std::uint32_t> course_graphs(std::int32_t) noexcept;
std::span<const std::uint32_t> course_weighted_states(std::int32_t) noexcept;
std::span<const std::uint32_t> course_roots(std::int32_t) noexcept;
bool course_typed_dependency(std::int32_t, std::uint32_t) noexcept;
struct BoundCountedOption { std::uint8_t weight; IsleGameSourceCallback function; };
struct BoundCountedStates { std::uint32_t source_identity; std::span<const BoundCountedOption> rows; };
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
    BehaviorBindings(const BehaviorResources&,
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
    const BoundCountedStates& counted_state_options(std::uint32_t identity) const;
    IsleGameSourceCallback callback(std::uint32_t identity) const;
    bool contains(IsleGameSourceCallback) const;
    // Rechecks mutable/caller-retained rows and every executable callback's
    // constructor-resolved transitive closure. The predicate must additionally
    // prove actual compiled source, factory/assets and service admission.
    // No allocation, behavior invocation or guest-function conversion occurs.
    bool admits_native_graph(const IsleActorInteractionGraph&,
        bool (*admit)(IsleGameSourceCallback, void*), void*) const;
    void require_live() const;
    std::int32_t course_id() const noexcept { return resources_->course_id(); }
private:
    struct Graph {
        std::vector<IsleActorInteraction> rows;
        IsleActorInteractionGraph view{};
    };
    struct Weighted {
        std::vector<IsleActorRandomState> rows;
        BoundWeightedStates view;
    };
    struct Counted { std::vector<BoundCountedOption> rows; BoundCountedStates view; };
    struct Callback {
        std::uint32_t identity;
        IsleGameSourceCallback function;
        std::vector<IsleGameSourceCallback> closure;
    };
    const BehaviorResources* resources_;
    std::vector<Graph> graphs_;
    std::vector<Weighted> weighted_;
    std::vector<Counted> counted_;
    std::vector<Callback> callbacks_;
};
} // namespace isle::actor
#endif
