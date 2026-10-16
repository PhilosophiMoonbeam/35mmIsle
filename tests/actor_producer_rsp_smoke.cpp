// Opt-in actual PRIVATE_ROM RSP/RDP consumer. Main executes; workers do not.
// Authored placement/control, NOT a native actor factory/FSM/player/detector.
#include "actor_producer.hpp"
#include "contextual_model_math.h"
#include "graphics_task.hpp"
#include "rsp_render.hpp"
#include "photo_lighting.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
using namespace isle;
namespace a = menu::animation;
namespace k = menu::model;
void require(bool value, const char* text) { if (!value) throw std::runtime_error(text); }
void native(IsleGameResult result) { require(result == ISLE_GAME_OK,"actor consumer native operation failed"); }
template<class F> void rejects(F&& action, const char* text) {
    bool rejected = false; try { action(); } catch (const std::exception&) { rejected = true; }
    require(rejected,text);
}
constexpr std::uint32_t top = 0x626000, identity = 0x627f40, viewport = 0x627f80, stack = 0x630000,
    yield = 0x634000, task = 0x636000, boot = 0x637000, output = 0x640000,
    output_end = 0x680000, color = 0x3b5000, alternate_color = 0x3da800,
    depth = 0x600000, guard = 0x7f0000;
constexpr std::size_t plane_bytes = 320*240*2, live_slots = 6;
constexpr actor::ProducerLayout layout{{0x400000,0x80000},{0x480000,0x80000},{0x500000,0x100000},
    {{{color,plane_bytes},{alternate_color,plane_bytes}}}};
constexpr std::array<menu::RamRange,7> other{{{top,0x2000},{stack,0x10000},
    {output,output_end-output},{depth,plane_bytes},{guard,256},{0xaebd0,24},{0xaec18,64}}};
struct Owners {
    std::vector<std::byte> bytes = std::vector<std::byte>(16*1024*1024);
    isle_arena arena{};
    IsleGame* game = nullptr;
    IsleGameCamera* camera = nullptr;
    Owners() {
        require(isle_arena_init(&arena,bytes.data(),bytes.size()) == ISLE_OK,"actor arena initialization");
        native(isle_game_create(&arena,&game));
        IsleGameObject* object = nullptr; native(isle_game_add_object(game,1,0,0,nullptr,nullptr,&object));
        native(isle_game_attach_camera(game,object,&camera));
        native(isle_game_camera_add_matrix(game,camera,ISLE_GAME_CAMERA_PERSPECTIVE_FAST));
        native(isle_game_camera_add_matrix(game,camera,ISLE_GAME_CAMERA_LOOK_AT_REFLECT_ROLL));
        const IsleGamePerspective perspective{60,4.0f/3.0f,10,25600,1};
        // Preserve genuine distance LOD: Snorlax and Pikachu draw nothing beyond
        // their far BRANCH_Z choices. Frame the complete six-actor cohort nearer.
        const IsleGameLookAt look{{325,800,2600},{325,200,0},{0,1,0}};
        native(isle_game_camera_set_perspective(game,camera,&perspective));
        native(isle_game_camera_set_look_at(game,camera,&look));
    }
    ~Owners() { if (game && isle_game_destroy(game) != ISLE_GAME_OK) std::terminate(); }
};
struct Capture {
    std::vector<std::uint32_t> triangles;
    std::vector<std::byte> pixels, depths;
    bool operator==(const Capture&) const = default;
};
std::size_t rgb5_changes(const Capture& a,const Capture& b) {
    require(a.pixels.size()==b.pixels.size() && a.pixels.size()==plane_bytes,"actual actor color extent changed");
    std::size_t changed=0;
    for (std::size_t p=0;p<a.pixels.size();p+=2)
        changed+=((std::to_integer<unsigned>(a.pixels[p]^b.pixels[p])<<8)|
                  (std::to_integer<unsigned>(a.pixels[p+1]^b.pixels[p+1])&0xfeu))!=0;
    return changed; // RGBA16 bit0 is coverage; it is not a visible RGB5 change.
}
void save_capture(const std::filesystem::path& directory,const char* name,const Capture& capture) {
    if (directory.empty()) return;
    const auto path=directory/name;
    require(!std::filesystem::exists(path),"private actor capture already exists");
    std::ofstream out(path,std::ios::binary);
    out<<"P6\n320 240\n255\n";
    for (std::size_t p=0;p<capture.pixels.size();p+=2) {
        const auto value=(std::to_integer<unsigned>(capture.pixels[p])<<8)|std::to_integer<unsigned>(capture.pixels[p+1]);
        const char rgb[]{static_cast<char>(((value>>11)&31)*255/31),
            static_cast<char>(((value>>6)&31)*255/31),static_cast<char>(((value>>1)&31)*255/31)};
        out.write(rgb,3);
    }
    out.flush(); require(bool(out),"private actual actor capture write failed");
}
void top_list(std::span<std::uint32_t> words, const scene::render::Prepared& frame,
              const beach::WorldMetadata& world,std::uint32_t pre_render) {
    graphics::Memory memory(words); std::uint32_t cursor = top;
    auto cmd = [&](std::uint32_t x, std::uint32_t y) {
        require(cursor+8 <= identity,"actor top-list capacity");
        memory.word(cursor,x); memory.word(cursor+4,y); cursor += 8;
    };
    auto mode = [&](bool high,unsigned shift,unsigned length,std::uint32_t value) {
        cmd((high?0xe3000000u:0xe2000000u)|((32u-shift-length)<<8)|(length-1),value);
    };
    for (unsigned i=0;i<16;++i) {
        memory.half(identity+i*2,i%5==0?1:0);
        memory.half(identity+32+i*2,0);
    }
    const std::array<std::int16_t,8> vp{640,480,511,0,640,480,511,0};
    for (unsigned i = 0; i < vp.size(); ++i) memory.half(viewport+i*2,std::uint16_t(vp[i]));
    const auto& rgb = world.background_rgb;
    const auto background = (std::uint32_t(rgb[0]>>3)<<11)|(std::uint32_t(rgb[1]>>3)<<6)|
        (std::uint32_t(rgb[2]>>3)<<1)|1u;
    cmd(0xdb060000,0); cmd(0xdb06003c,color); cmd(0xe7000000,0); cmd(0xed000000,(1280u<<12)|960u);
    cmd(0xef300000,0); cmd(0xff10013f,depth); cmd(0xf7000000,0xfffcfffc); cmd(0xf64fc3bc,0); cmd(0xe7000000,0);
    cmd(0xff10013f,color); cmd(0xf7000000,(background<<16)|background); cmd(0xf64fc3bc,0);
    // Exact GAME srdpReset; use the same public numeric encoding as Commands::reset.
    // Actor display lists may inherit these defaults, unlike self-contained terrain.
    cmd(0xe7000000,0); cmd(0xfe000000,depth); cmd(0xdc080008,viewport);
    constexpr std::uint32_t cleared=1u|4u|0x600u|0x10000u|0x20000u|0x40000u|0x80000u|0x100000u|0x200000u;
    cmd(0xd9000000u|((~cleared)&0xffffffu),0);
    cmd(0xdb040004,1); cmd(0xdb04000c,1); cmd(0xdb040014,0xffff); cmd(0xdb04001c,0xffff);
    cmd(0xd7000000,0); cmd(0xd9ffffff,0x200405);
    cmd(0xda380007,identity); cmd(0xda380003,identity);
    mode(true,20,2,0); mode(true,23,1,0);
    cmd(0xfcffffff,0xfffe793c);
    mode(true,16,1,0); mode(true,14,2,0); mode(true,17,2,0);
    mode(true,19,1,0x80000); mode(true,12,2,0x2000);
    mode(true,9,3,0xc00); mode(true,8,1,0);
    mode(false,0,2,0); mode(false,3,29,0x0f0a4000);
    mode(true,6,2,0); cmd(0xe7000000,0);
    cmd(0xed000000,(1280u<<12)|960u);
    cmd(0xde000000,pre_render); // Exact Beach SceneSetup callback800A1A50 -> common source lights.
    // Same source compositor lane order; each genuine EndDL sublist returns.
    for (unsigned lane : std::array<unsigned,4>{0,2,1,3}) {
        cmd(0xde000000,frame.camera.address); cmd(0xde000000,frame.lanes[lane].address);
    }
    cmd(0xe9000000,0); cmd(0xdf000000,0);
}
void bind_initial(const beach::ActorResources& resources, const beach::ActorInitialization& init,
                  a::Runtime& runtime, a::InstanceHandle handle) {
    if (!init.animations_enabled) return; // Actual geometryless1005 NULL header.
    const auto& header = resources.headers()[init.setup.initial_header];
    std::vector<std::size_t> models, materials;
    for (std::size_t n = 0; n < header.nodes.size(); ++n) {
        models.push_back(header.nodes[n].model_animation);
        const auto count = resources.models()[init.model].nodes[n].materials.size();
        const auto begin = materials.size(); materials.resize(begin+count,menu::no_index);
        std::copy(header.nodes[n].material_animations.begin(),header.nodes[n].material_animations.end(),materials.begin()+static_cast<std::ptrdiff_t>(begin));
    }
    require(bool(runtime.bind_model(handle,models,0,a::ModelBindPose::restore)),"actual source initial model header");
    if (header.resets_materials()) require(bool(runtime.reset_materials(handle)),"actual source texture reset");
    else require(bool(runtime.bind_materials(handle,materials)) && bool(runtime.set_texture_speed(handle,header.speed)),"actual source texture header");
    require(bool(runtime.set_model_speed(handle,header.speed)) && bool(runtime.step_model(handle)),"source immediate header evaluation");
    // This diagnostic does not dispatch retained animation events to sound/FX.
}
int run(const std::filesystem::path& path,const std::filesystem::path& output_directory={}) {
    require(output_directory.empty() || (std::filesystem::is_directory(output_directory) &&
            std::filesystem::is_empty(output_directory)),"private capture directory must exist and be empty");
    const auto snapshot = rom::read_snapshot(path);
    const auto world = beach::world_descriptors(snapshot);
    const beach::ActorResources resources(snapshot,{world.world.fog_rgb,world.world.fog_min,world.world.fog_max});
    const photo::LightingInputs lighting(snapshot);
    const std::array<photo::LightingSpan,2> common_render{lighting.installation_spans()[0],lighting.pre_render()};
    for (std::size_t i=0;i<common_render.size();++i)
        require(common_render[i].guest_address-0x80000000u==other[5+i].offset &&
                common_render[i].bytes.size()==other[5+i].size,"source global lighting differs from admitted complete RAM union");
    const auto models = resources.models(); const auto inits = resources.initializations();
    std::array<std::size_t,live_slots> selected{};
    constexpr std::array<std::uint32_t,live_slots> ids{143,25,1006,1005,25,1003};
    a::SlotCapacity capacity{};
    for (const auto& model : models) {
        const auto c = a::slot_capacity(model);
        capacity.nodes = std::max(capacity.nodes,c.nodes);
        capacity.material_attachments = std::max(capacity.material_attachments,c.material_attachments);
    }
    std::array<a::SlotCapacity,live_slots> capacities; capacities.fill(capacity);
    for (std::size_t slot = 0; slot < live_slots; ++slot) {
        const auto found = std::find_if(inits.begin(),inits.end(),[&](const auto& init) { return init.actor_id == ids[slot]; });
        require(found != inits.end(),"actual actor source initialization missing");
        selected[slot] = static_cast<std::size_t>(found-inits.begin());
    }
    auto made = a::Runtime::create({models,resources.materials(),resources.animations(),menu::no_index},capacities,{},{32768,4096});
    require(bool(made),"all actor animations must compile once in shared Runtime"); auto& runtime = *made.runtime;
    Owners owners; std::array<a::InstanceHandle,live_slots> handles{};
    std::array<actor::ActorRenderBinding,live_slots> bindings{};
    std::array<IsleGameModelTransform,live_slots> placements{};
    std::vector<IsleGameModelNodeInit> attachment(capacity.nodes);
    auto spawn = [&](std::size_t slot) {
        const auto& init = inits[selected[slot]];
        const auto acquired = runtime.acquire(slot,init.model,true); require(bool(acquired),"actual actor Runtime acquisition");
        handles[slot] = acquired.handle; bind_initial(resources,init,runtime,handles[slot]);
        IsleGameObject* object = nullptr; native(isle_game_add_object(owners.game,100+static_cast<std::uint32_t>(slot),4,0x80000000,nullptr,nullptr,&object));
        IsleGameModel* model = nullptr;
        require(bool(k::attach_source_model(owners.game,object,models[init.model],runtime.model(handles[slot]),
            k::RootRecipe::source,attachment,&model)),"actual native attachment using source recipes");
        placements[slot] = {{(float(slot)-2.0f)*650.0f,0,float(slot%2)*-500.0f},{0,0,0},
            {init.scale.x*0.1f,init.scale.y*0.1f,init.scale.z*0.1f}};
        native(isle_game_model_set_placement(owners.game,model,ISLE_GAME_MODEL_PLACEMENT_RTS,&placements[slot]));
        bindings[slot] = {{slot,runtime.model(handles[slot]),object,model,k::PoseSource::animation,
            k::RootRecipe::source,true,&runtime},{static_cast<std::uint8_t>(slot == 2 || slot == 5 ? 4 : 5),0x80000000,1u<<slot},false};
    };
    for (std::size_t slot = 0; slot < live_slots; ++slot) spawn(slot);
    require(handles[1] != handles[4] && runtime.model(handles[1]).resource_model == runtime.model(handles[4]).resource_model,
            "two Pikachu instances must share one immutable resource without sharing incarnation");
    bool source54 = false, source48 = false, ordinary28 = false;
    for (std::size_t slot = 0; slot < live_slots; ++slot) for (std::size_t n = 0; n < models[inits[selected[slot]].model].nodes.size(); ++n) {
        k::MatrixRecipe recipe; require(bool(k::decode_model_recipe(models[inits[selected[slot]].model],n,recipe)),"actual recipe decode");
        source54 |= ids[slot] == 143 && recipe.operations[0] == 54;
        source48 |= ids[slot] == 25 && recipe.count == 2 && recipe.operations[1] == 48;
        ordinary28 |= recipe.count == 1 && recipe.operations[0] == 28;
    }
    require(source54 && source48 && ordinary28,"actual Snorlax54/Pikachu48/ordinary28 recipe admission");
    require(models[inits[selected[3]].model].nodes[0].payloads.empty(),"1005 must remain genuinely geometryless");
    k::CameraBasisState basis;
    {
        const IsleGameCameraReflection* reflection=nullptr;
        native(isle_game_camera_reflection(owners.game,owners.camera,1,&reflection));
        const IsleGameCameraParameters* parameters=nullptr;
        native(isle_game_camera_parameters(owners.game,owners.camera,&parameters));
        auto changed=parameters->look_at; changed.eye[0]+=1;
        native(isle_game_camera_set_look_at(owners.game,owners.camera,&changed));
        std::array<actor::ActorRenderBinding,1> vacant{};
        const std::array<a::SlotCapacity,1> empty_capacity{{{1,0}}};
        actor::Producer empty(owners.game,owners.camera,basis,resources,empty_capacity,vacant,layout,other,actor::DetectorPolicy::disabled);
        auto* start=reinterpret_cast<std::uint32_t*>(const_cast<IsleGameCameraReflection*>(reflection));
        require(reinterpret_cast<std::byte*>(start)>=owners.bytes.data() &&
                reinterpret_cast<std::byte*>(start)+beach::rdram_size<=owners.bytes.data()+owners.bytes.size(),
                "Actual native camera alias witness exceeds owned arena storage");
        const std::span alias(start,beach::rdram_size/4);
        const std::vector<std::uint32_t> unchanged(alias.begin(),alias.end());
        rejects([&] { empty.prepare(alias,1); },
                "Actor preparation accepted RAM aliasing native reflection/cache after its matrix result");
        require(std::equal(alias.begin(),alias.end(),unchanged.begin()),
                "Rejected camera-storage alias changed native state or canonical RAM");
        changed.eye[0]-=1;
        native(isle_game_camera_set_look_at(owners.game,owners.camera,&changed));
    }
    actor::Producer producer(owners.game,owners.camera,basis,resources,capacities,bindings,layout,other,actor::DetectorPolicy::disabled);
    // Full-union constructor rejection: never weaken old World/task guards.
    auto bad_layout = layout; bad_layout.payloads = {0x170000,0x10000};
    rejects([&] { actor::Producer bad(owners.game,owners.camera,basis,resources,capacities,bindings,bad_layout,other,actor::DetectorPolicy::disabled); },"old World actor overlap accepted");
    auto bad_other = other; bad_other[1] = {0x100000,0x10000};
    rejects([&] { actor::Producer bad(owners.game,owners.camera,basis,resources,capacities,bindings,layout,bad_other,actor::DetectorPolicy::disabled); },"old task actor overlap accepted");
    auto fb_layout = layout; fb_layout.lists = fb_layout.framebuffers[0];
    rejects([&] { actor::Producer bad(owners.game,owners.camera,basis,resources,capacities,bindings,fb_layout,other,actor::DetectorPolicy::disabled); },"framebuffer overlap accepted");
    rejects([&] { actor::Producer bad(owners.game,owners.camera,basis,resources,capacities,bindings,layout,other,
        static_cast<actor::DetectorPolicy>(1)); },"enabled/unadmitted detector policy silently ignored");
    scene::render::Pass pass; pass.camera = owners.camera; pass.camera_tag = UINT32_MAX;
    pass.link_mask = (std::uint64_t(1)<<4)|(std::uint64_t(1)<<5);
    pass.prepare = actor::Producer::prepare_pass; pass.userdata = &producer;
    rsp::CanonicalExecutor executor(true,{16000000,4*1024*1024});
    std::size_t arena_used = owners.arena.cursor;
    auto execute = [&](bool cold, std::uint64_t epoch = 1, rsp::CanonicalExecutor* alternate = nullptr) {
        auto& active = alternate ? *alternate : executor;
        require(active.quiescent(),"native mutation/preparation requires actual GPU/readback retirement");
        auto preparation = active.begin_preparation(); auto words = preparation.words();
        if (cold) {
            graphics::validate_cold_inputs(words,snapshot,snapshot.canonical().data(),boot);
            std::vector<beach::RamReservation> reservations{{0,graphics::bss_end}};
            for (auto r : producer.reservations()) reservations.push_back({r.offset,r.size});
            for (auto r : layout.framebuffers) reservations.push_back({r.offset,r.size});
            for (auto r : other) reservations.push_back({r.offset,r.size});
            resources.install(std::as_writable_bytes(words),reservations);
            graphics::install_cold_inputs(words,snapshot,boot);
            auto bytes=std::as_writable_bytes(words);
            for (const auto& span:common_render) {
                const auto offset=span.guest_address-0x80000000u;
                for (std::size_t i=0;i<span.bytes.size();++i) bytes[(offset+i)^3u]=span.bytes[i];
            }
            for (auto r : producer.reservations()) std::fill(words.begin()+r.offset/4,words.begin()+(r.offset+r.size)/4,0xdeadbeef);
            for (unsigned i = 0; i < 64; ++i) words[guard/4+i] = 0x13579bdf;
        }
        const IsleGameCameraParameters* light_camera=nullptr;
        native(isle_game_camera_parameters(owners.game,owners.camera,&light_camera));
        const auto& eye=light_camera->look_at.eye; const auto& light_at=light_camera->look_at.at;
        lighting.update_direction(std::as_writable_bytes(words),{eye[0],eye[1],eye[2]},{light_at[0],light_at[1],light_at[2]});
        const auto before = std::vector<std::uint32_t>(words.begin(),words.end());
        scene::render::Prepared frame; require(pass.prepare(pass.userdata,words,epoch,pass,frame),"actual compiled pass adapter");
        for (const auto& list : frame.lanes)
            require(list.words >= 2 && words[(list.address+list.words*4-8)/4] == 0xdf000000,"lane lacks real terminating EndDL");
        require(words[(frame.camera.address+frame.camera.words*4-8)/4] == 0xdf000000,"camera lacks real terminating EndDL");
        bool contextual_upload = false, inline48 = false;
        for (const auto& list : frame.lanes) for (std::size_t i = 0; i < list.words; i += 2) {
            const auto at = list.address/4+i;
            const auto opcode = words[at];
            require(opcode!=0xde000000 || words[at+1]!=0,"actual source NULL draw escaped into a call to RAM zero");
            inline48 |= (opcode&0xffffff00u) == 0xdb000000u;
            if ((opcode&0xfffffffeu) != 0xda380000u) continue;
            const auto address = words[at+1];
            for (std::size_t slot = 0; slot < live_slots; ++slot) {
                const auto base = layout.payloads.offset+256+slot*capacity.nodes*64;
                const auto native_nodes = producer.native_nodes(handles[slot]);
                if (address < base || address >= base+native_nodes.size()*64 || (address-base)%64) continue;
                const auto n = (address-base)/64;
                if (native_nodes[n].matrix_kind != ISLE_GAME_MODEL_CONTEXTUAL_RPY) continue;
                const IsleGameFixedMatrix zeros{};
                require(std::memcmp(&native_nodes[n].fixed,&zeros,sizeof(zeros)) == 0,"native contextual fixed must remain reserved zero");
                require(std::memcmp(words.data()+address/4,&zeros,sizeof(zeros)) != 0,"consumed source54 uploaded native zeros");
                contextual_upload = true;
            }
        }
        if (pass.camera_tag == UINT32_MAX && !bindings[0].source_flag100 && bindings[0].model.draw)
            require(contextual_upload && inline48,"actual source54/source48 outputs missing from GPU lists");
        if (pass.includes(bindings[2].render) && bindings[2].model.draw) {
            // The second1006 sibling consumes its predecessor's retained TIMG.
            // Both real roots must occur in source order on the actual lane1.
            const auto list = frame.lanes[1]; bool supplier = false, consumer = false;
            for (std::size_t i = 0; i < list.words; i += 2) if (words[list.address/4+i] == 0xde000000) {
                const auto target = words[list.address/4+i+1];
                if (target == 0x80139470) supplier = true;
                if (target == 0x80139558) { require(supplier,"1006 retained-TIMG consumer preceded real supplier"); consumer = true; }
            }
            require(supplier && consumer,"1006 retained-TIMG sibling roots missing from actual lane1");
        }
        for (std::size_t i = 0; i < words.size(); ++i) {
            const auto address = i*4;
            const bool owned = std::any_of(producer.reservations().begin(),producer.reservations().end(),
                [&](auto r) { return address >= r.offset && address < r.offset+r.size; });
            if (!owned) require(words[i] == before[i],"actor producer wrote outside exact RAM owners");
        }
        top_list(words,frame,world.world,lighting.pre_render_guest());
        const auto task_result = graphics::finish_task(words,snapshot,boot,stack,yield,task,output,output_end,top);
        auto completed = active.execute(std::move(preparation),task_result.task,task_result.boot);
        Capture result;
        const auto& packets = completed.packets();
        for (const auto packet : packets.packets()) {
            const auto data = packets.words().subspan(packet.offset,packet.count); const auto opcode = (data[0]>>24)&63;
            if (opcode >= 8 && opcode <= 15) result.triangles.insert(result.triangles.end(),data.begin(),data.end());
        }
        result.pixels.resize(plane_bytes); result.depths.resize(plane_bytes);
        for (std::size_t i = 0; i < plane_bytes; ++i) {
            result.pixels[i] = completed.rdram()[(color+i)^3]; result.depths[i] = completed.rdram()[(depth+i)^3];
        }
        for (unsigned i = 0; i < 64; ++i)
            require(reinterpret_cast<const std::uint32_t*>(completed.rdram().data())[guard/4+i] == 0x13579bdf,"actual RSP/RDP consumer changed guard");
        completed.release(); require(active.quiescent(),"actual completed-memory/GPU lease survived release");
        require(owners.arena.cursor == arena_used,"actor hot preparation allocated native storage");
        return result;
    };
    const auto initial = execute(true);
    save_capture(output_directory,"actual-actors-initial.ppm",initial);
    require(initial == execute(false),"unchanged actor frame changed actual packets/color/depth");
    bindings[3].model.draw = false; require(initial == execute(false),"geometryless1005 fabricated visible output"); bindings[3].model.draw = true;
    // Per-object source gate, actual native hidden flag, source links and tags.
    bindings[0].source_flag100 = true; const auto gated = execute(false);
    std::cout<<"actor_source_gate packets_changed="<<(gated.triangles!=initial.triangles)
        <<" depth_changed="<<(gated.depths!=initial.depths)<<" rgb5_changed_pixels="<<rgb5_changes(gated,initial)
        <<" initial_triangle_words="<<initial.triangles.size()<<" gated_triangle_words="<<gated.triangles.size()<<'\n';
    require(gated.triangles != initial.triangles && gated.depths != initial.depths &&
            rgb5_changes(gated,initial)!=0,"source flag100 gate did not suppress visible actual Snorlax");
    bindings[0].source_flag100 = false; require(initial == execute(false),"source flag100 reveal did not restore output");
    native(isle_game_object_set_flags(owners.game,bindings[1].model.object,1)); const auto hidden = execute(false);
    native(isle_game_object_set_flags(owners.game,bindings[1].model.object,0)); const auto revealed=execute(false);
    std::cout<<"pikachu_native_hide packets_changed="<<(hidden.triangles!=initial.triangles)
        <<" depth_changed="<<(hidden.depths!=initial.depths)<<" rgb5_changed_pixels="<<rgb5_changes(hidden,initial)
        <<" exact_reveal="<<(initial==revealed)<<'\n';
    require(hidden.triangles != initial.triangles && hidden.depths != initial.depths &&
        rgb5_changes(hidden,initial)!=0 && initial==revealed,"native hide/reveal failed actual visible Pikachu48 or retained stale residency");
    pass.camera_tag = 1u<<1; const auto tagged = execute(false);
    pass.tag_match = scene::render::TagMatch::equality; require(tagged == execute(false),"actual equality tag selection differs from matching intersection");
    pass.link_mask = std::uint64_t(1)<<4; const auto filtered = execute(false);
    require(tagged.triangles != filtered.triangles && tagged.depths != filtered.depths,"DL4/5 source pass filtering ignored tags/links");
    pass.tag_match = scene::render::TagMatch::intersection; pass.camera_tag = UINT32_MAX;
    pass.link_mask = (std::uint64_t(1)<<4)|(std::uint64_t(1)<<5);
    require(initial == execute(false),"source pass restoration changed retained actor output");
    // Source local mutation shares Runtime state; no per-frame pose copies.
    const auto original_local = runtime.model(handles[0]).nodes[0].transform; auto local = original_local;
    local.position.x += 180; require(bool(runtime.set_node_transform(handles[0],0,local)),"source local mutation rejected");
    const auto local_changed = execute(false);
    save_capture(output_directory,"actual-actors-local-moved.ppm",local_changed);
    require(local_changed.triangles != initial.triangles && local_changed.depths != initial.depths &&
            rgb5_changes(local_changed,initial)!=0,"source local mutation failed actual visible color/GPU/depth consumer");
    require(bool(runtime.set_node_transform(handles[0],0,original_local)),"source local restore");
    require(initial == execute(false),"source local restoration failed");
    const auto locals = std::vector<IsleGameModelNode>(producer.native_nodes(handles[0]).begin(),producer.native_nodes(handles[0]).end());
    auto placement = placements[0]; placement.position.x += 350; placement.rotation.y = 0.2f;
    native(isle_game_model_set_placement(owners.game,bindings[0].model.model,ISLE_GAME_MODEL_PLACEMENT_RTS,&placement));
    const auto placed = execute(false);
    require(placed.triangles != initial.triangles && placed.depths != initial.depths,"source native placement ignored by actual source54 hierarchy");
    const auto after_placement = producer.native_nodes(handles[0]);
    for (std::size_t n = 0; n < locals.size(); ++n)
        require(locals[n].revision == after_placement[n].revision && std::memcmp(&locals[n].transform,&after_placement[n].transform,sizeof(locals[n].transform)) == 0,
                "placement changed independent source locals");
    native(isle_game_model_set_placement(owners.game,bindings[0].model.model,ISLE_GAME_MODEL_PLACEMENT_RTS,&placements[0]));
    require(initial == execute(false),"native placement restore failed");
    const IsleGameCameraParameters* parameters = nullptr; native(isle_game_camera_parameters(owners.game,owners.camera,&parameters));
    const auto original_look = parameters->look_at; auto look = original_look; look.eye[0] += 700;
    native(isle_game_camera_set_look_at(owners.game,owners.camera,&look));
    const auto camera_changed = execute(false);
    require(camera_changed.triangles != initial.triangles && camera_changed.depths != initial.depths,"camera3/14/source48 change failed actual GPU/depth consumer");
    native(isle_game_camera_set_look_at(owners.game,owners.camera,&original_look)); require(initial == execute(false),"camera restore failed");
    // Admission failures use independent host RAM, not an abandoned real lease.
    std::vector<std::uint32_t> admission(beach::rdram_size/4,0x5a3cc3a5); const auto untouched = admission;
    const IsleGameCameraMatrix* camera0 = nullptr; const IsleGameCameraMatrix* camera1 = nullptr;
    native(isle_game_camera_matrix(owners.game,owners.camera,0,&camera0)); native(isle_game_camera_matrix(owners.game,owners.camera,1,&camera1));
    const IsleGameCameraReflection* reflection = nullptr; native(isle_game_camera_reflection(owners.game,owners.camera,1,&reflection));
    const auto saved_camera0 = *camera0, saved_camera1 = *camera1; const auto saved_reflection = *reflection;
    std::uint16_t normalization = 0; native(isle_game_camera_perspective_normalization(owners.game,owners.camera,&normalization));
    const auto retained = basis.value;
    const auto saved_world = std::vector<k::WorldMatrix>(producer.world_nodes(handles[0]).begin(),producer.world_nodes(handles[0]).end());
    const auto original_projection = parameters->perspective; auto projection = original_projection; projection.fovy += 1;
    native(isle_game_camera_set_perspective(owners.game,owners.camera,&projection));
    auto tight_layout = layout; tight_layout.lists.size = 104;
    actor::Producer tight(owners.game,owners.camera,basis,resources,capacities,bindings,tight_layout,other,actor::DetectorPolicy::disabled);
    rejects([&] { tight.prepare(admission,1); },"late EndDL/list capacity rejection accepted");
    require(tight.world_nodes(handles[0]).empty(),"rejected frame published a new world cache"); tight.retire();
    const auto saved_binding = bindings[5]; bindings[5].model.slot = capacities.size(); bindings[5].model.draw = false;
    rejects([&] { producer.prepare(admission,1,pass); },"hidden invalid slot escaped whole-frame admission"); bindings[5] = saved_binding;
    rejects([&] { producer.prepare(std::span(admission).first(admission.size()-1),1); },"short RAM accepted");
    rejects([&] { producer.prepare(admission,0); },"zero epoch accepted");
    require(admission == untouched && std::memcmp(camera0,&saved_camera0,sizeof(saved_camera0)) == 0 &&
        std::memcmp(camera1,&saved_camera1,sizeof(saved_camera1)) == 0 && std::memcmp(reflection,&saved_reflection,sizeof(saved_reflection)) == 0 &&
        std::memcmp(&basis.value,&retained,sizeof(retained)) == 0 &&
        std::memcmp(producer.world_nodes(handles[0]).data(),saved_world.data(),saved_world.size()*sizeof(k::WorldMatrix)) == 0,
        "failure partially published RAM/camera/reflection/basis/world cache");
    native(isle_game_camera_set_perspective(owners.game,owners.camera,&original_projection));
    auto distant_look = original_look; distant_look.eye[2] = 100000000.0f;
    native(isle_game_camera_set_look_at(owners.game,owners.camera,&distant_look));
    rejects([&] { producer.prepare(admission,1); },"unrepresentable native camera fixed result accepted");
    require(admission == untouched && std::memcmp(camera0,&saved_camera0,sizeof(saved_camera0)) == 0 &&
        std::memcmp(camera1,&saved_camera1,sizeof(saved_camera1)) == 0 &&
        std::memcmp(reflection,&saved_reflection,sizeof(saved_reflection)) == 0 &&
        std::memcmp(&basis.value,&retained,sizeof(retained)) == 0,
        "late camera batch failure published projection/reflection/RAM/basis");
    native(isle_game_camera_set_look_at(owners.game,owners.camera,&original_look));
    std::uint16_t after_norm = 0; native(isle_game_camera_perspective_normalization(owners.game,owners.camera,&after_norm));
    require(after_norm == normalization,"rejected preparation changed normalization");
    require(initial == execute(false),"rejected preparation corrupted subsequent actual output");
    // Pointer + epoch, retained1006 predecessor-TIMG, and hidden/reveal residency.
    require(initial == execute(true,2),"new epoch did not republish ordinary/contextual/material payloads");
    {
        rsp::CanonicalExecutor fresh(true,{16000000,4*1024*1024});
        require(initial == execute(true,3,&fresh),"new RAM pointer trusted old producer residency");
    }
    require(initial == execute(false,2),"return to canonical owner retained foreign residency");
    bindings[2].model.draw = false; const auto without_gate = execute(true,4);
    bindings[2].model.draw = true; require(initial == execute(false,4) && without_gate.triangles != initial.triangles &&
        rgb5_changes(without_gate,initial)!=0,"retained-texture1006 reveal failed visible actual RSP/RDP consumer");
    save_capture(output_directory,"actual-actors-without1006.ppm",without_gate);
    // Real retire/reuse: release only AFTER actual GPU completion/readback, and
    // join native deletion before releasing the Runtime incarnation.
    require(executor.quiescent(),"retirement preceded actual GPU release");
    const auto retired_handle = handles[4]; const auto stale = bindings[4];
    require(bool(runtime.begin_retire(retired_handle)),"real Runtime retire rejected");
    native(isle_game_delete_object(owners.game,bindings[4].model.object)); bindings[4] = {};
    require(bool(runtime.release_retired(retired_handle)),"real joined Runtime release rejected");
    execute(false,4); require(producer.world_nodes(retired_handle).empty(),"retired actor world borrow survived vacating slot");
    bindings[4] = stale; bindings[4].model.draw = false;
    rejects([&] { producer.prepare(admission,4); },"copied retired hidden view authorized reused native ownership"); bindings[4] = {};
    spawn(4); arena_used = owners.arena.cursor;
    require(handles[4] != retired_handle && initial == execute(false,4),"same-slot respawn reused stale incarnation/payloads");
    // Rebind the same explicit capacity slot across different immutable recipes,
    // with real native deletion/reacquisition, then restore the original resource.
    for (const auto replacement : std::array<std::size_t,2>{selected[0],selected[1]}) {
        require(executor.quiescent(),"resource rebind preceded GPU/readback release");
        require(bool(runtime.begin_retire(handles[4])),"resource rebind retire");
        native(isle_game_delete_object(owners.game,bindings[4].model.object)); bindings[4] = {};
        require(bool(runtime.release_retired(handles[4])),"resource rebind joined release");
        selected[4] = replacement; spawn(4); arena_used = owners.arena.cursor;
        const auto rebound = execute(false,4);
        if (replacement == selected[0])
            require(rebound.triangles != initial.triangles && rebound.depths != initial.depths,
                    "same-capacity Pikachu48 to Snorlax54 rebind retained old payloads");
        else require(rebound == initial,"resource rebind restoration retained old source54 topology/materials");
    }
    require(executor.quiescent(),"final native retirement before GPU/readback release"); producer.retire();
    for (std::size_t slot = 0; slot < live_slots; ++slot) {
        require(bool(runtime.begin_retire(handles[slot])),"final actor retire");
        native(isle_game_delete_object(owners.game,bindings[slot].model.object)); bindings[slot] = {};
        require(bool(runtime.release_retired(handles[slot])),"final actor joined release");
    }
    runtime.retire(); IsleGameCounts counts{}; native(isle_game_counts(owners.game,&counts));
    require(counts.objects == 1 && counts.models == 0 && counts.model_nodes == 0 && counts.processes == 0 && counts.matrices == 2,
            "actor retirement leaked native model/placement/process owners");
    std::cout << "actor_native_rsp_rdp=passed recipes=snorlax54_pikachu48_28 retained_texture1006=consumed geometryless1005=skipped "
                 "mutation=local_placement_camera admission=failure_atomic residency=pointer_epoch_hide_respawn detector_fsm_gameplay=not_claimed\n";
    return 0;
}
}
#ifdef _WIN32
int wmain(int argc, wchar_t** argv) {
#else
int main(int argc, char** argv) {
#endif
    try {
        if (argc == 2) return run(std::filesystem::path(argv[1]));
        if (argc == 3) return run(std::filesystem::path(argv[1]),std::filesystem::path(argv[2]));
        const char* path = std::getenv("PRIVATE_ROM");
        require(argc == 1 && path && *path,"usage: actor-producer-rsp-smoke PRIVATE_ROM [EMPTY_PRIVATE_CAPTURE_DIRECTORY], or set PRIVATE_ROM");
        return run(std::filesystem::path(path));
    } catch (const std::exception& error) { std::cerr << "private actor consumer: " << error.what() << '\n'; return 1; }
}
