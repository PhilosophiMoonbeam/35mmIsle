#include "beach_actor_scene.hpp"
#include "actor_behavior_resources.hpp"
#include "model_asset_closure.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <utility>

// Opt-in actual admitted ROM consumer. No source-text parsing, extracted data,
// guest calls, actor admission, FSM, device, GPU or runtime scheduling claim.
namespace b=isle::beach;
namespace r=isle::rom;
namespace {
void require(bool value,const char* message) { if (!value) throw std::runtime_error(message); }
template<class Exception=std::exception,class F> void rejects(F&& action,const char* message) {
    bool rejected=false;
    try { action(); } catch (const Exception&) { rejected=true; }
    require(rejected,message);
}
constexpr std::array<isle::model_assets::Mapping,2> mappings{{
    {0x802cbe10,0x563e80,0x22e0},{0x80388310,0x528720,0x90},
}};
// Independent OWN source-reference closure, grouped by complete source body.
// These are guest data identities, not a serialized scalar table oracle.
constexpr std::array<std::uint32_t,71> source_graphs{
    // 55D690.c
    0x802cc1d4,0x802cc234,0x802cc264,0x802cc294,
    // 55DCD0.c, including all behavior1..4 branches after line278.
    0x802cc40c,0x802cc4c4,0x802cc4f4,0x802cc524,0x802cc564,
    0x802cc594,0x802cc5c4,0x802cc644,0x802cc6b4,0x802cc6e4,
    0x802cc714,0x802cc784,0x802cc7b4,0x802cc7e4,0x802cc804,
    0x802cc824,0x802cc854,0x802cc874,0x802cc8a4,0x802cc8d4,
    0x802cc904,0x802cc934,
    // 55F350.c
    0x802cca10,0x802ccab8,0x802ccae8,0x802ccb28,0x802ccba8,
    0x802ccbf8,0x802ccc28,0x802ccc68,0x802ccca8,
    // 55FB80.c, 560320.c
    0x802ccd60,0x802cce1c,0x802ccf14,0x802ccf44,
    // 560620.c (802cd064 is declared locally at line122).
    0x802cd010,0x802cd040,0x802cd064,0x802cd0b4,0x802cd0e4,
    // 560E20.c, 5612B0.c (includes automatic-copy template802cd438).
    0x802cd1e4,0x802cd214,0x802cd35c,0x802cd3f8,0x802cd438,
    0x802cd478,0x802cd4a8,0x802cd4e8,0x802cd518,
    // 561B30.c, 562110.c, 562370.c, 562BC0.c
    0x802cd634,0x802cd694,0x802cd6c4,0x802cd6f4,0x802cd858,
    0x802cd9d8,0x802cdc60,0x802cdc80,0x802cdca0,
    // 563000.c and own common4FE330.c23-25.
    0x802cdcd0,0x802cdd40,0x802cdd80,0x802cde48,0x802cde78,
    0x802cdef8,0x802cdf98,0x802cdfd8,0x80388324,
};
constexpr std::array<std::uint32_t,8> source_states{
    0x802cc2c4,0x802cc4ac,0x802ccaa0,0x802cd3dc,
    0x802cd764,0x802cd774,0x802cd8b8,0x802cde30,
};
void borrowed(const r::Snapshot& snapshot,const r::GuestView& wire) {
    const auto bytes=snapshot.canonical();
    require(wire.rom_offset<=bytes.size() && wire.bytes.size()<=bytes.size()-wire.rom_offset &&
            wire.bytes.data()==bytes.data()+wire.rom_offset,"Behavior view lost exact Snapshot alias/extent");
    const auto mapping=std::find_if(mappings.begin(),mappings.end(),[&](const auto& m) {
        return wire.guest_address>=m.guest_begin && wire.guest_address-m.guest_begin<m.bytes;
    });
    require(mapping!=mappings.end() && wire.rom_offset==mapping->rom_begin+wire.guest_address-mapping->guest_begin &&
            wire.bytes.size()<=mapping->bytes-(wire.guest_address-mapping->guest_begin),"Behavior view escaped admitted mapping");
}
void consume_graph(const isle::actor::BehaviorResources& resources,const r::Snapshot& snapshot,
                   const isle::model_assets::Reader& reader,std::uint32_t identity) {
    const auto& graph=resources.graph(identity);
    require(&graph==&resources.graph(identity),"Graph identity lookup is not stable");
    borrowed(snapshot,graph.wire); borrowed(snapshot,graph.terminator);
    const auto raw=reader.view(identity,graph.wire.bytes.size());
    require(raw.bytes.data()==graph.wire.bytes.data() && graph.wire.bytes.size()==(graph.entries.size()+1)*16 &&
            graph.terminator.guest_address==identity+graph.entries.size()*16 && graph.terminator.u32(0)==58 &&
            graph.terminator.bytes.data()==raw.bytes.data()+graph.entries.size()*16,"Graph terminal/span ownership mismatch");
    // Keep each original bounded record available to future source consumers.
    // This data-only smoke does not implement or qualify command predicates.
    for (std::size_t i=0;i<graph.entries.size();++i) {
        const auto& entry=graph.entries[i]; borrowed(snapshot,entry.wire);
        require(entry.wire.bytes.size()==16 && entry.wire.bytes.data()==raw.bytes.data()+i*16 &&
                entry.command!=58,"Live graph record aliases its terminal or other source storage");
    }
    rejects<std::out_of_range>([&] { static_cast<void>(resources.graph(identity+1)); },"Misaligned graph identity accepted");
    rejects<std::out_of_range>([&] { static_cast<void>(resources.graph(identity+16)); },"Interior/noncatalog graph record accepted");
    if (!graph.entries.empty())
        rejects<std::out_of_range>([&] { static_cast<void>(resources.graph(graph.terminator.guest_address)); },"Graph terminal mistaken for admitted graph head");
}
void consume_states(const isle::actor::BehaviorResources& resources,const r::Snapshot& snapshot,
                    const isle::model_assets::Reader& reader,std::uint32_t identity) {
    const auto& states=resources.weighted_states(identity);
    require(&states==&resources.weighted_states(identity),"State-list lookup is not stable");
    borrowed(snapshot,states.wire); borrowed(snapshot,states.terminator);
    const auto raw=reader.view(identity,states.wire.bytes.size());
    require(states.wire.bytes.size()==(states.entries.size()+1)*8 &&
            states.terminator.bytes.data()==raw.bytes.data()+states.entries.size()*8 &&
            states.terminator.guest_address==identity+states.entries.size()*8 && states.terminator.u32(4)==0,
            "State-list sentinel extent or NULL identity changed");
    std::int32_t cumulative=0;
    for (std::size_t i=0;i<states.entries.size();++i) {
        const auto& entry=states.entries[i]; borrowed(snapshot,entry.wire);
        require(entry.wire.bytes.size()==8 && entry.wire.bytes.data()==raw.bytes.data()+i*8 &&
                entry.state_identity && entry.weight>=0,"Live weighted record aliases its terminal or other source storage");
        cumulative+=entry.weight;
    }
    require(cumulative==states.total_weight && cumulative>0,"Source random total changed");
    rejects<std::out_of_range>([&] { static_cast<void>(resources.weighted_states(identity+4)); },"Interior state identity accepted");
}
void consume_chooser(const isle::beach::ActorSceneResources& resources,const r::Snapshot& snapshot,
                     const isle::model_assets::Reader& reader) {
    const auto& chooser=resources.scene_chooser(0x802cc004);
    require(&chooser==&resources.scene_chooser(),"Scene chooser lookup is not stable");
    borrowed(snapshot,chooser.wire);
    const auto raw=reader.view(0x802cc004,16);
    require(chooser.wire.bytes.size()==16 && chooser.wire.bytes.data()==raw.bytes.data(),"Scene chooser fixed source extent changed");
    std::int32_t total=0;
    bool null_choice=false;
    for (std::size_t i=0;i<chooser.entries.size();++i) {
        const auto& entry=chooser.entries[i]; borrowed(snapshot,entry.wire);
        require(entry.wire.bytes.size()==8 && entry.wire.bytes.data()==raw.bytes.data()+i*8,
                "Fixed scene choice lost its original bounded record");
        null_choice|=entry.weight && !entry.function_identity;
        total+=entry.weight;
    }
    require(total==chooser.total_weight && total>0 && null_choice,"Actual scene NULL no-spawn choice was discarded");
}
void run(const char* filename) {
    auto snapshot=r::read_snapshot(filename);
    isle::actor::BehaviorResources resources(snapshot,0);
    isle::beach::ActorSceneResources sceneData(snapshot);
    const isle::model_assets::Reader reader(snapshot,mappings);
    for (std::size_t i=0;i<source_graphs.size();++i) {
        require(&resources.graph(source_graphs[i])==&resources.graphs()[i],"Graph catalog identity/order differs from source closure");
        consume_graph(resources,snapshot,reader,source_graphs[i]);
    }
    for (std::size_t i=0;i<source_states.size();++i) {
        require(&resources.weighted_states(source_states[i])==&resources.weighted_states()[i],"Weighted catalog identity/order differs from source closure");
        consume_states(resources,snapshot,reader,source_states[i]);
    }
    consume_chooser(sceneData,snapshot,reader);
    require(resources.graph(0x80388324).entries.empty(),"Own common cmd58-only graph gained executable entries");
    require(resources.graph(0x802cd438).wire.bytes.size()==4*16,"Chansey copied graph template lost its exact source extent");
    for (const auto identity:std::array<std::uint32_t,8>{0,1,0x802cbe10,0x802cc004,0x802cc2f0,0x802ce0f0,0x80388310,0xffffffff}) {
        rejects<std::out_of_range>([&] { static_cast<void>(resources.graph(identity)); },"Unknown graph identity accepted");
        rejects<std::out_of_range>([&] { static_cast<void>(resources.weighted_states(identity)); },"Unknown weighted identity accepted");
    }
    rejects<std::out_of_range>([&] { static_cast<void>(sceneData.scene_chooser(0)); },"NULL scene identity accepted");
    rejects<std::out_of_range>([&] { static_cast<void>(sceneData.scene_chooser(0x802cc00c)); },"Interior scene choice mistaken for chooser");
    // Borrowing survives repeat lookups and construction of another owner.
    const auto* graph=&resources.graph(0x802cd438);
    const auto* alias=graph->wire.bytes.data();
    isle::actor::BehaviorResources other(snapshot,0);
    require(other.graph(0x802cd438).wire.bytes.data()==alias && &resources.graph(0x802cd438)==graph,
            "Independent metadata owners copied/moved original initialized bytes");
    auto moved=std::move(snapshot);
    rejects([&] { static_cast<void>(resources.graph(0x802cc1d4)); },"Moved Snapshot graph lookup accepted");
    rejects([&] { isle::actor::BehaviorResources invalid(snapshot,0); },"Construction from moved-from Snapshot accepted");
    rejects([&] { other.require_live(); },"Second owner's moved Snapshot lifetime accepted");
    isle::actor::BehaviorResources rebound(moved,0);
    require(rebound.graph(0x802cd438).wire.bytes.data()==alias,"New owner failed to borrow moved-to Snapshot storage");
}
} // namespace
int main(int argc,char** argv) {
    if (argc!=2) { std::cerr<<"usage: beach-behavior-resources-rom-smoke <admitted-original.z64>\n"; return EXIT_FAILURE; }
    try { run(argv[1]); std::cout<<"Admitted Beach behavior data, bounded source records, weighted totals, NULL scene choice and borrowed lifetimes passed\n"; return EXIT_SUCCESS; }
    catch (const std::exception& error) { std::cerr<<error.what()<<'\n'; return EXIT_FAILURE; }
}
