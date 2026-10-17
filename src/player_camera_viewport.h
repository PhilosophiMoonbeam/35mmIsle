#ifndef ISLE_PLAYER_CAMERA_VIEWPORT_H
#define ISLE_PLAYER_CAMERA_VIEWPORT_H

#include "35mmisle/engine.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Convert own-GAME rectangle coordinates to native quarter-pixel lanes.
 * Midpoints and truncation precede signed low-16-bit retention; Z becomes 511
 * and reserved lanes are preserved. Native owner/status and viewport admission
 * errors return unchanged, with no camera mutation on any rejection. */
IsleGameResult isle_player_camera_set_viewport_rect(IsleGame*, IsleGameCamera*,
    float ulx, float uly, float lrx, float lry);

#ifdef __cplusplus
}
#endif
#endif
