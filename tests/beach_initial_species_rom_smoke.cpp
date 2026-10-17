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
    world::World& world;
    ApiSourceOwner& player;
    Resources& resources;
    void exercise();
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
          bindings(std::make_unique<actor::BehaviorBindings>(behavior,beach::initial_species_callbacks(),beach::initial_species_graphs(),std::span<const std::uint32_t>{},std::array<std::uint32_t,2>{0x802c7b10,0x802c82b0})) {
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
        world::World world(services,resources.world,
            world::Callbacks{&shift,nullptr,nullptr,nullptr,Shift::camera_shift,Shift::effects_shift,nullptr});
        {
            actor::Owner actors(services,resources.actors,world,environment,detector,player_source.value,resources.actor_capacities,resources.banks,{32768,4096});
            shift.actors=&actors;
            it::Globals globals;
            {
                it::Owner items(services,resources.items,world,actors,globals,player_source.value,resources.item_capacities,{32768,4096});
                shift.items=&items;
                actor::InteractionServices interactions(services,actors,items,world,detector,*resources.bindings,resources.actors);
                accepted(actors.bind_interactions(interactions)); // BEFORE any Actor factory.
                require(world.enter_first(0)!=nullptr,"first World admission failed");
                require(!player_source.value.object&&!player_source.value.model&&!actors.particle_object()&&!actors.effect_object(),
                        "species factory proof did not retain original pre-Player/pre-FX Root order");
                std::array<actor::SpawnResult,2> staged_actors{};
                std::size_t staged_count=0;
                for(const std::int32_t id:std::array{12,84}) {
                    std::size_t definition=beach::actor_no_index;
                    for(std::size_t n=0;n<resources.actors.definitions().size();++n)
                        if(resources.actors.definitions()[n].actor_id==static_cast<std::uint32_t>(id)) definition=n;
                    const auto& rows=resources.world.blocks()[0].spawns;
                    const auto row=std::find_if(rows.begin(),rows.end(),[&](const auto& x){return x.id==id;});
                    require(definition!=beach::actor_no_index&&row!=rows.end(),"pre-Player ROM species tuple absent");
                    auto staged=actors.spawn({static_cast<std::uint32_t>(9000+id),definition,&world.blocks()[0],&world.blocks()[0],&*row});
                    accepted(staged.status);
                    auto* source=actors.source(staged.instance);
                    require(staged.object&&source&&source->stateProc&&!source->pathProc&&source->counter==0&&
                            !player_source.value.object&&!player_source.value.model&&!actors.particle_object()&&!actors.effect_object(),
                            "genuine species factory required early Player/FX or executed initial state before OM");
                    staged_actors[staged_count++]=staged;
                }
                // Canonical Player publication and FX initialization happen only
                // after both genuine factories, before the first native OM tick.
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
                for(const auto& staged:staged_actors) accepted(actors.cleanup(staged.instance));
                accepted(actors.collect_joined());
                Consumer consumer{services,actors,world,player,resources};
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
void Consumer::exercise() {
    auto tick=[&] { native(isle_game_tick(services.game)); require(!services.failed(),services.error()); };
    auto no_render_owners=[&] {
        const auto bindings=actors.render_bindings();
        return std::none_of(bindings.begin(),bindings.end(),[](const auto& binding) {
            return binding.model.object!=nullptr;
        });
    };
    auto spawn=[&](std::int32_t id) {
        std::size_t def=beach::actor_no_index;
        for(std::size_t n=0;n<resources.actors.definitions().size();++n)
            if(resources.actors.definitions()[n].actor_id==static_cast<std::uint32_t>(id)) def=n;
        require(def!=beach::actor_no_index,"ROM species definition absent");
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
        try { const std::array<std::uint32_t,2> heads{0x802c7b10,0x802c82b0}; actor::BehaviorBindings missing(resources.behavior,incomplete,beach::initial_species_graphs(),{},heads); }
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
        with_graph(resources,session,state,[](Consumer& c) { c.exercise(); });
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
    std::cout<<"native_initial_species_original_C_CPU=passed Player_Root_fullgame_Windows11=not_claimed\n";
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
