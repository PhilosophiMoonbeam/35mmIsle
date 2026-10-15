#ifndef ISLE_HOST_CLOCK_HPP
#define ISLE_HOST_CLOCK_HPP

#include <array>
#include <cstddef>
#include <cstdint>

namespace isle::host {
// GAME policy, not an OS-priority scheduler. The surrounding owner serializes
// access; rendering may retain a context only through its exact token.
struct ClockConfig {
    std::uint16_t update_interval = 1, draw_interval = 1;
    std::uint8_t contexts = 2;
    bool blocking_draw = true;
    std::uint32_t initial_frame = 0;
};
struct FrameToken {
    std::uint64_t owner = 0, epoch = 0, serial = 0;
    std::uint8_t context = 0;
    friend bool operator==(FrameToken, FrameToken) = default;
};
enum class ClockError { none, full, closed, invalid, stale, duplicate, not_ready, exhausted };
enum class Terminal { completed, failed, cancelled };
enum class ActionKind { wait, update, draw, paused, stopped };
struct ClockAction { ActionKind kind = ActionKind::wait; FrameToken token{}; };
struct ClockStats {
    std::uint64_t retraces = 0, dropped_retraces = 0, updates = 0;
    std::uint64_t submissions = 0, skipped_draws = 0, terminals = 0;
};
class GtlClock final {
  public:
    explicit GtlClock(ClockConfig);
    GtlClock(const GtlClock&) = delete;
    GtlClock& operator=(const GtlClock&) = delete;
    ClockError offer_retraces(std::uint64_t count = 1) noexcept;
    ClockAction poll() noexcept;
    ClockError finish_update() noexcept;
    // Cancellation/failure requires ceased update accesses; it is not progress.
    ClockError abandon_update() noexcept;
    // A full/closed transport leaves the produced frame owned by the producer.
    // Call accept_draw only after actual admission, or abandon_draw before any
    // consumer has obtained access. Never rebuild a rejected frame.
    ClockError accept_draw(FrameToken) noexcept;
    ClockError abandon_draw(FrameToken) noexcept;
    // Terminal means execution and all remaining accesses have actually ceased.
    ClockError offer_terminal(FrameToken, Terminal) noexcept;
    ClockError set_intervals(std::uint16_t update, std::uint16_t draw) noexcept;
    ClockError request_pause() noexcept;
    ClockError resume() noexcept;
    void request_stop() noexcept;
    // A new epoch requires resolved work, no producer and a stopped policy.
    ClockError reset(ClockConfig) noexcept;
    bool stopped() const noexcept;
    bool paused() const noexcept;
    bool stopping() const noexcept { return stop_requested_; }
    bool failed() const noexcept { return failed_; }
    std::uint32_t frame() const noexcept { return frame_; }
    std::uint32_t drawn_frames() const noexcept { return drawn_frames_; }
    std::size_t buffered_retraces() const noexcept { return ticks_; }
    ClockStats stats() const noexcept { return stats_; }
  private:
    enum class Phase { boundary, ticks, updating, context, drawing, paused, draining, stopped };
    struct Slot { FrameToken token{}; bool active = false, accepted = false, pending = false; };
    static bool valid(ClockConfig) noexcept;
    void drain_completions() noexcept;
    bool any_active() const noexcept;
    void exhaust() noexcept;
    ClockConfig config_;
    Phase phase_ = Phase::boundary;
    std::array<Slot, 2> slots_{};
    std::array<FrameToken, 3> completions_{};
    std::size_t completion_head_ = 0, completion_count_ = 0, ticks_ = 0;
    std::uint16_t received_ = 0;
    std::uint32_t frame_ = 0, drawn_frames_ = 0;
    std::uint64_t owner_ = 0, epoch_ = 1, next_serial_ = 1;
    ClockStats stats_{};
    bool pause_requested_ = false, stop_requested_ = false, failed_ = false;
};

// Exact rational elapsed-time accumulation. Frequency is explicit, never inferred
// from presentation callbacks. No repeated rounded-nanosecond-period drift.
class RetraceClock final {
  public:
    RetraceClock(std::uint32_t numerator_hz, std::uint32_t denominator_hz);
    std::uint64_t advance(std::uint64_t elapsed_nanoseconds);
    std::uint64_t nanoseconds_until_next() const noexcept;
    static std::uint64_t guest_ticks(std::uint64_t elapsed_nanoseconds) noexcept;
  private:
    std::uint32_t numerator_, denominator_;
    std::uint64_t previous_ns_ = 0, remainder_ = 0;
};
} // namespace isle::host
#endif
