// Opt-in private admitted-ROM DATA/animator/native-camera witness only.
// No cart C lifecycle, fullRoot, renderer submission, audio or device claim.
#include "player_cart_resources.hpp"
#include "player_camera_animation.hpp"
#include "cart_animation_reference.hpp"
#include "beach_world_resources.hpp"
#include "scene_storage.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cfenv>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace m=isle::menu;
namespace a=m::animation;
namespace p=isle::player;
namespace c=p::camera_animation;
namespace {
void require(bool ok,const char* message) { if (!ok) throw std::runtime_error(message); }
void native(IsleGameResult result) { require(result==ISLE_GAME_OK,"Native camera operation rejected"); }
void accepted(a::Status status) {
    if (!status) {
        std::cerr<<"animation_error="<<static_cast<unsigned>(status.error)<<" guest="<<std::hex<<status.address<<std::dec<<'\n';
        throw std::runtime_error("Shared animator rejected actual cart stream");
    }
}
void accepted(c::Status status) { accepted(status.animation); native(status.native); }
void near(float actual,float expected,const char* domain="scalar") {
    if (!(std::isfinite(actual) && std::abs(actual-expected)<=0.002f))
        throw std::runtime_error(std::string("Actual trajectory differs from pinned source numeric reference: domain=")+domain+
            " actual="+std::to_string(actual)+" expected="+std::to_string(expected));
}
void near(m::Vec3 actual,m::Vec3 expected,const char* domain="position") {
    near(actual.x,expected.x,domain); near(actual.y,expected.y,domain); near(actual.z,expected.z,domain);
}
const m::AnimationCommand& command(const m::Animation& program,std::uint32_t guest,unsigned opcode,unsigned mask,unsigned duration) {
    for (const auto& q:program.commands) if (q.wire.guest_address==guest) {
        require(q.opcode==opcode && q.mask==mask && q.duration==duration,"Inspected real segment framing changed"); return q;
    }
    throw std::runtime_error("Inspected real segment missing");
}
float scalar(const m::Animation& program,std::uint32_t guest,unsigned opcode,unsigned mask,unsigned duration,unsigned operand=0) {
    return command(program,guest,opcode,mask,duration).wire.f32(4+operand*4);
}
// Explicit inspected segment schedule; arithmetic and independently decoded
// path DATA come from the private, source-pinned numeric reference.
struct Samples {
    a::Resources resources;
    const p::CartClip& clip;
    bool intro;
    const p::reference::CartNumericReference& reference;
    const m::Animation& camera() const { return resources.animations[clip.camera_animation]; }
    float camera_parameter(unsigned channel,double time) const {
        const auto& q=camera();
        if (intro) {
            if (channel==3) return p::reference::camera_cubic(0,scalar(q,0x8013dab8,6,520,150),
                scalar(q,0x8013da9c,6,136,0,1),scalar(q,0x8013dab8,6,520,150,1),150,static_cast<float>(time));
            if (time<60) return p::reference::camera_cubic(0,scalar(q,0x8013dacc,5,128,60),
                scalar(q,0x8013da9c,6,136,0,3),scalar(q,0x8013dacc,5,128,60,1),60,static_cast<float>(time));
            return p::reference::camera_cubic(scalar(q,0x8013dacc,5,128,60),scalar(q,0x8013dad8,5,128,90),
                scalar(q,0x8013dacc,5,128,60,1),scalar(q,0x8013dad8,5,128,90,1),90,static_cast<float>(time-60));
        }
        if (channel==3) return p::reference::camera_cubic(0,scalar(q,0x8013c56c,8,8,150),
            scalar(q,0x8013c53c,5,648,0,1),0,150,static_cast<float>(time));
        return p::reference::camera_cubic(0,scalar(q,0x8013c558,6,640,150),
            scalar(q,0x8013c53c,5,648,0,3),scalar(q,0x8013c558,6,640,150,1),150,static_cast<float>(time));
    }
    float fov(double time) const {
        const auto& q=camera();
        if (intro) return p::reference::camera_cubic(scalar(q,0x8013dab0,8,512,0),scalar(q,0x8013dab8,6,520,150,2),
            0,scalar(q,0x8013dab8,6,520,150,3),150,static_cast<float>(time));
        return p::reference::camera_cubic(scalar(q,0x8013c53c,5,648,0,4),scalar(q,0x8013c558,6,640,150,2),
            scalar(q,0x8013c53c,5,648,0,5),scalar(q,0x8013c558,6,640,150,3),150,static_cast<float>(time));
    }
    float model_parameter(double time) const {
        const auto& q=resources.animations[clip.model_bindings[0]];
        if (intro) {
            if (time<60) return 0;
            return p::reference::model_cubic(0,scalar(q,0x8013c6dc,5,8,90),scalar(q,0x8013c6d4,7,8,0),
                scalar(q,0x8013c6dc,5,8,90,1),90,static_cast<float>(time-60));
        }
        return p::reference::model_cubic(0,scalar(q,0x8013b15c,5,8,150),scalar(q,0x8013b148,6,8,0,1),
            scalar(q,0x8013b15c,5,8,150,1),150,static_cast<float>(time));
    }
    void model(const a::ModelState& state,double time) const {
        try {
        const auto& q=resources.animations[clip.model_bindings[0]];
        require(q.commands.front().opcode==13,"Root real path binding missing");
        near(state.nodes[0].transform.position,
            reference.sample(intro,p::reference::PathDomain::model,std::clamp(model_parameter(time),0.0f,1.0f)).position,"model");
        // Source animSetModelAnimation preserves DObj controls. The outro root
        // has no control command, so it retains the intro's terminal flag2.
        require(state.nodes[0].flags==(!intro || time<60 || time>=150 ? 2u : 0u),
                "Real root visibility control boundary lost");
        if (time<=2) {
            const auto& texture=resources.animations[clip.nodes[7].material_animations[0]];
            const auto first=texture.commands[0].wire.u32(4),target=texture.commands[1].wire.u32(4);
            require(texture.commands[0].opcode==20 && texture.commands[1].opcode==20 && texture.commands[1].duration==2,
                    "Real material sample is not the inspected two-frame color segment");
            const unsigned weight=static_cast<unsigned>(time*128); std::uint32_t color=0;
            for (unsigned shift=0;shift<32;shift+=8)
                color|=(((((first>>shift)&255u)*(256-weight))+(((target>>shift)&255u)*weight))>>8)<<shift;
            const auto attachment=state.nodes[7].first_material;
            require(state.materials[attachment].color_and_trailing_words[0]==color,
                    "Real material interpolation did not reach authoritative attached MObj state");
        }
        } catch (...) { std::cerr<<"model_sample intro="<<intro<<" time="<<time<<'\n'; throw; }
    }
    void camera(IsleGame* game,IsleGameCamera* native_camera,const a::CameraState& state,double time) const {
        try {
        const auto eye=reference.sample(intro,p::reference::PathDomain::eye,std::clamp(camera_parameter(3,time),0.0f,1.0f));
        const float at_parameter=std::clamp(camera_parameter(7,time),0.0f,1.0f);
        const auto at=reference.sample(intro,p::reference::PathDomain::at,at_parameter);
        near(state.eye,eye.position,"eye"); near(state.at,at.position,"at"); near(state.fov_y,fov(time),"fov");
        const IsleGameCameraParameters* value=nullptr; native(isle_game_camera_parameters(game,native_camera,&value));
        const m::Vec3 native_eye{value->look_at.eye[0],value->look_at.eye[1],value->look_at.eye[2]};
        const m::Vec3 native_at{value->look_at.at[0],value->look_at.at[1],value->look_at.at[2]};
        near(native_eye,eye.position,"native_eye"); near(native_at,at.position,"native_at");
        near(value->perspective.fovy,fov(time),"native_fov");
        if (intro && time==28.5) {
            std::uint32_t bits=0; static_assert(sizeof(bits)==sizeof(at_parameter));
            std::memcpy(&bits,&at_parameter,sizeof(bits));
            const auto precision=std::cout.precision(9);
            std::cout<<"bounded28.5 intro_at_source_parameter="<<at_parameter<<" bits=0x"<<std::hex<<bits<<std::dec
                <<" mapped_parameter="<<at.mapped_parameter<<" local_midpoint="<<at.mapped_parameter*4.0f
                <<" reference_xyz=("<<at.position.x<<','<<at.position.y<<','<<at.position.z
                <<") native_at_xyz=("<<native_at.x<<','<<native_at.y<<','<<native_at.z<<")\n";
            std::cout.precision(precision);
        }
        require(value->perspective.aspect==1.625f && value->perspective.near_plane==19 &&
                value->perspective.far_plane==43000 && value->look_at.up[1]==1,
                "Path camera publication overwrote unrelated native camera settings");
        float roll=0; native(isle_game_camera_roll(game,native_camera,&roll)); require(roll==0.375f,"Path clip overwrote unanimated roll");
        require(state.written_channels==0x277,"Path channels did not publish their actual XYZ write domains");
        native(isle_game_camera_compute_all(game,native_camera));
        for (unsigned i=0;i<2;++i) {
            const IsleGameCameraMatrix* matrix=nullptr; native(isle_game_camera_matrix(game,native_camera,i,&matrix));
            require(matrix->revision>0,"Real native camera matrix was not computed");
            for (const auto& row:matrix->floating) for (float x:row) require(std::isfinite(x),"Nonfinite actual path camera matrix");
        }
        } catch (...) { std::cerr<<"camera_sample intro="<<intro<<" time="<<time<<'\n'; throw; }
    }
};
void closure(const p::CartResources& cart,const isle::rom::Snapshot& snapshot) {
    require(!cart.display_lists().empty() && !cart.vertex_loads().empty() && !cart.image_loads().empty() &&
            !cart.dynamic_material_calls().empty(),"Actual cart graphics closure was omitted");
    const auto source=snapshot.canonical();
    for (const auto& span:cart.installation_spans())
        require(span.rom_offset<=source.size() && span.bytes.size()<=source.size()-span.rom_offset &&
                span.bytes.data()==source.data()+span.rom_offset,"Fabricated/unretained cart graphics bytes");
    std::vector<std::byte> ram(m::rdram_size,std::byte{0x5a});
    // Isolated CPU installation, explicitly no selected graphics task/scene.
    // This caller's reserved low-memory page is its complete reservation union.
    const std::array<m::RamRange,1> reservations{{{0,4096}}};
    cart.install(ram,reservations);
    std::vector<bool> touched(ram.size(),false);
    for (const auto& span:cart.installation_spans()) for (std::size_t i=0;i<span.bytes.size();++i) {
        const auto index=((span.guest_address-0x80000000u)+i)^3u;
        require(ram[index]==span.bytes[i],"Actual cart install changed retained immutable bytes"); touched[index]=true;
    }
    for (std::size_t i=0;i<ram.size();++i) if (!touched[i]) require(ram[i]==std::byte{0x5a},"Cart installed outside bounded graphics closure");
    const auto first=cart.installation_spans().front();
    const std::array<m::RamRange,2> collision{{reservations[0],{first.guest_address-0x80000000u,1}}};
    std::fill(ram.begin(),ram.end(),std::byte{0x3c}); bool rejected=false;
    try { cart.install(ram,collision); } catch (const std::invalid_argument&) { rejected=true; }
    require(rejected && std::all_of(ram.begin(),ram.end(),[](auto x) { return x==std::byte{0x3c}; }),
            "Actual reservation union collision partially installed cart bytes");
}
void run(const char* filename) {
    require(std::fegetround()==FE_TONEAREST,"Pinned binary32 numeric reference requires FE_TONEAREST");
    const auto snapshot=isle::rom::read_snapshot(filename);
    const p::reference::CartNumericReference reference(snapshot);
    const auto world=isle::beach::world_descriptors(snapshot);
    const p::CartResources cart(snapshot,0,{world.world.fog_rgb,world.world.fog_min,world.world.fog_max});
    const auto resources=cart.animation_resources();
    require(resources.models.size()==1 && resources.models[0].nodes.size()==43 && resources.materials.size()==8 &&
            resources.animations.size()==92 && resources.paths.size()==6 && resources.camera_animation==m::no_index,
            "Exact complete immutable cart resource family missing");
    require(resources.models[0].tree.guest_address==0x803aaa30 && resources.models[0].material_table.guest_address==0x8039d938 &&
            resources.models[0].render_type==m::RenderType::j_fogged && resources.models[0].matrix_kinds==std::array<std::uint8_t,3>{28,0,0},
            "Pinned PlayerModel_Init model/render/matrix metadata changed");
    for (const auto* clip:{&cart.intro(),&cart.outro()}) {
        require(clip->nodes.size()==43 && clip->model_bindings.size()==43 && clip->material_bindings.size()==8,
                "Exact per-node and per-attachment animation list extents lost");
        for (std::size_t i=0;i<43;++i) {
            const auto guest=clip->model_animation_table.u32(i*4);
            const auto index=clip->model_bindings[i];
            require((guest==0)==(index==m::no_index),"NULL model list slot was not preserved");
            if (guest) require(resources.animations[index].wire.guest_address==guest,"Actual model list identity lost");
            const auto& binding=clip->nodes[i];
            if (!binding.material_animation_table.bytes.empty())
                require(binding.material_animations.size()==resources.models[0].nodes[i].materials.size(),"Actual material-list extent changed");
        }
    }
    closure(cart,snapshot);
    const std::array<a::SlotCapacity,1> capacities{{a::slot_capacity(resources.models[0])}};
    require(capacities[0].nodes==43 && capacities[0].material_attachments==8,"Cart live slot capacity differs from actual model");
    auto made=a::Runtime::create(resources,capacities,{},{}); accepted(made.status);
    auto& runtime=*made.runtime; const auto acquired=runtime.acquire(0,0,true); accepted(acquired.status); const auto handle=acquired.handle;
    isle::scene::Storage storage; native(storage.create()); auto* game=storage.game();
    IsleGameObject* object=nullptr; native(isle_game_add_object(game,7,9,9,nullptr,nullptr,&object));
    IsleGameCamera* camera=nullptr; native(isle_game_attach_camera(game,object,&camera));
    native(isle_game_camera_add_matrix(game,camera,ISLE_GAME_CAMERA_PERSPECTIVE_FAST));
    native(isle_game_camera_add_matrix(game,camera,ISLE_GAME_CAMERA_LOOK_AT_REFLECT_ROLL));
    IsleGamePerspective perspective{63,1.625f,19,43000,0.75f};
    native(isle_game_camera_set_perspective(game,camera,&perspective)); native(isle_game_camera_set_roll(game,camera,0.375f));
    IsleGameLookAt look{{720,310,900},{-100,70,50},{0,1,0}};
    native(isle_game_camera_set_look_at(game,camera,&look));
    auto driver_made=c::NativeDriver::create(game,camera,resources,{}); accepted(driver_made.status); auto& driver=*driver_made.driver;
    // Explicit future Player-owner initialization; default/Title remains0.5.
    accepted(driver.set_speed(1));
    for (bool intro:{true,false}) {
        const auto& clip=intro?cart.intro():cart.outro(); const Samples sample{resources,clip,intro,reference};
        require(clip.camera_identity==(intro?0x8013da90u:0x8013c530u),"Wrong actual Beach camera clip");
        IslePlayerCameraAnimationClip camera_clip{}; accepted(driver.clip(clip.camera_animation,camera_clip));
        auto bind=[&](float skip) {
            accepted(runtime.bind_model(handle,clip.model_bindings,skip)); accepted(runtime.bind_materials(handle,clip.material_bindings,skip));
            accepted(runtime.set_model_speed(handle,0.5f)); accepted(runtime.set_texture_speed(handle,0.5f));
            accepted(driver.set_speed(0.5f)); accepted(driver.set(&camera_clip,skip));
        };
        bind(0); unsigned completed=0;
        for (unsigned tick=0;tick<=300;++tick) {
            const double time=tick*0.5; accepted(runtime.step_model(handle)); accepted(driver.update(0));
            for (const auto& event:runtime.events()) {
                require(event.event_id==-1 && event.instance==handle && event.node==0 && tick==300,
                        "Real cart callback receiver/end boundary changed"); ++completed;
            }
            sample.model(runtime.model(handle),time); sample.camera(game,camera,driver.state(),time);
            require(driver.state().clock.phase==(tick==300?a::Phase::disabled:a::Phase::running),"Camera ended before/after actual150-frame terminal");
        }
        require(completed==1,"Actual model END callback missing or duplicated");
        const auto ended=runtime.model(handle).nodes[0].transform.position; const auto camera_ended=driver.state().eye;
        accepted(runtime.step_model(handle)); accepted(driver.update(0));
        near(runtime.model(handle).nodes[0].transform.position,ended); near(driver.state().eye,camera_ended);
        require(runtime.events().empty() && driver.state().written_channels==0,"Disabled real clip continued publishing");
        // Real control/path boundaries and skipped END in the same retained
        // tracks/programs. No rewritten clip, narrowed program or new interpreter.
        for (float skip:{0.0f,59.5f,60.0f,60.5f,149.5f,150.0f,151.0f}) {
            bind(skip); accepted(runtime.step_model(handle)); accepted(driver.update(0));
            const auto time=std::min<double>(skip,150); sample.model(runtime.model(handle),time); sample.camera(game,camera,driver.state(),time);
            // Pinned anim.c END corrects active track time even when skipped,
            // emits the root callback, then the updater disables the clock.
            const bool skipped_end=skip>=150;
            require(driver.state().clock.phase==(skipped_end?a::Phase::disabled:a::Phase::running),
                    "Skipped camera END phase changed");
            require(runtime.model(handle).nodes[0].clock.phase==(skipped_end?a::Phase::disabled:a::Phase::running),
                    "Skipped root model END phase changed");
            require(runtime.events().size()==(skipped_end?1u:0u),"Skipped model END callback missing or duplicated");
            for (const auto& event:runtime.events())
                require(event.event_id==-1 && event.argument==0 && event.instance==handle && event.node==0,
                        "Skipped model END callback receiver changed");
        }
        bind(30);
        const auto suppressed=driver.state();
        const IsleGameCameraParameters* before=nullptr; native(isle_game_camera_parameters(game,camera,&before));
        const auto native_suppressed=*before;
        // GOBJ_FLAG_2 suppresses writes, not interpreter/track advancement
        // (pinned anim.c 2301–2305); the next publication must be at30.5.
        accepted(driver.update(2)); require(driver.state().written_channels==0,"Native flag2 did not suppress camera path publication");
        near(driver.state().eye,suppressed.eye,"flag2_eye");
        near(driver.state().at,suppressed.at,"flag2_at"); near(driver.state().fov_y,suppressed.fov_y,"flag2_fov");
        const IsleGameCameraParameters* after=nullptr; native(isle_game_camera_parameters(game,camera,&after));
        for (unsigned axis=0;axis<3;++axis) {
            near(after->look_at.eye[axis],native_suppressed.look_at.eye[axis],"flag2_native_eye");
            near(after->look_at.at[axis],native_suppressed.look_at.at[axis],"flag2_native_at");
        }
        near(after->perspective.fovy,native_suppressed.perspective.fovy,"flag2_native_fov");
        accepted(driver.update(0)); sample.camera(game,camera,driver.state(),30.5);
        accepted(driver.set(nullptr,0)); accepted(driver.update(0));
        require(driver.state().clock.phase==a::Phase::disabled,"Real NULL camera-list termination changed");
    }
    driver.retire(); runtime.retire();
    std::cout<<"cart actual intro/outro model/material/path/native-camera and bounded graphics consumer passed; no cart-owner/fullRoot/game/audio/device claim\n";
}
}
int main(int argc,char** argv) {
    if (argc!=2) { std::cerr<<"usage: player-cart-resources-rom-smoke PRIVATE_ADMITTED_ROM\n"; return EXIT_FAILURE; }
    try { run(argv[1]); return EXIT_SUCCESS; }
    catch (const std::exception& error) { std::cerr<<error.what()<<'\n'; return EXIT_FAILURE; }
}
