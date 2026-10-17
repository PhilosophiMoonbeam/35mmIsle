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

void player_source_sprite_borrow(IslePlayerSource* source, void* user) {
    PlayerSpriteBorrow* request = user;
    IsleSprite* view;
    ++request->entered;
    source->ThrowItemTimeout = 41;
    if (request->context != NULL) source = request->context;
    if (request->view_only) {
        view = isle_player_source_sprite_view(source, request->sprite);
    } else {
        view = isle_player_source_sprite_view(source,
            isle_player_source_first_sprite(source, request->object));
    }
    isle_sprite_clear_attribute(view, ISLE_SP_HIDDEN);
    isle_sprite_color(view, 33, 66, 99, 132);
    ++request->completed;
    source->ThrowItemTimeout = 42;
}

void player_source_sprite_nested_borrow(IslePlayerSource* source, void* user) {
    IsleGameResult result = isle_player_source_call(source, player_source_sprite_borrow, user);
    isle_player_source_checked(source, result);
    source->ThrowItemTimeout = 98;
}

void player_source_sprite_wait(IsleGameObject* object) {
    IslePlayerSource* source = isle_player_source_current(object);
    {
        IsleSprite* view = isle_player_source_sprite_view(source,
            isle_player_source_first_sprite(source, source->D_80382C38_523048));
        isle_sprite_move(view, 32768, -65537);
        isle_sprite_scale(view, 0.5f, 1.5f);
        isle_sprite_set_attribute(view, ISLE_SP_HIDDEN);
        isle_sprite_color(view, 1, 2, 3, 254);
    }
    source->ThrowItemTimeout = 41;
    isle_player_source_wait(source, 1);
    source = isle_player_source_current(object);
    {
        /* Native ownership may have changed while suspended. No cached view or
         * copied scalar authority survives this wait. */
        IsleSprite* view = isle_player_source_sprite_view(source,
            isle_player_source_first_sprite(source, source->D_80382C38_523048));
        isle_sprite_clear_attribute(view, ISLE_SP_HIDDEN | ISLE_SP_SCALE);
        view->scalex = 3.0f;
        view->scaley = 0.25f;
        isle_sprite_move(view, INT32_MIN, INT32_MAX);
        isle_sprite_color(view, 4, 5, 6, 17);
    }
    source->ThrowItemTimeout = 42;
    omEndProcess(NULL);
    source->ThrowItemTimeout = 99;
}

void player_source_sprite_delete_self(IsleGameObject* object) {
    IslePlayerSource* source = isle_player_source_current(object);
    isle_player_source_checked(source, isle_game_delete_object(source->game, object));
    /* A self-delete request is not actual teardown until this C frame yields. */
    isle_sprite_color(isle_player_source_sprite_view(source,
        isle_player_source_first_sprite(source, object)), 7, 8, 9, 101);
    source->ThrowItemTimeout = 41;
    isle_player_source_wait(source, 1);
    source->ThrowItemTimeout = 99;
}

void player_source_sprite_explicit_context(IsleGameObject* object, void* user) {
    IslePlayerSource* source = user;
    isle_player_source_first_sprite(source, object);
    source->ThrowItemTimeout = 99;
}

void player_source_pause_all(IslePlayerSource* source, void* user) {
    PlayerObjectControl* request = user;
    isle_player_source_pause_object_processes(source, request->object, request->paused);
    ++request->completed;
}
void player_source_find_object(IslePlayerSource* source, void* user) {
    PlayerObjectControl* request = user;
    request->found = isle_player_source_find_object(source, request->link, request->id);
    ++request->completed;
}
void player_source_pause_current(IsleGameObject* object) {
    IslePlayerSource* source = isle_player_source_entry(object);
    ++source->ThrowItemTimeout;
    isle_player_source_pause_object_processes(source, NULL, 1);
    ++source->ThrowItemTimeout;
}
