#ifndef ISLE_RAINBOW_ACTOR_SOURCE_H
#define ISLE_RAINBOW_ACTOR_SOURCE_H
#include "actor_owner_source.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
struct IsleRainbowActorSource;
struct IsleRainbowNativeCollision;
typedef struct IsleRainbowActorSource IsleRainbowActorSource;
typedef struct IsleRainbowDefinition {
    uint32_t identity; /* Zero ONLY for source-automatic records. */
    uint32_t origin, actor_id, factory, change_block, remove, initialization;
} IsleRainbowDefinition;
/* Source helpers write transforms and id, never path or behavior. The latter
 * are copied by Pokemon_SpawnOnGround but never consumed by this closed Rainbow
 * callback graph. Native admission must preserve that distinction: no invented
 * path, behavior or automatic guest address; do not read uninitialized C data. */
typedef struct IsleRainbowAutomaticSpawn {
    int32_t id;
    IsleGameVec3 translation, euler, scale;
    uint32_t helper_identity, written_fields;
} IsleRainbowAutomaticSpawn;
enum { ISLE_RAINBOW_SPAWN_ID_TRANSFORMS = 0x3ffu };
typedef struct IsleRainbowCollision {
    uint32_t identity, tree_identity;
    int32_t actor_id;
    float scale;
    struct IsleRainbowNativeCollision* native;
} IsleRainbowCollision;
typedef void (*IsleRainbowAnimationCallback)(IsleRainbowActorSource*, const IsleGameModelRef*, int32_t, float);
typedef int32_t (*IsleRainbowCollisionCallback)(IsleRainbowActorSource*, IsleGameVec3*, IsleGameVec3*, IsleGameVec3*, IsleGameVec3*);
/* Every service is a real returning, nonwaiting native operation. All function
 * pointers are REQUIRED, retained and immutable. No service may wait, stop,
 * fault, throw or longjmp through C++; the C bridge checks its returned result.
 * Native owners must prove same-scene model/object/resource/incarnation and full
 * callback/child-factory closure before admitting any Source process. */
typedef struct IsleRainbowActorServices {
    IsleGameResult (*reference_assign)(IsleRainbowActorSource*, IsleGameObject**, IsleGameObject*);
    IsleGameResult (*current_block)(IsleRainbowActorSource*, void**);
    IsleGameResult (*add_one)(IsleRainbowActorSource*, void*, void*, const IsleRainbowAutomaticSpawn*, const IsleRainbowDefinition*, IsleGameObject**);
    IsleGameResult (*sound_play)(IsleRainbowActorSource*, int32_t, int32_t*);
    IsleGameResult (*sound_active)(IsleRainbowActorSource*, int32_t, int32_t*);
    IsleGameResult (*sound_stop)(IsleRainbowActorSource*, int32_t);
    IsleGameResult (*sound_stop_all)(IsleRainbowActorSource*);
    IsleGameResult (*bgm_play)(IsleRainbowActorSource*, int32_t, int32_t);
    IsleGameResult (*bgm_stop)(IsleRainbowActorSource*, int32_t);
    IsleGameResult (*bgm_volume)(IsleRainbowActorSource*, int32_t, uint32_t);
    IsleGameResult (*bgm_channel_volume)(IsleRainbowActorSource*, int32_t, uint8_t, int32_t);
    IsleGameResult (*remove_flying_items)(IsleRainbowActorSource*);
    IsleGameResult (*reset_camera)(IsleRainbowActorSource*);
    IsleGameResult (*reset_messages)(IsleRainbowActorSource*);
    IsleGameResult (*remove_pokemon)(IsleRainbowActorSource*, const int16_t*);
    IsleGameResult (*cutscene_start)(IsleRainbowActorSource*, IsleGameObject*, float);
    IsleGameResult (*resume_processes)(IsleRainbowActorSource*, IsleGameObject*);
    IsleGameResult (*main_camera)(IsleRainbowActorSource*, IsleGameObject**);
    IsleGameResult (*pause_processes)(IsleRainbowActorSource*, IsleGameObject*);
    IsleGameResult (*camera_speed)(IsleRainbowActorSource*, IsleGameObject*, float);
    IsleGameResult (*camera_animation)(IsleRainbowActorSource*, IsleGameObject*, uint32_t, float);
    IsleGameResult (*camera_animation_process)(IsleRainbowActorSource*, IsleGameObject*, int32_t, int32_t);
    IsleGameResult (*sky_speed)(IsleRainbowActorSource*, float);
    /* NULL output on successful init is the original real allocation-failure branch. */
    IsleGameResult (*player_model_init)(IsleRainbowActorSource*, IsleGameObject**);
    IsleGameResult (*player_model_animation)(IsleRainbowActorSource*, uint32_t, uint32_t, float, float);
    IsleGameResult (*animation_callback)(IsleRainbowActorSource*, IsleGameObject*, IsleRainbowAnimationCallback);
    IsleGameResult (*collision_initialize)(IsleRainbowActorSource*, IsleRainbowCollision*);
    IsleGameResult (*collision_query)(IsleRainbowActorSource*, IsleGameVec3*, IsleGameVec3*, IsleGameVec3*, IsleGameVec3*, const IsleRainbowCollision*, const IsleGameVec3*, const IsleGameVec3*, int32_t*);
    IsleGameResult (*collision_callback)(IsleRainbowActorSource*, IsleRainbowCollisionCallback);
} IsleRainbowActorServices;
typedef enum IsleRainbowActorPhase {
    ISLE_RAINBOW_ACTOR_UNPREPARED = 0,
    ISLE_RAINBOW_ACTOR_PREPARED,
    ISLE_RAINBOW_ACTOR_ACTIVE,
    ISLE_RAINBOW_ACTOR_DETACHED
} IsleRainbowActorPhase;
/* One fixed-address record beside the independent lifecycle entry, outside
 * disposable scene Storage. All five families and lifecycle borrow it. Native
 * bindings retire before the original post-omSetupScene entry return; Source
 * scalars and the independent owner survive detachment. Exact D_ names identify
 * Source storage, not SDK/console struct layout. Phases only advance once. */
struct IsleRainbowActorSource {
    IsleRainbowActorPhase phase;
    IsleGame* nativeGame;
    void* owner; /* Independent entry owner, never cleared by deactivation. */
    const IsleRainbowActorServices* services;
    void* nonwaiting_call_frame;
    IsleGameResult status;
    IsleGameObject* D_8034AB94_82A304;
    IsleGameObject* D_8034AB98_82A308;
    int32_t D_8034AB9C_82A30C, D_8034ABA0_82A310, D_8034ABA4_82A314;
    int32_t D_8034AE48_82A5B8, D_8034AE4C_82A5BC;
    float D_8034AE50_82A5C0, D_8034AE54_82A5C4, D_8034AE58_82A5C8;
    int32_t D_8034AE5C_82A5CC;
    /* Actual BSS declarations in mew/code.c159-167, not assumed data zeros. */
    uint8_t D_80350170_82F8E0[0x18];
    float D_80350188_82F8F8, D_8035018C_82F8FC;
    int32_t D_80350190_82F900, D_80350194_82F904, D_80350198_82F908;
    uint32_t D_8035019C_82F90C;
    uint8_t D_803501A0_82F910[0x40];
    IsleGameObject* D_8034AF2C_82A69C;
    IsleGameModelRef mew_model, shield_model, gate_model;
    int32_t D_8034AF30_82A6A0, D_8034AF34_82A6A4, D_8034AF38_82A6A8;
    int16_t D_8034AF5C_82A6CC[3];
    IsleRainbowCollision D_8034AF20_82A690;
    IsleGameVec3 D_8034B528_82AC98, D_8034B534_82ACA4;
    float D_800F5DB0_4A8180;
};
/* ROM is the admitted canonical 16MiB image. Prepare requires zero/unprepared
 * fixed-address storage and an independent owner, but no Game or services.
 * It validates/decodes initialized words and initializes actual BSS exactly
 * once. PREPARED does not authorize callbacks, factories or live publication. */
IsleGameResult isle_rainbow_actor_prepare(IsleRainbowActorSource*, void* independent_owner,
    const uint8_t* rom, size_t bytes);
/* Host-only, after genuine omSetupScene intent and native Storage/owners exist.
 * All 29 immutable real services and the same canonical native context must be
 * bound before ACTIVE is published. No Source field is rehydrated or reset. */
IsleGameResult isle_rainbow_actor_activate(IsleRainbowActorSource*, IsleGame*,
    const IsleRainbowActorServices*);
/* Host-only after real native/Source process joins, GPU/readback/Photo/audio
 * fences, semantic object holds and model/FX credits have closed. Idle/process
 * counts alone DO NOT prove these external joins. Requires null retained GObjs,
 * invalidated ModelRefs and collision.native; clears only scene-native bindings,
 * preserves owner/scalars and latches terminal Game fault/STOPPED in status.
 * OK reports successful detachment, not successful original Source execution. */
IsleGameResult isle_rainbow_actor_deactivate(IsleRainbowActorSource*);
/* Returning Root -> ActorOwner -> typed course-context lookup, no registry or
 * Source execution. During activate only, it may recognize PREPARED with the
 * provisional nativeGame/services binding; this is NOT callback/factory
 * admission. Every executable require/invoke/callback still requires ACTIVE.
 * Failure rolls back the provisional bindings; ACTIVE is published last. */
IsleGameResult isle_rainbow_actor_native_context(IsleGame*, IsleRainbowActorSource**);
/* Returning ACTIVE-only execution admission: canonical bound Actor Owner,
 * complete immutable Source/assets/services closure and cold capacity proof.
 * Never required by the provisional activation lookup above. */
IsleGameResult isle_rainbow_actor_native_admission(IsleRainbowActorSource*);
IsleRainbowActorSource* isle_rainbow_actor_require(IsleGame*);
void isle_rainbow_actor_checked(IsleRainbowActorSource*, IsleGameResult);
/* Nonwaiting C fences for native lifecycle/animation/item callback entry. */
IsleGameResult isle_rainbow_actor_invoke_spawn(IsleRainbowActorSource*, uint32_t);
IsleGameResult isle_rainbow_actor_invoke_animation(IsleRainbowActorSource*, const IsleGameModelRef*, int32_t, float);
IsleGameResult isle_rainbow_actor_invoke_collision(IsleRainbowActorSource*, IsleGameVec3*, IsleGameVec3*, IsleGameVec3*, IsleGameVec3*, int32_t*);
void func_803466C0_825E30(IsleRainbowActorSource*);
void func_80346898_826008(IsleRainbowActorSource*);
void func_803484F0_827C60(int32_t, int32_t*);
void func_80348BDC_82834C(IsleRainbowActorSource*, const IsleGameModelRef*, int32_t, float);
int32_t func_80348FF0_828760(IsleRainbowActorSource*, IsleGameVec3*, IsleGameVec3*, IsleGameVec3*, IsleGameVec3*);
void func_803467A4_825F14(IsleGameObject*);
void func_80346FA0_826710(IsleGameObject*);
void func_803470CC_82683C(IsleGameObject*);
void func_80347188_8268F8(IsleGameObject*);
void func_803471D4_826944(IsleGameObject*);
void func_80347334_826AA4(IsleGameObject*);
void func_80347398_826B08(IsleGameObject*);
void func_80347574_826CE4(IsleGameObject*);
void func_803475D0_826D40(IsleGameObject*);
void func_80347724_826E94(IsleGameObject*);
void func_80347900_827070(IsleGameObject*);
void func_80347940_8270B0(IsleGameObject*);
void func_8034799C_82710C(IsleGameObject*);
void func_80347B88_8272F8(IsleGameObject*);
void func_80347CC8_827438(IsleGameObject*);
void func_80347E0C_82757C(IsleGameObject*);
void func_80347E44_8275B4(IsleGameObject*);
void func_8034800C_82777C(IsleGameObject*);
void func_803480C0_827830(IsleGameObject*);
void func_80348208_827978(IsleGameObject*);
void func_803482EC_827A5C(IsleGameObject*);
void func_80348540_827CB0(IsleGameObject*);
void func_8034877C_827EEC(IsleGameObject*);
void func_803487CC_827F3C(IsleGameObject*);
void func_80348850_827FC0(IsleGameObject*);
void func_80348970_8280E0(IsleGameObject*);
void func_80348994_828104(IsleGameObject*);
void func_80348A04_828174(IsleGameObject*);
void func_80348B34_8282A4(IsleGameObject*);
void func_80348C08_828378(IsleGameObject*);
void func_80348D48_8284B8(IsleGameObject*);
void func_80348DD4_828544(IsleGameObject*);
void func_80349084_8287F4(IsleGameObject*);
void func_80349180_8288F0(IsleGameObject*);
void func_803491D4_828944(IsleGameObject*);
void func_8034930C_828A7C(IsleGameObject*);
void func_8034935C_828ACC(IsleGameObject*);
void func_803493F4_828B64(IsleGameObject*);
void func_80349480_828BF0(IsleGameObject*);
void func_80349514_828C84(IsleGameObject*);
void func_8034956C_828CDC(IsleGameObject*);
void func_803495FC_828D6C(IsleGameObject*);
void func_80349654_828DC4(IsleGameObject*);
void func_803496BC_828E2C(IsleGameObject*);
void func_80349714_828E84(IsleGameObject*);
void func_8034977C_828EEC(IsleGameObject*);
void func_80349814_828F84(IsleGameObject*);
void func_803498A0_829010(IsleGameObject*);
void func_80349934_8290A4(IsleGameObject*);
void func_8034998C_8290FC(IsleGameObject*);
void func_80349A1C_82918C(IsleGameObject*);
void func_80349A74_8291E4(IsleGameObject*);
void func_80349ADC_82924C(IsleGameObject*);
void func_80349B34_8292A4(IsleGameObject*);
void func_80349B98_829308(IsleGameObject*);
void func_80349C18_829388(IsleGameObject*);
void func_80349D40_8294B0(IsleGameObject*);
void func_80349DB0_829520(IsleGameObject*);
void func_80349DE8_829558(IsleGameObject*);
void func_80349E28_829598(IsleGameObject*);
void func_80349F00_829670(IsleGameObject*);
void func_80349F88_8296F8(IsleGameObject*);
void func_8034A09C_82980C(IsleGameObject*);
void func_8034A190_829900(IsleGameObject*);
void func_8034A27C_8299EC(IsleGameObject*);
void func_8034A334_829AA4(IsleGameObject*);
void func_8034A3A0_829B10(IsleGameObject*);
IsleGameObject* func_80348938_8280A8(int32_t, uint16_t, IsleActorFactoryBlock*, IsleActorFactoryBlock*, IsleActorFactorySpawn*);
IsleGameObject* func_80348FB8_828728(int32_t, uint16_t, IsleActorFactoryBlock*, IsleActorFactoryBlock*, IsleActorFactorySpawn*);
IsleGameObject* func_80349EB8_829628(int32_t, uint16_t, IsleActorFactoryBlock*, IsleActorFactoryBlock*, IsleActorFactorySpawn*);
IsleGameObject* func_8034A064_8297D4(int32_t, uint16_t, IsleActorFactoryBlock*, IsleActorFactoryBlock*, IsleActorFactorySpawn*);
IsleGameObject* func_8034A4E8_829C58(int32_t, uint16_t, IsleActorFactoryBlock*, IsleActorFactoryBlock*, IsleActorFactorySpawn*);
/* Shared factory wrapper is the existing ordinary-then-real-DL4 transport. */
IsleGameObject* isle_actor_owner_spawn_dl4(int32_t, uint16_t, IsleActorFactoryBlock*, IsleActorFactoryBlock*, IsleActorFactorySpawn*, const IsleActorFactoryInitialization*);
const IsleActorRandomState* isle_rainbow_actor_weighted(IsleGameObject*, uint32_t);
/* Returning parent weighted lookup; resolves only compiled admitted callbacks. */
IsleGameResult isle_rainbow_actor_native_weighted(IsleRainbowActorSource*, uint32_t, const IsleActorRandomState**);
#ifdef __cplusplus
}
#endif
#endif
