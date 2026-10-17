#include "player_photo_source.h"
#include "engine_internal.h"
#include <stdbool.h>
#include <stddef.h>
#define ARRAY_COUNT(a) (sizeof(a) / sizeof((a)[0]))
/* Complete pinned roll/query/reset bodies, generated with counted semantic
 * copies rather than memcpy of guest packed records. */
#include "player_photo_original.inc"

int32_t isle_player_photo_take_photo(IslePlayerPhotoSource* source, IsleGameObject* subject) {
    IslePlayerPhotoTicket ticket;
    IsleGameResult first, released;
    int32_t value;
    if (isle_player_photo_borrow_taken(source, &ticket) != ISLE_GAME_OK) return -1;
    /* Both C++ leaves return normally. Only this plain C frame encloses the
     * original C body; no owning C++ guard survives any later checked transfer. */
    value = isle_player_photo_makePhoto(source, subject);
    first = source->status;
    released = isle_player_photo_release_taken(source, &ticket);
    if (first != ISLE_GAME_OK) source->status = first;
    else if (source->status == ISLE_GAME_OK) source->status = released;
    return value;
}

void isle_player_photo_PokemonDetector_CopyInfo(IsleGameObject* object) {
    IslePlayerSource* player = isle_player_source_current(object);
    if (player->photo == NULL)
        isle_native_execution_fault(player->game, "source CopyInfo has no canonical photo owner");
    if (isle_player_photo_publish_completed(player->photo) != ISLE_GAME_OK)
        isle_native_execution_fault(player->game, "source CopyInfo completed-frame publication failed");
}
