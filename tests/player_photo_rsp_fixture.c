#include "player_photo_rsp_fixture.h"

void player_photo_rsp_make(IslePlayerSource* player, void* user) {
    PlayerPhotoRspMake* result = user;
    ++result->entered;
    result->value = isle_player_photo_makePhoto(player->photo, result->subject);
    result->status = player->photo->status;
    ++result->returned;
    /* The caller releases its short consumer borrow before checking status. */
}

void player_photo_rsp_copy_info(IsleGameObject* object, void* user) {
    PlayerPhotoRspCopies* result = user;
    ++result->entered;
    isle_player_photo_PokemonDetector_CopyInfo(object);
    ++result->returned;
}
