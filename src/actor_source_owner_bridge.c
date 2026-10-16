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

void isle_actor_source_yaw(IsleGameObject* object, float yaw) {
    IsleActorSource* actor = isle_actor_source_require(object);
    checked(actor, isle_actor_owner_yaw(actor, yaw));
}
void isle_actor_source_magikarp_active(IsleGameObject* object, int active) {
    IsleActorSource* actor = isle_actor_source_require(object);
    checked(actor, isle_actor_owner_magikarp_active(actor, active));
}
size_t isle_actor_source_ground_node(IsleGameObject* object) {
    IsleActorSource* actor = isle_actor_source_require(object);
    size_t node = 0;
    checked(actor, isle_actor_owner_ground_node(actor, &node));
    return node;
}
void isle_actor_source_node_world(IsleGameObject* object, size_t node, IsleGameVec3* position, IsleGameVec3* velocity) {
    IsleActorSource* actor = isle_actor_source_require(object);
    checked(actor, isle_actor_owner_node_world(actor, node, position, velocity));
}
void isle_actor_source_ground(IsleGameObject* object, float x, float z, IsleActorGround* ground) {
    IsleActorSource* actor = isle_actor_source_require(object);
    checked(actor, isle_actor_owner_ground(actor, x, z, ground));
}
void* isle_actor_source_current_block(IsleGameObject* object) {
    IsleActorSource* actor = isle_actor_source_require(object);
    void* block = isle_actor_owner_current_block(actor);
    if (block == NULL) checked(actor, ISLE_GAME_INVALID_STATE);
    return block;
}
IsleGameObject* isle_actor_source_add_one(IsleGameObject* object, void* block, void* reference,
    const IsleActorGeoSpawn* spawn, const IsleActorGeoDefinition* definition) {
    IsleActorSource* actor = isle_actor_source_require(object);
    IsleGameObject* child = NULL;
    checked(actor, isle_actor_owner_add_one(actor, block, reference, spawn, definition, &child));
    if (child == NULL) checked(actor, ISLE_GAME_EXHAUSTED);
    return child;
}
void isle_actor_source_copy_position(IsleGameObject* object, IsleGameObject* child) {
    IsleActorSource* actor = isle_actor_source_require(object);
    checked(actor, isle_actor_owner_copy_position(actor, child));
}
void isle_actor_source_publish_position(IsleGameObject* object, IsleGameVec3 position) {
    IsleActorSource* actor = isle_actor_source_require(object);
    checked(actor, isle_actor_owner_trigger_position(actor, position));
}
IsleGameResult isle_actor_source_id_in_use(IsleGame* game, uint32_t id, int* output) {
    IsleGameCounts counts;
    GObj* object;
    IsleGameResult result = isle_game_counts(game, &counts);
    if (result != ISLE_GAME_OK) return result;
    if (output == NULL) return ISLE_GAME_INVALID_ARGUMENT;
    *output = 0;
    /* Actual ohFindByLinkAndId semantics: source link3, not just Owner slots.
     * A retained joined identity no longer appears in this native live list. */
    for (object = omGObjListHead[3]; object != NULL; object = object->next) {
        if (object->game == game && object->id == id) { *output = 1; break; }
    }
    return ISLE_GAME_OK;
}
