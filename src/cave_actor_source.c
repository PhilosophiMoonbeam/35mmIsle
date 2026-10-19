#include "cave_actor_source.h"
#include "engine_internal.h"
#include <stdlib.h>
#include <string.h>
static uint16_t be16(const uint8_t* p) { return (uint16_t)((uint16_t)p[0]<<8 | p[1]); }
static uint32_t be32(const uint8_t* p) { return (uint32_t)p[0]<<24 | (uint32_t)p[1]<<16 | (uint32_t)p[2]<<8 | p[3]; }
static int32_t signed32(uint32_t v) { return v<=INT32_MAX ? (int32_t)v : -1-(int32_t)(UINT32_MAX-v); }
static float bef32(const uint8_t* p) { uint32_t b=be32(p); float f; memcpy(&f,&b,4); return f; }
static int services_complete(const IsleCaveActorServices* p) { return p &&
    p->model &&
    p->model_read &&
    p->model_store &&
    p->model_child_flags &&
    p->model_speed &&
    p->player_model_ref &&
    p->player_object &&
    p->reference &&
    p->current_block &&
    p->block_position &&
    p->add_one &&
    p->add_geo &&
    p->sound &&
    p->sound_params &&
    p->sound_active &&
    p->sound_stop &&
    p->sound_volume &&
    p->sound_stop_all &&
    p->bgm_play &&
    p->bgm_stop &&
    p->bgm_volume &&
    p->bgm_channel_volume &&
    p->bgm_reverb &&
    p->global_reverb &&
    p->background_music &&
    p->texture_speed &&
    p->animation_speed &&
    p->header_speed &&
    p->level_progress &&
    p->ground &&
    p->remove_flying_items &&
    p->camera_reset &&
    p->messages_reset &&
    p->cutscene_start &&
    p->process_pause &&
    p->camera_speed &&
    p->camera_animation &&
    p->player_animation &&
    p->animation_callback &&
    p->remove_pokemon &&
    p->environment_sound &&
    p->path_save &&
    p->main_camera &&
    p->player_model_init &&
    p->flute &&
    p->path_restore &&
    p->path_point &&
    p->interpolated_position &&
    p->camera_process &&
    p->sky_speed &&
    p->item_collision_velocity &&
    p->camera_object &&
    p->rand &&
    p->distance &&
    p->distance_vectors &&
    p->weighted &&
    p->path_assign; }
IsleGameResult isle_cave_actor_prepare(IsleCaveActorSource* s, void* owner, const uint8_t* rom, size_t bytes, const IsleEggPreparation* egg_preparation) {
    if (!s || !owner || !rom || bytes!=0x1000000u) return ISLE_GAME_INVALID_ARGUMENT;
    if (s->base.phase!=ISLE_COURSE_ACTOR_UNPREPARED || s->base.owner || s->base.nativeGame || s->base.nonwaiting_call_frame || s->services || s->base.status!=ISLE_GAME_OK) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x6491fc)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x649200)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x649204)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x649208)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x649750)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x649754)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x649758)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x64975c)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x649a9c)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x649aa0)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x649ffc)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x64a0ec)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x64a4f4)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x648804)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x648828)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x6488e0)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x648f54)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x6491fc)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x649200)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x649204)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x649208)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x649750)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x649754)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x649758)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x64975c)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x64974c)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x6498c0)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x6498c4)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x6498c8)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x64992c]!=UINT32_C(0x3)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x64992d]!=UINT32_C(0x4)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x64992e]!=UINT32_C(0x5)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x649930]!=UINT32_C(0x3)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x649931]!=UINT32_C(0x4)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x649932]!=UINT32_C(0x5)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x649933]!=UINT32_C(0x6)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x649934]!=UINT32_C(0x7)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x649935]!=UINT32_C(0x8)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x649936]!=UINT32_C(0x9)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x649937]!=UINT32_C(0xa)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x649938]!=UINT32_C(0xb)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x649939]!=UINT32_C(0xc)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x64993a]!=UINT32_C(0xd)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x64993b]!=UINT32_C(0xe)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x649a9c)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x649aa0)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x649e30)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x649e74)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x649e78)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x649e7c)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x649ffc)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x64a0ec)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x64a0d8)!=UINT32_C(0x8017a780)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x64a0dc)!=UINT32_C(0x802c7c20)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x64a0e0)!=UINT32_C(0x144)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x64a0e4)!=UINT32_C(0x2)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x64a0e8)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x1f5ea8)!=UINT32_C(0x40c00000)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x64a470)!=UINT32_C(0xffffffff)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x64a474)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x64a498)!=UINT32_C(0x3e9)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x64a49a)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x64a4f0)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x64a4f4)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x64920c]!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x64920d]!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x64920e]!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x64920f]!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x64a0d0)!=UINT32_C(0x8033e210)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x64a0d4)!=UINT32_C(0x8017c090)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x648818]!=3 || be32(rom+0x64881c)!=0x802bdd00u || rom[0x648820]!=7 || be32(rom+0x648824)) return ISLE_GAME_INVALID_STATE;
    s->D_802C6354_648804=be16(rom+0x648804);
    s->D_802C6378_648828=signed32(be32(rom+0x648828));
    s->D_802C6430_6488E0=be32(rom+0x6488e0);
    s->D_802C6AA4_648F54=bef32(rom+0x648f54);
    s->D_802C6D4C_6491FC[0]=NULL;
    s->D_802C6D4C_6491FC[1]=NULL;
    s->D_802C6D4C_6491FC[2]=NULL;
    s->D_802C6D4C_6491FC[3]=NULL;
    s->D_802C72A0_649750[0]=NULL;
    s->D_802C72A0_649750[1]=NULL;
    s->D_802C72A0_649750[2]=NULL;
    s->D_802C72A0_649750[3]=NULL;
    s->D_802C729C_64974C=bef32(rom+0x64974c);
    s->D_802C7410_6498C0.x=bef32(rom+0x6498c0);
    s->D_802C7410_6498C0.y=bef32(rom+0x6498c4);
    s->D_802C7410_6498C0.z=bef32(rom+0x6498c8);
    memcpy(s->D_802C747C_64992C.unk_00,rom+0x64992c,sizeof(s->D_802C747C_64992C.unk_00));
    memcpy(s->D_802C7480_649930.unk_00,rom+0x649930,sizeof(s->D_802C7480_649930.unk_00));
    s->D_802C75EC_649A9C=NULL;
    s->D_802C75F0_649AA0=NULL;
    s->D_802C7980_649E30=bef32(rom+0x649e30);
    s->D_802C79C4_649E74.x=bef32(rom+0x649e74);
    s->D_802C79C4_649E74.y=bef32(rom+0x649e78);
    s->D_802C79C4_649E74.z=bef32(rom+0x649e7c);
    s->D_802C7B4C_649FFC=NULL;
    s->D_802C7C3C_64A0EC=NULL;
    s->D_802C7C28_64A0D8.data_identity=be32(rom+0x64a0d8);
    s->D_802C7C28_64A0D8.temp_buffer_identity=be32(rom+0x64a0dc);
    s->D_802C7C28_64A0D8.vertex_count=signed32(be32(rom+0x64a0e0));
    s->D_802C7C28_64A0D8.end=signed32(be32(rom+0x64a0e4));
    s->D_802C7C28_64A0D8.timer=bef32(rom+0x64a0e8);
    s->D_8012A0F8_1F5EA8=bef32(rom+0x1f5ea8);
    s->D_802C7FC0_64A470=signed32(be32(rom+0x64a470));
    s->D_802C7FC4_64A474=signed32(be32(rom+0x64a474));
    s->D_802C7FE8_64A498.values[0]=be16(rom+0x64a498);
    s->D_802C7FE8_64A498.values[1]=be16(rom+0x64a49a);
    s->D_802C8040_64A4F0=signed32(be32(rom+0x64a4f0));
    s->D_802C8044_64A4F4=NULL;
    memset(&s->cave_unused_bss,0,sizeof(s->cave_unused_bss));
    memset(&s->D_8033E130_6C05E0,0,sizeof(s->D_8033E130_6C05E0));
    memset(&s->D_8033E138_6C05E8,0,sizeof(s->D_8033E138_6C05E8));
    memset(&s->D_8033E140_6C05E0,0,sizeof(s->D_8033E140_6C05E0));
    memset(&s->D_8033E148_6C05F8,0,sizeof(s->D_8033E148_6C05F8));
    memset(&s->D_8033E149_6C05F9,0,sizeof(s->D_8033E149_6C05F9));
    memset(&s->D_8033E14A_6C05FA,0,sizeof(s->D_8033E14A_6C05FA));
    memset(&s->D_8033E14C_6C05FC,0,sizeof(s->D_8033E14C_6C05FC));
    memset(&s->D_8033E150_6C0600,0,sizeof(s->D_8033E150_6C0600));
    memset(&s->D_8033F690_6C05E0,0,sizeof(s->D_8033F690_6C05E0));
    memset(&s->D_8033F698_6C1B48,0,sizeof(s->D_8033F698_6C1B48));
    memset(&s->D_8033F699_6C1B49,0,sizeof(s->D_8033F699_6C1B49));
    memset(&s->D_8033F69A_6C1B4A,0,sizeof(s->D_8033F69A_6C1B4A));
    memset(&s->D_8033F69B_6C1B4B,0,sizeof(s->D_8033F69B_6C1B4B));
    memset(&s->D_8033F69C_6C1B4C,0,sizeof(s->D_8033F69C_6C1B4C));
    memset(&s->D_8033F6A0_6C1B50,0,sizeof(s->D_8033F6A0_6C1B50));
    memcpy(s->D_802C6D5C_64920C,rom+0x64920c,4);
    s->D_802C6368_648818[0]=(IsleCaveCountedOption){3,func_802BDD00_6401B0};
    s->D_802C6368_648818[1]=(IsleCaveCountedOption){7,NULL};
    s->base.course_id=4; s->base.owner=owner;
    {
        IsleGameResult result=isle_egg_source_prepare(&s->D_802C7C28_64A0D8,&s->base,s,owner,rom,bytes,
            s->D_8033E150_6C0600.frame0,324,egg_preparation);
        if (result!=ISLE_GAME_OK) { s->base.status=result; return result; }
    }
    s->base.phase=ISLE_COURSE_ACTOR_PREPARED;
    return ISLE_GAME_OK;
}
IsleGameResult isle_cave_actor_activate(IsleCaveActorSource* s, IsleGame* game, const IsleCaveActorServices* services) {
    IsleGameResult r;
    if (!s || !game || !services_complete(services)) return ISLE_GAME_INVALID_ARGUMENT;
    if (s->base.status!=ISLE_GAME_OK) return s->base.status;
    if (s->base.course_id!=4 || !s->base.owner || s->base.nonwaiting_call_frame) return ISLE_GAME_INVALID_STATE;
    if (s->base.phase==ISLE_COURSE_ACTOR_ACTIVE) {
        if (s->base.nativeGame!=game || s->services!=services) return ISLE_GAME_INVALID_STATE;
        r=isle_cave_actor_native_admission(s,game,services);
        if (s->base.status==ISLE_GAME_OK && r!=ISLE_GAME_OK) s->base.status=r;
        return r;
    }
    if (s->base.phase!=ISLE_COURSE_ACTOR_PREPARED || s->base.nativeGame || s->services) return ISLE_GAME_INVALID_STATE;
    r=isle_cave_actor_native_admission(s,game,services);
    if (r!=ISLE_GAME_OK) { if (s->base.status==ISLE_GAME_OK) s->base.status=r; return r; }
    s->base.nativeGame=game; s->services=services;
    s->base.phase=ISLE_COURSE_ACTOR_ACTIVE;
    return ISLE_GAME_OK;
}
IsleGameResult isle_cave_actor_deactivate(IsleCaveActorSource* s) {
    IsleGameResult r;
    if (!s || s->base.phase!=ISLE_COURSE_ACTOR_ACTIVE || s->base.nonwaiting_call_frame) return ISLE_GAME_INVALID_STATE;
    r=isle_actor_source_retirement_status(s->base.nativeGame);
    if (s->base.status==ISLE_GAME_OK && r!=ISLE_GAME_OK) s->base.status=r;
    r=isle_cave_actor_native_retirement(s);
    if (r!=ISLE_GAME_OK) { if (s->base.status==ISLE_GAME_OK) s->base.status=r; return r; }
    s->base.nativeGame=NULL; s->services=NULL;
    s->base.phase=ISLE_COURSE_ACTOR_DETACHED;
    return ISLE_GAME_OK;
}
void isle_cave_actor_checked(IsleCaveActorSource* s, IsleGameResult r) {
    if (r==ISLE_GAME_OK && s) r=s->base.status;
    isle_course_actor_checked(s ? &s->base : NULL,r);
}
IsleCaveActorSource* isle_cave_actor_require(IsleGame* game, IsleGameObject* object) {
    IsleCaveActorSource* s=NULL;
    IsleGameResult r=isle_cave_actor_native_context(game,object,&s);
    if (r!=ISLE_GAME_OK || !s) { isle_native_execution_fault(game,"Cave canonical receiver context failed"); abort(); }
    if (s->base.phase!=ISLE_COURSE_ACTOR_ACTIVE || s->base.nativeGame!=game || !services_complete(s->services))
        isle_cave_actor_checked(s,ISLE_GAME_INVALID_STATE);
    isle_cave_actor_checked(s,isle_cave_actor_native_admission(s,game,s->services));
    return s;
}
IsleCaveActorSource* isle_cave_actor_factory_require(const IsleActorFactoryBlock* a,const IsleActorFactoryBlock* b,const IsleActorFactorySpawn* spawn) {
    IsleCaveActorSource* s=NULL;
    IsleGameResult r=isle_cave_actor_native_factory_context(a,b,spawn,&s);
    isle_actor_source_factory_checked(a,b,spawn,r);
    if (!s) isle_actor_source_factory_checked(a,b,spawn,ISLE_GAME_INVALID_STATE);
    if (!s) abort();
    if (s->base.phase!=ISLE_COURSE_ACTOR_ACTIVE || !s->base.nativeGame || !services_complete(s->services))
        isle_actor_source_factory_checked(a,b,spawn,ISLE_GAME_INVALID_STATE);
    isle_cave_actor_checked(s,isle_cave_actor_native_admission(s,s->base.nativeGame,s->services));
    return s;
}
void isle_cave_actor_visibility(IsleCaveActorSource* s,IsleGameObject* object,int hidden) {
    IsleActorSource* actor=isle_actor_source_require(object);
    IsleGameObjectMetadata metadata;
    actor->tangible=hidden ? 0 : 1;
    isle_cave_actor_checked(s,isle_game_object_metadata(s->base.nativeGame,object,&metadata));
    isle_cave_actor_checked(s,isle_game_object_set_flags(s->base.nativeGame,object,hidden ? metadata.flags|3u : 0u));
}
void isle_cave_actor_copy_graph(IsleCaveActorSource* s,IsleGameObject* object,uint32_t id,IsleActorInteraction* rows,IsleActorInteractionGraph* destination) {
    const IsleActorInteractionGraph* source=isle_actor_source_graph(object,id);
    if (!source || source->count!=destination->count || !rows || destination->rows!=rows)
        isle_cave_actor_checked(s,ISLE_GAME_INVALID_STATE);
    memcpy(rows,source->rows,source->count*sizeof(*rows));
    /* Automatic native graph identity remains zero; exact owner/function/line
     * provenance is retained in actor_dependencies(), not an invented guest ID. */
}
IsleCaveDefinition isle_cave_actor_definition(IsleCaveActorSource* s,uint32_t id) {
    switch(id) {
    case 0x802c6358u: return (IsleCaveDefinition){0x802c6358u,0x0u,0x81u,0x802c3388u,0x80363eb4u,0x80364280u,0x802c77fcu};
    case 0x802c65d8u: return (IsleCaveDefinition){0x802c65d8u,0x0u,0x84u,0x802bf55cu,0x80363eb4u,0x80364280u,0x802c69c0u};
    case 0x802c6fc4u: return (IsleCaveDefinition){0x802c6fc4u,0x0u,0x59u,0x802c13b4u,0x80363dbcu,0x80364280u,0x802c714cu};
    default: isle_cave_actor_checked(s,ISLE_GAME_INVALID_ARGUMENT); abort();
    }
}
const IsleCaveDefinition* isle_cave_actor_common1003_definition(void) {
    static const IsleCaveDefinition definition={0x8038837cu,0,1003,0x8035e0d4u,0x80363dbcu,0x80364280u,0x80388348u,0,0};
    return &definition;
}
IsleGameModelRef isle_cave_actor_model(IsleCaveActorSource* s, IsleGameObject* object) {
    IsleGameModelRef output = {0};
    isle_cave_actor_checked(s, s->services->model(s, object, &output));
    return output;
}
float isle_cave_actor_model_read(IsleCaveActorSource* s, IsleGameModelRef model, uint32_t component) {
    float output = {0};
    isle_cave_actor_checked(s, s->services->model_read(s, model, component, &output));
    return output;
}
void isle_cave_actor_model_store(IsleCaveActorSource* s, IsleGameModelRef model, uint32_t component, float value) {
    isle_cave_actor_checked(s, s->services->model_store(s, model, component, value));
}
void isle_cave_actor_model_child_flags(IsleCaveActorSource* s, IsleGameObject* object, uint32_t mask, uint32_t value) {
    isle_cave_actor_checked(s, s->services->model_child_flags(s, object, mask, value));
}
float isle_cave_actor_model_speed(IsleCaveActorSource* s, IsleGameModelRef model) {
    float output = {0};
    isle_cave_actor_checked(s, s->services->model_speed(s, model, &output));
    return output;
}
IsleGameModelRef isle_cave_actor_player_model_ref(IsleCaveActorSource* s) {
    IsleGameModelRef output = {0};
    isle_cave_actor_checked(s, s->services->player_model_ref(s, &output));
    return output;
}
IsleGameObject* isle_cave_actor_player_object(IsleCaveActorSource* s) {
    IsleGameObject* output = {0};
    isle_cave_actor_checked(s, s->services->player_object(s, &output));
    return output;
}
void isle_cave_actor_reference(IsleCaveActorSource* s, IsleGameObject** field, IsleGameObject* object) {
    isle_cave_actor_checked(s, s->services->reference(s, field, object));
}
IsleCaveWorldBlock* isle_cave_actor_current_block(IsleCaveActorSource* s) {
    IsleCaveWorldBlock* output = {0};
    isle_cave_actor_checked(s, s->services->current_block(s, &output));
    return output;
}
IsleGameVec3 isle_cave_actor_block_position(IsleCaveActorSource* s, IsleCaveWorldBlock* block) {
    IsleGameVec3 output = {0};
    isle_cave_actor_checked(s, s->services->block_position(s, block, &output));
    return output;
}
IsleGameObject* isle_cave_actor_add_one(IsleCaveActorSource* s, IsleCaveWorldBlock* a, IsleCaveWorldBlock* b, const IsleCaveAutomaticSpawn* spawn, const IsleCaveDefinition* definition) {
    IsleGameObject* output = {0};
    isle_cave_actor_checked(s, s->services->add_one(s, a, b, spawn, definition, &output));
    return output;
}
IsleGameObject* isle_cave_actor_add_geo(IsleCaveActorSource* s, IsleGameObject* object, uint16_t id, const IsleCaveDefinition* definition) {
    IsleGameObject* output = {0};
    isle_cave_actor_checked(s, s->services->add_geo(s, object, id, definition, &output));
    return output;
}
int32_t isle_cave_actor_sound(IsleCaveActorSource* s, int32_t id) {
    int32_t output = {0};
    isle_cave_actor_checked(s, s->services->sound(s, id, &output));
    return output;
}
int32_t isle_cave_actor_sound_params(IsleCaveActorSource* s, int32_t id, int32_t volume, int32_t pan, float pitch, int32_t reverb) {
    int32_t output = {0};
    isle_cave_actor_checked(s, s->services->sound_params(s, id, volume, pan, pitch, reverb, &output));
    return output;
}
int32_t isle_cave_actor_sound_active(IsleCaveActorSource* s, int32_t handle) {
    int32_t output = {0};
    isle_cave_actor_checked(s, s->services->sound_active(s, handle, &output));
    return output;
}
void isle_cave_actor_sound_stop(IsleCaveActorSource* s, int32_t handle) {
    isle_cave_actor_checked(s, s->services->sound_stop(s, handle));
}
void isle_cave_actor_sound_volume(IsleCaveActorSource* s, int32_t handle, int32_t volume) {
    isle_cave_actor_checked(s, s->services->sound_volume(s, handle, volume));
}
void isle_cave_actor_sound_stop_all(IsleCaveActorSource* s) {
    isle_cave_actor_checked(s, s->services->sound_stop_all(s));
}
void isle_cave_actor_bgm_play(IsleCaveActorSource* s, int32_t player, int32_t id) {
    isle_cave_actor_checked(s, s->services->bgm_play(s, player, id));
}
void isle_cave_actor_bgm_stop(IsleCaveActorSource* s, int32_t player) {
    isle_cave_actor_checked(s, s->services->bgm_stop(s, player));
}
void isle_cave_actor_bgm_volume(IsleCaveActorSource* s, int32_t player, uint32_t volume) {
    isle_cave_actor_checked(s, s->services->bgm_volume(s, player, volume));
}
void isle_cave_actor_bgm_channel_volume(IsleCaveActorSource* s, int32_t player, uint8_t channel, int32_t volume) {
    isle_cave_actor_checked(s, s->services->bgm_channel_volume(s, player, channel, volume));
}
void isle_cave_actor_bgm_reverb(IsleCaveActorSource* s, int32_t player, int32_t reverb) {
    isle_cave_actor_checked(s, s->services->bgm_reverb(s, player, reverb));
}
void isle_cave_actor_global_reverb(IsleCaveActorSource* s, int32_t reverb) {
    isle_cave_actor_checked(s, s->services->global_reverb(s, reverb));
}
void isle_cave_actor_background_music(IsleCaveActorSource* s, int32_t id) {
    isle_cave_actor_checked(s, s->services->background_music(s, id));
}
void isle_cave_actor_texture_speed(IsleCaveActorSource* s, IsleGameObject* object, float speed) {
    isle_cave_actor_checked(s, s->services->texture_speed(s, object, speed));
}
void isle_cave_actor_animation_speed(IsleCaveActorSource* s, IsleGameObject* object, float speed) {
    isle_cave_actor_checked(s, s->services->animation_speed(s, object, speed));
}
float isle_cave_actor_header_speed(IsleCaveActorSource* s, uint32_t header_identity) {
    float output = {0};
    isle_cave_actor_checked(s, s->services->header_speed(s, header_identity, &output));
    return output;
}
void isle_cave_actor_level_progress(IsleCaveActorSource* s, int32_t* block, float* fraction) {
    isle_cave_actor_checked(s, s->services->level_progress(s, block, fraction));
}
void isle_cave_actor_ground(IsleCaveActorSource* s, float x, float z, IsleActorGround* ground) {
    isle_cave_actor_checked(s, s->services->ground(s, x, z, ground));
}
void isle_cave_actor_remove_flying_items(IsleCaveActorSource* s) {
    isle_cave_actor_checked(s, s->services->remove_flying_items(s));
}
void isle_cave_actor_camera_reset(IsleCaveActorSource* s) {
    isle_cave_actor_checked(s, s->services->camera_reset(s));
}
void isle_cave_actor_messages_reset(IsleCaveActorSource* s) {
    isle_cave_actor_checked(s, s->services->messages_reset(s));
}
void isle_cave_actor_cutscene_start(IsleCaveActorSource* s, IsleGameObject* object, uint32_t animation, float duration) {
    isle_cave_actor_checked(s, s->services->cutscene_start(s, object, animation, duration));
}
void isle_cave_actor_process_pause(IsleCaveActorSource* s, IsleGameObject* object) {
    isle_cave_actor_checked(s, s->services->process_pause(s, object));
}
void isle_cave_actor_camera_speed(IsleCaveActorSource* s, IsleCaveCamera* camera, float speed) {
    isle_cave_actor_checked(s, s->services->camera_speed(s, camera, speed));
}
void isle_cave_actor_camera_animation(IsleCaveActorSource* s, IsleCaveCamera* camera, uint32_t animation, float start) {
    isle_cave_actor_checked(s, s->services->camera_animation(s, camera, animation, start));
}
void isle_cave_actor_player_animation(IsleCaveActorSource* s, uint32_t models, uint32_t materials, float start, float speed) {
    isle_cave_actor_checked(s, s->services->player_animation(s, models, materials, start, speed));
}
void isle_cave_actor_animation_callback(IsleCaveActorSource* s, IsleGameObject* object, IsleCaveAnimationCallback callback) {
    isle_cave_actor_checked(s, s->services->animation_callback(s, object, callback));
}
void isle_cave_actor_remove_pokemon(IsleCaveActorSource* s, const IsleCaveRemovalList* list) {
    isle_cave_actor_checked(s, s->services->remove_pokemon(s, list));
}
void isle_cave_actor_environment_sound(IsleCaveActorSource* s, IsleGameObject* object, int32_t category, int32_t sound) {
    isle_cave_actor_checked(s, s->services->environment_sound(s, object, category, sound));
}
void isle_cave_actor_path_save(IsleCaveActorSource* s, IsleGameObject* object, const void* path) {
    isle_cave_actor_checked(s, s->services->path_save(s, object, path));
}
IsleCaveCamera* isle_cave_actor_main_camera(IsleCaveActorSource* s) {
    IsleCaveCamera* output = {0};
    isle_cave_actor_checked(s, s->services->main_camera(s, &output));
    return output;
}
IsleGameObject* isle_cave_actor_player_model_init(IsleCaveActorSource* s) {
    IsleGameObject* output = {0};
    isle_cave_actor_checked(s, s->services->player_model_init(s, &output));
    return output;
}
int32_t isle_cave_actor_flute(IsleCaveActorSource* s) {
    int32_t output = {0};
    isle_cave_actor_checked(s, s->services->flute(s, &output));
    return output;
}
const void* isle_cave_actor_path_restore(IsleCaveActorSource* s, IsleGameObject* object) {
    const void* output = {0};
    isle_cave_actor_checked(s, s->services->path_restore(s, object, &output));
    return output;
}
float isle_cave_actor_path_point(IsleCaveActorSource* s, const void* path, size_t index) {
    float output = {0};
    isle_cave_actor_checked(s, s->services->path_point(s, path, index, &output));
    return output;
}
void isle_cave_actor_interpolated_position(IsleCaveActorSource* s, IsleGameVec3* output, const void* path, float t) {
    isle_cave_actor_checked(s, s->services->interpolated_position(s, output, path, t));
}
IsleGameProcess* isle_cave_actor_camera_process(IsleCaveActorSource* s, IsleGameObject* object, int32_t kind, int32_t priority) {
    IsleGameProcess* output = {0};
    isle_cave_actor_checked(s, s->services->camera_process(s, object, kind, priority, &output));
    return output;
}
void isle_cave_actor_sky_speed(IsleCaveActorSource* s, float speed) {
    isle_cave_actor_checked(s, s->services->sky_speed(s, speed));
}
IsleGameVec3 isle_cave_actor_item_collision_velocity(IsleCaveActorSource* s, IsleGameObject* object) {
    IsleGameVec3 output = {0};
    isle_cave_actor_checked(s, s->services->item_collision_velocity(s, object, &output));
    return output;
}
IsleGameObject* isle_cave_actor_camera_object(IsleCaveActorSource* s, IsleCaveCamera* camera) {
    IsleGameObject* output = {0};
    isle_cave_actor_checked(s, s->services->camera_object(s, camera, &output));
    return output;
}
int32_t isle_cave_actor_rand(IsleCaveActorSource* s, int32_t range) {
    int32_t output = {0};
    isle_cave_actor_checked(s, s->services->rand(s, range, &output));
    return output;
}
float isle_cave_actor_distance(IsleCaveActorSource* s, IsleGameObject* a, IsleGameObject* b) {
    float output = {0};
    isle_cave_actor_checked(s, s->services->distance(s, a, b, &output));
    return output;
}
float isle_cave_actor_distance_vectors(IsleCaveActorSource* s, IsleCaveVectorRef a, IsleCaveVectorRef b) {
    float output = {0};
    isle_cave_actor_checked(s, s->services->distance_vectors(s, a, b, &output));
    return output;
}
const IsleActorRandomState* isle_cave_actor_weighted(IsleCaveActorSource* s, IsleGameObject* object, uint32_t identity) {
    const IsleActorRandomState* output = {0};
    isle_cave_actor_checked(s, s->services->weighted(s, object, identity, &output));
    return output;
}
void isle_cave_actor_path_assign(IsleCaveActorSource* s, IsleGameObject* object, const void* path) {
    isle_cave_actor_checked(s, s->services->path_assign(s, object, path));
}
static IsleGameResult actor_return(IsleCaveActorSource* s,IsleGameResult result) {
    if (s && s->base.status==ISLE_GAME_OK && result!=ISLE_GAME_OK) s->base.status=result;
    return s && s->base.status!=ISLE_GAME_OK ? s->base.status : result;
}

typedef struct CaveMovementCall { IsleCaveActorSource* source; int32_t block; float fraction; } CaveMovementCall;
static void movement_call(void* p) { CaveMovementCall* c=p; func_802BE450_640900(c->source,c->block,c->fraction); }
static IsleGameResult admission(IsleCaveActorSource* s) {
    if (!s || s->base.phase!=ISLE_COURSE_ACTOR_ACTIVE || !s->base.nativeGame || !services_complete(s->services)) return ISLE_GAME_INVALID_STATE;
    if (s->base.status!=ISLE_GAME_OK) return s->base.status;
    {
        IsleGameResult r=isle_cave_actor_native_admission(s,s->base.nativeGame,s->services);
        if (r!=ISLE_GAME_OK && s->base.status==ISLE_GAME_OK) s->base.status=r;
        return r;
    }
}
IsleGameResult isle_cave_actor_invoke_movement(IsleCaveActorSource* s,int32_t block,float fraction) {
    CaveMovementCall call={s,block,fraction}; IsleGameResult r=admission(s);
    return actor_return(s,r==ISLE_GAME_OK ? isle_course_actor_invoke(&s->base,movement_call,&call) : r);
}
typedef struct CaveItemCall { IsleGameObject* object; IsleActorGround* ground; } CaveItemCall;
static void item_call(void* p) { CaveItemCall* c=p; func_802BDDCC_64027C(c->object,c->ground); }
IsleGameResult isle_cave_actor_invoke_item(IsleCaveActorSource* s,IsleGameObject* object,IsleActorGround* ground) {
    IsleCaveActorSource* canonical=NULL; IsleGameResult r=admission(s); CaveItemCall call={object,ground};
    if (r!=ISLE_GAME_OK) return actor_return(s,r);
    if (!object || !ground) return actor_return(s,ISLE_GAME_INVALID_ARGUMENT);
    r=isle_cave_actor_native_context(s->base.nativeGame,object,&canonical);
    if (r!=ISLE_GAME_OK) return actor_return(s,r);
    if (canonical!=s) return actor_return(s,ISLE_GAME_INVALID_STATE);
    return actor_return(s,isle_course_actor_invoke(&s->base,item_call,&call));
}
typedef struct CaveAnimationCall { IsleCaveActorSource* source; const IsleGameModelRef* model; int32_t event; float value; } CaveAnimationCall;
static void animation_call(void* p) { CaveAnimationCall* c=p; func_802C5778_647C28(c->source,c->model,c->event,c->value); }
IsleGameResult isle_cave_actor_invoke_animation(IsleCaveActorSource* s,const IsleGameModelRef* model,int32_t event,float value) {
    IsleGameModel* resolved=NULL;
    IsleCaveActorSource* canonical=NULL; IsleGameResult r=admission(s); CaveAnimationCall call={s,model,event,value};
    if (r!=ISLE_GAME_OK) return actor_return(s,r);
    if (!model || !model->object) return actor_return(s,ISLE_GAME_INVALID_ARGUMENT);
    r=isle_game_model_resolve(s->base.nativeGame,model,&resolved);
    if (r!=ISLE_GAME_OK) return actor_return(s,r);
    r=isle_cave_actor_native_context(s->base.nativeGame,model->object,&canonical);
    if (r!=ISLE_GAME_OK) return actor_return(s,r);
    if (canonical!=s) return actor_return(s,ISLE_GAME_INVALID_STATE);
    return actor_return(s,isle_course_actor_invoke(&s->base,animation_call,&call));
}
IsleGameObject* isle_cave_actor_factory_func_802BEFD8_641488(int32_t object_id,uint16_t actor_id,IsleActorFactoryBlock* block,IsleActorFactoryBlock* reference,IsleActorFactorySpawn* spawn) {
    return func_802BEFD8_641488(object_id,actor_id,block,reference,spawn,NULL);
}
IsleGameObject* isle_cave_actor_factory_func_802BF55C_641A0C(int32_t object_id,uint16_t actor_id,IsleActorFactoryBlock* block,IsleActorFactoryBlock* reference,IsleActorFactorySpawn* spawn) {
    return func_802BF55C_641A0C(object_id,actor_id,block,reference,spawn,NULL);
}
IsleGameObject* isle_cave_actor_factory_func_802BFDA0_642250(int32_t object_id,uint16_t actor_id,IsleActorFactoryBlock* block,IsleActorFactoryBlock* reference,IsleActorFactorySpawn* spawn) {
    return func_802BFDA0_642250(object_id,actor_id,block,reference,spawn,NULL);
}
IsleGameObject* isle_cave_actor_factory_func_802C0104_6425B4(int32_t object_id,uint16_t actor_id,IsleActorFactoryBlock* block,IsleActorFactoryBlock* reference,IsleActorFactorySpawn* spawn) {
    return func_802C0104_6425B4(object_id,actor_id,block,reference,spawn,NULL);
}
IsleGameObject* isle_cave_actor_factory_func_802C07D4_642C84(int32_t object_id,uint16_t actor_id,IsleActorFactoryBlock* block,IsleActorFactoryBlock* reference,IsleActorFactorySpawn* spawn) {
    return func_802C07D4_642C84(object_id,actor_id,block,reference,spawn,NULL);
}
IsleGameObject* isle_cave_actor_factory_func_802C10C8_643578(int32_t object_id,uint16_t actor_id,IsleActorFactoryBlock* block,IsleActorFactoryBlock* reference,IsleActorFactorySpawn* spawn) {
    return func_802C10C8_643578(object_id,actor_id,block,reference,spawn,NULL);
}
IsleGameObject* isle_cave_actor_factory_func_802C13B4_643864(int32_t object_id,uint16_t actor_id,IsleActorFactoryBlock* block,IsleActorFactoryBlock* reference,IsleActorFactorySpawn* spawn) {
    return func_802C13B4_643864(object_id,actor_id,block,reference,spawn,NULL);
}
IsleGameObject* isle_cave_actor_factory_func_802C13EC_64389C(int32_t object_id,uint16_t actor_id,IsleActorFactoryBlock* block,IsleActorFactoryBlock* reference,IsleActorFactorySpawn* spawn) {
    return func_802C13EC_64389C(object_id,actor_id,block,reference,spawn,NULL);
}
IsleGameObject* isle_cave_actor_factory_func_802C2994_644E44(int32_t object_id,uint16_t actor_id,IsleActorFactoryBlock* block,IsleActorFactoryBlock* reference,IsleActorFactorySpawn* spawn) {
    return func_802C2994_644E44(object_id,actor_id,block,reference,spawn,NULL);
}
IsleGameObject* isle_cave_actor_factory_func_802C3250_645700(int32_t object_id,uint16_t actor_id,IsleActorFactoryBlock* block,IsleActorFactoryBlock* reference,IsleActorFactorySpawn* spawn) {
    return func_802C3250_645700(object_id,actor_id,block,reference,spawn,NULL);
}
IsleGameObject* isle_cave_actor_factory_func_802C3388_645838(int32_t object_id,uint16_t actor_id,IsleActorFactoryBlock* block,IsleActorFactoryBlock* reference,IsleActorFactorySpawn* spawn) {
    return func_802C3388_645838(object_id,actor_id,block,reference,spawn,NULL);
}
IsleGameObject* isle_cave_actor_factory_func_802C465C_646B0C(int32_t object_id,uint16_t actor_id,IsleActorFactoryBlock* block,IsleActorFactoryBlock* reference,IsleActorFactorySpawn* spawn) {
    return func_802C465C_646B0C(object_id,actor_id,block,reference,spawn,NULL);
}
IsleGameObject* isle_cave_actor_factory_func_802C5548_6479F8(int32_t object_id,uint16_t actor_id,IsleActorFactoryBlock* block,IsleActorFactoryBlock* reference,IsleActorFactorySpawn* spawn) {
    return func_802C5548_6479F8(object_id,actor_id,block,reference,spawn,NULL);
}
IsleGameObject* isle_cave_actor_factory_func_802C5B28_647FD8(int32_t object_id,uint16_t actor_id,IsleActorFactoryBlock* block,IsleActorFactoryBlock* reference,IsleActorFactorySpawn* spawn) {
    return func_802C5B28_647FD8(object_id,actor_id,block,reference,spawn,NULL);
}
