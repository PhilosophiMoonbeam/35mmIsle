#ifndef ISLE_COURSE_ACTOR_SOURCE_H
#define ISLE_COURSE_ACTOR_SOURCE_H
#include "actor_owner_source.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef enum IsleCourseActorPhase {
    ISLE_COURSE_ACTOR_UNPREPARED = 0,
    ISLE_COURSE_ACTOR_PREPARED,
    ISLE_COURSE_ACTOR_ACTIVE,
    ISLE_COURSE_ACTOR_DETACHED
} IsleCourseActorPhase;

/* First member of each distinct course1..5 typed actor record. This header
 * shares only the returning C fault fence, never admission or native services.
 * Typed owners alone establish and retire the canonical course binding. */
typedef struct IsleCourseActorBase {
    IsleCourseActorPhase phase;
    int32_t course_id;
    IsleGame* nativeGame;
    void* owner;
    void* nonwaiting_call_frame;
    IsleGameResult status;
} IsleCourseActorBase;

/* Internal native bridge callback only: never resolve a guest value as a call.
 * The typed caller MUST complete canonical course-specific execution admission
 * before invoking. ACTIVE and a selected course are guards, not that admission.
 * The callback is nonnull; its opaque context contract belongs to the typed
 * caller and may permit NULL. Both are borrowed only for this call.
 * Execution is nonwaiting and C-only across failure transfers: native C++
 * leaves MUST return their result before checked() runs; never throw or longjmp
 * through an intervening C++ frame. No automatic cleanup is performed. */
typedef void (*IsleCourseActorCall)(void*);
IsleGameResult isle_course_actor_invoke(IsleCourseActorBase*, IsleCourseActorCall, void*);
void isle_course_actor_checked(IsleCourseActorBase*, IsleGameResult);

#ifdef __cplusplus
}
#endif
#endif
