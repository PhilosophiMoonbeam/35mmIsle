#ifndef ISLE_POST_COURSE_WINDOW_SOURCE_H
#define ISLE_POST_COURSE_WINDOW_SOURCE_H
#include "35mmisle/engine.h"
#include "35mmisle/sprite.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct IslePostCourseWindow IslePostCourseWindow;
/* Handles identify both the scene context and an unreused incarnation. */
typedef struct IslePostCourseWindowHandle { const void* owner; uint64_t serial; } IslePostCourseWindowHandle;
typedef IslePostCourseWindowHandle IslePostCourseElement;
typedef IslePostCourseWindowHandle IslePostCourseFrame;
typedef IslePostCourseWindowHandle IslePostCourseOperation;
typedef IslePostCourseWindowHandle IslePostCourseThumbnail;
typedef IslePostCourseWindowHandle IslePostCourseButtonTable;
typedef struct IslePostCourseBytes { const char* data; size_t size; } IslePostCourseBytes;
typedef struct IslePostCourseColor { uint8_t r,g,b,a; } IslePostCourseColor;
typedef struct IslePostCourseButton { int id; IslePostCourseBytes text; } IslePostCourseButton;
typedef enum IslePostCourseStepStatus {
    ISLE_POST_WINDOW_MORE, ISLE_POST_WINDOW_EVENT, ISLE_POST_WINDOW_WAIT,
    ISLE_POST_WINDOW_DONE, ISLE_POST_WINDOW_ERROR
} IslePostCourseStepStatus;
typedef enum IslePostCourseStepError {
    ISLE_POST_WINDOW_NO_ERROR, ISLE_POST_WINDOW_INVALID_OPERATION,
    ISLE_POST_WINDOW_BUSY, ISLE_POST_WINDOW_CANCELLED, ISLE_POST_WINDOW_INVALID_TEXT,
    ISLE_POST_WINDOW_INVALID_SURFACE, ISLE_POST_WINDOW_SURFACE_BOUNDARY,
    ISLE_POST_WINDOW_MISSING_BUTTON, ISLE_POST_WINDOW_NATIVE_FAILURE
} IslePostCourseStepError;
typedef enum IslePostCourseEventKind {
    ISLE_POST_WINDOW_NO_EVENT, ISLE_POST_WINDOW_CHARACTER,
    ISLE_POST_WINDOW_BUTTON, ISLE_POST_WINDOW_CHANGED
} IslePostCourseEventKind;
typedef struct IslePostCourseWindowEvent {
    IslePostCourseEventKind kind;
    uint16_t character;
    int x,y,scale;
    char button;
    size_t offset,size;
} IslePostCourseWindowEvent;
typedef struct IslePostCourseWindowStep {
    IslePostCourseStepStatus status;
    IslePostCourseStepError error;
    uint32_t updates;
    IslePostCourseWindowEvent event;
} IslePostCourseWindowStep;
/* C callbacks run only after a returning native leaf. They may wait in actual
 * Source C; return failure instead of acknowledging a failed callback. The
 * caller owns callback backing and live text until completion/cancel. */
typedef struct IslePostCourseTextCallbacks {
    void* user;
    IsleGameResult (*character)(void*,uint16_t);
    IsleGameResult (*button)(void*,int,int,char,int);
    IsleGameResult (*changed)(void*,size_t,size_t);
} IslePostCourseTextCallbacks;

IsleGameResult isle_post_window_status(IslePostCourseWindow*);
IsleGame* isle_post_window_game(IslePostCourseWindow*);
IsleGameResult isle_post_window_fail(IslePostCourseWindow*,IsleGameResult);
IsleGameResult isle_post_window_text_initialize(IslePostCourseWindow*);
IsleGameResult isle_post_window_text_font(IslePostCourseWindow*,int);
IsleGameResult isle_post_window_text_spacing(IslePostCourseWindow*,int,int);
IsleGameResult isle_post_window_text_shadow_offset(IslePostCourseWindow*,int);
IsleGameResult isle_post_window_text_shadow_color(IslePostCourseWindow*,IslePostCourseColor);
/* NULL per-print callbacks selects this scene registration. Its character
 * callback is re-read at each event, matching func_8036EB8C_84233C; the original
 * Source callback itself owns character sounds. Explicit per-print callbacks
 * are borrowed for that operation instead. Neither setter invokes Source. */
IsleGameResult isle_post_window_text_callbacks(IslePostCourseWindow*,const IslePostCourseTextCallbacks*);
IsleGameResult isle_post_window_text_get_callbacks(IslePostCourseWindow*,IslePostCourseTextCallbacks*);
IsleGameResult isle_post_window_text_delay(IslePostCourseWindow*,int);
IsleGameResult isle_post_window_text_proportional(IslePostCourseWindow*,int);
IsleGameResult isle_post_window_text_skip(IslePostCourseWindow*,int);
IsleGameResult isle_post_window_text_input(IslePostCourseWindow*,uint32_t);
IsleGameResult isle_post_window_text_width(IslePostCourseWindow*,IslePostCourseBytes,int*);
IsleGameResult isle_post_window_element_create(IslePostCourseWindow*,int,int,int,int,uint32_t,IslePostCourseElement*);
IsleGameResult isle_post_window_element_delete(IslePostCourseWindow*,IslePostCourseElement);
IsleGameResult isle_post_window_element_position(IslePostCourseWindow*,IslePostCourseElement,int,int);
IsleGameResult isle_post_window_element_alt_position(IslePostCourseWindow*,IslePostCourseElement,int,int);
IsleGameResult isle_post_window_element_visible(IslePostCourseWindow*,IslePostCourseElement,int);
IsleGameResult isle_post_window_element_scale(IslePostCourseWindow*,IslePostCourseElement,float,float);
IsleGameResult isle_post_window_element_sprite_color(IslePostCourseWindow*,IslePostCourseElement,IslePostCourseColor);
IsleGameResult isle_post_window_element_color(IslePostCourseWindow*,IslePostCourseElement,int,IslePostCourseColor);
IsleGameResult isle_post_window_element_font(IslePostCourseWindow*,IslePostCourseElement,int);
IsleGameResult isle_post_window_element_text_position(IslePostCourseWindow*,IslePostCourseElement,int,int);
IsleGameResult isle_post_window_element_fill(IslePostCourseWindow*,IslePostCourseElement,int,int,int,int,IslePostCourseColor);
IsleGameResult isle_post_window_element_fill_default(IslePostCourseWindow*,IslePostCourseElement,int,int,int,int);
IsleGameResult isle_post_window_element_clear(IslePostCourseWindow*,IslePostCourseElement);
IsleGameResult isle_post_window_element_background(IslePostCourseWindow*,IslePostCourseElement);
IsleGameResult isle_post_window_element_scroll(IslePostCourseWindow*,IslePostCourseElement,int,unsigned);
IsleGameResult isle_post_window_element_vertical_scroll(IslePostCourseWindow*,IslePostCourseElement,int,unsigned,int,int,int,int);
IsleGameResult isle_post_window_element_image(IslePostCourseWindow*,IslePostCourseElement,int,int,int,int,const uint8_t*,size_t);
IsleGameResult isle_post_window_element_swap(IslePostCourseWindow*,IslePostCourseElement);
IsleGameResult isle_post_window_element_alt_sprite(IslePostCourseWindow*,IslePostCourseElement,uint32_t);
IsleGameResult isle_post_window_element_show_alt(IslePostCourseWindow*,IslePostCourseElement,int);
IsleGameResult isle_post_window_element_move_head(IslePostCourseWindow*,IslePostCourseElement);
IsleGameResult isle_post_window_element_move_tail(IslePostCourseWindow*,IslePostCourseElement);
IsleGameResult isle_post_window_element_attachment(IslePostCourseWindow*,IslePostCourseElement,IsleGameObject**,IsleGameSprite**);
/* Mutable views and traversal are actual Services records, never snapshots.
 * Only objects explicitly adopted by this context or its own Element objects
 * are admitted. Adoption does not transfer object retirement ownership. */
IsleGameResult isle_post_window_adopt_object(IslePostCourseWindow*,IsleGameObject*);
IsleGameResult isle_post_window_release_object(IslePostCourseWindow*,IsleGameObject*);
IsleGameResult isle_post_window_sprite_root(IslePostCourseWindow*,IsleGameObject*,IsleGameSprite**);
IsleGameResult isle_post_window_sprite_view(IslePostCourseWindow*,IsleGameSprite*,IsleSprite**);
IsleGameResult isle_post_window_sprite_links(IslePostCourseWindow*,IsleGameSprite*,IsleGameObject**,IsleGameSprite**,IsleGameSprite**,IsleGameSprite**,IsleGameSprite**);
IsleGameResult isle_post_window_sprite_sibling(IslePostCourseWindow*,IsleGameSprite*,uint32_t,IsleGameSprite**);
IsleGameResult isle_post_window_sprite_child(IslePostCourseWindow*,IsleGameSprite*,uint32_t,IsleGameSprite**);
IsleGameResult isle_post_window_sprite_rebind(IslePostCourseWindow*,IsleGameSprite*,uint32_t);
/* Concrete native equivalents of func_80371C68_845418/func_80371D14_8454C4.
 * The returning create leaf installs no Source process; the C driver installs
 * the exact typed callback after the leaf returns. Scene-owned objects join
 * before context destruction. Adopted objects remain caller-retired. */
IsleGameResult isle_post_window_sprite_object_create(IslePostCourseWindow*,int,uint32_t,IsleGameObject**,IsleGameSprite**);
IsleGameResult isle_post_window_sprite_object_delete(IslePostCourseWindow*,IsleGameObject*);
IsleGameResult isle_post_window_sprite_object(IslePostCourseWindow*,int,uint32_t,IsleGameSourceCallback,int,IsleGameObject**,IsleGameSprite**);
/* Actual SObj::unk_54, independently checked for this context's native owner. */
IsleGameResult isle_post_window_sprite_aux_get(IslePostCourseWindow*,IsleGameSprite*,int32_t*);
IsleGameResult isle_post_window_sprite_aux_set(IslePostCourseWindow*,IsleGameSprite*,int32_t);
/* Existing Photo-image child only: independent copies of the real RGBA16
 * prototype, including every bitmap piece and its actual pixel-buffer offset,
 * registered in Producer with a real generation. Admitted prototypes are
 * CameraCheck 0x80201520/0x80208900 and PhotoCheck 0x801F3B10. No Element or
 * immutable-bank mutation. All calls require actual graphics retirement.
 * write_photo accepts ONLY the completed source Photo replay's canonical BE
 * RGBA16 image at exactly twice the target dimensions; stride is in bytes.
 * It performs original AVG_PIXEL, using first-bitmap width_img/actualHeight
 * and resetting row/XOR-4 parity whenever Source advances a bitmap.
 * clear is the original explicit NULL/failed-Photo branch, not a fallback
 * provider. Neither call executes Photo/Frame/GTL or awards update credit.
 * release restores the original immutable bitmap on a still-live child while
 * preserving every scalar and hierarchy link, then drops its independent
 * storage. A retired child needs no restoration. Borrowed graphics reject
 * before mutation/free; failed cleanup keeps storage alive for retry. */
IsleGameResult isle_post_window_thumbnail_create(IslePostCourseWindow*,IsleGameSprite*,uint32_t,IslePostCourseThumbnail*);
IsleGameResult isle_post_window_thumbnail_write_photo(IslePostCourseWindow*,IslePostCourseThumbnail,const uint8_t*,size_t,size_t);
IsleGameResult isle_post_window_thumbnail_clear(IslePostCourseWindow*,IslePostCourseThumbnail);
IsleGameResult isle_post_window_thumbnail_release(IslePostCourseWindow*,IslePostCourseThumbnail);

IsleGameResult isle_post_window_frame_create(IslePostCourseWindow*,IslePostCourseFrame*);
IsleGameResult isle_post_window_frame_delete(IslePostCourseWindow*,IslePostCourseFrame);
IsleGameResult isle_post_window_frame_style(IslePostCourseWindow*,IslePostCourseFrame,int);
IsleGameResult isle_post_window_frame_position(IslePostCourseWindow*,IslePostCourseFrame,int,int);
IsleGameResult isle_post_window_frame_size(IslePostCourseWindow*,IslePostCourseFrame,int,int);
IsleGameResult isle_post_window_frame_visible(IslePostCourseWindow*,IslePostCourseFrame,int);
IsleGameResult isle_post_window_frame_background(IslePostCourseWindow*,IslePostCourseFrame,int);
IsleGameResult isle_post_window_frame_opacity(IslePostCourseWindow*,IslePostCourseFrame,int);
IsleGameResult isle_post_window_frame_color(IslePostCourseWindow*,IslePostCourseFrame,IslePostCourseColor*);
IsleGameResult isle_post_window_focus_create(IslePostCourseWindow*);
IsleGameResult isle_post_window_focus_delete(IslePostCourseWindow*);
IsleGameResult isle_post_window_focus_visible(IslePostCourseWindow*,int);
IsleGameResult isle_post_window_focus_front(IslePostCourseWindow*);
IsleGameResult isle_post_window_focus_position(IslePostCourseWindow*,int,int);
IsleGameResult isle_post_window_focus_target_position(IslePostCourseWindow*,int,int);
IsleGameResult isle_post_window_focus_size(IslePostCourseWindow*,int,int);
IsleGameResult isle_post_window_focus_target_size(IslePostCourseWindow*,int,int);
IsleGameResult isle_post_window_focus_stop(IslePostCourseWindow*,int);
IsleGameResult isle_post_window_layout_initialize(IslePostCourseWindow*);
IsleGameResult isle_post_window_layout_instant(IslePostCourseWindow*,int);
IsleGameResult isle_post_window_layout_direction(IslePostCourseWindow*);
IsleGameResult isle_post_window_layout_speed(IslePostCourseWindow*);
IsleGameResult isle_post_window_layout_disable(IslePostCourseWindow*,int,int);
IsleGameResult isle_post_window_layout_disable_except(IslePostCourseWindow*,int,int);
IsleGameResult isle_post_window_layout_show_button(IslePostCourseWindow*,int,int);
IsleGameResult isle_post_window_layout_scale(IslePostCourseWindow*,float,int);
IsleGameResult isle_post_window_layout_header_flags(IslePostCourseWindow*,int);
IsleGameResult isle_post_window_layout_show_header(IslePostCourseWindow*,int);
IsleGameResult isle_post_window_layout_hide_header(IslePostCourseWindow*,int);
IsleGameResult isle_post_window_layout_panel(IslePostCourseWindow*,int);
/* Read the canonical live Layout slot, including partially-built replacements.
 * A selector retained before CreateButtons retains only this scene context,
 * never a copied slot array. Valid indices are those of Layout::buttons(). */
IsleGameResult isle_post_window_layout_slot_id(IslePostCourseWindow*,size_t,int*);
/* Source button entries/text stay live until scene retirement; Layout retains
 * even a cancelled partially-created prefix. Resource tables borrow real ROM
 * decoding. Caller tables have stable converted descriptors, no text copies.
 * IDs and text extents stay fixed across a live Layout operation; bytes in the
 * same text backing remain live. Re-borrowing an existing table refreshes its
 * descriptors in place without reallocating. Creation refreshes that table,
 * and text lookup reads its current caller entries. */
IsleGameResult isle_post_window_table_borrow(IslePostCourseWindow*,const IslePostCourseButton*,size_t,IslePostCourseButtonTable*);
IsleGameResult isle_post_window_table_resource(IslePostCourseWindow*,uint32_t,IslePostCourseButtonTable*);
IsleGameResult isle_post_window_table_text(IslePostCourseWindow*,IslePostCourseButtonTable,int,IslePostCourseBytes*);
IsleGameResult isle_post_window_buttons_create(IslePostCourseWindow*,int,int,char,int);
IsleGameResult isle_post_window_buttons_clear(IslePostCourseWindow*);
IsleGameResult isle_post_window_buttons_erase(IslePostCourseWindow*,int,int,int,int);
IsleGameResult isle_post_window_buttons_show(IslePostCourseWindow*,int,int,int,int,int);
IsleGameResult isle_post_window_buttons_interval(IslePostCourseWindow*,char,float);

/* Returning native leaves: begin/step/cancel NEVER call Source or wait.
 * resume acknowledges precisely the last returned event/wait. A pending result
 * is sticky until acknowledged. Terminal operations remain queryable until
 * release; release rejects nonterminal operations. */
IslePostCourseWindowStep isle_post_window_print_begin(IslePostCourseWindow*,IslePostCourseElement,IslePostCourseBytes,int,const IslePostCourseTextCallbacks*,IslePostCourseOperation*);
IslePostCourseWindowStep isle_post_window_fade_in_begin(IslePostCourseWindow*,IslePostCourseFrame,int,IslePostCourseOperation*);
IslePostCourseWindowStep isle_post_window_fade_out_begin(IslePostCourseWindow*,IslePostCourseFrame,IslePostCourseOperation*);
IslePostCourseWindowStep isle_post_window_layout_wait_begin(IslePostCourseWindow*,IslePostCourseOperation*);
IslePostCourseWindowStep isle_post_window_layout_hide_begin(IslePostCourseWindow*,IslePostCourseOperation*);
IslePostCourseWindowStep isle_post_window_layout_create_begin(IslePostCourseWindow*,IslePostCourseButtonTable,IslePostCourseOperation*);
IslePostCourseWindowStep isle_post_window_step(IslePostCourseWindow*,IslePostCourseOperation,int);
IslePostCourseWindowStep isle_post_window_cancel(IslePostCourseWindow*,IslePostCourseOperation);
IsleGameResult isle_post_window_operation_release(IslePostCourseWindow*,IslePostCourseOperation);
IsleGameResult isle_post_window_cancel_all(IslePostCourseWindow*);
/* These drivers and stop are pure C. Call only from the genuine Source process;
 * no owning C++ automatic frame may enclose them. Stop cancels claims before
 * entering the existing source-only update fence; it creates no OM/GTL credit. */
IsleGameResult isle_post_window_print(IslePostCourseWindow*,IslePostCourseElement,IslePostCourseBytes,int,const IslePostCourseTextCallbacks*);
IsleGameResult isle_post_window_fade_in(IslePostCourseWindow*,IslePostCourseFrame,int);
IsleGameResult isle_post_window_fade_out(IslePostCourseWindow*,IslePostCourseFrame);
IsleGameResult isle_post_window_layout_wait(IslePostCourseWindow*);
IsleGameResult isle_post_window_layout_hide(IslePostCourseWindow*);
IsleGameResult isle_post_window_layout_create(IslePostCourseWindow*,IslePostCourseButtonTable);
IsleGameResult isle_post_window_stop(IslePostCourseWindow*);
#ifdef __cplusplus
}
#endif
#endif
