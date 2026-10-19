#ifndef ISLE_CREDITS_WINDOW_EXTENSION_H
#define ISLE_CREDITS_WINDOW_EXTENSION_H
#include "post_course_window_source.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Implemented by the precise private Window patch. Only Credits resource kind6
 * and original photo root0x801e6058 are admitted. Eight bitmap descriptors are
 * cloned with independent3920-byte backing, even if original buffers alias.
 * Native scalar placement/tint is preserved; no thumbnail-child restriction.
 * Called at actual Original UIMem allocation sites after graphics retirement. */
IsleGameResult isle_post_window_credits_image_create(IslePostCourseWindow*,IsleGameSprite*,uint32_t,size_t,size_t,IslePostCourseThumbnail*);
/* Acquired queue/inflight borrows are bounded by FIFO10+one inflight. A held
 * incarnation cannot be released/rebound. Real object join invalidates resolve
 * but keeps its raster/bitmap backing alive until every actual hold is dropped.
 * These calls stay on constructing owner, contain no Source/renderer/ticks and
 * never allocate. release_hold remains available after sticky cancellation. */
IsleGameResult isle_post_window_credits_image_retain(IslePostCourseWindow*,IslePostCourseThumbnail);
IsleGameResult isle_post_window_credits_image_resolve(IslePostCourseWindow*,IslePostCourseThumbnail,IsleGameSprite**,int*,int*);
IsleGameResult isle_post_window_credits_image_release_hold(IslePostCourseWindow*,IslePostCourseThumbnail);
/* Before Original ohRemoveSprite: invalidate only this joined queue's Sprite
 * aliases before native pool reuse, retaining actual raster backing for join. */
IsleGameResult isle_post_window_credits_image_detach_object(IslePostCourseWindow*,IsleGameObject*);
#ifdef __cplusplus
}
#endif
#endif
