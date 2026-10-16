#ifndef ISLE_ACTOR_COLLISION_HPP
#define ISLE_ACTOR_COLLISION_HPP

#include "actor_owner.hpp"

namespace isle::actor::collision {
// Synchronous native leaves: no wait, source fault or owning automatic crosses
// the caller's C failure fence. The caller supplies its actual World and
// canonical source/model from the same native game. World/resources and all
// bindings remain live and excluded from mutation for the complete call.
OwnerStatus ground_query(world::World&, float x, float z, IsleActorGround&) noexcept;
// An ordinary source rejection succeeds with blocked=true. Errors preserve
// blocked, but a completed destination query has already updated currGround.
// A nonNULL forbiddenGround borrows forbiddenGroundCount readable floats;
// only entries through the first zero or match are consumed.
OwnerStatus step_walk(world::World&, IsleActorSource&, IsleGameModel*,
                      float velocity_x, float velocity_z, std::uint32_t flags,
                      bool& blocked) noexcept;
} // namespace isle::actor::collision

#endif
