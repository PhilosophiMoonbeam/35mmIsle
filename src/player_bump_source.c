#include "player_bump_source.h"
#include "engine_internal.h"

IsleGameResult isle_player_bump_source_bind(IslePlayerSource* player,
    IslePlayerBumpSource* source, void* owner) {
    IsleGameResult result;
    if (!player || !source || !owner || !player->game || !player->owner || !player->rng)
        return ISLE_GAME_INVALID_ARGUMENT;
    result = isle_game_status(player->game);
    if (result != ISLE_GAME_OK) return result;
    if (player->status != ISLE_GAME_OK) return player->status;
    if (player->bump || source->player || source->owner || source->BumpDetector_Object)
        return ISLE_GAME_INVALID_STATE;
    *source = (IslePlayerBumpSource){
        .player = player, .owner = owner,
        .BumpDetector_Thresholds = {90.0f, 60.0f, 30.0f, 5.0f}
    };
    player->bump = source;
    return ISLE_GAME_OK;
}

void isle_player_bump_source_record(IslePlayerBumpSource* source, IsleGameResult result) {
    if (source && result != ISLE_GAME_OK && source->status == ISLE_GAME_OK)
        source->status = result;
}

void isle_player_bump_source_checked(IslePlayerBumpSource* source) {
    isle_player_source_checked(source->player, source->status);
}

IslePlayerBumpSource* isle_player_bump_source_current(IsleGameObject* object) {
    IslePlayerSource* player = isle_player_source_current(object);
    IslePlayerBumpSource* source = player->bump;
    if (!source || source->player != player || !source->owner ||
        source->BumpDetector_Object != object)
        isle_player_source_checked(player, ISLE_GAME_INVALID_STATE);
    isle_player_bump_source_checked(source);
    return source;
}

IsleActorSource* isle_player_bump_source_actor(IslePlayerBumpSource* source,
    IsleGameObject* object) {
    IsleActorSource* actor = NULL;
    isle_player_bump_source_record(source,
        isle_actor_source_get(source->player->game, object, &actor));
    isle_player_bump_source_checked(source);
    return actor;
}

IsleGameObject* isle_player_bump_source_object(IslePlayerBumpSource* source,
    uint32_t id, uint8_t link, uint32_t priority) {
    IsleGameObject* object = NULL;
    isle_player_bump_source_record(source,
        isle_player_bump_native_object(source, id, link, priority, &object));
    return object;
}

void isle_player_bump_source_process(IslePlayerBumpSource* source, IsleGameObject* object,
    IsleGameSourceCallback entry, int32_t kind, uint32_t priority) {
    IsleGameProcess* process = NULL;
    isle_player_bump_source_record(source,
        isle_game_add_source_process(source->player->game, object, entry,
            (IsleGameProcessKind)kind, priority, &process));
}

void isle_player_bump_source_pause(IslePlayerBumpSource* source,
    IsleGameObject* object, int paused) {
    IsleGameObjectMetadata metadata;
    IsleGameResult result = isle_game_object_metadata(source->player->game, object, &metadata);
    GObjProcess* process;
    if (result != ISLE_GAME_OK) {
        isle_player_bump_source_record(source, result);
        return;
    }
    for (process = object->processListHead; process; process = process->next) {
        result = isle_game_pause_process(source->player->game, process, paused);
        if (result != ISLE_GAME_OK) {
            isle_player_bump_source_record(source, result);
            return;
        }
    }
}

void isle_player_bump_source_wait(IslePlayerBumpSource* source, int32_t updates) {
    isle_player_bump_source_checked(source);
    isle_player_source_wait(source->player, updates);
}
