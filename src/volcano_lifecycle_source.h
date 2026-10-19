#ifndef ISLE_VOLCANO_LIFECYCLE_SOURCE_H
#define ISLE_VOLCANO_LIFECYCLE_SOURCE_H
#include "volcano_actor_source.h"
#include "35mmisle/continuation.h"
#include "player_cart_source.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef void (*IsleVolcanoIntroAnimationCallback)(IsleVolcanoLifecycleSource*,const IslePlayerCartNode*,int32_t,float);
typedef void (*IsleVolcanoWorldAdd)(IsleVolcanoLifecycleSource*,IsleVolcanoBlock*,IsleVolcanoBlock*);
typedef void (*IsleVolcanoWorldRemove)(IsleVolcanoLifecycleSource*,IsleVolcanoBlock*);
typedef void (*IsleVolcanoEndCallback)(IsleVolcanoLifecycleSource*,int32_t);
typedef void (*IsleVolcanoPauseCallback)(IsleVolcanoLifecycleSource*,int32_t);
/* Semantic discriminants, NOT raw SDK bit values. Native FX applies the actual
 * Source G_CD_DISABLE / G_AD_DISABLE modes to its real producer state. */
typedef enum IsleVolcanoColorDither { ISLE_VOLCANO_COLOR_DITHER_DISABLED } IsleVolcanoColorDither;
typedef enum IsleVolcanoAlphaDither { ISLE_VOLCANO_ALPHA_DITHER_DISABLED } IsleVolcanoAlphaDither;
typedef struct IsleVolcanoEnvironment { uint16_t sound; uint8_t pitch, range; } IsleVolcanoEnvironment;
typedef struct IsleVolcanoScreen { uint32_t framebuffer1,framebuffer2; void* framebuffer3; void* depth; uint32_t width,height,flags; } IsleVolcanoScreen;
/* Source scene descriptor, semantic fields; sizes for native objects are chosen
 * by backend, never copied N64 sizeof values into native allocations. */
typedef struct IsleVolcanoScene {
    uint32_t heap_size;
    uint32_t contexts, display_list_bytes[4], graphics_heap_bytes, rdp_bytes;
    uint32_t thread_stack_bytes, sprite_bytes;
    uint32_t heap_base, update_callback, draw_callback, pre_render_callback, input_callback;
    uint32_t gtl_unknown00, gtl_unknown14, gtl_unknown30;
    uint32_t num_threads, num_stacks, unknown4c, num_processes, num_objects;
    uint32_t num_matrices, unknown60, num_animations, num_materials, num_models;
    uint32_t num_sprites, num_cameras;
    /* Source sizeof(GObj/DObj/OMCamera) becomes exact native structure size,
     * not copied four-byte-console layout sizes. */
    void (*post_init)(IsleVolcanoLifecycleSource*);
    void (*initialize)(IsleVolcanoLifecycleSource*,int32_t);
} IsleVolcanoScene;
typedef struct IsleVolcanoUi {
    IsleVolcanoWorldRemove exit_block;
    void (*movement)(IsleVolcanoActorSource*,int32_t,float);
    IsleGameSourceCallback item_update;
    uint8_t item_update_kind;
    void (*item_collision)(IsleGameObject*,IsleActorGround*);
} IsleVolcanoUi;
typedef enum IsleVolcanoLifecyclePhase { ISLE_VOLCANO_LIFECYCLE_UNPREPARED,ISLE_VOLCANO_LIFECYCLE_READY,ISLE_VOLCANO_LIFECYCLE_RUNNING,ISLE_VOLCANO_LIFECYCLE_COMPLETE } IsleVolcanoLifecyclePhase;
typedef struct IsleVolcanoLifecycleServices {
    IsleGameResult (*block_next)(IsleVolcanoLifecycleSource*, IsleVolcanoBlock*, IsleVolcanoBlock**);
    IsleGameResult (*block_position)(IsleVolcanoLifecycleSource*, IsleVolcanoBlock*, IsleGameVec3*);
    IsleGameResult (*displace_items)(IsleVolcanoLifecycleSource*, float, float, float);
    IsleGameResult (*world_add)(IsleVolcanoLifecycleSource*, IsleVolcanoBlock*, IsleVolcanoBlock*, const IsleVolcanoDefinition*, size_t);
    IsleGameResult (*world_change)(IsleVolcanoLifecycleSource*, IsleVolcanoBlock*, IsleVolcanoBlock*, const IsleVolcanoDefinition*, size_t);
    IsleGameResult (*world_remove)(IsleVolcanoLifecycleSource*, IsleVolcanoBlock*, const IsleVolcanoDefinition*, size_t);
    IsleGameResult (*height_map)(IsleVolcanoLifecycleSource*, uint32_t);
    IsleGameResult (*world_create)(IsleVolcanoLifecycleSource*, uint32_t, int32_t, int32_t, int32_t, int32_t, int32_t, IsleVolcanoWorldAdd, IsleVolcanoWorldRemove, IsleVolcanoWorldAdd);
    IsleGameResult (*sky_follow)(IsleVolcanoLifecycleSource*);
    IsleGameResult (*actor_initialize_range)(IsleVolcanoLifecycleSource*, int32_t, int32_t, int32_t, int32_t);
    IsleGameResult (*end_level)(IsleVolcanoLifecycleSource*, int32_t);
    IsleGameResult (*detector_disable)(IsleVolcanoLifecycleSource*);
    IsleGameResult (*environment_cleanup)(IsleVolcanoLifecycleSource*);
    IsleGameResult (*bgm_stop)(IsleVolcanoLifecycleSource*);
    IsleGameResult (*sound_stop_all)(IsleVolcanoLifecycleSource*);
    IsleGameResult (*world_destroy)(IsleVolcanoLifecycleSource*);
    IsleGameResult (*player_stop)(IsleVolcanoLifecycleSource*);
    IsleGameResult (*camera_remove)(IsleVolcanoLifecycleSource*);
    IsleGameResult (*scene_end)(IsleVolcanoLifecycleSource*);
    IsleGameResult (*load_asset)(IsleVolcanoLifecycleSource*, uint32_t, uint32_t, const IsleVolcanoAsset**);
    IsleGameResult (*fx_bank)(IsleVolcanoLifecycleSource*, int32_t, const IsleVolcanoAsset*, const IsleVolcanoAsset*);
    IsleGameResult (*fx_camera)(IsleVolcanoLifecycleSource*, int32_t, int32_t, IsleGameCamera*, IsleGameObject**);
    IsleGameResult (*fx_allocate)(IsleVolcanoLifecycleSource*, int32_t, IsleGameObject**);
    IsleGameResult (*fx_dither)(IsleVolcanoLifecycleSource*, IsleVolcanoColorDither, IsleVolcanoAlphaDither);
    IsleGameResult (*fx_reference)(IsleVolcanoLifecycleSource*, uint32_t, IsleGameObject*);
    IsleGameResult (*depth_allocate)(IsleVolcanoLifecycleSource*, uint32_t, uint32_t, void**);
    IsleGameResult (*apply_screen)(IsleVolcanoLifecycleSource*, const IsleVolcanoScreen*);
    IsleGameResult (*fx_reset_bank)(IsleVolcanoLifecycleSource*, size_t);
    IsleGameResult (*engine_initialize)(IsleVolcanoLifecycleSource*);
    IsleGameResult (*add_object)(IsleVolcanoLifecycleSource*, int32_t, IsleGameSourceCallback, int32_t, uint32_t, IsleGameObject**);
    IsleGameResult (*actors_initialize)(IsleVolcanoLifecycleSource*);
    IsleGameResult (*background_color)(IsleVolcanoLifecycleSource*, uint8_t*, uint8_t*, uint8_t*);
    IsleGameResult (*cameras_create)(IsleVolcanoLifecycleSource*, uint32_t);
    IsleGameResult (*ui_initialize)(IsleVolcanoLifecycleSource*, const IsleVolcanoUi*);
    IsleGameResult (*end_callback)(IsleVolcanoLifecycleSource*, IsleVolcanoEndCallback);
    IsleGameResult (*pause_callback)(IsleVolcanoLifecycleSource*, IsleVolcanoPauseCallback);
    IsleGameResult (*environment_initialize)(IsleVolcanoLifecycleSource*, const IsleVolcanoEnvironment*, size_t);
    IsleGameResult (*detector_create)(IsleVolcanoLifecycleSource*);
    IsleGameResult (*detector_enable)(IsleVolcanoLifecycleSource*);
    IsleGameResult (*illegal_copy)(IsleVolcanoLifecycleSource*, int32_t*);
    IsleGameResult (*player_flag)(IsleVolcanoLifecycleSource*, int32_t, int32_t);
    IsleGameResult (*set_intervals)(IsleVolcanoLifecycleSource*, int32_t, int32_t);
    IsleGameResult (*near_clipping)(IsleVolcanoLifecycleSource*, int32_t);
    IsleGameResult (*setup_scene)(IsleVolcanoLifecycleSource*, const IsleVolcanoScene*);
    IsleGameResult (*photo_count)(IsleVolcanoLifecycleSource*, int32_t*);
    IsleGameResult (*notice)(IsleVolcanoLifecycleSource*, uint32_t);
    IsleGameResult (*input_pressed)(IsleVolcanoLifecycleSource*, uint32_t*);
    IsleGameResult (*cutscene_prepare)(IsleVolcanoLifecycleSource*);
    IsleGameResult (*cutscene_model)(IsleVolcanoLifecycleSource*, IsleGameObject*);
    IsleGameResult (*cutscene_run)(IsleVolcanoLifecycleSource*);
    IsleGameResult (*cutscene_stop)(IsleVolcanoLifecycleSource*);
    IsleGameResult (*intro_restore)(IsleVolcanoLifecycleSource*);
    IsleGameResult (*effect_create)(IsleVolcanoLifecycleSource*, int32_t, int32_t, IsleVolcanoEffect**);
    IsleGameResult (*effect_attach)(IsleVolcanoLifecycleSource*, IsleVolcanoEffect*, const IslePlayerCartNode*);
    IsleGameResult (*camera_read)(IsleVolcanoLifecycleSource*, IsleGameCamera*, uint32_t, float*);
    IsleGameResult (*camera_store)(IsleVolcanoLifecycleSource*, IsleGameCamera*, uint32_t, float);
    IsleGameResult (*delete_intro)(IsleVolcanoLifecycleSource*);
    IsleGameResult (*animation_callback)(IsleVolcanoLifecycleSource*, IsleGameObject*, IsleVolcanoIntroAnimationCallback);
    IsleGameResult (*scene_joined)(IsleVolcanoLifecycleSource*);
    IsleGameResult (*entry_joined)(IsleVolcanoLifecycleSource*);
    IsleGameResult (*create_process)(IsleVolcanoLifecycleSource*, IsleGameObject*, IsleGameSourceCallback, int32_t, int32_t, IsleGameProcess**);
    IsleGameResult (*reference_assign)(IsleVolcanoLifecycleSource*, IsleGameObject**, IsleGameObject*);
    IsleGameResult (*sound_identity)(IsleVolcanoLifecycleSource*, uint32_t, int32_t*);
    IsleGameResult (*sound_stop)(IsleVolcanoLifecycleSource*, uint32_t);
    IsleGameResult (*sound_play_params)(IsleVolcanoLifecycleSource*, int32_t, int32_t, int32_t, float, int32_t, int32_t*);
    IsleGameResult (*main_camera)(IsleVolcanoLifecycleSource*, IsleGameCamera**);
    IsleGameResult (*camera_object)(IsleVolcanoLifecycleSource*, IsleGameCamera*, IsleGameObject**);
    IsleGameResult (*camera_speed)(IsleVolcanoLifecycleSource*, IsleGameCamera*, float);
    IsleGameResult (*camera_animation)(IsleVolcanoLifecycleSource*, IsleGameCamera*, uint32_t, float);
    IsleGameResult (*camera_process)(IsleVolcanoLifecycleSource*, IsleGameObject*, int32_t, int32_t, IsleGameProcess**);
    IsleGameResult (*player_animation)(IsleVolcanoLifecycleSource*, uint32_t, uint32_t, float, float);
    IsleGameResult (*player_model_init)(IsleVolcanoLifecycleSource*, IsleGameObject**);
    IsleGameResult (*reset_camera)(IsleVolcanoLifecycleSource*);
} IsleVolcanoLifecycleServices;
/* Independent stable entry record. No live-Game or actor fence is used to
 * recover entry/end/retry failures; owner/domain/services outlive scene Storage.
 * All mutable Source globals are borrowed through actors, never mirrored. */
struct IsleVolcanoLifecycleSource {
    void* owner;
    IsleContinuationDomain* domain;
    uint64_t clock;
    void* nonwaiting_call_frame;
    IsleGameResult status;
    IsleVolcanoLifecyclePhase phase;
    IsleVolcanoActorSource* actors;
    const IsleVolcanoLifecycleServices* services;
    IsleVolcanoScreen screen;
    IsleVolcanoScene scene;
    int scene_pending, scene_completed;
    IsleGameResult scene_result;
    int32_t input_scene, output_scene;
};
IsleGameResult isle_volcano_lifecycle_native_context(IsleGameObject*,IsleVolcanoLifecycleSource**);
IsleGameResult isle_volcano_lifecycle_prepare(IsleVolcanoLifecycleSource*,void*,IsleContinuationDomain*,IsleVolcanoActorSource*,const IsleVolcanoLifecycleServices*,int32_t);
IsleVolcanoLifecycleSource* isle_volcano_lifecycle_current(IsleGameObject*);
void isle_volcano_lifecycle_enter(IsleVolcanoLifecycleSource*);
void isle_volcano_lifecycle_checked(IsleVolcanoLifecycleSource*,IsleGameResult);
IsleGameResult isle_volcano_lifecycle_invoke(IsleVolcanoLifecycleSource*,void (*)(void*),void*);
IsleGameResult isle_volcano_lifecycle_start(IsleVolcanoLifecycleSource*,int32_t,int32_t*);
/* Actual independent C continuation entry; waiting entry never retains a
 * nonwaiting fault frame across scene suspension or an OM-domain callback. */
void isle_volcano_lifecycle_run(void*);
IsleGameResult isle_volcano_lifecycle_complete_scene(IsleVolcanoLifecycleSource*,IsleGameResult);
IsleGameResult isle_volcano_lifecycle_abandon(IsleVolcanoLifecycleSource*,IsleGameResult);
IsleVolcanoBlock* isle_volcano_lifecycle_block_next(IsleVolcanoLifecycleSource* s, IsleVolcanoBlock* block);
IsleGameVec3 isle_volcano_lifecycle_block_position(IsleVolcanoLifecycleSource* s, IsleVolcanoBlock* block);
void isle_volcano_lifecycle_displace_items(IsleVolcanoLifecycleSource* s, float x, float y, float z);
void isle_volcano_lifecycle_world_add(IsleVolcanoLifecycleSource* s, IsleVolcanoBlock* a, IsleVolcanoBlock* b, const IsleVolcanoDefinition* definitions, size_t count);
void isle_volcano_lifecycle_world_change(IsleVolcanoLifecycleSource* s, IsleVolcanoBlock* a, IsleVolcanoBlock* b, const IsleVolcanoDefinition* definitions, size_t count);
void isle_volcano_lifecycle_world_remove(IsleVolcanoLifecycleSource* s, IsleVolcanoBlock* a, const IsleVolcanoDefinition* definitions, size_t count);
void isle_volcano_lifecycle_height_map(IsleVolcanoLifecycleSource* s, uint32_t identity);
void isle_volcano_lifecycle_world_create(IsleVolcanoLifecycleSource* s, uint32_t identity, int32_t sky, int32_t first, int32_t last, int32_t link, int32_t draw, IsleVolcanoWorldAdd add, IsleVolcanoWorldRemove remove, IsleVolcanoWorldAdd change);
void isle_volcano_lifecycle_sky_follow(IsleVolcanoLifecycleSource* s);
void isle_volcano_lifecycle_actor_initialize_range(IsleVolcanoLifecycleSource* s, int32_t first, int32_t last, int32_t link, int32_t kind);
void isle_volcano_lifecycle_end_level(IsleVolcanoLifecycleSource* s, int32_t reason);
void isle_volcano_lifecycle_detector_disable(IsleVolcanoLifecycleSource* s);
void isle_volcano_lifecycle_environment_cleanup(IsleVolcanoLifecycleSource* s);
void isle_volcano_lifecycle_bgm_stop(IsleVolcanoLifecycleSource* s);
void isle_volcano_lifecycle_sound_stop_all(IsleVolcanoLifecycleSource* s);
void isle_volcano_lifecycle_world_destroy(IsleVolcanoLifecycleSource* s);
void isle_volcano_lifecycle_player_stop(IsleVolcanoLifecycleSource* s);
void isle_volcano_lifecycle_camera_remove(IsleVolcanoLifecycleSource* s);
void isle_volcano_lifecycle_scene_end(IsleVolcanoLifecycleSource* s);
const IsleVolcanoAsset* isle_volcano_lifecycle_load_asset(IsleVolcanoLifecycleSource* s, uint32_t start, uint32_t end);
void isle_volcano_lifecycle_fx_bank(IsleVolcanoLifecycleSource* s, int32_t bank, const IsleVolcanoAsset* script, const IsleVolcanoAsset* sprites);
IsleGameObject* isle_volcano_lifecycle_fx_camera(IsleVolcanoLifecycleSource* s, int32_t link, int32_t count, IsleGameCamera* camera);
IsleGameObject* isle_volcano_lifecycle_fx_allocate(IsleVolcanoLifecycleSource* s, int32_t count);
void isle_volcano_lifecycle_fx_dither(IsleVolcanoLifecycleSource* s, IsleVolcanoColorDither color, IsleVolcanoAlphaDither alpha);
void isle_volcano_lifecycle_fx_reference(IsleVolcanoLifecycleSource* s, uint32_t identity, IsleGameObject* object);
void* isle_volcano_lifecycle_depth_allocate(IsleVolcanoLifecycleSource* s, uint32_t bytes, uint32_t alignment);
void isle_volcano_lifecycle_apply_screen(IsleVolcanoLifecycleSource* s, const IsleVolcanoScreen* screen);
void isle_volcano_lifecycle_fx_reset_bank(IsleVolcanoLifecycleSource* s, size_t index);
void isle_volcano_lifecycle_engine_initialize(IsleVolcanoLifecycleSource* s);
IsleGameObject* isle_volcano_lifecycle_add_object(IsleVolcanoLifecycleSource* s, int32_t id, IsleGameSourceCallback callback, int32_t link, uint32_t priority);
void isle_volcano_lifecycle_actors_initialize(IsleVolcanoLifecycleSource* s);
void isle_volcano_lifecycle_background_color(IsleVolcanoLifecycleSource* s, uint8_t* r, uint8_t* g, uint8_t* b);
void isle_volcano_lifecycle_cameras_create(IsleVolcanoLifecycleSource* s, uint32_t rgba);
void isle_volcano_lifecycle_ui_initialize(IsleVolcanoLifecycleSource* s, const IsleVolcanoUi* callbacks);
void isle_volcano_lifecycle_end_callback(IsleVolcanoLifecycleSource* s, IsleVolcanoEndCallback callback);
void isle_volcano_lifecycle_pause_callback(IsleVolcanoLifecycleSource* s, IsleVolcanoPauseCallback callback);
void isle_volcano_lifecycle_environment_initialize(IsleVolcanoLifecycleSource* s, const IsleVolcanoEnvironment* sounds, size_t count);
void isle_volcano_lifecycle_detector_create(IsleVolcanoLifecycleSource* s);
void isle_volcano_lifecycle_detector_enable(IsleVolcanoLifecycleSource* s);
int32_t isle_volcano_lifecycle_illegal_copy(IsleVolcanoLifecycleSource* s);
void isle_volcano_lifecycle_player_flag(IsleVolcanoLifecycleSource* s, int32_t flag, int32_t value);
void isle_volcano_lifecycle_set_intervals(IsleVolcanoLifecycleSource* s, int32_t update, int32_t draw);
void isle_volcano_lifecycle_near_clipping(IsleVolcanoLifecycleSource* s, int32_t disabled);
void isle_volcano_lifecycle_setup_scene(IsleVolcanoLifecycleSource* s, const IsleVolcanoScene* scene);
int32_t isle_volcano_lifecycle_photo_count(IsleVolcanoLifecycleSource* s);
void isle_volcano_lifecycle_notice(IsleVolcanoLifecycleSource* s, uint32_t mask);
uint32_t isle_volcano_lifecycle_input_pressed(IsleVolcanoLifecycleSource* s);
void isle_volcano_lifecycle_cutscene_prepare(IsleVolcanoLifecycleSource* s);
void isle_volcano_lifecycle_cutscene_model(IsleVolcanoLifecycleSource* s, IsleGameObject* object);
void isle_volcano_lifecycle_cutscene_run(IsleVolcanoLifecycleSource* s);
void isle_volcano_lifecycle_cutscene_stop(IsleVolcanoLifecycleSource* s);
void isle_volcano_lifecycle_intro_restore(IsleVolcanoLifecycleSource* s);
IsleVolcanoEffect* isle_volcano_lifecycle_effect_create(IsleVolcanoLifecycleSource* s, int32_t bank, int32_t id);
void isle_volcano_lifecycle_effect_attach(IsleVolcanoLifecycleSource* s, IsleVolcanoEffect* effect, const IslePlayerCartNode* model);
float isle_volcano_lifecycle_camera_read(IsleVolcanoLifecycleSource* s, IsleGameCamera* camera, uint32_t component);
void isle_volcano_lifecycle_camera_store(IsleVolcanoLifecycleSource* s, IsleGameCamera* camera, uint32_t component, float value);
void isle_volcano_lifecycle_delete_intro(IsleVolcanoLifecycleSource* s);
void isle_volcano_lifecycle_animation_callback(IsleVolcanoLifecycleSource* s, IsleGameObject* object, IsleVolcanoIntroAnimationCallback callback);
void isle_volcano_lifecycle_scene_joined(IsleVolcanoLifecycleSource* s);
void isle_volcano_lifecycle_entry_joined(IsleVolcanoLifecycleSource* s);
IsleGameProcess* isle_volcano_lifecycle_create_process(IsleVolcanoLifecycleSource* s, IsleGameObject* object, IsleGameSourceCallback callback, int32_t kind, int32_t priority);
void isle_volcano_lifecycle_reference_assign(IsleVolcanoLifecycleSource* s, IsleGameObject** field, IsleGameObject* value);
int32_t isle_volcano_lifecycle_sound_identity(IsleVolcanoLifecycleSource* s, uint32_t handle);
void isle_volcano_lifecycle_sound_stop(IsleVolcanoLifecycleSource* s, uint32_t handle);
int32_t isle_volcano_lifecycle_sound_play_params(IsleVolcanoLifecycleSource* s, int32_t id, int32_t volume, int32_t pan, float pitch, int32_t priority);
IsleGameCamera* isle_volcano_lifecycle_main_camera(IsleVolcanoLifecycleSource* s);
IsleGameObject* isle_volcano_lifecycle_camera_object(IsleVolcanoLifecycleSource* s, IsleGameCamera* camera);
void isle_volcano_lifecycle_camera_speed(IsleVolcanoLifecycleSource* s, IsleGameCamera* camera, float speed);
void isle_volcano_lifecycle_camera_animation(IsleVolcanoLifecycleSource* s, IsleGameCamera* camera, uint32_t clip, float frame);
IsleGameProcess* isle_volcano_lifecycle_camera_process(IsleVolcanoLifecycleSource* s, IsleGameObject* object, int32_t kind, int32_t priority);
void isle_volcano_lifecycle_player_animation(IsleVolcanoLifecycleSource* s, uint32_t model, uint32_t material, float frame, float speed);
IsleGameObject* isle_volcano_lifecycle_player_model_init(IsleVolcanoLifecycleSource* s);
void isle_volcano_lifecycle_reset_camera(IsleVolcanoLifecycleSource* s);
void volcano_IntroStartCutscene(IsleGameObject* obj);
void volcano_IntroStopCutscene(IsleVolcanoLifecycleSource* l);
void volcano_IntroAnimationCallback(IsleVolcanoLifecycleSource* l, const IslePlayerCartNode* dobj, int32_t param, float value);
void volcano_StopIntro(IsleVolcanoLifecycleSource* l);
void volcano_PlayIntroSounds(IsleGameObject* obj);
void volcano_UpdateIntro(IsleGameObject* obj);
void volcano_StartIntro(IsleVolcanoLifecycleSource* l);
void volcano_PokemonAdd(IsleVolcanoLifecycleSource* l, IsleVolcanoBlock* arg0, IsleVolcanoBlock* arg1);
void volcano_PokemonChangeBlock(IsleVolcanoLifecycleSource* l, IsleVolcanoBlock* arg0, IsleVolcanoBlock* arg1);
void volcano_PokemonRemove(IsleVolcanoLifecycleSource* l, IsleVolcanoBlock* arg0);
void volcano_InitWorld(IsleVolcanoLifecycleSource* l);
void volcano_EndLevel(IsleVolcanoLifecycleSource* l, int32_t endLevelReason);
void volcano_func_802D64A0_7276A0(IsleGameObject* obj);
void volcano_Pause(IsleVolcanoLifecycleSource* l, int32_t arg0);
void volcano_ExitBlock(IsleVolcanoLifecycleSource* l, IsleVolcanoBlock* arg0);
void volcano_LoadEffects(IsleVolcanoLifecycleSource* l);
void volcano_Init(IsleVolcanoLifecycleSource* l);
void volcano_func_802D6780_727980(IsleVolcanoLifecycleSource* l, int32_t arg0);
void volcano_CheckIllegalCopy(IsleVolcanoLifecycleSource* l);
int32_t volcano_Start(IsleVolcanoLifecycleSource* l, int32_t arg0);
void volcano_stub(IsleVolcanoLifecycleSource* l);
IsleGameResult isle_volcano_lifecycle_invoke_intro_animation(IsleVolcanoLifecycleSource*,const IslePlayerCartNode*,int32_t,float);
IsleGameResult isle_volcano_lifecycle_invoke_init(IsleVolcanoLifecycleSource*);
IsleGameResult isle_volcano_lifecycle_invoke_end(IsleVolcanoLifecycleSource*,int32_t);
IsleGameResult isle_volcano_lifecycle_invoke_exit_block(IsleVolcanoLifecycleSource*,IsleVolcanoBlock*);
IsleGameResult isle_volcano_lifecycle_invoke_world_add(IsleVolcanoLifecycleSource*,IsleVolcanoBlock*,IsleVolcanoBlock*);
IsleGameResult isle_volcano_lifecycle_invoke_world_change(IsleVolcanoLifecycleSource*,IsleVolcanoBlock*,IsleVolcanoBlock*);
IsleGameResult isle_volcano_lifecycle_invoke_world_remove(IsleVolcanoLifecycleSource*,IsleVolcanoBlock*);
IsleGameResult isle_volcano_lifecycle_invoke_pause(IsleVolcanoLifecycleSource*,int32_t);

#ifdef __cplusplus
}
#endif
#endif
