#ifndef ISLE_RIVER_ACTOR_HPP
#define ISLE_RIVER_ACTOR_HPP
#include "river_actor_source.h"
#include "river_lifecycle_source.h"
#include "actor_behavior_bindings.hpp"
#include <cstdint>
#include <span>
namespace isle::river {
enum class ActorFactoryWrapper : std::uint8_t { ground, ordinary, ordinary_dl4, ground_dl4 };
using ActorFactoryFive = IsleGameObject* (*)(std::int32_t, std::uint16_t, IsleActorFactoryBlock*, IsleActorFactoryBlock*, IsleActorFactorySpawn*);
using ActorFactorySix = IsleGameObject* (*)(std::int32_t, std::uint16_t, IsleActorFactoryBlock*, IsleActorFactoryBlock*, IsleActorFactorySpawn*, IsleActorFactoryInitialization*);
struct ActorFactoryBinding {
    std::uint32_t actor_id, factory, initialization, initial_state;
    ActorFactoryWrapper wrapper;
    ActorFactoryFive native_adapter, source_five;
    ActorFactorySix source_six;
};
enum class ActorSourceAbi : std::uint8_t { gobj_callback, factory_five, factory_six, animation, lifecycle_animation, item_collision, returning_gobj_utility, actor_utility, lifecycle_utility, file_local_utility };
enum class ActorNativeService : std::uint8_t {
    reference_assign,
    model,
    model_read,
    model_store,
    root_position,
    root_store,
    node_descend,
    node_world,
    current_block,
    block_position,
    path_count,
    path_duration,
    path_parameter,
    path_point,
    item,
    player_object,
    player_model,
    rng_range,
    weighted,
    counted_options,
    graph,
    add_one,
    ground,
    sound,
    sound_params,
    sound_active,
    sound_stop,
    bgm_play,
    bgm_volume,
    bgm_volume_smooth,
    sound_volume_smooth,
    sound_global_volume,
    animation_speed,
    model_speed,
    camera,
    camera_read,
    camera_store,
    camera_speed,
    camera_animation,
    camera_process,
    player_model_init,
    player_animation,
    player_change_animation,
    player_animation_speed,
    animation_callback,
    pause_processes,
    resume_processes,
    sky_speed,
    notice,
    progress_read,
    progress_write,
    transition_prepare,
    transition_color,
    transition_fade,
    transition_active,
    create_helper,
    delete_object,
    send_command,
    send_command_link,
    distance,
    sound_stop_all,
    bgm_stop_all,
    remove_flying_items,
    reset_messages,
    audio_cutscene,
    source_process,
    environment_sound,
    definition,
    root_read,
    count
};
enum class LifecycleNativeService : std::uint8_t {
    sound_params,
    sound_active,
    sound_stop,
    sound_stop_all,
    bgm_stop_all,
    bgm_play,
    bgm_stop,
    bgm_volume,
    bgm_channel_volume,
    camera,
    camera_read,
    camera_store,
    camera_speed,
    camera_animation,
    camera_process,
    player_model_init,
    player_animation,
    animation_callback,
    create_helper,
    delete_object,
    notice,
    effect_create,
    effect_attach,
    input_pressed,
    world_add,
    world_change,
    world_remove,
    height_map,
    create_world,
    sky_follow,
    pokemon_registry,
    transition_end,
    destroy_world,
    environment_cleanup,
    detector_disable,
    remove_camera,
    exit_scene,
    course_cleanup,
    flute_command,
    block_next,
    block_position,
    displace_items,
    load_asset,
    fx_bank,
    fx_camera,
    fx_allocate,
    fx_dither,
    depth_allocate,
    screen,
    fx_bank_count,
    fx_bank_reset,
    photo_reset,
    pokemons_init,
    background,
    create_cameras,
    ui_bind,
    end_callback,
    pause_callback,
    environment_init,
    detector_create,
    detector_enable,
    intervals,
    near_clipping,
    photo_count,
    request_scene,
    scene_joined,
    source_process,
    reference_assign,
    count
};
struct ActorProviderDependency {
    std::uint32_t identity;
    const char* symbol;
    const char* source_file;
    std::uint32_t source_first, source_last;
    const char* source_abi;
    ActorSourceAbi abi;
    std::span<const std::uint32_t> functions, graphs, weighted_states, counted_options, headers, child_factories;
    std::span<const ActorNativeService> services;
    std::span<const LifecycleNativeService> lifecycle_services;
};
struct ActorChildFactoryDependency {
    IsleRiverAutomaticOrigin origin;
    std::uint32_t copied_singleton, written_fields;
    IsleRiverDefinition definition;
};
struct ActorCountedOptionsBinding { std::uint32_t identity, count; };
struct ActorAnimationBinding { std::uint32_t identity; IsleRiverAnimationCallback function; };
struct ActorLifecycleAnimationBinding { std::uint32_t identity; IsleRiverLifecycleAnimationCallback function; };
struct ActorItemCollisionBinding { std::uint32_t identity; IsleRiverItemCollision function; };
struct ActorReturningUtilityBinding { std::uint32_t identity; std::int32_t (*function)(IsleGameObject*); };
struct ActorSequentialMaterialFamily {
    std::uint32_t initialization, initial_header, first_header, stop_header;
    std::uint32_t first_state, sequential_state;
    float first_speed, stop_speed;
};
std::span<const actor::BehaviorCallbackBinding> actor_callbacks() noexcept;
std::span<const actor::BehaviorCallbackBinding> lifecycle_callbacks() noexcept;
std::span<const std::uint32_t> actor_graphs() noexcept;
std::span<const std::uint32_t> actor_weighted_states() noexcept;
std::span<const ActorCountedOptionsBinding> actor_counted_options() noexcept;
std::span<const std::uint32_t> actor_roots() noexcept;
std::span<const ActorFactoryBinding> actor_factories() noexcept;
std::span<const ActorChildFactoryDependency> actor_child_factories() noexcept;
std::span<const ActorProviderDependency> actor_dependencies() noexcept;
std::span<const ActorAnimationBinding> actor_animation_callbacks() noexcept;
std::span<const ActorLifecycleAnimationBinding> lifecycle_animation_callbacks() noexcept;
std::span<const ActorItemCollisionBinding> actor_item_collisions() noexcept;
std::span<const ActorReturningUtilityBinding> actor_returning_utilities() noexcept;
std::span<const ActorSequentialMaterialFamily> actor_sequential_material_families() noexcept;
std::span<const ActorNativeService> actor_required_services() noexcept;
} // namespace isle::river
#endif
