#ifndef ISLE_PHOTO_SCORE_SOURCE_H
#define ISLE_PHOTO_SCORE_SOURCE_H
#include "player_photo_source.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Own native scalar/handle ABI, not OMCamera/GObj/Gfx or an SDK layout. The
 * private complete score_Render body is admitted from the immutable GAME blob. */
typedef struct IslePhotoScoreSource IslePhotoScoreSource;
typedef struct IslePhotoScoreNative {
    IsleGameResult (*initialize)(void*, IslePhotoScoreSource*, IsleGameObject*, const IslePlayerPhotoData*);
    IsleGameResult (*viewport)(void*, IsleGameCamera*);
    IsleGameResult (*command)(void*, uint32_t, uint32_t);
    IsleGameResult (*prepare_camera)(void*, IsleGameCamera*);
    IsleGameResult (*first)(void*, int32_t, IsleGameObject**);
    IsleGameResult (*next)(void*, IsleGameObject*, IsleGameObject**);
    IsleGameResult (*object_tag)(void*, IsleGameObject*, uint32_t*);
    IsleGameResult (*draw)(void*, IsleGameObject*);
    IsleGameResult (*mark_drawn)(void*, IsleGameObject*, uint32_t);
    IsleGameResult (*post)(void*, IsleGameCamera*);
} IslePhotoScoreNative;
struct IslePhotoScoreSource {
    IsleGame* game;
    void* context;
    const IslePhotoScoreNative* native;
    IsleGameResult status;
    void* failure_frame;
    IsleGameObject* camera_object;
    IsleGameObject* current_camera;
    IsleGameObject* rendered_object;
    const IslePlayerPhotoData* current_photo;
    uint32_t image_buffer, current_z_buffer, pokemon_z_buffer[12], z_buffer_before[12];
    uint32_t setup_list, copy_list, link_mask, buffer_mask, camera_tag, drawn_frame_counter;
    IsleGameObject* photo_objects[12];
    int32_t photo_count;
    uint16_t ready, wide, pokemon_count;
};
IsleGameResult isle_photo_score_call(IslePhotoScoreSource*);
void isle_photo_score_render(IslePhotoScoreSource*, IsleGameObject*);
void isle_photo_score_initialize(IslePhotoScoreSource*, IsleGameObject*, const IslePlayerPhotoData*);
IsleGameCamera* isle_photo_score_camera(IslePhotoScoreSource*, IsleGameObject*);
void isle_photo_score_current_camera(IslePhotoScoreSource*, IsleGameObject*);
void isle_photo_score_flags(IslePhotoScoreSource*, IsleGameCamera*, uint32_t, int);
void isle_photo_score_fovy_scale(IslePhotoScoreSource*, IsleGameCamera*, float);
float isle_photo_score_aspect(IslePhotoScoreSource*, IsleGameCamera*);
void isle_photo_score_set_aspect(IslePhotoScoreSource*, IsleGameCamera*, float);
void isle_photo_score_viewport_component(IslePhotoScoreSource*, IsleGameCamera*, unsigned, unsigned, int16_t);
void isle_photo_score_viewport(IslePhotoScoreSource*, IsleGameCamera*);
void isle_photo_score_segment(IslePhotoScoreSource*, uint32_t, uint32_t);
void isle_photo_score_display_list(IslePhotoScoreSource*, uint32_t);
void isle_photo_score_prepare_camera(IslePhotoScoreSource*, IsleGameCamera*);
IsleGameObject* isle_photo_score_first(IslePhotoScoreSource*, int32_t);
IsleGameObject* isle_photo_score_next(IslePhotoScoreSource*, IsleGameObject*);
uint32_t isle_photo_score_object_flags(IslePhotoScoreSource*, IsleGameObject*);
uint32_t isle_photo_score_object_tag(IslePhotoScoreSource*, IsleGameObject*);
IsleGameObject* isle_photo_score_photo_object(IslePhotoScoreSource*, uint16_t);
void isle_photo_score_draw(IslePhotoScoreSource*, IsleGameObject*);
void isle_photo_score_mark_drawn(IslePhotoScoreSource*, IsleGameObject*, uint32_t);
void isle_photo_score_pipe_sync(IslePhotoScoreSource*);
void isle_photo_score_depth_restore(IslePhotoScoreSource*);
void isle_photo_score_post(IslePhotoScoreSource*, IsleGameCamera*);
#ifdef __cplusplus
}
#endif
#endif
