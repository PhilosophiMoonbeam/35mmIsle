#include "photo_check_source.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* The private materializer admits only pinned original definitions/data. These
 * aliases describe semantic native values, never an SDK or guest layout. */
typedef IslePhotoCheckSource Context;
typedef IsleGameObject GObj;
typedef IsleGameSprite SObj;
typedef IslePhotoCheckPhoto Photo;
typedef IslePlayerPhotoData PhotoData;
typedef IslePhotoCheckElement UIElement;
typedef IslePhotoCheckFrame UIFrame;
typedef IslePhotoCheckObjPair ObjPair;
typedef IslePostCourseScoredPhoto Unk803A6C18;
typedef IslePhotoCheckInput UnkStruct800BEDF8;
typedef IslePostCourseColor ucolor;
#define ARRAY_COUNT(a) (sizeof(a) / sizeof((a)[0]))
#define ABS(x) ((x) < 0 ? -(x) : (x))
#include "photo_check_declarations.inc"
#include "photo_check_data.inc"

#define pc_check(source, result) \
    isle_post_course_source_checked(&((source)->common), (result), __func__, #result, __LINE__)
static void pc_wait_at(Context* source,int32_t count,const char* caller) {
    if (!strcmp(caller,"isle_photo_check_func_80374F30_8486E0"))
        isle_post_course_ack_wait(&source->common,count);
    else isle_post_course_source_wait(&source->common,count);
}
#define pc_wait(source,count) pc_wait_at(source,count,__func__)
static SObj* pc_root(Context* source, GObj* object) {
    SObj* sprite = NULL;
    pc_check(source, isle_post_window_sprite_root(source->common.window, object, &sprite));
    return sprite;
}
static IsleSprite* pc_sprite(Context* source, SObj* object) {
    IsleSprite* sprite = NULL;
    pc_check(source, isle_post_window_sprite_view(source->common.window, object, &sprite));
    return sprite;
}
static void pc_userdata_set(Context* source, GObj* object, const void* value) {
    pc_check(source, isle_photo_check_native_userdata_set(source, object, value));
}
static GObj* pc_process(Context* source, IsleGameSourceCallback callback,
                       int32_t link, int32_t dllink, void* dlist) {
    GObj* object = NULL;
    IsleGameProcess* process = NULL;
    pc_check(source, isle_photo_check_native_process_object(source, link, dllink, dlist, &object));
    pc_check(source, isle_game_add_source_process(source->common.game, object, callback,
        ISLE_GAME_STACKFUL, 0, &process));
    return object;
}
static void pc_delete(Context* source, GObj* object) {
    pc_check(source, isle_game_delete_object(source->common.game, object));
}
static GObj* pc_sprite_object(Context* source, IsleGameSourceCallback callback,
                             int32_t link, uint32_t prototype, int32_t kind) {
    GObj* object = NULL;
    SObj* sprite = NULL;
    pc_check(source, isle_post_window_sprite_object(source->common.window, link,
        prototype, callback, kind, &object, &sprite));
    return object;
}
static GObj* pc_sprite_stackful(Context* source, IsleGameSourceCallback callback,
                               int32_t link, uint32_t prototype) {
    return pc_sprite_object(source, callback, link, prototype, 0);
}
static GObj* pc_sprite_synchronous(Context* source, IsleGameSourceCallback callback,
                                  int32_t link, uint32_t prototype) {
    return pc_sprite_object(source, callback, link, prototype, 1);
}
static SObj* pc_sibling(Context* source, SObj* parent, uint32_t prototype) {
    SObj* output = NULL;
    pc_check(source, isle_post_window_sprite_sibling(source->common.window, parent, prototype, &output));
    return output;
}
static SObj* pc_child(Context* source, SObj* parent, uint32_t prototype) {
    SObj* output = NULL;
    pc_check(source, isle_post_window_sprite_child(source->common.window, parent, prototype, &output));
    return output;
}
static void pc_thumbnail(Context* source, int32_t index, SObj* child) {
    pc_check(source, isle_post_window_thumbnail_create(source->common.window, child,
        UINT32_C(0x801f3b10), &source->D_802290A0_9D8B10[index].thumbnail));
}
typedef struct PreviewConsumer {
    Context* source;
    IslePostCourseThumbnail thumbnail;
} PreviewConsumer;
static IsleGameResult pc_consume_preview(void* user, const IslePostCoursePreview* view) {
    PreviewConsumer* consumer = (PreviewConsumer*)user;
    if (!view) return isle_post_window_thumbnail_clear(consumer->source->common.window, consumer->thumbnail);
    return isle_post_window_thumbnail_write_photo(consumer->source->common.window,
        consumer->thumbnail, view->rgba5551, view->size, view->stride_bytes);
}
static int32_t pc_preview(Context* source, const PhotoData* photo, SObj* child) {
    size_t i;
    IsleSprite* sprite = pc_sprite(source, child);
    PreviewConsumer consumer = {source, {0}};
    for (i = 0; i < ARRAY_COUNT(source->D_802290A0_9D8B10); ++i)
        if (source->D_802290A0_9D8B10[i].sobj == child) {
            consumer.thumbnail = source->D_802290A0_9D8B10[i].thumbnail;
            break;
        }
    if (!consumer.thumbnail.serial) pc_check(source, ISLE_GAME_INVALID_STATE);
    float scale;
    pc_check(source, isle_photo_check_native_preview_scale(source, &scale));
    pc_check(source, isle_post_replay_preview(source->common.replay, photo,
        sprite->width * 2, sprite->height * 2, scale, pc_consume_preview, &consumer));
    return photo == NULL;
}
static Unk803A6C18* pc_score(Context* source, const PhotoData* photo) {
    int present = 0;
    pc_check(source, isle_post_replay_score(source->common.replay, photo, &source->score_scratch, &present));
    return present ? &source->score_scratch : NULL;
}
static UnkStruct800BEDF8* pc_input_at(Context* source,int32_t player,const char* caller) {
    pc_check(source,isle_photo_check_native_input(source,player,&source->input));
    if (!strcmp(caller,"isle_photo_check_func_80374F30_8486E0"))
        isle_post_course_ack_input(&source->common,source->input.currentButtons,source->input.pressedButtons);
    return &source->input;
}
#define pc_input(source,player) pc_input_at(source,player,__func__)

/* Every UI pointer is only a stable typed handle cell. Deleted cells are never
 * reused: the canonical Window checks its actual incarnation on every call. */
static UIElement* pc_element_create(Context* source, int x, int y, int w, int h, uint32_t flags) {
    UIElement* output;
    if (source->element_count == ARRAY_COUNT(source->elements)) pc_check(source, ISLE_GAME_EXHAUSTED);
    output = &source->elements[source->element_count++];
    pc_check(source, isle_post_window_element_create(source->common.window, x, y, w, h, flags, &output->handle));
    return output;
}
static UIFrame* pc_frame_create(Context* source) {
    pc_check(source, isle_post_window_frame_create(source->common.window, &source->frame.handle));
    return &source->frame;
}
static void pc_element_delete(Context* source, UIElement* element) {
    pc_check(source, isle_post_window_element_delete(source->common.window, element->handle));
}
static void pc_element_draw(Context* source, UIElement* element) {
    pc_check(source, isle_post_window_element_clear(source->common.window, element->handle));
}
static void pc_element_background(Context* source, UIElement* element) {
    pc_check(source, isle_post_window_element_background(source->common.window, element->handle));
}
static void pc_element_state(Context* source, UIElement* element, int state) {
    pc_check(source, isle_post_window_element_visible(source->common.window, element->handle, state == UI_NORMAL));
}
static void pc_element_font(Context* source, UIElement* element, int font) {
    pc_check(source, isle_post_window_element_font(source->common.window, element->handle, font));
}
static void pc_element_position(Context* source, UIElement* element, int x, int y) {
    pc_check(source, isle_post_window_element_text_position(source->common.window, element->handle, x, y));
}
static void pc_element_color(Context* source, UIElement* element, int foreground, int r, int g, int b, int a) {
    pc_check(source, isle_post_window_element_color(source->common.window, element->handle,
        foreground == UI_FOREGROUND, (IslePostCourseColor){(uint8_t)r,(uint8_t)g,(uint8_t)b,(uint8_t)a}));
}
static void pc_element_fill(Context* source, UIElement* element, int x1, int y1, int x2, int y2,
                            int r, int g, int b, int a) {
    pc_check(source, isle_post_window_element_fill(source->common.window, element->handle,
        x1,y1,x2,y2,(IslePostCourseColor){(uint8_t)r,(uint8_t)g,(uint8_t)b,(uint8_t)a}));
}
static void pc_element_scroll(Context* source, UIElement* element, int amount, unsigned direction,
                              int x1, int y1, int x2, int y2) {
    pc_check(source, isle_post_window_element_vertical_scroll(source->common.window, element->handle,
        amount,direction,x1,y1,x2,y2));
}
static int32_t pc_print(Context* source, UIElement* element, const char* text, int ascii) {
    IslePostCourseBytes bytes = {text, text ? strlen(text) : 0};
    pc_check(source, isle_post_window_print(source->common.window, element->handle, bytes, ascii, NULL));
    return 0;
}
static void pc_print_source(Context* source, UIElement* element, const char* text) {
    (void)pc_print(source, element, text, 0);
}
static void pc_print_ascii(Context* source, UIElement* element, const char* text) {
    (void)pc_print(source, element, text, 1);
}
static int32_t pc_formatted(Context* source, UIElement* element, const char* format, ...) {
    int count;
    va_list args;
    va_start(args, format);
    count = vsnprintf(source->formatted, sizeof(source->formatted), format, args);
    va_end(args);
    if (count < 0 || (size_t)count >= sizeof(source->formatted)) pc_check(source, ISLE_GAME_INVALID_STATE);
    pc_print_source(source, element, source->formatted);
    return count;
}
/* 8486E0.c44-62: the original s16[]/char* alias becomes explicit big-endian
 * source bytes; it never serializes host-endian uint16_t objects. */
static char* pc_number(Context* source, const char* format, int32_t value) {
    int count = snprintf(source->number_ascii, sizeof(source->number_ascii), format, value);
    int i;
    if (count < 0 || (size_t)count >= sizeof(source->number_ascii)) pc_check(source, ISLE_GAME_INVALID_STATE);
    for (i = 0; i < count; ++i) {
        unsigned char ch = (unsigned char)source->number_ascii[i];
        source->number_source[i*2] = (char)(ch >= '0' && ch <= '9' ? 0xa3 : 0xa1);
        source->number_source[i*2+1] = (char)(ch >= '0' && ch <= '9' ? ch-'0'+0xb0 : 0xa1);
    }
    source->number_source[count*2] = 0;
    return source->number_source;
}
static int32_t pc_width(Context* source, const char* text) {
    int output = 0;
    pc_check(source, isle_post_window_text_width(source->common.window,
        (IslePostCourseBytes){text, text ? strlen(text) : 0}, &output));
    return output;
}
static void pc_frame_color(Context* source, UIFrame* frame, ucolor* output) {
    pc_check(source, isle_post_window_frame_color(source->common.window, frame->handle, output));
}
static IsleGameResult pc_character(void* user, uint16_t character) {
    Context* source = (Context*)user;
    isle_photo_check_func_801DD2D4_98CD44(source, character);
    return source->common.status;
}
static void pc_character_register(Context* source, void (*function)(Context*,int32_t)) {
    IslePostCourseTextCallbacks callbacks = {source, pc_character, NULL, NULL};
    if (function != isle_photo_check_func_801DD2D4_98CD44) pc_check(source, ISLE_GAME_INVALID_ARGUMENT);
    pc_check(source, isle_post_window_text_callbacks(source->common.window, &callbacks));
}

static int32_t pc_selected_count(Context* source) {
    int32_t output = 0;
    pc_check(source, isle_application_photos_selected_count(source->common.photos, &output));
    return output;
}
static const IsleSelectedPhoto* pc_selected(Context* source, int32_t index) {
    const IsleSelectedPhoto* output = NULL;
    pc_check(source, isle_application_photos_selected(source->common.photos, index, &output));
    return output;
}
static const PhotoData* pc_report(Context* source, int32_t species) {
    IslePhotoRecord output = {0};
    pc_check(source, isle_application_photos_report(source->common.photos, species, &output));
    return output.photo;
}
static int32_t pc_report_present(Context* source, int32_t species) {
    int output = 0;
    pc_check(source, isle_application_photos_report_present(source->common.photos, species, &output));
    return output;
}
static const PhotoData* pc_temporary(Context* source, int32_t index) {
    IslePhotoRecord output = {0};
    pc_check(source, isle_application_photos_temporary(source->common.photos, index, &output));
    return output.photo;
}
static const PhotoData* pc_album(Context* source, int32_t index) {
    IslePhotoRecord output = {0};
    pc_check(source, isle_application_photos_album(source->common.photos, index, &output));
    return output.photo;
}
static int32_t pc_report_score(Context* source, int32_t species) {
    int32_t output = 0;
    pc_check(source, isle_application_photos_report_score(source->common.photos, species, &output));
    return output;
}
static void pc_set_report(Context* source, int32_t species, const PhotoData* photo) {
    pc_check(source, isle_application_photos_set_report(source->common.photos, species, photo));
}
static void pc_set_report_score(Context* source, int32_t species, int32_t score) {
    pc_check(source, isle_application_photos_set_report_score(source->common.photos, species, score));
}
static void pc_set_temporary(Context* source, int32_t index, const PhotoData* photo) {
    pc_check(source, isle_application_photos_set_temporary(source->common.photos, index, photo));
}
static void pc_set_album(Context* source, int32_t index, const PhotoData* photo, const uint16_t* comment) {
    pc_check(source, isle_application_photos_set_album(source->common.photos, index, photo, comment));
}
static int32_t pc_album_capacity(Context* source) {
    (void)source;
    return ISLE_PHOTO_ALBUM_CAPACITY;
}
static void pc_pending_item(Context* source, int32_t item, int32_t value) {
    pc_check(source, isle_application_photos_set_pending_item_grant(source->common.photos, item, value));
}

/* Native-only operations return before the C failure fence. */
#define PC_VOID0(name, call) \
    static void name(Context* source) { pc_check(source, (call)); }
#define PC_VOID(name, parameters, call) \
    static void name parameters { pc_check(source, (call)); }
PC_VOID0(pc_text_initialize, isle_post_window_text_initialize(source->common.window))
static void pc_input_initialize(Context* source) {
    pc_check(source,isle_photo_check_native_input_initialize(source));
    source->common.input_invalidate=isle_post_course_native_input_invalidate;
    pc_check(source,isle_post_course_source_install_input(&source->common));
}
PC_VOID0(pc_replay_initialize, isle_photo_check_native_replay_initialize(source))
PC_VOID0(pc_replay_cleanup, isle_photo_check_native_replay_cleanup(source))
PC_VOID0(pc_focus_create, isle_post_window_focus_create(source->common.window))
PC_VOID0(pc_focus_front, isle_post_window_focus_front(source->common.window))
PC_VOID0(pc_layout_initialize, isle_post_window_layout_initialize(source->common.window))
PC_VOID0(pc_layout_hide, isle_post_window_layout_hide(source->common.window))
PC_VOID0(pc_layout_wait, isle_post_window_layout_wait(source->common.window))
PC_VOID0(pc_buttons_clear, isle_post_window_buttons_clear(source->common.window))
PC_VOID(pc_text_delay, (Context* source, int delay), isle_post_window_text_delay(source->common.window,delay))
PC_VOID(pc_text_shadow, (Context* source, int offset), isle_post_window_text_shadow_offset(source->common.window,offset))
PC_VOID(pc_text_spacing, (Context* source, int x, int y), isle_post_window_text_spacing(source->common.window,x,y))
PC_VOID(pc_text_skip, (Context* source, int enabled), isle_post_window_text_skip(source->common.window,enabled))
PC_VOID(pc_focus_show, (Context* source, int enabled), isle_post_window_focus_visible(source->common.window,enabled))
PC_VOID(pc_focus_position, (Context* source, int x, int y), isle_post_window_focus_position(source->common.window,x,y))
PC_VOID(pc_focus_target, (Context* source, int x, int y), isle_post_window_focus_target_position(source->common.window,x,y))
PC_VOID(pc_focus_size, (Context* source, int x, int y), isle_post_window_focus_size(source->common.window,x,y))
PC_VOID(pc_layout_header, (Context* source, int flags), isle_post_window_layout_header_flags(source->common.window,flags))
PC_VOID(pc_layout_hide_header, (Context* source, int flags), isle_post_window_layout_hide_header(source->common.window,flags))
PC_VOID(pc_layout_panel, (Context* source, int enabled), isle_post_window_layout_panel(source->common.window,enabled))
PC_VOID(pc_frame_style, (Context* source, UIFrame* frame, int style), isle_post_window_frame_style(source->common.window,frame->handle,style))
PC_VOID(pc_frame_position, (Context* source, UIFrame* frame, int x, int y), isle_post_window_frame_position(source->common.window,frame->handle,x,y))
PC_VOID(pc_frame_size, (Context* source, UIFrame* frame, int x, int y), isle_post_window_frame_size(source->common.window,frame->handle,x,y))
PC_VOID(pc_frame_opacity, (Context* source, UIFrame* frame, int opacity), isle_post_window_frame_opacity(source->common.window,frame->handle,opacity))
PC_VOID(pc_frame_show, (Context* source, UIFrame* frame, int enabled), isle_post_window_frame_visible(source->common.window,frame->handle,enabled))
PC_VOID(pc_frame_background, (Context* source, UIFrame* frame, int enabled), isle_post_window_frame_background(source->common.window,frame->handle,enabled))
PC_VOID(pc_repeat, (Context* source, int a, int b), isle_photo_check_native_input_repeat(source,a,b))
PC_VOID(pc_input_mask, (Context* source, uint32_t mask), isle_photo_check_native_input_mask(source,mask))
PC_VOID(pc_aux_camera, (Context* source, int a, int b), isle_photo_check_native_aux_camera(source,a,b))
PC_VOID(pc_sound, (Context* source, int id), isle_photo_check_native_sound(source,id))
PC_VOID(pc_sound_params, (Context* source, int id, int volume, int pan, float pitch, int reverb), isle_photo_check_native_sound_params(source,id,volume,pan,pitch,reverb))
PC_VOID(pc_song, (Context* source, int channel, int id), isle_photo_check_native_song(source,channel,id))
PC_VOID(pc_volume, (Context* source, int channel, int volume), isle_photo_check_native_volume(source,channel,volume))
PC_VOID(pc_volume_smooth, (Context* source, int channel, int volume, int frames), isle_photo_check_native_volume_smooth(source,channel,volume,frames))
PC_VOID(pc_unlock_set, (Context* source, int value), isle_photo_check_native_set_unlock_progress(source,value))
PC_VOID(pc_notices_or, (Context* source, uint32_t flags), isle_photo_check_native_notices_or(source,flags))
PC_VOID(pc_scene_result, (Context* source, int result), isle_photo_check_native_scene_result(source,result))
#undef PC_VOID
#undef PC_VOID0
#define PC_INT0(name, leaf) \
    static int32_t name(Context* source) { int32_t out = 0; pc_check(source, leaf(source,&out)); return out; }
#define PC_INT(name, leaf, type) \
    static int32_t name(Context* source, type argument) { int32_t out = 0; pc_check(source, leaf(source,argument,&out)); return out; }
PC_INT0(pc_level_id,isle_photo_check_native_level_id)
PC_INT0(pc_selected_course,isle_photo_check_native_selected_course)
PC_INT0(pc_unlock_progress,isle_photo_check_native_unlock_progress)
PC_INT(pc_flag,isle_photo_check_native_player_flag,int32_t)
PC_INT(pc_notices_test,isle_photo_check_native_notices_test,uint32_t)
#undef PC_INT0
#undef PC_INT
static const char* pc_player_name(Context* source) {
    const char* out = NULL;
    pc_check(source,isle_photo_check_native_player_name(source,&out));
    return out;
}
static const char* pc_pokemon_name(Context* source, int32_t id) {
    const char* out = NULL;
    pc_check(source,isle_photo_check_native_pokemon_name(source,id,&out));
    return out;
}
static const char* pc_level_name(Context* source, int32_t id) {
    const char* out = NULL;
    pc_check(source,isle_photo_check_native_level_name(source,id,&out));
    return out;
}
static IslePostCourseWindow* pc_selector(Context* source) { return source->common.window; }
static void pc_camera_create(Context* source) {
    GObj* camera = NULL;
    IsleGameProcess* process = NULL;
    pc_check(source,isle_photo_check_native_camera(source,&camera));
    pc_check(source,isle_game_add_source_process(source->common.game,camera,
        isle_photo_check_func_801DCA14_98C484,ISLE_GAME_STACKFUL,0,&process));
}

/* Intentional Source-defect corrections are counted in the materializer:
 * - skip sign-only dead sp1FA arithmetic before the real total overwrites it;
 * - return zero on BOTH new/poor completion paths, independently of skip;
 *   this corrects the proven original missing assignment, not recovered residue;
 * - refresh main's evaluation handle after summary deletes/replaces it.
 * All dialogue/sounds/Report flags, 10->9 and 60->59 predecrement waits remain.
 * PANIC is not imported: the only caller fixes sp54=-26 throughout evaluation.
 * Rendering/setup SDK syntax is translated to typed actual native owner leaves.
 */
static int32_t pc_ack_at(Context*,UIElement*,int32_t,int32_t);
#include "photo_check_original.inc"
static int32_t pc_ack_at(Context* source,UIElement* element,int32_t sound,int32_t line) {
    int32_t result;
    pc_check(source,isle_post_course_ack_begin(&source->common,element->handle,line));
    result=isle_photo_check_func_80374F30_8486E0(source,element,sound);
    pc_check(source,isle_post_course_ack_end(&source->common));
    return result;
}

void isle_photo_check_source_initialize(IslePhotoCheckSource* source) {
    IslePostCourseSceneSource common = source->common;
    memset(source,0,sizeof(*source));
    source->common = common;
    source->common.scene = ISLE_POST_COURSE_PHOTO_CHECK;
    source->D_801F3E30_9A38A0 = 4;
    source->D_801F3E38_9A38A8 = 15;
    source->D_801F4180_9A3BF0 = -1;
    source->D_801F4194_9A3C04 = -1;
}
void isle_photo_check_source_init(IslePhotoCheckSource* source) {
    isle_photo_check_func_801DCBF4_98C664(source);
}
void isle_photo_check_source_init_call(IslePostCourseSceneSource* common, void* user) {
    (void)user;
    isle_photo_check_source_init((IslePhotoCheckSource*)common);
}
void isle_photo_check_func_801DCC74_98C6E4(void* user) {
    IslePhotoCheckEntry* entry = (IslePhotoCheckEntry*)user;
    IslePhotoCheckSource* source = entry->source;
    IsleContinuationDomain* domain = source->common.domain;
    void* owner = source->common.owner;
    int retired = 0;
    int32_t result = 0;
    IsleGameResult status;
    /* begin applies actual screen/near-clipping/setup; it cannot call Source.
     * source is deliberately not dereferenced again after this returning leaf. */
    status = isle_photo_check_native_entry_begin(source,entry->argument);
    if (status != ISLE_GAME_OK) {
        entry->status = status;
        (void)isle_continuation_fail(domain,"PhotoCheck15 entry setup");
        return;
    }
    while (!retired) {
        if (isle_continuation_wait(domain,1) != ISLE_CONT_OK) {
            entry->status = ISLE_GAME_INVALID_STATE;
            (void)isle_continuation_fail(domain,"PhotoCheck15 entry continuation");
            return;
        }
        status = isle_photo_check_native_entry_poll(owner,&retired,&result);
        if (status != ISLE_GAME_OK) {
            entry->status = status;
            (void)isle_continuation_fail(domain,"PhotoCheck15 entry retirement");
            return;
        }
    }
    entry->result = result; /* Actual D800AF3C0, not synthesized success. */
    entry->status = ISLE_GAME_OK;
}
void isle_photo_check_source_entry(void* user) {
    isle_photo_check_func_801DCC74_98C6E4(user);
}
