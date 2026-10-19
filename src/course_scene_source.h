#ifndef ISLE_COURSE_SCENE_SOURCE_H
#define ISLE_COURSE_SCENE_SOURCE_H
#include "course_actor_source.h"
#include "player_root_source.h"
#include "player_cart_source.h"
#ifdef __cplusplus
extern "C" {
#endif
IsleGame* isle_course_scene_object_game(IsleGameObject*);
IsleGameResult isle_course_scene_control(IslePlayerSource*,void (*)(IslePlayerSource*));
IsleGameResult isle_course_scene_object_control(IslePlayerSource*,void (*)(IslePlayerSource*,IsleGameObject*),IsleGameObject*);
IsleGameResult isle_course_scene_invoke_object(IsleCourseActorBase*,IsleGameSourceCallback,IsleGameObject*);
IsleGameResult isle_course_scene_invoke_block(IsleCourseActorBase*,void (*)(IsleGameObject*,float,float,float,float,float,float),IsleGameObject*,const IsleGameVec3*,const IsleGameVec3*);
void isle_course_scene_camera_update(IsleGameObject*);
void isle_course_scene_input_update(IsleGameObject*);
void isle_course_scene_volcano_cart_event(void*,const IslePlayerCartNode*,int32_t,float);
void isle_course_scene_river_cart_event(void*,const IslePlayerCartNode*,int32_t,float);
void isle_course_scene_cave_cart_event(void*,const IslePlayerCartNode*,int32_t,float);
void isle_course_scene_valley_cart_event(void*,const IslePlayerCartNode*,int32_t,float);
void isle_course_scene_tunnel_item(IsleGameObject*,IsleActorGround*);
struct IsleRiverLifecycleSource;
IsleGameResult isle_course_scene_river_pause(struct IsleRiverLifecycleSource*,int32_t);
struct IsleVolcanoLifecycleSource;
struct IsleCaveLifecycleSource;
struct IsleValleyLifecycleSource;
IsleGameResult isle_course_scene_volcano_transition_end(struct IsleVolcanoLifecycleSource*,int32_t);
IsleGameResult isle_course_scene_river_transition_end(struct IsleRiverLifecycleSource*,int32_t);
IsleGameResult isle_course_scene_cave_transition_end(struct IsleCaveLifecycleSource*,int32_t);
IsleGameResult isle_course_scene_valley_transition_end(struct IsleValleyLifecycleSource*,int32_t);
#ifdef __cplusplus
}
#endif
#endif
