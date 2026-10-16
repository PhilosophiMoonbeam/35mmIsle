#include "world_producer.hpp"
#include "borrowed_model_kernel.hpp"
#include "graphics_task.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>


namespace isle::world {
namespace {
using Command = menu::model::Command;
using Range = menu::RamRange;
void require(bool v, const char* message) { if (!v) throw std::runtime_error(message); }
void native(IsleGameResult r) { require(r == ISLE_GAME_OK,"World producer native operation failed"); }
bool overlap(Range a, Range b) { return a.offset < b.offset+b.size && b.offset < a.offset+a.size; }
void valid(Range r, std::size_t alignment) {
    require(r.size && r.offset%alignment == 0 && r.size%alignment == 0 && r.offset <= beach::rdram_size &&
            r.size <= beach::rdram_size-r.offset,"World producer range is invalid");
}
}
struct Producer::Impl {
    struct Instance {
        std::size_t render_index = 0, slot = no_index;
        std::uint32_t matrices = 0;
        menu::model::RootRecipe root = menu::model::RootRecipe::source;
    };
    IsleGame* game;
    IsleGameCamera* camera;
    menu::model::CameraBasisState& camera_basis;
    const Resources& resources;
    const World& world;
    ProducerLayout layout;
    std::size_t local_payload_end = 0;
    std::array<Range,3> owned;
    std::vector<Instance> instances;
    std::vector<menu::model::BorrowedBinding> bindings;
    std::unique_ptr<menu::model::BorrowedKernel> kernel;
    std::array<std::vector<Command>,4> lanes;
    std::vector<Command> pending;
    std::array<std::size_t,4> counts{};
    std::array<menu::model::ListRange,4> ranges{};
    std::array<Command,9> camera_commands{};
    std::array<std::uint64_t,2> camera_revisions{};
    std::uint64_t reflection_revision = 0, revision = 0, epoch = 0;
    const std::uint32_t* ram = nullptr;
    const IsleGameModelPlacement* sky_placement = nullptr;
    std::uint32_t sky_placement_address = 0, published_sky_placement_address = 0, resident_sky_placement_address = 0;
    std::uint64_t sky_placement_revision = 0, sky_revision = 0;
    IsleGameModel* sky_model = nullptr;
    IsleGameModel* resident_sky_model = nullptr;
    const IsleGameModelNode* sky_node = nullptr;
    bool sky_draw = false, sky_resident = false, retired = false;
    Impl(IsleGame* g, IsleGameCamera* c, menu::model::CameraBasisState& basis,
         const Resources& r, const World& w, ProducerLayout l)
        : game(g),camera(c),camera_basis(basis),resources(r),world(w),layout(l),
          owned{l.payloads,l.lists,l.materials} {}
    std::uint32_t payload(std::size_t o) const { return static_cast<std::uint32_t>(layout.payloads.offset+o); }
    void command(unsigned lane, std::uint32_t a, std::uint32_t b) {
        require(lane < 4 && counts[lane] < lanes[lane].size(),"World producer command capacity exceeded");
        lanes[lane][counts[lane]++] = {a,b};
    }
    void high(unsigned shift, unsigned bits, std::uint32_t value) { command(0,0xe3000000u|((32-shift-bits)<<8)|(bits-1),value); }
    void low(unsigned shift, unsigned bits, std::uint32_t value) { command(0,0xe2000000u|((32-shift-bits)<<8)|(bits-1),value); }
    void fog(bool enabled) {
        const auto& f = resources.setup();
        if (enabled) {
            const auto delta = std::int32_t(f.fog_max)-f.fog_min;
            require(delta > 0 && f.fog_max <= 1000,"World producer invalid fog distance");
            const auto multiplier = 128000/delta, offset = (500-std::int32_t(f.fog_min))*256/delta;
            require(multiplier <= 32767 && offset >= -32768 && offset <= 32767,"World producer fog numeric range");
            command(0,0xe7000000,0); command(0,0xdb080000,(std::uint32_t(multiplier)<<16)|(std::uint32_t(offset)&65535));
            command(0,0xf8000000,(std::uint32_t(f.fog_rgb[0])<<24)|(std::uint32_t(f.fog_rgb[1])<<16)|(std::uint32_t(f.fog_rgb[2])<<8));
            high(20,2,1u<<20); low(3,29,0xc8112078); command(0,0xd9ffffff,0x00010001);
        } else {
            command(0,0xe7000000,0); high(20,2,0); low(3,29,0x00552078); command(0,0xd9feffff,0);
        }
    }
    void stage(const scene::render::Pass* pass, const menu::model::RenderContext& context) {
        counts.fill(0); sky_draw = false; sky_model = nullptr; sky_placement_address = 0;
        const auto owners = world.render_objects(); std::size_t active = 0;
        for (const auto& instance : instances) {
            require(instance.render_index < owners.size(),"World render owners changed before producer retirement");
            const auto& object = owners[instance.render_index];
            if (!object.object || !object.model) continue;
            const bool included = !pass || pass->includes({object.render_link,object.priority,UINT32_MAX});
            if (object.sky) {
                require(instance.render_index == 0,"World direct sky source order changed");
                sky_model = object.model;
                native(isle_game_model_placement(game,object.model,&sky_placement));
                if (sky_placement->mode != ISLE_GAME_MODEL_PLACEMENT_DISABLED) {
                    require(64 <= layout.payloads.offset+layout.payloads.size-local_payload_end,
                            "World direct sky placement payload reservation exhausted");
                    sky_placement_address = static_cast<std::uint32_t>(local_payload_end);
                }
                std::size_t count = 0;
                native(isle_game_model_peek_nodes(game,object.model,&sky_node,&count));
                require(count == 1 && sky_node->matrix_kind == ISLE_GAME_MODEL_TRANSLATE,
                        "World direct sky native attachment changed");
                IsleGameObjectMetadata metadata{}; native(isle_game_object_metadata(game,object.object,&metadata));
                sky_draw = included && !(metadata.flags&1u);
            } else {
                const auto state = world.animation_model(object.instance);
                require(bool(state) && state.resource_model == object.resource_model,"World instance resource ownership changed");
                bindings[active++] = {instance.slot,state,object.object,object.model,
                    menu::model::PoseSource::native,instance.root,included,&world.animation_runtime()};
            }
        }
        require(bool(kernel->stage(std::span(bindings).first(active),sky_placement_address ? 64 : 0)),
                "World borrowed model kernel rejected native pose/material/capacity");
        for (const auto& instance : instances) {
            const auto& object = owners[instance.render_index];
            if (!object.object || !object.model) continue;
            if (object.sky) {
                if (!sky_draw) continue;
                command(0,0xe7000000,0); high(20,2,0); low(3,29,0x00552048); command(0,0xd9fefffe,0);
                if (sky_placement_address) command(0,0xda380000,sky_placement_address);
                command(0,sky_placement_address ? 0xda380001 : 0xda380000,instance.matrices);
                command(0,0xd5000001,0); command(0,0xde000000,object.display_list);
                command(0,0xd8380002,64); command(0,0xe7000000,0); command(0,0xd9ffffff,0x00010001);
                continue;
            }
            if (!kernel->draws(instance.slot)) continue;
            const bool fogged = object.callback_identity == 0x800a1530 || object.callback_identity == 0x800a15d8;
            require(fogged || object.callback_identity == 0x80014f98,"Unadmitted source world render callback");
            if (fogged) fog(true);
            menu::model::HierarchyBuffer buffer{{std::span(lanes[0]),std::span(lanes[1]),
                std::span(lanes[2]),std::span(lanes[3])},counts,pending};
            require(bool(kernel->encode(instance.slot,context,buffer)),"World shared hierarchy encoder rejected source pose");
            if (fogged) fog(false);
        }
        std::size_t cursor = layout.lists.offset+camera_commands.size()*8;
        for (unsigned lane = 0; lane < 4; ++lane) {
            command(lane,0xdf000000,0);
            require(cursor <= layout.lists.offset+layout.lists.size && counts[lane]*8 <= layout.lists.offset+layout.lists.size-cursor,
                    "World producer display lists exceed owner reservation");
            ranges[lane] = {static_cast<std::uint32_t>(cursor),static_cast<std::uint32_t>(counts[lane])};
            cursor += counts[lane]*8;
        }
    }
};
Producer::Producer(IsleGame* game, IsleGameCamera* camera, menu::model::CameraBasisState& basis,
                   const Resources& resources, const World& world, ProducerLayout layout,
                   std::span<const Range> other): impl_(std::make_unique<Impl>(game,camera,basis,resources,world,layout)) {
    auto& s = *impl_;
    require(game && camera && !world.retired(),"World producer requires live native owners");
    resources.require_live();
    require(world.resources().setup == &resources.setup() && world.resources().sky == &resources.sky() &&
            world.resources().blocks.data() == resources.blocks().data() && world.resources().blocks.size() == resources.blocks().size(),
            "World producer and native component borrow different resource owners");
    for (std::size_t i = 0; i < s.owned.size(); ++i) {
        valid(s.owned[i],i == 0 ? 64 : 8);
        require(!overlap(s.owned[i],{0,graphics::bss_end}),"World producer overlaps cold image/BSS");
        for (std::size_t j = 0; j < i; ++j) require(!overlap(s.owned[i],s.owned[j]),"World producer owners overlap");
        for (const auto r : other) { valid(r,1); require(!overlap(s.owned[i],r),"World producer overlaps another runtime owner"); }
        for (const auto& span : resources.installation_spans())
            require(!overlap(s.owned[i],{span.guest_address-0x80000000,span.bytes.size()}),"World producer overlaps source assets");
    }
    std::size_t matrix = layout.payloads.offset+256, material = layout.materials.offset, nodes_total = 0;
    const auto objects = world.render_objects(); s.instances.reserve(objects.size());
    std::vector<menu::model::KernelSlot> slots; slots.reserve(objects.size());
    for (std::size_t i = 0; i < objects.size(); ++i) {
        const auto& object = objects[i]; const IsleGameModelNode* nodes = nullptr; std::size_t count = 0;
        native(isle_game_model_peek_nodes(game,object.model,&nodes,&count));
        Impl::Instance instance{i,no_index,static_cast<std::uint32_t>(matrix)};
        if (!object.sky) {
            require(object.resource_model < resources.models().size(),"World resource model escapes borrowed resources");
            const auto capacity = menu::animation::slot_capacity(resources.models()[object.resource_model]);
            require(capacity.nodes == count,"World native model capacity differs from admitted source");
            instance.slot = slots.size();
            // Contextual source18+44/48 also uses a native TRANSLATE cache.
            // Only the synthetic terrain resource marker selects root override.
            instance.root = resources.models()[object.resource_model].matrix_kinds ==
                std::array<std::uint8_t,3>{1,0,0} ?
                menu::model::RootRecipe::translate : menu::model::RootRecipe::source;
            slots.push_back({capacity,static_cast<std::uint32_t>(matrix),static_cast<std::uint32_t>(material)});
            material += capacity.material_attachments*264;
        }
        matrix += count*64; nodes_total += count; s.instances.push_back(instance);
    }
    require(matrix <= layout.payloads.offset+layout.payloads.size && material <= layout.materials.offset+layout.materials.size,
            "World producer payload/material reservation exhausted");
    s.local_payload_end = matrix; s.bindings.resize(slots.size());
    s.kernel = std::make_unique<menu::model::BorrowedKernel>(game,resources.models(),resources.materials(),slots,
        Range{matrix,layout.payloads.offset+layout.payloads.size-matrix});
    for (auto& lane : s.lanes) lane.resize(layout.lists.size/8);
    s.pending.resize(nodes_total*14+objects.size());
}
Producer::~Producer() = default;
ProducerFrame Producer::prepare_filtered(std::span<std::uint32_t> words, std::uint64_t epoch, const scene::render::Pass* pass) {
    auto& s = *impl_;
    require(!s.retired && !s.world.retired() && words.data() && epoch && words.size_bytes() == beach::rdram_size,
            "World producer preparation requires live exclusive 8MiB owner/epoch");
    require(!pass || pass->camera == s.camera,"World pass camera differs from native producer owner");
    native(isle_game_camera_validate_disjoint(s.game,s.camera,words.data(),words.size_bytes()));
    s.resources.require_live();
    const auto begin = reinterpret_cast<std::uintptr_t>(words.data()), end = begin+words.size_bytes();
    require(begin <= UINTPTR_MAX-words.size_bytes() && begin%alignof(std::uint32_t) == 0,
            "World producer RAM pointer range is invalid");
    for (const auto& span : s.resources.installation_spans()) {
        const auto pointer = reinterpret_cast<std::uintptr_t>(span.bytes.data());
        require(end <= pointer || pointer+span.bytes.size() <= begin,"World producer RAM aliases immutable ROM input");
    }
    auto disjoint = [&](const void* pointer, std::size_t bytes) {
        if (!bytes) return;
        const auto at = reinterpret_cast<std::uintptr_t>(pointer);
        require(pointer && at <= UINTPTR_MAX-bytes && (end <= at || at+bytes <= begin),
                "World producer RAM aliases preparation state");
    };
    disjoint(this,sizeof(*this)); disjoint(&s,sizeof(s));
    disjoint(&s.camera_basis,sizeof(s.camera_basis));
    disjoint(s.instances.data(),s.instances.size()*sizeof(Impl::Instance));
    disjoint(s.bindings.data(),s.bindings.size()*sizeof(menu::model::BorrowedBinding));
    disjoint(s.pending.data(),s.pending.size()*sizeof(Command));
    for (const auto& lane : s.lanes) disjoint(lane.data(),lane.size()*sizeof(Command));
    std::size_t count = 0; native(isle_game_camera_matrix_count(s.game,s.camera,&count));
    require(count == 2,"World producer source camera matrix count changed");
    std::array<const IsleGameCameraMatrix*,2> camera{};
    for (std::size_t i = 0; i < 2; ++i) {
        native(isle_game_camera_peek_matrix(s.game,s.camera,i,&camera[i]));
        require(camera[i]->kind == (i == 0 ? ISLE_GAME_CAMERA_PERSPECTIVE_FAST : ISLE_GAME_CAMERA_LOOK_AT_REFLECT_ROLL),
                "World producer requires source camera kinds3,14");
        disjoint(camera[i],sizeof(*camera[i]));
    }
    const IsleGameCameraParameters* parameters = nullptr;
    native(isle_game_camera_parameters(s.game,s.camera,&parameters));
    disjoint(parameters,sizeof(*parameters));
    menu::model::RenderContext context;
    constexpr std::array<menu::model::CameraMatrixOperation,2> camera_operations{{{3,0},{14,0}}};
    require(bool(menu::model::prepare_render_context(s.camera_basis.value,*parameters,camera_operations,
        s.camera_basis.selector,context)),"World source camera context is unrepresentable");
    require(bool(s.kernel->validate_upload(words,epoch)),"World kernel RAM aliases fixed preparation scratch");
    s.stage(pass,context);
    require(bool(s.kernel->validate_upload(words,epoch)),"World kernel RAM aliases borrowed state or has invalid extent");
    native(isle_game_camera_compute_all(s.game,s.camera));
    const IsleGameCameraReflection* reflection = nullptr;
    native(isle_game_camera_reflection(s.game,s.camera,1,&reflection));
    std::uint16_t norm = 0; native(isle_game_camera_perspective_normalization(s.game,s.camera,&norm));
    s.kernel->commit_pose();
    if (s.sky_model) { std::size_t sky_count = 0; native(isle_game_model_nodes(s.game,s.sky_model,&s.sky_node,&sky_count)); }
    s.camera_commands = {{{0xda380007,s.payload(0)},{0xdb0e0000,norm},{0xdc08000a,s.payload(192)},
        {0xdc08030a,s.payload(208)},{0xda380005,s.payload(64)},{0xda380003,s.payload(128)},
        {0xdb060000,0},{0xd5000001,0},{0xdf000000,0}}};
    graphics::Memory memory(words);
    const bool fresh = s.ram != words.data() || s.epoch != epoch;
    for (std::size_t i = 0; i < camera.size(); ++i) if (fresh || s.camera_revisions[i] != camera[i]->revision) {
        memory.matrix(s.payload(i*64),camera[i]->fixed); s.camera_revisions[i] = camera[i]->revision;
    }
    if (fresh) {
        const IsleGameFixedMatrix identity{{{0x00010000,0,1,0},{0,0x00010000,0,1},{0,0,0,0},{0,0,0,0}}};
        memory.matrix(s.payload(128),identity);
    }
    if (fresh || s.reflection_revision != reflection->revision) {
        for (unsigned axis = 0; axis < 2; ++axis) {
            const auto at = s.payload(192+axis*16);
            for (unsigned b = 0; b < 16; ++b) memory.byte(at+b,0);
            for (unsigned c = 0; c < 3; ++c) {
                memory.byte(at+c,reflection->color[axis][c]); memory.byte(at+4+c,reflection->color_copy[axis][c]);
                memory.byte(at+8+c,static_cast<std::uint8_t>(reflection->direction[axis][c]));
            }
        }
        s.reflection_revision = reflection->revision;
    }
    s.kernel->upload(words,epoch);
    if (s.published_sky_placement_address != s.sky_placement_address) s.sky_resident = false;
    s.published_sky_placement_address = s.sky_placement_address;
    if (s.sky_draw) {
        const bool sky_fresh = fresh || !s.sky_resident || s.resident_sky_model != s.sky_model;
        if (s.sky_placement_address && (sky_fresh || s.resident_sky_placement_address != s.sky_placement_address ||
            s.sky_placement_revision != s.sky_placement->revision)) memory.matrix(s.sky_placement_address,s.sky_placement->fixed);
        if (sky_fresh || s.sky_revision != s.sky_node->revision) memory.matrix(s.instances[0].matrices,s.sky_node->fixed);
        s.sky_revision = s.sky_node->revision; s.sky_placement_revision = s.sky_placement->revision;
        s.resident_sky_placement_address = s.sky_placement_address; s.resident_sky_model = s.sky_model; s.sky_resident = true;
    } else if (fresh) s.sky_resident = false;
    const auto write = [&](std::uint32_t at, Command c) {
        if (fresh || words[at/4] != c.w0) memory.word(at,c.w0);
        if (fresh || words[at/4+1] != c.w1) memory.word(at+4,c.w1);
    };
    for (std::size_t i = 0; i < s.camera_commands.size(); ++i) write(static_cast<std::uint32_t>(s.layout.lists.offset+i*8),s.camera_commands[i]);
    for (unsigned lane = 0; lane < 4; ++lane) for (std::size_t i = 0; i < s.counts[lane]; ++i)
        write(s.ranges[lane].address+static_cast<std::uint32_t>(i*8),s.lanes[lane][i]);
    s.ram = words.data(); s.epoch = epoch; ++s.revision;
    s.camera_basis.value = context;
    return {{static_cast<std::uint32_t>(s.layout.lists.offset),static_cast<std::uint32_t>(s.camera_commands.size())},s.ranges,s.owned,s.revision};
}
ProducerFrame Producer::prepare(std::span<std::uint32_t> words, std::uint64_t epoch) { return prepare_filtered(words,epoch,nullptr); }
ProducerFrame Producer::prepare(std::span<std::uint32_t> words, std::uint64_t epoch, const scene::render::Pass& pass) {
    return prepare_filtered(words,epoch,&pass);
}
bool Producer::prepare_pass(void* user, std::span<std::uint32_t> words, std::uint64_t epoch,
                            const scene::render::Pass& pass, scene::render::Prepared& out) {
    require(user != nullptr,"World compiled pass requires its actual producer owner");
    const auto frame = static_cast<Producer*>(user)->prepare(words,epoch,pass);
    scene::render::Prepared staged;
    staged.camera = {frame.camera.address,static_cast<std::size_t>(frame.camera.commands)*2};
    for (unsigned lane = 0; lane < 4; ++lane)
        staged.lanes[lane] = {frame.lanes[lane].address,static_cast<std::size_t>(frame.lanes[lane].commands)*2};
    out = staged; return true;
}
std::span<const Range> Producer::reservations() const noexcept { return impl_->owned; }
void Producer::retire() noexcept { impl_->retired = true; }
} // namespace isle::world
