#ifndef ISLE_SCREEN_TRANSITION_SOURCE_H
#define ISLE_SCREEN_TRANSITION_SOURCE_H
#include "35mmisle/engine.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct IsleScreenTransitionObject IsleScreenTransitionObject;
typedef struct IsleScreenTransitionSource {
    IsleGame* game;
    void* owner;
    int32_t D_800AF050, D_800AF054, D_800AF058, D_800AF05C, D_800AF060;
    int32_t D_800BE2C0, D_800BE2C4;
    IsleGameCamera* D_800BE2C8;
    int D_800BE2CC;
    IsleGameObject* D_800BE2D0;
    IsleGameObject* camera_object;
    uint32_t camera_flags, camera_mask, camera_buffer_mask;
    IsleGameResult status;
} IsleScreenTransitionSource;
struct IsleScreenTransitionObject {
    IsleScreenTransitionObject* next;
    IsleScreenTransitionSource* source;
    IsleGameObject* object;
    IsleGameModel* model;
    float duration;
    IsleGameSourceCallback callback;
    uint32_t display_list;
    uint8_t render_kind;
    int joined;
};
float isle_screen_transition_func_800A7430(IsleScreenTransitionSource*, float);
int32_t isle_screen_transition_func_800A7460(IsleScreenTransitionSource*);
IsleGameResult isle_screen_transition_func_800A7470(IsleScreenTransitionSource*, int32_t, int32_t, int32_t);
IsleGameResult isle_screen_transition_func_800A7860(IsleScreenTransitionSource*, int32_t, float);
typedef struct IsleScreenTransitionRender {
    uint32_t primitive_rgba, render_mode, display_list;
} IsleScreenTransitionRender;
IsleGameResult isle_screen_transition_func_800A750C(IsleScreenTransitionSource*, IsleScreenTransitionObject*, IsleScreenTransitionRender*);
IsleGameResult isle_screen_transition_func_800A7948(IsleScreenTransitionSource*, IsleScreenTransitionObject*, IsleScreenTransitionRender*);
IsleGameResult isle_screen_transition_func_800A7A58(IsleScreenTransitionSource*, IsleScreenTransitionObject*, IsleScreenTransitionRender*);
IsleGameResult isle_screen_transition_func_800A7918(IsleScreenTransitionSource*, int32_t, float);
IsleGameResult isle_screen_transition_func_800A7CC4(IsleScreenTransitionSource*, float, float, int32_t, float);
IsleGameResult isle_screen_transition_func_800A7F40(IsleScreenTransitionSource*, float, float, int32_t, float);
IsleGameResult isle_screen_transition_func_800A7F68(IsleScreenTransitionSource*, int32_t, int32_t);
void isle_screen_transition_func_800A7660(IsleGameObject*);
void isle_screen_transition_func_800A7760(IsleGameObject*);
void isle_screen_transition_func_800A7BAC(IsleGameObject*);
/* Returning leaves; C control alone owns waits/deletion/fault transfers. */
IsleScreenTransitionObject* isle_screen_transition_current(IsleGameObject*);
IsleScreenTransitionObject* isle_screen_transition_create(IsleScreenTransitionSource*, IsleGameSourceCallback, uint32_t);
IsleScreenTransitionObject* isle_screen_transition_func_800A748C(IsleScreenTransitionSource*, IsleGameSourceCallback, uint32_t);
IsleGameResult isle_screen_transition_camera(IsleScreenTransitionSource*, int32_t, int32_t);
IsleGameResult isle_screen_transition_bind(IsleScreenTransitionObject*);
IsleGameResult isle_screen_transition_transform(IsleScreenTransitionObject*, IsleGameModelTransform*, int write);
void isle_screen_transition_delete(IsleScreenTransitionSource*, IsleGameObject*);
void isle_screen_transition_checked(IsleScreenTransitionSource*, IsleGameResult);
#ifdef __cplusplus
}
#endif
#endif
