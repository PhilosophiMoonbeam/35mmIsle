#include "player_cart_owner_rsp_fixture.h"
#include "engine_internal.h"

void player_cart_rsp_init(IsleGameObject* object,void* user) {
    PlayerCartRspInit* result=user;
    (void)object;
    if (result->entered) return;
    ++result->entered;
    result->status=isle_player_cart_source_call_init(result->source,&result->object);
    ++result->returned;
}

void player_cart_rsp_callback(void* user,const IslePlayerCartNode* node,int32_t event,float argument) {
    PlayerCartRspEvents* result=user;
    IsleGameModel* model=NULL;
    ++result->entered;
    if (node==NULL || node->binding.object!=result->expected ||
        isle_game_model_resolve(result->player->game,&node->binding,&model)!=ISLE_GAME_OK || model!=node->binding.model)
        ++result->invalid;
    if (result->count<ARRAY_COUNT(result->ids)) {
        result->nodes[result->count]=*node;
        result->ids[result->count]=event;
        result->arguments[result->count++]=argument;
    } else ++result->invalid;
    if (result->retire)
        isle_player_source_checked(result->player,isle_game_delete_object(result->player->game,result->expected));
    if (result->reenter)
        isle_player_cart_PlayerModel_ChangeAnimation(result->player->cart,result->reenter_model,NULL,0);
    if (result->fail) isle_player_source_checked(result->player,ISLE_GAME_INVALID_ARGUMENT);
    ++result->returned;
}
void player_cart_rsp_topology(IsleGame* game,IsleGameObject* cart,PlayerCartRspTopology* out) {
    *out=(PlayerCartRspTopology){0};
    out->identity_holds=game->held_identities;
    for (size_t link=0;link<ARRAY_COUNT(omGObjListHead);++link)
        for (GObj* object=omGObjListHead[link];object!=NULL;object=object->next) {
            ++out->objects;
            if (object==cart) {
                if (object->id!=35 || object->link!=9 || object->priority!=0x80000000u ||
                    object->userData!=NULL || object->sourcePayloadKind!=ISLE_SOURCE_PAYLOAD_NONE) ++out->invalid;
            }
        }
    for (size_t priority=0;priority<ARRAY_COUNT(omGProcessList);++priority)
        for (GObjProcess* process=omGProcessList[priority];process!=NULL;process=process->nextInPriorityList) {
            ++out->processes;
            if (process->object==cart && (process->function!=isle_player_cart_source_update || process->kind!=1 || process->priority!=1))
                ++out->invalid;
        }
    if (cart!=NULL) {
        const IsleGameModelNode* nodes=NULL;
        size_t count=0;
        if (isle_game_model_peek_nodes(game,cart->data.model,&nodes,&count)!=ISLE_GAME_OK) ++out->invalid;
        else { out->models=1; out->nodes=count; }
    }
}
