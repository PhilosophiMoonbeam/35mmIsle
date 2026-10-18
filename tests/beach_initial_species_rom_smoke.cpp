#include "beach_actor_scene.hpp"
// Private ROM native initial-species consumer. Player input is component-only, not Root admission.
#include "beach_initial_species.hpp"
#include "beach_initial_species_source.h"
#include "actor_behavior_bindings.hpp"
#include "actor_interaction_services.hpp"
#include "actor_interaction_probe.h"
#include "item_owner.hpp"
#include "actual_audio_driver.hpp"
#include "actual_player_source.hpp"
#include "effect_bank_resources.hpp"
#include "scene_storage.hpp"
#include "world_movement.hpp"
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
};
struct Resources;
struct Consumer {
    scene::Services& services;
    actor::Owner& actors;
    it::Owner& items;
    world::World& world;
    ApiSourceOwner& player;
    Resources& resources;
    std::uint32_t next_id=16000;
    void tick();
    actor::SpawnResult spawn_row(std::size_t block,std::size_t row);
    actor::SpawnResult spawn_row(std::size_t block,const world::Spawn& row);
    IsleActorSource& source(const actor::SpawnResult&);
    IsleGameModelTransform placement(const actor::SpawnResult&);
    bool graph(const actor::SpawnResult&,std::uint32_t);
    bool animation(const actor::SpawnResult&,std::uint32_t);
    bool hidden(const actor::SpawnResult&);
    void place_player(IsleGameVec3);
    void collect(std::span<const actor::SpawnResult>);
    template<class Predicate>
    void until(unsigned bound,Predicate done,const char* failure) {
        for(unsigned n=0;n<bound&&!done();++n) tick();
        require(done(),failure);
    }
    void pikachu();
    void pidgey();
    void lapras();
    void sign();
    void exercise();
};
struct Resources {
    world::Resources world;
    beach::ActorSceneResources sceneData;
    actor::Resources actors;
    it::Resources items;
    actor::BehaviorResources behavior;
    environment_sound::Resources sound;
    menu::EffectResources effects;
    std::array<const menu::EffectBank*,2> banks;
    std::array<a::SlotCapacity,8> actor_capacities{};
    std::array<a::SlotCapacity,32> item_capacities{};
    std::unique_ptr<actor::BehaviorBindings> bindings;
    explicit Resources(const rom::Snapshot& snapshot)
        : world(snapshot, isle::course::beach_id),sceneData(snapshot),actors(snapshot,0,{world.setup().fog_rgb,world.setup().fog_min,world.setup().fog_max}),
          items(snapshot,world.setup().fog_rgb,world.setup().fog_min,world.setup().fog_max),
          behavior(snapshot,0),sound(snapshot,0),effects(snapshot),
          banks{&effects.bank(menu::EffectGroup::common),&effects.bank(menu::EffectGroup::beach)},
          bindings(std::make_unique<actor::BehaviorBindings>(behavior,beach::initial_species_callbacks(),beach::initial_species_graphs(),std::span<const std::uint32_t>{},beach::initial_species_roots())) {
        a::SlotCapacity ac{},ic{};
        for (const auto& model:actors.models()) { const auto c=a::slot_capacity(model); ac.nodes=std::max(ac.nodes,c.nodes); ac.material_attachments=std::max(ac.material_attachments,c.material_attachments); }
        for (const auto& model:items.models()) { const auto c=a::slot_capacity(model); ic.nodes=std::max(ic.nodes,c.nodes); ic.material_attachments=std::max(ic.material_attachments,c.material_attachments); }
        // Fixed predeclared physical capacities, independent of execution and
        // distinct from Items' actual twenty-entry logical array capacity.
        actor_capacities.fill(ac); item_capacities.fill(ic);
    }
};

template<class Body>
void with_graph(Resources& resources,game_audio::Session& audio,game::State& state,Body body) {
    game_input::Service input; scene::Storage storage(32*1024*1024); native(storage.create());
    {
        scene::Services services(storage.game(),storage.arena(),{{4,6,0,1,2},{}},input,audio,state);
        private_player::Source player_source(services);
        auto* camera_object=services.object(1,0); require(camera_object,"actual selector camera object absent");
        IsleGameCamera* camera=nullptr; native(isle_game_attach_camera(services.game,camera_object,&camera));
        const IsleGameLookAt look{{0,0,0},{1,0,0},{0,1,0}};
        native(isle_game_camera_set_look_at(services.game,camera,&look));
        std::unique_ptr<ApiSourceOwner> player_owner;
        environment_sound::Environment environment(services,storage.arena(),resources.sound);
        require(environment.initialize(nullptr,camera),"environment source initialization failed");
        focus::Detector detector; Shift shift{&services,camera};
        world::ModuleState world_module;
        world::World world(services,resources.world,world_module,
            world::Callbacks{&shift,nullptr,nullptr,nullptr,Shift::camera_shift,Shift::effects_shift,nullptr});
        {
            const auto& sceneData=resources.sceneData;
            std::uint16_t scene_latch=sceneData.initial_scalars().magikarp_active;
            beach::ActorSceneState sceneState(sceneData,scene_latch);
            actor::Owner actors(services,resources.actors,actor::CourseContext(sceneState),world,environment,detector,player_source.value,resources.actor_capacities,resources.banks,{32768,4096});
            shift.actors=&actors;
            it::Globals globals;
            {
                it::Owner items(services,resources.items,world,actors,globals,player_source.value,resources.item_capacities,{32768,4096});
                shift.items=&items;
                actor::InteractionServices interactions(services,actors,items,world,detector,*resources.bindings,resources.actors);
                accepted(actors.bind_interactions(interactions)); // BEFORE any Actor factory.
                player_source.value.movement.speed_multiplier=0.0005f;
                world::Movement movement(world,player_source.value.movement,{});
                require(movement.initialize(0),"actual first World movement admission failed");
                services.rng.seed=0x13579bdf;
                require(!player_source.value.object&&!player_source.value.model&&!actors.particle_object()&&!actors.effect_object(),
                        "species factory proof did not retain original pre-Player/pre-FX Root order");
                std::array<actor::SpawnResult,8> staged_actors{};
                // Exact ROM first eight: sign1004 is distinct from ending1001.
                // The next block's ordinary Lapras has no path.
                constexpr std::array<std::pair<std::size_t,std::size_t>,8> rows{{
                    {0,0},{0,1},{0,2},{0,3},{0,4},{0,5},{0,6},{1,0}}};
                for(std::size_t n=0;n<rows.size();++n) {
                    const auto [block,row_index]=rows[n];
                    const auto& row=resources.world.blocks()[block].spawns[row_index];
                    std::size_t definition=actor::actor_no_index;
                    for(std::size_t i=0;i<resources.actors.definitions().size();++i)
                        if(!resources.actors.definitions()[i].singleton&&
                           resources.actors.definitions()[i].actor_id==static_cast<std::uint32_t>(row.id)) definition=i;
                    require(definition!=actor::actor_no_index,"pre-Player ROM species tuple absent");
                    auto staged=actors.spawn({static_cast<std::uint32_t>(9000+n),definition,&world.blocks()[block],&world.blocks()[0],&row});
                    accepted(staged.status);
                    auto* source=actors.source(staged.instance);
                    require(staged.object&&source&&source->stateProc&&!source->pathProc&&source->counter==0&&
                            !player_source.value.object&&!player_source.value.model&&!actors.particle_object()&&!actors.effect_object(),
                            "genuine species factory required early Player/FX or executed initial state before OM");
                    staged_actors[n]=staged;
                }
                // Canonical Player publication and FX initialization happen only
                // after all eight genuine factories, before the first native OM tick.
                player_owner=std::make_unique<ApiSourceOwner>(services,2);
                auto& player=*player_owner;
                player_source.bind(player.object,player.model,camera);
                player_source.value.cam_target={100,900,300};
                require(environment.set_listener(player.model,camera),"environment listener binding failed");
                accepted(items.init()); accepted(items.set_custom_functions(nullptr,0,nullptr));
                accepted(actors.publish_effects());
                // The SAME prepublication factory actors now execute their
                // original initial callbacks, under the actual native OM.
                native(isle_game_tick(services.game));
                require(!services.failed(),services.error());
                auto* staged_butterfly=actors.source(staged_actors[0].instance);
                auto* staged_doduo=actors.source(staged_actors[1].instance);
                require(staged_butterfly&&staged_butterfly->pathProc&&staged_butterfly->transitionGraph&&
                        staged_butterfly->counter>=60&&staged_butterfly->counter<660&&
                        staged_doduo&&staged_doduo->pathProc&&staged_doduo->stateProc,
                        "prepublication actors did not execute original initial states after later Player/FX publication");
                IsleGameObjectMetadata staged_metadata{};
                native(isle_game_object_metadata(services.game,staged_actors[1].object,&staged_metadata));
                require((staged_metadata.flags&1)&&staged_doduo->tangible,
                        "staged Doduo original initial hidden/tangible semantics changed");
                auto* staged_pikachu=actors.source(staged_actors[2].instance);
                auto* staged_lapras=actors.source(staged_actors[7].instance);
                require(staged_pikachu&&staged_pikachu->behavior==4&&staged_pikachu->transitionGraph&&
                        staged_lapras&&staged_lapras->transitionGraph,
                        "prepublication Pikachu/Lapras did not enter genuine ordinary initial states");
                for(std::size_t n=4;n<7;++n)
                    require(actors.source(staged_actors[n].instance)->pathProc,
                            "prepublication Pidgey flock did not start genuine path fibers");
                auto* staged_sign=actors.source(staged_actors[3].instance);
                native(isle_game_object_metadata(services.game,staged_actors[3].object,&staged_metadata));
                require(staged_sign&&staged_sign->id==1004&&staged_sign->stateProc&&
                        !(staged_sign->flags&4u)&&(staged_metadata.flags&2),
                        "first-entry1004 did not execute its own progress state");
                for(const auto& staged:staged_actors) accepted(actors.cleanup(staged.instance));
                accepted(actors.collect_joined());
                Consumer consumer{services,actors,items,world,player,resources};
                body(consumer);
                accepted(actors.reset()); accepted(actors.collect_joined()); accepted(items.retire()); accepted(items.collect_joined());
                shift.items=nullptr;
            }
            shift.actors=nullptr;
        }
        require(player_owner->close() && !services.source_owner_registered(player_owner->object) &&
                player_owner->retiring==1 && player_owner->joined==1 && !player_owner->errors,
                "Player API semantic owner did not join/unregister exactly once after all dependent owners retired");
        require(world.retire(),"semantic World retirement failed"); environment.cleanup();
    }
    native(storage.destroy());
}
void Consumer::tick() {
    native(isle_game_tick(services.game));
    require(!services.failed(),services.error());
}
actor::SpawnResult Consumer::spawn_row(std::size_t block,std::size_t row) {
    return spawn_row(block,resources.world.blocks()[block].spawns[row]);
}
actor::SpawnResult Consumer::spawn_row(std::size_t block,const world::Spawn& row) {
    std::size_t definition=actor::actor_no_index;
    for(std::size_t n=0;n<resources.actors.definitions().size();++n)
        if(!resources.actors.definitions()[n].singleton&&
           resources.actors.definitions()[n].actor_id==static_cast<std::uint32_t>(row.id)) definition=n;
    require(definition!=actor::actor_no_index,"ordinary ROM species definition absent");
    auto result=actors.spawn({next_id++,definition,&world.blocks()[block],world.current(),&row});
    accepted(result.status);
    require(result.object&&actors.native_model(result.instance)&&actors.source(result.instance),
            "ordinary factory did not publish its real native model/source");
    const auto bindings=actors.render_bindings();
    require(std::any_of(bindings.begin(),bindings.end(),[&](const auto& binding) {
        return binding.model.object==result.object&&binding.model.model==actors.native_model(result.instance);
    }),"ordinary factory omitted native render publication");
    return result;
}
IsleActorSource& Consumer::source(const actor::SpawnResult& actor) {
    auto* value=actors.source(actor.instance);
    require(value,"live original-C Actor source absent");
    return *value;
}
IsleGameModelTransform Consumer::placement(const actor::SpawnResult& actor) {
    const IsleGameModelPlacement* value=nullptr;
    native(isle_game_model_placement(services.game,actors.native_model(actor.instance),&value));
    return value->transform;
}
bool Consumer::graph(const actor::SpawnResult& actor,std::uint32_t identity) {
    const auto* value=source(actor).transitionGraph;
    return value&&value->source_identity==identity;
}
bool Consumer::animation(const actor::SpawnResult& actor,std::uint32_t identity) {
    for(const auto& header:resources.actors.headers())
        if(header.wire.guest_address==identity) return source(actor).animHeader==&header;
    require(false,"original animation header absent from actual ROM resources");
    return false;
}
bool Consumer::hidden(const actor::SpawnResult& actor) {
    IsleGameObjectMetadata value{};
    native(isle_game_object_metadata(services.game,actor.object,&value));
    return (value.flags&1)!=0;
}
void Consumer::place_player(IsleGameVec3 position) {
    const IsleGameModelTransform value{position,{0,0,0},{1,1,1}};
    native(isle_game_model_set_placement(services.game,player.model,ISLE_GAME_MODEL_PLACEMENT_RTS,&value));
}
void Consumer::collect(std::span<const actor::SpawnResult> cohort) {
    for(const auto& actor:cohort) {
        const auto membership=actors.membership(actor.instance);
        require(!membership.logical&&!membership.registered&&membership.retiring&&membership.joined&&
                !actor_interaction_object_alive(actor.object)&&
                actor_interaction_process_count(actor.object)==0,
                "source cleanup did not unlink/join all real native processes");
    }
    accepted(actors.collect_joined());
    for(const auto& actor:cohort)
        require(!actors.source(actor.instance)&&!actors.native_model(actor.instance),
                "joined ordinary species retained its semantic/model slot");
}
void Consumer::pikachu() {
    // Pinned 562BC0.c47-60 and 563000.c92-235: ordinary behavior4,
    // its CDE30 weighted idle, actual Flute, Item approach/eat and self cmd35.
    services.rng.seed=0x2468ace1;
    const auto actor=spawn_row(0,2);
    auto& p=source(actor);
    require(p.id==25&&p.behavior==4,"ordinary Pikachu ROM tuple changed");
    place_player({20000,20000,20000});
    until(4,[&] { return graph(actor,0x802cdd80); },"Pikachu behavior4 did not enter its genuine idle graph");
    bool first_weighted=false,second_weighted=false;
    for(unsigned n=0;n<2400&&!(first_weighted&&second_weighted);++n) {
        if(animation(actor,0x802cdb70)) second_weighted=true;
        if(second_weighted&&animation(actor,0x802cdb5c)) first_weighted=true;
        tick();
    }
    require(first_weighted&&second_weighted&&graph(actor,0x802cdd80),
            "deterministic original CDE30 choices did not execute both idle animations");
    const auto idle=placement(actor).position;
    place_player({idle.x+1400,idle.y,idle.z});
    tick(); tick();
    require(items.flute().play()==audio::Error::none,"actual Item Flute start failed");
    tick(); tick();
    require(graph(actor,0x802cdd80)&&near(p.playerDist,1400),
            "Pikachu auto flute incorrectly admitted the exclusive1400 boundary");
    place_player({idle.x+100,idle.y,idle.z});
    until(8,[&] { return animation(actor,0x802cdb84)&&!p.transitionGraph&&p.pathProc; },
            "actual Flute did not select original Pikachu dance/path state");
    for(unsigned n=0;n<32;++n) tick();
    require(animation(actor,0x802cdb84)&&!p.transitionGraph&&p.pathProc&&p.interactionTarget==player.object,
            "Pikachu dance did not remain in the real flute-controlled path wait");
    require(items.flute().stop()==audio::Error::none,"actual Item Flute stop failed");
    until(600,[&] { return graph(actor,0x802cdd80)&&!p.pathProc; },
            "Pikachu did not leave genuine dance after actual Flute stopped");

    // A real throw settles through original Item physics, never a forged
    // STILL record or replacement collision provider. Its extra velocity
    // cancels the public factory's horizontal base throw.
    world::GroundResult ground{};
    const IsleGameVec3 landing{idle.x+100,0,idle.z};
    require(world.ground_at(landing.x,landing.z,ground)&&ground.normal.y>=0.866f,
            "Pikachu apple regression needs actual walkable Beach ground");
    const IsleGameVec3 origin{landing.x,ground.height+1,landing.z};
    actors.player_source().cam_target={origin.x,origin.y-100,origin.z+100};
    const auto apple=items.spawn_apple(origin,{0,0,-50}); accepted(apple.status);
    require(apple.object&&items.source(apple.instance)&&
            items.source(apple.instance)->state==ISLE_ITEM_FLYING,
            "actual apple factory did not start a flying Item");
    bool settled=false,noticed=false,approached=false,chewed=false,eaten=false;
    for(unsigned n=0;n<1200&&!eaten;++n) {
        const auto* item=items.source(apple.instance);
        if(item&&item->state==ISLE_ITEM_STILL) settled=true;
        if(p.apple==apple.object&&p.interactionTarget==apple.object&&graph(actor,0x802cde78)) noticed=true;
        if(graph(actor,0x802cdef8)&&animation(actor,0x802cdbac)&&p.pathProc) approached=true;
        if(animation(actor,0x802cdc10)&&p.counter>0&&p.counter<=17) {
            chewed=true;
            require(items.exists(apple.object),"Pikachu ate before its original17-tick chew wait");
        }
        if(chewed&&!items.exists(apple.object)) eaten=true;
        if(!eaten) tick();
    }
    require(settled&&noticed&&approached&&chewed&&eaten,
            "original Pikachu failed real Item settle/notice/approach/17-tick eat sequence");
    tick(); tick();
    require(!p.interactionTarget,"real Item death command did not clear Pikachu's canonical target");
    // Source apple storage remains a retained identity until Actor cleanup;
    // do not mutate it or demand collection while that real borrow is held.
    until(600,[&] { return graph(actor,0x802cdd80); },"Pikachu did not return from actual apple animation");
    auto moved=placement(actor);
    require(std::abs(moved.position.x-idle.x)>1||std::abs(moved.position.z-idle.z)>1,
            "original apple approach never moved Pikachu's native placement");

    const auto& path=resources.world.paths()[resources.world.blocks()[0].spawns[2].path];
    const auto endpoint=world::path_position(path,0.99999f);
    const auto world_position=world.current()->resource->metadata.world_position;
    const IsleGameVec3 destination{(endpoint.x-world_position.x)*100,
                                  (endpoint.y-world_position.y)*100,
                                  (endpoint.z-world_position.z)*100};
    moved.position={destination.x-300,destination.y,destination.z};
    accepted(actors.set_placement(actor.instance,moved));
    until(2400,[&] { return p.specialPoseID==1&&graph(actor,0x802cdfd8)&&!p.pathProc; },
            "Pikachu actual endpoint auxiliary/self35 did not complete source walk/turn/special pose");
    const auto final=placement(actor).position;
    require(std::abs(final.x-destination.x)<5&&std::abs(final.z-destination.z)<5&&
            p.interactionTarget==actor.object&&(p.flags&0xa00)==0,
            "Pikachu endpoint state lost real self-sender/root destination/flag cutover");
    accepted(actors.cleanup(actor.instance));
    collect(std::span{&actor,1});
    accepted(items.collect_joined());
    require(!items.source(apple.instance),"eaten Item semantic storage survived final canonical-borrow release");
}
void Consumer::pidgey() {
    // Pinned 560620.c9-98: all three original first-block path rows
    // pause hidden, emerge through actual Player proximity and clean themselves.
    place_player({20000,20000,20000});
    const std::array flock{spawn_row(0,4),spawn_row(0,5),spawn_row(0,6)};
    until(5,[&] {
        return std::all_of(flock.begin(),flock.end(),[&](const auto& bird) {
            return graph(bird,0x802cd040);
        });
    },"original Pidgey flock did not reach its one-tick path pause");
    std::array<float,3> paused{};
    for(std::size_t n=0;n<flock.size();++n) {
        const auto& p=source(flock[n]); paused[n]=p.pathParam;
        require(hidden(flock[n])&&p.pathProc&&(p.processFlags&POKEMON_PROCESS_FLAG_MOVEMENT_PAUSED),
                "original Pidgey path pause did not preserve hidden path fiber");
    }
    tick(); tick();
    for(std::size_t n=0;n<flock.size();++n)
        require(source(flock[n]).pathParam==paused[n],"paused original Pidgey path advanced");
    place_player(placement(flock[1]).position);
    until(8,[&] {
        return std::all_of(flock.begin(),flock.end(),[&](const auto& bird) {
            return !hidden(bird)&&graph(bird,0x802cd010)&&
                   !(source(bird).processFlags&POKEMON_PROCESS_FLAG_MOVEMENT_PAUSED);
        });
    },"actual proximity did not release all three genuine Pidgey path states");
    const auto before=placement(flock[0]).position;
    for(unsigned n=0;n<16;++n) tick();
    const auto after=placement(flock[0]).position;
    require(source(flock[0]).pathParam>paused[0]&&
            (before.x!=after.x||before.y!=after.y||before.z!=after.z),
            "released Pidgey source path did not move its real native model");
    until(6000,[&] {
        return std::none_of(flock.begin(),flock.end(),[&](const auto& bird) { return actors.registered(bird.instance); });
    },"original Pidgey flock failed path/jump/native cleanup");
    collect(flock);

    // Explicit public ObjectSpawn behavior2 exercises the pinned flock signal
    // branch; this is NOT represented as a ROM-authored behavior2 spawn row.
    auto leader_row=resources.world.blocks()[0].spawns[4]; leader_row.behavior=2;
    auto follower_row=resources.world.blocks()[0].spawns[5]; follower_row.behavior=2;
    const std::array linked{spawn_row(0,leader_row),spawn_row(0,follower_row)};
    until(5,[&] { return graph(linked[0],0x802cd0b4)&&graph(linked[1],0x802cd0b4); },
            "source behavior2 Pidgey flock did not wait in its actual hit/link31 graph");
    require(services.send_command(linked[0].object,9,nullptr),"Pidgey leader hit enqueue failed");
    until(8,[&] { return graph(linked[0],0x802cd010)&&graph(linked[1],0x802cd010); },
            "original Pidgey leader did not release its peer through actual link3 cmd31");
    require(!source(linked[0]).interactionTarget&&
            source(linked[1]).interactionTarget==linked[0].object&&
            !hidden(linked[0])&&!hidden(linked[1]),
            "Pidgey link31 replaced real leader sender identity or lost visible flock publication");
    until(6000,[&] { return !actors.registered(linked[0].instance)&&!actors.registered(linked[1].instance); },
            "linked source Pidgeys did not finish actual path/jump cleanup");
    collect(linked);

    place_player({20000,20000,20000});
    const auto delayed=spawn_row(3,3);
    until(5,[&] { return graph(delayed,0x802cd0e4); },
            "actual behavior3 Pidgey did not retain its hidden proximity wait");
    require(hidden(delayed)&&(source(delayed).processFlags&POKEMON_PROCESS_FLAG_MOVEMENT_PAUSED),
            "behavior3 Pidgey fell through the nonreturning branch into ordinary0");
    place_player(placement(delayed).position);
    until(8,[&] { return !hidden(delayed)&&graph(delayed,0x802cd010); },
            "actual behavior3 Pidgey did not emerge through its own proximity graph");
    until(6000,[&] { return !actors.registered(delayed.instance); },
            "behavior3 Pidgey failed original path/jump cleanup");
    collect(std::span{&delayed,1});

    // Both actual behavior1 ROM rows take the hit escape state, which updates
    // the scene-owned shared hit counter and emits source link3 cmd36 on hit2.
    place_player({20000,20000,20000});
    const std::array escape{spawn_row(6,2),spawn_row(6,3)};
    until(5,[&] { return graph(escape[0],0x802cd040)&&graph(escape[1],0x802cd040); },
            "behavior1 Pidgeys failed initial source path pause");
    place_player(placement(escape[0]).position);
    until(8,[&] { return graph(escape[0],0x802cd064)&&graph(escape[1],0x802cd064); },
            "actual Player did not expose both original behavior1 Pidgeys");
    for(const auto& bird:escape)
        require(services.send_command(bird.object,9,nullptr),"behavior1 Pidgey hit enqueue failed");
    until(8,[&] {
        return animation(escape[0],0x802ccfd4)&&animation(escape[1],0x802ccfd4)&&
               !source(escape[0]).transitionGraph&&!source(escape[1]).transitionGraph;
    },"original behavior1 Pidgey hit states did not replace their normal escape paths");
    require(source(escape[0]).pathProc&&source(escape[1]).pathProc&&
            !source(escape[0]).transitionGraph&&!source(escape[1]).transitionGraph,
            "hit Pidgeys retained normal interaction graph instead of real escape fibers");
    until(6000,[&] { return !actors.registered(escape[0].instance)&&!actors.registered(escape[1].instance); },
            "original behavior1 Pidgey escape fibers failed native cleanup");
    collect(escape);
}
void Consumer::lapras() {
    // Pinned 55FB80.c89-200: SetState is nonreturning. Waiting branches2/4/5
    // must not fall through to ordinary0 or bypass actual scene-shared signals.
    services.rng.seed=0x10203040;
    const std::array waiting{spawn_row(2,4),spawn_row(5,2),spawn_row(6,1)};
    for(unsigned n=0;n<8;++n) tick();
    for(const auto& actor:waiting)
        require(hidden(actor)&&!source(actor).tangible&&source(actor).stateProc&&
                !source(actor).transitionGraph&&!source(actor).pathProc&&source(actor).counter==0,
                "Lapras branch2/4/5 incorrectly fell through its genuine shared-counter wait");
    // A real FIFO24 with no installed graph cannot unlock itself. NULL means
    // no sender claim: the original game's cmd24 sender remains unresolved.
    require(services.send_command(waiting[0].object,24,nullptr),"waiting Lapras cmd24 enqueue failed");
    tick(); tick();
    for(const auto& actor:waiting)
        require(!source(actor).transitionGraph&&source(actor).counter==0,
                "waiting Lapras accepted cmd24 without its original interaction graph");
    const auto ordinary=spawn_row(1,0);
    until(4,[&] { return graph(ordinary,0x802cce1c); },"ordinary Lapras did not install genuine cmd24 graph");
    require(services.send_command(ordinary.object,24,nullptr),"ordinary Lapras cmd24 enqueue failed");
    until(8,[&] { return graph(waiting[0],0x802cce1c); },
            "ordinary behavior0 cmd24 did not increment real CCDFC and unlock behavior2");
    require(!source(ordinary).stateProc&&!source(ordinary).transitionGraph&&
            !source(waiting[1]).transitionGraph&&!source(waiting[2]).transitionGraph,
            "Lapras first shared signal failed its nonreturning state or unlocked wrong counter cohorts");
    require(services.send_command(waiting[0].object,24,nullptr),"behavior2 Lapras cmd24 enqueue failed");
    until(8,[&] { return graph(waiting[1],0x802cce1c)&&graph(waiting[2],0x802cce1c); },
            "behavior2 cmd24 did not increment actual CCE00/CCE04 and unlock branches4/5");
    require(!source(waiting[0]).stateProc&&!source(waiting[0]).transitionGraph,
            "Lapras behavior2 counter branch returned into an unrelated state");
    const std::array cohort{ordinary,waiting[0],waiting[1],waiting[2]};
    std::array<bool,4> saw_hidden{},saw_visible{};
    for(unsigned n=0;n<1800;++n) {
        for(std::size_t i=0;i<cohort.size();++i) {
            const auto& actor=cohort[i];
            const bool invisible=hidden(actor);
            if(invisible) saw_hidden[i]=true; else saw_visible[i]=true;
            require(bool(source(actor).tangible)==!invisible&&
                    (animation(actor,0x802cce08)||animation(actor,0x802ccde8)),
                    "Lapras source visibility/tangibility/actual animation publication diverged");
        }
        if(std::all_of(saw_hidden.begin(),saw_hidden.end(),[](bool v) { return v; })&&
           std::all_of(saw_visible.begin(),saw_visible.end(),[](bool v) { return v; })) break;
        tick();
    }
    require(std::all_of(saw_hidden.begin(),saw_hidden.end(),[](bool v) { return v; })&&
            std::all_of(saw_visible.begin(),saw_visible.end(),[](bool v) { return v; }),
            "Lapras real random timers/auxiliary visibility stopped after nonreturning cmd24 states");
    for(const auto& actor:cohort) accepted(actors.cleanup(actor.instance));
    collect(cohort);
    const auto late=spawn_row(6,1);
    until(4,[&] { return graph(late,0x802cce1c); },
            "scene-owned Lapras signal was incorrectly reset by source cleanup or slot reuse");
    accepted(actors.cleanup(late.instance)); collect(std::span{&late,1});
}
void Consumer::sign() {
    const auto actor=spawn_row(0,3);
    auto& progress=actors.player_source().movement;
    world::Movement movement(world,progress,{});
    tick();
    IsleGameObjectMetadata metadata{};
    native(isle_game_object_metadata(services.game,actor.object,&metadata));
    require(source(actor).id==1004&&source(actor).stateProc&&
            !(source(actor).flags&4u)&&(metadata.flags&2),
            "ordinary1004 did not wait below its real first progress threshold");
    bool eligible=false;
    unsigned steps=0;
    while(source(actor).stateProc&&steps<10000) {
        require(movement.update(),"real movement failed during sign progress");
        tick();
        require(world.current()&&world.current()->index==0,
                "sign process outlived its original first-block thresholds");
        const bool recognized=(source(actor).flags&4u)!=0;
        if(progress.move_time<0.92f)
            require(!recognized,"sign became recognizable before original progress0.92");
        else if(progress.move_time<0.99f) {
            require(recognized&&source(actor).stateProc,
                    "sign lost its original progress0.92-to0.99 recognition window");
            eligible=true;
        } else
            require(!recognized&&!source(actor).stateProc,
                    "sign failed its original progress0.99 flag clear/nonreturning state end");
        ++steps;
    }
    require(eligible&&progress.move_time>=0.99f&&!source(actor).stateProc,
            "real movement never completed both source sign progress boundaries");
    accepted(actors.cleanup(actor.instance));
    collect(std::span{&actor,1});
}


void Consumer::exercise() {
    auto tick=[&] { native(isle_game_tick(services.game)); require(!services.failed(),services.error()); };
    auto no_render_owners=[&] {
        const auto bindings=actors.render_bindings();
        return std::none_of(bindings.begin(),bindings.end(),[](const auto& binding) {
            return binding.model.object!=nullptr;
        });
    };
    auto spawn=[&](std::int32_t id) {
        std::size_t def=actor::actor_no_index;
        for(std::size_t n=0;n<resources.actors.definitions().size();++n)
            if(resources.actors.definitions()[n].actor_id==static_cast<std::uint32_t>(id)) def=n;
        require(def!=actor::actor_no_index,"ROM species definition absent");
        const auto& rows=resources.world.blocks()[0].spawns;
        const auto row=std::find_if(rows.begin(),rows.end(),[&](const auto& x){return x.id==id;});
        require(row!=rows.end()&&row->path!=world::no_index,"ROM initial species path absent");
        auto result=actors.spawn({static_cast<std::uint32_t>(10000+id),def,&world.blocks()[0],&world.blocks()[0],&*row});
        accepted(result.status); require(result.object&&actors.source(result.instance),"native factory absent"); return result;
    };
    // Missing a later transitive auxiliary rejects the entire immutable closure,
    // before native allocation, model publication, RNG or source process mutation.
    const auto seed=services.rng.seed;
    for(const auto& omitted:beach::initial_species_callbacks()) {
        auto incomplete=std::vector<actor::BehaviorCallbackBinding>(beach::initial_species_callbacks().begin(),beach::initial_species_callbacks().end());
        incomplete.erase(std::remove_if(incomplete.begin(),incomplete.end(),[&](auto& x){return x.identity==omitted.identity;}),incomplete.end());
        bool rejected=false;
        try { actor::BehaviorBindings missing(resources.behavior,incomplete,beach::initial_species_graphs(),{},beach::initial_species_roots()); }
        catch(const std::exception&) { rejected=true; }
        if (!rejected)
            std::cerr<<"omitted native callback accepted: 0x"<<std::hex<<omitted.identity<<std::dec<<'\n';
        require(rejected,"immutable source closure admitted a missing callback");
        require(services.rng.seed==seed&&no_render_owners(),
                "missing transitive source closure mutated native ownership");
    }
    auto butterfly=spawn(12); auto* b=actors.source(butterfly.instance);
    const IsleGameModelPlacement* placement=nullptr;
    native(isle_game_model_placement(services.game,actors.native_model(butterfly.instance),&placement));
    const float late_y=placement->transform.position.y;
    const auto& butterfly_row=resources.world.blocks()[0].spawns[0];
    require(butterfly_row.id==12&&near(late_y,butterfly_row.position.y*100.0f),"ordinary factory did not apply exact late AUX-Y");
    world::GroundResult ground{};
    require(world.ground_at(b->pos1.x,b->pos1.z,ground)&&near(b->pos1.y,ground.height),
            "late ordinary AUX overwrite incorrectly replaced ground source reset position");
    require(b->counter==0&&b->pathProc==nullptr&&b->stateProc,"factory ran source initial state early");
    tick();
    require(near(b->miscVars[0].field0,late_y)&&b->pathProc&&b->counter>=60&&b->counter<660&&b->transitionGraph,
            "original Butterfree initial state/random wait/path did not execute");
    const float path_before=b->pathParam;
    tick(); require(b->pathParam!=path_before,"original random airborne path did not advance");
    require(services.send_command(butterfly.object,8,nullptr),"actual hit enqueue failed"); tick();
    bool jumped=false,fell=false,returned=false;
    for(unsigned n=0;n<1600;++n) {
        if(b->jumpVel==300.0f||b->jumpVel>100.0f) jumped=true;
        if(jumped&&b->jumpVel==90.0f) fell=true;
        if(fell&&b->transitionGraph&&(b->processFlags&POKEMON_PROCESS_FLAG_AUX_ENDED)&&b->counter>=1) { returned=true; break; }
        tick();
    }
    require(jumped&&fell&&returned,"real queued hit failed original Jump/Fall auxiliary/wait return");
    // The source fnUpdate keeps draining FIFO while every attached process is paused.
    require(services.send_command(butterfly.object,-2,nullptr),"pause enqueue failed"); tick();
    const auto process_count=actor_interaction_process_count(butterfly.object);
    require(process_count>0&&actor_interaction_paused_count(butterfly.object)==process_count,
            "actual FIFO pause did not suspend all native attached processes");
    const auto state_before=b->stateProc;
    const auto path_before_pause=b->pathParam;
    tick();
    require(b->stateProc==state_before&&b->pathParam==path_before_pause&&
            actor_interaction_paused_count(butterfly.object)==process_count,
            "paused native source state/path process advanced");
    require(services.send_command(butterfly.object,-3,nullptr),"resume enqueue failed"); tick(); tick();
    require(actor_interaction_paused_count(butterfly.object)==0&&
            actor_interaction_process_count(butterfly.object)>0,
            "actual source fnUpdate did not drain FIFO resume for native processes");
    accepted(actors.cleanup(butterfly.instance)); accepted(actors.collect_joined());
    auto doduo=spawn(84); auto* d=actors.source(doduo.instance);
    // Far outside cmd16 proximity, within native signed16.16 placement range.
    IsleGameModelTransform far{{20000,20000,20000},{0,0,0},{1,1,1}};
    native(isle_game_model_set_placement(services.game,player.model,ISLE_GAME_MODEL_PLACEMENT_RTS,&far));
    tick(); tick();
    IsleGameObjectMetadata metadata{}; native(isle_game_object_metadata(services.game,doduo.object,&metadata));
    require((metadata.flags&1)&&d->tangible&&d->transitionGraph&&d->pathProc,"Doduo initial hidden tangible wait/path missing");
    native(isle_game_model_placement(services.game,actors.native_model(doduo.instance),&placement));
    far.position=placement->transform.position;
    native(isle_game_model_set_placement(services.game,player.model,ISLE_GAME_MODEL_PLACEMENT_RTS,&far));
    tick(); tick(); native(isle_game_object_metadata(services.game,doduo.object,&metadata));
    require(metadata.flags==0&&(d->flags&0x200)&&d->interactionTarget==player.object&&d->interactionDist<1000,
            "actual proximity cmd16 did not emerge via original C with corrected distance");
    require(services.send_command(doduo.object,9,nullptr),"Doduo hit enqueue failed"); tick();
    bool waited=false,cleaned=false;
    for(unsigned n=0;n<6000;++n) {
        if(d->counter>0&&d->counter<=120) waited=true;
        if(!actors.registered(doduo.instance)) { cleaned=true; break; }
        tick();
    }
    require(waited&&cleaned,"original Doduo hit timer/path cleanup did not complete");
    accepted(actors.collect_joined());
    require(!actors.source(doduo.instance)&&no_render_owners(),"joined species semantic/model storage retained");
    auto reused=spawn(12); require(reused.object,"joined semantic capacity not reusable");
    accepted(actors.cleanup(reused.instance)); accepted(actors.collect_joined());
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
        with_graph(resources,session,state,[](Consumer& c) { c.exercise(); c.pikachu(); c.pidgey(); c.lapras(); });
        // Reconstruct the real scene, not its canonical counters. The repeated
        // initial waits reject accidental process-global Lapras signal storage.
        with_graph(resources,session,state,[](Consumer& c) { c.lapras(); });
        with_graph(resources,session,state,[](Consumer& c) { c.sign(); });
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
            session.poll_drain()==audio::Error::none && capture.finishes==1,
            "actual selector audio client retirement/actual RSP owned PCM drain failed");
    std::cout<<"native_initial_species_original_C_CPU=passed first_entry_source_factories=8 audio_tasks="<<audio_tasks
             <<" Player_Root_fullgame_Windows11=not_claimed\n";
}
}
int main(int argc,char** argv) {
    try {
        require(argc==2||argc==3,"usage: beach-initial-species-rom-smoke ROM [private-profile-dir]");
        auto snapshot=rom::read_snapshot(argv[1]);
        const auto profile=argc==3?std::filesystem::path(argv[2]):std::filesystem::temp_directory_path()/"35mmisle-initial-species-private";
        if(argc==2) require(std::filesystem::create_directory(profile),"default private profile already exists; provide a new empty directory");
        run(snapshot,profile); return EXIT_SUCCESS;
    } catch(const std::exception& e) { std::cerr<<"beach-initial-species-rom-smoke: "<<e.what()<<'\n'; return EXIT_FAILURE; }
}
