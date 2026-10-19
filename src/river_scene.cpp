#include "river_scene.hpp"
#include "menu_scene_bridge.h"
#include "environment_sound.hpp"
#include <limits>
namespace isle::river {
namespace {
IsleRiverBlock* block(world::Block* value) noexcept { return reinterpret_cast<IsleRiverBlock*>(value); }
world::Block* block(IsleRiverBlock* value) noexcept { return reinterpret_cast<world::Block*>(value); }
constexpr std::array<actor::RetainedFieldFamily,0> retained{};
IsleRiverLifecycleServices make_lifecycle_services() noexcept;
}
Adapter::Adapter() noexcept : actor_services(course::make_river_actor_services()), lifecycle_services(make_lifecycle_services()) {}
IsleGameResult Adapter::prepare(void* owner,IsleContinuationDomain* domain,std::span<const std::byte> rom,std::int32_t argument) noexcept {
    if (!owner || !domain || entry || source.owner || actors.base.owner || course_state) return ISLE_GAME_INVALID_STATE;
    entry=static_cast<course::Lifecycle<Adapter>*>(owner);
    native.manager=&entry->manager(); native.photos=&entry->photos(); native.state=&entry->game_state();
    native.input=&entry->input(); native.audio=&entry->audio(); native.snapshot=&entry->snapshot();
    native.receipt=&entry->scene_join_receipt(); native.native_profile=entry->native_profile();
    if (rom.data()!=native.snapshot->canonical().data() || rom.size()!=isle::rom::us_size) return ISLE_GAME_INVALID_ARGUMENT;
    auto result=isle_river_actor_prepare(&actors,this,reinterpret_cast<const std::uint8_t*>(rom.data()),rom.size());
    if (result==ISLE_GAME_OK) result=isle_river_lifecycle_prepare(&source,this,domain,&actors,&lifecycle_services,reinterpret_cast<const std::uint8_t*>(rom.data()),rom.size(),argument);
    if (result!=ISLE_GAME_OK) return result;
    try {
        course_state.emplace(actors,source,actor_services);
        native.prepare_capacity(3,reference_fields.size());
        constexpr std::array<std::array<std::uint32_t,2>,4> ranges{{{0xab5860u,0xab5980u},{0xab5980u,0xab85e0u},{11306256u,11307056u},{11307056u,11335408u}}};
        for (std::size_t i=0;i<ranges.size();++i) native.assets[i]={ranges[i][0],ranges[i][1],rom.subspan(ranges[i][0],ranges[i][1]-ranges[i][0]),false};
        return ISLE_GAME_OK;
    } catch (...) { if (source.status==ISLE_GAME_OK) source.status=ISLE_GAME_INVALID_STATE; return source.status; }
}
course::NativeDescription Adapter::description() {
    if (!course_state || !native.requested || native.host) throw std::logic_error("typed scene description requires its one Source request");
    course::NativeDescription result;
    result.course_id=3; result.effects=menu::EffectGroup::river; result.capacity=native.capacity;
    result.player={3,end,this,pause,this,{this,exit_block,movement},nullptr,0,func_802D8E98_6C0C78};
    result.world={this,add,remove,change,nullptr,nullptr,nullptr};
    result.input={1u<<9,9,0,9,isle_course_scene_input_update};
    result.initial_helper={0,0x80000000u,0,func_802D895C_6C073C};
    result.callbacks=actor_callbacks(); result.graphs=actor_graphs(); result.weighted_states=actor_weighted_states(); result.roots=actor_roots();
    result.retained_fields={retained}; result.reference_fields=reference_fields;
    return result;
}
actor::CourseContext Adapter::context() { if (!course_state) throw std::logic_error("typed CourseState not prepared"); return actor::CourseContext(*course_state); }
IsleGameResult Adapter::bind_scene(course::NativeHost& host) noexcept {
    if (native.host || !course_state || !host.closure_prepared() || !host.root() || !host.actors() || host.root()->bound_actors()!=host.actors()) return ISLE_GAME_INVALID_STATE;
    try { if (&host.actors()->course_context().river()!=&*course_state) return ISLE_GAME_INVALID_STATE; } catch (...) { return ISLE_GAME_INVALID_STATE; }
    native.host=&host; course_state->native_host=&host;
    return ISLE_GAME_OK;
}
bool Adapter::active() const noexcept {
    return native.host && course_state && course_state->native_host==native.host && actors.base.phase==ISLE_COURSE_ACTOR_ACTIVE && actors.base.nativeGame==native.host->game() && actors.services==&actor_services && source.owner==this && source.actors==&actors;
}
IsleGameResult Adapter::validate_admission() const noexcept {
    if (!entry || source.owner!=this || actors.base.owner!=this || source.actors!=&actors || !source.domain || actors.base.course_id!=3 || !course_state || &course_state->actors!=&actors || &course_state->source!=&source || &course_state->actor_services!=&actor_services) return ISLE_GAME_INVALID_STATE;
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
    return isle_river_actor_activate(&actors,host.game(),&actor_services);
}
IsleGameResult Adapter::invoke_init() noexcept { return active() ? isle_river_lifecycle_invoke_init(&source) : ISLE_GAME_INVALID_STATE; }
IsleGameResult Adapter::release_references(course::NativeHost& host) noexcept {
    if (native.host && native.host!=&host) return ISLE_GAME_INVALID_STATE;
    return native.release_references();
}
IsleGameResult Adapter::deactivate(course::NativeHost& host) noexcept {
    if (actors.base.phase==ISLE_COURSE_ACTOR_PREPARED) return !actors.base.nativeGame && !actors.services && !actors.base.nonwaiting_call_frame ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE;
    if (actors.base.phase==ISLE_COURSE_ACTOR_DETACHED) return !actors.base.nativeGame && !actors.services ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE;
    if (actors.base.phase!=ISLE_COURSE_ACTOR_ACTIVE || native.host!=&host || !host.detach_ready()) return ISLE_GAME_INVALID_STATE;
    return isle_river_actor_deactivate(&actors);
}
void Adapter::clear_scene() noexcept {
    if (course_state) course_state->native_host=nullptr;
    native.host=nullptr; native.helper=nullptr; source.screen.depth_buffer=nullptr;
}
IsleGameResult Adapter::source_status() const noexcept { return source.status!=ISLE_GAME_OK ? source.status : actors.base.status; }
bool Adapter::requested() const noexcept { return source.scene_requested!=0; }
bool Adapter::completed() const noexcept { return source.completed!=0; }
bool Adapter::completion_ready() const noexcept { return source.scene_completion_ready!=0; }
const Adapter::SceneSetup& Adapter::setup() const noexcept { return source.scene_setup; }
IsleGameResult Adapter::complete_scene(const course::SceneJoinReceipt& receipt,IsleGameResult result) noexcept {
    if (&receipt!=native.receipt || !receipt.joined() || native.joined()!=ISLE_GAME_OK) return ISLE_GAME_INVALID_STATE;
    return isle_river_lifecycle_complete_scene(&source,result);
}
IsleGameResult Adapter::native_context(IsleGame* game,IsleGameObject* object,LifecycleSource** output) noexcept {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT; *output=nullptr;
    if (!game) return ISLE_GAME_INVALID_ARGUMENT;
    IslePlayerSource* player=nullptr; auto result=isle_player_source_context(game,&player);
    if (result!=ISLE_GAME_OK) return result;
    if (!player || !player->native_root || player->game!=game) return ISLE_GAME_INVALID_STATE;
    auto& root=*static_cast<isle::player::Root*>(player->native_root); auto* owner=root.bound_actors();
    if (&root.source()!=player || !owner || &owner->player_source()!=player || owner->course_context().course_id()!=3) return ISLE_GAME_INVALID_STATE;
    try {
        auto& selected=owner->course_context().river();
        if (selected.source.actors!=&selected.actors || selected.source.owner!=selected.actors.base.owner || !selected.source.owner) return ISLE_GAME_INVALID_STATE;
        auto& self=*static_cast<Adapter*>(selected.source.owner);
        if (&self.source!=&selected.source || &self.actors!=&selected.actors || !self.course_state || &*self.course_state!=&selected || &self.actor_services!=&selected.actor_services || !self.active() || self.native.host->root()!=&root || self.native.host->actors()!=owner || (object && !self.native.receiver(object))) return ISLE_GAME_INVALID_STATE;
        *output=&selected.source; return ISLE_GAME_OK;
    } catch (...) { return ISLE_GAME_INVALID_STATE; }
}
bool Adapter::add(void* user,world::Block& first,world::Block& second) noexcept { auto& a=*static_cast<Adapter*>(user); return isle_river_lifecycle_invoke_world_pair(&a.source,func_802D8800_6C05E0,block(&first),block(&second))==ISLE_GAME_OK; }
bool Adapter::change(void* user,world::Block& first,world::Block& second) noexcept { auto& a=*static_cast<Adapter*>(user); return isle_river_lifecycle_invoke_world_pair(&a.source,func_802D8824_6C0604,block(&first),block(&second))==ISLE_GAME_OK; }
bool Adapter::remove(void* user,world::Block& value) noexcept { auto& a=*static_cast<Adapter*>(user); return isle_river_lifecycle_invoke_world_single(&a.source,func_802D8848_6C0628,block(&value))==ISLE_GAME_OK; }
bool Adapter::exit_block(void* user,world::Block& value) noexcept { auto& a=*static_cast<Adapter*>(user); return isle_river_lifecycle_invoke_world_single(&a.source,func_802D89B4_6C0794,block(&value))==ISLE_GAME_OK; }
bool Adapter::movement(void* user,std::size_t index,float fraction) noexcept { auto& a=*static_cast<Adapter*>(user); return index<=std::size_t(INT32_MAX) && isle_river_lifecycle_invoke_movement(&a.source,std::int32_t(index),fraction)==ISLE_GAME_OK; }
IsleGameResult Adapter::end(void* user,std::int32_t reason) noexcept { auto& a=*static_cast<Adapter*>(user); return isle_river_lifecycle_invoke_end(&a.source,reason); }
IsleGameResult Adapter::pause(void* user,std::int32_t reason) noexcept { auto& a=*static_cast<Adapter*>(user); return isle_course_scene_river_pause(&a.source,reason); }
IsleGameResult Adapter::block_change(IsleCourseActorBase* base,std::uint32_t identity,IsleGameObject* object,const IsleGameVec3* previous,const IsleGameVec3* current) noexcept {
    if (!base || !object || !previous || !current) return ISLE_GAME_INVALID_ARGUMENT;
    int policy;
    switch (identity) { case 0x80363dbcu: policy=0; break; case 0x80363eb4u: policy=1; break; case 0x80362d2cu: policy=2; break; default:  return ISLE_GAME_INVALID_ARGUMENT; }
    return isle_meowth_source_call_change_block(object,previous->x,previous->y,previous->z,current->x,current->y,current->z,policy);
}
IsleGameResult Adapter::block_remove(IsleCourseActorBase* base,std::uint32_t identity,IsleGameObject* object) noexcept {
    if (!base || !object || base->course_id!=3) return ISLE_GAME_INVALID_ARGUMENT;
    LifecycleSource* source=nullptr; auto result=native_context(base->nativeGame,object,&source); if (result!=ISLE_GAME_OK) return result;
    auto& self=*static_cast<Adapter*>(source->owner);
    if (&self.actors.base!=base) return ISLE_GAME_INVALID_STATE;
    for (const auto& callback:actor_callbacks()) if (callback.identity==identity) return isle_course_scene_invoke_object(base,callback.function,object);
    return ISLE_GAME_INVALID_ARGUMENT;
}
namespace {
IsleRiverLifecycleServices make_lifecycle_services() noexcept {
    return {
        .sound_params=+[](IsleRiverLifecycleSource* p0,int32_t p1,int32_t p2,int32_t p3,float p4,int32_t p5,int32_t* p6) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.native.sound(p1,p2,p3,p4,p5,p6); });
        },
        .sound_active=+[](IsleRiverLifecycleSource* p0,int32_t p1,int32_t* p2) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!p2) return ISLE_GAME_INVALID_ARGUMENT; *p2=a.native.audio->playing_sound(std::uint32_t(p1)); return ISLE_GAME_OK; });
        },
        .sound_stop=+[](IsleRiverLifecycleSource* p0,int32_t p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return course::SceneMechanics::audio_result(a.native.audio->stop_sound(std::uint32_t(p1))); });
        },
        .sound_stop_all=+[](IsleRiverLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return course::SceneMechanics::audio_result(a.native.audio->stop_sounds()); });
        },
        .bgm_stop_all=+[](IsleRiverLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return course::SceneMechanics::audio_result(a.native.audio->stop_bgm()); });
        },
        .bgm_play=+[](IsleRiverLifecycleSource* p0,int32_t p1,int32_t p2) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return course::SceneMechanics::audio_result(a.native.audio->play_song(std::uint32_t(p1),std::uint32_t(p2))); });
        },
        .bgm_stop=+[](IsleRiverLifecycleSource* p0,int32_t p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return course::SceneMechanics::audio_result(a.native.audio->stop_song(std::uint32_t(p1))); });
        },
        .bgm_volume=+[](IsleRiverLifecycleSource* p0,int32_t p1,uint32_t p2) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return course::SceneMechanics::audio_result(a.native.audio->bgm_volume(std::uint32_t(p1),p2)); });
        },
        .bgm_channel_volume=+[](IsleRiverLifecycleSource* p0,int32_t p1,int32_t p2,int32_t p3) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return course::SceneMechanics::audio_result(a.native.audio->bgm_channel_volume(std::uint32_t(p1),std::uint8_t(p2),std::int8_t(p3))); });
        },
        .camera=+[](IsleRiverLifecycleSource* p0,IsleGameObject** p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.actor_services.camera(&a.actors,p1) : ISLE_GAME_INVALID_STATE; });
        },
        .camera_read=+[](IsleRiverLifecycleSource* p0,IsleGameObject* p1,const IsleRiverCameraView** p2) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.actor_services.camera_read(&a.actors,p1,p2) : ISLE_GAME_INVALID_STATE; });
        },
        .camera_store=+[](IsleRiverLifecycleSource* p0,IsleGameObject* p1,uint32_t p2,float p3) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.actor_services.camera_store(&a.actors,p1,p2,p3) : ISLE_GAME_INVALID_STATE; });
        },
        .camera_speed=+[](IsleRiverLifecycleSource* p0,IsleGameObject* p1,float p2) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.actor_services.camera_speed(&a.actors,p1,p2) : ISLE_GAME_INVALID_STATE; });
        },
        .camera_animation=+[](IsleRiverLifecycleSource* p0,IsleGameObject* p1,uint32_t p2,float p3) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.actor_services.camera_animation(&a.actors,p1,p2,p3) : ISLE_GAME_INVALID_STATE; });
        },
        .camera_process=+[](IsleRiverLifecycleSource* p0,IsleGameObject* p1,int32_t p2,int32_t p3,IsleGameProcess** p4) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!a.active() || p2!=1 || p3!=1 || !p4) return ISLE_GAME_INVALID_ARGUMENT; IsleGameObject* object=nullptr; auto result=a.actor_services.camera(&a.actors,&object); if (result!=ISLE_GAME_OK) return result; if (p1!=object) return ISLE_GAME_INVALID_ARGUMENT; return a.native.process(object,isle_course_scene_camera_update,p2,p3,p4); });
        },
        .player_model_init=+[](IsleRiverLifecycleSource* p0,IsleGameObject** p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.actor_services.player_model_init(&a.actors,p1) : ISLE_GAME_INVALID_STATE; });
        },
        .player_animation=+[](IsleRiverLifecycleSource* p0,uint32_t p1,uint32_t p2,float p3,float p4) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.actor_services.player_animation(&a.actors,p1,p2,p3,p4) : ISLE_GAME_INVALID_STATE; });
        },
        .animation_callback=+[](IsleRiverLifecycleSource* p0,IsleGameObject* p1,IsleRiverLifecycleAnimationCallback p2) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!a.active() || p2!=func_802E2120_6C9F00 || !a.native.host->cart()->instance(p1)) return ISLE_GAME_INVALID_ARGUMENT; return a.native.host->cart()->set_callback(p1,isle_course_scene_river_cart_event,&a.source) ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE; });
        },
        .create_helper=+[](IsleRiverLifecycleSource* p0,int32_t p1,IsleGameSourceCallback p2,int32_t p3,uint32_t p4,IsleGameObject** p5) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (p2!=func_802D895C_6C073C) return ISLE_GAME_INVALID_ARGUMENT; return a.native.helper_create(p1,p2,p3,p4,p5); });
        },
        .delete_object=+[](IsleRiverLifecycleSource* p0,IsleGameObject* p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!a.active()) return ISLE_GAME_INVALID_STATE; auto* object=p1 ? p1 : isle_menu_current_object(a.native.host->game()); if (!object) return ISLE_GAME_INVALID_ARGUMENT; if (a.native.host->cart()->instance(object)) return a.native.cart_retire(object); if (!a.native.receiver(object)) return ISLE_GAME_INVALID_ARGUMENT; a.native.host->services()->erase(object); return ISLE_GAME_OK; });
        },
        .notice=+[](IsleRiverLifecycleSource* p0,uint32_t p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { a.native.state->add_scene_notices(std::uint32_t(p1)); return ISLE_GAME_OK; });
        },
        .effect_create=+[](IsleRiverLifecycleSource* p0,int32_t p1,int32_t p2,IsleRiverEffect** p3) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!p3) return ISLE_GAME_INVALID_ARGUMENT; *p3=nullptr; course::SceneEffect* effect=nullptr; auto result=a.native.effect(p1,p2,&effect); if (result==ISLE_GAME_OK && effect) *p3=reinterpret_cast<IsleRiverEffect*>(effect); return result; });
        },
        .effect_attach=+[](IsleRiverLifecycleSource* p0,IsleRiverEffect* p1,const IsleActorNodeRef* p2) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!p2) return ISLE_GAME_INVALID_ARGUMENT; if (!a.active() || !a.native.host->cart()->callback_matches(p2->model.object,isle_course_scene_river_cart_event,&a.source)) return ISLE_GAME_INVALID_ARGUMENT; if (!p2) return ISLE_GAME_INVALID_ARGUMENT; IslePlayerCartNode node{p2->model,p2->node}; return a.native.attach(reinterpret_cast<course::SceneEffect*>(p1),&node); });
        },
        .input_pressed=+[](IsleRiverLifecycleSource* p0,uint16_t* p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!p1) return ISLE_GAME_INVALID_ARGUMENT; auto query=a.native.input->menu(a.native.state->controller); if (query.error!=game_input::Error::none || !query.input) return ISLE_GAME_INVALID_STATE; *p1=std::remove_reference_t<decltype(*p1)>(query.input->pressed); return ISLE_GAME_OK; });
        },
        .world_add=+[](IsleRiverLifecycleSource* p0,IsleRiverBlock* p1,IsleRiverBlock* p2,uint32_t p3) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!a.active()) return ISLE_GAME_INVALID_STATE; if (p3!=0x802e271cu) return ISLE_GAME_INVALID_ARGUMENT; return a.native.pokemon_add(block(p1),block(p2),0x802e271cu,24); });
        },
        .world_change=+[](IsleRiverLifecycleSource* p0,IsleRiverBlock* p1,IsleRiverBlock* p2,uint32_t p3) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!a.active()) return ISLE_GAME_INVALID_STATE; if (p3!=0x802e271cu) return ISLE_GAME_INVALID_ARGUMENT; return a.native.pokemon_change(block(p1),block(p2),0x802e271cu,24,&a.actors.base,Adapter::block_change); });
        },
        .world_remove=+[](IsleRiverLifecycleSource* p0,IsleRiverBlock* p1,uint32_t p2) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!a.active()) return ISLE_GAME_INVALID_STATE; if (p2!=0x802e271cu) return ISLE_GAME_INVALID_ARGUMENT; return a.native.pokemon_remove(block(p1),0x802e271cu,24,&a.actors.base,Adapter::block_remove); });
        },
        .height_map=+[](IsleRiverLifecycleSource* p0,uint32_t p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.native.map(p1,false); });
        },
        .create_world=+[](IsleRiverLifecycleSource* p0,uint32_t p1,int32_t p2,int32_t p3,int32_t p4,int32_t p5,int32_t p6,IsleRiverLifecycleWorldPair p7,IsleRiverLifecycleWorldSingle p8,IsleRiverLifecycleWorldPair p9) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (p7!=func_802D8800_6C05E0 || p8!=func_802D8848_6C0628 || p9!=func_802D8824_6C0604) return ISLE_GAME_INVALID_ARGUMENT; return a.native.world(p1,p2,p3,p4,p5,p6); });
        },
        .sky_follow=+[](IsleRiverLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.native.host->sky_follow() : ISLE_GAME_INVALID_STATE; });
        },
        .pokemon_registry=+[](IsleRiverLifecycleSource* p0,int32_t p1,int32_t p2,int32_t p3,int32_t p4) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.native.registry(p1,p2,p3,p4); });
        },
        .transition_end=isle_course_scene_river_transition_end,
        .destroy_world=+[](IsleRiverLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.native.host->destroy_world() : ISLE_GAME_INVALID_STATE; });
        },
        .environment_cleanup=+[](IsleRiverLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.native.host->environment_cleanup() : ISLE_GAME_INVALID_STATE; });
        },
        .detector_disable=+[](IsleRiverLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.native.host->detector_enabled(false) : ISLE_GAME_INVALID_STATE; });
        },
        .remove_camera=+[](IsleRiverLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() && a.native.host->renderer() ? a.native.host->renderer()->remove_main() : ISLE_GAME_INVALID_STATE; });
        },
        .exit_scene=+[](IsleRiverLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.native.host->exit_scene() : ISLE_GAME_INVALID_STATE; });
        },
        .course_cleanup=+[](IsleRiverLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? isle_course_scene_control(a.native.player(),isle_player_func_80356FBC_4F73CC) : ISLE_GAME_INVALID_STATE; });
        },
        .flute_command=+[](IsleRiverLifecycleSource* p0,int32_t* p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.native.host->flute_command(p1) : ISLE_GAME_INVALID_STATE; });
        },
        .block_next=+[](IsleRiverLifecycleSource* p0,IsleRiverBlock* p1,IsleRiverBlock** p2) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!p2) return ISLE_GAME_INVALID_ARGUMENT; *p2=nullptr; world::Block* next=nullptr; auto result=a.native.block_next(block(p1),&next); if (result==ISLE_GAME_OK) *p2=block(next); return result; });
        },
        .block_position=+[](IsleRiverLifecycleSource* p0,IsleRiverBlock* p1,IsleGameVec3* p2) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.native.block_position(block(p1),p2); });
        },
        .displace_items=+[](IsleRiverLifecycleSource* p0,float p1,float p2,float p3) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() && a.native.host->items_owner()->displace({p1,p2,p3}) ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE; });
        },
        .load_asset=+[](IsleRiverLifecycleSource* p0,IsleRiverLifecycleAssetId p1,IsleRiverLifecycleAsset** p2) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!p2) return ISLE_GAME_INVALID_ARGUMENT; *p2=nullptr; const auto index=std::size_t(p1); if (index>=4) return ISLE_GAME_INVALID_ARGUMENT; course::SceneAsset* asset=nullptr; auto result=a.native.asset(index,a.native.assets[index].first,a.native.assets[index].end,&asset); if (result==ISLE_GAME_OK) *p2=reinterpret_cast<IsleRiverLifecycleAsset*>(asset); return result; });
        },
        .fx_bank=+[](IsleRiverLifecycleSource* p0,int32_t p1,IsleRiverLifecycleAsset* p2,IsleRiverLifecycleAsset* p3) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.native.bank(p1,reinterpret_cast<const course::SceneAsset*>(p2),reinterpret_cast<const course::SceneAsset*>(p3),menu::EffectGroup::river); });
        },
        .fx_camera=+[](IsleRiverLifecycleSource* p0,int32_t p1,int32_t p2,IsleGameObject* p3) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!a.active()) return ISLE_GAME_INVALID_STATE; IsleGameCamera* camera=nullptr; auto result=isle_game_object_camera(a.native.host->game(),p3,&camera); if (result!=ISLE_GAME_OK) return result; IsleGameObject* object=nullptr; result=a.native.fx_camera(p1,p2,camera,&object); return result; });
        },
        .fx_allocate=+[](IsleRiverLifecycleSource* p0,int32_t p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { IsleGameObject* object=nullptr; return a.native.fx_pool(p1,&object); });
        },
        .fx_dither=+[](IsleRiverLifecycleSource* p0,int32_t p1,int32_t p2) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.native.dither(std::uint32_t(p1),std::uint32_t(p2)); });
        },
        .depth_allocate=+[](IsleRiverLifecycleSource* p0,uint32_t p1,uint32_t p2,void** p3) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.native.host->allocate_depth(std::uint32_t(p1),std::uint32_t(p2),p3) : ISLE_GAME_INVALID_STATE; });
        },
        .screen=+[](IsleRiverLifecycleSource* p0,const IsleRiverLifecycleScreen* p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!a.native.setup() || p1!=&a.source.screen) return ISLE_GAME_INVALID_ARGUMENT; return a.native.host->apply_screen({p1->width,p1->height,p1->flags,{p1->framebuffer_identity[0],p1->framebuffer_identity[1],p1->framebuffer_identity[2]}},p1->depth_buffer); });
        },
        .fx_bank_count=+[](IsleRiverLifecycleSource* p0,size_t* p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!p1 || !a.native.setup()) return ISLE_GAME_INVALID_ARGUMENT; *p1=menu::EffectRuntime::bank_count; return ISLE_GAME_OK; });
        },
        .fx_bank_reset=+[](IsleRiverLifecycleSource* p0,size_t p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.native.setup() ? a.native.host->reset_effect_bank(p1) : ISLE_GAME_INVALID_STATE; });
        },
        .photo_reset=+[](IsleRiverLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.native.setup() ? a.native.host->reset_photo_roll() : ISLE_GAME_INVALID_STATE; });
        },
        .pokemons_init=+[](IsleRiverLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!a.native.setup()) return ISLE_GAME_INVALID_STATE; if (!a.native.helper) a.native.helper=a.native.host->reference_owner(); IsleGameObjectMetadata metadata{}; if (!a.native.helper || isle_game_object_metadata(a.native.host->game(),a.native.helper,&metadata)!=ISLE_GAME_OK || metadata.id!=0 || metadata.update_link!=0 || metadata.priority!=0x80000000u) return ISLE_GAME_INVALID_STATE; return a.native.host->initialize_collisions(a.native.helper); });
        },
        .background=+[](IsleRiverLifecycleSource* p0,uint8_t* p1,uint8_t* p2,uint8_t* p3) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.native.background(p1,p2,p3); });
        },
        .create_cameras=+[](IsleRiverLifecycleSource* p0,uint32_t p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.native.setup() ? a.native.host->create_cameras(std::uint32_t(p1)) : ISLE_GAME_INVALID_STATE; });
        },
        .ui_bind=+[](IsleRiverLifecycleSource* p0,IsleRiverLifecycleWorldSingle p1,IsleRiverLifecycleMovement p2,IsleGameSourceCallback p3,uint8_t p4,IsleRiverItemCollision p5) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (p1!=func_802D89B4_6C0794 || p2!=func_802D92B0_6C1090 || p3 || p4 || p5!=func_802D8E98_6C0C78) return ISLE_GAME_INVALID_ARGUMENT; return a.native.initialize_ui(); });
        },
        .end_callback=+[](IsleRiverLifecycleSource* p0,IsleRiverLifecycleEnd p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!a.native.setup() || p1!=func_802D88F8_6C06D8 || a.native.end_bound || a.native.player()->EndLevelCb!=Adapter::end || a.native.player()->end_level_user!=&a) return ISLE_GAME_INVALID_ARGUMENT; a.native.end_bound=true; return ISLE_GAME_OK; });
        },
        .pause_callback=+[](IsleRiverLifecycleSource* p0,IsleRiverLifecycleEnd p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!a.native.setup() || p1!=func_802D89AC_6C078C || a.native.pause_bound || a.native.player()->PauseCb!=Adapter::pause || a.native.player()->pause_user!=&a) return ISLE_GAME_INVALID_ARGUMENT; a.native.pause_bound=true; return ISLE_GAME_OK; });
        },
        .environment_init=+[](IsleRiverLifecycleSource* p0,const IsleRiverLifecycleEnvironment* p1,size_t p2) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (p1!=a.source.environment) return ISLE_GAME_INVALID_ARGUMENT; return a.native.environment(p1,p2,62,+[](const void* p,std::size_t n){return static_cast<const IsleRiverLifecycleEnvironment*>(p)[n].sound;},+[](const void* p,std::size_t n){return static_cast<const IsleRiverLifecycleEnvironment*>(p)[n].pitch_modifier;},+[](const void* p,std::size_t n){return static_cast<const IsleRiverLifecycleEnvironment*>(p)[n].hearing_range;}); });
        },
        .detector_create=+[](IsleRiverLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.native.setup() ? a.native.host->initialize_detector(a.native.helper) : ISLE_GAME_INVALID_STATE; });
        },
        .detector_enable=+[](IsleRiverLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.native.host->detector_enabled(true) : ISLE_GAME_INVALID_STATE; });
        },
        .intervals=+[](IsleRiverLifecycleSource* p0,uint16_t p1,uint16_t p2) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.entry->intervals(p1,p2); });
        },
        .near_clipping=+[](IsleRiverLifecycleSource* p0,int32_t p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (p1!=0 && p1!=1) return ISLE_GAME_INVALID_ARGUMENT; return a.entry->near_clipping(p1!=0); });
        },
        .photo_count=+[](IsleRiverLifecycleSource* p0,int32_t* p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!p1 || a.native.joined()!=ISLE_GAME_OK) return ISLE_GAME_INVALID_STATE; *p1=a.native.photos->count(); return ISLE_GAME_OK; });
        },
        .request_scene=+[](IsleRiverLifecycleSource* p0,const IsleRiverLifecycleSceneSetup* p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (a.actors.base.phase!=ISLE_COURSE_ACTOR_PREPARED || a.source.scene_setup.heap_size!=1300768u) return ISLE_GAME_INVALID_STATE; return a.native.request(p1,&a.source.scene_setup); });
        },
        .scene_joined=+[](IsleRiverLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.native.joined(); });
        },
        .source_process=+[](IsleRiverLifecycleSource* p0,IsleGameObject* p1,IsleGameSourceCallback p2,int32_t p3,int32_t p4,IsleGameProcess** p5) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (p2!=func_802E22E4_6CA0C4 && p2!=func_802E222C_6CA00C) return ISLE_GAME_INVALID_ARGUMENT; return a.native.process(p1,p2,p3,p4,p5); });
        },
        .reference_assign=+[](IsleRiverLifecycleSource* p0,IsleGameObject** p1,IsleGameObject* p2) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.native.reference(p1,p2) : ISLE_GAME_INVALID_STATE; });
        },
    };
}
}
} // namespace isle::river
extern "C" IsleGameResult isle_river_lifecycle_native_context(IsleGame* game,IsleGameObject* object,IsleRiverLifecycleSource** output) { return isle::river::Adapter::native_context(game,object,output); }
extern "C" IsleGameResult isle_river_lifecycle_native_admission(const IsleRiverLifecycleSource* source,const IsleRiverLifecycleServices* services) {
    return isle::river::Adapter::invoke(const_cast<IsleRiverLifecycleSource*>(source),[&](isle::river::Adapter& a) { return services==&a.lifecycle_services && source->services==services ? a.validate_admission() : ISLE_GAME_INVALID_STATE; });
}
extern "C" IsleGameResult isle_course_scene_river_owner_transition(IsleRiverLifecycleSource* source,int32_t reason) {
    return isle::river::Adapter::invoke(source,[&](isle::river::Adapter& a) { return a.active() ? a.native.manager->transition_end(reason) : ISLE_GAME_INVALID_STATE; });
}
