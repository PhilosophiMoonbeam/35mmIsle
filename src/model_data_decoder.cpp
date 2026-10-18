#include "model_data_decoder.hpp"
#include "menu_animation.hpp"

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
menu::Material metadata(const Reader& reader, std::uint32_t address) {
    menu::Material result{};
    result.wire=reader.view(address,0x78);
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
            result.auxiliary_size_code<=3,"Invalid source Texture format",address);
    return result;
}
}
DataDecoder::DataDecoder(const Reader& reader, std::vector<menu::Material>& materials,
                         std::vector<menu::Animation>& animations,
                         std::span<const MaterialExtent> extents,
                         std::vector<menu::AnimationPath>* paths)
    : reader_(reader), materials_(materials), animations_(animations), material_extents_(extents), paths_(paths) {}

std::size_t DataDecoder::material(std::uint32_t address) {
    for (std::size_t i=0; i<materials_.size(); ++i)
        if (materials_[i].wire.guest_address==address) return i;
    const auto fact=std::find_if(material_extents_.begin(),material_extents_.end(),
        [address](const auto& value) { return value.material==address; });
    require(fact!=material_extents_.end(),"Unknown source Texture identity",address);
    return material(address,*fact);
}

std::size_t DataDecoder::material(std::uint32_t address, const MaterialExtent& fact) {
    require(fact.material==address,"Source Texture identity mismatch",address);
    for (std::size_t i=0; i<materials_.size(); ++i)
        if (materials_[i].wire.guest_address==address) {
            require(materials_[i].images.size()==fact.images && materials_[i].palettes.size()==fact.palettes &&
                    materials_[i].wire.u32(4)==fact.table && materials_[i].wire.u32(44)==fact.palette_table,
                    "Source Texture was admitted with a different domain",address);
            return i;
        }
    auto result=metadata(reader_,address);
    const auto& w=result.wire;
    require(w.u32(4)==fact.table && fact.images<=fact.slots &&
            w.u32(44)==fact.palette_table && fact.palettes<=fact.palette_slots,
            "Source Texture pointer table identity/extent mismatch",address);
    auto table=[&](std::uint32_t pointer, std::size_t entries, std::size_t slots,
                   rom::GuestView& wire, std::vector<std::uint32_t>& values) {
        if (!slots) { require(!entries,"Empty source Texture table domain",pointer); return; }
        require(slots<=reader_.remaining_bytes(pointer)/4,
                "Source Texture table escapes initialized mapping",pointer);
        wire=reader_.view(pointer,slots*4);
        values.reserve(entries);
        for (std::size_t i=0; i<entries; ++i) {
            const auto value=wire.u32(i*4);
            reader_.view(value,1,8); values.push_back(value);
        }
        for (std::size_t i=entries; i<slots; ++i)
            require(!wire.u32(i*4),"Source Texture NULL slot mismatch",pointer);
    };
    table(fact.table,fact.images,fact.slots,result.image_table,result.images);
    table(fact.palette_table,fact.palettes,fact.palette_slots,result.palette_table,result.palettes);
    const auto initial=describe_material(address);
    require(fact.images>=initial.images && fact.palettes>=initial.palettes,
            "Source Texture lacks initial renderer-consumed entries",address);
    materials_.push_back(std::move(result));
    return materials_.size()-1;
}

MaterialExtent DataDecoder::describe_material(std::uint32_t address,
                                               std::span<const std::size_t> streams) const {
    const auto value=metadata(reader_,address);
    const auto domain=menu::animation::texture_domain(value,animations_,streams,
        paths_ ? std::span<const menu::AnimationPath>(*paths_) : std::span<const menu::AnimationPath>{});
    require(static_cast<bool>(domain),domain.status.error==menu::animation::Error::unsupported ?
        "Source material requires an explicit sampling capability" : "Source material animation domain is not bounded",
        domain.status.address);
    return {address,value.wire.u32(4),domain.images,domain.images,
            value.wire.u32(44),domain.palettes,domain.palettes};
}

std::size_t DataDecoder::sampled_material(std::uint32_t address,
    std::span<const menu::animation::TextureSample> samples,
    std::span<const std::size_t> universal_streams) {
    const auto value=metadata(reader_,address);
    const auto paths=paths_ ? std::span<const menu::AnimationPath>(*paths_) : std::span<const menu::AnimationPath>{};
    const auto domain=menu::animation::texture_sample_domain(value,animations_,samples,paths);
    const auto universal=menu::animation::texture_domain(value,animations_,universal_streams,paths);
    require(static_cast<bool>(domain),"Source material sampling domain is not bounded",domain.status.address);
    require(static_cast<bool>(universal),universal.status.error==menu::animation::Error::unsupported ?
        "Source live material requires its own sampling capability" : "Source material universal domain is not bounded",
        universal.status.address);
    const auto images=std::max(domain.images,universal.images), palettes=std::max(domain.palettes,universal.palettes);
    const MaterialExtent extent{address,value.wire.u32(4),images,images,
        value.wire.u32(44),palettes,palettes};
    const auto index=material(address,extent);
    for (const auto& sample : samples) if (sample.program!=menu::no_index &&
        std::none_of(sampled_bindings_.begin(),sampled_bindings_.end(),
            [&](const auto& binding) { return binding.material==address && binding.stream==sample.program; }))
        sampled_bindings_.push_back({address,sample.program});
    return index;
}

std::size_t DataDecoder::path(std::uint32_t address) {
    require(paths_ && address,"Source path requires retained native path storage",address);
    for (std::size_t i=0; i<paths_->size(); ++i)
        if ((*paths_)[i].wire.guest_address==address) return i;
    menu::AnimationPath result{};
    result.wire=reader_.view(address,24);
    const auto& w=result.wire;
    result.kind=w.u8(0);
    const auto count=std::bit_cast<std::int16_t>(w.u16(2));
    require(result.kind<=3 && count>=2,"Invalid source interpolation kind/count",address);
    result.segment_point_count=static_cast<std::size_t>(count);
    result.tension=finite(w,4); result.duration=finite(w,12);
    require(result.kind==0 || result.duration>0,"Invalid source path duration",address);
    // Bounded position controls only. Source velocity's non-linear t==1
    // terminal-segment overread does not authorize unrelated trailing data;
    // native path_velocity retains its explicit control-extent rejection.
    const auto point_count=result.kind==0 ? result.segment_point_count :
        result.kind==1 ? result.segment_point_count*3-2 : result.segment_point_count+2;
    const auto points=reader_.view(w.u32(8),point_count*12);
    for (std::size_t i=0; i<point_count; ++i)
        result.points.push_back(vector3(points,i*12));
    const auto times=reader_.view(w.u32(16),result.segment_point_count*4);
    for (std::size_t i=0; i<result.segment_point_count; ++i) {
        const auto time=finite(times,i*4);
        require(time>=0 && time<=1 && (i==0 || time>result.timestamps.back()),
                "Invalid source path timestamp interval",address);
        result.timestamps.push_back(time);
    }
    require(result.timestamps.front()==0 && result.timestamps.back()==1,
            "Source path lacks exact endpoints",address);
    if (result.kind!=0) {
        const auto speeds=reader_.view(w.u32(20),(result.segment_point_count-1)*20);
        for (std::size_t i=0; i<(result.segment_point_count-1)*5; ++i)
            result.speed_polynomials.push_back(finite(speeds,i*4));
    }
    paths_->push_back(std::move(result));
    return paths_->size()-1;
}

std::size_t DataDecoder::animation(std::uint32_t address, menu::AnimationKind kind,
                                   bool model_callback_installed) {
    if (!address) return menu::no_index;
    const bool callback=kind==menu::AnimationKind::model && model_callback_installed;
    for (std::size_t i=0; i<animations_.size(); ++i)
        if (animations_[i].wire.guest_address==address && animations_[i].kind==kind &&
            animations_[i].callback_installed==callback) return i;
    menu::Animation result{}; result.kind=kind; result.callback_installed=callback;
    result.commands.reserve(16);
    std::uint32_t cursor=address, fragment=address;
    auto finish_fragment=[&] {
        if (cursor!=fragment) result.fragments.push_back(reader_.view(fragment,cursor-fragment));
    };
    for (;;) {
        require(cursor && !(cursor&3u),"Unaligned source animation target",cursor);
        const auto prior=std::find_if(result.commands.begin(),result.commands.end(),
            [cursor](const auto& c) { return c.wire.guest_address==cursor; });
        if (prior!=result.commands.end()) {
            finish_fragment();
            break;
        }
        for (const auto& c:result.commands)
            require(cursor<c.wire.guest_address ||
                    cursor-c.wire.guest_address>=c.wire.bytes.size(),
                    "Source animation target enters command operands",cursor);
        const auto available=reader_.remaining_bytes(cursor);
        require(available>=4,"Source animation lacks bounded terminal",cursor);
        const auto head=reader_.view(cursor,4);
        const auto word=head.u32(0), opcode=word>>25, mask=(word>>15)&1023u, duration=word&32767u;
        const auto count=static_cast<std::size_t>(std::popcount(mask));
        std::size_t words=1; bool floats=false;
        switch (opcode) {
        case 0: require(word==0,"Malformed source animation END",head.guest_address); break;
        case 1:
            require(mask==0 && duration==0,"Malformed source JUMP",head.guest_address);
            words=2; break;
        case 2: require(mask==0,"Malformed source animation WAIT",head.guest_address); break;
        case 3: case 4: case 7: case 8: case 9: case 10: case 11:
            words+=count; floats=true; break;
        case 5: case 6: words+=count*2; floats=true; break;
        case 12: break;
        case 13:
            require(kind==menu::AnimationKind::model || kind==menu::AnimationKind::camera,
                    "Texture cannot bind an interpolation path",head.guest_address);
            require(kind==menu::AnimationKind::model || (mask && !(mask&~0x88u)),
                    "Invalid source interpolation mask",head.guest_address);
            words+=kind==menu::AnimationKind::camera ? count : 1; break;
        case 14:
            require(mask==0 && duration==0,"Malformed source SET_ANIMATION",head.guest_address);
            words=2; break;
        case 15: case 16: case 17:
            require(kind==menu::AnimationKind::model,"Model-only source animation command",head.guest_address);
            if (opcode==17) { words+=count; floats=true; }
            break;
        case 18: case 19: case 20: case 21:
            require(kind==menu::AnimationKind::texture && !(mask&~31u),
                    "Invalid source packed-color mask",head.guest_address);
            words+=count; break;
        case 22:
            require(kind==menu::AnimationKind::texture && !(mask&~31u),
                    "Invalid source texture word mask",head.guest_address);
            words+=count; break;
        case 23:
            require(kind==menu::AnimationKind::camera,"Camera-only source animation command",head.guest_address);
            words=3; break;
        default: throw std::runtime_error("Unadmitted source animation opcode at guest "+std::to_string(head.guest_address));
        }
        require(words<=available/4,"Source animation operands escape initialized mapping",head.guest_address);
        const auto wire=reader_.view(head.guest_address,words*4);
        if (floats) for (std::size_t i=1; i<words; ++i) finite(wire,i*4);
        result.commands.push_back({wire,static_cast<std::uint8_t>(opcode),
            static_cast<std::uint16_t>(mask),static_cast<std::uint16_t>(duration)});
        if (opcode==13) {
            auto& command=result.commands.back();
            std::size_t operand=4;
            if (kind==menu::AnimationKind::model || (mask&8)) {
                command.path_index=path(wire.u32(operand)); operand+=4;
            }
            if (kind==menu::AnimationKind::camera && (mask&128))
                command.camera_at_path_index=path(wire.u32(operand));
        }
        require(words*4<=UINT32_MAX-cursor,"Source animation address overflow",cursor);
        const auto end=cursor+static_cast<std::uint32_t>(words*4);
        for (std::size_t i=0; i+1<result.commands.size(); ++i) {
            const auto& c=result.commands[i];
            require(end<=c.wire.guest_address || cursor>=c.wire.guest_address+c.wire.bytes.size(),
                    "Overlapping source animation command",cursor);
        }
        cursor=end;
        if (!opcode || opcode==1 || opcode==14) {
            finish_fragment();
            if (!opcode || !wire.u32(4)) break;
            cursor=fragment=wire.u32(4);
        }
    }
    result.wire=result.fragments.front();
    animations_.push_back(std::move(result));
    return animations_.size()-1;
}

void DataDecoder::validate_material_animation(std::size_t stream, std::size_t index) const {
    if (stream==menu::no_index) return;
    const auto& material=materials_.at(index);
    require(stream<animations_.size() && animations_[stream].kind==menu::AnimationKind::texture,
            "Source material binding is not a decoded texture program",material.wire.guest_address);
    const auto declared=std::find_if(material_extents_.begin(),material_extents_.end(),
        [&](const auto& extent) { return extent.material==material.wire.guest_address; });
    if (declared!=material_extents_.end()) {
        // Source-declared pointer allocations are independent of playback.
        // Their initialized entries/NULL tail were checked by material(); every
        // rendered pose still bounds-checks its actual indices before RAM writes.
        // Seek-domain inference is needed only when deriving an unknown extent,
        // not to reinterpret an already authoritative allocation as STEP-only.
        require(material.images.size()==declared->images && material.palettes.size()==declared->palettes &&
                material.wire.u32(4)==declared->table && material.wire.u32(44)==declared->palette_table,
                "Source material differs from its declared allocation",material.wire.guest_address);
        return;
    }
    if (std::any_of(sampled_bindings_.begin(),sampled_bindings_.end(),
        [&](const auto& binding) {
            return binding.material==material.wire.guest_address && binding.stream==stream;
        })) return; // Already proved under this cold owner's explicit capability.
    const std::array<std::size_t,1> streams{stream};
    const auto domain=describe_material(material.wire.guest_address,streams);
    require(domain.images<=material.images.size() && domain.palettes<=material.palettes.size(),
            "Source material animation escapes admitted pointer domains",material.wire.guest_address);
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
            else if (render_type==menu::RenderType::i_fogged || render_type==menu::RenderType::i_no_fog) {
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
