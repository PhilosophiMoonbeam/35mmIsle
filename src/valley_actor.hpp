#ifndef ISLE_VALLEY_ACTOR_HPP
#define ISLE_VALLEY_ACTOR_HPP
#include "valley_actor_source.h"
#include "valley_lifecycle_source.h"
#include "actor_behavior_bindings.hpp"
#include <cstdint>
#include <span>
namespace isle::valley {
enum class SourceAbi : std::uint8_t { gobj_callback, factory_returning5, factory_returning6, factory_void5, animation, lifecycle_animation, item_collision, block_change, utility, file_local_empty };
enum class SourceDomain : std::uint8_t { actor, lifecycle };
enum class ActorFactoryWrapper : std::uint8_t { ground, ordinary, ordinary_dl4, behavior_not3_ground_else_ordinary };
using ActorFactory5 = IsleGameObject* (*)(std::int32_t,std::uint16_t,IsleActorFactoryBlock*,IsleActorFactoryBlock*,IsleActorFactorySpawn*);
using ActorFactory6 = IsleGameObject* (*)(std::int32_t,std::uint16_t,IsleActorFactoryBlock*,IsleActorFactoryBlock*,IsleActorFactorySpawn*,IsleActorFactoryInitialization*);
using ActorVoidFactory5 = void (*)(std::int32_t,std::uint16_t,IsleActorFactoryBlock*,IsleActorFactoryBlock*,IsleActorFactorySpawn*);
using ActorBlockChange = void (*)(IsleGameObject*,float,float,float,float,float,float);
struct ActorFactoryBinding { std::uint32_t actor_id, factory, initialization, initial_state; ActorFactoryWrapper wrapper; ActorFactory5 returning5; ActorFactory6 returning6; ActorVoidFactory5 nonreturning5; };
// ObjectSpawn and copied PokemonDef have distinct local Source declaration origins.
struct ActorChildFactoryDependency { std::uint32_t helper_identity, spawn_declaration_line, definition_declaration_line, singleton_origin, requested_id, family, factory, initialization, change_block, remove, written_fields; };
struct ActorCountedOption { std::uint8_t weight; std::uint32_t callback; };
struct ActorCountedOptions { std::uint32_t identity; std::span<const ActorCountedOption> rows; };
struct ActorAnimationBinding { std::uint32_t identity; IsleValleyAnimationCallback function; };
struct ActorLifecycleAnimationBinding { std::uint32_t identity; IsleValleyLifecycleAnimationCallback function; };
struct ActorItemCollisionBinding { std::uint32_t identity; IsleValleyItemCollisionCallback function; };
struct ActorBlockChangeBinding { std::uint32_t identity; ActorBlockChange function; };
enum class ActorNativeService : std::uint16_t { reference_assign, model, player_model, node_path, vector, vector_read, vector_store, item, item_direction, current_block, graph, graph_copy, weighted, weighted_copy, forbidden_ground, add_one, add_geo, ground, node_world, level_progress, path_position, path_start, animation_callback, model_animation_speed, camera, camera_speed, camera_read, camera_component, camera_object, camera_animation, camera_animation_process, pause_processes, resume_processes, player_model_init, player_model_animation, player_model_change, player_model_speed, cutscene_start, sound, sound_params, sound_active, sound_stop, sound_stop_all, sound_volume, sound_global_volume, sound_volume_smooth, bgm_play, bgm_stop, bgm_volume, bgm_volume_smooth, background_music, check_player_flag, set_player_flag, remove_flying_items, reset_camera, sky_speed, message_reset, message_interactive, message_noninteractive, message_printed, message_show, message_handler, dialogue, sprite, sprite_list, sprite_add, sprite_move, sprite_x2_move, sprite_scale, sprite_color, sprite_link, object_add, object_default_add, input_pressed, player_object, player_name, effect_create, effect_attach, application_notice, transition_begin, transition_color, transition_busy, audio_cutscene_reset, environment_sound, kick_item, jump_bounce, common_child1, common_child2, block_change, block_change_ground, common_remove, misc_object_get, misc_object_set, model_time_left, rng_range, rng_float, player_cutscene_process, flute, command, command_link, vector_distance, dialogue_format,  count };
struct ActorDependencyEdge { std::uint32_t identity; const char* symbol; const char* source_abi; };
struct ActorResourceDependency { std::uint32_t identity; const char* symbol; const char* source_type; };
/* Exact Source ABI metadata is deliberately not a universal callable pointer.
 * Only true void(GObj*) definitions enter BehaviorCallbackBinding. Factories,
 * animation, collision, block-change and utilities keep their actual distinct
 * typed bindings. File-local empty bodies retain selector provenance, not a
 * guessed global nullsub identity. Native host must additionally prove actual
 * Source/asset/service and receiver/model-incarnation admission and joins. */
struct ActorProviderDependency { std::uint32_t identity; const char* selector; const char* source_abi; const char* native_abi; SourceAbi abi; SourceDomain domain; std::span<const ActorDependencyEdge> calls; std::span<const ActorResourceDependency> resources; std::span<const ActorNativeService> services; };
struct ActorUtilityBindings { 
    decltype(&::func_802C5C20_79F1B0) func_802C5C20_79F1B0;
    decltype(&::func_802C5C44_79F1D4) func_802C5C44_79F1D4;
    decltype(&::func_802C5C68_79F1F8) func_802C5C68_79F1F8;
    decltype(&::func_802C5C8C_79F21C) func_802C5C8C_79F21C;
    decltype(&::func_802C5D18_79F2A8) func_802C5D18_79F2A8;
    decltype(&::func_802C5DCC_79F35C) func_802C5DCC_79F35C;
    decltype(&::func_802C5DD4_79F364) func_802C5DD4_79F364;
    decltype(&::func_802C6300_79F890) func_802C6300_79F890;
    decltype(&::func_802C63EC_79F97C) func_802C63EC_79F97C;
    decltype(&::func_802C653C_79FACC) func_802C653C_79FACC;
    decltype(&::func_802C6544_79FAD4) func_802C6544_79FAD4;
    decltype(&::func_802C6610_79FBA0) func_802C6610_79FBA0;
    decltype(&::valley_stopIntro) valley_stopIntro;
    decltype(&::func_802C6AD4_7A0064) func_802C6AD4_7A0064;
    decltype(&::func_802C6B30_7A00C0) func_802C6B30_7A00C0;
    decltype(&::func_802C89F4_7A1F84) func_802C89F4_7A1F84;
    decltype(&::func_802C8A4C_7A1FDC) func_802C8A4C_7A1FDC;
    decltype(&::isle_valley_7A3560_nullsub) isle_valley_7A3560_nullsub;
    decltype(&::func_802CC980_7A5F10) func_802CC980_7A5F10;
    decltype(&::func_802CEC18_7A81A8) func_802CEC18_7A81A8;
    decltype(&::func_802CFE8C_7A941C) func_802CFE8C_7A941C;
    decltype(&::func_802D017C_7A970C) func_802D017C_7A970C;
    decltype(&::func_802D039C_7A992C) func_802D039C_7A992C;
    decltype(&::func_802D03D0_7A9960) func_802D03D0_7A9960;
    decltype(&::func_802D0700_7A9C90) func_802D0700_7A9C90;
    decltype(&::func_802D0754_7A9CE4) func_802D0754_7A9CE4;
    decltype(&::isle_valley_7AA790_nullsub) isle_valley_7AA790_nullsub;
 };
std::span<const actor::BehaviorCallbackBinding> actor_callbacks() noexcept;
std::span<const std::uint32_t> actor_graphs() noexcept;
std::span<const std::uint32_t> actor_weighted_states() noexcept;
std::span<const std::uint32_t> actor_roots() noexcept;
std::span<const ActorFactoryBinding> actor_factories() noexcept;
std::span<const ActorChildFactoryDependency> actor_child_factories() noexcept;
std::span<const ActorProviderDependency> actor_dependencies() noexcept;
std::span<const ActorCountedOptions> actor_counted_options() noexcept;
std::span<const ActorAnimationBinding> actor_animation_callbacks() noexcept;
std::span<const ActorLifecycleAnimationBinding> lifecycle_animation_callbacks() noexcept;
std::span<const ActorItemCollisionBinding> actor_item_collision_callbacks() noexcept;
std::span<const ActorBlockChangeBinding> actor_block_change_callbacks() noexcept;
std::span<const ActorNativeService> actor_required_services() noexcept;
const ActorUtilityBindings& actor_utilities() noexcept;
} // namespace isle::valley
#endif
