#ifndef ISLE_CAVE_LIFECYCLE_SOURCE_H
#define ISLE_CAVE_LIFECYCLE_SOURCE_H
#include "cave_actor_source.h"
#include "35mmisle/continuation.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct IsleCaveLifecycleSource IsleCaveLifecycleSource;
typedef struct IsleCaveLifecycleAsset IsleCaveLifecycleAsset;
typedef struct IsleCaveLifecycleEnvironment {
    uint16_t sound;
    uint8_t pitch_modifier, hearing_range;
} IsleCaveLifecycleEnvironment;
typedef struct IsleCaveLifecycleDefinition {
    uint32_t id, initialize_identity, update_identity, remove_identity;
} IsleCaveLifecycleDefinition;
/* Semantic wire decode, not guest SceneSetup/ScreenSettings layouts. Guest
 * callbacks remain numeric identities; none is converted to a host pointer. */
typedef struct IsleCaveLifecycleSceneSetup {
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
} IsleCaveLifecycleSceneSetup;
typedef struct IsleCaveLifecycleScreen {
    uint32_t framebuffer_identity[3], initial_depth_identity, width, height, flags;
    void* depth_buffer;
} IsleCaveLifecycleScreen;
typedef enum IsleCaveLifecycleAssetId { ISLE_CAVE_FX_SCRIPT_COMMON, ISLE_CAVE_FX_IMAGES_COMMON, ISLE_CAVE_FX_SCRIPT_CAVE, ISLE_CAVE_FX_IMAGES_CAVE } IsleCaveLifecycleAssetId;
typedef void (*IsleCaveWorldPairCallback)(IsleCaveLifecycleSource*, IsleCaveWorldBlock*, IsleCaveWorldBlock*);
typedef void (*IsleCaveWorldSingleCallback)(IsleCaveLifecycleSource*, IsleCaveWorldBlock*);
typedef void (*IsleCaveEndCallback)(IsleCaveLifecycleSource*, int32_t);
typedef void (*IsleCaveMovementCallback)(IsleCaveActorSource*, int32_t, float);
typedef void (*IsleCaveItemCallback)(IsleGameObject*, IsleActorGround*);
typedef void (*IsleCaveLifecycleAnimationCallback)(IsleCaveLifecycleSource*, const IsleActorNodeRef*, int32_t, float);
typedef struct IsleCaveLifecycleServices {
    IsleGameResult (*reference)(IsleCaveLifecycleSource*, IsleGameObject** field, IsleGameObject* object);
    IsleGameResult (*block_position)(IsleCaveLifecycleSource*, IsleCaveWorldBlock* block, IsleGameVec3* output);
    IsleGameResult (*sound_params)(IsleCaveLifecycleSource*, int32_t id, int32_t volume, int32_t pan, float pitch, int32_t reverb, int32_t* output);
    IsleGameResult (*sound_active)(IsleCaveLifecycleSource*, int32_t handle, int32_t* output);
    IsleGameResult (*sound_stop)(IsleCaveLifecycleSource*, int32_t handle);
    IsleGameResult (*sound_stop_all)(IsleCaveLifecycleSource*);
    IsleGameResult (*bgm_stop_all)(IsleCaveLifecycleSource*);
    IsleGameResult (*camera_reset)(IsleCaveLifecycleSource*);
    IsleGameResult (*camera_speed)(IsleCaveLifecycleSource*, IsleCaveCamera* camera, float speed);
    IsleGameResult (*camera_animation)(IsleCaveLifecycleSource*, IsleCaveCamera* camera, uint32_t animation, float start);
    IsleGameResult (*player_animation)(IsleCaveLifecycleSource*, uint32_t models, uint32_t materials, float start, float speed);
    IsleGameResult (*animation_callback)(IsleCaveLifecycleSource*, IsleGameObject* object, IsleCaveLifecycleAnimationCallback callback);
    IsleGameResult (*main_camera)(IsleCaveLifecycleSource*, IsleCaveCamera** output);
    IsleGameResult (*player_model_init)(IsleCaveLifecycleSource*, IsleGameObject** output);
    IsleGameResult (*camera_process)(IsleCaveLifecycleSource*, IsleGameObject* object, int32_t kind, int32_t priority, IsleGameProcess** output);
    IsleGameResult (*player_process)(IsleCaveLifecycleSource*, IsleGameObject* object, IsleGameSourceCallback callback, int32_t kind, int32_t priority, IsleGameProcess** output);
    IsleGameResult (*effect_create)(IsleCaveLifecycleSource*, int32_t bank, int32_t id, IsleCaveEffect** output);
    IsleGameResult (*effect_model)(IsleCaveLifecycleSource*, IsleCaveEffect* effect, IsleActorNodeRef model);
    IsleGameResult (*camera_read)(IsleCaveLifecycleSource*, IsleCaveCamera* camera, uint32_t component, float* output);
    IsleGameResult (*camera_store)(IsleCaveLifecycleSource*, IsleCaveCamera* camera, uint32_t component, float value);
    IsleGameResult (*input_pressed)(IsleCaveLifecycleSource*, uint32_t* output);
    IsleGameResult (*camera_object)(IsleCaveLifecycleSource*, IsleCaveCamera* camera, IsleGameObject** output);
    IsleGameResult (*height_map)(IsleCaveLifecycleSource*, uint32_t identity);
    IsleGameResult (*ceiling_map)(IsleCaveLifecycleSource*, uint32_t identity);
    IsleGameResult (*world_create)(IsleCaveLifecycleSource*, uint32_t descriptor, int32_t sky, int32_t ground, int32_t camera, int32_t blocks, int32_t objects, IsleCaveWorldPairCallback add, IsleCaveWorldSingleCallback remove, IsleCaveWorldPairCallback change);
    IsleGameResult (*sky_follow)(IsleCaveLifecycleSource*);
    IsleGameResult (*source_80363928)(IsleCaveLifecycleSource*, int32_t a, int32_t b, int32_t c, int32_t d);
    IsleGameResult (*source_800A19D8)(IsleCaveLifecycleSource*, int32_t reason);
    IsleGameResult (*detector_disable)(IsleCaveLifecycleSource*);
    IsleGameResult (*environment_cleanup)(IsleCaveLifecycleSource*);
    IsleGameResult (*world_destroy)(IsleCaveLifecycleSource*);
    IsleGameResult (*source_80356FBC)(IsleCaveLifecycleSource*);
    IsleGameResult (*camera_remove)(IsleCaveLifecycleSource*);
    IsleGameResult (*source_800067DC)(IsleCaveLifecycleSource*);
    IsleGameResult (*items_displace)(IsleCaveLifecycleSource*, float x, float y, float z);
    IsleGameResult (*fx_bank)(IsleCaveLifecycleSource*, int32_t bank, IsleCaveLifecycleAsset* script, IsleCaveLifecycleAsset* sprites);
    IsleGameResult (*fx_dither)(IsleCaveLifecycleSource*, int32_t color, int32_t alpha);
    IsleGameResult (*screen_apply)(IsleCaveLifecycleSource*, const IsleCaveLifecycleScreen* screen);
    IsleGameResult (*fx_reset_bank)(IsleCaveLifecycleSource*, size_t index);
    IsleGameResult (*source_8009CE00)(IsleCaveLifecycleSource*);
    IsleGameResult (*pokemon_initialize)(IsleCaveLifecycleSource*);
    IsleGameResult (*background_color)(IsleCaveLifecycleSource*, uint8_t* r, uint8_t* g, uint8_t* b);
    IsleGameResult (*camera_create)(IsleCaveLifecycleSource*, uint32_t color);
    IsleGameResult (*camera_flags)(IsleCaveLifecycleSource*, uint32_t mask, uint32_t value);
    IsleGameResult (*ui_initialize)(IsleCaveLifecycleSource*, IsleCaveWorldSingleCallback exit, IsleCaveMovementCallback movement, IsleGameSourceCallback update_items, uint8_t item_kind, IsleCaveItemCallback collision);
    IsleGameResult (*end_callback)(IsleCaveLifecycleSource*, IsleCaveEndCallback callback);
    IsleGameResult (*pause_callback)(IsleCaveLifecycleSource*, IsleCaveEndCallback callback);
    IsleGameResult (*environment_initialize)(IsleCaveLifecycleSource*, const IsleCaveLifecycleEnvironment* rows, size_t count);
    IsleGameResult (*detector_create)(IsleCaveLifecycleSource*);
    IsleGameResult (*detector_enable)(IsleCaveLifecycleSource*);
    IsleGameResult (*pitch_limits)(IsleCaveLifecycleSource*, float minimum, float maximum);
    IsleGameResult (*intervals)(IsleCaveLifecycleSource*, int32_t update, int32_t draw);
    IsleGameResult (*near_clipping)(IsleCaveLifecycleSource*, int32_t disabled);
    IsleGameResult (*source_800AAED0)(IsleCaveLifecycleSource*, int32_t flags);
    IsleGameResult (*source_803570B0)(IsleCaveLifecycleSource*);
    IsleGameResult (*source_80357120)(IsleCaveLifecycleSource*, IsleGameObject* object);
    IsleGameResult (*source_80357170)(IsleCaveLifecycleSource*);
    IsleGameResult (*source_803571C4)(IsleCaveLifecycleSource*);
    IsleGameResult (*source_803572B0)(IsleCaveLifecycleSource*);
    IsleGameResult (*block_next)(IsleCaveLifecycleSource*, IsleCaveWorldBlock* block, IsleCaveWorldBlock** output);
    IsleGameResult (*asset_load)(IsleCaveLifecycleSource*, IsleCaveLifecycleAssetId id, IsleCaveLifecycleAsset** output);
    IsleGameResult (*fx_pool)(IsleCaveLifecycleSource*, int32_t count, IsleGameObject** output);
    IsleGameResult (*fx_manager)(IsleCaveLifecycleSource*, int32_t link, int32_t count, IsleCaveCamera* camera, IsleGameObject** output);
    IsleGameResult (*allocate)(IsleCaveLifecycleSource*, size_t bytes, size_t alignment, void** output);
    IsleGameResult (*add_helper)(IsleCaveLifecycleSource*, int32_t id, IsleGameSourceCallback callback, uint8_t link, uint32_t priority, IsleGameObject** output);
    IsleGameResult (*source_8009BC68)(IsleCaveLifecycleSource*, int32_t* output);
    IsleGameResult (*scene_request)(IsleCaveLifecycleSource*, const IsleCaveLifecycleSceneSetup* setup);
    IsleGameResult (*pokemon_add)(IsleCaveLifecycleSource*, IsleCaveWorldBlock* a, IsleCaveWorldBlock* b, const IsleCaveLifecycleDefinition* definitions, size_t count);
    IsleGameResult (*pokemon_change)(IsleCaveLifecycleSource*, IsleCaveWorldBlock* a, IsleCaveWorldBlock* b, const IsleCaveLifecycleDefinition* definitions, size_t count);
    IsleGameResult (*pokemon_remove)(IsleCaveLifecycleSource*, IsleCaveWorldBlock* block, const IsleCaveLifecycleDefinition* definitions, size_t count);
    IsleGameResult (*fx_manager_assign)(IsleCaveLifecycleSource*, IsleGameObject* object);
    IsleGameResult (*fx_pool_assign)(IsleCaveLifecycleSource*, IsleGameObject* object);
    IsleGameResult (*scene_joined)(IsleCaveLifecycleSource*);
} IsleCaveLifecycleServices;
struct IsleCaveLifecycleSource {
    void* owner;
    IsleContinuationDomain* domain;
    IsleCaveActorSource* actors;
    const IsleCaveLifecycleServices* services;
    IsleGameResult status, scene_completion_status;
    void* nonwaiting_call_frame;
    int32_t input_scene, output_scene;
    uint8_t running, completed, scene_requested, scene_completion_ready, entry_executing;
    IsleCaveLifecycleSceneSetup scene_setup;
    IsleCaveLifecycleScreen screen;
    IsleCaveLifecycleEnvironment environment[57];
    IsleCaveLifecycleDefinition definitions[18];
};
/* prepare is pre-Game. actors is the same fixed-address PREPARED record and
 * owner is independent of disposable scene storage. All original globals,
 * including BGM/intro words, are borrowed through actors; none is mirrored. */
IsleGameResult isle_cave_lifecycle_prepare(IsleCaveLifecycleSource*, void*, IsleContinuationDomain*, IsleCaveActorSource*, const IsleCaveLifecycleServices*, const uint8_t*, size_t, int32_t);
IsleGameResult isle_cave_lifecycle_native_context(IsleGame*, IsleGameObject*, IsleCaveLifecycleSource**);
IsleCaveLifecycleSource* isle_cave_lifecycle_require(IsleGame*, IsleGameObject*);
void isle_cave_lifecycle_checked(IsleCaveLifecycleSource*, IsleGameResult);
void isle_cave_lifecycle_run(void*);
void isle_cave_lifecycle_setup_scene(IsleCaveLifecycleSource*, const IsleCaveLifecycleSceneSetup*);
/* Real scene/process/reference/model/FX/GPU/readback/Photo/audio joins are
 * REQUIRED before complete_scene. STOPPED never becomes original success. */
IsleGameResult isle_cave_lifecycle_complete_scene(IsleCaveLifecycleSource*, IsleGameResult);
/* Owner-only outside all Source/native traversals. Reports genuine pre-Game
 * setup failure or cancellation without consulting a Game or faking scene join.
 * The independent entry's next clock observes the first fault and fails in its
 * own C domain, even if native scene resources still require actual joins. */
IsleGameResult isle_cave_lifecycle_report_failure(IsleCaveLifecycleSource*, IsleGameResult);
IsleGameResult isle_cave_lifecycle_invoke_world_pair(IsleCaveLifecycleSource*,IsleCaveWorldPairCallback,IsleCaveWorldBlock*,IsleCaveWorldBlock*);
IsleGameResult isle_cave_lifecycle_invoke_world_single(IsleCaveLifecycleSource*,IsleCaveWorldSingleCallback,IsleCaveWorldBlock*);
IsleGameResult isle_cave_lifecycle_invoke_end_callback(IsleCaveLifecycleSource*,IsleCaveEndCallback,int32_t);
typedef void (*IsleCaveLifecycleCall)(IsleCaveLifecycleSource*, void*);
IsleGameResult isle_cave_lifecycle_invoke(IsleCaveLifecycleSource*, IsleCaveLifecycleCall, void*);
IsleGameResult isle_cave_lifecycle_invoke_init(IsleCaveLifecycleSource*);
IsleGameResult isle_cave_lifecycle_invoke_end(IsleCaveLifecycleSource*,int32_t);
IsleGameResult isle_cave_lifecycle_invoke_animation(IsleCaveLifecycleSource*,const IsleActorNodeRef*,int32_t,float);
IsleGameResult isle_cave_lifecycle_native_animation_node(IsleCaveLifecycleSource*,const IsleActorNodeRef*);
void isle_cave_lifecycle_reference(IsleCaveLifecycleSource* s, IsleGameObject** field, IsleGameObject* object);
IsleGameVec3 isle_cave_lifecycle_block_position(IsleCaveLifecycleSource* s, IsleCaveWorldBlock* block);
int32_t isle_cave_lifecycle_sound_params(IsleCaveLifecycleSource* s, int32_t id, int32_t volume, int32_t pan, float pitch, int32_t reverb);
int32_t isle_cave_lifecycle_sound_active(IsleCaveLifecycleSource* s, int32_t handle);
void isle_cave_lifecycle_sound_stop(IsleCaveLifecycleSource* s, int32_t handle);
void isle_cave_lifecycle_sound_stop_all(IsleCaveLifecycleSource* s);
void isle_cave_lifecycle_bgm_stop_all(IsleCaveLifecycleSource* s);
void isle_cave_lifecycle_camera_reset(IsleCaveLifecycleSource* s);
void isle_cave_lifecycle_camera_speed(IsleCaveLifecycleSource* s, IsleCaveCamera* camera, float speed);
void isle_cave_lifecycle_camera_animation(IsleCaveLifecycleSource* s, IsleCaveCamera* camera, uint32_t animation, float start);
void isle_cave_lifecycle_player_animation(IsleCaveLifecycleSource* s, uint32_t models, uint32_t materials, float start, float speed);
void isle_cave_lifecycle_animation_callback(IsleCaveLifecycleSource* s, IsleGameObject* object, IsleCaveLifecycleAnimationCallback callback);
IsleCaveCamera* isle_cave_lifecycle_main_camera(IsleCaveLifecycleSource* s);
IsleGameObject* isle_cave_lifecycle_player_model_init(IsleCaveLifecycleSource* s);
IsleGameProcess* isle_cave_lifecycle_camera_process(IsleCaveLifecycleSource* s, IsleGameObject* object, int32_t kind, int32_t priority);
IsleGameProcess* isle_cave_lifecycle_player_process(IsleCaveLifecycleSource* s, IsleGameObject* object, IsleGameSourceCallback callback, int32_t kind, int32_t priority);
IsleCaveEffect* isle_cave_lifecycle_effect_create(IsleCaveLifecycleSource* s, int32_t bank, int32_t id);
void isle_cave_lifecycle_effect_model(IsleCaveLifecycleSource* s, IsleCaveEffect* effect, IsleActorNodeRef model);
float isle_cave_lifecycle_camera_read(IsleCaveLifecycleSource* s, IsleCaveCamera* camera, uint32_t component);
void isle_cave_lifecycle_camera_store(IsleCaveLifecycleSource* s, IsleCaveCamera* camera, uint32_t component, float value);
uint32_t isle_cave_lifecycle_input_pressed(IsleCaveLifecycleSource* s);
IsleGameObject* isle_cave_lifecycle_camera_object(IsleCaveLifecycleSource* s, IsleCaveCamera* camera);
void isle_cave_lifecycle_height_map(IsleCaveLifecycleSource* s, uint32_t identity);
void isle_cave_lifecycle_ceiling_map(IsleCaveLifecycleSource* s, uint32_t identity);
void isle_cave_lifecycle_world_create(IsleCaveLifecycleSource* s, uint32_t descriptor, int32_t sky, int32_t ground, int32_t camera, int32_t blocks, int32_t objects, IsleCaveWorldPairCallback add, IsleCaveWorldSingleCallback remove, IsleCaveWorldPairCallback change);
void isle_cave_lifecycle_sky_follow(IsleCaveLifecycleSource* s);
void isle_cave_lifecycle_source_80363928(IsleCaveLifecycleSource* s, int32_t a, int32_t b, int32_t c, int32_t d);
void isle_cave_lifecycle_source_800A19D8(IsleCaveLifecycleSource* s, int32_t reason);
void isle_cave_lifecycle_detector_disable(IsleCaveLifecycleSource* s);
void isle_cave_lifecycle_environment_cleanup(IsleCaveLifecycleSource* s);
void isle_cave_lifecycle_world_destroy(IsleCaveLifecycleSource* s);
void isle_cave_lifecycle_source_80356FBC(IsleCaveLifecycleSource* s);
void isle_cave_lifecycle_camera_remove(IsleCaveLifecycleSource* s);
void isle_cave_lifecycle_source_800067DC(IsleCaveLifecycleSource* s);
void isle_cave_lifecycle_items_displace(IsleCaveLifecycleSource* s, float x, float y, float z);
void isle_cave_lifecycle_fx_bank(IsleCaveLifecycleSource* s, int32_t bank, IsleCaveLifecycleAsset* script, IsleCaveLifecycleAsset* sprites);
void isle_cave_lifecycle_fx_dither(IsleCaveLifecycleSource* s, int32_t color, int32_t alpha);
void isle_cave_lifecycle_screen_apply(IsleCaveLifecycleSource* s, const IsleCaveLifecycleScreen* screen);
void isle_cave_lifecycle_fx_reset_bank(IsleCaveLifecycleSource* s, size_t index);
void isle_cave_lifecycle_source_8009CE00(IsleCaveLifecycleSource* s);
void isle_cave_lifecycle_pokemon_initialize(IsleCaveLifecycleSource* s);
void isle_cave_lifecycle_background_color(IsleCaveLifecycleSource* s, uint8_t* r, uint8_t* g, uint8_t* b);
void isle_cave_lifecycle_camera_create(IsleCaveLifecycleSource* s, uint32_t color);
void isle_cave_lifecycle_camera_flags(IsleCaveLifecycleSource* s, uint32_t mask, uint32_t value);
void isle_cave_lifecycle_ui_initialize(IsleCaveLifecycleSource* s, IsleCaveWorldSingleCallback exit, IsleCaveMovementCallback movement, IsleGameSourceCallback update_items, uint8_t item_kind, IsleCaveItemCallback collision);
void isle_cave_lifecycle_end_callback(IsleCaveLifecycleSource* s, IsleCaveEndCallback callback);
void isle_cave_lifecycle_pause_callback(IsleCaveLifecycleSource* s, IsleCaveEndCallback callback);
void isle_cave_lifecycle_environment_initialize(IsleCaveLifecycleSource* s, const IsleCaveLifecycleEnvironment* rows, size_t count);
void isle_cave_lifecycle_detector_create(IsleCaveLifecycleSource* s);
void isle_cave_lifecycle_detector_enable(IsleCaveLifecycleSource* s);
void isle_cave_lifecycle_pitch_limits(IsleCaveLifecycleSource* s, float minimum, float maximum);
void isle_cave_lifecycle_intervals(IsleCaveLifecycleSource* s, int32_t update, int32_t draw);
void isle_cave_lifecycle_near_clipping(IsleCaveLifecycleSource* s, int32_t disabled);
void isle_cave_lifecycle_source_800AAED0(IsleCaveLifecycleSource* s, int32_t flags);
void isle_cave_lifecycle_source_803570B0(IsleCaveLifecycleSource* s);
void isle_cave_lifecycle_source_80357120(IsleCaveLifecycleSource* s, IsleGameObject* object);
void isle_cave_lifecycle_source_80357170(IsleCaveLifecycleSource* s);
void isle_cave_lifecycle_source_803571C4(IsleCaveLifecycleSource* s);
void isle_cave_lifecycle_source_803572B0(IsleCaveLifecycleSource* s);
IsleCaveWorldBlock* isle_cave_lifecycle_block_next(IsleCaveLifecycleSource* s, IsleCaveWorldBlock* block);
IsleCaveLifecycleAsset* isle_cave_lifecycle_asset_load(IsleCaveLifecycleSource* s, IsleCaveLifecycleAssetId id);
IsleGameObject* isle_cave_lifecycle_fx_pool(IsleCaveLifecycleSource* s, int32_t count);
IsleGameObject* isle_cave_lifecycle_fx_manager(IsleCaveLifecycleSource* s, int32_t link, int32_t count, IsleCaveCamera* camera);
void* isle_cave_lifecycle_allocate(IsleCaveLifecycleSource* s, size_t bytes, size_t alignment);
IsleGameObject* isle_cave_lifecycle_add_helper(IsleCaveLifecycleSource* s, int32_t id, IsleGameSourceCallback callback, uint8_t link, uint32_t priority);
int32_t isle_cave_lifecycle_source_8009BC68(IsleCaveLifecycleSource* s);
void isle_cave_lifecycle_scene_request(IsleCaveLifecycleSource* s, const IsleCaveLifecycleSceneSetup* setup);
void isle_cave_lifecycle_pokemon_add(IsleCaveLifecycleSource* s, IsleCaveWorldBlock* a, IsleCaveWorldBlock* b, const IsleCaveLifecycleDefinition* definitions, size_t count);
void isle_cave_lifecycle_pokemon_change(IsleCaveLifecycleSource* s, IsleCaveWorldBlock* a, IsleCaveWorldBlock* b, const IsleCaveLifecycleDefinition* definitions, size_t count);
void isle_cave_lifecycle_pokemon_remove(IsleCaveLifecycleSource* s, IsleCaveWorldBlock* block, const IsleCaveLifecycleDefinition* definitions, size_t count);
void isle_cave_lifecycle_fx_manager_assign(IsleCaveLifecycleSource* s, IsleGameObject* object);
void isle_cave_lifecycle_fx_pool_assign(IsleCaveLifecycleSource* s, IsleGameObject* object);
void func_8012A0C0_1F5E70(IsleCaveLifecycleSource* s);
void func_802BDF34_6403E4(IsleCaveLifecycleSource* s, IsleCaveWorldBlock* arg0, IsleCaveWorldBlock* arg1);
void func_802BDF58_640408(IsleCaveLifecycleSource* s, IsleCaveWorldBlock* arg0, IsleCaveWorldBlock* arg1);
void func_802BDF7C_64042C(IsleCaveLifecycleSource* s, IsleCaveWorldBlock* arg0);
void func_802BDFA0_640450(IsleCaveLifecycleSource* s);
void func_802BE038_6404E8(IsleCaveLifecycleSource* s, int32_t arg0);
void func_802BE09C_64054C(IsleGameObject* arg0);
void func_802BE0A4_640554(IsleCaveLifecycleSource* s, int32_t arg0);
void func_802BE0AC_64055C(IsleCaveLifecycleSource* s, IsleCaveWorldBlock* arg0);
void func_802BE140_6405F0(IsleCaveLifecycleSource* s);
void func_802BE22C_6406DC(IsleCaveLifecycleSource* s);
void func_802BE3A8_640858(IsleCaveLifecycleSource* s, int32_t arg0);
int32_t func_802BE3B0_640860(IsleCaveLifecycleSource* s, int32_t arg0);
void func_802C5B60_648010(IsleGameObject* arg0);
void func_802C5B90_648040(IsleCaveLifecycleSource* s);
void func_802C5BB0_648060(IsleCaveLifecycleSource* s, const IsleActorNodeRef* callback_model, int32_t arg1, float arg2);
void func_802C5C24_6480D4(IsleCaveLifecycleSource* s);
void func_802C5CEC_64819C(IsleGameObject* obj);
void func_802C5D80_648230(IsleGameObject* obj);
void func_802C5DFC_6482AC(IsleGameObject* obj);
void func_802C60F4_6485A4(IsleCaveLifecycleSource* s);
#ifdef __cplusplus
}
#endif
#endif
