#include "35mmisle/game_math.h"

/* Independently expressed arithmetic from GAME src/sys/misc.c:15-76 at
 * df8815e75f3d79618466e01e0827c6165b93d11c. Coefficients, continued-fraction
 * evaluation order and comparison-based quadrants are GAME facts, not SDK
 * implementations or a host-libm approximation.
 *
 * misc.c:4-5 explicitly makes F_PI binary32 and HALF_PI = F_PI / 2.0f.
 * The binary32 pi value is also admitted by src/photo_score.cpp. For PI_2 in
 * atanf's reciprocal correction, retain the existing native 1.5707964f policy
 * (tools/prepare_actor_control.py). Its original SDK macro value/type remains
 * unproven; no SDK header was inspected. These equal binary32 half-pi values
 * are intentionally distinct provenance claims, not proof of that macro. */
static const float game_pi = 0x1.921fb6p+1f;
static const float game_half_pi = 0x1.921fb6p+0f; /* Exact game_pi / 2.0f. */
static const float game_reciprocal_half_pi = 0x1.921fb6p+0f;

/* Materialize each GAME f32 operation, including on excess-precision targets.
 * Build this unit with rounding-mode access enabled, no contraction and no
 * fast-math; no expression may reassociate adjacent continued-fraction steps. */
static float game_add(float left, float right) {
    volatile float result = left + right;
    return result;
}

static float game_subtract(float left, float right) {
    volatile float result = left - right;
    return result;
}

static float game_multiply(float left, float right) {
    volatile float result = left * right;
    return result;
}

static float game_divide(float left, float right) {
    volatile float result = left / right;
    return result;
}

float isle_game_atanf(float value) {
    int reciprocal = 0;
    float square;
    float fraction;
    float angle;

    if (value == 0.0f) return 0.0f;

    if (value > 1.0f) {
        reciprocal = 1;
        value = game_divide(1.0f, value);
    } else if (value < -1.0f) {
        reciprocal = -1;
        value = game_divide(1.0f, value);
    }

    square = game_multiply(value, value);
    fraction = game_add(game_divide(square, -0.10810675f), -44.57192f);
    fraction = game_add(game_divide(square, fraction), -0.1619081f);
    fraction = game_add(game_divide(square, fraction), -15.774018f);
    fraction = game_add(game_divide(square, fraction), -0.55556977f);
    fraction = game_add(game_divide(square, fraction), -3.000003f);
    fraction = game_add(game_divide(square, fraction), 1.0f);
    angle = game_multiply(fraction, value);

    if (reciprocal > 0) return game_subtract(game_reciprocal_half_pi, angle);
    if (reciprocal < 0) return game_subtract(-game_reciprocal_half_pi, angle);
    return angle;
}

float isle_game_atan2f(float y, float x) {
    if (x > 0.0f) return isle_game_atanf(game_divide(y, x));

    if (x < 0.0f) {
        const float direction = y < 0.0f ? -1.0f : 1.0f;
        float slope = game_divide(y, x);
        if (slope < 0.0f) slope = -slope;
        return game_multiply(game_subtract(game_pi, isle_game_atanf(slope)), direction);
    }

    if (y != 0.0f) {
        const float direction = y < 0.0f ? -1.0f : 1.0f;
        return game_multiply(direction, game_half_pi);
    }
    return 0.0f;
}
