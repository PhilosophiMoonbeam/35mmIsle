#include "beach_music_source.h"

int32_t isle_beach_music_play_sound(IsleBeachMusicSource* source, uint32_t sound,
    int32_t volume, int32_t pan, float pitch, int32_t reverb) {
    int32_t handle = -1;
    isle_player_source_checked(source->player, isle_beach_music_native_play_sound(
        source, sound, volume, pan, pitch, reverb, &handle));
    return handle;
}
void isle_beach_music_volume(IsleBeachMusicSource* source, int32_t handle, uint32_t volume) {
    isle_player_source_checked(source->player, isle_beach_music_native_volume(source, handle, volume));
}
void isle_beach_music_stop(IsleBeachMusicSource* source, uint32_t handle) {
    isle_player_source_checked(source->player, isle_beach_music_native_stop(source, handle));
}
typedef struct MusicCall {
    IsleBeachMusicSource* source;
    int32_t block;
    float part;
} MusicCall;
static void update(IslePlayerSource* player, void* user) {
    MusicCall* call = user;
    if (call->source->player != player)
        isle_player_source_checked(player, ISLE_GAME_INVALID_STATE);
    isle_beach_music_func_802C4800(call->source, call->block, call->part);
}
IsleGameResult isle_beach_music_invoke(IsleBeachMusicSource* source, int32_t block, float part) {
    MusicCall call = {source, block, part};
    if (source == NULL || source->player == NULL || source->owner == NULL)
        return ISLE_GAME_INVALID_ARGUMENT;
    return isle_player_source_call(source->player, update, &call);
}
