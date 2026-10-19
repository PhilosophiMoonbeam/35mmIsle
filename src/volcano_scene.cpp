#include "volcano_scene.hpp"
#include "menu_scene_bridge.h"
#include "environment_sound.hpp"
#include <limits>
extern "C" {
extern const IsleVolcanoEnvironment volcano_EnvSounds[49];
extern const IsleVolcanoDefinition volcano_PokemonDefs[21];
}
namespace isle::volcano {
namespace {
IsleVolcanoBlock* block(world::Block* value) noexcept { return reinterpret_cast<IsleVolcanoBlock*>(value); }
world::Block* block(IsleVolcanoBlock* value) noexcept { return reinterpret_cast<world::Block*>(value); }
constexpr std::array<actor::RetainedFieldFamily,1> retained{{{1026,0x802e2fd8u,2u,0u}}};
IsleVolcanoLifecycleServices make_lifecycle_services() noexcept;
}
Adapter::Adapter() noexcept : actor_services(course::make_volcano_actor_services()), lifecycle_services(make_lifecycle_services()) {}
IsleGameResult Adapter::prepare(void* owner,IsleContinuationDomain* domain,std::span<const std::byte> rom,std::int32_t argument) noexcept {
    if (!owner || !domain || entry || source.owner || actors.base.owner || course_state) return ISLE_GAME_INVALID_STATE;
    entry=static_cast<course::Lifecycle<Adapter>*>(owner);
    native.manager=&entry->manager(); native.photos=&entry->photos(); native.state=&entry->game_state();
    native.input=&entry->input(); native.audio=&entry->audio(); native.snapshot=&entry->snapshot();
    native.receipt=&entry->scene_join_receipt(); native.native_profile=entry->native_profile();
    if (rom.data()!=native.snapshot->canonical().data() || rom.size()!=isle::rom::us_size) return ISLE_GAME_INVALID_ARGUMENT;
    auto result=isle_volcano_actor_prepare(&actors,this);
    if (result==ISLE_GAME_OK) result=isle_volcano_lifecycle_prepare(&source,this,domain,&actors,&lifecycle_services,argument);
    if (result!=ISLE_GAME_OK) return result;
    try {
        course_state.emplace(actors,source,actor_services);
        native.prepare_capacity(2,reference_fields.size());
        constexpr std::array<std::array<std::uint32_t,2>,4> ranges{{{0xab5860u,0xab5980u},{0xab5980u,0xab85e0u},{11335408u,11336096u},{11336096u,11341312u}}};
        for (std::size_t i=0;i<ranges.size();++i) native.assets[i]={ranges[i][0],ranges[i][1],rom.subspan(ranges[i][0],ranges[i][1]-ranges[i][0]),false};
        return ISLE_GAME_OK;
    } catch (...) { if (source.status==ISLE_GAME_OK) source.status=ISLE_GAME_INVALID_STATE; return source.status; }
}
course::NativeDescription Adapter::description() {
    if (!course_state || !native.requested || native.host) throw std::logic_error("typed scene description requires its one Source request");
    course::NativeDescription result;
    result.course_id=2; result.effects=menu::EffectGroup::volcano; result.capacity=native.capacity;
    result.player={2,end,this,pause,this,{this,exit_block,movement},nullptr,0,volcano_HandleCollision};
    result.world={this,add,remove,change,nullptr,nullptr,nullptr};
    result.input={1u<<9,9,0,9,isle_course_scene_input_update};
    result.initial_helper={0,0x80000000u,0,volcano_func_802D64A0_7276A0};
    result.callbacks=actor_callbacks(); result.graphs=actor_graphs(); result.weighted_states=actor_weighted_states(); result.roots=actor_roots();
    result.retained_fields={retained}; result.reference_fields=reference_fields;
    return result;
}
actor::CourseContext Adapter::context() { if (!course_state) throw std::logic_error("typed CourseState not prepared"); return actor::CourseContext(*course_state); }
IsleGameResult Adapter::bind_scene(course::NativeHost& host) noexcept {
    if (native.host || !course_state || !host.closure_prepared() || !host.root() || !host.actors() || host.root()->bound_actors()!=host.actors()) return ISLE_GAME_INVALID_STATE;
    try { if (&host.actors()->course_context().volcano()!=&*course_state) return ISLE_GAME_INVALID_STATE; } catch (...) { return ISLE_GAME_INVALID_STATE; }
    native.host=&host; course_state->native_host=&host;
    return ISLE_GAME_OK;
}
bool Adapter::active() const noexcept {
    return native.host && course_state && course_state->native_host==native.host && actors.base.phase==ISLE_COURSE_ACTOR_ACTIVE && actors.base.nativeGame==native.host->game() && actors.services==&actor_services && source.owner==this && source.actors==&actors;
}
IsleGameResult Adapter::validate_admission() const noexcept {
    if (!entry || source.owner!=this || actors.base.owner!=this || source.actors!=&actors || !source.domain || actors.base.course_id!=2 || !course_state || &course_state->actors!=&actors || &course_state->source!=&source || &course_state->actor_services!=&actor_services) return ISLE_GAME_INVALID_STATE;
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
    return isle_volcano_actor_activate(&actors,host.game(),&actor_services);
}
IsleGameResult Adapter::invoke_init() noexcept { return active() ? isle_volcano_lifecycle_invoke_init(&source) : ISLE_GAME_INVALID_STATE; }
IsleGameResult Adapter::release_references(course::NativeHost& host) noexcept {
    if (native.host && native.host!=&host) return ISLE_GAME_INVALID_STATE;
    return native.release_references();
}
IsleGameResult Adapter::deactivate(course::NativeHost& host) noexcept {
    if (actors.base.phase==ISLE_COURSE_ACTOR_PREPARED) return !actors.base.nativeGame && !actors.services && !actors.base.nonwaiting_call_frame ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE;
    if (actors.base.phase==ISLE_COURSE_ACTOR_DETACHED) return !actors.base.nativeGame && !actors.services ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE;
    if (actors.base.phase!=ISLE_COURSE_ACTOR_ACTIVE || native.host!=&host || !host.detach_ready()) return ISLE_GAME_INVALID_STATE;
    return isle_volcano_actor_deactivate(&actors);
}
void Adapter::clear_scene() noexcept {
    if (course_state) course_state->native_host=nullptr;
    native.host=nullptr; native.helper=nullptr; source.screen.depth=nullptr;
}
IsleGameResult Adapter::source_status() const noexcept { return source.status!=ISLE_GAME_OK ? source.status : actors.base.status; }
bool Adapter::requested() const noexcept { return source.scene_pending!=0; }
bool Adapter::completed() const noexcept { return source.phase==ISLE_VOLCANO_LIFECYCLE_COMPLETE; }
bool Adapter::completion_ready() const noexcept { return source.scene_completed!=0; }
const Adapter::SceneSetup& Adapter::setup() const noexcept { return source.scene; }
IsleGameResult Adapter::complete_scene(const course::SceneJoinReceipt& receipt,IsleGameResult result) noexcept {
    if (&receipt!=native.receipt || !receipt.joined() || native.joined()!=ISLE_GAME_OK) return ISLE_GAME_INVALID_STATE;
    return isle_volcano_lifecycle_complete_scene(&source,result);
}
IsleGameResult Adapter::native_context(IsleGame* game,IsleGameObject* object,LifecycleSource** output) noexcept {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT; *output=nullptr;
    if (!game) return ISLE_GAME_INVALID_ARGUMENT;
    IslePlayerSource* player=nullptr; auto result=isle_player_source_context(game,&player);
    if (result!=ISLE_GAME_OK) return result;
    if (!player || !player->native_root || player->game!=game) return ISLE_GAME_INVALID_STATE;
    auto& root=*static_cast<isle::player::Root*>(player->native_root); auto* owner=root.bound_actors();
    if (&root.source()!=player || !owner || &owner->player_source()!=player || owner->course_context().course_id()!=2) return ISLE_GAME_INVALID_STATE;
    try {
        auto& selected=owner->course_context().volcano();
        if (selected.source.actors!=&selected.actors || selected.source.owner!=selected.actors.base.owner || !selected.source.owner) return ISLE_GAME_INVALID_STATE;
        auto& self=*static_cast<Adapter*>(selected.source.owner);
        if (&self.source!=&selected.source || &self.actors!=&selected.actors || !self.course_state || &*self.course_state!=&selected || &self.actor_services!=&selected.actor_services || !self.active() || self.native.host->root()!=&root || self.native.host->actors()!=owner || (object && !self.native.receiver(object))) return ISLE_GAME_INVALID_STATE;
        *output=&selected.source; return ISLE_GAME_OK;
    } catch (...) { return ISLE_GAME_INVALID_STATE; }
}
bool Adapter::add(void* user,world::Block& first,world::Block& second) noexcept { auto& a=*static_cast<Adapter*>(user); return isle_volcano_lifecycle_invoke_world_add(&a.source,block(&first),block(&second))==ISLE_GAME_OK; }
bool Adapter::change(void* user,world::Block& first,world::Block& second) noexcept { auto& a=*static_cast<Adapter*>(user); return isle_volcano_lifecycle_invoke_world_change(&a.source,block(&first),block(&second))==ISLE_GAME_OK; }
bool Adapter::remove(void* user,world::Block& value) noexcept { auto& a=*static_cast<Adapter*>(user); return isle_volcano_lifecycle_invoke_world_remove(&a.source,block(&value))==ISLE_GAME_OK; }
bool Adapter::exit_block(void* user,world::Block& value) noexcept { auto& a=*static_cast<Adapter*>(user); return isle_volcano_lifecycle_invoke_exit_block(&a.source,block(&value))==ISLE_GAME_OK; }
bool Adapter::movement(void* user,std::size_t index,float fraction) noexcept { auto& a=*static_cast<Adapter*>(user); return index<=std::size_t(INT32_MAX) && isle_volcano_actor_invoke_sounds(&a.actors,std::int32_t(index),fraction)==ISLE_GAME_OK; }
IsleGameResult Adapter::end(void* user,std::int32_t reason) noexcept { auto& a=*static_cast<Adapter*>(user); return isle_volcano_lifecycle_invoke_end(&a.source,reason); }
IsleGameResult Adapter::pause(void* user,std::int32_t reason) noexcept { auto& a=*static_cast<Adapter*>(user); return isle_volcano_lifecycle_invoke_pause(&a.source,reason); }
IsleGameResult Adapter::block_change(IsleCourseActorBase* base,std::uint32_t identity,IsleGameObject* object,const IsleGameVec3* previous,const IsleGameVec3* current) noexcept {
    if (!base || !object || !previous || !current) return ISLE_GAME_INVALID_ARGUMENT;
    int policy;
    switch (identity) { case 0x80363dbcu: policy=0; break; case 0x80363eb4u: policy=1; break; case 0x80362d2cu: policy=2; break; default: if (identity==0x802d6a14u) return isle_course_scene_invoke_block(base,volcano_CharmeleonChangeBlock,object,previous,current); return ISLE_GAME_INVALID_ARGUMENT; }
    return isle_meowth_source_call_change_block(object,previous->x,previous->y,previous->z,current->x,current->y,current->z,policy);
}
IsleGameResult Adapter::block_remove(IsleCourseActorBase* base,std::uint32_t identity,IsleGameObject* object) noexcept {
    if (!base || !object || base->course_id!=2) return ISLE_GAME_INVALID_ARGUMENT;
    LifecycleSource* source=nullptr; auto result=native_context(base->nativeGame,object,&source); if (result!=ISLE_GAME_OK) return result;
    auto& self=*static_cast<Adapter*>(source->owner);
    if (&self.actors.base!=base) return ISLE_GAME_INVALID_STATE;
    for (const auto& callback:actor_callbacks()) if (callback.identity==identity) return isle_course_scene_invoke_object(base,callback.function,object);
    return ISLE_GAME_INVALID_ARGUMENT;
}
namespace {
IsleVolcanoLifecycleServices make_lifecycle_services() noexcept {
    return {
        .block_next=+[](IsleVolcanoLifecycleSource* p0,IsleVolcanoBlock* p1,IsleVolcanoBlock** p2) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!p2) return ISLE_GAME_INVALID_ARGUMENT; *p2=nullptr; world::Block* next=nullptr; auto result=a.native.block_next(block(p1),&next); if (result==ISLE_GAME_OK) *p2=block(next); return result; });
        },
        .block_position=+[](IsleVolcanoLifecycleSource* p0,IsleVolcanoBlock* p1,IsleGameVec3* p2) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.native.block_position(block(p1),p2); });
        },
        .displace_items=+[](IsleVolcanoLifecycleSource* p0,float p1,float p2,float p3) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() && a.native.host->items_owner()->displace({p1,p2,p3}) ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE; });
        },
        .world_add=+[](IsleVolcanoLifecycleSource* p0,IsleVolcanoBlock* p1,IsleVolcanoBlock* p2,const IsleVolcanoDefinition* p3,size_t p4) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!a.active()) return ISLE_GAME_INVALID_STATE; if (p3!=volcano_PokemonDefs || p4!=21) return ISLE_GAME_INVALID_ARGUMENT; const auto rows=a.native.host->actor_resources().definitions(); for (std::size_t i=0;i<20;++i) { const auto& expected=p3[i]; auto row=std::find_if(rows.begin(),rows.end(),[&](const auto& r){ return r.wire.guest_address==0x802e0d44u+16*i; }); if (row==rows.end() || expected.identity!=row->wire.guest_address || expected.actor_id!=std::int32_t(row->actor_id) || expected.factory!=row->factory_identity || expected.change_block!=row->change_block_identity || expected.remove!=row->remove_identity) return ISLE_GAME_INVALID_ARGUMENT; } const auto& terminal=p3[20]; if (terminal.actor_id || terminal.factory || terminal.change_block || terminal.remove) return ISLE_GAME_INVALID_ARGUMENT; return a.native.pokemon_add(block(p1),block(p2),0x802e0d44u,20); });
        },
        .world_change=+[](IsleVolcanoLifecycleSource* p0,IsleVolcanoBlock* p1,IsleVolcanoBlock* p2,const IsleVolcanoDefinition* p3,size_t p4) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!a.active()) return ISLE_GAME_INVALID_STATE; if (p3!=volcano_PokemonDefs || p4!=21) return ISLE_GAME_INVALID_ARGUMENT; const auto rows=a.native.host->actor_resources().definitions(); for (std::size_t i=0;i<20;++i) { const auto& expected=p3[i]; auto row=std::find_if(rows.begin(),rows.end(),[&](const auto& r){ return r.wire.guest_address==0x802e0d44u+16*i; }); if (row==rows.end() || expected.identity!=row->wire.guest_address || expected.actor_id!=std::int32_t(row->actor_id) || expected.factory!=row->factory_identity || expected.change_block!=row->change_block_identity || expected.remove!=row->remove_identity) return ISLE_GAME_INVALID_ARGUMENT; } const auto& terminal=p3[20]; if (terminal.actor_id || terminal.factory || terminal.change_block || terminal.remove) return ISLE_GAME_INVALID_ARGUMENT; return a.native.pokemon_change(block(p1),block(p2),0x802e0d44u,20,&a.actors.base,Adapter::block_change); });
        },
        .world_remove=+[](IsleVolcanoLifecycleSource* p0,IsleVolcanoBlock* p1,const IsleVolcanoDefinition* p2,size_t p3) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!a.active()) return ISLE_GAME_INVALID_STATE; if (p2!=volcano_PokemonDefs || p3!=21) return ISLE_GAME_INVALID_ARGUMENT; const auto rows=a.native.host->actor_resources().definitions(); for (std::size_t i=0;i<20;++i) { const auto& expected=p2[i]; auto row=std::find_if(rows.begin(),rows.end(),[&](const auto& r){ return r.wire.guest_address==0x802e0d44u+16*i; }); if (row==rows.end() || expected.identity!=row->wire.guest_address || expected.actor_id!=std::int32_t(row->actor_id) || expected.factory!=row->factory_identity || expected.change_block!=row->change_block_identity || expected.remove!=row->remove_identity) return ISLE_GAME_INVALID_ARGUMENT; } const auto& terminal=p2[20]; if (terminal.actor_id || terminal.factory || terminal.change_block || terminal.remove) return ISLE_GAME_INVALID_ARGUMENT; return a.native.pokemon_remove(block(p1),0x802e0d44u,20,&a.actors.base,Adapter::block_remove); });
        },
        .height_map=+[](IsleVolcanoLifecycleSource* p0,uint32_t p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.native.map(p1,false); });
        },
        .world_create=+[](IsleVolcanoLifecycleSource* p0,uint32_t p1,int32_t p2,int32_t p3,int32_t p4,int32_t p5,int32_t p6,IsleVolcanoWorldAdd p7,IsleVolcanoWorldRemove p8,IsleVolcanoWorldAdd p9) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (p7!=volcano_PokemonAdd || p8!=volcano_PokemonRemove || p9!=volcano_PokemonChangeBlock) return ISLE_GAME_INVALID_ARGUMENT; return a.native.world(p1,p2,p3,p4,p5,p6); });
        },
        .sky_follow=+[](IsleVolcanoLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.native.host->sky_follow() : ISLE_GAME_INVALID_STATE; });
        },
        .actor_initialize_range=+[](IsleVolcanoLifecycleSource* p0,int32_t p1,int32_t p2,int32_t p3,int32_t p4) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.native.registry(p1,p2,p3,p4); });
        },
        .end_level=isle_course_scene_volcano_transition_end,
        .detector_disable=+[](IsleVolcanoLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.native.host->detector_enabled(false) : ISLE_GAME_INVALID_STATE; });
        },
        .environment_cleanup=+[](IsleVolcanoLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.native.host->environment_cleanup() : ISLE_GAME_INVALID_STATE; });
        },
        .bgm_stop=+[](IsleVolcanoLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return course::SceneMechanics::audio_result(a.native.audio->stop_bgm()); });
        },
        .sound_stop_all=+[](IsleVolcanoLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return course::SceneMechanics::audio_result(a.native.audio->stop_sounds()); });
        },
        .world_destroy=+[](IsleVolcanoLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.native.host->destroy_world() : ISLE_GAME_INVALID_STATE; });
        },
        .player_stop=+[](IsleVolcanoLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? isle_course_scene_control(a.native.player(),isle_player_func_80356FBC_4F73CC) : ISLE_GAME_INVALID_STATE; });
        },
        .camera_remove=+[](IsleVolcanoLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() && a.native.host->renderer() ? a.native.host->renderer()->remove_main() : ISLE_GAME_INVALID_STATE; });
        },
        .scene_end=+[](IsleVolcanoLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.native.host->exit_scene() : ISLE_GAME_INVALID_STATE; });
        },
        .load_asset=+[](IsleVolcanoLifecycleSource* p0,uint32_t p1,uint32_t p2,const IsleVolcanoAsset** p3) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!p3) return ISLE_GAME_INVALID_ARGUMENT; *p3=nullptr; for (std::size_t i=0;i<4;++i) if (a.native.assets[i].first==p1 && a.native.assets[i].end==p2) { course::SceneAsset* asset=nullptr; auto result=a.native.asset(i,p1,p2,&asset); if (result==ISLE_GAME_OK) *p3=reinterpret_cast<const IsleVolcanoAsset*>(asset); return result; } return ISLE_GAME_INVALID_ARGUMENT; });
        },
        .fx_bank=+[](IsleVolcanoLifecycleSource* p0,int32_t p1,const IsleVolcanoAsset* p2,const IsleVolcanoAsset* p3) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.native.bank(p1,reinterpret_cast<const course::SceneAsset*>(p2),reinterpret_cast<const course::SceneAsset*>(p3),menu::EffectGroup::volcano); });
        },
        .fx_camera=+[](IsleVolcanoLifecycleSource* p0,int32_t p1,int32_t p2,IsleGameCamera* p3,IsleGameObject** p4) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.native.fx_camera(p1,p2,p3,p4); });
        },
        .fx_allocate=+[](IsleVolcanoLifecycleSource* p0,int32_t p1,IsleGameObject** p2) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.native.fx_pool(p1,p2); });
        },
        .fx_dither=+[](IsleVolcanoLifecycleSource* p0,IsleVolcanoColorDither p1,IsleVolcanoAlphaDither p2) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (p1!=ISLE_VOLCANO_COLOR_DITHER_DISABLED || p2!=ISLE_VOLCANO_ALPHA_DITHER_DISABLED) return ISLE_GAME_INVALID_ARGUMENT; return a.native.dither(0xc0u,0x30u); });
        },
        .fx_reference=+[](IsleVolcanoLifecycleSource* p0,uint32_t p1,IsleGameObject* p2) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.native.fx_publish(p1,p2); });
        },
        .depth_allocate=+[](IsleVolcanoLifecycleSource* p0,uint32_t p1,uint32_t p2,void** p3) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.native.host->allocate_depth(std::uint32_t(p1),std::uint32_t(p2),p3) : ISLE_GAME_INVALID_STATE; });
        },
        .apply_screen=+[](IsleVolcanoLifecycleSource* p0,const IsleVolcanoScreen* p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!a.native.setup() || p1!=&a.source.screen || p1->framebuffer3) return ISLE_GAME_INVALID_ARGUMENT; return a.native.host->apply_screen({p1->width,p1->height,p1->flags,{p1->framebuffer1,p1->framebuffer2,0}},p1->depth); });
        },
        .fx_reset_bank=+[](IsleVolcanoLifecycleSource* p0,size_t p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.native.setup() ? a.native.host->reset_effect_bank(p1) : ISLE_GAME_INVALID_STATE; });
        },
        .engine_initialize=+[](IsleVolcanoLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.native.setup() ? a.native.host->reset_photo_roll() : ISLE_GAME_INVALID_STATE; });
        },
        .add_object=+[](IsleVolcanoLifecycleSource* p0,int32_t p1,IsleGameSourceCallback p2,int32_t p3,uint32_t p4,IsleGameObject** p5) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (p2!=volcano_func_802D64A0_7276A0) return ISLE_GAME_INVALID_ARGUMENT; return a.native.helper_create(p1,p2,p3,p4,p5); });
        },
        .actors_initialize=+[](IsleVolcanoLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!a.native.setup()) return ISLE_GAME_INVALID_STATE; if (!a.native.helper) a.native.helper=a.native.host->reference_owner(); IsleGameObjectMetadata metadata{}; if (!a.native.helper || isle_game_object_metadata(a.native.host->game(),a.native.helper,&metadata)!=ISLE_GAME_OK || metadata.id!=0 || metadata.update_link!=0 || metadata.priority!=0x80000000u) return ISLE_GAME_INVALID_STATE; return a.native.host->initialize_collisions(a.native.helper); });
        },
        .background_color=+[](IsleVolcanoLifecycleSource* p0,uint8_t* p1,uint8_t* p2,uint8_t* p3) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.native.background(p1,p2,p3); });
        },
        .cameras_create=+[](IsleVolcanoLifecycleSource* p0,uint32_t p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.native.setup() ? a.native.host->create_cameras(std::uint32_t(p1)) : ISLE_GAME_INVALID_STATE; });
        },
        .ui_initialize=+[](IsleVolcanoLifecycleSource* p0,const IsleVolcanoUi* p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!p1 || p1->exit_block!=volcano_ExitBlock || p1->movement!=volcano_UpdateSounds || p1->item_update || p1->item_update_kind || p1->item_collision!=volcano_HandleCollision) return ISLE_GAME_INVALID_ARGUMENT; return a.native.initialize_ui(); });
        },
        .end_callback=+[](IsleVolcanoLifecycleSource* p0,IsleVolcanoEndCallback p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!a.native.setup() || p1!=volcano_EndLevel || a.native.end_bound || a.native.player()->EndLevelCb!=Adapter::end || a.native.player()->end_level_user!=&a) return ISLE_GAME_INVALID_ARGUMENT; a.native.end_bound=true; return ISLE_GAME_OK; });
        },
        .pause_callback=+[](IsleVolcanoLifecycleSource* p0,IsleVolcanoPauseCallback p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!a.native.setup() || p1!=volcano_Pause || a.native.pause_bound || a.native.player()->PauseCb!=Adapter::pause || a.native.player()->pause_user!=&a) return ISLE_GAME_INVALID_ARGUMENT; a.native.pause_bound=true; return ISLE_GAME_OK; });
        },
        .environment_initialize=+[](IsleVolcanoLifecycleSource* p0,const IsleVolcanoEnvironment* p1,size_t p2) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (p1!=volcano_EnvSounds) return ISLE_GAME_INVALID_ARGUMENT; return a.native.environment(p1,p2,49,+[](const void* p,std::size_t n){return static_cast<const IsleVolcanoEnvironment*>(p)[n].sound;},+[](const void* p,std::size_t n){return static_cast<const IsleVolcanoEnvironment*>(p)[n].pitch;},+[](const void* p,std::size_t n){return static_cast<const IsleVolcanoEnvironment*>(p)[n].range;}); });
        },
        .detector_create=+[](IsleVolcanoLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.native.setup() ? a.native.host->initialize_detector(a.native.helper) : ISLE_GAME_INVALID_STATE; });
        },
        .detector_enable=+[](IsleVolcanoLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.native.host->detector_enabled(true) : ISLE_GAME_INVALID_STATE; });
        },
        .illegal_copy=+[](IsleVolcanoLifecycleSource* p0,int32_t* p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!p1 || a.native.native_profile!=course::EntryNativeProfile::hosted_canonical_image || !a.native.snapshot || a.native.snapshot->canonical().size()!=rom::us_size || a.native.snapshot->identity().size!=rom::us_size) return ISLE_GAME_INVALID_STATE; constexpr std::array<std::uint8_t,20> identity{0xed,0xc7,0xc4,0x9c,0xc5,0x68,0xc0,0x45,0xfe,0x48,0xbe,0x0d,0x18,0x01,0x1c,0x30,0xf3,0x93,0xcb,0xaf}; if (a.native.snapshot->identity().sha1!=identity) return ISLE_GAME_INVALID_STATE; *p1=0; return ISLE_GAME_OK; });
        },
        .player_flag=+[](IsleVolcanoLifecycleSource* p0,int32_t p1,int32_t p2) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { auto* save=a.native.state->save(); return save && !save->set_player_flag(std::uint32_t(p1),p2!=0) ? ISLE_GAME_OK : ISLE_GAME_EXECUTION_FAILURE; });
        },
        .set_intervals=+[](IsleVolcanoLifecycleSource* p0,int32_t p1,int32_t p2) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (p1<0 || p1>UINT16_MAX || p2<0 || p2>UINT16_MAX) return ISLE_GAME_INVALID_ARGUMENT; return a.entry->intervals(std::uint16_t(p1),std::uint16_t(p2)); });
        },
        .near_clipping=+[](IsleVolcanoLifecycleSource* p0,int32_t p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (p1!=0 && p1!=1) return ISLE_GAME_INVALID_ARGUMENT; return a.entry->near_clipping(p1!=0); });
        },
        .setup_scene=+[](IsleVolcanoLifecycleSource* p0,const IsleVolcanoScene* p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (a.actors.base.phase!=ISLE_COURSE_ACTOR_PREPARED || a.source.scene.heap_size!=1230816u) return ISLE_GAME_INVALID_STATE; return a.native.request(p1,&a.source.scene); });
        },
        .photo_count=+[](IsleVolcanoLifecycleSource* p0,int32_t* p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!p1 || a.native.joined()!=ISLE_GAME_OK) return ISLE_GAME_INVALID_STATE; *p1=a.native.photos->count(); return ISLE_GAME_OK; });
        },
        .notice=+[](IsleVolcanoLifecycleSource* p0,uint32_t p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { a.native.state->add_scene_notices(std::uint32_t(p1)); return ISLE_GAME_OK; });
        },
        .input_pressed=+[](IsleVolcanoLifecycleSource* p0,uint32_t* p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!p1) return ISLE_GAME_INVALID_ARGUMENT; auto query=a.native.input->menu(a.native.state->controller); if (query.error!=game_input::Error::none || !query.input) return ISLE_GAME_INVALID_STATE; *p1=std::remove_reference_t<decltype(*p1)>(query.input->pressed); return ISLE_GAME_OK; });
        },
        .cutscene_prepare=+[](IsleVolcanoLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? isle_course_scene_control(a.native.player(),isle_player_func_803570B0_4F74C0) : ISLE_GAME_INVALID_STATE; });
        },
        .cutscene_model=+[](IsleVolcanoLifecycleSource* p0,IsleGameObject* p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() && a.native.receiver(p1) ? isle_course_scene_object_control(a.native.player(),isle_player_func_80357120_4F7530,p1) : ISLE_GAME_INVALID_STATE; });
        },
        .cutscene_run=+[](IsleVolcanoLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? isle_course_scene_control(a.native.player(),isle_player_func_803571C4_4F75D4) : ISLE_GAME_INVALID_STATE; });
        },
        .cutscene_stop=+[](IsleVolcanoLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? isle_course_scene_control(a.native.player(),isle_player_func_803572B0_4F76C0) : ISLE_GAME_INVALID_STATE; });
        },
        .intro_restore=+[](IsleVolcanoLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? isle_course_scene_control(a.native.player(),isle_player_func_80357170_4F7580) : ISLE_GAME_INVALID_STATE; });
        },
        .effect_create=+[](IsleVolcanoLifecycleSource* p0,int32_t p1,int32_t p2,IsleVolcanoEffect** p3) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!p3) return ISLE_GAME_INVALID_ARGUMENT; *p3=nullptr; course::SceneEffect* effect=nullptr; auto result=a.native.effect(p1,p2,&effect); if (result==ISLE_GAME_OK && effect) *p3=reinterpret_cast<IsleVolcanoEffect*>(effect); return result; });
        },
        .effect_attach=+[](IsleVolcanoLifecycleSource* p0,IsleVolcanoEffect* p1,const IslePlayerCartNode* p2) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!p2) return ISLE_GAME_INVALID_ARGUMENT; if (!a.active() || !a.native.host->cart()->callback_matches(p2->binding.object,isle_course_scene_volcano_cart_event,&a.source)) return ISLE_GAME_INVALID_ARGUMENT; return a.native.attach(reinterpret_cast<course::SceneEffect*>(p1),p2); });
        },
        .camera_read=+[](IsleVolcanoLifecycleSource* p0,IsleGameCamera* p1,uint32_t p2,float* p3) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.native.camera_component(p1,p2,p3,false); });
        },
        .camera_store=+[](IsleVolcanoLifecycleSource* p0,IsleGameCamera* p1,uint32_t p2,float p3) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.native.camera_component(p1,p2,nullptr,true,p3); });
        },
        .delete_intro=+[](IsleVolcanoLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.native.cart_retire(isle_menu_current_object(a.native.host->game())) : ISLE_GAME_INVALID_STATE; });
        },
        .animation_callback=+[](IsleVolcanoLifecycleSource* p0,IsleGameObject* p1,IsleVolcanoIntroAnimationCallback p2) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!a.active() || p2!=volcano_IntroAnimationCallback || !a.native.host->cart()->instance(p1)) return ISLE_GAME_INVALID_ARGUMENT; return a.native.host->cart()->set_callback(p1,isle_course_scene_volcano_cart_event,&a.source) ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE; });
        },
        .scene_joined=+[](IsleVolcanoLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.native.joined(); });
        },
        .entry_joined=+[](IsleVolcanoLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.entry->entry_joined() ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE; });
        },
        .create_process=+[](IsleVolcanoLifecycleSource* p0,IsleGameObject* p1,IsleGameSourceCallback p2,int32_t p3,int32_t p4,IsleGameProcess** p5) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (p2!=volcano_UpdateIntro && p2!=volcano_PlayIntroSounds) return ISLE_GAME_INVALID_ARGUMENT; return a.native.process(p1,p2,p3,p4,p5); });
        },
        .reference_assign=+[](IsleVolcanoLifecycleSource* p0,IsleGameObject** p1,IsleGameObject* p2) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.native.reference(p1,p2) : ISLE_GAME_INVALID_STATE; });
        },
        .sound_identity=+[](IsleVolcanoLifecycleSource* p0,uint32_t p1,int32_t* p2) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!p2) return ISLE_GAME_INVALID_ARGUMENT; *p2=a.native.audio->playing_sound(std::uint32_t(p1)); return ISLE_GAME_OK; });
        },
        .sound_stop=+[](IsleVolcanoLifecycleSource* p0,uint32_t p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return course::SceneMechanics::audio_result(a.native.audio->stop_sound(std::uint32_t(p1))); });
        },
        .sound_play_params=+[](IsleVolcanoLifecycleSource* p0,int32_t p1,int32_t p2,int32_t p3,float p4,int32_t p5,int32_t* p6) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.native.sound(p1,p2,p3,p4,p5,p6); });
        },
        .main_camera=+[](IsleVolcanoLifecycleSource* p0,IsleGameCamera** p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.actor_services.main_camera(&a.actors,p1) : ISLE_GAME_INVALID_STATE; });
        },
        .camera_object=+[](IsleVolcanoLifecycleSource* p0,IsleGameCamera* p1,IsleGameObject** p2) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.actor_services.camera_object(&a.actors,p1,p2) : ISLE_GAME_INVALID_STATE; });
        },
        .camera_speed=+[](IsleVolcanoLifecycleSource* p0,IsleGameCamera* p1,float p2) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.actor_services.camera_speed(&a.actors,p1,p2) : ISLE_GAME_INVALID_STATE; });
        },
        .camera_animation=+[](IsleVolcanoLifecycleSource* p0,IsleGameCamera* p1,uint32_t p2,float p3) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.actor_services.camera_animation(&a.actors,p1,p2,p3) : ISLE_GAME_INVALID_STATE; });
        },
        .camera_process=+[](IsleVolcanoLifecycleSource* p0,IsleGameObject* p1,int32_t p2,int32_t p3,IsleGameProcess** p4) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!a.active() || p2!=1 || p3!=1 || !p4) return ISLE_GAME_INVALID_ARGUMENT; IsleGameCamera* camera=nullptr; auto result=a.actor_services.main_camera(&a.actors,&camera); IsleGameObject* object=nullptr; if (result==ISLE_GAME_OK) result=a.actor_services.camera_object(&a.actors,camera,&object); if (result!=ISLE_GAME_OK) return result; if (p1!=object) return ISLE_GAME_INVALID_ARGUMENT; return a.native.process(object,isle_course_scene_camera_update,p2,p3,p4); });
        },
        .player_animation=+[](IsleVolcanoLifecycleSource* p0,uint32_t p1,uint32_t p2,float p3,float p4) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.actor_services.player_animation(&a.actors,p1,p2,p3,p4) : ISLE_GAME_INVALID_STATE; });
        },
        .player_model_init=+[](IsleVolcanoLifecycleSource* p0,IsleGameObject** p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.actor_services.player_model_init(&a.actors,p1) : ISLE_GAME_INVALID_STATE; });
        },
        .reset_camera=+[](IsleVolcanoLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? isle_player_root_invoke_reset_main_camera_settings(a.native.player()) : ISLE_GAME_INVALID_STATE; });
        },
    };
}
}
} // namespace isle::volcano
extern "C" IsleGameResult isle_volcano_lifecycle_native_context(IsleGameObject* object,IsleVolcanoLifecycleSource** output) { return isle::volcano::Adapter::native_context(isle_course_scene_object_game(object),object,output); }
extern "C" IsleGameResult isle_course_scene_volcano_owner_transition(IsleVolcanoLifecycleSource* source,int32_t reason) {
    return isle::volcano::Adapter::invoke(source,[&](isle::volcano::Adapter& a) { return a.active() ? a.native.manager->transition_end(reason) : ISLE_GAME_INVALID_STATE; });
}
