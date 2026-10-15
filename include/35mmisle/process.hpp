#ifndef ISLE_PROCESS_HPP
#define ISLE_PROCESS_HPP

#include <condition_variable>
#include <cstdint>
#include <exception>
#include <functional>
#include <mutex>
#include <thread>

namespace isle {

class Process;

// Valid only on its callback's worker, and only during that callback. Do not
// retain this reference, use it on another thread, or copy it into other work.
class Yield final {
public:
    Yield(const Yield&) = delete;
    Yield& operator=(const Yield&) = delete;

    // Zero stays inline. A positive count suspends this exact stack until that
    // many subsequent owner resume() calls; no clock or scheduler is consulted.
    void wait(std::uint64_t iterations = 1);

    // End the callback immediately, unwinding its automatic objects as for a
    // return. This also works from nested helpers; it is not a process restart.
    [[noreturn]] void finish();

    // Synchronously service an action on the owner inside the current resume,
    // without a wait tick or callback dispatch. The action may construct or
    // cancel another process, but cannot resume callbacks or cancel this one.
    // Exceptions return to this worker. No action storage is allocated/copied.
    void on_owner(void (*action)(void*), void* payload);

private:
    friend class Process;
    explicit Yield(Process& process) noexcept : process_(process) {}
    Process& process_;
};

// A stack-preserving continuation implemented by a strictly serialized handoff
// to one parked, joinable native thread. The constructing thread is the sole
// owner: only it may resume, inspect, cancel, or destroy this nonmovable object.
// resume() does not return while callback code is running. Multiple processes
// with the same owner therefore cannot execute callbacks concurrently.
//
// Cancellation is cooperative: cancel()/destruction wake a parked worker,
// unwind its suspended stack, and join it. They never detach or kill a thread.
// The owner is blocked during a running callback, so non-yielding user code
// cannot be forcibly interrupted. Callback code must not destroy its Process.
// wait()/finish() use private control exceptions: application catch(...) blocks
// must rethrow, and user destructors must not throw. Do not suspend while an
// exception is already unwinding. A callback exception is retained and rethrown
// by resume(), including every subsequent resume() of a failed process;
// cancellation/destruction do not rethrow an already recorded failure.
//
// The worker explicitly installs FE_DFL_ENV before callback entry, independently
// of the creator's FP environment. Its FP environment, including rounding and
// exception flags, remains private across suspension; owner changes do not
// overwrite it. This is NOT a Snap FP initialization/equivalence claim.
// Callback arithmetic also needs compiler settings honoring dynamic FP control.
// Original admission must identify
// the initial R4300 FPCSR (rounding, enables, flags, and denormal/flush mode) and
// any later changes, then establish their native mapping and arithmetic rules.
// Likewise native stack size, guard/overflow behavior and supported sanitizer
// evidence still require platform admission; std::thread uses host defaults.
//
// Callback storage and the native worker are created once. Successful resumes
// and waits perform no dynamic allocation or callback copy. Error transport and
// one-time cancellation/finish unwinding use the C++ exception runtime.
class Process final {
public:
    enum class State { ready, running, waiting, completed, failed, cancelled };
    using Callback = std::function<void(Yield&)>;

    explicit Process(Callback callback);
    ~Process() noexcept;
    Process(const Process&) = delete;
    Process& operator=(const Process&) = delete;
    Process(Process&&) = delete;
    Process& operator=(Process&&) = delete;

    // One scheduled iteration. The first call enters the callback. Calls while
    // waiting consume its count, resuming user code exactly when it reaches
    // zero. Terminal calls never restart it; failed calls rethrow the failure.
    State resume();
    State state() const;

    // Synchronously cancel a ready/waiting process and join its worker.
    // Completed/failed/cancelled states are retained. No failure is suppressed:
    // state() still reports failed and resume() still rethrows it afterward.
    State cancel();

private:
    friend class Yield;
    struct Cancelled {};
    struct Finished {};

    void require_owner() const;
    void require_worker() const;
    void wait(std::uint64_t iterations);
    [[noreturn]] void finish();
    void on_owner(void (*action)(void*), void* payload);
    void run() noexcept;
    State cancel_and_join();
    void join();

    const std::thread::id owner_;
    Callback callback_;
    mutable std::mutex mutex_;
    std::condition_variable changed_;
    State state_ = State::ready;
    std::uint64_t remaining_ = 0;
    bool parked_ = false;
    bool cancellation_requested_ = false;
    std::exception_ptr failure_;
    void (*owner_action_)(void*) = nullptr;
    void* owner_payload_ = nullptr;
    std::exception_ptr owner_failure_;
    std::thread::id worker_id_;
    std::thread worker_;
};

} // namespace isle

#endif
