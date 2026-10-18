#ifndef ISLE_RAINBOW_LIFECYCLE_SOURCE_H
#define ISLE_RAINBOW_LIFECYCLE_SOURCE_H
#include "rainbow_actor_source.h"
#include "player_root_source.h"
#include "player_cart_source.h"
#include "35mmisle/continuation.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct IsleRainbowLifecycleSource IsleRainbowLifecycleSource;
typedef struct IsleRainbowLifecycleBlock IsleRainbowLifecycleBlock;
typedef struct IsleRainbowLifecycleAsset IsleRainbowLifecycleAsset;
typedef struct IsleRainbowLifecycleEffect IsleRainbowLifecycleEffect;
typedef struct IsleRainbowLifecycleEnvironment {
    uint16_t sound;
    uint8_t pitch_modifier, hearing_range;
} IsleRainbowLifecycleEnvironment;
typedef struct IsleRainbowLifecyclePokemonDef {
    uint32_t id, initialize_identity, update_identity, remove_identity;
} IsleRainbowLifecyclePokemonDef;
/* Semantic wire decode, not guest SceneSetup/ScreenSettings layouts. Guest
 * callbacks remain numeric identities; none is converted to a host pointer. */
typedef struct IsleRainbowLifecycleSceneSetup {
    uint16_t unknown_00;
    uint32_t update_identity, draw_identity, heap_base, heap_size, unknown_14;
    int32_t contexts;
    uint32_t display_list_bytes[4], graphics_heap_bytes;
    uint16_t unknown_30;
    int32_t rdp_output_bytes;
    uint32_t prerender_identity, input_identity, threads, thread_stack_bytes, stacks;
    int32_t unknown_4c;
    uint32_t processes, objects, object_bytes, matrices;
    uint32_t matrix_callback_identity, dynamic_store_identity;
    uint32_t animations, materials, models, model_bytes;
    uint32_t sprites, sprite_bytes, cameras, camera_bytes, initialize_identity;
} IsleRainbowLifecycleSceneSetup;
typedef struct IsleRainbowLifecycleScreen {
    uint32_t framebuffer_identity[3], initial_depth_identity, width, height, flags;
    void* depth_buffer;
} IsleRainbowLifecycleScreen;
typedef enum IsleRainbowLifecycleAssetId {
    ISLE_RAINBOW_LIFECYCLE_FX_SCRIPT_COMMON,
    ISLE_RAINBOW_LIFECYCLE_FX_SPRITES_COMMON,
    ISLE_RAINBOW_LIFECYCLE_FX_SCRIPT_RAINBOW,
    ISLE_RAINBOW_LIFECYCLE_FX_SPRITES_RAINBOW
} IsleRainbowLifecycleAssetId;
typedef IsleGameResult (*IsleRainbowLifecycleWorldPair)(void*, IsleRainbowLifecycleBlock*, IsleRainbowLifecycleBlock*);
typedef IsleGameResult (*IsleRainbowLifecycleWorldSingle)(void*, IsleRainbowLifecycleBlock*);
typedef struct IsleRainbowLifecycleUiCallbacks {
    void* user;
    IsleRainbowLifecycleWorldSingle exit_block;
    /* Original Rainbow initUI passes NULL,NULL,0,NULL for these slots. */
    IsleGameResult (*movement)(void*, int32_t, float);
    IsleGameSourceCallback update_items;
    uint8_t update_items_kind;
    IsleGameResult (*collide_item)(void*, IsleGameObject*, const IsleActorGround*);
} IsleRainbowLifecycleUiCallbacks;

/* Application-owned independent entry storage and actors both live OUTSIDE
 * sceneStorage. actors is the ONE fixed-address canonical actor/BGM record:
 * PREPARED before setup intent, ACTIVE only with actual scene owners, DETACHED
 * after every real join. It is never cloned, rehydrated or rebound. player is
 * an ACTIVE-only scene borrow; complete_scene clears it without dereferencing
 * sceneStorage, which may already be gone. Intro fields are original state,
 * not mirrors of native Cart/camera poses. Source scalars survive detachment. */
struct IsleRainbowLifecycleSource {
    void* owner;
    IsleContinuationDomain* domain;
    IsleRainbowActorSource* actors;
    IslePlayerSource* player;
    IsleGameResult status;
    void* nonwaiting_call_frame;
    int32_t input_scene, output_scene;
    uint8_t running, completed, scene_requested, scene_completion_ready;
    /* Only complete_scene publishes ready, after native_scene_joined returns
     * the independent owner's actual retained closure receipt. Not an exit,
     * STOPPED, Game-process-count or graphics-task-only readiness flag. */
    IsleGameResult scene_completion_status;
    IsleRainbowLifecycleSceneSetup scene_setup;
    IsleRainbowLifecycleScreen screen;
    IsleRainbowLifecycleEnvironment environment[9];
    IsleRainbowLifecyclePokemonDef pokemon_defs[6]; /* five + four-word zero terminal */
    int32_t rainbow_endLevelReason;
    int32_t D_8034B670_82ADE0;
    /* Semantic-reference registry MUST retain this exact field through Cart
     * process join and every real FX model-incarnation/node attachment. */
    IsleGameObject* D_8034B674_82ADE4;
    uint8_t D_803501E0_82F8E0[8];
    uint8_t D_803501E8_82F958, D_803501E9_82F959, D_803501EA_82F95A;
};

/* canonical_rom is the already-admitted US Snapshot, SHA1
 * edc7c49cc568c045fe48be0d18011c30f393cbaf. Requires zero lifecycle storage and
 * actors positively PREPARED by actor_prepare with this SAME independent
 * owner, no Game/services binding and no failure. No zero-record bypass.
 * Allocates/publishes nothing and requires no native sceneStorage or owners.
 * Main .data and intro initialized words are wire-decoded; original intro BSS
 * remains zero. Actor prepare alone admits/hydrates the shared BGM words. */
IsleGameResult isle_rainbow_lifecycle_prepare(IsleRainbowLifecycleSource*, void* owner,
    IsleContinuationDomain*, IsleRainbowActorSource*, const uint8_t*, size_t, int32_t scene);
void isle_rainbow_lifecycle_run(void*);
/* Owner calls only outside every Source/native traversal, after actor
 * deactivation has reached DETACHED. native_scene_joined MUST return a receipt
 * retained by the independent owner for actual native/Source process joins,
 * graphics/readback/photo and scene-local audio fences and every Actor/Cart/FX
 * semantic/model credit. It MUST NOT inspect the old player/Game/sceneStorage.
 * STOPPED/cancelled/exit-request/process-count alone is NOT that proof.
 * Once joined, this clears only scene borrows, preserving original scalars.
 * It latches/returns the first lifecycle/actor/caller failure; STOPPED or host
 * cancellation never permits completed/output_scene or an original success
 * route. Pass ISLE_GAME_STOPPED for intentional host cancellation, not OK;
 * the entry domain may instead be cancelled/joined without resuming Source.
 * Only healthy completion lets omSetupScene return to the original scene
 * route. Application Audio/Photos/notice state and the independent entry owner
 * and continuation domain must remain alive through that original return. */
IsleGameResult isle_rainbow_lifecycle_complete_scene(IsleRainbowLifecycleSource*, IsleGameResult);
/* After genuine setup intent and actual scene creation, before publication:
 * requires actors ACTIVE, the same owner, the SAME Game as canonical Player,
 * canonical Actor/Player source bindings and Player's actual bound Cart source.
 * Activation never resets shared BGM or any Source-mutated actor scalar. */
IsleGameResult isle_rainbow_lifecycle_invoke_init(IsleRainbowLifecycleSource*);
IsleGameResult isle_rainbow_lifecycle_invoke_end(IsleRainbowLifecycleSource*, int32_t);
IsleGameResult isle_rainbow_lifecycle_end_event(void*, int32_t);
IsleGameResult isle_rainbow_lifecycle_pause_event(void*, int32_t);
IsleGameResult isle_rainbow_lifecycle_world_add(void*, IsleRainbowLifecycleBlock*, IsleRainbowLifecycleBlock*);
IsleGameResult isle_rainbow_lifecycle_world_change(void*, IsleRainbowLifecycleBlock*, IsleRainbowLifecycleBlock*);
IsleGameResult isle_rainbow_lifecycle_world_remove(void*, IsleRainbowLifecycleBlock*);
IsleGameResult isle_rainbow_lifecycle_exit_block(void*, IsleRainbowLifecycleBlock*);
void isle_rainbow_lifecycle_cart_event(void*, const IslePlayerCartNode*, int32_t, float);
void isle_rainbow_lifecycle_actor_cart_event(void*, const IslePlayerCartNode*, int32_t, float);
void isle_rainbow_lifecycle_camera_animation_update(IsleGameObject*);
void isle_rainbow_lifecycle_delete_intro_cart(IsleRainbowLifecycleSource*);
/* Actual common Input invalidation process; the application does not also
 * invalidate this course's cache before the OM tick. */
void isle_rainbow_lifecycle_input_update(IsleGameObject*);

/* Complete pinned main13 + intro8 definitions, materialized privately. The
 * original empty 46EE8 callback is intentionally exact, not a native fallback. */
void isle_rainbow_lifecycle_func_80346968_8260D8(IsleGameObject*);
void isle_rainbow_lifecycle_func_80346994_826104(IsleRainbowLifecycleSource*, IsleRainbowLifecycleBlock*, IsleRainbowLifecycleBlock*);
void isle_rainbow_lifecycle_func_803469E0_826150(IsleRainbowLifecycleSource*, IsleRainbowLifecycleBlock*, IsleRainbowLifecycleBlock*);
void isle_rainbow_lifecycle_func_80346A04_826174(IsleRainbowLifecycleSource*, IsleRainbowLifecycleBlock*);
void isle_rainbow_lifecycle_func_80346A28_826198(IsleRainbowLifecycleSource*);
void isle_rainbow_lifecycle_func_80346AA8_826218(IsleRainbowLifecycleSource*, int32_t);
void isle_rainbow_lifecycle_func_80346B0C_82627C(IsleGameObject*);
void isle_rainbow_lifecycle_func_80346B94_826304(IsleRainbowLifecycleSource*, int32_t);
void isle_rainbow_lifecycle_func_80346BD0_826340(IsleRainbowLifecycleSource*, IsleRainbowLifecycleBlock*);
void isle_rainbow_lifecycle_func_80346C64_8263D4(IsleRainbowLifecycleSource*);
void isle_rainbow_lifecycle_func_80346D50_8264C0(IsleRainbowLifecycleSource*);
void isle_rainbow_lifecycle_func_80346EE8_826658(int32_t);
int32_t isle_rainbow_lifecycle_func_80346EF0_826660(IsleRainbowLifecycleSource*, int32_t);
void isle_rainbow_lifecycle_func_8034A520_829C90(IsleRainbowLifecycleSource*, IsleGameObject*);
void isle_rainbow_lifecycle_func_8034A550_829CC0(IsleRainbowLifecycleSource*);
void isle_rainbow_lifecycle_func_8034A578_829CE8(void*, const IslePlayerCartNode*, int32_t, float);
void isle_rainbow_lifecycle_func_8034A5EC_829D5C(IsleRainbowLifecycleSource*);
void isle_rainbow_lifecycle_func_8034A6B4_829E24(IsleGameObject*);
void isle_rainbow_lifecycle_func_8034A748_829EB8(IsleGameObject*);
void isle_rainbow_lifecycle_func_8034A7C4_829F34(IsleGameObject*);
void isle_rainbow_lifecycle_func_8034AABC_82A22C(IsleRainbowLifecycleSource*);

/* All native hooks are REQUIRED returning, typed, actual owner operations:
 * no default/fallback/no-op implementations, Source waits/faults/nonlocal
 * transfer or physical C++ owner destruction inside them. World/UI callbacks
 * above are local nonwaiting C fences. No other original callback is authorized
 * under a C++ traversal. Request_scene publishes setup intent ONLY while
 * actors is PREPARED; host creates native sceneStorage only after the entry
 * continuation yields, activates the SAME actors, then invokes init. Host
 * drains all real borrowers before actor deactivation and completion. It must
 * honor Rainbow setup/cameras/world/clips; this contract claims no readiness. */
IsleGameResult isle_rainbow_lifecycle_native_context(IsleGame*, IsleGameObject*, IsleRainbowLifecycleSource**);
IsleGameResult isle_rainbow_lifecycle_native_request_scene(IsleRainbowLifecycleSource*, const IsleRainbowLifecycleSceneSetup*);
IsleGameResult isle_rainbow_lifecycle_native_invalidate_input(IsleRainbowLifecycleSource*);
/* Read ONLY the independent owner's real retained join receipt. This hook runs
 * with actors DETACHED and may run after sceneStorage/Game destruction; never
 * derive closure from STOPPED, exit intent, a flag or Game process count.
 * External GPU/photo/readback/audio/model-credit joins remain mandatory. */
IsleGameResult isle_rainbow_lifecycle_native_scene_joined(IsleRainbowLifecycleSource*);
IsleGameResult isle_rainbow_lifecycle_native_intervals(IsleRainbowLifecycleSource*, uint16_t, uint16_t);
IsleGameResult isle_rainbow_lifecycle_native_near_clipping(IsleRainbowLifecycleSource*, int32_t);
/* Application's sole D_800AEBC0 plus three transition color bytes (255 iff2).
 * C native_transition_end performs owner rendezvous before this native leaf. */
IsleGameResult isle_rainbow_lifecycle_owner_transition_end(IsleRainbowLifecycleSource*, int32_t);
IsleGameResult isle_rainbow_lifecycle_native_transition_end(IsleRainbowLifecycleSource*, int32_t);
IsleGameResult isle_rainbow_lifecycle_native_detector_disable(IsleRainbowLifecycleSource*);
IsleGameResult isle_rainbow_lifecycle_native_environment_cleanup(IsleRainbowLifecycleSource*);
IsleGameResult isle_rainbow_lifecycle_native_stop_bgm(IsleRainbowLifecycleSource*);
IsleGameResult isle_rainbow_lifecycle_native_stop_all_sounds(IsleRainbowLifecycleSource*);
/* Logical removals only during callbacks. Host physically destroys owners only
 * after returning and every true native/source/consumer fence has closed. */
IsleGameResult isle_rainbow_lifecycle_native_destroy_world(IsleRainbowLifecycleSource*);
IsleGameResult isle_rainbow_lifecycle_native_remove_main_camera(IsleRainbowLifecycleSource*);
IsleGameResult isle_rainbow_lifecycle_native_exit_scene(IsleRainbowLifecycleSource*);
IsleGameResult isle_rainbow_lifecycle_native_create_world(IsleRainbowLifecycleSource*, uint32_t setup_identity,
    int32_t sky_id, int32_t block_min, int32_t block_max, int32_t link, int32_t dl_link,
    IsleRainbowLifecycleWorldPair add, IsleRainbowLifecycleWorldSingle remove,
    IsleRainbowLifecycleWorldPair change, void* user);
IsleGameResult isle_rainbow_lifecycle_native_sky_follow(IsleRainbowLifecycleSource*);
IsleGameResult isle_rainbow_lifecycle_native_pokemon_registry(IsleRainbowLifecycleSource*, int32_t, int32_t, int32_t, int32_t);
/* All five definitions, not a supported subset. Terminal remains admitted too. */
IsleGameResult isle_rainbow_lifecycle_native_pokemon_change(IsleRainbowLifecycleSource*, IsleRainbowLifecycleBlock*, IsleRainbowLifecycleBlock*, uint32_t, const IsleRainbowLifecyclePokemonDef*, size_t);
IsleGameResult isle_rainbow_lifecycle_native_pokemon_remove(IsleRainbowLifecycleSource*, IsleRainbowLifecycleBlock*, uint32_t, const IsleRainbowLifecyclePokemonDef*, size_t);
IsleGameResult isle_rainbow_lifecycle_native_flute_command(IsleRainbowLifecycleSource*, int32_t*);
IsleGameResult isle_rainbow_lifecycle_native_bgm_tempo(IsleRainbowLifecycleSource*, int32_t player, int32_t tempo);
IsleGameResult isle_rainbow_lifecycle_native_sky_speed(IsleRainbowLifecycleSource*, float);
IsleGameResult isle_rainbow_lifecycle_native_block_next(IsleRainbowLifecycleSource*, IsleRainbowLifecycleBlock*, IsleRainbowLifecycleBlock**);
IsleGameResult isle_rainbow_lifecycle_native_block_position(IsleRainbowLifecycleSource*, IsleRainbowLifecycleBlock*, IsleGameVec3*);
IsleGameResult isle_rainbow_lifecycle_native_displace_items(IsleRainbowLifecycleSource*, float, float, float);
IsleGameResult isle_rainbow_lifecycle_native_depth_buffer(IsleRainbowLifecycleSource*, uint32_t, uint32_t, void**);
IsleGameResult isle_rainbow_lifecycle_native_screen(IsleRainbowLifecycleSource*, const IsleRainbowLifecycleScreen*);
IsleGameResult isle_rainbow_lifecycle_native_fx_bank_count(IsleRainbowLifecycleSource*, size_t*);
IsleGameResult isle_rainbow_lifecycle_native_fx_reset_bank(IsleRainbowLifecycleSource*, size_t);
/* Exact application photo-roll reset8009CE00, not a course-local counter. */
IsleGameResult isle_rainbow_lifecycle_native_reset_photo_roll(IsleRainbowLifecycleSource*);
IsleGameResult isle_rainbow_lifecycle_native_create_helper(IsleRainbowLifecycleSource*, int32_t id, IsleGameSourceCallback update, int32_t link, uint32_t priority);
IsleGameResult isle_rainbow_lifecycle_native_pokemons_init(IsleRainbowLifecycleSource*);
IsleGameResult isle_rainbow_lifecycle_native_background(IsleRainbowLifecycleSource*, uint8_t*, uint8_t*, uint8_t*);
IsleGameResult isle_rainbow_lifecycle_native_create_cameras(IsleRainbowLifecycleSource*, uint32_t);
IsleGameResult isle_rainbow_lifecycle_native_ui_bind(IsleRainbowLifecycleSource*, const IsleRainbowLifecycleUiCallbacks*);
IsleGameResult isle_rainbow_lifecycle_native_ui_initialized(IsleRainbowLifecycleSource*, IsleGameObject*);
IsleGameResult isle_rainbow_lifecycle_native_environment_init(IsleRainbowLifecycleSource*, const IsleRainbowLifecycleEnvironment*, size_t);
IsleGameResult isle_rainbow_lifecycle_native_bgm_reverb(IsleRainbowLifecycleSource*, int32_t, int32_t);
IsleGameResult isle_rainbow_lifecycle_native_sound_reverb(IsleRainbowLifecycleSource*, int32_t);
IsleGameResult isle_rainbow_lifecycle_native_play_sound(IsleRainbowLifecycleSource*, int32_t, int32_t, int32_t, float, int32_t, int32_t*);
IsleGameResult isle_rainbow_lifecycle_native_stop_sound(IsleRainbowLifecycleSource*, uint32_t);
IsleGameResult isle_rainbow_lifecycle_native_sound_identity(IsleRainbowLifecycleSource*, uint32_t, int32_t*);
IsleGameResult isle_rainbow_lifecycle_native_load(IsleRainbowLifecycleSource*, IsleRainbowLifecycleAssetId, uint32_t, uint32_t, const IsleRainbowLifecycleAsset**);
IsleGameResult isle_rainbow_lifecycle_native_fx_setup_bank(IsleRainbowLifecycleSource*, int32_t, const IsleRainbowLifecycleAsset*, const IsleRainbowLifecycleAsset*);
/* Returned objects MUST be stored in Root's actual fields523120/124 using their
 * registered semantic credits and real resources/processes, never token objs. */
IsleGameResult isle_rainbow_lifecycle_native_fx_camera(IsleRainbowLifecycleSource*, int32_t, int32_t, IsleGameCamera*, IsleGameObject**);
IsleGameResult isle_rainbow_lifecycle_native_fx_effects(IsleRainbowLifecycleSource*, int32_t, IsleGameObject**);
IsleGameResult isle_rainbow_lifecycle_native_fx_dither(IsleRainbowLifecycleSource*, uint32_t, uint32_t);
IsleGameResult isle_rainbow_lifecycle_native_detector_create(IsleRainbowLifecycleSource*);
IsleGameResult isle_rainbow_lifecycle_native_detector_enable(IsleRainbowLifecycleSource*);
/* These application-owned post-scene operations remain available after actors
 * DETACHED and sceneStorage destruction. No Actor services/Game/Player access;
 * photo_count is used by entry only after the genuine completion receipt. */
IsleGameResult isle_rainbow_lifecycle_native_photo_count(IsleRainbowLifecycleSource*, int32_t*);
IsleGameResult isle_rainbow_lifecycle_native_or_notice(IsleRainbowLifecycleSource*, uint32_t);
/* EXACT Rainbow camera8011B3E0/cart80119AE0/material8011A970, resolved through
 * native CameraAnimation/CartResources, never Beach clips or copied poses. */
IsleGameResult isle_rainbow_lifecycle_native_camera_clip(IsleRainbowLifecycleSource*, uint32_t, const IslePlayerCameraAnimationClip**);
IsleGameResult isle_rainbow_lifecycle_native_cart_model_clip(IsleRainbowLifecycleSource*, uint32_t, const IslePlayerCartModelAnimation**);
IsleGameResult isle_rainbow_lifecycle_native_cart_material_clip(IsleRainbowLifecycleSource*, uint32_t, const IslePlayerCartMaterialAnimation**);
IsleGameResult isle_rainbow_lifecycle_native_camera_speed(IsleRainbowLifecycleSource*, IsleGameCamera*, float);
IsleGameResult isle_rainbow_lifecycle_native_create_effect(IsleRainbowLifecycleSource*, int32_t, int32_t, IsleRainbowLifecycleEffect**);
/* Retain actual Cart model-incarnation/node credit until FX lifetime ends.
 * The event-batch borrowed node expires at Cart finish_events. */
IsleGameResult isle_rainbow_lifecycle_native_attach_effect(IsleRainbowLifecycleSource*, IsleRainbowLifecycleEffect*, const IslePlayerCartNode*);
IsleGameResult isle_rainbow_lifecycle_native_store_cart(IsleRainbowLifecycleSource*, IsleGameObject*);
IsleGameResult isle_rainbow_lifecycle_native_retire_intro_cart(IsleRainbowLifecycleSource*, IsleGameObject*);
/* Host-only after process join + FX node-credit closure. Registry store-NULL
 * then unregister exact fields; no physical owner destruction here. */
IsleGameResult isle_rainbow_lifecycle_native_release_cart_reference(IsleRainbowLifecycleSource*);
IsleGameResult isle_rainbow_lifecycle_native_release_fx_references(IsleRainbowLifecycleSource*);

/* Plain-C checked seams consumed by private translation recipes. */
void isle_rainbow_lifecycle_checked(IsleRainbowLifecycleSource*, IsleGameResult);
IsleRainbowLifecycleSource* isle_rainbow_lifecycle_current(IsleGameObject*);
void isle_rainbow_lifecycle_setup_scene(IsleRainbowLifecycleSource*, const IsleRainbowLifecycleSceneSetup*);
IsleRainbowLifecycleBlock* isle_rainbow_lifecycle_block_next(IsleRainbowLifecycleSource*, IsleRainbowLifecycleBlock*);
IsleGameVec3 isle_rainbow_lifecycle_block_position(IsleRainbowLifecycleSource*, IsleRainbowLifecycleBlock*);
void isle_rainbow_lifecycle_displace_items(IsleRainbowLifecycleSource*, float, float, float);
void* isle_rainbow_lifecycle_depth_buffer(IsleRainbowLifecycleSource*, uint32_t, uint32_t);
size_t isle_rainbow_lifecycle_fx_bank_count(IsleRainbowLifecycleSource*);
int32_t isle_rainbow_lifecycle_flute_command(IsleRainbowLifecycleSource*);
int32_t isle_rainbow_lifecycle_photo_count(IsleRainbowLifecycleSource*);
int32_t isle_rainbow_lifecycle_play_sound(IsleRainbowLifecycleSource*, int32_t, int32_t, int32_t, float, int32_t);
int32_t isle_rainbow_lifecycle_sound_identity(IsleRainbowLifecycleSource*, uint32_t);
const IsleRainbowLifecycleAsset* isle_rainbow_lifecycle_load(IsleRainbowLifecycleSource*, IsleRainbowLifecycleAssetId);
void isle_rainbow_lifecycle_init_ui(IsleRainbowLifecycleSource*);
IsleGameObject* isle_rainbow_lifecycle_fx_camera(IsleRainbowLifecycleSource*, int32_t, int32_t, IsleGameCamera*);
IsleGameObject* isle_rainbow_lifecycle_fx_effects(IsleRainbowLifecycleSource*, int32_t);
IsleRainbowLifecycleEffect* isle_rainbow_lifecycle_create_effect(IsleRainbowLifecycleSource*, int32_t, int32_t);
const IslePlayerCameraAnimationClip* isle_rainbow_lifecycle_camera_clip(IsleRainbowLifecycleSource*, uint32_t);
const IslePlayerCartModelAnimation* isle_rainbow_lifecycle_cart_model_clip(IsleRainbowLifecycleSource*, uint32_t);
const IslePlayerCartMaterialAnimation* isle_rainbow_lifecycle_cart_material_clip(IsleRainbowLifecycleSource*, uint32_t);
IsleGameCamera* isle_rainbow_lifecycle_camera(IsleRainbowLifecycleSource*);
IsleGameObject* isle_rainbow_lifecycle_camera_object(IsleRainbowLifecycleSource*);
#ifdef __cplusplus
}
#endif
#endif
