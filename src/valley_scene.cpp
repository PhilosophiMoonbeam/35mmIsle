#include "valley_scene.hpp"
#include "menu_scene_bridge.h"
#include "environment_sound.hpp"
#include <limits>
namespace isle::valley {
namespace {
world::Block* block(Adapter& owner,IsleValleyWorldBlock* value) {
    auto* native=owner.actor_native.native_block(value);
    if (value && !native) throw std::invalid_argument("foreign or inconsistent Valley Source World view");
    return native;
}
constexpr std::array<actor::RetainedFieldFamily,1> retained{{{147,0x802d3100u,1u,0u}}};
IsleValleyLifecycleServices make_lifecycle_services() noexcept;
}
Adapter::Adapter() noexcept : actor_services(course::make_valley_actor_services()), lifecycle_services(make_lifecycle_services()) {}
IsleGameResult Adapter::prepare(void* owner,IsleContinuationDomain* domain,std::span<const std::byte> rom,std::int32_t argument) noexcept {
    if (!owner || !domain || entry || source.owner || actors.base.owner || course_state) return ISLE_GAME_INVALID_STATE;
    entry=static_cast<course::Lifecycle<Adapter>*>(owner);
    native.manager=&entry->manager(); native.photos=&entry->photos(); native.state=&entry->game_state();
    native.input=&entry->input(); native.audio=&entry->audio(); native.snapshot=&entry->snapshot();
    native.receipt=&entry->scene_join_receipt(); native.native_profile=entry->native_profile();
    if (rom.data()!=native.snapshot->canonical().data() || rom.size()!=isle::rom::us_size) return ISLE_GAME_INVALID_ARGUMENT;
    auto result=isle_valley_actor_prepare(&actors,this,reinterpret_cast<const std::uint8_t*>(rom.data()),rom.size());
    if (result==ISLE_GAME_OK) result=isle_valley_lifecycle_prepare(&source,this,domain,&actors,&lifecycle_services,reinterpret_cast<const std::uint8_t*>(rom.data()),rom.size(),argument);
    if (result!=ISLE_GAME_OK) return result;
    result=actor_native.prepare(*native.snapshot,actors);
    if (result!=ISLE_GAME_OK) return result;
    try {
        course_state.emplace(actors,source,actor_services);
        course_state->native_actor_mechanics=&actor_native;
        native.prepare_capacity(5,reference_fields.size());
        constexpr std::array<std::array<std::uint32_t,2>,4> ranges{{{0xab5860u,0xab5980u},{0xab5980u,0xab85e0u},{11341312u,11343424u},{11343424u,11391760u}}};
        for (std::size_t i=0;i<ranges.size();++i) native.assets[i]={ranges[i][0],ranges[i][1],rom.subspan(ranges[i][0],ranges[i][1]-ranges[i][0]),false};
        return ISLE_GAME_OK;
    } catch (...) { if (source.status==ISLE_GAME_OK) source.status=ISLE_GAME_INVALID_STATE; return source.status; }
}
course::NativeDescription Adapter::description() {
    if (!course_state || !native.requested || native.host) throw std::logic_error("typed scene description requires its one Source request");
    course::NativeDescription result;
    result.course_id=5; result.effects=menu::EffectGroup::valley; result.capacity=native.capacity;
    result.player={5,end,this,pause,this,{this,exit_block,movement},nullptr,0,func_802C6168_79F6F8};
    result.world={this,add,remove,change,nullptr,nullptr,nullptr};
    result.input={1u<<9,9,0,9,isle_course_scene_input_update};
    result.callbacks=actor_callbacks(); result.graphs=actor_graphs(); result.weighted_states=actor_weighted_states(); result.roots=actor_roots();
    result.retained_fields={retained}; result.reference_fields=reference_fields;
    result.extra_sprites=actor_native.resources().resources();
    result.extra_sprite_roots=actor_native.resources().roots();
    result.extra_sprite_banks=actor_native.resources().banks();
    return result;
}
actor::CourseContext Adapter::context() { if (!course_state) throw std::logic_error("typed CourseState not prepared"); return actor::CourseContext(*course_state); }
IsleGameResult Adapter::bind_scene(course::NativeHost& host) noexcept {
    if (native.host || !course_state || !host.closure_prepared() || !host.root() || !host.actors() || host.root()->bound_actors()!=host.actors()) return ISLE_GAME_INVALID_STATE;
    try { if (&host.actors()->course_context().valley()!=&*course_state) return ISLE_GAME_INVALID_STATE; } catch (...) { return ISLE_GAME_INVALID_STATE; }
    native.host=&host; course_state->native_host=&host;
    return actor_native.bind(host);
}
bool Adapter::active() const noexcept {
    return native.host && course_state && course_state->native_host==native.host && course_state->native_actor_mechanics==&actor_native && actor_native.host()==native.host && actors.base.phase==ISLE_COURSE_ACTOR_ACTIVE && actors.base.nativeGame==native.host->game() && actors.services==&actor_services && source.owner==this && source.actors==&actors;
}
IsleGameResult Adapter::validate_admission() const noexcept {
    if (!entry || source.owner!=this || actors.base.owner!=this || source.actors!=&actors || !source.domain || actors.base.course_id!=5 || !course_state || &course_state->actors!=&actors || &course_state->source!=&source || &course_state->actor_services!=&actor_services) return ISLE_GAME_INVALID_STATE;
    if (source.status!=ISLE_GAME_OK) return source.status;
    if (actors.base.status!=ISLE_GAME_OK) return actors.base.status;
    if (actors.base.phase==ISLE_COURSE_ACTOR_PREPARED) return course_state->native_actor_mechanics==&actor_native && !actors.base.nativeGame && !actors.services ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE;
    if (actors.base.phase==ISLE_COURSE_ACTOR_ACTIVE) return active() ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE;
    if (actors.base.phase==ISLE_COURSE_ACTOR_DETACHED) return !course_state->native_actor_mechanics && !actors.base.nativeGame && !actors.services && native.joined()==ISLE_GAME_OK ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE;
    return ISLE_GAME_INVALID_STATE;
}
IsleGameResult Adapter::activate(course::NativeHost& host) noexcept {
    if (native.host!=&host || !course_state || course_state->native_host!=&host || !host.closure_prepared() || source_status()!=ISLE_GAME_OK) return ISLE_GAME_INVALID_STATE;
    // The genuine C activate performs proposed-Game/table admission and publishes
    // ACTIVE last. No Source scalar or PREPARED phase is forged here.
    return isle_valley_actor_activate(&actors,host.game(),&actor_services);
}
IsleGameResult Adapter::invoke_init() noexcept { return active() ? isle_valley_lifecycle_invoke_init(&source) : ISLE_GAME_INVALID_STATE; }
IsleGameResult Adapter::release_references(course::NativeHost& host) noexcept {
    if (native.host && native.host!=&host) return ISLE_GAME_INVALID_STATE;
    return native.release_references();
}
IsleGameResult Adapter::deactivate(course::NativeHost& host) noexcept {
    if (actors.base.phase==ISLE_COURSE_ACTOR_PREPARED) return !actors.base.nativeGame && !actors.services && !actors.base.nonwaiting_call_frame ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE;
    if (actors.base.phase==ISLE_COURSE_ACTOR_DETACHED) return !actors.base.nativeGame && !actors.services ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE;
    if (actors.base.phase!=ISLE_COURSE_ACTOR_ACTIVE || native.host!=&host || !host.detach_ready()) return ISLE_GAME_INVALID_STATE;
    return isle_valley_actor_deactivate(&actors);
}
void Adapter::clear_scene() noexcept {
    actor_native.clear();
    if (course_state) { course_state->native_host=nullptr; course_state->native_actor_mechanics=nullptr; }
    native.host=nullptr; native.helper=nullptr; source.screen.depth_buffer=nullptr;
}
IsleGameResult Adapter::source_status() const noexcept { return source.status!=ISLE_GAME_OK ? source.status : actors.base.status; }
bool Adapter::requested() const noexcept { return source.scene_requested!=0; }
bool Adapter::completed() const noexcept { return source.completed!=0; }
bool Adapter::completion_ready() const noexcept { return source.scene_completion_ready!=0; }
const Adapter::SceneSetup& Adapter::setup() const noexcept { return source.scene_setup; }
IsleGameResult Adapter::complete_scene(const course::SceneJoinReceipt& receipt,IsleGameResult result) noexcept {
    if (&receipt!=native.receipt || !receipt.joined() || native.joined()!=ISLE_GAME_OK) return ISLE_GAME_INVALID_STATE;
    return isle_valley_lifecycle_complete_scene(&source,result);
}
IsleGameResult Adapter::native_context(IsleGame* game,IsleGameObject* object,LifecycleSource** output) noexcept {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT; *output=nullptr;
    if (!game) return ISLE_GAME_INVALID_ARGUMENT;
    IslePlayerSource* player=nullptr; auto result=isle_player_source_context(game,&player);
    if (result!=ISLE_GAME_OK) return result;
    if (!player || !player->native_root || player->game!=game) return ISLE_GAME_INVALID_STATE;
    auto& root=*static_cast<isle::player::Root*>(player->native_root); auto* owner=root.bound_actors();
    if (&root.source()!=player || !owner || &owner->player_source()!=player || owner->course_context().course_id()!=5) return ISLE_GAME_INVALID_STATE;
    try {
        auto& selected=owner->course_context().valley();
        if (selected.source.actors!=&selected.actors || selected.source.owner!=selected.actors.base.owner || !selected.source.owner) return ISLE_GAME_INVALID_STATE;
        auto& self=*static_cast<Adapter*>(selected.source.owner);
        if (&self.source!=&selected.source || &self.actors!=&selected.actors || !self.course_state || &*self.course_state!=&selected || &self.actor_services!=&selected.actor_services || !self.active() || self.native.host->root()!=&root || self.native.host->actors()!=owner || (object && !self.native.receiver(object))) return ISLE_GAME_INVALID_STATE;
        *output=&selected.source; return ISLE_GAME_OK;
    } catch (...) { return ISLE_GAME_INVALID_STATE; }
}
bool Adapter::add(void* user,world::Block& first,world::Block& second) noexcept {
    auto& a=*static_cast<Adapter*>(user);
    auto* first_view=a.actor_native.source_block(&first); auto* second_view=a.actor_native.source_block(&second);
    return first_view && second_view && isle_valley_lifecycle_world_add(&a.source,first_view,second_view)==ISLE_GAME_OK;
}
bool Adapter::change(void* user,world::Block& first,world::Block& second) noexcept {
    auto& a=*static_cast<Adapter*>(user);
    auto* first_view=a.actor_native.source_block(&first); auto* second_view=a.actor_native.source_block(&second);
    return first_view && second_view && isle_valley_lifecycle_world_change(&a.source,first_view,second_view)==ISLE_GAME_OK;
}
bool Adapter::remove(void* user,world::Block& value) noexcept {
    auto& a=*static_cast<Adapter*>(user); auto* view=a.actor_native.source_block(&value);
    return view && isle_valley_lifecycle_world_remove(&a.source,view)==ISLE_GAME_OK;
}
bool Adapter::exit_block(void* user,world::Block& value) noexcept {
    auto& a=*static_cast<Adapter*>(user); auto* view=a.actor_native.source_block(&value);
    return view && isle_valley_lifecycle_exit_block(&a.source,view)==ISLE_GAME_OK;
}
bool Adapter::movement(void* user,std::size_t index,float fraction) noexcept { auto& a=*static_cast<Adapter*>(user); return index<=std::size_t(INT32_MAX) && isle_valley_lifecycle_invoke_movement(&a.source,std::int32_t(index),fraction)==ISLE_GAME_OK; }
IsleGameResult Adapter::end(void* user,std::int32_t reason) noexcept { auto& a=*static_cast<Adapter*>(user); return isle_valley_lifecycle_invoke_end(&a.source,reason); }
IsleGameResult Adapter::pause(void* user,std::int32_t reason) noexcept { auto& a=*static_cast<Adapter*>(user); return isle_valley_lifecycle_invoke_pause(&a.source,reason); }
IsleGameResult Adapter::block_change(IsleCourseActorBase* base,std::uint32_t identity,IsleGameObject* object,const IsleGameVec3* previous,const IsleGameVec3* current) noexcept {
    if (!base || !object || !previous || !current) return ISLE_GAME_INVALID_ARGUMENT;
    int policy;
    switch (identity) { case 0x80363dbcu: policy=0; break; case 0x80363eb4u: policy=1; break; case 0x80362d2cu: policy=2; break; default: if (identity==0x802cc8e0u) return isle_course_scene_invoke_block(base,func_802CC8E0_7A5E70,object,previous,current); return ISLE_GAME_INVALID_ARGUMENT; }
    return isle_meowth_source_call_change_block(object,previous->x,previous->y,previous->z,current->x,current->y,current->z,policy);
}
IsleGameResult Adapter::block_remove(IsleCourseActorBase* base,std::uint32_t identity,IsleGameObject* object) noexcept {
    if (!base || !object || base->course_id!=5) return ISLE_GAME_INVALID_ARGUMENT;
    LifecycleSource* source=nullptr; auto result=native_context(base->nativeGame,object,&source); if (result!=ISLE_GAME_OK) return result;
    auto& self=*static_cast<Adapter*>(source->owner);
    if (&self.actors.base!=base) return ISLE_GAME_INVALID_STATE;
    for (const auto& callback:actor_callbacks()) if (callback.identity==identity) return isle_course_scene_invoke_object(base,callback.function,object);
    return ISLE_GAME_INVALID_ARGUMENT;
}
namespace {
IsleValleyLifecycleServices make_lifecycle_services() noexcept {
    return {
        .intervals=+[](IsleValleyLifecycleSource* p0,uint16_t p1,uint16_t p2) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.entry->intervals(p1,p2); });
        },
        .near_clipping=+[](IsleValleyLifecycleSource* p0,int32_t p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (p1!=0 && p1!=1) return ISLE_GAME_INVALID_ARGUMENT; return a.entry->near_clipping(p1!=0); });
        },
        .photo_count=+[](IsleValleyLifecycleSource* p0,int32_t* p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!p1 || a.native.joined()!=ISLE_GAME_OK) return ISLE_GAME_INVALID_STATE; *p1=a.native.photos->count(); return ISLE_GAME_OK; });
        },
        .height_map=+[](IsleValleyLifecycleSource* p0,uint32_t p1,int32_t* p2) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!p2) return ISLE_GAME_INVALID_ARGUMENT; auto result=a.native.map(p1,false); if (result==ISLE_GAME_OK) *p2=1; return result; });
        },
        .world_create=+[](IsleValleyLifecycleSource* p0,uint32_t p1,int32_t p2,int32_t p3,int32_t p4,int32_t p5,int32_t p6,IsleValleyWorldPair p7,IsleValleyWorldSingle p8,IsleValleyWorldPair p9,int32_t* p10) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (p7!=func_802C5C20_79F1B0 || p8!=func_802C5C68_79F1F8 || p9!=func_802C5C44_79F1D4) return ISLE_GAME_INVALID_ARGUMENT; if (!p10) return ISLE_GAME_INVALID_ARGUMENT; *p10=0; auto result=a.native.world(p1,p2,p3,p4,p5,p6); if (result==ISLE_GAME_OK) *p10=1; return result; });
        },
        .sky_follow=+[](IsleValleyLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.native.host->sky_follow() : ISLE_GAME_INVALID_STATE; });
        },
        .pokemon_registry=+[](IsleValleyLifecycleSource* p0,int32_t p1,int32_t p2,int32_t p3,int32_t p4) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.native.registry(p1,p2,p3,p4); });
        },
        .pokemon_add=+[](IsleValleyLifecycleSource* p0,IsleValleyWorldBlock* p1,IsleValleyWorldBlock* p2,uint32_t p3) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!a.active()) return ISLE_GAME_INVALID_STATE; if (p3!=0x802d282cu) return ISLE_GAME_INVALID_ARGUMENT; return a.native.pokemon_add(block(a,p1),block(a,p2),0x802d282cu,20); });
        },
        .pokemon_change=+[](IsleValleyLifecycleSource* p0,IsleValleyWorldBlock* p1,IsleValleyWorldBlock* p2,uint32_t p3) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!a.active()) return ISLE_GAME_INVALID_STATE; if (p3!=0x802d282cu) return ISLE_GAME_INVALID_ARGUMENT; return a.native.pokemon_change(block(a,p1),block(a,p2),0x802d282cu,20,&a.actors.base,Adapter::block_change); });
        },
        .pokemon_remove=+[](IsleValleyLifecycleSource* p0,IsleValleyWorldBlock* p1,uint32_t p2) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!a.active()) return ISLE_GAME_INVALID_STATE; if (p2!=0x802d282cu) return ISLE_GAME_INVALID_ARGUMENT; return a.native.pokemon_remove(block(a,p1),0x802d282cu,20,&a.actors.base,Adapter::block_remove); });
        },
        .items_displace=+[](IsleValleyLifecycleSource* p0,float p1,float p2,float p3) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() && a.native.host->items_owner()->displace({p1,p2,p3}) ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE; });
        },
        .transition_end=isle_course_scene_valley_transition_end,
        .detector_disable=+[](IsleValleyLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.native.host->detector_enabled(false) : ISLE_GAME_INVALID_STATE; });
        },
        .environment_cleanup=+[](IsleValleyLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.native.host->environment_cleanup() : ISLE_GAME_INVALID_STATE; });
        },
        .world_destroy=+[](IsleValleyLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.native.host->destroy_world() : ISLE_GAME_INVALID_STATE; });
        },
        .world_cleanup=+[](IsleValleyLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? isle_course_scene_control(a.native.player(),isle_player_func_80356FBC_4F73CC) : ISLE_GAME_INVALID_STATE; });
        },
        .camera_remove=+[](IsleValleyLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() && a.native.host->renderer() ? a.native.host->renderer()->remove_main() : ISLE_GAME_INVALID_STATE; });
        },
        .scene_exit=+[](IsleValleyLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.native.host->exit_scene() : ISLE_GAME_INVALID_STATE; });
        },
        .depth_allocate=+[](IsleValleyLifecycleSource* p0,size_t p1,size_t p2,void** p3) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (p1>UINT32_MAX || p2>UINT32_MAX) return ISLE_GAME_INVALID_ARGUMENT; return a.active() ? a.native.host->allocate_depth(std::uint32_t(p1),std::uint32_t(p2),p3) : ISLE_GAME_INVALID_STATE; });
        },
        .screen_apply=+[](IsleValleyLifecycleSource* p0,const IsleValleyLifecycleScreen* p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!a.native.setup() || p1!=&a.source.screen) return ISLE_GAME_INVALID_ARGUMENT; return a.native.host->apply_screen({p1->width,p1->height,p1->flags,{p1->framebuffer_identity[0],p1->framebuffer_identity[1],p1->framebuffer_identity[2]}},p1->depth_buffer); });
        },
        .effect_banks_reset=+[](IsleValleyLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!a.native.setup()) return ISLE_GAME_INVALID_STATE; for (std::size_t i=0;i<menu::EffectRuntime::bank_count;++i) { auto result=a.native.host->reset_effect_bank(i); if (result!=ISLE_GAME_OK) return result; } return ISLE_GAME_OK; });
        },
        .game_state_reset=+[](IsleValleyLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.native.setup() ? a.native.host->reset_photo_roll() : ISLE_GAME_INVALID_STATE; });
        },
        .pokemons_init=+[](IsleValleyLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!a.native.setup()) return ISLE_GAME_INVALID_STATE; if (!a.native.helper) a.native.helper=a.native.host->reference_owner(); IsleGameObjectMetadata metadata{}; if (!a.native.helper || isle_game_object_metadata(a.native.host->game(),a.native.helper,&metadata)!=ISLE_GAME_OK || metadata.id!=0 || metadata.update_link!=0 || metadata.priority!=0x80000000u) return ISLE_GAME_INVALID_STATE; return a.native.host->initialize_collisions(a.native.helper); });
        },
        .background_color=+[](IsleValleyLifecycleSource* p0,uint8_t* p1,uint8_t* p2,uint8_t* p3) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.native.background(p1,p2,p3); });
        },
        .cameras_create=+[](IsleValleyLifecycleSource* p0,int32_t p1,IsleGameObject** p2) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!p2) return ISLE_GAME_INVALID_ARGUMENT; *p2=nullptr; if (!a.native.setup()) return ISLE_GAME_INVALID_STATE; auto result=a.native.host->create_cameras(std::uint32_t(p1)); if (result==ISLE_GAME_OK) *p2=a.native.host->renderer()->ui_camera_object(); return result; });
        },
        .ui_init=+[](IsleValleyLifecycleSource* p0,IsleValleyWorldSingle p1,IsleValleyMovementCallback p2,IsleGameSourceCallback p3,uint8_t p4,IsleValleyItemCollisionCallback p5,IsleGameObject** p6) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (p1!=func_802C5DD4_79F364 || p2!=func_802C6B30_7A00C0 || p3 || p4 || p5!=func_802C6168_79F6F8 || !p6) return ISLE_GAME_INVALID_ARGUMENT; return a.native.initialize_ui(p6); });
        },
        .end_callback=+[](IsleValleyLifecycleSource* p0,IsleValleyEndCallback p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!a.native.setup() || p1!=func_802C5D18_79F2A8 || a.native.end_bound || a.native.player()->EndLevelCb!=Adapter::end || a.native.player()->end_level_user!=&a) return ISLE_GAME_INVALID_ARGUMENT; a.native.end_bound=true; return ISLE_GAME_OK; });
        },
        .pause_callback=+[](IsleValleyLifecycleSource* p0,IsleValleyEndCallback p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!a.native.setup() || p1!=func_802C5DCC_79F35C || a.native.pause_bound || a.native.player()->PauseCb!=Adapter::pause || a.native.player()->pause_user!=&a) return ISLE_GAME_INVALID_ARGUMENT; a.native.pause_bound=true; return ISLE_GAME_OK; });
        },
        .environment_init=+[](IsleValleyLifecycleSource* p0,const IsleValleyEnvironment* p1,size_t p2) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (p1!=a.source.environment) return ISLE_GAME_INVALID_ARGUMENT; return a.native.environment(p1,p2,58,+[](const void* p,std::size_t n){return static_cast<const IsleValleyEnvironment*>(p)[n].sound;},+[](const void* p,std::size_t n){return static_cast<const IsleValleyEnvironment*>(p)[n].pitch_modifier;},+[](const void* p,std::size_t n){return static_cast<const IsleValleyEnvironment*>(p)[n].hearing_range;}); });
        },
        .asset_load=+[](IsleValleyLifecycleSource* p0,uint32_t p1,uint32_t p2,void** p3) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!p3) return ISLE_GAME_INVALID_ARGUMENT; *p3=nullptr; for (std::size_t i=0;i<4;++i) if (a.native.assets[i].first==p1 && a.native.assets[i].end==p2) { course::SceneAsset* asset=nullptr; auto result=a.native.asset(i,p1,p2,&asset); if (result==ISLE_GAME_OK) *p3=reinterpret_cast<void*>(asset); return result; } return ISLE_GAME_INVALID_ARGUMENT; });
        },
        .effect_bank=+[](IsleValleyLifecycleSource* p0,int32_t p1,void* p2,void* p3) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.native.bank(p1,reinterpret_cast<const course::SceneAsset*>(p2),reinterpret_cast<const course::SceneAsset*>(p3),menu::EffectGroup::valley); });
        },
        .effect_camera=+[](IsleValleyLifecycleSource* p0,int32_t p1,int32_t p2,IsleValleyCamera* p3,IsleGameObject** p4) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!a.active()) return ISLE_GAME_INVALID_STATE; IsleGameObject* object=nullptr; auto result=a.actor_services.camera_object(&a.actors,p3,&object); if (result!=ISLE_GAME_OK) return result; IsleGameCamera* camera=nullptr; result=isle_game_object_camera(a.native.host->game(),object,&camera); return result==ISLE_GAME_OK ? a.native.fx_camera(p1,p2,camera,p4) : result; });
        },
        .effects_allocate=+[](IsleValleyLifecycleSource* p0,int32_t p1,void** p2) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!p2) return ISLE_GAME_INVALID_ARGUMENT; *p2=nullptr; IsleGameObject* object=nullptr; auto result=a.native.fx_pool(p1,&object); if (result==ISLE_GAME_OK) *p2=object; return result; });
        },
        .effects_dither=+[](IsleValleyLifecycleSource* p0,uint32_t p1,uint32_t p2) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.native.dither(std::uint32_t(p1),std::uint32_t(p2)); });
        },
        .detector_create=+[](IsleValleyLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.native.setup() ? a.native.host->initialize_detector(a.native.helper) : ISLE_GAME_INVALID_STATE; });
        },
        .detector_enable=+[](IsleValleyLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.native.host->detector_enabled(true) : ISLE_GAME_INVALID_STATE; });
        },
        .intro_initialize=+[](IsleValleyLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? isle_course_scene_control(a.native.player(),isle_player_func_803570B0_4F74C0) : ISLE_GAME_INVALID_STATE; });
        },
        .intro_object=+[](IsleValleyLifecycleSource* p0,IsleGameObject* p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() && a.native.receiver(p1) ? isle_course_scene_object_control(a.native.player(),isle_player_func_80357120_4F7530,p1) : ISLE_GAME_INVALID_STATE; });
        },
        .intro_finish=+[](IsleValleyLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? isle_course_scene_control(a.native.player(),isle_player_func_803572B0_4F76C0) : ISLE_GAME_INVALID_STATE; });
        },
        .intro_prepare=+[](IsleValleyLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? isle_course_scene_control(a.native.player(),isle_player_func_803571C4_4F75D4) : ISLE_GAME_INVALID_STATE; });
        },
        .intro_stop=+[](IsleValleyLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? isle_course_scene_control(a.native.player(),isle_player_func_80357170_4F7580) : ISLE_GAME_INVALID_STATE; });
        },
        .animation_callback=+[](IsleValleyLifecycleSource* p0,IsleGameObject* p1,IsleValleyLifecycleAnimationCallback p2) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!a.active() || p2!=func_802C6630_79FBC0 || !a.native.host->cart()->instance(p1)) return ISLE_GAME_INVALID_ARGUMENT; return a.native.host->cart()->set_callback(p1,isle_course_scene_valley_cart_event,&a.source) ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE; });
        },
        .effect_camera_publish=+[](IsleValleyLifecycleSource* p0,IsleGameObject* p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.native.fx_publish(0x80382d10u,p1); });
        },
        .effects_publish=+[](IsleValleyLifecycleSource* p0,void* p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.native.fx_publish(0x80382d14u,static_cast<IsleGameObject*>(p1)); });
        },
    };
}
}
} // namespace isle::valley
extern "C" IsleGameResult isle_valley_lifecycle_native_context(IsleGame* game,IsleGameObject* object,IsleValleyLifecycleSource** output) { return isle::valley::Adapter::native_context(game,object,output); }
extern "C" IsleGameResult isle_valley_lifecycle_native_admission(const IsleValleyLifecycleSource* source) {
    return isle::valley::Adapter::invoke(const_cast<IsleValleyLifecycleSource*>(source),[](isle::valley::Adapter& a) { return a.source.services==&a.lifecycle_services ? a.validate_admission() : ISLE_GAME_INVALID_STATE; });
}
extern "C" IsleGameResult isle_valley_lifecycle_native_request_scene(IsleValleyLifecycleSource* source,const IsleValleyLifecycleSceneSetup* setup) {
    return isle::valley::Adapter::invoke(source,[&](isle::valley::Adapter& a) { return a.actors.base.phase==ISLE_COURSE_ACTOR_PREPARED && a.source.scene_setup.heap_size==0x13f110u ? a.native.request(setup,&a.source.scene_setup) : ISLE_GAME_INVALID_STATE; });
}
extern "C" IsleGameResult isle_valley_lifecycle_native_scene_joined(IsleValleyLifecycleSource* source) { return isle::valley::Adapter::invoke(source,[](isle::valley::Adapter& a) { return a.native.joined(); }); }
extern "C" IsleGameResult isle_valley_lifecycle_native_or_notice(IsleValleyLifecycleSource* source,uint32_t flags) {
    if (!source || !source->domain) return ISLE_GAME_INVALID_ARGUMENT;
    if (isle_continuation_domain_check_access(source->domain)!=ISLE_CONT_OK)
        return ISLE_GAME_INVALID_STATE;
    return isle::valley::Adapter::invoke(source,[&](isle::valley::Adapter& a) {
        if (!a.native.state) return ISLE_GAME_INVALID_STATE;
        a.native.state->add_scene_notices(flags);
        return ISLE_GAME_OK;
    });
}
extern "C" IsleGameResult isle_course_scene_valley_owner_transition(IsleValleyLifecycleSource* source,int32_t reason) {
    return isle::valley::Adapter::invoke(source,[&](isle::valley::Adapter& a) { return a.active() ? a.native.manager->transition_end(reason) : ISLE_GAME_INVALID_STATE; });
}
