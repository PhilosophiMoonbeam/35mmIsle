#ifndef ISLE_REPORT_SOURCE_H
#define ISLE_REPORT_SOURCE_H
#include "post_course_scene_source.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct IsleReportSource IsleReportSource;
typedef struct IsleReportGraphics IsleReportGraphics;
typedef int32_t (*IsleReportCompare)(IsleReportSource*, const void*, const void*);
typedef struct IsleReportElement { IslePostCourseElement handle; } IsleReportElement;
typedef struct IsleReportFrame { IslePostCourseFrame handle; } IsleReportFrame;
typedef struct IsleReportTriple {
    IsleGameObject* unk_00;
    IsleGameSprite* unk_04;
    IslePostCourseThumbnail thumbnail;
    uint8_t* unk_08;
} IsleReportTriple;
typedef struct IsleReportInput { uint32_t currentButtons, pressedButtons; } IsleReportInput;
/* Immutable decoded module data, admitted by the actual resource owner. Source
 * addresses are origins, never guest pointers. All strings are original encoded
 * NUL-terminated bytes, not UTF-8; their backing outlives scene retirement.
 * captions[k] has caption_count[k]+1 pointers and a genuine terminal NULL.
 * Each sign bitmap is the exact 0xFD8-byte original image, retained by owner.
 * comparator_origin is NULL or one of the four actual C function origins.
 * Parent validates all bytes/pointers against its canonical admitted bank. */
typedef struct IsleReportData {
    uint64_t generation;
    uint32_t comparator_origin;
    int32_t D_80202D30_9F96A0;
    int32_t D_80202EC0_9F9830;
    int32_t D_80202EC4_9F9834;
    int32_t D_80202FF8_9F9968;
    int32_t D_80202FFC_9F996C;
    int32_t D_80203000_9F9970;
    int32_t D_80203004_9F9974;
    int32_t D_80203008_9F9978;
    int32_t D_8020300C_9F997C;
    int32_t D_80203014_9F9984;

    int32_t sign_ids[6];
    int16_t poses[66];
    uint32_t sign_sprites[6];
    const uint8_t* sign_pixels[6];
    size_t sign_pixel_sizes[6];
    const char* const* captions[12];
    size_t caption_count[12];
    const IslePostCourseButton* buttons[8];
    size_t button_count[8];
} IsleReportData;
/* Returning leaves ONLY. No leaf invokes an Original callback, waits, pumps,
 * ticks, publishes a fabricated result, or executes a C terminal transfer.
 * On failure pointer/value outputs clear; exceptions never cross this boundary.
 * prepare_data validates complete initialized Source banks and native resources,
 * including every caption/button text extent and typed comparator origin.
 * setup_begin binds 801e3c80 ScreenSettings/801e3c9c SceneSetup and exact arena;
 * setup_poll/notices_or_owner use copied manager owner after source destruction.
 * sprite_add/remove/detach affect the actual existing hierarchy, not a proxy.
 * thumbnail_bind copies exact admitted sign bytes into actual independent bitmap
 * pieces and retains through GPU join. preview uses canonical Photo replay,
 * never a screenshot. thumbnail_bind may not allocate on subsequent preview. */
typedef struct IsleReportNative {
    IsleGameResult (*prepare_data)(IsleReportSource*, const IsleReportData**);
    IsleGameResult (*ui_heap)(IsleReportSource*, size_t);
    IsleGameResult (*input_initialize)(IsleReportSource*);
    IsleGameResult (*input)(IsleReportSource*, int32_t, IsleReportInput*);
    IsleGameResult (*repeat)(IsleReportSource*, int32_t, int32_t);
    IsleGameResult (*input_mask)(IsleReportSource*, uint32_t);
    IsleGameResult (*camera)(IsleReportSource*, IsleGameObject**);
    IsleGameResult (*aux_camera)(IsleReportSource*, int32_t, int32_t);
    IsleGameResult (*process_object)(IsleReportSource*, int32_t, int32_t, void*, IsleGameObject**);
    IsleGameResult (*replay_initialize)(IsleReportSource*);
    IsleGameResult (*sound)(IsleReportSource*, int32_t);
    IsleGameResult (*song)(IsleReportSource*, int32_t, int32_t);
    IsleGameResult (*volume)(IsleReportSource*, int32_t, int32_t);
    IsleGameResult (*volume_smooth)(IsleReportSource*, int32_t, int32_t, int32_t);
    IsleGameResult (*player_flag)(IsleReportSource*, int32_t, int32_t*);
    IsleGameResult (*player_name)(IsleReportSource*, const char**);
    IsleGameResult (*pokemon_name)(IsleReportSource*, int32_t, const char**);
    IsleGameResult (*level_name)(IsleReportSource*, int32_t, const char**);
    IsleGameResult (*selected_course)(IsleReportSource*, int32_t*);
    IsleGameResult (*pokemon_index)(IsleReportSource*, int32_t, int32_t*);
    IsleGameResult (*scene_result)(IsleReportSource*, int32_t);
    IsleGameResult (*sprite_add)(IsleReportSource*, IsleGameObject*, uint32_t, IsleGameSprite**);
    IsleGameResult (*sprite_remove)(IsleReportSource*, IsleGameObject*);
    IsleGameResult (*sprite_detach)(IsleReportSource*, IsleGameSprite*, int32_t);
    IsleGameResult (*sprite_bitmap)(IsleReportSource*, IsleGameSprite*, const uint8_t*, size_t);
    IsleGameResult (*thumbnail_bind)(IsleReportSource*, int32_t, IsleGameSprite*, IslePostCourseThumbnail*, uint8_t**, size_t*);
    IsleGameResult (*preview)(IsleReportSource*, const IslePlayerPhotoData*, IsleGameSprite*);
    IsleGameResult (*window_mode)(IsleReportSource*, int32_t, int32_t);
    IsleGameResult (*render_bind)(IsleReportSource*, IsleReportGraphics**);
    IsleGameResult (*render_state)(IsleReportSource*, int32_t, int32_t);
    IsleGameResult (*setup_begin)(IsleReportSource*, int32_t);
    IsleGameResult (*setup_poll)(void*, int*, int32_t*);
    IsleGameResult (*notices_or_owner)(void*, uint32_t);
} IsleReportNative;
struct IsleReportSource {
    IslePostCourseSceneSource common;
    const IsleReportNative* native;
    const IsleReportData* data;
    IsleReportCompare D_80202D2C_9F969C;
    int32_t D_80202D30_9F96A0;
    int32_t D_80202EC0_9F9830;
    int32_t D_80202EC4_9F9834;
    int32_t D_80202FF8_9F9968;
    int32_t D_80202FFC_9F996C;
    int32_t D_80203000_9F9970;
    int32_t D_80203004_9F9974;
    int32_t D_80203008_9F9978;
    int32_t D_8020300C_9F997C;
    int32_t D_80203014_9F9984;
    IsleGameObject* D_80230C10_A27580;
    IsleGameSprite* D_80230C14_A27584;
    uint32_t D_80230C18_A27588;
    IsleReportElement* D_80230C1C_A2758C;
    IslePostCourseScoredPhoto* D_80230C20_A27590;
    int32_t D_80230C24_A27594;
    IsleReportFrame* D_80230C28_A27598;
    uint32_t D_80230C2C_A2759C;
    IsleReportElement* D_80230C30_A275A0;
    IsleReportElement* D_80230C34_A275A4;
    IsleReportElement* D_80230C38_A275A8;
    IsleGameSprite* D_80230C3C_A275AC;
    IsleGameObject* D_80230C40_A275B0;
    IsleGameSprite* D_80230C48_A275B8[6];
    IsleGameObject* D_80230C60_A275D0;
    IsleGameSprite* D_80230C68_A275D8[2];
    IsleGameObject* D_80230C70_A275E0;
    int32_t D_80230C74_A275E4;
    IslePostCourseScoredPhoto* D_80230C78_A275E8;
    IsleReportElement* D_80230C80_A275F0[10];
    int32_t D_80230CA8_A27618;
    int32_t D_80230CAC_A2761C;
    int32_t D_80230CB0_A27620[63];
    int32_t D_80230DAC_A2771C;
    int32_t D_80230DB0_A27720;
    int32_t D_80230DB4_A27724;
    int32_t D_80230DB8_A27728;
    int32_t D_80230DBC_A2772C;
    int32_t D_80230DC0_A27730;
    IsleReportTriple D_80230DC8_A27738[6];

    const IslePostCourseButton* D_80230E10_A27780;
    IslePostCourseWindow* D_80230E14_A27784;
    IsleReportCompare sort_compare;
    IsleReportElement elements[32];
    uint8_t element_live[32];
    IsleReportFrame frame;
    IsleReportInput input;
    IslePostCourseScoredPhoto score_scratch;
    char formatted[400], number_ascii[16], number_source[34];
};
typedef struct IsleReportEntry {
    IsleReportSource* source;
    int32_t argument, result;
    IsleGameResult status;
} IsleReportEntry;
void isle_report_source_initialize(IsleReportSource*);
void isle_report_source_init(IsleReportSource*);
void isle_report_source_init_call(IslePostCourseSceneSource*, void*);
void isle_report_source_entry(void*);
void isle_report_func_801DCA14_9D3384(IsleGameObject*);
void isle_report_func_801E24B4_9D8E24(IsleGameObject*);
#ifdef __cplusplus
#define ISLE_REPORT_NATIVE_NOEXCEPT noexcept
#else
#define ISLE_REPORT_NATIVE_NOEXCEPT
#endif
IsleGameResult isle_report_native_preview_consume(IsleReportSource*,IsleGameSprite*,const IslePostCoursePreview*) ISLE_REPORT_NATIVE_NOEXCEPT;
IsleGameResult isle_report_native_sprite_rebind(IsleReportSource*,IsleGameSprite*,uint32_t) ISLE_REPORT_NATIVE_NOEXCEPT;
IsleGameResult isle_report_native_thumbnail_publish(IsleReportSource*,int32_t) ISLE_REPORT_NATIVE_NOEXCEPT;
IsleGameResult isle_report_native_preview_scale(IsleReportSource*,float*) ISLE_REPORT_NATIVE_NOEXCEPT;
#undef ISLE_REPORT_NATIVE_NOEXCEPT
#ifdef __cplusplus
}
#endif
#endif
