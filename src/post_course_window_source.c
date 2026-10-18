#include "post_course_window_source.h"

/* Authority: pinned CameraCheck 87A330.c/87D1A0.c and PhotoCheck
 * 98C330.c/993C50.c UI call sites; the native Window returning operations own
 * glyph/escape, fade and layout phases. Only this C stack invokes the original
 * caller's Source callbacks (including its character sounds) and actual waits.
 * No native leaf remains on this stack during either operation. */
static IsleGameResult finish_operation(IslePostCourseWindow* context,
                                      IslePostCourseOperation operation,
                                      IsleGameResult result) {
    IsleGameResult released;
    if (operation.serial) {
        if (result != ISLE_GAME_OK) (void)isle_post_window_cancel(context, operation);
        released = isle_post_window_operation_release(context, operation);
        if (result == ISLE_GAME_OK) result = released;
    }
    if (result != ISLE_GAME_OK) return isle_post_window_fail(context, result);
    return isle_post_window_status(context);
}

static IsleGameResult drive(IslePostCourseWindow* context,
                            IslePostCourseOperation operation,
                            IslePostCourseWindowStep step,
                            const IslePostCourseTextCallbacks* callbacks) {
    IsleGameResult result;
    IslePostCourseTextCallbacks source = {0};
    if (callbacks) source = *callbacks;
    for (;;) {
        switch (step.status) {
        case ISLE_POST_WINDOW_MORE:
            step = isle_post_window_step(context, operation, 0);
            break;
        case ISLE_POST_WINDOW_WAIT:
            result = isle_game_source_wait(isle_post_window_game(context), step.updates);
            if (result != ISLE_GAME_OK) return finish_operation(context, operation, result);
            step = isle_post_window_step(context, operation, 1);
            break;
        case ISLE_POST_WINDOW_EVENT:
            if (!callbacks) {
                result = isle_post_window_text_get_callbacks(context, &source);
                if (result != ISLE_GAME_OK) return finish_operation(context, operation, result);
            }
            result = ISLE_GAME_OK;
            switch (step.event.kind) {
            case ISLE_POST_WINDOW_CHARACTER:
                if (source.character) result = source.character(source.user, step.event.character);
                break;
            case ISLE_POST_WINDOW_BUTTON:
                if (source.button) result = source.button(source.user, step.event.x, step.event.y,
                                                         step.event.button, step.event.scale);
                else result = isle_post_window_buttons_create(context, step.event.x, step.event.y,
                                                              step.event.button, step.event.scale);
                break;
            case ISLE_POST_WINDOW_CHANGED:
                if (source.changed) result = source.changed(source.user, step.event.offset, step.event.size);
                break;
            case ISLE_POST_WINDOW_NO_EVENT:
                result = ISLE_GAME_INVALID_STATE;
                break;
            }
            if (result != ISLE_GAME_OK) return finish_operation(context, operation, result);
            step = isle_post_window_step(context, operation, 1);
            break;
        case ISLE_POST_WINDOW_DONE:
            return finish_operation(context, operation, ISLE_GAME_OK);
        case ISLE_POST_WINDOW_ERROR:
            result = isle_post_window_status(context);
            if (result == ISLE_GAME_OK) result = ISLE_GAME_INVALID_STATE;
            return finish_operation(context, operation, result);
        default:
            return finish_operation(context, operation, ISLE_GAME_INVALID_STATE);
        }
    }
}

IsleGameResult isle_post_window_print(IslePostCourseWindow* context, IslePostCourseElement element,
                                     IslePostCourseBytes text, int ascii,
                                     const IslePostCourseTextCallbacks* callbacks) {
    IslePostCourseOperation operation = {0};
    IslePostCourseWindowStep step;
    step = isle_post_window_print_begin(context, element, text, ascii, callbacks, &operation);
    return drive(context, operation, step, callbacks);
}
IsleGameResult isle_post_window_fade_in(IslePostCourseWindow* context, IslePostCourseFrame frame, int style) {
    IslePostCourseOperation operation = {0};
    IslePostCourseWindowStep step = isle_post_window_fade_in_begin(context, frame, style, &operation);
    return drive(context, operation, step, NULL);
}
IsleGameResult isle_post_window_fade_out(IslePostCourseWindow* context, IslePostCourseFrame frame) {
    IslePostCourseOperation operation = {0};
    IslePostCourseWindowStep step = isle_post_window_fade_out_begin(context, frame, &operation);
    return drive(context, operation, step, NULL);
}
IsleGameResult isle_post_window_layout_wait(IslePostCourseWindow* context) {
    IslePostCourseOperation operation = {0};
    IslePostCourseWindowStep step = isle_post_window_layout_wait_begin(context, &operation);
    return drive(context, operation, step, NULL);
}
IsleGameResult isle_post_window_layout_hide(IslePostCourseWindow* context) {
    IslePostCourseOperation operation = {0};
    IslePostCourseWindowStep step = isle_post_window_layout_hide_begin(context, &operation);
    return drive(context, operation, step, NULL);
}
IsleGameResult isle_post_window_layout_create(IslePostCourseWindow* context, IslePostCourseButtonTable table) {
    IslePostCourseOperation operation = {0};
    IslePostCourseWindowStep step = isle_post_window_layout_create_begin(context, table, &operation);
    return drive(context, operation, step, NULL);
}
IsleGameResult isle_post_window_stop(IslePostCourseWindow* context) {
    IsleGameResult result = isle_post_window_cancel_all(context);
    if (result != ISLE_GAME_OK) return result;
    result = isle_game_stop_update(isle_post_window_game(context));
    return isle_post_window_fail(context, result);
}

IsleGameResult isle_post_window_sprite_object(IslePostCourseWindow* context, int link, uint32_t identity,
                                             IsleGameSourceCallback callback, int synchronous,
                                             IsleGameObject** object, IsleGameSprite** sprite) {
    IsleGameProcess* process = NULL;
    IsleGameResult result = isle_post_window_sprite_object_create(context, link, identity, object, sprite);
    if (result != ISLE_GAME_OK || !callback) return result;
    result = isle_game_add_source_process(isle_post_window_game(context), *object, callback,
                                         synchronous ? ISLE_GAME_SYNCHRONOUS : ISLE_GAME_STACKFUL, 0, &process);
    if (result != ISLE_GAME_OK) {
        (void)isle_post_window_sprite_object_delete(context, *object);
        *object = NULL;
        *sprite = NULL;
        return isle_post_window_fail(context, result);
    }
    return isle_post_window_status(context);
}
