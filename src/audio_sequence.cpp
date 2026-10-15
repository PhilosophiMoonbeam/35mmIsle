// Original native decoder of S1 wire-layout facts. No SDK implementation.
#include "35mmisle/audio_sequence.hpp"

#include <new>
#include <stdexcept>
#include <utility>

namespace isle::audio {
namespace {
std::uint16_t u16(const std::byte* p) noexcept {
    return static_cast<std::uint16_t>((std::to_integer<unsigned>(p[0]) << 8) |
                                      std::to_integer<unsigned>(p[1]));
}
std::uint32_t u32(const std::byte* p) noexcept {
    return (std::uint32_t(u16(p)) << 16) | u16(p + 2);
}
} // namespace

struct SequenceFile::Storage {
    ByteOwner bytes;
    std::vector<SequenceEntry> sequences;
};

std::span<const SequenceEntry> SequenceFile::sequences() const noexcept {
    return storage_ ? std::span<const SequenceEntry>(storage_->sequences) :
                      std::span<const SequenceEntry>{};
}
SequenceResult hydrate_sequences(ByteOwner bytes) noexcept {
    if (!bytes) return {SequenceError::invalid_owner, 0, {}};
    if (bytes->size() < 4) return {SequenceError::truncated, 0, {}};
    const auto* header = bytes->data();
    if (u16(header) != 0x5331) return {SequenceError::revision, 0, {}};
    const auto count = static_cast<std::size_t>(u16(header + 2));
    if (count > (bytes->size() - 4) / 8)
        return {SequenceError::truncated, 4, {}};
    const auto table_end = 4 + count * 8;
    try {
        auto storage = std::make_shared<SequenceFile::Storage>();
        storage->bytes = std::move(bytes);
        storage->sequences.reserve(count);
        const auto input = std::span<const std::byte>(*storage->bytes);
        for (std::size_t i = 0; i < count; ++i) {
            const auto* row = header + 4 + i * 8;
            const auto offset = u32(row);
            const auto length = static_cast<std::size_t>(u32(row + 4));
            if (offset < table_end || offset > input.size())
                return {SequenceError::invalid_offset, offset, {}};
            const auto available = input.size() - offset;
            const auto padding = length & std::size_t{1};
            if (length > available || padding > available - length)
                return {SequenceError::invalid_offset, offset, {}};
            storage->sequences.push_back({offset, input.subspan(offset, length),
                                         input.subspan(offset, length + padding)});
        }
        SequenceFile asset;
        asset.storage_ = std::move(storage);
        return {SequenceError::none, 0, std::move(asset)};
    } catch (const std::bad_alloc&) {
        return {SequenceError::allocation_failure, 0, {}};
    } catch (const std::length_error&) {
        return {SequenceError::allocation_failure, 0, {}};
    }
}
} // namespace isle::audio
