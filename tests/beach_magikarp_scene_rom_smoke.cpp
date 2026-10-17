// Explicit private-ROM CPU consumer; never CTest/default discovery. Native
// trigger/player construction is this caller's bounded input, NOT Item physics.
#include "beach_magikarp_scene.hpp"
#include "actor_photo_eligibility.hpp"
#include "actual_audio_driver.hpp"
#include "actual_player_source.hpp"
#include "effect_bank_resources.hpp"
#include "scene_storage.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace {
using namespace isle;
namespace a=menu::animation;
using Outcome=beach::MagikarpSceneOutcome;
void require(bool value,const char* text) { if (!value) throw std::runtime_error(text); }
void native(IsleGameResult value) { require(value==ISLE_GAME_OK,"native scene consumer operation failed"); }
void accepted(actor::OwnerStatus value) { require(bool(value),value.detail); }
void accepted(const beach::MagikarpSceneResult& value) { require(bool(value),value.detail); }
bool equal(IsleGameVec3 x,IsleGameVec3 y) { return x.x==y.x&&x.y==y.y&&x.z==y.z; }
bool equal_counts(IsleGameCounts x,IsleGameCounts y) {
    return x.objects==y.objects&&x.processes==y.processes&&x.sprites==y.sprites&&x.cameras==y.cameras&&
        x.matrices==y.matrices&&x.models==y.models&&x.model_nodes==y.model_nodes;
}
struct Shift {
    scene::Services* services;
    IsleGameCamera* camera;
    actor::Owner* owner=nullptr;
    static bool camera_shift(void* user,world::Vec3 delta) noexcept {
        auto& self=*static_cast<Shift*>(user);
        const IsleGameCameraParameters* parameters=nullptr;
        if (!self.services->check(isle_game_camera_parameters(self.services->game,self.camera,&parameters),"scene camera borrow")) return false;
        auto view=parameters->look_at;
        const std::array d{delta.x,delta.y,delta.z};
        for (unsigned i=0;i<3;++i) { view.eye[i]+=d[i]; view.at[i]+=d[i]; }
        return self.services->check(isle_game_camera_set_look_at(self.services->game,self.camera,&view),"scene camera shift");
    }
    static bool effects_shift(void* user,world::Vec3 delta) noexcept {
        auto& self=*static_cast<Shift*>(user);
        if (!self.owner) { self.services->fail("scene effects shift without Owner"); return false; }
        self.owner->effects().translate({delta.x,delta.y,delta.z});
        return bool(self.owner->effects().status());
    }
};
struct Consumer {
    scene::Services& services;
    actor::Owner& owner;
    environment_sound::Environment& environment;
    world::World& world;
    const beach::ActorResources& resources;
    const beach::BehaviorResources& behaviors;
    beach::MagikarpScene scene;
    beach::MagikarpTrigger trigger;
    world::GroundResult ground{};
    std::size_t callback=beach::actor_no_index,null_choice=beach::actor_no_index;
    Consumer(scene::Services& s,actor::Owner& o,environment_sound::Environment& e,world::World& w,const beach::ActorResources& r,
             const beach::BehaviorResources& b,IsleGameCamera* camera,private_player::Source& context)
        :services(s),owner(o),environment(e),world(w),resources(r),behaviors(b),scene(s,o,w,r,b) {
        IsleGameObject* player_object=nullptr;
        IsleGameModel* player_model=nullptr;
        const auto make_model=[&](std::uint32_t id,IsleGameObject*& object,IsleGameModel*& model) {
            object=services.object(id,4); require(object,"native scene input allocation failed");
            IsleGameModelNodeInit node{}; node.parent=UINT32_MAX;
            node.transform.scale={1,1,1}; node.matrix_kind=ISLE_GAME_MODEL_CONTEXTUAL_RPY;
            native(isle_game_attach_model(services.game,object,&node,1,&model));
        };
        make_model(0x1000,trigger.object,trigger.model);
        make_model(0x1001,player_object,player_model);
        IsleGameModelTransform player{}; player.position={31,47,59}; player.scale={1,1,1};
        native(isle_game_model_set_placement(services.game,player_model,ISLE_GAME_MODEL_PLACEMENT_RT,&player));
        context.bind(player_object,player_model,camera);
        require(environment.set_listener(player_model,camera),"environment listener binding failed");
        // Distinguish trigger DObj position from its unrelated AUX and player
        // AUX from its unrelated DObj. These are real canonical native inputs.
        IsleGameModelTransform wrong{}; wrong.position={1000,2000,3000}; wrong.scale={1,1,1};
        native(isle_game_model_set_placement(services.game,trigger.model,ISLE_GAME_MODEL_PLACEMENT_RTS,&wrong));
        wrong.position={-700,-800,-900};
        native(isle_game_model_set_transform(services.game,player_model,0,&wrong));
        ground.surface=0x337fb2;
        const auto& choices=behaviors.scene_chooser();
        for (std::size_t i=0;i<choices.entries.size();++i) {
            require(choices.entries[i].weight>0,"actual chooser lacks a meaningful positive choice");
            if (choices.entries[i].function_identity==0x802c40a0) callback=i;
            else if (!choices.entries[i].function_identity) null_choice=i;
            else require(false,"actual chooser contains an unknown callback identity");
        }
        require(callback!=beach::actor_no_index&&null_choice!=beach::actor_no_index&&callback!=null_choice,
                "actual two-entry chooser is not helper plus weighted NULL");
    }
    void position(IsleGameVec3 p) {
        IsleGameModelTransform transform{}; transform.position=p;
        transform.rotation={.25f,-.5f,.75f}; transform.scale={3,5,7};
        native(isle_game_model_set_transform(services.game,trigger.model,0,&transform));
    }
    std::uint32_t seed_for(std::int32_t draw) const {
        const auto total=behaviors.scene_chooser().total_weight;
        for (std::uint32_t seed=0;seed<1048576;++seed) {
            auto rng=services.rng; rng.seed=seed;
            if (isle_rng_range(&rng,total)==draw) return seed;
        }
        throw std::runtime_error("bounded seed search could not reach actual chooser draw");
    }
    beach::MagikarpSceneResult draw(std::int32_t wanted) {
        services.rng.seed=seed_for(wanted); auto expected=services.rng;
        require(isle_rng_range(&expected,behaviors.scene_chooser().total_weight)==wanted,"RNG oracle seed mismatch");
        auto result=scene.choose(trigger,ground);
        require(result.draw==wanted,"chooser selected a draw other than the shared application RNG projection");
        // Actual initial-header immediate events belong to the real factory;
        // do not assert those cannot consume RNG without inspecting the ROM.
        if (!result.actor.object) require(services.rng.seed==expected.seed,
                "no-factory chooser branch did not consume exactly one shared source RNG draw");
        return result;
    }
    std::int32_t first_draw(std::size_t choice) const {
        std::int32_t first=0;
        for (std::size_t i=0;i<choice;++i) first+=behaviors.scene_chooser().entries[i].weight;
        return first;
    }
    void reset() { accepted(owner.reset()); accepted(owner.collect_joined()); }
    void boundaries() {
        const auto seed=services.rng.seed;
        world::GroundResult wrong=ground; wrong.surface=0;
        auto result=scene.choose({},wrong); accepted(result);
        require(result.outcome==Outcome::filtered&&result.draw==-1&&services.rng.seed==seed,
                "wrong surface consumed trigger/player or RNG");
        position({61,87,59}); // Exactly 3-4-5 distance50 from player AUX31,47,59.
        result=scene.choose(trigger,ground); accepted(result);
        require(result.outcome==Outcome::filtered&&result.draw==-1&&services.rng.seed==seed,"distance50 was not strict");
        position({31,47,59});
        result=scene.choose(trigger,ground); accepted(result);
        require(result.outcome==Outcome::filtered&&services.rng.seed==seed,"zero distance consumed RNG");
        position({61,87.125f,59}); // Just beyond50; AUX1000,2000,3000 is irrelevant.
        owner.course().magikarp_active=true;
        const auto boundary=behaviors.scene_chooser().entries[0].weight;
        for (const auto wanted:std::array<std::int32_t,4>{0,boundary-1,boundary,behaviors.scene_chooser().total_weight-1}) {
            result=draw(wanted); accepted(result);
            const std::size_t index=wanted<boundary?0:1;
            require(result.choice==index&&!result.actor.object&&!result.actor.instance&&owner.course().magikarp_active&&
                    result.outcome==(index==callback?Outcome::already_active:Outcome::null_choice),
                    "sum>draw/NULL/active branch ordering changed");
        }
        owner.course().magikarp_active=false;
        result=draw(first_draw(null_choice)); accepted(result);
        require(result.outcome==Outcome::null_choice&&!owner.course().magikarp_active&&!result.actor.object,
                "weighted NULL was treated as terminator or spawn/latch success");
    }
    void inspect(const beach::MagikarpSceneResult& result,IsleGameVec3 copied) {
        accepted(result); require(result.outcome==Outcome::spawned&&result.actor.object&&result.actor.instance,
                                  "scene helper did not publish actual native factory actor");
        const auto h=result.actor.instance;
        auto* record=owner.source(h); require(record,"scene factory record absent");
        std::size_t definition=beach::actor_no_index;
        for (std::size_t i=0;i<resources.definitions().size();++i)
            if (resources.definitions()[i].wire.guest_address==0x802cbff4) definition=i;
        require(definition!=beach::actor_no_index,"scene definition absent");
        const auto& init=resources.initializations()[resources.definitions()[definition].initialization];
        world::GroundResult zero_ground{};
        const bool has_zero_ground=world.ground_at(0,0,zero_ground);
        const IsleGameVec3 zero_spawn{0,has_zero_ground?zero_ground.height:0.0f,0};
        const IsleGameModelPlacement* placement=nullptr;
        native(isle_game_model_placement(services.game,owner.native_model(h),&placement));
        require(record->id==129&&record->initData==&init&&record->baseBlock==world.current()&&record->behavior==0&&record->path==nullptr&&
            equal(record->pos1,zero_spawn)&&equal(record->targetPos,zero_spawn)&&equal(record->euler,{0,0,0})&&
            record->stateProc&&record->pathProc==nullptr&&record->processFlags==0&&record->loopCount==0&&record->counter==0&&
            static_cast<const beach::ActorAnimationHeader*>(record->animHeader)->wire.guest_address==0x802cda8c&&
            equal(placement->transform.position,copied)&&equal(placement->transform.rotation,{0,0,0})&&
            equal(placement->transform.scale,{init.scale.x*.1f,init.scale.y*.1f,init.scale.z*.1f}),
            "current/current zero factory then exact DObj-to-AUX postcopy/header/init scale/native tail policy changed");
        bool binding=false;
        for (const auto& b:owner.render_bindings()) if (b.model.object==result.actor.object) {
            require(b.model.state.instance==h&&b.render.link==5,"scene actor bypassed same Runtime/DL5 binding"); binding=true;
        }
        require(binding&&owner.registered(h)&&owner.animation().model(h),"scene actor bypassed real Owner membership/Runtime");
        accepted(owner.update_collisions());
        const IsleGameModelNode* nodes=nullptr; std::size_t node_count=0;
        native(isle_game_model_peek_nodes(services.game,owner.native_model(h),&nodes,&node_count));
        require(node_count!=0,"actual source collision root absent");
        // At this pre-state boundary AUX Euler is zero. Independent analytic
        // translation oracle catches using rendered origin, scaling the offset,
        // omitting flag1 root position, or updating the wrong canonical record.
        const auto root=nodes[0].transform.position;
        const auto scale=placement->transform.scale;
        const auto offset=record->collisionOffset;
        const bool root_offset=(record->flags&1)!=0;
        const auto close=[](float value,double expected) { return std::abs(double(value)-expected)<0.002; };
        require(close(record->collPosition.x,double(copied.x)+offset.x+(root_offset?double(root.x)*scale.x:0))&&
                close(record->collPosition.y,double(copied.y)+offset.y+(root_offset?double(root.y)*scale.y:0))&&
                close(record->collPosition.z,double(copied.z)+offset.z+(root_offset?double(root.z)*scale.z:0)),
                "actual source collision field differs from analytic zero-Euler AUX/root/offset composition");
        actor::photo::Camera camera;
        native(actor::photo::prepare_camera(services.game,owner.player_source().camera,camera));
        bool eligible=false;
        native(actor::photo::admitted(*record,camera,record->collPosition,eligible));
        // Actual caller camera is eye0,at+X,up+Y: view=(worldZ,worldY,-worldX).
        // This is CPU predicate proof, not a tuned GPU framing/focus claim.
        const double depth=-double(record->collPosition.x);
        bool expected=depth<=-1&&depth>=-10000;
        if (expected) {
            const auto x=std::trunc(double(record->collPosition.z)*228.506134/depth);
            const auto y=std::trunc(double(record->collPosition.y)*228.506134/depth);
            expected=x>=-240&&x<=240&&y>=-180&&y<=180;
        }
        expected=expected&&record->playerDist<=10000&&record->tangible!=0;
        require(eligible==expected&&((record->flags&0x100)==0)==expected,
                "actual camera/source-record photograph eligibility or flag100 publication differs");
        std::cout<<"actual_scene_source_collision="<<record->collPosition.x<<','<<record->collPosition.y<<','
                 <<record->collPosition.z<<" source_photo_predicate="<<eligible<<'\n';
    }
    void helpers_and_failures() {
        const IsleGameVec3 copied{123.25f,157.5f,-93.75f}; position(copied);
        auto first=scene.spawn_at(trigger); inspect(first,copied);
        require(!owner.course().magikarp_active,"direct helper set chooser latch");
        owner.course().magikarp_active=true;
        auto second=scene.spawn_at(trigger); inspect(second,copied);
        require(owner.course().magikarp_active,"direct helper cleared existing latch");
        owner.course().magikarp_active=false;
        IsleGameCounts before{},after{}; native(isle_game_counts(services.game,&before));
        auto failed=draw(first_draw(callback));
        native(isle_game_counts(services.game,&after));
        require(!failed&&failed.error==beach::MagikarpSceneError::factory&&
            failed.actor.status.error==actor::OwnerError::capacity&&!failed.actor.object&&!failed.actor.instance&&
            !owner.course().magikarp_active&&equal_counts(before,after)&&!services.failed(),
            "chosen factory failure set latch, invented actor or changed native lifetime");
        reset();
        position({40000,157.5f,-93.75f}); // Contextual DObj accepts it; AUX fixed translation cannot.
        failed=draw(first_draw(callback));
        require(!failed&&failed.error==beach::MagikarpSceneError::postcopy&&failed.actor.object&&
                owner.registered(failed.actor.instance)&&!owner.course().magikarp_active,
                "unrepresentable postcopy silently clamped, hid successful factory or set latch");
        reset();
    }
    void lifetime() {
        const IsleGameVec3 copied{123.25f,157.5f,-93.75f}; position(copied);
        const auto result=draw(first_draw(callback)); inspect(result,copied);
        require(owner.course().magikarp_active,"successful chosen helper did not publish latch");
        const auto h=result.actor.instance;
        bool animation_ended=false,retired=false,cleared=false;
        // Fixed original diagnostic envelope, no advance-until-pass or timer edits.
        for (unsigned frame=0;frame<512;++frame) {
            native(isle_game_tick(services.game)); require(!services.failed(),services.error());
            const auto membership=owner.membership(h);
            animation_ended|=(membership.process_flags&POKEMON_PROCESS_FLAG_ANIMATION_ENDED)!=0;
            if (frame==0) {
                const auto* record=owner.source(h);
                require(record&&record->pathProc&&record->pokemonLoopTarget==1&&
                    static_cast<const beach::ActorAnimationHeader*>(record->animHeader)->wire.guest_address==0x802cdab4,
                    "actual source state/header/path did not run on native scheduler");
            }
            if (!owner.course().magikarp_active) {
                require(animation_ended,"latch cleared before actual source animation end"); cleared=true;
            }
            if (membership.retiring) {
                require(animation_ended&&cleared&&membership.joined&&!membership.logical&&!membership.registered,
                        "actual source latch clear/RunCleanup/native join order changed"); retired=true;
            }
            accepted(owner.collect_joined());
        }
        require(animation_ended&&cleared&&retired&&!owner.source(h)&&!owner.object(h)&&!owner.animation().model(h)&&
                !owner.set_animation(h,nullptr),"source actor failed joined physical retirement/stale handle rejection within512");
        reset();
        require(environment.set_listener(nullptr,nullptr),"environment listener release failed");
        services.erase(trigger.object); trigger.object=nullptr;
        const auto seed=services.rng.seed;
        const auto stale=scene.choose(trigger,ground);
        require(!stale&&stale.error==beach::MagikarpSceneError::native&&services.rng.seed==seed,
                "expired native trigger identity consumed RNG or invented position");
    }
};
void run(rom::Snapshot& snapshot,const std::filesystem::path& profile) {
    require(std::filesystem::is_directory(profile)&&std::filesystem::is_empty(profile),"private profile directory must exist and be empty");
    const world::Resources world_resources(snapshot); const auto& setup=world_resources.setup();
    const beach::ActorResources resources(snapshot,{setup.fog_rgb,setup.fog_min,setup.fog_max});
    const beach::BehaviorResources behaviors(snapshot);
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
    const auto pump=[&] {
        ++retrace; require(session.retrace(retrace,retrace*1000000000ull/60)==audio::Error::none,"audio retrace failed");
        require(session.submit()==audio::Error::none,"PCM submission failed");
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
            auto* object=services.object(1,0); require(object,"camera object allocation failed");
            IsleGameCamera* camera=nullptr; native(isle_game_attach_camera(services.game,object,&camera));
            const IsleGameLookAt view{{0,0,0},{1,0,0},{0,1,0}}; native(isle_game_camera_set_look_at(services.game,camera,&view));
            environment_sound::Environment environment(services,storage.arena(),sound_resources);
            require(environment.initialize(nullptr,camera),"environment source initialization failed");
            focus::Detector detector; Shift shift{&services,camera};
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
                require(world.enter_first(0)!=nullptr,"actual first World admission failed");
                Consumer consumer(services,owner,environment,world,resources,behaviors,camera,context);
                consumer.boundaries(); consumer.helpers_and_failures(); consumer.lifetime();
                shift.owner=nullptr;
            }
            require(world.retire(),"scene World retirement failed"); environment.cleanup();
        }
        native(storage.destroy());
    }
    require(session.stop()==audio::Error::none,"actual audio stop failed");
    for (unsigned n=0;n<240&&session.state()!=game_audio::Session::State::draining;++n) pump();
    require(session.state()==game_audio::Session::State::draining&&session.service().voices_retired(),"actual audio voices failed retirement");
    require(session.poll_drain()==audio::Error::none&&session.poll_drain()==audio::Error::none&&session.state()==game_audio::Session::State::drained,
            "actual PCM owner failed drain");
    std::cout<<"beach_magikarp_scene_private_CPU_behavior=passed Item_physics_GPU_Original_Windows11=not_claimed\n";
}
}
int main(int argc,char** argv) {
    try {
        require(argc==3,"usage: beach-magikarp-scene-rom-smoke PRIVATE_ROM EXISTING_EMPTY_PRIVATE_PROFILE_DIRECTORY");
        auto snapshot=isle::rom::read_snapshot(argv[1]); run(snapshot,argv[2]); return EXIT_SUCCESS;
    } catch (const std::exception& error) { std::cerr<<"beach-magikarp-scene-rom-smoke: "<<error.what()<<'\n'; return EXIT_FAILURE; }
}
