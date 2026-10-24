#ifndef ISLE_HOST_PROGRESS_HPP
#define ISLE_HOST_PROGRESS_HPP

#include "game_host.hpp"
#include "game_audio_driver.hpp"
#include "host_clock.hpp"
#include <array>
#include <chrono>

namespace isle::host {
enum class ProgressStatus { running, stopped, failed };
// Borrowed native-only leaves. Neither callback may dispatch Source/OM, pump a
// Runtime, publish input, release a frame, or transfer control out of C++.
struct InputAcquisition {
    void* owner = nullptr;
    game_host::Input (*read)(void*) noexcept = nullptr;
};
struct InputAcknowledgement {
    void* owner = nullptr;
    bool (*acknowledge)(void*, const game_input::EventResult&) noexcept = nullptr;
};
// Native platform leaves. Direct consumers execute them on SDL main; the
// product supplies real main-owner transfers instead. A read retains its exact
// ReadToken until the actual acquisition completes. Callbacks never run GAME.
struct PlatformProgress {
    void* owner = nullptr;
    bool (*poll_events)(void*) noexcept = nullptr;
    game_host::Input (*read)(void*, game_input::ReadToken) noexcept = nullptr;
    game_host::Error (*error)(void*) noexcept = nullptr;
    const char* (*message)(void*) noexcept = nullptr;
};
PlatformProgress direct_platform(game_host::Host&) noexcept;
struct ProgressPolicy {
    bool request_input = true, complete_input = true;
    bool scene_active = false, offer_retraces = true, poll_events = true;
};
struct ProgressTick {
    std::uint64_t ns = 0, count = 0, read_skips = 0;
    game_input::ReadToken requested{};
    game_input::EventResult completion{};
    bool completed = false, completion_accepted = false;
};
// One serialized GAME service owner, shared by ordinary and nested native
// servicing. Construct before Runtime; bind its actual acknowledgement afterward.
// Platform, Driver, Service and acquisition storage must outlive this object.
// Clock and acknowledgement storage remain live until unbind(), after nested
// work has ceased and before Runtime/clock destruction. Unbinding does not reset
// the epoch, retrace fraction or VI ordinal.
class Progress final {
public:
    using Time = std::chrono::steady_clock;
    Progress(PlatformProgress, game_input::Service&, game_audio::Driver&, InputAcquisition = {});
    Progress(const Progress&) = delete;
    Progress& operator=(const Progress&) = delete;
    void bind(GtlClock&, InputAcknowledgement) noexcept;
    void unbind() noexcept;
    void set_policy(ProgressPolicy policy) noexcept { policy_ = policy; }
    ProgressPolicy policy() const noexcept { return policy_; }
    ProgressTick poll() noexcept;
    // Explicit completion also services an outstanding read during cancellation;
    // it never starts a read or fabricates a publication acknowledgement.
    ProgressTick complete_input() noexcept;
    // Only not-yet-offered genuine retraces observed in the bound active scene
    // may be offered. Full retains GTL's existing capacity/drop policy.
    ClockError offer_retraces(std::uint64_t count) noexcept;
    void request_stop() noexcept;
    ProgressStatus status() const noexcept { return status_; }
    const char* message() const noexcept { return message_.data(); }
    std::uint64_t ordinal() const noexcept { return ordinal_; }
    std::uint64_t elapsed_ns() const noexcept;
    static std::uint32_t raw(const ProgressTick& tick) noexcept;
    // Deadline observation is failure, never a native/GPU terminal. Clearing or
    // replacing a deadline cannot clear an already latched failure.
    void set_deadline(Time::time_point, const char* diagnostic) noexcept;
    void clear_deadline() noexcept { deadline_ = Time::time_point::max(); }
    bool deadline_expired() const noexcept;
    // Native pacing only: at most 1ms, shortened for real VI/deadline proximity.
    // Does not perform another poll or yield an original Source update.
    ProgressStatus idle() noexcept;
private:
    void fail(const char*) noexcept;
    void observe_deadline() noexcept;
    void acquire(ProgressTick&) noexcept;
    PlatformProgress platform_;
    game_input::Service& input_;
    game_audio::Driver& audio_;
    InputAcquisition acquisition_;
    GtlClock* clock_ = nullptr;
    InputAcknowledgement acknowledgement_{};
    ProgressPolicy policy_{};
    const Time::time_point epoch_ = Time::now();
    RetraceClock retraces_{2250000,37609};
    std::uint64_t ordinal_ = 0, offerable_ = 0, sampled_ns_ = 0;
    game_input::ReadToken live_read_{};
    ProgressStatus status_ = ProgressStatus::running;
    std::array<char,512> message_{}, deadline_message_{};
    Time::time_point deadline_ = Time::time_point::max();
};
} // namespace isle::host
#endif
