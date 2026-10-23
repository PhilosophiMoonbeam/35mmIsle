#include "post_course_scene_source.h"
#include "gallery_source.h"
#include "engine_internal.h"
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct PostCourseCallFrame { jmp_buf failure; void* previous; } PostCourseCallFrame;
#define SOURCE_CHECK(source, result) \
    isle_post_course_source_checked((source), (result), __func__, #result, __LINE__)
static int valid_scene(int32_t scene) {
    switch (scene) {
    case ISLE_POST_COURSE_PRINTER_DIAGNOSTIC:
    case ISLE_POST_COURSE_CAMERA_CHECK:
    case ISLE_POST_COURSE_ALBUM:
    case ISLE_POST_COURSE_REPORT:
    case ISLE_POST_COURSE_PHOTO_CHECK:
    case ISLE_POST_COURSE_CREDITS_17:
    case ISLE_POST_COURSE_CREDITS_18:
    case ISLE_POST_COURSE_CREDITS_19:
    case ISLE_POST_COURSE_CREDITS_20:
    case ISLE_POST_COURSE_GALLERY_22:
    case ISLE_POST_COURSE_GALLERY_23:
    case ISLE_POST_COURSE_END_LEVEL_8:
        return 1;
    default:
        return 0;
    }
}
IsleGameResult isle_post_course_source_activate(IslePostCourseSceneSource* source) {
    IsleGame* game;
    IsleGameResult result;
    if (!source || !source->owner || !source->domain || !source->game || !valid_scene(source->scene))
        return ISLE_GAME_INVALID_ARGUMENT;
    game = source->game;
    result = isle_game_status(game);
    if (result != ISLE_GAME_OK) return result;
    if (source->status != ISLE_GAME_OK) return source->status;
    if (isle_continuation_domain_check_access(source->domain) != ISLE_CONT_OK ||
        game->ticking || game->callback_depth || game->retirement_observer ||
        game->postCourseSource || game->playerSource || source->nonwaiting_call_frame)
        return ISLE_GAME_INVALID_STATE;
    game->postCourseSource = source;
    return ISLE_GAME_OK;
}
IsleGameResult isle_post_course_source_deactivate(IslePostCourseSceneSource* source) {
    IsleGame* game;
    IsleGameCounts counts;
    IsleGameResult result, fault;
    if (!source || !source->game) return ISLE_GAME_INVALID_ARGUMENT;
    game = source->game;
    /* Ordinary ownership validation remains available after sticky failure.
     * The owner has already joined every callback/GPU/Window/Replay borrower. */
    result = isle_game_counts(game, &counts);
    if (result != ISLE_GAME_OK) return result;
    if (game->ticking || game->callback_depth || game->retirement_observer ||
        omCurrentObject || omCurrentProcess || game->postCourseSource != source ||
        source->nonwaiting_call_frame) return ISLE_GAME_INVALID_STATE;
    fault = game->fault;
    game->postCourseSource = NULL;
    source->game = NULL;
    source->window = NULL;
    source->replay = NULL;
    source->input_object = NULL;
    source->input_process = NULL;
    source->input_invalidate = NULL;
    return fault;
}
IsleGameResult isle_post_course_source_context(IsleGame* game, IslePostCourseSceneSource** output) {
    IslePostCourseSceneSource* source;
    IsleGameResult result;
    if (output) *output = NULL;
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    result = isle_game_status(game);
    if (result != ISLE_GAME_OK) return result;
    source = game->postCourseSource;
    if (!source) return ISLE_GAME_OK;
    if (source->game != game || !source->owner || !source->domain || !valid_scene(source->scene))
        return ISLE_GAME_INVALID_STATE;
    if (source->status != ISLE_GAME_OK) return source->status;
    *output = source;
    return ISLE_GAME_OK;
}
IslePostCourseSceneSource* isle_post_course_source_require(IsleGameObject* object, int32_t scene) {
    IsleGame* game;
    IslePostCourseSceneSource* source;
    IsleGameObjectMetadata metadata;
    if (!object) abort(); /* Original registered callbacks always receive GObj. */
    game = object->game;
    source = game->postCourseSource;
    if (!source)
        isle_native_execution_fault(game, "original post-course callback requires its live scene context");
    SOURCE_CHECK(source, game->fault);
    SOURCE_CHECK(source, isle_game_object_metadata(game, object, &metadata));
    if (source->game != game || !source->owner || !source->domain || source->scene != scene ||
        !valid_scene(scene) || (!source->nonwaiting_call_frame && (!game->ticking || !omCurrentObject)))
        SOURCE_CHECK(source, ISLE_GAME_INVALID_STATE);
    SOURCE_CHECK(source, source->status);
    return source;
}
IsleGallerySource* isle_gallery_source_require(IsleGameObject* object,
    IsleGameSourceCallback expected) {
    IslePostCourseSceneSource* source;
    IsleGame* game;
    GObjProcess* process;
    IsleGameSourceCallback registered = NULL;
    if (!object || !object->game) abort();
    game = object->game;
    source = isle_post_course_source_require(object,
        game->postCourseSource ? game->postCourseSource->scene : 0);
    if (!expected ||
        (source->scene != ISLE_POST_COURSE_GALLERY_22 &&
         source->scene != ISLE_POST_COURSE_GALLERY_23) ||
        !game->ticking || omCurrentObject != object)
        SOURCE_CHECK(source, ISLE_GAME_INVALID_STATE);
    process = omCurrentProcess;
    if (process) {
        SOURCE_CHECK(source, isle_game_process_source_function(game, process, &registered));
        if (process->game != game || !process->alive || process->object != object ||
            registered != expected)
            SOURCE_CHECK(source, ISLE_GAME_INVALID_STATE);
    } else if (object->fnUpdate != expected) {
        SOURCE_CHECK(source, ISLE_GAME_INVALID_STATE);
    }
    /* Scene22/23 share this exact C provider; common is its first member.
     * This is not a console GObj/userData or a guest overlay layout. */
    return (IsleGallerySource*)source;
}
static void source_input_update(IsleGameObject* object) {
    IslePostCourseSceneSource* source;
    if (!object || !object->game || !object->game->postCourseSource) abort();
    source = isle_post_course_source_require(object,object->game->postCourseSource->scene);
    if (source->input_object != object)
        SOURCE_CHECK(source,ISLE_GAME_INVALID_STATE);
    /* Reached normal mode of func_800AAB5C. These overlay providers never
     * select the app-render recording/playback modes; no fake ROM/DMA replay. */
    for (;;) {
        SOURCE_CHECK(source,source->input_invalidate(source));
        isle_post_course_source_wait(source,1);
    }
}
IsleGameResult isle_post_course_source_install_input(IslePostCourseSceneSource* source) {
    IsleGameObjectMetadata metadata;
    IsleGameResult result;
    if (!source || !source->game || !source->input_object || source->input_process ||
        !source->input_invalidate)
        return ISLE_GAME_INVALID_ARGUMENT;
    result = isle_game_object_metadata(source->game,source->input_object,&metadata);
    if (result != ISLE_GAME_OK) return result;
    if (metadata.update_link != 9) return ISLE_GAME_INVALID_STATE;
    return isle_game_add_source_process(source->game,source->input_object,source_input_update,
        ISLE_GAME_STACKFUL,0,&source->input_process);
}
IsleGameResult isle_post_course_source_call(IslePostCourseSceneSource* source,
    IslePostCourseNonwaitingCall callback, void* user) {
    PostCourseCallFrame frame;
    IsleGameResult result;
    if (!source || !source->game || !callback) return ISLE_GAME_INVALID_ARGUMENT;
    result = isle_game_status(source->game);
    if (result != ISLE_GAME_OK) return result;
    if (source->game->postCourseSource != source || !source->owner || !source->domain ||
        !valid_scene(source->scene)) return ISLE_GAME_INVALID_STATE;
    if (source->status != ISLE_GAME_OK) return source->status;
    frame.previous = source->nonwaiting_call_frame;
    source->nonwaiting_call_frame = &frame;
    if (setjmp(frame.failure) == 0) callback(source, user);
    source->nonwaiting_call_frame = frame.previous;
    return source->status;
}
void isle_post_course_source_checked(IslePostCourseSceneSource* source, IsleGameResult result,
    const char* caller, const char* expression, int32_t line) {
    if (result == ISLE_GAME_OK) return;
    if (source->status == ISLE_GAME_OK ||
        (source->status == ISLE_GAME_STOPPED && result != ISLE_GAME_STOPPED))
        source->status = result;
    if (result!=ISLE_GAME_STOPPED && !source->failure[0])
        (void)snprintf(source->failure,sizeof(source->failure),"post-course %.64s:%d: %.140s -> %d",
            caller, (int)line, expression, (int)result);
    if (source->nonwaiting_call_frame)
        longjmp(((PostCourseCallFrame*)source->nonwaiting_call_frame)->failure, 1);
    if (source->status == ISLE_GAME_STOPPED) {
        /* Replay has already returned through every native C++ leaf and drained
         * its actual GPU/Photo borrowers. Discard only this active C stack; the
         * owner-side C resume fence abandons the OM traversal, not its result. */
        if (source->game) {
            result = isle_game_stop_update(source->game);
        } else {
            IsleContinuationResult finished = isle_continuation_finish(source->domain);
            result = finished == ISLE_CONT_WRONG_OWNER ? ISLE_GAME_WRONG_OWNER :
                finished == ISLE_CONT_RESOURCE_FAILURE ? ISLE_GAME_EXHAUSTED :
                finished == ISLE_CONT_EXECUTION_FAILURE ? ISLE_GAME_EXECUTION_FAILURE :
                ISLE_GAME_INVALID_STATE;
        }
        /* A valid stop never returns. An invalid fence is a real sticky fault,
         * never permission for original C to publish a result and continue. */
        source->status = result;
    }
    if (!source->failure[0])
        (void)snprintf(source->failure,sizeof(source->failure),"post-course stop fence returned: %d",(int)result);
    if (source->game)
        isle_native_execution_fault(source->game, source->failure);
    /* The independent original entry may fail before setup or after retirement.
     * It owns a separate C stack; never inspect retired scene storage here. */
    (void)isle_continuation_fail(source->domain, source->failure);
    abort(); /* An invalid/nonactive entry cannot authorize a failure transfer. */
}
void isle_post_course_source_wait(IslePostCourseSceneSource* source, int32_t updates) {
    if (updates < 0 || (updates && source->nonwaiting_call_frame)) {
        SOURCE_CHECK(source, ISLE_GAME_INVALID_STATE);
        return;
    }
    SOURCE_CHECK(source, isle_game_source_wait(source->game, (uint32_t)updates));
}
IsleGameResult isle_post_course_ack_begin(IslePostCourseSceneSource* source,
    IslePostCourseElement element,int32_t caller_line) {
    IslePostCourseAcknowledgment* ack;
    uint64_t serial,consumed_serial;
    uint32_t consumed_buttons;
    if (!source || !element.owner || !element.serial || caller_line<=0)
        return ISLE_GAME_INVALID_ARGUMENT;
    ack=&source->acknowledgment;
    if (ack->active || ack->serial==UINT64_MAX) return ISLE_GAME_INVALID_STATE;
    serial=ack->serial+1;
    consumed_serial=ack->consumed_serial;consumed_buttons=ack->consumed_buttons;
    *ack=(IslePostCourseAcknowledgment){0};
    ack->element=element;ack->serial=serial;ack->caller_line=caller_line;ack->active=1;
    ack->consumed_serial=consumed_serial;ack->consumed_buttons=consumed_buttons;
    return ISLE_GAME_OK;
}
IsleGameResult isle_post_course_ack_end(IslePostCourseSceneSource* source) {
    if (!source || !source->acknowledgment.active) return ISLE_GAME_INVALID_STATE;
    source->acknowledgment.active=0;source->acknowledgment.waiting=0;
    return ISLE_GAME_OK;
}
void isle_post_course_ack_input(IslePostCourseSceneSource* source,uint32_t current,uint32_t pressed) {
    IslePostCourseAcknowledgment* ack=&source->acknowledgment;
    if (!ack->active) return;
    if (ack->polls!=UINT64_MAX) ++ack->polls;
    ack->current_buttons=current;ack->pressed_buttons=pressed;
    if (pressed&0xc000u) {
        ack->consumed_serial=ack->serial;ack->consumed_buttons=pressed;
    }
}
void isle_post_course_ack_wait(IslePostCourseSceneSource* source,int32_t updates) {
    IslePostCourseAcknowledgment* ack=&source->acknowledgment;
    ack->waiting=ack->active;
    isle_post_course_source_wait(source,updates);
    ack->waiting=0;
}
IsleGameResult isle_post_course_source_join_processes(IslePostCourseSceneSource* source) {
    IsleGame* game;
    IsleGameResult result;
    size_t link;
    if (!source || !source->game) return ISLE_GAME_INVALID_ARGUMENT;
    game = source->game;
    result = isle_game_require_idle(game);
    if (result != ISLE_GAME_OK) return result;
    if (game->postCourseSource != source || source->nonwaiting_call_frame)
        return ISLE_GAME_INVALID_STATE;
    for (link=0;link<ARRAY_COUNT(omGObjListHead);++link) {
        GObj* object = omGObjListHead[link];
        while (object) {
            GObj* next_object = object->next;
            while (object->processListHead) {
                GObjProcess* process = object->processListHead;
                (void)isle_game_end_process(game,process);
                if (object->processListHead == process)
                    return ISLE_GAME_INVALID_STATE;
            }
            object = next_object;
        }
    }
    return game->fault;
}
