#include "beach_scyther_special_source.h"
#include "engine_internal.h"

void isle_beach_scyther_special_play_sound(IsleGameObject* object, uint8_t category, int32_t sound) {
    IsleActorSource* actor = isle_actor_source_require(object);
    IsleGameResult result = isle_actor_owner_environment_sound(actor, category, sound);
    if (result != ISLE_GAME_OK)
        isle_native_execution_fault(actor->game, "Beach special environmental sound failed");
}
