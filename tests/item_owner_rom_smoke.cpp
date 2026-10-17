// Explicit private-ROM native CPU consumer; never CTest/default discovery.
// Caller-authored Player binding/collision inputs below are API fixtures, NOT a
// Player FSM, Original capture, GPU-fence proof, or complete game qualification.
#include "item_owner.hpp"
#include "actor_spatial.h"
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
#include <span>
#include <string_view>
#include <utility>

namespace {
using namespace isle;
namespace a=menu::animation;
namespace it=items;
void require(bool value,const char* text) {
    if (!value) {
        std::cerr<<"Item CPU guard: "<<text<<'\n';
        throw std::runtime_error(text);
    }
}
void native(IsleGameResult value) { require(value==ISLE_GAME_OK,"native Item consumer operation failed"); }
void accepted(it::OwnerStatus value) { require(bool(value),value.detail); }
void accepted(actor::OwnerStatus value) { require(bool(value),value.detail); }
bool same(IsleGameVec3 x,IsleGameVec3 y) { return std::abs(x.x-y.x)<0.0001f && std::abs(x.y-y.y)<0.0001f && std::abs(x.z-y.z)<0.0001f; }
enum class Scenario { complete, physical, fume, threshold, fault_empty, fault_live, fault_collected };
struct Shift {
    scene::Services* services;
    IsleGameCamera* camera;
    actor::Owner* actors=nullptr;
    it::Owner* items=nullptr;
    static bool camera_shift(void* user,world::Vec3 delta) noexcept {
        auto& self=*static_cast<Shift*>(user);
        const IsleGameCameraParameters* parameters=nullptr;
        if (!self.services->check(isle_game_camera_parameters(self.services->game,self.camera,&parameters),"Item fixture camera borrow")) return false;
        auto look=parameters->look_at;
        for (unsigned n=0;n<3;++n) { const auto d=n==0?delta.x:n==1?delta.y:delta.z; look.eye[n]+=d; look.at[n]+=d; }
        if (!self.services->check(isle_game_camera_set_look_at(self.services->game,self.camera,&look),"Item fixture camera shift")) return false;
        if (self.items && !self.items->displace(delta)) {self.services->fail("Item fixture world shift");return false;}
        return true;
    }
    static bool effects_shift(void* user,world::Vec3 delta) noexcept {
        auto& self=*static_cast<Shift*>(user);
        if (!self.actors) { self.services->fail("Item fixture effects owner absent"); return false; }
        self.actors->effects().translate({delta.x,delta.y,delta.z});
        return bool(self.actors->effects().status());
    }
};
struct Consumer {
    scene::Services& services;
    it::Owner& items;
    actor::Owner& actors;
    it::Globals& globals;
    void tick(unsigned count=1) {
        for (unsigned i=0;i<count;++i) { native(isle_game_tick(services.game)); require(!services.failed(),services.error()); }
    }
    IsleGameModelTransform root(a::InstanceHandle handle) {
        const IsleGameModelNode* nodes=nullptr; std::size_t count=0;
        native(isle_game_model_peek_nodes(services.game,items.native_model(handle),&nodes,&count));
        require(count,"Item native root absent");
        const auto pose=items.animation().model(handle);
        require(bool(pose) && same(nodes[0].transform.position,{pose.nodes[0].transform.position.x,pose.nodes[0].transform.position.y,pose.nodes[0].transform.position.z}) &&
            same(nodes[0].transform.scale,{pose.nodes[0].transform.scale.x,pose.nodes[0].transform.scale.y,pose.nodes[0].transform.scale.z}),
            "source Item root and authoritative Runtime differ");
        return nodes[0].transform;
    }
    void clear() { accepted(items.retire()); tick(); accepted(items.collect_joined()); }
    void startup_flight_pause_and_displacement() {
        const auto spawned=items.spawn_apple({100,1000,200},{3,4,5}); accepted(spawned.status);
        const auto* source=items.source(spawned.instance);
        require(source && source->itemID==163 && source->state==1 && source->flags==0 && source->restTimer==0 &&
            same(source->velocity,{3,4,55}) && same(source->prevPos,{100,1000,200}) && same(source->collisionVelocity,{0,0,0}),
            "source factory or explicit native unwritten-field policy differs");
        require(same(root(spawned.instance).position,{100,1000,200}) && same(root(spawned.instance).scale,{.1f,.1f,.1f}),"source initial root differs");
        float sender_distance=-1;
        native(isle_actor_spatial_distance(services.game,actors.player_source().object,spawned.object,&sender_distance));
        require(std::abs(sender_distance-std::sqrt((100.0f*100.0f+1000.0f*1000.0f)+200.0f*200.0f))<0.0001f,
                "Item source distance consumed a fabricated AUX instead of its actual root");
        tick(); source=items.source(spawned.instance);
        require(source && same(source->velocity,{3,3.4f,55}) && same(root(spawned.instance).position,{101.5f,1001.7f,227.5f}),
                "original gravity-before-half-step flight changed");
        accepted(items.pause()); const auto paused=root(spawned.instance).position; tick(3);
        require(same(root(spawned.instance).position,paused),"pause failed to suspend native Item processes");
        accepted(items.displace({11,13,17}));
        require(same(root(spawned.instance).position,{paused.x+11,paused.y+13,paused.z+17}) &&
            same(items.source(spawned.instance)->prevPos,{111,1013,217}),"displacement omitted actual root or canonical previous position");
        accepted(items.resume()); tick(); require(!same(root(spawned.instance).position,paused),"resume failed to continue source movement");
        clear();
    }
    void canonical_throw_boundaries() {
        auto& player=actors.player_source();
        const auto target=player.cam_target;
        const auto velocity=player.PlayerVelocity;
        const IsleGameCameraParameters* parameters=nullptr;
        native(isle_game_camera_parameters(services.game,player.camera,&parameters));
        const auto original=parameters->look_at;
        player.PlayerVelocity={3,4,5};
        for (bool second:{false,true}) {
            const IsleGameLookAt view=second?IsleGameLookAt{{-50,60,70},{-50,60,170},{0,1,0}}:
                IsleGameLookAt{{20,30,40},{20,30,140},{0,1,0}};
            native(isle_game_camera_set_look_at(services.game,player.camera,&view));
            player.cam_target=second?IsleGameVec3{50,-40,70}:IsleGameVec3{20,-70,140};
            native(isle_game_camera_parameters(services.game,player.camera,&parameters));
            const auto& eye=parameters->look_at.eye;
            // The original Player supplies current main-camera eye and its
            // velocity; original Item init consumes the same Source target.
            const auto item=items.spawn_apple({eye[0],eye[1],eye[2]},player.PlayerVelocity); accepted(item.status);
            require(same(root(item.instance).position,{eye[0],eye[1],eye[2]}) &&
                    same(items.source(item.instance)->velocity,second?IsleGameVec3{53,4,5}:IsleGameVec3{3,4,55}),
                    "throw retained an earlier native camera eye or canonical Source target");
            clear();
        }
        player.cam_target=target; player.PlayerVelocity=velocity;
        native(isle_game_camera_set_look_at(services.game,player.camera,&original));
    }
    void mixed_oldest_and_iterator() {
        const auto oldest=items.spawn_apple({100,1000,200},{0,0,0}); accepted(oldest.status);
        for (unsigned i=0;i<5;++i) accepted(items.spawn_ball({100,1000,200},{0,0,0}).status);
        require(globals.source.total==6 && globals.source.balls==5 && globals.source.apples==1,"initial mixed source counts differ");
        const auto newest=items.spawn_ball({100,1000,200},{0,0,0}); accepted(newest.status);
        require(!items.exists(oldest.object) && globals.source.apples==0 && globals.source.balls==6 && globals.source.total==6,
                "family>=5 eviction did not delete overall allocated-list oldest");
        items.init_iterator(); const auto* next=items.next_valid(); accepted(items.query_status());
        require(next==newest.object,"free-list-head reuse was confused with allocated-list-tail order");
        require(items.source(oldest.instance)->state==0,"logical unlink wrongly destroyed held canonical Item record immediately");
        clear(); require(globals.source.payload_cache>=7,"source reusable payload cache was omitted");
    }
    void kind1_actor_tail_and_real_target_retention(const actor::SpawnResult& actor) {
        auto* pokemon=actors.source(actor.instance); require(pokemon,"actual canonical Actor absent");
        native(isle_actor_source_pause_object(pokemon,1));
        // Authored canonical collision API input, deliberately not an eager
        // cache-refresh call or alternate native link3 sphere implementation.
        pokemon->tangible=1; pokemon->collisionRadius=5; pokemon->collPosition={100,1000,220}; pokemon->flags|=2;
        const auto ball=items.spawn_ball({100,1000,200},{0,0,0}); accepted(ball.status);
        tick(); auto* source=items.source(ball.instance);
        require(source && same(source->collisionVelocity,{0,0,50}) && same(source->velocity,{0,-.6f,-12.5f}) &&
            same(root(ball.instance).position,{100,999.7f,208.75f}),"kind1 self-end discarded original Actor-hit response tail");
        clear();
        const auto apple=items.spawn_apple({100,1000,200},{0,0,0}); accepted(apple.status);
        auto* held=items.source(apple.instance);
        require(services.source_reference_assign(actor.object,&pokemon->apple,apple.object) &&
                services.source_reference_assign(actor.object,&pokemon->interactionTarget,apple.object),"canonical Item adoption failed");
        accepted(items.delete_item(apple.object));
        require(pokemon->apple==apple.object && pokemon->interactionTarget==apple.object,"source deletion bypassed queued21/22 ordering");
        tick();
        require(!pokemon->apple && pokemon->interactionTarget==apple.object && items.membership(apple.instance).joined,
                "apple21-before-unlink or scheduled logical-target22-before-nativeDelete order changed");
        const auto blocked=items.collect_joined();
        require(!blocked && blocked.error==it::OwnerError::lifetime,"held canonical target/queued22 was physically recycled");
        const auto other=items.spawn_apple({100,1000,200},{0,0,0}); accepted(other.status);
        require(other.object!=apple.object && items.source(other.instance)!=held,"held Item semantic/native address was stolen");
        tick(); require(!pokemon->interactionTarget,"real logical Actor target22 did not clear canonical field");
        accepted(items.collect_joined()); clear();
    }
    void actual_waiting_splash() {
        const auto trigger=items.spawn_apple({100,1000,200},{0,0,0}); accepted(trigger.status);
        auto* source=items.source(trigger.instance); source->state=ISLE_ITEM_STILL;
        native(isle_game_end_source_processes(services.game,trigger.object,Items_UpdateItemMovement));
        // Explicit source-process API entry: no substitute fume timer/algorithm.
        IsleGameProcess* process=nullptr;
        native(isle_game_add_source_process(services.game,trigger.object,Items_CreateWaterSplash,ISLE_GAME_STACKFUL,7,&process));
        tick();
        IsleGameObject* fume=nullptr; a::InstanceHandle fume_instance;
        for (const auto& binding:items.render_bindings()) if (binding.model.object && binding.family==it::Family::water_splash) {
            fume=binding.model.object; fume_instance=items.instance(fume);
        }
        require(fume && !items.source(fume) && globals.source.total==1,"fume acquired a logical Item payload/count or was not spawned");
        require(same(root(trigger.instance).position,{100,998,200}) && same(root(fume_instance).scale,{.1f,.1f,.1f}),"splash failed first sink-before-wait/root scale");
        accepted(items.pause()); const auto held=root(trigger.instance).position; tick(2);
        require(same(root(trigger.instance).position,held),"pause excluded waiting sink process");
        accepted(items.displace({7,11,13}));
        require(same(root(fume_instance).position,{107,-8989,213}),"displacement excluded NULL-payload fume");
        accepted(items.resume()); tick(59);
        require(!items.exists(trigger.object) && !items.object(trigger.instance) && items.object(fume_instance)==fume,
                "59-then-delete source sink wait was replaced by fume lifetime or coupled to trigger deletion");
        require(!items.source(fume) && items.animation().model(fume_instance),"independent fume lost actual animation storage");
        tick(8); require(!items.object(fume_instance),"actual .9 animation-time fume cutoff did not delete independent native object");
        accepted(items.collect_joined());
    }
    void reject_without_source_mutation(const it::SpawnResult* live=nullptr,bool pester=false) {
        const auto total=globals.source.total, balls=globals.source.balls, apples=globals.source.apples;
        const auto cursor=globals.source.object_counter;
        const auto cache=globals.source.payload_cache;
        const auto* head=globals.source.allocated_head;
        const auto* tail=globals.source.allocated_tail;
        const auto* free=globals.source.free_head;
        IsleGameCounts before{}; native(isle_game_counts(services.game,&before));
        for (unsigned attempt=0;attempt<7;++attempt) {
            const auto rejected=pester?items.spawn_ball({100,1000,200},{0,0,0}):items.spawn_apple({100,1000,200},{0,0,0});
            require(!rejected.status && rejected.status.error==it::OwnerError::capacity,
                    "physical exhaustion was not a recoverable pre-admission rejection");
            require(globals.source.total==total && globals.source.balls==balls && globals.source.apples==apples &&
                    globals.source.object_counter==cursor && globals.source.payload_cache==cache &&
                    globals.source.allocated_head==head && globals.source.allocated_tail==tail && globals.source.free_head==free,
                    "rejected Item factory mutated source counts, cursor, cache or oldest/logical lists");
            IsleGameCounts after{}; native(isle_game_counts(services.game,&after));
            require(after.objects==before.objects && after.processes==before.processes && after.model_nodes==before.model_nodes,
                    "rejected Item factory acquired or retired native resources");
            if (live) require(items.exists(live->object) && items.object(live->instance)==live->object &&
                              items.source(live->instance)->state!=ISLE_ITEM_INVALID,
                              "repeated rejected factories evicted the original logical Item");
        }
    }
    void physical_exhaustion(const actor::SpawnResult& actor) {
        const auto first=items.spawn_apple({100,1000,200},{0,0,0}); accepted(first.status);
        reject_without_source_mutation(&first);
        reject_without_source_mutation(&first,true);
        auto* pokemon=actors.source(actor.instance); require(pokemon,"retention Actor absent");
        require(services.source_reference_assign(actor.object,&pokemon->interactionTarget,first.object),
                "physical-exhaustion fixture could not adopt actual Item target");
        accepted(items.delete_item(first.object)); tick();
        require(items.membership(first.instance).joined && pokemon->interactionTarget==first.object,
                "physical-exhaustion fixture did not retain its actually joined Item");
        require(!items.collect_joined(),"joined-but-borrowed Item was collected");
        const auto cursor=globals.source.object_counter;
        reject_without_source_mutation();
        tick(); require(!pokemon->interactionTarget,"actual queued22 did not release retained physical Item");
        accepted(items.collect_joined());
        const auto next=items.spawn_apple({100,1000,200},{0,0,0}); accepted(next.status);
        std::uint32_t id=0; native(isle_game_object_id(services.game,next.object,&id));
        require(id==300u+(static_cast<std::uint32_t>(cursor)+1u)%20u &&
                globals.source.total==1 && globals.source.apples==1 &&
                next.instance.slot==first.instance.slot && next.instance.incarnation!=first.instance.incarnation,
                "legitimate collection did not preserve next source ID/count and fresh physical incarnation");
    }
    void true_threshold_exhaustion(const actor::SpawnResult& actor) {
        const auto oldest=items.spawn_apple({100,1000,200},{0,0,0}); accepted(oldest.status);
        for (unsigned n=0;n<5;++n) accepted(items.spawn_ball({100,1000,200},{0,0,0}).status);
        accepted(items.pause());
        auto* pokemon=actors.source(actor.instance); require(pokemon,"threshold Actor absent");
        require(services.source_reference_assign(actor.object,&pokemon->apple,oldest.object),
                "threshold fixture could not adopt actual oldest apple");
        require(globals.source.total==6 && globals.source.balls==5 && globals.source.apples==1 &&
                globals.source.allocated_head->obj==oldest.object,"threshold fixture lost actual mixed overall-oldest cohort");
        reject_without_source_mutation(&oldest,true);
        tick();
        require(pokemon->apple==oldest.object && items.exists(oldest.object),
                "failed threshold admission enqueued apple21 or retired/cancelled its legitimate oldest");
        require(services.source_reference_assign(actor.object,&pokemon->apple,nullptr),"threshold canonical hold release failed");
    }
    void independent_fume_exhaustion() {
        const auto trigger=items.spawn_apple({100,1000,200},{0,0,0}); accepted(trigger.status);
        items.source(trigger.instance)->state=ISLE_ITEM_STILL;
        native(isle_game_end_source_processes(services.game,trigger.object,Items_UpdateItemMovement));
        IsleGameProcess* process=nullptr;
        native(isle_game_add_source_process(services.game,trigger.object,Items_CreateWaterSplash,ISLE_GAME_STACKFUL,7,&process));
        tick();
        IsleGameObject* fume=nullptr;
        for (const auto& binding:items.render_bindings()) if (binding.family==it::Family::water_splash) fume=binding.model.object;
        require(fume && !items.source(fume),"independent source fume absent");
        accepted(items.delete_item(trigger.object)); tick(2); accepted(items.collect_joined());
        require(items.instance(fume) && !items.exists(fume) && globals.source.total==0,
                "fume did not retain an independent physical-only slot");
        const auto apple=items.spawn_apple({100,1000,200},{0,0,0}); accepted(apple.status);
        reject_without_source_mutation(&apple);
        require(bool(items.instance(fume)),"physical rejection retired an independent source fume");
    }
    void fault(const actor::SpawnResult& actor,Scenario scenario) {
        if (scenario==Scenario::fault_live) {
            const auto live=items.spawn_apple({100,1000,200},{0,0,0}); accepted(live.status);
            const auto pending=items.spawn_ball({100,1000,200},{0,0,0}); accepted(pending.status);
            auto* pokemon=actors.source(actor.instance);
            require(services.source_reference_assign(actor.object,&pokemon->interactionTarget,live.object) &&
                    services.source_reference_assign(actor.object,&pokemon->apple,pending.object),
                    "fault fixture failed to retain both real Item fields");
            accepted(items.delete_item(pending.object));
            require(items.exists(live.object) && !items.exists(pending.object) && !items.membership(pending.instance).retiring,
                    "fault fixture requires both live and logically-deleted, not-yet-retired Items");
        } else if (scenario==Scenario::fault_collected) {
            const auto item=items.spawn_apple({100,1000,200},{0,0,0}); accepted(item.status);
            accepted(items.retire()); tick(); accepted(items.collect_joined());
            require(globals.source.payload_cache==1 && !items.animation().model(item.instance),
                    "fault fixture did not first collect an actually retired Item");
        }
        static const IsleActorInteractionGraph invalid_graph{};
        actors.source(actor.instance)->transitionGraph=&invalid_graph;
        require(services.send_command(actor.object,26,actor.object),"fault fixture could not enqueue actual custom command");
        require(isle_game_tick(services.game)==ISLE_GAME_EXECUTION_FAILURE && services.failed(),
                "invalid actual C graph did not produce a terminal native and Services failure");
    }
};
void run(rom::Snapshot& snapshot,const std::filesystem::path& profile,Scenario scenario) {
    require(std::filesystem::is_directory(profile) && std::filesystem::is_empty(profile),"private profile must exist and be empty");
    const world::Resources world_resources(snapshot); const auto& setup=world_resources.setup();
    const beach::ActorResources actor_resources(snapshot,{setup.fog_rgb,setup.fog_min,setup.fog_max});
    const it::Resources item_resources(snapshot,setup.fog_rgb,setup.fog_min,setup.fog_max);
    const environment_sound::BeachResources sound_resources(snapshot);
    const menu::EffectResources effect_resources(snapshot);
    const std::array banks{&effect_resources.bank(menu::EffectGroup::common),&effect_resources.bank(menu::EffectGroup::beach)};
    auto hydrated=game_audio::hydrate(snapshot); require(bool(hydrated),"actual audio hydration failed");
    std::array<audio::effects::Section,8> sections; audio::effects::Config effects;
    require(game_audio::native_startup_effects(hydrated.resources,sections,effects)==audio::Error::none,"actual audio effects admission failed");
    private_audio::Capture capture; private_audio::Execution execution{snapshot,nullptr,{}};
    capture.samples.resize(8192*audio::AudioService::max_frames*2);
    auto made=game_audio::Session::create(std::move(hydrated.resources),effects,capture.output(),execution.executor());
    require(bool(made),"actual Item audio Session creation failed"); auto& session=*made.session;
    {
        game::State state(profile); require(state.initialize(1),"private profile initialization failed");
        game_input::Service input; scene::Storage storage(32*1024*1024); native(storage.create());
        {
            scene::Services services(storage.game(),storage.arena(),{{4,6,0,1,2},{}},input,session,state);
            private_player::Source player_source(services);
            auto* camera_object=services.object(1,0); require(camera_object,"native camera allocation failed");
            IsleGameCamera* camera=nullptr; native(isle_game_attach_camera(services.game,camera_object,&camera));
            const IsleGameLookAt look{{0,0,0},{1,0,0},{0,1,0}}; native(isle_game_camera_set_look_at(services.game,camera,&look));
            auto* player=services.object(2,9); require(player,"API fixture Player object allocation failed");
            IsleGameModelNodeInit node{}; node.parent=UINT32_MAX; node.transform.scale={1,1,1}; node.matrix_kind=ISLE_GAME_MODEL_CONTEXTUAL_RPY;
            IsleGameModel* player_model=nullptr; native(isle_game_attach_model(services.game,player,&node,1,&player_model));
            const IsleGameModelTransform player_placement{{0,0,0},{0,0,0},{1,1,1}};
            native(isle_game_model_set_placement(services.game,player_model,ISLE_GAME_MODEL_PLACEMENT_RTS,&player_placement));
            player_source.value.cam_target={100,900,300};
            environment_sound::Environment environment(services,storage.arena(),sound_resources);
            require(environment.initialize(nullptr,camera),"environment source initialization failed");
            focus::Detector detector; Shift shift{&services,camera};
            world::World world(services,world_resources,{&shift,nullptr,nullptr,nullptr,Shift::camera_shift,Shift::effects_shift,nullptr});
            a::SlotCapacity actor_capacity{},item_capacity{};
            for (const auto& model:actor_resources.models()) { const auto c=a::slot_capacity(model); actor_capacity.nodes=std::max(actor_capacity.nodes,c.nodes); actor_capacity.material_attachments=std::max(actor_capacity.material_attachments,c.material_attachments); }
            for (const auto& model:item_resources.models()) { const auto c=a::slot_capacity(model); item_capacity.nodes=std::max(item_capacity.nodes,c.nodes); item_capacity.material_attachments=std::max(item_capacity.material_attachments,c.material_attachments); }
            const std::array actor_capacities{actor_capacity,actor_capacity};
            // Fixed caller-declared test physical capacity; NOT a production
            // source20 bound or capacity selected after observing execution.
            std::array<a::SlotCapacity,32> item_capacities; item_capacities.fill(item_capacity);
            const auto item_count=scenario==Scenario::physical?std::size_t{1}:scenario==Scenario::fume?std::size_t{2}:
                scenario==Scenario::threshold?std::size_t{6}:item_capacities.size();
            const auto selected_capacities=std::span<const a::SlotCapacity>(item_capacities).first(item_count);
            {
                actor::Owner actors(services,actor_resources,world,environment,detector,player_source.value,actor_capacities,banks,{32768,4096}); shift.actors=&actors;
                accepted(actors.publish_effects());
                require(environment.set_listener(player_model,camera),"environment listener binding failed");
                it::Globals globals;
                {
                    it::Owner items(services,item_resources,world,actors,globals,player_source.value,selected_capacities,{32768,4096}); shift.items=&items;
                    require(!items.init(),"Item initialization consumed an unpublished Player Root");
                    player_source.bind(player,player_model,nullptr);
                    require(!items.init(),"Item initialization consumed an unpublished main camera");
                    player_source.value.camera=camera;
                    native(isle_game_model_set_placement(services.game,player_model,ISLE_GAME_MODEL_PLACEMENT_DISABLED,&player_placement));
                    require(!items.init(),"Item initialization accepted a Player without current native AUX");
                    native(isle_game_model_set_placement(services.game,player_model,ISLE_GAME_MODEL_PLACEMENT_RTS,&player_placement));
                    accepted(items.init()); accepted(items.set_custom_functions(nullptr,0,nullptr));
                    require(items.matches(services,world,actors),"Item dependency identity projection rejected its actual services");
                    Consumer consumer{services,items,actors,globals};
                    if (scenario==Scenario::complete) {
                        consumer.startup_flight_pause_and_displacement(); consumer.mixed_oldest_and_iterator(); consumer.actual_waiting_splash();
                        consumer.canonical_throw_boundaries();
                    }
                    std::size_t definition=beach::actor_no_index;
                    for (std::size_t n=0;n<actor_resources.definitions().size();++n) if (actor_resources.definitions()[n].actor_id==1003) definition=n;
                    require(definition!=beach::actor_no_index,"actual common1003 definition absent");
                    world::Spawn spawn; spawn.id=1003; spawn.behavior=1; spawn.scale={1,1,1};
                    const auto actor=actors.spawn({10000,definition,&world.blocks()[0],&world.blocks()[0],&spawn}); accepted(actor.status);
                    if (scenario==Scenario::complete) consumer.kind1_actor_tail_and_real_target_retention(actor);
                    else {
                        native(isle_actor_source_pause_object(actors.source(actor.instance),1));
                        if (scenario==Scenario::physical) consumer.physical_exhaustion(actor);
                        else if (scenario==Scenario::fume) consumer.independent_fume_exhaustion();
                        else if (scenario==Scenario::threshold) consumer.true_threshold_exhaustion(actor);
                        else consumer.fault(actor,scenario);
                    }
                    const bool faulted=scenario==Scenario::fault_empty || scenario==Scenario::fault_live || scenario==Scenario::fault_collected;
                    const auto actors_retired=actors.reset();
                    if (faulted) require(!actors_retired && actors_retired.error==actor::OwnerError::services,
                                         "Actor teardown concealed the terminal Services status");
                    else accepted(actors_retired);
                    accepted(actors.collect_joined());
                    const auto items_retired=items.retire();
                    if (faulted) require(!items_retired && items_retired.error==it::OwnerError::services,
                                         "Item teardown concealed the terminal Services status");
                    else accepted(items_retired);
                    accepted(items.collect_joined());
                    require(globals.source.total==0 && globals.source.balls==0 && globals.source.apples==0,
                            "completed Item teardown retained logical family counts");
                    shift.items=nullptr;
                }
                const auto cursor=globals.source.object_counter;
                const auto cache=globals.source.payload_cache;
                if (scenario==Scenario::complete) {
                    it::Owner reconstructed(services,item_resources,world,actors,globals,player_source.value,selected_capacities,{32768,4096});
                    require(globals.source.object_counter==cursor && globals.source.payload_cache==cache && globals.source.update==Items_UpdateItemMovement,
                            "wrapper reconstruction reset caller cursor/cache/hooks");
                }
                shift.actors=nullptr;
            }
            require(world.retire(),"Item fixture World retirement failed"); environment.cleanup();
        }
        native(storage.destroy());
    }
    require(session.stop()==audio::Error::none,"actual Item audio stop failed");
    for (std::uint64_t n=1;n<=240 && session.state()!=game_audio::Session::State::draining;++n) {
        require(session.retrace(n,n*1000000000ull/60)==audio::Error::none && session.submit()==audio::Error::none,"actual Item audio retrace/PCM acceptance failed");
        audio::TaskView task; require(session.build(task)==audio::Error::none,"actual Item audio task admission failed");
        if (task.token.serial) { private_audio::execute(session,execution,task); require(session.submit()==audio::Error::none,"actual Item PCM submission failed"); }
    }
    require(session.state()==game_audio::Session::State::draining && session.poll_drain()==audio::Error::none && session.poll_drain()==audio::Error::none,
            "actual Item audio client retirement/drain failed");
}
}
int main(int argc,char** argv) {
    try {
        require(argc==3 || argc==4,"usage: item-owner-rom-smoke PRIVATE_ROM EXISTING_EMPTY_PROFILE [complete|physical|fume|threshold|fault-empty|fault-live|fault-collected]");
        const std::filesystem::path profile(argv[2]);
        require(std::filesystem::is_directory(profile) && std::filesystem::is_empty(profile),"private profile root must exist and be empty");
        auto snapshot=isle::rom::read_snapshot(argv[1]);
        constexpr std::array cases{
            std::pair{"complete",Scenario::complete},std::pair{"physical",Scenario::physical},std::pair{"fume",Scenario::fume},std::pair{"threshold",Scenario::threshold},
            std::pair{"fault-empty",Scenario::fault_empty},std::pair{"fault-live",Scenario::fault_live},std::pair{"fault-collected",Scenario::fault_collected}};
        bool selected=false;
        for (const auto& [name,scenario]:cases) {
            if (argc==4 && std::string_view(argv[3])!=name) continue;
            selected=true; const auto child=profile/name;
            require(std::filesystem::create_directory(child),"fresh case profile creation failed");
            run(snapshot,child,scenario);
            std::cout<<"native_Item_CPU_case="<<name<<" passed\n";
        }
        require(selected,"unknown private Item consumer scenario");
        if (argc==3) std::cout<<"native_Item_complete7_CPU_API=passed PlayerFSM_GPU_Original_fullgame_Windows11=not_claimed\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& error) { std::cerr<<"item-owner-rom-smoke: "<<error.what()<<'\n'; return EXIT_FAILURE; }
}
