#include "world_resources.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <functional>
#include <stdexcept>
#include <string>
#include <utility>

namespace isle::world {
namespace {
constexpr std::uint32_t ram_base = 0x80000000;
constexpr std::array<std::uint8_t, 20> admitted_sha1{
    0xed,0xc7,0xc4,0x9c,0xc5,0x68,0xc0,0x45,0xfe,0x48,
    0xbe,0x0d,0x18,0x01,0x1c,0x30,0xf3,0x93,0xcb,0xaf
};
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
rom::GuestView view(const rom::Snapshot& snapshot, std::uint32_t address,
                    std::size_t size, std::size_t alignment = 4) {
    require(size && address % alignment == 0, "Misaligned/empty world resource");
    std::size_t begin = 0, end = 0;
    std::uint32_t base = 0;
    if (address >= 0x8011b050 && address < 0x801b02f0) {
        begin = 0xa74e0; end = 0x13c780; base = 0x8011b050;
    } else if (address >= 0x800edd40 && address < 0x800ee104) {
        // Exact GAME common default collider/hitbox records, not world code.
        begin = 0x6b4f0; end = 0x6b8b4; base = 0x800edd40;
    } else if (address >= 0x802ce1b0 && address < 0x80318f08) {
        // Beach's initialized height patch/tree/root records only.
        begin = 0x566220; end = 0x5b0f78; base = 0x802ce1b0;
    } else throw std::runtime_error("World resource outside admitted GAME data mappings");
    const auto delta = static_cast<std::size_t>(address - base);
    require(delta <= end - begin && size <= end - begin - delta,
            "World resource extent escapes admitted mapping");
    const auto bytes = snapshot.canonical();
    require(begin + delta <= bytes.size() && size <= bytes.size() - begin - delta,
            "World resource extent escapes Snapshot");
    return {address, begin + delta, bytes.subspan(begin + delta, size)};
}
float finite(const rom::GuestView& wire, std::size_t offset) {
    const auto result = wire.f32(offset);
    require(std::isfinite(result), "Nonfinite GAME world scalar");
    return result;
}
menu::Vec3 vector3(const rom::GuestView& wire, std::size_t offset) {
    return {finite(wire, offset), finite(wire, offset + 4), finite(wire, offset + 8)};
}
Vec3 native(menu::Vec3 v) { return {v.x, v.y, v.z}; }
std::int32_t signed32(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); }
void valid_reservation(beach::RamReservation r) {
    if (r.offset > beach::rdram_size || r.size > beach::rdram_size - r.offset)
        throw std::invalid_argument("World reservation escapes 8 MiB RDRAM");
}
void disjoint(const rom::GuestView& wire, beach::RamReservation r) {
    const auto offset = static_cast<std::size_t>(wire.guest_address - ram_base);
    if (r.size && offset < r.offset + r.size && r.offset < offset + wire.bytes.size())
        throw std::invalid_argument("World resource overlaps runtime reservation");
}
} // namespace

struct ResourceBuilder {
    Resources& out;
    const rom::Snapshot& snapshot;
    rom::GuestView at(std::uint32_t address, std::size_t size, std::size_t alignment = 4) const {
        return view(snapshot, address, size, alignment);
    }
    std::size_t path(std::uint32_t address) {
        if (!address) return no_index;
        for (std::size_t i = 0; i < out.paths_.size(); ++i)
            if (out.paths_[i].wire.guest_address == address) return i;
        menu::AnimationPath result{};
        result.wire = at(address, 24);
        const auto& w = result.wire;
        result.kind = w.u8(0);
        const auto count = std::bit_cast<std::int16_t>(w.u16(2));
        require(result.kind == 2 && count >= 2 && count < 4096,
                "Beach path is not the admitted GAME Bezier interpolation kind");
        result.segment_point_count = static_cast<std::size_t>(count);
        result.tension = finite(w, 4); result.duration = finite(w, 12);
        require(result.duration > 0, "Invalid GAME path duration");
        // GAME kind2 consumes four adjacent points per segment; numPoints-1
        // segments therefore reach point numPoints+1, including endpoint guards.
        const auto points = at(w.u32(8), (result.segment_point_count + 2) * 12);
        for (std::size_t i = 0; i < result.segment_point_count + 2; ++i)
            result.points.push_back(vector3(points, i * 12));
        const auto times = at(w.u32(16), result.segment_point_count * 4);
        for (std::size_t i = 0; i < result.segment_point_count; ++i) {
            const auto time = finite(times, i * 4);
            require(time >= 0 && time <= 1 && (i == 0 || time >= result.timestamps.back()),
                    "Invalid GAME path parameter points");
            result.timestamps.push_back(time);
        }
        require(result.timestamps.front() == 0 && result.timestamps.back() == 1,
                "GAME path parameters lack exact endpoints");
        const auto speeds = at(w.u32(20), (result.segment_point_count - 1) * 20);
        for (std::size_t i = 0; i < (result.segment_point_count - 1) * 5; ++i)
            result.speed_polynomials.push_back(finite(speeds, i * 4));
        out.paths_.push_back(std::move(result));
        return out.paths_.size() - 1;
    }
    std::size_t animation(std::uint32_t address, menu::AnimationKind kind, bool callback) {
        if (!address) return no_index;
        for (std::size_t i = 0; i < out.animations_.size(); ++i)
            if (out.animations_[i].wire.guest_address == address && out.animations_[i].kind == kind) {
                require(out.animations_[i].callback_installed == callback, "Inconsistent animation callback binding");
                return i;
            }
        menu::Animation result{};
        result.kind = kind; result.callback_installed = callback;
        std::size_t offset = 0;
        for (std::size_t i = 0; ; ++i) {
            require(i < 65536, "Unterminated world animation stream");
            const auto head = at(address + static_cast<std::uint32_t>(offset), 4);
            const auto word = head.u32(0), opcode = word >> 25, mask = (word >> 15) & 1023;
            const auto count = static_cast<std::size_t>(std::popcount(mask));
            std::size_t words = 1;
            bool floats = false;
            switch (opcode) {
            case 0: require(word == 0, "Malformed world animation END"); break;
            case 2: case 12: break;
            case 3: case 4: case 7: case 8: case 9: case 10: case 11:
                words += count; floats = true; break;
            case 5: case 6: words += count * 2; floats = true; break;
            case 13:
                require(kind == menu::AnimationKind::model && mask == 8, "Invalid world path opcode");
                ++words; break;
            case 15: case 16:
                require(kind == menu::AnimationKind::model, "Invalid world model opcode"); break;
            case 17:
                require(kind == menu::AnimationKind::model, "Invalid world callback opcode");
                words += count; floats = true; break;
            case 18: case 19: case 20: case 21: case 22:
                require(kind == menu::AnimationKind::texture && (mask & ~31u) == 0,
                        "Invalid world material word opcode");
                words += count; break;
            default: throw std::runtime_error("Unadmitted world animation opcode");
            }
            const auto wire = at(head.guest_address, words * 4);
            if (floats) for (std::size_t j = 1; j < words; ++j) finite(wire, j * 4);
            menu::AnimationCommand command{wire, static_cast<std::uint8_t>(opcode),
                static_cast<std::uint16_t>(mask), static_cast<std::uint16_t>(word & 32767)};
            if (opcode == 13) command.path_index = path(wire.u32(4));
            result.commands.push_back(command);
            offset += words * 4;
            if (!opcode) break;
        }
        result.wire = at(address, offset);
        out.animations_.push_back(std::move(result));
        return out.animations_.size() - 1;
    }
    std::size_t material(std::uint32_t address) {
        for (std::size_t i = 0; i < out.materials_.size(); ++i)
            if (out.materials_[i].wire.guest_address == address) return i;
        // Initialized pointer array extents, NOT a null-termination assumption.
        constexpr std::array<std::uint32_t, 6> identities{
            0x80130f60,0x80131b20,0x80131ff0,0x80132a58,0x80132ad0,0x80133070};
        require(std::find(identities.begin(), identities.end(), address) != identities.end(),
                "Unadmitted Beach material identity");
        menu::Material result{};
        result.wire = at(address, 0x78);
        const auto& w = result.wire;
        result.format = w.u8(2); result.size_code = w.u8(3);
        result.scale = w.u16(8); result.image_origin = w.u16(10);
        result.width = w.u16(12); result.height = w.u16(14);
        result.flags = w.u16(48); result.auxiliary_format = w.u8(50); result.auxiliary_size_code = w.u8(51);
        result.block_width = w.u16(52); result.block_height = w.u16(54);
        result.auxiliary_width = w.u16(56); result.auxiliary_height = w.u16(58);
        constexpr std::array<std::size_t, 9> offsets{20,24,28,32,36,40,60,64,68};
        for (std::size_t i = 0; i < offsets.size(); ++i) result.parameters[i] = finite(w, offsets[i]);
        for (std::size_t i = 0; i < 10; ++i) result.color_and_trailing_words[i] = w.u32(80 + i * 4);
        require(result.format <= 4 && result.size_code <= 3 && result.auxiliary_format <= 4 &&
                result.auxiliary_size_code <= 3 && result.scale && w.u32(44) == 0,
                "Malformed Beach material fields/palette ownership");
        result.image_table = at(w.u32(4), 8);
        for (std::size_t i = 0; i < 2; ++i) {
            const auto image = result.image_table.u32(i * 4);
            at(image, 1, 8); result.images.push_back(image);
        }
        out.materials_.push_back(std::move(result));
        return out.materials_.size() - 1;
    }
    std::size_t single_model(std::uint32_t display_list, menu::RenderType type) {
        menu::Model model{}; model.render_type = type;
        model.matrix_kinds = {1,0,0}; // Native translate-only, overridden for roads/decorations by consumer.
        model.fog_rgb = out.setup_.fog_rgb; model.fog_min = out.setup_.fog_min; model.fog_max = out.setup_.fog_max;
        menu::Node node{}; node.scale = {1,1,1};
        if (display_list) node.payloads.push_back({{},0,0,display_list});
        model.nodes.push_back(std::move(node));
        out.models_.push_back(std::move(model));
        return out.models_.size() - 1;
    }
    void attachments(menu::Model& model, std::uint32_t table, std::uint32_t animation_table) {
        if (table) model.material_table = at(table, model.nodes.size() * 4);
        if (animation_table) model.material_animation_table = at(animation_table, model.nodes.size() * 4);
        for (std::size_t i = 0; i < model.nodes.size(); ++i) {
            auto& node = model.nodes[i];
            if (table) {
                const auto list = model.material_table.u32(i * 4);
                if (list) for (std::size_t j = 0; ; ++j) {
                    require(j < 16, "Unterminated Beach material attachment list");
                    const auto target = at(list + static_cast<std::uint32_t>(j * 4), 4).u32(0);
                    if (!target) break;
                    node.materials.push_back(material(target));
                }
            }
            if (animation_table) {
                const auto list = model.material_animation_table.u32(i * 4);
                if (list) {
                    require(!node.materials.empty(), "Material animation without material attachment");
                    const auto entries = at(list, node.materials.size() * 4);
                    for (std::size_t j = 0; j < node.materials.size(); ++j)
                        node.material_animations.push_back(animation(entries.u32(j * 4), menu::AnimationKind::texture, false));
                }
            }
        }
    }
    std::size_t road(std::uint32_t tree, std::uint32_t animations, std::size_t count) {
        menu::Model model{}; model.render_type = menu::RenderType::b;
        model.matrix_kinds = {0x1c,0,0}; model.animation_speed = 0;
        model.receives_effect_callback = true; // Actual source world road callback.
        model.tree = at(tree, (count + 1) * 44);
        model.animation_table = at(animations, count * 4);
        std::array<std::size_t, 18> ancestors; ancestors.fill(no_index);
        for (std::size_t i = 0; i < count; ++i) {
            menu::Node node{}; node.wire = at(tree + static_cast<std::uint32_t>(i * 44), 44);
            node.depth_flags = node.wire.u32(0);
            const auto depth = node.depth_flags & 4095;
            require(depth < 18 && (node.depth_flags & ~0xffffu) == 0 && (i || depth == 0), "Invalid road tree depth");
            if (depth) { node.parent = ancestors[depth - 1]; require(node.parent != no_index, "Orphan road node"); }
            ancestors[depth] = i;
            for (std::size_t d = depth + 1; d < ancestors.size(); ++d) ancestors[d] = no_index;
            node.position = vector3(node.wire, 8); node.rotation = vector3(node.wire, 20); node.scale = vector3(node.wire, 32);
            if (const auto payload = node.wire.u32(4)) node.payloads.push_back({{},0,0,payload});
            node.animation = animation(model.animation_table.u32(i * 4), menu::AnimationKind::model, true);
            model.nodes.push_back(std::move(node));
        }
        require(model.tree.u32(count * 44) == 18, "Road tree control-line count mismatches terminator");
        out.models_.push_back(std::move(model));
        return out.models_.size() - 1;
    }
    std::vector<StaticObject> statics(std::uint32_t address) {
        std::vector<StaticObject> result;
        if (!address) return result;
        for (std::size_t i = 0; ; ++i) {
            require(i < 4096, "Unterminated Beach static object list");
            const auto w = at(address + static_cast<std::uint32_t>(i * 40), 40);
            const auto id = signed32(w.u32(0));
            if (id == -1) break;
            result.push_back({id,native(vector3(w,4)),native(vector3(w,16)),native(vector3(w,28)),w});
        }
        return result;
    }
    void spawns(BlockResource& block) {
        const auto address = block.metadata.spawn_guest;
        if (!address) return;
        // ObjectSpawn offsets/stride verified from pinned include/common_structs.h.
        for (std::size_t i = 0; ; ++i) {
            require(i < 4096, "Unterminated Beach spawn list");
            const auto w = at(address + static_cast<std::uint32_t>(i * 48), 48);
            const auto id = signed32(w.u32(0));
            if (id == -1) break;
            block.spawns.push_back({id,signed32(w.u32(4)),native(vector3(w,8)),native(vector3(w,20)),
                                    native(vector3(w,32)),path(w.u32(44)),w});
        }
    }
    void definitions() {
        for (std::size_t i = 0; ; ++i) {
            require(i < 100, "Unterminated world static definition list");
            const auto w = at(out.setup_.static_definitions_guest + static_cast<std::uint32_t>(i * 12), 12);
            const auto id = signed32(w.u32(0));
            if (id == -1) break;
            require(w.u32(4) == 0x800e30b0 && w.u32(8), "Unadmitted world decoration callback/payload");
            const auto model = single_model(w.u32(8), menu::RenderType::b_fogged);
            out.static_definitions_.push_back({id,w.u32(4),w.u32(8),model});
        }
    }
    void decorate(BlockResource& block) {
        auto& model = out.models_[block.model];
        for (const auto& object : block.decorations) {
            const auto definition = std::find_if(out.static_definitions_.begin(), out.static_definitions_.end(),
                [&](const auto& d) { return d.id == object.id; });
            if (definition == out.static_definitions_.end()) continue; // Source ignores unmatched IDs.
            require(definition->callback_identity == 0x800e30b0, "Unadmitted decoration callback");
            menu::Node node{}; node.parent = 0; node.depth_flags = 1;
            node.position = {object.position.x * 100,object.position.y * 100,object.position.z * 100};
            node.rotation = {object.rotation.x,object.rotation.y,object.rotation.z};
            node.scale = {object.scale.x,object.scale.y,object.scale.z};
            node.payloads.push_back({{},0,0,definition->payload_identity});
            model.nodes.push_back(std::move(node));
            model.render_type = menu::RenderType::b_fogged;
        }
    }
    void collision() {
        for (std::size_t i = 0; ; ++i) {
            require(i < 100, "Unterminated world collision model table");
            const auto w = at(out.setup_.collision_models_guest + static_cast<std::uint32_t>(i * 12), 12);
            const auto id = signed32(w.u32(0));
            if (id == -1) break;
            CollisionModel model{id,finite(w,8),{}};
            const auto address = w.u32(4);
            for (std::size_t j = 0; ; ++j) {
                require(j < 4096, "Unterminated world collider tree");
                const auto c = at(address + static_cast<std::uint32_t>(j * 44), 44);
                const auto depth = signed32(c.u32(0));
                if (depth == 18) break;
                require(depth >= 0 && depth < 18, "Invalid GAME collider depth");
                Collider collider{}; collider.depth = depth;
                collider.position = native(vector3(c,8)); collider.rotation = native(vector3(c,20)); collider.scale = native(vector3(c,32));
                if (const auto hitbox = c.u32(4)) {
                    const auto type = at(hitbox,1,4).u8(0);
                    require(type >= 1 && type <= 4, "Unknown GAME hitbox type");
                    const std::size_t parameters = type == 1 ? 1 : type == 4 ? 3 : 2;
                    const auto h = at(hitbox,132 + parameters * 4);
                    collider.has_hitbox = true; collider.hitbox.type = type;
                    collider.hitbox.parameters.x = finite(h,132);
                    if (parameters >= 2) collider.hitbox.parameters.y = finite(h,136);
                    if (parameters >= 3) collider.hitbox.parameters.z = finite(h,140);
                }
                model.colliders.push_back(collider);
            }
            out.collision_models_.push_back(std::move(model));
        }
    }
    void height() {
        const auto root = at(0x80318f00,8);
        // Exact initialized GAME array extents; all indexed nodes/patches are
        // retained in source order, so native indices need no relocation table.
        constexpr std::size_t patch_count = 6386, node_count = 6385;
        const auto patches = at(root.u32(0),patch_count * 20);
        const auto nodes = at(root.u32(4),node_count * 28);
        auto index = [](std::uint32_t value, std::size_t count) {
            if (value == 0xffffffffu) return no_index;
            require(value < count, "Height tree index escapes exact GAME array");
            return static_cast<std::size_t>(value);
        };
        out.height_map_.patches.reserve(patch_count); out.height_map_.nodes.reserve(node_count);
        for (std::size_t i = 0; i < patch_count; ++i) {
            const auto o = i * 20;
            out.height_map_.patches.push_back({finite(patches,o),finite(patches,o+4),finite(patches,o+8),finite(patches,o+12),patches.u32(o+16)});
        }
        for (std::size_t i = 0; i < node_count; ++i) {
            const auto o = i * 28;
            out.height_map_.nodes.push_back({finite(nodes,o),finite(nodes,o+4),finite(nodes,o+8),
                index(nodes.u32(o+12),node_count),index(nodes.u32(o+16),node_count),
                index(nodes.u32(o+20),patch_count),index(nodes.u32(o+24),patch_count)});
        }
        std::vector<std::uint8_t> state(node_count), seen_patches(patch_count);
        const auto visit = [&](auto&& self, std::size_t i) -> void {
            if (i == no_index) return;
            require(state[i] == 0, "Cyclic or multiply-parented GAME height tree");
            state[i] = 1;
            const auto& n = out.height_map_.nodes[i];
            if (n.left_patch != no_index) seen_patches[n.left_patch] = 1;
            if (n.right_patch != no_index) seen_patches[n.right_patch] = 1;
            self(self,n.left_child); self(self,n.right_child);
        };
        visit(visit,0);
        require(std::all_of(state.begin(),state.end(),[](auto v){return v == 1;}) &&
                std::all_of(seen_patches.begin(),seen_patches.end(),[](auto v){return v == 1;}),
                "Height resource extent contains unreachable records");
    }
    struct ImageState {
        std::span<const std::uint32_t> sources;
        std::uint32_t single_source = 0;
        std::uint8_t format = 0, size = 0;
        std::uint16_t width = 0;
    };
    std::vector<std::uint32_t> active_lists;
    std::size_t command_visits = 0;
    void image_load(const ImageState& state, std::uint32_t command, std::size_t bytes, menu::ImageKind kind) {
        require(!state.sources.empty() && bytes, "Image load without source");
        for (const auto address : state.sources) {
            const auto wire = at(address,bytes,8);
            const auto old = std::find_if(out.images_.begin(),out.images_.end(),[&](const auto& image) {
                return image.command_identity == command && image.wire.guest_address == address &&
                       image.wire.bytes.size() == bytes && image.kind == kind;
            });
            if (old == out.images_.end()) {
                out.images_.push_back({wire,command,kind,state.format,state.size,state.width});
                out.installation_.push_back(wire);
            }
        }
    }
    void dynamic_call(std::uint32_t command, std::uint32_t target,
                      std::span<const std::size_t> materials, ImageState& image) {
        require((target & 0xffffffe7u) == 0x0e000000u && !(target & 7u), "Invalid dynamic material slot");
        const auto slot = (target & 0xffffffu) / 8;
        require(slot < materials.size(), "Dynamic material call lacks native attachment");
        const auto index = materials[slot];
        const auto& material = out.materials_[index];
        const auto old = std::find_if(out.dynamic_calls_.begin(),out.dynamic_calls_.end(),[&](const auto& c) {
            return c.command_identity == command && c.segmented_target == target && c.material_index == index;
        });
        if (old == out.dynamic_calls_.end()) out.dynamic_calls_.push_back({command,target,index});
        const auto flags = material.flags ? material.flags : 0xa1;
        require(!(flags & 4), "Beach material unexpectedly owns dynamic palette");
        if (flags & (0x10 | 2)) {
            image.sources = material.images; image.format = material.auxiliary_format;
            image.size = static_cast<std::uint8_t>(material.auxiliary_size_code == 3 ? 3 : 2); image.width = 1;
            if (flags & (0x10 | 1)) {
                const auto bytes = (static_cast<std::size_t>(material.block_width) * material.block_height *
                                    (4u << material.auxiliary_size_code) + 7) / 8;
                image_load(image,command,bytes,menu::ImageKind::texture_block);
            }
        }
        if (flags & (0x10 | 1)) {
            image.sources = material.images; image.format = material.format;
            image.size = static_cast<std::uint8_t>(material.size_code == 3 ? 3 : 2); image.width = 1;
        }
    }
    void display_list(std::uint32_t address, std::span<const std::size_t> materials, ImageState& image) {
        require(active_lists.size() < 32 && std::find(active_lists.begin(),active_lists.end(),address) == active_lists.end(),
                "Cyclic/overdeep world GBI graph");
        active_lists.push_back(address);
        std::size_t count = 0;
        for (;;) {
            require(++command_visits <= 1048576, "World GBI closure exceeds finite traversal bound");
            const auto command64 = static_cast<std::uint64_t>(address) + count * 8;
            require(command64 <= 0xffffffffu, "World GBI command overflow");
            const auto command = static_cast<std::uint32_t>(command64);
            const auto w = at(command,8,8);
            const auto w0 = w.u32(0), w1 = w.u32(4);
            ++count; bool done = false;
            switch (w0 >> 24) {
            case 1: {
                const auto n = (w0 >> 12) & 255, end = (w0 & 4095) >> 1;
                require(n && end >= n && end <= 32 && (w0 & 0xfff00fffu) == (0x01000000u | end * 2), "Invalid world vertex cache range");
                const auto wire = at(w1,n*16,8);
                const auto old = std::find_if(out.vertices_.begin(),out.vertices_.end(),[&](const auto& v){return v.command_identity == command;});
                if (old == out.vertices_.end()) {
                    out.vertices_.push_back({wire,command,static_cast<std::uint8_t>(end-n),static_cast<std::uint8_t>(n)});
                    out.installation_.push_back(wire);
                }
                break;
            }
            case 0xde:
                require(w0 == 0xde000000 || w0 == 0xde010000, "Malformed world G_DL");
                if ((w1 >> 24) == 0x0e) dynamic_call(command,w1,materials,image);
                else display_list(w1,materials,image);
                done = w0 == 0xde010000; break;
            case 0xdf: require(w0 == 0xdf000000 && !w1,"Malformed world ENDDL"); done = true; break;
            case 0xfd:
                at(w1,1,8); image.single_source = w1; image.sources = {&image.single_source,1};
                image.format = static_cast<std::uint8_t>((w0 >> 21) & 7);
                image.size = static_cast<std::uint8_t>((w0 >> 19) & 3); image.width = static_cast<std::uint16_t>((w0 & 4095) + 1);
                require(image.format <= 4 && !(w0 & 0x0007f000u),"Malformed world SETTIMG"); break;
            case 0xf0: {
                require(w0 == 0xf0000000 && image.format == 0 && image.size == 2 && image.width == 1,"Invalid world TLUT source");
                const auto entries = ((w1 >> 14) & 1023) + 1;
                require(entries <= 256 && !(w1 & 0xf0003fffu),"Malformed world TLUT count");
                image_load(image,command,entries*2,menu::ImageKind::palette); break;
            }
            case 0xf3: {
                require(!(w0 & 0xffffffu) && image.width == 1,"Unsupported world LOADBLOCK origin");
                const auto units = ((w1 >> 12) & 4095) + 1;
                image_load(image,command,(static_cast<std::size_t>(units)*(4u<<image.size)+7)/8,menu::ImageKind::texture_block); break;
            }
            case 0xf4: {
                const auto sl=(w0>>12)&4095,tl=w0&4095,sh=(w1>>12)&4095,th=w1&4095;
                require(sh>=sl && th>=tl && (sl|tl|sh|th)%4==0 && image.width && (sh>>2)<image.width,"Invalid world LOADTILE extent");
                const auto texels=static_cast<std::size_t>(th>>2)*image.width+(sh>>2)+1;
                image_load(image,command,(texels*(4u<<image.size)+7)/8,menu::ImageKind::texture_tile); break;
            }
            case 0xdb:
                require((w0&0xffff0000u)==0xdb0a0000u && ((w0&65535)==0 || (w0&65535)==4 ||
                        (w0&65535)==24 || (w0&65535)==28),"Unhandled world address-bearing MOVEWORD"); break;
            case 0x02: case 0x03: case 0x05: case 0x06: case 0xd7: case 0xd9:
            case 0xe2: case 0xe3: case 0xe6: case 0xe7: case 0xe8:
            case 0xf2: case 0xf5: case 0xf9: case 0xfa: case 0xfb: case 0xfc: break;
            default: throw std::runtime_error("Unhandled world GBI resource opcode at " + std::to_string(command));
            }
            if (done) break;
        }
        active_lists.pop_back();
        const auto old = std::find_if(out.lists_.begin(),out.lists_.end(),[&](const auto& l){return l.wire.guest_address==address;});
        if (old == out.lists_.end()) {
            const auto wire = at(address,count*8,8); out.lists_.push_back({wire}); out.installation_.push_back(wire);
        } else require(old->wire.bytes.size()==count*8,"State-dependent world list extent");
    }
    void closure() {
        for (const auto& model : out.models_) for (const auto& node : model.nodes) for (const auto& payload : node.payloads) {
            ImageState image{};
            if (payload.pre_list) display_list(payload.pre_list,node.materials,image);
            if (payload.draw_list) display_list(payload.draw_list,node.materials,image);
        }
        ImageState image{}; display_list(out.sky_.display_list_guest,{},image);
        auto& spans = out.installation_;
        std::sort(spans.begin(),spans.end(),[](const auto& a,const auto& b){return a.guest_address<b.guest_address;});
        std::vector<rom::GuestView> merged; merged.reserve(spans.size());
        for (const auto& span : spans) {
            if (!merged.empty()) {
                auto& last=merged.back(); const auto end=static_cast<std::uint64_t>(last.guest_address)+last.bytes.size();
                if (span.guest_address<=end) {
                    const auto next=static_cast<std::uint64_t>(span.guest_address)+span.bytes.size();
                    if (next>end) last=at(last.guest_address,static_cast<std::size_t>(next-last.guest_address),1);
                    continue;
                }
            }
            merged.push_back(span);
        }
        spans=std::move(merged);
    }
};

Resources::Resources(const rom::Snapshot& snapshot)
    : snapshot_(&snapshot), source_data_(snapshot.canonical().data()) {
    require(snapshot.canonical().size()==rom::us_size && snapshot.identity().size==rom::us_size &&
            snapshot.identity().sha1==admitted_sha1,"World resources require admitted US Snapshot");
    // Share only metadata decoding with the independent first-block diagnostic;
    // no discarded diagnostic GBI closure or redundant asset traversal.
    auto descriptors = beach::world_descriptors(snapshot);
    setup_=descriptors.world; sky_=descriptors.sky; primary_count_=descriptors.primary.size();
    require(primary_count_==8 && descriptors.secondary.size()==4,"Wrong Beach descriptor groups");
    ResourceBuilder builder{*this,snapshot,{},0};
    builder.definitions();
    blocks_.reserve(12);
    for (const auto group : {std::span<const beach::BlockMetadata>(descriptors.primary),
                             std::span<const beach::BlockMetadata>(descriptors.secondary)}) for (const auto& metadata : group) {
        BlockResource block{}; block.metadata=metadata;
        require(metadata.render_callback_guest==0x800a1530,"Unadmitted Beach block render callback");
        block.model=builder.single_model(metadata.display_list_guest,menu::RenderType::a_fogged);
        builder.attachments(models_[block.model],metadata.materials_guest,metadata.texture_animation_guest);
        require(metadata.control_line_count==0 || metadata.control_line_count==2,"Wrong Beach road control count");
        if (metadata.control_line_count) {
            const auto count=static_cast<std::size_t>(metadata.control_line_count);
            const auto times=builder.at(metadata.gfx_wire.guest_address+32,(count-1)*4);
            for (std::size_t i=0;i<count-1;++i) block.control_timestamps.push_back(finite(times,i*4));
            block.road_model=builder.road(metadata.road_guest,metadata.movement_animation_guest,count);
        } else require(!metadata.road_guest && !metadata.movement_animation_guest,"Road resources without control lines");
        block.decorations=builder.statics(metadata.statics_guest);
        block.collision_objects=builder.statics(metadata.static_objects_guest);
        builder.spawns(block); builder.decorate(block);
        blocks_.push_back(std::move(block));
    }
    builder.collision(); builder.height(); builder.closure();
}
void Resources::require_live() const {
    const auto bytes=snapshot_->canonical();
    require(bytes.size()==rom::us_size && bytes.data()==source_data_,"World resources borrow moved-from Snapshot");
}
void Resources::install(std::span<std::byte> ram, std::span<const beach::RamReservation> reservations) const {
    require_live();
    if (ram.size()!=beach::rdram_size) throw std::invalid_argument("World install requires word-swapped 8 MiB RDRAM");
    const auto source=snapshot_->canonical();
    const std::less<const std::byte*> less;
    if (less(ram.data(),source.data()+source.size()) && less(source.data(),ram.data()+ram.size()))
        throw std::invalid_argument("World installation aliases Snapshot");
    for (const auto r:reservations) valid_reservation(r);
    // Validate the entire union before the first byte is touched.
    for (const auto& span:installation_) {
        const auto offset=static_cast<std::size_t>(span.guest_address-ram_base);
        require(span.guest_address>=ram_base && offset<=ram.size() && span.bytes.size()<=ram.size()-offset &&
                span.rom_offset<=source.size() && span.bytes.size()<=source.size()-span.rom_offset &&
                span.bytes.data()==source.data()+span.rom_offset,"Invalid world install provenance/extent");
        for (const auto r:beach::producer_reservations) disjoint(span,r);
        for (const auto r:reservations) disjoint(span,r);
    }
    for (const auto& span:installation_) {
        const auto offset=static_cast<std::size_t>(span.guest_address-ram_base);
        for (std::size_t i=0;i<span.bytes.size();++i) ram[(offset+i)^3u]=span.bytes[i];
    }
}
} // namespace isle::world
