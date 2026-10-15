#ifndef ISLE_REQUIRED_WORK_HPP
#define ISLE_REQUIRED_WORK_HPP

#include "host_clock.hpp"

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <span>

namespace isle::rsp_probe { class Capture; }

namespace isle::host {
struct RequiredToken {
    std::uint64_t owner = 0, epoch = 0, serial = 0;
    friend bool operator==(RequiredToken, RequiredToken) = default;
};
struct RequiredRequest {
    RequiredToken token{};
    const isle::rsp_probe::Capture* capture = nullptr;
};
// Read-only word-swapped32 memory: canonical byte address A is at A ^ 3.
// Main retains the actual backend lease. These spans grant no ownership and may
// be accessed only after successful wait_ready and before release_result.
struct RequiredMemoryView {
    std::span<const std::byte> rdram, hidden;
};
struct RequiredRetirement {
    RequiredToken token{};
    Terminal outcome = Terminal::cancelled;
};
// Lifetime totals survive reset. Outcome totals count fully finished retirement,
// not submission, GPU readiness, or CPU acknowledgement. Waiter counts describe
// threads currently inside the channel's condition-variable waits.
struct RequiredStats {
    std::uint64_t submissions = 0, gpu_ready = 0, cpu_borrows = 0;
    std::uint64_t retirements = 0, completed = 0, failed = 0, cancelled = 0;
    std::size_t capacity_waiters = 0, result_waiters = 0;
};

// One fixed slot, with no per-operation allocation, capture copies, callbacks,
// or implicit backend ownership. Main performs take/publish/take_retirement/
// finish_retirement on the actual executor's owning thread. The producer/CPU
// owner performs offer/wait_ready/release_result; capacity is not a reservation.
//
// Producer-owned Capture storage must remain immutable and alive through COMPLETE
// retirement (finish_retirement), even if wait_ready returns closed early. In
// particular, close does not stop an executing GPU task or release borrowed RAM.
// A closed early waiter may exit/join, but its capture owner must survive until
// quiescent. Close BEFORE joining waiting continuations, then drain actual GPU
// work and CPU borrows and finish every retirement before destroying the channel.
class RequiredChannel final {
  public:
    RequiredChannel();
    ~RequiredChannel();
    RequiredChannel(const RequiredChannel&) = delete;
    RequiredChannel& operator=(const RequiredChannel&) = delete;
    RequiredChannel(RequiredChannel&&) = delete;
    RequiredChannel& operator=(RequiredChannel&&) = delete;

    // Full/closed leaves both the capture and output token untouched.
    ClockError offer(const isle::rsp_probe::Capture&, RequiredToken& out);
    ClockError wait_capacity();
    bool take(RequiredRequest& out);
    // Only publish after actual GPU completion, error/guard checks, and mapping.
    // Both views must be nonempty. A closed channel cancels unborrowed results,
    // but main must still retain/release the real lease via retirement.
    ClockError publish_ready(RequiredToken, RequiredMemoryView);
    // Only publish after actual execution accesses have ceased, including error
    // cleanup. Failure closes admission; no CPU borrow is manufactured.
    ClockError publish_failed(RequiredToken);
    // Success grants exactly one CPU borrow. All errors leave out unchanged.
    // Closed is an early wake, NOT permission to destroy the input Capture.
    ClockError wait_ready(RequiredToken, RequiredMemoryView& out);
    // Call only after all CPU accesses have ceased. Exceptions must release as
    // failed; cancelled consumption is not completed. Non-completed outcomes
    // close admission. A completed borrow may finish normally during close.
    ClockError release_result(RequiredToken, Terminal outcome);
    // Nonblocking main reservation. GPU/CPU execution accesses have ceased;
    // main releases its backend lease AFTER this and BEFORE finish_retirement.
    bool take_retirement(RequiredRetirement& out);
    ClockError finish_retirement(RequiredToken);
    ClockError close();
    // Requires closed, idle, and no blocked waiters. Reset the backend first.
    // Changes epoch, preserves lifetime serial/counters; never wraps identities.
    ClockError reset();
    bool quiescent() const;
    bool closed() const;
    RequiredStats stats() const;

  private:
    enum class Phase { idle, queued, executing, ready, borrowed, acknowledged, retiring };
    bool matches(RequiredToken) const noexcept;
    ClockError publication_error() const noexcept;
    void acknowledge(Terminal) noexcept;

    mutable std::mutex mutex_;
    std::condition_variable changed_;
    const std::uint64_t owner_;
    std::uint64_t epoch_ = 1, next_serial_ = 1;
    RequiredRequest request_{};
    RequiredMemoryView memory_{};
    Terminal outcome_ = Terminal::cancelled;
    Phase phase_ = Phase::idle;
    RequiredStats stats_{};
    bool closed_ = false;
};
} // namespace isle::host
#endif
