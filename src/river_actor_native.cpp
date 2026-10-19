#include "course_actor_native_services.hpp"
#include "actor_native_context.hpp"
#include "actor_interaction_services.hpp"
#include "actor_spatial.h"
#include "course_native_host.hpp"
#include "course_scene_source.h"
#include "river_actor.hpp"
#include "river_actor_native_source.h"
#include "scene_services.hpp"
#include "player_cart.hpp"
#include "player_cart_resources.hpp"
#include "player_camera_animation.hpp"
#include "player_root_source.h"
#include "player_ui_source.h"
#include "item_owner.hpp"
#include "screen_transition_source.h"
#include <cmath>
#include <limits>
#include <type_traits>

namespace isle::course {
namespace {
using Source = IsleRiverActorSource;
struct Context {
    Source& source;
    NativeHost& host;
    actor::Owner& owner() const noexcept { return *host.actors(); }
    IslePlayerSource& player() const noexcept { return owner().player_source(); }
    scene::Services& services() const noexcept { return *host.services(); }
    bool receiver(IsleGameObject* object) const noexcept {
        IsleGameObjectMetadata metadata{};
        return object && services().source_owner_registered(object) &&
            !services().source_owner_joined(object) &&
            isle_game_object_metadata(host.game(),object,&metadata)==ISLE_GAME_OK;
    }
    bool admitted_actor(IsleActorSource* source) const {
        auto* interactions=owner().bound_interactions();
        if (!source || !interactions) return false;
        menu::animation::InstanceHandle instance; IsleGameModel* model=nullptr;
        if (!actor_model(source->object,instance,model) || owner().source(instance)!=source) return false;
        for (const auto& initialization:host.actor_resources().initializations())
            if (&initialization==source->initData)
                return interactions->admits_initial_species(initialization,source->path);
        return false;
    }
    bool actor_model(IsleGameObject* object,menu::animation::InstanceHandle& instance,
               IsleGameModel*& model) const noexcept {
        instance=owner().instance(object);
        model=instance ? owner().native_model(instance) : nullptr;
        return instance && model &&
            isle_game_model_validate_binding(host.game(),object,model)==ISLE_GAME_OK;
    }
    IsleGameResult model(IsleGameObject* object,IsleGameModelRef* output) const noexcept {
        if (!output) return ISLE_GAME_INVALID_ARGUMENT;
        *output={};
        if (!receiver(object)) return ISLE_GAME_INVALID_ARGUMENT;
        IsleGameModel* value=nullptr;
        const auto result=isle_actor_spatial_object_model(host.game(),object,&value);
        if (result!=ISLE_GAME_OK) return result;
        return value ? isle_game_model_reference(host.game(),object,value,output) : ISLE_GAME_INVALID_STATE;
    }
    IsleGameResult resolve(const IsleGameModelRef& reference,IsleGameModel** output) const noexcept {
        if (!output) return ISLE_GAME_INVALID_ARGUMENT;
        *output=nullptr;
        if (!receiver(reference.object)) return ISLE_GAME_INVALID_ARGUMENT;
        return isle_game_model_resolve(host.game(),&reference,output);
    }
    const menu::AnimationPath* path(const void* value) const noexcept {
        if (!value || !host.world_owner() || host.world_owner()->retired() || !host.world_owner()->error().empty()) return nullptr;
        for (const auto& candidate:host.world_owner()->resources().paths) if (&candidate==value) return &candidate;
        return nullptr;
    }
    IsleGameResult camera(IsleGameObject* object) const noexcept {
        IsleGameObject* actual=nullptr;
        if (!player().camera || !host.camera() || !host.camera()->matches(host.game(),player().camera)) return ISLE_GAME_INVALID_STATE;
        const auto result=isle_game_camera_object(host.game(),player().camera,&actual);
        if (result!=ISLE_GAME_OK) return result;
        return actual==object && receiver(object) ? ISLE_GAME_OK : ISLE_GAME_INVALID_ARGUMENT;
    }
};
template<class Function> IsleGameResult call(Source* source,Function function) noexcept {
    try {
        NativeHost* host=nullptr;
        const auto result=actor::native_host(source,&host);
        if (result!=ISLE_GAME_OK) return result;
        if (!host || !host->actors() || !host->services() || !host->items_owner() || !host->cart()) return ISLE_GAME_INVALID_STATE;
        Context context{*source,*host};
        return function(context);
    } catch (...) { return ISLE_GAME_INVALID_STATE; }
}
IsleGameResult audio(Context& c,isle::audio::Error result) noexcept {
    return c.services().check(result,"River Original audio command failed") ? ISLE_GAME_OK : ISLE_GAME_EXECUTION_FAILURE;
}
IsleGameResult owner(actor::OwnerStatus result) noexcept {
    if (result) return ISLE_GAME_OK;
    return result.error==actor::OwnerError::admission ? ISLE_GAME_INVALID_ARGUMENT : ISLE_GAME_EXECUTION_FAILURE;
}
float* component(IsleGameModelTransform& value,std::uint32_t index) noexcept {
    switch (index) {
    case 0:return &value.position.x; case 1:return &value.position.y; case 2:return &value.position.z;
    case 3:return &value.rotation.x; case 4:return &value.rotation.y; case 5:return &value.rotation.z;
    case 6:return &value.scale.x; case 7:return &value.scale.y; case 8:return &value.scale.z;
    default:return nullptr;
    }
}
IsleGameResult placement(Context& c,IsleGameModelRef reference,const IsleGameModelPlacement** output) noexcept {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    *output=nullptr; IsleGameModel* model=nullptr;
    auto result=c.resolve(reference,&model);
    if (result!=ISLE_GAME_OK) return result;
    result=isle_game_model_placement(c.host.game(),model,output);
    if (result!=ISLE_GAME_OK) return result;
    if (!*output || (*output)->mode==ISLE_GAME_MODEL_PLACEMENT_DISABLED) { *output=nullptr; return ISLE_GAME_INVALID_ARGUMENT; }
    return ISLE_GAME_OK;
}
IsleGameResult root(Context& c,IsleGameModelRef reference,const IsleGameModelNode** output) noexcept {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    *output=nullptr; IsleGameModel* model=nullptr;
    auto result=c.resolve(reference,&model); if (result!=ISLE_GAME_OK) return result;
    std::size_t count=0;
    result=isle_game_model_peek_nodes(c.host.game(),model,output,&count);
    if (result!=ISLE_GAME_OK) return result;
    if (!count || !*output || (*output)[0].parent!=UINT32_MAX) { *output=nullptr; return ISLE_GAME_INVALID_STATE; }
    return ISLE_GAME_OK;
}
IsleGameResult process(Context& c,IsleGameObject* object,IsleGameSourceCallback callback,
                       std::int32_t kind,std::int32_t priority,IsleGameProcess** output) noexcept {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    *output=nullptr;
    if (!c.receiver(object) || !callback || (kind!=0 && kind!=1) || priority<0) return ISLE_GAME_INVALID_ARGUMENT;
    return isle_game_add_source_process(c.host.game(),object,callback,
        kind==0 ? ISLE_GAME_STACKFUL : ISLE_GAME_SYNCHRONOUS,std::uint32_t(priority),output);
}
}
IsleRiverActorServices make_river_actor_services() noexcept {
    static_assert(std::is_same_v<menu::Vec3,IsleGameVec3>);
    static_assert(std::is_same_v<IsleRiverItemView,IsleItemSource>);
    static_assert(std::is_same_v<IsleRiverCameraView,IsleGameLookAt>);
    return {
        .reference_assign=+[](Source* s,IsleGameObject** field,IsleGameObject* value) noexcept { return call(s,[&](Context& c) { return c.host.reference(field,value); }); },
        .model=+[](Source* s,IsleGameObject* object,IsleGameModelRef* out) noexcept { return call(s,[&](Context& c) { return c.model(object,out); }); },
        .model_read=+[](Source* s,IsleGameModelRef ref,const IsleGameModelTransform** out) noexcept { if (out) *out=nullptr; return call(s,[&](Context& c) { if (!out) return ISLE_GAME_INVALID_ARGUMENT; const IsleGameModelPlacement* value=nullptr; auto r=placement(c,ref,&value); if (r==ISLE_GAME_OK) *out=&value->transform; return r; }); },
        .model_store=+[](Source* s,IsleGameModelRef ref,std::uint32_t index,float value) noexcept { return call(s,[&](Context& c) { const IsleGameModelPlacement* p=nullptr; auto r=placement(c,ref,&p); if (r!=ISLE_GAME_OK) return r; if (index>8 || (index>=6 && p->mode!=ISLE_GAME_MODEL_PLACEMENT_RTS)) return ISLE_GAME_INVALID_ARGUMENT; auto next=p->transform; *component(next,index)=value; return isle_game_model_set_placement(c.host.game(),ref.model,p->mode,&next); }); },
        .root_position=+[](Source* s,IsleGameModelRef ref,IsleGameVec3* out) noexcept { return call(s,[&](Context& c) { if (!out) return ISLE_GAME_INVALID_ARGUMENT; const IsleGameModelNode* nodes=nullptr; auto r=root(c,ref,&nodes); if (r==ISLE_GAME_OK) *out=nodes[0].transform.position; return r; }); },
        .root_store=+[](Source* s,IsleGameModelRef ref,std::uint32_t index,float value) noexcept { return call(s,[&](Context& c) { if (index>2) return ISLE_GAME_INVALID_ARGUMENT; const IsleGameModelNode* nodes=nullptr; auto r=root(c,ref,&nodes); if (r!=ISLE_GAME_OK) return r; auto next=nodes[0].transform; *component(next,index)=value; menu::animation::InstanceHandle h; IsleGameModel* m=nullptr; if (c.actor_model(ref.object,h,m)) return owner(c.owner().set_local(h,0,{{next.position.x,next.position.y,next.position.z},{next.rotation.x,next.rotation.y,next.rotation.z},{next.scale.x,next.scale.y,next.scale.z}})); return isle_game_model_set_transform(c.host.game(),ref.model,0,&next); }); },
        .node_descend=+[](Source* s,IsleGameModelRef ref,const std::uint8_t* steps,std::size_t size,IsleActorNodeRef* out) noexcept { if (out) *out={}; return call(s,[&](Context& c) { if (!out || (size && !steps)) return ISLE_GAME_INVALID_ARGUMENT; IsleGameModel* m=nullptr; auto r=c.resolve(ref,&m); if (r!=ISLE_GAME_OK) return r; const IsleGameModelNode* nodes=nullptr; std::size_t count=0; r=isle_game_model_peek_nodes(c.host.game(),m,&nodes,&count); if (r!=ISLE_GAME_OK) return r; if (!count) return ISLE_GAME_INVALID_STATE; std::size_t node=0; for (std::size_t i=0;i<size;++i) { if (steps[i]>1) return ISLE_GAME_INVALID_ARGUMENT; const auto next=steps[i] ? nodes[node].next_sibling : nodes[node].first_child; if (next==UINT32_MAX || next>=count) return ISLE_GAME_INVALID_ARGUMENT; node=next; } *out={ref,node}; return ISLE_GAME_OK; }); },
        .node_world=+[](Source* s,IsleActorNodeRef node,IsleGameVec3* position,IsleGameVec3* velocity) noexcept { return call(s,[&](Context& c) { IsleGameModel* m=nullptr; auto r=c.resolve(node.model,&m); if (r!=ISLE_GAME_OK) return r; IsleActorSource* actor=nullptr; r=isle_actor_source_get(c.host.game(),node.model.object,&actor); return r==ISLE_GAME_OK ? isle_actor_owner_node_world_ref(actor,&node,position,velocity) : r; }); },
        .current_block=+[](Source* s,IsleRiverBlock** out) noexcept { if (out) *out=nullptr; return call(s,[&](Context& c) { if (!out || !c.host.world_owner() || c.host.world_owner()->retired()) return ISLE_GAME_INVALID_STATE; auto* b=c.host.world_owner()->current(); if (b && !c.host.valid_block(b)) return ISLE_GAME_INVALID_STATE; *out=reinterpret_cast<IsleRiverBlock*>(b); return ISLE_GAME_OK; }); },
        .block_position=+[](Source* s,IsleRiverBlock* block,IsleGameVec3* out) noexcept { return call(s,[&](Context& c) { auto* b=reinterpret_cast<world::Block*>(block); if (!out || !c.host.valid_block(b) || !b->resource) return ISLE_GAME_INVALID_ARGUMENT; const auto p=b->resource->metadata.world_position; *out={p.x,p.y,p.z}; return ISLE_GAME_OK; }); },
        .path_count=+[](Source* s,const void* path,std::int32_t* out) noexcept { return call(s,[&](Context& c) { const auto* p=c.path(path); if (!out || !p || p->segment_point_count>std::size_t(INT32_MAX)) return ISLE_GAME_INVALID_ARGUMENT; *out=std::int32_t(p->segment_point_count); return ISLE_GAME_OK; }); },
        .path_duration=+[](Source* s,const void* path,float* out) noexcept { return call(s,[&](Context& c) { const auto* p=c.path(path); if (!out || !p) return ISLE_GAME_INVALID_ARGUMENT; *out=p->duration; return ISLE_GAME_OK; }); },
        .path_parameter=+[](Source* s,const void* path,std::int32_t index,float* out) noexcept { return call(s,[&](Context& c) { const auto* p=c.path(path); if (!out || !p || index<0 || std::size_t(index)>=p->timestamps.size()) return ISLE_GAME_INVALID_ARGUMENT; *out=p->timestamps[std::size_t(index)]; return ISLE_GAME_OK; }); },
        .path_point=+[](Source* s,const void* path,std::int32_t index,const IsleGameVec3** out) noexcept { if (out) *out=nullptr; return call(s,[&](Context& c) { const auto* p=c.path(path); if (!out || !p || index<0 || std::size_t(index)>=p->points.size()) return ISLE_GAME_INVALID_ARGUMENT; *out=&p->points[std::size_t(index)]; return ISLE_GAME_OK; }); },
        .item=+[](Source* s,IsleGameObject* object,const IsleRiverItemView** out) noexcept { if (out) *out=nullptr; return call(s,[&](Context& c) { if (!out || !c.receiver(object)) return ISLE_GAME_INVALID_ARGUMENT; const auto h=c.host.items_owner()->instance(object); const auto* item=c.host.items_owner()->source(h); if (!h || !item || item->object!=object || item->game!=c.host.game()) return ISLE_GAME_INVALID_ARGUMENT; IsleItemSource* canonical=nullptr; auto r=isle_item_source_get(c.host.game(),object,&canonical); if (r==ISLE_GAME_OK && canonical!=item) return ISLE_GAME_INVALID_STATE; if (r==ISLE_GAME_OK) *out=canonical; return r; }); },
        .player_object=+[](Source* s,IsleGameObject** out) noexcept { if (out) *out=nullptr; return call(s,[&](Context& c) { if (!out || !c.receiver(c.player().object)) return ISLE_GAME_INVALID_STATE; *out=c.player().object; return ISLE_GAME_OK; }); },
        .player_model=+[](Source* s,IsleGameModelRef* out) noexcept { if (out) *out={}; return call(s,[&](Context& c) { return c.model(c.player().object,out); }); },
        .rng_range=+[](Source* s,std::int32_t range,std::int32_t* out) noexcept { return call(s,[&](Context& c) { if (!out || !c.player().rng) return ISLE_GAME_INVALID_ARGUMENT; *out=isle_rng_range(c.player().rng,range); return ISLE_GAME_OK; }); },
        .weighted=+[](Source* s,IsleGameObject* receiver,std::uint32_t identity,const IsleActorRandomState** out) noexcept { if (out) *out=nullptr; return call(s,[&](Context& c) { if (!out) return ISLE_GAME_INVALID_ARGUMENT; IsleActorSource* a=nullptr; auto r=isle_actor_source_get(c.host.game(),receiver,&a); if (r!=ISLE_GAME_OK) return r; if (!c.admitted_actor(a)) return ISLE_GAME_INVALID_STATE; *out=c.owner().bound_interactions()->native_weighted(identity).rows.data(); return ISLE_GAME_OK; }); },
        .counted_options=+[](Source* s,std::uint32_t identity,std::size_t* count,const IsleRiverCountedOption** out) noexcept { if (count) *count=0; if (out) *out=nullptr; return call(s,[&](Context& c) { if (!count || !out || !c.owner().bound_interactions()) return ISLE_GAME_INVALID_ARGUMENT; const auto rows=c.owner().bound_interactions()->river_counted(identity); if (rows.size()!=3) return ISLE_GAME_INVALID_STATE; *count=rows.size(); *out=rows.data(); return ISLE_GAME_OK; }); },
        .graph=+[](Source* s,IsleGameObject* receiver,std::uint32_t identity,const IsleActorInteractionGraph** out) noexcept { if (out) *out=nullptr; return call(s,[&](Context& c) { if (!out) return ISLE_GAME_INVALID_ARGUMENT; IsleActorSource* a=nullptr; auto r=isle_actor_source_get(c.host.game(),receiver,&a); if (r!=ISLE_GAME_OK) return r; if (!c.admitted_actor(a)) return ISLE_GAME_INVALID_STATE; *out=&c.owner().bound_interactions()->native_graph(identity); return ISLE_GAME_OK; }); },
        .add_one=+[](Source* s,IsleRiverBlock* first,IsleRiverBlock* second,const IsleRiverAutomaticSpawn* spawn,const IsleRiverDefinition* definition,IsleGameObject** out) noexcept { if (out) *out=nullptr; return call(s,[&](Context& c) { auto* a=reinterpret_cast<world::Block*>(first); auto* b=reinterpret_cast<world::Block*>(second); if (!out || !spawn || !definition || !c.host.valid_block(a) || !c.host.valid_block(b)) return ISLE_GAME_INVALID_ARGUMENT; const auto made=c.owner().add_one_river(*definition,*spawn,a,b); *out=made.object; return owner(made.status); }); },
        .ground=+[](Source* s,float x,float z,IsleActorGround* out) noexcept { return call(s,[&](Context& c) { if (!out || !c.host.world_owner()) return ISLE_GAME_INVALID_ARGUMENT; world::GroundResult result; c.host.world_owner()->ground_at(x,z,result); *out={result.height,result.normal,std::uint32_t(result.surface)}; return ISLE_GAME_OK; }); },
        .sound=+[](Source* s,std::int32_t sound,std::int32_t* out) noexcept { return call(s,[&](Context& c) { return isle_player_root_native_play_sound(&c.player(),sound,out); }); },
        .sound_params=+[](Source* s,std::int32_t sound,std::int32_t volume,std::int32_t pan,float pitch,std::int32_t reverb,std::int32_t* out) noexcept { if (out) *out=-1; return call(s,[&](Context& c) { if (!out) return ISLE_GAME_INVALID_ARGUMENT; int slot=-1; auto r=audio(c,c.host.audio().play_sound(std::uint32_t(sound),{std::uint16_t(volume),std::uint8_t(pan),std::uint8_t(reverb),pitch},slot)); *out=slot; return r; }); },
        .sound_active=+[](Source* s,std::int32_t handle,std::int32_t* out) noexcept { return call(s,[&](Context& c) { if (!out) return ISLE_GAME_INVALID_ARGUMENT; *out=c.host.audio().playing_sound(std::uint32_t(handle)); return ISLE_GAME_OK; }); },
        .sound_stop=+[](Source* s,std::int32_t handle) noexcept { return call(s,[&](Context& c) { return isle_player_root_native_stop_sound(&c.player(),handle); }); },
        .bgm_play=+[](Source* s,std::int32_t player,std::int32_t song) noexcept { return call(s,[&](Context& c) { return isle_player_root_native_play_song(&c.player(),player,song); }); },
        .bgm_volume=+[](Source* s,std::int32_t player,std::uint32_t volume) noexcept { return call(s,[&](Context& c) { return audio(c,c.host.audio().bgm_volume(std::uint32_t(player),volume)); }); },
        .bgm_volume_smooth=+[](Source* s,std::int32_t player,std::int32_t volume,std::int32_t frames) noexcept { return call(s,[&](Context& c) { return isle_player_root_native_bgm_volume_smooth(&c.player(),player,volume,frames); }); },
        .sound_volume_smooth=+[](Source* s,std::int32_t handle,std::int32_t from,std::int32_t to,std::int32_t frames) noexcept { return call(s,[&](Context& c) { std::int32_t result=-1; return c.services().ramp_sound_volume(handle,std::uint32_t(from),std::uint32_t(to),frames,&result); }); },
        .sound_global_volume=+[](Source* s,std::int32_t volume) noexcept { return call(s,[&](Context& c) { return isle_player_root_native_sound_global_volume(&c.player(),volume); }); },
        .animation_speed=+[](Source* s,IsleGameObject* object,float speed,std::int32_t materials) noexcept { return call(s,[&](Context& c) { menu::animation::InstanceHandle h; IsleGameModel* m=nullptr; if (!c.actor_model(object,h,m) || (materials!=0 && materials!=1)) return ISLE_GAME_INVALID_ARGUMENT; if (!c.owner().animation().set_model_speed(h,speed)) return ISLE_GAME_EXECUTION_FAILURE; return !materials || c.owner().animation().set_texture_speed(h,speed) ? ISLE_GAME_OK : ISLE_GAME_EXECUTION_FAILURE; }); },
        .model_speed=+[](Source* s,IsleGameModelRef ref,float* out) noexcept { return call(s,[&](Context& c) { if (!out) return ISLE_GAME_INVALID_ARGUMENT; IsleGameModel* actual=nullptr; auto r=c.resolve(ref,&actual); if (r!=ISLE_GAME_OK) return r; menu::animation::InstanceHandle h; IsleGameModel* m=nullptr; if (!c.actor_model(ref.object,h,m) || m!=actual) return ISLE_GAME_INVALID_ARGUMENT; const auto state=c.owner().animation().model(h); if (!state || state.nodes.empty()) return ISLE_GAME_INVALID_STATE; *out=state.nodes[0].clock.speed; return ISLE_GAME_OK; }); },
        .camera=+[](Source* s,IsleGameObject** out) noexcept { if (out) *out=nullptr; return call(s,[&](Context& c) { if (!out || !c.player().camera) return ISLE_GAME_INVALID_ARGUMENT; auto r=isle_game_camera_object(c.host.game(),c.player().camera,out); return r==ISLE_GAME_OK ? c.camera(*out) : r; }); },
        .camera_read=+[](Source* s,IsleGameObject* object,const IsleRiverCameraView** out) noexcept { if (out) *out=nullptr; return call(s,[&](Context& c) { if (!out) return ISLE_GAME_INVALID_ARGUMENT; auto r=c.camera(object); if (r!=ISLE_GAME_OK) return r; const IsleGameCameraParameters* p=nullptr; r=isle_game_camera_parameters(c.host.game(),c.player().camera,&p); if (r==ISLE_GAME_OK) *out=&p->look_at; return r; }); },
        .camera_store=+[](Source* s,IsleGameObject* object,std::uint32_t index,float value) noexcept { return call(s,[&](Context& c) { auto r=c.camera(object); if (r!=ISLE_GAME_OK) return r; if (index>5) return ISLE_GAME_INVALID_ARGUMENT; const IsleGameCameraParameters* p=nullptr; r=isle_game_camera_parameters(c.host.game(),c.player().camera,&p); if (r!=ISLE_GAME_OK) return r; auto next=p->look_at; if (index<3) next.eye[index]=value; else next.at[index-3]=value; return isle_game_camera_set_look_at(c.host.game(),c.player().camera,&next); }); },
        .camera_speed=+[](Source* s,IsleGameObject* object,float speed) noexcept { return call(s,[&](Context& c) { auto r=c.camera(object); return r!=ISLE_GAME_OK ? r : c.host.camera()->set_speed(speed) ? ISLE_GAME_OK : ISLE_GAME_EXECUTION_FAILURE; }); },
        .camera_animation=+[](Source* s,IsleGameObject* object,std::uint32_t identity,float start) noexcept { return call(s,[&](Context& c) { auto r=c.camera(object); if (r!=ISLE_GAME_OK) return r; if (!identity) return isle_player_root_native_camera_animation_set(&c.player(),c.player().camera,nullptr,start); const auto index=c.host.cart_resources().camera_animation(identity); if (index==menu::no_index) return ISLE_GAME_INVALID_ARGUMENT; IslePlayerCameraAnimationClip native{}; if (!c.host.camera()->clip(index,native)) return ISLE_GAME_INVALID_ARGUMENT; return isle_player_root_native_camera_animation_set(&c.player(),c.player().camera,&native,start); }); },
        .camera_process=+[](Source* s,IsleGameObject* object,std::int32_t kind,std::int32_t priority,IsleGameProcess** out) noexcept { if (out) *out=nullptr; return call(s,[&](Context& c) { auto r=c.camera(object); return r==ISLE_GAME_OK ? process(c,object,isle_course_scene_camera_update,kind,priority,out) : r; }); },
        .player_model_init=+[](Source* s,IsleGameObject** out) noexcept { if (out) *out=nullptr; return call(s,[&](Context& c) { if (!out) return ISLE_GAME_INVALID_ARGUMENT; const auto made=c.host.cart()->init(); *out=made.object; if (made) return ISLE_GAME_OK; if (c.player().status!=ISLE_GAME_OK) return c.player().status; const auto status=isle_game_status(c.host.game()); if (status!=ISLE_GAME_OK) return status; return !c.services().failed() && !made.object && made.status.error==player::cart::OwnerError::capacity ? ISLE_GAME_OK : ISLE_GAME_EXECUTION_FAILURE; }); },
        .player_animation=+[](Source* s,std::uint32_t model,std::uint32_t material,float start,float speed) noexcept { return call(s,[&](Context& c) { auto* carts=c.host.cart(); const auto* m=model ? carts->model_animation(model) : nullptr; const auto* t=material ? carts->material_animation(material) : nullptr; if ((model && !m) || (material && !t)) return ISLE_GAME_INVALID_ARGUMENT; return carts->set_animation(m,t,start,speed) ? ISLE_GAME_OK : ISLE_GAME_EXECUTION_FAILURE; }); },
        .player_change_animation=+[](Source* s,std::uint32_t model,std::uint32_t material,float start) noexcept { return call(s,[&](Context& c) { auto* carts=c.host.cart(); const auto* m=model ? carts->model_animation(model) : nullptr; const auto* t=material ? carts->material_animation(material) : nullptr; if ((model && !m) || (material && !t)) return ISLE_GAME_INVALID_ARGUMENT; return carts->change_animation(m,t,start) ? ISLE_GAME_OK : ISLE_GAME_EXECUTION_FAILURE; }); },
        .player_animation_speed=+[](Source* s,float speed) noexcept { return call(s,[&](Context& c) { return c.host.cart()->set_animation_speed(speed) ? ISLE_GAME_OK : ISLE_GAME_EXECUTION_FAILURE; }); },
        .animation_callback=+[](Source* s,IsleGameObject* object,IsleRiverAnimationCallback callback) noexcept { return call(s,[&](Context& c) { if (c.host.cart()->instance(object)) { IslePlayerCartCallback adapter=nullptr; if (callback==func_802E01C8_6C7FA8) adapter=isle_river_native_gate_cart_event; else if (callback==func_802E19A0_6C9780) adapter=isle_river_native_outro_cart_event; else if (callback) return ISLE_GAME_INVALID_ARGUMENT; return c.host.cart()->set_callback(object,adapter,callback ? s : nullptr) ? ISLE_GAME_OK : ISLE_GAME_EXECUTION_FAILURE; } return owner(c.owner().bind_animation_callback(*s,object,callback)); }); },
        .pause_processes=+[](Source* s,IsleGameObject* object) noexcept { return call(s,[&](Context& c) { return c.receiver(object) ? isle_game_pause_object_processes(c.host.game(),object,1) : ISLE_GAME_INVALID_ARGUMENT; }); },
        .resume_processes=+[](Source* s,IsleGameObject* object) noexcept { return call(s,[&](Context& c) { return c.receiver(object) ? isle_game_pause_object_processes(c.host.game(),object,0) : ISLE_GAME_INVALID_ARGUMENT; }); },
        .sky_speed=+[](Source* s,float speed) noexcept { return call(s,[&](Context& c) { return isle_player_root_native_skybox_animation_speed(&c.player(),speed); }); },
        .notice=+[](Source* s,std::uint32_t bits) noexcept { return call(s,[&](Context& c) { c.host.game_state().add_scene_notices(bits); return ISLE_GAME_OK; }); },
        .progress_read=+[](Source* s,std::int32_t* out) noexcept { return call(s,[&](Context& c) { if (!out) return ISLE_GAME_INVALID_ARGUMENT; auto* save=c.host.game_state().save(); if (!save) return ISLE_GAME_INVALID_STATE; const auto snapshot=save->snapshot(); if (!snapshot) return ISLE_GAME_INVALID_STATE; const auto value=snapshot.value.selected_course(); if (!value) return ISLE_GAME_INVALID_STATE; *out=value.value; return ISLE_GAME_OK; }); },
        .progress_write=+[](Source* s,std::int32_t value) noexcept { return call(s,[&](Context& c) { auto* save=c.host.game_state().save(); return save && !save->set_unlocked_course(value) ? ISLE_GAME_OK : ISLE_GAME_EXECUTION_FAILURE; }); },
        .transition_prepare=+[](Source* s,std::int32_t a,std::int32_t b) noexcept { return call(s,[&](Context& c) { auto* screen=c.services().screen_transition_source(); return screen ? isle_screen_transition_func_800A7F68(screen,a,b) : ISLE_GAME_INVALID_STATE; }); },
        .transition_color=+[](Source* s,std::int32_t r,std::int32_t g,std::int32_t b) noexcept { return call(s,[&](Context& c) { auto* screen=c.services().screen_transition_source(); return screen ? isle_screen_transition_func_800A7470(screen,r,g,b) : ISLE_GAME_INVALID_STATE; }); },
        .transition_fade=+[](Source* s,std::int32_t direction,float duration) noexcept { return call(s,[&](Context& c) { auto* screen=c.services().screen_transition_source(); return screen ? isle_screen_transition_func_800A7918(screen,direction,duration) : ISLE_GAME_INVALID_STATE; }); },
        .transition_active=+[](Source* s,std::int32_t* out) noexcept { return call(s,[&](Context& c) { auto* screen=c.services().screen_transition_source(); if (!out || !screen) return ISLE_GAME_INVALID_ARGUMENT; *out=isle_screen_transition_func_800A7460(screen); return ISLE_GAME_OK; }); },
        .create_helper=+[](Source* s,std::int32_t id,IsleGameSourceCallback update,std::int32_t link,std::uint32_t priority,IsleGameObject** out) noexcept { if (out) *out=nullptr; return call(s,[&](Context& c) { if (id<0 || link<0 || link>31 || (update && (!c.host.bindings() || !c.host.bindings()->contains(update)))) return ISLE_GAME_INVALID_ARGUMENT; return c.host.create_helper(std::uint32_t(id),update,std::uint8_t(link),priority,out); }); },
        .delete_object=+[](Source* s,IsleGameObject* object) noexcept { return call(s,[&](Context& c) { if (!c.receiver(object)) return ISLE_GAME_INVALID_ARGUMENT; if (c.host.cart()->instance(object)) return c.host.cart()->request_retire(object) ? ISLE_GAME_OK : ISLE_GAME_EXECUTION_FAILURE; c.services().erase(object); return c.services().failed() ? ISLE_GAME_EXECUTION_FAILURE : ISLE_GAME_OK; }); },
        .send_command=+[](Source* s,IsleGameObject* destination,std::int32_t command,IsleGameObject* sender) noexcept { return call(s,[&](Context& c) { if (!c.receiver(destination) || (sender && !c.services().source_owner_registered(sender))) return ISLE_GAME_INVALID_ARGUMENT; return c.services().send_command(destination,command,sender) ? ISLE_GAME_OK : ISLE_GAME_EXECUTION_FAILURE; }); },
        .send_command_link=+[](Source* s,std::int32_t link,std::int32_t command,IsleGameObject* sender) noexcept { return call(s,[&](Context& c) { if (link<0 || link>31 || (sender && !c.services().source_owner_registered(sender))) return ISLE_GAME_INVALID_ARGUMENT; return c.services().send_command_to_link(std::uint8_t(link),command,sender) ? ISLE_GAME_OK : ISLE_GAME_EXECUTION_FAILURE; }); },
        .distance=+[](Source* s,IsleGameObject* a,IsleGameObject* b,float* out) noexcept { return call(s,[&](Context& c) { return isle_actor_spatial_distance(c.host.game(),a,b,out); }); },
        .sound_stop_all=+[](Source* s) noexcept { return call(s,[&](Context& c) { return isle_player_root_native_stop_all_sounds(&c.player()); }); },
        .bgm_stop_all=+[](Source* s) noexcept { return call(s,[&](Context& c) { return isle_player_root_native_stop_bgm(&c.player()); }); },
        .remove_flying_items=+[](Source* s) noexcept { return call(s,[&](Context& c) { return c.host.items_owner()->remove_flying() ? ISLE_GAME_OK : ISLE_GAME_EXECUTION_FAILURE; }); },
        .reset_messages=+[](Source* s) noexcept { return call(s,[&](Context& c) { return c.player().ui ? isle_player_ui_Msg_Reset(c.player().ui) : ISLE_GAME_INVALID_STATE; }); },
        .audio_cutscene=+[](Source* s) noexcept { return call(s,[&](Context& c) { return c.services().reset_sound_ramps(); }); },
        .source_process=+[](Source* s,IsleGameObject* object,IsleGameSourceCallback callback,std::int32_t kind,std::int32_t priority,IsleGameProcess** out) noexcept { if (out) *out=nullptr; return call(s,[&](Context& c) { if (!c.host.bindings() || !c.host.bindings()->contains(callback)) return ISLE_GAME_INVALID_ARGUMENT; return process(c,object,callback,kind,priority,out); }); },
        .environment_sound=+[](Source* s,IsleGameObject* object,std::uint8_t category,std::int32_t sound) noexcept { return call(s,[&](Context& c) { IsleActorSource* a=nullptr; auto r=isle_actor_source_get(c.host.game(),object,&a); return r==ISLE_GAME_OK ? isle_actor_owner_environment_sound(a,category,sound) : r; }); },
        .definition=+[](Source* s,std::uint32_t identity,std::uint32_t function,std::uint32_t line,IsleRiverDefinition* out) noexcept { if (out) *out={}; return call(s,[&](Context& c) { if (!out) return ISLE_GAME_INVALID_ARGUMENT; const auto& resources=c.host.actor_resources(); if (identity) { if (function || line) return ISLE_GAME_INVALID_ARGUMENT; const auto& d=resources.definition(identity); if (!d.singleton || d.initialization>=resources.initializations().size()) return ISLE_GAME_INVALID_ARGUMENT; *out={identity,d.actor_id,d.factory_identity,d.change_block_identity,d.remove_identity,resources.initializations()[d.initialization].wire.guest_address,{0,0}}; return ISLE_GAME_OK; } for (const auto& d:resources.automatic_definitions()) if (d.origin.function_identity==function && d.origin.line==line) { if (d.initialization>=resources.initializations().size()) return ISLE_GAME_INVALID_STATE; *out={0,d.actor_id,d.factory_identity,d.change_block_identity,d.remove_identity,resources.initializations()[d.initialization].wire.guest_address,{function,line}}; return ISLE_GAME_OK; } return ISLE_GAME_INVALID_ARGUMENT; }); },
        .root_read=+[](Source* s,IsleGameModelRef ref,const IsleGameVec3** out) noexcept { if (out) *out=nullptr; return call(s,[&](Context& c) { if (!out) return ISLE_GAME_INVALID_ARGUMENT; const IsleGameModelNode* nodes=nullptr; auto r=root(c,ref,&nodes); if (r==ISLE_GAME_OK) *out=&nodes[0].transform.position; return r; }); },
    };
}
} // namespace isle::course

extern "C" IsleGameResult isle_river_native_cart_event_admit(IsleRiverActorSource* source,
    const IslePlayerCartNode* node,IslePlayerCartCallback callback) {
    return isle::course::call(source,[&](isle::course::Context& c) {
        if (!node || !callback ||
            !c.host.cart()->callback_matches(node->binding.object,callback,source) ||
            !c.host.cart()->issued_effect_node(node->binding,node->node))
            return ISLE_GAME_INVALID_ARGUMENT;
        return ISLE_GAME_OK;
    });
}
