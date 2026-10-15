#include "35mmisle/audio_cseq.hpp"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace a = isle::audio;
namespace {
int failures = 0;
void check(bool condition, std::string_view meaning) {
    if (!condition) { ++failures; std::cerr << "FAIL: " << meaning << '\n'; }
}
using Wire = std::vector<std::byte>;
void put32(Wire& bytes, std::size_t offset, std::uint32_t value) {
    for (unsigned i = 0; i != 4; ++i) bytes[offset + i] = std::byte((value >> (24 - i * 8)) & 255);
}
void add(Wire& bytes, std::initializer_list<unsigned> values) {
    for (const auto value : values) bytes.push_back(std::byte(value));
}
void vlq(Wire& bytes, std::uint32_t value) {
    std::uint8_t digits[5]{};
    unsigned count = 0;
    do { digits[count++] = static_cast<std::uint8_t>(value & 127); value >>= 7; } while (value);
    while (count) { --count; bytes.push_back(std::byte(digits[count] | (count ? 128 : 0))); }
}
struct Fixture {
    Wire bytes = Wire(68);
    Fixture() { put32(bytes, 64, 480); }
    std::size_t track(unsigned slot, std::initializer_list<unsigned> values) {
        const auto at = bytes.size();
        put32(bytes, slot * 4, static_cast<std::uint32_t>(at));
        add(bytes, values); return at;
    }
    a::SequenceEntry entry() const { return {0, bytes, bytes}; }
};
bool equal(const a::SequenceEvent& left, const a::SequenceEvent& right) {
    return std::tie(left.kind, left.track, left.status, left.data1, left.data2, left.delta_ticks,
                    left.tick, left.duration_ticks, left.tempo_us_per_quarter) ==
           std::tie(right.kind, right.track, right.status, right.data1, right.data2, right.delta_ticks,
                    right.tick, right.duration_ticks, right.tempo_us_per_quarter);
}
a::SequenceEvent step(a::SequenceReader& reader) {
    a::SequenceEvent event;
    a::SequenceReadResult result;
    check(reader.next(event, result) == a::SequenceStep::event && bool(result), "complete next event succeeds");
    return event;
}
std::vector<a::SequenceEvent> calendar(a::SequenceReader& reader, std::size_t limit = 10000) {
    std::vector<a::SequenceEvent> result;
    for (std::size_t i = 0; i != limit && !reader.ended(); ++i) {
        a::SequenceEvent event;
        a::SequenceReadResult status;
        if (reader.next(event, status) != a::SequenceStep::event) {
            check(false, "calendar has no decoding errors"); break;
        }
        result.push_back(event);
    }
    return result;
}
void same_calendar(a::SequenceReader left, a::SequenceReader right, std::size_t limit = 10000) {
    const auto l = calendar(left, limit), r = calendar(right, limit);
    check(l.size() == r.size(), "checkpoint calendars have equal lengths");
    for (std::size_t i = 0; i < l.size() && i < r.size(); ++i)
        check(equal(l[i], r[i]), "checkpoint calendars have equal semantic events");
}
void rejected_next(a::SequenceReader& reader, a::SequenceReadError error) {
    a::SequenceEvent output;
    output.kind = a::SequenceEventKind::tempo; output.tick = 777; output.data1 = 33;
    const auto before = output;
    const auto tick = reader.ticks();
    const auto ended = reader.ended();
    a::SequenceReadResult first, second;
    check(reader.next(output, first) == a::SequenceStep::error && !first && first.error == error,
          "malformed event reports its error category");
    check(equal(output, before) && tick == reader.ticks() && ended == reader.ended(),
          "failure publishes neither partial event nor clock/terminal changes");
    check(reader.next(output, second) == a::SequenceStep::error && !second &&
          first.error == second.error && first.offset == second.offset && first.track == second.track &&
          equal(output, before), "error repeats with identical diagnostic and unchanged event");
}
void midi_timing_and_normalization() {
    Fixture f;
    f.track(7, {0,0x92,60,80,0x81,0x70, 0x10,61,0,0, 0,0x82,60,32,
                0,0xa2,61,3, 0,0xb2,7,100, 0,0xc2,9, 0,10,
                0,0xd2,11, 0,0xe2,1,64, 0,0xff,0x51,7,0xa1,0x20, 0,0xff,0x2f});
    const auto original = f.bytes;
    a::SequenceReader reader;
    check(bool(reader.start(f.entry())) && reader.division() == 480 && reader.ticks() == 0,
          "header initializes division without consuming event time");
    const auto events = calendar(reader);
    check(events.size() == 11 && reader.ended() && reader.ticks() == 16, "all MIDI families and terminal emitted");
    if (events.size() != 11) return;
    check(events[0].track == 7 && events[0].status == 0x92 && events[0].duration_ticks == 240 &&
          events[0].data1 == 60 && events[0].data2 == 80 && events[0].tick == 0,
          "note duration and encoded channel are independent of track slot");
    check(events[1].status == 0x92 && events[1].data2 == 0 && events[1].duration_ticks == 0 &&
          events[1].tick == 16 && events[1].delta_ticks == 16, "running velocity-zero note stays note-on");
    check(events[2].status == 0x82 && events[2].data2 == 32 && events[2].duration_ticks == 0,
          "explicit note-off preserved without synthesized duration");
    for (std::size_t i = 3; i <= 8; ++i)
        check(events[i].kind == a::SequenceEventKind::midi && events[i].duration_ticks == 0 &&
              events[i].tempo_us_per_quarter == 0 && events[i].delta_ticks == 0,
              "non-note events normalize unused duration/tempo and keep zero delta");
    check(events[5].data1 == 9 && events[5].data2 == 0 && events[6].data1 == 10 &&
          events[7].data2 == 0, "program running status and channel pressure consume one datum");
    check(events[9].kind == a::SequenceEventKind::tempo && events[9].tempo_us_per_quarter == 500000 &&
          events[9].status == 0 && events[9].data1 == 0, "reduced tempo normalizes only meaningful fields");
    check(events.back().kind == a::SequenceEventKind::sequence_end && f.bytes == original,
          "last track emits only sequence end and source stays immutable");
    auto output = events[0]; const auto before = output;
    a::SequenceReadResult result;
    for (unsigned i = 0; i != 3; ++i)
        check(reader.next(output, result) == a::SequenceStep::end && result && equal(output, before) &&
              reader.ticks() == 16, "postterminal calls are stable and do not republish final event");
}
void delta_and_duration_ranges() {
    Fixture f;
    f.track(0, {});
    const std::uint32_t values[] = {0,127,128,16383,16384,0x0fffffff,0xffffffff};
    std::uint64_t total = 0;
    for (auto value : values) {
        vlq(f.bytes, value); add(f.bytes, {0x90,60,80}); vlq(f.bytes, value);
    }
    add(f.bytes, {0,0xff,0x2f});
    a::SequenceReader reader;
    check(bool(reader.start(f.entry())), "bounded u32 VLQs admitted");
    for (auto value : values) {
        const auto event = step(reader); total += value;
        check(event.delta_ticks == value && event.tick == total && event.duration_ticks == value,
              "all bounded VLQ widths preserve exact unsigned integer values");
    }
    check(step(reader).tick == total, "durations never advance track deltas");
}
void tie_order_and_independent_status() {
    Fixture f;
    f.track(0, {5,0xc0,1,0,2,0,3,5,0xff,0x2f});
    f.track(1, {5,0xc1,4,0,5,5,0xff,0x2f});
    f.track(15,{5,0xcf,6,0,7,5,0xff,0x2f});
    a::SequenceReader reader;
    check(bool(reader.start(f.entry())), "multi-track tie fixture admitted");
    const auto events = calendar(reader);
    check(events.size() == 10, "tie fixture has seven MIDI records and three end controls");
    const unsigned slots[] = {0,0,0,1,1,15,15,0,1,15};
    for (std::size_t i = 0; i < events.size() && i < 10; ++i) {
        check(events[i].track == slots[i] && events[i].tick == (i < 7 ? 5 : 10),
              "lower track drains all equal-time zero-delta events before higher track");
        if (i < 7) check(events[i].status == 0xc0 + slots[i], "running status remains per-track");
        else check(events[i].kind == (i == 9 ? a::SequenceEventKind::sequence_end : a::SequenceEventKind::track_end),
                   "intermediate versus final track endings preserve tie order");
    }
}
void compression_families_and_checkpoints() {
    {
        Fixture f; f.track(0, {0,0xc0,5,0x10,0xc0,6,0xfe,0,3,3,0,0xff,0x2f});
        a::SequenceReader reader; check(bool(reader.start(f.entry())), "whole event backup admitted");
        auto events = calendar(reader);
        check(events.size() == 4 && events[2].data1 == 6 && events[2].tick == 32,
              "whole event backup replays delta/status/data");
    }
    {
        Fixture f; f.track(0, {0,0xc0,5,0x10,0xc0,0xfe,0,3,1,0,0xff,0x2f});
        a::SequenceReader reader; check(bool(reader.start(f.entry())), "partial event backup admitted");
        auto events = calendar(reader);
        check(events.size() == 3 && events[1].data1 == 5 && events[1].tick == 16,
              "backup can replace only a program datum");
    }
    {
        Fixture f; const auto source = f.track(0, {0,0xc0,5,0,0xff,0x2f});
        const auto marker = f.track(1, {0xfe,0,0,3,0,0xff,0x2f});
        f.bytes[marker + 1] = std::byte((marker - source) >> 8);
        f.bytes[marker + 2] = std::byte((marker - source) & 255);
        a::SequenceReader reader; check(bool(reader.start(f.entry())), "cross-track backup admitted");
        auto events = calendar(reader);
        check(events.size() == 4 && events[0].status == 0xc0 && events[1].kind == a::SequenceEventKind::track_end &&
              events[2].track == 1 && events[2].status == 0xc0 && events[2].data1 == 5,
              "cross-track copy preserves encoded channel and end ordering");
    }
    {
        Fixture f; const auto start = f.track(0, {0,0xff,0x51,1,0xfe,0xfe,2,0,0xff,0x51});
        const auto marker = f.bytes.size();
        add(f.bytes, {0xfe,0,static_cast<unsigned>(marker - (start + 4)),3,0,0xff,0x2f});
        a::SequenceReader reader; check(bool(reader.start(f.entry())), "escaped and raw FE fixture admitted");
        check(step(reader).tempo_us_per_quarter == 0x01fe02, "outside backup FE FE emits one literal FE");
        check(step(reader).tempo_us_per_quarter == 0xfefe02, "active backup FE bytes are neither unescaped nor nested");
    }
    {
        Fixture f;
        const auto start = f.track(0, {0,0xc0,5,0,0xc0,6,0,0xff,0x2f});
        const auto marker = f.track(1, {0xfe,0,0,6,0,0xff,0x2f});
        f.bytes[marker + 1] = std::byte((marker - start) >> 8);
        f.bytes[marker + 2] = std::byte((marker - start) & 255);
        a::SequenceReader reader; check(bool(reader.start(f.entry())), "multi-event backup admitted");
        bool entered_backup = false;
        for (unsigned i = 0; i != 4; ++i)
            if (step(reader).track == 1) { entered_backup = true; break; }
        check(entered_backup, "bounded prefix reaches track-one backup");
        // Track 1 has emitted its first copied event, with three raw bytes left.
        const auto checkpoint = reader;
        check(step(reader).data1 == 6, "active backup spans multiple next calls");
        auto restored = checkpoint;
        check(step(restored).data1 == 6, "value copy preserves mid-backup cursor");
        same_calendar(reader, restored);
    }
    {
        Fixture f;
        f.track(0,{0,0xff,0x51,1,2,3,0,0xff,0x51,0xfe,0,1,3,0,0xff,0x2f});
        a::SequenceReader reader;
        check(bool(reader.start(f.entry())), "overlapping physical pattern range admitted");
        step(reader);
        check(step(reader).tempo_us_per_quarter == 0x51fe00,
              "overlap copies physical marker bytes without LZ expansion");
    }
    for (unsigned distance : {0xfdffu,0xffffu}) {
        Fixture f;
        const auto source = f.track(0,{0,0xc0,5,0,0xff,0x2f});
        f.bytes.resize(source+distance);
        f.track(1,{0xfe,distance>>8,distance&255,3,0,0xff,0x2f});
        a::SequenceReader reader;
        check(bool(reader.start(f.entry())), "documented FDFF and original-observed FFFF distances admitted");
        const auto events = calendar(reader);
        check(events.size() == 4 && events[2].data1 == 5 && events[2].track == 1,
              "maximum pattern distance still reads bounded cross-track physical bytes");
    }
    {
        Fixture f;
        const auto source = f.track(0,{});
        for (unsigned i = 0; i != 85; ++i) add(f.bytes,{0,0xc0,i});
        add(f.bytes,{0,0xff,0x2f});
        const auto marker = f.track(1,{0xfe,0,0,255,0,0xff,0x2f});
        f.bytes[marker+1] = std::byte((marker-source)>>8);
        f.bytes[marker+2] = std::byte((marker-source)&255);
        a::SequenceReader reader;
        check(bool(reader.start(f.entry())), "maximum 255-byte raw backup admitted");
        const auto events = calendar(reader);
        check(events.size() == 172, "maximum backup spans 85 distinct event calls without count wrap");
        for (std::size_t i = 0; i != 85 && events.size() == 172; ++i)
            check(events[i].data1 == i && events[86+i].data1 == i && events[86+i].track == 1,
                  "maximum raw backup preserves every copied event datum");
    }
    for (unsigned final_delta : {0u,7u}) {
        Fixture f;
        const auto source = f.bytes.size();
        for (unsigned i = 0; i != 85; ++i) add(f.bytes,{0,0xc0,i});
        add(f.bytes,{final_delta,0xc0,77});
        f.bytes.resize(400);
        const auto marker = f.track(0,{0xfe,0,0,0,0xc1,66,0,0xff,0x2f});
        f.bytes[marker+1] = std::byte((marker-source)>>8);
        f.bytes[marker+2] = std::byte((marker-source)&255);
        a::SequenceReader reader;
        check(bool(reader.start(f.entry())), "zero-encoded 256-byte backup has a bounded source");
        const auto events = calendar(reader);
        check(events.size() == 87 && reader.ended() && reader.ticks() == final_delta,
              "zero pattern length copies exactly 256 raw bytes and the last copied delta advances time");
        if (events.size() == 87) {
            for (std::size_t i = 0; i != 85; ++i)
                check(events[i].status == 0xc0 && events[i].data1 == i, "zero-length backup retains all first 255 bytes");
            check(events[85].status == 0xc1 && events[85].data1 == 66 &&
                  events[86].kind == a::SequenceEventKind::sequence_end,
                  "copied 256th byte supplies delta then primary sentinel resumes, not source byte 257");
        }
    }
}
Fixture loop(unsigned initial, unsigned current, std::uint32_t delta = 16) {
    Fixture f;
    f.track(0, {0,0xff,0x2e,0,0xff});
    const auto body = f.bytes.size();
    vlq(f.bytes, delta); add(f.bytes, {0xc0,5,0,0xff,0x2d,initial,current,0,0,0,0});
    put32(f.bytes, f.bytes.size() - 4, static_cast<std::uint32_t>(f.bytes.size() - body));
    add(f.bytes, {0,0xff,0x2f});
    return f;
}
void compressed_loop_primary_payload_and_retained_backup() {
    for (unsigned prefix : {1u,2u,3u,4u}) {
        Fixture f;
        const auto source = f.bytes.size();
        const unsigned start[] = {0xff,0x2e,0,0xff};
        add(f.bytes,{0xff,0x2e,0,0xff});
        f.track(0,{0});
        const auto marker = f.bytes.size();
        add(f.bytes,{0xfe,0,static_cast<unsigned>(marker-source),prefix});
        for (unsigned i = prefix; i != 4; ++i) add(f.bytes,{start[i]});
        add(f.bytes,{0,0xc0,5,0,0xff,0x2f});
        a::SequenceReader reader;
        check(bool(reader.start(f.entry())), "compressed loop-start prefix admitted");
        const auto events = calendar(reader);
        check(events.size() == 3 && events[0].kind == a::SequenceEventKind::loop_start &&
              events[1].data1 == 5 && events[2].kind == a::SequenceEventKind::sequence_end,
              "loop-start bytes pass normally through backup and primary boundaries");
    }
    for (unsigned length : {2u,5u,8u}) {
        Fixture f;
        const auto source = f.bytes.size();
        add(f.bytes,{0xff,0x2d,0,0xc0,55,0,0xc0,56});
        f.track(0,{0,0xff,0x2e,0,0xff});
        const auto body = f.bytes.size();
        add(f.bytes,{1,0xc0,17,0});
        const auto marker = f.bytes.size();
        add(f.bytes,{0xfe,0,static_cast<unsigned>(marker-source),length,1,1,0,0,0,0});
        put32(f.bytes,f.bytes.size()-4,static_cast<std::uint32_t>(f.bytes.size()-body));
        add(f.bytes,{0,0xff,0x2f});
        const auto original = f.bytes;
        a::SequenceReader reader;
        check(bool(reader.start(f.entry())), "copied loop-end control with raw primary payload admitted");
        const auto events = calendar(reader);
        const auto extra = (length-2)/3;
        check(events.size() == 6+2*extra && reader.ticks() == 2 && reader.ended(),
              "copied FF2D uses primary six-byte payload and primary payload-end jump anchor");
        if (events.size() != 6+2*extra) continue;
        check(events[2].kind == a::SequenceEventKind::loop_end &&
              events[3+extra].data1 == 17 && events[3+extra].tick == 2,
              "jump returns to body delta after all retained backup records");
        for (unsigned iteration = 0; iteration != 2; ++iteration)
            for (unsigned i = 0; i < extra; ++i) {
                const auto& event = events[3+iteration*(2+extra)+i];
                check(event.kind == a::SequenceEventKind::midi && event.data1 == 55+i &&
                      event.tick == iteration+1,
                      "unconsumed raw backup bytes survive both jump and loop fallthrough");
            }
        check(f.bytes == original, "copied loop mutates neither source bytes nor primary bytes");
    }
    {
        Fixture f;
        const auto source = f.bytes.size(); add(f.bytes,{0xff});
        f.track(0,{0,0xff,0x2e,0,0xff});
        const auto body = f.bytes.size(); add(f.bytes,{1,0xc0,17,0});
        const auto marker = f.bytes.size();
        add(f.bytes,{0xfe,0,static_cast<unsigned>(marker-source),1,0x2d,1,1,0,0,0,0});
        put32(f.bytes,f.bytes.size()-4,static_cast<std::uint32_t>(f.bytes.size()-body));
        add(f.bytes,{0,0xff,0x2f});
        a::SequenceReader reader;
        check(bool(reader.start(f.entry())) && calendar(reader).size() == 6 && reader.ticks() == 2,
              "FF and loop subtype may straddle backup/primary boundary");
    }
}

void finite_infinite_and_nested_loops() {
    for (unsigned count = 0; count != 4; ++count) {
        auto f = loop(count, count); const auto original = f.bytes;
        a::SequenceReader reader; check(bool(reader.start(f.entry())), "finite loop admitted");
        const auto events = calendar(reader);
        unsigned bodies = 0, starts = 0, ends = 0;
        for (const auto& e : events) {
            bodies += e.kind == a::SequenceEventKind::midi;
            starts += e.kind == a::SequenceEventKind::loop_start;
            ends += e.kind == a::SequenceEventKind::loop_end;
        }
        check(bodies == count + 1 && starts == 1 && ends == count + 1 && reader.ticks() == (count + 1) * 16,
              "current count decrements and zero resets/falls; jumps target body delta not loop-start meta");
        check(original == f.bytes, "loop decrement never mutates immutable wire bytes");
    }
    for (const auto counts : {std::pair{3u,0u},std::pair{0u,3u},std::pair{255u,0u},std::pair{255u,1u}}) {
        auto f = loop(counts.first, counts.second);
        a::SequenceReader reader; check(bool(reader.start(f.entry())), "mismatched loop counts admitted");
        const auto events = calendar(reader);
        check(events.size() == 2 * (counts.second + 1) + 2 && reader.ticks() == 16 * (counts.second + 1),
              "current rather than initial controls finite branch");
    }
    for (unsigned initial : {0u,1u,3u}) {
        auto f = loop(initial,255);
        a::SequenceReader reader;
        check(bool(reader.start(f.entry())), "mismatched CURRENT255 loop admitted");
        check(calendar(reader,12).size() == 12 && !reader.ended() && reader.ticks() == 96,
              "CURRENT255 repeats unchanged regardless of INITIAL value");
    }
    {
        auto f = loop(254,254);
        a::SequenceReader reader;
        check(bool(reader.start(f.entry())), "raw primary FE count bytes admitted");
        check(calendar(reader).size() == 512 && reader.ticks() == 4080,
              "loop payload FE FE are two raw count bytes, not a literal escape");
    }
    {
        auto f = loop(255,255);
        a::SequenceReader reader; check(bool(reader.start(f.entry())), "255 sentinel loop admitted");
        const auto events = calendar(reader,1024);
        unsigned ends = 0;
        for (const auto& e : events) ends += e.kind == a::SequenceEventKind::loop_end;
        check(events.size() == 1024 && ends == 511 && reader.ticks() == 8192 && !reader.ended(),
              "255 repeats without decrement through bounded 1024-event prefix");
        auto zero = loop(255,255,0);
        check(bool(reader.start(zero.entry())), "zero-time sentinel loop admitted");
        check(calendar(reader,1024).size() == 1024 && reader.ticks() == 0 && !reader.ended(),
              "one next returns one encoded event even in zero-time repeating loop");
    }
    {
        Fixture f;
        f.track(0, {0,0xff,0x2e,0,0xff}); const auto outer = f.bytes.size();
        add(f.bytes, {0,0xff,0x2e,1,0xff}); const auto inner = f.bytes.size();
        add(f.bytes, {10,0xc0,5,0,0xff,0x2d,1,1,0,0,0,0});
        put32(f.bytes,f.bytes.size()-4,static_cast<std::uint32_t>(f.bytes.size()-inner));
        add(f.bytes, {5,0xc0,6,0,0xff,0x2d,1,1,0,0,0,0});
        put32(f.bytes,f.bytes.size()-4,static_cast<std::uint32_t>(f.bytes.size()-outer));
        add(f.bytes, {0,0xff,0x2f});
        a::SequenceReader reader; check(bool(reader.start(f.entry())), "nested finite loops admitted");
        const auto events = calendar(reader); unsigned five = 0, six = 0;
        for (const auto& e : events) if (e.kind == a::SequenceEventKind::midi) {
            five += e.data1 == 5; six += e.data1 == 6;
        }
        check(events.size() == 16 && five == 4 && six == 2 && reader.ticks() == 50,
              "inner counter reset allows full replay on next outer iteration");
    }
}
void reader_independence_shadow_rollback_and_rebind() {
    auto f = loop(2,2);
    a::SequenceReader first, second;
    check(bool(first.start(f.entry())) && bool(second.start(f.entry())), "two readers bind same immutable bytes");
    step(first); step(first); step(first); // first CURRENT decrement
    const auto checkpoint = first;
    const auto untouched = calendar(second);
    check(untouched.size() == 8 && second.ticks() == 48, "another reader does not observe first reader's decrement");
    same_calendar(first, checkpoint);
    auto live = checkpoint;
    {
        auto shadow = live;
        step(shadow); step(shadow); // stage another decrement, then discard failed CPU transaction
    }
    same_calendar(live, checkpoint);
    check(bool(first.start(f.entry())), "successful rebind resets overlay and cursors");
    auto events = calendar(first);
    check(events.size() == 8 && first.ticks() == 48, "rebind reads initial wire counter, not stale overlay");
    Fixture bad; put32(bad.bytes,64,0);
    auto active = checkpoint;
    const auto result = active.start(bad.entry());
    check(!result && result.error == a::SequenceReadError::invalid_header, "invalid rebind rejected");
    same_calendar(active, checkpoint);
    Fixture late;
    late.track(0,{0,0xc0,9,0,0xff,0x2f});
    late.track(15,{0x80});
    const auto late_result = active.start(late.entry());
    check(!late_result && late_result.error == a::SequenceReadError::truncated && late_result.track == 15,
          "late initial-delta rejection does not publish earlier primed tracks");
    same_calendar(active, checkpoint);
}
void header_and_terminal_bounds() {
    a::SequenceReader reader;
    rejected_next(reader,a::SequenceReadError::uninitialized);
    Fixture empty;
    check(bool(reader.start(empty.entry())) && !reader.ended(), "empty track set is admitted");
    check(step(reader).kind == a::SequenceEventKind::sequence_end && reader.ended(), "empty track set emits final event once");
    for (unsigned size = 0; size != 68; ++size) {
        Fixture f; f.bytes.resize(size);
        const auto result = reader.start(f.entry());
        check(!result && result.error == a::SequenceReadError::truncated && reader.ended(),
              "every truncated header preserves previous terminal binding");
    }
    for (auto offset : {1u,64u,67u,68u,0xffffffffu}) {
        Fixture f; put32(f.bytes,0,offset);
        const auto result = reader.start(f.entry());
        check(!result && result.error == a::SequenceReadError::invalid_offset && result.offset == offset,
              "present track starts must point to a payload byte");
    }
    Fixture aliased; aliased.track(0,{0,0xc0,5,0,0xff,0x2f}); put32(aliased.bytes,4,68);
    check(bool(reader.start(aliased.entry())), "duplicate track starts admitted");
    auto events = calendar(reader);
    check(events.size() == 4 && events[0].track == 0 && events[2].track == 1,
          "aliased tracks retain independent cursors and slot tie scheduling");
    Fixture dma; dma.track(0,{0,0xc0}); add(dma.bytes,{5});
    auto entry = dma.entry(); entry.encoded = entry.encoded.first(entry.encoded.size()-1);
    check(bool(reader.start(entry)), "start can prime delta with unavailable following datum");
    rejected_next(reader,a::SequenceReadError::truncated);
}
void physical_counter_aliases_and_raw_reads() {
    {
        auto f = loop(2,2);
        put32(f.bytes,4,68);
        a::SequenceReader reader;
        check(bool(reader.start(f.entry())), "two tracks can share a physical loop counter");
        const auto events = calendar(reader);
        unsigned bodies[2]{};
        for (const auto& event : events)
            if (event.kind == a::SequenceEventKind::midi) ++bodies[event.track];
        check(events.size() == 16 && bodies[0] == 2 && bodies[1] == 4 && reader.ticks() == 64,
              "same-reader aliases see each other's decrements and resets");
    }
    {
        auto f = loop(2,2);
        // CURRENT is byte 80 of this independently authored uncompressed loop.
        const auto second = f.track(1,{16,0xc1});
        const auto marker = f.bytes.size();
        add(f.bytes,{0xfe,0,static_cast<unsigned>(marker-80),1,0,0xff,0x2f});
        a::SequenceReader reader;
        check(bool(reader.start(f.entry())), "counter reused as ordinary backup data admitted");
        bool found = false;
        for (const auto& event : calendar(reader))
            if (event.kind == a::SequenceEventKind::midi && event.track == 1) {
                found = true;
                check(event.data1 == 1 && event.tick == 16,
                      "raw backup consults reader-local physical counter overlay");
            }
        check(found && second > 80, "counter-data observer emitted its program event");
    }
    {
        auto f = loop(1,1);
        const auto second = loop(1,1);
        f.bytes.resize(68+4096);
        f.track(1,{});
        f.bytes.insert(f.bytes.end(),second.bytes.begin()+68,second.bytes.end());
        a::SequenceReader reader;
        check(bool(reader.start(f.entry())), "colliding sparse counter keys admitted");
        const auto events = calendar(reader);
        unsigned bodies[2]{};
        for (const auto& event : events)
            if (event.kind == a::SequenceEventKind::midi) ++bodies[event.track];
        check(events.size() == 12 && bodies[0] == 2 && bodies[1] == 2 && reader.ticks() == 32,
              "open-address collisions preserve distinct physical counter identities");
    }
}

void every_event_field_truncation() {
    const std::initializer_list<unsigned> complete[] = {
        {0,0x80,60,32}, {0,0x90,60,80,0x81,0x70}, {0,0xa0,60,32},
        {0,0xb0,7,100}, {0,0xc0,5}, {0,0xd0,5}, {0,0xe0,0,64},
        {0,0xff,0x51,7,0xa1,0x20}, {0,0xff,0x2f}, {0,0xff,0x2e,0,0xff},
        {0,0xff,0x2d,1,1,0,0,0,9}
    };
    for (const auto record : complete)
        for (std::size_t length = 1; length < record.size(); ++length) {
            Fixture f; f.track(0,record); f.bytes.resize(68+length);
            a::SequenceReader reader;
            check(bool(reader.start(f.entry())), "field truncation leaves complete initial delta");
            rejected_next(reader,a::SequenceReadError::truncated);
        }
}

void malformed_grammar_and_atomicity() {
    const std::initializer_list<unsigned> records[] = {
        {0,0x90,60,80,0x81}, {0,0xff}, {0,0xff,0x51,1,2}, {0,0xff,0x2e,0},
        {0,0xff,0x2d,1,1,0,0,0}, {0,0xfe}, {0,0xfe,0}, {0,0xfe,0,1}
    };
    for (const auto record : records) {
        Fixture f; f.track(0,record);
        a::SequenceReader reader; check(bool(reader.start(f.entry())), "valid initial delta admitted before truncated event");
        rejected_next(reader,a::SequenceReadError::truncated);
    }
    struct Bad { std::initializer_list<unsigned> bytes; a::SequenceReadError error; };
    const Bad cases[] = {
        {{0,60},a::SequenceReadError::invalid_status},
        {{0,0xf0},a::SequenceReadError::invalid_status},
        {{0,0xc0,0x80},a::SequenceReadError::invalid_status},
        {{0,0x90,60,0xff},a::SequenceReadError::invalid_status},
        {{0,0xff,0x50},a::SequenceReadError::invalid_meta},
        {{0,0xff,0x2e,128,0xff},a::SequenceReadError::invalid_loop},
        {{0,0xff,0x2e,0,0},a::SequenceReadError::invalid_loop},
        {{0,0xff,0x2d,1,1,0xff,0xff,0xff,0xff},a::SequenceReadError::invalid_loop},
        {{0,0xfe,0,0,1},a::SequenceReadError::invalid_backref},
        {{0,0xfe,0,2,1},a::SequenceReadError::invalid_backref},
        {{0,0xfe,0xff,0,1},a::SequenceReadError::invalid_backref},
        {{0,0xfe,0,1,255},a::SequenceReadError::invalid_backref},
        {{0,0xfe,0,1,0},a::SequenceReadError::invalid_backref},
        {{0,0x90,60,80,0x90,0x80,0x80,0x80,0},a::SequenceReadError::invalid_vlq},
        {{0,0x90,60,80,0x80,0x80,0x80,0x80,0x80},a::SequenceReadError::invalid_vlq}
    };
    for (const auto& bad : cases) {
        Fixture f; f.track(0,bad.bytes);
        a::SequenceReader reader; check(bool(reader.start(f.entry())), "malformed body has independently valid initial delta");
        rejected_next(reader,bad.error);
    }
    {
        Fixture f; f.track(0,{0x90,0x80,0x80,0x80,0});
        a::SequenceReader reader;
        check(reader.start(f.entry()).error == a::SequenceReadError::invalid_vlq, "initial delta overflow rejected atomically");
    }
    {
        Fixture f; f.track(0,{0,0xc0,5,0,0xff,0x51,0,0,1,0,6});
        a::SequenceReader reader; check(bool(reader.start(f.entry())), "meta clears running fixture admitted");
        step(reader); step(reader); rejected_next(reader,a::SequenceReadError::invalid_status);
    }
    {
        Fixture f; f.track(0,{0,0xc0,5,0,0xff,0x2e,0,0xff,0,6});
        a::SequenceReader reader; check(bool(reader.start(f.entry())), "loop clears running fixture admitted");
        step(reader); step(reader); rejected_next(reader,a::SequenceReadError::invalid_status);
    }
    {
        Fixture f;
        f.track(0,{0,0xc0,5,1,0xc0,6,0,0xff,0x2f});
        f.track(1,{0,0xc1,0x80});
        a::SequenceReader reader;
        check(bool(reader.start(f.entry())), "two-track staged-delta failure fixture admitted");
        check(step(reader).data1 == 5, "lower track emits before malformed equal-time higher track");
        rejected_next(reader,a::SequenceReadError::invalid_status);
    }
    {
        Fixture f; f.track(0,{0,0xc0,5,0x81}); f.track(1,{10,0xc1,6,0,0xff,0x2f});
        // Remove the second track bytes from the first cursor's possible VLQ by
        // placing malformed over-wide continuation bytes before that track.
        f.bytes.insert(f.bytes.begin()+72, {std::byte{0x80},std::byte{0x80},std::byte{0x80},std::byte{0x80}});
        put32(f.bytes,4,76);
        a::SequenceReader reader; check(bool(reader.start(f.entry())), "deferred delta error fixture admitted");
        check(step(reader).data1 == 5, "complete preceding event publishes before malformed following delta");
        rejected_next(reader,a::SequenceReadError::invalid_vlq);
    }
}
void counter_capacity_is_checked_atomically() {
    Fixture f; f.track(0,{});
    for (std::size_t i = 0; i != a::SequenceReader::counter_capacity + 1; ++i)
        add(f.bytes,{0,0xff,0x2d,0,0,0,0,0,9});
    add(f.bytes,{0,0xff,0x2f});
    a::SequenceReader reader;
    check(bool(reader.start(f.entry())), "counter capacity fixture admits initial delta");
    for (std::size_t i = 0; i != a::SequenceReader::counter_capacity; ++i)
        check(step(reader).kind == a::SequenceEventKind::loop_end, "distinct physical counter admitted through exact capacity");
    rejected_next(reader,a::SequenceReadError::loop_capacity);
    auto clean = loop(1,1);
    check(bool(reader.start(clean.entry())) && calendar(reader).size() == 6,
          "rebind clears full overlay, including after capacity failure");
}
} // namespace
int main() {
    midi_timing_and_normalization(); delta_and_duration_ranges(); tie_order_and_independent_status();
    compression_families_and_checkpoints(); finite_infinite_and_nested_loops();
    compressed_loop_primary_payload_and_retained_backup();
    reader_independence_shadow_rollback_and_rebind(); header_and_terminal_bounds();
    malformed_grammar_and_atomicity(); every_event_field_truncation();
    physical_counter_aliases_and_raw_reads(); counter_capacity_is_checked_atomically();
    if (failures) return EXIT_FAILURE;
    std::cout << "Native compressed sequence behavior, bounds and transaction checks passed\n";
    return EXIT_SUCCESS;
}
