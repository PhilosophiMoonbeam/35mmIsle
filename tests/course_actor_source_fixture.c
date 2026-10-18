/* C-only fault-fence regression, not gameplay/typed-course admission. No scene is
 * activated here. Real native Game and canonical Player lifetimes exercise the
 * shared C mechanism independently of the five course-specific admission APIs. */
#include "course_actor_source.h"
#include "engine_internal.h"
#include "player_source.h"
#include <stdlib.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) return __LINE__; } while (0)
#define RECORD(s, condition) do { \
    if (!(condition) && !(s)->failure) (s)->failure = __LINE__; \
} while (0)

typedef struct Smoke {
    IsleCourseActorBase source;
    IslePlayerSource player;
    isle_rng rng;
    isle_arena arena;
    unsigned char* storage;
    IsleGame* game;
    void* outer_source_frame;
    void* outer_player_frame;
    void* enclosing_player_frame;
    int player_active;
    int fail_inner;
    int failure;
    int entered;
    int inner_after;
    int outer_after;
    int enclosing_after;
} Smoke;

static int open_game(Smoke* s, int with_player) {
    memset(s, 0, sizeof(*s));
    s->storage = malloc(65536);
    CHECK(s->storage != NULL);
    CHECK(isle_arena_init(&s->arena, s->storage, 65536) == ISLE_OK);
    CHECK(isle_game_create(&s->arena, &s->game) == ISLE_GAME_OK);
    if (with_player) {
        s->rng.seed = 1;
        CHECK(isle_player_source_prepare(s->game, &s->player, &s->rng, s, 10) == ISLE_GAME_OK);
        CHECK(isle_player_source_activate(&s->player) == ISLE_GAME_OK);
        s->player_active = 1;
    }
    s->source = (IsleCourseActorBase){ISLE_COURSE_ACTOR_ACTIVE, 3, s->game, s, NULL, ISLE_GAME_OK};
    return 0;
}

static int close_game(Smoke* s) {
    IsleGameResult fault = isle_game_status(s->game);
    if (s->player_active)
        CHECK(isle_player_source_deactivate(&s->player) == fault);
    CHECK(isle_game_destroy(s->game) == ISLE_GAME_OK);
    free(s->storage);
    s->storage = NULL;
    return 0;
}

static void count_call(void* argument) {
    Smoke* s = argument;
    ++s->entered;
}

static void null_payload_call(void* argument) {
    /* The opaque payload belongs to the typed caller; NULL is not rejected by
     * the shared fence. No guest callback or invented pointer is involved. */
    if (argument != NULL) abort();
}

static void deepest_player_call(IslePlayerSource* player, void* argument) {
    Smoke* s = argument;
    RECORD(s, player->nonwaiting_call_frame != s->outer_player_frame);
    isle_course_actor_checked(&s->source, ISLE_GAME_EXHAUSTED);
    ++s->inner_after;
}

static void inner_call(void* argument) {
    Smoke* s = argument;
    void* source_frame = s->source.nonwaiting_call_frame;
    void* player_frame = s->player.nonwaiting_call_frame;
    RECORD(s, source_frame != NULL && source_frame != s->outer_source_frame);
    RECORD(s, player_frame != NULL && player_frame != s->outer_player_frame);
    ++s->entered;
    if (s->fail_inner) {
        IsleGameResult result = isle_player_source_call(&s->player, deepest_player_call, s);
        /* The deepest Player must restore itself before the Source/Player
         * fences containing it unwind through their returning C calls. */
        RECORD(s, result == ISLE_GAME_EXHAUSTED);
        RECORD(s, s->source.nonwaiting_call_frame == source_frame);
        RECORD(s, s->player.nonwaiting_call_frame == player_frame);
        isle_course_actor_checked(&s->source, result);
    }
    ++s->inner_after;
}

static void outer_call(void* argument) {
    Smoke* s = argument;
    IsleGameResult result;
    s->outer_source_frame = s->source.nonwaiting_call_frame;
    s->outer_player_frame = s->player.nonwaiting_call_frame;
    RECORD(s, s->outer_source_frame != NULL && s->outer_player_frame != NULL);
    result = isle_course_actor_invoke(&s->source, inner_call, s);
    RECORD(s, result == (s->fail_inner ? ISLE_GAME_EXHAUSTED : ISLE_GAME_OK));
    RECORD(s, s->source.nonwaiting_call_frame == s->outer_source_frame);
    RECORD(s, s->player.nonwaiting_call_frame == s->outer_player_frame);
    if (s->fail_inner) {
        /* A second failure cannot replace the first fault while propagating. */
        isle_course_actor_checked(&s->source, ISLE_GAME_INVALID_STATE);
    }
    ++s->outer_after;
}

static void enclosing_player_call(IslePlayerSource* player, void* argument) {
    Smoke* s = argument;
    IsleGameResult result;
    s->enclosing_player_frame = player->nonwaiting_call_frame;
    result = isle_course_actor_invoke(&s->source, outer_call, s);
    RECORD(s, result == ISLE_GAME_EXHAUSTED);
    RECORD(s, s->source.nonwaiting_call_frame == NULL);
    RECORD(s, player->nonwaiting_call_frame == s->enclosing_player_frame);
    isle_player_source_checked(player, result);
    ++s->enclosing_after;
}

static void actual_game_fault_call(void* argument) {
    Smoke* s = argument;
    ++s->entered;
    isle_native_execution_fault(s->game, "private course fence smoke native failure");
    ++s->inner_after;
}

static int nested_success(void) {
    Smoke s;
    int result = open_game(&s, 1);
    CHECK(result == 0);
    CHECK(isle_course_actor_invoke(&s.source, outer_call, &s) == ISLE_GAME_OK);
    CHECK(!s.failure && s.entered == 1 && s.inner_after == 1 && s.outer_after == 1);
    CHECK(s.source.nonwaiting_call_frame == NULL && s.player.nonwaiting_call_frame == NULL);
    CHECK(isle_course_actor_invoke(&s.source, null_payload_call, NULL) == ISLE_GAME_OK);
    return close_game(&s);
}

static int nested_failure(void) {
    Smoke s;
    int result = open_game(&s, 1);
    CHECK(result == 0);
    s.fail_inner = 1;
    CHECK(isle_player_source_call(&s.player, enclosing_player_call, &s) == ISLE_GAME_EXHAUSTED);
    CHECK(!s.failure && s.entered == 1);
    CHECK(!s.inner_after && !s.outer_after && !s.enclosing_after);
    CHECK(s.source.status == ISLE_GAME_EXHAUSTED && s.player.status == ISLE_GAME_EXHAUSTED);
    CHECK(s.source.nonwaiting_call_frame == NULL && s.player.nonwaiting_call_frame == NULL);
    CHECK(isle_course_actor_invoke(&s.source, count_call, &s) == ISLE_GAME_EXHAUSTED);
    CHECK(s.entered == 1);
    CHECK(isle_game_status(s.game) == ISLE_GAME_OK);
    return close_game(&s);
}

static int guards(void) {
    Smoke s;
    int course;
    int result = open_game(&s, 1);
    CHECK(result == 0);
    CHECK(isle_course_actor_invoke(NULL, count_call, &s) == ISLE_GAME_INVALID_ARGUMENT);
    CHECK(isle_course_actor_invoke(&s.source, NULL, &s) == ISLE_GAME_INVALID_ARGUMENT);
    s.source.phase = ISLE_COURSE_ACTOR_UNPREPARED;
    CHECK(isle_course_actor_invoke(&s.source, count_call, &s) == ISLE_GAME_INVALID_STATE);
    s.source.phase = ISLE_COURSE_ACTOR_PREPARED;
    CHECK(isle_course_actor_invoke(&s.source, count_call, &s) == ISLE_GAME_INVALID_STATE);
    s.source.phase = ISLE_COURSE_ACTOR_ACTIVE;
    s.source.course_id = 0;
    CHECK(isle_course_actor_invoke(&s.source, count_call, &s) == ISLE_GAME_INVALID_STATE);
    s.source.course_id = 6;
    CHECK(isle_course_actor_invoke(&s.source, count_call, &s) == ISLE_GAME_INVALID_STATE);
    s.source.course_id = 3;
    s.source.owner = NULL;
    CHECK(isle_course_actor_invoke(&s.source, count_call, &s) == ISLE_GAME_INVALID_STATE);
    s.source.owner = &s;
    s.source.nativeGame = NULL;
    CHECK(isle_course_actor_invoke(&s.source, count_call, &s) == ISLE_GAME_INVALID_STATE);
    s.source.nativeGame = s.game;
    CHECK(s.entered == 0 && s.source.status == ISLE_GAME_OK);
    for (course = 1; course <= 5; course += 4) {
        s.source.course_id = course;
        CHECK(isle_course_actor_invoke(&s.source, count_call, &s) == ISLE_GAME_OK);
    }
    CHECK(s.entered == 2);
    s.source.phase = ISLE_COURSE_ACTOR_DETACHED;
    result = close_game(&s);
    CHECK(result == 0);
    /* nativeGame deliberately retains its actual, now-destroyed borrow. With
     * ASan this also detects any dereference before the detached-phase guard. */
    CHECK(isle_course_actor_invoke(&s.source, count_call, &s) == ISLE_GAME_INVALID_STATE);
    CHECK(s.entered == 2 && s.source.nonwaiting_call_frame == NULL);
    return 0;
}

static int canonical_player_required(void) {
    Smoke s;
    int result = open_game(&s, 0);
    CHECK(result == 0);
    CHECK(isle_course_actor_invoke(&s.source, count_call, &s) == ISLE_GAME_INVALID_STATE);
    CHECK(!s.entered && s.source.nonwaiting_call_frame == NULL);
    return close_game(&s);
}

static int actual_game_error(void) {
    Smoke s;
    IsleCourseActorBase fresh;
    int result = open_game(&s, 1);
    CHECK(result == 0);
    CHECK(isle_course_actor_invoke(&s.source, actual_game_fault_call, &s) == ISLE_GAME_EXECUTION_FAILURE);
    CHECK(s.entered == 1 && !s.inner_after);
    CHECK(s.source.nonwaiting_call_frame == NULL && s.player.nonwaiting_call_frame == NULL);
    CHECK(isle_game_status(s.game) == ISLE_GAME_EXECUTION_FAILURE);
    fresh = (IsleCourseActorBase){ISLE_COURSE_ACTOR_ACTIVE, 1, s.game, &s, NULL, ISLE_GAME_OK};
    CHECK(isle_course_actor_invoke(&fresh, count_call, &s) == ISLE_GAME_EXECUTION_FAILURE);
    CHECK(fresh.status == ISLE_GAME_EXECUTION_FAILURE && s.entered == 1);
    return close_game(&s);
}

int isle_course_actor_fence_smoke(void) {
    int result = nested_success();
    if (!result) result = nested_failure();
    if (!result) result = guards();
    if (!result) result = canonical_player_required();
    if (!result) result = actual_game_error();
    return result;
}
