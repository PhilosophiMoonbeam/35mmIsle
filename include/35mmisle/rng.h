#ifndef ISLE_RNG_H
#define ISLE_RNG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Numeric seed bits are explicit, copyable replay state. No global RNG or
 * allocation. Mutating calls require a valid state pointer and serialized access.
 * The game's initial seed is 1; callers select initialization/reset policy. */
typedef struct isle_rng {
    uint32_t seed;
} isle_rng;

/* Each call advances the modulo-2^32 LCG exactly once, including range == 0.
 * next16 returns all sixteen high bits, not the fifteen-bit C-library rand().
 * next_float returns an exact binary fraction in [0, 1).
 * range preserves signed 32-bit product wrap and division toward zero; large or
 * negative ranges therefore do not have the usual [0, range) interpretation. */
uint16_t isle_rng_next16(isle_rng *state);
float isle_rng_next_float(isle_rng *state);
int32_t isle_rng_range(isle_rng *state, int32_t range);

/* These projections consume no LCG state. guest_ticks is the caller's already
 * sampled guest osGetTime-domain value, never a host wall-clock substitution.
 * time_range preserves wrapped multiplication, binary32 conversion and integer
 * truncation. As on the game CPU, non-exact conversion follows the FP mode. */
uint8_t isle_rng_time8(uint64_t guest_ticks);
float isle_rng_time_float(uint64_t guest_ticks);
int32_t isle_rng_time_range(uint64_t guest_ticks, int32_t range);

#ifdef __cplusplus
}
#endif

#endif
