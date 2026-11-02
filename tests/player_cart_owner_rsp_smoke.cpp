// Opt-in admitted-ROM C/native/compiled-pass/RSP/RDP component consumer.
// Not a CTest, original capture, full course continuation or game/device proof.
#include "player_cart.hpp"
#include "player_cart_producer.hpp"
#include "player_cart_owner_rsp_fixture.h"
#include "player_camera_animation.hpp"
#include "player_camera_viewport.h"
#include "beach_world_resources.hpp"
#include "course_resources.hpp"
#include "photo_lighting.hpp"
#include "actual_audio_driver.hpp"
#include "actual_player_source.hpp"
#include "scene_graphics.hpp"
#include "scene_storage.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <vector>

namespace {
using namespace isle;
namespace c=player::cart;
namespace a=menu::animation;
namespace g=scene::graphics;
constexpr std::size_t plane=320*240*2;
constexpr c::ProducerLayout layout{{0x400000,0x10000},{0x410000,0x10000},{0x420000,0x10000},
    {{{0x3b5000,plane},{0x3da800,plane}}}};
constexpr g::Layout frame_layout{{0x626000,0x2000},{0x630000,0x10000},{0x640000,0x40000},0x600000};
constexpr menu::RamRange guard{0x7f0000,256};
constexpr menu::RamRange fragments{0x430000,0x10000};
void require(bool value,const char* message) { if (!value) throw std::runtime_error(message); }
void native(IsleGameResult value) { require(value==ISLE_GAME_OK,"Cart consumer native operation failed"); }
void accepted(c::OwnerStatus value) { require(bool(value),value.detail); }
void accepted(player::camera_animation::Status value) { require(bool(value),"Cart source camera animation failed"); }
std::uint32_t physical(std::uint32_t value) { return value&0x1fffffffu; }
bool same(IsleGameVec3 x,menu::Vec3 y) { return x.x==y.x && x.y==y.y && x.z==y.z; }
bool equal(const a::Clock& x,const a::Clock& y) {
    return x.phase==y.phase && x.time_left==y.time_left && x.time_passed==y.time_passed && x.speed==y.speed;
}
bool different(const a::Transform& x,const a::Transform& y) {
    return x.position.x!=y.position.x || x.position.y!=y.position.y || x.position.z!=y.position.z ||
        x.rotation.x!=y.rotation.x || x.rotation.y!=y.rotation.y || x.rotation.z!=y.rotation.z ||
        x.scale.x!=y.scale.x || x.scale.y!=y.scale.y || x.scale.z!=y.scale.z;
}
IsleGameCamera* source_camera(scene::Services& services) {
    auto* object=services.object(1,0); require(object,"Cart source camera allocation failed");
    IsleGameCamera* camera=nullptr; native(isle_game_attach_camera(services.game,object,&camera));
    // Exact own player.c2936-2986 initialization. Aspect/scale are inherited
    // from the actual fresh native camera, not diagnostic projection tuning.
    const IsleGameCameraParameters* parameters=nullptr;
    native(isle_game_camera_parameters(services.game,camera,&parameters));
    auto perspective=parameters->perspective; perspective.fovy=60; perspective.near_plane=10; perspective.far_plane=25600;
    native(isle_game_camera_set_perspective(services.game,camera,&perspective));
    const IsleGameLookAt look{{0,1600,60},{0,1600,0},{0,1,0}};
    native(isle_game_camera_set_look_at(services.game,camera,&look));
    native(isle_player_camera_set_viewport_rect(services.game,camera,14,12,304,232));
    native(isle_game_camera_add_matrix(services.game,camera,ISLE_GAME_CAMERA_PERSPECTIVE_FAST));
    native(isle_game_camera_add_matrix(services.game,camera,ISLE_GAME_CAMERA_LOOK_AT_REFLECT_ROLL));
    return camera;
}
void publication(scene::Services& services,c::Owner& owner,const c::InitResult& cart) {
    const auto state=owner.animation().model(cart.instance);
    const IsleGameModelNode* nodes=nullptr; std::size_t count=0;
    native(isle_game_model_peek_nodes(services.game,owner.native_model(cart.instance),&nodes,&count));
    require(bool(state) && count==43 && state.nodes.size()==count,"Cart complete native pose extent changed");
    for (std::size_t n=0;n<count;++n) {
        require(same(nodes[n].transform.position,state.nodes[n].transform.position) &&
            same(nodes[n].transform.rotation,state.nodes[n].transform.rotation) && same(nodes[n].transform.scale,state.nodes[n].transform.scale) &&
            nodes[n].flags==state.nodes[n].flags,"Cart Runtime/native node publication diverged");
    }
}
struct OwnerScope {
    c::Owner& owner;
    ~OwnerScope() { owner.retire(); if (!owner.collect_joined()) std::terminate(); }
};
struct Capture {
    std::vector<std::uint32_t> triangles;
    std::vector<std::byte> pixels,depths;
    bool operator==(const Capture&) const=default;
};
struct Readback {
    Capture* result=nullptr; unsigned calls=0;
    static void run(void* user,rsp::CanonicalExecutor::MemoryLease& lease,std::uint32_t color) {
        auto& self=*static_cast<Readback*>(user); require(self.result && ++self.calls==1,"Cart completed mapping count changed");
        const auto& packets=lease.packets();
        for (const auto packet:packets.packets()) {
            const auto words=packets.words().subspan(packet.offset,packet.count);
            const auto opcode=(words[0]>>24)&63;
            if (opcode>=8 && opcode<=15) self.result->triangles.insert(self.result->triangles.end(),words.begin(),words.end());
        }
        const auto bytes=lease.rdram(); self.result->pixels.resize(plane); self.result->depths.resize(plane);
        for (std::size_t i=0;i<plane;++i) {
            self.result->pixels[i]=bytes[(color+i)^3]; self.result->depths[i]=bytes[(frame_layout.depth+i)^3];
        }
        const auto words=std::span(reinterpret_cast<const std::uint32_t*>(bytes.data()),bytes.size()/4);
        for (std::size_t i=0;i<guard.size/4;++i) require(words[guard.offset/4+i]==0x13579bdfu,"Cart task crossed disjoint guard");
        require(!lease.postprocess_active(),"Cart readback retained postprocessing");
    }
};
struct Prepare {
    c::Producer& producer; const photo::LightingInputs& lighting; IsleGame* game; IsleGameCamera* camera;
    const std::uint32_t* mapping=nullptr;
    std::uint64_t epoch=0;
    bool compose_fragments=false;
    c::Producer* reject_filtered=nullptr;
    std::span<std::uint32_t> rejection_snapshot{};
    std::array<IsleGameObject*,3> object_order{};
    std::size_t object_count=0;
    static bool run(void* user,std::span<std::uint32_t> words,std::uint64_t epoch,const scene::render::Pass& pass,scene::render::Prepared& out) {
        auto& self=*static_cast<Prepare*>(user);
        self.lighting.install(std::as_writable_bytes(words));
        const IsleGameCameraParameters* parameters=nullptr; native(isle_game_camera_parameters(self.game,self.camera,&parameters));
        const auto& eye=parameters->look_at.eye; const auto& at=parameters->look_at.at;
        self.lighting.update_direction(std::as_writable_bytes(words),{eye[0],eye[1],eye[2]},{at[0],at[1],at[2]});
        std::fill_n(words.begin()+guard.offset/4,guard.size/4,0x13579bdfu);
        if (self.reject_filtered) {
            require(self.rejection_snapshot.size()==words.size(),"Cart rejection snapshot was not prepared");
            std::copy(words.begin(),words.end(),self.rejection_snapshot.begin());
            auto filtered=pass; filtered.tag_match=scene::render::TagMatch::equality; filtered.camera_tag=0;
            bool rejected=false;
            try { static_cast<void>(self.reject_filtered->prepare(words,epoch,filtered)); }
            catch (const std::invalid_argument&) { rejected=true; }
            require(rejected && std::equal(words.begin(),words.end(),self.rejection_snapshot.begin()),
                "Hidden/pass-filtered invalid Cart binding escaped preflight or mutated canonical RAM");
            self.reject_filtered=nullptr;
        }
        const auto prepared=self.producer.prepare(words,epoch,pass);
        self.mapping=words.data(); self.epoch=epoch; self.object_count=prepared.objects.size();
        require(self.object_count<=self.object_order.size(),"Cart returned too many physical objects");
        for (std::size_t i=0;i<self.object_count;++i) self.object_order[i]=prepared.objects[i].object;
        out.camera={prepared.camera.address,static_cast<std::size_t>(prepared.camera.commands)*2};
        std::size_t cursor=fragments.offset/4;
        for (unsigned lane=0;lane<4;++lane) {
            if (!self.compose_fragments) {
                out.lanes[lane]={prepared.lanes[lane].address,static_cast<std::size_t>(prepared.lanes[lane].commands)*2};
                continue;
            }
            const auto begin=cursor;
            for (const auto& object:prepared.objects) {
                const auto list=object.lanes[lane]; const auto count=std::size_t(list.commands)*2;
                require(cursor<=(fragments.offset+fragments.size)/4 &&
                    count<=(fragments.offset+fragments.size)/4-cursor,"Cart fragment composition exceeds its real workspace");
                std::copy_n(words.begin()+list.address/4,count,words.begin()+cursor); cursor+=count;
            }
            require(cursor+2<=(fragments.offset+fragments.size)/4,"Cart fragment EndDL exceeds its real workspace");
            words[cursor++]=0xdf000000u; words[cursor++]=0;
            out.lanes[lane]={static_cast<std::uint32_t>(begin*4),cursor-begin};
        }
        return true;
    }
};
std::size_t color_changes(const Capture& x,const Capture& y) {
    require(x.pixels.size()==plane && y.pixels.size()==plane,"Cart completed RGBA16 extent changed");
    std::size_t count=0;
    for (std::size_t i=0;i<plane;i+=2)
        count+=((std::to_integer<unsigned>(x.pixels[i]^y.pixels[i])<<8)|
            (std::to_integer<unsigned>(x.pixels[i+1]^y.pixels[i+1])&0xfeu))!=0;
    return count;
}
void save(const std::filesystem::path& path,std::span<const std::byte> bytes) {
    std::ofstream file(path,std::ios::binary); require(bool(file),"Cart capture open failed");
    file.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));
    require(bool(file),"Cart capture write failed");
}
void save_image(const std::filesystem::path& output,const char* name,g::Completed image) {
    require(bool(image) && image.rgba.size()==320*240*4,"Cart positive RGBA8 mapping missing");
    save(output/(std::string(name)+".rgba8"),image.rgba);
    std::ofstream file(output/(std::string(name)+".ppm"),std::ios::binary); require(bool(file),"Cart PPM open failed");
    file<<"P6\n320 240\n255\n";
    for (std::size_t i=0;i<image.rgba.size();i+=4) file.write(reinterpret_cast<const char*>(image.rgba.data()+i),3);
    require(bool(file),"Cart PPM write failed");
}
void boundaries(scene::Services& services,c::Owner& owner,isle_arena& arena,const player::CartResources& resources) {
    // All four actual original C functions are reached through their C guards.
    const auto nan=std::numeric_limits<float>::quiet_NaN();
    accepted(owner.set_animation(owner.intro_model(),owner.intro_material(),nan,nan)); // no current: original early return
    accepted(owner.set_animation_speed(nan));
    PlayerCartRspInit creation{}; creation.source=owner.source();
    auto* helper=services.object(99,0); require(helper,"Cart C factory test process owner failed");
    require(services.process(helper,player_cart_rsp_init,&creation,ISLE_GAME_SYNCHRONOUS,0)!=nullptr,
        "Cart C factory test process allocation failed");
    const auto allocation=arena.cursor;
    native(isle_game_tick(services.game));
    require(creation.entered==1 && creation.returned==1 && creation.status==ISLE_GAME_OK && creation.object && arena.cursor==allocation,
        "Prepared original C Init allocated during OM or failed its concrete publication");
    services.erase(helper);
    const c::InitResult first{{},creation.object,owner.instance(creation.object)}; require(bool(first),first.status.detail);
    PlayerCartRspTopology topology{}; player_cart_rsp_topology(services.game,first.object,&topology);
    require(!topology.invalid && topology.nodes==43 && topology.processes==1,"Cart real C factory identity/cohort changed");
    scene::RenderMetadata render{}; require(services.render_metadata(first.object,render) && render.link==3 &&
        render.priority==0x80000000u && render.camera_tag==UINT32_MAX,"Cart native J-fogged DL metadata changed");
    accepted(owner.change_animation(owner.intro_model(),owner.intro_material(),0));
    native(isle_game_tick(services.game));
    auto state=owner.animation().model(first.instance);
    const auto attachment=state.nodes[7].first_material;
    const auto color0=state.materials[attachment].color_and_trailing_words[0];
    native(isle_game_tick(services.game)); state=owner.animation().model(first.instance);
    require(state.materials[attachment].clock.time_passed==1 &&
        state.materials[attachment].color_and_trailing_words[0]!=color0,
        "Fresh material speed1 did not reach the real one-frame interpolated source color");
    accepted(owner.change_animation(owner.intro_model(),owner.intro_material(),60));
    native(isle_game_tick(services.game)); publication(services,owner,first);
    const auto root60=owner.animation().model(first.instance).nodes[0].transform;
    native(isle_game_tick(services.game)); const auto root61=owner.animation().model(first.instance).nodes[0].transform;
    require(different(root60,root61) && owner.animation().model(first.instance).animation_time==61,
        "Fresh default1 did not advance actual source transforms by one frame");
    // A separate pure Runtime consumes the admitted programs as a lower-level
    // control. Compare consumed native poses/materials after real C ticks, not
    // immediate setter echoes or a second Cart control implementation.
    const auto capacity=a::slot_capacity(resources.animation_resources().models[0]);
    const std::array control_capacity{capacity};
    auto control=a::Runtime::create(resources.animation_resources(),control_capacity,{},{});
    require(bool(control),"Mixed-domain control Runtime admission failed");
    const auto control_cart=control.runtime->acquire(0,0,false);
    require(bool(control_cart) && bool(control.runtime->set_model_speed(control_cart.handle,1)) &&
        bool(control.runtime->set_texture_speed(control_cart.handle,1)),"Mixed-domain control acquisition failed");
    for (const float start:{0.0f,60.0f}) {
        require(bool(control.runtime->bind_model(control_cart.handle,resources.intro().model_bindings,start)) &&
            bool(control.runtime->bind_materials(control_cart.handle,resources.intro().material_bindings,start)) &&
            bool(control.runtime->step_model(control_cart.handle)) &&
            bool(control.runtime->step_model(control_cart.handle)),"Mixed-domain control trajectory failed");
    }
    const auto consume_mixed=[&] {
        for (unsigned tick=0;tick<3;++tick) {
            native(isle_game_tick(services.game));
            require(bool(control.runtime->step_model(control_cart.handle)),"Mixed-domain control step failed");
            const auto expected=control.runtime->model(control_cart.handle);
            const auto actual=owner.animation().model(first.instance);
            const IsleGameModelNode* nodes=nullptr; std::size_t count=0;
            native(isle_game_model_peek_nodes(services.game,owner.native_model(first.instance),&nodes,&count));
            require(count==expected.nodes.size() && actual.materials.size()==expected.materials.size(),
                "Mixed-domain consumed model/material extent changed");
            for (std::size_t n=0;n<count;++n)
                require(same(nodes[n].transform.position,expected.nodes[n].transform.position) &&
                    same(nodes[n].transform.rotation,expected.nodes[n].transform.rotation) &&
                    same(nodes[n].transform.scale,expected.nodes[n].transform.scale) && nodes[n].flags==expected.nodes[n].flags,
                    "Outer NULL damaged the retained or rebound source native trajectory");
            for (std::size_t m=0;m<actual.materials.size();++m)
                require(actual.materials[m].color_and_trailing_words==expected.materials[m].color_and_trailing_words &&
                    equal(actual.materials[m].clock,expected.materials[m].clock),
                    "Outer NULL damaged consumed material interpolation or its retained speed");
        }
    };
    accepted(owner.set_animation(nullptr,owner.outro_material(),1,0.25f));
    require(bool(control.runtime->bind_materials(control_cart.handle,resources.outro().material_bindings,1)) &&
        bool(control.runtime->set_texture_speed(control_cart.handle,0.25f)),"Material-only control binding failed");
    consume_mixed();
    accepted(owner.set_animation(owner.outro_model(),nullptr,4,0.75f));
    require(bool(control.runtime->bind_model(control_cart.handle,resources.outro().model_bindings,4)) &&
        bool(control.runtime->set_model_speed(control_cart.handle,0.75f)),"Model-only control binding failed");
    consume_mixed();
    state=owner.animation().model(first.instance);
    const auto model_clock=state.nodes[0].clock,material_clock=state.materials[0].clock;
    accepted(owner.set_animation(nullptr,nullptr,nan,nan)); accepted(owner.change_animation(nullptr,nullptr,nan));
    accepted(owner.set_animation(owner.intro_model(),owner.intro_material(),-1,nan));
    accepted(owner.set_animation(owner.intro_model(),owner.intro_material(),0,-1)); accepted(owner.set_animation_speed(-1));
    state=owner.animation().model(first.instance);
    require(equal(state.nodes[0].clock,model_clock) && equal(state.materials[0].clock,material_clock),
        "Original early-return/NULL behavior mutated an unrequested domain");
    accepted(owner.change_animation(owner.intro_model(),owner.outro_material(),30));
    require(bool(control.runtime->bind_model(control_cart.handle,resources.intro().model_bindings,30)) &&
        bool(control.runtime->bind_materials(control_cart.handle,resources.outro().material_bindings,30)),
        "Independent-speed ChangeAnimation control binding failed");
    consume_mixed();
    control.runtime->retire();
    accepted(owner.change_animation(owner.intro_model(),owner.outro_material(),30));
    accepted(owner.set_animation_speed(0.5f));
    // Real object flag2 advances source tracks, but suppresses native pose writes.
    native(isle_game_tick(services.game)); const auto frozen=owner.animation().model(first.instance).nodes[0].transform;
    native(isle_game_object_set_flags(services.game,first.object,2)); native(isle_game_tick(services.game));
    state=owner.animation().model(first.instance);
    require(!different(frozen,state.nodes[0].transform) && state.animation_time==30.5f,
        "GObj flag2 was confused with source node controls or stopped the clock");
    publication(services,owner,first); native(isle_game_object_set_flags(services.game,first.object,0));
    PlayerCartRspEvents first_events{}; first_events.player=owner.source()->player; first_events.expected=first.object;
    accepted(owner.set_callback(first.object,player_cart_rsp_callback,&first_events));
    accepted(owner.set_animation(owner.intro_model(),owner.intro_material(),149.5f,0.5f)); native(isle_game_tick(services.game));
    const auto second=owner.init(); require(bool(second) && second.object!=first.object && second.instance!=first.instance,"Init rebound a singleton instead of making NEW cart");
    PlayerCartRspEvents second_events{}; second_events.player=owner.source()->player; second_events.expected=second.object;
    accepted(owner.set_callback(second.object,player_cart_rsp_callback,&second_events));
    accepted(owner.set_animation(owner.outro_model(),owner.outro_material(),149,1));
    native(isle_game_tick(services.game));
    require(first_events.count==1 && first_events.ids[0]==-1 && !first_events.invalid && second_events.count==0 &&
        owner.source()->cartGObj==second.object,"A callback followed current B instead of explicit object A");
    native(isle_game_tick(services.game));
    require(second_events.count==1 && second_events.ids[0]==-1 && !second_events.invalid && first_events.count==1,
        "Actual independent C END callback sequence duplicated or disappeared");
    require(owner.animation().model(first.instance).nodes[0].flags==2 && owner.animation().model(second.instance).nodes[0].flags==0,
        "Fresh outro inherited intro terminal controls");
    accepted(owner.set_callback(first.object,nullptr,nullptr)); accepted(owner.set_callback(second.object,nullptr,nullptr));
    // Real source FIFO retention and default update: no gameplay fake receiver.
    require(services.send_command(second.object,123,first.object) && services.source_owner_borrows(first.object).queued==1,
        "Cart default FIFO failed to retain actual sender identity");
    accepted(owner.retire(first.object));
    require(!owner.collect_joined() && owner.membership(first.instance).joined &&
        services.source_owner_borrows(first.object).queued==1,
        "Queued sender did not retain a natively joined Cart identity until actual FIFO delivery");
    native(isle_game_tick(services.game)); require(services.source_owner_borrows(first.object).queued==0,
        "Cart did not consume its actual default-update FIFO");
    const auto third=owner.init(); require(bool(third),third.status.detail);
    IsleGameCounts before{},after{}; native(isle_game_counts(services.game,&before));
    const auto unavailable=owner.init(); native(isle_game_counts(services.game,&after));
    require(unavailable.status.error==c::OwnerError::capacity && !unavailable.object && owner.source()->cartGObj==third.object &&
        before.objects==after.objects && before.models==after.models && before.model_nodes==after.model_nodes && before.processes==after.processes &&
        owner.source()->player->status==ISLE_GAME_OK,"Prepared exhaustion changed publication/current/counts or source fault");
    auto caller=owner.retain(first.object); require(bool(caller),"Cart actual caller hold failed");
    accepted(owner.retire(first.object)); require(!owner.collect_joined() && !owner.instance(first.object) &&
        owner.membership(first.instance).joined,"Cart caller-held retired identity became reusable or live");
    caller={}; accepted(owner.collect_joined()); accepted(owner.refill_prepared());
    const auto replacement=owner.init(); require(bool(replacement) && replacement.instance!=first.instance,
        "Joined released physical Cart did not acquire a fresh Runtime incarnation");
    accepted(owner.retire(second.object)); accepted(owner.retire(third.object)); accepted(owner.collect_joined());
    accepted(owner.refill_prepared());
    // Keep only replacement live for subsequent real GPU observations. Its
    // callback is NULL, exactly source's fresh default, not Actor loop policy.
}
// The decoder orders only reachable commands, including camera jump fragments.
// These finite Rainbow streams use additive waits, not Texture22's timer reset.
// Derive half-speed update bounds from those waits; do not borrow Beach timing
// or the lifecycle's separately bounded intro-wait/camera-restore processes.
bool source_wait(std::uint8_t opcode) {
    return opcode==2 || opcode==3 || opcode==5 || opcode==8 || opcode==10 ||
        opcode==15 || opcode==16 || opcode==17 || opcode==18 || opcode==20 || opcode==23;
}
unsigned source_ticks(const menu::Animation& program) {
    require(!program.commands.empty() && program.commands.back().opcode==0,
        "Rainbow Cart stream lacks a finite real END");
    unsigned ticks=0;
    for (const auto& command:program.commands) {
        require(command.opcode!=22,"Rainbow Cart timing requires an unadmitted timer reset");
        if (source_wait(command.opcode)) {
            require(command.duration<=(std::numeric_limits<unsigned>::max()-2-ticks)/2,"Rainbow Cart source duration overflow");
            ticks+=2u*command.duration;
        }
    }
    return ticks;
}
struct RainbowSchedule {
    std::array<unsigned,43> models{};
    std::array<unsigned,8> materials{};
    unsigned camera=0,limit=0,sample=0;
};
RainbowSchedule rainbow_schedule(const player::CartResources& resources,bool intro) {
    const auto& clip=intro?resources.intro():resources.outro();
    const auto animation=resources.animation_resources();
    require(resources.course_id()==6 &&
        clip.model_animation_table.guest_address==(intro?0x80119ae0u:0x801183e0u) &&
        clip.material_animation_table.guest_address==(intro?0x8011a970u:0x80119050u) &&
        clip.camera_identity==(intro?0x8011b3e0u:0x80119a8cu),
        "Rainbow Cart selected another course's model/material/camera tuple");
    require(clip.model_bindings.size()==43 && clip.material_bindings.size()==8,
        "Rainbow Cart lost its complete model/material attachment interfaces");
    RainbowSchedule result;
    const auto duration=[&](std::size_t index,menu::AnimationKind kind) {
        require(index<animation.animations.size() && animation.animations[index].kind==kind,
            "Rainbow Cart binding did not resolve a real typed stream");
        const auto& program=animation.animations[index];
        const auto ticks=source_ticks(program); result.limit=std::max(result.limit,ticks); return ticks;
    };
    for (std::size_t n=0;n<result.models.size();++n) {
        const auto index=clip.model_bindings[n];
        result.models[n]=duration(index,menu::AnimationKind::model);
    }
    for (std::size_t m=0;m<result.materials.size();++m)
        if (clip.material_bindings[m]!=menu::no_index) {
            result.materials[m]=duration(clip.material_bindings[m],menu::AnimationKind::texture);
        }
    result.camera=duration(clip.camera_animation,menu::AnimationKind::camera);
    unsigned cursor=0,visible_begin=0,visible_end=result.models[0];
    for (const auto& command:animation.animations[clip.model_bindings[0]].commands) {
        if (command.opcode==15) {
            if (!(command.mask&2u)) visible_begin=cursor;
            else if (cursor>visible_begin) { visible_end=cursor; break; }
        }
        if (source_wait(command.opcode)) cursor+=2u*command.duration;
    }
    require(visible_begin<visible_end,"Rainbow Cart has no source-visible root interval");
    result.sample=visible_begin+(visible_end-visible_begin)/2;
    require(result.sample<result.models[0] && result.sample<result.camera,
        "Rainbow Cart interior sample reached a source terminal");
    return result;
}
bool different(const a::MaterialState& x,const a::MaterialState& y) {
    return x.image_index!=y.image_index || x.next_image_index!=y.next_image_index ||
        x.palette_index!=y.palette_index || x.lod_level!=y.lod_level || x.parameters!=y.parameters ||
        x.color_and_trailing_words!=y.color_and_trailing_words || x.word_4c!=y.word_4c;
}
bool different(const a::CameraState& x,const a::CameraState& y) {
    return x.eye.x!=y.eye.x || x.eye.y!=y.eye.y || x.eye.z!=y.eye.z ||
        x.at.x!=y.at.x || x.at.y!=y.at.y || x.at.z!=y.at.z || x.roll!=y.roll || x.fov_y!=y.fov_y;
}
void gpu(const rom::Snapshot& snapshot,scene::Services& services,c::Owner& owner,const player::CartResources& resources,
    IsleGameCamera* camera,const photo::LightingInputs& lighting,const beach::WorldMetadata& world,const std::filesystem::path& output,
    bool rainbow=false) {
    std::array<scene::RenderOrderEntry,8> order{};
    const auto bindings=owner.render_bindings(order);
    auto created=g::Frame::create(snapshot,{true,{16000000,4*1024*1024},frame_layout}); require(bool(created),created.message.data());
    std::vector<menu::RamRange> mutable_ranges{layout.payloads,layout.lists,layout.materials,guard,fragments};
    for (const auto& span:lighting.installation_spans()) mutable_ranges.push_back({physical(span.guest_address),span.bytes.size()});
    std::vector<menu::RamRange> other{guard,fragments};
    for (const auto& span:lighting.installation_spans()) other.push_back({physical(span.guest_address),span.bytes.size()});
    for (const auto range:created.frame->reservations()) if (range.address) other.push_back({range.address,range.size});
    const auto capacity=a::slot_capacity(resources.animation_resources().models[0]); const std::array capacities{capacity,capacity,capacity};
    c::Producer producer(services.game,camera,services.camera_basis(),resources,capacities,bindings,layout,other);
    auto camera_made=player::camera_animation::NativeDriver::create(services.game,camera,resources.animation_resources(),{});
    require(bool(camera_made),"Cart actual source camera driver admission failed"); auto& driver=*camera_made.driver;
    std::vector<sprite::AssetSpan> immutable;
    for (const auto& span:resources.installation_spans()) immutable.push_back({physical(span.guest_address),
        {reinterpret_cast<const std::uint8_t*>(span.bytes.data()),span.bytes.size()}});
    std::vector<sprite::RamRange> reservations;
    for (const auto range:mutable_ranges) reservations.push_back({static_cast<std::uint32_t>(range.offset),static_cast<std::uint32_t>(range.size)});
    Prepare preparation{producer,lighting,services.game,camera};
    std::array<scene::render::Pass,2> passes{};
    auto& clear=passes[0]; clear.kind=scene::render::Kind::clear; clear.priority=6; clear.flags=3;
    auto& pass=passes[1]; pass.camera=camera; pass.priority=5; pass.flags=5;
    pass.background_rgba=(std::uint32_t(world.background_rgb[0])<<24)|(std::uint32_t(world.background_rgb[1])<<16)|
        (std::uint32_t(world.background_rgb[2])<<8)|255u;
    // A component-only clear pass provides a deterministic control framebuffer;
    // it does not change the original main camera's OR1|4 flag policy.
    clear.background_rgba=pass.background_rgba;
    pass.link_mask=(std::uint64_t(1)<<3)|(std::uint64_t(1)<<4)|(std::uint64_t(1)<<5);
    pass.camera_tag=UINT32_MAX; pass.prepare=Prepare::run; pass.userdata=&preparation;
    scene::render::Input input{services.game,{1,nullptr,immutable,reservations,{0x3b5000,0x3da800},physical(lighting.pre_render_guest())},passes,{}};
    Readback readback; auto frame=std::move(created.frame); // drains before native/producer owners on every path
    const auto draw_on=[&](g::Frame& target,unsigned context=0) {
        require(target.quiescent(),"Cart mutation crossed actual GPU/preparation/readback boundary"); owner.render_bindings(order);
        auto borrow=owner.borrow(); require(bool(borrow),"Cart graphics borrow failed");
        Capture capture; readback.result=&capture; readback.calls=0;
        require(target.begin(input,{context},{&readback,Readback::run}),target.message());
        while (target.poll()==g::Poll::pending) std::this_thread::yield();
        require(target.error()==g::Error::none && target.quiescent() && readback.calls==1,target.message());
        return capture;
    };
    const auto draw=[&](unsigned context=0) { return draw_on(*frame,context); };
    const auto cart=owner.source()->cartGObj; const auto instance=owner.instance(cart); require(bool(instance),"GPU current Cart missing");
    const auto bind=[&](bool intro,float time) {
        accepted(owner.set_animation(intro?owner.intro_model():owner.outro_model(),intro?owner.intro_material():owner.outro_material(),time,0.5f));
        IslePlayerCameraAnimationClip clip{}; accepted(driver.clip((intro?resources.intro():resources.outro()).camera_animation,clip));
        accepted(driver.set_speed(0.5f)); accepted(driver.set(&clip,time));
        native(isle_game_tick(services.game)); accepted(driver.update(0));
    };
    c::InitResult fresh{{},cart,instance};
    if (rainbow) {
        // This course-only flow does not claim Beach's mixed-domain, FIFO,
        // multi-object priority or workspace-handoff boundary scenarios.
        for (const bool is_intro:{true,false}) {
            if (!is_intro) {
                const auto previous=fresh;
                fresh=owner.init(); require(bool(fresh),fresh.status.detail);
                accepted(owner.retire(previous.object)); accepted(owner.collect_joined());
            }
            const auto schedule=rainbow_schedule(resources,is_intro);
            const auto& clip=is_intro?resources.intro():resources.outro();
            PlayerCartRspEvents events{}; events.player=owner.source()->player; events.expected=fresh.object;
            accepted(owner.set_callback(fresh.object,player_cart_rsp_callback,&events));
            accepted(owner.set_animation(is_intro?owner.intro_model():owner.outro_model(),
                is_intro?owner.intro_material():owner.outro_material(),0,0.5f));
            IslePlayerCameraAnimationClip camera_clip{}; accepted(driver.clip(clip.camera_animation,camera_clip));
            accepted(driver.set_speed(0.5f)); accepted(driver.set(&camera_clip,0));
            std::array<a::Transform,43> initial_pose{},terminal_pose{};
            std::array<a::MaterialState,8> initial_material{},terminal_material{};
            a::CameraState initial_camera{},terminal_camera{};
            bool model_advanced=false,material_advanced=false,camera_advanced=false,rendered=false;
            for (unsigned tick=0;tick<=schedule.limit+1;++tick) {
                native(isle_game_tick(services.game)); accepted(driver.update(0)); publication(services,owner,fresh);
                const auto state=owner.animation().model(fresh.instance);
                const auto& view=driver.state();
                require(state.nodes.size()==initial_pose.size() && state.materials.size()==initial_material.size(),
                    "Rainbow Cart consumed incomplete pose/material state");
                for (std::size_t n=0;n<state.nodes.size();++n) {
                    require(state.nodes[n].clock.phase==(tick<schedule.models[n]?a::Phase::running:a::Phase::disabled),
                        "Rainbow Cart model did not terminate at its own source END");
                    if (!tick) initial_pose[n]=state.nodes[n].transform;
                    else model_advanced|=different(initial_pose[n],state.nodes[n].transform);
                    if (tick==schedule.limit) terminal_pose[n]=state.nodes[n].transform;
                    if (tick>schedule.limit) require(!different(terminal_pose[n],state.nodes[n].transform),
                        "Rainbow Cart disabled model continued moving");
                }
                for (std::size_t m=0;m<state.materials.size();++m) {
                    if (clip.material_bindings[m]!=menu::no_index)
                        require(state.materials[m].clock.phase==(tick<schedule.materials[m]?a::Phase::running:a::Phase::disabled),
                            "Rainbow Cart material did not terminate at its own source END");
                    if (!tick) initial_material[m]=state.materials[m];
                    else material_advanced|=different(initial_material[m],state.materials[m]);
                    if (tick==schedule.limit) terminal_material[m]=state.materials[m];
                    if (tick>schedule.limit) require(!different(terminal_material[m],state.materials[m]),
                        "Rainbow Cart disabled material continued changing");
                }
                require(view.clock.phase==(tick<schedule.camera?a::Phase::running:a::Phase::disabled),
                    "Rainbow Cart camera did not terminate at its own source END");
                if (!tick) initial_camera=view;
                else camera_advanced|=different(initial_camera,view);
                if (tick==schedule.limit) terminal_camera=view;
                if (tick>schedule.limit) require(!different(terminal_camera,view) && view.written_channels==0,
                    "Rainbow Cart disabled camera continued publishing");
                const auto ended=tick>=schedule.models[0]?1u:0u;
                require(!events.invalid && events.count==ended && events.entered==ended && events.returned==ended &&
                    (!ended || (events.ids[0]==-1 && events.arguments[0]==0 && events.nodes[0].node==0)),
                    "Rainbow Cart real C END receiver/return fence changed");
                if (tick==schedule.sample) {
                    require(!(state.nodes[0].flags&2u),"Rainbow Cart source interior sample hid its root");
                    native(isle_game_object_set_flags(services.game,fresh.object,1)); const auto hidden=draw();
                    require(hidden.triangles.empty(),"Hidden Rainbow Cart reached real RSP geometry");
                    native(isle_game_object_set_flags(services.game,fresh.object,0)); const auto visible=draw();
                    require(!visible.triangles.empty() && color_changes(visible,hidden)>0 && visible.depths!=hidden.depths,
                        "Rainbow Cart source geometry failed completed RSP/RDP color/depth proof");
                    save_image(output,is_intro?"cart-intro":"cart-outro",frame->completed());
                    require(draw(1)==visible,"Rainbow Cart second real framebuffer changed unchanged source output");
                    rendered=true;
                }
            }
            require(model_advanced && material_advanced && camera_advanced && rendered,
                "Rainbow Cart did not consume advancing real model/material/camera and raster output");
            accepted(owner.set_callback(fresh.object,nullptr,nullptr));
            std::cout<<"Rainbow Cart "<<(is_intro?"intro":"outro")<<" source_half_ticks="<<schedule.limit
                <<" sample_half_tick="<<schedule.sample<<" C_END_callbacks="<<events.count<<'\n';
            // Return through the real C setters and camera updater to the same
            // source-derived visible sample for the held submission below.
            if (!is_intro) bind(false,static_cast<float>(schedule.sample)*0.5f);
        }
    } else {
    // Fixed, source-defined trajectory samples, not a view/scale/pose search.
    // Both cutscene clocks start at the same actual source sample.
    bind(true,90);
    native(isle_game_object_set_flags(services.game,cart,1)); const auto hidden=draw();
    require(hidden.triangles.empty(),"Hidden native Cart reached RSP geometry");
    native(isle_game_object_set_flags(services.game,cart,0)); const auto intro=draw();
    require(!intro.triangles.empty() && color_changes(intro,hidden)>0 && intro.depths!=hidden.depths,
        "Actual source intro Cart J-fogged geometry failed RSP/RDP color/depth proof");
    save_image(output,"cart-intro",frame->completed());
    require(draw(1)==intro,"Second framebuffer did not reproduce source Cart output");
    const auto* mapping_a=preparation.mapping;
    {
        auto independent=g::Frame::create(snapshot,{true,{16000000,4*1024*1024},frame_layout});
        require(bool(independent),independent.message.data());
        require(draw_on(*independent.frame)==intro && preparation.mapping!=mapping_a,
            "Independent canonical RAM lost unchanged Cart matrices/materials/camera residency");
        require(draw()==intro && preparation.mapping==mapping_a,
            "Returning to the first actual RAM mapping retained foreign Cart residency");
        require(independent.frame->retire(),"Independent Cart preparation context did not retire");
    }
    {
        std::array<c::RenderBinding,3> invalid{};
        std::copy(bindings.begin(),bindings.end(),invalid.begin()); invalid[0].model.model=nullptr;
        c::Producer rejecting(services.game,camera,services.camera_basis(),resources,capacities,invalid,layout,other);
        std::vector<std::uint32_t> unchanged(menu::rdram_size/4);
        preparation.reject_filtered=&rejecting; preparation.rejection_snapshot=unchanged;
        native(isle_game_object_set_flags(services.game,cart,1));
        require(draw()==hidden && preparation.reject_filtered==nullptr,
            "Actual hidden/pass-filtered binding rejection failed");
        native(isle_game_object_set_flags(services.game,cart,0));
        preparation.rejection_snapshot={}; rejecting.retire();
    }
    pass.tag_match=scene::render::TagMatch::equality; pass.camera_tag=0; require(draw()==hidden,"Cart pass filter retained GPU geometry");
    pass.tag_match=scene::render::TagMatch::intersection; pass.camera_tag=UINT32_MAX; require(draw()==intro,"Cart reveal retained stale GPU residency");
    bind(true,150); const auto controlled=draw(); require(controlled.triangles.empty(),"Real intro END node control2 did not suppress geometry");
    accepted(owner.change_animation(owner.outro_model(),owner.outro_material(),90)); native(isle_game_tick(services.game));
    require(owner.animation().model(instance).nodes[0].flags==2 && draw().triangles.empty(),"Rebind incorrectly restored source control flags");
    // A genuinely fresh C factory is essential: resetting a retained pose/control
    // is not the original outro lifecycle. There is prepared physical capacity.
    fresh=owner.init(); require(bool(fresh),fresh.status.detail); accepted(owner.retire(cart)); accepted(owner.collect_joined());
    bind(false,90); // source current now identifies only the new fresh object
    const auto outro=draw(); require(!outro.triangles.empty() && color_changes(outro,hidden)>0,
        "Fresh source outro J-fogged Cart did not reach actual RSP/RDP");
    require(owner.animation().model(fresh.instance).nodes[0].flags==0,"Fresh outro source controls were not zero");
    save_image(output,"cart-outro",frame->completed());
    {
        // Two real producers sequentially own the same compiled workspace.
        // Their stable identities describe actual reuse, not diagnostic resets.
        auto* second_camera=source_camera(services);
        auto second_driver=player::camera_animation::NativeDriver::create(services.game,second_camera,resources.animation_resources(),{});
        require(bool(second_driver),"Second source camera driver admission failed");
        IslePlayerCameraAnimationClip clip{}; accepted(second_driver.driver->clip(resources.outro().camera_animation,clip));
        accepted(second_driver.driver->set_speed(0.5f)); accepted(second_driver.driver->set(&clip,91));
        accepted(second_driver.driver->update(0));
        c::Producer second(services.game,second_camera,services.camera_basis(),resources,capacities,bindings,layout,other);
        Prepare alternate{second,lighting,services.game,second_camera};
        const auto epoch_a=preparation.epoch;
        pass.camera=second_camera; pass.userdata=&alternate; input.resources.identity=2;
        const auto other_camera=draw();
        require(alternate.mapping==mapping_a && alternate.epoch>epoch_a && other_camera.triangles!=outro.triangles,
            "Real Cart workspace handoff did not replace the same RAM camera payload");
        pass.camera=camera; pass.userdata=&preparation; input.resources.identity=1;
        require(draw()==outro && preparation.mapping==mapping_a && preparation.epoch>alternate.epoch,
            "Same-RAM epoch renewal failed to restore unchanged source Cart camera/matrix residency");
        second_driver.driver->retire(); second.retire();
    }
    {
        const auto previous=fresh;
        fresh=owner.init(); require(bool(fresh),"Second live Cart source Init failed");
        bind(false,90);
        const auto aggregate=draw();
        require(preparation.object_count==2 && preparation.object_order[0]==previous.object &&
            preparation.object_order[1]==fresh.object,"Cart producer ignored actual equal-priority Services DL3 order");
        preparation.compose_fragments=true;
        require(draw()==aggregate,"Per-object raw Cart fragments changed aggregate RSP/RDP output");
        require(services.set_render(fresh.object,{3,0x80000001u,UINT32_MAX}),"Cart actual DL3 priority reorder failed");
        preparation.compose_fragments=false; const auto reordered=draw();
        require(preparation.object_count==2 && preparation.object_order[0]==fresh.object &&
            preparation.object_order[1]==previous.object && reordered.triangles!=aggregate.triangles,
            "Cart source DL3 reorder retained physical-slot geometry order");
        preparation.compose_fragments=true;
        require(draw()==reordered,"Native-order Cart fragment composition lost object boundaries or zero-length lanes");
        preparation.compose_fragments=false;
        require(services.set_render(fresh.object,{3,0x80000000u,UINT32_MAX}),"Cart source DL3 priority restore failed");
        accepted(owner.retire(previous.object)); accepted(owner.collect_joined());
        bind(false,90);
    }
    }
    // Delay real completion with the submission's actual Owner borrow. Logical
    // retire closes new binding access immediately; it cannot delete/reuse the
    // native model while preparation/GPU/readback still owns this borrow.
    owner.render_bindings(order); auto gpu_hold=owner.borrow(); require(bool(gpu_hold),"Cart delayed task hold failed");
    Capture delayed; readback.result=&delayed; readback.calls=0;
    require(frame->begin(input,{0},{&readback,Readback::run}),frame->message());
    const auto token=frame->submission(); require(token!=0,"Cart real GPU submission token missing");
    accepted(owner.retire(fresh.object)); require(owner.source()->cartGObj==fresh.object && !owner.collect_joined() &&
        !owner.membership(fresh.instance).joined,"GPU borrow failed to retain actual native Cart storage");
    while (frame->poll()==g::Poll::pending) std::this_thread::yield();
    require(frame->error()==g::Error::none && frame->quiescent() && readback.calls==1,frame->message());
    if (rainbow) require(!delayed.triangles.empty() && !owner.collect_joined() && !owner.membership(fresh.instance).joined,
        "Completed Rainbow readback lost geometry or bypassed its retained Owner borrow");
    gpu_hold={}; require(owner.membership(fresh.instance).joined && !owner.collect_joined() && owner.source()->cartGObj==fresh.object,
        "Native join incorrectly cleared source current held identity");
    driver.retire(); producer.retire(); require(frame->retire(),"Cart real GPU consumer retirement failed");
    accepted(owner.retire()); accepted(owner.collect_joined());
}
void callback_failure(scene::Services& services,c::Owner& owner,unsigned scenario,float terminal=150) {
    const auto cart=owner.init(); require(bool(cart),cart.status.detail);
    PlayerCartRspEvents events{}; events.player=owner.source()->player; events.expected=cart.object;
    events.fail=scenario!=3; events.retire=scenario==2; events.reenter=scenario==3; events.reenter_model=owner.intro_model();
    accepted(owner.set_callback(cart.object,player_cart_rsp_callback,&events));
    accepted(owner.set_animation(owner.outro_model(),owner.outro_material(),terminal,1));
    require(isle_game_tick(services.game)!=ISLE_GAME_OK && events.entered==1 && events.returned==0 && events.count==1 && !events.invalid &&
        owner.source()->player->status==(scenario==3?ISLE_GAME_INVALID_STATE:ISLE_GAME_INVALID_ARGUMENT),
        "Actual C callback failure did not preserve first fault or rejected reentrant Runtime mutation");
    // If C finish_events leaked its lease, teardown/collection cannot finish.
    owner.retire(); accepted(owner.collect_joined());
}
void nonfinite_failure(c::Owner& owner) {
    const auto cart=owner.init(); require(bool(cart),cart.status.detail);
    const auto before=owner.animation().model(cart.instance);
    const auto model=before.nodes[0].clock,material=before.materials[0].clock;
    const auto rejected=owner.set_animation(owner.intro_model(),owner.intro_material(),0,std::numeric_limits<float>::infinity());
    const auto after=owner.animation().model(cart.instance);
    require(!rejected && owner.source()->player->status==ISLE_GAME_INVALID_ARGUMENT &&
        equal(model,after.nodes[0].clock) && equal(material,after.materials[0].clock),
        "Nonfinite real Runtime input did not reject both domains before mutation");
    owner.retire(); accepted(owner.collect_joined());
}
void run(const std::filesystem::path& filename,const std::filesystem::path& profile,const std::filesystem::path& output,bool rainbow=false) {
    require(std::filesystem::is_directory(profile) && std::filesystem::is_empty(profile),"Cart private profile must exist and be empty");
    require(std::filesystem::is_directory(output) && std::filesystem::is_empty(output),"Cart capture directory must exist and be empty");
    const auto snapshot=rom::read_snapshot(filename);
    const auto world=course::world_descriptors(snapshot,rainbow?6:0).world;
    const player::CartResources resources(snapshot,rainbow?6:0,{world.fog_rgb,world.fog_min,world.fog_max});
    const photo::LightingInputs lighting(snapshot);
    auto hydrated=game_audio::hydrate(snapshot); require(bool(hydrated),"Cart borrowed Services audio resource admission failed");
    std::array<audio::effects::Section,8> sections; audio::effects::Config effects;
    require(game_audio::native_startup_effects(hydrated.resources,sections,effects)==audio::Error::none,"Cart shared audio configuration failed");
    private_audio::Capture pcm; private_audio::Execution execution{snapshot,nullptr,{}};
    auto audio=game_audio::Session::create(std::move(hydrated.resources),effects,pcm.output(),execution.executor()); require(bool(audio),"Cart borrowed Services Session failed");
    game::State state(profile); require(state.initialize(1),"Cart private profile initialization failed");
    const auto capacity=a::slot_capacity(resources.animation_resources().models[0]); const std::array capacities{capacity,capacity,capacity};
    const float callback_terminal=rainbow?static_cast<float>(rainbow_schedule(resources,false).models[0])*0.5f:150.0f;
    for (unsigned scenario=0;scenario<5;++scenario) {
        game_input::Service input; scene::Storage storage(16*1024*1024); native(storage.create());
        scene::Services services(storage.game(),storage.arena(),{{4,6,0,1,2},{}},input,*audio.session,state);
        private_player::Source context(services); auto* camera=source_camera(services); context.value.camera=camera;
        c::Owner owner(services,resources,context.value,capacities,{}); OwnerScope scope{owner};
        if (!scenario) {
            if (rainbow) { const auto cart=owner.init(); require(bool(cart),cart.status.detail); }
            else boundaries(services,owner,storage.arena(),resources);
            gpu(snapshot,services,owner,resources,camera,lighting,world,output,rainbow);
        }
        else if (scenario<4) callback_failure(services,owner,scenario,callback_terminal);
        else nonfinite_failure(owner);
    }
    if (rainbow) std::cout<<"Rainbow Cart focused source intro+fresh-outro advancement, real J-fogged RSP/RDP/readback, held graphics retirement and C callback-failure fences passed; no Beach-only boundary/full course/game/device claim\n";
    else std::cout<<"Cart actual four-function C Owner, independent native animations/callbacks/default FIFO/prepared capacity/retained joins and real J-fogged compiled RSP/RDP intro+fresh-outro consumer passed; no full Beach/game/device claim\n";
}
}
int main(int argc,char** argv) {
    const bool rainbow=argc==5 && std::string_view(argv[4])=="--rainbow";
    if (argc!=4 && !rainbow) { std::cerr<<"usage: player-cart-owner-rsp-smoke PRIVATE_ADMITTED_ROM EMPTY_PRIVATE_PROFILE EMPTY_CAPTURE_DIRECTORY [--rainbow]\n"; return EXIT_FAILURE; }
    try { run(argv[1],argv[2],argv[3],rainbow); return EXIT_SUCCESS; }
    catch (const std::exception& error) { std::cerr<<error.what()<<'\n'; return EXIT_FAILURE; }
}
