#include "post_course_replay_source.h"

/* Pinned app_render/score.c320-326 owns sound85, wait60 and final wait1.
 * Native pending has NO original Source-update meaning. Host/native servicing
 * below returns normally; ordinary display/World runs only in actual waits. */
static IsleGameResult retire_all(IslePostCourseReplay* context) {
    int retired = 0;
    IsleGameResult result;
    do {
        result = isle_post_replay_retire(context, &retired);
        if (!retired) (void)isle_post_replay_progress(context);
    } while (!retired);
    return result;
}

static IsleGameResult finish(IslePostCourseReplay* context,
                             IslePostCourseReplayOperation operation,
                             IsleGameResult result) {
    IslePostCourseReplayStep step;
    if (result != ISLE_GAME_OK) (void)isle_post_replay_fail(context, result);
    if (!operation.serial) return retire_all(context);
    if (isle_post_replay_status(context) != ISLE_GAME_OK) {
        /* Failure is not a fence. Poll drains the retained real GPU/readback
         * and native owners, even when Host events/deadline have stopped. */
        for (;;) {
            step = isle_post_replay_poll(context, operation);
            if (step.signal == ISLE_POST_REPLAY_CANCELLED || step.signal == ISLE_POST_REPLAY_FAILED) break;
            (void)isle_post_replay_progress(context);
        }
    }
    return isle_post_replay_operation_release(context, operation);
}

static IsleGameResult drive(IslePostCourseReplay* context,
                            IslePostCourseReplayOperation operation,
                            IslePostCourseReplayStep step,
                            IslePostCourseScoredPhoto* score, int* present,
                            IslePostCoursePreviewConsumer consume, void* user) {
    IsleGameResult result;
    IslePostCoursePreview preview;
    for (;;) {
        switch (step.signal) {
        case ISLE_POST_REPLAY_PENDING:
            (void)isle_post_replay_progress(context);
            step = isle_post_replay_poll(context, operation);
            break;
        case ISLE_POST_REPLAY_SOUND85:
            result = isle_post_replay_sound85(context, operation, step.obligation);
            if (result != ISLE_GAME_OK) return finish(context, operation, result);
            result = isle_post_replay_acknowledge(context, operation, step.obligation);
            if (result != ISLE_GAME_OK) return finish(context, operation, result);
            step = isle_post_replay_poll(context, operation);
            break;
        case ISLE_POST_REPLAY_WAIT60:
        case ISLE_POST_REPLAY_WAIT1:
            /* All C++ leaves have returned before this genuine Source wait. */
            result = isle_game_source_wait(isle_post_replay_game(context),
                step.signal == ISLE_POST_REPLAY_WAIT60 ? 60u : 1u);
            if (result != ISLE_GAME_OK) return finish(context, operation, result);
            result = isle_post_replay_acknowledge(context, operation, step.obligation);
            if (result != ISLE_GAME_OK) return finish(context, operation, result);
            step = isle_post_replay_poll(context, operation);
            break;
        case ISLE_POST_REPLAY_RESULT:
            if (score) {
                result = isle_post_replay_take_score(context, operation, score);
                if (result == ISLE_GAME_OK) *present = 1;
            } else {
                result = isle_post_replay_preview_result(context, operation, &preview);
                if (result == ISLE_GAME_OK) result = consume(user, &preview);
                if (result == ISLE_GAME_OK) result = isle_post_replay_acknowledge_preview(context, operation);
            }
            return finish(context, operation, result);
        case ISLE_POST_REPLAY_ABSENT:
            /* 847B60.c59-64/88-90 return NULL; thumbnail caller's explicit
             * NULL branch clears its actual raster, never substitutes pixels. */
            result = consume ? consume(user, NULL) : ISLE_GAME_OK;
            return finish(context, operation, result);
        case ISLE_POST_REPLAY_CANCELLED:
        case ISLE_POST_REPLAY_FAILED:
            result = isle_post_replay_status(context);
            if (result == ISLE_GAME_OK) result = ISLE_GAME_INVALID_STATE;
            return finish(context, operation, result);
        default:
            return finish(context, operation, ISLE_GAME_INVALID_STATE);
        }
    }
}

IsleGameResult isle_post_replay_score(IslePostCourseReplay* context,
    const IslePlayerPhotoData* photo, IslePostCourseScoredPhoto* output, int* present) {
    IslePostCourseReplayOperation operation = {0};
    IslePostCourseReplayStep step;
    if (present) *present = 0;
    if (!context) return ISLE_GAME_INVALID_ARGUMENT;
    if (!output || !present) return finish(context, operation, ISLE_GAME_INVALID_ARGUMENT);
    step = isle_post_replay_score_begin(context, photo, &operation);
    return drive(context, operation, step, output, present, NULL, NULL);
}

IsleGameResult isle_post_replay_preview(IslePostCourseReplay* context,
    const IslePlayerPhotoData* photo, int32_t width, int32_t height, float scale,
    IslePostCoursePreviewConsumer consume, void* user) {
    IslePostCourseReplayOperation operation = {0};
    IslePostCourseReplayStep step;
    if (!context) return ISLE_GAME_INVALID_ARGUMENT;
    if (!consume) return finish(context, operation, ISLE_GAME_INVALID_ARGUMENT);
    step = isle_post_replay_preview_begin(context, photo, width, height, scale, &operation);
    return drive(context, operation, step, NULL, NULL, consume, user);
}

IsleGameResult isle_post_replay_preview_working(IslePostCourseReplay* context,
    IslePhotoWorking* working, int32_t width, int32_t height, float scale,
    IslePostCoursePreviewConsumer consume, void* user) {
    IslePostCourseReplayOperation operation = {0};
    IslePostCourseReplayStep step;
    if (!context) return ISLE_GAME_INVALID_ARGUMENT;
    if (!consume) return finish(context, operation, ISLE_GAME_INVALID_ARGUMENT);
    step = isle_post_replay_preview_working_begin(context, working, width, height, scale, &operation);
    return drive(context, operation, step, NULL, NULL, consume, user);
}

IsleGameResult isle_post_replay_stop(IslePostCourseReplay* context) {
    IsleGameResult result;
    if (!context) return ISLE_GAME_INVALID_ARGUMENT;
    (void)isle_post_replay_cancel(context);
    result = retire_all(context);
    if (result != ISLE_GAME_STOPPED) return result;
    /* The real terminal precedes this C-only nonreturning stop fence. */
    result = isle_game_stop_update(isle_post_replay_game(context));
    return isle_post_replay_fail(context, result);
}
