#include "borrowed_model_kernel.hpp"
#include "graphics_task.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

extern "C" void hal_rotate_rpy_translate_scale_f(float [4][4], float, float, float,
    float, float, float, float, float, float);
extern "C" void hal_rotate_rpy_translate_f(float [4][4], float, float, float, float, float, float);
extern "C" void hal_mtx_f2l(float [4][4], IsleGameFixedMatrix*);

namespace isle::menu::model {
namespace {
struct Failure { Status status; };
void require(bool v, Error error = Error::invalid_input) { if (!v) throw Failure{{error}}; }
void native(IsleGameResult r) { require(r == ISLE_GAME_OK,Error::native); }
std::int32_t integer(float f) {
    require(std::isfinite(f) && double(f) >= -2147483648.0 && double(f) < 2147483648.0);
    return static_cast<std::int32_t>(f);
}
IsleGameVec3 vec(Vec3 v) { return {v.x,v.y,v.z}; }
IsleGameModelTransform transform(const animation::Transform& t, bool translate) {
    return {vec(t.position),translate ? IsleGameVec3{} : vec(t.rotation),
        translate ? IsleGameVec3{1,1,1} : vec(t.scale)};
}
bool same(IsleGameVec3 a, IsleGameVec3 b) { return a.x == b.x && a.y == b.y && a.z == b.z; }
bool same(const IsleGameModelTransform& a, const IsleGameModelTransform& b) {
    return same(a.position,b.position) && same(a.rotation,b.rotation) && same(a.scale,b.scale);
}
void finite(const WorldMatrix& m) {
    for (const auto& row : m.m) for (float f : row) require(std::isfinite(f));
}
WorldMatrix local(const IsleGameModelTransform& t) {
    WorldMatrix m;
    hal_rotate_rpy_translate_scale_f(m.m,t.position.x,t.position.y,t.position.z,
        t.rotation.x,t.rotation.y,t.rotation.z,t.scale.x,t.scale.y,t.scale.z);
    finite(m); return m;
}
WorldMatrix compose(const WorldMatrix& a, const WorldMatrix& b) {
    WorldMatrix out;
    for (unsigned r = 0; r < 4; ++r) for (unsigned c = 0; c < 4; ++c)
        out.m[r][c] = ((a.m[r][0]*b.m[0][c] + a.m[r][1]*b.m[1][c]) +
            a.m[r][2]*b.m[2][c]) + a.m[r][3]*b.m[3][c];
    finite(out); return out;
}
}
Status encode_material(const Material& texture, const animation::MaterialState& state,
                       MaterialCommands& output) noexcept {
    try {
        MaterialCommands staged;
        auto cmd = [&](std::uint32_t a, std::uint32_t b) {
            require(staged.count < staged.words.size(), Error::extent);
            staged.words[staged.count++] = {a,b};
        };
        const unsigned flags = texture.flags ? texture.flags : 0xa1;
        require((flags & ~0x3fffu) == 0 && texture.format <= 4 && texture.size_code <= 3 &&
                texture.auxiliary_format <= 4 && texture.auxiliary_size_code <= 3);
        for (float f : state.parameters) require(std::isfinite(f));
        unsigned image = state.image_index, next = state.next_image_index;
        auto image_at = [&](unsigned i) {
            require(i < texture.images.size() && texture.images[i] >= 0x80000000u &&
                texture.images[i] - 0x80000000u < rdram_size);
            return texture.images[i];
        };
        float scale_s = state.parameters[2], scale_t = state.parameters[3];
        float origin_s = state.parameters[0], origin_t = state.parameters[1];
        float aux_s = state.parameters[6], aux_t = state.parameters[7];
        if (flags & 0xe0) {
            require(texture.wire.bytes.size() >= 0x78);
            if (texture.wire.u32(0x10)) {
                scale_s *= 0.5f;
                origin_s = (origin_s - state.parameters[4] + 1.0f - state.parameters[5] * 0.5f) * 0.5f;
                aux_s = (aux_s - state.parameters[8] + 1.0f - state.parameters[5] * 0.5f) * 0.5f;
            }
        }
        if (flags & 4) {
            const auto palette = integer(state.palette_index);
            require(palette >= 0 && static_cast<std::size_t>(palette) < texture.palettes.size());
            const auto identity = texture.palettes[static_cast<std::size_t>(palette)];
            require(identity >= 0x80000000u && identity - 0x80000000u < rdram_size);
            cmd(0xfd100000, identity);
            if (flags & 3) {
                cmd(0xe8000000,0); cmd(0xf5000100,0x05000000);
                cmd(0xe6000000,0); cmd(0xf0000000,0x05000000u |
                    ((texture.size_code == 1 ? 255u : 15u) << 14));
                cmd(0xe7000000,0);
            }
        }
        for (unsigned light = 0; light < 2; ++light) if (flags & (0x1000u << light)) {
            const auto color = state.color_and_trailing_words[4 + light];
            cmd(0xdb0a0000u | (light * 24),color);
            cmd(0xdb0a0004u | (light * 24),color);
        }
        if (flags & 0x218) {
            std::int32_t lod;
            if (flags & 0x10) {
                const auto level = integer(state.lod_level);
                require(level >= 0 && level < 65535);
                image = static_cast<unsigned>(level); next = image + 1;
                lod = integer((state.lod_level - static_cast<float>(level)) * 256.0f);
            } else lod = integer(state.lod_level * 255.0f);
            const auto minimum = (state.color_and_trailing_words[1] >> 16) & 255;
            cmd(0xfa000000u | (minimum << 8) | (std::uint32_t(lod) & 255),
                state.color_and_trailing_words[0]);
        }
        if (flags & 0x400) cmd(0xfb000000,state.color_and_trailing_words[2]);
        if (flags & 0x800) cmd(0xf9000000,state.color_and_trailing_words[3]);
        if (flags & 0x12) {
            const unsigned size = texture.auxiliary_size_code;
            cmd(0xfd000000u | (std::uint32_t(texture.auxiliary_format) << 21) |
                ((size == 3 ? 3u : 2u) << 19), image_at(next));
            if (flags & 0x11) {
                require(texture.block_width && texture.block_height);
                const unsigned pixels = unsigned(texture.block_width) * texture.block_height;
                const unsigned texels = size == 0 ? (pixels + 3) / 4 :
                    size == 1 ? (pixels + 1) / 2 : pixels;
                const unsigned line = std::max(1u,size == 0 ? unsigned(texture.block_width) / 16 :
                    (unsigned(texture.block_width) * (1u << (size - 1))) / 8);
                require(texels && texels <= 4096);
                const unsigned dxt = (2048 + line - 1) / line;
                require(dxt <= 4095);
                cmd(0xe6000000,0);
                cmd(0xf3000000,0x06000000u | ((texels - 1) << 12) | dxt);
                cmd(0xe6000000,0);
            }
        }
        if (flags & 0x11)
            cmd(0xfd000000u | (std::uint32_t(texture.format) << 21) |
                (std::uint32_t(texture.size_code) << 19),image_at(image));
        auto tile = [&](unsigned index, unsigned width, unsigned height, float os, float ot) {
            require(width && height);
            const auto u = std::abs(scale_s) > 1.0f / 65535 ?
                integer(((os * static_cast<float>(width) + texture.image_origin) / scale_s) * 4.0f) : 0;
            const auto v = std::abs(scale_t) > 1.0f / 65535 ?
                integer((((1.0f - scale_t - ot) * static_cast<float>(height) + texture.image_origin) / scale_t) * 4.0f) : 0;
            cmd(0xf2000000u | ((std::uint32_t(u) & 4095) << 12) | (std::uint32_t(v) & 4095),
                (index << 24) | ((std::uint32_t(u + std::int64_t((width - 1) * 4)) & 4095) << 12) |
                (std::uint32_t(v + std::int64_t((height - 1) * 4)) & 4095));
        };
        if (flags & 0x20) tile(0,texture.width,texture.height,origin_s,origin_t);
        if (flags & 0x40) tile(1,texture.auxiliary_width,texture.auxiliary_height,aux_s,aux_t);
        if (flags & 0x80) {
            require(texture.scale);
            auto scale = [&](float value) {
                const auto n = std::abs(value) > 1.0f / 65535 ?
                    integer(2097152.0f / static_cast<float>(texture.scale) / value) : 0;
                return std::uint32_t(std::min(n,65535)) & 65535;
            };
            cmd(0xd7000002,(scale(scale_s) << 16) | scale(scale_t));
        }
        cmd(0xdf000000,0);
        output = staged;
        return {};
    } catch (const Failure& f) { return f.status; }
      catch (...) { return {Error::invalid_input}; }
}
namespace {
struct HierarchyEncoder {
    const HierarchyInput& input;
    HierarchyBuffer& output;
    std::array<std::size_t,4> copied{};
    std::size_t pending_count = 0;
    void command(unsigned lane, std::uint32_t a, std::uint32_t b) {
        require(lane < 4 && output.counts[lane] < output.lanes[lane].size(),Error::extent);
        output.lanes[lane][output.counts[lane]++] = {a,b};
    }
    void call(unsigned lane, std::uint32_t target) { command(lane,0xde000000,target); }
    void append(Command c) {
        require(pending_count < output.pending.size(),Error::extent);
        output.pending[pending_count++] = c;
    }
    void matrix(std::size_t n, bool push, float parent_scale) {
        append({push ? 0xda380000u : 0xda380001u,
            input.matrices + static_cast<std::uint32_t>(n * 64)});
        const auto recipe = input.source.nodes[n].depth_flags & 0xf000u;
        require(recipe == 0 || recipe == 0x8000u);
        if (!recipe) return;
        WorldMatrix matrix;
        const auto scale = input.nodes[n].transform.scale;
        const auto x = parent_scale * scale.x;
        const auto y = scale.y * parent_scale;
        matrix.m[0][0] = input.projection.m[0][0] * x;
        matrix.m[1][1] = input.projection.m[1][1] * y;
        matrix.m[2][2] = input.projection.m[2][2] * x;
        matrix.m[2][3] = input.projection.m[2][3] * x;
        for (const auto& row : matrix.m) for (float f : row)
            require(std::isfinite(f) && double(f * 65536.0f) >= -2147483648.0 &&
                double(f * 65536.0f) < 2147483648.0);
        IsleGameFixedMatrix fixed;
        hal_mtx_f2l(matrix.m,&fixed);
        append({0xd5000001,0});
        // Source kind44 replaces three combined-MVP rows, retaining translation.
        for (unsigned row = 0; row < 3; ++row) for (unsigned pair = 0; pair < 2; ++pair)
            append({0xdb000000u | (row * 8 + pair * 4),
                fixed.m[row / 2][(row % 2) * 2 + pair]});
        for (unsigned row = 0; row < 3; ++row) for (unsigned pair = 0; pair < 2; ++pair)
            append({0xdb000020u | (row * 8 + pair * 4),
                fixed.m[2 + row / 2][(row % 2) * 2 + pair]});
    }
    void flush(unsigned lane) {
        while (copied[lane] < pending_count) {
            const auto c = output.pending[copied[lane]++];
            command(lane,c.w0,c.w1);
        }
    }
    void node(std::size_t n, float parent_scale, bool deferred) {
        const auto& pose = input.nodes[n];
        const auto flags = input.native_nodes.empty() ? pose.flags : input.native_nodes[n].flags;
        const auto sibling = input.next_sibling[n], child = input.first_child[n];
        if (flags & 2) return;
        const auto saved = pending_count;
        const bool push = pose.parent == no_index || sibling != UINT32_MAX;
        if (!deferred && !(flags & 1))
            for (const auto& payload : input.source.nodes[n].payloads)
                if (payload.pre_list) call(0,payload.pre_list);
        const bool placed_root = pose.parent == no_index &&
            input.placement.mode != ISLE_GAME_MODEL_PLACEMENT_DISABLED;
        if (placed_root) append({0xda380000u,input.placement_address});
        matrix(n,push && !placed_root,parent_scale);
        if (!deferred) flush(0);
        if (!(flags & 1)) for (const auto& payload : input.source.nodes[n].payloads) {
            const auto lane = payload.display_list_id;
            require(lane < 4);
            if (deferred && payload.pre_list) call(lane,payload.pre_list);
            if (deferred) flush(lane);
            if (pose.material_count) command(lane,0xdb060038,input.material_bases[n]);
            call(lane,payload.draw_list);
        }
        const bool source_scale = input.native_nodes.empty() ||
            (input.source.nodes[n].depth_flags & 0xf000u) == 0x8000u;
        const float scale = parent_scale * (source_scale ? pose.transform.scale.x :
            input.native_nodes[n].transform.scale.x);
        require(std::isfinite(scale));
        for (auto c = child; c != UINT32_MAX; c = input.next_sibling[c]) node(c,scale,deferred);
        pending_count = saved;
        for (unsigned lane = 0; lane < 4; ++lane) if (copied[lane] > saved) {
            copied[lane] = saved;
            if (push) command(lane,0xd8380002,64);
        }
    }
};
}
Status encode_hierarchy(const HierarchyInput& input, HierarchyBuffer& output) noexcept {
    try {
        const auto count = input.nodes.size();
        require(count && input.source.nodes.size() == count && input.first_child.size() == count &&
            input.next_sibling.size() == count && input.material_bases.size() == count &&
            input.nodes[0].parent == no_index &&
            (input.native_nodes.empty() || input.native_nodes.size() == count));
        require((input.source.matrix_kinds[0] == 0x1c || input.source.matrix_kinds[0] == 1) &&
            input.source.matrix_kinds[1] == 0 && input.source.matrix_kinds[2] == 0);
        for (const auto& node : input.source.nodes) {
            const auto recipe = node.depth_flags & 0xf000u;
            require(recipe == 0 || recipe == 0x8000u);
        }
        const bool deferred = input.source.render_type == RenderType::d_fogged ||
            input.source.render_type == RenderType::j_fogged;
        HierarchyEncoder encoder{input,output};
        encoder.node(0,input.placement.mode == ISLE_GAME_MODEL_PLACEMENT_RTS ?
            input.placement.transform.scale.x : 1.0f,deferred);
        return {};
    } catch (const Failure& f) { return f.status; }
      catch (...) { return {Error::invalid_input}; }
}
struct BorrowedKernel::Impl {
    enum class Recipe { rts, kind44 };
    struct Resource { std::vector<Recipe> recipes; };
    struct Key {
        animation::InstanceHandle instance;
        std::size_t resource = no_index;
        IsleGameObject* object = nullptr;
        IsleGameModel* model = nullptr;
        PoseSource pose = PoseSource::animation;
        RootRecipe root = RootRecipe::source;
        bool operator==(const Key&) const = default;
    };
    struct Slot {
        KernelSlot plan;
        BorrowedBinding staged;
        Key key, resident_key, encoded_key;
        bool live = false, present = false, draw = false, live_draw = false, identity_changed = false;
        bool resident = false, encoded_valid = false;
        std::uint32_t* ram = nullptr;
        std::uint64_t epoch = 0;
        std::span<const IsleGameModelNode> nodes, published_nodes;
        const IsleGameModelPlacement* placement = nullptr;
        std::uint32_t placement_address = 0, published_placement_address = 0, resident_placement_address = 0;
        std::uint64_t placement_revision = 0, resident_placement_revision = 0;
        IsleGameModelPlacementMode placement_mode = ISLE_GAME_MODEL_PLACEMENT_DISABLED;
        float placement_x = 1;
        std::size_t live_count = 0;
        std::size_t topology_resource = no_index;
        const animation::Runtime* owner = nullptr;
        std::vector<std::uint32_t> first, next, bases, flags;
        std::vector<std::uint64_t> poses, matrices, materials, encoded_revisions;
        std::vector<std::size_t> material_resources, encoded_resources;
        std::vector<IsleGameModelTransform> transforms;
        std::vector<WorldMatrix> world, staged_world;
        std::vector<unsigned char> changed;
        std::vector<MaterialCommands> encoded;
        Slot(KernelSlot p) : plan(p),first(p.capacity.nodes),next(p.capacity.nodes),bases(p.capacity.nodes),
            flags(p.capacity.nodes),poses(p.capacity.nodes),matrices(p.capacity.nodes),
            materials(p.capacity.material_attachments),encoded_revisions(p.capacity.material_attachments),
            material_resources(p.capacity.material_attachments,no_index),encoded_resources(p.capacity.material_attachments,no_index),
            transforms(p.capacity.nodes),world(p.capacity.nodes),staged_world(p.capacity.nodes),
            changed(p.capacity.nodes),encoded(p.capacity.material_attachments) {}
        Key staged_key() const { return {staged.state.instance,staged.state.resource_model,
            staged.object,staged.model,staged.pose,staged.root}; }
    };
    IsleGame* game;
    std::span<const Model> models;
    std::span<const Material> materials;
    RamRange placements;
    std::vector<Resource> resources;
    std::vector<Slot> slots;
    bool staged = false, dirty = true;
    Impl(IsleGame* g, std::span<const Model> m, std::span<const Material> t,
         std::span<const KernelSlot> plans, RamRange p) : game(g),models(m),materials(t),placements(p) {
        require(game && !plans.empty());
        require(p.offset % 64 == 0 && p.size % 64 == 0 && p.offset <= rdram_size &&
            p.size <= rdram_size-p.offset,Error::extent);
        resources.resize(models.size());
        for (std::size_t r = 0; r < models.size(); ++r) {
            const auto& source = models[r];
            require(!source.nodes.empty());
            require((source.matrix_kinds[0] == 0x1c || source.matrix_kinds[0] == 1) &&
                source.matrix_kinds[1] == 0 && source.matrix_kinds[2] == 0);
            require(source.render_type == RenderType::b || source.render_type == RenderType::a_fogged ||
                source.render_type == RenderType::b_fogged || source.render_type == RenderType::i_fogged ||
                source.render_type == RenderType::d_fogged || source.render_type == RenderType::j_fogged);
            auto& recipes = resources[r].recipes;
            recipes.reserve(source.nodes.size());
            for (std::size_t n = 0; n < source.nodes.size(); ++n) {
                const auto& node = source.nodes[n];
                require(n == 0 ? node.parent == no_index : node.parent < n);
                const auto flags = node.depth_flags & 0xf000u;
                require(flags == 0 || flags == 0x8000u);
                recipes.push_back(flags ? Recipe::kind44 : Recipe::rts);
                for (auto material : node.materials) require(material < materials.size());
                for (const auto& payload : node.payloads)
                    require(payload.display_list_id < 4 && payload.draw_list >= 0x80000000u &&
                        payload.draw_list-0x80000000u < rdram_size && (!payload.pre_list ||
                        (payload.pre_list >= 0x80000000u && payload.pre_list-0x80000000u < rdram_size)));
            }
        }
        slots.reserve(plans.size());
        for (const auto plan : plans) {
            require(plan.capacity.nodes && plan.matrices % 64 == 0 && plan.materials % 8 == 0);
            require(plan.matrices <= rdram_size && plan.capacity.nodes <= (rdram_size-plan.matrices)/64 &&
                plan.materials <= rdram_size && plan.capacity.material_attachments <= (rdram_size-plan.materials)/264,
                Error::extent);
            const RamRange matrix{plan.matrices,plan.capacity.nodes*64};
            const RamRange material{plan.materials,plan.capacity.material_attachments*264};
            auto overlap = [](RamRange a, RamRange b) {
                return a.size && b.size && a.offset < b.offset+b.size && b.offset < a.offset+a.size;
            };
            require(!overlap(matrix,material) && !overlap(matrix,placements) &&
                !overlap(material,placements),Error::reservation);
            for (const auto& previous : slots) {
                const RamRange pm{previous.plan.matrices,previous.plan.capacity.nodes*64};
                const RamRange pt{previous.plan.materials,previous.plan.capacity.material_attachments*264};
                require(!overlap(matrix,pm) && !overlap(matrix,pt) && !overlap(material,pm) &&
                    !overlap(material,pt),Error::reservation);
            }
            slots.emplace_back(plan);
        }
    }
    bool baked(const Slot& s, std::size_t n) const {
        return resources[s.staged.state.resource_model].recipes[n] == Recipe::kind44;
    }
};
Status BorrowedKernel::validate_upload(std::span<std::uint32_t> words, std::uint64_t epoch) const noexcept {
    try {
        const auto& p = *impl_;
        require(p.staged && words.data() && words.size_bytes() == rdram_size && epoch,Error::extent);
        const auto begin = reinterpret_cast<std::uintptr_t>(words.data());
        require(begin % alignof(std::uint32_t) == 0 &&
            begin <= std::numeric_limits<std::uintptr_t>::max()-rdram_size,Error::extent);
        const auto end = begin+rdram_size;
        auto disjoint = [&](const void* data, std::size_t bytes) {
            if (!bytes) return;
            const auto at = reinterpret_cast<std::uintptr_t>(data);
            require(data && at <= std::numeric_limits<std::uintptr_t>::max()-bytes,Error::extent);
            require(end <= at || at+bytes <= begin);
        };
        disjoint(this,sizeof(*this)); disjoint(&p,sizeof(p)); disjoint(p.game,1);
        disjoint(p.models.data(),p.models.size_bytes()); disjoint(p.materials.data(),p.materials.size_bytes());
        disjoint(p.slots.data(),p.slots.size()*sizeof(Impl::Slot));
        disjoint(p.resources.data(),p.resources.size()*sizeof(Impl::Resource));
        for (const auto& resource : p.resources)
            disjoint(resource.recipes.data(),resource.recipes.size()*sizeof(Impl::Recipe));
        for (const auto& source : p.models) {
            disjoint(source.tree.bytes.data(),source.tree.bytes.size());
            disjoint(source.nodes.data(),source.nodes.size()*sizeof(Node));
            for (const auto& node : source.nodes) {
                disjoint(node.wire.bytes.data(),node.wire.bytes.size());
                disjoint(node.materials.data(),node.materials.size()*sizeof(std::size_t));
                disjoint(node.payloads.data(),node.payloads.size()*sizeof(Payload));
            }
        }
        for (const auto& source : p.materials) {
            disjoint(source.wire.bytes.data(),source.wire.bytes.size());
            disjoint(source.images.data(),source.images.size()*sizeof(std::uint32_t));
            disjoint(source.palettes.data(),source.palettes.size()*sizeof(std::uint32_t));
        }
        for (const auto& s : p.slots) {
            disjoint(s.first.data(),s.first.size()*sizeof(std::uint32_t));
            disjoint(s.next.data(),s.next.size()*sizeof(std::uint32_t));
            disjoint(s.bases.data(),s.bases.size()*sizeof(std::uint32_t));
            disjoint(s.flags.data(),s.flags.size()*sizeof(std::uint32_t));
            disjoint(s.poses.data(),s.poses.size()*sizeof(std::uint64_t));
            disjoint(s.matrices.data(),s.matrices.size()*sizeof(std::uint64_t));
            disjoint(s.materials.data(),s.materials.size()*sizeof(std::uint64_t));
            disjoint(s.material_resources.data(),s.material_resources.size()*sizeof(std::size_t));
            disjoint(s.encoded_revisions.data(),s.encoded_revisions.size()*sizeof(std::uint64_t));
            disjoint(s.encoded_resources.data(),s.encoded_resources.size()*sizeof(std::size_t));
            disjoint(s.transforms.data(),s.transforms.size()*sizeof(IsleGameModelTransform));
            disjoint(s.world.data(),s.world.size()*sizeof(WorldMatrix));
            disjoint(s.staged_world.data(),s.staged_world.size()*sizeof(WorldMatrix));
            disjoint(s.changed.data(),s.changed.size()); disjoint(s.encoded.data(),s.encoded.size()*sizeof(MaterialCommands));
            if (s.present) {
                disjoint(s.staged.runtime,sizeof(*s.staged.runtime));
                disjoint(s.staged.state.nodes.data(),s.staged.state.nodes.size_bytes());
                disjoint(s.staged.state.materials.data(),s.staged.state.materials.size_bytes());
                disjoint(s.nodes.data(),s.nodes.size_bytes()); disjoint(s.placement,sizeof(*s.placement));
            }
        }
        return {};
    } catch (const Failure& f) { return f.status; }
      catch (...) { return {Error::invalid_input}; }
}
BorrowedKernel::BorrowedKernel(IsleGame* g, std::span<const Model> models, std::span<const Material> materials,
    std::span<const KernelSlot> plans, RamRange placements) {
    try { impl_ = std::make_unique<Impl>(g,models,materials,plans,placements); }
    catch (const Failure&) { throw std::invalid_argument("Borrowed model kernel rejected resource/capacity plan"); }
}
BorrowedKernel::~BorrowedKernel() = default;
Status BorrowedKernel::stage(std::span<const BorrowedBinding> bindings, std::size_t placement_prefix_bytes) noexcept {
    try {
        auto& p = *impl_; p.staged = false; p.dirty = false;
        for (auto& s : p.slots) s.present = false;
        require(placement_prefix_bytes % 64 == 0 && placement_prefix_bytes <= p.placements.size,Error::extent);
        std::size_t placement_cursor = p.placements.offset+placement_prefix_bytes;
        for (std::size_t i = 0; i < bindings.size(); ++i) {
            auto b = bindings[i];
            require(b.runtime != nullptr);
            b.state = b.runtime->model(b.state.instance);
            require(b.slot < p.slots.size() && b.object && b.model && bool(b.state) && bool(b.state.instance));
            require(b.pose == PoseSource::animation || b.pose == PoseSource::native);
            for (std::size_t j = 0; j < i; ++j) require(bindings[j].slot != b.slot &&
                bindings[j].state.instance != b.state.instance && bindings[j].object != b.object && bindings[j].model != b.model);
            auto& s = p.slots[b.slot]; s.staged = b; s.present = true;
            require(b.state.resource_model < p.models.size());
            const auto& source = p.models[b.state.resource_model];
            require(b.root == (source.matrix_kinds[0] == 1 ? RootRecipe::translate : RootRecipe::source));
            const auto count = source.nodes.size();
            require(b.state.nodes.size() == count && count <= s.plan.capacity.nodes &&
                b.state.materials.size() <= s.plan.capacity.material_attachments,Error::extent);
            const IsleGameModelNode* nodes = nullptr; std::size_t native_count = 0;
            native(isle_game_model_peek_nodes(p.game,b.model,&nodes,&native_count));
            require(native_count == count,Error::native); s.nodes = {nodes,native_count};
            native(isle_game_model_placement(p.game,b.model,&s.placement));
            IsleGameObjectMetadata metadata{}; native(isle_game_object_metadata(p.game,b.object,&metadata));
            s.draw = b.draw && !(metadata.flags & 1u) && !(b.state.object_flags & 1u);
            s.identity_changed = !s.live || s.key != s.staged_key();
            p.dirty |= s.identity_changed || s.draw != s.live_draw;
            const float placement_x = s.placement->mode == ISLE_GAME_MODEL_PLACEMENT_RTS ? s.placement->transform.scale.x : 1;
            p.dirty |= s.placement_mode != s.placement->mode || s.placement_x != placement_x;
            const bool placement_changed = s.identity_changed || s.placement_revision != s.placement->revision;
            s.placement_address = 0;
            if (s.placement->mode != ISLE_GAME_MODEL_PLACEMENT_DISABLED) {
                require(placement_cursor <= p.placements.offset+p.placements.size &&
                    64 <= p.placements.offset+p.placements.size-placement_cursor,Error::extent);
                s.placement_address = static_cast<std::uint32_t>(placement_cursor); placement_cursor += 64;
            }
            p.dirty |= s.published_placement_address != s.placement_address;
            if (s.topology_resource != b.state.resource_model) {
                std::fill_n(s.first.begin(),count,UINT32_MAX); std::fill_n(s.next.begin(),count,UINT32_MAX);
                std::size_t cursor = s.plan.materials;
                for (std::size_t n = 0; n < count; ++n) {
                    const auto attachments = source.nodes[n].materials.size();
                    s.bases[n] = attachments ? static_cast<std::uint32_t>(cursor) : 0;
                    cursor += attachments*264;
                }
                for (std::size_t n = count; n-- > 1;) {
                    const auto parent = source.nodes[n].parent;
                    s.next[n] = s.first[parent]; s.first[parent] = static_cast<std::uint32_t>(n);
                }
                s.topology_resource = b.state.resource_model;
            }
            std::size_t attachment = 0;
            for (std::size_t n = 0; n < count; ++n) {
                const auto& pose = b.state.nodes[n]; const auto& node = source.nodes[n];
                require(pose.parent == node.parent && pose.first_material == attachment && pose.material_count == node.materials.size());
                require(attachment <= b.state.materials.size() && pose.material_count <= b.state.materials.size()-attachment);
                require(nodes[n].parent == (pose.parent == no_index ? UINT32_MAX : pose.parent),Error::native);
                const bool translate_root = n == 0 && b.root == RootRecipe::translate;
                require(!translate_root || (b.pose == PoseSource::native && !p.baked(s,n)));
                const auto kind = p.baked(s,n) || translate_root ? ISLE_GAME_MODEL_TRANSLATE : ISLE_GAME_MODEL_ROTATE_RPY_TRANSLATE_SCALE;
                require(nodes[n].matrix_kind == kind,Error::native);
                attachment += pose.material_count;
                const auto t = b.pose == PoseSource::native ? nodes[n].transform : transform(pose.transform,p.baked(s,n));
                const auto flags = b.pose == PoseSource::native ? nodes[n].flags : pose.flags;
                const bool local_changed = s.identity_changed || (b.pose == PoseSource::animation ?
                    s.poses[n] != pose.revision : !same(s.transforms[n],t) ||
                    (p.baked(s,n) && s.poses[n] != pose.revision));
                if (local_changed) native(isle_game_model_validate_transform(p.game,b.model,n,&t));
                const bool parent_changed = pose.parent != no_index && s.changed[pose.parent];
                s.changed[n] = local_changed || parent_changed || (n == 0 && placement_changed);
                p.dirty |= s.flags[n] != flags || (p.baked(s,n) && s.changed[n]);
                if (!s.changed[n]) continue;
                // Kind44's native local is translation-only; the float attachment
                // view deliberately retains its source RTS, matching callbacks.
                auto matrix = local(b.pose == PoseSource::animation || p.baked(s,n) ? transform(pose.transform,false) : t);
                if (pose.parent == no_index && s.placement->mode != ISLE_GAME_MODEL_PLACEMENT_DISABLED) {
                    WorldMatrix placement;
                    const auto& placement_transform = s.placement->transform;
                    if (s.placement->mode == ISLE_GAME_MODEL_PLACEMENT_RT)
                        hal_rotate_rpy_translate_f(placement.m,placement_transform.position.x,placement_transform.position.y,placement_transform.position.z,
                            placement_transform.rotation.x,placement_transform.rotation.y,placement_transform.rotation.z);
                    else placement = local(placement_transform);
                    finite(placement); matrix = compose(matrix,placement);
                } else if (pose.parent != no_index) matrix = compose(matrix,
                    parent_changed ? s.staged_world[pose.parent] : s.world[pose.parent]);
                s.staged_world[n] = matrix;
            }
            require(attachment == b.state.materials.size());
            if (s.encoded_key != s.staged_key()) s.encoded_valid = false;
            for (std::size_t m = 0; m < attachment; ++m) {
                const auto& state = b.state.materials[m]; require(state.resource < p.materials.size());
                if (!s.encoded_valid || s.encoded_revisions[m] != state.revision ||
                    s.encoded_resources[m] != state.resource) {
                    const auto status = encode_material(p.materials[state.resource],state,s.encoded[m]);
                    if (!status) throw Failure{status};
                    s.encoded_revisions[m] = state.revision; s.encoded_resources[m] = state.resource;
                }
            }
            s.encoded_key = s.staged_key(); s.encoded_valid = true;
        }
        for (const auto& s : p.slots) if (!s.present && s.live_draw) p.dirty = true;
        p.staged = true; return {};
    } catch (const Failure& f) { return f.status; }
      catch (...) { return {Error::invalid_input}; }
}
Status BorrowedKernel::encode(std::size_t slot, const WorldMatrix& projection, HierarchyBuffer& buffer) const noexcept {
    auto& p = *impl_;
    if (!p.staged || slot >= p.slots.size() || !p.slots[slot].present) {
        p.staged = false; return {Error::invalid_input};
    }
    const auto& s = p.slots[slot]; if (!s.draw) return {};
    const auto count = s.staged.state.nodes.size();
    const auto status = encode_hierarchy({p.models[s.staged.state.resource_model],s.staged.state.nodes,
        std::span(s.first).first(count),std::span(s.next).first(count),std::span(s.bases).first(count),
        *s.placement,projection,s.plan.matrices,s.placement_address,
        s.staged.pose == PoseSource::native ? s.nodes : std::span<const IsleGameModelNode>{}},buffer);
    if (!status) p.staged = false;
    return status;
}
bool BorrowedKernel::lists_changed() const noexcept { return impl_->dirty; }
bool BorrowedKernel::draws(std::size_t slot) const noexcept {
    return impl_->staged && slot < impl_->slots.size() && impl_->slots[slot].present && impl_->slots[slot].draw;
}
void BorrowedKernel::commit_pose() noexcept {
    auto& p = *impl_; if (!p.staged) return;
    for (auto& s : p.slots) if (s.present) {
        const auto& b = s.staged;
        for (std::size_t n = 0; n < b.state.nodes.size(); ++n) {
            const auto& pose = b.state.nodes[n];
            if (b.pose == PoseSource::animation) {
                const auto t = transform(pose.transform,p.baked(s,n));
                static_cast<void>(isle_game_model_set_transform(p.game,b.model,n,&t));
                static_cast<void>(isle_game_model_set_flags(p.game,b.model,n,pose.flags));
            }
            if (s.changed[n]) s.world[n] = s.staged_world[n];
            s.poses[n] = pose.revision;
        }
        const IsleGameModelNode* nodes = nullptr; std::size_t count = 0;
        static_cast<void>(isle_game_model_nodes(p.game,b.model,&nodes,&count));
        s.nodes = {nodes,count}; s.published_nodes = s.nodes;
        for (std::size_t n = 0; n < count; ++n) { s.transforms[n] = nodes[n].transform; s.flags[n] = nodes[n].flags; }
        s.live = true; s.live_draw = s.draw; s.live_count = count; s.key = s.staged_key();
        s.placement_revision = s.placement->revision; s.placement_mode = s.placement->mode;
        s.placement_x = s.placement_mode == ISLE_GAME_MODEL_PLACEMENT_RTS ? s.placement->transform.scale.x : 1;
        if (s.published_placement_address != s.placement_address) s.resident_placement_address = 0;
        s.published_placement_address = s.placement_address;
        s.owner = b.runtime;
    }
    for (auto& s : p.slots) if (!s.present) {
        s.live = false; s.live_draw = false; s.resident = false; s.published_nodes = {}; s.live_count = 0;
    }
}
void BorrowedKernel::upload(std::span<std::uint32_t> words, std::uint64_t epoch) noexcept {
    auto& p = *impl_; if (!p.staged) return;
    graphics::Memory memory(words);
    for (auto& s : p.slots) if (s.present && s.draw) {
        const auto& state = s.staged.state;
        const bool fresh = !s.resident || s.resident_key != s.staged_key() || s.ram != words.data() || s.epoch != epoch;
        if (s.placement_address && (fresh || s.resident_placement_address != s.placement_address ||
            s.resident_placement_revision != s.placement->revision)) memory.matrix(s.placement_address,s.placement->fixed);
        for (std::size_t n = 0; n < s.nodes.size(); ++n) {
            if (fresh || s.matrices[n] != s.nodes[n].revision) memory.matrix(s.plan.matrices+static_cast<std::uint32_t>(n*64),s.nodes[n].fixed);
            s.matrices[n] = s.nodes[n].revision;
            const auto& pose = state.nodes[n];
            for (std::size_t j = 0; j < pose.material_count; ++j) {
                const auto m = pose.first_material+j; const auto& material = state.materials[m];
                const auto branch = s.bases[n]+static_cast<std::uint32_t>(j*8);
                const auto body = s.bases[n]+static_cast<std::uint32_t>(pose.material_count*8+j*256);
                if (fresh) { memory.word(branch,0xde010000); memory.word(branch+4,body); }
                if (fresh || s.materials[m] != material.revision || s.material_resources[m] != material.resource) {
                    for (std::size_t c = 0; c < s.encoded[m].count; ++c) {
                        memory.word(body+static_cast<std::uint32_t>(c*8),s.encoded[m].words[c].w0);
                        memory.word(body+static_cast<std::uint32_t>(c*8+4),s.encoded[m].words[c].w1);
                    }
                    s.materials[m] = material.revision; s.material_resources[m] = material.resource;
                }
            }
        }
        s.resident = true; s.resident_key = s.staged_key(); s.ram = words.data(); s.epoch = epoch;
        s.resident_placement_revision = s.placement->revision; s.resident_placement_address = s.placement_address;
    }
}
std::span<const WorldMatrix> BorrowedKernel::world_nodes(animation::InstanceHandle handle) const noexcept {
    for (const auto& s : impl_->slots)
        if (s.live && s.key.instance == handle && s.owner && s.owner->model(handle))
            return std::span(s.world).first(s.live_count);
    return {};
}
std::span<const IsleGameModelNode> BorrowedKernel::native_nodes(animation::InstanceHandle handle) const noexcept {
    for (const auto& s : impl_->slots)
        if (s.live && s.key.instance == handle && s.owner && s.owner->model(handle)) return s.published_nodes;
    return {};
}
} // namespace isle::menu::model
