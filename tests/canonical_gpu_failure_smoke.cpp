// ROM/SDK-free authored RSP -> real canonical RDP consumer. CPU Vulkan only.
// This tests the pinned renderer's LoadTile4bpp validation failure and actual
// task/CPU-only completed lease reuse, NOT device loss, a prolonged GPU drain,
// Source producer reachability, or a hardware renderer. No output files are used.
#include "rsp_render.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <mutex>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

namespace {
using Executor = isle::rsp::CanonicalExecutor;
using Clock = std::chrono::steady_clock;
constexpr std::uint32_t commands_address = 0x7000;
constexpr std::uint32_t texture_address = 0x8000;
constexpr std::uint32_t color_address = 0x300000;
constexpr unsigned width = 8, height = 8;
constexpr std::size_t color_bytes = width * height * 2;
constexpr std::uint32_t guard_word = 0xa55aa55a;
constexpr std::uint16_t fill_pixel = 0xf801;
constexpr auto poll_deadline = std::chrono::seconds(30);
constexpr auto poll_call_limit = std::chrono::milliseconds(250);

void require(bool condition, const char* why) {
    if (!condition) throw std::runtime_error(why);
}

// Blocking APIs intentionally run on their creating thread. A stuck call is a
// failing process, never a synthesized completion or an unsafe detached owner.
// Normal paths keep every owner alive until actual retirement has returned.
class Watchdog {
public:
    Watchdog() : thread_([this] {
        std::unique_lock lock(mutex_);
        if (!condition_.wait_for(lock, std::chrono::seconds(120), [this] { return finished_; })) {
            std::fputs("canonical_gpu_failure_smoke: FAIL: 120s hard timeout; retirement did not finish\n", stderr);
            std::fflush(stderr);
            std::_Exit(EXIT_FAILURE);
        }
    }) {}
    ~Watchdog() {
        {
            std::lock_guard lock(mutex_);
            finished_ = true;
        }
        condition_.notify_one();
        thread_.join();
    }
    Watchdog(const Watchdog&) = delete;
    Watchdog& operator=(const Watchdog&) = delete;
private:
    std::mutex mutex_;
    std::condition_variable condition_;
    bool finished_ = false;
    std::thread thread_;
};

// Original scalar instructions, not imported microcode. MTC0 writes DPC start
// and end so the real RSP transport reads the owned RDRAM command stream.
struct Program {
    std::vector<std::byte> bytes;
    void emit(std::uint32_t word) {
        for (int shift = 24; shift >= 0; shift -= 8)
            bytes.push_back(std::byte((word >> shift) & 255u));
    }
    void write(unsigned cp0, std::uint32_t value) {
        emit(0x3c010000u | (value >> 16));             // LUI r1, high
        emit(0x34210000u | (value & 65535u));         // ORI r1, r1, low
        emit(0x40810000u | (cp0 << 11));              // MTC0 r1, cp0
    }
    explicit Program(std::size_t command_words) {
        write(8, commands_address);
        write(9, commands_address + static_cast<std::uint32_t>(command_words * 4));
        emit(0x0000000du);                           // BREAK
    }
};

isle::rsp::Task make_task(const Program& program, std::size_t command_words) {
    isle::rsp::Task task{};
    task.type = 1;
    task.flags = 4;
    task.boot_address = 0x1000;
    task.boot_size = static_cast<std::uint32_t>(program.bytes.size());
    task.stack_address = 0x10000;
    task.stack_size = 0x400;
    task.output_address = 0x20000;
    task.output_end = 0x21000;
    task.commands_address = commands_address;
    task.commands_size = static_cast<std::uint32_t>(command_words * 4);
    return task;
}

// RGBA, size=4bpp, width=1; RGBA4 tile 0, line=1; inclusive
// (slo,tlo)=(shi,thi)=(0,0) loads ONE pixel, not an empty rectangle.
// The pinned renderer rejects the actual 4bpp VRAM source in load_tile_iteration.
constexpr std::array<std::uint32_t, 8> invalid_load{
    0xfd000000, texture_address, // SetTextureImage RGBA4
    0xf5000200, 0,               // SetTile RGBA4, line 1, TMEM 0, tile 0
    0xf4000000, 0,               // LoadTile, one pixel
    0xe9000000, 0                // FullSync (a command, not a completion fence)
};
constexpr std::array<std::uint32_t, 14> simple_fill{
    0xe7000000, 0,               // PipeSync
    0xff100007, color_address,   // SetColorImage RGBA16, width 8
    0xed000000, 0x00020020,      // Scissor [0,8) x [0,8)
    0xef300000, 0,               // Fill cycle
    0xf7000000, 0xf801f801,      // Opaque red RGBA5551
    0xf601c01c, 0,               // Inclusive fill rectangle [0,7] x [0,7]
    0xe9000000, 0                // FullSync
};

Executor::PreparationLease prepare(Executor& executor,
                                    std::span<const std::uint32_t> commands,
                                    bool cold) {
    auto lease = executor.begin_preparation();
    {
        auto words = lease.words();
        if (cold) std::fill(words.begin(), words.end(), 0u);
        std::copy(commands.begin(), commands.end(), words.begin() + commands_address / 4);
        words[texture_address / 4] = 0x12345678;
        // Distinct initial pixels and guards ensure that an unchanged mapping
        // or an out-of-bounds fill cannot satisfy the successful draw oracle.
        std::fill_n(words.begin() + color_address / 4, color_bytes / 4, 0x11223344u);
        std::fill_n(words.begin() + color_address / 4 - 4, 4, guard_word);
        std::fill_n(words.begin() + (color_address + color_bytes) / 4, 4, guard_word);
    } // No RAM borrower survives the subsequent begin/execute.
    return lease;
}

template<class Function>
std::string expect_error(Function&& function) {
    try {
        std::forward<Function>(function)();
    } catch (const std::exception& error) {
        const std::string observed = error.what();
        return observed;
    }
    throw std::runtime_error("operation unexpectedly succeeded instead of throwing its required diagnostic");
}

void require_poisoned_quiescent(Executor& executor) {
    require(executor.quiescent(), "failed execution did not become quiescent after retirement");
    expect_error([&] { auto preparation = executor.begin_preparation(); });
    require(executor.quiescent(), "rejected preparation changed quiescence");
}

void asynchronous_failure() {
    Executor executor(true, {10000, 64});
    const Program program(invalid_load.size());
    const auto task = make_task(program, invalid_load.size());
    auto preparation = prepare(executor, invalid_load, true);
    const auto token = executor.begin(std::move(preparation), task, program.bytes);
    require(token != 0 && !preparation, "begin did not consume preparation and issue a real token");
    require(!executor.quiescent(), "outstanding GPU token was reported quiescent");
    const auto started = Clock::now();
    std::size_t polls = 0, pending = 0;
    std::chrono::nanoseconds maximum_poll{};
    std::string observed;
    bool failed = false;
    bool timed_out = false;
    while (!failed) {
        const auto before = Clock::now();
        bool ready = false;
        try {
            ready = executor.poll(token);
        } catch (const std::exception& error) {
            observed = error.what();
            failed = true;
        }
        maximum_poll = std::max(maximum_poll,
            std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - before));
        ++polls;
        require(!ready, "invalid LoadTile unexpectedly published successful completion");
        if (failed) break;
        ++pending;
        require(!executor.quiescent(), "pending/failure-drain token was prematurely quiescent");
        if (Clock::now() - started >= poll_deadline) {
            timed_out = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    if (timed_out) {
        // Retire the real owner before reporting deadline failure. Even a real
        // diagnostic from this later blocking drain cannot turn timeout into PASS.
        expect_error([&] { executor.retire(token); });
        throw std::runtime_error("30s asynchronous deadline exceeded; blocking cleanup retired the token");
    }
    require(pending > 0, "asynchronous failure did not expose a pending poll before terminal failure");
    require_poisoned_quiescent(executor);
    expect_error([&] { (void)executor.poll(token); });
    Executor::MemoryLease escaped;
    expect_error([&] { escaped = executor.completed(token); });
    require(!escaped, "failed token exposed a MemoryLease");
    require(maximum_poll <= poll_call_limit, "poll exceeded its 250ms nonblocking-call budget");
    std::cout << "async pending_polls=" << pending << " total_polls=" << polls
              << " max_poll_ns=" << maximum_poll.count()
              << " poll_budget_ns=" << std::chrono::duration_cast<std::chrono::nanoseconds>(poll_call_limit).count()
              << " token_rejections=2 preparation_rejections=1 escaped_memory_leases=0 diagnostic=\""
              << observed << "\"\n";
}

void blocking_retire_failure() {
    Executor executor(true, {10000, 64});
    const Program program(invalid_load.size());
    auto preparation = prepare(executor, invalid_load, true);
    const auto token = executor.begin(std::move(preparation), make_task(program, invalid_load.size()), program.bytes);
    // No external poll: retire must drive owner handoff and worker drain itself.
    const auto observed = expect_error([&] { executor.retire(token); });
    require_poisoned_quiescent(executor);
    expect_error([&] { executor.retire(token); });
    std::cout << "retire external_polls=0 token_rejections=1 preparation_rejections=1 diagnostic=\""
              << observed << "\"\n";
}

void blocking_wait_failure() {
    Executor executor(true, {10000, 64});
    const Program program(invalid_load.size());
    auto preparation = prepare(executor, invalid_load, true);
    const auto token = executor.begin(std::move(preparation), make_task(program, invalid_load.size()), program.bytes);
    Executor::MemoryLease escaped;
    // No external poll: this acquisition must drive owner handoff and the
    // worker's real failure drain before throwing, without exposing mappings.
    const auto observed = expect_error([&] { escaped = executor.wait_completed(token); });
    require(!escaped && !preparation, "failed wait exposed memory or restored consumed preparation");
    require_poisoned_quiescent(executor);
    expect_error([&] { escaped = executor.wait_completed(token); });
    require(!escaped, "consumed failed wait token exposed a memory lease");
    std::cout << "wait_completed external_polls=0 token_rejections=1 preparation_rejections=1 "
                 "escaped_memory_leases=0 diagnostic=\"" << observed << "\"\n";
}

void blocking_execute_failure() {
    Executor executor(true, {10000, 64});
    const Program program(invalid_load.size());
    auto preparation = prepare(executor, invalid_load, true);
    Executor::MemoryLease escaped;
    const auto observed = expect_error([&] {
        escaped = executor.execute(std::move(preparation), make_task(program, invalid_load.size()), program.bytes);
    });
    require(!escaped && !preparation, "failed execute exposed memory or retained preparation");
    require_poisoned_quiescent(executor);
    std::cout << "execute external_polls=0 preparation_rejections=1 escaped_memory_leases=0 diagnostic=\""
              << observed << "\"\n";
}

void destructor_failure() {
    const Program program(invalid_load.size());
    {
        Executor executor(true, {10000, 64});
        auto preparation = prepare(executor, invalid_load, true);
        const auto token = executor.begin(std::move(preparation), make_task(program, invalid_load.size()), program.bytes);
        require(token != 0 && !preparation && !executor.quiescent(),
                "destructor case did not leave a real outstanding token");
        // No poll, retire, execute or later callback. The same actual invalid
        // stream reaches the backend; the destructor itself must retire it.
    }
    // The public noexcept destructor intentionally suppresses its exception.
    // Do not count it as an observed API diagnostic or claim a GPU drain duration.
    std::cout << "destructor outstanding_submission_scope_returned=1 external_polls=0 "
                 "diagnostic=not_exposed_by_destructor_api\n";
}

std::uint8_t guest_byte(std::span<const std::byte> ram, std::size_t address) {
    return std::to_integer<std::uint8_t>(ram[address ^ 3u]);
}

struct Draw {
    std::array<std::uint16_t, width * height> pixels{};
    std::size_t packets = 0, packet_words = 0, full_syncs = 0;
};
Draw completed_draw(Executor& executor, const Executor::MemoryLease& memory,
                    std::span<const std::uint32_t> expected_commands = simple_fill,
                    std::uint16_t expected_pixel = fill_pixel) {
    require(bool(memory), "successful fill did not return actual completed mappings");
    require(!executor.quiescent(), "active successful result lease was reported quiescent");
    Draw draw;
    {
        const auto& result = memory.rsp_result();
        require(bool(result), "successful fill has an RSP error");
        draw.packets = result.packets;
        draw.packet_words = result.packet_words;
        draw.full_syncs = result.full_syncs;
        require(draw.packets == expected_commands.size() / 2 &&
                draw.packet_words == expected_commands.size() && draw.full_syncs == 1,
                "successful fill's actual RSP packet counters disagree with the authored stream");
        require(memory.packets().words().size() == expected_commands.size() &&
                std::equal(expected_commands.begin(), expected_commands.end(), memory.packets().words().begin()),
                "successful real RSP packet contents changed");
        const auto ram = memory.rdram();
        require(ram.size() == (8u << 20) && memory.hidden().size() == (4u << 20),
                "successful fill did not expose full actual memory mappings");
        for (std::size_t index = 0; index < draw.pixels.size(); ++index) {
            const auto address = color_address + index * 2;
            draw.pixels[index] = static_cast<std::uint16_t>(
                (std::uint16_t(guest_byte(ram, address)) << 8) | guest_byte(ram, address + 1));
            require(draw.pixels[index] == expected_pixel, "actual fill pixels do not match the authored RGBA5551 value");
        }
        for (std::size_t byte = 0; byte < 16; ++byte) {
            const auto expected = static_cast<std::uint8_t>(guard_word >> ((3u - (byte & 3u)) * 8));
            require(guest_byte(ram, color_address - 16 + byte) == expected &&
                    guest_byte(ram, color_address + color_bytes + byte) == expected,
                    "actual fill corrupted a neighboring RDRAM guard");
        }
    } // Every mapping/reference borrower ends before releasing the owner lease.
    return draw;
}

Draw successful_draw(Executor& executor, bool cold) {
    const Program program(simple_fill.size());
    auto preparation = prepare(executor, simple_fill, cold);
    auto memory = executor.execute(std::move(preparation), make_task(program, simple_fill.size()), program.bytes);
    const auto draw = completed_draw(executor, memory);
    memory.release();
    require(executor.quiescent(), "successful lease release did not restore quiescence");
    return draw;
}

void cold_warm_success() {
    // Also proves a fresh real CPU Vulkan/JIT session remains usable after the
    // preceding destructor case. Warm execution reuses this same executor/RAM.
    Executor executor(true, {10000, 64});
    const auto cold = successful_draw(executor, true);
    const auto warm = successful_draw(executor, false);
    require(cold.pixels == warm.pixels, "cold and warm actual fill output differs");
    std::cout << "fill cold_pixels=" << cold.pixels.size() << " warm_pixels=" << warm.pixels.size()
              << " cold_packets=" << cold.packets << " warm_packets=" << warm.packets
              << " cold_packet_words=" << cold.packet_words << " warm_packet_words=" << warm.packet_words
              << " cold_full_syncs=" << cold.full_syncs << " warm_full_syncs=" << warm.full_syncs
              << " rgba5551=0xf801 guards=unchanged cold_warm=equal\n";
}

void cpu_finish_success() {
    Executor executor(true, {10000, 64});
    const Program program(simple_fill.size());
    const auto task = make_task(program, simple_fill.size());
    auto commands = simple_fill;
    constexpr std::uint16_t first_pixel = fill_pixel & ~1u;
    // Clearing the RGBA16 low bit makes the real fill establish non-cold hidden
    // history. The oracle compares actual completed bytes, not authored hidden.
    commands[9] = (std::uint32_t(first_pixel) << 16) | first_pixel;
    auto preparation = prepare(executor, commands, true);
    const auto first_token = executor.begin(std::move(preparation), task, program.bytes);
    require(first_token != 0 && !preparation, "first real task did not consume preparation");
    expect_error([&] { auto invalid = executor.wait_completed(0); });
    expect_error([&] { auto invalid = executor.wait_completed(first_token + 1); });
    require(!executor.quiescent(), "wrong wait token released the real outstanding task");
    auto memory = executor.wait_completed(first_token);
    const auto hidden_fingerprint = [](std::span<const std::byte> hidden) {
        std::uint64_t value = 14695981039346656037ull;
        for (const auto byte : hidden)
            value = (value ^ std::to_integer<std::uint8_t>(byte)) * 1099511628211ull;
        return value;
    };
    std::uintptr_t rdram_identity = 0, hidden_identity = 0;
    std::uint64_t hidden_history = 0;
    isle::rsp::Result actual_result{};
    std::array<isle::rsp::PacketRange, simple_fill.size() / 2> actual_ranges{};
    {
        (void)completed_draw(executor, memory, commands, first_pixel);
        actual_result = memory.rsp_result();
        const auto& packets = memory.packets();
        require(packets.packets().size() == actual_ranges.size(),
                "first task did not retain its exact authored packet ranges");
        std::copy(packets.packets().begin(), packets.packets().end(), actual_ranges.begin());
        const auto ram = memory.rdram(), hidden = memory.hidden();
        rdram_identity = reinterpret_cast<std::uintptr_t>(ram.data());
        hidden_identity = reinterpret_cast<std::uintptr_t>(hidden.data());
        require(std::any_of(hidden.begin(), hidden.end(),
                           [](std::byte value) { return value != std::byte{3}; }),
                "actual fill did not establish non-cold hidden history");
        hidden_history = hidden_fingerprint(hidden);
    }
    expect_error([&] { auto invalid = executor.begin_preparation(); });
    memory.release();
    require(executor.quiescent(), "wait-completed result release did not restore quiescence");
    expect_error([&] { auto invalid = executor.wait_completed(first_token); });

    constexpr std::uint32_t marker_address = texture_address + 4;
    constexpr std::uint32_t marker = 0x5a0123ef;
    preparation = executor.begin_preparation();
    {
        auto words = preparation.words();
        words[marker_address / 4] = marker;
        // CPU-only preparation may replace task bytes. A fabricated execution
        // would now encounter this genuinely invalid RDP stream.
        std::copy(invalid_load.begin(), invalid_load.end(), words.begin() + commands_address / 4);
    }
    expect_error([&] { auto invalid = executor.finish_preparation(std::move(preparation), 0); });
    require(bool(preparation) && preparation.words()[marker_address / 4] == marker,
            "zero CPU-finish receipt consumed or changed preparation");
    expect_error([&] { auto invalid = executor.finish_preparation(std::move(preparation), first_token + 1); });
    require(bool(preparation) && preparation.words()[marker_address / 4] == marker,
            "wrong CPU-finish receipt consumed or changed preparation");
    memory = executor.finish_preparation(std::move(preparation), first_token);
    require(!preparation && bool(memory) && !executor.quiescent(),
            "CPU finish did not transfer exclusive preparation into a completed result");
    {
        (void)completed_draw(executor, memory, commands, first_pixel);
        const auto& result = memory.rsp_result();
        require(result.error == actual_result.error && result.sp_status == actual_result.sp_status &&
                result.dma_reads == actual_result.dma_reads && result.dma_writes == actual_result.dma_writes &&
                result.imem_writes == actual_result.imem_writes && result.packets == actual_result.packets &&
                result.packet_words == actual_result.packet_words && result.full_syncs == actual_result.full_syncs,
                "CPU finish replaced the prior actual task result");
        const auto& packets = memory.packets();
        require(packets.packets().size() == actual_ranges.size() &&
                std::equal(actual_ranges.begin(), actual_ranges.end(), packets.packets().begin()),
                "CPU finish replaced the prior actual packet ranges");
        const auto ram = memory.rdram(), hidden = memory.hidden();
        require(reinterpret_cast<std::uintptr_t>(ram.data()) == rdram_identity &&
                reinterpret_cast<std::uintptr_t>(hidden.data()) == hidden_identity &&
                hidden_fingerprint(hidden) == hidden_history,
                "CPU finish replaced persistent RDRAM/hidden storage or changed hidden history");
        for (std::size_t byte = 0; byte < sizeof(marker); ++byte)
            require(guest_byte(ram, marker_address + byte) ==
                    static_cast<std::uint8_t>(marker >> ((3u - byte) * 8)),
                    "CPU finish discarded preparation writes");
    }
    expect_error([&] { auto invalid = executor.begin_preparation(); });
    memory.release();
    require(executor.quiescent(), "CPU-finished result release did not restore quiescence");

    preparation = prepare(executor, simple_fill, false);
    require(preparation.words()[marker_address / 4] == marker,
            "next preparation invalidated CPU-finished writeback");
    const auto second_token = executor.begin(std::move(preparation), task, program.bytes);
    require(second_token == first_token + 1, "CPU finish fabricated an intervening task token");
    memory = executor.wait_completed(second_token);
    {
        (void)completed_draw(executor, memory);
        require(reinterpret_cast<std::uintptr_t>(memory.rdram().data()) == rdram_identity &&
                reinterpret_cast<std::uintptr_t>(memory.hidden().data()) == hidden_identity,
                "next healthy actual task replaced persistent executor storage");
    }
    memory.release();
    preparation = executor.begin_preparation();
    expect_error([&] { auto invalid = executor.finish_preparation(std::move(preparation), first_token); });
    require(bool(preparation), "stale CPU-finish receipt consumed preparation");
    memory = executor.finish_preparation(std::move(preparation), second_token);
    require(!preparation && bool(memory), "current actual receipt failed after rejecting a stale receipt");
    memory.release();
    require(executor.quiescent(), "CPU-finish/reuse sequence retained an exclusive lease");
    std::cout << "cpu_finish actual_tasks=2 consecutive_tokens=1 external_polls=0 cpu_finishes=2 "
                 "zero_wrong_stale_receipts_preserve_preparation=1 actual_result_packets=unchanged "
                 "rdram_hidden_identity=stable hidden_history=preserved cpu_writeback=preserved\n";
}

void second_task_failure(bool fail_in_rsp) {
    // Authored canonical mechanism coverage, not a naturally reached Source
    // producer: retain one executor across a real success, CPU finish and failure.
    Executor executor(true, {10000, 64});
    const Program first(simple_fill.size());
    auto preparation = prepare(executor, simple_fill, true);
    const auto first_token = executor.begin(std::move(preparation), make_task(first, simple_fill.size()), first.bytes);
    auto memory = executor.wait_completed(first_token);
    (void)completed_draw(executor, memory);
    memory.release();
    require(executor.quiescent(), "first actual task did not drain before sequential failure case");
    preparation = executor.begin_preparation();
    {
        auto words = preparation.words();
        words[texture_address / 4] = 0x6a5b4c3d;
    }
    memory = executor.finish_preparation(std::move(preparation), first_token);
    require(!preparation && bool(memory), "successful prefix CPU finish did not transfer preparation");
    (void)completed_draw(executor, memory);
    memory.release();
    require(executor.quiescent(), "successful CPU-finished prefix retained a mapping before task two");

    const auto commands = fail_in_rsp ? std::span<const std::uint32_t>(simple_fill) :
                                        std::span<const std::uint32_t>(invalid_load);
    Program second(commands.size());
    if (fail_in_rsp) {
        // The actual RSP emits the valid fill packets, then reaches the same
        // bounded backward-edge failure exercised by rsp_tests.cpp. Partial
        // packet output must never be submitted to the GPU or returned as ready.
        second.bytes.resize(second.bytes.size() - sizeof(std::uint32_t));
        second.emit(0x1000ffffu); // Local BEQ r0,r0,-1.
        second.emit(0);           // Delay-slot NOP.
    }
    preparation = prepare(executor, commands, false);
    Executor::Token second_token = 0;
    std::string observed;
    if (fail_in_rsp) {
        observed = expect_error([&] {
            second_token = executor.begin(std::move(preparation), make_task(second, commands.size()), second.bytes);
        });
        require(second_token == 0 && observed.find("error=execution_budget") != std::string::npos,
                "second actual RSP task did not fail at its authored bounded loop before GPU admission");
    } else {
        second_token = executor.begin(std::move(preparation), make_task(second, commands.size()), second.bytes);
        require(second_token == first_token + 1, "CPU-finished prefix fabricated a task before failing GPU work");
        observed = expect_error([&] { memory = executor.wait_completed(second_token); });
        require(observed.find("canonical RSP/RDP backend error:") != std::string::npos,
                "second actual task did not surface its authored backend validation failure");
        expect_error([&] { memory = executor.wait_completed(second_token); });
    }
    require(!memory && !preparation, "failed second task exposed memory or restored consumed preparation");
    require_poisoned_quiescent(executor);
    // A prior successful receipt cannot bypass the failed second task's poison.
    // Its preparation was consumed, and a fresh one was explicitly refused.
    expect_error([&] { memory = executor.finish_preparation(std::move(preparation), first_token); });
    expect_error([&] { memory = executor.wait_completed(first_token); });
    require(!memory && executor.quiescent(),
            "prior task receipt resurrected completed mappings after the second task failed");
    std::cout << "second_task_" << (fail_in_rsp ? "rsp" : "gpu")
              << "_failure successful_prefix_tasks=1 successful_prefix_cpu_finishes=1 external_polls=0 "
                 "prior_receipt_rejected=1 escaped_memory_leases=0 poisoned_quiescent=1 diagnostic=\""
              << observed << "\"\n";
}
} // namespace

int main(int argc, char**) {
    if (argc != 1) {
        std::cerr << "usage: canonical-gpu-failure-smoke (no ROM, SDK, output root, or device argument)\n";
        return EXIT_FAILURE;
    }
    Watchdog watchdog;
    try {
        asynchronous_failure();
        blocking_retire_failure();
        blocking_wait_failure();
        blocking_execute_failure();
        destructor_failure();
        cold_warm_success();
        cpu_finish_success();
        second_task_failure(false);
        second_task_failure(true);
        std::cout << "canonical_gpu_failure_smoke=passed observed_backend_exceptions=5 observed_rsp_execution_exceptions=1 "
                     "failure_input_packets_per_submission=4 failure_input_words_per_submission=8 "
                     "vulkan_requirement=CPU device_loss_not_tested=1 prolonged_drain_not_tested=1\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "canonical_gpu_failure_smoke: FAIL: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
