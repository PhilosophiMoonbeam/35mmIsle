#include "actual_actor_world_hooks.hpp"
// Separate opt-in native physics API/canonical GPU consumer. Three genuine
// factory actors have explicitly authored collision flags/radii/list inputs;
// this does not admit Magikarp's distinct nonNULL-graph collision state.
#include "actor_photo_scene.hpp"
#include "actual_audio_driver.hpp"
#include "course_model_producer.hpp"
#include "effect_bank_resources.hpp"
#include "photo_lighting.hpp"
#include "scene_graphics.hpp"
#include "scene_storage.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>

namespace {
using namespace isle;
namespace a=menu::animation;
namespace g=scene::graphics;
constexpr unsigned width=320,height=240;
constexpr std::uint32_t plane=width*height*2;
constexpr menu::RamRange joined_lists{0x686000,0x100},guard{0x7f0000,256};
constexpr world::ProducerLayout world_layout{{0x200000,0x10000},{0x210000,0x10000},{0x220000,0x10000}};
constexpr actor::ProducerLayout actor_layout{{0x400000,0x80000},{0x480000,0x80000},{0x500000,0x100000},{{{0x3b5000,plane},{0x3da800,plane}}}};
constexpr focus::CaptureLayout capture_layout{{0x680000,0x50},{0x680080,0x80},{0x680100,0xa00}};
constexpr g::Layout frame_layout{{0x626000,0x2000},{0x630000,0x10000},{0x640000,0x40000},0x600000};
void require(bool value,const char* detail) { if(!value)throw std::runtime_error(detail); }
void native(IsleGameResult result) { require(result==ISLE_GAME_OK,"native collision GPU operation failed"); }
void accepted(actor::OwnerStatus result) { require(bool(result),result.detail); }
std::uint32_t physical(std::uint32_t value) { return value&0x1fffffffu; }
bool near(float a,float b) { return std::fabs(a-b)<0.0001f; }
bool same_focus(const focus::Focus& a,const focus::Focus& b) {
    return a.object==b.object&&a.pokemon_id==b.pokemon_id&&a.flags==b.flags&&a.present==b.present&&a.context==b.context;
}
using Hooks=private_actor::WorldHooks;
using WorldLifetime=private_actor::WorldLifetime;
struct ModelPass {
    course::ModelProducer& models;focus::RegionCopies& copies;const photo::LightingInputs& lighting;
    IsleGame* game;IsleGameCamera* camera;std::uint64_t epoch=0;const std::uint32_t* memory=nullptr;
    static bool prepare(void* user,std::span<std::uint32_t> words,std::uint64_t generation,const scene::render::Pass& pass,scene::render::Prepared& output) {
        auto& self=*static_cast<ModelPass*>(user);
        if(self.epoch!=generation||self.memory!=words.data()) {
            if(self.copies.install(words)!=focus::CaptureStatus::ok)return false;
            self.lighting.install(std::as_writable_bytes(words));std::fill_n(words.begin()+guard.offset/4,guard.size/4,0x13579bdfu);
            self.epoch=generation;self.memory=words.data();
        }
        const IsleGameCameraParameters* camera=nullptr;if(isle_game_camera_parameters(self.game,self.camera,&camera)!=ISLE_GAME_OK)return false;
        const auto& eye=camera->look_at.eye;const auto& at=camera->look_at.at;
        self.lighting.update_direction(std::as_writable_bytes(words),{eye[0],eye[1],eye[2]},{at[0],at[1],at[2]});
        output=self.models.prepare(words,generation,pass);return true;
    }
};
struct Capture {
    std::vector<std::uint16_t> color=std::vector<std::uint16_t>(width*height);
    std::vector<std::uint16_t> depth=std::vector<std::uint16_t>(width*height);
};
struct Readback {
    actor::DetectorBinding& binding;Capture* target=nullptr;unsigned completions=0;
    static void finish(void* user,rsp::CanonicalExecutor::MemoryLease& lease,std::uint32_t framebuffer) {
        auto& self=*static_cast<Readback*>(user);require(self.target&&++self.completions==1,"collision frame lost exclusive completed lease");
        const auto bytes=lease.rdram();
        const std::span words(reinterpret_cast<const std::uint32_t*>(bytes.data()),bytes.size()/4);
        const auto pixel=[&](std::uint32_t at) {return static_cast<std::uint16_t>((std::to_integer<unsigned>(bytes[at^3u])<<8)|std::to_integer<unsigned>(bytes[(at+1)^3u]));};
        require(self.binding.history.analyze(self.binding.context,words,self.binding.copies.region_offset(0))==focus::Error::none,
                "actual collision source history analysis failed");
        for(std::size_t i=0;i<self.target->color.size();++i) {
            self.target->color[i]=pixel(framebuffer+static_cast<std::uint32_t>(i*2));
            self.target->depth[i]=pixel(frame_layout.depth+static_cast<std::uint32_t>(i*2));
        }
        for(std::size_t i=guard.offset/4;i<(guard.offset+guard.size)/4;++i)require(words[i]==0x13579bdfu,"collision GPU crossed disjoint guard");
        require(!lease.postprocess_active(),"collision proof fabricated a writable/readback fence");
    }
};
void save(const std::filesystem::path& path,g::Completed image) {
    require(bool(image),"collision completed surface absent");std::ofstream out(path,std::ios::binary);
    require(bool(out),"collision surface open failed");out<<"P6\n"<<image.width<<' '<<image.height<<"\n255\n";
    for(std::size_t i=0;i<image.rgba.size();i+=4)out.write(reinterpret_cast<const char*>(image.rgba.data()+i),3);
    require(bool(out),"collision surface write failed");
}
void run(rom::Snapshot& snapshot,const std::filesystem::path& profile,const std::filesystem::path& output) {
    require(std::filesystem::is_directory(profile)&&std::filesystem::is_empty(profile),"collision GPU profile must exist and be empty");
    std::filesystem::create_directories(output);
    const world::Resources world_resources(snapshot);const auto& setup=world_resources.setup();
    const beach::ActorResources resources(snapshot,{setup.fog_rgb,setup.fog_min,setup.fog_max});
    const environment_sound::BeachResources sounds(snapshot);const menu::EffectResources effects_data(snapshot);const photo::LightingInputs lighting(snapshot);
    const std::array banks{&effects_data.bank(menu::EffectGroup::common),&effects_data.bank(menu::EffectGroup::beach)};
    auto hydrated=game_audio::hydrate(snapshot);require(bool(hydrated),"collision GPU audio hydrate failed");
    std::array<audio::effects::Section,8> sections;audio::effects::Config effects;
    require(game_audio::native_startup_effects(hydrated.resources,sections,effects)==audio::Error::none,"collision GPU audio effects failed");
    private_audio::Capture pcm;private_audio::Execution execution{snapshot,nullptr,{}};pcm.samples.resize(8192*audio::AudioService::max_frames*2);
    auto made=game_audio::Session::create(std::move(hydrated.resources),effects,pcm.output(),execution.executor());
    require(bool(made),"collision GPU actual audioSession failed");auto& session=*made.session;std::uint64_t retrace=0;
    const auto pump=[&] {
        ++retrace;require(session.retrace(retrace,retrace*1000000000ull/60)==audio::Error::none,"collision GPU audio retrace failed");
        require(session.submit()==audio::Error::none,"collision GPU PCM submit failed");audio::TaskView task;
        require(session.build(task)==audio::Error::none,"collision GPU audio task failed");
        if(task.token.serial) {private_audio::execute(session,execution,task);require(session.submit()==audio::Error::none,"collision GPU completed PCM failed");}
    };
    unsigned draws=0,ticks=0;std::size_t changed_after_collision=0,changed_when_hidden=0;
    game::State state(profile);require(state.initialize(1),"collision GPU state initialize failed");game_input::Service input;
    scene::Storage storage(32*1024*1024);native(storage.create());
    {
        scene::Services services(storage.game(),storage.arena(),{{4,6,0,1,2},{}},input,session,state);require(!services.failed(),services.error());
        auto* root=services.object(0,0);auto* camera_object=services.object(1,0);auto* player=services.object(0x1001,0);require(root&&camera_object&&player,"collision GPU native inputs failed");
        IsleGameCamera* camera=nullptr;native(isle_game_attach_camera(services.game,camera_object,&camera));
        native(isle_game_camera_add_matrix(services.game,camera,ISLE_GAME_CAMERA_PERSPECTIVE_FAST));native(isle_game_camera_add_matrix(services.game,camera,ISLE_GAME_CAMERA_LOOK_AT_REFLECT_ROLL));
        const IsleGamePerspective perspective{55,4.0f/3.0f,10,25600,1};native(isle_game_camera_set_perspective(services.game,camera,&perspective));
        const IsleGameLookAt view{{0,1600,6000},{0,500,0},{0,1,0}};native(isle_game_camera_set_look_at(services.game,camera,&view));
        const IsleGameViewport viewport{{640,480,511,0},{640,480,511,0}};native(isle_game_camera_set_viewport(services.game,camera,&viewport));
        IsleGameModelNodeInit node{};node.parent=UINT32_MAX;node.transform.scale={1,1,1};node.matrix_kind=ISLE_GAME_MODEL_CONTEXTUAL_RPY;
        IsleGameModel* player_model=nullptr;native(isle_game_attach_model(services.game,player,&node,1,&player_model));
        IsleGameModelTransform player_pose{};player_pose.position={0,1600,6000};player_pose.scale={1,1,1};native(isle_game_model_set_placement(services.game,player_model,ISLE_GAME_MODEL_PLACEMENT_RT,&player_pose));
        environment_sound::Environment environment(services,storage.arena(),sounds,nullptr,camera);focus::Detector detector;focus::RegionCopies copies(capture_layout);actor::DetectorBinding detection{copies,detector};
        Hooks hooks{services,camera};world::World world(services,world_resources,hooks.callbacks());world.follow_camera(camera);
        a::SlotCapacity maximum{};for(const auto& model:resources.models()) {const auto capacity=a::slot_capacity(model);maximum.nodes=std::max(maximum.nodes,capacity.nodes);maximum.material_attachments=std::max(maximum.material_attachments,capacity.material_attachments);}
        const std::array capacities{maximum,maximum,maximum};
        {
            actor::Owner owner(services,resources,world,environment,detector,capacities,banks,{32768,4096});hooks.owner=&owner;WorldLifetime lifetime{world,hooks};accepted(owner.set_player(player,player_model,camera));
            actor::PhotoScene controller(services,owner,root,camera,detection);require(world.enter_first(0)==&world.blocks()[0],"collision GPU first actual World absent");
            std::size_t definition=beach::actor_no_index;for(std::size_t i=0;i<resources.definitions().size();++i)if(resources.definitions()[i].actor_id==129&&resources.definitions()[i].factory_identity==0x802cac48u)definition=i;
            require(definition!=beach::actor_no_index,"collision GPU genuine factory absent");
            std::array<actor::SpawnResult,3> actors;
            constexpr std::array<float,3> initial_z{4000,4100,4175};
            static constexpr std::array<float,1> allowed{0};
            for(unsigned i=0;i<3;++i) {
                world::Spawn spawn{};spawn.id=129;spawn.scale={1,1,1};spawn.path=world::no_index;
                actors[i]=owner.spawn({0x7000u+i,definition,world.current(),world.current(),&spawn});accepted(actors[i].status);
                const IsleGameModelPlacement* old=nullptr;native(isle_game_model_placement(services.game,owner.native_model(actors[i].instance),&old));
                auto pose=old->transform;pose.position={0,1233,initial_z[i]};accepted(owner.set_placement(actors[i].instance,pose));
                auto* source=owner.source(actors[i].instance);source->collisionOffset={};source->collisionRadius=100;
                source->flags=static_cast<std::uint16_t>(source->flags&~std::uint16_t{1});source->forbiddenGround=allowed.data();source->forbiddenGroundCount=allowed.size();
            }
            std::array<actor::ActorRenderBinding,3> bindings{};
            auto created=g::Frame::create(snapshot,{true,{16000000,4*1024*1024},frame_layout});require(bool(created),created.message.data());
            std::vector<menu::RamRange> mutable_ranges{world_layout.payloads,world_layout.lists,world_layout.materials,actor_layout.payloads,actor_layout.lists,actor_layout.materials,joined_lists,guard};
            for(const auto range:copies.reservations())mutable_ranges.push_back(range);
            for(const auto& span:lighting.installation_spans())mutable_ranges.push_back({physical(span.guest_address),span.bytes.size()});
            auto all=mutable_ranges;for(const auto range:created.frame->reservations())if(range.address)all.push_back({range.address,range.size});
            for(const auto range:actor_layout.framebuffers)all.push_back(range);
            const auto other=[&](std::array<menu::RamRange,3> own,bool framebuffers) {
                std::vector<menu::RamRange> result;for(const auto range:all) {bool skip=false;for(const auto mine:own)skip|=range.offset==mine.offset&&range.size==mine.size;
                    if(framebuffers)for(const auto fb:actor_layout.framebuffers)skip|=range.offset==fb.offset&&range.size==fb.size;
                    if(!skip)result.push_back(range);
                }
                return result;
            };
            auto world_other=other({world_layout.payloads,world_layout.lists,world_layout.materials},false),actor_other=other({actor_layout.payloads,actor_layout.lists,actor_layout.materials},true);
            menu::model::CameraBasisState basis;world::Producer world_producer(services.game,camera,basis,world_resources,world,world_layout,world_other);
            actor::Producer actor_producer(services.game,camera,basis,resources,capacities,bindings,actor_layout,actor_other,&detection);
            course::ModelProducer models(world_producer,actor_producer,joined_lists);ModelPass model_pass{models,copies,lighting,services.game,camera};
            std::vector<sprite::AssetSpan> immutable;const auto append=[&](auto spans){for(const auto& span:spans)immutable.push_back({physical(span.guest_address),{reinterpret_cast<const std::uint8_t*>(span.bytes.data()),span.bytes.size()}});};
            append(world_resources.installation_spans());append(resources.installation_spans());std::vector<sprite::RamRange> reservations;
            for(const auto range:mutable_ranges)reservations.push_back({static_cast<std::uint32_t>(range.offset),static_cast<std::uint32_t>(range.size)});
            const auto rgba=(std::uint32_t(setup.background_rgb[0])<<24)|(std::uint32_t(setup.background_rgb[1])<<16)|(std::uint32_t(setup.background_rgb[2])<<8)|255u;
            scene::render::Pass pass;pass.camera=camera;pass.priority=5;pass.flags=3;pass.background_rgba=rgba;
            pass.link_mask=(std::uint64_t{1}<<3)|(std::uint64_t{1}<<4)|(std::uint64_t{1}<<5);pass.camera_tag=UINT32_MAX;pass.prepare=ModelPass::prepare;pass.userdata=&model_pass;
            scene::render::Input scene_input{services.game,{1,nullptr,immutable,reservations,{0x3b5000,0x3da800},physical(lighting.pre_render_guest())},std::span(&pass,1),{}};
            Readback readback{detection};Capture before,after,repeat,hidden;auto frame=std::move(created.frame);
            const auto tick=[&] {require(frame->quiescent(),"native collision tick crossed live GPU lease");const auto previous=detector.computed();native(isle_game_tick(services.game));++ticks;
                require(!services.failed()&&same_focus(previous,detector.published()),"collision tick lost source phase/CopyInfo ordering");accepted(owner.collect_joined());pump();};
            const auto draw=[&](Capture& target,const char* name) {
                require(frame->quiescent(),"collision draw overlapped native/graphics state");const auto current=owner.render_bindings();require(current.size()==bindings.size(),"collision source slot plan changed");std::copy(current.begin(),current.end(),bindings.begin());
                const auto published=detector.published();readback.target=&target;readback.completions=0;require(frame->begin(scene_input,{draws&1u},{&readback,Readback::finish}),frame->message());
                while(frame->poll()==g::Poll::pending)std::this_thread::yield();
                require(frame->error()==g::Error::none&&frame->quiescent()&&readback.completions==1,frame->message());
                require(same_focus(published,detector.published()),"collision readback bypassed native CopyInfo");if(name)save(output/name,frame->completed());++draws;
            };
            tick(); // Genuine source state7/animation3 and native distance8 run once.
            for(const auto& actor:actors) {auto* source=owner.source(actor.instance);require(source&&std::isfinite(source->playerDist)&&source->playerDist<10000&&source->transitionGraph==nullptr,
                    "collision diagnostic warmup did not establish real native player distance/NULL graph");native(isle_actor_source_pause_object(source,1));}
            tick();draw(before,"actual-collision-before.ppm");
            for(const auto& actor:actors)owner.source(actor.instance)->flags=static_cast<std::uint16_t>(owner.source(actor.instance)->flags|0xa00u);
            tick();
            for(unsigned i=0;i<3;++i) {
                const auto* source=owner.source(actors[i].instance);const IsleGameModelPlacement* pose=nullptr;native(isle_game_model_placement(services.game,owner.native_model(actors[i].instance),&pose));
                world::GroundResult ground{};require(world.ground_at(pose->transform.position.x,pose->transform.position.z,ground),"actual collision landed outside source terrain");
                require(source&&(source->processFlags&0x80u)&&source->transitionGraph==nullptr&&near(source->collPosition.x,pose->transform.position.x)&&near(source->collPosition.y,pose->transform.position.y)&&near(source->collPosition.z,pose->transform.position.z)&&near(pose->transform.position.y,ground.height),
                    "actual collision2 failed real ground/canonical position/notification contract");
                if(i==0)require(near(pose->transform.position.z,3937.5f),"actual source cached-outer first endpoint displacement changed");
                std::cout<<"collision_native_actor="<<i<<" AUX="<<pose->transform.position.x<<','<<pose->transform.position.y<<','<<pose->transform.position.z<<'\n';
            }
            draw(after,"actual-collision-after.ppm");draw(repeat,nullptr);require(after.color==repeat.color&&after.depth==repeat.depth,"same completed collision state failed canonical repeat");
            IsleGameObjectMetadata flags{};native(isle_game_object_metadata(services.game,actors[0].object,&flags));native(isle_game_object_set_flags(services.game,actors[0].object,flags.flags|1u));
            draw(hidden,"actual-collision-hidden.ppm");native(isle_game_object_set_flags(services.game,actors[0].object,flags.flags));draw(repeat,nullptr);
            require(after.color==repeat.color&&after.depth==repeat.depth,"actual collision source hide/reveal failed exact completed restoration");
            for(std::size_t i=0;i<after.color.size();++i) {changed_after_collision+=(before.color[i]>>1)!=(after.color[i]>>1);changed_when_hidden+=(after.color[i]>>1)!=(hidden.color[i]>>1);}
            accepted(owner.reset());accepted(owner.collect_joined());tick();draw(repeat,"actual-collision-retired.ppm");
            require(frame->retire()&&frame->quiescent(),"actual collision GPU failed retirement");actor_producer.retire();world_producer.retire();controller.retire();accepted(owner.set_player(nullptr,nullptr,nullptr));
        }
        environment.cleanup();
    }
    IsleGameCounts counts{};native(isle_game_counts(storage.game(),&counts));require(!counts.objects&&!counts.processes&&!counts.models&&!counts.model_nodes&&!counts.cameras&&!counts.matrices,"actual collision GPU retained native ownership");native(storage.destroy());
    require(session.stop()==audio::Error::none,"actual collision GPU audio stop failed");for(unsigned n=0;n<240&&session.state()!=game_audio::Session::State::draining;++n)pump();
    require(session.state()==game_audio::Session::State::draining&&session.service().voices_retired()&&session.poll_drain()==audio::Error::none&&session.poll_drain()==audio::Error::none&&session.state()==game_audio::Session::State::drained,"actual collision owned audio failed checked drain");
    std::cout<<"actual_native_pair_ground_canonical_GPU=passed frames="<<draws<<" native_updates="<<ticks<<" changed_RGB5_after_tick="<<changed_after_collision<<" changed_RGB5_hide="<<changed_when_hidden
        <<" zero_visible_change_is_not_tuned_into_positive=true authored_physics_inputs_not_source_graph_state=true Original_fullgame_Windows11=not_claimed\n";
}
}
int main(int argc,char** argv) {
    try {require(argc==4,"usage: course-collision-rsp-smoke PRIVATE_ROM EXISTING_EMPTY_PRIVATE_PROFILE OUTPUT_DIRECTORY");auto snapshot=isle::rom::read_snapshot(argv[1]);run(snapshot,argv[2],argv[3]);return 0;}
    catch(const std::exception& error){std::cerr<<"course-collision-rsp-smoke: "<<error.what()<<'\n';return 1;}
}
