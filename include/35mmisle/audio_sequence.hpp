#ifndef ISLE_AUDIO_SEQUENCE_HPP
#define ISLE_AUDIO_SEQUENCE_HPP

#include "35mmisle/audio_bank.hpp"

namespace isle::audio {
struct SequenceEntry {
    std::uint32_t offset;
    std::span<const std::byte> encoded;
    std::span<const std::byte> dma_bytes;
};

enum class SequenceError {
    none, invalid_owner, truncated, revision, invalid_offset, allocation_failure
};
struct SequenceResult;
class SequenceFile final {
public:
    SequenceFile() noexcept = default;
    std::span<const SequenceEntry> sequences() const noexcept;
    explicit operator bool() const noexcept { return storage_ != nullptr; }
private:
    struct Storage;
    std::shared_ptr<const Storage> storage_;
    friend SequenceResult hydrate_sequences(ByteOwner) noexcept;
};
struct SequenceResult {
    SequenceError error = SequenceError::none;
    std::uint32_t offset = 0;
    SequenceFile asset;
    explicit operator bool() const noexcept { return error == SequenceError::none && bool(asset); }
};
// Original BE S1 catalog: ASCII S1, unsigned BE16 count, then count pairs of
// BE32 file-relative offset and BE32 declared length. Payloads begin after the
// complete table; bounded aliases, overlaps, gaps and empty entries are retained.
// encoded preserves the declared length; dma_bytes includes an extra owner byte
// for odd lengths. Both spans are zero-copy; no alignment or CSeq rules apply.
// A zero-entry catalog is valid. Every failure publishes an empty asset. Error
// offsets are zero for header/allocation failures, four for a truncated table,
// and the offending payload offset for invalid_offset.
// The asset retains the immutable input and descriptors. Bare spans require a
// live SequenceFile copy through consumer completion. Input bytes must not be
// mutated via another alias. No DSP/player state or wire relocation is made.
SequenceResult hydrate_sequences(ByteOwner bytes) noexcept;
} // namespace isle::audio
#endif
