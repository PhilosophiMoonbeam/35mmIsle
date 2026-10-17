// Explicit opt-in private-ROM CPU consumer; never CTest/default discovery.
// Uses actual selected GAME FSM/path/factories, canonical Runtime/native models,
// World/ground, shared FX/RNG/audio and real OM retirement. No GPU proof here.
#include "actor_owner.hpp"
#include "actual_audio_driver.hpp"
#include "actual_player_source.hpp"
#include "effect_bank_resources.hpp"
#include "scene_storage.hpp"
#include "source_effect_world.hpp"
#include "magikarp_owner_probe.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <utility>

namespace {
using namespace isle;
namespace a=menu::animation;
void require(bool value,const char* text) { if (!value) throw std::runtime_error(text); }
void native(IsleGameResult value) { require(value==ISLE_GAME_OK,"native Magikarp consumer operation failed"); }
void accepted(actor::OwnerStatus value) { require(bool(value),value.detail); }
bool near(float x,float y) { return std::abs(x-y)<0.0001f; }
bool equal(IsleGameVec3 x,IsleGameVec3 y) { return near(x.x,y.x)&&near(x.y,y.y)&&near(x.z,y.z); }
bool counts_equal(IsleGameCounts x,IsleGameCounts y) {
    return x.objects==y.objects&&x.processes==y.processes&&x.sprites==y.sprites&&x.cameras==y.cameras&&
        x.matrices==y.matrices&&x.models==y.models&&x.model_nodes==y.model_nodes;
}
struct Shift {
    scene::Services* services=nullptr;
    IsleGameCamera* camera=nullptr;
    actor::Owner* owner=nullptr;
    static void camera_shift(void* user,world::Vec3 delta) noexcept {
        auto& self=*static_cast<Shift*>(user);
        const IsleGameCameraParameters* parameters=nullptr;
        if (!self.services->check(isle_game_camera_parameters(self.services->game,self.camera,&parameters),"Magikarp camera shift borrow")) return;
        auto look=parameters->look_at;
        const std::array d{delta.x,delta.y,delta.z};
        for (unsigned i=0;i<3;++i) { look.eye[i]+=d[i]; look.at[i]+=d[i]; }
        self.services->check(isle_game_camera_set_look_at(self.services->game,self.camera,&look),"Magikarp camera shift");
    }
    static void effects_shift(void* user,world::Vec3 delta) noexcept {
        auto& self=*static_cast<Shift*>(user);
        if (!self.owner) { self.services->fail("Magikarp effects shift without canonical owner"); return; }
        self.owner->effects().translate({delta.x,delta.y,delta.z});
    }
};
struct YawObservation {
    scene::Services* services;
    unsigned calls=0;
    float expected=0;
    static void capture(IsleGameObject*,void* user) noexcept {
        auto& self=*static_cast<YawObservation*>(user);
        if (self.calls++==0) {
            auto rng=self.services->rng;
            // Observer runs at native priority8, immediately before genuine
            // state7. It observes, but does not consume, the one shared RNG.
            // Explicit native C-double PI policy; Original macro/type unproven.
            self.expected=static_cast<float>(isle_rng_range(&rng,360)*3.14159265358979323846/180.0f);
        }
    }
};
struct Consumer {
    scene::Services& services;
    actor::Owner& owner;
    world::World& world;
    const beach::ActorResources& resources;
    std::size_t magikarp=beach::actor_no_index,common=beach::actor_no_index,unsupported=beach::actor_no_index;
    Consumer(scene::Services& s,actor::Owner& o,world::World& w,const beach::ActorResources& r)
        :services(s),owner(o),world(w),resources(r) {
        for (std::size_t i=0;i<r.definitions().size();++i) {
            const auto& d=r.definitions()[i];
            if (d.actor_id==129&&d.factory_identity==0x802cac48) magikarp=i;
            else if (d.wire.guest_address==0x8038837c) common=i;
            else if (d.actor_id!=1003) unsupported=i;
        }
        require(magikarp!=beach::actor_no_index&&common!=beach::actor_no_index&&unsupported!=beach::actor_no_index,
                "actual source definition identities absent");
    }
    world::Spawn descriptor() const {
        world::Spawn spawn;
        spawn.id=129; spawn.behavior=7;
        spawn.position={0,17,0}; spawn.rotation={0.125f,0.25f,-0.375f};
        require(world.current()&&world.current()->resource,"actual current block metadata absent");
        bool grounded=false;
        for (const auto& candidate:world.current()->resource->spawns) {
            world::GroundResult ground{};
            if (world.ground_at(candidate.position.x*100.0f,candidate.position.z*100.0f,ground)) {
                spawn.position.x=candidate.position.x; spawn.position.z=candidate.position.z;
                grounded=true; break;
            }
        }
        require(grounded,"actual first-block source spawns contain no ground-query subject");
        spawn.scale={19,23,29}; // Ground ignores this, preserving init.scale*.1.
        if (!world.resources().paths.empty()) spawn.path=0;
        return spawn;
    }
    actor::SpawnResult spawn() {
        auto descriptor=this->descriptor();
        auto* block=world.current(); require(block,"Magikarp requires actual current World");
        auto result=owner.spawn({0x90,magikarp,block,block,&descriptor});
        accepted(result.status);
        const auto* record=owner.source(result.instance);
        const auto& init=resources.initializations()[resources.definitions()[magikarp].initialization];
        require(record&&record->id==129&&record->initData==&init&&record->animSetup==&init.setup&&record->baseBlock==block&&
            record->behavior==7&&record->tangible&&record->flags==init.flags&&record->stateProc&&record->pathProc==nullptr&&
            record->transitionGraph==nullptr&&record->loopCount==0&&record->pokemonLoopTarget==0&&record->processFlags==0&&
            record->counter==0&&record->lastAnimationFrame==0&&record->unk_18==-1&&record->unk_E4==255&&
            record->poseID==0&&record->specialPoseID==0&&record->jumpVel==0&&record->hSpeed==0&&record->facingYaw==0,
            "Magikarp Ground factory lost configured tuple or source late resets");
        require(static_cast<const beach::ActorAnimationHeader*>(record->animHeader)->wire.guest_address==0x802cda8c,
            "factory substituted state header for genuine initial802cda8c");
        const IsleGameModelPlacement* placement=nullptr;
        native(isle_game_model_placement(services.game,owner.native_model(result.instance),&placement));
        world::GroundResult ground{};
        const bool found=world.ground_at(descriptor.position.x*100.0f,descriptor.position.z*100.0f,ground);
        require(placement->mode==ISLE_GAME_MODEL_PLACEMENT_RTS&&placement->transform.position.y==(found?ground.height:0)&&
            placement->transform.position.x==descriptor.position.x*100.0f&&placement->transform.position.z==descriptor.position.z*100.0f&&
            equal(placement->transform.position,record->pos1)&&equal(record->pos1,record->targetPos)&&
            equal(placement->transform.rotation,descriptor.rotation)&&equal(record->euler,descriptor.rotation)&&
            equal(placement->transform.scale,{init.scale.x*.1f,init.scale.y*.1f,init.scale.z*.1f}),
            "Magikarp Ground placement/ground0/Euler/init scale policy changed");
        const auto bindings=owner.render_bindings(); bool rendered=false;
        for (const auto& binding:bindings) if (binding.model.object==result.object) {
            require(binding.render.link==5&&binding.render.priority==0x80000000u&&binding.render.camera_tag==UINT32_MAX,
                    "Magikarp incorrectly used common GroundDL4 or lost tag/priority"); rendered=true;
        }
        require(rendered&&owner.registered(result.instance),"Magikarp missing real logical/block/render membership");
        return result;
    }
    void rejects() {
        IsleGameCounts before{},after{}; native(isle_game_counts(services.game,&before));
        const auto original_rng=services.rng;
        auto wrong=descriptor(); wrong.id=1003;
        auto result=owner.spawn({0x90,magikarp,world.current(),world.current(),&wrong});
        require(!result&&result.status.error==actor::OwnerError::admission&&!result.object&&!result.instance,
            "wrong species/init tuple admitted");
        result=owner.spawn({0x90,unsupported,world.current(),world.current(),&wrong});
        require(!result&&result.status.error==actor::OwnerError::admission&&!result.object&&!result.instance,
            "unimplemented species/graph falsely admitted");
        native(isle_game_counts(services.game,&after));
        require(counts_equal(before,after)&&original_rng.seed==services.rng.seed&&!services.failed()&&!owner.course().magikarp_active,
            "rejected tuple mutated canonical owner/native/RNG/scene state");
        for (const auto& binding:owner.render_bindings()) require(!binding.model.object,"rejected factory published render borrower");
    }
    void capacity() {
        const auto parent=spawn(); auto* record=owner.source(parent.instance);
        auto descriptor=this->descriptor(); descriptor.id=1003; descriptor.behavior=1;
        const auto sibling=owner.spawn({0x91,common,world.current(),world.current(),&descriptor});
        accepted(sibling.status);
        IsleActorGeoSpawn geo{}; geo.id=1003; geo.path=record->path; geo.behavior=record->behavior;
        geo.scale={1,1,1};
        const IsleActorGeoDefinition definition{0x8038837c};
        IsleGameCounts before{},after{}; native(isle_game_counts(services.game,&before));
        IsleGameObject* child=reinterpret_cast<IsleGameObject*>(uintptr_t(1));
        require(isle_actor_owner_add_one(record,world.current(),world.current(),&geo,&definition,&child)==ISLE_GAME_EXHAUSTED&&
            !child&&!services.failed(),"actual same-owner child capacity failure invented success or poisoned Services");
        native(isle_game_counts(services.game,&after));
        require(counts_equal(before,after)&&owner.registered(parent.instance)&&owner.registered(sibling.instance)&&
            owner.source(parent.instance)==record&&!record->pathProc&&record->processFlags==0,
            "capacity failure partially published/retired existing canonical actors");
        auto full=this->descriptor();
        auto rejected=owner.spawn({0x92,magikarp,world.current(),world.current(),&full});
        require(!rejected&&rejected.status.error==actor::OwnerError::capacity&&!rejected.object&&!rejected.instance,
            "full physical owner fabricated capacity");
        accepted(owner.reset()); accepted(owner.collect_joined());
    }
    void add_at_geo() {
        const auto parent=spawn();
        const IsleGameModelPlacement* placement=nullptr;
        native(isle_game_model_placement(services.game,owner.native_model(parent.instance),&placement));
        auto changed=placement->transform; changed.position.y+=1;
        accepted(owner.set_placement(parent.instance,changed));
        const auto* inherited=owner.source(parent.instance)->path;
        IsleMagikarpGeoProbe probe{}; IsleGameProcess* process=nullptr;
        native(isle_game_add_process(services.game,parent.object,isle_magikarp_geo_probe,&probe,ISLE_GAME_SYNCHRONOUS,9,&process));
        native(isle_game_tick(services.game)); require(!services.failed(),services.error());
        native(isle_game_end_process(services.game,process));
        require(probe.calls==1&&probe.child&&probe.child!=parent.object,"actual C AddAtGeo failed native callback child creation");
        IsleActorSource* child=nullptr; native(isle_actor_source_get(services.game,probe.child,&child));
        require(child->id==1003&&child->behavior==7&&child->path==inherited&&child->baseBlock==world.current(),
            "AddAtGeo did not inherit parent behavior/path before helper's behavior0 override");
        a::InstanceHandle handle{};
        for (const auto& binding:owner.render_bindings()) if (binding.model.object==probe.child) handle=binding.model.state.instance;
        require(bool(handle)&&owner.registered(handle),"AddAtGeo child was not in canonical owner's real registry/Runtime");
        native(isle_game_model_placement(services.game,owner.native_model(handle),&placement));
        require(equal(placement->transform.position,changed.position)&&equal(placement->transform.rotation,{0,0,0}),
            "actual selected AddAtGeo failed parent AUX position copy or zero Euler policy");
        IsleGameObjectMetadata metadata{}; native(isle_game_object_metadata(services.game,probe.child,&metadata));
        require(metadata.id==0x82,"source cursor did not survive reset after prior failed capacity allocation");
        accepted(owner.reset()); accepted(owner.collect_joined());
    }
    void normal() {
        // The real link3 lookup must notice a live object outside this Owner.
        auto* occupied=services.object(0x83,3); require(occupied,"source ID collision subject allocation failed");
        const auto parent=spawn(); const auto handle=parent.instance;
        owner.course().magikarp_active=true; // Actual scene chooser publication.
        YawObservation yaw{&services}; IsleGameProcess* observation=nullptr;
        native(isle_game_add_process(services.game,parent.object,YawObservation::capture,&yaw,ISLE_GAME_SYNCHRONOUS,8,&observation));
        native(isle_game_tick(services.game)); require(!services.failed(),services.error());
        native(isle_game_end_process(services.game,observation));
        auto* record=owner.source(handle); require(record&&record->pathProc,"genuine initial state did not start native path5");
        require(static_cast<const beach::ActorAnimationHeader*>(record->animHeader)->wire.guest_address==0x802cdab4&&
            record->pokemonLoopTarget==1&&record->transitionGraph==nullptr&&owner.course().magikarp_active,
            "actual state header/wait/scene publication missing");
        const IsleGameModelPlacement* placement=nullptr;
        native(isle_game_model_placement(services.game,owner.native_model(handle),&placement));
        require(yaw.calls==1&&placement->transform.rotation.y==yaw.expected&&
            placement->transform.rotation.x==0.125f&&placement->transform.rotation.z==-0.375f&&record->euler.y==0.25f,
            "source randRange360 yaw did not update actual AUX rotation independently of initial Euler");
        const IsleGameModelNode* nodes=nullptr; std::size_t count=0;
        native(isle_game_model_peek_nodes(services.game,owner.native_model(handle),&nodes,&count));
        const auto first=nodes[0].first_child;
        require(first<count&&nodes[first].first_child<count,"actual Magikarp child topology absent");
        const auto second=nodes[nodes[first].first_child].next_sibling;
        require(second<count&&nodes[second].parent==first,"actual Magikarp second child topology absent");
        std::size_t selected=std::numeric_limits<std::size_t>::max();
        native(isle_actor_owner_ground_node(record,&selected)); require(selected==second,"source selection replaced child/child/next with arbitrary root");
        // The actual path starts at zero velocity. Exercise the same real
        // nonwaiting leaf with a nonzero vector so a zero/no-op implementation,
        // row normalization or translation contamination cannot pass unnoticed.
        menu::EffectWorld velocity_world{};
        require(bool(menu::model::source_effect_world(services.game,owner.native_model(handle),selected,velocity_world)),
                "canonical velocity oracle world failed");
        const IsleGameVec3 input_velocity{1.25f,-2.0f,3.5f};
        std::array<float,3> expected_velocity{};
        for (unsigned column=0;column<3;++column) {
            const double x=velocity_world.matrix[0][column],y=velocity_world.matrix[1][column],z=velocity_world.matrix[2][column];
            const double length=std::hypot(x,y,z);
            expected_velocity[column]=length==0 ? 0.0f :
                static_cast<float>((input_velocity.x*x+input_velocity.y*y+input_velocity.z*z)/length);
        }
        IsleGameVec3 sampled_position{},sampled_velocity=input_velocity;
        native(isle_actor_owner_node_world(record,selected,&sampled_position,&sampled_velocity));
        require(equal(sampled_position,{velocity_world.matrix[3][0],velocity_world.matrix[3][1],velocity_world.matrix[3][2]})&&
                equal(sampled_velocity,{expected_velocity[0],expected_velocity[1],expected_velocity[2]}),
                "source nonzero velocity lost normalized canonical columns or acquired translation");
        a::InstanceHandle child_handle{}; IsleGameObject* child_object=nullptr;
        bool path_ended=false,animation_ended=false,joined=false,credit_observed=false;
        unsigned frames=1;
        for (;frames<512;++frames) {
            menu::EffectWorld raw{}; world::GroundResult ground{};
            const bool live=owner.source(handle)!=nullptr;
            if (live) {
                require(bool(menu::model::source_effect_world(services.game,owner.native_model(handle),selected,raw)),"canonical child world failed");
                world.ground_at(raw.matrix[3][0],raw.matrix[3][2],ground);
            }
            // Source state7 starts path5 in tick1, before OM descends to5;
            // wait10 first resumes in tick11. Path5 reads this pre-animation3
            // pose, so every subsequent eligible sample must emit immediately.
            const bool awaiting_child=child_object==nullptr;
            const bool should_emit=awaiting_child&&live&&frames>=10&&raw.matrix[3][1]<ground.height;
            native(isle_game_tick(services.game)); require(!services.failed(),services.error());
            const auto membership=owner.membership(handle);
            path_ended|=(membership.process_flags&POKEMON_PROCESS_FLAG_PATH_ENDED)!=0;
            animation_ended|=(membership.process_flags&POKEMON_PROCESS_FLAG_ANIMATION_ENDED)!=0;
            if (auto* current=owner.source(handle)) {
                require(!path_ended||!current->pathProc,"source path end retained a native process handle");
                if (!animation_ended) require(owner.course().magikarp_active,"scene flag cleared before genuine ANIMATION_ENDED");
            }
            for (const auto& binding:owner.render_bindings()) if (binding.model.object&&binding.model.object!=parent.object) {
                IsleActorSource* child=nullptr; native(isle_actor_source_get(services.game,binding.model.object,&child));
                require(child->id==1003,"same owner emitted wrong source species");
                if (!child_object) {
                    require(live&&frames>=10&&raw.matrix[3][1]<ground.height&&path_ended,
                        "ground child emitted before ten-frame wait, strict ground crossing or path completion");
                    child_object=binding.model.object; child_handle=binding.model.state.instance;
                    require(child_object!=parent.object&&child_handle!=handle&&owner.registered(child_handle)&&child->baseBlock==world.current()&&
                        child->path==record->path&&child->behavior==0&&binding.render.link==4,
                        "AddAtGeo lost same-owner identity/current block/path/behavior0/DL4 chain");
                    IsleGameObjectMetadata metadata{}; native(isle_game_object_metadata(services.game,child_object,&metadata));
                    // Failed capacity consumed cursor1; direct real AddAtGeo
                    // consumed2; normal3 must skip the outside-Owner collision83.
                    require(metadata.id==0x84&&metadata.update_link==3&&metadata.priority==0x80000000u,
                        "source cursor/link-wide ID lookup/registration semantics changed");
                    const IsleGameModelPlacement* child_placement=nullptr;
                    native(isle_game_model_placement(services.game,owner.native_model(child_handle),&child_placement));
                    require(equal(child_placement->transform.position,{raw.matrix[3][0],ground.height,raw.matrix[3][2]})&&
                        equal(child_placement->transform.rotation,{0,0,0}),
                        "helper failed exact raw child-world position/ground clamp/zero AddAtGeo Euler");
                    require(services.source_reference_assign(child_object,&child->interactionTarget,parent.object),
                        "real child did not retain canonical parent target");
                    require(services.send_command(child_object,-2,parent.object),"real child FIFO suspension failed");
                } else require(binding.model.object==child_object,"completed path emitted a second sibling");
            }
            require(!awaiting_child || (child_object!=nullptr)==should_emit,
                    "source ground poll skipped first eligible tick or emitted during initial wait/noncrossing");
            if (membership.retiring) {
                require(membership.joined&&!membership.logical&&!membership.registered&&!owner.course().magikarp_active&&animation_ended,
                    "source wait/scene clear/RunCleanup/native join ordering changed");
                joined=true;
                const auto collected=owner.collect_joined();
                if (!collected) {
                    require(collected.error==actor::OwnerError::lifetime,"retirement failed outside identity/FIFO hold");
                    credit_observed=true;
                }
            } else accepted(owner.collect_joined());
            if (joined&&!owner.membership(handle).retiring) break;
        }
        require(frames<512&&path_ended&&animation_ended&&joined&&child_object&&credit_observed,
            "actual Magikarp FSM did not demonstrate ground polling/emission/animation wait/FIFO-protected retirement");
        require(!owner.source(handle)&&!owner.object(handle)&&!owner.animation().model(handle)&&
            owner.source(child_handle)&&owner.source(child_handle)->interactionTarget==nullptr,
            "parent stale payload survived or real common child lost source22 clearing");
        require(services.send_command(child_object,-3,nullptr),"actual common child resume failed");
        for (unsigned n=0;n<512&&owner.registered(child_handle);++n) {
            native(isle_game_tick(services.game)); require(!services.failed(),services.error()); accepted(owner.collect_joined());
        }
        require(!owner.source(child_handle)&&!owner.animation().model(child_handle),"emitted common1003 did not execute actual normal cleanup");
        for (unsigned n=0;n<4;++n) { native(isle_game_tick(services.game)); require(!services.failed(),services.error()); }
        require(!owner.course().magikarp_active&&!owner.set_animation(handle,nullptr)&&!owner.set_animation(child_handle,nullptr),
            "ended source ran again or admitted stale native mutation");
        for (const auto& binding:owner.render_bindings()) require(!binding.model.object,"retired sibling retained live rendering borrower");
        services.erase(occupied); accepted(owner.reset()); accepted(owner.collect_joined());
    }
    void id_boundaries() {
        // Prior full cases consumed source cursor1(capacity),2(direct AddAtGeo)
        // and3(normal emission). Candidate84 did not rewrite that source cursor.
        const auto parent=spawn();auto* record=owner.source(parent.instance);
        auto descriptor=this->descriptor();descriptor.id=1003;descriptor.behavior=1;
        const auto sibling=owner.spawn({0x91,common,world.current(),world.current(),&descriptor});
        accepted(sibling.status);
        IsleActorGeoSpawn geo{};geo.id=1003;geo.path=record->path;geo.behavior=record->behavior;geo.scale={1,1,1};
        const IsleActorGeoDefinition definition{0x8038837c};
        // Actual nonwaiting source leaf failures consume cursor through100
        // without fabricating a child or invoking a C fault outside OM.
        for (unsigned n=0;n<97;++n) {
            IsleGameObject* output=nullptr;
            require(isle_actor_owner_add_one(record,world.current(),world.current(),&geo,&definition,&output)==ISLE_GAME_EXHAUSTED&&
                    !output&&!services.failed(),"source cursor boundary capacity failure partially succeeded");
        }
        accepted(owner.reset());accepted(owner.collect_joined());
        const auto exercise=[&](std::uint32_t first,std::uint32_t last,std::uint32_t expected,bool other_link) {
            std::array<IsleGameObject*,0xe4-0x80+2> occupied{};
            std::size_t count=0;IsleGameObject* maximum=nullptr;
            for (auto id=first;id<=last;++id) {
                auto* object=services.object(id,3);require(object,"native source-ID occupancy allocation failed");
                occupied[count++]=object;if(id==0xe4)maximum=object;
            }
            if (other_link) {
                auto* object=services.object(0x80,4);require(object,"other-link occupancy allocation failed");
                occupied[count++]=object;
            }
            const auto source=spawn();
            IsleMagikarpGeoProbe probe{};IsleGameProcess* process=nullptr;
            native(isle_game_add_process(services.game,source.object,isle_magikarp_geo_probe,&probe,ISLE_GAME_SYNCHRONOUS,9,&process));
            native(isle_game_tick(services.game));require(!services.failed(),services.error());
            native(isle_game_end_process(services.game,process));
            require(probe.child&&probe.child!=source.object,"boundary case did not execute real C AddAtGeo");
            IsleGameObjectMetadata metadata{};native(isle_game_object_metadata(services.game,probe.child,&metadata));
            require(metadata.id==expected&&metadata.update_link==3,"source wrap/half-open search/max fallback changed");
            if (maximum) {
                IsleGameObjectMetadata existing{};native(isle_game_object_metadata(services.game,maximum,&existing));
                require(maximum!=probe.child&&existing.id==metadata.id&&metadata.id==0xe4,
                        "source inclusive occupied-max fallback was replaced by uniqueness rejection");
            }
            for (std::size_t n=0;n<count;++n)services.erase(occupied[n]);
            accepted(owner.reset());accepted(owner.collect_joined());
        };
        exercise(1,0,0x80,true);        // Increment100->101 wraps0; link4 is ignored.
        exercise(0x81,0xe3,0x80,true);  // Empty first half, real second-half search.
        exercise(0x80,0xe4,0xe4,false); // Max fallback remains valid even occupied.
    }
};
void run(rom::Snapshot& snapshot,const std::filesystem::path& profile) {
    require(std::filesystem::is_directory(profile)&&std::filesystem::is_empty(profile),"private profile directory must exist and be empty");
    const world::Resources world_resources(snapshot);
    const auto& setup=world_resources.setup();
    const beach::ActorResources resources(snapshot,{setup.fog_rgb,setup.fog_min,setup.fog_max});
    const environment_sound::BeachResources sound_resources(snapshot);
    const menu::EffectResources effect_resources(snapshot);
    const std::array banks{&effect_resources.bank(menu::EffectGroup::common),&effect_resources.bank(menu::EffectGroup::beach)};
    auto hydrated=game_audio::hydrate(snapshot); require(bool(hydrated),"actual audio hydration failed");
    std::array<audio::effects::Section,8> sections; audio::effects::Config effects;
    require(game_audio::native_startup_effects(hydrated.resources,sections,effects)==audio::Error::none,"actual audio effects configuration failed");
    private_audio::Capture capture; private_audio::Execution execution{snapshot,nullptr,{}};
    capture.samples.resize(8192*audio::AudioService::max_frames*2);
    auto made=game_audio::Session::create(std::move(hydrated.resources),effects,capture.output(),execution.executor());
    require(bool(made),"actual audio Session creation failed"); auto& session=*made.session;
    std::uint64_t retrace=0;
    auto pump=[&] {
        ++retrace; require(session.retrace(retrace,retrace*1000000000ull/60)==audio::Error::none,"audio retrace failed");
        require(session.submit()==audio::Error::none,"checked PCM submission failed");
        audio::TaskView task; require(session.build(task)==audio::Error::none,"source audio task build failed");
        if (task.token.serial) { private_audio::execute(session,execution,task); require(session.submit()==audio::Error::none,"completed PCM submission failed"); }
    };
    {
        game::State state(profile); require(state.initialize(1),"private profile initialization failed");
        game_input::Service input; scene::Storage storage(32*1024*1024); native(storage.create());
        {
            scene::Services services(storage.game(),storage.arena(),{{4,6,0,1,2},{}},input,session,state);
            require(!services.failed(),services.error());
            private_player::Source context(services);
            auto* camera_object=services.object(1,0); require(camera_object,"camera object allocation failed");
            IsleGameCamera* camera=nullptr; native(isle_game_attach_camera(services.game,camera_object,&camera));
            const IsleGameLookAt view{{0,0,0},{1,0,0},{0,1,0}}; native(isle_game_camera_set_look_at(services.game,camera,&view));
            context.create(camera);
            environment_sound::Environment environment(services,storage.arena(),sound_resources);
            require(environment.initialize(nullptr,camera),"environment source initialization failed");
            focus::Detector detector; Shift shift{&services,camera,nullptr};
            world::World world(services,world_resources,{&shift,nullptr,nullptr,nullptr,Shift::camera_shift,Shift::effects_shift,nullptr});
            a::SlotCapacity capacity{};
            for (const auto& model:resources.models()) {
                const auto c=a::slot_capacity(model); capacity.nodes=std::max(capacity.nodes,c.nodes);
                capacity.material_attachments=std::max(capacity.material_attachments,c.material_attachments);
            }
            const std::array capacities{capacity,capacity};
            {
                actor::Owner owner(services,resources,world,environment,detector,context.value,capacities,banks,{32768,4096}); shift.owner=&owner;
                accepted(owner.publish_effects());
                require(world.enter_first(0)!=nullptr,"real World first-block admission failed");
                Consumer consumer(services,owner,world,resources);
                consumer.rejects(); consumer.capacity(); consumer.add_at_geo(); consumer.normal(); consumer.id_boundaries();
                shift.owner=nullptr;
            }
            world.retire(); environment.cleanup();
        }
        native(storage.destroy());
    }
    require(session.stop()==audio::Error::none,"actual audio stop failed");
    for (unsigned n=0;n<240&&session.state()!=game_audio::Session::State::draining;++n) pump();
    require(session.state()==game_audio::Session::State::draining&&session.service().voices_retired(),"actual audio voice graph failed retirement");
    require(session.poll_drain()==audio::Error::none&&session.poll_drain()==audio::Error::none&&session.state()==game_audio::Session::State::drained,
        "actual PCM owner failed drain");
    std::cout<<"magikarp_owner_private_CPU_behavior=passed GPU_Original_Windows11=not_claimed\n";
}
}
int main(int argc,char** argv) {
    try {
        require(argc==3,"usage: magikarp-owner-rom-smoke PRIVATE_ROM EXISTING_EMPTY_PRIVATE_PROFILE_DIRECTORY");
        auto snapshot=isle::rom::read_snapshot(argv[1]); run(snapshot,argv[2]); return EXIT_SUCCESS;
    } catch (const std::exception& error) { std::cerr<<"magikarp-owner-rom-smoke: "<<error.what()<<'\n'; return EXIT_FAILURE; }
}
