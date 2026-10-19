#ifndef ISLE_TUNNEL_ACTOR_NATIVE_STATE_HPP
#define ISLE_TUNNEL_ACTOR_NATIVE_STATE_HPP
#include "world_collision.hpp"
#include "tunnel_actor_source.h"

struct IsleTunnelNativeCollision {
    const isle::world::CollisionModel* resource;
    const isle::world::collision::PreparedModels* prepared;
};
namespace isle::world {
// Implemented beside decode_collision, using the same qualified Reader.
CollisionModel tunnel_gate_collision(const rom::Snapshot&);
}
namespace isle::course {
// Owned cold by the actual NativeHost, never by an ambient current-course map.
// Resources, matrices and the opaque semantic borrow survive every Source call
// and are destroyed only after Host's actual Item/process/model/GPU joins.
struct TunnelActorNativeState final {
    world::CollisionModel gate_resource;
    world::collision::PreparedModels gate;
    IsleTunnelNativeCollision native_gate;
    explicit TunnelActorNativeState(const rom::Snapshot& snapshot)
        : gate_resource(world::tunnel_gate_collision(snapshot)),
          gate(std::span<const world::CollisionModel>(&gate_resource,1)),
          native_gate{&gate_resource,&gate} {}
    TunnelActorNativeState(const TunnelActorNativeState&) = delete;
    TunnelActorNativeState& operator=(const TunnelActorNativeState&) = delete;
};
}
#endif
