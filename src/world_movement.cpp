#include "world_movement.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

// GAME pin df8815e75f3d79618466e01e0827c6165b93d11c world/movement.c.
namespace isle::world {
Vec3 path_position(const menu::AnimationPath& p, float time) {
    const auto v = menu::animation::path_position(p,time); return {v.x,v.y,v.z};
}
Vec3 path_velocity(const menu::AnimationPath& p, float time) {
    const auto v = menu::animation::path_velocity(p,time); return {v.x,v.y,v.z};
}
Movement::Movement(World& world, MovementState& state, MovementCallbacks callbacks)
    : world_(world),state_(state),callbacks_(callbacks) {}
bool Movement::initialize(std::size_t index) {
    if (state_.speed_multiplier < 0 || !std::isfinite(state_.speed_multiplier) || index >= 13) return false;
    const float move = std::clamp(state_.move_time,0.0f,1.0f);
    const float cp = std::clamp(state_.control_time,0.0f,1.0f);
    auto* block = world_.enter_first(index);
    if (!block) return false;
    state_.block = block; state_.move_speed = 0;
    update_position(*block,move,cp);
    return true;
}
void Movement::update_position(Block& block, float move, float cp) {
    if (callbacks_.update) callbacks_.update(callbacks_.user,block.index,move);
    if (world_.retired()) return;
    const auto sample = world_.sample(block,move,cp);
    float speed = sample.speed;
    const auto* next = block.next;
    if (next && next->road_model && next->control_points && next->resource->road_model != no_index &&
        next->resource->metadata.road_guest && next->resource->metadata.movement_animation_guest &&
        next->resource->metadata.control_line_count >= 2) {
        const auto next_speed = world_.next_speed(*next,cp);
        speed += (next_speed-speed)*move;
    }
    const auto& descriptor = block.resource->metadata;
    const float cosine = std::cos(descriptor.yaw), sine = std::sin(descriptor.yaw);
    state_.position = {cosine*sample.position.x+sine*sample.position.z,sample.position.y,
                       -sine*sample.position.x+cosine*sample.position.z};
    state_.rotation = sample.rotation;
    state_.rotation.y += descriptor.yaw;
    if (descriptor.reversed) state_.rotation.y += 3.1415927f;
    state_.move_speed = state_.speed_multiplier*speed;
}
void Movement::update() {
    auto* block = state_.block;
    if (!block || world_.retired()) return;
    const float cp = std::clamp(state_.control_time,0.0f,1.0f);
    float move = std::clamp(state_.move_time,0.0f,1.0f);
    if (!std::isfinite(cp) || !std::isfinite(move) || !std::isfinite(state_.move_speed))
        throw std::invalid_argument("Nonfinite world movement state");
    if (state_.move_speed+move > 1) {
        if (callbacks_.exit_block) callbacks_.exit_block(callbacks_.user,*block);
        if (world_.retired()) { state_.block = nullptr; return; }
        if (!block->next || !block->next->control_points) {
            update_position(*block,move,cp); state_.move_speed = 0; return;
        }
        move = state_.move_speed+move-1;
        block = world_.enter_next();
        if (!block) return;
        state_.block = block;
        if (block->road_model && block->resource->metadata.movement_animation_guest) world_.reset_road(*block,0);
    } else move = state_.move_speed+move;
    update_position(*block,move,cp);
    state_.move_time = move;
}
} // namespace isle::world
