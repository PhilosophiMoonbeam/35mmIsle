#ifndef ISLE_RIVER_ACTOR_NATIVE_SOURCE_H
#define ISLE_RIVER_ACTOR_NATIVE_SOURCE_H
#include "river_actor_source.h"
#include "player_cart_source.h"
#ifdef __cplusplus
extern "C" {
#endif
IsleGameResult isle_river_native_cart_event_admit(IsleRiverActorSource*,const IslePlayerCartNode*,IslePlayerCartCallback);
IsleGameResult isle_actor_native_river_animation_event(IsleRiverActorSource*,IsleRiverAnimationCallback,const IsleActorNodeRef*,int32_t,float);
void isle_river_native_gate_cart_event(void*,const IslePlayerCartNode*,int32_t,float);
void isle_river_native_outro_cart_event(void*,const IslePlayerCartNode*,int32_t,float);
#ifdef __cplusplus
}
#endif
#endif
