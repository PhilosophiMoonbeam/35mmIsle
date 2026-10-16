#ifndef ISLE_CONTEXTUAL_MODEL_MATH_H
#define ISLE_CONTEXTUAL_MODEL_MATH_H

#include "35mmisle/engine.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Renderer-owned source-54 payload. The accepted finite pose has no kind-28
 * quantization limits; inherited XYZ is consumed exactly by GAME's reciprocal
 * expressions. A zero/unrepresentable inherited scale is not repaired. Native
 * binary32 sinf/cosf are the explicit C-libm seam, not original trig bit parity.
 * Reject nonfinite or signed-16.16-unrepresentable consumed results before any
 * output write. No allocation. Output may alias input storage.
 */
IsleGameResult isle_model_source54_matrix(const IsleGameModelTransform* transform,
    const float inherited_scale[3], IsleGameFixedMatrix* output);

/* Source render.c's camera-derived AFA8 basis, not ordinary perspective.
 * axis 1 consumes XZ distance and eye/at Y; axis 2 consumes XY distance and
 * eye/at Z. The near-zero distance branch is scale(0,0,0), W=1, with NO
 * perspective concatenation. Otherwise use GAME synthetic look-at followed by
 * an independently written left-associated binary32 row multiply. Only rows
 * 0..2 are consumed by recipe 48; row 3 is computed but has no extra finite
 * admission requirement. Caller owns selector/retention. Failure leaves the
 * whole output unchanged; no allocation, and output may alias perspective.
 */
IsleGameResult isle_model_source48_basis(const IsleGameCameraParameters* camera,
    const float perspective[4][4], unsigned axis, float output[4][4]);

#ifdef __cplusplus
}
#endif
#endif
