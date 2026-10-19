#include "album_source.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
typedef IsleAlbumSource Context;
typedef IsleGameObject GObj;
typedef IsleGameSprite SObj;
typedef IslePlayerPhotoData PhotoData;
typedef IsleAlbumElement UIElement;
typedef IsleAlbumFrame UIFrame;
typedef IsleAlbumPhoto AlbumPhoto;
typedef IsleAlbumComment AlbumComment;
typedef IslePostCourseButton UIButton;
typedef IslePostCourseScoredPhoto Unk803A6C18;
typedef IsleAlbumInput UnkStruct800BEDF8;
#define ARRAY_COUNT(a) (sizeof(a)/sizeof((a)[0]))
#include "album_declarations.inc"
#include "album_data.inc"
#define ab_check(source,result) isle_post_course_source_checked(&(source)->common,(result),__func__,#result,__LINE__)
static void ab_wait(Context* source,int32_t count) { isle_post_course_source_wait(&source->common,count); }
static SObj* ab_root(Context* source, GObj* object) {
    SObj* sprite = NULL;
    ab_check(source, isle_post_window_sprite_root(source->common.window, object, &sprite));
    return sprite;
}
static IsleSprite* ab_sprite(Context* source, SObj* object) {
    IsleSprite* sprite = NULL;
    ab_check(source, isle_post_window_sprite_view(source->common.window, object, &sprite));
    return sprite;
}
static void ab_delete(Context* source, GObj* object) {
    ab_check(source, isle_game_delete_object(source->common.game, object));
}
static GObj* ab_sprite_object(Context* source, IsleGameSourceCallback callback,
                             int32_t link, uint32_t prototype, int32_t kind) {
    GObj* object = NULL;
    SObj* sprite = NULL;
    ab_check(source, isle_post_window_sprite_object(source->common.window, link,
        prototype, callback, kind, &object, &sprite));
    return object;
}
static GObj* ab_sprite_stackful(Context* source, IsleGameSourceCallback callback,
                               int32_t link, uint32_t prototype) {
    return ab_sprite_object(source, callback, link, prototype, 0);
}
static GObj* ab_sprite_synchronous(Context* source, IsleGameSourceCallback callback,
                                  int32_t link, uint32_t prototype) {
    return ab_sprite_object(source, callback, link, prototype, 1);
}
static SObj* ab_sibling(Context* source, SObj* parent, uint32_t prototype) {
    SObj* output = NULL;
    ab_check(source, isle_post_window_sprite_sibling(source->common.window, parent, prototype, &output));
    return output;
}
static SObj* ab_child(Context* source, SObj* parent, uint32_t prototype) {
    SObj* output = NULL;
    ab_check(source, isle_post_window_sprite_child(source->common.window, parent, prototype, &output));
    return output;
}
static UIElement* ab_element_create(Context* source, int x, int y, int w, int h, uint32_t flags) {
    UIElement* output=NULL;
    size_t i;
    for(i=0;i<ARRAY_COUNT(source->elements);i++)
        if(!source->elements[i].handle.serial) { output=&source->elements[i];break; }
    if(!output) ab_check(source,ISLE_GAME_EXHAUSTED);
    ab_check(source, isle_post_window_element_create(source->common.window, x, y, w, h, flags, &output->handle));
    return output;
}
static UIFrame* ab_frame_create(Context* source) {
    if(source->frame.handle.serial) {
        ab_check(source, isle_post_window_frame_retire_hidden(source->common.window, source->frame.handle));
        memset(&source->frame.handle,0,sizeof(source->frame.handle));
    }
    ab_check(source, isle_post_window_frame_create(source->common.window, &source->frame.handle));
    return &source->frame;
}
static void ab_element_delete(Context* source, UIElement* element) {
    ab_check(source, isle_post_window_element_delete(source->common.window, element->handle));
    memset(&element->handle,0,sizeof(element->handle));
}
static void ab_element_draw(Context* source, UIElement* element) {
    ab_check(source, isle_post_window_element_clear(source->common.window, element->handle));
}
static void ab_element_background(Context* source, UIElement* element) {
    ab_check(source, isle_post_window_element_background(source->common.window, element->handle));
}
static void ab_element_state(Context* source, UIElement* element, int state) {
    if (element == NULL) return; /* Original UIElement_SetState accepts NULL. */
    ab_check(source, isle_post_window_element_visible(source->common.window, element->handle, state == UI_NORMAL));
}
static void ab_element_font(Context* source, UIElement* element, int font) {
    ab_check(source, isle_post_window_element_font(source->common.window, element->handle, font));
}
static void ab_element_position(Context* source, UIElement* element, int x, int y) {
    ab_check(source, isle_post_window_element_text_position(source->common.window, element->handle, x, y));
}
static void ab_element_color(Context* source, UIElement* element, int foreground, int r, int g, int b, int a) {
    ab_check(source, isle_post_window_element_color(source->common.window, element->handle,
        foreground == UI_FOREGROUND, (IslePostCourseColor){(uint8_t)r,(uint8_t)g,(uint8_t)b,(uint8_t)a}));
}
static int32_t ab_print(Context* source, UIElement* element, const char* text, int ascii) {
    IslePostCourseBytes bytes = {text, text ? strlen(text) : 0};
    ab_check(source, isle_post_window_print(source->common.window, element->handle, bytes, ascii, NULL));
    return 0;
}
static void ab_print_source(Context* source, UIElement* element, const char* text) {
    (void)ab_print(source, element, text, 0);
}
static int32_t ab_formatted(Context* source, UIElement* element, const char* format, ...) {
    int count;
    va_list args;
    va_start(args, format);
    count = vsnprintf(source->formatted, sizeof(source->formatted), format, args);
    va_end(args);
    if (count < 0 || (size_t)count >= sizeof(source->formatted)) ab_check(source, ISLE_GAME_INVALID_STATE);
    ab_print_source(source, element, source->formatted);
    return count;
}
/* 8486E0.c44-62: the original s16[]/char* alias becomes explicit big-endian
 * source bytes; it never serializes host-endian uint16_t objects. */
static char* ab_number(Context* source, const char* format, int32_t value) {
    int32_t count;
    int i;
    if(!strcmp(format,"%5.2f"))
        ab_check(source,source->native->float_format_integer(source,value,source->number_ascii,sizeof(source->number_ascii),&count));
    else
        count=snprintf(source->number_ascii,sizeof(source->number_ascii),format,value);
    if (count < 0 || (size_t)count >= sizeof(source->number_ascii)) ab_check(source, ISLE_GAME_INVALID_STATE);
    for (i = 0; i < count; ++i) {
        unsigned char ch = (unsigned char)source->number_ascii[i];
        source->number_source[i*2] = (char)(ch >= '0' && ch <= '9' ? 0xa3 : 0xa1);
        source->number_source[i*2+1] = (char)(ch >= '0' && ch <= '9' ? ch-'0'+0xb0 : 0xa1);
    }
    source->number_source[count*2] = 0;
    return source->number_source;
}
static int32_t ab_width(Context* source, const char* text) {
    int output = 0;
    ab_check(source, isle_post_window_text_width(source->common.window,
        (IslePostCourseBytes){text, text ? strlen(text) : 0}, &output));
    return output;
}
static int32_t ab_letter_width(Context* source,const char* text,int initialized) {
    int32_t width;
    if(initialized) return ab_width(source,text);
    ab_check(source,source->native->inactive_letter_width(source,&width));
    return width;
}
static void ab_element_absolute(Context* source,UIElement* element,int x,int y) {
    ab_check(source,isle_post_window_element_position(source->common.window,element->handle,x,y));
}
static void ab_element_tint(Context* source,UIElement* element,int r,int g,int b,int a) {
    ab_check(source,isle_post_window_element_sprite_color(source->common.window,element->handle,(IslePostCourseColor){r,g,b,a}));
}
static GObj* ab_process(Context* source,IsleGameSourceCallback callback,int32_t link,int32_t dllink,const void* dlist) {
    GObj* object=NULL; IsleGameProcess* process=NULL;
    ab_check(source,source->native->process_object(source,link,dllink,dlist,&object));
    ab_check(source,isle_game_add_source_process(source->common.game,object,callback,ISLE_GAME_STACKFUL,0,&process));
    return object;
}
static void ab_camera_create(Context* source) {
    GObj* object=NULL; IsleGameProcess* process=NULL;
    ab_check(source,source->native->camera(source,&object));
    ab_check(source,isle_game_add_source_process(source->common.game,object,isle_album_func_801DCA14_9A6C64,ISLE_GAME_STACKFUL,0,&process));
}
static void ab_thumbnail(Context* source,SObj* child,uint32_t prototype) {
    size_t n=source->thumbnail_count;
    if(n==ARRAY_COUNT(source->thumbnails)) ab_check(source,ISLE_GAME_EXHAUSTED);
    ab_check(source,isle_post_window_thumbnail_create(source->common.window,child,prototype,&source->thumbnails[n]));
    source->thumbnail_children[n]=child; source->thumbnail_count=n+1;
}
typedef struct AlbumPreview { Context* source; IslePostCourseThumbnail target; } AlbumPreview;
static IsleGameResult ab_consume_preview(void* user,const IslePostCoursePreview* view) {
    AlbumPreview* p=(AlbumPreview*)user;
    if(!view) return isle_post_window_thumbnail_clear(p->source->common.window,p->target);
    return isle_post_window_thumbnail_write_photo(p->source->common.window,p->target,view->rgba5551,view->size,view->stride_bytes);
}
static const PhotoData* ab_working_photo(Context* source,const IslePhotoWorking* working) {
    const PhotoData* photo=NULL;
    ab_check(source,isle_photo_working_photo(working,&photo));
    return photo;
}
static float ab_working_present(Context* source,const IslePhotoWorking* working) {
    return ab_working_photo(source,working)->unk_04.f32;
}
static AlbumPreview ab_preview_target(Context* source,SObj* child) {
    size_t n; AlbumPreview p={source,{0}};
    for(n=0;n<source->thumbnail_count;n++)
        if(source->thumbnail_children[n]==child) { p.target=source->thumbnails[n];break; }
    if(!p.target.serial) ab_check(source,ISLE_GAME_INVALID_STATE);
    return p;
}
static int32_t ab_preview(Context* source,const PhotoData* photo,SObj* child) {
    float scale; AlbumPreview p=ab_preview_target(source,child);
    IsleSprite* sprite=ab_sprite(source,child);
    ab_check(source,source->native->preview_scale(source,&scale));
    ab_check(source,isle_post_replay_preview(source->common.replay,photo,sprite->width*2,sprite->height*2,scale,ab_consume_preview,&p));
    return photo==NULL;
}
static void ab_preview_working(Context* source,IslePhotoWorking* working,SObj* child) {
    float scale; AlbumPreview p=ab_preview_target(source,child);
    IsleSprite* sprite=ab_sprite(source,child);
    ab_check(source,source->native->preview_scale(source,&scale));
    ab_check(source,isle_post_replay_preview_working(source->common.replay,working,sprite->width*2,sprite->height*2,scale,ab_consume_preview,&p));
}
static Unk803A6C18* ab_score(Context* source,const PhotoData* photo) {
    int present=0;
    ab_check(source,isle_post_replay_score(source->common.replay,photo,&source->score_scratch,&present));
    return present?&source->score_scratch:NULL;
}
static const PhotoData* ab_album(Context* source,int32_t index) {
    IslePhotoRecord record={0};
    ab_check(source,isle_application_photos_album(source->common.photos,index,&record));
    return record.photo;
}
static void ab_copy_record(Context* source,IslePhotoWorking* to,int32_t index) {
    IslePhotoRecord record={0};
    ab_check(source,isle_application_photos_album(source->common.photos,index,&record));
    ab_check(source,isle_photo_working_copy_record(to,&record));
}
static void ab_set_album(Context* source,int32_t index,const IslePhotoWorking* working,const AlbumComment* comment) {
    ab_check(source,isle_application_photos_set_album_working(source->common.photos,index,working,comment->text));
}
static void ab_remove_album(Context* source,int32_t index) { ab_check(source,isle_application_photos_remove_album(source->common.photos,index)); }
static const uint16_t* ab_comment(Context* source,int32_t index) {
    const uint16_t* text=NULL;
    ab_check(source,isle_application_photos_album_comment(source->common.photos,index,&text));
    return text;
}
static const AlbumComment* ab_get_comment(Context* source,int32_t index) {
    const uint16_t* text=ab_comment(source,index);
    if(!text) ab_check(source,ISLE_GAME_INVALID_STATE);
    memcpy(source->comment_scratch.text,text,sizeof(source->comment_scratch.text));
    return &source->comment_scratch;
}
static void ab_set_comment(Context* source,const AlbumComment* comment,int32_t index) { ab_check(source,isle_application_photos_set_album_comment(source->common.photos,index,comment->text)); }
static void ab_comment_store(Context* source,int32_t slot,int32_t character,uint16_t value) { ab_check(source,isle_application_photos_set_album_comment_char(source->common.photos,slot,character,value)); }
static int32_t ab_comment_width(Context* source,const uint16_t* text,char* output) {
    char temp[64]; char* out=output?output:temp; size_t i,n=0;
    if(!text) ab_check(source,ISLE_GAME_INVALID_ARGUMENT);
    for(i=0;i<32 && text[i];i++) {
        uint16_t ch=text[i];
        if(ch==0xa1a1) out[n++]=' ';
        else { out[n++]=(char)(ch>>8);out[n++]=(char)(ch&255); }
    }
    if(n>=64) ab_check(source,ISLE_GAME_INVALID_STATE);
    out[n]=0;return ab_width(source,out);
}
static int32_t ab_album_capacity(Context* source) { (void)source;return ISLE_PHOTO_ALBUM_CAPACITY; }
static UnkStruct800BEDF8* ab_input(Context* source,int32_t player) { ab_check(source,source->native->input(source,player,&source->input));return &source->input; }
static void ab_input_initialize(Context* source) {
    ab_check(source,source->native->input_initialize(source));
    source->common.input_invalidate=isle_post_course_native_input_invalidate;
    ab_check(source,isle_post_course_source_install_input(&source->common));
}
static SObj* ab_cover_get(Context* source,int32_t index) {
    SObj* sprite=NULL;
    if(index>=0 && index<4) return source->album_CoverTitleSObj[index];
    if(index!=4 && index!=5) ab_check(source,ISLE_GAME_INVALID_ARGUMENT);
    ab_check(source,source->native->cover_overflow_get(source,index,&sprite));
    return sprite;
}
static void ab_cover_set(Context* source,int32_t index,SObj* sprite) {
    if(index>=0 && index<4) { source->album_CoverTitleSObj[index]=sprite;return; }
    if(index!=4 && index!=5) ab_check(source,ISLE_GAME_INVALID_ARGUMENT);
    ab_check(source,source->native->cover_overflow_set(source,index,sprite));
}
static SObj* ab_unwritten_sp4(Context* source) { SObj* out=NULL;ab_check(source,source->native->unwritten_sp4(source,&out));return out; }
static int32_t ab_empty_last(Context* source) { int32_t out;ab_check(source,source->native->empty_last_photo(source,&out));return out; }
static const uint8_t* ab_grid(Context* source,uint32_t page) {
    const uint8_t* bytes=NULL;size_t size=0;
    if(page==0) return ab_character_grid_0;
    if(page>2) ab_check(source,ISLE_GAME_INVALID_ARGUMENT);
    ab_check(source,source->native->character_grid(source,page,&bytes,&size));
    if(!bytes || size<190) ab_check(source,ISLE_GAME_INVALID_STATE);
    return bytes;
}
static void ab_rebind(Context* source,SObj* sprite,uint32_t asset) { ab_check(source,isle_post_window_sprite_rebind(source->common.window,sprite,asset)); }
static void ab_clone_sprite(Context* source,SObj* dest,SObj* origin) {
    ab_check(source,isle_post_window_sprite_assign_thumbnail(source->common.window,dest,origin));
}
static int32_t ab_slot_id(Context* source,int32_t index) { int out=0;ab_check(source,isle_post_window_layout_slot_id(source->common.window,(size_t)index,&out));return out; }
static IslePostCourseWindow* ab_selector(Context* source) { return source->common.window; }
static void ab_layout_create(Context* source,const UIButton* table) {
    size_t count=0;IslePostCourseButtonTable handle={0};
    if(table==album_CoverButtonsDef) count=ARRAY_COUNT(album_CoverButtonsDef);
    else if(table==album_AlbumButtonsDef) count=ARRAY_COUNT(album_AlbumButtonsDef);
    else if(table==album_PhotoButtonsDef) count=ARRAY_COUNT(album_PhotoButtonsDef);
    else ab_check(source,ISLE_GAME_INVALID_ARGUMENT);
    ab_check(source,isle_post_window_table_borrow(source->common.window,table,count,&handle));
    ab_check(source,isle_post_window_layout_create(source->common.window,handle));
}
static const char* ab_player_name(Context* source) { const char* out=NULL;ab_check(source,source->native->player_name(source,&out));return out; }
static const char* ab_pokemon_name(Context* source,int32_t id) { const char* out=NULL;ab_check(source,source->native->pokemon_name(source,id,&out));return out; }
static const char* ab_level_name(Context* source,int32_t id) { const char* out=NULL;ab_check(source,source->native->level_name(source,id,&out));return out; }
#define AB_VOID0(name,call) static void name(Context* source) { ab_check(source,(call)); }
#define AB_VOID(name,args,call) static void name args { ab_check(source,(call)); }
AB_VOID0(ab_text_initialize,isle_post_window_text_initialize(source->common.window))
AB_VOID0(ab_replay_initialize,source->native->replay_initialize(source))
AB_VOID0(ab_replay_cleanup,source->native->replay_cleanup(source))
AB_VOID0(ab_focus_create,isle_post_window_focus_create(source->common.window))
AB_VOID0(ab_focus_front,isle_post_window_focus_front(source->common.window))
AB_VOID0(ab_layout_initialize,isle_post_window_layout_initialize(source->common.window))
AB_VOID0(ab_layout_hide,isle_post_window_layout_hide(source->common.window))
AB_VOID0(ab_layout_wait,isle_post_window_layout_wait(source->common.window))
AB_VOID(ab_text_font,(Context* source,int n),isle_post_window_text_font(source->common.window,n))
AB_VOID(ab_text_delay,(Context* source,int n),isle_post_window_text_delay(source->common.window,n))
AB_VOID(ab_text_shadow,(Context* source,int n),isle_post_window_text_shadow_offset(source->common.window,n))
AB_VOID(ab_text_spacing,(Context* source,int x,int y),isle_post_window_text_spacing(source->common.window,x,y))
AB_VOID(ab_focus_show,(Context* source,int n),isle_post_window_focus_visible(source->common.window,n))
AB_VOID(ab_focus_stop,(Context* source,int n),isle_post_window_focus_stop(source->common.window,n))
AB_VOID(ab_focus_position,(Context* source,int x,int y),isle_post_window_focus_position(source->common.window,x,y))
AB_VOID(ab_focus_target,(Context* source,int x,int y),isle_post_window_focus_target_position(source->common.window,x,y))
AB_VOID(ab_focus_size,(Context* source,int x,int y),isle_post_window_focus_size(source->common.window,x,y))
AB_VOID(ab_focus_target_size,(Context* source,int x,int y),isle_post_window_focus_target_size(source->common.window,x,y))
AB_VOID(ab_layout_header,(Context* source,int n),isle_post_window_layout_header_flags(source->common.window,n))
AB_VOID(ab_layout_show_header,(Context* source,int n),isle_post_window_layout_show_header(source->common.window,n))
AB_VOID(ab_layout_hide_header,(Context* source,int n),isle_post_window_layout_hide_header(source->common.window,n))
AB_VOID(ab_layout_panel,(Context* source,int n),isle_post_window_layout_panel(source->common.window,n))
AB_VOID(ab_frame_position,(Context* source,UIFrame* f,int x,int y),isle_post_window_frame_position(source->common.window,f->handle,x,y))
AB_VOID(ab_frame_show,(Context* source,UIFrame* f,int n),isle_post_window_frame_visible(source->common.window,f->handle,n))
AB_VOID(ab_frame_fade_in,(Context* source,UIFrame* f,int n),isle_post_window_fade_in(source->common.window,f->handle,n))
AB_VOID(ab_frame_fade_out,(Context* source,UIFrame* f),isle_post_window_fade_out(source->common.window,f->handle))
AB_VOID(ab_repeat,(Context* source,int x,int y),source->native->input_repeat(source,x,y))
AB_VOID(ab_input_mask,(Context* source,uint32_t n),source->native->input_mask(source,n))
AB_VOID(ab_aux_camera,(Context* source,int x,int y),source->native->aux_camera(source,x,y))
AB_VOID(ab_sound,(Context* source,int n),source->native->sound(source,n))
AB_VOID(ab_song,(Context* source,int x,int y),source->native->song(source,x,y))
AB_VOID(ab_volume,(Context* source,int x,int y),source->native->volume(source,x,y))
AB_VOID(ab_volume_smooth,(Context* source,int x,int y,int z),source->native->volume_smooth(source,x,y,z))
static void ab_font_option(Context* source,int flags,int enabled) {
    /* Exact Original calls are flags1 only (text.c221-226, Album c1050/1063). */
    if(flags!=1) ab_check(source,ISLE_GAME_INVALID_ARGUMENT);
    ab_check(source,isle_post_window_text_proportional(source->common.window,enabled));
}
#undef AB_VOID
#undef AB_VOID0
static void ab_scene_result(Context* source,int32_t result) { ab_check(source,source->native->scene_result(source,result));source->actual_result=result; }
#include "album_original.inc"
void isle_album_source_display_list(Context* source,void* cursor) { ab_check(source,source->native->display_list(source,cursor)); }
void isle_album_source_render_state(Context* source,int32_t a,int32_t b) { ab_check(source,source->native->render_state(source,a,b)); }
void isle_album_func_801DCC74_9A6EC4(void* user) {
    IsleAlbumDeallocation* allocation=(IsleAlbumDeallocation*)user;
    Context* source=allocation->source;
    ab_check(source,source->native->deallocate(source,allocation->allocation));
}
void isle_album_source_init(Context* source) { isle_album_album_Init(source); }
void isle_album_source_init_call(IslePostCourseSceneSource* common,void* user) { (void)user;isle_album_source_init((Context*)common); }
IsleGameResult isle_album_source_initialize(Context* source,const IsleAlbumNative* native,IslePhotoWorking* first,IslePhotoWorking* second) {
    IslePostCourseSceneSource common;
    if(!source || !native || !first || !second || first==second) return ISLE_GAME_INVALID_ARGUMENT;
    if(!source->common.owner || !source->common.domain || !source->common.game ||
       !source->common.window || !source->common.photos) return ISLE_GAME_INVALID_STATE;
    if(!native->ui_heap || !native->input_initialize || !native->input ||
       !native->input_repeat || !native->input_mask || !native->camera ||
       !native->aux_camera || !native->process_object || !native->replay_initialize ||
       !native->replay_cleanup || !native->preview_scale || !native->sound ||
       !native->song || !native->volume || !native->volume_smooth ||
       !native->player_name || !native->pokemon_name || !native->level_name ||
       !native->scene_result || !native->entry_begin || !native->entry_poll ||
       !native->notices_or || !native->display_list || !native->render_state ||
       !native->unwritten_sp4 || !native->cover_overflow_get || !native->cover_overflow_set || !native->empty_last_photo ||
       !native->character_grid || !native->sprite_clear_links ||
       !native->float_format_integer || !native->inactive_letter_width ||
       !native->deallocate) return ISLE_GAME_INVALID_ARGUMENT;
    common=source->common;
    memset(source,0,sizeof(*source));
    source->common=common;
    source->common.scene=12;
    source->native=native;
    source->album_D_80250170_A1A3C0.photoData=first;
    source->album_D_80250558_A1A7A8.photoData=second;
    source->D_80208B54_9D2DA4=-1;
    source->album_IsEditingComment=1;
    source->album_CursorAlpha=10;
    source->album_CursorAlphaRate=5;
    source->album_LetterAlpha=10;
    source->album_LetterAlphaRate=5;
    source->D_80208C18_9D2E68=-1;
    source->album_CurrentColumn=-1;
    source->album_CurrentRow=-1;
    source->album_DraggedPhotoColumn=-1;
    source->album_DraggedPhotoRow=-1;
    source->D_80208C2C_9D2E7C=-1;
    source->D_80208C30_9D2E80=-1;
    return ISLE_GAME_OK;
}
void isle_album_source_entry(void* user) {
    IsleAlbumEntry* entry=(IsleAlbumEntry*)user;
    Context* source=entry->source;
    const IsleAlbumNative* native=source->native;
    IsleContinuationDomain* domain=source->common.domain;
    void* owner=source->common.owner;
    int retired=0;
    int32_t result=0;
    IsleGameResult status;
    entry->complete=0;
    if(entry->argument!=12) {
        entry->status=ISLE_GAME_INVALID_ARGUMENT;
        (void)isle_continuation_fail(domain,"Album12 entry argument");
        return;
    }
    source->entry_argument=entry->argument;
    status=native->entry_begin(source,entry->argument);
    /* No Source storage dereference after returning begin: independent entry,
     * owner, native table and domain survive actual scene retirement. */
    if(status!=ISLE_GAME_OK) {
        entry->status=status;
        (void)isle_continuation_fail(domain,"Album12 entry setup");
        return;
    }
    while(!retired) {
        if(isle_continuation_wait(domain,1)!=ISLE_CONT_OK) {
            entry->status=ISLE_GAME_INVALID_STATE;
            (void)isle_continuation_fail(domain,"Album12 entry wait");
            return;
        }
        status=native->entry_poll(owner,&retired,&result);
        if(status!=ISLE_GAME_OK) {
            entry->status=status;
            (void)isle_continuation_fail(domain,"Album12 entry retirement");
            return;
        }
    }
    /* Original album_Start c151: notice occurs AFTER successful setup return,
     * including all native/Photo/graphics joins, never on creation/cancel. */
    status=native->notices_or(owner,UINT32_C(0x100000));
    if(status!=ISLE_GAME_OK) {
        entry->status=status;
        (void)isle_continuation_fail(domain,"Album12 returned notice");
        return;
    }
    entry->result=result;
    entry->complete=1;
    entry->status=ISLE_GAME_OK;
}
