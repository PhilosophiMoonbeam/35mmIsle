#ifndef ISLE_MESSAGE_QUEUE_HPP
#define ISLE_MESSAGE_QUEUE_HPP

#include <condition_variable>
#include <cstddef>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace isle {

enum class QueueResult { accepted, full, empty, closed };

template <class T>
struct QueuePop {
    QueueResult result;
    // Engaged exactly when result is accepted, including for a null owned pointer.
    std::optional<T> payload;
};

struct QueueState {
    std::size_t capacity;
    // Accepted messages still owned by the queue, not processing/completion state.
    std::size_t pending;
    std::size_t waiting_pushes;
    std::size_t waiting_pops;
    bool closed;
};

// Original bounded MPMC ownership queue, not an N64 priority scheduler. An
// accepted push means ownership admission only, never execution or completion.
// The ring is allocated once at construction; operations do not allocate queue
// storage, erase payload types, or copy payloads. The queue invokes no user
// callbacks. T needs only nonthrowing move construction and destruction, not
// default construction or assignment. Payload move/destruction executes under
// the mutex: it MUST be nonblocking and MUST NOT reenter this queue. Allocations
// made by T remain T's concern. FIFO admission and removal use that same mutex;
// no reservation/publication stages or out-of-order transfer states exist.
//
// close() rejects later admissions and wakes all waiters. Earlier admissions
// remain drainable in FIFO order before pop reports closed.
// close() does not join users or complete their work. Before destruction, the
// owner MUST prevent new access and join all producers/consumers. Destruction
// releases any undrained ownership; payload code must not reenter a destructing
// queue. The destructor cannot safely substitute for that lifetime protocol.
template <class T>
class MessageQueue final {
    static_assert(std::is_nothrow_move_constructible_v<T>,
                  "MessageQueue payloads must have nonthrowing move construction");
    static_assert(std::is_nothrow_destructible_v<T>,
                  "MessageQueue payloads must have nonthrowing destruction");

public:
    // Zero capacity is a configuration error, not an always-full queue.
    explicit MessageQueue(std::size_t capacity)
        : capacity_(checked_capacity(capacity)),
          slots_(std::make_unique<std::optional<T>[]>(capacity_)) {}

    ~MessageQueue() {
        // User-owned destruction runs with no queue lock, while its members live.
        slots_.reset();
    }

    MessageQueue(const MessageQueue&) = delete;
    MessageQueue& operator=(const MessageQueue&) = delete;
    MessageQueue(MessageQueue&&) = delete;
    MessageQueue& operator=(MessageQueue&&) = delete;

    // A failed push never moves from payload. A successful push transfers it
    // once, after admission; the caller must not access it concurrently.
    [[nodiscard]] QueueResult push(T&& payload) {
        return push_impl(std::move(payload), true);
    }

    [[nodiscard]] QueueResult try_push(T&& payload) {
        return push_impl(std::move(payload), false);
    }

    // Nonblocking means no wait for queue space/data, not a lock-free operation.
    // A pop transfers ownership into its result without default construction.
    [[nodiscard]] QueuePop<T> pop() { return pop_impl(true); }
    [[nodiscard]] QueuePop<T> try_pop() { return pop_impl(false); }

    // Idempotent; never destroys accepted payloads or waits for their processing.
    void close() {
        {
            std::lock_guard lock(mutex_);
            closed_ = true;
        }
        can_push_.notify_all();
        can_pop_.notify_all();
    }

    // A diagnostic snapshot, not a reservation or a readiness/completion promise.
    // Waiting counts describe threads actually entering the condition wait,
    // not merely threads about to call a queue operation.
    [[nodiscard]] QueueState state() const {
        std::lock_guard lock(mutex_);
        return {capacity_, pending_, waiting_pushes_, waiting_pops_, closed_};
    }

private:
    struct WaitCount {
        std::size_t& count;
        explicit WaitCount(std::size_t& value) noexcept : count(value) { ++count; }
        ~WaitCount() { --count; }
        WaitCount(const WaitCount&) = delete;
        WaitCount& operator=(const WaitCount&) = delete;
    };

    // Construct the result directly in the caller, then commit removal before
    // unlocking. A named local result could add a payload move without NRVO.
    struct PopCommit {
        MessageQueue& queue;
        std::unique_lock<std::mutex>& lock;

        ~PopCommit() {
            queue.slots_[queue.head_].reset();
            queue.head_ = queue.next(queue.head_);
            --queue.pending_;
            lock.unlock();
            queue.can_push_.notify_one();
        }
    };

    static std::size_t checked_capacity(std::size_t capacity) {
        if (capacity == 0) {
            throw std::invalid_argument("MessageQueue capacity must be positive");
        }
        return capacity;
    }

    std::size_t next(std::size_t index) const noexcept {
        return index == capacity_ - 1 ? 0 : index + 1;
    }

    QueueResult push_impl(T&& payload, bool blocking) {
        std::unique_lock lock(mutex_);
        if (blocking && !closed_ && pending_ == capacity_) {
            WaitCount waiting(waiting_pushes_);
            can_push_.wait(lock, [this] { return closed_ || pending_ < capacity_; });
        }
        if (closed_) {
            return QueueResult::closed;
        }
        if (pending_ == capacity_) {
            return QueueResult::full;
        }
        slots_[tail_].emplace(std::move(payload));
        tail_ = next(tail_);
        ++pending_;
        lock.unlock();
        can_pop_.notify_one();
        return QueueResult::accepted;
    }

    QueuePop<T> pop_impl(bool blocking) {
        std::unique_lock lock(mutex_);
        if (blocking && !closed_ && pending_ == 0) {
            WaitCount waiting(waiting_pops_);
            can_pop_.wait(lock, [this] { return closed_ || pending_ != 0; });
        }
        if (pending_ == 0) {
            return {closed_ ? QueueResult::closed : QueueResult::empty, std::nullopt};
        }
        PopCommit commit{*this, lock};
        return {QueueResult::accepted,
                std::optional<T>{std::in_place, std::move(*slots_[head_])}};
    }

    const std::size_t capacity_;
    std::unique_ptr<std::optional<T>[]> slots_;
    mutable std::mutex mutex_;
    std::condition_variable can_push_;
    std::condition_variable can_pop_;
    std::size_t head_ = 0;
    std::size_t tail_ = 0;
    std::size_t pending_ = 0;
    std::size_t waiting_pushes_ = 0;
    std::size_t waiting_pops_ = 0;
    bool closed_ = false;
};

} // namespace isle

#endif
