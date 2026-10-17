#include "owned_command_fixture.h"
#include "engine_internal.h"
#include "menu_scene_bridge.h"

#include <stddef.h>
#include <string.h>

/* All callbacks that may wait or cross a native C fault fence live in C. These
 * tests drive the real queue, OM updates/processes and continuation ownership. */
typedef struct CommandFixture {
    /* Arena allocation aligns absolute addresses, not the backing array type. */
    unsigned char storage[128 * 1024];
    isle_arena arena;
    IsleGame* game;
    IsleGameObject* receiver;
    IsleGameObject* sender;
    IsleGameObject* caller;
    IsleGameCommandPool pool;
    IsleGameCommandQueue queue;
    int failure;
    unsigned acquired, released, calls, admissions, completions;
    int32_t trace[8];
    int32_t reject_before, reject_after, append_on, finish_on, cancel_on;
    int32_t fault_on, wait_on, transfer_on;
    unsigned resumed, after_fault, transfer_returned;
    unsigned aborts, abort_returned, update_returned, dispatch_returned;
    unsigned joined, target_retiring;
    unsigned depth_before_abort, depth_after_abort, join_depth_before, join_depth_after;
    size_t retained_before_append, retained_after_append, consumed_at_append;
    size_t cursor_before_append, cursor_after_append;
    int free_during_append, held_receiver, probe_scope;
    IsleGameResult result, normal_end, wrong_receiver, nested_begin, wrong_end;
    IsleGameResult process_begin, recursive_dispatch, tick_after_finish;
    IsleGameResult identity_after_finish, send_after_cancel;
} CommandFixture;

#define VERIFY(f, expression) do { \
    if (!(expression) && (f)->failure == 0) (f)->failure = __LINE__; \
} while (0)

static void* acquire(IsleGameObject* source, void* user) {
    CommandFixture* f = user;
    if (source == NULL) return NULL;
    ++f->acquired;
    return f;
}

static void release(void* token, void* user) {
    CommandFixture* f = user;
    VERIFY(f, token == f && f->released < f->acquired);
    ++f->released;
}

static int initialize(CommandFixture* f) {
    memset(f, 0, sizeof(*f));
    VERIFY(f, isle_arena_init(&f->arena, f->storage, sizeof(f->storage)) == ISLE_OK);
    VERIFY(f, isle_game_create(&f->arena, &f->game) == ISLE_GAME_OK);
    if (f->game == NULL) return 0;
    f->pool.arena = &f->arena;
    f->queue.acquire = acquire;
    f->queue.release = release;
    f->queue.borrow_user = f;
    VERIFY(f, isle_game_add_object(f->game, 14, 0, 0x80000000, NULL, NULL, &f->receiver) == ISLE_GAME_OK);
    VERIFY(f, isle_game_add_object(f->game, 15, 0, 0x80000000, NULL, NULL, &f->sender) == ISLE_GAME_OK);
    VERIFY(f, isle_game_add_object(f->game, 16, 0, 0x80000000, NULL, NULL, &f->caller) == ISLE_GAME_OK);
    return f->failure == 0;
}

static int finish(CommandFixture* f) {
    /* Joining the actual caller precedes abandoned-frame cleanup. Target
     * retirement alone is deliberately never used as that proof. */
    if (f->caller != NULL && f->caller->alive) {
        const IsleGameResult fault = f->game->fault;
        VERIFY(f, isle_game_delete_object(f->game, f->caller) == fault);
        VERIFY(f, !f->caller->alive && f->game->fault == fault);
    }
    isle_game_commands_abort(f->game, &f->pool, &f->queue, 0);
    isle_game_commands_release(&f->pool, &f->queue);
    VERIFY(f, f->released == f->acquired);
    if (f->game->command_fault_context == f)
        VERIFY(f, isle_menu_command_fault_end(f->game, f) == ISLE_GAME_OK);
    if (f->held_receiver)
        VERIFY(f, isle_game_object_release_identity(f->game, f->receiver) == ISLE_GAME_OK);
    VERIFY(f, isle_game_destroy(f->game) == ISLE_GAME_OK);
    return f->failure;
}

static void send(CommandFixture* f, int32_t command) {
    VERIFY(f, isle_game_command_send(&f->pool, &f->queue, command, f->sender) == ISLE_GAME_OK);
}

static uint32_t flags(CommandFixture* f) {
    IsleGameObjectMetadata metadata = {0};
    VERIFY(f, isle_game_object_metadata(f->game, f->receiver, &metadata) == ISLE_GAME_OK);
    return metadata.flags;
}

static IsleGameResult before(IsleGameObject* object, int32_t command, IsleGameObject* source, void* user) {
    CommandFixture* f = user;
    (void) source;
    VERIFY(f, object == f->receiver && f->queue.processing && !f->queue.custom_callback_open);
    ++f->admissions;
    return command == f->reject_before ? ISLE_GAME_INVALID_ARGUMENT : ISLE_GAME_OK;
}

static IsleGameResult after(IsleGameObject* object, int32_t command, IsleGameObject* source, void* user) {
    CommandFixture* f = user;
    (void) source;
    VERIFY(f, object == f->receiver && f->queue.processing && !f->queue.custom_callback_open);
    ++f->completions;
    return command == f->reject_after ? ISLE_GAME_EXECUTION_FAILURE : ISLE_GAME_OK;
}

static void handler(IsleGameObject* object, int32_t command, IsleGameObject* source, void* user) {
    CommandFixture* f = user;
    unsigned depth = f->game->callback_depth;
    int erase = 0;
    (void) source;
    VERIFY(f, f->queue.processing && f->queue.custom_callback_open && depth != 0);
    VERIFY(f, f->queue.last_consumed != NULL && f->queue.consumed_count != 0);
    if (f->calls < ARRAY_COUNT(f->trace)) f->trace[f->calls] = command;
    else VERIFY(f, 0);
    ++f->calls;
    /* Observable native mutation happens before every deliberate failure. */
    VERIFY(f, isle_game_object_set_flags(f->game, object,
        UINT32_C(0x10000) | ((uint32_t) command & UINT32_C(0xffff))) == ISLE_GAME_OK);
    if (command == f->append_on) {
        f->retained_before_append = f->queue.count;
        f->consumed_at_append = f->queue.consumed_count;
        f->free_during_append = f->pool.free != NULL;
        f->cursor_before_append = f->arena.cursor;
        VERIFY(f, isle_game_command_send(&f->pool, &f->queue, 9, object) == ISLE_GAME_OK);
        f->cursor_after_append = f->arena.cursor;
        f->retained_after_append = f->queue.count;
    }
    if (command == f->finish_on) {
        isle_game_command_finish(&f->queue);
        isle_game_command_finish(&f->queue);
        VERIFY(f, f->queue.active_borrow == NULL && f->queue.custom_callback_open &&
            f->queue.processing && f->queue.consumed_count == 1 && f->game->callback_depth == depth);
        f->recursive_dispatch = isle_game_commands(f->game, object, &f->pool, &f->queue, handler, f, &erase);
        f->tick_after_finish = isle_game_tick(f->game);
        f->identity_after_finish = isle_game_object_release_identity(f->game, object);
    }
    if (command == f->cancel_on) {
        isle_game_commands_release(&f->pool, &f->queue);
        VERIFY(f, f->queue.cancelled && f->queue.processing && f->queue.custom_callback_open &&
            f->queue.count == 3 && f->queue.active_borrow != NULL && f->acquired - f->released == 1);
        f->send_after_cancel = isle_game_command_send(&f->pool, &f->queue, 9, object);
    }
    if (command == f->transfer_on) {
        omEndProcess(NULL);
        ++f->transfer_returned;
    }
    if (command == f->fault_on) {
        /* Real selected-source wait validation faults directly in native C. */
        ohWait(-1);
        ++f->after_fault;
    }
    if (command == f->wait_on) {
        VERIFY(f, isle_game_wait(f->game, 2) == ISLE_GAME_OK);
        ++f->resumed;
    }
}

static IsleGameResult checked(CommandFixture* f, int* erase) {
    return isle_game_commands_checked(f->game, f->receiver, &f->pool, &f->queue,
        handler, f, before, after, f, erase);
}

static void abort_owned(void* context) {
    CommandFixture* f = context;
    ++f->aborts;
    VERIFY(f, f->game->command_fault_abort == NULL && f->game->command_fault_context == NULL);
    VERIFY(f, f->game->fault == ISLE_GAME_EXECUTION_FAILURE && f->game->ticking);
    f->depth_before_abort = f->game->callback_depth;
    isle_game_commands_abort(f->game, &f->pool, &f->queue, 1);
    f->depth_after_abort = f->game->callback_depth;
    VERIFY(f, !f->queue.processing && !f->queue.custom_callback_open && f->queue.active_borrow == NULL);
    ++f->abort_returned;
}

static void owned_update(IsleGameObject* object, void* user) {
    CommandFixture* f = user;
    int erase = 0;
    f->wrong_receiver = isle_menu_command_fault_begin(f->game, f->sender, abort_owned, f);
    VERIFY(f, isle_menu_command_fault_begin(f->game, object, abort_owned, f) == ISLE_GAME_OK);
    if (f->probe_scope) {
        f->nested_begin = isle_menu_command_fault_begin(f->game, object, abort_owned, f);
        f->wrong_end = isle_menu_command_fault_end(f->game, &f->failure);
    }
    f->result = checked(f, &erase);
    f->normal_end = isle_menu_command_fault_end(f->game, f);
    VERIFY(f, f->normal_end == ISLE_GAME_OK && !erase);
    ++f->update_returned;
}

static void legacy_process(IsleGameObject* object, void* user) {
    CommandFixture* f = user;
    int erase = 0;
    if (f->probe_scope)
        f->process_begin = isle_menu_command_fault_begin(f->game, object, abort_owned, f);
    f->result = isle_game_commands(f->game, f->receiver, &f->pool, &f->queue, handler, f, &erase);
    VERIFY(f, !erase);
    ++f->dispatch_returned;
}

static void caller_joined(IsleGameObject* object, void* user) {
    CommandFixture* f = user;
    VERIFY(f, object == f->caller && f->game->retirement_observer);
    f->join_depth_before = f->game->callback_depth;
    isle_game_commands_abort(f->game, &f->pool, &f->queue, 0);
    f->join_depth_after = f->game->callback_depth;
    ++f->joined;
}

static void target_retiring(IsleGameObject* object, void* user) {
    CommandFixture* f = user;
    VERIFY(f, object == f->receiver && f->game->retirement_observer);
    isle_game_commands_release(&f->pool, &f->queue);
    ++f->target_retiring;
}

static void start_legacy(CommandFixture* f) {
    IsleGameProcess* process = NULL;
    VERIFY(f, isle_game_object_set_retire_callback(f->game, f->caller, caller_joined, f) == ISLE_GAME_OK);
    VERIFY(f, isle_game_add_process(f->game, f->caller, legacy_process, f,
        ISLE_GAME_STACKFUL, 1, &process) == ISLE_GAME_OK);
}

int owned_command_admission_regression(void) {
    CommandFixture f;
    int erase = 0;
    IsleGameCommand* first;
    IsleGameCommand* tail;
    if (!initialize(&f)) return f.failure;
    send(&f, -6); send(&f, 8);
    first = f.queue.head;
    tail = f.queue.tail;
    f.reject_before = -6;
    VERIFY(&f, checked(&f, &erase) == ISLE_GAME_INVALID_ARGUMENT && !erase);
    VERIFY(&f, f.calls == 0 && f.admissions == 1 && f.completions == 0 &&
        f.queue.head == first && f.queue.tail == tail && f.queue.count == 2 &&
        f.released == 0 && f.pool.free == NULL && f.queue.active_borrow == NULL &&
        !f.queue.processing && !f.queue.last_consumed && !f.queue.consumed_count && flags(&f) == 0);
    isle_game_commands_release(&f.pool, &f.queue);
    send(&f, -4); send(&f, 7); send(&f, 8);
    f.reject_before = 7;
    VERIFY(&f, checked(&f, &erase) == ISLE_GAME_INVALID_ARGUMENT);
    VERIFY(&f, f.calls == 0 && f.queue.count == 2 && f.acquired - f.released == 2 &&
        isle_menu_object_hidden(f.game, f.receiver) && f.pool.free != NULL);
    /* Repeated abort cannot settle or detach an undelivered node. */
    first = f.queue.head;
    isle_game_commands_abort(f.game, &f.pool, &f.queue, 1);
    VERIFY(&f, f.queue.head == first && f.queue.count == 2 && f.acquired - f.released == 2);
    return finish(&f);
}

int owned_command_completion_regression(void) {
    CommandFixture f;
    int erase = 0;
    if (!initialize(&f)) return f.failure;
    send(&f, 7); send(&f, -5); send(&f, 8);
    f.reject_after = 7;
    VERIFY(&f, checked(&f, &erase) == ISLE_GAME_EXECUTION_FAILURE && !erase);
    VERIFY(&f, f.calls == 1 && f.trace[0] == 7 && f.admissions == 1 && f.completions == 1 &&
        flags(&f) == UINT32_C(0x10007) && isle_menu_object_hidden(f.game, f.receiver) &&
        f.queue.count == 2 && f.released == 1 && !f.queue.processing &&
        !f.queue.custom_callback_open && f.queue.active_borrow == NULL && f.game->callback_depth == 0);
    /* The next drain can see only the ordinary suffix, never the failed node. */
    f.reject_after = 0;
    VERIFY(&f, checked(&f, &erase) == ISLE_GAME_OK && f.calls == 2 && f.trace[1] == 8 &&
        f.queue.count == 0 && f.released == 3 && flags(&f) == UINT32_C(0x10008));
    return finish(&f);
}

int owned_command_append_regression(void) {
    CommandFixture f;
    int erase = 0;
    size_t cursor;
    if (!initialize(&f)) return f.failure;
    send(&f, 7); send(&f, 8);
    f.append_on = 7;
    VERIFY(&f, checked(&f, &erase) == ISLE_GAME_OK && !erase);
    VERIFY(&f, f.calls == 3 && f.trace[0] == 7 && f.trace[1] == 8 && f.trace[2] == 9 &&
        f.retained_before_append == 2 && f.retained_after_append == 3 && f.consumed_at_append == 1 &&
        !f.free_during_append && f.cursor_after_append > f.cursor_before_append &&
        !f.queue.count && f.released == 3 && flags(&f) == UINT32_C(0x10009));
    cursor = f.arena.cursor;
    send(&f, 12);
    VERIFY(&f, f.arena.cursor == cursor);
    return finish(&f);
}

int owned_command_delete_regression(void) {
    CommandFixture f;
    int erase = 0;
    if (!initialize(&f)) return f.failure;
    send(&f, -4); send(&f, -1); send(&f, 8);
    f.reject_before = 8;
    VERIFY(&f, checked(&f, &erase) == ISLE_GAME_OK && erase);
    VERIFY(&f, f.admissions == 0 && f.calls == 0 && f.queue.count == 3 &&
        f.queue.consumed_count == 2 && !f.queue.processing && f.queue.delete_stopped &&
        f.queue.active_borrow != NULL && f.released == 1 && isle_menu_object_hidden(f.game, f.receiver));
    VERIFY(&f, checked(&f, &erase) == ISLE_GAME_INVALID_STATE && !erase && f.queue.count == 3);
    isle_game_command_finish(&f.queue);
    isle_game_command_finish(&f.queue);
    VERIFY(&f, f.released == 2 && f.queue.delete_stopped && f.queue.count == 3);
    VERIFY(&f, checked(&f, &erase) == ISLE_GAME_INVALID_STATE && f.admissions == 0);
    isle_game_commands_abort(f.game, &f.pool, &f.queue, 1);
    VERIFY(&f, !f.queue.count && !f.queue.delete_stopped && f.released == 3);
    return finish(&f);
}

int owned_command_token_regression(void) {
    for (int credited = 0; credited != 2; ++credited) {
        CommandFixture f;
        int erase = 0;
        if (!initialize(&f)) return f.failure;
        VERIFY(&f, isle_game_object_retain_identity(f.game, f.receiver) == ISLE_GAME_OK);
        f.held_receiver = 1;
        f.finish_on = 7;
        VERIFY(&f, isle_game_command_send(&f.pool, &f.queue, 7, credited ? f.sender : NULL) == ISLE_GAME_OK);
        VERIFY(&f, checked(&f, &erase) == ISLE_GAME_OK && !erase && f.calls == 1);
        VERIFY(&f, f.recursive_dispatch == ISLE_GAME_INVALID_STATE &&
            f.tick_after_finish == ISLE_GAME_INVALID_STATE && f.identity_after_finish == ISLE_GAME_INVALID_STATE &&
            f.acquired == (unsigned) credited && f.released == (unsigned) credited &&
            !f.queue.custom_callback_open && !f.queue.processing && f.game->callback_depth == 0);
        if (finish(&f) != 0) return f.failure;
    }
    return 0;
}

int owned_command_cancel_regression(void) {
    CommandFixture f;
    int erase = 0;
    if (!initialize(&f)) return f.failure;
    send(&f, 7); send(&f, -5); send(&f, 8);
    f.cancel_on = 7;
    VERIFY(&f, checked(&f, &erase) == ISLE_GAME_OK && !erase && f.calls == 1 &&
        f.send_after_cancel == ISLE_GAME_INVALID_ARGUMENT && flags(&f) == UINT32_C(0x10007) &&
        !f.queue.count && !f.queue.cancelled && f.released == 3 && !f.queue.custom_callback_open);
    return finish(&f);
}

int owned_command_fault_regression(void) {
    CommandFixture f;
    if (!initialize(&f)) return f.failure;
    send(&f, 7); send(&f, 8);
    f.fault_on = 7;
    VERIFY(&f, isle_menu_set_update(f.game, f.receiver, owned_update, &f) == ISLE_GAME_OK);
    VERIFY(&f, isle_game_tick(f.game) == ISLE_GAME_EXECUTION_FAILURE);
    VERIFY(&f, f.aborts == 1 && f.abort_returned == 1 && !f.after_fault && !f.update_returned &&
        f.depth_before_abort == 1 && f.depth_after_abort == 0 && f.calls == 1 &&
        flags(&f) == UINT32_C(0x10007) && f.queue.count == 1 && f.released == 1 &&
        !f.queue.last_consumed && !f.queue.consumed_count && !f.queue.processing &&
        !f.queue.custom_callback_open && f.queue.active_borrow == NULL && f.game->callback_depth == 0 &&
        f.game->command_fault_abort == NULL && f.game->command_fault_context == NULL);
    VERIFY(&f, isle_menu_command_fault_end(f.game, &f) == ISLE_GAME_INVALID_STATE);
    return finish(&f);
}

int owned_command_scope_regression(void) {
    CommandFixture f;
    if (!initialize(&f)) return f.failure;
    VERIFY(&f, isle_menu_command_fault_begin(f.game, f.receiver, abort_owned, &f) == ISLE_GAME_INVALID_STATE);
    send(&f, 7); send(&f, 8);
    f.probe_scope = 1;
    f.transfer_on = 7;
    VERIFY(&f, isle_menu_set_update(f.game, f.receiver, owned_update, &f) == ISLE_GAME_OK);
    VERIFY(&f, isle_game_tick(f.game) == ISLE_GAME_OK && f.update_returned == 1);
    VERIFY(&f, f.wrong_receiver == ISLE_GAME_INVALID_STATE && f.nested_begin == ISLE_GAME_INVALID_STATE &&
        f.wrong_end == ISLE_GAME_INVALID_STATE && f.normal_end == ISLE_GAME_OK &&
        f.transfer_returned == 1 && f.calls == 2 && f.trace[1] == 8 && !f.queue.count && !f.aborts &&
        f.game->command_fault_abort == NULL && f.game->command_fault_context == NULL);
    return finish(&f);
}

int owned_command_legacy_wait_regression(void) {
    CommandFixture f;
    if (!initialize(&f)) return f.failure;
    send(&f, 7); send(&f, 8);
    f.wait_on = 7;
    f.probe_scope = 1;
    start_legacy(&f);
    VERIFY(&f, isle_game_tick(f.game) == ISLE_GAME_OK && f.calls == 1 && !f.resumed &&
        !f.dispatch_returned && f.queue.processing && f.queue.custom_callback_open &&
        f.queue.count == 2 && !f.released && f.game->callback_depth == 0 &&
        f.process_begin == ISLE_GAME_INVALID_STATE);
    VERIFY(&f, isle_game_tick(f.game) == ISLE_GAME_OK && !f.resumed);
    VERIFY(&f, isle_game_tick(f.game) == ISLE_GAME_OK && f.resumed == 1 &&
        f.dispatch_returned == 1 && f.calls == 2 && f.trace[1] == 8 && f.released == 2 &&
        !f.queue.count && !f.queue.custom_callback_open && f.game->callback_depth == 0);
    return finish(&f);
}

int owned_command_target_retirement_regression(void) {
    CommandFixture f;
    if (!initialize(&f)) return f.failure;
    send(&f, 7); send(&f, 8);
    f.wait_on = 7;
    VERIFY(&f, isle_game_object_retain_identity(f.game, f.receiver) == ISLE_GAME_OK);
    f.held_receiver = 1;
    VERIFY(&f, isle_game_object_set_pre_retire_callback(f.game, f.receiver, target_retiring, &f) == ISLE_GAME_OK);
    start_legacy(&f);
    VERIFY(&f, isle_game_tick(f.game) == ISLE_GAME_OK && f.calls == 1 && f.queue.processing);
    VERIFY(&f, isle_game_delete_object(f.game, f.receiver) == ISLE_GAME_OK && f.target_retiring == 1);
    VERIFY(&f, !f.receiver->alive && !f.joined && !f.resumed && !f.dispatch_returned &&
        f.queue.cancelled && f.queue.processing && f.queue.custom_callback_open &&
        f.queue.active_borrow != NULL && f.queue.count == 2 && f.released == 1);
    VERIFY(&f, isle_game_tick(f.game) == ISLE_GAME_OK && !f.resumed);
    VERIFY(&f, isle_game_tick(f.game) == ISLE_GAME_OK && f.resumed == 1 &&
        f.dispatch_returned == 1 && f.calls == 1 && f.released == 2 &&
        !f.queue.count && !f.queue.processing && !f.queue.custom_callback_open);
    return finish(&f);
}

int owned_command_caller_join_regression(void) {
    CommandFixture f;
    int erase = 0;
    if (!initialize(&f)) return f.failure;
    send(&f, 7); send(&f, 8);
    f.wait_on = 7;
    start_legacy(&f);
    VERIFY(&f, isle_game_tick(f.game) == ISLE_GAME_OK && f.calls == 1 && f.queue.processing);
    VERIFY(&f, isle_game_delete_object(f.game, f.caller) == ISLE_GAME_OK && f.joined == 1);
    VERIFY(&f, f.join_depth_before == 1 && f.join_depth_after == 1 && !f.resumed && !f.dispatch_returned &&
        f.receiver->alive && f.queue.count == 1 && f.released == 1 && !f.queue.processing &&
        !f.queue.custom_callback_open && f.queue.active_borrow == NULL && f.game->callback_depth == 0);
    f.wait_on = 0;
    VERIFY(&f, checked(&f, &erase) == ISLE_GAME_OK && f.calls == 2 && f.trace[1] == 8 &&
        f.queue.count == 0 && f.released == 2);
    return finish(&f);
}

int owned_command_legacy_fault_regression(void) {
    CommandFixture f;
    if (!initialize(&f)) return f.failure;
    send(&f, 7); send(&f, 8);
    f.fault_on = 7;
    start_legacy(&f);
    VERIFY(&f, isle_game_tick(f.game) == ISLE_GAME_EXECUTION_FAILURE);
    VERIFY(&f, !f.aborts && !f.dispatch_returned && !f.after_fault && f.calls == 1 &&
        f.queue.processing && f.queue.custom_callback_open && f.queue.active_borrow != NULL &&
        f.queue.count == 2 && !f.released);
    VERIFY(&f, isle_game_delete_object(f.game, f.caller) == ISLE_GAME_EXECUTION_FAILURE && f.joined == 1 &&
        f.join_depth_before == f.join_depth_after && f.queue.count == 1 && f.released == 1 &&
        !f.queue.processing && !f.queue.custom_callback_open);
    return finish(&f);
}
