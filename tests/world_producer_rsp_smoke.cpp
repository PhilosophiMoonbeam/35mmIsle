// Opt-in private-ROM component renderer consumer. NOT a Beach scene, player or
// actor implementation, and not an Original/gameplay parity claim. Main executes.
#include "world_producer.hpp"
#include "contextual_model_math.h"
#include "world_movement.hpp"
#include "graphics_task.hpp"
#include "rsp_render.hpp"
#include "effect_bank_resources.hpp"
#include "menu_effects.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <filesystem>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <utility>
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
    menu::EffectRuntime& effects;
    explicit Owners(menu::EffectRuntime& pool) : effects(pool) {
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
    static bool command(void* p,IsleGameObject*) noexcept { ++static_cast<Owners*>(p)->commands; return true; }
    static bool shift_camera(void* p,world::Vec3 delta) noexcept {
        auto& o=*static_cast<Owners*>(p); const IsleGameCameraParameters* old=nullptr;
        if (isle_game_camera_parameters(o.game,o.camera,&old)!=ISLE_GAME_OK) return false;
        auto look=old->look_at; look.eye[0]+=delta.x;look.eye[1]+=delta.y;look.eye[2]+=delta.z;
        look.at[0]+=delta.x;look.at[1]+=delta.y;look.at[2]+=delta.z;
        if (isle_game_camera_set_look_at(o.game,o.camera,&look)!=ISLE_GAME_OK) return false;
        ++o.shifts;
        return true;
    }
    static bool shift_effects(void* p,world::Vec3 d) noexcept {
        auto& o=*static_cast<Owners*>(p);
        o.effects.translate({d.x,d.y,d.z});
        if (!o.effects.status()) return false;
        ++o.effect_shifts;
        return true;
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
struct Result { std::vector<std::uint32_t> triangles;std::vector<std::byte> pixels,depths; };
int run(const std::filesystem::path& path) {
    const auto snapshot=rom::read_snapshot(path);const world::Resources resources(snapshot);
    const menu::EffectResources effect_resources(snapshot);
    const std::array effect_banks{&effect_resources.bank(menu::EffectGroup::common),
        &effect_resources.bank(menu::EffectGroup::beach)};
    isle_rng effect_rng{0x12345678};
    menu::EffectRuntime effects(effect_banks,effect_rng);
    require(bool(effects.status()),"actual common3/Beach0 effect bank admission failed");
    const auto initial_effect=effects.create_particle(3,0,{1,2,3},{});
    require(bool(initial_effect.status)&&initial_effect.spawned,"real common-bank particle did not remain live");
    Owners owners(effects);
    const auto view=world::ResourceView{&resources.setup(),&resources.sky(),resources.blocks(),resources.primary_count(),resources.models(),
        resources.materials(),resources.animations(),resources.paths(),resources.static_definitions(),&resources.height_map(),resources.collision_models()};
    world::Callbacks callbacks;callbacks.user=&owners;callbacks.shift_camera=Owners::shift_camera;
    callbacks.shift_effects=Owners::shift_effects;callbacks.process_commands=Owners::command;
    world::World component(owners.game,view,callbacks);component.follow_camera(owners.camera);
    const auto source_terrain_pose=[&]{
        for(const auto& block:component.blocks())if(block.model){
            const IsleGameModelNode* nodes=nullptr;std::size_t count=0;
            native(isle_game_model_nodes(owners.game,block.model,&nodes,&count));
            const auto& root=nodes[0].transform;
            require(resources.models()[block.resource->model].matrix_kinds==std::array<std::uint8_t,3>{1,0,0}&&
                nodes[0].matrix_kind==ISLE_GAME_MODEL_TRANSLATE&&root.rotation.x==0&&
                root.rotation.y==block.resource->metadata.yaw&&root.rotation.z==0&&
                root.scale.x==1&&root.scale.y==1&&root.scale.z==1,
                "source terrain initial yaw/unit scale lost or render recipe promoted to RTS");
            const auto runtime=component.animation_model(block.instance).nodes[0].transform;
            require(runtime.rotation.y==block.resource->metadata.yaw&&runtime.scale.x==1&&
                runtime.scale.y==1&&runtime.scale.z==1,"source terrain Runtime lost per-instance canonical yaw");
            const auto origin=component.current()?component.current()->resource->metadata.world_position:
                block.resource->metadata.world_position;
            const auto position=block.resource->metadata.world_position;
            require(root.position.x==(position.x-origin.x)*100.0f&&
                root.position.y==(position.y-origin.y)*100.0f&&root.position.z==(position.z-origin.z)*100.0f,
                "source terrain canonical yaw changed block-relative position");
        }
    };
    source_terrain_pose();
    IsleWorldMovementState movement_state = ISLE_WORLD_MOVEMENT_STATE_INITIALIZER;world::Movement movement(component,movement_state,{});require(movement.initialize(0),"real road admission");
    source_terrain_pose();
    const std::array<menu::RamRange,6> other{{{top,0x10000},{stack,0x10000},{output,output_end-output},
        {color,target_bytes},{depth,target_bytes},{guard,256}}};
    menu::model::CameraBasisState camera_basis;
    world::Producer producer(owners.game,owners.camera,camera_basis,resources,component,{},other);
    rsp::CanonicalExecutor executor(true,{16000000,4*1024*1024});
    const auto arena_used=owners.arena.cursor;
    auto execute=[&](bool cold,std::uint64_t epoch=1,rsp::CanonicalExecutor* alternate=nullptr){
        auto& active_executor=alternate ? *alternate : executor;
        auto preparation=active_executor.begin_preparation();auto words=preparation.words();
        if(cold){
            graphics::validate_cold_inputs(words,snapshot,snapshot.canonical().data(),boot);
            std::vector<beach::RamReservation> reserved;reserved.reserve(other.size()+producer.reservations().size()+1);
            reserved.push_back({0,graphics::bss_end});for(auto r:other)reserved.push_back({r.offset,r.size});
            for(auto r:producer.reservations())reserved.push_back({r.offset,r.size});
            resources.install(std::as_writable_bytes(words),reserved);graphics::install_cold_inputs(words,snapshot,boot);
            for(unsigned i=0;i<64;++i)words[guard/4+i]=0x13579bdf;
            // Fresh canonical memory/epoch must not depend on equal native
            // revisions or old payload bytes surviving in the owner ranges.
            for(auto r:producer.reservations())
                std::fill(words.begin()+r.offset/4,words.begin()+(r.offset+r.size)/4,0xdeadbeefu);
        }
        const auto before=std::vector<std::uint32_t>(words.begin(),words.end());
        bool rejected=false;try{producer.prepare(words.first(words.size()-1),epoch);}catch(const std::exception&){rejected=true;}
        require(rejected&&std::equal(words.begin(),words.end(),before.begin()),"invalid preparation modified RAM");
        std::vector<std::pair<IsleGameModel*,IsleGameModelTransform>> terrain_before;
        for(const auto& block:component.blocks())if(block.model){
            const IsleGameModelNode* nodes=nullptr;std::size_t count=0;
            native(isle_game_model_peek_nodes(owners.game,block.model,&nodes,&count));
            terrain_before.emplace_back(block.model,nodes[0].transform);
        }
        scene::render::Pass update_link_only;
        update_link_only.camera=owners.camera;update_link_only.link_mask=std::uint64_t(1)<<9;
        const auto filtered=producer.prepare(words,epoch,update_link_only);
        for(const auto lane:filtered.lanes) require(lane.commands==1,"update link9 was incorrectly treated as renderDL3");
        const auto frame=producer.prepare(words,epoch);
        for(const auto& [model,pose]:terrain_before){
            const IsleGameModelNode* nodes=nullptr;std::size_t count=0;
            native(isle_game_model_nodes(owners.game,model,&nodes,&count));
            require(std::memcmp(&nodes[0].transform,&pose,sizeof(pose))==0,
                "World producer preparation normalized or rewrote full native terrain pose");
            const std::array translation{pose.position.x,pose.position.y,pose.position.z};
            for(unsigned row=0;row<4;++row)for(unsigned col=0;col<4;++col){
                const auto pair=row*2+col/2,shift=col%2?0u:16u;
                const auto bits=(((nodes[0].fixed.m[pair/4][pair%4]>>shift)&65535u)<<16)|
                    ((nodes[0].fixed.m[2+pair/4][pair%4]>>shift)&65535u);
                const auto expected=row==3&&col<3?static_cast<std::int32_t>(translation[col]*65536.0f):
                    (row==col?65536:0);
                require(std::bit_cast<std::int32_t>(bits)==expected,
                    "World source terrain yaw/scale entered translation-only fixed output");
            }
        }
        const IsleGameCameraParameters* context_camera=nullptr;
        native(isle_game_camera_parameters(owners.game,owners.camera,&context_camera));
        menu::model::WorldMatrix expected_basis;
        native(isle_model_source48_basis(context_camera,camera_basis.value.perspective.m,1,expected_basis.m));
        require(std::memcmp(&expected_basis,&camera_basis.value.basis48,sizeof(expected_basis))==0,
                "World camera3/14 did not publish its retained source-derived basis");
        for(std::size_t i=0;i<words.size();++i){const auto address=i*4;
            const bool owned=std::any_of(frame.reservations.begin(),frame.reservations.end(),[&](menu::RamRange r){return address>=r.offset&&address<r.offset+r.size;});
            if(!owned)require(words[i]==before[i],"world producer wrote outside its exact owners");}
        top_list(words,frame);const auto task_result=graphics::finish_task(words,snapshot,boot,stack,yield,task,output,output_end,top);
        auto completed=active_executor.execute(std::move(preparation),task_result.task,task_result.boot);Result result;
        const auto& packets=completed.packets();for(const auto packet:packets.packets()){
            const auto data=packets.words().subspan(packet.offset,packet.count);const auto opcode=(data[0]>>24)&63;
            if(opcode>=8&&opcode<=15)result.triangles.insert(result.triangles.end(),data.begin(),data.end());}
        result.pixels.resize(target_bytes);for(std::size_t i=0;i<target_bytes;++i)result.pixels[i]=completed.rdram()[(color+i)^3];
        result.depths.resize(target_bytes);for(std::size_t i=0;i<target_bytes;++i)result.depths[i]=completed.rdram()[(depth+i)^3];
        for(unsigned i=0;i<64;++i)require(reinterpret_cast<const std::uint32_t*>(completed.rdram().data())[guard/4+i]==0x13579bdf,"actual consumer changed guard");
        completed.release();require(active_executor.quiescent(),"GPU borrow not retired");require(owners.arena.cursor==arena_used,"world frame allocated native storage");return result;
    };
    native(isle_game_tick(owners.game));source_terrain_pose();
    const auto initial=execute(true);source_terrain_pose();require(!initial.triangles.empty(),"world canonical task emitted no geometry");
    // Raw native R/S may change independently of Runtime without changing
    // translation-only terrain rendering or being normalized by preparation.
    IsleGameModel* raw_terrain=nullptr;IsleGameModelTransform original_terrain{};
    for(const auto& block:component.blocks())if(block.model){
        IsleGameObjectMetadata metadata{};native(isle_game_object_metadata(owners.game,block.object,&metadata));
        if(metadata.flags&1u)continue;
        const IsleGameModelNode* nodes=nullptr;std::size_t count=0;
        native(isle_game_model_nodes(owners.game,block.model,&nodes,&count));
        raw_terrain=block.model;original_terrain=nodes[0].transform;break;
    }
    require(raw_terrain,"visible terrain owner missing from source regression");
    auto raw_pose=original_terrain;raw_pose.rotation={0.25f,-0.75f,1.25f};raw_pose.scale={-2,3,0.5f};
    native(isle_game_model_set_transform(owners.game,raw_terrain,0,&raw_pose));
    const auto raw_rendered=execute(false);
    require(raw_rendered.triangles==initial.triangles&&raw_rendered.pixels==initial.pixels&&
        raw_rendered.depths==initial.depths,"canonical terrain R/S incorrectly rotated/scaled rendered geometry");
    native(isle_game_model_set_transform(owners.game,raw_terrain,0,&original_terrain));
    execute(false);source_terrain_pose();
    // Freeze animation: placement-only RT/RTS edits must reach both native
    // hierarchy and direct sky rendering without rewriting their ordinary roots.
    const auto objects=component.render_objects();
    IsleGameModel* hierarchy=nullptr;IsleGameModel* sky=nullptr;
    struct LocalSnapshot { IsleGameModel* model;std::vector<IsleGameModelNode> nodes; };
    std::vector<LocalSnapshot> locals;
    std::size_t local_payload_bytes=256;
    for(const auto& object:objects){
        const IsleGameModelNode* nodes=nullptr;std::size_t count=0;
        native(isle_game_model_nodes(owners.game,object.model,&nodes,&count));
        locals.push_back({object.model,{nodes,nodes+count}});local_payload_bytes+=count*64;
        if(object.sky)sky=object.model;
        else if(!hierarchy&&count>1){
            IsleGameObjectMetadata metadata{};native(isle_game_object_metadata(owners.game,object.object,&metadata));
            if(!(metadata.flags&1u))hierarchy=object.model;
        }
    }
    require(hierarchy&&sky,"real hierarchy/sky placement owners missing");
    auto unchanged_locals=[&]{
        for(const auto& saved:locals){
            const IsleGameModelNode* nodes=nullptr;std::size_t count=0;
            native(isle_game_model_nodes(owners.game,saved.model,&nodes,&count));
            require(count==saved.nodes.size(),"placement changed source node indices");
            for(std::size_t n=0;n<count;++n)
                require(nodes[n].revision==saved.nodes[n].revision&&
                        std::memcmp(&nodes[n].transform,&saved.nodes[n].transform,sizeof(nodes[n].transform))==0&&
                        std::memcmp(&nodes[n].fixed,&saved.nodes[n].fixed,sizeof(nodes[n].fixed))==0,
                        "placement modified ordinary local pose/fixed revision");
        }
    };
    const auto same=[](const Result& a,const Result& b){
        return a.triangles==b.triangles&&a.pixels==b.pixels&&a.depths==b.depths;
    };
    IsleGameCounts baseline_counts{};native(isle_game_counts(owners.game,&baseline_counts));
    const IsleGameModelTransform terrain_placement{{400,200,-300},{0.08f,0.12f,-0.05f},{1.1f,0.9f,1.2f}};
    const IsleGameModelTransform sky_placement{{1500,0,-500},{0,0.15f,0},{1,1,1}};
    native(isle_game_model_set_placement(owners.game,hierarchy,ISLE_GAME_MODEL_PLACEMENT_RTS,&terrain_placement));
    unchanged_locals();const auto placed_hierarchy=execute(false);
    require(placed_hierarchy.triangles!=initial.triangles&&placed_hierarchy.depths!=initial.depths,
            "placement-only hierarchy change did not reach actual geometry/depth");
    native(isle_game_model_set_placement(owners.game,hierarchy,ISLE_GAME_MODEL_PLACEMENT_RT,&terrain_placement));
    const auto rt_hierarchy=execute(false);
    require(!same(placed_hierarchy,rt_hierarchy),"RT/RTS mode change without tick retained stale world output");
    native(isle_game_model_set_placement(owners.game,hierarchy,ISLE_GAME_MODEL_PLACEMENT_RTS,&terrain_placement));
    require(same(placed_hierarchy,execute(false)),"RTS restoration without tick changed output");
    auto moved_terrain=terrain_placement;moved_terrain.position.x+=350;moved_terrain.rotation.y-=0.2f;
    native(isle_game_model_set_placement(owners.game,hierarchy,ISLE_GAME_MODEL_PLACEMENT_RTS,&moved_terrain));
    require(!same(placed_hierarchy,execute(false)),"same-mode placement revision without tick retained stale output");
    native(isle_game_model_set_placement(owners.game,hierarchy,ISLE_GAME_MODEL_PLACEMENT_RTS,&terrain_placement));
    require(same(placed_hierarchy,execute(false)),"same-mode placement restore did not restore output");
    native(isle_game_model_set_placement(owners.game,sky,ISLE_GAME_MODEL_PLACEMENT_RT,&sky_placement));
    unchanged_locals();const auto placed_both=execute(false);
    native(isle_game_model_set_placement(owners.game,sky,ISLE_GAME_MODEL_PLACEMENT_DISABLED,nullptr));
    require(same(placed_hierarchy,execute(false)),"active hierarchy payload compaction retained stale residency");
    native(isle_game_model_set_placement(owners.game,sky,ISLE_GAME_MODEL_PLACEMENT_RT,&sky_placement));
    require(same(placed_both,execute(false)),"placement topology restoration without tick changed output");
    require(placed_both.triangles!=placed_hierarchy.triangles,"direct sky branch ignored native placement");
    // While hierarchy is hidden, compact it away from and back onto its old
    // slot, letting a later owner overwrite that slot. Re-show must republish,
    // not trust the pre-hide address/revision pair.
    IsleGameObject* hierarchy_object=nullptr;IsleGameModel* later_model=nullptr;
    for(const auto& object:objects){
        if(object.model==hierarchy)hierarchy_object=object.object;
        else if(hierarchy_object&&!later_model){
            IsleGameObjectMetadata metadata{};native(isle_game_object_metadata(owners.game,object.object,&metadata));
            if(!(metadata.flags&1u))later_model=object.model;
        }
    }
    require(hierarchy_object&&later_model,"real hidden-placement compaction owners missing");
    IsleGameObjectMetadata hierarchy_metadata{};
    native(isle_game_object_metadata(owners.game,hierarchy_object,&hierarchy_metadata));
    native(isle_game_object_set_flags(owners.game,hierarchy_object,hierarchy_metadata.flags|1u));
    const IsleGameModelTransform neutral_placement{{0,0,0},{0,0,0},{1,1,1}};
    native(isle_game_model_set_placement(owners.game,sky,ISLE_GAME_MODEL_PLACEMENT_DISABLED,nullptr));
    native(isle_game_model_set_placement(owners.game,later_model,ISLE_GAME_MODEL_PLACEMENT_RT,&neutral_placement));
    execute(false);
    native(isle_game_model_set_placement(owners.game,sky,ISLE_GAME_MODEL_PLACEMENT_RT,&sky_placement));
    native(isle_game_model_set_placement(owners.game,later_model,ISLE_GAME_MODEL_PLACEMENT_DISABLED,nullptr));
    execute(false);
    native(isle_game_object_set_flags(owners.game,hierarchy_object,hierarchy_metadata.flags));
    require(same(placed_both,execute(false)),"hidden topology churn reused overwritten placement payload");
    IsleGameCounts active_counts{};native(isle_game_counts(owners.game,&active_counts));
    require(active_counts.matrices==baseline_counts.matrices+2&&active_counts.model_nodes==baseline_counts.model_nodes,
            "active world placements changed node indices or matrix count");
    // One extra64-byte slot succeeds; two active placements exceed it before
    // canonical RAM or any native camera/model publication can be changed.
    auto tight_layout=world::ProducerLayout{};tight_layout.payloads.size=local_payload_bytes+64;
    world::Producer tight(owners.game,owners.camera,camera_basis,resources,component,tight_layout,other);
    {
        // This is a CPU admission boundary, not an execution. Never acquire a
        // canonical preparation lease that would be abandoned on scope exit:
        // its fail-closed destructor must poison that executor.
        std::vector<std::uint32_t> admission_ram(beach::rdram_size/4,0x5a3cc3a5u);
        const auto words=std::span<std::uint32_t>(admission_ram);
        const auto before=std::vector<std::uint32_t>(words.begin(),words.end());
        const IsleGameCameraMatrix* camera_before=nullptr;native(isle_game_camera_matrix(owners.game,owners.camera,0,&camera_before));
        const auto camera_revision=camera_before->revision;
        const auto camera_fixed=camera_before->fixed;
        const IsleGameCameraParameters* camera_parameters=nullptr;
        native(isle_game_camera_parameters(owners.game,owners.camera,&camera_parameters));
        const auto original_perspective=camera_parameters->perspective;
        auto dirty_perspective=original_perspective;dirty_perspective.fovy+=1;
        native(isle_game_camera_set_perspective(owners.game,owners.camera,&dirty_perspective));
        const auto retained_before=camera_basis.value;
        bool rejected=false;try{tight.prepare(words,1);}catch(const std::exception&){rejected=true;}
        require(rejected&&std::equal(words.begin(),words.end(),before.begin())&&camera_before->revision==camera_revision&&
                std::memcmp(&camera_fixed,&camera_before->fixed,sizeof(camera_fixed))==0&&
                std::memcmp(&retained_before,&camera_basis.value,sizeof(retained_before))==0,
                "placement capacity rejection partially published native/RAM/retained-camera state");
        native(isle_game_camera_set_perspective(owners.game,owners.camera,&original_perspective));
        native(isle_game_model_set_placement(owners.game,sky,ISLE_GAME_MODEL_PLACEMENT_DISABLED,nullptr));
        tight.prepare(words,1); // Exact disabled-local budget plus one active64B.
    }
    tight.retire();
    // Independently exercise canonical epoch residency with poisoned owner
    // bytes; equal native placement revisions must still republish payloads.
    native(isle_game_model_set_placement(owners.game,sky,ISLE_GAME_MODEL_PLACEMENT_RT,&sky_placement));
    require(same(placed_both,execute(true,2)),"new epoch did not republish active world placement");
    {
        rsp::CanonicalExecutor fresh_executor(true,{16000000,4*1024*1024});
        require(same(placed_both,execute(true,3,&fresh_executor)),"new canonical pointer retained stale placement residency");
        require(fresh_executor.quiescent(),"fresh-memory execution lease survived completion");
    }
    native(isle_game_model_set_placement(owners.game,hierarchy,ISLE_GAME_MODEL_PLACEMENT_DISABLED,nullptr));
    const auto placed_sky=execute(false,2);
    require(placed_sky.triangles!=initial.triangles,"sky-only active placement was lost during payload compaction");
    native(isle_game_model_set_placement(owners.game,sky,ISLE_GAME_MODEL_PLACEMENT_DISABLED,nullptr));
    unchanged_locals();require(same(initial,execute(false,2)),"placement disable did not restore exact baseline");
    native(isle_game_counts(owners.game,&active_counts));
    require(active_counts.matrices==baseline_counts.matrices&&active_counts.model_nodes==baseline_counts.model_nodes,
            "placement disable retained native matrix ownership");
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
        menu::EffectParticleView before_effect{};
        require(effects.particle(0,before_effect),"real effect particle disappeared before World entry");
        const auto old_world=component.current()->resource->metadata.world_position;
        const auto rng_before=effect_rng.seed;
        auto* block=component.enter_next();
        require(block==&component.blocks()[i],"complete primary chain transition failed");
        source_terrain_pose();
        const auto next_world=block->resource->metadata.world_position;
        const menu::Vec3 displacement{(old_world.x-next_world.x)*100.0f,
            (old_world.y-next_world.y)*100.0f,(old_world.z-next_world.z)*100.0f};
        menu::EffectParticleView after_effect{};
        require(effects.particle(0,after_effect)&&after_effect.effect_id==before_effect.effect_id&&
            after_effect.position.x==before_effect.position.x+displacement.x&&
            after_effect.position.y==before_effect.position.y+displacement.y&&
            after_effect.position.z==before_effect.position.z+displacement.z&&
            after_effect.velocity.x==before_effect.velocity.x&&
            after_effect.velocity.y==before_effect.velocity.y&&
            after_effect.velocity.z==before_effect.velocity.z&&effect_rng.seed==rng_before,
            "World primary entry did not translate the real source effect particle without advancing physics/RNG");
        if(block->road_model&&block->resource->metadata.movement_animation_guest)component.reset_road(*block,0);
        movement_state.block=block;movement_state.move_time=0.5f;
        require(movement.update_position(*block,movement_state.move_time,movement_state.control_time),"real road position sample failed");
        require(component.current()==block&&movement_state.block==block,"road sampling changed primary chain ownership");
        require(owners.shifts==i&&owners.effect_shifts==i,"native camera/effect translation did not follow each primary entry");
        native(isle_game_tick(owners.game));source_terrain_pose();execute(false);source_terrain_pose();
    }
    // Retire an enabled World-owned placement only after the actual completed
    // lease has been released; removal must subtract that matrix exactly once.
    native(isle_game_model_set_placement(owners.game,sky,ISLE_GAME_MODEL_PLACEMENT_RTS,&sky_placement));
    execute(false);
    producer.retire();require(component.retire(),"World producer component retirement failed");IsleGameCounts counts{};native(isle_game_counts(owners.game,&counts));
    require(counts.objects==1&&counts.models==0&&counts.processes==0&&counts.matrices==2,"world retirement retained native resources");
    effects.reset();
    require(effects.particle_count()==0&&effects.effect_count()==0,"World effect pool survived joined GPU teardown");
    std::cout<<"world_component_rsp_rdp=passed primary=8 secondary=4 placement=hierarchy_sky_rt_rts_restore_fresh visibility=hide_restore "
        "common3_Beach0_real_effect_particle_translation=passed native_retirement=joined combined_FX_render_gameplay=not_claimed\n";return 0;
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
