#include "river_lifecycle_source.h"
#include "engine_internal.h"
#include "player_source.h"
#include <setjmp.h>
#include <stdlib.h>
static uint16_t river_be16(const uint8_t* p) { return (uint16_t)((uint16_t)p[0] << 8 | p[1]); }
static uint32_t river_be32(const uint8_t* p) { return (uint32_t)p[0]<<24 | (uint32_t)p[1]<<16 | (uint32_t)p[2]<<8 | p[3]; }
static int32_t river_signed32(uint32_t value) { return value <= INT32_MAX ? (int32_t)value : -1-(int32_t)(UINT32_MAX-value); }
static const uint8_t* river_wire(const uint8_t* rom, size_t bytes, uint32_t guest, size_t count) {
    static const uint32_t mappings[][3] = {
        {UINT32_C(0x8012a0d0), UINT32_C(0x0029a1a0), UINT32_C(0x70df0)},
        {UINT32_C(0x8019aee0), UINT32_C(0x0030af90), UINT32_C(0x1bc80)},
        {UINT32_C(0x802e2620), UINT32_C(0x006ca400), UINT32_C(0x5cee0)},
    };
    size_t i;
    for (i=0;i<sizeof(mappings)/sizeof(mappings[0]);++i) {
        const uint32_t* map=mappings[i];
        if (guest >= map[0] && guest-map[0] <= map[2] && count <= map[2]-(guest-map[0]) &&
            map[1] <= bytes && guest-map[0] <= bytes-map[1] && count <= bytes-map[1]-(guest-map[0]))
            return rom+map[1]+(guest-map[0]);
    }
    return NULL;
}
typedef struct RiverLifecycleFrame { jmp_buf failure; void* previous; void* previous_player_frame; } RiverLifecycleFrame;
typedef struct RiverLifecycleInvocation { IsleRiverLifecycleSource* s; IsleRiverLifecycleCall callback; void* argument; } RiverLifecycleInvocation;
static int river_lifecycle_services_complete(const IsleRiverLifecycleServices* p) { return p && p->sound_params && p->sound_active && p->sound_stop && p->sound_stop_all && p->bgm_stop_all && p->bgm_play && p->bgm_stop && p->bgm_volume && p->bgm_channel_volume && p->camera && p->camera_read && p->camera_store && p->camera_speed && p->camera_animation && p->camera_process && p->player_model_init && p->player_animation && p->animation_callback && p->create_helper && p->delete_object && p->notice && p->effect_create && p->effect_attach && p->input_pressed && p->world_add && p->world_change && p->world_remove && p->height_map && p->create_world && p->sky_follow && p->pokemon_registry && p->transition_end && p->destroy_world && p->environment_cleanup && p->detector_disable && p->remove_camera && p->exit_scene && p->course_cleanup && p->flute_command && p->block_next && p->block_position && p->displace_items && p->load_asset && p->fx_bank && p->fx_camera && p->fx_allocate && p->fx_dither && p->depth_allocate && p->screen && p->fx_bank_count && p->fx_bank_reset && p->photo_reset && p->pokemons_init && p->background && p->create_cameras && p->ui_bind && p->end_callback && p->pause_callback && p->environment_init && p->detector_create && p->detector_enable && p->intervals && p->near_clipping && p->photo_count && p->request_scene && p->scene_joined && p->source_process && p->reference_assign; }
static IsleGameResult river_lifecycle_ready(const IsleRiverLifecycleSource* s) {
    if (!s || !s->owner || !s->domain || !s->actors || s->actors->base.owner != s->owner ||
        s->actors->base.course_id != 3 || !river_lifecycle_services_complete(s->services)) return ISLE_GAME_INVALID_STATE;
    if (s->status != ISLE_GAME_OK) return s->status;
    if (s->actors->base.status != ISLE_GAME_OK) return s->actors->base.status;
    return isle_river_lifecycle_native_admission(s,s->services);
}
void isle_river_lifecycle_checked(IsleRiverLifecycleSource* s, IsleGameResult result) {
    IslePlayerSource* player=NULL;
    RiverLifecycleFrame* frame=s ? s->nonwaiting_call_frame : NULL;
    if (result == ISLE_GAME_OK) return;
    if (!s) abort();
    if (s->status == ISLE_GAME_OK) s->status=result;
    result=s->status;
    /* Detached and pre-Game paths cannot inspect old native scene storage. */
    if (s->actors && s->actors->base.phase == ISLE_COURSE_ACTOR_ACTIVE && s->actors->base.nativeGame)
        player=s->actors->base.nativeGame->playerSource;
    if (player && player->nonwaiting_call_frame &&
        (!frame || player->nonwaiting_call_frame != frame->previous_player_frame))
        isle_player_source_checked(player,result);
    if (frame) longjmp(frame->failure,1);
    if (player) isle_player_source_checked(player,result);
    if (s->running) (void)isle_continuation_fail(s->domain,"Original River lifecycle leaf failed");
    abort();
}
void isle_river_lifecycle_enter(IsleRiverLifecycleSource* s) {
    isle_river_lifecycle_checked(s,river_lifecycle_ready(s));
}
IsleRiverLifecycleSource* isle_river_lifecycle_current(IsleGameObject* object) {
    IsleRiverLifecycleSource* s=NULL;
    IsleGameResult result=isle_river_lifecycle_native_context(object ? object->game : NULL,object,&s);
    if (result != ISLE_GAME_OK || !s || !object || !s->actors ||
        s->actors->base.phase != ISLE_COURSE_ACTOR_ACTIVE || s->actors->base.nativeGame != object->game)
        isle_native_execution_fault(object ? object->game : NULL,"River lifecycle callback requires its actual receiver context");
    isle_river_lifecycle_enter(s);
    return s;
}
static void river_lifecycle_player_call(IslePlayerSource* player, void* argument) {
    RiverLifecycleInvocation* call=argument;
    (void)player;
    call->callback(call->s,call->argument);
}
IsleGameResult isle_river_lifecycle_invoke(IsleRiverLifecycleSource* s, IsleRiverLifecycleCall callback, void* argument) {
    RiverLifecycleFrame frame;
    RiverLifecycleInvocation call={s,callback,argument};
    IslePlayerSource* player=NULL;
    IsleGameResult result=river_lifecycle_ready(s);
    if (!callback) return ISLE_GAME_INVALID_ARGUMENT;
    if (result != ISLE_GAME_OK) return result;
    if (s->actors->base.phase != ISLE_COURSE_ACTOR_ACTIVE || !s->actors->base.nativeGame)
        return ISLE_GAME_INVALID_STATE;
    result=isle_player_source_context(s->actors->base.nativeGame,&player);
    if (result != ISLE_GAME_OK || !player) return result == ISLE_GAME_OK ? ISLE_GAME_INVALID_STATE : result;
    frame.previous=s->nonwaiting_call_frame;
    frame.previous_player_frame=player->nonwaiting_call_frame;
    s->nonwaiting_call_frame=&frame;
    if (setjmp(frame.failure) == 0) {
        result=isle_player_source_call(player,river_lifecycle_player_call,&call);
        if (result != ISLE_GAME_OK && s->status == ISLE_GAME_OK) s->status=result;
    }
    s->nonwaiting_call_frame=frame.previous;
    return s->status;
}
static IsleGameResult river_entry_joined(IsleRiverLifecycleSource* s) {
    if (!s || !s->running || !s->scene_requested || !s->scene_completion_ready || !s->actors)
        return ISLE_GAME_INVALID_STATE;
    if (s->status != ISLE_GAME_OK) return s->status;
    if (s->actors->base.status != ISLE_GAME_OK) return s->actors->base.status;
    if (!s->scene_initialized || s->actors->base.phase != ISLE_COURSE_ACTOR_DETACHED ||
        s->actors->base.nativeGame || s->actors->services || s->actors->base.nonwaiting_call_frame)
        return ISLE_GAME_INVALID_STATE;
    return s->scene_completion_status;
}
void isle_river_lifecycle_setup_scene(IsleRiverLifecycleSource* s, const IsleRiverLifecycleSceneSetup* setup) {
    if (!s || !s->running || s->scene_requested || s->nonwaiting_call_frame || !s->actors ||
        s->actors->base.phase != ISLE_COURSE_ACTOR_PREPARED || s->actors->base.nativeGame || s->actors->services)
        isle_river_lifecycle_checked(s,ISLE_GAME_INVALID_STATE);
    isle_river_lifecycle_request_scene(s,setup);
    s->scene_requested=1;
    /* The independent entry domain survives scene cancellation and joins. */
    while (!s->scene_completion_ready) {
        IsleContinuationResult result=isle_continuation_wait(s->domain,1);
        if (result != ISLE_CONT_OK) isle_river_lifecycle_checked(s,ISLE_GAME_EXECUTION_FAILURE);
    }
    isle_river_lifecycle_checked(s,river_entry_joined(s));
}
IsleGameResult isle_river_lifecycle_complete_scene(IsleRiverLifecycleSource* s, IsleGameResult result) {
    IsleGameResult joined;
    if (!s || !s->owner || !s->domain || !s->actors || !s->running || !s->scene_requested ||
        s->scene_completion_ready || s->nonwaiting_call_frame ||
        s->actors->base.owner != s->owner ||
        s->actors->base.nativeGame || s->actors->services || s->actors->base.nonwaiting_call_frame ||
        !river_lifecycle_services_complete(s->services)) return ISLE_GAME_INVALID_STATE;
    if (s->status != ISLE_GAME_OK) result=s->status;
    else if (s->actors->base.status != ISLE_GAME_OK) result=s->actors->base.status;
    if (s->actors->base.phase != ISLE_COURSE_ACTOR_DETACHED &&
        !(s->actors->base.phase == ISLE_COURSE_ACTOR_PREPARED && result != ISLE_GAME_OK))
        return ISLE_GAME_INVALID_STATE;
    if (result == ISLE_GAME_OK && !s->scene_initialized) return ISLE_GAME_INVALID_STATE;
    if (isle_continuation_domain_check_access(s->domain) != ISLE_CONT_OK) return ISLE_GAME_INVALID_STATE;
    /* Returning, independent-owner-only real closure receipt; NEVER old Game. */
    joined=s->services->scene_joined(s);
    if (joined != ISLE_GAME_OK) return joined;
    s->screen.depth_buffer=NULL;
    if (s->status == ISLE_GAME_OK && result != ISLE_GAME_OK) s->status=result;
    s->scene_completion_status=result;
    s->scene_completion_ready=1;
    return result;
}
void isle_river_lifecycle_run(void* argument) {
    IsleRiverLifecycleSource* s=argument;
    int32_t output;
    if (!s) abort();
    if (s->running || s->completed || s->scene_requested || s->scene_completion_ready || s->scene_initialized || !s->actors ||
        s->actors->base.phase != ISLE_COURSE_ACTOR_PREPARED) isle_river_lifecycle_checked(s,ISLE_GAME_INVALID_STATE);
    s->running=1;
    isle_river_lifecycle_enter(s);
    output=func_802D9210_6C0FF0(s,s->input_scene);
    isle_river_lifecycle_checked(s,river_entry_joined(s));
    s->output_scene=output;
    s->running=0;
    s->completed=1;
}
static void river_init_call(IsleRiverLifecycleSource* s, void* ignored) { (void)ignored; func_802D90B8_6C0E98(s); }
IsleGameResult isle_river_lifecycle_invoke_init(IsleRiverLifecycleSource* s) {
    IsleGameResult result;
    if (!s || !s->running || !s->scene_requested || s->scene_completion_ready || s->scene_initialized ||
        !s->actors || s->actors->base.phase != ISLE_COURSE_ACTOR_ACTIVE) return ISLE_GAME_INVALID_STATE;
    result=isle_river_lifecycle_invoke(s,river_init_call,NULL);
    if (result == ISLE_GAME_OK) s->scene_initialized=1;
    return result;
}
static void river_end_call(IsleRiverLifecycleSource* s, void* argument) { func_802D88F8_6C06D8(s,*(int32_t*)argument); }
IsleGameResult isle_river_lifecycle_invoke_end(IsleRiverLifecycleSource* s, int32_t reason) { return isle_river_lifecycle_invoke(s,river_end_call,&reason); }
typedef struct RiverWorldPairCall { IsleRiverLifecycleWorldPair callback; IsleRiverBlock *a,*b; } RiverWorldPairCall;
static void river_world_pair_call(IsleRiverLifecycleSource* s, void* argument) { RiverWorldPairCall* call=argument; call->callback(s,call->a,call->b); }
IsleGameResult isle_river_lifecycle_invoke_world_pair(IsleRiverLifecycleSource* s, IsleRiverLifecycleWorldPair callback, IsleRiverBlock* a, IsleRiverBlock* b) {
    RiverWorldPairCall call={callback,a,b}; if (!callback) return ISLE_GAME_INVALID_ARGUMENT;
    return isle_river_lifecycle_invoke(s,river_world_pair_call,&call);
}
typedef struct RiverWorldSingleCall { IsleRiverLifecycleWorldSingle callback; IsleRiverBlock* block; } RiverWorldSingleCall;
static void river_world_single_call(IsleRiverLifecycleSource* s, void* argument) { RiverWorldSingleCall* call=argument; call->callback(s,call->block); }
IsleGameResult isle_river_lifecycle_invoke_world_single(IsleRiverLifecycleSource* s, IsleRiverLifecycleWorldSingle callback, IsleRiverBlock* block) {
    RiverWorldSingleCall call={callback,block}; if (!callback) return ISLE_GAME_INVALID_ARGUMENT;
    return isle_river_lifecycle_invoke(s,river_world_single_call,&call);
}
typedef struct RiverMovementCall { int32_t block; float fraction; } RiverMovementCall;
static void river_movement_call(IsleRiverLifecycleSource* s, void* argument) { RiverMovementCall* call=argument; func_802D92B0_6C1090(s,call->block,call->fraction); }
IsleGameResult isle_river_lifecycle_invoke_movement(IsleRiverLifecycleSource* s, int32_t block, float fraction) { RiverMovementCall call={block,fraction}; return isle_river_lifecycle_invoke(s,river_movement_call,&call); }
typedef struct RiverLifecycleAnimationCall { IsleRiverLifecycleAnimationCallback callback; const IsleActorNodeRef* receiver; int32_t event; float value; } RiverLifecycleAnimationCall;
static void river_lifecycle_animation_call(IsleRiverLifecycleSource* s, void* argument) { RiverLifecycleAnimationCall* call=argument; call->callback(s,call->receiver,call->event,call->value); }
IsleGameResult isle_river_lifecycle_invoke_animation(IsleRiverLifecycleSource* s, IsleRiverLifecycleAnimationCallback callback, const IsleActorNodeRef* receiver, int32_t event, float value) {
    RiverLifecycleAnimationCall call={callback,receiver,event,value}; if (!callback || !receiver) return ISLE_GAME_INVALID_ARGUMENT;
    return isle_river_lifecycle_invoke(s,river_lifecycle_animation_call,&call);
}
IsleGameResult isle_river_lifecycle_prepare(IsleRiverLifecycleSource* s, void* owner, IsleContinuationDomain* domain, IsleRiverActorSource* actors, const IsleRiverLifecycleServices* services, const uint8_t* rom, size_t bytes, int32_t scene) {
    const uint8_t *setup, *screen, *environment;
    size_t i;
    IsleGameResult validation;
    if (!s || !owner || !domain || !actors || !rom || bytes != UINT32_C(0x1000000) || !river_lifecycle_services_complete(services)) return ISLE_GAME_INVALID_ARGUMENT;
    if (s->owner || s->domain || s->actors || s->services || s->running || s->completed || s->scene_requested ||
        s->scene_completion_ready || s->scene_initialized || s->nonwaiting_call_frame || s->status != ISLE_GAME_OK ||
        s->scene_completion_status != ISLE_GAME_OK || s->screen.depth_buffer) return ISLE_GAME_INVALID_STATE;
    if (actors->base.phase != ISLE_COURSE_ACTOR_PREPARED || actors->base.owner != owner ||
        actors->base.course_id != 3 || actors->base.status != ISLE_GAME_OK || actors->base.nativeGame ||
        actors->services || actors->base.nonwaiting_call_frame) return ISLE_GAME_INVALID_STATE;
    if (isle_continuation_domain_check_access(domain) != ISLE_CONT_OK) return ISLE_GAME_INVALID_STATE;
    validation=isle_river_lifecycle_validate_initial_fields(rom,bytes);
    if (validation != ISLE_GAME_OK) return validation;
    setup=river_wire(rom,bytes,UINT32_C(0x802e293c),140);
    screen=river_wire(rom,bytes,UINT32_C(0x802e2920),28);
    environment=river_wire(rom,bytes,UINT32_C(0x802e2624),62*4);
    if (!setup || !screen || !environment) return ISLE_GAME_INVALID_STATE;
    s->scene_setup.unknown_00=river_be16(setup+0);
    s->scene_setup.update_identity=river_be32(setup+4);
    s->scene_setup.draw_identity=river_be32(setup+8);
    s->scene_setup.heap_base=river_be32(setup+12);
    s->scene_setup.heap_size=river_be32(setup+16);
    s->scene_setup.unknown_14=river_be32(setup+20);
    s->scene_setup.contexts=river_signed32(river_be32(setup+24));
    s->scene_setup.graphics_heap_bytes=river_be32(setup+44);
    s->scene_setup.unknown_30=river_be16(setup+48);
    s->scene_setup.rdp_output_bytes=river_signed32(river_be32(setup+52));
    s->scene_setup.prerender_identity=river_be32(setup+56);
    s->scene_setup.input_identity=river_be32(setup+60);
    s->scene_setup.threads=river_be32(setup+64);
    s->scene_setup.thread_stack_bytes=river_be32(setup+68);
    s->scene_setup.stacks=river_be32(setup+72);
    s->scene_setup.unknown_4c=river_signed32(river_be32(setup+76));
    s->scene_setup.processes=river_be32(setup+80);
    s->scene_setup.objects=river_be32(setup+84);
    s->scene_setup.object_bytes=river_be32(setup+88);
    s->scene_setup.matrices=river_be32(setup+92);
    s->scene_setup.matrix_callback_identity=river_be32(setup+96);
    s->scene_setup.dynamic_store_identity=river_be32(setup+100);
    s->scene_setup.animations=river_be32(setup+104);
    s->scene_setup.materials=river_be32(setup+108);
    s->scene_setup.models=river_be32(setup+112);
    s->scene_setup.model_bytes=river_be32(setup+116);
    s->scene_setup.sprites=river_be32(setup+120);
    s->scene_setup.sprite_bytes=river_be32(setup+124);
    s->scene_setup.cameras=river_be32(setup+128);
    s->scene_setup.camera_bytes=river_be32(setup+132);
    s->scene_setup.initialize_identity=river_be32(setup+136);
    for (i=0;i<4;++i) s->scene_setup.display_list_bytes[i]=river_be32(setup+28+i*4);
    for (i=0;i<3;++i) s->screen.framebuffer_identity[i]=river_be32(screen+i*4);
    s->screen.initial_depth_identity=river_be32(screen+12);
    s->screen.width=river_be32(screen+16); s->screen.height=river_be32(screen+20); s->screen.flags=river_be32(screen+24);
    for (i=0;i<62;++i) {
        s->environment[i].sound=river_be16(environment+i*4);
        s->environment[i].pitch_modifier=environment[i*4+2];
        s->environment[i].hearing_range=environment[i*4+3];
    }
    s->owner=owner; s->domain=domain; s->actors=actors; s->services=services;
    s->input_scene=scene; s->status=ISLE_GAME_OK;
    return ISLE_GAME_OK;
}
int32_t isle_river_lifecycle_sound_params(IsleRiverLifecycleSource* s, int32_t id, int32_t volume, int32_t pan, float pitch, int32_t priority) {
    int32_t value = {0};
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->sound_params(s, id, volume, pan, pitch, priority, &value));
    return value;
}

int32_t isle_river_lifecycle_sound_active(IsleRiverLifecycleSource* s, int32_t handle) {
    int32_t value = {0};
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->sound_active(s, handle, &value));
    return value;
}

void isle_river_lifecycle_sound_stop(IsleRiverLifecycleSource* s, int32_t handle) {
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->sound_stop(s, handle));
}

void isle_river_lifecycle_sound_stop_all(IsleRiverLifecycleSource* s) {
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->sound_stop_all(s));
}

void isle_river_lifecycle_bgm_stop_all(IsleRiverLifecycleSource* s) {
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->bgm_stop_all(s));
}

void isle_river_lifecycle_bgm_play(IsleRiverLifecycleSource* s, int32_t player, int32_t song) {
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->bgm_play(s, player, song));
}

void isle_river_lifecycle_bgm_stop(IsleRiverLifecycleSource* s, int32_t player) {
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->bgm_stop(s, player));
}

void isle_river_lifecycle_bgm_volume(IsleRiverLifecycleSource* s, int32_t player, uint32_t volume) {
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->bgm_volume(s, player, volume));
}

void isle_river_lifecycle_bgm_channel_volume(IsleRiverLifecycleSource* s, int32_t player, int32_t channel, int32_t volume) {
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->bgm_channel_volume(s, player, channel, volume));
}

IsleGameObject* isle_river_lifecycle_camera(IsleRiverLifecycleSource* s) {
    IsleGameObject* value = NULL;
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->camera(s, &value));
    return value;
}

const IsleRiverCameraView* isle_river_lifecycle_camera_read(IsleRiverLifecycleSource* s, IsleGameObject* camera) {
    const IsleRiverCameraView* value = NULL;
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->camera_read(s, camera, &value));
    return value;
}

void isle_river_lifecycle_camera_store(IsleRiverLifecycleSource* s, IsleGameObject* camera, uint32_t component, float value) {
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->camera_store(s, camera, component, value));
}

void isle_river_lifecycle_camera_speed(IsleRiverLifecycleSource* s, IsleGameObject* camera, float speed) {
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->camera_speed(s, camera, speed));
}

void isle_river_lifecycle_camera_animation(IsleRiverLifecycleSource* s, IsleGameObject* camera, uint32_t identity, float start) {
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->camera_animation(s, camera, identity, start));
}

IsleGameProcess* isle_river_lifecycle_camera_process(IsleRiverLifecycleSource* s, IsleGameObject* camera, int32_t kind, int32_t priority) {
    IsleGameProcess* value = NULL;
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->camera_process(s, camera, kind, priority, &value));
    return value;
}

IsleGameObject* isle_river_lifecycle_player_model_init(IsleRiverLifecycleSource* s) {
    IsleGameObject* value = NULL;
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->player_model_init(s, &value));
    return value;
}

void isle_river_lifecycle_player_animation(IsleRiverLifecycleSource* s, uint32_t model, uint32_t material, float start, float speed) {
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->player_animation(s, model, material, start, speed));
}

void isle_river_lifecycle_animation_callback(IsleRiverLifecycleSource* s, IsleGameObject* receiver, IsleRiverLifecycleAnimationCallback callback) {
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->animation_callback(s, receiver, callback));
}

IsleGameObject* isle_river_lifecycle_create_helper(IsleRiverLifecycleSource* s, int32_t id, IsleGameSourceCallback update, int32_t link, uint32_t priority) {
    IsleGameObject* value = NULL;
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->create_helper(s, id, update, link, priority, &value));
    return value;
}

void isle_river_lifecycle_delete_object(IsleRiverLifecycleSource* s, IsleGameObject* object) {
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->delete_object(s, object));
    if (!object) omEndProcess(NULL);
}


void isle_river_lifecycle_notice(IsleRiverLifecycleSource* s, uint32_t bits) {
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->notice(s, bits));
}

IsleRiverEffect* isle_river_lifecycle_effect_create(IsleRiverLifecycleSource* s, int32_t bank, int32_t effect) {
    IsleRiverEffect* value = NULL;
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->effect_create(s, bank, effect, &value));
    return value;
}

void isle_river_lifecycle_effect_attach(IsleRiverLifecycleSource* s, IsleRiverEffect* effect, const IsleActorNodeRef* receiver) {
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->effect_attach(s, effect, receiver));
}

uint16_t isle_river_lifecycle_input_pressed(IsleRiverLifecycleSource* s) {
    uint16_t value = {0};
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->input_pressed(s, &value));
    return value;
}





void isle_river_lifecycle_world_add(IsleRiverLifecycleSource* s, IsleRiverBlock* a, IsleRiverBlock* b, uint32_t registry) {
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->world_add(s, a, b, registry));
}

void isle_river_lifecycle_world_change(IsleRiverLifecycleSource* s, IsleRiverBlock* a, IsleRiverBlock* b, uint32_t registry) {
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->world_change(s, a, b, registry));
}

void isle_river_lifecycle_world_remove(IsleRiverLifecycleSource* s, IsleRiverBlock* block, uint32_t registry) {
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->world_remove(s, block, registry));
}

void isle_river_lifecycle_height_map(IsleRiverLifecycleSource* s, uint32_t identity) {
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->height_map(s, identity));
}

void isle_river_lifecycle_create_world(IsleRiverLifecycleSource* s, uint32_t setup, int32_t sky, int32_t minimum, int32_t maximum, int32_t link, int32_t draw_link, IsleRiverLifecycleWorldPair add, IsleRiverLifecycleWorldSingle remove, IsleRiverLifecycleWorldPair change) {
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->create_world(s, setup, sky, minimum, maximum, link, draw_link, add, remove, change));
}

void isle_river_lifecycle_sky_follow(IsleRiverLifecycleSource* s) {
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->sky_follow(s));
}

void isle_river_lifecycle_pokemon_registry(IsleRiverLifecycleSource* s, int32_t a, int32_t b, int32_t c, int32_t d) {
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->pokemon_registry(s, a, b, c, d));
}

void isle_river_lifecycle_transition_end(IsleRiverLifecycleSource* s, int32_t reason) {
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->transition_end(s, reason));
}

void isle_river_lifecycle_destroy_world(IsleRiverLifecycleSource* s) {
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->destroy_world(s));
}

void isle_river_lifecycle_environment_cleanup(IsleRiverLifecycleSource* s) {
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->environment_cleanup(s));
}

void isle_river_lifecycle_detector_disable(IsleRiverLifecycleSource* s) {
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->detector_disable(s));
}

void isle_river_lifecycle_remove_camera(IsleRiverLifecycleSource* s) {
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->remove_camera(s));
}

void isle_river_lifecycle_exit_scene(IsleRiverLifecycleSource* s) {
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->exit_scene(s));
}

void isle_river_lifecycle_course_cleanup(IsleRiverLifecycleSource* s) {
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->course_cleanup(s));
}

int32_t isle_river_lifecycle_flute_command(IsleRiverLifecycleSource* s) {
    int32_t value = {0};
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->flute_command(s, &value));
    return value;
}

IsleRiverBlock* isle_river_lifecycle_block_next(IsleRiverLifecycleSource* s, IsleRiverBlock* block) {
    IsleRiverBlock* value = NULL;
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->block_next(s, block, &value));
    return value;
}

IsleGameVec3 isle_river_lifecycle_block_position(IsleRiverLifecycleSource* s, IsleRiverBlock* block) {
    IsleGameVec3 value = {0};
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->block_position(s, block, &value));
    return value;
}

void isle_river_lifecycle_displace_items(IsleRiverLifecycleSource* s, float x, float y, float z) {
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->displace_items(s, x, y, z));
}

IsleRiverLifecycleAsset* isle_river_lifecycle_load_asset(IsleRiverLifecycleSource* s, IsleRiverLifecycleAssetId asset) {
    IsleRiverLifecycleAsset* value = NULL;
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->load_asset(s, asset, &value));
    return value;
}

void isle_river_lifecycle_fx_bank(IsleRiverLifecycleSource* s, int32_t bank, IsleRiverLifecycleAsset* script, IsleRiverLifecycleAsset* sprites) {
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->fx_bank(s, bank, script, sprites));
}

void isle_river_lifecycle_fx_camera(IsleRiverLifecycleSource* s, int32_t a, int32_t b, IsleGameObject* camera) {
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->fx_camera(s, a, b, camera));
}

void isle_river_lifecycle_fx_allocate(IsleRiverLifecycleSource* s, int32_t count) {
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->fx_allocate(s, count));
}

void isle_river_lifecycle_fx_dither(IsleRiverLifecycleSource* s, int32_t a, int32_t b) {
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->fx_dither(s, a, b));
}

void* isle_river_lifecycle_depth_allocate(IsleRiverLifecycleSource* s, uint32_t bytes, uint32_t alignment) {
    void* value = NULL;
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->depth_allocate(s, bytes, alignment, &value));
    return value;
}

void isle_river_lifecycle_screen(IsleRiverLifecycleSource* s, const IsleRiverLifecycleScreen* screen) {
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->screen(s, screen));
}

size_t isle_river_lifecycle_fx_bank_count(IsleRiverLifecycleSource* s) {
    size_t value = {0};
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->fx_bank_count(s, &value));
    return value;
}

void isle_river_lifecycle_fx_bank_reset(IsleRiverLifecycleSource* s, size_t bank) {
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->fx_bank_reset(s, bank));
}

void isle_river_lifecycle_photo_reset(IsleRiverLifecycleSource* s) {
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->photo_reset(s));
}

void isle_river_lifecycle_pokemons_init(IsleRiverLifecycleSource* s) {
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->pokemons_init(s));
}

void isle_river_lifecycle_background(IsleRiverLifecycleSource* s, uint8_t* r, uint8_t* g, uint8_t* b) {
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->background(s, r, g, b));
}

void isle_river_lifecycle_create_cameras(IsleRiverLifecycleSource* s, uint32_t background) {
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->create_cameras(s, background));
}

void isle_river_lifecycle_ui_bind(IsleRiverLifecycleSource* s, IsleRiverLifecycleWorldSingle exit, IsleRiverLifecycleMovement movement, IsleGameSourceCallback items, uint8_t kind, IsleRiverItemCollision collision) {
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->ui_bind(s, exit, movement, items, kind, collision));
}

void isle_river_lifecycle_end_callback(IsleRiverLifecycleSource* s, IsleRiverLifecycleEnd callback) {
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->end_callback(s, callback));
}

void isle_river_lifecycle_pause_callback(IsleRiverLifecycleSource* s, IsleRiverLifecycleEnd callback) {
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->pause_callback(s, callback));
}

void isle_river_lifecycle_environment_init(IsleRiverLifecycleSource* s, const IsleRiverLifecycleEnvironment* rows, size_t count) {
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->environment_init(s, rows, count));
}

void isle_river_lifecycle_detector_create(IsleRiverLifecycleSource* s) {
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->detector_create(s));
}

void isle_river_lifecycle_detector_enable(IsleRiverLifecycleSource* s) {
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->detector_enable(s));
}

void isle_river_lifecycle_intervals(IsleRiverLifecycleSource* s, uint16_t update, uint16_t draw) {
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->intervals(s, update, draw));
}

void isle_river_lifecycle_near_clipping(IsleRiverLifecycleSource* s, int32_t disabled) {
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->near_clipping(s, disabled));
}

int32_t isle_river_lifecycle_photo_count(IsleRiverLifecycleSource* s) {
    int32_t value = {0};
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->photo_count(s, &value));
    return value;
}


void isle_river_lifecycle_request_scene(IsleRiverLifecycleSource* s, const IsleRiverLifecycleSceneSetup* setup) {
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->request_scene(s, setup));
}

void isle_river_lifecycle_scene_joined(IsleRiverLifecycleSource* s) {
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->scene_joined(s));
}

IsleGameProcess* isle_river_lifecycle_source_process(IsleRiverLifecycleSource* s, IsleGameObject* object, IsleGameSourceCallback callback, int32_t kind, int32_t priority) {
    IsleGameProcess* value = NULL;
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->source_process(s, object, callback, kind, priority, &value));
    return value;
}

void isle_river_lifecycle_reference_assign(IsleRiverLifecycleSource* s, IsleGameObject** field, IsleGameObject* value) {
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s, s->services->reference_assign(s, field, value));
}


IslePlayerSource* isle_river_lifecycle_player(IsleRiverLifecycleSource* s) {
    IslePlayerSource* player=NULL;
    if (!s || !s->actors || s->actors->base.phase != ISLE_COURSE_ACTOR_ACTIVE || !s->actors->base.nativeGame)
        isle_river_lifecycle_checked(s,ISLE_GAME_INVALID_STATE);
    isle_river_lifecycle_enter(s);
    isle_river_lifecycle_checked(s,isle_player_source_context(s->actors->base.nativeGame,&player));
    if (!player) isle_river_lifecycle_checked(s,ISLE_GAME_INVALID_STATE);
    return player;
}
