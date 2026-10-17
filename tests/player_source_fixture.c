#include "player_source_fixture.h"
#include "engine_internal.h"

void player_source_helper_userdata(IsleGameObject* object, void* value) {
    object->userData = value;
}

void player_source_helper_wait(IsleGameObject* object) {
    IslePlayerSource* source = isle_player_source_current(object);
    source->ThrowItemTimeout = 41;
    isle_player_source_wait(source, 1);
    source = isle_player_source_current(object);
    source->ThrowItemTimeout = 42;
    omEndProcess(NULL);
    source->ThrowItemTimeout = 99;
}

static void original_dual_use_callback(IsleGameObject* object) {
    IslePlayerSource* source = isle_player_source_entry(object);
    source->ThrowItemTimeout = 77;
}
void player_source_cold_entry(IslePlayerSource* source, void* user) {
    (void) source;
    original_dual_use_callback(user);
}

static void returned_native_failure(IslePlayerSource* source, void* user) {
    IsleGameObjectMetadata metadata;
    (void) user;
    source->ThrowItemTimeout = 41;
    /* The native leaf returns before the local C fence transfers control. */
    isle_player_source_checked(source,
        isle_game_object_metadata(source->game, NULL, &metadata));
    source->ThrowItemTimeout = 99;
}

void player_source_nested_failure(IslePlayerSource* source, void* user) {
    IsleGameResult result = isle_player_source_call(source, returned_native_failure, user);
    isle_player_source_checked(source, result);
    source->ThrowItemTimeout = 98;
}

void player_source_forbidden_wait(IslePlayerSource* source, void* user) {
    (void) user;
    source->ThrowItemTimeout = 41;
    isle_player_source_wait(source, 1);
    source->ThrowItemTimeout = 99;
}
