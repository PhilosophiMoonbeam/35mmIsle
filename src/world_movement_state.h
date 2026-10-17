#ifndef ISLE_WORLD_MOVEMENT_STATE_H
#define ISLE_WORLD_MOVEMENT_STATE_H

#include "35mmisle/engine.h"

/* Canonical state shared by original C Player code and native World movement.
 * The retained World owns block; C must not dereference the opaque identity.
 * Neither this record nor its block identity survives World replacement. */
typedef struct IsleWorldMovementState {
    float move_time, control_time;
    void* block;
    IsleGameVec3 position, rotation;
    float move_speed, speed_multiplier;
} IsleWorldMovementState;

#define ISLE_WORLD_MOVEMENT_STATE_INITIALIZER \
    {0.0f, 0.5f, NULL, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 0.0f, 1.0f}

#endif
