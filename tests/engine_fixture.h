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
void engine_bind_actor_payload(IsleGameObject* object, int* payload);
void engine_actor_context_update(IsleGameObject* object, void* user);
IsleGameResult engine_replace_actor_update(IsleGame* game, IsleGameObject* object, int* context);
void engine_bind_source_payload(IsleGameObject* object, EngineFixture* fixture);
void engine_source_a(IsleGameObject* object);
void engine_source_b(IsleGameObject* object);
void engine_source_retired(IsleGameObject* object, void* user);
void engine_source_delete(IsleGameObject* object);
void engine_observe_retiring(IsleGameObject* object, void* user);
void engine_observe_joined(IsleGameObject* object, void* user);
void engine_direct_delete(IsleGameObject* object);
IsleGameResult engine_delete_corrupt_payload(IsleGameObject* object, int mode);
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
