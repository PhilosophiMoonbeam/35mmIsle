#ifndef ISLE_PLAYER_ROOT_SOURCE_H
#define ISLE_PLAYER_ROOT_SOURCE_H
#include "player_commands_source.h"
#include "player_camera_animation_clip.h"
#include "screen_transition_source.h"
#include "35mmisle/sprite.h"
#ifdef __cplusplus
extern "C" {
#endif

/* All native leaves return before checked C transfers failure. Source and its
 * native_root borrower survive every continuation, callback and resource lease.
 * initUI reads typed retained Root configuration at each original source point. */
IsleGameObject* isle_player_initUI(IslePlayerSource*);
void isle_player_func_80357428_4F7838(IslePlayerSource*, IsleGameObject*);
void isle_player_func_80356118_4F6528(IslePlayerSource*, IslePlayerCommandData);
IsleGameResult isle_player_root_invoke_init(IslePlayerSource*, IsleGameObject**);
/* Returning native-world/application entrypoints. Their packet thunks and every
 * frame between source_call and checked() are plain C. None enters a positive
 * wait directly; published stackful processes run later under genuine OM. */
IsleGameResult isle_player_root_invoke_bind_camera_next_block(IslePlayerSource*, float dx, float dy, float dz);
IsleGameResult isle_player_root_invoke_set_background_music(IslePlayerSource*, int32_t song);
IsleGameResult isle_player_root_invoke_start_stop_cutscene(IslePlayerSource*, IsleGameObject*,
    int32_t argument, const IslePlayerCameraAnimationClip*, float time);
IsleGameResult isle_player_root_invoke_start_cutscene(IslePlayerSource*, IsleGameObject*,
    const IslePlayerCameraAnimationClip*, float time);
/* Exact source571C4/572B0 hide/suspend and show/resume controls respectively;
 * these do not invent a camera-animation lifecycle or advance the two-tick wait. */
IsleGameResult isle_player_root_invoke_hide_controls(IslePlayerSource*);
IsleGameResult isle_player_root_invoke_show_controls(IslePlayerSource*);
IsleGameResult isle_player_root_invoke_reset_main_camera_settings(IslePlayerSource*);
IsleGameResult isle_player_root_invoke_set_pitch_limits(IslePlayerSource*, float minimum, float maximum);


/* Complete original Root control API. These C entrypoints require an original
 * C continuation or source_call fence; no owning C++ frame may enclose them. */
int32_t isle_player_func_80350200_4F0610(IslePlayerSource*, int32_t);
void isle_player_updateMovementState(IsleGameObject*);
void isle_player_bindCameraNextBlock(IslePlayerSource*, float, float, float);
void isle_player_stopLevelProcesses(IslePlayerSource*);
void isle_player_func_80350458_4F0868(IslePlayerSource*, int32_t, int32_t);
void isle_player_updateIdle(IsleGameObject*);
void isle_player_initIdle(IslePlayerSource*, IsleGameObject*);
void isle_player_setBackgroundMusic(IslePlayerSource*, int32_t);
void isle_player_updateMusic(IsleGameObject*);
void isle_player_shakeCamera(IsleGameObject*);
void isle_player_handleItemButtonsPress(IsleGameObject*);
void isle_player_handleAnalogStick(IsleGameObject*);
void isle_player_func_80351114_4F1524(IsleGameObject*);
void isle_player_playDashEngineSounds(IslePlayerSource*, int32_t);
void isle_player_updateDashEngine(IslePlayerSource*);
void isle_player_handleCButtons(IsleGameObject*);
void isle_player_processTurnToDirection(IsleGameObject*);
void isle_player_updateCameraZoomedOut(IsleGameObject*);
void isle_player_WorldToPlayerCoors(IslePlayerSource*, IsleGameVec3*);
void isle_player_PlayerToWorldCoors(IslePlayerSource*, IsleGameVec3*);
void isle_player_processZoomingIn(IsleGameObject*);
void isle_player_processZoomingOut(IsleGameObject*);
int32_t isle_player_strcmp2(const uint8_t*, const uint8_t*);
void isle_player_showPokemonLabel(IslePlayerSource*, int32_t, int32_t, int32_t*);
void isle_player_func_80352F20_4F3330(IsleGameObject*);
void isle_player_Camera_EndProcessByFunction(IslePlayerSource*, IsleGameObject*, IsleGameSourceCallback);
void isle_player_func_80353118_4F3528(IsleGameObject*);
void isle_player_updateCameraZoomedIn(IsleGameObject*);
void isle_player_vibrateCamera(IsleGameObject*);
void isle_player_setEndLevelCallback(IslePlayerSource*, IslePlayerEventCallback, void*);
void isle_player_setPauseCallback(IslePlayerSource*, IslePlayerEventCallback, void*);
int isle_player_func_80353D68_4F4178(IslePlayerSource*);
void isle_player_Pause_UpdateSelection(IsleGameObject*);
void isle_player_func_8035403C_4F444C(IsleGameObject*);
void isle_player_Pause_ShowUI(IsleGameObject*);
void isle_player_Pause_HideUI(IsleGameObject*);
void isle_player_func_8035453C_4F494C(IslePlayerSource*);
void isle_player_quitCourse(IsleGameObject*);
void isle_player_retryCourse(IsleGameObject*);
void isle_player_func_8035464C_4F4A5C(IsleGameObject*);
void isle_player_func_80354860_4F4C70(IsleGameObject*);
void isle_player_updatePauseMenu(IsleGameObject*);
void isle_player_func_80354D38_4F5148(IslePlayerSource*, int32_t*);
void isle_player_func_80354E04_4F5214(IslePlayerSource*, int32_t);
void isle_player_func_80354E7C_4F528C(IslePlayerSource*, int32_t*, IsleGameSourceCallback);
void isle_player_Tutorial_ShowMessage(IslePlayerSource*, int32_t);
int32_t isle_player_func_80354FB8_4F53C8(IslePlayerSource*, int32_t);
void isle_player_Tutorial_PauseAll(IslePlayerSource*, IsleGameObject*, int32_t*);
void isle_player_Tutorial_UnPauseAll(IslePlayerSource*, IsleGameObject*, int32_t*);
void isle_player_Tutorial_ShowMessageAndWait(IslePlayerSource*, int32_t);
void isle_player_func_80355228_4F5638(IsleGameObject*);
void isle_player_func_803552B0_4F56C0(IsleGameObject*);
void isle_player_updateTutorialMain(IsleGameObject*);
void isle_player_func_803555B0_4F59C0(IslePlayerSource*, uint8_t, uint8_t, uint8_t);
void isle_player_freezePokemons(IslePlayerSource*, IsleGameObject*);
void isle_player_Camera_StartStopCutscene(IslePlayerSource*, IsleGameObject*, int32_t, const IslePlayerCameraAnimationClip*, float);
void isle_player_processOutOfFilm(IsleGameObject*);
void isle_player_processBump(IsleGameObject*);
void isle_player_finishLevel(IsleGameObject*);
void isle_player_func_80355D54_4F6164(IsleGameObject*);
void isle_player_handlePlayerCmd(IslePlayerSource*, IslePlayerCommandData);
void isle_player_updatePlayer(IsleGameObject*);
void isle_player_updateReticleScreenPos(IsleGameObject*);
void isle_player_func_80356074_4F6484(IsleGameObject*);
void isle_player_updateReticleSpritesPos(IsleGameObject*);
void isle_player_func_80356FBC_4F73CC(IslePlayerSource*);
void isle_player_func_80356FE0_4F73F0(IslePlayerSource*);
void isle_player_getMovementTimes(IslePlayerSource*, float*, float*);
IsleGameObject* isle_player_getPlayerObject(IslePlayerSource*);
IsleGameModel* isle_player_getPlayerModel(IslePlayerSource*);
uint32_t isle_player_getProgressFlags(IslePlayerSource*);
void isle_player_setPitchLimits(IslePlayerSource*, float, float);
void isle_player_func_80357068_4F7478(IslePlayerSource*);
void isle_player_func_803570B0_4F74C0(IslePlayerSource*);
void isle_player_Camera_StartCutScene(IslePlayerSource*, IsleGameObject*, const IslePlayerCameraAnimationClip*, float);
void isle_player_func_80357120_4F7530(IslePlayerSource*, IsleGameObject*);
void isle_player_func_80357150_4F7560(IslePlayerSource*, IsleGameObject*);
void isle_player_func_80357170_4F7580(IslePlayerSource*);
void isle_player_func_803571C4_4F75D4(IslePlayerSource*);
void isle_player_func_80357278_4F7688(IsleGameObject*);
void isle_player_func_803572B0_4F76C0(IslePlayerSource*);
IsleWorldMovementState* isle_player_func_80357378_4F7788(IslePlayerSource*);
void isle_player_getLevelProgress(IslePlayerSource*, int32_t*, float*);
uint8_t isle_player_getIsPaused(IslePlayerSource*);
IsleGameModel* isle_player_func_803573B0_4F77C0(IslePlayerSource*);
void isle_player_resetMainCameraSettings(IslePlayerSource*);
float isle_player_isInScreenCenter(IslePlayerSource*, IsleGameCamera*, float, float, float);
float isle_player_matrixDeterminant(float[3][3]);
float isle_player_worldCoorsToScreen(IslePlayerSource*, float, float, float, float*, float*, float*);
void isle_player_screenCoorsToWorld(IslePlayerSource*, float*, float*, float*);
IsleGameResult isle_player_root_native_object(IslePlayerSource*, uint32_t id,
    IsleGameSourceCallback, uint8_t link, uint32_t priority, IsleGameObject**);
IsleGameResult isle_player_root_native_prepare_owners(IslePlayerSource*, IsleGameObject*, IsleGameModel*);
IsleGameResult isle_player_root_native_sprite_render(IslePlayerSource*, IsleGameObject*, uint8_t link, uint32_t priority, int32_t tag);
IsleGameResult isle_player_root_native_sprite(IslePlayerSource*, IsleGameObject*, uint32_t resource_identity, IsleGameSprite**);
IsleGameResult isle_player_root_native_movement_init(IslePlayerSource*, int32_t block);
IsleGameResult isle_player_root_native_movement_update(IslePlayerSource*);
IsleGameResult isle_player_root_native_world_block_index(IslePlayerSource*, const void* block, int32_t*);
IsleGameResult isle_player_root_native_idle_script(IslePlayerSource*, const IslePlayerIdleScript**);
IsleGameResult isle_player_root_native_idle_interrupt_gate(IslePlayerSource*, int32_t*);
IsleGameResult isle_player_root_native_set_flag(IslePlayerSource*, int32_t flag, int32_t value);
IsleGameResult isle_player_root_native_registered_species_count(IslePlayerSource*, int32_t*);
IsleGameResult isle_player_root_native_level_id(IslePlayerSource*, int32_t*);
IsleGameResult isle_player_root_native_postprocess(IslePlayerSource*, int enabled);
IsleGameResult isle_player_root_native_drawn_frame_counter(IslePlayerSource*, int32_t*);
IsleGameResult isle_player_root_native_detector_enabled(IslePlayerSource*, int enabled);
IsleGameResult isle_player_root_native_focus_assign(IslePlayerSource*, IsleGameObject*);
IsleGameResult isle_player_root_native_items_init(IslePlayerSource*);
IsleGameResult isle_player_root_native_items_custom_functions(IslePlayerSource*);
IsleGameResult isle_player_root_native_items_pause(IslePlayerSource*, int paused);
IsleGameResult isle_player_root_native_flute_state(IslePlayerSource*, int32_t*);
IsleGameResult isle_player_root_native_flute_play(IslePlayerSource*);
IsleGameResult isle_player_root_native_flute_stop(IslePlayerSource*);
IsleGameResult isle_player_root_native_spawn_item(IslePlayerSource*, int32_t item,
    const IsleGameVec3* origin, const IsleGameVec3* velocity);
IsleGameResult isle_player_root_native_skybox_animation_speed(IslePlayerSource*, float);
IsleGameResult isle_player_root_native_camera_animation_set(IslePlayerSource*, IsleGameCamera*, const IslePlayerCameraAnimationClip*, float);
IsleGameResult isle_player_root_native_camera_animation_update(IslePlayerSource*, IsleGameObject*);
IsleGameResult isle_player_root_native_play_sound(IslePlayerSource*, int32_t sound, int32_t* handle);
IsleGameResult isle_player_root_native_stop_sound(IslePlayerSource*, int32_t handle);
IsleGameResult isle_player_root_native_play_song(IslePlayerSource*, int32_t player, int32_t song);
IsleGameResult isle_player_root_native_stop_song(IslePlayerSource*, int32_t player);
IsleGameResult isle_player_root_native_stop_bgm(IslePlayerSource*);
IsleGameResult isle_player_root_native_stop_all_sounds(IslePlayerSource*);
IsleGameResult isle_player_root_native_bgm_volume(IslePlayerSource*, int32_t player, int32_t volume);
IsleGameResult isle_player_root_native_bgm_volume_smooth(IslePlayerSource*, int32_t player, int32_t volume, int32_t frames);
IsleGameResult isle_player_root_native_bgm_channel_volume(IslePlayerSource*, int32_t player, int32_t channel, int32_t volume);
IsleGameResult isle_player_root_native_sound_global_volume(IslePlayerSource*, int32_t volume);
IsleGameResult isle_player_root_native_current_sounds_global_volume(IslePlayerSource*, int32_t volume);

/* Checked C scalar/resource adapters; never call these from an owning C++
 * frame. Native callers use the returning leaves above or the C init fence. */
IsleGameObject* isle_player_root_object(IslePlayerSource*, uint32_t, IsleGameSourceCallback, uint8_t, uint32_t);
IsleGameSprite* isle_player_root_sprite(IslePlayerSource*, IsleGameObject*, uint32_t);
IsleGameObject* isle_player_root_reticle(IslePlayerSource*, IsleGameSourceCallback, IsleGameSourceCallback);
void isle_player_root_sprite_render(IslePlayerSource*, IsleGameObject*, uint8_t, uint32_t, int32_t);
const IslePlayerIdleScript* isle_player_root_idle_script(IslePlayerSource*);
int32_t isle_player_root_idle_interrupt_gate(IslePlayerSource*);
int32_t isle_player_root_check_flag(IslePlayerSource*, int32_t);
int32_t isle_player_root_registered_species_count(IslePlayerSource*);
int32_t isle_player_root_species_registered(IslePlayerSource*, int32_t);
const uint8_t* isle_player_root_species_name(IslePlayerSource*, int32_t);
int32_t isle_player_root_level_id(IslePlayerSource*);
int32_t isle_player_root_drawn_frame_counter(IslePlayerSource*);
int32_t isle_player_root_flute_state(IslePlayerSource*);
int32_t isle_player_root_play_sound(IslePlayerSource*, int32_t);

#ifdef __cplusplus
}
#endif
#endif
