// Private original-ROM component consumer, not the game entrypoint.
// Frozen legacy camera/limits and native trigger inputs; source helper, native
// distance/position processes, production membership and native CopyInfo only.
#include "actor_photo_scene.hpp"
#include "beach_magikarp_scene.hpp"
#include "actor_movement_probe.h"
#include "actual_audio_driver.hpp"
#include "actual_player_source.hpp"
#include "course_model_producer.hpp"
#include "effect_bank_resources.hpp"
#include "effect_graphics_resources.hpp"
#include "photo_lighting.hpp"
#include "scene_graphics.hpp"
#include "scene_storage.hpp"
#include "source_effect_camera.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

namespace {
using namespace isle;
namespace a=menu::animation;
namespace g=scene::graphics;
constexpr std::uint32_t width=320,height=240,plane=width*height*2;
constexpr menu::RamRange fx_commands{0x680c00,0x5400},joined_lists{0x686000,0x100},guard{0x7f0000,256};
constexpr world::ProducerLayout world_layout{{0x200000,0x10000},{0x210000,0x10000},{0x220000,0x10000}};
constexpr actor::ProducerLayout actor_layout{{0x400000,0x80000},{0x480000,0x80000},{0x500000,0x100000},
    {{{0x3b5000,plane},{0x3da800,plane}}}};
constexpr focus::CaptureLayout capture_layout{{0x680000,0x50},{0x680080,0x80},{0x680100,0xa00}};
constexpr g::Layout frame_layout{{0x626000,0x2000},{0x630000,0x10000},{0x640000,0x40000},0x600000};
constexpr std::array<unsigned,12> checkpoints{1,2,3,9,10,11,16,32,64,128,256,512};
constexpr IsleGameVec3 trigger_point{0,1233,4000}; // Existing diagnostic endpoint, never derived from the optical axis.
constexpr IsleGameVec3 near_player{0,1600,6000},far_player{0,1600,-20000};
void require(bool value,const char* text) { if(!value) throw std::runtime_error(text); }
void native(IsleGameResult result) { require(result==ISLE_GAME_OK,"native source-focus operation failed"); }
void accepted(actor::OwnerStatus status) { require(bool(status),status.detail); }
std::uint32_t physical(std::uint32_t address) { return address&0x1fffffffu; }
bool near(float actual,float expected) { return std::fabs(actual-expected)<0.0001f; }
bool same_focus(const focus::Focus& x,const focus::Focus& y) {
    return x.object==y.object&&x.pokemon_id==y.pokemon_id&&x.flags==y.flags&&x.present==y.present&&x.context==y.context;
}
std::uint64_t fingerprint(std::span<const std::byte> bytes) {
    std::uint64_t value=14695981039346656037ull;
    for(const auto byte:bytes) value=(value^std::to_integer<unsigned char>(byte))*1099511628211ull;
    return value;
}
struct WorldHooks {
    scene::Services& services;
    IsleGameCamera* camera;
    actor::Owner* owner=nullptr;
    static void remove(void* user,world::Block& block) noexcept {
        auto& self=*static_cast<WorldHooks*>(user);
        if(!self.owner||!self.owner->remove_block(block)) self.services.fail("source-focus World removal failed");
    }
    static void camera_shift(void* user,world::Vec3 delta) noexcept {
        auto& self=*static_cast<WorldHooks*>(user);
        const IsleGameCameraParameters* current=nullptr;
        if(!self.services.check(isle_game_camera_parameters(self.services.game,self.camera,&current),"source-focus camera borrow")) return;
        auto view=current->look_at;
        const std::array offset{delta.x,delta.y,delta.z};
        for(unsigned i=0;i<3;++i) { view.eye[i]+=offset[i];view.at[i]+=offset[i]; }
        self.services.check(isle_game_camera_set_look_at(self.services.game,self.camera,&view),"source-focus camera shift");
    }
    static void effects_shift(void* user,world::Vec3 delta) noexcept {
        auto& self=*static_cast<WorldHooks*>(user);
        if(!self.owner) { self.services.fail("source-focus FX owner absent");return; }
        self.owner->effects().translate({delta.x,delta.y,delta.z});
        if(!self.owner->effects().status()) self.services.fail("source-focus FX translation failed");
    }
};
struct ProcessLifetime {
    scene::Services& services;
    IsleGameProcess* process;
    void finish() noexcept {
        if(process) { services.end_process(process);process=nullptr; }
    }
    ~ProcessLifetime() { finish(); }
};
struct WorldLifetime {
    world::World& world;
    WorldHooks& hooks;
    ~WorldLifetime() { world.retire();hooks.owner=nullptr; }
};
struct ModelPass {
    course::ModelProducer& models;
    focus::RegionCopies& copies;
    const photo::LightingInputs& lighting;
    IsleGame* game;
    IsleGameCamera* camera;
    std::uint64_t epoch=0;
    const std::uint32_t* memory=nullptr;
    static bool prepare(void* user,std::span<std::uint32_t> words,std::uint64_t generation,
                        const scene::render::Pass& pass,scene::render::Prepared& output) {
        auto& self=*static_cast<ModelPass*>(user);
        if(self.epoch!=generation||self.memory!=words.data()) {
            if(self.copies.install(words)!=focus::CaptureStatus::ok) return false;
            self.lighting.install(std::as_writable_bytes(words));
            std::fill_n(words.begin()+guard.offset/4,guard.size/4,0x13579bdfu);
            self.epoch=generation;self.memory=words.data();
        }
        const IsleGameCameraParameters* camera=nullptr;
        if(isle_game_camera_parameters(self.game,self.camera,&camera)!=ISLE_GAME_OK) return false;
        const auto& eye=camera->look_at.eye;const auto& at=camera->look_at.at;
        self.lighting.update_direction(std::as_writable_bytes(words),{eye[0],eye[1],eye[2]},{at[0],at[1],at[2]});
        output=self.models.prepare(words,generation,pass);
        return true;
    }
};
struct EffectPass {
    IsleGame* game;
    IsleGameCamera* camera;
    menu::EffectRuntime& effects;
    const menu::EffectGraphicsResources& resources;
    static bool prepare(void* user,std::span<std::uint32_t> words,std::uint64_t,
                        const scene::render::Pass&,scene::render::Prepared& output) {
        auto& self=*static_cast<EffectPass*>(user);
        menu::EffectCamera camera;
        if(!menu::source_effect_camera(self.game,self.camera,camera)) return false;
        const auto result=self.effects.render(camera,{words.subspan(fx_commands.offset/4,menu::EffectRuntime::max_render_words),
            self.resources.bindings(),self.resources.reservations()});
        if(!result.status||(result.words&1u)||result.words>menu::EffectRuntime::max_render_words) return false;
        words[fx_commands.offset/4+result.words]=0xdf000000;words[fx_commands.offset/4+result.words+1]=0;
        output.lanes[0]={static_cast<std::uint32_t>(fx_commands.offset),result.words+2};
        return true;
    }
};
struct Capture {
    std::vector<std::uint16_t> pixels=std::vector<std::uint16_t>(width*height);
    std::vector<std::uint16_t> depth=std::vector<std::uint16_t>(width*height);
    std::array<unsigned,focus::history_capacity> full16{},rgb5{};
    focus::Focus computed{};
    bool marked=false;
    unsigned red_positions=0,changed_mark_pixels=0;
};
struct Readback {
    actor::DetectorBinding& detection;
    actor::Owner& owner;
    Capture* target=nullptr;
    unsigned completions=0;
    bool genuine_positive=false;
    const IsleGameModelPlacement* expected_placement=nullptr;
    std::uint32_t placement_address=0;
    std::span<std::uint16_t> unmarked_pixels{};
    static void finish(void* user,rsp::CanonicalExecutor::MemoryLease& lease,std::uint32_t framebuffer) {
        auto& self=*static_cast<Readback*>(user);auto& result=*self.target;
        ++self.completions;require(self.completions==1,"source draw acquired more than one actual completed lease");
        const auto bytes=lease.rdram();
        const std::span words(reinterpret_cast<const std::uint32_t*>(bytes.data()),bytes.size()/4);
        const auto pixel=[&](std::size_t address) { return static_cast<std::uint16_t>((std::to_integer<unsigned>(bytes[address^3u])<<8)|
            std::to_integer<unsigned>(bytes[(address+1)^3u])); };
        result.full16.fill(0);result.rgb5.fill(0);result.marked=false;result.red_positions=0;result.changed_mark_pixels=0;
        const auto context=self.detection.context;
        const auto rows=self.detection.history.history(context);
        focus::Subject selected{};
        for(std::size_t row=1;row<rows.size();++row) {
            for(std::size_t sample=0;sample<focus::tile_pixels;++sample) {
                const auto previous=pixel(self.detection.copies.region_offset(static_cast<unsigned>(row-1))+sample*2);
                const auto current=pixel(self.detection.copies.region_offset(static_cast<unsigned>(row))+sample*2);
                result.full16[row]+=previous!=current;result.rgb5[row]+=(previous&0xfffeu)!=(current&0xfffeu);
            }
            if(result.full16[row]>=32) selected=rows[row];
            std::cout<<"source_gpu_row context="<<context<<" row="<<row<<" id="<<rows[row].pokemon_id
                     <<" flags="<<rows[row].flags<<" full16="<<result.full16[row]<<" rgb5="<<result.rgb5[row]<<'\n';
        }
        const auto published=self.detection.history.published();
        require(self.detection.history.analyze(context,words,self.detection.copies.region_offset(0))==focus::Error::none,
                "real completed source capture analysis failed");
        require(same_focus(published,self.detection.history.published()),"GPU analysis published before native CopyInfo update");
        const auto* photo=self.detection.history.photo(context);require(photo,"source context membership absent");
        bool expected=selected.object!=0;
        const bool sign=selected.pokemon_id==1004||selected.pokemon_id==1018||selected.pokemon_id==1022||selected.pokemon_id==1035;
        if(expected&&!sign) expected=std::find(photo->objects.begin(),photo->objects.end(),selected.object)!=photo->objects.end();
        result.computed=self.detection.history.computed();
        require(result.computed.present==expected&&(!expected||(result.computed.object==selected.object&&
            result.computed.pokemon_id==selected.pokemon_id&&result.computed.flags==selected.flags&&result.computed.context==context)),
            "source last-full16 candidate/membership/context selection differs from independent reduction");
        self.genuine_positive|=expected&&selected.pokemon_id==129;
        for(std::size_t sample=0;sample<result.pixels.size();++sample) {
            result.pixels[sample]=pixel(framebuffer+sample*2);result.depth[sample]=pixel(frame_layout.depth+sample*2);
        }
        if(!self.unmarked_pixels.empty()) {
            require(self.unmarked_pixels.size()==result.pixels.size(),"movement canonical color extent changed");
            std::copy(result.pixels.begin(),result.pixels.end(),self.unmarked_pixels.begin());
        }
        if(self.expected_placement) {
            require(self.placement_address%64==0&&self.placement_address>=actor_layout.payloads.offset&&
                    self.placement_address+64<=actor_layout.payloads.offset+actor_layout.payloads.size,
                    "source-bound movement placement address escaped actual actor payload allocation");
            for(unsigned row=0;row<4;++row) for(unsigned col=0;col<4;++col)
                require(words[self.placement_address/4+row*4+col]==self.expected_placement->fixed.m[row][col],
                        "completed actual GPU RAM lost canonical C movement placement fixed upload");
            // Producer's nine camera commands precede the current lane0 list.
            // Stop at its actual EndDL, never accept a stale later slot's list.
            bool source_bound=false;
            for(std::size_t at=actor_layout.lists.offset/4+18;at+1<(actor_layout.lists.offset+actor_layout.lists.size)/4;at+=2) {
                if(words[at]==0xdf000000u) break;
                source_bound|=words[at]==0xda380000u&&words[at+1]==self.placement_address;
            }
            require(source_bound,"actual source hierarchy did not bind the admitted movement placement RAM address");
        }
        const auto hidden=fingerprint(lease.hidden());
        if(result.computed.present&&(result.computed.flags&4u)&&self.owner.player_source().gDirectionIndex==-1) {
            auto writable=lease.postprocess_words();
            require(self.detection.history.mark_center(writable,framebuffer,-1)==focus::Error::none,"actual source center writeback rejected");
            for(unsigned y=0;y<height;++y) for(unsigned x=0;x<width;++x) {
                const auto at=y*width+x;const auto after=pixel(framebuffer+at*2);
                const bool mark=x>=157&&x<=161&&y>=117&&y<=121&&!((x==157||x==161)&&(y==117||y==121));
                require(after==(mark?0xf801u:result.pixels[at]),"source21 marker changed wrong center/neighbor halfword");
                result.red_positions+=mark;result.changed_mark_pixels+=after!=result.pixels[at];result.pixels[at]=after;
                require(pixel(frame_layout.depth+at*2)==result.depth[at],"source marker changed actual completed depth");
            }
            require(result.red_positions==21&&fingerprint(lease.hidden())==hidden,"source marker changed hidden RDP memory or pattern");
            lease.finish_postprocess();result.marked=true;
        }
        for(std::size_t at=guard.offset/4;at<(guard.offset+guard.size)/4;++at)
            require(words[at]==0x13579bdfu,"actual source GPU/postprocess crossed disjoint guard");
        require(!lease.postprocess_active(),"source completed callback retained writable postprocess ownership");
    }
};
// A real priority1 callback witnesses the priority2 position pass after source
// animation3; a post-tick helper invocation would be too late for this oracle.
struct PositionWitness {
    scene::Services& services;
    actor::Owner& owner;
    bool saw_live=false;
    static void observe(IsleGameObject*,void* user) noexcept {
        auto& self=*static_cast<PositionWitness*>(user);unsigned paired=0;
        for(const auto& binding:self.owner.render_bindings()) {
            if(!binding.model.object) continue;
            IsleActorSource* source=nullptr;const IsleGameModelPlacement* auxiliary=nullptr;
            const IsleGameModelNode* nodes=nullptr;std::size_t count=0;
            if(isle_actor_source_get(self.services.game,binding.model.object,&source)!=ISLE_GAME_OK||
                isle_game_model_placement(self.services.game,binding.model.model,&auxiliary)!=ISLE_GAME_OK||
                isle_game_model_peek_nodes(self.services.game,binding.model.model,&nodes,&count)!=ISLE_GAME_OK||!count) {
                self.services.fail("native priority1 collision witness borrow failed");return;
            }
            paired+=(source->flags&0x200u)!=0;
            const auto& t=auxiliary->transform;const auto root=nodes[0].transform.position;
            if(t.rotation.x!=0||t.rotation.z!=0) { self.services.fail("source witness needs declared yaw-only AUX input");return; }
            const double x=source->collisionOffset.x+((source->flags&1)?double(root.x)*t.scale.x:0);
            const double y=source->collisionOffset.y+((source->flags&1)?double(root.y)*t.scale.y:0);
            const double z=source->collisionOffset.z+((source->flags&1)?double(root.z)*t.scale.z:0);
            const double c=std::cos(double(t.rotation.y)),s=std::sin(double(t.rotation.y));
            const auto near=[](double actual,double expected) { return std::abs(actual-expected)<0.02; };
            if(!near(source->collPosition.x,x*c+z*s+t.position.x)||!near(source->collPosition.y,y+t.position.y)||
                !near(source->collPosition.z,-x*s+z*c+t.position.z)) {
                self.services.fail("priority1 observed stale/wrong canonical collision point after source animation3/refresh2");return;
            }
            self.saw_live=true;
        }
        if(paired>1) self.services.fail("bounded source cohort requires genuine omitted pair collision services");
    }
};
void save_surface(const std::filesystem::path& path,g::Completed image) {
    require(bool(image),"completed source surface unavailable");std::ofstream out(path,std::ios::binary);
    require(bool(out),"source surface output open failed");out<<"P6\n"<<image.width<<' '<<image.height<<"\n255\n";
    for(std::size_t i=0;i<image.rgba.size();i+=4) out.write(reinterpret_cast<const char*>(image.rgba.data()+i),3);
    require(bool(out),"source surface output write failed");
}
void run(rom::Snapshot& snapshot,const std::filesystem::path& profile,const std::filesystem::path& output) {
    require(std::filesystem::is_directory(profile)&&std::filesystem::is_empty(profile),"private source profile must exist and be empty");
    std::filesystem::create_directories(output);
    const world::Resources world_resources(snapshot);const auto& setup=world_resources.setup();
    const beach::ActorResources resources(snapshot,{setup.fog_rgb,setup.fog_min,setup.fog_max});
    const beach::BehaviorResources behaviors(snapshot);const environment_sound::BeachResources sounds(snapshot);
    const menu::EffectResources effect_resources(snapshot);const photo::LightingInputs lighting(snapshot);
    const std::array banks{&effect_resources.bank(menu::EffectGroup::common),&effect_resources.bank(menu::EffectGroup::beach)};
    const menu::EffectGraphicsResources effect_graphics(snapshot,banks,{0x700000,0x80000});
    auto hydrated=game_audio::hydrate(snapshot);require(bool(hydrated),"source-focus audio hydration failed");
    std::array<audio::effects::Section,8> sections;audio::effects::Config effects;
    require(game_audio::native_startup_effects(hydrated.resources,sections,effects)==audio::Error::none,"source-focus audio effects failed");
    private_audio::Capture pcm;private_audio::Execution execution{snapshot,nullptr,{}};
    pcm.samples.resize(8192*audio::AudioService::max_frames*2);
    auto made=game_audio::Session::create(std::move(hydrated.resources),effects,pcm.output(),execution.executor());
    require(bool(made),"source-focus audio Session failed");auto& session=*made.session;std::uint64_t retrace=0;
    const auto pump=[&] {
        ++retrace;require(session.retrace(retrace,retrace*1000000000ull/60)==audio::Error::none,"source audio retrace failed");
        require(session.submit()==audio::Error::none,"source PCM submit failed");audio::TaskView task;
        require(session.build(task)==audio::Error::none,"source audio task failed");
        if(task.token.serial) { private_audio::execute(session,execution,task);require(session.submit()==audio::Error::none,"completed source PCM submit failed"); }
    };
    unsigned draws=0;bool positive=false;
    {
        game::State state(profile);require(state.initialize(1),"source-focus private profile init failed");
        game_input::Service input;scene::Storage storage(32*1024*1024);native(storage.create());
        {
            scene::Services services(storage.game(),storage.arena(),{{4,6,0,1,2},{}},input,session,state);
            require(!services.failed(),services.error());
            private_player::Source context(services);
            auto* root=services.object(0,0);auto* camera_object=services.object(1,0);
            require(root&&camera_object,"source root0/camera creation failed");
            IsleGameCamera* camera=nullptr;native(isle_game_attach_camera(services.game,camera_object,&camera));
            native(isle_game_camera_add_matrix(services.game,camera,ISLE_GAME_CAMERA_PERSPECTIVE_FAST));
            native(isle_game_camera_add_matrix(services.game,camera,ISLE_GAME_CAMERA_LOOK_AT_REFLECT_ROLL));
            const IsleGamePerspective perspective{55,4.0f/3.0f,10,25600,1};native(isle_game_camera_set_perspective(services.game,camera,&perspective));
            const IsleGameLookAt view{{0,1600,6000},{0,500,0},{0,1,0}};native(isle_game_camera_set_look_at(services.game,camera,&view));
            const IsleGameViewport viewport{{640,480,511,0},{640,480,511,0}};native(isle_game_camera_set_viewport(services.game,camera,&viewport));
            environment_sound::Environment environment(services,storage.arena(),sounds);
            require(environment.initialize(nullptr,camera),"environment source initialization failed");
            focus::Detector detector;focus::RegionCopies copies(capture_layout);actor::DetectorBinding detection{copies,detector};detection.enabled=true;
            WorldHooks hooks{services,camera};
            world::World world(services,world_resources,{&hooks,nullptr,WorldHooks::remove,nullptr,WorldHooks::camera_shift,WorldHooks::effects_shift,nullptr});
            world.follow_camera(camera);
            a::SlotCapacity maximum{};
            for(const auto& model:resources.models()) {
                const auto capacity=a::slot_capacity(model);maximum.nodes=std::max(maximum.nodes,capacity.nodes);
                maximum.material_attachments=std::max(maximum.material_attachments,capacity.material_attachments);
            }
            const std::array capacities{maximum,maximum};
            ActorMovementProbe movement{}; // Outlives Owner cancellation/join, including exception unwinding.
            {
                actor::Owner owner(services,resources,world,environment,detector,context.value,capacities,banks,{32768,4096});hooks.owner=&owner;
                accepted(owner.publish_effects());
                WorldLifetime world_lifetime{world,hooks};
                const auto make_model=[&](std::uint32_t id,std::uint8_t link,IsleGameVec3 position) {
                    auto* object=services.object(id,link);require(object,"native source input object failed");
                    IsleGameModelNodeInit node{};node.parent=UINT32_MAX;node.transform.position=position;node.transform.scale={1,1,1};
                    node.matrix_kind=ISLE_GAME_MODEL_CONTEXTUAL_RPY;IsleGameModel* model=nullptr;
                    native(isle_game_attach_model(services.game,object,&node,1,&model));return std::pair{object,model};
                };
                const auto trigger=make_model(0x1000,4,trigger_point),player=make_model(0x1001,0,{});
                const auto place_player=[&](IsleGameVec3 position) {
                    IsleGameModelTransform transform{};transform.position=position;transform.scale={1,1,1};
                    native(isle_game_model_set_placement(services.game,player.second,ISLE_GAME_MODEL_PLACEMENT_RT,&transform));
                };
                place_player(near_player);context.bind(player.first,player.second,camera);context.value.gDirectionIndex=-1;
                require(environment.set_listener(player.second,camera),"environment listener binding failed");
                actor::PhotoScene controller(services,owner,root,camera,detection,nullptr);
                require(controller.active(),"real source photo controller inactive");
                PositionWitness witness{services,owner};
                auto* witness_process=services.process(root,PositionWitness::observe,&witness,ISLE_GAME_SYNCHRONOUS,1);
                require(witness_process,"native priority1 position witness failed");
                ProcessLifetime witness_lifetime{services,witness_process};
                require(world.enter_first(0)==&world.blocks()[0],"source-focus initial World failed");
                IsleGameCamera* fx_camera=nullptr;native(isle_game_attach_camera(services.game,owner.particle_object(),&fx_camera));
                native(isle_game_camera_set_viewport(services.game,fx_camera,&viewport));
                beach::MagikarpScene spawner(services,owner,world,resources,behaviors);
                std::array<actor::ActorRenderBinding,2> bindings{};
                auto created=g::Frame::create(snapshot,{true,{16000000,4*1024*1024},frame_layout});require(bool(created),created.message.data());
                std::vector<menu::RamRange> mutable_ranges{world_layout.payloads,world_layout.lists,world_layout.materials,
                    actor_layout.payloads,actor_layout.lists,actor_layout.materials,fx_commands,joined_lists,guard};
                for(const auto range:copies.reservations()) mutable_ranges.push_back(range);
                for(const auto& span:lighting.installation_spans()) mutable_ranges.push_back({physical(span.guest_address),span.bytes.size()});
                auto all=mutable_ranges;
                for(const auto range:created.frame->reservations()) if(range.address) all.push_back({range.address,range.size});
                for(const auto range:actor_layout.framebuffers) all.push_back(range);
                for(const auto range:effect_graphics.reservations()) all.push_back(range);
                const auto other=[&](std::array<menu::RamRange,3> own,bool framebuffer) {
                    std::vector<menu::RamRange> result;
                    for(const auto range:all) {
                        bool skip=false;for(const auto mine:own) skip|=range.offset==mine.offset&&range.size==mine.size;
                        if(framebuffer) for(const auto fb:actor_layout.framebuffers) skip|=range.offset==fb.offset&&range.size==fb.size;
                        if(!skip) result.push_back(range);
                    }
                    return result;
                };
                auto world_other=other({world_layout.payloads,world_layout.lists,world_layout.materials},false);
                auto actor_other=other({actor_layout.payloads,actor_layout.lists,actor_layout.materials},true);
                menu::model::CameraBasisState basis;
                world::Producer world_producer(services.game,camera,basis,world_resources,world,world_layout,world_other);
                actor::Producer actor_producer(services.game,camera,basis,resources,capacities,bindings,actor_layout,actor_other,&detection);
                course::ModelProducer models(world_producer,actor_producer,joined_lists);
                ModelPass model_pass{models,copies,lighting,services.game,camera};EffectPass effect_pass{services.game,camera,owner.effects(),effect_graphics};
                std::vector<sprite::AssetSpan> immutable;
                const auto append=[&](auto spans) { for(const auto& span:spans) immutable.push_back({physical(span.guest_address),
                    {reinterpret_cast<const std::uint8_t*>(span.bytes.data()),span.bytes.size()}}); };
                append(world_resources.installation_spans());append(resources.installation_spans());
                for(const auto span:effect_graphics.installation_spans()) immutable.push_back(span);
                std::vector<sprite::RamRange> reservations;
                for(const auto range:mutable_ranges) reservations.push_back({static_cast<std::uint32_t>(range.offset),static_cast<std::uint32_t>(range.size)});
                const auto rgba=(std::uint32_t(setup.background_rgb[0])<<24)|(std::uint32_t(setup.background_rgb[1])<<16)|
                    (std::uint32_t(setup.background_rgb[2])<<8)|255u;
                std::array<scene::render::Pass,2> passes;
                passes[0].camera=camera;passes[0].priority=5;passes[0].flags=3;passes[0].background_rgba=rgba;
                const auto full_mask=(std::uint64_t(1)<<3)|(std::uint64_t(1)<<4)|(std::uint64_t(1)<<5);
                passes[0].link_mask=full_mask;passes[0].camera_tag=UINT32_MAX;passes[0].prepare=ModelPass::prepare;passes[0].userdata=&model_pass;
                passes[1].camera=fx_camera;passes[1].priority=4;passes[1].camera_tag=UINT32_MAX;passes[1].prepare=EffectPass::prepare;passes[1].userdata=&effect_pass;
                scene::render::Input scene_input{services.game,{1,nullptr,immutable,reservations,{0x3b5000,0x3da800},physical(lighting.pre_render_guest())},passes,{}};
                Readback readback{detection,owner};std::array<Capture,3> captures;
                Capture moved;
                std::vector<std::uint16_t> actual_color(width*height),control_color(width*height);
                // Last owner drains real GPU/postprocess before producer/native borrows end.
                auto frame=std::move(created.frame);
                bool diagnostic_route=false;
                const auto refresh_bindings=[&] {
                    const auto current=owner.render_bindings();require(current.size()==bindings.size(),"source owner changed fixed two-slot plan");
                    std::copy(current.begin(),current.end(),bindings.begin());
                    if(diagnostic_route&&bindings[0].model.object) bindings[0].render.link=4;
                };
                const auto draw=[&](Capture& capture,bool with_effects,const char* surface) {
                    require(frame->quiescent(),"source draw crossed GPU/native ownership");refresh_bindings();
                    const auto prior=detector.published();readback.target=&capture;readback.completions=0;
                    scene_input.passes=std::span(passes).first(with_effects?2:1);
                    require(frame->begin(scene_input,{draws&1u},{&readback,Readback::finish}),frame->message());
                    while(frame->poll()==g::Poll::pending) std::this_thread::yield();
                    require(frame->error()==g::Error::none&&frame->quiescent()&&readback.completions==1,frame->message());
                    require(same_focus(prior,detector.published()),"source draw bypassed native CopyInfo phase");
                    if(surface) save_surface(output/surface,frame->completed());
                    ++draws;
                };
                const auto tick=[&] {
                    require(frame->quiescent(),"source native tick preceded actual GPU/postprocess release");
                    const auto expected=detector.computed();native(isle_game_tick(services.game));require(!services.failed(),services.error());
                    require(same_focus(expected,detector.published()),"native CopyInfo failed to publish the previous completed computation before processes");
                    accepted(owner.collect_joined());pump();
                };
                // Separate renderer-order API fixture. Both objects are genuine
                // native source Magikarp; ONLY the borrowed first DL route is
                // authored DL4 here. It is not an Original Magikarp DL claim.
                const auto pre=spawner.spawn_at({trigger.first,trigger.second});const auto post=spawner.spawn_at({trigger.first,trigger.second});
                require(bool(pre)&&bool(post),"source prefix-order fixture factories failed");diagnostic_route=true;
                place_player(far_player);tick();draw(captures[0],false,nullptr);
                require((owner.source(pre.actor.instance)->flags&0x100u)&&(owner.source(post.actor.instance)->flags&0x100u),"real prefix did not apply source far-distance visibility");
                const auto old_context=static_cast<std::uint16_t>(1-detection.context);
                const auto old_rows=detector.history(old_context);
                require(!old_rows.empty()&&old_rows.back().object==reinterpret_cast<focus::Object>(pre.actor.object)&&
                    !(old_rows.back().flags&0x100u)&&detector.history(detection.context).size()==1,
                    "preprefix old draw/flags or postprefix new hidden gate was lost");
                place_player(near_player);tick();draw(captures[1],false,nullptr);
                const auto revealed=detector.history(detection.context);
                require(revealed.size()==2&&revealed[1].object==reinterpret_cast<focus::Object>(post.actor.object)&&
                    !(revealed[1].flags&0x100u),"prefix reveal failed to reach actual postprefix draw/capture");
                place_player(far_player);tick();const auto no_prefix_context=detection.context;
                const auto no_prefix_photo=*detector.photo(no_prefix_context);
                passes[0].link_mask=std::uint64_t(1)<<4;draw(captures[0],false,nullptr);
                require(detection.context==no_prefix_context&&detector.photo(no_prefix_context)->objects==no_prefix_photo.objects&&
                    !(owner.source(pre.actor.instance)->flags&0x100u)&&!(owner.source(post.actor.instance)->flags&0x100u),
                    "excluded prefix still collected membership/changed source flags/context");
                passes[0].link_mask=full_mask;detection.enabled=false;draw(captures[0],false,nullptr);
                require(detector.history(detection.context).size()==1&&(owner.source(pre.actor.instance)->flags&0x100u)&&
                    (owner.source(post.actor.instance)->flags&0x100u),"disabled actor capture incorrectly disabled source membership/baseline");
                detection.enabled=true;accepted(owner.reset());accepted(owner.collect_joined());diagnostic_route=false;place_player(near_player);
                context.value.gDirectionIndex=-1;readback.genuine_positive=false;
                const auto magikarp=spawner.spawn_at({trigger.first,trigger.second});require(bool(magikarp),magikarp.detail);
                const auto mag_identity=reinterpret_cast<focus::Object>(magikarp.actor.object);const auto mag_handle=magikarp.actor.instance;
                const auto particle=owner.effects().create_particle(3,0,{500,1233,4000},{});
                require(bool(particle.status)&&particle.spawned,"real fixed common FX creation failed");
                std::cout<<"frozen_genuine_source_rng_after_spawn="<<services.rng.seed<<" native_trigger=0,1233,4000 camera=legacy55 two_slots=true\n";
                bool observed_cleanup=false;
                for(unsigned update=1;update<=512;++update) {
                    tick();
                    if(!owner.source(mag_handle)) {
                        observed_cleanup=true;
                        for(unsigned context=0;context<2;++context) for(const auto row:detector.history(static_cast<std::uint16_t>(context)))
                            require(row.object!=mag_identity,"source cleanup left identity in a history before next preparation");
                        require(!detector.computed().present||detector.computed().object!=mag_identity,"source cleanup retained computed focus presence");
                    }
                    if(std::find(checkpoints.begin(),checkpoints.end(),update)!=checkpoints.end()) {
                        const char* surface=update==1?"genuine-magikarp-update1.ppm":update==11?"genuine-magikarp-update11.ppm":update==512?"genuine-source-retired.ppm":nullptr;
                        draw(captures[0],false,surface);draw(captures[1],true,nullptr);
                        if(update==1) {
                            IsleGameObjectMetadata metadata{};native(isle_game_object_metadata(services.game,magikarp.actor.object,&metadata));
                            native(isle_game_object_set_flags(services.game,magikarp.actor.object,metadata.flags|1u));draw(captures[2],false,"genuine-magikarp-hidden.ppm");
                            native(isle_game_object_set_flags(services.game,magikarp.actor.object,metadata.flags));draw(captures[2],false,nullptr);
                            require(captures[0].pixels==captures[2].pixels&&captures[0].depth==captures[2].depth,
                                    "genuine source hide/restore changed unchanged native pose/frame");
                        }
                    }
                }
                require(observed_cleanup&&witness.saw_live&&!owner.source(mag_handle),"fixed512 source lifecycle/priority2 witness incomplete");
                positive=readback.genuine_positive;
                // Three fixed extra native updates are solely a predeclared
                // physical-slot reuse tail, not an extension of the512 guard.
                const auto definitions=resources.definitions();
                const auto common=std::find_if(definitions.begin(),definitions.end(),[](const auto& d){return d.actor_id==1003;});
                require(common!=definitions.end(),"source ordinary reuse definition missing");
                world::Spawn spawn;spawn.id=1003;spawn.behavior=1;spawn.position={0,0,40};
                const auto reused=owner.add_one(static_cast<std::size_t>(common-definitions.begin()),world.current(),world.current(),spawn);
                accepted(reused.status);
                require(reused.instance.pool==mag_handle.pool&&reused.instance.slot==mag_handle.slot&&
                        reused.instance.incarnation!=mag_handle.incarnation,
                        "ordinary source factory did not reuse the collected physical slot with a fresh incarnation");
                tick();accepted(owner.trigger_common1003(reused.instance,{0,1233,4000}));tick();
                draw(captures[0],true,"ordinary-common-reused-slot.ppm");
                require(!detector.computed().present&&!detector.published().present&&detector.history(detection.context).size()==1,
                        "ordinary nongated reused slot inherited gated capture/focus");
                accepted(owner.cleanup(reused.instance));accepted(owner.collect_joined());tick();
                draw(captures[0],false,nullptr);
                // Independent original-C helper diagnostic, AFTER the genuine
                // fixed512 lifecycle and its unchanged ordinary-slot reuse tail.
                // A fresh single actor keeps real root priority2 live without an
                // overlapping pair; no source pose/camera/flags/radii are tuned.
                require(frame->quiescent(),"movement spawn crossed actual GPU/readback ownership");
                const auto diagnostic=spawner.spawn_at({trigger.first,trigger.second});
                require(bool(diagnostic),diagnostic.detail);
                const auto subject=diagnostic.actor;
                tick(); // One genuine source state7/animation3/distance8 warmup.
                auto* source=owner.source(subject.instance);
                require(source&&source->transitionGraph==nullptr&&std::isfinite(source->playerDist)&&source->playerDist<10000&&
                        !(source->processFlags&POKEMON_PROCESS_FLAG_MOVEMENT_PAUSED),
                        "fresh source movement warmup lacks real distance/NULL graph/unpaused movement");
                require(frame->quiescent(),"movement pause crossed actual GPU/readback ownership");
                native(isle_actor_source_pause_object(source,1));
                native(isle_game_pause_updates(services.game,subject.object,true));
                const IsleGameModelPlacement* canonical=nullptr;
                native(isle_game_model_placement(services.game,owner.native_model(subject.instance),&canonical));
                require(canonical->mode==ISLE_GAME_MODEL_PLACEMENT_RTS,"fresh actual source movement requires canonical RTS placement");
                const auto saved=*canonical;
                const auto saved_hspeed=source->hSpeed,saved_jumpvel=source->jumpVel;
                const auto saved_offset=source->collisionOffset,saved_collision_position=source->collPosition;
                const auto saved_radius=source->collisionRadius;
                const auto saved_process_flags=source->processFlags;
                // Actor Producer reserves camera256, then EVERY planned slot's
                // local matrices. BorrowedKernel assigns AUX matrices after that
                // region in active binding order, not by physical slot number.
                std::size_t placement_cursor=actor_layout.payloads.offset+256;
                for(const auto capacity:capacities) placement_cursor+=capacity.nodes*64;
                unsigned live=0;bool bound=false;
                for(const auto& binding:owner.render_bindings()) {
                    if(!binding.model.object) continue;
                    ++live;
                    const IsleGameModelPlacement* placement=nullptr;
                    native(isle_game_model_placement(services.game,binding.model.model,&placement));
                    if(binding.model.object==subject.object) {
                        require(binding.model.model==owner.native_model(subject.instance)&&
                                binding.model.state.instance==subject.instance&&binding.model.slot<capacities.size(),
                                "fresh diagnostic lost actual source model/incarnation/renderer slot binding");
                        readback.placement_address=static_cast<std::uint32_t>(placement_cursor);bound=true;
                    }
                    if(placement->mode!=ISLE_GAME_MODEL_PLACEMENT_DISABLED) placement_cursor+=64;
                }
                require(live==1&&bound,"independent movement diagnostic requires exactly its one genuine live source actor");
                readback.expected_placement=canonical;
                draw(captures[0],true,"actual-movement-initial.ppm");
                const auto local_span=actor_producer.native_nodes(subject.instance);
                const std::vector<IsleGameModelNode> saved_locals(local_span.begin(),local_span.end());
                const auto world_span=actor_producer.world_nodes(subject.instance);
                const std::vector<menu::model::WorldMatrix> saved_world(world_span.begin(),world_span.end());
                require(saved_world.size()>1&&saved_locals.size()==saved_world.size(),"actual movement model lacks real root/descendant topology");
                std::vector<menu::model::WorldMatrix> accepted_world(saved_world.size());
                std::cout<<"source_movement_initial_AUX="<<saved.transform.position.x<<','<<saved.transform.position.y<<','<<saved.transform.position.z
                         <<" yaw="<<saved.transform.rotation.y<<" scale="<<saved.transform.scale.x<<','<<saved.transform.scale.y<<','<<saved.transform.scale.z
                         <<" actual_fixed_RAM="<<readback.placement_address<<" single_live_source_actor=true\n";
                const auto independent_locals=[&] {
                    const auto locals=actor_producer.native_nodes(subject.instance);
                    require(locals.size()==saved_locals.size(),"C movement changed actual local topology");
                    for(std::size_t node=0;node<saved_locals.size();++node)
                        require(locals[node].revision==saved_locals[node].revision&&
                                std::memcmp(&locals[node].transform,&saved_locals[node].transform,sizeof(IsleGameModelTransform))==0,
                                "original C movement changed independent animated root/descendant locals");
                };
                const auto publish=[&](const IsleGameModelTransform& transform) {
                    require(frame->quiescent(),"movement component control crossed actual GPU/readback ownership");
                    accepted(owner.set_placement(subject.instance,transform));
                    native(isle_game_model_placement(services.game,owner.native_model(subject.instance),&canonical));
                    readback.expected_placement=canonical;
                };
                const auto step=[&](ActorMovementOperation operation,float first=0,float second=0) {
                    require(frame->quiescent(),"original C movement process crossed actual GPU/readback ownership");
                    movement.operation=operation;movement.a=first;movement.b=second;movement.c=0;movement.d=0;movement.flags=0;
                    native(actor_movement_probe_start(&movement,source));tick();
                    require(movement.entered==1&&movement.returned==1,"actual original C movement GPU probe did not complete");
                    native(isle_game_model_placement(services.game,owner.native_model(subject.instance),&canonical));
                    readback.expected_placement=canonical;
                };
                const auto completed_effect=[&](const IsleGameModelTransform& without_component,const char* surface,const char* prerequisite) {
                    const auto accepted_transform=canonical->transform;
                    readback.unmarked_pixels=actual_color;draw(moved,true,surface);independent_locals();
                    const auto actual_world=actor_producer.world_nodes(subject.instance);
                    require(actual_world.size()==accepted_world.size(),"C movement changed descendant world extent");
                    std::copy(actual_world.begin(),actual_world.end(),accepted_world.begin());
                    // NO native tick: source effects/camera/animation and all
                    // other AUX components stay identical to the actual C step.
                    publish(without_component);
                    readback.unmarked_pixels=control_color;draw(captures[1],true,nullptr);
                    require(actual_color!=control_color&&moved.depth!=captures[1].depth,prerequisite);
                    const auto counterfactual_world=actor_producer.world_nodes(subject.instance);
                    require(counterfactual_world.size()==accepted_world.size()&&
                            std::memcmp(&counterfactual_world[1],&accepted_world[1],sizeof(menu::model::WorldMatrix))!=0,
                            "component-isolated C movement did not affect actual renderer descendant world");
                    publish(accepted_transform);draw(captures[1],true,nullptr);independent_locals();
                    const auto restored_world=actor_producer.world_nodes(subject.instance);
                    require(actual_color==control_color&&moved.depth==captures[1].depth&&restored_world.size()==accepted_world.size()&&
                            std::memcmp(restored_world.data(),accepted_world.data(),accepted_world.size()*sizeof(menu::model::WorldMatrix))==0,
                            "same-state C component control failed exact completed color/depth/descendant restoration");
                };
                require(frame->quiescent(),"movement scalar inputs crossed actual GPU/readback ownership");
                source->hSpeed=1000;source->jumpVel=100;
                const float horizontal_speed=static_cast<float>(1000.0*0.033);
                const float dx=std::sin(saved.transform.rotation.y)*horizontal_speed,dz=std::cos(saved.transform.rotation.y)*horizontal_speed;
                step(ACTOR_MOVE_XZ);
                require(near(canonical->transform.position.x,saved.transform.position.x+dx)&&
                        near(canonical->transform.position.z,saved.transform.position.z+dz)&&
                        near(canonical->transform.position.y,saved.transform.position.y)&&canonical->revision>saved.revision&&
                        std::memcmp(&canonical->fixed,&saved.fixed,sizeof(saved.fixed))!=0,
                        "original C MoveXZ lost actual-yaw canonical placement/fixed/revision");
                auto without_component=canonical->transform;
                without_component.position.x=saved.transform.position.x;without_component.position.z=saved.transform.position.z;
                completed_effect(without_component,"actual-movement-horizontal.ppm",
                    "untouched genuine source MoveXZ pose needs observable canonical color AND depth; camera/pose/magnitude remain untuned");
                const auto before_jump=*canonical;
                step(ACTOR_JUMP,3);
                require(near(canonical->transform.position.x,before_jump.transform.position.x+dx)&&
                        near(canonical->transform.position.z,before_jump.transform.position.z+dz)&&
                        near(canonical->transform.position.y,before_jump.transform.position.y+static_cast<float>(100.0*0.033))&&
                        canonical->revision>before_jump.revision&&std::memcmp(&canonical->fixed,&before_jump.fixed,sizeof(before_jump.fixed))!=0,
                        "original C nested Jump lost actual-yaw horizontal/vertical canonical placement/fixed/revision");
                without_component=canonical->transform;without_component.position.y=before_jump.transform.position.y;
                completed_effect(without_component,"actual-movement-vertical.ppm",
                    "untouched genuine source Jump pose needs observable Y-only canonical color AND depth; nested XZ cannot stand in for vertical proof");
                const auto before_turn=*canonical;
                // Match the declared original-C angle normalization policy,
                // then request the existing .5 step relative to real source yaw.
                constexpr double tau=2.0*3.14159265358979323846;
                float yaw=before_turn.transform.rotation.y;
                yaw=static_cast<float>(double(yaw)-static_cast<std::int32_t>(double(yaw)/tau)*tau);
                if(yaw<0) yaw=static_cast<float>(double(yaw)+tau);
                const float turn_target=yaw+.5f;
                step(ACTOR_TURN,turn_target,.5f);
                require(movement.result==int(std::fabs(turn_target-yaw)<=.5f)&&near(canonical->transform.rotation.y,turn_target)&&
                        canonical->revision>before_turn.revision&&std::memcmp(&canonical->fixed,&before_turn.fixed,sizeof(before_turn.fixed))!=0,
                        "original C GPU turn lost source-yaw-relative .5 boundary/fixed/revision");
                without_component=canonical->transform;without_component.rotation.y=before_turn.transform.rotation.y;
                completed_effect(without_component,"actual-movement-turn.ppm",
                    "untouched genuine source Turn pose needs observable yaw-only canonical color AND depth; source yaw remains unforced");
                const auto before_scale=*canonical;
                step(ACTOR_SCALE,1.5f);
                IsleActorScaleInitialization initial{};native(isle_actor_owner_scale_initialization(source,&initial));
                require(near(canonical->transform.scale.x,initial.scale.x*0.1f*1.5f)&&
                        near(canonical->transform.scale.y,initial.scale.y*0.1f*1.5f)&&
                        near(canonical->transform.scale.z,initial.scale.z*0.1f*1.5f)&&
                        near(source->collisionOffset.x,initial.collision_center.x/initial.scale.x*1.5f)&&
                        near(source->collisionOffset.y,initial.collision_center.y/initial.scale.y*1.5f)&&
                        near(source->collisionOffset.z,initial.collision_center.z/initial.scale.z*1.5f)&&
                        near(source->collisionRadius,initial.radius*initial.scale.y*1.5f)&&
                        canonical->revision>before_scale.revision&&std::memcmp(&canonical->fixed,&before_scale.fixed,sizeof(before_scale.fixed))!=0,
                        "original C GPU scale lost actual factory initialization/collision semantics/fixed/revision");
                without_component=canonical->transform;without_component.scale=before_scale.transform.scale;
                completed_effect(without_component,"actual-movement-scale.ppm",
                    "untouched genuine source SetScale pose needs observable scale-only canonical color AND depth; initialization and scalar remain untuned");
                publish(saved.transform);
                require(frame->quiescent(),"movement source restoration crossed actual GPU/readback ownership");
                source->hSpeed=saved_hspeed;source->jumpVel=saved_jumpvel;
                source->collisionOffset=saved_offset;source->collisionRadius=saved_radius;source->collPosition=saved_collision_position;
                require(source->processFlags==saved_process_flags,"single-actor C movement changed unrelated source process flags");
                draw(captures[1],true,"actual-movement-restored.ppm");independent_locals();
                const auto restored_world=actor_producer.world_nodes(subject.instance);
                require(std::memcmp(&canonical->transform,&saved.transform,sizeof(saved.transform))==0&&
                        std::memcmp(&canonical->fixed,&saved.fixed,sizeof(saved.fixed))==0&&restored_world.size()==saved_world.size()&&
                        std::memcmp(restored_world.data(),saved_world.data(),saved_world.size()*sizeof(menu::model::WorldMatrix))==0,
                        "restored initial source movement pose did not restore canonical fixed/local/descendant semantics");
                readback.expected_placement=nullptr;readback.unmarked_pixels={};
                require(frame->quiescent(),"movement retirement crossed actual GPU/readback ownership");
                native(isle_game_pause_updates(services.game,subject.object,false));native(isle_actor_source_pause_object(source,0));
                accepted(owner.cleanup(subject.instance));accepted(owner.collect_joined());
                require(!owner.source(subject.instance),"actual diagnostic actor retained source ownership after joined retirement");
                tick();draw(captures[0],true,"actual-movement-retired.ppm");
                require(frame->retire()&&frame->quiescent(),"real source graphics failed retirement");
                world_producer.retire();world.retire();
                accepted(owner.reset());accepted(owner.collect_joined());
                // Real late source-cursor rejection, not a fabricated callback.
                // GPU is already drained; this pure producer preparation must
                // not publish metadata or write RAM before the prefix fails.
                auto* unbound=services.object(0x1002,3);require(unbound,"late native cursor-error fixture failed");
                std::array<std::vector<focus::Subject>,2> saved_rows;
                std::array<focus::Photo,2> saved_photos;
                for(unsigned context=0;context<2;++context) {
                    const auto rows=detector.history(static_cast<std::uint16_t>(context));
                    saved_rows[context].assign(rows.begin(),rows.end());
                    saved_photos[context]=*detector.photo(static_cast<std::uint16_t>(context));
                }
                const auto saved_context=detection.context;
                const auto saved_photo=detection.photo;
                const auto saved_computed=detector.computed(),saved_published=detector.published();
                std::vector<std::uint32_t> rejected_ram(beach::rdram_size/4,0x58a19c3du);
                bool rejected=false;
                try { static_cast<void>(actor_producer.prepare(rejected_ram,100000)); }
                catch(const std::exception&) { rejected=true; }
                require(rejected&&services.failed()&&frame->quiescent()&&detection.context==saved_context&&
                        detection.photo.objects==saved_photo.objects&&detection.photo.focus==saved_photo.focus&&
                        same_focus(saved_computed,detector.computed())&&same_focus(saved_published,detector.published())&&
                        std::all_of(rejected_ram.begin(),rejected_ram.end(),[](auto word){return word==0x58a19c3du;}),
                        "rejected real membership preparation published detector/frame/RAM state");
                for(unsigned context=0;context<2;++context) {
                    const auto rows=detector.history(static_cast<std::uint16_t>(context));
                    require(rows.size()==saved_rows[context].size(),"rejected membership changed source row extent");
                    for(std::size_t i=0;i<rows.size();++i)
                        require(rows[i].object==saved_rows[context][i].object&&rows[i].pokemon_id==saved_rows[context][i].pokemon_id&&
                                rows[i].flags==saved_rows[context][i].flags,"rejected membership changed source history");
                    const auto* photo=detector.photo(static_cast<std::uint16_t>(context));
                    require(photo->objects==saved_photos[context].objects&&photo->focus==saved_photos[context].focus,
                            "rejected membership changed source per-context photo state");
                }
                services.erase(unbound);
                actor_producer.retire();
                witness_lifetime.finish();
                // Sticky failure arose from the real native cursor rejection.
                // Detach validated live-root observers without healing it.
                controller.retire();
                require(!controller.active()&&!controller.prefix_object()&&!controller.copy_info_object(),
                        "failed-scope source controller kept native borrowers or callbacks");
                require(environment.set_listener(nullptr,nullptr),"environment listener release failed");services.erase(trigger.first);services.erase(root);
                hooks.owner=nullptr;
            }
            environment.cleanup();
        }
        native(storage.destroy());
    }
    require(session.stop()==audio::Error::none,"source-focus audio stop failed");
    for(unsigned n=0;n<240&&session.state()!=game_audio::Session::State::draining;++n) pump();
    require(session.state()==game_audio::Session::State::draining&&session.service().voices_retired(),"source-focus voices did not retire");
    require(session.poll_drain()==audio::Error::none&&session.poll_drain()==audio::Error::none&&session.state()==game_audio::Session::State::drained,
            "source-focus checked PCM owner did not drain");
    std::cout<<"native_source_membership_prefix_CopyInfo_GPU_lifetime=passed gpu_frames="<<draws
             <<" positive_gated_Magikarp_focus="<<(positive?"established":"NOT_ESTABLISHED")
             <<" full_snapshot_Item_Player_gameplay_Original_Windows11=not_claimed\n";
}
}
int main(int argc,char** argv) {
    try {
        require(argc==4,"usage: course-magikarp-rsp-smoke PRIVATE_ROM EMPTY_PRIVATE_PROFILE OUTPUT_DIRECTORY");
        auto snapshot=isle::rom::read_snapshot(argv[1]);run(snapshot,argv[2],argv[3]);return 0;
    } catch(const std::exception& error) { std::cerr<<"course-magikarp-rsp-smoke: "<<error.what()<<'\n';return 1; }
}
