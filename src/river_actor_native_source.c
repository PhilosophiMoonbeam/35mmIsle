#include "river_actor_native_source.h"

/* Cart's actual C dispatch owns current_event and catches Player failures.
 * The typed Actor invocation has its own C-only fence; checked transfers below
 * occur only after every native C++ leaf has returned. No callback is cast. */
void isle_river_native_gate_cart_event(void* user,const IslePlayerCartNode* node,int32_t event,float value) {
    IsleRiverActorSource* source=user;
    IsleActorNodeRef reference;
    isle_river_actor_checked(source,isle_river_native_cart_event_admit(source,node,isle_river_native_gate_cart_event));
    reference=(IsleActorNodeRef){node->binding,node->node};
    isle_river_actor_checked(source,isle_actor_native_river_animation_event(source,
        func_802E01C8_6C7FA8,&reference,event,value));
}
void isle_river_native_outro_cart_event(void* user,const IslePlayerCartNode* node,int32_t event,float value) {
    IsleRiverActorSource* source=user;
    IsleActorNodeRef reference;
    isle_river_actor_checked(source,isle_river_native_cart_event_admit(source,node,isle_river_native_outro_cart_event));
    reference=(IsleActorNodeRef){node->binding,node->node};
    isle_river_actor_checked(source,isle_actor_native_river_animation_event(source,
        func_802E19A0_6C9780,&reference,event,value));
}
