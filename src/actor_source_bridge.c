#include "actor_source.h"
#include "engine_internal.h"

IsleGameResult isle_actor_source_bind(IsleGame* game, IsleGameObject* object,
    IsleActorSource* actor, isle_rng* rng, void* owner) {
    IsleGameObjectMetadata metadata;
    IsleGameResult result = isle_game_object_metadata(game, object, &metadata);
    if (result != ISLE_GAME_OK) return result;
    if (object == NULL || actor == NULL || rng == NULL || owner == NULL) return ISLE_GAME_INVALID_ARGUMENT;
    if (game->fault != ISLE_GAME_OK) return game->fault;
    if (object->retirementStarted || object->actorSourceBound || object->userData != NULL || actor->game != NULL || actor->object != NULL)
        return ISLE_GAME_INVALID_STATE;
    *actor = (IsleActorSource){.rng = rng, .owner = owner, .game = game, .object = object};
    object->userData = actor;
    object->actorSourceBound = true;
    return ISLE_GAME_OK;
}

IsleGameResult isle_actor_source_get(IsleGame* game, IsleGameObject* object, IsleActorSource** output) {
    IsleGameObjectMetadata metadata;
    IsleActorSource* actor;
    IsleGameResult result;
    if (output != NULL) *output = NULL;
    result = isle_game_object_metadata(game, object, &metadata);
    if (result != ISLE_GAME_OK) return result;
    if (object == NULL || output == NULL) return ISLE_GAME_INVALID_ARGUMENT;
    if (game->fault != ISLE_GAME_OK) return game->fault;
    if (object->retirementStarted || !object->actorSourceBound || object->userData == NULL) return ISLE_GAME_INVALID_STATE;
    actor = object->userData;
    if (actor->game != game || actor->object != object || actor->rng == NULL || actor->owner == NULL)
        return ISLE_GAME_INVALID_STATE;
    *output = actor;
    return ISLE_GAME_OK;
}

IsleActorSource* isle_actor_source_require(IsleGameObject* object) {
    IsleActorSource* actor = NULL;
    /* Selected source entrypoints require a live callback. Owner-side startup
     * uses the status-returning engine API, never this nonreturning C boundary. */
    if (isle_actor_source_get(object->game, object, &actor) != ISLE_GAME_OK ||
        !object->game->ticking || omCurrentObject == NULL) {
        isle_native_execution_fault(object->game, "selected actor control requires a live native actor callback");
    }
    return actor;
}

IsleGameProcess* isle_actor_source_create_process(IsleGameObject* object,
    IsleGameSourceCallback function, int32_t kind, int32_t priority) {
    IsleActorSource* actor = isle_actor_source_require(object);
    IsleGameProcess* process = NULL;
    IsleGameResult result = isle_game_add_source_process(actor->game, object, function,
        (IsleGameProcessKind)kind, (uint32_t)priority, &process);
    if (result != ISLE_GAME_OK) {
        /* Worker fail exits its saved C fence. Synchronous owner failure exits
         * the existing tick C fence. The selected body cannot publish NULL and
         * continue as though allocation/continuation creation had succeeded. */
        isle_native_execution_fault(actor->game, "native actor source process creation failed");
    }
    return process;
}

int32_t isle_actor_source_rand_range(IsleGameObject* object, int32_t range) {
    IsleActorSource* actor = isle_actor_source_require(object);
    return isle_rng_range(actor->rng, range);
}
