#include "menu_model_producer.hpp"
#include "graphics_task.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

// Existing privately admitted, unchanged GAME math seam (not an SDK body).
extern "C" void hal_perspective_fast_f(float [4][4], std::uint16_t*, float, float, float, float, float);

namespace isle::menu::model {
namespace {
struct Failure { Status status; };
void require(bool ok, Error error = Error::invalid_input, std::uint32_t address = 0) {
    if (!ok) throw Failure{{error, address}};
}
void native(IsleGameResult result) { require(result == ISLE_GAME_OK, Error::native); }
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
}


struct Producer::Impl {
    IsleGame* game;
    IsleGameCamera* camera;
    const Catalog& catalog;
    const animation::Runtime& animation;
    Layout layout;
    std::array<RamRange,3> owned;
    std::vector<Binding> bindings;
    std::vector<BorrowedBinding> staged;
    std::unique_ptr<BorrowedKernel> kernel;
    std::array<std::vector<Command>,4> lanes;
    std::array<std::size_t,4> counts{};
    std::vector<Command> pending;
    std::array<ListRange,4> ranges{};
    std::array<std::uint64_t,2> camera_revisions{};
    std::array<const IsleGameCameraMatrix*,2> camera_matrices{};
    std::array<Command,4> camera_commands{};
    std::uint32_t* resident_memory = nullptr;
    std::uint64_t resident_epoch = 0;
    bool retired = false, have_lists = false, lists_dirty = true, have_projection = false;
    WorldMatrix projection;
    IsleGamePerspective list_perspective{}, projection_perspective{};
    std::vector<rom::GuestView> source_spans;
    Impl(IsleGame* g, IsleGameCamera* c, const Catalog& cat, const animation::Runtime& a, Layout l)
        : game(g),camera(c),catalog(cat),animation(a),layout(l),owned{l.payloads,l.lists,l.materials} {}
    void command(unsigned lane, std::uint32_t a, std::uint32_t b) {
        require(lane < 4 && counts[lane] < lanes[lane].size(),Error::extent);
        lanes[lane][counts[lane]++] = {a,b};
    }
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
    void stage() {
        require(!retired,Error::retired);
        static_cast<void>(catalog.models());
        for (std::size_t i = 0; i < bindings.size(); ++i) {
            const auto& b = bindings[i];
            staged[i] = {i,animation.model(b.instance),b.object,b.model};
            staged[i].runtime = &animation;
        }
        const auto status = kernel->stage(staged);
        if (!status) throw Failure{status};
        lists_dirty |= kernel->lists_changed();
    }
    void build_lists() {
        counts.fill(0);
        for (std::size_t i = 0; i < bindings.size(); ++i) {
            if (!kernel->draws(i)) continue;
            const auto& source = catalog.models()[staged[i].state.resource_model];
            const bool deferred = source.render_type == RenderType::d_fogged ||
                source.render_type == RenderType::j_fogged;
            fog(0,source,true); if (deferred) fog(1,source,true);
            HierarchyBuffer buffer{{std::span(lanes[0]),std::span(lanes[1]),
                std::span(lanes[2]),std::span(lanes[3])},counts,pending};
            const auto result = kernel->encode(i,projection,buffer);
            if (!result) throw Failure{result};
            fog(0,source,false); if (deferred) fog(1,source,false);
        }
        std::size_t cursor = layout.lists.offset+32;
        for (unsigned lane = 0; lane < 4; ++lane) {
            command(lane,0xdf000000,0);
            require(cursor <= layout.lists.offset+layout.lists.size &&
                counts[lane]*8 <= layout.lists.offset+layout.lists.size-cursor,Error::extent);
            ranges[lane] = {address(cursor),static_cast<std::uint32_t>(counts[lane])};
            cursor += counts[lane]*8;
        }
    }
    std::size_t find(animation::InstanceHandle handle) const noexcept {
        if (retired || !animation.model(handle)) return no_index;
        for (std::size_t i = 0; i < bindings.size(); ++i)
            if (bindings[i].instance == handle) return i;
        return no_index;
    }
};

Status attach_title_model(IsleGame* game, IsleGameObject* object, const Model& source,
    animation::ModelState state, IsleGameModel** output) noexcept {
    if (output) *output = nullptr;
    try {
        require(game && object && output && bool(state) && state.nodes.size() == source.nodes.size() &&
            !source.nodes.empty());
        require(source.matrix_kinds == std::array<std::uint8_t,3>{0x1c,0,0});
        std::vector<IsleGameModelNodeInit> initial(source.nodes.size());
        for (std::size_t n = 0; n < source.nodes.size(); ++n) {
            const auto& node = source.nodes[n];
            const auto flags = node.depth_flags & 0xf000u;
            require(flags == 0 || flags == 0x8000u);
            require(n == 0 ? node.parent == no_index : node.parent < n);
            const bool baked = flags != 0;
            initial[n] = {node.parent == no_index ? UINT32_MAX : static_cast<std::uint32_t>(node.parent),
                transform(state.nodes[n].transform,baked),baked ? ISLE_GAME_MODEL_TRANSLATE :
                ISLE_GAME_MODEL_ROTATE_RPY_TRANSLATE_SCALE};
        }
        native(isle_game_attach_model(game,object,initial.data(),initial.size(),output));
        return {};
    } catch (const Failure& f) { return f.status; }
      catch (const std::bad_alloc&) { return {Error::allocation}; }
      catch (...) { return {Error::invalid_input}; }
}
Producer::Producer(std::unique_ptr<Impl> impl) noexcept : impl_(std::move(impl)) {}
Producer::~Producer() = default;
CreateResult Producer::create(IsleGame* game, IsleGameCamera* camera, const Catalog& catalog,
    const animation::Runtime& animation, std::span<const Binding> bindings,
    Layout layout, std::span<const RamRange> other) noexcept {
    try {
        require(game && camera && bindings.size() == catalog.models().size() && bindings.size() == 5);
        auto p = std::make_unique<Impl>(game,camera,catalog,animation,layout);
        for (std::size_t i = 0; i < p->owned.size(); ++i) {
            range(p->owned[i],i == 0 ? 64 : 8);
            require(!overlaps(p->owned[i],{0,graphics::bss_end}),Error::reservation);
            require(!overlaps(p->owned[i],{0x3b5000,0x25800}) &&
                !overlaps(p->owned[i],{0x3da800,0x25800}),Error::reservation);
            for (std::size_t j = 0; j < i; ++j) require(!overlaps(p->owned[i],p->owned[j]),Error::reservation);
            for (const auto r : other) { range(r,1); require(!overlaps(p->owned[i],r),Error::reservation); }
        }
        require(layout.payloads.size >= 128 && layout.lists.size >= 64,Error::extent);
        std::vector<RamRange> combined(p->owned.begin(),p->owned.end());
        combined.insert(combined.end(),other.begin(),other.end());
        const auto plan = catalog.plan_install(Scene::title_menu,combined);
        require(bool(plan.status),plan.status.error == menu::Error::relocation_required ?
            Error::relocation_required : Error::reservation,plan.status.address);
        p->source_spans = plan.spans;
        p->bindings.assign(bindings.begin(),bindings.end()); p->staged.resize(bindings.size());
        std::vector<KernelSlot> slots; slots.reserve(bindings.size());
        std::size_t matrix = layout.payloads.offset+128, material = layout.materials.offset, nodes = 0;
        for (std::size_t i = 0; i < bindings.size(); ++i) {
            const auto state = animation.model(bindings[i].instance); require(bool(state));
            require(state.resource_model == i);
            const auto capacity = animation::slot_capacity(catalog.models()[state.resource_model]);
            slots.push_back({capacity,address(matrix),address(material)});
            matrix += capacity.nodes*64; material += capacity.material_attachments*264; nodes += capacity.nodes;
        }
        require(nodes == 292);
        require(matrix <= layout.payloads.offset+layout.payloads.size &&
            material <= layout.materials.offset+layout.materials.size,Error::extent);
        p->kernel = std::make_unique<BorrowedKernel>(game,catalog.models(),catalog.materials(),slots,
            RamRange{matrix,layout.payloads.offset+layout.payloads.size-matrix});
        for (auto& lane : p->lanes) lane.resize(layout.lists.size/8);
        p->pending.resize(nodes*14+bindings.size());
        p->stage();
        std::size_t camera_count = 0; native(isle_game_camera_matrix_count(game,camera,&camera_count));
        require(camera_count == 2,Error::native);
        for (unsigned i = 0; i < 2; ++i) native(isle_game_camera_matrix(game,camera,i,&p->camera_matrices[i]));
        require(p->camera_matrices[0]->kind == ISLE_GAME_CAMERA_PERSPECTIVE_FAST &&
            p->camera_matrices[1]->kind == ISLE_GAME_CAMERA_LOOK_AT_ROLL,Error::native);
        return {{},std::unique_ptr<Producer>(new Producer(std::move(p)))};
    } catch (const Failure& f) { return {f.status,{}}; }
      catch (const std::bad_alloc&) { return {{Error::allocation},{}}; }
      catch (...) { return {{Error::invalid_input},{}}; }
}
Status Producer::synchronize() noexcept {
    try { impl_->stage(); impl_->kernel->commit_pose(); return {}; }
    catch (const Failure& f) { return f.status; }
    catch (...) { return {Error::invalid_input}; }
}
PrepareResult Producer::prepare(std::span<std::uint32_t> words, std::uint64_t epoch) noexcept {
    try {
        auto& p = *impl_; require(!p.retired,Error::retired);
        require(words.data() && words.size_bytes() == rdram_size && epoch,Error::extent);
        const auto begin = reinterpret_cast<std::uintptr_t>(words.data());
        require(begin <= std::numeric_limits<std::uintptr_t>::max()-rdram_size,Error::extent);
        for (const auto& span : p.source_spans) {
            const auto source = reinterpret_cast<std::uintptr_t>(span.bytes.data());
            require(begin+rdram_size <= source || source+span.bytes.size() <= begin);
        }
        if (p.resident_memory != words.data() || p.resident_epoch != epoch) {
            auto disjoint = [&](const void* data, std::size_t bytes) {
                if (!bytes) return;
                const auto at = reinterpret_cast<std::uintptr_t>(data);
                require(data && at <= std::numeric_limits<std::uintptr_t>::max()-bytes,Error::extent);
                require(begin+rdram_size <= at || at+bytes <= begin);
            };
            disjoint(this,sizeof(*this)); disjoint(&p,sizeof(p)); disjoint(p.camera,1);
            disjoint(&p.catalog,sizeof(p.catalog)); disjoint(&p.animation,sizeof(p.animation));
            disjoint(p.bindings.data(),p.bindings.size()*sizeof(Binding));
            disjoint(p.staged.data(),p.staged.size()*sizeof(BorrowedBinding));
            disjoint(p.pending.data(),p.pending.size()*sizeof(Command));
            for (const auto& lane : p.lanes) disjoint(lane.data(),lane.size()*sizeof(Command));
        }
        p.stage();
        const auto upload_status = p.kernel->validate_upload(words,epoch);
        if (!upload_status) throw Failure{upload_status};
        std::size_t camera_count = 0; native(isle_game_camera_matrix_count(p.game,p.camera,&camera_count));
        require(camera_count == 2,Error::native);
        const IsleGameCameraParameters* parameters = nullptr;
        native(isle_game_camera_parameters(p.game,p.camera,&parameters));
        const auto& perspective = parameters->perspective; const auto& old = p.list_perspective;
        const bool changed = !p.have_lists || perspective.fovy != old.fovy || perspective.aspect != old.aspect ||
            perspective.near_plane != old.near_plane || perspective.far_plane != old.far_plane || perspective.scale != old.scale;
        const bool rebuild = !p.have_lists || p.lists_dirty || changed;
        if (rebuild) {
            p.lists_dirty = true;
            const auto& cached = p.projection_perspective;
            if (!p.have_projection || perspective.fovy != cached.fovy || perspective.aspect != cached.aspect ||
                perspective.near_plane != cached.near_plane || perspective.far_plane != cached.far_plane ||
                perspective.scale != cached.scale) {
                p.have_projection = false;
                require(std::isfinite(perspective.near_plane+perspective.far_plane));
                std::uint16_t ignored = 0;
                hal_perspective_fast_f(p.projection.m,&ignored,perspective.fovy,perspective.aspect,
                    perspective.near_plane,perspective.far_plane,perspective.scale);
                finite(p.projection);
                p.projection_perspective = perspective; p.have_projection = true;
            }
            p.build_lists();
        }
        native(isle_game_camera_compute_all(p.game,p.camera));
        for (unsigned i = 0; i < 2; ++i) native(isle_game_camera_matrix(p.game,p.camera,i,&p.camera_matrices[i]));
        std::uint16_t normalization = 0;
        native(isle_game_camera_perspective_normalization(p.game,p.camera,&normalization));
        p.kernel->commit_pose();
        graphics::Memory memory(words);
        const bool fresh = p.resident_memory != words.data() || p.resident_epoch != epoch;
        for (unsigned i = 0; i < 2; ++i) if (fresh || p.camera_revisions[i] != p.camera_matrices[i]->revision) {
            memory.matrix(address(p.layout.payloads.offset+i*64),p.camera_matrices[i]->fixed);
            p.camera_revisions[i] = p.camera_matrices[i]->revision;
        }
        const std::array<Command,4> camera_commands{{
            {0xda380007,address(p.layout.payloads.offset)}, {0xdb0e0000,normalization},
            {0xda380005,address(p.layout.payloads.offset+64)}, {0xdf000000,0}}};
        for (std::size_t i = 0; i < camera_commands.size(); ++i) {
            const auto c = camera_commands[i];
            if (fresh || c.w0 != p.camera_commands[i].w0 || c.w1 != p.camera_commands[i].w1) {
                memory.word(address(p.layout.lists.offset+i*8),c.w0);
                memory.word(address(p.layout.lists.offset+i*8+4),c.w1);
            }
        }
        p.kernel->upload(words,epoch);
        if (fresh || rebuild) for (unsigned lane = 0; lane < 4; ++lane) for (std::size_t i = 0; i < p.counts[lane]; ++i) {
            const auto c = p.lanes[lane][i]; const auto at = p.ranges[lane].address+static_cast<std::uint32_t>(i*8);
            if (fresh || words[at/4] != c.w0) memory.word(at,c.w0);
            if (fresh || words[at/4+1] != c.w1) memory.word(at+4,c.w1);
        }
        p.camera_commands = camera_commands; p.list_perspective = perspective;
        p.have_lists = true; p.lists_dirty = false;
        p.resident_memory = words.data(); p.resident_epoch = epoch;
        return {{},{{address(p.layout.lists.offset),4},p.ranges,p.owned,p.animation.generation()}};
    } catch (const Failure& f) { return {f.status,{}}; }
      catch (...) { return {{Error::invalid_input},{}}; }
}
std::span<const WorldMatrix> Producer::world_nodes(animation::InstanceHandle handle) const noexcept {
    const auto slot = impl_->find(handle); return slot == no_index ? std::span<const WorldMatrix>{} : impl_->kernel->world_nodes(handle);
}
std::span<const IsleGameModelNode> Producer::native_nodes(animation::InstanceHandle handle) const noexcept {
    const auto slot = impl_->find(handle); return slot == no_index ? std::span<const IsleGameModelNode>{} : impl_->kernel->native_nodes(handle);
}
IsleGameModel* Producer::native_model(animation::InstanceHandle handle) const noexcept {
    const auto slot = impl_->find(handle); return slot == no_index ? nullptr : impl_->bindings[slot].model;
}
std::span<const RamRange> Producer::reservations() const noexcept { return impl_->owned; }
void Producer::retire() noexcept { impl_->retired = true; }
} // namespace isle::menu::model
