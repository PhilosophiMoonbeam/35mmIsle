#include "report_source.h"
#include "original_sort_source.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
typedef IsleReportSource Context;
typedef IsleGameObject GObj;
typedef IsleGameSprite SObj;
typedef IsleReportElement UIElement;
typedef IsleReportFrame UIFrame;
typedef IslePlayerPhotoData PhotoData;
typedef IslePostCourseScoredPhoto Unk803A6C18;
typedef IsleReportInput UnkStruct800BEDF8;
typedef IslePostCourseButton UIButton;
typedef IslePostCourseColor ucolor;
#define ARRAY_COUNT(a) (sizeof(a)/sizeof((a)[0]))
#include "report_declarations.inc"
#define rp_check(source,result) isle_post_course_source_checked(&(source)->common,(result),__func__,#result,__LINE__)
static void rp_wait(Context* source,int32_t count) { isle_post_course_source_wait(&source->common,count); }
static SObj* rp_root(Context* source,GObj* object) {
    SObj* out=NULL;
    rp_check(source,isle_post_window_sprite_root(source->common.window,object,&out));
    return out;
}
static IsleSprite* rp_sprite(Context* source,SObj* object) {
    IsleSprite* out=NULL;
    rp_check(source,isle_post_window_sprite_view(source->common.window,object,&out));
    return out;
}
static SObj* rp_link(Context* source,SObj* object,int child) {
    GObj* owner=NULL;
    SObj *parent=NULL,*first=NULL,*next=NULL,*previous=NULL;
    rp_check(source,isle_post_window_sprite_links(source->common.window,object,&owner,&parent,&first,&next,&previous));
    return child?first:next;
}
static void rp_rebind(Context* source,SObj* object,uint32_t prototype) {
    rp_check(source,isle_report_native_sprite_rebind(source,object,prototype));
}
static int32_t rp_slot(Context* source,int32_t index) {
    int out=0;
    if(index<0) rp_check(source,ISLE_GAME_INVALID_ARGUMENT);
    rp_check(source,isle_post_window_layout_slot_id(source->common.window,(size_t)index,&out));
    return out;
}
static IslePostCourseWindow* rp_selector(Context* source) { return source->common.window; }
static void rp_layout_create(Context* source,const UIButton* buttons) {
    IslePostCourseButtonTable table={0}; size_t i;
    for(i=0;i<8;++i) if(source->data->buttons[i]==buttons) break;
    if(i==8) rp_check(source,ISLE_GAME_INVALID_ARGUMENT);
    rp_check(source,isle_post_window_table_borrow(source->common.window,buttons,source->data->button_count[i],&table));
    rp_check(source,isle_post_window_layout_create(source->common.window,table));
}
static void rp_delete(Context* source,GObj* object) { rp_check(source,isle_game_delete_object(source->common.game,object)); }
static GObj* rp_process(Context* source,IsleGameSourceCallback callback,int32_t link,int32_t dllink,void* list) {
    GObj* object=NULL; IsleGameProcess* process=NULL;
    rp_check(source,source->native->process_object(source,link,dllink,list,&object));
    rp_check(source,isle_game_add_source_process(source->common.game,object,callback,ISLE_GAME_STACKFUL,0,&process));
    return object;
}
static GObj* rp_sprite_object(Context* source,IsleGameSourceCallback callback,int32_t link,uint32_t prototype,int32_t kind) {
    GObj* out=NULL; SObj* sprite=NULL;
    rp_check(source,isle_post_window_sprite_object(source->common.window,link,prototype,callback,kind,&out,&sprite));
    return out;
}
static GObj* rp_sprite_synchronous(Context* source,IsleGameSourceCallback callback,int32_t link,uint32_t prototype) { return rp_sprite_object(source,callback,link,prototype,1); }
static GObj* rp_sprite_stackful(Context* source,IsleGameSourceCallback callback,int32_t link,uint32_t prototype) { return rp_sprite_object(source,callback,link,prototype,0); }
static SObj* rp_child(Context* source,SObj* parent,uint32_t prototype) {
    SObj* out=NULL; rp_check(source,isle_post_window_sprite_child(source->common.window,parent,prototype,&out)); return out;
}
static SObj* rp_sibling(Context* source,SObj* parent,uint32_t prototype) {
    SObj* out=NULL; rp_check(source,isle_post_window_sprite_sibling(source->common.window,parent,prototype,&out)); return out;
}
static SObj* rp_add(Context* source,GObj* object,uint32_t prototype) {
    SObj* out=NULL; rp_check(source,source->native->sprite_add(source,object,prototype,&out)); return out;
}
static void rp_remove(Context* source,GObj* object) { rp_check(source,source->native->sprite_remove(source,object)); }
static void rp_detach(Context* source,SObj* object,int32_t which) { rp_check(source,source->native->sprite_detach(source,object,which)); }
static void rp_bitmap(Context* source,SObj* object,int32_t sign) {
    if(sign<0||sign>=6) rp_check(source,ISLE_GAME_INVALID_ARGUMENT);
    rp_check(source,source->native->sprite_bitmap(source,object,source->data->sign_pixels[sign],source->data->sign_pixel_sizes[sign]));
}
static uint8_t* rp_thumbnail(Context* source,int32_t sign,SObj* object) {
    uint8_t* pixels=NULL;
    size_t size=0;
    if(sign<0||sign>=6) rp_check(source,ISLE_GAME_INVALID_ARGUMENT);
    rp_check(source,source->native->thumbnail_bind(source,sign,object,&source->D_80230DC8_A27738[sign].thumbnail,&pixels,&size));
    if(!pixels||size!=0xFD8) rp_check(source,ISLE_GAME_INVALID_STATE);
    return pixels;
}
typedef struct ReportPreviewConsumer { Context* source; SObj* sprite; } ReportPreviewConsumer;
static IsleGameResult rp_preview_consume(void* user,const IslePostCoursePreview* image) {
    ReportPreviewConsumer* consumer=(ReportPreviewConsumer*)user;
    return isle_report_native_preview_consume(consumer->source,consumer->sprite,image);
}
static int32_t rp_preview(Context* source,const PhotoData* photo,SObj* object) {
    ReportPreviewConsumer consumer={source,object};
    IsleSprite* sprite;
    float scale=0;
    rp_check(source,source->native->preview(source,photo,object));
    sprite=rp_sprite(source,object);
    rp_check(source,isle_report_native_preview_scale(source,&scale));
    rp_check(source,isle_post_replay_preview(source->common.replay,photo,
        sprite->width*2,sprite->height*2,scale,rp_preview_consume,&consumer));
    return photo==NULL;
}
static Unk803A6C18* rp_score(Context* source,const PhotoData* photo) {
    int present=0;
    rp_check(source,isle_post_replay_score(source->common.replay,photo,&source->score_scratch,&present));
    return present?&source->score_scratch:NULL;
}
static UnkStruct800BEDF8* rp_input(Context* source,int32_t player) {
    rp_check(source,source->native->input(source,player,&source->input)); return &source->input;
}
static void rp_camera(Context* source) {
    GObj* object=NULL; IsleGameProcess* process=NULL;
    rp_check(source,source->native->camera(source,&object));
    rp_check(source,isle_game_add_source_process(source->common.game,object,isle_report_func_801DCA14_9D3384,ISLE_GAME_STACKFUL,0,&process));
}
static int32_t rp_sort_compare(void* user,const void* a,const void* b) {
    Context* source=(Context*)user; return source->sort_compare(source,a,b);
}
static void rp_qsort(Context* source,void* items,int32_t count,int32_t size,IsleReportCompare compare) {
    if(count<0||size<=0||!compare||source->sort_compare) rp_check(source,ISLE_GAME_INVALID_ARGUMENT);
    source->sort_compare=compare;
    isle_original_source_qsort2(source,items,(size_t)count,(size_t)size,rp_sort_compare);
    source->sort_compare=NULL;
}
/* Original UI allocator lifetimes become bounded Source handle cells; a cell
 * is reused only after actual Window deletion has retired its incarnation.
 * Native handles themselves are never reused or copied from SDK layouts. */
static UIElement* rp_element_create(Context* source, int x, int y, int w, int h, uint32_t flags) {
    UIElement* output;
    size_t slot;
    for(slot=0;slot<ARRAY_COUNT(source->elements);++slot) if(!source->element_live[slot]) break;
    if(slot==ARRAY_COUNT(source->elements)) rp_check(source,ISLE_GAME_EXHAUSTED);
    output=&source->elements[slot];
    source->element_live[slot]=1;
    rp_check(source, isle_post_window_element_create(source->common.window, x, y, w, h, flags, &output->handle));
    return output;
}
static UIFrame* rp_frame_create(Context* source) {
    rp_check(source, isle_post_window_frame_create(source->common.window, &source->frame.handle));
    return &source->frame;
}
static void rp_element_delete(Context* source, UIElement* element) {
    size_t i;
    rp_check(source, isle_post_window_element_delete(source->common.window, element->handle));
    for(i=0;i<ARRAY_COUNT(source->elements);++i) if(element==&source->elements[i]) { source->element_live[i]=0; return; }
    rp_check(source,ISLE_GAME_INVALID_ARGUMENT);
}
static void rp_element_draw(Context* source, UIElement* element) {
    rp_check(source, isle_post_window_element_clear(source->common.window, element->handle));
}
static void rp_element_background(Context* source, UIElement* element) {
    rp_check(source, isle_post_window_element_background(source->common.window, element->handle));
}
static void rp_element_state(Context* source, UIElement* element, int state) {
    if (element == NULL) return; /* Original UIElement_SetState accepts NULL. */
    rp_check(source, isle_post_window_element_visible(source->common.window, element->handle, state == UI_NORMAL));
}
static void rp_element_font(Context* source, UIElement* element, int font) {
    rp_check(source, isle_post_window_element_font(source->common.window, element->handle, font));
}
static void rp_element_position(Context* source, UIElement* element, int x, int y) {
    rp_check(source, isle_post_window_element_text_position(source->common.window, element->handle, x, y));
}
static void rp_element_color(Context* source, UIElement* element, int foreground, int r, int g, int b, int a) {
    rp_check(source, isle_post_window_element_color(source->common.window, element->handle,
        foreground == UI_FOREGROUND, (IslePostCourseColor){(uint8_t)r,(uint8_t)g,(uint8_t)b,(uint8_t)a}));
}
static int32_t rp_print(Context* source, UIElement* element, const char* text, int ascii) {
    IslePostCourseBytes bytes = {text, text ? strlen(text) : 0};
    rp_check(source, isle_post_window_print(source->common.window, element->handle, bytes, ascii, NULL));
    return 0;
}
static void rp_print_source(Context* source, UIElement* element, const char* text) {
    (void)rp_print(source, element, text, 0);
}
static int32_t rp_formatted(Context* source, UIElement* element, const char* format, ...) {
    int count;
    va_list args;
    va_start(args, format);
    count = vsnprintf(source->formatted, sizeof(source->formatted), format, args);
    va_end(args);
    if (count < 0 || (size_t)count >= sizeof(source->formatted)) rp_check(source, ISLE_GAME_INVALID_STATE);
    rp_print_source(source, element, source->formatted);
    return count;
}
/* 8486E0.c44-62: the original s16[]/char* alias becomes explicit big-endian
 * source bytes; it never serializes host-endian uint16_t objects. */
static char* rp_number(Context* source, const char* format, int32_t value) {
    int count = snprintf(source->number_ascii, sizeof(source->number_ascii), format, value);
    int i;
    if (count < 0 || (size_t)count >= sizeof(source->number_ascii)) rp_check(source, ISLE_GAME_INVALID_STATE);
    for (i = 0; i < count; ++i) {
        unsigned char ch = (unsigned char)source->number_ascii[i];
        source->number_source[i*2] = (char)(ch >= '0' && ch <= '9' ? 0xa3 : 0xa1);
        source->number_source[i*2+1] = (char)(ch >= '0' && ch <= '9' ? ch-'0'+0xb0 : 0xa1);
    }
    source->number_source[count*2] = 0;
    return source->number_source;
}
static int32_t rp_width(Context* source, const char* text) {
    int output = 0;
    rp_check(source, isle_post_window_text_width(source->common.window,
        (IslePostCourseBytes){text, text ? strlen(text) : 0}, &output));
    return output;
}
static void rp_frame_color(Context* source, UIFrame* frame, ucolor* output) {
    rp_check(source, isle_post_window_frame_color(source->common.window, frame->handle, output));
}
static const PhotoData* rp_report(Context* source, int32_t species) {
    IslePhotoRecord output = {0};
    rp_check(source, isle_application_photos_report(source->common.photos, species, &output));
    return output.photo;
}
static int32_t rp_report_present(Context* source, int32_t species) {
    int output = 0;
    rp_check(source, isle_application_photos_report_present(source->common.photos, species, &output));
    return output;
}
static int32_t rp_report_score(Context* source, int32_t species) {
    int32_t output = 0;
    rp_check(source, isle_application_photos_report_score(source->common.photos, species, &output));
    return output;
}

static int32_t rp_report_count(Context* source) {
    int32_t out=0;
    rp_check(source,isle_application_photos_report_count(source->common.photos,&out));
    return out;
}
#define RP_VOID0(name,call) static void name(Context* source) { rp_check(source,(call)); }
#define RP_VOID(name,parameters,call) static void name parameters { rp_check(source,(call)); }
RP_VOID0(rp_text_initialize,isle_post_window_text_initialize(source->common.window))
RP_VOID0(rp_replay_initialize,source->native->replay_initialize(source))
RP_VOID0(rp_focus_create,isle_post_window_focus_create(source->common.window))
RP_VOID0(rp_focus_front,isle_post_window_focus_front(source->common.window))
RP_VOID0(rp_layout_initialize,isle_post_window_layout_initialize(source->common.window))
RP_VOID0(rp_layout_hide,isle_post_window_layout_hide(source->common.window))
RP_VOID0(rp_layout_wait,isle_post_window_layout_wait(source->common.window))
RP_VOID(rp_text_delay,(Context* source,int v),isle_post_window_text_delay(source->common.window,v))
RP_VOID(rp_text_shadow,(Context* source,int v),isle_post_window_text_shadow_offset(source->common.window,v))
RP_VOID(rp_text_spacing,(Context* source,int a,int b),isle_post_window_text_spacing(source->common.window,a,b))
RP_VOID(rp_text_skip,(Context* source,int v),isle_post_window_text_skip(source->common.window,v))
RP_VOID(rp_focus_show,(Context* source,int v),isle_post_window_focus_visible(source->common.window,v))
RP_VOID(rp_focus_position,(Context* source,int a,int b),isle_post_window_focus_position(source->common.window,a,b))
RP_VOID(rp_focus_target,(Context* source,int a,int b),isle_post_window_focus_target_position(source->common.window,a,b))
RP_VOID(rp_focus_size,(Context* source,int a,int b),isle_post_window_focus_size(source->common.window,a,b))
RP_VOID(rp_focus_target_size,(Context* source,int a,int b),isle_post_window_focus_target_size(source->common.window,a,b))
RP_VOID(rp_layout_header,(Context* source,int v),isle_post_window_layout_header_flags(source->common.window,v))
RP_VOID(rp_layout_hide_header,(Context* source,int v),isle_post_window_layout_hide_header(source->common.window,v))
RP_VOID(rp_layout_show_header,(Context* source,int v),isle_post_window_layout_show_header(source->common.window,v))
RP_VOID(rp_layout_panel,(Context* source,int v),isle_post_window_layout_panel(source->common.window,v))
RP_VOID(rp_frame_style,(Context* source,UIFrame* f,int v),isle_post_window_frame_style(source->common.window,f->handle,v))
RP_VOID(rp_frame_position,(Context* source,UIFrame* f,int a,int b),isle_post_window_frame_position(source->common.window,f->handle,a,b))
RP_VOID(rp_frame_show,(Context* source,UIFrame* f,int v),isle_post_window_frame_visible(source->common.window,f->handle,v))
RP_VOID(rp_frame_fade_in,(Context* source,UIFrame* f,int v),isle_post_window_fade_in(source->common.window,f->handle,v))
RP_VOID(rp_frame_fade_out,(Context* source,UIFrame* f),isle_post_window_fade_out(source->common.window,f->handle))
RP_VOID(rp_element_sprite_color,(Context* source,UIElement* e,int r,int g,int b,int a),isle_post_window_element_sprite_color(source->common.window,e->handle,(IslePostCourseColor){(uint8_t)r,(uint8_t)g,(uint8_t)b,(uint8_t)a}))
RP_VOID(rp_repeat,(Context* source,int a,int b),source->native->repeat(source,a,b))
RP_VOID(rp_input_mask,(Context* source,uint32_t v),source->native->input_mask(source,v))
RP_VOID(rp_aux_camera,(Context* source,int a,int b),source->native->aux_camera(source,a,b))
RP_VOID(rp_sound,(Context* source,int v),source->native->sound(source,v))
RP_VOID(rp_song,(Context* source,int a,int b),source->native->song(source,a,b))
RP_VOID(rp_volume,(Context* source,int a,int b),source->native->volume(source,a,b))
RP_VOID(rp_volume_smooth,(Context* source,int a,int b,int c),source->native->volume_smooth(source,a,b,c))
RP_VOID(rp_scene_result,(Context* source,int v),source->native->scene_result(source,v))
RP_VOID(rp_window_mode,(Context* source,int a,int b),source->native->window_mode(source,a,b))
#undef RP_VOID
#undef RP_VOID0
static void rp_input_initialize(Context* source) {
    rp_check(source,source->native->input_initialize(source));
    source->common.input_invalidate=isle_post_course_native_input_invalidate;
    rp_check(source,isle_post_course_source_install_input(&source->common));
}
static int32_t rp_flag(Context* source,int32_t id) {
    int32_t out=0; rp_check(source,source->native->player_flag(source,id,&out)); return out;
}
static int32_t rp_pokemon_index(Context* source,int32_t id) {
    int32_t out=0; rp_check(source,source->native->pokemon_index(source,id,&out)); return out;
}
static int32_t rp_selected_course(Context* source) {
    int32_t out=0; rp_check(source,source->native->selected_course(source,&out)); return out;
}
static const char* rp_player_name(Context* source) {
    const char* out=NULL; rp_check(source,source->native->player_name(source,&out)); return out;
}
static const char* rp_pokemon_name(Context* source,int32_t id) {
    const char* out=NULL; rp_check(source,source->native->pokemon_name(source,id,&out)); return out;
}
static const char* rp_level_name(Context* source,int32_t id) {
    const char* out=NULL; rp_check(source,source->native->level_name(source,id,&out)); return out;
}
#include "report_original.inc"

void isle_report_source_initialize(IsleReportSource* source) {
    IslePostCourseSceneSource common=source->common;
    const IsleReportNative* native=source->native;
    size_t i;
    memset(source,0,sizeof(*source));
    source->common=common;
    source->common.scene=14;
    source->native=native;
    if(!native) rp_check(source,ISLE_GAME_INVALID_ARGUMENT);
    /* Every leaf is mandatory; the caller supplies one immutable typed table. */
#define REQUIRED(member) if(!native->member) rp_check(source,ISLE_GAME_INVALID_ARGUMENT)
    REQUIRED(prepare_data); REQUIRED(ui_heap); REQUIRED(input_initialize);
    REQUIRED(input); REQUIRED(repeat); REQUIRED(input_mask); REQUIRED(camera);
    REQUIRED(aux_camera); REQUIRED(process_object); REQUIRED(replay_initialize);
    REQUIRED(sound); REQUIRED(song); REQUIRED(volume); REQUIRED(volume_smooth);
    REQUIRED(player_flag); REQUIRED(player_name); REQUIRED(pokemon_name);
    REQUIRED(level_name); REQUIRED(selected_course); REQUIRED(pokemon_index);
    REQUIRED(scene_result); REQUIRED(sprite_add); REQUIRED(sprite_remove);
    REQUIRED(sprite_detach); REQUIRED(sprite_bitmap); REQUIRED(thumbnail_bind);
    REQUIRED(preview); REQUIRED(window_mode); REQUIRED(render_bind); REQUIRED(render_state);
    REQUIRED(setup_begin); REQUIRED(setup_poll); REQUIRED(notices_or_owner);
#undef REQUIRED
    rp_check(source,native->prepare_data(source,&source->data));
    if(!source->data||!source->data->generation) rp_check(source,ISLE_GAME_INVALID_STATE);
    for(i=0;i<6;++i)
        if(!source->data->sign_sprites[i]||!source->data->sign_pixels[i]||
           source->data->sign_pixel_sizes[i]!=0xFD8)
            rp_check(source,ISLE_GAME_INVALID_STATE);
    for(i=0;i<12;++i)
        if(!source->data->captions[i]||
           source->data->captions[i][source->data->caption_count[i]]!=NULL)
            rp_check(source,ISLE_GAME_INVALID_STATE);
    for(i=0;i<8;++i)
        if(!source->data->buttons[i]||!source->data->button_count[i])
            rp_check(source,ISLE_GAME_INVALID_STATE);
#define INITIAL(field) source->field=source->data->field
    INITIAL(D_80202D30_9F96A0);
    INITIAL(D_80202EC0_9F9830); INITIAL(D_80202EC4_9F9834);
    INITIAL(D_80202FF8_9F9968); INITIAL(D_80202FFC_9F996C);
    INITIAL(D_80203000_9F9970); INITIAL(D_80203004_9F9974);
    INITIAL(D_80203008_9F9978); INITIAL(D_8020300C_9F997C);
    INITIAL(D_80203014_9F9984);
#undef INITIAL
    switch(source->data->comparator_origin) {
    case 0: break;
    case 0x801DE118: source->D_80202D2C_9F969C=isle_report_func_801DE118_9D4A88; break;
    case 0x801DE18C: source->D_80202D2C_9F969C=isle_report_func_801DE18C_9D4AFC; break;
    case 0x801DE204: source->D_80202D2C_9F969C=isle_report_func_801DE204_9D4B74; break;
    case 0x801DE318: source->D_80202D2C_9F969C=isle_report_func_801DE318_9D4C88; break;
    default: rp_check(source,ISLE_GAME_INVALID_STATE);
    }
}
void isle_report_source_init(IsleReportSource* source) {
    isle_report_func_801DCBF4_9D3564(source);
}
void isle_report_source_init_call(IslePostCourseSceneSource* common,void* user) {
    (void)user; isle_report_source_init((IsleReportSource*)common);
}
void isle_report_source_entry(void* user) {
    IsleReportEntry* entry=(IsleReportEntry*)user;
    Context* source=entry->source;
    IsleContinuationDomain* domain=source->common.domain;
    void* owner=source->common.owner;
    const IsleReportNative* native=source->native;
    int retired=0;
    int32_t result=0;
    IsleGameResult status;
    if(entry->argument!=14) {
        entry->status=ISLE_GAME_INVALID_ARGUMENT;
        (void)isle_continuation_fail(domain,"Report14 entry argument");
        return;
    }
    status=native->setup_begin(source,entry->argument);
    /* No dereference of Source after setup_begin: owner/table/entry backing are
     * independently retained through actual scene retirement and setup return. */
    if(status!=ISLE_GAME_OK) goto fail;
    while(!retired) {
        if(isle_continuation_wait(domain,1)!=ISLE_CONT_OK) {
            status=ISLE_GAME_INVALID_STATE;
            goto fail;
        }
        status=native->setup_poll(owner,&retired,&result);
        if(status!=ISLE_GAME_OK) goto fail;
    }
    status=native->notices_or_owner(owner,UINT32_C(0x200000));
    if(status!=ISLE_GAME_OK) goto fail;
    entry->result=result;
    entry->status=ISLE_GAME_OK;
    return;
fail:
    entry->status=status;
    (void)isle_continuation_fail(domain,"Report14 setup/retirement");
}
