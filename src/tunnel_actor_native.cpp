#include "course_actor_native_services.hpp"
#include "actor_native_context.hpp"
#include "actor_interaction_services.hpp"
#include "course_native_host.hpp"
#include "tunnel_actor.hpp"
#include "tunnel_actor_native_state.hpp"
#include "tunnel_actor_native_fences.h"
#include "tunnel_lifecycle_source.h"
#include "player_cart.hpp"
#include "player_cart_resources.hpp"
#include "player_camera_animation.hpp"
#include "player_ui_source.h"
#include "item_owner.hpp"
#include "scene_services.hpp"
#include "game_audio.hpp"
#include "game_state.hpp"
#include "menu_scene_bridge.h"
#include "source_effect_world.hpp"
#include <array>
#include <cmath>
#include <iterator>

extern "C" void hal_rotate_rpy_translate_scale_f(float[4][4],float,float,float,float,float,float,float,float,float);
namespace isle::course {
namespace {
using S=IsleTunnelActorSource;
using H=NativeHost;
constexpr auto invalid=ISLE_GAME_INVALID_ARGUMENT;
constexpr auto state_error=ISLE_GAME_INVALID_STATE;
IsleGameResult audio_result(audio::Error r) noexcept { return r==audio::Error::none ? ISLE_GAME_OK : ISLE_GAME_EXECUTION_FAILURE; }
template<class F> IsleGameResult invoke(S* s,F function) noexcept {
    H* h=nullptr;
    const auto r=actor::native_host(s,&h);
    if (r!=ISLE_GAME_OK) return r;
    try { return function(*h); } catch (...) { return ISLE_GAME_EXECUTION_FAILURE; }
}
IsleTunnelLifecycleSource& lifecycle(H& h) { return h.actors()->course_context().tunnel().source; }
IslePlayerSource& player(H& h) noexcept { return h.root()->source(); }
IsleGameResult camera_clip(H& h,uint32_t identity,IslePlayerCameraAnimationClip& storage,const IslePlayerCameraAnimationClip** out) {
    *out=nullptr;
    if (!identity) return ISLE_GAME_OK;
    if (!h.camera()) return state_error;
    const auto index=h.cart_resources().camera_animation(identity);
    if (index==menu::no_index) return invalid;
    if (!h.camera()->clip(index,storage)) return state_error;
    *out=&storage; return ISLE_GAME_OK;
}
IsleGameResult actor_source(H& h,IsleGameObject* object,IsleActorSource** out) noexcept {
    return isle_actor_source_get(h.game(),object,out);
}
IsleGameModel* native_model(H& h,IsleGameObject* object) noexcept {
    if (object==player(h).object) return player(h).model;
    for (const auto& binding:h.actors()->render_bindings())
        if (binding.model.object==object) return binding.model.model;
    if (auto* model=h.items_owner()->native_model(h.items_owner()->instance(object))) return model;
    return h.cart()->native_model(h.cart()->instance(object));
}
IsleGameResult model_reference(H& h,IsleGameObject* object,IsleGameModelRef* out) noexcept {
    if (!out) return invalid;
    *out={};
    auto* model=native_model(h,object);
    if (!model) return invalid;
    return isle_game_model_reference(h.game(),object,model,out);
}
IsleGameResult pose(H& h,const IsleGameModelRef* ref,const IsleGameModelTransform** out) noexcept {
    if (!ref || !out) return invalid;
    *out=nullptr;
    IsleGameModel* model=nullptr;
    auto r=isle_game_model_resolve(h.game(),ref,&model);
    if (r!=ISLE_GAME_OK) return r;
    // Player is the canonical Root's actual animated model, not Actor AUX or a
    // copied PlayerPos. All Actor PokemonTransforms are independent placement.
    if (ref->object!=player(h).object) {
        const IsleGameModelPlacement* placement=nullptr;
        r=isle_game_model_placement(h.game(),model,&placement);
        if (r!=ISLE_GAME_OK) return r;
        if (placement->mode!=ISLE_GAME_MODEL_PLACEMENT_DISABLED) {
            *out=&placement->transform; return ISLE_GAME_OK;
        }
    }
    const IsleGameModelNode* nodes=nullptr; std::size_t count=0;
    r=isle_game_model_peek_nodes(h.game(),model,&nodes,&count);
    if (r!=ISLE_GAME_OK) return r;
    if (!count || nodes[0].parent!=UINT32_MAX) return state_error;
    *out=&nodes[0].transform; return ISLE_GAME_OK;
}
IsleGameResult node_read(H& h,const IsleActorNodeRef* ref,const IsleGameModelNode** nodes,std::size_t* count) noexcept {
    if (!ref || !nodes || !count) return invalid;
    IsleGameModel* model=nullptr;
    auto r=isle_game_model_resolve(h.game(),&ref->model,&model);
    if (r==ISLE_GAME_OK) r=isle_game_model_peek_nodes(h.game(),model,nodes,count);
    if (r==ISLE_GAME_OK && ref->node>=*count) return invalid;
    return r;
}
const menu::AnimationPath* path(H& h,const void* value) noexcept {
    for (const auto& p:h.world_resources().paths()) if (&p==value) return &p;
    return nullptr;
}
constexpr std::uint32_t header_ids[]={
    0x802ee20c,0x802ee220,0x802ee234,0x802ee248,0x802ee25c,0x802ee270,0x802ee6a4,0x802ee7ec,0x802ee800,0x802ee814,
    0x802ee828,0x802ee994,0x802ee9a8,0x802ee9bc,0x802ee9d0,0x802ee9e4,0x802eeac4,0x802eead8,0x802eeaec,0x802eeb00,
    0x802eeb14,0x802eebf4,0x802eec08,0x802eedbc,0x802eedd0,0x802eede4,0x802eee0c,0x802eee20,0x802eee34,0x802eee5c,
    0x802eee70,0x802eeeac,0x802ef338,0x802ef34c,0x802ef360,0x802ef374,0x802ef388,0x802ef39c,0x802ef3b0,0x802ef3c4,
    0x802ef3d8,0x802ef750,0x802ef764,0x802efa18,0x802efa2c,0x802efa40,0x802efaa0,0x802efab4,0x802efb54,0x802efb68,
    0x802efb7c,0x802efb90,0x802efc48,0x802efd58,0x802efe0c,0x802efe20,0x802efebc,0x802efed0,0x802efee4,0x802efef8,
    0x802effd4,0x802effe8,0x802f0074,0x802f0088,0x802f0110,0x802f0124,0x802f0138,0x802f014c,0x802f01f0,0x80388310};
constexpr auto headers=[] {
    std::array<IsleTunnelAnimationHeader,std::size(header_ids)> result{};
    for (std::size_t i=0;i<result.size();++i) result[i].identity=header_ids[i];
    return result;
}();
IsleGameResult spawn_result(H& h,const actor::SpawnResult& made,IsleGameObject** out) noexcept {
    if (!made) return h.fail(made.status.error==actor::OwnerError::capacity ? ISLE_GAME_EXHAUSTED : state_error,made.status.detail);
    *out=made.object; return ISLE_GAME_OK;
}
template<IsleTunnelAnimationCallback Callback> void cart_event(void* user,const IslePlayerCartNode* node,int32_t event,float value) noexcept {
    auto* s=static_cast<S*>(user);
    const auto r=invoke(s,[&](H& h) {
        IsleGameResult result=invalid;
        if (node && h.cart()->callback_matches(node->binding.object,cart_event<Callback>,s)) {
            IsleActorNodeRef actual{node->binding,node->node};
            result=isle_actor_native_tunnel_animation_event(s,Callback,&actual,event,value);
        }
        if (result!=ISLE_GAME_OK && player(h).status==ISLE_GAME_OK) player(h).status=result;
        return result;
    });
    // C animation dispatch checks actual Player/Actor status after the returning
    // callback. Never checked-transfer from this C++ receiver.
    if (s && r!=ISLE_GAME_OK && s->base.status==ISLE_GAME_OK) s->base.status=r;
}
IsleGameResult collide(void* user,IsleGameVec3* a,IsleGameVec3* b,IsleGameVec3* c,IsleGameVec3* d,int32_t* hit) noexcept {
    auto* s=static_cast<S*>(user);
    return invoke(s,[&](H& h) {
        auto* native=h.tunnel_actor_state();
        if (!native || s->D_802EF9E4_5ECAB4.native!=&native->native_gate) return state_error;
        return isle_tunnel_native_collision(s,func_802EAE7C_5E7F4C,a,b,c,d,hit);
    });
}
#define LEAF(name,args,...) IsleGameResult name args noexcept { return invoke(s,[&](H& h)->IsleGameResult { __VA_ARGS__ }); }
LEAF(reference_assign,(S* s,IsleGameObject** field,IsleGameObject* value),
    if (!field) return invalid;
    for (const auto& b:h.actors()->render_bindings()) {
        if (!b.model.object) continue;
        IsleActorSource* source=nullptr;
        const auto r=actor_source(h,b.model.object,&source);
        if (r!=ISLE_GAME_OK) return r;
        if (field==&source->apple || field==&source->interactionTarget)
            return isle_actor_owner_reference_assign(source,field,value);
    }
    return h.reference(field,value);)
LEAF(source,(S* s,IsleGameObject* object,IsleActorSource** out),return actor_source(h,object,out);)
LEAF(model,(S* s,IsleGameObject* object,IsleGameModelRef* out),return model_reference(h,object,out);)
LEAF(model_transform,(S* s,const IsleGameModelRef* ref,const IsleGameModelTransform** out),return pose(h,ref,out);)
LEAF(model_store,(S* s,const IsleGameModelRef* ref,uint32_t component,float value),
    if (!ref || !component || (component&(component-1)) || component>ISLE_ACTOR_SZ) return invalid;
    IsleActorSource* source=nullptr; auto r=actor_source(h,ref->object,&source); if (r!=ISLE_GAME_OK) return r;
    IsleGameModelTransform values{};
    switch (component) {
    case ISLE_ACTOR_PX: values.position.x=value; break; case ISLE_ACTOR_PY: values.position.y=value; break; case ISLE_ACTOR_PZ: values.position.z=value; break;
    case ISLE_ACTOR_RX: values.rotation.x=value; break; case ISLE_ACTOR_RY: values.rotation.y=value; break; case ISLE_ACTOR_RZ: values.rotation.z=value; break;
    case ISLE_ACTOR_SX: values.scale.x=value; break; case ISLE_ACTOR_SY: values.scale.y=value; break; case ISLE_ACTOR_SZ: values.scale.z=value; break;
    default: return invalid;
    }
    return isle_actor_owner_aux_store(source,ref,component,&values);)
LEAF(node,(S* s,IsleGameObject* object,const uint16_t* steps,size_t count,IsleActorNodeRef* out,int32_t* present),
    if (!out || !present || (count && !steps)) return invalid;
    *out={}; *present=0;
    IsleGameObjectMetadata metadata{}; auto r=isle_game_object_metadata(h.game(),object,&metadata); if (r!=ISLE_GAME_OK) return r;
    auto* attached=native_model(h,object); if (!attached) return ISLE_GAME_OK;
    IsleGameModelRef ref{}; r=isle_game_model_reference(h.game(),object,attached,&ref); if (r!=ISLE_GAME_OK) return r;
    const IsleGameModelNode* nodes=nullptr; size_t size=0; r=isle_game_model_peek_nodes(h.game(),attached,&nodes,&size); if (r!=ISLE_GAME_OK) return r;
    if (!size) return ISLE_GAME_OK;
    size_t index=0;
    for (size_t i=0;i<count;++i) {
        if (steps[i]>1) return invalid;
        index=steps[i]==0 ? nodes[index].first_child : nodes[index].next_sibling;
        if (index==UINT32_MAX) return ISLE_GAME_OK;
        if (index>=size) return state_error;
    }
    *out={ref,index}; *present=1; return ISLE_GAME_OK;)
LEAF(node_transform,(S* s,const IsleActorNodeRef* ref,IsleGameModelTransform* out),
    if (!out) return invalid; const IsleGameModelNode* nodes=nullptr; size_t count=0;
    auto r=node_read(h,ref,&nodes,&count); if (r==ISLE_GAME_OK) *out=nodes[ref->node].transform; return r;)
LEAF(node_world,(S* s,const IsleActorNodeRef* ref,IsleGameVec3* position,IsleGameVec3* velocity),
    if (!ref || !position || !velocity) return invalid;
    IsleGameModel* model=nullptr; auto r=isle_game_model_resolve(h.game(),&ref->model,&model);
    return r==ISLE_GAME_OK ? menu::model::source_effect_position_velocity(h.game(),model,ref->node,position,velocity) : r;)
LEAF(misc_object_get,(S* s,IsleGameObject* object,uint32_t index,IsleGameObject** out),
    IsleActorSource* source=nullptr; auto r=actor_source(h,object,&source);
    return r==ISLE_GAME_OK ? isle_actor_owner_misc_object_get(source,index,out) : r;)
LEAF(misc_object_set,(S* s,IsleGameObject* object,uint32_t index,IsleGameObject* value),
    IsleActorSource* source=nullptr; auto r=actor_source(h,object,&source);
    return r==ISLE_GAME_OK ? isle_actor_owner_misc_object_set(source,index,value) : r;)
LEAF(current_block,(S* s,void** out),
    if (!out) return invalid; *out=h.world_owner()->current(); return ISLE_GAME_OK;)
LEAF(add_one,(S* s,void* block,void* reference,const IsleTunnelAutomaticSpawn* spawn,const IsleTunnelDefinition* definition,IsleGameObject** out),
    if (!out) return invalid; *out=nullptr;
    auto* a=static_cast<world::Block*>(block); auto* b=static_cast<world::Block*>(reference);
    if (!spawn || !definition || !h.valid_block(a) || !h.valid_block(b)) return invalid;
    return spawn_result(h,h.actors()->add_one_tunnel(*definition,*spawn,a,b),out);)
LEAF(add_at_geo,(S* s,IsleGameObject* object,uint16_t id,const IsleTunnelDefinition* definition,IsleGameObject** out),
    if (!out) return invalid; *out=nullptr; if (!definition) return invalid;
    return spawn_result(h,h.actors()->add_geo_tunnel(object,id,*definition),out);)
LEAF(weighted,(S* s,uint32_t identity,const IsleActorRandomState** out),
    if (!out || !h.actors()->bound_interactions()) return invalid; *out=nullptr;
    const auto& rows=h.actors()->bound_interactions()->native_weighted(identity);
    if (rows.rows.empty()) return state_error; *out=rows.rows.data(); return ISLE_GAME_OK;)
LEAF(header,(S* s,uint32_t identity,const IsleTunnelAnimationHeader** out),
    if (!out) return invalid; *out=nullptr;
    for (const auto& header:headers) if (header.identity==identity)
        for (const auto& admitted:h.actor_resources().headers()) if (admitted.wire.guest_address==identity) { *out=&header; return ISLE_GAME_OK; }
    return invalid;)
LEAF(sound_play,(S* s,int32_t id,int32_t* out),return isle_player_root_native_play_sound(&player(h),id,out);)
LEAF(sound_play_params,(S* s,int32_t id,int32_t volume,int32_t pan,float pitch,int32_t reverb,int32_t* out),
    return isle_tunnel_lifecycle_native_play_sound(&lifecycle(h),id,volume,pan,pitch,reverb,out);)
LEAF(sound_identity,(S* s,int32_t handle,int32_t* out),
    if (!out) return invalid; *out=h.audio().playing_sound(static_cast<unsigned>(handle)); return ISLE_GAME_OK;)
LEAF(sound_stop,(S* s,int32_t handle),return isle_player_root_native_stop_sound(&player(h),handle);)
LEAF(sound_stop_all,(S* s),return isle_player_root_native_stop_all_sounds(&player(h));)
LEAF(sound_volume,(S* s,int32_t handle,int32_t value),return audio_result(h.audio().sound_volume(static_cast<unsigned>(handle),static_cast<uint32_t>(value)));)
LEAF(sound_global_volume,(S* s,int32_t value),return isle_player_root_native_sound_global_volume(&player(h),value);)
LEAF(sound_global_reverb,(S* s,int32_t value),return audio_result(h.audio().global_sound_reverb(static_cast<uint8_t>(value)));)
LEAF(bgm_play,(S* s,int32_t channel,int32_t song),return isle_player_root_native_play_song(&player(h),channel,song);)
LEAF(bgm_stop,(S* s,int32_t channel),return isle_player_root_native_stop_song(&player(h),channel);)
LEAF(bgm_volume,(S* s,int32_t channel,uint32_t volume),return audio_result(h.audio().bgm_volume(static_cast<unsigned>(channel),volume));)
LEAF(bgm_volume_smooth,(S* s,int32_t channel,uint32_t volume,int32_t frames),
    return isle_player_root_native_bgm_volume_smooth(&player(h),channel,static_cast<int32_t>(volume),frames);)
LEAF(bgm_channel_volume,(S* s,int32_t channel,uint8_t track,int32_t volume),return isle_player_root_native_bgm_channel_volume(&player(h),channel,track,volume);)
LEAF(bgm_reverb,(S* s,int32_t channel,int32_t value),return audio_result(h.audio().bgm_extra_reverb(static_cast<unsigned>(channel),static_cast<int8_t>(value)));)
LEAF(remove_flying_items,(S* s),return h.items_owner()->remove_flying() ? ISLE_GAME_OK : ISLE_GAME_EXECUTION_FAILURE;)
LEAF(reset_camera,(S* s),return isle_player_root_invoke_reset_main_camera_settings(&player(h));)
LEAF(reset_messages,(S* s),return player(h).ui ? isle_player_ui_Msg_Reset(player(h).ui) : state_error;)
LEAF(remove_pokemon,(S* s,const uint16_t* keep,size_t count),
    return isle_actor_source_remove_pokemons_call(h.game(),isle_menu_current_object(h.game()),keep,count);)
LEAF(cutscene_start,(S* s,IsleGameObject* object,uint32_t identity,float time),
    IslePlayerCameraAnimationClip storage{}; const IslePlayerCameraAnimationClip* clip=nullptr;
    auto r=camera_clip(h,identity,storage,&clip); if (r!=ISLE_GAME_OK) return r;
    return isle_player_root_invoke_start_cutscene(&player(h),object,clip,time);)
LEAF(resume_processes,(S* s,IsleGameObject* object),return isle_game_pause_object_processes(h.game(),object,0);)
LEAF(pause_processes,(S* s,IsleGameObject* object),return isle_game_pause_object_processes(h.game(),object,1);)
LEAF(main_camera,(S* s,IsleGameCamera** out,IsleGameObject** object),
    if (!out || !object) return invalid; *out=nullptr; *object=nullptr;
    auto r=isle_game_camera_object(h.game(),player(h).camera,object); if (r==ISLE_GAME_OK) *out=player(h).camera; return r;)
LEAF(camera_speed,(S* s,IsleGameCamera* camera,float speed),return isle_tunnel_lifecycle_native_camera_speed(&lifecycle(h),camera,speed);)
LEAF(camera_animation,(S* s,IsleGameCamera* camera,uint32_t identity,float time),
    if (camera!=player(h).camera) return invalid;
    IslePlayerCameraAnimationClip storage{}; const IslePlayerCameraAnimationClip* clip=nullptr;
    auto r=camera_clip(h,identity,storage,&clip);
    return r==ISLE_GAME_OK ? isle_player_root_native_camera_animation_set(&player(h),camera,clip,time) : r;)
LEAF(camera_animation_process,(S* s,IsleGameObject* object,int32_t kind,int32_t priority),
    IsleGameObject* actual=nullptr; auto r=isle_game_camera_object(h.game(),player(h).camera,&actual); if (r!=ISLE_GAME_OK) return r;
    if (object!=actual || kind!=1 || priority!=1) return invalid; IsleGameProcess* process=nullptr;
    return isle_game_add_source_process(h.game(),object,isle_tunnel_lifecycle_camera_animation_update,ISLE_GAME_SYNCHRONOUS,1,&process);)
LEAF(sky_speed,(S* s,float speed),return isle_player_root_native_skybox_animation_speed(&player(h),speed);)
LEAF(player_model_init,(S* s,IsleGameObject** out),
    if (!out) return invalid; *out=nullptr; const auto made=h.cart()->init();
    if (!made) return made.status.error==isle::player::cart::OwnerError::capacity ? ISLE_GAME_OK : ISLE_GAME_EXECUTION_FAILURE;
    *out=made.object; return ISLE_GAME_OK;)
IsleGameResult cart_animation(H& h,uint32_t model,uint32_t material,float time,float speed,bool change) {
    const auto* model_clip=model ? h.cart()->model_animation(model) : nullptr;
    const auto* material_clip=material ? h.cart()->material_animation(material) : nullptr;
    if ((model && !model_clip) || (material && !material_clip)) return invalid;
    const auto result=change ? h.cart()->change_animation(model_clip,material_clip,time) : h.cart()->set_animation(model_clip,material_clip,time,speed);
    return result ? ISLE_GAME_OK : ISLE_GAME_EXECUTION_FAILURE;
}
LEAF(player_model_animation,(S* s,uint32_t model,uint32_t material,float time,float speed),return cart_animation(h,model,material,time,speed,false);)
LEAF(player_model_change_animation,(S* s,uint32_t model,uint32_t material,float time),return cart_animation(h,model,material,time,0,true);)
LEAF(player_model_speed,(S* s,float speed),return h.cart()->set_animation_speed(speed) ? ISLE_GAME_OK : ISLE_GAME_EXECUTION_FAILURE;)
LEAF(model_speed,(S* s,IsleGameObject* object,float speed),
    if (h.cart()->instance(object)) return isle_player_cart_backend_set_model_speed(h.cart()->source(),object,speed);
    for (const auto& binding:h.actors()->render_bindings()) if (binding.model.object==object)
        return h.actors()->animation().set_model_speed(binding.model.state.instance,speed) ? ISLE_GAME_OK : ISLE_GAME_EXECUTION_FAILURE;
    return invalid;)
LEAF(animation_callback,(S* s,IsleGameObject* object,IsleTunnelAnimationCallback callback),
    if (h.cart()->instance(object)) {
        IslePlayerCartCallback native=nullptr;
        if (callback==func_802ECC18_5E9CE8) native=cart_event<func_802ECC18_5E9CE8>;
        else if (callback==func_802ED720_5EA7F0) native=cart_event<func_802ED720_5EA7F0>;
        else if (callback==func_802ED7BC_5EA88C) native=cart_event<func_802ED7BC_5EA88C>;
        else if (callback) return invalid;
        return h.cart()->set_callback(object,native,callback ? s : nullptr) ? ISLE_GAME_OK : ISLE_GAME_EXECUTION_FAILURE;
    }
    const auto r=h.actors()->bind_animation_callback(*s,object,callback); return r ? ISLE_GAME_OK : state_error;)
LEAF(collision_initialize,(S* s,IsleTunnelCollision* collision),
    auto* native=h.tunnel_actor_state();
    if (!native || collision!=&s->D_802EF9E4_5ECAB4 || collision->native || collision->identity!=0x802ef9e4u ||
        collision->tree_identity!=0x802ef908u || collision->actor_id!=1009 || collision->scale!=1) return invalid;
    const auto& resource=native->gate_resource;
    if (resource.id!=collision->actor_id || resource.scale!=collision->scale || resource.wire.guest_address!=collision->identity || resource.tree_wire.guest_address!=collision->tree_identity) return state_error;
    collision->native=&native->native_gate; return ISLE_GAME_OK;)
LEAF(collision_query,(S* s,IsleGameVec3* a,IsleGameVec3* b,IsleGameVec3* c,IsleGameVec3* d,const IsleTunnelCollision* collision,const IsleGameVec3* position,const IsleGameVec3* rotation,int32_t* hit),
    if (!a || !b || !c || !d || !position || !rotation || !hit) return invalid;
    auto* native=h.tunnel_actor_state();
    if (!native || collision!=&s->D_802EF9E4_5ECAB4 || collision->native!=&native->native_gate ||
        collision->native->resource!=&native->gate_resource || collision->native->prepared!=&native->gate) return state_error;
    IsleGameVec3 out_position=*c,out_velocity=*d;
    const bool result=native->gate.collision_model_one(0,*position,*rotation,*a,*b,out_position,out_velocity);
    *c=out_position; *d=out_velocity; *hit=result ? 1009 : -1; return ISLE_GAME_OK;)
LEAF(collision_callback,(S* s,IsleTunnelCollisionCallback callback),
    if (callback!=func_802EAE7C_5E7F4C) return invalid;
    auto* native=h.tunnel_actor_state(); if (!native || s->D_802EF9E4_5ECAB4.native!=&native->native_gate) return state_error;
    return h.bind_item_collision(collide,s);)
LEAF(level_progress,(S* s,int32_t* block,float* fraction),
    if (!block || !fraction) return invalid; const auto& movement=player(h).movement;
    for (const auto& current:h.world_owner()->blocks()) if (&current==movement.block) {
        if (current.index>static_cast<size_t>(INT32_MAX)) return state_error;
        *block=static_cast<int32_t>(current.index); *fraction=movement.move_time; return ISLE_GAME_OK;
    }
    return state_error;)
LEAF(environment_sound,(S* s,IsleGameObject* object,uint8_t category,int32_t sound),
    IsleActorSource* source=nullptr; auto r=actor_source(h,object,&source);
    return r==ISLE_GAME_OK ? isle_actor_owner_environment_sound(source,category,sound) : r;)
LEAF(ground,(S* s,float x,float z,IsleActorGround* out),
    if (!out) return invalid; world::GroundResult ground{}; h.world_owner()->ground_at(x,z,ground);
    *out={ground.height,ground.normal,static_cast<uint32_t>(ground.surface)}; return ISLE_GAME_OK;)
LEAF(background_music,(S* s,int32_t song),return isle_player_root_invoke_set_background_music(&player(h),song);)
LEAF(notice,(S* s,int32_t bits),h.game_state().add_scene_notices(static_cast<uint32_t>(bits)); return ISLE_GAME_OK;)
LEAF(player_model,(S* s,IsleGameModelRef* out),return model_reference(h,player(h).object,out);)
LEAF(player_object,(S* s,IsleGameObject** out),
    if (!out) return invalid; *out=nullptr; IsleGameObjectMetadata metadata{};
    auto r=isle_game_object_metadata(h.game(),player(h).object,&metadata); if (r==ISLE_GAME_OK) *out=player(h).object; return r;)
LEAF(random_range,(S* s,int32_t range,int32_t* out),
    if (!out || !player(h).rng || player(h).rng!=&h.game_state().rng) return invalid;
    *out=isle_rng_range(player(h).rng,range); return ISLE_GAME_OK;)
LEAF(path_count,(S* s,const void* value,int32_t* out),
    auto* p=path(h,value); if (!p || !out || p->segment_point_count>static_cast<size_t>(INT32_MAX)) return invalid;
    *out=static_cast<int32_t>(p->segment_point_count); return ISLE_GAME_OK;)
LEAF(path_point,(S* s,const void* value,int32_t index,IsleGameVec3* out),
    auto* p=path(h,value); if (!p || !out || index<0 || static_cast<size_t>(index)>=p->points.size()) return invalid;
    const auto point=p->points[static_cast<size_t>(index)]; *out={point.x,point.y,point.z}; return ISLE_GAME_OK;)
LEAF(path_param,(S* s,const void* value,int32_t index,float* out),
    auto* p=path(h,value); if (!p || !out || index<0 || static_cast<size_t>(index)>=p->timestamps.size()) return invalid;
    *out=p->timestamps[static_cast<size_t>(index)]; return ISLE_GAME_OK;)
LEAF(block_position,(S* s,const void* value,IsleGameVec3* out),
    auto* block=static_cast<const world::Block*>(value); if (!out || !h.valid_block(block) || !block->resource) return invalid;
    const auto v=block->resource->metadata.world_position; *out={v.x,v.y,v.z}; return ISLE_GAME_OK;)
LEAF(camera_look_read,(S* s,int32_t eye,IsleGameVec3* out),
    if (!out || (eye!=0 && eye!=1)) return invalid; const IsleGameCameraParameters* parameters=nullptr;
    auto r=isle_game_camera_parameters(h.game(),player(h).camera,&parameters); if (r!=ISLE_GAME_OK) return r;
    const auto* v=eye ? parameters->look_at.eye : parameters->look_at.at; *out={v[0],v[1],v[2]}; return ISLE_GAME_OK;)
LEAF(camera_look_store,(S* s,int32_t eye,uint32_t axis,float value),
    if ((eye!=0 && eye!=1) || axis>=3) return invalid; const IsleGameCameraParameters* parameters=nullptr;
    auto r=isle_game_camera_parameters(h.game(),player(h).camera,&parameters); if (r!=ISLE_GAME_OK) return r;
    auto look=parameters->look_at; (eye ? look.eye : look.at)[axis]=value;
    return isle_game_camera_set_look_at(h.game(),player(h).camera,&look);)
LEAF(command,(S* s,IsleGameObject* target,int32_t command,IsleGameObject* sender),return h.services()->send_command(target,command,sender) ? ISLE_GAME_OK : ISLE_GAME_EXECUTION_FAILURE;)
LEAF(command_link,(S* s,int32_t link,int32_t command,IsleGameObject* sender),
    if (link<0 || link>255) return invalid;
    return h.services()->send_command_to_link(static_cast<uint8_t>(link),command,sender) ? ISLE_GAME_OK : ISLE_GAME_EXECUTION_FAILURE;)
LEAF(flute_command,(S* s,int32_t* out),return h.flute_command(out);)
LEAF(vector_distance,(S* s,const IsleGameVec3* a,const IsleGameVec3* b,float* out),
    (void)h; if (!a || !b || !out) return invalid;
    const float x=a->x-b->x,y=a->y-b->y,z=a->z-b->z; *out=std::sqrt(x*x+y*y+z*z); return ISLE_GAME_OK;)
LEAF(matrix_rts,(S* s,float matrix[4][4],float x,float y,float z,float rx,float ry,float rz,float sx,float sy,float sz),
    (void)h; if (!matrix) return invalid;
    hal_rotate_rpy_translate_scale_f(matrix,x,y,z,rx,ry,rz,sx,sy,sz); return ISLE_GAME_OK;)
LEAF(matrix_cat,(S* s,const float a[4][4],const float b[4][4],float out[4][4]),
    (void)h; if (!a || !b || !out) return invalid; float product[4][4];
    for (unsigned i=0;i<4;++i) for (unsigned j=0;j<4;++j)
        product[i][j]=((a[i][0]*b[0][j]+a[i][1]*b[1][j])+a[i][2]*b[2][j])+a[i][3]*b[3][j];
    for (unsigned i=0;i<4;++i) for (unsigned j=0;j<4;++j) out[i][j]=product[i][j]; return ISLE_GAME_OK;)
LEAF(sound_pan_ramp,(S* s,int32_t handle,uint32_t from,uint32_t to,int32_t frames,int32_t* out),return h.services()->ramp_sound_pan(handle,from,to,frames,out);)
LEAF(sound_volume_ramp,(S* s,int32_t handle,uint32_t from,uint32_t to,int32_t frames,int32_t* out),return h.services()->ramp_sound_volume(handle,from,to,frames,out);)
LEAF(sound_ramp_reset,(S* s),return h.services()->reset_sound_ramps();)
LEAF(fade_status,(S* s,int32_t* out),(void)h; if (!out) return invalid; return isle_tunnel_native_transition(s,0,0,0,0,0,out);)
LEAF(fade_color,(S* s,int32_t red,int32_t green,int32_t blue),(void)h; return isle_tunnel_native_transition(s,1,red,green,blue,0,nullptr);)
LEAF(fade_duration,(S* s,int32_t mode,float duration),(void)h; return isle_tunnel_native_transition(s,2,mode,0,0,duration,nullptr);)
LEAF(fade_start,(S* s,int32_t mode,int32_t flag),(void)h; return isle_tunnel_native_transition(s,3,mode,flag,0,0,nullptr);)
LEAF(progress_get,(S* s,int32_t* out),
    if (!out || !h.game_state().save()) return invalid;
    auto snapshot=h.game_state().save()->snapshot(); if (!snapshot) return state_error;
    const auto selected=snapshot.value.selected_course(); if (!selected) return state_error;
    *out=static_cast<int32_t>(selected.value); return ISLE_GAME_OK;)
LEAF(progress_set,(S* s,int32_t value),
    auto* save=h.game_state().save(); return save && !save->set_unlocked_course(value) ? ISLE_GAME_OK : state_error;)
LEAF(helper_create,(S* s,int32_t id,int32_t link,uint32_t priority,IsleGameObject** out),
    if (link<0 || link>255) return invalid;
    return h.create_helper(static_cast<uint32_t>(id),nullptr,static_cast<uint8_t>(link),priority,out);)
LEAF(helper_delete,(S* s,IsleGameObject* object),
    if (!object || object==h.reference_owner()) return invalid; IsleGameObjectMetadata metadata{};
    auto r=isle_game_object_metadata(h.game(),object,&metadata); if (r!=ISLE_GAME_OK) return r;
    return isle_tunnel_native_delete(s,object);)
LEAF(camera_object,(S* s,IsleGameCamera* camera,IsleGameObject** out),return isle_game_camera_object(h.game(),camera,out);)
LEAF(spawn_effect,(S* s,IsleGameObject* object,IsleGameObject** out),
    IsleGameObjectMetadata metadata{}; auto r=isle_game_object_metadata(h.game(),object,&metadata);
    return r==ISLE_GAME_OK ? isle_tunnel_native_spawn_effect(s,object,out) : r;)
LEAF(sky_default_speed,(S* s,float* out),if (!out) return invalid; *out=h.world_resources().setup().sky_animation_speed; return ISLE_GAME_OK;)
LEAF(root_position,(S* s,const IsleGameModelRef* ref,IsleGameVec3* out),
    if (!ref || !out) return invalid; IsleGameModel* model=nullptr;
    auto r=isle_game_model_resolve(h.game(),ref,&model); if (r!=ISLE_GAME_OK) return r;
    const IsleGameModelNode* nodes=nullptr; size_t count=0; r=isle_game_model_peek_nodes(h.game(),model,&nodes,&count);
    if (r!=ISLE_GAME_OK) return r; if (!count || nodes[0].parent!=UINT32_MAX) return state_error;
    *out=nodes[0].transform.position; return ISLE_GAME_OK;)
#undef LEAF
} // namespace
IsleTunnelActorServices make_tunnel_actor_services() noexcept {
    return {reference_assign,source,model,model_transform,model_store,node,node_transform,node_world,
        misc_object_get,misc_object_set,current_block,add_one,add_at_geo,weighted,header,
        sound_play,sound_play_params,sound_identity,sound_stop,sound_stop_all,sound_volume,sound_global_volume,sound_global_reverb,
        bgm_play,bgm_stop,bgm_volume,bgm_volume_smooth,bgm_channel_volume,bgm_reverb,
        remove_flying_items,reset_camera,reset_messages,remove_pokemon,cutscene_start,resume_processes,pause_processes,
        main_camera,camera_speed,camera_animation,camera_animation_process,sky_speed,player_model_init,
        player_model_animation,player_model_change_animation,player_model_speed,model_speed,animation_callback,
        collision_initialize,collision_query,collision_callback,level_progress,environment_sound,ground,background_music,
        notice,player_model,player_object,random_range,path_count,path_point,path_param,block_position,
        camera_look_read,camera_look_store,command,command_link,flute_command,vector_distance,matrix_rts,matrix_cat,
        sound_pan_ramp,sound_volume_ramp,sound_ramp_reset,fade_status,fade_color,fade_duration,fade_start,
        progress_get,progress_set,helper_create,helper_delete,camera_object,spawn_effect,sky_default_speed,root_position};
}
} // namespace isle::course
