#ifndef ISLE_PLAYER_CART_OWNER_RSP_FIXTURE_H
#define ISLE_PLAYER_CART_OWNER_RSP_FIXTURE_H
#include "player_cart_source.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct PlayerCartRspInit {
    IslePlayerCartSource* source;
    IsleGameObject* object;
    IsleGameResult status;
    unsigned entered,returned;
} PlayerCartRspInit;
void player_cart_rsp_init(IsleGameObject*,void*);
// Actual C callback observations only, not a fake gameplay receiver or pose.
typedef struct PlayerCartRspEvents {
    IslePlayerSource* player;
    IsleGameObject* expected;
    const IslePlayerCartModelAnimation* reenter_model;
    IslePlayerCartNode nodes[16];
    int32_t ids[16];
    float arguments[16];
    size_t count;
    unsigned invalid,entered,returned;
    int fail,retire,reenter;
} PlayerCartRspEvents;
typedef struct PlayerCartRspTopology {
    size_t objects,processes,models,nodes,identity_holds;
    unsigned invalid;
} PlayerCartRspTopology;
void player_cart_rsp_callback(void*,const IslePlayerCartNode*,int32_t,float);
void player_cart_rsp_topology(IsleGame*,IsleGameObject*,PlayerCartRspTopology*);
#ifdef __cplusplus
}
#endif
#endif
