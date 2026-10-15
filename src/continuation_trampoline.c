#include "continuation_trampoline.h"

void isle_continuation_trampoline(IsleContinuationFrame* frame,
                                  IsleContinuationGuardedEntry entry,
                                  void* context) {
    /* C11 7.13.1.1: setjmp is the comparison in an if controlling expression.
     * These parameters never change between setjmp and longjmp. The mutable
     * exception/control state belongs to the stable C++ node, not this frame.
     * Compile this file as C, including on MSVC: no C++ destructor unwinding is
     * requested from longjmp, and no Windows-invoked callback frame is crossed.
     */
    if (setjmp(frame->environment) == 0) {
        entry(context);
    }
}

_Noreturn void isle_continuation_abort(IsleContinuationFrame* frame) {
    longjmp(frame->environment, 1);
}
