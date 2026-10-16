#include "world_producer.hpp"
#include "graphics_task.hpp"
#include <algorithm>
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
        std::size_t render_index;
        std::uint32_t matrices = 0;
        std::span<const IsleGameModelNode> nodes;
        std::vector<std::uint64_t> revisions;
        std::vector<std::uint32_t> material_bases;
        std::vector<menu::model::MaterialCommands> encoded;
        std::vector<std::uint64_t> material_revisions;
        bool resident = false;
        Instance(std::size_t index, std::uint32_t address, std::span<const IsleGameModelNode> input)
            : render_index(index),matrices(address),nodes(input) {}
    };
    IsleGame* game;
    IsleGameCamera* camera;
    const Resources& resources;
    const World& world;
    ProducerLayout layout;
    std::array<Range,3> owned;
    std::vector<Instance> instances;
    std::array<std::vector<Command>,4> lanes;
    std::array<std::size_t,4> counts{};
    std::array<menu::model::ListRange,4> ranges{};
    std::array<Command,9> camera_commands{};
    std::array<std::uint64_t,2> camera_revisions{};
    std::uint64_t reflection_revision = 0, revision = 0, epoch = 0;
    const std::uint32_t* ram = nullptr;
    bool retired = false;
    Impl(IsleGame* g, IsleGameCamera* c, const Resources& r, const World& w, ProducerLayout l)
        : game(g),camera(c),resources(r),world(w),layout(l),owned{l.payloads,l.lists,l.materials} {}
    std::uint32_t payload(std::size_t o) const { return static_cast<std::uint32_t>(layout.payloads.offset+o); }
    void command(unsigned lane, std::uint32_t a, std::uint32_t b) {
        require(counts[lane] < lanes[lane].size(),"World producer command capacity exceeded");
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
    void node(Instance& instance, const menu::Model& model, std::size_t index) {
        const auto& node = instance.nodes[index];
        if (node.flags&2) return;
        command(0,0xda380000,instance.matrices+static_cast<std::uint32_t>(index*64));
        command(0,0xd5000001,0);
        if (!(node.flags&1)) {
            const auto& source = model.nodes[index];
            for (const auto& p : source.payloads) {
                require(p.display_list_id == 0,"Beach source payload changed output lane");
                if (p.pre_list) command(0,0xde000000,p.pre_list);
                if (!source.materials.empty()) command(0,0xdb060038,instance.material_bases[index]);
                if (p.draw_list) command(0,0xde000000,p.draw_list);
            }
        }
        for (auto child = node.first_child; child != UINT32_MAX; child = instance.nodes[child].next_sibling) node_visit(instance,model,child);
        command(0,0xd8380002,64);
    }
    void node_visit(Instance& instance, const menu::Model& model, std::size_t index) { node(instance,model,index); }
    void stage(const scene::render::Pass* pass) {
        counts.fill(0);
        const auto render = world.render_objects();
        for (auto& instance : instances) {
            require(instance.render_index < render.size(),"World render owners changed before producer retirement");
            const auto& object = render[instance.render_index];
            if (!object.object || !object.model) continue;
            if (pass && !pass->includes({object.render_link,object.priority,UINT32_MAX})) continue;
            IsleGameObjectMetadata metadata{};
            native(isle_game_object_metadata(game,object.object,&metadata));
            if (metadata.flags&1u) continue;
            const IsleGameModelNode* nodes = nullptr; std::size_t count = 0;
            native(isle_game_model_nodes(game,object.model,&nodes,&count));
            require(count == instance.revisions.size(),"World native model node ownership changed");
            instance.nodes = {nodes,count};
            if (object.sky) {
                command(0,0xe7000000,0); high(20,2,0); low(3,29,0x00552048); command(0,0xd9fefffe,0);
                command(0,0xda380000,instance.matrices); command(0,0xd5000001,0); command(0,0xde000000,object.display_list);
                command(0,0xd8380002,64); command(0,0xe7000000,0); command(0,0xd9ffffff,0x00010001);
                continue;
            }
            const auto& model = resources.models()[object.resource_model];
            const auto state = world.animation_model(object.resource_model);
            std::size_t material = 0;
            for (std::size_t n = 0; n < model.nodes.size(); ++n) for (const auto resource : model.nodes[n].materials) {
                require(resource < resources.materials().size() && material < state.materials.size(),"World material ownership changed");
                require(bool(menu::model::encode_material(resources.materials()[resource],state.materials[material],instance.encoded[material])),
                        "World material encoder rejected source animation state");
                ++material;
            }
            const bool fogged = object.callback_identity == 0x800a1530 || object.callback_identity == 0x800a15d8;
            require(fogged || object.callback_identity == 0x80014f98,"Unadmitted source world render callback");
            if (fogged) fog(true);
            node(instance,model,0);
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
Producer::Producer(IsleGame* game, IsleGameCamera* camera, const Resources& resources, const World& world,
                   ProducerLayout layout, std::span<const Range> other): impl_(std::make_unique<Impl>(game,camera,resources,world,layout)) {
    auto& s = *impl_;
    require(game && camera && !world.retired(),"World producer requires live native owners");
    resources.require_live();
    require(world.resources().setup == &resources.setup() && world.resources().sky == &resources.sky() &&
            world.resources().blocks.data() == resources.blocks().data() &&
            world.resources().blocks.size() == resources.blocks().size(),
            "World producer and native component borrow different resource owners");
    for (std::size_t i = 0; i < s.owned.size(); ++i) {
        valid(s.owned[i],i == 0 ? 64 : 8);
        require(!overlap(s.owned[i],{0,graphics::bss_end}),"World producer overlaps cold image/BSS");
        for (std::size_t j = 0; j < i; ++j) require(!overlap(s.owned[i],s.owned[j]),"World producer owners overlap");
        for (const auto r : other) { valid(r,1); require(!overlap(s.owned[i],r),"World producer overlaps another runtime owner"); }
        for (const auto& span : resources.installation_spans())
            require(!overlap(s.owned[i],{span.guest_address-0x80000000,span.bytes.size()}),"World producer overlaps source assets");
    }
    std::size_t matrix = layout.payloads.offset+256, material = layout.materials.offset;
    std::size_t command_capacity = 128;
    const auto objects = world.render_objects();
    s.instances.reserve(objects.size());
    for (std::size_t i = 0; i < objects.size(); ++i) {
        const auto& object = objects[i];
        const IsleGameModelNode* nodes = nullptr; std::size_t count = 0;
        native(isle_game_model_nodes(game,object.model,&nodes,&count));
        s.instances.emplace_back(i,static_cast<std::uint32_t>(matrix),std::span(nodes,count));
        auto& instance = s.instances.back(); instance.revisions.resize(count);
        instance.material_bases.resize(count); matrix += count*64;
        if (!object.sky) {
            const auto& source = resources.models()[object.resource_model];
            std::size_t materials = 0;
            for (std::size_t n = 0; n < count; ++n) {
                const auto size = source.nodes[n].materials.size();
                if (size) { instance.material_bases[n] = static_cast<std::uint32_t>(material); material += size*(8+256); }
                materials += size; command_capacity += source.nodes[n].payloads.size()*3+5;
            }
            instance.encoded.resize(materials); instance.material_revisions.resize(materials);
        }
    }
    require(matrix <= layout.payloads.offset+layout.payloads.size && material <= layout.materials.offset+layout.materials.size,
            "World producer payload/material reservation exhausted");
    for (auto& lane : s.lanes) lane.resize(command_capacity);
}
Producer::~Producer() = default;
ProducerFrame Producer::prepare_filtered(std::span<std::uint32_t> words, std::uint64_t epoch, const scene::render::Pass* pass) {
    auto& s = *impl_;
    require(!s.retired && !s.world.retired() && epoch && words.size_bytes() == beach::rdram_size,
            "World producer preparation requires live exclusive 8MiB owner/epoch");
    require(!pass || pass->camera == s.camera,"World pass camera differs from native producer owner");
    s.resources.require_live();
    const auto begin = reinterpret_cast<std::uintptr_t>(words.data()), end = begin+words.size_bytes();
    for (const auto& span : s.resources.installation_spans()) {
        const auto pointer = reinterpret_cast<std::uintptr_t>(span.bytes.data());
        require(end <= pointer || pointer+span.bytes.size() <= begin,"World producer RAM aliases immutable ROM input");
    }
    std::size_t count = 0; native(isle_game_camera_matrix_count(s.game,s.camera,&count));
    require(count == 2,"World producer source camera matrix count changed");
    std::array<const IsleGameCameraMatrix*,2> camera{};
    for (std::size_t i = 0; i < 2; ++i) {
        native(isle_game_camera_matrix(s.game,s.camera,i,&camera[i]));
        require(camera[i]->kind == (i == 0 ? ISLE_GAME_CAMERA_PERSPECTIVE_FAST : ISLE_GAME_CAMERA_LOOK_AT_REFLECT_ROLL),
                "World producer requires source camera kinds3,14");
    }
    s.stage(pass); // All bounded list/material rejection precedes canonical RAM writes.
    native(isle_game_camera_compute_all(s.game,s.camera));
    const IsleGameCameraReflection* reflection = nullptr;
    native(isle_game_camera_reflection(s.game,s.camera,1,&reflection));
    std::uint16_t norm = 0; native(isle_game_camera_perspective_normalization(s.game,s.camera,&norm));
    s.camera_commands = {{{0xda380007,s.payload(0)},{0xdb0e0000,norm},{0xdc08000a,s.payload(192)},
        {0xdc08030a,s.payload(208)},{0xda380005,s.payload(64)},{0xda380003,s.payload(128)},
        {0xdb060000,0},{0xd5000001,0},{0xdf000000,0}}};
    graphics::Memory memory(words);
    const bool fresh = s.ram != words.data() || s.epoch != epoch;
    if (fresh) for (auto& instance : s.instances) instance.resident = false;
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
    const auto render = s.world.render_objects();
    for (auto& instance : s.instances) {
        const auto& object = render[instance.render_index];
        if (!object.object || !object.model) continue;
        if (pass && !pass->includes({object.render_link,object.priority,UINT32_MAX})) continue;
        IsleGameObjectMetadata metadata{};
        native(isle_game_object_metadata(s.game,object.object,&metadata));
        if (metadata.flags&1u) continue;
        const bool instance_fresh = !instance.resident;
        for (std::size_t n = 0; n < instance.nodes.size(); ++n) if (instance_fresh || instance.revisions[n] != instance.nodes[n].revision) {
            memory.matrix(instance.matrices+static_cast<std::uint32_t>(n*64),instance.nodes[n].fixed);
            instance.revisions[n] = instance.nodes[n].revision;
        }
        if (object.sky) { instance.resident = true; continue; }
        const auto state = s.world.animation_model(object.resource_model);
        for (std::size_t n = 0; n < state.nodes.size(); ++n) {
            const auto& node = state.nodes[n];
            for (std::size_t j = 0; j < node.material_count; ++j) {
                const auto index = node.first_material+j;
                const auto branch = instance.material_bases[n]+static_cast<std::uint32_t>(j*8);
                const auto body = instance.material_bases[n]+static_cast<std::uint32_t>(node.material_count*8+j*256);
                if (instance_fresh) { memory.word(branch,0xde010000); memory.word(branch+4,body); }
                if (!instance_fresh && instance.material_revisions[index] == state.materials[index].revision) continue;
                const auto& commands = instance.encoded[index];
                for (std::size_t c = 0; c < commands.count; ++c) {
                    memory.word(body+static_cast<std::uint32_t>(c*8),commands.words[c].w0);
                    memory.word(body+static_cast<std::uint32_t>(c*8+4),commands.words[c].w1);
                }
                instance.material_revisions[index] = state.materials[index].revision;
            }
        }
        instance.resident = true;
    }
    const auto write = [&](std::uint32_t at, Command c) {
        if (fresh || words[at/4] != c.w0) memory.word(at,c.w0);
        if (fresh || words[at/4+1] != c.w1) memory.word(at+4,c.w1);
    };
    for (std::size_t i = 0; i < s.camera_commands.size(); ++i) write(static_cast<std::uint32_t>(s.layout.lists.offset+i*8),s.camera_commands[i]);
    for (unsigned lane = 0; lane < 4; ++lane) for (std::size_t i = 0; i < s.counts[lane]; ++i)
        write(s.ranges[lane].address+static_cast<std::uint32_t>(i*8),s.lanes[lane][i]);
    s.ram = words.data(); s.epoch = epoch; ++s.revision;
    return {{static_cast<std::uint32_t>(s.layout.lists.offset),static_cast<std::uint32_t>(s.camera_commands.size())},s.ranges,s.owned,s.revision};
}
ProducerFrame Producer::prepare(std::span<std::uint32_t> words, std::uint64_t epoch) {
    return prepare_filtered(words,epoch,nullptr);
}
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
    out = staged;
    return true;
}
std::span<const Range> Producer::reservations() const noexcept { return impl_->owned; }
void Producer::retire() noexcept { impl_->retired = true; }
} // namespace isle::world
