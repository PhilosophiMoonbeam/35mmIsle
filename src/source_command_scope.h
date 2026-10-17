#ifndef ISLE_SOURCE_COMMAND_SCOPE_H
#define ISLE_SOURCE_COMMAND_SCOPE_H
#include "35mmisle/engine.h"

/* Borrowed native bookkeeping, not a second queue or a stack-owned session.
 * Only the real synchronous object-update boundary may begin this scope.
 * Services retains the stable context through normal completion or the native
 * C fault-abort hook. No C++ frame encloses the C interpreter. */
typedef struct IsleSourceCommandScope {
    IsleGameCommandPool* pool;
    IsleGameCommandQueue* queue;
    void* context;
} IsleSourceCommandScope;
#endif
