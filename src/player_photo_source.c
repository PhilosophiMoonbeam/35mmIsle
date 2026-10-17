#include "player_photo_source.h"
#include "engine_internal.h"
#include <stdbool.h>
#include <stddef.h>
#define ARRAY_COUNT(a) (sizeof(a) / sizeof((a)[0]))
/* Complete pinned roll/query/reset bodies, generated with counted semantic
 * copies rather than memcpy of guest packed records. */
#include "player_photo_original.inc"

void isle_player_photo_PokemonDetector_CopyInfo(IsleGameObject* object) {
    IslePlayerSource* player = isle_player_source_current(object);
    if (player->photo == NULL)
        isle_native_execution_fault(player->game, "source CopyInfo has no canonical photo owner");
    if (isle_player_photo_publish_completed(player->photo) != ISLE_GAME_OK)
        isle_native_execution_fault(player->game, "source CopyInfo completed-frame publication failed");
}
