#ifndef ISLE_PLAYER_CAMERA_ANIMATION_CLIP_H
#define ISLE_PLAYER_CAMERA_ANIMATION_CLIP_H
#include <stdint.h>
/* Issued only by the associated live native camera driver. Never a guest
 * address or an anonymous catalog index. Copyable; invalid after retirement.
 * A NULL pointer to this record denotes the source NULL animation list. */
typedef struct IslePlayerCameraAnimationClip {
    uint64_t owner;
    uint64_t index;
} IslePlayerCameraAnimationClip;
#endif
