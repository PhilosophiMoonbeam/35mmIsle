#include "35mmisle/native.h"
#include "35mmisle/rng.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned int failures;

#define CHECK(condition)                                                        \
    do {                                                                        \
        if (!(condition)) {                                                     \
            fprintf(stderr, "native_tests.c:%d: %s\n", __LINE__, #condition);    \
            ++failures;                                                         \
        }                                                                       \
    } while (0)

static void check_span(const isle_span *span, const unsigned char *data,
                       size_t size) {
    CHECK(span->data == data);
    CHECK(span->size == size);
}

static void check_arena(const isle_arena *arena, const isle_arena *before) {
    CHECK(arena->storage.data == before->storage.data);
    CHECK(arena->storage.size == before->storage.size);
    CHECK(arena->cursor == before->cursor);
}

static void test_arithmetic(void) {
    static const struct {
        size_t left;
        size_t right;
        size_t expected;
    } add_cases[] = {
        {0, 0, 0}, {23, 19, 42}, {SIZE_MAX, 0, SIZE_MAX},
        {0, SIZE_MAX, SIZE_MAX}, {SIZE_MAX - 3, 3, SIZE_MAX}
    };
    static const struct {
        size_t left;
        size_t right;
        size_t expected;
    } multiply_cases[] = {
        {0, SIZE_MAX, 0}, {SIZE_MAX, 0, 0}, {7, 9, 63},
        {SIZE_MAX, 1, SIZE_MAX}, {SIZE_MAX / 2, 2, SIZE_MAX - 1}
    };
    static const struct {
        size_t value;
        size_t alignment;
        size_t expected;
    } align_cases[] = {
        {0, 8, 0}, {1, 8, 8}, {8, 8, 8}, {9, 8, 16},
        {SIZE_MAX, 1, SIZE_MAX}, {SIZE_MAX - 7, 8, SIZE_MAX - 7},
        {0, SIZE_MAX / 2 + 1, 0},
        {SIZE_MAX / 2 + 1, SIZE_MAX / 2 + 1, SIZE_MAX / 2 + 1}
    };
    static const size_t invalid_alignments[] = {0, 3, 6, SIZE_MAX};
    size_t result;
    size_t index;

    for (index = 0; index < sizeof(add_cases) / sizeof(add_cases[0]); ++index) {
        result = 91;
        CHECK(isle_checked_add(add_cases[index].left, add_cases[index].right,
                               &result) == ISLE_OK);
        CHECK(result == add_cases[index].expected);
    }
    result = 91;
    CHECK(isle_checked_add(SIZE_MAX, 1, &result) == ISLE_OVERFLOW);
    CHECK(result == 91);
    CHECK(isle_checked_add(SIZE_MAX - 3, 4, &result) == ISLE_OVERFLOW);
    CHECK(result == 91);
    CHECK(isle_checked_add(1, 2, NULL) == ISLE_INVALID_ARGUMENT);

    for (index = 0; index < sizeof(multiply_cases) / sizeof(multiply_cases[0]);
         ++index) {
        result = 91;
        CHECK(isle_checked_multiply(multiply_cases[index].left,
                                    multiply_cases[index].right,
                                    &result) == ISLE_OK);
        CHECK(result == multiply_cases[index].expected);
    }
    result = 91;
    CHECK(isle_checked_multiply(SIZE_MAX, 2, &result) == ISLE_OVERFLOW);
    CHECK(result == 91);
    CHECK(isle_checked_multiply(SIZE_MAX / 2 + 1, 2, &result) == ISLE_OVERFLOW);
    CHECK(result == 91);
    CHECK(isle_checked_multiply(2, SIZE_MAX, &result) == ISLE_OVERFLOW);
    CHECK(result == 91);
    CHECK(isle_checked_multiply(0, SIZE_MAX, NULL) == ISLE_INVALID_ARGUMENT);

    for (index = 0; index < sizeof(align_cases) / sizeof(align_cases[0]); ++index) {
        result = 91;
        CHECK(isle_checked_align(align_cases[index].value,
                                 align_cases[index].alignment, &result) == ISLE_OK);
        CHECK(result == align_cases[index].expected);
    }
    result = 91;
    CHECK(isle_checked_align(SIZE_MAX, 8, &result) == ISLE_OVERFLOW);
    CHECK(result == 91);
    CHECK(isle_checked_align(SIZE_MAX - 6, 8, &result) == ISLE_OVERFLOW);
    CHECK(result == 91);
    CHECK(isle_checked_align(SIZE_MAX / 2 + 2, SIZE_MAX / 2 + 1,
                             &result) == ISLE_OVERFLOW);
    CHECK(result == 91);
    for (index = 0; index < sizeof(invalid_alignments) / sizeof(invalid_alignments[0]);
         ++index) {
        CHECK(isle_checked_align(7, invalid_alignments[index],
                                 &result) == ISLE_INVALID_ARGUMENT);
        CHECK(result == 91);
    }
    CHECK(isle_checked_align(7, 8, NULL) == ISLE_INVALID_ARGUMENT);
}

static void test_spans(void) {
    unsigned char storage[32];
    unsigned char sentinel[3] = {0};
    isle_span source = {sentinel, sizeof(sentinel)};
    isle_span result = {sentinel, sizeof(sentinel)};
    isle_span in_place;
    isle_span malformed;
    isle_status status;
    size_t index;

    for (index = 0; index < sizeof(storage); ++index) {
        storage[index] = (unsigned char)index;
    }
    status = isle_span_init(&source, storage, sizeof(storage));
    CHECK(status == ISLE_OK);
    if (status != ISLE_OK) {
        return;
    }
    check_span(&source, storage, sizeof(storage));
    CHECK(isle_span_resolve(&source, 0, sizeof(storage), &result) == ISLE_OK);
    check_span(&result, storage, sizeof(storage));
    CHECK(isle_span_resolve(&source, 7, 9, &result) == ISLE_OK);
    check_span(&result, storage + 7, 9);
    if (result.data == storage + 7 && result.size == 9) {
        result.data[0] = 0x9b;
        result.data[8] = 0xa7;
        CHECK(storage[7] == 0x9b);
        CHECK(storage[15] == 0xa7);
        CHECK(storage[6] == 6);
        CHECK(storage[16] == 16);
    }
    CHECK(isle_span_resolve(&source, sizeof(storage) - 1, 1, &result) == ISLE_OK);
    check_span(&result, storage + sizeof(storage) - 1, 1);

    result.data = sentinel;
    result.size = sizeof(sentinel);
    CHECK(isle_span_resolve(&source, SIZE_MAX, 1, &result) == ISLE_OVERFLOW);
    check_span(&result, sentinel, sizeof(sentinel));
    CHECK(isle_span_resolve(&source, 1, SIZE_MAX, &result) == ISLE_OVERFLOW);
    check_span(&result, sentinel, sizeof(sentinel));
    CHECK(isle_span_resolve(&source, 31, 2, &result) == ISLE_OUT_OF_BOUNDS);
    check_span(&result, sentinel, sizeof(sentinel));
    CHECK(isle_span_resolve(&source, 33, 1, &result) == ISLE_OUT_OF_BOUNDS);
    check_span(&result, sentinel, sizeof(sentinel));
    CHECK(isle_span_resolve(&source, 0, SIZE_MAX, &result) == ISLE_OUT_OF_BOUNDS);
    check_span(&result, sentinel, sizeof(sentinel));
    CHECK(isle_span_resolve(&source, 32, 0, &result) == ISLE_INVALID_ARGUMENT);
    check_span(&result, sentinel, sizeof(sentinel));
    CHECK(isle_span_resolve(NULL, 0, 1, &result) == ISLE_INVALID_ARGUMENT);
    check_span(&result, sentinel, sizeof(sentinel));
    CHECK(isle_span_resolve(&source, 0, 1, NULL) == ISLE_INVALID_ARGUMENT);
    check_span(&source, storage, sizeof(storage));

    in_place = source;
    CHECK(isle_span_resolve(&in_place, 8, 10, &in_place) == ISLE_OK);
    check_span(&in_place, storage + 8, 10);
    CHECK(isle_span_resolve(&in_place, 10, 1, &in_place) == ISLE_OUT_OF_BOUNDS);
    check_span(&in_place, storage + 8, 10);
    CHECK(isle_span_resolve(&in_place, SIZE_MAX, 1, &in_place) == ISLE_OVERFLOW);
    check_span(&in_place, storage + 8, 10);

    CHECK(isle_span_init(&result, NULL, sizeof(storage)) == ISLE_INVALID_ARGUMENT);
    check_span(&result, sentinel, sizeof(sentinel));
    CHECK(isle_span_init(&result, storage, 0) == ISLE_INVALID_ARGUMENT);
    check_span(&result, sentinel, sizeof(sentinel));
    /* A real pointer plus an impossible numeric extent: no fake pointer formed. */
    CHECK(isle_span_init(&result, storage, SIZE_MAX) == ISLE_OVERFLOW);
    check_span(&result, sentinel, sizeof(sentinel));
    CHECK(isle_span_init(NULL, storage, sizeof(storage)) == ISLE_INVALID_ARGUMENT);

    malformed.data = NULL;
    malformed.size = sizeof(storage);
    CHECK(isle_span_resolve(&malformed, 0, 1, &result) == ISLE_INVALID_ARGUMENT);
    check_span(&result, sentinel, sizeof(sentinel));
    malformed.data = storage;
    malformed.size = 0;
    CHECK(isle_span_resolve(&malformed, 0, 1, &result) == ISLE_INVALID_ARGUMENT);
    check_span(&result, sentinel, sizeof(sentinel));
    malformed.size = SIZE_MAX;
    CHECK(isle_span_resolve(&malformed, 0, 1, &result) == ISLE_OVERFLOW);
    check_span(&result, sentinel, sizeof(sentinel));
}

static void test_arena_initialization(void) {
    unsigned char storage[32];
    unsigned char sentinel[3] = {0};
    isle_arena arena = {{storage, sizeof(storage)}, 3};
    isle_arena before = arena;
    isle_arena malformed;
    isle_span result = {sentinel, sizeof(sentinel)};
    isle_status status;

    CHECK(isle_arena_init(&arena, NULL, sizeof(storage)) == ISLE_INVALID_ARGUMENT);
    check_arena(&arena, &before);
    CHECK(isle_arena_init(&arena, storage, 0) == ISLE_INVALID_ARGUMENT);
    check_arena(&arena, &before);
    CHECK(isle_arena_init(&arena, storage, SIZE_MAX) == ISLE_OVERFLOW);
    check_arena(&arena, &before);
    CHECK(isle_arena_init(NULL, storage, sizeof(storage)) == ISLE_INVALID_ARGUMENT);
    CHECK(isle_arena_reset(NULL) == ISLE_INVALID_ARGUMENT);

    status = isle_arena_init(&arena, storage, sizeof(storage));
    CHECK(status == ISLE_OK);
    if (status != ISLE_OK) {
        return;
    }
    check_span(&arena.storage, storage, sizeof(storage));
    CHECK(arena.cursor == 0);

    malformed = arena;
    malformed.cursor = sizeof(storage) + 1;
    before = malformed;
    CHECK(isle_arena_allocate(&malformed, 1, 1, &result) == ISLE_INVALID_ARGUMENT);
    check_arena(&malformed, &before);
    check_span(&result, sentinel, sizeof(sentinel));
    CHECK(isle_arena_reset(&malformed) == ISLE_INVALID_ARGUMENT);
    check_arena(&malformed, &before);

    malformed = arena;
    malformed.storage.data = NULL;
    before = malformed;
    CHECK(isle_arena_allocate(&malformed, 1, 1, &result) == ISLE_INVALID_ARGUMENT);
    check_arena(&malformed, &before);
    check_span(&result, sentinel, sizeof(sentinel));
    CHECK(isle_arena_reset(&malformed) == ISLE_INVALID_ARGUMENT);
    check_arena(&malformed, &before);

    malformed = arena;
    malformed.storage.size = 0;
    before = malformed;
    CHECK(isle_arena_allocate(&malformed, 1, 1, &result) == ISLE_INVALID_ARGUMENT);
    check_arena(&malformed, &before);
    check_span(&result, sentinel, sizeof(sentinel));
    CHECK(isle_arena_reset(&malformed) == ISLE_INVALID_ARGUMENT);
    check_arena(&malformed, &before);

    malformed = arena;
    malformed.storage.size = SIZE_MAX;
    before = malformed;
    CHECK(isle_arena_allocate(&malformed, 1, 1, &result) == ISLE_OVERFLOW);
    check_arena(&malformed, &before);
    check_span(&result, sentinel, sizeof(sentinel));
    CHECK(isle_arena_reset(&malformed) == ISLE_OVERFLOW);
    check_arena(&malformed, &before);
}

static void test_arena_exact_fit_and_reset(void) {
    unsigned char storage[19];
    unsigned char snapshot[sizeof(storage)];
    unsigned char sentinel[3] = {0};
    isle_arena arena = {{sentinel, sizeof(sentinel)}, 0};
    isle_arena before;
    isle_span first = {sentinel, sizeof(sentinel)};
    isle_span result = {sentinel, sizeof(sentinel)};
    isle_status status;

    memset(storage, 0xcd, sizeof(storage));
    status = isle_arena_init(&arena, storage, sizeof(storage));
    CHECK(status == ISLE_OK);
    if (status != ISLE_OK) {
        return;
    }
    CHECK(isle_arena_allocate(&arena, 5, 1, &first) == ISLE_OK);
    check_span(&first, storage, 5);
    CHECK(arena.cursor == 5);
    CHECK(isle_arena_allocate(&arena, 14, 1, &result) == ISLE_OK);
    check_span(&result, storage + 5, 14);
    CHECK(arena.cursor == sizeof(storage));
    if (first.data == storage && first.size == 5 &&
        result.data == storage + 5 && result.size == 14) {
        memset(first.data, 0x31, first.size);
        memset(result.data, 0x72, result.size);
        CHECK(storage[4] == 0x31);
        CHECK(storage[5] == 0x72);
        CHECK(storage[18] == 0x72);
    }
    memcpy(snapshot, storage, sizeof(storage));
    before = arena;
    result.data = sentinel;
    result.size = sizeof(sentinel);
    CHECK(isle_arena_allocate(&arena, 1, 1, &result) == ISLE_EXHAUSTED);
    check_arena(&arena, &before);
    check_span(&result, sentinel, sizeof(sentinel));
    CHECK(memcmp(storage, snapshot, sizeof(storage)) == 0);

    CHECK(isle_arena_reset(&arena) == ISLE_OK);
    CHECK(arena.cursor == 0);
    check_span(&arena.storage, storage, sizeof(storage));
    CHECK(memcmp(storage, snapshot, sizeof(storage)) == 0);
    CHECK(isle_arena_allocate(&arena, 5, 1, &result) == ISLE_OK);
    check_span(&result, storage, 5);
    CHECK(arena.cursor == 5);
    CHECK(memcmp(storage, snapshot, sizeof(storage)) == 0);
}

static void test_arena_padding(void) {
    unsigned char storage[128];
    unsigned char sentinel[3] = {0};
    size_t aligned_offset = (size_t)((32 - (uintptr_t)storage % 32) % 32);
    unsigned char *base = storage + aligned_offset + 1;
    isle_arena arena = {{sentinel, sizeof(sentinel)}, 0};
    isle_arena before;
    isle_span first = {sentinel, sizeof(sentinel)};
    isle_span second = {sentinel, sizeof(sentinel)};
    isle_status status;
    size_t index;

    memset(storage, 0x44, sizeof(storage));
    /* base is a real interior pointer with remainder 1, so padding is 31. */
    status = isle_arena_init(&arena, base, 31);
    CHECK(status == ISLE_OK);
    if (status != ISLE_OK) {
        return;
    }
    before = arena;
    CHECK(isle_arena_allocate(&arena, 1, 32, &first) == ISLE_EXHAUSTED);
    check_arena(&arena, &before);
    check_span(&first, sentinel, sizeof(sentinel));

    CHECK(isle_arena_init(&arena, base, 32) == ISLE_OK);
    CHECK(isle_arena_allocate(&arena, 1, 32, &first) == ISLE_OK);
    check_span(&first, base + 31, 1);
    CHECK(arena.cursor == 32);
    if (first.data == base + 31) {
        CHECK((uintptr_t)first.data % 32 == 0);
    }

    CHECK(isle_arena_init(&arena, base, 64) == ISLE_OK);
    CHECK(isle_arena_allocate(&arena, 8, 32, &first) == ISLE_OK);
    check_span(&first, base + 31, 8);
    CHECK(arena.cursor == 39);
    CHECK(isle_arena_allocate(&arena, 25, 1, &second) == ISLE_OK);
    check_span(&second, base + 39, 25);
    CHECK(arena.cursor == 64);
    if (first.data == base + 31 && first.size == 8 &&
        second.data == base + 39 && second.size == 25) {
        memset(first.data, 0x77, first.size);
        memset(second.data, 0x22, second.size);
        for (index = 0; index < 31; ++index) {
            CHECK(base[index] == 0x44);
        }
        CHECK(base[31] == 0x77);
        CHECK(base[38] == 0x77);
        CHECK(base[39] == 0x22);
        CHECK(base[63] == 0x22);
        CHECK(base[-1] == 0x44);
        CHECK(base[64] == 0x44);
    }
}

static void test_arena_rejections(void) {
    static const size_t invalid_alignments[] = {0, 3, 6, SIZE_MAX};
    unsigned char storage[64];
    unsigned char sentinel[3] = {0};
    isle_arena arena = {{sentinel, sizeof(sentinel)}, 0};
    isle_arena before;
    isle_span result = {sentinel, sizeof(sentinel)};
    isle_status status;
    size_t index;

    status = isle_arena_init(&arena, storage, sizeof(storage));
    CHECK(status == ISLE_OK);
    if (status != ISLE_OK) {
        return;
    }
    CHECK(isle_arena_allocate(&arena, 1, 1, &result) == ISLE_OK);
    CHECK(arena.cursor == 1);
    before = arena;
    result.data = sentinel;
    result.size = sizeof(sentinel);

    for (index = 0; index < sizeof(invalid_alignments) / sizeof(invalid_alignments[0]);
         ++index) {
        CHECK(isle_arena_allocate(&arena, 1, invalid_alignments[index],
                                  &result) == ISLE_INVALID_ARGUMENT);
        check_arena(&arena, &before);
        check_span(&result, sentinel, sizeof(sentinel));
    }
    CHECK(isle_arena_allocate(&arena, 0, 1, &result) == ISLE_INVALID_ARGUMENT);
    check_arena(&arena, &before);
    check_span(&result, sentinel, sizeof(sentinel));
    CHECK(isle_arena_allocate(&arena, 1, 1, NULL) == ISLE_INVALID_ARGUMENT);
    check_arena(&arena, &before);
    CHECK(isle_arena_allocate(NULL, 1, 1, &result) == ISLE_INVALID_ARGUMENT);
    check_span(&result, sentinel, sizeof(sentinel));
    CHECK(isle_arena_allocate(&arena, SIZE_MAX, 1, &result) == ISLE_OVERFLOW);
    check_arena(&arena, &before);
    check_span(&result, sentinel, sizeof(sentinel));
    CHECK(isle_arena_allocate(&arena, sizeof(storage), 1, &result) == ISLE_EXHAUSTED);
    check_arena(&arena, &before);
    check_span(&result, sentinel, sizeof(sentinel));
}

static void test_real_high_address_arena(void) {
    unsigned char *allocation = (unsigned char *)malloc(1024);
    unsigned char sentinel[3] = {0};
    unsigned char *base;
    isle_arena arena = {{sentinel, sizeof(sentinel)}, 0};
    isle_span first = {sentinel, sizeof(sentinel)};
    isle_span second = {sentinel, sizeof(sentinel)};
    isle_span tail = {sentinel, sizeof(sentinel)};
    isle_status status;
    size_t first_offset;
    size_t second_offset;
    size_t second_padding;
    size_t index;
    unsigned int before_failures = failures;
    int high_available;

    CHECK(allocation != NULL);
    if (allocation == NULL) {
        puts("high-address arena: UNAVAILABLE (malloc failed)");
        return;
    }
    high_available = (uintptr_t)allocation > UINT32_MAX;
    base = allocation + 1;
    first_offset = (size_t)((256 - (uintptr_t)base % 256) % 256);
    second_offset = first_offset + 64;
    second_padding = (size_t)((64 - (uintptr_t)(base + second_offset) % 64) % 64);
    second_offset += second_padding;
    memset(allocation, 0xa5, 1024);

    status = isle_arena_init(&arena, base, 1023);
    CHECK(status == ISLE_OK);
    if (status == ISLE_OK) {
        CHECK(isle_arena_allocate(&arena, 64, 256, &first) == ISLE_OK);
        check_span(&first, base + first_offset, 64);
        CHECK(arena.cursor == first_offset + 64);
        CHECK(isle_arena_allocate(&arena, 96, 64, &second) == ISLE_OK);
        check_span(&second, base + second_offset, 96);
        CHECK(arena.cursor == second_offset + 96);
        if (first.data == base + first_offset && first.size == 64 &&
            second.data == base + second_offset && second.size == 96) {
            CHECK((uintptr_t)first.data % 256 == 0);
            CHECK((uintptr_t)second.data % 64 == 0);
            if (high_available) {
                CHECK((uintptr_t)first.data > UINT32_MAX);
                CHECK((uintptr_t)second.data > UINT32_MAX);
            }
            memset(first.data, 0x6a, first.size);
            memset(second.data, 0x93, second.size);
            CHECK(isle_span_resolve(&first, 63, 1, &tail) == ISLE_OK);
            check_span(&tail, base + first_offset + 63, 1);
            if (tail.data == base + first_offset + 63 && tail.size == 1) {
                tail.data[0] = 0x7d;
            }
            for (index = 0; index < 1024; ++index) {
                unsigned char expected = 0xa5;
                if (index >= first_offset + 1 && index < first_offset + 65) {
                    expected = index == first_offset + 64 ? 0x7d : 0x6a;
                }
                if (index >= second_offset + 1 && index < second_offset + 97) {
                    expected = 0x93;
                }
                CHECK(allocation[index] == expected);
            }
        }
    }

    if (high_available) {
        printf("high-address arena: %s real malloc storage at 0x%" PRIxPTR
               " (>4 GiB), aligned allocations and bounded read/write\n",
               failures == before_failures ? "EXERCISED" : "FAILED with available",
               (uintptr_t)allocation);
    } else {
        printf("high-address arena: UNAVAILABLE (malloc storage at 0x%" PRIxPTR
               " is below 4 GiB); real-storage checks %s\n",
               (uintptr_t)allocation,
               failures == before_failures ? "passed" : "failed");
    }
    free(allocation);
}

static void test_game_rng(void) {
    static const uint32_t seeds[] = {
        UINT32_C(0x0029e2c0), UINT32_C(0xc823f683), UINT32_C(0x18be873a),
        UINT32_C(0xe7847115), UINT32_C(0xcae1df84), UINT32_C(0x3d6c1037),
        UINT32_C(0x2cd60a1e), UINT32_C(0x72ae7869)
    };
    isle_rng state = {1};
    size_t i;
    for (i = 0; i < sizeof(seeds) / sizeof(seeds[0]); ++i) {
        CHECK(isle_rng_next16(&state) == seeds[i] >> 16);
        CHECK(state.seed == seeds[i]);
    }

    state.seed = 1;
    CHECK(isle_rng_next16(&state) == 41);
    CHECK(isle_rng_next_float(&state) == 51235.0f / 65536.0f);
    CHECK(isle_rng_range(&state, 100) == 9);
    CHECK(isle_rng_range(&state, 0) == 0);
    CHECK(isle_rng_next16(&state) == 51937);

    state.seed = UINT32_C(0x701bf641);
    CHECK(isle_rng_next_float(&state) == 65535.0f / 65536.0f);
    state.seed = UINT32_C(0xa170f641);
    CHECK(isle_rng_next_float(&state) == 0.0f);
    CHECK(state.seed == 0);
}

static void test_game_rng_signed_ranges(void) {
    static const struct {
        int32_t range;
        int32_t expected;
    } cases[] = {
        {0, 0}, {100, 99}, {65536, -1}, {32769, -32767},
        {-1, 0}, {-32768, -32767}, {INT32_MAX, 32767}, {INT32_MIN, -32768}
    };
    size_t i;
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        isle_rng state = {UINT32_C(0x701bf641)};
        CHECK(isle_rng_range(&state, cases[i].range) == cases[i].expected);
        CHECK(state.seed == UINT32_C(0xffff0000));
    }
}

static void test_guest_time_random(void) {
    CHECK(isle_rng_time8(UINT64_C(0x1234567801)) == 1);
    CHECK(isle_rng_time8(UINT64_MAX) == 255);
    CHECK(isle_rng_time_float(UINT64_C(0x100)) == 0.0f);
    CHECK(isle_rng_time_float(UINT64_MAX) == 255.0f / 256.0f);
    CHECK(isle_rng_time_range(UINT64_C(0x100), INT32_MAX) == 0);
    CHECK(isle_rng_time_range(255, 65536) == 65280);
    CHECK(isle_rng_time_range(255, -1) == 0);
    CHECK(isle_rng_time_range(128, INT32_MAX) == 0);
    /* Binary32 rounding before scaling differs from integer division here. */
    CHECK(isle_rng_time_range(1, INT32_MAX) == 8388608);
    CHECK(isle_rng_time_range(UINT64_MAX, INT32_MAX) == 8388607);
    CHECK(isle_rng_time_range(1, INT32_MIN) == -8388608);
}

int main(void) {
    test_arithmetic();
    test_spans();
    test_arena_initialization();
    test_arena_exact_fit_and_reset();
    test_arena_padding();
    test_arena_rejections();
    test_real_high_address_arena();
    test_game_rng();
    test_game_rng_signed_ranges();
    test_guest_time_random();

    if (failures != 0) {
        fprintf(stderr, "native mechanism tests: %u failure(s)\n", failures);
        return EXIT_FAILURE;
    }
    puts("native mechanism tests passed (synthetic memory and arithmetic)");
    return EXIT_SUCCESS;
}
