#ifndef ISLE_TUNNEL_ACTOR_NATIVE_FENCES_H
#define ISLE_TUNNEL_ACTOR_NATIVE_FENCES_H
#include "tunnel_actor_source.h"
#ifdef __cplusplus
extern "C" {
#endif
IsleGameResult isle_tunnel_native_spawn_effect(IsleTunnelActorSource*,IsleGameObject*,IsleGameObject**);
IsleGameResult isle_tunnel_native_collision(IsleTunnelActorSource*,IsleTunnelCollisionCallback,IsleGameVec3*,IsleGameVec3*,IsleGameVec3*,IsleGameVec3*,int32_t*);
IsleGameResult isle_tunnel_native_transition(IsleTunnelActorSource*,uint32_t,int32_t,int32_t,int32_t,float,int32_t*);
IsleGameResult isle_tunnel_native_delete(IsleTunnelActorSource*,IsleGameObject*);
#ifdef __cplusplus
}
#endif
#endif
