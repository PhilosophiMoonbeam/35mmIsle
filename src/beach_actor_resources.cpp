#include "beach_actor_resources.hpp"
#include "source_matrix_configuration.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <functional>
#include <stdexcept>
#include <string>
#include <utility>

namespace isle::beach {
namespace {
constexpr std::uint32_t ram_base = 0x80000000;
constexpr std::array<std::uint8_t, 20> admitted_sha1{
    0xed,0xc7,0xc4,0x9c,0xc5,0x68,0xc0,0x45,0xfe,0x48,
    0xbe,0x0d,0x18,0x01,0x1c,0x30,0xf3,0x93,0xcb,0xaf
};
void require(bool condition, const char* message, std::uint32_t address = 0) {
    if (!condition) throw std::runtime_error(std::string(message) + " at guest " + std::to_string(address));
}
float finite(const rom::GuestView& wire, std::size_t offset) {
    const auto value = wire.f32(offset);
    require(std::isfinite(value), "Nonfinite actor scalar", wire.guest_address);
    return value;
}
menu::Vec3 vector3(const rom::GuestView& wire, std::size_t offset) {
    return {finite(wire,offset),finite(wire,offset+4),finite(wire,offset+8)};
}
// Own GAME declarations + typed ROM identities only. See beach-actor-inputs.json.
struct ActorFact { std::uint32_t id, initialization; std::size_t nodes, header_begin, header_count; };
constexpr std::array<ActorFact, 17> actor_facts{{
    {143,0x802cc2f0,15,0,10},
    {52,0x802cc968,20,10,10},
    {133,0x802cccec,14,20,5},
    {12,0x802ccda4,12,25,3},
    {131,0x802cce50,11,28,2},
    {84,0x802ccf78,17,30,5},
    {16,0x802cd118,17,35,4},
    {123,0x802cd258,17,39,6},
    {113,0x802cd55c,17,45,9},
    {115,0x802cd7a8,18,54,7},
    {1005,0x802cd8e4,1,61,0},
    {1004,0x802cd948,7,61,1},
    {1001,0x802cda10,4,62,4},
    {129,0x802cdaec,18,66,2},
    {25,0x802ce038,22,68,9},
    {1006,0x802ce0a8,5,77,1},
    {1003,0x80388348,4,78,1},
}};
constexpr std::array<std::uint32_t, 79> header_identities{
    0x802cc10c,0x802cc120,0x802cc134,0x802cc148,0x802cc15c,
    0x802cc170,0x802cc184,0x802cc198,0x802cc1ac,0x802cc1c0,
    0x802cc344,0x802cc358,0x802cc36c,0x802cc380,0x802cc394,
    0x802cc3a8,0x802cc3bc,0x802cc3d0,0x802cc3e4,0x802cc3f8,
    0x802cc9ac,0x802cc9c0,0x802cc9d4,0x802cc9e8,0x802cc9fc,
    0x802ccd24,0x802ccd38,0x802ccd4c,0x802cce08,0x802ccde8,
    0x802cceb0,0x802ccec4,0x802cced8,0x802cceec,0x802ccf00,
    0x802ccfc0,0x802ccfd4,0x802ccfe8,0x802ccffc,0x802cd168,
    0x802cd17c,0x802cd190,0x802cd1a4,0x802cd1b8,0x802cd1cc,
    0x802cd2a8,0x802cd2bc,0x802cd2d0,0x802cd2e4,0x802cd2f8,
    0x802cd30c,0x802cd320,0x802cd334,0x802cd348,0x802cd5a8,
    0x802cd5bc,0x802cd5d0,0x802cd5e4,0x802cd5f8,0x802cd60c,
    0x802cd620,0x802cd920,0x802cd980,0x802cd994,0x802cd9a8,
    0x802cd9bc,0x802cda8c,0x802cdab4,0x802cdb5c,0x802cdb70,
    0x802cdb84,0x802cdbac,0x802cdbc0,0x802cdbd4,0x802cdbe8,
    0x802cdbfc,0x802cdc10,0x802ce070,0x80388310,
};
struct MaterialExtent { std::uint32_t material, table; std::size_t images, slots; };
constexpr std::array<MaterialExtent, 33> material_extents{{
    {0x800eb520,0x0,0,0},
    {0x800f07b8,0x800f07b0,1,1},
    {0x800f0830,0x800f07b4,1,1},
    {0x800f08a8,0x0,0,0},
    {0x801397f8,0x0,0,0},
    {0x80139870,0x0,0,0},
    {0x801398e8,0x801397d4,4,4},
    {0x80139960,0x801397e4,4,4},
    {0x8031c278,0x8031c264,2,2},
    {0x8031c2f0,0x8031c26c,2,3},
    {0x80320128,0x80320108,4,4},
    {0x803201a0,0x80320118,4,4},
    {0x80324e98,0x80324e88,4,4},
    {0x8032b960,0x8032b934,7,7},
    {0x8032b9d8,0x8032b950,2,2},
    {0x8032ba50,0x8032b958,2,2},
    {0x8032bac8,0x0,0,0},
    {0x803303c8,0x803303bc,3,3},
    {0x80332a08,0x803329e4,6,6},
    {0x80332a80,0x803329fc,3,3},
    {0x80335c18,0x80335be0,6,6},
    {0x80335c90,0x80335bf8,3,3},
    {0x80335d08,0x80335c04,4,5},
    {0x8033b210,0x8033b204,2,3},
    {0x80347770,0x803476f8,2,2},
    {0x803477e8,0x80347700,2,2},
    {0x80347860,0x80347708,5,5},
    {0x803478d8,0x8034771c,5,5},
    {0x80347950,0x80347730,5,5},
    {0x803479c8,0x80347744,2,2},
    {0x80347a40,0x8034774c,5,5},
    {0x80347ab8,0x80347760,2,2},
    {0x80347b30,0x80347768,2,2},
}};
constexpr std::array<model_assets::Mapping, 10> mappings{{
    {0x800e18a0,0x5f050,0x14160},
    {0x800f5d90,0x731b0,0xa200},
    {0x800fff90,0x7d3b0,0x1b0c0},
    {0x8011b050,0xa74e0,0x952a0},
    {0x801b0310,0x13c780,0x26530},
    {0x8034e130,0x54b5d0,0x20d0},
    {0x803476a0,0x54d6a0,0x6a90},
    {0x802cbe10,0x563e80,0x22e0},
    {0x80318f10,0x5b0f80,0x2e650},
    {0x80388310,0x528720,0x90},
}};

menu::RenderType render_type(std::uint32_t identity) {
    switch (identity) {
    case 0x80014f98: return menu::RenderType::b;
    case 0x803594dc: return menu::RenderType::b_fogged;
    case 0x800a1608: case 0x80359534: return menu::RenderType::d_fogged;
    case 0x8035942c: return menu::RenderType::i_fogged;
    case 0x80359484: return menu::RenderType::j_fogged;
    default: throw std::runtime_error("Unknown actor render callback identity");
    }
}
void valid_reservation(RamReservation range) {
    if (range.offset > rdram_size || range.size > rdram_size-range.offset)
        throw std::invalid_argument("Actor reservation escapes 8 MiB RDRAM");
}
void disjoint(const rom::GuestView& wire, RamReservation range) {
    const auto offset = static_cast<std::size_t>(wire.guest_address-ram_base);
    if (range.size && offset < range.offset+range.size && range.offset < offset+wire.bytes.size())
        throw std::invalid_argument("Actor resource overlaps runtime reservation");
}
} // namespace

struct ActorResourceBuilder {
    ActorResources& out;
    const model_assets::Reader& reader;
    FogProfile fog;
    rom::GuestView at(std::uint32_t address, std::size_t bytes, std::size_t alignment = 4) const {
        return reader.view(address,bytes,alignment);
    }
    std::size_t material(std::uint32_t address) {
        for (std::size_t i=0; i<out.materials_.size(); ++i)
            if (out.materials_[i].wire.guest_address==address) return i;
        const auto fact = std::find_if(material_extents.begin(),material_extents.end(),
            [address](const auto& f) { return f.material==address; });
        require(fact!=material_extents.end(),"Unknown actor Texture identity",address);
        menu::Material result{};
        result.wire=at(address,0x78);
        const auto& w=result.wire;
        result.format=w.u8(2); result.size_code=w.u8(3);
        result.scale=w.u16(8); result.image_origin=w.u16(10);
        result.width=w.u16(12); result.height=w.u16(14);
        result.flags=w.u16(48); result.auxiliary_format=w.u8(50); result.auxiliary_size_code=w.u8(51);
        result.block_width=w.u16(52); result.block_height=w.u16(54);
        result.auxiliary_width=w.u16(56); result.auxiliary_height=w.u16(58);
        constexpr std::array<std::size_t,9> offsets{20,24,28,32,36,40,60,64,68};
        for (std::size_t i=0; i<offsets.size(); ++i) result.parameters[i]=finite(w,offsets[i]);
        for (std::size_t i=0; i<10; ++i) result.color_and_trailing_words[i]=w.u32(80+i*4);
        require(result.format<=4 && result.size_code<=3 && result.auxiliary_format<=4 &&
                result.auxiliary_size_code<=3 && w.u32(44)==0 && !(result.flags&4),
                "Malformed actor Texture/palette ownership",address);
        require(w.u32(4)==fact->table,"Actor image table identity mismatch",address);
        if (fact->slots) {
            result.image_table=at(fact->table,fact->slots*4);
            result.images.reserve(fact->images);
            for (std::size_t i=0; i<fact->images; ++i) {
                const auto image=result.image_table.u32(i*4);
                at(image,1,8); result.images.push_back(image);
            }
            for (std::size_t i=fact->images; i<fact->slots; ++i)
                require(result.image_table.u32(i*4)==0,"Actor image NULL padding mismatch",address);
        } else require(!(result.flags&0x17),"NULL images cannot supply image/palette-emitting Texture flags",address);
        require(!(result.flags&0x10) || result.images.size()>=2,"LOD Texture lacks adjacent images",address);
        out.materials_.push_back(std::move(result));
        return out.materials_.size()-1;
    }
    std::size_t animation(std::uint32_t address, menu::AnimationKind kind) {
        if (!address) return actor_no_index;
        for (std::size_t i=0; i<out.animations_.size(); ++i)
            if (out.animations_[i].wire.guest_address==address && out.animations_[i].kind==kind) return i;
        menu::Animation result{}; result.kind=kind;
        result.callback_installed=kind==menu::AnimationKind::model;
        const std::size_t limit=kind==menu::AnimationKind::model ? 178 : 67;
        result.commands.reserve(16);
        std::size_t bytes=0, positive_wait=0;
        for (std::size_t i=0; ; ++i) {
            require(i<limit,"Actor animation exceeds exact admitted framing bound",address);
            const auto head=at(address+static_cast<std::uint32_t>(bytes),4);
            const auto word=head.u32(0), opcode=word>>25, mask=(word>>15)&1023u, duration=word&32767u;
            const auto count=static_cast<std::size_t>(std::popcount(mask));
            std::size_t words=1; bool floats=false, adds_wait=false;
            switch (opcode) {
            case 0: require(word==0,"Malformed actor END",head.guest_address); break;
            case 2: require(mask==0,"Malformed actor WAIT",head.guest_address); adds_wait=true; break;
            case 3: case 4: case 7: case 8: case 9: case 10: case 11:
                words+=count; floats=true; adds_wait=opcode==3 || opcode==8 || opcode==10; break;
            case 5: case 6: words+=count*2; floats=true; adds_wait=opcode==5; break;
            case 12: break;
            case 14:
                require(mask==0 && duration==0,"Malformed actor SET_ANIMATION",head.guest_address);
                words=2; break;
            case 15: case 16:
                require(kind==menu::AnimationKind::model,"Model-only actor command",head.guest_address);
                adds_wait=true; break;
            case 18: case 19: case 20: case 21:
                require(kind==menu::AnimationKind::texture && !(mask&~31u),
                        "Invalid actor packed-color mask",head.guest_address);
                words+=count; adds_wait=opcode==18 || opcode==20; break;
            default: throw std::runtime_error("Unadmitted actor animation opcode at guest "+std::to_string(head.guest_address));
            }
            const auto wire=at(head.guest_address,words*4);
            if (floats) for (std::size_t j=1; j<words; ++j) finite(wire,j*4);
            result.commands.push_back({wire,static_cast<std::uint8_t>(opcode),
                static_cast<std::uint16_t>(mask),static_cast<std::uint16_t>(duration)});
            bytes+=words*4;
            if (adds_wait) positive_wait+=duration;
            if (!opcode) break;
            if (opcode==14) {
                // Every admitted actor stream is contiguous and cycles to its
                // own framed entry. No JUMP1/interior/pointer guess is admitted.
                require(wire.u32(4)==address && positive_wait>0,
                        "Actor cycle lacks framed entry/positive wait",wire.guest_address);
                break;
            }
        }
        result.wire=at(address,bytes);
        out.animations_.push_back(std::move(result));
        return out.animations_.size()-1;
    }
    void validate_material_animation(std::size_t stream, std::size_t material_index) const {
        if (stream==actor_no_index) return;
        const auto& material=out.materials_[material_index];
        for (const auto& command:out.animations_[stream].commands) {
            if (command.opcode<3 || command.opcode>11) continue;
            std::size_t operand=1;
            for (unsigned bit=0; bit<10; ++bit) if (command.mask&(1u<<bit)) {
                if (bit==0 || bit==5) {
                    const auto value=std::trunc(finite(command.wire,operand*4));
                    require((command.opcode==3 || command.opcode==4 || command.opcode==10 || command.opcode==11) &&
                            value>=0 && value<static_cast<double>(material.images.size()),
                            "Actor image channel escapes bounded image entries",command.wire.guest_address);
                    // LINEAR endpoints and the source initial zero are in the
                    // domain; STEP is exact. No cubic/rate image assumption.
                }
                require(bit!=9,"Unexpected dynamic actor palette channel",command.wire.guest_address);
                require(bit!=8 || !(material.flags&0x10),"Unadmitted animated actor LOD",command.wire.guest_address);
                operand+=(command.opcode==5 || command.opcode==6) ? 2 : 1;
            }
        }
    }
    void model(const ActorFact& fact) {
        ActorInitialization initialization{};
        initialization.wire=at(fact.initialization,52); initialization.actor_id=fact.id;
        const auto& w=initialization.wire;
        initialization.render_callback_identity=w.u32(8);
        initialization.textures_present=w.u32(4)!=0;
        initialization.scale=vector3(w,16); initialization.collision_center=vector3(w,28);
        initialization.radius=finite(w,40); initialization.flags=w.u16(44);
        initialization.configured_matrix_kinds={w.u8(46),w.u8(47),w.u8(48)};
        require(initialization.scale.x!=0 && initialization.scale.y!=0 && initialization.scale.z!=0 &&
                initialization.radius>=0,"Invalid actor initialization scale/radius",fact.initialization);
        auto& setup=initialization.setup; setup.wire=at(w.u32(12),20);
        setup.initial_state_identity=setup.wire.u32(4); setup.process_kind=std::bit_cast<std::int8_t>(setup.wire.u8(8));
        setup.unknown_0c_identity=setup.wire.u32(12); setup.transition_graph_identity=setup.wire.u32(16);
        require(setup.initial_state_identity && setup.process_kind==0 && !setup.unknown_0c_identity &&
                !setup.transition_graph_identity,"Unexpected actor setup identities",setup.wire.guest_address);
        initialization.animations_enabled=setup.wire.u32(0)!=0;
        require(initialization.animations_enabled==(fact.header_count!=0),"Actor setup/header domain mismatch",fact.initialization);
        menu::Model result{}; result.render_type=render_type(initialization.render_callback_identity);
        result.animation_speed=0; // Disabled1005 has no header; enabled models get actual initial speed below.
        result.matrix_kinds=menu::model::select_source_matrix_kinds(initialization.flags,initialization.configured_matrix_kinds);
        result.fog_rgb=fog.rgb; result.fog_min=fog.min; result.fog_max=fog.max;
        result.tree=at(w.u32(0),(fact.nodes+1)*44); result.nodes.reserve(fact.nodes);
        if (w.u32(4)) result.material_table=at(w.u32(4),fact.nodes*4);
        std::array<std::size_t,18> ancestors; ancestors.fill(actor_no_index);
        for (std::size_t i=0; i<fact.nodes; ++i) {
            menu::Node node{}; node.wire=at(w.u32(0)+static_cast<std::uint32_t>(i*44),44);
            node.depth_flags=node.wire.u32(0); const auto depth=node.depth_flags&4095u;
            require(depth<18 && !(node.depth_flags&0xffff0000u) && ((depth==0)==(i==0)),
                    "Malformed actor tree depth",node.wire.guest_address);
            if (depth) { node.parent=ancestors[depth-1]; require(node.parent!=actor_no_index,"Orphan actor node",node.wire.guest_address); }
            ancestors[depth]=i;
            for (std::size_t d=depth+1; d<ancestors.size(); ++d) ancestors[d]=actor_no_index;
            node.position=vector3(node.wire,8); node.rotation=vector3(node.wire,20); node.scale=vector3(node.wire,32);
            if (const auto payload=node.wire.u32(4)) {
                if (result.render_type==menu::RenderType::b || result.render_type==menu::RenderType::b_fogged)
                    node.payloads.push_back({{},0,0,payload});
                else if (result.render_type==menu::RenderType::i_fogged) {
                    const auto p=at(payload,8); node.payloads.push_back({p,0,p.u32(0),p.u32(4)});
                } else {
                    const std::size_t stride=result.render_type==menu::RenderType::d_fogged ? 8 : 12;
                    for (std::size_t j=0; ; ++j) {
                        require(j<5,"Actor payload lacks bounded lane sentinel",payload);
                        const auto p=at(payload+static_cast<std::uint32_t>(j*stride),stride);
                        const auto id=p.u32(0);
                        if (id==4) break;
                        require(id<4,"Unknown actor model lane",p.guest_address);
                        node.payloads.push_back({p,id,stride==12 ? p.u32(4) : 0,p.u32(stride==12 ? 8 : 4)});
                    }
                }
            }
            if (!result.material_table.bytes.empty()) if (const auto list=result.material_table.u32(i*4)) {
                for (std::size_t j=0; ; ++j) {
                    require(j<4,"Actor attachment list exceeds exact maximum3 plus NULL",list);
                    const auto target=at(list+static_cast<std::uint32_t>(j*4),4).u32(0);
                    if (!target) break;
                    node.materials.push_back(material(target));
                }
            }
            result.nodes.push_back(std::move(node));
        }
        require(result.tree.u32(fact.nodes*44)==18,"Actor tree missing source sentinel",w.u32(0));
        for (std::size_t i=4; i<44; i+=4)
            require(result.tree.u32(fact.nodes*44+i)==0,"Actor sentinel contains unrelated fields",w.u32(0));
        initialization.model=out.models_.size();
        out.models_.push_back(std::move(result)); out.initializations_.push_back(std::move(initialization));
        out.visibility_.emplace_back(fact.nodes,std::uint8_t{1});
    }
    void header(std::size_t initialization_index, std::uint32_t address) {
        auto& initialization=out.initializations_[initialization_index];
        auto& model=out.models_[initialization.model];
        ActorAnimationHeader header{}; header.wire=at(address,20);
        header.initialization=initialization_index; header.speed=finite(header.wire,0); header.unknown_04=finite(header.wire,4);
        if (const auto table=header.wire.u32(8)) header.model_animation_table=at(table,model.nodes.size()*4);
        if (const auto table=header.wire.u32(12)) header.material_animation_table=at(table,model.nodes.size()*4);
        header.nodes.reserve(model.nodes.size());
        header.model_bindings.reserve(model.nodes.size());
        if (!header.material_animation_table.bytes.empty()) {
            std::size_t attachments=0;
            for (const auto& node:model.nodes) attachments+=node.materials.size();
            header.material_bindings.reserve(attachments);
        }
        std::size_t sound_count=0;
        for (std::size_t i=0; i<model.nodes.size(); ++i) {
            const auto& node=model.nodes[i]; ActorNodeBinding binding{};
            if (!header.model_animation_table.bytes.empty())
                binding.model_animation=animation(header.model_animation_table.u32(i*4),menu::AnimationKind::model);
            if (binding.model_animation!=actor_no_index) for (const auto& command:out.animations_[binding.model_animation].commands) {
                if (command.opcode==15)
                    out.visibility_[initialization.model][i]|=static_cast<std::uint8_t>(1u<<(command.mask&3u));
                if (command.opcode==16 && node.parent==actor_no_index && (command.mask>>8)<=3) {
                    const auto argument=command.mask&255u;
                    if (argument) sound_count=std::max(sound_count,static_cast<std::size_t>(argument));
                }
            }
            if (!header.material_animation_table.bytes.empty()) if (const auto list=header.material_animation_table.u32(i*4)) {
                require(!node.materials.empty(),"Actor material animation without MObj",list);
                binding.material_animation_table=at(list,node.materials.size()*4);
                binding.material_animations.reserve(node.materials.size());
                for (std::size_t j=0; j<node.materials.size(); ++j) {
                    const auto stream=animation(binding.material_animation_table.u32(j*4),menu::AnimationKind::texture);
                    validate_material_animation(stream,node.materials[j]);
                    binding.material_animations.push_back(stream); // NULL preserves this slot, never terminates.
                }
            }
            header.model_bindings.push_back(binding.model_animation);
            if (!header.material_animation_table.bytes.empty()) {
                if (binding.material_animation_table.bytes.empty())
                    header.material_bindings.insert(header.material_bindings.end(),node.materials.size(),actor_no_index);
                else
                    header.material_bindings.insert(header.material_bindings.end(),
                                                    binding.material_animations.begin(),binding.material_animations.end());
            }
            header.nodes.push_back(std::move(binding));
        }
        if (const auto ids=header.wire.u32(16); ids && sound_count) {
            header.sound_id_table=at(ids,sound_count*4); header.sound_ids.reserve(sound_count);
            for (std::size_t i=0; i<sound_count; ++i) header.sound_ids.push_back(std::bit_cast<std::int32_t>(header.sound_id_table.u32(i*4)));
        }
        if (initialization.setup.wire.u32(0)==address) {
            require(initialization.setup.initial_header==actor_no_index,"Duplicate actor initial header",address);
            initialization.setup.initial_header=out.headers_.size();
            model.animation_speed=header.speed; model.animation_table=header.model_animation_table;
            model.material_animation_table=header.material_animation_table;
            for (std::size_t i=0; i<model.nodes.size(); ++i) {
                model.nodes[i].animation=header.nodes[i].model_animation;
                model.nodes[i].material_animations=header.nodes[i].material_animations;
            }
        }
        out.headers_.push_back(std::move(header));
    }
    void definition(std::uint32_t address, bool singleton) {
        ActorDefinition result{}; result.wire=at(address,16); result.singleton=singleton;
        result.actor_id=result.wire.u32(0); result.factory_identity=result.wire.u32(4);
        result.change_block_identity=result.wire.u32(8); result.remove_identity=result.wire.u32(12);
        if (result.actor_id) {
            for (std::size_t i=0; i<out.initializations_.size(); ++i)
                if (out.initializations_[i].actor_id==result.actor_id) result.initialization=i;
            require(result.initialization!=actor_no_index && result.factory_identity && result.change_block_identity,
                    "Unbound actor definition",address);
        } else require(!result.factory_identity && !result.change_block_identity && !result.remove_identity,
                       "Malformed actor definition terminator",address);
        out.definitions_.push_back(result);
    }
};

ActorResources::ActorResources(const rom::Snapshot& snapshot, FogProfile fog)
    : snapshot_(&snapshot), source_data_(snapshot.canonical().data()) {
    require(snapshot.canonical().size()==rom::us_size && snapshot.identity().size==rom::us_size &&
            snapshot.identity().sha1==admitted_sha1,"Actor resources require admitted US Snapshot");
    require(fog.min<fog.max,"Invalid explicit actor fog profile");
    models_.reserve(17); materials_.reserve(33); animations_.reserve(1317);
    headers_.reserve(79); initializations_.reserve(17); definitions_.reserve(22); visibility_.reserve(17);
    const model_assets::Reader reader(snapshot,mappings);
    ActorResourceBuilder builder{*this,reader,fog};
    for (const auto& fact:actor_facts) builder.model(fact);
    for (std::size_t i=0; i<actor_facts.size(); ++i) {
        const auto& fact=actor_facts[i];
        for (std::size_t j=0; j<fact.header_count; ++j) builder.header(i,header_identities[fact.header_begin+j]);
        require(!initializations_[i].animations_enabled || initializations_[i].setup.initial_header!=actor_no_index,
                "Actor initial header absent from complete catalog",fact.initialization);
    }
    for (std::size_t i=0; i<17; ++i) builder.definition(0x802cbee4+static_cast<std::uint32_t>(i*16),false);
    for (const auto identity:std::array<std::uint32_t,5>{0x802cbff4,0x802cd838,0x802cd848,0x802ce0dc,0x8038837c})
        builder.definition(identity,true);
    require(models_.size()==17 && materials_.size()==33 && animations_.size()==1317 && headers_.size()==79,
            "Incomplete Beach actor descriptor/program catalog");
    std::array<model_assets::ModelPlan,17> plans{};
    for (std::size_t i=0; i<models_.size(); ++i) plans[i]={&models_[i],visibility_[i]};
    closure_.emplace(reader,plans,materials_);
    closure_->require_complete();
}
void ActorResources::require_live() const {
    const auto bytes=snapshot_->canonical();
    require(bytes.size()==rom::us_size && bytes.data()==source_data_,"Actor resources borrow moved-from Snapshot");
}
std::span<const menu::Model> ActorResources::models() const { require_live(); return models_; }
std::span<const menu::Material> ActorResources::materials() const { require_live(); return materials_; }
std::span<const menu::Animation> ActorResources::animations() const { require_live(); return animations_; }
std::span<const ActorAnimationHeader> ActorResources::headers() const { require_live(); return headers_; }
std::span<const ActorInitialization> ActorResources::initializations() const { require_live(); return initializations_; }
std::span<const ActorDefinition> ActorResources::definitions() const { require_live(); return definitions_; }
std::span<const std::uint8_t> ActorResources::visibility_domains(std::size_t model) const {
    require_live(); if (model>=visibility_.size()) throw std::out_of_range("Actor visibility model index"); return visibility_[model];
}
std::span<const menu::DisplayList> ActorResources::display_lists() const { require_live(); return closure_->display_lists(); }
std::span<const menu::VertexLoad> ActorResources::vertex_loads() const { require_live(); return closure_->vertex_loads(); }
std::span<const menu::ImageLoad> ActorResources::image_loads() const { require_live(); return closure_->image_loads(); }
std::span<const menu::DynamicMaterialCall> ActorResources::dynamic_material_calls() const { require_live(); return closure_->dynamic_material_calls(); }
std::span<const rom::GuestView> ActorResources::installation_spans() const { require_live(); return closure_->installation_spans(); }
void ActorResources::install(std::span<std::byte> ram, std::span<const RamReservation> reservations) const {
    require_live(); closure_->require_complete();
    if (ram.size()!=rdram_size) throw std::invalid_argument("Actor install requires word-swapped 8 MiB RDRAM");
    const auto source=snapshot_->canonical(); const std::less<const std::byte*> less;
    if (less(ram.data(),source.data()+source.size()) && less(source.data(),ram.data()+ram.size()))
        throw std::invalid_argument("Actor installation aliases Snapshot");
    for (const auto range:reservations) valid_reservation(range);
    const auto spans=closure_->installation_spans();
    for (const auto& span:spans) {
        const auto offset=static_cast<std::size_t>(span.guest_address-ram_base);
        require(span.guest_address>=ram_base && offset<=ram.size() && span.bytes.size()<=ram.size()-offset &&
                span.rom_offset<=source.size() && span.bytes.size()<=source.size()-span.rom_offset &&
                span.bytes.data()==source.data()+span.rom_offset,"Invalid actor install provenance/extent",span.guest_address);
        for (const auto range:reservations) disjoint(span,range);
    }
    for (const auto& span:spans) {
        const auto offset=static_cast<std::size_t>(span.guest_address-ram_base);
        for (std::size_t i=0; i<span.bytes.size(); ++i) ram[(offset+i)^3u]=span.bytes[i];
    }
}
} // namespace isle::beach
