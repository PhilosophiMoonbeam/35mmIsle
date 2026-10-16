#include "model_data_decoder.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>

namespace isle::model_assets {
namespace {
void require(bool condition, const char* message, std::uint32_t address = 0) {
    if (!condition) throw std::runtime_error(std::string(message) + " at guest " + std::to_string(address));
}
float finite(const rom::GuestView& wire, std::size_t offset) {
    const auto value = wire.f32(offset);
    require(std::isfinite(value), "Nonfinite source model scalar", wire.guest_address);
    return value;
}
menu::Vec3 vector3(const rom::GuestView& wire, std::size_t offset) {
    return {finite(wire,offset),finite(wire,offset+4),finite(wire,offset+8)};
}
}
DataDecoder::DataDecoder(const Reader& reader, std::vector<menu::Material>& materials,
                         std::vector<menu::Animation>& animations,
                         std::span<const MaterialExtent> extents)
    : reader_(reader), materials_(materials), animations_(animations), material_extents_(extents) {}

std::size_t DataDecoder::material(std::uint32_t address) {
    for (std::size_t i=0; i<materials_.size(); ++i)
        if (materials_[i].wire.guest_address==address) return i;
    const auto fact=std::find_if(material_extents_.begin(),material_extents_.end(),
        [address](const auto& value) { return value.material==address; });
    require(fact!=material_extents_.end(),"Unknown source Texture identity",address);
    menu::Material result{};
    result.wire=reader_.view(address,0x78);
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
            "Unadmitted source Texture/palette ownership",address);
    require(w.u32(4)==fact->table && fact->images<=fact->slots,
            "Source image table identity/extent mismatch",address);
    if (fact->slots) {
        require(fact->slots<=reader_.remaining_bytes(fact->table)/4,
                "Source image table escapes initialized mapping",address);
        result.image_table=reader_.view(fact->table,fact->slots*4);
        result.images.reserve(fact->images);
        for (std::size_t i=0; i<fact->images; ++i) {
            const auto image=result.image_table.u32(i*4);
            reader_.view(image,1,8); result.images.push_back(image);
        }
        for (std::size_t i=fact->images; i<fact->slots; ++i)
            require(result.image_table.u32(i*4)==0,"Source image NULL slot mismatch",address);
    } else require(!fact->images && !(result.flags&0x17),
                   "NULL images cannot supply image/palette-emitting Texture flags",address);
    require(!(result.flags&0x10) || result.images.size()>=2,"LOD Texture lacks adjacent images",address);
    materials_.push_back(std::move(result));
    return materials_.size()-1;
}

std::size_t DataDecoder::animation(std::uint32_t address, menu::AnimationKind kind,
                                   bool model_callback_installed) {
    if (!address) return menu::no_index;
    const bool callback=kind==menu::AnimationKind::model && model_callback_installed;
    for (std::size_t i=0; i<animations_.size(); ++i)
        if (animations_[i].wire.guest_address==address && animations_[i].kind==kind &&
            animations_[i].callback_installed==callback) return i;
    menu::Animation result{}; result.kind=kind; result.callback_installed=callback;
    const auto available=reader_.remaining_bytes(address);
    result.commands.reserve(16);
    std::size_t bytes=0, positive_wait=0;
    for (;;) {
        require(available-bytes>=4,"Source animation lacks bounded terminal",address);
        const auto head=reader_.view(address+static_cast<std::uint32_t>(bytes),4);
        const auto word=head.u32(0), opcode=word>>25, mask=(word>>15)&1023u, duration=word&32767u;
        const auto count=static_cast<std::size_t>(std::popcount(mask));
        std::size_t words=1; bool floats=false, adds_wait=false;
        switch (opcode) {
        case 0: require(word==0,"Malformed source animation END",head.guest_address); break;
        case 2: require(mask==0,"Malformed source animation WAIT",head.guest_address); adds_wait=true; break;
        case 3: case 4: case 7: case 8: case 9: case 10: case 11:
            words+=count; floats=true; adds_wait=opcode==3 || opcode==8 || opcode==10; break;
        case 5: case 6: words+=count*2; floats=true; adds_wait=opcode==5; break;
        case 12: break;
        case 14:
            require(mask==0 && duration==0,"Malformed source SET_ANIMATION",head.guest_address);
            words=2; break;
        case 15: case 16:
            require(kind==menu::AnimationKind::model,"Model-only source animation command",head.guest_address);
            adds_wait=true; break;
        case 18: case 19: case 20: case 21:
            require(kind==menu::AnimationKind::texture && !(mask&~31u),
                    "Invalid source packed-color mask",head.guest_address);
            words+=count; adds_wait=opcode==18 || opcode==20; break;
        default: throw std::runtime_error("Unadmitted source animation opcode at guest "+std::to_string(head.guest_address));
        }
        require(words<=(available-bytes)/4,"Source animation operands escape initialized mapping",head.guest_address);
        const auto wire=reader_.view(head.guest_address,words*4);
        if (floats) for (std::size_t i=1; i<words; ++i) finite(wire,i*4);
        result.commands.push_back({wire,static_cast<std::uint8_t>(opcode),
            static_cast<std::uint16_t>(mask),static_cast<std::uint16_t>(duration)});
        bytes+=words*4;
        if (adds_wait) positive_wait+=duration;
        if (!opcode) break;
        if (opcode==14) {
            // The selected families use framed self-cycles, not interior jumps.
            require(wire.u32(4)==address && positive_wait>0,
                    "Source animation cycle lacks framed entry/positive wait",wire.guest_address);
            break;
        }
    }
    result.wire=reader_.view(address,bytes);
    animations_.push_back(std::move(result));
    return animations_.size()-1;
}

void DataDecoder::validate_material_animation(std::size_t stream, std::size_t index) const {
    if (stream==menu::no_index) return;
    const auto& material=materials_.at(index);
    for (const auto& command:animations_.at(stream).commands) {
        if (command.opcode<3 || command.opcode>11) continue;
        std::size_t operand=1;
        for (unsigned bit=0; bit<10; ++bit) if (command.mask&(1u<<bit)) {
            if (bit==0 || bit==5) {
                const auto value=std::trunc(finite(command.wire,operand*4));
                require((command.opcode==3 || command.opcode==4 || command.opcode==10 || command.opcode==11) &&
                        value>=0 && value<static_cast<double>(material.images.size()),
                        "Source image channel escapes bounded image entries",command.wire.guest_address);
            }
            require(bit!=9,"Unadmitted dynamic source palette channel",command.wire.guest_address);
            require(bit!=8 || !(material.flags&0x10),"Unadmitted animated source LOD",command.wire.guest_address);
            operand+=(command.opcode==5 || command.opcode==6) ? 2 : 1;
        }
    }
}

menu::Model DataDecoder::model(std::uint32_t tree, std::uint32_t material_table,
                               std::size_t nodes, menu::RenderType render_type) {
    require(nodes && nodes<reader_.remaining_bytes(tree)/44,"Source tree escapes initialized mapping",tree);
    menu::Model result{}; result.render_type=render_type;
    result.tree=reader_.view(tree,(nodes+1)*44); result.nodes.reserve(nodes);
    if (material_table) result.material_table=reader_.view(material_table,nodes*4);
    std::array<std::size_t,18> ancestors; ancestors.fill(menu::no_index);
    for (std::size_t i=0; i<nodes; ++i) {
        menu::Node node{}; node.wire=reader_.view(tree+static_cast<std::uint32_t>(i*44),44);
        node.depth_flags=node.wire.u32(0); const auto depth=node.depth_flags&4095u;
        require(depth<ancestors.size() && !(node.depth_flags&0xffff0000u) && ((depth==0)==(i==0)),
                "Malformed source tree depth",node.wire.guest_address);
        if (depth) {
            node.parent=ancestors[depth-1];
            require(node.parent!=menu::no_index,"Orphan source model node",node.wire.guest_address);
        }
        ancestors[depth]=i;
        for (std::size_t d=depth+1; d<ancestors.size(); ++d) ancestors[d]=menu::no_index;
        node.position=vector3(node.wire,8); node.rotation=vector3(node.wire,20); node.scale=vector3(node.wire,32);
        if (const auto payload=node.wire.u32(4)) {
            if (render_type==menu::RenderType::b || render_type==menu::RenderType::b_fogged)
                node.payloads.push_back({{},0,0,payload});
            else if (render_type==menu::RenderType::i_fogged) {
                const auto p=reader_.view(payload,8); node.payloads.push_back({p,0,p.u32(0),p.u32(4)});
            } else {
                require(render_type==menu::RenderType::d_fogged || render_type==menu::RenderType::j_fogged,
                        "Unadmitted source model payload recipe",node.wire.guest_address);
                const std::size_t stride=render_type==menu::RenderType::d_fogged ? 8 : 12;
                const auto available=reader_.remaining_bytes(payload)/stride;
                bool terminated=false;
                for (std::size_t j=0; j<available; ++j) {
                    const auto p=reader_.view(payload+static_cast<std::uint32_t>(j*stride),stride);
                    const auto id=p.u32(0);
                    if (id==4) { terminated=true; break; }
                    require(id<4,"Unknown source model lane",p.guest_address);
                    node.payloads.push_back({p,id,stride==12 ? p.u32(4) : 0,p.u32(stride==12 ? 8 : 4)});
                }
                require(terminated,"Source payload lacks mapped lane sentinel",payload);
            }
        }
        if (!result.material_table.bytes.empty()) if (const auto list=result.material_table.u32(i*4)) {
            const auto available=reader_.remaining_bytes(list)/4;
            bool terminated=false;
            for (std::size_t j=0; j<available; ++j) {
                const auto target=reader_.view(list+static_cast<std::uint32_t>(j*4),4).u32(0);
                if (!target) { terminated=true; break; }
                node.materials.push_back(material(target));
            }
            require(terminated,"Source material attachments lack mapped NULL sentinel",list);
        }
        result.nodes.push_back(std::move(node));
    }
    // The source consumes only the sentinel id, never its unused trailing40.
    require(result.tree.u32(nodes*44)==18,"Source tree missing sentinel",tree);
    return result;
}

void DataDecoder::bind_initial_animations(menu::Model& model, std::uint32_t model_table,
                                          std::uint32_t material_table, bool callback) {
    const auto nodes=model.nodes.size();
    if (model_table) model.animation_table=reader_.view(model_table,nodes*4);
    if (material_table) model.material_animation_table=reader_.view(material_table,nodes*4);
    for (std::size_t i=0; i<nodes; ++i) {
        auto& node=model.nodes[i];
        if (model_table) node.animation=animation(model.animation_table.u32(i*4),menu::AnimationKind::model,callback);
        if (material_table && !node.materials.empty()) {
            if (const auto list=model.material_animation_table.u32(i*4)) {
                const auto table=reader_.view(list,node.materials.size()*4);
                node.material_animations.reserve(node.materials.size());
                for (std::size_t j=0; j<node.materials.size(); ++j) {
                    const auto stream=animation(table.u32(j*4),menu::AnimationKind::texture);
                    validate_material_animation(stream,node.materials[j]);
                    node.material_animations.push_back(stream);
                }
            }
        }
    }
}
} // namespace isle::model_assets
