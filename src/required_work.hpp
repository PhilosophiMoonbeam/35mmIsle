#ifndef ISLE_REQUIRED_WORK_HPP
#define ISLE_REQUIRED_WORK_HPP

#include "host_clock.hpp"
#include "35mmisle/rsp.hpp"

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <span>


namespace isle::host {
struct RequiredToken {
    std::uint64_t owner = 0, epoch = 0, serial = 0;
    friend bool operator==(RequiredToken, RequiredToken) = default;
};
struct RequiredRequest {
    RequiredToken token{};
    rsp::Task task{};
    std::span<const std::byte> boot;
};
// Main owns the actual write lease. Successful wait_preparation grants one
// writer; submit/abandon_preparation may be called only after its accesses cease.
struct RequiredWritableView {
    std::span<std::uint32_t> rdram;
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
    std::uint64_t submissions = 0, cpu_writes = 0, gpu_ready = 0, cpu_borrows = 0;
    std::uint64_t retirements = 0, completed = 0, failed = 0, cancelled = 0;
    std::size_t capacity_waiters = 0, preparation_waiters = 0, result_waiters = 0;
};

// One fixed slot, with no per-operation allocation, RAM copies, callbacks, or
// implicit backend ownership. Main performs take/publish/take_retirement/
// finish_retirement on the actual executor's owning thread. The CPU owner
// requests preparation, borrows the writer, submits task values and borrowed boot
// bytes, then waits for and releases the read-only result. Capacity is not a
// reservation.
//
// Input/resource owners must remain alive through COMPLETE retirement, even if
// a wait returns closed early. Close never revokes a borrowed writer/reader or
// pretends main's mapping/execution has ceased. Close BEFORE joining waiting
// continuations, then drain actual accesses and finish every retirement before
// destroying the channel or its resources.
class RequiredChannel final {
  public:
    RequiredChannel();
    ~RequiredChannel();
    RequiredChannel(const RequiredChannel&) = delete;
    RequiredChannel& operator=(const RequiredChannel&) = delete;
    RequiredChannel(RequiredChannel&&) = delete;
    RequiredChannel& operator=(RequiredChannel&&) = delete;

    // Full/closed/errors leave output tokens/views untouched.
    ClockError request_preparation(RequiredToken& out);
    ClockError wait_capacity();
    bool take_preparation(RequiredToken& out);
    // Publish only after main obtains its actual write lease. The span must be
    // nonempty. A concurrent close retains the lease for main retirement.
    ClockError publish_preparation(RequiredToken, RequiredWritableView);
    ClockError wait_preparation(RequiredToken, RequiredWritableView& out);
    // Both calls certify cessation of every borrowed CPU write. Submit copies
    // only task values and the borrowed boot view; it does not validate either.
    // Submit during close acknowledges cancellation without queuing execution.
    ClockError submit(RequiredToken, const rsp::Task&, std::span<const std::byte> boot);
    // Only cancelled/failed are valid; either closes admission.
    ClockError abandon_preparation(RequiredToken, Terminal);
    bool take(RequiredRequest& out);
    // Only publish after actual GPU completion, error/guard checks, and mapping.
    // Both views must be nonempty. A closed channel cancels unborrowed results,
    // but main must still retain/release the real lease via retirement.
    ClockError publish_ready(RequiredToken, RequiredMemoryView);
    // Only publish after actual main preparation-mapping or execution accesses
    // have ceased, including error cleanup. Failure closes admission; neither
    // a CPU writer nor a CPU reader may be revoked with this method.
    ClockError publish_failed(RequiredToken);
    // Success grants exactly one CPU borrow. All errors leave out unchanged.
    // Closed is an early wake, NOT permission to destroy task/boot resources.
    ClockError wait_ready(RequiredToken, RequiredMemoryView& out);
    // Call only after all CPU accesses have ceased. Exceptions must release as
    // failed; cancelled consumption is not completed. Non-completed outcomes
    // close admission. A completed borrow may finish normally during close.
    ClockError release_result(RequiredToken, Terminal outcome);
    // Nonblocking main reservation. All CPU/main execution accesses have ceased;
    // main cancels its write lease or releases its read lease AFTER this and
    // BEFORE finish_retirement.
    bool take_retirement(RequiredRetirement& out);
    ClockError finish_retirement(RequiredToken);
    ClockError close();
    // Requires closed, idle, and no waiters/borrows. Caller must separately prove
    // actual backend leases retired and cold backend reset before changing epoch.
    // Changes epoch, preserves lifetime serial/counters; never wraps identities.
    ClockError reset();
    bool quiescent() const;
    bool closed() const;
    RequiredStats stats() const;

  private:
    enum class Phase {
        idle, preparation_requested, preparation_mapping, preparation_ready,
        preparing, queued, executing, ready, borrowed, acknowledged, retiring
    };
    bool matches(RequiredToken) const noexcept;
    ClockError transition_error(Phase expected) const noexcept;
    void acknowledge(Terminal) noexcept;

    mutable std::mutex mutex_;
    std::condition_variable changed_;
    const std::uint64_t owner_;
    std::uint64_t epoch_ = 1, next_serial_ = 1;
    RequiredRequest request_{};
    RequiredWritableView writable_{};
    RequiredMemoryView memory_{};
    Terminal outcome_ = Terminal::cancelled;
    Phase phase_ = Phase::idle;
    RequiredStats stats_{};
    bool closed_ = false;
};
} // namespace isle::host
#endif
