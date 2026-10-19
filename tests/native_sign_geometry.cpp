#include "native_sign_geometry.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace isle::test {
namespace {
using Vec3 = menu::Vec3;
using Rejection = SignGeometry::Rejection;
using Result = SignGeometry::Result;
using Status = SignGeometry::Status;

Result rejected(Rejection reason, std::size_t node = menu::no_index,
                std::uint32_t command = 0) noexcept {
    Result result;
    result.rejection = reason;
    result.node = node;
    result.command_identity = command;
    return result;
}
bool finite(Vec3 v) noexcept {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}
float signed_coordinate(std::uint16_t value) noexcept {
    return static_cast<float>(value < 0x8000u ? static_cast<int>(value) :
                             static_cast<int>(value) - 0x10000);
}
} // namespace

void SignGeometry::include(Bounds& bounds, Vec3 value) noexcept {
    if (!bounds.present) {
        bounds.minimum = bounds.maximum = value;
        bounds.present = true;
        return;
    }
    bounds.minimum.x = std::min(bounds.minimum.x, value.x);
    bounds.minimum.y = std::min(bounds.minimum.y, value.y);
    bounds.minimum.z = std::min(bounds.minimum.z, value.z);
    bounds.maximum.x = std::max(bounds.maximum.x, value.x);
    bounds.maximum.y = std::max(bounds.maximum.y, value.y);
    bounds.maximum.z = std::max(bounds.maximum.z, value.z);
}

// Cold bounded interpreter of the already-admitted immutable asset graph. It
// follows calls and NOPUSH tails, rather than treating descriptor order or a
// vertex address's proximity to a model as geometric membership.
struct SignGeometry::Builder {
    const actor::Resources& resources;
    Model& output;
    std::size_t node = 0, commands = 0, depth = 0;
    std::uint32_t command = 0;
    std::array<std::uint32_t, 32> active{};
    std::array<std::array<Bounds, 32>, 4> vertices{};
    std::array<std::uint32_t, 4> valid{};
    std::array<Bounds, node_capacity> bounds{};

    [[noreturn]] void fail(Rejection reason) const { throw rejected(reason, node, command); }
    void require(bool condition, Rejection reason) const {
        if (!condition) fail(reason);
    }
    void membership(std::uint32_t address) {
        for (std::size_t i = 0; i < output.memberships; ++i)
            if (output.lists[i].node == node && output.lists[i].list == address) return;
        require(output.memberships < membership_capacity, Rejection::capacity);
        output.lists[output.memberships++] = {address, static_cast<std::uint16_t>(node)};
    }
    void triangle(unsigned lane, std::uint32_t encoded) {
        require(!(encoded & 0xff000000u), Rejection::geometry_command);
        for (unsigned shift : {16u, 8u, 0u}) {
            const auto index = (encoded >> shift) & 255u;
            require(!(index & 1u) && index < 64, Rejection::geometry_command);
            const auto slot = index / 2;
            require(valid[lane] & (std::uint32_t{1} << slot), Rejection::incoming_vertices);
            include(bounds[node], vertices[lane][slot].minimum);
            include(bounds[node], vertices[lane][slot].maximum);
        }
    }
    void load(unsigned lane, std::uint32_t w0, std::uint32_t w1) {
        const auto count = (w0 >> 12) & 255u, end = (w0 & 4095u) >> 1;
        require(count && end >= count && end <= 32 &&
                (w0 & 0xfff00fffu) == (0x01000000u | end * 2),
                Rejection::vertex_descriptor);
        const menu::VertexLoad* descriptor = nullptr;
        for (const auto& entry : resources.vertex_loads()) {
            if (entry.command_identity != command) continue;
            require(!descriptor, Rejection::vertex_descriptor);
            descriptor = &entry;
        }
        require(descriptor && descriptor->count == count &&
                descriptor->first_slot == end - count &&
                descriptor->wire.guest_address == w1 &&
                descriptor->wire.bytes.size() == count * 16,
                Rejection::vertex_descriptor);
        for (unsigned i = 0; i < count; ++i) {
            const auto offset = i * 16;
            auto& vertex = vertices[lane][end - count + i];
            vertex = {};
            include(vertex, {
                signed_coordinate(descriptor->wire.u16(offset)),
                signed_coordinate(descriptor->wire.u16(offset + 2)),
                signed_coordinate(descriptor->wire.u16(offset + 4))});
            valid[lane] |= std::uint32_t{1} << (end - count + i);
        }
    }
    void execute(std::uint32_t address, unsigned lane, bool pre) {
        require(depth < active.size(), Rejection::capacity);
        for (std::size_t i = 0; i < depth; ++i)
            require(active[i] != address, Rejection::display_list);
        active[depth++] = address;
        membership(address);
        const menu::DisplayList* list = nullptr;
        for (const auto& entry : resources.display_lists()) {
            if (entry.wire.guest_address != address) continue;
            require(!list, Rejection::display_list);
            list = &entry;
        }
        require(list && !list->wire.bytes.empty() && list->wire.bytes.size() % 8 == 0,
                Rejection::display_list);
        // CULLDL may return from THIS list at any tested instruction. Preserve
        // each possible exit cache as well as the continuation cache: merely
        // ignoring culls could lose caller triangles that use pre-cull vertices.
        std::array<Bounds, 32> exits{};
        std::uint32_t exit_valid = 0;
        bool has_exit = false;
        const auto merge_bounds = [](auto& destination, const auto& source) {
            for (std::size_t slot = 0; slot < destination.size(); ++slot)
                if (source[slot].present) {
                    include(destination[slot], source[slot].minimum);
                    include(destination[slot], source[slot].maximum);
                }
        };
        const auto cull_exit = [&] {
            if (!has_exit) {
                exits = vertices[lane];
                exit_valid = valid[lane];
                has_exit = true;
            } else {
                merge_bounds(exits, vertices[lane]);
                exit_valid &= valid[lane];
            }
        };
        const auto finish = [&] {
            if (has_exit) {
                merge_bounds(vertices[lane], exits);
                valid[lane] &= exit_valid;
            }
            --depth;
        };
        for (std::size_t offset = 0; offset < list->wire.bytes.size(); offset += 8) {
            require(++commands <= 1048576, Rejection::capacity);
            require(offset <= std::numeric_limits<std::uint32_t>::max() - address,
                    Rejection::display_list);
            command = address + static_cast<std::uint32_t>(offset);
            const auto w0 = list->wire.u32(offset), w1 = list->wire.u32(offset + 4);
            switch (w0 >> 24) {
            case 0x01:
                // Immediate I/B pre-lists execute BEFORE the node matrix. Do
                // not misattribute their vertices to this node's world cache.
                require(!pre, Rejection::geometry_command);
                load(lane, w0, w1);
                break;
            case 0x02: {
                // RGBA/ST modification cannot move a bounding vertex. Screen
                // XY/Z modification is not an object-space geometry operation.
                const auto field = (w0 >> 16) & 255u;
                require(!pre && (field == 0x10u || field == 0x14u) &&
                        !(w0 & 1u) && (w0 & 65535u) < 64,
                        Rejection::geometry_command);
                break;
            }
            case 0x03: {
                const auto first = w0 & 65535u, last = w1;
                require((w0 & 0xffff0000u) == 0x03000000u &&
                        !((first | last) & 1u) && first <= last && last <= 62,
                        Rejection::geometry_command);
                cull_exit();
                break;
            }
            case 0x05:
                require(!pre && w1 == 0, Rejection::geometry_command);
                triangle(lane, w0 & 0x00ffffffu);
                break;
            case 0x06:
                require(!pre, Rejection::geometry_command);
                triangle(lane, w0 & 0x00ffffffu);
                triangle(lane, w1);
                break;
            case 0xde:
                require(w0 == 0xde000000u || w0 == 0xde010000u,
                        Rejection::display_list);
                if ((w1 >> 24) == 0x0e) {
                    // Closure::material resolves this exact command/target
                    // pair to real generated material programs. encode_material
                    // emits texture/light/color state and ENDDL only; no model
                    // matrix, vertex load, primitive or nested DL is generated.
                    // Do not treat arbitrary Segment14 targets as geometry-free.
                    require(!(w1 & 7u), Rejection::display_list);
                    bool found = false;
                    for (const auto& call : resources.dynamic_material_calls())
                        if (call.command_identity == command && call.segmented_target == w1) {
                            require(call.material_index < resources.materials().size(),
                                    Rejection::display_list);
                            found = true;
                        }
                    require(found, Rejection::display_list);
                } else execute(w1, lane, pre);
                if (w0 == 0xde010000u) { finish(); return; }
                break;
            case 0xdf:
                require(w0 == 0xdf000000u && w1 == 0, Rejection::display_list);
                finish();
                return;
            // Closure-admitted non-positional F3DEX2/RDP state. None changes
            // modelview, vertices, or control flow. The immutable descriptors
            // already establish texture/light/material address closure.
            case 0xd7: case 0xd9: case 0xdb: case 0xe1:
            case 0xe2: case 0xe3: case 0xe6: case 0xe7: case 0xe8:
            case 0xf0: case 0xf2: case 0xf3: case 0xf4: case 0xf5:
            case 0xf9: case 0xfa: case 0xfb: case 0xfc: case 0xfd:
                break;
            // BRANCH_Z's alternate graph is not interpreted here. Matrix,
            // viewport, QUAD and all unadmitted opcodes also fail closed.
            default: fail(Rejection::geometry_command);
            }
        }
        fail(Rejection::display_list); // An admitted list has an explicit exit.
    }
    void build(const actor::ActorInitialization& initialization) {
        bool actual = false;
        for (const auto& entry : resources.initializations())
            if (&entry == &initialization) actual = true;
        require(actual && initialization.model < resources.models().size(),
                Rejection::initialization);
        const auto& source = resources.models()[initialization.model];
        require(!source.nodes.empty() && source.nodes.size() <= node_capacity,
                Rejection::capacity);
        output.count = source.nodes.size();
        switch (source.render_type) {
        case menu::RenderType::a: case menu::RenderType::a_fogged:
            output.root_only = true;
            break;
        case menu::RenderType::b: case menu::RenderType::b_fogged:
        case menu::RenderType::d_fogged: case menu::RenderType::i_fogged:
        case menu::RenderType::i_no_fog: case menu::RenderType::j_fogged:
            output.root_only = false;
            break;
        default: fail(Rejection::render_type);
        }
        std::array<std::size_t, node_capacity> ancestors{};
        std::size_t ancestors_count = 0;
        for (node = 0; node < output.count; ++node) {
            const auto parent = source.nodes[node].parent;
            require(node ? parent < node : parent == menu::no_index, Rejection::topology);
            if (node) {
                while (ancestors_count && ancestors[ancestors_count - 1] != parent)
                    output.nodes[ancestors[--ancestors_count]].subtree_end = node;
                require(ancestors_count != 0, Rejection::topology);
            }
            auto& cached = output.nodes[node];
            cached.geometry = false;
            cached.parent = parent;
            ancestors[ancestors_count++] = node;
        }
        while (ancestors_count)
            output.nodes[ancestors[--ancestors_count]].subtree_end = output.count;
        const bool deferred = source.render_type == menu::RenderType::d_fogged ||
                              source.render_type == menu::RenderType::j_fogged;
        for (node = 0; node < (output.root_only ? 1 : output.count); ++node) {
            command = 0;
            menu::model::MatrixRecipe recipe;
            require(static_cast<bool>(menu::model::decode_model_recipe(source, node, recipe)),
                    Rejection::matrix_recipe);
            // Ordinary cached world matrices are RTS (RT for 27), not the
            // rendered billboard matrix. For exact 18+44, only the invariant
            // origin can be reused, under the further cold/live gates below.
            const bool billboard = recipe.count == 2 && recipe.operations[0] == 18 &&
                                   recipe.operations[1] == 44;
            require(billboard || (recipe.count == 1 &&
                    (recipe.operations[0] == 28 || recipe.operations[0] == 27)),
                    Rejection::matrix_recipe);
            output.nodes[node].origin_only = billboard ||
                (output.nodes[node].parent != menu::no_index &&
                 output.nodes[output.nodes[node].parent].origin_only);
            output.nodes[node].billboard44 = billboard;
            output.nodes[node].matrix_kind = recipe.native_kind;
            valid = {};
            const auto& payloads = source.nodes[node].payloads;
            if (output.root_only && !payloads.empty())
                require(payloads.size() == 1 && payloads[0].display_list_id == 0 &&
                        payloads[0].pre_list == 0, Rejection::render_type);
            for (const auto& payload : payloads) {
                require(payload.display_list_id < 4, Rejection::render_type);
                if (payload.pre_list && (!deferred || payload.draw_list))
                    execute(payload.pre_list, deferred ? payload.display_list_id : 0, true);
                if (payload.draw_list) execute(payload.draw_list, payload.display_list_id, false);
            }
            const auto& bound = bounds[node];
            auto& cached = output.nodes[node];
            cached.geometry = bound.present;
            if (bound.present) {
                cached.center = {
                    bound.minimum.x * 0.5f + bound.maximum.x * 0.5f,
                    bound.minimum.y * 0.5f + bound.maximum.y * 0.5f,
                    bound.minimum.z * 0.5f + bound.maximum.z * 0.5f};
                cached.half_extent = {
                    bound.maximum.x * 0.5f - bound.minimum.x * 0.5f,
                    bound.maximum.y * 0.5f - bound.minimum.y * 0.5f,
                    bound.maximum.z * 0.5f - bound.minimum.z * 0.5f};
                require(!cached.origin_only ||
                        (cached.center.x == 0 && cached.center.y == 0 &&
                         (billboard ? cached.half_extent.z == 0 : cached.center.z == 0)),
                        Rejection::matrix_recipe);
                output.axis_camera |= cached.origin_only && cached.center.z != 0;
            }
        }
        output.admission = {};
        output.admission.status = Status::ready;
    }
};

SignGeometry::Result SignGeometry::observe(const actor::Resources& resources,
    const actor::ActorInitialization& initialization,
    std::span<const menu::model::WorldMatrix> world,
    std::span<const IsleGameModelNode> native, bool object_visible,
    const BillboardView* billboard) noexcept {
    Model* selected = nullptr;
    for (std::size_t i = 0; i < count_; ++i)
        if (models_[i].resources == &resources && models_[i].initialization == &initialization)
            selected = &models_[i];
    if (!selected) {
        if (count_ == model_capacity) return rejected(Rejection::capacity);
        selected = &models_[count_++];
        selected->resources = &resources;
        selected->initialization = &initialization;
        selected->count = selected->memberships = 0;
        selected->axis_camera = false;
        try {
            Builder{resources, *selected}.build(initialization);
        } catch (const Result& error) {
            selected->admission = error;
        } catch (...) {
            selected->admission = rejected(Rejection::display_list);
        }
    }
    if (selected->admission.status != Status::ready) return selected->admission;
    if (world.size() != selected->count || native.size() != selected->count)
        return rejected(Rejection::world_view);
    Result result;
    result.status = Status::no_visible_geometry;
    const auto invisible = [](std::size_t node = menu::no_index) noexcept {
        Result value;
        value.status = Status::no_visible_geometry;
        value.node = node;
        return value;
    };
    if (!object_visible) return result;
    // Off-origin billboard planes need the ACTUAL prepared centered camera and
    // actual placement scale. No matrix is computed or native API called here.
    float placement_x = 1;
    if (selected->axis_camera) {
        if (!billboard || !billboard->perspective || !billboard->view || !billboard->placement)
            return rejected(Rejection::camera_view);
        if (billboard->perspective->kind != ISLE_GAME_CAMERA_PERSPECTIVE_FAST ||
            billboard->view->kind != ISLE_GAME_CAMERA_LOOK_AT_REFLECT_ROLL)
            return rejected(Rejection::camera_view);
        const auto& p = billboard->perspective->floating;
        const auto& v = billboard->view->floating;
        for (unsigned row = 0; row < 4; ++row)
            for (unsigned column = 0; column < 4; ++column) {
                if (!std::isfinite(p[row][column]) || !std::isfinite(v[row][column]))
                    return invisible();
                const bool perspective_entry = (row == column && row < 3) ||
                    (row == 2 && column == 3) || (row == 3 && column == 2);
                if (!perspective_entry && p[row][column] != 0)
                    return rejected(Rejection::camera_view);
            }
        if (!(p[0][0] > 0 && p[1][1] > 0 && p[2][3] < 0) ||
            v[0][3] != 0 || v[1][3] != 0 || v[2][3] != 0 || v[3][3] != 1)
            return rejected(Rejection::camera_view);
        switch (billboard->placement->mode) {
        case ISLE_GAME_MODEL_PLACEMENT_DISABLED: case ISLE_GAME_MODEL_PLACEMENT_RT:
            break;
        case ISLE_GAME_MODEL_PLACEMENT_RTS:
            placement_x = billboard->placement->transform.scale.x;
            if (!std::isfinite(placement_x)) return invisible();
            break;
        default: return rejected(Rejection::camera_view);
        }
    }
    std::array<float, node_capacity> inherited_x;
    Bounds total;
    bool invariant = false;
    bool optical_axis = false;
    Vec3 invariant_origin{};
    const auto count = selected->root_only ? 1 : selected->count;
    for (std::size_t n = 0; n < count; ++n) {
        const auto& cached = selected->nodes[n];
        const auto& pose = native[n];
        if (pose.parent != (cached.parent == menu::no_index ? UINT32_MAX : cached.parent) ||
            pose.matrix_kind != cached.matrix_kind)
            return rejected(Rejection::pose, n);
        if (selected->root_only ? pose.flags != 0 : (pose.flags & 2u) != 0) {
            n = cached.subtree_end - 1;
            continue;
        }
        if (!finite(pose.transform.position) || !finite(pose.transform.rotation) ||
            !finite(pose.transform.scale))
            return selected->axis_camera ? invisible(n) : rejected(Rejection::pose, n);
        if (selected->axis_camera) {
            float scale = cached.parent == menu::no_index ? placement_x : inherited_x[cached.parent];
            if (cached.billboard44 ||
                cached.matrix_kind == ISLE_GAME_MODEL_ROTATE_RPY_TRANSLATE_SCALE)
                scale *= pose.transform.scale.x;
            if (!std::isfinite(scale)) return invisible(n);
            inherited_x[n] = scale;
        }
        // Source44 replaces rows0..2 of combined MVP, never row3. A zero
        // descendant translation cannot consume those differing linear rows;
        // hence the cache's row3 remains the same mathematical anchor (not a
        // bit-exact claim about GPU fixed-point quantization).
        if (cached.origin_only && cached.parent != menu::no_index &&
            selected->nodes[cached.parent].origin_only &&
            (pose.transform.position.x != 0 || pose.transform.position.y != 0 ||
             pose.transform.position.z != 0))
            return rejected(Rejection::matrix_recipe, n);
        if ((pose.flags & 1u) || !cached.geometry) continue;
        const auto& matrix = world[n].m;
        for (const auto& row : matrix)
            for (const float value : row)
                if (!std::isfinite(value))
                    return selected->axis_camera ? invisible(n) : rejected(Rejection::arithmetic, n);
        if (matrix[0][3] != 0 || matrix[1][3] != 0 ||
            matrix[2][3] != 0 || matrix[3][3] != 1)
            return rejected(Rejection::world_view, n);
        if (cached.origin_only) {
            const Vec3 origin{matrix[3][0], matrix[3][1], matrix[3][2]};
            // Unknown billboard extents cannot be unioned with ordinary bounds
            // or a different origin. The shared center alone is proved.
            if (total.present || (invariant &&
                (origin.x != invariant_origin.x || origin.y != invariant_origin.y ||
                 origin.z != invariant_origin.z)))
                return rejected(Rejection::matrix_recipe, n);
            if (selected->axis_camera && cached.billboard44) {
                const auto& v = billboard->view->floating;
                const auto& p = billboard->perspective->floating;
                const float view_z = ((origin.x * v[0][2] + origin.y * v[1][2]) +
                                      origin.z * v[2][2]) + v[3][2];
                const float anchor_w = view_z * p[2][3];
                const float replacement_w = p[2][3] * inherited_x[n];
                const float plane_w = cached.center.z * replacement_w + anchor_w;
                // Source44 adds no clip X/Y for (0,0,z). Thus aiming this
                // anchor onto the optical axis also centres the planar mesh.
                // Away from that fixed point, W differs: this is NOT its
                // current projected centre, a world extent, or focus evidence.
                if (!std::isfinite(anchor_w) || !std::isfinite(plane_w) ||
                    anchor_w <= 0 || plane_w <= 0)
                    return invisible(n);
                optical_axis |= cached.center.z != 0;
            }
            invariant = true;
            invariant_origin = origin;
            ++result.visible_nodes;
            continue;
        }
        if (invariant) return rejected(Rejection::matrix_recipe, n);
        // Transform a point and absolute linear extents, not native/renderer
        // matrices. This is the eight-corner AABB without eight point products.
        const auto c = cached.center, e = cached.half_extent;
        const Vec3 center{
            ((c.x * matrix[0][0] + c.y * matrix[1][0]) + c.z * matrix[2][0]) + matrix[3][0],
            ((c.x * matrix[0][1] + c.y * matrix[1][1]) + c.z * matrix[2][1]) + matrix[3][1],
            ((c.x * matrix[0][2] + c.y * matrix[1][2]) + c.z * matrix[2][2]) + matrix[3][2]};
        const Vec3 extent{
            e.x * std::abs(matrix[0][0]) + e.y * std::abs(matrix[1][0]) + e.z * std::abs(matrix[2][0]),
            e.x * std::abs(matrix[0][1]) + e.y * std::abs(matrix[1][1]) + e.z * std::abs(matrix[2][1]),
            e.x * std::abs(matrix[0][2]) + e.y * std::abs(matrix[1][2]) + e.z * std::abs(matrix[2][2])};
        const Vec3 minimum{center.x - extent.x, center.y - extent.y, center.z - extent.z};
        const Vec3 maximum{center.x + extent.x, center.y + extent.y, center.z + extent.z};
        if (!finite(minimum) || !finite(maximum)) return rejected(Rejection::arithmetic, n);
        include(total, minimum);
        include(total, maximum);
        ++result.visible_nodes;
    }
    if (invariant) {
        result.target = invariant_origin;
        result.kind = optical_axis ? TargetKind::optical_axis_hint : TargetKind::invariant_origin;
        result.status = Status::ready;
        return result;
    }
    if (!total.present) return result;
    result.minimum = total.minimum;
    result.maximum = total.maximum;
    result.target = {
        total.minimum.x * 0.5f + total.maximum.x * 0.5f,
        total.minimum.y * 0.5f + total.maximum.y * 0.5f,
        total.minimum.z * 0.5f + total.maximum.z * 0.5f};
    if (!finite(result.target)) return rejected(Rejection::arithmetic);
    result.status = Status::ready;
    return result;
}

void SignGeometry::reset() noexcept { count_ = 0; }

} // namespace isle::test
