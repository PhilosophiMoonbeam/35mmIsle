#ifndef ISLE_CONTINUATION_TRAMPOLINE_H
#define ISLE_CONTINUATION_TRAMPOLINE_H

#include <setjmp.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct IsleContinuationFrame {
    jmp_buf environment;
} IsleContinuationFrame;

typedef void (*IsleContinuationGuardedEntry)(void* context);

/* The saved frame lives only during this call, on the same native worker. */
void isle_continuation_trampoline(IsleContinuationFrame* frame,
                                  IsleContinuationGuardedEntry entry,
                                  void* context);

#ifdef __cplusplus
[[noreturn]] void isle_continuation_abort(IsleContinuationFrame* frame);
#else
_Noreturn void isle_continuation_abort(IsleContinuationFrame* frame);
#endif

#ifdef __cplusplus
}
#endif

#endif
