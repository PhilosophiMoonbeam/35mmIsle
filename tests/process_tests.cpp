#include "35mmisle/process.hpp"

#include <array>
#include <atomic>
#include <cfenv>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <thread>

#if defined(_MSC_VER)
#pragma fenv_access(on)
#endif

namespace {

using State = isle::Process::State;
int failures = 0;

void check(bool condition, const char* description) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << description << '\n';
    }
}

struct Lifetime {
    int& live;
    int& destroyed;
    Lifetime(int& live_count, int& destroyed_count) : live(live_count), destroyed(destroyed_count) {
        ++live;
    }
    ~Lifetime() { --live; ++destroyed; }
};

struct ThreadExit {
    std::atomic<int>& exits;
    ~ThreadExit() { exits.fetch_add(1, std::memory_order_relaxed); }
};

void entry_zero_and_return() {
    const auto owner = std::this_thread::get_id();
    int calls = 0;
    int step = 0;
    isle::Process process([&](isle::Yield& yield) {
        check(std::this_thread::get_id() != owner, "callback executes on its native worker");
        ++calls;
        step = 1;
        yield.wait(0);
        step = 2;
    });
    check(process.state() == State::ready && calls == 0, "construction parks without invoking callback");
    check(process.resume() == State::completed, "normal callback return completes first resume");
    check(step == 2 && calls == 1, "zero wait never returns control to owner");
    check(process.resume() == State::completed && calls == 1, "completed process never restarts");
    check(process.cancel() == State::completed, "cancelling a completed process preserves completion");

    bool rejected = false;
    try {
        isle::Process empty(isle::Process::Callback{});
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    check(rejected, "missing callback fails construction explicitly");
}

void nested_wait(isle::Yield& yield, int& outer, int& step, int& live, int& destroyed) {
    Lifetime lifetime(live, destroyed);
    int inner = 23;
    const auto* const address = &inner;
    step = 2;
    yield.wait(3);
    check(inner == 23 && &inner == address && outer == 11,
          "nested stack values and addresses survive a multi-iteration wait");
    inner += outer;
    outer = inner;
    step = 3;
    yield.wait();
    check(inner == 34 && outer == 34, "modified nested locals survive a second wait");
    step = 4;
}

void exact_waits_and_nested_locals() {
    int step = 0;
    int calls = 0;
    int live = 0;
    int destroyed = 0;
    isle::Process process([&](isle::Yield& yield) {
        ++calls;
        Lifetime lifetime(live, destroyed);
        int outer = 11;
        const auto* const address = &outer;
        step = 1;
        nested_wait(yield, outer, step, live, destroyed);
        check(&outer == address && outer == 34, "outer stack frame survives nested suspension");
        step = 5;
    });
    check(process.resume() == State::waiting && step == 2, "first iteration enters nested wait");
    check(live == 2 && destroyed == 0, "suspension retains both automatic lifetimes");
    check(process.resume() == State::waiting && step == 2, "wait three stays parked after one iteration");
    check(process.resume() == State::waiting && step == 2, "wait three stays parked after two iterations");
    check(process.resume() == State::waiting && step == 3,
          "wait three resumes exactly on the third subsequent scheduled iteration");
    check(process.resume() == State::completed && step == 5, "default wait resumes after one iteration");
    check(calls == 1 && live == 0 && destroyed == 2, "callback is entered once and returns through both frames");
}

void finish_nested(isle::Yield& yield, int& live, int& destroyed) {
    Lifetime lifetime(live, destroyed);
    yield.wait();
    yield.finish();
}

void self_finish() {
    int live = 0;
    int destroyed = 0;
    bool after_finish = false;
    isle::Process process([&](isle::Yield& yield) {
        Lifetime lifetime(live, destroyed);
        finish_nested(yield, live, destroyed);
        after_finish = true;
    });
    check(process.resume() == State::waiting && live == 2, "nested self-finish first suspends normally");
    check(process.resume() == State::completed, "nested self-finish reports completion, not failure");
    check(!after_finish && live == 0 && destroyed == 2, "self-finish unwinds all suspended automatic objects");
    isle::Process immediate([&](isle::Yield& yield) {
        Lifetime lifetime(live, destroyed);
        yield.finish();
    });
    check(immediate.resume() == State::completed && live == 0 && destroyed == 3,
          "self-finish also works before any suspension");
}

struct CallbackFailure { int value; };

bool receives_failure(isle::Process& process, int value) {
    try {
        process.resume();
    } catch (const CallbackFailure& failure) {
        return failure.value == value;
    } catch (...) {
        return false;
    }
    return false;
}

void exception_transport() {
    isle::Process immediate([](isle::Yield&) { throw CallbackFailure{17}; });
    check(receives_failure(immediate, 17), "callback entry failure reaches owner with original type and value");
    check(immediate.state() == State::failed, "entry failure has explicit failed state");
    check(receives_failure(immediate, 17), "later resume rethrows recorded failure without restarting");
    check(immediate.cancel() == State::failed, "cancellation cannot replace failure with success");

    int live = 0;
    int destroyed = 0;
    std::atomic<int> thread_exits{0};
    isle::Process suspended([&](isle::Yield& yield) {
        thread_local ThreadExit exit{thread_exits};
        Lifetime lifetime(live, destroyed);
        yield.wait();
        throw CallbackFailure{29};
    });
    check(suspended.resume() == State::waiting && live == 1, "failing callback first preserves suspended stack");
    check(receives_failure(suspended, 29), "failure after suspension reaches the owner");
    check(suspended.state() == State::failed && live == 0 && destroyed == 1,
          "failed callback unwinds before owner regains control");
    check(thread_exits.load(std::memory_order_relaxed) == 1, "failure joins worker through thread-local destruction");
}

void cancellation_before_entry() {
    bool entered = false;
    isle::Process process([&](isle::Yield&) { entered = true; });
    check(process.cancel() == State::cancelled, "cancellation wakes and joins a never-started worker");
    check(!entered && process.state() == State::cancelled, "pre-entry cancellation never runs callback");
    check(process.cancel() == State::cancelled && process.resume() == State::cancelled,
          "cancelled process remains terminal on repeated owner operations");
}

void cancellation_nested(isle::Yield& yield, int& live, int& destroyed, bool& after_wait) {
    Lifetime lifetime(live, destroyed);
    yield.wait(std::numeric_limits<std::uint64_t>::max());
    after_wait = true;
}

void cancellation_while_parked() {
    int live = 0;
    int destroyed = 0;
    int catch_count = 0;
    bool after_wait = false;
    std::atomic<int> thread_exits{0};
    isle::Process process([&](isle::Yield& yield) {
        thread_local ThreadExit exit{thread_exits};
        Lifetime lifetime(live, destroyed);
        try {
            cancellation_nested(yield, live, destroyed, after_wait);
        } catch (...) {
            ++catch_count;
            throw;
        }
    });
    check(process.resume() == State::waiting && live == 2, "large wait parks with nested stack intact");
    check(process.resume() == State::waiting && live == 2, "maximum wait count does not overflow or resume early");
    check(process.cancel() == State::cancelled, "parked cancellation wakes before joining");
    check(live == 0 && destroyed == 2 && catch_count == 1 && !after_wait,
          "cancellation unwinds nested objects through rethrowing application handlers");
    check(thread_exits.load(std::memory_order_relaxed) == 1, "cancellation joins through thread-local destruction");
    check(process.resume() == State::cancelled, "cancelled stack cannot be resumed or restarted");
}

void destruction_cancels() {
    bool entered = false;
    {
        isle::Process never_started([&](isle::Yield&) { entered = true; });
    }
    check(!entered, "destruction wakes and joins a ready worker without callback entry");

    int live = 0;
    int destroyed = 0;
    bool after_wait = false;
    std::atomic<int> thread_exits{0};
    {
        isle::Process parked([&](isle::Yield& yield) {
            thread_local ThreadExit exit{thread_exits};
            Lifetime lifetime(live, destroyed);
            cancellation_nested(yield, live, destroyed, after_wait);
        });
        check(parked.resume() == State::waiting && live == 2, "destructor fixture has a genuinely suspended stack");
    }
    check(live == 0 && destroyed == 2 && !after_wait, "destruction safely unwinds parked nested stack");
    check(thread_exits.load(std::memory_order_relaxed) == 1, "destruction retains ownership until worker fully exits");
}

void owner_and_yield_boundaries() {
    isle::Yield* suspended_yield = nullptr;
    isle::Process* self = nullptr;
    bool callback_rejected = false;
    isle::Process process([&](isle::Yield& yield) {
        suspended_yield = &yield;
        try {
            self->resume();
        } catch (const std::logic_error&) {
            callback_rejected = true;
        }
        yield.wait();
    });
    self = &process;
    bool resume_rejected = false;
    bool state_rejected = false;
    bool cancel_rejected = false;
    std::thread foreign([&] {
        try { process.resume(); } catch (const std::logic_error&) { resume_rejected = true; }
        try { process.state(); } catch (const std::logic_error&) { state_rejected = true; }
        try { process.cancel(); } catch (const std::logic_error&) { cancel_rejected = true; }
    });
    foreign.join();
    check(resume_rejected && state_rejected && cancel_rejected, "non-owner API calls fail explicitly without running callback");
    check(process.state() == State::ready, "rejected foreign operations leave process ready");
    check(process.resume() == State::waiting && callback_rejected, "callback cannot recursively become its own owner");
    bool wait_rejected = false;
    bool finish_rejected = false;
    try { suspended_yield->wait(0); } catch (const std::logic_error&) { wait_rejected = true; }
    try { suspended_yield->finish(); } catch (const std::logic_error&) { finish_rejected = true; }
    check(wait_rejected && finish_rejected, "even zero wait and finish require the actual callback worker");
    check(process.resume() == State::completed, "boundary errors do not damage later valid handoff");
}

void serialized_execution() {
    std::atomic<int> execution{1}; // 1 owner, 2 callback, 0 handoff
    std::atomic<int> phase{0};
    std::array<int, 8> trace{};
    std::size_t used = 0;
    const auto owner = std::this_thread::get_id();
    auto body = [&](isle::Yield& yield, int process_index) {
        const auto worker = std::this_thread::get_id();
        for (int iteration = 0; iteration < 4; ++iteration) {
            check(execution.exchange(2) == 0, "callback starts only after owner hands off execution");
            check(phase.load() == iteration * 2 + process_index, "callback cannot run before its scheduled owner iteration");
            check(worker != owner && worker == std::this_thread::get_id(), "process retains a single separate worker identity");
            trace[used++] = iteration * 2 + process_index;
            check(execution.exchange(0) == 2, "callback alone owns each callback execution segment");
            if (iteration != 3) {
                yield.wait();
            }
        }
    };
    isle::Process first([&](isle::Yield& yield) { body(yield, 0); });
    isle::Process second([&](isle::Yield& yield) { body(yield, 1); });
    for (int iteration = 0; iteration < 4; ++iteration) {
        for (int process_index = 0; process_index < 2; ++process_index) {
            phase.store(iteration * 2 + process_index);
            check(execution.exchange(0) == 1, "owner alone executes between handoffs");
            auto& process = process_index == 0 ? first : second;
            const auto result = process.resume();
            check(execution.exchange(1) == 0, "resume returns only after callback relinquishes execution");
            check(result == (iteration == 3 ? State::completed : State::waiting), "each serialized callback yields or returns at its boundary");
            check(used == static_cast<std::size_t>(iteration * 2 + process_index + 1), "exactly one callback segment runs per handoff");
        }
    }
    for (std::size_t i = 0; i < trace.size(); ++i) {
        check(trace[i] == static_cast<int>(i), "multiple processes follow only explicit owner order");
    }
}

struct CountingCallback {
    int& copies;
    int& moves;
    int& segments;
    CountingCallback(int& copy_count, int& move_count, int& segment_count)
        : copies(copy_count), moves(move_count), segments(segment_count) {}
    CountingCallback(const CountingCallback& other)
        : copies(other.copies), moves(other.moves), segments(other.segments) { ++copies; }
    CountingCallback(CountingCallback&& other) noexcept
        : copies(other.copies), moves(other.moves), segments(other.segments) { ++moves; }
    void operator()(isle::Yield& yield) {
        for (int i = 0; i < 128; ++i) {
            ++segments;
            yield.wait();
        }
    }
};

void callback_retention() {
    int copies = 0;
    int moves = 0;
    int segments = 0;
    isle::Process process(CountingCallback{copies, moves, segments});
    const int initial_copies = copies;
    const int initial_moves = moves;
    for (int i = 0; i < 128; ++i) {
        check(process.resume() == State::waiting && segments == i + 1, "retained callback continues from each prior wait");
    }
    check(process.resume() == State::completed, "retained callback returns after its final wait");
    check(copies == initial_copies && moves == initial_moves, "handoff never copies, moves, or reconstructs callback");
}

struct EnvironmentRestore {
    std::fenv_t environment{};
    bool captured = std::fegetenv(&environment) == 0;
    ~EnvironmentRestore() {
        if (captured) {
            std::fesetenv(&environment);
        }
    }
};

void floating_point_isolation() {
    EnvironmentRestore restore;
    check(restore.captured, "owner FP environment can be preserved for test cleanup");
    check(std::fesetenv(FE_DFL_ENV) == 0 && std::fesetround(FE_DOWNWARD) == 0,
          "owner installs a nondefault initial rounding mode");
    check(std::feraiseexcept(FE_INVALID) == 0, "owner installs its initial exception flag");
    isle::Process process([&](isle::Yield& yield) {
        check(std::fegetround() == FE_TONEAREST, "worker explicitly starts with default rounding, not creator rounding");
        check(std::fetestexcept(FE_ALL_EXCEPT) == 0, "worker explicitly starts with default exception flags");
        check(std::fesetround(FE_TOWARDZERO) == 0, "worker changes its private rounding mode");
        check(std::feraiseexcept(FE_DIVBYZERO) == 0, "worker changes its private exception flags");
        yield.wait(2);
        check(std::fegetround() == FE_TOWARDZERO, "worker rounding survives multi-iteration suspension");
        check(std::fetestexcept(FE_ALL_EXCEPT) == FE_DIVBYZERO, "worker flags survive suspension without owner contamination");
    });
    check(std::fesetround(FE_UPWARD) == 0, "owner can change rounding after process construction");
    check(process.resume() == State::waiting, "FP fixture genuinely suspends");
    check(std::fegetround() == FE_UPWARD && std::fetestexcept(FE_ALL_EXCEPT) == FE_INVALID,
          "callback FP changes cannot alter owner control or status");
    check(std::feclearexcept(FE_ALL_EXCEPT) == 0 && std::fesetround(FE_DOWNWARD) == 0,
          "owner changes its environment while worker is parked");
    check(std::feraiseexcept(FE_INVALID) == 0, "owner retains a distinct exception history");
    check(process.resume() == State::waiting, "FP wait countdown consumes one scheduled iteration");
    check(process.resume() == State::completed, "FP worker resumes on exact second iteration");
    check(std::fegetround() == FE_DOWNWARD && std::fetestexcept(FE_ALL_EXCEPT) == FE_INVALID,
          "FP restoration remains isolated after worker completion and join");
}

} // namespace

int main() {
    try {
        entry_zero_and_return();
        exact_waits_and_nested_locals();
        self_finish();
        exception_transport();
        cancellation_before_entry();
        cancellation_while_parked();
        destruction_cancels();
        owner_and_yield_boundaries();
        serialized_execution();
        callback_retention();
        floating_point_isolation();
    } catch (const std::exception& error) {
        check(false, error.what());
    } catch (...) {
        check(false, "unexpected nonstandard exception reached test runner");
    }
    if (failures != 0) {
        std::cerr << failures << " process checks failed\n";
        return 1;
    }
    std::cout << "process checks passed\n";
    return 0;
}
