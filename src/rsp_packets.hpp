#ifndef ISLE_RSP_PACKETS_HPP
#define ISLE_RSP_PACKETS_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

namespace isle::rsp {
struct PacketRange {
    std::size_t offset, count;
    bool operator==(const PacketRange&) const = default;
};

// Allocate the complete bound before execution. The serial noexcept RSP sink
// copies every packet before return, without growth or allocation in JIT frames.
class PacketBuffer {
public:
    explicit PacketBuffer(std::size_t word_capacity);
    void clear() noexcept;
    static bool sink(void* context, std::span<const std::uint32_t> packet) noexcept;
    std::span<const std::uint32_t> words() const noexcept;
    std::span<const PacketRange> packets() const noexcept;
    bool rejected() const noexcept { return rejected_; }
private:
    std::unique_ptr<std::uint32_t[]> words_;
    std::unique_ptr<PacketRange[]> packets_;
    std::size_t word_capacity_, packet_capacity_;
    std::size_t word_count_ = 0, packet_count_ = 0;
    bool rejected_ = false;
};
} // namespace isle::rsp
#endif
