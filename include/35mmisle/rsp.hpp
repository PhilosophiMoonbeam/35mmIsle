#ifndef ISLE_RSP_HPP
#define ISLE_RSP_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

namespace isle::rsp {
// Console task fields in their specified order, never a native OSTask layout.
// Pointer operands are canonical RDRAM byte addresses. Graphics FIFO output_end
// is an end address, not a byte count. SDK structures are not imported.
struct Task {
    std::uint32_t type, flags;
    std::uint32_t boot_address, boot_size;
    std::uint32_t text_address, text_size;
    std::uint32_t data_address, data_size;
    std::uint32_t stack_address, stack_size;
    std::uint32_t output_address, output_end;
    std::uint32_t commands_address, commands_size;
    std::uint32_t yield_address, yield_size;
};

enum class Error {
    none, invalid_task, address_out_of_range, execution_budget,
    malformed_packet, packet_capacity, sink_rejected, unexpected_stop
};
struct Limits {
    // Budget at emitted JIT basic-block entry, not simulated cycles. Every
    // backward edge must cross a guard, including loops internal to one region.
    std::uint32_t block_entries = 1000000;
    std::size_t packet_words = 1u << 20;
};
struct Result {
    Error error = Error::none;
    std::uint32_t sp_status = 0;
    std::size_t dma_reads = 0, dma_writes = 0, imem_writes = 0;
    std::size_t packets = 0, packet_words = 0, full_syncs = 0;
    explicit operator bool() const noexcept { return error == Error::none; }
};
// The callback must copy packet words before return and must not throw. False
// rejects the task without fabricated completion. Callbacks are serial on the
// caller thread. Real downstream DP completion remains the sink owner's duty.
using PacketSink = bool (*)(void*, std::span<const std::uint32_t>) noexcept;

class Machine final {
public:
    // Borrowed 8-MiB word-swapped RDRAM. It and sink context outlive the machine;
    // no concurrent CPU/GPU access. No native pointers are encoded into it.
    Machine(std::span<std::uint32_t> rdram, PacketSink sink, void* context);
    ~Machine();
    Machine(const Machine&) = delete;
    Machine& operator=(const Machine&) = delete;
    // Explicit cold coprocessor reset; does not reset caller RDRAM or DP state.
    void reset();
    // Boot bytes are canonical byte order, copied into IMEM. Exact caller-owned
    // ROM snapshot and resource identities are checked by the private consumer.
    // No C++ exception may cross generated JIT frames. Failure invalidates task
    // output; caller must not publish it. No R4300/game CPU instructions execute.
    // Admits graphics (type 1, flags 4) and audio (type 2, flags 0, unused
    // stack/output/yield fields zero). DMA is synchronous transport, not cycle
    // emulation or yield/resume. FullSync counts commands, not GPU completion.
    Result execute(const Task& task, std::span<const std::byte> boot, Limits limits = {});
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace isle::rsp
#endif
