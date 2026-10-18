#include "post_course_replay_probe.h"
#include "engine_internal.h"
#include <string.h>

void isle_replay_probe_bind(IsleGameObject* object, IsleReplayProbe* probe) { object->userData = probe; }
void isle_replay_probe_heartbeat(IsleGameObject* object) { ++((IsleReplayProbe*)object->userData)->beats; }
void isle_replay_probe_joined(IsleGameObject* object, void* user) {
    (void)object;
    ++((IsleReplayProbe*)user)->joined;
}
static IsleGameResult leased(IsleReplayProbe* p) {
    IslePhotoRecord current = {0};
    if (memcmp(p->photo, &p->unchanged, sizeof(p->unchanged))) return ISLE_GAME_INVALID_STATE;
    if (isle_application_photos_temporary(p->photos, 0, &current) != ISLE_GAME_OK ||
        current.photo != p->photo) return ISLE_GAME_INVALID_STATE;
    /* A real canonical write must reject until actual result acknowledgment,
     * including after GPU retirement and during suspended callback borrows. */
    if (isle_application_photos_set_temporary(p->photos, 0, &p->unchanged) != ISLE_GAME_INVALID_STATE ||
        isle_application_photos_remove_temporary(p->photos, 0) != ISLE_GAME_INVALID_STATE ||
        memcmp(p->photo, &p->unchanged, sizeof(p->unchanged))) return ISLE_GAME_INVALID_STATE;
    ++p->lease_checks;
    return ISLE_GAME_OK;
}
static IsleGameResult score(IsleReplayProbe* p) {
    IslePostCourseReplayOperation operation = {0};
    IslePostCourseReplayStep step = isle_post_replay_score_begin(p->replay, p->photo, &operation);
    IsleGameResult result = ISLE_GAME_OK;
    for (;;) {
        if (step.signal != ISLE_POST_REPLAY_FAILED && step.signal != ISLE_POST_REPLAY_CANCELLED) {
            result = leased(p);
            if (result != ISLE_GAME_OK) break;
        }
        switch (step.signal) {
        case ISLE_POST_REPLAY_PENDING:
            result = isle_post_replay_progress(p->replay);
            break;
        case ISLE_POST_REPLAY_SOUND85:
            result = isle_post_replay_sound85(p->replay, operation, step.obligation);
            if (result == ISLE_GAME_OK) {
                ++p->sounds;
                result = isle_post_replay_acknowledge(p->replay, operation, step.obligation);
            }
            break;
        case ISLE_POST_REPLAY_WAIT60:
        case ISLE_POST_REPLAY_WAIT1: {
            unsigned before = p->beats;
            const int long_wait = step.signal == ISLE_POST_REPLAY_WAIT60;
            if (long_wait) { ++p->waits60; p->wait60_begin = before; p->stage = 60; }
            else { ++p->waits1; p->wait1_begin = before; p->stage = 1; }
            /* Only pure C frames remain on this suspended Source stack. */
            result = isle_game_source_wait(p->game, long_wait ? 60u : 1u);
            if (long_wait) p->wait60_end = p->beats;
            else p->wait1_end = p->beats;
            if (result == ISLE_GAME_OK && p->beats - before != (long_wait ? 60u : 1u))
                result = ISLE_GAME_INVALID_STATE;
            if (result == ISLE_GAME_OK) result = leased(p);
            if (result == ISLE_GAME_OK) result = isle_post_replay_acknowledge(p->replay, operation, step.obligation);
            p->stage = 0;
            break;
        }
        case ISLE_POST_REPLAY_RESULT:
            result = isle_post_replay_take_score(p->replay, operation, &p->score);
            if (result == ISLE_GAME_OK) p->present = 1;
            if (result == ISLE_GAME_OK) return isle_post_replay_operation_release(p->replay, operation);
            break;
        default:
            result = ISLE_GAME_INVALID_STATE;
            break;
        }
        if (result != ISLE_GAME_OK) break;
        step = isle_post_replay_poll(p->replay, operation);
    }
    (void)isle_post_replay_fail(p->replay, result);
    /* Preserve callback/operation backing until the real native fence. */
    for (;;) {
        step = isle_post_replay_poll(p->replay, operation);
        if (step.signal == ISLE_POST_REPLAY_FAILED || step.signal == ISLE_POST_REPLAY_CANCELLED) break;
        (void)isle_post_replay_progress(p->replay);
    }
    (void)isle_post_replay_operation_release(p->replay, operation);
    return result;
}
static IsleGameResult preview(void* user, const IslePostCoursePreview* image) {
    IsleReplayProbe* p = user;
    IsleGameResult result;
    ++p->preview_calls;
    if (!image || !image->rgba5551 || image->size > p->preview_capacity) return ISLE_GAME_INVALID_STATE;
    result = leased(p);
    if (result != ISLE_GAME_OK) return result;
    p->preview_size = image->size; p->preview_stride = image->stride_bytes;
    p->preview_width = image->width; p->preview_height = image->height;
    memcpy(p->preview_copy, image->rgba5551, image->size);
    result = isle_post_window_thumbnail_write_photo(p->window, p->thumbnail,
        image->rgba5551, image->size, image->stride_bytes);
    if (result != ISLE_GAME_OK) return result;
    /* This is a genuinely waiting pure-C consumer AFTER every native leaf has
     * returned. Its canonical record and preview borrow must survive too. */
    p->callback_begin = p->beats;
    result = isle_game_source_wait(p->game, 1);
    p->callback_end = p->beats;
    if (result != ISLE_GAME_OK || p->callback_end - p->callback_begin != 1 ||
        memcmp(p->preview_copy, image->rgba5551, image->size)) return ISLE_GAME_INVALID_STATE;
    return leased(p);
}
void isle_replay_probe_main(IsleGameObject* object) {
    IsleReplayProbe* p = object->userData;
    if (p->entered) return;
    p->entered = 1;
    if (p->mode == ISLE_REPLAY_PROBE_SCORE) p->result = score(p);
    else if (p->mode == ISLE_REPLAY_PROBE_PREVIEW)
        p->result = isle_post_replay_preview(p->replay, p->photo, 103, 77, 1, preview, p);
    else if (p->mode == ISLE_REPLAY_PROBE_DRIVER_SCORE) {
        /* Also exercise the production pure-C score driver, not only the
         * instrumented obligation-by-obligation consumer above. Empty terrain
         * has no sound60 branch; GPU pending must contribute zero OM updates. */
        p->wait1_begin = p->beats;
        p->result = isle_post_replay_score(p->replay, p->photo, &p->score, &p->present);
        p->wait1_end = p->beats;
        if (p->result == ISLE_GAME_OK) {
            if (p->wait1_end - p->wait1_begin != 1) p->result = ISLE_GAME_INVALID_STATE;
            else ++p->waits1;
        }
    }
    else {
        IslePostCourseReplayOperation operation = {0};
        IslePostCourseReplayStep step = isle_post_replay_preview_begin(p->replay, p->photo, 103, 77, 1, &operation);
        if (step.signal != ISLE_POST_REPLAY_PENDING) p->result = ISLE_GAME_INVALID_STATE;
        else {
            p->stage = -1;
            /* The actual service observes a submitted Replay GPU stage before
             * requesting Host stop; neither pending nor deadline is a fence. */
            while (isle_post_replay_status(p->replay) == ISLE_GAME_OK)
                (void)isle_post_replay_progress(p->replay);
            p->result = isle_post_replay_stop(p->replay);
        }
    }
    p->returned = 1;
}
