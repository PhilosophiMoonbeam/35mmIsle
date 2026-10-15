#include "35mmisle/rsp.hpp"
#include "rsp_jit.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>

namespace isle::rsp {
namespace {
constexpr std::uint32_t ram_bytes = 8u * 1024u * 1024u;
constexpr std::uint32_t dpc_xbus = 1u;
constexpr std::uint32_t dpc_freeze = 2u;
constexpr std::uint32_t dpc_flush = 4u;
constexpr std::uint32_t dpc_ready = 0x80u;
constexpr std::uint32_t dpc_start_pending = 0x400u;

bool range(std::uint32_t address, std::uint32_t size) noexcept {
    return address <= ram_bytes && size <= ram_bytes - address;
}

// Command lengths in 32-bit words: eight triangle combinations, two textured
// rectangles, and single-doubleword state/synchronization commands.
std::size_t packet_length(std::uint32_t first) noexcept {
    const auto opcode = (first >> 24) & 63u;
    constexpr std::array<std::size_t, 8> triangles{8, 12, 24, 28, 24, 28, 40, 44};
    if (opcode >= 8 && opcode <= 15) {
        return triangles[opcode - 8];
    }
    if (opcode == 0x24 || opcode == 0x25) {
        return 4;
    }
    if (opcode == 0 || (opcode >= 0x26 && opcode <= 0x3f && opcode != 0x31)) {
        return 2;
    }
    return 0;
}

void change_pair(std::uint32_t& target, std::uint32_t value,
                 std::uint32_t clear, std::uint32_t set, std::uint32_t bit) noexcept {
    const auto pair = value & (clear | set);
    if (pair == clear) {
        target &= ~bit;
    } else if (pair == set) {
        target |= bit;
    }
}
} // namespace

struct Machine::Impl {
    std::span<std::uint32_t> ram;
    PacketSink sink;
    void* context;
    alignas(64) std::array<std::uint32_t, 1024> dmem{};
    alignas(64) std::array<std::uint32_t, 1024> imem{};
    std::array<std::uint32_t, 16> registers{};
    std::uint32_t irq = 0;
    std::uint32_t pending_sp = 0;
    std::uint32_t pending_ram = 0;
    std::uint32_t dp_active_end = 0;
    bool dp_active = false;
    bool dp_pending_end = false;
    std::array<std::uint32_t, 44> packet{};
    std::size_t packet_used = 0;
    std::size_t packet_expected = 0;
    std::size_t copied_words = 0;
    Limits limits{};
    Result result{};
    bool failed = false;
    RSP::JIT::CPU cpu;

    Impl(std::span<std::uint32_t> memory, PacketSink callback, void* opaque)
        : ram(memory), sink(callback), context(opaque) {
        if (ram.size() != ram_bytes / sizeof(std::uint32_t) || sink == nullptr) {
            throw std::invalid_argument("RSP machine requires exactly 8 MiB of word-swapped RDRAM and a packet sink");
        }
        reset();
    }

    void bind() noexcept {
        auto& state = cpu.get_state();
        state.dmem = dmem.data();
        state.imem = imem.data();
        state.rdram = ram.data();
        for (std::size_t index = 0; index != registers.size(); ++index) {
            state.cp0.cr[index] = &registers[index];
        }
        state.cp0.irq = &irq;
    }

    void reset() noexcept {
        dmem.fill(0);
        imem.fill(0);
        registers.fill(0);
        registers[RSP::CP0_REGISTER_SP_STATUS] = SP_STATUS_HALT;
        // These describe only the front-end DMA transport. Rasterization and
        // real DP completion/interrupts belong to the external packet sink.
        registers[RSP::CP0_REGISTER_CMD_STATUS] = dpc_ready;
        irq = 0;
        pending_sp = pending_ram = 0;
        dp_active_end = 0;
        dp_active = dp_pending_end = false;
        packet_used = packet_expected = copied_words = 0;
        failed = false;
        result = {};
        cpu.get_state() = {};
        bind();
        cpu.invalidate_imem();
    }

    void fail(Error error) noexcept {
        if (!failed) {
            result.error = error;
            failed = true;
        }
        registers[RSP::CP0_REGISTER_SP_STATUS] |= SP_STATUS_HALT;
    }

    bool valid_task(const Task& task, std::span<const std::byte> boot) const noexcept {
        // Only loadable, non-yield-resume graphics tasks are this experiment's
        // input. There is no CPU scheduler, yield replay or game emulation.
        if (task.type != 1 || task.flags != 4 || task.boot_size == 0 ||
            task.boot_size != boot.size() || task.boot_size > 4096 ||
            (task.boot_size & 3u) != 0 || task.text_size > 4096 || task.data_size > 4096 ||
            task.output_end < task.output_address || (task.stack_address & 15u) != 0) {
            return false;
        }
        return range(task.boot_address, task.boot_size) && range(task.text_address, task.text_size) &&
               range(task.data_address, task.data_size) && range(task.stack_address, task.stack_size) &&
               range(task.output_address, task.output_end - task.output_address) &&
               range(task.commands_address, task.commands_size) && range(task.yield_address, task.yield_size);
    }

    void dma(bool read, std::uint32_t value) noexcept {
        const auto length = (value & 0xff8u) + 8u;
        const auto count = ((value >> 12) & 255u) + 1u;
        const auto skip = (value >> 20) & 0xff8u;
        const auto bank = pending_sp & 0x1000u;
        auto sp = pending_sp & 0xff8u;
        auto dram = pending_ram & 0xfffff8u;
        // Validate every row before the first bank/RDRAM write. 24-bit SP DMA
        // operands beyond the actual 8-MiB allocation never alias host memory.
        auto address = dram;
        for (std::uint32_t row = 0; row != count; ++row) {
            if (!range(address, length)) {
                fail(Error::address_out_of_range);
                return;
            }
            if (row + 1 != count) {
                address = (address + length + skip) & 0xffffffu;
            }
        }
        auto& memory = bank != 0 ? imem : dmem;
        for (std::uint32_t row = 0; row != count; ++row) {
            for (std::uint32_t offset = 0; offset != length; offset += 4) {
                auto& local = memory[((sp + offset) & 0xfffu) / 4];
                auto& external = ram[(dram + offset) / 4];
                if (read) {
                    local = external;
                } else {
                    external = local;
                }
            }
            sp = (sp + length) & 0xfffu;
            dram = (dram + length + (row + 1 != count ? skip : 0u)) & 0xffffffu;
        }
        // Programmed endpoint latches survive completion. Only current/readback
        // registers advance; a later DMA may reprogram just one endpoint.
        registers[RSP::CP0_REGISTER_DMA_CACHE] = bank | sp;
        registers[RSP::CP0_REGISTER_DMA_DRAM] = dram;
        registers[RSP::CP0_REGISTER_DMA_READ_LENGTH] =
            registers[RSP::CP0_REGISTER_DMA_WRITE_LENGTH] = (value & 0xff800000u) | 0xff8u;
        if (read) {
            ++result.dma_reads;
            if (bank != 0) {
                ++result.imem_writes;
                // The MTC0 already returned through the normal JIT return
                // thunk, so no instruction compiled from old IMEM can run.
                cpu.invalidate_imem();
            }
        } else {
            ++result.dma_writes;
        }
    }

    bool append_word(std::uint32_t value) noexcept {
        if (copied_words == limits.packet_words) {
            fail(Error::packet_capacity);
            return false;
        }
        ++copied_words;
        if (packet_used == 0) {
            packet_expected = packet_length(value);
            if (packet_expected == 0) {
                fail(Error::malformed_packet);
                return false;
            }
        }
        packet[packet_used++] = value;
        if (packet_used != packet_expected) {
            return true;
        }
        // Only ordinary C++ host frames are active here. PacketSink is a
        // noexcept contract: a violating throw terminates in C++, not a JIT
        // unwind. There is deliberately no fake successful completion.
        if (!sink(context, std::span<const std::uint32_t>(packet.data(), packet_used))) {
            fail(Error::sink_rejected);
            return false;
        }
        ++result.packets;
        result.packet_words += packet_used;
        if (((packet[0] >> 24) & 63u) == 0x29) {
            ++result.full_syncs; // Observed command, not downstream completion.
        }
        packet_used = packet_expected = 0;
        return true;
    }

    void drain_dp() noexcept {
        auto& status = registers[RSP::CP0_REGISTER_CMD_STATUS];
        auto& current = registers[RSP::CP0_REGISTER_CMD_CURRENT];
        if ((status & dpc_flush) != 0) {
            // Transport flush cannot cancel published packets or manufacture
            // downstream DP completion.
            current = dp_active_end;
            dp_active = dp_pending_end = false;
            status &= ~(dpc_start_pending | 0x200u);
            packet_used = packet_expected = 0;
            return;
        }
        if ((status & dpc_freeze) != 0) {
            return;
        }
        for (;;) {
            if (!dp_active && dp_pending_end) {
                current = registers[RSP::CP0_REGISTER_CMD_START];
                dp_active_end = registers[RSP::CP0_REGISTER_CMD_END];
                dp_active = true;
                dp_pending_end = false;
                status &= ~(dpc_start_pending | 0x200u);
            }
            if (!dp_active) {
                return;
            }
            if (dp_active_end < current || ((dp_active_end - current) & 7u) != 0) {
                fail(Error::address_out_of_range);
                return;
            }
            const bool xbus = (status & dpc_xbus) != 0;
            if (!xbus && !range(current, dp_active_end - current)) {
                fail(Error::address_out_of_range);
                return;
            }
            while (current != dp_active_end && !failed) {
                const auto value = xbus ? dmem[(current & 0xfffu) / 4] : ram[current / 4];
                if (!append_word(value)) {
                    return;
                }
                // Incomplete packet words are privately owned before any
                // RSP instruction may recycle the source buffer.
                current += 4;
            }
            dp_active = false;
            if (failed) {
                return;
            }
        }
    }

    void sp_status(std::uint32_t value) noexcept {
        auto& status = registers[RSP::CP0_REGISTER_SP_STATUS];
        change_pair(status, value, 1u, 2u, SP_STATUS_HALT);
        if ((value & 4u) != 0) {
            status &= ~std::uint32_t(SP_STATUS_BROKE);
        }
        change_pair(irq, value, 8u, 16u, 1u);
        change_pair(status, value, 32u, 64u, SP_STATUS_SSTEP);
        change_pair(status, value, 128u, 256u, SP_STATUS_INTR_BREAK);
        for (unsigned bit = 0; bit != 8; ++bit) {
            change_pair(status, value, 0x200u << (2 * bit), 0x400u << (2 * bit), 0x80u << bit);
        }
        if ((status & SP_STATUS_SSTEP) != 0) {
            // Single-step hardware/pipeline timing is outside this task-only
            // adapter; fail explicitly instead of silently ignoring the mode.
            fail(Error::unexpected_stop);
        }
    }

    void service() noexcept {
        const auto& state = cpu.get_state();
        const auto value = state.service_value;
        switch (state.service_register) {
        case RSP::CP0_REGISTER_DMA_CACHE:
            pending_sp = value & 0x1ff8u;
            break;
        case RSP::CP0_REGISTER_DMA_DRAM:
            pending_ram = value & 0xfffff8u;
            break;
        case RSP::CP0_REGISTER_DMA_READ_LENGTH:
            dma(true, value);
            break;
        case RSP::CP0_REGISTER_DMA_WRITE_LENGTH:
            dma(false, value);
            break;
        case RSP::CP0_REGISTER_SP_STATUS:
            sp_status(value);
            break;
        case RSP::CP0_REGISTER_SP_SEMAPHORE:
            registers[RSP::CP0_REGISTER_SP_SEMAPHORE] = 0;
            break;
        case RSP::CP0_REGISTER_CMD_START:
            registers[RSP::CP0_REGISTER_CMD_START] = value & 0xfffff8u;
            registers[RSP::CP0_REGISTER_CMD_STATUS] |= dpc_start_pending;
            break;
        case RSP::CP0_REGISTER_CMD_END:
            registers[RSP::CP0_REGISTER_CMD_END] = value & 0xfffff8u;
            if ((registers[RSP::CP0_REGISTER_CMD_STATUS] & dpc_start_pending) != 0) {
                dp_pending_end = true;
                registers[RSP::CP0_REGISTER_CMD_STATUS] |= 0x200u;
            } else {
                dp_active_end = registers[RSP::CP0_REGISTER_CMD_END];
                dp_active = true;
            }
            drain_dp();
            break;
        case RSP::CP0_REGISTER_CMD_STATUS: {
            auto& status = registers[RSP::CP0_REGISTER_CMD_STATUS];
            change_pair(status, value, 1u, 2u, dpc_xbus);
            change_pair(status, value, 4u, 8u, dpc_freeze);
            change_pair(status, value, 16u, 32u, dpc_flush);
            for (unsigned bit = 0; bit != 4; ++bit) {
                if ((value & (0x40u << bit)) != 0) {
                    registers[15u - bit] = 0;
                }
            }
            drain_dp();
            break;
        }
        default:
            // DMA_FULL/BUSY and DPC_CURRENT/activity clocks are read-only.
            break;
        }
    }

    Result execute(const Task& task, std::span<const std::byte> boot, Limits requested) {
        result = {};
        failed = false;
        if (!valid_task(task, boot)) {
            result.error = Error::invalid_task;
            result.sp_status = registers[RSP::CP0_REGISTER_SP_STATUS];
            return result;
        }
        if ((registers[RSP::CP0_REGISTER_SP_STATUS] & SP_STATUS_SSTEP) != 0) {
            result.error = Error::unexpected_stop;
            result.sp_status = registers[RSP::CP0_REGISTER_SP_STATUS];
            return result;
        }
        limits = requested;
        copied_words = packet_used = packet_expected = 0;
        // The load service uses console-order field writes, not native OSTask
        // or an SDK implementation. All operands were validated above.
        const std::array<std::uint32_t, 16> descriptor{
            task.type, task.flags, task.boot_address, task.boot_size,
            task.text_address, task.text_size, task.data_address, task.data_size,
            task.stack_address, task.stack_size, task.output_address, task.output_end,
            task.commands_address, task.commands_size, task.yield_address, task.yield_size};
        std::copy(descriptor.begin(), descriptor.end(), dmem.begin() + 0xfc0 / 4);
        for (std::size_t index = 0; index != boot.size(); index += 4) {
            imem[index / 4] = (std::uint32_t(std::to_integer<unsigned char>(boot[index])) << 24) |
                             (std::uint32_t(std::to_integer<unsigned char>(boot[index + 1])) << 16) |
                             (std::uint32_t(std::to_integer<unsigned char>(boot[index + 2])) << 8) |
                             std::uint32_t(std::to_integer<unsigned char>(boot[index + 3]));
        }
        ++result.imem_writes;
        auto& state = cpu.get_state();
        state.pc = 0;
        state.has_delay_slot = state.branch_target = 0;
        state.block_budget = limits.block_entries;
        state.sr[0] = 0;
        registers[RSP::CP0_REGISTER_SP_STATUS] &=
            ~std::uint32_t(SP_STATUS_HALT | SP_STATUS_BROKE | SP_STATUS_SIG0 | SP_STATUS_SIG1 | SP_STATUS_SIG2);
        irq &= ~1u;
        cpu.invalidate_imem();
        while (!failed) {
            const auto mode = cpu.run();
            if (mode == RSP::MODE_BUDGET) {
                fail(Error::execution_budget);
            } else if (mode == RSP::MODE_HOST_ERROR) {
                fail(Error::unexpected_stop);
            } else if (mode == RSP::MODE_HOST_SERVICE) {
                service();
                if (!failed && (registers[RSP::CP0_REGISTER_SP_STATUS] & SP_STATUS_HALT) != 0) {
                    fail(Error::unexpected_stop);
                }
            } else if (mode == RSP::MODE_BREAK) {
                if (packet_used != 0) {
                    fail(Error::malformed_packet);
                } else if (dp_active || dp_pending_end) {
                    fail(Error::unexpected_stop);
                }
                break;
            } else {
                fail(Error::unexpected_stop);
            }
        }
        result.sp_status = registers[RSP::CP0_REGISTER_SP_STATUS];
        return result;
    }
};

Machine::Machine(std::span<std::uint32_t> rdram, PacketSink sink, void* context)
    : impl_(std::make_unique<Impl>(rdram, sink, context)) {}
Machine::~Machine() = default;
void Machine::reset() { impl_->reset(); }
Result Machine::execute(const Task& task, std::span<const std::byte> boot, Limits limits) {
    return impl_->execute(task, boot, limits);
}
} // namespace isle::rsp
