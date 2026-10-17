#ifndef ISLE_SOURCE_FACTORY_RESOURCES_H
#define ISLE_SOURCE_FACTORY_RESOURCES_H

#include "35mmisle/engine.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Private Source factory admission, not a second OM or rollback transaction.
 * A NULL descriptor object means the future prepared object. All continuations
 * are really parked before success; no node/process/model/live count is exposed
 * until its corresponding publish call. The fixed bound is the exact Item
 * cohort: animation, configured update, delayed show, optional old removal. */
enum { ISLE_SOURCE_FACTORY_PROCESS_CAPACITY = 4 };
typedef struct IsleSourceFactoryProcess {
    IsleGameObject* object;
    IsleGameSourceCallback function;
    IsleGameProcessKind kind;
    uint32_t priority;
} IsleSourceFactoryProcess;

typedef struct IsleSourceFactoryResources {
    IsleGame* game;
    IsleGameObject* object;
    IsleGameModel* model;
    IsleGameProcess* processes[ISLE_SOURCE_FACTORY_PROCESS_CAPACITY];
    IsleSourceFactoryProcess descriptors[ISLE_SOURCE_FACTORY_PROCESS_CAPACITY];
    size_t nodes;
    size_t process_count;
    uint32_t published_processes;
    uint8_t object_published;
    uint8_t model_published;
    uint8_t active;
} IsleSourceFactoryResources;

/* Caller-owned stable zero-initialized record, one nonwaiting admission.
 * Preparation failure preserves exact status and cancels only unpublished
 * resources. If cancellation cannot really join, retained active ownership must
 * remain visible to the caller; never clear a ticket over an unjoined worker.
 * Game destruction rejects outstanding factory reservations. */
IsleGameResult isle_source_factory_prepare(IsleGame*, size_t nodes,
    const IsleSourceFactoryProcess*, size_t process_count,
    IsleSourceFactoryResources*);
IsleGameResult isle_source_factory_publish_object(IsleSourceFactoryResources*,
    uint32_t id, uint8_t link, uint32_t priority, IsleGameCallback update,
    void* user, IsleGameObject**);
IsleGameResult isle_source_factory_attach_model(IsleSourceFactoryResources*,
    IsleGameObject*, const IsleGameModelNodeInit*, size_t, IsleGameModel**);
IsleGameResult isle_source_factory_publish_process(IsleSourceFactoryResources*,
    IsleGameObject*, IsleGameSourceCallback, IsleGameProcessKind,
    uint32_t priority, IsleGameProcess**);
/* Cancel never erases a published object/model/process. Finish requires every
 * planned resource consumed; both release the admission record only after its
 * actual remaining unpublished ownership is gone. Neither clears scene fault. */
IsleGameResult isle_source_factory_cancel(IsleSourceFactoryResources*);
IsleGameResult isle_source_factory_finish(IsleSourceFactoryResources*);

#ifdef __cplusplus
}
#endif
#endif
