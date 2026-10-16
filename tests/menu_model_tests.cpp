#include "menu_model_producer.hpp"
#include <array>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
using namespace isle::menu;
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
struct Input {
    std::array<std::byte,0x78> wire{};
    Material material;
    animation::MaterialState state;
    Input() {
        material.wire = {0x80300000,0,wire};
        material.images = {0x80310000,0x80311000,0x80312000};
        material.palettes = {0x80320000,0x80320200};
        material.format = 0; material.size_code = 2;
        material.auxiliary_format = 0; material.auxiliary_size_code = 2;
        material.width = material.block_width = 32;
        material.height = material.block_height = 16;
        material.scale = 32;
        state.parameters[2] = state.parameters[3] = 1;
    }
};
void defaults_and_scroll() {
    Input input;
    model::MaterialCommands commands;
    require(bool(model::encode_material(input.material,input.state,commands)),"default material rejected");
    require(commands.count == 4 && commands.words[0].w0 == 0xfd100000 &&
        commands.words[0].w1 == input.material.images[0] && commands.words[1].w0 == 0xf2000000 &&
        commands.words[1].w1 == ((124u << 12) | 60) && commands.words[2].w1 == 0xffffffff,
        "flags0 did not apply genuine A1 image/tile/texture defaults");
    input.material.width = 16; input.material.height = 8;
    input.state.parameters[0] = 1; input.state.parameters[1] = 0.25f;
    input.state.parameters[2] = 2; input.state.parameters[3] = 0.5f;
    require(bool(model::encode_material(input.material,input.state,commands)),"scroll rejected");
    require(commands.words[1].w0 == (0xf2000000u | (32u << 12) | 16u) &&
        commands.words[1].w1 == ((92u << 12) | 44u) && commands.words[2].w1 == 0x8000ffff,
        "animated tile scroll/scale lost source grouping or upper saturation");
}
void palette_and_light_colors() {
    Input input;
    input.material.flags = 0x3007;
    input.state.palette_index = 1.9f; input.state.next_image_index = 2;
    input.state.color_and_trailing_words[4] = 0x12345678;
    input.state.color_and_trailing_words[5] = 0x90abcdef;
    model::MaterialCommands commands;
    require(bool(model::encode_material(input.material,input.state,commands)),"palette material rejected");
    require(commands.words[0].w1 == input.material.palettes[1] &&
        commands.words[4].w0 == 0xf0000000 && commands.words[4].w1 == 0x0503c000,
        "palette float truncation or16-entryTLUT wire changed");
    require(commands.words[6].w0 == 0xdb0a0000 && commands.words[7].w0 == 0xdb0a0004 &&
        commands.words[6].w1 == 0x12345678 && commands.words[7].w1 == 0x12345678 &&
        commands.words[8].w0 == 0xdb0a0018 && commands.words[9].w0 == 0xdb0a001c &&
        commands.words[9].w1 == 0x90abcdef,"light colors did not update BOTH public light color words");
    require(commands.words[10].w1 == input.material.images[2],
        "auxiliary load selected the wrong animated image");
    // Public RDP wire consumer: the caller's auxiliary block descriptor owns
    // the upper TMEM half; the primary block descriptor owns the lower half.
    // A LOADBLOCK must resolve through that descriptor, not a presumed tile7.
    std::array<unsigned,8> descriptor_tmem{};
    std::array<unsigned,512> loaded_texels{};
    const std::array<model::Command,2> descriptors{{
        {0xf5100100,0x06000000}, {0xf5100000,0x07000000}}};
    auto consume = [&](model::Command command) {
        const auto opcode = command.w0 >> 24;
        const auto tile = (command.w1 >> 24) & 7u;
        if (opcode == 0xf5) descriptor_tmem[tile] = command.w0 & 511u;
        if (opcode == 0xf3) loaded_texels[descriptor_tmem[tile]] +=
            ((command.w1 >> 12) & 4095u) + 1;
    };
    for (const auto command : descriptors) consume(command);
    for (std::size_t i = 0; i < commands.count; ++i) consume(commands.words[i]);
    require(loaded_texels[0x100] == 32 * 16 && loaded_texels[0] == 0,
        "auxiliary LOADBLOCK did not target its caller-selected upper-TMEM descriptor");
}
void lod_and_atomic_rejection() {
    Input input;
    input.material.flags = 0xe10;
    input.state.lod_level = 1.25f;
    input.state.color_and_trailing_words[0] = 0xabcdef01;
    input.state.color_and_trailing_words[1] = 0x00330000;
    input.state.color_and_trailing_words[2] = 0x23456789;
    input.state.color_and_trailing_words[3] = 0x11223344;
    model::MaterialCommands commands;
    require(bool(model::encode_material(input.material,input.state,commands)),"lod material rejected");
    require(commands.words[0].w0 == 0xfa003340 && commands.words[0].w1 == 0xabcdef01 &&
        commands.words[1].w0 == 0xfb000000 && commands.words[2].w0 == 0xf9000000 &&
        commands.words[3].w1 == input.material.images[2] &&
        commands.words[7].w1 == input.material.images[1],"lod blend must select floor and floor+1, not last/static frame");
    const auto saved = commands;
    input.state.lod_level = 2.0f;
    require(!model::encode_material(input.material,input.state,commands),"out-of-range nextLOD frame admitted");
    require(commands.count == saved.count,"rejection changed command extent");
    for (std::size_t i = 0; i < commands.words.size(); ++i)
        require(commands.words[i].w0 == saved.words[i].w0 && commands.words[i].w1 == saved.words[i].w1,
            "rejection partially published command body");
    input.material.flags = 4; input.state.palette_index = -1;
    require(!model::encode_material(input.material,input.state,commands),"negative palette index admitted");
    input.material.flags = 0; input.state.parameters[2] = std::numeric_limits<float>::infinity();
    require(!model::encode_material(input.material,input.state,commands),"nonfinite scale admitted");
}
void fractional_lod_above_terminal_key() {
    Input input;
    input.material.images = {0x80310000,0x80311000,0x80312000,0x80313000,0x80314000};
    input.material.flags = 0x10;
    input.state.lod_level = 3.25f;
    model::MaterialCommands commands;
    // A Hermite track may exceed its terminal key3 while staying below4.
    // The five-entry table admits this fractional blend of images3 and4.
    require(bool(model::encode_material(input.material,input.state,commands)),
        "legitimate fractional LOD above terminal key was rejected");
    require(commands.words[0].w0 == 0xfa000040 &&
        commands.words[1].w1 == input.material.images[4] &&
        commands.words[5].w1 == input.material.images[3],
        "fractional LOD was clamped to the terminal key or selected the wrong frame pair");
}
struct Hierarchy {
    Model source;
    std::array<animation::NodeState,6> nodes{};
    std::array<std::uint32_t,6> first{1,2,UINT32_MAX,4,UINT32_MAX,UINT32_MAX};
    std::array<std::uint32_t,6> next{UINT32_MAX,3,UINT32_MAX,5,UINT32_MAX,UINT32_MAX}, materials{};
    IsleGameModelPlacement placement{};
    model::WorldMatrix projection{};
    std::array<std::array<model::Command,128>,4> storage{};
    std::array<model::Command,128> pending{};
    std::array<std::size_t,4> counts{};
    model::HierarchyBuffer buffer;
    Hierarchy() : buffer{{storage[0],storage[1],storage[2],storage[3]},counts,pending} {
        source.render_type = RenderType::j_fogged;
        source.nodes.resize(nodes.size());
        for (std::size_t n = 0; n < nodes.size(); ++n) {
            nodes[n].parent = n == 0 ? no_index : n == 2 ? 1 : n == 4 ? 3 : 0;
            nodes[n].transform.scale = {1,1,1};
            source.nodes[n].parent = nodes[n].parent;
        }
        source.nodes[2].payloads.push_back({{},0,0x80002000,0x80001000});
        source.nodes[4].payloads.push_back({{},2,0x80002008,0x80001008});
        source.nodes[5].payloads.push_back({{},1,0x80002010,0x80001010});
        projection.m[0][0] = 2; projection.m[1][1] = 3;
        projection.m[2][2] = 4; projection.m[2][3] = -1;
        placement.transform.scale = {1,1,1};
    }
    model::Status encode(bool reset = true) {
        if (reset) buffer.counts.fill(0);
        return model::encode_hierarchy({source,nodes,first,next,materials,placement,projection,
            0x1000,0x2000},buffer);
    }
};
model::WorldMatrix one() {
    model::WorldMatrix matrix;
    for (unsigned i = 0; i < 4; ++i) matrix.m[i][i] = 1;
    return matrix;
}
model::WorldMatrix cat(const model::WorldMatrix& a, const model::WorldMatrix& b) {
    model::WorldMatrix result;
    for (unsigned row = 0; row < 4; ++row) for (unsigned col = 0; col < 4; ++col)
        for (unsigned k = 0; k < 4; ++k) result.m[row][col] += a.m[row][k] * b.m[k][col];
    return result;
}
void deferred_placement_scopes_and_hide() {
    Hierarchy h;
    h.placement.mode = ISLE_GAME_MODEL_PLACEMENT_RTS;
    h.placement.transform.scale = {-2,3,4};
    h.placement.transform.position = {11,-13,17};
    h.placement.transform.rotation.y = 1.57079637f;
    h.nodes[0].transform.position = {2,3,4};
    h.nodes[0].transform.rotation.z = 1.57079637f;
    h.nodes[1].transform.position = {5,0,0};
    h.nodes[2].transform.position = {0,7,0};
    h.nodes[3].transform.position = {0,0,6};
    h.nodes[4].transform.position = {1,0,0};
    h.nodes[5].transform.position = {-2,0,1};
    require(bool(h.encode()),"deferred geometry-less placement encoding failed");
    // Numeric public GBI consumer, using noncommuting root/placement matrices.
    // Translation-only descendants make independent origin oracles explicit.
    std::array<model::WorldMatrix,6> local;
    for (std::size_t n = 0; n < local.size(); ++n) {
        local[n] = one();
        const auto position = h.nodes[n].transform.position;
        local[n].m[3][0] = position.x;
        local[n].m[3][1] = position.y;
        local[n].m[3][2] = position.z;
    }
    local[0].m[0][0] = local[0].m[1][1] = 0;
    local[0].m[0][1] = 1; local[0].m[1][0] = -1;
    auto placement = one();
    placement.m[0][0] = placement.m[2][2] = 0;
    placement.m[0][2] = -h.placement.transform.scale.x;
    placement.m[1][1] = h.placement.transform.scale.y;
    placement.m[2][0] = h.placement.transform.scale.z;
    placement.m[3][0] = h.placement.transform.position.x;
    placement.m[3][1] = h.placement.transform.position.y;
    placement.m[3][2] = h.placement.transform.position.z;
    const std::array<std::size_t,3> draw_nodes{2,4,5};
    auto origin = [&](std::size_t leaf) {
        // Independent point operations over the declared fixture hierarchy,
        // not matrix concatenation, decoded commands or hand-pinned coordinates.
        IsleGameVec3 point{};
        for (auto n = leaf; n != no_index; n = h.nodes[n].parent) {
            if (n == 0) point = {-point.y,point.x,point.z}; // root +90 degrees Z
            const auto position = h.nodes[n].transform.position;
            point.x += position.x; point.y += position.y; point.z += position.z;
        }
        const auto& actor = h.placement.transform;
        point.x *= actor.scale.x; point.y *= actor.scale.y; point.z *= actor.scale.z;
        // Placement +90 degrees Y, followed by its independently owned position.
        return std::array<float,3>{point.z + actor.position.x,point.y + actor.position.y,
            -point.x + actor.position.z};
    };
    auto consume = [&](unsigned repetitions) {
        std::array<unsigned,3> drawn{};
        for (unsigned lane = 0; lane < 4; ++lane) {
            auto current = one();
            std::array<model::WorldMatrix,16> stack;
            std::size_t depth = 0;
            unsigned root_pushes = 0;
            bool pre_seen = false;
            for (std::size_t i = 0; i < h.buffer.counts[lane]; ++i) {
                const auto c = h.storage[lane][i];
                if ((c.w0 >> 24) == 0xda) {
                    if (!(c.w0 & 1)) stack[depth++] = current;
                    const bool actor = c.w1 == 0x2000;
                    if (actor) {
                        require(pre_seen && depth == 1,"deferred pre-list was moved after root scope preparation");
                        ++root_pushes;
                    }
                    current = cat(actor ? placement : local[(c.w1 - 0x1000) / 64],current);
                } else if ((c.w0 >> 24) == 0xd8) {
                    require(depth > 0,"unused deferred lane/sibling popped an unflushed root");
                    current = stack[--depth];
                } else if ((c.w0 >> 24) == 0xde) {
                    if (c.w1 >= 0x80002000) pre_seen = true;
                    else {
                        const auto draw = (c.w1 - 0x80001000) / 8;
                        require(draw < 3,"unexpected geometry target");
                        const auto expected = origin(draw_nodes[draw]);
                        for (unsigned axis = 0; axis < 3; ++axis)
                            require(current.m[3][axis] == expected[axis],
                                "placement/root/child order or sibling scope produced wrong geometry");
                        ++drawn[draw];
                    }
                }
            }
            require(depth == 0 && root_pushes == (lane == 3 ? 0 : repetitions),
                "placement did not retain exactly one root scope on each flushed lane");
        }
        for (unsigned count : drawn) require(count == repetitions,"deferred geometry missing/duplicated");
    };
    consume(1);
    require(bool(h.encode(false)),"append second sibling object failed");
    consume(2);
    h.nodes[0].flags = 1; // hide only the geometry-less root; descendants remain.
    h.nodes[3].flags = 1;
    require(bool(h.encode()),"hide-self traversal failed"); consume(1);
    h.nodes[1].flags = 2; // whole subtree: lane0 must have no matrix or pop.
    require(bool(h.encode()) && h.buffer.counts[0] == 0 && h.buffer.counts[3] == 0 &&
        h.buffer.counts[1] && h.buffer.counts[2],"subtree hide flushed an unused pending prefix");
    h.nodes[0].flags = 2;
    require(bool(h.encode()) && std::all_of(h.buffer.counts.begin(),h.buffer.counts.end(),
        [](auto count) { return count == 0; }),"hidden root emitted placement/local/pre-list commands");
}
void immediate_prelist_and_disabled_capacity() {
    Hierarchy h;
    h.source.render_type = RenderType::i_fogged;
    h.source.nodes[0].payloads.push_back({{},0,0x80003000,0x80001800});
    // Immediate source variants use lane0.
    for (auto& node : h.source.nodes) for (auto& payload : node.payloads) payload.display_list_id = 0;
    require(bool(h.encode()),"disabled immediate baseline failed");
    const auto baseline = h.storage;
    const auto counts = h.buffer.counts;
    h.buffer.lanes[0] = std::span(h.storage[0]).first(counts[0]);
    require(bool(h.encode()),"disabled exact command capacity was not preserved");
    h.placement.mode = ISLE_GAME_MODEL_PLACEMENT_RT;
    require(!h.encode(),"active placement ignored additional bounded command capacity");
    h.buffer.lanes[0] = h.storage[0];
    require(bool(h.encode()),"active immediate hierarchy failed");
    const auto& lane = h.storage[0];
    require(lane[0].w1 == 0x80003000 && lane[1].w1 == 0x2000 &&
        !(lane[1].w0 & 1u) && lane[2].w1 == 0x1000 && (lane[2].w0 & 1u),
        "immediate pre-list/root placement/root-local logical scope order changed");
    h.placement.mode = ISLE_GAME_MODEL_PLACEMENT_DISABLED;
    require(bool(h.encode()) && h.buffer.counts == counts,"disable changed original command extents");
    for (unsigned lane_id = 0; lane_id < 4; ++lane_id)
        for (std::size_t i = 0; i < counts[lane_id]; ++i)
            require(h.storage[lane_id][i].w0 == baseline[lane_id][i].w0 &&
                h.storage[lane_id][i].w1 == baseline[lane_id][i].w1,
                "disable did not restore exact old command stream/local payload addresses");
    h.placement.mode = ISLE_GAME_MODEL_PLACEMENT_RT;
    h.buffer.pending = std::span(h.pending).first(1);
    require(!h.encode(),"bounded pending prefix exhaustion admitted active placement");
}
void kind44_placement_inherits_only_x() {
    Hierarchy h;
    h.source.nodes[2].depth_flags = 0x8000;
    h.nodes[0].transform.scale = {2,13,17};
    h.nodes[1].transform.scale = {3,19,23};
    h.nodes[2].transform.scale = {5,7,11};
    auto check = [&](float x, float y) {
        require(bool(h.encode()),"kind44 placement encoding failed");
        const auto& commands = h.storage[0];
        bool found = false;
        for (std::size_t i = 0; i + 13 < h.buffer.counts[0]; ++i)
            if ((commands[i].w0 >> 24) == 0xda && commands[i].w1 == 0x1080) {
                // Diagonal row0 integral occupies upper16; row1 lower16.
                const auto expected_x = static_cast<std::uint32_t>(static_cast<std::int32_t>(2 * x));
                const auto expected_y = static_cast<std::uint32_t>(static_cast<std::int32_t>(3 * y));
                require((commands[i + 2].w1 >> 16) == (expected_x & 65535) &&
                    (commands[i + 4].w1 & 65535) == (expected_y & 65535),
                    "kind44 used placement/local Y accumulation instead of source inherited X");
                found = true;
            }
        require(found,"kind44 command rows were not emitted");
    };
    h.placement.mode = ISLE_GAME_MODEL_PLACEMENT_RT;
    h.placement.transform.scale = {std::numeric_limits<float>::quiet_NaN(),99,101};
    check(30,42); // RT ignores scale. Ordinary kind28 accumulates ONLY X.
    h.placement.mode = ISLE_GAME_MODEL_PLACEMENT_RTS;
    h.placement.transform.scale = {-2,29,31};
    check(-60,-84);
    h.placement.transform.scale.x = 0;
    check(0,0);
}
}
int main() {
    try {
        defaults_and_scroll(); palette_and_light_colors();
        lod_and_atomic_rejection(); fractional_lod_above_terminal_key();
        deferred_placement_scopes_and_hide(); immediate_prelist_and_disabled_capacity();
        kind44_placement_inherits_only_x();
    }
    catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
    return 0;
}
