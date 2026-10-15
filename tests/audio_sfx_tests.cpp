#include "35mmisle/audio_sfx.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>

namespace a = isle::audio;
namespace s = isle::audio::sfx;
namespace {
int failures = 0;
void check(bool condition, std::string_view meaning) {
    if (!condition) { ++failures; std::cerr << "FAIL: " << meaning << '\n'; }
}
void put16(a::Bytes& bytes, std::size_t offset, std::uint16_t value) {
    bytes[offset] = std::byte(value >> 8); bytes[offset + 1] = std::byte(value & 255);
}
void put32(a::Bytes& bytes, std::size_t offset, std::uint32_t value) {
    put16(bytes, offset, static_cast<std::uint16_t>(value >> 16));
    put16(bytes, offset + 2, static_cast<std::uint16_t>(value));
}
struct Input {
    std::shared_ptr<a::Bytes> bank = std::make_shared<a::Bytes>(208);
    std::shared_ptr<a::Bytes> table = std::make_shared<a::Bytes>(18);
    Input() {
        auto& b = *bank;
        put16(b, 0, 0x4231); put16(b, 2, 1); put32(b, 4, 16);
        put16(b, 16, 1); put32(b, 20, 32000); put32(b, 28, 32);
        b[32] = std::byte{100}; b[33] = std::byte{64};
        put16(b, 44, 200); put16(b, 46, 1); put32(b, 48, 64);
        put32(b, 64, 80); put32(b, 68, 96); put32(b, 72, 104);
        b[76] = std::byte{64}; b[77] = std::byte{75};
        put32(b, 80, 0); put32(b, 84, 0xffffffff); put32(b, 88, 6000);
        b[92] = std::byte{127}; b[93] = std::byte{127};
        b[97] = std::byte{127}; b[99] = std::byte{127}; b[100] = std::byte{60};
        put32(b, 104, 0); put32(b, 108, 18);
        put32(b, 116, 124); put32(b, 120, 168);
        put32(b, 124, 0); put32(b, 128, 32); put32(b, 132, 0xffffffff);
        put16(b, 136, 0x8000); put16(b, 166, 0x7fff);
        put32(b, 168, 2); put32(b, 172, 1);
        put16(b, 176, 0x8000); put16(b, 206, 0x7fff);
    }
    a::BankResult decode() const { return a::hydrate_bank(bank, table); }
};
const a::Sound& selected(const a::BankFile& bank) {
    return *bank.banks()[0]->instruments[0]->sounds[0];
}
const s::Coefficients& coefficients() {
    // Authored linear pan and midpoint logarithms, not original asset bytes.
    static const auto data = [] {
        s::Coefficients values{};
        for (std::uint32_t i = 0; i < values.pan.size(); ++i)
            values.pan[i] = static_cast<std::uint16_t>(((127u - i) * 32767u) / 127u);
        for (std::size_t i = 0; i < values.logarithm.size(); ++i)
            values.logarithm[i] = std::log2((8.5 + static_cast<double>(i)) / 16.0);
        return values;
    }();
    return data;
}
void quotas_and_admission() {
    std::uint32_t frames = 999;
    check(s::service_quota({32006}, {1, 0, 0}, frames) == s::Error::none && frames == 640,
          "quota uses actual AI domain, not nominal gameplay frames");
    check(s::service_quota({32006}, {1, std::numeric_limits<std::uint32_t>::max(), 0}, frames) == s::Error::none && frames == 528,
          "large AI lag cannot wrap quota unsigned");
    check(s::service_quota({32006}, {1, 0, 17}, frames) == s::Error::invalid_service && frames == 0,
          "unaligned explicit quota fails without output");
    Input input;
    auto bank = input.decode();
    check(bool(bank), "synthetic retained graph hydrates");
    if (!bank) return;
    a::Sound foreign = selected(bank.asset);
    auto rejected = s::Producer::create(bank.asset, foreign, {32006}, coefficients());
    check(rejected.error == s::Error::foreign_sound && !rejected.producer, "foreign descriptor is not retained by coincidence");
    Input unaligned_input;
    put32(*unaligned_input.bank, 124, 1);
    auto unaligned = unaligned_input.decode();
    check(bool(unaligned), "bank decoder remains general for non-frame loop");
    if (unaligned) {
        rejected = s::Producer::create(unaligned.asset, selected(unaligned.asset), {32006}, coefficients());
        check(rejected.error == s::Error::unsupported_loop && !rejected.producer, "producer explicitly rejects unsupported loop alignment");
    }
}
void staging_leases_and_failure() {
    Input input;
    auto bank = input.decode();
    if (!bank) { check(false, "fixture hydrates"); return; }
    const auto* sound = &selected(bank.asset);
    auto made = s::Producer::create(bank.asset, *sound, {32006}, coefficients());
    check(bool(made), "retained aligned ADPCM sound admitted");
    if (!made) return;
    auto& producer = *made.producer;
    const auto* stable_words = producer.rdram_words().data();
    std::weak_ptr<const a::Bytes> bank_owner = input.bank, table_owner = input.table;
    bank.asset = {}; input.bank.reset(); input.table.reset();
    check(!bank_owner.expired() && !table_owner.expired(), "producer retains whole immutable graph");
    std::uint64_t generation = 0;
    check(producer.stage_start({}, generation) == s::Error::none && generation != 0, "start reserves a fresh generation");
    s::TaskView idle;
    check(producer.build({1, 0, 64}, idle) == s::Error::none && idle.generation != generation,
          "posted start does not own the task built before staging");
    check(producer.stage() == s::Error::none && producer.voice_active(), "stage applies for the next build while current task lives");
    check(producer.reset() == s::Error::task_busy, "pending task forbids reset/recycling");
    s::TaskView task;
    check(producer.build({2, 0, 64}, task) == s::Error::task_busy && task.task_id == 0,
          "second build publishes nothing while task is pending");
    s::PcmView pcm;
    check(producer.complete(idle.task_id + 1, idle.generation, true, pcm) == s::Error::stale_task && producer.has_lease(),
          "stale completion cannot recycle a real pending task");
    // Terminal acknowledgements here exercise ownership only, NOT RSP/PCM
    // execution. Exact private aspMain integration is a separate runtime gate.
    check(producer.complete(idle.task_id, idle.generation, true, pcm) == s::Error::none, "matching completion grants a PCM lease");
    check(producer.reset() == s::Error::task_busy && producer.build({2, 0, 64}, task) == s::Error::task_busy,
          "completion does not release the consumer's PCM");
    std::array<std::int16_t, 4> destination{17, 31, 47, 63};
    const auto untouched = destination;
    check(producer.copy_pcm(pcm, std::span(destination).subspan(1, 2)) == s::Error::invalid_parameter &&
          destination == untouched && producer.has_lease(),
          "short PCM destination neither overwrites guards nor consumes its lease");
    check(producer.release(pcm.task_id, pcm.generation) == s::Error::none, "explicit release retires completed output");
    check(producer.build({2, 0, 64}, task) == s::Error::none && task.generation == idle.generation,
          "posted controls do not bypass the next sound-client callback");
    check(producer.complete(task.task_id, task.generation, true, pcm) == s::Error::none &&
          producer.release(pcm.task_id, pcm.generation) == s::Error::none,
          "pre-callback task retires without consuming the staged voice");
    check(producer.build({3, 0, 512}, task) == s::Error::none && task.generation == generation,
          "staged generation starts when production crosses its client deadline");
    check(producer.stage_pitch(generation, 1.5f) == s::Error::none && producer.stage_pan(generation, 127) == s::Error::none &&
          producer.stage() == s::Error::none, "live controls may stage independently of task completion");
    check(producer.complete(task.task_id, task.generation, false, pcm) == s::Error::execution_failed && !producer.has_lease(),
          "terminal execution failure never publishes PCM");
    check(producer.build({4, 0, 64}, task) == s::Error::needs_reset, "failed DSP state is poisoned, not silently reused");
    check(producer.reset() == s::Error::none && producer.rdram_words().data() == stable_words,
          "quiescent reset retains stable borrowed RDRAM storage");
    check(producer.stage_stop(generation) == s::Error::stale_generation, "reset invalidates old voice handles");
    std::uint64_t replay = 0;
    check(producer.stage_start({}, replay) == s::Error::none && replay > generation && producer.stage() == s::Error::none,
          "replay has a fresh non-recycled voice generation");
    check(producer.build({1, 0, 16}, task) == s::Error::none && task.task_id > idle.task_id && task.generation == replay,
          "reset preserves monotonic task identities");
    check(producer.release(idle.task_id, idle.generation) == s::Error::stale_task && producer.has_lease(),
          "delayed release cannot free a replay task");
    check(producer.complete(task.task_id, task.generation, false, pcm) == s::Error::execution_failed && producer.reset() == s::Error::none,
          "test terminally retires its final task");
}
void bounded_events() {
    Input input;
    auto bank = input.decode();
    if (!bank) { check(false, "event fixture hydrates"); return; }
    auto made = s::Producer::create(bank.asset, selected(bank.asset), {32006}, coefficients());
    if (!made) { check(false, "event producer creates"); return; }
    auto& p = *made.producer;
    std::uint64_t generation = 0;
    check(p.stage_start({}, generation) == s::Error::none, "event start posts");
    for (std::size_t i = 1; i < s::Producer::event_capacity; ++i)
        check(p.stage_pan(generation, static_cast<std::uint8_t>(i)) == s::Error::none, "bounded event slot accepts");
    check(p.stage_stop(generation) == s::Error::event_capacity, "overflow is explicit before stage/publication");
    check(p.stage() == s::Error::none && p.stage_stop(generation) == s::Error::none,
          "staging restores bounded event capacity");
    check(p.reset() == s::Error::none, "unpublished queued controls can reset safely");
}
} // namespace
int main() {
    quotas_and_admission(); staging_leases_and_failure(); bounded_events();
    if (failures) return EXIT_FAILURE;
    std::cout << "Audio SFX resource admission, staging and lease checks passed\n";
    return EXIT_SUCCESS;
}
