#ifndef ISLE_BEACH_LIFECYCLE_SOURCE_H
#define ISLE_BEACH_LIFECYCLE_SOURCE_H
#include "actor_source.h"
#include "beach_music_source.h"
#include "player_cart_source.h"
#include "player_root_source.h"
#include "35mmisle/continuation.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct IsleBeachLifecycleSource IsleBeachLifecycleSource;
typedef struct IsleBeachLifecycleAsset IsleBeachLifecycleAsset;
typedef struct IsleBeachLifecycleEffect IsleBeachLifecycleEffect;
/* Native World identity, never a guest WorldBlock layout. */
typedef struct IsleBeachLifecycleBlock IsleBeachLifecycleBlock;
typedef struct IsleBeachLifecycleEnvironment {
    uint16_t sound;
    uint8_t pitch_modifier, hearing_range;
} IsleBeachLifecycleEnvironment;
typedef struct IsleBeachLifecyclePokemonDef {
    uint32_t id, initialize_identity, update_identity, remove_identity;
} IsleBeachLifecyclePokemonDef;
/* Original helper writes only these fields. The standalone CBFF4 ground factory
 * copies behavior/path, but its complete129 root/path/cleanup closure never
 * consumes either value. Native transport omits those dead, indeterminate
 * copies; no guessed behavior/path is supplied or claimed as ROM data. */
typedef struct IsleBeachLifecycleSpawn {
    uint32_t id;
    IsleGameVec3 translation, euler, scale;
} IsleBeachLifecycleSpawn;
typedef struct IsleBeachLifecycleRandom {
    uint8_t weight;
    void (*func)(IsleBeachLifecycleSource*, IsleGameObject*);
} IsleBeachLifecycleRandom;
/* Semantic decoded own-GAME setup, not a binary-compatible BufferSetup,
 * SceneSetup or ScreenSettings. Guest callback words are identities ONLY.
 * Native capacity policy must consume these actual Beach facts, not menu sizes. */
typedef struct IsleBeachLifecycleSceneSetup {
    uint16_t unknown_00;
    uint32_t update_identity, draw_identity, heap_base, heap_size;
    uint32_t unknown_14;
    int32_t contexts;
    uint32_t display_list_bytes[4], graphics_heap_bytes;
    uint16_t unknown_30;
    int32_t rdp_output_bytes;
    uint32_t prerender_identity, input_identity;
    uint32_t threads, thread_stack_bytes, stacks;
    int32_t unknown_4c;
    uint32_t processes, objects, object_bytes, matrices;
    uint32_t matrix_callback_identity, dynamic_store_identity;
    uint32_t animations, materials, models, model_bytes;
    uint32_t sprites, sprite_bytes, cameras, camera_bytes, initialize_identity;
} IsleBeachLifecycleSceneSetup;
typedef struct IsleBeachLifecycleScreen {
    uint32_t framebuffer_identity[3], initial_depth_identity;
    uint32_t width, height, flags;
    void* depth_buffer;
} IsleBeachLifecycleScreen;
typedef enum IsleBeachLifecycleAssetId {
    ISLE_BEACH_LIFECYCLE_FX_SCRIPT_COMMON,
    ISLE_BEACH_LIFECYCLE_FX_SPRITES_COMMON,
    ISLE_BEACH_LIFECYCLE_FX_SCRIPT_BEACH,
    ISLE_BEACH_LIFECYCLE_FX_SPRITES_BEACH,
    ISLE_BEACH_LIFECYCLE_IDLE_SCRIPT
} IsleBeachLifecycleAssetId;
typedef struct IsleBeachLifecycleUiCallbacks {
    void* user;
    IsleGameResult (*exit_block)(void*, IsleBeachLifecycleBlock*);
    IsleGameResult (*movement)(void*, int32_t, float);
    IsleGameSourceCallback update_items;
    uint8_t update_items_kind;
    IsleGameResult (*collide_item)(void*, IsleGameObject*, const IsleActorGround*);
} IsleBeachLifecycleUiCallbacks;
typedef IsleGameResult (*IsleBeachLifecycleWorldPair)(void*, IsleBeachLifecycleBlock*, IsleBeachLifecycleBlock*);
typedef IsleGameResult (*IsleBeachLifecycleWorldSingle)(void*, IsleBeachLifecycleBlock*);

/* One application-owned record per independently reserved Beach scene. It and
 * its continuation domain MUST outlive sceneStorage, World/Root, native OM
 * drain, then the post-omSetupScene route. No global callback-context cache.
 * player/music are nonowning active-scene links, detached by complete_scene.
 * .data is hydrated once from an admitted Snapshot's canonical US ROM. All
 * below original BSS (including unused bytes) starts zero, not process-global.
 * No model/camera pose, animation clock or intro timer is mirrored here. */
struct IsleBeachLifecycleSource {
    void* owner;
    IsleContinuationDomain* domain;
    IslePlayerSource* player;
    IsleBeachMusicSource* music;
    IsleGameResult status;
    void* nonwaiting_call_frame;
    int32_t input_scene, output_scene;
    uint8_t running, completed, scene_requested, scene_completion_ready;
    IsleGameResult scene_completion_status;
    IsleBeachLifecycleSceneSetup scene_setup;
    IsleBeachLifecycleScreen screen;
    IsleBeachLifecycleEnvironment environment[53];
    IsleBeachLifecyclePokemonDef magikarp;
    IsleBeachLifecycleRandom D_beach_802CC004[2];
    uint16_t D_beach_802CC014;
    int32_t D_beach_802CC018, D_beach_802CC0E0;
    /* Register this exact field with the existing semantic-reference registry
     * before init. native_store_cart performs the actual retained store; never
     * substitute cartSource.cartGObj or drop the credit on logical deletion. */
    IsleGameObject* D_beach_802CC0E4;
    uint8_t D_beach_80347580[8], D_beach_80347588, D_beach_80347589;
    uint16_t D_beach_8034758A;
    uint32_t D_beach_8034758C;
    uint8_t D_beach_80347590[0xC0];
};

/* canonical_rom is already identity-admitted by rom::Snapshot (US SHA1
 * edc7c49cc568c045fe48be0d18011c30f393cbaf), never dirty generated C/data.
 * prepare requires a zero record and does not allocate/publish native objects. */
IsleGameResult isle_beach_lifecycle_prepare(IsleBeachLifecycleSource*, void* owner,
    IsleContinuationDomain*, const uint8_t* canonical_rom, size_t bytes, int32_t scene);
/* Actual independent compiled-C continuation entry. The host consumes request
 * ONLY after resume returns, creates/drives/retires scene owners outside every
 * callback, then calls complete_scene ONLY after native/source/graphics/photo/
 * scene-local audio-facing borrowers have drained. Application Audio remains live.
 * A new cold native Frame is required for this independently reserved scene. */
void isle_beach_lifecycle_run(void*);
IsleGameResult isle_beach_lifecycle_complete_scene(IsleBeachLifecycleSource*, IsleGameResult);
/* Returning local C fences for host init and native callback adapters. Positive
 * waits are forbidden here. Intro waits execute only in actual C OM processes. */
IsleGameResult isle_beach_lifecycle_invoke_init(IsleBeachLifecycleSource*);
IsleGameResult isle_beach_lifecycle_invoke_end(IsleBeachLifecycleSource*, int32_t);
IsleGameResult isle_beach_lifecycle_world_add(void*, IsleBeachLifecycleBlock*, IsleBeachLifecycleBlock*);
IsleGameResult isle_beach_lifecycle_world_change(void*, IsleBeachLifecycleBlock*, IsleBeachLifecycleBlock*);
IsleGameResult isle_beach_lifecycle_world_remove(void*, IsleBeachLifecycleBlock*);
IsleGameResult isle_beach_lifecycle_exit_block(void*, IsleBeachLifecycleBlock*);
IsleGameResult isle_beach_lifecycle_movement(void*, int32_t, float);
IsleGameResult isle_beach_lifecycle_collide_item(void*, IsleGameObject*, const IsleActorGround*);
IsleGameResult isle_beach_lifecycle_end_event(void*, int32_t);
IsleGameResult isle_beach_lifecycle_pause_event(void*, int32_t);
void isle_beach_lifecycle_cart_event(void*, const IslePlayerCartNode*, int32_t, float);
void isle_beach_lifecycle_item_collision(IsleGameObject*, IsleActorGround*);
void isle_beach_lifecycle_delete_intro_cart(IsleBeachLifecycleSource*);
void isle_beach_lifecycle_camera_animation_update(IsleGameObject*);

/* Complete private Source definitions: fourteen setup/end/world/item functions
 * from55C110 and seven intro functions from55D1C0. No music, sign1004,
 * ending1001 factory or seventeen-actor runtime closure is claimed here. */
void isle_beach_lifecycle_beachSpawnMagikarpAtGObj(IsleBeachLifecycleSource*, IsleGameObject*);
void isle_beach_lifecycle_func_802C416C(IsleBeachLifecycleSource*, IsleGameObject*, const IsleActorGround*);
void isle_beach_lifecycle_beachPokemonAdd(IsleBeachLifecycleSource*, IsleBeachLifecycleBlock*, IsleBeachLifecycleBlock*);
void isle_beach_lifecycle_func_802C42F8(IsleBeachLifecycleSource*, IsleBeachLifecycleBlock*, IsleBeachLifecycleBlock*);
void isle_beach_lifecycle_func_802C431C(IsleBeachLifecycleSource*, IsleBeachLifecycleBlock*);
void isle_beach_lifecycle_func_802C4340(IsleBeachLifecycleSource*);
void isle_beach_lifecycle_func_802C43CC(IsleBeachLifecycleSource*, int32_t);
void isle_beach_lifecycle_func_802C4430(IsleGameObject*);
void isle_beach_lifecycle_func_802C4438(IsleBeachLifecycleSource*, int32_t);
void isle_beach_lifecycle_func_802C4440(IsleBeachLifecycleSource*, IsleBeachLifecycleBlock*);
void isle_beach_lifecycle_func_802C44D4(IsleBeachLifecycleSource*);
void isle_beach_lifecycle_func_802C45C0(IsleBeachLifecycleSource*);
void isle_beach_lifecycle_func_802C4738(IsleBeachLifecycleSource*, int32_t);
int32_t isle_beach_lifecycle_func_802C4740(IsleBeachLifecycleSource*, int32_t);
void isle_beach_lifecycle_func_802C5150(IsleBeachLifecycleSource*, IsleGameObject*);
void isle_beach_lifecycle_func_802C5180(IsleBeachLifecycleSource*);
void isle_beach_lifecycle_func_802C51A0(void*, const IslePlayerCartNode*, int32_t, float);
void isle_beach_lifecycle_func_802C5214(IsleBeachLifecycleSource*);
void isle_beach_lifecycle_func_802C527C(IsleGameObject*);
void isle_beach_lifecycle_func_802C52EC(IsleGameObject*);
void isle_beach_lifecycle_func_802C55CC(IsleBeachLifecycleSource*);

/* Returning native leaves never invoke unfenced Source, wait, checked-transfer,
 * physically destroy a C++ owner or manufacture success. The explicit callback
 * routes above are returning, nonwaiting local C fences; World/Movement may
 * invoke those routes and inspect their result without a nonlocal transfer
 * crossing a C++ traversal frame. No other Source callback is authorized here. */
IsleGameResult isle_beach_lifecycle_native_context(IsleGameObject*, IsleBeachLifecycleSource**);
IsleGameResult isle_beach_lifecycle_native_intervals(IsleBeachLifecycleSource*, uint16_t, uint16_t);
IsleGameResult isle_beach_lifecycle_native_near_clipping(IsleBeachLifecycleSource*, int32_t);
IsleGameResult isle_beach_lifecycle_native_request_scene(IsleBeachLifecycleSource*, const IsleBeachLifecycleSceneSetup*);
/* Exact800A19D8: write application's sole D_800AEBC0, then all three transition
 * colors to255 iff code==2, otherwise0. Not a fade or a scene-exit boolean. */
IsleGameResult isle_beach_lifecycle_native_transition_end(IsleBeachLifecycleSource*, int32_t);
/* Returning native owner leaf. The C wrapper above performs any synchronous
 * cross-domain owner rendezvous before entering C++; never a Source tick. */
IsleGameResult isle_beach_lifecycle_owner_transition_end(IsleBeachLifecycleSource*, int32_t);
IsleGameResult isle_beach_lifecycle_native_detector_disable(IsleBeachLifecycleSource*);
IsleGameResult isle_beach_lifecycle_native_environment_cleanup(IsleBeachLifecycleSource*);
/* End ordering: destroy_world requests original logical World removal and real
 * native object deletion, but retains C++ World during active traversal.
 * remove_camera likewise removes its native attachment only after its current
 * borrows permit it. exit_scene is the original800067DC GTL-stop request.
 * Physical owners are destroyed by host only after resume/tick/callback return,
 * then native process join AND all independent consumer fences have closed. */
IsleGameResult isle_beach_lifecycle_native_destroy_world(IsleBeachLifecycleSource*);
IsleGameResult isle_beach_lifecycle_native_remove_main_camera(IsleBeachLifecycleSource*);
IsleGameResult isle_beach_lifecycle_native_exit_scene(IsleBeachLifecycleSource*);
IsleGameResult isle_beach_lifecycle_native_height_map(IsleBeachLifecycleSource*, uint32_t identity);
IsleGameResult isle_beach_lifecycle_native_create_world(IsleBeachLifecycleSource*, uint32_t setup_identity,
    int32_t, int32_t, int32_t, int32_t, int32_t, IsleBeachLifecycleWorldPair,
    IsleBeachLifecycleWorldSingle, IsleBeachLifecycleWorldPair, void* user);
IsleGameResult isle_beach_lifecycle_native_sky_follow(IsleBeachLifecycleSource*);
IsleGameResult isle_beach_lifecycle_native_pokemon_registry(IsleBeachLifecycleSource*, int32_t, int32_t, int32_t, int32_t);
/* The complete original table identity802CBEE4 and all17 definitions must be
 * resolved by the genuine actor Owner. No subset/unsupported factory fallback. */
IsleGameResult isle_beach_lifecycle_native_pokemon_add(IsleBeachLifecycleSource*, IsleBeachLifecycleBlock*, IsleBeachLifecycleBlock*, uint32_t table);
IsleGameResult isle_beach_lifecycle_native_pokemon_change(IsleBeachLifecycleSource*, IsleBeachLifecycleBlock*, IsleBeachLifecycleBlock*, uint32_t table);
IsleGameResult isle_beach_lifecycle_native_pokemon_remove(IsleBeachLifecycleSource*, IsleBeachLifecycleBlock*, uint32_t table);
IsleGameResult isle_beach_lifecycle_native_current_block(IsleBeachLifecycleSource*, IsleBeachLifecycleBlock**);
IsleGameResult isle_beach_lifecycle_native_add_one(IsleBeachLifecycleSource*, IsleBeachLifecycleBlock*, IsleBeachLifecycleBlock*, const IsleBeachLifecycleSpawn*, const IsleBeachLifecyclePokemonDef*, IsleGameObject**);
IsleGameResult isle_beach_lifecycle_native_model(IsleBeachLifecycleSource*, IsleGameObject*, IsleGameModel**);
IsleGameResult isle_beach_lifecycle_native_model_position(IsleBeachLifecycleSource*, IsleGameModel*, int32_t transformed, IsleGameVec3*);
IsleGameResult isle_beach_lifecycle_native_set_position_component(IsleBeachLifecycleSource*, IsleGameModel*, uint32_t axis, float);
IsleGameResult isle_beach_lifecycle_native_block_next(IsleBeachLifecycleSource*, IsleBeachLifecycleBlock*, IsleBeachLifecycleBlock**);
IsleGameResult isle_beach_lifecycle_native_block_position(IsleBeachLifecycleSource*, IsleBeachLifecycleBlock*, IsleGameVec3*);
IsleGameResult isle_beach_lifecycle_native_displace_items(IsleBeachLifecycleSource*, float, float, float);
IsleGameResult isle_beach_lifecycle_native_depth_buffer(IsleBeachLifecycleSource*, uint32_t bytes, uint32_t alignment, void**);
IsleGameResult isle_beach_lifecycle_native_screen(IsleBeachLifecycleSource*, const IsleBeachLifecycleScreen*);
IsleGameResult isle_beach_lifecycle_native_fx_bank_count(IsleBeachLifecycleSource*, size_t*);
IsleGameResult isle_beach_lifecycle_native_fx_reset_bank(IsleBeachLifecycleSource*, size_t);
/* Exact8009CE00 resets total gPhotoCount, AE27C/AE280, every BDF20 slot to-1,
 * and every B0598.levelID/unk_00_24 to-1/0 in the application's one60-photo roll. */
IsleGameResult isle_beach_lifecycle_native_reset_photo_roll(IsleBeachLifecycleSource*);
IsleGameResult isle_beach_lifecycle_native_pokemons_init(IsleBeachLifecycleSource*);
/* The real Beach helper0 is NOT one of Root's three Player/UI objects. */
IsleGameResult isle_beach_lifecycle_native_create_helper(IsleBeachLifecycleSource*);
IsleGameResult isle_beach_lifecycle_native_background(IsleBeachLifecycleSource*, uint8_t*, uint8_t*, uint8_t*);
IsleGameResult isle_beach_lifecycle_native_create_cameras(IsleBeachLifecycleSource*, uint32_t color);
IsleGameResult isle_beach_lifecycle_native_idle_enabled(IsleBeachLifecycleSource*, int32_t*);
IsleGameResult isle_beach_lifecycle_native_load(IsleBeachLifecycleSource*, IsleBeachLifecycleAssetId, uint32_t first, uint32_t end, const IsleBeachLifecycleAsset**);
IsleGameResult isle_beach_lifecycle_native_idle_script(IsleBeachLifecycleSource*, const IsleBeachLifecycleAsset*);
/* Bind actual retained Root configuration BEFORE original C initUI; never call
 * original C from this leaf. end/pause callbacks are installed by original C. */
IsleGameResult isle_beach_lifecycle_native_ui_bind(IsleBeachLifecycleSource*, const IsleBeachLifecycleUiCallbacks*);
/* Account for the actual original initUI body after it returns; never replay it. */
IsleGameResult isle_beach_lifecycle_native_ui_initialized(IsleBeachLifecycleSource*, IsleGameObject*);
IsleGameResult isle_beach_lifecycle_native_environment_init(IsleBeachLifecycleSource*, const IsleBeachLifecycleEnvironment*, size_t);
IsleGameResult isle_beach_lifecycle_native_fx_setup_bank(IsleBeachLifecycleSource*, int32_t, const IsleBeachLifecycleAsset*, const IsleBeachLifecycleAsset*);
/* Both returned FX GObj identities are the real Root Source fields523120/124;
 * register their semantic credits, retain actual FX resources/processes, and
 * honor the original hide/freeze masks. No token object or unowned mirror. */
IsleGameResult isle_beach_lifecycle_native_fx_camera(IsleBeachLifecycleSource*, int32_t, int32_t, IsleGameCamera*, IsleGameObject**);
IsleGameResult isle_beach_lifecycle_native_fx_effects(IsleBeachLifecycleSource*, int32_t, IsleGameObject**);
IsleGameResult isle_beach_lifecycle_native_fx_dither(IsleBeachLifecycleSource*, uint32_t, uint32_t);
IsleGameResult isle_beach_lifecycle_native_detector_create(IsleBeachLifecycleSource*);
IsleGameResult isle_beach_lifecycle_native_detector_enable(IsleBeachLifecycleSource*);
IsleGameResult isle_beach_lifecycle_native_photo_count(IsleBeachLifecycleSource*, int32_t*);
IsleGameResult isle_beach_lifecycle_native_or_notice(IsleBeachLifecycleSource*, uint32_t);
/* Resolve EXACT clip identities through retained CartResources/cart::Owner and
 * camera_animation::NativeDriver. Opaque typed owner-issued handles only. */
IsleGameResult isle_beach_lifecycle_native_camera_clip(IsleBeachLifecycleSource*, uint32_t identity, const IslePlayerCameraAnimationClip**);
IsleGameResult isle_beach_lifecycle_native_cart_model_clip(IsleBeachLifecycleSource*, uint32_t identity, const IslePlayerCartModelAnimation**);
IsleGameResult isle_beach_lifecycle_native_cart_material_clip(IsleBeachLifecycleSource*, uint32_t identity, const IslePlayerCartMaterialAnimation**);
IsleGameResult isle_beach_lifecycle_native_camera_speed(IsleBeachLifecycleSource*, IsleGameCamera*, float);
IsleGameResult isle_beach_lifecycle_native_sound_identity(IsleBeachLifecycleSource*, uint32_t handle, int32_t*);
IsleGameResult isle_beach_lifecycle_native_create_effect(IsleBeachLifecycleSource*, int32_t, int32_t, IsleBeachLifecycleEffect**);
/* Retain the real model-incarnation/node credit for the effect's lifetime.
 * The event-batch node pointer itself expires at Cart finish_events; neither a
 * copied pose nor that borrowed pointer is a persistent FX attachment. */
IsleGameResult isle_beach_lifecycle_native_attach_effect(IsleBeachLifecycleSource*, IsleBeachLifecycleEffect*, const IslePlayerCartNode*);
IsleGameResult isle_beach_lifecycle_native_store_cart(IsleBeachLifecycleSource*, IsleGameObject*);
IsleGameResult isle_beach_lifecycle_native_retire_intro_cart(IsleBeachLifecycleSource*, IsleGameObject*);
/* Host-only returning retirement leaves, outside traversal/Source callbacks:
 * after native Cart processes join AND every real FX node attachment closes,
 * release_cart_reference performs registry store-NULL then unregisters the
 * exact lifecycle field. release_fx_references similarly closes both original
 * Root FX fields while Root Source storage is still alive. Neither destroys
 * an owner. These credits must close before collect/join and complete_scene. */
IsleGameResult isle_beach_lifecycle_native_release_cart_reference(IsleBeachLifecycleSource*);
IsleGameResult isle_beach_lifecycle_native_release_fx_references(IsleBeachLifecycleSource*);

/* Plain-C checked adapters used by privately materialized bodies. */
void isle_beach_lifecycle_checked(IsleBeachLifecycleSource*, IsleGameResult);
IsleBeachLifecycleSource* isle_beach_lifecycle_current(IsleGameObject*);
void isle_beach_lifecycle_setup_scene(IsleBeachLifecycleSource*, const IsleBeachLifecycleSceneSetup*);
IsleBeachLifecycleBlock* isle_beach_lifecycle_current_block(IsleBeachLifecycleSource*);
IsleGameObject* isle_beach_lifecycle_add_one(IsleBeachLifecycleSource*, IsleBeachLifecycleBlock*, IsleBeachLifecycleBlock*, const IsleBeachLifecycleSpawn*, const IsleBeachLifecyclePokemonDef*);
IsleGameModel* isle_beach_lifecycle_model(IsleBeachLifecycleSource*, IsleGameObject*);
IsleGameVec3 isle_beach_lifecycle_model_position(IsleBeachLifecycleSource*, IsleGameModel*, int32_t transformed);
float isle_beach_lifecycle_distance_from_player(IsleBeachLifecycleSource*, IsleGameModel*);
void isle_beach_lifecycle_set_position_component(IsleBeachLifecycleSource*, IsleGameModel*, uint32_t, float);
IsleBeachLifecycleBlock* isle_beach_lifecycle_block_next(IsleBeachLifecycleSource*, IsleBeachLifecycleBlock*);
IsleGameVec3 isle_beach_lifecycle_block_position(IsleBeachLifecycleSource*, IsleBeachLifecycleBlock*);
void* isle_beach_lifecycle_depth_buffer(IsleBeachLifecycleSource*, uint32_t, uint32_t);
size_t isle_beach_lifecycle_fx_bank_count(IsleBeachLifecycleSource*);
int32_t isle_beach_lifecycle_idle_enabled(IsleBeachLifecycleSource*);
const IsleBeachLifecycleAsset* isle_beach_lifecycle_load(IsleBeachLifecycleSource*, IsleBeachLifecycleAssetId);
void isle_beach_lifecycle_init_ui(IsleBeachLifecycleSource*);
IsleGameObject* isle_beach_lifecycle_fx_camera(IsleBeachLifecycleSource*, int32_t, int32_t, IsleGameCamera*);
IsleGameObject* isle_beach_lifecycle_fx_effects(IsleBeachLifecycleSource*, int32_t);
int32_t isle_beach_lifecycle_photo_count(IsleBeachLifecycleSource*);
int32_t isle_beach_lifecycle_sound_identity(IsleBeachLifecycleSource*, uint32_t);
IsleBeachLifecycleEffect* isle_beach_lifecycle_create_effect(IsleBeachLifecycleSource*, int32_t, int32_t);
const IslePlayerCameraAnimationClip* isle_beach_lifecycle_camera_clip(IsleBeachLifecycleSource*, uint32_t);
const IslePlayerCartModelAnimation* isle_beach_lifecycle_cart_model_clip(IsleBeachLifecycleSource*, uint32_t);
const IslePlayerCartMaterialAnimation* isle_beach_lifecycle_cart_material_clip(IsleBeachLifecycleSource*, uint32_t);
IsleGameCamera* isle_beach_lifecycle_camera(IsleBeachLifecycleSource*);
IsleGameObject* isle_beach_lifecycle_camera_object(IsleBeachLifecycleSource*);
#ifdef __cplusplus
}
#endif
#endif
