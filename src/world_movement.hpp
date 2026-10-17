#ifndef ISLE_WORLD_MOVEMENT_HPP
#define ISLE_WORLD_MOVEMENT_HPP
#include "world_runtime.hpp"
#include "world_movement_state.h"

namespace isle::world {
struct MovementCallbacks {
    void* user = nullptr;
    // Both nullable in pinned Movement_Init. Invoked in source order, before
    // transition and before position sampling respectively. False returns
    // normally and stops later sampling/publication; prior effects stay intact.
    bool (*exit_block)(void*, Block&) noexcept = nullptr;
    bool (*update)(void*, std::size_t block_index, float move_time) noexcept = nullptr;
};
class Movement final {
public:
    Movement(World&, IsleWorldMovementState&, MovementCallbacks);
    // False stops this operation without rolling back completed source effects.
    // Existing World/native admission exceptions still require a checked C++
    // caller; no native noexcept callback may let them escape its return leaf.
    bool initialize(std::size_t block_index);
    // Movement_Update owns carry-driven block entry as well as pose sampling.
    bool update();
    // Movement_UpdatePos only samples this block. Explicit-entry callers keep
    // IsleWorldMovementState::block/move_time synchronized; no chain transition occurs.
    bool update_position(Block&, float move_time, float control_time);
private:
    World& world_;
    IsleWorldMovementState& state_;
    MovementCallbacks callbacks_;
};
} // namespace isle::world
#endif
