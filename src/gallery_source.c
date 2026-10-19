#include "gallery_source.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
typedef IsleGallerySource Context;
typedef IsleGameObject GObj;
typedef IsleGameSprite SObj;
typedef IsleGameObject OMCamera;
typedef const IslePlayerPhotoData PhotoData;
typedef IsleGalleryInput UnkStruct800BEDF8;
typedef IsleGalleryElement UIElement;
typedef IsleGalleryFrame UIFrame;
typedef IslePostCourseColor ucolor;
typedef const IslePostCourseButton UIButton;
#define ARRAY_COUNT(a) (sizeof(a)/sizeof((a)[0]))
#define UNUSED
#define gc_check(s,r) isle_post_course_source_checked(&(s)->common,(r),__func__,#r,__LINE__)
static void gc_wait(Context* source,int32_t n) { isle_post_course_source_wait(&source->common,n); }
enum { UI_NORMAL=0, UI_HIDDEN=1, UI_BACKGROUND=0, UI_FOREGROUND=1,
FONT_8=8,FONT_12=12,FRAME_STYLE_0=0,FRAME_STYLE_1=1,FRAME_STYLE_2=2,
FRAME_STYLE_3=3,HEADER_PREV=1,HEADER_NEXT=2,HEADER_TITLE=4,
UI_FLAG_BORDER=1,UI_FLAG_THICK_BORDER=2,UI_FLAG_10=0x10,
UI_FLAG_SHOW_ALT=0x40,UI_FLAG_USE_ALT_IMAGE=0x80,UI_FLAG_SCALABLE=0x100,
UI_FLAG_8BIT=0x200,UI_FLAG_32BIT=0x400,UI_FLAG_GRAYSCALE=0x800 };
#include "gallery_declarations.inc"
static SObj* gc_root(Context* source, GObj* object) {
    SObj* sprite = NULL;
    gc_check(source, isle_post_window_sprite_root(source->common.window, object, &sprite));
    return sprite;
}
static IsleSprite* gc_sprite(Context* source, SObj* object) {
    IsleSprite* sprite = NULL;
    gc_check(source, isle_post_window_sprite_view(source->common.window, object, &sprite));
    return sprite;
}
static GObj* gc_process(Context* source, IsleGameSourceCallback callback,
                       int32_t link, int32_t dllink, void* dlist) {
    GObj* object = NULL;
    IsleGameProcess* process = NULL;
    gc_check(source, source->native->process_object(source, link, dllink, dlist, &object));
    gc_check(source, isle_game_add_source_process(source->common.game, object, callback,
        ISLE_GAME_STACKFUL, 0, &process));
    return object;
}
static GObj* gc_sprite_object(Context* source, IsleGameSourceCallback callback,
                             int32_t link, uint32_t prototype, int32_t kind) {
    GObj* object = NULL;
    SObj* sprite = NULL;
    gc_check(source, isle_post_window_sprite_object(source->common.window, link,
        prototype, callback, kind, &object, &sprite));
    return object;
}
static GObj* gc_sprite_stackful(Context* source, IsleGameSourceCallback callback,
                               int32_t link, uint32_t prototype) {
    return gc_sprite_object(source, callback, link, prototype, 0);
}
static GObj* gc_sprite_synchronous(Context* source, IsleGameSourceCallback callback,
                                  int32_t link, uint32_t prototype) {
    return gc_sprite_object(source, callback, link, prototype, 1);
}
static SObj* gc_sibling(Context* source, SObj* parent, uint32_t prototype) {
    SObj* output = NULL;
    gc_check(source, isle_post_window_sprite_sibling(source->common.window, parent, prototype, &output));
    return output;
}
static SObj* gc_child(Context* source, SObj* parent, uint32_t prototype) {
    SObj* output = NULL;
    gc_check(source, isle_post_window_sprite_child(source->common.window, parent, prototype, &output));
    return output;
}
/* Every UI pointer is only a stable typed handle cell. Deleted cells are never
 * reused: the canonical Window checks its actual incarnation on every call. */
static UIElement* gc_element_create(Context* source, int x, int y, int w, int h, uint32_t flags) {
    UIElement* output;
    if (source->element_count == ARRAY_COUNT(source->elements)) gc_check(source, ISLE_GAME_EXHAUSTED);
    output = &source->elements[source->element_count++];
    gc_check(source, isle_post_window_element_create(source->common.window, x, y, w, h, flags, &output->handle));
    return output;
}
static UIFrame* gc_frame_create(Context* source) {
    gc_check(source, isle_post_window_frame_create(source->common.window, &source->frame.handle));
    return &source->frame;
}
static void gc_element_delete(Context* source, UIElement* element) {
    gc_check(source, isle_post_window_element_delete(source->common.window, element->handle));
}
static void gc_element_draw(Context* source, UIElement* element) {
    gc_check(source, isle_post_window_element_clear(source->common.window, element->handle));
}
static void gc_element_background(Context* source, UIElement* element) {
    gc_check(source, isle_post_window_element_background(source->common.window, element->handle));
}
static void gc_element_state(Context* source, UIElement* element, int state) {
    if (element == NULL) return; /* Original UIElement_SetState accepts NULL. */
    gc_check(source, isle_post_window_element_visible(source->common.window, element->handle, state == UI_NORMAL));
}
static void gc_element_font(Context* source, UIElement* element, int font) {
    gc_check(source, isle_post_window_element_font(source->common.window, element->handle, font));
}
static void gc_element_position(Context* source, UIElement* element, int x, int y) {
    gc_check(source, isle_post_window_element_text_position(source->common.window, element->handle, x, y));
}
static void gc_element_color(Context* source, UIElement* element, int foreground, int r, int g, int b, int a) {
    gc_check(source, isle_post_window_element_color(source->common.window, element->handle,
        foreground == UI_FOREGROUND, (IslePostCourseColor){(uint8_t)r,(uint8_t)g,(uint8_t)b,(uint8_t)a}));
}
static int32_t gc_print(Context* source, UIElement* element, const char* text, int ascii) {
    IslePostCourseBytes bytes = {text, text ? strlen(text) : 0};
    gc_check(source, isle_post_window_print(source->common.window, element->handle, bytes, ascii, NULL));
    return 0;
}
static void gc_print_source(Context* source, UIElement* element, const char* text) {
    (void)gc_print(source, element, text, 0);
}
static int32_t gc_formatted(Context* source, UIElement* element, const char* format, ...) {
    int count;
    va_list args;
    va_start(args, format);
    count = vsnprintf(source->formatted, sizeof(source->formatted), format, args);
    va_end(args);
    if (count < 0 || (size_t)count >= sizeof(source->formatted)) gc_check(source, ISLE_GAME_INVALID_STATE);
    gc_print_source(source, element, source->formatted);
    return count;
}
/* 8486E0.c44-62: the original s16[]/char* alias becomes explicit big-endian
 * source bytes; it never serializes host-endian uint16_t objects. */
static char* gc_number(Context* source, const char* format, int32_t value) {
    int count = snprintf(source->number_ascii, sizeof(source->number_ascii), format, value);
    int i;
    if (count < 0 || (size_t)count >= sizeof(source->number_ascii)) gc_check(source, ISLE_GAME_INVALID_STATE);
    for (i = 0; i < count; ++i) {
        unsigned char ch = (unsigned char)source->number_ascii[i];
        source->number_source[i*2] = (char)(ch >= '0' && ch <= '9' ? 0xa3 : 0xa1);
        source->number_source[i*2+1] = (char)(ch >= '0' && ch <= '9' ? ch-'0'+0xb0 : 0xa1);
    }
    source->number_source[count*2] = 0;
    return source->number_source;
}
static int32_t gc_width(Context* source, const char* text) {
    int output = 0;
    gc_check(source, isle_post_window_text_width(source->common.window,
        (IslePostCourseBytes){text, text ? strlen(text) : 0}, &output));
    return output;
}
static void gc_frame_color(Context* source, UIFrame* frame, ucolor* output) {
    gc_check(source, isle_post_window_frame_color(source->common.window, frame->handle, output));
}
static void gc_text_initialize(Context* source) { gc_check(source,isle_post_window_text_initialize(source->common.window)); }
static void gc_focus_create(Context* source) { gc_check(source,isle_post_window_focus_create(source->common.window)); }
static void gc_focus_front(Context* source) { gc_check(source,isle_post_window_focus_front(source->common.window)); }
static void gc_layout_initialize(Context* source) { gc_check(source,isle_post_window_layout_initialize(source->common.window)); }
static void gc_layout_hide(Context* source) { gc_check(source,isle_post_window_layout_hide(source->common.window)); }
static void gc_layout_wait(Context* source) { gc_check(source,isle_post_window_layout_wait(source->common.window)); }
static void gc_text_delay(Context* source,int delay) { gc_check(source,isle_post_window_text_delay(source->common.window,delay)); }
static void gc_text_shadow(Context* source,int offset) { gc_check(source,isle_post_window_text_shadow_offset(source->common.window,offset)); }
static void gc_text_spacing(Context* source,int x,int y) { gc_check(source,isle_post_window_text_spacing(source->common.window,x,y)); }
static void gc_text_font(Context* source,int font) { gc_check(source,isle_post_window_text_font(source->common.window,font)); }
static void gc_text_skip(Context* source,int enabled) { gc_check(source,isle_post_window_text_skip(source->common.window,enabled)); }
static void gc_focus_show(Context* source,int enabled) { gc_check(source,isle_post_window_focus_visible(source->common.window,enabled)); }
static void gc_focus_position(Context* source,int x,int y) { gc_check(source,isle_post_window_focus_position(source->common.window,x,y)); }
static void gc_focus_target(Context* source,int x,int y) { gc_check(source,isle_post_window_focus_target_position(source->common.window,x,y)); }
static void gc_focus_target_size(Context* source,int x,int y) { gc_check(source,isle_post_window_focus_target_size(source->common.window,x,y)); }
static void gc_focus_stop(Context* source,int enabled) { gc_check(source,isle_post_window_focus_stop(source->common.window,enabled)); }
static void gc_layout_header(Context* source,int flags) { gc_check(source,isle_post_window_layout_header_flags(source->common.window,flags)); }
static void gc_layout_hide_header(Context* source,int flags) { gc_check(source,isle_post_window_layout_hide_header(source->common.window,flags)); }
static void gc_layout_show_header(Context* source,int flags) { gc_check(source,isle_post_window_layout_show_header(source->common.window,flags)); }
static void gc_layout_panel(Context* source,int enabled) { gc_check(source,isle_post_window_layout_panel(source->common.window,enabled)); }
static void gc_layout_disable(Context* source,int disabled,int id) { gc_check(source,isle_post_window_layout_disable(source->common.window,disabled,id)); }
static void gc_frame_style(Context* source,UIFrame* frame,int style) { gc_check(source,isle_post_window_frame_style(source->common.window,frame->handle,style)); }
static void gc_frame_position(Context* source,UIFrame* frame,int x,int y) { gc_check(source,isle_post_window_frame_position(source->common.window,frame->handle,x,y)); }
static void gc_frame_show(Context* source,UIFrame* frame,int enabled) { gc_check(source,isle_post_window_frame_visible(source->common.window,frame->handle,enabled)); }
static void gc_frame_background(Context* source,UIFrame* frame,int enabled) { gc_check(source,isle_post_window_frame_background(source->common.window,frame->handle,enabled)); }
static void gc_frame_fade_in(Context* source,UIFrame* frame,int style) { gc_check(source,isle_post_window_fade_in(source->common.window,frame->handle,style)); }
static void gc_frame_fade_out(Context* source,UIFrame* frame) { gc_check(source,isle_post_window_fade_out(source->common.window,frame->handle)); }
static void gc_repeat(Context* source,int a,int b) { gc_check(source,source->native->input_repeat(source,a,b)); }
static void gc_input_mask(Context* source,uint32_t mask) { gc_check(source,source->native->input_mask(source,mask)); }
static void gc_aux_camera(Context* source,int a,int b) { gc_check(source,source->native->aux_camera(source,a,b)); }
static void gc_sound(Context* source,int id) { gc_check(source,source->native->sound(source,id)); }
static void gc_sound_params(Context* source,int id,int volume,int pan,float pitch,int reverb) { gc_check(source,source->native->sound_params(source,id,volume,pan,pitch,reverb)); }
static void gc_song(Context* source,int channel,int id) { gc_check(source,source->native->song(source,channel,id)); }
static void gc_volume(Context* source,int channel,int volume) { gc_check(source,source->native->volume(source,channel,volume)); }
static void gc_volume_smooth(Context* source,int channel,int volume,int frames) { gc_check(source,source->native->volume_smooth(source,channel,volume,frames)); }
static void gc_scene_result(Context* source,int value) { gc_check(source,source->native->scene_result(source,value)); }
static void gc_fade(Context* source,int dir,float duration) { gc_check(source,source->native->fade(source,dir,duration)); }
static void gc_graphics_join(Context* source) {
    int32_t pending;
    do {
        gc_check(source,source->native->graphics_join(source,&pending));
        if(pending) gc_check(source,isle_post_course_native_progress(&source->common));
    } while(pending);
}
static void gc_replay_initialize(Context* source) { gc_check(source,source->native->replay_initialize(source)); }
static void gc_replay_cleanup(Context* source) {
    int32_t pending;
    do {
        gc_check(source,source->native->replay_cleanup(source,&pending));
        if(pending) gc_check(source,isle_post_course_native_progress(&source->common));
    } while(pending);
}
static void gc_printer_command(Context* source,int cmd) { gc_check(source,source->native->printer_command(source,cmd)); }
static void gc_deallocate(Context* source,void* p) { gc_check(source,source->native->deallocate(source,p)); }
static int32_t gc_printer_available(Context* source) { int32_t out; gc_check(source,source->native->printer_available(source,&out)); return out; }
static int32_t gc_printer_status(Context* source) { int32_t out; gc_check(source,source->native->printer_status(source,&out)); return out; }
static int32_t gc_fade_poll(Context* source) { int32_t out; gc_check(source,source->native->fade_poll(source,&out)); return out; }
static int32_t gc_save(Context* source) { int32_t out; gc_check(source,source->native->save(source,&out)); return out; }
static int32_t gc_report_count(Context* source) { int32_t out; gc_check(source,isle_application_photos_report_count(source->common.photos,&out)); return out; }
static UnkStruct800BEDF8* gc_input(Context* source,int32_t player) {
    gc_check(source,source->native->input(source,player,&source->input)); return &source->input;
}
static void gc_input_initialize(Context* source) {
    gc_check(source,source->native->input_initialize(source));
    source->common.input_invalidate=isle_post_course_native_input_invalidate;
    gc_check(source,isle_post_course_source_install_input(&source->common));
}
static const char* gc_pokemon_name(Context* source,int32_t species) {
    const char* out; gc_check(source,source->native->pokemon_name(source,species,&out)); return out;
}
static int gc_report_present(Context* source,int32_t species) {
    int out; gc_check(source,isle_application_photos_report_present(source->common.photos,species,&out)); return out;
}
static int32_t gc_report_score(Context* source,int32_t species) {
    int32_t out; gc_check(source,isle_application_photos_report_score(source->common.photos,species,&out)); return out;
}
static const PhotoData* gc_report(Context* source,int32_t species) {
    IslePhotoRecord out; gc_check(source,isle_application_photos_report(source->common.photos,species,&out)); return out.photo;
}
static const PhotoData* gc_album(Context* source,int32_t index) {
    IslePhotoRecord out; gc_check(source,isle_application_photos_album(source->common.photos,index,&out)); return out.photo;
}
static const PhotoData* gc_temporary(Context* source,int32_t index) {
    IslePhotoRecord out; gc_check(source,isle_application_photos_temporary(source->common.photos,index,&out)); return out.photo;
}
static void gc_snapshot(Context* source,int32_t index,const PhotoData* photo) {
    IslePhotoRecord record;
    gc_check(source,isle_application_photos_identify(source->common.photos,photo,&record));
    gc_check(source,isle_photo_working_copy_record(source->working[index],&record));
}
static void gc_write_snapshot(Context* source,int32_t index) {
    gc_check(source,isle_application_photos_set_temporary_working(source->common.photos,index,source->working[index]));
}
static void gc_camera(Context* source,int mode) {
    GObj* object; IsleGameProcess* process;
    gc_check(source,source->native->camera(source,mode,&object));
    if (!mode) {
        source->D_801EA7F4_A084B4=source->D_801EA7F8_A084B8=object;
        gc_check(source,isle_game_add_source_process(source->common.game,object,isle_gallery_func_801DCA58_9FA718,ISLE_GAME_STACKFUL,0,&process));
    } else source->D_801EA7FC_A084BC=source->D_801EA800_A084C0=object;
}
static void gc_hidden(Context* source,GObj* object,int value) {
    int32_t pending;
    gc_check(source,source->native->object_hidden(source,object,value));
    if(value && object==source->D_801EA7FC_A084BC) {
        /* Actual secondary-camera hide: stop all real prior display borrows
         * before releasing its Photo/World back to the same thumbnail Replay.
         * This SDK completion driver creates no OM/GTL/ohWait update. */
        gc_graphics_join(source);
        do {
            gc_check(source,source->native->presentation_release(source,&pending));
            if(pending) gc_check(source,isle_post_course_native_progress(&source->common));
        } while(pending);
    }
}
static GObj* gc_first(Context* source,int link) { GObj* out; gc_check(source,source->native->object_first(source,link,&out)); return out; }
static GObj* gc_next(Context* source,GObj* object) { GObj* out; gc_check(source,source->native->object_next(source,object,&out)); return out; }
static int32_t gc_id(Context* source,GObj* object) { int32_t out; gc_check(source,source->native->object_id(source,object,&out)); return out; }
static void gc_pause(Context* source,GObj* object) { gc_check(source,source->native->object_pause(source,object)); }
static void gc_present(Context* source,GObj* camera,const PhotoData* photo) {
    int32_t pending;
    gc_check(source,source->native->presentation_begin(source,camera,photo));
    do {
        gc_check(source,source->native->presentation_poll(source,&pending));
        if (pending) gc_check(source,isle_post_course_native_progress(&source->common));
    } while(pending);
}
static int gc_slot_id(Context* source,size_t index) { int out; gc_check(source,isle_post_window_layout_slot_id(source->common.window,index,&out)); return out; }
static IslePostCourseWindow* gc_selector(Context* source) { return source->common.window; }
static void gc_layout_create(Context* source,const UIButton* buttons) {
    IslePostCourseButtonTable table;
    size_t count;
    if(buttons==source->initial->D_801EA210_A07ED0) count=source->initial->absent_button_count;
    else if(buttons==source->initial->D_801EA248_A07F08) count=source->initial->present_button_count;
    else { gc_check(source,ISLE_GAME_INVALID_ARGUMENT); return; }
    gc_check(source,isle_post_window_table_borrow(source->common.window,buttons,count,&table));
    gc_check(source,isle_post_window_layout_create(source->common.window,table));
}
static IslePostCourseThumbnail* gc_thumbnail_handle(Context* source,SObj* child) {
    size_t i;
    for(i=0;i<6;++i) if(source->D_80230818_A4E4D8[i].sobj==child) return &source->D_80230818_A4E4D8[i].thumbnail;
    for(i=0;i<4;++i) if(source->D_80230848_A4E508[i].sobj==child) return &source->D_80230848_A4E508[i].thumbnail;
    if(source->D_80230874_A4E534==child) return &source->dragged_thumbnail;
    gc_check(source,ISLE_GAME_INVALID_ARGUMENT); return NULL;
}
static void gc_thumbnail(Context* source,SObj* child) {
    gc_check(source,isle_post_window_thumbnail_create(source->common.window,child,UINT32_C(0x801ea1b0),gc_thumbnail_handle(source,child)));
}
typedef struct GalleryPreviewConsumer { Context* source; IslePostCourseThumbnail thumbnail; } GalleryPreviewConsumer;
static IsleGameResult gc_consume_preview(void* user,const IslePostCoursePreview* view) {
    GalleryPreviewConsumer* c=(GalleryPreviewConsumer*)user;
    if(!view) return isle_post_window_thumbnail_clear(c->source->common.window,c->thumbnail);
    return isle_post_window_thumbnail_write_photo(c->source->common.window,c->thumbnail,view->rgba5551,view->size,view->stride_bytes);
}
static int32_t gc_preview(Context* source,const PhotoData* photo,SObj* child) {
    float scale; IsleSprite* sprite=gc_sprite(source,child);
    GalleryPreviewConsumer c={source,*gc_thumbnail_handle(source,child)};
    gc_check(source,source->native->preview_scale(source,&scale));
    gc_check(source,isle_post_replay_preview(source->common.replay,photo,sprite->width*2,sprite->height*2,scale,gc_consume_preview,&c));
    return photo==NULL;
}
static void gc_rebind(Context* source,SObj* sprite,uint32_t prototype) {
    gc_check(source,isle_post_window_sprite_rebind(source->common.window,sprite,prototype));
}
static void gc_clear_matrices(Context* source,SObj* sprite) { gc_check(source,source->native->sprite_clear_matrices(source,sprite)); }

#include "gallery_original.inc"
IsleGameResult isle_gallery_source_initialize(Context* source,const IsleGalleryNative* native,const IsleGalleryInitialData* initial,IslePhotoWorking* const working[4],int32_t argument) {
    size_t i,j; IslePostCourseSceneSource common; IsleGameResult result;
    if(!source || !native || !initial || !initial->canonical_receipt || !working || (argument!=22 && argument!=23)) return ISLE_GAME_INVALID_ARGUMENT;
    if(!native->initial_validate) return ISLE_GAME_INVALID_ARGUMENT;
    if(!native->input) return ISLE_GAME_INVALID_ARGUMENT;
    if(!native->input_initialize) return ISLE_GAME_INVALID_ARGUMENT;
    if(!native->input_repeat) return ISLE_GAME_INVALID_ARGUMENT;
    if(!native->input_mask) return ISLE_GAME_INVALID_ARGUMENT;
    if(!native->camera) return ISLE_GAME_INVALID_ARGUMENT;
    if(!native->aux_camera) return ISLE_GAME_INVALID_ARGUMENT;
    if(!native->process_object) return ISLE_GAME_INVALID_ARGUMENT;
    if(!native->display_list) return ISLE_GAME_INVALID_ARGUMENT;
    if(!native->render_state) return ISLE_GAME_INVALID_ARGUMENT;
    if(!native->screen) return ISLE_GAME_INVALID_ARGUMENT;
    if(!native->ui_heap) return ISLE_GAME_INVALID_ARGUMENT;
    if(!native->deallocate) return ISLE_GAME_INVALID_ARGUMENT;
    if(!native->object_hidden) return ISLE_GAME_INVALID_ARGUMENT;
    if(!native->object_first) return ISLE_GAME_INVALID_ARGUMENT;
    if(!native->object_next) return ISLE_GAME_INVALID_ARGUMENT;
    if(!native->object_id) return ISLE_GAME_INVALID_ARGUMENT;
    if(!native->object_pause) return ISLE_GAME_INVALID_ARGUMENT;
    if(!native->graphics_join) return ISLE_GAME_INVALID_ARGUMENT;
    if(!native->presentation_begin) return ISLE_GAME_INVALID_ARGUMENT;
    if(!native->presentation_poll) return ISLE_GAME_INVALID_ARGUMENT;
    if(!native->presentation_release) return ISLE_GAME_INVALID_ARGUMENT;
    if(!native->fade) return ISLE_GAME_INVALID_ARGUMENT;
    if(!native->fade_poll) return ISLE_GAME_INVALID_ARGUMENT;
    if(!native->sprite_clear_matrices) return ISLE_GAME_INVALID_ARGUMENT;
    if(!native->preview_scale) return ISLE_GAME_INVALID_ARGUMENT;
    if(!native->replay_initialize) return ISLE_GAME_INVALID_ARGUMENT;
    if(!native->replay_cleanup) return ISLE_GAME_INVALID_ARGUMENT;
    if(!native->printer_available) return ISLE_GAME_INVALID_ARGUMENT;
    if(!native->printer_status) return ISLE_GAME_INVALID_ARGUMENT;
    if(!native->printer_command) return ISLE_GAME_INVALID_ARGUMENT;
    if(!native->sound) return ISLE_GAME_INVALID_ARGUMENT;
    if(!native->sound_params) return ISLE_GAME_INVALID_ARGUMENT;
    if(!native->song) return ISLE_GAME_INVALID_ARGUMENT;
    if(!native->volume) return ISLE_GAME_INVALID_ARGUMENT;
    if(!native->volume_smooth) return ISLE_GAME_INVALID_ARGUMENT;
    if(!native->scene_result) return ISLE_GAME_INVALID_ARGUMENT;
    if(!native->pokemon_name) return ISLE_GAME_INVALID_ARGUMENT;
    if(!native->save) return ISLE_GAME_INVALID_ARGUMENT;
    if(!native->entry_begin) return ISLE_GAME_INVALID_ARGUMENT;
    if(!native->entry_poll) return ISLE_GAME_INVALID_ARGUMENT;
    if(!initial->D_801EA210_A07ED0 || !initial->D_801EA248_A07F08 || !initial->absent_button_count || !initial->present_button_count) return ISLE_GAME_INVALID_ARGUMENT;
    for(i=0;i<4;++i) { if(!working[i]) return ISLE_GAME_INVALID_ARGUMENT; for(j=0;j<i;++j) if(working[i]==working[j]) return ISLE_GAME_INVALID_ARGUMENT; }
    /* Exact BE scalar/table facts: qualified-640a-menu-initialized-Snapshot,
     * gallery_entry_initialized and gallery_tables_and_flags. These are
     * admission comparisons, never defaults synthesized by this provider. */
    if(initial->D_801E16BC_9FF37C!=0 ||
       initial->D_801EA1FC_A07EBC!=0 || initial->D_801EA1F4_A07EB4!=1 ||
       initial->D_801EA1F8_A07EB8!=0 || initial->D_801EA200_A07EC0!=-1 ||
       initial->D_801EA204_A07EC4!=0 || initial->D_801EA288_A07F48!=0 ||
       initial->D_801EA28C_A07F4C!=0 || initial->D_801EA290_A07F50!=0 ||
       initial->D_801EA294_A07F54!=0 || initial->D_801EA298_A07F58!=-1 ||
       initial->D_801EA29C_A07F5C!=-1 || initial->D_801EA2A0_A07F60!=-1 ||
       initial->D_801EA2A4_A07F64!=-1 || initial->D_801EA2A8_A07F68!=-1 ||
       initial->D_801EA2AC_A07F6C!=-1 || initial->D_801EA2B0_A07F70!=-1 ||
       initial->absent_button_count!=7 || initial->present_button_count!=8)
        return ISLE_GAME_INVALID_ARGUMENT;
    {
        static const int absent_ids[7]={2,3,26,20,34,4,35};
        static const int present_ids[8]={2,3,26,20,34,32,4,35};
        for(i=0;i<7;++i) if(initial->D_801EA210_A07ED0[i].id!=absent_ids[i] ||
            initial->D_801EA210_A07ED0[i].text.data || initial->D_801EA210_A07ED0[i].text.size)
            return ISLE_GAME_INVALID_ARGUMENT;
        for(i=0;i<8;++i) if(initial->D_801EA248_A07F08[i].id!=present_ids[i] ||
            initial->D_801EA248_A07F08[i].text.data || initial->D_801EA248_A07F08[i].text.size)
            return ISLE_GAME_INVALID_ARGUMENT;
    }
    common=source->common;
    /* Cold reset only on actual overlay-generation load, never cache/page miss. */
    memset(source,0,sizeof(*source)); source->common=common;
    source->native=native; source->initial=initial; source->common.scene=argument;
    result=native->initial_validate(source,initial); if(result!=ISLE_GAME_OK) return result;
    source->D_801E16BC_9FF37C=initial->D_801E16BC_9FF37C;
    source->D_801EA1FC_A07EBC=initial->D_801EA1FC_A07EBC;
    source->D_801EA1F4_A07EB4=initial->D_801EA1F4_A07EB4;
    source->D_801EA1F8_A07EB8=initial->D_801EA1F8_A07EB8;
    source->D_801EA200_A07EC0=initial->D_801EA200_A07EC0;
    source->D_801EA204_A07EC4=initial->D_801EA204_A07EC4;
    source->D_801EA288_A07F48=initial->D_801EA288_A07F48;
    source->D_801EA28C_A07F4C=initial->D_801EA28C_A07F4C;
    source->D_801EA290_A07F50=initial->D_801EA290_A07F50;
    source->D_801EA294_A07F54=initial->D_801EA294_A07F54;
    source->D_801EA298_A07F58=initial->D_801EA298_A07F58;
    source->D_801EA29C_A07F5C=initial->D_801EA29C_A07F5C;
    source->D_801EA2A0_A07F60=initial->D_801EA2A0_A07F60;
    source->D_801EA2A4_A07F64=initial->D_801EA2A4_A07F64;
    source->D_801EA2A8_A07F68=initial->D_801EA2A8_A07F68;
    source->D_801EA2AC_A07F6C=initial->D_801EA2AC_A07F6C;
    source->D_801EA2B0_A07F70=initial->D_801EA2B0_A07F70;

    for(i=0;i<4;++i) source->working[i]=working[i];
    source->D_801EA7F0_A084B0=(argument==22);
    return ISLE_GAME_OK;
}
void isle_gallery_source_init_call(IslePostCourseSceneSource* common,void* user) {
    (void)user; isle_gallery_func_801DCDDC_9FAA9C((Context*)common);
}
void isle_gallery_source_entry(void* user) {
    IsleGalleryEntry* entry=(IsleGalleryEntry*)user;
    Context* source=entry->source;
    IsleContinuationDomain* domain=source->common.domain;
    void* owner=source->common.owner;
    const IsleGalleryNative* native=source->native;
    int retired=0; int32_t result; IsleGameResult status;
    entry->completed=0;
    source->D_801EA7F0_A084B0=(entry->argument==22);
    status=native->entry_begin(source,entry->argument);
    if(status!=ISLE_GAME_OK) { entry->status=status; (void)isle_continuation_fail(domain,"Gallery entry setup"); return; }
    /* Source storage may now retire; use only independent entry/owner/domain. */
    while(!retired) {
        if(isle_continuation_wait(domain,1)!=ISLE_CONT_OK) { entry->status=ISLE_GAME_INVALID_STATE; (void)isle_continuation_fail(domain,"Gallery entry continuation"); return; }
        status=native->entry_poll(owner,&retired,&result);
        if(status!=ISLE_GAME_OK) { entry->status=status; (void)isle_continuation_fail(domain,"Gallery entry retirement"); return; }
    }
    entry->result=result; entry->status=ISLE_GAME_OK; entry->completed=1;
}
void isle_gallery_func_801DCEBC_9FAB7C(void* user) { isle_gallery_source_entry(user); }
