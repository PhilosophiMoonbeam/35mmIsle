#include "menu_resources.hpp"
#include "effect_bank_resources.hpp"

#include "35mmisle/vpk0.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace isle::menu {
namespace {
struct Failure { Status status; };
void require(bool condition, Error error = Error::malformed, std::uint32_t address = 0) {
    if (!condition) throw Failure{{error, address}};
}
constexpr std::array<std::uint8_t, 20> admitted_sha1{
    0xed, 0xc7, 0xc4, 0x9c, 0xc5, 0x68, 0xc0, 0x45, 0xfe, 0x48,
    0xbe, 0x0d, 0x18, 0x01, 0x1c, 0x30, 0xf3, 0x93, 0xcb, 0xaf
};
constexpr std::array<std::uint32_t, 64> sprite_roots{
    0x802dabc0, 0x802db328, 0x802db588, 0x802dbce8, 0x802dc1a8, 0x802dc718,
    0x802dc978, 0x802dce88, 0x802dfab0, 0x802e8dd0, 0x802f20f0, 0x802f4828,
    0x802f6490, 0x802f82c8, 0x803051f0, 0x803072a0, 0x80308a00, 0x80309770,
    0x8030a4e0, 0x8030b250, 0x8030bfc0, 0x8030e020, 0x80310080, 0x803120e0,
    0x80312238, 0x803128e0, 0x80316230, 0x80319b80, 0x8031d4d0, 0x80320e20,
    0x80324770, 0x803280c0, 0x8032ba10, 0x8032f360, 0x80332cb0, 0x80336600,
    0x80339f50, 0x8033d8a0, 0x8033e210, 0x8033e938, 0x8033ecd0, 0x8033ed70,
    0x8033f498, 0x8033fbb8, 0x80341160, 0x80341790, 0x80341c70, 0x80342150,
    0x80342630, 0x80342b10, 0x80342ff0, 0x803434d0, 0x80343b28, 0x80343c08,
    0x80344388, 0x803444d8, 0x80344618, 0x8034b020, 0x8034b920, 0x8034bbb8,
    0x8034c4b0, 0x8034c710, 0x8034d3a0, 0x8034fad8
};
struct ModelFact {
    std::uint32_t tree, materials, animation, material_animation;
    std::size_t nodes;
    RenderType type;
    bool callback;
};
constexpr std::array<ModelFact, 5> model_facts{{
    {0x8037f658, 0x8036a458, 0x80381d60, 0x80385700, 226, RenderType::d_fogged, false},
    {0x803a1730, 0x8039bc48, 0x803a1be0, 0x803a7f90, 26, RenderType::j_fogged, false},
    {0x80359a20, 0x80359378, 0x80359b30, 0x80359cd0, 5, RenderType::d_fogged, true},
    {0x8038f3e0, 0x8038ad40, 0x8038f9c0, 0x80393ae0, 33, RenderType::i_fogged, true},
    {0x80393d30, 0, 0x80393dc0, 0, 2, RenderType::b_fogged, false}
}};
struct MaterialFact { std::uint32_t address; std::size_t image_count; };
// Initialized pointer array extents, derived from the admitted bank. In
// particular these arrays are NOT uniformly NULL-terminated.
constexpr std::array<MaterialFact, 10> material_facts{{
    {0x803593a0, 5}, {0x80359430, 6}, {0x803594b0, 2}, {0x8036a7e0, 0},
    {0x8036a860, 2}, {0x8036a8e0, 2}, {0x8036a960, 2}, {0x8036a9e0, 2},
    {0x8038ade0, 5}, {0x8039bcc0, 3}
}};
constexpr std::array<RamRange, 2> framebuffers{{{0x3b5000, 0x25800}, {0x3da800, 0x25800}}};
std::size_t index(Scene scene) {
    switch (scene) {
    case Scene::logo: return 0;
    case Scene::title_menu: return 1;
    }
    throw Failure{{Error::malformed, 0}};
}
rom::GuestView rom_view(const rom::Snapshot& snapshot, std::size_t begin, std::size_t size,
                        std::uint32_t guest = 0) {
    const auto bytes = snapshot.canonical();
    require(begin <= bytes.size() && size <= bytes.size() - begin && size != 0, Error::extent, guest);
    return {guest, begin, bytes.subspan(begin, size)};
}
rom::GuestView bank_view(const BankFacts& bank, std::uint32_t guest, std::size_t size,
                         std::size_t alignment = 4) {
    require(bank.owner && guest >= bank.guest_begin && guest % alignment == 0 && size != 0,
            Error::extent, guest);
    const auto offset = static_cast<std::size_t>(guest - bank.guest_begin);
    require(offset <= bank.owner->size() && size <= bank.owner->size() - offset, Error::extent, guest);
    return {guest, decompressed_rom_offset, std::as_bytes(std::span(*bank.owner)).subspan(offset, size)};
}
float finite(const rom::GuestView& view, std::size_t offset) {
    const auto base = view.guest_address ? static_cast<std::uint64_t>(view.guest_address) : view.rom_offset;
    const auto identity = static_cast<std::uint32_t>(base + offset);
    float value;
    try { value = view.f32(offset); }
    catch (const std::out_of_range&) { throw Failure{{Error::extent, identity}}; }
    require(std::isfinite(value), Error::malformed, identity);
    return value;
}
Vec3 vector3(const rom::GuestView& view, std::size_t offset) {
    return {finite(view, offset), finite(view, offset + 4), finite(view, offset + 8)};
}
Callback callback(std::uint32_t value, Callback expected, std::uint32_t descriptor) {
    Callback result{};
    require(static_cast<bool>(callback_selector(value, result)) && result == expected, Error::callback, descriptor);
    return result;
}
BankFacts decode_bank(const rom::Snapshot& snapshot, std::size_t begin, std::size_t end,
                       std::size_t expected_size) {
    const auto input = rom_view(snapshot, begin, end - begin);
    require(input.u32(4) == expected_size, Error::extent, bank_guest);
    auto writable = std::make_shared<sprite::Bank>(expected_size);
    const auto decoded = vpk0::decode(input.bytes, std::as_writable_bytes(std::span(*writable)));
    require(static_cast<bool>(decoded) && decoded.bytes_written == expected_size &&
            decoded.bytes_consumed == end - begin, Error::malformed, bank_guest);
    return {sprite::BankOwner(std::move(writable)), begin, end, decoded.bytes_consumed, bank_guest};
}
SceneSetupFacts scene_setup_wire(rom::GuestView wire, Scene scene) {
    require(scene == Scene::logo || scene == Scene::title_menu, Error::malformed, wire.guest_address);
    const bool logo = scene == Scene::logo;
    require(wire.bytes.size() == 0x8c && wire.guest_address == (logo ? 0x800e1d40u : 0x800e8188u) &&
            wire.rom_offset == (logo ? 0xaa0af0u : 0xa0f718u), Error::extent, wire.guest_address);
    SceneSetupFacts result{};
    result.wire = wire;
    const auto& w = result.wire;
    result.flags = w.u16(0);
    require(w.u16(2) == 0, Error::malformed, wire.guest_address + 2);
    result.update = callback(w.u32(4), Callback::update_all, wire.guest_address + 4);
    result.draw = callback(w.u32(8), Callback::draw_all, wire.guest_address + 8);
    result.heap_guest = w.u32(12);
    require(result.heap_guest == (logo ? 0x800e1de0u : 0x800e83a0u), Error::malformed, wire.guest_address + 12);
    result.initialized_heap_size = w.u32(16);
    require(result.initialized_heap_size == 0, Error::malformed, wire.guest_address + 16);
    result.entry_heap_size = bank_guest - result.heap_guest;
    result.setup_count = w.u32(20); result.contexts = w.u32(24);
    for (std::size_t i = 0; i < 4; ++i) result.display_list_bytes[i] = w.u32(28 + i * 4);
    result.graphics_heap_bytes = w.u32(44); result.flags30 = w.u16(48);
    require(w.u16(50) == 0, Error::malformed, wire.guest_address + 50);
    result.rdp_output_bytes = w.u32(52);
    result.pre_render = callback(w.u32(56), logo ? Callback::logo_pre_render : Callback::menu_pre_render, wire.guest_address + 56);
    result.input = callback(w.u32(60), Callback::controller, wire.guest_address + 60);
    result.threads = w.u32(64); result.thread_stack_bytes = w.u32(68); result.stacks = w.u32(72);
    result.unk4c = w.u32(76); result.processes = w.u32(80); result.objects = w.u32(84);
    result.source_object_stride = w.u32(88); result.matrices = w.u32(92);
    require(w.u32(96) == 0 && w.u32(100) == 0, Error::callback, wire.guest_address + 96);
    result.animation_objects = w.u32(104); result.materials = w.u32(108); result.model_nodes = w.u32(112);
    result.source_model_stride = w.u32(116); result.sprites = w.u32(120);
    result.source_sprite_stride = w.u32(124); result.cameras = w.u32(128);
    result.source_camera_stride = w.u32(132);
    result.initialize = callback(w.u32(136), logo ? Callback::logo_init : Callback::menu_init, wire.guest_address + 136);
    // These source initialized scalar facts are identical, independently read
    // from BOTH mapped overlays. No menu setup is inferred from the logo.
    constexpr std::array<std::uint32_t, 18> pools{
        0x20, 0x400, 0x80, 0, 0x40, 0x40, 0x5c, 0x400, 0, 0, 0x10, 8, 0x10, 0x88, 0x80, 0x58, 8, 0x90
    };
    for (std::size_t i = 0; i < pools.size(); ++i)
        require(w.u32(64 + i * 4) == pools[i], Error::malformed, wire.guest_address + static_cast<std::uint32_t>(64 + i * 4));
    require(result.flags == 0 && result.flags30 == 0 && result.setup_count == 1 && result.contexts == 2 &&
            result.display_list_bytes == std::array<std::uint32_t, 4>{0x5000, 0x2000, 0, 0} &&
            result.graphics_heap_bytes == 0xc800 && result.rdp_output_bytes == 0, Error::malformed, wire.guest_address);
    return result;
}
SceneSetupFacts scene_setup(const rom::Snapshot& snapshot, Scene scene) {
    const bool logo = scene == Scene::logo;
    return scene_setup_wire(rom_view(snapshot, logo ? 0xaa0af0 : 0xa0f718, 0x8c,
                                    logo ? 0x800e1d40 : 0x800e8188), scene);
}
ScreenFacts screen_facts(const rom::Snapshot& snapshot, Scene scene) {
    const bool logo = scene == Scene::logo;
    ScreenFacts result{};
    result.wire = rom_view(snapshot, logo ? 0xaa0a60 : 0xa0f68c, 28,
                           logo ? 0x800e1cb0 : 0x800e80fc);
    for (std::size_t i = 0; i < 3; ++i) result.framebuffer_guests[i] = result.wire.u32(i * 4);
    result.initialized_depth_guest = result.wire.u32(12);
    result.width = result.wire.u32(16); result.height = result.wire.u32(20); result.flags = result.wire.u32(24);
    result.allocated_depth_bytes = 0x25800;
    require(result.framebuffer_guests == std::array<std::uint32_t, 3>{0x803b5000, 0x803da800, 0} &&
            result.initialized_depth_guest == 0 && result.width == 320 && result.height == 240 && result.flags == 0x16a99,
            Error::malformed, result.wire.guest_address);
    return result;
}
PreRenderFacts pre_render_facts(const rom::Snapshot& snapshot, Scene scene) {
    const bool logo = scene == Scene::logo;
    PreRenderFacts result{};
    result.display_list = rom_view(snapshot, logo ? 0xaa0a98 : 0xa0f6c0, 0x58,
                                  logo ? 0x800e1ce8 : 0x800e8130);
    result.lights = rom_view(snapshot, logo ? 0xaa0a80 : 0xa0f6a8, 24,
                            logo ? 0x800e1cd0 : 0x800e8118);
    const auto& list = result.display_list;
    // Address-bearing public F3DEX2 MOVEMEM light records. This is wire ABI,
    // not sizeof/offsetof or a native pointer to an SDK Lights1 structure.
    require(list.u32(64) == 0xdc08060a && list.u32(68) == result.lights.guest_address + 8 &&
            list.u32(72) == 0xdc08090a && list.u32(76) == result.lights.guest_address &&
            list.u32(80) == 0xdf000000 && list.u32(84) == 0, Error::malformed, list.guest_address + 64);
    for (std::size_t i = 0; i < 3; ++i) {
        result.ambient[i] = result.lights.u8(i);
        result.diffuse[i] = result.lights.u8(8 + i);
        result.direction[i] = std::bit_cast<std::int8_t>(result.lights.u8(16 + i));
        require(result.lights.u8(i + 4) == result.ambient[i] &&
                result.lights.u8(i + 12) == result.diffuse[i], Error::malformed, result.lights.guest_address);
    }
    require(result.ambient == std::array<std::uint8_t, 3>{0x61, 0x61, 0x61} &&
            result.diffuse == std::array<std::uint8_t, 3>{0xcf, 0xcf, 0xcf} &&
            result.direction == std::array<std::int8_t, 3>{0x1a, 0x4d, 0x7f}, Error::malformed, result.lights.guest_address);
    return result;
}
bool overlaps(std::size_t begin, std::size_t size, RamRange range) {
    return size != 0 && range.size != 0 && begin < range.offset + range.size && range.offset < begin + size;
}
void valid_range(RamRange range) {
    require(range.offset <= rdram_size && range.size <= rdram_size - range.offset, Error::reservation);
}
} // namespace

SceneSetupResult inspect_setup(rom::GuestView wire, Scene scene) noexcept {
    try { return {{}, scene_setup_wire(wire, scene)}; }
    catch (const Failure& failure) { return {failure.status, {}}; }
    catch (...) { return {{Error::resource_failure, wire.guest_address}, {}}; }
}

Status callback_selector(std::uint32_t identity, Callback& output) noexcept {
    switch (identity) {
    case 0x8000affc: case 0x8000ada0: case 0x80004860:
    case 0x800e1b30: case 0x800e1b54: case 0x800e4610: case 0x800e4634: case 0x800e48e0:
        output = static_cast<Callback>(identity); return {};
    default: return {Error::callback, identity};
    }
}

struct Builder {
    Catalog& c;
    const BankFacts& bank() const { return c.banks_[1]; }
    rom::GuestView view(std::uint32_t address, std::size_t size, std::size_t alignment = 4) const {
        return bank_view(bank(), address, size, alignment);
    }
    void install_view(const rom::GuestView& wire) { c.install_.push_back(wire); }

    std::size_t material(std::uint32_t address) {
        for (std::size_t i = 0; i < c.materials_.size(); ++i)
            if (c.materials_[i].wire.guest_address == address) return i;
        const auto fact = std::find_if(material_facts.begin(), material_facts.end(), [address](const auto& f) {
            return f.address == address;
        });
        require(fact != material_facts.end(), Error::extent, address);
        Material result{};
        result.wire = view(address, 0x78);
        const auto& w = result.wire;
        result.format = w.u8(2); result.size_code = w.u8(3);
        result.scale = w.u16(8); result.image_origin = w.u16(10);
        result.width = w.u16(12); result.height = w.u16(14);
        result.flags = w.u16(48); result.auxiliary_format = w.u8(50); result.auxiliary_size_code = w.u8(51);
        result.block_width = w.u16(52); result.block_height = w.u16(54);
        result.auxiliary_width = w.u16(56); result.auxiliary_height = w.u16(58);
        constexpr std::array<std::size_t, 9> offsets{20, 24, 28, 32, 36, 40, 60, 64, 68};
        for (std::size_t i = 0; i < offsets.size(); ++i) result.parameters[i] = finite(w, offsets[i]);
        for (std::size_t i = 0; i < 10; ++i) result.color_and_trailing_words[i] = w.u32(80 + i * 4);
        require(result.format <= 4 && result.size_code <= 3 && result.auxiliary_format <= 4 &&
                result.auxiliary_size_code <= 3 && result.scale != 0, Error::malformed, address);
        require(w.u32(44) == 0, Error::extent, address); // All selected palettes are DL-owned, not guessed texture tables.
        if (fact->image_count != 0) {
            result.image_table = view(w.u32(4), fact->image_count * 4);
            for (std::size_t i = 0; i < fact->image_count; ++i) {
                const auto image = result.image_table.u32(i * 4);
                view(image, 1, 8);
                result.images.push_back(image);
            }
        } else require(w.u32(4) == 0, Error::malformed, address + 4);
        c.materials_.push_back(std::move(result));
        return c.materials_.size() - 1;
    }

    std::size_t animation(std::uint32_t address, AnimationKind kind, bool receives_callback,
                          std::size_t material_index = no_index) {
        if (address == 0) return no_index;
        for (std::size_t i = 0; i < c.animations_.size(); ++i)
            if (c.animations_[i].wire.guest_address == address && c.animations_[i].kind == kind) return i;
        Animation result{};
        result.kind = kind;
        result.callback_installed = receives_callback;
        // Source omMObjAddAObj initializes targetValue/targetRate to zero.
        // Track the selected LOD channel's endpoint state for a resource-domain
        // certificate; this does not evaluate animation at runtime.
        double lod_target = 0, lod_target_rate = 0;
        std::size_t offset = 0;
        for (std::size_t command_count = 0; ; ++command_count) {
            require(command_count < 65536, Error::malformed, address);
            const auto command_address = static_cast<std::uint64_t>(address) + offset;
            require(command_address <= 0xffffffffu, Error::extent, address);
            const auto head = view(static_cast<std::uint32_t>(command_address), 4);
            const auto word = head.u32(0), opcode = word >> 25, mask = (word >> 15) & 0x3ff;
            const auto count = static_cast<std::size_t>(std::popcount(mask));
            std::size_t words = 1;
            bool float_operands = false;
            switch (opcode) {
            case 0: require(word == 0, Error::malformed, head.guest_address); break;
            case 3: case 4: case 7: case 8: case 9: case 10: case 11:
                words += count; float_operands = true; break;
            case 5: case 6: words += 2 * count; float_operands = true; break;
            case 2: case 12: break;
            case 15: require(kind == AnimationKind::model, Error::malformed, head.guest_address); break;
            case 16:
                require(kind == AnimationKind::model, Error::callback, head.guest_address);
                // Source callback is bank ID in the high mask bits and script+1
                // in low bits. All four selected events are bank0/script0 or3.
                require((mask >> 8) == 0 && ((mask & 255) == 1 || (mask & 255) == 4),
                        Error::callback, head.guest_address);
                break;
            case 18: case 19: case 20: case 21: case 22:
                require(kind == AnimationKind::texture && (mask & ~31u) == 0, Error::malformed, head.guest_address);
                words += count; break; // Packed RGBA/word operands are NOT floats.
            default:
                // Exact selected streams have no branch/path/unknown opcodes.
                // These are corruption rejections, not ignored operations.
                require(false, Error::malformed, head.guest_address);
            }
            const auto wire = view(head.guest_address, words * 4);
            if (float_operands) {
                for (std::size_t j = 1; j < words; ++j) finite(wire, j * 4);
                if (kind == AnimationKind::texture && material_index != no_index && opcode != 7) {
                    const auto& m = c.materials_[material_index];
                    std::size_t j = 1;
                    for (unsigned bit = 0; bit < 10; ++bit) if (mask & (1u << bit)) {
                        const auto value = finite(wire, j * 4);
                        if (bit == 0 || bit == 5) {
                            // GAME conversion truncates towards zero (including
                            // its tiny negative initialized epsilon).
                            const auto truncated = std::trunc(value);
                            require(truncated >= 0 && truncated < static_cast<float>(m.images.size()),
                                    Error::malformed, head.guest_address);
                            require(opcode == 10 || opcode == 11, Error::malformed, head.guest_address);
                        }
                        if (bit == 8 && (m.flags & 0x10)) {
                            require(m.images.size() >= 2, Error::extent, m.wire.guest_address);
                            const auto limit = static_cast<double>(m.images.size() - 1);
                            const auto in_domain = [limit](double v) { return std::isfinite(v) && v >= 0 && v < limit; };
                            require(in_domain(value), Error::malformed, head.guest_address);
                            if (opcode == 5 || opcode == 6 || opcode == 8 || opcode == 9) {
                                const auto next_rate = opcode == 5 || opcode == 6 ? finite(wire, (j + 1) * 4) : 0.0f;
                                const auto duration = static_cast<double>(word & 0x7fff);
                                require(duration > 0, Error::malformed, head.guest_address);
                                // A Hermite interval is a cubic Bezier with
                                // controls P0,P0+r0*d/3,P1-r1*d/3,P1. Its convex
                                // hull certifies every LOD (and floor(LOD)+1)
                                // addresses a real image throughout the interval.
                                // The source has a nonzero ~0.06 endpoint rate:
                                // requiring zero rates would reject lawful data.
                                require(in_domain(lod_target) &&
                                        in_domain(lod_target + lod_target_rate * duration / 3) &&
                                        in_domain(static_cast<double>(value) - next_rate * duration / 3),
                                        Error::malformed, head.guest_address);
                                lod_target_rate = next_rate;
                            } else {
                                lod_target_rate = 0;
                            }
                            lod_target = value;
                        }
                        if (bit == 9) require(!m.palettes.empty(), Error::extent, head.guest_address);
                        j += (opcode == 5 || opcode == 6) ? 2 : 1;
                    }
                }
            }
            result.commands.push_back({wire, static_cast<std::uint8_t>(opcode),
                static_cast<std::uint16_t>(mask), static_cast<std::uint16_t>(word & 0x7fff)});
            offset += words * 4;
            if (opcode == 0) break;
        }
        result.wire = view(address, offset);
        c.animations_.push_back(std::move(result));
        return c.animations_.size() - 1;
    }

    void model(const ModelFact& fact) {
        Model result{};
        result.render_type = fact.type;
        result.receives_effect_callback = fact.callback;
        result.tree = view(fact.tree, (fact.nodes + 1) * 44);
        result.animation_table = view(fact.animation, fact.nodes * 4);
        if (fact.materials) result.material_table = view(fact.materials, fact.nodes * 4);
        if (fact.material_animation) result.material_animation_table = view(fact.material_animation, fact.nodes * 4);
        std::array<std::size_t, 18> ancestors;
        ancestors.fill(no_index);
        result.nodes.reserve(fact.nodes);
        for (std::size_t i = 0; i < fact.nodes; ++i) {
            Node node{};
            node.wire = view(fact.tree + static_cast<std::uint32_t>(i * 44), 44);
            node.depth_flags = node.wire.u32(0);
            const auto depth = node.depth_flags & 0xfff;
            require(depth < ancestors.size() && (node.depth_flags & 0xffff0fffu) == depth, Error::malformed, node.wire.guest_address);
            require((depth == 0) == (i == 0), Error::malformed, node.wire.guest_address);
            if (depth) { node.parent = ancestors[depth - 1]; require(node.parent != no_index, Error::malformed, node.wire.guest_address); }
            ancestors[depth] = i;
            for (std::size_t d = depth + 1; d < ancestors.size(); ++d) ancestors[d] = no_index;
            node.position = vector3(node.wire, 8); node.rotation = vector3(node.wire, 20); node.scale = vector3(node.wire, 32);
            const auto payload = node.wire.u32(4);
            if (payload) {
                if (fact.type == RenderType::b_fogged) {
                    node.payloads.push_back({{}, 0, 0, payload});
                } else if (fact.type == RenderType::i_fogged) {
                    const auto p = view(payload, 8);
                    node.payloads.push_back({p, 0, p.u32(0), p.u32(4)});
                } else {
                    const std::size_t stride = fact.type == RenderType::d_fogged ? 8 : 12;
                    for (std::size_t j = 0; ; ++j) {
                        require(j < 16, Error::malformed, payload);
                        const auto p = view(payload + static_cast<std::uint32_t>(j * stride), stride);
                        const auto id = p.u32(0);
                        if (id == 4) break;
                        require(id < 4, Error::malformed, p.guest_address);
                        node.payloads.push_back({p, id, stride == 12 ? p.u32(4) : 0,
                                                p.u32(stride == 12 ? 8 : 4)});
                    }
                }
            }
            if (fact.materials) {
                const auto list = result.material_table.u32(i * 4);
                if (list) for (std::size_t j = 0; ; ++j) {
                    require(j < 16, Error::malformed, list);
                    const auto target = view(list + static_cast<std::uint32_t>(j * 4), 4).u32(0);
                    if (!target) break;
                    node.materials.push_back(material(target));
                }
            }
            node.animation = animation(result.animation_table.u32(i * 4), AnimationKind::model, fact.callback);
            if (fact.material_animation) {
                const auto list = result.material_animation_table.u32(i * 4);
                if (list) {
                    require(!node.materials.empty(), Error::malformed, list);
                    const auto entries = view(list, node.materials.size() * 4);
                    for (std::size_t j = 0; j < node.materials.size(); ++j)
                        node.material_animations.push_back(animation(entries.u32(j * 4), AnimationKind::texture,
                                                                     false, node.materials[j]));
                }
            }
            result.nodes.push_back(std::move(node));
        }
        require(result.tree.u32(fact.nodes * 44) == 18, Error::malformed, fact.tree + static_cast<std::uint32_t>(fact.nodes * 44));
        for (std::size_t i = 4; i < 44; i += 4)
            require(result.tree.u32(fact.nodes * 44 + i) == 0, Error::malformed,
                    fact.tree + static_cast<std::uint32_t>(fact.nodes * 44 + i));
        c.models_.push_back(std::move(result));
    }

    struct ImageState {
        std::span<const std::uint32_t> sources{};
        std::uint32_t single_source = 0;
        std::uint8_t format = 0, size = 0;
        std::uint16_t width = 0;
    };
    std::vector<std::uint32_t> active_lists;
    std::size_t visited_commands = 0;
    void image_load(const ImageState& image, std::uint32_t command, std::size_t bytes, ImageKind kind) {
        require(!image.sources.empty() && bytes != 0, Error::extent, command);
        for (const auto source : image.sources) {
            const auto wire = view(source, bytes, 8);
            const auto existing = std::find_if(c.image_loads_.begin(), c.image_loads_.end(), [&](const auto& item) {
                return item.command_identity == command && item.wire.guest_address == source &&
                       item.wire.bytes.size() == bytes && item.kind == kind;
            });
            if (existing == c.image_loads_.end()) {
                c.image_loads_.push_back({wire, command, kind, image.format, image.size, image.width});
                install_view(wire);
            }
        }
    }
    void dynamic_call(std::uint32_t command, std::uint32_t target,
                      std::span<const std::size_t> material_indices, ImageState& image) {
        require((target & 0xffffffe7u) == 0x0e000000u && (target & 7u) == 0,
                Error::extent, target);
        const auto slot = (target & 0xffffffu) / 8;
        require(slot < material_indices.size(), Error::extent, target);
        const auto mi = material_indices[slot];
        const auto& m = c.materials_[mi];
        const auto old = std::find_if(c.dynamic_calls_.begin(), c.dynamic_calls_.end(), [&](const auto& d) {
            return d.command_identity == command && d.segmented_target == target && d.material_index == mi;
        });
        if (old == c.dynamic_calls_.end()) c.dynamic_calls_.push_back({command, target, mi});
        const auto flags = m.flags ? m.flags : 0xa1;
        require((flags & 4) == 0, Error::malformed, m.wire.guest_address); // All selected palettes are static GBI.
        if (flags & (0x10 | 2)) {
            require(!m.images.empty(), Error::extent, m.wire.guest_address + 4);
            image.sources = m.images;
            image.format = m.auxiliary_format;
            image.size = static_cast<std::uint8_t>(m.auxiliary_size_code == 3 ? 3 : 2);
            image.width = 1;
            if (flags & (0x10 | 1)) {
                require(m.block_width != 0 && m.block_height != 0, Error::extent, m.wire.guest_address + 52);
                const auto bits = std::size_t{4} << m.auxiliary_size_code;
                const auto bytes = (static_cast<std::size_t>(m.block_width) * m.block_height * bits + 7) / 8;
                image_load(image, command, bytes, ImageKind::texture_block);
            }
        }
        if (flags & (0x10 | 1)) {
            require(!m.images.empty(), Error::extent, m.wire.guest_address + 4);
            image.sources = m.images;
            image.format = m.format;
            image.size = static_cast<std::uint8_t>(m.size_code == 3 ? 3 : 2);
            image.width = 1;
        }
    }
    void display_list(std::uint32_t address, std::span<const std::size_t> materials,
                      ImageState& image) {
        require(active_lists.size() < 32 && std::find(active_lists.begin(), active_lists.end(), address) == active_lists.end(),
                Error::malformed, address);
        active_lists.push_back(address);
        std::size_t count = 0;
        for (;;) {
            require(++visited_commands <= 1048576, Error::malformed, address);
            const auto command64 = static_cast<std::uint64_t>(address) + count * 8;
            require(command64 <= 0xffffffffu, Error::extent, address);
            const auto command = static_cast<std::uint32_t>(command64);
            const auto w = view(command, 8, 8);
            const auto w0 = w.u32(0), w1 = w.u32(4), opcode = w0 >> 24;
            ++count;
            bool done = false;
            switch (opcode) {
            case 1: {
                const auto n = (w0 >> 12) & 255, end = (w0 & 4095) >> 1;
                require(n && end >= n && end <= 32 && (w0 & 0xfff00fffu) == (0x01000000u | end * 2), Error::malformed, command);
                const auto vertices = view(w1, n * 16, 8);
                const auto old = std::find_if(c.vertex_loads_.begin(), c.vertex_loads_.end(), [command](const auto& v) {
                    return v.command_identity == command;
                });
                if (old == c.vertex_loads_.end()) {
                    c.vertex_loads_.push_back({vertices, command, static_cast<std::uint8_t>(end - n), static_cast<std::uint8_t>(n)});
                    install_view(vertices);
                }
                break;
            }
            case 0xde:
                require(w0 == 0xde000000 || w0 == 0xde010000, Error::malformed, command);
                if ((w1 >> 24) == 0x0e) dynamic_call(command, w1, materials, image);
                else display_list(w1, materials, image);
                done = w0 == 0xde010000;
                break;
            case 0xdf: require(w0 == 0xdf000000 && w1 == 0, Error::malformed, command); done = true; break;
            case 0xfd:
                view(w1, 1, 8);
                image.single_source = w1;
                image.sources = {&image.single_source, 1};
                image.format = static_cast<std::uint8_t>((w0 >> 21) & 7);
                image.size = static_cast<std::uint8_t>((w0 >> 19) & 3);
                image.width = static_cast<std::uint16_t>((w0 & 4095) + 1);
                require(image.format <= 4 && (w0 & 0x0007f000u) == 0, Error::malformed, command);
                break;
            case 0xf0: {
                require(w0 == 0xf0000000 && image.format == 0 && image.size == 2 && image.width == 1, Error::malformed, command);
                const auto entries = ((w1 >> 14) & 1023) + 1;
                require(entries <= 256 && (w1 & 0xf0003fffu) == 0, Error::malformed, command);
                image_load(image, command, entries * 2, ImageKind::palette);
                break;
            }
            case 0xf3: {
                require((w0 & 0xffffffu) == 0 && image.width == 1, Error::malformed, command);
                const auto units = ((w1 >> 12) & 4095) + 1;
                const auto bytes = (static_cast<std::size_t>(units) * (4u << image.size) + 7) / 8;
                image_load(image, command, bytes, ImageKind::texture_block);
                break;
            }
            case 0xf4: {
                // Public RDP LOADTILE uses 10.2 coordinates in source-image
                // space; retained extent includes row stride before the last
                // selected texel, not a guessed width*height image declaration.
                const auto sl = (w0 >> 12) & 4095, tl = w0 & 4095;
                const auto sh = (w1 >> 12) & 4095, th = w1 & 4095;
                require(sh >= sl && th >= tl && (sl | tl | sh | th) % 4 == 0, Error::malformed, command);
                require(image.width != 0 && (sh >> 2) < image.width, Error::malformed, command);
                const auto texels = static_cast<std::size_t>(th >> 2) * image.width + (sh >> 2) + 1;
                image_load(image, command, (texels * (4u << image.size) + 7) / 8, ImageKind::texture_tile);
                break;
            }
            case 0xdb:
                // Selected MOVEWORDs alter light colors only. Segment/matrix
                // rebinding is address-bearing and may not pass as scalar state.
                require((w0 & 0xffff0000u) == 0xdb0a0000u &&
                        ((w0 & 0xffffu) == 0 || (w0 & 0xffffu) == 4 ||
                         (w0 & 0xffffu) == 24 || (w0 & 0xffffu) == 28), Error::malformed, command);
                break;
            case 0x02: // MODIFYVTX: attribute update, not a source address.
            case 0x03: // CULLDL: retain visible continuation as well.
            case 0x05: case 0x06: case 0xd7: case 0xd9:
            case 0xe2: case 0xe3: case 0xe6: case 0xe7: case 0xe8:
            case 0xf2: case 0xf5: case 0xf9: case 0xfa: case 0xfb: case 0xfc:
                break;
            default: require(false, Error::malformed, command);
            }
            if (done) break;
        }
        active_lists.pop_back();
        const auto old = std::find_if(c.display_lists_.begin(), c.display_lists_.end(), [address](const auto& d) {
            return d.wire.guest_address == address;
        });
        if (old == c.display_lists_.end()) {
            const auto wire = view(address, count * 8, 8);
            c.display_lists_.push_back({wire}); install_view(wire);
        } else require(old->wire.bytes.size() == count * 8, Error::malformed, address);
    }

    void merge_installation() {
        auto& spans = c.install_;
        std::sort(spans.begin(), spans.end(), [](const auto& a, const auto& b) { return a.guest_address < b.guest_address; });
        std::vector<rom::GuestView> merged;
        merged.reserve(spans.size());
        for (const auto& span : spans) {
            if (!merged.empty()) {
                auto& last = merged.back();
                const auto end = static_cast<std::uint64_t>(last.guest_address) + last.bytes.size();
                if (span.guest_address <= end) {
                    const auto next_end = static_cast<std::uint64_t>(span.guest_address) + span.bytes.size();
                    if (next_end > end) last = view(last.guest_address, static_cast<std::size_t>(next_end - last.guest_address), 1);
                    continue;
                }
            }
            merged.push_back(span);
        }
        spans = std::move(merged);
    }

    void effects();
};

void Builder::effects() {
    constexpr std::size_t script_begin = 0xadec60, script_end = 0xadedf0;
    constexpr std::size_t image_begin = 0xadedf0, image_end = 0xae0510;
    try {
        c.effects_ = decode_effect_bank(
            rom_view(*c.snapshot_, script_begin, script_end - script_begin),
            rom_view(*c.snapshot_, image_begin, image_end - image_begin), 0);
    } catch (const EffectResourceError& error) {
        throw Failure{error.status()};
    }
    // Title identity certification belongs to this caller, not the reusable
    // relative decoder. Scene capacities/dither are not resource properties.
    const auto& out = c.effects_;
    require(out.scripts.size() == 4 && out.images.size() == 2, Error::malformed, script_begin);
    constexpr std::array<std::size_t, 4> script_offsets{0x14, 0x74, 0xd4, 0x134};
    for (std::size_t i = 0; i < out.scripts.size(); ++i) {
        const auto& script = out.scripts[i];
        require(script.header.rom_offset == script_begin + script_offsets[i] &&
                script.kind == (i == 3 ? 3 : 0) && script.texture == (i == 3 ? 1 : 0),
                Error::malformed, static_cast<std::uint32_t>(script.header.rom_offset));
    }
    constexpr std::array<std::size_t, 2> image_offsets{0x10, 0xc40};
    for (std::size_t i = 0; i < out.images.size(); ++i) {
        const auto& image = out.images[i];
        require(image.header.rom_offset == image_begin + image_offsets[i] &&
                image.frames.size() == (i == 0 ? 6 : 5) && image.format == (i == 0 ? 4 : 2) &&
                image.size_code == 0 && image.width == 32 && image.height == 32 &&
                image.flags == (i == 0 ? 1 : 0), Error::malformed,
                static_cast<std::uint32_t>(image.header.rom_offset));
    }
}

Catalog::Catalog(const rom::Snapshot& snapshot) : snapshot_(&snapshot), source_data_(snapshot.canonical().data()) {
    require(snapshot.canonical().size() == rom::us_size && snapshot.identity().size == rom::us_size &&
            snapshot.identity().sha1 == admitted_sha1, Error::invalid_snapshot);
    constexpr std::size_t report_table_rom = 0x46270 + (0x800ae4e4 - 0x8009a8c0);
    static_assert(report_table_rom + 151 <= 0x59f30);
    report_index_table_ = rom_view(snapshot, report_table_rom, 151, 0x800ae4e4);
    setups_[0] = scene_setup(snapshot, Scene::logo); setups_[1] = scene_setup(snapshot, Scene::title_menu);
    screens_[0] = screen_facts(snapshot, Scene::logo); screens_[1] = screen_facts(snapshot, Scene::title_menu);
    pre_render_[0] = pre_render_facts(snapshot, Scene::logo);
    pre_render_[1] = pre_render_facts(snapshot, Scene::title_menu);
    menu_initialized_.wire = rom_view(snapshot, 0xa0f660, 0x2c, 0x800e80d0);
    const auto& scalar = menu_initialized_.wire;
    menu_initialized_.initial_variant = std::bit_cast<std::int8_t>(scalar.u8(0));
    require(menu_initialized_.initial_variant == 0, Error::malformed, scalar.guest_address);
    constexpr std::array<std::int32_t, 4> pan{0, 42, 84, 127};
    constexpr std::array<std::array<std::int16_t, 4>, 3> y_positions{{
        {138, 162, 0, 0}, {132, 150, 168, 0}, {120, 138, 155, 174}
    }};
    for (std::size_t i = 0; i < 4; ++i) {
        menu_initialized_.pan[i] = std::bit_cast<std::int32_t>(scalar.u32(4 + i * 4));
        require(menu_initialized_.pan[i] == pan[i], Error::malformed, scalar.guest_address + static_cast<std::uint32_t>(4 + i * 4));
    }
    for (std::size_t row = 0; row < 3; ++row) for (std::size_t column = 0; column < 4; ++column) {
        menu_initialized_.y_positions[row][column] =
            std::bit_cast<std::int16_t>(scalar.u16(20 + (row * 4 + column) * 2));
        require(menu_initialized_.y_positions[row][column] == y_positions[row][column], Error::malformed,
                scalar.guest_address + static_cast<std::uint32_t>(20 + (row * 4 + column) * 2));
    }
    banks_[0] = decode_bank(snapshot, 0xaa0b80, 0xaa18d3, 0x7110);
    banks_[1] = decode_bank(snapshot, 0xa0f830, 0xa5cc46, 0xf3770);
    const auto add_sprite = [&](std::size_t scene_index, std::uint32_t guest) {
        auto result = sprite::hydrate(banks_[scene_index].owner, bank_guest, guest);
        require(static_cast<bool>(result), Error::malformed, result.address);
        sprites_[scene_index].push_back({guest, std::move(result.asset)});
    };
    add_sprite(0, 0x802bc0c0);
    sprites_[1].reserve(sprite_roots.size());
    for (const auto root : sprite_roots) add_sprite(1, root);
    Builder builder{*this, {}, 0};
    for (const auto& fact : model_facts) builder.model(fact);
    camera_animation_ = builder.animation(0x803a80d0, AnimationKind::camera, false);
    builder.effects();
    for (const auto& model : models_) for (const auto& node : model.nodes) {
        Builder::ImageState image{};
        for (const auto& payload : node.payloads) {
            if (payload.pre_list) builder.display_list(payload.pre_list, node.materials, image);
            if (payload.draw_list) builder.display_list(payload.draw_list, node.materials, image);
        }
    }
    require(models_.size() == 5 && materials_.size() == 10 && animations_.size() == 116 &&
            display_lists_.size() == 226 && effects_.scripts.size() == 4 && effects_.images.size() == 2,
            Error::malformed, bank_guest);
    builder.merge_installation();
    logo_install_ = {pre_render_[0].lights, pre_render_[0].display_list};
    install_.insert(install_.begin(), {pre_render_[1].lights, pre_render_[1].display_list});
}

HydrateResult hydrate(const rom::Snapshot& snapshot) noexcept {
    try { return {{}, std::unique_ptr<Catalog>(new Catalog(snapshot))}; }
    catch (const Failure& failure) { return {failure.status, {}}; }
    catch (...) { return {{Error::resource_failure, 0}, {}}; }
}
void Catalog::require_live() const {
    require(snapshot_->canonical().size() == rom::us_size && snapshot_->canonical().data() == source_data_,
            Error::invalid_snapshot);
}
const SceneSetupFacts& Catalog::setup(Scene scene) const { require_live(); return setups_[index(scene)]; }
const ScreenFacts& Catalog::screen(Scene scene) const { require_live(); return screens_[index(scene)]; }
const PreRenderFacts& Catalog::pre_render(Scene scene) const { require_live(); return pre_render_[index(scene)]; }
const MenuInitializedFacts& Catalog::menu_initialized() const { require_live(); return menu_initialized_; }
std::int32_t Catalog::report_photo_index(std::int32_t sourcePokemonID) const {
    require_live();
    if (sourcePokemonID == 603) sourcePokemonID = 90;
    if (sourcePokemonID > 0 && sourcePokemonID <= 151)
        return std::bit_cast<std::int8_t>(report_index_table_.u8(static_cast<std::size_t>(sourcePokemonID - 1)));
    std::int32_t offset;
    switch (sourcePokemonID) {
    case 1004: offset = 1; break;
    case 1010: offset = 2; break;
    case 1028: offset = 3; break;
    case 1022: offset = 4; break;
    case 1018: offset = 5; break;
    case 1035: offset = 6; break;
    default: return -1;
    }
    return std::bit_cast<std::int8_t>(report_index_table_.u8(150)) + offset;
}
const BankFacts& Catalog::bank(Scene scene) const { require_live(); return banks_[index(scene)]; }
std::span<const scene::SpriteResource> Catalog::sprites(Scene scene) const { require_live(); return sprites_[index(scene)]; }
const sprite::Sprite& Catalog::sprite(Scene scene, std::uint32_t identity) const {
    require_live();
    for (const auto& asset : sprites_[index(scene)]) if (asset.identity == identity) return asset.sprite;
    throw std::out_of_range("Sprite identity is not in selected initialized menu closure");
}
std::span<const Model> Catalog::models() const { require_live(); return models_; }
std::span<const Material> Catalog::materials() const { require_live(); return materials_; }
std::span<const Animation> Catalog::animations() const { require_live(); return animations_; }
std::size_t Catalog::camera_animation() const { require_live(); return camera_animation_; }
const EffectBank& Catalog::effects() const { require_live(); return effects_; }
std::span<const DisplayList> Catalog::display_lists() const { require_live(); return display_lists_; }
std::span<const VertexLoad> Catalog::vertex_loads() const { require_live(); return vertex_loads_; }
std::span<const ImageLoad> Catalog::image_loads() const { require_live(); return image_loads_; }
std::span<const DynamicMaterialCall> Catalog::dynamic_material_calls() const { require_live(); return dynamic_calls_; }

InstallationPlan Catalog::plan_install(Scene scene, std::span<const RamRange> runtime_ranges) const noexcept {
    InstallationPlan result{};
    try {
        require_live();
        for (const auto& range : runtime_ranges) valid_range(range);
        result.spans = index(scene) == 0 ? logo_install_ : install_;
        for (const auto& wire : result.spans) {
            require(wire.guest_address >= 0x80000000u, Error::extent, wire.guest_address);
            const auto offset = static_cast<std::size_t>(wire.guest_address - 0x80000000u);
            valid_range({offset, wire.bytes.size()});
            result.bytes += wire.bytes.size();
            for (const auto& range : framebuffers)
                if (overlaps(offset, wire.bytes.size(), range)) result.collisions.push_back({wire, range});
            for (const auto& range : runtime_ranges)
                if (overlaps(offset, wire.bytes.size(), range)) result.collisions.push_back({wire, range});
        }
        if (!result.collisions.empty()) result.status = {Error::relocation_required, result.collisions.front().source.guest_address};
    } catch (const Failure& failure) { result.status = failure.status; }
    catch (...) { result.status = {Error::resource_failure, 0}; }
    return result;
}
Status Catalog::install(Scene scene, std::span<std::byte> ram, std::span<const RamRange> runtime_ranges) const noexcept {
    try {
        require_live();
        const auto& spans = index(scene) == 0 ? logo_install_ : install_;
        require(ram.size() == rdram_size, Error::extent);
        for (const auto& range : runtime_ranges) valid_range(range);
        // All preflight checks precede the first byte write. Unlike planning,
        // installation performs no allocation (also at scene re-entry).
        const auto begin = reinterpret_cast<std::uintptr_t>(ram.data());
        const auto end = begin + ram.size();
        const auto source = reinterpret_cast<std::uintptr_t>(source_data_);
        require(end >= begin && !(begin < source + rom::us_size && source < end), Error::aliasing);
        for (const auto& bank : banks_) {
            const auto data = reinterpret_cast<std::uintptr_t>(bank.owner->data());
            require(!(begin < data + bank.owner->size() && data < end), Error::aliasing);
        }
        for (const auto& wire : spans) {
            require(wire.guest_address >= 0x80000000u, Error::extent, wire.guest_address);
            const auto offset = static_cast<std::size_t>(wire.guest_address - 0x80000000u);
            valid_range({offset, wire.bytes.size()});
            for (const auto& range : framebuffers)
                require(!overlaps(offset, wire.bytes.size(), range), Error::relocation_required, wire.guest_address);
            for (const auto& range : runtime_ranges)
                require(!overlaps(offset, wire.bytes.size(), range), Error::relocation_required, wire.guest_address);
        }
        for (const auto& wire : spans) {
            const auto offset = static_cast<std::size_t>(wire.guest_address - 0x80000000u);
            for (std::size_t i = 0; i < wire.bytes.size(); ++i) ram[(offset + i) ^ 3u] = wire.bytes[i];
        }
        return {};
    } catch (const Failure& failure) { return failure.status; }
    catch (...) { return {Error::resource_failure, 0}; }
}
} // namespace isle::menu
