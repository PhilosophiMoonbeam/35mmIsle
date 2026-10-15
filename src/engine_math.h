#ifndef ISLE_ENGINE_MATH_H
#define ISLE_ENGINE_MATH_H

#include "35mmisle/engine.h"
#include <math.h>
#include <float.h>
#include <stdint.h>

/* Independently owned public matrix ABI, not an SDK header/layout import.
 * GAME computations retain binary32 intermediates. Conversion is legal only
 * after the native caller checks finite signed-16.16 representability.
 */
typedef float f32;
typedef float Mtx4f[4][4];
typedef IsleGameFixedMatrix Mtx;
#define FTOFIX32(value) ((uint32_t) (int32_t) ((value) * 65536.0f))
_Static_assert(sizeof(float) == 4 && FLT_RADIX == 2 && FLT_MANT_DIG == 24 &&
               FLT_MAX_EXP == 128, "GAME matrix math requires binary32 float");
_Static_assert(sizeof(Mtx) == 64, "public fixed matrix ABI is 64 bytes");

/* Native declarations checked against the privately admitted GAME prototypes. */
void hal_mtx_f2l(Mtx4f source, Mtx* destination);
void hal_look_at_f(Mtx4f matrix, f32 eye_x, f32 eye_y, f32 eye_z,
                   f32 at_x, f32 at_y, f32 at_z, f32 up_x, f32 up_y, f32 up_z);
void hal_perspective_fast_f(Mtx4f matrix, uint16_t* normalization, f32 fovy,
                            f32 aspect, f32 near_plane, f32 far_plane, f32 scale);
void hal_rotate_rpy_translate_scale(Mtx* matrix, f32 dx, f32 dy, f32 dz,
                                   f32 r, f32 p, f32 h, f32 sx, f32 sy, f32 sz);
void hal_rotate_rpy_translate_scale_f(Mtx4f matrix, f32 dx, f32 dy, f32 dz,
                                     f32 r, f32 p, f32 h, f32 sx, f32 sy, f32 sz);

/* Defined MIPS signed interpretation, low-word multiply and arithmetic shift.
 * No implementation-defined unsigned-to-signed cast or negative signed shift.
 */
static inline int32_t isle_game_mips_signed(uint32_t bits) {
    return bits <= INT32_MAX ? (int32_t) bits :
        (int32_t) ((int64_t) bits - INT64_C(4294967296));
}
static inline int32_t isle_game_mips_mul(int32_t a, int32_t b) {
    return isle_game_mips_signed((uint32_t) a * (uint32_t) b);
}
static inline int32_t isle_game_mips_sar(int32_t value, unsigned shift) {
    uint32_t bits = (uint32_t) value;
    uint32_t result = bits >> shift;
    if (value < 0) result |= UINT32_MAX << (32u - shift);
    return isle_game_mips_signed(result);
}

#endif
