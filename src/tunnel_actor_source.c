#include "tunnel_actor_source.h"
#include "engine_internal.h"
#include <stdlib.h>
#include <string.h>

static int services_complete(const IsleTunnelActorServices* p) {
    return p && p->reference_assign && p->source && p->model &&
        p->model_transform && p->model_store && p->node && p->node_transform &&
        p->node_world && p->misc_object_get && p->misc_object_set &&
        p->current_block && p->add_one && p->add_at_geo && p->weighted &&
        p->header && p->sound_play && p->sound_play_params && p->sound_identity &&
        p->sound_stop && p->sound_stop_all && p->sound_volume &&
        p->sound_global_volume && p->sound_global_reverb && p->bgm_play &&
        p->bgm_stop && p->bgm_volume && p->bgm_volume_smooth &&
        p->bgm_channel_volume && p->bgm_reverb && p->remove_flying_items &&
        p->reset_camera && p->reset_messages && p->remove_pokemon &&
        p->cutscene_start && p->resume_processes && p->pause_processes &&
        p->main_camera && p->camera_speed && p->camera_animation &&
        p->camera_animation_process && p->sky_speed && p->player_model_init &&
        p->player_model_animation && p->player_model_change_animation &&
        p->player_model_speed && p->model_speed && p->animation_callback &&
        p->collision_initialize && p->collision_query && p->collision_callback &&
        p->level_progress && p->environment_sound && p->ground &&
        p->background_music &&
        p->notice && p->player_model && p->player_object && p->random_range &&
        p->path_count && p->path_point && p->path_param && p->block_position &&
        p->camera_look_read && p->camera_look_store && p->command &&
        p->command_link && p->flute_command &&
        p->vector_distance && p->matrix_rts && p->matrix_cat &&
        p->sound_pan_ramp && p->sound_volume_ramp && p->sound_ramp_reset &&
        p->fade_status && p->fade_color && p->fade_duration && p->fade_start &&
        p->progress_get && p->progress_set && p->helper_create &&
        p->helper_delete && p->camera_object && p->spawn_effect &&
        p->sky_default_speed && p->root_position;
}
static IsleGameResult live_status(IsleGame* game) {
    return isle_actor_source_execution_status(game);
}
void isle_tunnel_actor_checked(IsleTunnelActorSource* s, IsleGameResult result) {
    if (!s) abort();
    isle_course_actor_checked(&s->base, result);
}
void isle_tunnel_actor_enter(IsleTunnelActorSource* s) {
    if (!s) abort();
    if (s->base.phase != ISLE_COURSE_ACTOR_ACTIVE || s->base.course_id != 1 ||
        !s->base.owner || !s->base.nativeGame || !services_complete(s->services))
        isle_tunnel_actor_checked(s, ISLE_GAME_INVALID_STATE);
    isle_tunnel_actor_checked(s, s->base.status);
    isle_tunnel_actor_checked(s, live_status(s->base.nativeGame));
    isle_tunnel_actor_checked(s, isle_tunnel_actor_native_admission(s, s->base.nativeGame, s->services));
}
IsleTunnelActorSource* isle_tunnel_actor_require(IsleGameObject* object) {
    IsleTunnelActorSource* s = NULL;
    IsleGameResult result;
    if (!object) abort();
    result = isle_tunnel_actor_native_context(object->game, object, &s);
    if (result != ISLE_GAME_OK || !s)
        isle_native_execution_fault(object->game, "Tunnel Source requires its canonical receiver context");
    isle_tunnel_actor_enter(s);
    return s;
}
IsleGameResult isle_tunnel_actor_activate(IsleTunnelActorSource* s, IsleGame* game,
    const IsleTunnelActorServices* services) {
    IsleGameResult result;
    if (!s || !game || !services_complete(services)) return ISLE_GAME_INVALID_ARGUMENT;
    if (s->base.phase != ISLE_COURSE_ACTOR_PREPARED || s->base.course_id != 1 ||
        !s->base.owner || s->base.nativeGame || s->services || s->base.nonwaiting_call_frame)
        return ISLE_GAME_INVALID_STATE;
    if (s->base.status != ISLE_GAME_OK) return s->base.status;
    result = isle_game_require_idle(game);
    if (result == ISLE_GAME_OK) result = live_status(game);
    if (result == ISLE_GAME_OK) result = isle_tunnel_actor_native_admission(s, game, services);
    if (result != ISLE_GAME_OK) return result;
    s->base.nativeGame = game;
    s->services = services;
    s->base.phase = ISLE_COURSE_ACTOR_ACTIVE;
    return ISLE_GAME_OK;
}
IsleGameResult isle_tunnel_actor_deactivate(IsleTunnelActorSource* s) {
    IsleGameResult result, status;
    if (!s || s->base.phase != ISLE_COURSE_ACTOR_ACTIVE || s->base.course_id != 1 ||
        !s->base.owner || !s->base.nativeGame || !services_complete(s->services) ||
        s->base.nonwaiting_call_frame) return ISLE_GAME_INVALID_STATE;
    /* Native retirement validates actual queue/process/reference/model/FX/GPU/
     * Photo/audio joins; idle counts are not a retirement certificate. */
    result = isle_tunnel_actor_native_retirement(s);
    if (result != ISLE_GAME_OK) return result;
    status = live_status(s->base.nativeGame);
    if (s->base.status == ISLE_GAME_OK && status != ISLE_GAME_OK) s->base.status = status;
    s->base.nativeGame = NULL;
    s->services = NULL;
    s->base.phase = ISLE_COURSE_ACTOR_DETACHED;
    return ISLE_GAME_OK;
}
IsleGameResult isle_tunnel_actor_invoke(IsleTunnelActorSource* s, void (*call)(void*), void* payload) {
    IsleGameResult result;
    if (!s || !call || s->base.phase != ISLE_COURSE_ACTOR_ACTIVE ||
        s->base.course_id != 1 || !s->base.nativeGame || !services_complete(s->services))
        return ISLE_GAME_INVALID_STATE;
    if (s->base.status != ISLE_GAME_OK) return s->base.status;
    result = live_status(s->base.nativeGame);
    if (result == ISLE_GAME_OK)
        result = isle_tunnel_actor_native_admission(s, s->base.nativeGame, s->services);
    if (result != ISLE_GAME_OK) {
        if (s->base.status == ISLE_GAME_OK) s->base.status = result;
        return result;
    }
    return isle_course_actor_invoke(&s->base, call, payload);
}

/* Typed wrappers contain no native stack ownership across checked(). */
void tunnel_reference_assign(IsleTunnelActorSource* s, IsleGameObject** field, IsleGameObject* value) {
    isle_tunnel_actor_checked(s, s->services->reference_assign(s, field, value));
}
IsleActorSource* tunnel_source(IsleTunnelActorSource* s, IsleGameObject* object) {
    IsleActorSource* out = {0};
    isle_tunnel_actor_checked(s, s->services->source(s, object, &out));
    return out;
}
IsleGameModelRef tunnel_model(IsleTunnelActorSource* s, IsleGameObject* object) {
    IsleGameModelRef out = {0};
    isle_tunnel_actor_checked(s, s->services->model(s, object, &out));
    return out;
}
const IsleGameModelTransform* tunnel_model_transform(IsleTunnelActorSource* s, const IsleGameModelRef* model) {
    const IsleGameModelTransform* out = {0};
    isle_tunnel_actor_checked(s, s->services->model_transform(s, model, &out));
    return out;
}
void tunnel_model_store(IsleTunnelActorSource* s, const IsleGameModelRef* model, uint32_t component, float value) {
    isle_tunnel_actor_checked(s, s->services->model_store(s, model, component, value));
}
void tunnel_node(IsleTunnelActorSource* s, IsleGameObject* object, const uint16_t* path, size_t count, IsleActorNodeRef* out, int32_t* present) {
    isle_tunnel_actor_checked(s, s->services->node(s, object, path, count, out, present));
}
IsleGameModelTransform tunnel_node_transform(IsleTunnelActorSource* s, const IsleActorNodeRef* node) {
    IsleGameModelTransform out = {0};
    isle_tunnel_actor_checked(s, s->services->node_transform(s, node, &out));
    return out;
}
void tunnel_node_world(IsleTunnelActorSource* s, const IsleActorNodeRef* node, IsleGameVec3* position, IsleGameVec3* velocity) {
    isle_tunnel_actor_checked(s, s->services->node_world(s, node, position, velocity));
}
IsleGameObject* tunnel_misc_object_get(IsleTunnelActorSource* s, IsleGameObject* object, uint32_t index) {
    IsleGameObject* out = {0};
    isle_tunnel_actor_checked(s, s->services->misc_object_get(s, object, index, &out));
    return out;
}
void tunnel_misc_object_set(IsleTunnelActorSource* s, IsleGameObject* object, uint32_t index, IsleGameObject* value) {
    isle_tunnel_actor_checked(s, s->services->misc_object_set(s, object, index, value));
}
void* tunnel_current_block(IsleTunnelActorSource* s) {
    void* out = {0};
    isle_tunnel_actor_checked(s, s->services->current_block(s, &out));
    return out;
}
IsleGameObject* tunnel_add_one(IsleTunnelActorSource* s, void* block, void* block_b, const IsleTunnelAutomaticSpawn* spawn, const IsleTunnelDefinition* definition) {
    IsleGameObject* out = {0};
    isle_tunnel_actor_checked(s, s->services->add_one(s, block, block_b, spawn, definition, &out));
    return out;
}
IsleGameObject* tunnel_add_at_geo(IsleTunnelActorSource* s, IsleGameObject* object, uint16_t id, const IsleTunnelDefinition* definition) {
    IsleGameObject* out = {0};
    isle_tunnel_actor_checked(s, s->services->add_at_geo(s, object, id, definition, &out));
    return out;
}
const IsleActorRandomState* tunnel_weighted(IsleTunnelActorSource* s, uint32_t identity) {
    const IsleActorRandomState* out = {0};
    isle_tunnel_actor_checked(s, s->services->weighted(s, identity, &out));
    return out;
}
const IsleTunnelAnimationHeader* tunnel_header(IsleTunnelActorSource* s, uint32_t identity) {
    const IsleTunnelAnimationHeader* out = {0};
    isle_tunnel_actor_checked(s, s->services->header(s, identity, &out));
    return out;
}
int32_t tunnel_sound_play(IsleTunnelActorSource* s, int32_t id) {
    int32_t out = {0};
    isle_tunnel_actor_checked(s, s->services->sound_play(s, id, &out));
    return out;
}
int32_t tunnel_sound_play_params(IsleTunnelActorSource* s, int32_t id, int32_t volume, int32_t pan, float pitch, int32_t reverb) {
    int32_t out = {0};
    isle_tunnel_actor_checked(s, s->services->sound_play_params(s, id, volume, pan, pitch, reverb, &out));
    return out;
}
int32_t tunnel_sound_identity(IsleTunnelActorSource* s, int32_t handle) {
    int32_t out = {0};
    isle_tunnel_actor_checked(s, s->services->sound_identity(s, handle, &out));
    return out;
}
void tunnel_sound_stop(IsleTunnelActorSource* s, int32_t handle) {
    isle_tunnel_actor_checked(s, s->services->sound_stop(s, handle));
}
void tunnel_sound_stop_all(IsleTunnelActorSource* s) {
    isle_tunnel_actor_checked(s, s->services->sound_stop_all(s));
}
void tunnel_sound_volume(IsleTunnelActorSource* s, int32_t handle, int32_t volume) {
    isle_tunnel_actor_checked(s, s->services->sound_volume(s, handle, volume));
}
void tunnel_sound_global_volume(IsleTunnelActorSource* s, int32_t volume) {
    isle_tunnel_actor_checked(s, s->services->sound_global_volume(s, volume));
}
void tunnel_sound_global_reverb(IsleTunnelActorSource* s, int32_t reverb) {
    isle_tunnel_actor_checked(s, s->services->sound_global_reverb(s, reverb));
}
void tunnel_bgm_play(IsleTunnelActorSource* s, int32_t player, int32_t song) {
    isle_tunnel_actor_checked(s, s->services->bgm_play(s, player, song));
}
void tunnel_bgm_stop(IsleTunnelActorSource* s, int32_t player) {
    isle_tunnel_actor_checked(s, s->services->bgm_stop(s, player));
}
void tunnel_bgm_volume(IsleTunnelActorSource* s, int32_t player, uint32_t volume) {
    isle_tunnel_actor_checked(s, s->services->bgm_volume(s, player, volume));
}
void tunnel_bgm_volume_smooth(IsleTunnelActorSource* s, int32_t player, uint32_t volume, int32_t frames) {
    isle_tunnel_actor_checked(s, s->services->bgm_volume_smooth(s, player, volume, frames));
}
void tunnel_bgm_channel_volume(IsleTunnelActorSource* s, int32_t player, uint8_t channel, int32_t volume) {
    isle_tunnel_actor_checked(s, s->services->bgm_channel_volume(s, player, channel, volume));
}
void tunnel_bgm_reverb(IsleTunnelActorSource* s, int32_t player, int32_t reverb) {
    isle_tunnel_actor_checked(s, s->services->bgm_reverb(s, player, reverb));
}
void tunnel_remove_flying_items(IsleTunnelActorSource* s) {
    isle_tunnel_actor_checked(s, s->services->remove_flying_items(s));
}
void tunnel_reset_camera(IsleTunnelActorSource* s) {
    isle_tunnel_actor_checked(s, s->services->reset_camera(s));
}
void tunnel_reset_messages(IsleTunnelActorSource* s) {
    isle_tunnel_actor_checked(s, s->services->reset_messages(s));
}
void tunnel_remove_pokemon(IsleTunnelActorSource* s, const uint16_t* keep, size_t count) {
    isle_tunnel_actor_checked(s, s->services->remove_pokemon(s, keep, count));
}
void tunnel_cutscene_start(IsleTunnelActorSource* s, IsleGameObject* object, uint32_t animation_identity, float time) {
    isle_tunnel_actor_checked(s, s->services->cutscene_start(s, object, animation_identity, time));
}
void tunnel_resume_processes(IsleTunnelActorSource* s, IsleGameObject* object) {
    isle_tunnel_actor_checked(s, s->services->resume_processes(s, object));
}
void tunnel_pause_processes(IsleTunnelActorSource* s, IsleGameObject* object) {
    isle_tunnel_actor_checked(s, s->services->pause_processes(s, object));
}
void tunnel_main_camera(IsleTunnelActorSource* s, IsleGameCamera** out, IsleGameObject** object) {
    isle_tunnel_actor_checked(s, s->services->main_camera(s, out, object));
}
void tunnel_camera_speed(IsleTunnelActorSource* s, IsleGameCamera* camera, float speed) {
    isle_tunnel_actor_checked(s, s->services->camera_speed(s, camera, speed));
}
void tunnel_camera_animation(IsleTunnelActorSource* s, IsleGameCamera* camera, uint32_t identity, float time) {
    isle_tunnel_actor_checked(s, s->services->camera_animation(s, camera, identity, time));
}
void tunnel_camera_animation_process(IsleTunnelActorSource* s, IsleGameObject* object, int32_t kind, int32_t priority) {
    isle_tunnel_actor_checked(s, s->services->camera_animation_process(s, object, kind, priority));
}
void tunnel_sky_speed(IsleTunnelActorSource* s, float speed) {
    isle_tunnel_actor_checked(s, s->services->sky_speed(s, speed));
}
IsleGameObject* tunnel_player_model_init(IsleTunnelActorSource* s) {
    IsleGameObject* out = {0};
    isle_tunnel_actor_checked(s, s->services->player_model_init(s, &out));
    return out;
}
void tunnel_player_model_animation(IsleTunnelActorSource* s, uint32_t model, uint32_t material, float time, float speed) {
    isle_tunnel_actor_checked(s, s->services->player_model_animation(s, model, material, time, speed));
}
void tunnel_player_model_change_animation(IsleTunnelActorSource* s, uint32_t model, uint32_t material, float time) {
    isle_tunnel_actor_checked(s, s->services->player_model_change_animation(s, model, material, time));
}
void tunnel_player_model_speed(IsleTunnelActorSource* s, float speed) {
    isle_tunnel_actor_checked(s, s->services->player_model_speed(s, speed));
}
void tunnel_model_speed(IsleTunnelActorSource* s, IsleGameObject* object, float speed) {
    isle_tunnel_actor_checked(s, s->services->model_speed(s, object, speed));
}
void tunnel_animation_callback(IsleTunnelActorSource* s, IsleGameObject* object, IsleTunnelAnimationCallback callback) {
    isle_tunnel_actor_checked(s, s->services->animation_callback(s, object, callback));
}
void tunnel_collision_initialize(IsleTunnelActorSource* s, IsleTunnelCollision* collision) {
    isle_tunnel_actor_checked(s, s->services->collision_initialize(s, collision));
}
void tunnel_collision_query(IsleTunnelActorSource* s, IsleGameVec3* in_position, IsleGameVec3* in_velocity, IsleGameVec3* out_position, IsleGameVec3* out_velocity, const IsleTunnelCollision* collision, const IsleGameVec3* position, const IsleGameVec3* rotation, int32_t* hit) {
    isle_tunnel_actor_checked(s, s->services->collision_query(s, in_position, in_velocity, out_position, out_velocity, collision, position, rotation, hit));
}
void tunnel_collision_callback(IsleTunnelActorSource* s, IsleTunnelCollisionCallback callback) {
    isle_tunnel_actor_checked(s, s->services->collision_callback(s, callback));
}
void tunnel_level_progress(IsleTunnelActorSource* s, int32_t* block, float* fraction) {
    isle_tunnel_actor_checked(s, s->services->level_progress(s, block, fraction));
}
void tunnel_environment_sound(IsleTunnelActorSource* s, IsleGameObject* object, uint8_t category, int32_t sound) {
    isle_tunnel_actor_checked(s, s->services->environment_sound(s, object, category, sound));
}
IsleActorGround tunnel_ground(IsleTunnelActorSource* s, float x, float z) {
    IsleActorGround out = {0};
    isle_tunnel_actor_checked(s, s->services->ground(s, x, z, &out));
    return out;
}
void tunnel_background_music(IsleTunnelActorSource* s, int32_t song) {
    isle_tunnel_actor_checked(s, s->services->background_music(s, song));
}
void tunnel_notice(IsleTunnelActorSource* s, int32_t bits) {
    isle_tunnel_actor_checked(s, s->services->notice(s, bits));
}
IsleGameModelRef tunnel_player_model(IsleTunnelActorSource* s) {
    IsleGameModelRef out = {0};
    isle_tunnel_actor_checked(s, s->services->player_model(s, &out));
    return out;
}
IsleGameObject* tunnel_player_object(IsleTunnelActorSource* s) {
    IsleGameObject* out = {0};
    isle_tunnel_actor_checked(s, s->services->player_object(s, &out));
    return out;
}
int32_t tunnel_random_range(IsleTunnelActorSource* s, int32_t range) {
    int32_t out = {0};
    isle_tunnel_actor_checked(s, s->services->random_range(s, range, &out));
    return out;
}
int32_t tunnel_path_count(IsleTunnelActorSource* s, const void* path) {
    int32_t out = {0};
    isle_tunnel_actor_checked(s, s->services->path_count(s, path, &out));
    return out;
}
IsleGameVec3 tunnel_path_point(IsleTunnelActorSource* s, const void* path, int32_t index) {
    IsleGameVec3 out = {0};
    isle_tunnel_actor_checked(s, s->services->path_point(s, path, index, &out));
    return out;
}
float tunnel_path_param(IsleTunnelActorSource* s, const void* path, int32_t index) {
    float out = {0};
    isle_tunnel_actor_checked(s, s->services->path_param(s, path, index, &out));
    return out;
}
IsleGameVec3 tunnel_block_position(IsleTunnelActorSource* s, const void* block) {
    IsleGameVec3 out = {0};
    isle_tunnel_actor_checked(s, s->services->block_position(s, block, &out));
    return out;
}
IsleGameVec3 tunnel_camera_look_read(IsleTunnelActorSource* s, int32_t eye) {
    IsleGameVec3 out = {0};
    isle_tunnel_actor_checked(s, s->services->camera_look_read(s, eye, &out));
    return out;
}
void tunnel_camera_look_store(IsleTunnelActorSource* s, int32_t eye, uint32_t axis, float value) {
    isle_tunnel_actor_checked(s, s->services->camera_look_store(s, eye, axis, value));
}
void tunnel_command(IsleTunnelActorSource* s, IsleGameObject* target, int32_t command, IsleGameObject* sender) {
    isle_tunnel_actor_checked(s, s->services->command(s, target, command, sender));
}
void tunnel_command_link(IsleTunnelActorSource* s, int32_t link, int32_t command, IsleGameObject* sender) {
    isle_tunnel_actor_checked(s, s->services->command_link(s, link, command, sender));
}
int32_t tunnel_flute_command(IsleTunnelActorSource* s) {
    int32_t out = {0};
    isle_tunnel_actor_checked(s, s->services->flute_command(s, &out));
    return out;
}
float tunnel_vector_distance(IsleTunnelActorSource* s, const IsleGameVec3* a, const IsleGameVec3* b) {
    float out = {0};
    isle_tunnel_actor_checked(s, s->services->vector_distance(s, a, b, &out));
    return out;
}
void tunnel_matrix_rts(IsleTunnelActorSource* s, float matrix[4][4], float x, float y, float z, float rx, float ry, float rz, float sx, float sy, float sz) {
    isle_tunnel_actor_checked(s, s->services->matrix_rts(s, matrix, x, y, z, rx, ry, rz, sx, sy, sz));
}
void tunnel_matrix_cat(IsleTunnelActorSource* s, const float a[4][4], const float b[4][4], float out[4][4]) {
    isle_tunnel_actor_checked(s, s->services->matrix_cat(s, a, b, out));
}
int32_t tunnel_sound_pan_ramp(IsleTunnelActorSource* s, int32_t handle, uint32_t start, uint32_t end, int32_t frames) {
    int32_t out = {0};
    isle_tunnel_actor_checked(s, s->services->sound_pan_ramp(s, handle, start, end, frames, &out));
    return out;
}
int32_t tunnel_sound_volume_ramp(IsleTunnelActorSource* s, int32_t handle, uint32_t start, uint32_t end, int32_t frames) {
    int32_t out = {0};
    isle_tunnel_actor_checked(s, s->services->sound_volume_ramp(s, handle, start, end, frames, &out));
    return out;
}
void tunnel_sound_ramp_reset(IsleTunnelActorSource* s) {
    isle_tunnel_actor_checked(s, s->services->sound_ramp_reset(s));
}
int32_t tunnel_fade_status(IsleTunnelActorSource* s) {
    int32_t out = {0};
    isle_tunnel_actor_checked(s, s->services->fade_status(s, &out));
    return out;
}
void tunnel_fade_color(IsleTunnelActorSource* s, int32_t red, int32_t green, int32_t blue) {
    isle_tunnel_actor_checked(s, s->services->fade_color(s, red, green, blue));
}
void tunnel_fade_duration(IsleTunnelActorSource* s, int32_t mode, float duration) {
    isle_tunnel_actor_checked(s, s->services->fade_duration(s, mode, duration));
}
void tunnel_fade_start(IsleTunnelActorSource* s, int32_t mode, int32_t flag) {
    isle_tunnel_actor_checked(s, s->services->fade_start(s, mode, flag));
}
int32_t tunnel_progress_get(IsleTunnelActorSource* s) {
    int32_t out = {0};
    isle_tunnel_actor_checked(s, s->services->progress_get(s, &out));
    return out;
}
void tunnel_progress_set(IsleTunnelActorSource* s, int32_t value) {
    isle_tunnel_actor_checked(s, s->services->progress_set(s, value));
}
IsleGameObject* tunnel_helper_create(IsleTunnelActorSource* s, int32_t id, int32_t link, uint32_t priority) {
    IsleGameObject* out = {0};
    isle_tunnel_actor_checked(s, s->services->helper_create(s, id, link, priority, &out));
    return out;
}
void tunnel_helper_delete(IsleTunnelActorSource* s, IsleGameObject* object) {
    isle_tunnel_actor_checked(s, s->services->helper_delete(s, object));
}
IsleGameObject* tunnel_camera_object(IsleTunnelActorSource* s, IsleGameCamera* camera) {
    IsleGameObject* out = {0};
    isle_tunnel_actor_checked(s, s->services->camera_object(s, camera, &out));
    return out;
}
IsleGameObject* tunnel_spawn_effect(IsleTunnelActorSource* s, IsleGameObject* object) {
    IsleGameObject* out = {0};
    isle_tunnel_actor_checked(s, s->services->spawn_effect(s, object, &out));
    return out;
}
float tunnel_sky_default_speed(IsleTunnelActorSource* s) {
    float out = {0};
    isle_tunnel_actor_checked(s, s->services->sky_default_speed(s, &out));
    return out;
}
IsleGameVec3 tunnel_root_position(IsleTunnelActorSource* s, const IsleGameModelRef* model) {
    IsleGameVec3 out = {0};
    isle_tunnel_actor_checked(s, s->services->root_position(s, model, &out));
    return out;
}

IsleTunnelActorSource* tunnel_factory_context(const IsleActorFactoryBlock* a,
    const IsleActorFactoryBlock* b, const IsleActorFactorySpawn* spawn) {
    IsleTunnelActorSource* s = NULL;
    IsleGameResult result = isle_tunnel_actor_native_factory_context(a, b, spawn, &s);
    /* Resolve/latch errors only through the actual validated factory ticket. */
    isle_actor_source_factory_checked(a, b, spawn,
        result == ISLE_GAME_OK && !s ? ISLE_GAME_INVALID_STATE : result);
    isle_tunnel_actor_enter(s);
    return s;
}
IsleActorNodeRef tunnel_node_ref(IsleTunnelActorSource* s, IsleGameObject* object,
    const uint16_t* path, size_t count) {
    IsleActorNodeRef out = {0};
    int32_t present = 0;
    tunnel_node(s, object, path, count, &out, &present);
    if (!present) isle_tunnel_actor_checked(s, ISLE_GAME_INVALID_STATE);
    return out;
}
int32_t tunnel_node_present(IsleTunnelActorSource* s, IsleGameObject* object,
    const uint16_t* path, size_t count) {
    IsleActorNodeRef out = {0};
    int32_t present = 0;
    if (!object) return 0;
    tunnel_node(s, object, path, count, &out, &present);
    return present;
}
IsleGameCamera* tunnel_camera(IsleTunnelActorSource* s) {
    IsleGameCamera* camera = NULL;
    IsleGameObject* object = NULL;
    tunnel_main_camera(s, &camera, &object);
    return camera;
}
void tunnel_registry(IsleTunnelActorSource* s, int32_t index, IsleGameObject* object) {
    /* Native admission proves actual World/setup behavior bounds as well. */
    if (index < 0 || index >= 3) isle_tunnel_actor_checked(s, ISLE_GAME_INVALID_STATE);
    tunnel_reference_assign(s, &s->D_802EF9F4_5ECAC4[index], object);
}
float tunnel_player_distance(IsleTunnelActorSource* s, const IsleGameModelRef* model) {
    IsleGameModelRef player = tunnel_player_model(s);
    IsleGameVec3 a = tunnel_model_transform(s, &player)->position;
    IsleGameVec3 b = tunnel_root_position(s, model);
    return tunnel_vector_distance(s, &a, &b);
}
void tunnel_copy_graph(IsleTunnelActorSource* s, IsleGameObject* object,
    uint32_t identity, IsleActorInteraction* rows, size_t count) {
    const IsleActorInteractionGraph* graph = isle_actor_source_graph(object, identity);
    if (!graph || graph->count != count || !graph->rows)
        isle_tunnel_actor_checked(s, ISLE_GAME_INVALID_STATE);
    /* Destination is the actual continuation's C stack, never host scratch. */
    memcpy(rows, graph->rows, count * sizeof(*rows));
}
int32_t tunnel_collide(IsleTunnelActorSource* s, IsleGameVec3* a, IsleGameVec3* b,
    IsleGameVec3* c, IsleGameVec3* d, const IsleTunnelCollision* collision,
    const IsleGameVec3* position, const IsleGameVec3* rotation) {
    int32_t hit = 0;
    tunnel_collision_query(s, a, b, c, d, collision, position, rotation, &hit);
    return hit;
}

/* Exact admitted-ROM data; offsets use the qualified matching-map segment.
 * Pointer words below are checked/retained as provenance, never converted. */
static uint32_t tunnel_prepare_u32(const uint8_t* p) {
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
}
static int32_t tunnel_prepare_i32(const uint8_t* p) {
    uint32_t bits = tunnel_prepare_u32(p);
    int32_t value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}
static float tunnel_prepare_f32(const uint8_t* p) {
    uint32_t bits = tunnel_prepare_u32(p);
    float value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}
IsleGameResult isle_tunnel_actor_prepare(IsleTunnelActorSource* s, void* owner,
    const uint8_t* rom, size_t bytes, const IsleEggPreparation* egg_preparation) {
    /* Qualified638z fields plus exact Source initializers. Widths preserve
     * byte/halfword objects rather than borrowing adjacent alignment bytes. */
    static const struct { uint32_t rom, bits; uint8_t width; } fields[] = {
        {0x5ec554u, 0x00000000u, 4}, {0x5ec558u, 0x00000000u, 4}, {0x5ec77cu, 0x00000000u, 4},
        {0x5ed0a0u, 0x00000000u, 4}, {0x5ed140u, 0x00000000u, 4}, {0x5ed230u, 0xffffffffu, 4},
        {0x5ed234u, 0x00000000u, 4}, {0x5ed258u, 0x03e90000u, 4}, {0x5ecac0u, 0x00000000u, 4},
        {0x5ecac4u, 0x00000000u, 4}, {0x5ecac8u, 0x00000000u, 4}, {0x5ecaccu, 0x00000000u, 4},
        {0x5ed2d4u, 0x3dcccccdu, 4}, {0x5ed2d8u, 0x3f000000u, 4}, {0x5ed2dcu, 0x3f800000u, 4},
        {0x5ed2e0u, 0x00000000u, 4}, {0x5ed2e4u, 0x00000000u, 4}, {0x5ed2e8u, 0x00000000u, 4},
        {0x5ed2f0u, 0xbf800000u, 4}, {0x5ed2f4u, 0xbf800000u, 4}, {0x5ecc74u, 0x00000000u, 4},
        {0x5ecff8u, 0x00000000u, 4}, {0x5ecfe4u, 0x80188d28u, 4}, {0x5ecfe8u, 0x802eff0cu, 4},
        {0x5ecfecu, 0x00000150u, 4}, {0x5ecff0u, 0x00000002u, 4}, {0x5ecff4u, 0x00000000u, 4},
        {0x5ed03cu, 0x0000001eu, 4}, {0x5ed040u, 0x00000028u, 4}, {0x5ed044u, 0x00000028u, 4},
        {0x5ed048u, 0x00000028u, 4}, {0x5ed04cu, 0x00000028u, 4}, {0x5eb97cu, 0x00000000u, 4},
        {0x5eb354u, 0x00000000u, 4}, {0x5eb358u, 0x00000000u, 4}, {0x5eb35cu, 0x00000000u, 4},
        {0x5eb360u, 0x00000000u, 4}, {0x5eb364u, 0x00000000u, 4}, {0x5eb370u, 0x00000000u, 4},
        {0x5eb1dcu, 0x00000000u, 2}, {0x5ebf90u, 0x00000000u, 4}, {0x5ebf94u, 0x00000000u, 4},
        {0x5ebf98u, 0x00000000u, 4}, {0x5ebf9cu, 0x00000000u, 4}, {0x5ebfa0u, 0x00000000u, 4},
        {0x5ebfa4u, 0x00000001u, 4}, {0x5eb2b0u, 0x00000000u, 4}, {0x5eb2b4u, 0x00000000u, 4},
        {0x5eb2b8u, 0x00000000u, 4}, {0x5eb2bcu, 0x00000000u, 1}, {0x5eb2c0u, 0x00000000u, 1},
        {0x5eb36cu, 0x00000000u, 4},
        /* Qualified639a CollisionModel and actual two-entry Egg pointer table. */
        {0x5ecab4u, 0x000003f1u, 4}, {0x5ecab8u, 0x802ef908u, 4}, {0x5ecabcu, 0x3f800000u, 4},
        {0x5ecfdcu, 0x803431f0u, 4}, {0x5ecfe0u, 0x8018a6f0u, 4},
    };
    IsleGameResult result;
    size_t i, j;
    if (!s || !owner || !rom || bytes != 0x1000000u) return ISLE_GAME_INVALID_ARGUMENT;
    if (s->base.phase != ISLE_COURSE_ACTOR_UNPREPARED || s->base.course_id ||
        s->base.owner || s->base.nativeGame || s->base.nonwaiting_call_frame || s->services ||
        s->base.status != ISLE_GAME_OK) return ISLE_GAME_INVALID_STATE;
    for (i = 0; i < sizeof(fields) / sizeof(fields[0]); ++i) {
        uint32_t bits = 0;
        for (j = 0; j < fields[i].width; ++j) bits = (bits << 8) | rom[fields[i].rom + j];
        if (bits != fields[i].bits) return ISLE_GAME_INVALID_STATE;
    }
    *s = (IsleTunnelActorSource){0};
    s->D_802EF6AC_5EC77C = tunnel_prepare_i32(rom + 0x5ec77cu);
    s->D_802F0160_5ED230 = tunnel_prepare_i32(rom + 0x5ed230u);
    s->D_802F0164_5ED234 = tunnel_prepare_i32(rom + 0x5ed234u);
    s->magnemite_NumConnected = tunnel_prepare_i32(rom + 0x5eb370u);
    s->D_802EE10C_5EB1DC = (uint16_t)((uint16_t)rom[0x5eb1dcu] << 8 | rom[0x5eb1ddu]);
    s->D_802EEED0_5EBFA0 = tunnel_prepare_i32(rom + 0x5ebfa0u);
    s->D_802EEED4_5EBFA4 = tunnel_prepare_i32(rom + 0x5ebfa4u);
    s->D_802EF9F0_5ECAC0 = tunnel_prepare_i32(rom + 0x5ecac0u);
    for (i = 0; i < 3; ++i) s->D_802F0204_5ED2D4[i] = tunnel_prepare_f32(rom + 0x5ed2d4u + i * 4);
    s->D_802F0218_5ED2E8 = tunnel_prepare_i32(rom + 0x5ed2e8u);
    s->D_802F0220_5ED2F0 = tunnel_prepare_f32(rom + 0x5ed2f0u);
    s->D_802F0224_5ED2F4 = tunnel_prepare_f32(rom + 0x5ed2f4u);
    s->D_802EE1E0_5EB2B0 = tunnel_prepare_u32(rom + 0x5eb2b0u);
    s->D_802EE1E4_5EB2B4 = tunnel_prepare_u32(rom + 0x5eb2b4u);
    s->D_802EE1E8_5EB2B8 = tunnel_prepare_u32(rom + 0x5eb2b8u);
    s->D_802EE1EC_5EB2BC = rom[0x5eb2bcu];
    s->D_802EE1F0_5EB2C0 = rom[0x5eb2c0u];
    for (i = 0; i < 2; ++i) {
        uint16_t bits = (uint16_t)((uint16_t)rom[0x5ed258u + i * 2] << 8 | rom[0x5ed259u + i * 2]);
        memcpy(&s->D_802F0188_5ED258[i], &bits, sizeof(bits));
    }
    for (i = 0; i < 5; ++i)
        s->D_802EFF6C_5ED03C[i] = tunnel_prepare_i32(rom + 0x5ed03cu + i * 4);
    s->D_802EFF14_5ECFE4.data_identity = tunnel_prepare_u32(rom + 0x5ecfe4u);
    s->D_802EFF14_5ECFE4.temp_buffer_identity = tunnel_prepare_u32(rom + 0x5ecfe8u);
    s->D_802EFF14_5ECFE4.vertex_count = tunnel_prepare_i32(rom + 0x5ecfecu);
    s->D_802EFF14_5ECFE4.end = tunnel_prepare_i32(rom + 0x5ecff0u);
    s->D_802EFF14_5ECFE4.timer = tunnel_prepare_f32(rom + 0x5ecff4u);
    /* Source extern CollisionModel tuple, pkmn_1009/code.c:7, 12 bytes. */
    s->D_802EF9E4_5ECAB4.identity = UINT32_C(0x802ef9e4);
    s->D_802EF9E4_5ECAB4.actor_id = tunnel_prepare_i32(rom + 0x5ecab4u);
    s->D_802EF9E4_5ECAB4.tree_identity = tunnel_prepare_u32(rom + 0x5ecab8u);
    s->D_802EF9E4_5ECAB4.scale = tunnel_prepare_f32(rom + 0x5ecabcu);
    s->base.course_id = 1;
    s->base.owner = owner;
    /* Bind only after the actual Source zeroing and identity decode. */
    result = isle_egg_source_prepare(&s->D_802EFF14_5ECFE4,&s->base,s,owner,rom,bytes,
        s->egg_frame0_803431f0,336,egg_preparation);
    if (result != ISLE_GAME_OK) { s->base.status = result; return result; }
    s->base.phase = ISLE_COURSE_ACTOR_PREPARED;
    return ISLE_GAME_OK;
}
