#include "menu_model_producer.hpp"
#include "graphics_task.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

// Existing privately admitted, unchanged GAME math seam (not an SDK body).
extern "C" void hal_rotate_rpy_translate_scale_f(float [4][4], float, float, float,
    float, float, float, float, float, float);
extern "C" void hal_perspective_fast_f(float [4][4], std::uint16_t*, float, float, float, float, float);
extern "C" void hal_mtx_f2l(float [4][4], IsleGameFixedMatrix*);

namespace isle::menu::model {
namespace {
struct Failure { Status status; };
void require(bool ok, Error error = Error::invalid_input, std::uint32_t address = 0) {
    if (!ok) throw Failure{{error, address}};
}
void native(IsleGameResult result) { require(result == ISLE_GAME_OK, Error::native); }
std::int32_t integer(float f) {
    require(std::isfinite(f) && double(f) >= -2147483648.0 && double(f) < 2147483648.0);
    return static_cast<std::int32_t>(f);
}
bool overlaps(RamRange a, RamRange b) {
    return a.offset < b.offset + b.size && b.offset < a.offset + a.size;
}
void range(RamRange r, std::size_t alignment) {
    require(r.size && r.offset < rdram_size && r.size <= rdram_size - r.offset &&
        r.offset % alignment == 0 && r.size % alignment == 0, Error::extent);
}
std::uint32_t address(std::size_t value) {
    require(value < rdram_size, Error::extent);
    return static_cast<std::uint32_t>(value);
}
IsleGameVec3 vec(Vec3 v) { return {v.x,v.y,v.z}; }
IsleGameModelTransform transform(const animation::Transform& t, bool baked) {
    if (baked) return {vec(t.position), {0,0,0}, {1,1,1}};
    return {vec(t.position), vec(t.rotation), vec(t.scale)};
}
void finite(const WorldMatrix& m) {
    for (const auto& row : m.m) for (float f : row) require(std::isfinite(f));
}
WorldMatrix compose(const WorldMatrix& a, const WorldMatrix& b) {
    WorldMatrix out;
    for (unsigned r = 0; r < 4; ++r) for (unsigned c = 0; c < 4; ++c)
        out.m[r][c] = ((a.m[r][0] * b.m[0][c] + a.m[r][1] * b.m[1][c]) +
            a.m[r][2] * b.m[2][c]) + a.m[r][3] * b.m[3][c];
    finite(out);
    return out;
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

struct Producer::Impl {
    struct Instance {
        IsleGameModel* handle = nullptr;
        std::vector<std::uint32_t> first_child, next_sibling, material_bases;
        std::vector<std::uint64_t> pose_revisions, resident_revisions, material_revisions;
        std::vector<std::uint32_t> flags;
        std::vector<WorldMatrix> world, staged_world;
        std::vector<unsigned char> changed;
        std::vector<MaterialCommands> material_commands;
        std::span<const IsleGameModelNode> native_nodes;
        std::uint32_t matrices = 0;
        std::uint32_t object_flags = 0;
    };
    IsleGame* game;
    IsleGameCamera* camera;
    const Catalog& catalog;
    const animation::Runtime& animation;
    Layout layout;
    std::array<RamRange,3> owned;
    std::vector<Instance> instances;
    std::array<std::vector<Command>,4> lanes;
    std::array<std::size_t,4> counts{}, copied{};
    std::vector<Command> pending;
    std::size_t pending_count = 0;
    std::array<ListRange,4> ranges{};
    std::array<std::uint64_t,2> camera_revisions{};
    std::array<const IsleGameCameraMatrix*,2> camera_matrices{};
    std::array<Command,4> camera_commands{};
    std::uint32_t* resident_memory = nullptr;
    std::uint64_t resident_epoch = 0, synchronized_generation = 0;
    bool have_world = false, retired = false;
    WorldMatrix staged_projection;
    std::vector<rom::GuestView> source_spans;
    IsleGamePerspective list_perspective{};
    bool have_lists = false;
    bool lists_dirty = true;

    Impl(IsleGame* g, IsleGameCamera* c, const Catalog& cat,
         const animation::Runtime& a, Layout l)
        : game(g),camera(c),catalog(cat),animation(a),layout(l),owned{l.payloads,l.lists,l.materials} {}
    ~Impl() {
        for (auto& instance : instances)
            if (instance.handle) static_cast<void>(isle_game_remove_model(game,instance.handle));
    }
    bool baked(std::size_t model, std::size_t node) const {
        return (catalog.models()[model].nodes[node].depth_flags & 0xf000u) != 0;
    }
    void command(unsigned lane, std::uint32_t a, std::uint32_t b) {
        require(lane < 4 && counts[lane] < lanes[lane].size(), Error::extent);
        lanes[lane][counts[lane]++] = {a,b};
    }
    void call(unsigned lane, std::uint32_t target) { command(lane,0xde000000,target); }
    void fog(unsigned lane, const Model& model, bool enable) {
        if (lane > 1) return;
        const bool transparent = lane == 1;
        command(lane,0xe7000000,0);
        if (enable) {
            const auto difference = std::int32_t(model.fog_max) - model.fog_min;
            require(difference != 0);
            const auto multiplier = 128000 / difference;
            const auto offset = (500 - std::int32_t(model.fog_min)) * 256 / difference;
            command(lane,0xdb080000,(std::uint32_t(multiplier) << 16) | (std::uint32_t(offset) & 65535));
            command(lane,0xf8000000,(std::uint32_t(model.fog_rgb[0]) << 24) |
                (std::uint32_t(model.fog_rgb[1]) << 16) | (std::uint32_t(model.fog_rgb[2]) << 8));
        }
        command(lane,0xe3000a01,enable ? 0x00100000 : 0);
        command(lane,0xe200001c,enable ? (transparent ? 0xc81049d8 : 0xc8112078) :
            (transparent ? 0x005049d8 : 0x00552078));
        command(lane,enable ? 0xd9ffffff : 0xd9feffff,enable ? 0x10000 : 0);
    }
    void stage_pose() {
        require(!retired && animation.model_count() == instances.size(), Error::retired);
        static_cast<void>(catalog.models()); // Snapshot move/lifetime fence.
        for (std::size_t m = 0; m < instances.size(); ++m) {
            auto& instance = instances[m];
            const auto state = animation.model(m);
            const auto& source = catalog.models()[m];
            require(state.nodes.size() == source.nodes.size() &&
                state.materials.size() == instance.material_commands.size());
            if ((state.object_flags ^ instance.object_flags) & 1u) lists_dirty = true;
            for (std::size_t n = 0; n < state.nodes.size(); ++n) {
                const auto& pose = state.nodes[n];
                require(pose.parent == source.nodes[n].parent &&
                    pose.first_material <= state.materials.size() &&
                    pose.material_count <= state.materials.size() - pose.first_material &&
                    pose.material_count == source.nodes[n].materials.size());
                if (n == 0 || !have_world || instance.pose_revisions[n] != pose.revision) {
                    const auto local = transform(pose.transform,baked(m,n));
                    native(isle_game_model_validate_transform(game,instance.handle,n,&local));
                }
                const bool parent_changed = pose.parent != no_index && instance.changed[pose.parent];
                instance.changed[n] = !have_world || instance.pose_revisions[n] != pose.revision || parent_changed;
                if (pose.flags != instance.flags[n] || (baked(m,n) && instance.changed[n]))
                    lists_dirty = true;
                if (!instance.changed[n]) continue;
                WorldMatrix matrix;
                const auto& t = pose.transform;
                hal_rotate_rpy_translate_scale_f(matrix.m,t.position.x,t.position.y,t.position.z,
                    t.rotation.x,t.rotation.y,t.rotation.z,t.scale.x,t.scale.y,t.scale.z);
                finite(matrix);
                instance.staged_world[n] = pose.parent == no_index ? matrix :
                    compose(matrix,parent_changed ? instance.staged_world[pose.parent] : instance.world[pose.parent]);
            }
            for (std::size_t i = 0; i < state.materials.size(); ++i) {
                const auto& material = state.materials[i];
                require(material.resource < catalog.materials().size());
                if (instance.material_revisions[i] == material.revision) continue;
                const auto result = encode_material(catalog.materials()[material.resource],material,
                    instance.material_commands[i]);
                if (!result) throw Failure{result};
            }
        }
    }
    void publish_pose() {
        for (std::size_t m = 0; m < instances.size(); ++m) {
            auto& instance = instances[m];
            const auto state = animation.model(m);
            bool local_changed = false;
            for (std::size_t n = 0; n < state.nodes.size(); ++n) {
                const auto& pose = state.nodes[n];
                if (!have_world || instance.pose_revisions[n] != pose.revision) {
                    const auto t = transform(pose.transform,baked(m,n));
                    // All ownership/kind/scalar checks completed for ALL models.
                    static_cast<void>(isle_game_model_set_transform(game,instance.handle,n,&t));
                    static_cast<void>(isle_game_model_set_flags(game,instance.handle,n,pose.flags));
                    local_changed = true;
                }
                if (instance.changed[n]) instance.world[n] = instance.staged_world[n];
                instance.pose_revisions[n] = pose.revision;
                instance.flags[n] = pose.flags;
            }
            if (local_changed || instance.native_nodes.empty()) {
                const IsleGameModelNode* nodes = nullptr;
                std::size_t count = 0;
                static_cast<void>(isle_game_model_nodes(game,instance.handle,&nodes,&count));
                instance.native_nodes = {nodes,count};
            }
            instance.object_flags = state.object_flags;
        }
        have_world = true;
        synchronized_generation = animation.generation();
    }
    void append_matrix(std::size_t m, std::size_t n, bool push, float parent_scale) {
        auto& instance = instances[m];
        require(pending_count < pending.size(),Error::extent);
        pending[pending_count++] = {push ? 0xda380000u : 0xda380001u,
            instance.matrices + static_cast<std::uint32_t>(n * 64)};
        if (!baked(m,n)) return;
        const auto& pose = animation.model(m).nodes[n].transform;
        const auto& projection = staged_projection;
        WorldMatrix matrix;
        const auto x = parent_scale * pose.scale.x;
        const auto y = pose.scale.y * parent_scale;
        matrix.m[0][0] = projection.m[0][0] * x;
        matrix.m[1][1] = projection.m[1][1] * y;
        matrix.m[2][2] = projection.m[2][2] * x;
        matrix.m[2][3] = projection.m[2][3] * x;
        for (const auto& row : matrix.m) for (float f : row)
            require(std::isfinite(f) && double(f * 65536.0f) >= -2147483648.0 &&
                double(f * 65536.0f) < 2147483648.0);
        IsleGameFixedMatrix fixed;
        hal_mtx_f2l(matrix.m,&fixed);
        require(pending_count + 13 <= pending.size(),Error::extent);
        pending[pending_count++] = {0xd5000001,0};
        // Kind44 replaces the first three combined-MVP rows. Translation row
        // remains the freshly recalculated real modelview x projection result.
        for (unsigned row = 0; row < 3; ++row) for (unsigned pair = 0; pair < 2; ++pair)
            pending[pending_count++] = {0xdb000000u | (row * 8 + pair * 4),
                fixed.m[row / 2][(row % 2) * 2 + pair]};
        for (unsigned row = 0; row < 3; ++row) for (unsigned pair = 0; pair < 2; ++pair)
            pending[pending_count++] = {0xdb000020u | (row * 8 + pair * 4),
                fixed.m[2 + row / 2][(row % 2) * 2 + pair]};
    }
    void flush(unsigned lane) {
        while (copied[lane] < pending_count) {
            const auto c = pending[copied[lane]++]; command(lane,c.w0,c.w1);
        }
    }
    void node(std::size_t m, std::size_t n, float parent_scale, bool deferred) {
        const auto& source = catalog.models()[m];
        auto& instance = instances[m];
        const auto state = animation.model(m);
        const auto& pose = state.nodes[n];
        if (pose.flags & 2) return;
        const auto saved = pending_count;
        const bool push = pose.parent == no_index || instance.next_sibling[n] != UINT32_MAX;
        if (!deferred && !(pose.flags & 1))
            for (const auto& payload : source.nodes[n].payloads)
                if (payload.pre_list) call(0,payload.pre_list);
        append_matrix(m,n,push,parent_scale);
        if (!deferred) flush(0);
        if (!(pose.flags & 1)) for (const auto& payload : source.nodes[n].payloads) {
            const auto lane = payload.display_list_id;
            require(lane < 4);
            if (deferred && payload.pre_list) call(lane,payload.pre_list);
            if (deferred) flush(lane);
            if (pose.material_count) command(lane,0xdb060038,instance.material_bases[n]);
            call(lane,payload.draw_list);
        }
        const float scale = parent_scale * pose.transform.scale.x;
        require(std::isfinite(scale));
        for (auto child = instance.first_child[n]; child != UINT32_MAX; child = instance.next_sibling[child])
            node(m,child,scale,deferred);
        pending_count = saved;
        for (unsigned lane = 0; lane < 4; ++lane) if (copied[lane] > saved) {
            copied[lane] = saved;
            if (push) command(lane,0xd8380002,64);
        }
    }
    void build_lists() {
        counts.fill(0);
        for (std::size_t m = 0; m < instances.size(); ++m) {
            if (animation.model(m).object_flags & 1u) continue;
            const auto& source = catalog.models()[m];
            const bool deferred = source.render_type == RenderType::d_fogged ||
                source.render_type == RenderType::j_fogged;
            fog(0,source,true); if (deferred) fog(1,source,true);
            copied.fill(0); pending_count = 0;
            node(m,0,1.0f,deferred);
            fog(0,source,false); if (deferred) fog(1,source,false);
        }
        std::size_t cursor = layout.lists.offset + 32;
        for (unsigned lane = 0; lane < 4; ++lane) {
            command(lane,0xdf000000,0);
            require(cursor <= layout.lists.offset + layout.lists.size &&
                counts[lane] * 8 <= layout.lists.offset + layout.lists.size - cursor,Error::extent);
            ranges[lane] = {address(cursor),static_cast<std::uint32_t>(counts[lane])};
            cursor += counts[lane] * 8;
        }
    }
};

Producer::Producer(std::unique_ptr<Impl> impl) noexcept : impl_(std::move(impl)) {}
Producer::~Producer() = default;
CreateResult Producer::create(IsleGame* game, IsleGameCamera* camera, const Catalog& catalog,
    const animation::Runtime& animation, std::span<IsleGameObject* const> objects,
    Layout layout, std::span<const RamRange> other) noexcept {
    try {
        require(game && camera && objects.size() == catalog.models().size() &&
            objects.size() == 5 && animation.model_count() == objects.size());
        auto impl = std::make_unique<Impl>(game,camera,catalog,animation,layout);
        for (std::size_t i = 0; i < impl->owned.size(); ++i) {
            range(impl->owned[i],i == 0 ? 64 : 8);
            require(!overlaps(impl->owned[i],{0,graphics::bss_end}),Error::reservation);
            require(!overlaps(impl->owned[i],{0x3b5000,0x25800}) &&
                !overlaps(impl->owned[i],{0x3da800,0x25800}),Error::reservation);
            for (std::size_t j = 0; j < i; ++j)
                require(!overlaps(impl->owned[i],impl->owned[j]),Error::reservation);
            for (const auto r : other) { range(r,1); require(!overlaps(impl->owned[i],r),Error::reservation); }
        }
        require(layout.payloads.size >= 128 && layout.lists.size >= 64,Error::extent);
        std::vector<RamRange> combined(impl->owned.begin(),impl->owned.end());
        combined.insert(combined.end(),other.begin(),other.end());
        const auto plan = catalog.plan_install(Scene::title_menu,combined);
        require(static_cast<bool>(plan.status),plan.status.error == menu::Error::relocation_required ?
            Error::relocation_required : Error::reservation,plan.status.address);
        impl->source_spans = plan.spans;
        impl->instances.resize(objects.size());
        std::size_t matrix_cursor = layout.payloads.offset + 128;
        std::size_t material_cursor = layout.materials.offset;
        std::size_t node_total = 0;
        for (std::size_t m = 0; m < objects.size(); ++m) {
            require(objects[m]);
            for (std::size_t j = 0; j < m; ++j) require(objects[j] != objects[m]);
            auto& instance = impl->instances[m];
            const auto& source = catalog.models()[m];
            const auto state = animation.model(m);
            const auto count = source.nodes.size();
            require(count && state.nodes.size() == count);
            node_total += count;
            instance.matrices = address(matrix_cursor); matrix_cursor += count * 64;
            require(matrix_cursor <= layout.payloads.offset + layout.payloads.size,Error::extent);
            instance.first_child.assign(count,UINT32_MAX); instance.next_sibling.assign(count,UINT32_MAX);
            instance.material_bases.resize(count);
            instance.pose_revisions.resize(count); instance.resident_revisions.resize(count);
            instance.flags.resize(count); instance.world.resize(count); instance.staged_world.resize(count);
            instance.changed.resize(count); instance.material_revisions.resize(state.materials.size());
            instance.material_commands.resize(state.materials.size());
            std::vector<IsleGameModelNodeInit> initial(count);
            for (std::size_t n = 0; n < count; ++n) {
                const auto& node = source.nodes[n];
                require((node.depth_flags & 0xf000u) == 0 || (node.depth_flags & 0xf000u) == 0x8000u);
                require(n == 0 ? node.parent == no_index : node.parent < n);
                const bool baked = (node.depth_flags & 0xf000u) != 0;
                initial[n] = {node.parent == no_index ? UINT32_MAX : static_cast<std::uint32_t>(node.parent),
                    transform(state.nodes[n].transform,baked),baked ? ISLE_GAME_MODEL_TRANSLATE :
                    ISLE_GAME_MODEL_ROTATE_RPY_TRANSLATE_SCALE};
                if (!node.materials.empty()) {
                    instance.material_bases[n] = address(material_cursor);
                    material_cursor += node.materials.size() * (8 + 256);
                    require(material_cursor <= layout.materials.offset + layout.materials.size,Error::extent);
                }
                for (const auto& p : node.payloads) require(p.display_list_id < 4);
            }
            for (std::size_t n = count; n-- > 1;) {
                const auto parent = source.nodes[n].parent;
                instance.next_sibling[n] = instance.first_child[parent];
                instance.first_child[parent] = static_cast<std::uint32_t>(n);
            }
            native(isle_game_attach_model(game,objects[m],initial.data(),initial.size(),&instance.handle));
            const IsleGameModelNode* nodes = nullptr;
            std::size_t native_count = 0;
            native(isle_game_model_nodes(game,instance.handle,&nodes,&native_count));
            instance.native_nodes = {nodes,native_count};
        }
        require(node_total == 292);
        // Allocate maximum capacities once; prepare neither grows nor allocates.
        for (auto& lane : impl->lanes) lane.resize(layout.lists.size / 8);
        impl->pending.resize(node_total * 14);
        std::size_t camera_count = 0;
        native(isle_game_camera_matrix_count(game,camera,&camera_count));
        require(camera_count == 2,Error::native);
        native(isle_game_camera_compute_all(game,camera));
        for (unsigned i = 0; i < 2; ++i)
            native(isle_game_camera_matrix(game,camera,i,&impl->camera_matrices[i]));
        require(impl->camera_matrices[0]->kind == ISLE_GAME_CAMERA_PERSPECTIVE_FAST &&
            impl->camera_matrices[1]->kind == ISLE_GAME_CAMERA_LOOK_AT_ROLL,Error::native);
        auto producer = std::unique_ptr<Producer>(new Producer(std::move(impl)));
        return {{},std::move(producer)};
    } catch (const Failure& f) { return {f.status,{}}; }
      catch (const std::bad_alloc&) { return {{Error::allocation},{}}; }
      catch (...) { return {{Error::invalid_input},{}}; }
}

Status Producer::synchronize() noexcept {
    try {
        impl_->stage_pose();
        impl_->publish_pose();
        return {};
    } catch (const Failure& f) { return f.status; }
      catch (...) { return {Error::invalid_input}; }
}
PrepareResult Producer::prepare(std::span<std::uint32_t> words, std::uint64_t epoch) noexcept {
    try {
        auto& p = *impl_;
        require(!p.retired,Error::retired);
        require(words.data() && words.size() == rdram_size / sizeof(std::uint32_t) && epoch,Error::extent);
        const auto ram_begin = reinterpret_cast<std::uintptr_t>(words.data());
        require(ram_begin % alignof(std::uint32_t) == 0 &&
            ram_begin <= std::numeric_limits<std::uintptr_t>::max() - rdram_size,Error::extent);
        if (p.resident_memory != words.data() || p.resident_epoch != epoch) {
            const auto ram_end = ram_begin + rdram_size;
            auto disjoint = [&](const void* data, std::size_t size) {
                if (!size) return;
                const auto begin = reinterpret_cast<std::uintptr_t>(data);
                require(data && begin <= std::numeric_limits<std::uintptr_t>::max() - size,Error::extent);
                require(ram_end <= begin || begin + size <= ram_begin,Error::invalid_input);
            };
            disjoint(this,sizeof(*this)); disjoint(&p,sizeof(p));
            disjoint(p.game,1); disjoint(p.camera,1); disjoint(&p.catalog,sizeof(p.catalog));
            disjoint(&p.animation,sizeof(p.animation));
            for (const auto& wire : p.source_spans) disjoint(wire.bytes.data(),wire.bytes.size());
            for (std::size_t m = 0; m < p.instances.size(); ++m) {
                const auto& instance = p.instances[m];
                const auto state = p.animation.model(m);
                disjoint(state.nodes.data(),state.nodes.size_bytes());
                disjoint(state.materials.data(),state.materials.size_bytes());
                disjoint(instance.world.data(),instance.world.size() * sizeof(WorldMatrix));
                disjoint(instance.staged_world.data(),instance.staged_world.size() * sizeof(WorldMatrix));
                disjoint(instance.native_nodes.data(),instance.native_nodes.size_bytes());
                disjoint(instance.material_commands.data(),instance.material_commands.size() * sizeof(MaterialCommands));
            }
            for (const auto& lane : p.lanes) disjoint(lane.data(),lane.size() * sizeof(Command));
            disjoint(p.pending.data(),p.pending.size() * sizeof(Command));
        }
        p.stage_pose();
        std::size_t camera_count = 0;
        native(isle_game_camera_matrix_count(p.game,p.camera,&camera_count));
        require(camera_count == 2,Error::native);
        const IsleGameCameraParameters* parameters = nullptr;
        native(isle_game_camera_parameters(p.game,p.camera,&parameters));
        const auto& perspective = parameters->perspective;
        const auto& old = p.list_perspective;
        const bool projection_changed = !p.have_lists || perspective.fovy != old.fovy ||
            perspective.aspect != old.aspect || perspective.near_plane != old.near_plane ||
            perspective.far_plane != old.far_plane || perspective.scale != old.scale;
        const bool rebuild = !p.have_lists || p.lists_dirty || projection_changed;
        if (rebuild) {
            p.have_lists = false;
            if (projection_changed) {
                require(std::isfinite(perspective.near_plane + perspective.far_plane));
                std::uint16_t ignored = 0;
                hal_perspective_fast_f(p.staged_projection.m,&ignored,perspective.fovy,
                    perspective.aspect,perspective.near_plane,perspective.far_plane,perspective.scale);
                finite(p.staged_projection);
            }
            p.build_lists(); // Every bounded-list/fixed kind44 failure precedes native publication.
        }
        // All remaining native calls have validated ownership and immutable
        // kinds. compute_all publishes its camera batch only on success.
        native(isle_game_camera_compute_all(p.game,p.camera));
        for (unsigned i = 0; i < 2; ++i)
            native(isle_game_camera_matrix(p.game,p.camera,i,&p.camera_matrices[i]));
        std::uint16_t normalization = 0;
        native(isle_game_camera_perspective_normalization(p.game,p.camera,&normalization));
        if (rebuild) {
            p.list_perspective = perspective;
            p.have_lists = true;
            p.lists_dirty = false;
        }
        p.publish_pose();
        graphics::Memory memory(words);
        const bool fresh = p.resident_memory != words.data() || p.resident_epoch != epoch;
        for (unsigned i = 0; i < 2; ++i) if (fresh ||
            p.camera_revisions[i] != p.camera_matrices[i]->revision) {
            memory.matrix(address(p.layout.payloads.offset + i * 64),p.camera_matrices[i]->fixed);
            p.camera_revisions[i] = p.camera_matrices[i]->revision;
        }
        const std::array<Command,4> camera_commands{{
            {0xda380007,address(p.layout.payloads.offset)}, {0xdb0e0000,normalization},
            {0xda380005,address(p.layout.payloads.offset + 64)}, {0xdf000000,0}}};
        for (std::size_t i = 0; i < camera_commands.size(); ++i) {
            const auto c = camera_commands[i];
            if (fresh || c.w0 != p.camera_commands[i].w0 || c.w1 != p.camera_commands[i].w1) {
                memory.word(address(p.layout.lists.offset + i * 8),c.w0);
                memory.word(address(p.layout.lists.offset + i * 8 + 4),c.w1);
            }
        }
        p.camera_commands = camera_commands;
        for (std::size_t m = 0; m < p.instances.size(); ++m) {
            auto& instance = p.instances[m];
            const auto state = p.animation.model(m);
            for (std::size_t n = 0; n < instance.native_nodes.size(); ++n) {
                const auto& node = instance.native_nodes[n];
                if (fresh || instance.resident_revisions[n] != node.revision) {
                    memory.matrix(instance.matrices + static_cast<std::uint32_t>(n * 64),node.fixed);
                    instance.resident_revisions[n] = node.revision;
                }
                const auto& pose = state.nodes[n];
                for (std::size_t j = 0; j < pose.material_count; ++j) {
                    const auto index = pose.first_material + j;
                    const auto branch = instance.material_bases[n] + static_cast<std::uint32_t>(j * 8);
                    const auto body = instance.material_bases[n] + static_cast<std::uint32_t>(pose.material_count * 8 + j * 256);
                    if (fresh) { memory.word(branch,0xde010000); memory.word(branch + 4,body); }
                    if (!fresh && instance.material_revisions[index] == state.materials[index].revision) continue;
                    const auto& encoded = instance.material_commands[index];
                    for (std::size_t c = 0; c < encoded.count; ++c) {
                        memory.word(body + static_cast<std::uint32_t>(c * 8),encoded.words[c].w0);
                        memory.word(body + static_cast<std::uint32_t>(c * 8 + 4),encoded.words[c].w1);
                    }
                    instance.material_revisions[index] = state.materials[index].revision;
                }
            }
        }
        if (fresh || rebuild) for (unsigned lane = 0; lane < 4; ++lane)
            for (std::size_t i = 0; i < p.counts[lane]; ++i) {
                const auto c = p.lanes[lane][i];
                const auto at = p.ranges[lane].address + static_cast<std::uint32_t>(i * 8);
                if (fresh || words[at / 4] != c.w0) memory.word(at,c.w0);
                if (fresh || words[at / 4 + 1] != c.w1) memory.word(at + 4,c.w1);
            }
        p.resident_memory = words.data(); p.resident_epoch = epoch;
        return {{},{{address(p.layout.lists.offset),4},p.ranges,p.owned,p.synchronized_generation}};
    } catch (const Failure& f) { return {f.status,{}}; }
      catch (...) { return {{Error::invalid_input},{}}; }
}
std::span<const WorldMatrix> Producer::world_nodes(std::size_t model) const noexcept {
    if (impl_->retired || !impl_->have_world || model >= impl_->instances.size()) return {};
    return impl_->instances[model].world;
}
std::span<const IsleGameModelNode> Producer::native_nodes(std::size_t model) const noexcept {
    if (impl_->retired || !impl_->have_world || model >= impl_->instances.size()) return {};
    return impl_->instances[model].native_nodes;
}
std::span<const RamRange> Producer::reservations() const noexcept { return impl_->owned; }
void Producer::retire() noexcept { impl_->retired = true; }
} // namespace isle::menu::model
