#ifndef ISLE_CREDITS_SOURCE_H
#define ISLE_CREDITS_SOURCE_H
#include "post_course_scene_source.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct IsleCreditsSource IsleCreditsSource;
typedef struct IsleCreditsWorker IsleCreditsWorker;
typedef struct IsleCreditsDisplayListCursor IsleCreditsDisplayListCursor;
typedef struct IsleCreditsElement { IslePostCourseElement handle; } IsleCreditsElement;
typedef struct IsleCreditsCredit {
    uint32_t unk_0;
    float unk_4;
    const char* unk_8;
    IsleCreditsElement* unk_C;
} IsleCreditsCredit;
/* A real Window-issued writable-raster incarnation, never a Sprite snapshot.
 * The fixed Window owner retains its eight separate pieces until release. */
typedef struct IsleCreditsDestination {
    IsleGameSprite* sprite;
    IslePostCourseThumbnail image;
    uint64_t incarnation;
} IsleCreditsDestination;
typedef void (*IsleCreditsCleanup)(void*);
typedef void (*IsleCreditsPreRender)(IsleCreditsDisplayListCursor**);
/* Every entry is mandatory and returning. Implementations route to their actual
 * constructing owner before touching native services. They never invoke Source,
 * advance OM/GTL, or translate STOPPED into scene completion. */
typedef struct IsleCreditsNative {
    IsleGameResult (*screen)(IsleCreditsSource*,uint32_t);
    IsleGameResult (*heap)(IsleCreditsSource*,size_t);
    IsleGameResult (*heap_free)(IsleCreditsSource*,void*);
    IsleGameResult (*input_initialize)(IsleCreditsSource*);
    IsleGameResult (*window_initialize)(IsleCreditsSource*);
    IsleGameResult (*cameras)(IsleCreditsSource*);
    IsleGameResult (*input_mask)(IsleCreditsSource*,int,uint32_t);
    IsleGameResult (*process_owner)(IsleCreditsSource*,int32_t,uint32_t,uint32_t,IsleGameObject**);
    IsleGameResult (*sprite_object)(IsleCreditsSource*,uint32_t,IsleGameObject**,IsleGameSprite**);
    IsleGameResult (*sprite_append)(IsleCreditsSource*,IsleGameObject*,uint32_t,IsleGameSprite**);
    IsleGameResult (*remove_sprites)(IsleCreditsSource*,IsleGameObject*);
    IsleGameResult (*setup_begin)(IsleCreditsSource*,int32_t,uint32_t,uint32_t,uint32_t,int,int,int);
    /* Completion means the actual Original setup returned. The owner keeps
     * Window/Game/raster storage alive until worker_join has returned. */
    IsleGameResult (*setup_poll)(IsleCreditsSource*,int*);
    IsleGameResult (*pressed)(IsleCreditsSource*,uint32_t*);
    IsleGameResult (*sound)(IsleCreditsSource*,int,int,int,float,int,int32_t*);
    IsleGameResult (*song)(IsleCreditsSource*,int,int);
    IsleGameResult (*volume)(IsleCreditsSource*,int,int,int);
    IsleGameResult (*fade_color)(IsleCreditsSource*,int,int,int);
    IsleGameResult (*fade_duration)(IsleCreditsSource*,int,float);
    IsleGameResult (*exit_scene)(IsleCreditsSource*);
    IsleGameResult (*display_list)(IsleCreditsSource*,IsleCreditsDisplayListCursor**,uint32_t);
    /* osGetTime in Original N64 tick units, not rand()/fixed/sample-count RNG.
     * Actual pinned misc.c randTRange is compiled into the private provider. */
    IsleGameResult (*time)(IsleCreditsSource*,uint64_t*);
    /* Exact 8009FC38: prepare course World/effects only. The Presentation owner
     * retains the canonical record for the duration; this is not a render. */
    IsleGameResult (*prepare_course)(IsleCreditsSource*,const IslePlayerPhotoData*,int* done);
    IsleGameResult (*preview_scale)(IsleCreditsSource*,float*);
    /* Signal only: wake/cancel native pending service without waiting, joining,
     * traversing OM, or releasing any borrowed storage. Legal from the owner's
     * nested Host-progress return path while the C worker is parked. */
    IsleGameResult (*request_cancel)(IsleCreditsSource*);
    /* Returning cancel/drain, called on the owner after worker stack join.
     * done=1 only after real Replay/World/GPU/readback borrowers have retired. */
    IsleGameResult (*cancel_drain)(IsleCreditsSource*,int*);
} IsleCreditsNative;

struct IsleCreditsSource {
    IslePostCourseSceneSource common;
    const IsleCreditsNative* native;
    IsleCreditsWorker* worker;
    IsleGameObject* D_credits_801ECC80[5];
    IsleGameObject* D_credits_801ECC94;
    IsleGameObject* D_credits_801ECC98[2];
    IsleGameObject* D_credits_801ECCA0;
    uint32_t D_credits_801ECCA4;
    uint8_t D_credits_801ECCA8[8];
    IsleCreditsCredit D_credits_801ECCB0[15];
    int32_t D_credits_801ECDA0;
    /* 801ECDA3 is the final BE byte of the preceding word, NOT another field. */
    uint8_t D_credits_801ECDA4;
    int32_t D_credits_801ECDA8;
    int32_t D_credits_801ECDB0[152];
    int32_t D_credits_801ED010;
    int32_t D_credits_801E9414[3];
    uint32_t D_credits_801DEC40;
    IsleCreditsElement elements[14];
    size_t element_count;
    IsleCreditsDestination destinations[5];
    size_t destination_count;
    int near_disabled,update_interval,draw_interval;
    int32_t argument,source_result;
    int entry_complete,worker_active;
};

/* initialize is called exactly once per actual overlay-generation admission,
 * before first preparation; common/native/worker addresses remain stable. */
IsleGameResult isle_credits_source_initialize(IsleCreditsSource*,int32_t argument);
void isle_credits_source_entry(void*);
void isle_credits_source_init(IslePostCourseSceneSource*,void*);
void isle_credits_source_worker(void*);
int32_t func_credits_801DCB24(IsleCreditsSource*,int32_t);
void func_credits_801DC8A0(IsleCreditsDisplayListCursor**);
void func_credits_801DC9FC(void*);
void func_credits_801DCA1C(void*);
void func_credits_801DCEF0(IsleGameObject*);
void func_credits_801DD168(IsleGameObject*);
void func_credits_801DD49C(IsleGameObject*);
void func_credits_801DD540(IsleGameObject*);
void func_credits_801DDE24(IsleGameObject*);
void func_credits_801DDF48(IsleGameObject*);
void func_credits_801DE734(IsleGameObject*);
void func_credits_801DE844(IsleGameObject*);
/* Exact cleanup ABI lookup belongs to actual UI-heap allocation metadata.
 * It must reject foreign/freed pointers; no pointer whitelist/context global. */
IsleGameResult isle_credits_native_cleanup_context(void*,IsleCreditsSource**);
IsleGameResult isle_credits_native_display_context(IsleCreditsDisplayListCursor**,IsleCreditsSource**);
IsleGameResult isle_credits_source_release(IsleCreditsSource*);

/* Credits-specific returning worker leaves. Native pending is resumed only by
 * these C fences/owner service, never by an extra Source update or ohWait. */
IsleGameResult isle_credits_worker_queue(IsleCreditsWorker*,size_t);
IsleGameResult isle_credits_worker_create_thread(IsleCreditsWorker*,int32_t,IsleContinuationEntry,void*,int32_t);
IsleGameResult isle_credits_worker_start(IsleCreditsWorker*);
IsleGameResult isle_credits_worker_send(IsleCreditsWorker*,IsleGameSprite*,int*);
IsleGameResult isle_credits_worker_receive(IsleCreditsWorker*,IsleGameSprite**,int*);
IsleGameResult isle_credits_worker_priority(IsleCreditsWorker*,int32_t);
IsleGameResult isle_credits_worker_exclude_main(IsleCreditsWorker*,int);
IsleGameResult isle_credits_worker_suspend(IsleCreditsWorker*);
IsleGameResult isle_credits_worker_service(IsleCreditsWorker*);
IsleGameResult isle_credits_worker_cancel(IsleCreditsWorker*);
IsleGameResult isle_credits_worker_join(IsleCreditsWorker*,int*);
IsleGameResult isle_credits_worker_image(IsleCreditsWorker*,IsleGameSprite*,IsleCreditsDestination*,int*,int*);
/* Own C cancellation/failure fence for the separate worker domain. */
void isle_credits_worker_checked(IsleCreditsSource*,IsleGameResult,const char*,int32_t);
#ifdef __cplusplus
}
#endif
#endif
