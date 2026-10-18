#ifndef ISLE_BEACH_MUSIC_SOURCE_H
#define ISLE_BEACH_MUSIC_SOURCE_H
#include "player_source.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Exact own Beach data/BSS semantics. One course owner initializes .data from
 * the admitted ROM once; binding or movement updates never reset these fields. */
typedef struct IsleBeachMusicSource {
    IslePlayerSource* player;
    void* owner;
    int32_t D_beach_802CC0D0, D_beach_802CC0D4, D_beach_802CC0D8;
    uint8_t D_beach_80347578, D_beach_80347579, D_beach_8034757A, D_beach_8034757B;
    uint16_t D_beach_8034757C;
} IsleBeachMusicSource;
void isle_beach_music_func_802C4800(IsleBeachMusicSource*, int32_t block, float part);
IsleGameResult isle_beach_music_invoke(IsleBeachMusicSource*, int32_t block, float part);
/* Returning native leaves; only the C adapters below may transfer failure. */
IsleGameResult isle_beach_music_native_play_sound(IsleBeachMusicSource*, uint32_t sound,
    int32_t volume, int32_t pan, float pitch, int32_t reverb, int32_t* handle);
IsleGameResult isle_beach_music_native_volume(IsleBeachMusicSource*, int32_t handle, uint32_t volume);
IsleGameResult isle_beach_music_native_stop(IsleBeachMusicSource*, uint32_t handle);
int32_t isle_beach_music_play_sound(IsleBeachMusicSource*, uint32_t sound,
    int32_t volume, int32_t pan, float pitch, int32_t reverb);
void isle_beach_music_volume(IsleBeachMusicSource*, int32_t handle, uint32_t volume);
void isle_beach_music_stop(IsleBeachMusicSource*, uint32_t handle);
#ifdef __cplusplus
}
#endif
#endif
