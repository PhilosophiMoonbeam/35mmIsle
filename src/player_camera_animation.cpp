#include "player_camera_animation.hpp"
#include <atomic>
#include <limits>
#include <new>
#include <utility>

namespace isle::player::camera_animation {
namespace a = menu::animation;
NativeDriver::NativeDriver(IsleGame* game, IsleGameCamera* camera, a::Resources resources,
                          std::unique_ptr<a::Runtime> runtime, std::uint64_t owner) noexcept
    : game_(game), camera_(camera), resources_(resources), runtime_(std::move(runtime)), owner_(owner) {}
CreateResult NativeDriver::create(IsleGame* game, IsleGameCamera* camera, a::Resources resources,
                                  a::ExecutionCapacity capacity) noexcept {
    const IsleGameCameraParameters* view = nullptr;
    auto native = isle_game_camera_parameters(game,camera,&view);
    if (native != ISLE_GAME_OK) return {{{},native},nullptr};
    float roll = 0;
    native = isle_game_camera_roll(game,camera,&roll);
    if (native != ISLE_GAME_OK) return {{{},native},nullptr};
    a::CameraState initial;
    initial.eye = {view->look_at.eye[0],view->look_at.eye[1],view->look_at.eye[2]};
    initial.at = {view->look_at.at[0],view->look_at.at[1],view->look_at.at[2]};
    initial.roll = roll;
    initial.fov_y = view->perspective.fovy;
    auto made = a::Runtime::create(resources,{},initial,capacity);
    if (!made) return {{made.status,ISLE_GAME_OK},nullptr};
    static std::atomic<std::uint64_t> next{1};
    auto owner = next.load(std::memory_order_relaxed);
    do {
        if (owner == std::numeric_limits<std::uint64_t>::max())
            return {{{a::Error::exhausted,0},ISLE_GAME_OK},nullptr};
    } while (!next.compare_exchange_weak(owner,owner+1,std::memory_order_relaxed));
    auto* driver = new (std::nothrow) NativeDriver(game,camera,resources,std::move(made.runtime),owner);
    if (!driver) return {{{a::Error::allocation,0},ISLE_GAME_OK},nullptr};
    return {{},std::unique_ptr<NativeDriver>(driver)};
}
Status NativeDriver::clip(std::size_t index, IslePlayerCameraAnimationClip& output) const noexcept {
    if (retired_) return {{a::Error::retired,0}};
    if (index >= resources_.animations.size() || resources_.animations[index].kind != menu::AnimationKind::camera)
        return {{a::Error::handle,0}};
    output = {owner_,index};
    return {};
}
Status NativeDriver::set(const IslePlayerCameraAnimationClip* clip, float skip) noexcept {
    if (retired_) return {{a::Error::retired,0}};
    if (clip && (clip->owner != owner_ || clip->index >= resources_.animations.size() ||
        resources_.animations[static_cast<std::size_t>(clip->index)].kind != menu::AnimationKind::camera))
        return {{a::Error::handle,0}};
    return {runtime_->bind_camera(clip ? static_cast<std::size_t>(clip->index) : menu::no_index,skip)};
}
Status NativeDriver::set_speed(float speed) noexcept { return {runtime_->set_camera_speed(speed)}; }
Status NativeDriver::update(std::uint32_t flags) noexcept {
    if (retired_) return {{a::Error::retired,0}};
    // Borrow afresh before evaluating; no cached native camera mirrors. Invalid
    // association therefore cannot advance source clocks.
    const IsleGameCameraParameters* view = nullptr;
    auto native = isle_game_camera_parameters(game_,camera_,&view);
    if (native != ISLE_GAME_OK) return {{},native};
    const auto stepped = runtime_->step_camera(flags);
    if (!stepped) return {stepped};
    const auto& value = runtime_->camera();
    const auto mask = value.written_channels;
    if (mask & 0x77u) {
        auto look = view->look_at;
        const float eye[] = {value.eye.x,value.eye.y,value.eye.z};
        const float at[] = {value.at.x,value.at.y,value.at.z};
        for (unsigned i = 0; i < 3; ++i) {
            if (mask & (1u << i)) look.eye[i] = eye[i];
            if (mask & (1u << (i+4))) look.at[i] = at[i];
        }
        native = isle_game_camera_set_look_at(game_,camera_,&look);
        if (native != ISLE_GAME_OK) return {{},native};
    }
    if (mask & (1u << 8)) {
        native = isle_game_camera_set_roll(game_,camera_,value.roll);
        if (native != ISLE_GAME_OK) return {{},native};
    }
    if (mask & (1u << 9)) {
        auto perspective = view->perspective;
        perspective.fovy = value.fov_y;
        native = isle_game_camera_set_perspective(game_,camera_,&perspective);
        if (native != ISLE_GAME_OK) return {{},native};
    }
    return {};
}
void NativeDriver::retire() noexcept {
    runtime_->retire();
    retired_ = true;
}
} // namespace isle::player::camera_animation
