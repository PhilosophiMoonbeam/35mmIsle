#include "player_photo_rsp_fixture.h"
#include "actor_source.h"
#include "player_render_source.h"

static void make(IslePlayerSource* player, void* user, int raw, int checked) {
    PlayerPhotoRspMake* result = user;
    ++result->entered;
    result->value = raw ? isle_player_photo_makePhoto(player->photo, result->subject) :
        isle_player_photo_take_photo(player->photo, result->subject);
    result->status = player->photo->status;
    /* take_photo already released its exact short borrow before this transfer. */
    if (checked) isle_player_source_checked(player, result->status);
    ++result->returned;
}

void player_photo_rsp_make(IslePlayerSource* player, void* user) {
    make(player, user, 0, 0);
}

void player_photo_rsp_make_raw(IslePlayerSource* player, void* user) {
    make(player, user, 1, 0);
}

void player_photo_rsp_make_checked(IslePlayerSource* player, void* user) {
    make(player, user, 0, 1);
}

void player_photo_rsp_release_checked(IslePlayerSource* player, void* user) {
    PlayerPhotoRspRelease* result = user;
    ++result->entered;
    result->released = isle_player_photo_release_taken(player->photo, &result->ticket);
    result->status = player->photo->status;
    isle_player_source_checked(player, result->status);
    ++result->returned;
}

void player_photo_rsp_consume_retired_subject(IslePlayerSource* player, void* user) {
    PlayerPhotoRspRetiredSubject* result = user;
    IsleActorSource* actor = NULL;
    ++result->entered;
    isle_player_source_checked(player, isle_player_photo_published_focus(player->photo, &result->before));
    result->lookup = isle_actor_source_get_nullable(player->game, result->subject, &actor);
    isle_player_source_checked(player, result->lookup);
    result->missing = actor == NULL;
    if (result->missing && result->before.present && result->before.object == result->subject) {
        isle_player_source_checked(player, isle_player_photo_clear_published_presence(player->photo));
        ++result->cleared;
    }
    isle_player_source_checked(player, isle_player_photo_published_focus(player->photo, &result->after));
    ++result->returned;
}

void player_photo_rsp_copy_info(IsleGameObject* object, void* user) {
    PlayerPhotoRspCopies* result = user;
    ++result->entered;
    isle_player_photo_PokemonDetector_CopyInfo(object);
    ++result->returned;
}

static PlayerPhotoRspBoundary* boundary;
void player_photo_rsp_boundary_bind(PlayerPhotoRspBoundary* value) {
    boundary = value;
}
void player_photo_rsp_boundary_render(IslePlayerSource* player, IsleGameObject* object) {
    PlayerPhotoRspBoundary* current = boundary;
    isle_player_source_checked(player, current != NULL && current->render != NULL &&
        current->observe != NULL ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE);
    ++current->entered;
    current->render(player, object);
    isle_player_source_checked(player, current->observe(current->context));
    /* Independently owned SDK mechanism commands, not a GAME producer rewrite.
     * These execute only after the real callback and its task boundary return. */
    for (uint32_t lane = 0; lane < 4; ++lane) {
        static const uint32_t colors[4] = {
            0xf801f801u, 0x07c107c1u, 0x003f003fu, 0xffffffffu
        };
        const int32_t x = 20 + (int32_t)lane * 12;
        if (!(current->suffix_lanes & (1u << lane))) continue;
        isle_player_source_checked(player, isle_player_render_backend_pipe_sync(player, lane));
        isle_player_source_checked(player, isle_player_render_backend_color_target(
            player, lane, ISLE_PLAYER_RENDER_COLOR_TARGET));
        isle_player_source_checked(player, isle_player_render_backend_scissor(player, lane, 0, 0, 320, 240));
        isle_player_source_checked(player, isle_player_render_backend_cycle(player, lane, ISLE_PLAYER_RENDER_FILL_CYCLE));
        isle_player_source_checked(player, isle_player_render_backend_mode(player, lane, ISLE_PLAYER_RENDER_NOOP));
        isle_player_source_checked(player, isle_player_render_backend_fill_color(player, lane, colors[lane]));
        isle_player_source_checked(player, isle_player_render_backend_fill_rectangle(player, lane, x, 100, x + 7, 107));
        isle_player_source_checked(player, isle_player_render_backend_pipe_sync(player, lane));
        isle_player_source_checked(player, isle_player_render_backend_cycle(player, lane, ISLE_PLAYER_RENDER_ONE_CYCLE));
        isle_player_source_checked(player, isle_player_render_backend_mode(player, lane, ISLE_PLAYER_RENDER_AA_ZB_OPAQUE));
    }
    ++current->returned;
}
