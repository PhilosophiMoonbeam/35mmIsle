#include "beach_actor_scene.hpp"
// Opt-in private-ROM COMPONENT consumer. This executes the complete original-C
// camera component, not original initUI/Root106, a second Player FSM, game entry,
// physical-device probe, or a claim of hardware/Windows parity.
// Usage: player-source-renderer-rsp-smoke PRIVATE_ROM NEW_OUTPUT_DIRECTORY
#include "player_renderer.hpp"
#include "player_commands.hpp"
#include "player_ui.hpp"
#include "player_cart.hpp"
#include "actor_photo_scene.hpp"
#include "course_model_producer.hpp"
#include "effect_bank_resources.hpp"
#include "actual_player_source.hpp"
#include "actual_audio_driver.hpp"
#include "scene_storage.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <filesystem>
#include <cmath>
#include <fstream>
#include <iostream>
#include <source_location>
#include <iterator>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {
using namespace isle;
namespace g=scene::graphics;
namespace r=scene::render;
namespace a=menu::animation;
constexpr std::size_t plane=320*240*2;
constexpr menu::RamRange line_arena{0x7d0000,0x2000};
constexpr world::ProducerLayout world_layout{{0x200000,0x10000},{0x210000,0x10000},{0x220000,0x10000}};
constexpr actor::ProducerLayout actor_layout{{0x400000,0x80000},{0x480000,0x80000},{0x500000,0x100000},
    {{{0x3b5000,plane},{0x3da800,plane}}}};
constexpr menu::model::PacketLayout item_layout{{0x230000,0x10000},{0x240000,0x10000},{0x250000,0x10000},
    {{{0x3b5000,plane},{0x3da800,plane}}}};
constexpr player::cart::ProducerLayout cart_layout{{0x260000,0x10000},{0x270000,0x10000},{0x2a0000,0x10000},
    {{{0x3b5000,plane},{0x3da800,plane}}}};
constexpr focus::CaptureLayout capture_layout{{0x680000,0x50},{0x680080,0x80},{0x680100,0xa00}};
constexpr menu::RamRange fx_commands{0x680c00,0x5400},joined_lists{0x686000,0x2800},guard_range{0x7f0000,256};
constexpr sprite::RamRange lanes{0x690000,0x40000},sprite_commands{0x780000,0x50000};
constexpr g::Layout frame_layout{{0x626000,0x2000},{0x630000,0x10000},{0x640000,0x40000},0x600000};
constexpr IsleGameLookAt view{{0,1600,6000},{0,500,0},{0,1,0}};
void require(bool value,const char* message) {
    if(!value) {
        std::cerr<<"source renderer requirement: "<<message<<'\n';
        throw std::runtime_error(message);
    }
}
void native(IsleGameResult result,std::source_location location=std::source_location::current()) {
    if(result!=ISLE_GAME_OK) {
        const auto message=std::string(location.file_name())+":"+
            std::to_string(location.line())+" source renderer result="+std::to_string(int(result));
        std::cerr<<message<<'\n';
        throw std::runtime_error(message);
    }
}
template<class Status> void accepted(Status value) {require(bool(value),value.detail);}
std::uint32_t physical(std::uint32_t value) {return value&0x1fffffffu;}
std::uint32_t word(std::span<const std::byte> ram,std::uint32_t address) {
    require(address<=ram.size()-4,"wire evidence escaped real completed mapping");
    std::uint32_t value=0;for(unsigned i=0;i<4;++i) value=(value<<8)|std::to_integer<unsigned>(ram[(address+i)^3u]);
    return value;
}
void save(const std::filesystem::path& path,std::span<const std::byte> bytes) {
    require(!std::filesystem::exists(path),"private renderer evidence would overwrite an artifact");
    std::ofstream file(path,std::ios::binary);file.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));
    require(bool(file),"source renderer artifact write failed");
}
struct Audio {
    game_audio::Session& session;private_audio::Execution& execution;std::uint64_t retrace=0;
    void tick() {
        ++retrace;require(session.retrace(retrace,retrace*1000000000ull/60)==audio::Error::none,"source audio retrace failed");
        require(session.submit()==audio::Error::none,"source audio submit failed");audio::TaskView task;
        require(session.build(task)==audio::Error::none,"source audio task build failed");
        if(task.token.serial) {private_audio::execute(session,execution,task);require(session.submit()==audio::Error::none,"completed source audio submit failed");}
    }
};
struct Hooks {
    scene::Services& services;IsleGameCamera* camera;actor::Owner* actors=nullptr;items::Owner* items=nullptr;
    static bool remove(void* user,world::Block& block) noexcept {
        auto& self=*static_cast<Hooks*>(user);return !self.actors||bool(self.actors->remove_block(block));
    }
    static bool camera_shift(void* user,world::Vec3 delta) noexcept {
        auto& self=*static_cast<Hooks*>(user);const IsleGameCameraParameters* current=nullptr;
        if(!self.services.check(isle_game_camera_parameters(self.services.game,self.camera,&current),"source camera shift borrow")) return false;
        auto next=current->look_at;const std::array shift{delta.x,delta.y,delta.z};
        for(unsigned i=0;i<3;++i) {next.eye[i]+=shift[i];next.at[i]+=shift[i];}
        return self.services.check(isle_game_camera_set_look_at(self.services.game,self.camera,&next),"source camera shift");
    }
    static bool effects_shift(void* user,world::Vec3 delta) noexcept {
        auto& self=*static_cast<Hooks*>(user);
        if(self.actors) {self.actors->effects().translate({delta.x,delta.y,delta.z});if(!self.actors->effects().status()) return false;}
        return !self.items||bool(self.items->displace({delta.x,delta.y,delta.z}));
    }
};
struct Owners {
    actor::Owner& actors;items::Owner& items;player::cart::Owner& cart;world::World& world;
    ~Owners() {
        const auto actor_retired=actors.reset();const auto items_retired=items.retire();const auto cart_retired=cart.retire();
        const auto actor_joined=actors.collect_joined();const auto item_joined=items.collect_joined();const auto cart_joined=cart.collect_joined();
        if(!actor_retired||!items_retired||!cart_retired||!actor_joined||!item_joined||!cart_joined) {
            std::cerr<<"source renderer owner retirement/join failure: actors="<<bool(actor_retired)<<'/'<<bool(actor_joined)
                <<" items="<<bool(items_retired)<<'/'<<bool(item_joined)
                <<" cart="<<bool(cart_retired)<<'/'<<bool(cart_joined)<<'\n';
            std::terminate();
        }
        if(!world.retire()) {
            std::cerr<<"source renderer World retirement failure\n";
            std::terminate();
        }
    }
};
struct Content {
    course::ModelProducer* models=nullptr;
    focus::RegionCopies& copies;
    actor::Owner* actors=nullptr;items::Owner* items=nullptr;player::cart::Owner* cart=nullptr;
    std::array<scene::RenderOrderEntry,128> cart_order{};
    unsigned calls=0;
    bool fail_after_prefix=false;
    static bool prepare(void* user,std::span<std::uint32_t> ram,std::uint64_t epoch,const r::Pass& pass,r::Prepared& output) {
        auto& self=*static_cast<Content*>(user);
        require(self.models&&self.actors&&self.items&&self.cart,"real source content not attached before dispatch");
        require(self.copies.install(ram)==focus::CaptureStatus::ok,"actual capture regions did not install");
        self.actors->render_bindings();self.items->render_bindings();
        output=self.models->prepare(ram,epoch,pass);++self.calls;
        // A real producer has captured the prefix and committed history. Reject
        // this preparation before original ProcessImage; never fake completion.
        return !self.fail_after_prefix;
    }
};
// Explicit MICROCODE/LINEDRAW diagnostic, not authored GAME line geometry.
// Vertices remain the actual admitted Cart bytes at their actual ROM-backed
// physical address. Only genuine native camera matrices and bounded GBI packets
// are written; no synthetic vertex, Actor, Player field or Root is substituted.
// Public scalar wire semantics: Nintendo gSPLine3D (1999-02-01),
// https://ultra64.ca/files/documentation/online-manuals/man-v5-1/n64man/gsp/gSPLine3D.htm
struct LineProbe {
    const photo::LightingInputs& lighting;
    player::Renderer& renderer;
    scene::Services& services;
    menu::VertexLoad vertices{};
    IsleGameObject* object=nullptr;
    IsleGameCamera* camera=nullptr;
    unsigned mask=1;
    bool task_line=false;
    LineProbe(scene::Services& owner,const player::CartResources& resources,
        const photo::LightingInputs& inputs,player::Renderer& domain):lighting(inputs),renderer(domain),services(owner) {
        const auto signed_half=[](std::span<const std::byte> bytes,std::size_t at) {
            return std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(
                (std::to_integer<unsigned>(bytes[at])<<8)|std::to_integer<unsigned>(bytes[at+1])));
        };
        bool found=false;
        IsleGameVec3 middle{};
        for(const auto& load:resources.vertex_loads()) {
            if(load.count<2||load.count>32||load.wire.bytes.size()<std::size_t(load.count)*16) continue;
            const auto x0=signed_half(load.wire.bytes,0),y0=signed_half(load.wire.bytes,2),z0=signed_half(load.wire.bytes,4);
            const auto x1=signed_half(load.wire.bytes,16),y1=signed_half(load.wire.bytes,18),z1=signed_half(load.wire.bytes,20);
            if((x0==x1&&y0==y1)||std::abs(int(x0))>4000||std::abs(int(x1))>4000||
                std::abs(int(y0))>4000||std::abs(int(y1))>4000||std::abs(int(z0))>4000||std::abs(int(z1))>4000) continue;
            vertices=load;middle={(float(x0)+float(x1))*0.5f,(float(y0)+float(y1))*0.5f,(float(z0)+float(z1))*0.5f};found=true;break;
        }
        require(found,"admitted Cart has no bounded nondegenerate source pair for the explicit line diagnostic");
        object=services.object(0x7001000,0);require(object,"native line diagnostic camera creation failed");
        native(isle_game_attach_camera(services.game,object,&camera));
        native(isle_game_camera_add_matrix(services.game,camera,ISLE_GAME_CAMERA_PERSPECTIVE_FAST));
        native(isle_game_camera_add_matrix(services.game,camera,ISLE_GAME_CAMERA_LOOK_AT));
        const IsleGamePerspective perspective{45,4.0f/3.0f,1,16384,1};
        const IsleGameLookAt pose{{middle.x,middle.y,middle.z+12000},{middle.x,middle.y,middle.z},{0,1,0}};
        native(isle_game_camera_set_perspective(services.game,camera,&perspective));
        native(isle_game_camera_set_look_at(services.game,camera,&pose));
    }
    static bool prepare(void* user,std::span<std::uint32_t> ram,std::uint64_t,const r::Pass&,r::Prepared& output) {
        auto& self=*static_cast<LineProbe*>(user);graphics::Memory memory(ram);
        if(self.task_line) {
            self.lighting.install(std::as_writable_bytes(ram));
            self.lighting.install_direction(std::as_writable_bytes(ram),self.renderer.light_direction());
        }
        native(isle_game_camera_compute_all(self.services.game,self.camera));
        const IsleGameCameraMatrix* projection=nullptr;const IsleGameCameraMatrix* view_matrix=nullptr;
        native(isle_game_camera_matrix(self.services.game,self.camera,0,&projection));
        native(isle_game_camera_matrix(self.services.game,self.camera,1,&view_matrix));
        std::uint16_t normalization=0;native(isle_game_camera_perspective_normalization(self.services.game,self.camera,&normalization));
        const auto base=static_cast<std::uint32_t>(line_arena.offset);
        memory.matrix(base,projection->fixed);memory.matrix(base+64,view_matrix->fixed);
        std::uint32_t cursor=base+0x100,end=cursor+0x100;
        const auto emit=[&](std::uint32_t first,std::uint32_t second) {
            require(cursor<=end-8,"line diagnostic packet exceeded its cold reservation");
            memory.word(cursor,first);memory.word(cursor+4,second);cursor+=8;
        };
        const auto camera_begin=cursor;
        emit(0xda380007u,base);emit(0xdb0e0000u,normalization);emit(0xda380003u,base+64);
        emit(0xd5000001u,0);emit(0xd9000000u,0x00200004u);emit(0xd7000000u,0);
        emit(0xfcffffffu,0xfffe793cu);emit(0xdf000000u,0);
        output.camera={camera_begin,(cursor-camera_begin)/4};
        for(unsigned lane=0;lane<4;++lane) if(self.mask&(1u<<lane)) {
            const auto begin=base+0x200+lane*0x100;cursor=begin;end=begin+0x100;
            emit(0xe7000000u,0);emit(0xe200001cu,0x00507048u);
            emit(0xf7000000u,0x5a100000u|lane); // actual GPU lane-order marker.
            emit(0x01000000u|(std::uint32_t(self.vertices.count)<<12)|(std::uint32_t(self.vertices.count)<<1),
                physical(self.vertices.wire.guest_address));
            // Literal source lane1->3 keeps triangle microcode active: that
            // case probes an actual RDP state marker, NOT unsupported LINE3D
            // disguised as a successful line draw or a relocated load.
            if(self.task_line||lane==2||(lane==3&&!(self.mask&2u)))
                emit(0x08000200u,0); // actual source vertices0/1, width1.5, flat color0.
            emit(0xdf000000u,0);output.lanes[lane]={begin,(cursor-begin)/4};
        }
        return true;
    }
};
struct Evidence {
    const photo::LightingInputs& lighting;
    photo::LightDirection expected{};
    player::Renderer& renderer;
    IsleGameObject* main_object;
    player::Renderer::Dispatch dispatch{};
    std::uintptr_t mapping_identity=0;
    bool fail_completed=false;
    unsigned calls=0;std::size_t triangles=0,rectangles=0;
    std::vector<std::byte> color=std::vector<std::byte>(plane),depth=std::vector<std::byte>(plane);
    std::vector<std::uint32_t> packets;
    std::array<std::vector<std::uint32_t>,4> lane_words;
    std::vector<unsigned> gpu_order;
    std::vector<std::uint32_t> main_lane;
    std::vector<std::uint32_t> prefix_words;
    Evidence(const photo::LightingInputs& input,player::Renderer& owner,IsleGameObject* main):
        lighting(input),expected(owner.light_direction()),renderer(owner),main_object(main) {}
    static void complete(void* user,rsp::CanonicalExecutor::MemoryLease& lease,std::uint32_t framebuffer) {
        auto& self=*static_cast<Evidence*>(user);require(++self.calls==1,"source frame acquired multiple completed mappings");
        const auto ram=lease.rdram();require(ram.size()==8*1024*1024,"source completed mapping is not actual full RDRAM");
        for(unsigned i=0;i<3;++i) require(static_cast<std::int8_t>(std::to_integer<unsigned>(ram[(physical(self.lighting.direction_guest())+i)^3u]))==self.expected[i],
            "source OM light bytes were recomputed, reordered or not installed exactly");
        const auto& packets=lease.packets();
        for(const auto packet:packets.packets()) {
            const auto words=packets.words().subspan(packet.offset,packet.count);require(!words.empty(),"empty real RDP packet");
            self.packets.insert(self.packets.end(),words.begin(),words.end());const auto op=(words[0]>>24)&63u;
            self.triangles+=op>=8&&op<=15;self.rectangles+=op==0x24||op==0x25;
            if(op==0x37&&words.size()>=2&&(words[1]&0xfffffffcu)==0x5a100000u)
                self.gpu_order.push_back(words[1]&3u);
        }
        for(std::size_t i=0;i<plane;++i) {self.color[i]=ram[(framebuffer+i)^3u];self.depth[i]=ram[(frame_layout.depth+i)^3u];}
        self.mapping_identity=reinterpret_cast<std::uintptr_t>(ram.data());
        native(self.renderer.dispatch_metadata(self.main_object,self.dispatch));
        const auto append=[&](r::List list) {
            for(std::size_t offset=0;offset<list.words;++offset)
                self.main_lane.push_back(word(ram,list.address+static_cast<std::uint32_t>(offset*4)));
        };
        for(std::size_t offset=0;offset<self.dispatch.prefix.words;++offset)
            self.prefix_words.push_back(word(ram,self.dispatch.prefix.address+static_cast<std::uint32_t>(offset*4)));
        for(unsigned lane=0;lane<4;++lane) {
            const auto list=self.dispatch.lanes[lane];append(list);
            for(std::size_t offset=0;offset<list.words;++offset)
                self.lane_words[lane].push_back(word(ram,list.address+static_cast<std::uint32_t>(offset*4)));
        }
        require(!lease.postprocess_active(),"source completed consumer retained exclusive Photo mapping");
        if(self.fail_completed) throw std::runtime_error("intentional actual completed-memory failure after Photo analysis");
    }
    bool contains(std::uint32_t first,std::uint32_t second) const {
        for(std::size_t i=0;i+1<main_lane.size();i+=2) if(main_lane[i]==first&&main_lane[i+1]==second) return true;
        return false;
    }
    bool prefix_contains(std::uint32_t first,std::uint32_t second) const {
        for(std::size_t i=0;i+1<prefix_words.size();i+=2) if(prefix_words[i]==first&&prefix_words[i+1]==second) return true;
        return false;
    }
};
// Isolated actual producer/GPU regression: deliberately NOT SourceMain's image
// request path and NOT a Controller preparation window. PhotoScene's genuine
// membership hook and both Detector history generations are the observables.
void hidden_prefix_regression(scene::Services& services,g::Frame& frame,Content& content,
    const photo::LightingInputs& lighting,player::Renderer& renderer,IsleGameCamera* camera,
    actor::PhotoScene& photo_scene,actor::DetectorBinding& detection,focus::Detector& detector,
    player::cart::Owner& cart,const player::Renderer::Config& config,const std::filesystem::path& output) {
    struct Prepare {
        Content& content;const photo::LightingInputs& lighting;player::Renderer& renderer;
        static bool run(void* user,std::span<std::uint32_t> ram,std::uint64_t epoch,const r::Pass& pass,r::Prepared& result) {
            auto& self=*static_cast<Prepare*>(user);
            self.lighting.install(std::as_writable_bytes(ram));
            self.lighting.install_direction(std::as_writable_bytes(ram),self.renderer.light_direction());
            return Content::prepare(&self.content,ram,epoch,pass,result);
        }
    } prepare{content,lighting,renderer};
    struct Readback {
        std::size_t triangles=0;unsigned calls=0;
        static void complete(void* user,rsp::CanonicalExecutor::MemoryLease& lease,std::uint32_t) {
            auto& self=*static_cast<Readback*>(user);++self.calls;
            const auto& packets=lease.packets();
            for(const auto packet:packets.packets()) {
                const auto op=(packets.words()[packet.offset]>>24)&63u;self.triangles+=op>=8&&op<=15;
            }
            require(!lease.postprocess_active(),"isolated prefix regression retained a writable mapping");
        }
    };
    const auto run=[&](const char* artifact) {
        content.cart->render_bindings(content.cart_order);auto borrow=cart.borrow();require(bool(borrow),"isolated prefix GPU Cart borrow rejected");
        r::Pass pass;pass.camera=camera;pass.flags=3;pass.priority=5;pass.camera_tag=UINT32_MAX;pass.link_mask=0x38;
        pass.prepare=Prepare::run;pass.userdata=&prepare;
        Readback readback;require(frame.begin({services.game,config.resources,{&pass,1},{}},{0},{&readback,Readback::complete}),frame.message());
        g::Poll result;do {result=frame.poll();if(result==g::Poll::pending) std::this_thread::yield();} while(result==g::Poll::pending);
        require(result==g::Poll::completed&&readback.calls==1&&readback.triangles,
            "isolated prefix visibility case did not execute complete real World/Actor/Item/Cart GPU geometry");
        save(output/artifact,frame.completed().rgba);
    };
    auto* prefix=photo_scene.prefix_object();IsleGameObjectMetadata metadata{};
    native(isle_game_object_metadata(services.game,prefix,&metadata));
    const auto context=detection.context;const std::array generations{detector.generation(0),detector.generation(1)};
    const auto membership=detection.photo;
    native(isle_game_object_set_flags(services.game,prefix,metadata.flags|1u));
    run("hidden-prefix-producer.rgba8");
    const bool hidden_unchanged=detection.context==context&&detector.generation(0)==generations[0]&&
        detector.generation(1)==generations[1]&&detection.photo.objects==membership.objects&&detection.photo.focus==membership.focus;
    native(isle_game_object_set_flags(services.game,prefix,metadata.flags));
    require(hidden_unchanged,"hidden actual PhotoScene prefix still invoked membership hook or flipped/staged Detector history");
    run("visible-prefix-producer.rgba8");
    const auto next=static_cast<std::uint16_t>(1-context);
    require(detection.context==next&&detector.generation(next)==generations[next]+1,
        "restored actual prefix visibility did not stage the genuine membership/history context on actual GPU preparation");
}
template<class Tick>
void source_queue_cases(scene::Services& services,g::Frame& frame,
    player::Renderer& renderer,player::Controller& controller,player::Photo& photo,
    IslePlayerSource& source,IslePlayerPhotoSource& application,actor::PhotoScene& photo_scene,
    actor::DetectorBinding& detection,focus::Detector& detector,std::span<IsleGameObject* const> cameras,
    const std::filesystem::path& output,Tick&& tick) {
    const auto encoded=[&](std::uint16_t context) {
        std::array<std::byte,0x3a0> bytes{};
        player::encode_photo(application.PokemonDetector_Photo[context].main,bytes);return bytes;
    };
    const auto same=[](const focus::Focus& a,const focus::Focus& b) {
        return a.present==b.present&&a.object==b.object&&a.pokemon_id==b.pokemon_id&&
            a.flags==b.flags&&a.context==b.context;
    };
    auto* prefix=photo_scene.prefix_object();IsleGameObjectMetadata prefix_metadata{};
    native(isle_game_object_metadata(services.game,prefix,&prefix_metadata));
    const auto hide_prefix=[&](bool hidden) {
        native(isle_game_object_set_flags(services.game,prefix,prefix_metadata.flags|(hidden?1u:0u)));
    };
    struct SharedRows {
        const std::string& name;focus::Detector& detector;IslePlayerPhotoSource& source;
        focus::Focus before{};std::uint16_t context=0;bool analyze=true;unsigned calls=0;
        static void complete(void* user,rsp::CanonicalExecutor::MemoryLease& lease,std::uint32_t) {
            auto& self=*static_cast<SharedRows*>(user);
            require(++self.calls==1,"source FIFO draw acquired multiple completed mappings");
            auto expected=self.before;
            if(self.analyze) {
                expected.context=self.context;expected.present=false;
                const auto ram=lease.rdram();const auto subjects=self.detector.history(self.context);
                const auto pixel=[&](std::size_t row,std::size_t i) {
                    const auto address=static_cast<std::uint32_t>(capture_layout.regions.offset+row*128+i*2);
                    const auto value=word(ram,address&~3u);return (value>>((address&2u)?0u:16u))&0xffffu;
                };
                for(std::size_t row=1;row<subjects.size();++row) if(subjects[row].object) {
                    unsigned changed=0;for(std::size_t i=0;i<64;++i) changed+=pixel(row-1,i)!=pixel(row,i);
                    if(changed>=32) {static_cast<focus::Subject&>(expected)=subjects[row];expected.present=true;}
                }
                if(expected.present&&expected.object) {
                    const auto id=expected.pokemon_id;const auto& photo=self.source.PokemonDetector_Photo[self.context];
                    const bool sign=id==1004||id==1018||id==1022||id==1035;
                    const bool member=std::any_of(std::begin(photo.pokemonObjects),std::end(photo.pokemonObjects),
                        [&](IsleGameObject* object){return reinterpret_cast<focus::Object>(object)==expected.object;});
                    if(!sign&&!member) {expected.present=false;expected.object=0;}
                } else {expected.present=false;expected.object=0;}
            }
            const auto& actual=self.detector.computed();
            const bool matches=actual.present==expected.present&&actual.object==expected.object&&actual.pokemon_id==expected.pokemon_id&&
                actual.flags==expected.flags&&actual.context==expected.context;
            if(!matches) std::cerr<<"source FIFO observer case: "<<self.name<<'\n';
            require(matches,"actual callback did not analyze latest context metadata against THIS completed shared 20-region RAM");
            require(!lease.postprocess_active(),"source FIFO draw retained an exclusive Photo mapping");
        }
    };
    const auto draw=[&](const std::string& name,bool analyze=true,std::uint16_t head=UINT16_MAX,bool remove_at_submit=false) {
        const auto before=std::bit_cast<std::uint32_t>(renderer.drawn_frame_counter());
        // Queue/counter observations also cover genuine presentation-only
        // blink callbacks; they do not require a 3D camera's light/DL payload.
        SharedRows rows{name,detector,application,detector.computed(),head,analyze};
        require(renderer.begin(frame,{0},{&rows,SharedRows::complete}),renderer.message());
        if(head==UINT16_MAX) rows.context=detection.context;
        if(remove_at_submit) native(renderer.set_postprocess(false));
        require(frame.busy()&&std::bit_cast<std::uint32_t>(renderer.drawn_frame_counter())==before+1u,
            "signed source counter did not advance once after whole draw returned before GPU completion");
        const auto admitted=renderer.drawn_frame_counter();
        g::Poll result;do {result=renderer.poll();if(result==g::Poll::pending) std::this_thread::yield();} while(result==g::Poll::pending);
        if(result!=g::Poll::completed) std::cerr<<"source FIFO draw failed in "<<name<<": "<<renderer.message()<<'\n';
        require(result==g::Poll::completed&&rows.calls==1&&renderer.drawn_frame_counter()==admitted,
            "actual GPU completion failed or advanced the independent Source draw counter");
        save(output/(name+".rgba8"),frame.completed().rgba);
        return controller.ticket(frame);
    };
    const auto publish_release=[&](IslePlayerPhotoTicket ticket) {
        const auto before=renderer.drawn_frame_counter();tick();
        require(renderer.drawn_frame_counter()==before&&detector.published().context==ticket.context,
            "later original OM CopyInfo changed draw counter or published the wrong analyzed context");
        application.PokemonDetector_TakenPhotoIndex=ticket.context;
        IslePlayerPhotoTicket borrowed{};native(isle_player_photo_borrow_taken(&application,&borrowed));
        require(borrowed==ticket&&renderer.release_completed()==ISLE_GAME_INVALID_STATE,
            "Source shutter borrow did not retain the actual analyzed metadata/history until release");
        native(isle_player_photo_release_taken(&application,&borrowed));native(renderer.release_completed());
    };
    // Genuine Create/BSS context1: no prefix capture, no actor-region saves.
    require(detection.context==1&&!detector.generation(0)&&!detector.generation(1),
        "empty-context scenario was not entered from actual Create/BSS");
    detection.enabled=false;hide_prefix(true);
    const auto empty=draw("source-initial-empty");
    const auto empty_metadata=encoded(1);
    require(empty.context==1&&!empty.generation&&empty.submission&&controller.has_pending(frame)&&
        !detector.generation(0)&&!detector.generation(1)&&detector.history(1).empty()&&
        !detector.computed().present&&controller.queued_requests()==0&&
        std::all_of(empty_metadata.begin(),empty_metadata.end(),[](std::byte value){return value==std::byte{0};}),
        "initial ProcessImage fabricated a capture/generation or lost its real empty-context request");
    publish_release(empty);
    // Capture once, then process the unchanged canonical context after the live
    // camera changes while the real prefix stays hidden.
    detection.enabled=true;hide_prefix(false);const auto seeded=draw("source-existing-seed");
    const auto snapshot=encoded(seeded.context);publish_release(seeded);
    const std::array generations{detector.generation(0),detector.generation(1)};
    const auto original=source.camera;const IsleGameCameraParameters* parameters=nullptr;
    native(isle_game_camera_parameters(services.game,original,&parameters));const auto old_view=parameters->look_at;
    auto changed=old_view;changed.eye[0]+=200;changed.at[0]+=200;
    native(isle_game_camera_set_look_at(services.game,original,&changed));
    detection.enabled=false;hide_prefix(true);const auto existing=draw("source-existing-no-prefix");
    require(existing.context==seeded.context&&existing.generation==seeded.generation&&
        existing.submission!=seeded.submission&&encoded(existing.context)==snapshot&&
        detector.generation(0)==generations[0]&&detector.generation(1)==generations[1],
        "no-prefix ProcessImage recaptured changed live camera state or conflated GPU and metadata identities");
    publish_release(existing);
    // Remove ONLY the callback. Four real draws fill the two-ID NOBLOCK FIFO,
    // overwrite both unbound CPU contexts and reuse the actual Frame/RAM.
    detection.enabled=true;hide_prefix(false);
    const auto prior_computed=detector.computed(),prior_published=detector.published();
    std::uint16_t first_context=0;std::uint64_t first_generation=0;std::array<std::byte,0x3a0> first_metadata{};
    for(unsigned n=0;n<4;++n) {
        changed=old_view;changed.eye[0]+=float(10*(n+1));changed.at[0]+=float(10*(n+1));
        native(isle_game_camera_set_look_at(services.game,original,&changed));
        draw("source-callback-removed-"+std::to_string(n),false,UINT16_MAX,n==0);
        if(!n) {first_context=detection.context;first_generation=detector.generation(first_context);first_metadata=encoded(first_context);}
        require(controller.queued_requests()==std::min<std::size_t>(n+1,2)&&!controller.has_pending(frame)&&
            photo.quiescent()&&detector.context_available(0)&&detector.context_available(1)&&
            same(detector.computed(),prior_computed)&&same(detector.published(),prior_published),
            "callback removal analyzed/dropped a request, disabled captures or stranded successful GPU/history holds");
    }
    require(detector.generation(first_context)==first_generation+1&&encoded(first_context)!=first_metadata,
        "queued ID pinned its obsolete enqueue-generation metadata instead of allowing genuine Source overwrite");
    const auto latest=encoded(first_context);const auto latest_generation=detector.generation(first_context);
    // No matching Source task means no callback, not last retired token replay.
    struct NoSourceTask {
        player::Renderer& renderer;std::span<IsleGameObject* const> cameras;
        std::array<std::uint64_t,4> epochs{};unsigned calls=0;
        static void complete(void* user,rsp::CanonicalExecutor::MemoryLease& lease,std::uint32_t) {
            auto& self=*static_cast<NoSourceTask*>(user);
            require(++self.calls==1,"no-source GPU draw acquired multiple completed mappings");
            for(std::size_t n=0;n<self.cameras.size();++n) {
                player::Renderer::Dispatch last{};native(self.renderer.dispatch_metadata(self.cameras[n],last));
                require(last.epoch==self.epochs[n],"hidden camera fabricated a new Source dispatch");
            }
            require(!lease.postprocess_active(),"no-source draw retained an exclusive Photo mapping");
        }
    } no_source{renderer,cameras};
    native(renderer.set_postprocess(true));std::array<std::uint32_t,4> flags{};
    require(cameras.size()==flags.size(),"no-task scenario needs every actual scheduling camera");
    for(std::size_t n=0;n<cameras.size();++n) {
        player::Renderer::Dispatch last{};native(renderer.dispatch_metadata(cameras[n],last));
        no_source.epochs[n]=last.epoch;
        IsleGameObjectMetadata metadata{};native(isle_game_object_metadata(services.game,cameras[n],&metadata));
        flags[n]=metadata.flags;native(isle_game_object_set_flags(services.game,cameras[n],metadata.flags|1u));
    }
    // No Source camera ran: its light installation/main-camera payload is not
    // this task's contract. Observe genuine GPU completion and unchanged last
    // Source dispatches instead; the queued IDs must survive the GFX_END.
    const auto before_empty=std::bit_cast<std::uint32_t>(renderer.drawn_frame_counter());
    require(renderer.begin(frame,{0},{&no_source,NoSourceTask::complete}),renderer.message());
    require(frame.busy()&&std::bit_cast<std::uint32_t>(renderer.drawn_frame_counter())==before_empty+1u,
        "whole no-source draw did not advance its independent counter before GPU completion");
    const auto empty_counter=renderer.drawn_frame_counter();
    g::Poll empty_result;do {empty_result=renderer.poll();if(empty_result==g::Poll::pending) std::this_thread::yield();} while(empty_result==g::Poll::pending);
    require(empty_result==g::Poll::completed&&no_source.calls==1&&renderer.drawn_frame_counter()==empty_counter,
        "no-source GPU completion failed or advanced the independent draw counter");
    save(output/"source-no-matching-task.rgba8",frame.completed().rgba);
    require(controller.queued_requests()==2&&!controller.has_pending(frame)&&same(detector.computed(),prior_computed),
        "GFX_END without a matching Source task reused an old token/callback and consumed Detector FIFO");
    for(std::size_t n=0;n<cameras.size();++n) native(isle_game_object_set_flags(services.game,cameras[n],flags[n]));
    // The current context differs from FIFO head; callback consumes latest head
    // metadata against the one shared 20-region array in this actual mapping.
    detection.enabled=false;hide_prefix(true);changed.eye[0]+=700;
    native(isle_game_camera_set_look_at(services.game,original,&changed));
    const auto resumed=draw("source-callback-reenabled",true,first_context);
    require(resumed.context==first_context&&resumed.context!=detection.context&&
        resumed.generation==latest_generation&&encoded(first_context)==latest&&
        detector.computed().context==first_context&&controller.queued_requests()==1,
        "re-enabled actual callback froze old metadata/pixels or consumed current context instead of FIFO head ID");
    publish_release(resumed);
    // A genuine original blink emits a presentation task but no ProcessImage.
    // Its eligible callback can drain the second old ID without a new prefix.
    require(services.send_command(cameras[0],ISLE_PLAYER_CAMERA_BLINK),"queued-drain blink FIFO enqueue failed");tick();
    const auto drained=draw("source-queued-blink-callback");
    require(drained.context==detection.context&&controller.queued_requests()==0,
        "registered presentation callback did not drain queued Source ID during a no-request blink draw");
    publish_release(drained);
    native(isle_game_camera_set_look_at(services.game,original,&old_view));hide_prefix(false);detection.enabled=true;
}
struct LateUi {
    scene::Services& services;player::Ui& ui;g::Frame& frame;
    std::vector<sprite::RamRange> protected_ranges;
    std::array<scene::SpriteDraw,128> views{};
    std::array<sprite::Draw,128> draws{};
    IsleGameObject* film=nullptr;
    std::uint32_t old_flags=0;
    std::size_t before=0,after=0;
    unsigned calls=0;
    LateUi(scene::Services& s,player::Ui& u,g::Frame& f,const player::Renderer::Config& config):
        services(s),ui(u),frame(f),film(u.source().D_803B09D8_550DE8) {
        require(film,"late UI diagnostic requires the actual original film owner");
        for(const auto range:config.resources.reservations)
            if(range.address!=line_arena.offset||range.size!=line_arena.size) protected_ranges.push_back(range);
        for(const auto range:frame.reservations()) protected_ranges.push_back(range);
        for(auto target:config.resources.framebuffers) protected_ranges.push_back({target,plane});
        IsleGameObjectMetadata metadata{};native(isle_game_object_metadata(services.game,film,&metadata));old_flags=metadata.flags;
    }
    static bool prepare(void* user,std::span<std::uint32_t> ram,std::uint64_t epoch,const r::Pass&,r::Prepared& output) {
        auto& self=*static_cast<LateUi*>(user);++self.calls;
        require(self.services.sprite_draws(self.views,self.before),"early actual UI projection failed");
        native(isle_game_object_set_flags(self.services.game,self.film,self.old_flags|1u));
        require(self.services.sprite_draws(self.views,self.after)&&self.after<self.before,
            "native film hidden flag did not change actual same-frame sprite views");
        for(std::size_t i=0;i<self.after;++i) self.draws[i]=self.views[i].draw;
        const auto list=self.ui.producer().prepare(ram,epoch,std::span<const sprite::Draw>(self.draws).first(self.after),{},
            {static_cast<std::uint32_t>(line_arena.offset),static_cast<std::uint32_t>(line_arena.size)},self.protected_ranges);
        output.lanes[0]={list.address,list.words};
        return true;
    }
};
std::unique_ptr<g::Frame> line_diagnostics(rom::Snapshot& snapshot,scene::Services& services,
    player::Renderer& renderer,IslePlayerSource& source,g::Frame& primary,const player::CartResources& cart,
    const photo::LightingInputs& lighting,const player::Renderer::Config& config,
    std::span<IsleGameObject* const> ordinary,const std::filesystem::path& output) {
    auto made=g::Frame::create(snapshot,{true,{16000000,4*1024*1024},frame_layout});
    require(bool(made),made.message.data());require(renderer.reserve(*made.frame),made.frame->message());
    LineProbe probe(services,cart,lighting,renderer);
    player::Renderer::Camera camera{probe.object,probe.camera,isle_player_render_camera_render,
        isle_player_mainCameraPreRender,5,5,0,0,UINT32_MAX};
    camera.prepare=LineProbe::prepare;camera.prepare_user=&probe;native(renderer.attach(camera));
    struct NativeGuard {
        scene::Services& services;player::Renderer& renderer;g::Frame& primary;g::Frame& secondary;IsleGameObject* object;
        IsleGameObject* deferred=nullptr;
        ~NativeGuard() {
            while(primary.busy()) if(renderer.poll()==g::Poll::pending) std::this_thread::yield();
            while(secondary.busy()) if(secondary.poll()==g::Poll::pending) std::this_thread::yield();
            if(deferred) services.erase(deferred);
            services.erase(object);if(renderer.collect_joined()!=ISLE_GAME_OK) std::terminate();
        }
    } guard{services,renderer,primary,*made.frame,probe.object};
    // Actual initial task8, not a task2 descriptor or host-side program flag.
    probe.task_line=true;
    r::Pass pass;pass.camera=probe.camera;pass.flags=3;pass.priority=5;pass.background_rgba=0x000000ff;
    pass.prepare=LineProbe::prepare;pass.userdata=&probe;
    Evidence initial{lighting,renderer,probe.object};
    require(made.frame->begin({services.game,config.resources,{&pass,1},{},graphics::SourceProgram::l3dex2_fifo},
        {0},{&initial,Evidence::complete}),made.frame->message());
    g::Poll result;do {result=made.frame->poll();if(result==g::Poll::pending) std::this_thread::yield();} while(result==g::Poll::pending);
    require(result==g::Poll::completed&&initial.triangles&&initial.gpu_order==std::vector<unsigned>{0},
        "actual included task8 did not generate LINE3D RDP geometry from admitted source vertices");
    save(output/"line-task8.rgba8",made.frame->completed().rgba);
    probe.task_line=false;
    std::vector<std::uint32_t> flags;flags.reserve(ordinary.size());
    for(auto* object:ordinary) {IsleGameObjectMetadata metadata{};native(isle_game_object_metadata(services.game,object,&metadata));
        flags.push_back(metadata.flags);native(isle_game_object_set_flags(services.game,object,metadata.flags|1u));}
    std::ofstream report(output/"line-microcode-diagnostic.tsv");
    report<<"scope\tMICROCODE-LINEDRAW-native-camera-admitted-Cart-vertices-not-GAME-line-provider-parity\n";
    const auto line_program=config.programs->descriptor(graphics::SourceProgram::l3dex2_fifo);
    for(unsigned bits=0;bits<8;++bits) {
        probe.mask=1u|(bits<<1);
        Evidence evidence{lighting,renderer,probe.object};
        require(renderer.begin(primary,{bits&1u},{&evidence,Evidence::complete}),renderer.message());
        do {result=renderer.poll();if(result==g::Poll::pending) std::this_thread::yield();} while(result==g::Poll::pending);
        require(result==g::Poll::completed,"source four-lane diagnostic did not actually complete RSP/RDP execution");
        std::vector<unsigned> gpu{0},cpu{0};
        for(unsigned lane=1;lane<4;++lane) if(probe.mask&(1u<<lane)) cpu.push_back(lane);
        for(unsigned lane:{2u,1u,3u}) if(probe.mask&(1u<<lane)) gpu.push_back(lane);
        require(evidence.gpu_order==gpu&&evidence.dispatch.prerender_count==cpu.size()&&
            std::equal(cpu.begin(),cpu.end(),evidence.dispatch.prerender_order.begin()),
            "actual C CPU preRender order or GPU unpushed lane branch order differs");
        const auto executed_lines=unsigned(bool(probe.mask&4u))+unsigned(bool(probe.mask&8u)&&!(probe.mask&2u));
        require(evidence.triangles==initial.triangles*executed_lines,
            "source in-list triangle/line transitions did not execute the exact qualified LINE3D contexts");
        if((probe.mask&0xau)==0xau) {
            const auto& lane0=evidence.lane_words[0];const auto& lane1=evidence.lane_words[1];
            const auto& lane3=evidence.lane_words[3];std::size_t first_branch=lane0.size(),last_line_load=0;
            bool line_load=false,misplaced=false;
            for(std::size_t i=0;i+1<lane0.size();i+=2) {
                if(lane0[i]==0xde010000u) first_branch=std::min(first_branch,i);
                if(lane0[i]==0xdd0007ffu&&lane0[i+1]==line_program.text.address) {line_load=true;last_line_load=i;}
            }
            for(std::size_t i=0;i+1<lane1.size();i+=2)
                misplaced=misplaced||(lane1[i]==0xdd0007ffu&&lane1[i+1]==line_program.text.address);
            require(line_load&&last_line_load>first_branch&&!misplaced&&lane1.size()>=2&&lane3.size()>=2&&
                lane1[lane1.size()-2]==0xde010000u&&lane1.back()==evidence.dispatch.lanes[3].address&&
                lane3[lane3.size()-2]==0xde010000u&&lane3.back()==evidence.dispatch.lanes[0].address+
                    static_cast<std::uint32_t>(evidence.dispatch.lanes[0].words*4-16),
                "literal lane1->3 source quirk was relocated, flattened, made reachable or returned to the wrong current0 cursor");
        }
        report<<probe.mask<<'\t'<<evidence.triangles<<'\t'<<executed_lines<<"\tCPU0123-GPU0213\n";
    }
    native(isle_player_render_backend_camera_flags(&source,probe.camera,0x20u));
    probe.mask=3;Evidence alternate{lighting,renderer,probe.object};
    require(renderer.begin(primary,{0},{&alternate,Evidence::complete}),renderer.message());
    do {result=renderer.poll();if(result==g::Poll::pending) std::this_thread::yield();} while(result==g::Poll::pending);
    require(result==g::Poll::completed&&alternate.dispatch.combined&&alternate.dispatch.prefix.words>=6&&
        alternate.gpu_order==std::vector<unsigned>({0,1})&&!alternate.lane_words[1].empty()&&
        alternate.lane_words[1][0]==0xe1000000u,
        "source flag20 failed its genuine alternate load/init0 and early restore1, or replayed secondary initialization");
    report<<"flag20\tactual-alternate-reset-init0-early-restore1-source-combine\n";
    IsleGameObjectMetadata probe_visibility{};native(isle_game_object_metadata(services.game,probe.object,&probe_visibility));
    native(isle_game_object_set_flags(services.game,probe.object,probe_visibility.flags|1u));
    guard.deferred=services.object(0x7001001,0);require(guard.deferred,"deferred native camera creation failed");
    IsleGameCamera* deferred_attachment=nullptr;native(isle_game_attach_camera(services.game,guard.deferred,&deferred_attachment));
    player::Renderer::Camera deferred{guard.deferred,probe.camera,isle_player_render_camera_render,
        isle_player_mainCameraPreRender,5,1,0,0,UINT32_MAX};
    deferred.prepare=LineProbe::prepare;deferred.prepare_user=&probe;native(renderer.attach(deferred));
    for(unsigned bits=0;bits<8;++bits) {
        probe.mask=1u|(bits<<1);Evidence evidence{lighting,renderer,guard.deferred};
        require(renderer.begin(primary,{bits&1u},{&evidence,Evidence::complete}),renderer.message());
        do {result=renderer.poll();if(result==g::Poll::pending) std::this_thread::yield();} while(result==g::Poll::pending);
        std::vector<unsigned> gpu{0};
        for(unsigned lane:{2u,1u,3u}) if(probe.mask&(1u<<lane)) gpu.push_back(lane);
        const auto lines=unsigned(bool(probe.mask&4u))+unsigned(bool(probe.mask&8u)&&!(probe.mask&2u));
        require(result==g::Poll::completed&&!evidence.dispatch.combined&&evidence.gpu_order==gpu&&
            evidence.triangles==initial.triangles*lines,"flag4-off source lanes did not wait for actual final-GTL joining");
        const unsigned terminal_lane=(probe.mask&8u)?3u:(probe.mask&2u)?1u:(probe.mask&4u)?2u:0u;
        const auto& tail=evidence.lane_words[terminal_lane];
        require(tail.size()>=4&&tail[tail.size()-4]==0xe9000000u&&tail[tail.size()-3]==0&&
            tail[tail.size()-2]==0xdf000000u&&tail.back()==0,
            "final-GTL tail restored/branched like per-camera combine instead of real FullSync/EndDL");
        if(terminal_lane==2) for(std::size_t i=0;i+1<tail.size();i+=2)
            require(tail[i]!=0xdd0007ffu,"final line-only tail invented a normal-program restore");
        report<<"deferred\t"<<probe.mask<<'\t'<<evidence.triangles<<"\tfinal-FullSync-EndDL-no-camera-combine\n";
    }
    for(std::size_t i=0;i<ordinary.size();++i) native(isle_game_object_set_flags(services.game,ordinary[i],flags[i]));
    require(bool(report),"line microcode diagnostic evidence write failed");
    return std::move(made.frame);
}
struct GraphicsLifetime {
    player::Renderer& renderer;g::Frame& frame;player::Controller& controller;actor::PhotoScene& photo_scene;
    const std::unique_ptr<g::Frame>& secondary;
    ~GraphicsLifetime() {
        while(frame.busy()||(secondary&&secondary->busy())) {if(renderer.poll()==g::Poll::pending) std::this_thread::yield();}
        for(auto* current:{&frame,secondary.get()}) if(current&&controller.has_pending(*current)&&current->completed()) {
            const auto released=renderer.release_completed();
            if(released!=ISLE_GAME_OK) {
                std::cerr<<"source renderer completed ticket retirement failure: "<<int(released)<<' '<<renderer.message()<<'\n';
                std::terminate();
            }
        }
        photo_scene.retire();
        const auto detached=renderer.detach();
        if(detached!=ISLE_GAME_OK) {
            std::cerr<<"source renderer receiver retirement failure: "<<int(detached)<<' '<<renderer.message()<<'\n';
            std::terminate();
        }
    }
};
enum class Failure {none,after_prefix,after_seal,completed_memory,nested_wire,source_queue,removed_callback_memory};
void run(rom::Snapshot& snapshot,const std::filesystem::path& output,Failure failure,unsigned independent) {
    require(std::filesystem::create_directory(output),"source scenario directory already exists");
    require(std::filesystem::create_directory(output/"profile"),"source scenario profile creation failed");
    const world::Resources world_resources(snapshot, isle::course::beach_id);const auto& setup=world_resources.setup();
    const actor::Resources actor_resources(snapshot,0,{setup.fog_rgb,setup.fog_min,setup.fog_max});
    const items::Resources item_resources(snapshot,setup.fog_rgb,setup.fog_min,setup.fog_max);
    const player::CartResources cart_resources(snapshot,0,{setup.fog_rgb,setup.fog_min,setup.fog_max});
    const player::UiResources ui_resources(snapshot);const player::PhotoResources photo_resources(snapshot);
    const environment_sound::Resources sounds(snapshot,0);const menu::EffectResources effect_resources(snapshot);
    const photo::LightingInputs lighting(snapshot);const graphics::SourcePrograms programs(snapshot);
    const std::array banks{&effect_resources.bank(menu::EffectGroup::common),&effect_resources.bank(menu::EffectGroup::beach)};
    const menu::EffectGraphicsResources effect_graphics(snapshot,banks,{0x700000,0x80000});
    sprite::Producer sprites(ui_resources.roots(),ui_resources.banks());
    auto hydrated=game_audio::hydrate(snapshot);require(bool(hydrated),"source renderer audio admission failed");
    std::array<audio::effects::Section,8> sections;audio::effects::Config audio_effects;
    require(game_audio::native_startup_effects(hydrated.resources,sections,audio_effects)==audio::Error::none,"source audio effects admission failed");
    private_audio::Capture pcm;pcm.samples.resize(1024*audio::AudioService::max_frames*2);
    private_audio::Execution execution{snapshot,nullptr,{}};
    auto audio=game_audio::Session::create(std::move(hydrated.resources),audio_effects,pcm.output(),execution.executor());
    require(bool(audio),"source renderer audio Session failed");Audio clock{*audio.session,execution};
    game::State state(output/"profile");require(state.initialize(1)&&state.save(),"source renderer private save initialization failed");
    // Isolated host entry performs the manager's post-Boot VI service once;
    // scene rendering must not implicitly recover this output from profile RAM.
    {
        const auto initial_settings=state.save()->snapshot();require(bool(initial_settings),"source renderer entry settings unavailable");
        const auto initial_vi=initial_settings.value.screen_offsets();require(bool(initial_vi),"source renderer entry VI unavailable");
        state.set_screen_offsets({initial_vi.value.horizontal,initial_vi.value.horizontal,initial_vi.value.vertical,initial_vi.value.vertical});
    }
    IslePlayerPhotoSource application{};native(photo_resources.bind(application,*state.save()));isle_player_photo_func_8009CE00(&application);
    game_input::Service input;scene::Storage storage(32*1024*1024);native(storage.create());
    {
        scene::Services services(storage.game(),storage.arena(),{{8,16,32,3,4},ui_resources.resources()},input,*audio.session,state);
        private_player::Source component_player(services);auto& source=component_player.value;
        component_player.create(nullptr);source.photo=&application;source.cam_target={0,1133,5000};source.gDirectionIndex=-1;
        // This reused fixture is explicitly COMPONENT-ONLY: ID0x1001 and native
        // contextual RPY/RTS are not original initUI's canonical Player7/RT prefix.
        player::commands::Binding commands(services,source);
        player::Ui ui(ui_resources,services,source,sprites,0);source.ui=&ui.source();
        focus::Detector detector;focus::RegionCopies copies(capture_layout);actor::DetectorBinding detection{copies,detector};detection.enabled=true;
        auto made=g::Frame::create(snapshot,{true,{16000000,4*1024*1024},frame_layout});require(bool(made),made.message.data());auto& frame=*made.frame;
        std::vector<menu::RamRange> mutable_ranges{world_layout.payloads,world_layout.lists,world_layout.materials,
            actor_layout.payloads,actor_layout.lists,actor_layout.materials,item_layout.payloads,item_layout.lists,item_layout.materials,
            cart_layout.payloads,cart_layout.lists,cart_layout.materials,fx_commands,joined_lists,guard_range,line_arena,
            {sprite_commands.address,sprite_commands.size}};
        for(const auto range:copies.reservations()) mutable_ranges.push_back(range);
        for(const auto& span:lighting.installation_spans()) mutable_ranges.push_back({physical(span.guest_address),span.bytes.size()});
        std::vector<sprite::AssetSpan> immutable(sprites.assets().begin(),sprites.assets().end());
        const auto append=[&](auto spans) {for(const auto& span:spans) immutable.push_back({physical(span.guest_address),
            {reinterpret_cast<const std::uint8_t*>(span.bytes.data()),span.bytes.size()}});};
        append(world_resources.installation_spans());append(actor_resources.installation_spans());append(item_resources.installation_spans());append(cart_resources.installation_spans());
        for(const auto span:effect_graphics.installation_spans()) immutable.push_back(span);
        std::vector<sprite::RamRange> reservations;
        for(const auto range:mutable_ranges) reservations.push_back({static_cast<std::uint32_t>(range.offset),static_cast<std::uint32_t>(range.size)});
        Content content{nullptr,copies};
        player::Renderer::Config config;
        config.resources={independent+1,&sprites,immutable,reservations,{0x3b5000,0x3da800},physical(lighting.pre_render_guest())};
        config.prepare=Content::prepare;config.prepare_user=&content;config.programs=&programs;config.effect_resources=&effect_graphics;
        if(failure==Failure::nested_wire) config.lanes.size=32*0x400;
        if(failure==Failure::after_seal) config.sprite_capacity=1;
        player::Renderer renderer(services,source,lighting,config);
        IsleGameObject* actual_ui=nullptr;native(renderer.create_main(0x385878ff,&actual_ui));
        require(actual_ui&&renderer.ui_camera_object()==actual_ui&&source.camera,"original C constructor lost its actual UI result/main camera");
        IsleGameObject* getter_object=nullptr;IsleGameCamera* getter_camera=nullptr;
        native(isle_player_render_invoke_getters(&source,&getter_object,&getter_camera));
        require(getter_camera==source.camera&&getter_object,"complete original C main-camera getters lost canonical identity");
        auto* retained_ui=actual_ui;
        native(renderer.remove_main());native(renderer.collect_joined());
        require(!source.camera,"complete original C remove did not perform actual native retirement");
        native(renderer.create_main(0x385878ff,&actual_ui));
        require(actual_ui&&actual_ui!=retained_ui,"recreated original cameras discarded the genuine earlier UI record");
        IsleGameObject* main_object=nullptr;native(isle_game_camera_object(services.game,source.camera,&main_object));
        player::Renderer::Camera metadata{};native(renderer.metadata(main_object,metadata));
        require(metadata.flags==5&&metadata.priority==5&&metadata.link_mask==0x38&&metadata.tag==UINT32_MAX,
            "original main camera source flags/priority/DL mask/tag changed");
        native(renderer.metadata(actual_ui,metadata));require(metadata.flags==0&&metadata.priority==3&&metadata.link_mask==2,
            "original UI source metadata changed");
        std::size_t matrices=0;native(isle_game_camera_matrix_count(services.game,source.camera,&matrices));require(matrices==2,"original C did not attach complete3/14 camera matrices");
        const IsleGameCameraMatrix* projection_kind=nullptr;const IsleGameCameraMatrix* view_kind=nullptr;
        native(isle_game_camera_peek_matrix(services.game,source.camera,0,&projection_kind));
        native(isle_game_camera_peek_matrix(services.game,source.camera,1,&view_kind));
        require(projection_kind->kind==ISLE_GAME_CAMERA_PERSPECTIVE_FAST&&view_kind->kind==ISLE_GAME_CAMERA_LOOK_AT_REFLECT_ROLL,
            "original source main camera matrix kinds were replaced by constructor-default3/6");
        native(isle_game_camera_set_look_at(services.game,source.camera,&view));
        auto* root=services.object(0,0);require(root,"real PhotoScene root creation failed");
        environment_sound::Environment environment(services,storage.arena(),sounds);require(environment.initialize(nullptr,source.camera),"source environment initialization failed");
        world::ModuleState world_module;
        Hooks hooks{services,source.camera};world::World world(services,world_resources,world_module,{&hooks,nullptr,Hooks::remove,nullptr,Hooks::camera_shift,Hooks::effects_shift,nullptr});world.follow_camera(source.camera);
        a::SlotCapacity actor_capacity{},item_capacity{};
        const auto capacity=[](auto models,a::SlotCapacity& maximum) {for(const auto& model:models) {const auto value=a::slot_capacity(model);
            maximum.nodes=std::max(maximum.nodes,value.nodes);maximum.material_attachments=std::max(maximum.material_attachments,value.material_attachments);}};
        capacity(actor_resources.models(),actor_capacity);capacity(item_resources.models(),item_capacity);
        const std::array actor_capacities{actor_capacity,actor_capacity};std::array<a::SlotCapacity,8> item_capacities;item_capacities.fill(item_capacity);
        const auto cart_capacity=a::slot_capacity(cart_resources.animation_resources().models[0]);const std::array cart_capacities{cart_capacity,cart_capacity};
        {
            beach::ActorSceneResources sceneData(snapshot);
            std::uint16_t scene_latch=sceneData.initial_scalars().magikarp_active;
            beach::ActorSceneState sceneState(sceneData,scene_latch);
            actor::Owner actors(services,actor_resources,actor::CourseContext(sceneState),world,environment,detector,source,actor_capacities,banks,{32768,4096});hooks.actors=&actors;
            accepted(actors.publish_effects());native(renderer.adopt_effects(actors));
            require(source.D_80382D10_523120==actors.particle_object()&&source.D_80382D14_523124==actors.effect_object(),"source globals did not adopt actual ActorOwner -6/-7");
            require(environment.set_listener(source.model,source.camera),"actual Player/model listener binding failed");
            items::Globals globals;items::Owner items(services,item_resources,world,actors,globals,source,item_capacities,{32768,4096});hooks.items=&items;
            player::cart::Owner cart(services,cart_resources,source,cart_capacities,{});Owners owners{actors,items,cart,world};
            require(world.enter_first(0)==&world.blocks()[0],"source World first block failed");source.movement.block=world.current();
            accepted(items.init());accepted(items.set_custom_functions(nullptr,0,nullptr));
            const auto ball=items.spawn_ball({-180,1233,4000},{}),apple=items.spawn_apple({180,1233,4000},{});accepted(ball.status);accepted(apple.status);
            const auto cart_object=cart.init();require(bool(cart_object),cart_object.status.detail);accepted(cart.set_animation(cart.intro_model(),cart.intro_material(),0,0));
            auto* trigger=services.object(0x1000,4);require(trigger,"actual Magikarp trigger creation failed");
            IsleGameModelNodeInit node{};node.parent=UINT32_MAX;node.matrix_kind=ISLE_GAME_MODEL_CONTEXTUAL_RPY;node.transform.scale={1,1,1};node.transform.position={0,1233,4000};
            IsleGameModel* trigger_model=nullptr;native(isle_game_attach_model(services.game,trigger,&node,1,&trigger_model));
            beach::MagikarpScene spawner(services,actors,world,actor_resources,sceneState);const auto actor=spawner.spawn_at({trigger,trigger_model});require(bool(actor),actor.detail);
            player::Photo photo(application,source,actors,items,detection,photo_resources,world,0);
            actor::PhotoScene photo_scene(services,actors,root,source.camera,detection,&application);
            player::Controller controller(photo,detection);native(renderer.bind_photos(controller));
            std::unique_ptr<g::Frame> secondary;
            GraphicsLifetime graphics_lifetime{renderer,frame,controller,photo_scene,secondary};
            auto all=mutable_ranges;all.push_back({config.lanes.address,config.lanes.size});
            for(const auto range:frame.reservations()) if(range.address) all.push_back({range.address,range.size});
            for(const auto range:actor_layout.framebuffers) all.push_back(range);
            for(const auto range:effect_graphics.reservations()) all.push_back(range);
            const auto others=[&](std::array<menu::RamRange,3> own,bool framebuffer) {
                std::vector<menu::RamRange> result;for(const auto range:all) {bool skip=false;
                    for(const auto mine:own) skip|=range.offset==mine.offset&&range.size==mine.size;
                    if(framebuffer) for(const auto fb:actor_layout.framebuffers) skip|=range.offset==fb.offset&&range.size==fb.size;
                    if(!skip) result.push_back(range);}return result;
            };
            const auto wo=others({world_layout.payloads,world_layout.lists,world_layout.materials},false);
            const auto ao=others({actor_layout.payloads,actor_layout.lists,actor_layout.materials},true);
            const auto io=others({item_layout.payloads,item_layout.lists,item_layout.materials},true);
            const auto co=others({cart_layout.payloads,cart_layout.lists,cart_layout.materials},true);
            world::Producer worlds(services.game,source.camera,services.camera_basis(),world_resources,world.render_view(),world_layout,wo);
            actor::Producer actor_producer(services.game,source.camera,services.camera_basis(),actor_resources,actor_capacities,actors.render_bindings(),actor_layout,ao,&detection);
            menu::model::PacketProducer item_producer(services.game,source.camera,services.camera_basis(),
                {item_resources.models(),item_resources.materials(),item_resources.installation_spans()},
                item_capacities,items.render_bindings(),item_layout,io);
            player::cart::Producer carts(services.game,source.camera,services.camera_basis(),cart_resources,cart_capacities,cart.render_bindings(content.cart_order),cart_layout,co);
            course::ItemComposition composition{services,item_producer,128,&carts,photo_scene.prefix_object(),true};
            course::ModelProducer models(worlds,actor_producer,joined_lists,&composition);
            content.models=&models;content.actors=&actors;content.items=&items;content.cart=&cart;
            native(renderer.bind_cart(cart,content.cart_order));
            require(renderer.reserve(frame),frame.message());
            source.ProgressFlags=0x1027;native(isle_player_ui_Icons_Init(&ui.source()));native(isle_player_ui_func_8035E37C_4FE78C(&ui.source()));
            const auto tick=[&] {require(frame.quiescent(),"source native OM mutation crossed actual GPU/readback ownership");native(isle_game_tick(services.game));require(!services.failed(),services.error());clock.tick();};
            for(unsigned n=0;n<6;++n) tick();
            accepted(items.pause());
            const auto particle=actors.effects().create_particle(3,0,{500,1233,4000},{});require(bool(particle.status)&&particle.spawned,"actual shared common3 FX did not spawn");tick();tick();
            native(isle_actor_source_pause_object(actors.source(actor.actor.instance),1));native(isle_game_pause_updates(services.game,actor.actor.object,true));
            native(isle_player_render_invoke_viewport(&source,8,0,312,232));
            if(failure==Failure::source_queue) {
                const std::array ordinary{main_object,actual_ui,retained_ui,actors.particle_object()};
                source_queue_cases(services,frame,renderer,controller,photo,source,application,
                    photo_scene,detection,detector,ordinary,output,tick);
            } else {
            content.fail_after_prefix=failure==Failure::after_prefix;
            const auto before=detector.generation(static_cast<std::uint16_t>(1-detection.context));
            Evidence evidence{lighting,renderer,main_object};
            evidence.fail_completed=failure==Failure::completed_memory||failure==Failure::removed_callback_memory;
            if(failure==Failure::removed_callback_memory) native(renderer.set_postprocess(false));
            auto changed=view;changed.eye[0]+=300;
            native(isle_game_camera_set_look_at(services.game,source.camera,&changed));
            require(renderer.light_direction()==evidence.expected,"native camera change unexpectedly recomputed source-owned OM light bytes");
            const bool begun=renderer.begin(frame,{detection.context},{&evidence,Evidence::complete});
            require(renderer.drawn_frame_counter()==(begun?1:0),
                "failed/skipped whole source draw fabricated a counter advance or actual dispatch did not advance");
            if(failure==Failure::after_prefix||failure==Failure::after_seal||failure==Failure::nested_wire) {
                require(!begun&&!frame.submission()&&frame.quiescent()&&!controller.has_pending(frame)&&photo.quiescent(),
                    "source preparation failure retained sink/ticket or fabricated a GPU submission/completion");
                require(content.calls==(failure==Failure::nested_wire?0u:1u),"source failure crossed the wrong real prefix/history phase");
                require(failure!=Failure::nested_wire||source.status==ISLE_GAME_EXHAUSTED,"nested C GBI failure did not preserve exact exhaustion status");
            } else {
                require(begun,renderer.message());require(frame.submission()&&controller.ticket(frame).submission==frame.submission(),"source request did not bind actual Frame/token");
                require(controller.ticket(frame).context!=frame.submission_context()&&controller.ticket(frame).generation==before+1,
                    "source Detector context was conflated with real framebuffer context/generation");
                require(renderer.detach()==ISLE_GAME_INVALID_STATE&&renderer.collect_joined()==ISLE_GAME_INVALID_STATE,
                    "actual GPU/Photo ownership allowed camera slot reuse or deletion");
                const auto actual_token=frame.submission();
                g::Poll result;do {result=renderer.poll();if(result==g::Poll::pending) std::this_thread::yield();} while(result==g::Poll::pending);
                if(failure==Failure::completed_memory||failure==Failure::removed_callback_memory) {
                    require(result==g::Poll::failed&&frame.failed_submission(actual_token)&&!controller.has_pending(frame)&&photo.quiescent(),
                        "actual completed-memory failure did not retire its exact failed ticket");
                    require(controller.queued_requests()==(failure==Failure::removed_callback_memory?1u:0u),
                        "failed completion erased an unconsumed Source ID or restored an already-received message");
                    const auto first_error=renderer.status();native(renderer.set_postprocess(false));
                    require(renderer.status()==first_error,"SC removal cleanup erased the first Renderer failure");
                    if(failure==Failure::removed_callback_memory) {
                        photo_scene.retire();
                        require(controller.queued_requests()==0,"genuine Source teardown retained its old message FIFO");
                    }
                } else {
                    require(result==g::Poll::completed&&evidence.calls==1&&evidence.triangles&&evidence.rectangles,"normal original C camera did not execute actual geometry and UI");
                    const auto ticket=controller.ticket(frame);require(!detector.published().present,"source Photo published before native OM CopyInfo");
                    tick();require(detector.published().context==ticket.context,"later actual OM CopyInfo did not publish completed source Photo");
                    save(output/"normal.rgba8",frame.completed().rgba);save(output/"normal.rgba16be",evidence.color);save(output/"normal.depth16be",evidence.depth);
                    require(evidence.contains(0xff10013fu,frame_layout.depth)&&evidence.contains(0xff10013fu,0x3b5000u+(frame.submission_context()?0x25800u:0u)),
                        "source border packets did not bind actual depth then physical color targets");
                    require(evidence.contains(0xe3000a01u,0)&&evidence.contains(0xe200001cu,0),"normal borders were not source ONE_CYCLE/NOOP packets");
                    const auto xy=[](std::int32_t x,std::int32_t y) {
                        return ((static_cast<std::uint32_t>(x)*4u&4095u)<<12)|(static_cast<std::uint32_t>(y)*4u&4095u);
                    };
                    require(source.MainCameraBorderYmin==0&&
                        evidence.contains(0xf6000000u|xy(319,0),xy(0,0))&&
                        evidence.contains(0xf6000000u|xy(319,239),xy(0,232))&&
                        evidence.contains(0xf6000000u|xy(8,232),xy(0,-1))&&
                        evidence.contains(0xf6000000u|xy(319,232),xy(312,-1))&&
                        evidence.contains(0xed000000u|xy(8,0),xy(312,232)),
                        "source inclusive top/bottom/side fields or signed Ymin-1/scissor were clamped, reordered or made exclusive");
                    require(renderer.remove_main()==ISLE_GAME_INVALID_STATE,"completed GPU bypassed outstanding real Photo camera/resource ownership");
                    native(renderer.release_completed());
                    hidden_prefix_regression(services,frame,content,lighting,renderer,source.camera,
                        photo_scene,detection,detector,cart,config,output);
                    native(isle_player_render_invoke_viewport(&source,0,0,320,240));
                    LateUi late(services,ui,frame,config);
                    auto* late_object=services.object(0x7003000,0);require(late_object,"late native UI scheduling camera allocation failed");
                    IsleGameCamera* late_attachment=nullptr;native(isle_game_attach_camera(services.game,late_object,&late_attachment));
                    player::Renderer::Camera late_camera{late_object,source.camera,isle_player_render_camera_render,
                        isle_player_mainCameraPreRender,4,0,0,0,UINT32_MAX};
                    late_camera.prepare=LateUi::prepare;late_camera.prepare_user=&late;native(renderer.attach(late_camera));
                    Evidence no_border{lighting,renderer,main_object};
                    require(renderer.begin(frame,{0},{&no_border,Evidence::complete}),renderer.message());
                    do {result=renderer.poll();if(result==g::Poll::pending) std::this_thread::yield();} while(result==g::Poll::pending);
                    require(result==g::Poll::completed&&no_border.dispatch.prefix.words==2&&
                        !no_border.prefix_contains(0xff10013fu,frame_layout.depth),
                        "Xmin==0 still emitted source border target/fill packets instead of only the actual scissor prefix");
                    player::Renderer::Dispatch ui_dispatch{};native(renderer.dispatch_metadata(actual_ui,ui_dispatch));
                    player::Renderer::Dispatch late_dispatch{};native(renderer.dispatch_metadata(late_object,late_dispatch));
                    require(late.calls==1&&late.before>late.after&&ui_dispatch.sprite_views==late.after&&!late_dispatch.combined,
                        "real UI camera gathered before the preceding native camera changed actual film visibility");
                    native(renderer.release_completed());
                    services.erase(late_object);native(renderer.collect_joined());
                    native(isle_game_object_set_flags(services.game,late.film,late.old_flags));
                    native(isle_player_render_invoke_viewport(&source,8,0,312,232));
                    require(services.send_command(main_object,ISLE_PLAYER_CAMERA_BLINK),"actual source camera blink FIFO enqueue failed");tick();
                    require(source.MainCameraBlinkTimeout>0,"original C camera command did not consume blink FIFO");
                    const auto timeout=source.MainCameraBlinkTimeout;const auto prefix_calls=content.calls;
                    Evidence blink{lighting,renderer,main_object};require(renderer.begin(frame,{1},{&blink,Evidence::complete}),renderer.message());
                    do {result=renderer.poll();if(result==g::Poll::pending) std::this_thread::yield();} while(result==g::Poll::pending);
                    require(result==g::Poll::completed&&source.MainCameraBlinkTimeout==timeout-1&&content.calls==prefix_calls&&!controller.has_pending(frame),
                        "original blink prepared geometry/prefix/request or lost its timeout decrement");
                    IsleGameObjectMetadata fx{};native(isle_game_object_metadata(services.game,actors.particle_object(),&fx));
                    require(fx.flags&1u,"blink did not hide actual -6 before its dispatch");
                    require(blink.triangles==0&&blink.rectangles>0&&blink.contains(0xe3000a01u,0x300000u)&&
                        blink.contains(0xf7000000u,0x00010001u)&&blink.contains(0xe3000a01u,0)&&blink.contains(0xe200001cu,0x00552078u),
                        "original blink was not FILLblack/NOOP then ONE_CYCLE/AA-ZB restore with independent eligible UI");
                    save(output/"blink.rgba8",frame.completed().rgba);
                    while(source.MainCameraBlinkTimeout>0) {tick();Evidence remaining{lighting,renderer,main_object};require(renderer.begin(frame,{0},{&remaining,Evidence::complete}),renderer.message());
                        do {result=renderer.poll();if(result==g::Poll::pending) std::this_thread::yield();} while(result==g::Poll::pending);require(result==g::Poll::completed,renderer.message());}
                    tick();Evidence restored{lighting,renderer,main_object};require(renderer.begin(frame,{0},{&restored,Evidence::complete}),renderer.message());
                    do {result=renderer.poll();if(result==g::Poll::pending) std::this_thread::yield();} while(result==g::Poll::pending);
                    require(result==g::Poll::completed&&restored.triangles&&content.calls==prefix_calls+1,"blink did not return to complete original normal renderer");
                    native(isle_game_object_metadata(services.game,actors.particle_object(),&fx));require(!(fx.flags&1u),"normal source did not restore actual FX visibility");
                    native(renderer.release_completed());
                    const std::array ordinary{main_object,actual_ui,retained_ui,actors.particle_object()};
                    secondary=line_diagnostics(snapshot,services,renderer,source,frame,cart_resources,lighting,config,ordinary,output);
                    tick();Evidence independent_ram{lighting,renderer,main_object};
                    require(renderer.begin(*secondary,{detection.context},{&independent_ram,Evidence::complete}),renderer.message());
                    const auto held_identity=reinterpret_cast<std::uintptr_t>(main_object);
                    services.erase(main_object); // genuine native joins while actual GPU/Photo consumers still hold.
                    require(!source.camera&&services.source_owner_joined(main_object)&&
                        renderer.collect_joined()==ISLE_GAME_INVALID_STATE,
                        "native camera join was mistaken for GPU/Photo completion");
                    auto* while_held=services.object(0x7002000,0);require(while_held&&reinterpret_cast<std::uintptr_t>(while_held)!=held_identity,
                        "actual native allocator reused a camera scheduling identity while GPU/Photo holds remained");
                    IsleGameCamera* new_payload=nullptr;native(isle_game_attach_camera(services.game,while_held,&new_payload));
                    do {result=renderer.poll();if(result==g::Poll::pending) std::this_thread::yield();} while(result==g::Poll::pending);
                    require(result==g::Poll::completed&&independent_ram.triangles&&
                        independent_ram.mapping_identity!=evidence.mapping_identity&&independent_ram.dispatch.epoch==1&&
                        renderer.collect_joined()==ISLE_GAME_INVALID_STATE,
                        "independent actual RAM/epoch failed or completed GPU released a still-Photo-held camera record");
                    save(output/"independent-frame.rgba8",secondary->completed().rgba);
                    native(renderer.release_completed());native(renderer.collect_joined());
                    auto* after_release=services.object(0x7002001,0);
                    require(after_release&&reinterpret_cast<std::uintptr_t>(after_release)==held_identity,
                        "actual native camera slot did not become reusable after joins plus exact GPU/Photo release");
                    services.erase(while_held);services.erase(after_release);
                }
            }
            }
        }
        native(ui.retire());native(renderer.detach());native(commands.detach());
    }
    native(storage.destroy());
    require(audio.session->stop()==audio::Error::none,"source audio stop failed");
    for(unsigned n=0;n<240&&audio.session->state()!=game_audio::Session::State::draining;++n) clock.tick();
    require(audio.session->state()==game_audio::Session::State::draining&&audio.session->service().voices_retired(),
        "source audio voices did not retire");
    require(audio.session->poll_drain()==audio::Error::none&&audio.session->poll_drain()==audio::Error::none&&
        audio.session->state()==game_audio::Session::State::drained&&pcm.finishes==1,
        "source audio actual RSP/owned PCM drain failed");
    std::ofstream report(output/"scope.txt");report<<"component=complete-original-C-camera-renderer\nroot=COMPONENT-ONLY-Player-ID0x1001-contextual-RPY-RTS-not-original-initUI\n"
        <<"source=immutable-ownGAME-df8815e75f3d79618466e01e0827c6165b93d11c\n"
        <<"scene=actual-World-Actor-Item-Cart-prefix-ActorOwner-minus6-minus7-UI\n"
        <<"failure="<<int(failure)<<" independent_RAM_scene="<<independent<<"\n"
        <<"not_claimed=Root106-fullgame-physicaldevices-Windows11-hardwareparity\n";
    require(bool(report),"source renderer scope evidence write failed");
}
}
int main(int argc,char** argv) {
    try {
        require(argc==3,"usage: player-source-renderer-rsp-smoke PRIVATE_ROM NEW_OUTPUT_DIRECTORY");
        const std::filesystem::path output=argv[2];require(!output.empty()&&!std::filesystem::exists(output),"source renderer output root must be new");
        require(std::filesystem::create_directories(output),"source renderer private output root creation failed");
        auto snapshot=rom::read_snapshot(argv[1]);
        run(snapshot,output/"normal-blink-normal",Failure::none,0);
        run(snapshot,output/"independent-ram",Failure::none,1);
        run(snapshot,output/"failed-prefix",Failure::after_prefix,2);
        run(snapshot,output/"failed-seal",Failure::after_seal,3);
        run(snapshot,output/"failed-completed-memory",Failure::completed_memory,4);
        run(snapshot,output/"failed-nested-wire",Failure::nested_wire,5);
        run(snapshot,output/"source-context-queue",Failure::source_queue,6);
        run(snapshot,output/"failed-removed-callback",Failure::removed_callback_memory,7);
        std::cout<<"original-C-source-renderer-component passed; originalRoot/fullgame/hardware/Windows11 not claimed\n";return 0;
    } catch(const std::exception& error) {std::cerr<<"player-source-renderer-rsp-smoke: "<<error.what()<<'\n';return 1;}
}
