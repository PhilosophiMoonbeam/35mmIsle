
#include "valley_lifecycle_source.h"
#include "engine_internal.h"
#include <stdlib.h>
#include <string.h>
typedef struct ValleyLifecycleFrame { jmp_buf failure; void* previous; } ValleyLifecycleFrame;
static uint32_t read32(const uint8_t* p) { return (uint32_t)p[0]<<24 | (uint32_t)p[1]<<16 | (uint32_t)p[2]<<8 | p[3]; }
static uint16_t read16(const uint8_t* p) { return (uint16_t)((uint16_t)p[0]<<8 | p[1]); }
static int services_complete(const IsleValleyLifecycleServices* p) { return p && p->intervals && p->near_clipping && p->photo_count && p->height_map && p->world_create && p->sky_follow && p->pokemon_registry && p->pokemon_add && p->pokemon_change && p->pokemon_remove && p->items_displace && p->transition_end && p->detector_disable && p->environment_cleanup && p->world_destroy && p->world_cleanup && p->camera_remove && p->scene_exit && p->depth_allocate && p->screen_apply && p->effect_banks_reset && p->game_state_reset && p->pokemons_init && p->background_color && p->cameras_create && p->ui_init && p->end_callback && p->pause_callback && p->environment_init && p->asset_load && p->effect_bank && p->effect_camera && p->effects_allocate && p->effects_dither && p->detector_create && p->detector_enable && p->intro_initialize && p->intro_object && p->intro_finish && p->intro_prepare && p->intro_stop && p->animation_callback && p->effect_camera_publish && p->effects_publish; }
IsleGameResult isle_valley_lifecycle_prepare(IsleValleyLifecycleSource* s, void* owner, IsleContinuationDomain* domain, IsleValleyActorSource* actors, const IsleValleyLifecycleServices* services, const uint8_t* rom, size_t bytes, int32_t scene) {
    if (!s || !owner || !domain || !actors || !services_complete(services) || !rom || bytes!=0x1000000u) return ISLE_GAME_INVALID_ARGUMENT;
    if (s->owner || s->domain || s->actors || s->services || s->running || s->completed || s->nonwaiting_call_frame || s->status!=ISLE_GAME_OK || actors->base.phase!=ISLE_COURSE_ACTOR_PREPARED || actors->base.owner!=owner || actors->base.nativeGame || actors->services || actors->base.status!=ISLE_GAME_OK || isle_continuation_domain_check_access(domain)!=ISLE_CONT_OK) return ISLE_GAME_INVALID_STATE;
    s->screen.framebuffer_identity[0]=read32(rom+0x7abf64);
    s->screen.framebuffer_identity[1]=read32(rom+0x7abf68);
    s->screen.framebuffer_identity[2]=read32(rom+0x7abf6c);
    s->screen.initial_depth_identity=read32(rom+0x7abf70);
    s->screen.width=read32(rom+0x7abf74);
    s->screen.height=read32(rom+0x7abf78);
    s->screen.flags=read32(rom+0x7abf7c);
    s->screen.depth_buffer=NULL;
    s->scene_setup.unknown_00=read16(rom+0x7abf80);
    s->scene_setup.update_identity=read32(rom+0x7abf84);
    s->scene_setup.draw_identity=read32(rom+0x7abf88);
    s->scene_setup.heap_base=read32(rom+0x7abf8c);
    s->scene_setup.heap_size=read32(rom+0x7abf90);
    s->scene_setup.unknown_14=read32(rom+0x7abf94);
    s->scene_setup.contexts=read32(rom+0x7abf98);
    s->scene_setup.graphics_heap_bytes=read32(rom+0x7abfac);
    s->scene_setup.unknown_30=read16(rom+0x7abfb0);
    s->scene_setup.rdp_output_bytes=read32(rom+0x7abfb4);
    s->scene_setup.prerender_identity=read32(rom+0x7abfb8);
    s->scene_setup.input_identity=read32(rom+0x7abfbc);
    s->scene_setup.threads=read32(rom+0x7abfc0);
    s->scene_setup.thread_stack_bytes=read32(rom+0x7abfc4);
    s->scene_setup.stacks=read32(rom+0x7abfc8);
    s->scene_setup.unknown_4c=read32(rom+0x7abfcc);
    s->scene_setup.processes=read32(rom+0x7abfd0);
    s->scene_setup.objects=read32(rom+0x7abfd4);
    s->scene_setup.object_bytes=read32(rom+0x7abfd8);
    s->scene_setup.matrices=read32(rom+0x7abfdc);
    s->scene_setup.matrix_callback_identity=read32(rom+0x7abfe0);
    s->scene_setup.dynamic_store_identity=read32(rom+0x7abfe4);
    s->scene_setup.animations=read32(rom+0x7abfe8);
    s->scene_setup.materials=read32(rom+0x7abfec);
    s->scene_setup.models=read32(rom+0x7abff0);
    s->scene_setup.model_bytes=read32(rom+0x7abff4);
    s->scene_setup.sprites=read32(rom+0x7abff8);
    s->scene_setup.sprite_bytes=read32(rom+0x7abffc);
    s->scene_setup.cameras=read32(rom+0x7ac000);
    s->scene_setup.camera_bytes=read32(rom+0x7ac004);
    s->scene_setup.initialize_identity=read32(rom+0x7ac008);
    s->scene_setup.display_list_bytes[0]=read32(rom+0x7abf9c);
    s->scene_setup.display_list_bytes[1]=read32(rom+0x7abfa0);
    s->scene_setup.display_list_bytes[2]=read32(rom+0x7abfa4);
    s->scene_setup.display_list_bytes[3]=read32(rom+0x7abfa8);
    s->environment[0]=(IsleValleyEnvironment){read16(rom+0x7abcd0),rom[0x7abcd2],rom[0x7abcd3]};
    s->environment[1]=(IsleValleyEnvironment){read16(rom+0x7abcd4),rom[0x7abcd6],rom[0x7abcd7]};
    s->environment[2]=(IsleValleyEnvironment){read16(rom+0x7abcd8),rom[0x7abcda],rom[0x7abcdb]};
    s->environment[3]=(IsleValleyEnvironment){read16(rom+0x7abcdc),rom[0x7abcde],rom[0x7abcdf]};
    s->environment[4]=(IsleValleyEnvironment){read16(rom+0x7abce0),rom[0x7abce2],rom[0x7abce3]};
    s->environment[5]=(IsleValleyEnvironment){read16(rom+0x7abce4),rom[0x7abce6],rom[0x7abce7]};
    s->environment[6]=(IsleValleyEnvironment){read16(rom+0x7abce8),rom[0x7abcea],rom[0x7abceb]};
    s->environment[7]=(IsleValleyEnvironment){read16(rom+0x7abcec),rom[0x7abcee],rom[0x7abcef]};
    s->environment[8]=(IsleValleyEnvironment){read16(rom+0x7abcf0),rom[0x7abcf2],rom[0x7abcf3]};
    s->environment[9]=(IsleValleyEnvironment){read16(rom+0x7abcf4),rom[0x7abcf6],rom[0x7abcf7]};
    s->environment[10]=(IsleValleyEnvironment){read16(rom+0x7abcf8),rom[0x7abcfa],rom[0x7abcfb]};
    s->environment[11]=(IsleValleyEnvironment){read16(rom+0x7abcfc),rom[0x7abcfe],rom[0x7abcff]};
    s->environment[12]=(IsleValleyEnvironment){read16(rom+0x7abd00),rom[0x7abd02],rom[0x7abd03]};
    s->environment[13]=(IsleValleyEnvironment){read16(rom+0x7abd04),rom[0x7abd06],rom[0x7abd07]};
    s->environment[14]=(IsleValleyEnvironment){read16(rom+0x7abd08),rom[0x7abd0a],rom[0x7abd0b]};
    s->environment[15]=(IsleValleyEnvironment){read16(rom+0x7abd0c),rom[0x7abd0e],rom[0x7abd0f]};
    s->environment[16]=(IsleValleyEnvironment){read16(rom+0x7abd10),rom[0x7abd12],rom[0x7abd13]};
    s->environment[17]=(IsleValleyEnvironment){read16(rom+0x7abd14),rom[0x7abd16],rom[0x7abd17]};
    s->environment[18]=(IsleValleyEnvironment){read16(rom+0x7abd18),rom[0x7abd1a],rom[0x7abd1b]};
    s->environment[19]=(IsleValleyEnvironment){read16(rom+0x7abd1c),rom[0x7abd1e],rom[0x7abd1f]};
    s->environment[20]=(IsleValleyEnvironment){read16(rom+0x7abd20),rom[0x7abd22],rom[0x7abd23]};
    s->environment[21]=(IsleValleyEnvironment){read16(rom+0x7abd24),rom[0x7abd26],rom[0x7abd27]};
    s->environment[22]=(IsleValleyEnvironment){read16(rom+0x7abd28),rom[0x7abd2a],rom[0x7abd2b]};
    s->environment[23]=(IsleValleyEnvironment){read16(rom+0x7abd2c),rom[0x7abd2e],rom[0x7abd2f]};
    s->environment[24]=(IsleValleyEnvironment){read16(rom+0x7abd30),rom[0x7abd32],rom[0x7abd33]};
    s->environment[25]=(IsleValleyEnvironment){read16(rom+0x7abd34),rom[0x7abd36],rom[0x7abd37]};
    s->environment[26]=(IsleValleyEnvironment){read16(rom+0x7abd38),rom[0x7abd3a],rom[0x7abd3b]};
    s->environment[27]=(IsleValleyEnvironment){read16(rom+0x7abd3c),rom[0x7abd3e],rom[0x7abd3f]};
    s->environment[28]=(IsleValleyEnvironment){read16(rom+0x7abd40),rom[0x7abd42],rom[0x7abd43]};
    s->environment[29]=(IsleValleyEnvironment){read16(rom+0x7abd44),rom[0x7abd46],rom[0x7abd47]};
    s->environment[30]=(IsleValleyEnvironment){read16(rom+0x7abd48),rom[0x7abd4a],rom[0x7abd4b]};
    s->environment[31]=(IsleValleyEnvironment){read16(rom+0x7abd4c),rom[0x7abd4e],rom[0x7abd4f]};
    s->environment[32]=(IsleValleyEnvironment){read16(rom+0x7abd50),rom[0x7abd52],rom[0x7abd53]};
    s->environment[33]=(IsleValleyEnvironment){read16(rom+0x7abd54),rom[0x7abd56],rom[0x7abd57]};
    s->environment[34]=(IsleValleyEnvironment){read16(rom+0x7abd58),rom[0x7abd5a],rom[0x7abd5b]};
    s->environment[35]=(IsleValleyEnvironment){read16(rom+0x7abd5c),rom[0x7abd5e],rom[0x7abd5f]};
    s->environment[36]=(IsleValleyEnvironment){read16(rom+0x7abd60),rom[0x7abd62],rom[0x7abd63]};
    s->environment[37]=(IsleValleyEnvironment){read16(rom+0x7abd64),rom[0x7abd66],rom[0x7abd67]};
    s->environment[38]=(IsleValleyEnvironment){read16(rom+0x7abd68),rom[0x7abd6a],rom[0x7abd6b]};
    s->environment[39]=(IsleValleyEnvironment){read16(rom+0x7abd6c),rom[0x7abd6e],rom[0x7abd6f]};
    s->environment[40]=(IsleValleyEnvironment){read16(rom+0x7abd70),rom[0x7abd72],rom[0x7abd73]};
    s->environment[41]=(IsleValleyEnvironment){read16(rom+0x7abd74),rom[0x7abd76],rom[0x7abd77]};
    s->environment[42]=(IsleValleyEnvironment){read16(rom+0x7abd78),rom[0x7abd7a],rom[0x7abd7b]};
    s->environment[43]=(IsleValleyEnvironment){read16(rom+0x7abd7c),rom[0x7abd7e],rom[0x7abd7f]};
    s->environment[44]=(IsleValleyEnvironment){read16(rom+0x7abd80),rom[0x7abd82],rom[0x7abd83]};
    s->environment[45]=(IsleValleyEnvironment){read16(rom+0x7abd84),rom[0x7abd86],rom[0x7abd87]};
    s->environment[46]=(IsleValleyEnvironment){read16(rom+0x7abd88),rom[0x7abd8a],rom[0x7abd8b]};
    s->environment[47]=(IsleValleyEnvironment){read16(rom+0x7abd8c),rom[0x7abd8e],rom[0x7abd8f]};
    s->environment[48]=(IsleValleyEnvironment){read16(rom+0x7abd90),rom[0x7abd92],rom[0x7abd93]};
    s->environment[49]=(IsleValleyEnvironment){read16(rom+0x7abd94),rom[0x7abd96],rom[0x7abd97]};
    s->environment[50]=(IsleValleyEnvironment){read16(rom+0x7abd98),rom[0x7abd9a],rom[0x7abd9b]};
    s->environment[51]=(IsleValleyEnvironment){read16(rom+0x7abd9c),rom[0x7abd9e],rom[0x7abd9f]};
    s->environment[52]=(IsleValleyEnvironment){read16(rom+0x7abda0),rom[0x7abda2],rom[0x7abda3]};
    s->environment[53]=(IsleValleyEnvironment){read16(rom+0x7abda4),rom[0x7abda6],rom[0x7abda7]};
    s->environment[54]=(IsleValleyEnvironment){read16(rom+0x7abda8),rom[0x7abdaa],rom[0x7abdab]};
    s->environment[55]=(IsleValleyEnvironment){read16(rom+0x7abdac),rom[0x7abdae],rom[0x7abdaf]};
    s->environment[56]=(IsleValleyEnvironment){read16(rom+0x7abdb0),rom[0x7abdb2],rom[0x7abdb3]};
    s->environment[57]=(IsleValleyEnvironment){read16(rom+0x7abdb4),rom[0x7abdb6],rom[0x7abdb7]};
    s->owner=owner; s->domain=domain; s->actors=actors; s->services=services; s->input_scene=scene;
    return ISLE_GAME_OK;
}
void isle_valley_lifecycle_checked(IsleValleyLifecycleSource* s, IsleGameResult result) {
    if (result==ISLE_GAME_OK) return;
    if (!s) abort();
    if (s->status==ISLE_GAME_OK) s->status=result;
    if (s->actors && s->actors->base.phase==ISLE_COURSE_ACTOR_ACTIVE && s->actors->base.nativeGame)
        isle_course_actor_checked(&s->actors->base,s->status);
    /* No detached Player/Game/Storage access. The independent entry domain and
     * owner outlive the scene and carry failure through the real end route. */
    if (s->nonwaiting_call_frame) {
        ValleyLifecycleFrame* frame=s->nonwaiting_call_frame;
        longjmp(frame->failure,1);
    }
    if (s->running) (void)isle_continuation_fail(s->domain,"Valley lifecycle native leaf failed");
    abort();
}
static IsleGameResult lifecycle_active(IsleValleyLifecycleSource* s) {
    if (!s || !s->owner || !s->domain || !s->actors || s->actors->base.owner!=s->owner || s->actors->base.phase!=ISLE_COURSE_ACTOR_ACTIVE || !s->actors->base.nativeGame || !services_complete(s->services)) return ISLE_GAME_INVALID_STATE;
    if (s->status!=ISLE_GAME_OK) return s->status;
    if (s->actors->base.status!=ISLE_GAME_OK) return s->actors->base.status;
    return isle_valley_lifecycle_native_admission(s);
}
IsleValleyLifecycleSource* isle_valley_lifecycle_current(IsleGameObject* object) {
    IsleValleyLifecycleSource* s=NULL;
    IsleGameResult result=isle_valley_lifecycle_native_context(object ? object->game : NULL,object,&s);
    if (result==ISLE_GAME_OK) result=lifecycle_active(s);
    if (result!=ISLE_GAME_OK) {
        if (s) isle_valley_lifecycle_checked(s,result);
        isle_native_execution_fault(object ? object->game : NULL,"Valley lifecycle receiver requires canonical native context");
    }
    return s;
}
void isle_valley_lifecycle_setup_scene(IsleValleyLifecycleSource* s, const IsleValleyLifecycleSceneSetup* setup) {
    if (!s || !s->running || s->scene_requested || s->nonwaiting_call_frame || !s->actors || s->actors->base.phase!=ISLE_COURSE_ACTOR_PREPARED || s->actors->base.owner!=s->owner || s->actors->base.nativeGame || s->actors->services) isle_valley_lifecycle_checked(s,ISLE_GAME_INVALID_STATE);
    isle_valley_lifecycle_checked(s,isle_valley_lifecycle_native_request_scene(s,setup));
    s->scene_requested=1;
    while (!s->scene_completion_ready) {
        if (isle_continuation_wait(s->domain,1)!=ISLE_CONT_OK) isle_valley_lifecycle_checked(s,ISLE_GAME_INVALID_STATE);
        ++s->clock;
    }
    isle_valley_lifecycle_checked(s,s->scene_completion_status);
    isle_valley_lifecycle_checked(s,s->actors->base.status);
}
void isle_valley_lifecycle_run(void* user) {
    IsleValleyLifecycleSource* s=user; int32_t result;
    if (!s || !s->owner || !s->domain || !s->actors || s->running || s->completed) return;
    s->running=1;
    if (s->actors->base.phase!=ISLE_COURSE_ACTOR_PREPARED || s->actors->base.owner!=s->owner || s->scene_requested || s->scene_completion_ready) isle_valley_lifecycle_checked(s,ISLE_GAME_INVALID_STATE);
    isle_valley_lifecycle_checked(s,s->status);
    isle_valley_lifecycle_checked(s,s->actors->base.status);
    result=func_802C6544_79FAD4(s,s->input_scene);
    if (!s->scene_completion_ready || s->actors->base.phase!=ISLE_COURSE_ACTOR_DETACHED) isle_valley_lifecycle_checked(s,ISLE_GAME_INVALID_STATE);
    isle_valley_lifecycle_checked(s,s->scene_completion_status);
    isle_valley_lifecycle_checked(s,s->actors->base.status);
    s->output_scene=result; s->running=0; s->completed=1;
}
IsleGameResult isle_valley_lifecycle_complete_scene(IsleValleyLifecycleSource* s, IsleGameResult result) {
    IsleGameResult joined;
    if (!s || !s->owner || !s->domain || !s->actors || !s->running || !s->scene_requested || s->scene_completion_ready || s->nonwaiting_call_frame || s->actors->base.phase!=ISLE_COURSE_ACTOR_DETACHED || s->actors->base.owner!=s->owner || s->actors->base.nativeGame || s->actors->services || s->actors->base.nonwaiting_call_frame || isle_continuation_domain_check_access(s->domain)!=ISLE_CONT_OK) return ISLE_GAME_INVALID_STATE;
    joined=isle_valley_lifecycle_native_scene_joined(s);
    if (joined!=ISLE_GAME_OK) return joined;
    s->screen.depth_buffer=NULL;
    if (s->status!=ISLE_GAME_OK) result=s->status;
    else if (s->actors->base.status!=ISLE_GAME_OK) result=s->actors->base.status;
    if (result!=ISLE_GAME_OK && s->status==ISLE_GAME_OK) s->status=result;
    s->scene_completion_status=result; s->scene_completion_ready=1;
    return result;
}
typedef struct ValleyLifecycleCall { IsleValleyLifecycleSource* source; int32_t integer; float value; const IsleActorNodeRef* node; IsleValleyWorldBlock* block; IsleValleyWorldBlock* reference; } ValleyLifecycleCall;
static void init_call(void* user) { ValleyLifecycleCall* p=user; func_802C63EC_79F97C(p->source); }
static void end_call(void* user) { ValleyLifecycleCall* p=user; func_802C5D18_79F2A8(p->source,p->integer); }
static void pause_call(void* user) { ValleyLifecycleCall* p=user; func_802C5DCC_79F35C(p->source,p->integer); }
static void movement_call(void* user) { ValleyLifecycleCall* p=user; func_802C6B30_7A00C0(p->source,p->integer,p->value); }
static void animation_call(void* user) { ValleyLifecycleCall* p=user; func_802C6630_79FBC0(p->source,p->node,p->integer,p->value); }
static void add_call(void* user) { ValleyLifecycleCall* p=user; func_802C5C20_79F1B0(p->source,p->block,p->reference); }
static void change_call(void* user) { ValleyLifecycleCall* p=user; func_802C5C44_79F1D4(p->source,p->block,p->reference); }
static void remove_call(void* user) { ValleyLifecycleCall* p=user; func_802C5C68_79F1F8(p->source,p->block); }
static void exit_call(void* user) { ValleyLifecycleCall* p=user; func_802C5DD4_79F364(p->source,p->block); }
static IsleGameResult lifecycle_call(IsleValleyLifecycleSource* s, IsleCourseActorCall callback, ValleyLifecycleCall* argument) {
    ValleyLifecycleFrame frame;
    IsleGameResult result=lifecycle_active(s);
    if (result!=ISLE_GAME_OK) {
        if(s && s->status==ISLE_GAME_OK) s->status=result;
        return result;
    }
    frame.previous=s->nonwaiting_call_frame;
    s->nonwaiting_call_frame=&frame;
    if(setjmp(frame.failure)==0) result=isle_valley_actor_invoke(s->actors,callback,argument);
    else result=s->status;
    s->nonwaiting_call_frame=frame.previous;
    if (result!=ISLE_GAME_OK && s->status==ISLE_GAME_OK) s->status=result;
    return result;
}
IsleGameResult isle_valley_lifecycle_invoke_init(IsleValleyLifecycleSource* s) { ValleyLifecycleCall p={0}; p.source=s; return lifecycle_call(s,init_call,&p); }
IsleGameResult isle_valley_lifecycle_invoke_end(IsleValleyLifecycleSource* s,int32_t reason) { ValleyLifecycleCall p={0}; p.source=s; p.integer=reason; return lifecycle_call(s,end_call,&p); }
IsleGameResult isle_valley_lifecycle_invoke_pause(IsleValleyLifecycleSource* s,int32_t reason) { ValleyLifecycleCall p={0}; p.source=s; p.integer=reason; return lifecycle_call(s,pause_call,&p); }
IsleGameResult isle_valley_lifecycle_invoke_movement(IsleValleyLifecycleSource* s,int32_t block,float part) { ValleyLifecycleCall p={0}; p.source=s; p.integer=block; p.value=part; return lifecycle_call(s,movement_call,&p); }
IsleGameResult isle_valley_lifecycle_invoke_animation(IsleValleyLifecycleSource* s,const IsleActorNodeRef* node,int32_t event,float value) {
    ValleyLifecycleCall p={0};
    IsleGameResult result=lifecycle_active(s);
    if(result==ISLE_GAME_OK) result=isle_valley_actor_validate_animation_node(s->actors,node);
    if(result!=ISLE_GAME_OK) {
        if(s && s->status==ISLE_GAME_OK) s->status=result;
        return result;
    }
    p.source=s; p.node=node; p.integer=event; p.value=value;
    return lifecycle_call(s,animation_call,&p);
}
IsleGameResult isle_valley_lifecycle_world_add(IsleValleyLifecycleSource* s,IsleValleyWorldBlock* a,IsleValleyWorldBlock* b) { ValleyLifecycleCall p={0}; p.source=s;p.block=a;p.reference=b;return lifecycle_call(s,add_call,&p); }
IsleGameResult isle_valley_lifecycle_world_change(IsleValleyLifecycleSource* s,IsleValleyWorldBlock* a,IsleValleyWorldBlock* b) { ValleyLifecycleCall p={0}; p.source=s;p.block=a;p.reference=b;return lifecycle_call(s,change_call,&p); }
IsleGameResult isle_valley_lifecycle_world_remove(IsleValleyLifecycleSource* s,IsleValleyWorldBlock* a) { ValleyLifecycleCall p={0};p.source=s;p.block=a;return lifecycle_call(s,remove_call,&p); }
IsleGameResult isle_valley_lifecycle_exit_block(IsleValleyLifecycleSource* s,IsleValleyWorldBlock* a) { ValleyLifecycleCall p={0};p.source=s;p.block=a;return lifecycle_call(s,exit_call,&p); }
void isle_valley_lifecycle_intervals(IsleValleyLifecycleSource* s, uint16_t update, uint16_t draw) {
    isle_valley_lifecycle_checked(s, s->services->intervals(s, update, draw));
}

void isle_valley_lifecycle_near_clipping(IsleValleyLifecycleSource* s, int32_t disabled) {
    isle_valley_lifecycle_checked(s, s->services->near_clipping(s, disabled));
}

int32_t isle_valley_lifecycle_photo_count(IsleValleyLifecycleSource* s) {
    int32_t value = 0;
    isle_valley_lifecycle_checked(s, s->services->photo_count(s, &value));
    return value;
}

int32_t isle_valley_lifecycle_height_map(IsleValleyLifecycleSource* s, uint32_t identity) {
    int32_t value = 0;
    isle_valley_lifecycle_checked(s, s->services->height_map(s, identity, &value));
    return value;
}

int32_t isle_valley_lifecycle_world_create(IsleValleyLifecycleSource* s, uint32_t identity, int32_t sky, int32_t minimum, int32_t maximum, int32_t link, int32_t draw_link, IsleValleyWorldPair add, IsleValleyWorldSingle remove, IsleValleyWorldPair change) {
    int32_t value = 0;
    isle_valley_lifecycle_checked(s, s->services->world_create(s, identity, sky, minimum, maximum, link, draw_link, add, remove, change, &value));
    return value;
}

void isle_valley_lifecycle_sky_follow(IsleValleyLifecycleSource* s) {
    isle_valley_lifecycle_checked(s, s->services->sky_follow(s));
}

void isle_valley_lifecycle_pokemon_registry(IsleValleyLifecycleSource* s, int32_t minimum, int32_t maximum, int32_t link, int32_t course) {
    isle_valley_lifecycle_checked(s, s->services->pokemon_registry(s, minimum, maximum, link, course));
}

void isle_valley_lifecycle_pokemon_add(IsleValleyLifecycleSource* s, IsleValleyWorldBlock* a, IsleValleyWorldBlock* b, uint32_t registry) {
    isle_valley_lifecycle_checked(s, s->services->pokemon_add(s, a, b, registry));
}

void isle_valley_lifecycle_pokemon_change(IsleValleyLifecycleSource* s, IsleValleyWorldBlock* a, IsleValleyWorldBlock* b, uint32_t registry) {
    isle_valley_lifecycle_checked(s, s->services->pokemon_change(s, a, b, registry));
}

void isle_valley_lifecycle_pokemon_remove(IsleValleyLifecycleSource* s, IsleValleyWorldBlock* block, uint32_t registry) {
    isle_valley_lifecycle_checked(s, s->services->pokemon_remove(s, block, registry));
}

void isle_valley_lifecycle_items_displace(IsleValleyLifecycleSource* s, float x, float y, float z) {
    isle_valley_lifecycle_checked(s, s->services->items_displace(s, x, y, z));
}

void isle_valley_lifecycle_transition_end(IsleValleyLifecycleSource* s, int32_t reason) {
    isle_valley_lifecycle_checked(s, s->services->transition_end(s, reason));
}

void isle_valley_lifecycle_detector_disable(IsleValleyLifecycleSource* s) {
    isle_valley_lifecycle_checked(s, s->services->detector_disable(s));
}

void isle_valley_lifecycle_environment_cleanup(IsleValleyLifecycleSource* s) {
    isle_valley_lifecycle_checked(s, s->services->environment_cleanup(s));
}

void isle_valley_lifecycle_world_destroy(IsleValleyLifecycleSource* s) {
    isle_valley_lifecycle_checked(s, s->services->world_destroy(s));
}

void isle_valley_lifecycle_world_cleanup(IsleValleyLifecycleSource* s) {
    isle_valley_lifecycle_checked(s, s->services->world_cleanup(s));
}

void isle_valley_lifecycle_camera_remove(IsleValleyLifecycleSource* s) {
    isle_valley_lifecycle_checked(s, s->services->camera_remove(s));
}

void isle_valley_lifecycle_scene_exit(IsleValleyLifecycleSource* s) {
    isle_valley_lifecycle_checked(s, s->services->scene_exit(s));
}

void* isle_valley_lifecycle_depth_allocate(IsleValleyLifecycleSource* s, size_t bytes, size_t alignment) {
    void* value = 0;
    isle_valley_lifecycle_checked(s, s->services->depth_allocate(s, bytes, alignment, &value));
    return value;
}

void isle_valley_lifecycle_screen_apply(IsleValleyLifecycleSource* s, const IsleValleyLifecycleScreen* screen) {
    isle_valley_lifecycle_checked(s, s->services->screen_apply(s, screen));
}

void isle_valley_lifecycle_effect_banks_reset(IsleValleyLifecycleSource* s) {
    isle_valley_lifecycle_checked(s, s->services->effect_banks_reset(s));
}

void isle_valley_lifecycle_game_state_reset(IsleValleyLifecycleSource* s) {
    isle_valley_lifecycle_checked(s, s->services->game_state_reset(s));
}

void isle_valley_lifecycle_pokemons_init(IsleValleyLifecycleSource* s) {
    isle_valley_lifecycle_checked(s, s->services->pokemons_init(s));
}

void isle_valley_lifecycle_background_color(IsleValleyLifecycleSource* s, uint8_t* r, uint8_t* g, uint8_t* b) {
    isle_valley_lifecycle_checked(s, s->services->background_color(s, r, g, b));
}

IsleGameObject* isle_valley_lifecycle_cameras_create(IsleValleyLifecycleSource* s, int32_t color) {
    IsleGameObject* value = 0;
    isle_valley_lifecycle_checked(s, s->services->cameras_create(s, color, &value));
    return value;
}

IsleGameObject* isle_valley_lifecycle_ui_init(IsleValleyLifecycleSource* s, IsleValleyWorldSingle exit, IsleValleyMovementCallback movement, IsleGameSourceCallback items, uint8_t kind, IsleValleyItemCollisionCallback collision) {
    IsleGameObject* value = 0;
    isle_valley_lifecycle_checked(s, s->services->ui_init(s, exit, movement, items, kind, collision, &value));
    return value;
}

void isle_valley_lifecycle_end_callback(IsleValleyLifecycleSource* s, IsleValleyEndCallback callback) {
    isle_valley_lifecycle_checked(s, s->services->end_callback(s, callback));
}

void isle_valley_lifecycle_pause_callback(IsleValleyLifecycleSource* s, IsleValleyEndCallback callback) {
    isle_valley_lifecycle_checked(s, s->services->pause_callback(s, callback));
}

void isle_valley_lifecycle_environment_init(IsleValleyLifecycleSource* s, const IsleValleyEnvironment* rows, size_t count) {
    isle_valley_lifecycle_checked(s, s->services->environment_init(s, rows, count));
}

void* isle_valley_lifecycle_asset_load(IsleValleyLifecycleSource* s, uint32_t begin, uint32_t end) {
    void* value = 0;
    isle_valley_lifecycle_checked(s, s->services->asset_load(s, begin, end, &value));
    return value;
}

void isle_valley_lifecycle_effect_bank(IsleValleyLifecycleSource* s, int32_t id, void* script, void* sprites) {
    isle_valley_lifecycle_checked(s, s->services->effect_bank(s, id, script, sprites));
}

IsleGameObject* isle_valley_lifecycle_effect_camera(IsleValleyLifecycleSource* s, int32_t id, int32_t count, IsleValleyCamera* camera) {
    IsleGameObject* value = 0;
    isle_valley_lifecycle_checked(s, s->services->effect_camera(s, id, count, camera, &value));
    return value;
}

void* isle_valley_lifecycle_effects_allocate(IsleValleyLifecycleSource* s, int32_t count) {
    void* value = 0;
    isle_valley_lifecycle_checked(s, s->services->effects_allocate(s, count, &value));
    return value;
}

void isle_valley_lifecycle_effects_dither(IsleValleyLifecycleSource* s, uint32_t color, uint32_t alpha) {
    isle_valley_lifecycle_checked(s, s->services->effects_dither(s, color, alpha));
}

void isle_valley_lifecycle_detector_create(IsleValleyLifecycleSource* s) {
    isle_valley_lifecycle_checked(s, s->services->detector_create(s));
}

void isle_valley_lifecycle_detector_enable(IsleValleyLifecycleSource* s) {
    isle_valley_lifecycle_checked(s, s->services->detector_enable(s));
}

void isle_valley_lifecycle_intro_initialize(IsleValleyLifecycleSource* s) {
    isle_valley_lifecycle_checked(s, s->services->intro_initialize(s));
}

void isle_valley_lifecycle_intro_object(IsleValleyLifecycleSource* s, IsleGameObject* object) {
    isle_valley_lifecycle_checked(s, s->services->intro_object(s, object));
}

void isle_valley_lifecycle_intro_finish(IsleValleyLifecycleSource* s) {
    isle_valley_lifecycle_checked(s, s->services->intro_finish(s));
}

void isle_valley_lifecycle_intro_prepare(IsleValleyLifecycleSource* s) {
    isle_valley_lifecycle_checked(s, s->services->intro_prepare(s));
}

void isle_valley_lifecycle_intro_stop(IsleValleyLifecycleSource* s) {
    isle_valley_lifecycle_checked(s, s->services->intro_stop(s));
}

void isle_valley_lifecycle_animation_callback(IsleValleyLifecycleSource* s, IsleGameObject* object, IsleValleyLifecycleAnimationCallback callback) {
    isle_valley_lifecycle_checked(s, s->services->animation_callback(s, object, callback));
}

void isle_valley_lifecycle_effect_camera_publish(IsleValleyLifecycleSource* s, IsleGameObject* object) {
    isle_valley_lifecycle_checked(s, s->services->effect_camera_publish(s, object));
}

void isle_valley_lifecycle_effects_publish(IsleValleyLifecycleSource* s, void* effects) {
    isle_valley_lifecycle_checked(s, s->services->effects_publish(s, effects));
}

