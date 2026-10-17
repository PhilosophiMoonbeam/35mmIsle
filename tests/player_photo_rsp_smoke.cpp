// Private admitted-ROM component consumer, never a game entrypoint or CTest.
// Main alone runs it under the qualified CPU Vulkan/RSP/RDP/audio trace policy.
// API-authored native Player/camera/trigger inputs; actual source Actor/Item/FX,
// source photo bodies, native OM CopyInfo and concrete Frame failure evidence.
#include "actor_photo_scene.hpp"
#include "actual_audio_driver.hpp"
#include "course_model_producer.hpp"
#include "effect_bank_resources.hpp"
#include "effect_graphics_resources.hpp"
#include "photo_lighting.hpp"
#include "player_photo.hpp"
#include "player_photo_rsp_fixture.h"
#include "scene_storage.hpp"
#include "source_effect_camera.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {
using namespace isle;
namespace a=menu::animation;
namespace g=scene::graphics;
constexpr std::size_t width=320,height=240,plane=width*height*2;
constexpr world::ProducerLayout world_layout{{0x200000,0x10000},{0x210000,0x10000},{0x220000,0x10000}};
constexpr actor::ProducerLayout actor_layout{{0x400000,0x80000},{0x480000,0x80000},{0x500000,0x100000},
    {{{0x3b5000,plane},{0x3da800,plane}}}};
constexpr items::ProducerLayout item_layout{{0x230000,0x10000},{0x240000,0x10000},{0x250000,0x10000},
    {{{0x3b5000,plane},{0x3da800,plane}}}};
constexpr focus::CaptureLayout capture_layout{{0x680000,0x50},{0x680080,0x80},{0x680100,0xa00}};
constexpr menu::RamRange fx_commands{0x680c00,0x5400},joined_lists{0x686000,0x1000},guard{0x7f0000,256};
constexpr g::Layout frame_layout{{0x626000,0x2000},{0x630000,0x10000},{0x640000,0x40000},0x600000};
constexpr IsleGameLookAt source_view{{0,1600,6000},{0,500,0},{0,1,0}};
constexpr IsleGameVec3 trigger_position{0,1233,4000};
using Encoded=std::array<std::byte,0x3a0>;
void require(bool value,const char* text) {
    if(!value) {
        std::cerr << "player-photo-rsp-smoke failure before teardown: " << text << '\n';
        throw std::runtime_error(text);
    }
}
void native(IsleGameResult result) { require(result==ISLE_GAME_OK,"photo consumer native/source operation failed"); }
void accepted(actor::OwnerStatus result) { require(bool(result),result.detail); }
void accepted(items::OwnerStatus result) { require(bool(result),result.detail); }
std::uint32_t physical(std::uint32_t value) { return value&0x1fffffffu; }
bool same(IsleGameVec3 x,IsleGameVec3 y) { return x.x==y.x&&x.y==y.y&&x.z==y.z; }
bool same(const focus::Focus& x,const focus::Focus& y) {
    return x.object==y.object&&x.pokemon_id==y.pokemon_id&&x.flags==y.flags&&x.present==y.present&&x.context==y.context;
}
Encoded encode(const IslePlayerPhotoData& value) { Encoded result;player::encode_photo(value,result);return result; }
std::uint32_t be32(std::span<const std::byte> bytes,std::size_t at) {
    std::uint32_t value=0;for(unsigned i=0;i<4;++i) value=value*256+std::to_integer<unsigned>(bytes[at+i]);return value;
}
void save(const std::filesystem::path& path,std::span<const std::byte> bytes) {
    std::ofstream file(path,std::ios::binary);require(bool(file),"photo artifact open failed");
    file.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));
    require(bool(file),"photo artifact write failed");
}
void save_image(const std::filesystem::path& output,g::Completed image) {
    require(bool(image)&&image.rgba.size()==width*height*4,"actual completed RGBA8 extent changed");
    save(output/"photo-full.rgba8",image.rgba);
    std::ofstream file(output/"photo-full.ppm",std::ios::binary);require(bool(file),"photo PPM open failed");
    file<<"P6\n320 240\n255\n";
    for(std::size_t i=0;i<image.rgba.size();i+=4) file.write(reinterpret_cast<const char*>(image.rgba.data()+i),3);
    require(bool(file),"photo PPM write failed");
}
// Fixed wire vectors cover signed seven/thirteen/sixteen-bit extrema, packed
// animation/special/pose fields and byte order. They are codec-only fixtures,
// never supplied to Photo, Detector, a native owner or the GPU.
void packing_edges() {
    IslePlayerPhotoData semantic{};semantic.levelID=-64;semantic.unk_00_24=1;semantic.unk_00_16=255;
    semantic.unk_04.f32=1.0f;semantic.pokemons[0].pokemonID=-4096;
    semantic.pokemons[0].unk_00_13=63;semantic.pokemons[0].specialID=31;semantic.pokemons[0].poseID=255;
    semantic.pokemons[1].pokemonID=4095;semantic.pokemons[2].pokemonID=-1;
    semantic.items[0].itemType=-1;semantic.items[0].animationTime=255;
    auto& effect=semantic.effects[0];effect.textureID=-1;effect.dataID=255;effect.bankID=7;effect.unk_03=255;
    effect.posX=-32768;effect.posY=32767;effect.posZ=-1;effect.size=65535;
    effect.primColor[0]=0x12;effect.primColor[1]=0x34;effect.primColor[2]=0x56;effect.primColor[3]=0x78;
    Encoded literal{};
    const auto put=[&](std::size_t at,std::initializer_list<unsigned> values) {
        for(const auto value:values) literal[at++]=std::byte(value);
    };
    put(0,{0x81,0xff});put(4,{0x3f,0x80,0,0});put(32,{0x80,0x07,0xff,0xff});
    put(56,{0x7f,0xf8,0,0});put(80,{0xff,0xf8,0,0});put(0x140,{0xff,0xff});
    put(0x1a0,{0xff,0xff,7,0xff,0x80,0,0x7f,0xff,0xff,0xff,0xff,0xff,0x12,0x34,0x56,0x78});
    require(encode(semantic)==literal,"photo codec differs from independent literal signed/packed wire vector");
    IslePlayerPhotoData decoded{};native(player::decode_photo(literal,decoded));
    require(decoded.levelID==-64&&decoded.unk_00_24==1&&decoded.unk_00_16==255&&decoded.unk_04.f32==1&&
        decoded.pokemons[0].pokemonID==-4096&&decoded.pokemons[0].unk_00_13==63&&
        decoded.pokemons[0].specialID==31&&decoded.pokemons[0].poseID==255&&
        decoded.pokemons[1].pokemonID==4095&&decoded.pokemons[2].pokemonID==-1&&
        decoded.items[0].itemType==-1&&decoded.items[0].animationTime==255&&decoded.effects[0].posX==-32768&&
        decoded.effects[0].posY==32767&&decoded.effects[0].posZ==-1&&decoded.effects[0].size==65535,
        "literal canonical photo decode lost signed extrema or packed fields");
}
struct SourceLifetime {
    IslePlayerSource& source;bool active=true;
    void finish() { if(active) { native(isle_player_source_deactivate(&source));active=false; } }
    ~SourceLifetime() { if(active&&isle_player_source_deactivate(&source)!=ISLE_GAME_OK) std::terminate(); }
};
struct WorldHooks {
    scene::Services& services;IsleGameCamera* camera;actor::Owner* actors=nullptr;items::Owner* items=nullptr;
    static void remove(void* user,world::Block& block) noexcept {
        auto& self=*static_cast<WorldHooks*>(user);
        if(self.actors&&!self.actors->remove_block(block)) self.services.fail("photo World actor removal failed");
    }
    static void camera_shift(void* user,world::Vec3 delta) noexcept {
        auto& self=*static_cast<WorldHooks*>(user);const IsleGameCameraParameters* current=nullptr;
        if(!self.services.check(isle_game_camera_parameters(self.services.game,self.camera,&current),"photo camera shift borrow")) return;
        auto view=current->look_at;const std::array shift{delta.x,delta.y,delta.z};
        for(unsigned i=0;i<3;++i) { view.eye[i]+=shift[i];view.at[i]+=shift[i]; }
        self.services.check(isle_game_camera_set_look_at(self.services.game,self.camera,&view),"photo camera shift");
    }
    static void effects_shift(void* user,world::Vec3 delta) noexcept {
        auto& self=*static_cast<WorldHooks*>(user);
        if(self.actors) self.actors->effects().translate({delta.x,delta.y,delta.z});
        if(self.items&&!self.items->displace({delta.x,delta.y,delta.z})) self.services.fail("photo Item shift failed");
    }
};
struct OwnerLifetime {
    actor::Owner& actors;items::Owner& items;world::World& world;WorldHooks& hooks;
    ~OwnerLifetime() {
        // Release both sides of real Actor/Item field/queue borrows before
        // collecting either semantic owner; attempt every teardown on failure.
        const auto actors_retired=actors.reset();
        const auto items_retired=items.retire();
        const auto actors_collected=actors.collect_joined();
        const auto items_collected=items.collect_joined();
        if(!actors_retired||!items_retired||!actors_collected||!items_collected) std::terminate();
        world.retire();hooks.actors=nullptr;hooks.items=nullptr;
    }
};
struct ModelPass {
    course::ModelProducer& models;focus::RegionCopies& copies;const photo::LightingInputs& lighting;
    IsleGame* game;IsleGameCamera* camera;
    static bool prepare(void* user,std::span<std::uint32_t> words,std::uint64_t generation,
                        const scene::render::Pass& pass,scene::render::Prepared& output) {
        auto& self=*static_cast<ModelPass*>(user);
        if(self.copies.install(words)!=focus::CaptureStatus::ok) return false;
        self.lighting.install(std::as_writable_bytes(words));
        std::fill_n(words.begin()+guard.offset/4,guard.size/4,0x13579bdfu);
        const IsleGameCameraParameters* camera=nullptr;
        if(isle_game_camera_parameters(self.game,self.camera,&camera)!=ISLE_GAME_OK) return false;
        const auto& eye=camera->look_at.eye;const auto& at=camera->look_at.at;
        self.lighting.update_direction(std::as_writable_bytes(words),{eye[0],eye[1],eye[2]},{at[0],at[1],at[2]});
        output=self.models.prepare(words,generation,pass);return true;
    }
};
struct EffectPass {
    IsleGame* game;IsleGameCamera* camera;menu::EffectRuntime& effects;const menu::EffectGraphicsResources& resources;
    std::size_t particles=0;
    static bool prepare(void* user,std::span<std::uint32_t> words,std::uint64_t,
                        const scene::render::Pass&,scene::render::Prepared& output) {
        auto& self=*static_cast<EffectPass*>(user);menu::EffectCamera camera;
        if(!menu::source_effect_camera(self.game,self.camera,camera)) return false;
        const auto result=self.effects.render(camera,{words.subspan(fx_commands.offset/4,menu::EffectRuntime::max_render_words),
            self.resources.bindings(),self.resources.reservations()});
        if(!result.status||(result.words&1u)||result.words>menu::EffectRuntime::max_render_words) return false;
        self.particles=result.particles;words[fx_commands.offset/4+result.words]=0xdf000000;
        words[fx_commands.offset/4+result.words+1]=0;
        output.lanes[0]={static_cast<std::uint32_t>(fx_commands.offset),result.words+2};return true;
    }
};
struct Capture {
    std::vector<std::byte> color=std::vector<std::byte>(plane),depth=std::vector<std::byte>(plane);
    std::size_t triangles=0;std::uint64_t geometry=14695981039346656037ull;
};
struct Readback {
    enum class Failure { none,before_postprocess,after_postprocess } failure=Failure::none;
    player::Photo* photo=nullptr;actor::DetectorBinding* detection=nullptr;g::Frame* frame=nullptr;
    player::Photo::Ticket ticket{},preceding{};Capture capture;
    bool check_queue_order=false,queue_order_rejected=false,preceding_completed=false;
    unsigned calls=0;bool postprocessed=false;
    static void finish(void* user,rsp::CanonicalExecutor::MemoryLease& lease,std::uint32_t framebuffer) {
        auto& self=*static_cast<Readback*>(user);require(++self.calls==1,"photo acquired multiple actual completed mappings");
        require(self.frame->busy()&&!self.frame->quiescent()&&self.frame->submission()==self.ticket.submission,
            "photo completed callback lost its concrete in-flight Frame token");
        // This rejection is witnessed INSIDE a real completed GPU mapping, not
        // by a caller-provided busy flag or a simulated executor.
        require(self.photo->retire_failed(self.ticket,*self.frame)==ISLE_GAME_INVALID_STATE,
            "photo accepted failed retirement while actual completed mapping/readback was live");
        const auto& packets=lease.packets();
        for(const auto packet:packets.packets()) {
            const auto words=packets.words().subspan(packet.offset,packet.count);const auto opcode=(words[0]>>24)&63u;
            if(opcode>=8&&opcode<=15) { ++self.capture.triangles;
                for(const auto word:words) self.capture.geometry=(self.capture.geometry^word)*1099511628211ull;
            }
        }
        require(self.capture.triangles,"actual photo task completed without real RDP geometry");
        if(self.failure==Failure::before_postprocess) {
            const auto bytes=lease.rdram();
            for(std::size_t i=0;i<plane;++i) {
                self.capture.color[i]=bytes[(framebuffer+i)^3u];self.capture.depth[i]=bytes[(frame_layout.depth+i)^3u];
            }
            throw std::runtime_error("photo intended completed-memory failure before postprocess");
        }
        const auto old_published=self.detection->history.published();
        auto words=lease.postprocess_words();
        if(self.check_queue_order) {
            require(self.photo->postprocess(self.ticket,words,self.detection->copies.region_offset(0),framebuffer)==ISLE_GAME_INVALID_STATE,
                "second actual queued photo bypassed the first postprocessed ticket");
            self.queue_order_rejected=true;
            // The first concrete Frame already returned poll(completed). Only
            // now discharge that queue head, then process this actual mapping.
            native(self.photo->complete(self.preceding));self.preceding_completed=true;
        }
        native(self.photo->postprocess(self.ticket,words,self.detection->copies.region_offset(0),framebuffer));
        self.postprocessed=true;
        require(same(old_published,self.detection->history.published()),"photo postprocess published before native OM CopyInfo");
        const auto bytes=lease.rdram();
        for(std::size_t i=0;i<plane;++i) {
            self.capture.color[i]=bytes[(framebuffer+i)^3u];self.capture.depth[i]=bytes[(frame_layout.depth+i)^3u];
        }
        for(std::size_t at=guard.offset/4;at<(guard.offset+guard.size)/4;++at)
            require(words[at]==0x13579bdfu,"photo GPU/postprocess crossed its disjoint guard");
        lease.finish_postprocess();
        require(!lease.postprocess_active(),"photo retained a writable completed mapping");
        if(self.failure==Failure::after_postprocess) throw std::runtime_error("photo intended completed-memory failure after postprocess");
    }
};
// Cleanup is driven by actual Frame poll/failed-token evidence. This is not a
// backend, normal-completion shortcut, or a caller-supplied completion flag.
struct Tickets {
    struct Entry { player::Photo::Ticket ticket;g::Frame* frame;bool active=true; };
    player::Photo* photo;std::vector<Entry> entries;
    void add(player::Photo::Ticket ticket,g::Frame& frame) { entries.push_back({ticket,&frame,true}); }
    void enqueued(player::Photo::Ticket prepared,player::Photo::Ticket executing) {
        for(auto& entry:entries) if(entry.active&&entry.ticket==prepared) { entry.ticket=executing;return; }
        throw std::runtime_error("photo enqueue lost its prepared ticket ledger entry");
    }
    void retired(player::Photo::Ticket ticket) {
        for(auto& entry:entries) if(entry.active&&entry.ticket==ticket) { entry.active=false;return; }
        throw std::runtime_error("photo consumer lost its actual ticket ledger");
    }
    void retire(player::Photo::Ticket ticket) { native(photo->retire(ticket));retired(ticket); }
    bool empty() const { return std::none_of(entries.begin(),entries.end(),[](const auto& e){return e.active;}); }
    ~Tickets() {
        for(auto& entry:entries) if(entry.active) {
            auto& frame=*entry.frame;g::Poll result=g::Poll::idle;
            while(frame.busy()) { result=frame.poll();if(result==g::Poll::pending) std::this_thread::yield(); }
            if(frame.failed_submission(entry.ticket.submission)) {
                if(photo->retire_failed(entry.ticket,frame)!=ISLE_GAME_OK) std::terminate();
            } else {
                auto retired=photo->retire(entry.ticket);
                if(retired!=ISLE_GAME_OK) {
                    if((result!=g::Poll::completed&&!frame.completed())||photo->complete(entry.ticket)!=ISLE_GAME_OK||
                        photo->retire(entry.ticket)!=ISLE_GAME_OK) std::terminate();
                }
            }
        }
    }
};
PlayerPhotoRspMake source_make(IslePlayerSource& source,IsleGameObject* subject) {
    PlayerPhotoRspMake result{subject,-99,ISLE_GAME_INVALID_STATE,0,0};
    const auto status=isle_player_source_call(&source,player_photo_rsp_make,&result);
    if(status!=ISLE_GAME_OK) result.status=status;
    return result;
}
bool returned(const PlayerPhotoRspMake& result) { return result.entered==1&&result.returned==1; }
struct RollQueries {
    int focused=0;std::array<int,3> species{-1,-1,-1};int distinct=0;
    void verify(IslePlayerPhotoSource& source) const {
        require(isle_player_photo_func_8009BBF4(&source)==focused&&isle_player_photo_func_8009BC00(&source)==distinct,
            "original focused/distinct photo queries differ from actual successful source calls");
        for(const auto id:species) if(id>=0)
            require(isle_player_photo_func_8009BC0C(&source,id)==0,"original observed-species query lost actual source capture");
        require(isle_player_photo_func_8009BC0C(&source,-12345)==1,"original absent-species query fabricated membership");
        require(isle_player_photo_func_8009BC68(&source)==source.gPhotoCount&&
            isle_player_photo_func_8009BC74(&source)==source.D_800B0598&&
            isle_player_photo_func_8009BC80(&source,-1)==nullptr,"original roll count/base/negative-index queries changed");
        if(source.gPhotoCount<60) {
            require(isle_player_photo_func_8009BC80(&source,source.gPhotoCount)==&source.D_800B0598[source.gPhotoCount]&&
                isle_player_photo_func_8009BC80(&source,source.gPhotoCount+1)==nullptr,
                "original inclusive-count roll query semantics changed");
        }
    }
    void taken(bool focus,int id) {
        if(!focus) return;++focused;
        if(distinct<3&&std::find(species.begin(),species.begin()+distinct,id)==species.begin()+distinct) species[distinct++]=id;
    }
};
void verify_snapshot(IslePlayerPhotoSource& source,player::Photo::Ticket ticket,IslePlayerSource& player,
    actor::Owner& actors,items::Owner& items,actor::DetectorBinding& detection,world::World& world,bool require_fx) {
    const auto& captured=source.PokemonDetector_Photo[ticket.context];const auto& data=captured.main;
    const IsleGameCameraParameters* camera=nullptr;native(isle_game_camera_parameters(player.game,player.camera,&camera));
    require(data.levelID==0&&data.unk_00_16==world.current()->index&&data.unk_04.f32==world.global_time()&&
        same(data.unk_08,{camera->look_at.eye[0],camera->look_at.eye[1],camera->look_at.eye[2]})&&
        same(data.unk_14,{camera->look_at.at[0],camera->look_at.at[1],camera->look_at.at[2]}),
        "actual source snapshot lost native camera/World block/time");
    const auto* members=detection.history.photo(ticket.context);require(members,"actual staged membership is absent");
    unsigned pokemon=0,item_count=0,effect_count=0;
    for(std::size_t i=0;i<12;++i) {
        require(captured.pokemonObjects[i]==reinterpret_cast<IsleGameObject*>(members->objects[i]),
            "source photo recollected membership on a different clock");
        if(!captured.pokemonObjects[i]) { require(data.pokemons[i].pokemonID==-1,"source empty Pokemon lacks original sentinel");continue; }
        bool found=false;
        for(const auto& binding:actors.render_bindings()) if(binding.model.object==captured.pokemonObjects[i]) {
            auto* native_source=actors.source(binding.model.state.instance);const IsleGameModelPlacement* placement=nullptr;
            native(isle_game_model_placement(player.game,binding.model.model,&placement));
            require(native_source&&data.pokemons[i].pokemonID==native_source->id&&same(data.pokemons[i].position,placement->transform.position)&&
                data.pokemons[i].yaw==placement->transform.rotation.y&&
                data.pokemons[i].animationTime==isle_actor_owner_animation_time(native_source),
                "actual Pokemon photo lost canonical native identity/AUX/animation state");
            require(data.pokemons[i].pokemonID==129,"bounded photo cohort captured a non-Magikarp substitute");found=true;++pokemon;
        }
        require(found,"source captured a Pokemon outside the actual Owner bindings");
    }
    require(pokemon==1,"actual Magikarp was not admitted by the unchanged native source camera/membership");
    for(const auto& item:data.items) if(item.itemType>=0) {
        bool found=false;
        for(const auto& binding:items.render_bindings()) {
            if(!binding.model.object) continue;
            const IsleGameModelNode* nodes=nullptr;std::size_t count=0;
            native(isle_game_model_peek_nodes(player.game,binding.model.model,&nodes,&count));
            const int type=binding.family==items::Family::apple?2:binding.family==items::Family::pester_ball?1:
                binding.family==items::Family::water_splash?3:4;
            if(count&&item.itemType==type&&same(item.pos,nodes[0].transform.position)) {
                const auto model=binding.model.runtime->model(binding.model.state.instance);require(bool(model),"actual Item runtime expired");
                unsigned time=0;
                if(type<3) {
                    require(model.nodes.size()>1&&model.nodes[1].material_count,"actual photographed Item lacks source material clock");
                    time=static_cast<unsigned>(static_cast<int>(model.materials[model.nodes[1].first_material].clock.time_passed));
                    if(type==2) time=(time&15u)|(static_cast<unsigned>(static_cast<int>((nodes[0].transform.scale.x*15.0f)/0.1f))*16u);
                } else time=static_cast<unsigned>(static_cast<int>(model.animation_time));
                require(item.animationTime==static_cast<std::uint8_t>(time),"actual Item source clock/scale packing changed");found=true;
            }
        }
        require(found,"photo Item has no actual native ItemOwner counterpart");++item_count;
    }
    require(item_count>=1,"actual source photo omitted all genuine ItemOwner objects");
    for(const auto& effect:data.effects) if(effect.textureID>=0) {
        bool found=false;
        for(std::size_t i=0;i<actors.effects().particle_count();++i) {
            menu::EffectParticleView value;require(actors.effects().particle(i,value),"actual FX particle borrow failed");
            if(effect.textureID==value.texture&&effect.dataID==value.frame&&effect.bankID==(value.bank_id&7u)&&
                effect.unk_03==((value.flags>>4)&255u)&&effect.posX==static_cast<std::int16_t>(static_cast<int>(value.position.x*8))&&
                effect.posY==static_cast<std::int16_t>(static_cast<int>(value.position.y*8))&&
                effect.posZ==static_cast<std::int16_t>(static_cast<int>(value.position.z*8))&&
                effect.size==static_cast<std::uint16_t>(static_cast<int>(value.size*128))&&
                std::equal(std::begin(effect.primColor),std::end(effect.primColor),value.primitive.begin())) found=true;
        }
        require(found,"photo FX has no actual shared EffectRuntime counterpart");++effect_count;
    }
    require(!require_fx||effect_count>0,"qualified actual common3 FX was not captured; no empty FX fallback is accepted");
    const auto wire=encode(data);
    require(be32(wire,4)==std::bit_cast<std::uint32_t>(world.global_time())&&
        be32(wire,8)==std::bit_cast<std::uint32_t>(camera->look_at.eye[0])&&
        be32(wire,12)==std::bit_cast<std::uint32_t>(camera->look_at.eye[1])&&
        be32(wire,16)==std::bit_cast<std::uint32_t>(camera->look_at.eye[2]),
        "canonical encoded proof lost actual camera/World binary32 fields");
    for(std::size_t i=0;i<12;++i) {
        const auto packed=be32(wire,32+i*24);const auto& p=data.pokemons[i];
        require((packed>>19)==(static_cast<unsigned>(p.pokemonID)&8191u)&&((packed>>13)&63u)==p.unk_00_13&&
            ((packed>>8)&31u)==p.specialID&&(packed&255u)==p.poseID,"actual canonical Pokemon wire packing changed");
    }
}
void run(rom::Snapshot& snapshot,const std::filesystem::path& output) {
    packing_edges();
    const auto profile=output/"profile";std::filesystem::create_directory(profile);
    const world::Resources world_resources(snapshot);const auto& setup=world_resources.setup();
    const beach::ActorResources actor_resources(snapshot,{setup.fog_rgb,setup.fog_min,setup.fog_max});
    const beach::BehaviorResources behaviors(snapshot);
    const items::Resources item_resources(snapshot,setup.fog_rgb,setup.fog_min,setup.fog_max);
    const player::PhotoResources photo_resources(snapshot);const environment_sound::BeachResources sounds(snapshot);
    const menu::EffectResources effect_resources(snapshot);const photo::LightingInputs lighting(snapshot);
    const std::array banks{&effect_resources.bank(menu::EffectGroup::common),&effect_resources.bank(menu::EffectGroup::beach)};
    const menu::EffectGraphicsResources effect_graphics(snapshot,banks,{0x700000,0x80000});
    auto hydrated=game_audio::hydrate(snapshot);require(bool(hydrated),"actual photo audio hydration failed");
    std::array<audio::effects::Section,8> sections;audio::effects::Config effects;
    require(game_audio::native_startup_effects(hydrated.resources,sections,effects)==audio::Error::none,"actual photo audio effects failed");
    private_audio::Capture pcm;private_audio::Execution execution{snapshot,nullptr,{}};
    pcm.samples.resize(512*audio::AudioService::max_frames*2);
    auto made=game_audio::Session::create(std::move(hydrated.resources),effects,pcm.output(),execution.executor());
    require(bool(made),"actual photo audio Session failed");auto& session=*made.session;std::uint64_t retrace=0;
    const auto pump=[&] {
        ++retrace;require(session.retrace(retrace,retrace*1000000000ull/60)==audio::Error::none,"actual photo audio retrace failed");
        require(session.submit()==audio::Error::none,"actual photo PCM submit failed");audio::TaskView task;
        require(session.build(task)==audio::Error::none,"actual photo RSP audio task failed");
        if(task.token.serial) { private_audio::execute(session,execution,task);require(session.submit()==audio::Error::none,"actual completed photo PCM submit failed"); }
    };
    unsigned gpu_frames=0,copy_updates=0,failed_frames=0;std::uint64_t generation=0;
    {
        game::State state(profile);require(state.initialize(1)&&state.save(),"actual private photo application save initialization failed");
        IslePlayerPhotoSource application{};native(photo_resources.bind(application,*state.save()));
        // Exactly one application initialization, not a course-owner reset.
        isle_player_photo_func_8009CE00(&application);RollQueries queries;queries.verify(application);
        for(const auto& photo:application.D_800B0598) require(photo.levelID==-1&&photo.unk_00_24==0,"original app roll initialization omitted a record");
        game_input::Service input;scene::Storage storage(32*1024*1024);native(storage.create());
        IslePlayerSource source{};
        {
            scene::Services services(storage.game(),storage.arena(),{{4,6,0,1,2},{}},input,session,state);
            require(!services.failed(),services.error());
            native(isle_player_source_prepare(services.game,&source,&services.rng,&source,10));
            native(isle_player_source_activate(&source));SourceLifetime source_lifetime{source};source.photo=&application;
            auto* root=services.object(0,0);auto* camera_object=services.object(1,0);auto* player_object=services.object(3,9);
            require(root&&camera_object&&player_object,"actual photo scene/player/camera roots failed");
            IsleGameCamera* camera=nullptr;native(isle_game_attach_camera(services.game,camera_object,&camera));
            native(isle_game_camera_add_matrix(services.game,camera,ISLE_GAME_CAMERA_PERSPECTIVE_FAST));
            native(isle_game_camera_add_matrix(services.game,camera,ISLE_GAME_CAMERA_LOOK_AT_REFLECT_ROLL));
            const IsleGamePerspective perspective{55,4.0f/3.0f,10,25600,1};
            const IsleGameViewport viewport{{640,480,511,0},{640,480,511,0}};
            native(isle_game_camera_set_perspective(services.game,camera,&perspective));
            native(isle_game_camera_set_look_at(services.game,camera,&source_view));native(isle_game_camera_set_viewport(services.game,camera,&viewport));
            IsleGameModelNodeInit node{};node.parent=UINT32_MAX;node.transform.scale={1,1,1};node.matrix_kind=ISLE_GAME_MODEL_CONTEXTUAL_RPY;
            IsleGameModel* player_model=nullptr;native(isle_game_attach_model(services.game,player_object,&node,1,&player_model));
            const IsleGameModelTransform placement{{0,1600,6000},{0,0,0},{1,1,1}};
            native(isle_game_model_set_placement(services.game,player_model,ISLE_GAME_MODEL_PLACEMENT_RT,&placement));
            source.model=player_model;source.camera=camera;source.cam_target={0,1133,5000};source.gDirectionIndex=-1;
            native(isle_player_source_bind(services.game,player_object,&source));
            IslePlayerSource* actual=nullptr;native(isle_player_source_get(services.game,player_object,&actual));require(actual==&source,"native PLAYER root lost canonical source identity");
            environment_sound::Environment environment(services,storage.arena(),sounds);
            require(environment.initialize(nullptr,camera),"environment source initialization failed");
            focus::Detector detector;focus::RegionCopies copies(capture_layout);actor::DetectorBinding detection{copies,detector};detection.enabled=true;
            WorldHooks hooks{services,camera};
            world::World world(services,world_resources,{&hooks,nullptr,WorldHooks::remove,nullptr,WorldHooks::camera_shift,WorldHooks::effects_shift,nullptr});world.follow_camera(camera);
            a::SlotCapacity actor_capacity{},item_capacity{};
            const auto capacity=[](auto models,a::SlotCapacity& maximum) {
                for(const auto& model:models) { const auto c=a::slot_capacity(model);maximum.nodes=std::max(maximum.nodes,c.nodes);
                    maximum.material_attachments=std::max(maximum.material_attachments,c.material_attachments); }
            };
            capacity(actor_resources.models(),actor_capacity);capacity(item_resources.models(),item_capacity);
            const std::array actor_capacities{actor_capacity,actor_capacity};std::array<a::SlotCapacity,8> item_capacities;item_capacities.fill(item_capacity);
            {
                actor::Owner actors(services,actor_resources,world,environment,detector,actor_capacities,banks,{32768,4096});hooks.actors=&actors;
                accepted(actors.publish_effects());
                accepted(actors.set_player(player_object,player_model,camera));actors.set_direction(-1);
                require(environment.set_listener(player_model,camera),"environment listener binding failed");
                items::Globals globals;items::Owner items(services,item_resources,world,actors,globals,
                    {player_object,player_model,camera,&source.cam_target},item_capacities,{32768,4096});hooks.items=&items;
                OwnerLifetime owner_lifetime{actors,items,world,hooks};
                require(world.enter_first(0)==&world.blocks()[0],"actual photo World first block failed");source.movement.block=world.current();
                accepted(items.init());accepted(items.set_custom_functions(nullptr,0,nullptr));
                const auto ball=items.spawn_ball({-180,1233,4000},{}),apple=items.spawn_apple({180,1233,4000},{});
                accepted(ball.status);accepted(apple.status);
                auto* trigger=services.object(0x1000,4);require(trigger,"actual Magikarp trigger root failed");
                node.transform.position=trigger_position;IsleGameModel* trigger_model=nullptr;
                native(isle_game_attach_model(services.game,trigger,&node,1,&trigger_model));
                beach::MagikarpScene spawner(services,actors,world,actor_resources,behaviors);
                auto actor=spawner.spawn_at({trigger,trigger_model});require(bool(actor),actor.detail);
                std::unique_ptr<player::Photo> photo=std::make_unique<player::Photo>(application,source,actors,items,detection,photo_resources,world,0);
                std::unique_ptr<actor::PhotoScene> controller=std::make_unique<actor::PhotoScene>(services,actors,root,camera,detection,&application);
                PlayerPhotoRspCopies copy_witness{};
                require(services.set_update(controller->copy_info_object(),player_photo_rsp_copy_info,&copy_witness),"original C OM CopyInfo fixture installation failed");
                IsleGameCamera* fx_camera=nullptr;native(isle_game_attach_camera(services.game,actors.particle_object(),&fx_camera));native(isle_game_camera_set_viewport(services.game,fx_camera,&viewport));
                auto created=g::Frame::create(snapshot,{true,{16000000,4*1024*1024},frame_layout});require(bool(created),created.message.data());
                std::vector<menu::RamRange> mutable_ranges{world_layout.payloads,world_layout.lists,world_layout.materials,
                    actor_layout.payloads,actor_layout.lists,actor_layout.materials,item_layout.payloads,item_layout.lists,item_layout.materials,
                    fx_commands,joined_lists,guard};
                for(const auto range:copies.reservations()) mutable_ranges.push_back(range);
                for(const auto& span:lighting.installation_spans()) mutable_ranges.push_back({physical(span.guest_address),span.bytes.size()});
                auto all=mutable_ranges;
                for(const auto range:created.frame->reservations()) if(range.address) all.push_back({range.address,range.size});
                for(const auto range:actor_layout.framebuffers) all.push_back(range);
                for(const auto range:effect_graphics.reservations()) all.push_back(range);
                const auto others=[&](std::array<menu::RamRange,3> own,bool framebuffers) {
                    std::vector<menu::RamRange> result;
                    for(const auto range:all) { bool skip=false;
                        for(const auto mine:own) skip|=range.offset==mine.offset&&range.size==mine.size;
                        if(framebuffers) for(const auto fb:actor_layout.framebuffers) skip|=range.offset==fb.offset&&range.size==fb.size;
                        if(!skip) result.push_back(range);
                    }return result;
                };
                const auto world_other=others({world_layout.payloads,world_layout.lists,world_layout.materials},false);
                const auto actor_other=others({actor_layout.payloads,actor_layout.lists,actor_layout.materials},true);
                const auto item_other=others({item_layout.payloads,item_layout.lists,item_layout.materials},true);
                menu::model::CameraBasisState basis;
                world::Producer world_producer(services.game,camera,basis,world_resources,world,world_layout,world_other);
                actor::Producer actor_producer(services.game,camera,basis,actor_resources,actor_capacities,actors.render_bindings(),actor_layout,actor_other,&detection);
                items::Producer item_producer(services.game,camera,basis,item_resources,item_capacities,items.render_bindings(),item_layout,item_other);
                course::ItemComposition composition{services,item_producer,128};course::ModelProducer models(world_producer,actor_producer,joined_lists,&composition);
                ModelPass model_pass{models,copies,lighting,services.game,camera};EffectPass effect_pass{services.game,camera,actors.effects(),effect_graphics};
                std::vector<sprite::AssetSpan> immutable;
                const auto append=[&](auto spans) { for(const auto& span:spans) immutable.push_back({physical(span.guest_address),
                    {reinterpret_cast<const std::uint8_t*>(span.bytes.data()),span.bytes.size()}}); };
                append(world_resources.installation_spans());append(actor_resources.installation_spans());append(item_resources.installation_spans());
                for(const auto span:effect_graphics.installation_spans()) immutable.push_back(span);
                std::vector<sprite::RamRange> reservations;
                for(const auto range:mutable_ranges) reservations.push_back({static_cast<std::uint32_t>(range.offset),static_cast<std::uint32_t>(range.size)});
                std::array<scene::render::Pass,2> passes;
                passes[0].camera=camera;passes[0].priority=5;passes[0].flags=3;passes[0].camera_tag=UINT32_MAX;
                passes[0].background_rgba=(std::uint32_t(setup.background_rgb[0])<<24)|(std::uint32_t(setup.background_rgb[1])<<16)|(std::uint32_t(setup.background_rgb[2])<<8)|255u;
                passes[0].link_mask=(std::uint64_t(1)<<3)|(std::uint64_t(1)<<4)|(std::uint64_t(1)<<5);
                passes[0].prepare=ModelPass::prepare;passes[0].userdata=&model_pass;
                passes[1].camera=fx_camera;passes[1].priority=4;passes[1].camera_tag=UINT32_MAX;passes[1].prepare=EffectPass::prepare;passes[1].userdata=&effect_pass;
                scene::render::Input scene_input{services.game,{1,nullptr,immutable,reservations,{0x3b5000,0x3da800},physical(lighting.pre_render_guest())},passes,{}};
                std::array<Readback,4> readbacks;
                auto primary=std::move(created.frame);std::unique_ptr<g::Frame> secondary;
                Tickets tickets{photo.get(),{}};tickets.entries.reserve(70);
                const auto boundary=[&] { require(primary->quiescent()&&(!secondary||secondary->quiescent()),"photo native mutation crossed actual GPU/readback ownership"); };
                const auto tick=[&] {
                    boundary();const auto before=copy_witness.returned;
                    native(isle_game_tick(services.game));require(!services.failed(),services.error());
                    require(copy_witness.entered==copy_witness.returned&&copy_witness.returned==before+1,
                        "original C one-argument CopyInfo did not run exactly once in native OM");
                    ++copy_updates;pump();
                };
                const auto fresh_secondary=[&] {
                    require(!secondary,"secondary real Frame would be overwritten");
                    auto next=g::Frame::create(snapshot,{true,{16000000,4*1024*1024},frame_layout});require(bool(next),next.message.data());secondary=std::move(next.frame);
                };
                const auto begin=[&](g::Frame& frame,Readback& readback,bool with_fx,bool expect_fx) {
                    require(std::none_of(tickets.entries.begin(),tickets.entries.end(),[&](const auto& entry) {
                        return entry.active&&entry.frame==&frame;
                    }),"concrete Frame begin reuse over an outstanding photo ticket");
                    boundary();readback.calls=0;readback.postprocessed=false;
                    readback.capture.triangles=0;readback.capture.geometry=14695981039346656037ull;
                    readback.photo=photo.get();readback.detection=&detection;readback.frame=&frame;
                    actors.render_bindings();items.render_bindings();scene_input.passes=std::span(passes).first(with_fx?2:1);
                    const auto expected_context=static_cast<unsigned>(1-detection.context);
                    require(frame.begin(scene_input,{expected_context},{&readback,Readback::finish}),frame.message());
                    // No tick, animation, input publication or source mutation may
                    // intervene between successful begin, snapshot and enqueue.
                    const player::Photo::Ticket ticket{detection.context,++generation,frame.submission()};
                    readback.ticket=ticket;
                    require(ticket.context==frame.submission_context()&&ticket.submission,"actual staged Detector/Frame context or token mismatch");
                    const player::Photo::Ticket prepared{ticket.context,ticket.generation,0};
                    const auto snapshotted=photo->snapshot(ticket.context,ticket.generation);
                    if(snapshotted!=ISLE_GAME_OK) { static_cast<void>(photo->retire(prepared));native(snapshotted); }
                    tickets.add(prepared,frame);
                    const auto queued=photo->enqueue(ticket,frame);
                    if(queued==ISLE_GAME_OK) tickets.enqueued(prepared,ticket);
                    native(queued);
                    verify_snapshot(application,ticket,source,actors,items,detection,world,expect_fx);
                    ++gpu_frames;return ticket;
                };
                const auto poll=[&](g::Frame& frame,bool failure) {
                    g::Poll result;do { result=frame.poll();if(result==g::Poll::pending) std::this_thread::yield(); } while(result==g::Poll::pending);
                    require(frame.quiescent()&&!frame.busy(),"actual photo Frame did not end all GPU/mapping/readback ownership");
                    if(failure) {
                        require(result==g::Poll::failed&&frame.error()==g::Error::execution&&!frame.completed(),"completed-memory exception did not become an unpublished real failed Frame");
                        ++failed_frames;
                    } else require(result==g::Poll::completed&&frame.error()==g::Error::none&&bool(frame.completed()),frame.message());
                };
                const auto make=[&](player::Photo::Ticket ticket) {
                    boundary();const auto published=detector.published();
                    require(published.context==ticket.context,"makePhoto did not follow matching native CopyInfo publication");
                    application.PokemonDetector_TakenPhotoIndex=ticket.context;
                    auto* subject=published.present&&(published.flags&4u)?reinterpret_cast<IsleGameObject*>(published.object):nullptr;
                    const auto before=application.gPhotoCount;const IslePlayerPhotoContext* borrowed=nullptr;
                    native(photo->borrow(ticket,borrowed));
                    auto expected=encode(borrowed->main);
                    const bool subject_matches=!subject||(borrowed->pokemonObjects[0]==subject&&borrowed->main.pokemons[0].pokemonID==published.pokemon_id);
                    if(subject) expected[1]|=std::byte{0x20}; // Sole actual species129 is already first.
                    // No owning C++ frame is crossed by source failure: the C
                    // wrapper captures value/status, and release occurs first.
                    const auto result=source_make(source,subject);const auto released=photo->release(ticket);
                    native(released);require(returned(result)&&result.value==1&&result.status==ISLE_GAME_OK&&application.gPhotoCount==before+1,
                        "original makePhoto did not append exactly one actually completed/published/borrowed context");
                    queries.taken(subject!=nullptr,published.pokemon_id);queries.verify(application);
                    const auto& stored=application.D_800B0598[before];
                    require(subject_matches&&encode(stored)==expected,"original capture did not preserve actual canonical camera/Pokemon/Item/FX source values");
                    if(subject) {
                        require(stored.pokemons[0].pokemonID==published.pokemon_id&&(stored.unk_00_16&0xe0u),
                            "original focused capture failed subject-first/flag packing");
                        IslePlayerPhotoContext query{};query.main=stored;
                        require(isle_player_photo_func_8009BCC4(&application,&query)==published.pokemon_id,"original captured-subject query disagrees with real focus");
                    }
                };
                // Native source warmup, then the qualified common3 third-VM
                // positive-step phase. No source particle arrays or pose search.
                for(unsigned n=0;n<6;++n) tick();accepted(items.pause());
                const auto particle=actors.effects().create_particle(3,0,{500,1233,4000},{});
                require(bool(particle.status)&&particle.spawned,"actual common3 FX factory failed");tick();tick();
                native(isle_actor_source_pause_object(actors.source(actor.actor.instance),1));native(isle_game_pause_updates(services.game,actor.actor.object,true));
                fresh_secondary();
                const auto first=begin(*primary,readbacks[0],false,true);poll(*primary,false);
                require(readbacks[0].postprocessed,"first actual queue head was not postprocessed");
                readbacks[1].check_queue_order=true;readbacks[1].preceding=first;
                const auto full=begin(*secondary,readbacks[1],true,true);
                require(photo->retire(full)==ISLE_GAME_INVALID_STATE,"queued GPU photo retired without failed-token proof");
                poll(*secondary,false);native(photo->complete(full));
                require(readbacks[1].queue_order_rejected&&readbacks[1].preceding_completed&&readbacks[1].postprocessed,
                    "actual two-Frame sequential-GPU photo queue order was not exercised");
                require(effect_pass.particles&&readbacks[0].capture.geometry==readbacks[1].capture.geometry&&
                    readbacks[0].capture.color!=readbacks[1].capture.color&&readbacks[0].capture.depth==readbacks[1].capture.depth,
                    "qualified actual FX failed paired GPU color-only proof at unchanged native World/Actor/Item pose");
                save_image(output,secondary->completed());save(output/"photo-full.rgba16be",readbacks[1].capture.color);
                save(output/"photo-full.depth16be",readbacks[1].capture.depth);
                application.PokemonDetector_TakenPhotoIndex=full.context;
                const IslePlayerPhotoContext* unpub=nullptr;native(photo->borrow(full,unpub));
                const auto rejected_unpublished=source_make(source,nullptr);native(photo->release(full));
                require(returned(rejected_unpublished)&&rejected_unpublished.value==-1&&rejected_unpublished.status==ISLE_GAME_INVALID_STATE&&application.gPhotoCount==0,
                    "original makePhoto accepted a completed but unpublished context");
                tick();const auto rejected_unborrowed=source_make(source,nullptr);
                require(returned(rejected_unborrowed)&&rejected_unborrowed.value==-1&&rejected_unborrowed.status==ISLE_GAME_INVALID_STATE&&application.gPhotoCount==0,
                    "original makePhoto accepted an unborrowed published context");
                auto wrong=full;++wrong.generation;const IslePlayerPhotoContext* absent=&application.PokemonDetector_Photo[full.context];
                require(photo->borrow(wrong,absent)==ISLE_GAME_INVALID_STATE&&!absent&&photo->release(full)==ISLE_GAME_INVALID_STATE&&
                    photo->retire(wrong)==ISLE_GAME_INVALID_STATE,"wrong-ticket/unborrowed photo APIs did not reject");
                make(full);tickets.retire(first);tickets.retire(full);
                require(secondary->retire(),"actual queue Frame retirement failed");secondary.reset();
                readbacks[1].check_queue_order=false;

                const auto successful=[&](Readback& readback) {
                    const auto ticket=begin(*primary,readback,true,false);poll(*primary,false);native(photo->complete(ticket));tick();make(ticket);return ticket;
                };
                // Both cases use actual GPU completion, then a throwing actual
                // memory callback. The second throws only after Photo postprocess
                // and finish_postprocess. No failed ticket uses normal complete.
                for(unsigned failure_case=0;failure_case<2;++failure_case) {
                    const auto previous=successful(readbacks[0]);
                    const auto prior_source=encode(application.PokemonDetector_Photo[previous.context].main);
                    std::array<Encoded,60> prior_roll;for(std::size_t i=0;i<60;++i) prior_roll[i]=encode(application.D_800B0598[i]);
                    const auto prior_count=application.gPhotoCount,prior_focused=application.D_800AE27C,prior_distinct=application.D_800AE280;
                    const auto original_actor=actor.actor;
                    const auto held_before=services.source_owner_borrows(original_actor.object).fields;
                    require(held_before>0,"completed context retained no actual Actor semantic field borrow");
                    fresh_secondary();auto& failed_readback=readbacks[2+failure_case];
                    failed_readback.failure=failure_case?Readback::Failure::after_postprocess:Readback::Failure::before_postprocess;
                    const auto failed=begin(*secondary,failed_readback,true,false);
                    require(services.source_owner_borrows(original_actor.object).fields>held_before,"failed candidate did not acquire actual Actor semantic/native ownership");
                    require(photo->retire_failed(failed,*secondary)==ISLE_GAME_INVALID_STATE&&
                        !secondary->failed_submission(failed.submission),"nonquiescent executing photo accepted failed retirement");
                    poll(*secondary,true);
                    require(failed_readback.calls==1&&failed_readback.capture.triangles&&
                        failed_readback.postprocessed==(failure_case!=0)&&secondary->failed_submission(failed.submission),
                        "failure case lacks its exact real completed-GPU/postprocess/failed-token evidence");
                    const std::string failure_name=failure_case?"failed-after-postprocess":"failed-before-postprocess";
                    save(output/(failure_name+".rgba16be"),failed_readback.capture.color);
                    save(output/(failure_name+".depth16be"),failed_readback.capture.depth);
                    save(output/(failure_name+".unpublished-photo.bin"),encode(application.PokemonDetector_Photo[failed.context].main));
                    // Cleanup actually joins the native Actor. Its semantic slot
                    // and native dead identity must remain held by both tickets.
                    accepted(actors.cleanup(original_actor.instance));
                    require(actors.membership(original_actor.instance).joined&&services.source_owner_joined(original_actor.object)&&
                        services.source_owner_registered(original_actor.object),"actual failed-photo Actor did not join while semantic/native identity stayed held");
                    const auto held_collection=actors.collect_joined();
                    require(!held_collection&&held_collection.error==actor::OwnerError::lifetime,"actual Actor slot collected through outstanding photo holds");
                    const auto published_before_retirement=detector.published();
                    auto wrong_generation=failed;++wrong_generation.generation;auto wrong_token=failed;++wrong_token.submission;
                    auto wrong_context=failed;wrong_context.context=previous.context;
                    require(photo->retire_failed(failed,*primary)==ISLE_GAME_INVALID_STATE&&
                        photo->retire_failed(wrong_generation,*secondary)==ISLE_GAME_INVALID_STATE&&
                        photo->retire_failed(wrong_token,*secondary)==ISLE_GAME_INVALID_STATE&&
                        photo->retire_failed(wrong_context,*secondary)==ISLE_GAME_INVALID_STATE&&
                        photo->retire(failed)==ISLE_GAME_INVALID_STATE,"failed photo accepted wrong Frame/token/generation/context or ordinary retirement");
                    native(photo->retire_failed(failed,*secondary));tickets.retired(failed);
                    require(detector.context_available(failed.context)&&!detector.context_available(previous.context)&&
                        services.source_owner_borrows(original_actor.object).fields==held_before&&
                        encode(application.PokemonDetector_Photo[previous.context].main)==prior_source&&
                        same(detector.published(),published_before_retirement)&&application.gPhotoCount==prior_count&&
                        application.D_800AE27C==prior_focused&&application.D_800AE280==prior_distinct,
                        "exact failed retirement changed the previous completed/published context/roll or failed to release its actual holds");
                    for(std::size_t i=0;i<60;++i) require(encode(application.D_800B0598[i])==prior_roll[i],"failed photo modified an application roll record");
                    // Native OM republishes the still-completed prior context,
                    // never the failed one, including after its postprocessing.
                    tick();application.PokemonDetector_TakenPhotoIndex=previous.context;
                    const IslePlayerPhotoContext* previous_borrow=nullptr;native(photo->borrow(previous,previous_borrow));
                    const auto previous_bytes=encode(previous_borrow->main);const auto taken=photo->require_taken_context();
                    const auto previous_released=photo->release(previous);native(previous_released);native(taken);
                    require(detector.published().context==previous.context&&previous_bytes==prior_source,
                        "failed retirement lost the previous completed CopyInfo publication");
                    require(secondary->retire(),"actual failed Frame did not retire after exact ticket release");secondary.reset();
                    // The next real prefix targets the retained completed context.
                    // Exercise its production read-only preflight, not a made-up
                    // detector error or Frame reuse over an outstanding ticket.
                    const auto saved_context=detection.context;const auto saved_photo=detection.photo;
                    const auto saved_computed=detector.computed(),saved_published=detector.published();
                    std::vector<std::uint32_t> rejected_ram(beach::rdram_size/4,0x58a19c3du);bool retained_rejected=false;
                    try { static_cast<void>(actor_producer.prepare(rejected_ram,900000+generation,passes[0])); }
                    catch(const std::exception& error) { retained_rejected=std::string(error.what()).find("prefix context is retained")!=std::string::npos; }
                    require(retained_rejected&&!services.failed()&&detection.context==saved_context&&
                        detection.photo.objects==saved_photo.objects&&detection.photo.focus==saved_photo.focus&&
                        same(detector.computed(),saved_computed)&&same(detector.published(),saved_published)&&
                        std::all_of(rejected_ram.begin(),rejected_ram.end(),[](auto value){return value==0x58a19c3du;}),
                        "retained completed context did not reject actual next preparation before RAM/detector publication");
                    const IslePlayerPhotoContext* held=nullptr;native(photo->borrow(previous,held));
                    const auto borrowed_retirement=photo->retire(previous);native(photo->release(previous));
                    require(borrowed_retirement==ISLE_GAME_INVALID_STATE,"completed photo retired through a real consumer borrow");
                    tickets.retire(previous);
                    require(services.source_owner_borrows(original_actor.object).fields==0,"retired actual photo tickets left an Actor semantic field borrow");
                    accepted(actors.collect_joined());require(!services.source_owner_registered(original_actor.object)&&
                        !actors.membership(original_actor.instance).joined,"actual Actor semantic/native held slot did not collect after exact releases");
                    actor=spawner.spawn_at({trigger,trigger_model});require(bool(actor),actor.detail);tick();
                    native(isle_actor_source_pause_object(actors.source(actor.actor.instance),1));native(isle_game_pause_updates(services.game,actor.actor.object,true));
                    // The next loop's successful() is the real next GPU after
                    // release; after the final failure it is the first roll fill.
                }
                require(tickets.empty()&&photo->quiescent(),"course photo boundary retained tickets");
                std::array<Encoded,60> retained_roll;for(std::size_t i=0;i<60;++i) retained_roll[i]=encode(application.D_800B0598[i]);
                const auto retained_count=application.gPhotoCount;
                controller->retire();controller.reset();photo.reset();
                require(!application.owner&&application.gPhotoCount==retained_count,"course Photo/PhotoScene retirement reset application roll ownership/count");
                for(std::size_t i=0;i<60;++i) require(encode(application.D_800B0598[i])==retained_roll[i],"course photo retirement changed application roll bytes");
                photo=std::make_unique<player::Photo>(application,source,actors,items,detection,photo_resources,world,0);
                controller=std::make_unique<actor::PhotoScene>(services,actors,root,camera,detection,&application);
                require(services.set_update(controller->copy_info_object(),player_photo_rsp_copy_info,&copy_witness),"reentered actual C CopyInfo installation failed");
                tickets.photo=photo.get();
                while(application.gPhotoCount<60) { const auto ticket=successful(readbacks[0]);tickets.retire(ticket); }
                require(application.gPhotoCount==60&&photo->quiescent(),"actual application roll did not reach sixty completed source photos");queries.verify(application);
                // Source selection toggles the actual captured record's low
                // wire flag; payload/camera/subjects remain byte-for-byte intact.
                auto& selection=application.D_800B0598[0].unk_00_24;
                const auto unselected=encode(application.D_800B0598[0]);auto selected=unselected;selected[0]|=std::byte{1};
                isle_player_photo_func_8009CDC0(&application,&selection,1);
                require(isle_player_photo_func_8009CDE4(&application,&selection)==1&&encode(application.D_800B0598[0])==selected,
                    "original actual-photo selection changed payload bits or lost its canonical wire flag");
                isle_player_photo_func_8009CDC0(&application,&selection,0);
                require(isle_player_photo_func_8009CDE4(&application,&selection)==0&&encode(application.D_800B0598[0])==unselected,
                    "original actual-photo deselection failed exact payload restoration");
                isle_player_photo_func_8009CDC0(&application,&selection,1);
                for(unsigned i=0;i<60;++i) {
                    const auto wire=encode(application.D_800B0598[i]);const auto name=std::string("photo-")+(i<10?"0":"")+std::to_string(i)+".bin";save(output/name,wire);
                    require(application.D_800B0598[i].levelID==0,"full actual roll contains an uncaptured source record");
                }
                const auto full_count=application.gPhotoCount,focused=application.D_800AE27C,distinct=application.D_800AE280;
                std::array<Encoded,60> full_roll;for(std::size_t i=0;i<60;++i) full_roll[i]=encode(application.D_800B0598[i]);
                const auto exhausted=source_make(source,nullptr);
                require(returned(exhausted)&&exhausted.value==0&&exhausted.status==ISLE_GAME_OK&&application.gPhotoCount==full_count&&
                    application.D_800AE27C==focused&&application.D_800AE280==distinct,
                    "original sixty-photo exhaustion was not a successful-status zero/no-append result");
                for(std::size_t i=0;i<60;++i) require(encode(application.D_800B0598[i])==full_roll[i],"exhausted makePhoto changed canonical roll bytes");
                require(isle_player_photo_func_8009BC80(&application,59)==&application.D_800B0598[59]&&
                    isle_player_photo_func_8009BC80(&application,60)==application.D_800B0598+60&&
                    isle_player_photo_func_8009BC80(&application,61)==nullptr,
                    "original full-roll inclusive-count query semantics changed");
                // Source BC80 permits the count index, even count60's one-past
                // pointer. Observe that identity only; NEVER dereference it.
                controller->retire();controller.reset();photo.reset();
                require(application.gPhotoCount==60,"second course photo retirement reset the full application roll");
                // Explicit actual application reset, never an owner destructor.
                isle_player_photo_func_8009CE00(&application);queries=RollQueries{};queries.verify(application);
                for(const auto& record:application.D_800B0598) require(record.levelID==-1&&record.unk_00_24==0,"original application reset omitted a roll record");
                for(const auto id:application.D_800BDF20) require(id==-1,"original application reset retained distinct species");
                photo=std::make_unique<player::Photo>(application,source,actors,items,detection,photo_resources,world,0);
                controller=std::make_unique<actor::PhotoScene>(services,actors,root,camera,detection,&application);
                require(services.set_update(controller->copy_info_object(),player_photo_rsp_copy_info,&copy_witness),"post-reset C CopyInfo installation failed");tickets.photo=photo.get();
                const auto reset_ticket=successful(readbacks[0]);tickets.retire(reset_ticket);save(output/"photo-after-application-reset.bin",encode(application.D_800B0598[0]));
                require(application.gPhotoCount==1,"actual next GPU/source capture failed after application reset");
                require(tickets.empty()&&photo->quiescent(),"final photo ticket retirement incomplete");
                controller->retire();controller.reset();photo.reset();
                require(primary->retire()&&primary->quiescent(),"actual final photo Frame retirement failed");
                item_producer.retire();actor_producer.retire();world_producer.retire();
                const auto actors_retired=actors.reset();
                const auto items_retired=items.retire();
                const auto actors_collected=actors.collect_joined();
                const auto items_collected=items.collect_joined();
                accepted(actors_retired);accepted(items_retired);accepted(actors_collected);accepted(items_collected);
                world.retire();
                accepted(actors.set_player(nullptr,nullptr,nullptr));
                require(environment.set_listener(nullptr,nullptr),"environment listener release failed");
                services.erase(trigger);services.erase(root);services.erase(player_object);services.erase(camera_object);
                require(!services.failed(),services.error());
            }
            environment.cleanup();source_lifetime.finish();
        }
        native(storage.destroy());
    }
    require(session.stop()==audio::Error::none,"actual photo audio stop failed");
    for(unsigned n=0;n<240&&session.state()!=game_audio::Session::State::draining;++n) pump();
    require(session.state()==game_audio::Session::State::draining&&session.service().voices_retired(),"actual photo audio voices did not retire");
    require(session.poll_drain()==audio::Error::none&&session.poll_drain()==audio::Error::none&&session.state()==game_audio::Session::State::drained,
        "actual photo owned PCM drain failed");
    std::ofstream report(output/"photo-proof.txt");require(bool(report),"photo proof report open failed");
    report<<"component=actual-source-photo-World-Actor-Item-FX\n"
        <<"gpu_frames="<<gpu_frames<<"\nfailed_completed_memory_frames="<<failed_frames<<"\noriginal_C_OM_CopyInfo_updates="<<copy_updates<<'\n'
        <<"roll=60_actual_GPU_captures_then_exhaustion_then_application_reset_then_one_actual_capture\n"
        <<"selection=original_actual_roll_flag_set_clear_restore_then_CE00_clears_it\n"
        <<"roll_query=original_BC80_inclusive_count_including_count60_one_past_pointer_never_dereferenced\n"
        <<"queue=two_concrete_Frames_one_native_Game_serial_GPU_first_head_rejection_then_matching_order\n"
        <<"failure_cases=throw_after_GPU_before_postprocess,throw_after_GPU_after_postprocess_and_finish\n"
        <<"failed_retirement=wrong_Frame_token_generation_context_nonquiescent_rejected_exact_releases_without_publication\n"
        <<"lifetime=retained_completed_preflight_rejection_release_next_actual_GPU_Actor_semantic_native_join_holds\n"
        <<"wire=60_big_endian_0x3a0_PhotoData_records_plus_post_application_reset_record\n"
        <<"artifacts=photo-full.ppm,photo-full.rgba8,photo-full.rgba16be,photo-full.depth16be,photo-00.bin..photo-59.bin,photo-after-application-reset.bin\n"
        <<"failure_artifacts=failed-before-postprocess_and_failed-after-postprocess_each_rgba16be_depth16be_unpublished-photo.bin\n"
        <<"RGBA8=320x240_tightly_packed_RGBA;RGBA16_depth16=320x240_big_endian_halfwords\n"
        <<"limits=one_Beach_World_fixed_legacy55_camera_API_Player_and_trigger_two_Actor_slots_eight_Item_slots\n"
        <<"not_claimed=full_Player_input_gameplay_all_courses_Original_capture_Windows11_or_physical_devices\n";
    require(bool(report),"photo proof report write failed");
    std::cout<<"actual_source_photo_GPU_ticket_roll_failed_retirement=passed gpu_frames="<<gpu_frames
        <<" actual_failed_completed_callbacks="<<failed_frames<<" original_C_OM_CopyInfo_updates="<<copy_updates
        <<" full_Player_game_Original_Windows11=not_claimed\n";
}
}
int main(int argc,char** argv) {
    try {
        require(argc==3,"usage: player-photo-rsp-smoke PRIVATE_ROM NEW_OUTPUT_DIRECTORY");
        const std::filesystem::path output=argv[2];
        require(!output.empty()&&!std::filesystem::exists(output),"photo output root must be new; failed roots are never reopened");
        require(std::filesystem::create_directories(output),"new explicit photo output root creation failed");
        auto snapshot=rom::read_snapshot(argv[1]);run(snapshot,output);return 0;
    } catch(const std::exception& error) { std::cerr<<"player-photo-rsp-smoke: "<<error.what()<<'\n';return 1; }
}
