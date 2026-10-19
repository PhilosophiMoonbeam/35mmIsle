#ifndef ISLE_VALLEY_LIFECYCLE_SOURCE_H
#define ISLE_VALLEY_LIFECYCLE_SOURCE_H
#include "valley_actor_source.h"
#include "35mmisle/continuation.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct IsleValleyEnvironment { uint16_t sound; uint8_t pitch_modifier, hearing_range; } IsleValleyEnvironment;
typedef struct IsleValleyLifecycleScreen { uint32_t framebuffer_identity[3], initial_depth_identity, width, height, flags; void* depth_buffer; } IsleValleyLifecycleScreen;
typedef struct IsleValleyLifecycleSceneSetup {
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
} IsleValleyLifecycleSceneSetup;
typedef struct IsleValleyLifecycleServices {
    IsleGameResult (*intervals)(IsleValleyLifecycleSource*, uint16_t, uint16_t);
    IsleGameResult (*near_clipping)(IsleValleyLifecycleSource*, int32_t);
    IsleGameResult (*photo_count)(IsleValleyLifecycleSource*, int32_t*);
    IsleGameResult (*height_map)(IsleValleyLifecycleSource*, uint32_t, int32_t*);
    IsleGameResult (*world_create)(IsleValleyLifecycleSource*, uint32_t, int32_t, int32_t, int32_t, int32_t, int32_t, IsleValleyWorldPair, IsleValleyWorldSingle, IsleValleyWorldPair, int32_t*);
    IsleGameResult (*sky_follow)(IsleValleyLifecycleSource*);
    IsleGameResult (*pokemon_registry)(IsleValleyLifecycleSource*, int32_t, int32_t, int32_t, int32_t);
    IsleGameResult (*pokemon_add)(IsleValleyLifecycleSource*, IsleValleyWorldBlock*, IsleValleyWorldBlock*, uint32_t);
    IsleGameResult (*pokemon_change)(IsleValleyLifecycleSource*, IsleValleyWorldBlock*, IsleValleyWorldBlock*, uint32_t);
    IsleGameResult (*pokemon_remove)(IsleValleyLifecycleSource*, IsleValleyWorldBlock*, uint32_t);
    IsleGameResult (*items_displace)(IsleValleyLifecycleSource*, float, float, float);
    IsleGameResult (*transition_end)(IsleValleyLifecycleSource*, int32_t);
    IsleGameResult (*detector_disable)(IsleValleyLifecycleSource*);
    IsleGameResult (*environment_cleanup)(IsleValleyLifecycleSource*);
    IsleGameResult (*world_destroy)(IsleValleyLifecycleSource*);
    IsleGameResult (*world_cleanup)(IsleValleyLifecycleSource*);
    IsleGameResult (*camera_remove)(IsleValleyLifecycleSource*);
    IsleGameResult (*scene_exit)(IsleValleyLifecycleSource*);
    IsleGameResult (*depth_allocate)(IsleValleyLifecycleSource*, size_t, size_t, void**);
    IsleGameResult (*screen_apply)(IsleValleyLifecycleSource*, const IsleValleyLifecycleScreen*);
    IsleGameResult (*effect_banks_reset)(IsleValleyLifecycleSource*);
    IsleGameResult (*game_state_reset)(IsleValleyLifecycleSource*);
    IsleGameResult (*pokemons_init)(IsleValleyLifecycleSource*);
    IsleGameResult (*background_color)(IsleValleyLifecycleSource*, uint8_t*, uint8_t*, uint8_t*);
    IsleGameResult (*cameras_create)(IsleValleyLifecycleSource*, int32_t, IsleGameObject**);
    IsleGameResult (*ui_init)(IsleValleyLifecycleSource*, IsleValleyWorldSingle, IsleValleyMovementCallback, IsleGameSourceCallback, uint8_t, IsleValleyItemCollisionCallback, IsleGameObject**);
    IsleGameResult (*end_callback)(IsleValleyLifecycleSource*, IsleValleyEndCallback);
    IsleGameResult (*pause_callback)(IsleValleyLifecycleSource*, IsleValleyEndCallback);
    IsleGameResult (*environment_init)(IsleValleyLifecycleSource*, const IsleValleyEnvironment*, size_t);
    IsleGameResult (*asset_load)(IsleValleyLifecycleSource*, uint32_t, uint32_t, void**);
    IsleGameResult (*effect_bank)(IsleValleyLifecycleSource*, int32_t, void*, void*);
    IsleGameResult (*effect_camera)(IsleValleyLifecycleSource*, int32_t, int32_t, IsleValleyCamera*, IsleGameObject**);
    IsleGameResult (*effects_allocate)(IsleValleyLifecycleSource*, int32_t, void**);
    IsleGameResult (*effects_dither)(IsleValleyLifecycleSource*, uint32_t, uint32_t);
    IsleGameResult (*detector_create)(IsleValleyLifecycleSource*);
    IsleGameResult (*detector_enable)(IsleValleyLifecycleSource*);
    IsleGameResult (*intro_initialize)(IsleValleyLifecycleSource*);
    IsleGameResult (*intro_object)(IsleValleyLifecycleSource*, IsleGameObject*);
    IsleGameResult (*intro_finish)(IsleValleyLifecycleSource*);
    IsleGameResult (*intro_prepare)(IsleValleyLifecycleSource*);
    IsleGameResult (*intro_stop)(IsleValleyLifecycleSource*);
    IsleGameResult (*animation_callback)(IsleValleyLifecycleSource*, IsleGameObject*, IsleValleyLifecycleAnimationCallback);
    IsleGameResult (*effect_camera_publish)(IsleValleyLifecycleSource*, IsleGameObject*);
    IsleGameResult (*effects_publish)(IsleValleyLifecycleSource*, void*);
} IsleValleyLifecycleServices;

/* Fixed-address independent entry record lives beside, never inside disposable
 * native Storage. actors borrows the ONE canonical mutable course record.
 * SceneSetup.heap_size is also Source D_802D2A00_7ABF90; there is no mirror.
 * Native hooks after detachment read only the independent owner's real retained
 * join receipt, never Player/Game/Storage. STOPPED is not successful completion.
 * World/UI native callback routing MUST call the returning C invoke/world entry
 * APIs below, never call a raw Source callback under a C++ traversal.
 */
struct IsleValleyLifecycleSource {
    void* owner;
    IsleContinuationDomain* domain;
    IsleValleyActorSource* actors;
    const IsleValleyLifecycleServices* services;
    IsleGameResult status, scene_completion_status;
    void* nonwaiting_call_frame;
    uint64_t clock;
    int32_t input_scene, output_scene;
    uint8_t running, completed, scene_requested, scene_completion_ready;
    IsleValleyLifecycleSceneSetup scene_setup;
    IsleValleyLifecycleScreen screen;
    IsleValleyEnvironment environment[58];
};

IsleGameResult isle_valley_lifecycle_prepare(IsleValleyLifecycleSource*, void*, IsleContinuationDomain*, IsleValleyActorSource*, const IsleValleyLifecycleServices*, const uint8_t*, size_t, int32_t);
void isle_valley_lifecycle_run(void*);
IsleGameResult isle_valley_lifecycle_complete_scene(IsleValleyLifecycleSource*, IsleGameResult);
IsleGameResult isle_valley_lifecycle_invoke_init(IsleValleyLifecycleSource*);
IsleGameResult isle_valley_lifecycle_invoke_end(IsleValleyLifecycleSource*, int32_t);
IsleGameResult isle_valley_lifecycle_invoke_movement(IsleValleyLifecycleSource*, int32_t, float);
IsleGameResult isle_valley_lifecycle_invoke_animation(IsleValleyLifecycleSource*, const IsleActorNodeRef*, int32_t, float);
IsleGameResult isle_valley_lifecycle_native_context(IsleGame*, IsleGameObject*, IsleValleyLifecycleSource**);
IsleGameResult isle_valley_lifecycle_native_admission(const IsleValleyLifecycleSource*);
IsleGameResult isle_valley_lifecycle_native_request_scene(IsleValleyLifecycleSource*, const IsleValleyLifecycleSceneSetup*);
IsleGameResult isle_valley_lifecycle_native_scene_joined(IsleValleyLifecycleSource*);
IsleGameResult isle_valley_lifecycle_native_or_notice(IsleValleyLifecycleSource*, uint32_t);
IsleValleyLifecycleSource* isle_valley_lifecycle_current(IsleGameObject*);
void isle_valley_lifecycle_checked(IsleValleyLifecycleSource*, IsleGameResult);
void isle_valley_lifecycle_setup_scene(IsleValleyLifecycleSource*, const IsleValleyLifecycleSceneSetup*);

IsleGameResult isle_valley_lifecycle_invoke_pause(IsleValleyLifecycleSource*,int32_t);
IsleGameResult isle_valley_lifecycle_world_add(IsleValleyLifecycleSource*,IsleValleyWorldBlock*,IsleValleyWorldBlock*);
IsleGameResult isle_valley_lifecycle_world_change(IsleValleyLifecycleSource*,IsleValleyWorldBlock*,IsleValleyWorldBlock*);
IsleGameResult isle_valley_lifecycle_world_remove(IsleValleyLifecycleSource*,IsleValleyWorldBlock*);
IsleGameResult isle_valley_lifecycle_exit_block(IsleValleyLifecycleSource*,IsleValleyWorldBlock*);

void func_802C5C20_79F1B0(IsleValleyLifecycleSource* s, IsleValleyWorldBlock* arg0, IsleValleyWorldBlock* arg1);
void func_802C5C44_79F1D4(IsleValleyLifecycleSource* s, IsleValleyWorldBlock* arg0, IsleValleyWorldBlock* arg1);
void func_802C5C68_79F1F8(IsleValleyLifecycleSource* s, IsleValleyWorldBlock* arg0);
void func_802C5C8C_79F21C(IsleValleyLifecycleSource* s);
void func_802C5D18_79F2A8(IsleValleyLifecycleSource* s, int32_t arg0);
void func_802C5DCC_79F35C(IsleValleyLifecycleSource* s, int32_t arg0);
void func_802C5DD4_79F364(IsleValleyLifecycleSource* s, IsleValleyWorldBlock* arg0);
void func_802C6300_79F890(IsleValleyLifecycleSource* s);
void func_802C63EC_79F97C(IsleValleyLifecycleSource* s);
void func_802C653C_79FACC(IsleValleyLifecycleSource* s, int32_t arg0);
int32_t func_802C6544_79FAD4(IsleValleyLifecycleSource* s, int32_t arg0);
void func_802C65E0_79FB70(IsleGameObject* arg0);
void func_802C6610_79FBA0(IsleValleyLifecycleSource* s);
void func_802C6630_79FBC0(IsleValleyLifecycleSource* s, const IsleActorNodeRef* dobj, int32_t arg1, float arg2);
void valley_stopIntro(IsleValleyLifecycleSource* s);
void func_802C673C_79FCCC(IsleGameObject* arg0);
void func_802C67F4_79FD84(IsleGameObject* obj);
void func_802C6AD4_7A0064(IsleValleyLifecycleSource* s);
void func_802C6B30_7A00C0(IsleValleyLifecycleSource* s, int32_t arg0, float arg1);

void isle_valley_lifecycle_intervals(IsleValleyLifecycleSource* s, uint16_t update, uint16_t draw);
void isle_valley_lifecycle_near_clipping(IsleValleyLifecycleSource* s, int32_t disabled);
int32_t isle_valley_lifecycle_photo_count(IsleValleyLifecycleSource* s);
int32_t isle_valley_lifecycle_height_map(IsleValleyLifecycleSource* s, uint32_t identity);
int32_t isle_valley_lifecycle_world_create(IsleValleyLifecycleSource* s, uint32_t identity, int32_t sky, int32_t minimum, int32_t maximum, int32_t link, int32_t draw_link, IsleValleyWorldPair add, IsleValleyWorldSingle remove, IsleValleyWorldPair change);
void isle_valley_lifecycle_sky_follow(IsleValleyLifecycleSource* s);
void isle_valley_lifecycle_pokemon_registry(IsleValleyLifecycleSource* s, int32_t minimum, int32_t maximum, int32_t link, int32_t course);
void isle_valley_lifecycle_pokemon_add(IsleValleyLifecycleSource* s, IsleValleyWorldBlock* a, IsleValleyWorldBlock* b, uint32_t registry);
void isle_valley_lifecycle_pokemon_change(IsleValleyLifecycleSource* s, IsleValleyWorldBlock* a, IsleValleyWorldBlock* b, uint32_t registry);
void isle_valley_lifecycle_pokemon_remove(IsleValleyLifecycleSource* s, IsleValleyWorldBlock* block, uint32_t registry);
void isle_valley_lifecycle_items_displace(IsleValleyLifecycleSource* s, float x, float y, float z);
void isle_valley_lifecycle_transition_end(IsleValleyLifecycleSource* s, int32_t reason);
void isle_valley_lifecycle_detector_disable(IsleValleyLifecycleSource* s);
void isle_valley_lifecycle_environment_cleanup(IsleValleyLifecycleSource* s);
void isle_valley_lifecycle_world_destroy(IsleValleyLifecycleSource* s);
void isle_valley_lifecycle_world_cleanup(IsleValleyLifecycleSource* s);
void isle_valley_lifecycle_camera_remove(IsleValleyLifecycleSource* s);
void isle_valley_lifecycle_scene_exit(IsleValleyLifecycleSource* s);
void* isle_valley_lifecycle_depth_allocate(IsleValleyLifecycleSource* s, size_t bytes, size_t alignment);
void isle_valley_lifecycle_screen_apply(IsleValleyLifecycleSource* s, const IsleValleyLifecycleScreen* screen);
void isle_valley_lifecycle_effect_banks_reset(IsleValleyLifecycleSource* s);
void isle_valley_lifecycle_game_state_reset(IsleValleyLifecycleSource* s);
void isle_valley_lifecycle_pokemons_init(IsleValleyLifecycleSource* s);
void isle_valley_lifecycle_background_color(IsleValleyLifecycleSource* s, uint8_t* r, uint8_t* g, uint8_t* b);
IsleGameObject* isle_valley_lifecycle_cameras_create(IsleValleyLifecycleSource* s, int32_t color);
IsleGameObject* isle_valley_lifecycle_ui_init(IsleValleyLifecycleSource* s, IsleValleyWorldSingle exit, IsleValleyMovementCallback movement, IsleGameSourceCallback items, uint8_t kind, IsleValleyItemCollisionCallback collision);
void isle_valley_lifecycle_end_callback(IsleValleyLifecycleSource* s, IsleValleyEndCallback callback);
void isle_valley_lifecycle_pause_callback(IsleValleyLifecycleSource* s, IsleValleyEndCallback callback);
void isle_valley_lifecycle_environment_init(IsleValleyLifecycleSource* s, const IsleValleyEnvironment* rows, size_t count);
void* isle_valley_lifecycle_asset_load(IsleValleyLifecycleSource* s, uint32_t begin, uint32_t end);
void isle_valley_lifecycle_effect_bank(IsleValleyLifecycleSource* s, int32_t id, void* script, void* sprites);
IsleGameObject* isle_valley_lifecycle_effect_camera(IsleValleyLifecycleSource* s, int32_t id, int32_t count, IsleValleyCamera* camera);
void* isle_valley_lifecycle_effects_allocate(IsleValleyLifecycleSource* s, int32_t count);
void isle_valley_lifecycle_effects_dither(IsleValleyLifecycleSource* s, uint32_t color, uint32_t alpha);
void isle_valley_lifecycle_detector_create(IsleValleyLifecycleSource* s);
void isle_valley_lifecycle_detector_enable(IsleValleyLifecycleSource* s);
void isle_valley_lifecycle_intro_initialize(IsleValleyLifecycleSource* s);
void isle_valley_lifecycle_intro_object(IsleValleyLifecycleSource* s, IsleGameObject* object);
void isle_valley_lifecycle_intro_finish(IsleValleyLifecycleSource* s);
void isle_valley_lifecycle_intro_prepare(IsleValleyLifecycleSource* s);
void isle_valley_lifecycle_intro_stop(IsleValleyLifecycleSource* s);
void isle_valley_lifecycle_animation_callback(IsleValleyLifecycleSource* s, IsleGameObject* object, IsleValleyLifecycleAnimationCallback callback);
void isle_valley_lifecycle_effect_camera_publish(IsleValleyLifecycleSource* s, IsleGameObject* object);
void isle_valley_lifecycle_effects_publish(IsleValleyLifecycleSource* s, void* effects);
#ifdef __cplusplus
}
#endif
#endif
