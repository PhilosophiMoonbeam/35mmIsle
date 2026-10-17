#include "player_photo_rsp_fixture.h"
#include "actor_source.h"

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
