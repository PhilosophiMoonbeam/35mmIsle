#include "actor_movement_probe.h"
#include "engine_internal.h"

static ActorMovementProbe* active;
IsleGameResult actor_movement_probe_start(ActorMovementProbe* probe, IsleActorSource* source) {
    if (probe == NULL || source == NULL) return ISLE_GAME_INVALID_ARGUMENT;
    probe->game = source->game;
    probe->object = source->object;
    probe->entered = probe->returned = 0;
    probe->result = -1;
    active = probe;
    return isle_game_add_source_process(source->game, source->object,
        actor_movement_probe_entry, ISLE_GAME_STACKFUL, 7, &probe->process);
}
void actor_movement_probe_entry(IsleGameObject* object) {
    ActorMovementProbe* probe = active;
    if (probe == NULL || probe->object != object || probe->game != object->game ||
        omCurrentObject != object || omCurrentProcess == NULL ||
        omCurrentProcess->object != object || omCurrentProcess->function != actor_movement_probe_entry)
        isle_native_execution_fault(object->game, "movement probe lost native C process context");
    ++probe->entered;
    switch (probe->operation) {
    case ACTOR_MOVE_XZ: Pokemon_MoveXZ(object); break;
    case ACTOR_JUMP: Pokemon_Jump(object, probe->a, probe->b, probe->c, probe->d); break;
    case ACTOR_JUMP_HEIGHT: Pokemon_JumpToHeight(object, probe->a, probe->b, probe->c, probe->d); break;
    case ACTOR_FALL: Pokemon_Fall(object, probe->a, probe->b, probe->c, probe->d); break;
    case ACTOR_FALL_HEIGHT: Pokemon_FallToHeight(object, probe->a, probe->b, probe->c, probe->d); break;
    case ACTOR_FALL_GROUND: Pokemon_FallDownOnGround(object, probe->a, (int32_t)probe->flags); break;
    case ACTOR_TURN: probe->result = Pokemon_Turn(object, isle_actor_source_self_model(object), probe->a, probe->b); break;
    case ACTOR_TURN_TARGET: Pokemon_TurnToTarget(object, probe->a, probe->flags); break;
    case ACTOR_TURN_NODE: Pokemon_TurnToModelNode(object, probe->node, probe->a, probe->flags); break;
    case ACTOR_RUN_TARGET: Pokemon_RunToTarget(object, probe->a, probe->b, probe->flags); break;
    case ACTOR_RUN_AWAY: Pokemon_RunAwayFromTarget(object, probe->a, probe->b, probe->flags); break;
    case ACTOR_STEP_TARGET: probe->result = Pokemon_StepToTargetPos(object, probe->a, probe->flags); break;
    case ACTOR_RUN_POSITION: Pokemon_RunToTargetPos(object, probe->a); break;
    case ACTOR_PATH: Pokemon_FollowPath(object, probe->a, probe->b, probe->c, probe->d, probe->flags); break;
    case ACTOR_PATH_SAMPLE: func_80361FBC_5023CC(object, &probe->vector, probe->path, probe->a); break;
    case ACTOR_RESET_PATH: Pokemon_ResetPathPos(object); break;
    case ACTOR_SCALE: Pokemon_SetScale(object, probe->a); break;
    case ACTOR_GROUND:
        probe->scalar = Pokemon_GetGroundAt(object, probe->a, probe->b);
        probe->result = isle_actor_source_ground(object, probe->a, probe->b, &probe->ground);
        break;
    case ACTOR_STEP_DIRECTION: probe->result = Pokemon_StepWalkInDirection(object, probe->a, probe->flags); break;
    case ACTOR_TURN_FLUTE: Pokemon_TurnToPokeFlute(object, probe->a); break;
    case ACTOR_CIRCLES: Pokemon_RunInCircles(object, probe->a, probe->b, (int32_t)probe->flags); break;
    case ACTOR_CIRCLES_DEFAULT: Pokemon_RunInCirclesDefault(object, probe->a); break;
    case ACTOR_HEADING_VECTOR:
        probe->scalar = Pokemon_GetHeadingVector(&probe->point_to, &probe->point_from, &probe->vector, probe->a);
        break;
    default: isle_native_execution_fault(object->game, "invalid movement diagnostic operation");
    }
    ++probe->returned;
    omEndProcess(NULL);
}
