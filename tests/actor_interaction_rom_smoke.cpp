// Explicit private-ROM CPU/API selector consumer, excluded from default/CTest.
// Actual Source23 automatic/FIFO C executes under native OM fences. Player,
// graphs, cached distances, detector pixels and poses are fixed API-authored
// inputs, NOT Original observations, a production Player FSM, all71 graphs,
// graphics/GPU-fence proof, fullgame or Windows11 qualification. Actual audio
// RSP tasks and an owned PCM stream are drained at the end; no GPU is required.
#include "actor_interaction_probe.h"
#include "actor_movement_probe.h"
#include "actor_behavior_bindings.hpp"
#include "actor_interaction_services.hpp"
#include "item_owner.hpp"
#include "actual_audio_driver.hpp"
#include "actual_player_source.hpp"
#include "effect_bank_resources.hpp"
#include "scene_storage.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <memory>
#include <source_location>
#include <span>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace {
using namespace isle;
namespace a = menu::animation;
namespace it = items;
constexpr auto descend = func_8035DFB0_4FE3C0;
constexpr auto hidden = func_8035E034_4FE444;
constexpr float untouched_distance = -123.0f;
void require(bool value,const char* text) {
    if (!value) {
        std::cerr<<"C selector guard: "<<text<<'\n';
        throw std::runtime_error(text);
    }
}
void native(IsleGameResult value,std::source_location where=std::source_location::current()) {
    if (value!=ISLE_GAME_OK)
        std::cerr<<"C native result="<<static_cast<int>(value)<<" function="<<where.function_name()<<" line="<<where.line()<<'\n';
    require(value==ISLE_GAME_OK,"native interaction consumer operation failed");
}
void accepted(actor::OwnerStatus value) { require(bool(value),value.detail); }
void accepted(it::OwnerStatus value) { require(bool(value),value.detail); }
bool near(float x,float y) { return std::abs(x-y)<0.0001f; }
bool same(IsleGameVec3 x,IsleGameVec3 y) { return near(x.x,y.x)&&near(x.y,y.y)&&near(x.z,y.z); }

struct Rows {
    std::array<IsleActorInteraction,4> rows{};
    IsleActorInteractionGraph graph{};
    Rows(std::initializer_list<IsleActorInteraction> input) {
        require(input.size()<rows.size(),"fixed API fixture graph storage exceeded");
        std::copy(input.begin(),input.end(),rows.begin());
        rows[input.size()]={58,nullptr,0,nullptr};
        graph={rows.data(),input.size()+1,0};
    }
    Rows(const Rows&)=delete;
    Rows& operator=(const Rows&)=delete;
};

// A semantic API fixture owns its actual native identity until join AND all
// queue/canonical-field credits close. The primary instance is the Player
// fixture; its native PLAYER payload and semantic lifetime are independent.
// Its one real fixture-owned target field also exercises receiver-side holds.
struct ApiSourceOwner {
    scene::Services& services;
    IsleGameObject* object=nullptr;
    IsleGameModel* model=nullptr;
    IsleGameObject* held_target=nullptr;
    std::array<IsleGameObject**,1> fields{{&held_target}};
    unsigned retiring=0,joined=0,errors=0;
    bool released=false;
    ApiSourceOwner(scene::Services& s,std::uint32_t id,bool camera_sender=false) : services(s) {
        object=services.object(id,9); require(object,"API semantic source allocation failed");
        if (camera_sender) {
            IsleGameCamera* camera=nullptr;
            native(isle_game_attach_camera(services.game,object,&camera));
        } else {
            IsleGameModelNodeInit node{}; node.parent=UINT32_MAX;
            node.transform.scale={1,1,1}; node.matrix_kind=ISLE_GAME_MODEL_CONTEXTUAL_RPY;
            native(isle_game_attach_model(services.game,object,&node,1,&model));
            const IsleGameModelTransform placement{{0,0,0},{0,0,0},{1,1,1}};
            native(isle_game_model_set_placement(services.game,model,ISLE_GAME_MODEL_PLACEMENT_RTS,&placement));
        }
        require(services.set_lifecycle(object,{before,after,this}) &&
                services.source_owner_register(object,fields),"API semantic source ownership admission failed");
    }
    ApiSourceOwner(const ApiSourceOwner&)=delete;
    ApiSourceOwner& operator=(const ApiSourceOwner&)=delete;
    static void before(IsleGameObject* object,void* user) noexcept {
        auto& self=*static_cast<ApiSourceOwner*>(user); ++self.retiring;
        if (self.retiring!=1 || self.joined || object!=self.object ||
            !actor_interaction_object_alive(object) ||
            (self.model && !actor_interaction_model_alive(self.model)) || self.held_target) ++self.errors;
    }
    static void after(IsleGameObject* object,void* user) noexcept {
        auto& self=*static_cast<ApiSourceOwner*>(user); ++self.joined;
        if (self.retiring!=1 || self.joined!=1 || object!=self.object ||
            (self.model && actor_interaction_model_alive(self.model))) ++self.errors;
    }
    bool close() noexcept {
        if (released) return true;
        if (held_target && !services.source_reference_assign(object,&held_target,nullptr)) return false;
        if (!services.source_owner_joined(object)) services.erase(object);
        if (!services.source_owner_joined(object) || !services.source_owner_collectable(object) ||
            !services.source_owner_release(object)) return false;
        released=true;
        return retiring==1 && joined==1 && errors==0;
    }
    ~ApiSourceOwner() { if (!released && !close()) std::terminate(); }
};

struct Shift {
    scene::Services* services;
    IsleGameCamera* camera;
    actor::Owner* actors=nullptr;
    it::Owner* items=nullptr;
    IsleGameCommandPool native_world_pool{};
    std::array<IsleGameObject*,27> native_world_objects{};
    std::array<IsleGameCommandQueue,27> native_world_queues{};
    static bool camera_shift(void* user,world::Vec3 delta) noexcept {
        auto& self=*static_cast<Shift*>(user);
        const IsleGameCameraParameters* parameters=nullptr;
        if (!self.services->check(isle_game_camera_parameters(self.services->game,self.camera,&parameters),"selector camera borrow")) return false;
        auto look=parameters->look_at;
        for (unsigned n=0;n<3;++n) { const float d=n==0?delta.x:n==1?delta.y:delta.z; look.eye[n]+=d; look.at[n]+=d; }
        if (!self.services->check(isle_game_camera_set_look_at(self.services->game,self.camera,&look),"selector camera shift")) return false;
        if (self.items && !self.items->displace(delta)) {self.services->fail("selector Item displacement");return false;}
        return true;
    }
    static bool effects_shift(void* user,world::Vec3 delta) noexcept {
        auto& self=*static_cast<Shift*>(user);
        if (!self.actors) { self.services->fail("selector effects owner absent"); return false; }
        self.actors->effects().translate({delta.x,delta.y,delta.z});
        return bool(self.actors->effects().status());
    }
    static bool commands(void* user,IsleGameObject* object) noexcept {
        auto& self=*static_cast<Shift*>(user);
        for(std::size_t n=0;n<self.native_world_objects.size();++n)
            if(!self.native_world_objects[n] || self.native_world_objects[n]==object) {
                self.native_world_objects[n]=object; int erase=0;
                if(!self.services->check(isle_game_commands(self.services->game,object,&self.native_world_pool,
                        &self.native_world_queues[n],nullptr,nullptr,&erase),"semantic World native command drain")) return false;
                return !erase || self.services->check(isle_game_delete_object(self.services->game,object),"semantic World command deletion");
            }
        self.services->fail("bounded semantic World command receiver capacity exceeded");
        return false;
    }
    ~Shift() {
        for(auto& queue:native_world_queues)isle_game_commands_release(&native_world_pool,&queue);
    }
};

std::unique_ptr<actor::BehaviorBindings> make_bindings(const beach::BehaviorResources& resources,
    std::span<const std::uint32_t> graphs={},std::span<const std::uint32_t> weighted={}) {
    // These are the actual compiled six-function closure, not mappings for any
    // uncompiled pinned callback. Constructor consumes these temporary spans.
    const std::array<std::uint32_t,2> common_deps{0x8035dfb0u,0x8035e034u};
    const std::array<std::uint32_t,1> hidden_deps{0x8035dfb0u};
    const std::array<std::uint32_t,1> magikarp_deps{0x802cac08u};
    const std::array<std::uint32_t,1> path_deps{0x8035e298u};
    const std::array<std::uint32_t,1> emission_deps{0x8035df20u};
    const std::array bindings{
        actor::BehaviorCallbackBinding{0x8035df20u,func_8035DF20_4FE330,common_deps,{},{}},
        actor::BehaviorCallbackBinding{0x8035dfb0u,descend,{},{},{}},
        actor::BehaviorCallbackBinding{0x8035e034u,hidden,hidden_deps,{},{}},
        actor::BehaviorCallbackBinding{0x802cab50u,func_beach_802CAB50,magikarp_deps,{},{}},
        actor::BehaviorCallbackBinding{0x802cac08u,func_beach_802CAC08,path_deps,{},{}},
        actor::BehaviorCallbackBinding{0x8035e298u,func_8035E298_4FE6A8,emission_deps,{},{}}
    };
    // Only these two roots are requested: later local graphs actually execute
    // DFB0/E034 obtained through retained transitive closure, not extra roots.
    const std::array<std::uint32_t,2> roots{0x8035df20u,0x802cab50u};
    return std::make_unique<actor::BehaviorBindings>(resources,bindings,graphs,weighted,roots);
}

struct Consumer {
    scene::Services& services;
    actor::Owner& actors;
    environment_sound::Environment& environment;
    it::Owner& items;
    it::Globals& globals;
    world::World& world;
    focus::Detector& detector;
    ApiSourceOwner& player;
    const beach::ActorResources& resources;
    const beach::BehaviorResources& behavior;
    std::vector<actor::SpawnResult> subjects;
    std::vector<std::unique_ptr<ApiSourceOwner>> extra_sources;
    ActorInteractionProbe probe{};
    ActorMovementProbe movement{};
    std::uint32_t next_id=10000;
    bool shut_down=false;
    Consumer(scene::Services& s,actor::Owner& ac,environment_sound::Environment& e,it::Owner& i,it::Globals& g,
             world::World& w,focus::Detector& d,ApiSourceOwner& p,
             const beach::ActorResources& ar,const beach::BehaviorResources& br)
        : services(s),actors(ac),environment(e),items(i),globals(g),world(w),detector(d),player(p),resources(ar),behavior(br) {}

    actor::SpawnResult spawn(IsleGameVec3 position={0,0,0},std::uint32_t actor_id=1003) {
        std::size_t definition=beach::actor_no_index;
        for (std::size_t n=0;n<resources.definitions().size();++n)
            if (resources.definitions()[n].actor_id==actor_id) definition=n;
        require(definition!=beach::actor_no_index,"actual requested Actor definition absent");
        world::Spawn descriptor; descriptor.id=actor_id; descriptor.behavior=1;
        descriptor.position={0,0,0}; descriptor.scale={1,1,1};
        auto result=actors.spawn({next_id++,definition,&world.blocks()[0],&world.blocks()[0],&descriptor});
        accepted(result.status); subjects.push_back(result);
        require(actor_interaction_native_update(result.object),"Actor is not using its actual Pokemon_Update fnUpdate");
        prepare(result); place(result,position); return result;
    }
    IsleActorSource& source(const actor::SpawnResult& subject) {
        auto* result=actors.source(subject.instance); require(result,"live canonical Actor record absent"); return *result;
    }
    void suppress(IsleGameObject* object,bool value) { native(isle_game_pause_updates(services.game,object,value)); }
    void tick(std::source_location where=std::source_location::current()) {
        native(isle_game_tick(services.game),where); require(!services.failed(),services.error());
    }
    void place(const actor::SpawnResult& subject,IsleGameVec3 position) {
        const IsleGameModelTransform value{position,{0,0,0},{1,1,1}};
        accepted(actors.set_placement(subject.instance,value));
    }
    const IsleGameModelNode& borrow_root(const actor::SpawnResult& subject) {
        const IsleGameModelNode* nodes=nullptr; std::size_t count=0;
        native(isle_game_model_peek_nodes(services.game,actors.native_model(subject.instance),&nodes,&count));
        require(count!=0,"actual Actor root absent"); return nodes[0];
    }
    float root_y(const actor::SpawnResult& subject) { return borrow_root(subject).transform.position.y; }
    void reference(const actor::SpawnResult& subject,IsleGameObject** field,IsleGameObject* sender) {
        require(services.source_reference_assign(subject.object,field,sender),"canonical reference assignment failed");
    }
    void prepare(const actor::SpawnResult& subject) {
        auto& p=source(subject); p.transitionGraph=nullptr;
        reference(subject,&p.apple,nullptr); reference(subject,&p.interactionTarget,nullptr);
        native(isle_actor_source_pause_object(&p,1));
        if (p.stateProc) native(isle_game_end_process(services.game,p.stateProc));
        p.stateProc=nullptr;
        if (p.pathProc && actor_interaction_process_alive(p.pathProc)) native(isle_game_end_process(services.game,p.pathProc));
        p.pathProc=nullptr;
        native(isle_game_end_source_processes(services.game,subject.object,descend));
        native(isle_game_end_source_processes(services.game,subject.object,hidden));
        auto local=actors.animation().model(subject.instance).nodes[0].transform;
        local.position={0,0,0}; accepted(actors.set_local(subject.instance,0,local));
        place(subject,{0,0,0});
        p.playerDist=2000; p.interactionDist=untouched_distance; p.dirToNeighbors={17,19,23};
        p.processFlags=0; p.counter=0; p.loopCount=0; p.pokemonLoopTarget=100;
        p.tangible=1; p.miscVars[0].field1=0;
        native(isle_game_object_set_flags(services.game,subject.object,0x40u));
        actors.player_source().IsPaused=0; actors.player_source().D_80382D44_523154=0;
    }
    void automatic(const actor::SpawnResult& subject,Rows* rows,IsleGameProcessKind kind=ISLE_GAME_STACKFUL,
                   std::source_location where=std::source_location::current()) {
        auto& p=source(subject); p.transitionGraph=rows?&rows->graph:nullptr;
        suppress(subject.object,true);
        native(actor_interaction_probe_start(&probe,&p,kind,0,1));
        tick(where);
        require(probe.entered==1 && probe.failures==0 && probe.observed_kind==static_cast<int>(kind),
                "automatic C did not execute once in its real native kind/priority context");
        p.transitionGraph=nullptr; // End the caller's graph borrow before Rows dies.
    }
    void no_selection(const actor::SpawnResult& subject) {
        const auto& p=source(subject);
        require(!p.apple && !p.interactionTarget && p.interactionDist==untouched_distance &&
                actor_interaction_function_count(subject.object,descend,6)==0 && root_y(subject)==0,
                "ineligible automatic row acquired references, distance, or an actual auxiliary effect");
    }
    void selected_aux(const actor::SpawnResult& subject,IsleGameObject* target=nullptr) {
        require(source(subject).interactionTarget==target &&
                actor_interaction_function_count(subject.object,descend,6)==1 && near(root_y(subject),-.2f),
                "selected actual auxiliary did not subtract .2 at source priority6 or retain expected target");
    }
    void enqueue(const actor::SpawnResult& subject,int command,IsleGameObject* sender=nullptr) {
        require(services.send_command(subject.object,command,sender),"actual source FIFO enqueue failed");
    }
    void fifo(const actor::SpawnResult& subject,Rows* rows,std::source_location where=std::source_location::current()) {
        auto& p=source(subject); p.transitionGraph=rows?&rows->graph:nullptr;
        suppress(subject.object,false); tick(where); suppress(subject.object,true);
        p.transitionGraph=nullptr;
    }
    void clear_items() {
        for (const auto& subject:subjects) if (auto* p=actors.source(subject.instance)) {
            p->transitionGraph=nullptr;
            reference(subject,&p->apple,nullptr); reference(subject,&p->interactionTarget,nullptr);
            native(isle_actor_source_pause_object(p,1)); suppress(subject.object,false);
        }
        accepted(items.retire()); tick(); tick(); accepted(items.collect_joined());
        for (const auto& subject:subjects) if (actors.source(subject.instance)) suppress(subject.object,true);
    }
    void remove_seed_item(const it::SpawnResult& obsolete) {
        accepted(items.delete_item(obsolete.object));
        for (const auto& subject:subjects) if (auto* p=actors.source(subject.instance)) {
            p->transitionGraph=nullptr; suppress(subject.object,false);
        }
        tick(); tick(); accepted(items.collect_joined());
        for (const auto& subject:subjects) if (actors.source(subject.instance)) suppress(subject.object,true);
    }
    it::SpawnResult item(float x,bool apple=true,std::uint8_t state=ISLE_ITEM_STILL) {
        auto result=apple?items.spawn_apple({x,0,0},{0,0,0}):items.spawn_ball({x,0,0},{0,0,0});
        accepted(result.status); items.source(result.instance)->state=state;
        accepted(items.pause()); suppress(result.object,true); return result;
    }

    IsleGameModelPlacement placement(const actor::SpawnResult& subject) {
        const IsleGameModelPlacement* value=nullptr;
        native(isle_game_model_placement(services.game,actors.native_model(subject.instance),&value));
        return *value;
    }
    void movement_start(const actor::SpawnResult& subject,ActorMovementOperation operation,
                        float a=0,float b=0,float c=0,float d=0,std::uint32_t flags=0) {
        require(!movement.process || movement.returned || !actor_interaction_process_alive(movement.process),
                "movement context reused before real process completion/cancellation");
        movement.operation=operation; movement.a=a; movement.b=b; movement.c=c; movement.d=d; movement.flags=flags;
        suppress(subject.object,true);
        native(actor_movement_probe_start(&movement,&source(subject))); tick();
        require(movement.entered==1,"original C movement helper did not enter its real process");
    }
    void movement_cancel() {
        require(movement.process && actor_interaction_process_alive(movement.process) && !movement.returned,
                "permanent/waiting helper unexpectedly completed before cancellation");
        native(isle_game_end_process(services.game,movement.process));
        require(!actor_interaction_process_alive(movement.process),"native movement cancellation failed");
        movement.process=nullptr;
    }
    void movement_heading_and_circles(const actor::SpawnResult& subject) {
        prepare(subject);
        movement.point_to={3,120,4}; movement.point_from={0,0,0};
        movement_start(subject,ACTOR_HEADING_VECTOR,10);
        require(movement.returned && same(movement.vector,{6,0,8}) &&
                near(movement.scalar,.6435011088f),
                "heading-vector horizontal normalization consumed vertical distance");
        movement.point_to={-10,100,0};
        movement_start(subject,ACTOR_HEADING_VECTOR,-5);
        require(movement.returned && movement.vector.x==-5 && movement.vector.y==0 &&
                movement.vector.z==0 && std::signbit(movement.vector.z) &&
                near(movement.scalar,4.7123889804f),
                "same-Z heading fallback lost negative-length direction or signed zero");

        prepare(subject); auto& p=source(subject); p.hSpeed=100;
        p.processFlags=POKEMON_PROCESS_FLAG_MOVEMENT_PAUSED;
        const std::array<float,1> forbid{4294967296.0f};
        p.forbiddenGround=forbid.data(); p.forbiddenGroundCount=forbid.size();
        services.rng.seed=0x12345678u;
        movement_start(subject,ACTOR_CIRCLES_DEFAULT,50);
        require(movement.returned && services.rng.seed==0xb3e97b5bu &&
                same(placement(subject).transform.position,{0,0,0}) &&
                placement(subject).transform.rotation.y==0 && p.currGround.height==-9000,
                "blocked circle did not consume one RNG draw before walking or published yaw after rejection");
        p.forbiddenGround=nullptr; p.forbiddenGroundCount=0;
        services.rng.seed=0x12345678u;
        movement_start(subject,ACTOR_CIRCLES,50,.1f);
        auto first=placement(subject).transform;
        require(!movement.returned && services.rng.seed==0xb3e97b5bu &&
                near(first.position.x,-2.8013877869f) && near(first.position.z,1.7430839539f) &&
                first.position.y==-9000 && near(first.rotation.y,6.1831853072f),
                "circle source RNG, theoretical arc, shortest turn or caller-only pause distinction differs");
        first.position={100,77,200}; accepted(actors.set_placement(subject.instance,first));
        tick();
        const auto second=placement(subject).transform;
        require(!movement.returned && services.rng.seed==0xb3e97b5bu &&
                near(second.position.x,97.3196792603f) && near(second.position.z,201.9240379333f) &&
                second.position.y==-9000,
                "circle recentered after a wait, replayed RNG or lost its source theoretical previous point");
        movement_cancel();
    }
    void movement_vertical(const actor::SpawnResult& subject) {
        prepare(subject); auto& p=source(subject);
        p.hSpeed=100; p.jumpVel=100;
        const auto root=borrow_root(subject);
        movement_start(subject,ACTOR_JUMP,9,0,0,0);
        require(!movement.returned && same(placement(subject).transform.position,{0,3.3f,3.3f}),
                "nested original MoveXZ lost the first vertical publication");
        auto edited=placement(subject).transform;
        edited.position={11,20,30}; edited.rotation={.25f,0,.5f}; edited.scale={2,3,4};
        accepted(actors.set_placement(subject.instance,edited));
        accepted(actors.change_block(subject.instance,{0,0,0},{1,2,3}));
        const auto relocated=placement(subject).transform;
        p.processFlags|=POKEMON_PROCESS_FLAG_MOVEMENT_PAUSED; tick();
        require(same(placement(subject).transform.position,relocated.position) && !movement.returned,
                "Jump did not preserve external relocation during a paused wait");
        p.processFlags&=~POKEMON_PROCESS_FLAG_MOVEMENT_PAUSED; tick();
        auto now=placement(subject);
        require(near(now.transform.position.y,relocated.position.y+3.3f) &&
                near(now.transform.position.z,relocated.position.z+3.3f) &&
                same(now.transform.scale,edited.scale) && same(now.transform.rotation,edited.rotation),
                "Jump resumed through a stale AUX view or overwrote interleaved fields");
        tick(); require(movement.returned==1,"Jump accumulated-height terminal boundary did not finish");
        require(same(borrow_root(subject).transform.position,root.transform.position),
                "movement modified independent animated root-local position");
        for (auto op:{ACTOR_JUMP_HEIGHT,ACTOR_FALL,ACTOR_FALL_HEIGHT}) {
            prepare(subject); p.hSpeed=100; p.jumpVel=100; place(subject,{0,10,0});
            movement_start(subject,op,op==ACTOR_JUMP_HEIGHT?13.3f:op==ACTOR_FALL_HEIGHT?6.7f:3.3f);
            require(movement.returned==1 && near(placement(subject).transform.position.z,3.3f) &&
                    near(placement(subject).transform.position.y,op==ACTOR_JUMP_HEIGHT?13.3f:6.7f),
                    "absolute/relative vertical boundary or nested horizontal preservation differs");
        }
        prepare(subject); p.hSpeed=100; p.processFlags=POKEMON_PROCESS_FLAG_MOVEMENT_PAUSED;
        movement_start(subject,ACTOR_MOVE_XZ);
        require(movement.returned && near(placement(subject).transform.position.z,3.3f),
                "MoveXZ incorrectly acquired the caller-only pause gate");
        movement_start(subject,ACTOR_STEP_DIRECTION);
        require(movement.result==0 && near(placement(subject).transform.position.z,3.3f),
                "direction helper ignored its own pause gate");
        movement_start(subject,ACTOR_TURN,.5f,.5f);
        require(movement.result==1 && near(placement(subject).transform.rotation.y,.5f),
                "Turn equality or per-helper pause distinction differs");
        movement_start(subject,ACTOR_TURN,1.0f,.25f);
        require(movement.result==0 && near(placement(subject).transform.rotation.y,.75f),
                "Turn strict outside-speed boundary differs");
        prepare(subject);
        auto wrap_pose=placement(subject).transform; wrap_pose.rotation.y=-.25f;
        accepted(actors.set_placement(subject.instance,wrap_pose));
        movement_start(subject,ACTOR_TURN,0,.1f);
        require(movement.result==0 && near(placement(subject).transform.rotation.y,6.133185307f),
                "Turn lost signed-truncation normalization or shortest cross-wrap direction");
        prepare(subject); p.jumpVel=-100; p.processFlags=POKEMON_PROCESS_FLAG_MOVEMENT_PAUSED; place(subject,{0,-8999,0});
        movement_start(subject,ACTOR_FALL_GROUND,0);
        require(movement.returned && near(placement(subject).transform.position.y,-9000) &&
                p.currGround.height==-9000 && near(p.jumpVel,-100),
                "FallDownOnGround did not consume full missing-ground record independently of scalar helper");
    }

    void movement_targets(const actor::SpawnResult& subject) {
        prepare(subject); auto& p=source(subject); p.hSpeed=100;
        // The Item has a genuine root-local position, unlike Actor/Player AUX.
        const auto target=item(0); const auto replacement=item(0);
        auto item_position=[&](const it::SpawnResult& value,IsleGameVec3 position) {
            const auto* nodes=static_cast<const IsleGameModelNode*>(nullptr); std::size_t count=0;
            auto* model=items.native_model(value.instance);
            native(isle_game_model_peek_nodes(services.game,model,&nodes,&count));
            require(count>0,"actual Item target root absent");
            auto local=nodes[0].transform; local.position=position;
            native(isle_game_model_set_transform(services.game,model,0,&local));
        };
        item_position(target,{100,0,100}); item_position(replacement,{100,0,100});
        reference(subject,&p.interactionTarget,target.object);
        movement_start(subject,ACTOR_TURN_TARGET,.1f,0,0,0,
                       MOVEMENT_FLAG_UPDATE_TARGET_POS|MOVEMENT_FLAG_STOP_WHEN_TURN_COMPLETED);
        require(!movement.returned && near(placement(subject).transform.rotation.y,.1f),
                "Item root target did not enter original waiting turn");
        reference(subject,&p.interactionTarget,replacement.object);
        item_position(target,{-100,0,100}); tick();
        require(near(placement(subject).transform.rotation.y,0),
                "captured target failed to track original Item movement or retargeted replacement");
        movement_cancel();
        prepare(subject);
        const IsleGameModelTransform player_pose{{100,0,100},{0,0,0},{1,1,1}};
        native(isle_game_model_set_placement(services.game,player.model,ISLE_GAME_MODEL_PLACEMENT_RTS,&player_pose));
        movement_start(subject,ACTOR_TURN_TARGET,1,0,0,0,MOVEMENT_FLAG_TURN_TO_PLAYER);
        require(movement.returned && near(placement(subject).transform.rotation.y,.7853981634f),
                "registered Player AUX was replaced by its unrelated root-local origin");
        const auto actor_target=spawn({-100,0,100});
        prepare(subject); reference(subject,&p.interactionTarget,actor_target.object);
        movement_start(subject,ACTOR_TURN_TARGET,6);
        require(movement.returned && near(placement(subject).transform.rotation.y,5.4977871438f),
                "Actor target did not use canonical AUX");
        prepare(subject); p.hSpeed=100; p.targetPos={0,0,0};
        movement_start(subject,ACTOR_STEP_TARGET,.1f);
        require(movement.result==1 && (p.processFlags&POKEMON_PROCESS_TARGET_REACHED),
                "already-arrived strict interior did not produce reached1");
        prepare(subject); p.hSpeed=100; p.targetPos={0,0,6.6f};
        movement_start(subject,ACTOR_STEP_TARGET,.1f);
        require(movement.result==0 && !(p.processFlags&POKEMON_PROCESS_TARGET_REACHED) &&
                near(placement(subject).transform.position.z,3.3f),
                "exact remaining-step boundary did not remain reached0");
        movement_start(subject,ACTOR_STEP_TARGET,.1f);
        require(movement.result==1,"next exact endpoint failed reached1");
        prepare(subject); p.hSpeed=100; p.targetPos={0,0,100};
        const std::array<float,1> forbid{4294967296.0f}; p.forbiddenGround=forbid.data(); p.forbiddenGroundCount=forbid.size();
        movement_start(subject,ACTOR_STEP_TARGET,.1f);
        require(movement.result==2 && !(p.processFlags&POKEMON_PROCESS_TARGET_REACHED) &&
                same(placement(subject).transform.position,{0,0,0}),
                "blocked actual ground step did not return reached2 without arrival");
        p.forbiddenGround=nullptr; p.forbiddenGroundCount=0;
        clear_items();
    }

    void movement_ground_and_null(const actor::SpawnResult& subject) {
        prepare(subject); require(world.current()==nullptr,"missing-ground diagnostic needs initial NULL World");
        movement_start(subject,ACTOR_GROUND);
        require(movement.returned && movement.scalar==0 && movement.result==0 &&
                movement.ground.height==-9000 && same(movement.ground.normal,{0,1,0}) &&
                movement.ground.surfaceType==UINT32_MAX,
                "original scalar missing-ground0 was conflated with full-record -9000");
        auto& p=source(subject); p.path=nullptr;
        movement_start(subject,ACTOR_PATH,0,1,1);
        require(!movement.returned && p.pathParam==0,"initial NULL path did not permanently wait");
        require(!world.resources().paths.empty(),"actual World has no path for NULL-wait mutation diagnostic");
        p.path=&world.resources().paths.front(); tick(); tick();
        require(!movement.returned && p.pathParam==0 && same(placement(subject).transform.position,{0,0,0}),
                "initial NULL path wait incorrectly resumed after a path appeared");
        movement_cancel();
        for (float parameter:{-1.0f,std::numeric_limits<float>::quiet_NaN()}) {
            prepare(subject); p.path=&world.resources().paths.front();
            movement_start(subject,ACTOR_PATH,parameter,1,1); tick();
            require(!movement.returned && same(placement(subject).transform.position,{0,0,0}),
                    "invalid parameter failed permanent-wait/no-prefix semantics");
            movement_cancel();
        }
    }

    void movement_paths(const actor::SpawnResult& subject) {
        const auto paths=world.resources().paths;
        require(paths.size()>=4,"bounded semantic World lacks four authored path kinds");
        const auto authored=paths.last(4);
        for (const auto& path:authored) {
            prepare(subject); movement.path=&path;
            movement_start(subject,ACTOR_PATH_SAMPLE,1);
            require(movement.returned && same(movement.vector,{5,6,7}),
                    "original four-kind C dispatch failed independent affine endpoint");
        }
        prepare(subject); auto& p=source(subject); p.path=&authored[0];
        movement_start(subject,ACTOR_PATH,1,1.1f,1);
        require(movement.returned && near(p.pathParam,1.25f) &&
                same(placement(subject).transform.position,{500,600,700}),
                "FollowPath wrapped before positive end test or lost endpoint XYZ100 units");
        prepare(subject); p.path=&authored[0];
        movement_start(subject,ACTOR_PATH,0,0,1);
        require(!movement.returned && near(p.pathParam,.25f) &&
                same(placement(subject).transform.position,{100,200,300}),
                "airborne path incorrectly required current World or initial XYZ100 sample");
        auto edited=placement(subject).transform; edited.position.y+=17;
        accepted(actors.set_placement(subject.instance,edited));
        p.processFlags|=POKEMON_PROCESS_FLAG_MOVEMENT_PAUSED; tick();
        require(near(p.pathParam,.25f) && near(placement(subject).transform.position.y,217),
                "FollowPath pause consumed parameter or discarded external height");
        p.processFlags&=~POKEMON_PROCESS_FLAG_MOVEMENT_PAUSED;
        // Replacement path has a different duration. Samples use CURRENT path,
        // but the original captured duration keeps increments at one quarter.
        p.path=&authored[1]; tick();
        require(near(p.pathParam,.5f) && same(placement(subject).transform.position,{500,617,700}),
                "FollowPath failed current-path sampling or recaptured replacement duration after wait");
        movement_cancel();
        prepare(subject); p.path=&authored[0];
        movement_start(subject,ACTOR_PATH,0,0,9);
        require(!movement.returned && near(p.pathParam,1.25f),
                "FollowPath replaced original single subtraction with full wrapping");
        tick(); require(!movement.returned && near(p.pathParam,1.25f),
                        "single-wrap out-of-range parameter did not enter permanent wait");
        movement_cancel();
        prepare(subject); p.path=&authored[0]; place(subject,{0,17,0});
        movement_start(subject,ACTOR_PATH,1,1,1,0,MOVEMENT_FLAG_ON_GROUND);
        require(movement.returned && same(placement(subject).transform.position,{500,17,700}),
                "ON_GROUND path used record -9000 instead of scalar missing-ground0 or multiplied height by100");
        prepare(subject); p.path=&authored[0]; place(subject,{0,17,0});
        movement_start(subject,ACTOR_PATH,1,1,1,0,MOVEMENT_FLAG_FIXED_HEIGHT);
        require(movement.returned && same(placement(subject).transform.position,{500,17,700}),
                "fixed-height path changed independent canonical Y");
        require(world.enter_first(1)==&world.blocks()[1],"actual current-block reset prerequisite absent");
        const auto origin=world.current()->resource->metadata.world_position;
        prepare(subject); place(subject,{17,19,23});
        movement_start(subject,ACTOR_RESET_PATH);
        require(movement.returned && same(placement(subject).transform.position,
                {-origin.x*100,-origin.y*100,-origin.z*100}),
                "original ResetPathPos failed actual current descriptor XYZ100 relocation");
    }
    void movement_path_fault(const actor::SpawnResult& subject) {
        prepare(subject); auto& p=source(subject); p.path=&world.resources().paths[world.resources().paths.size()-4];
        movement_start(subject,ACTOR_PATH,0,0,1);
        auto edited=placement(subject).transform; edited.position={111,222,333};
        accepted(actors.set_placement(subject.instance,edited));
        auto* model=actors.native_model(subject.instance);
        const IsleGameModelPlacement* observed=nullptr;
        native(isle_game_model_placement(services.game,model,&observed));
        p.path=nullptr; // Real current-path mutation, not a forged descriptor/provider.
        require(isle_game_tick(services.game)==ISLE_GAME_EXECUTION_FAILURE &&
                movement.entered==1 && !movement.returned &&
                actor_interaction_model_alive(model) && same(observed->transform.position,{11,22,33}) && near(p.pathParam,.25f),
                "later current-path fault rolled back/replayed prior-sample subtraction prefix");
    }
    void movement_retired_target_fault(const actor::SpawnResult& subject) {
        prepare(subject); auto& p=source(subject);
        const auto target=item(100),replacement=item(-100);
        reference(subject,&p.interactionTarget,target.object);
        movement_start(subject,ACTOR_TURN_TARGET,.1f,0,0,0,
                       MOVEMENT_FLAG_UPDATE_TARGET_POS|MOVEMENT_FLAG_STOP_WHEN_TURN_COMPLETED);
        native(isle_game_pause_source_processes(services.game,subject.object,actor_movement_probe_entry,1));
        reference(subject,&p.interactionTarget,replacement.object);
        // Let the genuine Actor update drain deletion broadcasts while only
        // the captured-reference helper remains suspended. Otherwise queued
        // sender credits correctly forbid the physical reuse we need to test.
        suppress(subject.object,false);
        accepted(items.delete_item(target.object)); tick(); tick(); accepted(items.collect_joined());
        suppress(subject.object,true);
        const auto reused=item(300);
        require(reused.object==target.object,"retired captured target did not reach actual native physical reuse");
        const auto before=placement(subject).transform;
        auto* model=actors.native_model(subject.instance);
        const IsleGameModelPlacement* observed=nullptr;
        native(isle_game_model_placement(services.game,model,&observed));
        native(isle_game_pause_source_processes(services.game,subject.object,actor_movement_probe_entry,0));
        require(isle_game_tick(services.game)==ISLE_GAME_EXECUTION_FAILURE && !movement.returned &&
                actor_interaction_model_alive(model) && same(observed->transform.rotation,before.rotation),
                "captured retired incarnation was silently retargeted to physically reused Item model");
    }
    void movement_reset_null_fault(const actor::SpawnResult& subject) {
        prepare(subject); movement.operation=ACTOR_RESET_PATH;
        native(actor_movement_probe_start(&movement,&source(subject)));
        auto* model=actors.native_model(subject.instance);
        const IsleGameModelPlacement* observed=nullptr;
        native(isle_game_model_placement(services.game,model,&observed));
        require(isle_game_tick(services.game)==ISLE_GAME_EXECUTION_FAILURE && movement.entered==1 &&
                !movement.returned && actor_interaction_model_alive(model) && same(observed->transform.position,{0,0,0}),
                "ResetPathPos accepted initial NULL current descriptor");
    }

    void movement_pursuit_and_node(const actor::SpawnResult& subject) {
        prepare(subject); auto& p=source(subject); p.hSpeed=100;
        const auto target=spawn({0,-9000,100}),replacement=spawn({100,-9000,100});
        reference(subject,&p.interactionTarget,target.object);
        movement_start(subject,ACTOR_RUN_TARGET,1,.1f);
        require(!movement.returned && same(p.targetPos,{0,-9000,100}) &&
                same(placement(subject).transform.position,{0,-9000,3.3f}),
                "original pursuit did not sample Actor AUX and perform first real ON_GROUND step");
        place(target,{100,-9000,100}); reference(subject,&p.interactionTarget,replacement.object);
        tick();
        require(same(p.targetPos,{0,-9000,100}) &&
                same(placement(subject).transform.position,{0,-9000,6.6f}),
                "snapshot pursuit unexpectedly reselected moved/replacement target");
        movement_cancel();
        prepare(subject); p.hSpeed=100; place(target,{0,-9000,100});
        reference(subject,&p.interactionTarget,target.object);
        movement_start(subject,ACTOR_RUN_TARGET,1,.1f,0,0,MOVEMENT_FLAG_UPDATE_TARGET_POS);
        reference(subject,&p.interactionTarget,replacement.object); tick();
        require(!movement.returned && same(p.targetPos,{100,-9000,100}) &&
                placement(subject).transform.position.x>0,
                "updating pursuit failed original reselection branch after canonical target replacement");
        movement_cancel();
        prepare(subject); p.hSpeed=100; place(target,{0,-9000,100});
        reference(subject,&p.interactionTarget,target.object);
        movement_start(subject,ACTOR_RUN_TARGET,100,.1f,0,0,MOVEMENT_FLAG_UPDATE_TARGET_POS);
        require(movement.returned && (p.processFlags&POKEMON_PROCESS_TARGET_REACHED),
                "RunToTarget failed post-step distance arrival flag");
        prepare(subject); p.hSpeed=100; place(target,{0,-9000,-100});
        reference(subject,&p.interactionTarget,target.object);
        movement_start(subject,ACTOR_RUN_AWAY,102,.1f);
        require(movement.returned && (p.processFlags&POKEMON_PROCESS_TARGET_REACHED) &&
                same(placement(subject).transform.position,{0,-9000,3.3f}),
                "RunAwayFromTarget failed real outward step/strict-distance completion");
        prepare(subject); p.hSpeed=100; p.targetPos={0,-9000,100};
        movement_start(subject,ACTOR_RUN_POSITION,.1f);
        require(!movement.returned,"RunToTargetPos did not preserve original per-step wait");
        p.targetPos=placement(subject).transform.position; tick();
        require(movement.returned && (p.processFlags&POKEMON_PROCESS_TARGET_REACHED),
                "waiting RunToTargetPos did not consume current target mutation and terminate");
        prepare(subject);
        const IsleGameModelTransform player_pose{{100,0,100},{0,0,0},{1,1,1}};
        native(isle_game_model_set_placement(services.game,player.model,ISLE_GAME_MODEL_PLACEMENT_RTS,&player_pose));
        native(isle_game_model_reference(services.game,player.object,player.model,&movement.node.model));
        movement.node.node=0;
        movement_start(subject,ACTOR_TURN_NODE,1);
        require(movement.returned && near(placement(subject).transform.rotation.y,.7853981634f),
                "original TurnToModelNode failed actual registered Player world node");
        prepare(subject); p.playerDist=1400; reference(subject,&p.interactionTarget,player.object);
        require(items.flute().play()==audio::Error::none,"actual movement flute start failed");
        movement_start(subject,ACTOR_TURN_FLUTE,.1f);
        require(!movement.returned && near(placement(subject).transform.rotation.y,.1f),
                "flute helper incorrectly stopped at inclusive1400 boundary");
        require(items.flute().stop()==audio::Error::none,"actual movement flute stop failed");
        tick(); require(movement.returned,"waiting flute turn failed actual stop cancellation condition");
    }

    void automatic_structure(const actor::SpawnResult& subject) {
        prepare(subject);
        // NULL exits before execution-policy suspension, unlike an empty graph.
        native(isle_game_pause_source_processes(services.game,subject.object,isle_actor_source_distance_process,0));
        actors.player_source().D_80382D44_523154=1;
        automatic(subject,nullptr);
        require(probe.returned==1 && actor_interaction_paused_count(subject.object)<actor_interaction_process_count(subject.object),
                "NULL automatic graph reached the ordinary pause tail");
        prepare(subject);
        Rows terminal{{58,actor_interaction_probe_entry,0,actor_interaction_probe_entry},{16,hidden,3000,descend}};
        automatic(subject,&terminal); no_selection(subject);
        prepare(subject);
        native(isle_game_pause_source_processes(services.game,subject.object,isle_actor_source_distance_process,0));
        actors.player_source().D_80382D44_523154=1;
        automatic(subject,&terminal);
        require(actor_interaction_paused_count(subject.object)==actor_interaction_process_count(subject.object),
                "nonNULL terminal graph skipped the ordinary automatic pause tail");
        prepare(subject);
        Rows skip{{16,nullptr,3000,nullptr},{16,nullptr,3000,descend}};
        automatic(subject,&skip); selected_aux(subject,player.object);
        require(probe.returned==1 && source(subject).interactionDist==2000,
                "NULL/NULL automatic skip blocked aux-only row or changed BUG16 correction");
        prepare(subject); source(subject).playerDist=10;
        Rows ordered{{16,hidden,20,descend},{16,func_8035DF20_4FE330,20,nullptr}};
        automatic(subject,&ordered);
        IsleGameSourceCallback function=nullptr;
        native(isle_game_process_source_function(services.game,source(subject).stateProc,&function));
        require(function==hidden && probe.returned==0 && !source(subject).tangible &&
                actor_interaction_aux_before_state(subject.object,descend,hidden) && near(root_y(subject),-.2f),
                "automatic first winner, aux6-before-state7 creation, or real nonreturning kind0 state replacement differs");
        prepare(subject); source(subject).playerDist=10;
        actors.player_source().D_80382D44_523154=1;
        Rows paused{{16,nullptr,20,descend}};
        automatic(subject,&paused);
        require(probe.returned==1 && source(subject).interactionTarget==player.object && root_y(subject)==0 &&
                actor_interaction_paused_count(subject.object)==actor_interaction_process_count(subject.object),
                "ordinary automatic tail did not pause its newly created auxiliary and all native workers");
        prepare(subject); source(subject).playerDist=10; actors.player_source().D_80382D44_523154=1;
        Rows kind1{{16,hidden,20,descend}};
        automatic(subject,&kind1,ISLE_GAME_SYNCHRONOUS);
        require(probe.returned==1 && source(subject).tangible &&
                actor_interaction_process_paused(source(subject).stateProc) && root_y(subject)==0,
                "kind1 current-state replacement did not return through the actual pause tail");
        for (auto kind:{ISLE_GAME_STACKFUL,ISLE_GAME_SYNCHRONOUS}) {
            prepare(subject); native(actor_interaction_probe_start(&probe,&source(subject),kind,2,1)); tick();
            require(probe.entered==1 && probe.returned==(kind==ISLE_GAME_SYNCHRONOUS?1:0) && !source(subject).stateProc,
                    "actual NULL-current SetState kind0/kind1 control transfer differs");
        }
    }

    void nearest(const actor::SpawnResult& subject) {
        Rows rows{{15,nullptr,0,descend}};
        prepare(subject); automatic(subject,&rows); no_selection(subject);
        for (float distance:{600.0f,599.0f}) {
            prepare(subject); const auto apple=item(distance); automatic(subject,&rows);
            if (distance==600) no_selection(subject);
            else {
                selected_aux(subject,apple.object);
                require(source(subject).apple==apple.object && source(subject).interactionDist==599 &&
                        services.source_owner_borrows(apple.object).fields==2,
                        "strict-near still apple did not receive both independent canonical holds");
            }
            clear_items();
        }
        prepare(subject); const auto current_target=item(100);
        reference(subject,&source(subject).interactionTarget,current_target.object);
        automatic(subject,&rows); selected_aux(subject,current_target.object);
        require(source(subject).apple==current_target.object && services.source_owner_borrows(current_target.object).fields==2,
                "current interactionTarget incorrectly blocked nearest apple adoption (only current apple blocks)");
        clear_items();
        for (unsigned blocker=0;blocker<3;++blocker) {
            prepare(subject);
            const auto close=item(100,blocker!=0,blocker==1?ISLE_ITEM_FLYING:ISLE_ITEM_STILL);
            const auto far=item(200);
            if (blocker==2) reference(subject,&source(subject).apple,close.object);
            automatic(subject,&rows);
            require(!source(subject).interactionTarget && root_y(subject)==0 &&
                    source(subject).apple==(blocker==2?close.object:nullptr) && source(subject).interactionDist==untouched_distance,
                    "nearest ALL-items winner was filtered into a farther eligible still apple");
            require(!services.source_owner_borrows(far.object).fields,"blocked farther apple gained a reference");
            clear_items();
        }
        prepare(subject); const auto invalid=item(10,true,ISLE_ITEM_INVALID); const auto valid=item(50);
        automatic(subject,&rows); selected_aux(subject,valid.object);
        require(!services.source_owner_borrows(invalid.object).fields,"INVALID item entered nearest valid cohort");
        items.source(invalid.instance)->state=ISLE_ITEM_STILL; // Restore the authored iterator-only input before real deletion.
        clear_items();
        // Fixed natural reuse makes ARRAY order disagree with native link4:
        // seed0 dies, native-first1 remains, then a new tail object reuses0.
        // No array entry/native link is rewritten to manufacture that ordering.
        accepted(items.init()); prepare(subject);
        const auto seed=item(3000),native_first=item(-100);
        remove_seed_item(seed); const auto array_first=item(100);
        require(globals.source.entries[0].obj==array_first.object && globals.source.entries[1].obj==native_first.object &&
                actor_interaction_link_precedes(native_first.object,array_first.object),
                "natural nearest tie fixture lost ARRAY0 versus native-first1 disagreement");
        automatic(subject,&rows); selected_aux(subject,array_first.object); clear_items();
        accepted(items.init()); prepare(subject);
        const auto next_seed=item(3000),tied_apple=item(-100);
        remove_seed_item(next_seed); const auto tied_ball=item(100,false);
        require(globals.source.entries[0].obj==tied_ball.object &&
                actor_interaction_link_precedes(tied_apple.object,tied_ball.object),
                "natural ball/Apple tie fixture did not preserve native-versus-array ordering");
        automatic(subject,&rows); no_selection(subject);
        require(!services.source_owner_borrows(tied_apple.object).fields,
                "strict tie replaced earlier ARRAY ball with equal-distance native-first apple"); clear_items();
        // AUX, not animated root-local coordinates, is the Actor-side origin.
        prepare(subject); place(subject,{1000,0,0}); const auto aux_near=item(1100);
        automatic(subject,&rows); selected_aux(subject,aux_near.object);
        require(source(subject).interactionDist==100,"nearest used Actor root instead of actual AUX origin"); clear_items();
    }

    void source_distance_inputs(const actor::SpawnResult& subject) {
        prepare(subject);
        const IsleGameModelTransform placement{{3,4,12},{0,0,0},{1,1,1}};
        native(isle_game_model_set_placement(services.game,player.model,ISLE_GAME_MODEL_PLACEMENT_RTS,&placement));
        const IsleGameModelTransform root{{91,92,93},{0,0,0},{1,1,1}};
        native(isle_game_model_set_transform(services.game,player.model,0,&root));
        for (bool present:{false,true}) {
            probe.second=present?player.object:nullptr;
            native(actor_interaction_probe_start(&probe,&source(subject),ISLE_GAME_STACKFUL,1,0));
            tick();
            require(probe.entered==1 && probe.returned==1 &&
                    probe.distance==(present?13.0f:std::numeric_limits<float>::max()) &&
                    !source(subject).interactionTarget && !services.source_owner_borrows(player.object).fields,
                    "actual source NULL/live distance used root rather than AUX or acquired an implicit canonical hold");
        }
        const IsleGameModelTransform origin{{0,0,0},{0,0,0},{1,1,1}};
        native(isle_game_model_set_placement(services.game,player.model,ISLE_GAME_MODEL_PLACEMENT_RTS,&origin));
    }

    void cached_player_and_flute(const actor::SpawnResult& subject) {
        const auto history=actors.player_source().PlayerPos;
        actors.player_source().PlayerPos={90000,-80000,70000};
        for (float distance:{200.0f,199.0f}) {
            prepare(subject); source(subject).playerDist=distance;
            Rows rows{{16,nullptr,200,descend}}; automatic(subject,&rows);
            if (distance==200) no_selection(subject);
            else { selected_aux(subject,player.object); require(source(subject).interactionDist==199,"BUG16 did not assign the identical cached playerDist"); }
        }
        // An ineligible15 still leaves nearest's scratch value at50. Native
        // BUG16 correction MUST use cache199, never that earlier local value.
        prepare(subject); const auto ball=item(50,false); source(subject).playerDist=199;
        Rows bug{{15,nullptr,0,descend},{16,nullptr,200,descend}};
        automatic(subject,&bug); selected_aux(subject,player.object);
        require(source(subject).interactionDist==199 && !source(subject).apple && !services.source_owner_borrows(ball.object).fields,
                "BUG16 reused preceding nearest scratch distance"); clear_items();
        require(items.flute().stop()==audio::Error::none,"real Flute stop failed");
        for (int command=5;command<=7;++command) {
            require(items.flute().play()==audio::Error::none,"real Flute song start/cycle failed");
            for (float distance:{1400.0f,1399.0f}) {
                prepare(subject);
                const IsleGameModelTransform placement{{distance,0,0},{0,0,0},{1,1,1}};
                native(isle_game_model_set_placement(services.game,player.model,ISLE_GAME_MODEL_PLACEMENT_RTS,&placement));
                native(isle_game_pause_source_processes(services.game,subject.object,isle_actor_source_distance_process,0));
                Rows rows{{command,nullptr,0,descend}}; automatic(subject,&rows);
                require(source(subject).playerDist==distance,"Flute predicate did not consume the actual priority8 native Player-distance update");
                if (distance==1400) no_selection(subject); else selected_aux(subject,player.object);
            }
            prepare(subject);
            const IsleGameModelTransform close{{1,0,0},{0,0,0},{1,1,1}};
            native(isle_game_model_set_placement(services.game,player.model,ISLE_GAME_MODEL_PLACEMENT_RTS,&close));
            native(isle_game_pause_source_processes(services.game,subject.object,isle_actor_source_distance_process,0));
            Rows mismatch{{command==7?5:command+1,nullptr,0,descend}};
            automatic(subject,&mismatch); no_selection(subject);
        }
        require(items.flute().stop()==audio::Error::none,"real Flute final stop failed");
        prepare(subject);
        native(isle_game_pause_source_processes(services.game,subject.object,isle_actor_source_distance_process,0));
        Rows stopped{{5,nullptr,0,descend},{6,nullptr,0,descend},{7,nullptr,0,descend}};
        automatic(subject,&stopped); no_selection(subject);
        const IsleGameModelTransform origin{{0,0,0},{0,0,0},{1,1,1}};
        native(isle_game_model_set_placement(services.game,player.model,ISLE_GAME_MODEL_PLACEMENT_RTS,&origin));
        actors.player_source().PlayerPos=history;
    }

    void neighbors(const actor::SpawnResult& subject) {
        const auto left=spawn({-25,0,0}),right=spawn({25,0,0}),edge=spawn({0,100,0});
        Rows rows{{17,nullptr,0,descend}};
        prepare(subject); automatic(subject,&rows); no_selection(subject);
        require(same(source(subject).dirToNeighbors,{17,19,23}),"zero-magnitude neighbor cancellation overwrote canonical direction");
        place(right,{0,0,-25});
        prepare(subject); automatic(subject,&rows); selected_aux(subject);
        const float diagonal=25.0f/std::sqrt(1250.0f);
        require(same(source(subject).dirToNeighbors,{diagonal,0,diagonal}),
                "neighbor direction used collision/root positions, wrong sign, normalization, or included distance100");
        place(left,{0,100,0}); place(right,{0,-100,0}); place(edge,{100,0,0});
        prepare(subject); automatic(subject,&rows); no_selection(subject);
        // All three nearby terms are visited in actual native link3 insertion
        // order. Their grouped float sum retains tiny X only in that order.
        place(left,{-64,0,0}); place(right,{64,0,0}); place(edge,{-0.000001f,0,0});
        prepare(subject); automatic(subject,&rows); selected_aux(subject);
        require(source(subject).dirToNeighbors.x==1 && source(subject).dirToNeighbors.y==0 && source(subject).dirToNeighbors.z==0,
                "source-ordered native neighbor accumulation was reordered or replaced by an unordered reduction");
        place(left,{2000,0,0}); place(right,{3000,0,0}); place(edge,{4000,0,0});
    }

    void publish_focus(IsleGameObject* object,bool present=true) {
        // Deterministic API detector input, NOT a GPU capture or claimed fence.
        const auto identity=reinterpret_cast<focus::Object>(object);
        const std::array focus_subjects{focus::Subject{0,-1,0},focus::Subject{identity,1003,4}};
        focus::Photo photo; photo.objects[0]=identity;
        std::array<std::uint32_t,80> words{};
        std::fill(words.begin()+48,words.begin()+80,0xffffffffu);
        require(detector.stage(0,focus_subjects,photo)==focus::Error::none &&
                detector.analyze(0,words,64)==focus::Error::none,"actual Detector fixture analysis failed");
        if (!present) detector.cleanup(identity);
        detector.publish();
    }
    void focus_and_direction(const actor::SpawnResult& subject) {
        for (const auto& [timer,value,hit]:std::array<std::tuple<std::uint32_t,float,bool>,6>{{
                {20,20.0f,false},{21,20.0f,true},
                {0x80000000u,2147483520.0f,true},{0x80000000u,2147483648.0f,false},
                {0xffffffffu,4294967296.0f,false},{0xffffffffu,4294967040.0f,true}}}) {
            prepare(subject); publish_focus(subject.object);
            actors.player_source().sTimerInFocus=timer;
            Rows rows{{23,nullptr,value,descend}}; automatic(subject,&rows);
            if (hit) selected_aux(subject); else no_selection(subject);
        }
        prepare(subject); publish_focus(player.object);
        actors.player_source().gPreviousPokemonInFocus=subject.object;
        actors.player_source().D_80382C3C_52304C=1; actors.player_source().sTimerInFocus=100;
        Rows rows{{23,nullptr,1,descend}}; automatic(subject,&rows); no_selection(subject);
        prepare(subject); publish_focus(subject.object,false);
        actors.player_source().sTimerInFocus=100; automatic(subject,&rows); selected_aux(subject);
        require(!detector.published().present && detector.published().object==reinterpret_cast<focus::Object>(subject.object),
                "focus fixture failed to retain current pointer with cleared present flag");
        for (auto direction:{-2,-1,0,1}) for (float distance:{100.0f,99.0f}) {
            prepare(subject); actors.player_source().gDirectionIndex=direction; source(subject).playerDist=distance;
            Rows negative{{25,nullptr,100,descend}}; automatic(subject,&negative);
            if (direction<0 && distance<100) selected_aux(subject); else no_selection(subject);
        }
    }

    void queued_nullable_and_ranges(const actor::SpawnResult& subject) {
        constexpr std::array nullable{8,9,12,13,24,26,27};
        for (int command:nullable) for (bool callback:{false,true}) for (bool live:{false,true}) {
            prepare(subject); reference(subject,&source(subject).interactionTarget,player.object);
            Rows rows{{command,nullptr,0,callback?descend:nullptr},{command,hidden,0,descend}};
            enqueue(subject,command,live?player.object:nullptr);
            require(services.source_owner_borrows(player.object).queued==(live?1u:0u),"successful enqueue did not hold its real sender credit");
            fifo(subject,&rows);
            require(source(subject).interactionTarget==(live?player.object:nullptr) && !source(subject).stateProc &&
                    source(subject).tangible && services.source_owner_borrows(player.object).queued==0,
                    "queued nullable first match was skipped or left a sender credit");
            if (callback) selected_aux(subject,live?player.object:nullptr);
            else require(root_y(subject)==0 && actor_interaction_function_count(subject.object,descend,6)==0,
                         "queued first NULL/NULL match incorrectly fell through to duplicate callback");
        }
        for (int command=28;command<=57;++command) for (bool live:{false,true}) {
            prepare(subject); reference(subject,&source(subject).interactionTarget,subject.object);
            Rows rows{{command,nullptr,0,descend}};
            enqueue(subject,command,live?player.object:nullptr); fifo(subject,&rows);
            selected_aux(subject,live?player.object:subject.object);
            require(services.source_owner_borrows(player.object).queued==0,
                    "28..57 FIFO did not settle its consumed sender credit");
        }
        for (int command:{14,10,20}) {
            const float threshold=command==10?150.0f:600.0f;
            for (bool callback:{false,true}) for (float distance:{threshold,threshold-1}) {
                prepare(subject);
                const IsleGameModelTransform placement{{distance,0,0},{0,0,0},{1,1,1}};
                native(isle_game_model_set_placement(services.game,player.model,ISLE_GAME_MODEL_PLACEMENT_RTS,&placement));
                Rows rows{{command,nullptr,0,callback?descend:nullptr},{command,hidden,0,descend}};
                enqueue(subject,-4,player.object); enqueue(subject,command,player.object);
                enqueue(subject,-5,player.object); fifo(subject,&rows);
                IsleGameObjectMetadata metadata{};
                native(isle_game_object_metadata(services.game,subject.object,&metadata));
                require(!(metadata.flags&1) && services.source_owner_borrows(player.object).queued==0,
                        "radius rejection/success did not consume its node and execute hide-then-show FIFO suffix");
                if (distance==threshold) {
                    no_selection(subject);
                } else {
                    require(source(subject).interactionTarget==player.object && source(subject).apple==(command==20?nullptr:player.object) &&
                            source(subject).interactionDist==(command==20?untouched_distance:distance),
                            "queued sender distance or target-only20/dual-reference14/10 policy differs");
                    if (callback) selected_aux(subject,player.object);
                    else require(root_y(subject)==0 && actor_interaction_function_count(subject.object,descend,6)==0,
                                 "queued radius first NULL/NULL row fell through to duplicate callback");
                }
            }
            prepare(subject); Rows null_sender{{command,nullptr,0,descend}};
            enqueue(subject,command); fifo(subject,&null_sender); no_selection(subject);
        }
        const IsleGameModelTransform origin{{0,0,0},{0,0,0},{1,1,1}};
        native(isle_game_model_set_placement(services.game,player.model,ISLE_GAME_MODEL_PLACEMENT_RTS,&origin));
        for (float distance:{400.0f,399.0f}) for (bool live:{false,true}) {
            prepare(subject); source(subject).playerDist=distance;
            Rows rows{{18,nullptr,0,descend}}; enqueue(subject,18,live?player.object:nullptr); fifo(subject,&rows);
            if (distance==400) no_selection(subject); else selected_aux(subject,live?player.object:nullptr);
        }
        for (int command:{5,6,7,11,15,16,17,23,25,58}) {
            prepare(subject); source(subject).playerDist=1;
            Rows rows{{command,hidden,3000,descend}};
            enqueue(subject,command,player.object); fifo(subject,&rows); no_selection(subject);
            require(services.source_owner_borrows(player.object).queued==0,
                    "automatic-only/unused queued command did not consume its FIFO node without selection");
        }
    }

    void queued_clear_world_and_tail(const actor::SpawnResult& subject) {
        prepare(subject); auto& p=source(subject);
        reference(subject,&p.apple,player.object); reference(subject,&p.interactionTarget,player.object);
        enqueue(subject,21,subject.object); enqueue(subject,22,subject.object); fifo(subject,nullptr);
        require(p.apple==player.object && p.interactionTarget==player.object,"nonmatching21/22 cleared canonical references");
        enqueue(subject,21,player.object); fifo(subject,nullptr);
        require(!p.apple && p.interactionTarget==player.object && services.source_owner_borrows(player.object).fields==1,
                "21 did not clear apple before NULL-graph early exit or cleared target prematurely");
        enqueue(subject,22,player.object); fifo(subject,nullptr);
        require(!p.interactionTarget && services.source_owner_borrows(player.object).fields==0,
                "22 did not close actual canonical target hold before NULL-graph exit");
        for (int command:{21,22}) {
            prepare(subject);
            reference(subject,&p.apple,player.object); reference(subject,&p.interactionTarget,player.object);
            Rows rows{{command,nullptr,0,descend}};
            enqueue(subject,command,player.object); fifo(subject,&rows);
            selected_aux(subject,command==21?player.object:nullptr);
            require(p.apple==(command==21?nullptr:player.object) &&
                    services.source_owner_borrows(player.object).fields==1 &&
                    services.source_owner_borrows(player.object).queued==0,
                    "matching nonNULL-graph21/22 did not clear only its field before actual auxiliary selection");
        }
        prepare(subject); p.baseBlock=nullptr;
        Rows last_null{{19,hidden,0,descend},{19,nullptr,0,nullptr}};
        enqueue(subject,19); fifo(subject,&last_null);
        require(!p.stateProc && root_y(subject)==0,"19 was first-match rather than last-match including NULL/NULL");
        Rows last_aux{{19,nullptr,0,nullptr},{19,nullptr,0,descend}};
        enqueue(subject,19); fifo(subject,&last_aux); selected_aux(subject);
        prepare(subject); p.baseBlock=&world.blocks()[0];
        enqueue(subject,19); fifo(subject,&last_aux); no_selection(subject);
        require(world.current()==nullptr,"nullable currentWorld precondition changed");
        auto* entered=world.enter_first(0); require(entered && world.error().empty(),"real World first-block publication failed");
        prepare(subject); p.baseBlock=entered;
        enqueue(subject,19); fifo(subject,&last_aux); selected_aux(subject);
        prepare(subject); p.baseBlock=entered->next;
        enqueue(subject,19); fifo(subject,&last_aux); no_selection(subject);
        prepare(subject); p.stateProc=nullptr; p.counter=2;
        reference(subject,&p.apple,player.object);
        Rows transfer{{8,hidden,0,descend}};
        enqueue(subject,8,player.object); enqueue(subject,-4,player.object);
        enqueue(subject,21,player.object); enqueue(subject,22,player.object);
        fifo(subject,&transfer);
        IsleGameObjectMetadata metadata{}; native(isle_game_object_metadata(services.game,subject.object,&metadata));
        require(p.counter==1 && !p.apple && !p.interactionTarget && (metadata.flags&1) &&
                actor_interaction_aux_before_state(subject.object,descend,hidden) &&
                services.source_owner_borrows(player.object).queued==0 && !p.tangible && near(root_y(subject),-.2f),
                "NULL-current SetState from native fnUpdate did not continue ordinary tail and remaining FIFO");
        prepare(subject); actors.player_source().D_80382D44_523154=1;
        Rows ordinary{{8,hidden,0,descend}}; enqueue(subject,8,player.object); fifo(subject,&ordinary);
        require(source(subject).tangible && root_y(subject)==0 &&
                actor_interaction_paused_count(subject.object)==actor_interaction_process_count(subject.object),
                "queued ordinary tail did not pause aux6 then new state7 before either ran");
        prepare(subject);
        native(isle_game_pause_source_processes(services.game,subject.object,isle_actor_source_distance_process,0));
        actors.player_source().D_80382D44_523154=1;
        enqueue(subject,58,player.object); fifo(subject,nullptr);
        require(actor_interaction_paused_count(subject.object)==actor_interaction_process_count(subject.object),
                "ordinary queued NULL graph skipped its pause tail like automatic NULL graph");
    }

    void builtins(const actor::SpawnResult& subject) {
        prepare(subject);
        // Invalid callbacks must never be admitted for independent builtins.
        Rows invalid{{8,actor_interaction_probe_entry,0,nullptr}};
        enqueue(subject,-4,player.object); fifo(subject,&invalid);
        IsleGameObjectMetadata metadata{}; native(isle_game_object_metadata(services.game,subject.object,&metadata));
        require(metadata.flags&1,"real FIFO -4 failed to hide");
        enqueue(subject,-5,player.object); fifo(subject,&invalid);
        native(isle_game_object_metadata(services.game,subject.object,&metadata)); require(!(metadata.flags&1),"real FIFO -5 failed to show");
        enqueue(subject,-3,player.object); fifo(subject,&invalid);
        require(actor_interaction_paused_count(subject.object)==0,"real FIFO -3 failed to resume every native process");
        enqueue(subject,-2,player.object); fifo(subject,&invalid);
        require(actor_interaction_paused_count(subject.object)==actor_interaction_process_count(subject.object),"real FIFO -2 failed to suspend every native process");
        const auto victim=spawn();
        source(victim).transitionGraph=&invalid.graph;
        enqueue(victim,-1,player.object); enqueue(victim,8,player.object); enqueue(victim,-5,player.object);
        suppress(victim.object,false); tick();
        require(actors.membership(victim.instance).joined && !actors.registered(victim.instance) &&
                services.source_owner_borrows(player.object).queued==0,
                "-1 did not bypass invalid custom tail, join receiver, and cancel remaining sender credits");
        accepted(actors.collect_joined());
    }

    void sender_lifetime(const actor::SpawnResult& subject) {
        prepare(subject); const auto held=item(100); auto* semantic=items.source(held.instance);
        auto& p=source(subject);
        reference(subject,&p.apple,held.object); reference(subject,&p.interactionTarget,held.object);
        require(services.source_reference_assign(player.object,&player.held_target,held.object),"Player fixture could not own an actual canonical Item field borrow");
        enqueue(subject,20,held.object);
        require(services.source_owner_borrows(held.object).fields==3 && services.source_owner_borrows(held.object).queued==1,
                "canonical fields and queued sender were not independent Item holds");
        accepted(items.delete_item(held.object));
        for (const auto& other:subjects)
            if (other.instance!=subject.instance && actors.source(other.instance)) suppress(other.object,false);
        // The subject's genuine flag40 suppression preserves20/21/22 positions;
        // unrelated paused receivers may drain their own broadcast21 credits.
        tick(); tick();
        require(items.membership(held.instance).joined && services.source_owner_joined(held.object) &&
                semantic->state==ISLE_ITEM_INVALID && !items.source(held.instance) &&
                p.apple==held.object && p.interactionTarget==held.object,
                "native join erased held canonical fields or failed logical Item removal");
        for (const auto& other:subjects)
            if (other.instance!=subject.instance && actors.source(other.instance)) suppress(other.object,true);
        const auto blocked=items.collect_joined();
        require(!blocked && blocked.error==it::OwnerError::lifetime && !services.source_owner_release(held.object),
                "real native join was mistaken for permission to collect a borrowed sender");
        const auto replacement=item(200);
        require(replacement.object!=held.object && items.source(replacement.instance)!=semantic,
                "held joined native/semantic address was reused by another Item");
        // Actual C source distance under a native process returns FLT_MAX for
        // this second, Services-proven held joined sender, never dereferences it.
        probe.second=held.object;
        native(actor_interaction_probe_start(&probe,&p,ISLE_GAME_STACKFUL,1,0)); tick();
        require(probe.entered==1 && probe.returned==1 && probe.distance==std::numeric_limits<float>::max(),
                "actual source distance rejected or dereferenced a registered joined held second sender");
        Rows radius{{20,nullptr,0,descend}}; fifo(subject,&radius);
        require(!p.apple && !p.interactionTarget && root_y(subject)==0 &&
                services.source_owner_borrows(held.object).queued==0 && services.source_owner_borrows(held.object).fields==1,
                "joined sender radius or actual FIFO20-before21/22 prefix consumption differs");
        require(services.source_reference_assign(player.object,&player.held_target,nullptr),"Player canonical Item hold release failed");
        accepted(items.collect_joined());
        require(!services.source_owner_registered(held.object) && !items.source(held.instance),
                "joined unborrowed Item failed to unregister/release its canonical payload");
        // Reuse now really is permitted, while replacement remains allocated:
        // the next source allocation reclaims the just-released held identity.
        const auto reused=item(300);
        require(reused.object==held.object && items.source(reused.instance)==semantic,
                "released native identity/source payload did not return to actual free-list reuse");
        clear_items();
    }

    void constructor_rejections() {
        // No graph/list callback word is mapped to a convenient common state.
        // Constructor must reject the real unresolved callable closure, even
        // when its requested root would not currently win any selector row.
        bool graph_rejected=false,weighted_rejected=false;
        for (const auto& graph:behavior.graphs()) {
            const std::array root{graph.wire.guest_address};
            try { (void)make_bindings(behavior,root); }
            catch (const std::invalid_argument& error) {
                graph_rejected=std::string_view(error.what()).find("Unresolved native callback")!=std::string_view::npos;
            }
            if (graph_rejected) break;
        }
        for (const auto& list:behavior.weighted_states()) {
            const std::array root{list.wire.guest_address};
            try { (void)make_bindings(behavior,{},root); }
            catch (const std::invalid_argument& error) {
                weighted_rejected=std::string_view(error.what()).find("Unresolved native callback")!=std::string_view::npos;
            }
            if (weighted_rejected) break;
        }
        require(graph_rejected && weighted_rejected,"uncompiled actual graph/weighted callable closure was silently accepted");
    }

    // Every expected fault uses a fresh actual game/context. Native fault state
    // is intentionally sticky; no reset/retry pretends that the rejected FIFO
    // can run again. Credits/prefix/payload ownership are inspected directly.
    void automatic_fault(unsigned kind) {
        const auto subject=spawn({0,0,0},kind==5?129:1003); auto& p=source(subject); p.playerDist=1;
        Rows rows{{16,nullptr,10,descend},{25,actor_interaction_probe_entry,0,nullptr}};
        if (kind==1 || kind==5) rows.rows[1]={25,func_beach_802CAB50,0,nullptr};
        if (kind==2) rows.graph.count=1; // Missing terminal, not an out-of-bounds read.
        if (kind==3) {
            require(environment.set_listener(nullptr,nullptr),"environment listener release failed");
            native(isle_game_remove_model(services.game,player.model));
            player.model=nullptr; actors.player_source().model=nullptr;
            rows.rows[1]={58,nullptr,0,nullptr};
        }
        if (kind==4) {
            require(environment.set_listener(nullptr,nullptr),"environment listener release failed");
            native(isle_game_remove_camera(services.game,actors.player_source().camera));
            actors.player_source().camera=nullptr;
            rows.rows[1]={58,nullptr,0,nullptr};
        }
        p.transitionGraph=&rows.graph;
        if (kind==5) require(world.current()==nullptr && p.id==129,
                             "compatible Magikarp closure fixture requires actual nullable current World");
        const auto& retained_root=borrow_root(subject);
        native(actor_interaction_probe_start(&probe,&p,ISLE_GAME_STACKFUL,0,1));
        const auto result=isle_game_tick(services.game);
        require(result==ISLE_GAME_EXECUTION_FAILURE && probe.entered==1 && !probe.returned && !probe.failures &&
                !p.apple && !p.interactionTarget && p.interactionDist==untouched_distance && retained_root.transform.position.y==0 &&
                actor_interaction_function_count(subject.object,descend,6)==0,
                "automatic whole-graph/callback/resource/Player admission did not fault before earlier winning row effects");
        p.transitionGraph=nullptr;
    }
    void rejected_prefix(unsigned kind) {
        const auto subject=spawn({0,0,0},kind==3?129:1003); auto& p=source(subject); p.counter=2;
        Rows invalid{{8,nullptr,0,nullptr},{25,actor_interaction_probe_entry,0,nullptr}};
        if (kind==1 || kind==3) invalid.rows[1]={25,func_beach_802CAB50,0,nullptr};
        if (kind==2) {
            require(environment.set_listener(nullptr,nullptr),"environment listener release failed");
            native(isle_game_remove_model(services.game,player.model));
            player.model=nullptr; actors.player_source().model=nullptr;
            invalid.rows[1]={16,nullptr,10,descend};
        }
        if (kind==3) require(world.current()==nullptr && p.id==129,
                             "compatible Magikarp FIFO closure fixture requires actual nullable current World");
        p.transitionGraph=&invalid.graph;
        for (int command:{-4,-2,8,-5,-3}) enqueue(subject,command,player.object);
        suppress(subject.object,false);
        require(isle_game_tick(services.game)==ISLE_GAME_EXECUTION_FAILURE && services.failed(),
                "actual native FIFO did not fence failed whole-graph admission");
        IsleGameObjectMetadata metadata{}; native(isle_game_object_metadata(services.game,subject.object,&metadata));
        require(p.counter==1 && !p.interactionTarget && (metadata.flags&1) &&
                actor_interaction_paused_count(subject.object)==actor_interaction_process_count(subject.object) &&
                services.source_owner_borrows(player.object).queued==3,
                "failed native drain replayed consumed builtins, consumed rejected node/suffix, or leaked prefix credits");
        require(isle_game_tick(services.game)==ISLE_GAME_EXECUTION_FAILURE && p.counter==1 &&
                services.source_owner_borrows(player.object).queued==3,
                "sticky native failure replayed consumed prefix or resumed rejected-node suffix");
        p.transitionGraph=nullptr;
    }
    void callback_fault_prefix() {
        const auto subject=spawn(); auto& p=source(subject);
        extra_sources.push_back(std::make_unique<ApiSourceOwner>(services,50,true));
        auto* invalid_sender=extra_sources.back()->object;
        Rows rows{{8,nullptr,0,nullptr},{20,nullptr,0,descend},{22,nullptr,0,nullptr}};
        p.transitionGraph=&rows.graph;
        enqueue(subject,8,player.object); enqueue(subject,20,invalid_sender); enqueue(subject,22,player.object);
        const auto& retained_root=borrow_root(subject);
        suppress(subject.object,false);
        require(isle_game_tick(services.game)==ISLE_GAME_EXECUTION_FAILURE && isle_game_status(services.game)==ISLE_GAME_EXECUTION_FAILURE,
                "actual C sender-distance model error did not reach native fault fence");
        require(p.interactionTarget==player.object && retained_root.transform.position.y==0 &&
                services.source_owner_borrows(player.object).queued==1 && services.source_owner_borrows(player.object).fields==1 &&
                services.source_owner_borrows(invalid_sender).queued==0,
                "current C-fault credit or consumed prefix was retained/replayed, or unvisited suffix was discarded");
        p.transitionGraph=nullptr;
    }
    void joined_first_fault() {
        const auto subject=spawn(); auto& p=source(subject);
        extra_sources.push_back(std::make_unique<ApiSourceOwner>(services,51));
        auto& joined=*extra_sources.back();
        reference(subject,&p.interactionTarget,joined.object);
        services.erase(joined.object);
        require(services.source_owner_joined(joined.object),"joined-distance sender precondition did not really join");
        // Actual public model removal, not forged flags/payloads. A joined SECOND
        // object is no excuse to bypass validation of the FIRST Actor's model.
        native(isle_game_remove_model(services.game,actors.native_model(subject.instance)));
        probe.second=joined.object;
        native(actor_interaction_probe_start(&probe,&p,ISLE_GAME_STACKFUL,1,0));
        require(isle_game_tick(services.game)==ISLE_GAME_EXECUTION_FAILURE && probe.entered==1 && !probe.returned &&
                probe.distance==-1 && p.interactionTarget==joined.object,
                "joined-second FLT_MAX exception rescued an invalid first model or published a partial distance");
    }
    bool shutdown() noexcept {
        if (shut_down) return true;
        bool good=true;
        const bool failed_scene=services.failed() || isle_game_status(services.game)!=ISLE_GAME_OK;
        // End every graph borrow and close real references before retiring any
        // held Item/Player. No owning C++ frame is inside a source transfer here.
        for (const auto& subject:subjects) if (auto* p=actors.source(subject.instance)) {
            p->transitionGraph=nullptr;
            good=services.source_reference_assign(subject.object,&p->apple,nullptr)&&good;
            good=services.source_reference_assign(subject.object,&p->interactionTarget,nullptr)&&good;
        }
        if (player.held_target) good=services.source_reference_assign(player.object,&player.held_target,nullptr)&&good;
        for (const auto& subject:subjects) if (actors.source(subject.instance)) services.erase(subject.object);
        good=bool(actors.collect_joined())&&good;
        const auto reset=actors.reset();
        good=(bool(reset)||(failed_scene&&reset.error==actor::OwnerError::services))&&good;
        const auto retired=items.retire();
        good=(bool(retired)||(failed_scene&&retired.error==it::OwnerError::services))&&good;
        good=bool(items.collect_joined())&&good;
        good=environment.set_listener(nullptr,nullptr)&&good;
        for (auto& sender:extra_sources) good=sender->close()&&good;
        shut_down=good;
        return good;
    }
    ~Consumer() { if (!shut_down && !shutdown()) std::terminate(); }
};

struct Resources {
    world::Resources world;
    beach::ActorResources actors;
    it::Resources items;
    beach::BehaviorResources behavior;
    environment_sound::BeachResources sound;
    menu::EffectResources effects;
    std::array<const menu::EffectBank*,2> banks;
    std::array<a::SlotCapacity,8> actor_capacities{};
    std::array<a::SlotCapacity,32> item_capacities{};
    std::unique_ptr<actor::BehaviorBindings> bindings;
    explicit Resources(const rom::Snapshot& snapshot)
        : world(snapshot),actors(snapshot,{world.setup().fog_rgb,world.setup().fog_min,world.setup().fog_max}),
          items(snapshot,world.setup().fog_rgb,world.setup().fog_min,world.setup().fog_max),
          behavior(snapshot),sound(snapshot),effects(snapshot),
          banks{&effects.bank(menu::EffectGroup::common),&effects.bank(menu::EffectGroup::beach)},
          bindings(make_bindings(behavior)) {
        a::SlotCapacity ac{},ic{};
        for (const auto& model:actors.models()) { const auto c=a::slot_capacity(model); ac.nodes=std::max(ac.nodes,c.nodes); ac.material_attachments=std::max(ac.material_attachments,c.material_attachments); }
        for (const auto& model:items.models()) { const auto c=a::slot_capacity(model); ic.nodes=std::max(ic.nodes,c.nodes); ic.material_attachments=std::max(ic.material_attachments,c.material_attachments); }
        // Fixed predeclared physical capacities, independent of execution and
        // distinct from Items' actual twenty-entry logical array capacity.
        actor_capacities.fill(ac); item_capacities.fill(ic);
    }
};

template<class Body>
void with_graph(Resources& resources,game_audio::Session& audio,game::State& state,Body body,
                std::span<const menu::AnimationPath> semantic_paths={}) {
    game_input::Service input; scene::Storage storage(32*1024*1024); native(storage.create());
    {
        scene::Services services(storage.game(),storage.arena(),{{4,6,0,1,2},{}},input,audio,state);
        private_player::Source player_source(services);
        auto* camera_object=services.object(1,0); require(camera_object,"actual selector camera object absent");
        IsleGameCamera* camera=nullptr; native(isle_game_attach_camera(services.game,camera_object,&camera));
        const IsleGameLookAt look{{0,0,0},{1,0,0},{0,1,0}};
        native(isle_game_camera_set_look_at(services.game,camera,&look));
        ApiSourceOwner player(services,2);
        player_source.bind(player.object,player.model,camera);
        player_source.value.cam_target={100,900,300};
        environment_sound::Environment environment(services,storage.arena(),resources.sound);
        require(environment.initialize(nullptr,camera),"environment source initialization failed");
        focus::Detector detector; Shift shift{&services,camera};
        shift.native_world_pool.arena=&storage.arena();
        std::unique_ptr<world::World> admitted_world;
        if (semantic_paths.empty())
            admitted_world=std::make_unique<world::World>(services,resources.world,
                world::Callbacks{&shift,nullptr,nullptr,nullptr,Shift::camera_shift,Shift::effects_shift,nullptr});
        else {
            // CPU-only bounded native semantic path resources. All actual World
            // models, materials, terrain and services remain the existing ones.
            const auto& r=resources.world;
            world::ResourceView view{&r.setup(),&r.sky(),r.blocks(),r.primary_count(),r.models(),
                r.materials(),r.animations(),semantic_paths,r.static_definitions(),&r.height_map(),r.collision_models()};
            admitted_world=std::make_unique<world::World>(services.game,view,
                world::Callbacks{&shift,nullptr,nullptr,nullptr,Shift::camera_shift,Shift::effects_shift,Shift::commands});
        }
        auto& world=*admitted_world;
        {
            actor::Owner actors(services,resources.actors,world,environment,detector,player_source.value,resources.actor_capacities,resources.banks,{32768,4096});
            accepted(actors.publish_effects());
            shift.actors=&actors;
            require(environment.set_listener(player.model,camera),"environment listener binding failed");
            it::Globals globals;
            {
                it::Owner items(services,resources.items,world,actors,globals,player_source.value,resources.item_capacities,{32768,4096});
                shift.items=&items; accepted(items.init()); accepted(items.set_custom_functions(nullptr,0,nullptr));
                actor::InteractionServices interactions(services,actors,items,world,detector,*resources.bindings,resources.actors);
                accepted(actors.bind_interactions(interactions)); // BEFORE any Actor factory.
                Consumer consumer{services,actors,environment,items,globals,world,detector,player,resources.actors,resources.behavior};
                body(consumer);
                require(consumer.shutdown(),"actual native/semantic fault cleanup left borrowers or failed real joins");
                shift.items=nullptr;
            }
            shift.actors=nullptr;
        }
        require(player.close() && !services.source_owner_registered(player.object) &&
                player.retiring==1 && player.joined==1 && !player.errors,
                "Player API semantic owner did not join/unregister exactly once after all dependent owners retired");
        require(world.retire(),"semantic World retirement failed"); environment.cleanup();
    }
    native(storage.destroy());
}

void run(rom::Snapshot& snapshot,const std::filesystem::path& profile) {
    require(std::filesystem::is_directory(profile)&&std::filesystem::is_empty(profile),"private profile must exist and be empty");
    Resources resources(snapshot);
    auto hydrated=game_audio::hydrate(snapshot); require(bool(hydrated),"actual selector audio hydration failed");
    std::array<audio::effects::Section,8> sections; audio::effects::Config effects;
    require(game_audio::native_startup_effects(hydrated.resources,sections,effects)==audio::Error::none,"actual selector audio effects admission failed");
    private_audio::Capture capture; private_audio::Execution execution{snapshot,nullptr,{}};
    capture.samples.resize(8192*audio::AudioService::max_frames*2);
    auto made=game_audio::Session::create(std::move(hydrated.resources),effects,capture.output(),execution.executor());
    require(bool(made),"actual selector audio Session creation failed"); auto& session=*made.session;
    {
        game::State state(profile); require(state.initialize(1),"selector profile initialization failed");
        with_graph(resources,session,state,[](Consumer& c) {
            c.constructor_rejections();
            const auto subject=c.spawn();
            c.automatic_structure(subject); c.nearest(subject); c.cached_player_and_flute(subject);
            c.source_distance_inputs(subject); c.neighbors(subject); c.focus_and_direction(subject); c.queued_nullable_and_ranges(subject);
            c.queued_clear_world_and_tail(subject); c.builtins(subject); c.sender_lifetime(subject);
        });
        for (unsigned fault=0;fault<6;++fault)
            with_graph(resources,session,state,[fault](Consumer& c) { c.automatic_fault(fault); });
        for (unsigned fault=0;fault<4;++fault)
            with_graph(resources,session,state,[fault](Consumer& c) { c.rejected_prefix(fault); });
        with_graph(resources,session,state,[](Consumer& c) { c.callback_fault_prefix(); });
        with_graph(resources,session,state,[](Consumer& c) { c.joined_first_fault(); });
        with_graph(resources,session,state,[](Consumer& c) { c.movement_heading_and_circles(c.spawn()); });
        with_graph(resources,session,state,[](Consumer& c) { c.movement_vertical(c.spawn()); });
        with_graph(resources,session,state,[](Consumer& c) { c.movement_targets(c.spawn()); });
        with_graph(resources,session,state,[](Consumer& c) { c.movement_pursuit_and_node(c.spawn()); });
        with_graph(resources,session,state,[](Consumer& c) { c.movement_ground_and_null(c.spawn()); });
        with_graph(resources,session,state,[](Consumer& c) { c.movement_retired_target_fault(c.spawn()); });
        with_graph(resources,session,state,[](Consumer& c) { c.movement_reset_null_fault(c.spawn()); });
        // These four backed native semantic descriptors are deliberately NOT
        // called actual ROM trajectories. They exercise C dispatch/control and
        // native identity admission, not another interpolation-solver test.
        std::vector<menu::AnimationPath> paths(resources.world.paths().begin(),resources.world.paths().end());
        for (int kind=0;kind<4;++kind) {
            menu::AnimationPath p; p.kind=kind; p.duration=kind==1?8:4; p.tension=.5f;
            p.segment_point_count=2; p.timestamps={0,1}; p.speed_polynomials={0,0,0,0,16};
            if (kind==0) p.points={{1,2,3},{5,6,7}};
            else if (kind==1) p.points={{5,6,7},{5,6,7},{5,6,7},{5,6,7}};
            else p.points={{-3,-2,-1},{1,2,3},{5,6,7},{9,10,11}};
            paths.push_back(std::move(p));
        }
        with_graph(resources,session,state,[](Consumer& c) { c.movement_paths(c.spawn()); },paths);
        with_graph(resources,session,state,[](Consumer& c) { c.movement_path_fault(c.spawn()); },paths);
    }
    unsigned audio_tasks=0;
    require(session.retrace(1,1000000000ull/60)==audio::Error::none && session.submit()==audio::Error::none,
            "actual selector initial audio frame admission failed");
    audio::TaskView first_task;
    require(session.build(first_task)==audio::Error::none && first_task.token.serial,
            "actual selector audio did not produce its initial native task");
    private_audio::execute(session,execution,first_task); ++audio_tasks;
    require(session.submit()==audio::Error::none,"actual selector initial owned PCM submission failed");
    require(session.stop()==audio::Error::none,"actual selector audio stop failed");
    for (std::uint64_t n=2;n<=241 && session.state()!=game_audio::Session::State::draining;++n) {
        require(session.retrace(n,n*1000000000ull/60)==audio::Error::none && session.submit()==audio::Error::none,"actual selector audio retrace/PCM admission failed");
        audio::TaskView task; require(session.build(task)==audio::Error::none,"actual selector audio task admission failed");
        if (task.token.serial) { private_audio::execute(session,execution,task); ++audio_tasks; require(session.submit()==audio::Error::none,"actual selector PCM submission failed"); }
    }
    require(session.state()==game_audio::Session::State::draining && session.poll_drain()==audio::Error::none &&
            session.poll_drain()==audio::Error::none && capture.finishes==1 && capture.used>0 && audio_tasks>0,
            "actual selector audio client retirement/actual RSP owned PCM drain failed");
    std::cout<<"native_Actor_actual_C_automatic_FIFO_CPU_API=passed native_original_C_movement_CPU_diagnostics=passed bounded_semantic_four_pathkinds_not_ROM_trajectories=true audio_RSP_owned_PCM_drain=passed PlayerFSM_all71graphs_GPU_Original_fullgame_Windows11=not_claimed\n";
}
} // namespace
int main(int argc,char** argv) {
    try {
        require(argc==3,"usage: actor-interaction-rom-smoke PRIVATE_ROM EXISTING_EMPTY_PROFILE");
        auto snapshot=rom::read_snapshot(argv[1]); run(snapshot,argv[2]); return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr<<"actor-interaction-rom-smoke: "<<error.what()<<'\n'; return EXIT_FAILURE;
    }
}
