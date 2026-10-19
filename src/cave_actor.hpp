#ifndef ISLE_CAVE_ACTOR_HPP
#define ISLE_CAVE_ACTOR_HPP
#include "cave_actor_source.h"
#include "egg_source.h"
#include "cave_lifecycle_source.h"
#include "actor_behavior_bindings.hpp"
#include <cstdint>
#include <span>
namespace isle::cave {
enum class ActorFactoryWrapper : std::uint8_t { ground, ordinary, ordinary_dl4 };
using ActorSixArgumentFactory=IsleGameObject* (*)(std::int32_t,std::uint16_t,IsleActorFactoryBlock*,IsleActorFactoryBlock*,IsleActorFactorySpawn*,const IsleActorFactoryInitialization*);
struct ActorFactoryBinding {
    std::uint32_t actor_id,factory,initialization,initial_state;
    ActorFactoryWrapper wrapper;
    IsleActorReturningFactory returning;
    IsleActorVoidFactory nonreturning;
    ActorSixArgumentFactory original_six_argument;
    bool sixth_argument_unused;
};
enum class ActorSourceAbi : std::uint8_t { callback, utility, factory6, factory5, void_factory5, object_utility, animation, model_utility, common_egg_driver };
struct ActorDependencyEvent { const char* symbol; std::uint32_t identity,line; bool indirect; };
struct ActorTypedDependency { std::uint32_t identity; ActorSourceAbi abi; const char* source_signature; };
struct ActorProviderDependency {
    std::uint32_t identity; ActorSourceAbi abi; bool lifecycle;
    const char* path; std::uint32_t first,last; const char* source_signature;
    std::span<const ActorDependencyEvent> calls;
    std::span<const ActorTypedDependency> typed_helpers;
    std::span<const std::uint32_t> callbacks,graphs,weighted_states,headers,child_factories;
};
struct ActorChildFactoryDependency { std::uint32_t helper_identity; IsleCaveDefinition definition; std::uint32_t declaration_line,written_fields; const char* path; };
struct ActorHeaderBinding { std::uint32_t initialization,header; };
struct ActorWeightedBinding { std::uint32_t identity; std::span<const IsleActorRandomState> rows; };
struct ActorCountedOptionsBinding { std::uint32_t identity; std::span<const IsleCaveCountedOption> rows; };
struct ActorAutomaticAggregate { std::uint32_t owner_identity,declaration_line,count; const char* path; const char* name; const char* source_type; };
struct ActorAnimationBinding { std::uint32_t identity; IsleCaveAnimationCallback function; };
struct LifecycleAnimationBinding { std::uint32_t identity; IsleCaveLifecycleAnimationCallback function; };
/* Imported once-only common Egg driver; invoke only from an admitted C continuation. */
struct ActorEggHelperBinding { std::uint32_t identity; void (*function)(IsleActorSource*,IsleEgg*,float); };
struct ActorObjectHelperBinding { std::uint32_t identity; IsleGameObject* (*function)(IsleGameObject*); };
struct ActorItemBinding { std::uint32_t identity; IsleCaveItemCallback function; };
struct ActorMovementBinding { std::uint32_t identity; IsleCaveMovementCallback function; };
struct ActorGraphHelperBinding { std::uint32_t identity; void (*function)(IsleGameObject*,const IsleActorInteractionGraph*); };
struct ActorTransformBinding { std::uint32_t identity; void (*function)(IsleCaveActorSource*,IsleGameModelRef,float); };
struct ActorUtilityBinding { std::uint32_t identity; void (*function)(IsleCaveActorSource*); };
std::span<const actor::BehaviorCallbackBinding> actor_callbacks() noexcept;
std::span<const actor::BehaviorCallbackBinding> lifecycle_callbacks() noexcept;
std::span<const std::uint32_t> actor_graphs() noexcept;
std::span<const std::uint32_t> actor_weighted_states() noexcept;
std::span<const std::uint32_t> actor_roots() noexcept;
std::span<const ActorFactoryBinding> actor_factories() noexcept;
std::span<const ActorChildFactoryDependency> actor_child_factories() noexcept;
std::span<const ActorProviderDependency> actor_dependencies() noexcept;
std::span<const ActorHeaderBinding> actor_headers() noexcept;
std::span<const ActorWeightedBinding> actor_weighted_bindings() noexcept;
std::span<const ActorCountedOptionsBinding> actor_counted_options() noexcept;
std::span<const ActorAutomaticAggregate> actor_automatic_aggregates() noexcept;
std::span<const ActorAnimationBinding> actor_animation_callbacks() noexcept;
std::span<const LifecycleAnimationBinding> lifecycle_animation_callbacks() noexcept;
std::span<const ActorEggHelperBinding> actor_egg_helpers() noexcept;
std::span<const ActorObjectHelperBinding> actor_object_helpers() noexcept;
std::span<const ActorItemBinding> actor_item_callbacks() noexcept;
std::span<const ActorMovementBinding> actor_movement_callbacks() noexcept;
std::span<const ActorGraphHelperBinding> actor_graph_helpers() noexcept;
std::span<const ActorTransformBinding> actor_transform_helpers() noexcept;
std::span<const ActorUtilityBinding> actor_utilities() noexcept;
}
#endif
