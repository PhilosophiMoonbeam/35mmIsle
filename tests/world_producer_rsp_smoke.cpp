// Opt-in private-ROM component renderer consumer. NOT a Beach scene, player or
// actor implementation, and not an Original/gameplay parity claim. Main executes.
#include "world_producer.hpp"
#include "world_movement.hpp"
#include "graphics_task.hpp"
#include "rsp_render.hpp"
#include <algorithm>
#include <array>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
using namespace isle;
void require(bool b, const char* message) { if (!b) throw std::runtime_error(message); }
void native(IsleGameResult r) { require(r == ISLE_GAME_OK,"native world consumer failure"); }
constexpr std::uint32_t top=0x200000, viewport=0x20ff80, boot=0x277000, stack=0x270000,
    yield=0x274000, task=0x276000, output=0x400000, output_end=0x440000,
    color=0x3b5000,depth=0x600000,guard=0x7f0000;
constexpr std::size_t target_bytes=320*240*2;
struct Owners {
    std::vector<std::byte> bytes=std::vector<std::byte>(4*1024*1024);
    isle_arena arena{};
    IsleGame* game=nullptr;
    IsleGameCamera* camera=nullptr;
    std::size_t commands=0, shifts=0, effect_shifts=0;
    world::Vec3 effect_position{1,2,3};
    Owners() {
        require(isle_arena_init(&arena,bytes.data(),bytes.size())==ISLE_OK,"arena init"); native(isle_game_create(&arena,&game));
        IsleGameObject* object=nullptr; native(isle_game_add_object(game,1,0,0,nullptr,nullptr,&object));
        native(isle_game_attach_camera(game,object,&camera));
        native(isle_game_camera_add_matrix(game,camera,ISLE_GAME_CAMERA_PERSPECTIVE_FAST));
        native(isle_game_camera_add_matrix(game,camera,ISLE_GAME_CAMERA_LOOK_AT_REFLECT_ROLL));
        const IsleGamePerspective perspective{55,4.0f/3.0f,10,25600,1}; native(isle_game_camera_set_perspective(game,camera,&perspective));
        // Explicit diagnostic pose: not a fabricated first Player camera frame.
        const IsleGameLookAt look{{0,1600,6000},{0,500,0},{0,1,0}}; native(isle_game_camera_set_look_at(game,camera,&look));
    }
    ~Owners() { if (isle_game_destroy(game)!=ISLE_GAME_OK) std::terminate(); }
    static void command(void* p,IsleGameObject*) noexcept { ++static_cast<Owners*>(p)->commands; }
    static void shift_camera(void* p,world::Vec3 delta) noexcept {
        auto& o=*static_cast<Owners*>(p); const IsleGameCameraParameters* old=nullptr;
        if (isle_game_camera_parameters(o.game,o.camera,&old)!=ISLE_GAME_OK) std::terminate();
        auto look=old->look_at; look.eye[0]+=delta.x;look.eye[1]+=delta.y;look.eye[2]+=delta.z;
        look.at[0]+=delta.x;look.at[1]+=delta.y;look.at[2]+=delta.z;
        if (isle_game_camera_set_look_at(o.game,o.camera,&look)!=ISLE_GAME_OK) std::terminate();
        ++o.shifts;
    }
    static void shift_effects(void* p,world::Vec3 d) noexcept {
        auto& o=*static_cast<Owners*>(p); o.effect_position.x+=d.x;o.effect_position.y+=d.y;o.effect_position.z+=d.z;++o.effect_shifts;
    }
};
void top_list(std::span<std::uint32_t> words,const world::ProducerFrame& frame) {
    graphics::Memory m(words);std::uint32_t cursor=top;
    auto cmd=[&](std::uint32_t a,std::uint32_t b){require(cursor+8<viewport,"top capacity");m.word(cursor,a);m.word(cursor+4,b);cursor+=8;};
    const std::array<std::int16_t,8> vp{640,480,511,0,640,480,511,0};
    for(unsigned i=0;i<8;++i)m.half(viewport+i*2,std::uint16_t(vp[i]));
    cmd(0xdb060000,0);cmd(0xe7000000,0);cmd(0xed000000,(1280u<<12)|960u);
    cmd(0xef300000,0);cmd(0xff10013f,depth);cmd(0xf7000000,0xfffcfffc);cmd(0xf64fc3bc,0);cmd(0xe7000000,0);
    cmd(0xff10013f,color);cmd(0xf7000000,0x5c735c73);cmd(0xf64fc3bc,0);cmd(0xe7000000,0);cmd(0xfe000000,depth);
    cmd(0xef080cf0,0x00552078);cmd(0xd9000000,0x00200405);
    cmd(0xdb040004,1);cmd(0xdb04000c,1);cmd(0xdb040014,0xffff);cmd(0xdb04001c,0xffff);cmd(0xd7000000,0);
    cmd(0xdc080008,viewport);
    for(const unsigned lane:std::array<unsigned,4>{0,2,1,3}){cmd(0xde000000,frame.camera.address);cmd(0xde000000,frame.lanes[lane].address);}
    cmd(0xe9000000,0);cmd(0xdf000000,0);
}
struct Result { std::vector<std::uint32_t> triangles;std::vector<std::byte> pixels; };
int run(const std::filesystem::path& path) {
    const auto snapshot=rom::read_snapshot(path);const world::Resources resources(snapshot);Owners owners;
    const auto view=world::ResourceView{&resources.setup(),&resources.sky(),resources.blocks(),resources.primary_count(),resources.models(),
        resources.materials(),resources.animations(),resources.paths(),resources.static_definitions(),&resources.height_map(),resources.collision_models()};
    world::Callbacks callbacks;callbacks.user=&owners;callbacks.shift_camera=Owners::shift_camera;
    callbacks.shift_effects=Owners::shift_effects;callbacks.process_commands=Owners::command;
    world::World component(owners.game,view,callbacks);component.follow_camera(owners.camera);
    world::MovementState movement_state;world::Movement movement(component,movement_state,{});require(movement.initialize(0),"real road admission");
    const std::array<menu::RamRange,6> other{{{top,0x10000},{stack,0x10000},{output,output_end-output},
        {color,target_bytes},{depth,target_bytes},{guard,256}}};
    world::Producer producer(owners.game,owners.camera,resources,component,{},other);
    rsp::CanonicalExecutor executor(true,{16000000,4*1024*1024});
    const auto arena_used=owners.arena.cursor;
    auto execute=[&](bool cold){
        auto preparation=executor.begin_preparation();auto words=preparation.words();
        if(cold){
            graphics::validate_cold_inputs(words,snapshot,snapshot.canonical().data(),boot);
            std::vector<beach::RamReservation> reserved;reserved.reserve(other.size()+producer.reservations().size()+1);
            reserved.push_back({0,graphics::bss_end});for(auto r:other)reserved.push_back({r.offset,r.size});
            for(auto r:producer.reservations())reserved.push_back({r.offset,r.size});
            resources.install(std::as_writable_bytes(words),reserved);graphics::install_cold_inputs(words,snapshot,boot);
            for(unsigned i=0;i<64;++i)words[guard/4+i]=0x13579bdf;
        }
        const auto before=std::vector<std::uint32_t>(words.begin(),words.end());
        bool rejected=false;try{producer.prepare(words.first(words.size()-1),1);}catch(const std::exception&){rejected=true;}
        require(rejected&&std::equal(words.begin(),words.end(),before.begin()),"invalid preparation modified RAM");
        scene::render::Pass update_link_only;
        update_link_only.camera=owners.camera;update_link_only.link_mask=std::uint64_t(1)<<9;
        const auto filtered=producer.prepare(words,1,update_link_only);
        for(const auto lane:filtered.lanes) require(lane.commands==1,"update link9 was incorrectly treated as renderDL3");
        const auto frame=producer.prepare(words,1);
        for(std::size_t i=0;i<words.size();++i){const auto address=i*4;
            const bool owned=std::any_of(frame.reservations.begin(),frame.reservations.end(),[&](menu::RamRange r){return address>=r.offset&&address<r.offset+r.size;});
            if(!owned)require(words[i]==before[i],"world producer wrote outside its exact owners");}
        top_list(words,frame);const auto task_result=graphics::finish_task(words,snapshot,boot,stack,yield,task,output,output_end,top);
        auto completed=executor.execute(std::move(preparation),task_result.task,task_result.boot);Result result;
        const auto& packets=completed.packets();for(const auto packet:packets.packets()){
            const auto data=packets.words().subspan(packet.offset,packet.count);const auto opcode=(data[0]>>24)&63;
            if(opcode>=8&&opcode<=15)result.triangles.insert(result.triangles.end(),data.begin(),data.end());}
        result.pixels.resize(target_bytes);for(std::size_t i=0;i<target_bytes;++i)result.pixels[i]=completed.rdram()[(color+i)^3];
        for(unsigned i=0;i<64;++i)require(reinterpret_cast<const std::uint32_t*>(completed.rdram().data())[guard/4+i]==0x13579bdf,"actual consumer changed guard");
        completed.release();require(executor.quiescent(),"GPU borrow not retired");require(owners.arena.cursor==arena_used,"world frame allocated native storage");return result;
    };
    native(isle_game_tick(owners.game));const auto initial=execute(true);require(!initial.triangles.empty(),"world canonical task emitted no geometry");
    std::uint32_t original_visibility=0;
    for(const auto& block:component.blocks()) {
        IsleGameObjectMetadata metadata{};native(isle_game_object_metadata(owners.game,block.object,&metadata));
        if(!(metadata.flags&1u)) original_visibility|=1u<<block.index;
    }
    component.set_visibility(0);const auto hidden=execute(false);component.set_visibility(original_visibility);const auto restored=execute(false);
    require(initial.triangles!=hidden.triangles,"source visibility did not change actual geometry");
    require(initial.triangles==restored.triangles&&initial.pixels==restored.pixels,"unchanged world re-show did not restore canonical output");
    // Explicit component entry owns this diagnostic chain. Movement::update()
    // also enters the next block on carried speed; it is not a pose-only sample.
    // Match movement.c:270-277: restart the entered road, then sample its clock.
    for(std::size_t i=1;i<resources.primary_count();++i){
        auto* block=component.enter_next();
        require(block==&component.blocks()[i],"complete primary chain transition failed");
        if(block->road_model&&block->resource->metadata.movement_animation_guest)component.reset_road(*block,0);
        movement_state.block=block;movement_state.move_time=0.5f;
        movement.update_position(*block,movement_state.move_time,movement_state.control_time);
        require(component.current()==block&&movement_state.block==block,"road sampling changed primary chain ownership");
        require(owners.shifts==i&&owners.effect_shifts==i,"native camera/effect translation did not follow each primary entry");
        native(isle_game_tick(owners.game));execute(false);
    }
    producer.retire();component.retire();IsleGameCounts counts{};native(isle_game_counts(owners.game,&counts));
    require(counts.objects==1&&counts.models==0&&counts.processes==0,"world retirement retained native resources");
    std::cout<<"world_component_rsp_rdp=passed primary=8 secondary=4 visibility=hide_restore native_retirement=joined gameplay=not_claimed\n";return 0;
}
}
#ifdef _WIN32
int wmain(int argc,wchar_t** argv){
#else
int main(int argc,char** argv){
#endif
    try{require(argc==2,"expected one admitted private ROM path");return run(std::filesystem::path(argv[1]));}
    catch(const std::exception& e){std::cerr<<"private world consumer: "<<e.what()<<'\n';return 1;}
}
