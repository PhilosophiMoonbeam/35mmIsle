#include "35mmisle/process.hpp"

#include <cfenv>
#include <stdexcept>
#include <utility>

#if defined(_MSC_VER)
#pragma fenv_access(on)
#endif

namespace isle {
namespace {

bool terminal(Process::State state) noexcept {
    return state == Process::State::completed || state == Process::State::failed ||
           state == Process::State::cancelled;
}

} // namespace

void Yield::wait(std::uint64_t iterations) {
    process_.wait(iterations);
}

[[noreturn]] void Yield::finish() {
    process_.finish();
}

Process::Process(Callback callback)
    : owner_(std::this_thread::get_id()), callback_(std::move(callback)) {
    if (!callback_) {
        throw std::invalid_argument("a process requires a callback");
    }
    worker_ = std::thread([this] { run(); });
    try {
        std::unique_lock lock(mutex_);
        changed_.wait(lock, [this] { return parked_; });
    } catch (...) {
        cancel_and_join();
        throw;
    }
}

Process::~Process() noexcept {
    cancel_and_join();
}

void Process::require_owner() const {
    if (std::this_thread::get_id() != owner_) {
        throw std::logic_error("process operations require the constructing owner thread");
    }
}

void Process::require_worker() const {
    if (std::this_thread::get_id() != worker_id_) {
        throw std::logic_error("yield operations require their active process callback");
    }
}

Process::State Process::state() const {
    require_owner();
    std::lock_guard lock(mutex_);
    return state_;
}

Process::State Process::resume() {
    require_owner();
    std::unique_lock lock(mutex_);
    if (!terminal(state_)) {
        if (state_ == State::running) {
            throw std::logic_error("a running process cannot be resumed recursively");
        }
        if (state_ == State::waiting && remaining_ > 1) {
            --remaining_;
            return State::waiting;
        }
        remaining_ = 0;
        state_ = State::running;
        changed_.notify_one();
        changed_.wait(lock, [this] { return state_ != State::running; });
    }
    const auto result = state_;
    lock.unlock();
    if (terminal(result)) {
        join();
    }
    if (result == State::failed) {
        std::rethrow_exception(failure_);
    }
    return result;
}

Process::State Process::cancel() {
    require_owner();
    return cancel_and_join();
}

Process::State Process::cancel_and_join() {
    std::unique_lock lock(mutex_);
    if (!terminal(state_)) {
        cancellation_requested_ = true;
        changed_.notify_one();
        changed_.wait(lock, [this] { return terminal(state_); });
    }
    const auto result = state_;
    lock.unlock();
    join();
    return result;
}

void Process::join() {
    if (worker_.joinable()) {
        worker_.join();
    }
}

void Process::wait(std::uint64_t iterations) {
    require_worker();
    if (iterations == 0) {
        return;
    }
    std::unique_lock lock(mutex_);
    if (cancellation_requested_) {
        throw Cancelled{};
    }
    remaining_ = iterations;
    state_ = State::waiting;
    changed_.notify_one();
    changed_.wait(lock, [this] {
        return state_ == State::running || cancellation_requested_;
    });
    if (cancellation_requested_) {
        throw Cancelled{};
    }
}

[[noreturn]] void Process::finish() {
    require_worker();
    throw Finished{};
}

void Process::run() noexcept {
    std::unique_lock lock(mutex_);
    worker_id_ = std::this_thread::get_id();
    parked_ = true;
    changed_.notify_one();
    changed_.wait(lock, [this] {
        return state_ == State::running || cancellation_requested_;
    });
    if (cancellation_requested_) {
        state_ = State::cancelled;
        lock.unlock();
        changed_.notify_one();
        return;
    }
    lock.unlock();

    State result = State::completed;
    std::exception_ptr failure;
    try {
        if (std::fesetenv(FE_DFL_ENV) != 0) {
            throw std::runtime_error("cannot initialize process floating-point environment");
        }
        Yield yield(*this);
        callback_(yield);
    } catch (const Cancelled&) {
        result = State::cancelled;
    } catch (const Finished&) {
        result = State::completed;
    } catch (...) {
        result = State::failed;
        failure = std::current_exception();
    }

    lock.lock();
    // A callback must rethrow cancellation, but cannot manufacture successful
    // completion by swallowing it and returning normally.
    if (result == State::completed && cancellation_requested_) {
        result = State::cancelled;
    }
    failure_ = std::move(failure);
    state_ = result;
    lock.unlock();
    changed_.notify_one();
}

} // namespace isle
