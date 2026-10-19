#include "actor_owner_source.h"
#include "actor_spatial.h"
#include "actor_interaction_services.h"
#include "beach_initial_species_source.h"
#include "engine_internal.h"
#include "player_source.h"
#include "menu_scene_bridge.h"
#include "tunnel_actor_source.h"
#include "volcano_actor_source.h"
#include "river_actor_source.h"
#include "cave_actor_source.h"
#include "valley_actor_source.h"

#include <stdlib.h>
IsleGameResult isle_actor_source_execution_status(IsleGame* game) {
    IsleGameResult result = isle_game_status(game);
    return result == ISLE_GAME_OK && game->update_stopped ? ISLE_GAME_STOPPED : result;
}
IsleGameResult isle_actor_source_retirement_status(IsleGame* game) {
    IsleGameResult result = isle_actor_source_execution_status(game);
    if (result != ISLE_GAME_STOPPED ||
        game->returning_update_stop != ISLE_UPDATE_STOP_ACKNOWLEDGED) return result;
    return isle_game_require_idle(game);
}

int32_t isle_actor_factory_behavior(const IsleActorFactorySpawn* spawn) {
    /* Owner proves the actual authored Meowth/Valley borrow, or the exact
     * automatic Valley portability-repaired effective input, before this real
     * conditional factory executes. This never reads a Source-unwritten word. */
    return *spawn->behavior;
}
void isle_actor_source_factory_checked(const IsleActorFactoryBlock* block,
    const IsleActorFactoryBlock* reference, const IsleActorFactorySpawn* spawn,
    IsleGameResult result) {
    IsleGame* game = NULL;
    IslePlayerSource* player;
    if (result == ISLE_GAME_OK) return;
    /* This returning native leaf is gone before the live C fence transfers.
     * No Source output or receiver exists on every factory admission failure. */
    if (isle_actor_owner_factory_game(block, reference, spawn, &game) != ISLE_GAME_OK ||
        game == NULL) abort();
    /* Unlike the fallible context accessor, canonical identity is still
     * available after Game/Player has latched the failure being propagated. */
    player = game->playerSource;
    if (player == NULL || player->game != game || player->owner == NULL ||
        player->rng == NULL || player->nonwaiting_call_frame == NULL) abort();
    isle_player_source_checked(player, result);
}
typedef struct ActorFactoryCall {
    IsleActorReturningFactory returning;
    IsleActorVoidFactory nonreturning;
    int32_t object_id;
    uint16_t actor_id;
    IsleActorFactoryBlock* block;
    IsleActorFactoryBlock* reference;
    IsleActorFactorySpawn* spawn;
    IsleGameObject* object;
} ActorFactoryCall;
static void actor_factory_call(IslePlayerSource* player, void* context) {
    ActorFactoryCall* call = context;
    if (call->returning)
        call->object = call->returning(call->object_id, call->actor_id,
            call->block, call->reference, call->spawn);
    else
        call->nonreturning(call->object_id, call->actor_id,
            call->block, call->reference, call->spawn);
    /* Native leaves have returned: the returning Player C fence alone owns
     * any failure transfer, never an intervening C++ allocation frame. */
    isle_player_source_checked(player, isle_actor_source_execution_status(player->game));
}
IsleGameResult isle_actor_source_factory_call(IsleGame* game,
    IsleActorReturningFactory returning, IsleActorVoidFactory nonreturning,
    int32_t object_id, uint16_t actor_id, IsleActorFactoryBlock* block,
    IsleActorFactoryBlock* reference, IsleActorFactorySpawn* spawn,
    IsleGameObject** output) {
    IslePlayerSource* player = NULL;
    IsleGameResult result;
    ActorFactoryCall call;
    if (output) *output = NULL;
    if (!output || !block || !reference || !spawn ||
        (!!returning == !!nonreturning)) return ISLE_GAME_INVALID_ARGUMENT;
    result = isle_player_source_context(game, &player);
    if (result != ISLE_GAME_OK) return result;
    if (!player || player->game != game) return ISLE_GAME_INVALID_STATE;
    call = (ActorFactoryCall){returning, nonreturning, object_id, actor_id,
        block, reference, spawn, NULL};
    result = isle_player_source_call(player, actor_factory_call, &call);
    if (result == ISLE_GAME_OK) *output = call.object;
    return result;
}
typedef struct ActorRemoveCall {
    IsleGameObject* requester;
    const uint16_t* exceptions;
} ActorRemoveCall;
static void actor_remove_call(IslePlayerSource* player, void* context) {
    const ActorRemoveCall* call = context;
    isle_actor_remove_pokemons(call->requester, call->exceptions);
    isle_player_source_checked(player, isle_actor_source_execution_status(player->game));
}
IsleGameResult isle_actor_source_remove_pokemons_call(IsleGame* game,
    IsleGameObject* requester, const uint16_t* exceptions, size_t count) {
    IsleActorSource* actor = NULL;
    IslePlayerSource* player = NULL;
    IsleGameResult result;
    size_t i;
    int retained = 0;
    ActorRemoveCall call;
    if (!requester || !exceptions || !count) return ISLE_GAME_INVALID_ARGUMENT;
    result = isle_actor_source_execution_status(game);
    if (result != ISLE_GAME_OK) return result;
    if (!game->ticking || omCurrentObject != requester) return ISLE_GAME_INVALID_STATE;
    result = isle_actor_source_get(game, requester, &actor);
    if (result != ISLE_GAME_OK) return result;
    for (i = 0; i < count; ++i) {
        if (!exceptions[i]) break;
        if (actor->id == exceptions[i]) retained = 1;
    }
    if (i != count - 1 || !retained) return ISLE_GAME_INVALID_ARGUMENT;
    result = isle_player_source_context(game, &player);
    if (result != ISLE_GAME_OK) return result;
    call = (ActorRemoveCall){requester, exceptions};
    return isle_player_source_call(player, actor_remove_call, &call);
}


static void checked(IsleActorSource* actor, IsleGameResult result) {
    if (result != ISLE_GAME_OK)
        isle_native_execution_fault(actor->game, "native actor owner source leaf failed");
}
void isle_actor_source_level_progress(IsleGameObject* object, int32_t* block_count, float* block_part) {
    IsleActorSource* actor = isle_actor_source_require(object);
    checked(actor, isle_actor_interaction_level_progress(actor, block_count, block_part));
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
void isle_actor_source_set_animation(IsleGameObject* object, uint32_t identity, float start, int force) {
    IsleActorSource* actor = isle_actor_source_require(object);
    IsleGameResult result = isle_actor_owner_animation(actor, identity, start, force);
    checked(actor, result);
}
const IsleActorInteractionGraph* isle_actor_source_graph(IsleGameObject* object, uint32_t identity) {
    IsleActorSource* actor = isle_actor_source_require(object);
    const IsleActorInteractionGraph* graph = NULL;
    checked(actor, isle_actor_owner_graph(actor, identity, &graph));
    return graph;
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
void isle_actor_source_object_flags(IsleGameObject* object, uint32_t mask, uint32_t value) {
    IsleActorSource* actor = isle_actor_source_require(object);
    IsleGameObjectMetadata metadata;
    checked(actor, isle_game_object_metadata(actor->game, object, &metadata));
    checked(actor, isle_game_object_set_flags(actor->game, object,
        (metadata.flags & ~mask) | (value & mask)));
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
IsleGameResult isle_actor_source_id_in_use_link(IsleGame* game, uint8_t link, uint32_t id, int* output) {
    IsleGameCounts counts;
    GObj* object;
    IsleGameResult result = isle_game_counts(game, &counts);
    if (result != ISLE_GAME_OK) return result;
    if (output == NULL || link >= 32) return ISLE_GAME_INVALID_ARGUMENT;
    *output = 0;
    /* Actual ohFindByLinkAndId semantics over the configured native update
     * link, not just Owner slots. Joined identities are absent from this list. */
    for (object = omGObjListHead[link]; object != NULL; object = object->next) {
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
    return isle_game_object_set_source_update(game,object,Pokemon_Update);
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
void isle_actor_source_eat_apple(IsleGameObject* object) {
    IsleActorSource* actor = isle_actor_source_require(object);
    checked(actor,isle_actor_interaction_eat_apple(actor));
}
void isle_actor_source_send_command(IsleGameObject* object, int32_t command,
    IsleGameObject* sender) {
    IsleActorSource* actor = isle_actor_source_require(object);
    checked(actor,isle_actor_interaction_send_command(actor,command,sender));
}
void isle_actor_source_send_addressed(IsleGameObject* requester, IsleGameObject* destination,
    int32_t command, IsleGameObject* sender) {
    IsleActorSource* actor = isle_actor_source_require(requester);
    checked(actor,isle_actor_interaction_send_addressed(actor,destination,command,sender));
}
void isle_actor_source_send_to_link(IsleGameObject* object, uint8_t link,
    int32_t command, IsleGameObject* sender) {
    IsleActorSource* actor = isle_actor_source_require(object);
    checked(actor,isle_actor_interaction_send_to_link(actor,link,command,sender));
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
int32_t* isle_actor_source_scene_counter(IsleGameObject* object, uint32_t identity) {
    IsleActorSource* actor = isle_actor_source_require(object);
    int32_t* value = NULL;
    checked(actor,isle_actor_interaction_scene_counter(actor,identity,&value));
    return value;
}
IsleGameVec3 isle_actor_source_pikachu_temporary(IsleGameObject* object) {
    IsleActorSource* actor = isle_actor_source_require(object);
    IsleGameVec3 value;
    checked(actor,isle_actor_interaction_pikachu_temporary(actor,&value));
    return value;
}
const IsleActorRandomState* isle_actor_source_weighted(IsleGameObject* object, uint32_t identity) {
    IsleActorSource* actor = isle_actor_source_require(object);
    const IsleActorRandomState* value = NULL;
    checked(actor,isle_actor_interaction_weighted(actor,identity,&value));
    return value;
}

IsleGameResult isle_actor_owner_associated_player(IsleGame* game, IslePlayerSource** output) {
    IsleGameCounts counts;
    IsleGameResult result;
    if (output) *output=NULL;
    if (!game || !output) return ISLE_GAME_INVALID_ARGUMENT;
    /* counts validates the actual owner/thread without discarding a legitimate
     * association merely because Game has already latched its first fault. */
    result=isle_game_counts(game,&counts);
    if (result!=ISLE_GAME_OK) return result;
    if (!game->playerSource || game->playerSource->game!=game) return ISLE_GAME_INVALID_STATE;
    *output=game->playerSource;
    return ISLE_GAME_OK;
}
typedef IsleGameObject* (*IsleActorSixFactory)(int32_t,uint16_t,IsleActorFactoryBlock*,
    IsleActorFactoryBlock*,IsleActorFactorySpawn*,IsleActorFactoryInitialization*);
typedef struct ActorSixFactoryCall {
    IsleActorSixFactory function;
    int32_t object_id; uint16_t actor_id;
    IsleActorFactoryBlock* block; IsleActorFactoryBlock* reference;
    IsleActorFactorySpawn* spawn; IsleActorFactoryInitialization* initialization;
    IsleGameObject* object;
} ActorSixFactoryCall;
static void actor_six_factory_call(IslePlayerSource* player,void* value) {
    ActorSixFactoryCall* call=value;
    call->object=call->function(call->object_id,call->actor_id,call->block,
        call->reference,call->spawn,call->initialization);
    isle_player_source_checked(player,isle_actor_source_execution_status(player->game));
}
IsleGameResult isle_actor_source_factory_six_call(IsleGame* game,IsleActorSixFactory function,
    int32_t object_id,uint16_t actor_id,IsleActorFactoryBlock* block,IsleActorFactoryBlock* reference,
    IsleActorFactorySpawn* spawn,IsleActorFactoryInitialization* initialization,IsleGameObject** output) {
    IslePlayerSource* player=NULL; IsleGameResult result; ActorSixFactoryCall call;
    if (output) *output=NULL;
    if (!function || !block || !reference || !spawn || !initialization || !output)
        return ISLE_GAME_INVALID_ARGUMENT;
    result=isle_player_source_context(game,&player);
    if (result!=ISLE_GAME_OK) return result;
    call=(ActorSixFactoryCall){function,object_id,actor_id,block,reference,spawn,initialization,NULL};
    result=isle_player_source_call(player,actor_six_factory_call,&call);
    if (result==ISLE_GAME_OK) *output=call.object;
    return result;
}
typedef void (*IsleActorBlockChange)(IsleGameObject*,float,float,float,float,float,float);
typedef struct ActorBlockCallbackCall {
    IsleActorBlockChange function; IsleGameObject* object;
    IsleCourseActorBase* source;
    float px,py,pz,cx,cy,cz;
} ActorBlockCallbackCall;
static void actor_block_callback_body(void* value) {
    const ActorBlockCallbackCall* call=value;
    call->function(call->object,call->px,call->py,call->pz,call->cx,call->cy,call->cz);
}
static void actor_block_callback_call(IslePlayerSource* player,void* value) {
    ActorBlockCallbackCall* call=value;
    isle_player_source_checked(player,isle_course_actor_invoke(call->source,actor_block_callback_body,call));
    isle_player_source_checked(player,isle_actor_source_execution_status(player->game));
}
IsleGameResult isle_actor_source_block_callback_call(IsleGame* game,IsleCourseActorBase* source,IsleActorBlockChange function,
    IsleGameObject* object,float px,float py,float pz,float cx,float cy,float cz) {
    IslePlayerSource* player=NULL; IsleGameResult result; ActorBlockCallbackCall call;
    if (!function || !object || !source || source->nativeGame!=game) return ISLE_GAME_INVALID_ARGUMENT;
    result=isle_player_source_context(game,&player);
    if (result!=ISLE_GAME_OK) return result;
    call=(ActorBlockCallbackCall){function,object,source,px,py,pz,cx,cy,cz};
    return isle_player_source_call(player,actor_block_callback_call,&call);
}
typedef struct TunnelNativeAnimationCall {
    IsleTunnelActorSource* source; IsleTunnelAnimationCallback function;
    const IsleActorNodeRef* node; int32_t event; float argument;
} TunnelNativeAnimationCall;
static void tunnel_native_animation_call(void* value) {
    const TunnelNativeAnimationCall* call=value;
    call->function(call->source,&call->node->model,call->event,call->argument);
}
IsleGameResult isle_actor_native_tunnel_animation_event(IsleTunnelActorSource* source,
    IsleTunnelAnimationCallback function,const IsleActorNodeRef* node,int32_t event,float argument) {
    IsleTunnelActorSource* canonical=NULL;
    IsleGameModel* model=NULL; const IsleGameModelNode* nodes=NULL; size_t count=0;
    IsleGameResult result;
    TunnelNativeAnimationCall call;
    if (!source || !function || !node || !node->model.object) return ISLE_GAME_INVALID_ARGUMENT;
    result=isle_tunnel_actor_native_context(source->base.nativeGame,node->model.object,&canonical);
    if (result!=ISLE_GAME_OK) return result;
    if (canonical!=source) return ISLE_GAME_INVALID_STATE;
    result=isle_tunnel_actor_native_admission(source,source->base.nativeGame,source->services);
    if (result==ISLE_GAME_OK) result=isle_game_model_resolve(source->base.nativeGame,&node->model,&model);
    if (result==ISLE_GAME_OK) result=isle_game_model_peek_nodes(source->base.nativeGame,model,&nodes,&count);
    if (result!=ISLE_GAME_OK) return result;
    if (!nodes || node->node>=count) return ISLE_GAME_INVALID_ARGUMENT;
    call=(TunnelNativeAnimationCall){source,function,node,event,argument};
    return isle_course_actor_invoke(&source->base,tunnel_native_animation_call,&call);
}
typedef struct VolcanoNativeAnimationCall {
    IsleVolcanoActorSource* source; IsleVolcanoAnimationCallback function;
    const IsleActorNodeRef* node; int32_t event; float argument;
} VolcanoNativeAnimationCall;
static void volcano_native_animation_call(void* value) {
    const VolcanoNativeAnimationCall* call=value;
    call->function(call->source,&call->node->model,call->event,call->argument);
}
IsleGameResult isle_actor_native_volcano_animation_event(IsleVolcanoActorSource* source,
    IsleVolcanoAnimationCallback function,const IsleActorNodeRef* node,int32_t event,float argument) {
    IsleVolcanoActorSource* canonical=NULL;
    IsleGameModel* model=NULL; const IsleGameModelNode* nodes=NULL; size_t count=0;
    IsleGameResult result;
    VolcanoNativeAnimationCall call;
    if (!source || !function || !node || !node->model.object) return ISLE_GAME_INVALID_ARGUMENT;
    result=isle_volcano_actor_native_context(source->base.nativeGame,node->model.object,&canonical);
    if (result!=ISLE_GAME_OK) return result;
    if (canonical!=source) return ISLE_GAME_INVALID_STATE;
    result=isle_volcano_actor_native_admission(source,source->base.nativeGame,source->services);
    if (result==ISLE_GAME_OK) result=isle_game_model_resolve(source->base.nativeGame,&node->model,&model);
    if (result==ISLE_GAME_OK) result=isle_game_model_peek_nodes(source->base.nativeGame,model,&nodes,&count);
    if (result!=ISLE_GAME_OK) return result;
    if (!nodes || node->node>=count) return ISLE_GAME_INVALID_ARGUMENT;
    call=(VolcanoNativeAnimationCall){source,function,node,event,argument};
    return isle_course_actor_invoke(&source->base,volcano_native_animation_call,&call);
}
typedef struct RiverNativeAnimationCall {
    IsleRiverActorSource* source; IsleRiverAnimationCallback function;
    const IsleActorNodeRef* node; int32_t event; float argument;
} RiverNativeAnimationCall;
static void river_native_animation_call(void* value) {
    const RiverNativeAnimationCall* call=value;
    call->function(call->source,call->node,call->event,call->argument);
}
IsleGameResult isle_actor_native_river_animation_event(IsleRiverActorSource* source,
    IsleRiverAnimationCallback function,const IsleActorNodeRef* node,int32_t event,float argument) {
    IsleRiverActorSource* canonical=NULL;
    IsleGameModel* model=NULL; const IsleGameModelNode* nodes=NULL; size_t count=0;
    IsleGameResult result;
    RiverNativeAnimationCall call;
    if (!source || !function || !node || !node->model.object) return ISLE_GAME_INVALID_ARGUMENT;
    result=isle_river_actor_native_context(source->base.nativeGame,node->model.object,&canonical);
    if (result!=ISLE_GAME_OK) return result;
    if (canonical!=source) return ISLE_GAME_INVALID_STATE;
    result=isle_river_actor_native_admission(source,source->base.nativeGame,source->services);
    if (result==ISLE_GAME_OK) result=isle_game_model_resolve(source->base.nativeGame,&node->model,&model);
    if (result==ISLE_GAME_OK) result=isle_game_model_peek_nodes(source->base.nativeGame,model,&nodes,&count);
    if (result!=ISLE_GAME_OK) return result;
    if (!nodes || node->node>=count) return ISLE_GAME_INVALID_ARGUMENT;
    call=(RiverNativeAnimationCall){source,function,node,event,argument};
    return isle_course_actor_invoke(&source->base,river_native_animation_call,&call);
}
typedef struct CaveNativeAnimationCall {
    IsleCaveActorSource* source; IsleCaveAnimationCallback function;
    const IsleActorNodeRef* node; int32_t event; float argument;
} CaveNativeAnimationCall;
static void cave_native_animation_call(void* value) {
    const CaveNativeAnimationCall* call=value;
    call->function(call->source,&call->node->model,call->event,call->argument);
}
IsleGameResult isle_actor_native_cave_animation_event(IsleCaveActorSource* source,
    IsleCaveAnimationCallback function,const IsleActorNodeRef* node,int32_t event,float argument) {
    IsleCaveActorSource* canonical=NULL;
    IsleGameModel* model=NULL; const IsleGameModelNode* nodes=NULL; size_t count=0;
    IsleGameResult result;
    CaveNativeAnimationCall call;
    if (!source || !function || !node || !node->model.object) return ISLE_GAME_INVALID_ARGUMENT;
    result=isle_cave_actor_native_context(source->base.nativeGame,node->model.object,&canonical);
    if (result!=ISLE_GAME_OK) return result;
    if (canonical!=source) return ISLE_GAME_INVALID_STATE;
    result=isle_cave_actor_native_admission(source,source->base.nativeGame,source->services);
    if (result==ISLE_GAME_OK) result=isle_game_model_resolve(source->base.nativeGame,&node->model,&model);
    if (result==ISLE_GAME_OK) result=isle_game_model_peek_nodes(source->base.nativeGame,model,&nodes,&count);
    if (result!=ISLE_GAME_OK) return result;
    if (!nodes || node->node>=count) return ISLE_GAME_INVALID_ARGUMENT;
    call=(CaveNativeAnimationCall){source,function,node,event,argument};
    return isle_course_actor_invoke(&source->base,cave_native_animation_call,&call);
}
typedef struct ValleyNativeAnimationCall {
    IsleValleyActorSource* source; IsleValleyAnimationCallback function;
    const IsleActorNodeRef* node; int32_t event; float argument;
} ValleyNativeAnimationCall;
static void valley_native_animation_call(void* value) {
    const ValleyNativeAnimationCall* call=value;
    call->function(call->source,call->node,call->event,call->argument);
}
IsleGameResult isle_actor_native_valley_animation_event(IsleValleyActorSource* source,
    IsleValleyAnimationCallback function,const IsleActorNodeRef* node,int32_t event,float argument) {
    IsleValleyActorSource* canonical=NULL;
    IsleGameModel* model=NULL; const IsleGameModelNode* nodes=NULL; size_t count=0;
    IsleGameResult result;
    ValleyNativeAnimationCall call;
    if (!source || !function || !node || !node->model.object) return ISLE_GAME_INVALID_ARGUMENT;
    result=isle_valley_actor_native_context(source->base.nativeGame,node->model.object,&canonical);
    if (result!=ISLE_GAME_OK) return result;
    if (canonical!=source) return ISLE_GAME_INVALID_STATE;
    result=isle_valley_actor_native_admission(source,source->base.nativeGame,source->services);
    if (result==ISLE_GAME_OK) result=isle_game_model_resolve(source->base.nativeGame,&node->model,&model);
    if (result==ISLE_GAME_OK) result=isle_game_model_peek_nodes(source->base.nativeGame,model,&nodes,&count);
    if (result!=ISLE_GAME_OK) return result;
    if (!nodes || node->node>=count) return ISLE_GAME_INVALID_ARGUMENT;
    call=(ValleyNativeAnimationCall){source,function,node,event,argument};
    return isle_course_actor_invoke(&source->base,valley_native_animation_call,&call);
}

typedef struct ActorAddGeoCall {
    IsleGameObject* parent; uint16_t requested; const IsleActorGeoDefinition* definition;
    IsleGameObject* object;
} ActorAddGeoCall;
static void actor_add_geo_call(IslePlayerSource* player,void* value) {
    ActorAddGeoCall* call=value;
    call->object=Pokemon_AddAtGeo(call->parent,call->requested,call->definition);
    isle_player_source_checked(player,isle_actor_source_execution_status(player->game));
}
IsleGameResult isle_actor_source_add_geo_call(IsleGame* game,IsleGameObject* parent,
    uint16_t requested,const IsleActorGeoDefinition* definition,IsleGameObject** output) {
    IslePlayerSource* player=NULL; IsleGameResult result; ActorAddGeoCall call;
    if (output) *output=NULL;
    if (!parent || !definition || !output) return ISLE_GAME_INVALID_ARGUMENT;
    result=isle_player_source_context(game,&player);
    if (result!=ISLE_GAME_OK) return result;
    call=(ActorAddGeoCall){parent,requested,definition,NULL};
    result=isle_player_source_call(player,actor_add_geo_call,&call);
    if (result==ISLE_GAME_OK) *output=call.object;
    return result;
}
