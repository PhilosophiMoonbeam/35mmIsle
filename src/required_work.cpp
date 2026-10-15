#include "required_work.hpp"

#include <atomic>
#include <exception>
#include <limits>
#include <stdexcept>

namespace isle::host {
namespace {
std::uint64_t new_owner() {
    static std::atomic<std::uint64_t> next{1};
    auto value = next.load(std::memory_order_relaxed);
    for (;;) {
        if (value == std::numeric_limits<std::uint64_t>::max())
            throw std::overflow_error("Required-work owner identities exhausted");
        if (next.compare_exchange_weak(value, value + 1, std::memory_order_relaxed)) return value;
    }
}
bool valid(Terminal outcome) noexcept {
    return outcome == Terminal::completed || outcome == Terminal::failed || outcome == Terminal::cancelled;
}
}

RequiredChannel::RequiredChannel() : owner_(new_owner()) {}
RequiredChannel::~RequiredChannel() {
    // Waiting here would deadlock main's own retirement responsibilities. Never
    // silently destroy synchronization protecting an executing input/CPU borrow.
    std::lock_guard lock(mutex_);
    if (phase_ != Phase::idle || stats_.capacity_waiters != 0 ||
        stats_.preparation_waiters != 0 || stats_.result_waiters != 0)
        std::terminate();
}
bool RequiredChannel::matches(RequiredToken token) const noexcept {
    return token.owner == owner_ && token.epoch == epoch_ && token.serial != 0 && request_.token == token;
}
ClockError RequiredChannel::transition_error(Phase expected) const noexcept {
    return phase_ != Phase::idle && phase_ < expected ? ClockError::not_ready : ClockError::duplicate;
}
void RequiredChannel::acknowledge(Terminal outcome) noexcept {
    outcome_ = outcome;
    writable_ = {};
    memory_ = {};
    phase_ = Phase::acknowledged;
}
ClockError RequiredChannel::request_preparation(RequiredToken& out) {
    std::lock_guard lock(mutex_);
    if (closed_) return ClockError::closed;
    if (phase_ != Phase::idle) return ClockError::full;
    if (next_serial_ == std::numeric_limits<std::uint64_t>::max()) {
        closed_ = true;
        changed_.notify_all();
        return ClockError::exhausted;
    }
    request_ = {{owner_, epoch_, next_serial_++}, {}, {}};
    phase_ = Phase::preparation_requested;
    ++stats_.submissions;
    out = request_.token;
    changed_.notify_all();
    return ClockError::none;
}
ClockError RequiredChannel::wait_capacity() {
    std::unique_lock lock(mutex_);
    if (!closed_ && phase_ != Phase::idle) {
        ++stats_.capacity_waiters;
        try {
            changed_.wait(lock, [this] { return closed_ || phase_ == Phase::idle; });
        } catch (...) {
            --stats_.capacity_waiters;
            throw;
        }
        --stats_.capacity_waiters;
    }
    return closed_ ? ClockError::closed : ClockError::none;
}
bool RequiredChannel::take_preparation(RequiredToken& out) {
    std::lock_guard lock(mutex_);
    if (phase_ != Phase::preparation_requested) return false;
    phase_ = Phase::preparation_mapping;
    out = request_.token;
    return true;
}
ClockError RequiredChannel::publish_preparation(RequiredToken token, RequiredWritableView memory) {
    std::lock_guard lock(mutex_);
    if (!matches(token)) return ClockError::stale;
    if (phase_ != Phase::preparation_mapping) return transition_error(Phase::preparation_mapping);
    if (memory.rdram.empty()) return ClockError::invalid;
    if (closed_) acknowledge(Terminal::cancelled);
    else {
        writable_ = memory;
        phase_ = Phase::preparation_ready;
    }
    changed_.notify_all();
    return ClockError::none;
}
ClockError RequiredChannel::wait_preparation(RequiredToken token, RequiredWritableView& out) {
    std::unique_lock lock(mutex_);
    if (!matches(token)) return ClockError::stale;
    if (!closed_ && (phase_ == Phase::preparation_requested || phase_ == Phase::preparation_mapping)) {
        ++stats_.preparation_waiters;
        try {
            changed_.wait(lock, [this, token] {
                return closed_ || !matches(token) ||
                       (phase_ != Phase::preparation_requested && phase_ != Phase::preparation_mapping);
            });
        } catch (...) {
            --stats_.preparation_waiters;
            throw;
        }
        --stats_.preparation_waiters;
    }
    if (!matches(token)) return ClockError::stale;
    if (closed_) return ClockError::closed;
    if (phase_ != Phase::preparation_ready) return transition_error(Phase::preparation_ready);
    phase_ = Phase::preparing;
    ++stats_.cpu_writes;
    out = writable_;
    return ClockError::none;
}
ClockError RequiredChannel::submit(RequiredToken token, const rsp::Task& task,
                                   std::span<const std::byte> boot) {
    std::lock_guard lock(mutex_);
    if (!matches(token)) return ClockError::stale;
    if (phase_ != Phase::preparing) return transition_error(Phase::preparing);
    request_.task = task;
    request_.boot = boot;
    writable_ = {};
    if (closed_) acknowledge(Terminal::cancelled);
    else phase_ = Phase::queued;
    changed_.notify_all();
    return ClockError::none;
}
ClockError RequiredChannel::abandon_preparation(RequiredToken token, Terminal outcome) {
    std::lock_guard lock(mutex_);
    if (outcome != Terminal::cancelled && outcome != Terminal::failed) return ClockError::invalid;
    if (!matches(token)) return ClockError::stale;
    if (phase_ != Phase::preparing) return transition_error(Phase::preparing);
    closed_ = true;
    acknowledge(outcome);
    changed_.notify_all();
    return ClockError::none;
}
bool RequiredChannel::take(RequiredRequest& out) {
    std::lock_guard lock(mutex_);
    if (phase_ != Phase::queued) return false;
    phase_ = Phase::executing;
    out = request_;
    return true;
}
ClockError RequiredChannel::publish_ready(RequiredToken token, RequiredMemoryView memory) {
    std::lock_guard lock(mutex_);
    if (!matches(token)) return ClockError::stale;
    if (phase_ != Phase::executing) return transition_error(Phase::executing);
    if (memory.rdram.empty() || memory.hidden.empty()) return ClockError::invalid;
    ++stats_.gpu_ready;
    if (closed_) acknowledge(Terminal::cancelled);
    else {
        memory_ = memory;
        phase_ = Phase::ready;
    }
    changed_.notify_all();
    return ClockError::none;
}
ClockError RequiredChannel::publish_failed(RequiredToken token) {
    std::lock_guard lock(mutex_);
    if (!matches(token)) return ClockError::stale;
    if (phase_ != Phase::preparation_mapping && phase_ != Phase::executing)
        return transition_error(Phase::executing);
    closed_ = true;
    acknowledge(Terminal::failed);
    changed_.notify_all();
    return ClockError::none;
}
ClockError RequiredChannel::wait_ready(RequiredToken token, RequiredMemoryView& out) {
    std::unique_lock lock(mutex_);
    if (!matches(token)) return ClockError::stale;
    if (!closed_ && (phase_ == Phase::queued || phase_ == Phase::executing)) {
        ++stats_.result_waiters;
        try {
            changed_.wait(lock, [this, token] {
                return closed_ || !matches(token) || (phase_ != Phase::queued && phase_ != Phase::executing);
            });
        } catch (...) {
            --stats_.result_waiters;
            throw;
        }
        --stats_.result_waiters;
    }
    if (!matches(token)) return ClockError::stale;
    if (closed_) return ClockError::closed;
    if (phase_ != Phase::ready) return transition_error(Phase::ready);
    phase_ = Phase::borrowed;
    ++stats_.cpu_borrows;
    out = memory_;
    return ClockError::none;
}
ClockError RequiredChannel::release_result(RequiredToken token, Terminal outcome) {
    std::lock_guard lock(mutex_);
    if (!valid(outcome)) return ClockError::invalid;
    if (!matches(token)) return ClockError::stale;
    if (phase_ == Phase::acknowledged || phase_ == Phase::retiring || phase_ == Phase::idle)
        return ClockError::duplicate;
    if (phase_ != Phase::borrowed) return ClockError::not_ready;
    if (outcome != Terminal::completed) closed_ = true;
    acknowledge(outcome);
    changed_.notify_all();
    return ClockError::none;
}
bool RequiredChannel::take_retirement(RequiredRetirement& out) {
    std::lock_guard lock(mutex_);
    if (phase_ != Phase::acknowledged) return false;
    phase_ = Phase::retiring;
    out = {request_.token, outcome_};
    return true;
}
ClockError RequiredChannel::finish_retirement(RequiredToken token) {
    std::lock_guard lock(mutex_);
    if (!matches(token)) return ClockError::stale;
    if (phase_ == Phase::idle) return ClockError::duplicate;
    if (phase_ != Phase::retiring) return ClockError::not_ready;
    // The caller has now released the actual backend lease on main. Only here
    // can a new preparation be admitted or old task/boot resources be freed.
    request_.task = {};
    request_.boot = {};
    phase_ = Phase::idle;
    ++stats_.retirements;
    switch (outcome_) {
    case Terminal::completed: ++stats_.completed; break;
    case Terminal::failed: ++stats_.failed; break;
    case Terminal::cancelled: ++stats_.cancelled; break;
    }
    changed_.notify_all();
    return ClockError::none;
}
ClockError RequiredChannel::close() {
    std::lock_guard lock(mutex_);
    closed_ = true;
    if (phase_ == Phase::preparation_requested || phase_ == Phase::preparation_ready ||
        phase_ == Phase::queued || phase_ == Phase::ready) acknowledge(Terminal::cancelled);
    // Mapping, preparing, executing and borrowed phases are untouched. Wake
    // before joining continuations, without pretending those accesses ceased.
    changed_.notify_all();
    return ClockError::none;
}
ClockError RequiredChannel::reset() {
    std::lock_guard lock(mutex_);
    if (!closed_ || phase_ != Phase::idle || stats_.capacity_waiters != 0 ||
        stats_.preparation_waiters != 0 || stats_.result_waiters != 0)
        return ClockError::not_ready;
    if (epoch_ == std::numeric_limits<std::uint64_t>::max() ||
        next_serial_ == std::numeric_limits<std::uint64_t>::max()) return ClockError::exhausted;
    ++epoch_;
    request_ = {};
    writable_ = {};
    memory_ = {};
    closed_ = false;
    return ClockError::none;
}
bool RequiredChannel::quiescent() const {
    std::lock_guard lock(mutex_);
    return phase_ == Phase::idle && stats_.capacity_waiters == 0 &&
           stats_.preparation_waiters == 0 && stats_.result_waiters == 0;
}
bool RequiredChannel::closed() const {
    std::lock_guard lock(mutex_);
    return closed_;
}
RequiredStats RequiredChannel::stats() const {
    std::lock_guard lock(mutex_);
    return stats_;
}
} // namespace isle::host
