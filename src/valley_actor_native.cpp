#include "valley_actor_native.hpp"
#include "valley_actor_window_mechanics.hpp"
#include "valley_actor_native_source.h"
#include "valley_actor.hpp"
#include "course_actor_native_services.hpp"
#include "actor_native_context.hpp"
#include "actor_interaction_services.hpp"
#include "actor_spatial.h"
#include "course_native_host.hpp"
#include "course_scene_source.h"
#include "beach_meowth_source.h"
#include "player_root.hpp"
#include "player_cart.hpp"
#include "player_cart_resources.hpp"
#include "player_camera_animation.hpp"
#include "scene_services.hpp"
#include "item_owner.hpp"
#include "menu_scene_bridge.h"
#include <bit>
#include <cmath>
#include <limits>
#include <utility>

namespace isle::valley {
ActorNativeMechanics::ActorNativeMechanics() noexcept = default;
ActorNativeMechanics::~ActorNativeMechanics() = default;
IsleGameResult ActorNativeMechanics::prepare(const rom::Snapshot& snapshot,IsleValleyActorSource& source) noexcept {
    if(source_ || resources_ || host_) return ISLE_GAME_INVALID_STATE;
    try { resources_=DialogueResources::load(snapshot,source); source_=&source; return ISLE_GAME_OK; }
    catch(...) { return ISLE_GAME_INVALID_STATE; }
}
IsleGameResult ActorNativeMechanics::bind(course::NativeHost& host) noexcept {
    if(!source_ || !resources_ || host_ || !host.closure_prepared() || !host.world_owner() || !host.root() || !host.actors() || !host.services() || !host.ui()) return ISLE_GAME_INVALID_STATE;
    try {
        auto& context=host.actors()->course_context().valley();
        if(&context.actors!=source_ || context.native_actor_mechanics!=this || context.native_host!=&host) return ISLE_GAME_INVALID_STATE;
        const auto native=host.world_owner()->blocks();
        descriptors_.resize(native.size()); blocks_.resize(native.size());
        for(std::size_t i=0;i<native.size();++i) {
            if(!native[i].resource) return ISLE_GAME_INVALID_STATE;
            const auto& p=native[i].resource->metadata.world_position;
            // The decoded resource position is immutable, not a model/World
            // mutable transform. This stable C view belongs to that resource.
            descriptors_[i].worldPos={p.x,p.y,p.z};
            blocks_[i]={nullptr,&descriptors_[i],&native[i]};
        }
        for(std::size_t i=0;i<native.size();++i) if(native[i].next) {
            bool found=false;
            for(std::size_t j=0;j<native.size();++j) if(native[i].next==&native[j]) { blocks_[i].next=&blocks_[j]; found=true; break; }
            if(!found) return ISLE_GAME_INVALID_STATE;
        }
        window_.emplace(*host.services(),*host.ui(),resources_->resources(),host.extra_sprite_base());
        host_=&host;
        camera={&host,nullptr};
        for(auto& effect:effects) effect.owner=&host;
        return block_views_valid() ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE;
    } catch(...) { return ISLE_GAME_INVALID_STATE; }
}
void ActorNativeMechanics::clear() noexcept {
    window_.reset(); camera={}; for(auto& effect:effects) effect={};
    clips={}; blocks_.clear(); descriptors_.clear(); host_=nullptr;
}
bool ActorNativeMechanics::block_views_valid() const noexcept {
    if(!host_ || !host_->world_owner()) return false;
    const auto native=host_->world_owner()->blocks();
    if(native.size()!=blocks_.size() || native.size()!=descriptors_.size()) return false;
    for(std::size_t i=0;i<native.size();++i) {
        if(blocks_[i].native!=&native[i] || blocks_[i].descriptor!=&descriptors_[i] || !native[i].resource) return false;
        const auto& p=native[i].resource->metadata.world_position;
        const auto& q=descriptors_[i].worldPos;
        if(p.x!=q.x || p.y!=q.y || p.z!=q.z) return false;
        if(!native[i].next) { if(blocks_[i].next) return false; }
        else {
            bool found=false;
            for(std::size_t j=0;j<native.size();++j) if(native[i].next==&native[j]) { found=blocks_[i].next==&blocks_[j]; break; }
            if(!found) return false;
        }
    }
    return true;
}
IsleValleyWorldBlock* ActorNativeMechanics::source_block(world::Block* value) noexcept {
    if(!value || !block_views_valid()) return nullptr;
    for(auto& view:blocks_) if(view.native==value) return &view;
    return nullptr;
}
world::Block* ActorNativeMechanics::native_block(IsleValleyWorldBlock* value) noexcept {
    if(!value || !block_views_valid()) return nullptr;
    const auto native=host_->world_owner()->blocks();
    for(std::size_t i=0;i<blocks_.size();++i) if(value==&blocks_[i]) return &native[i];
    return nullptr;
}
const IslePlayerCameraAnimationClip* ActorNativeMechanics::clip(std::uint32_t identity) const noexcept {
    constexpr std::array<std::uint32_t,3> identities{{0x8011c550u,0x8011da1cu,0x801204a0u}};
    for(std::size_t i=0;i<identities.size();++i) if(identity==identities[i] && clips[i].owner) return &clips[i];
    return nullptr;
}
IsleGameResult actor_native_mechanics(IsleValleyActorSource* source,ActorNativeMechanics** output) noexcept {
    if(output) *output=nullptr;
    if(!output) return ISLE_GAME_INVALID_ARGUMENT;
    actor::Owner* owner=nullptr;
    auto result=actor::native_owner(source,&owner);
    if(result!=ISLE_GAME_OK) return result;
    try {
        auto& selected=owner->course_context().valley();
        auto* mechanics=selected.native_actor_mechanics;
        if(!mechanics || &selected.actors!=source || mechanics->host()!=selected.native_host) return ISLE_GAME_INVALID_STATE;
        *output=mechanics; return ISLE_GAME_OK;
    } catch(...) { return ISLE_GAME_INVALID_STATE; }
}
namespace {
struct Context { actor::Owner& actors; course::NativeHost& host; ActorNativeMechanics& native; IslePlayerSource& player; };
template<class Function> IsleGameResult with(IsleValleyActorSource* source,Function function) noexcept {
    actor::Owner* owner=nullptr;
    const auto result=actor::native_owner(source,&owner);
    if(result!=ISLE_GAME_OK) return result;
    try {
        auto& selected=owner->course_context().valley();
        auto* host=selected.native_host;
        auto* native=selected.native_actor_mechanics;
        if(!host || !native || host->actors()!=owner || native->host()!=host ||
            host->game()!=source->base.nativeGame || !host->services() || !host->root() ||
            host->root()->bound_actors()!=owner || &host->root()->source()!=&owner->player_source())
            return ISLE_GAME_INVALID_STATE;
        Context context{*owner,*host,*native,owner->player_source()};
        return function(context);
    } catch(...) { return ISLE_GAME_INVALID_STATE; }
}
IsleGameResult status(actor::OwnerStatus value) noexcept {
    return value ? ISLE_GAME_OK : value.error==actor::OwnerError::capacity ? ISLE_GAME_EXHAUSTED : ISLE_GAME_EXECUTION_FAILURE;
}
IsleGameResult audio_result(isle::audio::Error value) noexcept { return value==isle::audio::Error::none ? ISLE_GAME_OK : ISLE_GAME_EXECUTION_FAILURE; }
IsleGameResult receiver(Context& c,IsleGameObject* object) noexcept {
    IsleGameObjectMetadata metadata{};
    return object ? isle_game_object_metadata(c.host.game(),object,&metadata) : ISLE_GAME_INVALID_ARGUMENT;
}
IsleGameResult actor_source(Context& c,IsleGameObject* object,IsleActorSource** output) noexcept {
    if(output) *output=nullptr;
    if(!output || !object) return ISLE_GAME_INVALID_ARGUMENT;
    auto result=isle_actor_source_get(c.host.game(),object,output);
    if(result!=ISLE_GAME_OK) return result;
    for(const auto& binding:c.actors.render_bindings()) if(binding.model.object==object && c.actors.source(binding.model.state.instance)==*output) return ISLE_GAME_OK;
    *output=nullptr; return ISLE_GAME_INVALID_STATE;
}
menu::animation::InstanceHandle instance(Context& c,IsleGameObject* object) noexcept {
    for(const auto& binding:c.actors.render_bindings()) if(binding.model.object==object) return binding.model.state.instance;
    return {};
}
IsleGameResult nodes(Context& c,IsleActorNodeRef reference,IsleGameModel** model,const IsleGameModelNode** output,std::size_t* count) noexcept {
    auto result=isle_game_model_resolve(c.host.game(),&reference.model,model);
    if(result==ISLE_GAME_OK) result=isle_game_model_peek_nodes(c.host.game(),*model,output,count);
    if(result==ISLE_GAME_OK && reference.node>=*count) result=ISLE_GAME_INVALID_ARGUMENT;
    return result;
}
const IsleGameVec3* member(const IsleGameModelTransform& transform,int32_t index) noexcept {
    return index==0 ? &transform.position : index==1 ? &transform.rotation : index==2 ? &transform.scale : nullptr;
}
IsleGameVec3* member(IsleGameModelTransform& transform,int32_t index) noexcept {
    return index==0 ? &transform.position : index==1 ? &transform.rotation : index==2 ? &transform.scale : nullptr;
}
void component(IsleGameVec3& vector,uint32_t index,float value) noexcept {
    if(index==0) vector.x=value; else if(index==1) vector.y=value; else vector.z=value;
}
IsleGameResult vector_read(Context& c,IsleValleyVectorRef reference,const IsleGameVec3** output) noexcept {
    if(output) *output=nullptr;
    if(!output || reference.local || reference.member<0 || reference.member>2 || reference.space<0 || reference.space>1) return ISLE_GAME_INVALID_ARGUMENT;
    IsleGameModel* model=nullptr; const IsleGameModelNode* view=nullptr; std::size_t count=0;
    auto result=nodes(c,reference.node,&model,&view,&count); if(result!=ISLE_GAME_OK) return result;
    if(reference.space==0) { *output=member(view[reference.node.node].transform,reference.member); return ISLE_GAME_OK; }
    if(reference.node.node!=0) return ISLE_GAME_INVALID_ARGUMENT;
    const IsleGameModelPlacement* placement=nullptr;
    result=isle_game_model_placement(c.host.game(),model,&placement);
    if(result!=ISLE_GAME_OK) return result;
    if(!placement || (placement->mode!=ISLE_GAME_MODEL_PLACEMENT_RT && placement->mode!=ISLE_GAME_MODEL_PLACEMENT_RTS) || (reference.member==2 && placement->mode!=ISLE_GAME_MODEL_PLACEMENT_RTS)) return ISLE_GAME_INVALID_STATE;
    *output=member(placement->transform,reference.member); return ISLE_GAME_OK;
}
IsleGameResult model(Context& c,IsleGameObject* object,IsleActorNodeRef* output) noexcept {
    if(output) *output={}; if(!output || !object) return ISLE_GAME_INVALID_ARGUMENT;
    IsleGameModel* native=nullptr; auto result=isle_actor_spatial_object_model(c.host.game(),object,&native);
    if(result!=ISLE_GAME_OK) return result;
    if(!native) return ISLE_GAME_INVALID_STATE;
    return isle_game_model_reference(c.host.game(),object,native,&output->model);
}
IsleGameResult camera(Context& c,IsleValleyCamera* value) noexcept {
    if(value!=&c.native.camera || value->owner!=&c.host || !value->camera || value->camera!=c.player.camera) return ISLE_GAME_INVALID_ARGUMENT;
    IsleGameObject* object=nullptr; return isle_game_camera_object(c.host.game(),value->camera,&object);
}
const menu::AnimationPath* path(Context& c,const void* value) noexcept {
    for(const auto& candidate:c.host.world_owner()->resources().paths) if(&candidate==value) return &candidate;
    return nullptr;
}
IsleGameResult clips(Context& c) noexcept {
    if(!c.host.camera()) return ISLE_GAME_INVALID_STATE;
    if(c.native.clips[0].owner && c.native.clips[1].owner && c.native.clips[2].owner) return ISLE_GAME_OK;
    constexpr std::array<std::uint32_t,3> identities{{0x8011c550u,0x8011da1cu,0x801204a0u}};
    for(std::size_t i=0;i<identities.size();++i) {
        const auto index=c.host.cart_resources().camera_animation(identities[i]);
        if(index==menu::no_index || !c.host.camera()->clip(index,c.native.clips[i])) return ISLE_GAME_INVALID_STATE;
    }
    return ISLE_GAME_OK;
}
IslePlayerCartCallback cart_callback(IsleValleyAnimationCallback callback) noexcept {
    if(callback==func_802D0420_7A99B0) return isle_valley_native_ceremony_cart_event;
    if(callback==func_802D0CD8_7AA268) return isle_valley_native_gate_cart_event;
    return nullptr;
}
IsleGameResult cart_animation(Context& c,uint32_t model_id,uint32_t material_id,float start,float speed,bool change) noexcept {
    auto* cart=c.host.cart(); if(!cart) return ISLE_GAME_INVALID_STATE;
    const auto* model_animation=model_id ? cart->model_animation(model_id) : nullptr;
    const auto* material_animation=material_id ? cart->material_animation(material_id) : nullptr;
    if((model_id && !model_animation) || (material_id && !material_animation)) return ISLE_GAME_INVALID_ARGUMENT;
    const auto result=change ? cart->change_animation(model_animation,material_animation,start) : cart->set_animation(model_animation,material_animation,start,speed);
    return result ? ISLE_GAME_OK : ISLE_GAME_EXECUTION_FAILURE;
}
}
} // namespace isle::valley

namespace isle::course {
IsleValleyActorServices make_valley_actor_services() noexcept {
    using namespace isle::valley;
    return {
        .reference_assign=+[](IsleValleyActorSource* s,IsleGameObject** field,IsleGameObject* value) noexcept { return with(s,[&](Context& c){ return c.host.reference(field,value); }); },
        .model=+[](IsleValleyActorSource* s,IsleGameObject* object,IsleActorNodeRef* out) noexcept { return with(s,[&](Context& c){ return model(c,object,out); }); },
        .player_model=+[](IsleValleyActorSource* s,IsleActorNodeRef* out) noexcept { return with(s,[&](Context& c){ return model(c,c.player.object,out); }); },
        .node_path=+[](IsleValleyActorSource* s,IsleActorNodeRef ref,const char* steps,IsleActorNodeRef* out) noexcept { if(out) *out={}; if(!steps || !out) return ISLE_GAME_INVALID_ARGUMENT; return with(s,[&](Context& c){ IsleGameModel* m=nullptr; const IsleGameModelNode* n=nullptr; size_t count=0; auto r=nodes(c,ref,&m,&n,&count); if(r!=ISLE_GAME_OK) return r; for(const char* p=steps;*p;++p) { uint32_t next; if(*p=='c') next=n[ref.node].first_child; else if(*p=='n') next=n[ref.node].next_sibling; else return ISLE_GAME_INVALID_ARGUMENT; if(next>=count) return ISLE_GAME_INVALID_STATE; ref.node=next; } *out=ref; return ISLE_GAME_OK; }); },
        .vector=+[](IsleValleyActorSource* s,IsleActorNodeRef node,int32_t space,int32_t which,IsleValleyVectorRef* out) noexcept { if(out) *out={}; if(!out) return ISLE_GAME_INVALID_ARGUMENT; return with(s,[&](Context& c){ IsleValleyVectorRef ref{node,space,which,nullptr}; const IsleGameVec3* value=nullptr; auto r=vector_read(c,ref,&value); if(r==ISLE_GAME_OK) *out=ref; return r; }); },
        .vector_read=+[](IsleValleyActorSource* s,IsleValleyVectorRef ref,const IsleGameVec3** out) noexcept { return with(s,[&](Context& c){ return vector_read(c,ref,out); }); },
        .vector_store=+[](IsleValleyActorSource* s,IsleValleyVectorRef ref,uint32_t part,float value) noexcept { if(part>2) return ISLE_GAME_INVALID_ARGUMENT; return with(s,[&](Context& c){ const IsleGameVec3* selected=nullptr; auto r=vector_read(c,ref,&selected); if(r!=ISLE_GAME_OK) return r; IsleGameModel* m=nullptr; r=isle_game_model_resolve(c.host.game(),&ref.node.model,&m); if(r!=ISLE_GAME_OK) return r; if(ref.space==1) { const IsleGameModelPlacement* placement=nullptr; r=isle_game_model_placement(c.host.game(),m,&placement); if(r!=ISLE_GAME_OK) return r; auto transform=placement->transform; component(*member(transform,ref.member),part,value); return isle_game_model_set_placement(c.host.game(),m,placement->mode,&transform); } const IsleGameModelNode* view=nullptr; size_t count=0; r=isle_game_model_peek_nodes(c.host.game(),m,&view,&count); if(r!=ISLE_GAME_OK) return r; auto transform=view[ref.node.node].transform; component(*member(transform,ref.member),part,value); auto handle=instance(c,ref.node.model.object); if(handle) { menu::animation::Transform pose{{transform.position.x,transform.position.y,transform.position.z},{transform.rotation.x,transform.rotation.y,transform.rotation.z},{transform.scale.x,transform.scale.y,transform.scale.z}}; return status(c.actors.set_local(handle,ref.node.node,pose)); } return isle_game_model_set_transform(c.host.game(),m,ref.node.node,&transform); }); },
        .item=+[](IsleValleyActorSource* s,IsleGameObject* object,IsleItemSource** out) noexcept { if(out) *out=nullptr; if(!out) return ISLE_GAME_INVALID_ARGUMENT; return with(s,[&](Context& c){ auto* item=c.host.items_owner()->source(object); if(!item) return ISLE_GAME_INVALID_ARGUMENT; *out=item; return ISLE_GAME_OK; }); },
        .item_direction=+[](IsleValleyActorSource* s,IsleGameObject* object,IsleGameVec3* out) noexcept { if(!out) return ISLE_GAME_INVALID_ARGUMENT; return with(s,[&](Context& c){ auto* item=c.host.items_owner()->source(object); if(!item) return ISLE_GAME_INVALID_ARGUMENT; *out=item->collisionVelocity; return ISLE_GAME_OK; }); },
        .current_block=+[](IsleValleyActorSource* s,IsleValleyWorldBlock** out) noexcept { if(out) *out=nullptr; if(!out) return ISLE_GAME_INVALID_ARGUMENT; return with(s,[&](Context& c){ auto* actual=c.host.world_owner()->current(); if(!actual) return ISLE_GAME_OK; *out=c.native.source_block(actual); return *out ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE; }); },
        .graph=+[](IsleValleyActorSource* s,uint32_t id,const IsleActorInteractionGraph** out) noexcept { if(out) *out=nullptr; if(!out) return ISLE_GAME_INVALID_ARGUMENT; return with(s,[&](Context& c){ auto* interaction=c.actors.bound_interactions(); if(!interaction) return ISLE_GAME_INVALID_STATE; *out=&interaction->native_graph(id); return ISLE_GAME_OK; }); },
        .graph_copy=+[](IsleValleyActorSource* s,uint32_t id,IsleActorInteraction* out,size_t count) noexcept { if(!out) return ISLE_GAME_INVALID_ARGUMENT; return with(s,[&](Context& c){ auto* interaction=c.actors.bound_interactions(); if(!interaction) return ISLE_GAME_INVALID_STATE; const auto& graph=interaction->native_graph(id); if(graph.count!=count) return ISLE_GAME_INVALID_ARGUMENT; for(size_t i=0;i<count;++i) out[i]=graph.rows[i]; return ISLE_GAME_OK; }); },
        .weighted=+[](IsleValleyActorSource* s,uint32_t id,const IsleActorRandomState** out) noexcept { if(out) *out=nullptr; if(!out) return ISLE_GAME_INVALID_ARGUMENT; return with(s,[&](Context& c){ auto* interaction=c.actors.bound_interactions(); if(!interaction) return ISLE_GAME_INVALID_STATE; *out=interaction->native_weighted(id).rows.data(); return ISLE_GAME_OK; }); },
        .weighted_copy=+[](IsleValleyActorSource* s,uint32_t id,IsleActorRandomState* out,size_t count) noexcept { if(!out) return ISLE_GAME_INVALID_ARGUMENT; return with(s,[&](Context& c){ auto* interaction=c.actors.bound_interactions(); if(!interaction) return ISLE_GAME_INVALID_STATE; auto rows=interaction->native_weighted(id).rows; if(rows.size()!=count) return ISLE_GAME_INVALID_ARGUMENT; for(size_t i=0;i<count;++i) out[i]=rows[i]; return ISLE_GAME_OK; }); },
        .forbidden_ground=+[](IsleValleyActorSource* s,IsleActorSource* actor,uint32_t id) noexcept { return with(s,[&](Context& c){ IsleActorSource* actual=nullptr; if(!actor) return ISLE_GAME_INVALID_ARGUMENT; auto r=actor_source(c,actor->object,&actual); if(r!=ISLE_GAME_OK || actual!=actor) return ISLE_GAME_INVALID_ARGUMENT; const float* field=nullptr; switch(id) { case 0: break; case 0x802d2c0cu: field=&s->D_802D2C0C_7AC19C; break; case 0x802d3638u: field=&s->D_802D3638_7ACBC8; break; case 0x802d384cu: field=&s->D_802D384C_7ACDDC; break; case 0x802ec894u: field=&s->D_802EC894_7C5E24; break; default: return ISLE_GAME_INVALID_ARGUMENT; } actor->forbiddenGround=field; actor->forbiddenGroundCount=field?1:0; return ISLE_GAME_OK; }); },
        .add_one=+[](IsleValleyActorSource* s,IsleValleyWorldBlock* first,IsleValleyWorldBlock* second,const IsleValleyAutomaticSpawn* spawn,const IsleValleyDefinition* definition,IsleGameObject** out) noexcept { if(out) *out=nullptr; if(!spawn || !definition || !out) return ISLE_GAME_INVALID_ARGUMENT; return with(s,[&](Context& c){ auto* a=c.native.native_block(first); auto* b=c.native.native_block(second); if(!a || !b) return ISLE_GAME_INVALID_ARGUMENT; const auto result=c.actors.add_one_valley(*definition,*spawn,a,b); if(result) *out=result.object; return status(result.status); }); },
        .add_geo=+[](IsleValleyActorSource* s,IsleGameObject* object,uint16_t requested,const IsleValleyDefinition* definition,IsleGameObject** out) noexcept { if(out) *out=nullptr; if(!definition || !out) return ISLE_GAME_INVALID_ARGUMENT; return with(s,[&](Context& c){ const auto result=c.actors.add_geo_valley(object,requested,*definition); if(result) *out=result.object; return status(result.status); }); },
        .ground=+[](IsleValleyActorSource* s,float x,float z,IsleActorGround* out,int32_t* found) noexcept { if(!out || !found) return ISLE_GAME_INVALID_ARGUMENT; return with(s,[&](Context& c){ world::GroundResult value; const bool present=c.host.world_owner()->ground_at(x,z,value); *out={value.height,{value.normal.x,value.normal.y,value.normal.z},static_cast<uint32_t>(value.surface)}; *found=present?1:0; return ISLE_GAME_OK; }); },
        .node_world=+[](IsleValleyActorSource* s,IsleGameVec3* position,IsleGameVec3* velocity,IsleActorNodeRef ref) noexcept { if(!position || !velocity) return ISLE_GAME_INVALID_ARGUMENT; return with(s,[&](Context& c){ IsleActorSource* requester=nullptr; auto r=actor_source(c,isle_menu_current_object(c.host.game()),&requester); return r==ISLE_GAME_OK ? isle_actor_owner_node_world_ref(requester,&ref,position,velocity) : r; }); },
        .level_progress=+[](IsleValleyActorSource* s,int32_t* block,float* part) noexcept { if(!block || !part) return ISLE_GAME_INVALID_ARGUMENT; return with(s,[&](Context& c){ for(const auto& value:c.host.world_owner()->blocks()) if(&value==c.player.movement.block && value.index<=size_t(INT32_MAX)) { *block=static_cast<int32_t>(value.index); *part=c.player.movement.move_time; return ISLE_GAME_OK; } return ISLE_GAME_INVALID_STATE; }); },
        .path_position=+[](IsleValleyActorSource* s,IsleGameVec3* out,const void* value,float parameter) noexcept { if(!out) return ISLE_GAME_INVALID_ARGUMENT; return with(s,[&](Context& c){ auto* admitted=path(c,value); if(!admitted) return ISLE_GAME_INVALID_ARGUMENT; const auto p=menu::animation::path_position(*admitted,parameter); *out={p.x,p.y,p.z}; return ISLE_GAME_OK; }); },
        .path_start=+[](IsleValleyActorSource* s,const void* value,float* out) noexcept { if(!out) return ISLE_GAME_INVALID_ARGUMENT; return with(s,[&](Context& c){ auto* admitted=path(c,value); if(!admitted || admitted->timestamps.empty()) return ISLE_GAME_INVALID_ARGUMENT; *out=admitted->timestamps.front(); return ISLE_GAME_OK; }); },
        .animation_callback=+[](IsleValleyActorSource* s,IsleGameObject* object,IsleValleyAnimationCallback callback) noexcept {
            return with(s,[&](Context& c){
                if(auto* cart=c.host.cart(); cart && cart->instance(object)) {
                    const auto adapter=cart_callback(callback);
                    if(callback && !adapter) return ISLE_GAME_INVALID_ARGUMENT;
                    return cart->set_callback(object,adapter,callback ? s : nullptr) ? ISLE_GAME_OK : ISLE_GAME_EXECUTION_FAILURE;
                }
                return status(c.actors.bind_animation_callback(*s,object,callback));
            });
        },
        .model_animation_speed=+[](IsleValleyActorSource* s,IsleGameObject* object,float speed) noexcept { return with(s,[&](Context& c){ const auto handle=instance(c,object); return handle && c.actors.animation().set_model_speed(handle,speed) ? ISLE_GAME_OK : ISLE_GAME_INVALID_ARGUMENT; }); },
        .camera=+[](IsleValleyActorSource* s,IsleValleyCamera** out) noexcept { if(out) *out=nullptr; if(!out) return ISLE_GAME_INVALID_ARGUMENT; return with(s,[&](Context& c){ if(!c.player.camera) return ISLE_GAME_INVALID_STATE; c.native.camera.camera=c.player.camera; auto r=camera(c,&c.native.camera); if(r==ISLE_GAME_OK) *out=&c.native.camera; return r; }); },
        .camera_speed=+[](IsleValleyActorSource* s,IsleValleyCamera* value,float speed) noexcept { return with(s,[&](Context& c){ auto r=camera(c,value); if(r!=ISLE_GAME_OK) return r; return c.host.camera() && c.host.camera()->set_speed(speed) ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE; }); },
        .camera_read=+[](IsleValleyActorSource* s,IsleValleyCamera* value,const IsleGameLookAt** out) noexcept { if(out) *out=nullptr; if(!out) return ISLE_GAME_INVALID_ARGUMENT; return with(s,[&](Context& c){ auto r=camera(c,value); if(r!=ISLE_GAME_OK) return r; const IsleGameCameraParameters* params=nullptr; r=isle_game_camera_parameters(c.host.game(),value->camera,&params); if(r==ISLE_GAME_OK) *out=&params->look_at; return r; }); },
        .camera_component=+[](IsleValleyActorSource* s,IsleValleyCamera* value,int32_t target,uint32_t part,float number) noexcept { if(part>2 || target<0 || target>1) return ISLE_GAME_INVALID_ARGUMENT; return with(s,[&](Context& c){ auto r=camera(c,value); if(r!=ISLE_GAME_OK) return r; const IsleGameCameraParameters* params=nullptr; r=isle_game_camera_parameters(c.host.game(),value->camera,&params); if(r!=ISLE_GAME_OK) return r; auto next=params->look_at; (target?next.at:next.eye)[part]=number; return isle_game_camera_set_look_at(c.host.game(),value->camera,&next); }); },
        .camera_object=+[](IsleValleyActorSource* s,IsleValleyCamera* value,IsleGameObject** out) noexcept { if(out) *out=nullptr; if(!out) return ISLE_GAME_INVALID_ARGUMENT; return with(s,[&](Context& c){ auto r=camera(c,value); return r==ISLE_GAME_OK ? isle_game_camera_object(c.host.game(),value->camera,out) : r; }); },
        .camera_animation=+[](IsleValleyActorSource* s,IsleValleyCamera* value,uint32_t id,float start) noexcept { return with(s,[&](Context& c){ auto r=camera(c,value); if(r!=ISLE_GAME_OK) return r; r=clips(c); if(r!=ISLE_GAME_OK) return r; auto* clip=c.native.clip(id); if(!clip) return ISLE_GAME_INVALID_ARGUMENT; return c.host.camera()->set(clip,start) ? ISLE_GAME_OK : ISLE_GAME_EXECUTION_FAILURE; }); },
        .camera_animation_process=+[](IsleValleyActorSource* s,IsleGameObject* object,int32_t kind,uint32_t priority,IsleGameProcess** out) noexcept { if(out) *out=nullptr; if(!out || kind!=1 || priority!=1) return ISLE_GAME_INVALID_ARGUMENT; return with(s,[&](Context& c){ IsleGameObject* actual=nullptr; auto r=isle_game_camera_object(c.host.game(),c.player.camera,&actual); if(r!=ISLE_GAME_OK || object!=actual) return ISLE_GAME_INVALID_ARGUMENT; return isle_game_add_source_process(c.host.game(),object,isle_course_scene_camera_update,ISLE_GAME_SYNCHRONOUS,priority,out); }); },
        .pause_processes=+[](IsleValleyActorSource* s,IsleGameObject* object) noexcept { return with(s,[&](Context& c){ return isle_game_pause_object_processes(c.host.game(),object,1); }); },
        .resume_processes=+[](IsleValleyActorSource* s,IsleGameObject* object) noexcept { return with(s,[&](Context& c){ return isle_game_pause_object_processes(c.host.game(),object,0); }); },
        .player_model_init=+[](IsleValleyActorSource* s,IsleGameObject** out) noexcept {
            if(out) *out=nullptr;
            if(!out) return ISLE_GAME_INVALID_ARGUMENT;
            return with(s,[&](Context& c){
                auto* cart=c.host.cart();
                if(!cart) return ISLE_GAME_INVALID_STATE;
                const auto made=cart->init();
                if(c.player.status!=ISLE_GAME_OK) return c.player.status;
                const auto game_status=isle_game_status(c.host.game());
                if(game_status!=ISLE_GAME_OK) return game_status;
                if(c.host.services()->failed()) return ISLE_GAME_EXECUTION_FAILURE;
                if(made) { *out=made.object; return ISLE_GAME_OK; }
                // Owner::init reports this only after the actual C call returns
                // OK+NULL; a failed C call also leaves Player/Services terminal.
                return !made.object && !made.instance && made.status.error==player::cart::OwnerError::capacity ?
                    ISLE_GAME_OK : ISLE_GAME_EXECUTION_FAILURE;
            });
        },
        .player_model_animation=+[](IsleValleyActorSource* s,uint32_t m,uint32_t t,float start,float speed) noexcept { return with(s,[&](Context& c){ return cart_animation(c,m,t,start,speed,false); }); },
        .player_model_change=+[](IsleValleyActorSource* s,uint32_t m,uint32_t t,float start) noexcept { return with(s,[&](Context& c){ return cart_animation(c,m,t,start,0,true); }); },
        .player_model_speed=+[](IsleValleyActorSource* s,float speed) noexcept { return with(s,[&](Context& c){ return c.host.cart() && c.host.cart()->set_animation_speed(speed) ? ISLE_GAME_OK : ISLE_GAME_EXECUTION_FAILURE; }); },
        .cutscene_start=+[](IsleValleyActorSource* s,IsleGameObject* object,uint32_t id,float start) noexcept { return with(s,[&](Context& c){ auto r=receiver(c,object); if(r!=ISLE_GAME_OK) return r; r=clips(c); if(r!=ISLE_GAME_OK) return r; const auto* clip=id?c.native.clip(id):nullptr; if(id && !clip) return ISLE_GAME_INVALID_ARGUMENT; return isle_player_root_invoke_start_cutscene(&c.player,object,clip,start); }); },
        .sound=+[](IsleValleyActorSource* s,uint32_t id,int32_t* out) noexcept { if(!out) return ISLE_GAME_INVALID_ARGUMENT; return with(s,[&](Context& c){ *out=c.host.services()->play_sound(id); return c.host.services()->failed()?ISLE_GAME_EXECUTION_FAILURE:ISLE_GAME_OK; }); },
        .sound_params=+[](IsleValleyActorSource* s,uint32_t id,int32_t volume,int32_t pan,float pitch,int32_t reverb,int32_t* out) noexcept { if(!out || volume<0 || volume>0x7fff || pan<0 || pan>127 || reverb<0 || reverb>127) return ISLE_GAME_INVALID_ARGUMENT; return with(s,[&](Context& c){ *out=c.host.services()->play_sound(id,{static_cast<uint16_t>(volume),static_cast<uint8_t>(pan),static_cast<uint8_t>(reverb),pitch}); return c.host.services()->failed()?ISLE_GAME_EXECUTION_FAILURE:ISLE_GAME_OK; }); },
        .sound_active=+[](IsleValleyActorSource* s,int32_t handle,int32_t* out) noexcept { if(!out || handle<0 || size_t(handle)>=game_audio::sound_slots) return ISLE_GAME_INVALID_ARGUMENT; return with(s,[&](Context& c){ *out=c.host.audio().playing_sound(static_cast<unsigned>(handle)); return ISLE_GAME_OK; }); },
        .sound_stop=+[](IsleValleyActorSource* s,uint32_t handle) noexcept { return with(s,[&](Context& c){ return audio_result(c.host.audio().stop_sound(handle)); }); },
        .sound_stop_all=+[](IsleValleyActorSource* s) noexcept { return with(s,[&](Context& c){ return audio_result(c.host.audio().stop_sounds()); }); },
        .sound_volume=+[](IsleValleyActorSource* s,int32_t handle,uint32_t value) noexcept { return with(s,[&](Context& c){ return handle<0?ISLE_GAME_INVALID_ARGUMENT:audio_result(c.host.audio().sound_volume(static_cast<unsigned>(handle),value)); }); },
        .sound_global_volume=+[](IsleValleyActorSource* s,uint8_t value) noexcept { return with(s,[&](Context& c){ return audio_result(c.host.audio().global_sound_volume(value)); }); },
        .sound_volume_smooth=+[](IsleValleyActorSource* s,int32_t handle,uint32_t from,uint32_t to,int32_t frames,int32_t* out) noexcept { if(!out) return ISLE_GAME_INVALID_ARGUMENT; return with(s,[&](Context& c){ return c.host.services()->ramp_sound_volume(handle,from,to,frames,out); }); },
        .bgm_play=+[](IsleValleyActorSource* s,int32_t player,uint32_t song,uint32_t* out) noexcept { if(!out || player<0 || player>1) return ISLE_GAME_INVALID_ARGUMENT; return with(s,[&](Context& c){ if(!c.host.audio().has_song(song)) { *out=UINT32_MAX; return ISLE_GAME_OK; } auto r=audio_result(c.host.audio().play_song(static_cast<unsigned>(player),song)); if(r==ISLE_GAME_OK) *out=song; return r; }); },
        .bgm_stop=+[](IsleValleyActorSource* s) noexcept { return with(s,[&](Context& c){ return audio_result(c.host.audio().stop_bgm()); }); },
        .bgm_volume=+[](IsleValleyActorSource* s,int32_t player,uint32_t volume) noexcept { if(player<0 || player>1) return ISLE_GAME_INVALID_ARGUMENT; return with(s,[&](Context& c){ return audio_result(c.host.audio().bgm_volume(static_cast<unsigned>(player),volume)); }); },
        .bgm_volume_smooth=+[](IsleValleyActorSource* s,int32_t player,uint32_t volume,uint32_t frames) noexcept { return with(s,[&](Context& c){ return isle_player_root_native_bgm_volume_smooth(&c.player,player,static_cast<int32_t>(volume),static_cast<int32_t>(frames)); }); },
        .background_music=+[](IsleValleyActorSource* s,int32_t song) noexcept { return with(s,[&](Context& c){ return isle_player_root_invoke_set_background_music(&c.player,song); }); },
        .check_player_flag=isle_valley_window_check_player_flag,
        .set_player_flag=isle_valley_window_set_player_flag,
        .remove_flying_items=+[](IsleValleyActorSource* s) noexcept { return with(s,[&](Context& c){ return c.host.items_owner()->remove_flying()?ISLE_GAME_OK:ISLE_GAME_EXECUTION_FAILURE; }); },
        .reset_camera=+[](IsleValleyActorSource* s) noexcept { return with(s,[&](Context& c){ return isle_player_root_invoke_reset_main_camera_settings(&c.player); }); },
        .sky_speed=+[](IsleValleyActorSource* s,float speed) noexcept { return with(s,[&](Context& c){ return isle_player_root_native_skybox_animation_speed(&c.player,speed); }); },
        .message_reset=isle_valley_window_message_reset,
        .message_interactive=isle_valley_window_message_interactive,
        .message_noninteractive=isle_valley_window_message_noninteractive,
        .message_printed=isle_valley_window_message_printed,
        .message_show=isle_valley_window_message_show,
        .message_handler=isle_valley_window_message_handler,
        .dialogue=+[](IsleValleyActorSource* s,uint32_t id,size_t index,const IsleValleyDialogue** out) noexcept { return with(s,[&](Context& c){ return c.native.resources().dialogue(id,index,out); }); },
        .sprite=+[](IsleValleyActorSource* s,uint32_t id,const IsleSprite** out) noexcept { return with(s,[&](Context& c){ return c.native.resources().sprite(id,out); }); },
        .sprite_list=+[](IsleValleyActorSource* s,uint32_t id,const IsleSprite* const** out) noexcept { return with(s,[&](Context& c){ return c.native.resources().sprite_list(id,out); }); },
        .sprite_add=isle_valley_window_sprite_add,
        .sprite_move=isle_valley_window_sprite_move,
        .sprite_x2_move=isle_valley_window_sprite_x2_move,
        .sprite_scale=isle_valley_window_sprite_scale,
        .sprite_color=isle_valley_window_sprite_color,
        .sprite_link=isle_valley_window_sprite_link,
        .object_add=+[](IsleValleyActorSource* s,uint32_t id,IsleGameSourceCallback update,uint8_t link,uint32_t priority,IsleGameObject** out) noexcept { return with(s,[&](Context& c){ if(update) { bool admitted=false; for(const auto& callback:valley::actor_callbacks()) admitted|=callback.function==update; if(!admitted) return ISLE_GAME_INVALID_ARGUMENT; } return c.host.create_helper(id,update,link,priority,out); }); },
        .object_default_add=+[](IsleValleyActorSource* s,uint32_t id,uint8_t link,uint32_t priority,IsleGameObject** out) noexcept { return with(s,[&](Context& c){ return c.host.create_helper(id,nullptr,link,priority,out); }); },
        .input_pressed=+[](IsleValleyActorSource* s,uint32_t* out) noexcept { if(!out) return ISLE_GAME_INVALID_ARGUMENT; return with(s,[&](Context& c){ *out=c.player.input.pressed_buttons; return ISLE_GAME_OK; }); },
        .player_object=+[](IsleValleyActorSource* s,IsleGameObject** out) noexcept { if(out) *out=nullptr; if(!out) return ISLE_GAME_INVALID_ARGUMENT; return with(s,[&](Context& c){ auto r=receiver(c,c.player.object); if(r==ISLE_GAME_OK) *out=c.player.object; return r; }); },
        .player_name=isle_valley_window_player_name,
        .effect_create=+[](IsleValleyActorSource* s,int32_t bank,int32_t id,IsleValleyEffect** out) noexcept { if(out) *out=nullptr; if(!out || bank<0 || bank>UINT16_MAX || id<0) return ISLE_GAME_INVALID_ARGUMENT; return with(s,[&](Context& c){ const auto result=c.actors.effects().create_effect(static_cast<uint16_t>(bank),static_cast<size_t>(id)); if(!result.status) return ISLE_GAME_EXECUTION_FAILURE; if(!result.spawned) return ISLE_GAME_OK; if(result.handle.slot>=c.native.effects.size()) return ISLE_GAME_INVALID_STATE; auto& effect=c.native.effects[result.handle.slot]; effect={&c.host,result.handle}; *out=&effect; return ISLE_GAME_OK; }); },
        .effect_attach=+[](IsleValleyActorSource* s,IsleValleyEffect* effect,IsleActorNodeRef node) noexcept { return with(s,[&](Context& c){ bool admitted=false; for(auto& candidate:c.native.effects) admitted|=&candidate==effect; if(!admitted || effect->owner!=&c.host || !effect->handle || !c.host.cart()) return ISLE_GAME_INVALID_ARGUMENT; auto credit=c.host.cart()->retain_effect_node(node.model,node.node); if(!credit) return ISLE_GAME_INVALID_STATE; return c.actors.effects().set_effect_attachment(effect->handle,std::move(credit))?ISLE_GAME_OK:ISLE_GAME_EXECUTION_FAILURE; }); },
        .application_notice=isle_valley_window_application_notice,
        .transition_begin=isle_valley_window_transition_begin,
        .transition_color=isle_valley_window_transition_color,
        .transition_busy=isle_valley_window_transition_busy,
        .audio_cutscene_reset=+[](IsleValleyActorSource* s) noexcept { return with(s,[&](Context& c){ return c.host.services()->reset_sound_ramps(); }); },
        .environment_sound=+[](IsleValleyActorSource* s,IsleGameObject* object,uint8_t category,int32_t sound) noexcept { return with(s,[&](Context& c){ IsleActorSource* actor=nullptr; auto r=actor_source(c,object,&actor); return r==ISLE_GAME_OK?isle_actor_owner_environment_sound(actor,category,sound):r; }); },
        .kick_item=+[](IsleValleyActorSource* s,IsleGameObject* object,float speed) noexcept { return with(s,[&](Context& c){ return c.host.items_owner()->kick(object,speed)?ISLE_GAME_OK:ISLE_GAME_EXECUTION_FAILURE; }); },
        .jump_bounce=+[](IsleValleyActorSource* s,IsleGameObject* object,float speed,float backwards,float parameter) noexcept { return with(s,[&](Context& c){ IsleActorSource* actor=nullptr; auto r=actor_source(c,object,&actor); if(r!=ISLE_GAME_OK) return r; if(isle_menu_current_object(c.host.game())!=object || !std::isfinite(speed) || !std::isfinite(backwards) || !std::isfinite(parameter)) return ISLE_GAME_INVALID_ARGUMENT; IsleActorNodeRef ref{}; r=model(c,object,&ref); if(r!=ISLE_GAME_OK) return r; const IsleGameVec3* value=nullptr; return vector_read(c,{ref,1,0,nullptr},&value); }); },
        .common_child1=+[](IsleValleyActorSource* s,IsleGameObject* object,IsleGameObject** out) noexcept { return with(s,[&](Context& c){ auto r=receiver(c,object); return r==ISLE_GAME_OK?isle_valley_native_child(s,object,0,out):r; }); },
        .common_child2=+[](IsleValleyActorSource* s,IsleGameObject* object,IsleGameObject** out) noexcept { return with(s,[&](Context& c){ auto r=receiver(c,object); return r==ISLE_GAME_OK?isle_valley_native_child(s,object,1,out):r; }); },
        .block_change=+[](IsleValleyActorSource* s,IsleGameObject* object,float a,float b,float c,float d,float e,float f) noexcept { return with(s,[&](Context& context){ auto r=receiver(context,object); return r==ISLE_GAME_OK?isle_meowth_source_call_change_block(object,a,b,c,d,e,f,0):r; }); },
        .block_change_ground=+[](IsleValleyActorSource* s,IsleGameObject* object,float a,float b,float c,float d,float e,float f) noexcept { return with(s,[&](Context& context){ auto r=receiver(context,object); return r==ISLE_GAME_OK?isle_meowth_source_call_change_block(object,a,b,c,d,e,f,1):r; }); },
        .common_remove=+[](IsleValleyActorSource* s,IsleGameObject* object) noexcept { return with(s,[&](Context& c){ IsleActorSource* actor=nullptr; auto r=actor_source(c,object,&actor); return r==ISLE_GAME_OK ? isle_actor_owner_schedule_cleanup(actor) : r; }); },
        .misc_object_get=+[](IsleValleyActorSource* s,IsleGameObject* object,uint32_t index,IsleGameObject** out) noexcept { if(out) *out=nullptr; return with(s,[&](Context& c){ IsleActorSource* actor=nullptr; auto r=actor_source(c,object,&actor); return r==ISLE_GAME_OK?isle_actor_owner_misc_object_get(actor,index,out):r; }); },
        .misc_object_set=+[](IsleValleyActorSource* s,IsleGameObject* object,uint32_t index,IsleGameObject* value) noexcept { return with(s,[&](Context& c){ IsleActorSource* actor=nullptr; auto r=actor_source(c,object,&actor); return r==ISLE_GAME_OK?isle_actor_owner_misc_object_set(actor,index,value):r; }); },
        .model_time_left=+[](IsleValleyActorSource* s,IsleActorNodeRef ref,float value) noexcept {
            return with(s,[&](Context& c){
                IsleGameModel* m=nullptr; const IsleGameModelNode* n=nullptr; size_t count=0;
                auto r=nodes(c,ref,&m,&n,&count);
                if(r!=ISLE_GAME_OK) return r;
                if(auto* cart=c.host.cart(); cart && cart->instance(ref.model.object)) {
                    if(!cart->callback_matches(ref.model.object,isle_valley_native_gate_cart_event,s))
                        return ISLE_GAME_INVALID_ARGUMENT;
                    return cart->set_node_time_left(ref.model,ref.node,value) ? ISLE_GAME_OK : ISLE_GAME_EXECUTION_FAILURE;
                }
                const auto handle=instance(c,ref.model.object);
                return handle && c.actors.animation().set_node_time_left(handle,ref.node,value) ? ISLE_GAME_OK : ISLE_GAME_INVALID_ARGUMENT;
            });
        },
        .rng_range=+[](IsleValleyActorSource* s,int32_t range,int32_t* out) noexcept { if(!out) return ISLE_GAME_INVALID_ARGUMENT; return with(s,[&](Context& c){ if(!c.player.rng) return ISLE_GAME_INVALID_STATE; *out=isle_rng_range(c.player.rng,range); return ISLE_GAME_OK; }); },
        .rng_float=+[](IsleValleyActorSource* s,float* out) noexcept { if(!out) return ISLE_GAME_INVALID_ARGUMENT; return with(s,[&](Context& c){ if(!c.player.rng) return ISLE_GAME_INVALID_STATE; *out=isle_rng_next_float(c.player.rng); return ISLE_GAME_OK; }); },
        .player_cutscene_process=+[](IsleValleyActorSource* s,IsleGameObject* object) noexcept { return with(s,[&](Context& c){ auto r=receiver(c,object); return r==ISLE_GAME_OK?isle_course_scene_object_control(&c.player,isle_player_func_80357150_4F7560,object):r; }); },
        .flute=+[](IsleValleyActorSource* s,int32_t* out) noexcept { return with(s,[&](Context& c){ return c.host.flute_command(out); }); },
        .command=+[](IsleValleyActorSource* s,IsleGameObject* destination,int32_t command,IsleGameObject* sender) noexcept { return with(s,[&](Context& c){ return c.host.services()->send_command(destination,command,sender)?ISLE_GAME_OK:ISLE_GAME_EXECUTION_FAILURE; }); },
        .command_link=+[](IsleValleyActorSource* s,uint8_t link,int32_t command,IsleGameObject* sender) noexcept { return with(s,[&](Context& c){ return c.host.services()->send_command_to_link(link,command,sender)?ISLE_GAME_OK:ISLE_GAME_EXECUTION_FAILURE; }); },
        .vector_distance=+[](IsleValleyActorSource* s,const IsleGameVec3* a,const IsleGameVec3* b,float* out) noexcept { if(!a || !b || !out) return ISLE_GAME_INVALID_ARGUMENT; return with(s,[&](Context&){ const float x=a->x-b->x,y=a->y-b->y,z=a->z-b->z; *out=std::sqrt(x*x+y*y+z*z); return ISLE_GAME_OK; }); },
        .dialogue_format=+[](IsleValleyActorSource* s,char* out,size_t capacity,const IsleValleyDialogue* row,int32_t* count) noexcept { return with(s,[&](Context& c){ bool admitted=false; for(size_t i=0;i<14;++i) { const IsleValleyDialogue* value=nullptr; if(c.native.resources().dialogue(0x802ec51cu,i,&value)==ISLE_GAME_OK && value==row) admitted=true; } const IsleValleyDialogue* value=nullptr; if(c.native.resources().dialogue(0x802ec5d0u,0,&value)==ISLE_GAME_OK && value==row) admitted=true; return admitted?DialogueResources::dialogue_format(out,capacity,row,count):ISLE_GAME_INVALID_ARGUMENT; }); }
    };
}
} // namespace isle::course

extern "C" IsleGameResult isle_valley_native_cart_event_admit(IsleValleyActorSource* source,
    const IslePlayerCartNode* node,IsleValleyAnimationCallback callback,int32_t event,float value) {
    return isle::valley::with(source,[&](isle::valley::Context& c){
        auto* cart=c.host.cart();
        const auto adapter=isle::valley::cart_callback(callback);
        if(!cart || !adapter || cart->source()!=c.player.cart || cart->source()->player!=&c.player)
            return ISLE_GAME_INVALID_STATE;
        const auto* issued=cart->source()->current_event;
        if(!issued || node!=&issued->node || event!=issued->event ||
            std::bit_cast<std::uint32_t>(value)!=std::bit_cast<std::uint32_t>(issued->argument) ||
            !cart->callback_matches(node->binding.object,adapter,source) ||
            !cart->issued_effect_node(node->binding,node->node))
            return ISLE_GAME_INVALID_ARGUMENT;
        return ISLE_GAME_OK;
    });
}
