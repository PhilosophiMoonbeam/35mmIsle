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
IsleGameResult engine_attach_model_with_invalid_kind(IsleGame* game, IsleGameObject* owner,
                                                     const IsleGameModelNodeInit nodes[2],
                                                     IsleGameModel** model);

#ifdef __cplusplus
}
#endif

#endif
