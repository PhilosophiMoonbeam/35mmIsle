#ifndef ISLE_BEACH_MAGIKARP_SCENE_HPP
#define ISLE_BEACH_MAGIKARP_SCENE_HPP
#include "actor_owner.hpp"
#include "beach_actor_scene.hpp"

namespace isle::beach {
// Genuine native GObj/data.dobj identity. Root node-local position represents
// source DObj.position; actor placement/AUX, world matrices and GPU are NOT it.
struct MagikarpTrigger {
    IsleGameObject* object = nullptr;
    IsleGameModel* model = nullptr;
};
// Source55C110.c leaves path/behavior unwritten but the factory reads them.
// The native helper deliberately initializes NULL path/behavior0; equivalence
// to the original uninitialized automatic storage contents is UNPROVEN.
enum class MagikarpSceneError { none, resources, native, factory, postcopy };
enum class MagikarpSceneOutcome { filtered, null_choice, already_active, spawned };
struct MagikarpSceneResult {
    MagikarpSceneError error = MagikarpSceneError::none;
    const char* detail = "";
    MagikarpSceneOutcome outcome = MagikarpSceneOutcome::filtered;
    std::int32_t draw = -1;
    std::size_t choice = actor::actor_no_index;
    actor::SpawnResult actor{{actor::OwnerError::admission,"scene did not invoke actor factory"},{},nullptr};
    explicit operator bool() const noexcept { return error == MagikarpSceneError::none; }
};
// Bounded source55C110.c10-65 only: the caller owns actual trigger/player
// construction, GroundResult and event timing. No Item physics/event producer
// is inferred. All references must be the SAME Services/resources/World used
// to construct Owner and outlive this object. Owner's ordinary native/GPU
// exclusion applies. Resources and their original Snapshot remain unmoved/live.
class MagikarpScene final {
public:
    MagikarpScene(scene::Services&, actor::Owner&, world::World&,
                  const actor::Resources&, ActorSceneState&) noexcept;
    // Direct source helper: never reads or writes the scene active latch.
    MagikarpSceneResult spawn_at(MagikarpTrigger) noexcept;
    // Reads canonical Owner.player AUX only on the matching-surface branch;
    // consumes the application RNG before inspecting the canonical active latch.
    MagikarpSceneResult choose(MagikarpTrigger, const world::GroundResult&) noexcept;
private:
    scene::Services& services_;
    actor::Owner& owner_;
    world::World& world_;
    const actor::Resources& actors_;
    ActorSceneState& state_;
};
} // namespace isle::beach
#endif
