#include "pidgey_resources.hpp"

#include <bit>
#include <cmath>
#include <initializer_list>
#include <limits>
#include <stdexcept>

namespace isle::pidgey {
namespace {
static_assert(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559);
constexpr std::uint32_t ram_base = 0x80000000;
constexpr std::uint32_t model_guest = 0x801d2180;
constexpr std::uint32_t payload_guest = 0x801d2108;
constexpr std::uint32_t material_table_guest = 0x801cdc40;
constexpr std::uint32_t material_guest = 0x801cdc90;
constexpr std::uint32_t material_list_guest = 0x801cdd08;
constexpr std::uint32_t images_guest = 0x801cdc84;
constexpr std::uint32_t vertex_guest = 0x801cdd10;
constexpr std::size_t tree_record_size = 44;

// Pinned splat.yaml segment bases. The end bounds deliberately cover only the
// admitted region needed here, not arbitrary guest RAM or neighboring overlays.
struct Mapping { std::size_t rom_begin, rom_end; std::uint32_t guest_begin; };
constexpr Mapping extra{0x13c780, 0x15e910, 0x801b0310};
constexpr Mapping assets{0x0a74e0, 0x13c780, 0x8011b050};
constexpr Mapping app{0x046270, 0x059f30, 0x8009a8c0};

// Public symbol/extent facts only. Payload bytes always come from the Snapshot.
constexpr std::array<std::uint32_t, display_list_count> list_guests{
    0x801d02b0, 0x801d02e8, 0x801d0428, 0x801d0488, 0x801d0570,
    0x801d05b0, 0x801d06f0, 0x801d0968, 0x801d09a8, 0x801d0aa8,
    0x801d0d20, 0x801d0d80, 0x801d0fd8, 0x801d1038, 0x801d1510,
    0x801d1690, 0x801d16c8, 0x801d1838, 0x801d19c8, 0x801d1bb8,
    0x801d1c00, 0x801d1d58, 0x801d1ee8
};
struct TextureFact {
    std::uint32_t guest;
    TextureKind kind;
    std::uint16_t width, height;
};
constexpr std::array<TextureFact, texture_count> texture_facts{{
    {0x8019e4f8, TextureKind::rgba16_palette, 16, 1},
    {0x8019e520, TextureKind::ci4, 32, 32},
    {0x8019e728, TextureKind::rgba16_palette, 16, 1},
    {0x8019e750, TextureKind::ci4, 64, 32},
    {0x8019eb58, TextureKind::rgba16_palette, 16, 1},
    {0x8019eb80, TextureKind::ci4, 32, 32},
    {0x8019ed88, TextureKind::rgba16_palette, 16, 1},
    {0x8019edb0, TextureKind::ci4, 32, 32},
    {0x8019efb8, TextureKind::rgba16_palette, 16, 1},
    {0x8019efe0, TextureKind::ci4, 64, 32},
    {0x8019f3e8, TextureKind::rgba16_palette, 16, 1},
    {0x8019f410, TextureKind::ci4, 32, 32},
    {0x8019f618, TextureKind::rgba16_palette, 256, 1},
    {0x8019f820, TextureKind::ci8, 32, 32},
    {0x8019fe30, TextureKind::ci8, 32, 32},
    {0x801a0238, TextureKind::rgba16_palette, 16, 1},
    {0x801a0260, TextureKind::ci4, 32, 32},
    {0x801a0468, TextureKind::rgba16_palette, 16, 1},
    {0x801a0490, TextureKind::ci4, 32, 32},
    {0x801a0698, TextureKind::rgba16_palette, 16, 1},
    {0x801a06c0, TextureKind::ci4, 32, 32}
}};
struct StreamFact { std::uint32_t guest; std::size_t size; };
// Extents include END, framed from GAME AnimCommand and anim.c:269-513.
// These are resource boundaries, not an animation evaluator or copied commands.
constexpr std::array<StreamFact, node_count> stream_facts{{
    {0x801a6714, 0xb8}, {0, 0}, {0x801a67cc, 0xb0},
    {0x801a687c, 0x1b0}, {0x801a6a2c, 0x94}, {0x801a6ac0, 0x44},
    {0x801a6b04, 0xb0}, {0x801a6bb4, 0x5c}, {0x801a6c10, 0xb0},
    {0x801a6cc0, 0xb0}, {0x801a6d70, 0x44}, {0x801a6db4, 0x52c},
    {0x801a72e0, 0x52c}, {0x801a780c, 0x52c}, {0x801a7d38, 0x52c},
    {0x801a8264, 0x52c}, {0x801a8790, 0x52c}
}};

void require(bool condition) {
    if (!condition) throw std::runtime_error("Invalid admitted HD Pidgey resource graph");
}
rom::GuestView view(const rom::Snapshot& snapshot, Mapping mapping, std::uint32_t guest,
             std::size_t size, std::size_t alignment = 4) {
    require(guest >= mapping.guest_begin && guest % alignment == 0 && size != 0);
    const auto delta = static_cast<std::size_t>(guest - mapping.guest_begin);
    require(delta <= mapping.rom_end - mapping.rom_begin);
    const auto offset = mapping.rom_begin + delta;
    require(size <= mapping.rom_end - offset);
    const auto bytes = snapshot.canonical();
    require(offset <= bytes.size() && size <= bytes.size() - offset);
    return {guest, offset, bytes.subspan(offset, size)};
}
Vec3 vec3(const rom::GuestView& wire, std::size_t offset) {
    Vec3 value{wire.f32(offset), wire.f32(offset + 4), wire.f32(offset + 8)};
    require(std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z));
    return value;
}
std::array<std::uint8_t, 4> bytes4(const rom::GuestView& wire, std::size_t offset) {
    return {wire.u8(offset), wire.u8(offset + 1), wire.u8(offset + 2), wire.u8(offset + 3)};
}
std::uint8_t list_index(std::uint32_t guest) {
    if (guest == 0) return no_index;
    for (std::size_t i = 0; i < list_guests.size(); ++i)
        if (list_guests[i] == guest) return static_cast<std::uint8_t>(i);
    throw std::runtime_error("HD Pidgey payload references an unknown display list");
}
std::uint8_t texture_index(std::uint32_t guest) {
    for (std::size_t i = 0; i < texture_facts.size(); ++i)
        if (texture_facts[i].guest == guest) return static_cast<std::uint8_t>(i);
    throw std::runtime_error("HD Pidgey references an unknown texture");
}
void validate_stream(const rom::GuestView& wire) {
    std::size_t offset = 0;
    while (offset < wire.bytes.size()) {
        const auto word = wire.u32(offset);
        const auto opcode = word >> 25;
        const auto values = static_cast<std::size_t>(std::popcount((word >> 15) & 0x3ffu));
        std::size_t words = 1;
        switch (opcode) {
        case 0: // END, not just an arbitrary zero float in command operands.
            require(word == 0 && offset + 4 == wire.bytes.size());
            return;
        case 3: case 4: case 7: case 8: case 9: case 10: case 11: case 17:
            words += values;
            break;
        case 5: case 6:
            words += 2 * values;
            break;
        case 2: case 12: case 15: case 16:
            break;
        default:
            // No jumps, interpolation pointers, or opaque commands occur in
            // this exact pose. Never pretend a branch's target is installed.
            require(false);
        }
        require(words <= (wire.bytes.size() - offset) / 4);
        offset += words * 4;
    }
    require(false); // Every admitted stream must contain its framed END.
}
} // namespace

Command DisplayList::command(std::size_t index) const {
    if (index >= command_count()) throw std::out_of_range("HD Pidgey display-list command index");
    return {wire.u32(index * 8), wire.u32(index * 8 + 4)};
}

HdResources::HdResources(const rom::Snapshot& snapshot) : snapshot_(&snapshot) {
    constexpr std::array<std::uint8_t, 20> sha1{
        0xed, 0xc7, 0xc4, 0x9c, 0xc5, 0x68, 0xc0, 0x45, 0xfe, 0x48,
        0xbe, 0x0d, 0x18, 0x01, 0x1c, 0x30, 0xf3, 0x93, 0xcb, 0xaf
    };
    require(snapshot.canonical().size() == rom::us_size && snapshot.identity().size == rom::us_size &&
            snapshot.identity().sha1 == sha1);
    tree_ = view(snapshot, extra, model_guest, tree_record_count * tree_record_size);
    vertices_ = view(snapshot, extra, vertex_guest, vertex_count * 16, 8);
    require(tree_.rom_offset == 0x15e5f0 && vertices_.rom_offset == 0x15a180);
    const auto payload_wire = view(snapshot, extra, payload_guest, payload_count * 8);
    require(payload_wire.rom_offset == 0x15e578);
    for (std::size_t i = 0; i < lists_.size(); ++i) {
        const auto end = i + 1 < lists_.size() ? list_guests[i + 1] : payload_guest;
        require(end > list_guests[i] && (end - list_guests[i]) % 8 == 0);
        lists_[i].wire = view(snapshot, extra, list_guests[i], end - list_guests[i], 8);
    }
    for (std::size_t i = 0; i < textures_.size(); ++i) {
        const auto& fact = texture_facts[i];
        const auto pixels = static_cast<std::size_t>(fact.width) * fact.height;
        const auto size = fact.kind == TextureKind::rgba16_palette ? pixels * 2 :
                          fact.kind == TextureKind::ci4 ? pixels / 2 : pixels;
        textures_[i] = {view(snapshot, assets, fact.guest, size, 8), fact.kind, fact.width, fact.height};
    }
    for (std::size_t i = 0; i < payloads_.size(); ++i) {
        auto& payload = payloads_[i];
        payload.guest_address = payload_guest + static_cast<std::uint32_t>(i * 8);
        payload.pre_guest = payload_wire.u32(i * 8);
        payload.draw_guest = payload_wire.u32(i * 8 + 4);
        payload.pre_index = list_index(payload.pre_guest);
        payload.draw_index = list_index(payload.draw_guest);
        require(payload.draw_index != no_index);
    }

    std::array<std::uint8_t, 6> ancestors;
    ancestors.fill(no_index);
    std::array<bool, payload_count> seen_payload{};
    for (std::size_t i = 0; i < nodes_.size(); ++i) {
        auto& node = nodes_[i];
        const auto offset = i * tree_record_size;
        node.depth_flags = tree_.u32(offset);
        node.payload_guest = tree_.u32(offset + 4);
        // Pinned Pidgey has no matrix-kind override flag bits and max depth 5.
        require(node.depth_flags < ancestors.size());
        const auto depth = static_cast<std::size_t>(node.depth_flags);
        require((depth == 0) == (i == 0));
        if (depth != 0) {
            require(ancestors[depth - 1] != no_index);
            node.parent = ancestors[depth - 1];
        }
        ancestors[depth] = static_cast<std::uint8_t>(i);
        for (std::size_t d = depth + 1; d < ancestors.size(); ++d) ancestors[d] = no_index;
        if (node.payload_guest != 0) {
            require(node.payload_guest >= payload_guest && (node.payload_guest - payload_guest) % 8 == 0);
            const auto index = (node.payload_guest - payload_guest) / 8;
            require(index < payloads_.size() && !seen_payload[index]);
            node.payload_index = static_cast<std::uint8_t>(index);
            seen_payload[index] = true;
        }
        node.position = vec3(tree_, offset + 8);
        node.rotation = vec3(tree_, offset + 20);
        node.scale = vec3(tree_, offset + 32);
        require(node.scale.x > 0 && node.scale.y > 0 && node.scale.z > 0);
    }
    for (bool seen : seen_payload) require(seen);
    const auto sentinel = node_count * tree_record_size;
    require(tree_.u32(sentinel) == 18);
    for (std::size_t i = 4; i < tree_record_size; i += 4) require(tree_.u32(sentinel + i) == 0);

    const auto materials = view(snapshot, extra, material_table_guest, node_count * 4);
    for (std::size_t i = 0; i < node_count; ++i) {
        node_materials_[i] = materials.u32(i * 4);
        require(node_materials_[i] == (i == 9 ? material_list_guest : 0));
    }
    const auto material_list = view(snapshot, extra, material_list_guest, 8);
    require(material_list.u32(0) == material_guest && material_list.u32(4) == 0);
    auto& m = material_;
    m.wire = view(snapshot, extra, material_guest, 0x78);
    const auto& w = m.wire;
    m.pad00 = w.u16(0); m.format = w.u8(2); m.palette_bit_depth = w.u8(3);
    m.images_guest = w.u32(4);
    require(m.images_guest == images_guest);
    const auto images = view(snapshot, extra, m.images_guest, 12);
    for (std::size_t i = 0; i < 2; ++i) {
        m.image_guests[i] = images.u32(i * 4);
        m.image_indices[i] = texture_index(m.image_guests[i]);
        require(textures_[m.image_indices[i]].kind == TextureKind::ci8);
    }
    require(images.u32(8) == 0);
    m.scale = w.u16(8); m.unk0a = w.u16(10);
    m.width_main = w.u16(12); m.height_main = w.u16(14);
    m.unk10 = std::bit_cast<std::int32_t>(w.u32(0x10));
    m.unk14 = w.f32(0x14); m.unk18 = w.f32(0x18);
    m.scale_s = w.f32(0x1c); m.scale_t = w.f32(0x20);
    m.unk24 = w.f32(0x24); m.unk28 = w.f32(0x28);
    m.palettes_guest = w.u32(0x2c); m.flags = w.u16(0x30);
    m.image_format = w.u8(0x32); m.image_bit_depth = w.u8(0x33);
    m.block_width = w.u16(0x34); m.block_height = w.u16(0x36);
    m.width_aux = w.u16(0x38); m.height_aux = w.u16(0x3a);
    m.unk3c = w.f32(0x3c); m.unk40 = w.f32(0x40); m.unk44 = w.f32(0x44);
    m.unk48 = bytes4(w, 0x48); m.unk4c = w.u32(0x4c);
    m.prim_rgba = bytes4(w, 0x50); m.lod_fields = bytes4(w, 0x54);
    m.env_rgba = bytes4(w, 0x58); m.blend_rgba = bytes4(w, 0x5c);
    m.light_color1 = bytes4(w, 0x60); m.light_color2 = bytes4(w, 0x64);
    for (std::size_t i = 0; i < m.trailing.size(); ++i)
        m.trailing[i] = std::bit_cast<std::int32_t>(w.u32(0x68 + i * 4));
    require(m.format == 2 && m.palette_bit_depth == 2 && m.flags == 0xa1 &&
            m.image_format == 2 && m.image_bit_depth == 1 && m.palettes_guest == 0 &&
            m.scale == 32 && m.width_main == 32 && m.height_main == 32 &&
            m.block_width == 32 && m.block_height == 32 && m.width_aux == 32 && m.height_aux == 32);
    for (float value : {m.unk14, m.unk18, m.scale_s, m.scale_t, m.unk24, m.unk28, m.unk3c, m.unk40, m.unk44})
        require(std::isfinite(value));

    auto& selection = selection_;
    selection.render_record = view(snapshot, app, 0x800adf48, 20);
    selection.animation_set_record = view(snapshot, app, 0x800ad6bc, 12);
    const auto& render = selection.render_record;
    const auto& set = selection.animation_set_record;
    require(render.rom_offset == 0x598f8 && set.rom_offset == 0x5906c);
    selection.pokemon_id = render.u32(0); selection.scale = render.f32(4);
    selection.model_guest = render.u32(8); selection.materials_guest = render.u32(12);
    selection.render_callback_guest = render.u32(16);
    selection.pose_table_guest = set.u32(4); selection.pose_count = set.u32(8);
    require(selection.pokemon_id == 16 && set.u32(0) == 16 && selection.scale == 2.0f &&
            selection.model_guest == model_guest && selection.materials_guest == material_table_guest &&
            selection.render_callback_guest == 0x800a1650 && selection.pose_table_guest == 0x800ac414 &&
            selection.pose_count == 4);
    pose0_.record = view(snapshot, app, selection.pose_table_guest, 12);
    require(pose0_.record.rom_offset == 0x57dc4);
    pose0_.pose_id = pose0_.record.f32(0);
    pose0_.material_animation_guest = pose0_.record.u32(8);
    require(pose0_.pose_id == 0.0f && pose0_.material_animation_guest == 0 &&
            pose0_.record.u32(4) == 0x801a66d0);
    pose0_.model_table = view(snapshot, assets, pose0_.record.u32(4), node_count * 4);
    require(pose0_.model_table.rom_offset == 0x132b60);
    for (std::size_t i = 0; i < stream_facts.size(); ++i) {
        const auto& fact = stream_facts[i];
        require(pose0_.model_table.u32(i * 4) == fact.guest);
        if (fact.guest == 0) continue;
        pose0_.model_streams[i] = view(snapshot, assets, fact.guest, fact.size);
        validate_stream(pose0_.model_streams[i]);
    }

    // Validate address-bearing operations against their actual referenced extents.
    // Command words themselves are neither rewritten nor regenerated.
    std::size_t external_count = 0;
    std::array<bool, texture_count> seen_texture{};
    for (const auto index : m.image_indices) seen_texture[index] = true;
    for (const auto& list : lists_) {
        auto image = no_index;
        std::size_t image_texel_bytes = 0;
        bool dynamic_image = false;
        for (std::size_t i = 0; i < list.command_count(); ++i) {
            const auto cmd = list.command(i);
            switch (cmd.w0 >> 24) {
            case 0x01: { // Actual F3DEX2 vertex command, count and cache end slot.
                const auto count = (cmd.w0 >> 12) & 0xffu;
                const auto end_slot = (cmd.w0 & 0xfffu) >> 1;
                require(count > 0 && end_slot >= count && end_slot <= 32 && cmd.w1 >= vertex_guest);
                const auto offset = static_cast<std::size_t>(cmd.w1 - vertex_guest);
                require(offset % 16 == 0 && offset <= vertices_.bytes.size() &&
                        count * 16 <= vertices_.bytes.size() - offset);
                break;
            }
            case 0xde:
                require(cmd.w0 == 0xde000000);
                if (cmd.w1 == 0x0e000000) {
                    ++external_count;
                    external_ = {list.wire.guest_address + static_cast<std::uint32_t>(i * 8), cmd.w1};
                    image = no_index;
                    dynamic_image = true;
                } else {
                    const auto target = list_index(cmd.w1);
                    // The sole immutable call is hd_first -> hd_part0_draw.
                    require(list.wire.guest_address == list_guests[0] && target == 1);
                }
                break;
            case 0xfd: {
                image = texture_index(cmd.w1);
                seen_texture[image] = true;
                const auto size_code = (cmd.w0 >> 19) & 3u;
                require(size_code == 2); // Lists load these source words as 16-bit texels.
                image_texel_bytes = 2;
                dynamic_image = false;
                break;
            }
            case 0xf0: { // TLUT entry count; palettes may consume fewer than 16 colors.
                require(image != no_index && textures_[image].kind == TextureKind::rgba16_palette);
                const auto count = ((cmd.w1 >> 14) & 0x3ffu) + 1;
                require(count * 2 <= textures_[image].wire.bytes.size());
                break;
            }
            case 0xf3: {
                const auto count = ((cmd.w1 >> 12) & 0xfffu) + 1;
                if (image != no_index) {
                    require(textures_[image].kind != TextureKind::rgba16_palette &&
                            count * image_texel_bytes <= textures_[image].wire.bytes.size());
                } else {
                    require(dynamic_image); // Supplied by the genuine segment-0E material producer.
                }
                break;
            }
            case 0xdf:
                require(cmd.w0 == 0xdf000000 && cmd.w1 == 0 && i + 1 == list.command_count());
                break;
            case 0x02: case 0x05: case 0x06: case 0xd7: case 0xe2: case 0xe3:
            case 0xe6: case 0xe7: case 0xe8: case 0xf2: case 0xf5: case 0xf9: case 0xfc:
                break;
            default:
                require(false);
            }
        }
        const auto last = list.command(list.command_count() - 1);
        require(last.w0 == 0xdf000000 && last.w1 == 0);
    }
    require(external_count == 1 && external_.command_guest == 0x801d1070);
    for (bool seen : seen_texture) require(seen);

    std::size_t next = 0;
    // Exact HD TU resources, ending after the tree sentinel; do not write the
    // next eight alignment bytes or any part of the following Scyther bank.
    install_[next++] = view(snapshot, extra, material_table_guest,
                            model_guest + tree_record_count * tree_record_size - material_table_guest, 8);
    for (const auto& texture : textures_) install_[next++] = texture.wire;
    install_[next++] = selection.render_record;
    install_[next++] = selection.animation_set_record;
    install_[next++] = pose0_.record;
    install_[next++] = pose0_.model_table;
    for (const auto& stream : pose0_.model_streams)
        if (!stream.bytes.empty()) install_[next++] = stream;
    require(next == install_.size());
    for (std::size_t i = 0; i < install_.size(); ++i) {
        const auto& span = install_[i];
        require(span.guest_address >= ram_base && span.bytes.size() % 4 == 0);
        const auto offset = static_cast<std::size_t>(span.guest_address - ram_base);
        require(offset <= rdram_size && span.bytes.size() <= rdram_size - offset);
        for (std::size_t j = 0; j < i; ++j) {
            const auto& previous = install_[j];
            const auto end = static_cast<std::uint64_t>(span.guest_address) + span.bytes.size();
            const auto previous_end = static_cast<std::uint64_t>(previous.guest_address) + previous.bytes.size();
            require(end <= previous.guest_address || previous_end <= span.guest_address);
        }
    }
}

Vertex HdResources::vertex(std::size_t index) const {
    if (index >= vertex_count) throw std::out_of_range("HD Pidgey vertex index");
    const auto offset = index * 16;
    return {{std::bit_cast<std::int16_t>(vertices_.u16(offset)),
             std::bit_cast<std::int16_t>(vertices_.u16(offset + 2)),
             std::bit_cast<std::int16_t>(vertices_.u16(offset + 4))},
            vertices_.u16(offset + 6),
            {std::bit_cast<std::int16_t>(vertices_.u16(offset + 8)),
             std::bit_cast<std::int16_t>(vertices_.u16(offset + 10))},
            bytes4(vertices_, offset + 12)};
}
std::size_t HdResources::installed_byte_count() const noexcept {
    std::size_t count = 0;
    for (const auto& span : install_) count += span.bytes.size();
    return count;
}
void HdResources::install(std::span<std::byte> ram) const {
    if (ram.size() != rdram_size)
        throw std::invalid_argument("HD Pidgey install requires caller-owned word-swapped 8 MiB RAM");
    const auto canonical = snapshot_->canonical();
    require(canonical.size() == rom::us_size &&
            canonical.data() + tree_.rom_offset == tree_.bytes.data());
    for (const auto& span : install_) {
        const auto offset = static_cast<std::size_t>(span.guest_address - ram_base);
        // A byte-wise word swap is alignment-safe and independent of host endian.
        for (std::size_t i = 0; i < span.bytes.size(); ++i) ram[(offset + i) ^ 3u] = span.bytes[i];
    }
}
} // namespace isle::pidgey
