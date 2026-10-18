#ifndef ISLE_RAINBOW_ACTOR_HPP
#define ISLE_RAINBOW_ACTOR_HPP
#include "rainbow_actor_source.h"
#include "actor_behavior_bindings.hpp"
#include <cstdint>
#include <span>
namespace isle::rainbow {
enum class ActorFactoryWrapper { ground, ordinary_dl4 };
using ActorFactoryFunction = IsleGameObject* (*)(std::int32_t, std::uint16_t,
    IsleActorFactoryBlock*, IsleActorFactoryBlock*, IsleActorFactorySpawn*);
struct ActorFactoryBinding {
    std::uint32_t actor_id, factory, initialization, initial_state;
    ActorFactoryWrapper wrapper;
    ActorFactoryFunction function;
};
struct ActorChildFactoryDependency {
    std::uint32_t helper_identity;
    IsleRainbowDefinition definition;
    std::uint32_t written_fields;
};
enum class ActorNativeService : std::uint8_t {
    reference_assign,
    current_block,
    add_one,
    sound_play,
    sound_active,
    sound_stop,
    sound_stop_all,
    bgm_play,
    bgm_stop,
    bgm_volume,
    bgm_channel_volume,
    remove_flying_items,
    reset_camera,
    reset_messages,
    remove_pokemon,
    cutscene_start,
    resume_processes,
    main_camera,
    pause_processes,
    camera_speed,
    camera_animation,
    camera_animation_process,
    sky_speed,
    player_model_init,
    player_model_animation,
    animation_callback,
    collision_initialize,
    collision_query,
    collision_callback,
    count
};
/* These immutable dependencies describe executable Source authority, not an
 * admission shortcut. Native Owner must additionally bind resources, services,
 * exact definition origin, wrappers, model incarnations and completion fences.
 * callbacks includes native utility/animation/collision identities when used;
 * only actual GObj callbacks appear in actor_callbacks(). */
struct ActorProviderDependency {
    std::uint32_t identity;
    std::span<const std::uint32_t> callbacks, graphs, weighted_states, headers, child_factories;
    std::uint64_t services;
};
struct ActorAnimationBinding { std::uint32_t identity; IsleRainbowAnimationCallback function; };
struct ActorCollisionBinding { std::uint32_t identity; IsleRainbowCollisionCallback function; };
std::span<const actor::BehaviorCallbackBinding> actor_callbacks() noexcept;
std::span<const std::uint32_t> actor_graphs() noexcept;
std::span<const std::uint32_t> actor_weighted_states() noexcept;
std::span<const std::uint32_t> actor_roots() noexcept;
std::span<const ActorFactoryBinding> actor_factories() noexcept;
std::span<const ActorChildFactoryDependency> actor_child_factories() noexcept;
std::span<const ActorProviderDependency> actor_dependencies() noexcept;
std::span<const ActorAnimationBinding> actor_animation_callbacks() noexcept;
std::span<const ActorCollisionBinding> actor_collision_callbacks() noexcept;
std::uint64_t actor_required_services() noexcept;
} // namespace isle::rainbow
#endif
