#ifndef ISLE_GALLERY_SOURCE_H
#define ISLE_GALLERY_SOURCE_H
#include "post_course_scene_source.h"
#include "application_photos_source.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct IsleGallerySource IsleGallerySource;
typedef struct IsleGalleryInput { uint32_t currentButtons, pressedButtons; } IsleGalleryInput;
typedef struct IsleGalleryElement { IslePostCourseElement handle; } IsleGalleryElement;
typedef struct IsleGalleryFrame { IslePostCourseFrame handle; } IsleGalleryFrame;
typedef struct IsleGalleryPair { IsleGameObject* gobj; IsleGameSprite* sobj; IslePostCourseThumbnail thumbnail; } IsleGalleryPair;
typedef struct IsleGallerySelection { int8_t unk_00, unk_01; } IsleGallerySelection;
/* Parent qualifies this immutable capability against the canonical initialized
 * bank and callback/data receipt. No provider default words or fake photos. */
typedef struct IsleGalleryInitialData {
    const void* canonical_receipt;
    int32_t D_801E16BC_9FF37C;
    int32_t D_801EA1FC_A07EBC;
    int32_t D_801EA1F4_A07EB4;
    int32_t D_801EA1F8_A07EB8;
    int32_t D_801EA200_A07EC0;
    int32_t D_801EA204_A07EC4;
    int32_t D_801EA288_A07F48;
    int32_t D_801EA28C_A07F4C;
    int32_t D_801EA290_A07F50;
    int32_t D_801EA294_A07F54;
    int32_t D_801EA298_A07F58;
    int32_t D_801EA29C_A07F5C;
    int32_t D_801EA2A0_A07F60;
    int32_t D_801EA2A4_A07F64;
    int32_t D_801EA2A8_A07F68;
    int32_t D_801EA2AC_A07F6C;
    int32_t D_801EA2B0_A07F70;
    const IslePostCourseButton *D_801EA210_A07ED0, *D_801EA248_A07F08;
    size_t absent_button_count, present_button_count;
} IsleGalleryInitialData;
/* All leaves return before a C wait/callback/terminal transfer. Every field is
 * mandatory. Native failure is never an Original scalar result. Cameras and
 * object traversal return real Game handles, never copied SDK layouts. */
typedef struct IsleGalleryNative {
    IsleGameResult (*initial_validate)(IsleGallerySource*, const IsleGalleryInitialData*);
    IsleGameResult (*input)(IsleGallerySource*, int32_t, IsleGalleryInput*);
    IsleGameResult (*input_initialize)(IsleGallerySource*);
    IsleGameResult (*input_repeat)(IsleGallerySource*, int32_t, int32_t);
    IsleGameResult (*input_mask)(IsleGallerySource*, uint32_t);
    IsleGameResult (*camera)(IsleGallerySource*, int32_t, IsleGameObject**);
    IsleGameResult (*aux_camera)(IsleGallerySource*, int32_t, int32_t);
    IsleGameResult (*process_object)(IsleGallerySource*, int32_t, int32_t, void*, IsleGameObject**);
    IsleGameResult (*display_list)(IsleGallerySource*, void**);
    IsleGameResult (*render_state)(IsleGallerySource*, int32_t, int32_t);
    IsleGameResult (*screen)(IsleGallerySource*, size_t, size_t);
    IsleGameResult (*ui_heap)(IsleGallerySource*, size_t);
    IsleGameResult (*deallocate)(IsleGallerySource*, void*);
    IsleGameResult (*object_hidden)(IsleGallerySource*, IsleGameObject*, int);
    IsleGameResult (*object_first)(IsleGallerySource*, int32_t, IsleGameObject**);
    IsleGameResult (*object_next)(IsleGallerySource*, IsleGameObject*, IsleGameObject**);
    IsleGameResult (*object_id)(IsleGallerySource*, IsleGameObject*, int32_t*);
    IsleGameResult (*object_pause)(IsleGallerySource*, IsleGameObject*);
    IsleGameResult (*graphics_join)(IsleGallerySource*, int32_t* pending);
    IsleGameResult (*presentation_begin)(IsleGallerySource*, IsleGameObject*, const IslePlayerPhotoData*);
    IsleGameResult (*presentation_poll)(IsleGallerySource*, int32_t*);
    IsleGameResult (*presentation_release)(IsleGallerySource*, int32_t* pending);
    IsleGameResult (*fade)(IsleGallerySource*, int32_t, float);
    IsleGameResult (*fade_poll)(IsleGallerySource*, int32_t*);
    IsleGameResult (*sprite_clear_matrices)(IsleGallerySource*, IsleGameSprite*);
    IsleGameResult (*preview_scale)(IsleGallerySource*, float*);
    IsleGameResult (*replay_initialize)(IsleGallerySource*);
    IsleGameResult (*replay_cleanup)(IsleGallerySource*, int32_t* pending);
    IsleGameResult (*printer_available)(IsleGallerySource*, int32_t*);
    IsleGameResult (*printer_status)(IsleGallerySource*, int32_t*);
    IsleGameResult (*printer_command)(IsleGallerySource*, int32_t);
    IsleGameResult (*sound)(IsleGallerySource*, int32_t);
    IsleGameResult (*sound_params)(IsleGallerySource*, int32_t, int32_t, int32_t, float, int32_t);
    IsleGameResult (*song)(IsleGallerySource*, int32_t, int32_t);
    IsleGameResult (*volume)(IsleGallerySource*, int32_t, int32_t);
    IsleGameResult (*volume_smooth)(IsleGallerySource*, int32_t, int32_t, int32_t);
    IsleGameResult (*scene_result)(IsleGallerySource*, int32_t);
    IsleGameResult (*pokemon_name)(IsleGallerySource*, int32_t, const char**);
    IsleGameResult (*save)(IsleGallerySource*, int32_t*);
    IsleGameResult (*entry_begin)(IsleGallerySource*, int32_t);
    IsleGameResult (*entry_poll)(void*, int*, int32_t*);
} IsleGalleryNative;
struct IsleGallerySource {
    IslePostCourseSceneSource common; /* common first; never object userData */
    const IsleGalleryNative* native;
    const IsleGalleryInitialData* initial;
    int32_t D_801E16BC_9FF37C;
    int32_t D_801EA1FC_A07EBC;
    int32_t D_801EA1F4_A07EB4;
    int32_t D_801EA1F8_A07EB8;
    int32_t D_801EA200_A07EC0;
    int32_t D_801EA204_A07EC4;
    int32_t D_801EA288_A07F48;
    int32_t D_801EA28C_A07F4C;
    int32_t D_801EA290_A07F50;
    int32_t D_801EA294_A07F54;
    int32_t D_801EA298_A07F58;
    int32_t D_801EA29C_A07F5C;
    int32_t D_801EA2A0_A07F60;
    int32_t D_801EA2A4_A07F64;
    int32_t D_801EA2A8_A07F68;
    int32_t D_801EA2AC_A07F6C;
    int32_t D_801EA2B0_A07F70;
int32_t D_801EA7F0_A084B0;
IsleGameObject* D_801EA7F4_A084B4;
IsleGameObject* D_801EA7FC_A084BC;
int32_t D_80230810_A4E4D0;
IsleGalleryPair D_80230818_A4E4D8[6];
IsleGalleryPair D_80230848_A4E508[4];
IsleGameObject* D_80230868_A4E528;
IsleGameSprite* D_8023086C_A4E52C;
IsleGameObject* D_80230870_A4E530;
IsleGameSprite* D_80230874_A4E534;
IsleGalleryFrame* D_80230878_A4E538;
IsleGalleryElement* D_8023087C_A4E53C;
IsleGalleryElement* D_80230880_A4E540;
IsleGalleryElement* D_80230884_A4E544;
int32_t D_80230888_A4E548;
int32_t D_8023088C_A4E54C;
int32_t D_80230890_A4E550;
IslePostCourseWindow* D_802308A0_A4E560;
int32_t D_802308A4_A4E564;
int32_t D_802308A8_A4E568;
int32_t D_802308AC_A4E56C;
int32_t D_802308B0_A4E570;
int32_t D_802308B4_A4E574;
int32_t D_802308B8_A4E578;
const IslePlayerPhotoData* D_802308C0_A4E580[60];
const IslePlayerPhotoData* D_802309B0_A4E670[4];
const IslePlayerPhotoData* D_802309C0_A4E680[69];
const IslePlayerPhotoData* D_80230AD4_A4E794;
IsleGallerySelection D_80230AD8_A4E798;
int32_t D_80230ADC_A4E79C;
int32_t D_80230AE0_A4E7A0;
    IsleGameObject *D_801EA7F8_A084B8, *D_801EA800_A084C0;
    IsleGalleryElement elements[8];
    size_t element_count;
    IsleGalleryFrame frame;
    IsleGalleryInput input;
    IslePostCourseThumbnail dragged_thumbnail;
    IslePhotoWorking* working[4];
    char formatted[400], number_ascii[16], number_source[34];
};
typedef struct IsleGalleryEntry { IsleGallerySource* source; int32_t argument, result; IsleGameResult status; int completed; } IsleGalleryEntry;
IsleGameResult isle_gallery_source_initialize(IsleGallerySource*, const IsleGalleryNative*, const IsleGalleryInitialData*, IslePhotoWorking* const[4], int32_t argument);
void isle_gallery_source_init_call(IslePostCourseSceneSource*,void*);
void isle_gallery_source_entry(void*);
/* Shared envelope resolves actual Game membership and exact registered callback,
 * accepting only this typed provider's actual scene22/23. */
IsleGallerySource* isle_gallery_source_require(IsleGameObject*,IsleGameSourceCallback expected);
void isle_gallery_func_801DCA58_9FA718(IsleGameObject*);
void isle_gallery_func_801DCF50_9FAC10(IsleGameObject*);
void isle_gallery_func_801DF078_9FCD38(IsleGameObject*);
void isle_gallery_func_801DC8C0_9FA580(IsleGallerySource*,void**);
void isle_gallery_func_801DC948_9FA608(IsleGallerySource*,int32_t,int32_t);
void isle_gallery_func_801DCE90_9FAB50(IsleGallerySource*,void*);
void isle_gallery_func_801DCEBC_9FAB7C(void*);
int32_t isle_gallery_func_801DC904_9FA5C4(IsleGallerySource*);
void isle_gallery_func_801DCA98_9FA758(IsleGallerySource*);
void isle_gallery_func_801DCC60_9FA920(IsleGallerySource*);
void isle_gallery_func_801DCDDC_9FAA9C(IsleGallerySource*);
/* Retained even without a direct C call: initialized callback data, not a
 * guessed arming event, determines whether this exact identity is registered. */
void isle_gallery_func_801DD270_9FAF30(IsleGallerySource*);
#ifdef __cplusplus
}
#endif
#endif
