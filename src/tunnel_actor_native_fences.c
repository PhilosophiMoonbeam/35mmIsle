#include "tunnel_actor_native_fences.h"
#include "player_source.h"
#include "screen_transition_source.h"

typedef struct SpawnCall { IsleGameObject* object; IsleGameObject* result; } SpawnCall;
static void spawn_call(void* argument) {
    SpawnCall* p = argument;
    p->result = func_8035E1D4_4FE5E4(p->object);
}
IsleGameResult isle_tunnel_native_spawn_effect(IsleTunnelActorSource* s,IsleGameObject* object,IsleGameObject** out) {
    SpawnCall p = {object,NULL};
    IsleGameResult result;
    if (!out || !object) return ISLE_GAME_INVALID_ARGUMENT;
    *out = NULL;
    result = isle_tunnel_actor_invoke(s,spawn_call,&p);
    if (result == ISLE_GAME_OK) *out = p.result;
    return result;
}
typedef struct CollisionCall {
    IsleTunnelActorSource* source;
    IsleTunnelCollisionCallback callback;
    IsleGameVec3 *a,*b,*c,*d;
    int32_t hit;
} CollisionCall;
static void collision_call(void* argument) {
    CollisionCall* p = argument;
    p->hit = p->callback(p->source,p->a,p->b,p->c,p->d);
}
IsleGameResult isle_tunnel_native_collision(IsleTunnelActorSource* s,IsleTunnelCollisionCallback callback,
    IsleGameVec3* a,IsleGameVec3* b,IsleGameVec3* c,IsleGameVec3* d,int32_t* hit) {
    CollisionCall p = {s,callback,a,b,c,d,0};
    IsleGameResult result;
    if (!callback || !a || !b || !c || !d || !hit) return ISLE_GAME_INVALID_ARGUMENT;
    result = isle_tunnel_actor_invoke(s,collision_call,&p);
    if (result == ISLE_GAME_OK) *hit = p.hit;
    return result;
}
typedef struct TransitionCall {
    IsleTunnelActorSource* source;
    uint32_t operation;
    int32_t a,b,c,result;
    float duration;
} TransitionCall;
static void transition_call(void* argument) {
    TransitionCall* p = argument;
    IslePlayerSource* player = NULL;
    IsleGameResult result = isle_player_source_context(p->source->base.nativeGame,&player);
    isle_tunnel_actor_checked(p->source,result);
    if (!player || !player->screen) isle_tunnel_actor_checked(p->source,ISLE_GAME_INVALID_STATE);
    switch (p->operation) {
    case 0: p->result = isle_screen_transition_func_800A7460(player->screen); break;
    case 1: result = isle_screen_transition_func_800A7470(player->screen,p->a,p->b,p->c); break;
    case 2: result = isle_screen_transition_func_800A7918(player->screen,p->a,p->duration); break;
    case 3: result = isle_screen_transition_func_800A7F68(player->screen,p->a,p->b); break;
    default: result = ISLE_GAME_INVALID_ARGUMENT; break;
    }
    isle_tunnel_actor_checked(p->source,result);
}
IsleGameResult isle_tunnel_native_transition(IsleTunnelActorSource* s,uint32_t operation,
    int32_t a,int32_t b,int32_t c,float duration,int32_t* out) {
    TransitionCall p = {s,operation,a,b,c,0,duration};
    IsleGameResult result = isle_tunnel_actor_invoke(s,transition_call,&p);
    if (result == ISLE_GAME_OK && out) *out = p.result;
    return result;
}
typedef struct DeleteCall { IsleTunnelActorSource* source; IsleGameObject* object; } DeleteCall;
static void delete_call(void* argument) {
    DeleteCall* p = argument;
    isle_tunnel_actor_checked(p->source,isle_game_delete_object(p->source->base.nativeGame,p->object));
}
IsleGameResult isle_tunnel_native_delete(IsleTunnelActorSource* s,IsleGameObject* object) {
    DeleteCall p = {s,object};
    if (!object) return ISLE_GAME_INVALID_ARGUMENT;
    return isle_tunnel_actor_invoke(s,delete_call,&p);
}
