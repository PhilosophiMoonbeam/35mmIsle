#include "../src/host_clock.hpp"

#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
using namespace isle::host;
void check(bool condition, const char* description) {
    if (!condition) throw std::runtime_error(description);
}
FrameToken submit(GtlClock& clock) {
    check(clock.offer_retraces() == ClockError::none, "one retrace admitted");
    check(clock.poll().kind == ActionKind::update, "retrace permits update");
    check(clock.finish_update() == ClockError::none, "update returns to policy");
    const auto draw = clock.poll();
    check(draw.kind == ActionKind::draw, "available context permits draw");
    check(clock.accept_draw(draw.token) == ClockError::none, "draw transport admitted");
    return draw.token;
}
void bounded_ticks() {
    GtlClock clock({});
    check(clock.offer_retraces(6) == ClockError::full, "four-slot retrace queue drops newest");
    check(clock.buffered_retraces() == 4 && clock.stats().dropped_retraces == 2,
          "surplus retraces counted, not accumulated unboundedly");
    check(clock.poll().kind == ActionKind::update, "buffered retraces permit one update");
    check(clock.buffered_retraces() == 0, "surplus ticks drained before update");
    check(clock.finish_update() == ClockError::none, "finish single update");
    const auto draw = clock.poll();
    check(draw.kind == ActionKind::draw, "draw after bounded update");
    check(clock.accept_draw(draw.token) == ClockError::none, "admit draw");
    check(clock.poll().kind == ActionKind::wait && clock.stats().updates == 1,
          "no catch-up updates from drained ticks");

    GtlClock long_interval({6, 2, 1, true, 0});
    check(long_interval.offer_retraces(4) == ClockError::none, "first interval fragment");
    check(long_interval.poll().kind == ActionKind::wait && long_interval.buffered_retraces() == 0,
          "large interval consumes ticks individually");
    check(long_interval.offer_retraces(2) == ClockError::none, "second interval fragment");
    check(long_interval.poll().kind == ActionKind::update, "interval larger than capacity progresses");
    check(long_interval.finish_update() == ClockError::none, "finish interval update");
    check(long_interval.poll().kind == ActionKind::wait && long_interval.stats().submissions == 0,
          "draw interval suppresses draw, not update");

    GtlClock changed({6, 2, 1, true, 0});
    check(changed.offer_retraces(4) == ClockError::none && changed.poll().kind == ActionKind::wait,
          "interval change begins with an entered fifth blocking receive");
    check(changed.set_intervals(2, 2) == ClockError::none && changed.poll().kind == ActionKind::wait,
          "shrinking interval cannot cancel an already-entered receive");
    check(changed.offer_retraces() == ClockError::none && changed.poll().kind == ActionKind::update,
          "pending receive completes before changed interval is reconsidered");
    check(changed.finish_update() == ClockError::none && changed.stats().updates == 1,
          "interval shrink produces one update after actual tick credit");
}
void context_leases() {
    GtlClock clock({});
    const auto first = submit(clock);
    const auto second = submit(clock);
    check(first.context == 0 && second.context == 1, "lowest free context selected");
    check(clock.offer_retraces() == ClockError::none && clock.poll().kind == ActionKind::update,
          "third update precedes blocking context acquisition");
    check(clock.finish_update() == ClockError::none, "third update finished");
    check(clock.poll().kind == ActionKind::wait, "both borrowed contexts remain unavailable");
    check(clock.offer_retraces(6) == ClockError::full, "ticks stay bounded during context wait");
    check(clock.poll().kind == ActionKind::wait && clock.stats().updates == 3,
          "blocking draw cannot run another update");
    check(clock.offer_terminal(first, Terminal::completed) == ClockError::none, "real terminal admitted");
    check(clock.offer_terminal(first, Terminal::completed) == ClockError::duplicate,
          "duplicate pending terminal cannot retire twice");
    const auto third = clock.poll();
    check(third.kind == ActionKind::draw && third.token.context == 0 && third.token != first,
          "only retired context reused with new identity");
    check(clock.accept_draw(third.token) == ClockError::none, "third draw admitted");
    check(clock.offer_terminal(first, Terminal::completed) == ClockError::stale,
          "old completion cannot retire reused context");
    check(clock.poll().kind == ActionKind::update, "buffered ticks release exactly next update");
    check(clock.finish_update() == ClockError::none && clock.poll().kind == ActionKind::wait,
          "new update still waits for actual terminal");
    check(clock.stats().updates == 4 && clock.stats().terminals == 1,
          "one terminal and one drained tick batch have exact effects");
    check(clock.offer_terminal(second, Terminal::completed) == ClockError::none, "second terminal");
    const auto fourth = clock.poll();
    check(fourth.kind == ActionKind::draw && fourth.token.context == 1, "second context retires independently");
    check(clock.abandon_draw(fourth.token) == ClockError::none, "unadmitted producer can cancel safely");
    check(clock.poll().kind == ActionKind::wait, "stop waits for remaining accepted consumer");
    check(clock.offer_terminal(third.token, Terminal::cancelled) == ClockError::none, "last consumer ceased");
    check(clock.poll().kind == ActionKind::stopped && !clock.failed(), "cancelled shutdown drains without fake completion");
    check(clock.reset({}) == ClockError::none, "quiescent reset opens new epoch");
    const auto replacement = submit(clock);
    check(replacement.epoch != first.epoch && replacement.serial > third.token.serial,
          "reset preserves lifetime serial and changes epoch");
    check(clock.offer_terminal(first, Terminal::completed) == ClockError::stale, "pre-reset terminal rejected");
    GtlClock foreign({});
    const auto foreign_token = submit(foreign);
    check(clock.offer_terminal(foreign_token, Terminal::completed) == ClockError::stale,
          "same-slot foreign owner cannot complete this service");
}
void opportunistic_and_pause() {
    GtlClock clock({1, 1, 2, false, 0});
    submit(clock);
    submit(clock);
    check(clock.offer_retraces() == ClockError::none && clock.poll().kind == ActionKind::update,
          "opportunistic update continues with busy contexts");
    check(clock.finish_update() == ClockError::none && clock.poll().kind == ActionKind::wait,
          "opportunistic draw skips rather than waits");
    check(clock.stats().updates == 3 && clock.stats().submissions == 2 && clock.stats().skipped_draws == 1,
          "skipped draw does not skip game logic");
    check(clock.request_pause() == ClockError::none && clock.resume() == ClockError::not_ready,
          "early resume is not a stored wake acknowledgement");
    check(clock.offer_retraces() == ClockError::none && clock.poll().kind == ActionKind::update,
          "pending pause does not interrupt an already-entered tick wait");
    check(clock.finish_update() == ClockError::none && clock.poll().kind == ActionKind::paused,
          "pause acknowledged only at next loop boundary");
    check(clock.paused() && clock.offer_retraces(6) == ClockError::full,
          "paused game still has bounded incoming retraces");
    check(clock.poll().kind == ActionKind::wait && clock.stats().updates == 4, "pause blocks update");
    check(clock.resume() == ClockError::none && clock.poll().kind == ActionKind::update,
          "acknowledged resume consumes one accumulated batch");
    check(clock.finish_update() == ClockError::none && clock.poll().kind == ActionKind::wait,
          "resumed opportunistic frame remains independent of presentation");
    check(clock.stats().updates == 5 && clock.stats().submissions == 2, "no post-resume catch-up burst");
}
void failure_and_wrap() {
    GtlClock paused({});
    const auto paused_frame = submit(paused);
    check(paused.request_pause() == ClockError::none && paused.poll().kind == ActionKind::paused,
          "pause with a live accepted consumer");
    check(paused.offer_terminal(paused_frame, Terminal::failed) == ClockError::none,
          "execution failure delivered while paused");
    check(paused.offer_retraces() == ClockError::closed && paused.failed() &&
          paused.poll().kind == ActionKind::stopped,
          "failure immediately closes admission and wakes paused shutdown without resume");
    GtlClock clock({1, 1, 2, true, UINT32_MAX});
    const auto first = submit(clock);
    check(clock.frame() == 0, "original frame low word wraps deliberately");
    const auto second = submit(clock);
    check(clock.offer_terminal(first, Terminal::failed) == ClockError::none, "failed execution terminal");
    clock.request_stop();
    check(clock.poll().kind == ActionKind::wait && clock.failed(), "failure stops but preserves other borrow");
    check(clock.reset({}) == ClockError::not_ready, "reset cannot invalidate live consumer");
    check(clock.offer_terminal(second, Terminal::cancelled) == ClockError::none, "peer execution ceased");
    check(clock.poll().kind == ActionKind::stopped && clock.stats().terminals == 2,
          "failed generation resolves every accepted consumer");
    GtlClock cancelled({});
    check(cancelled.offer_retraces() == ClockError::none && cancelled.poll().kind == ActionKind::update,
          "update reserved before cancellation");
    check(cancelled.abandon_update() == ClockError::none && cancelled.poll().kind == ActionKind::stopped &&
          cancelled.stats().updates == 0, "cancelled update is not fabricated game progress");
    GtlClock exhausted({});
    check(exhausted.offer_retraces(UINT64_MAX) == ClockError::full, "bulk backlog bounded without looping");
    check(exhausted.offer_retraces() == ClockError::exhausted && exhausted.poll().kind == ActionKind::stopped &&
          exhausted.failed(), "lifetime counter exhaustion fails closed, never wraps identity");
}
void rational_time() {
    RetraceClock clock(3, 2);
    check(clock.advance(666666666) == 0 && clock.nanoseconds_until_next() == 1,
          "fractional period rounds deadline upward, not tick count");
    check(clock.advance(666666667) == 1 && clock.advance(2000000000) == 2,
          "rational accumulation has no repeated-period drift");
    bool rejected = false;
    try { clock.advance(1999999999); } catch (const std::invalid_argument&) { rejected = true; }
    check(rejected && clock.advance(4000000000) == 3, "backwards clock rejection preserves accumulator");
    RetraceClock fragmented(7, 3);
    std::uint64_t ticks = 0;
    for (std::uint64_t ns = 1000000; ns <= 9000000000; ns += 1000000) ticks += fragmented.advance(ns);
    check(ticks == 21 && fragmented.nanoseconds_until_next() == 428571429,
          "arbitrary host polling partitions preserve rational cadence and next deadline");
    RetraceClock maximum(1000000000, 1);
    check(maximum.advance(UINT64_MAX) == UINT64_MAX && maximum.advance(UINT64_MAX) == 0,
          "maximum elapsed time avoids multiplication overflow");
    check(RetraceClock::guest_ticks(63) == 2 && RetraceClock::guest_ticks(64) == 3 &&
          RetraceClock::guest_ticks(1000000000) == 46875000 &&
          RetraceClock::guest_ticks(UINT64_MAX) == UINT64_C(864691128455135231),
          "public guest Count-domain projection is exact across boundaries and full host range");
}
}
int main() {
    try {
        bounded_ticks(); context_leases(); opportunistic_and_pause(); failure_and_wrap(); rational_time();
        std::cout << "GTL queue, context lease, pause/reset/failure and rational clock regressions passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
