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
void engine_bind_actor_payload(IsleGameObject* object, int* payload);
void engine_actor_context_update(IsleGameObject* object, void* user);
IsleGameResult engine_replace_actor_update(IsleGame* game, IsleGameObject* object, int* context);
void engine_bind_source_payload(IsleGameObject* object, EngineFixture* fixture);
void engine_source_a(IsleGameObject* object);
void engine_source_b(IsleGameObject* object);
void engine_source_retired(IsleGameObject* object, void* user);

#ifdef __cplusplus
}
#endif

#endif
