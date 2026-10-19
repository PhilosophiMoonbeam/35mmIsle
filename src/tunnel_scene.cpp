#include "tunnel_scene.hpp"
#include "menu_scene_bridge.h"
#include "environment_sound.hpp"
#include <limits>
namespace isle::tunnel {
namespace {
IsleTunnelLifecycleBlock* block(world::Block* value) noexcept { return reinterpret_cast<IsleTunnelLifecycleBlock*>(value); }
world::Block* block(IsleTunnelLifecycleBlock* value) noexcept { return reinterpret_cast<world::Block*>(value); }
constexpr std::array<actor::RetainedFieldFamily,1> retained{{{81,0x802ee660u,1u,0u}}};
}
Adapter::Adapter() noexcept : actor_services(course::make_tunnel_actor_services()) {}
IsleGameResult Adapter::prepare(void* owner,IsleContinuationDomain* domain,std::span<const std::byte> rom,std::int32_t argument) noexcept {
    if (!owner || !domain || entry || source.owner || actors.base.owner || course_state) return ISLE_GAME_INVALID_STATE;
    entry=static_cast<course::Lifecycle<Adapter>*>(owner);
    native.manager=&entry->manager(); native.photos=&entry->photos(); native.state=&entry->game_state();
    native.input=&entry->input(); native.audio=&entry->audio(); native.snapshot=&entry->snapshot();
    native.receipt=&entry->scene_join_receipt(); native.native_profile=entry->native_profile();
    if (rom.data()!=native.snapshot->canonical().data() || rom.size()!=isle::rom::us_size) return ISLE_GAME_INVALID_ARGUMENT;
    auto result=egg_geometry.prepare(*native.snapshot,actors,this);
    if (result==ISLE_GAME_OK) result=isle_tunnel_actor_prepare(&actors,this,reinterpret_cast<const std::uint8_t*>(rom.data()),rom.size(),egg_geometry.preparation());
    if (result==ISLE_GAME_OK) result=isle_tunnel_lifecycle_prepare(&source,this,domain,&actors,reinterpret_cast<const std::uint8_t*>(rom.data()),rom.size(),argument);
    if (result!=ISLE_GAME_OK) return result;
    try {
        course_state.emplace(actors,source,actor_services);
        if (!egg_geometry.source_matches()) return ISLE_GAME_INVALID_STATE;
        course_state->native_egg_geometry=&egg_geometry;
        native.prepare_capacity(1,reference_fields.size());
        constexpr std::array<std::array<std::uint32_t,2>,4> ranges{{{0xab5860u,0xab5980u},{0xab5980u,0xab85e0u},{11265952u,11267024u},{11267024u,11298960u}}};
        for (std::size_t i=0;i<ranges.size();++i) native.assets[i]={ranges[i][0],ranges[i][1],rom.subspan(ranges[i][0],ranges[i][1]-ranges[i][0]),false};
        native.assets[4]={0xab1470u,0xab5860u,rom.subspan(0xab1470u,0x43f0u),false};
        return ISLE_GAME_OK;
    } catch (...) { if (source.status==ISLE_GAME_OK) source.status=ISLE_GAME_INVALID_STATE; return source.status; }
}
course::NativeDescription Adapter::description() {
    if (!course_state || !native.requested || native.host) throw std::logic_error("typed scene description requires its one Source request");
    course::NativeDescription result;
    result.course_id=1; result.effects=menu::EffectGroup::tunnel; result.capacity=native.capacity;
    result.player={1,end,this,pause,this,{this,exit_block,movement},nullptr,0,isle_course_scene_tunnel_item};
    result.world={this,add,remove,change,nullptr,nullptr,nullptr};
    result.input={1u<<9,9,0,9,isle_tunnel_lifecycle_input_update};
    result.initial_helper={0,0x80000000u,0,isle_tunnel_lifecycle_func_802E28A4_5DF974};
    result.callbacks=actor_callbacks(); result.graphs=actor_graphs(); result.weighted_states=actor_weighted_states(); result.roots=actor_roots();
    result.retained_fields={retained}; result.reference_fields=reference_fields;
    result.source_vertices=egg_geometry.publications();
    return result;
}
actor::CourseContext Adapter::context() { if (!course_state) throw std::logic_error("typed CourseState not prepared"); return actor::CourseContext(*course_state); }
IsleGameResult Adapter::bind_scene(course::NativeHost& host) noexcept {
    if (native.host || !course_state || !host.closure_prepared() || !host.root() || !host.actors() || host.root()->bound_actors()!=host.actors()) return ISLE_GAME_INVALID_STATE;
    try { if (&host.actors()->course_context().tunnel()!=&*course_state) return ISLE_GAME_INVALID_STATE; } catch (...) { return ISLE_GAME_INVALID_STATE; }
    const auto geometry_result=egg_geometry.attach(host);
    if (geometry_result!=ISLE_GAME_OK) return geometry_result;
    native.host=&host; course_state->native_host=&host;
    source.player=&host.root()->source();
    return ISLE_GAME_OK;
}
bool Adapter::active() const noexcept {
    return native.host && course_state && course_state->native_host==native.host && actors.base.phase==ISLE_COURSE_ACTOR_ACTIVE && actors.base.nativeGame==native.host->game() && actors.services==&actor_services && source.owner==this && source.actors==&actors;
}
IsleGameResult Adapter::validate_admission() const noexcept {
    if (!entry || source.owner!=this || actors.base.owner!=this || source.actors!=&actors || !source.domain || actors.base.course_id!=1 || !course_state || &course_state->actors!=&actors || &course_state->source!=&source || &course_state->actor_services!=&actor_services || course_state->native_egg_geometry!=&egg_geometry || !egg_geometry.source_matches()) return ISLE_GAME_INVALID_STATE;
    if (source.status!=ISLE_GAME_OK) return source.status;
    if (actors.base.status!=ISLE_GAME_OK) return actors.base.status;
    if (actors.base.phase==ISLE_COURSE_ACTOR_PREPARED) return !actors.base.nativeGame && !actors.services ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE;
    if (actors.base.phase==ISLE_COURSE_ACTOR_ACTIVE) return active() ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE;
    if (actors.base.phase==ISLE_COURSE_ACTOR_DETACHED) return !actors.base.nativeGame && !actors.services && native.joined()==ISLE_GAME_OK ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE;
    return ISLE_GAME_INVALID_STATE;
}
IsleGameResult Adapter::activate(course::NativeHost& host) noexcept {
    if (native.host!=&host || !course_state || course_state->native_host!=&host || !host.closure_prepared() || source_status()!=ISLE_GAME_OK) return ISLE_GAME_INVALID_STATE;
    // The genuine C activate performs proposed-Game/table admission and publishes
    // ACTIVE last. No Source scalar or PREPARED phase is forged here.
    return isle_tunnel_actor_activate(&actors,host.game(),&actor_services);
}
IsleGameResult Adapter::invoke_init() noexcept { return active() ? isle_tunnel_lifecycle_invoke_init(&source) : ISLE_GAME_INVALID_STATE; }
IsleGameResult Adapter::release_references(course::NativeHost& host) noexcept {
    if (native.host && native.host!=&host) return ISLE_GAME_INVALID_STATE;
    return native.release_references();
}
IsleGameResult Adapter::deactivate(course::NativeHost& host) noexcept {
    if (actors.base.phase==ISLE_COURSE_ACTOR_PREPARED) return !actors.base.nativeGame && !actors.services && !actors.base.nonwaiting_call_frame ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE;
    if (actors.base.phase==ISLE_COURSE_ACTOR_DETACHED) return !actors.base.nativeGame && !actors.services ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE;
    if (actors.base.phase!=ISLE_COURSE_ACTOR_ACTIVE || native.host!=&host || !host.detach_ready()) return ISLE_GAME_INVALID_STATE;
    return isle_tunnel_actor_deactivate(&actors);
}
void Adapter::clear_scene() noexcept {
    if (course_state) course_state->native_host=nullptr;
    native.host=nullptr; native.helper=nullptr; source.screen.depth_buffer=nullptr;
    source.player=nullptr;
}
IsleGameResult Adapter::source_status() const noexcept { return source.status!=ISLE_GAME_OK ? source.status : actors.base.status; }
bool Adapter::requested() const noexcept { return source.scene_requested!=0; }
bool Adapter::completed() const noexcept { return source.completed!=0; }
bool Adapter::completion_ready() const noexcept { return source.scene_completion_ready!=0; }
const Adapter::SceneSetup& Adapter::setup() const noexcept { return source.scene_setup; }
IsleGameResult Adapter::complete_scene(const course::SceneJoinReceipt& receipt,IsleGameResult result) noexcept {
    if (&receipt!=native.receipt || !receipt.joined() || native.joined()!=ISLE_GAME_OK) return ISLE_GAME_INVALID_STATE;
    return isle_tunnel_lifecycle_complete_scene(&source,result);
}
IsleGameResult Adapter::native_context(IsleGame* game,IsleGameObject* object,LifecycleSource** output) noexcept {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT; *output=nullptr;
    if (!game) return ISLE_GAME_INVALID_ARGUMENT;
    IslePlayerSource* player=nullptr; auto result=isle_player_source_context(game,&player);
    if (result!=ISLE_GAME_OK) return result;
    if (!player || !player->native_root || player->game!=game) return ISLE_GAME_INVALID_STATE;
    auto& root=*static_cast<isle::player::Root*>(player->native_root); auto* owner=root.bound_actors();
    if (&root.source()!=player || !owner || &owner->player_source()!=player || owner->course_context().course_id()!=1) return ISLE_GAME_INVALID_STATE;
    try {
        auto& selected=owner->course_context().tunnel();
        if (selected.source.actors!=&selected.actors || selected.source.owner!=selected.actors.base.owner || !selected.source.owner) return ISLE_GAME_INVALID_STATE;
        auto& self=*static_cast<Adapter*>(selected.source.owner);
        if (&self.source!=&selected.source || &self.actors!=&selected.actors || !self.course_state || &*self.course_state!=&selected || &self.actor_services!=&selected.actor_services || !self.active() || self.native.host->root()!=&root || self.native.host->actors()!=owner || (object && !self.native.receiver(object))) return ISLE_GAME_INVALID_STATE;
        *output=&selected.source; return ISLE_GAME_OK;
    } catch (...) { return ISLE_GAME_INVALID_STATE; }
}
bool Adapter::add(void* user,world::Block& first,world::Block& second) noexcept { auto& a=*static_cast<Adapter*>(user); return isle_tunnel_lifecycle_world_add(&a.source,block(&first),block(&second))==ISLE_GAME_OK; }
bool Adapter::change(void* user,world::Block& first,world::Block& second) noexcept { auto& a=*static_cast<Adapter*>(user); return isle_tunnel_lifecycle_world_change(&a.source,block(&first),block(&second))==ISLE_GAME_OK; }
bool Adapter::remove(void* user,world::Block& value) noexcept { auto& a=*static_cast<Adapter*>(user); return isle_tunnel_lifecycle_world_remove(&a.source,block(&value))==ISLE_GAME_OK; }
bool Adapter::exit_block(void* user,world::Block& value) noexcept { auto& a=*static_cast<Adapter*>(user); return isle_tunnel_lifecycle_exit_block(&a.source,block(&value))==ISLE_GAME_OK; }
bool Adapter::movement(void* user,std::size_t index,float fraction) noexcept { auto& a=*static_cast<Adapter*>(user); return index<=std::size_t(INT32_MAX) && isle_tunnel_lifecycle_movement(&a.source,std::int32_t(index),fraction)==ISLE_GAME_OK; }
IsleGameResult Adapter::end(void* user,std::int32_t reason) noexcept { auto& a=*static_cast<Adapter*>(user); return isle_tunnel_lifecycle_invoke_end(&a.source,reason); }
IsleGameResult Adapter::pause(void* user,std::int32_t reason) noexcept { auto& a=*static_cast<Adapter*>(user); return isle_tunnel_lifecycle_pause_event(&a.source,reason); }
IsleGameResult Adapter::block_change(IsleCourseActorBase* base,std::uint32_t identity,IsleGameObject* object,const IsleGameVec3* previous,const IsleGameVec3* current) noexcept {
    if (!base || !object || !previous || !current) return ISLE_GAME_INVALID_ARGUMENT;
    int policy;
    switch (identity) { case 0x80363dbcu: policy=0; break; case 0x80363eb4u: policy=1; break; case 0x80362d2cu: policy=2; break; default:  return ISLE_GAME_INVALID_ARGUMENT; }
    return isle_meowth_source_call_change_block(object,previous->x,previous->y,previous->z,current->x,current->y,current->z,policy);
}
IsleGameResult Adapter::block_remove(IsleCourseActorBase* base,std::uint32_t identity,IsleGameObject* object) noexcept {
    if (!base || !object || base->course_id!=1) return ISLE_GAME_INVALID_ARGUMENT;
    LifecycleSource* source=nullptr; auto result=native_context(base->nativeGame,object,&source); if (result!=ISLE_GAME_OK) return result;
    auto& self=*static_cast<Adapter*>(source->owner);
    if (&self.actors.base!=base) return ISLE_GAME_INVALID_STATE;
    for (const auto& callback:actor_callbacks()) if (callback.identity==identity) return isle_course_scene_invoke_object(base,callback.function,object);
    return ISLE_GAME_INVALID_ARGUMENT;
}
} // namespace isle::tunnel
extern "C" IsleGameResult isle_tunnel_lifecycle_native_context(IsleGame* p0,IsleGameObject* p1,IsleTunnelLifecycleSource** p2) { return isle::tunnel::Adapter::native_context(p0,p1,p2); }
extern "C" IsleGameResult isle_tunnel_lifecycle_native_request_scene(IsleTunnelLifecycleSource* p0,const IsleTunnelLifecycleSceneSetup* p1) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { if (a.actors.base.phase!=ISLE_COURSE_ACTOR_PREPARED || a.source.scene_setup.heap_size!=1403056u) return ISLE_GAME_INVALID_STATE; return a.native.request(p1,&a.source.scene_setup); });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_invalidate_input(IsleTunnelLifecycleSource* p0) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.native.host->invalidate_input(isle_menu_current_object(a.native.host->game())) : ISLE_GAME_INVALID_STATE; });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_scene_joined(IsleTunnelLifecycleSource* p0) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { return a.native.joined(); });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_intervals(IsleTunnelLifecycleSource* p0,uint16_t p1,uint16_t p2) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { return a.entry->intervals(p1,p2); });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_near_clipping(IsleTunnelLifecycleSource* p0,int32_t p1) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { if (p1!=0 && p1!=1) return ISLE_GAME_INVALID_ARGUMENT; return a.entry->near_clipping(p1!=0); });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_owner_transition_end(IsleTunnelLifecycleSource* p0,int32_t p1) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { return a.native.manager->transition_end(p1); });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_detector_disable(IsleTunnelLifecycleSource* p0) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.native.host->detector_enabled(false) : ISLE_GAME_INVALID_STATE; });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_environment_cleanup(IsleTunnelLifecycleSource* p0) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.native.host->environment_cleanup() : ISLE_GAME_INVALID_STATE; });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_stop_bgm(IsleTunnelLifecycleSource* p0) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { return course::SceneMechanics::audio_result(a.native.audio->stop_bgm()); });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_stop_all_sounds(IsleTunnelLifecycleSource* p0) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { return course::SceneMechanics::audio_result(a.native.audio->stop_sounds()); });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_destroy_world(IsleTunnelLifecycleSource* p0) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.native.host->destroy_world() : ISLE_GAME_INVALID_STATE; });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_remove_main_camera(IsleTunnelLifecycleSource* p0) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { return a.active() && a.native.host->renderer() ? a.native.host->renderer()->remove_main() : ISLE_GAME_INVALID_STATE; });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_exit_scene(IsleTunnelLifecycleSource* p0) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.native.host->exit_scene() : ISLE_GAME_INVALID_STATE; });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_create_world(IsleTunnelLifecycleSource* p0,uint32_t p1,int32_t p2,int32_t p3,int32_t p4,int32_t p5,int32_t p6,IsleTunnelLifecycleWorldPair p7,IsleTunnelLifecycleWorldSingle p8,IsleTunnelLifecycleWorldPair p9,void* p10) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { if (p7!=isle_tunnel_lifecycle_world_add || p8!=isle_tunnel_lifecycle_world_remove || p9!=isle_tunnel_lifecycle_world_change || p10!=&a.source) return ISLE_GAME_INVALID_ARGUMENT; return a.native.world(p1,p2,p3,p4,p5,p6); });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_sky_follow(IsleTunnelLifecycleSource* p0) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.native.host->sky_follow() : ISLE_GAME_INVALID_STATE; });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_pokemon_registry(IsleTunnelLifecycleSource* p0,int32_t p1,int32_t p2,int32_t p3,int32_t p4) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { return a.native.registry(p1,p2,p3,p4); });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_pokemon_change(IsleTunnelLifecycleSource* p0,IsleTunnelLifecycleBlock* p1,IsleTunnelLifecycleBlock* p2,uint32_t p3,const IsleTunnelLifecyclePokemonDef* p4,size_t p5) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { if (!a.active() || p3!=0x802edfacu || p4!=a.source.pokemon_defs || p5!=21 || !a.native.definitions(p4,p5,0x802edfacu)) return ISLE_GAME_INVALID_ARGUMENT; return a.native.pokemon_change(block(p1),block(p2),0x802edfacu,21,&a.actors.base,Adapter::block_change); });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_pokemon_remove(IsleTunnelLifecycleSource* p0,IsleTunnelLifecycleBlock* p1,uint32_t p2,const IsleTunnelLifecyclePokemonDef* p3,size_t p4) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { if (!a.active() || p2!=0x802edfacu || p3!=a.source.pokemon_defs || p4!=21 || !a.native.definitions(p3,p4,0x802edfacu)) return ISLE_GAME_INVALID_ARGUMENT; return a.native.pokemon_remove(block(p1),0x802edfacu,21,&a.actors.base,Adapter::block_remove); });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_block_next(IsleTunnelLifecycleSource* p0,IsleTunnelLifecycleBlock* p1,IsleTunnelLifecycleBlock** p2) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { if (!p2) return ISLE_GAME_INVALID_ARGUMENT; *p2=nullptr; world::Block* next=nullptr; auto result=a.native.block_next(block(p1),&next); if (result==ISLE_GAME_OK) *p2=block(next); return result; });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_block_position(IsleTunnelLifecycleSource* p0,IsleTunnelLifecycleBlock* p1,IsleGameVec3* p2) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { return a.native.block_position(block(p1),p2); });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_displace_items(IsleTunnelLifecycleSource* p0,float p1,float p2,float p3) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { return a.active() && a.native.host->items_owner()->displace({p1,p2,p3}) ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE; });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_depth_buffer(IsleTunnelLifecycleSource* p0,uint32_t p1,uint32_t p2,void** p3) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.native.host->allocate_depth(std::uint32_t(p1),std::uint32_t(p2),p3) : ISLE_GAME_INVALID_STATE; });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_screen(IsleTunnelLifecycleSource* p0,const IsleTunnelLifecycleScreen* p1) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { if (!a.native.setup() || p1!=&a.source.screen) return ISLE_GAME_INVALID_ARGUMENT; return a.native.host->apply_screen({p1->width,p1->height,p1->flags,{p1->framebuffer_identity[0],p1->framebuffer_identity[1],p1->framebuffer_identity[2]}},p1->depth_buffer); });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_fx_bank_count(IsleTunnelLifecycleSource* p0,size_t* p1) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { if (!p1 || !a.native.setup()) return ISLE_GAME_INVALID_ARGUMENT; *p1=menu::EffectRuntime::bank_count; return ISLE_GAME_OK; });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_fx_reset_bank(IsleTunnelLifecycleSource* p0,size_t p1) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { return a.native.setup() ? a.native.host->reset_effect_bank(p1) : ISLE_GAME_INVALID_STATE; });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_reset_photo_roll(IsleTunnelLifecycleSource* p0) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { return a.native.setup() ? a.native.host->reset_photo_roll() : ISLE_GAME_INVALID_STATE; });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_create_helper(IsleTunnelLifecycleSource* p0,int32_t p1,IsleGameSourceCallback p2,int32_t p3,uint32_t p4) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { if (p2!=isle_tunnel_lifecycle_func_802E28A4_5DF974) return ISLE_GAME_INVALID_ARGUMENT; IsleGameObject* object=nullptr; return a.native.helper_create(p1,p2,p3,p4,&object); });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_pokemons_init(IsleTunnelLifecycleSource* p0) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { if (!a.native.setup()) return ISLE_GAME_INVALID_STATE; if (!a.native.helper) a.native.helper=a.native.host->reference_owner(); IsleGameObjectMetadata metadata{}; if (!a.native.helper || isle_game_object_metadata(a.native.host->game(),a.native.helper,&metadata)!=ISLE_GAME_OK || metadata.id!=0 || metadata.update_link!=0 || metadata.priority!=0x80000000u) return ISLE_GAME_INVALID_STATE; return a.native.host->initialize_collisions(a.native.helper); });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_background(IsleTunnelLifecycleSource* p0,uint8_t* p1,uint8_t* p2,uint8_t* p3) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { return a.native.background(p1,p2,p3); });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_create_cameras(IsleTunnelLifecycleSource* p0,uint32_t p1) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { return a.native.setup() ? a.native.host->create_cameras(std::uint32_t(p1)) : ISLE_GAME_INVALID_STATE; });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_ui_bind(IsleTunnelLifecycleSource* p0,const IsleTunnelLifecycleUiCallbacks* p1) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { if (!a.native.setup() || !p1 || p1->user!=&a.source || p1->exit_block!=isle_tunnel_lifecycle_exit_block || p1->movement!=isle_tunnel_lifecycle_movement || p1->update_items || p1->update_items_kind || p1->collide_item!=isle_tunnel_lifecycle_collide_item || a.native.ui_bound || !a.native.host->camera()) return ISLE_GAME_INVALID_ARGUMENT; a.native.ui_bound=true; return ISLE_GAME_OK; });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_ui_initialized(IsleTunnelLifecycleSource* p0,IsleGameObject* p1) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { return a.native.setup() && a.native.ui_bound ? a.native.host->root()->complete_initialization(p1) : ISLE_GAME_INVALID_STATE; });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_environment_init(IsleTunnelLifecycleSource* p0,const IsleTunnelLifecycleEnvironment* p1,size_t p2) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { if (p1!=a.source.environment) return ISLE_GAME_INVALID_ARGUMENT; return a.native.environment(p1,p2,43,+[](const void* p,std::size_t n){return static_cast<const IsleTunnelLifecycleEnvironment*>(p)[n].sound;},+[](const void* p,std::size_t n){return static_cast<const IsleTunnelLifecycleEnvironment*>(p)[n].pitch_modifier;},+[](const void* p,std::size_t n){return static_cast<const IsleTunnelLifecycleEnvironment*>(p)[n].hearing_range;}); });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_play_sound(IsleTunnelLifecycleSource* p0,int32_t p1,int32_t p2,int32_t p3,float p4,int32_t p5,int32_t* p6) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { return a.native.sound(p1,p2,p3,p4,p5,p6); });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_stop_sound(IsleTunnelLifecycleSource* p0,uint32_t p1) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { return course::SceneMechanics::audio_result(a.native.audio->stop_sound(std::uint32_t(p1))); });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_sound_identity(IsleTunnelLifecycleSource* p0,uint32_t p1,int32_t* p2) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { if (!p2) return ISLE_GAME_INVALID_ARGUMENT; *p2=a.native.audio->playing_sound(std::uint32_t(p1)); return ISLE_GAME_OK; });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_load(IsleTunnelLifecycleSource* p0,IsleTunnelLifecycleAssetId p1,uint32_t p2,uint32_t p3,const IsleTunnelLifecycleAsset** p4) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { if (!p4) return ISLE_GAME_INVALID_ARGUMENT; *p4=nullptr; course::SceneAsset* asset=nullptr; auto result=a.native.asset(std::size_t(p1),p2,p3,&asset); if (result==ISLE_GAME_OK) *p4=reinterpret_cast<const IsleTunnelLifecycleAsset*>(asset); return result; });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_fx_setup_bank(IsleTunnelLifecycleSource* p0,int32_t p1,const IsleTunnelLifecycleAsset* p2,const IsleTunnelLifecycleAsset* p3) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { return a.native.bank(p1,reinterpret_cast<const course::SceneAsset*>(p2),reinterpret_cast<const course::SceneAsset*>(p3),menu::EffectGroup::tunnel); });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_fx_camera(IsleTunnelLifecycleSource* p0,int32_t p1,int32_t p2,IsleGameCamera* p3,IsleGameObject** p4) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { return a.native.fx_camera(p1,p2,p3,p4); });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_fx_effects(IsleTunnelLifecycleSource* p0,int32_t p1,IsleGameObject** p2) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { return a.native.fx_pool(p1,p2); });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_fx_dither(IsleTunnelLifecycleSource* p0,uint32_t p1,uint32_t p2) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { return a.native.dither(std::uint32_t(p1),std::uint32_t(p2)); });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_detector_create(IsleTunnelLifecycleSource* p0) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { return a.native.setup() ? a.native.host->initialize_detector(a.native.helper) : ISLE_GAME_INVALID_STATE; });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_detector_enable(IsleTunnelLifecycleSource* p0) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.native.host->detector_enabled(true) : ISLE_GAME_INVALID_STATE; });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_photo_count(IsleTunnelLifecycleSource* p0,int32_t* p1) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { if (!p1 || a.native.joined()!=ISLE_GAME_OK) return ISLE_GAME_INVALID_STATE; *p1=a.native.photos->count(); return ISLE_GAME_OK; });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_or_notice(IsleTunnelLifecycleSource* p0,uint32_t p1) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { a.native.state->add_scene_notices(std::uint32_t(p1)); return ISLE_GAME_OK; });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_camera_clip(IsleTunnelLifecycleSource* p0,uint32_t p1,const IslePlayerCameraAnimationClip** p2) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) {
        if (!p2) return ISLE_GAME_INVALID_ARGUMENT;
        *p2=nullptr;
        if (!a.active() || !a.native.host->camera()) return ISLE_GAME_INVALID_STATE;
        if (!p1) return ISLE_GAME_OK;
        const auto index=a.native.host->cart_resources().camera_animation(p1);
        if (index==menu::no_index) return ISLE_GAME_INVALID_ARGUMENT;
        for (const auto& clip:a.camera_clips) if (clip.owner && clip.index==index) { *p2=&clip; return ISLE_GAME_OK; }
        for (auto& clip:a.camera_clips) if (!clip.owner) {
            if (!a.native.host->camera()->clip(index,clip)) return ISLE_GAME_INVALID_STATE;
            *p2=&clip; return ISLE_GAME_OK;
        }
        return ISLE_GAME_EXHAUSTED;
    });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_cart_model_clip(IsleTunnelLifecycleSource* p0,uint32_t p1,const IslePlayerCartModelAnimation** p2) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { if (!p2) return ISLE_GAME_INVALID_ARGUMENT; *p2=nullptr; if (!a.active()) return ISLE_GAME_INVALID_STATE; if (!p1) return ISLE_GAME_OK; *p2=a.native.host->cart()->model_animation(p1); return *p2 ? ISLE_GAME_OK : ISLE_GAME_INVALID_ARGUMENT; });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_cart_material_clip(IsleTunnelLifecycleSource* p0,uint32_t p1,const IslePlayerCartMaterialAnimation** p2) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { if (!p2) return ISLE_GAME_INVALID_ARGUMENT; *p2=nullptr; if (!a.active()) return ISLE_GAME_INVALID_STATE; if (!p1) return ISLE_GAME_OK; *p2=a.native.host->cart()->material_animation(p1); return *p2 ? ISLE_GAME_OK : ISLE_GAME_INVALID_ARGUMENT; });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_camera_speed(IsleTunnelLifecycleSource* p0,IsleGameCamera* p1,float p2) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { return a.active() && a.native.host->camera() && p1==a.native.player()->camera && a.native.host->camera()->set_speed(p2) ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE; });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_create_effect(IsleTunnelLifecycleSource* p0,int32_t p1,int32_t p2,IsleTunnelLifecycleEffect** p3) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { if (!p3) return ISLE_GAME_INVALID_ARGUMENT; *p3=nullptr; course::SceneEffect* effect=nullptr; auto result=a.native.effect(p1,p2,&effect); if (result==ISLE_GAME_OK && effect) *p3=reinterpret_cast<IsleTunnelLifecycleEffect*>(effect); return result; });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_attach_effect(IsleTunnelLifecycleSource* p0,IsleTunnelLifecycleEffect* p1,const IslePlayerCartNode* p2) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { if (!a.active() || !p2 || !a.native.host->cart()->callback_matches(p2->binding.object,isle_tunnel_lifecycle_cart_event,&a.source)) return ISLE_GAME_INVALID_ARGUMENT; return a.native.attach(reinterpret_cast<course::SceneEffect*>(p1),p2); });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_store_cart(IsleTunnelLifecycleSource* p0,IsleGameObject* p1) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { if (!a.active() || (p1 && !a.native.host->cart()->instance(p1))) return ISLE_GAME_INVALID_ARGUMENT; return a.native.reference(&a.source.D_802F01E4_5ED2B4,p1); });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_retire_intro_cart(IsleTunnelLifecycleSource* p0,IsleGameObject* p1) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { return p1 && p1==a.source.D_802F01E4_5ED2B4 ? a.native.cart_retire(p1) : ISLE_GAME_INVALID_ARGUMENT; });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_release_cart_reference(IsleTunnelLifecycleSource* p0) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { if (!a.native.host || !a.native.host->services() || isle_game_require_idle(a.native.host->game())!=ISLE_GAME_OK || a.native.host->graphics_busy() || a.native.host->actors()->effects().effect_count()) return ISLE_GAME_INVALID_STATE; if (!a.source.D_802F01E4_5ED2B4) return ISLE_GAME_OK; if (!a.native.host->services()->source_owner_joined(a.source.D_802F01E4_5ED2B4)) return ISLE_GAME_INVALID_STATE; return a.native.reference(&a.source.D_802F01E4_5ED2B4,nullptr); });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_release_fx_references(IsleTunnelLifecycleSource* p0) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { return a.native.release_references(); });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_height_map(IsleTunnelLifecycleSource* p0,uint32_t p1) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { return a.native.map(p1,false); });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_ceiling_map(IsleTunnelLifecycleSource* p0,uint32_t p1) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { return a.native.map(p1,true); });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_pokemon_add(IsleTunnelLifecycleSource* p0,IsleTunnelLifecycleBlock* p1,IsleTunnelLifecycleBlock* p2,uint32_t p3,const IsleTunnelLifecyclePokemonDef* p4,size_t p5) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { if (!a.active() || p3!=0x802edfacu || p4!=a.source.pokemon_defs || p5!=21 || !a.native.definitions(p4,p5,0x802edfacu)) return ISLE_GAME_INVALID_ARGUMENT; return a.native.pokemon_add(block(p1),block(p2),0x802edfacu,21); });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_idle_enabled(IsleTunnelLifecycleSource* p0,int32_t* p1) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { if (!p1) return ISLE_GAME_INVALID_ARGUMENT; *p1=a.native.state->scene_latch(); return ISLE_GAME_OK; });
}
extern "C" IsleGameResult isle_tunnel_lifecycle_native_idle_script(IsleTunnelLifecycleSource* p0,const IsleTunnelLifecycleAsset* p1) {
    using namespace isle; using namespace isle::tunnel;
    return Adapter::invoke(p0,[&](Adapter& a) { if (!a.native.setup() || reinterpret_cast<const course::SceneAsset*>(p1)!=&a.native.assets[4] || !a.native.assets[4].loaded) return ISLE_GAME_INVALID_ARGUMENT; a.native.state->set_idle_script(&a.native.photos->tunnel_idle_script()); return ISLE_GAME_OK; });
}
