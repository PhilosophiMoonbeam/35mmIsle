#include "35mmisle/audio_cseq.hpp"

#include <limits>

namespace isle::audio {
namespace {
constexpr std::uint32_t header_bytes = 68;
bool fail(SequenceReadResult& result, SequenceReadError error, std::uint32_t offset) noexcept {
    result.error = error; result.offset = offset; result.success = false;
    return false;
}
std::uint32_t be32(std::span<const std::byte> bytes, std::size_t at) noexcept {
    std::uint32_t value = 0;
    for (unsigned i = 0; i != 4; ++i) value = (value << 8) | std::to_integer<std::uint8_t>(bytes[at + i]);
    return value;
}
} // namespace

std::size_t SequenceReader::counter_bucket(std::uint32_t offset) const noexcept {
    auto bucket = static_cast<std::size_t>((offset * 2654435761u) & (counter_buckets - 1));
    // Load never exceeds one half, so every search has an empty terminator.
    while (counter_keys_[bucket] != 0 && counter_keys_[bucket] != offset)
        bucket = (bucket + 1) & (counter_buckets - 1);
    return bucket;
}
std::uint8_t SequenceReader::physical(std::uint32_t offset) const noexcept {
    if (counter_count_ != 0) {
        const auto bucket = counter_bucket(offset);
        if (counter_keys_[bucket] != 0) return counter_values_[bucket];
    }
    return std::to_integer<std::uint8_t>(bytes_[offset]);
}
bool SequenceReader::byte(Track& track, Byte& out, SequenceReadResult& result) const noexcept {
    for (;;) {
        if (track.remaining != 0) {
            out.offset = track.backup++;
            out.value = physical(out.offset);
            --track.remaining;
            return true; // Active pattern bytes are raw, including FE.
        }
        const auto at = track.cursor;
        if (at >= bytes_.size()) return fail(result, SequenceReadError::truncated, at);
        out = {at, physical(at)};
        ++track.cursor;
        if (out.value != 0xfe) return true;
        if (track.cursor >= bytes_.size()) return fail(result, SequenceReadError::truncated, track.cursor);
        const auto high = physical(track.cursor++);
        if (high == 0xfe) return true;
        if (bytes_.size() - track.cursor < 2) return fail(result, SequenceReadError::truncated, track.cursor);
        const auto distance = (std::uint32_t(high) << 8) | physical(track.cursor++);
        const auto encoded_length = physical(track.cursor++);
        const auto length = static_cast<std::uint16_t>(encoded_length == 0 ? 256 : encoded_length);
        if (distance == 0 || distance > at || at - distance < header_bytes)
            return fail(result, SequenceReadError::invalid_backref, at);
        const auto source = at - distance;
        if (length > bytes_.size() - source)
            return fail(result, SequenceReadError::invalid_backref, at);
        track.backup = source;
        track.remaining = length;
        // Original execution proves zero encodes 256 raw copied bytes, not an
        // empty marker. The widened remaining count keeps that extent explicit.
    }
}
bool SequenceReader::vlq(Track& track, std::uint32_t& value, SequenceReadResult& result) const noexcept {
    value = 0;
    for (unsigned count = 0; count != 5; ++count) {
        Byte next;
        if (!byte(track, next, result)) return false;
        const auto bits = std::uint32_t(next.value & 0x7f);
        if (value > (std::numeric_limits<std::uint32_t>::max() - bits) / 128)
            return fail(result, SequenceReadError::invalid_vlq, next.offset);
        value = value * 128 + bits;
        if ((next.value & 0x80) == 0) return true;
        if (count == 4) return fail(result, SequenceReadError::invalid_vlq, next.offset);
    }
    return false;
}
bool SequenceReader::prime(Track& track, SequenceReadResult& result) const noexcept {
    std::uint32_t delta = 0;
    if (!vlq(track, delta, result)) return false;
    if (track.due > std::numeric_limits<std::uint64_t>::max() - delta)
        return fail(result, SequenceReadError::tick_overflow, track.cursor);
    track.due += delta;
    return true;
}
SequenceReadResult SequenceReader::start(const SequenceEntry& entry) noexcept {
    SequenceReadResult result;
    if (entry.encoded.size() < header_bytes) {
        fail(result, SequenceReadError::truncated, 0); return result;
    }
    if (entry.encoded.size() > std::numeric_limits<std::uint32_t>::max()) {
        fail(result, SequenceReadError::invalid_header, 0); return result;
    }
    const auto division = be32(entry.encoded, 64);
    if (division == 0) {
        fail(result, SequenceReadError::invalid_header, 64); return result;
    }
    // Initialization may construct a complete new value. Per-event stepping
    // below stages only two tracks and one sparse counter write.
    SequenceReader staged;
    staged.bytes_ = entry.encoded;
    staged.division_ = division;
    for (std::uint8_t slot = 0; slot != track_count; ++slot) {
        result.track = slot;
        auto& track = staged.tracks_[slot];
        track.cursor = be32(entry.encoded, std::size_t(slot) * 4);
        if (track.cursor == 0) continue;
        if (track.cursor < header_bytes || track.cursor >= entry.encoded.size()) {
            fail(result, SequenceReadError::invalid_offset, track.cursor); return result;
        }
        track.active = true;
        if (!staged.prime(track, result)) return result;
    }
    staged.initialized_ = true;
    *this = staged;
    return {};
}
bool SequenceReader::decode(Track& track, SequenceEvent& event, SequenceReadResult& result,
                            std::uint32_t& counter, std::uint8_t& value) const noexcept {
    Byte first;
    if (!byte(track, first, result)) return false;
    auto status = first.value;
    bool running = status < 0x80;
    if (running) {
        status = track.status;
        if (status == 0) return fail(result, SequenceReadError::invalid_status, first.offset);
    }
    if (status == 0xff) {
        track.status = 0;
        Byte subtype;
        if (!byte(track, subtype, result)) return false;
        switch (subtype.value) {
        case 0x51:
            event.kind = SequenceEventKind::tempo;
            for (unsigned i = 0; i != 3; ++i) {
                Byte part;
                if (!byte(track, part, result)) return false;
                event.tempo_us_per_quarter = (event.tempo_us_per_quarter << 8) | part.value;
            }
            return true;
        case 0x2f:
            event.kind = SequenceEventKind::track_end;
            track.active = false;
            return true;
        case 0x2e: {
            Byte id, end;
            if (!byte(track, id, result) || !byte(track, end, result)) return false;
            if (id.value > 127 || end.value != 0xff)
                return fail(result, SequenceReadError::invalid_loop, id.value > 127 ? id.offset : end.offset);
            event.kind = SequenceEventKind::loop_start;
            return true;
        }
        case 0x2d: {
            // Original public execution distinguishes this from ordinary byte
            // decoding: only FF/2D pass through the pattern reader. The six
            // payload bytes are raw PRIMARY bytes, even with a backup active.
            const auto payload = track.cursor;
            if (bytes_.size() - payload < 6)
                return fail(result, SequenceReadError::truncated, static_cast<std::uint32_t>(bytes_.size()));
            const auto initial = physical(payload);
            const auto current = physical(payload + 1);
            std::uint32_t distance = 0;
            for (unsigned i = 0; i != 4; ++i)
                distance = (distance << 8) | physical(payload + 2 + i);
            track.cursor += 6;
            const auto anchor = track.cursor;
            if (distance == 0 || distance > anchor || anchor - distance < header_bytes ||
                anchor - distance >= bytes_.size())
                return fail(result, SequenceReadError::invalid_loop, anchor);
            event.kind = SequenceEventKind::loop_end;
            counter = payload + 1;
            value = current == 0 ? initial :
                static_cast<std::uint8_t>(current == 255 ? 255 : current - 1);
            if (current != 0) track.cursor = anchor - distance;
            // A jump changes only PRIMARY position. Unconsumed backup bytes
            // continue first, then the decoder resumes at the jump target.
            return true;
        }
        default: return fail(result, SequenceReadError::invalid_meta, subtype.offset);
        }
    }
    if (status < 0x80 || status >= 0xf0)
        return fail(result, SequenceReadError::invalid_status, first.offset);
    event.kind = SequenceEventKind::midi;
    event.status = status;
    track.status = status;
    Byte data = first;
    if (!running && !byte(track, data, result)) return false;
    if (data.value >= 0x80) return fail(result, SequenceReadError::invalid_status, data.offset);
    event.data1 = data.value;
    const auto family = status & 0xf0;
    if (family != 0xc0 && family != 0xd0) {
        if (!byte(track, data, result)) return false;
        if (data.value >= 0x80) return fail(result, SequenceReadError::invalid_status, data.offset);
        event.data2 = data.value;
    }
    if (family == 0x90 && !vlq(track, event.duration_ticks, result)) return false;
    return true;
}
SequenceStep SequenceReader::next(SequenceEvent& output, SequenceReadResult& result) noexcept {
    result = {};
    if (!initialized_) {
        fail(result, SequenceReadError::uninitialized, 0); return SequenceStep::error;
    }
    if (ended_) return SequenceStep::end;
    Track primed;
    const bool pending = deferred_ < track_count;
    if (pending) {
        result.track = deferred_;
        primed = tracks_[deferred_];
        if (!prime(primed, result)) return SequenceStep::error;
    }
    std::uint8_t selected = track_count;
    for (std::uint8_t slot = 0; slot != track_count; ++slot) {
        const auto& track = pending && slot == deferred_ ? primed : tracks_[slot];
        if (!track.active) continue;
        const auto best_due = selected == track_count ? std::numeric_limits<std::uint64_t>::max() :
            (pending && selected == deferred_ ? primed.due : tracks_[selected].due);
        if (selected == track_count || track.due < best_due) selected = slot;
    }
    SequenceEvent event;
    if (selected == track_count) {
        event.kind = SequenceEventKind::sequence_end;
        event.tick = tick_;
        ended_ = true;
        output = event;
        return SequenceStep::event;
    }
    result.track = selected;
    auto staged = pending && selected == deferred_ ? primed : tracks_[selected];
    event.track = selected;
    event.tick = staged.due;
    event.delta_ticks = staged.due - tick_;
    std::uint32_t counter = 0;
    std::uint8_t value = 0;
    if (!decode(staged, event, result, counter, value)) return SequenceStep::error;
    std::size_t bucket = 0;
    if (counter != 0) {
        bucket = counter_bucket(counter);
        if (counter_keys_[bucket] == 0 && counter_count_ == counter_capacity) {
            fail(result, SequenceReadError::loop_capacity, counter); return SequenceStep::error;
        }
    }
    bool any_active = staged.active;
    for (std::uint8_t slot = 0; slot != track_count; ++slot)
        if (slot != selected && tracks_[slot].active) any_active = true;
    if (!any_active) event.kind = SequenceEventKind::sequence_end;
    // All rejecting operations are above this point. Commit the small journal.
    if (pending && selected != deferred_) tracks_[deferred_] = primed;
    tracks_[selected] = staged;
    if (counter != 0) {
        if (counter_keys_[bucket] == 0) { counter_keys_[bucket] = counter; ++counter_count_; }
        counter_values_[bucket] = value;
    }
    deferred_ = staged.active ? selected : static_cast<std::uint8_t>(track_count);
    tick_ = event.tick;
    ended_ = !any_active;
    output = event;
    result = {};
    return SequenceStep::event;
}
} // namespace isle::audio
