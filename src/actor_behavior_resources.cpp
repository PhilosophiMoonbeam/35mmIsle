#include "actor_behavior_resources.hpp"
#include "model_asset_closure.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <limits>
#include <stdexcept>

namespace isle::actor {
namespace detail {
struct BehaviorRecord {
    std::uint32_t identity;
    // Consumed entries only, excluding the original sentinel record.
    std::size_t entries;
};
struct BehaviorCatalog {
    std::int32_t course_id;
    std::span<const model_assets::Mapping> mappings;
    std::span<const BehaviorRecord> graphs, weighted_states, counted_state_options;
};
} // namespace detail
namespace {
constexpr std::array<std::uint8_t, 20> admitted_sha1{
    0xed,0xc7,0xc4,0x9c,0xc5,0x68,0xc0,0x45,0xfe,0x48,
    0xbe,0x0d,0x18,0x01,0x1c,0x30,0xf3,0x93,0xcb,0xaf
};
// Numeric extents were extracted read-only from the admitted canonical US ROM:
// SHA256 a1d5d816db7f8557ee04c35a011326d058b2c1fbca76b57b352b1d705a1ec1cc.
// Catalogs contain identities/counts, not Source, ROM or asset payload arrays.
constexpr std::array<model_assets::Mapping, 2> beach_mappings{{
    {0x802cbe10,0x563e80,0x22e0},
    {0x80388310,0x528720,0x90},
}};
// Preserve the complete pinned Beach own-GAME declaration/reference closure,
// including the C-local Pidgey graph, Chansey's copied automatic initializer
// and the empty common graph. Root order is initialized guest identity order;
// source row order and every state/aux reference remain untouched.
constexpr std::array<detail::BehaviorRecord, 71> beach_graphs{{
    {0x802cc1d4,5},{0x802cc234,2},{0x802cc264,2},{0x802cc294,2},
    {0x802cc40c,9},{0x802cc4c4,2},{0x802cc4f4,2},{0x802cc524,3},
    {0x802cc564,2},{0x802cc594,2},{0x802cc5c4,7},{0x802cc644,6},
    {0x802cc6b4,2},{0x802cc6e4,2},{0x802cc714,6},{0x802cc784,2},
    {0x802cc7b4,2},{0x802cc7e4,1},{0x802cc804,1},{0x802cc824,2},
    {0x802cc854,1},{0x802cc874,2},{0x802cc8a4,2},{0x802cc8d4,2},
    {0x802cc904,2},{0x802cc934,1},{0x802cca10,8},{0x802ccab8,2},
    {0x802ccae8,3},{0x802ccb28,7},{0x802ccba8,4},{0x802ccbf8,2},
    {0x802ccc28,3},{0x802ccc68,3},{0x802ccca8,2},{0x802ccd60,2},
    {0x802cce1c,1},{0x802ccf14,2},{0x802ccf44,1},{0x802cd010,2},
    {0x802cd040,1},{0x802cd064,4},{0x802cd0b4,2},{0x802cd0e4,1},
    {0x802cd1e4,2},{0x802cd214,2},{0x802cd35c,7},{0x802cd3f8,3},
    {0x802cd438,3},{0x802cd478,2},{0x802cd4a8,3},{0x802cd4e8,2},
    {0x802cd518,2},{0x802cd634,5},{0x802cd694,2},{0x802cd6c4,2},
    {0x802cd6f4,2},{0x802cd858,5},{0x802cd9d8,1},{0x802cdc60,1},
    {0x802cdc80,1},{0x802cdca0,2},{0x802cdcd0,6},{0x802cdd40,3},
    {0x802cdd80,10},{0x802cde48,2},{0x802cde78,7},{0x802cdef8,9},
    {0x802cdf98,3},{0x802cdfd8,3},{0x80388324,0},
}};
constexpr std::array<detail::BehaviorRecord, 8> beach_weighted_states{{
    {0x802cc2c4,2},{0x802cc4ac,2},{0x802ccaa0,2},{0x802cd3dc,2},
    {0x802cd764,1},{0x802cd774,3},{0x802cd8b8,2},{0x802cde30,2},
}};

// Pinned Source df8815e75f3d79618466e01e0827c6165b93d11c:
// splat.yaml:9827-9853, blob d2690bd3d2cb0aeac5c648db20666c5a42e5ba45,
// establishes rainbow_code ROM 825e30 / VRAM 803466c0 and initialized
// .data 82a280..82adf0 (before .rodata). Select this overlay independently;
// it must never be combined with another course's overlapping VRAM mapping.
constexpr std::array<model_assets::Mapping, 1> rainbow_mappings{{
    {0x8034ab10,0x82a280,0x82adf0-0x82a280},
}};
// Complete finite graph/list declaration and use closure across main, Mew,
// Gate, 1037, evolution_controller, 1038 and intro at the pin above:
// mew/code.c:120-149, blob 40455edb782421b6add1b1107a61586854989633;
// gate/code.c:25,37, blob ee5d0b94feada6dcafa0df3a8b07c36baa5b10d1;
// pkmn_1037/code.c:26-40,73-295,
//   blob 03f323a788a58896889f78aed3e1efaa207711f7.
// The real main/Gate/controller helpers spawn those same families, not a
// substitute Beach graph. main.c:49-158 (bee9dab9845cd7ceb6d55c08d623b82ba7a775cf),
// gate/code.c:193-224 and evolution_controller/code.c:37-65
// (603722c803dea37983a783ca5200f8a8c2d96bca) introduce no other graph/list roots.
// 1038/code.c:1-65 (b0519e04b178d5ed8b2096713045a6948e342bc6) and
// intro.c:1-161 (370dd2fb22d8bed3ccc898fc8dd0b0d4c40d9c9d) add no roots.
// ROM-decoded setups ae7c/af64/b540/b5a4/b624 all have NULL setup graph;
// their actual state bodies are included in this declaration/use closure.
// Sentinel extents below come from each named ROM record, never a Top-N scan
// or inferred missing rows. There are 54 graph entries and two weighted entries.
constexpr std::array<detail::BehaviorRecord, 20> rainbow_graphs{{
    {0x8034ad50,3},{0x8034ad90,3},{0x8034add0,3},{0x8034ae10,1},
    {0x8034af3c,1},{0x8034b188,1},{0x8034b1a8,5},{0x8034b208,2},
    {0x8034b238,2},{0x8034b268,2},{0x8034b298,2},{0x8034b2c8,5},
    {0x8034b328,1},{0x8034b348,6},{0x8034b3b8,2},{0x8034b3e8,2},
    {0x8034b418,2},{0x8034b448,2},{0x8034b478,6},{0x8034b4e8,3},
}};
constexpr std::array<detail::BehaviorRecord, 1> rainbow_weighted_states{{
    {0x8034ae30,2},
}};
constexpr detail::BehaviorCatalog beach_catalog{
    0,beach_mappings,beach_graphs,beach_weighted_states,{}
};
constexpr detail::BehaviorCatalog rainbow_catalog{
    6,rainbow_mappings,rainbow_graphs,rainbow_weighted_states,{}
};
// Pinned GAME df8815e75f3d79618466e01e0827c6165b93d11c.
// Qualified 638k graph_root/weighted_root/option records, plus qualified 638n
// Valley 802ecb00. Preserve Source row order; these are data, not callbacks.
// common app_level/4FE330.c:23-25 declares the genuine empty cmd58 graph.
// Each course has its own initialized overlay mapping; never union overlays.
constexpr std::array<model_assets::Mapping, 2> tunnel_mappings{{
    {0x802edf00,0x5eafd0,0x551e0},
    {0x80388310,0x528720,0x90},
}};
constexpr std::array<detail::BehaviorRecord, 69> tunnel_graphs{{
    {0x802ee2a4,9},{0x802ee354,3},{0x802ee394,3},{0x802ee3d4,7},
    {0x802ee454,6},{0x802ee4c4,2},{0x802ee4f4,2},{0x802ee524,3},
    {0x802ee564,4},{0x802ee5cc,3},{0x802ee6b8,3},{0x802ee718,2},
    {0x802ee748,2},{0x802ee778,0},{0x802ee83c,4},{0x802ee88c,1},
    {0x802ee8b0,4},{0x802ee900,3},{0x802ee9f8,1},{0x802eea18,1},
    {0x802eea38,1},{0x802eea58,1},{0x802eeb28,1},{0x802eeb48,1},
    {0x802eeb68,1},{0x802eeb88,1},{0x802eec1c,2},{0x802eec4c,1},
    {0x802eec6c,1},{0x802eec8c,3},{0x802eeccc,1},{0x802eecec,3},
    {0x802eeed8,9},{0x802ef0a0,10},{0x802ef150,3},{0x802ef190,4},
    {0x802ef1e0,3},{0x802ef220,3},{0x802ef260,1},{0x802ef280,1},
    {0x802ef3ec,5},{0x802ef464,1},{0x802ef48c,5},{0x802ef4ec,2},
    {0x802ef51c,2},{0x802ef54c,4},{0x802ef59c,2},{0x802ef5cc,4},
    {0x802ef61c,2},{0x802ef64c,3},{0x802ef68c,1},{0x802ef6b0,4},
    {0x802ef778,0},{0x802ef788,1},{0x802efac8,1},{0x802efba8,1},
    {0x802efbc8,2},{0x802efc5c,1},{0x802efc7c,2},{0x802efcac,1},
    {0x802efccc,2},{0x802efe48,1},{0x802eff2c,1},{0x802eff4c,1},
    {0x802efffc,1},{0x802f009c,1},{0x802f0168,1},{0x802f0228,1},
    {0x80388324,0},
}};
constexpr std::array<detail::BehaviorRecord, 4> tunnel_weighted_states{{
    {0x802ee344,1},{0x802ee6f8,3},{0x802eef78,2},{0x802ef44c,2},
}};
constexpr std::array<detail::BehaviorRecord, 1> tunnel_counted_states{{
    {0x802ee120,2},
}};
constexpr detail::BehaviorCatalog tunnel_catalog{
    1,tunnel_mappings,tunnel_graphs,tunnel_weighted_states,tunnel_counted_states
};
constexpr std::array<model_assets::Mapping, 2> volcano_mappings{{
    {0x802e0c80,0x731e80,0x6d330},
    {0x80388310,0x528720,0x90},
}};
constexpr std::array<detail::BehaviorRecord, 82> volcano_graphs{{
    {0x802e10a4,1},{0x802e10c4,8},{0x802e1154,9},{0x802e11f4,9},
    {0x802e1294,8},{0x802e1324,4},{0x802e1374,4},{0x802e13c4,2},
    {0x802e13f4,3},{0x802e1434,4},{0x802e14a4,1},{0x802e15b8,8},
    {0x802e1648,7},{0x802e16c8,4},{0x802e1718,2},{0x802e1748,2},
    {0x802e1778,3},{0x802e17b8,3},{0x802e1818,5},{0x802e1878,4},
    {0x802e1a34,13},{0x802e1b14,8},{0x802e1ba4,9},{0x802e1c44,3},
    {0x802e1c84,3},{0x802e1cc4,4},{0x802e1d14,8},{0x802e1da4,4},
    {0x802e1df4,6},{0x802e1e90,1},{0x802e1eb0,1},{0x802e1ed0,1},
    {0x802e1ef0,4},{0x802e1f40,1},{0x802e1f60,1},{0x802e1f80,1},
    {0x802e2098,7},{0x802e2118,5},{0x802e2178,5},{0x802e21d8,2},
    {0x802e2208,2},{0x802e2238,3},{0x802e2278,5},{0x802e2304,3},
    {0x802e2444,10},{0x802e24f4,7},{0x802e2574,7},{0x802e25f4,2},
    {0x802e2624,2},{0x802e2654,3},{0x802e2694,3},{0x802e2804,10},
    {0x802e28b4,7},{0x802e2934,4},{0x802e2984,2},{0x802e29b4,2},
    {0x802e29e4,3},{0x802e2a24,3},{0x802e2ba0,3},{0x802e2be0,1},
    {0x802e2c00,3},{0x802e2ca4,5},{0x802e2d24,2},{0x802e2d54,4},
    {0x802e2da4,4},{0x802e2ec8,3},{0x802e2f08,1},{0x802e2fa4,1},
    {0x802e3038,2},{0x802e314c,1},{0x802e317c,1},{0x802e3230,0},
    {0x802e32a4,0},{0x802e3378,1},{0x802e3398,1},{0x802e3454,1},
    {0x802e3474,1},{0x802e354c,2},{0x802e3648,0},{0x802e3708,1},
    {0x802e3794,0},{0x80388324,0},
}};
constexpr std::array<detail::BehaviorRecord, 12> volcano_weighted_states{{
    {0x802e1484,3},{0x802e17f8,3},{0x802e1e64,4},{0x802e22d8,3},
    {0x802e26d4,4},{0x802e2a64,4},{0x802e2c40,5},{0x802e2c70,2},
    {0x802e2c88,2},{0x802e2d04,3},{0x802e2f28,2},{0x802e33b8,1},
}};
constexpr std::array<detail::BehaviorRecord, 1> volcano_counted_states{{
    {0x802e0ea4,2},
}};
constexpr detail::BehaviorCatalog volcano_catalog{
    2,volcano_mappings,volcano_graphs,volcano_weighted_states,volcano_counted_states
};
constexpr std::array<model_assets::Mapping, 2> river_mappings{{
    {0x802e2620,0x6ca400,0x5cee0},
    {0x80388310,0x528720,0x90},
}};
constexpr std::array<detail::BehaviorRecord, 59> river_graphs{{
    {0x802e2ac4,5},{0x802e2b3c,3},{0x802e2b7c,1},{0x802e2b9c,1},
    {0x802e2ca8,6},{0x802e2d40,3},{0x802e2d80,3},{0x802e2dc0,2},
    {0x802e2e58,2},{0x802e2e88,1},{0x802e2f34,0},{0x802e3028,1},
    {0x802e30e0,2},{0x802e324c,3},{0x802e328c,2},{0x802e32e8,1},
    {0x802e3494,3},{0x802e34d4,2},{0x802e351c,4},{0x802e3584,4},
    {0x802e35ec,1},{0x802e360c,1},{0x802e362c,1},{0x802e39b8,2},
    {0x802e39e8,4},{0x802e3a38,4},{0x802e3a88,4},{0x802e3ad8,4},
    {0x802e3b28,4},{0x802e3b78,4},{0x802e3c28,2},{0x802e3c58,2},
    {0x802e3d34,1},{0x802e3e40,2},{0x802e3e70,1},{0x802e3f6c,5},
    {0x802e3fcc,3},{0x802e400c,2},{0x802e403c,2},{0x802e406c,2},
    {0x802e4234,6},{0x802e42a4,1},{0x802e42c4,4},{0x802e4314,1},
    {0x802e4334,2},{0x802e4478,1},{0x802e4518,1},{0x802e45a8,1},
    {0x802e4638,1},{0x802e4838,7},{0x802e48b8,4},{0x802e4908,2},
    {0x802e4938,3},{0x802e49a0,3},{0x802e49e0,2},{0x802e4a10,2},
    {0x802e4a40,1},{0x802e4b14,1},{0x80388324,0},
}};
constexpr std::array<detail::BehaviorRecord, 13> river_weighted_states{{
    {0x802e2b24,2},{0x802e2d18,4},{0x802e3048,2},{0x802e32bc,3},
    {0x802e3474,3},{0x802e3504,2},{0x802e356c,2},{0x802e35d4,2},
    {0x802e3748,3},{0x802e3bc8,3},{0x802e3be8,3},{0x802e3c08,3},
    {0x802e4978,4},
}};
constexpr std::array<detail::BehaviorRecord, 1> river_counted_states{{
    {0x802e2908,3},
}};
constexpr detail::BehaviorCatalog river_catalog{
    3,river_mappings,river_graphs,river_weighted_states,river_counted_states
};
constexpr std::array<model_assets::Mapping, 2> cave_mappings{{
    {0x802c6150,0x648600,0x77fe0},
    {0x80388310,0x528720,0x90},
}};
constexpr std::array<detail::BehaviorRecord, 49> cave_graphs{{
    {0x802c65e8,8},{0x802c6678,7},{0x802c66f8,2},{0x802c6728,4},
    {0x802c6778,2},{0x802c67a8,3},{0x802c6924,3},{0x802c697c,2},
    {0x802c6aa8,2},{0x802c6ad8,2},{0x802c6b08,1},{0x802c6be0,2},
    {0x802c6c10,1},{0x802c6c30,1},{0x802c6d60,2},{0x802c6d90,1},
    {0x802c6db0,5},{0x802c6e10,4},{0x802c6e60,4},{0x802c6eb0,4},
    {0x802c6fd4,3},{0x802c70f0,2},{0x802c72b0,6},{0x802c7320,3},
    {0x802c7360,3},{0x802c73a0,3},{0x802c73e0,2},{0x802c741c,5},
    {0x802c748c,5},{0x802c74ec,2},{0x802c751c,2},{0x802c754c,2},
    {0x802c7614,1},{0x802c7634,3},{0x802c7674,1},{0x802c7694,3},
    {0x802c76d4,3},{0x802c7984,3},{0x802c79d0,7},{0x802c7a50,4},
    {0x802c7b50,1},{0x802c7c40,5},{0x802c7d94,3},{0x802c7ddc,3},
    {0x802c7e1c,2},{0x802c7e4c,2},{0x802c7e7c,2},{0x802c7fc8,1},
    {0x80388324,0},
}};
constexpr std::array<detail::BehaviorRecord, 5> cave_weighted_states{{
    {0x802c67e8,4},{0x802c6964,2},{0x802c6b28,3},{0x802c7014,2},
    {0x802c7120,2},
}};
constexpr std::array<detail::BehaviorRecord, 1> cave_counted_states{{
    {0x802c6368,2},
}};
constexpr detail::BehaviorCatalog cave_catalog{
    4,cave_mappings,cave_graphs,cave_weighted_states,cave_counted_states
};
constexpr std::array<model_assets::Mapping, 2> valley_mappings{{
    {0x802d2740,0x7abcd0,0x7a160},
    {0x80388310,0x528720,0x90},
}};
constexpr std::array<detail::BehaviorRecord, 63> valley_graphs{{
    {0x802d2b94,1},{0x802d2bb4,1},{0x802d2bd4,1},{0x802d2c10,2},
    {0x802d2c40,2},{0x802d2c70,2},{0x802d2d7c,2},{0x802d2dac,1},
    {0x802d2ed8,1},{0x802d2ef8,1},{0x802d2ff8,1},{0x802d3224,4},
    {0x802d3294,1},{0x802d32b4,1},{0x802d32d4,2},{0x802d3304,2},
    {0x802d3334,3},{0x802d3374,3},{0x802d33b4,2},{0x802d3568,9},
    {0x802d363c,2},{0x802d366c,2},{0x802d369c,2},{0x802d36cc,3},
    {0x802d370c,1},{0x802d372c,1},{0x802d374c,1},{0x802d376c,1},
    {0x802d378c,1},{0x802d3850,2},{0x802d3880,1},{0x802d38a0,3},
    {0x802d38e0,1},{0x802d3994,1},{0x802d39d0,1},{0x802d39f0,1},
    {0x802d3a10,1},{0x802d3b38,2},{0x802d3b68,2},{0x802d3b98,2},
    {0x802d3bc8,2},{0x802d3d24,5},{0x802d3da4,1},{0x802d3dc4,1},
    {0x802d3de4,2},{0x802d3e14,2},{0x802d3e44,4},{0x802d3e94,4},
    {0x802d3ee4,3},{0x802d3ffc,1},{0x802d401c,2},{0x802d404c,1},
    {0x802d4070,2},{0x802d40b0,1},{0x802d40d0,3},{0x802d41a8,1},
    {0x802ec658,1},{0x802ec718,1},{0x802ec898,1},{0x802ec8b8,1},
    {0x802ec8e4,2},{0x802ec9d0,1},{0x80388324,0},
}};
constexpr std::array<detail::BehaviorRecord, 12> valley_weighted_states{{
    {0x802d2b74,2},{0x802d2dcc,2},{0x802d2ec0,2},{0x802d30c0,3},
    {0x802d3274,3},{0x802d3608,5},{0x802d39b4,2},{0x802d3d84,3},
    {0x802d40a0,1},{0x802ec870,3},{0x802ec9f0,3},{0x802ecb00,2},
}};
constexpr std::array<detail::BehaviorRecord, 1> valley_counted_states{{
    {0x802d29b4,4},
}};
constexpr detail::BehaviorCatalog valley_catalog{
    5,valley_mappings,valley_graphs,valley_weighted_states,valley_counted_states
};
const detail::BehaviorCatalog& catalog_for(std::int32_t course_id) {
    switch (course_id) {
    case 0: return beach_catalog;
    case 1: return tunnel_catalog;
    case 2: return volcano_catalog;
    case 3: return river_catalog;
    case 4: return cave_catalog;
    case 5: return valley_catalog;
    case 6: return rainbow_catalog;
    default: throw std::invalid_argument("Unsupported actor behavior course catalog");
    }
}
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
std::int32_t signed_word(const rom::GuestView& wire, std::size_t offset) {
    return std::bit_cast<std::int32_t>(wire.u32(offset));
}
void add_weight(std::int32_t& total, std::int32_t weight) {
    require(weight>=0 && weight<=std::numeric_limits<std::int32_t>::max()-total,
            "Behavior weight cannot form source signed32 cumulative total");
    total+=weight;
}
InteractionGraph read_graph(const model_assets::Reader& reader, detail::BehaviorRecord record) {
    InteractionGraph graph{};
    graph.wire=reader.view(record.identity,(record.entries+1)*16);
    graph.entries.reserve(record.entries);
    for (std::size_t i=0;i<record.entries;++i) {
        const auto wire=reader.view(record.identity+static_cast<std::uint32_t>(i*16),16);
        const auto command=signed_word(wire,0);
        require(command!=58,"Behavior graph terminates before its admitted extent");
        // No command-specific policy or finite/range restrictions on values
        // the source may never consume. Preserve original bits and references.
        // These data identities are not native callback admission.
        graph.entries.push_back({wire,command,wire.u32(4),wire.f32(8),wire.u32(12)});
    }
    graph.terminator=reader.view(record.identity+static_cast<std::uint32_t>(record.entries*16),16);
    require(signed_word(graph.terminator,0)==58,
            "Behavior graph has no cmd58 at its admitted extent");
    return graph;
}
WeightedStates read_states(const model_assets::Reader& reader, detail::BehaviorRecord record) {
    WeightedStates states{};
    states.wire=reader.view(record.identity,(record.entries+1)*8);
    states.entries.reserve(record.entries);
    for (std::size_t i=0;i<record.entries;++i) {
        const auto wire=reader.view(record.identity+static_cast<std::uint32_t>(i*8),8);
        const auto state=wire.u32(4);
        require(state!=0,"Behavior state list terminates before its admitted extent");
        const auto weight=signed_word(wire,0);
        add_weight(states.total_weight,weight);
        states.entries.push_back({wire,weight,state});
    }
    states.terminator=reader.view(record.identity+static_cast<std::uint32_t>(record.entries*8),8);
    require(states.terminator.u32(4)==0,
            "Behavior state list has no NULL state at its admitted extent");
    require(states.total_weight>0,"Behavior state list has no positive source weight total");
    return states;
}
// Pinned include/common_structs.h:197-200 explicitly specifies RandomState2:
// u8 weight at 0, GObjFunc identity at 4, sizeof 8. Bytes 1..3 are padding,
// not a signed32 weight. tunnel/main.c:151-154 and volcano/main.c:149-152
// initialize two outcomes including NULL; cave/6401B0.c:17,58 and
// valley/79F1B0.c:12,186 declare/consume counts 2 and 4. River's declared
// array in river/6C05E0.c:18 is qualified by 638k's three consumed options.
// Counts are closed catalog facts; callback identities never authorize calls.
CountedStateOptions read_options(const model_assets::Reader& reader, detail::BehaviorRecord record) {
    CountedStateOptions options{};
    options.wire=reader.view(record.identity,record.entries*8);
    options.entries.reserve(record.entries);
    for (std::size_t i=0;i<record.entries;++i) {
        const auto wire=reader.view(record.identity+static_cast<std::uint32_t>(i*8),8);
        options.entries.push_back({wire,wire.u8(0),wire.u32(4)});
    }
    return options;
}
template<class T> const T& lookup(const std::vector<T>& values, std::uint32_t identity) {
    const auto found=std::lower_bound(values.begin(),values.end(),identity,
        [](const T& value,std::uint32_t key) { return value.wire.guest_address<key; });
    if (found==values.end() || found->wire.guest_address!=identity)
        throw std::out_of_range("Unknown admitted actor behavior identity");
    return *found;
}
} // namespace

BehaviorResources::BehaviorResources(const rom::Snapshot& snapshot, std::int32_t course_id)
    : snapshot_(&snapshot), source_data_(snapshot.canonical().data()),
      catalog_(&catalog_for(course_id)) {
    require(snapshot.canonical().size()==rom::us_size && snapshot.identity().size==rom::us_size &&
            snapshot.identity().sha1==admitted_sha1,"Behavior resources require admitted US Snapshot");
    const model_assets::Reader reader(snapshot,catalog_->mappings);
    graphs_.reserve(catalog_->graphs.size());
    weighted_states_.reserve(catalog_->weighted_states.size());
    counted_state_options_.reserve(catalog_->counted_state_options.size());
    for (const auto record:catalog_->counted_state_options) counted_state_options_.push_back(read_options(reader,record));
    for (const auto record:catalog_->graphs) graphs_.push_back(read_graph(reader,record));
    for (const auto record:catalog_->weighted_states) weighted_states_.push_back(read_states(reader,record));
}
std::int32_t BehaviorResources::course_id() const noexcept { return catalog_->course_id; }
void BehaviorResources::require_live() const {
    const auto bytes=snapshot_->canonical();
    require(bytes.size()==rom::us_size && bytes.data()==source_data_,
            "Behavior resources borrow moved-from Snapshot");
}
std::span<const InteractionGraph> BehaviorResources::graphs() const { require_live(); return graphs_; }
std::span<const WeightedStates> BehaviorResources::weighted_states() const { require_live(); return weighted_states_; }
const InteractionGraph& BehaviorResources::graph(std::uint32_t identity) const { require_live(); return lookup(graphs_,identity); }
const WeightedStates& BehaviorResources::weighted_states(std::uint32_t identity) const { require_live(); return lookup(weighted_states_,identity); }
std::span<const CountedStateOptions> BehaviorResources::counted_state_options() const { require_live(); return counted_state_options_; }
const CountedStateOptions& BehaviorResources::counted_state_options(std::uint32_t identity) const { require_live(); return lookup(counted_state_options_,identity); }
} // namespace isle::actor
