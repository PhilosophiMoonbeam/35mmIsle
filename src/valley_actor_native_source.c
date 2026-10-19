#include "valley_actor_native_source.h"
typedef struct ValleyChildCall { IsleGameObject* parent; IsleGameObject* result; int second; } ValleyChildCall;
static void child_call(void* user) {
    ValleyChildCall* call=user;
    call->result=call->second ? func_8035E238_4FE648(call->parent) : func_8035E1D4_4FE5E4(call->parent);
}
IsleGameResult isle_valley_native_child(IsleValleyActorSource* source,IsleGameObject* parent,int second,IsleGameObject** output) {
    ValleyChildCall call={parent,NULL,second};
    IsleGameResult result;
    if(output) *output=NULL;
    if(!source || !parent || !output || (second!=0 && second!=1)) return ISLE_GAME_INVALID_ARGUMENT;
    result=isle_valley_actor_invoke(source,child_call,&call);
    if(result==ISLE_GAME_OK) *output=call.result;
    return result;
}
void isle_valley_native_ceremony_cart_event(void* user,const IslePlayerCartNode* node,int32_t event,float value) {
    IsleValleyActorSource* source=user;
    IsleActorNodeRef reference;
    isle_valley_actor_checked(source,isle_valley_native_cart_event_admit(source,node,func_802D0420_7A99B0,event,value));
    reference=(IsleActorNodeRef){node->binding,node->node};
    isle_valley_actor_checked(source,isle_valley_actor_invoke_animation(source,func_802D0420_7A99B0,&reference,event,value));
}
void isle_valley_native_gate_cart_event(void* user,const IslePlayerCartNode* node,int32_t event,float value) {
    IsleValleyActorSource* source=user;
    IsleActorNodeRef reference;
    isle_valley_actor_checked(source,isle_valley_native_cart_event_admit(source,node,func_802D0CD8_7AA268,event,value));
    reference=(IsleActorNodeRef){node->binding,node->node};
    isle_valley_actor_checked(source,isle_valley_actor_invoke_animation(source,func_802D0CD8_7AA268,&reference,event,value));
}
