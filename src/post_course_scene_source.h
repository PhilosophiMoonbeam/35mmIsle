#ifndef ISLE_POST_COURSE_SCENE_SOURCE_H
#define ISLE_POST_COURSE_SCENE_SOURCE_H
#include "35mmisle/continuation.h"
#include "post_course_window_source.h"
#include "post_course_replay_source.h"
#ifdef __cplusplus
extern "C" {
#endif

enum { ISLE_POST_COURSE_CAMERA_CHECK = 9, ISLE_POST_COURSE_PHOTO_CHECK = 15 };
/* One actual overlay execution context. Its owner and entry domain outlive
 * sceneStorage. Services below are borrowed only while the real scene exists.
 * Original GObj::userData remains available to original C; it is not this link.
 * Canonical Photos, Window and Replay are never mirrored in this record. */
struct IslePostCourseSceneSource;
typedef IsleGameResult (*IslePostCourseInputInvalidate)(struct IslePostCourseSceneSource*);
/* Native execution-boundary record, not a Source phase/clock. The actual
 * acknowledgment call wrapper records its pinned caller line and Element;
 * only its real input reads and suspended waits update the remaining fields. */
typedef struct IslePostCourseAcknowledgment {
    IslePostCourseElement element;
    uint64_t serial,polls,consumed_serial;
    int32_t caller_line;
    uint32_t current_buttons,pressed_buttons,consumed_buttons;
    int active,waiting;
} IslePostCourseAcknowledgment;
typedef struct IslePostCourseSceneSource {
    void* owner;
    IsleContinuationDomain* domain;
    IsleGame* game;
    IslePostCourseWindow* window;
    IslePostCourseReplay* replay;
    IsleApplicationPhotos* photos;
    IsleGameObject* input_object;
    IsleGameProcess* input_process;
    IslePostCourseInputInvalidate input_invalidate;
    int32_t scene;
    IsleGameResult status;
    void* nonwaiting_call_frame;
    IslePostCourseAcknowledgment acknowledgment;
    /* Native-only first failure, retained across C unwind and scene retirement.
     * No success telemetry, guest state, pointer dump or STOPPED-as-error. */
    char failure[256];
} IslePostCourseSceneSource;

typedef void (*IslePostCourseNonwaitingCall)(IslePostCourseSceneSource*, void*);
/* Native bind/unbind occur outside callbacks and require the sole contextual
 * Game link. Lookup validates actual object membership and this scene identity.
 * Source callbacks/direct C calls use matching C-only failure/STOPPED fences;
 * no owning C++ frame may lie below a Source wait or terminal transfer. STOPPED
 * abandons Source only after the native caller has joined its actual borrowers;
 * it does not return an original entry result or complete an OM update. */
IsleGameResult isle_post_course_source_activate(IslePostCourseSceneSource*);
IsleGameResult isle_post_course_source_deactivate(IslePostCourseSceneSource*);
IsleGameResult isle_post_course_source_context(IsleGame*, IslePostCourseSceneSource**);
IslePostCourseSceneSource* isle_post_course_source_require(IsleGameObject*, int32_t scene);
IsleGameResult isle_post_course_source_call(IslePostCourseSceneSource*, IslePostCourseNonwaitingCall, void*);
void isle_post_course_source_checked(IslePostCourseSceneSource*, IsleGameResult,
    const char* caller, const char* expression, int32_t line);
void isle_post_course_source_wait(IslePostCourseSceneSource*, int32_t updates);
IsleGameResult isle_post_course_ack_begin(IslePostCourseSceneSource*,
    IslePostCourseElement,int32_t caller_line);
IsleGameResult isle_post_course_ack_end(IslePostCourseSceneSource*);
void isle_post_course_ack_input(IslePostCourseSceneSource*,uint32_t current,uint32_t pressed);
void isle_post_course_ack_wait(IslePostCourseSceneSource*,int32_t updates);
/* Owner-only, outside traversal/graphics borrowers: join every actual native
 * process before releasing C callback/Window backing. Cleanup continues after
 * sticky Game failure; its return preserves that failure, not a fake success. */
IsleGameResult isle_post_course_source_join_processes(IslePostCourseSceneSource*);
/* Actual Source kind-0/priority-0 Link9 normal-mode cache invalidation owner.
 * Install only after the allocation-only input initializer has returned. */
IsleGameResult isle_post_course_source_install_input(IslePostCourseSceneSource*);
IsleGameResult isle_post_course_native_input_invalidate(IslePostCourseSceneSource*);
#ifdef __cplusplus
}
#endif
#endif
