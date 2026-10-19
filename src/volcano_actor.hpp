#ifndef ISLE_VOLCANO_ACTOR_HPP
#define ISLE_VOLCANO_ACTOR_HPP
#include "volcano_actor_source.h"
#include "actor_behavior_bindings.hpp"
#include <cstdint>
#include <span>
namespace isle::volcano {
enum class ActorFactoryWrapper { ground, ordinary, ground_dl4, ordinary_dl4 };
using ActorFactoryFunction=IsleActorReturningFactory;
struct ActorFactoryBinding {
    std::uint32_t actor_id,factory,initialization,initial_state;
    ActorFactoryWrapper wrapper;
    ActorFactoryFunction function;
    bool borrowed_common=false;
};
struct ActorChildFactoryDependency {
    std::uint32_t helper_identity,declaration_line,requested_id;
    IsleVolcanoDefinition definition;
    std::uint32_t written_fields;
    bool copy_path, from_geo;
};
/* Helpers retain exact Source-qualified origins and ABI tags. In particular a
 * returning utility, block callback, factory or unemitted empty helper is never
 * looked up or admitted as an interchangeable GObj callback. */
struct ActorHelperDependency {
    std::uint32_t identity;
    const char* source_path;
    std::uint32_t first,last;
    const char* native_signature;
};
struct ActorProviderDependency {
    std::uint32_t identity;
    const char* source_path;
    std::uint32_t first,last;
    const char* source_signature;
    const char* native_signature;
    const char* abi;
    std::span<const std::uint32_t> callbacks,graphs,weighted_states,headers,child_factories;
    std::span<const ActorHelperDependency> helpers;
    std::span<const char* const> services;
};
struct ActorAnimationBinding { std::uint32_t identity; IsleVolcanoAnimationCallback function; };
struct ActorRenderBinding { std::uint32_t identity; void (*function)(IsleGameObject*); std::uint8_t stream; };
struct ActorMovementBinding { std::uint32_t identity; void (*function)(IsleGameObject*); };
struct ActorPathBinding { std::uint32_t identity; void (*function)(IsleGameObject*,float*,float,float,int32_t); };
struct ActorBlockChangeBinding { std::uint32_t identity; void (*function)(IsleGameObject*,float,float,float,float,float,float); };
struct ActorCountedOption { std::uint8_t weight; IsleGameSourceCallback function; };
struct ActorCountedOptions { std::uint32_t identity; std::span<const ActorCountedOption> rows; };
std::span<const actor::BehaviorCallbackBinding> actor_callbacks() noexcept;
std::span<const std::uint32_t> actor_graphs() noexcept;
std::span<const std::uint32_t> actor_weighted_states() noexcept;
std::span<const std::uint32_t> actor_roots() noexcept;
std::span<const ActorFactoryBinding> actor_factories() noexcept;
std::span<const ActorChildFactoryDependency> actor_child_factories() noexcept;
std::span<const ActorProviderDependency> actor_dependencies() noexcept;
std::span<const ActorAnimationBinding> actor_animation_callbacks() noexcept;
std::span<const IsleItemCollisionCallback> actor_item_collision_callbacks() noexcept;
std::span<const ActorRenderBinding> actor_render_callbacks() noexcept;
std::span<const ActorMovementBinding> actor_movement_callbacks() noexcept;
std::span<const ActorPathBinding> actor_path_callbacks() noexcept;
std::span<const ActorBlockChangeBinding> actor_block_change_callbacks() noexcept;
std::span<const ActorCountedOptions> actor_counted_options() noexcept;
}
#endif
