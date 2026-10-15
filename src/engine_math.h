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

#endif
