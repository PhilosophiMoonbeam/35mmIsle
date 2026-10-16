#ifndef ISLE_CONTINUATION_H
#define ISLE_CONTINUATION_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct IsleContinuationDomain IsleContinuationDomain;
typedef struct IsleContinuation IsleContinuation;
typedef void (*IsleContinuationEntry)(void* user);

typedef enum IsleContinuationResult {
    ISLE_CONT_OK = 0,
    ISLE_CONT_INVALID_ARGUMENT,
    ISLE_CONT_WRONG_OWNER,
    ISLE_CONT_INVALID_STATE,
    ISLE_CONT_RESOURCE_FAILURE,
    ISLE_CONT_EXECUTION_FAILURE
} IsleContinuationResult;

typedef enum IsleContinuationState {
    ISLE_CONT_READY = 0,
    ISLE_CONT_RUNNING,
    ISLE_CONT_WAITING,
    ISLE_CONT_COMPLETED,
    ISLE_CONT_CANCELLED,
    ISLE_CONT_FAILED
} IsleContinuationState;

/* A domain has one constructing/dispatching owner thread. Its compiled-C
 * callbacks execute serially on stack-preserving continuations, never in
 * parallel with the owner or another gameplay callback. Creation/cancellation/
 * destruction from an active callback rendezvous synchronously with the owner;
 * this service handoff is not a gameplay tick or a wait/resume opportunity.
 * Other threads cannot use the domain. All handles expire at domain destruction.
 * Creation publishes an output pointer only after success. No C++ exception
 * crosses a C frame or this interface, including cancellation and self-finish.
 */
IsleContinuationResult isle_continuation_domain_create(IsleContinuationDomain** output);
IsleContinuationResult isle_continuation_domain_destroy(IsleContinuationDomain* domain);
/* Side-effect-free creator/active-callback access check for C game services. */
IsleContinuationResult isle_continuation_domain_check_access(const IsleContinuationDomain* domain);
/* Active callback only: run a non-yielding service synchronously on this
 * domain's constructing owner, then continue the SAME resume. This is not a
 * gameplay wait/tick. The callback receives only caller-owned storage and
 * must not resume/wait on this domain. Exceptions become EXECUTION_FAILURE.
 */
IsleContinuationResult isle_continuation_on_owner(IsleContinuationEntry service,
                                                 void* user);
IsleContinuationResult isle_continuation_create(IsleContinuationDomain* domain,
                                               IsleContinuationEntry entry,
                                               void* user,
                                               IsleContinuation** output);

/* Only the owner dispatches. Terminal resume is idempotent; failures return
 * EXECUTION_FAILURE and retain FAILED. Cancellation joins before returning.
 * Destroying another continuation from a callback also joins before releasing
 * its storage. The current callback must use finish, not cancel/destroy itself.
 */
IsleContinuationResult isle_continuation_resume(IsleContinuation* continuation);
IsleContinuationResult isle_continuation_cancel(IsleContinuation* continuation);
IsleContinuationResult isle_continuation_destroy(IsleContinuation* continuation);
IsleContinuationResult isle_continuation_state(IsleContinuation* continuation,
                                              IsleContinuationState* output);

/* Call only from the domain's active compiled-C callback. wait(0) stays inline;
 * wait(N) counts N subsequent owner resumes. On cancellation, wait does not
 * return into retired C code. Successful finish never returns to its C caller.
 * Invalid-context calls return an explicit error instead. These termination
 * operations discard C automatic state as original thread termination does:
 * callers must not retain owning C++ automatic objects in intervening frames.
 * Resources need explicit game/scene ownership, not stack-exit cleanup hooks.
 */
IsleContinuationResult isle_continuation_wait(IsleContinuationDomain* domain,
                                             uint64_t resumes);
IsleContinuationResult isle_continuation_finish(IsleContinuationDomain* domain);
/* Abort a failing active C callback through the same C-only termination fence;
 * resume reports EXECUTION_FAILURE. The diagnostic is copied before C state is
 * discarded. Invalid-context calls return an error without terminating code.
 */
IsleContinuationResult isle_continuation_fail(IsleContinuationDomain* domain,
                                             const char* message);

/* Owner/active-callback diagnostic view, valid until the next error or domain
 * destruction. No owning storage or C++ type escapes. NULL domain returns a
 * fixed diagnostic. Successful calls do not manufacture a completion event.
 */
const char* isle_continuation_error(const IsleContinuationDomain* domain);

#ifdef __cplusplus
}
#endif

#endif
