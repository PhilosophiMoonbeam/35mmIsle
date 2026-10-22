#include "photo_score_source.h"
#include <setjmp.h>

static void checked(IslePhotoScoreSource* source, IsleGameResult result) {
    if (result == ISLE_GAME_OK) return;
    if (source->status == ISLE_GAME_OK) source->status = result;
    if (source->failure_frame != NULL) longjmp(*(jmp_buf*)source->failure_frame, 1);
}
static void active(IslePhotoScoreSource* source) {
    checked(source, source->game != NULL && source->native != NULL &&
        source->context != NULL && source->failure_frame != NULL ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE);
    checked(source, source->status);
}
IsleGameResult isle_photo_score_call(IslePhotoScoreSource* source) {
    jmp_buf frame;
    if (source == NULL || source->game == NULL || source->native == NULL ||
        source->context == NULL || source->failure_frame != NULL || source->camera_object == NULL)
        return ISLE_GAME_INVALID_STATE;
    if (source->status != ISLE_GAME_OK) return source->status;
    source->failure_frame = &frame;
    if (setjmp(frame) == 0) isle_photo_score_render(source, source->camera_object);
    source->failure_frame = NULL;
    return source->status;
}
void isle_photo_score_initialize(IslePhotoScoreSource* source, IsleGameObject* camera, const IslePlayerPhotoData* photo) {
    active(source); checked(source, source->native->initialize(source->context, source, camera, photo));
}
IsleGameCamera* isle_photo_score_camera(IslePhotoScoreSource* source, IsleGameObject* object) {
    IsleGameCamera* camera = NULL;
    active(source); checked(source, isle_game_object_camera(source->game, object, &camera));
    return camera;
}
void isle_photo_score_current_camera(IslePhotoScoreSource* source, IsleGameObject* object) {
    active(source); source->current_camera = object;
}
void isle_photo_score_flags(IslePhotoScoreSource* source, IsleGameCamera* camera, uint32_t mask, int set) {
    IsleGameCameraRenderState state;
    active(source); checked(source, isle_game_camera_render_state(source->game, camera, &state));
    state.flags = set ? state.flags | mask : state.flags & ~mask;
    checked(source, isle_game_camera_set_render_state(source->game, camera, &state));
}
static IsleGamePerspective perspective(IslePhotoScoreSource* source, IsleGameCamera* camera) {
    const IsleGameCameraParameters* parameters = NULL;
    active(source); checked(source, isle_game_camera_parameters(source->game, camera, &parameters));
    return parameters->perspective;
}
void isle_photo_score_fovy_scale(IslePhotoScoreSource* source, IsleGameCamera* camera, float factor) {
    IsleGamePerspective value = perspective(source, camera);
    value.fovy *= factor;
    checked(source, isle_game_camera_set_perspective(source->game, camera, &value));
}
float isle_photo_score_aspect(IslePhotoScoreSource* source, IsleGameCamera* camera) {
    return perspective(source, camera).aspect;
}
void isle_photo_score_set_aspect(IslePhotoScoreSource* source, IsleGameCamera* camera, float aspect) {
    IsleGamePerspective value = perspective(source, camera);
    value.aspect = aspect;
    checked(source, isle_game_camera_set_perspective(source->game, camera, &value));
}
void isle_photo_score_viewport_component(IslePhotoScoreSource* source, IsleGameCamera* camera,
    unsigned translate, unsigned index, int16_t component) {
    const IsleGameCameraParameters* parameters = NULL;
    IsleGameViewport value;
    active(source); checked(source, isle_game_camera_parameters(source->game, camera, &parameters));
    checked(source, index < 4 && translate < 2 ? ISLE_GAME_OK : ISLE_GAME_INVALID_ARGUMENT);
    value = parameters->viewport;
    if (translate) value.translate[index] = component; else value.scale[index] = component;
    checked(source, isle_game_camera_set_viewport(source->game, camera, &value));
}
void isle_photo_score_viewport(IslePhotoScoreSource* source, IsleGameCamera* camera) {
    active(source); checked(source, source->native->viewport(source->context, camera));
}
static void command(IslePhotoScoreSource* source, uint32_t a, uint32_t b) {
    active(source); checked(source, source->native->command(source->context, a, b));
}
void isle_photo_score_segment(IslePhotoScoreSource* source, uint32_t segment, uint32_t address) {
    active(source); checked(source, segment < 16 ? ISLE_GAME_OK : ISLE_GAME_INVALID_ARGUMENT);
    command(source, UINT32_C(0xdb060000) | segment * 4, address);
}
void isle_photo_score_display_list(IslePhotoScoreSource* source, uint32_t address) {
    command(source, UINT32_C(0xde000000), address);
}
void isle_photo_score_prepare_camera(IslePhotoScoreSource* source, IsleGameCamera* camera) {
    active(source); checked(source, source->native->prepare_camera(source->context, camera));
}
IsleGameObject* isle_photo_score_first(IslePhotoScoreSource* source, int32_t link) {
    IsleGameObject* object = NULL;
    active(source); checked(source, source->native->first(source->context, link, &object)); return object;
}
IsleGameObject* isle_photo_score_next(IslePhotoScoreSource* source, IsleGameObject* current) {
    IsleGameObject* object = NULL;
    active(source); checked(source, source->native->next(source->context, current, &object)); return object;
}
uint32_t isle_photo_score_object_flags(IslePhotoScoreSource* source, IsleGameObject* object) {
    IsleGameObjectMetadata metadata;
    active(source); checked(source, isle_game_object_metadata(source->game, object, &metadata)); return metadata.flags;
}
uint32_t isle_photo_score_object_tag(IslePhotoScoreSource* source, IsleGameObject* object) {
    uint32_t tag = 0;
    active(source); checked(source, source->native->object_tag(source->context, object, &tag)); return tag;
}
void isle_photo_score_draw(IslePhotoScoreSource* source, IsleGameObject* object) {
    active(source); checked(source, source->native->draw(source->context, object));
}
void isle_photo_score_mark_drawn(IslePhotoScoreSource* source, IsleGameObject* object, uint32_t frame) {
    active(source); checked(source, source->native->mark_drawn(source->context, object, frame));
}
void isle_photo_score_pipe_sync(IslePhotoScoreSource* source) { command(source, UINT32_C(0xe7000000), 0); }
void isle_photo_score_depth_restore(IslePhotoScoreSource* source) { command(source, UINT32_C(0xfe000000), UINT32_C(0x0c000000)); }
void isle_photo_score_post(IslePhotoScoreSource* source, IsleGameCamera* camera) {
    active(source); checked(source, source->native->post(source->context, camera));
}
IsleGameObject* isle_photo_score_photo_object(IslePhotoScoreSource* source, uint16_t index) {
    active(source);
    /* Original compares the pointer before its count condition. A native
     * exhausted slot has no object; do not read beyond the owned twelve. */
    return index < 12 ? source->photo_objects[index] : NULL;
}
