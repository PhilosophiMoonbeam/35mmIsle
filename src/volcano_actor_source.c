#include "volcano_actor_source.h"
#include "engine_internal.h"
#include <string.h>
static int services_complete(const IsleVolcanoActorServices* p) {
    return p && p->misc_object_get &&
        p->misc_object_set &&
        p->reference_assign &&
        p->send_to_link &&
        p->send_addressed &&
        p->model &&
        p->aux_read &&
        p->root_read &&
        p->component_store &&
        p->item &&
        p->current_block &&
        p->block_position &&
        p->add_one &&
        p->add_geo &&
        p->random_range &&
        p->random_float &&
        p->ground &&
        p->player &&
        p->player_model &&
        p->sound_play &&
        p->sound_play_params &&
        p->sound_identity &&
        p->sound_stop &&
        p->sound_volume &&
        p->sound_stop_all &&
        p->bgm_volume &&
        p->bgm_stop &&
        p->bgm_play &&
        p->environment_sound &&
        p->remove_items &&
        p->reset_camera &&
        p->reset_messages &&
        p->cutscene_start &&
        p->main_camera &&
        p->camera_object &&
        p->camera_speed &&
        p->camera_animation &&
        p->camera_process &&
        p->pause_processes &&
        p->sky_speed &&
        p->player_model_init &&
        p->player_animation &&
        p->animation_callback &&
        p->animation_speed &&
        p->change_block_ground &&
        p->gb_pipe_sync &&
        p->gb_cycle &&
        p->gb_render_mode &&
        p->gb_texture_lut &&
        p->render_type_i &&
        p->render_type_d_fogged &&
        p->vector_distance;
}
IsleGameResult isle_volcano_actor_prepare(IsleVolcanoActorSource* s,void* owner) {
    if (!s || !owner) return ISLE_GAME_INVALID_ARGUMENT;
    if (s->base.phase!=ISLE_COURSE_ACTOR_UNPREPARED || s->base.owner || s->base.nativeGame || s->services || s->base.nonwaiting_call_frame) return ISLE_GAME_INVALID_STATE;
    s->base.owner=owner; s->base.course_id=2; s->base.status=ISLE_GAME_OK;
    s->volcano_gate_SoundHandle=-1;
    s->volcano_gate_IsPlayerNearGate=false;
    s->volcano_IntroEnded=false;
    s->volcano_intro_SoundHandle1=0;
    s->volcano_intro_SoundHandle2=0;
    s->volcano_intro_SoundHandle3=0;
    s->volcano_intro_SoundHandle4=0;
    s->volcano_MagikarpHasSpawned=false;
    s->volcano_EndLevelReason=0;
    s->volcano_GrowlitheAndArcanineCounter=4;
    s->D_802E0FA8_7321A8=(IsleGameVec3){ 0, 0, 0 };
    s->volcano_SoundState1=0;
    s->volcano_SoundState2=0;
    s->volcano_SoundState3=0;
    s->volcano_SoundHandle1=0;
    s->volcano_SoundHandle2=0;
    s->volcano_SoundHandle3=0;
    s->volcano_SoundHandle4=0;
    s->vulpix_ZeroVector=(IsleGameVec3){ 0, 0, 0 };
    s->charmander_Group1Pokemon=NULL;
    s->charmander_Group2Pokemon=NULL;
    s->charmander_Group3Pokemon=NULL;
    s->charmander_Group4Pokemon=NULL;
    s->charmander_Group5Pokemon=NULL;
    s->volcano_PlayerObj=NULL;
    s->magmar_loneMagmar=NULL;
    s->base.phase=ISLE_COURSE_ACTOR_PREPARED;
    return ISLE_GAME_OK;
}
IsleGameResult isle_volcano_actor_activate(IsleVolcanoActorSource* s,IsleGame* game,const IsleVolcanoActorServices* services) {
    IsleGameResult r;
    if (!s || !game || !services_complete(services)) return ISLE_GAME_INVALID_ARGUMENT;
    if (!s->base.owner || s->base.course_id!=2 || s->base.nonwaiting_call_frame) return ISLE_GAME_INVALID_STATE;
    if (s->base.status!=ISLE_GAME_OK) return s->base.status;
    if (s->base.phase==ISLE_COURSE_ACTOR_ACTIVE) {
        if (s->base.nativeGame!=game || s->services!=services) return ISLE_GAME_INVALID_STATE;
        r=isle_volcano_actor_native_admission(s,game,services);
        if(r!=ISLE_GAME_OK && s->base.status==ISLE_GAME_OK)s->base.status=r;
        return r;
    }
    if (s->base.phase!=ISLE_COURSE_ACTOR_PREPARED || s->base.nativeGame || s->services) return ISLE_GAME_INVALID_STATE;
    r=isle_volcano_actor_native_admission(s,game,services);
    if (r!=ISLE_GAME_OK) { if(s->base.status==ISLE_GAME_OK)s->base.status=r; return r; }
    s->base.nativeGame=game; s->services=services;
    s->base.phase=ISLE_COURSE_ACTOR_ACTIVE;
    return ISLE_GAME_OK;
}
IsleGameResult isle_volcano_actor_deactivate(IsleVolcanoActorSource* s) {
    IsleGameResult r;
    if (!s || s->base.phase!=ISLE_COURSE_ACTOR_ACTIVE || !s->base.nativeGame || s->base.nonwaiting_call_frame) return ISLE_GAME_INVALID_STATE;
    r=isle_actor_source_retirement_status(s->base.nativeGame);
    if(r!=ISLE_GAME_OK && s->base.status==ISLE_GAME_OK)s->base.status=r;
    r=isle_volcano_actor_native_retirement(s);
    if (r!=ISLE_GAME_OK) { if(s->base.status==ISLE_GAME_OK)s->base.status=r; return r; }
    s->base.nativeGame=NULL; s->services=NULL;
    s->base.phase=ISLE_COURSE_ACTOR_DETACHED;
    return ISLE_GAME_OK;
}
void isle_volcano_actor_checked(IsleVolcanoActorSource* s,IsleGameResult r) {
    isle_course_actor_checked(s?&s->base:NULL,r);
}
void isle_volcano_actor_enter(IsleVolcanoActorSource* s) {
    if (!s || s->base.phase!=ISLE_COURSE_ACTOR_ACTIVE || !s->base.nativeGame || !s->services)
        isle_volcano_actor_checked(s,ISLE_GAME_INVALID_STATE);
    isle_volcano_actor_checked(s,s->base.status);
    isle_volcano_actor_checked(s,isle_volcano_actor_native_admission(s,s->base.nativeGame,s->services));
}
IsleVolcanoActorSource* isle_volcano_actor_current(IsleGameObject* object) {
    IsleVolcanoActorSource* s=NULL;
    IsleGameResult r;
    if (!object) isle_native_execution_fault(NULL,"Volcano callback requires its real receiver");
    r=isle_volcano_actor_native_context(object->game,object,&s);
    if (r!=ISLE_GAME_OK || !s) isle_native_execution_fault(object->game,"Volcano canonical native context failed");
    isle_volcano_actor_enter(s);
    return s;
}
IsleGameResult isle_volcano_actor_invoke(IsleVolcanoActorSource* s,IsleCourseActorCall fn,void* payload) {
    IsleGameResult r;
    if(!s || !fn || s->base.phase!=ISLE_COURSE_ACTOR_ACTIVE || !s->services)return ISLE_GAME_INVALID_ARGUMENT;
    r=isle_volcano_actor_native_admission(s,s->base.nativeGame,s->services);
    if(r!=ISLE_GAME_OK){if(s->base.status==ISLE_GAME_OK)s->base.status=r;return r;}
    return isle_course_actor_invoke(&s->base,fn,payload);
}
float isle_volcano_actor_component(IsleVolcanoActorSource* s,const IsleGameModelRef* model,uint32_t index) {
    const IsleGameModelTransform* v=isle_volcano_actor_aux_read(s,model);
    switch(index) {
    case 0:return v->position.x;case 1:return v->position.y;case 2:return v->position.z;
    case 3:return v->rotation.x;case 4:return v->rotation.y;case 5:return v->rotation.z;
    case 6:return v->scale.x;case 7:return v->scale.y;case 8:return v->scale.z;
    default:isle_volcano_actor_checked(s,ISLE_GAME_INVALID_ARGUMENT); return 0;
    }
}
float isle_volcano_actor_position_component(IsleVolcanoActorSource* s,const IsleVolcanoPosition* pos,uint32_t component) {
    if (!pos || component>2 || pos->kind>3) isle_volcano_actor_checked(s,ISLE_GAME_INVALID_ARGUMENT);
    if(pos->kind==3) {
        const IsleGameVec3* v=isle_volcano_actor_root_read(s,&pos->model);
        switch(component){case 0:return v->x;case 1:return v->y;default:return v->z;}
    }
    return isle_volcano_actor_component(s,&pos->model,pos->kind*3+component);
}
void isle_volcano_actor_position_store(IsleVolcanoActorSource* s,const IsleVolcanoPosition* pos,uint32_t component,float value) {
    if(!pos || pos->kind>2 || component>2)isle_volcano_actor_checked(s,ISLE_GAME_INVALID_ARGUMENT);
    isle_volcano_actor_component_store(s,&pos->model,1u<<(pos->kind*3+component),value);
}
void isle_volcano_actor_visibility(IsleVolcanoActorSource* s,IsleActorSource* actor,IsleGameObject* object,int hidden) {
    actor->tangible=hidden?0:1;
    isle_actor_source_object_flags(object,hidden?3u:UINT32_MAX,hidden?3u:0u);
}
IsleGameObject* isle_volcano_actor_misc_object_get(IsleVolcanoActorSource* s, IsleGameObject* object, uint32_t index) {
    IsleGameObject* out={0};
    isle_volcano_actor_enter(s);
    IsleGameResult result=s->services->misc_object_get(s, object, index, &out);
    isle_volcano_actor_checked(s,result);
    return out;
}

void isle_volcano_actor_misc_object_set(IsleVolcanoActorSource* s, IsleGameObject* object, uint32_t index, IsleGameObject* target) {
        isle_volcano_actor_enter(s);
    IsleGameResult result=s->services->misc_object_set(s, object, index, target);
    isle_volcano_actor_checked(s,result);
}

void isle_volcano_actor_reference_assign(IsleVolcanoActorSource* s, IsleGameObject** field, IsleGameObject* value) {
        isle_volcano_actor_enter(s);
    IsleGameResult result=s->services->reference_assign(s, field, value);
    isle_volcano_actor_checked(s,result);
}

void isle_volcano_actor_send_to_link(IsleVolcanoActorSource* s, IsleGameObject* requester, uint8_t link, int32_t command, IsleGameObject* sender) {
        isle_volcano_actor_enter(s);
    IsleGameResult result=s->services->send_to_link(s, requester, link, command, sender);
    isle_volcano_actor_checked(s,result);
}

void isle_volcano_actor_send_addressed(IsleVolcanoActorSource* s, IsleGameObject* requester, IsleGameObject* destination, int32_t command, IsleGameObject* sender) {
        isle_volcano_actor_enter(s);
    IsleGameResult result=s->services->send_addressed(s, requester, destination, command, sender);
    isle_volcano_actor_checked(s,result);
}

IsleGameModelRef isle_volcano_actor_model(IsleVolcanoActorSource* s, IsleGameObject* object) {
    IsleGameModelRef out={0};
    isle_volcano_actor_enter(s);
    IsleGameResult result=s->services->model(s, object, &out);
    isle_volcano_actor_checked(s,result);
    return out;
}

const IsleGameModelTransform* isle_volcano_actor_aux_read(IsleVolcanoActorSource* s, const IsleGameModelRef* model) {
    const IsleGameModelTransform* out={0};
    isle_volcano_actor_enter(s);
    IsleGameResult result=s->services->aux_read(s, model, &out);
    isle_volcano_actor_checked(s,result);
    return out;
}

const IsleGameVec3* isle_volcano_actor_root_read(IsleVolcanoActorSource* s, const IsleGameModelRef* model) {
    const IsleGameVec3* out={0};
    isle_volcano_actor_enter(s);
    IsleGameResult result=s->services->root_read(s, model, &out);
    isle_volcano_actor_checked(s,result);
    return out;
}

void isle_volcano_actor_component_store(IsleVolcanoActorSource* s, const IsleGameModelRef* model, uint32_t mask, float value) {
        isle_volcano_actor_enter(s);
    IsleGameResult result=s->services->component_store(s, model, mask, value);
    isle_volcano_actor_checked(s,result);
}

const IsleItemSource* isle_volcano_actor_item(IsleVolcanoActorSource* s, IsleGameObject* object) {
    const IsleItemSource* out={0};
    isle_volcano_actor_enter(s);
    IsleGameResult result=s->services->item(s, object, &out);
    isle_volcano_actor_checked(s,result);
    return out;
}

IsleVolcanoBlock* isle_volcano_actor_current_block(IsleVolcanoActorSource* s) {
    IsleVolcanoBlock* out={0};
    isle_volcano_actor_enter(s);
    IsleGameResult result=s->services->current_block(s, &out);
    isle_volcano_actor_checked(s,result);
    return out;
}

IsleGameVec3 isle_volcano_actor_block_position(IsleVolcanoActorSource* s, IsleVolcanoBlock* block) {
    IsleGameVec3 out={0};
    isle_volcano_actor_enter(s);
    IsleGameResult result=s->services->block_position(s, block, &out);
    isle_volcano_actor_checked(s,result);
    return out;
}

IsleGameObject* isle_volcano_actor_add_one(IsleVolcanoActorSource* s, IsleVolcanoBlock* a, IsleVolcanoBlock* b, const IsleVolcanoAutomaticSpawn* spawn, const IsleVolcanoDefinition* definition) {
    IsleGameObject* out={0};
    isle_volcano_actor_enter(s);
    IsleGameResult result=s->services->add_one(s, a, b, spawn, definition, &out);
    isle_volcano_actor_checked(s,result);
    return out;
}

IsleGameObject* isle_volcano_actor_add_geo(IsleVolcanoActorSource* s, IsleGameObject* object, uint16_t requested_id, const IsleVolcanoDefinition* definition) {
    IsleGameObject* out={0};
    isle_volcano_actor_enter(s);
    IsleGameResult result=s->services->add_geo(s, object, requested_id, definition, &out);
    isle_volcano_actor_checked(s,result);
    return out;
}

int32_t isle_volcano_actor_random_range(IsleVolcanoActorSource* s, int32_t range) {
    int32_t out={0};
    isle_volcano_actor_enter(s);
    IsleGameResult result=s->services->random_range(s, range, &out);
    isle_volcano_actor_checked(s,result);
    return out;
}

float isle_volcano_actor_random_float(IsleVolcanoActorSource* s) {
    float out={0};
    isle_volcano_actor_enter(s);
    IsleGameResult result=s->services->random_float(s, &out);
    isle_volcano_actor_checked(s,result);
    return out;
}

int32_t isle_volcano_actor_ground(IsleVolcanoActorSource* s, float x, float z, IsleActorGround* ground) {
    int32_t out={0};
    isle_volcano_actor_enter(s);
    IsleGameResult result=s->services->ground(s, x, z, ground, &out);
    isle_volcano_actor_checked(s,result);
    return out;
}

IsleGameObject* isle_volcano_actor_player(IsleVolcanoActorSource* s) {
    IsleGameObject* out={0};
    isle_volcano_actor_enter(s);
    IsleGameResult result=s->services->player(s, &out);
    isle_volcano_actor_checked(s,result);
    return out;
}

IsleGameModelRef isle_volcano_actor_player_model(IsleVolcanoActorSource* s) {
    IsleGameModelRef out={0};
    isle_volcano_actor_enter(s);
    IsleGameResult result=s->services->player_model(s, &out);
    isle_volcano_actor_checked(s,result);
    return out;
}

int32_t isle_volcano_actor_sound_play(IsleVolcanoActorSource* s, int32_t id) {
    int32_t out={0};
    isle_volcano_actor_enter(s);
    IsleGameResult result=s->services->sound_play(s, id, &out);
    isle_volcano_actor_checked(s,result);
    return out;
}

int32_t isle_volcano_actor_sound_play_params(IsleVolcanoActorSource* s, int32_t id, int32_t volume, int32_t pan, float pitch, int32_t priority) {
    int32_t out={0};
    isle_volcano_actor_enter(s);
    IsleGameResult result=s->services->sound_play_params(s, id, volume, pan, pitch, priority, &out);
    isle_volcano_actor_checked(s,result);
    return out;
}

int32_t isle_volcano_actor_sound_identity(IsleVolcanoActorSource* s, uint32_t handle) {
    int32_t out={0};
    isle_volcano_actor_enter(s);
    IsleGameResult result=s->services->sound_identity(s, handle, &out);
    isle_volcano_actor_checked(s,result);
    return out;
}

void isle_volcano_actor_sound_stop(IsleVolcanoActorSource* s, uint32_t handle) {
        isle_volcano_actor_enter(s);
    IsleGameResult result=s->services->sound_stop(s, handle);
    isle_volcano_actor_checked(s,result);
}

void isle_volcano_actor_sound_volume(IsleVolcanoActorSource* s, uint32_t handle, int32_t volume) {
        isle_volcano_actor_enter(s);
    IsleGameResult result=s->services->sound_volume(s, handle, volume);
    isle_volcano_actor_checked(s,result);
}

void isle_volcano_actor_sound_stop_all(IsleVolcanoActorSource* s) {
        isle_volcano_actor_enter(s);
    IsleGameResult result=s->services->sound_stop_all(s);
    isle_volcano_actor_checked(s,result);
}

void isle_volcano_actor_bgm_volume(IsleVolcanoActorSource* s, int32_t player, int32_t volume) {
        isle_volcano_actor_enter(s);
    IsleGameResult result=s->services->bgm_volume(s, player, volume);
    isle_volcano_actor_checked(s,result);
}

void isle_volcano_actor_bgm_stop(IsleVolcanoActorSource* s) {
        isle_volcano_actor_enter(s);
    IsleGameResult result=s->services->bgm_stop(s);
    isle_volcano_actor_checked(s,result);
}

void isle_volcano_actor_bgm_play(IsleVolcanoActorSource* s, int32_t song) {
        isle_volcano_actor_enter(s);
    IsleGameResult result=s->services->bgm_play(s, song);
    isle_volcano_actor_checked(s,result);
}

void isle_volcano_actor_environment_sound(IsleVolcanoActorSource* s, IsleGameObject* object, uint8_t category, int32_t sound) {
        isle_volcano_actor_enter(s);
    IsleGameResult result=s->services->environment_sound(s, object, category, sound);
    isle_volcano_actor_checked(s,result);
}

void isle_volcano_actor_remove_items(IsleVolcanoActorSource* s) {
        isle_volcano_actor_enter(s);
    IsleGameResult result=s->services->remove_items(s);
    isle_volcano_actor_checked(s,result);
}

void isle_volcano_actor_reset_camera(IsleVolcanoActorSource* s) {
        isle_volcano_actor_enter(s);
    IsleGameResult result=s->services->reset_camera(s);
    isle_volcano_actor_checked(s,result);
}

void isle_volcano_actor_reset_messages(IsleVolcanoActorSource* s) {
        isle_volcano_actor_enter(s);
    IsleGameResult result=s->services->reset_messages(s);
    isle_volcano_actor_checked(s,result);
}

void isle_volcano_actor_cutscene_start(IsleVolcanoActorSource* s, IsleGameObject* object, uint32_t optional_clip_identity, float frame) {
        isle_volcano_actor_enter(s);
    IsleGameResult result=s->services->cutscene_start(s, object, optional_clip_identity, frame);
    isle_volcano_actor_checked(s,result);
}

IsleGameCamera* isle_volcano_actor_main_camera(IsleVolcanoActorSource* s) {
    IsleGameCamera* out={0};
    isle_volcano_actor_enter(s);
    IsleGameResult result=s->services->main_camera(s, &out);
    isle_volcano_actor_checked(s,result);
    return out;
}

IsleGameObject* isle_volcano_actor_camera_object(IsleVolcanoActorSource* s, IsleGameCamera* camera) {
    IsleGameObject* out={0};
    isle_volcano_actor_enter(s);
    IsleGameResult result=s->services->camera_object(s, camera, &out);
    isle_volcano_actor_checked(s,result);
    return out;
}

void isle_volcano_actor_camera_speed(IsleVolcanoActorSource* s, IsleGameCamera* camera, float speed) {
        isle_volcano_actor_enter(s);
    IsleGameResult result=s->services->camera_speed(s, camera, speed);
    isle_volcano_actor_checked(s,result);
}

void isle_volcano_actor_camera_animation(IsleVolcanoActorSource* s, IsleGameCamera* camera, uint32_t clip, float frame) {
        isle_volcano_actor_enter(s);
    IsleGameResult result=s->services->camera_animation(s, camera, clip, frame);
    isle_volcano_actor_checked(s,result);
}

IsleGameProcess* isle_volcano_actor_camera_process(IsleVolcanoActorSource* s, IsleGameObject* object, int32_t kind, int32_t priority) {
    IsleGameProcess* out={0};
    isle_volcano_actor_enter(s);
    IsleGameResult result=s->services->camera_process(s, object, kind, priority, &out);
    isle_volcano_actor_checked(s,result);
    return out;
}

void isle_volcano_actor_pause_processes(IsleVolcanoActorSource* s, IsleGameObject* object) {
        isle_volcano_actor_enter(s);
    IsleGameResult result=s->services->pause_processes(s, object);
    isle_volcano_actor_checked(s,result);
}

void isle_volcano_actor_sky_speed(IsleVolcanoActorSource* s, float speed) {
        isle_volcano_actor_enter(s);
    IsleGameResult result=s->services->sky_speed(s, speed);
    isle_volcano_actor_checked(s,result);
}

IsleGameObject* isle_volcano_actor_player_model_init(IsleVolcanoActorSource* s) {
    IsleGameObject* out={0};
    isle_volcano_actor_enter(s);
    IsleGameResult result=s->services->player_model_init(s, &out);
    isle_volcano_actor_checked(s,result);
    return out;
}

void isle_volcano_actor_player_animation(IsleVolcanoActorSource* s, uint32_t model, uint32_t material, float frame, float speed) {
        isle_volcano_actor_enter(s);
    IsleGameResult result=s->services->player_animation(s, model, material, frame, speed);
    isle_volcano_actor_checked(s,result);
}

void isle_volcano_actor_animation_callback(IsleVolcanoActorSource* s, IsleGameObject* object, IsleVolcanoAnimationCallback callback) {
        isle_volcano_actor_enter(s);
    IsleGameResult result=s->services->animation_callback(s, object, callback);
    isle_volcano_actor_checked(s,result);
}

void isle_volcano_actor_animation_speed(IsleVolcanoActorSource* s, IsleGameObject* object, float speed) {
        isle_volcano_actor_enter(s);
    IsleGameResult result=s->services->animation_speed(s, object, speed);
    isle_volcano_actor_checked(s,result);
}

void isle_volcano_actor_change_block_ground(IsleVolcanoActorSource* s, IsleGameObject* object, float v0, float v1, float v2, float v3, float v4, float v5) {
        isle_volcano_actor_enter(s);
    IsleGameResult result=s->services->change_block_ground(s, object, v0, v1, v2, v3, v4, v5);
    isle_volcano_actor_checked(s,result);
}

void isle_volcano_actor_gb_pipe_sync(IsleVolcanoActorSource* s, uint32_t stream) {
        isle_volcano_actor_enter(s);
    IsleGameResult result=s->services->gb_pipe_sync(s, stream);
    isle_volcano_actor_checked(s,result);
}

void isle_volcano_actor_gb_cycle(IsleVolcanoActorSource* s, uint32_t stream, IsleVolcanoCycle cycle) {
        isle_volcano_actor_enter(s);
    IsleGameResult result=s->services->gb_cycle(s, stream, cycle);
    isle_volcano_actor_checked(s,result);
}

void isle_volcano_actor_gb_render_mode(IsleVolcanoActorSource* s, uint32_t stream, IsleVolcanoRenderMode first, IsleVolcanoRenderMode second) {
        isle_volcano_actor_enter(s);
    IsleGameResult result=s->services->gb_render_mode(s, stream, first, second);
    isle_volcano_actor_checked(s,result);
}

void isle_volcano_actor_gb_texture_lut(IsleVolcanoActorSource* s, uint32_t stream, IsleVolcanoTextureLut lut) {
        isle_volcano_actor_enter(s);
    IsleGameResult result=s->services->gb_texture_lut(s, stream, lut);
    isle_volcano_actor_checked(s,result);
}

void isle_volcano_actor_render_type_i(IsleVolcanoActorSource* s, IsleGameObject* object) {
        isle_volcano_actor_enter(s);
    IsleGameResult result=s->services->render_type_i(s, object);
    isle_volcano_actor_checked(s,result);
}

void isle_volcano_actor_render_type_d_fogged(IsleVolcanoActorSource* s, IsleGameObject* object) {
        isle_volcano_actor_enter(s);
    IsleGameResult result=s->services->render_type_d_fogged(s, object);
    isle_volcano_actor_checked(s,result);
}

float isle_volcano_actor_vector_distance(IsleVolcanoActorSource* s, const IsleGameVec3* a, const IsleGameVec3* b) {
    float out={0};
    isle_volcano_actor_enter(s);
    IsleGameResult result=s->services->vector_distance(s, a, b, &out);
    isle_volcano_actor_checked(s,result);
    return out;
}

typedef struct VolcanoAnimationCall { IsleVolcanoActorSource* source; const IsleGameModelRef* model; int32_t event; float value; } VolcanoAnimationCall;
static void invoke_animation_c(void* p) { VolcanoAnimationCall* c=p; volcano_gate_PlayerAnimCallback(c->source,c->model,c->event,c->value); }
IsleGameResult isle_volcano_actor_invoke_animation(IsleVolcanoActorSource* s,const IsleGameModelRef* model,int32_t event,float value) {
    VolcanoAnimationCall call={s,model,event,value};
    if(!model)return ISLE_GAME_INVALID_ARGUMENT;
    return isle_volcano_actor_invoke(s,invoke_animation_c,&call);
}
typedef struct VolcanoCollisionCall { IsleGameObject* object; IsleActorGround* ground; } VolcanoCollisionCall;
static void invoke_collision_c(void* p) { VolcanoCollisionCall* c=p; volcano_HandleCollision(c->object,c->ground); }
IsleGameResult isle_volcano_actor_invoke_collision(IsleVolcanoActorSource* s,IsleGameObject* object,IsleActorGround* ground) {
    VolcanoCollisionCall call={object,ground};
    if(!object || !ground)return ISLE_GAME_INVALID_ARGUMENT;
    return isle_volcano_actor_invoke(s,invoke_collision_c,&call);
}
typedef struct VolcanoRenderCall { IsleGameObject* object; uint32_t identity; } VolcanoRenderCall;
static void invoke_render_c(void* p) { VolcanoRenderCall* c=p;
    if(c->identity==0x802de26cu)moltres_Render(c->object);
    else volcano_smoke_Render(c->object);
}
IsleGameResult isle_volcano_actor_invoke_render(IsleVolcanoActorSource* s,IsleGameObject* object,uint32_t identity) {
    VolcanoRenderCall call={object,identity};
    if(!object || (identity!=0x802de26cu && identity!=0x802dfae4u))return ISLE_GAME_INVALID_ARGUMENT;
    return isle_volcano_actor_invoke(s,invoke_render_c,&call);
}
static void invoke_movement_c(void* p) { IsleGameObject* object=p; volcano_PokemonMove(object); }
IsleGameResult isle_volcano_actor_invoke_movement(IsleVolcanoActorSource* s,IsleGameObject* object) {
    if(!object)return ISLE_GAME_INVALID_ARGUMENT;
    return isle_volcano_actor_invoke(s,invoke_movement_c,object);
}
typedef struct VolcanoSoundCall { IsleVolcanoActorSource* source; int32_t block; float fraction; } VolcanoSoundCall;
static void invoke_sounds_c(void* p) { VolcanoSoundCall* c=p; volcano_UpdateSounds(c->source,c->block,c->fraction); }
IsleGameResult isle_volcano_actor_invoke_sounds(IsleVolcanoActorSource* s,int32_t block,float fraction) {
    VolcanoSoundCall call={s,block,fraction}; return isle_volcano_actor_invoke(s,invoke_sounds_c,&call);
}
typedef struct VolcanoBlockCall { IsleGameObject* object; float previous_x,previous_y,previous_z,current_x,current_y,current_z; } VolcanoBlockCall;
static void invoke_block_c(void* p) { VolcanoBlockCall* c=p; volcano_CharmeleonChangeBlock(c->object,c->previous_x,c->previous_y,c->previous_z,c->current_x,c->current_y,c->current_z); }
IsleGameResult isle_volcano_actor_invoke_change_block(IsleVolcanoActorSource* s,IsleGameObject* object,float px,float py,float pz,float cx,float cy,float cz) {
    VolcanoBlockCall call={object,px,py,pz,cx,cy,cz};
    if(!object)return ISLE_GAME_INVALID_ARGUMENT;
    return isle_volcano_actor_invoke(s,invoke_block_c,&call);
}
