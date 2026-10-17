#include "item_source.h"
#include "engine_internal.h"

IsleGameResult isle_item_source_bind(IsleGame* game, IsleGameObject* object,
    IsleItemSource* item, void* owner) {
    IsleGameObjectMetadata metadata;
    IsleGameResult result = isle_game_object_metadata(game, object, &metadata);
    if (result != ISLE_GAME_OK) return result;
    if (object == NULL || item == NULL || owner == NULL) return ISLE_GAME_INVALID_ARGUMENT;
    if (game->fault != ISLE_GAME_OK) return game->fault;
    if (object->retirementStarted || object->sourcePayloadKind != ISLE_SOURCE_PAYLOAD_NONE ||
        object->userData != NULL || item->game != NULL || item->object != NULL)
        return ISLE_GAME_INVALID_STATE;
    *item = (IsleItemSource){.owner = owner, .game = game, .object = object};
    object->userData = item;
    object->sourcePayloadKind = ISLE_SOURCE_PAYLOAD_ITEM;
    return ISLE_GAME_OK;
}

IsleGameResult isle_item_source_get(IsleGame* game, IsleGameObject* object, IsleItemSource** output) {
    IsleGameObjectMetadata metadata;
    IsleItemSource* item;
    IsleGameResult result;
    if (output != NULL) *output = NULL;
    result = isle_game_object_metadata(game, object, &metadata);
    if (result != ISLE_GAME_OK) return result;
    if (object == NULL || output == NULL) return ISLE_GAME_INVALID_ARGUMENT;
    if (game->fault != ISLE_GAME_OK) return game->fault;
    if (object->retirementStarted || object->sourcePayloadKind != ISLE_SOURCE_PAYLOAD_ITEM ||
        object->userData == NULL) return ISLE_GAME_INVALID_STATE;
    item = object->userData;
    if (item->game != game || item->object != object || item->owner == NULL)
        return ISLE_GAME_INVALID_STATE;
    *output = item;
    return ISLE_GAME_OK;
}

IsleItemSource* isle_item_source_require(IsleGameObject* object) {
    IsleGame* game = object != NULL ? object->game :
        (omCurrentObject != NULL ? omCurrentObject->game : NULL);
    IsleItemSource* item = NULL;
    if (game == NULL || isle_item_source_get(game, object, &item) != ISLE_GAME_OK ||
        !game->ticking || omCurrentObject == NULL)
        isle_native_execution_fault(game, "selected Item source requires a live native Item callback");
    return item;
}
