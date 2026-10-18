#ifndef ISLE_POST_COURSE_REPLAY_PROBE_H
#define ISLE_POST_COURSE_REPLAY_PROBE_H
#include "post_course_replay_source.h"
#include "post_course_window_source.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Authored native OM updates, not an Original GTL/gameplay controller. */
typedef struct IsleReplayProbe {
    IsleGame* game;
    IslePostCourseReplay* replay;
    IsleApplicationPhotos* photos;
    IslePostCourseWindow* window;
    IslePostCourseThumbnail thumbnail;
    const IslePlayerPhotoData* photo;
    IslePlayerPhotoData unchanged;
    IslePostCourseScoredPhoto score;
    IsleGameResult result;
    int mode, entered, returned, present, stage, cancelled_submitted;
    unsigned beats, sounds, waits60, waits1, lease_checks, preview_calls;
    unsigned wait60_begin, wait60_end, wait1_begin, wait1_end, callback_begin, callback_end;
    size_t preview_size, preview_stride;
    uint32_t preview_width, preview_height;
    uint8_t* preview_copy;
    size_t preview_capacity;
    unsigned joined;
} IsleReplayProbe;
enum { ISLE_REPLAY_PROBE_SCORE, ISLE_REPLAY_PROBE_PREVIEW, ISLE_REPLAY_PROBE_STOP, ISLE_REPLAY_PROBE_DRIVER_SCORE };
void isle_replay_probe_bind(IsleGameObject*, IsleReplayProbe*);
void isle_replay_probe_main(IsleGameObject*);
void isle_replay_probe_heartbeat(IsleGameObject*);
void isle_replay_probe_joined(IsleGameObject*, void*);
#ifdef __cplusplus
}
#endif
#endif
