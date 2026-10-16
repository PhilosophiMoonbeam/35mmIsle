#include "actor_owner_source.h"
#include "engine_internal.h"

static void checked(IsleActorSource* actor, IsleGameResult result) {
    if (result != ISLE_GAME_OK)
        isle_native_execution_fault(actor->game, "native actor owner source leaf failed");
}
void isle_actor_source_set_animation(IsleGameObject* object, uint32_t identity, int force) {
    IsleActorSource* actor = isle_actor_source_require(object);
    IsleGameResult result = isle_actor_owner_animation(actor, identity, 0.0f, force);
    checked(actor, result);
}
void isle_actor_source_subtract_y(IsleGameObject* object, float amount) {
    IsleActorSource* actor = isle_actor_source_require(object);
    IsleGameResult result = isle_actor_owner_local_subtract_y(actor, amount);
    checked(actor, result);
}
void isle_actor_source_visibility(IsleGameObject* object, int hidden) {
    IsleActorSource* actor = isle_actor_source_require(object);
    IsleGameResult result = isle_actor_owner_visibility(actor, hidden);
    checked(actor, result);
}
void isle_actor_source_commands(IsleGameObject* object) {
    IsleActorSource* actor = isle_actor_source_require(object);
    IsleGameResult result = isle_actor_owner_commands(actor);
    checked(actor, result);
}
void Pokemon_Cleanup(IsleGameObject* object) {
    IsleActorSource* actor = isle_actor_source_require(object);
    IsleGameResult result = isle_actor_owner_cleanup(actor);
    checked(actor, result);
}
void isle_actor_source_interactions(IsleGameObject* object) {
    IsleActorSource* actor = isle_actor_source_require(object);
    IsleGameResult result = isle_actor_owner_interactions(actor);
    checked(actor, result);
}
void isle_actor_source_animation_process(IsleGameObject* object) {
    IsleActorSource* actor = isle_actor_source_require(object);
    IsleGameResult result = isle_actor_owner_step(actor);
    checked(actor, result);
}
void isle_actor_source_distance_process(IsleGameObject* object) {
    IsleActorSource* actor = isle_actor_source_require(object);
    IsleGameResult result = isle_actor_owner_distance(actor);
    checked(actor, result);
}
void isle_actor_source_common1003_trigger(IsleGameObject* object, IsleGameVec3 position) {
    IsleActorSource* actor = isle_actor_source_require(object);
    IsleGameResult result = func_8035E264_4FE674(object, &position);
    checked(actor, result);
}

IsleGameResult isle_actor_source_pause_object(IsleActorSource* actor, int paused) {
    IsleActorSource* bound = NULL;
    IsleGameResult result;
    GObjProcess* process;
    if (actor == NULL) return ISLE_GAME_INVALID_ARGUMENT;
    result = isle_actor_source_get(actor->game, actor->object, &bound);
    if (result != ISLE_GAME_OK || bound != actor) return ISLE_GAME_INVALID_STATE;
    for (process = actor->object->processListHead; process != NULL; process = process->next) {
        result = isle_game_pause_process(actor->game, process, paused);
        if (result != ISLE_GAME_OK) return result;
    }
    return ISLE_GAME_OK;
}

IsleGameResult isle_actor_source_validate_model(IsleGame* game, IsleGameObject* object, IsleGameModel* model) {
    IsleGameObjectMetadata metadata;
    const IsleGameModelNode* nodes = NULL;
    size_t count = 0;
    IsleGameResult result = isle_game_object_metadata(game, object, &metadata);
    if (result != ISLE_GAME_OK) return result;
    if (object == NULL || model == NULL || object->retirementStarted ||
        object->type != 1 || object->data.model != model) return ISLE_GAME_INVALID_ARGUMENT;
    return isle_game_model_peek_nodes(game, model, &nodes, &count);
}
