#include "course_scene_source.h"
#include "engine_internal.h"
#include "tunnel_lifecycle_source.h"
#include "volcano_lifecycle_source.h"
#include "river_lifecycle_source.h"
#include "cave_lifecycle_source.h"
#include "valley_lifecycle_source.h"
#include "menu_scene_bridge.h"
IsleGame* isle_course_scene_object_game(IsleGameObject* object) { return object ? object->game : NULL; }
typedef struct ControlCall { void (*function)(IslePlayerSource*); } ControlCall;
static void control_call(IslePlayerSource* source,void* argument) { ControlCall* call=argument; call->function(source); }
IsleGameResult isle_course_scene_control(IslePlayerSource* source,void (*function)(IslePlayerSource*)) {
    ControlCall call={function}; if (!source || !function) return ISLE_GAME_INVALID_ARGUMENT;
    return isle_player_source_call(source,control_call,&call);
}
typedef struct ObjectControlCall { void (*function)(IslePlayerSource*,IsleGameObject*); IsleGameObject* object; } ObjectControlCall;
static void object_control_call(IslePlayerSource* source,void* argument) { ObjectControlCall* call=argument; call->function(source,call->object); }
IsleGameResult isle_course_scene_object_control(IslePlayerSource* source,void (*function)(IslePlayerSource*,IsleGameObject*),IsleGameObject* object) {
    ObjectControlCall call={function,object}; if (!source || !function || !object) return ISLE_GAME_INVALID_ARGUMENT;
    return isle_player_source_call(source,object_control_call,&call);
}
typedef struct CallbackCall { IsleGameSourceCallback callback; IsleGameObject* object; } CallbackCall;
static void callback_call(void* argument) { CallbackCall* call=argument; call->callback(call->object); }
IsleGameResult isle_course_scene_invoke_object(IsleCourseActorBase* source,IsleGameSourceCallback callback,IsleGameObject* object) {
    CallbackCall call={callback,object}; if (!source || !callback || !object) return ISLE_GAME_INVALID_ARGUMENT;
    return isle_course_actor_invoke(source,callback_call,&call);
}
typedef struct BlockCall { void (*callback)(IsleGameObject*,float,float,float,float,float,float); IsleGameObject* object; IsleGameVec3 previous,current; } BlockCall;
static void block_call(void* argument) { BlockCall* call=argument; call->callback(call->object,call->previous.x,call->previous.y,call->previous.z,call->current.x,call->current.y,call->current.z); }
IsleGameResult isle_course_scene_invoke_block(IsleCourseActorBase* source,void (*callback)(IsleGameObject*,float,float,float,float,float,float),IsleGameObject* object,const IsleGameVec3* previous,const IsleGameVec3* current) {
    BlockCall call; if (!source || !callback || !object || !previous || !current) return ISLE_GAME_INVALID_ARGUMENT;
    call=(BlockCall){callback,object,*previous,*current}; return isle_course_actor_invoke(source,block_call,&call);
}
void isle_course_scene_camera_update(IsleGameObject* object) {
    IslePlayerSource* player=isle_player_source_current(object);
    isle_player_source_checked(player,isle_player_root_native_camera_animation_update(player,object));
}
/* The input invalidation remains an actual C Source updater. The returning
 * native leaf verifies the actual selected Host input receiver before mutation. */
IsleGameResult isle_course_scene_native_input_update(IsleGame*,IsleGameObject*);
void isle_course_scene_input_update(IsleGameObject* object) {
    IslePlayerSource* player=isle_player_source_current(object);
    for (;;) {
        isle_player_source_checked(player,isle_course_scene_native_input_update(object->game,object));
        isle_player_source_wait(player,1);
    }
}
void isle_course_scene_volcano_cart_event(void* user,const IslePlayerCartNode* node,int32_t event,float value) {
    IsleVolcanoLifecycleSource* source=user;
    isle_volcano_lifecycle_checked(source,isle_volcano_lifecycle_invoke_intro_animation(source,node,event,value));
}
void isle_course_scene_river_cart_event(void* user,const IslePlayerCartNode* node,int32_t event,float value) {
    IsleRiverLifecycleSource* source=user; IsleActorNodeRef reference={node->binding,node->node};
    isle_river_lifecycle_checked(source,isle_river_lifecycle_invoke_animation(source,func_802E2120_6C9F00,&reference,event,value));
}
void isle_course_scene_cave_cart_event(void* user,const IslePlayerCartNode* node,int32_t event,float value) {
    IsleCaveLifecycleSource* source=user; IsleActorNodeRef reference={node->binding,node->node};
    isle_cave_lifecycle_checked(source,isle_cave_lifecycle_invoke_animation(source,&reference,event,value));
}
void isle_course_scene_valley_cart_event(void* user,const IslePlayerCartNode* node,int32_t event,float value) {
    IsleValleyLifecycleSource* source=user; IsleActorNodeRef reference={node->binding,node->node};
    isle_valley_lifecycle_checked(source,isle_valley_lifecycle_invoke_animation(source,&reference,event,value));
}
void isle_course_scene_tunnel_item(IsleGameObject* object,IsleActorGround* ground) {
    IsleTunnelLifecycleSource* source=isle_tunnel_lifecycle_current(object);
    isle_tunnel_lifecycle_checked(source,isle_tunnel_lifecycle_collide_item(source,object,ground));
}
static void river_pause_call(IsleRiverLifecycleSource* source,void* argument) {
    func_802D89AC_6C078C(source,*(int32_t*)argument);
}
IsleGameResult isle_course_scene_river_pause(IsleRiverLifecycleSource* source,int32_t reason) {
    return isle_river_lifecycle_invoke(source,river_pause_call,&reason);
}
extern IsleGameResult isle_course_scene_volcano_owner_transition(IsleVolcanoLifecycleSource*,int32_t);
typedef struct VolcanoTransitionCall { IsleVolcanoLifecycleSource* source; int32_t reason; IsleGameResult result; } VolcanoTransitionCall;
static void volcano_transition_owner(void* user) {
    VolcanoTransitionCall* call=user;
    call->result=isle_course_scene_volcano_owner_transition(call->source,call->reason);
}
IsleGameResult isle_course_scene_volcano_transition_end(IsleVolcanoLifecycleSource* source,int32_t reason) {
    VolcanoTransitionCall call={source,reason,ISLE_GAME_INVALID_STATE};
    IsleContinuationResult result;
    if (!source || !source->owner) return ISLE_GAME_INVALID_ARGUMENT;
    result=isle_continuation_on_owner(volcano_transition_owner,&call);
    if (result==ISLE_CONT_INVALID_STATE) volcano_transition_owner(&call);
    else if (result!=ISLE_CONT_OK) return result==ISLE_CONT_WRONG_OWNER ? ISLE_GAME_WRONG_OWNER :
        result==ISLE_CONT_RESOURCE_FAILURE ? ISLE_GAME_EXHAUSTED : ISLE_GAME_EXECUTION_FAILURE;
    return call.result;
}
extern IsleGameResult isle_course_scene_river_owner_transition(IsleRiverLifecycleSource*,int32_t);
typedef struct RiverTransitionCall { IsleRiverLifecycleSource* source; int32_t reason; IsleGameResult result; } RiverTransitionCall;
static void river_transition_owner(void* user) {
    RiverTransitionCall* call=user;
    call->result=isle_course_scene_river_owner_transition(call->source,call->reason);
}
IsleGameResult isle_course_scene_river_transition_end(IsleRiverLifecycleSource* source,int32_t reason) {
    RiverTransitionCall call={source,reason,ISLE_GAME_INVALID_STATE};
    IsleContinuationResult result;
    if (!source || !source->owner) return ISLE_GAME_INVALID_ARGUMENT;
    result=isle_continuation_on_owner(river_transition_owner,&call);
    if (result==ISLE_CONT_INVALID_STATE) river_transition_owner(&call);
    else if (result!=ISLE_CONT_OK) return result==ISLE_CONT_WRONG_OWNER ? ISLE_GAME_WRONG_OWNER :
        result==ISLE_CONT_RESOURCE_FAILURE ? ISLE_GAME_EXHAUSTED : ISLE_GAME_EXECUTION_FAILURE;
    return call.result;
}
extern IsleGameResult isle_course_scene_cave_owner_transition(IsleCaveLifecycleSource*,int32_t);
typedef struct CaveTransitionCall { IsleCaveLifecycleSource* source; int32_t reason; IsleGameResult result; } CaveTransitionCall;
static void cave_transition_owner(void* user) {
    CaveTransitionCall* call=user;
    call->result=isle_course_scene_cave_owner_transition(call->source,call->reason);
}
IsleGameResult isle_course_scene_cave_transition_end(IsleCaveLifecycleSource* source,int32_t reason) {
    CaveTransitionCall call={source,reason,ISLE_GAME_INVALID_STATE};
    IsleContinuationResult result;
    if (!source || !source->owner) return ISLE_GAME_INVALID_ARGUMENT;
    result=isle_continuation_on_owner(cave_transition_owner,&call);
    if (result==ISLE_CONT_INVALID_STATE) cave_transition_owner(&call);
    else if (result!=ISLE_CONT_OK) return result==ISLE_CONT_WRONG_OWNER ? ISLE_GAME_WRONG_OWNER :
        result==ISLE_CONT_RESOURCE_FAILURE ? ISLE_GAME_EXHAUSTED : ISLE_GAME_EXECUTION_FAILURE;
    return call.result;
}
extern IsleGameResult isle_course_scene_valley_owner_transition(IsleValleyLifecycleSource*,int32_t);
typedef struct ValleyTransitionCall { IsleValleyLifecycleSource* source; int32_t reason; IsleGameResult result; } ValleyTransitionCall;
static void valley_transition_owner(void* user) {
    ValleyTransitionCall* call=user;
    call->result=isle_course_scene_valley_owner_transition(call->source,call->reason);
}
IsleGameResult isle_course_scene_valley_transition_end(IsleValleyLifecycleSource* source,int32_t reason) {
    ValleyTransitionCall call={source,reason,ISLE_GAME_INVALID_STATE};
    IsleContinuationResult result;
    if (!source || !source->owner) return ISLE_GAME_INVALID_ARGUMENT;
    result=isle_continuation_on_owner(valley_transition_owner,&call);
    if (result==ISLE_CONT_INVALID_STATE) valley_transition_owner(&call);
    else if (result!=ISLE_CONT_OK) return result==ISLE_CONT_WRONG_OWNER ? ISLE_GAME_WRONG_OWNER :
        result==ISLE_CONT_RESOURCE_FAILURE ? ISLE_GAME_EXHAUSTED : ISLE_GAME_EXECUTION_FAILURE;
    return call.result;
}
