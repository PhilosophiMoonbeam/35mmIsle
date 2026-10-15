#include "35mmisle/rng.h"

#include <float.h>
#include <limits.h>

_Static_assert(FLT_RADIX == 2 && FLT_MANT_DIG == 24 && sizeof(float) == 4,
               "Game random projections require binary32 float");

static int32_t signed_bits(uint32_t value) {
    /* Both conversions are in range; no signed overflow or implementation-
     * defined unsigned-to-signed conversion is needed for the original wrap. */
    return value <= INT32_MAX ? (int32_t)value :
        -(int32_t)(UINT32_MAX - value) - 1;
}

uint16_t isle_rng_next16(isle_rng *state) {
    state->seed = state->seed * UINT32_C(214013) + UINT32_C(2531011);
    return (uint16_t)(state->seed >> 16);
}

float isle_rng_next_float(isle_rng *state) {
    return (float)isle_rng_next16(state) * (1.0f / 65536.0f);
}

int32_t isle_rng_range(isle_rng *state, int32_t range) {
    const uint32_t product = (uint32_t)isle_rng_next16(state) * (uint32_t)range;
    return signed_bits(product) / INT32_C(65536);
}

uint8_t isle_rng_time8(uint64_t guest_ticks) {
    return (uint8_t)guest_ticks;
}

float isle_rng_time_float(uint64_t guest_ticks) {
    return (float)(uint8_t)guest_ticks * (1.0f / 256.0f);
}

int32_t isle_rng_time_range(uint64_t guest_ticks, int32_t range) {
    const uint32_t product = (uint32_t)(uint8_t)guest_ticks * (uint32_t)range;
    const float scaled = (float)signed_bits(product) * (1.0f / 256.0f);
    return (int32_t)scaled;
}
