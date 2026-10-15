#ifndef ISLE_AUDIO_CSEQ_HPP
#define ISLE_AUDIO_CSEQ_HPP

#include "35mmisle/audio_sequence.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace isle::audio {
enum class SequenceEventKind { midi, tempo, loop_start, loop_end, track_end, sequence_end };
struct SequenceEvent {
    SequenceEventKind kind = SequenceEventKind::midi;
    std::uint8_t track = 0, status = 0, data1 = 0, data2 = 0;
    std::uint64_t delta_ticks = 0, tick = 0;
    std::uint32_t duration_ticks = 0, tempo_us_per_quarter = 0;
};
enum class SequenceReadError {
    none, uninitialized, truncated, invalid_header, invalid_offset, invalid_vlq,
    invalid_status, invalid_meta, invalid_backref, invalid_loop, tick_overflow,
    loop_capacity
};
struct SequenceReadResult {
    SequenceReadError error = SequenceReadError::none;
    std::uint32_t offset = 0;
    std::uint8_t track = 0;
    bool success = true;
    explicit operator bool() const noexcept { return success; }
};
enum class SequenceStep { event, end, error };

// A borrowed, value-copyable decoder/checkpoint. The SequenceFile (or other
// immutable encoded-byte owner) must outlive this reader AND every reader copy.
// No DMA padding is consumed. Copies share only immutable bytes, never counters.
// Each next() publishes one encoded event, including loop controls: consumers
// must separately bound work when draining zero-time or nonterminating loops.
class SequenceReader final {
public:
    static constexpr std::size_t track_count = 16;
    static constexpr std::size_t counter_capacity = 2048;
    SequenceReadResult start(const SequenceEntry&) noexcept;
    SequenceStep next(SequenceEvent&, SequenceReadResult&) noexcept;
    std::uint64_t ticks() const noexcept { return tick_; }
    std::uint32_t division() const noexcept { return division_; }
    bool ended() const noexcept { return ended_; }

private:
    struct Track {
        std::uint64_t due = 0;
        std::uint32_t cursor = 0, backup = 0;
        std::uint16_t remaining = 0;
        std::uint8_t status = 0;
        bool active = false;
    };
    struct Byte {
        std::uint32_t offset = 0;
        std::uint8_t value = 0;
    };
    static constexpr std::size_t counter_buckets = 4096;
    std::span<const std::byte> bytes_;
    std::array<Track, track_count> tracks_{};
    // Zero is an empty bucket; counters can only occupy payload offsets >=68.
    std::array<std::uint32_t, counter_buckets> counter_keys_{};
    std::array<std::uint8_t, counter_buckets> counter_values_{};
    std::uint64_t tick_ = 0;
    std::uint32_t division_ = 0;
    std::uint16_t counter_count_ = 0;
    std::uint8_t deferred_ = track_count;
    bool initialized_ = false, ended_ = false;

    std::size_t counter_bucket(std::uint32_t) const noexcept;
    std::uint8_t physical(std::uint32_t) const noexcept;
    bool byte(Track&, Byte&, SequenceReadResult&) const noexcept;
    bool vlq(Track&, std::uint32_t&, SequenceReadResult&) const noexcept;
    bool prime(Track&, SequenceReadResult&) const noexcept;
    bool decode(Track&, SequenceEvent&, SequenceReadResult&,
                std::uint32_t& counter, std::uint8_t& value) const noexcept;
};
// Native admission/error policy (not an assertion about undefined SDK inputs):
// require a complete 68-byte header, nonzero division, 32-bit encoded extent and
// present track starts outside the header. Deltas/durations are at most five
// VLQ bytes and must fit u32; cumulative ticks must fit u64. Pattern sources and
// loop jumps must remain in the payload. At most 2048 distinct physical loop
// counters are admitted (16 tracks * the documented 128-loop bound).
// Original-executed pattern lengths 1..255 copy that many bytes; encoded zero
// copies exactly 256 bytes. The complete source range must be in encoded.
// Distance FFFF is supported as observed; FE in the high byte means a literal
// FE escape instead, regardless of the following physical byte.
// Original-executed loop semantics override conflicting manual prose: CURRENT
// zero resets to INITIAL and falls through; 255 repeats unchanged; otherwise
// decrement and jump. After decoded FF/2D, the six payload bytes are raw PRIMARY
// bytes (not pattern-expanded). The jump subtracts from their primary end and
// preserves any still-active raw backup. All physical reads see local counters.
// start() failure preserves the old binding. next() failure preserves all reader
// state and the output event; diagnostics identify a physical sequence offset.
// A complete event is published before its following delta is read. The last
// track emits sequence_end instead of track_end; subsequent calls return end,
// leave the event unchanged and report success. An empty track set emits one
// sequence_end at tick zero. Unused normalized event fields are zero.
} // namespace isle::audio
#endif
