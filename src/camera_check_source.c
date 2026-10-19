#include "camera_check_source.h"
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* SDK-free names are local translation vocabulary, not guest structures. */
typedef int32_t s32;
typedef int16_t s16;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef float f32;
typedef IsleGameObject GObj;
typedef IsleGameSprite SObj;
typedef IsleCameraCheckElement UIElement;
typedef IsleCameraCheckFrame UIFrame;
typedef IsleCameraCheckButton UIButton;
typedef IsleCameraCheckInput UnkStruct800BEDF8;
typedef IsleCameraCheckPhoto UnkIndigoHalibut;
typedef IsleCameraCheckGroup UnkTealFlounder;
typedef IslePlayerPhotoData PhotoData;
typedef IslePostCourseScoredPhoto Unk803A6C18;
typedef IslePostCourseColor ucolor;
#define UNUSED
#define UNK_TYPE int32_t
#define PI 3.14159265358979323846f
#define TAU 6.28318530717958647692f
#define SP_SCALE ISLE_SP_SCALE
#define SP_HIDDEN ISLE_SP_HIDDEN
#define SP_TRANSPARENT ISLE_SP_TRANSPARENT
/* Numeric Source constants are emitted only after pinned header validation. */
#include "camera_check_constants.inc"

#define cc_checked(source, result) \
    isle_post_course_source_checked(&((source)->common), (result), __func__, #result, __LINE__)
static void cc_wait_at(IsleCameraCheckSource* c,int updates,const char* caller) {
    if (!strcmp(caller,"func_80374F30_8486E0"))
        isle_post_course_ack_wait(&c->common,updates);
    else isle_post_course_source_wait(&c->common,updates);
}
#define cc_wait(c,updates) cc_wait_at(c,updates,__func__)
static IsleSprite* cc_sprite(IsleCameraCheckSource* c,SObj* s) {
    IsleSprite* out=NULL;
    cc_checked(c,isle_post_window_sprite_view(c->common.window,s,&out));
    return out;
}
static SObj* cc_root(IsleCameraCheckSource* c,GObj* g) {
    SObj* out=NULL;
    cc_checked(c,isle_post_window_sprite_root(c->common.window,g,&out));
    return out;
}
static int cc_aux(IsleCameraCheckSource* c,SObj* s) {
    int32_t out=0;
    cc_checked(c,isle_post_window_sprite_aux_get(c->common.window,s,&out));
    return out;
}
static void cc_set_aux(IsleCameraCheckSource* c,SObj* s,int value) {
    cc_checked(c,isle_post_window_sprite_aux_set(c->common.window,s,value));
}
static GObj* cc_sprite_object(IsleCameraCheckSource* c,IsleGameSourceCallback callback,int link,uint32_t prototype) {
    GObj* g=NULL; SObj* s=NULL;
    cc_checked(c,isle_post_window_sprite_object(c->common.window,link,prototype,callback,1,&g,&s));
    return g;
}
static GObj* cc_plain_sprite(IsleCameraCheckSource* c,int unused,int link,uint32_t prototype) {
    (void)unused;
    return cc_sprite_object(c,NULL,link,prototype);
}
static SObj* cc_child(IsleCameraCheckSource* c,SObj* parent,uint32_t prototype) {
    SObj* out=NULL;
    cc_checked(c,isle_post_window_sprite_child(c->common.window,parent,prototype,&out));
    return out;
}
static SObj* cc_sibling(IsleCameraCheckSource* c,SObj* parent,uint32_t prototype) {
    SObj* out=NULL;
    cc_checked(c,isle_post_window_sprite_sibling(c->common.window,parent,prototype,&out));
    return out;
}
static void cc_bind_thumbnail(IsleCameraCheckSource* c,SObj* s,uint32_t prototype) {
    size_t i=c->thumbnail_count;
    if(i>=8) { cc_checked(c,ISLE_GAME_INVALID_STATE); return; }
    cc_checked(c,isle_post_window_thumbnail_create(c->common.window,s,prototype,&c->thumbnails[i]));
    c->thumbnail_sprites[i]=s;
    ++c->thumbnail_count;
}
typedef struct CameraPreviewConsumer {
    IsleCameraCheckSource* source;
    IslePostCourseThumbnail thumbnail;
} CameraPreviewConsumer;
static IsleGameResult cc_consume_preview(void* user,const IslePostCoursePreview* view) {
    CameraPreviewConsumer* p=user;
    if(!view) return isle_post_window_thumbnail_clear(p->source->common.window,p->thumbnail);
    return isle_post_window_thumbnail_write_photo(p->source->common.window,p->thumbnail,
        view->rgba5551,view->size,view->stride_bytes);
}
static int cc_preview(IsleCameraCheckSource* c,const PhotoData* photo,SObj* sprite) {
    size_t i; CameraPreviewConsumer p;
    float scale=0.0f;
    for(i=0;i<c->thumbnail_count;++i) if(c->thumbnail_sprites[i]==sprite) break;
    if(i==c->thumbnail_count) { cc_checked(c,ISLE_GAME_INVALID_STATE); return 1; }
    p.source=c; p.thumbnail=c->thumbnails[i];
    /* 847B60.c:131 requests twice the live Sprite dimensions.
     * Replay owns the original 16..320 / 16..210 viewport clamps. */
    { int width=cc_sprite(c,sprite)->width*2;
      int height=cc_sprite(c,sprite)->height*2;
      cc_checked(c,isle_camera_check_native_preview_scale(c,&scale));
      cc_checked(c,isle_post_replay_preview(c->common.replay,photo,width,height,scale,cc_consume_preview,&p)); }
    return photo==NULL;
}
static const Unk803A6C18* cc_score(IsleCameraCheckSource* c,const PhotoData* photo) {
    int present=0;
    cc_checked(c,isle_post_replay_score(c->common.replay,photo,&c->scored,&present));
    if(!present) cc_checked(c,ISLE_GAME_INVALID_STATE);
    return &c->scored;
}
static UIElement* cc_element_create(IsleCameraCheckSource* c,int x,int y,int w,int h,uint32_t flags) {
    UIElement* out;
    if(c->element_count>=8) { cc_checked(c,ISLE_GAME_INVALID_STATE); return NULL; }
    out=&c->elements[c->element_count++];
    cc_checked(c,isle_post_window_element_create(c->common.window,x,y,w,h,flags,&out->handle));
    return out;
}
static void cc_element_delete(IsleCameraCheckSource* c,UIElement* e) { cc_checked(c,isle_post_window_element_delete(c->common.window,e->handle)); }
static void cc_element_clear(IsleCameraCheckSource* c,UIElement* e) { cc_checked(c,isle_post_window_element_clear(c->common.window,e->handle)); }
static void cc_element_background(IsleCameraCheckSource* c,UIElement* e) { cc_checked(c,isle_post_window_element_background(c->common.window,e->handle)); }
static void cc_element_font(IsleCameraCheckSource* c,UIElement* e,int font) { cc_checked(c,isle_post_window_element_font(c->common.window,e->handle,font)); }
static void cc_element_position(IsleCameraCheckSource* c,UIElement* e,int x,int y) { cc_checked(c,isle_post_window_element_text_position(c->common.window,e->handle,x,y)); }
static void cc_element_state(IsleCameraCheckSource* c,UIElement* e,int state) { if(e==NULL)return; /* Original UIElement_SetState accepts NULL. */ cc_checked(c,isle_post_window_element_visible(c->common.window,e->handle,state==UI_NORMAL)); }
static void cc_element_color(IsleCameraCheckSource* c,UIElement* e,int which,int r,int g,int b,int a) { cc_checked(c,isle_post_window_element_color(c->common.window,e->handle,which==UI_FOREGROUND,(IslePostCourseColor){r,g,b,a})); }
static void cc_element_fill(IsleCameraCheckSource* c,UIElement* e,int x1,int y1,int x2,int y2,int r,int g,int b,int a) { cc_checked(c,isle_post_window_element_fill(c->common.window,e->handle,x1,y1,x2,y2,(IslePostCourseColor){r,g,b,a})); }
static void cc_element_fill_default(IsleCameraCheckSource* c,UIElement* e,int x1,int y1,int x2,int y2) { cc_checked(c,isle_post_window_element_fill_default(c->common.window,e->handle,x1,y1,x2,y2)); }
static void cc_print(IsleCameraCheckSource* c,UIElement* e,const char* text) { cc_checked(c,isle_post_window_print(c->common.window,e->handle,(IslePostCourseBytes){text,strlen(text)},0,NULL)); }
static int cc_printf(IsleCameraCheckSource* c,UIElement* e,const char* format,...) {
    int n; va_list ap;
    va_start(ap,format); n=vsnprintf(c->formatted,sizeof(c->formatted),format,ap); va_end(ap);
    if(n<0 || (size_t)n>=sizeof(c->formatted)) cc_checked(c,ISLE_GAME_INVALID_STATE);
    cc_print(c,e,c->formatted); return n;
}
static int cc_width(IsleCameraCheckSource* c,const char* text) { int out=0; cc_checked(c,isle_post_window_text_width(c->common.window,(IslePostCourseBytes){text,strlen(text)},&out)); return out; }
static void cc_spacing(IsleCameraCheckSource* c,int x,int y) { cc_checked(c,isle_post_window_text_spacing(c->common.window,x,y)); }
static void cc_shadow(IsleCameraCheckSource* c,int n) { cc_checked(c,isle_post_window_text_shadow_offset(c->common.window,n)); }
static void cc_delay(IsleCameraCheckSource* c,int n) { cc_checked(c,isle_post_window_text_delay(c->common.window,n)); }
static void cc_skip(IsleCameraCheckSource* c,int n) { cc_checked(c,isle_post_window_text_skip(c->common.window,n)); }
static void cc_text_init(IsleCameraCheckSource* c) { cc_checked(c,isle_post_window_text_initialize(c->common.window)); }
static UIFrame* cc_frame_create(IsleCameraCheckSource* c) { cc_checked(c,isle_post_window_frame_create(c->common.window,&c->frame.handle)); return &c->frame; }
static void cc_frame_style(IsleCameraCheckSource* c,UIFrame* f,int style) { cc_checked(c,isle_post_window_frame_style(c->common.window,f->handle,style)); }
static void cc_frame_position(IsleCameraCheckSource* c,UIFrame* f,int x,int y) { cc_checked(c,isle_post_window_frame_position(c->common.window,f->handle,x,y)); }
static void cc_frame_show(IsleCameraCheckSource* c,UIFrame* f,int n) { cc_checked(c,isle_post_window_frame_visible(c->common.window,f->handle,n)); }
static void cc_frame_color(IsleCameraCheckSource* c,UIFrame* f,ucolor* out) { cc_checked(c,isle_post_window_frame_color(c->common.window,f->handle,out)); }
static void cc_fade_in(IsleCameraCheckSource* c,UIFrame* f,int style) { cc_checked(c,isle_post_window_fade_in(c->common.window,f->handle,style)); }
static void cc_fade_out(IsleCameraCheckSource* c,UIFrame* f) { cc_checked(c,isle_post_window_fade_out(c->common.window,f->handle)); }
#define CC_WINDOW0(name,leaf) static void name(IsleCameraCheckSource* c) { cc_checked(c,leaf(c->common.window)); }
#define CC_WINDOW1(name,leaf) static void name(IsleCameraCheckSource* c,int n) { cc_checked(c,leaf(c->common.window,n)); }
#define CC_WINDOW2(name,leaf) static void name(IsleCameraCheckSource* c,int x,int y) { cc_checked(c,leaf(c->common.window,x,y)); }
CC_WINDOW0(cc_focus_create,isle_post_window_focus_create)
CC_WINDOW0(cc_focus_front,isle_post_window_focus_front)
CC_WINDOW1(cc_focus_show,isle_post_window_focus_visible)
CC_WINDOW2(cc_focus_position,isle_post_window_focus_position)
CC_WINDOW2(cc_focus_size,isle_post_window_focus_size)
CC_WINDOW2(cc_focus_target_position,isle_post_window_focus_target_position)
CC_WINDOW2(cc_focus_target_size,isle_post_window_focus_target_size)
CC_WINDOW0(cc_buttons_clear,isle_post_window_buttons_clear)
CC_WINDOW0(cc_layout_init,isle_post_window_layout_initialize)
CC_WINDOW0(cc_layout_hide,isle_post_window_layout_hide)
CC_WINDOW0(cc_layout_wait,isle_post_window_layout_wait)
CC_WINDOW1(cc_header_show,isle_post_window_layout_show_header)
CC_WINDOW1(cc_header_hide,isle_post_window_layout_hide_header)
CC_WINDOW1(cc_header_flags,isle_post_window_layout_header_flags)
CC_WINDOW1(cc_panel,isle_post_window_layout_panel)
CC_WINDOW2(cc_disable,isle_post_window_layout_disable)
CC_WINDOW2(cc_disable_except,isle_post_window_layout_disable_except)
#undef CC_WINDOW0
#undef CC_WINDOW1
#undef CC_WINDOW2
static int cc_slot(IsleCameraCheckSource* c,int i) { int out=0; cc_checked(c,isle_post_window_layout_slot_id(c->common.window,(size_t)i,&out)); return out; }
static IslePostCourseButtonTable cc_table(IsleCameraCheckSource* c,UIButton* b) {
    size_t i,k;
    if(b==c->D_camera_check_8020896C) i=0;
    else if(b==c->D_camera_check_80208994) i=1;
    else if(b==c->D_camera_check_802089BC) i=2;
    else { cc_checked(c,ISLE_GAME_INVALID_STATE); return (IslePostCourseButtonTable){0}; }
    for(k=0;k<5;++k) c->table_entries[i][k]=(IslePostCourseButton){b[k].id,{b[k].text,b[k].text?strlen(b[k].text):0}};
    cc_checked(c,isle_post_window_table_borrow(c->common.window,c->table_entries[i],5,&c->tables[i]));
    return c->tables[i];
}
static void cc_layout_create(IsleCameraCheckSource* c,UIButton* b) { IslePostCourseButtonTable table=cc_table(c,b); cc_checked(c,isle_post_window_layout_create(c->common.window,table)); }
static const char* cc_button_text(IsleCameraCheckSource* c,UIButton* b,int id) { IslePostCourseBytes out={0}; IslePostCourseButtonTable table=cc_table(c,b); cc_checked(c,isle_post_window_table_text(c->common.window,table,id,&out)); return out.data; }
static const UnkStruct800BEDF8* cc_input_at(IsleCameraCheckSource* c,int controller,const char* caller) {
    const IsleCameraCheckInput* out=NULL;
    cc_checked(c,isle_camera_check_native_input(c,controller,0,&out));
    if (!strcmp(caller,"func_80374F30_8486E0"))
        isle_post_course_ack_input(&c->common,out->currentButtons,out->pressedButtons);
    return out;
}
#define cc_input(c,controller) cc_input_at(c,controller,__func__)
static const UnkStruct800BEDF8* cc_neutral(IsleCameraCheckSource* c,int controller) { const IsleCameraCheckInput* out=NULL; cc_checked(c,isle_camera_check_native_input(c,controller,1,&out)); return out; }
static void cc_repeat(IsleCameraCheckSource* c,int a,int b) { cc_checked(c,isle_camera_check_native_repeat(c,a,b)); }
static void cc_mask(IsleCameraCheckSource* c,uint32_t n) { cc_checked(c,isle_camera_check_native_input_mask(c,n)); }
static int cc_flag(IsleCameraCheckSource* c,int id) { int out=0; cc_checked(c,isle_camera_check_native_flag(c,id,&out)); return out; }
static void cc_set_flag(IsleCameraCheckSource* c,int id,int n) { cc_checked(c,isle_camera_check_native_set_flag(c,id,n)); }
static int cc_level(IsleCameraCheckSource* c) { int32_t out=0; cc_checked(c,isle_camera_check_native_level(c,&out)); return out; }
static const char* cc_name(IsleCameraCheckSource* c,int kind,int id) { const char* out=NULL; cc_checked(c,isle_camera_check_native_name(c,kind,id,&out)); return out; }
static const char* cc_level_name(IsleCameraCheckSource* c,int id) { return cc_name(c,0,id); }
static const char* cc_pokemon_name(IsleCameraCheckSource* c,int id) { return cc_name(c,1,id); }
static const char* cc_player_name(IsleCameraCheckSource* c) { return cc_name(c,2,0); }
static int cc_course_score(IsleCameraCheckSource* c,int id) { int32_t out=0; cc_checked(c,isle_camera_check_native_course_score(c,id,&out)); return out; }
static void cc_set_course_score(IsleCameraCheckSource* c,int id,int n) { cc_checked(c,isle_camera_check_native_set_course_score(c,id,n)); }
static int cc_challenge(IsleCameraCheckSource* c,int id) { int32_t out=0; if(id<0 || id>=7) return 0; cc_checked(c,isle_camera_check_native_challenge_score(c,id,&out)); return out; }
static int cc_sound_params(IsleCameraCheckSource* c,int id,int volume,int pan,float pitch,int reverb) { int32_t out=0; cc_checked(c,isle_camera_check_native_sound(c,id,volume,pan,pitch,reverb,&out)); return out; }
static int cc_sound(IsleCameraCheckSource* c,int id) { return cc_sound_params(c,id,0x7fff,0x40,1.0f,0); }
static void cc_song(IsleCameraCheckSource* c,int player,int song) { cc_checked(c,isle_camera_check_native_song(c,player,song)); }
static void cc_volume(IsleCameraCheckSource* c,int player,int volume) { cc_checked(c,isle_camera_check_native_volume(c,player,volume,0)); }
static void cc_volume_smooth(IsleCameraCheckSource* c,int player,int volume,int frames) { cc_checked(c,isle_camera_check_native_volume(c,player,volume,frames)); }
static void cc_input_initialize(IsleCameraCheckSource* c) {
    cc_checked(c,isle_camera_check_native_input_initialize(c));
    c->common.input_invalidate=isle_post_course_native_input_invalidate;
    cc_checked(c,isle_post_course_source_install_input(&c->common));
}
static void cc_ui_heap(IsleCameraCheckSource* c,size_t bytes) { cc_checked(c,isle_camera_check_native_ui_heap(c,bytes)); }
static void cc_transition(IsleCameraCheckSource* c,int scene) { cc_checked(c,isle_camera_check_native_transition(c,scene)); }
static void cc_notice(IsleCameraCheckSource* c,int notice) { cc_checked(c,isle_camera_check_native_notice(c,notice)); }
static void cc_replay_init(IsleCameraCheckSource* c) { cc_checked(c,isle_camera_check_native_replay_initialize(c)); }
static void cc_replay_cleanup(IsleCameraCheckSource* c) { cc_checked(c,isle_camera_check_native_replay_cleanup(c)); }
static int cc_report_index(IsleCameraCheckSource* c,int id) { int32_t out=0; cc_checked(c,isle_application_photos_report_index(c->common.photos,id,&out)); return out; }
static int cc_report_present(IsleCameraCheckSource* c,int id) { int out=0; cc_checked(c,isle_application_photos_report_present(c->common.photos,id,&out)); return out; }
static int cc_report_count(IsleCameraCheckSource* c) { int32_t out=0; cc_checked(c,isle_application_photos_report_count(c->common.photos,&out)); return out; }
static const PhotoData* cc_report(IsleCameraCheckSource* c,int id) { IslePhotoRecord out; cc_checked(c,isle_application_photos_report(c->common.photos,id,&out)); return out.photo; }
static const PhotoData* cc_album(IsleCameraCheckSource* c,int id) { IslePhotoRecord out; cc_checked(c,isle_application_photos_album(c->common.photos,id,&out)); return out.photo; }
/* Actual roll count is a typed native leaf; selected_count is NOT roll count. */
static int cc_roll_count(IsleCameraCheckSource* c) { int32_t out=0; cc_checked(c,isle_camera_check_native_roll_count(c,&out)); return out; }
static const PhotoData* cc_roll(IsleCameraCheckSource* c,int i) { IslePhotoRecord out; cc_checked(c,isle_application_photos_record(c->common.photos,ISLE_PHOTO_ROLL,i,&out)); return out.photo; }
static void cc_select(IsleCameraCheckSource* c,const PhotoData* photo,int oak,int second,int album) { IslePhotoRecord record; cc_checked(c,isle_application_photos_identify(c->common.photos,photo,&record)); cc_checked(c,isle_application_photos_select(c->common.photos,(IslePlayerPhotoData*)record.photo,oak,second,album)); }
static int cc_photo_order(IsleCameraCheckSource* c,const PhotoData* a,const PhotoData* b) { IslePhotoRecord x,y; cc_checked(c,isle_application_photos_identify(c->common.photos,a,&x)); cc_checked(c,isle_application_photos_identify(c->common.photos,b,&y)); if(x.storage!=ISLE_PHOTO_ROLL || y.storage!=ISLE_PHOTO_ROLL) cc_checked(c,ISLE_GAME_INVALID_STATE); return (x.index>y.index)-(x.index<y.index); }
static int cc_caption(IsleCameraCheckSource* c,const UnkIndigoHalibut* p) {
    IslePhotoRecord record; int populated=0;
    cc_checked(c,isle_application_photos_identify(c->common.photos,p->unk_00,&record));
    if(record.storage!=ISLE_PHOTO_REPORT || !record.photo) cc_checked(c,ISLE_GAME_INVALID_STATE);
    if(c->scored.score.pokemonInFocus!=0) return 1;
    cc_checked(c,isle_camera_check_native_zero_focus_caption(c,record.photo,&populated));
    return populated;
}
static void cc_delete_object(IsleCameraCheckSource* c,GObj* g) { cc_checked(c,isle_post_window_sprite_object_delete(c->common.window,g)); }
static GObj* cc_process_owner(IsleCameraCheckSource* c,IsleGameSourceCallback function,int link,int draw_link,const void* display_list) {
    GObj* out=NULL; IsleGameProcess* process=NULL;
    if(display_list) cc_checked(c,ISLE_GAME_INVALID_ARGUMENT);
    cc_checked(c,isle_camera_check_native_process_owner(c,link,draw_link,&out));
    cc_checked(c,isle_game_add_source_process(c->common.game,out,function,ISLE_GAME_STACKFUL,0,&process));
    return out;
}
static GObj* cc_sync_owner(IsleCameraCheckSource* c,IsleGameSourceCallback function,int link,int draw_link,const void* display_list) {
    GObj* out=NULL; IsleGameProcess* process=NULL;
    if(display_list) cc_checked(c,ISLE_GAME_INVALID_ARGUMENT);
    cc_checked(c,isle_camera_check_native_process_owner(c,link,draw_link,&out));
    cc_checked(c,isle_game_add_source_process(c->common.game,out,function,ISLE_GAME_SYNCHRONOUS,0,&process));
    return out;
}
/* Only the pinned caller's actual helper calls select this wrapper. It keeps
 * the original C helper/return value and every wait/input operation unchanged. */
static int cc_ack_at(IsleCameraCheckSource*,UIElement*,int,int32_t);
/* Full pinned definitions, exact callbacks and initialized byte strings. */
#include "camera_check_source.inc"
static int cc_ack_at(IsleCameraCheckSource* c,UIElement* e,int sound,int32_t line) {
    int result;
    cc_checked(c,isle_post_course_ack_begin(&c->common,e->handle,line));
    result=func_80374F30_8486E0(c,e,sound);
    cc_checked(c,isle_post_course_ack_end(&c->common));
    return result;
}

void isle_camera_check_source_initialize(IsleCameraCheckSource* c) {
    IslePostCourseSceneSource common=c->common;
    memset(c,0,sizeof(*c));
    c->common=common;
    c->common.scene=ISLE_POST_COURSE_CAMERA_CHECK;
    c->D_camera_check_802089E4=-1;
    c->D_camera_check_802089E8=-1;
    c->D_camera_check_802089EC=-1;
    memcpy(c->D_camera_check_8020896C,cc_initial_D_camera_check_8020896C,sizeof(c->D_camera_check_8020896C));
    memcpy(c->D_camera_check_80208994,cc_initial_D_camera_check_80208994,sizeof(c->D_camera_check_80208994));
    memcpy(c->D_camera_check_802089BC,cc_initial_D_camera_check_802089BC,sizeof(c->D_camera_check_802089BC));
    if(c->common.status==ISLE_GAME_OK) {
        c->common.status=isle_camera_check_native_music_state(c,&c->music);
        if(c->common.status==ISLE_GAME_OK && !c->music) c->common.status=ISLE_GAME_INVALID_STATE;
    }
}
void func_camera_check_801DC8D0(GObj* object) {
    IsleCameraCheckSource* c=(IsleCameraCheckSource*)isle_post_course_source_require(object,9);
    func_camera_check_801DC8D0_body(c,object);
    cc_checked(c,c->common.status);
}
void func_camera_check_801DF2D8(GObj* object) {
    IsleCameraCheckSource* c=(IsleCameraCheckSource*)isle_post_course_source_require(object,9);
    func_camera_check_801DF2D8_body(c,object);
    cc_checked(c,c->common.status);
}
void func_camera_check_801DDF28(GObj* object) {
    IsleCameraCheckSource* c=(IsleCameraCheckSource*)isle_post_course_source_require(object,9);
    func_camera_check_801DDF28_body(c,object);
    cc_checked(c,c->common.status);
}
void func_80375224_8489D4(GObj* object) {
    IsleCameraCheckSource* c=(IsleCameraCheckSource*)isle_post_course_source_require(object,9);
    func_80375224_8489D4_body(c,object);
    cc_checked(c,c->common.status);
}

void isle_camera_check_source_entry(void* opaque) {
    IsleCameraCheckSource* c=opaque;
    c->source_result=func_camera_check_801DCACC(c,c->argument);
    c->entry_complete=1;
}

void isle_camera_check_source_init(IslePostCourseSceneSource* common,void* unused) {
    (void)unused;
    cc_checked((IsleCameraCheckSource*)common,common->status);
    func_camera_check_801DCA60((IsleCameraCheckSource*)common);
}
IsleGameResult isle_camera_check_source_release(IsleCameraCheckSource* c) {
    while(c->thumbnail_count) {
        size_t i=c->thumbnail_count-1;
        IsleGameResult r=isle_post_window_thumbnail_release(c->common.window,c->thumbnails[i]);
        if(r!=ISLE_GAME_OK) return r;
        --c->thumbnail_count;
    }
    return ISLE_GAME_OK;
}
