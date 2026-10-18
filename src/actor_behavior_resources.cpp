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
    std::span<const BehaviorRecord> graphs, weighted_states;
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
    0,beach_mappings,beach_graphs,beach_weighted_states
};
constexpr detail::BehaviorCatalog rainbow_catalog{
    6,rainbow_mappings,rainbow_graphs,rainbow_weighted_states
};
const detail::BehaviorCatalog& catalog_for(std::int32_t course_id) {
    switch (course_id) {
    case 0: return beach_catalog;
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
} // namespace isle::actor
