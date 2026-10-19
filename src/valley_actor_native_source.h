#ifndef ISLE_VALLEY_ACTOR_NATIVE_SOURCE_H
#define ISLE_VALLEY_ACTOR_NATIVE_SOURCE_H
#include "valley_actor_source.h"
#include "player_cart_source.h"
#ifdef __cplusplus
extern "C" {
#endif
IsleGameResult isle_valley_native_child(IsleValleyActorSource*,IsleGameObject*,int,IsleGameObject**);
IsleGameResult isle_valley_native_cart_event_admit(IsleValleyActorSource*,const IslePlayerCartNode*,IsleValleyAnimationCallback,int32_t,float);
void isle_valley_native_ceremony_cart_event(void*,const IslePlayerCartNode*,int32_t,float);
void isle_valley_native_gate_cart_event(void*,const IslePlayerCartNode*,int32_t,float);
// The actual shared waiting Original C definition. Never call it from C++.
void Pokemon_JumpAndBounceFromGround(IsleGameObject*,float,float,float);
#ifdef __cplusplus
}
#endif
#endif
