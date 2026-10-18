#include "game_state.hpp"

#include <exception>
#include <utility>

namespace isle::game {
bool VideoOffsets::translation(::isle::save::ScreenOffsets& out) const noexcept {
    if (horizontal_start != horizontal_end || vertical_start != vertical_end) return false;
    out = {horizontal_start, vertical_start};
    return true;
}
State::State(std::filesystem::path profile, std::uint32_t seed)
    : rng{seed}, profile_(std::move(profile)) {}
State::~State() = default;
bool State::initialize(std::uint32_t now) noexcept {
    if (attempted_) return initialized();
    attempted_ = true;
    const char* failure = nullptr;
    try {
        opened_ = ::isle::save::GameSession::open(profile_, now);
        if (!opened_) failure = "profile requires recovery or could not be opened; original evidence retained";
    } catch (const std::exception& e) {
        std::size_t n = 0;
        while (n + 1 < error_.size() && e.what()[n] != '\0') { error_[n] = e.what()[n]; ++n; }
        error_[n] = '\0';
    } catch (...) { failure = "profile initialization exception"; }
    if (failure != nullptr) {
        std::size_t n = 0;
        while (n + 1 < error_.size() && failure[n] != '\0') { error_[n] = failure[n]; ++n; }
        error_[n] = '\0';
    }
    return initialized();
}
bool State::initialization_attempted() const noexcept { return attempted_; }
bool State::initialized() const noexcept { return attempted_ && bool(opened_); }
const char* State::error() const noexcept { return error_.data(); }
const std::filesystem::path& State::profile_path() const noexcept { return profile_; }
const ::isle::save::GameOpenResult* State::save_initialization() const noexcept { return attempted_ ? &opened_ : nullptr; }
::isle::save::GameSession* State::save() noexcept { return initialized() ? &opened_.session : nullptr; }
const ::isle::save::GameSession* State::save() const noexcept { return initialized() ? &opened_.session : nullptr; }
const VideoOffsets& State::screen_offsets() const noexcept { return screen_offsets_; }
void State::set_screen_offsets(VideoOffsets offsets) noexcept { screen_offsets_ = offsets; }
void State::select_controller(const game_input::Snapshot& snapshot) noexcept {
    controller = snapshot.indices[0] == 0 ? 0 : -1;
}
void State::set_idle_script(const IslePlayerIdleScript* script) noexcept { manager_state_.idle_script = script; }
const IslePlayerIdleScript* State::idle_script() const noexcept { return manager_state_.idle_script; }
void State::add_scene_notices(std::uint32_t flags) noexcept { notices_ |= flags; }
bool State::consume_scene_notices(std::uint32_t flags) noexcept {
    const bool result = (notices_ & flags) != 0;
    notices_ &= ~flags;
    return result;
}
void State::clear_scene_notices() noexcept { notices_ = 0; }
std::uint32_t State::scene_notices() const noexcept { return notices_; }
void State::set_scene_latch(std::int32_t value) noexcept { manager_state_.latch = value; }
std::int32_t State::scene_latch() const noexcept { return manager_state_.latch; }
IsleApplicationManagerState& State::manager_state() noexcept { return manager_state_; }
} // namespace isle::game
