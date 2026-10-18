#ifndef ISLE_POST_COURSE_WINDOW_PROBE_H
#define ISLE_POST_COURSE_WINDOW_PROBE_H
#include "post_course_window_source.h"
#include "player_source.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Test-owned C records survive the real native continuation and its joins. No
 * callback or wait crosses a C++ automatic frame. These are test instructions,
 * not replacement CameraCheck/PhotoCheck providers or captured-photo records. */
enum IsleWindowProbeMode { ISLE_WINDOW_PROBE_PRINT, ISLE_WINDOW_PROBE_CANCEL,
    ISLE_WINDOW_PROBE_NESTED, ISLE_WINDOW_PROBE_SYNC, ISLE_WINDOW_PROBE_LAYOUT };
typedef struct IsleWindowProbe {
    IsleGame* game;
    IslePostCourseWindow* window;
    IslePostCourseElement element;
    IslePostCourseButtonTable table;
    IslePlayerSource* player;
    IsleGameSprite* aux_target;
    int mode, entered, returned, callback_waits, count, old_callbacks, new_callbacks;
    int beats, aux_observed, aux_reads;
    uint16_t characters[16];
    int character_beats[16];
    int slots_before[9], slots_after[9], actions[3];
    size_t help_sizes[3];
    IsleGameResult result, nested_result, zero_wait_result, callback_result, aux_result;
} IsleWindowProbe;
void isle_window_probe_bind(IsleGameObject*,IsleWindowProbe*);
void isle_window_probe_main(IsleGameObject*);
void isle_window_probe_heartbeat(IsleGameObject*);
IsleGameResult isle_window_probe_character(void*,uint16_t);
#ifdef __cplusplus
}
#endif
#endif
