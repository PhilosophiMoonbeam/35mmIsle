#include "cave_scene.hpp"
#include "menu_scene_bridge.h"
#include "environment_sound.hpp"
#include <limits>
namespace isle::cave {
namespace {
IsleCaveWorldBlock* block(world::Block* value) noexcept { return reinterpret_cast<IsleCaveWorldBlock*>(value); }
world::Block* block(IsleCaveWorldBlock* value) noexcept { return reinterpret_cast<world::Block*>(value); }
constexpr std::array<actor::RetainedFieldFamily,1> retained{{{39,0x802c7590u,0u,4u}}};
IsleCaveLifecycleServices make_lifecycle_services() noexcept;
}
Adapter::Adapter() noexcept : actor_services(course::make_cave_actor_services()), lifecycle_services(make_lifecycle_services()) {}
IsleGameResult Adapter::prepare(void* owner,IsleContinuationDomain* domain,std::span<const std::byte> rom,std::int32_t argument) noexcept {
    if (!owner || !domain || entry || source.owner || actors.base.owner || course_state) return ISLE_GAME_INVALID_STATE;
    entry=static_cast<course::Lifecycle<Adapter>*>(owner);
    native.manager=&entry->manager(); native.photos=&entry->photos(); native.state=&entry->game_state();
    native.input=&entry->input(); native.audio=&entry->audio(); native.snapshot=&entry->snapshot();
    native.receipt=&entry->scene_join_receipt(); native.native_profile=entry->native_profile();
    if (rom.data()!=native.snapshot->canonical().data() || rom.size()!=isle::rom::us_size) return ISLE_GAME_INVALID_ARGUMENT;
    auto result=egg_geometry.prepare(*native.snapshot,actors,this);
    if (result==ISLE_GAME_OK) result=isle_cave_actor_prepare(&actors,this,reinterpret_cast<const std::uint8_t*>(rom.data()),rom.size(),egg_geometry.preparation());
    if (result==ISLE_GAME_OK) result=isle_cave_lifecycle_prepare(&source,this,domain,&actors,&lifecycle_services,reinterpret_cast<const std::uint8_t*>(rom.data()),rom.size(),argument);
    if (result!=ISLE_GAME_OK) return result;
    try {
        course_state.emplace(actors,source,actor_services);
        if (!egg_geometry.source_matches()) return ISLE_GAME_INVALID_STATE;
        course_state->native_egg_geometry=&egg_geometry;
        native.prepare_capacity(4,reference_fields.size());
        constexpr std::array<std::array<std::uint32_t,2>,4> ranges{{{0xab5860u,0xab5980u},{0xab5980u,0xab85e0u},{11298960u,11299456u},{11299456u,11306256u}}};
        for (std::size_t i=0;i<ranges.size();++i) native.assets[i]={ranges[i][0],ranges[i][1],rom.subspan(ranges[i][0],ranges[i][1]-ranges[i][0]),false};
        return ISLE_GAME_OK;
    } catch (...) { if (source.status==ISLE_GAME_OK) source.status=ISLE_GAME_INVALID_STATE; return source.status; }
}
course::NativeDescription Adapter::description() {
    if (!course_state || !native.requested || native.host) throw std::logic_error("typed scene description requires its one Source request");
    course::NativeDescription result;
    result.course_id=4; result.effects=menu::EffectGroup::cave; result.capacity=native.capacity;
    result.player={4,end,this,pause,this,{this,exit_block,movement},nullptr,0,func_802BDDCC_64027C};
    result.world={this,add,remove,change,nullptr,nullptr,nullptr};
    result.input={1u<<9,9,0,9,isle_course_scene_input_update};
    result.initial_helper={0,0x80000000u,0,func_802BE09C_64054C};
    result.callbacks=actor_callbacks(); result.graphs=actor_graphs(); result.weighted_states=actor_weighted_states(); result.roots=actor_roots();
    result.retained_fields={retained}; result.reference_fields=reference_fields;
    result.source_vertices=egg_geometry.publications();
    return result;
}
actor::CourseContext Adapter::context() { if (!course_state) throw std::logic_error("typed CourseState not prepared"); return actor::CourseContext(*course_state); }
IsleGameResult Adapter::bind_scene(course::NativeHost& host) noexcept {
    if (native.host || !course_state || !host.closure_prepared() || !host.root() || !host.actors() || host.root()->bound_actors()!=host.actors()) return ISLE_GAME_INVALID_STATE;
    try { if (&host.actors()->course_context().cave()!=&*course_state) return ISLE_GAME_INVALID_STATE; } catch (...) { return ISLE_GAME_INVALID_STATE; }
    const auto geometry_result=egg_geometry.attach(host);
    if (geometry_result!=ISLE_GAME_OK) return geometry_result;
    native.host=&host; course_state->native_host=&host;
    return ISLE_GAME_OK;
}
bool Adapter::active() const noexcept {
    return native.host && course_state && course_state->native_host==native.host && actors.base.phase==ISLE_COURSE_ACTOR_ACTIVE && actors.base.nativeGame==native.host->game() && actors.services==&actor_services && source.owner==this && source.actors==&actors;
}
IsleGameResult Adapter::validate_admission() const noexcept {
    if (!entry || source.owner!=this || actors.base.owner!=this || source.actors!=&actors || !source.domain || actors.base.course_id!=4 || !course_state || &course_state->actors!=&actors || &course_state->source!=&source || &course_state->actor_services!=&actor_services || course_state->native_egg_geometry!=&egg_geometry || !egg_geometry.source_matches()) return ISLE_GAME_INVALID_STATE;
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
    return isle_cave_actor_activate(&actors,host.game(),&actor_services);
}
IsleGameResult Adapter::invoke_init() noexcept { return active() ? isle_cave_lifecycle_invoke_init(&source) : ISLE_GAME_INVALID_STATE; }
IsleGameResult Adapter::release_references(course::NativeHost& host) noexcept {
    if (native.host && native.host!=&host) return ISLE_GAME_INVALID_STATE;
    return native.release_references();
}
IsleGameResult Adapter::deactivate(course::NativeHost& host) noexcept {
    if (actors.base.phase==ISLE_COURSE_ACTOR_PREPARED) return !actors.base.nativeGame && !actors.services && !actors.base.nonwaiting_call_frame ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE;
    if (actors.base.phase==ISLE_COURSE_ACTOR_DETACHED) return !actors.base.nativeGame && !actors.services ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE;
    if (actors.base.phase!=ISLE_COURSE_ACTOR_ACTIVE || native.host!=&host || !host.detach_ready()) return ISLE_GAME_INVALID_STATE;
    return isle_cave_actor_deactivate(&actors);
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
    if (result!=ISLE_GAME_OK && actors.base.phase==ISLE_COURSE_ACTOR_PREPARED) return isle_cave_lifecycle_report_failure(&source,result);
    return isle_cave_lifecycle_complete_scene(&source,result);
}
IsleGameResult Adapter::native_context(IsleGame* game,IsleGameObject* object,LifecycleSource** output) noexcept {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT; *output=nullptr;
    if (!game) return ISLE_GAME_INVALID_ARGUMENT;
    IslePlayerSource* player=nullptr; auto result=isle_player_source_context(game,&player);
    if (result!=ISLE_GAME_OK) return result;
    if (!player || !player->native_root || player->game!=game) return ISLE_GAME_INVALID_STATE;
    auto& root=*static_cast<isle::player::Root*>(player->native_root); auto* owner=root.bound_actors();
    if (&root.source()!=player || !owner || &owner->player_source()!=player || owner->course_context().course_id()!=4) return ISLE_GAME_INVALID_STATE;
    try {
        auto& selected=owner->course_context().cave();
        if (selected.source.actors!=&selected.actors || selected.source.owner!=selected.actors.base.owner || !selected.source.owner) return ISLE_GAME_INVALID_STATE;
        auto& self=*static_cast<Adapter*>(selected.source.owner);
        if (&self.source!=&selected.source || &self.actors!=&selected.actors || !self.course_state || &*self.course_state!=&selected || &self.actor_services!=&selected.actor_services || !self.active() || self.native.host->root()!=&root || self.native.host->actors()!=owner || (object && !self.native.receiver(object))) return ISLE_GAME_INVALID_STATE;
        *output=&selected.source; return ISLE_GAME_OK;
    } catch (...) { return ISLE_GAME_INVALID_STATE; }
}
bool Adapter::add(void* user,world::Block& first,world::Block& second) noexcept { auto& a=*static_cast<Adapter*>(user); return isle_cave_lifecycle_invoke_world_pair(&a.source,func_802BDF34_6403E4,block(&first),block(&second))==ISLE_GAME_OK; }
bool Adapter::change(void* user,world::Block& first,world::Block& second) noexcept { auto& a=*static_cast<Adapter*>(user); return isle_cave_lifecycle_invoke_world_pair(&a.source,func_802BDF58_640408,block(&first),block(&second))==ISLE_GAME_OK; }
bool Adapter::remove(void* user,world::Block& value) noexcept { auto& a=*static_cast<Adapter*>(user); return isle_cave_lifecycle_invoke_world_single(&a.source,func_802BDF7C_64042C,block(&value))==ISLE_GAME_OK; }
bool Adapter::exit_block(void* user,world::Block& value) noexcept { auto& a=*static_cast<Adapter*>(user); return isle_cave_lifecycle_invoke_world_single(&a.source,func_802BE0AC_64055C,block(&value))==ISLE_GAME_OK; }
bool Adapter::movement(void* user,std::size_t index,float fraction) noexcept { auto& a=*static_cast<Adapter*>(user); return index<=std::size_t(INT32_MAX) && isle_cave_actor_invoke_movement(&a.actors,std::int32_t(index),fraction)==ISLE_GAME_OK; }
IsleGameResult Adapter::end(void* user,std::int32_t reason) noexcept { auto& a=*static_cast<Adapter*>(user); return isle_cave_lifecycle_invoke_end(&a.source,reason); }
IsleGameResult Adapter::pause(void* user,std::int32_t reason) noexcept { auto& a=*static_cast<Adapter*>(user); return isle_cave_lifecycle_invoke_end_callback(&a.source,func_802BE0A4_640554,reason); }
IsleGameResult Adapter::block_change(IsleCourseActorBase* base,std::uint32_t identity,IsleGameObject* object,const IsleGameVec3* previous,const IsleGameVec3* current) noexcept {
    if (!base || !object || !previous || !current) return ISLE_GAME_INVALID_ARGUMENT;
    int policy;
    switch (identity) { case 0x80363dbcu: policy=0; break; case 0x80363eb4u: policy=1; break; case 0x80362d2cu: policy=2; break; default:  return ISLE_GAME_INVALID_ARGUMENT; }
    return isle_meowth_source_call_change_block(object,previous->x,previous->y,previous->z,current->x,current->y,current->z,policy);
}
IsleGameResult Adapter::block_remove(IsleCourseActorBase* base,std::uint32_t identity,IsleGameObject* object) noexcept {
    if (!base || !object || base->course_id!=4) return ISLE_GAME_INVALID_ARGUMENT;
    LifecycleSource* source=nullptr; auto result=native_context(base->nativeGame,object,&source); if (result!=ISLE_GAME_OK) return result;
    auto& self=*static_cast<Adapter*>(source->owner);
    if (&self.actors.base!=base) return ISLE_GAME_INVALID_STATE;
    for (const auto& callback:actor_callbacks()) if (callback.identity==identity) return isle_course_scene_invoke_object(base,callback.function,object);
    return ISLE_GAME_INVALID_ARGUMENT;
}
namespace {
bool valid_definitions(const Adapter& adapter,const IsleCaveLifecycleDefinition* rows,std::size_t count) noexcept {
    if (rows!=adapter.source.definitions || count!=std::size(adapter.source.definitions)) return false;
    // Source passes all 18 rows; the ROM catalog stores 17 live rows and
    // validates the zero terminator separately from its live definition span.
    const auto& terminal=rows[count-1];
    return !terminal.id && !terminal.initialize_identity && !terminal.update_identity &&
        !terminal.remove_identity && adapter.native.definitions(rows,count-1,0x802c6234u);
}
IsleCaveLifecycleServices make_lifecycle_services() noexcept {
    return {
        .reference=+[](IsleCaveLifecycleSource* p0,IsleGameObject** p1,IsleGameObject* p2) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.native.reference(p1,p2) : ISLE_GAME_INVALID_STATE; });
        },
        .block_position=+[](IsleCaveLifecycleSource* p0,IsleCaveWorldBlock* p1,IsleGameVec3* p2) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.native.block_position(block(p1),p2); });
        },
        .sound_params=+[](IsleCaveLifecycleSource* p0,int32_t p1,int32_t p2,int32_t p3,float p4,int32_t p5,int32_t* p6) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.native.sound(p1,p2,p3,p4,p5,p6); });
        },
        .sound_active=+[](IsleCaveLifecycleSource* p0,int32_t p1,int32_t* p2) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!p2) return ISLE_GAME_INVALID_ARGUMENT; *p2=a.native.audio->playing_sound(std::uint32_t(p1)); return ISLE_GAME_OK; });
        },
        .sound_stop=+[](IsleCaveLifecycleSource* p0,int32_t p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return course::SceneMechanics::audio_result(a.native.audio->stop_sound(std::uint32_t(p1))); });
        },
        .sound_stop_all=+[](IsleCaveLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return course::SceneMechanics::audio_result(a.native.audio->stop_sounds()); });
        },
        .bgm_stop_all=+[](IsleCaveLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return course::SceneMechanics::audio_result(a.native.audio->stop_bgm()); });
        },
        .camera_reset=+[](IsleCaveLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? isle_player_root_invoke_reset_main_camera_settings(a.native.player()) : ISLE_GAME_INVALID_STATE; });
        },
        .camera_speed=+[](IsleCaveLifecycleSource* p0,IsleCaveCamera* p1,float p2) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.actor_services.camera_speed(&a.actors,p1,p2) : ISLE_GAME_INVALID_STATE; });
        },
        .camera_animation=+[](IsleCaveLifecycleSource* p0,IsleCaveCamera* p1,uint32_t p2,float p3) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.actor_services.camera_animation(&a.actors,p1,p2,p3) : ISLE_GAME_INVALID_STATE; });
        },
        .player_animation=+[](IsleCaveLifecycleSource* p0,uint32_t p1,uint32_t p2,float p3,float p4) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.actor_services.player_animation(&a.actors,p1,p2,p3,p4) : ISLE_GAME_INVALID_STATE; });
        },
        .animation_callback=+[](IsleCaveLifecycleSource* p0,IsleGameObject* p1,IsleCaveLifecycleAnimationCallback p2) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!a.active() || p2!=func_802C5BB0_648060 || !a.native.host->cart()->instance(p1)) return ISLE_GAME_INVALID_ARGUMENT; return a.native.host->cart()->set_callback(p1,isle_course_scene_cave_cart_event,&a.source) ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE; });
        },
        .main_camera=+[](IsleCaveLifecycleSource* p0,IsleCaveCamera** p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.actor_services.main_camera(&a.actors,p1) : ISLE_GAME_INVALID_STATE; });
        },
        .player_model_init=+[](IsleCaveLifecycleSource* p0,IsleGameObject** p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.actor_services.player_model_init(&a.actors,p1) : ISLE_GAME_INVALID_STATE; });
        },
        .camera_process=+[](IsleCaveLifecycleSource* p0,IsleGameObject* p1,int32_t p2,int32_t p3,IsleGameProcess** p4) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!a.active() || p2!=1 || p3!=1 || !p4) return ISLE_GAME_INVALID_ARGUMENT; IsleCaveCamera* camera=nullptr; auto result=a.actor_services.main_camera(&a.actors,&camera); IsleGameObject* object=nullptr; if (result==ISLE_GAME_OK) result=a.actor_services.camera_object(&a.actors,camera,&object); if (result!=ISLE_GAME_OK) return result; if (p1!=object) return ISLE_GAME_INVALID_ARGUMENT; return a.native.process(object,isle_course_scene_camera_update,p2,p3,p4); });
        },
        .player_process=+[](IsleCaveLifecycleSource* source,IsleGameObject* object,IsleGameSourceCallback callback,int32_t kind,int32_t priority,IsleGameProcess** output) noexcept -> IsleGameResult {
            return Adapter::invoke(source,[&](Adapter& a) {
                if (!a.active() || !output || kind!=0 || priority!=1 ||
                    object!=a.actors.D_802C8044_64A4F4 || !a.native.host->cart() ||
                    !a.native.host->cart()->instance(object) ||
                    (callback!=func_802C5DFC_6482AC && callback!=func_802C5CEC_64819C &&
                     callback!=func_802C5D80_648230)) return ISLE_GAME_INVALID_ARGUMENT;
                return a.native.process(object,callback,kind,priority,output);
            });
        },
        .effect_create=+[](IsleCaveLifecycleSource* p0,int32_t p1,int32_t p2,IsleCaveEffect** p3) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!p3) return ISLE_GAME_INVALID_ARGUMENT; *p3=nullptr; course::SceneEffect* effect=nullptr; auto result=a.native.effect(p1,p2,&effect); if (result==ISLE_GAME_OK && effect) *p3=reinterpret_cast<IsleCaveEffect*>(effect); return result; });
        },
        .effect_model=+[](IsleCaveLifecycleSource* p0,IsleCaveEffect* p1,IsleActorNodeRef p2) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!a.active() || !a.native.host->cart()->callback_matches(p2.model.object,isle_course_scene_cave_cart_event,&a.source)) return ISLE_GAME_INVALID_ARGUMENT; IslePlayerCartNode node{p2.model,p2.node}; return a.native.attach(reinterpret_cast<course::SceneEffect*>(p1),&node); });
        },
        .camera_read=+[](IsleCaveLifecycleSource* p0,IsleCaveCamera* p1,uint32_t p2,float* p3) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { IsleGameObject* object=nullptr; auto result=a.actor_services.camera_object(&a.actors,p1,&object); if (result!=ISLE_GAME_OK) return result; IsleGameCamera* camera=nullptr; result=isle_game_object_camera(a.native.host->game(),object,&camera); if (result!=ISLE_GAME_OK) return result; return a.native.camera_component(camera,p2,p3,false); });
        },
        .camera_store=+[](IsleCaveLifecycleSource* p0,IsleCaveCamera* p1,uint32_t p2,float p3) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { IsleGameObject* object=nullptr; auto result=a.actor_services.camera_object(&a.actors,p1,&object); if (result!=ISLE_GAME_OK) return result; IsleGameCamera* camera=nullptr; result=isle_game_object_camera(a.native.host->game(),object,&camera); if (result!=ISLE_GAME_OK) return result; return a.native.camera_component(camera,p2,nullptr,true,p3); });
        },
        .input_pressed=+[](IsleCaveLifecycleSource* p0,uint32_t* p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!p1) return ISLE_GAME_INVALID_ARGUMENT; auto query=a.native.input->menu(a.native.state->controller); if (query.error!=game_input::Error::none || !query.input) return ISLE_GAME_INVALID_STATE; *p1=std::remove_reference_t<decltype(*p1)>(query.input->pressed); return ISLE_GAME_OK; });
        },
        .camera_object=+[](IsleCaveLifecycleSource* p0,IsleCaveCamera* p1,IsleGameObject** p2) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.actor_services.camera_object(&a.actors,p1,p2) : ISLE_GAME_INVALID_STATE; });
        },
        .height_map=+[](IsleCaveLifecycleSource* p0,uint32_t p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.native.map(p1,false); });
        },
        .ceiling_map=+[](IsleCaveLifecycleSource* p0,uint32_t p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.native.map(p1,true); });
        },
        .world_create=+[](IsleCaveLifecycleSource* p0,uint32_t p1,int32_t p2,int32_t p3,int32_t p4,int32_t p5,int32_t p6,IsleCaveWorldPairCallback p7,IsleCaveWorldSingleCallback p8,IsleCaveWorldPairCallback p9) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (p7!=func_802BDF34_6403E4 || p8!=func_802BDF7C_64042C || p9!=func_802BDF58_640408) return ISLE_GAME_INVALID_ARGUMENT; return a.native.world(p1,p2,p3,p4,p5,p6); });
        },
        .sky_follow=+[](IsleCaveLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.native.host->sky_follow() : ISLE_GAME_INVALID_STATE; });
        },
        .source_80363928=+[](IsleCaveLifecycleSource* p0,int32_t p1,int32_t p2,int32_t p3,int32_t p4) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.native.registry(p1,p2,p3,p4); });
        },
        .source_800A19D8=isle_course_scene_cave_transition_end,
        .detector_disable=+[](IsleCaveLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.native.host->detector_enabled(false) : ISLE_GAME_INVALID_STATE; });
        },
        .environment_cleanup=+[](IsleCaveLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.native.host->environment_cleanup() : ISLE_GAME_INVALID_STATE; });
        },
        .world_destroy=+[](IsleCaveLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.native.host->destroy_world() : ISLE_GAME_INVALID_STATE; });
        },
        .source_80356FBC=+[](IsleCaveLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? isle_course_scene_control(a.native.player(),isle_player_func_80356FBC_4F73CC) : ISLE_GAME_INVALID_STATE; });
        },
        .camera_remove=+[](IsleCaveLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() && a.native.host->renderer() ? a.native.host->renderer()->remove_main() : ISLE_GAME_INVALID_STATE; });
        },
        .source_800067DC=+[](IsleCaveLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.native.host->exit_scene() : ISLE_GAME_INVALID_STATE; });
        },
        .items_displace=+[](IsleCaveLifecycleSource* p0,float p1,float p2,float p3) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() && a.native.host->items_owner()->displace({p1,p2,p3}) ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE; });
        },
        .fx_bank=+[](IsleCaveLifecycleSource* p0,int32_t p1,IsleCaveLifecycleAsset* p2,IsleCaveLifecycleAsset* p3) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.native.bank(p1,reinterpret_cast<const course::SceneAsset*>(p2),reinterpret_cast<const course::SceneAsset*>(p3),menu::EffectGroup::cave); });
        },
        .fx_dither=+[](IsleCaveLifecycleSource* p0,int32_t p1,int32_t p2) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.native.dither(std::uint32_t(p1),std::uint32_t(p2)); });
        },
        .screen_apply=+[](IsleCaveLifecycleSource* p0,const IsleCaveLifecycleScreen* p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!a.native.setup() || p1!=&a.source.screen) return ISLE_GAME_INVALID_ARGUMENT; return a.native.host->apply_screen({p1->width,p1->height,p1->flags,{p1->framebuffer_identity[0],p1->framebuffer_identity[1],p1->framebuffer_identity[2]}},p1->depth_buffer); });
        },
        .fx_reset_bank=+[](IsleCaveLifecycleSource* p0,size_t p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.native.setup() ? a.native.host->reset_effect_bank(p1) : ISLE_GAME_INVALID_STATE; });
        },
        .source_8009CE00=+[](IsleCaveLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.native.setup() ? a.native.host->reset_photo_roll() : ISLE_GAME_INVALID_STATE; });
        },
        .pokemon_initialize=+[](IsleCaveLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!a.native.setup()) return ISLE_GAME_INVALID_STATE; if (!a.native.helper) a.native.helper=a.native.host->reference_owner(); IsleGameObjectMetadata metadata{}; if (!a.native.helper || isle_game_object_metadata(a.native.host->game(),a.native.helper,&metadata)!=ISLE_GAME_OK || metadata.id!=0 || metadata.update_link!=0 || metadata.priority!=0x80000000u) return ISLE_GAME_INVALID_STATE; return a.native.host->initialize_collisions(a.native.helper); });
        },
        .background_color=+[](IsleCaveLifecycleSource* p0,uint8_t* p1,uint8_t* p2,uint8_t* p3) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.native.background(p1,p2,p3); });
        },
        .camera_create=+[](IsleCaveLifecycleSource* p0,uint32_t p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.native.setup() ? a.native.host->create_cameras(std::uint32_t(p1)) : ISLE_GAME_INVALID_STATE; });
        },
        .camera_flags=+[](IsleCaveLifecycleSource* p0,uint32_t p1,uint32_t p2) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!a.active() || !a.native.player()->camera || (p2 & ~p1)) return ISLE_GAME_INVALID_ARGUMENT; IsleGameCameraRenderState state{}; auto result=isle_game_camera_render_state(a.native.host->game(),a.native.player()->camera,&state); if (result!=ISLE_GAME_OK) return result; state.flags=(state.flags & ~p1)|p2; return isle_game_camera_set_render_state(a.native.host->game(),a.native.player()->camera,&state); });
        },
        .ui_initialize=+[](IsleCaveLifecycleSource* p0,IsleCaveWorldSingleCallback p1,IsleCaveMovementCallback p2,IsleGameSourceCallback p3,uint8_t p4,IsleCaveItemCallback p5) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (p1!=func_802BE0AC_64055C || p2!=func_802BE450_640900 || p3 || p4 || p5!=func_802BDDCC_64027C) return ISLE_GAME_INVALID_ARGUMENT; return a.native.initialize_ui(); });
        },
        .end_callback=+[](IsleCaveLifecycleSource* p0,IsleCaveEndCallback p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!a.native.setup() || p1!=func_802BE038_6404E8 || a.native.end_bound || a.native.player()->EndLevelCb!=Adapter::end || a.native.player()->end_level_user!=&a) return ISLE_GAME_INVALID_ARGUMENT; a.native.end_bound=true; return ISLE_GAME_OK; });
        },
        .pause_callback=+[](IsleCaveLifecycleSource* p0,IsleCaveEndCallback p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!a.native.setup() || p1!=func_802BE0A4_640554 || a.native.pause_bound || a.native.player()->PauseCb!=Adapter::pause || a.native.player()->pause_user!=&a) return ISLE_GAME_INVALID_ARGUMENT; a.native.pause_bound=true; return ISLE_GAME_OK; });
        },
        .environment_initialize=+[](IsleCaveLifecycleSource* p0,const IsleCaveLifecycleEnvironment* p1,size_t p2) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (p1!=a.source.environment) return ISLE_GAME_INVALID_ARGUMENT; return a.native.environment(p1,p2,57,+[](const void* p,std::size_t n){return static_cast<const IsleCaveLifecycleEnvironment*>(p)[n].sound;},+[](const void* p,std::size_t n){return static_cast<const IsleCaveLifecycleEnvironment*>(p)[n].pitch_modifier;},+[](const void* p,std::size_t n){return static_cast<const IsleCaveLifecycleEnvironment*>(p)[n].hearing_range;}); });
        },
        .detector_create=+[](IsleCaveLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.native.setup() ? a.native.host->initialize_detector(a.native.helper) : ISLE_GAME_INVALID_STATE; });
        },
        .detector_enable=+[](IsleCaveLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? a.native.host->detector_enabled(true) : ISLE_GAME_INVALID_STATE; });
        },
        .pitch_limits=+[](IsleCaveLifecycleSource* p0,float p1,float p2) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? isle_player_root_invoke_set_pitch_limits(a.native.player(),p1,p2) : ISLE_GAME_INVALID_STATE; });
        },
        .intervals=+[](IsleCaveLifecycleSource* p0,int32_t p1,int32_t p2) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (p1<0 || p1>UINT16_MAX || p2<0 || p2>UINT16_MAX) return ISLE_GAME_INVALID_ARGUMENT; return a.entry->intervals(std::uint16_t(p1),std::uint16_t(p2)); });
        },
        .near_clipping=+[](IsleCaveLifecycleSource* p0,int32_t p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (p1!=0 && p1!=1) return ISLE_GAME_INVALID_ARGUMENT; return a.entry->near_clipping(p1!=0); });
        },
        .source_800AAED0=+[](IsleCaveLifecycleSource* p0,int32_t p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { a.native.state->add_scene_notices(std::uint32_t(p1)); return ISLE_GAME_OK; });
        },
        .source_803570B0=+[](IsleCaveLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? isle_course_scene_control(a.native.player(),isle_player_func_803570B0_4F74C0) : ISLE_GAME_INVALID_STATE; });
        },
        .source_80357120=+[](IsleCaveLifecycleSource* p0,IsleGameObject* p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() && a.native.receiver(p1) ? isle_course_scene_object_control(a.native.player(),isle_player_func_80357120_4F7530,p1) : ISLE_GAME_INVALID_STATE; });
        },
        .source_80357170=+[](IsleCaveLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? isle_course_scene_control(a.native.player(),isle_player_func_80357170_4F7580) : ISLE_GAME_INVALID_STATE; });
        },
        .source_803571C4=+[](IsleCaveLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? isle_course_scene_control(a.native.player(),isle_player_func_803571C4_4F75D4) : ISLE_GAME_INVALID_STATE; });
        },
        .source_803572B0=+[](IsleCaveLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.active() ? isle_course_scene_control(a.native.player(),isle_player_func_803572B0_4F76C0) : ISLE_GAME_INVALID_STATE; });
        },
        .block_next=+[](IsleCaveLifecycleSource* p0,IsleCaveWorldBlock* p1,IsleCaveWorldBlock** p2) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!p2) return ISLE_GAME_INVALID_ARGUMENT; *p2=nullptr; world::Block* next=nullptr; auto result=a.native.block_next(block(p1),&next); if (result==ISLE_GAME_OK) *p2=block(next); return result; });
        },
        .asset_load=+[](IsleCaveLifecycleSource* p0,IsleCaveLifecycleAssetId p1,IsleCaveLifecycleAsset** p2) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!p2) return ISLE_GAME_INVALID_ARGUMENT; *p2=nullptr; const auto index=std::size_t(p1); if (index>=4) return ISLE_GAME_INVALID_ARGUMENT; course::SceneAsset* asset=nullptr; auto result=a.native.asset(index,a.native.assets[index].first,a.native.assets[index].end,&asset); if (result==ISLE_GAME_OK) *p2=reinterpret_cast<IsleCaveLifecycleAsset*>(asset); return result; });
        },
        .fx_pool=+[](IsleCaveLifecycleSource* p0,int32_t p1,IsleGameObject** p2) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.native.fx_pool(p1,p2); });
        },
        .fx_manager=+[](IsleCaveLifecycleSource* p0,int32_t p1,int32_t p2,IsleCaveCamera* p3,IsleGameObject** p4) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!a.active()) return ISLE_GAME_INVALID_STATE; IsleGameObject* object=nullptr; auto result=a.actor_services.camera_object(&a.actors,p3,&object); if (result!=ISLE_GAME_OK) return result; IsleGameCamera* camera=nullptr; result=isle_game_object_camera(a.native.host->game(),object,&camera); return result==ISLE_GAME_OK ? a.native.fx_camera(p1,p2,camera,p4) : result; });
        },
        .allocate=+[](IsleCaveLifecycleSource* p0,size_t p1,size_t p2,void** p3) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (p1>UINT32_MAX || p2>UINT32_MAX) return ISLE_GAME_INVALID_ARGUMENT; return a.active() ? a.native.host->allocate_depth(std::uint32_t(p1),std::uint32_t(p2),p3) : ISLE_GAME_INVALID_STATE; });
        },
        .add_helper=+[](IsleCaveLifecycleSource* p0,int32_t p1,IsleGameSourceCallback p2,uint8_t p3,uint32_t p4,IsleGameObject** p5) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (p2!=func_802BE09C_64054C) return ISLE_GAME_INVALID_ARGUMENT; return a.native.helper_create(p1,p2,p3,p4,p5); });
        },
        .source_8009BC68=+[](IsleCaveLifecycleSource* p0,int32_t* p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!p1 || a.native.joined()!=ISLE_GAME_OK) return ISLE_GAME_INVALID_STATE; *p1=a.native.photos->count(); return ISLE_GAME_OK; });
        },
        .scene_request=+[](IsleCaveLifecycleSource* p0,const IsleCaveLifecycleSceneSetup* p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (a.actors.base.phase!=ISLE_COURSE_ACTOR_PREPARED || a.source.scene_setup.heap_size!=1109776u) return ISLE_GAME_INVALID_STATE; return a.native.request(p1,&a.source.scene_setup); });
        },
        .pokemon_add=+[](IsleCaveLifecycleSource* p0,IsleCaveWorldBlock* p1,IsleCaveWorldBlock* p2,const IsleCaveLifecycleDefinition* p3,size_t p4) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!a.active()) return ISLE_GAME_INVALID_STATE; if (!valid_definitions(a,p3,p4)) return ISLE_GAME_INVALID_ARGUMENT; return a.native.pokemon_add(block(p1),block(p2),0x802c6234u,17); });
        },
        .pokemon_change=+[](IsleCaveLifecycleSource* p0,IsleCaveWorldBlock* p1,IsleCaveWorldBlock* p2,const IsleCaveLifecycleDefinition* p3,size_t p4) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!a.active()) return ISLE_GAME_INVALID_STATE; if (!valid_definitions(a,p3,p4)) return ISLE_GAME_INVALID_ARGUMENT; return a.native.pokemon_change(block(p1),block(p2),0x802c6234u,17,&a.actors.base,Adapter::block_change); });
        },
        .pokemon_remove=+[](IsleCaveLifecycleSource* p0,IsleCaveWorldBlock* p1,const IsleCaveLifecycleDefinition* p2,size_t p3) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { if (!a.active()) return ISLE_GAME_INVALID_STATE; if (!valid_definitions(a,p2,p3)) return ISLE_GAME_INVALID_ARGUMENT; return a.native.pokemon_remove(block(p1),0x802c6234u,17,&a.actors.base,Adapter::block_remove); });
        },
        .fx_manager_assign=+[](IsleCaveLifecycleSource* p0,IsleGameObject* p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.native.fx_publish(0x80382d10u,p1); });
        },
        .fx_pool_assign=+[](IsleCaveLifecycleSource* p0,IsleGameObject* p1) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.native.fx_publish(0x80382d14u,p1); });
        },
        .scene_joined=+[](IsleCaveLifecycleSource* p0) noexcept -> IsleGameResult {
            return Adapter::invoke(p0,[&](Adapter& a) { return a.native.joined(); });
        },
    };
}
}
} // namespace isle::cave
extern "C" IsleGameResult isle_cave_lifecycle_native_context(IsleGame* game,IsleGameObject* object,IsleCaveLifecycleSource** output) { return isle::cave::Adapter::native_context(game,object,output); }
extern "C" IsleGameResult isle_cave_lifecycle_native_animation_node(IsleCaveLifecycleSource* source,const IsleActorNodeRef* node) {
    return isle::cave::Adapter::invoke(source,[&](isle::cave::Adapter& a) {
        if (!a.active() || !node || !a.native.host->cart()->callback_matches(node->model.object,isle_course_scene_cave_cart_event,&a.source) || !a.native.host->cart()->issued_effect_node(node->model,node->node)) return ISLE_GAME_INVALID_ARGUMENT;
        return ISLE_GAME_OK;
    });
}
extern "C" IsleGameResult isle_course_scene_cave_owner_transition(IsleCaveLifecycleSource* source,int32_t reason) {
    return isle::cave::Adapter::invoke(source,[&](isle::cave::Adapter& a) { return a.active() ? a.native.manager->transition_end(reason) : ISLE_GAME_INVALID_STATE; });
}
