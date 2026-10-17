// Explicit private-ROM component integration, never CTest or a game entrypoint.
// Actual common1003 FSM + complete admitted World/Actor assets + sole course FX.
// Real native Player Root/AUX only; no Player FSM/items/photo queue/game runtime claim.
#include "actor_owner.hpp"
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
constexpr std::uint32_t width=320,height=240,plane=width*height*2;
constexpr menu::RamRange fx_commands{0x680c00,0x5400},joined_lists{0x686000,0x100},guard{0x7f0000,256};
constexpr world::ProducerLayout world_layout{{0x200000,0x10000},{0x210000,0x10000},{0x220000,0x10000}};
constexpr actor::ProducerLayout actor_layout{{0x400000,0x80000},{0x480000,0x80000},{0x500000,0x100000},
    {{{0x3b5000,plane},{0x3da800,plane}}}};
constexpr focus::CaptureLayout capture_layout{{0x680000,0x50},{0x680080,0x80},{0x680100,0xa00}};
constexpr g::Layout frame_layout{{0x626000,0x2000},{0x630000,0x10000},{0x640000,0x40000},0x600000};
static_assert((menu::EffectRuntime::max_render_words+2)*4<=fx_commands.size);
void require(bool value,const char* message) { if(!value) throw std::runtime_error(message); }
void native(IsleGameResult result) { require(result==ISLE_GAME_OK,"composed native operation failed"); }
void accepted(actor::OwnerStatus result) { require(bool(result),result.detail); }
std::uint32_t physical(std::uint32_t address) { return address&0x1fffffffu; }
bool same(menu::Vec3 x,menu::Vec3 y) { return x.x==y.x&&x.y==y.y&&x.z==y.z; }
struct Bindings {
    scene::Services& services;
    IsleGameCamera* camera=nullptr;
    actor::Owner* owner=nullptr;
    world::Block* previous_reference=nullptr;
    std::array<a::InstanceHandle,2> actors{};
    std::array<world::Block*,2> bases{};
    unsigned camera_shifts=0,effect_shifts=0,actor_shifts=0;
    static void camera_shift(void* context,world::Vec3 delta) noexcept {
        auto& self=*static_cast<Bindings*>(context);
        const IsleGameCameraParameters* parameters=nullptr;
        if(!self.services.check(isle_game_camera_parameters(self.services.game,self.camera,&parameters),"composed camera shift borrow")) return;
        auto view=parameters->look_at;
        for(unsigned axis=0;axis<3;++axis) {
            const auto d=axis==0?delta.x:axis==1?delta.y:delta.z;
            view.eye[axis]+=d; view.at[axis]+=d;
        }
        self.services.check(isle_game_camera_set_look_at(self.services.game,self.camera,&view),"composed source camera shift");
        ++self.camera_shifts;
    }
    static void effect_shift(void* context,world::Vec3 delta) noexcept {
        auto& self=*static_cast<Bindings*>(context);
        if(!self.owner) { self.services.fail("composed FX owner missing at real World shift"); return; }
        self.owner->effects().translate({delta.x,delta.y,delta.z});
        if(!self.owner->effects().status()) self.services.fail("composed source FX translation failed");
        ++self.effect_shifts;
    }
    static void change(void* context,world::Block& block,world::Block& reference) noexcept {
        auto& self=*static_cast<Bindings*>(context);
        if(!self.owner||!self.previous_reference) { self.services.fail("composed actor reference missing"); return; }
        const auto previous=self.previous_reference->resource->metadata.world_position;
        const auto current=reference.resource->metadata.world_position;
        for(std::size_t slot=0;slot<self.actors.size();++slot)
            if(self.bases[slot]==&block&&self.owner->registered(self.actors[slot])) {
                if(!self.owner->change_block(self.actors[slot],{previous.x,previous.y,previous.z},{current.x,current.y,current.z}))
                    self.services.fail("composed actual actor block shift failed");
                ++self.actor_shifts;
            }
    }
    static void remove(void* context,world::Block& block) noexcept {
        auto& self=*static_cast<Bindings*>(context);
        if(!self.owner||!self.owner->remove_block(block)) self.services.fail("composed actual actor block removal failed");
    }
};
// Lifetime ordering only, never a completion fence: later-declared Frame really
// drains first, then producer borrows end, then World callbacks retire actors
// while their Owner is still alive. This also preserves the original exception.
struct WorldOwnerScope {
    world::World& world;
    Bindings& bindings;
    ~WorldOwnerScope() { world.retire(); bindings.owner=nullptr; }
};
struct ModelPass {
    course::ModelProducer& producer;
    focus::RegionCopies& copies;
    const photo::LightingInputs& lighting;
    IsleGame* game;
    IsleGameCamera* camera;
    std::uint64_t installed_epoch=0;
    const std::uint32_t* installed_ram=nullptr;
    static bool prepare(void* context,std::span<std::uint32_t> words,std::uint64_t epoch,
                        const scene::render::Pass& pass,scene::render::Prepared& output) {
        auto& self=*static_cast<ModelPass*>(context);
        if(self.installed_epoch!=epoch||self.installed_ram!=words.data()) {
            if(self.copies.install(words)!=focus::CaptureStatus::ok) return false;
            self.lighting.install(std::as_writable_bytes(words));
            std::fill_n(words.begin()+guard.offset/4,guard.size/4,0x13579bdfu);
            self.installed_epoch=epoch; self.installed_ram=words.data();
        }
        const IsleGameCameraParameters* camera=nullptr;
        if(isle_game_camera_parameters(self.game,self.camera,&camera)!=ISLE_GAME_OK||!camera) return false;
        const auto& eye=camera->look_at.eye; const auto& at=camera->look_at.at;
        self.lighting.update_direction(std::as_writable_bytes(words),{eye[0],eye[1],eye[2]},{at[0],at[1],at[2]});
        output=self.producer.prepare(words,epoch,pass);
        return true;
    }
};
struct EffectPass {
    IsleGame* game;
    IsleGameCamera* main_camera;
    menu::EffectRuntime& runtime;
    const menu::EffectGraphicsResources& resources;
    std::size_t rendered_particles=0;
    std::array<int,2> derivatives{};
    static bool prepare(void* context,std::span<std::uint32_t> words,std::uint64_t,
                        const scene::render::Pass&,scene::render::Prepared& output) {
        auto& self=*static_cast<EffectPass*>(context);
        menu::EffectCamera camera;
        if(!menu::source_effect_camera(self.game,self.main_camera,camera)) return false;
        const auto rendered=self.runtime.render(camera,{words.subspan(fx_commands.offset/4,menu::EffectRuntime::max_render_words),
            self.resources.bindings(),self.resources.reservations()});
        if(!rendered.status||rendered.words>menu::EffectRuntime::max_render_words||(rendered.words&1u)) return false;
        self.rendered_particles=rendered.particles;
        for(std::size_t index=0;index<rendered.words;index+=2) {
            const auto first=words[fx_commands.offset/4+index],second=words[fx_commands.offset/4+index+1];
            if((first>>24)==0xf1u) {
                const auto signed16=[](std::uint32_t value) { return int(value&32767u)-int(value&32768u); };
                self.derivatives={signed16(second>>16),signed16(second)};
            }
        }
        const auto end=fx_commands.offset/4+rendered.words;
        words[end]=0xdf000000u; words[end+1]=0;
        output.lanes[0]={static_cast<std::uint32_t>(fx_commands.offset),rendered.words+2};
        return true;
    }
};
struct Capture {
    std::vector<std::uint16_t> pixels=std::vector<std::uint16_t>(width*height);
    std::vector<std::uint16_t> depths=std::vector<std::uint16_t>(width*height);
    std::uint64_t geometry=14695981039346656037ull;
    std::size_t triangles=0;
    bool marked=false;
};
struct Readback {
    actor::DetectorBinding& detector;
    Capture* destination=nullptr;
    bool clear_oracle=false;
    std::uint16_t clear_color=0;
    static void finish(void* context,rsp::CanonicalExecutor::MemoryLease& lease,std::uint32_t framebuffer) {
        auto& self=*static_cast<Readback*>(context); auto& result=*self.destination;
        const auto bytes=lease.rdram();
        const std::span words(reinterpret_cast<const std::uint32_t*>(bytes.data()),bytes.size()/4);
        const auto pixel=[&](std::size_t at) { return static_cast<std::uint16_t>((std::to_integer<unsigned>(bytes[at^3u])<<8)|
            std::to_integer<unsigned>(bytes[(at+1)^3u])); };
        if(self.clear_oracle) for(std::size_t sample=0;sample<focus::tile_pixels;++sample)
            require((pixel(self.detector.copies.region_offset(0)+sample*2)&0xfffeu)==(self.clear_color&0xfffeu),
                    "actual source DL5 baseline copy differs from the independent cleared-color oracle");
        require(self.detector.history.analyze(self.detector.context,words,self.detector.copies.region_offset(0))==focus::Error::none,
                "actual composed GPU detector analysis failed");
        self.detector.history.publish();
        result.marked=false;
        const auto& focus=self.detector.history.computed();
        if(focus.present&&(focus.flags&4u)) {
            auto writable=lease.postprocess_words();
            require(self.detector.history.mark_center(writable,framebuffer,-1)==focus::Error::none,"actual composed focus mark failed");
            lease.finish_postprocess(); result.marked=true;
        }
        result.geometry=14695981039346656037ull; result.triangles=0;
        const auto& packets=lease.packets();
        for(const auto packet:packets.packets()) {
            const auto data=packets.words().subspan(packet.offset,packet.count);
            const auto opcode=(data[0]>>24)&63u;
            if(opcode>=8&&opcode<=15) {
                ++result.triangles;
                for(const auto word:data) result.geometry=(result.geometry^word)*1099511628211ull;
            }
        }
        for(std::size_t sample=0;sample<result.pixels.size();++sample) {
            result.pixels[sample]=pixel(framebuffer+sample*2);
            result.depths[sample]=pixel(frame_layout.depth+sample*2);
        }
        for(std::size_t at=guard.offset/4;at<(guard.offset+guard.size)/4;++at)
            require(words[at]==0x13579bdfu,"composed GPU task or completed postprocess changed the disjoint guard");
        require(!lease.postprocess_active(),"composed source postprocess retained a writer");
    }
};
void ppm(const std::filesystem::path& path,g::Completed image) {
    require(bool(image),"actual composed surface unavailable");
    std::ofstream file(path,std::ios::binary); require(bool(file),"actual composed PPM open failed");
    file<<"P6\n"<<image.width<<' '<<image.height<<"\n255\n";
    for(std::size_t pixel=0;pixel<image.rgba.size();pixel+=4)
        file.write(reinterpret_cast<const char*>(image.rgba.data()+pixel),3);
    require(bool(file),"actual composed PPM write failed");
}
void run(rom::Snapshot& snapshot,const std::filesystem::path& profile,const std::filesystem::path& output) {
    require(std::filesystem::is_directory(profile)&&std::filesystem::is_empty(profile),"private composed profile must be empty");
    std::filesystem::create_directories(output);
    const world::Resources world_resources(snapshot); const auto& setup=world_resources.setup();
    const beach::ActorResources actor_resources(snapshot,{setup.fog_rgb,setup.fog_min,setup.fog_max});
    const environment_sound::BeachResources sound_resources(snapshot);
    const menu::EffectResources effect_resources(snapshot); const photo::LightingInputs lighting(snapshot);
    const std::array banks{&effect_resources.bank(menu::EffectGroup::common),&effect_resources.bank(menu::EffectGroup::beach)};
    const menu::EffectGraphicsResources effect_graphics(snapshot,banks,{0x700000,0x80000});
    auto hydrated=game_audio::hydrate(snapshot); require(bool(hydrated),"composed actual audio hydration failed");
    std::array<audio::effects::Section,8> sections; audio::effects::Config audio_effects;
    require(game_audio::native_startup_effects(hydrated.resources,sections,audio_effects)==audio::Error::none,"composed source audio setup failed");
    private_audio::Capture audio_capture; private_audio::Execution audio_execution{snapshot,nullptr,{}};
    audio_capture.samples.resize(8192*audio::AudioService::max_frames*2);
    auto audio=game_audio::Session::create(std::move(hydrated.resources),audio_effects,audio_capture.output(),audio_execution.executor());
    require(bool(audio),"composed actual audio Session failed"); auto& session=*audio.session;
    std::uint64_t retrace=0;
    unsigned total_draws=0;
    const auto pump=[&] {
        ++retrace; require(session.retrace(retrace,retrace*1000000000ull/60)==audio::Error::none,"composed audio retrace");
        require(session.submit()==audio::Error::none,"composed PCM submit");
        audio::TaskView task; require(session.build(task)==audio::Error::none,"composed source audio build");
        if(task.token.serial) { private_audio::execute(session,audio_execution,task); require(session.submit()==audio::Error::none,"composed completed PCM"); }
    };
    {
        game::State state(profile); require(state.initialize(1),"private composed profile initialization failed");
        game_input::Service input;
        game_input::HostFrame controlled{}; controlled[0]={game_input::Connection::present,1};
        require(input.start(controlled)==game_input::Error::none,"owned neutral controller start failed");
        scene::Storage storage(32*1024*1024); native(storage.create());
        {
            scene::Services services(storage.game(),storage.arena(),{{4,6,0,1,2},{}},input,session,state);
            require(!services.failed(),services.error());
            private_player::Source context(services);
            auto* camera_object=services.object(1,0); require(camera_object,"composed main camera object");
            IsleGameCamera* camera=nullptr; native(isle_game_attach_camera(services.game,camera_object,&camera));
            native(isle_game_camera_add_matrix(services.game,camera,ISLE_GAME_CAMERA_PERSPECTIVE_FAST));
            native(isle_game_camera_add_matrix(services.game,camera,ISLE_GAME_CAMERA_LOOK_AT_REFLECT_ROLL));
            const IsleGamePerspective perspective{55,4.0f/3.0f,10,25600,1}; native(isle_game_camera_set_perspective(services.game,camera,&perspective));
            const IsleGameLookAt view{{0,1600,6000},{0,500,0},{0,1,0}}; native(isle_game_camera_set_look_at(services.game,camera,&view));
            const IsleGameViewport viewport{{640,480,511,0},{640,480,511,0}}; native(isle_game_camera_set_viewport(services.game,camera,&viewport));
            context.create(camera);
            environment_sound::Environment environment(services,storage.arena(),sound_resources);
            require(environment.initialize(nullptr,camera),"environment source initialization failed");
            focus::Detector detector; focus::RegionCopies copies(capture_layout);
            Bindings bindings{services,camera};
            world::World world(services,world_resources,{&bindings,nullptr,Bindings::remove,Bindings::change,Bindings::camera_shift,Bindings::effect_shift,nullptr});
            world.follow_camera(camera);
            a::SlotCapacity capacity{};
            for(const auto& model:actor_resources.models()) {
                const auto required=a::slot_capacity(model);
                capacity.nodes=std::max(capacity.nodes,required.nodes);
                capacity.material_attachments=std::max(capacity.material_attachments,required.material_attachments);
            }
            const std::array capacities{capacity,capacity};
            {
                actor::Owner owner(services,actor_resources,world,environment,detector,context.value,capacities,banks,{32768,4096});
                accepted(owner.publish_effects());
                bindings.owner=&owner; bindings.previous_reference=&world.blocks()[0];
                WorldOwnerScope world_owner_scope{world,bindings};
                require(world.enter_first(0)==&world.blocks()[0],"composed source first block entry failed");
                IsleGameCamera* fx_camera=nullptr; native(isle_game_attach_camera(services.game,owner.particle_object(),&fx_camera));
                native(isle_game_camera_set_viewport(services.game,fx_camera,&viewport));
                const auto definitions=actor_resources.definitions();
                const auto found=std::find_if(definitions.begin(),definitions.end(),[](const auto& definition){return definition.actor_id==1003;});
                require(found!=definitions.end(),"source common1003 definition unavailable"); const auto definition=static_cast<std::size_t>(found-definitions.begin());
                world::Spawn normal_spawn; normal_spawn.id=1003; normal_spawn.behavior=0; normal_spawn.position={-3,0,40};
                world::Spawn hidden_spawn=normal_spawn; hidden_spawn.behavior=1; hidden_spawn.position={0,0,40};
                const auto normal=owner.spawn({10000,definition,&world.blocks()[0],world.current(),&normal_spawn}); accepted(normal.status);
                const auto hidden=owner.spawn({10001,definition,&world.blocks()[0],world.current(),&hidden_spawn}); accepted(hidden.status);
                bindings.actors={normal.instance,hidden.instance}; bindings.bases={&world.blocks()[0],&world.blocks()[0]};
                auto created=g::Frame::create(snapshot,{true,{16000000,4*1024*1024},frame_layout}); require(bool(created),created.message.data());
                std::vector<menu::RamRange> mutable_ranges{world_layout.payloads,world_layout.lists,world_layout.materials,
                    actor_layout.payloads,actor_layout.lists,actor_layout.materials,fx_commands,joined_lists,guard};
                for(const auto range:copies.reservations()) mutable_ranges.push_back(range);
                for(const auto& span:lighting.installation_spans()) mutable_ranges.push_back({physical(span.guest_address),span.bytes.size()});
                auto all_ranges=mutable_ranges;
                for(const auto range:created.frame->reservations()) if(range.address) all_ranges.push_back({range.address,range.size});
                for(const auto range:actor_layout.framebuffers) all_ranges.push_back(range);
                for(const auto range:effect_graphics.reservations()) all_ranges.push_back(range);
                const auto excluding=[&](std::array<menu::RamRange,3> owned,bool framebuffers) {
                    std::vector<menu::RamRange> other;
                    for(const auto range:all_ranges) {
                        bool skip=false;
                        for(const auto mine:owned) skip=skip||(range.offset==mine.offset&&range.size==mine.size);
                        if(framebuffers) for(const auto target:actor_layout.framebuffers) skip=skip||(range.offset==target.offset&&range.size==target.size);
                        if(!skip) other.push_back(range);
                    }
                    return other;
                };
                auto world_other=excluding({world_layout.payloads,world_layout.lists,world_layout.materials},false);
                auto actor_other=excluding({actor_layout.payloads,actor_layout.lists,actor_layout.materials},true);
                menu::model::CameraBasisState basis;
                world::Producer world_producer(services.game,camera,basis,world_resources,world,world_layout,world_other);
                actor::DetectorBinding detection{copies,detector}; detection.enabled=true;
                detection.photo.objects[0]=reinterpret_cast<focus::Object>(hidden.object);
                actor::Producer actor_producer(services.game,camera,basis,actor_resources,capacities,owner.render_bindings(),actor_layout,actor_other,&detection);
                course::ModelProducer models(world_producer,actor_producer,joined_lists);
                ModelPass model_pass{models,copies,lighting,services.game,camera};
                EffectPass effect_pass{services.game,camera,owner.effects(),effect_graphics};
                std::vector<sprite::AssetSpan> immutable;
                const auto append=[&](auto spans) { for(const auto& span:spans) immutable.push_back({physical(span.guest_address),
                    {reinterpret_cast<const std::uint8_t*>(span.bytes.data()),span.bytes.size()}}); };
                append(world_resources.installation_spans()); append(actor_resources.installation_spans());
                for(const auto span:effect_graphics.installation_spans()) immutable.push_back(span);
                std::vector<sprite::RamRange> reservations;
                for(const auto range:mutable_ranges) reservations.push_back({static_cast<std::uint32_t>(range.offset),static_cast<std::uint32_t>(range.size)});
                const auto rgba=(std::uint32_t(setup.background_rgb[0])<<24)|(std::uint32_t(setup.background_rgb[1])<<16)|
                    (std::uint32_t(setup.background_rgb[2])<<8)|255u;
                std::array<scene::render::Pass,2> passes;
                passes[0].camera=camera; passes[0].priority=5; passes[0].flags=3; passes[0].background_rgba=rgba;
                passes[0].link_mask=(std::uint64_t(1)<<3)|(std::uint64_t(1)<<4)|(std::uint64_t(1)<<5);
                passes[0].camera_tag=UINT32_MAX; passes[0].prepare=ModelPass::prepare; passes[0].userdata=&model_pass;
                passes[1].camera=fx_camera; passes[1].priority=4; passes[1].camera_tag=UINT32_MAX;
                passes[1].prepare=EffectPass::prepare; passes[1].userdata=&effect_pass;
                scene::render::Input scene_input{services.game,{1,nullptr,immutable,reservations,{0x3b5000,0x3da800},physical(lighting.pre_render_guest())},passes,{}};
                // Declare the real executor owner LAST: it drains before producers,
                // native models, sole FX and canonical actor records on every exit.
                auto frame=std::move(created.frame);
                Readback readback{detection}; std::array<Capture,5> captures;
                unsigned draws=0;
                const auto draw=[&](Capture& capture,bool effects,bool clear,const char* surface) {
                    require(frame->quiescent(),"native scene draw crossed outstanding actual GPU/readback ownership");
                    owner.render_bindings();
                    readback.destination=&capture; readback.clear_oracle=clear;
                    readback.clear_color=static_cast<std::uint16_t>(((rgba>>16)&0xf800u)|((rgba>>13)&0x7c0u)|((rgba>>10)&0x3eu)|((rgba>>7)&1u));
                    scene_input.passes=std::span(passes).first(effects?2:1);
                    require(frame->begin(scene_input,{draws&1u},{&readback,Readback::finish}),frame->message());
                    while(frame->poll()==g::Poll::pending) std::this_thread::yield();
                    require(frame->error()==g::Error::none&&frame->quiescent(),frame->message());
                    require(bool(frame->completed()),"actual composed GPU completion not published");
                    if(surface) ppm(output/surface,frame->completed());
                    ++draws;
                };
                const auto tick=[&] {
                    require(frame->quiescent(),"native tick preceded real GPU/readback/postprocess release");
                    const auto reading=input.read_and_update();
                    require(reading.error==game_input::Error::none&&bool(reading.read),"owned neutral controller read request failed");
                    require(input.complete_read(reading.read,controlled).error==game_input::Error::none,"owned neutral controller publication failed");
                    require(input.begin_update()==game_input::Error::none,"composed source input update failed"); native(isle_game_tick(services.game));
                    require(!services.failed(),services.error()); accepted(owner.collect_joined()); pump();
                };
                tick(); require(!owner.source(hidden.instance)->tangible,"source hidden state did not execute before trigger");
                accepted(owner.trigger_common1003(hidden.instance,{0,1233,4000})); tick();
                require(owner.source(hidden.instance)->tangible,"actual common1003 trigger did not show source actor");
                passes[0].link_mask=std::uint64_t(1)<<5;
                draw(captures[4],false,true,nullptr); // Real DL5 prefix; full DL3/4 bindings still validated.
                passes[0].link_mask=(std::uint64_t(1)<<3)|(std::uint64_t(1)<<4)|(std::uint64_t(1)<<5);
                draw(captures[0],false,false,"actual-world-native-actors.ppm");
                require(captures[0].triangles&&captures[0].pixels!=captures[4].pixels,"actual composed World emitted no GPU geometry");
                IsleGameObjectMetadata flags{}; native(isle_game_object_metadata(services.game,hidden.object,&flags));
                native(isle_game_object_set_flags(services.game,hidden.object,flags.flags|1u));
                draw(captures[1],false,false,"actual-world-without-hidden-subject.ppm");
                native(isle_game_object_set_flags(services.game,hidden.object,flags.flags));
                draw(captures[2],false,false,nullptr);
                require(captures[0].geometry==captures[2].geometry&&captures[0].pixels==captures[2].pixels&&
                    captures[0].depths==captures[2].depths,"source actor visibility restore changed the actual composed frame");
                require(captures[1].pixels!=captures[0].pixels&&captures[1].geometry!=captures[0].geometry,
                    "native source actor visibility did not reach the real composed geometry and pixels");
                const auto particle=owner.effects().create_particle(3,0,{500,1233,4000},{});
                require(bool(particle.status)&&particle.spawned,"actual common3 particle API failed");
                tick(); draw(captures[3],false,false,nullptr);
                draw(captures[4],true,false,nullptr);
                // Actual common0 grows size 5 -> 15 over six VM updates. Its
                // second update wraps both S5.10 steps negative at this unchanged
                // camera; clamping samples the transparent I8 corner. Preserve
                // that source checkpoint rather than enlarging/moving the FX.
                require(effect_pass.rendered_particles&&effect_pass.derivatives[0]<0&&effect_pass.derivatives[1]<0&&
                    captures[3].geometry==captures[4].geometry&&captures[3].pixels==captures[4].pixels&&
                    captures[3].depths==captures[4].depths&&captures[3].marked==captures[4].marked,
                    "early source signed-step/clamped-alpha checkpoint changed the composed frame");
                // The next, exactly third VM update is the first positive-step
                // phase here. No source duration, pose, scale, camera or budget changes.
                tick();
                draw(captures[3],false,false,nullptr);
                draw(captures[4],true,false,"actual-world-native-actors-fx.ppm");
                require(effect_pass.rendered_particles&&effect_pass.derivatives[0]>0&&effect_pass.derivatives[1]>0&&
                    captures[3].geometry==captures[4].geometry&&captures[3].pixels!=captures[4].pixels,
                    "actual shared source FX did not change composed GPU pixels at its positive-step phase");
                require(captures[3].depths==captures[4].depths&&captures[3].marked==captures[4].marked,
                    "source translucent FX changed model depth or paired focus-mark state");
                for(unsigned n=0;n<512&&owner.source(normal.instance);++n) { tick(); if(n==28||n==56) draw(captures[4],true,false,nullptr); }
                require(!owner.source(normal.instance)&&owner.source(hidden.instance)&&owner.registered(hidden.instance),
                    "normal source cleanup or hidden self-replacement lost native scene lifetime");
                // common1003 uses generic 800a1608, not a Pokemon photo-gated
                // wrapper. The real prefix must save NULL without inventing a
                // subject, even though this object is present in photo inputs.
                for(unsigned context=0;context<2;++context) {
                    const auto history=detector.history(static_cast<std::uint16_t>(context));
                    require(history.size()==1&&!history.front().object&&!detector.computed().present,
                        "nongated source common1003 fabricated a photo subject or lost the actual GPU NULL baseline");
                }
                const auto moving=owner.effects().create_particle(3,0,{17,29,41},{2,3,4});
                require(bool(moving.status)&&moving.spawned,"World shift actual particle fixture failed");
                menu::EffectParticleView before_effect; require(owner.effects().particle(0,before_effect),"actual shared particle borrow failed");
                const auto rng=services.rng.seed; auto* old_reference=world.current(); const auto old_position=old_reference->resource->metadata.world_position;
                bindings.previous_reference=old_reference;
                auto* next=world.enter_next(); require(next&&next!=old_reference,"composed actual World advance failed");
                const auto new_position=next->resource->metadata.world_position;
                menu::EffectParticleView after_effect; require(owner.effects().particle(0,after_effect),"World shift lost real particle");
                require(same(after_effect.position,{before_effect.position.x+(old_position.x-new_position.x)*100,
                    before_effect.position.y+(old_position.y-new_position.y)*100,before_effect.position.z+(old_position.z-new_position.z)*100})&&
                    same(before_effect.velocity,after_effect.velocity)&&services.rng.seed==rng&&bindings.actor_shifts&&bindings.effect_shifts,
                    "source World shift did not preserve shared actual FX velocity/RNG and actor placement ownership");
                // The camera moved outside OM. Finish a real native update before
                // rendering: the existing priority0 sky-follow process must see it.
                // The instantaneous FX translation/RNG invariant above precedes this tick.
                tick();
                draw(captures[4],true,false,"actual-world-next-block-shared-fx.ppm");
                accepted(owner.cleanup(hidden.instance)); accepted(owner.collect_joined());
                draw(captures[4],true,false,"actual-world-after-native-cleanup.ppm");
                require(frame->retire()&&frame->quiescent(),"composed renderer retirement lacked a real release boundary");
                actor_producer.retire(); world_producer.retire();
                world.retire(); require(!services.failed(),services.error());
                total_draws=draws;
                bindings.owner=nullptr;
            }
            environment.cleanup();
        }
        native(storage.destroy());
        require(input.stop().error==game_input::Error::none,"owned neutral controller stop failed");
    }
    require(session.stop()==audio::Error::none,"composed actual audio stop");
    for(unsigned n=0;n<240&&session.state()!=game_audio::Session::State::draining;++n) pump();
    require(session.state()==game_audio::Session::State::draining&&session.service().voices_retired(),"composed actual audio retirement");
    require(session.poll_drain()==audio::Error::none&&session.poll_drain()==audio::Error::none&&session.state()==game_audio::Session::State::drained,
            "composed owned PCM drain failed");
    std::cout<<"composed_actual_gpu_frames="<<total_draws<<" source_world_actor_FX_NULLbaseline_cleanup=passed gated_subject_focus_full_game_Original_Windows11=not_claimed\n";
}
}
int main(int argc,char** argv) {
    try {
        require(argc==4,"usage: course-scene-rsp-smoke PRIVATE_ROM EMPTY_PRIVATE_PROFILE OUTPUT_DIRECTORY");
        auto snapshot=isle::rom::read_snapshot(argv[1]); run(snapshot,argv[2],argv[3]); return 0;
    } catch(const std::exception& error) { std::cerr<<"course-scene-rsp-smoke: "<<error.what()<<'\n'; return 1; }
}
