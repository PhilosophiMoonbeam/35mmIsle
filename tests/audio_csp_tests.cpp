#include "35mmisle/audio_csp.hpp"
#include "audio_fixture.hpp"
#include "../src/audio_synth_internal.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <vector>

namespace a = isle::audio;
namespace csp = a::csp;
namespace d = a::detail;
namespace f = isle::audio_test;
namespace {
int failures = 0;
void check(bool ok, std::string_view message) {
    if (!ok) { ++failures; std::cerr << "FAIL: " << message << '\n'; }
}
// Reference tables record public perceptual volume, not linear DSP amplitude.
constexpr std::uint16_t linear_public_volume(std::uint16_t value) {
    return static_cast<std::uint16_t>((std::uint32_t{value} * value) >> 15);
}
a::SequenceFile sequence_bytes(std::span<const unsigned> track, std::uint32_t division = 48) {
    const auto payload = 68 + track.size();
    auto bytes = std::make_shared<a::Bytes>(12 + payload + (payload & 1u));
    (*bytes)[0] = std::byte{'S'}; (*bytes)[1] = std::byte{'1'};
    f::put16(*bytes, 2, 1); f::put32(*bytes, 4, 12);
    f::put32(*bytes, 8, static_cast<std::uint32_t>(68 + track.size()));
    f::put32(*bytes, 12, 68); f::put32(*bytes, 12 + 64, division);
    std::size_t at = 12 + 68;
    for (auto byte : track) (*bytes)[at++] = std::byte(byte);
    return a::hydrate_sequences(bytes).asset;
}
a::SequenceFile sequence(std::initializer_list<unsigned> track, std::uint32_t division = 48) {
    return sequence_bytes({track.begin(), track.size()}, division);
}
a::ServiceResult service(const a::BankFile& bank, std::uint16_t logical = 42) {
    const std::array owners{bank};
    return a::AudioService::create(owners, {32006, 22, logical}, f::coefficients(), {a::effects::Routing::direct});
}
// CPU scheduling tests only: acknowledge lease ownership without claiming RSP
// execution, meaningful PCM, original fidelity, or effect-network qualification.
void release(a::AudioService& audio, const a::TaskView& task) {
    a::PcmView pcm{};
    check(audio.complete(task.token, true, pcm) == a::Error::none, "matching completion accepts CPU test task");
    check(audio.release(pcm.token) == a::Error::none, "PCM ownership explicitly released");
}
void ownership_and_capacity() {
    f::Fixture fixture;
    const auto bank = fixture.decode();
    const auto other = f::Fixture{}.decode();
    auto made = service(bank.asset);
    check(bool(made), "authored service created"); if (!made) return;
    auto& audio = *made.service;
    auto song = sequence({0, 0xff, 0x2f});
    a::PlayerId rejected{1, 2, 3}, ids[3]{};
    csp::Config config{other.asset, 0, song};
    check(audio.add_sequence_player(config, rejected) == a::Error::foreign_resource && rejected.owner == 0,
          "foreign bank rejects before publishing identity");
    config.bank = bank.asset;
    check(audio.add_sequence_player(config, ids[0]) == a::Error::none &&
          audio.add_sequence_player(config, ids[1]) == a::Error::none,
          "two typed clients reserve exactly sixteen descriptors apiece");
    a::sfx::Config sfx{};
    check(audio.add_sound_player(sfx, ids[2]) == a::Error::none,
          "ten-note SFX plus two sixteen-note CSP clients fit forty-two descriptors");
    check(audio.add_sequence_player(config, rejected) == a::Error::client_capacity && rejected.owner == 0,
          "fourth generic client has no partial publication");
    csp::Status status{};
    check(audio.sequence_player(ids[2]).status(status) == a::Error::stale_handle,
          "SFX identity cannot dispatch through CSP typed storage");
    const auto stale = audio.sequence_player(ids[0]);
    check(audio.reset() == a::Error::none, "quiescent reset invalidates typed resources");
    check(stale.status(status) == a::Error::stale_handle, "old CSP view cannot survive reset");
    check(audio.add_sequence_player(config, rejected) == a::Error::none && rejected.generation > ids[2].generation,
          "reset never recycles external registration identities");
}
void staged_lifecycle_and_lease() {
    f::Fixture fixture;
    const auto bank = fixture.decode();
    auto song = sequence({0, 0xff, 0x51, 0, 0xbb, 0x80,
                          0, 0x90, 60, 100, 100, 100, 0xff, 0x2f});
    auto made = service(bank.asset); if (!made) { check(false, "lifecycle service created"); return; }
    auto& audio = *made.service;
    a::PlayerId id{};
    check(audio.add_sequence_player({bank.asset, 0, song}, id) == a::Error::none, "CSP retains song owner");
    song = {};
    auto player = audio.sequence_player(id);
    check(player.stage_sequence(0) == a::Error::none && player.stage_play() == a::Error::none,
          "bind and play enter unpublished inbox");
    csp::Status status{};
    check(player.status(status) == a::Error::none && !status.bound && status.playback == csp::Playback::stopped,
          "staged commands do not masquerade as processed state");
    check(player.stage() == a::Error::none, "commands publish atomically");
    a::TaskView task{};
    check(audio.build({1, 0, 64}, task) == a::Error::none, "real callback decodes and starts authored note");
    check(player.status(status) == a::Error::none && status.active_notes == 1 && status.bound_notes == 1 &&
          status.playback == csp::Playback::playing && status.tempo == 48000,
          "typed client owns logical note and actual shared physical binding");
    check(player.stage_sequence(0) == a::Error::invalid_state, "playing song cannot be rebound");
    const auto words = audio.rdram_words();
    const auto first = words[task.command_address / 4];
    check(player.stage_tempo(96000) == a::Error::none && player.stage() == a::Error::none,
          "tempo may stage during command execution lease");
    std::uint32_t tempo{};
    check(player.tempo(tempo) == a::Error::none && tempo == 48000 && words[task.command_address / 4] == first,
          "pending tempo changes neither processed getter nor published RAM");
    release(audio, task);
    check(audio.build({2, 0, 1024}, task) == a::Error::none, "next callback consumes predicted-bank control");
    check(player.tempo(tempo) == a::Error::none && tempo == 96000, "processed tempo remains after completion");
    check(player.stage_stop() == a::Error::none && player.stage() == a::Error::none, "stop stages during lease");
    release(audio, task);
    for (std::uint64_t service = 3; service < 8; ++service) {
        check(audio.build({service, 0, 1024}, task) == a::Error::none, "bounded stop release advances");
        release(audio, task);
    }
    check(player.status(status) == a::Error::none && status.playback == csp::Playback::stopped && status.active_notes == 0,
          "explicit stop reaches effective stopped only after finite release retirement");
}
void failure_rolls_back_reader_and_controls() {
    f::Fixture fixture;
    const auto bank = fixture.decode();
    // Complete first note, then a truncated program event. Failure occurs after
    // reader advancement, physical allocation, and internal event publication.
    const auto song = sequence({0, 0x90, 60, 100, 10, 0, 0xc0});
    auto made = service(bank.asset); if (!made) return;
    auto& audio = *made.service;
    a::PlayerId id{};
    check(audio.add_sequence_player({bank.asset, 0, song}, id) == a::Error::none, "rollback client registered");
    auto player = audio.sequence_player(id);
    player.stage_sequence(0); player.stage_play(); player.stage();
    a::TaskView task{};
    check(audio.build({1, 0, 512}, task) == a::Error::invalid_resource && task.token.serial == 0,
          "malformed reader fails entire speculative task");
    csp::Status status{};
    check(player.status(status) == a::Error::none && !status.bound && status.active_notes == 0,
          "reader, bind, note, and shared voice allocation roll back together");
    check(audio.build({1, 0, 512}, task) == a::Error::invalid_resource && task.token.serial == 0,
          "retry consumes same published controls and repeats same failure");
}
void typed_shadow_and_clock_policy() {
    f::Fixture fixture;
    const auto bank = fixture.decode();
    d::Context c;
    c.owner = 11; c.config = {32006, 22, 42}; c.coefficients = f::coefficients();
    c.memory = std::make_unique<std::uint32_t[]>(a::AudioService::rdram_bytes / 4);
    const std::array owners{bank.asset};
    check(d::build_catalog(c, owners) == a::Error::none, "direct semantic context admits bank");
    auto song = sequence({0, 0xff, 0x51, 0, 0xbb, 0x80,
                          0, 0x90, 60, 100, 100, 100, 0xff, 0x2f});
    a::PlayerId id{};
    check(d::add_sequence_player(c, {bank.asset, 0, song}, id) == a::Error::none, "typed pair allocated quiescently");
    const auto index = c.state().clients[id.slot].typed_index;
    auto& live = c.csp_state(c.live, index);
    live.cue = 7; live.next_incarnation = 4;
    c.state().oscillators.initialized = true; c.state().oscillators.free_head = 3;
    auto shadow = d::prepare_shadow(c);
    c.csp_state(shadow, index).cue = 8;
    c.cpu[shadow].oscillators.free_head = 9;
    check(live.cue == 7 && c.state().oscillators.free_head == 3,
          "typed reader state and shared oscillator membership have isolated value banks");
    live.notes[0].incarnation = 6;
    live.notes[0].phase = d::CspPhase::sustain;
    live.events[0] = {true, d::CspEventKind::off, 0, live.cue, 5, 0, 1}; live.event_count = 1;
    shadow = d::prepare_shadow(c);
    check(d::dispatch_sequence_player(c, shadow, id.slot) == a::Error::none &&
          c.csp_state(shadow, index).notes[0].phase == d::CspPhase::sustain,
          "stale note-incarnation duration cannot release a reused descriptor");
}
struct Calendar {
    d::Context context;
    a::PlayerId id{};
    Calendar(const a::BankFile& bank, const a::SequenceFile& song) {
        context.owner = 17; context.config = {32006, 22, 42}; context.coefficients = f::coefficients();
        context.memory = std::make_unique<std::uint32_t[]>(a::AudioService::rdram_bytes / 4);
        const std::array owners{bank};
        check(d::build_catalog(context, owners) == a::Error::none &&
              d::add_sequence_player(context, {bank, 0, song}, id) == a::Error::none,
              "calendar harness retains authored resources and descriptors");
        player().published[0] = {d::CspControlKind::bind, 0, 0};
        player().published[1] = {d::CspControlKind::play};
        // Canonical gain fixtures explicitly set master, never infer the
        // original constructor's effective default from game bookkeeping.
        player().published[2] = {d::CspControlKind::volume, 0, 32512};
        player().published_count = 3;
    }
    d::CspCheckpoint& player() {
        return context.csp_state(context.live, context.state().clients[id.slot].typed_index);
    }
    void step() {
        auto& state = context.state();
        state.sample_clock = state.clients[id.slot].next_raw_due & ~std::uint64_t{15};
        check(d::advance_parameters(context, state, {state.sample_clock}) == a::Error::none &&
              d::dispatch_sequence_player(context, context.live, id.slot) == a::Error::none,
              "one actual typed callback advances bounded calendar");
    }
};
void finite_envelopes_and_retime() {
    f::Fixture fixture;
    f::put32(*fixture.bank, 80, 1001); f::put32(*fixture.bank, 84, 2001);
    f::put32(*fixture.bank, 88, 3001); (*fixture.bank)[93] = std::byte{64};
    const auto bank = fixture.decode();
    const auto song = sequence({0, 0xff, 0x51, 0, 0xbb, 0x80,
                                0, 0x90, 60, 100, 4, 100, 0xff, 0x2f});
    Calendar calendar(bank.asset, song);
    calendar.step();
    check(calendar.player().notes[0].phase == d::CspPhase::attack &&
          calendar.player().client_us == 1001, "finite attack posts microsecond endpoint");
    calendar.step();
    check(calendar.player().notes[0].phase == d::CspPhase::decay &&
          calendar.player().client_us == 3002, "attack endpoint starts independently timed finite decay");
    calendar.step();
    check(calendar.player().notes[0].phase == d::CspPhase::sustain, "finite decay reaches sustain before duration off");
    calendar.step();
    check(calendar.player().notes[0].phase == d::CspPhase::release &&
          calendar.player().client_us == 7001, "duration off uses authored release rather than stop release");
    calendar.step();
    check(calendar.player().notes[0].phase == d::CspPhase::free, "release endpoint stops and frees logical source");

    f::Fixture steady;
    const auto steady_bank = steady.decode();
    const auto long_song = sequence({0, 0xff, 0x51, 0, 0xbb, 0x80,
                                     0, 0x90, 60, 100, 96, 100, 0xff, 0x2f});
    Calendar clock(steady_bank.asset, long_song);
    clock.step();
    const auto source_due = clock.player().sequence_due;
    clock.player().published[0] = {d::CspControlKind::tempo, 0, 96000};
    clock.player().published_count = 1;
    clock.step();
    std::uint64_t off = 0;
    for (const auto& e : clock.player().events)
        if (e.live && e.kind == d::CspEventKind::off) off = e.due;
    check(off == 176000 && clock.player().sequence_due == source_due,
          "API tempo retimes whole remaining duration ticks but leaves posted source deadline");
}
void repeated_keys_and_sustain() {
    f::Fixture fixture;
    const auto bank = fixture.decode();
    const auto song = sequence({0, 0xff, 0x51, 0, 0xbb, 0x80,
                                0, 0xb0, 64, 127,
                                0, 0x90, 60, 100, 24,
                                0, 0x90, 60, 100, 48,
                                72, 0xb0, 64, 0, 72, 0xff, 0x2f});
    Calendar calendar(bank.asset, song);
    calendar.step();
    check(calendar.player().notes[0].incarnation != calendar.player().notes[1].incarnation &&
          calendar.player().notes[0].descriptor.slot != calendar.player().notes[1].descriptor.slot,
          "identical keys retain separate note identities and descriptors");
    for (unsigned steps = 0; calendar.player().client_us < 72000 && steps < 64; ++steps) calendar.step();
    check(calendar.player().client_us == 72000, "bounded sustain driver reaches authored control deadline");
    check(calendar.player().notes[0].sustained && calendar.player().notes[1].sustained,
          "both independent duration offs defer release under sustain");
    calendar.step();
    check(calendar.player().notes[0].phase == d::CspPhase::release &&
          calendar.player().notes[1].phase == d::CspPhase::release,
          "sustain-off releases each deferred note without key-based aliasing");
}
void real_fx_send_and_progress() {
    f::Fixture fixture;
    const auto bank = fixture.decode();
    const auto song = sequence({0, 0x90, 60, 100, 100, 100, 0xff, 0x2f});
    Calendar calendar(bank.asset, song);
    auto& c = calendar.context;
    c.config.physical_voices = 1;
    calendar.step();
    auto& state = c.state();
    const auto descriptor = calendar.player().notes[0].descriptor;
    auto& voice = state.physical[0].current;
    check(d::gain_voice(c, state, descriptor, {8192}, {64}, {0}) == a::Error::none,
          "gain ramp established independently of FX");
    c.write_address = a::AudioService::command_address; c.write_error = a::Error::none;
    check(d::render(c, state, 16, 0, 16) == a::Error::none && voice.gain_elapsed == 16,
          "actual native renderer advances the event-owned gain clock before FX controls");
    auto expected = voice.gain_from;
    for (unsigned control = 0; control < 4; ++control)
        for (std::size_t side = 0; side < 2; ++side)
            expected[side] = std::max<std::uint16_t>(1, d::cpu_gain(expected[side], voice.gain_to[side],
                voice.gain_rate[side], voice.gain_elapsed, voice.gain_frames));
    const a::ParamTime control_time{state.sample_clock};
    check(d::fx_voice(c, state, descriptor, 0, control_time) == a::Error::none &&
          d::fx_voice(c, state, descriptor, 127, control_time) == a::Error::none &&
          d::fx_voice(c, state, descriptor, 64, control_time) == a::Error::none &&
          d::pan_voice(c, state, descriptor, 127, control_time) == a::Error::none &&
          voice.parameters.fx_mix == 64 && voice.gain_frames == 64 && voice.gain_elapsed == 16 &&
          voice.retarget_current == expected,
          "every repeated FX and Pan control reevaluates CPU progress without deduplication or clock reset");
    c.write_address = a::AudioService::command_address; c.write_error = a::Error::none;
    check(d::render(c, state, 16, 0, 16) == a::Error::none, "FX source emits real envelope commands");
    bool aux = false;
    for (auto at = a::AudioService::command_address; at < c.write_address; at += 8) {
        const auto first = c.memory[at / 4];
        if ((first & 0xffff0000u) != 0x09080000u) continue;
        aux = (first & 65535u) == c.coefficients.pan[64] &&
              c.memory[at / 4 + 1] == c.coefficients.pan[63];
    }
    check(aux, "actual AUX command consumes complementary supplied dry/wet coefficients");
    a::VoiceHandle replacement{};
    a::Allocation allocation{};
    const a::ParamTime now{state.sample_clock};
    check(d::create_voice(c, state, replacement) == a::Error::none &&
          d::allocate_voice(c, state, replacement, 255, now, allocation) == a::Error::none,
          "one physical filter can be stolen by independent logical descriptor");
    const auto& map = c.csp[state.clients[calendar.id.slot].typed_index]->resources.sounds.front();
    check(d::start_voice(c, state, replacement, map.wave, {1.0f, {4096}, 64, 1}, {}, now) == a::Error::none &&
          d::fx_voice(c, state, replacement, 127, now) == a::Error::none,
          "FX update posts against deferred replacement source epoch");
    state.sample_clock += c.config.steal.restart_frames;
    check(d::advance_parameters(c, state, {state.sample_clock}) == a::Error::none &&
          state.physical[0].current.parameters.fx_mix == 127,
          "deferred replacement retains its own FX mix at actual source start");
    check(d::fx_voice(c, state, descriptor, 0, {state.sample_clock}) == a::Error::unbound &&
          state.physical[0].current.parameters.fx_mix == 127,
          "old stolen descriptor cannot lend FX controls to replacement");
}
void shared_oscillators_and_local_limit() {
    f::Fixture fixture;
    (*fixture.bank)[36] = std::byte{1}; (*fixture.bank)[37] = std::byte{255};
    (*fixture.bank)[38] = std::byte{63}; (*fixture.bank)[39] = std::byte{1};
    (*fixture.bank)[40] = std::byte{129}; (*fixture.bank)[41] = std::byte{255};
    (*fixture.bank)[42] = std::byte{63}; (*fixture.bank)[43] = std::byte{1};
    const auto bank = fixture.decode();
    std::vector<unsigned> track;
    for (unsigned i = 0; i < 17; ++i) track.insert(track.end(), {0, 0x90, 60, 100, 0});
    track.insert(track.end(), {100, 0xff, 0x2f});
    const auto song = sequence_bytes(track);
    Calendar calendar(bank.asset, song);
    calendar.step();
    check(calendar.player().next_incarnation == 16,
          "local seventeenth simultaneous note drops before reserving another incarnation");
    const auto count = [&]() {
        std::size_t occupied = 0;
        for (const auto& o : calendar.context.state().oscillators.slots) if (o.allocated) ++occupied;
        return occupied;
    };
    check(count() == 32, "first sixteen dual-modulated notes use thirty-two shared oscillator slots");
    auto& c = calendar.context;
    a::PlayerId second{};
    check(d::add_sequence_player(c, {bank.asset, 0, song}, second) == a::Error::none,
          "second typed client shares same oscillator checkpoint");
    auto& second_player = c.csp_state(c.live, c.state().clients[second.slot].typed_index);
    second_player.published[0] = {d::CspControlKind::bind, 0, 0};
    second_player.published[1] = {d::CspControlKind::play}; second_player.published_count = 2;
    check(d::dispatch_sequence_player(c, c.live, second.slot) == a::Error::none && count() == 44,
          "cross-client oscillator exhaustion stops at forty-four without fabricated states");
    calendar.player().published[0] = {d::CspControlKind::stop}; calendar.player().published_count = 1;
    second_player.published[0] = {d::CspControlKind::stop}; second_player.published_count = 1;
    for (unsigned steps = 0; steps < 128; ++steps) {
        auto& state = c.state();
        const auto chosen = state.clients[calendar.id.slot].next_raw_due <= state.clients[second.slot].next_raw_due ?
                            calendar.id.slot : second.slot;
        state.sample_clock = state.clients[chosen].next_raw_due & ~std::uint64_t{15};
        check(d::advance_parameters(c, state, {state.sample_clock}) == a::Error::none &&
              d::dispatch_sequence_player(c, c.live, chosen) == a::Error::none,
              "shared positive-delay modulation and release callbacks remain bounded");
        if (calendar.player().playback == csp::Playback::stopped &&
            second_player.playback == csp::Playback::stopped) break;
    }
    check(count() == 0, "positive-delay oscillator ownership is reclaimed exactly once at note retirement");

    f::Fixture zero;
    (*zero.bank)[36] = std::byte{1}; (*zero.bank)[37] = std::byte{255};
    (*zero.bank)[38] = std::byte{63};
    const auto zero_bank = zero.decode();
    Calendar zero_delay(zero_bank.asset, sequence({0, 0x90, 60, 100, 0, 100, 0xff, 0x2f}));
    zero_delay.step();
    zero_delay.player().published[0] = {d::CspControlKind::stop}; zero_delay.player().published_count = 1;
    for (unsigned i = 0; i < 10 && zero_delay.player().playback != csp::Playback::stopped; ++i) zero_delay.step();
    std::size_t leaked = 0;
    for (const auto& o : zero_delay.context.state().oscillators.slots) if (o.allocated) ++leaked;
    check(leaked == 1, "successful zero-delay init preserves measured bounded occupancy without update or reclaim");
}
void bounded_zero_time_parser_and_inbox() {
    f::Fixture fixture;
    const auto bank = fixture.decode();
    const auto song = sequence({0, 0xff, 0x2e, 0, 0xff,
                                0, 0xc0, 0, 0, 0xff, 0x2d, 255, 255, 0, 0, 0, 12,
                                0, 0xff, 0x2f});
    auto made = service(bank.asset); if (!made) return;
    auto& audio = *made.service;
    a::PlayerId id{};
    check(audio.add_sequence_player({bank.asset, 0, song}, id) == a::Error::none,
          "zero-time authored loop remains a valid reader resource");
    auto player = audio.sequence_player(id);
    player.stage_sequence(0); player.stage_play(); player.stage();
    a::TaskView task{};
    check(audio.build({1, 0, 64}, task) == a::Error::event_capacity && task.token.serial == 0,
          "inner reader drains count loop controls and fail boundedly instead of hanging");
    csp::Status status{};
    check(player.status(status) == a::Error::none && !status.bound && status.active_notes == 0 &&
          audio.build({1, 0, 64}, task) == a::Error::event_capacity,
          "loop-counter mutation and published commands roll back together on parser budget failure");
    check(audio.reset() == a::Error::none &&
          audio.add_sequence_player({bank.asset, 0, song}, id) == a::Error::none,
          "quiescent reset clears failed transaction without recycling registration identity");
    player = audio.sequence_player(id);
    for (unsigned i = 0; i < csp::SequencePlayer::event_capacity; ++i)
        check(player.stage_volume(1234) == a::Error::none, "bounded inbox admits every reserved control slot");
    check(player.stage_volume(5678) == a::Error::event_capacity && player.stage() == a::Error::none,
          "sixty-fifth control rejects without losing first sixty-four");
}
void eight_oscillator_shapes() {
    struct Case { std::uint8_t type; float initial, first; };
    // Public GAME callback outputs at rate255/depth63, not DSP/PCM goldens.
    constexpr float up = 1.003471851348877f, down = 0.996540367603302f;
    const std::array cases{Case{1, 96, 127}, Case{2, 127, 64}, Case{3, 127, 64}, Case{4, 64, 127},
                           Case{128, 1, up}, Case{129, up, down}, Case{130, up, up}, Case{131, down, down}};
    for (const auto item : cases) {
        f::Fixture fixture;
        const bool vibrato = item.type >= 128;
        const auto at = vibrato ? 40u : 36u;
        (*fixture.bank)[at] = std::byte{item.type}; (*fixture.bank)[at + 1] = std::byte{255};
        (*fixture.bank)[at + 2] = std::byte{63}; (*fixture.bank)[at + 3] = std::byte{1};
        const auto bank = fixture.decode();
        Calendar calendar(bank.asset, sequence({0, 0x90, 60, 100, 0, 100, 0xff, 0x2f}));
        calendar.step();
        const auto value = [&]() {
            const auto& n = calendar.player().notes[0];
            return vibrato ? n.vibrato : n.tremolo;
        };
        check(value() == item.initial, "each independent oscillator shape preserves measured initial value");
        calendar.step(); calendar.step();
        check(value() == item.first, "each independent waveform advances on its delayed first callback");
    }
}
void key_velocity_and_wire_channel_resolution() {
    f::Fixture fixture;
    (*fixture.bank)[96] = std::byte{64}; (*fixture.bank)[98] = std::byte{60};
    (*fixture.bank)[99] = std::byte{60};
    const auto bank = fixture.decode();
    // Every encoded event is on track0; status channels, not track slots,
    // select the program/controller state and note-off identity.
    const auto song = sequence({0, 0x95, 59, 100, 0,
                                0, 0x95, 60, 63, 0,
                                0, 0x95, 60, 0, 0,
                                0, 0x95, 60, 64, 0,
                                0, 0xb5, 7, 32,
                                10, 0xff, 0x2f});
    Calendar calendar(bank.asset, song);
    calendar.step();
    const auto& p = calendar.player();
    check(p.next_incarnation == 1 && p.notes[0].key == 60 && p.notes[0].velocity == 64 &&
          p.notes[0].channel == 5 && p.channels[5].controls.volume == 32 &&
          p.channels[0].controls.volume == 100,
          "inclusive key/velocity selection drops misses and zero velocity, preserving wire channel independence");
}
void independent_note_and_channel_gain_factors() {
    struct Case {
        std::uint16_t master;
        std::uint8_t volume, scale, level, velocity;
        std::int8_t extra;
        std::uint16_t expected;
    };
    // Typed public Start arguments from isolated axes and independent heldouts;
    // these exercise arithmetic policy, not synthesized PCM qualification.
    const std::array cases{
        Case{32512, 127, 120, 127, 100, -1, 23257},
        Case{32512, 1, 120, 127, 100, -1, 183},
        Case{32512, 64, 64, 127, 100, -1, 6251},
        Case{32512, 127, 120, 1, 100, -1, 182},
        Case{32512, 127, 120, 127, 100, 127, 23075},
        Case{257, 32, 89, 127, 103, 64, 17},
        Case{24577, 34, 49, 71, 20, 127, 213},
        Case{16383, 22, 3, 29, 105, 127, 11}
    };
    for (const auto row : cases) {
        f::Fixture fixture;
        (*fixture.bank)[32] = std::byte{1};
        (*fixture.bank)[77] = std::byte{1};
        f::put32(*fixture.bank, 80, 1001);
        (*fixture.bank)[92] = std::byte{row.level};
        const auto bank = fixture.decode();
        const std::vector<unsigned> track{0, 0xb0, 7, row.volume,
                                           0, 0xb0, 21, row.scale,
                                           0, 0x90, 60, row.velocity, 0,
                                           100, 0xff, 0x2f};
        Calendar calendar(bank.asset, sequence_bytes(track));
        calendar.player().published[2] = {d::CspControlKind::volume, 0, row.master};
        calendar.player().published[3] = {d::CspControlKind::extra_volume, 0,
                                         static_cast<std::uint8_t>(row.extra)};
        calendar.player().published_count = 4;
        calendar.step();
        check(calendar.context.state().physical[0].current.parameters.gain.value == linear_public_volume(row.expected),
              "separate Q15 note/channel factors preserve signed extra-volume placement and ignore sound volume");
    }
}
void explicit_off_releases_one_oldest_duplicate() {
    for (const auto status : {0x80u, 0x90u}) {
        f::Fixture fixture;
        const auto bank = fixture.decode();
        std::vector<unsigned> track{0, 0xff, 0x51, 0, 0xbb, 0x80,
                                     0, 0x90, 60, 100, 0,
                                     0, 0x90, 60, 100, 0,
                                     48, status, 60, 0};
        if (status == 0x90) track.push_back(0);
        track.insert(track.end(), {48, status, 60, 0});
        if (status == 0x90) track.push_back(0);
        track.insert(track.end(), {104, 0xff, 0x2f});
        Calendar calendar(bank.asset, sequence_bytes(track));
        calendar.step();
        for (unsigned steps = 0; calendar.player().client_us < 48000 && steps < 64; ++steps) calendar.step();
        check(calendar.player().client_us == 48000, "bounded duplicate driver reaches first off deadline");
        calendar.step();
        check(calendar.player().notes[0].phase == d::CspPhase::release &&
              calendar.player().notes[1].phase == d::CspPhase::sustain,
              "one explicit off or velocity-zero event releases only first-started matching duplicate");
        for (unsigned steps = 0; calendar.player().client_us < 96000 && steps < 64; ++steps) calendar.step();
        check(calendar.player().client_us == 96000, "bounded duplicate driver reaches second off deadline");
        calendar.step();
        check(calendar.player().notes[0].phase == d::CspPhase::free &&
              calendar.player().notes[1].phase == d::CspPhase::release,
              "second explicit off reaches remaining duplicate, not retired first incarnation");
    }
}
void raw_note_pitch_and_dsp_saturation() {
    f::Fixture fixture;
    const auto bank = fixture.decode();
    const auto song = sequence({0, 0xff, 0x51, 0, 0xbb, 0x80,
                                0, 0x90, 72, 100, 0,
                                24, 0xe0, 0, 0,
                                24, 0xe0, 127, 127,
                                24, 0xe0, 0, 0,
                                100, 0xff, 0x2f});
    Calendar calendar(bank.asset, song);
    calendar.step();
    auto& c = calendar.context;
    auto& state = c.state();
    const auto& n = calendar.player().notes[0];
    const auto& voice = state.physical[0].current;
    const auto base = d::cents_ratio(1200);
    check(n.base_pitch == base && voice.parameters.pitch == base && voice.pitch == 65534,
          "valid octave-up note retains original raw cents ratio above DSP saturation");
    c.write_address = a::AudioService::command_address; c.write_error = a::Error::none;
    check(d::render(c, state, 16, 0, 16) == a::Error::none, "high-pitch source emits bounded real synthesis commands");
    unsigned resamplers = 0;
    for (auto at = a::AudioService::command_address; at < c.write_address; at += 8) {
        const auto first = c.memory[at / 4];
        if ((first >> 24) != 5) continue;
        ++resamplers;
        check((first & 65535u) == 65534, "actual RESAMPLE operand uses measured saturated word");
    }
    check(resamplers == 1, "one physical source retains one resample stage, not a fabricated multipass path");
    const auto down = base * d::cents_ratio(-200);
    const auto down_word = static_cast<std::uint16_t>(static_cast<double>(down) * 32768.0);
    for (const auto at : {24000u, 48000u, 72000u}) {
        for (unsigned steps = 0; calendar.player().client_us < at && steps < 64; ++steps) calendar.step();
        check(calendar.player().client_us == at, "bounded bend driver reaches authored source event");
        calendar.step();
        const bool high = at == 48000;
        const auto raw = high ? base * d::cents_ratio(199) : down;
        check(n.base_pitch == base && voice.parameters.pitch == raw &&
              voice.pitch == (high ? 65534 : down_word),
              "bend round-trip composes from unchanged raw note base, never previous bent or clamped DSP ratio");
    }
}
void fading_gain_control_targets_its_one_ms_endpoint() {
    f::Fixture fixture;
    (*fixture.bank)[32] = std::byte{127};
    f::put32(*fixture.bank, 84, 10000000); (*fixture.bank)[93] = std::byte{0};
    const auto bank = fixture.decode();
    const auto song = sequence({0, 0xff, 0x51, 7, 0x53, 0,
                                0, 0xb0, 7, 127, 0, 0xb0, 21, 120,
                                0, 0x90, 60, 127, 0,
                                24, 0xb0, 21, 100, 48, 0xff, 0x2f}, 480);
    Calendar calendar(bank.asset, song);
    calendar.step(); calendar.step(); calendar.step();
    const auto& voice = calendar.context.state().physical[0].current;
    check(voice.parameters.gain.value == linear_public_volume(24226) &&
          voice.gain_frames == d::duration_frames(1000, 32006) &&
          calendar.player().client_us == 25000,
          "fading CC21 evaluates envelope at one-ms adjustment endpoint, not linear current-time estimate");
    calendar.step();
    check(voice.parameters.gain.value == 0 &&
          voice.gain_frames == d::duration_frames(9975000, 32006) &&
          calendar.player().notes[0].envelope_end == 10000000,
          "one-ms adjustment resumes original decay deadline without restarting phase duration");
}
void fractional_tremolo_is_quantized_before_gain_product() {
    f::Fixture fixture;
    (*fixture.bank)[32] = std::byte{127};
    (*fixture.bank)[36] = std::byte{1}; (*fixture.bank)[37] = std::byte{0};
    (*fixture.bank)[38] = std::byte{63}; (*fixture.bank)[39] = std::byte{1};
    const auto bank = fixture.decode();
    Calendar calendar(bank.asset, sequence({0, 0xb0, 7, 127, 0, 0xb0, 21, 120,
                                            0, 0x90, 60, 100, 0, 100, 0xff, 0x2f}));
    calendar.step(); calendar.step(); calendar.step();
    const auto& n = calendar.player().notes[0];
    const auto& voice = calendar.context.state().physical[0].current;
    check(n.tremolo > 96.0f && n.tremolo < 97.0f &&
          voice.parameters.gain.value == linear_public_volume(17580),
          "nonzero oscillator with rate0 initializes and quantizes fractional tremolo before integer gain composition");
    calendar.step(); calendar.step();
    check(n.tremolo > 97.0f && n.tremolo < 98.0f &&
          voice.parameters.gain.value == linear_public_volume(17763),
          "later modulation uses unmodulated note gain instead of compounding previous tremolo output");
}
void gain_controls_preserve_attack_and_release_ramps() {
    for (const bool releasing : {false, true}) {
        f::Fixture fixture;
        (*fixture.bank)[32] = std::byte{127};
        f::put32(*fixture.bank, 80, releasing ? 0 : 100000);
        f::put32(*fixture.bank, 84, 10000000); (*fixture.bank)[93] = std::byte{0};
        f::put32(*fixture.bank, 88, 100000);
        const auto bank = fixture.decode();
        std::vector<unsigned> track{0, 0xff, 0x51, 7, 0x53, 0,
                                     0, 0xb0, 7, 127, 0, 0xb0, 21, 120,
                                     0, 0x90, 60, 127, 0};
        if (releasing) track.insert(track.end(), {24, 0x80, 60, 0});
        track.insert(track.end(), {24, 0xb0, 21, 100});
        if (!releasing) track.insert(track.end(), {24, 0x90, 61, 127, 0});
        track.insert(track.end(), {100, 0xff, 0x2f});
        Calendar calendar(bank.asset, sequence_bytes(track, 480));
        calendar.step();
        const auto at = releasing ? 48000u : 24000u;
        for (unsigned steps = 0; calendar.player().client_us < at && steps < 64; ++steps) calendar.step();
        check(calendar.player().client_us == at, "bounded phase driver reaches channel-control deadline");
        const auto& voice = calendar.context.state().physical[0].current;
        const auto gain = voice.parameters.gain.value;
        const auto frames = voice.gain_frames;
        const auto end = calendar.player().notes[0].envelope_end;
        calendar.step();
        check(calendar.player().channels[0].controls.gain_scale == 100 &&
              voice.parameters.gain.value == gain && voice.gain_frames == frames &&
              calendar.player().notes[0].envelope_end == end,
              "gain control changes processed channel state without replacing an attack/release DSP ramp");
        if (!releasing) {
            for (unsigned steps = 0; calendar.player().client_us < 48000 && steps < 64; ++steps) calendar.step();
            calendar.step();
            check(calendar.context.state().physical[1].current.parameters.gain.value == linear_public_volume(24614) &&
                  voice.parameters.gain.value == linear_public_volume(29536),
                  "future note consumes new channel gain while existing attack retains authored target");
        }
    }
}
void rising_decay_control_estimate_holds_starting_scalar() {
    f::Fixture fixture;
    (*fixture.bank)[32] = std::byte{127};
    f::put32(*fixture.bank, 84, 10000000);
    (*fixture.bank)[92] = std::byte{64}; (*fixture.bank)[93] = std::byte{127};
    const auto bank = fixture.decode();
    const auto song = sequence({0, 0xff, 0x51, 7, 0x53, 0,
                                0, 0xb0, 7, 127, 0, 0xb0, 21, 120,
                                0, 0x90, 60, 127, 0,
                                0x81, 0x17, 0xb0, 21, 100, 100, 0xff, 0x2f}, 480);
    Calendar calendar(bank.asset, song);
    calendar.step();
    for (unsigned steps = 0; calendar.player().client_us < 151000 && steps < 64; ++steps) calendar.step();
    check(calendar.player().client_us == 151000, "bounded rising-decay driver reaches heldout deadline");
    calendar.step();
    check(calendar.context.state().physical[0].current.parameters.gain.value == linear_public_volume(12404),
          "rising decay uses observed starting-scalar estimate, not symmetric upward exponential interpolation");
}
void tremolo_retargets_remaining_attack_and_release() {
    f::Fixture fixture;
    (*fixture.bank)[32] = std::byte{127};
    (*fixture.bank)[36] = std::byte{1}; (*fixture.bank)[37] = std::byte{255};
    (*fixture.bank)[38] = std::byte{63}; (*fixture.bank)[39] = std::byte{1};
    f::put32(*fixture.bank, 80, 100000); f::put32(*fixture.bank, 84, 10000000);
    const auto bank = fixture.decode();
    Calendar calendar(bank.asset, sequence({0, 0xb0, 7, 127, 0, 0xb0, 21, 120,
                                            0, 0x90, 60, 100, 0, 100, 0xff, 0x2f}));
    calendar.step(); calendar.step(); calendar.step();
    const auto& voice = calendar.context.state().physical[0].current;
    const auto slot = calendar.player().notes[0].tremolo_slot;
    check(calendar.player().notes[0].phase == d::CspPhase::attack &&
          voice.parameters.gain.value == linear_public_volume(23257) && voice.gain_frames == d::duration_frames(83616, 32006),
          "tremolo during attack retargets phase-end gain over remaining attack, not sixteen-ms smoothing");
    calendar.step(); calendar.step();
    check(voice.parameters.gain.value == linear_public_volume(17580) && voice.gain_frames == d::duration_frames(67616, 32006),
          "next tremolo sample keeps original attack deadline and recomputes unmodulated target");
    calendar.player().published[0] = {d::CspControlKind::stop}; calendar.player().published_count = 1;
    calendar.step(); calendar.step();
    check(calendar.player().notes[0].phase == d::CspPhase::release &&
          voice.parameters.gain.value == 0 && voice.gain_frames == d::duration_frames(49616, 32006) &&
          calendar.context.state().oscillators.slots[slot].allocated,
          "tremolo persists through explicit-stop release and retargets zero over remaining release");
    for (unsigned steps = 0; calendar.player().client_us < 98000 && steps < 64; ++steps) calendar.step();
    check(calendar.player().client_us == 98000, "bounded modulation driver reaches stop-release endpoint");
    calendar.step();
    check(calendar.player().notes[0].phase == d::CspPhase::free &&
          !calendar.context.state().oscillators.slots[slot].allocated,
          "oscillator reclamation follows source stop/free, not note-off or player-stop request");

    f::Fixture authored;
    (*authored.bank)[32] = std::byte{127};
    (*authored.bank)[36] = std::byte{1}; (*authored.bank)[37] = std::byte{255};
    (*authored.bank)[38] = std::byte{63}; (*authored.bank)[39] = std::byte{1};
    f::put32(*authored.bank, 84, 10000000); f::put32(*authored.bank, 88, 100000);
    const auto authored_bank = authored.decode();
    Calendar releasing(authored_bank.asset, sequence({0, 0xff, 0x51, 0, 0xbb, 0x80,
                                                       0, 0xb0, 7, 127, 0, 0xb0, 21, 120,
                                                       0, 0x90, 60, 100, 0,
                                                       24, 0x80, 60, 0, 100, 0xff, 0x2f}));
    releasing.step();
    for (unsigned steps = 0; releasing.player().client_us < 32384 && steps < 64; ++steps) releasing.step();
    check(releasing.player().client_us == 32384, "bounded authored-release driver reaches first post-off modulation");
    releasing.step();
    const auto& tail = releasing.context.state().physical[0].current;
    check(releasing.player().notes[0].phase == d::CspPhase::release &&
          tail.parameters.gain.value == 0 && tail.gain_frames == d::duration_frames(91616, 32006),
          "authored release has the same remaining-phase tremolo policy without adopting stop's fifty-ms duration");
}
void effective_constructor_master_reaches_real_synthesis() {
    f::Fixture fixture;
    (*fixture.bank)[32] = std::byte{127};
    const auto bank = fixture.decode();
    const auto song = sequence({0, 0xb0, 7, 127, 0, 0xb0, 21, 120, 0, 0xb0, 10, 0,
                                0, 0x90, 60, 100, 0, 100, 0xff, 0x2f});
    auto made = service(bank.asset); if (!made) return;
    auto& audio = *made.service;
    a::PlayerId id{};
    check(audio.add_sequence_player({bank.asset, 0, song}, id) == a::Error::none,
          "constructor behavior registers real native client");
    auto player = audio.sequence_player(id);
    player.stage_sequence(0); player.stage_play(); player.stage(); // Intentionally no master setter.
    a::TaskView task{};
    check(audio.build({1, 0, 16}, task) == a::Error::none, "constructor default drives actual shared synthesis commands");
    bool left_target = false;
    const auto words = audio.rdram_words();
    for (auto at = task.command_address; at < task.command_address + task.command_bytes; at += 8)
        if ((words[at / 4] & 0xffff0000u) == 0x09020000u)
            left_target = (words[at / 4] & 65535u) == ((std::uint32_t{linear_public_volume(21976)} * 32767u) >> 15);
    check(left_target, "no-master-setter constructor produces observed gain21976, not GAME bookkeeping master");
    release(audio, task);
}
void staged_game_controls_and_wire_fx_override() {
    f::Fixture fixture;
    (*fixture.bank)[32] = std::byte{127};
    const auto bank = fixture.decode();
    const auto song = sequence({0, 0xff, 0x51, 0, 0xbb, 0x80,
                                0, 0xb0, 21, 120, 0, 0x90, 60, 100, 0,
                                48, 0xb0, 91, 1, 100, 0xff, 0x2f});
    auto made = service(bank.asset); if (!made) return;
    auto& audio = *made.service;
    a::PlayerId id{};
    check(audio.add_sequence_player({bank.asset, 0, song}, id) == a::Error::none, "game-control client registers");
    auto player = audio.sequence_player(id);
    check(player.stage_sequence(0) == a::Error::none &&
          player.stage_volume(32512) == a::Error::none &&
          player.stage_channel_program(0, 0) == a::Error::none &&
          player.stage_channel_pan(0, 0) == a::Error::none &&
          player.stage_channel_volume(0, 127) == a::Error::none &&
          player.stage_channel_priority(0, 100) == a::Error::none &&
          player.stage_channel_fx(0, 64) == a::Error::none &&
          player.stage_channel_extra_volume(0, -1) == a::Error::none &&
          player.stage_player_extra_fx(-128) == a::Error::none &&
          player.stage_play() == a::Error::none && player.stage() == a::Error::none,
          "all authoritative game control forms publish through one typed inbox");
    const auto aux = [&](const a::TaskView& task, std::uint8_t mix) {
        const auto words = audio.rdram_words();
        const auto table = f::coefficients().pan;
        bool found = false;
        for (auto at = task.command_address; at < task.command_address + task.command_bytes; at += 8)
            if ((words[at / 4] & 0xffff0000u) == 0x09080000u)
                found = (words[at / 4] & 65535u) == table[mix] && words[at / 4 + 1] == table[127u - mix];
        return found;
    };
    a::TaskView task{};
    check(audio.build({1, 0, 16}, task) == a::Error::none && aux(task, 127),
          "signed extra-FX wraps before clamp and reaches real configured-start AUX coefficients");
    csp::ChannelStatus channel{};
    check(player.channel_status(0, channel) == a::Error::none && channel.pan == 0 && channel.volume == 127 &&
          channel.priority == 100 && channel.fx_mix == 64 && channel.extra_volume == -1,
          "channel getter observes processed controls independently of wire track identity");
    check(player.stage_player_extra_fx(-1) == a::Error::none && player.stage() == a::Error::none,
          "signed extra-FX update stages safely during execution lease");
    release(audio, task);
    check(audio.build({2, 0, 1024}, task) == a::Error::none, "existing source advances after extra-FX control");
    bool retargeted = false;
    const auto words = audio.rdram_words();
    for (auto at = task.command_address; at < task.command_address + task.command_bytes; at += 8)
        if ((words[at / 4] & 0xffff0000u) == 0x09080000u) retargeted = true;
    check(!retargeted, "player-extra FX does not rewrite an existing note's send or envelope progress");
    release(audio, task);
    check(audio.build({3, 0, 1024}, task) == a::Error::none && aux(task, 0) &&
          player.channel_status(0, channel) == a::Error::none && channel.fx_mix == 1,
          "authored CC91 overrides API channel FX while retaining signed player-extra composition");
    release(audio, task);
}
void natural_end_preserves_only_existing_release_tails() {
    f::Fixture fixture;
    (*fixture.bank)[36] = std::byte{1}; (*fixture.bank)[37] = std::byte{255};
    (*fixture.bank)[38] = std::byte{63}; (*fixture.bank)[39] = std::byte{1};
    f::put32(*fixture.bank, 88, 100000);
    const auto bank = fixture.decode();
    Calendar tail(bank.asset, sequence({0, 0xff, 0x51, 0, 0xbb, 0x80,
                                       0, 0x90, 60, 100, 0,
                                       24, 0x80, 60, 0, 24, 0xff, 0x2f}));
    tail.step();
    const auto oscillator = tail.player().notes[0].tremolo_slot;
    for (unsigned steps = 0; tail.player().client_us < 48000 && steps < 64; ++steps) tail.step();
    check(tail.player().client_us == 48000, "release-tail driver reaches EOT");
    tail.step();
    check(tail.player().ended && tail.player().playback == csp::Playback::stopping &&
          tail.player().notes[0].phase == d::CspPhase::release &&
          tail.player().notes[0].envelope_end == 124000 &&
          tail.context.state().oscillators.slots[oscillator].allocated,
          "EOT preserves existing release deadline and oscillator ownership while draining");
    for (unsigned steps = 0; tail.player().client_us < 124000 && steps < 64; ++steps) tail.step();
    check(tail.player().client_us == 124000, "EOT does not replace authored tail with fifty-ms stop");
    tail.step();
    check(tail.player().notes[0].phase == d::CspPhase::free &&
          tail.player().playback == csp::Playback::stopped &&
          !tail.context.state().oscillators.slots[oscillator].allocated,
          "terminal release retires before rebind becomes available");

    Calendar held(bank.asset, sequence({0, 0xff, 0x51, 0, 0xbb, 0x80,
                                       0, 0x90, 60, 100, 0, 48, 0xff, 0x2f}));
    held.step();
    for (unsigned steps = 0; held.player().client_us < 48000 && steps < 64; ++steps) held.step();
    held.step();
    check(held.player().ended && held.player().playback == csp::Playback::stopped &&
          held.player().notes[0].phase == d::CspPhase::free,
          "duration-zero nonreleasing source still stops at natural end");
}
void sequence_replacement_preserves_custom_gain_fields() {
    f::Fixture fixture;
    (*fixture.bank)[32] = std::byte{127};
    const auto bank = fixture.decode();
    const std::array tracks{
        std::vector<unsigned>{0, 0xff, 0x51, 0, 0xbb, 0x80, 0, 0xb0, 7, 64,
                              0, 0xb0, 21, 64, 0, 0x90, 60, 100, 0, 100, 0xff, 0x2f},
        std::vector<unsigned>{0, 0xff, 0x51, 0, 0xbb, 0x80,
                              0, 0x90, 60, 100, 0, 100, 0xff, 0x2f}};
    std::size_t size = 20;
    for (const auto& track : tracks) size += (68 + track.size() + 1) & ~std::size_t{1};
    auto bytes = std::make_shared<a::Bytes>(size);
    (*bytes)[0] = std::byte{'S'}; (*bytes)[1] = std::byte{'1'}; f::put16(*bytes, 2, 2);
    std::size_t offset = 20;
    for (std::size_t i = 0; i < tracks.size(); ++i) {
        f::put32(*bytes, 4 + i * 8, static_cast<std::uint32_t>(offset));
        f::put32(*bytes, 8 + i * 8, static_cast<std::uint32_t>(68 + tracks[i].size()));
        f::put32(*bytes, offset, 68); f::put32(*bytes, offset + 64, 48);
        for (std::size_t j = 0; j < tracks[i].size(); ++j) (*bytes)[offset + 68 + j] = std::byte(tracks[i][j]);
        offset += (68 + tracks[i].size() + 1) & ~std::size_t{1};
    }
    const auto songs = a::hydrate_sequences(bytes);
    check(bool(songs), "two authored sequences admit for replacement regression");
    for (const auto extra : {-1, 64}) {
        Calendar calendar(bank.asset, songs.asset);
        calendar.player().published[3] = {d::CspControlKind::extra_volume, 0,
                                          static_cast<std::uint8_t>(extra)};
        calendar.player().published_count = 4;
        calendar.step(); calendar.step();
        check(calendar.context.state().physical[0].current.parameters.gain.value == linear_public_volume(extra < 0 ? 6251 : 3125),
              "first cue applies explicit independent CC7/CC21/extra factors");
        calendar.player().published[0] = {d::CspControlKind::stop};
        calendar.player().published_count = 1;
        for (unsigned steps = 0; calendar.player().playback != csp::Playback::stopped && steps < 64; ++steps)
            calendar.step();
        check(calendar.player().playback == csp::Playback::stopped, "first cue drains before replacement");
        calendar.player().published[0] = {d::CspControlKind::bind, 0, 1};
        calendar.player().published[1] = {d::CspControlKind::play};
        calendar.player().published_count = 2;
        calendar.step(); calendar.step();
        const auto& controls = calendar.player().channels[0].controls;
        check(controls.volume == 127 && controls.gain_scale == 64 && controls.extra_volume == extra &&
              calendar.context.state().physical[0].current.parameters.gain.value == linear_public_volume(extra < 0 ? 12404 : 6201),
              "SetSeq resets instrument-derived CC7 but preserves CC21 and signed extra across cues");
    }
}
void public_perceptual_gain_reaches_measured_DSP_amplitude() {
    f::Fixture fixture;
    (*fixture.bank)[32] = std::byte{127};
    const auto bank = fixture.decode();
    const auto song = sequence({0, 0xb0, 7, 127, 0, 0xb0, 21, 120,
                                0, 0x90, 60, 127, 0, 100, 0xff, 0x2f});
    auto coefficients = f::coefficients();
    coefficients.pan[64] = 23026; coefficients.pan[63] = 23313;
    const std::array owners{bank.asset};
    auto made = a::AudioService::create(owners, {32006, 22, 42}, coefficients, {a::effects::Routing::direct});
    check(bool(made), "gain-domain regression admits authored bank and measured numeric coefficients");
    if (!made) return;
    auto& audio = *made.service;
    a::PlayerId id{};
    check(audio.add_sequence_player({bank.asset, 0, song}, id) == a::Error::none, "gain-domain client registers");
    auto player = audio.sequence_player(id);
    check(player.stage_sequence(0) == a::Error::none && player.stage_volume(32512) == a::Error::none &&
          player.stage_play() == a::Error::none && player.stage() == a::Error::none, "gain-domain input publishes");
    a::TaskView task{};
    check(audio.build({1, 0, 16}, task) == a::Error::none, "gain-domain input produces real DSP command graph");
    const auto words = audio.rdram_words();
    bool exact_left = false;
    for (auto at = task.command_address; at < task.command_address + task.command_bytes; at += 8)
        if (words[at / 4] == 0x09064913u) exact_left = true;
    check(exact_left, "public volume29536 serializes measured left18707, not unsquared20754");
    release(audio, task);
}
void explicit_stop_retargets_an_existing_release() {
    f::Fixture fixture;
    f::put32(*fixture.bank, 88, 100000);
    const auto bank = fixture.decode();
    Calendar calendar(bank.asset, sequence({0, 0xff, 0x51, 0, 0xbb, 0x80,
                                           0, 0x90, 60, 100, 24,
                                           72, 0xff, 0x51, 0, 0xbb, 0x80, 100, 0xff, 0x2f}));
    calendar.step();
    for (unsigned steps = 0; calendar.player().client_us < 72000 && steps < 64; ++steps) calendar.step();
    check(calendar.player().client_us == 72000 &&
          calendar.player().notes[0].phase == d::CspPhase::release &&
          calendar.player().notes[0].envelope_end == 124000, "stop discriminator reaches existing authored release");
    calendar.player().published[0] = {d::CspControlKind::stop};
    calendar.player().published_count = 1;
    calendar.step();
    check(calendar.player().notes[0].envelope_end == 122000 &&
          calendar.player().playback == csp::Playback::stopping,
          "explicit Stop replaces an existing release with its own fifty-ms deadline");
    for (unsigned steps = 0; calendar.player().client_us < 122000 && steps < 64; ++steps) calendar.step();
    check(calendar.player().client_us == 122000, "retimed release remains the next exact retirement boundary");
    calendar.step();
    check(calendar.player().notes[0].phase == d::CspPhase::free &&
          calendar.player().playback == csp::Playback::stopped,
          "explicit Stop retires at its new endpoint, not the previous authored endpoint");
}
} // namespace
int main() {
    ownership_and_capacity();
    staged_lifecycle_and_lease();
    failure_rolls_back_reader_and_controls();
    typed_shadow_and_clock_policy();
    finite_envelopes_and_retime();
    repeated_keys_and_sustain();
    real_fx_send_and_progress();
    shared_oscillators_and_local_limit();
    bounded_zero_time_parser_and_inbox();
    eight_oscillator_shapes();
    key_velocity_and_wire_channel_resolution();
    independent_note_and_channel_gain_factors();
    explicit_off_releases_one_oldest_duplicate();
    raw_note_pitch_and_dsp_saturation();
    fading_gain_control_targets_its_one_ms_endpoint();
    fractional_tremolo_is_quantized_before_gain_product();
    gain_controls_preserve_attack_and_release_ramps();
    rising_decay_control_estimate_holds_starting_scalar();
    tremolo_retargets_remaining_attack_and_release();
    effective_constructor_master_reaches_real_synthesis();
    staged_game_controls_and_wire_fx_override();
    natural_end_preserves_only_existing_release_tails();
    sequence_replacement_preserves_custom_gain_fields();
    public_perceptual_gain_reaches_measured_DSP_amplitude();
    explicit_stop_retargets_an_existing_release();
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
