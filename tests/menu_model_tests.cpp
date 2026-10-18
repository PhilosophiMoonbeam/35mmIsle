#include "menu_model_producer.hpp"
#include "borrowed_model_kernel.hpp"
#include "contextual_model_math.h"
#include <array>
#include <algorithm>
#include <bit>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <new>

namespace allocation_probe { bool enabled = false; std::size_t count = 0; }
void* operator new(std::size_t bytes) {
    if (allocation_probe::enabled) ++allocation_probe::count;
    if (auto* p = std::malloc(bytes ? bytes : 1)) return p;
    throw std::bad_alloc();
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }

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
    model::RenderContext context{};
    std::array<model::MatrixRecipe,6> recipes{};
    std::array<IsleGameFixedMatrix,6> fixed{};
    std::array<unsigned char,6> consumed{};
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
        context.perspective.m[0][0] = 2; context.perspective.m[1][1] = 3;
        context.perspective.m[2][2] = 4; context.perspective.m[2][3] = -1;
        placement.transform.scale = {1,1,1};
    }
    model::Status encode(bool reset = true) {
        if (reset) buffer.counts.fill(0);
        consumed.fill(0);
        for (std::size_t n = 0; n < nodes.size(); ++n) {
            const auto status = model::decode_model_recipe(source,n,recipes[n]);
            if (!status) return status;
        }
        return model::encode_hierarchy({source,nodes,first,next,materials,placement,context,
            0x1000,0x2000,{},model::PoseSource::animation,recipes,fixed,consumed},buffer);
    }
};
void source_null_draw_guards() {
    for (const auto type:{RenderType::i_fogged,RenderType::j_fogged}) {
        Hierarchy h; h.source.render_type=type;
        for (auto& node:h.source.nodes) node.payloads.clear();
        const bool immediate=type==RenderType::i_fogged;
        const unsigned guarded_lane=immediate ? 0u : 1u;
        h.source.nodes[0].payloads.push_back({{},guarded_lane,0x80003000,0});
        h.nodes[0].material_count=1; h.materials[0]=0x4000;
        h.source.nodes[0].materials.push_back(0);
        h.source.nodes[2].payloads.push_back({{},immediate ? 0u : 2u,0,0x80004000});
        require(bool(h.encode()),"NULL draw hierarchy rejected valid source node");
        unsigned pre=0,draws=0;
        for (unsigned lane=0;lane<4;++lane) for (std::size_t i=0;i<h.counts[lane];++i) {
            const auto command=h.storage[lane][i];
            require(command.w0!=0xdb060038,"NULL draw unnecessarily bound material state");
            if (command.w0!=0xde000000) continue;
            require(command.w1!=0,"NULL source draw became a branch into RAM zero");
            if (command.w1==0x80003000) ++pre;
            else { require(command.w1==0x80004000,"NULL source payload invented a draw target"); ++draws; }
        }
        require(pre==(immediate ? 1u : 0u) && draws==1,
                "TypeI pre-only side effect or deferred NULL pre/draw source guard changed");
        if (!immediate) require(h.counts[guarded_lane]==0,
                "Deferred NULL draw flushed matrices/materials/pop into an otherwise unused lane");
    }
}
void type_a_draws_only_an_enabled_root() {
    for (const auto type : {RenderType::a,RenderType::a_fogged}) {
        Hierarchy h;
        h.source.render_type = type;
        h.source.matrix_kinds = {27,0,0};
        h.source.nodes[0].payloads.push_back({{},0,0,0x80001800});
        h.nodes[0].material_count = 1;
        h.materials[0] = 0x4000;
        for (bool placed : {false,true}) {
            h.placement.mode = placed ? ISLE_GAME_MODEL_PLACEMENT_RT : ISLE_GAME_MODEL_PLACEMENT_DISABLED;
            require(bool(h.encode()),"TypeA root draw rejected");
            unsigned depth = 0, draws = 0, bindings = 0;
            for (unsigned lane = 0; lane < 4; ++lane) for (std::size_t i = 0; i < h.counts[lane]; ++i) {
                const auto command = h.storage[lane][i];
                require(lane == 0,"TypeA traversed a child display lane");
                if ((command.w0 >> 24) == 0xda) {
                    require(command.w1 == 0x1000 || (placed && command.w1 == 0x2000),
                        "TypeA consumed a descendant matrix");
                    if (!(command.w0 & 1u)) ++depth;
                } else if ((command.w0 >> 24) == 0xd8) {
                    require(depth != 0,"TypeA popped an absent root scope"); --depth;
                } else if (command.w0 == 0xde000000) {
                    require(command.w1 == 0x80001800 && depth == 1,"TypeA drew a child or escaped its root scope");
                    ++draws;
                } else if (command.w0 == 0xdb060038) {
                    require(command.w1 == 0x4000,"TypeA bound another node's materials"); ++bindings;
                }
            }
            require(depth == 0 && draws == 1 && bindings == 1,"TypeA lost root material/draw/scope semantics");
        }
        for (auto flags : {1u,2u,4u,0x4000u}) {
            h.nodes[0].flags = flags;
            require(bool(h.encode()) && std::all_of(h.counts.begin(),h.counts.end(),[](auto n) { return n == 0; }),
                "TypeA nonzero root flags still traversed children or emitted root work");
        }
        h.nodes[0].flags = 0;
        h.source.nodes[0].payloads[0].draw_list = 0;
        require(bool(h.encode()) && std::all_of(h.counts.begin(),h.counts.end(),[](auto n) { return n == 0; }),
            "TypeA null root payload traversed children or bound materials");
    }
}
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
bool equal(const model::WorldMatrix& a, const model::WorldMatrix& b) {
    return std::memcmp(&a,&b,sizeof(a)) == 0;
}
void configured_recipe_selection() {
    require(model::select_source_matrix_kinds(0,{54,32,26}) ==
        std::array<std::uint8_t,3>{28,0,0},"stored kinds used without Pokemon flag10");
    require(model::select_source_matrix_kinds(0x35,{54,0,0}) ==
        std::array<std::uint8_t,3>{54,0,0},"configured Snorlax54 lost flag10 selection");
    for (const auto ordinary : std::array<std::uint8_t,2>{28,54}) {
        model::MatrixRecipe recipe;
        require(bool(model::decode_recipe({ordinary,0,0},0,recipe)) &&
            recipe.count == 1 && recipe.operations[0] == ordinary,"ordinary configured kind changed");
        require(bool(model::decode_recipe({ordinary,0,0},0x2000,recipe)) &&
            recipe.count == 2 && recipe.operations[0] == 18 && recipe.operations[1] == 48 &&
            recipe.native_kind == ISLE_GAME_MODEL_TRANSLATE,"reached high2000 did not become translate+48");
        require(bool(model::decode_recipe({ordinary,0,0},0x8000,recipe)) &&
            recipe.operations[1] == 44,"high8000 did not become translate+44");
        require(bool(model::decode_recipe({ordinary,0,0},0xa000,recipe)) &&
            recipe.operations[1] == 48,"high2000 precedence over scale changed");
        const auto saved = recipe;
        for (const auto high : {0x4000u,0x6000u,0x1000u,0x9000u})
            require(!model::decode_recipe({ordinary,0,0},high,recipe) &&
                std::memcmp(&recipe,&saved,sizeof(recipe)) == 0,"unsupported override coerced or partially published");
    }
    model::MatrixRecipe recipe;
    require(!model::decode_recipe({31,0,0},0x2000,recipe) &&
        !model::decode_recipe({32,0,0},0x8000,recipe) &&
        !model::decode_recipe({26,0,0},0x8000,recipe),"PYR47/scale43/no-scale42 became RPY");
    require(bool(model::decode_recipe({18,26,32},0x8000,recipe)) && recipe.operations[1] == 44,
        "decomposition before override lost second rotation/third scale");
    require(!model::decode_recipe({28,29,0},0x2000,recipe),"second-kind PYR override was ignored");
    Model proxy; proxy.matrix_kinds = {1,0,0}; proxy.nodes.resize(2);
    proxy.nodes[1].parent = 0;
    require(bool(model::decode_model_recipe(proxy,0,recipe)) && recipe.operations[0] == 1 &&
        bool(model::decode_model_recipe(proxy,1,recipe)) && recipe.operations[0] == 28,
        "World native translate proxy changed decoration children");
}
void camera_value_context_and_retention() {
    model::CameraBasisState shared;
    IsleGameCameraParameters camera{};
    camera.perspective = {45,4.0f/3.0f,10,1000,1};
    camera.look_at = {{3,7,11},{1,2,4},{0,1,0}};
    constexpr std::array<model::CameraMatrixOperation,2> yup{{{3,0},{8,0}}};
    model::RenderContext first;
    require(bool(model::prepare_render_context(shared.value,camera,yup,0,first)),"source camera context rejected");
    model::WorldMatrix axis1, axis2;
    require(isle_model_source48_basis(&camera,first.perspective.m,1,axis1.m) == ISLE_GAME_OK &&
        isle_model_source48_basis(&camera,first.perspective.m,2,axis2.m) == ISLE_GAME_OK &&
        equal(first.basis48,axis1) && !equal(axis1,axis2),"source48 did not use synthetic Y-up axis1");
    shared.value = first;
    for (unsigned selector = 0; selector < 8; ++selector) {
        model::RenderContext next;
        require(bool(model::prepare_render_context(shared.value,camera,yup,selector,next)),"selector rejected");
        const auto& expected = selector == 5 || selector == 6 ? axis2 : axis1;
        require(equal(next.basis48,expected),"source selector mapping changed");
    }
    auto eye_only = camera; eye_only.look_at.eye[1] = 23;
    model::RenderContext moved;
    require(bool(model::prepare_render_context(first,eye_only,yup,0,moved)) &&
        !equal(moved.basis48,first.basis48) && equal(moved.perspective,first.perspective),
        "camera-only eye change at fixed perspective retained stale48 basis");
    auto other_camera = eye_only; other_camera.perspective.fovy = 55;
    model::RenderContext other;
    for (const auto selector : {1u,4u,7u})
        require(bool(model::prepare_render_context(shared.value,other_camera,yup,selector,other)) &&
            equal(other.basis48,first.basis48) && !equal(other.perspective,first.perspective),
            "cross-camera retention replaced AFA8 with current perspective/basis");
    constexpr std::array<model::CameraMatrixOperation,2> cached{{{3,2},{8,2}}};
    require(bool(model::prepare_render_context(first,other_camera,cached,0,other)) &&
        equal(other.perspective,first.perspective) && equal(other.basis48,first.basis48),
        "source-cache2 recomputed retained globals");
    require(bool(model::prepare_render_context(first,other_camera,{},2,other)) &&
        equal(other.basis48,first.basis48),"zero camera matrices executed forced selector");
    shared.selector = 7; shared.reset_selector();
    require(shared.selector == 0 && equal(shared.value.basis48,first.basis48),"source reset erased retained basis");
    constexpr std::array<model::CameraMatrixOperation,2> plain{{{3,0},{6,0}}};
    camera.look_at.up[1] = 0; camera.look_at.up[2] = 1;
    require(bool(model::prepare_render_context(first,camera,plain,0,other)) &&
        equal(other.basis48,axis2),"plain look-at up-axis selector changed");
    constexpr std::array<model::CameraMatrixOperation,2> zup{{{3,0},{16,0}}};
    require(bool(model::prepare_render_context(first,camera,zup,0,other)) &&
        equal(other.basis48,axis2),"reflected Z-up selector changed");
    camera.look_at.eye[0] = camera.look_at.at[0];
    camera.look_at.eye[2] = camera.look_at.at[2];
    require(bool(model::prepare_render_context(first,camera,yup,0,other)),"degenerate source48 basis rejected");
    for (unsigned r = 0; r < 3; ++r) for (float value : other.basis48.m[r])
        require(value == 0,"near-zero source48 distance concatenated perspective");
    require(other.basis48.m[3][3] == 1,"near-zero source48 W changed");
    const auto saved = other;
    camera.look_at.eye[0] = std::numeric_limits<float>::max();
    require(!model::prepare_render_context(first,camera,yup,0,other) &&
        std::memcmp(&other,&saved,sizeof(other)) == 0 && equal(shared.value.basis48,first.basis48),
        "camera context failure published partial retained state");
}
void contextual_xyz_siblings_and_hidden() {
    Hierarchy h; h.source.matrix_kinds = {54,0,0};
    h.placement.mode = ISLE_GAME_MODEL_PLACEMENT_RTS;
    h.placement.transform.scale = {-2,3,5};
    h.nodes[0].transform.scale = {2,7,11};
    h.nodes[1].transform.scale = {3,13,17};
    for (auto& node : h.nodes) node.transform.rotation = {0.2f,-0.3f,0.4f};
    h.nodes[1].flags = 1;
    auto check = [&] {
        require(bool(h.encode()),"source54 deferred XYZ traversal rejected");
        const auto root = h.nodes[0].transform.scale, a = h.nodes[1].transform.scale,
            b = h.nodes[3].transform.scale;
        const auto p = h.placement.transform.scale;
        const IsleGameVec3 r{p.x*root.x,p.y*root.y,p.z*root.z};
        const std::array<IsleGameVec3,6> inherited{{p,r,{r.x*a.x,r.y*a.y,r.z*a.z},r,
            {r.x*b.x,r.y*b.y,r.z*b.z},r}};
        for (std::size_t n = 0; n < h.nodes.size(); ++n) {
            const auto& t = h.nodes[n].transform;
            const IsleGameModelTransform pose{{t.position.x,t.position.y,t.position.z},
                {t.rotation.x,t.rotation.y,t.rotation.z},{t.scale.x,t.scale.y,t.scale.z}};
            const float xyz[]{inherited[n].x,inherited[n].y,inherited[n].z};
            IsleGameFixedMatrix expected;
            require(isle_model_source54_matrix(&pose,xyz,&expected) == ISLE_GAME_OK &&
                h.consumed[n] && std::memcmp(&expected,&h.fixed[n],sizeof(expected)) == 0,
                "source54 sibling/deferred lane inherited XYZ advanced more than once");
        }
    };
    check(); const auto child = h.fixed[2], sibling = h.fixed[3];
    h.nodes[1].transform.scale.y *= 2; check();
    require(std::memcmp(&child,&h.fixed[2],sizeof(child)) != 0 &&
        std::memcmp(&sibling,&h.fixed[3],sizeof(sibling)) == 0,"ancestor Y did not invalidate child or leaked to sibling");
    h.placement.transform.scale.z *= 2; check();
    h.nodes[2].transform.scale = {0,0,0}; check(); // Zero LOCAL scale is lawful.
    h.placement.transform.scale.x = 0;
    require(!h.encode(),"zero inherited scale was rescued in source54");
    h.nodes[0].flags = 2;
    require(bool(h.encode()) && std::all_of(h.consumed.begin(),h.consumed.end(),
        [](auto v) { return v == 0; }),"hidden subtree manufactured contextual consumption/errors");
    h.nodes[0].flags = 0; h.placement.transform.scale = {-2,3,5}; h.nodes[2].flags = 2;
    require(bool(h.encode()) && !h.consumed[2] && h.consumed[3],"hidden child corrupted sibling context");
}
void source48_rows_are_not44() {
    Hierarchy h; h.source.nodes[2].depth_flags = 0x2000;
    h.nodes[0].transform.scale = {2,13,17}; h.nodes[1].transform.scale = {3,19,23};
    h.nodes[2].transform.scale = {5,7,11};
    for (unsigned r = 0; r < 3; ++r) for (unsigned c = 0; c < 4; ++c)
        h.context.basis48.m[r][c] = float(r*4+c+1)/8;
    h.context.basis48.m[3][0] = std::numeric_limits<float>::infinity(); // Unconsumed row.
    require(bool(h.encode()),"source48 rejected unused basis row3");
    const auto old = h.storage[0];
    auto inline_words = [&] {
        std::array<std::uint32_t,12> rows{}; std::size_t count = 0;
        for (std::size_t i = 0; i < h.counts[0]; ++i) if ((h.storage[0][i].w0 >> 24) == 0xdb)
            rows[count++] = h.storage[0][i].w1;
        require(count == 12,"source48 emitted wrong row command extent"); return rows;
    };
    const auto rows = inline_words();
    require((rows[0] >> 16) == 3 && (rows[2] & 65535) == 31,
        "source48 did not use inheritedX for localX/Y independently");
    h.context.basis48.m[0][1] += 1;
    require(bool(h.encode()) && inline_words() != rows,"camera-only basis change reused inline48 rows");
    h.source.nodes[2].depth_flags = 0x8000;
    require(bool(h.encode()) && h.storage[0][0].w0 == old[0].w0 && inline_words() != rows,
        "kind44 silently consumed camera48 basis");
}
void native(IsleGameResult result) { require(result == ISLE_GAME_OK,"kernel native owner operation rejected"); }
struct KernelFixture {
    std::vector<std::byte> arena_bytes = std::vector<std::byte>(1024*1024);
    isle_arena arena{};
    IsleGame* game = nullptr;
    std::array<IsleGameObject*,2> objects{};
    std::array<IsleGameModel*,2> native_models{};
    std::array<Input,3> inputs;
    std::array<Material,3> materials;
    std::array<Model,2> models;
    std::array<animation::SlotCapacity,2> capacities{{{3,2},{3,2}}};
    std::unique_ptr<animation::Runtime> runtime;
    std::array<animation::InstanceHandle,2> handles;
    const std::array<model::KernelSlot,2> plans{{{{3,2},0x1000,0x2000},{{3,2},0x1100,0x2400}}};
    std::unique_ptr<model::BorrowedKernel> kernel;
    std::array<IsleGameModelNodeInit,3> attachment_scratch{};
    std::vector<std::uint32_t> ram = std::vector<std::uint32_t>(rdram_size/4,0xdeadbeef);
    std::array<std::array<model::Command,512>,4> storage{};
    std::array<model::Command,128> pending{};
    std::array<std::size_t,4> counts{};
    model::HierarchyBuffer output{{storage[0],storage[1],storage[2],storage[3]},counts,pending};
    bool native_pose = false;
    explicit KernelFixture(bool contextual = false, std::uint32_t row_flags = 0,
                           std::uint8_t ordinary_kind = 28) {
        require(isle_arena_init(&arena,arena_bytes.data(),arena_bytes.size()) == ISLE_OK,"kernel arena initialization");
        native(isle_game_create(&arena,&game));
        for (std::size_t i = 0; i < materials.size(); ++i) {
            materials[i] = inputs[i].material;
            materials[i].parameters[2] = materials[i].parameters[3] = 1;
            for (auto& image : materials[i].images) image += static_cast<std::uint32_t>(i*0x10000);
        }
        // A finite, representable source scroll that cannot fit the encoder's
        // required signed fixed-coordinate conversion; selected only by API.
        materials[2].flags = 0x20; materials[2].parameters[0] = 1.0e30f;
        for (std::size_t r = 0; r < models.size(); ++r) {
            auto& source = models[r]; source.render_type = RenderType::i_fogged;
            source.matrix_kinds = {ordinary_kind,0,0};
            source.nodes.resize(r ? 1 : 3);
            for (std::size_t n = 0; n < source.nodes.size(); ++n) {
                auto& node = source.nodes[n]; node.parent = n ? 0 : no_index;
                node.scale = {1,1,1}; node.position = {float(10+r*10+n),0,0};
            }
            source.nodes[0].materials = r ? std::vector<std::size_t>{1} : std::vector<std::size_t>{0,0};
            source.nodes[0].payloads.push_back({{},0,0x80004000,0x80005000});
        }
        if (contextual) {
            models[0].matrix_kinds = {54,0,0};
            models[0].nodes[1].depth_flags = 0x2000;
            models[0].nodes[0].scale = {2,3,5};
            models[0].nodes[2].rotation = {0.2f,-0.3f,0.4f};
        }
        if (row_flags) {
            models[0].nodes[1].depth_flags = row_flags;
            models[0].nodes[1].rotation = {0.2f,-0.3f,0.4f};
            models[0].nodes[1].scale = {3,-4,5};
            models[0].nodes[1].payloads.push_back({{},0,0,0x80005008});
        }
        auto created = animation::Runtime::create(animation::Resources{models,materials,{}},capacities,
            {{0,0,100},{0,0,0},0,45,true,true,0,{}},{});
        require(bool(created),"kernel animator creation rejected"); runtime = std::move(created.runtime);
        for (std::size_t i = 0; i < handles.size(); ++i) {
            native(isle_game_add_object(game,static_cast<std::uint32_t>(i+1),0,0,nullptr,nullptr,&objects[i]));
            const auto acquired = runtime->acquire(i,0,false); require(bool(acquired),"kernel instance acquisition rejected");
            handles[i] = acquired.handle; attach(i,0);
        }
        kernel = std::make_unique<model::BorrowedKernel>(game,models,materials,plans,RamRange{0x3000,128});
    }
    ~KernelFixture() { if (game && isle_game_destroy(game) != ISLE_GAME_OK) std::terminate(); }
    void attach(std::size_t slot, std::size_t resource) {
        require(bool(model::attach_source_model(game,objects[slot],models[resource],runtime->model(handles[slot]),
            model::RootRecipe::source,attachment_scratch,&native_models[slot])),"kernel borrowed native attachment rejected");
    }
    std::array<model::BorrowedBinding,2> bindings(bool draw_first = true) {
        const auto pose = native_pose ? model::PoseSource::native : model::PoseSource::animation;
        return {{{1,runtime->model(handles[1]),objects[1],native_models[1],pose,
                model::RootRecipe::source,true,runtime.get()},
            {0,runtime->model(handles[0]),objects[0],native_models[0],pose,
                model::RootRecipe::source,draw_first,runtime.get()}}};
    }
    void prepare(bool draw_first = true, std::uint64_t epoch = 1) {
        const auto ordered = bindings(draw_first);
        require(bool(kernel->stage(ordered)) && bool(kernel->validate_upload(ram,epoch)),"kernel bounded stage rejected");
        counts.fill(0);
        for (const auto& binding : ordered)
            require(bool(kernel->encode(binding.slot,{one(),{}},output)),"kernel hierarchy rejected");
        kernel->commit_pose(); kernel->upload(ram,epoch);
    }
    void replace(std::size_t resource) {
        const auto old = handles[0];
        require(bool(runtime->begin_retire(old)),"kernel retirement begin rejected");
        native(isle_game_remove_model(game,native_models[0])); native_models[0] = nullptr;
        // This harness has no native processes or submitted graphics/effects:
        // removing the real model completes every outstanding native borrow.
        require(bool(runtime->release_retired(old)),"kernel joined retirement release rejected");
        const auto acquired = runtime->acquire(0,resource,false); require(bool(acquired),"kernel reusable acquisition rejected");
        handles[0] = acquired.handle; attach(0,resource);
    }
};
IsleGameModelTransform native_transform(const animation::Transform& t) {
    return {{t.position.x,t.position.y,t.position.z},{t.rotation.x,t.rotation.y,t.rotation.z},
        {t.scale.x,t.scale.y,t.scale.z}};
}
bool same_pose(const IsleGameModelTransform& a, const IsleGameModelTransform& b) {
    return a.position.x == b.position.x && a.position.y == b.position.y && a.position.z == b.position.z &&
        a.rotation.x == b.rotation.x && a.rotation.y == b.rotation.y && a.rotation.z == b.rotation.z &&
        a.scale.x == b.scale.x && a.scale.y == b.scale.y && a.scale.z == b.scale.z;
}
float fixed_component(const IsleGameFixedMatrix& m, unsigned row, unsigned col) {
    const auto index = (row % 2) * 2 + col / 2;
    const auto shift = col % 2 ? 0 : 16;
    const auto integer = (m.m[row / 2][index] >> shift) & 65535u;
    const auto fraction = (m.m[2 + row / 2][index] >> shift) & 65535u;
    const auto bits = (integer << 16) | fraction;
    return static_cast<float>(std::bit_cast<std::int32_t>(bits)) / 65536.0f;
}
IsleGameFixedMatrix uploaded_child(const KernelFixture& f) {
    IsleGameFixedMatrix result;
    for (unsigned row = 0; row < 4; ++row) for (unsigned col = 0; col < 4; ++col)
        result.m[row][col] = f.ram[0x1040 / 4 + row * 4 + col];
    return result;
}
void translation_payload(const IsleGameFixedMatrix& m, IsleGameVec3 position) {
    const std::array<float,3> xyz{position.x,position.y,position.z};
    for (unsigned row = 0; row < 4; ++row) for (unsigned col = 0; col < 4; ++col) {
        const auto expected = row == 3 && col < 3 ? xyz[col] : row == col ? 1.0f : 0.0f;
        require(fixed_component(m,row,col) == expected,"contextual node uploaded more than its translation cache");
    }
}
void kernel_rt_local_scale_and_residency() {
    KernelFixture f(false,0,27);
    animation::Transform pose{{1.5f,-2.25f,3.125f},{0,1.57079637f,0},
        {std::numeric_limits<float>::max(),-7,0}};
    require(bool(f.runtime->set_node_transform(f.handles[0],0,pose)),"RT animation pose rejected");
    f.prepare();
    auto nodes = f.kernel->native_nodes(f.handles[0]);
    require(nodes[0].matrix_kind == ISLE_GAME_MODEL_ROTATE_RPY_TRANSLATE &&
        nodes[0].transform.scale.x == pose.scale.x &&
        fixed_component(nodes[0].fixed,0,2) == -1 && fixed_component(nodes[0].fixed,2,0) == 1 &&
        fixed_component(nodes[0].fixed,1,1) == 1 && fixed_component(nodes[0].fixed,3,0) == 1.5f,
        "kernel attachment/upload did not consume actual Source27 math");
    const auto original_fixed = nodes[0].fixed;
    const auto original_world = f.kernel->world_nodes(f.handles[0])[0];
    const auto original_revision = nodes[0].revision;
    require(std::abs(original_world.m[0][2]+1) < 0.0001f &&
        std::abs(original_world.m[2][0]-1) < 0.0001f && original_world.m[1][1] == 1 &&
        original_world.m[3][0] == 1.5f,"RT world projection consumed canonical scale");
    auto resident = [&] {
        return std::memcmp(f.ram.data()+0x1000/4,&nodes[0].fixed,sizeof(nodes[0].fixed)) == 0;
    };
    require(resident(),"RT native fixed cache did not reach canonical RAM");
    pose.scale = {-std::numeric_limits<float>::max(),0,37};
    require(bool(f.runtime->set_node_transform(f.handles[0],0,pose)),"RT scale-only animation edit rejected");
    f.prepare();
    require(nodes[0].revision == original_revision+1 && nodes[0].transform.scale.z == 37 &&
        std::memcmp(&nodes[0].fixed,&original_fixed,sizeof(original_fixed)) == 0 &&
        equal(original_world,f.kernel->world_nodes(f.handles[0])[0]) && resident(),
        "RT scale-only edit erased fields or changed fixed/world matrix bytes");
    f.native_pose = true;
    auto native_pose = nodes[0].transform;
    native_pose.position.x = 13;
    native_pose.scale = {29,31,41};
    native(isle_game_model_set_transform(f.game,f.native_models[0],0,&native_pose));
    std::fill_n(f.ram.begin()+0x1000/4,16,0xdeadbeefu);
    f.prepare(false);
    require(nodes[0].transform.position.x == 13 && f.ram[0x1000/4] == 0xdeadbeefu,
        "hidden RT binding overwrote RAM or restored divergent Runtime pose");
    f.prepare();
    require(resident() && fixed_component(nodes[0].fixed,3,0) == 13 &&
        f.kernel->world_nodes(f.handles[0])[0].m[3][0] == 13,
        "revealed RT binding reused old matrix residency");
    const auto revision = nodes[0].revision;
    std::fill_n(f.ram.begin()+0x1000/4,16,0xdeadbeefu);
    f.prepare(true,2);
    require(resident() && nodes[0].revision == revision,"RT new RAM epoch did not republish the stable cache");
    std::vector<std::uint32_t> fresh(rdram_size/4,0xdeadbeef); f.ram.swap(fresh);
    f.prepare(true,2);
    require(resident() && nodes[0].revision == revision,"RT replacement RAM inherited stale residency");
}
void contextual_child_rows(const KernelFixture& f, const model::RenderContext& context,
    bool source48, const IsleGameModelTransform& root, const IsleGameModelTransform& child) {
    // Consume public G_MW_MATRIX integer/fraction pairs, not recipe metadata or
    // command counts. These rows replace the combined MVP, not the native RTS.
    IsleGameFixedMatrix rows{};
    bool found = false;
    for (std::size_t i = 0; i < f.counts[0]; ++i) {
        if ((f.storage[0][i].w0 >> 24) != 0xda || f.storage[0][i].w1 != 0x1040) continue;
        require(!found && i + 13 < f.counts[0] && f.storage[0][i + 1].w0 == 0xd5000001,
            "contextual child did not expose its combined-MVP row update");
        found = true;
        for (unsigned part = 0; part < 2; ++part) for (unsigned pair = 0; pair < 6; ++pair) {
            const auto command = f.storage[0][i + 2 + part * 6 + pair];
            require(command.w0 == (0xdb000000u | (part * 32 + pair * 4)),
                "contextual row consumer received a different matrix register");
            rows.m[part * 2 + pair / 4][pair % 4] = command.w1;
        }
    }
    require(found,"contextual child draw omitted its matrix row commands");
    const float x = root.scale.x * child.scale.x, y = root.scale.x * child.scale.y;
    for (unsigned row = 0; row < 3; ++row) for (unsigned col = 0; col < 4; ++col) {
        const float basis = source48 ? context.basis48.m[row][col] :
            ((row == col || (row == 2 && col == 3)) ? context.perspective.m[row][col] : 0);
        require(fixed_component(rows,row,col) == basis * (row == 1 ? y : x),
            "contextual row commands used a different pose or inherited scale authority");
    }
}
void mathematical_child_world(const KernelFixture& f, const IsleGameModelTransform& root,
    const IsleGameModelTransform& child) {
    // Independent Z-rotation/scale oracle; the root has scale and translation.
    // Canonical rotation and Z scale matter here even though 44/48 ignore them.
    const auto c = std::cos(child.rotation.z), s = std::sin(child.rotation.z);
    model::WorldMatrix expected;
    expected.m[0][0] = child.scale.x * c * root.scale.x;
    expected.m[0][1] = child.scale.x * s * root.scale.y;
    expected.m[1][0] = -child.scale.y * s * root.scale.x;
    expected.m[1][1] = child.scale.y * c * root.scale.y;
    expected.m[2][2] = child.scale.z * root.scale.z;
    expected.m[3][0] = child.position.x * root.scale.x + root.position.x;
    expected.m[3][1] = child.position.y * root.scale.y + root.position.y;
    expected.m[3][2] = child.position.z * root.scale.z + root.position.z;
    expected.m[3][3] = 1;
    const auto worlds = f.kernel->world_nodes(f.handles[0]);
    require(worlds.size() == 3,"selected pose did not publish its complete mathematical hierarchy");
    for (unsigned row = 0; row < 4; ++row) for (unsigned col = 0; col < 4; ++col)
        require(std::abs(worlds[1].m[row][col] - expected.m[row][col]) < 0.001f,
            "mathematical world used renderer rows or the unselected local pose");
}
void kernel_contextual_pose_authority() {
    for (const auto high : {0x8000u,0x2000u}) {
        KernelFixture f(false,high);
        const IsleGameModelNode* nodes = nullptr; std::size_t count = 0;
        native(isle_game_model_peek_nodes(f.game,f.native_models[0],&nodes,&count));
        const auto attached = native_transform(f.runtime->model(f.handles[0]).nodes[1].transform);
        require(count == 3 && nodes[1].matrix_kind == ISLE_GAME_MODEL_TRANSLATE &&
            same_pose(nodes[1].transform,attached),"44/48 attachment erased canonical rotation/scale");
        f.prepare();
        require(same_pose(nodes[1].transform,attached),"first animation commit erased attached 44/48 pose fields");
        translation_payload(uploaded_child(f),attached.position);

        animation::Transform runtime_root{{17,-19,23},{},{7,11,13}};
        animation::Transform runtime_child{{-2,3,5},{0,0,-1.57079637f},{5,-6,9}};
        auto native_root = native_transform(animation::Transform{{10,-3,7},{},{2,3,4}});
        auto native_child = native_transform(animation::Transform{{11,2,-5},{0,0,1.57079637f},{3,-4,5}});
        require(bool(f.runtime->set_node_transform(f.handles[0],0,runtime_root)) &&
            bool(f.runtime->set_node_transform(f.handles[0],1,runtime_child)),"divergent Runtime pose rejected");
        native(isle_game_model_set_transform(f.game,f.native_models[0],0,&native_root));
        native(isle_game_model_set_transform(f.game,f.native_models[0],1,&native_child));
        f.native_pose = true;
        model::RenderContext context;
        context.perspective.m[0][0] = 2; context.perspective.m[1][1] = 3;
        context.perspective.m[2][2] = 4; context.perspective.m[2][3] = -1;
        for (unsigned row = 0; row < 3; ++row) for (unsigned col = 0; col < 4; ++col)
            context.basis48.m[row][col] = float(row * 4 + col + 1) / 8;
        auto encode = [&] {
            const auto ordered = f.bindings();
            require(bool(f.kernel->stage(ordered)) && bool(f.kernel->validate_upload(f.ram,1)),
                "selected full-pose stage rejected");
            f.counts.fill(0);
            for (const auto& binding : ordered)
                require(bool(f.kernel->encode(binding.slot,context,f.output)),"selected full-pose rows rejected");
        };
        auto publish = [&] { f.kernel->commit_pose(); f.kernel->upload(f.ram,1); };
        encode(); contextual_child_rows(f,context,high == 0x2000,native_root,native_child); publish();
        mathematical_child_world(f,native_root,native_child);
        const auto native_world = f.kernel->world_nodes(f.handles[0])[1];
        const auto native_revision = nodes[1].revision;
        runtime_child.scale.x = 8; runtime_child.rotation.z = -0.5f;
        require(bool(f.runtime->set_node_transform(f.handles[0],1,runtime_child)),"unselected Runtime edit rejected");
        encode();
        require(high == 0x2000 || !f.kernel->lists_changed(),
            "native44 change detection borrowed the unselected Runtime revision");
        contextual_child_rows(f,context,high == 0x2000,native_root,native_child); publish();
        require(equal(native_world,f.kernel->world_nodes(f.handles[0])[1]) &&
            nodes[1].revision == native_revision && same_pose(nodes[1].transform,native_child),
            "unselected Runtime edit changed native pose publication or mathematical world");

        const auto old_child = nodes[1];
        const auto old_payload = uploaded_child(f);
        const auto sibling_world = f.kernel->world_nodes(f.handles[0])[2];
        native_child.rotation.z = -0.75f; native_child.scale = {-2,6,8};
        native(isle_game_model_set_transform(f.game,f.native_models[0],1,&native_child));
        encode();
        require(nodes[1].revision == old_child.revision &&
            std::memcmp(&nodes[1].fixed,&old_child.fixed,sizeof(old_child.fixed)) == 0,
            "R/S-only stage eagerly evaluated the native translation cache");
        contextual_child_rows(f,context,high == 0x2000,native_root,native_child); publish();
        const auto new_payload = uploaded_child(f);
        require(nodes[1].revision > old_child.revision && same_pose(nodes[1].transform,native_child) &&
            std::memcmp(&new_payload,&old_payload,sizeof(old_payload)) == 0 &&
            equal(sibling_world,f.kernel->world_nodes(f.handles[0])[2]),
            "R/S-only native edit erased fields, changed translation bytes or damaged a sibling");
        mathematical_child_world(f,native_root,native_child);

        // Native spans still contain the divergent old pose during encoding.
        // Animation authority must use staged Runtime fields before publication.
        f.native_pose = false; encode();
        contextual_child_rows(f,context,high == 0x2000,native_transform(runtime_root),native_transform(runtime_child));
        require(same_pose(nodes[1].transform,native_child),"animation stage published before list admission");
        publish();
        require(same_pose(nodes[0].transform,native_transform(runtime_root)) &&
            same_pose(nodes[1].transform,native_transform(runtime_child)),
            "animation commit erased canonical contextual pose fields");
        mathematical_child_world(f,native_transform(runtime_root),native_transform(runtime_child));
        translation_payload(uploaded_child(f),native_transform(runtime_child).position);

        const auto ram_before = f.ram;
        const std::array<IsleGameModelNode,3> before{nodes[0],nodes[1],nodes[2]};
        const auto worlds = f.kernel->world_nodes(f.handles[0]);
        const std::array<model::WorldMatrix,3> world_before{worlds[0],worlds[1],worlds[2]};
        auto staged_root = runtime_root; staged_root.position.x += 1;
        auto overflow = runtime_child; overflow.scale.x = 40000; overflow.rotation.z = 0.25f;
        require(bool(f.runtime->set_node_transform(f.handles[0],0,staged_root)) &&
            bool(f.runtime->set_node_transform(f.handles[0],1,overflow)),"finite late-rejection input rejected");
        require(bool(f.kernel->stage(f.bindings())),"translation cache incorrectly applied ordinary RTS scale bounds");
        f.counts.fill(0);
        require(!f.kernel->encode(0,context,f.output),"44/48 fixed-row overflow was admitted");
        f.kernel->commit_pose(); f.kernel->upload(f.ram,1);
        require(f.ram == ram_before,"late contextual rejection published RAM");
        for (std::size_t n = 0; n < before.size(); ++n)
            require(same_pose(nodes[n].transform,before[n].transform) && nodes[n].revision == before[n].revision &&
                nodes[n].flags == before[n].flags && std::memcmp(&nodes[n].fixed,&before[n].fixed,sizeof(before[n].fixed)) == 0 &&
                equal(world_before[n],f.kernel->world_nodes(f.handles[0])[n]),
                "late contextual rejection published staged animation/native caches/worlds");
        require(bool(f.runtime->set_node_transform(f.handles[0],0,runtime_root)) &&
            bool(f.runtime->set_node_transform(f.handles[0],1,runtime_child)),"contextual retry restore rejected");
        encode(); publish();
        require(f.ram == ram_before,"valid contextual retry retained rejected scratch");
    }
}
void kernel_independent_instances_and_order() {
    KernelFixture f;
    require(bool(f.runtime->set_model_speed(f.handles[0],2)) &&
        bool(f.runtime->set_model_speed(f.handles[1],-0.5f)),"independent speed setters rejected");
    const std::array<std::size_t,2> replacements{1,1};
    require(bool(f.runtime->reset_materials(f.handles[0],replacements)),"selected replacement material reset rejected");
    const IsleGameModelTransform a{{13,0,0},{},{1,1,1}}, b{{-29,0,0},{},{1,1,1}};
    native(isle_game_model_set_placement(f.game,f.native_models[0],ISLE_GAME_MODEL_PLACEMENT_RT,&a));
    native(isle_game_model_set_placement(f.game,f.native_models[1],ISLE_GAME_MODEL_PLACEMENT_RT,&b));
    f.prepare();
    require(f.runtime->model(f.handles[0]).nodes[0].clock.speed == 2 &&
        f.runtime->model(f.handles[1]).nodes[0].clock.speed == -0.5f &&
        f.kernel->world_nodes(f.handles[0])[0].m[3][0] == 23 && f.kernel->world_nodes(f.handles[1])[0].m[3][0] == -19,
        "same-resource instances coupled clocks or placement worlds");
    require(f.ram[0x2010/4+1] == f.materials[1].images[0] && f.ram[0x2410/4+1] == f.materials[0].images[0],
        "kernel selected original attachment metadata instead of MaterialState.resource");
    std::array<std::uint32_t,2> roots{}; std::size_t root_count = 0;
    for (std::size_t i = 0; i < f.counts[0]; ++i) {
        const auto c = f.storage[0][i];
        if (c.w0 == 0xda380000 && (c.w1 == 0x3000 || c.w1 == 0x3040)) {
            require(root_count < roots.size() && i && f.storage[0][i-1].w1 == 0x80004000,
                "actual ordered pre-list was moved inside the root scope");
            roots[root_count++] = f.storage[0][i+1].w1;
        }
    }
    require(root_count == 2 && roots[0] == 0x1100 && roots[1] == 0x1000,
        "kernel sorted actual render order by resource or cache slot");
    require(bool(f.runtime->set_object_flags(f.handles[0],1)),"independent hide rejected");
    f.prepare();
    require(!f.kernel->draws(0) && f.kernel->draws(1),"same-resource visibility was coupled");
    require(bool(f.runtime->set_object_flags(f.handles[0],0)),"independent reveal rejected");
    const auto cursor = f.arena.cursor; allocation_probe::count = 0; allocation_probe::enabled = true;
    for (unsigned i = 0; i < 32; ++i) f.prepare();
    allocation_probe::enabled = false;
    require(allocation_probe::count == 0 && f.arena.cursor == cursor,"kernel hot paths allocated heap/native storage");
}
void kernel_reuse_hidden_residency_and_epoch() {
    KernelFixture f; f.prepare();
    auto local_commands = [&] {
        std::size_t count = 0;
        for (unsigned lane = 0; lane < 4; ++lane) for (std::size_t i = 0; i < f.counts[lane]; ++i) {
            const auto c = f.storage[lane][i];
            if ((c.w0 >> 24) == 0xda && c.w1 >= 0x1000 && c.w1 < 0x10c0) ++count;
        }
        return count;
    };
    require(local_commands() == 3,"initial large instance omitted admitted hierarchy locals");
    const auto stale = f.handles[0]; const auto stale_view = f.runtime->model(stale);
    const auto* stable = stale_view.nodes.data();
    f.replace(1);
    require(f.runtime->model(f.handles[0]).nodes.data() == stable && !f.runtime->model(stale) &&
        f.kernel->world_nodes(stale).empty() && f.kernel->native_nodes(stale).empty(),
        "reused animator storage moved or old incarnation remained usable");
    std::fill_n(f.ram.begin()+0x1000/4,3*16,0xdeadbeef);
    std::fill_n(f.ram.begin()+0x2000/4,2*264/4,0xdeadbeef);
    f.prepare(false); // Actual pass filtering: new incarnation is staged but not uploaded.
    require(local_commands() == 0,"filtered new incarnation retained old hierarchy lists");
    require(f.ram[0x2000/4] == 0xdeadbeef && f.kernel->world_nodes(f.handles[0]).size() == 1,
        "filtered reuse wrote RAM or exposed a stale large-model tail");
    f.prepare();
    require(local_commands() == 1,"small revealed incarnation retained a large hierarchy tail");
    require(f.ram[0x2000/4] == 0xde010000 && f.ram[0x2004/4] == 0x2008 &&
        f.ram[0x2008/4+1] == f.materials[1].images[0] && f.ram[0x1000/4] != 0xdeadbeef,
        "same-epoch reveal retained old branch/material/matrix residency");
    f.replace(0);
    require(bool(f.runtime->set_object_flags(f.handles[0],1)),"hidden fresh instance rejected");
    f.prepare();
    require(local_commands() == 0,"hidden reused incarnation retained old hierarchy lists");
    require(bool(f.runtime->set_object_flags(f.handles[0],0)),"hidden fresh instance reveal rejected");
    f.prepare();
    require(local_commands() == 3,"large revealed incarnation retained short hierarchy lists");
    require(f.kernel->world_nodes(f.handles[0]).size() == 3 && f.ram[0x2004/4] == 0x2010 &&
        f.ram[0x2008/4] == 0xde010000 && f.ram[0x200c/4] == 0x2110 &&
        f.ram[0x2010/4+1] == f.materials[0].images[0],"small-to-large hidden reuse retained a short branch table");
    auto bad = f.bindings(); bad[1].state = stale_view;
    const auto before = f.ram;
    require(!f.kernel->stage(bad) && f.ram == before,"stale Runtime view changed live RAM");
    auto other = animation::Runtime::create(animation::Resources{f.models,f.materials,{}},f.capacities,
        {{0,0,100},{0,0,0},0,45,true,true,0,{}},{});
    require(bool(other),"wrong-pool regression runtime creation rejected");
    auto wrong_pool = f.bindings(); wrong_pool[1].runtime = other.runtime.get();
    require(!f.kernel->stage(wrong_pool) && f.ram == before,"wrong-pool view authorized another owner");
    std::fill(f.ram.begin(),f.ram.end(),0xdeadbeef); f.prepare(true,2);
    require(f.ram[0x2000/4] == 0xde010000 && f.ram[0x200c/4] == 0x2110 &&
        f.ram[0x1000/4] != 0xdeadbeef,"fresh epoch skipped unchanged matrices/branch payloads");
    std::vector<std::uint32_t> fresh(rdram_size/4,0xdeadbeef); f.ram.swap(fresh); f.prepare(true,2);
    require(f.ram[0x2000/4] == 0xde010000 && f.ram[0x2010/4+1] == f.materials[0].images[0],
        "new RAM pointer with equal epoch reused stale material residency");
}
void kernel_late_rejection_is_atomic() {
    KernelFixture f; f.prepare();
    const auto prior_ram = f.ram; const auto prior_world = f.kernel->world_nodes(f.handles[0])[0];
    const IsleGameModelNode* nodes = nullptr; std::size_t count = 0;
    native(isle_game_model_peek_nodes(f.game,f.native_models[0],&nodes,&count));
    const auto prior = nodes[0]; auto changed = prior.transform; changed.position.x = 123;
    native(isle_game_model_set_transform(f.game,f.native_models[0],0,&changed));
    auto ordered = f.bindings();
    // First binding validates a genuinely dirty native pose. A later binding
    // fails in the real material encoder, not an artificial capacity sentinel.
    std::swap(ordered[0],ordered[1]); ordered[0].pose = model::PoseSource::native;
    const std::array<std::size_t,2> rejected_materials{2,2};
    require(bool(f.runtime->reset_materials(f.handles[1],rejected_materials)),"late-reject material API reset failed");
    require(!f.kernel->stage(ordered),"late real material encoder rejection was admitted");
    const IsleGameModelNode* after = nullptr;
    native(isle_game_model_peek_nodes(f.game,f.native_models[0],&after,&count));
    require(after[0].transform.position.x == 123 && after[0].revision == prior.revision &&
        std::memcmp(&after[0].fixed,&prior.fixed,sizeof(prior.fixed)) == 0 && f.ram == prior_ram &&
        std::memcmp(&prior_world,&f.kernel->world_nodes(f.handles[0])[0],sizeof(prior_world)) == 0,
        "late rejection computed native fixed/revision caches or published world/RAM");
    require(bool(f.runtime->reset_materials(f.handles[1])),"late-reject material API restore failed");
    ordered = f.bindings(); std::swap(ordered[0],ordered[1]); ordered[0].pose = model::PoseSource::native;
    require(bool(f.kernel->stage(ordered)),"valid native preflight failed before list boundary");
    f.counts.fill(0);
    auto tight_output = f.output; tight_output.lanes[0] = tight_output.lanes[0].first(1);
    require(!f.kernel->encode(0,{one(),{}},tight_output),"late bounded hierarchy-list rejection was admitted");
    native(isle_game_model_peek_nodes(f.game,f.native_models[0],&after,&count));
    require(after[0].revision == prior.revision && std::memcmp(&after[0].fixed,&prior.fixed,sizeof(prior.fixed)) == 0 &&
        f.ram == prior_ram && std::memcmp(&prior_world,&f.kernel->world_nodes(f.handles[0])[0],sizeof(prior_world)) == 0,
        "late hierarchy rejection published native caches/world/RAM");
    native(isle_game_model_set_transform(f.game,f.native_models[0],0,&prior.transform)); f.prepare();
    require(f.ram == prior_ram,"valid retry retained rejected material scratch");
    auto tiny_plans = f.plans; tiny_plans[0].capacity = {1,1};
    model::BorrowedKernel tiny(f.game,f.models,f.materials,tiny_plans,{0x3000,0});
    require(!tiny.stage(f.bindings()) && f.ram == prior_ram,"native slot capacity rejection published payloads");
    for (const auto recipe : {48,47}) {
        auto bad_models = f.models; bad_models[0].matrix_kinds[0] = static_cast<std::uint8_t>(recipe);
        bool rejected = false;
        try { model::BorrowedKernel unsupported(f.game,bad_models,f.materials,f.plans,{0x3000,0}); }
        catch (const std::invalid_argument&) { rejected = true; }
        require(rejected && f.ram == prior_ram,"unadmitted raw override recipe was silently coerced to RTS");
    }
}
void kernel_contextual_residency_and_atomicity() {
    KernelFixture f(true); f.native_pose = true; f.prepare();
    const IsleGameModelNode* nodes = nullptr; std::size_t count = 0;
    native(isle_game_model_peek_nodes(f.game,f.native_models[0],&nodes,&count));
    require(nodes[0].matrix_kind == ISLE_GAME_MODEL_CONTEXTUAL_RPY &&
        nodes[1].matrix_kind == ISLE_GAME_MODEL_TRANSLATE &&
        f.kernel->world_nodes(f.handles[0]).size() == 3,"contextual attachment lost its mathematical world cache");
    auto payload = [&](std::size_t n) {
        IsleGameFixedMatrix result;
        for (unsigned r = 0; r < 4; ++r) for (unsigned c = 0; c < 4; ++c)
            result.m[r][c] = f.ram[(0x1000+n*64)/4+r*4+c];
        return result;
    };
    auto matches_child = [&] {
        const float inherited[]{nodes[0].transform.scale.x,nodes[0].transform.scale.y,nodes[0].transform.scale.z};
        IsleGameFixedMatrix expected;
        native(isle_model_source54_matrix(&nodes[2].transform,inherited,&expected));
        const auto actual = payload(2);
        require(std::memcmp(&expected,&actual,sizeof(actual)) == 0,"renderer uploaded contextual native reserved fixed bytes");
    };
    matches_child(); const auto old_payload = payload(2); const auto child_revision = nodes[2].revision;
    auto root = nodes[0].transform; root.scale.y = 7;
    native(isle_game_model_set_transform(f.game,f.native_models[0],0,&root)); f.prepare();
    matches_child();
    const auto next_payload = payload(2);
    require(nodes[2].revision == child_revision &&
        std::memcmp(&old_payload,&next_payload,sizeof(old_payload)) != 0,
        "same-child-revision ancestor Y reused a native-revision-only payload");
    const auto native_root = nodes[0];
    IsleGameModelTransform placement{{3,4,5},{0.1f,0.2f,0.3f},{2,-3,7}};
    native(isle_game_model_set_placement(f.game,f.native_models[0],ISLE_GAME_MODEL_PLACEMENT_RTS,&placement));
    f.prepare();
    const auto placed = payload(2);
    placement.scale.z = 11;
    native(isle_game_model_set_placement(f.game,f.native_models[0],ISLE_GAME_MODEL_PLACEMENT_RTS,&placement));
    f.prepare(); const auto replaced = payload(2);
    require(std::memcmp(&placed,&replaced,sizeof(placed)) != 0 && nodes[0].revision == native_root.revision &&
        std::memcmp(&nodes[0].fixed,&native_root.fixed,sizeof(native_root.fixed)) == 0,
        "placement Z did not invalidate source54 or rewrote ordinary local state");
    const auto ram_before = f.ram; const auto root_before = nodes[0], child_before = nodes[2];
    const auto world_before = f.kernel->world_nodes(f.handles[0])[2];
    auto overflow = nodes[0].transform; overflow.position.x = 32768;
    native(isle_game_model_set_transform(f.game,f.native_models[0],0,&overflow));
    require(bool(f.kernel->stage(f.bindings())),"finite contextual translation was constrained by old kind28 admission");
    f.counts.fill(0);
    require(!f.kernel->encode(0,{one(),{}},f.output),"contextual fixed-output overflow was admitted");
    f.kernel->commit_pose(); f.kernel->upload(f.ram,1);
    require(f.ram == ram_before && nodes[0].revision == root_before.revision &&
        equal(world_before,f.kernel->world_nodes(f.handles[0])[2]),
        "contextual overflow partially published native/world/RAM state");
    native(isle_game_model_set_transform(f.game,f.native_models[0],0,&root_before.transform));
    placement.scale.x = 0;
    native(isle_game_model_set_placement(f.game,f.native_models[0],ISLE_GAME_MODEL_PLACEMENT_RTS,&placement));
    require(bool(f.kernel->stage(f.bindings())),"finite contextual accepted pose rejected before consumption");
    f.counts.fill(0);
    require(!f.kernel->encode(0,{one(),{}},f.output),"source54 incoming zero was silently repaired");
    f.kernel->commit_pose(); f.kernel->upload(f.ram,1);
    require(f.ram == ram_before && nodes[0].revision == root_before.revision &&
        nodes[2].revision == child_before.revision &&
        equal(world_before,f.kernel->world_nodes(f.handles[0])[2]),
        "contextual math error published native cache/world/RAM");
    native(isle_game_model_set_flags(f.game,f.native_models[0],0,2));
    std::fill_n(f.ram.begin()+0x1000/4,3*16,0xdeadbeef);
    f.prepare(); // Hidden source traversal must not consume zero inherited XYZ.
    require(f.ram[0x1000/4] == 0xdeadbeef && f.ram[0x1080/4] == 0xdeadbeef,
        "hidden source54 uploaded unconsumed scratch/reserved native fixed");
    placement.scale.x = 2;
    native(isle_game_model_set_placement(f.game,f.native_models[0],ISLE_GAME_MODEL_PLACEMENT_RTS,&placement));
    native(isle_game_model_set_flags(f.game,f.native_models[0],0,0)); f.prepare();
    require(f.ram[0x1000/4] != 0xdeadbeef,"same-epoch contextual reveal retained hidden residency");
    const auto stale = f.handles[0]; f.replace(1); f.prepare(false);
    require(f.kernel->native_nodes(stale).empty(),"contextual incarnation survived retirement");
    f.prepare(); f.replace(0); f.prepare();
    native(isle_game_model_peek_nodes(f.game,f.native_models[0],&nodes,&count));
    require(nodes[0].matrix_kind == ISLE_GAME_MODEL_CONTEXTUAL_RPY &&
        f.ram[0x2004/4] == 0x2010,"large-small-large contextual reuse retained short branch/payload state");
    const auto complete = f.ram;
    auto active_equal = [&] {
        for (const auto range : {RamRange{0x1000,192},RamRange{0x1100,192},RamRange{0x2000,16},
                 RamRange{0x2010,32},RamRange{0x2110,32},RamRange{0x2400,16},
                 RamRange{0x2410,32},RamRange{0x2510,32}})
            if (!std::equal(f.ram.begin()+range.offset/4,f.ram.begin()+(range.offset+range.size)/4,
                    complete.begin()+range.offset/4)) return false;
        return true;
    };
    std::fill(f.ram.begin(),f.ram.end(),0xdeadbeef); f.prepare(true,2);
    require(active_equal(),"new contextual RAM epoch retained old installation residency");
    std::vector<std::uint32_t> fresh(rdram_size/4,0xdeadbeef); f.ram.swap(fresh); f.prepare(true,2);
    require(active_equal(),"contextual RAM pointer replacement reused old scratch residency");
    const auto cursor = f.arena.cursor; allocation_probe::count = 0; allocation_probe::enabled = true;
    for (unsigned i = 0; i < 16; ++i) f.prepare();
    allocation_probe::enabled = false;
    require(allocation_probe::count == 0 && f.arena.cursor == cursor,"contextual hot traversal allocated");
}
}
int main() {
    try {
        defaults_and_scroll(); palette_and_light_colors();
        lod_and_atomic_rejection(); fractional_lod_above_terminal_key();
        deferred_placement_scopes_and_hide(); immediate_prelist_and_disabled_capacity();
        source_null_draw_guards();
        type_a_draws_only_an_enabled_root();
        kind44_placement_inherits_only_x();
        configured_recipe_selection(); camera_value_context_and_retention();
        contextual_xyz_siblings_and_hidden(); source48_rows_are_not44();
        kernel_independent_instances_and_order(); kernel_reuse_hidden_residency_and_epoch();
        kernel_late_rejection_is_atomic();
        kernel_rt_local_scale_and_residency();
        kernel_contextual_pose_authority();
        kernel_contextual_residency_and_atomicity();
    }
    catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
    return 0;
}
