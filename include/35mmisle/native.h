#ifndef ISLE_NATIVE_H
#define ISLE_NATIVE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Original host-memory mechanisms only, not game or serialized address types. */
typedef enum isle_status {
    ISLE_OK = 0,
    ISLE_INVALID_ARGUMENT,
    ISLE_OVERFLOW,
    ISLE_EXHAUSTED,
    ISLE_OUT_OF_BOUNDS
} isle_status;

/*
 * All functions return an explicit status. A rejected operation changes neither
 * its output object nor an arena's cursor. Output pointers must be non-NULL.
 * Arithmetic accepts zero operands; alignment must be a nonzero power of two.
 * Alignment rounds up, including leaving an already aligned value unchanged.
 */
isle_status isle_checked_add(size_t left, size_t right, size_t *out);
isle_status isle_checked_multiply(size_t left, size_t right, size_t *out);
isle_status isle_checked_align(size_t value, size_t alignment, size_t *out);

/*
 * A mutable bounded view of actual caller-owned storage. This is not a ROM
 * offset, segmented address, resource identity, or tagged pointer selector.
 * For an extent that passes validation, the caller must still guarantee an
 * actual allocation of at least size bytes and keep it alive for every use.
 * Numeric extent checks cannot discover allocation size or lifetime.
 *
 * Memory APIs reject NULL storage and zero size, including zero-size resolves
 * and allocations. They never manufacture an empty or one-past-end result.
 * Metadata and output objects must not overlap backing storage. Apart from
 * resolving a span in place, input/output metadata objects must not overlap.
 * No synchronization, heap allocation, ownership transfer, or storage clearing
 * occurs.
 */
typedef struct isle_span {
    unsigned char *data;
    size_t size;
} isle_span;

isle_status isle_span_init(isle_span *out, void *storage, size_t size);

/*
 * Resolve a positive extent only after checked offset + size and bounds proof.
 * Overflow is distinct from an otherwise representable out-of-bounds extent.
 * out may equal span; both success and failure are safe for in-place resolution.
 */
isle_status isle_span_resolve(const isle_span *span, size_t offset, size_t size,
                              isle_span *out);

/*
 * Caller-owned arena state over caller-owned actual bytes. Use init before
 * allocation/reset and do not modify the fields directly. The arena does not
 * free storage. Positive allocations align the actual native address, account
 * for padding in cursor/extent checks, and return a bounded byte span.
 *
 * Reset rewinds only the cursor; it does not clear memory. All users of prior
 * allocations must stop before reset or reinitialization, since later
 * allocations can reuse those bytes. Reset/reinitialization invalidates prior
 * allocations logically, and destroying backing storage invalidates every view.
 */
typedef struct isle_arena {
    isle_span storage;
    size_t cursor;
} isle_arena;

isle_status isle_arena_init(isle_arena *out, void *storage, size_t size);
isle_status isle_arena_allocate(isle_arena *arena, size_t size, size_t alignment,
                                isle_span *out);
isle_status isle_arena_reset(isle_arena *arena);

#ifdef __cplusplus
}
#endif

#endif
