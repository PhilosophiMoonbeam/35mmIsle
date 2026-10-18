#include "beach_meowth_source.h"
#include "engine_internal.h"

IsleGameVec3 isle_beach_meowth_scale(IsleGameObject* object,
                                   const IsleGameModelRef* model) {
    IsleActorSource* actor = isle_actor_source_require(object);
    const IsleGameModelTransform* transform = NULL;
    const IsleGameResult result = isle_actor_owner_aux_read(actor, model, &transform);
    if (result != ISLE_GAME_OK)
        isle_native_execution_fault(actor->game, "Meowth captured model scale read failed");
    return transform->scale;
}

typedef struct IsleBeachBlockCall {
    jmp_buf failure;
    void* previous;
    volatile IsleGameResult* result;
} IsleBeachBlockCall;

static void block_checked(IsleActorSource* actor, IsleGameResult result) {
    if (result == ISLE_GAME_OK) return;
    if (actor->block_change_call_frame != NULL) {
        IsleBeachBlockCall* frame = actor->block_change_call_frame;
        *frame->result = result;
        longjmp(frame->failure, 1);
    }
    isle_native_execution_fault(actor->game, "original block relocation native leaf failed");
}

IsleActorSource* isle_beach_block_source(IsleGameObject* object) {
    IsleActorSource* actor;
    IsleActorSource* checked = NULL;
    if (object == NULL || object->sourcePayloadKind != ISLE_SOURCE_PAYLOAD_ACTOR ||
        object->userData == NULL)
        isle_native_execution_fault(object != NULL ? object->game : NULL,
                                    "original block relocation requires canonical actor");
    actor = object->userData;
    block_checked(actor, isle_actor_source_get(object->game, object, &checked));
    if (checked != actor) block_checked(actor, ISLE_GAME_INVALID_STATE);
    if (actor->block_change_call_frame == NULL &&
        (!actor->game->ticking || omCurrentObject == NULL))
        block_checked(actor, ISLE_GAME_INVALID_STATE);
    return actor;
}

IsleBeachBlockModel isle_beach_block_model(IsleGameObject* object, int require_auxiliary) {
    IsleActorSource* actor = isle_beach_block_source(object);
    IsleBeachBlockModel value = {0};
    IsleGameModel* model = NULL;
    const IsleGameModelPlacement* placement = NULL;
    block_checked(actor, isle_actor_owner_self_model(actor, &value.model));
    block_checked(actor, isle_game_model_resolve(actor->game, &value.model, &model));
    block_checked(actor, isle_game_model_placement(actor->game, model, &placement));
    value.auxiliary = placement->mode != ISLE_GAME_MODEL_PLACEMENT_DISABLED;
    value.translation_slots = value.auxiliary ? 1u : 0u;
    if (require_auxiliary && !value.auxiliary)
        block_checked(actor, ISLE_GAME_INVALID_STATE);
    return value;
}

IsleGameVec3 isle_beach_block_position(IsleGameObject* object,
    const IsleBeachBlockModel* model, int auxiliary) {
    IsleActorSource* actor = isle_beach_block_source(object);
    if (auxiliary) {
        const IsleGameModelTransform* transform = NULL;
        block_checked(actor, isle_actor_owner_aux_read(actor, &model->model, &transform));
        return transform->position;
    } else {
        IsleActorPositionRef reference = {model->model, ISLE_ACTOR_POSITION_ROOT};
        const IsleGameVec3* position = NULL;
        block_checked(actor, isle_actor_owner_position_read(actor, &reference, &position));
        return *position;
    }
}

void isle_beach_block_store(IsleGameObject* object, const IsleBeachBlockModel* model,
    int auxiliary, IsleGameVec3 position) {
    IsleActorSource* actor = isle_beach_block_source(object);
    if (auxiliary) {
        IsleGameModelTransform values;
        values.position = position;
        block_checked(actor, isle_actor_owner_aux_store(actor, &model->model,
            ISLE_ACTOR_PX | ISLE_ACTOR_PY | ISLE_ACTOR_PZ, &values));
    } else {
        block_checked(actor, isle_actor_owner_root_position_store(actor, &model->model, &position));
    }
}

int32_t isle_beach_block_ground(IsleGameObject* object, float x, float z,
    IsleActorGround* ground) {
    IsleActorSource* actor = isle_beach_block_source(object);
    int32_t found = 0;
    block_checked(actor, isle_actor_owner_ground(actor, x, z, ground, &found));
    return found;
}

int32_t isle_beach_block_passed(IsleGameObject* object) {
    IsleActorSource* actor = isle_beach_block_source(object);
    int32_t current = 0, base = 0;
    block_checked(actor, isle_actor_owner_block_indices(actor, &current, &base));
    return current > base;
}

void isle_beach_block_run_cleanup(IsleGameObject* object) {
    IsleActorSource* actor = isle_beach_block_source(object);
    block_checked(actor, isle_actor_owner_schedule_cleanup(actor));
}

IsleGameResult isle_meowth_source_call_change_block(IsleGameObject* object,
    float px, float py, float pz, float cx, float cy, float cz, int ground) {
    IsleActorSource* actor = NULL;
    IsleBeachBlockCall frame;
    volatile IsleGameResult result = ISLE_GAME_OK;
    IsleGameResult initial;
    if (object == NULL) return ISLE_GAME_INVALID_ARGUMENT;
    initial = isle_actor_source_get(object->game, object, &actor);
    if (initial != ISLE_GAME_OK) return initial;
    frame.previous = actor->block_change_call_frame;
    frame.result = &result;
    actor->block_change_call_frame = &frame;
    if (setjmp(frame.failure) == 0) {
        if (ground < 0) func_beach_802C71E8(object, px, py, pz, cx, cy, cz);
        else if (ground == 2) Pokemon_ChangeBlockAndRemove(object, px, py, pz, cx, cy, cz);
        else if (ground) pokemonChangeBlockOnGround(object, px, py, pz, cx, cy, cz);
        else pokemonChangeBlock(object, px, py, pz, cx, cy, cz);
    }
    actor->block_change_call_frame = frame.previous;
    return result;
}
