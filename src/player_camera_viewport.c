#include "player_camera_viewport.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>

/* Avoid implementation-defined unsigned-to-signed narrowing while retaining
 * exactly the source assignment's low sixteen bits after signed truncation. */
static int16_t viewport_lane(float value) {
    const uint32_t low = (uint32_t)(int32_t)value & UINT32_C(0xffff);
    return (int16_t)(low <= INT16_MAX ? (int32_t)low : (int32_t)low - 65536);
}

IsleGameResult isle_player_camera_set_viewport_rect(IsleGame* game, IsleGameCamera* camera,
    float ulx, float uly, float lrx, float lry) {
    const IsleGameCameraParameters* parameters = NULL;
    IsleGameViewport viewport;
    float h, v, scale_x, scale_y, translate_x, translate_y;
    IsleGameResult result = isle_game_camera_parameters(game, camera, &parameters);
    if (result != ISLE_GAME_OK) return result;
    if (!isfinite(ulx) || !isfinite(uly) || !isfinite(lrx) || !isfinite(lry))
        return ISLE_GAME_INVALID_ARGUMENT;

    h = (ulx + lrx) / 2.0f;
    v = (uly + lry) / 2.0f;
    scale_x = (lrx - h) * 4.0f;
    scale_y = (lry - v) * 4.0f;
    translate_x = h * 4.0f;
    translate_y = v * 4.0f;
    /* 2^31 is representable as float, INT32_MAX is not: exclude the former
     * rather than comparing with an implicitly rounded INT32_MAX. */
    if (!isfinite(scale_x) || scale_x < -2147483648.0f || scale_x >= 2147483648.0f ||
        !isfinite(scale_y) || scale_y < -2147483648.0f || scale_y >= 2147483648.0f ||
        !isfinite(translate_x) || translate_x < -2147483648.0f || translate_x >= 2147483648.0f ||
        !isfinite(translate_y) || translate_y < -2147483648.0f || translate_y >= 2147483648.0f)
        return ISLE_GAME_INVALID_ARGUMENT;

    viewport = parameters->viewport;
    viewport.scale[0] = viewport_lane(scale_x);
    viewport.scale[1] = viewport_lane(scale_y);
    viewport.translate[0] = viewport_lane(translate_x);
    viewport.translate[1] = viewport_lane(translate_y);
    viewport.scale[2] = viewport.translate[2] = 511;
    return isle_game_camera_set_viewport(game, camera, &viewport);
}
