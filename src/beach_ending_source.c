#include "beach_ending_source.h"
#include "engine_internal.h"

static void checked(IsleActorSource* actor, IsleGameResult result) {
    if (result != ISLE_GAME_OK)
        isle_native_execution_fault(actor->game, "Beach ending native leaf failed");
}
IsleBeachEndingSource* isle_beach_ending_source(IsleGameObject* object) {
    IsleActorSource* actor = isle_actor_source_require(object);
    IsleBeachEndingSource* source = NULL;
    checked(actor, isle_beach_ending_native_source(actor, &source));
    return source;
}
int32_t isle_beach_ending_play_sound(IsleGameObject* object, int32_t sound) {
    IsleActorSource* actor = isle_actor_source_require(object);
    int32_t handle = -1;
    checked(actor, isle_beach_ending_native_play_sound(actor, sound, &handle));
    return handle;
}
void isle_beach_ending_command(IsleGameObject* object, int32_t command) {
    IsleActorSource* actor = isle_actor_source_require(object);
    checked(actor, isle_beach_ending_native_command(actor, command));
}
void isle_beach_ending_bgm_volume(IsleGameObject* object, int32_t player, int32_t volume) {
    IsleActorSource* actor = isle_actor_source_require(object);
    checked(actor, isle_beach_ending_native_bgm_volume(actor, player, volume));
}
void isle_beach_ending_sound_volume(IsleGameObject* object, int32_t handle, uint32_t volume) {
    IsleActorSource* actor = isle_actor_source_require(object);
    checked(actor, isle_beach_ending_native_sound_volume(actor, handle, volume));
}
int32_t isle_beach_ending_playing_sound(IsleGameObject* object, int32_t handle) {
    IsleActorSource* actor = isle_actor_source_require(object);
    int32_t sound = -1;
    checked(actor, isle_beach_ending_native_playing_sound(actor, handle, &sound));
    return sound;
}
void isle_beach_ending_stop_sound(IsleGameObject* object, int32_t handle) {
    IsleActorSource* actor = isle_actor_source_require(object);
    checked(actor, isle_beach_ending_native_stop_sound(actor, handle));
}
void isle_beach_ending_stop_sounds(IsleGameObject* object) {
    IsleActorSource* actor = isle_actor_source_require(object);
    checked(actor, isle_beach_ending_native_stop_sounds(actor));
}
void isle_beach_ending_remove_items(IsleGameObject* object) {
    IsleActorSource* actor = isle_actor_source_require(object);
    checked(actor, isle_beach_ending_native_remove_items(actor));
}
void isle_beach_ending_reset_camera(IsleGameObject* object) {
    IsleActorSource* actor = isle_actor_source_require(object);
    checked(actor, isle_beach_ending_native_reset_camera(actor));
}
void isle_beach_ending_reset_message(IsleGameObject* object) {
    IsleActorSource* actor = isle_actor_source_require(object);
    checked(actor, isle_beach_ending_native_reset_message(actor));
}
void isle_beach_ending_start_camera(IsleGameObject* object) {
    IsleActorSource* actor = isle_actor_source_require(object);
    IslePlayerSource* player = isle_beach_ending_source(object)->player;
    IsleGameObject* camera = NULL;
    /* The late provider calls the existing nonwaiting C Root fence. */
    checked(actor, isle_beach_ending_native_start_cutscene(actor));
    checked(actor, isle_game_camera_object(actor->game, player->camera, &camera));
    isle_player_source_pause_object_processes(player, camera, 1);
    checked(actor, isle_beach_ending_native_camera(actor));
}
void isle_beach_ending_sky_speed(IsleGameObject* object, float speed) {
    IsleActorSource* actor = isle_actor_source_require(object);
    checked(actor, isle_beach_ending_native_sky_speed(actor, speed));
}
IsleGameObject* isle_beach_ending_cart_init(IsleGameObject* object) {
    IsleActorSource* actor = isle_actor_source_require(object);
    IsleGameObject* cart = NULL;
    checked(actor, isle_beach_ending_native_cart_init(actor, &cart));
    return cart;
}
void isle_beach_ending_cart_animation(IsleGameObject* object) {
    IsleActorSource* actor = isle_actor_source_require(object);
    checked(actor, isle_beach_ending_native_cart_animation(actor));
}
void isle_beach_ending_cart_publish(IsleGameObject* object, IsleGameObject* cart) {
    IsleActorSource* actor = isle_actor_source_require(object);
    checked(actor, isle_beach_ending_native_cart_publish(actor, cart));
}
void isle_beach_ending_cart_event(void* user, const IslePlayerCartNode* node, int32_t event, float argument) {
    IsleActorNodeRef reference = {node->binding, node->node};
    func_beach_802CA5A8((IsleBeachEndingSource*)user, &reference, event, argument);
}
void isle_beach_ending_cart_callback(IsleGameObject* object, IsleGameObject* cart) {
    IsleActorSource* actor = isle_actor_source_require(object);
    checked(actor, isle_beach_ending_native_cart_callback(actor, cart));
}
