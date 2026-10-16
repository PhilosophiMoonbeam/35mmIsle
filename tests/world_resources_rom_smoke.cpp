#include "world_resources.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <utility>

// Opt-in original-ROM input only. No capture, executable guest callback,
// extracted payload or expected rendered image is used by this resource test.
namespace w = isle::world;
namespace m = isle::menu;
namespace {
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
template<class F> void rejects(F&& action, const char* message) {
    bool rejected = false;
    try { action(); } catch (const std::exception&) { rejected = true; }
    require(rejected,message);
}
void run(const char* filename) {
    auto snapshot = isle::rom::read_snapshot(filename);
    const w::Resources resources(snapshot);
    constexpr std::array<std::uint32_t,12> descriptors{
        0x80135c64,0x80135c88,0x80135cac,0x80135cd0,0x80135cf4,0x80135d18,0x80135d3c,0x80135d60,
        0x80135bd4,0x80135bf8,0x80135c1c,0x80135c40};
    constexpr std::array<std::uint32_t,12> roots{
        0x8011cf00,0x80122bd0,0x80123c60,0x80125580,0x80128a40,0x8012a3c0,0x8012ce40,0x8012e7c0,
        0x80131480,0x80131c10,0x801320e0,0x80132c30};
    constexpr std::array<std::size_t,12> spawns{7,1,7,5,7,4,4,2,0,0,0,0};
    constexpr std::array<std::size_t,12> decorations{2,1,0,0,0,0,0,0,0,0,0,0};
    constexpr std::array<std::size_t,12> collision_objects{3,1,0,0,0,0,0,1,0,0,0,0};
    require(resources.primary_count()==8 && resources.blocks().size()==12,"Exact eight primary/four secondary Beach groups required");
    require(resources.models().size()==21 && resources.materials().size()==6 && resources.animations().size()==22 &&
            resources.paths().size()==33,"Complete native model/material/animation/path closure required");
    require(resources.static_definitions().size()==1 && resources.static_definitions()[0].id==1007 &&
            resources.static_definitions()[0].callback_identity==0x800e30b0 &&
            resources.static_definitions()[0].payload_identity==0x80138c80,"Exact GAME decoration callback/payload required");
    std::size_t nodes=0, material_attachments=0, model_streams=0, texture_streams=0, path_commands=0;
    for (std::size_t i=0;i<resources.blocks().size();++i) {
        const auto& block=resources.blocks()[i];
        require(block.metadata.wire.guest_address==descriptors[i] && block.metadata.wire.bytes.size()==36 &&
                block.metadata.display_list_guest==roots[i],"Wrong descriptor group order/root identity");
        require(block.model<resources.models().size(),"Missing consumer-ready terrain model");
        const auto& model=resources.models()[block.model];
        require(model.nodes.size()==1+decorations[i] && model.nodes[0].payloads.size()==1 &&
                model.nodes[0].payloads[0].draw_list==roots[i],"Native terrain/decorations not complete");
        require(model.render_type==(decorations[i] ? m::RenderType::b_fogged : m::RenderType::a_fogged),
                "Source A-to-B render conversion applies only to decorated blocks");
        require(block.spawns.size()==spawns[i] && block.decorations.size()==decorations[i] &&
                block.collision_objects.size()==collision_objects[i],"Wrong source spawn/static closure");
        for (const auto& spawn:block.spawns) {
            require(spawn.wire.bytes.size()==48 && spawn.wire.u32(0)==static_cast<std::uint32_t>(spawn.id),
                    "ObjectSpawn must use GAME 0x30 stride, not StaticObject layout");
            require((spawn.path==w::no_index)==(spawn.wire.u32(44)==0),"Spawn path null binding mismatch");
            if (spawn.path!=w::no_index)
                require(spawn.path<resources.paths().size() && resources.paths()[spawn.path].wire.guest_address==spawn.wire.u32(44),
                        "Spawn path native binding mismatch");
        }
        for (std::size_t j=0;j<block.decorations.size();++j) {
            const auto& source=block.decorations[j]; const auto& node=model.nodes[j+1];
            require(node.parent==0 && node.position.x==source.position.x*100 && node.position.y==source.position.y*100 &&
                    node.position.z==source.position.z*100 && node.rotation.y==source.rotation.y && node.scale.x==source.scale.x,
                    "Decoration native payload loses source parent/transform law");
        }
        if (i<8) {
            require(block.road_model<resources.models().size() && block.metadata.movement_animation_duration==100,
                    "Every primary block requires complete road/movement animation");
            const auto& road=resources.models()[block.road_model];
            require(road.render_type==m::RenderType::b && road.nodes.size()==2 && road.tree.bytes.size()==132 &&
                    road.tree.u32(88)==18 && road.animation_table.bytes.size()==8,"Exact two-node source road tree/END required");
            for (const auto& node:road.nodes)
                require(node.animation<resources.animations().size(),"Missing road model stream");
        } else require(block.road_model==w::no_index,"Secondary descriptor must not invent a road");
    }
    for (const auto& model:resources.models()) for (std::size_t i=0;i<model.nodes.size();++i) {
        const auto& node=model.nodes[i]; ++nodes; material_attachments+=node.materials.size();
        require(node.parent==w::no_index || node.parent<i,"Native hierarchy is not parent-before-child");
        for (const auto material:node.materials) require(material<resources.materials().size(),"Material native index out of range");
        for (const auto animation:node.material_animations)
            require(animation<resources.animations().size(),"Material animation native index out of range");
    }
    require(nodes==32 && material_attachments==6,"Native world model tree or material attachments missing");
    for (const auto& animation:resources.animations()) {
        if (animation.kind==m::AnimationKind::model) ++model_streams; else ++texture_streams;
        require(!animation.commands.empty() && animation.commands.back().opcode==0,"Exact END-inclusive animation closure missing");
        std::size_t bytes=0;
        for (const auto& command:animation.commands) {
            require(command.wire.guest_address==animation.wire.guest_address+bytes,"Animation operand framing is not contiguous");
            bytes+=command.wire.bytes.size();
            if (command.opcode==13) {
                ++path_commands;
                require(command.path_index<resources.paths().size() &&
                        resources.paths()[command.path_index].wire.guest_address==command.wire.u32(4),"Opcode13 native path missing");
            } else require(command.path_index==w::no_index,"Invented path binding on nonpath opcode");
        }
        require(bytes==animation.wire.bytes.size(),"Animation stream wire includes unrelated bytes");
    }
    require(model_streams==16 && texture_streams==6 && path_commands==8,"Road and all six material streams required");
    for (const auto& path:resources.paths()) {
        const auto count=path.segment_point_count;
        require(path.kind==2 && path.wire.bytes.size()==24 && count>=2 && path.points.size()==count+2 &&
                path.timestamps.size()==count && path.speed_polynomials.size()==(count-1)*5 &&
                path.timestamps.front()==0 && path.timestamps.back()==1,"GAME path spline guard points/times/polynomials incomplete");
    }
    const auto& height=resources.height_map();
    require(height.patches.size()==6386 && height.nodes.size()==6385,"Full source height patch/tree arrays required");
    for (const auto& node:height.nodes) {
        require((node.left_child==w::no_index || node.left_child<height.nodes.size()) &&
                (node.right_child==w::no_index || node.right_child<height.nodes.size()) &&
                (node.left_patch==w::no_index || node.left_patch<height.patches.size()) &&
                (node.right_patch==w::no_index || node.right_patch<height.patches.size()),"Unrelocated/invalid height index");
    }
    const auto collisions=resources.collision_models();
    require(collisions.size()==3 && collisions[0].id==1001 && collisions[0].colliders.size()==8 &&
            collisions[1].id==1004 && collisions[1].colliders.size()==13 &&
            collisions[2].id==1007 && collisions[2].colliders.size()==6 && collisions[2].scale==-1,
            "Default common collider plus both Beach collider trees required");
    std::array<bool,5> hitbox_types{};
    for (const auto& model:collisions) for (const auto& collider:model.colliders)
        if (collider.has_hitbox) { require(collider.hitbox.type<5,"Unknown hitbox type"); hitbox_types[collider.hitbox.type]=true; }
    require(hitbox_types[1] && hitbox_types[2] && hitbox_types[3] && hitbox_types[4],"All four source hitbox data kinds required");
    require(resources.display_lists().size()==14 && resources.vertex_loads().size()==315 &&
            resources.dynamic_material_calls().size()==6,"Full static/dynamic GBI reference closure missing");
    std::size_t commands=0,vertices=0;
    for (const auto& list:resources.display_lists()) commands+=list.wire.bytes.size()/8;
    for (const auto& load:resources.vertex_loads()) vertices+=load.count;
    require(commands==3064 && vertices==3838,"Exact original GBI/list/vertex extents required");
    const auto spans=resources.installation_spans();
    require(spans.size()==94,"Exact merged referenced-asset union required");
    std::size_t total=0; std::uint64_t previous_end=0;
    for (const auto& span:spans) {
        require(span.guest_address>previous_end && span.guest_address>=0x8011b050 &&
                static_cast<std::uint64_t>(span.guest_address)+span.bytes.size()<=0x801b02f0 &&
                span.rom_offset==0xa74e0+span.guest_address-0x8011b050,"Installation includes unrelated metadata/code/gaps");
        previous_end=static_cast<std::uint64_t>(span.guest_address)+span.bytes.size(); total+=span.bytes.size();
    }
    require(total==164936,"Exact installation byte union required");
    std::vector<std::byte> ram(isle::beach::rdram_size,std::byte{0x5a});
    const auto untouched=ram;
    const auto& last=spans.back();
    const std::array<isle::beach::RamReservation,1> conflict{{{last.guest_address-0x80000000,last.bytes.size()}}};
    rejects([&]{resources.install(ram,conflict);},"Late reservation collision accepted");
    require(ram==untouched,"Reservation rejection partially installed earlier resources");
    const std::array<isle::beach::RamReservation,1> invalid{{{isle::beach::rdram_size,1}}};
    rejects([&]{resources.install(ram,invalid);},"Invalid reservation extent accepted");
    require(ram==untouched,"Invalid reservation rejection modified RAM");
    rejects([&]{resources.install(std::span(ram).first(ram.size()-1));},"Short RAM accepted");
    auto alias=std::span(const_cast<std::byte*>(snapshot.canonical().data()),isle::beach::rdram_size);
    rejects([&]{resources.install(alias);},"RAM/ROM alias accepted");
    resources.install(ram);
    std::vector<bool> written(ram.size());
    for (const auto& span:spans) for (std::size_t i=0;i<span.bytes.size();++i) {
        const auto target=((span.guest_address-0x80000000)+i)^3u;
        written[target]=true; require(ram[target]==span.bytes[i],"Original referenced asset was modified or not installed");
    }
    for (std::size_t i=0;i<ram.size();++i) if (!written[i]) require(ram[i]==untouched[i],"World install overwrites unreferenced bytes");
    const auto installed=ram;
    auto moved=std::move(snapshot);
    rejects([&]{resources.require_live();},"Moved Snapshot lifetime breach accepted");
    rejects([&]{resources.install(ram);},"Moved Snapshot installed");
    require(ram==installed && moved.canonical().size()==isle::rom::us_size,"Lifetime rejection modified RAM or owner");
}
} // namespace
int main(int argc,char** argv) {
    if (argc!=2) { std::cerr<<"usage: world-resources-rom-smoke <admitted-original.z64>\n"; return EXIT_FAILURE; }
    try { run(argv[1]); std::cout<<"Beach full world resource closure passed\n"; return EXIT_SUCCESS; }
    catch (const std::exception& error) { std::cerr<<error.what()<<'\n'; return EXIT_FAILURE; }
}
