#ifndef ISLE_ACTOR_INTERACTION_SERVICES_HPP
#define ISLE_ACTOR_INTERACTION_SERVICES_HPP

#include "actor_interaction_services.h"

namespace isle::scene { class Services; }
namespace isle::items { class Owner; }
namespace isle::world { class World; }
namespace isle::focus { class Detector; }
namespace isle::beach { class ActorResources; struct ActorInitialization; }

namespace isle::actor {
class Owner;
class BehaviorBindings;
struct InteractionSourceAccess;

// Borrowed stable context, not an interpreter, source registry, iterator, clock
// or pose cache. All referenced owners/resources and their unmoved Snapshots
// outlive every bound Actor and its source/native/graphics borrowers. Unbind at
// the parent's serialized outside-traversal boundary before destroying this
// context. Native join alone does not authorize destruction or resource reuse.
class InteractionServices final {
public:
    InteractionServices(scene::Services&, Owner&, items::Owner&, world::World&,
        focus::Detector&, const BehaviorBindings&, const beach::ActorResources&);
    InteractionServices(const InteractionServices&) = delete;
    InteractionServices& operator=(const InteractionServices&) = delete;
    InteractionServices(InteractionServices&&) = delete;
    InteractionServices& operator=(InteractionServices&&) = delete;

    bool matches(Owner&, scene::Services&, const beach::ActorResources&,
        world::World&, focus::Detector&) const noexcept;
private:
    friend struct InteractionSourceAccess;
    scene::Services& services_;
    Owner& actors_;
    items::Owner& items_;
    world::World& world_;
    focus::Detector& detector_;
    const BehaviorBindings& bindings_;
    const beach::ActorResources& resources_;
    const beach::ActorInitialization* common_ = nullptr;
    const beach::ActorInitialization* magikarp_ = nullptr;
    bool common_assets_ = false, magikarp_assets_ = false;
};
} // namespace isle::actor
#endif
