#include "player_cart_source.h"
#include "engine_internal.h"

/* These helpers return from every native leaf before a checked transfer. They
 * are ordinary C frames, including when entered by an Owner's local C fence. */
static IsleGameObject* cart_try_object(IslePlayerCartSource* source) {
    IsleGameObject* object = NULL;
    IsleGameResult result = isle_player_cart_backend_try_object(source, &object);
    if (result == ISLE_GAME_EXHAUSTED && object == NULL) return NULL;
    isle_player_source_checked(source->player, result);
    if (object == NULL)
        isle_player_source_checked(source->player, ISLE_GAME_INVALID_STATE);
    return object;
}

static void cart_link_render(IslePlayerCartSource* source, IsleGameObject* object) {
    isle_player_source_checked(source->player,
        isle_player_cart_backend_link_render(source, object));
}

static void cart_attach_model(IslePlayerCartSource* source, IsleGameObject* object) {
    isle_player_source_checked(source->player,
        isle_player_cart_backend_attach_model(source, object));
}

static void cart_publish_animation_process(IslePlayerCartSource* source,
    IsleGameObject* object) {
    isle_player_source_checked(source->player,
        isle_player_cart_backend_publish_animation_process(source, object,
            isle_player_cart_source_update, 1, 1));
}

static void cart_finish_init(IslePlayerCartSource* source, IsleGameObject* object) {
    isle_player_source_checked(source->player,
        isle_player_cart_backend_finish_init(source, object));
}

static void cart_preflight_requested_animations(IslePlayerCartSource* source,
    const IslePlayerCartModelAnimation* model,
    const IslePlayerCartMaterialAnimation* material, float start_time,
    int32_t change_speed, float speed) {
    /* Outer NULL / NULL is source's actual no-op, not an empty-table bind. In
     * particular, its unconsumed NaN/Inf scalars must not invent a new failure. */
    if (model == NULL && material == NULL) return;
    isle_player_source_checked(source->player,
        isle_player_cart_backend_preflight_requested_animations(source,
            source->cartGObj, model, material, start_time, change_speed, speed));
}

static void cart_bind_model(IslePlayerCartSource* source, IsleGameObject* object,
    const IslePlayerCartModelAnimation* animation, float start_time) {
    isle_player_source_checked(source->player,
        isle_player_cart_backend_bind_model(source, object, animation, start_time));
}

static void cart_bind_material(IslePlayerCartSource* source, IsleGameObject* object,
    const IslePlayerCartMaterialAnimation* animation, float start_time) {
    isle_player_source_checked(source->player,
        isle_player_cart_backend_bind_material(source, object, animation, start_time));
}

static void cart_preflight_speed(IslePlayerCartSource* source, float speed) {
    isle_player_source_checked(source->player,
        isle_player_cart_backend_preflight_speed(source, source->cartGObj, speed));
}

static void cart_set_model_speed(IslePlayerCartSource* source, IsleGameObject* object,
    float speed) {
    isle_player_source_checked(source->player,
        isle_player_cart_backend_set_model_speed(source, object, speed));
}

static void cart_set_material_speed(IslePlayerCartSource* source, IsleGameObject* object,
    float speed) {
    isle_player_source_checked(source->player,
        isle_player_cart_backend_set_material_speed(source, object, speed));
}

/* Four complete guarded own-GAME bodies, generated only into private build
 * storage. No SDK header, SDK body, guest layout or raw ROM code is imported. */
#include "player_cart_original.inc"

typedef struct CartCallbackCall {
    IslePlayerCartCallback callback;
    void* user;
    const IslePlayerCartEvent* event;
} CartCallbackCall;

static void cart_dispatch_callback(IslePlayerSource* player, void* user) {
    const CartCallbackCall* call = user;
    (void) player;
    call->callback(call->user, &call->event->node,
        call->event->event, call->event->argument);
}

void isle_player_cart_source_update(IsleGameObject* object) {
    IslePlayerSource* player = isle_player_source_current(object);
    IslePlayerCartSource* source = player->cart;
    IslePlayerCartEventBatch batch = {0};
    IsleGameResult first, released;
    size_t index;
    if (source == NULL || source->player != player || source->owner == NULL)
        isle_player_source_checked(player, ISLE_GAME_INVALID_STATE);
    /* Step owns no source callback frame: all Runtime/native C++ work has
     * returned before dispatch can transfer through its local C-only fence. */
    isle_player_source_checked(player,
        isle_player_cart_backend_step(source, object, &batch));
    first = ISLE_GAME_OK;
    if (batch.count != 0 && batch.events == NULL) first = ISLE_GAME_INVALID_STATE;
    if (first == ISLE_GAME_OK && batch.callback != NULL) {
        for (index = 0; index < batch.count; ++index) {
            CartCallbackCall call = {batch.callback, batch.user, &batch.events[index]};
            if (source->current_event != NULL) { first = ISLE_GAME_INVALID_STATE; break; }
            source->current_event = call.event;
            first = isle_player_source_call(player, cart_dispatch_callback, &call);
            source->current_event = NULL;
            if (first != ISLE_GAME_OK) break;
        }
    }
    /* Even callback failure must close the event lease and attempt deferred
     * native/Runtime retirement before failure leaves this plain C frame.
     * The first failure wins, without clearing canonical sticky Source status. */
    released = isle_player_cart_backend_finish_events(source, object, &batch);
    if (first == ISLE_GAME_OK) first = released;
    isle_player_source_checked(player, first);
}

void isle_player_cart_source_set_callback(IslePlayerCartSource* source,
    IsleGameObject* object, IslePlayerCartCallback callback, void* user) {
    isle_player_source_checked(source->player,
        isle_player_cart_backend_set_callback(source, object, callback, user));
}

typedef enum CartCallKind {
    CART_CALL_INIT, CART_CALL_SET, CART_CALL_CHANGE, CART_CALL_SPEED, CART_CALL_CALLBACK
} CartCallKind;

typedef struct CartCall {
    IslePlayerCartSource* source;
    CartCallKind kind;
    const IslePlayerCartModelAnimation* model;
    const IslePlayerCartMaterialAnimation* material;
    float start_time, speed;
    IsleGameObject* object;
    IslePlayerCartCallback callback;
    void* user;
} CartCall;

static void cart_call_body(IslePlayerSource* player, void* user) {
    CartCall* call = user;
    (void) player;
    switch (call->kind) {
    case CART_CALL_INIT:
        call->object = isle_player_cart_PlayerModel_Init(call->source);
        break;
    case CART_CALL_SET:
        isle_player_cart_PlayerModel_SetAnimation(call->source, call->model,
            call->material, call->start_time, call->speed);
        break;
    case CART_CALL_CHANGE:
        isle_player_cart_PlayerModel_ChangeAnimation(call->source, call->model,
            call->material, call->start_time);
        break;
    case CART_CALL_SPEED:
        isle_player_cart_PlayerModel_SetAnimationSpeed(call->source, call->speed);
        break;
    case CART_CALL_CALLBACK:
        isle_player_cart_source_set_callback(call->source, call->object,
            call->callback, call->user);
        break;
    }
}

static IsleGameResult cart_call(IslePlayerCartSource* source, CartCall* call) {
    IslePlayerSource* active = NULL;
    IsleGameResult result;
    if (source == NULL || source->player == NULL || source->owner == NULL)
        return ISLE_GAME_INVALID_ARGUMENT;
    result = isle_player_source_context(source->player->game, &active);
    if (result != ISLE_GAME_OK) return result;
    if (active != source->player || active->cart != source) return ISLE_GAME_INVALID_STATE;
    call->source = source;
    return isle_player_source_call(active, cart_call_body, call);
}

IsleGameResult isle_player_cart_source_call_init(IslePlayerCartSource* source,
    IsleGameObject** output) {
    CartCall call = {0};
    IsleGameResult result;
    if (output == NULL) return ISLE_GAME_INVALID_ARGUMENT;
    *output = NULL;
    call.kind = CART_CALL_INIT;
    result = cart_call(source, &call);
    if (result == ISLE_GAME_OK) *output = call.object;
    return result;
}

IsleGameResult isle_player_cart_source_call_set(IslePlayerCartSource* source,
    const IslePlayerCartModelAnimation* model, const IslePlayerCartMaterialAnimation* material,
    float start_time, float speed) {
    CartCall call = {0};
    call.kind = CART_CALL_SET;
    call.model = model;
    call.material = material;
    call.start_time = start_time;
    call.speed = speed;
    return cart_call(source, &call);
}

IsleGameResult isle_player_cart_source_call_change(IslePlayerCartSource* source,
    const IslePlayerCartModelAnimation* model, const IslePlayerCartMaterialAnimation* material,
    float start_time) {
    CartCall call = {0};
    call.kind = CART_CALL_CHANGE;
    call.model = model;
    call.material = material;
    call.start_time = start_time;
    return cart_call(source, &call);
}

IsleGameResult isle_player_cart_source_call_speed(IslePlayerCartSource* source, float speed) {
    CartCall call = {0};
    call.kind = CART_CALL_SPEED;
    call.speed = speed;
    return cart_call(source, &call);
}

IsleGameResult isle_player_cart_source_call_callback(IslePlayerCartSource* source,
    IsleGameObject* object, IslePlayerCartCallback callback, void* user) {
    CartCall call = {0};
    call.kind = CART_CALL_CALLBACK;
    call.object = object;
    call.callback = callback;
    call.user = user;
    return cart_call(source, &call);
}
