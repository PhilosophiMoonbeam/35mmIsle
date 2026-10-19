#ifndef ISLE_RIVER_LIFECYCLE_SOURCE_H
#define ISLE_RIVER_LIFECYCLE_SOURCE_H
#include "river_actor_source.h"
#include "35mmisle/continuation.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct IsleRiverLifecycleSource IsleRiverLifecycleSource;
typedef struct IsleRiverLifecycleAsset IsleRiverLifecycleAsset;
typedef enum IsleRiverLifecycleAssetId {
    ISLE_RIVER_ASSET_COMMON_SCRIPT, ISLE_RIVER_ASSET_COMMON_SPRITES,
    ISLE_RIVER_ASSET_RIVER_SCRIPT, ISLE_RIVER_ASSET_RIVER_SPRITES
} IsleRiverLifecycleAssetId;
typedef struct IsleRiverLifecycleEnvironment { uint16_t sound; uint8_t pitch_modifier, hearing_range; } IsleRiverLifecycleEnvironment;
typedef struct IsleRiverLifecycleSceneSetup {
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
} IsleRiverLifecycleSceneSetup;
typedef struct IsleRiverLifecycleScreen {
    uint32_t framebuffer_identity[3], initial_depth_identity, width, height, flags;
    void* depth_buffer;
} IsleRiverLifecycleScreen;
typedef void (*IsleRiverLifecycleWorldPair)(IsleRiverLifecycleSource*, IsleRiverBlock*, IsleRiverBlock*);
typedef void (*IsleRiverLifecycleWorldSingle)(IsleRiverLifecycleSource*, IsleRiverBlock*);
typedef void (*IsleRiverLifecycleMovement)(IsleRiverLifecycleSource*, int32_t, float);
typedef void (*IsleRiverLifecycleEnd)(IsleRiverLifecycleSource*, int32_t);
typedef void (*IsleRiverLifecycleAnimationCallback)(IsleRiverLifecycleSource*, const IsleActorNodeRef*, int32_t, float);
typedef void (*IsleRiverLifecycleCall)(IsleRiverLifecycleSource*, void*);
struct IslePlayerSource* isle_river_lifecycle_player(IsleRiverLifecycleSource*);
/* Typed native backend; every leaf returns before C checks its result.
 * No service may wait, throw, fault or perform a nonlocal transfer through C++.
 * World/UI/animation producers enter only through the returning C invokers. */
typedef struct IsleRiverLifecycleServices {
    IsleGameResult (*sound_params)(IsleRiverLifecycleSource*, int32_t, int32_t, int32_t, float, int32_t, int32_t*);
    IsleGameResult (*sound_active)(IsleRiverLifecycleSource*, int32_t, int32_t*);
    IsleGameResult (*sound_stop)(IsleRiverLifecycleSource*, int32_t);
    IsleGameResult (*sound_stop_all)(IsleRiverLifecycleSource*);
    IsleGameResult (*bgm_stop_all)(IsleRiverLifecycleSource*);
    IsleGameResult (*bgm_play)(IsleRiverLifecycleSource*, int32_t, int32_t);
    IsleGameResult (*bgm_stop)(IsleRiverLifecycleSource*, int32_t);
    IsleGameResult (*bgm_volume)(IsleRiverLifecycleSource*, int32_t, uint32_t);
    IsleGameResult (*bgm_channel_volume)(IsleRiverLifecycleSource*, int32_t, int32_t, int32_t);
    IsleGameResult (*camera)(IsleRiverLifecycleSource*, IsleGameObject**);
    IsleGameResult (*camera_read)(IsleRiverLifecycleSource*, IsleGameObject*, const IsleRiverCameraView**);
    IsleGameResult (*camera_store)(IsleRiverLifecycleSource*, IsleGameObject*, uint32_t, float);
    IsleGameResult (*camera_speed)(IsleRiverLifecycleSource*, IsleGameObject*, float);
    IsleGameResult (*camera_animation)(IsleRiverLifecycleSource*, IsleGameObject*, uint32_t, float);
    IsleGameResult (*camera_process)(IsleRiverLifecycleSource*, IsleGameObject*, int32_t, int32_t, IsleGameProcess**);
    IsleGameResult (*player_model_init)(IsleRiverLifecycleSource*, IsleGameObject**);
    IsleGameResult (*player_animation)(IsleRiverLifecycleSource*, uint32_t, uint32_t, float, float);
    IsleGameResult (*animation_callback)(IsleRiverLifecycleSource*, IsleGameObject*, IsleRiverLifecycleAnimationCallback);
    IsleGameResult (*create_helper)(IsleRiverLifecycleSource*, int32_t, IsleGameSourceCallback, int32_t, uint32_t, IsleGameObject**);
    IsleGameResult (*delete_object)(IsleRiverLifecycleSource*, IsleGameObject*);
    IsleGameResult (*notice)(IsleRiverLifecycleSource*, uint32_t);
    IsleGameResult (*effect_create)(IsleRiverLifecycleSource*, int32_t, int32_t, IsleRiverEffect**);
    IsleGameResult (*effect_attach)(IsleRiverLifecycleSource*, IsleRiverEffect*, const IsleActorNodeRef*);
    IsleGameResult (*input_pressed)(IsleRiverLifecycleSource*, uint16_t*);
    IsleGameResult (*world_add)(IsleRiverLifecycleSource*, IsleRiverBlock*, IsleRiverBlock*, uint32_t);
    IsleGameResult (*world_change)(IsleRiverLifecycleSource*, IsleRiverBlock*, IsleRiverBlock*, uint32_t);
    IsleGameResult (*world_remove)(IsleRiverLifecycleSource*, IsleRiverBlock*, uint32_t);
    IsleGameResult (*height_map)(IsleRiverLifecycleSource*, uint32_t);
    IsleGameResult (*create_world)(IsleRiverLifecycleSource*, uint32_t, int32_t, int32_t, int32_t, int32_t, int32_t, IsleRiverLifecycleWorldPair, IsleRiverLifecycleWorldSingle, IsleRiverLifecycleWorldPair);
    IsleGameResult (*sky_follow)(IsleRiverLifecycleSource*);
    IsleGameResult (*pokemon_registry)(IsleRiverLifecycleSource*, int32_t, int32_t, int32_t, int32_t);
    IsleGameResult (*transition_end)(IsleRiverLifecycleSource*, int32_t);
    IsleGameResult (*destroy_world)(IsleRiverLifecycleSource*);
    IsleGameResult (*environment_cleanup)(IsleRiverLifecycleSource*);
    IsleGameResult (*detector_disable)(IsleRiverLifecycleSource*);
    IsleGameResult (*remove_camera)(IsleRiverLifecycleSource*);
    IsleGameResult (*exit_scene)(IsleRiverLifecycleSource*);
    IsleGameResult (*course_cleanup)(IsleRiverLifecycleSource*);
    IsleGameResult (*flute_command)(IsleRiverLifecycleSource*, int32_t*);
    IsleGameResult (*block_next)(IsleRiverLifecycleSource*, IsleRiverBlock*, IsleRiverBlock**);
    IsleGameResult (*block_position)(IsleRiverLifecycleSource*, IsleRiverBlock*, IsleGameVec3*);
    IsleGameResult (*displace_items)(IsleRiverLifecycleSource*, float, float, float);
    IsleGameResult (*load_asset)(IsleRiverLifecycleSource*, IsleRiverLifecycleAssetId, IsleRiverLifecycleAsset**);
    IsleGameResult (*fx_bank)(IsleRiverLifecycleSource*, int32_t, IsleRiverLifecycleAsset*, IsleRiverLifecycleAsset*);
    IsleGameResult (*fx_camera)(IsleRiverLifecycleSource*, int32_t, int32_t, IsleGameObject*);
    IsleGameResult (*fx_allocate)(IsleRiverLifecycleSource*, int32_t);
    IsleGameResult (*fx_dither)(IsleRiverLifecycleSource*, int32_t, int32_t);
    IsleGameResult (*depth_allocate)(IsleRiverLifecycleSource*, uint32_t, uint32_t, void**);
    IsleGameResult (*screen)(IsleRiverLifecycleSource*, const IsleRiverLifecycleScreen*);
    IsleGameResult (*fx_bank_count)(IsleRiverLifecycleSource*, size_t*);
    IsleGameResult (*fx_bank_reset)(IsleRiverLifecycleSource*, size_t);
    IsleGameResult (*photo_reset)(IsleRiverLifecycleSource*);
    IsleGameResult (*pokemons_init)(IsleRiverLifecycleSource*);
    IsleGameResult (*background)(IsleRiverLifecycleSource*, uint8_t*, uint8_t*, uint8_t*);
    IsleGameResult (*create_cameras)(IsleRiverLifecycleSource*, uint32_t);
    IsleGameResult (*ui_bind)(IsleRiverLifecycleSource*, IsleRiverLifecycleWorldSingle, IsleRiverLifecycleMovement, IsleGameSourceCallback, uint8_t, IsleRiverItemCollision);
    IsleGameResult (*end_callback)(IsleRiverLifecycleSource*, IsleRiverLifecycleEnd);
    IsleGameResult (*pause_callback)(IsleRiverLifecycleSource*, IsleRiverLifecycleEnd);
    IsleGameResult (*environment_init)(IsleRiverLifecycleSource*, const IsleRiverLifecycleEnvironment*, size_t);
    IsleGameResult (*detector_create)(IsleRiverLifecycleSource*);
    IsleGameResult (*detector_enable)(IsleRiverLifecycleSource*);
    IsleGameResult (*intervals)(IsleRiverLifecycleSource*, uint16_t, uint16_t);
    IsleGameResult (*near_clipping)(IsleRiverLifecycleSource*, int32_t);
    IsleGameResult (*photo_count)(IsleRiverLifecycleSource*, int32_t*);
    IsleGameResult (*request_scene)(IsleRiverLifecycleSource*, const IsleRiverLifecycleSceneSetup*);
    IsleGameResult (*scene_joined)(IsleRiverLifecycleSource*);
    IsleGameResult (*source_process)(IsleRiverLifecycleSource*, IsleGameObject*, IsleGameSourceCallback, int32_t, int32_t, IsleGameProcess**);
    IsleGameResult (*reference_assign)(IsleRiverLifecycleSource*, IsleGameObject**, IsleGameObject*);
} IsleRiverLifecycleServices;
struct IsleRiverLifecycleSource {
    void* owner;
    IsleContinuationDomain* domain;
    IsleRiverActorSource* actors;
    const IsleRiverLifecycleServices* services;
    IsleGameResult status, scene_completion_status;
    void* nonwaiting_call_frame;
    int32_t input_scene, output_scene;
    uint8_t running, completed, scene_requested, scene_completion_ready, scene_initialized;
    IsleRiverLifecycleSceneSetup scene_setup;
    IsleRiverLifecycleScreen screen;
    IsleRiverLifecycleEnvironment environment[62];
};
/* Preparation publishes no Game/assets and requires this same positively
 * PREPARED actor record at its final address, beside the independent owner. */
/* Pure independent-owner wire validation of all228 initialized lifecycle
 * fields, emitted privately from the fresh qualified638z exact typed rows. */
IsleGameResult isle_river_lifecycle_validate_initial_fields(const uint8_t*, size_t);
IsleGameResult isle_river_lifecycle_prepare(IsleRiverLifecycleSource*, void*, IsleContinuationDomain*, IsleRiverActorSource*, const IsleRiverLifecycleServices*, const uint8_t*, size_t, int32_t);
IsleGameResult isle_river_lifecycle_native_context(IsleGame*, IsleGameObject*, IsleRiverLifecycleSource**);
IsleGameResult isle_river_lifecycle_native_admission(const IsleRiverLifecycleSource*, const IsleRiverLifecycleServices*);
void isle_river_lifecycle_checked(IsleRiverLifecycleSource*, IsleGameResult);
void isle_river_lifecycle_enter(IsleRiverLifecycleSource*);
IsleRiverLifecycleSource* isle_river_lifecycle_current(IsleGameObject*);
IsleGameResult isle_river_lifecycle_invoke(IsleRiverLifecycleSource*, IsleRiverLifecycleCall, void*);
IsleGameResult isle_river_lifecycle_invoke_init(IsleRiverLifecycleSource*);
IsleGameResult isle_river_lifecycle_invoke_end(IsleRiverLifecycleSource*, int32_t);
IsleGameResult isle_river_lifecycle_invoke_world_pair(IsleRiverLifecycleSource*, IsleRiverLifecycleWorldPair, IsleRiverBlock*, IsleRiverBlock*);
IsleGameResult isle_river_lifecycle_invoke_world_single(IsleRiverLifecycleSource*, IsleRiverLifecycleWorldSingle, IsleRiverBlock*);
IsleGameResult isle_river_lifecycle_invoke_movement(IsleRiverLifecycleSource*, int32_t, float);
IsleGameResult isle_river_lifecycle_invoke_animation(IsleRiverLifecycleSource*, IsleRiverLifecycleAnimationCallback, const IsleActorNodeRef*, int32_t, float);
/* Healthy completion requires DETACHED plus actual successful Original post-init.
 * Failure may also complete never-activated PREPARED actors with no Game/services.
 * In both cases scene_joined reads only the independent owner's retained real
 * partial/full process/model/FX/GPU/Photo/audio joins, never old scene storage.
 * STOPPED/cancellation never publishes completed/output_scene. */
IsleGameResult isle_river_lifecycle_complete_scene(IsleRiverLifecycleSource*, IsleGameResult);
void isle_river_lifecycle_setup_scene(IsleRiverLifecycleSource*, const IsleRiverLifecycleSceneSetup*);
void isle_river_lifecycle_run(void*);
int32_t isle_river_lifecycle_sound_params(IsleRiverLifecycleSource* s, int32_t id, int32_t volume, int32_t pan, float pitch, int32_t priority);
int32_t isle_river_lifecycle_sound_active(IsleRiverLifecycleSource* s, int32_t handle);
void isle_river_lifecycle_sound_stop(IsleRiverLifecycleSource* s, int32_t handle);
void isle_river_lifecycle_sound_stop_all(IsleRiverLifecycleSource* s);
void isle_river_lifecycle_bgm_stop_all(IsleRiverLifecycleSource* s);
void isle_river_lifecycle_bgm_play(IsleRiverLifecycleSource* s, int32_t player, int32_t song);
void isle_river_lifecycle_bgm_stop(IsleRiverLifecycleSource* s, int32_t player);
void isle_river_lifecycle_bgm_volume(IsleRiverLifecycleSource* s, int32_t player, uint32_t volume);
void isle_river_lifecycle_bgm_channel_volume(IsleRiverLifecycleSource* s, int32_t player, int32_t channel, int32_t volume);
IsleGameObject* isle_river_lifecycle_camera(IsleRiverLifecycleSource* s);
const IsleRiverCameraView* isle_river_lifecycle_camera_read(IsleRiverLifecycleSource* s, IsleGameObject* camera);
void isle_river_lifecycle_camera_store(IsleRiverLifecycleSource* s, IsleGameObject* camera, uint32_t component, float value);
void isle_river_lifecycle_camera_speed(IsleRiverLifecycleSource* s, IsleGameObject* camera, float speed);
void isle_river_lifecycle_camera_animation(IsleRiverLifecycleSource* s, IsleGameObject* camera, uint32_t identity, float start);
IsleGameProcess* isle_river_lifecycle_camera_process(IsleRiverLifecycleSource* s, IsleGameObject* camera, int32_t kind, int32_t priority);
IsleGameObject* isle_river_lifecycle_player_model_init(IsleRiverLifecycleSource* s);
void isle_river_lifecycle_player_animation(IsleRiverLifecycleSource* s, uint32_t model, uint32_t material, float start, float speed);
void isle_river_lifecycle_animation_callback(IsleRiverLifecycleSource* s, IsleGameObject* receiver, IsleRiverLifecycleAnimationCallback callback);
IsleGameObject* isle_river_lifecycle_create_helper(IsleRiverLifecycleSource* s, int32_t id, IsleGameSourceCallback update, int32_t link, uint32_t priority);
void isle_river_lifecycle_delete_object(IsleRiverLifecycleSource* s, IsleGameObject* object);
void isle_river_lifecycle_notice(IsleRiverLifecycleSource* s, uint32_t bits);
IsleRiverEffect* isle_river_lifecycle_effect_create(IsleRiverLifecycleSource* s, int32_t bank, int32_t effect);
void isle_river_lifecycle_effect_attach(IsleRiverLifecycleSource* s, IsleRiverEffect* effect, const IsleActorNodeRef* receiver);
uint16_t isle_river_lifecycle_input_pressed(IsleRiverLifecycleSource* s);
void isle_river_lifecycle_world_add(IsleRiverLifecycleSource* s, IsleRiverBlock* a, IsleRiverBlock* b, uint32_t registry);
void isle_river_lifecycle_world_change(IsleRiverLifecycleSource* s, IsleRiverBlock* a, IsleRiverBlock* b, uint32_t registry);
void isle_river_lifecycle_world_remove(IsleRiverLifecycleSource* s, IsleRiverBlock* block, uint32_t registry);
void isle_river_lifecycle_height_map(IsleRiverLifecycleSource* s, uint32_t identity);
void isle_river_lifecycle_create_world(IsleRiverLifecycleSource* s, uint32_t setup, int32_t sky, int32_t minimum, int32_t maximum, int32_t link, int32_t draw_link, IsleRiverLifecycleWorldPair add, IsleRiverLifecycleWorldSingle remove, IsleRiverLifecycleWorldPair change);
void isle_river_lifecycle_sky_follow(IsleRiverLifecycleSource* s);
void isle_river_lifecycle_pokemon_registry(IsleRiverLifecycleSource* s, int32_t a, int32_t b, int32_t c, int32_t d);
void isle_river_lifecycle_transition_end(IsleRiverLifecycleSource* s, int32_t reason);
void isle_river_lifecycle_destroy_world(IsleRiverLifecycleSource* s);
void isle_river_lifecycle_environment_cleanup(IsleRiverLifecycleSource* s);
void isle_river_lifecycle_detector_disable(IsleRiverLifecycleSource* s);
void isle_river_lifecycle_remove_camera(IsleRiverLifecycleSource* s);
void isle_river_lifecycle_exit_scene(IsleRiverLifecycleSource* s);
void isle_river_lifecycle_course_cleanup(IsleRiverLifecycleSource* s);
int32_t isle_river_lifecycle_flute_command(IsleRiverLifecycleSource* s);
IsleRiverBlock* isle_river_lifecycle_block_next(IsleRiverLifecycleSource* s, IsleRiverBlock* block);
IsleGameVec3 isle_river_lifecycle_block_position(IsleRiverLifecycleSource* s, IsleRiverBlock* block);
void isle_river_lifecycle_displace_items(IsleRiverLifecycleSource* s, float x, float y, float z);
IsleRiverLifecycleAsset* isle_river_lifecycle_load_asset(IsleRiverLifecycleSource* s, IsleRiverLifecycleAssetId asset);
void isle_river_lifecycle_fx_bank(IsleRiverLifecycleSource* s, int32_t bank, IsleRiverLifecycleAsset* script, IsleRiverLifecycleAsset* sprites);
void isle_river_lifecycle_fx_camera(IsleRiverLifecycleSource* s, int32_t a, int32_t b, IsleGameObject* camera);
void isle_river_lifecycle_fx_allocate(IsleRiverLifecycleSource* s, int32_t count);
void isle_river_lifecycle_fx_dither(IsleRiverLifecycleSource* s, int32_t a, int32_t b);
void* isle_river_lifecycle_depth_allocate(IsleRiverLifecycleSource* s, uint32_t bytes, uint32_t alignment);
void isle_river_lifecycle_screen(IsleRiverLifecycleSource* s, const IsleRiverLifecycleScreen* screen);
size_t isle_river_lifecycle_fx_bank_count(IsleRiverLifecycleSource* s);
void isle_river_lifecycle_fx_bank_reset(IsleRiverLifecycleSource* s, size_t bank);
void isle_river_lifecycle_photo_reset(IsleRiverLifecycleSource* s);
void isle_river_lifecycle_pokemons_init(IsleRiverLifecycleSource* s);
void isle_river_lifecycle_background(IsleRiverLifecycleSource* s, uint8_t* r, uint8_t* g, uint8_t* b);
void isle_river_lifecycle_create_cameras(IsleRiverLifecycleSource* s, uint32_t background);
void isle_river_lifecycle_ui_bind(IsleRiverLifecycleSource* s, IsleRiverLifecycleWorldSingle exit, IsleRiverLifecycleMovement movement, IsleGameSourceCallback items, uint8_t kind, IsleRiverItemCollision collision);
void isle_river_lifecycle_end_callback(IsleRiverLifecycleSource* s, IsleRiverLifecycleEnd callback);
void isle_river_lifecycle_pause_callback(IsleRiverLifecycleSource* s, IsleRiverLifecycleEnd callback);
void isle_river_lifecycle_environment_init(IsleRiverLifecycleSource* s, const IsleRiverLifecycleEnvironment* rows, size_t count);
void isle_river_lifecycle_detector_create(IsleRiverLifecycleSource* s);
void isle_river_lifecycle_detector_enable(IsleRiverLifecycleSource* s);
void isle_river_lifecycle_intervals(IsleRiverLifecycleSource* s, uint16_t update, uint16_t draw);
void isle_river_lifecycle_near_clipping(IsleRiverLifecycleSource* s, int32_t disabled);
int32_t isle_river_lifecycle_photo_count(IsleRiverLifecycleSource* s);
void isle_river_lifecycle_request_scene(IsleRiverLifecycleSource* s, const IsleRiverLifecycleSceneSetup* setup);
void isle_river_lifecycle_scene_joined(IsleRiverLifecycleSource* s);
IsleGameProcess* isle_river_lifecycle_source_process(IsleRiverLifecycleSource* s, IsleGameObject* object, IsleGameSourceCallback callback, int32_t kind, int32_t priority);
void isle_river_lifecycle_reference_assign(IsleRiverLifecycleSource* s, IsleGameObject** field, IsleGameObject* value);
void func_802D8800_6C05E0(IsleRiverLifecycleSource* s, IsleRiverBlock* arg0, IsleRiverBlock* arg1);
void func_802D8824_6C0604(IsleRiverLifecycleSource* s, IsleRiverBlock* arg0, IsleRiverBlock* arg1);
void func_802D8848_6C0628(IsleRiverLifecycleSource* s, IsleRiverBlock* arg0);
void func_802D886C_6C064C(IsleRiverLifecycleSource* s);
void func_802D88F8_6C06D8(IsleRiverLifecycleSource* s, int32_t arg0);
void func_802D895C_6C073C(IsleGameObject* obj);
void func_802D89AC_6C078C(IsleRiverLifecycleSource* s, int32_t arg0);
void func_802D89B4_6C0794(IsleRiverLifecycleSource* s, IsleRiverBlock* arg0);
void func_802D8FCC_6C0DAC(IsleRiverLifecycleSource* s);
void func_802D90B8_6C0E98(IsleRiverLifecycleSource* s);
void func_802D9208_6C0FE8(IsleRiverLifecycleSource* s, int32_t arg0);
int32_t func_802D9210_6C0FF0(IsleRiverLifecycleSource* s, int32_t sceneId);
void func_802D92B0_6C1090(IsleRiverLifecycleSource* s, int32_t arg0, float arg1);
void func_802E20D0_6C9EB0(IsleGameObject* obj);
void func_802E2100_6C9EE0(IsleRiverLifecycleSource* s);
void func_802E2120_6C9F00(IsleRiverLifecycleSource* s, const IsleActorNodeRef* receiver, int32_t arg1, float arg2);
void func_802E2194_6C9F74(IsleRiverLifecycleSource* s);
void func_802E222C_6CA00C(IsleGameObject* obj);
void func_802E22E4_6CA0C4(IsleGameObject* obj);
void func_802E25C4_6CA3A4(IsleRiverLifecycleSource* s);
#ifdef __cplusplus
}
#endif
#endif
