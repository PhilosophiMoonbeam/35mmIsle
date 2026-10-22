#ifndef ISLE_GAME_MATH_H
#define ISLE_GAME_MATH_H

#ifdef __cplusplus
extern "C" {
#endif

/* GAME scalar arithmetic, not the host libm contract. In particular, both
 * signed zero inputs to atan collapse to +0, and atan2(-0, negative) is +pi.
 * Operations follow the caller's current floating-point rounding mode. */
float isle_game_atanf(float value);
float isle_game_atan2f(float y, float x);

#ifdef __cplusplus
}
#endif

#endif
