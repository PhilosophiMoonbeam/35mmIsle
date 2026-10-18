#ifndef ISLE_ENGINE_FIXTURE_H
#define ISLE_ENGINE_FIXTURE_H

#include "35mmisle/engine.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct EngineFixture {
    IsleGame* game;
    int events[512];
    size_t count;
    int failures;
    uint64_t local_result;
    IsleGameProcess* victim;
    IsleGameProcess* child;
    int shared_value;
    int retired_continued;
    int after_finish;
    int mode;
    int retire_calls;
    int pre_retire_calls;
    IsleGameModel* model;
    IsleGameProcess* observed_processes[4];
    size_t observed_process_count;
    size_t waiting_processes;
} EngineFixture;

typedef struct EngineRecord {
    EngineFixture* fixture;
    int value;
} EngineRecord;

typedef struct EngineIdentityFixture {
    EngineFixture retirement;
    IsleGameObject* held;
    IsleGameObject* target;
    IsleGameCommandPool pool;
    IsleGameCommandQueue nested;
    size_t release_rejections;
    size_t retain_successes;
    size_t deliveries;
} EngineIdentityFixture;

void engine_record_object(IsleGameObject* object, void* user);
void engine_record_process(IsleGameObject* object, void* user);
void engine_nested_process(IsleGameObject* object, void* user);
void engine_victim(IsleGameObject* object, void* user);
void engine_parent(IsleGameObject* object, void* user);
void engine_end_synchronous(IsleGameObject* object, void* user);
void engine_delete_self(IsleGameObject* object, void* user);
void engine_invalid_legacy_wait(IsleGameObject* object, void* user);
IsleGameResult engine_add_camera_matrix_with_invalid_kind(IsleGame* game, IsleGameCamera* camera);
IsleGameResult engine_attach_model_with_invalid_kind(IsleGame* game, IsleGameObject* owner,
                                                     const IsleGameModelNodeInit nodes[2],
                                                     IsleGameModel** model);
IsleGameResult engine_set_invalid_placement(IsleGame* game, IsleGameModel* model);
void engine_model_use_final_incarnation(IsleGame*);
int engine_model_free_storage_matches(const IsleGame*, const IsleGameObject*, const IsleGameModel*);
int engine_vector_normalize_regression(void);
int engine_vector_alias_regression(void);
int engine_vector_rotation_regression(void);
void engine_bind_actor_payload(IsleGameObject* object, int* payload);
void engine_actor_context_update(IsleGameObject* object, void* user);
IsleGameResult engine_replace_actor_update(IsleGame* game, IsleGameObject* object, int* context);
void engine_bind_source_payload(IsleGameObject* object, EngineFixture* fixture);
void engine_source_a(IsleGameObject* object);
void engine_source_b(IsleGameObject* object);
void engine_source_stop_update(IsleGameObject* object);
void engine_native_reject_stop_update(IsleGameObject* object, void* user);
void engine_source_reject_nested_stop(IsleGameObject* object);
void engine_source_wait_once(IsleGameObject* object);
void engine_native_reject_source_wait(IsleGameObject* object, void* user);
void engine_source_reject_nested_wait(IsleGameObject* object);
void engine_source_pause_pulse(IsleGameObject* object);
void engine_native_source_pause_pulse(IsleGameObject* object, void* user);
void engine_source_pause_wait(IsleGameObject* object);
void engine_source_pause_self(IsleGameObject* object);
void engine_source_pause_self_synchronous(IsleGameObject* object);
void engine_source_pause_update(IsleGameObject* object, void* user);
void engine_source_retired(IsleGameObject* object, void* user);
void engine_source_delete(IsleGameObject* object);
void engine_observe_retiring(IsleGameObject* object, void* user);
void engine_observe_joined(IsleGameObject* object, void* user);
void engine_direct_delete(IsleGameObject* object);
IsleGameResult engine_delete_corrupt_payload(IsleGameObject* object, int mode);
void engine_identity_retiring(IsleGameObject*, void*);
void engine_identity_joined(IsleGameObject*, void*);
void engine_identity_release_callback(IsleGameObject*, void*);
void engine_identity_retain_process(IsleGameObject*, void*);
void engine_identity_clear_target(IsleGameObject*, int32_t, IsleGameObject*, void*);
void engine_identity_nested_command(IsleGameObject*, int32_t, IsleGameObject*, void*);
void engine_identity_fault_update(IsleGameObject*, void*);
void engine_identity_wait_process(IsleGameObject*, void*);
IsleGameResult engine_identity_notify_queued(IsleGameObject*, EngineIdentityFixture*);
IsleGameResult engine_identity_retain_at_limit(IsleGame*, IsleGameObject*);
/* The selected game ABI uses this wait entry. It must reject synchronous use
 * without returning into the invalid callback or crossing a C++ exception. */
void ohWait(int32_t updates);
/* Observe actual C-owned records without exposing C-only layouts to C++ tests. */
int engine_object_alive(const IsleGameObject*);
int engine_model_alive(const IsleGameModel*);
int engine_process_alive(const IsleGameProcess*);
int engine_process_has_continuation(const IsleGameProcess*);
int engine_process_waiting(const IsleGameProcess*);
int engine_process_joined(const IsleGameProcess*);
void* engine_object_update_context(const IsleGameObject*);
void engine_set_actor_payload(IsleGameObject*, void*);
size_t engine_object_process_count(const IsleGameObject*);
int engine_model_before_join(const IsleGameObject*, const IsleGameModel*, const void*, const void*, float);
int engine_model_after_join(const IsleGameObject*, const IsleGameModel*, const void*, const void*);
void engine_delete_object_direct(IsleGameObject*);

#ifdef __cplusplus
}
#endif

#endif
