#ifndef ISLE_TEST_ACTOR_MOVEMENT_PROBE_H
#define ISLE_TEST_ACTOR_MOVEMENT_PROBE_H
#include "actor_owner_source.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum ActorMovementOperation {
    ACTOR_MOVE_XZ, ACTOR_JUMP, ACTOR_JUMP_HEIGHT, ACTOR_FALL, ACTOR_FALL_HEIGHT,
    ACTOR_FALL_GROUND, ACTOR_TURN, ACTOR_TURN_TARGET, ACTOR_TURN_NODE,
    ACTOR_RUN_TARGET, ACTOR_RUN_AWAY, ACTOR_STEP_TARGET, ACTOR_RUN_POSITION,
    ACTOR_PATH, ACTOR_PATH_SAMPLE, ACTOR_RESET_PATH, ACTOR_SCALE, ACTOR_GROUND,
    ACTOR_STEP_DIRECTION, ACTOR_TURN_FLUTE, ACTOR_CIRCLES, ACTOR_CIRCLES_DEFAULT,
    ACTOR_HEADING_VECTOR
} ActorMovementOperation;
/* Caller-owned POD survives every wait, native fault and cancellation/join.
 * Inputs are diagnostic scalars, not species states or a path provider. */
typedef struct ActorMovementProbe {
    IsleGame* game;
    IsleGameObject* object;
    IsleGameProcess* process;
    ActorMovementOperation operation;
    float a, b, c, d;
    uint32_t flags;
    const void* path;
    IsleActorNodeRef node;
    int entered, returned, result;
    float scalar;
    IsleGameVec3 vector;
    IsleGameVec3 point_to, point_from;
    IsleActorGround ground;
} ActorMovementProbe;
IsleGameResult actor_movement_probe_start(ActorMovementProbe*, IsleActorSource*);
void actor_movement_probe_entry(IsleGameObject*);
#ifdef __cplusplus
}
#endif
#endif
