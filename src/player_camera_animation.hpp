#ifndef ISLE_PLAYER_CAMERA_ANIMATION_HPP
#define ISLE_PLAYER_CAMERA_ANIMATION_HPP
#include "35mmisle/engine.h"
#include "menu_animation.hpp"
#include "player_camera_animation_clip.h"

namespace isle::player::camera_animation {
struct Status {
    menu::animation::Status animation;
    IsleGameResult native = ISLE_GAME_OK;
    explicit operator bool() const noexcept { return bool(animation) && native == ISLE_GAME_OK; }
};
struct CreateResult;
// Borrows actual admitted Snapshot decoded animations/paths and actual native
// Game/Camera. All must outlive this driver and every source clip use. The caller
// excludes concurrent mutation and joins real source callbacks and graphics/
// completed-memory borrows BEFORE retire/destruction. No synthetic fences.
// Uses Runtime's admitted grammar unchanged: unsupported streams fail creation
// (including camera path channels/commands); no partial or fabricated programs.
// Runtime admission/bind/step failures are atomic under its documented contract.
// Native setter failures are returned, not rolled back: evaluation has occurred
// and earlier accepted channel writes remain, matching normal backend ownership.
// No scheduling: source animSet and animUpdate are separate calls; cutscene start
// performs set then exactly one update, only when its source list is non-NULL.
class NativeDriver final {
public:
    static CreateResult create(IsleGame*, IsleGameCamera*, menu::animation::Resources,
                               menu::animation::ExecutionCapacity) noexcept;
    ~NativeDriver() = default;
    NativeDriver(const NativeDriver&) = delete;
    NativeDriver& operator=(const NativeDriver&) = delete;
    Status clip(std::size_t admitted_animation, IslePlayerCameraAnimationClip&) const noexcept;
    Status set(const IslePlayerCameraAnimationClip*, float skip_frames) noexcept;
    Status set_speed(float) noexcept;
    Status update(std::uint32_t actual_object_flags) noexcept;
    const menu::animation::CameraState& state() const noexcept { return runtime_->camera(); }
    bool matches(IsleGame* game, IsleGameCamera* camera) const noexcept {
        return !retired_ && game_ == game && camera_ == camera && game && camera;
    }
    void retire() noexcept;
private:
    NativeDriver(IsleGame*, IsleGameCamera*, menu::animation::Resources,
                 std::unique_ptr<menu::animation::Runtime>, std::uint64_t) noexcept;
    IsleGame* game_;
    IsleGameCamera* camera_;
    menu::animation::Resources resources_;
    std::unique_ptr<menu::animation::Runtime> runtime_;
    std::uint64_t owner_;
    bool retired_ = false;
};
struct CreateResult {
    Status status;
    std::unique_ptr<NativeDriver> driver;
    explicit operator bool() const noexcept { return bool(status); }
};
} // namespace isle::player::camera_animation
#endif
