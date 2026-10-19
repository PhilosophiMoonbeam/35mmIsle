#ifndef ISLE_PHOTO_CHECK_SOURCE_H
#define ISLE_PHOTO_CHECK_SOURCE_H
#include "post_course_scene_source.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct IslePhotoCheckSource IslePhotoCheckSource;
typedef int32_t (*IslePhotoCheckCompare)(IslePhotoCheckSource*, const void*, const void*);
/* Semantic overlay working records, never saved-photo ownership or guest ABI.
 * The signed one-bit flags intentionally compare as 0/-1, like Photo. */
typedef struct IslePhotoCheckPhoto {
    IslePlayerPhotoData* unk_0;
    uint32_t totalScore;
    uint8_t poseID, samePkmnNumber;
    int16_t pkmnID, specialID, isWellFramed, posePts, completenessScore;
    int16_t proximityScore, specialBonus, samePkmnBonus;
    signed int unk_1A_15 : 1;
    signed int unk_1A_14 : 1;
    signed int unk_1A_13 : 1;
    signed int unk_1A_12 : 1;
    signed int unk_1A_11 : 1;
} IslePhotoCheckPhoto;
typedef struct IslePhotoCheckElement { IslePostCourseElement handle; } IslePhotoCheckElement;
typedef struct IslePhotoCheckFrame { IslePostCourseFrame handle; } IslePhotoCheckFrame;
typedef struct IslePhotoCheckObjPair {
    IsleGameObject* gobj;
    IsleGameSprite* sobj;
    IslePostCourseThumbnail thumbnail;
} IslePhotoCheckObjPair;
typedef struct IslePhotoCheckInput {
    uint32_t currentButtons, pressedButtons;
} IslePhotoCheckInput;

struct IslePhotoCheckSource {
    IslePostCourseSceneSource common; /* MUST remain first. */
    IslePhotoCheckObjPair D_802290A0_9D8B10[6];
    IslePhotoCheckFrame* D_802290D8_9D8B48;
    IslePhotoCheckElement *D_802290DC_9D8B4C, *D_802290E0_9D8B50, *D_802290E4_9D8B54;
    char D_802290E8_9D8B58[128];
    IsleGameSprite *D_80229168_9D8BD8, *D_80229170_9D8BE0[4];
    IsleGameSprite *D_80229180_9D8BF0, *D_80229184_9D8BF4, *D_80229188_9D8BF8;
    int32_t D_8022918C_9D8BFC;
    const IslePostCourseButton* D_80229190_9D8C00;
    IslePostCourseWindow* D_80229194_9D8C04; /* Live selector, not copied slots. */
    IslePhotoCheckPhoto D_802291A0_9D8C10[60];
    int32_t D_80229838_9D92A8;
    int32_t D_801F3E28_9A3898, D_801F3E2C_9A389C, D_801F3E30_9A38A0;
    int32_t D_801F3E34_9A38A4, D_801F3E38_9A38A8;
    int32_t D_801F3E60_9A38D0, D_801F3E64_9A38D4;
    int32_t D_801F4144_9A3BB4;
    /* Original data words are float 0/1; the reached scalar is selected count.
     * Do not alias a host float array through an integer lvalue. */
    int32_t D_801F4148_9A3BB8, D_801F4180_9A3BF0, D_801F4194_9A3C04;
    IslePhotoCheckCompare D_801F4198_9A3C08;
    IslePhotoCheckElement elements[8];
    size_t element_count;
    IslePhotoCheckFrame frame;
    IslePhotoCheckInput input;
    IslePostCourseScoredPhoto score_scratch;
    char formatted[400], number_ascii[16], number_source[34];
    IslePhotoCheckCompare qsortCompareFunc;
};

/* Initialize only after native common ownership has been bound. No heap, SDK
 * global, Photos reset, Window reset or World reset is performed here. */
void isle_photo_check_source_init_call(IslePostCourseSceneSource*, void*);
void isle_photo_check_source_initialize(IslePhotoCheckSource*);
void isle_photo_check_source_init(IslePhotoCheckSource*);
/* Independent entry continuation backing MUST outlive native scene retirement.
 * The entry copies owner/domain before setup and never dereferences source after
 * requesting setup. Parent resumes it only for setup completion/retirement. */
typedef struct IslePhotoCheckEntry {
    IslePhotoCheckSource* source;
    int32_t argument, result;
    IsleGameResult status;
} IslePhotoCheckEntry;
void isle_photo_check_source_entry(void*);
void isle_photo_check_func_801DCC74_98C6E4(void*);
void isle_photo_check_func_801DCA14_98C484(IsleGameObject*);
void isle_photo_check_func_801DDCF8_98D768(IsleGameObject*);
void isle_photo_check_func_801DE02C_98DA9C(IsleGameObject*);
void isle_photo_check_func_801E39DC_99344C(IsleGameObject*);

/* Returning owner leaves: no Source callbacks, waits, ticks, Runtime::pump,
 * updates, retraces or draw credits. Every output clears on failure. Exceptions
 * stay behind the native boundary. Object ownership/context is independent of
 * original userData. These are actual services, not optional fallback hooks. */
/* Original UIMem_CreateHeap's 0x32000 capacity belongs to the canonical Window
 * memory owner. Admit/reserve it there; never create a second overlay byte heap. */
IsleGameResult isle_photo_check_native_ui_heap(IslePhotoCheckSource*, size_t bytes);
IsleGameResult isle_photo_check_native_input_initialize(IslePhotoCheckSource*);
IsleGameResult isle_photo_check_native_input(IslePhotoCheckSource*, int32_t, IslePhotoCheckInput*);
IsleGameResult isle_photo_check_native_input_repeat(IslePhotoCheckSource*, int32_t, int32_t);
IsleGameResult isle_photo_check_native_input_mask(IslePhotoCheckSource*, uint32_t);
/* 98C330:355-376: camera id 0x200/updateDefault/link5/priority5;
 * renderer 800191D8, render priority20, masks 0x100000, matrix1,
 * unk38=0x100000, flags (old|CAMERA_FLAG_4|CAMERA_FLAG_2)&
 * ~(CAMERA_FLAG_2|CAMERA_FLAG_1), viewport 320x240,
 * up=(0,0,1), at=D800AF0C4/eye=D800AF094, fovy33/far8000/
 * near62.5/scale1/bg1. Leaf creates/configures only. C adds original idle
 * process kind0/priority0 after return. */
IsleGameResult isle_photo_check_native_camera(IslePhotoCheckSource*, IsleGameObject**);
IsleGameResult isle_photo_check_native_aux_camera(IslePhotoCheckSource*, int32_t, int32_t);
/* Source func800A85E8 (53AD0.c140-157): returning creation only,
 * ohCreateModel(1<<link,NULL,link,link,renRenderModelTypeA,dllink,link,
 * cameraTag,dlist,true,0,procFunc,0), cameraTag=dllink<0?0:1<<dllink,
 * negative dllink becomes0, model rotation kind ROTATE_RPY.
 * C installs supplied original process only after return. */
IsleGameResult isle_photo_check_native_process_object(IslePhotoCheckSource*, int32_t, int32_t, void*, IsleGameObject**);
IsleGameResult isle_photo_check_native_userdata_set(IslePhotoCheckSource*, IsleGameObject*, const void*);
IsleGameResult isle_photo_check_native_replay_initialize(IslePhotoCheckSource*);
IsleGameResult isle_photo_check_native_replay_cleanup(IslePhotoCheckSource*);
/* Actual shared Window D803A6668 initialized/current value, never a provider
 * default. It remains common across CameraCheck -> PhotoCheck. */
IsleGameResult isle_photo_check_native_preview_scale(IslePhotoCheckSource*, float*);
IsleGameResult isle_photo_check_native_sound(IslePhotoCheckSource*, int32_t);
IsleGameResult isle_photo_check_native_sound_params(IslePhotoCheckSource*, int32_t, int32_t, int32_t, float, int32_t);
IsleGameResult isle_photo_check_native_song(IslePhotoCheckSource*, int32_t, int32_t);
IsleGameResult isle_photo_check_native_volume(IslePhotoCheckSource*, int32_t, int32_t);
IsleGameResult isle_photo_check_native_volume_smooth(IslePhotoCheckSource*, int32_t, int32_t, int32_t);
IsleGameResult isle_photo_check_native_player_flag(IslePhotoCheckSource*, int32_t, int32_t*);
/* Stable NUL-terminated original SOURCE BYTES, not host UTF-8. A genuine NULL
 * name is preserved. Borrow remains live through Source waits. */
IsleGameResult isle_photo_check_native_player_name(IslePhotoCheckSource*, const char**);
IsleGameResult isle_photo_check_native_pokemon_name(IslePhotoCheckSource*, int32_t, const char**);
IsleGameResult isle_photo_check_native_level_name(IslePhotoCheckSource*, int32_t, const char**);
IsleGameResult isle_photo_check_native_level_id(IslePhotoCheckSource*, int32_t*);
/* func800BFC5C is selected course, NOT unlock progress. */
IsleGameResult isle_photo_check_native_selected_course(IslePhotoCheckSource*, int32_t*);
IsleGameResult isle_photo_check_native_unlock_progress(IslePhotoCheckSource*, int32_t*);
IsleGameResult isle_photo_check_native_set_unlock_progress(IslePhotoCheckSource*, int32_t);
IsleGameResult isle_photo_check_native_notices_or(IslePhotoCheckSource*, uint32_t);
IsleGameResult isle_photo_check_native_notices_test(IslePhotoCheckSource*, uint32_t, int32_t*);
IsleGameResult isle_photo_check_native_scene_result(IslePhotoCheckSource*, int32_t);
/* Apply actual decoded 801e5410 ScreenSettings (including 0x16a99) and
 * gtlDisableNearClipping(1); setup uses decoded 801e542c SceneSetup and the
 * original arena extent 80369f80-80229840, not commented reconstructions.
 * setup_begin publishes only; entry polls retirement on its independent C
 * continuation. entry_poll is safe after native scene/source destruction.
 * Both hooks MUST retain the entry's stable owner and actual manager result. */
IsleGameResult isle_photo_check_native_entry_begin(IslePhotoCheckSource*, int32_t);
IsleGameResult isle_photo_check_native_entry_poll(void* owner, int* retired, int32_t* result);
#ifdef __cplusplus
}
#endif
#endif
