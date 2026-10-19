#include "world_resources.hpp"
#include "model_data_decoder.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <functional>
#include <stdexcept>
#include <utility>

namespace isle::world {
namespace {
constexpr std::uint32_t ram_base = 0x80000000;
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
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
CollisionModel decode_collision(const model_assets::Reader& reader, const rom::GuestView& w) {
    CollisionModel model{signed32(w.u32(0)),finite(w,8),{}};
    model.wire = w;
    const auto address = w.u32(4);
    for (std::size_t j = 0; ; ++j) {
        require(j < 4096, "Unterminated world collider tree");
        const auto c = reader.view(address + static_cast<std::uint32_t>(j * 44), 44);
        const auto depth = signed32(c.u32(0));
        if (depth == 18) {
            model.tree_wire = reader.view(address, (j + 1) * 44);
            break;
        }
        require(depth >= 0 && depth < 18, "Invalid GAME collider depth");
        Collider collider{}; collider.depth = depth; collider.wire = c;
        collider.position = native(vector3(c,8)); collider.rotation = native(vector3(c,20));
        collider.scale = native(vector3(c,32));
        if (const auto hitbox = c.u32(4)) {
            const auto type = reader.view(hitbox,1,4).u8(0);
            require(type >= 1 && type <= 4, "Unknown GAME hitbox type");
            const std::size_t parameters = type == 1 ? 1 : type == 4 ? 3 : 2;
            const auto h = reader.view(hitbox,132 + parameters * 4);
            collider.hitbox_wire = h;
            collider.has_hitbox = true; collider.hitbox.type = type;
            collider.hitbox.parameters.x = finite(h,132);
            if (parameters >= 2) collider.hitbox.parameters.y = finite(h,136);
            if (parameters >= 3) collider.hitbox.parameters.z = finite(h,140);
        }
        model.colliders.push_back(collider);
    }
    return model;
}
} // namespace

CollisionModel tunnel_gate_collision(const rom::Snapshot& snapshot) {
    // The exact independently initialized CollisionModel in Tunnel1009/code.c;
    // not the World collision_models array and not Rainbow's gate resource.
    constexpr std::array<std::uint8_t,20> sha1{
        0xed,0xc7,0xc4,0x9c,0xc5,0x68,0xc0,0x45,0xfe,0x48,
        0xbe,0x0d,0x18,0x01,0x1c,0x30,0xf3,0x93,0xcb,0xaf};
    require(snapshot.canonical().size()==rom::us_size &&
            snapshot.identity().size==rom::us_size && snapshot.identity().sha1==sha1,
            "Tunnel gate collision requires admitted US Snapshot");
    const model_assets::Reader reader(snapshot,course::layout(1).mappings);
    auto model=decode_collision(reader,reader.view(0x802ef9e4u,12));
    require(model.id==1009 && model.scale==1 && model.wire.rom_offset==0x5ecab4u &&
            model.wire.u32(4)==0x802ef908u && model.tree_wire.guest_address==0x802ef908u,
            "Tunnel1009 collision record/tree differs from qualified Source tuple");
    model.snapshot=&snapshot;
    model.source_data=snapshot.canonical().data();
    return model;
}

RainbowGateCollision::RainbowGateCollision(const rom::Snapshot& snapshot) {
    // df8815e75f3d79618466e01e0827c6165b93d11c gate/code.c
    // blob ee5d0b94feada6dcafa0df3a8b07c36baa5b10d1: InitOneCollisionModel
    // uses this singleton, independently of the World setup collision table.
    constexpr std::array<std::uint8_t,20> sha1{
        0xed,0xc7,0xc4,0x9c,0xc5,0x68,0xc0,0x45,0xfe,0x48,
        0xbe,0x0d,0x18,0x01,0x1c,0x30,0xf3,0x93,0xcb,0xaf};
    require(snapshot.canonical().size()==rom::us_size &&
            snapshot.identity().size==rom::us_size && snapshot.identity().sha1==sha1,
            "Rainbow Gate collision requires admitted US Snapshot");
    const model_assets::Reader reader(snapshot,course::layout(course::rainbow_id).mappings);
    model_=decode_collision(reader,reader.view(0x8034af20,12));
    require(model_.id==1001 && model_.scale==1 && model_.wire.rom_offset==0x82a690 &&
            model_.wire.u32(4)==0x800edf78 && model_.tree_wire.guest_address==0x800edf78 &&
            model_.tree_wire.rom_offset==0x6b728 && model_.tree_wire.bytes.size()==9*44 &&
            model_.colliders.size()==8, "Rainbow Gate collision record/tree differs from admitted resource");
    constexpr std::array<std::uint32_t,8> hitboxes{
        0,0,0x800edd40,0,0x800eddd0,0,0x800ede60,0x800edef0};
    constexpr std::array<int,8> depths{0,1,2,1,2,1,2,1};
    for(std::size_t i=0;i<model_.colliders.size();++i) {
        const auto& c=model_.colliders[i];
        require(c.depth==depths[i] && c.wire.u32(4)==hitboxes[i],
                "Rainbow Gate collider shape identity differs");
        if(hitboxes[i]) {
            const bool root=i==7;
            require(c.has_hitbox && c.hitbox.type==(root?1:4) &&
                    c.hitbox.parameters.x==(root?1.0f:.5f) &&
                    (root || (c.hitbox.parameters.y==.5f && c.hitbox.parameters.z==.5f)),
                    "Rainbow Gate hitbox shape differs");
        }
    }
    model_.snapshot=&snapshot;
    model_.source_data=snapshot.canonical().data();
}
void RainbowGateCollision::require_live() const {
    require(model_.snapshot && model_.snapshot->canonical().size()==rom::us_size &&
            model_.snapshot->canonical().data()==model_.source_data,
            "Rainbow Gate collision borrows moved-from Snapshot");
}

struct ResourceBuilder {
    Resources& out;
    const model_assets::Reader& reader;
    model_assets::DataDecoder& decoder;
    struct MaterialUse {
        std::uint32_t address;
        std::vector<std::size_t> streams; // Unrestricted live sky playback.
        std::vector<menu::animation::TextureSample> samples; // Restarted terrain only.
    };
    std::vector<MaterialUse> material_uses;
    rom::GuestView at(std::uint32_t address, std::size_t size, std::size_t alignment = 4) const {
        return reader.view(address, size, alignment);
    }
    void collect_materials(std::uint32_t table, std::uint32_t animation_table, std::size_t nodes,
                           bool restarted_terrain) {
        if (!table) return;
        const auto materials = at(table, nodes * 4);
        const auto animations = animation_table ? at(animation_table, nodes * 4) : rom::GuestView{};
        for (std::size_t i = 0; i < nodes; ++i) {
            const auto list = materials.u32(i * 4);
            if (!list) continue;
            const auto animation_list = animation_table ? animations.u32(i * 4) : 0;
            const auto available = reader.remaining_bytes(list) / 4;
            bool terminated = false;
            for (std::size_t j = 0; j < available; ++j) {
                const auto target = at(list + static_cast<std::uint32_t>(j * 4), 4).u32(0);
                if (!target) { terminated = true; break; }
                auto use = std::find_if(material_uses.begin(), material_uses.end(),
                    [target](const auto& value) { return value.address == target; });
                if (use == material_uses.end()) {
                    material_uses.push_back({target, {}, {}});
                    use = material_uses.end() - 1;
                }
                if (animation_list) {
                    const auto stream = decoder.animation(
                        at(animation_list + static_cast<std::uint32_t>(j * 4), 4).u32(0),
                        menu::AnimationKind::texture, false);
                    if (stream != no_index) {
                        if (restarted_terrain) {
                            // block.c:234-242,434-436: repeated subtraction100,
                            // kind-only restart, speed0, exactly one update.
                            if (std::none_of(use->samples.begin(),use->samples.end(),
                                [stream](const auto& sample) { return sample.program == stream; }))
                                use->samples.push_back({stream,0,std::nextafter(100.0f,0.0f),0});
                        } else if (std::find(use->streams.begin(),use->streams.end(),stream) == use->streams.end())
                            use->streams.push_back(stream);
                    }
                }
            }
            require(terminated, "World material attachments lack bounded NULL sentinel");
        }
    }
    void admit_materials() {
        for (const auto& use : material_uses) {
            decoder.sampled_material(use.address,use.samples,use.streams);
        }
    }
    menu::Model simple_model(std::uint32_t display_list, menu::RenderType type) {
        menu::Model model{}; model.render_type = type;
        // Native omGObjAddDObj/omDObjAddMObj initialize both clocks to1
        // (sys/om.c1089-1091,1130-1132), before any World speed setter.
        model.animation_speed = 1.0f;
        model.matrix_kinds = {1,0,0}; // Source translate-only terrain/one-cycle sky.
        model.fog_rgb = out.setup_.fog_rgb; model.fog_min = out.setup_.fog_min; model.fog_max = out.setup_.fog_max;
        menu::Node node{}; node.scale = {1,1,1};
        if (display_list) node.payloads.push_back({{},0,0,display_list});
        model.nodes.push_back(std::move(node));
        return model;
    }
    std::size_t single_model(std::uint32_t display_list, menu::RenderType type) {
        out.models_.push_back(simple_model(display_list, type));
        return out.models_.size() - 1;
    }
    void attachments(menu::Model& model, std::uint32_t table, std::uint32_t animation_table) {
        if (table) model.material_table = at(table, model.nodes.size() * 4);
        for (std::size_t i = 0; i < model.nodes.size(); ++i) if (table) {
            const auto list = model.material_table.u32(i * 4);
            if (!list) continue;
            const auto available = reader.remaining_bytes(list) / 4;
            bool terminated = false;
            for (std::size_t j = 0; j < available; ++j) {
                const auto target = at(list + static_cast<std::uint32_t>(j * 4), 4).u32(0);
                if (!target) { terminated = true; break; }
                model.nodes[i].materials.push_back(decoder.material(target));
            }
            require(terminated, "World material attachments lack bounded NULL sentinel");
        }
        decoder.bind_initial_animations(model, 0, animation_table, false);
    }
    std::size_t road(std::uint32_t tree, std::uint32_t animations, std::size_t count) {
        auto model = decoder.model(tree, 0, count, menu::RenderType::b);
        model.matrix_kinds = {0x1c,0,0}; model.animation_speed = 0;
        model.receives_effect_callback = true; // world/block.c:536, func_800E19A4.
        decoder.bind_initial_animations(model, animations, 0, true);
        out.models_.push_back(std::move(model));
        return out.models_.size() - 1;
    }
    std::vector<StaticObject> statics(std::uint32_t address) {
        std::vector<StaticObject> result;
        if (!address) return result;
        for (std::size_t i = 0; ; ++i) {
            require(i < 4096, "Unterminated GAME static object list");
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
            require(i < 4096, "Unterminated GAME spawn list");
            const auto w = at(address + static_cast<std::uint32_t>(i * 48), 48);
            const auto id = signed32(w.u32(0));
            if (id == -1) break;
            block.spawns.push_back({id,signed32(w.u32(4)),native(vector3(w,8)),native(vector3(w,20)),
                                    native(vector3(w,32)),w.u32(44) ? decoder.path(w.u32(44)) : no_index,w});
        }
    }
    void definitions() {
        if (!out.setup_.static_definitions_guest) return;
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
        if (block.model == no_index) return;
        auto& model = out.models_[block.model];
        // 60860.c6-39 only augments ordinary A/B callbacks, never sky wrappers.
        if (block.metadata.render_callback_guest == 0x800e1d80) return;
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
        if (!out.setup_.collision_models_guest) return;
        for (std::size_t i = 0; ; ++i) {
            require(i < 100, "Unterminated world collision model table");
            const auto w = at(out.setup_.collision_models_guest + static_cast<std::uint32_t>(i * 12), 12);
            const auto id = signed32(w.u32(0));
            if (id == -1) break;
            out.collision_models_.push_back(decode_collision(reader, w));
        }
    }
    void height(const course::HeightLayout& layout, HeightMap& map) {
        if (!layout.root) return; // Only source-absent maps have null metadata.
        const auto root = at(layout.root,8);
        require(root.u32(0) == layout.patches && root.u32(4) == layout.tree,
                "GAME height root differs from admitted course arrays");
        const auto patch_count = layout.patch_count, node_count = layout.node_count;
        map.wire = root;
        const auto patches = at(layout.patches,patch_count * 20);
        const auto nodes = at(layout.tree,node_count * 28);
        map.patches_wire = patches; map.nodes_wire = nodes;
        auto index = [](std::uint32_t value, std::size_t count) {
            if (value == 0xffffffffu) return no_index;
            require(value < count, "Height tree index escapes exact GAME array");
            return static_cast<std::size_t>(value);
        };
        map.patches.reserve(patch_count); map.nodes.reserve(node_count);
        for (std::size_t i = 0; i < patch_count; ++i) {
            const auto o = i * 20;
            map.patches.push_back({finite(patches,o),finite(patches,o+4),finite(patches,o+8),finite(patches,o+12),patches.u32(o+16)});
        }
        for (std::size_t i = 0; i < node_count; ++i) {
            const auto o = i * 28;
            map.nodes.push_back({finite(nodes,o),finite(nodes,o+4),finite(nodes,o+8),
                index(nodes.u32(o+12),node_count),index(nodes.u32(o+16),node_count),
                index(nodes.u32(o+20),patch_count),index(nodes.u32(o+24),patch_count)});
        }
        std::vector<std::uint8_t> state(node_count), seen_patches(patch_count);
        const auto visit = [&](auto&& self, std::size_t i) -> void {
            if (i == no_index) return;
            require(state[i] == 0, "Cyclic or multiply-parented GAME height tree");
            state[i] = 1;
            const auto& n = map.nodes[i];
            if (n.left_patch != no_index) seen_patches[n.left_patch] = 1;
            if (n.right_patch != no_index) seen_patches[n.right_patch] = 1;
            self(self,n.left_child); self(self,n.right_child);
        };
        visit(visit,0);
        require(std::all_of(state.begin(),state.end(),[](auto v){return v == 1;}) &&
                std::all_of(seen_patches.begin(),seen_patches.end(),[](auto v){return v == 1;}),
                "Height resource extent contains unreachable records");
    }
    void closure() {
        std::vector<model_assets::ModelPlan> plans;
        plans.reserve(out.models_.size());
        for (const auto& model : out.models_) plans.push_back({&model, {}});
        out.closure_.emplace(reader, plans, out.materials_);
        out.closure_->require_complete();
    }
};

Resources::Resources(const rom::Snapshot& snapshot, std::int32_t course_id)
    : snapshot_(&snapshot), source_data_(snapshot.canonical().data()), layout_(&course::layout(course_id)) {
    auto descriptors = course::world_descriptors(snapshot, course_id);
    setup_=descriptors.world; sky_=descriptors.sky; primary_count_=descriptors.primary.size();
    primary_loop_=setup_.primary_table_wire.u32(primary_count_*4)==descriptors.primary.front().wire.guest_address;
    const model_assets::Reader reader(snapshot, layout_->mappings);
    model_assets::DataDecoder decoder(reader, materials_, animations_, {}, &paths_);
    ResourceBuilder builder{*this, reader, decoder, {}};
    for (const auto group : {std::span<const beach::BlockMetadata>(descriptors.primary),
                             std::span<const beach::BlockMetadata>(descriptors.secondary)})
        for (const auto& metadata : group) if (metadata.display_list_guest)
            builder.collect_materials(metadata.materials_guest, metadata.texture_animation_guest, 1, true);
    // createSkyBox installs a live updater and animationSpeed (block.c:179-183),
    // not the terrain's reduced-time restart task. Never lend it that capability.
    if (sky_.display_list_guest) builder.collect_materials(sky_.materials_guest, sky_.animation_guest, 1, false);
    builder.admit_materials();
    builder.definitions();
    blocks_.reserve(primary_count_ + descriptors.secondary.size());
    for (const auto group : {std::span<const beach::BlockMetadata>(descriptors.primary),
                             std::span<const beach::BlockMetadata>(descriptors.secondary)}) for (const auto& metadata : group) {
        BlockResource block{}; block.metadata=metadata;
        if (metadata.display_list_guest) {
            require(metadata.render_callback_guest==layout_->block_render_callback,"Unadmitted GAME course block callback");
            const bool sky_cycle = metadata.render_callback_guest==0x800e1d80;
            block.model=builder.single_model(metadata.display_list_guest,
                sky_cycle ? menu::RenderType::a : menu::RenderType::a_fogged);
            if (sky_cycle) models_[block.model].matrix_kinds={0x1b,0,0};
            builder.attachments(models_[block.model],metadata.materials_guest,metadata.texture_animation_guest);
        }
        require(metadata.control_line_count>=0 && metadata.control_line_count<4,"GAME road exceeds source control capacity");
        if (metadata.control_line_count>=2) {
            const auto count=static_cast<std::size_t>(metadata.control_line_count);
            // The initialized GAME field is cpTimeStamps[1]. Neither actual
            // two- nor three-control-line road consumes an interior timestamp.
            const auto times=builder.at(metadata.gfx_wire.guest_address+32,4);
            block.control_timestamps.push_back(finite(times,0));
            block.road_model=builder.road(metadata.road_guest,metadata.movement_animation_guest,count);
        }
        block.decorations=builder.statics(metadata.statics_guest);
        block.collision_objects=builder.statics(metadata.static_objects_guest);
        builder.spawns(block); builder.decorate(block);
        blocks_.push_back(std::move(block));
    }
    if (sky_.display_list_guest) {
        require(sky_.render_callback_guest==layout_->sky_render_callback,"Unadmitted GAME course sky callback");
        const bool sky_cycle = sky_.render_callback_guest==0x800e1d80;
        sky_model_index_=builder.single_model(sky_.display_list_guest,menu::RenderType::a);
        auto& model=models_[sky_model_index_];
        if (sky_cycle) model.matrix_kinds={0x1b,0,0};
        builder.attachments(model,sky_.materials_guest,sky_.animation_guest);
    }
    builder.collision();
    builder.height(layout_->ground, height_map_); builder.height(layout_->ceiling, ceiling_map_);
    builder.closure();
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
    const auto installation = closure_->installation_spans();
    for (const auto& span:installation) {
        const auto offset=static_cast<std::size_t>(span.guest_address-ram_base);
        require(span.guest_address>=ram_base && offset<=ram.size() && span.bytes.size()<=ram.size()-offset &&
                span.rom_offset<=source.size() && span.bytes.size()<=source.size()-span.rom_offset &&
                span.bytes.data()==source.data()+span.rom_offset,"Invalid world install provenance/extent");
        for (const auto r:reservations) disjoint(span,r);
    }
    for (const auto& span:installation) {
        const auto offset=static_cast<std::size_t>(span.guest_address-ram_base);
        for (std::size_t i=0;i<span.bytes.size();++i) ram[(offset+i)^3u]=span.bytes[i];
    }
}
} // namespace isle::world
