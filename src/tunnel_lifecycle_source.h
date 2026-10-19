#ifndef ISLE_TUNNEL_LIFECYCLE_SOURCE_H
#define ISLE_TUNNEL_LIFECYCLE_SOURCE_H
#include "tunnel_actor_source.h"
#include "player_root_source.h"
#include "player_cart_source.h"
#include "35mmisle/continuation.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct IsleTunnelLifecycleSource IsleTunnelLifecycleSource;
typedef struct IsleTunnelLifecycleBlock IsleTunnelLifecycleBlock;
typedef struct IsleTunnelLifecycleAsset IsleTunnelLifecycleAsset;
typedef struct IsleTunnelLifecycleEffect IsleTunnelLifecycleEffect;
typedef struct IsleTunnelLifecycleEnvironment {
    uint16_t sound;
    uint8_t pitch_modifier, hearing_range;
} IsleTunnelLifecycleEnvironment;
typedef struct IsleTunnelLifecyclePokemonDef {
    uint32_t id, initialize_identity, update_identity, remove_identity;
} IsleTunnelLifecyclePokemonDef;
/* Semantic wire decode, not guest SceneSetup/ScreenSettings layouts. Guest
 * callbacks remain numeric identities; none is converted to a host pointer. */
typedef struct IsleTunnelLifecycleSceneSetup {
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
} IsleTunnelLifecycleSceneSetup;
typedef struct IsleTunnelLifecycleScreen {
    uint32_t framebuffer_identity[3], initial_depth_identity, width, height, flags;
    void* depth_buffer;
} IsleTunnelLifecycleScreen;
typedef enum IsleTunnelLifecycleAssetId {
    ISLE_TUNNEL_LIFECYCLE_FX_SCRIPT_COMMON,
    ISLE_TUNNEL_LIFECYCLE_FX_SPRITES_COMMON,
    ISLE_TUNNEL_LIFECYCLE_FX_SCRIPT_TUNNEL,
    ISLE_TUNNEL_LIFECYCLE_FX_SPRITES_TUNNEL,
    ISLE_TUNNEL_LIFECYCLE_IDLE_SCRIPT
} IsleTunnelLifecycleAssetId;
typedef IsleGameResult (*IsleTunnelLifecycleWorldPair)(void*, IsleTunnelLifecycleBlock*, IsleTunnelLifecycleBlock*);
typedef IsleGameResult (*IsleTunnelLifecycleWorldSingle)(void*, IsleTunnelLifecycleBlock*);
typedef struct IsleTunnelLifecycleUiCallbacks {
    void* user;
    IsleTunnelLifecycleWorldSingle exit_block;
    /* Source binds real tunnel_UpdateSounds and tunnel_HandleCollision. */
    IsleGameResult (*movement)(void*, int32_t, float);
    IsleGameSourceCallback update_items;
    uint8_t update_items_kind;
    IsleGameResult (*collide_item)(void*, IsleGameObject*, const IsleActorGround*);
} IsleTunnelLifecycleUiCallbacks;


struct IsleTunnelLifecycleSource {
    void* owner;
    IsleContinuationDomain* domain;
    IsleTunnelActorSource* actors;
    IslePlayerSource* player; /* ACTIVE-only borrow; never read after detach */
    IsleGameResult status;
    void* nonwaiting_call_frame;
    int32_t input_scene, output_scene;
    uint8_t running, completed, scene_requested, scene_completion_ready;
    IsleGameResult scene_completion_status;
    IsleTunnelLifecycleSceneSetup scene_setup;
    IsleTunnelLifecycleScreen screen;
    IsleTunnelLifecycleEnvironment environment[43];
    IsleTunnelLifecyclePokemonDef pokemon_defs[22]; /* twenty-one + zero terminal */
    int32_t tunnel_EndLevelReason;
    int32_t D_802F01E0_5ED2B0;
    /* Real canonical registered semantic reference, not an Actor/Cart mirror. */
    IsleGameObject* D_802F01E4_5ED2B4;
    uint8_t D_803430E0_6401B0[8], D_80344730_6401B0[8];
    uint8_t D_80344738_641808, D_80344739_641809;
    uint8_t D_8034473A_64180A, D_8034473B_64180B;
};
/* Once only, before Game, on zero storage with positively PREPARED canonical
 * actors and the same independent owner. Decode actual Scene/Screen/intro words
 * from the SAME identity-admitted canonical ROM, checking pinned Source/map
 * invariants. Preserve actual bool bits; reject a nonzero guest initial object
 * word as an unbound pre-Game reference. Never cast it or invent a GObj.
 * Qualified639a raw admission checks all35 Scene,7 Screen and2 intro words,
 * including both high-halfword padding words; no activation reset occurs. */
IsleGameResult isle_tunnel_lifecycle_prepare(IsleTunnelLifecycleSource*, void*,
    IsleContinuationDomain*, IsleTunnelActorSource*, const uint8_t*, size_t, int32_t);
void isle_tunnel_lifecycle_run(void*);
/* Host invokes outside traversals only after real retirement/joins. The native
 * receipt is independent of deleted Game/Player/Storage; STOPPED is not success.
 * Failures latch first and prohibit original output-scene publication. */
IsleGameResult isle_tunnel_lifecycle_complete_scene(IsleTunnelLifecycleSource*, IsleGameResult);
IsleGameResult isle_tunnel_lifecycle_invoke_init(IsleTunnelLifecycleSource*);
IsleGameResult isle_tunnel_lifecycle_invoke_end(IsleTunnelLifecycleSource*, int32_t);
IsleGameResult isle_tunnel_lifecycle_end_event(void*, int32_t);
IsleGameResult isle_tunnel_lifecycle_pause_event(void*, int32_t);
IsleGameResult isle_tunnel_lifecycle_world_add(void*, IsleTunnelLifecycleBlock*, IsleTunnelLifecycleBlock*);
IsleGameResult isle_tunnel_lifecycle_world_change(void*, IsleTunnelLifecycleBlock*, IsleTunnelLifecycleBlock*);
IsleGameResult isle_tunnel_lifecycle_world_remove(void*, IsleTunnelLifecycleBlock*);
IsleGameResult isle_tunnel_lifecycle_exit_block(void*, IsleTunnelLifecycleBlock*);
IsleGameResult isle_tunnel_lifecycle_movement(void*, int32_t, float);
IsleGameResult isle_tunnel_lifecycle_collide_item(void*, IsleGameObject*, const IsleActorGround*);
void isle_tunnel_lifecycle_cart_event(void*, const IslePlayerCartNode*, int32_t, float);
void isle_tunnel_lifecycle_camera_animation_update(IsleGameObject*);
void isle_tunnel_lifecycle_input_update(IsleGameObject*);
void isle_tunnel_lifecycle_delete_intro_cart(IsleTunnelLifecycleSource*);

/* Complete Source intro7/main12. Empty helper/pause/dynamic-store callbacks
 * retain their exact original ABI; none is an unsupported native fallback. */
void isle_tunnel_lifecycle_func_802ED000_5EA0D0(IsleTunnelLifecycleSource* s, IsleGameObject* obj);
void isle_tunnel_lifecycle_func_802ED030_5EA100(IsleTunnelLifecycleSource* s);
void isle_tunnel_lifecycle_func_802ED050_5EA120(void* user, const IslePlayerCartNode* dobj, int32_t param, float value);
void isle_tunnel_lifecycle_func_802ED0C4_5EA194(IsleTunnelLifecycleSource* s);
void isle_tunnel_lifecycle_func_802ED1BC_5EA28C(IsleGameObject* obj);
void isle_tunnel_lifecycle_func_802ED2E8_5EA3B8(IsleGameObject* obj);
void isle_tunnel_lifecycle_func_802ED5C8_5EA698(IsleTunnelLifecycleSource* s);
void isle_tunnel_lifecycle_tunnel_PokemonAdd(IsleTunnelLifecycleSource* s, IsleTunnelLifecycleBlock* arg0, IsleTunnelLifecycleBlock* arg1);
void isle_tunnel_lifecycle_tunnel_PokemonChangeBlock(IsleTunnelLifecycleSource* s, IsleTunnelLifecycleBlock* arg0, IsleTunnelLifecycleBlock* arg1);
void isle_tunnel_lifecycle_tunnel_PokemonRemove(IsleTunnelLifecycleSource* s, IsleTunnelLifecycleBlock* arg0);
void isle_tunnel_lifecycle_tunnel_InitWorld(IsleTunnelLifecycleSource* s);
void isle_tunnel_lifecycle_tunnel_EndLevel(IsleTunnelLifecycleSource* s, int32_t endLevelReason);
void isle_tunnel_lifecycle_func_802E28A4_5DF974(IsleGameObject* obj);
void isle_tunnel_lifecycle_tunnel_Pause(IsleTunnelLifecycleSource* s, int32_t arg0);
void isle_tunnel_lifecycle_tunnel_ExitBlock(IsleTunnelLifecycleSource* s, IsleTunnelLifecycleBlock* arg0);
void isle_tunnel_lifecycle_tunnel_LoadEffects(IsleTunnelLifecycleSource* s);
void isle_tunnel_lifecycle_tunnel_Init(IsleTunnelLifecycleSource* s);
void isle_tunnel_lifecycle_func_802E2BB0_5DFC80(int32_t arg0);
int32_t isle_tunnel_lifecycle_tunnel_Start(IsleTunnelLifecycleSource* s, int32_t arg0);

/* Required returning typed native leaves. They never wait, longjmp, throw,
 * invoke Source under C++ ownership frames, or physically destroy live owners.
 * request_scene runs PREPARED before Game; scene_joined/photo_count/or_notice
 * run after DETACHED using only the retained independent application owner.
 * FX/Cart reference-release leaves require actual process/node-credit joins. */
/* Context clears out on every failure. NULL receiver is authorized ONLY for
 * active non-object init/end/world/UI event preflight, with the actual Game.
 * Non-NULL receiver must be the actual retained helper/Player/Cart/camera/item
 * incarnation; no actor-payload requirement, base downcast, TLS or guessed Game. */
IsleGameResult isle_tunnel_lifecycle_native_context(IsleGame*, IsleGameObject*, IsleTunnelLifecycleSource**);
IsleGameResult isle_tunnel_lifecycle_native_request_scene(IsleTunnelLifecycleSource*, const IsleTunnelLifecycleSceneSetup*);
IsleGameResult isle_tunnel_lifecycle_native_invalidate_input(IsleTunnelLifecycleSource*);
/* Read ONLY the independent owner's real retained join receipt. This hook runs
 * with actors DETACHED and may run after sceneStorage/Game destruction; never
 * derive closure from STOPPED, exit intent, a flag or Game process count.
 * External GPU/photo/readback/audio/model-credit joins remain mandatory. */
IsleGameResult isle_tunnel_lifecycle_native_scene_joined(IsleTunnelLifecycleSource*);
IsleGameResult isle_tunnel_lifecycle_native_intervals(IsleTunnelLifecycleSource*, uint16_t, uint16_t);
IsleGameResult isle_tunnel_lifecycle_native_near_clipping(IsleTunnelLifecycleSource*, int32_t);
/* Application's sole D_800AEBC0 plus three transition color bytes (255 iff2).
 * C native_transition_end performs owner rendezvous before this native leaf. */
IsleGameResult isle_tunnel_lifecycle_owner_transition_end(IsleTunnelLifecycleSource*, int32_t);
IsleGameResult isle_tunnel_lifecycle_native_transition_end(IsleTunnelLifecycleSource*, int32_t);
IsleGameResult isle_tunnel_lifecycle_native_detector_disable(IsleTunnelLifecycleSource*);
IsleGameResult isle_tunnel_lifecycle_native_environment_cleanup(IsleTunnelLifecycleSource*);
IsleGameResult isle_tunnel_lifecycle_native_stop_bgm(IsleTunnelLifecycleSource*);
IsleGameResult isle_tunnel_lifecycle_native_stop_all_sounds(IsleTunnelLifecycleSource*);
/* Logical removals only during callbacks. Host physically destroys owners only
 * after returning and every true native/source/consumer fence has closed. */
IsleGameResult isle_tunnel_lifecycle_native_destroy_world(IsleTunnelLifecycleSource*);
IsleGameResult isle_tunnel_lifecycle_native_remove_main_camera(IsleTunnelLifecycleSource*);
IsleGameResult isle_tunnel_lifecycle_native_exit_scene(IsleTunnelLifecycleSource*);
IsleGameResult isle_tunnel_lifecycle_native_create_world(IsleTunnelLifecycleSource*, uint32_t setup_identity,
    int32_t sky_id, int32_t block_min, int32_t block_max, int32_t link, int32_t dl_link,
    IsleTunnelLifecycleWorldPair add, IsleTunnelLifecycleWorldSingle remove,
    IsleTunnelLifecycleWorldPair change, void* user);
IsleGameResult isle_tunnel_lifecycle_native_sky_follow(IsleTunnelLifecycleSource*);
IsleGameResult isle_tunnel_lifecycle_native_pokemon_registry(IsleTunnelLifecycleSource*, int32_t, int32_t, int32_t, int32_t);
/* All twenty-one definitions, including real NULL remove and zero terminal. */
IsleGameResult isle_tunnel_lifecycle_native_pokemon_change(IsleTunnelLifecycleSource*, IsleTunnelLifecycleBlock*, IsleTunnelLifecycleBlock*, uint32_t, const IsleTunnelLifecyclePokemonDef*, size_t);
IsleGameResult isle_tunnel_lifecycle_native_pokemon_remove(IsleTunnelLifecycleSource*, IsleTunnelLifecycleBlock*, uint32_t, const IsleTunnelLifecyclePokemonDef*, size_t);
IsleGameResult isle_tunnel_lifecycle_native_block_next(IsleTunnelLifecycleSource*, IsleTunnelLifecycleBlock*, IsleTunnelLifecycleBlock**);
IsleGameResult isle_tunnel_lifecycle_native_block_position(IsleTunnelLifecycleSource*, IsleTunnelLifecycleBlock*, IsleGameVec3*);
IsleGameResult isle_tunnel_lifecycle_native_displace_items(IsleTunnelLifecycleSource*, float, float, float);
IsleGameResult isle_tunnel_lifecycle_native_depth_buffer(IsleTunnelLifecycleSource*, uint32_t, uint32_t, void**);
IsleGameResult isle_tunnel_lifecycle_native_screen(IsleTunnelLifecycleSource*, const IsleTunnelLifecycleScreen*);
IsleGameResult isle_tunnel_lifecycle_native_fx_bank_count(IsleTunnelLifecycleSource*, size_t*);
IsleGameResult isle_tunnel_lifecycle_native_fx_reset_bank(IsleTunnelLifecycleSource*, size_t);
/* Exact application photo-roll reset8009CE00, not a course-local counter. */
IsleGameResult isle_tunnel_lifecycle_native_reset_photo_roll(IsleTunnelLifecycleSource*);
IsleGameResult isle_tunnel_lifecycle_native_create_helper(IsleTunnelLifecycleSource*, int32_t id, IsleGameSourceCallback update, int32_t link, uint32_t priority);
IsleGameResult isle_tunnel_lifecycle_native_pokemons_init(IsleTunnelLifecycleSource*);
IsleGameResult isle_tunnel_lifecycle_native_background(IsleTunnelLifecycleSource*, uint8_t*, uint8_t*, uint8_t*);
IsleGameResult isle_tunnel_lifecycle_native_create_cameras(IsleTunnelLifecycleSource*, uint32_t);
IsleGameResult isle_tunnel_lifecycle_native_ui_bind(IsleTunnelLifecycleSource*, const IsleTunnelLifecycleUiCallbacks*);
IsleGameResult isle_tunnel_lifecycle_native_ui_initialized(IsleTunnelLifecycleSource*, IsleGameObject*);
IsleGameResult isle_tunnel_lifecycle_native_environment_init(IsleTunnelLifecycleSource*, const IsleTunnelLifecycleEnvironment*, size_t);
IsleGameResult isle_tunnel_lifecycle_native_play_sound(IsleTunnelLifecycleSource*, int32_t, int32_t, int32_t, float, int32_t, int32_t*);
IsleGameResult isle_tunnel_lifecycle_native_stop_sound(IsleTunnelLifecycleSource*, uint32_t);
IsleGameResult isle_tunnel_lifecycle_native_sound_identity(IsleTunnelLifecycleSource*, uint32_t, int32_t*);
IsleGameResult isle_tunnel_lifecycle_native_load(IsleTunnelLifecycleSource*, IsleTunnelLifecycleAssetId, uint32_t, uint32_t, const IsleTunnelLifecycleAsset**);
IsleGameResult isle_tunnel_lifecycle_native_fx_setup_bank(IsleTunnelLifecycleSource*, int32_t, const IsleTunnelLifecycleAsset*, const IsleTunnelLifecycleAsset*);
/* Returned objects MUST be stored in Root's actual fields523120/124 using their
 * registered semantic credits and real resources/processes, never token objs. */
IsleGameResult isle_tunnel_lifecycle_native_fx_camera(IsleTunnelLifecycleSource*, int32_t, int32_t, IsleGameCamera*, IsleGameObject**);
IsleGameResult isle_tunnel_lifecycle_native_fx_effects(IsleTunnelLifecycleSource*, int32_t, IsleGameObject**);
IsleGameResult isle_tunnel_lifecycle_native_fx_dither(IsleTunnelLifecycleSource*, uint32_t, uint32_t);
IsleGameResult isle_tunnel_lifecycle_native_detector_create(IsleTunnelLifecycleSource*);
IsleGameResult isle_tunnel_lifecycle_native_detector_enable(IsleTunnelLifecycleSource*);
/* These application-owned post-scene operations remain available after actors
 * DETACHED and sceneStorage destruction. No Actor services/Game/Player access;
 * photo_count is used by entry only after the genuine completion receipt. */
IsleGameResult isle_tunnel_lifecycle_native_photo_count(IsleTunnelLifecycleSource*, int32_t*);
IsleGameResult isle_tunnel_lifecycle_native_or_notice(IsleTunnelLifecycleSource*, uint32_t);
/* EXACT Tunnel camera8013EE60/cart8013D920/material8013E3D0, resolved through
 * native CameraAnimation/CartResources, never Beach clips or copied poses. */
IsleGameResult isle_tunnel_lifecycle_native_camera_clip(IsleTunnelLifecycleSource*, uint32_t, const IslePlayerCameraAnimationClip**);
IsleGameResult isle_tunnel_lifecycle_native_cart_model_clip(IsleTunnelLifecycleSource*, uint32_t, const IslePlayerCartModelAnimation**);
IsleGameResult isle_tunnel_lifecycle_native_cart_material_clip(IsleTunnelLifecycleSource*, uint32_t, const IslePlayerCartMaterialAnimation**);
IsleGameResult isle_tunnel_lifecycle_native_camera_speed(IsleTunnelLifecycleSource*, IsleGameCamera*, float);
IsleGameResult isle_tunnel_lifecycle_native_create_effect(IsleTunnelLifecycleSource*, int32_t, int32_t, IsleTunnelLifecycleEffect**);
/* Retain actual Cart model-incarnation/node credit until FX lifetime ends.
 * The event-batch borrowed node expires at Cart finish_events. */
IsleGameResult isle_tunnel_lifecycle_native_attach_effect(IsleTunnelLifecycleSource*, IsleTunnelLifecycleEffect*, const IslePlayerCartNode*);
IsleGameResult isle_tunnel_lifecycle_native_store_cart(IsleTunnelLifecycleSource*, IsleGameObject*);
IsleGameResult isle_tunnel_lifecycle_native_retire_intro_cart(IsleTunnelLifecycleSource*, IsleGameObject*);
/* Host-only after process join + FX node-credit closure. Registry store-NULL
 * then unregister exact fields; no physical owner destruction here. */
IsleGameResult isle_tunnel_lifecycle_native_release_cart_reference(IsleTunnelLifecycleSource*);
IsleGameResult isle_tunnel_lifecycle_native_release_fx_references(IsleTunnelLifecycleSource*);

IsleGameResult isle_tunnel_lifecycle_native_height_map(IsleTunnelLifecycleSource*, uint32_t);
IsleGameResult isle_tunnel_lifecycle_native_ceiling_map(IsleTunnelLifecycleSource*, uint32_t);
IsleGameResult isle_tunnel_lifecycle_native_pokemon_add(IsleTunnelLifecycleSource*, IsleTunnelLifecycleBlock*, IsleTunnelLifecycleBlock*, uint32_t, const IsleTunnelLifecyclePokemonDef*, size_t);
IsleGameResult isle_tunnel_lifecycle_native_idle_enabled(IsleTunnelLifecycleSource*, int32_t*);
IsleGameResult isle_tunnel_lifecycle_native_idle_script(IsleTunnelLifecycleSource*, const IsleTunnelLifecycleAsset*);
/* Plain-C checked seams consumed by private translation recipes. */
void isle_tunnel_lifecycle_checked(IsleTunnelLifecycleSource*, IsleGameResult);
IsleTunnelLifecycleSource* isle_tunnel_lifecycle_current(IsleGameObject*);
void isle_tunnel_lifecycle_setup_scene(IsleTunnelLifecycleSource*, const IsleTunnelLifecycleSceneSetup*);
IsleTunnelLifecycleBlock* isle_tunnel_lifecycle_block_next(IsleTunnelLifecycleSource*, IsleTunnelLifecycleBlock*);
IsleGameVec3 isle_tunnel_lifecycle_block_position(IsleTunnelLifecycleSource*, IsleTunnelLifecycleBlock*);
void isle_tunnel_lifecycle_displace_items(IsleTunnelLifecycleSource*, float, float, float);
void* isle_tunnel_lifecycle_depth_buffer(IsleTunnelLifecycleSource*, uint32_t, uint32_t);
size_t isle_tunnel_lifecycle_fx_bank_count(IsleTunnelLifecycleSource*);
int32_t isle_tunnel_lifecycle_photo_count(IsleTunnelLifecycleSource*);
int32_t isle_tunnel_lifecycle_play_sound(IsleTunnelLifecycleSource*, int32_t, int32_t, int32_t, float, int32_t);
int32_t isle_tunnel_lifecycle_sound_identity(IsleTunnelLifecycleSource*, uint32_t);
const IsleTunnelLifecycleAsset* isle_tunnel_lifecycle_load(IsleTunnelLifecycleSource*, IsleTunnelLifecycleAssetId);
void isle_tunnel_lifecycle_init_ui(IsleTunnelLifecycleSource*);
IsleGameObject* isle_tunnel_lifecycle_fx_camera(IsleTunnelLifecycleSource*, int32_t, int32_t, IsleGameCamera*);
IsleGameObject* isle_tunnel_lifecycle_fx_effects(IsleTunnelLifecycleSource*, int32_t);
IsleTunnelLifecycleEffect* isle_tunnel_lifecycle_create_effect(IsleTunnelLifecycleSource*, int32_t, int32_t);
const IslePlayerCameraAnimationClip* isle_tunnel_lifecycle_camera_clip(IsleTunnelLifecycleSource*, uint32_t);
const IslePlayerCartModelAnimation* isle_tunnel_lifecycle_cart_model_clip(IsleTunnelLifecycleSource*, uint32_t);
const IslePlayerCartMaterialAnimation* isle_tunnel_lifecycle_cart_material_clip(IsleTunnelLifecycleSource*, uint32_t);
IsleGameCamera* isle_tunnel_lifecycle_camera(IsleTunnelLifecycleSource*);
IsleGameObject* isle_tunnel_lifecycle_camera_object(IsleTunnelLifecycleSource*);
int32_t isle_tunnel_lifecycle_idle_enabled(IsleTunnelLifecycleSource*);
#ifdef __cplusplus
}
#endif
#endif
