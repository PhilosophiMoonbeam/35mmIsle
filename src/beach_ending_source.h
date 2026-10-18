#ifndef ISLE_BEACH_ENDING_SOURCE_H
#define ISLE_BEACH_ENDING_SOURCE_H
#include "actor_owner_source.h"
#include "beach_music_source.h"
#include "player_cart_source.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Sole per-course original ending .data/BSS, not a native routing machine. */
typedef struct IsleBeachEndingSource {
    IslePlayerSource* player;
    IsleBeachMusicSource* music;
    int32_t D_beach_802CD9D0, D_beach_802CD9D4;
    uint16_t exceptions[2];
    float sky_speed;
    IsleGameObject* D_beach_80347658;
} IsleBeachEndingSource;
/* Borrowed typed returning services, as with World movement/FX callbacks.
 * Required as a complete binding before factory allocation. Only native
 * lifetimes live behind context; all original ending state remains above. */
typedef struct IsleBeachEndingServices {
    void* context;
    IsleGameResult (*validate)(void*, const IsleBeachEndingSource*);
    IsleGameResult (*release)(void*);
    IsleGameResult (*remove_items)(void*);
    IsleGameResult (*reset_message)(void*);
    IsleGameResult (*reset_camera)(void*);
    IsleGameResult (*start_cutscene)(void*, IsleGameObject*);
    IsleGameResult (*camera)(void*);
    IsleGameResult (*cart_init)(void*, IsleGameObject**);
    IsleGameResult (*cart_animation)(void*);
    IsleGameResult (*cart_retain)(void*, IsleGameObject*);
    IsleGameResult (*cart_callback)(void*, IsleGameObject*, IsleBeachEndingSource*);
} IsleBeachEndingServices;
void func_beach_802CA450(IsleGameObject*);
void func_beach_802CA474(IsleGameObject*);
void func_beach_802CA4CC(IsleGameObject*);
void func_beach_802CA5A8(IsleBeachEndingSource*, const IsleActorNodeRef*, int32_t, float);
void func_beach_802CA5D4(void);
void func_beach_802CA5DC(IsleGameObject*);
void func_beach_802CA8C4(IsleGameObject*);
void func_beach_802CA950(IsleGameObject*);
void func_beach_802CAB14(int32_t, uint16_t, IsleActorFactoryBlock*, IsleActorFactoryBlock*, IsleActorFactorySpawn*);
IsleGameObject* isle_actor_owner_spawn_dl4(int32_t, uint16_t, IsleActorFactoryBlock*, IsleActorFactoryBlock*, IsleActorFactorySpawn*, const IsleActorFactoryInitialization*);
IsleGameResult isle_beach_ending_native_source(IsleActorSource*, IsleBeachEndingSource**);
IsleGameResult isle_beach_ending_native_remove_items(IsleActorSource*);
IsleGameResult isle_beach_ending_native_reset_message(IsleActorSource*);
IsleGameResult isle_beach_ending_native_camera(IsleActorSource*);
IsleGameResult isle_beach_ending_native_cart_init(IsleActorSource*, IsleGameObject**);
IsleGameResult isle_beach_ending_native_cart_animation(IsleActorSource*);
IsleGameResult isle_beach_ending_native_cart_publish(IsleActorSource*, IsleGameObject*);
IsleGameResult isle_beach_ending_native_sound_volume(IsleActorSource*, int32_t, uint32_t);
IsleGameResult isle_beach_ending_native_cart_callback(IsleActorSource*, IsleGameObject*);
IsleGameResult isle_beach_ending_native_command(IsleActorSource*, int32_t);
IsleGameResult isle_beach_ending_native_playing_sound(IsleActorSource*, int32_t, int32_t*);
IsleGameResult isle_beach_ending_native_play_sound(IsleActorSource*, int32_t, int32_t*);
IsleGameResult isle_beach_ending_native_bgm_volume(IsleActorSource*, int32_t, int32_t);
IsleGameResult isle_beach_ending_native_stop_sound(IsleActorSource*, int32_t);
IsleGameResult isle_beach_ending_native_stop_sounds(IsleActorSource*);
IsleGameResult isle_beach_ending_native_reset_camera(IsleActorSource*);
IsleGameResult isle_beach_ending_native_start_cutscene(IsleActorSource*);
IsleGameResult isle_beach_ending_native_sky_speed(IsleActorSource*, float);
IsleBeachEndingSource* isle_beach_ending_source(IsleGameObject*);
int32_t isle_beach_ending_play_sound(IsleGameObject*, int32_t);
void isle_beach_ending_command(IsleGameObject*, int32_t);
void isle_beach_ending_bgm_volume(IsleGameObject*, int32_t, int32_t);
void isle_beach_ending_sound_volume(IsleGameObject*, int32_t, uint32_t);
int32_t isle_beach_ending_playing_sound(IsleGameObject*, int32_t);
void isle_beach_ending_stop_sound(IsleGameObject*, int32_t);
void isle_beach_ending_stop_sounds(IsleGameObject*);
void isle_beach_ending_remove_items(IsleGameObject*);
void isle_beach_ending_reset_camera(IsleGameObject*);
void isle_beach_ending_reset_message(IsleGameObject*);
void isle_beach_ending_start_camera(IsleGameObject*);
void isle_beach_ending_sky_speed(IsleGameObject*, float);
IsleGameObject* isle_beach_ending_cart_init(IsleGameObject*);
void isle_beach_ending_cart_animation(IsleGameObject*);
void isle_beach_ending_cart_event(void*, const IslePlayerCartNode*, int32_t, float);
void isle_beach_ending_cart_publish(IsleGameObject*, IsleGameObject*);
void isle_beach_ending_cart_callback(IsleGameObject*, IsleGameObject*);
#ifdef __cplusplus
}
#endif
#endif
