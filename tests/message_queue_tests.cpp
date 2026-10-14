#include "35mmisle/message_queue.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <barrier>
#include <cstddef>
#include <iostream>
#include <latch>
#include <memory>
#include <optional>
#include <stdexcept>
#include <thread>
#include <utility>

namespace {

using isle::MessageQueue;
using isle::QueuePop;
using isle::QueueResult;

int failures = 0;

void check(bool condition, const char* description) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << description << '\n';
    }
}

struct Item {
    int id;
    std::atomic<unsigned>& destroyed;

    ~Item() { destroyed.fetch_add(1); }
};

// No default constructor, copy operation, or move assignment: the queue must
// transfer ownership with move construction, without a dummy output payload.
struct OwnedMessage {
    std::unique_ptr<Item> item;
    std::atomic<unsigned>& moves;

    OwnedMessage(int id, std::atomic<unsigned>& destroyed, std::atomic<unsigned>& moved)
        : item(new Item{id, destroyed}), moves(moved) {}

    OwnedMessage(OwnedMessage&& other) noexcept
        : item(std::move(other.item)), moves(other.moves) {
        moves.fetch_add(1);
    }

    OwnedMessage(const OwnedMessage&) = delete;
    OwnedMessage& operator=(const OwnedMessage&) = delete;
    OwnedMessage& operator=(OwnedMessage&&) = delete;
};

int id(const QueuePop<OwnedMessage>& popped) {
    return popped.payload && popped.payload->item ? popped.payload->item->id : -1;
}

template <class T>
void wait_for_waiters(MessageQueue<T>& queue, std::size_t pushes, std::size_t pops) {
    // Barriers/latches establish entry; the real wait-state snapshot proves the
    // operation has released the mutex into its condition wait. No timing guess
    // or sleep is used, and no callback/test hook runs inside the queue lock.
    for (;;) {
        const auto state = queue.state();
        if (state.waiting_pushes == pushes && state.waiting_pops == pops) {
            return;
        }
        std::this_thread::yield();
    }
}

void reject_zero_capacity() {
    bool rejected = false;
    try {
        MessageQueue<std::unique_ptr<int>> queue(0);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    check(rejected, "zero capacity must be rejected explicitly");
}

void owned_admission_and_fifo_wraparound() {
    std::atomic<unsigned> destroyed{0};
    std::atomic<unsigned> moves{0};
    {
        MessageQueue<OwnedMessage> queue(3);
        auto empty = queue.try_pop();
        check(empty.result == QueueResult::empty && !empty.payload,
              "an open empty queue must report empty without fabricating a payload");
        OwnedMessage first(1, destroyed, moves);
        OwnedMessage second(2, destroyed, moves);
        OwnedMessage third(3, destroyed, moves);
        OwnedMessage fourth(4, destroyed, moves);
        auto* first_owner = first.item.get();
        auto* fourth_owner = fourth.item.get();
        check(queue.try_push(std::move(first)) == QueueResult::accepted && !first.item,
              "successful nonblocking push must transfer owned payload");
        check(moves.load() == 1, "admission must move the payload exactly once");
        check(queue.push(std::move(second)) == QueueResult::accepted && !second.item,
              "blocking push into available space must transfer ownership");
        check(queue.try_push(std::move(third)) == QueueResult::accepted && !third.item,
              "every capacity slot must hold an accepted payload");
        const auto moves_before_full = moves.load();
        check(queue.try_push(std::move(fourth)) == QueueResult::full,
              "a saturated queue must report full, not accepted");
        check(fourth.item.get() == fourth_owner && fourth.item->id == 4 &&
                  moves.load() == moves_before_full,
              "full rejection must leave the caller's exact ownership unmoved");
        {
            auto popped = queue.try_pop();
            check(popped.result == QueueResult::accepted && id(popped) == 1 &&
                      popped.payload->item.get() == first_owner,
                  "pop must transfer the oldest accepted allocation, not a copy");
            check(moves.load() == moves_before_full + 1,
                  "pop must construct its returned ownership with exactly one move");
        }
        check(destroyed.load() == 1, "dequeued ownership must be released only by its recipient");
        check(queue.try_push(std::move(fourth)) == QueueResult::accepted && !fourth.item,
              "a released ring slot must accept ownership after wraparound");
        auto two = queue.pop();
        auto three = queue.try_pop();
        auto four = queue.pop();
        check(two.result == QueueResult::accepted && id(two) == 2 &&
                  three.result == QueueResult::accepted && id(three) == 3 &&
                  four.result == QueueResult::accepted && id(four) == 4 &&
                  four.payload->item.get() == fourth_owner,
              "FIFO order and ownership identity must survive ring wraparound");
        auto empty_after = queue.try_pop();
        check(empty_after.result == QueueResult::empty && !empty_after.payload,
              "fully consumed open queue must return empty again");
        check(queue.state().pending == 0 && queue.state().capacity == 3,
              "consumed queue must retain its fixed capacity with no pending messages");
    }
    check(destroyed.load() == 4, "each accepted owned allocation must be destroyed exactly once");
}

void close_drains_without_dropping_ownership() {
    std::atomic<unsigned> destroyed{0};
    std::atomic<unsigned> moves{0};
    {
        MessageQueue<OwnedMessage> queue(2);
        OwnedMessage first(10, destroyed, moves);
        OwnedMessage second(20, destroyed, moves);
        OwnedMessage rejected(30, destroyed, moves);
        auto* rejected_owner = rejected.item.get();
        check(queue.push(std::move(first)) == QueueResult::accepted &&
                  queue.push(std::move(second)) == QueueResult::accepted,
              "messages to drain must actually have been accepted");
        queue.close();
        queue.close();
        check(queue.state().closed && queue.state().pending == 2 && destroyed.load() == 0,
              "idempotent close must retain all accepted ownership for draining");
        const auto before_rejection = moves.load();
        check(queue.try_push(std::move(rejected)) == QueueResult::closed &&
                  queue.push(std::move(rejected)) == QueueResult::closed,
              "close must reject blocking and nonblocking admission even when full");
        check(rejected.item.get() == rejected_owner && moves.load() == before_rejection,
              "closed rejection must leave caller ownership unmoved");
        auto first_out = queue.pop();
        auto second_out = queue.try_pop();
        check(first_out.result == QueueResult::accepted && id(first_out) == 10 &&
                  second_out.result == QueueResult::accepted && id(second_out) == 20,
              "close must drain already accepted messages in FIFO order");
        auto closed = queue.pop();
        auto also_closed = queue.try_pop();
        check(closed.result == QueueResult::closed && !closed.payload &&
                  also_closed.result == QueueResult::closed && !also_closed.payload,
              "only drained closed queues may report closed without a payload");
        check(queue.try_push(std::move(rejected)) == QueueResult::closed &&
                  rejected.item.get() == rejected_owner,
              "free slots after close must never reopen admission");
    }
    check(destroyed.load() == 3, "drained and rejected ownership must each release exactly once");
    {
        MessageQueue<OwnedMessage> queue(1);
        OwnedMessage undrained(40, destroyed, moves);
        check(queue.push(std::move(undrained)) == QueueResult::accepted,
              "undrained lifetime check must own an admitted message");
        queue.close();
        check(destroyed.load() == 3, "close must not destroy an undrained payload");
    }
    check(destroyed.load() == 4, "joined-user queue destruction must release undrained ownership");
}

void close_wakes_all_blocked_producers() {
    constexpr std::size_t count = 4;
    std::atomic<unsigned> destroyed{0};
    std::atomic<unsigned> moves{0};
    MessageQueue<OwnedMessage> queue(1);
    OwnedMessage initial(1, destroyed, moves);
    check(queue.push(std::move(initial)) == QueueResult::accepted,
          "producer wake test must begin with real full storage");
    std::array<std::optional<OwnedMessage>, count> inputs;
    std::array<QueueResult, count> outcomes{};
    std::array<std::thread, count> workers;
    std::barrier start(static_cast<std::ptrdiff_t>(count + 1));
    std::latch finished(static_cast<std::ptrdiff_t>(count));
    for (std::size_t i = 0; i < count; ++i) {
        inputs[i].emplace(static_cast<int>(i + 10), destroyed, moves);
        workers[i] = std::thread([&, i] {
            start.arrive_and_wait();
            outcomes[i] = queue.push(std::move(*inputs[i]));
            finished.count_down();
        });
    }
    start.arrive_and_wait();
    wait_for_waiters(queue, count, 0);
    check(!finished.try_wait() && moves.load() == 1,
          "full storage must block every producer without moving its payload");
    queue.close();
    finished.wait();
    for (auto& worker : workers) {
        worker.join();
    }
    for (std::size_t i = 0; i < count; ++i) {
        check(outcomes[i] == QueueResult::closed && inputs[i]->item &&
                  inputs[i]->item->id == static_cast<int>(i + 10),
              "close must wake every blocked producer with unmoved rejected ownership");
    }
    check(queue.state().waiting_pushes == 0 && moves.load() == 1,
          "all rejected producer waits must end without ownership moves");
    auto drained = queue.pop();
    check(drained.result == QueueResult::accepted && id(drained) == 1,
          "waking rejected producers must not discard the previously admitted message");
}

void close_wakes_all_blocked_consumers() {
    constexpr std::size_t count = 4;
    MessageQueue<std::unique_ptr<int>> queue(1);
    std::array<std::optional<QueuePop<std::unique_ptr<int>>>, count> outcomes;
    std::array<std::thread, count> workers;
    std::barrier start(static_cast<std::ptrdiff_t>(count + 1));
    std::latch finished(static_cast<std::ptrdiff_t>(count));
    for (std::size_t i = 0; i < count; ++i) {
        workers[i] = std::thread([&, i] {
            start.arrive_and_wait();
            outcomes[i].emplace(queue.pop());
            finished.count_down();
        });
    }
    start.arrive_and_wait();
    wait_for_waiters(queue, 0, count);
    check(!finished.try_wait(), "every empty-queue consumer must genuinely be blocked");
    queue.close();
    finished.wait();
    for (auto& worker : workers) {
        worker.join();
    }
    for (const auto& outcome : outcomes) {
        check(outcome && outcome->result == QueueResult::closed && !outcome->payload,
              "close must wake every blocked consumer without a fake accepted payload");
    }
    check(queue.state().waiting_pops == 0, "all closed consumer waits must end");
}

void admission_and_release_wake_waiters() {
    MessageQueue<std::unique_ptr<int>> queue(1);
    auto first = std::make_unique<int>(1);
    auto second = std::make_unique<int>(2);
    auto* second_owner = second.get();
    check(queue.push(std::move(first)) == QueueResult::accepted, "initial ownership must enter queue");
    QueueResult producer_result = QueueResult::empty;
    std::latch producer_entered(1);
    std::latch producer_finished(1);
    std::thread producer([&] {
        producer_entered.count_down();
        producer_result = queue.push(std::move(second));
        producer_finished.count_down();
    });
    producer_entered.wait();
    wait_for_waiters(queue, 1, 0);
    auto first_out = queue.pop();
    producer_finished.wait();
    producer.join();
    auto second_out = queue.pop();
    check(first_out.result == QueueResult::accepted && first_out.payload &&
                  *first_out.payload && **first_out.payload == 1,
          "space release must deliver the preceding owned message");
    check(producer_result == QueueResult::accepted && !second && second_out.payload &&
                  second_out.payload->get() == second_owner,
          "space release must wake a producer and actually transfer its ownership");

    std::optional<QueuePop<std::unique_ptr<int>>> consumed;
    std::latch consumer_entered(1);
    std::latch consumer_finished(1);
    std::thread consumer([&] {
        consumer_entered.count_down();
        consumed.emplace(queue.pop());
        consumer_finished.count_down();
    });
    consumer_entered.wait();
    wait_for_waiters(queue, 0, 1);
    auto third = std::make_unique<int>(3);
    auto* third_owner = third.get();
    check(queue.push(std::move(third)) == QueueResult::accepted,
          "consumer wake must come from a real accepted ownership transfer");
    consumer_finished.wait();
    consumer.join();
    check(consumed && consumed->result == QueueResult::accepted && consumed->payload &&
                  consumed->payload->get() == third_owner && **consumed->payload == 3,
          "admission must wake a consumer with the exact accepted ownership");
    queue.close();
}

void close_races_preserve_every_admission_and_ownership() {
    constexpr std::size_t count = 4;
    constexpr int rounds = 32;
    std::atomic<unsigned> destroyed{0};
    std::atomic<unsigned> moves{0};
    for (int round = 0; round < rounds; ++round) {
        {
            MessageQueue<OwnedMessage> queue(count);
            std::array<std::optional<OwnedMessage>, count> inputs;
            std::array<Item*, count> owners{};
            std::array<QueueResult, count> admissions{};
            std::array<std::thread, count> workers;
            std::barrier phase(static_cast<std::ptrdiff_t>(count + 1));
            for (std::size_t i = 0; i < count; ++i) {
                inputs[i].emplace(static_cast<int>(i), destroyed, moves);
                owners[i] = inputs[i]->item.get();
                workers[i] = std::thread([&, i] {
                    phase.arrive_and_wait();
                    admissions[i] = queue.push(std::move(*inputs[i]));
                    phase.arrive_and_wait();
                });
            }
            // Do not assume which participant wins this mutex race. Either
            // legal linearization must preserve the exact ownership outcome.
            phase.arrive_and_wait();
            queue.close();
            phase.arrive_and_wait();
            for (auto& worker : workers) {
                worker.join();
            }
            std::size_t accepted = 0;
            for (std::size_t i = 0; i < count; ++i) {
                if (admissions[i] == QueueResult::accepted) {
                    ++accepted;
                    check(!inputs[i]->item, "a racing accepted push must transfer caller ownership");
                } else {
                    check(admissions[i] == QueueResult::closed && inputs[i]->item.get() == owners[i],
                          "a racing close rejection must preserve the exact caller allocation");
                }
            }
            check(queue.state().pending == accepted,
                  "close race must retain exactly the accepted admissions");
            std::array<bool, count> received{};
            for (std::size_t i = 0; i < accepted; ++i) {
                auto popped = queue.pop();
                const auto value = id(popped);
                check(popped.result == QueueResult::accepted && value >= 0 &&
                          value < static_cast<int>(count),
                      "every racing accepted push must yield a real drainable message");
                if (value >= 0 && value < static_cast<int>(count)) {
                    const auto index = static_cast<std::size_t>(value);
                    check(!received[index] && admissions[index] == QueueResult::accepted &&
                              popped.payload->item.get() == owners[index],
                          "race drain must transfer each accepted allocation exactly once");
                    received[index] = true;
                }
            }
            auto closed = queue.pop();
            check(closed.result == QueueResult::closed && !closed.payload,
                  "racing close must report closed only after all accepted contents drain");
        }
        {
            MessageQueue<OwnedMessage> queue(count);
            std::array<std::optional<QueuePop<OwnedMessage>>, count> outputs;
            std::array<std::thread, count> workers;
            std::barrier phase(static_cast<std::ptrdiff_t>(count + 1));
            for (std::size_t i = 0; i < count; ++i) {
                OwnedMessage message(static_cast<int>(i), destroyed, moves);
                check(queue.push(std::move(message)) == QueueResult::accepted,
                      "consumer close race must start with real accepted contents");
            }
            for (std::size_t i = 0; i < count; ++i) {
                workers[i] = std::thread([&, i] {
                    phase.arrive_and_wait();
                    outputs[i].emplace(queue.pop());
                    phase.arrive_and_wait();
                });
            }
            phase.arrive_and_wait();
            queue.close();
            phase.arrive_and_wait();
            for (auto& worker : workers) {
                worker.join();
            }
            std::array<int, count> received{};
            for (std::size_t i = 0; i < count; ++i) {
                check(outputs[i] && outputs[i]->result == QueueResult::accepted,
                      "close racing consumers must not revoke any previously accepted message");
                received[i] = outputs[i] ? id(*outputs[i]) : -1;
            }
            std::sort(received.begin(), received.end());
            for (std::size_t i = 0; i < count; ++i) {
                check(received[i] == static_cast<int>(i),
                      "consumer close race must neither drop nor duplicate admitted ownership");
            }
            auto closed = queue.pop();
            check(closed.result == QueueResult::closed && !closed.payload,
                  "consumer close race must terminate closed after complete FIFO removal");
        }
    }
    check(destroyed.load() == static_cast<unsigned>(rounds * count * 2),
          "racing admission and drain must give every allocation exactly one final destruction");
}

void concurrent_producers_and_consumers_transfer_every_payload() {
    constexpr std::size_t count = 4;
    constexpr int rounds = 64;
    std::atomic<unsigned> destroyed{0};
    std::atomic<unsigned> moves{0};
    MessageQueue<OwnedMessage> queue(2);
    std::array<std::optional<OwnedMessage>, count> inputs;
    std::array<QueueResult, count> admissions{};
    std::array<std::optional<QueuePop<OwnedMessage>>, count> outputs;
    std::array<std::thread, count * 2> workers;
    std::barrier phase(static_cast<std::ptrdiff_t>(count * 2 + 1));
    for (std::size_t i = 0; i < count; ++i) {
        workers[i] = std::thread([&, i] {
            for (int round = 0; round < rounds; ++round) {
                phase.arrive_and_wait();
                admissions[i] = queue.push(std::move(*inputs[i]));
                phase.arrive_and_wait();
            }
        });
        workers[i + count] = std::thread([&, i] {
            for (int round = 0; round < rounds; ++round) {
                phase.arrive_and_wait();
                outputs[i].emplace(queue.pop());
                phase.arrive_and_wait();
            }
        });
    }
    for (int round = 0; round < rounds; ++round) {
        for (std::size_t i = 0; i < count; ++i) {
            inputs[i].emplace(round * static_cast<int>(count) + static_cast<int>(i), destroyed, moves);
        }
        phase.arrive_and_wait();
        phase.arrive_and_wait();
        std::array<int, count> received{};
        for (std::size_t i = 0; i < count; ++i) {
            check(admissions[i] == QueueResult::accepted && !inputs[i]->item,
                  "each concurrent producer must transfer its accepted owned message");
            check(outputs[i] && outputs[i]->result == QueueResult::accepted,
                  "each concurrent consumer must receive a real accepted message");
            received[i] = outputs[i] ? id(*outputs[i]) : -1;
        }
        std::sort(received.begin(), received.end());
        for (std::size_t i = 0; i < count; ++i) {
            check(received[i] == round * static_cast<int>(count) + static_cast<int>(i),
                  "concurrent wraparound must transfer every payload exactly once without drops or duplicates");
            outputs[i].reset();
        }
        const auto state = queue.state();
        check(state.pending == 0 && state.waiting_pushes == 0 && state.waiting_pops == 0,
              "completed barrier phase must leave no admitted messages or blocked operations");
    }
    for (auto& worker : workers) {
        worker.join();
    }
    queue.close();
    check(destroyed.load() == static_cast<unsigned>(rounds * count),
          "every concurrently accepted allocation must have exactly one final owner destruction");
}

} // namespace

int main() {
    try {
        reject_zero_capacity();
        owned_admission_and_fifo_wraparound();
        close_drains_without_dropping_ownership();
        close_wakes_all_blocked_producers();
        close_wakes_all_blocked_consumers();
        admission_and_release_wake_waiters();
        close_races_preserve_every_admission_and_ownership();
        concurrent_producers_and_consumers_transfer_every_payload();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: unexpected queue test exception: " << error.what() << '\n';
        return 1;
    } catch (...) {
        std::cerr << "FAIL: unexpected non-standard queue test exception\n";
        return 1;
    }
    if (failures != 0) {
        std::cerr << failures << " owned queue behavioral check(s) failed.\n";
        return 1;
    }
    std::cout << "Owned queue behavioral checks passed; admission is not task completion or N64 scheduler fidelity.\n";
    return 0;
}
