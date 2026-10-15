#include "../src/required_work.hpp"
#include "../src/rsp_fixture.hpp"

#include <array>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <initializer_list>
#include <iostream>
#include <latch>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>

namespace {
using namespace isle::host;
using isle::rsp_probe::Capture;
using namespace std::chrono_literals;

void check(bool condition, const char* description) {
    if (!condition) throw std::runtime_error(description);
}
// Bounds even a broken close/join implementation without relying on sleeps or
// scheduler timing to decide whether a transition is correct.
class Watchdog {
  public:
    Watchdog() : thread_([this] {
        std::unique_lock lock(mutex_);
        if (!changed_.wait_for(lock, 20s, [this] { return finished_; })) {
            std::fputs("FAIL: required-work lifetime/wake-before-join watchdog\n", stderr);
            std::abort();
        }
    }) {}
    ~Watchdog() {
        {
            std::lock_guard lock(mutex_);
            finished_ = true;
        }
        changed_.notify_all();
        thread_.join();
    }
  private:
    std::mutex mutex_;
    std::condition_variable changed_;
    bool finished_ = false;
    std::thread thread_;
};
class Worker {
  public:
    template<class Function> explicit Worker(Function function)
        : thread_([this, function = std::move(function)] {
            try { function(); } catch (...) { failure_ = std::current_exception(); }
            done_.count_down();
        }) {}
    ~Worker() { if (thread_.joinable()) thread_.join(); }
    void join() {
        done_.wait();
        thread_.join();
        if (failure_) std::rethrow_exception(failure_);
    }
  private:
    std::exception_ptr failure_;
    std::latch done_{1};
    std::thread thread_;
};
void waiters(RequiredChannel& channel, std::size_t capacity, std::size_t result) {
    const auto deadline = std::chrono::steady_clock::now() + 5s;
    for (;;) {
        const auto stats = channel.stats();
        if (stats.capacity_waiters == capacity && stats.result_waiters == result) return;
        check(std::chrono::steady_clock::now() < deadline, "workers enter exact channel waits");
        std::this_thread::yield();
    }
}
bool same(RequiredMemoryView a, RequiredMemoryView b) {
    return a.rdram.data() == b.rdram.data() && a.rdram.size() == b.rdram.size() &&
           a.hidden.data() == b.hidden.data() && a.hidden.size() == b.hidden.size();
}
// Real Capture objects, not forged references. This is a transport test: packets
// are stable producer input, NOT claimed RSP/GPU execution or expected GPU output.
void fill(Capture& capture, std::uint32_t marker) {
    const std::array<std::uint32_t, 2> packet{0x3a000000, marker};
    check(Capture::sink(&capture, packet), "populate actual preallocated capture");
}
// Ordinary owned arrays exercise span lifetime/identity only. publish_ready below
// drives the protocol explicitly; these tests never claim a device completion.
struct Memory {
    std::array<std::byte, 16> rdram{std::byte{0x62}};
    std::array<std::byte, 8> hidden{std::byte{0x37}};
    RequiredMemoryView view() const { return {rdram, hidden}; }
};
RequiredToken offer(RequiredChannel& channel, const Capture& capture) {
    RequiredToken token;
    check(channel.offer(capture, token) == ClockError::none, "admit stable input");
    return token;
}
void take(RequiredChannel& channel, RequiredToken token, const Capture& capture) {
    RequiredRequest request;
    check(channel.take(request) && request.token == token && request.capture == &capture,
          "main takes exact producer-owned capture and identity");
}
RequiredRetirement reserve(RequiredChannel& channel, RequiredToken token, Terminal outcome) {
    RequiredRetirement retirement;
    check(channel.take_retirement(retirement) && retirement.token == token && retirement.outcome == outcome,
          "reserve exact retirement and actual outcome");
    check(!channel.quiescent(), "retirement reservation does not release admission/input");
    return retirement;
}
void finish(RequiredChannel& channel, RequiredToken token, Terminal outcome) {
    reserve(channel, token, outcome);
    // Production main releases its real executor lease at this boundary.
    check(channel.finish_retirement(token) == ClockError::none, "main finishes retirement after lease release");
}
void cancel_queued(RequiredChannel& channel, RequiredToken token) {
    check(channel.close() == ClockError::none, "close queued work");
    finish(channel, token, Terminal::cancelled);
}

void backpressure_and_delayed_main_release() {
    Capture first, second;
    fill(first, 0x13579bdf);
    fill(second, 0x2468ace0);
    const auto* words = first.words().data();
    const auto* packets = first.packets().data();
    RequiredChannel channel;
    check(channel.wait_capacity() == ClockError::none, "idle channel has capacity");
    const auto token = offer(channel, first);
    RequiredToken untouched{91, 92, 93};
    const auto sentinel = untouched;
    check(channel.offer(second, untouched) == ClockError::full && untouched == sentinel,
          "full admission leaves producer output token unchanged");
    check(first.words().data() == words && first.packets().data() == packets &&
          first.words()[1] == 0x13579bdf && second.words()[1] == 0x2468ace0,
          "rejected offer does not copy/rebuild/mutate either capture");
    take(channel, token, first);
    RequiredRequest retained{sentinel, &second};
    check(!channel.take(retained) && retained.token == sentinel && retained.capture == &second,
          "nonblocking empty take leaves output unchanged");
    Memory memory;
    check(channel.publish_ready(token, memory.view()) == ClockError::none, "publish explicit ready transition");
    RequiredMemoryView borrowed;
    check(channel.wait_ready(token, borrowed) == ClockError::none && same(borrowed, memory.view()),
          "CPU receives exact borrowed spans, not copied RAM");
    Worker capacity([&] { check(channel.wait_capacity() == ClockError::none, "capacity wakes after main release"); });
    waiters(channel, 1, 0);
    check(channel.offer(second, untouched) == ClockError::full, "CPU borrow prevents next dependent request");
    check(channel.release_result(token, Terminal::completed) == ClockError::none, "CPU ceases accesses and acknowledges");
    check(channel.offer(second, untouched) == ClockError::full && !channel.quiescent(),
          "CPU acknowledgement alone cannot reuse main's backend lease");
    check(channel.stats().capacity_waiters == 1 && channel.stats().retirements == 0,
          "capacity remains blocked before main reservation");
    reserve(channel, token, Terminal::completed);
    check(channel.offer(second, untouched) == ClockError::full && untouched == sentinel &&
          channel.stats().capacity_waiters == 1 && channel.stats().completed == 0,
          "delayed actual main release still owns capacity and reserves outcome");
    RequiredRetirement untouched_retirement{sentinel, Terminal::failed};
    check(!channel.take_retirement(untouched_retirement) && untouched_retirement.token == sentinel &&
          untouched_retirement.outcome == Terminal::failed, "retirement can be reserved only once");
    check(channel.finish_retirement(token) == ClockError::none, "main explicitly finishes actual release");
    capacity.join();
    check(channel.quiescent(), "full lifetime ends only after main acknowledgement");
    const auto next = offer(channel, second);
    check(next.owner == token.owner && next.epoch == token.epoch && next.serial > token.serial,
          "new admission preserves owner and advances lifetime serial");
    cancel_queued(channel, next);
    const auto stats = channel.stats();
    check(stats.submissions == 2 && stats.gpu_ready == 1 && stats.cpu_borrows == 1 &&
          stats.retirements == 2 && stats.completed == 1 && stats.failed == 0 && stats.cancelled == 1,
          "account completed consumption separately from unexecuted cancellation");
}

void exact_identity_duplicate_and_reset() {
    static_assert(!std::is_convertible_v<RequiredToken, FrameToken>);
    static_assert(!std::is_copy_constructible_v<RequiredChannel>);
    static_assert(!std::is_move_constructible_v<RequiredChannel>);
    Capture capture;
    fill(capture, 0x12345678);
    RequiredChannel channel, foreign;
    check(channel.reset() == ClockError::not_ready, "open channel cannot reset");
    const auto token = offer(channel, capture);
    const auto other = offer(foreign, capture);
    check(token.owner != other.owner, "separate channel constructors have unique owners");
    Memory memory, sentinel;
    RequiredMemoryView out = sentinel.view();
    check(channel.wait_ready(other, out) == ClockError::stale && same(out, sentinel.view()),
          "foreign wait rejects owner and preserves output");
    check(channel.publish_failed(other) == ClockError::stale &&
          channel.publish_ready(other, memory.view()) == ClockError::stale &&
          channel.release_result(other, Terminal::completed) == ClockError::stale &&
          channel.finish_retirement(other) == ClockError::stale, "foreign outcomes cannot mutate slot");
    auto invented = token;
    ++invented.serial;
    check(channel.publish_failed(invented) == ClockError::stale &&
          channel.publish_failed({}) == ClockError::stale, "unissued serial and empty identity rejected");
    check(channel.publish_ready(token, memory.view()) == ClockError::not_ready &&
          channel.publish_failed(token) == ClockError::not_ready &&
          channel.release_result(token, Terminal::completed) == ClockError::not_ready &&
          channel.finish_retirement(token) == ClockError::not_ready, "no operation skips queued execution");
    take(channel, token, capture);
    check(channel.publish_ready(token, {}) == ClockError::invalid, "empty RAM cannot grant a CPU borrow");
    check(channel.publish_ready(token, memory.view()) == ClockError::none &&
          channel.publish_ready(token, memory.view()) == ClockError::duplicate &&
          channel.publish_failed(token) == ClockError::duplicate, "only one GPU publication owns outcome");
    check(channel.wait_ready(token, out) == ClockError::none && same(out, memory.view()), "grant first CPU borrow");
    out = sentinel.view();
    check(channel.wait_ready(token, out) == ClockError::duplicate && same(out, sentinel.view()),
          "duplicate waiter cannot obtain another RAM borrow");
    check(channel.release_result(token, static_cast<Terminal>(255)) == ClockError::invalid &&
          channel.finish_retirement(token) == ClockError::not_ready, "invalid CPU outcome preserves live borrow");
    check(channel.release_result(token, Terminal::completed) == ClockError::none &&
          channel.release_result(token, Terminal::failed) == ClockError::duplicate,
          "duplicate CPU release cannot replace completed outcome");
    finish(channel, token, Terminal::completed);
    check(channel.finish_retirement(token) == ClockError::duplicate &&
          channel.release_result(token, Terminal::failed) == ClockError::duplicate,
          "fully retired token cannot retire twice");
    const auto second = offer(channel, capture);
    check(channel.release_result(token, Terminal::completed) == ClockError::stale &&
          channel.publish_failed(token) == ClockError::stale, "old serial cannot affect reused slot");
    check(channel.close() == ClockError::none && channel.reset() == ClockError::not_ready,
          "closed pending retirement still prevents reset");
    reserve(channel, second, Terminal::cancelled);
    check(channel.reset() == ClockError::not_ready, "reserved main lease prevents reset");
    check(channel.finish_retirement(second) == ClockError::none && channel.reset() == ClockError::none,
          "closed fully retired channel starts a new epoch");
    check(!channel.closed() && channel.quiescent(), "reset reopens only quiescent channel");
    const auto third = offer(channel, capture);
    check(third.owner == token.owner && third.epoch != token.epoch && third.serial > second.serial,
          "reset changes epoch without recycling lifetime serial");
    check(channel.wait_ready(second, out) == ClockError::stale && same(out, sentinel.view()) &&
          channel.publish_ready(second, memory.view()) == ClockError::stale &&
          channel.publish_failed(second) == ClockError::stale &&
          channel.release_result(second, Terminal::completed) == ClockError::stale &&
          channel.finish_retirement(second) == ClockError::stale, "all pre-reset operations are rejected");
    cancel_queued(channel, third);
    cancel_queued(foreign, other);
    check(channel.stats().submissions == 3 && channel.stats().retirements == 3,
          "reset preserves lifetime counters");
}

void close_queued_wakes_before_join() {
    Capture capture;
    fill(capture, 0xabcdef01);
    RequiredChannel channel;
    const auto token = offer(channel, capture);
    Memory sentinel;
    Worker result([&] {
        auto out = sentinel.view();
        check(channel.wait_ready(token, out) == ClockError::closed && same(out, sentinel.view()),
              "queued result wait closes without fabricating RAM");
    });
    Worker capacity([&] { check(channel.wait_capacity() == ClockError::closed, "queued capacity wait closes"); });
    waiters(channel, 1, 1);
    check(channel.close() == ClockError::none && channel.close() == ClockError::none,
          "close is idempotent and wakes both classes before joins");
    result.join();
    capacity.join();
    check(channel.closed() && !channel.quiescent() && channel.reset() == ClockError::not_ready,
          "woken/joined owners do not implicitly free queued retirement");
    RequiredToken untouched{55, 66, 77};
    const auto sentinel_token = untouched;
    check(channel.offer(capture, untouched) == ClockError::closed && untouched == sentinel_token &&
          capture.words()[1] == 0xabcdef01, "closed offer retains exact input and output token");
    RequiredRequest request{sentinel_token, &capture};
    check(!channel.take(request) && request.token == sentinel_token && request.capture == &capture,
          "closed queued cancellation cannot subsequently execute");
    finish(channel, token, Terminal::cancelled);
    check(channel.quiescent() && channel.stats().gpu_ready == 0 && channel.stats().completed == 0,
          "unexecuted cancellation is not successful execution");
}

void close_executing_retains_input_after_early_wait() {
    Capture capture;
    fill(capture, 0x89abcdef);
    const auto* storage = capture.words().data();
    RequiredChannel channel;
    const auto token = offer(channel, capture);
    RequiredRequest executing;
    check(channel.take(executing), "main holds executing input");
    Memory memory, sentinel;
    Worker result([&] {
        auto out = sentinel.view();
        check(channel.wait_ready(token, out) == ClockError::closed && same(out, sentinel.view()),
              "executing result wait wakes early without GPU completion");
    });
    Worker capacity([&] { check(channel.wait_capacity() == ClockError::closed, "executing capacity wakes on close"); });
    waiters(channel, 1, 1);
    check(channel.close() == ClockError::none, "close while executor still owns capture");
    result.join();
    capacity.join();
    RequiredRetirement retirement;
    check(!channel.take_retirement(retirement) && !channel.quiescent() &&
          channel.reset() == ClockError::not_ready && channel.stats().retirements == 0,
          "early closed waiter/join is not GPU cancellation or input retirement");
    check(executing.capture == &capture && executing.token == token &&
          executing.capture->words().data() == storage && executing.capture->words()[1] == 0x89abcdef,
          "producer storage remains alive and stable after early-closed waiter exits");
    check(channel.publish_ready(token, memory.view()) == ClockError::none,
          "actual executor return must still be reported after close");
    auto out = sentinel.view();
    check(channel.wait_ready(token, out) == ClockError::closed && same(out, sentinel.view()) &&
          channel.stats().cpu_borrows == 0, "late ready result never grants post-close borrow");
    reserve(channel, token, Terminal::cancelled);
    check(channel.reset() == ClockError::not_ready, "late GPU lease still requires main release");
    check(channel.finish_retirement(token) == ClockError::none && channel.quiescent(),
          "input lifetime ends only after actual main release");
}

void close_ready_and_borrowed() {
    Capture capture;
    fill(capture, 0x90919293);
    Memory memory, sentinel;
    {
        RequiredChannel channel;
        const auto token = offer(channel, capture);
        take(channel, token, capture);
        check(channel.publish_ready(token, memory.view()) == ClockError::none &&
              channel.close() == ClockError::none, "close unborrowed ready RAM");
        auto out = sentinel.view();
        check(channel.wait_ready(token, out) == ClockError::closed && same(out, sentinel.view()),
              "ready close cancels before CPU access");
        finish(channel, token, Terminal::cancelled);
        check(channel.stats().cpu_borrows == 0 && channel.stats().completed == 0,
              "unconsumed ready result never counts as completion");
    }
    RequiredChannel channel;
    const auto token = offer(channel, capture);
    take(channel, token, capture);
    check(channel.publish_ready(token, memory.view()) == ClockError::none, "prepare borrowed close");
    std::latch borrowed{1}, allow_cpu_release{1};
    Worker cpu([&] {
        RequiredMemoryView out;
        check(channel.wait_ready(token, out) == ClockError::none && same(out, memory.view()), "CPU obtains borrow");
        borrowed.count_down();
        allow_cpu_release.wait();
        check(out.rdram[0] == std::byte{0x62} && out.hidden[0] == std::byte{0x37},
              "close preserves existing borrowed spans through last CPU access");
        check(channel.release_result(token, Terminal::cancelled) == ClockError::none, "CPU explicitly cancels ceased borrow");
    });
    borrowed.wait();
    Worker capacity([&] { check(channel.wait_capacity() == ClockError::closed, "close wakes capacity without reclaiming borrow"); });
    waiters(channel, 1, 0);
    check(channel.close() == ClockError::none, "close cannot revoke a CPU borrow");
    capacity.join();
    RequiredRetirement retirement;
    check(!channel.take_retirement(retirement) && !channel.quiescent() && channel.reset() == ClockError::not_ready,
          "close cannot release/reset borrowed RAM before CPU cessation");
    allow_cpu_release.count_down();
    cpu.join();
    check(!channel.quiescent() && channel.reset() == ClockError::not_ready,
          "cancelled CPU acknowledgement still retains main lease");
    finish(channel, token, Terminal::cancelled);
    check(channel.stats().completed == 0 && channel.stats().cancelled == 1 && channel.reset() == ClockError::none,
          "cancelled consumption never becomes success and reset waits for full retirement");
}

void failure_and_cpu_exception_paths() {
    Capture capture;
    fill(capture, 0x10203040);
    Memory memory, sentinel;
    for (const bool close_first : {false, true}) {
        RequiredChannel channel;
        const auto token = offer(channel, capture);
        take(channel, token, capture);
        Worker result([&] {
            auto out = sentinel.view();
            check(channel.wait_ready(token, out) == ClockError::closed && same(out, sentinel.view()),
                  "GPU failure wakes result wait without granting success");
        });
        Worker capacity([&] { check(channel.wait_capacity() == ClockError::closed, "failure wakes capacity wait"); });
        waiters(channel, 1, 1);
        if (close_first) check(channel.close() == ClockError::none, "close before actual GPU failure returns");
        check(channel.publish_failed(token) == ClockError::none && channel.closed(), "actual GPU failure closes admission");
        result.join();
        capacity.join();
        check(channel.publish_ready(token, memory.view()) == ClockError::duplicate &&
              channel.publish_failed(token) == ClockError::duplicate, "later publication cannot overwrite failed GPU outcome");
        finish(channel, token, Terminal::failed);
        const auto stats = channel.stats();
        check(stats.failed == 1 && stats.cancelled == 0 && stats.completed == 0 &&
              stats.cpu_borrows == 0 && stats.gpu_ready == 0, "actual GPU failure stays failed even during shutdown");
    }
    RequiredChannel channel;
    const auto token = offer(channel, capture);
    take(channel, token, capture);
    check(channel.publish_ready(token, memory.view()) == ClockError::none, "prepare CPU exception path");
    std::latch borrowed{1}, raise_exception{1};
    Worker cpu([&] {
        RequiredMemoryView out;
        check(channel.wait_ready(token, out) == ClockError::none, "CPU exception occurs only after a real protocol borrow");
        borrowed.count_down();
        raise_exception.wait();
        try {
            throw std::runtime_error("intentional CPU consumer exception");
        } catch (const std::runtime_error&) {
            check(channel.release_result(token, Terminal::failed) == ClockError::none,
                  "CPU exception acknowledges failed, never successful consumption");
        }
    });
    borrowed.wait();
    Worker capacity([&] { check(channel.wait_capacity() == ClockError::closed, "CPU failure closes/wakes capacity"); });
    waiters(channel, 1, 0);
    raise_exception.count_down();
    cpu.join();
    capacity.join();
    finish(channel, token, Terminal::failed);
    check(channel.closed() && channel.stats().failed == 1 && channel.stats().completed == 0 &&
          channel.stats().cpu_borrows == 1, "failed CPU consumption is accounted separately from successful GPU readiness");
}

void concurrent_result_waiters_grant_one_borrow() {
    Capture capture;
    fill(capture, 0x45464748);
    RequiredChannel channel;
    const auto token = offer(channel, capture);
    take(channel, token, capture);
    Memory memory, sentinel;
    std::array<ClockError, 2> results{ClockError::invalid, ClockError::invalid};
    std::latch returned{2}, release{1};
    auto wait = [&](std::size_t index) {
        auto out = sentinel.view();
        results[index] = channel.wait_ready(token, out);
        if (results[index] == ClockError::none) check(same(out, memory.view()), "winning waiter receives exact RAM");
        else check(results[index] == ClockError::duplicate && same(out, sentinel.view()), "losing waiter gets no borrow");
        returned.count_down();
        if (results[index] == ClockError::none) {
            release.wait();
            check(channel.release_result(token, Terminal::completed) == ClockError::none, "only winning waiter releases CPU borrow");
        }
    };
    Worker first([&] { wait(0); }), second([&] { wait(1); });
    waiters(channel, 0, 2);
    check(channel.publish_ready(token, memory.view()) == ClockError::none, "one publication wakes competing owner waits");
    returned.wait();
    check((results[0] == ClockError::none) != (results[1] == ClockError::none) &&
          channel.stats().cpu_borrows == 1, "exactly one waiter can acquire the reserved result");
    check(channel.close() == ClockError::none, "close while winning consumer still borrows");
    release.count_down();
    first.join();
    second.join();
    finish(channel, token, Terminal::completed);
    check(channel.stats().completed == 1 && channel.stats().cancelled == 0,
          "CPU genuinely completed during close is not retroactively cancelled");
}
} // namespace

int main() {
    Watchdog watchdog;
    try {
        backpressure_and_delayed_main_release();
        exact_identity_duplicate_and_reset();
        close_queued_wakes_before_join();
        close_executing_retains_input_after_early_wait();
        close_ready_and_borrowed();
        failure_and_cpu_exception_paths();
        concurrent_result_waiters_grant_one_borrow();
        std::cout << "Required-work identity, backpressure, borrow/retirement, failure and close/join regressions passed (transport only)\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
