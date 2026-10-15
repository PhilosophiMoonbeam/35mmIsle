#include "../src/required_work.hpp"

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
using namespace std::chrono_literals;

void check(bool condition, const char* description) {
    if (!condition) throw std::runtime_error(description);
}
// Only a deadlock bound, never a transition/scheduling oracle. Tests synchronize
// through latches and observed waiter counts, with no sleeps or elapsed-time guesses.
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
void waiters(RequiredChannel& channel, std::size_t capacity, std::size_t preparation,
             std::size_t result) {
    for (;;) {
        const auto stats = channel.stats();
        if (stats.capacity_waiters == capacity && stats.preparation_waiters == preparation &&
            stats.result_waiters == result) return;
        std::this_thread::yield();
    }
}
bool same(RequiredWritableView a, RequiredWritableView b) {
    return a.rdram.data() == b.rdram.data() && a.rdram.size() == b.rdram.size();
}
bool same(RequiredMemoryView a, RequiredMemoryView b) {
    return a.rdram.data() == b.rdram.data() && a.rdram.size() == b.rdram.size() &&
           a.hidden.data() == b.hidden.data() && a.hidden.size() == b.hidden.size();
}
bool same(const isle::rsp::Task& a, const isle::rsp::Task& b) {
    return a.type == b.type && a.flags == b.flags &&
           a.boot_address == b.boot_address && a.boot_size == b.boot_size &&
           a.text_address == b.text_address && a.text_size == b.text_size &&
           a.data_address == b.data_address && a.data_size == b.data_size &&
           a.stack_address == b.stack_address && a.stack_size == b.stack_size &&
           a.output_address == b.output_address && a.output_end == b.output_end &&
           a.commands_address == b.commands_address && a.commands_size == b.commands_size &&
           a.yield_address == b.yield_address && a.yield_size == b.yield_size;
}
bool same(const RequiredRequest& a, const RequiredRequest& b) {
    return a.token == b.token && same(a.task, b.task) &&
           a.boot.data() == b.boot.data() && a.boot.size() == b.boot.size();
}
// Small owned arrays test actual write/read borrows and input lifetimes. Main's
// publications below are explicit protocol transitions, not simulated RSP/GPU work.
struct Memory {
    std::array<std::uint32_t, 4> rdram{0x62626262u, 0, 0, 0};
    std::array<std::byte, 8> hidden{std::byte{0x37}};
    RequiredWritableView writable() { return {rdram}; }
    RequiredMemoryView view() const { return {std::as_bytes(std::span{rdram}), hidden}; }
};
struct Input {
    isle::rsp::Task task{1, 2, 0x100, 4, 0x200, 0x80, 0x300, 0x40,
                         0x400, 0x100, 0x500, 0x600, 0x700, 0x20, 0x800, 0x10};
    std::array<std::byte, 4> boot{std::byte{0x12}, std::byte{0x34}, std::byte{0x56}, std::byte{0x78}};
};
RequiredToken request(RequiredChannel& channel) {
    RequiredToken token;
    check(channel.request_preparation(token) == ClockError::none, "admit one preparation request");
    return token;
}
void map(RequiredChannel& channel, RequiredToken token, Memory& memory) {
    RequiredToken mapped;
    check(channel.take_preparation(mapped) && mapped == token, "main reserves exact preparation identity");
    check(channel.publish_preparation(token, memory.writable()) == ClockError::none,
          "main publishes its actual writable span");
}
RequiredWritableView prepare(RequiredChannel& channel, RequiredToken token, Memory& memory) {
    map(channel, token, memory);
    RequiredWritableView writer;
    check(channel.wait_preparation(token, writer) == ClockError::none && same(writer, memory.writable()),
          "owner borrows exact writable memory without a copy");
    return writer;
}
RequiredToken queue(RequiredChannel& channel, Memory& memory, const Input& input) {
    const auto token = request(channel);
    prepare(channel, token, memory);
    check(channel.submit(token, input.task, input.boot) == ClockError::none,
          "owner ceases CPU writes and submits task values plus borrowed boot");
    return token;
}
RequiredRequest take(RequiredChannel& channel, RequiredToken token, const Input& input) {
    RequiredRequest taken;
    const RequiredRequest expected{token, input.task, input.boot};
    check(channel.take(taken) && same(taken, expected), "main takes exact task value, boot span and identity");
    return taken;
}
void reserve(RequiredChannel& channel, RequiredToken token, Terminal outcome) {
    RequiredRetirement retirement;
    check(channel.take_retirement(retirement) && retirement.token == token && retirement.outcome == outcome,
          "main reserves exact retirement and actual outcome");
    check(!channel.quiescent(), "retirement reservation does not release capacity or resources");
}
void finish(RequiredChannel& channel, RequiredToken token, Terminal outcome) {
    reserve(channel, token, outcome);
    // Production main cancels/releases its actual backend lease at this boundary.
    check(channel.finish_retirement(token) == ClockError::none, "finish retirement after main lease release");
}
void no_retirement(RequiredChannel& channel) {
    const RequiredRetirement sentinel{{91, 92, 93}, Terminal::failed};
    auto out = sentinel;
    check(!channel.take_retirement(out) && out.token == sentinel.token && out.outcome == sentinel.outcome,
          "pending accesses cannot retire or overwrite a retirement output");
}
void cancel_requested(RequiredChannel& channel, RequiredToken token) {
    check(channel.close() == ClockError::none, "close an unreserved preparation");
    finish(channel, token, Terminal::cancelled);
}

void full_capacity_interval_and_task_transport() {
    RequiredChannel channel;
    Memory memory;
    Input input;
    check(channel.wait_capacity() == ClockError::none, "idle channel has capacity");
    const auto token = request(channel);
    const RequiredToken sentinel{91, 92, 93};
    auto untouched = sentinel;
    auto full = [&] {
        check(channel.request_preparation(untouched) == ClockError::full && untouched == sentinel &&
              !channel.quiescent() && channel.stats().capacity_waiters == 1 &&
              channel.stats().retirements == 0 && channel.stats().submissions == 1,
              "entire admitted lifetime retains the single slot and blocked capacity");
    };
    Worker capacity([&] { check(channel.wait_capacity() == ClockError::none, "capacity wakes after full main retirement"); });
    waiters(channel, 1, 0, 0);
    full();
    RequiredToken mapped;
    check(channel.take_preparation(mapped) && mapped == token, "reserve main mapping");
    full();
    untouched = sentinel;
    check(!channel.take_preparation(untouched) && untouched == sentinel, "duplicate mapping take preserves token");
    check(channel.publish_preparation(token, memory.writable()) == ClockError::none, "publish preparation grant");
    check(channel.stats().cpu_writes == 0, "unborrowed writable grant is not a CPU writer");
    full();
    RequiredWritableView writer;
    check(channel.wait_preparation(token, writer) == ClockError::none && same(writer, memory.writable()),
          "grant exact CPU writer");
    writer.rdram[2] = 0xa5a5a5a5u;
    full();
    const auto submitted = input.task;
    check(channel.submit(token, input.task, input.boot) == ClockError::none, "submit after final CPU write");
    input.task.flags ^= 0xff;
    input.task.output_end += 0x100;
    full();
    RequiredRequest execution;
    check(channel.take(execution) && execution.token == token && same(execution.task, submitted) &&
          execution.boot.data() == input.boot.data() && execution.boot.size() == input.boot.size(),
          "task is a value snapshot while boot retains exact caller-owned storage");
    const RequiredRequest sentinel_request{sentinel, input.task, input.boot};
    auto untouched_request = sentinel_request;
    check(!channel.take(untouched_request) && same(untouched_request, sentinel_request),
          "duplicate execution take leaves every output field unchanged");
    full();
    check(channel.publish_ready(token, memory.view()) == ClockError::none, "publish explicit result readiness");
    full();
    RequiredMemoryView reader;
    check(channel.wait_ready(token, reader) == ClockError::none && same(reader, memory.view()) &&
          reader.rdram[8] == std::byte{0xa5} && reader.hidden[0] == std::byte{0x37},
          "read-only borrow observes the same owned memory after writer cessation");
    full();
    check(channel.release_result(token, Terminal::completed) == ClockError::none, "reader ceases access");
    full();
    reserve(channel, token, Terminal::completed);
    full();
    no_retirement(channel);
    check(channel.stats().completed == 0, "outcome is not counted at retirement reservation");
    check(channel.finish_retirement(token) == ClockError::none, "main finishes real release boundary");
    capacity.join();
    check(channel.quiescent(), "only completed retirement reopens capacity");
    const auto next = request(channel);
    check(next.owner == token.owner && next.epoch == token.epoch && next.serial > token.serial,
          "next operation keeps owner/epoch and advances lifetime serial");
    cancel_requested(channel, next);
    const auto stats = channel.stats();
    check(stats.submissions == 2 && stats.cpu_writes == 1 && stats.gpu_ready == 1 && stats.cpu_borrows == 1 &&
          stats.retirements == 2 && stats.completed == 1 && stats.failed == 0 && stats.cancelled == 1,
          "one admission and one write/read borrow are counted per completed lifecycle");
}

void stale_operations(RequiredChannel& channel, RequiredToken token, Memory& memory, const Input& input) {
    auto writer = memory.writable();
    auto reader = memory.view();
    check(channel.wait_preparation(token, writer) == ClockError::stale && same(writer, memory.writable()) &&
          channel.wait_ready(token, reader) == ClockError::stale && same(reader, memory.view()),
          "stale/foreign waits preserve both output views");
    check(channel.publish_preparation(token, memory.writable()) == ClockError::stale &&
          channel.submit(token, input.task, input.boot) == ClockError::stale &&
          channel.abandon_preparation(token, Terminal::cancelled) == ClockError::stale &&
          channel.publish_ready(token, memory.view()) == ClockError::stale &&
          channel.publish_failed(token) == ClockError::stale &&
          channel.release_result(token, Terminal::completed) == ClockError::stale &&
          channel.finish_retirement(token) == ClockError::stale,
          "stale/foreign transitions cannot affect the occupied slot");
}
void exact_identity_ordering_duplicates_and_reset() {
    static_assert(!std::is_convertible_v<RequiredToken, FrameToken>);
    static_assert(!std::is_copy_constructible_v<RequiredChannel>);
    static_assert(!std::is_move_constructible_v<RequiredChannel>);
    RequiredChannel channel, foreign;
    Memory memory, sentinel;
    Input input;
    check(channel.reset() == ClockError::not_ready, "open channel cannot reset");
    const auto token = request(channel);
    const auto other = request(foreign);
    check(token.owner != other.owner, "channels issue distinct owner identities");
    stale_operations(channel, other, sentinel, input);
    auto invented = token;
    ++invented.serial;
    stale_operations(channel, invented, sentinel, input);
    auto reader = sentinel.view();
    check(channel.publish_preparation(token, memory.writable()) == ClockError::not_ready &&
          channel.submit(token, input.task, input.boot) == ClockError::not_ready &&
          channel.abandon_preparation(token, Terminal::cancelled) == ClockError::not_ready &&
          channel.publish_ready(token, memory.view()) == ClockError::not_ready &&
          channel.publish_failed(token) == ClockError::not_ready &&
          channel.release_result(token, Terminal::completed) == ClockError::not_ready &&
          channel.finish_retirement(token) == ClockError::not_ready &&
          channel.wait_ready(token, reader) == ClockError::not_ready && same(reader, sentinel.view()),
          "no transition skips preparation reservation or writer ownership");
    RequiredToken mapped;
    check(channel.take_preparation(mapped) && mapped == token, "reserve preparation for ordering checks");
    check(channel.publish_preparation(token, {}) == ClockError::invalid, "empty write mapping cannot grant ownership");
    check(channel.publish_preparation(token, memory.writable()) == ClockError::none &&
          channel.publish_preparation(token, memory.writable()) == ClockError::duplicate &&
          channel.submit(token, input.task, input.boot) == ClockError::not_ready,
          "publication occurs once and does not implicitly borrow the writer");
    RequiredWritableView writer;
    check(channel.wait_preparation(token, writer) == ClockError::none, "obtain writer for exact transition checks");
    writer = sentinel.writable();
    check(channel.wait_preparation(token, writer) == ClockError::duplicate && same(writer, sentinel.writable()) &&
          channel.publish_failed(token) == ClockError::not_ready &&
          channel.abandon_preparation(token, Terminal::completed) == ClockError::invalid &&
          channel.abandon_preparation(token, static_cast<Terminal>(255)) == ClockError::invalid,
          "duplicate/invalid transitions cannot revoke or manufacture a live writer");
    no_retirement(channel);
    check(channel.submit(token, input.task, input.boot) == ClockError::none &&
          channel.submit(token, input.task, input.boot) == ClockError::duplicate &&
          channel.abandon_preparation(token, Terminal::failed) == ClockError::duplicate,
          "only the first writer cessation may submit or abandon");
    check(channel.publish_ready(token, memory.view()) == ClockError::not_ready &&
          channel.publish_failed(token) == ClockError::not_ready, "queued work must first be taken for execution");
    take(channel, token, input);
    check(channel.publish_ready(token, {memory.view().rdram, {}}) == ClockError::invalid &&
          channel.publish_ready(token, {{}, memory.view().hidden}) == ClockError::invalid,
          "both read-only mappings are required without changing execution state");
    check(channel.publish_ready(token, memory.view()) == ClockError::none &&
          channel.publish_ready(token, memory.view()) == ClockError::duplicate &&
          channel.publish_failed(token) == ClockError::duplicate, "only one execution publication owns the result");
    check(channel.wait_ready(token, reader) == ClockError::none && same(reader, memory.view()), "obtain exact reader");
    reader = sentinel.view();
    check(channel.wait_ready(token, reader) == ClockError::duplicate && same(reader, sentinel.view()) &&
          channel.release_result(token, static_cast<Terminal>(255)) == ClockError::invalid &&
          channel.finish_retirement(token) == ClockError::not_ready,
          "duplicate reader and invalid outcomes leave the existing borrow live");
    check(channel.release_result(token, Terminal::completed) == ClockError::none &&
          channel.release_result(token, Terminal::failed) == ClockError::duplicate,
          "duplicate release cannot overwrite the original reader outcome");
    finish(channel, token, Terminal::completed);
    check(channel.finish_retirement(token) == ClockError::duplicate &&
          channel.submit(token, input.task, input.boot) == ClockError::duplicate &&
          channel.abandon_preparation(token, Terminal::cancelled) == ClockError::duplicate &&
          channel.release_result(token, Terminal::failed) == ClockError::duplicate,
          "fully retired identity cannot be reused without a new request");
    const auto second = request(channel);
    stale_operations(channel, token, sentinel, input);
    check(channel.close() == ClockError::none && channel.reset() == ClockError::not_ready,
          "acknowledged preparation still prevents epoch reset");
    reserve(channel, second, Terminal::cancelled);
    check(channel.reset() == ClockError::not_ready, "reserved actual main retirement prevents epoch reset");
    check(channel.finish_retirement(second) == ClockError::none && channel.reset() == ClockError::none &&
          channel.quiescent() && !channel.closed(), "only closed fully retired channel reopens with a new epoch");
    const auto third = request(channel);
    check(third.owner == token.owner && third.epoch != token.epoch && third.serial > second.serial,
          "reset changes epoch but never recycles owner or lifetime serial");
    stale_operations(channel, second, sentinel, input);
    cancel_requested(channel, third);
    cancel_requested(foreign, other);
    const auto stats = channel.stats();
    check(stats.submissions == 3 && stats.cpu_writes == 1 && stats.cpu_borrows == 1 &&
          stats.retirements == 3 && stats.completed == 1 && stats.cancelled == 2,
          "epoch reset preserves every lifetime counter");
}

void close_requested_wakes_before_join() {
    RequiredChannel channel;
    Memory sentinel;
    const auto token = request(channel);
    Worker preparation([&] {
        auto out = sentinel.writable();
        check(channel.wait_preparation(token, out) == ClockError::closed && same(out, sentinel.writable()),
              "closed request wakes preparation wait without granting a writer");
    });
    Worker capacity([&] { check(channel.wait_capacity() == ClockError::closed, "close wakes blocked admission"); });
    waiters(channel, 1, 1, 0);
    check(channel.close() == ClockError::none && channel.close() == ClockError::none, "close is idempotent before joins");
    preparation.join();
    capacity.join();
    const RequiredToken sentinel_token{55, 66, 77};
    auto out = sentinel_token;
    check(channel.request_preparation(out) == ClockError::closed && out == sentinel_token &&
          !channel.take_preparation(out) && out == sentinel_token,
          "closed admission/cancelled mapping take preserve output identity");
    check(channel.reset() == ClockError::not_ready && !channel.quiescent(), "early wake does not finish retirement");
    finish(channel, token, Terminal::cancelled);
    check(channel.quiescent() && channel.stats().cpu_writes == 0 && channel.stats().gpu_ready == 0,
          "requested cancellation never creates writer or execution ownership");
}

void close_pending_main_mapping() {
    for (const bool failed : {false, true}) {
        RequiredChannel channel;
        Memory memory, sentinel;
        const auto token = request(channel);
        std::latch mapping{1}, allow_publication{1};
        Worker main([&] {
            RequiredToken mapped;
            check(channel.take_preparation(mapped) && mapped == token, "main enters mapping reservation");
            mapping.count_down();
            allow_publication.wait();
            if (failed) check(channel.publish_failed(token) == ClockError::none, "main reports ceased failed mapping");
            else check(channel.publish_preparation(token, memory.writable()) == ClockError::none,
                       "main reports real mapping acquisition even after close");
        });
        mapping.wait();
        Worker preparation([&] {
            auto out = sentinel.writable();
            check(channel.wait_preparation(token, out) == ClockError::closed && same(out, sentinel.writable()),
                  "mapping waiter wakes early without owning the pending mapping");
        });
        Worker capacity([&] { check(channel.wait_capacity() == ClockError::closed, "mapping close wakes admission"); });
        waiters(channel, 1, 1, 0);
        check(channel.close() == ClockError::none, "close races deterministically with pending main mapping");
        preparation.join();
        capacity.join();
        no_retirement(channel);
        check(!channel.quiescent() && channel.reset() == ClockError::not_ready &&
              channel.stats().cpu_writes == 0 && channel.stats().retirements == 0,
              "closed/joined owner cannot retire or reset main's pending mapping");
        check(channel.publish_preparation(token, {}) == ClockError::invalid,
              "even closed mapping requires a valid publication or explicit failure");
        no_retirement(channel);
        allow_publication.count_down();
        main.join();
        auto out = sentinel.writable();
        check(channel.wait_preparation(token, out) == ClockError::closed && same(out, sentinel.writable()),
              "late mapping never grants a post-close writer");
        reserve(channel, token, failed ? Terminal::failed : Terminal::cancelled);
        check(channel.reset() == ClockError::not_ready, "late mapping lease still requires main cancellation/release");
        check(channel.finish_retirement(token) == ClockError::none && channel.quiescent(),
              "mapping becomes reusable only after explicit actual retirement boundary");
        const auto stats = channel.stats();
        check(stats.failed == (failed ? 1u : 0u) && stats.cancelled == (failed ? 0u : 1u) &&
              stats.cpu_writes == 0 && stats.gpu_ready == 0, "mapping failure remains failed during close");
    }
}

enum class WriterEnd { submit, cancel, fail };
void writer_cessation_and_close_races() {
    for (const auto ending : {WriterEnd::submit, WriterEnd::cancel, WriterEnd::fail}) {
        for (const bool close_first : {false, true}) {
            if (!close_first && ending == WriterEnd::submit) continue;
            RequiredChannel channel;
            Memory memory;
            Input input;
            const auto token = request(channel);
            RequiredToken mapped;
            check(channel.take_preparation(mapped) && mapped == token, "reserve live writer mapping");
            std::latch borrowed{1}, allow_cessation{1};
            Worker writer([&] {
                RequiredWritableView out;
                check(channel.wait_preparation(token, out) == ClockError::none && same(out, memory.writable()),
                      "writer acquires actual mutable span");
                out.rdram[0] = 0x91919191u;
                borrowed.count_down();
                allow_cessation.wait();
                check(out.rdram[0] == 0x91919191u, "live writer span survives concurrent close");
                out.rdram[1] = 0xe3e3e3e3u;
                if (ending == WriterEnd::submit)
                    check(channel.submit(token, input.task, input.boot) == ClockError::none,
                          "submit during close certifies final CPU write before acknowledging cancellation");
                else
                    check(channel.abandon_preparation(token, ending == WriterEnd::fail ? Terminal::failed : Terminal::cancelled)
                              == ClockError::none, "explicit writer abandonment follows final CPU access");
            });
            waiters(channel, 0, 1, 0);
            check(channel.publish_preparation(token, memory.writable()) == ClockError::none, "grant blocked writer");
            borrowed.wait();
            Worker capacity([&] { check(channel.wait_capacity() == ClockError::closed, "writer close/abandon wakes admission"); });
            waiters(channel, 1, 0, 0);
            if (close_first) {
                check(channel.close() == ClockError::none, "close while CPU still writes");
                capacity.join();
            }
            no_retirement(channel);
            check(!channel.quiescent() && channel.reset() == ClockError::not_ready &&
                  channel.publish_failed(token) == ClockError::not_ready,
                  "close and main failure publication cannot revoke a live writer");
            RequiredRequest untaken{{91, 92, 93}, input.task, input.boot};
            const auto sentinel_request = untaken;
            check(!channel.take(untaken) && same(untaken, sentinel_request), "live writer cannot execute prematurely");
            allow_cessation.count_down();
            writer.join();
            if (!close_first) capacity.join();
            check(memory.rdram[1] == 0xe3e3e3e3u && channel.closed() &&
                  !channel.take(untaken) && same(untaken, sentinel_request),
                  "ceased writer's final access precedes cancellation with no queued execution");
            const auto outcome = ending == WriterEnd::fail ? Terminal::failed : Terminal::cancelled;
            finish(channel, token, outcome);
            const auto stats = channel.stats();
            check(stats.cpu_writes == 1 && stats.gpu_ready == 0 && stats.cpu_borrows == 0 &&
                  stats.completed == 0 && stats.failed == (outcome == Terminal::failed ? 1u : 0u) &&
                  stats.cancelled == (outcome == Terminal::cancelled ? 1u : 0u),
                  "abandon/closed submit never masquerades as executed or completed work");
        }
    }
}

void close_unborrowed_grant_and_queued_task() {
    Memory memory, sentinel;
    Input input;
    {
        RequiredChannel channel;
        const auto token = request(channel);
        map(channel, token, memory);
        check(channel.close() == ClockError::none, "close an unborrowed preparation grant");
        auto out = sentinel.writable();
        check(channel.wait_preparation(token, out) == ClockError::closed && same(out, sentinel.writable()),
              "close cancels an unborrowed grant without manufacturing CPU access");
        finish(channel, token, Terminal::cancelled);
        check(channel.stats().cpu_writes == 0, "cancelled unborrowed mapping was never a writer");
    }
    RequiredChannel channel;
    const auto token = queue(channel, memory, input);
    Worker result([&] {
        auto out = sentinel.view();
        check(channel.wait_ready(token, out) == ClockError::closed && same(out, sentinel.view()),
              "queued result wait wakes on cancellation without a reader");
    });
    Worker capacity([&] { check(channel.wait_capacity() == ClockError::closed, "queued close wakes admission"); });
    waiters(channel, 1, 0, 1);
    check(channel.close() == ClockError::none, "cancel queued task before execution reservation");
    result.join();
    capacity.join();
    const RequiredRequest sentinel_request{{91, 92, 93}, input.task, input.boot};
    auto out = sentinel_request;
    check(!channel.take(out) && same(out, sentinel_request), "closed queued task cannot execute or overwrite output");
    finish(channel, token, Terminal::cancelled);
    check(channel.stats().cpu_writes == 1 && channel.stats().gpu_ready == 0 &&
          channel.stats().cpu_borrows == 0 && channel.stats().completed == 0,
          "prepared cancellation is distinct from execution/result completion");
}

void close_pending_main_execution() {
    for (const bool failed : {false, true}) {
        RequiredChannel channel;
        Memory memory, sentinel;
        Input input;
        const auto token = queue(channel, memory, input);
        std::latch executing{1}, allow_publication{1};
        Worker main([&] {
            const auto execution = take(channel, token, input);
            executing.count_down();
            allow_publication.wait();
            check(execution.boot.data() == input.boot.data() && execution.boot[2] == std::byte{0x56} &&
                  same(execution.task, input.task), "task and borrowed boot survive early-closed owner wait");
            if (failed) check(channel.publish_failed(token) == ClockError::none, "publish ceased execution failure");
            else check(channel.publish_ready(token, memory.view()) == ClockError::none,
                       "publish ceased execution readiness even after close");
        });
        executing.wait();
        Worker result([&] {
            auto out = sentinel.view();
            check(channel.wait_ready(token, out) == ClockError::closed && same(out, sentinel.view()),
                  "executing wait wakes early without claiming execution has stopped");
        });
        Worker capacity([&] { check(channel.wait_capacity() == ClockError::closed, "execution close wakes admission"); });
        waiters(channel, 1, 0, 1);
        check(channel.close() == ClockError::none, "close with main execution still pending");
        result.join();
        capacity.join();
        no_retirement(channel);
        check(!channel.quiescent() && channel.reset() == ClockError::not_ready && channel.stats().retirements == 0,
              "closed owner join cannot retire/reset pending main execution");
        allow_publication.count_down();
        main.join();
        auto out = sentinel.view();
        check(channel.wait_ready(token, out) == ClockError::closed && same(out, sentinel.view()) &&
              channel.stats().cpu_borrows == 0, "late execution publication grants no post-close reader");
        finish(channel, token, failed ? Terminal::failed : Terminal::cancelled);
        const auto stats = channel.stats();
        check(stats.gpu_ready == (failed ? 0u : 1u) && stats.failed == (failed ? 1u : 0u) &&
              stats.cancelled == (failed ? 0u : 1u) && stats.completed == 0,
              "main execution failure remains failed rather than shutdown cancellation");
    }
}

void close_ready_and_live_readers() {
    Memory memory, sentinel;
    Input input;
    {
        RequiredChannel channel;
        const auto token = queue(channel, memory, input);
        take(channel, token, input);
        check(channel.publish_ready(token, memory.view()) == ClockError::none &&
              channel.close() == ClockError::none, "close unborrowed ready result");
        auto out = sentinel.view();
        check(channel.wait_ready(token, out) == ClockError::closed && same(out, sentinel.view()),
              "unborrowed result cancels before CPU access");
        finish(channel, token, Terminal::cancelled);
        check(channel.stats().cpu_borrows == 0 && channel.stats().completed == 0,
              "ready alone is never successful consumption");
    }
    for (const auto outcome : {Terminal::completed, Terminal::cancelled, Terminal::failed}) {
        RequiredChannel channel;
        const auto token = queue(channel, memory, input);
        take(channel, token, input);
        check(channel.publish_ready(token, memory.view()) == ClockError::none, "publish result for live reader");
        std::latch borrowed{1}, allow_cessation{1};
        Worker reader([&] {
            RequiredMemoryView out;
            check(channel.wait_ready(token, out) == ClockError::none && same(out, memory.view()), "CPU borrows read-only spans");
            borrowed.count_down();
            allow_cessation.wait();
            check(out.rdram[0] == std::byte{0x62} && out.hidden[0] == std::byte{0x37},
                  "borrowed reader remains accessible through its last post-close read");
            check(channel.release_result(token, outcome) == ClockError::none, "reader explicitly reports cessation and outcome");
        });
        borrowed.wait();
        Worker capacity([&] { check(channel.wait_capacity() == ClockError::closed, "close wakes capacity without revoking reader"); });
        waiters(channel, 1, 0, 0);
        check(channel.close() == ClockError::none, "close while CPU reader is live");
        capacity.join();
        no_retirement(channel);
        check(!channel.quiescent() && channel.reset() == ClockError::not_ready &&
              channel.publish_failed(token) == ClockError::duplicate,
              "main cannot fail/reclaim/reset a borrowed result");
        allow_cessation.count_down();
        reader.join();
        check(channel.reset() == ClockError::not_ready, "reader acknowledgement still retains actual main lease");
        finish(channel, token, outcome);
        const auto stats = channel.stats();
        check(stats.completed == (outcome == Terminal::completed ? 1u : 0u) &&
              stats.cancelled == (outcome == Terminal::cancelled ? 1u : 0u) &&
              stats.failed == (outcome == Terminal::failed ? 1u : 0u) && channel.reset() == ClockError::none,
              "completed reader may finish during close; other outcomes remain non-completed");
    }
}

void reader_noncompletion_closes_admission() {
    for (const auto outcome : {Terminal::cancelled, Terminal::failed}) {
        RequiredChannel channel;
        Memory memory;
        Input input;
        const auto token = queue(channel, memory, input);
        take(channel, token, input);
        check(channel.publish_ready(token, memory.view()) == ClockError::none, "publish before reader noncompletion");
        std::latch borrowed{1}, allow_cessation{1};
        Worker reader([&] {
            RequiredMemoryView out;
            check(channel.wait_ready(token, out) == ClockError::none && same(out, memory.view()),
                  "reader failure/cancellation starts with a real protocol borrow");
            borrowed.count_down();
            allow_cessation.wait();
            check(out.hidden[0] == std::byte{0x37}, "noncompleted reader performs its last access before release");
            if (outcome == Terminal::failed) {
                try {
                    throw std::runtime_error("intentional CPU consumer exception");
                } catch (const std::runtime_error&) {
                    check(channel.release_result(token, Terminal::failed) == ClockError::none,
                          "CPU exception releases failed only after accesses cease");
                }
            } else {
                check(channel.release_result(token, Terminal::cancelled) == ClockError::none,
                      "CPU cancellation releases only after accesses cease");
            }
        });
        borrowed.wait();
        Worker capacity([&] { check(channel.wait_capacity() == ClockError::closed, "reader noncompletion closes blocked admission"); });
        waiters(channel, 1, 0, 0);
        check(!channel.closed(), "successful readiness and live reader have not closed admission");
        allow_cessation.count_down();
        reader.join();
        capacity.join();
        check(channel.closed() && channel.reset() == ClockError::not_ready,
              "reader noncompletion itself closes admission without releasing main's lease");
        finish(channel, token, outcome);
        const auto stats = channel.stats();
        check(stats.gpu_ready == 1 && stats.cpu_borrows == 1 && stats.completed == 0 &&
              stats.failed == (outcome == Terminal::failed ? 1u : 0u) &&
              stats.cancelled == (outcome == Terminal::cancelled ? 1u : 0u),
              "ready execution and failed/cancelled consumption retain distinct accounting");
    }
}

void main_failure_closes_and_wakes_waiters() {
    for (const bool preparing : {true, false}) {
        RequiredChannel channel;
        Memory memory, sentinel;
        Input input;
        RequiredToken token;
        if (preparing) {
            token = request(channel);
            RequiredToken mapped;
            check(channel.take_preparation(mapped) && mapped == token, "reserve mapping that will fail");
        } else {
            token = queue(channel, memory, input);
            take(channel, token, input);
        }
        Worker owner([&] {
            if (preparing) {
                auto out = sentinel.writable();
                check(channel.wait_preparation(token, out) == ClockError::closed && same(out, sentinel.writable()),
                      "mapping failure wakes preparation waiter without granting a writer");
            } else {
                auto out = sentinel.view();
                check(channel.wait_ready(token, out) == ClockError::closed && same(out, sentinel.view()),
                      "execution failure wakes result waiter without granting a reader");
            }
        });
        Worker capacity([&] { check(channel.wait_capacity() == ClockError::closed, "main failure wakes admission waiter"); });
        waiters(channel, 1, preparing ? 1 : 0, preparing ? 0 : 1);
        check(channel.publish_failed(token) == ClockError::none && channel.closed(), "main access cessation failure closes admission");
        owner.join();
        capacity.join();
        check(channel.publish_failed(token) == ClockError::duplicate &&
              channel.publish_preparation(token, memory.writable()) == ClockError::duplicate &&
              channel.publish_ready(token, memory.view()) == ClockError::duplicate,
              "later publications cannot replace a failed mapping/execution outcome");
        finish(channel, token, Terminal::failed);
        const auto stats = channel.stats();
        check(stats.failed == 1 && stats.cancelled == 0 && stats.completed == 0 &&
              stats.cpu_writes == (preparing ? 0u : 1u) && stats.gpu_ready == 0 && stats.cpu_borrows == 0,
              "actual main failure has no ready/read-borrow accounting");
    }
}

void competing_preparation_and_result_waiters() {
    RequiredChannel channel;
    Memory memory, sentinel;
    Input input;
    const auto token = request(channel);
    RequiredToken mapped;
    check(channel.take_preparation(mapped) && mapped == token, "reserve mapping for competing writer waits");
    std::array<ClockError, 2> writer_results{ClockError::invalid, ClockError::invalid};
    std::latch writers_returned{2}, allow_submit{1};
    auto wait_writer = [&](std::size_t index) {
        auto out = sentinel.writable();
        writer_results[index] = channel.wait_preparation(token, out);
        if (writer_results[index] == ClockError::none) check(same(out, memory.writable()), "winning writer gets exact mutable RAM");
        else check(writer_results[index] == ClockError::duplicate && same(out, sentinel.writable()), "losing writer gets no mutable view");
        writers_returned.count_down();
        if (writer_results[index] == ClockError::none) {
            allow_submit.wait();
            out.rdram[3] = 0xb4b4b4b4u;
            check(channel.submit(token, input.task, input.boot) == ClockError::none, "only winning writer submits after final write");
        }
    };
    Worker first_writer([&] { wait_writer(0); }), second_writer([&] { wait_writer(1); });
    waiters(channel, 0, 2, 0);
    check(channel.publish_preparation(token, memory.writable()) == ClockError::none, "one mapping wakes both preparation waiters");
    writers_returned.wait();
    check((writer_results[0] == ClockError::none) != (writer_results[1] == ClockError::none) &&
          channel.stats().cpu_writes == 1, "exactly one preparation waiter becomes CPU writer");
    no_retirement(channel);
    allow_submit.count_down();
    first_writer.join();
    second_writer.join();
    take(channel, token, input);
    std::array<ClockError, 2> reader_results{ClockError::invalid, ClockError::invalid};
    std::latch readers_returned{2}, allow_release{1};
    auto wait_reader = [&](std::size_t index) {
        auto out = sentinel.view();
        reader_results[index] = channel.wait_ready(token, out);
        if (reader_results[index] == ClockError::none)
            check(same(out, memory.view()) && out.rdram[12] == std::byte{0xb4}, "winning reader sees same ceased writer memory");
        else check(reader_results[index] == ClockError::duplicate && same(out, sentinel.view()), "losing reader gets no read-only borrow");
        readers_returned.count_down();
        if (reader_results[index] == ClockError::none) {
            allow_release.wait();
            check(channel.release_result(token, Terminal::completed) == ClockError::none, "only winning reader releases result");
        }
    };
    Worker first_reader([&] { wait_reader(0); }), second_reader([&] { wait_reader(1); });
    waiters(channel, 0, 0, 2);
    check(channel.publish_ready(token, memory.view()) == ClockError::none, "one execution publication wakes competing result waiters");
    readers_returned.wait();
    check((reader_results[0] == ClockError::none) != (reader_results[1] == ClockError::none) &&
          channel.stats().cpu_borrows == 1, "exactly one result waiter receives CPU read permission");
    check(channel.close() == ClockError::none, "close while winning reader still borrows");
    allow_release.count_down();
    first_reader.join();
    second_reader.join();
    finish(channel, token, Terminal::completed);
    check(channel.stats().completed == 1 && channel.stats().cancelled == 0,
          "genuine completed reader is not retroactively cancelled by close");
}
} // namespace

int main() {
    Watchdog watchdog;
    try {
        full_capacity_interval_and_task_transport();
        exact_identity_ordering_duplicates_and_reset();
        close_requested_wakes_before_join();
        close_pending_main_mapping();
        writer_cessation_and_close_races();
        close_unborrowed_grant_and_queued_task();
        close_pending_main_execution();
        close_ready_and_live_readers();
        reader_noncompletion_closes_admission();
        main_failure_closes_and_wakes_waiters();
        competing_preparation_and_result_waiters();
        std::cout << "Required-work preparation, exact identity, borrows, retirement and close regressions passed (transport only)\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
