#include "player_root_source.h"
#include "player_photo_source.h"
#include "player_root_original.h"

IsleGameObject* isle_player_root_object(IslePlayerSource* s, uint32_t id,
    IsleGameSourceCallback update, uint8_t link, uint32_t priority) {
    IsleGameObject* out = NULL;
    isle_player_source_checked(s, isle_player_root_native_object(s,id,update,link,priority,&out));
    return out;
}
IsleGameSprite* isle_player_root_sprite(IslePlayerSource* s, IsleGameObject* object, uint32_t resource) {
    IsleGameSprite* out = NULL;
    isle_player_source_checked(s, isle_player_root_native_sprite(s,object,resource,&out));
    return out;
}
void isle_player_root_sprite_render(IslePlayerSource* s, IsleGameObject* object,
    uint8_t link, uint32_t priority, int32_t tag) {
    isle_player_source_checked(s, isle_player_root_native_sprite_render(s,object,link,priority,tag));
}
IsleGameObject* isle_player_root_reticle(IslePlayerSource* s,
    IsleGameSourceCallback update, IsleGameSourceCallback process) {
    IsleGameObject* object = isle_player_root_object(s,25,update,0,UINT32_C(0x80000000));
    /* ohCreateSprite returns NULL on actual object admission failure. Native
     * status is checked before this helper touches any attachment. */
    if (object == NULL) return NULL;
    isle_player_root_sprite_render(s,object,1,UINT32_C(0x80000000),-1);
    (void)isle_player_root_sprite(s,object,UINT32_C(0x80366DF0));
    if (process != NULL) isle_player_source_add_process(s,object,process,1,9);
    return object;
}
const IslePlayerIdleScript* isle_player_root_idle_script(IslePlayerSource* s) {
    const IslePlayerIdleScript* out = NULL;
    isle_player_source_checked(s,isle_player_root_native_idle_script(s,&out));
    return out;
}
const uint8_t* isle_player_root_species_name(IslePlayerSource* s, int32_t species) {
    const char* out = isle_player_photo_getPokemonName(s->photo,species);
    isle_player_source_checked(s,s->photo->status);
    return (const uint8_t*)out;
}
int32_t isle_player_root_check_flag(IslePlayerSource* s, int32_t flag) {
    int32_t out = isle_player_photo_checkPlayerFlag(s->photo,flag);
    isle_player_source_checked(s,s->photo->status);
    return out;
}
int32_t isle_player_root_species_registered(IslePlayerSource* s, int32_t species) {
    int32_t out = isle_player_photo_func_800BF3D4_5C274(s->photo,species);
    isle_player_source_checked(s,s->photo->status);
    return out;
}
#define ROOT_SCALAR(name) \
int32_t isle_player_root_##name(IslePlayerSource* s) { \
    int32_t out = 0; \
    isle_player_source_checked(s,isle_player_root_native_##name(s,&out)); \
    return out; \
}
ROOT_SCALAR(idle_interrupt_gate)
ROOT_SCALAR(registered_species_count)
ROOT_SCALAR(level_id)
ROOT_SCALAR(drawn_frame_counter)
ROOT_SCALAR(flute_state)
#undef ROOT_SCALAR
#define ROOT_SCALAR_ARG(name) \
int32_t isle_player_root_##name(IslePlayerSource* s, int32_t value) { \
    int32_t out = 0; \
    isle_player_source_checked(s,isle_player_root_native_##name(s,value,&out)); \
    return out; \
}
ROOT_SCALAR_ARG(play_sound)
#undef ROOT_SCALAR_ARG

static void init_call(IslePlayerSource* s, void* output) {
    *(IsleGameObject**)output = isle_player_initUI(s);
}
IsleGameResult isle_player_root_invoke_init(IslePlayerSource* s, IsleGameObject** output) {
    if (output == NULL) return ISLE_GAME_INVALID_ARGUMENT;
    *output = NULL;
    return isle_player_source_call(s,init_call,output);
}

typedef struct RootBlockDelta {
    float x, y, z;
} RootBlockDelta;
static void bind_camera_next_block_call(IslePlayerSource* s, void* user) {
    const RootBlockDelta* delta = user;
    isle_player_bindCameraNextBlock(s,delta->x,delta->y,delta->z);
}
IsleGameResult isle_player_root_invoke_bind_camera_next_block(IslePlayerSource* s,
    float dx, float dy, float dz) {
    RootBlockDelta delta = {dx,dy,dz};
    return isle_player_source_call(s,bind_camera_next_block_call,&delta);
}
static void set_background_music_call(IslePlayerSource* s, void* user) {
    isle_player_setBackgroundMusic(s,*(const int32_t*)user);
}
IsleGameResult isle_player_root_invoke_set_background_music(IslePlayerSource* s, int32_t song) {
    return isle_player_source_call(s,set_background_music_call,&song);
}

typedef struct RootCutsceneCall {
    IsleGameObject* pokemon;
    int32_t argument;
    const IslePlayerCameraAnimationClip* animation;
    float time;
} RootCutsceneCall;
static void start_stop_cutscene_call(IslePlayerSource* s, void* user) {
    const RootCutsceneCall* call = user;
    isle_player_Camera_StartStopCutscene(s,call->pokemon,call->argument,call->animation,call->time);
}
IsleGameResult isle_player_root_invoke_start_stop_cutscene(IslePlayerSource* s,
    IsleGameObject* pokemon, int32_t argument, const IslePlayerCameraAnimationClip* animation, float time) {
    RootCutsceneCall call = {pokemon,argument,animation,time};
    return isle_player_source_call(s,start_stop_cutscene_call,&call);
}
static void start_cutscene_call(IslePlayerSource* s, void* user) {
    const RootCutsceneCall* call = user;
    isle_player_Camera_StartCutScene(s,call->pokemon,call->animation,call->time);
}
IsleGameResult isle_player_root_invoke_start_cutscene(IslePlayerSource* s,
    IsleGameObject* pokemon, const IslePlayerCameraAnimationClip* animation, float time) {
    RootCutsceneCall call = {pokemon,1,animation,time};
    return isle_player_source_call(s,start_cutscene_call,&call);
}

static void hide_controls_call(IslePlayerSource* s, void* user) {
    (void)user;
    isle_player_func_803571C4_4F75D4(s);
}
IsleGameResult isle_player_root_invoke_hide_controls(IslePlayerSource* s) {
    return isle_player_source_call(s,hide_controls_call,NULL);
}
static void show_controls_call(IslePlayerSource* s, void* user) {
    (void)user;
    isle_player_func_803572B0_4F76C0(s);
}
IsleGameResult isle_player_root_invoke_show_controls(IslePlayerSource* s) {
    return isle_player_source_call(s,show_controls_call,NULL);
}
static void reset_main_camera_settings_call(IslePlayerSource* s, void* user) {
    (void)user;
    isle_player_resetMainCameraSettings(s);
}
IsleGameResult isle_player_root_invoke_reset_main_camera_settings(IslePlayerSource* s) {
    return isle_player_source_call(s,reset_main_camera_settings_call,NULL);
}

typedef struct RootPitchLimits {
    float minimum, maximum;
} RootPitchLimits;
static void set_pitch_limits_call(IslePlayerSource* s, void* user) {
    const RootPitchLimits* limits = user;
    isle_player_setPitchLimits(s,limits->minimum,limits->maximum);
}
IsleGameResult isle_player_root_invoke_set_pitch_limits(IslePlayerSource* s,
    float minimum, float maximum) {
    RootPitchLimits limits = {minimum,maximum};
    return isle_player_source_call(s,set_pitch_limits_call,&limits);
}
