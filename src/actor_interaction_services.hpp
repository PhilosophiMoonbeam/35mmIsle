#ifndef ISLE_ACTOR_INTERACTION_SERVICES_HPP
#define ISLE_ACTOR_INTERACTION_SERVICES_HPP

#include "actor_interaction_services.h"
#include <array>

struct IsleRainbowActorSource;
struct IsleRainbowActorServices;
namespace isle::scene { class Services; }
namespace isle::items { class Owner; }
namespace isle::world { class World; }
namespace isle::focus { class Detector; }

namespace isle::actor {
class Owner;
class BehaviorBindings;
class Resources;
struct ActorInitialization;
struct InteractionSourceAccess;

// Borrowed stable context, not an interpreter, source registry, iterator, clock
// or pose cache. All referenced owners/resources and their unmoved Snapshots
// outlive every bound Actor and its source/native/graphics borrowers. Unbind at
// the parent's serialized outside-traversal boundary before destroying this
// context. Native join alone does not authorize destruction or resource reuse.
class InteractionServices final {
public:
    // Borrow the Owner's sole typed course context. Rainbow construction is
    // after real Actor Source activation and before original Source init; it
    // publishes nothing and does not require a current native World block yet.
    InteractionServices(scene::Services&, Owner&, items::Owner&, world::World&,
        focus::Detector&, const BehaviorBindings&, const Resources&);
    InteractionServices(const InteractionServices&) = delete;
    InteractionServices& operator=(const InteractionServices&) = delete;
    InteractionServices(InteractionServices&&) = delete;
    InteractionServices& operator=(InteractionServices&&) = delete;

    bool matches(Owner&, scene::Services&, const Resources&,
        world::World&, focus::Detector&) const noexcept;
    // Complete native course/family closure, checked before factory allocation/events.
    // path is NULL or a borrowed canonical World path; no Source is fabricated.
    bool admits_initial_species(const ActorInitialization&, const void* path) noexcept;
    bool admits_ending(const ActorInitialization&) noexcept;
    // Recheck dynamic liveness/identity against the constructor's immutable
    // closure proof. Does not require current World publication.
    bool admits_source() const noexcept;
private:
    friend struct InteractionSourceAccess;
    scene::Services& services_;
    Owner& actors_;
    items::Owner& items_;
    world::World& world_;
    focus::Detector& detector_;
    const BehaviorBindings& bindings_;
    const Resources& resources_;
    const ActorInitialization* common_ = nullptr;
    const ActorInitialization* magikarp_ = nullptr;
    const ActorInitialization* butterfree_ = nullptr;
    const ActorInitialization* doduo_ = nullptr;
    const ActorInitialization* lapras_ = nullptr;
    const ActorInitialization* pidgey_ = nullptr;
    const ActorInitialization* pikachu_ = nullptr;
    const ActorInitialization* sign_ = nullptr;
    bool common_assets_ = false, magikarp_assets_ = false;
    bool butterfree_assets_ = false, doduo_assets_ = false;
    bool lapras_assets_ = false, pidgey_assets_ = false, pikachu_assets_ = false;
    bool sign_assets_ = false;
    // The remaining original families share immutable admission metadata.
    std::array<const ActorInitialization*, 8> remaining_{};
    std::array<bool, 8> remaining_assets_{};
    // Immutable course/provider/resource/table proof is performed only cold.
    // References above retain exact resource/binding Snapshot ownership; live
    // checks never cache FSM, paths, native membership or capacity availability.
    IsleRainbowActorSource* rainbow_source_ = nullptr;
    const IsleRainbowActorServices* rainbow_services_ = nullptr;
    bool rainbow_closed_ = false;
    std::array<const ActorInitialization*, 5> rainbow_initializations_{};
    bool prove_rainbow_closure();
    bool admits_rainbow(const void* path) const;
};
} // namespace isle::actor
#endif
