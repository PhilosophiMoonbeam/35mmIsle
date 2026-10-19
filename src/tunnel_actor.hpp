#ifndef ISLE_TUNNEL_ACTOR_HPP
#define ISLE_TUNNEL_ACTOR_HPP
#include "tunnel_actor_source.h"
#include "egg_source.h"
#include "actor_behavior_bindings.hpp"
#include <cstdint>
#include <span>
namespace isle::tunnel {
enum class ActorFactoryWrapper { ground, ordinary, ordinary_dl4, ground_dl4 };
using ActorFactoryFunction = IsleGameObject* (*)(std::int32_t, std::uint16_t,
    IsleActorFactoryBlock*, IsleActorFactoryBlock*, IsleActorFactorySpawn*);
struct ActorFactoryBinding {
    std::uint32_t actor_id, factory, initialization, initial_state;
    ActorFactoryWrapper wrapper;
    ActorFactoryFunction function;
};
enum class SourceAbi { gobj_callback, factory_five_argument, animation_model_event,
    collision_node, collision_object, collision_vectors, gobj_vector_utility,
    item_ground_callback, float_utility, movement_callback, no_argument_utility,
    old_style_no_argument, lifecycle, file_local_empty };
struct ActorSourceOrigin { const char* path; const char* symbol; std::uint32_t first, last; };
/* ABI tags preserve utilities/factories/custom callbacks as distinct closure
 * requirements. No utility is smuggled into a GObj callback/function cast. */
struct ActorDependency {
    std::uint32_t identity;
    bool has_guest_identity; /* false: exact file/local provenance only. */
    SourceAbi abi;
    ActorSourceOrigin origin;
    const char* source_signature;
    std::span<const std::uint32_t> callbacks, graphs, weighted_states, headers;
    std::span<const std::uint32_t> exact_abi_dependencies;
    /* Original operation names include Source macros/common helpers. The
     * actual returning native service ABI is IsleTunnelActorServices; these
     * names are provenance, never a callback table or an admission exemption. */
    std::span<const char* const> source_external_operations;
};
struct ActorAutomaticOrigin { ActorSourceOrigin origin; std::uint32_t containing_function, declaration_line; };
struct ActorChildFactoryDependency {
    ActorAutomaticOrigin origin;
    IsleTunnelDefinition definition;
    bool from_geo;
};
/* Automatic instances have no guest address. initializer_identity names only
 * the actual static two-row object copied by Source, or zero for literals. */
struct ActorAutomaticGraphDependency {
    ActorAutomaticOrigin origin;
    std::uint32_t rows, initializer_identity;
};
struct ActorCountedOptions { std::uint32_t identity; std::span<const IsleTunnelCountedOption> rows; };
struct ActorAnimationBinding { std::uint32_t identity; IsleTunnelAnimationCallback function; };
struct ActorCollisionBinding { std::uint32_t identity; IsleTunnelCollisionCallback function; };
/* Imported once-only common Egg driver; invoke only from an admitted C continuation. */
struct ActorEggHelperBinding { std::uint32_t identity; void (*function)(IsleActorSource*, IsleEgg*, float); };
struct ActorRenderDependency { std::uint32_t identity; const char* symbol; const char* source_signature; };
std::span<const actor::BehaviorCallbackBinding> actor_callbacks() noexcept;
std::span<const std::uint32_t> actor_graphs() noexcept;
std::span<const std::uint32_t> actor_weighted_states() noexcept;
std::span<const std::uint32_t> actor_roots() noexcept;
std::span<const ActorFactoryBinding> actor_factories() noexcept;
std::span<const ActorChildFactoryDependency> actor_child_factories() noexcept;
std::span<const ActorDependency> actor_dependencies() noexcept;
std::span<const ActorCountedOptions> actor_counted_options() noexcept;
std::span<const ActorAnimationBinding> actor_animation_callbacks() noexcept;
std::span<const ActorCollisionBinding> actor_collision_callbacks() noexcept;
std::span<const std::uint32_t> actor_headers() noexcept;
std::span<const ActorRenderDependency> actor_render_dependencies() noexcept;
std::span<const ActorEggHelperBinding> actor_egg_helpers() noexcept;
std::span<const ActorAutomaticGraphDependency> actor_automatic_graphs() noexcept;
} // namespace isle::tunnel
#endif
