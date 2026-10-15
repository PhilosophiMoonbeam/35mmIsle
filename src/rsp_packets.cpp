#include "rsp_packets.hpp"

#include <algorithm>

namespace isle::rsp {
PacketBuffer::PacketBuffer(std::size_t word_capacity)
    : words_(std::make_unique_for_overwrite<std::uint32_t[]>(word_capacity)),
      packets_(std::make_unique_for_overwrite<PacketRange[]>(word_capacity / 2)),
      word_capacity_(word_capacity), packet_capacity_(word_capacity / 2) {}

void PacketBuffer::clear() noexcept {
    word_count_ = packet_count_ = 0;
    rejected_ = false;
}

bool PacketBuffer::sink(void* context, std::span<const std::uint32_t> packet) noexcept {
    auto& buffer = *static_cast<PacketBuffer*>(context);
    if (packet.size() < 2 || packet.size() > 44 || (packet.size() & 1) != 0 ||
        packet.size() > buffer.word_capacity_ - buffer.word_count_ ||
        buffer.packet_count_ == buffer.packet_capacity_) {
        buffer.rejected_ = true;
        return false;
    }
    buffer.packets_[buffer.packet_count_++] = {buffer.word_count_, packet.size()};
    std::copy(packet.begin(), packet.end(), buffer.words_.get() + buffer.word_count_);
    buffer.word_count_ += packet.size();
    return true;
}

std::span<const std::uint32_t> PacketBuffer::words() const noexcept {
    return {words_.get(), word_count_};
}

std::span<const PacketRange> PacketBuffer::packets() const noexcept {
    return {packets_.get(), packet_count_};
}
} // namespace isle::rsp
