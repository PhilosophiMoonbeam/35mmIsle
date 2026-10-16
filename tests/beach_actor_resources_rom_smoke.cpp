#include "beach_actor_resources.hpp"
#include "menu_animation.hpp"
#include "source_matrix_configuration.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <set>
#include <stdexcept>
#include <utility>

// Opt-in, caller-owned actual admitted ROM only. No extracted assets, authored
// geometry, guest CPU execution, actor process or GPU/capture claim is involved.
namespace b = isle::beach;
namespace m = isle::menu;
namespace a = isle::menu::animation;
namespace {
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
template<class F> void rejects(F&& action, const char* message) {
    bool rejected=false;
    try { action(); } catch (const std::exception&) { rejected=true; }
    require(rejected,message);
}
void consume_animations(const b::ActorResources& resources) {
    const auto models=resources.models();
    std::vector<a::SlotCapacity> capacities;
    capacities.reserve(models.size());
    for (const auto& model:models) {
        const auto capacity=a::slot_capacity(model);
        capacities.push_back(capacity);
    }
    auto made=a::Runtime::create({models,resources.materials(),resources.animations(),m::no_index},
                                capacities,{},{32768,4096});
    require(bool(made),"Complete actual actor catalog must compile in the shared animator");
    auto& runtime=*made.runtime;
    std::vector<a::InstanceHandle> instances(models.size());
    for (std::size_t i=0; i<models.size(); ++i) {
        const auto acquired=runtime.acquire(i,i,true);
        require(bool(acquired),"Actual actor instance acquisition failed");
        instances[i]=acquired.handle;
    }
    for (const auto& header:resources.headers()) {
        const auto model_index=resources.initializations()[header.initialization].model;
        const auto handle=instances[model_index];
        require(bool(runtime.bind_model(handle,header.model_bindings,0,a::ModelBindPose::restore)),
                "Actual header model binding failed");
        if (!header.resets_materials()) {
            require(bool(runtime.bind_materials(handle,header.material_bindings)) &&
                    bool(runtime.set_texture_speed(handle,header.speed)),"Actual header material binding/speed failed");
        } else if (resources.initializations()[header.initialization].textures_present) {
            require(bool(runtime.reset_materials(handle)),"Actual header original-texture reset failed");
        }
        require(bool(runtime.set_model_speed(handle,header.speed)) && bool(runtime.step_model(handle)),
                "Actual header source-speed immediate evaluation failed");
        if (header.speed==0) {
            const auto time=runtime.model(handle).animation_time;
            require(bool(runtime.step_model(handle)) && runtime.model(handle).animation_time==time,
                    "Actual zero-speed header must retain its evaluated model time");
        }
    }
    for (const auto& initialization:resources.initializations()) if (initialization.actor_id==1003) {
        const auto handle=instances[initialization.model];
        float expected=runtime.model(handle).nodes[0].transform.position.y;
        for (unsigned remaining=59;remaining;--remaining) {
            auto transform=runtime.model(handle).nodes[0].transform;
            transform.position.y-=0.2f;
            expected-=0.2f;
            require(bool(runtime.set_node_transform(handle,0,transform)) && bool(runtime.step_model(handle)) &&
                    runtime.model(handle).nodes[0].transform.position.y==expected,
                    "Actual common1003 animation overwrote its59 source root-local path writes");
        }
    }
    // Pure animator events are retained/validated, not dispatched to sound/FX.
    // No native actor factory, behavior, process, renderer or retirement claim.
}

void run(const char* filename) {
    auto snapshot=isle::rom::read_snapshot(filename);
    // Actual World metadata supplies fog; no Title defaults are borrowed.
    const auto world=b::world_descriptors(snapshot);
    const b::FogProfile fog{world.world.fog_rgb,world.world.fog_min,world.world.fog_max};
    const b::ActorResources resources(snapshot,fog);
    const auto initializations=resources.initializations();
    const auto models=resources.models(); const auto headers=resources.headers();
    const auto materials=resources.materials(); const auto animations=resources.animations();
    require(initializations.size()==17 && models.size()==17 && headers.size()==79 &&
            materials.size()==33 && animations.size()==1317,"Complete17/79/33/1317 actor catalog required");
    constexpr std::array<std::uint32_t,17> ids{143,52,133,12,131,84,16,123,113,115,1005,1004,1001,129,25,1006,1003};
    std::size_t empty=b::actor_no_index, common=b::actor_no_index, actor1006=b::actor_no_index;
    for (std::size_t i=0; i<initializations.size(); ++i) {
        const auto& init=initializations[i];
        require(init.actor_id==ids[i] && init.model==i && init.wire.bytes.size()==52 &&
                init.render_callback_identity==init.wire.u32(8),"Lost actor source initialization/render identity");
        require(init.textures_present==(init.wire.u32(4)!=0) &&
                init.textures_present==!models[i].material_table.bytes.empty(),
                "Initialization outer textures NULL semantics lost");
        const auto& model=models[i];
        require(model.matrix_kinds==m::model::select_source_matrix_kinds(init.flags,init.configured_matrix_kinds) &&
                model.fog_rgb==fog.rgb && model.fog_min==fog.min && model.fog_max==fog.max,
                "Effective matrix configuration or explicit World fog lost");
        require(model.tree.bytes.size()==(model.nodes.size()+1)*44 && model.tree.u32(model.nodes.size()*44)==18,
                "Actor tree lacks exact sentinel-inclusive source framing");
        const auto visibility=resources.visibility_domains(i);
        require(visibility.size()==model.nodes.size(),"Missing per-node visibility domains");
        for (std::size_t j=0; j<model.nodes.size(); ++j) {
            const auto& node=model.nodes[j];
            require((node.parent==b::actor_no_index || node.parent<j) && (visibility[j]&1),
                    "Actor parent/initial flags0 not preserved");
            if (!model.material_table.bytes.empty() && model.material_table.u32(j*4)) {
                require(!node.materials.empty(),"Empty non-NULL actor attachment list");
                for (const auto index:node.materials) require(index<materials.size(),"Invalid native material attachment");
            }
        }
        if (init.animations_enabled) {
            require(init.setup.initial_header<headers.size() && headers[init.setup.initial_header].initialization==i &&
                    headers[init.setup.initial_header].wire.guest_address==init.setup.wire.u32(0),
                    "Initial header must come from actual setup, not first-address order");
        } else require(init.setup.initial_header==b::actor_no_index && init.setup.wire.u32(0)==0,
                       "NULL initial header must remain explicitly disabled");
        if (init.actor_id==1005) empty=i;
        if (init.actor_id==1003) common=i;
        if (init.actor_id==1006) actor1006=i;
    }
    require(empty!=b::actor_no_index && !initializations[empty].animations_enabled &&
            models[empty].nodes.size()==1 && models[empty].nodes[0].payloads.empty(),
            "Geometryless1005 must remain present rather than excluded");
    require(common!=b::actor_no_index && models[common].nodes.size()==4 &&
            initializations[common].wire.guest_address==0x80388348 &&
            (models[common].nodes[1].depth_flags&0x2000),"Common1003 exact tree/high flags missing");
    std::size_t preserved_null_slots=0, null_material_nodes=0, reset_headers=0;
    for (const auto& header:headers) {
        require(header.initialization<initializations.size() && header.wire.bytes.size()==20 &&
                header.speed==header.wire.f32(0) && header.unknown_04==header.wire.f32(4),
                "Header fields or unqualified unknown04 lost");
        const auto& model=models[initializations[header.initialization].model];
        require(header.nodes.size()==model.nodes.size(),"Header outer tables need one entry per model node");
        require(header.model_bindings.size()==header.nodes.size() &&
                header.model_animation_table.guest_address==header.wire.u32(8) &&
                header.material_animation_table.guest_address==header.wire.u32(12),
                "Flattened binding construction changed source array identities");
        std::size_t attachment=0;
        if (header.resets_materials()) ++reset_headers;
        std::size_t sound_count=0;
        for (std::size_t i=0; i<header.nodes.size(); ++i) {
            const auto& binding=header.nodes[i]; const auto& node=model.nodes[i];
            const auto model_guest=header.model_animation_table.bytes.empty() ? 0 : header.model_animation_table.u32(i*4);
            require((binding.model_animation==b::actor_no_index)==(model_guest==0),"Model animation NULL binding changed");
            require(header.model_bindings[i]==binding.model_animation,
                    "Flattened model binding changed DFS slot/NULL semantics");
            if (binding.model_animation!=b::actor_no_index) {
                require(binding.model_animation<animations.size() && animations[binding.model_animation].wire.guest_address==model_guest,
                        "Unrelocated model animation header binding");
                for (const auto& command:animations[binding.model_animation].commands) {
                    if (command.opcode==15)
                        require(resources.visibility_domains(initializations[header.initialization].model)[i]&(1u<<(command.mask&3u)),
                                "Noninitial header visibility choice missing");
                    if (command.opcode==16 && node.parent==b::actor_no_index && (command.mask>>8)<=3 && (command.mask&255))
                        sound_count=std::max(sound_count,static_cast<std::size_t>(command.mask&255));
                }
            }
            const auto inner=header.material_animation_table.bytes.empty() ? 0 : header.material_animation_table.u32(i*4);
            require((inner==0)==binding.material_animation_table.bytes.empty(),"Material outer/per-node NULL distinction lost");
            if (!inner) { ++null_material_nodes; require(binding.material_animations.empty(),"NULL inner fabricated animations"); }
            else {
                require(binding.material_animations.size()==node.materials.size() &&
                        binding.material_animation_table.bytes.size()==node.materials.size()*4,
                        "Material animation inner must use actual MObj count");
                for (std::size_t j=0; j<node.materials.size(); ++j) {
                    const auto guest=binding.material_animation_table.u32(j*4);
                    const auto index=binding.material_animations[j];
                    require((guest==0)==(index==b::actor_no_index),"NULL material animation entry terminated or replaced binding");
                    if (!guest) ++preserved_null_slots;
                    else require(index<animations.size() && animations[index].wire.guest_address==guest &&
                                 animations[index].kind==m::AnimationKind::texture,"Invalid material program native binding");
                }
            }
            if (!header.resets_materials()) {
                for (std::size_t j=0; j<node.materials.size(); ++j) {
                    require(attachment<header.material_bindings.size() &&
                            header.material_bindings[attachment]==(inner ? binding.material_animations[j] : b::actor_no_index),
                            "Flattened material binding changed attachment order/NULL preservation");
                    ++attachment;
                }
            }
        }
        require(header.material_bindings.size()==attachment,
                "Outer NULL must omit bindings; non-NULL must retain every attachment");
        require(header.sound_ids.size()==(header.wire.u32(16) ? sound_count : 0) &&
                header.sound_id_table.bytes.size()==header.sound_ids.size()*4,
                "Sound ids must be bounded only by positive root callback0..3 uses");
    }
    require(preserved_null_slots && null_material_nodes && reset_headers,"Actual NULL preservation/reset cases not consumed");
    std::size_t model_programs=0, material_programs=0, model_commands=0, material_commands=0, model_cycles=0, material_cycles=0;
    std::set<std::pair<std::uint32_t,m::AnimationKind>> unique_programs;
    for (const auto& animation:animations) {
        require(unique_programs.emplace(animation.wire.guest_address,animation.kind).second,"Program was copied rather than deduplicated by guest+kind");
        const bool model=animation.kind==m::AnimationKind::model;
        if (model) { ++model_programs; model_commands+=animation.commands.size(); }
        else { ++material_programs; material_commands+=animation.commands.size(); }
        std::size_t bytes=0, wait=0;
        for (const auto& command:animation.commands) {
            require(command.wire.guest_address==animation.wire.guest_address+bytes && command.path_index==b::actor_no_index,
                    "Animation framing includes unrelated words/invented paths");
            bytes+=command.wire.bytes.size();
            if (command.opcode==2 || command.opcode==3 || command.opcode==5 || command.opcode==8 || command.opcode==10 ||
                command.opcode==15 || command.opcode==16 || command.opcode==18 || command.opcode==20) wait+=command.duration;
            require(command.opcode!=1 && command.opcode!=13 && command.opcode!=17 && command.opcode!=22 && command.opcode!=23,
                    "Unsupported opcode silently admitted");
        }
        require(bytes==animation.wire.bytes.size() && !animation.commands.empty(),"Incorrect animation stream borrowed extent");
        const auto& last=animation.commands.back();
        require(last.opcode==0 || (last.opcode==14 && last.wire.bytes.size()==8 &&
                last.wire.u32(4)==animation.wire.guest_address && wait>0),"Not a real END or positive-wait framed14 cycle");
        if (last.opcode==14) { if (model) ++model_cycles; else ++material_cycles; }
    }
    require(model_programs==1201 && material_programs==116 && model_commands==19664 && material_commands==1047 &&
            model_cycles==737 && material_cycles==69,"All79-header source program closure not consumed");
    std::size_t image_entries=0, primitive_only=0, null_images=0; std::set<std::uint32_t> images;
    for (const auto& material:materials) {
        image_entries+=material.images.size(); images.insert(material.images.begin(),material.images.end());
        if (material.images.empty()) {
            ++null_images;
            if (material.flags==0x200) ++primitive_only;
            require(!(material.flags&0x17) && material.wire.u32(4)==0,
                    "NULL-image Texture must preserve incoming TIMG, not fabricate an image");
        }
        require(material.palettes.empty() && material.wire.u32(44)==0,"Unexpected dynamic palette table");
    }
    require(image_entries==94 && images.size()==74 && primitive_only==4 && null_images==5,"Exact33 Texture image extents missing");
    require(actor1006!=b::actor_no_index && resources.visibility_domains(actor1006)[1]==1 &&
            resources.visibility_domains(actor1006)[2]==1,"Invented independent1006 supplier omission");
    const auto& gate=models[actor1006];
    const auto has_root=[](const m::Node& node,std::uint32_t root) {
        return std::any_of(node.payloads.begin(),node.payloads.end(),[root](const auto& p) { return p.display_list_id==1 && p.draw_list==root; });
    };
    require(gate.nodes[1].parent==gate.nodes[2].parent && has_root(gate.nodes[1],0x80139470) && has_root(gate.nodes[2],0x80139558),
            "1006 source consecutive lane1 sibling roots lost");
    require(std::any_of(resources.image_loads().begin(),resources.image_loads().end(),[](const auto& load) {
        return load.command_identity==0x801395d8 && load.wire.guest_address==0x8013daf8 && load.wire.bytes.size()==384 &&
               load.format==4 && load.source_size_code==2 && load.source_width==1;
    }),"1006 local predecessor TIMG failed to discharge actual384-byte consumer");
    const auto definitions=resources.definitions();
    require(definitions.size()==22 && definitions[16].actor_id==0 && definitions[16].initialization==b::actor_no_index &&
            definitions.back().factory_identity==0x8035e0d4 && definitions.back().initialization==common,
            "Primary definition terminator/singletons/common returning factory missing");
    consume_animations(resources);
    const auto spans=resources.installation_spans(); require(!spans.empty(),"No complete actor asset installation union");
    std::uint64_t previous_end=0;
    for (const auto& span:spans) {
        require(span.guest_address>previous_end && span.bytes.data()==snapshot.canonical().data()+span.rom_offset,
                "Actor install union overlaps or copied source ownership");
        previous_end=static_cast<std::uint64_t>(span.guest_address)+span.bytes.size();
    }
    std::vector<std::byte> ram(b::rdram_size,std::byte{0x5a}); const auto untouched=ram;
    const auto& last=spans.back();
    const std::array<b::RamReservation,1> late{{{last.guest_address-0x80000000,last.bytes.size()}}};
    rejects([&] { resources.install(ram,late); },"Late caller collision accepted");
    require(ram==untouched,"Late reservation failure partially installed actor assets");
    const std::array<b::RamReservation,1> old_world{{{0x170000,0x20000}}};
    require(std::any_of(spans.begin(),spans.end(),[](const auto& span) {
        return span.guest_address<0x80190000 && static_cast<std::uint64_t>(span.guest_address)+span.bytes.size()>0x80170000;
    }),"Actual actor assets must expose old World dynamic-buffer overlap");
    rejects([&] { resources.install(ram,old_world); },"Old World dynamic buffers silently overwrite actor assets");
    const std::array<b::RamReservation,1> invalid{{{b::rdram_size,1}}};
    rejects([&] { resources.install(ram,invalid); },"Invalid reservation accepted");
    rejects([&] { resources.install(std::span(ram).first(ram.size()-1)); },"Short RAM accepted");
    auto alias=std::span(const_cast<std::byte*>(snapshot.canonical().data()),b::rdram_size);
    rejects([&] { resources.install(alias); },"ROM/RAM alias accepted");
    require(ram==untouched,"Rejected install modified RAM");
    // Isolated byte-consumer proof only, NOT a production runtime RAM layout.
    resources.install(ram);
    std::vector<bool> written(ram.size());
    for (const auto& span:spans) for (std::size_t i=0; i<span.bytes.size(); ++i) {
        const auto target=((span.guest_address-0x80000000)+i)^3u;
        written[target]=true; require(ram[target]==span.bytes[i],"Borrowed original actor asset changed on install");
    }
    for (std::size_t i=0; i<ram.size(); ++i) if (!written[i]) require(ram[i]==untouched[i],"Actor install overwrites unreferenced RAM");
    const auto installed=ram; auto moved=std::move(snapshot);
    rejects([&] { resources.require_live(); },"Moved Snapshot borrow accepted");
    rejects([&] { static_cast<void>(resources.headers()); },"Moved Snapshot accessor accepted");
    rejects([&] { resources.install(ram); },"Moved Snapshot installation accepted");
    require(ram==installed && moved.canonical().size()==isle::rom::us_size,"Lifetime rejection changed RAM or owner");
}
} // namespace
int main(int argc,char** argv) {
    if (argc!=2) { std::cerr<<"usage: beach-actor-resources-rom-smoke <admitted-original.z64>\n"; return EXIT_FAILURE; }
    try { run(argv[1]); std::cout<<"Beach complete immutable actor resource consumer and all79 shared-runtime header evaluations passed\n"; return EXIT_SUCCESS; }
    catch (const std::exception& error) { std::cerr<<error.what()<<'\n'; return EXIT_FAILURE; }
}
