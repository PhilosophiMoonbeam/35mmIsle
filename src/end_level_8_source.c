#include "end_level_8_source.h"
#include <string.h>
#include <stdbool.h>
enum { FRAME_STYLE_1=1, UI_FOREGROUND=1, UI_BACKGROUND=0, FONT_12=12,
       UI_NORMAL=0, UI_HIDDEN=1, A_BUTTON=0x8000, B_BUTTON=0x4000,
       Z_TRIG=0x2000, SOUND_ID_67=67, SOUND_ID_66=66, SCENE_GALLERY_2=23 };
typedef IsleEndLevel8Source Context;
typedef IsleGameObject GObj;
typedef IsleEndLevel8Input UnkStruct800BEDF8;
typedef IslePostCourseColor ucolor;
typedef int32_t s32;
typedef int8_t s8;
#define e8_check(s,r) isle_post_course_source_checked(&(s)->common,(r),__func__,#r,__LINE__)
#include "end_level_8_declarations.inc"
/* Parent implements this owner-only returning Host/Frame/GPU/readback service.
 * It grants NO OM/GTL update and never joins the currently executing callback. */
IsleGameResult isle_post_course_native_progress(IslePostCourseSceneSource*);
static void e8_wait(Context* s,int n) { isle_post_course_source_wait(&s->common,n); }
static void e8_hidden(Context* s,int hidden) { e8_check(s,s->requiredNativeTable->hidden(s,s->D_801DD240_A08E30,hidden)); }
typedef IsleGameResult (*E8Drain)(Context*,int*);
static void e8_drain(Context* s,E8Drain poll) {
    int done=0;
    IsleGameResult first=ISLE_GAME_OK,result;
    do {
        result=poll(s,&done);
        if (first==ISLE_GAME_OK && result!=ISLE_GAME_OK) first=result;
        if (!done) {
            result=isle_post_course_native_progress(&s->common);
            if (first==ISLE_GAME_OK && result!=ISLE_GAME_OK) first=result;
        }
    } while (!done);
    /* A sticky fault is not a fence. Transfer only after actual drain. */
    e8_check(s,first);
}
static void e8_join_graphics(Context* s) { e8_drain(s,s->requiredNativeTable->join_graphics); }
static void e8_replay_cleanup(Context* s) { e8_drain(s,s->requiredNativeTable->replay_cleanup); }
static void e8_photo(Context* s,int index) {
    IslePhotoRecord record={0}; int ready=0;
    e8_check(s,isle_application_photos_temporary(s->common.photos,index,&record));
    e8_drain(s,s->requiredNativeTable->presentation_clear);
    e8_check(s,s->requiredNativeTable->presentation_begin(s,s->D_801DD244_A08E34,&record));
    while (!ready) {
        e8_check(s,s->requiredNativeTable->presentation_poll(s,&ready));
        if (!ready) e8_check(s,isle_post_course_native_progress(&s->common));
    }
}
static void e8_camera(Context* s) { e8_check(s,s->requiredNativeTable->camera(s,&s->D_801DD240_A08E30,&s->D_801DD244_A08E34)); }
static void e8_process(Context* s,int link,int dl,void* list) {
    IsleGameObject* obj=NULL; IsleGameProcess* process=NULL;
    e8_check(s,s->requiredNativeTable->process_object(s,link,dl,list,&obj));
    e8_check(s,isle_game_add_source_process(s->common.game,obj,isle_end_level_8_func_801DC9E8_A085D8,ISLE_GAME_STACKFUL,0,&process));
}
static void e8_input_initialize(Context* s) {
    e8_check(s,s->requiredNativeTable->input_initialize(s));
    s->common.input_invalidate=isle_post_course_native_input_invalidate;
    e8_check(s,isle_post_course_source_install_input(&s->common));
}
static UnkStruct800BEDF8* e8_input(Context* s,int id) { e8_check(s,s->requiredNativeTable->input(s,id,&s->input)); return &s->input; }
static int e8_fade_pending(Context* s) { int32_t n=0; e8_check(s,s->requiredNativeTable->fade_pending(s,&n)); return n; }
static void e8_UIFrame_SetStyle(Context* s,int style) { e8_check(s,isle_post_window_frame_style(s->common.window,s->frame,style)); }
static void e8_UIFrame_SetPos(Context* s,int x,int y) { e8_check(s,isle_post_window_frame_position(s->common.window,s->frame,x,y)); }
static void e8_UIFrame_Show(Context* s,int v) { e8_check(s,isle_post_window_frame_visible(s->common.window,s->frame,v)); }
static void e8_UIFrame_ShowBackground(Context* s,int v) { e8_check(s,isle_post_window_frame_background(s->common.window,s->frame,v)); }
static void e8_UIElement_SetState(Context* s,int v) { e8_check(s,isle_post_window_element_visible(s->common.window,s->element,v==0)); }
static void e8_UIElement_SetFont(Context* s,int v) { e8_check(s,isle_post_window_element_font(s->common.window,s->element,v)); }
static void e8_color(Context* s,int which,int r,int g,int b,int a) { IslePostCourseColor c={(uint8_t)r,(uint8_t)g,(uint8_t)b,(uint8_t)a}; e8_check(s,isle_post_window_element_color(s->common.window,s->element,which,c)); }
static void e8_print(Context* s,const char* text) { IslePostCourseBytes b={text,strlen(text)}; e8_check(s,isle_post_window_print(s->common.window,s->element,b,0,NULL)); }
#include "end_level_8_original.inc"
#include "end_level_8_initial_data.inc"
void isle_end_level_8_source_initialize(Context* s,uint64_t generation) {
    IslePostCourseSceneSource common=s->common; const IsleEndLevel8Native* native=s->requiredNativeTable;
    if (!native || !native->admit_initial_data || !native->camera || !native->hidden || !native->join_graphics ||
        !native->presentation_begin || !native->presentation_poll || !native->presentation_clear ||
        !native->replay_initialize || !native->replay_cleanup || !native->fade ||
        !native->fade_pending || !native->input || !native->input_initialize ||
        !native->sound || !native->volume || !native->aux_camera || !native->ui_heap ||
        !native->depth_screen || !native->deallocate || !native->process_object ||
        !native->scene_result || !native->entry_begin || !native->entry_poll || !generation) {
        e8_check(s,ISLE_GAME_INVALID_STATE);
        return;
    }
    if (s->module_generation==generation) return;
    e8_check(s,native->admit_initial_data(s,&e8_initial_data));
    memset(s,0,sizeof(*s)); s->common=common; s->requiredNativeTable=native; s->module_generation=generation; s->common.scene=24;
}
void isle_end_level_8_source_init_call(IslePostCourseSceneSource* c,void* user) { (void)user; isle_end_level_8_func_801DCFE0_A08BD0((Context*)c); }
void isle_end_level_8_source_entry(void* user) {
    IsleEndLevel8Entry* e=user; Context* s=e->source;
    IsleContinuationDomain* domain=s->common.domain; void* owner=s->common.owner;
    const IsleEndLevel8Native* native=s->requiredNativeTable; int retired=0; int32_t result=0;
    s->D_801DD250_A08E40=0x10000000; s->D_801DD24C_A08E3C=0;
    e->status=native->scene_result(s,23);
    if (e->status==ISLE_GAME_OK) e->status=native->entry_begin(s,e->argument);
    if (e->status!=ISLE_GAME_OK) { (void)isle_continuation_fail(domain,"End8 setup"); return; }
    while (!retired) {
        if (isle_continuation_wait(domain,1)!=ISLE_CONT_OK) { e->status=ISLE_GAME_INVALID_STATE; (void)isle_continuation_fail(domain,"End8 wait"); return; }
        e->status=native->entry_poll(owner,&retired,&result);
        if (e->status!=ISLE_GAME_OK) { (void)isle_continuation_fail(domain,"End8 retirement"); return; }
    }
    e->result=result;
}
