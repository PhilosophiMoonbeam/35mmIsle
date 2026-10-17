#include "actor_owner_source.h"
#include "actor_spatial.h"
#include "actor_interaction_services.h"
#include "engine_internal.h"

static void checked(IsleActorSource* actor, IsleGameResult result) {
    if (result != ISLE_GAME_OK)
        isle_native_execution_fault(actor->game, "native actor owner source leaf failed");
}
IsleGameModelRef isle_actor_source_self_model(IsleGameObject* object) {
    IsleActorSource* actor = isle_actor_source_require(object);
    IsleGameModelRef value = {0};
    checked(actor, isle_actor_owner_self_model(actor, &value));
    return value;
}
const IsleGameModelTransform* isle_actor_source_aux(IsleGameObject* object, const IsleGameModelRef* model) {
    IsleActorSource* actor = isle_actor_source_require(object);
    const IsleGameModelTransform* value = NULL;
    checked(actor, isle_actor_owner_aux_read(actor, model, &value));
    return value;
}
void isle_actor_source_aux_store(IsleGameObject* object, const IsleGameModelRef* model,
    uint32_t mask, const IsleGameModelTransform* values) {
    IsleActorSource* actor = isle_actor_source_require(object);
    checked(actor, isle_actor_owner_aux_store(actor, model, mask, values));
}
int32_t isle_actor_source_step_walk(IsleGameObject* object, float vx, float vz, uint32_t flags) {
    IsleActorSource* actor = isle_actor_source_require(object);
    int32_t value = 0;
    checked(actor, isle_actor_owner_step_walk(actor, vx, vz, flags, &value));
    return value;
}
IsleActorScaleInitialization isle_actor_source_scale_initialization(IsleGameObject* object) {
    IsleActorSource* actor = isle_actor_source_require(object);
    IsleActorScaleInitialization value = {0};
    checked(actor, isle_actor_owner_scale_initialization(actor, &value));
    return value;
}
IsleActorPositionRef isle_actor_source_target_position_ref(IsleGameObject* object, IsleGameObject* target) {
    IsleActorSource* actor = isle_actor_source_require(object);
    IsleActorPositionRef value = {0};
    checked(actor, isle_actor_owner_target_position_ref(actor, target, &value));
    return value;
}
const IsleGameVec3* isle_actor_source_position_read(IsleGameObject* object, const IsleActorPositionRef* reference) {
    IsleActorSource* actor = isle_actor_source_require(object);
    const IsleGameVec3* value = NULL;
    checked(actor, isle_actor_owner_position_read(actor, reference, &value));
    return value;
}
void isle_actor_source_node_world_ref(IsleGameObject* object, const IsleActorNodeRef* reference,
    IsleGameVec3* position, IsleGameVec3* velocity) {
    IsleActorSource* actor = isle_actor_source_require(object);
    checked(actor, isle_actor_owner_node_world_ref(actor, reference, position, velocity));
}
IsleActorPathInfo isle_actor_source_path_info(IsleGameObject* object, const void* path) {
    IsleActorSource* actor = isle_actor_source_require(object);
    IsleActorPathInfo value = {0};
    checked(actor, isle_actor_owner_path_info(actor, path, &value));
    return value;
}
void isle_actor_source_path_position(IsleGameObject* object, const void* path, float parameter, IsleGameVec3* value) {
    IsleActorSource* actor = isle_actor_source_require(object);
    checked(actor, isle_actor_owner_path_position(actor, path, parameter, value));
}
void isle_actor_source_path_velocity(IsleGameObject* object, const void* path, float parameter, IsleGameVec3* value) {
    IsleActorSource* actor = isle_actor_source_require(object);
    checked(actor, isle_actor_owner_path_velocity(actor, path, parameter, value));
}
IsleGameVec3 isle_actor_source_current_world_position(IsleGameObject* object) {
    IsleActorSource* actor = isle_actor_source_require(object);
    IsleGameVec3 value = {0};
    checked(actor, isle_actor_owner_current_world_position(actor, &value));
    return value;
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
static IsleGameResult command_admission(IsleGameObject* object, int32_t command,
    IsleGameObject* sender, void* user) {
    const IsleActorInteraction* rows = NULL;
    (void)object; (void)command; (void)sender;
    return isle_actor_interaction_rows((IsleActorSource*)user,&rows);
}
static IsleGameResult command_status(IsleGameObject* object, int32_t command,
    IsleGameObject* sender, void* user) {
    (void)object; (void)command; (void)sender;
    return isle_actor_owner_command_status((IsleActorSource*)user);
}
static void command_body(IsleGameObject* object, int32_t command,
    IsleGameObject* sender, void* user) {
    IsleActorCommandData value = {command,sender};
    (void)object; (void)user;
    Pokemon_HandleCommand(value);
}
void isle_actor_source_commands(IsleGameObject* object) {
    IsleActorSource* actor = isle_actor_source_require(object);
    IsleSourceCommandScope scope = {0};
    int erase = 0;
    IsleGameResult result = isle_actor_owner_commands_begin(actor,&scope);
    IsleGameResult ended;
    checked(actor,result);
    result = isle_game_commands_checked(actor->game,object,scope.pool,scope.queue,
        command_body,NULL,command_admission,command_status,actor,&erase);
    ended = isle_actor_owner_commands_end(actor,&scope,result,erase);
    checked(actor,result);
    checked(actor,ended);
}
void Pokemon_Cleanup(IsleGameObject* object) {
    IsleActorSource* actor = isle_actor_source_require(object);
    IsleGameResult result = isle_actor_owner_cleanup(actor);
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
int32_t isle_actor_source_ground(IsleGameObject* object, float x, float z, IsleActorGround* ground) {
    IsleActorSource* actor = isle_actor_source_require(object);
    int32_t found = 0;
    checked(actor, isle_actor_owner_ground(actor, x, z, ground, &found));
    return found;
}
void* isle_actor_source_current_block(IsleGameObject* object) {
    IsleActorSource* actor = isle_actor_source_require(object);
    void* block = NULL;
    checked(actor, isle_actor_owner_current_block(actor, &block));
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

IsleGameResult isle_actor_source_link3_next(IsleGame* game, IsleGameObject* previous,
    IsleGameObject** out_object, IsleActorSource** out_source, IsleGameModel** out_model) {
    IsleGameCounts counts;
    IsleGameObjectMetadata metadata;
    IsleActorSource* source = NULL;
    GObj* object;
    IsleGameResult result;
    if (out_object != NULL) *out_object = NULL;
    if (out_source != NULL) *out_source = NULL;
    if (out_model != NULL) *out_model = NULL;
    result = isle_game_counts(game, &counts);
    if (result != ISLE_GAME_OK) return result;
    if (out_object == NULL || out_source == NULL || out_model == NULL)
        return ISLE_GAME_INVALID_ARGUMENT;
    if (game->fault != ISLE_GAME_OK) return game->fault;
    if (previous != NULL) {
        result = isle_game_object_metadata(game, previous, &metadata);
        if (result != ISLE_GAME_OK) return result;
        if (previous->retirementStarted) return ISLE_GAME_INVALID_STATE;
        if (metadata.update_link != 3) return ISLE_GAME_INVALID_ARGUMENT;
        object = previous->next;
    } else {
        object = omGObjListHead[3];
    }
    if (object == NULL) return ISLE_GAME_OK;
    result = isle_actor_source_get(game, object, &source);
    if (result != ISLE_GAME_OK) return result;
    if (object->link != 3 || object->type != 1 || object->data.model == NULL)
        return ISLE_GAME_INVALID_STATE;
    result = isle_game_model_validate_binding(game, object, object->data.model);
    if (result != ISLE_GAME_OK) return result;
    *out_object = object;
    *out_source = source;
    *out_model = object->data.model;
    return ISLE_GAME_OK;
}

IsleGameResult isle_actor_spatial_object_model(IsleGame* game, IsleGameObject* object,
    IsleGameModel** out_model) {
    IsleGameObjectMetadata metadata;
    IsleGameCounts counts;
    IsleGameResult result;
    if (out_model != NULL) *out_model = NULL;
    /* Never let metadata's NULL-current-object convention change the source
     * distance NULL sentinel. No object is consumed on this branch. */
    if (object == NULL) {
        result = isle_game_counts(game, &counts);
        if (result != ISLE_GAME_OK) return result;
        if (out_model == NULL) return ISLE_GAME_INVALID_ARGUMENT;
        return game->fault;
    }
    result = isle_game_object_metadata(game, object, &metadata);
    if (result != ISLE_GAME_OK) return result;
    if (out_model == NULL) return ISLE_GAME_INVALID_ARGUMENT;
    if (game->fault != ISLE_GAME_OK) return game->fault;
    if (object->retirementStarted) return ISLE_GAME_INVALID_STATE;
    if (object->type == 0 && object->data.any == NULL) return ISLE_GAME_OK;
    if (object->type != 1) return ISLE_GAME_INVALID_STATE;
    if (object->data.model == NULL) return ISLE_GAME_OK;
    result = isle_game_model_validate_binding(game, object, object->data.model);
    if (result != ISLE_GAME_OK) return result;
    *out_model = object->data.model;
    return ISLE_GAME_OK;
}

IsleGameResult isle_actor_source_install_update(IsleGame* game, IsleGameObject* object) {
    IsleActorSource* actor = NULL;
    IsleGameResult result = isle_actor_source_get(game,object,&actor);
    if (result != ISLE_GAME_OK) return result;
    object->update = NULL;
    object->updateUser = NULL;
    object->fnUpdate = Pokemon_Update;
    return ISLE_GAME_OK;
}
const IsleActorInteraction* isle_actor_source_interaction_rows(IsleGameObject* object) {
    IsleActorSource* actor = isle_actor_source_require(object);
    const IsleActorInteraction* rows = NULL;
    checked(actor,isle_actor_interaction_rows(actor,&rows));
    return rows;
}
IsleGameVec3 isle_actor_source_position(IsleGameObject* requester, IsleGameObject* target, int auxiliary) {
    IsleActorSource* actor = isle_actor_source_require(requester);
    IsleGameVec3 position;
    checked(actor,isle_actor_interaction_position(actor,target,auxiliary,&position));
    return position;
}
void isle_actor_source_item_iterator(IsleGameObject* object) {
    IsleActorSource* actor = isle_actor_source_require(object);
    checked(actor,isle_actor_interaction_item_iterator(actor));
}
IsleGameObject* isle_actor_source_item_next(IsleGameObject* object) {
    IsleActorSource* actor = isle_actor_source_require(object);
    IsleGameObject* item = NULL;
    checked(actor,isle_actor_interaction_item_next(actor,&item));
    return item;
}
IsleItemSource* isle_actor_source_item(IsleGameObject* object, IsleGameObject* target) {
    IsleActorSource* actor = isle_actor_source_require(object);
    IsleItemSource* item = NULL;
    checked(actor,isle_actor_interaction_item_source(actor,target,&item));
    return item;
}
int32_t isle_actor_source_flute(IsleGameObject* object) {
    IsleActorSource* actor = isle_actor_source_require(object);
    int32_t command;
    checked(actor,isle_actor_interaction_flute(actor,&command));
    return command;
}
float isle_actor_source_distance(IsleGameObject* object, IsleGameObject* second) {
    IsleActorSource* actor = isle_actor_source_require(object);
    float distance;
    checked(actor,isle_actor_interaction_distance(actor,second,&distance));
    return distance;
}
float isle_actor_source_neighbors(IsleGameObject* object, IsleGameVec3* direction) {
    IsleActorSource* actor = isle_actor_source_require(object);
    float magnitude;
    checked(actor,isle_actor_interaction_neighbors(actor,direction,&magnitude));
    return magnitude;
}
IsleGameObject* isle_actor_source_player(IsleGameObject* object) {
    IsleActorSource* actor = isle_actor_source_require(object);
    IsleGameObject* player = NULL;
    checked(actor,isle_actor_interaction_player(actor,&player));
    return player;
}
IsleActorFocusState isle_actor_source_focus(IsleGameObject* object) {
    IsleActorSource* actor = isle_actor_source_require(object);
    IsleActorFocusState focus;
    checked(actor,isle_actor_interaction_focus(actor,&focus.timer,&focus.current));
    return focus;
}
int32_t isle_actor_source_direction(IsleGameObject* object) {
    IsleActorSource* actor = isle_actor_source_require(object);
    int32_t direction;
    checked(actor,isle_actor_interaction_direction(actor,&direction));
    return direction;
}
int32_t isle_actor_source_execution_enabled(IsleGameObject* object) {
    IsleActorSource* actor = isle_actor_source_require(object);
    int32_t enabled;
    checked(actor,isle_actor_owner_execution_enabled(actor,&enabled));
    return enabled;
}
void isle_actor_source_pause_all(IsleGameObject* object) {
    IsleActorSource* actor = isle_actor_source_require(object);
    checked(actor,isle_actor_source_pause_object(actor,1));
}
void isle_actor_source_reference_assign(IsleGameObject* object, IsleGameObject** field,
    IsleGameObject* sender) {
    IsleActorSource* actor = isle_actor_source_require(object);
    checked(actor,isle_actor_owner_reference_assign(actor,field,sender));
}
