// Explicit private-ROM CPU behavioral consumer. NEVER register with CTest or
// default discovery. No graphics executor is created: exclusion is serialized,
// but GPU/preparation/readback/postprocess lifetime is NOT proved here. Main's
// combined consumer owns that real boundary. No guest CPU or synthetic FSM runs.
#include "actor_owner.hpp"
#include "actual_audio_driver.hpp"
#include "effect_bank_resources.hpp"
#include "scene_storage.hpp"
#include "source_effect_world.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {
using namespace isle;
namespace a = menu::animation;
void require(bool value, const char* text) { if (!value) throw std::runtime_error(text); }
void native(IsleGameResult value) { require(value == ISLE_GAME_OK,"native owner consumer operation failed"); }
void accepted(actor::OwnerStatus value) { require(bool(value),value.detail); }
bool near(float x,float y) { return std::abs(x-y)<0.0001f; }
bool same(menu::Vec3 x,menu::Vec3 y) { return near(x.x,y.x)&&near(x.y,y.y)&&near(x.z,y.z); }
IsleGameModelTransform transform(a::Transform value) {
    return {{value.position.x,value.position.y,value.position.z},
            {value.rotation.x,value.rotation.y,value.rotation.z},
            {value.scale.x,value.scale.y,value.scale.z}};
}
void synchronized(scene::Services& services, IsleGameModel* model, a::ModelState pose) {
    require(bool(pose),"accepted Runtime pose unavailable");
    for (std::size_t n=0;n<pose.nodes.size();++n) {
        const auto value=transform(pose.nodes[n].transform);
        native(isle_game_model_validate_transform(services.game,model,n,&value));
        native(isle_game_model_set_transform(services.game,model,n,&value));
        native(isle_game_model_set_flags(services.game,model,n,pose.nodes[n].flags));
    }
}
void agrees(scene::Services& services, actor::Owner& owner,a::InstanceHandle handle) {
    const IsleGameModelNode* nodes=nullptr; std::size_t count=0;
    native(isle_game_model_peek_nodes(services.game,owner.native_model(handle),&nodes,&count));
    const auto pose=owner.animation().model(handle);
    require(bool(pose)&&pose.nodes.size()==count,"native/Runtime topology diverged");
    for (std::size_t n=0;n<count;++n) {
        const auto& p=pose.nodes[n].transform;
        require(same(p.position,{nodes[n].transform.position.x,nodes[n].transform.position.y,nodes[n].transform.position.z})&&
                same(p.rotation,{nodes[n].transform.rotation.x,nodes[n].transform.rotation.y,nodes[n].transform.rotation.z})&&
                same(p.scale,{nodes[n].transform.scale.x,nodes[n].transform.scale.y,nodes[n].transform.scale.z}),
                "source local write or immediate animation did not synchronize both pose domains");
    }
}
struct WorldShift {
    scene::Services* services=nullptr;
    IsleGameCamera* camera=nullptr;
    actor::Owner* owner=nullptr;
    static void camera_shift(void* context,world::Vec3 delta) noexcept {
        auto& self=*static_cast<WorldShift*>(context);
        const IsleGameCameraParameters* parameters=nullptr;
        if (!self.services->check(isle_game_camera_parameters(self.services->game,self.camera,&parameters),"camera shift borrow")) return;
        auto look=parameters->look_at;
        for (unsigned i=0;i<3;++i) {
            const float d=i==0?delta.x:i==1?delta.y:delta.z;
            look.eye[i]+=d; look.at[i]+=d;
        }
        self.services->check(isle_game_camera_set_look_at(self.services->game,self.camera,&look),"camera shift");
    }
    static void effect_shift(void* context,world::Vec3 delta) noexcept {
        auto& self=*static_cast<WorldShift*>(context);
        if (!self.owner) { self.services->fail("effect shift before owner binding"); return; }
        self.owner->effects().translate({delta.x,delta.y,delta.z});
    }
};
struct Consumer {
    scene::Services& services;
    actor::Owner& owner;
    world::World& world;
    const beach::ActorResources& resources;
    focus::Detector& detector;
    std::size_t common_definition=beach::actor_no_index;
    std::uint32_t next_id=10000;
    Consumer(scene::Services& s,actor::Owner& o,world::World& w,const beach::ActorResources& r,focus::Detector& d)
        : services(s),owner(o),world(w),resources(r),detector(d) {
        const auto defs=r.definitions();
        for (std::size_t i=0;i<defs.size();++i) if (defs[i].actor_id==1003) common_definition=i;
        require(common_definition!=beach::actor_no_index,"admitted common1003 definition absent");
    }
    actor::SpawnResult spawn(int behavior) {
        world::Spawn descriptor;
        descriptor.id=1003; descriptor.behavior=behavior;
        descriptor.position={12,17,-23}; // Authored representable API input, not a ROM spawn.
        descriptor.scale={19,23,29}; // Source factory must ignore spawn.scale.
        world::GroundResult ground{};
        require(world.current()==nullptr&&!world.ground_at(descriptor.position.x*100.0f,descriptor.position.z*100.0f,ground),
                "positive factory fixture must exercise explicit absent-current-World ground fallback");
        auto result=owner.spawn({next_id++,common_definition,&world.blocks()[0],&world.blocks()[0],&descriptor});
        accepted(result.status); require(result.object&&owner.registered(result.instance),"factory did not register actual source object");
        auto* record=owner.source(result.instance);
        require(record&&record->behavior==behavior&&record->pos1.y==0&&record->targetPos.y==0,
                "source ground failure must use0, independently of World fallback-9000");
        require(record->loopCount==0&&record->pokemonLoopTarget==0&&record->lastAnimationFrame==0&&record->processFlags==0,
                "factory late resets must remain distinct from the header setter's target1 reset");
        require(record->playerDist==std::numeric_limits<float>::max(),"genuinely absent player did not retain FLOATMAX");
        const IsleGameModelPlacement* placement=nullptr;
        native(isle_game_model_placement(services.game,owner.native_model(result.instance),&placement));
        const auto& init=resources.initializations()[resources.definitions()[common_definition].initialization];
        require(placement->mode==ISLE_GAME_MODEL_PLACEMENT_RTS&&
                placement->transform.position.x==descriptor.position.x*100.0f&&placement->transform.position.y==0&&
                placement->transform.position.z==descriptor.position.z*100.0f&&
                placement->transform.scale.x==init.scale.x*0.1f&&placement->transform.scale.y==init.scale.y*0.1f&&
                placement->transform.scale.z==init.scale.z*0.1f,
                "source ground factory changed grouping/placement/init scale or consumed spawn.scale");
        agrees(services,owner,result.instance);
        return result;
    }
    void tick() {
        // No traversal or graphics borrower survives this call. Collection is
        // intentionally after the native tick, never from a native callback.
        native(isle_game_tick(services.game));
        require(!services.failed(),services.error());
        accepted(owner.collect_joined());
    }
    void invalid_placement() {
        const auto same_counts=[](IsleGameCounts x,IsleGameCounts y) {
            return x.objects==y.objects&&x.processes==y.processes&&x.sprites==y.sprites&&x.cameras==y.cameras&&
                x.matrices==y.matrices&&x.models==y.models&&x.model_nodes==y.model_nodes;
        };
        for(unsigned trial=0;trial<2;++trial) {
            IsleGameCounts before{},after{}; native(isle_game_counts(services.game,&before));
            world::Spawn oversized; oversized.id=1003; oversized.behavior=1;
            oversized.position={12345,17,-23456}; // Preserve the exact former invalid positive input.
            oversized.scale={19,23,29};
            const auto rejected=owner.spawn({next_id++,common_definition,&world.blocks()[0],&world.blocks()[0],&oversized});
            require(!rejected&&rejected.status.error==actor::OwnerError::native&&!rejected.object&&!rejected.instance&&
                    !services.failed(),"out-of-fixed-domain factory input published a partial actor or poisoned Services");
            native(isle_game_counts(services.game,&after));
            require(same_counts(before,after),"failed factory leaked live native object/process/model/matrix ownership");
            accepted(owner.collect_joined());
            for(const auto& binding:owner.render_bindings())
                require(!binding.model.object&&!binding.model.model&&!binding.model.state,
                        "joined failed factory remained visible through the actual renderer bindings");
            const auto first=spawn(1),second=spawn(1);
            require(first.instance!=second.instance&&first.object!=second.object&&owner.registered(first.instance)&&owner.registered(second.instance),
                    "failed placement leaked physical Runtime/native identity capacity");
            const IsleGameModelPlacement* placement=nullptr;
            auto* model=owner.native_model(first.instance);
            native(isle_game_model_placement(services.game,model,&placement));
            const auto accepted_placement=*placement;
            auto invalid=placement->transform;
            invalid.position.x=oversized.position.x*100.0f;
            invalid.position.z=oversized.position.z*100.0f;
            const auto exact=[](IsleGameVec3 a,IsleGameVec3 b) { return a.x==b.x&&a.y==b.y&&a.z==b.z; };
            require(isle_game_model_set_placement(services.game,model,ISLE_GAME_MODEL_PLACEMENT_RTS,&invalid)==ISLE_GAME_INVALID_ARGUMENT&&
                    placement->revision==accepted_placement.revision&&placement->mode==accepted_placement.mode&&
                    exact(placement->transform.position,accepted_placement.transform.position)&&
                    exact(placement->transform.rotation,accepted_placement.transform.rotation)&&
                    exact(placement->transform.scale,accepted_placement.transform.scale)&&
                    std::memcmp(&placement->fixed,&accepted_placement.fixed,sizeof(placement->fixed))==0,
                    "rejected oversized placement changed accepted placement/fixed/revision");
            agrees(services,owner,first.instance);
            accepted(owner.reset()); accepted(owner.collect_joined());
            native(isle_game_counts(services.game,&after));
            require(same_counts(before,after),"failed/recovered factory cycle leaked joined native storage");
        }
    }
    void normal() {
        const auto actor=spawn(0); const auto handle=actor.instance;
        float previous=owner.animation().model(handle).nodes[0].transform.position.y;
        unsigned writes=0; bool path_ended=false,animation_ended=false;
        for (unsigned frame=0;frame<512&&owner.registered(handle);++frame) {
            native(isle_game_tick(services.game));
            require(!services.failed(),services.error());
            if (auto* record=owner.source(handle)) {
                const auto pose=owner.animation().model(handle);
                const auto y=pose.nodes[0].transform.position.y;
                if (y!=previous) { require(y==previous-0.2f,"actual path did not subtract .2 before its one-frame wait"); ++writes; previous=y; }
                path_ended|=(record->processFlags&POKEMON_PROCESS_FLAG_PATH_ENDED)!=0;
                animation_ended|=(record->processFlags&POKEMON_PROCESS_FLAG_ANIMATION_ENDED)!=0;
                require(record->pos1.y==0,"root-local descent changed grounded pos1");
                agrees(services,owner,handle);
                if (path_ended) require(record->pathProc==nullptr&&writes==59,"path termination preceded59 actual writes or retained source handle");
            }
            const auto membership=owner.membership(handle);
            animation_ended|=(membership.process_flags&POKEMON_PROCESS_FLAG_ANIMATION_ENDED)!=0;
            if (membership.retiring) require(membership.joined,"normal cleanup retained native source workers");
            accepted(owner.collect_joined());
        }
        require(writes==59&&path_ended&&animation_ended&&!owner.registered(handle),
                "normal actual C FSM failed path/wait/animation-end/cleanup");
        require(!owner.object(handle)&&!owner.source(handle)&&!owner.animation().model(handle),"joined actor retained live borrow");
        for (unsigned i=0;i<4;++i) tick();
        require(!owner.source(handle)&&!owner.set_animation(handle,nullptr),"retired source ran again or stale header mutation succeeded");
    }
    void pause_and_update() {
        const auto actor=spawn(1); const auto handle=actor.instance;
        for (unsigned n=0;n<4&&owner.source(handle)->tangible;++n) tick();
        require(!owner.source(handle)->tangible,"pause subject did not reach actual hidden state");
        setter(handle);
        auto* record=owner.source(handle);
        record->counter=2; record->processFlags&=~std::uint32_t(POKEMON_PROCESS_WAIT_ENDED);
        owner.course().paused=true;
        const auto running=owner.animation().model(handle).animation_time;
        require(services.send_command(actor.object,58,actor.object),"ordinary paused-course command enqueue failed");
        for (unsigned n=0;n<3;++n) tick();
        require(record->counter==2&&!(record->processFlags&POKEMON_PROCESS_WAIT_ENDED)&&
                owner.animation().model(handle).animation_time>running,
                "getIsPaused must gate only Update's counter, not animation or the command tail");
        owner.course().paused=false;
        owner.course().execution_enabled=false;
        auto time=owner.animation().model(handle).animation_time;
        tick();
        require(record->counter==1&&owner.animation().model(handle).animation_time>time,
                "execution predicate suspended processes without an ordinary source command");
        time=owner.animation().model(handle).animation_time;
        require(services.send_command(actor.object,58,actor.object),"execution-disabled ordinary command enqueue failed");
        tick();
        require(record->counter==0&&(record->processFlags&POKEMON_PROCESS_WAIT_ENDED)&&
                owner.animation().model(handle).animation_time==time,
                "ordinary command tail did not suspend actual processes after Update bookkeeping");
        owner.course().execution_enabled=true;
        for (unsigned n=0;n<2;++n) tick();
        require(owner.animation().model(handle).animation_time==time,
                "clearing the execution predicate implicitly resumed source processes");
        require(services.send_command(actor.object,-3,actor.object),"source resume command enqueue failed");
        tick();
        require(owner.animation().model(handle).animation_time>time,"source FIFO -3 did not resume animation");
        require(services.send_command(actor.object,-2,actor.object),"source suspend command enqueue failed");
        tick(); time=owner.animation().model(handle).animation_time;
        owner.course().paused=true; tick(); owner.course().paused=false; tick();
        require(owner.animation().model(handle).animation_time==time,
                "course predicate transitions overrode a real source FIFO -2 suspension");
        require(services.send_command(actor.object,-3,actor.object),"second source resume command enqueue failed");
        tick();
        require(owner.animation().model(handle).animation_time>time,"source FIFO -3 failed after predicate transitions");
        // Freeze actual source processes, not the object's real OM update.
        // Selected C entrypoints require that live native callback context.
        require(services.send_command(actor.object,-2,actor.object),"Update boundary process suspension enqueue failed");
        tick();

        // Observe the actual selected Pokemon_Update through its real receiver
        // FIFO. No animation/FSM replacement or fabricated program is admitted.
        struct Observation {
            IsleActorSource* record;
            std::int32_t counter=0,loops=0,target=0;
            std::uint32_t flags=0;
            float last=0;
            bool delivered=false;
            static void command(IsleGameObject*,std::int32_t command,IsleGameObject*,void* user) noexcept {
                auto& self=*static_cast<Observation*>(user);
                if (command!=58) return;
                self.counter=self.record->counter; self.loops=self.record->loopCount;
                self.target=self.record->pokemonLoopTarget; self.flags=self.record->processFlags;
                self.last=self.record->lastAnimationFrame; self.delivered=true;
            }
        } observation{record};
        require(services.set_command_handler(actor.object,Observation::command,&observation),"Update observer registration failed");
        const auto animation_time=owner.animation().model(handle).animation_time;
        require(animation_time>0,"Update strict-lastFrame subject lacks a real positive source clock");
        const auto observe=[&](bool paused,std::int32_t counter,std::uint32_t flags,
                               std::int32_t loops,std::int32_t target,float last,
                               std::int32_t expected_counter,std::uint32_t expected_flags) {
            owner.course().paused=paused;
            record->counter=counter; record->processFlags=flags; record->loopCount=loops;
            record->pokemonLoopTarget=target; record->lastAnimationFrame=last;
            observation.delivered=false;
            require(services.send_command(actor.object,58,actor.object),"Update boundary observer enqueue failed");
            tick();
            require(!services.failed()&&observation.delivered&&observation.counter==expected_counter&&
                    observation.flags==expected_flags&&observation.loops==loops&&observation.target==target&&
                    observation.last==last&&record->counter==expected_counter&&record->processFlags==expected_flags&&
                    owner.animation().model(handle).animation_time==animation_time,
                    "queued observer did not see exact prior Update bookkeeping before command dispatch");
        };
        constexpr auto path=std::uint32_t(POKEMON_PROCESS_FLAG_PATH_ENDED);
        constexpr auto wait=std::uint32_t(POKEMON_PROCESS_WAIT_ENDED);
        constexpr auto ended=std::uint32_t(POKEMON_PROCESS_FLAG_ANIMATION_ENDED);
        observe(true,1,path,0,1,0,1,path);
        observe(false,1,path,0,1,0,0,path|wait);
        observe(false,1,path|wait,0,1,0,1,path|wait);
        observe(false,0,path,0,1,0,0,path);
        observe(true,1,path,2,2,0,1,path|ended);
        observe(false,1,path,0,0,0,0,path|wait|ended);
        observe(false,1,path,0,1,animation_time,0,path|wait);
        observe(false,0,path,0,1,animation_time/2,0,path|ended);
        observe(false,0,path,0,1,-1,0,path);
        owner.course().paused=false;
        accepted(owner.cleanup(handle)); accepted(owner.collect_joined());
    }
    void setter(a::InstanceHandle handle) {
        auto* record=owner.source(handle);
        const auto& init=resources.initializations()[resources.definitions()[common_definition].initialization];
        const auto& header=resources.headers()[init.setup.initial_header];
        accepted(owner.set_animation(handle,&header,7,true));
        require(record->animHeader==&header&&record->modelAnims==header.model_animation_table.guest_address&&
                record->matAnims==header.material_animation_table.guest_address,"setter lost source ARRAY/current-header identities");
        const float time=owner.animation().model(handle).animation_time;
        accepted(owner.set_animation(handle,&header,31,false));
        require(owner.animation().model(handle).animation_time!=31&&owner.animation().model(handle).animation_time>=time,
                "same ARRAY/nonforce setter rebound startTime or omitted immediate step");
        accepted(owner.set_animation(handle,&header,7,true));
        require(owner.animation().model(handle).animation_time==time,"force/startTime did not restart exact source evaluation");
        record->loopCount=9; record->pokemonLoopTarget=8; record->lastAnimationFrame=13;
        record->processFlags|=POKEMON_PROCESS_FLAG_ANIMATION_ENDED|POKEMON_PROCESS_FLAG_PATH_ENDED;
        accepted(owner.set_animation(handle,nullptr));
        require(!record->animHeader&&!record->modelAnims&&!record->matAnims&&record->loopCount==0&&
                record->pokemonLoopTarget==1&&record->lastAnimationFrame==0&&
                !(record->processFlags&POKEMON_PROCESS_FLAG_ANIMATION_ENDED)&&
                (record->processFlags&POKEMON_PROCESS_FLAG_PATH_ENDED),"NULL header trailing resets changed unrelated flags");
        const auto stopped=owner.animation().model(handle).animation_time;
        for (unsigned i=0;i<3;++i) tick();
        require(owner.animation().model(handle).animation_time==stopped,"NULL header did not pause actual animation function");
        accepted(owner.set_animation(handle,&header,0,true)); agrees(services,owner,handle);
    }
    void hidden() {
        const auto actor=spawn(1); const auto handle=actor.instance;
        for (unsigned n=0;n<4&&owner.source(handle)->tangible;++n) tick();
        require(!owner.source(handle)->tangible&&owner.source(handle)->miscVars[0].field1==0,"behavior1 did not enter actual hide/doWait");
        const float held=owner.animation().model(handle).nodes[0].transform.position.y;
        for (unsigned n=0;n<3;++n) tick();
        require(owner.animation().model(handle).nodes[0].transform.position.y==held&&!owner.source(handle)->pathProc,"hidden doWait started path without trigger");
        // Neither this setup nor the interval between cycles calls a setter.
        // The actual source ForceAnimation must restore the baseline itself.
        const auto baseline=held;
        for (unsigned cycle=0;cycle<2;++cycle) {
            unsigned writes=0; bool path_ended=false,final_wait_observed=false;
            unsigned final_write_frame=0;
            auto previous=baseline;
            native(isle_game_object_set_flags(services.game,actor.object,0x82));
            const menu::Vec3 position{100+float(cycle)*31,200+float(cycle),300};
            accepted(owner.trigger_common1003(handle,position));
            require(owner.source(handle)->miscVars[0].field1!=0,"real trigger did not signal after placement");
            tick();
            IsleGameObjectMetadata metadata{}; native(isle_game_object_metadata(services.game,actor.object,&metadata));
            require(owner.source(handle)->tangible&&metadata.flags==0,"SHOW must clear ALL native flags, not just hidden");
            const auto first_y=owner.animation().model(handle).nodes[0].transform.position.y;
            require(first_y==baseline-0.2f&&owner.source(handle)->pathProc&&
                    !(owner.source(handle)->processFlags&POKEMON_PROCESS_FLAG_PATH_ENDED),
                    "source ForceAnimation failed baseline restore or priority5 path waited before its first decrement");
            ++writes; previous=first_y;
            const IsleGameModelPlacement* placement=nullptr;
            native(isle_game_model_placement(services.game,owner.native_model(handle),&placement));
            require(same({placement->transform.position.x,placement->transform.position.y,placement->transform.position.z},position)&&
                    owner.source(handle)->pos1.y==0,"real trigger mixed placement with grounded pos1/local path");
            for (unsigned frame=0;frame<512&&owner.source(handle)->tangible;++frame) {
                tick();
                auto* record=owner.source(handle); require(record&&owner.registered(handle),"behavior1 ran normal cleanup");
                const auto y=owner.animation().model(handle).nodes[0].transform.position.y;
                if (y!=previous) {
                    require(!path_ended&&y==previous-0.2f,"retrigger path changed nonlocal pose or wait order");
                    ++writes; previous=y;
                    if (writes==59) {
                        require(record->pathProc&&!(record->processFlags&POKEMON_PROCESS_FLAG_PATH_ENDED),
                                "59th decrement skipped its final source wait");
                        final_write_frame=frame; final_wait_observed=true;
                    }
                }
                if (!path_ended&&(record->processFlags&POKEMON_PROCESS_FLAG_PATH_ENDED)) {
                    require(final_wait_observed&&frame>final_write_frame&&writes==59&&!record->pathProc,
                            "PATH_ENDED/pathProc clear preceded final wait or59 decrements");
                    path_ended=true;
                }
                agrees(services,owner,handle);
            }
            require(path_ended&&final_wait_observed&&writes==59&&near(baseline-previous,float(writes)*0.2f)&&
                    !owner.source(handle)->tangible&&owner.source(handle)->miscVars[0].field1==0,
                    "behavior1 failed path/end-wait/self-replacement/second hide");
        }
        // Non-native graph admission fails unchanged, before invoking any graph.
        auto* record=owner.source(handle); const auto old=record->transitionGraph;
        record->transitionGraph=&resources;
        require(isle_actor_owner_interactions(record)!=ISLE_GAME_OK&&record->transitionGraph==&resources&&!services.failed(),
                "non-native transition graph executed or rejection mutated source state");
        record->transitionGraph=old;
        accepted(owner.set_focus(handle,true,7));
        const focus::Subject subject{reinterpret_cast<focus::Object>(actor.object),1003,0};
        require(detector.stage(0,std::span(&subject,1),{})==focus::Error::none,"real Detector metadata staging failed");
        accepted(owner.cleanup(handle)); accepted(owner.collect_joined());
        require(!owner.player().previous_focus&&!owner.player().previous_focus_identity&&!owner.player().focus_latch&&
                std::none_of(detector.history(0).begin(),detector.history(0).end(),[&](auto x){return x.object==subject.object;}),
                "actual cleanup retained player focus/latch or Detector identity");
        tick(); require(!owner.source(handle),"self-replaced hidden state ran after native join");
    }
    void capacity_and_reuse() {
        const auto definitions=resources.definitions();
        bool unsupported=false;
        for (std::size_t i=0;i<definitions.size();++i) {
            if (i==common_definition||definitions[i].initialization==beach::actor_no_index) continue;
            const auto& setup=resources.initializations()[definitions[i].initialization].setup;
            if (!setup.initial_state_identity&&!setup.transition_graph_identity) continue;
            world::Spawn descriptor; descriptor.id=static_cast<std::int32_t>(definitions[i].actor_id);
            const auto rejected=owner.spawn({next_id++,i,&world.blocks()[0],&world.blocks()[0],&descriptor});
            require(!rejected&&rejected.status.error==actor::OwnerError::admission,
                    "non-native source state/graph was admitted as a dummy behavior");
            unsupported=true; break;
        }
        require(unsupported,"actual resource catalog lacked non-native admission subject");
        const auto first=spawn(1);
        owner.source(first.instance)->apple=first.object;
        require(services.send_command(first.object,21,first.object),"live sender identity command enqueue failed");
        const auto second=spawn(1);
        require(services.process_commands(first.object)&&!owner.source(first.instance)->apple,
                "queued live sender blockaded available factory capacity or failed actual delivery");
        world::Spawn descriptor; descriptor.id=1003;
        const auto rejected=owner.spawn({next_id++,common_definition,&world.blocks()[0],&world.blocks()[0],&descriptor});
        require(!rejected&&rejected.status.error==actor::OwnerError::capacity&&owner.registered(first.instance)&&owner.registered(second.instance),
                "capacity failure silently partially spawned or damaged live actors");
        auto* recipient=owner.source(second.instance); recipient->interactionTarget=first.object;
        require(services.send_command(second.object,22,first.object),"actual FIFO command22 enqueue failed");
        accepted(owner.cleanup(first.instance));
        const auto unrelated_id=next_id++;
        auto* unrelated=services.object(unrelated_id);
        require(unrelated&&unrelated!=first.object,"unrelated native Services allocator reused a held retired sender identity");
        recipient->interactionTarget=unrelated;
        require(services.process_commands(second.object)&&recipient->interactionTarget==unrelated,
                "retiring command22 sender aliased/cleared unrelated native C");
        IsleGameObjectMetadata unrelated_metadata{};
        native(isle_game_object_metadata(services.game,unrelated,&unrelated_metadata));
        require(unrelated_metadata.id==unrelated_id,"retiring sender delivery damaged unrelated native C identity");
        recipient->interactionTarget=first.object;
        require(services.send_command(second.object,22,first.object)&&services.process_commands(second.object)&&
                !recipient->interactionTarget,"actual command22 delivery lost a held joined-dead sender identity");
        services.erase(unrelated);
        accepted(owner.collect_joined());
        const auto replacement=spawn(1);
        require(replacement.instance!=first.instance&&!owner.set_local(first.instance,0,{})&&!owner.trigger_common1003(first.instance,{}),
                "reused slot admitted stale full instance handle");
        accepted(owner.reset()); accepted(owner.collect_joined());
        require(!owner.source(second.instance)&&!owner.source(replacement.instance)&&owner.effects().effect_count()==0,
                "reset retained actor/source/emitter borrows");
        const auto reused=spawn(1); tick(); require(!owner.source(reused.instance)->tangible,"reset/reuse failed actual native initial state");
        accepted(owner.reset()); accepted(owner.collect_joined());
        const auto sender=spawn(1),recipient_actor=spawn(1);
        owner.source(recipient_actor.instance)->interactionTarget=sender.object;
        require(services.send_command(recipient_actor.object,22,sender.object)&&
                services.send_command(recipient_actor.object,22,sender.object),
                "duplicate real command22 enqueue failed");
        accepted(owner.cleanup(sender.instance));
        const auto blocked=owner.collect_joined();
        require(!blocked&&blocked.error==actor::OwnerError::lifetime&&owner.membership(sender.instance).joined,
                "queued retiring sender identity was collected before duplicate FIFO delivery/cancellation");
        const auto blocked_spawn=owner.spawn({next_id++,common_definition,&world.blocks()[0],&world.blocks()[0],&descriptor});
        require(!blocked_spawn&&blocked_spawn.status.error==actor::OwnerError::capacity,
                "held retiring slot must consume only its real physical capacity, not impose a global sender blockade");
        // Retire B without process_commands/native update: its queue is canceled,
        // not fake-delivered, and every duplicate sender credit must be released.
        accepted(owner.cleanup(recipient_actor.instance));
        require(owner.membership(sender.instance).joined&&owner.membership(recipient_actor.instance).joined,
                "recipient cancellation preceded actual native process/model joins");
        accepted(owner.collect_joined());
        require(!owner.membership(sender.instance).retiring&&!owner.membership(recipient_actor.instance).retiring,
                "canceled duplicate command22 retained sender/recipient physical slots");
        const auto after_cancel=spawn(1);
        require(after_cancel.instance!=sender.instance&&after_cancel.instance!=recipient_actor.instance,
                "queue cancellation reset full instance generations");
        accepted(owner.reset()); accepted(owner.collect_joined());
    }
};

void attached_lifetime(Consumer& consumer,std::span<const menu::EffectBank* const> banks) {
    auto& owner=consumer.owner; auto& services=consumer.services;
    const auto actor=consumer.spawn(1); consumer.tick();
    const auto handle=actor.instance;
    accepted(owner.set_animation(handle,nullptr));
    auto local=owner.animation().model(handle).nodes[0].transform;
    local.position={7,11,13}; accepted(owner.set_local(handle,0,local));
    const auto before_pos=owner.source(handle)->pos1,before_target=owner.source(handle)->targetPos;
    accepted(owner.change_block(handle,{1,2,3},{4,6,8}));
    require(owner.source(handle)->pos1.x==before_pos.x-300&&owner.source(handle)->pos1.y==before_pos.y-400&&
            owner.source(handle)->pos1.z==before_pos.z-500&&owner.source(handle)->targetPos.x==before_target.x-300&&
            owner.source(handle)->targetPos.y==before_target.y-400&&owner.source(handle)->targetPos.z==before_target.z-500&&
            same(owner.animation().model(handle).nodes[0].transform.position,local.position),
            "common1003 block change used ground variant or mixed placement and root-local descent");
    accepted(owner.change_block(handle,{4,6,8},{1,2,3}));
    const IsleGameModelTransform base{{31,47,59},{},{1,1,1}};
    const menu::Vec3 shift{101,203,307};
    bool world_observed=false;
    for (const auto* bank:banks) for (std::size_t script=0;script<bank->scripts.size();++script) {
        const auto seed=services.rng;
        const auto encoded=static_cast<std::uint16_t>(bank->bank_id);
        if (!world_observed) {
            std::array<menu::EffectParticleView,2> observations{};
            std::array<bool,2> emitted{};
            for (unsigned pass=0;pass<2;++pass) {
                owner.effects().reset(); services.rng=seed;
                auto placed=base;
                if (pass) { placed.position.x+=shift.x; placed.position.y+=shift.y; placed.position.z+=shift.z; }
                accepted(owner.set_placement(handle,placed));
                const auto made=owner.effects().create_effect(encoded,script,{handle,0});
                require(bool(made.status)&&made.spawned,"actual attached emitter creation failed");
                for (unsigned frame=0;frame<512&&!emitted[pass];++frame) {
                    accepted(owner.update_effects());
                    emitted[pass]=owner.effects().particle(0,observations[pass]);
                    if (!emitted[pass]) accepted(owner.update_particles());
                }
            }
            if (emitted[0]&&emitted[1]) {
                const auto p=observations[0].position,q=observations[1].position;
                world_observed=same({q.x-p.x,q.y-p.y,q.z-p.z},shift);
            }
        }
    }
    require(world_observed,"no actual attached emitter demonstrated raw native world placement");
    owner.effects().reset();
    bool survivor=false;
    for (const auto* bank:banks) {
        for (std::size_t script=0;script<bank->scripts.size()&&!survivor;++script) {
            const auto made=owner.effects().create_particle(static_cast<std::uint16_t>(bank->bank_id),script,{17,23,29},{3,5,7});
            require(bool(made.status),"actual independent particle admission failed");
            survivor=owner.effects().particle_count()!=0;
        }
        if (survivor) break;
    }
    require(survivor,"no actual independent particle survived immediate evaluation for cleanup");
    const auto attached=owner.effects().create_effect(static_cast<std::uint16_t>(banks.front()->bank_id),0,{handle,0});
    require(bool(attached.status)&&attached.spawned,"actual cleanup attached emitter subject failed");
    const auto particles=owner.effects().particle_count();
    const auto emitters=owner.effects().effect_count();
    accepted(owner.cleanup(handle)); accepted(owner.collect_joined());
    require(owner.effects().effect_count()+1==emitters&&owner.effects().particle_count()==particles,
            "actor detach retained attached emitter or killed ordinary independent particles");
    accepted(owner.update_particles()); accepted(owner.update_effects());
    require(!owner.source(handle),"FX target update revived detached actor");
    owner.effects().reset();
}

// Full-handle native pose context for the standalone actual-header scene. Its
// sole FX owner never resolves through an unrelated Owner/Runtime slot domain.
struct EventPose {
    scene::Services& services;
    a::Runtime& runtime;
    a::InstanceHandle handle;
    IsleGameModel* model=nullptr;
    bool live(menu::EffectAttachment attachment) const noexcept {
        return attachment.instance==handle&&bool(runtime.model(handle))&&model;
    }
    bool publish(std::size_t node,const a::Transform& pose) noexcept {
        const auto value=transform(pose);
        return isle_game_model_validate_transform(services.game,model,node,&value)==ISLE_GAME_OK&&
               bool(runtime.set_node_transform(handle,node,pose))&&
               isle_game_model_set_transform(services.game,model,node,&value)==ISLE_GAME_OK;
    }
    static bool world(void* user,menu::EffectAttachment attachment,menu::EffectWorld& output) noexcept {
        auto& self=*static_cast<EventPose*>(user);
        return self.live(attachment)&&bool(menu::model::source_effect_world(self.services.game,self.model,attachment.node,output));
    }
    static bool read(void* user,menu::EffectAttachment attachment,menu::Vec3& output) noexcept {
        auto& self=*static_cast<EventPose*>(user);
        const IsleGameModelNode* nodes=nullptr; std::size_t count=0;
        if (!self.live(attachment)||isle_game_model_peek_nodes(self.services.game,self.model,&nodes,&count)!=ISLE_GAME_OK||
            attachment.node>=count) return false;
        const auto p=nodes[attachment.node].transform.position; output={p.x,p.y,p.z}; return true;
    }
    static bool write(void* user,menu::EffectAttachment attachment,menu::Vec3 position) noexcept {
        auto& self=*static_cast<EventPose*>(user);
        const auto state=self.runtime.model(self.handle);
        if (!self.live(attachment)||!state||attachment.node>=state.nodes.size()) return false;
        auto pose=state.nodes[attachment.node].transform; pose.position=position;
        return self.publish(attachment.node,pose);
    }
};
bool child_event_emission(IsleActorSource& record,const a::CallbackEvent& event,
                          actor::EventServices context,EventPose& pose) {
    auto& effects=context.effects;
    const auto state=pose.runtime.model(pose.handle);
    std::vector<a::Transform> saved;
    for (const auto& node:state.nodes) saved.push_back(node.transform);
    const IsleGameModelPlacement* placement=nullptr;
    native(isle_game_model_placement(pose.services.game,pose.model,&placement));
    const auto saved_placement=*placement;
    const auto seed=pose.services.rng;
    const auto tangible=record.tangible;
    const menu::Vec3 placement_shift{101,203,307},child_shift{37,53,71};
    std::array<menu::EffectParticleView,3> particles{};
    bool emitted=true;
    for (unsigned pass=0;pass<3;++pass) {
        effects.reset(); pose.services.rng=seed; record.tangible=1;
        // Pure translations give an independent expected displacement. These
        // remain accepted transforms of the real callback's native child tree.
        for (std::size_t n=0;n<saved.size();++n) {
            auto value=saved[n]; value.rotation={}; value.scale={1,1,1};
            if (pass==2&&n==event.node) {
                value.position.x+=child_shift.x; value.position.y+=child_shift.y; value.position.z+=child_shift.z;
            }
            require(pose.publish(n,value),"actual child emission local pose publication failed");
        }
        IsleGameModelTransform placed{{31,47,59},{},{1,1,1}};
        if (pass==1) {
            placed.position.x+=placement_shift.x; placed.position.y+=placement_shift.y; placed.position.z+=placement_shift.z;
        }
        native(isle_game_model_set_placement(pose.services.game,pose.model,ISLE_GAME_MODEL_PLACEMENT_RTS,&placed));
        require(bool(effects.register_aim_target(0,{pose.handle,event.node})),"standalone canonical aim target failed");
        for (std::size_t n=0;n<8;++n)
            require(bool(effects.register_output_target(n,{pose.handle,event.node})),"standalone canonical output target failed");
        // Replay only the event actually emitted by the admitted animation,
        // never manufacture an event/program or allocate its emitter directly.
        accepted(actor::dispatch_event(record,event,context));
        bool found=false;
        for (unsigned frame=0;frame<512&&effects.effect_count()&&!found;++frame) {
            require(bool(effects.update_effects(EventPose::world,&pose)),"actual callback-created emitter update failed");
            found=effects.particle(0,particles[pass]);
            if (!found) require(bool(effects.update_particles()),"actual child particle update failed");
        }
        emitted&=found;
    }
    const auto displaced=[](menu::Vec3 p,menu::Vec3 q,menu::Vec3 expected) {
        return same({q.x-p.x,q.y-p.y,q.z-p.z},expected);
    };
    emitted=emitted&&displaced(particles[0].position,particles[1].position,placement_shift)&&
                     displaced(particles[0].position,particles[2].position,child_shift);
    if (emitted) {
        effects.reset(); record.tangible=0;
        accepted(actor::dispatch_event(record,event,context));
        require(bool(effects.update_effects(EventPose::world,&pose))&&bool(effects.update_particles())&&
                !effects.effect_count()&&!effects.particle_count(),"intangible actual child event emitted FX");
        record.tangible=1;
        for (;;) {
            const auto fill=effects.create_effect(static_cast<std::uint16_t>(event.event_id),
                static_cast<std::size_t>(static_cast<std::int32_t>(event.argument)-1),{pose.handle,event.node});
            require(bool(fill.status),"actual child full-pool setup failed");
            if (!fill.spawned) break;
        }
        const auto full=effects.effect_count();
        require(full>0,"actual emitted child event could not fill its real source effect pool");
        accepted(actor::dispatch_event(record,event,context));
        require(effects.effect_count()==full&&!effects.particle_count(),
                "full-pool actual child callback did not return successful no-spawn");
    }
    effects.reset(); pose.services.rng=seed; record.tangible=tangible;
    for (std::size_t n=0;n<saved.size();++n)
        require(pose.publish(n,saved[n]),"actual child emission pose restoration failed");
    native(isle_game_model_set_placement(pose.services.game,pose.model,saved_placement.mode,&saved_placement.transform));
    return emitted;
}

// Additional headers have real root sound IDs; common1003's IDs are NULL.
// This explicit policy context has NO invented initial state, native update,
// factory admission or scheduler. It owns the only FX pool during this scene.
template<class Pump>
void actual_header_events(scene::Services& services,isle_arena& arena,
                          environment_sound::Environment& environment,
                          const beach::ActorResources& resources,a::SlotCapacity capacity,
                          std::span<const menu::EffectBank* const> banks,Pump& pump) {
    const std::array capacities{capacity};
    auto made=a::Runtime::create({resources.models(),resources.materials(),resources.animations(),menu::no_index},
                                capacities,{},{32768,4096});
    require(bool(made),"actual header event Runtime admission failed");
    auto& runtime=*made.runtime;
    EventPose pose{services,runtime,{},nullptr};
    menu::EffectRuntime effects(banks,services.rng,{&pose,EventPose::read,EventPose::write});
    require(bool(effects.status()),"standalone canonical FX admission failed");
    auto* particle_owner=services.object(static_cast<std::uint32_t>(-6));
    auto* effect_owner=services.object(static_cast<std::uint32_t>(-7));
    require(particle_owner&&effect_owner,"standalone canonical FX native owners failed");
    isle_span storage{};
    require(isle_arena_allocate(&arena,sizeof(IsleActorSource),alignof(IsleActorSource),&storage)==ISLE_OK,
            "stable canonical event record allocation failed");
    auto* record=std::construct_at(reinterpret_cast<IsleActorSource*>(storage.data));
    bool sound_observed=false,stop_observed=false,loop_observed=false,child_observed=false,immediate_observed=false;
    bool null_sound_observed=false,terminal_immediate_observed=false;
    std::uint32_t identity=20000;
    for (const auto& header:resources.headers()) {
        const auto& init=resources.initializations()[header.initialization];
        const auto acquired=runtime.acquire(0,init.model,true);
        require(bool(acquired),"actual sound header instance acquisition failed");
        const auto handle=acquired.handle;
        auto* object=services.object(identity++,3);
        require(object,"actual event context native object allocation failed");
        *record={};
        native(isle_actor_source_bind(services.game,object,record,&services.rng,&runtime));
        record->id=static_cast<std::int32_t>(init.actor_id);
        record->tangible=1; record->playerDist=0; record->initData=&init; record->animSetup=&init.setup;
        std::vector<IsleGameModelNodeInit> attachment(capacity.nodes);
        IsleGameModel* model=nullptr;
        require(bool(menu::model::attach_source_model(services.game,object,resources.models()[init.model],
                    runtime.model(handle),menu::model::RootRecipe::source,attachment,&model)),
                "actual sound header native model attachment failed");
        const IsleGameModelTransform placement{{0,0,200},{},{1,1,1}};
        native(isle_game_model_set_placement(services.game,model,ISLE_GAME_MODEL_PLACEMENT_RTS,&placement));
        pose.handle=handle; pose.model=model;
        actor::EventServices context{services,environment,effects,runtime,model,true,particle_owner,effect_owner};
        accepted(actor::apply_source_animation(*record,handle,&header,7,true,init,resources,context));
        const auto forced_time=runtime.model(handle).animation_time;
        const auto forced_pose=runtime.model(handle).nodes[0].transform;
        accepted(actor::apply_source_animation(*record,handle,&header,31,false,init,resources,context));
        require(record->animHeader==&header&&record->modelAnims==header.model_animation_table.guest_address&&
                runtime.model(handle).animation_time!=31,"actual same-array setter rebound ignored startTime");
        accepted(actor::apply_source_animation(*record,handle,&header,7,true,init,resources,context));
        require(runtime.model(handle).animation_time==forced_time&&
                same(runtime.model(handle).nodes[0].transform.position,forced_pose.position),
                "actual force/startTime failed repeatable immediate pose");
        for (const auto& next:resources.headers()) if (next.initialization==header.initialization) {
            const auto old_model=record->modelAnims,old_material=record->matAnims;
            std::vector<float> palettes;
            for (const auto& material:runtime.model(handle).materials) palettes.push_back(material.palette_index);
            accepted(actor::apply_source_animation(*record,handle,&next,0,false,init,resources,context));
            require(record->animHeader==&next&&record->modelAnims==next.model_animation_table.guest_address&&
                    record->matAnims==next.material_animation_table.guest_address,
                    "cross-header setter confused header identity with actual ARRAY identities");
            if (next.resets_materials()) {
                const auto materials=runtime.model(handle).materials;
                for (std::size_t m=0;m<materials.size();++m)
                    require(materials[m].palette_index==palettes[m]&&materials[m].clock.phase==a::Phase::disabled,
                            "actual outerNULL material reset changed palette or retained animation");
            } else if (old_material!=next.material_animation_table.guest_address) {
                for (const auto& material:runtime.model(handle).materials)
                    require(material.clock.speed==next.speed,"actual material ARRAY rebind did not use header speed");
            }
            if (old_model!=next.model_animation_table.guest_address)
                require(runtime.model(handle).nodes[0].clock.speed==next.speed,"actual changed model ARRAY ignored source speed");
        }
        if (!null_sound_observed&&!header.sound_ids.empty()) {
            require(environment.play({object,model,&record->playerDist},1,header.sound_ids.front()),
                    "actual categorized sound for NULL-header preservation failed");
            const auto voice=environment.sound_handle(header.sound_ids.front());
            accepted(actor::apply_source_animation(*record,handle,nullptr,0,false,init,resources,context));
            if (voice>=0) {
                require(environment.sound_handle(header.sound_ids.front())==voice,
                        "NULL header blanket-stopped real environmental sound");
                null_sound_observed=true;
            }
        }
        if (!immediate_observed&&!header.sound_ids.empty()) {
            for (unsigned start=0;start<512&&!immediate_observed;++start) {
                if (!(start%32)) pump();
                require(environment.stop(object),"immediate current-header sound isolation failed");
                record->animHeader=nullptr;
                accepted(actor::apply_source_animation(*record,handle,&header,float(start),true,init,resources,context));
                for (const auto& event:runtime.events()) if (event.node==0&&event.event_id>=0&&event.event_id<4&&
                                                            static_cast<std::int32_t>(event.argument)>0) {
                    const auto index=static_cast<std::size_t>(static_cast<std::int32_t>(event.argument)-1);
                    require(record->animHeader==&header&&index<header.sound_ids.size(),
                            "immediate evaluation did not publish its actual current header before callback");
                    if (environment.sound_handle(header.sound_ids[index])>=0) immediate_observed=true;
                }
                effects.detach_instance(handle);
            }
        }
        if (!terminal_immediate_observed) {
            for (unsigned start=0;start<512&&!terminal_immediate_observed;++start) {
                if (!(start%32)) pump();
                record->loopCount=9; record->pokemonLoopTarget=8; record->lastAnimationFrame=13;
                record->processFlags|=POKEMON_PROCESS_FLAG_ANIMATION_ENDED|POKEMON_PROCESS_FLAG_PATH_ENDED;
                accepted(actor::apply_source_animation(*record,handle,&header,float(start),true,init,resources,context));
                for (const auto& event:runtime.events()) if (event.event_id==-1||event.event_id==-2) {
                    require(record->loopCount==0&&record->pokemonLoopTarget==1&&record->lastAnimationFrame==0&&
                            !(record->processFlags&POKEMON_PROCESS_FLAG_ANIMATION_ENDED)&&
                            (record->processFlags&POKEMON_PROCESS_FLAG_PATH_ENDED),
                            "real immediate terminal callback ran after setter resets or cleared unrelated flags");
                    terminal_immediate_observed=true;
                }
                effects.detach_instance(handle);
            }
        }
        accepted(actor::apply_source_animation(*record,handle,&header,0,true,init,resources,context));
        for (const auto& event:runtime.events())
            if (!child_observed&&event.node!=0&&event.event_id>=0&&event.event_id<4&&
                static_cast<std::int32_t>(event.argument)>0)
                child_observed=child_event_emission(*record,event,context,pose);
        for (unsigned frame=0;frame<512;++frame) {
            if (!(frame%32)) pump();
            require(bool(runtime.step_model(handle)),"actual sound header immediate/scheduled evaluation failed");
            synchronized(services,model,runtime.model(handle));
            for (const auto& event:runtime.events()) {
                const auto before=record->loopCount;
                const auto converted=static_cast<std::int32_t>(event.argument);
                const bool root=event.node==0&&event.event_id>=0&&event.event_id<4;
                if (root&&converted==0&&!header.sound_ids.empty()) {
                    const auto sound=header.sound_ids.front();
                    // sound_handle is scene-wide. Earlier source events may play
                    // this same clip in another category; isolate this category.
                    require(environment.stop(object),"actual zero-event category fixture isolation failed");
                    require(environment.play({object,model,&record->playerDist},static_cast<std::uint8_t>(event.event_id),sound),
                            "real categorized sound setup for actual stop event failed");
                    const auto voice=environment.sound_handle(sound);
                    native(isle_game_object_set_flags(services.game,object,3));
                    context.sound_enabled=false;
                    accepted(actor::dispatch_event(*record,event,context));
                    if (voice>=0) {
                        require(environment.sound_handle(sound)==-1,"actual zero event failed stop while hidden/sound-disabled");
                        stop_observed=true;
                    }
                    native(isle_game_object_set_flags(services.game,object,0)); context.sound_enabled=true;
                } else {
                    accepted(actor::dispatch_event(*record,event,context));
                    if (root&&converted>0&&!header.sound_ids.empty()) {
                        const auto index=static_cast<std::size_t>(converted-1);
                        require(index<header.sound_ids.size(),"actual root callback escaped admitted sound IDs");
                        if (environment.sound_handle(header.sound_ids[index])>=0) sound_observed=true;
                    }
                    if (!child_observed&&event.node!=0&&event.event_id>=0&&event.event_id<4&&converted>0)
                        child_observed=child_event_emission(*record,event,context,pose);
                }
                if (event.event_id==-1||event.event_id==-2) {
                    require(record->loopCount==before+1,"actual animation end event did not increment source loop");
                    loop_observed=true;
                }
            }
            require(bool(effects.update_effects(EventPose::world,&pose))&&bool(effects.update_particles()),
                    "standalone canonical FX failed while its actual native model remained live");
            if (runtime.model(handle).nodes[0].clock.phase==a::Phase::finished) break;
        }
        effects.detach_instance(handle); effects.clear_targets(handle);
        effects.reset();
        require(environment.stop(object),"standalone actual source sound release failed");
        environment.fix_parameters(object); environment.release_identity(object);
        require(bool(runtime.begin_retire(handle)),"event instance begin-retire failed");
        services.erase(object);
        require(bool(runtime.release_retired(handle)),"event native joined instance release failed");
        pose.handle={}; pose.model=nullptr;
        pump();
    }
    require(sound_observed,"no actual noncommon1003 sound-bearing header produced an accepted categorized voice");
    require(stop_observed,"no actual root zero event demonstrated hidden/sound-disabled category stop");
    require(loop_observed,"no actual admitted animation end event reached loop policy");
    require(child_observed,"no actual child callback emitted particles dependent on its canonical native child and placement");
    require(null_sound_observed,"no actual NULL-header categorized voice preservation subject was admitted");
    require(immediate_observed,"no actual sound-bearing immediate header callback reached current-header voice policy");
    require(terminal_immediate_observed,"no real immediate terminal event demonstrated setter trailing-reset ordering");
    services.erase(particle_owner); services.erase(effect_owner);
}
void run(rom::Snapshot& snapshot,const std::filesystem::path& profile) {
    require(std::filesystem::is_directory(profile)&&std::filesystem::is_empty(profile),"explicit private profile directory must exist and be empty");
    const world::Resources world_resources(snapshot);
    const auto& setup=world_resources.setup();
    const beach::ActorResources resources(snapshot,{setup.fog_rgb,setup.fog_min,setup.fog_max});
    const environment_sound::BeachResources sound_resources(snapshot);
    const menu::EffectResources effect_resources(snapshot);
    const std::array banks{&effect_resources.bank(menu::EffectGroup::common),&effect_resources.bank(menu::EffectGroup::beach)};
    auto hydrated=game_audio::hydrate(snapshot); require(bool(hydrated),"actual shared audio hydration failed");
    std::array<audio::effects::Section,8> sections; audio::effects::Config effects;
    require(game_audio::native_startup_effects(hydrated.resources,sections,effects)==audio::Error::none,"actual source audio effects configuration failed");
    private_audio::Capture capture; private_audio::Execution execution{snapshot,nullptr,{}};
    capture.samples.resize(8192*audio::AudioService::max_frames*2);
    auto made=game_audio::Session::create(std::move(hydrated.resources),effects,capture.output(),execution.executor());
    require(bool(made),"actual shared audio Session creation failed"); auto& session=*made.session;
    std::uint64_t retrace=0;
    auto pump=[&] {
        ++retrace; require(session.retrace(retrace,retrace*1000000000ull/60)==audio::Error::none,"actual audio retrace failed");
        require(session.submit()==audio::Error::none,"actual checked PCM submission failed");
        audio::TaskView task; require(session.build(task)==audio::Error::none,"actual source audio task build failed");
        if (task.token.serial) { private_audio::execute(session,execution,task); require(session.submit()==audio::Error::none,"completed PCM acceptance failed"); }
    };
    {
        game::State state(profile); require(state.initialize(1),"owned private profile initialization failed");
        game_input::Service input; scene::Storage storage(32*1024*1024); native(storage.create());
        {
            scene::Services services(storage.game(),storage.arena(),{{4,6,0,1,2},{}},input,session,state);
            require(!services.failed(),services.error());
            auto* camera_object=services.object(1,0); require(camera_object,"actual camera object failed");
            IsleGameCamera* camera=nullptr; native(isle_game_attach_camera(services.game,camera_object,&camera));
            const IsleGameLookAt view{{0,0,0},{1,0,0},{0,1,0}}; native(isle_game_camera_set_look_at(services.game,camera,&view));
            environment_sound::Environment environment(services,storage.arena(),sound_resources,nullptr,camera);
            focus::Detector detector;
            WorldShift shift{&services,camera,nullptr};
            world::World world(services,world_resources,{&shift,nullptr,nullptr,nullptr,WorldShift::camera_shift,WorldShift::effect_shift,nullptr});
            a::SlotCapacity capacity{};
            for (const auto& model:resources.models()) { const auto c=a::slot_capacity(model); capacity.nodes=std::max(capacity.nodes,c.nodes); capacity.material_attachments=std::max(capacity.material_attachments,c.material_attachments); }
            const std::array capacities{capacity,capacity};
            {
                auto* listener_object=services.object(2,9); require(listener_object,"standalone actual listener object failed");
                const IsleGameModelNodeInit listener_node{UINT32_MAX,{{},{},{1,1,1}},ISLE_GAME_MODEL_TRANSLATE};
                IsleGameModel* listener=nullptr; native(isle_game_attach_model(services.game,listener_object,&listener_node,1,&listener));
                require(environment.set_listener(listener,camera),"standalone actual listener binding failed");
                actual_header_events(services,storage.arena(),environment,resources,capacity,banks,pump);
                require(environment.set_listener(nullptr,nullptr),"standalone actual listener release failed");
                services.erase(listener_object);
            }
            {
                actor::Owner owner(services,resources,world,environment,detector,capacities,banks,{32768,4096}); shift.owner=&owner;
                Consumer consumer(services,owner,world,resources,detector);
                consumer.invalid_placement(); consumer.normal(); consumer.hidden(); consumer.pause_and_update(); consumer.capacity_and_reuse();
                auto* player_object=services.object(2,9); require(player_object,"actual listener object allocation failed");
                const IsleGameModelNodeInit player_node{UINT32_MAX,{{},{},{1,1,1}},ISLE_GAME_MODEL_TRANSLATE};
                IsleGameModel* player=nullptr; native(isle_game_attach_model(services.game,player_object,&player_node,1,&player));
                accepted(owner.set_player(player_object,player,camera));
                world::Spawn sound_spawn; sound_spawn.id=1003; sound_spawn.behavior=1;
                sound_spawn.position={0,0,2};
                const auto sounding=owner.spawn({30000,consumer.common_definition,&world.blocks()[0],&world.blocks()[0],&sound_spawn});
                accepted(sounding.status);
                auto* sound_record=owner.source(sounding.instance);
                sound_record->playerDist=200;
                require(environment.play({sounding.object,owner.native_model(sounding.instance),&sound_record->playerDist},1,0x34),
                        "actual source categorized voice setup failed");
                const auto voice=environment.sound_handle(0x34);
                require(voice>=0,"actual source0x34 refused cleanup/fix lifetime subject");
                accepted(owner.cleanup(sounding.instance));
                const auto retirement=owner.membership(sounding.instance);
                require(retirement.retiring&&retirement.joined&&!retirement.registered,
                        "cleanup did not expose actual native join before physical collection");
                accepted(owner.collect_joined());
                require(environment.sound_handle(0x34)==voice&&environment.update()&&
                        session.playing_sound(static_cast<unsigned>(voice))==0x34,
                        "cleanup blanket-stopped source voice or retained a freed live-model sound borrow");
                accepted(owner.set_player(nullptr,nullptr,nullptr)); services.erase(player_object);
                attached_lifetime(consumer,banks);
                shift.owner=nullptr;
            }
            world.retire(); environment.cleanup();
        }
        native(storage.destroy());
    }
    require(session.stop()==audio::Error::none,"actual shared audio stop failed");
    for (unsigned n=0;n<240&&session.state()!=game_audio::Session::State::draining;++n) pump();
    require(session.state()==game_audio::Session::State::draining&&session.service().voices_retired(),"actual audio voice graph failed retirement");
    require(session.poll_drain()==audio::Error::none&&session.poll_drain()==audio::Error::none&&session.state()==game_audio::Session::State::drained,"actual PCM owner failed drain");
    std::cout<<"actor_owner_private_CPU_behavior=passed GPU_lifetime_Original_Windows11=not_claimed\n";
}
} // namespace
int main(int argc,char** argv) {
    try {
        require(argc==3,"usage: actor-owner-rom-smoke PRIVATE_ROM EXISTING_EMPTY_PRIVATE_PROFILE_DIRECTORY");
        auto snapshot=isle::rom::read_snapshot(argv[1]); run(snapshot,argv[2]); return EXIT_SUCCESS;
    } catch (const std::exception& error) { std::cerr<<"actor-owner-rom-smoke: "<<error.what()<<'\n'; return EXIT_FAILURE; }
}
