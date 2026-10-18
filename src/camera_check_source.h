#ifndef ISLE_CAMERA_CHECK_SOURCE_H
#define ISLE_CAMERA_CHECK_SOURCE_H
#include "post_course_scene_source.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct IsleCameraCheckSource IsleCameraCheckSource;
typedef struct IsleCameraCheckElement { IslePostCourseElement handle; } IsleCameraCheckElement;
typedef struct IsleCameraCheckFrame { IslePostCourseFrame handle; } IsleCameraCheckFrame;
typedef struct IsleCameraCheckButton { int32_t id; const char* text; } IsleCameraCheckButton;
/* Shared Window globals, initialized from admitted data and preserved across
 * overlays. This is a borrow, not a second delayed-music implementation. */
typedef struct IsleCameraCheckMusicState {
    int32_t countdown,song,mode;
    IsleGameObject* object;
} IsleCameraCheckMusicState;
typedef struct IsleCameraCheckInput { uint32_t currentButtons, pressedButtons; } IsleCameraCheckInput;
typedef int32_t (*IsleCameraCheckCompare)(IsleCameraCheckSource*,const void*,const void*);
/* Original signed one-bit fields deliberately retain 0/-1 comparisons. */
typedef struct IsleCameraCheckPhoto {
    const IslePlayerPhotoData* unk_00;
    int32_t unk_04;
    int16_t unk_08,unk_0A,unk_0C,unk_0E,unk_10,unk_12,unk_14,unk_16;
    signed int unk_18_0x80000000:1,unk_18_0x40000000:1,unk_18_0x20000000:1,
        unk_18_0x10000000:1,unk_18_0x08000000:1,unk_18_0x04000000:1,
        unk_18_0x02000000:1,unk_18_0x01000000:1;
} IsleCameraCheckPhoto;
typedef struct IsleCameraCheckGroup {
    IsleGameObject* unk_00;
    IsleGameSprite *unk_04,*unk_08,*unk_0C,*unk_10,*unk_14,*unk_18,*unk_1C;
} IsleCameraCheckGroup;
/* Common MUST remain first. All other fields are overlay BSS/initialized data
 * or stable handle cells; no canonical save, Window or native sprite copies. */
struct IsleCameraCheckSource {
    IslePostCourseSceneSource common;
    IsleCameraCheckGroup D_camera_check_802498F0,D_camera_check_802499C0[7];
    IsleCameraCheckElement *D_camera_check_80249910,*D_camera_check_80249914,
        *D_camera_check_80249918,*D_camera_check_8024991C;
    IsleCameraCheckFrame* D_camera_check_80249920;
    char D_camera_check_80249928[128];
    IsleGameSprite *D_camera_check_802499A8,*D_camera_check_802499B0,
        *D_camera_check_802499B4,*D_camera_check_802499B8,*D_camera_check_802499BC;
    float D_camera_check_80208944,D_camera_check_80208948,D_camera_check_80208950;
    int32_t D_camera_check_80208960,D_camera_check_80208964;
    IsleCameraCheckButton D_camera_check_8020896C[5],D_camera_check_80208994[5],D_camera_check_802089BC[5];
    int32_t D_camera_check_802089E4,D_camera_check_802089E8,D_camera_check_802089EC;
    int32_t D_camera_check_802089F0;
    IsleCameraCheckCompare D_camera_check_802089F4;
    void (*D_camera_check_80249AA0)(IsleCameraCheckSource*,int32_t,int32_t);
    IsleCameraCheckButton* D_camera_check_80249AA4;
    /* 80249AA8 is a live Window selector, not a copied UIButton array. */
    char D_camera_check_80249AB0[128];
    IsleCameraCheckPhoto D_camera_check_80249B30[60];
    int32_t D_camera_check_8024A1C0,D_camera_check_8024A1C4;
    uint8_t D_camera_check_8024A1C8[9];
    int32_t D_camera_check_8024A1D4,D_camera_check_8024A1D8;
    IsleCameraCheckElement elements[8];
    size_t element_count;
    IsleCameraCheckFrame frame;
    IslePostCourseButton table_entries[3][5];
    IslePostCourseButtonTable tables[3];
    IslePostCourseThumbnail thumbnails[8];
    IsleGameSprite* thumbnail_sprites[8];
    size_t thumbnail_count;
    IslePostCourseScoredPhoto scored;
    char formatted[400];
    IsleCameraCheckMusicState* music;
    /* Set argument after initialize. Entry callback and its whole context must
     * outlive sceneStorage; entry_complete is not a Game STOPPED flag. */
    int32_t argument,source_result;
    int entry_complete;
};

/* Called once at actual overlay admission. Does not reset Photos or World.
 * initialize/init are nonwaiting; init is entered through source_call. */
void isle_camera_check_source_initialize(IsleCameraCheckSource*);
void isle_camera_check_source_init(IslePostCourseSceneSource*,void*);
int32_t func_camera_check_801DCACC(IsleCameraCheckSource*,int32_t);
void isle_camera_check_source_entry(void*);
void func_camera_check_801DC8D0(IsleGameObject*);
void func_camera_check_801DF2D8(IsleGameObject*);
void func_camera_check_801DDF28(IsleGameObject*);
void func_80375224_8489D4(IsleGameObject*);
/* Releases real raster bindings only after actual graphics retirement. Owner
 * first drains Replay, joins all Source callbacks, and cancels Window ops. */
IsleGameResult isle_camera_check_source_release(IsleCameraCheckSource*);

/* Returning native owner leaves. No leaf calls Source, waits, ticks OM/GTL,
 * pumps Runtime, grants retrace credit, or casts object userData to context.
 * A failure is transferred only by the C caller's checked fence. */
IsleGameResult isle_camera_check_native_setup_begin(IsleCameraCheckSource*,int32_t argument,
    uint32_t screen_address,uint32_t setup_address,uint32_t screen_flags,
    int near_clipping_disabled,int update_interval,int draw_interval);
/* Owner resumes independent entry only on service handoff/actual retirement;
 * complete is never derived from STOPPED. result is original signed AF3C0. */
IsleGameResult isle_camera_check_native_setup_poll(IsleCameraCheckSource*,int* complete,int32_t* result);
/* Original 801DC910 camera: id0x200, update/link5/priority5, draw priority0x14,
 * mask/tag0x100000, default matrices/default flags; final flags4,
 * viewport0,0,640,480, up0,0,1, canonical eye/at, bg0x7101,
 * scale1/far8000/fovy33/near62.5, unk38=0x100000. NO Source process;
 * C installs original idle callback kind0/priority0 after this leaf returns. */
IsleGameResult isle_camera_check_native_camera(IsleCameraCheckSource*,IsleGameObject**);
IsleGameResult isle_camera_check_native_ui_heap(IsleCameraCheckSource*,size_t bytes);
IsleGameResult isle_camera_check_native_input_initialize(IsleCameraCheckSource*);
IsleGameResult isle_camera_check_native_input(IsleCameraCheckSource*,int controller,int neutral,
    const IsleCameraCheckInput**);
IsleGameResult isle_camera_check_native_repeat(IsleCameraCheckSource*,int,int);
IsleGameResult isle_camera_check_native_input_mask(IsleCameraCheckSource*,uint32_t);
/* 800A85E8 owner creation only; callback installed in C as kind0/priority0.
 * 53AD0.c151: id1<<link, update link/priority link, draw tag1<<draw_link,
 * render link/priority supplied and rotation RPY, NULL display list. */
IsleGameResult isle_camera_check_native_process_owner(IsleCameraCheckSource*,int link,int draw_link,IsleGameObject**);
IsleGameResult isle_camera_check_native_replay_initialize(IsleCameraCheckSource*);
IsleGameResult isle_camera_check_native_replay_cleanup(IsleCameraCheckSource*);
/* Actual Window D_803A6668_879E18; decode/adopt its original value, not a
 * replacement zoom default. Native leaf does not run preview or Source. */
IsleGameResult isle_camera_check_native_preview_scale(IsleCameraCheckSource*,float*);
IsleGameResult isle_camera_check_native_sound(IsleCameraCheckSource*,int id,int volume,int pan,float pitch,int reverb,int32_t* handle);
IsleGameResult isle_camera_check_native_song(IsleCameraCheckSource*,int player,int song);
IsleGameResult isle_camera_check_native_volume(IsleCameraCheckSource*,int player,int volume,int frames);
IsleGameResult isle_camera_check_native_flag(IsleCameraCheckSource*,int id,int*);
IsleGameResult isle_camera_check_native_set_flag(IsleCameraCheckSource*,int id,int value);
IsleGameResult isle_camera_check_native_level(IsleCameraCheckSource*,int32_t*);
IsleGameResult isle_camera_check_native_roll_count(IsleCameraCheckSource*,int32_t*);
IsleGameResult isle_camera_check_native_music_state(IsleCameraCheckSource*,IsleCameraCheckMusicState**);
/* Stable Source-encoded byte strings, never UTF-8 or temporary std::strings.
 * name kind0=course,1=species,2=player; course may genuinely return NULL. */
IsleGameResult isle_camera_check_native_name(IsleCameraCheckSource*,int kind,int32_t id,const char**);
IsleGameResult isle_camera_check_native_course_score(IsleCameraCheckSource*,int level,int32_t*);
IsleGameResult isle_camera_check_native_set_course_score(IsleCameraCheckSource*,int level,int32_t);
/* Original 803A66A0 seven-entry initialized challenge table, not defaults. */
IsleGameResult isle_camera_check_native_challenge_score(IsleCameraCheckSource*,int level,int32_t*);
IsleGameResult isle_camera_check_native_transition(IsleCameraCheckSource*,int scene);
IsleGameResult isle_camera_check_native_notice(IsleCameraCheckSource*,int notice);
/* Caption ROM scratchpointer proof establishes populated branch when the
 * immediately preceding scorer has nonzero pokemonInFocus, including zero
 * totalScore signs. Zero-focus Report path is not proven; owner must supply an
 * explicit authorized policy or return failure, NEVER a guessed default. */
IsleGameResult isle_camera_check_native_zero_focus_caption(IsleCameraCheckSource*,const IslePlayerPhotoData*,int* populated);
#ifdef __cplusplus
}
#endif
#endif
