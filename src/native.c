#include "35mmisle/native.h"

#include <stdint.h>

static int isle_valid_alignment(size_t alignment) {
    return alignment != 0 && (alignment & (alignment - 1)) == 0;
}

static isle_status isle_storage_status(const unsigned char *storage, size_t size) {
    uintptr_t address;

    if (storage == NULL || size == 0) {
        return ISLE_INVALID_ARGUMENT;
    }
    address = (uintptr_t)storage;
    if (size > UINTPTR_MAX - address) {
        return ISLE_OVERFLOW;
    }
    return ISLE_OK;
}

static isle_status isle_arena_status(const isle_arena *arena) {
    isle_status status;

    if (arena == NULL) {
        return ISLE_INVALID_ARGUMENT;
    }
    status = isle_storage_status(arena->storage.data, arena->storage.size);
    if (status != ISLE_OK) {
        return status;
    }
    if (arena->cursor > arena->storage.size) {
        return ISLE_INVALID_ARGUMENT;
    }
    return ISLE_OK;
}

isle_status isle_checked_add(size_t left, size_t right, size_t *out) {
    if (out == NULL) {
        return ISLE_INVALID_ARGUMENT;
    }
    if (right > SIZE_MAX - left) {
        return ISLE_OVERFLOW;
    }
    *out = left + right;
    return ISLE_OK;
}

isle_status isle_checked_multiply(size_t left, size_t right, size_t *out) {
    if (out == NULL) {
        return ISLE_INVALID_ARGUMENT;
    }
    if (left != 0 && right > SIZE_MAX / left) {
        return ISLE_OVERFLOW;
    }
    *out = left * right;
    return ISLE_OK;
}

isle_status isle_checked_align(size_t value, size_t alignment, size_t *out) {
    size_t remainder;
    size_t padding;

    if (out == NULL || !isle_valid_alignment(alignment)) {
        return ISLE_INVALID_ARGUMENT;
    }
    remainder = value & (alignment - 1);
    padding = remainder == 0 ? 0 : alignment - remainder;
    return isle_checked_add(value, padding, out);
}

isle_status isle_span_init(isle_span *out, void *storage, size_t size) {
    isle_status status;
    isle_span result;

    if (out == NULL) {
        return ISLE_INVALID_ARGUMENT;
    }
    status = isle_storage_status((unsigned char *)storage, size);
    if (status != ISLE_OK) {
        return status;
    }
    result.data = (unsigned char *)storage;
    result.size = size;
    *out = result;
    return ISLE_OK;
}

isle_status isle_span_resolve(const isle_span *span, size_t offset, size_t size,
                              isle_span *out) {
    isle_status status;
    size_t end;
    isle_span result;

    if (span == NULL || out == NULL || size == 0) {
        return ISLE_INVALID_ARGUMENT;
    }
    status = isle_storage_status(span->data, span->size);
    if (status != ISLE_OK) {
        return status;
    }
    status = isle_checked_add(offset, size, &end);
    if (status != ISLE_OK) {
        return status;
    }
    if (end > span->size) {
        return ISLE_OUT_OF_BOUNDS;
    }
    result.data = span->data + offset;
    result.size = size;
    *out = result;
    return ISLE_OK;
}

isle_status isle_arena_init(isle_arena *out, void *storage, size_t size) {
    isle_status status;
    isle_arena result;

    if (out == NULL) {
        return ISLE_INVALID_ARGUMENT;
    }
    status = isle_span_init(&result.storage, storage, size);
    if (status != ISLE_OK) {
        return status;
    }
    result.cursor = 0;
    *out = result;
    return ISLE_OK;
}

isle_status isle_arena_allocate(isle_arena *arena, size_t size, size_t alignment,
                                isle_span *out) {
    isle_status status;
    uintptr_t address;
    size_t remainder;
    size_t padding;
    size_t offset;
    size_t end;
    isle_span result;

    if (out == NULL || size == 0 || !isle_valid_alignment(alignment)) {
        return ISLE_INVALID_ARGUMENT;
    }
    status = isle_arena_status(arena);
    if (status != ISLE_OK) {
        return status;
    }

    /* The validated storage extent also proves base + cursor is representable. */
    address = (uintptr_t)arena->storage.data + arena->cursor;
    remainder = (size_t)(address & (uintptr_t)(alignment - 1));
    padding = remainder == 0 ? 0 : alignment - remainder;
    if (padding > UINTPTR_MAX - address) {
        return ISLE_OVERFLOW;
    }
    status = isle_checked_add(arena->cursor, padding, &offset);
    if (status != ISLE_OK) {
        return status;
    }
    status = isle_checked_add(offset, size, &end);
    if (status != ISLE_OK) {
        return status;
    }
    if (end > arena->storage.size) {
        return ISLE_EXHAUSTED;
    }

    /* Form a real subrange pointer only after padding, extent and bounds proof. */
    result.data = arena->storage.data + offset;
    result.size = size;
    arena->cursor = end;
    *out = result;
    return ISLE_OK;
}

isle_status isle_arena_reset(isle_arena *arena) {
    isle_status status = isle_arena_status(arena);

    if (status != ISLE_OK) {
        return status;
    }
    arena->cursor = 0;
    return ISLE_OK;
}
