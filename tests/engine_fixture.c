#include "engine_fixture.h"
#include "../src/engine_internal.h"
#include "../src/menu_scene_bridge.h"
#include "../src/player_source.h"


static void record(EngineFixture* fixture, int value) {
    if (fixture->count >= sizeof(fixture->events) / sizeof(fixture->events[0])) {
        ++fixture->failures;
        return;
    }
    fixture->events[fixture->count++] = value;
}

static void require_ok(EngineFixture* fixture, IsleGameResult result) {
    if (result != ISLE_GAME_OK) ++fixture->failures;
}

void engine_record_object(IsleGameObject* object, void* user) {
    EngineFixture* fixture = user;
    uint32_t id = 0;
    (void) object;
    if (isle_game_object_id(fixture->game, NULL, &id) != ISLE_GAME_OK) {
        ++fixture->failures;
        return;
    }
    record(fixture, (int) id);
}

void engine_record_process(IsleGameObject* object, void* user) {
    EngineRecord* entry = user;
    (void) object;
    record(entry->fixture, entry->value);
}

static void nested_wait(EngineFixture* fixture, uint64_t value) {
    uint64_t nested = value + UINT64_C(17);
    record(fixture, 300);
    require_ok(fixture, isle_game_wait(fixture->game, 2));
    fixture->local_result = nested;
    record(fixture, 301);
}

void engine_nested_process(IsleGameObject* object, void* user) {
    EngineFixture* fixture = user;
    uint64_t local = UINT64_C(0x1234567887654321);
    (void) object;
    record(fixture, 200);
    require_ok(fixture, isle_game_wait(fixture->game, 0));
    record(fixture, 201);
    require_ok(fixture, isle_game_wait(fixture->game, 1));
    record(fixture, 202);
    nested_wait(fixture, local);
    record(fixture, 203);
}

void engine_victim(IsleGameObject* object, void* user) {
    EngineFixture* fixture = user;
    (void) object;
    record(fixture, 700);
    require_ok(fixture, isle_game_wait(fixture->game, 100));
    ++fixture->retired_continued;
}

static void child_entry(IsleGameObject* object, void* user) {
    EngineFixture* fixture = user;
    (void) object;
    if (fixture->shared_value != 99) ++fixture->failures;
    record(fixture, 900);
    require_ok(fixture, isle_game_wait(fixture->game, 1));
    record(fixture, 901);
}

void engine_parent(IsleGameObject* object, void* user) {
    EngineFixture* fixture = user;
    record(fixture, 800);
    require_ok(fixture, isle_game_end_process(fixture->game, fixture->victim));
    fixture->victim = NULL;
    fixture->shared_value = 99;
    record(fixture, 801);
    require_ok(fixture, isle_game_add_process(fixture->game, object, child_entry, fixture,
                                            ISLE_GAME_STACKFUL, 5, &fixture->child));
    require_ok(fixture, isle_game_end_process(fixture->game, NULL));
    ++fixture->after_finish;
}

void engine_end_synchronous(IsleGameObject* object, void* user) {
    EngineFixture* fixture = user;
    (void) object;
    record(fixture, 20);
    require_ok(fixture, isle_game_end_process(fixture->game, NULL));
    record(fixture, 21);
}

void engine_delete_self(IsleGameObject* object, void* user) {
    EngineFixture* fixture = user;
    (void) object;
    record(fixture, 30);
    require_ok(fixture, isle_game_delete_object(fixture->game, NULL));
    if (fixture->pre_retire_calls != 0 || fixture->retire_calls != 0) ++fixture->failures;
    record(fixture, 31);
    if (fixture->mode == 0) {
        require_ok(fixture, isle_game_wait(fixture->game, 1));
        ++fixture->retired_continued;
    }
}

void engine_invalid_legacy_wait(IsleGameObject* object, void* user) {
    EngineFixture* fixture = user;
    (void) object;
    record(fixture, 60);
    ohWait(fixture->mode == 1 ? -1 : 1);
    ++fixture->retired_continued;
}

IsleGameResult engine_attach_model_with_invalid_kind(IsleGame* game, IsleGameObject* owner,
                                                     const IsleGameModelNodeInit nodes[2],
                                                     IsleGameModel** model) {
    IsleGameModelNodeInit invalid[2] = {nodes[0], nodes[1]};
    /* An integer outside the named enum values is a valid C ABI input.
     * Construct it here: the equivalent C++ out-of-range enum cast is not. */
    invalid[1].matrix_kind = (IsleGameModelMatrixKind) 99;
    return isle_game_attach_model(game, owner, invalid, 2, model);
}

IsleGameResult engine_set_invalid_placement(IsleGame* game, IsleGameModel* model) {
    return isle_game_model_set_placement(game, model, (IsleGameModelPlacementMode) 99, NULL);
}
void engine_model_use_final_incarnation(IsleGame* game) {
    game->model_incarnation = UINT64_MAX - 1;
}

int engine_model_free_storage_matches(const IsleGame* game, const IsleGameObject* object,
    const IsleGameModel* model) {
    return game->free_objects == object && game->free_models == model &&
        game->factory_objects == NULL;
}

IsleGameResult engine_add_camera_matrix_with_invalid_kind(IsleGame* game, IsleGameCamera* camera) {
    /* Exercise an unknown integer at the C ABI, not an out-of-range C++ enum. */
    return isle_game_camera_add_matrix(game, camera, (IsleGameCameraMatrixKind) 99);
}

void engine_bind_actor_payload(IsleGameObject* object, int* payload) {
    object->userData = payload;
}

void engine_actor_context_update(IsleGameObject* object, void* user) {
    ++*(int*) user;
    int* payload = object->userData;
    *payload += 10;
    if (*payload == 120) omDeleteGObj(object);
}

IsleGameResult engine_replace_actor_update(IsleGame* game, IsleGameObject* object, int* context) {
    return isle_menu_set_update(game, object, engine_actor_context_update, context);
}

void engine_bind_source_payload(IsleGameObject* object, EngineFixture* fixture) {
    object->userData = fixture;
}

void engine_source_stop_update(IsleGameObject* object) {
    EngineFixture* fixture = object->userData;
    record(fixture, 480);
    fixture->local_result = isle_game_stop_update(fixture->game);
    ++fixture->after_finish;
    record(fixture, 481);
}
void engine_native_reject_stop_update(IsleGameObject* object, void* user) {
    EngineFixture* fixture = user;
    (void)object;
    if (isle_game_stop_update(fixture->game) != ISLE_GAME_INVALID_STATE) ++fixture->failures;
    record(fixture, 479);
}
static void nested_stop(IslePlayerSource* source, void* user) {
    EngineFixture* fixture = user;
    if (isle_game_stop_update(source->game) != ISLE_GAME_INVALID_STATE) ++fixture->failures;
    record(fixture, 483);
}
void engine_source_reject_nested_stop(IsleGameObject* object) {
    EngineFixture* fixture = object->userData;
    IslePlayerSource* source = object->game->playerSource;
    require_ok(fixture, isle_player_source_call(source,nested_stop,fixture));
    if (source->nonwaiting_call_frame != NULL) ++fixture->failures;
    record(fixture, 484);
}

void engine_source_wait_once(IsleGameObject* object) {
    EngineFixture* fixture = object->userData;
    record(fixture, 500);
    fixture->local_result = isle_game_source_wait(fixture->game, 1);
    record(fixture, 501);
}
void engine_native_reject_source_wait(IsleGameObject* object, void* user) {
    EngineFixture* fixture = user;
    (void)object;
    if (isle_game_source_wait(fixture->game, 1) != ISLE_GAME_INVALID_STATE) ++fixture->failures;
    record(fixture, 502);
}
static void nested_source_wait(IslePlayerSource* source, void* user) {
    EngineFixture* fixture = user;
    if (isle_game_source_wait(source->game, 1) != ISLE_GAME_INVALID_STATE) ++fixture->failures;
    require_ok(fixture, isle_game_source_wait(source->game, 0));
    record(fixture, 504);
}
void engine_source_reject_nested_wait(IsleGameObject* object) {
    EngineFixture* fixture = object->userData;
    require_ok(fixture, isle_player_source_call(object->game->playerSource,nested_source_wait,fixture));
    record(fixture, 505);
}

void engine_source_a(IsleGameObject* object) {
    EngineFixture* fixture = object->userData;
    IsleGameSourceCallback function = NULL;
    uint64_t local = (uint64_t) fixture->shared_value + UINT64_C(17);
    require_ok(fixture, isle_game_process_source_function(fixture->game, NULL, &function));
    if (function != engine_source_a) ++fixture->failures;
    record(fixture, 410);
    ohWait(fixture->mode == 1 ? 100 : 1);
    if (fixture->mode == 1) {
        ++fixture->retired_continued;
        return;
    }
    fixture->local_result = local;
    record(fixture, 411);
    require_ok(fixture, isle_game_add_source_process(fixture->game, NULL, engine_source_b,
        ISLE_GAME_STACKFUL, 5, &fixture->child));
    require_ok(fixture, isle_game_end_source_processes(fixture->game, NULL, engine_source_a));
    ++fixture->after_finish;
}

void engine_source_b(IsleGameObject* object) {
    EngineFixture* fixture = object->userData;
    IsleGameSourceCallback function = NULL;
    require_ok(fixture, isle_game_process_source_function(fixture->game, NULL, &function));
    if (function != engine_source_b) ++fixture->failures;
    record(fixture, 420);
    if (fixture->mode == 1) return;
    if (fixture->mode == 3) {
        require_ok(fixture, isle_game_end_source_processes(fixture->game, NULL, engine_source_b));
        record(fixture, 422);
        return;
    }
    ohWait(1);
    record(fixture, 421);
    require_ok(fixture, isle_game_delete_object(fixture->game, NULL));
    record(fixture, 422);
    ohWait(1);
    ++fixture->retired_continued;
}

void engine_source_pause_pulse(IsleGameObject* object) {
    EngineFixture* fixture = object->userData;
    record(fixture, 460);
}

void engine_native_source_pause_pulse(IsleGameObject* object, void* user) {
    EngineFixture* fixture = user;
    IsleGameSourceCallback function = engine_source_pause_pulse;
    require_ok(fixture, isle_game_process_source_function(fixture->game, NULL, &function));
    if (function != NULL || object->userData != fixture) ++fixture->failures;
    record(fixture, 461);
    engine_source_pause_pulse(object);
}

void engine_source_pause_wait(IsleGameObject* object) {
    EngineFixture* fixture = object->userData;
    IsleGameSourceCallback function = NULL;
    uint64_t local = (uint64_t) fixture->shared_value + UINT64_C(17);
    require_ok(fixture, isle_game_process_source_function(fixture->game, NULL, &function));
    if (function != engine_source_pause_wait) ++fixture->failures;
    record(fixture, 470);
    ohWait(2);
    fixture->local_result = local;
    record(fixture, 471);
    ohWait(1);
    ++fixture->after_finish;
    record(fixture, 472);
}

void engine_source_pause_self(IsleGameObject* object) {
    EngineFixture* fixture = object->userData;
    uint64_t local = (uint64_t) fixture->shared_value + UINT64_C(23);
    record(fixture, 480);
    require_ok(fixture, isle_game_pause_source_processes(fixture->game, NULL,
        engine_source_pause_self, -7));
    record(fixture, 481);
    ohWait(2);
    fixture->local_result = local;
    ++fixture->after_finish;
    record(fixture, 482);
}

void engine_source_pause_self_synchronous(IsleGameObject* object) {
    EngineFixture* fixture = object->userData;
    record(fixture, 483);
    if (++fixture->shared_value == 1) {
        require_ok(fixture, isle_game_pause_source_processes(fixture->game, NULL,
            engine_source_pause_self_synchronous, 1));
    }
    record(fixture, 484);
}

void engine_source_pause_update(IsleGameObject* object, void* user) {
    EngineFixture* fixture = user;
    (void) object;
    if (fixture->mode == 0) return;
    require_ok(fixture, isle_game_pause_source_processes(fixture->game, NULL,
        engine_source_pause_pulse, fixture->mode == 1));
    record(fixture, fixture->mode == 1 ? 490 : 491);
    fixture->mode = 0;
}

void engine_source_retired(IsleGameObject* object, void* user) {
    EngineFixture* fixture = user;
    IsleGameCounts counts = {0};
    if (object->userData != fixture) ++fixture->failures;
    require_ok(fixture, isle_game_counts(fixture->game, &counts));
    if (counts.processes != 0) ++fixture->failures;
    ++fixture->retire_calls;
    record(fixture, 450);
}

void engine_source_delete(IsleGameObject* object) {
    EngineFixture* fixture = object->userData;
    record(fixture, 30);
    omDeleteGObj(object);
    if (fixture->pre_retire_calls != 0 || fixture->retire_calls != 0) ++fixture->failures;
    record(fixture, 31);
    ohWait(1);
    ++fixture->retired_continued;
}

void engine_observe_retiring(IsleGameObject* object, void* user) {
    EngineFixture* fixture = user;
    size_t processes = 0, waiting = 0;
    if (!object->alive || !object->retirementStarted || object->userData != fixture ||
        object->type != 1 || object->data.model != fixture->model ||
        !fixture->model->alive || fixture->model->obj != object ||
        fixture->pre_retire_calls != 0 || fixture->retire_calls != 0) ++fixture->failures;
    for (GObjProcess* process = object->processListHead; process != NULL; process = process->next)
        ++processes;
    if (processes != fixture->observed_process_count) ++fixture->failures;
    for (size_t i = 0; i < fixture->observed_process_count; ++i) {
        GObjProcess* process = fixture->observed_processes[i];
        if (!process->alive || process->object != object) ++fixture->failures;
        if (process->kind == ISLE_GAME_STACKFUL) {
            IsleContinuationState state = ISLE_CONT_READY;
            if (process->continuation == NULL ||
                isle_continuation_state(process->continuation, &state) != ISLE_CONT_OK)
                ++fixture->failures;
            if (state == ISLE_CONT_WAITING) ++waiting;
        }
    }
    if (waiting != fixture->waiting_processes) ++fixture->failures;
    ++fixture->pre_retire_calls;
    record(fixture, 451);
}

void engine_observe_joined(IsleGameObject* object, void* user) {
    EngineFixture* fixture = user;
    if (object->alive || object->type != 0 || object->data.any != NULL ||
        object->processListHead != NULL || object->processListTail != NULL ||
        object->userData != fixture || fixture->model->alive ||
        fixture->pre_retire_calls != 1 || fixture->retire_calls != 0) ++fixture->failures;
    for (size_t i = 0; i < fixture->observed_process_count; ++i) {
        GObjProcess* process = fixture->observed_processes[i];
        if (process->alive || process->continuation != NULL) ++fixture->failures;
    }
    ++fixture->retire_calls;
    record(fixture, 452);
}

static void identity_release_rejected(EngineIdentityFixture* fixture) {
    if (isle_game_object_release_identity(fixture->retirement.game, fixture->held) !=
        ISLE_GAME_INVALID_STATE) ++fixture->retirement.failures;
    else ++fixture->release_rejections;
}

void engine_identity_retiring(IsleGameObject* object, void* user) {
    EngineIdentityFixture* fixture = user;
    engine_observe_retiring(object, &fixture->retirement);
    identity_release_rejected(fixture);
    if (isle_game_object_retain_identity(fixture->retirement.game, object) !=
        ISLE_GAME_INVALID_STATE ||
        isle_game_destroy(fixture->retirement.game) != ISLE_GAME_INVALID_STATE)
        ++fixture->retirement.failures;
}

void engine_identity_joined(IsleGameObject* object, void* user) {
    EngineIdentityFixture* fixture = user;
    engine_observe_joined(object, &fixture->retirement);
    identity_release_rejected(fixture);
    if (isle_game_object_retain_identity(fixture->retirement.game, object) !=
        ISLE_GAME_INVALID_ARGUMENT ||
        isle_game_destroy(fixture->retirement.game) != ISLE_GAME_INVALID_STATE)
        ++fixture->retirement.failures;
}

void engine_identity_release_callback(IsleGameObject* object, void* user) {
    EngineIdentityFixture* fixture = user;
    (void) object;
    identity_release_rejected(fixture);
}

void engine_identity_retain_process(IsleGameObject* object, void* user) {
    EngineIdentityFixture* fixture = user;
    require_ok(&fixture->retirement, isle_game_object_retain_identity(fixture->retirement.game, object));
    ++fixture->retain_successes;
    identity_release_rejected(fixture);
    record(&fixture->retirement, 710);
    require_ok(&fixture->retirement, isle_game_wait(fixture->retirement.game, 100));
    ++fixture->retirement.retired_continued;
}

void engine_identity_clear_target(IsleGameObject* object, int32_t command,
                                 IsleGameObject* source, void* user) {
    EngineIdentityFixture* fixture = user;
    (void) object;
    if (command != 22 || source != fixture->held) ++fixture->retirement.failures;
    if (fixture->target == source) fixture->target = NULL;
    ++fixture->deliveries;
}

void engine_identity_nested_command(IsleGameObject* object, int32_t command,
                                   IsleGameObject* source, void* user) {
    EngineIdentityFixture* fixture = user;
    int delete_requested = 0;
    (void) source;
    identity_release_rejected(fixture);
    if (command == 1) {
        require_ok(&fixture->retirement, isle_game_commands(fixture->retirement.game, object,
            &fixture->pool, &fixture->nested, engine_identity_nested_command, fixture, &delete_requested));
        if (delete_requested) ++fixture->retirement.failures;
        identity_release_rejected(fixture);
    }
}

static void identity_queued(IsleGameObject* object, int32_t command,
                            IsleGameObject* source, void* user) {
    EngineIdentityFixture* fixture = user;
    (void) object;
    (void) command;
    (void) source;
    identity_release_rejected(fixture);
    if (isle_game_object_retain_identity(fixture->retirement.game, fixture->target) !=
        ISLE_GAME_INVALID_STATE) ++fixture->retirement.failures;
}

IsleGameResult engine_identity_notify_queued(IsleGameObject* object, EngineIdentityFixture* fixture) {
    return isle_menu_observe_command_queued(fixture->retirement.game, object, 22, fixture->held,
        identity_queued, fixture);
}

static void identity_fault_command(IsleGameObject* object, int32_t command,
                                   IsleGameObject* source, void* user) {
    EngineIdentityFixture* fixture = user;
    (void) object;
    (void) command;
    (void) source;
    identity_release_rejected(fixture);
    ohWait(fixture->retirement.mode == 1 ? -1 : 1);
    ++fixture->retirement.failures;
}

void engine_identity_fault_update(IsleGameObject* object, void* user) {
    EngineIdentityFixture* fixture = user;
    int delete_requested = 0;
    (void) isle_game_commands(fixture->retirement.game, object, &fixture->pool, &fixture->nested,
        identity_fault_command, fixture, &delete_requested);
    ++fixture->retirement.failures;
}

static void identity_wait_command(IsleGameObject* object, int32_t command,
                                  IsleGameObject* source, void* user) {
    EngineIdentityFixture* fixture = user;
    (void) object;
    (void) command;
    (void) source;
    identity_release_rejected(fixture);
    ++fixture->retirement.shared_value;
    require_ok(&fixture->retirement, isle_game_wait(fixture->retirement.game, 1));
    identity_release_rejected(fixture);
    ++fixture->retirement.after_finish;
}

void engine_identity_wait_process(IsleGameObject* object, void* user) {
    EngineIdentityFixture* fixture = user;
    int delete_requested = 0;
    require_ok(&fixture->retirement, isle_game_commands(fixture->retirement.game, object,
        &fixture->pool, &fixture->nested, identity_wait_command, fixture, &delete_requested));
    if (delete_requested) ++fixture->retirement.failures;
}

IsleGameResult engine_identity_retain_at_limit(IsleGame* game, IsleGameObject* object) {
    u32 holds = object->identityHolds;
    object->identityHolds = UINT32_MAX;
    IsleGameResult result = isle_game_object_retain_identity(game, object);
    object->identityHolds = holds;
    return result;
}

void engine_direct_delete(IsleGameObject* object) {
    omDeleteGObj(object);
}

IsleGameResult engine_delete_corrupt_payload(IsleGameObject* object, int mode) {
    IsleGameModel* model = object->data.model;
    EngineFixture* fixture = object->userData;
    IsleGame* game = object->game;
    if (mode == 0) object->type = 4;
    if (mode == 1) object->type = 0; /* Non-null payload is invalid for type zero. */
    if (mode == 2) model->alive = false;
    if (mode == 3) model->game = NULL;
    if (mode == 4) model->obj = NULL;
    IsleGameResult result = isle_game_delete_object(game, object);
    if (!object->alive || object->retirementStarted || fixture->pre_retire_calls != 0 ||
        fixture->retire_calls != 0) ++fixture->failures;
    /* Restore only fixture-owned corruption so normal fault destruction can
     * reclaim its real model and parked continuation; no production recovery. */
    object->type = 1;
    model->alive = true;
    model->game = game;
    model->obj = object;
    return result;
}

int engine_object_alive(const IsleGameObject* object) { return object != NULL && object->alive; }
int engine_model_alive(const IsleGameModel* model) { return model != NULL && model->alive; }
int engine_process_alive(const IsleGameProcess* process) { return process != NULL && process->alive; }
int engine_process_has_continuation(const IsleGameProcess* process) {
    return process != NULL && process->continuation != NULL;
}
int engine_process_waiting(const IsleGameProcess* process) {
    IsleContinuationState state = ISLE_CONT_READY;
    return engine_process_alive(process) && process->continuation != NULL &&
        isle_continuation_state(process->continuation, &state) == ISLE_CONT_OK && state == ISLE_CONT_WAITING;
}
int engine_process_joined(const IsleGameProcess* process) {
    return process != NULL && !process->alive && process->continuation == NULL;
}
void* engine_object_update_context(const IsleGameObject* object) { return object->updateUser; }
void engine_set_actor_payload(IsleGameObject* object, void* payload) { object->userData = payload; }
size_t engine_object_process_count(const IsleGameObject* object) {
    size_t count = 0;
    for (const GObjProcess* process = object->processListHead; process != NULL; process = process->next) ++count;
    return count;
}
int engine_model_before_join(const IsleGameObject* object, const IsleGameModel* model,
                            const void* payload, const void* context, float root_y) {
    return object->alive && object->retirementStarted && object->userData == payload &&
        object->updateUser == context && object->type == 1 && object->data.model == model &&
        model->alive && model->obj == object && model->nodes[0].transform.position.y == root_y;
}
int engine_model_after_join(const IsleGameObject* object, const IsleGameModel* model,
                           const void* payload, const void* context) {
    return !object->alive && object->type == 0 && object->data.any == NULL && !model->alive &&
        object->processListHead == NULL && object->processListTail == NULL &&
        object->userData == payload && object->updateUser == context;
}
void engine_delete_object_direct(IsleGameObject* object) { omDeleteGObj(object); }

int engine_vector_normalize_regression(void) {
    Vec3f zero = {-0.0f, 0.0f, -0.0f};
    if (Vec3fNormalize(&zero) != 0.0f ||
        !signbit(zero.x) || signbit(zero.y) || !signbit(zero.z)) return 1;
    Vec3f tiny = {FLT_TRUE_MIN, -FLT_TRUE_MIN, -0.0f};
    if (Vec3fNormalize(&tiny) != 0.0f || tiny.x != FLT_TRUE_MIN ||
        tiny.y != -FLT_TRUE_MIN || !signbit(tiny.z)) return 2;
    Vec3f reciprocal = {2.0f, 3.0f, 6.0f};
    if (Vec3fNormalize(&reciprocal) != 7.0f ||
        reciprocal.x != 2.0f * (1.0f / 7.0f) ||
        reciprocal.y != 3.0f * (1.0f / 7.0f) ||
        reciprocal.z != 6.0f * (1.0f / 7.0f)) return 3;
    Vec3f overflow = {FLT_MAX, 1.0f, -1.0f};
    if (!isinf(Vec3fNormalize(&overflow)) || overflow.x != 0.0f ||
        overflow.y != 0.0f || overflow.z != 0.0f || !signbit(overflow.z)) return 4;
    return 0;
}

int engine_vector_alias_regression(void) {
    Vec3f left = {2.0f, -3.0f, 5.0f}, right = {-7.0f, 11.0f, 13.0f};
    if (Vec3fDiff(&left, &left, &right) != &left ||
        left.x != 9.0f || left.y != -14.0f || left.z != -8.0f) return 1;
    left = (Vec3f) {2.0f, -3.0f, 5.0f};
    if (Vec3fDiff(&right, &left, &right) != &right ||
        right.x != 9.0f || right.y != -14.0f || right.z != -8.0f) return 2;
    if (Vec3fDiff(&right, &right, &right) != &right ||
        right.x != 0.0f || right.y != 0.0f || right.z != 0.0f) return 3;
    if (Vec3fAdd(&left, &left) != &left ||
        left.x != 4.0f || left.y != -6.0f || left.z != 10.0f) return 4;
    if (Vec3fAdd(&right, &left) != &right ||
        right.x != 4.0f || right.y != -6.0f || right.z != 10.0f) return 5;
    return 0;
}

int engine_vector_rotation_regression(void) {
    const float quarter_turn = 1.5707963267948966f;
    const enum VectorAxisFlags axes[3] = {AXIS_X, AXIS_Y, AXIS_Z};
    const Vec3f expected[3] = {{2.0f, -5.0f, 3.0f},
                               {5.0f, 3.0f, -2.0f},
                               {-3.0f, 2.0f, 5.0f}};
    const Vec3f basis[3] = {{1.0f, 0.0f, 0.0f},
                           {0.0f, 1.0f, 0.0f},
                           {0.0f, 0.0f, 1.0f}};
    for (unsigned i = 0; i < 3; ++i) {
        Vec3f euler = {2.0f, 3.0f, 5.0f}, arbitrary = euler, axis = basis[i];
        if (Vec3fGetEulerRotation(&euler, axes[i], quarter_turn) != &euler ||
            Vec3f_func_8001A8B8(&arbitrary, &axis, quarter_turn) != &arbitrary) return 1;
        if (fabsf(euler.x - expected[i].x) > 0.000002f ||
            fabsf(euler.y - expected[i].y) > 0.000002f ||
            fabsf(euler.z - expected[i].z) > 0.000002f ||
            fabsf(arbitrary.x - expected[i].x) > 0.000002f ||
            fabsf(arbitrary.y - expected[i].y) > 0.000002f ||
            fabsf(arbitrary.z - expected[i].z) > 0.000002f) return 2;
    }
    Vec3f composed = {2.0f, 3.0f, 5.0f};
    Vec3fGetEulerRotation(&composed, AXIS_X, quarter_turn);
    Vec3fGetEulerRotation(&composed, AXIS_Y, quarter_turn);
    if (fabsf(composed.x - 3.0f) > 0.000002f ||
        fabsf(composed.y + 5.0f) > 0.000002f ||
        fabsf(composed.z + 2.0f) > 0.000002f) return 3;
    /* GAME does not normalize or replace a degenerate arbitrary axis. */
    Vec3f value = {2.0f, 3.0f, 5.0f}, axis = {0.0f, 2.0f, 0.0f};
    Vec3f_func_8001A8B8(&value, &axis, 0.0f);
    if (value.x != 8.0f || value.y != 12.0f || value.z != 5.0f) return 4;
    value = (Vec3f) {2.0f, 3.0f, 5.0f};
    axis = (Vec3f) {0.0f, 0.0f, 0.0f};
    Vec3f_func_8001A8B8(&value, &axis, 0.0f);
    if (value.x != 0.0f || value.y != 3.0f || value.z != 0.0f) return 5;
    axis = (Vec3f) {0.6f, 0.0f, 0.8f};
    if (Vec3f_func_8001A8B8(&axis, &axis, 0.37f) != &axis ||
        fabsf(axis.x - 0.6f) > 0.000002f ||
        fabsf(axis.y) > 0.000002f || fabsf(axis.z - 0.8f) > 0.000002f) return 6;
    return 0;
}
