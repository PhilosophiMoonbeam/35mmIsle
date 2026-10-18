#include "course_actor_source.h"
#include "engine_internal.h"
#include "player_source.h"
#include <stdlib.h>

typedef struct CourseActorCallFrame {
    jmp_buf failure;
    void* previous;
    void* previous_player_frame;
} CourseActorCallFrame;

typedef struct CourseActorInvocation {
    IsleCourseActorCall callback;
    void* argument;
} CourseActorInvocation;

static IsleGameResult native_status(IsleGame* game) {
    IsleGameResult result = isle_game_status(game);
    return result == ISLE_GAME_OK && game->update_stopped ? ISLE_GAME_STOPPED : result;
}

static void player_call(IslePlayerSource* player, void* argument) {
    CourseActorInvocation* invocation = argument;
    (void)player;
    invocation->callback(invocation->argument);
}

IsleGameResult isle_course_actor_invoke(IsleCourseActorBase* source,
    IsleCourseActorCall callback, void* argument) {
    CourseActorCallFrame frame;
    CourseActorInvocation invocation = {callback, argument};
    IslePlayerSource* player = NULL;
    IsleGameResult result;
    if (!source || !callback) return ISLE_GAME_INVALID_ARGUMENT;
    if (source->phase != ISLE_COURSE_ACTOR_ACTIVE || source->course_id < 1 ||
        source->course_id > 5 || !source->nativeGame || !source->owner)
        return ISLE_GAME_INVALID_STATE;
    if (source->status != ISLE_GAME_OK) return source->status;
    result = native_status(source->nativeGame);
    if (result != ISLE_GAME_OK) {
        source->status = result;
        return result;
    }
    result = isle_player_source_context(source->nativeGame, &player);
    if (result != ISLE_GAME_OK) return result;
    if (!player) return ISLE_GAME_INVALID_STATE;
    frame.previous = source->nonwaiting_call_frame;
    frame.previous_player_frame = player->nonwaiting_call_frame;
    source->nonwaiting_call_frame = &frame;
    if (setjmp(frame.failure) == 0) {
        result = isle_player_source_call(player, player_call, &invocation);
        if (result != ISLE_GAME_OK && source->status == ISLE_GAME_OK)
            source->status = result;
    }
    source->nonwaiting_call_frame = frame.previous;
    return source->status;
}

void isle_course_actor_checked(IsleCourseActorBase* source, IsleGameResult result) {
    CourseActorCallFrame* frame;
    IslePlayerSource* player = NULL;
    if (result == ISLE_GAME_OK) return;
    frame = source ? source->nonwaiting_call_frame : NULL;
    if (source) {
        if (source->status == ISLE_GAME_OK) source->status = result;
        result = source->status;
        /* Detachment may precede destruction of the old Game/Player storage.
         * Neither an old Player borrow nor its frame is valid after this phase. */
        if (source->phase == ISLE_COURSE_ACTOR_ACTIVE && source->nativeGame)
            player = source->nativeGame->playerSource;
    }
    /* Restore an inner Player link first. An enclosing Player fence must not
     * bypass restoration of the Source link owned by our live C stack frame. */
    if (player && player->nonwaiting_call_frame &&
        (!frame || player->nonwaiting_call_frame != frame->previous_player_frame))
        isle_player_source_checked(player, result);
    if (frame) longjmp(frame->failure, 1);
    if (source && source->phase == ISLE_COURSE_ACTOR_ACTIVE && source->nativeGame)
        isle_native_execution_fault(source->nativeGame, "native course actor leaf failed");
    /* Without an authorized C failure target, failure must never look like a
     * successful callback return or consult a detached scene's old Game. */
    abort();
}
