#ifndef ISLE_PLAYER_CART_SOURCE_H
#define ISLE_PLAYER_CART_SOURCE_H

#include "player_source.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Independent, Owner-issued borrowed list identities. These are not guest
 * pointers, SDK layouts or arbitrary animation spans. Outer NULL preserves the
 * complete corresponding domain; an absent stream inside a supplied list does
 * not mean outer NULL. */
typedef struct IslePlayerCartModelAnimation IslePlayerCartModelAnimation;
typedef struct IslePlayerCartMaterialAnimation IslePlayerCartMaterialAnimation;

struct IslePlayerCartEvent;
typedef struct IslePlayerCartSource {
    IslePlayerSource* player;
    void* owner;
    IsleGameObject* cartGObj;
    /* Borrow of the actual batch event only during its fenced C callback. */
    const struct IslePlayerCartEvent* current_event;
} IslePlayerCartSource;

/* An actual native model incarnation and node index, never a pose copy. */
typedef struct IslePlayerCartNode {
    IsleGameModelRef binding;
    size_t node;
} IslePlayerCartNode;

typedef void (*IslePlayerCartCallback)(void* user,
    const IslePlayerCartNode*, int32_t event, float argument);

typedef struct IslePlayerCartEvent {
    IslePlayerCartNode node;
    int32_t event;
    float argument;
} IslePlayerCartEvent;

/* Successful step acquires a real event lease even for an empty batch. The
 * backend keeps this ordered storage, callback and node identities stable until
 * finish_events. Reentrant mutation of this Cart Runtime must reject while the
 * lease is open. Events/nodes are borrowed only for the duration of dispatch. */
typedef struct IslePlayerCartEventBatch {
    const IslePlayerCartEvent* events;
    size_t count;
    IslePlayerCartCallback callback;
    void* user;
    uint64_t lease;
} IslePlayerCartEventBatch;

/* Complete original C bodies, generated privately from the admitted GAME pin.
 * Invoke from original C or through isle_player_source_call, never around an
 * owning C++ frame that a checked failure could cross. */
IsleGameObject* isle_player_cart_PlayerModel_Init(IslePlayerCartSource*);
void isle_player_cart_PlayerModel_SetAnimation(IslePlayerCartSource*,
    const IslePlayerCartModelAnimation*, const IslePlayerCartMaterialAnimation*,
    float start_time, float speed);
void isle_player_cart_PlayerModel_ChangeAnimation(IslePlayerCartSource*,
    const IslePlayerCartModelAnimation*, const IslePlayerCartMaterialAnimation*,
    float start_time);
void isle_player_cart_PlayerModel_SetAnimationSpeed(IslePlayerCartSource*, float speed);

/* The one actual source kind-1 / priority-1 process address. This C frame runs
 * callbacks only after the returning C++ step leaf has completely unwound. */
void isle_player_cart_source_update(IsleGameObject*);
void isle_player_cart_source_set_callback(IslePlayerCartSource*, IsleGameObject*,
    IslePlayerCartCallback, void* user);


/* Returning C entry fences for Owner calls; their internal thunks and all
 * frames between the fence and checked source leaves are ordinary C. */
IsleGameResult isle_player_cart_source_call_init(IslePlayerCartSource*, IsleGameObject**);
IsleGameResult isle_player_cart_source_call_set(IslePlayerCartSource*,
    const IslePlayerCartModelAnimation*, const IslePlayerCartMaterialAnimation*,
    float start_time, float speed);
IsleGameResult isle_player_cart_source_call_change(IslePlayerCartSource*,
    const IslePlayerCartModelAnimation*, const IslePlayerCartMaterialAnimation*,
    float start_time);
IsleGameResult isle_player_cart_source_call_speed(IslePlayerCartSource*, float speed);
IsleGameResult isle_player_cart_source_call_callback(IslePlayerCartSource*, IsleGameObject*,
    IslePlayerCartCallback, void* user);
/* Returning C++ backend leaves. None may checked-transfer or invoke source
 * callbacks. All failures are checked by C after the leaf returns. Only
 * try_object's prepared-capacity EXHAUSTED is normal NULL admission; all other
 * failures, including stale descriptors and wrong context, are source faults.
 * A failed step leaves no event lease; a successful step always requires finish.
 * finish_events must attempt all release/deferred-retirement work and remain
 * callable after the canonical Player Source records a callback failure. */
IsleGameResult isle_player_cart_backend_try_object(IslePlayerCartSource*, IsleGameObject**);
IsleGameResult isle_player_cart_backend_link_render(IslePlayerCartSource*, IsleGameObject*);
IsleGameResult isle_player_cart_backend_attach_model(IslePlayerCartSource*, IsleGameObject*);
IsleGameResult isle_player_cart_backend_publish_animation_process(IslePlayerCartSource*,
    IsleGameObject*, IsleGameSourceCallback, int32_t kind, int32_t priority);
IsleGameResult isle_player_cart_backend_finish_init(IslePlayerCartSource*, IsleGameObject*);
IsleGameResult isle_player_cart_backend_preflight_requested_animations(IslePlayerCartSource*,
    IsleGameObject*, const IslePlayerCartModelAnimation*,
    const IslePlayerCartMaterialAnimation*, float start_time,
    int32_t change_speed, float speed);
IsleGameResult isle_player_cart_backend_bind_model(IslePlayerCartSource*, IsleGameObject*,
    const IslePlayerCartModelAnimation*, float start_time);
IsleGameResult isle_player_cart_backend_bind_material(IslePlayerCartSource*, IsleGameObject*,
    const IslePlayerCartMaterialAnimation*, float start_time);
IsleGameResult isle_player_cart_backend_set_model_speed(IslePlayerCartSource*,
    IsleGameObject*, float speed);
IsleGameResult isle_player_cart_backend_set_material_speed(IslePlayerCartSource*,
    IsleGameObject*, float speed);
IsleGameResult isle_player_cart_backend_step(IslePlayerCartSource*, IsleGameObject*,
    IslePlayerCartEventBatch*);
IsleGameResult isle_player_cart_backend_preflight_speed(IslePlayerCartSource*,
    IsleGameObject*, float speed);
IsleGameResult isle_player_cart_backend_finish_events(IslePlayerCartSource*, IsleGameObject*,
    const IslePlayerCartEventBatch*);
IsleGameResult isle_player_cart_backend_set_callback(IslePlayerCartSource*, IsleGameObject*,
    IslePlayerCartCallback, void* user);

#ifdef __cplusplus
}
#endif
#endif
