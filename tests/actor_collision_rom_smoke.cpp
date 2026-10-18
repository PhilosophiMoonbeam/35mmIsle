#include "beach_actor_scene.hpp"
#include "actual_actor_world_hooks.hpp"
// Opt-in actual-ROM native collision API consumer. Authored collision inputs on
// real Magikarp factory actors do not admit the separate nonNULL-graph state.
#include "actor_photo_scene.hpp"
#include "actual_audio_driver.hpp"
#include "actual_player_source.hpp"
#include "effect_bank_resources.hpp"
#include "scene_storage.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

namespace {
using namespace isle;
namespace a=menu::animation;
void require(bool value,const char* detail) { if(!value) throw std::runtime_error(detail); }
void native(IsleGameResult result) { require(result==ISLE_GAME_OK,"native collision consumer operation failed"); }
void accepted(actor::OwnerStatus result) { require(bool(result),result.detail); }
bool close(float x,float y) { return std::fabs(x-y)<=0.00001f; }
bool same(IsleGameVec3 a,IsleGameVec3 b) { return close(a.x,b.x)&&close(a.y,b.y)&&close(a.z,b.z); }
using Hooks=private_actor::WorldHooks;
using WorldLifetime=private_actor::WorldLifetime;
struct Actor {
    a::InstanceHandle handle{};IsleGameObject* object=nullptr;
};
struct PairWitness {
    scene::Services& services;actor::Owner& owner;
    std::array<Actor,3> actors;
    bool observed=false;
    static void observe(IsleGameObject*,void* user) noexcept {
        auto& self=*static_cast<PairWitness*>(user);self.observed=true;
        constexpr std::array<float,3> z{-0.625f,0.6875f,2.6875f};
        for(std::size_t i=0;i<3;++i) {
            const auto* source=self.owner.source(self.actors[i].handle);
            const IsleGameModelPlacement* placement=nullptr;
            if(!source||isle_game_model_placement(self.services.game,self.owner.native_model(self.actors[i].handle),&placement)!=ISLE_GAME_OK||
                !placement||!same(placement->transform.position,{0,-9000,z[i]})||!same(source->collPosition,{0,-9000,z[i]})||!(source->processFlags&0x80u)) {
                self.services.fail("priority1 witness did not observe complete source-order collision2 result");return;
            }
        }
    }
};
struct Consumer {
    scene::Services& services;actor::Owner& owner;world::World& world;
    const actor::Resources& resources;std::size_t definition=actor::actor_no_index;
    std::uint32_t next_id=0x7000;
    Consumer(scene::Services& s,actor::Owner& o,world::World& w,const actor::Resources& r):services(s),owner(o),world(w),resources(r) {
        for(std::size_t i=0;i<r.definitions().size();++i)
            if(r.definitions()[i].actor_id==129&&r.definitions()[i].factory_identity==0x802cac48u)definition=i;
        require(definition!=actor::actor_no_index,"real Magikarp factory definition absent");
    }
    Actor spawn(IsleGameVec3 position,std::uint16_t flags=0x200,float radius=1) {
        world::Spawn input{};input.id=129;input.scale={1,1,1};input.path=world::no_index;
        auto* block=&world.blocks()[0];auto result=owner.spawn({next_id++,definition,block,block,&input});accepted(result.status);
        auto* source=owner.source(result.instance);require(source&&source->transitionGraph==nullptr,"actual factory did not yield admitted NULL graph");
        native(isle_actor_source_pause_object(source,1));
        const IsleGameModelPlacement* old=nullptr;native(isle_game_model_placement(services.game,owner.native_model(result.instance),&old));
        auto transform=old->transform;transform.position=position;accepted(owner.set_placement(result.instance,transform));
        source->flags=flags;source->collisionOffset={};source->collisionRadius=radius;
        return {result.instance,result.object};
    }
    IsleGameVec3 position(Actor actor) const {
        const IsleGameModelPlacement* placement=nullptr;native(isle_game_model_placement(services.game,owner.native_model(actor.handle),&placement));
        return placement->transform.position;
    }
    void reset() { accepted(owner.reset());accepted(owner.collect_joined());require(!services.failed(),services.error()); }
    void four_movement_cases() {
        for(unsigned mask=0;mask<4;++mask) {
            const auto x=spawn({0,-9000,0},static_cast<std::uint16_t>(0x200u|((mask&1u)?0x800u:0u)));
            const auto y=spawn({0,-9000,1},static_cast<std::uint16_t>(0x200u|((mask&2u)?0x800u:0u)));
            if(mask==3) {
                owner.source(x.handle)->tangible=0;
                native(isle_game_object_set_flags(services.game,x.object,1));
            }
            const auto* initial=owner.source(x.handle);const auto untouched_ground=initial->currGround;
            accepted(owner.update_collisions());
            const float expected_x=(mask&2u)?-0.5f:(mask&1u)?-1.0f:0.0f;
            const float expected_y=(mask&2u)?1.5f:1.0f;
            require(same(position(x),{0,-9000,expected_x})&&same(position(y),{0,-9000,expected_y}),"source duplicated-inner800 asymmetric movement changed");
            for(auto actor:{x,y}) { const auto* source=owner.source(actor.handle);
                require(source&&(source->processFlags&0x80u)&&same(source->collPosition,position(actor))&&source->interactionTarget==nullptr,
                    "collision command suppression/canonical position/NULL-graph target changed"); }
            if(mask==0)require(owner.source(x.handle)->currGround.height==untouched_ground.height&&
                owner.source(x.handle)->currGround.surfaceType==untouched_ground.surfaceType,"stationary collision invented a ground query");
            reset();
        }
    }
    void cached_order_and_reuse() {
        auto x=spawn({0,-9000,0},0xa00),old=spawn({0,-9000,9},0xa00),y=spawn({0,-9000,1},0xa00);
        native(isle_game_delete_object(services.game,old.object));accepted(owner.collect_joined());
        auto z=spawn({0,-9000,1.75f},0xa00);
        require(z.handle.pool==old.handle.pool&&z.handle.slot==old.handle.slot&&z.handle.incarnation!=old.handle.incarnation,
            "logical-order discriminator did not reuse the collected physical slot");
        accepted(owner.update_collisions());
        require(same(position(x),{0,-9000,-0.625f})&&same(position(y),{0,-9000,0.6875f})&&same(position(z),{0,-9000,2.6875f}),
            "collision manager refreshed cached outer position or traversed physical/render/link order");
        reset();
    }
    void source_heading_boundaries() {
        for(unsigned mode=0;mode<4;++mode) {
            const float dz=mode<2?0.0f:mode==2?1.0e-5f:std::nextafter(1.0e-5f,std::numeric_limits<float>::infinity());
            auto x=spawn({mode==1?0.0f:-1.0f,-9000,0},0xa00),y=spawn({0,-9000,dz},0xa00);
            accepted(owner.update_collisions());
            if(mode<3) require(position(x).x>(mode==1?0.0f:-1.0f)&&position(y).x>0,
                "source sameZ/coincident/unsuffixed-double fallback did not move both toward positiveX");
            else require(position(x).x<-1.0f&&position(y).x>0,"heading fallback incorrectly used float-rounded1e-5 threshold");
            reset();
        }
    }
    void three_dimensional_and_first_pass() {
        auto x=spawn({0,-9000,0},0x200,2),y=spawn({0,-8997,4},0x200,3);
        owner.source(x.handle)->processFlags|=0x80;owner.source(y.handle)->processFlags|=0x80;
        accepted(owner.update_collisions());
        require(!(owner.source(x.handle)->processFlags&0x80u)&&!(owner.source(y.handle)->processFlags&0x80u)&&
            same(position(x),{0,-9000,0})&&same(position(y),{0,-8997,4}),"strict3D5-unit tangency became planar/epsilon collision");
        auto transform=IsleGameModelTransform{};const IsleGameModelPlacement* placement=nullptr;
        native(isle_game_model_placement(services.game,owner.native_model(y.handle),&placement));transform=placement->transform;transform.position.z=3.9f;
        accepted(owner.set_placement(y.handle,transform));accepted(owner.update_collisions());
        require((owner.source(x.handle)->processFlags&0x80u)&&(owner.source(y.handle)->processFlags&0x80u),"strict3D overlap failed to issue per-pass collision notification");
        reset();
        x=spawn({2,-9000,3},0);auto* source=owner.source(x.handle);source->collPosition={91,92,93};source->processFlags=0x180;
        accepted(owner.update_collisions());require(same(source->collPosition,{2,-9000,3})&&source->processFlags==0x100,
            "first pass skipped nonparticipant or cleared unrelated process flags");reset();
    }
    void queued_pair_order() {
        // Logical A/B/C occupy physical2/0/1. This also lets successful earlier
        // collection be observed before the API reports a later held sender.
        const auto temporary0=spawn({0,-9000,20},0),temporary1=spawn({0,-9000,30},0),x=spawn({0,-9000,0});
        native(isle_game_delete_object(services.game,temporary0.object));accepted(owner.collect_joined());
        const auto y=spawn({0,-9000,1});
        native(isle_game_delete_object(services.game,temporary1.object));accepted(owner.collect_joined());
        const auto z=spawn({0,-9000,1.75f});
        accepted(owner.update_collisions());
        native(isle_game_delete_object(services.game,y.object));
        auto held_join=owner.collect_joined();
        require(!held_join&&held_join.error==actor::OwnerError::lifetime,"queued26 did not retain its joined source");
        world::Spawn input{};input.id=129;input.scale={1,1,1};input.path=world::no_index;auto* block=&world.blocks()[0];
        const auto held=owner.spawn({next_id++,definition,block,block,&input});
        require(!held&&held.status.error==actor::OwnerError::capacity,"first pair sender26 was collected while its receiver still queued it");
        native(isle_game_delete_object(services.game,x.object));
        held_join=owner.collect_joined();
        require(!held_join&&held_join.error==actor::OwnerError::lifetime,"last receiver did not retain original senderA");
        const auto replacement=spawn({0,-9000,4});
        require(replacement.handle.pool==y.handle.pool&&replacement.handle.slot==y.handle.slot&&replacement.handle.incarnation!=y.handle.incarnation&&
            replacement.object==y.object,"actual A-B/A-C command26 order or receiver cancellation did not release B before A");
        require(owner.membership(x.handle).retiring&&owner.membership(x.handle).joined,
            "C did not retain the original first qualifying senderA identity after A/B retirement");
        native(isle_game_delete_object(services.game,z.object));accepted(owner.collect_joined());
        const auto first_released=spawn({0,-9000,6}),released=spawn({0,-9000,7});
        require(first_released.handle.slot==z.handle.slot&&released.handle.pool==x.handle.pool&&released.handle.slot==x.handle.slot&&released.handle.incarnation!=x.handle.incarnation,
            "last queued26 receiver cancellation retained joined senderA physical slot");reset();
    }
    void partial_first_pass_error() {
        const auto x=spawn({2,-9000,3},0),y=spawn({4,-9000,5},0);
        auto* a=owner.source(x.handle);auto* b=owner.source(y.handle);a->collPosition={};a->processFlags=0x80;b->processFlags=0x80;
        b->collisionOffset.x=std::numeric_limits<float>::quiet_NaN();const auto error=owner.update_collisions();
        require(!error&&same(a->collPosition,{2,-9000,3})&&!(a->processFlags&0x80u)&&(b->processFlags&0x80u),
            "failed later canonical refresh fabricated success or rolled back/continued source first-pass effects");
        b->collisionOffset={};reset();
    }
    void actual_priority2(IsleGameObject* root,IsleGameCamera* camera,actor::DetectorBinding& detection) {
        const std::array actors{spawn({0,-9000,0},0xa00),spawn({0,-9000,1},0xa00),spawn({0,-9000,1.75f},0xa00)};
        actor::PhotoScene controller(services,owner,root,camera,detection,nullptr);
        PairWitness witness{services,owner,actors};auto* process=services.process(root,PairWitness::observe,&witness,ISLE_GAME_SYNCHRONOUS,1);
        require(process,"actual priority1 collision witness allocation failed");
        const auto result=isle_game_tick(services.game);const auto joined=isle_game_end_process(services.game,process);
        native(result);native(joined);require(witness.observed&&!services.failed(),services.error());
        controller.retire();reset();
    }
};
void run(rom::Snapshot& snapshot,const std::filesystem::path& profile) {
    require(std::filesystem::is_directory(profile)&&std::filesystem::is_empty(profile),"private collision profile must exist and be empty");
    const world::Resources world_resources(snapshot, isle::course::beach_id);const auto& setup=world_resources.setup();
    const actor::Resources resources(snapshot,0,{setup.fog_rgb,setup.fog_min,setup.fog_max});
    const environment_sound::Resources sounds(snapshot,0);const menu::EffectResources effects_data(snapshot);
    const std::array banks{&effects_data.bank(menu::EffectGroup::common),&effects_data.bank(menu::EffectGroup::beach)};
    auto hydrated=game_audio::hydrate(snapshot);require(bool(hydrated),"actual collision audio hydration failed");
    std::array<audio::effects::Section,8> sections;audio::effects::Config effects;
    require(game_audio::native_startup_effects(hydrated.resources,sections,effects)==audio::Error::none,"actual collision effects failed");
    private_audio::Capture pcm;private_audio::Execution execution{snapshot,nullptr,{}};pcm.samples.resize(8192*audio::AudioService::max_frames*2);
    auto made=game_audio::Session::create(std::move(hydrated.resources),effects,pcm.output(),execution.executor());
    require(bool(made),"actual collision audioSession failed");auto& session=*made.session;std::uint64_t retrace=0;
    const auto pump=[&] {
        ++retrace;require(session.retrace(retrace,retrace*1000000000ull/60)==audio::Error::none,"collision retrace failed");
        require(session.submit()==audio::Error::none,"collision PCM submit failed");audio::TaskView task;
        require(session.build(task)==audio::Error::none,"collision audio task failed");
        if(task.token.serial) { private_audio::execute(session,execution,task);require(session.submit()==audio::Error::none,"collision completedPCM failed"); }
    };
    game::State state(profile);require(state.initialize(1),"actual collision state init failed");game_input::Service input;
    scene::Storage storage(32*1024*1024);native(storage.create());
    {
        scene::Services services(storage.game(),storage.arena(),{{4,6,0,1,2},{}},input,session,state);require(!services.failed(),services.error());
        private_player::Source context(services);
        auto* root=services.object(0,0);auto* camera_object=services.object(1,0);auto* player=services.object(0x1001,0);
        require(root&&camera_object&&player,"actual collision root/camera/player allocation failed");
        IsleGameCamera* camera=nullptr;native(isle_game_attach_camera(services.game,camera_object,&camera));
        native(isle_game_camera_add_matrix(services.game,camera,ISLE_GAME_CAMERA_PERSPECTIVE_FAST));
        native(isle_game_camera_add_matrix(services.game,camera,ISLE_GAME_CAMERA_LOOK_AT_REFLECT_ROLL));
        const IsleGamePerspective perspective{55,4.0f/3.0f,10,25600,1};native(isle_game_camera_set_perspective(services.game,camera,&perspective));
        const IsleGameLookAt view{{0,1600,6000},{0,500,0},{0,1,0}};native(isle_game_camera_set_look_at(services.game,camera,&view));
        IsleGameModelNodeInit node{};node.parent=UINT32_MAX;node.transform.scale={1,1,1};node.matrix_kind=ISLE_GAME_MODEL_CONTEXTUAL_RPY;
        IsleGameModel* player_model=nullptr;native(isle_game_attach_model(services.game,player,&node,1,&player_model));
        native(isle_game_model_set_placement(services.game,player_model,ISLE_GAME_MODEL_PLACEMENT_RTS,&node.transform));
        context.bind(player,player_model,camera);
        environment_sound::Environment environment(services,storage.arena(),sounds);
        require(environment.initialize(nullptr,camera),"environment source initialization failed");
        focus::Detector detector;focus::RegionCopies copies({{0x680000,0x50},{0x680080,0x80},{0x680100,0xa00}});actor::DetectorBinding detection{copies,detector};
        world::ModuleState world_module;
        Hooks hooks{services,camera};world::World world(services,world_resources,world_module,hooks.callbacks());
        a::SlotCapacity maximum{};for(const auto& model:resources.models()) { const auto capacity=a::slot_capacity(model);
            maximum.nodes=std::max(maximum.nodes,capacity.nodes);maximum.material_attachments=std::max(maximum.material_attachments,capacity.material_attachments); }
        const std::array capacities{maximum,maximum,maximum};
        {
            beach::ActorSceneResources sceneData(snapshot);
            std::uint16_t scene_latch=sceneData.initial_scalars().magikarp_active;
            beach::ActorSceneState sceneState(sceneData,scene_latch);
            actor::Owner owner(services,resources,actor::CourseContext(sceneState),world,environment,detector,context.value,capacities,banks,{32768,4096});accepted(owner.publish_effects());hooks.owner=&owner;WorldLifetime lifetime{world,hooks};
            require(environment.set_listener(player_model,camera),"environment listener binding failed");context.value.D_80382D44_523154=1;
            require(!world.current(),"analytic source-default-ground fixture unexpectedly entered a block");
            Consumer consumer(services,owner,world,resources);
            consumer.four_movement_cases();consumer.cached_order_and_reuse();consumer.source_heading_boundaries();
            consumer.three_dimensional_and_first_pass();consumer.queued_pair_order();consumer.partial_first_pass_error();
            consumer.actual_priority2(root,camera,detection);
            require(environment.set_listener(nullptr,nullptr),"environment listener release failed");
        }
        environment.cleanup();
    }
    IsleGameCounts counts{};native(isle_game_counts(storage.game(),&counts));
    require(counts.objects==0&&counts.processes==0&&counts.models==0&&counts.model_nodes==0&&counts.cameras==0&&counts.matrices==0,
        "actual collision API leaked native owners/processes/model/cache borrowers");native(storage.destroy());
    require(session.stop()==audio::Error::none,"collision audio stop failed");
    for(unsigned n=0;n<240&&session.state()!=game_audio::Session::State::draining;++n)pump();
    require(session.state()==game_audio::Session::State::draining&&session.service().voices_retired(),"collision source voice retirement failed");
    require(session.poll_drain()==audio::Error::none&&session.poll_drain()==audio::Error::none&&session.state()==game_audio::Session::State::drained,
        "collision owned PCM drain failed");
    std::cout<<"actual_native_collision_asymmetry_cached_logical_order_heading_3D_FIFO_identity_priority2=passed "
        <<"authored_collision_inputs_not_original_graph_state=true fullgame_Original_GPU_Windows11=not_claimed\n";
}
}
int main(int argc,char** argv) {
    try { require(argc==3,"usage: actor-collision-rom-smoke PRIVATE_ROM EXISTING_EMPTY_PRIVATE_PROFILE");auto snapshot=isle::rom::read_snapshot(argv[1]);run(snapshot,argv[2]);return 0; }
    catch(const std::exception& error) { std::cerr<<"actor-collision-rom-smoke: "<<error.what()<<'\n';return 1; }
}
