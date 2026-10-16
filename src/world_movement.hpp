#ifndef ISLE_WORLD_MOVEMENT_HPP
#define ISLE_WORLD_MOVEMENT_HPP
#include "world_runtime.hpp"

namespace isle::world {
struct MovementState {
    float move_time = 0, control_time = 0.5f;
    Block* block = nullptr;
    Vec3 position{}, rotation{};
    float move_speed = 0, speed_multiplier = 1;
};
struct MovementCallbacks {
    void* user = nullptr;
    // Both nullable in pinned Movement_Init. Invoked in source order, before
    // transition and before position sampling respectively.
    void (*exit_block)(void*, Block&) noexcept = nullptr;
    void (*update)(void*, std::size_t block_index, float move_time) noexcept = nullptr;
};
class Movement final {
public:
    Movement(World&, MovementState&, MovementCallbacks);
    bool initialize(std::size_t block_index);
    // Movement_Update owns carry-driven block entry as well as pose sampling.
    void update();
    // Movement_UpdatePos only samples this block. Explicit-entry callers keep
    // MovementState::block/move_time synchronized; no chain transition occurs.
    void update_position(Block&, float move_time, float control_time);
private:
    World& world_;
    MovementState& state_;
    MovementCallbacks callbacks_;
};
} // namespace isle::world
#endif
