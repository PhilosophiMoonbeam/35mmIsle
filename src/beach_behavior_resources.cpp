#include "beach_behavior_resources.hpp"
#include "model_asset_closure.hpp"

#include <algorithm>
#include <bit>
#include <limits>
#include <stdexcept>
#include <utility>

namespace isle::beach {
namespace {
constexpr std::array<std::uint8_t, 20> admitted_sha1{
    0xed,0xc7,0xc4,0x9c,0xc5,0x68,0xc0,0x45,0xfe,0x48,
    0xbe,0x0d,0x18,0x01,0x1c,0x30,0xf3,0x93,0xcb,0xaf
};
constexpr std::array<model_assets::Mapping, 2> mappings{{
    {0x802cbe10,0x563e80,0x22e0},
    {0x80388310,0x528720,0x90},
}};
// Complete pinned own-GAME declaration/reference closure, including the
// C-local Pidgey graph and Chansey's copied automatic-graph initializer.
// Catalog order is initialized guest identity order; entry order is untouched.
constexpr std::array<std::uint32_t, 71> graph_identities{
    0x802cc1d4,0x802cc234,0x802cc264,0x802cc294,
    0x802cc40c,0x802cc4c4,0x802cc4f4,0x802cc524,0x802cc564,
    0x802cc594,0x802cc5c4,0x802cc644,0x802cc6b4,0x802cc6e4,
    0x802cc714,0x802cc784,0x802cc7b4,0x802cc7e4,0x802cc804,
    0x802cc824,0x802cc854,0x802cc874,0x802cc8a4,0x802cc8d4,
    0x802cc904,0x802cc934,
    0x802cca10,0x802ccab8,0x802ccae8,0x802ccb28,0x802ccba8,
    0x802ccbf8,0x802ccc28,0x802ccc68,0x802ccca8,
    0x802ccd60,0x802cce1c,0x802ccf14,0x802ccf44,
    0x802cd010,0x802cd040,0x802cd064,0x802cd0b4,0x802cd0e4,
    0x802cd1e4,0x802cd214,
    0x802cd35c,0x802cd3f8,0x802cd438,0x802cd478,0x802cd4a8,
    0x802cd4e8,0x802cd518,
    0x802cd634,0x802cd694,0x802cd6c4,0x802cd6f4,0x802cd858,
    0x802cd9d8,0x802cdc60,0x802cdc80,0x802cdca0,
    0x802cdcd0,0x802cdd40,0x802cdd80,0x802cde48,0x802cde78,
    0x802cdef8,0x802cdf98,0x802cdfd8,0x80388324,
};
constexpr std::array<std::uint32_t, 8> weighted_identities{
    0x802cc2c4,0x802cc4ac,0x802ccaa0,0x802cd3dc,
    0x802cd764,0x802cd774,0x802cd8b8,0x802cde30,
};
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
InteractionGraph read_graph(const model_assets::Reader& reader, std::uint32_t identity) {
    InteractionGraph graph{};
    const auto available=reader.remaining_bytes(identity)/16;
    for (std::size_t i=0;i<available;++i) {
        const auto wire=reader.view(identity+static_cast<std::uint32_t>(i*16),16);
        const auto command=signed_word(wire,0);
        if (command==58) {
            graph.terminator=wire;
            graph.wire=reader.view(identity,(i+1)*16);
            return graph;
        }
        // Do not impose command-specific policy or finite/range restrictions on
        // a value that the source may never consume. Preserve its exact bits.
        graph.entries.push_back({wire,command,wire.u32(4),wire.f32(8),wire.u32(12)});
    }
    throw std::runtime_error("Behavior graph has no cmd58 in initialized mapping");
}
WeightedStates read_states(const model_assets::Reader& reader, std::uint32_t identity) {
    WeightedStates states{};
    const auto available=reader.remaining_bytes(identity)/8;
    for (std::size_t i=0;i<available;++i) {
        const auto wire=reader.view(identity+static_cast<std::uint32_t>(i*8),8);
        const auto state=wire.u32(4);
        if (!state) {
            require(states.total_weight>0,"Behavior state list has no positive source weight total");
            states.terminator=wire;
            states.wire=reader.view(identity,(i+1)*8);
            return states;
        }
        const auto weight=signed_word(wire,0);
        add_weight(states.total_weight,weight);
        states.entries.push_back({wire,weight,state});
    }
    throw std::runtime_error("Behavior state list has no NULL state in initialized mapping");
}
SceneChooser read_chooser(const model_assets::Reader& reader) {
    SceneChooser chooser{};
    chooser.wire=reader.view(0x802cc004,16);
    for (std::size_t i=0;i<chooser.entries.size();++i) {
        const auto wire=reader.view(0x802cc004+static_cast<std::uint32_t>(i*8),8);
        const auto weight=wire.u8(0);
        chooser.entries[i]={wire,weight,wire.u32(4)};
        add_weight(chooser.total_weight,weight);
    }
    require(chooser.total_weight>0,"Scene chooser has no positive source weight total");
    return chooser;
}
template<class T> const T& lookup(const std::vector<T>& values, std::uint32_t identity) {
    const auto found=std::lower_bound(values.begin(),values.end(),identity,
        [](const T& value,std::uint32_t key) { return value.wire.guest_address<key; });
    if (found==values.end() || found->wire.guest_address!=identity)
        throw std::out_of_range("Unknown admitted Beach behavior identity");
    return *found;
}
} // namespace

BehaviorResources::BehaviorResources(const rom::Snapshot& snapshot)
    : snapshot_(&snapshot), source_data_(snapshot.canonical().data()) {
    require(snapshot.canonical().size()==rom::us_size && snapshot.identity().size==rom::us_size &&
            snapshot.identity().sha1==admitted_sha1,"Behavior resources require admitted US Snapshot");
    const model_assets::Reader reader(snapshot,mappings);
    graphs_.reserve(graph_identities.size());
    weighted_states_.reserve(weighted_identities.size());
    for (const auto identity:graph_identities) graphs_.push_back(read_graph(reader,identity));
    for (const auto identity:weighted_identities) weighted_states_.push_back(read_states(reader,identity));
    scene_chooser_=read_chooser(reader);
}
void BehaviorResources::require_live() const {
    const auto bytes=snapshot_->canonical();
    require(bytes.size()==rom::us_size && bytes.data()==source_data_,
            "Behavior resources borrow moved-from Snapshot");
}
std::span<const InteractionGraph> BehaviorResources::graphs() const { require_live(); return graphs_; }
std::span<const WeightedStates> BehaviorResources::weighted_states() const { require_live(); return weighted_states_; }
const SceneChooser& BehaviorResources::scene_chooser() const { require_live(); return scene_chooser_; }
const InteractionGraph& BehaviorResources::graph(std::uint32_t identity) const { require_live(); return lookup(graphs_,identity); }
const WeightedStates& BehaviorResources::weighted_states(std::uint32_t identity) const { require_live(); return lookup(weighted_states_,identity); }
const SceneChooser& BehaviorResources::scene_chooser(std::uint32_t identity) const {
    require_live();
    if (identity!=scene_chooser_.wire.guest_address)
        throw std::out_of_range("Unknown admitted Beach scene chooser identity");
    return scene_chooser_;
}
} // namespace isle::beach
