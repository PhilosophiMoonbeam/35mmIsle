// ROM/SDK-free authored RSP -> real canonical RDP consumer. CPU Vulkan only.
// This tests the pinned renderer's LoadTile4bpp validation failure, NOT device
// loss, a prolonged GPU drain, or a hardware renderer. No output files are used.
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
Draw successful_draw(Executor& executor, bool cold) {
    const Program program(simple_fill.size());
    auto preparation = prepare(executor, simple_fill, cold);
    auto memory = executor.execute(std::move(preparation), make_task(program, simple_fill.size()), program.bytes);
    require(bool(memory), "successful fill did not return actual completed mappings");
    require(!executor.quiescent(), "active successful result lease was reported quiescent");
    Draw draw;
    {
        const auto& result = memory.rsp_result();
        require(bool(result), "successful fill has an RSP error");
        draw.packets = result.packets;
        draw.packet_words = result.packet_words;
        draw.full_syncs = result.full_syncs;
        require(draw.packets == simple_fill.size() / 2 && draw.packet_words == simple_fill.size() && draw.full_syncs == 1,
                "successful fill's actual RSP packet counters disagree with the authored stream");
        require(memory.packets().words().size() == simple_fill.size() &&
                std::equal(simple_fill.begin(), simple_fill.end(), memory.packets().words().begin()),
                "successful real RSP packet contents changed");
        const auto ram = memory.rdram();
        require(ram.size() == (8u << 20) && memory.hidden().size() == (4u << 20),
                "successful fill did not expose full actual memory mappings");
        for (std::size_t index = 0; index < draw.pixels.size(); ++index) {
            const auto address = color_address + index * 2;
            draw.pixels[index] = static_cast<std::uint16_t>(
                (std::uint16_t(guest_byte(ram, address)) << 8) | guest_byte(ram, address + 1));
            require(draw.pixels[index] == fill_pixel, "actual fill pixels do not match RGBA5551 red");
        }
        for (std::size_t byte = 0; byte < 16; ++byte) {
            const auto expected = static_cast<std::uint8_t>(guard_word >> ((3u - (byte & 3u)) * 8));
            require(guest_byte(ram, color_address - 16 + byte) == expected &&
                    guest_byte(ram, color_address + color_bytes + byte) == expected,
                    "actual fill corrupted a neighboring RDRAM guard");
        }
    } // Every mapping/reference borrower ends before releasing the owner lease.
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
        blocking_execute_failure();
        destructor_failure();
        cold_warm_success();
        std::cout << "canonical_gpu_failure_smoke=passed observed_backend_exceptions=3 "
                     "failure_input_packets_per_submission=4 failure_input_words_per_submission=8 "
                     "vulkan_requirement=CPU device_loss_not_tested=1 prolonged_drain_not_tested=1\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "canonical_gpu_failure_smoke: FAIL: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
