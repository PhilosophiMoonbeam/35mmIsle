#include "beach_actor_scene.hpp"

#include <bit>
#include <cmath>
#include <stdexcept>

namespace isle::beach {
namespace {
constexpr std::array<std::uint8_t,20> admitted_sha1{
    0xed,0xc7,0xc4,0x9c,0xc5,0x68,0xc0,0x45,0xfe,0x48,
    0xbe,0x0d,0x18,0x01,0x1c,0x30,0xf3,0x93,0xcb,0xaf
};
constexpr std::array<model_assets::Mapping,1> mappings{{{0x802cbe10,0x563e80,0x22e0}}};
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
float finite(const rom::GuestView& wire, std::size_t offset) {
    const auto value=wire.f32(offset);
    require(std::isfinite(value),"Nonfinite Beach scene scalar");
    return value;
}
const ActorSceneInitialScalars& state_initials(const ActorSceneResources& resources, std::uint16_t actual_latch) {
    const auto& scalars=resources.initial_scalars();
    require(scalars.eevee_chansey_path_wire.guest_address==0x802cd3f4 &&
            scalars.eevee_chansey_path_identity==0,
            "Eevee/Chansey initial shared path differs from admitted original data");
    require(actual_latch==scalars.magikarp_active,
            "Beach scene requires the actual initialized Source Magikarp latch");
    return scalars;
}
} // namespace

ActorSceneResources::ActorSceneResources(const rom::Snapshot& snapshot)
    : snapshot_(&snapshot), source_data_(snapshot.canonical().data()) {
    require(snapshot.canonical().size()==rom::us_size && snapshot.identity().size==rom::us_size &&
            snapshot.identity().sha1==admitted_sha1,"Beach scene resources require admitted US Snapshot");
    const model_assets::Reader reader(snapshot,mappings);
    // Own GAME df8815e75f3d79618466e01e0827c6165b93d11c:
    // beach.h:102-104,122; 55FB80.c:89-204; 560620.c:9-232;
    // 563000.c:67-90. Values are actual admitted initialized ROM records, never
    // mutable standalone_data.c, guessed zeros or shared generic-course data.
    // Corresponding pinned blobs: 0aad5edbe49cd130449874b9b3b55941d3b73042,
    // c7371561341ca5f9fd4de5a38d87b257b54dc6d3,
    // fe72abeb5b19efe52bf00af9aa8871f52f3d4dd3,
    // 6767e7be06fbd22d95feb129ecd07c08db108d2e.
    // ROM SHA256 a1d5d816db7f8557ee04c35a011326d058b2c1fbca76b57b352b1d705a1ec1cc.
    auto& scalars=initial_scalars_;
    scalars.lapras_wire=reader.view(0x802ccdfc,12); // ROM564E6C..564E78
    scalars.pidgey_wire=reader.view(0x802cd060,4); // ROM5650D0..5650D4
    scalars.pikachu_wire=reader.view(0x802ce018,12); // ROM566088..566094
    scalars.eevee_chansey_path_wire=reader.view(0x802cd3f4,4); // ROM565464..565468
    scalars.eevee_chansey_path_identity=scalars.eevee_chansey_path_wire.u32(0);
    for (std::size_t i=0; i<scalars.lapras_counters.size(); ++i)
        scalars.lapras_counters[i]=std::bit_cast<std::int32_t>(scalars.lapras_wire.u32(i*4));
    scalars.pidgey_counter=std::bit_cast<std::int32_t>(scalars.pidgey_wire.u32(0));
    scalars.pikachu_temporary={finite(scalars.pikachu_wire,0),finite(scalars.pikachu_wire,4),finite(scalars.pikachu_wire,8)};
    // Same exact two-byte word hydrated by beach_lifecycle_source.c; the live
    // state borrows that Source field, not this immutable initial value.
    scalars.magikarp_latch_wire=reader.view(0x802cc014,2,2); // ROM564084..564086
    scalars.magikarp_active=scalars.magikarp_latch_wire.u16(0);
    auto& chooser=scene_chooser_;
    chooser.wire=reader.view(0x802cc004,16);
    for (std::size_t i=0; i<chooser.entries.size(); ++i) {
        const auto wire=reader.view(0x802cc004+static_cast<std::uint32_t>(i*8),8);
        const auto weight=wire.u8(0);
        chooser.entries[i]={wire,weight,wire.u32(4)};
        // Two unsigned-byte weights cannot overflow the source signed32 total.
        chooser.total_weight+=weight;
    }
    require(chooser.total_weight>0,"Scene chooser has no positive source weight total");
}
void ActorSceneResources::require_live() const {
    const auto bytes=snapshot_->canonical();
    require(bytes.size()==rom::us_size && bytes.data()==source_data_,"Beach scene resources borrow moved-from Snapshot");
}
const ActorSceneInitialScalars& ActorSceneResources::initial_scalars() const { require_live(); return initial_scalars_; }
const SceneChooser& ActorSceneResources::scene_chooser() const { require_live(); return scene_chooser_; }
const SceneChooser& ActorSceneResources::scene_chooser(std::uint32_t identity) const {
    require_live();
    if (identity!=scene_chooser_.wire.guest_address)
        throw std::out_of_range("Unknown admitted Beach scene chooser identity");
    return scene_chooser_;
}
ActorSceneState::ActorSceneState(const ActorSceneResources& scene_resources, std::uint16_t& actual_latch)
    : resources(scene_resources), lapras_counters(state_initials(scene_resources,actual_latch).lapras_counters),
      pidgey_counter(scene_resources.initial_scalars().pidgey_counter), magikarp_active(actual_latch),
      eevee_chansey_path(nullptr) {}
} // namespace isle::beach
