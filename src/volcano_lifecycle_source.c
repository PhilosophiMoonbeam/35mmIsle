#include "volcano_lifecycle_source.h"
#include "engine_internal.h"
#include "player_source.h"
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>
typedef struct VolcanoLifecycleFrame { jmp_buf failure; void* previous; void* previous_player_frame; } VolcanoLifecycleFrame;
typedef struct VolcanoLifecycleInvocation { void(*function)(void*); void* argument; } VolcanoLifecycleInvocation;
static void lifecycle_player_call(IslePlayerSource* player,void* argument) {
    VolcanoLifecycleInvocation* call=argument; (void)player;
    call->function(call->argument);
}
static int services_complete(const IsleVolcanoLifecycleServices* p) { return p && p->block_next &&
        p->block_position &&
        p->displace_items &&
        p->world_add &&
        p->world_change &&
        p->world_remove &&
        p->height_map &&
        p->world_create &&
        p->sky_follow &&
        p->actor_initialize_range &&
        p->end_level &&
        p->detector_disable &&
        p->environment_cleanup &&
        p->bgm_stop &&
        p->sound_stop_all &&
        p->world_destroy &&
        p->player_stop &&
        p->camera_remove &&
        p->scene_end &&
        p->load_asset &&
        p->fx_bank &&
        p->fx_camera &&
        p->fx_allocate &&
        p->fx_dither &&
        p->fx_reference &&
        p->depth_allocate &&
        p->apply_screen &&
        p->fx_reset_bank &&
        p->engine_initialize &&
        p->add_object &&
        p->actors_initialize &&
        p->background_color &&
        p->cameras_create &&
        p->ui_initialize &&
        p->end_callback &&
        p->pause_callback &&
        p->environment_initialize &&
        p->detector_create &&
        p->detector_enable &&
        p->illegal_copy &&
        p->player_flag &&
        p->set_intervals &&
        p->near_clipping &&
        p->setup_scene &&
        p->photo_count &&
        p->notice &&
        p->input_pressed &&
        p->cutscene_prepare &&
        p->cutscene_model &&
        p->cutscene_run &&
        p->cutscene_stop &&
        p->intro_restore &&
        p->effect_create &&
        p->effect_attach &&
        p->camera_read &&
        p->camera_store &&
        p->delete_intro &&
        p->animation_callback &&
        p->scene_joined &&
        p->entry_joined &&
        p->create_process &&
        p->reference_assign &&
        p->sound_identity &&
        p->sound_stop &&
        p->sound_play_params &&
        p->main_camera &&
        p->camera_object &&
        p->camera_speed &&
        p->camera_animation &&
        p->camera_process &&
        p->player_animation &&
        p->player_model_init &&
        p->reset_camera; }
IsleGameResult isle_volcano_lifecycle_prepare(IsleVolcanoLifecycleSource* l,void* owner,IsleContinuationDomain* domain,IsleVolcanoActorSource* actors,const IsleVolcanoLifecycleServices* services,int32_t input_scene) {
    if(!l || !owner || !domain || !actors || !services_complete(services))return ISLE_GAME_INVALID_ARGUMENT;
    if(l->phase!=ISLE_VOLCANO_LIFECYCLE_UNPREPARED || l->owner || l->actors || l->domain || l->services || l->nonwaiting_call_frame || actors->base.phase!=ISLE_COURSE_ACTOR_PREPARED || actors->base.owner!=owner || actors->lifecycle)return ISLE_GAME_INVALID_STATE;
    if(isle_continuation_domain_check_access(domain)!=ISLE_CONT_OK)return ISLE_GAME_INVALID_STATE;
    l->owner=owner; l->domain=domain; l->actors=actors; l->services=services; l->status=ISLE_GAME_OK;
    l->screen=(IsleVolcanoScreen){0x803b5000u,0x803da800u,NULL,NULL,320,240,0xA99};
    l->scene=(IsleVolcanoScene){
        .heap_size=0,.contexts=2,.display_list_bytes={0x5000,0x1400,0x400,0},
        .graphics_heap_bytes=0xC800,.rdp_bytes=0x4000,.thread_stack_bytes=1024,.sprite_bytes=0x58,
        .heap_base=0x801a9900u,
        .update_callback=0x8000affcu,.draw_callback=0x8000ada0u,
        .pre_render_callback=0x800a1a50u,.input_callback=0x80004860u,
        .gtl_unknown00=0,.gtl_unknown14=1,.gtl_unknown30=2,
        .num_threads=0,.num_stacks=0,.unknown4c=0,.num_processes=0,.num_objects=0,
        .num_matrices=0,.unknown60=0,.num_animations=0,.num_materials=0,.num_models=0,
        .num_sprites=0,.num_cameras=0,.post_init=volcano_Init,.initialize=volcano_func_802D6780_727980};
    l->clock=0; l->scene_pending=0; l->scene_completed=0; l->scene_result=ISLE_GAME_OK;
    l->input_scene=input_scene; l->output_scene=0;
    actors->lifecycle=l; l->phase=ISLE_VOLCANO_LIFECYCLE_READY;
    return ISLE_GAME_OK;
}
void isle_volcano_lifecycle_checked(IsleVolcanoLifecycleSource* l,IsleGameResult r) {
    VolcanoLifecycleFrame* frame=l?(VolcanoLifecycleFrame*)l->nonwaiting_call_frame:NULL;
    IslePlayerSource* player=NULL;
    if(r==ISLE_GAME_OK)return;
    if(l) { if(l->status==ISLE_GAME_OK)l->status=r; r=l->status; }
    if(l && l->actors && l->actors->base.phase==ISLE_COURSE_ACTOR_ACTIVE && l->actors->base.nativeGame)
        player=l->actors->base.nativeGame->playerSource;
    if(player && player->nonwaiting_call_frame && (!frame || player->nonwaiting_call_frame!=frame->previous_player_frame))
        isle_player_source_checked(player,r);
    if(frame)longjmp(frame->failure,1);
    if(l && l->domain)isle_continuation_fail(l->domain,"Original Volcano lifecycle native leaf failed");
    /* Entry domain and actual OM process domain are distinct. The first fail
     * returns only when this is not its active C continuation. Live intro OM
     * failures then use their actual Game, never detached Player/Storage. */
    if(l && l->actors && l->actors->base.phase==ISLE_COURSE_ACTOR_ACTIVE)
        isle_native_execution_fault(l->actors->base.nativeGame,"Original Volcano lifecycle OM leaf failed");
    abort();
}
static IsleGameResult execution_access(IsleVolcanoLifecycleSource* l) {
    if(isle_continuation_domain_check_access(l->domain)==ISLE_CONT_OK)return ISLE_GAME_OK;
    if(l->actors->base.phase==ISLE_COURSE_ACTOR_ACTIVE && l->actors->base.nativeGame)
        return isle_actor_source_execution_status(l->actors->base.nativeGame);
    return ISLE_GAME_INVALID_STATE;
}
void isle_volcano_lifecycle_enter(IsleVolcanoLifecycleSource* l) {
    if(!l || !l->owner || !l->domain || !l->actors || !l->services || (l->phase!=ISLE_VOLCANO_LIFECYCLE_READY && l->phase!=ISLE_VOLCANO_LIFECYCLE_RUNNING))isle_volcano_lifecycle_checked(l,ISLE_GAME_INVALID_STATE);
    isle_volcano_lifecycle_checked(l,execution_access(l));
    isle_volcano_lifecycle_checked(l,l->status);
}
IsleVolcanoLifecycleSource* isle_volcano_lifecycle_current(IsleGameObject* receiver) {
    IsleVolcanoLifecycleSource* l=NULL;
    IsleGameResult r=isle_volcano_lifecycle_native_context(receiver,&l);
    if(r!=ISLE_GAME_OK || !l)isle_native_execution_fault(receiver?receiver->game:NULL,"Volcano lifecycle callback context failed");
    isle_volcano_lifecycle_enter(l); return l;
}
IsleGameResult isle_volcano_lifecycle_invoke(IsleVolcanoLifecycleSource* l,void(*fn)(void*),void* payload) {
    VolcanoLifecycleFrame frame;
    IsleGameResult access;
    IslePlayerSource* player=NULL;
    VolcanoLifecycleInvocation call={fn,payload};
    if(!l || !fn || !l->owner || !l->domain || !l->services)return ISLE_GAME_INVALID_ARGUMENT;
    if(l->status!=ISLE_GAME_OK)return l->status;
    if(l->phase!=ISLE_VOLCANO_LIFECYCLE_READY && l->phase!=ISLE_VOLCANO_LIFECYCLE_RUNNING)return ISLE_GAME_INVALID_STATE;
    access=execution_access(l);if(access!=ISLE_GAME_OK)return access;
    if(l->actors->base.phase==ISLE_COURSE_ACTOR_ACTIVE && l->actors->base.nativeGame) {
        access=isle_player_source_context(l->actors->base.nativeGame,&player);
        if(access!=ISLE_GAME_OK){if(l->status==ISLE_GAME_OK)l->status=access;return access;}
        if(!player){l->status=ISLE_GAME_INVALID_STATE;return l->status;}
    }
    frame.previous=l->nonwaiting_call_frame;
    frame.previous_player_frame=player?player->nonwaiting_call_frame:NULL;
    l->nonwaiting_call_frame=&frame;
    if(setjmp(frame.failure)==0) {
        if(player) {
            access=isle_player_source_call(player,lifecycle_player_call,&call);
            if(access!=ISLE_GAME_OK && l->status==ISLE_GAME_OK)l->status=access;
        } else fn(payload);
    }
    l->nonwaiting_call_frame=frame.previous;
    return l->status;
}
IsleVolcanoBlock* isle_volcano_lifecycle_block_next(IsleVolcanoLifecycleSource* s, IsleVolcanoBlock* block) {
    IsleVolcanoBlock* out={0};
    isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->block_next(s, block, &out);
    isle_volcano_lifecycle_checked(s,result);
    return out;
}

IsleGameVec3 isle_volcano_lifecycle_block_position(IsleVolcanoLifecycleSource* s, IsleVolcanoBlock* block) {
    IsleGameVec3 out={0};
    isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->block_position(s, block, &out);
    isle_volcano_lifecycle_checked(s,result);
    return out;
}

void isle_volcano_lifecycle_displace_items(IsleVolcanoLifecycleSource* s, float x, float y, float z) {
        isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->displace_items(s, x, y, z);
    isle_volcano_lifecycle_checked(s,result);
}

void isle_volcano_lifecycle_world_add(IsleVolcanoLifecycleSource* s, IsleVolcanoBlock* a, IsleVolcanoBlock* b, const IsleVolcanoDefinition* definitions, size_t count) {
        isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->world_add(s, a, b, definitions, count);
    isle_volcano_lifecycle_checked(s,result);
}

void isle_volcano_lifecycle_world_change(IsleVolcanoLifecycleSource* s, IsleVolcanoBlock* a, IsleVolcanoBlock* b, const IsleVolcanoDefinition* definitions, size_t count) {
        isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->world_change(s, a, b, definitions, count);
    isle_volcano_lifecycle_checked(s,result);
}

void isle_volcano_lifecycle_world_remove(IsleVolcanoLifecycleSource* s, IsleVolcanoBlock* a, const IsleVolcanoDefinition* definitions, size_t count) {
        isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->world_remove(s, a, definitions, count);
    isle_volcano_lifecycle_checked(s,result);
}

void isle_volcano_lifecycle_height_map(IsleVolcanoLifecycleSource* s, uint32_t identity) {
        isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->height_map(s, identity);
    isle_volcano_lifecycle_checked(s,result);
}

void isle_volcano_lifecycle_world_create(IsleVolcanoLifecycleSource* s, uint32_t identity, int32_t sky, int32_t first, int32_t last, int32_t link, int32_t draw, IsleVolcanoWorldAdd add, IsleVolcanoWorldRemove remove, IsleVolcanoWorldAdd change) {
        isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->world_create(s, identity, sky, first, last, link, draw, add, remove, change);
    isle_volcano_lifecycle_checked(s,result);
}

void isle_volcano_lifecycle_sky_follow(IsleVolcanoLifecycleSource* s) {
        isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->sky_follow(s);
    isle_volcano_lifecycle_checked(s,result);
}

void isle_volcano_lifecycle_actor_initialize_range(IsleVolcanoLifecycleSource* s, int32_t first, int32_t last, int32_t link, int32_t kind) {
        isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->actor_initialize_range(s, first, last, link, kind);
    isle_volcano_lifecycle_checked(s,result);
}

void isle_volcano_lifecycle_end_level(IsleVolcanoLifecycleSource* s, int32_t reason) {
        isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->end_level(s, reason);
    isle_volcano_lifecycle_checked(s,result);
}

void isle_volcano_lifecycle_detector_disable(IsleVolcanoLifecycleSource* s) {
        isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->detector_disable(s);
    isle_volcano_lifecycle_checked(s,result);
}

void isle_volcano_lifecycle_environment_cleanup(IsleVolcanoLifecycleSource* s) {
        isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->environment_cleanup(s);
    isle_volcano_lifecycle_checked(s,result);
}

void isle_volcano_lifecycle_bgm_stop(IsleVolcanoLifecycleSource* s) {
        isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->bgm_stop(s);
    isle_volcano_lifecycle_checked(s,result);
}

void isle_volcano_lifecycle_sound_stop_all(IsleVolcanoLifecycleSource* s) {
        isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->sound_stop_all(s);
    isle_volcano_lifecycle_checked(s,result);
}

void isle_volcano_lifecycle_world_destroy(IsleVolcanoLifecycleSource* s) {
        isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->world_destroy(s);
    isle_volcano_lifecycle_checked(s,result);
}

void isle_volcano_lifecycle_player_stop(IsleVolcanoLifecycleSource* s) {
        isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->player_stop(s);
    isle_volcano_lifecycle_checked(s,result);
}

void isle_volcano_lifecycle_camera_remove(IsleVolcanoLifecycleSource* s) {
        isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->camera_remove(s);
    isle_volcano_lifecycle_checked(s,result);
}

void isle_volcano_lifecycle_scene_end(IsleVolcanoLifecycleSource* s) {
        isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->scene_end(s);
    isle_volcano_lifecycle_checked(s,result);
}

const IsleVolcanoAsset* isle_volcano_lifecycle_load_asset(IsleVolcanoLifecycleSource* s, uint32_t start, uint32_t end) {
    const IsleVolcanoAsset* out={0};
    isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->load_asset(s, start, end, &out);
    isle_volcano_lifecycle_checked(s,result);
    return out;
}

void isle_volcano_lifecycle_fx_bank(IsleVolcanoLifecycleSource* s, int32_t bank, const IsleVolcanoAsset* script, const IsleVolcanoAsset* sprites) {
        isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->fx_bank(s, bank, script, sprites);
    isle_volcano_lifecycle_checked(s,result);
}

IsleGameObject* isle_volcano_lifecycle_fx_camera(IsleVolcanoLifecycleSource* s, int32_t link, int32_t count, IsleGameCamera* camera) {
    IsleGameObject* out={0};
    isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->fx_camera(s, link, count, camera, &out);
    isle_volcano_lifecycle_checked(s,result);
    return out;
}

IsleGameObject* isle_volcano_lifecycle_fx_allocate(IsleVolcanoLifecycleSource* s, int32_t count) {
    IsleGameObject* out={0};
    isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->fx_allocate(s, count, &out);
    isle_volcano_lifecycle_checked(s,result);
    return out;
}

void isle_volcano_lifecycle_fx_dither(IsleVolcanoLifecycleSource* s, IsleVolcanoColorDither color, IsleVolcanoAlphaDither alpha) {
        isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->fx_dither(s, color, alpha);
    isle_volcano_lifecycle_checked(s,result);
}

void isle_volcano_lifecycle_fx_reference(IsleVolcanoLifecycleSource* s, uint32_t identity, IsleGameObject* object) {
        isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->fx_reference(s, identity, object);
    isle_volcano_lifecycle_checked(s,result);
}

void* isle_volcano_lifecycle_depth_allocate(IsleVolcanoLifecycleSource* s, uint32_t bytes, uint32_t alignment) {
    void* out={0};
    isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->depth_allocate(s, bytes, alignment, &out);
    isle_volcano_lifecycle_checked(s,result);
    return out;
}

void isle_volcano_lifecycle_apply_screen(IsleVolcanoLifecycleSource* s, const IsleVolcanoScreen* screen) {
        isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->apply_screen(s, screen);
    isle_volcano_lifecycle_checked(s,result);
}

void isle_volcano_lifecycle_fx_reset_bank(IsleVolcanoLifecycleSource* s, size_t index) {
        isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->fx_reset_bank(s, index);
    isle_volcano_lifecycle_checked(s,result);
}

void isle_volcano_lifecycle_engine_initialize(IsleVolcanoLifecycleSource* s) {
        isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->engine_initialize(s);
    isle_volcano_lifecycle_checked(s,result);
}

IsleGameObject* isle_volcano_lifecycle_add_object(IsleVolcanoLifecycleSource* s, int32_t id, IsleGameSourceCallback callback, int32_t link, uint32_t priority) {
    IsleGameObject* out={0};
    isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->add_object(s, id, callback, link, priority, &out);
    isle_volcano_lifecycle_checked(s,result);
    return out;
}

void isle_volcano_lifecycle_actors_initialize(IsleVolcanoLifecycleSource* s) {
        isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->actors_initialize(s);
    isle_volcano_lifecycle_checked(s,result);
}

void isle_volcano_lifecycle_background_color(IsleVolcanoLifecycleSource* s, uint8_t* r, uint8_t* g, uint8_t* b) {
        isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->background_color(s, r, g, b);
    isle_volcano_lifecycle_checked(s,result);
}

void isle_volcano_lifecycle_cameras_create(IsleVolcanoLifecycleSource* s, uint32_t rgba) {
        isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->cameras_create(s, rgba);
    isle_volcano_lifecycle_checked(s,result);
}

void isle_volcano_lifecycle_ui_initialize(IsleVolcanoLifecycleSource* s, const IsleVolcanoUi* callbacks) {
        isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->ui_initialize(s, callbacks);
    isle_volcano_lifecycle_checked(s,result);
}

void isle_volcano_lifecycle_end_callback(IsleVolcanoLifecycleSource* s, IsleVolcanoEndCallback callback) {
        isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->end_callback(s, callback);
    isle_volcano_lifecycle_checked(s,result);
}

void isle_volcano_lifecycle_pause_callback(IsleVolcanoLifecycleSource* s, IsleVolcanoPauseCallback callback) {
        isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->pause_callback(s, callback);
    isle_volcano_lifecycle_checked(s,result);
}

void isle_volcano_lifecycle_environment_initialize(IsleVolcanoLifecycleSource* s, const IsleVolcanoEnvironment* sounds, size_t count) {
        isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->environment_initialize(s, sounds, count);
    isle_volcano_lifecycle_checked(s,result);
}

void isle_volcano_lifecycle_detector_create(IsleVolcanoLifecycleSource* s) {
        isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->detector_create(s);
    isle_volcano_lifecycle_checked(s,result);
}

void isle_volcano_lifecycle_detector_enable(IsleVolcanoLifecycleSource* s) {
        isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->detector_enable(s);
    isle_volcano_lifecycle_checked(s,result);
}

int32_t isle_volcano_lifecycle_illegal_copy(IsleVolcanoLifecycleSource* s) {
    int32_t out={0};
    isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->illegal_copy(s, &out);
    isle_volcano_lifecycle_checked(s,result);
    return out;
}

void isle_volcano_lifecycle_player_flag(IsleVolcanoLifecycleSource* s, int32_t flag, int32_t value) {
        isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->player_flag(s, flag, value);
    isle_volcano_lifecycle_checked(s,result);
}

void isle_volcano_lifecycle_set_intervals(IsleVolcanoLifecycleSource* s, int32_t update, int32_t draw) {
        isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->set_intervals(s, update, draw);
    isle_volcano_lifecycle_checked(s,result);
}

void isle_volcano_lifecycle_near_clipping(IsleVolcanoLifecycleSource* s, int32_t disabled) {
        isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->near_clipping(s, disabled);
    isle_volcano_lifecycle_checked(s,result);
}

static void setup_scene_intent(IsleVolcanoLifecycleSource* s, const IsleVolcanoScene* scene) {
        isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->setup_scene(s, scene);
    isle_volcano_lifecycle_checked(s,result);
}

int32_t isle_volcano_lifecycle_photo_count(IsleVolcanoLifecycleSource* s) {
    int32_t out={0};
    isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->photo_count(s, &out);
    isle_volcano_lifecycle_checked(s,result);
    return out;
}

void isle_volcano_lifecycle_notice(IsleVolcanoLifecycleSource* s, uint32_t mask) {
        isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->notice(s, mask);
    isle_volcano_lifecycle_checked(s,result);
}

uint32_t isle_volcano_lifecycle_input_pressed(IsleVolcanoLifecycleSource* s) {
    uint32_t out={0};
    isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->input_pressed(s, &out);
    isle_volcano_lifecycle_checked(s,result);
    return out;
}

void isle_volcano_lifecycle_cutscene_prepare(IsleVolcanoLifecycleSource* s) {
        isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->cutscene_prepare(s);
    isle_volcano_lifecycle_checked(s,result);
}

void isle_volcano_lifecycle_cutscene_model(IsleVolcanoLifecycleSource* s, IsleGameObject* object) {
        isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->cutscene_model(s, object);
    isle_volcano_lifecycle_checked(s,result);
}

void isle_volcano_lifecycle_cutscene_run(IsleVolcanoLifecycleSource* s) {
        isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->cutscene_run(s);
    isle_volcano_lifecycle_checked(s,result);
}

void isle_volcano_lifecycle_cutscene_stop(IsleVolcanoLifecycleSource* s) {
        isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->cutscene_stop(s);
    isle_volcano_lifecycle_checked(s,result);
}

void isle_volcano_lifecycle_intro_restore(IsleVolcanoLifecycleSource* s) {
        isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->intro_restore(s);
    isle_volcano_lifecycle_checked(s,result);
}

IsleVolcanoEffect* isle_volcano_lifecycle_effect_create(IsleVolcanoLifecycleSource* s, int32_t bank, int32_t id) {
    IsleVolcanoEffect* out={0};
    isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->effect_create(s, bank, id, &out);
    isle_volcano_lifecycle_checked(s,result);
    return out;
}

void isle_volcano_lifecycle_effect_attach(IsleVolcanoLifecycleSource* s, IsleVolcanoEffect* effect, const IslePlayerCartNode* model) {
        isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->effect_attach(s, effect, model);
    isle_volcano_lifecycle_checked(s,result);
}

float isle_volcano_lifecycle_camera_read(IsleVolcanoLifecycleSource* s, IsleGameCamera* camera, uint32_t component) {
    float out={0};
    isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->camera_read(s, camera, component, &out);
    isle_volcano_lifecycle_checked(s,result);
    return out;
}

void isle_volcano_lifecycle_camera_store(IsleVolcanoLifecycleSource* s, IsleGameCamera* camera, uint32_t component, float value) {
        isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->camera_store(s, camera, component, value);
    isle_volcano_lifecycle_checked(s,result);
}

void isle_volcano_lifecycle_delete_intro(IsleVolcanoLifecycleSource* s) {
        isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->delete_intro(s);
    isle_volcano_lifecycle_checked(s,result);
}

void isle_volcano_lifecycle_animation_callback(IsleVolcanoLifecycleSource* s, IsleGameObject* object, IsleVolcanoIntroAnimationCallback callback) {
        isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->animation_callback(s, object, callback);
    isle_volcano_lifecycle_checked(s,result);
}

void isle_volcano_lifecycle_scene_joined(IsleVolcanoLifecycleSource* s) {
        isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->scene_joined(s);
    isle_volcano_lifecycle_checked(s,result);
}

void isle_volcano_lifecycle_entry_joined(IsleVolcanoLifecycleSource* s) {
        isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->entry_joined(s);
    isle_volcano_lifecycle_checked(s,result);
}

IsleGameProcess* isle_volcano_lifecycle_create_process(IsleVolcanoLifecycleSource* s, IsleGameObject* object, IsleGameSourceCallback callback, int32_t kind, int32_t priority) {
    IsleGameProcess* out={0};
    isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->create_process(s, object, callback, kind, priority, &out);
    isle_volcano_lifecycle_checked(s,result);
    return out;
}

void isle_volcano_lifecycle_reference_assign(IsleVolcanoLifecycleSource* s, IsleGameObject** field, IsleGameObject* value) {
        isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->reference_assign(s, field, value);
    isle_volcano_lifecycle_checked(s,result);
}

int32_t isle_volcano_lifecycle_sound_identity(IsleVolcanoLifecycleSource* s, uint32_t handle) {
    int32_t out={0};
    isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->sound_identity(s, handle, &out);
    isle_volcano_lifecycle_checked(s,result);
    return out;
}

void isle_volcano_lifecycle_sound_stop(IsleVolcanoLifecycleSource* s, uint32_t handle) {
        isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->sound_stop(s, handle);
    isle_volcano_lifecycle_checked(s,result);
}

int32_t isle_volcano_lifecycle_sound_play_params(IsleVolcanoLifecycleSource* s, int32_t id, int32_t volume, int32_t pan, float pitch, int32_t priority) {
    int32_t out={0};
    isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->sound_play_params(s, id, volume, pan, pitch, priority, &out);
    isle_volcano_lifecycle_checked(s,result);
    return out;
}

IsleGameCamera* isle_volcano_lifecycle_main_camera(IsleVolcanoLifecycleSource* s) {
    IsleGameCamera* out={0};
    isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->main_camera(s, &out);
    isle_volcano_lifecycle_checked(s,result);
    return out;
}

IsleGameObject* isle_volcano_lifecycle_camera_object(IsleVolcanoLifecycleSource* s, IsleGameCamera* camera) {
    IsleGameObject* out={0};
    isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->camera_object(s, camera, &out);
    isle_volcano_lifecycle_checked(s,result);
    return out;
}

void isle_volcano_lifecycle_camera_speed(IsleVolcanoLifecycleSource* s, IsleGameCamera* camera, float speed) {
        isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->camera_speed(s, camera, speed);
    isle_volcano_lifecycle_checked(s,result);
}

void isle_volcano_lifecycle_camera_animation(IsleVolcanoLifecycleSource* s, IsleGameCamera* camera, uint32_t clip, float frame) {
        isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->camera_animation(s, camera, clip, frame);
    isle_volcano_lifecycle_checked(s,result);
}

IsleGameProcess* isle_volcano_lifecycle_camera_process(IsleVolcanoLifecycleSource* s, IsleGameObject* object, int32_t kind, int32_t priority) {
    IsleGameProcess* out={0};
    isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->camera_process(s, object, kind, priority, &out);
    isle_volcano_lifecycle_checked(s,result);
    return out;
}

void isle_volcano_lifecycle_player_animation(IsleVolcanoLifecycleSource* s, uint32_t model, uint32_t material, float frame, float speed) {
        isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->player_animation(s, model, material, frame, speed);
    isle_volcano_lifecycle_checked(s,result);
}

IsleGameObject* isle_volcano_lifecycle_player_model_init(IsleVolcanoLifecycleSource* s) {
    IsleGameObject* out={0};
    isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->player_model_init(s, &out);
    isle_volcano_lifecycle_checked(s,result);
    return out;
}

void isle_volcano_lifecycle_reset_camera(IsleVolcanoLifecycleSource* s) {
        isle_volcano_lifecycle_enter(s);
    IsleGameResult result=s->services->reset_camera(s);
    isle_volcano_lifecycle_checked(s,result);
}

void isle_volcano_lifecycle_setup_scene(IsleVolcanoLifecycleSource* l,const IsleVolcanoScene* scene) {
    if(l->phase!=ISLE_VOLCANO_LIFECYCLE_RUNNING || l->nonwaiting_call_frame || l->scene_pending || l->scene_completed || l->actors->base.phase!=ISLE_COURSE_ACTOR_PREPARED || l->actors->base.nativeGame || l->actors->services)isle_volcano_lifecycle_checked(l,ISLE_GAME_INVALID_STATE);
    l->scene_pending=1;
    setup_scene_intent(l,scene);
    while(l->scene_pending) {
        IsleContinuationResult r=isle_continuation_wait(l->domain,1);
        if(r!=ISLE_CONT_OK)isle_volcano_lifecycle_checked(l,ISLE_GAME_EXECUTION_FAILURE);
        ++l->clock;
    }
    isle_volcano_lifecycle_checked(l,l->status);
    isle_volcano_lifecycle_checked(l,l->scene_result);
    if(!l->scene_completed || l->actors->base.phase!=ISLE_COURSE_ACTOR_DETACHED)isle_volcano_lifecycle_checked(l,ISLE_GAME_INVALID_STATE);
}
IsleGameResult isle_volcano_lifecycle_complete_scene(IsleVolcanoLifecycleSource* l,IsleGameResult outcome) {
    IsleGameResult r;
    if(!l || !l->scene_pending || l->scene_completed || !l->services || l->actors->base.phase!=ISLE_COURSE_ACTOR_DETACHED)return ISLE_GAME_INVALID_STATE;
    r=l->services->scene_joined(l);
    if(r!=ISLE_GAME_OK){if(l->status==ISLE_GAME_OK)l->status=r;return r;}
    if(outcome==ISLE_GAME_OK && l->status!=ISLE_GAME_OK)outcome=l->status;
    if(outcome==ISLE_GAME_OK && l->actors->base.status!=ISLE_GAME_OK)outcome=l->actors->base.status;
    l->scene_result=outcome; l->scene_completed=1; l->scene_pending=0;
    if(outcome!=ISLE_GAME_OK && l->status==ISLE_GAME_OK)l->status=outcome;
    return ISLE_GAME_OK;
}
IsleGameResult isle_volcano_lifecycle_start(IsleVolcanoLifecycleSource* l,int32_t argument,int32_t* scene) {
    int32_t result;
    if(!l || !scene || l->phase!=ISLE_VOLCANO_LIFECYCLE_READY || l->nonwaiting_call_frame)return ISLE_GAME_INVALID_ARGUMENT;
    *scene=0;
    isle_volcano_lifecycle_enter(l);
    l->phase=ISLE_VOLCANO_LIFECYCLE_RUNNING;
    /* Actual independent C continuation entry: NO nonwaiting setjmp frame is
     * retained across omSetupScene's waits or borrowed by another OM domain. */
    result=volcano_Start(l,argument);
    isle_volcano_lifecycle_checked(l,l->status);
    *scene=result; l->phase=ISLE_VOLCANO_LIFECYCLE_COMPLETE;
    return ISLE_GAME_OK;
}
void isle_volcano_lifecycle_run(void* user) {
    IsleVolcanoLifecycleSource* l=user;
    IsleGameResult r;
    if(!l)abort();
    r=isle_volcano_lifecycle_start(l,l->input_scene,&l->output_scene);
    if(r!=ISLE_GAME_OK)isle_volcano_lifecycle_checked(l,r);
}
IsleGameResult isle_volcano_lifecycle_abandon(IsleVolcanoLifecycleSource* l,IsleGameResult outcome) {
    IsleGameResult r;
    if(!l || !l->owner || !l->domain || !l->services || outcome==ISLE_GAME_OK)return ISLE_GAME_INVALID_ARGUMENT;
    r=l->services->entry_joined(l);
    if(r!=ISLE_GAME_OK){if(l->status==ISLE_GAME_OK)l->status=r;return r;}
    /* Actual canceled/completed entry join is proven above. Its old C stack is
     * now inaccessible; clear its fence without ever jumping into retired C. */
    l->nonwaiting_call_frame=NULL;
    if(l->status==ISLE_GAME_OK)l->status=outcome;
    l->scene_result=l->status; l->scene_pending=0;
    l->phase=ISLE_VOLCANO_LIFECYCLE_COMPLETE;
    return l->status;
}

typedef struct VolcanoIntroCall { IsleVolcanoLifecycleSource* source; const IslePlayerCartNode* model; int32_t event; float value; } VolcanoIntroCall;
static void intro_animation_c(void* p) { VolcanoIntroCall* c=p; volcano_IntroAnimationCallback(c->source,c->model,c->event,c->value); }
IsleGameResult isle_volcano_lifecycle_invoke_intro_animation(IsleVolcanoLifecycleSource* l,const IslePlayerCartNode* model,int32_t event,float value) {
    VolcanoIntroCall call={l,model,event,value};
    if(!model)return ISLE_GAME_INVALID_ARGUMENT;
    return isle_volcano_lifecycle_invoke(l,intro_animation_c,&call);
}
static void init_original_c(void* p) { IsleVolcanoLifecycleSource* l=p; volcano_Init(l); }
IsleGameResult isle_volcano_lifecycle_invoke_init(IsleVolcanoLifecycleSource* l) { return isle_volcano_lifecycle_invoke(l,init_original_c,l); }
typedef struct VolcanoEndCall { IsleVolcanoLifecycleSource* source; int32_t reason; } VolcanoEndCall;
static void end_original_c(void* p) { VolcanoEndCall* c=p; volcano_EndLevel(c->source,c->reason); }
IsleGameResult isle_volcano_lifecycle_invoke_end(IsleVolcanoLifecycleSource* l,int32_t reason) { VolcanoEndCall call={l,reason};return isle_volcano_lifecycle_invoke(l,end_original_c,&call); }
typedef struct VolcanoExitCall { IsleVolcanoLifecycleSource* source; IsleVolcanoBlock* block; } VolcanoExitCall;
static void exit_original_c(void* p) { VolcanoExitCall* c=p; volcano_ExitBlock(c->source,c->block); }
IsleGameResult isle_volcano_lifecycle_invoke_exit_block(IsleVolcanoLifecycleSource* l,IsleVolcanoBlock* block) { VolcanoExitCall call={l,block};if(!block)return ISLE_GAME_INVALID_ARGUMENT;return isle_volcano_lifecycle_invoke(l,exit_original_c,&call); }
typedef struct VolcanoWorldCall { IsleVolcanoLifecycleSource* source; IsleVolcanoBlock* a; IsleVolcanoBlock* b; } VolcanoWorldCall;
static void add_original_c(void* p) { VolcanoWorldCall* c=p; volcano_PokemonAdd(c->source,c->a,c->b); }
static void change_original_c(void* p) { VolcanoWorldCall* c=p; volcano_PokemonChangeBlock(c->source,c->a,c->b); }
static void remove_original_c(void* p) { VolcanoWorldCall* c=p; volcano_PokemonRemove(c->source,c->a); }
IsleGameResult isle_volcano_lifecycle_invoke_world_add(IsleVolcanoLifecycleSource* l,IsleVolcanoBlock* a,IsleVolcanoBlock* b) { VolcanoWorldCall call={l,a,b};return isle_volcano_lifecycle_invoke(l,add_original_c,&call); }
IsleGameResult isle_volcano_lifecycle_invoke_world_change(IsleVolcanoLifecycleSource* l,IsleVolcanoBlock* a,IsleVolcanoBlock* b) { VolcanoWorldCall call={l,a,b};return isle_volcano_lifecycle_invoke(l,change_original_c,&call); }
IsleGameResult isle_volcano_lifecycle_invoke_world_remove(IsleVolcanoLifecycleSource* l,IsleVolcanoBlock* a) { VolcanoWorldCall call={l,a,NULL};return isle_volcano_lifecycle_invoke(l,remove_original_c,&call); }
static void pause_original_c(void* p) { VolcanoEndCall* c=p; volcano_Pause(c->source,c->reason); }
IsleGameResult isle_volcano_lifecycle_invoke_pause(IsleVolcanoLifecycleSource* l,int32_t reason) { VolcanoEndCall call={l,reason};return isle_volcano_lifecycle_invoke(l,pause_original_c,&call); }
