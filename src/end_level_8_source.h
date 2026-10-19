#ifndef ISLE_END_LEVEL_8_SOURCE_H
#define ISLE_END_LEVEL_8_SOURCE_H
#include "post_course_scene_source.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct IsleEndLevel8Source IsleEndLevel8Source;
typedef IsleGameCamera IsleEndLevel8Camera;
typedef struct IsleEndLevel8Input { uint32_t currentButtons, pressedButtons; } IsleEndLevel8Input;
/* Canonical big-endian initialized words, never an SDK host layout. */
typedef struct IsleEndLevel8InitialData {
    uint32_t rom, guest;
    size_t byte_count;
    const uint32_t* words;
    const char* sha256;
} IsleEndLevel8InitialData;
typedef struct IsleEndLevel8Native {
    IsleGameResult (*admit_initial_data)(IsleEndLevel8Source*,const IsleEndLevel8InitialData*);
    IsleGameResult (*camera)(IsleEndLevel8Source*,IsleGameObject**,IsleEndLevel8Camera**);
    IsleGameResult (*hidden)(IsleEndLevel8Source*,IsleGameObject*,int);
    /* done is an actual retirement fence, independent of sticky error status.
     * A nonzero failure never manufactures done; C retains/polls the owner. */
    IsleGameResult (*join_graphics)(IsleEndLevel8Source*,int* done);
    IsleGameResult (*presentation_begin)(IsleEndLevel8Source*,IsleEndLevel8Camera*,const IslePhotoRecord*);
    IsleGameResult (*presentation_poll)(IsleEndLevel8Source*,int*);
    IsleGameResult (*presentation_clear)(IsleEndLevel8Source*,int* done);
    IsleGameResult (*replay_initialize)(IsleEndLevel8Source*,int,int,int);
    IsleGameResult (*replay_cleanup)(IsleEndLevel8Source*,int* done);
    IsleGameResult (*fade)(IsleEndLevel8Source*,int,float);
    IsleGameResult (*fade_pending)(IsleEndLevel8Source*,int32_t*);
    IsleGameResult (*input)(IsleEndLevel8Source*,int,IsleEndLevel8Input*);
    IsleGameResult (*input_initialize)(IsleEndLevel8Source*);
    IsleGameResult (*sound)(IsleEndLevel8Source*,int);
    IsleGameResult (*volume)(IsleEndLevel8Source*,int,int);
    IsleGameResult (*aux_camera)(IsleEndLevel8Source*,int,int);
    IsleGameResult (*ui_heap)(IsleEndLevel8Source*,size_t);
    IsleGameResult (*depth_screen)(IsleEndLevel8Source*,size_t,size_t);
    IsleGameResult (*deallocate)(IsleEndLevel8Source*,void*);
    IsleGameResult (*process_object)(IsleEndLevel8Source*,int,int,void*,IsleGameObject**);
    IsleGameResult (*scene_result)(IsleEndLevel8Source*,int32_t);
    IsleGameResult (*entry_begin)(IsleEndLevel8Source*,int32_t);
    IsleGameResult (*entry_poll)(void*,int*,int32_t*);
} IsleEndLevel8Native;
struct IsleEndLevel8Source {
    IslePostCourseSceneSource common;
    const IsleEndLevel8Native* requiredNativeTable;
    uint64_t module_generation;
    IsleGameObject* D_801DD240_A08E30;
    IsleEndLevel8Camera* D_801DD244_A08E34;
    int32_t D_801DD248_A08E38,D_801DD24C_A08E3C,D_801DD250_A08E40;
    IslePostCourseFrame frame;
    IslePostCourseElement element;
    int32_t D_801DD25C_A08E4C,D_801DD12C_A08D1C;
    IsleEndLevel8Input input;
};
typedef struct IsleEndLevel8Entry { IsleEndLevel8Source* source; int32_t argument,result; IsleGameResult status; } IsleEndLevel8Entry;
void isle_end_level_8_source_initialize(IsleEndLevel8Source*,uint64_t);
void isle_end_level_8_source_init_call(IslePostCourseSceneSource*,void*);
void isle_end_level_8_source_entry(void*);
void isle_end_level_8_func_801DC9E8_A085D8(IsleGameObject*);
#ifdef __cplusplus
}
#endif
#endif
