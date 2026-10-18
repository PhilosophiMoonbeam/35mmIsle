#ifndef ISLE_POST_COURSE_REPLAY_SOURCE_H
#define ISLE_POST_COURSE_REPLAY_SOURCE_H
#include "application_photos_source.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct IslePostCourseReplay IslePostCourseReplay;
typedef struct IslePostCourseReplayOperation { const void* owner; uint64_t serial; } IslePostCourseReplayOperation;
/* Semantic Source ScoreData, not its guest layout. No score is cached in PhotoData. */
typedef struct IslePostCourseScore {
    int32_t totalScore;
    uint16_t samePkmnBonus;
    uint8_t samePkmnNumber, isWellFramed, poseID;
    int16_t pokemonInFocus;
    uint16_t proximityScore, completenessScore, posePts;
    int16_t specialBonus;
    uint8_t specialID;
} IslePostCourseScore;
/* Original 847B60.c59-71 scratch projection. One operation result, never a
 * second canonical saved-photo store. The original photo flags are applied
 * only after Replay has acknowledged the unchanged canonical record. */
typedef struct IslePostCourseScoredPhoto {
    IslePlayerPhotoData photo;
    IslePostCourseScore score;
} IslePostCourseScoredPhoto;
typedef enum IslePostCourseReplaySignal {
    ISLE_POST_REPLAY_PENDING, ISLE_POST_REPLAY_SOUND85,
    ISLE_POST_REPLAY_WAIT60, ISLE_POST_REPLAY_WAIT1,
    ISLE_POST_REPLAY_RESULT, ISLE_POST_REPLAY_ABSENT,
    ISLE_POST_REPLAY_CANCELLED, ISLE_POST_REPLAY_FAILED
} IslePostCourseReplaySignal;
typedef struct IslePostCourseReplayStep {
    IslePostCourseReplaySignal signal;
    uint64_t obligation;
} IslePostCourseReplayStep;
/* Completed native big-endian RGBA5551, including the real backing stride.
 * The borrow expires at acknowledge_preview/cancel/retirement. Width/height
 * are the Source-clamped viewport, not a resampled screenshot. */
typedef struct IslePostCoursePreview {
    const uint8_t* rgba5551;
    size_t size, stride_bytes;
    uint32_t width, height;
} IslePostCoursePreview;
/* Called by pure C AFTER native leaves return, before acknowledgment. NULL
 * view is ONLY the genuine NULL/missing Photo input (original clear branch).
 * The real raster owner consumes/copies pixels before returning success. */
typedef IsleGameResult (*IslePostCoursePreviewConsumer)(void*, const IslePostCoursePreview*);

/* All leaves return normally. The single operation record and canonical read
 * lease survive actual Source waits; no C++ frame invokes Source/OM/GTL. A
 * stale/foreign handle or obligation is a sticky error and cancels native work.
 * begin accepts canonical retained pointers only; NULL is absent, not failure.
 * Source callers obtaining an absent saved record pass its genuine NULL photo.
 * No second operation may overlap, including an absent one. */
IsleGame* isle_post_replay_game(IslePostCourseReplay*);
IsleGameResult isle_post_replay_status(IslePostCourseReplay*);
const char* isle_post_replay_error(IslePostCourseReplay*);
IsleGameResult isle_post_replay_fail(IslePostCourseReplay*, IsleGameResult);
IslePostCourseReplayStep isle_post_replay_score_begin(IslePostCourseReplay*, const IslePlayerPhotoData*, IslePostCourseReplayOperation*);
IslePostCourseReplayStep isle_post_replay_preview_begin(IslePostCourseReplay*, const IslePlayerPhotoData*, int32_t width, int32_t height, float scale, IslePostCourseReplayOperation*);
IslePostCourseReplayStep isle_post_replay_poll(IslePostCourseReplay*, IslePostCourseReplayOperation);
IsleGameResult isle_post_replay_sound85(IslePostCourseReplay*, IslePostCourseReplayOperation, uint64_t obligation);
IsleGameResult isle_post_replay_acknowledge(IslePostCourseReplay*, IslePostCourseReplayOperation, uint64_t obligation);
IsleGameResult isle_post_replay_take_score(IslePostCourseReplay*, IslePostCourseReplayOperation, IslePostCourseScoredPhoto*);
IsleGameResult isle_post_replay_preview_result(IslePostCourseReplay*, IslePostCourseReplayOperation, IslePostCoursePreview*);
IsleGameResult isle_post_replay_acknowledge_preview(IslePostCourseReplay*, IslePostCourseReplayOperation);
/* Releases bookkeeping ONLY after score consumption, preview acknowledgment,
 * absent input, or actual cancelled/failed terminal. Never discards flags,
 * native callbacks, GPU tokens or mapped readback. Cleanup remains available
 * after sticky failure; its actual terminal is separate from status. */
IsleGameResult isle_post_replay_operation_release(IslePostCourseReplay*, IslePostCourseReplayOperation);
IsleGameResult isle_post_replay_cancel(IslePostCourseReplay*);
/* One bounded genuine Host/native service turn through the SAME Progress.
 * It is not Runtime::pump, an OM update, a Source wait, or GTL update credit.
 * Stopped/failed still services native draining and never permits new submits. */
IsleGameResult isle_post_replay_progress(IslePostCourseReplay*);
IsleGameResult isle_post_replay_retire(IslePostCourseReplay*, int* retired);

/* Pure-C drivers. score NULL -> *present=0 without touching output; preview
 * NULL invokes consumer(NULL). Callback/audio/waits occur after returning
 * leaves. GPU/admission pending does NOT consume an original ohWait(1).
 * Failure/stop cancels and drains before returning; STOPPED is not scene exit.
 * output pointers/callback backing remain owned by the suspended Source stack. */
IsleGameResult isle_post_replay_score(IslePostCourseReplay*, const IslePlayerPhotoData*, IslePostCourseScoredPhoto*, int* present);
IsleGameResult isle_post_replay_preview(IslePostCourseReplay*, const IslePlayerPhotoData*, int32_t width, int32_t height, float scale, IslePostCoursePreviewConsumer, void*);
/* Explicit Source-only stop fence: drain returning leaves, THEN abandon the
 * interrupted update in C. Successful stop does not return or publish a scene
 * result. The parent still retires this context outside Source traversal. */
IsleGameResult isle_post_replay_stop(IslePostCourseReplay*);
#ifdef __cplusplus
}
#endif
#endif
