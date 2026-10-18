#ifndef ISLE_BEACH_ACTOR_SCENE_HPP
#define ISLE_BEACH_ACTOR_SCENE_HPP

#include "model_asset_closure.hpp"

namespace isle::beach {
// Initial shared Beach scene facts only; mutable values have one scene owner.
// Lapras order is D_beach_802CCDFC, D_beach_802CCE00, D_beach_802CCE04.
struct ActorSceneInitialScalars {
    rom::GuestView lapras_wire, pidgey_wire, pikachu_wire;
    std::array<std::int32_t, 3> lapras_counters;
    std::int32_t pidgey_counter;
    menu::Vec3 pikachu_temporary;
    rom::GuestView eevee_chansey_path_wire;
    std::uint32_t eevee_chansey_path_identity;
    // Actual source uint16 D_beach_802CC014, not a native bool shadow.
    rom::GuestView magikarp_latch_wire;
    std::uint16_t magikarp_active;
};
struct SceneChoice {
    rom::GuestView wire;
    std::uint8_t weight = 0;
    // NULL is a weighted no-spawn choice, not a terminator.
    std::uint32_t function_identity = 0;
};
struct SceneChooser {
    rom::GuestView wire;
    std::array<SceneChoice, 2> entries;
    std::int32_t total_weight = 0;
};
// Beach-only initialized data. Neither neutral resource owner constructs this
// implicitly, and Rainbow never reads any of these guest identities.
class ActorSceneResources final {
public:
    explicit ActorSceneResources(const rom::Snapshot&);
    ActorSceneResources(rom::Snapshot&&) = delete;
    ActorSceneResources(const ActorSceneResources&) = delete;
    ActorSceneResources& operator=(const ActorSceneResources&) = delete;
    ActorSceneResources(ActorSceneResources&&) = delete;
    ActorSceneResources& operator=(ActorSceneResources&&) = delete;
    const ActorSceneInitialScalars& initial_scalars() const;
    const SceneChooser& scene_chooser() const;
    const SceneChooser& scene_chooser(std::uint32_t identity) const;
    void require_live() const;
private:
    const rom::Snapshot* snapshot_;
    const std::byte* source_data_;
    ActorSceneInitialScalars initial_scalars_;
    SceneChooser scene_chooser_;
};
// Hydrate counters/path exactly once per scene. The real lifecycle Source owns
// its latch; this state borrows that same word, never copies or shadows it.
// Keep this object, its resources and actual_latch live through all borrowers.
class ActorSceneState final {
public:
    explicit ActorSceneState(const ActorSceneResources&, std::uint16_t& actual_latch);
    ActorSceneState(ActorSceneResources&&, std::uint16_t&) = delete;
    ActorSceneState(const ActorSceneState&) = delete;
    ActorSceneState& operator=(const ActorSceneState&) = delete;
    ActorSceneState(ActorSceneState&&) = delete;
    ActorSceneState& operator=(ActorSceneState&&) = delete;
    const ActorSceneResources& resources;
    std::array<std::int32_t, 3> lapras_counters;
    std::int32_t pidgey_counter;
    std::uint16_t& magikarp_active;
    const void* eevee_chansey_path;
};
} // namespace isle::beach
#endif
