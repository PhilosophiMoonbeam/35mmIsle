#include "game_audio.hpp"
#include "item_flute.hpp"
#include "audio_fixture.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>
#include <vector>

namespace g = isle::game_audio;
namespace a = isle::audio;
namespace f = isle::audio_test;
namespace items = isle::items;
namespace {
int failures = 0;
void check(bool value, std::string_view text) {
    if (!value) { ++failures; std::cerr << "FAIL: " << text << '\n'; }
}
g::Resources resources(std::uint32_t release_us = 6000) {
    f::Fixture fixture;
    f::put32(*fixture.bank, 88, release_us);
    auto bank = fixture.decode();
    auto bytes = std::make_shared<a::Bytes>(92);
    (*bytes)[0] = std::byte{'S'}; (*bytes)[1] = std::byte{'1'};
    f::put16(*bytes, 2, 1); f::put32(*bytes, 4, 12); f::put32(*bytes, 8, 80);
    f::put32(*bytes, 12, 68); f::put32(*bytes, 12 + 64, 48);
    const std::array<unsigned, 12> track{0, 0xc0, 0, 0, 0x90, 60, 127, 48, 48, 0xff, 0x2f, 0};
    for (unsigned i = 0; i < track.size(); ++i) (*bytes)[80 + i] = std::byte(track[i]);
    auto sequence = a::hydrate_sequences(bytes);
    check(bool(bank) && bool(sequence), "authored bank and compact sequence hydrate");
    g::Resources r;
    r.sounds = bank.asset; r.music = bank.asset; r.sequences = sequence.asset;
    r.coefficients = f::coefficients(); r.priorities = {100};
    return r;
}
// Authored finite sequences exercise native note/termination and independent
// players. This CPU fixture acknowledges ownership only; actual song identity
// and PCM influence are qualified by the private RSP Flute consumer.
constexpr std::array<std::uint32_t, 4> flute_tempos{240000, 320000, 400000, 480000};
g::Resources flute_resources(bool short_song = false) {
    auto result = resources(0);
    constexpr std::uint32_t table_bytes = 36, payload_bytes = 85, stride = 86;
    auto bytes = std::make_shared<a::Bytes>(table_bytes + flute_tempos.size() * stride);
    (*bytes)[0] = std::byte{'S'}; (*bytes)[1] = std::byte{'1'};
    f::put16(*bytes, 2, static_cast<std::uint16_t>(flute_tempos.size()));
    for (std::uint32_t song = 0; song < flute_tempos.size(); ++song) {
        const auto offset = table_bytes + song * stride;
        f::put32(*bytes, 4 + song * 8, offset);
        f::put32(*bytes, 8 + song * 8, payload_bytes);
        f::put32(*bytes, offset, 68); f::put32(*bytes, offset + 64, 48);
        const auto tempo = flute_tempos[song];
        const unsigned duration = short_song ? 16 : 127;
        const std::array<unsigned, 17> track{
            0, 0xff, 0x51, tempo >> 16, (tempo >> 8) & 255u, tempo & 255u,
            0, 0xc0, 0, 0, 0x90, 60 + song, 127, duration, duration, 0xff, 0x2f};
        for (std::size_t i = 0; i < track.size(); ++i)
            (*bytes)[offset + 68 + i] = std::byte(track[i]);
    }
    auto sequences = a::hydrate_sequences(bytes);
    check(bool(sequences), "four distinct authored flute scheduling sequences hydrate");
    result.sequences = sequences.asset;
    return result;
}
struct Consumer {
    g::Output::State state = g::Output::State::ready;
    std::uint32_t rate = g::source_rate, limit = 0;
    bool fail_submit = false, fail_finish = false;
    std::uint64_t accepted = 0;
    std::array<std::int16_t, 65536> storage{};
    unsigned finishes = 0;
    // CPU-contract-only executor fixture: no RSP readiness or PCM claim.
    const std::uint32_t* borrowed = nullptr;
    bool fail_initialize = false;
    unsigned retirements = 0;
    static bool initialize(void* p, std::span<std::uint32_t> words) noexcept {
        auto& c = *static_cast<Consumer*>(p); c.borrowed = words.data();
        return !c.fail_initialize && words.size_bytes() == a::AudioService::rdram_bytes;
    }
    static bool ready(void* p, std::span<std::uint32_t> words) noexcept {
        const auto& c = *static_cast<Consumer*>(p); return c.borrowed == words.data() && !c.fail_initialize;
    }
    static void retire(void* p) noexcept {
        auto& c = *static_cast<Consumer*>(p); c.borrowed = nullptr; ++c.retirements;
    }
    g::Executor executor() noexcept { return {this, initialize, ready, retire}; }
    static g::Output::Info inspect(void* p) noexcept {
        const auto& c = *static_cast<Consumer*>(p); return {c.state, c.rate};
    }
    static g::Output::Acceptance submit(void* p, std::span<const std::int16_t> samples) noexcept {
        auto& c = *static_cast<Consumer*>(p);
        if (c.fail_submit || c.state != g::Output::State::ready) return {};
        const auto n = std::min<std::uint32_t>(c.limit, static_cast<std::uint32_t>(samples.size() / 2));
        if ((c.accepted + n) * 2 > c.storage.size()) return {};
        std::copy_n(samples.begin(), n * 2u, c.storage.begin() + static_cast<std::ptrdiff_t>(c.accepted * 2));
        c.accepted += n;
        return {n * 2 == samples.size() ? g::Output::Submit::accepted : g::Output::Submit::would_block, n};
    }
    static bool finish(void* p) noexcept {
        auto& c = *static_cast<Consumer*>(p); ++c.finishes;
        if (c.fail_finish) return false;
        c.state = g::Output::State::draining; return true;
    }
    static g::Output::State poll(void* p) noexcept { return static_cast<Consumer*>(p)->state; }
    g::Output output() noexcept { return {this, inspect, submit, finish, poll}; }
};
g::CreateResult make(Consumer& consumer, std::uint32_t release_us = 6000) {
    const std::array sections{a::effects::Section{0, 160, 1000, -1000, 2000}};
    return g::Session::create(resources(release_us), {a::effects::Routing::network, 512, sections}, consumer.output(), consumer.executor());
}
g::CreateResult make_flute(Consumer& consumer, bool short_song = false) {
    const std::array sections{a::effects::Section{0, 160, 1000, -1000, 2000}};
    return g::Session::create(flute_resources(short_song), {a::effects::Routing::network, 512, sections},
                              consumer.output(), consumer.executor());
}
// CPU-only ownership acknowledgements: no RSP/PCM-generation claim. Authored
// sentinels explicitly write the producer's leased PCM range to verify slicing.
void cpu_complete(g::Session& s, const a::TaskView& task, bool sentinels = false) {
    if (sentinels) {
        auto* bytes = reinterpret_cast<unsigned char*>(s.service().rdram_words().data());
        for (std::uint32_t i = 0; i < task.frame_count * 2; ++i) {
            const auto value = static_cast<std::uint16_t>(i + 1);
            bytes[(task.pcm_address + i * 2) ^ 3u] = static_cast<unsigned char>(value >> 8);
            bytes[(task.pcm_address + i * 2 + 1) ^ 3u] = static_cast<unsigned char>(value);
        }
    }
    check(s.complete(task.token, true) == a::Error::none, "CPU protocol acknowledges matching task only");
}
bool cpu_tick(g::Session& session, std::uint64_t ordinal, std::uint64_t ns) {
    const auto before = session.source_frame_epoch();
    auto error = session.retrace(ordinal, ns);
    check(error == a::Error::none, "CPU consumer independent scheduler notification succeeds");
    if (error != a::Error::none) return false;
    error = session.submit();
    check(error == a::Error::none, "CPU consumer accepts the preceding source-gated PCM owner");
    if (error != a::Error::none) return false;
    check(session.source_frame_epoch() == before, "scheduler/output progression alone retains the source equality epoch");
    a::TaskView task;
    error = session.build(task);
    check(error == a::Error::none, "CPU consumer progresses real native command/voice state without terminal generation failure");
    if (error != a::Error::none) {
        if (task.token.serial != 0) session.complete(task.token, false);
        return false;
    }
    check((task.token.serial != 0) == (session.source_frame_epoch() != before),
          "CPU service iteration changes source epoch exactly for an actual admitted build");
    if (task.token.serial != 0) {
        cpu_complete(session, task);
        if (session.state() == g::Session::State::failed) return false;
        error = session.submit();
        check(error == a::Error::none, "CPU consumer retains the next source-gated PCM owner");
        if (error != a::Error::none) return false;
        check(session.source_frame_epoch() == task.service_id, "completion and PCM acceptance retain the actual built service identity");
    }
    return true;
}
void local_bootstrap_and_first_update(bool tick_before_build) {
    Consumer sink; sink.limit = 1024;
    auto made = make(sink); if (!made) { check(false, "bootstrap command owner initializes"); return; }
    auto& session = *made.session;
    const g::Session& source = session;
    check(source.source_frame_epoch() == 0, "source equality epoch begins before the first actual build");
    int sound = -1;
    check(session.play_sound(0, {}, sound) == a::Error::none && sound == 0, "first source sound request queues before publication");
    if (tick_before_build) check(session.retrace(10, 0) == a::Error::none, "already-running scheduler can queue its first notification before initial build");
    a::TaskView initial;
    check(session.build(initial) == a::Error::none && initial.token.serial != 0, "initial source task publishes exactly once");
    const auto epoch = source.source_frame_epoch();
    check(epoch != 0 && initial.service_id == epoch, "bootstrap publishes the existing actual service identity as equality epoch");
    cpu_complete(session, initial);
    check(source.source_frame_epoch() == epoch, "task completion cannot manufacture another source audio frame");
    if (!tick_before_build) check(session.retrace(10, 0) == a::Error::none, "first source tick supplies the deferred post-initial-build update");
    check(session.submit() == a::Error::none && sink.accepted == 0 && session.native_ai_frames() == 0 &&
          session.service().has_lease(), "ordinal10 is only the session's first notification, never bootstrap output credit");
    check(source.source_frame_epoch() == epoch, "first scheduler tick and deferred source command update do not advance the build epoch");
    check(session.retrace(15, 16666667) == a::Error::none && session.submit() == a::Error::none &&
          sink.accepted == initial.frame_count && !session.service().has_lease(),
          "second actual notification releases initial PCM independently of external ordinal gaps");
    a::TaskView next;
    check(session.build(next) == a::Error::none && next.token.serial != 0, "next source task consumes first-tick published commands");
    check(source.source_frame_epoch() != epoch && next.service_id == source.source_frame_epoch(),
          "only the next admitted actual build publishes a distinct equality epoch");
    cpu_complete(session, next);
    check(session.retrace(16, 33333334) == a::Error::none && session.submit() == a::Error::none &&
          !session.service().has_lease() && !session.service().voices_retired(),
          "first tick's SFX update reaches an actual bound native voice in the next task, not a silent extra task");
    check(session.stop() == a::Error::none, "bootstrap regression requests genuine source retirement");
    for (std::uint64_t tick = 1; tick <= 12 && session.state() != g::Session::State::draining; ++tick)
        if (!cpu_tick(session, 16 + tick, (2 + tick) * 16666667)) return;
    check(session.state() == g::Session::State::draining && session.service().voices_retired(),
          "bootstrap command owner retires all real native task/voice/PCM users before drain");
    sink.state = g::Output::State::drained;
    check(session.poll_drain() == a::Error::none, "bootstrap regression finishes its checked owned consumer");
}
void reordered_sfx_descriptor_reuse() {
    Consumer sink; sink.limit = 1024;
    // Authored zero release is admitted and retires at the actual stop callback,
    // avoiding a wall-clock tolerance or assumptions about fractional timing.
    auto made = make(sink, 0); if (!made) { check(false, "zero-release ownership fixture initializes"); return; }
    auto& session = *made.session;
    for (unsigned i = 0; i < g::sound_slots; ++i) {
        int slot = -1;
        check(session.play_sound(0, {}, slot) == a::Error::none && slot == static_cast<int>(i),
              "ten independent facade slots reserve genuine native sound starts");
    }
    a::TaskView initial;
    check(session.build(initial) == a::Error::none, "SFX generation regression publishes bootstrap");
    cpu_complete(session, initial);
    std::uint64_t tick = 0;
    auto step = [&]() { ++tick; return cpu_tick(session, tick, tick * 1000000000ull / 60); };
    if (!step() || !step()) return;
    check(session.sound_priority(9, 1) == a::Error::none, "later facade slot becomes the genuine lowest-priority steal candidate");
    int replacement = -1;
    check(session.play_sound(0, {}, replacement) == a::Error::none && replacement == 9 &&
          session.stop_sound(0) == a::Error::none, "steal slot9 while retiring slot0 in the same source command window");
    if (!step() || !step()) return;
    // Slot9's delayed replacement reuses native descriptor0. Slot0's new start
    // consequently takes native descriptor9: facade/native order now differs.
    int earlier = -1;
    check(session.play_sound(0, {}, earlier) == a::Error::none && earlier == 0, "earlier facade slot reenters after out-of-order descriptor retirement");
    if (!step() || !step()) return;
    check(session.stop_sound(0) == a::Error::none, "earlier facade playback retires before the later playback");
    if (!step()) return;
    check(session.play_sound(0, {}, earlier) == a::Error::none && earlier == 0 &&
          session.stop_sound(9) == a::Error::none, "earlier pending start and later native-descriptor retirement share one update");
    if (!step()) return;
    check(session.state() == g::Session::State::ready && session.playing_sound(0) == 0 &&
          session.playing_sound(9) == -1,
          "all old generations retire before an earlier allocation can reuse the later facade slot's descriptor");
    if (!step()) return;
    check(session.stop() == a::Error::none, "reordered SFX graph requests genuine drain");
    for (unsigned i = 0; i < 12 && session.state() != g::Session::State::draining; ++i)
        if (!step()) return;
    check(session.state() == g::Session::State::draining && session.service().voices_retired(),
          "reordered native descriptors, physical voices and PCM owners retire without capacity inflation or stale generations");
    sink.state = g::Output::State::drained;
    check(session.poll_drain() == a::Error::none, "reordered SFX checked consumer completes drain");
}
void absent_sound_and_idle_aging() {
    Consumer sink;
    auto made = make(sink);
    if (!made) { check(false, "absent-sound boundary owner initializes"); return; }
    auto& session = *made.session;
    for (unsigned i = 0; i < g::sound_slots; ++i) {
        int slot = -1;
        check(session.play_sound(0, {}, slot) == a::Error::none && slot == static_cast<int>(i),
              "equal-priority starts reserve each source slot");
    }
    int slot = -1;
    check(session.play_sound(0, {}, slot) == a::Error::none && slot == 0,
          "oldest queued source slot is replaced before aging saturation");
    // Every ordinary absent request still ages slots. Saturation turns the
    // newer slot0 and older slot1 into a tie, resolved in source slot order.
    bool ordinary = true;
    for (unsigned i = 0; i < 255; ++i) {
        slot = 99;
        ordinary &= session.play_sound(std::numeric_limits<std::uint32_t>::max(), {}, slot) ==
                    a::Error::none && slot == -1;
    }
    check(ordinary, "absent sounds return ordinary -1 without reporting a service error");
    check(session.play_sound(0, {}, slot) == a::Error::none && slot == 0,
          "absent requests retain saturating idle aging and its observable steal precedence");
    auto malformed = g::Parameters{};
    malformed.pitch = 0.0f;
    check(session.play_sound(0, malformed, slot) == a::Error::invalid_parameter && slot == -1,
          "ordinary absence does not suppress malformed-parameter failures");
    check(session.stop_sounds() == a::Error::none, "queued boundary requests cancel without a descriptor");
}

enum class PendingControl { none, volume, pan, pitch, reverb };

std::vector<std::vector<std::uint8_t>> pending_sound_commands(PendingControl control) {
    Consumer sink; sink.limit = 1024;
    auto made = make(sink);
    if (!made) { check(false, "pending-volume producer initializes"); return {}; }
    auto& session = *made.session;
    int slot = -1;
    check(session.play_sound(0, {0x3000, 64, 0, 1.0f}, slot) == a::Error::none && slot >= 0,
          "explicit-volume sound queues before descriptor binding");
    a::Error result = a::Error::none;
    switch (control) {
        case PendingControl::none: break;
        case PendingControl::volume: result = session.sound_volume(static_cast<unsigned>(slot), 0); break;
        case PendingControl::pan: result = session.sound_pan(static_cast<unsigned>(slot), 0); break;
        case PendingControl::pitch: result = session.sound_pitch(static_cast<unsigned>(slot), 0.5f); break;
        case PendingControl::reverb: result = session.sound_reverb(static_cast<unsigned>(slot), 100); break;
    }
    check(result == a::Error::none, "pre-descriptor control remains an ordinary ignored request");
    a::TaskView initial;
    check(session.build(initial) == a::Error::none, "pending-volume producer publishes bootstrap");
    cpu_complete(session, initial);
    std::vector<std::vector<std::uint8_t>> commands;
    commands.reserve(4);
    std::uint64_t tick = 0;
    for (; tick < 4;) {
        ++tick;
        check(session.retrace(tick, tick * 1000000000ull / 60) == a::Error::none &&
              session.submit() == a::Error::none, "pending-volume source clock advances");
        a::TaskView task;
        check(session.build(task) == a::Error::none, "pending-volume source command production succeeds");
        if (task.token.serial == 0) continue;
        auto& bytes = commands.emplace_back(task.command_bytes);
        const auto* ram = reinterpret_cast<const std::uint8_t*>(session.service().rdram_words().data());
        for (std::uint32_t i = 0; i < task.command_bytes; ++i)
            bytes[i] = ram[(task.command_address + i) ^ 3u];
        cpu_complete(session, task);
        check(session.submit() == a::Error::none, "pending-volume producer retains checked PCM ownership");
    }
    check(session.stop() == a::Error::none, "pending-volume source requests retirement");
    for (unsigned i = 0; i < 20 && session.state() != g::Session::State::draining; ++i) {
        ++tick;
        if (!cpu_tick(session, tick, tick * 1000000000ull / 60)) return commands;
    }
    check(session.state() == g::Session::State::draining && session.service().voices_retired(),
          "pending-volume producer retires all command, voice and PCM owners");
    sink.state = g::Output::State::drained;
    check(session.poll_drain() == a::Error::none, "pending-volume checked consumer finishes");
    return commands;
}

void unbound_controls_preserve_pending_sound() {
    const auto baseline = pending_sound_commands(PendingControl::none);
    check(baseline == pending_sound_commands(PendingControl::volume),
          "unbound volume cannot alter pending producer commands");
    check(baseline == pending_sound_commands(PendingControl::pan),
          "unbound pan cannot alter pending producer commands");
    check(baseline == pending_sound_commands(PendingControl::pitch),
          "unbound pitch cannot alter pending producer commands");
    check(baseline == pending_sound_commands(PendingControl::reverb),
          "unbound reverb cannot alter pending producer commands");
}

void unbound_priority_preserves_steal_order() {
    Consumer sink;
    auto made = make(sink);
    if (!made) { check(false, "unbound-priority owner initializes"); return; }
    auto& session = *made.session;
    int slot = -1;
    for (unsigned i = 0; i < g::sound_slots; ++i)
        check(session.play_sound(0, {}, slot) == a::Error::none && slot == static_cast<int>(i),
              "priority fixture queues equal-priority sounds");
    check(session.sound_priority(g::sound_slots - 1, 1) == a::Error::none,
          "unbound priority request is ordinary");
    check(session.play_sound(0, {}, slot) == a::Error::none && slot == 0,
          "unbound priority cannot redirect stealing from the oldest equal-priority sound");
    check(session.stop_sounds() == a::Error::none, "unbound-priority sounds cancel cleanly");
}

void readiness_and_failure() {
    Consumer sink;
    sink.rate = 32000;
    check(!make(sink), "wrong checked source format cannot announce initialization ready");
    sink.rate = g::source_rate; sink.state = g::Output::State::failed;
    check(!make(sink), "failed output cannot announce initialization ready");
    sink.state = g::Output::State::ready; sink.fail_initialize = true;
    check(!make(sink) && sink.retirements == 1 && sink.borrowed == nullptr,
          "partially initialized executor failure unwinds borrower before service storage");
    sink.fail_initialize = false;
    sink.state = g::Output::State::ready;
    auto made = make(sink); check(bool(made), "real resources and all three calendars initialize");
    if (!made) return;
    auto& session = *made.session;
    a::TaskView task;
    check(session.build(task) == a::Error::none && task.frame_count == 640, "source bootstrap quota derives from 32006/60, sixteen alignment and eighty extra samples");
    const auto failed_epoch = session.source_frame_epoch();
    auto foreign = task.token; ++foreign.owner;
    check(session.complete(foreign, true) == a::Error::stale_task && session.service().has_lease(), "foreign completion cannot retire real task");
    check(session.complete(task.token, false) == a::Error::execution_failed &&
        session.state() == g::Session::State::failed && !session.initialization_ready(), "actual execution failure is terminal and revokes readiness");
    check(session.build(task) == a::Error::execution_failed, "failure wakes owner through retained concrete error, never fake completion");
    check(session.source_frame_epoch() == failed_epoch, "stale/failed completion and failed-state build leave the sampled epoch intact");
}
void partial_output_and_clock() {
    Consumer sink;
    auto made = make(sink); if (!made) { check(false, "partial owner initializes"); return; }
    auto& s = *made.session;
    a::TaskView first, absent;
    const g::Session& source = s;
    check(s.build(first) == a::Error::none, "first task builds once");
    const auto first_epoch = source.source_frame_epoch();
    check(first_epoch != 0 && first.service_id == first_epoch, "actual bootstrap build publishes its own source epoch");
    check(s.build(absent) == a::Error::none && absent.token.serial == 0 &&
          source.source_frame_epoch() == first_epoch, "executing-task no-work build retains the sampled source epoch");
    cpu_complete(s, first, true);
    check(source.source_frame_epoch() == first_epoch, "PCM completion and copied sentinels leave source epoch unchanged");
    const auto* stable = s.service().rdram_words().data();
    check(s.submit() == a::Error::none && s.service().has_lease() && s.accepted_frames() == 0, "blocked checked consumer retains complete PCM owner");
    check(s.retrace(1, 0) == a::Error::none && s.submit() == a::Error::none &&
        s.accepted_frames() == 0 && s.native_ai_frames() == 0, "first source tick submits no bootstrap PCM or fake AI credit");
    check(s.retrace(2, 16666667) == a::Error::none && s.retrace(3, 33333334) == a::Error::none,
          "scheduler mailbox coalesces independently of blocked output and presentation");
    check(source.source_frame_epoch() == first_epoch, "coalesced retraces and elapsed monotonic time alone are not source builds");
    check(s.build(absent) == a::Error::none && absent.token.serial == 0, "no task can recycle blocked PCM RAM");
    sink.limit = 7;
    check(s.submit() == a::Error::none && s.accepted_frames() == 7 && s.service().has_lease(), "partial prefix acceptance retains remainder and lease");
    sink.limit = 1024;
    check(s.submit() == a::Error::none && !s.service().has_lease() && s.accepted_frames() == first.frame_count,
          "only complete prefix ownership transfer releases PCM");
    check(source.source_frame_epoch() == first_epoch, "blocked, partial and complete host PCM acceptance never sample a new source epoch");
    bool ordered = true;
    for (std::uint32_t i = 0; i < first.frame_count * 2; ++i) ordered &= sink.storage[i] == static_cast<std::int16_t>(i + 1);
    check(ordered, "partial consumer sees every stereo sample once in order, never replayed prefix");
    a::TaskView next;
    check(s.build(next) == a::Error::none && next.token.serial == first.token.serial + 1 && next.frame_count == 640,
          "three blocked retraces become one service, with original lagged zero occupancy");
    const auto next_epoch = source.source_frame_epoch();
    check(next_epoch != first_epoch && next.service_id == next_epoch, "coalesced ticks publish one epoch at the real build, not one per retrace");
    cpu_complete(s, next);
    check(s.submit() == a::Error::none && s.service().has_lease(), "next completed PCM remains source-tick gated, never submitted early");
    check(s.native_ai_frames() == 640, "AI query is current stereo-frame remainder, not summed SDL queue bytes");
    // The source clock is free-running even while the AI queue is empty.
    // Earlier empty intervals retain their fraction: total floor deltas here
    // are533,533,534, not three independently rounded533-frame intervals.
    constexpr std::uint64_t frames_at_third_tick = 33333334ull * g::source_rate / 1000000000ull;
    constexpr std::uint64_t frames_at_fourth_tick = 50000001ull * g::source_rate / 1000000000ull;
    constexpr auto remainder = 640u - (frames_at_fourth_tick - frames_at_third_tick);
    static_assert(remainder == 106);
    check(s.retrace(4, 50000001) == a::Error::none && s.native_ai_frames() == remainder,
          "free-running monotonic source clock carries empty-bootstrap fractions into its actual534-frame interval");
    check(s.submit() == a::Error::none && !s.service().has_lease(), "next scheduler tick transfers the gated PCM without reexecuting its task");
    check(source.source_frame_epoch() == next_epoch, "AI consumption and a completed PCM release cannot advance the actual-build epoch");
    check(s.service().rdram_words().data() == stable, "all partial-output transitions preserve borrowed RDRAM allocation");
    a::TaskView final;
    check(s.build(final) == a::Error::none, "post-clock task builds"); cpu_complete(s, final);
    sink.fail_submit = true;
    check(s.retrace(5, 66666668) == a::Error::none, "failing output source gate opens independently");
    check(s.submit() == a::Error::execution_failed && s.state() == g::Session::State::failed && s.service().has_lease(),
          "output failure retains unaccepted PCM and never fabricates drain/reset");
    check(s.abandon_pcm() == a::Error::none && !s.service().has_lease() &&
          s.state() == g::Session::State::failed && sink.finishes == 0,
          "failed output can explicitly discard its completed owner without deadlock or fake drain");
}
void stop_restart_and_drain() {
    Consumer sink; sink.limit = 1024;
    auto made = make(sink); if (!made) { check(false, "stop owner initializes"); return; }
    auto& s = *made.session;
    const auto* stable = s.service().rdram_words().data();
    int slot = -1;
    check(s.play_song(0, 0) == a::Error::none && s.play_song(1, 0) == a::Error::none &&
          s.play_sound(0, {}, slot) == a::Error::none && slot == 0, "two source song requests and genuine indexed SFX queue independently");
    check(s.bgm_volume(0, 32512) == a::Error::none && s.bgm_volume(1, 4096) == a::Error::none,
          "BGM contexts carry independent configured volume controls");
    for (std::uint64_t tick = 1; tick <= 4; ++tick) {
        check(s.retrace(tick, tick * 16666667) == a::Error::none, "startup source retrace");
        check(s.submit() == a::Error::none, "preceding source-buffer output gate transfers its retained PCM");
        a::TaskView task; check(s.build(task) == a::Error::none && task.token.serial != 0, "startup source state advances on service");
        cpu_complete(s, task); check(s.submit() == a::Error::none, "startup PCM ownership transfers");
    }
    check(s.bgm_playing(0) && s.bgm_playing(1), "both source request states reach playing, not one substituted song");
    const auto before_restart = s.source_frame_epoch();
    check(s.restart() == a::Error::none, "restart requests real client stop rather than synchronous heap clear");
    check(!s.initialization_ready(), "restart cannot announce ready while actual source release is pending");
    check(s.source_frame_epoch() == before_restart, "player/effects restart request neither resets nor advances the shared source epoch");
    unsigned retirement_tasks = 0;
    bool reset = false;
    for (std::uint64_t tick = 5; tick <= 40; ++tick) {
        check(s.retrace(tick, tick * 16666667) == a::Error::none, "release source retrace");
        check(s.submit() == a::Error::none, "release source-buffer acceptance progresses independently");
        const auto before_build = s.source_frame_epoch();
        a::TaskView task; check(s.build(task) == a::Error::none, "release/retirement task builds");
        check((task.token.serial != 0) == (s.source_frame_epoch() != before_build),
              "restart retirement changes the equality epoch exactly when a real task is built");
        if (task.token.serial != 0) {
            ++retirement_tasks;
            check(task.service_id == s.source_frame_epoch(), "recreated clients retain the same monotonic service identity domain");
            cpu_complete(s, task);
            check(s.submit() == a::Error::none, "release PCM transfers");
        }
        if (s.initialization_ready() && !s.bgm_playing(0) && s.playing_sound(0) < 0) { reset = true; break; }
    }
    check(reset && retirement_tasks >= 2 && s.service().rdram_words().data() == stable,
          "restart waits actual multi-task CSP stop release/physical retirement without repeatedly retargeting it");
    check(s.stop() == a::Error::none, "idle stop requests drain");
    for (std::uint64_t tick = 41; tick < 50 && s.state() != g::Session::State::draining; ++tick) {
        check(s.retrace(tick, tick * 16666667ull) == a::Error::none, "stop scheduler notification");
        check(s.submit() == a::Error::none, "stop keeps required final PCM acceptance alive");
        a::TaskView task; check(s.build(task) == a::Error::none, "stop retirement service progresses");
        if (task.token.serial != 0) cpu_complete(s, task);
    }
    check(s.state() == g::Session::State::draining && sink.finishes == 1 && s.service().voices_retired(),
          "retired source clients flush checked output exactly once");
    const auto drained_epoch = s.source_frame_epoch();
    a::TaskView absent;
    check(s.build(absent) == a::Error::none && absent.token.serial == 0 && s.source_frame_epoch() == drained_epoch,
          "draining no-work build neither creates a task nor advances the source epoch");
    check(s.poll_drain() == a::Error::none && s.state() == g::Session::State::draining, "pending actual output drain is not completion");
    sink.state = g::Output::State::drained;
    check(s.poll_drain() == a::Error::none && s.state() == g::Session::State::drained, "only checked downstream drain changes terminal state");
    check(s.source_frame_epoch() == drained_epoch, "checked downstream drain polling is not another source audio build");
}
bool flute_step(g::Session& session, std::uint64_t& tick) {
    ++tick;
    return cpu_tick(session, tick, tick * 1000000000ull / 60);
}
void flute_finish(g::Session& session, Consumer& sink, std::uint64_t& tick) {
    check(session.stop() == a::Error::none, "flute API fixture stops the real shared song/voice graph");
    for (unsigned i = 0; i < 16 && session.state() != g::Session::State::draining; ++i)
        if (!flute_step(session, tick)) return;
    check(session.state() == g::Session::State::draining && session.service().voices_retired() &&
          !session.service().has_lease(), "flute fixture retires actual native clients, physical voices and PCM owners");
    sink.state = g::Output::State::drained;
    check(session.poll_drain() == a::Error::none, "flute fixture finishes its checked owned CPU consumer");
}
void flute_cycle_and_reconstruction() {
    Consumer sink; sink.limit = 1024;
    auto made = make_flute(sink); if (!made) { check(false, "flute cycle fixture initializes"); return; }
    auto& session = *made.session;
    items::FluteState retained;
    items::Flute flute(session, retained);
    check(flute.command() == 0 && flute.state() == -1 && retained.song_index == 0 && !retained.playing,
          "initial source flag/index and equal zero timestamp are preserved");
    check(session.play_song(1, 0) == a::Error::none && flute.play() == a::Error::none,
          "flute main0 request coexists with an independent secondary music request");
    check(flute.command() == 5 && flute.state() == -1 && retained.playing && retained.song_index == 0 &&
          session.bgm_playing(0) && session.bgm_playing(1),
          "first play requests song1 without advancing index and reports pending BGM status, not audible readiness");
    const auto requested_epoch = session.source_frame_epoch();
    for (unsigned i = 0; i < 32; ++i)
        check(flute.state() == -1 && flute.command() == 5, "same-epoch gameplay polling cannot clear flute uncertainty");
    check(session.source_frame_epoch() == requested_epoch && retained.song_start_epoch == requested_epoch,
          "play/command/state calls neither invent a clock nor rewrite the captured source epoch");
    a::TaskView bootstrap;
    check(session.build(bootstrap) == a::Error::none && bootstrap.token.serial != 0 && flute.state() == 1,
          "actual bootstrap build changes equality epoch even before the requested sequence has bound");
    cpu_complete(session, bootstrap);
    const auto built_epoch = session.source_frame_epoch();
    a::TaskView absent;
    check(session.build(absent) == a::Error::none && absent.token.serial == 0 && flute.state() == 1 &&
          session.source_frame_epoch() == built_epoch, "no-work build cannot resample the flute timestamp");
    std::uint64_t tick = 0;
    if (!flute_step(session, tick)) return;
    const auto start_epoch = retained.song_start_epoch;
    {
        items::Flute reconstructed(session, retained);
        check(reconstructed.command() == 5 && reconstructed.state() == 1 && retained.song_index == 0 &&
              retained.playing && retained.song_start_epoch == start_epoch,
              "wrapper reconstruction preserves the caller's actual source lifetime instead of reinitializing Item globals");
    }
    constexpr std::array<std::int32_t, 3> indices{1, 2, 0};
    constexpr std::array<std::int32_t, 3> commands{6, 7, 5};
    for (std::size_t i = 0; i < indices.size(); ++i) {
        items::Flute reconstructed(session, retained);
        const auto epoch = session.source_frame_epoch();
        check(reconstructed.play() == a::Error::none && retained.song_index == indices[i] &&
              reconstructed.command() == commands[i] && reconstructed.state() == -1 &&
              retained.song_start_epoch == epoch && session.source_frame_epoch() == epoch,
              "already-playing source flag cycles 1/3/2 with commands5/6/7 and wraps without sampling another epoch");
        if (!flute_step(session, tick)) return;
        check(reconstructed.state() == 1 && session.bgm_playing(1),
              "source command cycling leaves the independent secondary BGM request unchanged");
    }
    check(flute.stop() == a::Error::none && flute.command() == 0 && flute.state() == -1 &&
          !retained.playing && retained.song_index == 0 && !session.bgm_playing(0) && session.bgm_playing(1),
          "source stop clears only flute main0, retains the current index and timestamps the request");
    if (!flute_step(session, tick)) return;
    check(flute.state() == 0, "next build reports real stopped request status, not lingering release voices or flute flag");
    {
        items::Flute reconstructed(session, retained);
        check(reconstructed.play() == a::Error::none && retained.song_index == 0 &&
              reconstructed.command() == 5 && reconstructed.state() == -1,
              "stop then reconstructed play restarts the current song without cycling");
        for (unsigned i = 0; i < 8; ++i) if (!flute_step(session, tick)) return;
        check(reconstructed.state() == 1,
              "retained-index restart reports the real main0 BGM request after source builds");
    }
    check(flute.stop() == a::Error::none, "retained wrapper remains bound to the same source state");
    flute_finish(session, sink, tick);
}
void flute_natural_completion_and_stopped_timestamp() {
    Consumer sink; sink.limit = 1024;
    auto made = make_flute(sink, true); if (!made) { check(false, "finite flute fixture initializes"); return; }
    auto& session = *made.session;
    items::FluteState retained;
    items::Flute flute(session, retained);
    check(flute.play() == a::Error::none, "finite source song1 starts");
    std::uint64_t tick = 0;
    for (unsigned song : {1u, 3u}) {
        bool bound = false;
        const auto start = retained.song_start_epoch;
        for (unsigned i = 0; i < 32 && session.bgm_playing(0); ++i) {
            if (!flute_step(session, tick)) return;
            bound |= !session.service().voices_retired();
        }
        check(bound && !session.bgm_playing(0) && flute.state() == 0 && retained.playing &&
              flute.command() == (song == 1 ? 5 : 6) && retained.song_start_epoch == start,
              "real compact sequence termination changes BGM state without rewriting source play flag, command or start epoch");
        if (song == 1) {
            items::Flute reconstructed(session, retained);
            check(reconstructed.command() == 5 && reconstructed.state() == 0 &&
                  reconstructed.play() == a::Error::none && retained.song_index == 1 &&
                  reconstructed.command() == 6 && reconstructed.state() == -1,
                  "naturally completed audio is still source-playing, so reconstructed play advances to song3");
        }
    }
    check(flute.stop() == a::Error::none && !retained.playing && flute.command() == 0 && flute.state() == -1,
          "source stop timestamps even after the native sequence has already completed");
    if (!flute_step(session, tick)) return;
    check(flute.state() == 0, "a subsequent real audio build releases stopped-source uncertainty");
    const auto before_stop = retained.song_start_epoch;
    const auto current = session.source_frame_epoch();
    check(flute.stop() == a::Error::none && retained.song_start_epoch == current &&
          retained.song_start_epoch != before_stop && flute.state() == -1 && !retained.playing,
          "already-stopped stop captures the new current epoch rather than becoming an untimestamped no-op");
    check(session.play_song(0, 0) == a::Error::none && flute.stop() == a::Error::none &&
          session.bgm_playing(0) && flute.command() == 0 && flute.state() == -1,
          "already-stopped flute never stops unrelated main0 music, but still timestamps every stop");
    if (!flute_step(session, tick)) return;
    check(flute.state() == 1 && !retained.playing && flute.command() == 0,
          "state after a new epoch is real main0 BGM status, independent of the source flute-playing flag");
    if (!flute_step(session, tick)) return;
    flute_finish(session, sink, tick);
}
void flute_checked_audio_errors() {
    Consumer missing_sink; missing_sink.limit = 1024;
    auto missing = make(missing_sink);
    if (!missing) { check(false, "missing-song admission fixture initializes"); return; }
    items::FluteState missing_state;
    items::Flute missing_flute(*missing.session, missing_state);
    check(missing_flute.play() == a::Error::invalid_parameter && !missing_state.playing &&
          missing_state.song_index == 0 && missing_state.song_start_epoch == 0 && missing_flute.command() == 0,
          "missing real song1 reports admission error instead of fabricating successful flute playback");
    Consumer failed_sink; failed_sink.limit = 1024;
    auto failed = make_flute(failed_sink);
    if (!failed) { check(false, "failed-audio flute fixture initializes"); return; }
    auto& session = *failed.session;
    items::FluteState retained;
    items::Flute flute(session, retained);
    check(flute.play() == a::Error::none, "flute error fixture first admits a real source request");
    const auto start = retained.song_start_epoch;
    a::TaskView task; check(session.build(task) == a::Error::none, "flute error fixture owns a real audio task");
    const auto epoch = session.source_frame_epoch();
    check(session.complete(task.token, false) == a::Error::execution_failed &&
          flute.stop() == a::Error::invalid_state && retained.playing && retained.song_start_epoch == start,
          "failed real audio stop propagates its error before publishing source stop success");
    check(flute.play() == a::Error::invalid_state && retained.song_index == 1 &&
          retained.song_start_epoch == start && session.source_frame_epoch() == epoch,
          "failed play preserves source's index-before-stop prefix without inventing a timestamp or successful audio request");
    std::uint64_t tick = 0;
    flute_finish(*missing.session, missing_sink, tick);
}
void source_epoch_admission_and_equal_timestamps() {
    for (bool executor_failure : {false, true}) {
        Consumer sink;
        auto made = make(sink);
        if (!made) { check(false, "source admission fixture initializes before endpoint failure"); return; }
        if (executor_failure) sink.fail_initialize = true;
        else sink.state = g::Output::State::failed;
        a::TaskView task;
        check(made.session->build(task) == a::Error::execution_failed && task.token.serial == 0 &&
              made.session->source_frame_epoch() == 0,
              "lost actual executor/output admission fails before publishing any source-frame epoch");
    }
    Consumer sink; sink.limit = 1024;
    auto made = make(sink); if (!made) { check(false, "equal timestamp source fixture initializes"); return; }
    auto& session = *made.session;
    a::TaskView initial;
    check(session.build(initial) == a::Error::none && initial.token.serial != 0, "equal timestamp fixture publishes actual bootstrap");
    const auto initial_epoch = session.source_frame_epoch();
    cpu_complete(session, initial);
    constexpr std::uint64_t timestamp = 9000;
    check(session.retrace(7, timestamp) == a::Error::none && session.submit() == a::Error::none &&
          session.service().has_lease() && session.retrace(13, timestamp) == a::Error::none &&
          session.submit() == a::Error::none && !session.service().has_lease() &&
          session.source_frame_epoch() == initial_epoch,
          "equal monotonic timestamps with distinct real retraces release gated PCM without sampling another source epoch");
    a::TaskView next;
    check(session.build(next) == a::Error::none && next.token.serial != 0 &&
          session.source_frame_epoch() != initial_epoch && next.service_id == session.source_frame_epoch(),
          "a distinct admitted build has a distinct equality epoch even at the same host timestamp; no OSTime value is claimed");
    cpu_complete(session, next);
    const auto next_epoch = session.source_frame_epoch();
    check(session.stop() == a::Error::none && session.source_frame_epoch() == next_epoch,
          "stop requests do not create audio frame time");
    std::uint64_t ordinal = 13, ns = timestamp;
    for (unsigned i = 0; i < 16 && session.state() != g::Session::State::draining; ++i) {
        ns += 16666667;
        if (!cpu_tick(session, ++ordinal, ns)) return;
    }
    check(session.state() == g::Session::State::draining && !session.service().has_lease(),
          "equal-timestamp fixture subsequently retires through actual source builds and owned PCM acceptance");
    sink.state = g::Output::State::drained;
    check(session.poll_drain() == a::Error::none, "equal-timestamp checked source consumer completes drain");
}
void effects_reset_atomicity() {
    f::Fixture fixture; auto bank = fixture.decode(); const std::array owners{bank.asset};
    auto made = a::AudioService::create(owners, {32006}, f::coefficients(), {a::effects::Routing::direct});
    if (!made) { check(false, "effects reset owner creates"); return; }
    auto& s = *made.service;
    const auto* stable = s.rdram_words().data();
    a::PlayerId player;
    check(s.add_sound_player({}, player) == a::Error::none, "pre-reset registration exists");
    const auto before = std::vector<std::uint32_t>(s.rdram_words().begin(), s.rdram_words().end());
    const std::array invalid{a::effects::Section{0, 160, 0, 0, 0, {}, a::effects::Chorus{std::numeric_limits<std::uint32_t>::max(), 0}}};
    check(s.reset({a::effects::Routing::network, 512, invalid}) == a::Error::invalid_parameter &&
          std::equal(before.begin(), before.end(), s.rdram_words().begin()), "rate-specific chorus rejection is atomic across entire stable RAM");
    a::SoundId id; s.sound_id(f::selected(bank.asset), id); a::sfx::SoundHandle sound;
    check(s.player(player).stage_start(id, {}, sound) == a::Error::none, "invalid reset preserved live player capability");
    a::TaskView task; check(s.build({1, 0, 16}, task) == a::Error::none, "pre-reset task builds");
    const std::array valid{a::effects::Section{0, 160, 8192, -4096, 16384}};
    check(s.reset({a::effects::Routing::network, 512, valid}) == a::Error::task_busy, "executing lease fences even valid recreation");
    a::PcmView pcm; s.complete(task.token, true, pcm);
    check(s.reset({a::effects::Routing::network, 512, valid}) == a::Error::task_busy, "completed PCM lease still fences recreation");
    s.release(pcm.token);
    check(s.reset({a::effects::Routing::network, 512, valid}) == a::Error::none && s.rdram_words().data() == stable &&
          s.player(player).stage_stop(sound) == a::Error::stale_handle, "valid effects recreation resets registrations without changing stable RAM");
    a::TaskView next; check(s.build({1, 0, 160}, next) == a::Error::none && next.token.serial > task.token.serial &&
        s.release(task.token) == a::Error::stale_task, "effects recreation preserves task identity monotonicity");
    s.complete(next.token, true, pcm); s.release(pcm.token);
    check(s.voices_retired(), "recreated empty graph has no pending physical owner after real lease retirement");
}
void live_priority_steal_retirement() {
    f::Fixture fixture; auto bank = fixture.decode(); const std::array owners{bank.asset};
    auto made = a::AudioService::create(owners, {32006, 1, 42}, f::coefficients(), {a::effects::Routing::direct});
    if (!made) { check(false, "priority owner creates"); return; }
    auto& s = *made.service; a::PlayerId id; a::sfx::Config config; config.max_sounds = 3;
    check(s.add_sound_player(config, id) == a::Error::none, "priority player registers");
    auto player = s.player(id); a::SoundId sound; s.sound_id(f::selected(bank.asset), sound);
    a::sfx::SoundHandle first, second, third;
    player.stage_start(sound, {}, first); player.set_priority(first, 100); player.stage();
    a::TaskView task; a::PcmView pcm;
    s.build({1, 0, 16}, task); s.complete(task.token, true, pcm); s.release(pcm.token);
    check(player.set_priority(first, 1) == a::Error::none, "live priority lowers actual native allocation owner");
    player.stage_start(sound, {}, second); player.set_priority(second, 50); player.stage();
    s.build({2, 0, 640}, task); s.complete(task.token, true, pcm); s.release(pcm.token);
    a::sfx::Status one, two;
    player.status(first, one); player.status(second, two);
    check(!one.bound && two.bound && !s.voices_retired(), "lowered live priority enables genuine steal and new physical binding");
    player.stage_stop(first); player.stage_stop(second); player.stage();
    for (std::uint64_t serial = 3; serial < 12 && !s.voices_retired(); ++serial) {
        s.build({serial, 0, 640}, task); s.complete(task.token, true, pcm);
        check(!s.voices_retired(), "completed PCM lease remains a real retirement fence"); s.release(pcm.token);
    }
    check(s.voices_retired(), "all stolen/releasing DSP owners and pending events retire through actual service progression");
    check(player.set_priority(first, 127) == a::Error::stale_handle, "retired sound priority cannot mutate replacement playback");
    check(player.stage_start(sound, {}, third) == a::Error::none && third.generation != first.generation,
          "reused sound slot receives independent playback identity");
}
}
int main() {
    readiness_and_failure(); partial_output_and_clock(); stop_restart_and_drain();
    local_bootstrap_and_first_update(false); local_bootstrap_and_first_update(true);
    reordered_sfx_descriptor_reuse();
    absent_sound_and_idle_aging(); unbound_controls_preserve_pending_sound();
    unbound_priority_preserves_steal_order();
    flute_cycle_and_reconstruction(); flute_natural_completion_and_stopped_timestamp(); flute_checked_audio_errors();
    source_epoch_admission_and_equal_timestamps();
    effects_reset_atomicity(); live_priority_steal_retirement();
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
