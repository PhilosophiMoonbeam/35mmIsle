#include "host_clock.hpp"

#include <atomic>
#include <limits>
#include <numeric>
#include <stdexcept>

namespace isle::host {
namespace {
std::uint64_t new_owner() {
    static std::atomic<std::uint64_t> next{1};
    auto value = next.load(std::memory_order_relaxed);
    for (;;) {
        if (value == std::numeric_limits<std::uint64_t>::max())
            throw std::overflow_error("GTL owner identities exhausted");
        if (next.compare_exchange_weak(value, value + 1, std::memory_order_relaxed)) return value;
    }
}
}
bool GtlClock::valid(ClockConfig config) noexcept {
    return config.update_interval != 0 && config.draw_interval != 0 &&
           config.contexts != 0 && config.contexts <= 2;
}
GtlClock::GtlClock(ClockConfig config) : config_(config), frame_(config.initial_frame) {
    if (!valid(config)) throw std::invalid_argument("Unsupported GTL intervals/context count");
    owner_ = new_owner();
}
void GtlClock::exhaust() noexcept {
    failed_ = true;
    stop_requested_ = true;
}
ClockError GtlClock::offer_retraces(std::uint64_t count) noexcept {
    if (stop_requested_) return ClockError::closed;
    if (count > std::numeric_limits<std::uint64_t>::max() - stats_.retraces) {
        exhaust();
        return ClockError::exhausted;
    }
    const auto available = 4u - ticks_;
    const auto accepted = count < available ? count : available;
    stats_.retraces += count;
    stats_.dropped_retraces += count - accepted;
    ticks_ += static_cast<std::size_t>(accepted);
    return accepted == count ? ClockError::none : ClockError::full;
}
bool GtlClock::any_active() const noexcept {
    for (const auto& slot : slots_) if (slot.active) return true;
    return false;
}
void GtlClock::drain_completions() noexcept {
    while (completion_count_ != 0) {
        const auto completion = completions_[completion_head_];
        completion_head_ = (completion_head_ + 1) % completions_.size();
        --completion_count_;
        auto& slot = slots_[completion.context];
        // Admission already validated identity and reserved one terminal.
        slot.active = slot.pending = false;
        ++stats_.terminals;
    }
}
ClockAction GtlClock::poll() noexcept {
    for (;;) {
        if (stop_requested_ && phase_ != Phase::updating && phase_ != Phase::drawing &&
            phase_ != Phase::stopped) phase_ = Phase::draining;
        switch (phase_) {
        case Phase::boundary:
            if (pause_requested_) {
                phase_ = Phase::paused;
                return {ActionKind::paused, {}};
            }
            received_ = 0;
            phase_ = Phase::ticks;
            break;
        case Phase::ticks:
            // This phase is an entered blocking receive. Finish it before
            // reconsidering an interval changed while the receive was waiting.
            for (;;) {
                if (ticks_ == 0) return {};
                --ticks_;
                ++received_;
                if (received_ >= config_.update_interval) break;
            }
            ticks_ = 0; // GAME drains surplus ticks, then performs ONE update.
            if (stats_.updates == std::numeric_limits<std::uint64_t>::max()) {
                exhaust();
                break;
            }
            phase_ = Phase::updating;
            return {ActionKind::update, {}};
        case Phase::updating:
        case Phase::drawing:
        case Phase::paused:
            return {};
        case Phase::context:
            drain_completions();
            if (stop_requested_) break;
            for (std::uint8_t index = 0; index < config_.contexts; ++index) {
                auto& slot = slots_[index];
                if (slot.active) continue;
                if (next_serial_ == std::numeric_limits<std::uint64_t>::max()) {
                    exhaust();
                    break;
                }
                slot.token = {owner_, epoch_, next_serial_++, index};
                slot.active = true;
                slot.accepted = slot.pending = false;
                phase_ = Phase::drawing;
                return {ActionKind::draw, slot.token};
            }
            if (stop_requested_) break;
            if (config_.blocking_draw) return {};
            ++stats_.skipped_draws;
            phase_ = Phase::boundary;
            break;
        case Phase::draining:
            drain_completions();
            if (any_active()) return {};
            ticks_ = 0;
            phase_ = Phase::stopped;
            return {ActionKind::stopped, {}};
        case Phase::stopped:
            return {};
        }
    }
}
ClockError GtlClock::finish_update() noexcept {
    if (phase_ != Phase::updating) return ClockError::not_ready;
    ++stats_.updates;
    ++frame_; // Original 32-bit frame arithmetic, including wrap.
    phase_ = stop_requested_ ? Phase::draining :
        frame_ % config_.draw_interval == 0 ? Phase::context : Phase::boundary;
    return ClockError::none;
}
ClockError GtlClock::abandon_update() noexcept {
    if (phase_ != Phase::updating) return ClockError::not_ready;
    stop_requested_ = true;
    phase_ = Phase::draining;
    return ClockError::none;
}
ClockError GtlClock::accept_draw(FrameToken token) noexcept {
    if (token.owner != owner_ || token.epoch != epoch_ || token.context >= config_.contexts)
        return ClockError::stale;
    auto& slot = slots_[token.context];
    if (slot.token != token) return ClockError::stale;
    if (slot.accepted) return ClockError::duplicate;
    if (phase_ != Phase::drawing || !slot.active) return ClockError::not_ready;
    slot.accepted = true;
    ++stats_.submissions;
    ++drawn_frames_;
    phase_ = Phase::boundary;
    return ClockError::none;
}
ClockError GtlClock::abandon_draw(FrameToken token) noexcept {
    if (token.owner != owner_ || token.epoch != epoch_ || token.context >= config_.contexts)
        return ClockError::stale;
    auto& slot = slots_[token.context];
    if (slot.token != token) return ClockError::stale;
    if (phase_ != Phase::drawing || !slot.active || slot.accepted) return ClockError::not_ready;
    slot.active = false;
    stop_requested_ = true;
    phase_ = Phase::draining;
    return ClockError::none;
}
ClockError GtlClock::offer_terminal(FrameToken token, Terminal terminal) noexcept {
    if (terminal != Terminal::completed && terminal != Terminal::failed && terminal != Terminal::cancelled)
        return ClockError::invalid;
    if (token.owner != owner_ || token.epoch != epoch_ || token.context >= config_.contexts)
        return ClockError::stale;
    auto& slot = slots_[token.context];
    if (slot.token != token) return ClockError::stale;
    if (slot.pending || (!slot.active && slot.accepted)) return ClockError::duplicate;
    if (!slot.accepted) return ClockError::not_ready;
    if (completion_count_ == completions_.size()) return ClockError::full;
    completions_[(completion_head_ + completion_count_) % completions_.size()] = token;
    ++completion_count_;
    slot.pending = true;
    if (terminal == Terminal::failed || (terminal == Terminal::cancelled && !stop_requested_)) {
        failed_ = true;
        stop_requested_ = true; // Failure closes admission even during pause/tick waits.
    }
    return ClockError::none;
}
ClockError GtlClock::set_intervals(std::uint16_t update, std::uint16_t draw) noexcept {
    if (stop_requested_) return ClockError::closed;
    if (update == 0 || draw == 0) return ClockError::invalid;
    config_.update_interval = update;
    config_.draw_interval = draw;
    return ClockError::none;
}
ClockError GtlClock::request_pause() noexcept {
    if (stop_requested_) return ClockError::closed;
    pause_requested_ = true;
    return ClockError::none;
}
ClockError GtlClock::resume() noexcept {
    if (stop_requested_) return ClockError::closed;
    if (phase_ != Phase::paused) return ClockError::not_ready;
    pause_requested_ = false;
    phase_ = Phase::boundary;
    return ClockError::none;
}
void GtlClock::request_stop() noexcept { stop_requested_ = true; }
bool GtlClock::stopped() const noexcept { return phase_ == Phase::stopped; }
bool GtlClock::paused() const noexcept { return phase_ == Phase::paused; }
ClockError GtlClock::reset(ClockConfig config) noexcept {
    if (!valid(config)) return ClockError::invalid;
    if (!stopped() || any_active() || completion_count_ != 0) return ClockError::not_ready;
    if (epoch_ == std::numeric_limits<std::uint64_t>::max()) return ClockError::exhausted;
    config_ = config;
    ++epoch_;
    slots_ = {};
    completion_head_ = completion_count_ = ticks_ = 0;
    received_ = 0;
    frame_ = config.initial_frame;
    drawn_frames_ = 0;
    pause_requested_ = stop_requested_ = failed_ = false;
    phase_ = Phase::boundary;
    return ClockError::none;
}

RetraceClock::RetraceClock(std::uint32_t numerator_hz, std::uint32_t denominator_hz)
    : numerator_(numerator_hz), denominator_(denominator_hz) {
    if (numerator_hz == 0 || numerator_hz > 1000000000u || denominator_hz == 0)
        throw std::invalid_argument("Unsupported rational retrace frequency");
    const auto common = std::gcd(numerator_, denominator_);
    numerator_ /= common;
    denominator_ /= common;
}
std::uint64_t RetraceClock::advance(std::uint64_t elapsed_ns) {
    if (elapsed_ns < previous_ns_) throw std::invalid_argument("Retrace time moved backwards");
    const auto delta = elapsed_ns - previous_ns_;
    const auto seconds_product = (delta / 1000000000u) * numerator_;
    const auto whole = seconds_product / denominator_;
    const auto units = remainder_ + (seconds_product % denominator_) * 1000000000u +
                       (delta % 1000000000u) * numerator_;
    const auto denominator = static_cast<std::uint64_t>(denominator_) * 1000000000u;
    previous_ns_ = elapsed_ns;
    remainder_ = units % denominator;
    return whole + units / denominator;
}
std::uint64_t RetraceClock::nanoseconds_until_next() const noexcept {
    const auto remaining = static_cast<std::uint64_t>(denominator_) * 1000000000u - remainder_;
    return (remaining - 1u) / numerator_ + 1u;
}
std::uint64_t RetraceClock::guest_ticks(std::uint64_t elapsed_ns) noexcept {
    // Public osGetTime Count domain: 46.875MHz = 3 ticks per64ns.
    return (elapsed_ns / 64u) * 3u + ((elapsed_ns % 64u) * 3u) / 64u;
}
} // namespace isle::host
