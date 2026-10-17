#ifndef ISLE_WORLD_MOVEMENT_HPP
#define ISLE_WORLD_MOVEMENT_HPP
#include "world_runtime.hpp"
#include "world_movement_state.h"

namespace isle::world {
struct MovementCallbacks {
    void* user = nullptr;
    // Both nullable in pinned Movement_Init. Invoked in source order, before
    // transition and before position sampling respectively.
    void (*exit_block)(void*, Block&) noexcept = nullptr;
    void (*update)(void*, std::size_t block_index, float move_time) noexcept = nullptr;
};
class Movement final {
public:
    Movement(World&, IsleWorldMovementState&, MovementCallbacks);
    bool initialize(std::size_t block_index);
    // Movement_Update owns carry-driven block entry as well as pose sampling.
    void update();
    // Movement_UpdatePos only samples this block. Explicit-entry callers keep
    // IsleWorldMovementState::block/move_time synchronized; no chain transition occurs.
    void update_position(Block&, float move_time, float control_time);
private:
    World& world_;
    IsleWorldMovementState& state_;
    MovementCallbacks callbacks_;
};
} // namespace isle::world
#endif
