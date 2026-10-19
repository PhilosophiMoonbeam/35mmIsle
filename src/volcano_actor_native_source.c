#include "volcano_actor_source.h"
#include "player_cart_source.h"

/* Cart dispatch invokes this plain-C callback only after its native step leaf
 * has returned. The existing Volcano invoke fence runs the actual gate callback
 * wholly in C; the checked transfer cannot cross a C++ event/Runtime frame. */
void isle_volcano_actor_cart_event(void* user,const IslePlayerCartNode* node,int32_t event,float value) {
    IsleVolcanoActorSource* source=user;
    IsleGameResult result;
    if (node==NULL) {
        isle_volcano_actor_checked(source,ISLE_GAME_INVALID_ARGUMENT);
        return;
    }
    result=isle_volcano_actor_invoke_animation(source,&node->binding,event,value);
    isle_volcano_actor_checked(source,result);
}
