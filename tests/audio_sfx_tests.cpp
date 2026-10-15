#include "35mmisle/audio_sfx.hpp"
#include "audio_fixture.hpp"
#include "../src/audio_synth_internal.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <new>
#include <string_view>

namespace {
bool fail_next_allocation = false, count_allocations = false;
std::size_t allocation_attempts = 0;
}

void* operator new(std::size_t bytes) {
    if (count_allocations) ++allocation_attempts;
    if (fail_next_allocation) {
        fail_next_allocation = false;
        throw std::bad_alloc{};
    }
    if (auto* memory = std::malloc(bytes == 0 ? 1 : bytes)) return memory;
    throw std::bad_alloc{};
}
void* operator new[](std::size_t bytes) { return ::operator new(bytes); }
void operator delete(void* memory) noexcept { std::free(memory); }
void operator delete[](void* memory) noexcept { std::free(memory); }
void operator delete(void* memory, std::size_t) noexcept { std::free(memory); }
void operator delete[](void* memory, std::size_t) noexcept { std::free(memory); }

namespace a = isle::audio;
namespace s = isle::audio::sfx;
namespace d = isle::audio::detail;
namespace f = isle::audio_test;
namespace {
int failures = 0;
void check(bool condition, std::string_view meaning) {
    if (!condition) { ++failures; std::cerr << "FAIL: " << meaning << '\n'; }
}

a::ServiceResult service(const a::BankFile& bank, std::uint16_t physical = 22,
                         std::uint16_t logical = 42) {
    const std::array owners{bank};
    return a::AudioService::create(owners, {32006, physical, logical}, f::coefficients(), {a::effects::Routing::direct});
}

void author_envelope(f::Fixture& fixture, std::int32_t attack, std::int32_t decay,
                     std::int32_t release, std::uint8_t decay_volume = 127) {
    f::put32(*fixture.bank, 80, static_cast<std::uint32_t>(attack));
    f::put32(*fixture.bank, 84, static_cast<std::uint32_t>(decay));
    f::put32(*fixture.bank, 88, static_cast<std::uint32_t>(release));
    (*fixture.bank)[93] = std::byte{decay_volume};
}

// Acknowledge command ownership only. These tests neither execute an RSP nor
// inspect PCM; the actual ABI1/PCM reference checks are separate runtime gates.
void retire(a::AudioService& service, const a::TaskView& task) {
    a::PcmView pcm{};
    check(service.complete(task.token, true, pcm) == a::Error::none,
          "matching terminal acknowledgement retires command execution lease");
    check(service.release(pcm.token) == a::Error::none, "explicit consumer release retires PCM ownership");
}

bool hard_right_targets(const a::AudioService& service, const a::TaskView& task) {
    const auto words = service.rdram_words();
    int left = -1, right = -1;
    for (auto address = task.command_address; address < task.command_address + task.command_bytes; address += 8) {
        const auto first = words[address / 4];
        if ((first >> 24) != 9) continue;
        const auto flags = (first >> 16) & 0xffu;
        if (flags == 2) left = static_cast<int>(first & 0xffffu);
        if (flags == 0) right = static_cast<int>(first & 0xffffu);
    }
    return left == 0 && right > 0;
}

void registration_and_reservations() {
    f::Fixture fixture;
    auto bank = fixture.decode();
    check(bool(bank), "shared authored fixture hydrates");
    if (!bank) return;
    auto made = service(bank.asset, 1, 3);
    check(bool(made), "shared service admits authored resource");
    if (!made) return;
    auto& audio = *made.service;
    a::SoundId sound{};
    check(audio.sound_id(f::selected(bank.asset), sound) == a::Error::none, "catalog finds retained sound identity");
    a::PlayerId id{};
    s::Config config{};
    config.max_sounds = 2;
    check(audio.add_sound_player(config, id) == a::Error::none, "one player reserves two logical descriptors");
    auto player = audio.player(id);
    a::VoiceHandle remaining{}, overflow{};
    check(audio.synth().create_voice(remaining) == a::Error::none &&
          audio.synth().create_voice(overflow) == a::Error::descriptor_capacity,
          "player setup reserves from the shared logical descriptor pool");
    s::SoundHandle first{}, second{}, rejected{9, 9, 9, 9};
    check(player.stage_start(sound, {}, first) == a::Error::none, "start reserves an inactive playback slot");
    check(player.stage_start(sound, {.volume = 0x8000, .pan = 64, .pitch = 1.0f}, rejected) == a::Error::invalid_parameter &&
          rejected.generation == 0, "invalid start publishes no playback handle");
    check(player.stage_start(sound, {}, second) == a::Error::none && second.generation == first.generation + 1,
          "validation failure consumes neither playback generation nor capacity");
    s::Status first_status{}, second_status{};
    check(player.status(first, first_status) == a::Error::none && first_status.active && !first_status.bound &&
          player.status(second, second_status) == a::Error::none && second_status.active && !second_status.bound,
          "reservations are active logical playback, not premature physical allocations");
    check(first_status.voice.slot != second_status.voice.slot && first.slot != second.slot,
          "simultaneous playback uses independently reserved logical descriptors");
    check(player.stage_start(sound, {}, rejected) == a::Error::descriptor_capacity && rejected.generation == 0 &&
          player.active(first) && player.active(second), "full player rejects atomically without replacing either playback");
    check(audio.reset() == a::Error::none, "quiescent reset invalidates registrations");
    check(player.stage_stop(first) == a::Error::stale_handle && !player.active(first),
          "retained player view and playback handle cannot cross reset");
    a::PlayerId replay_id{};
    check(audio.add_sound_player(config, replay_id) == a::Error::none && replay_id.generation > id.generation,
          "re-registration receives a non-recycled registration generation");
    auto replay = audio.player(replay_id);
    s::SoundHandle fresh{};
    check(replay.stage_start(sound, {}, fresh) == a::Error::none && fresh.generation == second.generation + 1,
          "capacity failure and reset do not recycle playback generations");
    s::Status fresh_status{};
    check(replay.status(fresh, fresh_status) == a::Error::none &&
          fresh_status.voice.generation > second_status.voice.generation,
          "stable logical descriptor identity has its own reset generation");
    check(replay.stage_pan(first, 0) == a::Error::stale_handle,
          "old playback controls cannot address a newly registered slot");
}

void staged_publication_and_lease() {
    f::Fixture fixture;
    auto bank = fixture.decode();
    if (!bank) { check(false, "publication fixture hydrates"); return; }
    auto made = service(bank.asset);
    if (!made) { check(false, "publication service creates"); return; }
    auto& audio = *made.service;
    a::SoundId sound{};
    audio.sound_id(f::selected(bank.asset), sound);
    s::Config config{};
    config.first_due = a::RawSampleTime{32};
    a::PlayerId id{};
    check(audio.add_sound_player(config, id) == a::Error::none, "publication player registers");
    auto player = audio.player(id);
    s::SoundHandle handle{};
    check(player.stage_start(sound, {}, handle) == a::Error::none, "start enters bounded unpublished inbox");
    a::TaskView task{};
    check(audio.build({1, 0, 64}, task) == a::Error::none, "unpublished start allows an idle command task");
    s::Status status{};
    check(player.status(handle, status) == a::Error::none && status.active && !status.bound,
          "crossing a deadline cannot consume an unpublished start");
    const auto words = audio.rdram_words();
    const std::array watched{words[a::AudioService::command_address / 4],
                             words[a::AudioService::pcm_address / 4],
                             words[a::AudioService::state_address / 4]};
    check(player.stage() == a::Error::none && player.stage_volume(handle, 16384) == a::Error::none &&
          player.stage() == a::Error::none, "multiple publications append for the next callback during a task lease");
    check(words[a::AudioService::command_address / 4] == watched[0] &&
          words[a::AudioService::pcm_address / 4] == watched[1] &&
          words[a::AudioService::state_address / 4] == watched[2],
          "staging never rewrites the published command/PCM/DSP state storage");
    a::TaskView blocked{};
    check(audio.build({2, 0, 512}, blocked) == a::Error::task_busy && blocked.token.serial == 0 &&
          audio.reset() == a::Error::task_busy, "client staging never consumes or bypasses a live task lease");
    retire(audio, task);
    check(audio.build({2, 0, 512}, task) == a::Error::none, "next task crosses the next shared player deadline");
    check(player.status(handle, status) == a::Error::none && status.active && status.bound,
          "next callback consumes staged start and binds its logical descriptor");
    check(player.stage_pan(handle, 127) == a::Error::none && player.stage() == a::Error::none,
          "controls can stage after build publication without completion restoring stale client state");
    retire(audio, task);
    check(audio.build({3, 0, 512}, task) == a::Error::none && player.active(handle) &&
          hard_right_targets(audio, task),
          "completion preserves the staged pan in emitted left-zero/right-positive gain targets");
    retire(audio, task);
}

void bounded_publication() {
    f::Fixture fixture;
    auto bank = fixture.decode();
    if (!bank) return;
    auto made = service(bank.asset);
    if (!made) { check(false, "capacity service creates"); return; }
    auto& audio = *made.service;
    a::SoundId sound{};
    audio.sound_id(f::selected(bank.asset), sound);
    s::Config config{};
    config.first_due = a::RawSampleTime{0};
    a::PlayerId id{};
    audio.add_sound_player(config, id);
    auto player = audio.player(id);
    s::SoundHandle handle{};
    check(player.stage_start(sound, {}, handle) == a::Error::none, "bounded publication reserves start");
    check(player.stage_volume(handle, 0x8000) == a::Error::invalid_parameter &&
          player.stage_pitch(handle, std::numeric_limits<float>::infinity()) == a::Error::invalid_parameter,
          "invalid controls consume no inbox capacity");
    for (std::size_t i = 1; i < s::SoundPlayer::event_capacity; ++i)
        check(player.stage_pan(handle, static_cast<std::uint8_t>(i % 128)) == a::Error::none,
              "bounded inbox accepts each available entry");
    check(player.stage_stop(handle) == a::Error::event_capacity && player.stage() == a::Error::none,
          "inbox overflow is explicit before publication");
    for (std::size_t i = 0; i < s::SoundPlayer::event_capacity; ++i)
        check(player.stage_volume(handle, static_cast<std::uint16_t>(16000 + i)) == a::Error::none,
              "publication restores independent inbox capacity");
    check(player.stage() == a::Error::event_capacity && player.active(handle),
          "full next-callback publication rejects atomically and retains the inbox");
    a::TaskView task{};
    check(audio.build({1, 0, 16}, task) == a::Error::none, "callback consumes the first full publication");
    check(player.stage() == a::Error::none && player.stage_stop(handle) == a::Error::none,
          "failed publication retained its complete inbox and consumed no new inbox slot");
    retire(audio, task);
}

d::SfxCheckpoint& checkpoint(d::Context& context, unsigned bank, a::PlayerId id) {
    return context.sfx_state(bank, context.cpu[bank].clients[id.slot].typed_index);
}

struct ClientFixture {
    f::Fixture authored;
    a::BankResult bank;
    std::unique_ptr<d::Context> context = std::make_unique<d::Context>();
    a::SoundId sound{};
    a::PlayerId player{};
    explicit ClientFixture(std::int32_t attack = 0, std::uint16_t physical = 22,
                           std::int32_t decay = -1, std::int32_t release = 6000,
                           std::uint8_t decay_volume = 127) {
        author_envelope(authored, attack, decay, release, decay_volume);
        bank = authored.decode();
        if (!bank) { check(false, "client fixture hydrates"); return; }
        context->owner = 901;
        context->config = {32006, physical, 42};
        context->coefficients = f::coefficients();
        context->memory = std::make_unique<std::uint32_t[]>(a::AudioService::rdram_bytes / 4);
        const std::array owners{bank.asset};
        check(d::build_catalog(*context, owners) == a::Error::none, "client fixture packs genuine authored catalog");
        check(d::find_sound(*context, f::selected(bank.asset), sound) == a::Error::none, "client fixture resolves catalog identity");
        s::Config config{};
        config.max_sounds = 10;
        config.first_due = a::RawSampleTime{0};
        config.first_client_us = 0;
        check(d::add_player(*context, config, player) == a::Error::none,
              "client fixture registers one ten-descriptor calendar");
    }
    s::SoundHandle start(std::uint16_t slot = 0, s::Parameters parameters = {}) {
        auto& registered = checkpoint(*context, context->live, player);
        auto& voice = registered.sounds[slot];
        voice.phase = d::SoundPhase::reserved;
        voice.generation = ++context->next_sound;
        voice.sound = sound;
        voice.parameters = voice.desired = parameters;
        s::SoundHandle handle{context->owner, voice.generation, player.slot, slot};
        registered.published_events[registered.published_count++] = {d::PlayerEventKind::start, handle, sound, parameters};
        return handle;
    }
};

void registration_storage_failure_preserves_existing_client() {
    ClientFixture fixture;
    if (!fixture.bank) return;
    const auto handle = fixture.start();
    auto& context = *fixture.context;
    auto& original = checkpoint(context, context.live, fixture.player);
    original.inbox[original.inbox_count++] = {d::PlayerEventKind::pan, handle, {}, {.volume = 0, .pan = 127, .pitch = 1.0f}};
    const auto live_bank = context.live;
    const auto next_voice = context.next_voice;
    const auto next_player = context.next_player;
    const auto descriptor = original.sounds[0].descriptor;
    s::Config config{};
    config.max_sounds = 1;
    config.first_due = a::RawSampleTime{0};
    config.first_client_us = 0;
    a::PlayerId rejected{9, 9, 9};
    fail_next_allocation = true;
    const auto error = d::add_player(context, config, rejected);
    const auto failed_storage = !fail_next_allocation;
    fail_next_allocation = false;
    check(error == a::Error::allocation_failure && failed_storage && rejected.generation == 0,
          "typed registration reports storage exhaustion without publishing a capability");
    check(context.live == live_bank && context.next_voice == next_voice && context.next_player == next_player &&
          original.published_count == 1 && original.inbox_count == 1 &&
          original.sounds[0].generation == handle.generation &&
          std::count_if(context.state().clients.begin(), context.state().clients.end(),
                        [](const auto& client) { return client.live; }) == 1 &&
          std::count_if(context.state().logical.begin(), context.state().logical.end(),
                        [](const auto& voice) { return voice.live; }) == 10,
          "failed storage acquisition preserves prior registration, both queues, descriptors and identities");
    a::PlayerId added{};
    check(d::add_player(context, config, added) == a::Error::none,
          "registration can retry after transient storage exhaustion");
    const auto& retained = checkpoint(context, context.live, fixture.player);
    check(retained.published_count == 1 && retained.inbox_count == 1 &&
          retained.sounds[0].descriptor.owner == descriptor.owner &&
          retained.sounds[0].descriptor.generation == descriptor.generation &&
          retained.sounds[0].descriptor.slot == descriptor.slot &&
          retained.inbox[0].handle.generation == handle.generation &&
          retained.inbox[0].parameters.pan == 127 &&
          added.generation == next_player + 1 && context.next_voice == next_voice + 1,
          "publishing another typed pair preserves existing checkpoints without consuming failed identities");
}

void multi_client_failure_retry_and_reset() {
    f::Fixture fixture;
    auto bank = fixture.decode();
    if (!bank) return;
    auto made = service(bank.asset, 2, 2);
    if (!made) { check(false, "multi-client transaction service creates"); return; }
    auto& audio = *made.service;
    a::SoundId sound{};
    check(audio.sound_id(f::selected(bank.asset), sound) == a::Error::none,
          "multi-client transaction resolves retained sound");
    s::Config config{};
    config.max_sounds = 1;
    config.first_due = a::RawSampleTime{0};
    config.first_client_us = 0;
    a::PlayerId first_id{}, failing_id{};
    check(audio.add_sound_player(config, first_id) == a::Error::none,
          "first transaction participant registers");
    auto first = audio.player(first_id);
    s::SoundHandle one{}, two{};
    check(first.stage_start(sound, {}, one) == a::Error::none && first.stage() == a::Error::none,
          "existing participant publishes before another typed pair is registered");
    config.first_due = a::RawSampleTime{16};
    config.first_client_us = std::numeric_limits<std::uint64_t>::max() - 4;
    check(audio.add_sound_player(config, failing_id) == a::Error::none,
          "later participant has an independently overflowing client clock");
    auto failing = audio.player(failing_id);
    check(failing.stage_start(sound, {}, two) == a::Error::none && failing.stage() == a::Error::none,
          "later participant publishes independently");
    a::TaskView task{};
    allocation_attempts = 0;
    count_allocations = true;
    const auto failed_build = audio.build({1, 0, 32}, task);
    count_allocations = false;
    check(failed_build == a::Error::identity_exhausted && task.token.serial == 0 &&
          !audio.has_lease() && allocation_attempts == 0,
          "late typed callback failure publishes no task and allocates no service storage");
    s::Status first_status{}, failing_status{};
    check(audio.synth().control_frontier().value == 0 &&
          first.status(one, first_status) == a::Error::none && first_status.active && !first_status.bound &&
          failing.status(two, failing_status) == a::Error::none && failing_status.active && !failing_status.bound,
          "failure after earlier callbacks commits neither participant's checkpoints or common bindings");
    check(first.stage_pan(one, 127) == a::Error::none && first.stage() == a::Error::none &&
          audio.set_player_deadline(failing_id, {32}) == a::Error::none,
          "retry accepts new controls and defers the failing calendar without replacing its checkpoint");
    allocation_attempts = 0;
    count_allocations = true;
    const auto retried_build = audio.build({1, 0, 32}, task);
    count_allocations = false;
    check(retried_build == a::Error::none && task.token.serial == 1 && allocation_attempts == 0 &&
          first.status(one, first_status) == a::Error::none && first_status.bound &&
          failing.status(two, failing_status) == a::Error::none && !failing_status.bound,
          "retry recopies all typed checkpoints, preserves deferred publication and commits through one bank");
    if (retried_build != a::Error::none) return;
    check(hard_right_targets(audio, task),
          "retry renders the pan appended after failure instead of losing it with the dirty shadow");
    retire(audio, task);
    const auto* memory = audio.rdram_words().data();
    const auto resource_word = audio.rdram_words()[a::AudioService::resource_address / 4];
    allocation_attempts = 0;
    count_allocations = true;
    const auto reset = audio.reset();
    count_allocations = false;
    check(reset == a::Error::none && allocation_attempts == 0 && audio.rdram_words().data() == memory &&
          audio.rdram_words()[a::AudioService::resource_address / 4] == resource_word &&
          first.stage_stop(one) == a::Error::stale_handle && failing.stage_stop(two) == a::Error::stale_handle,
          "quiescent reset invalidates every typed registration without allocation or RDRAM replacement");
    config.max_sounds = 2;
    config.first_due = a::RawSampleTime{0};
    config.first_client_us = 0;
    a::PlayerId replacement{};
    check(audio.add_sound_player(config, replacement) == a::Error::none &&
          replacement.generation > failing_id.generation,
          "reset returns all typed descriptor capacity without recycling registration identities");
}

void shadow_isolation_and_envelope_calendar() {
    ClientFixture fixture(10000);
    if (!fixture.bank) return;
    fixture.start(0, {.volume = 16384, .pan = 64, .pitch = 1.5f});
    auto& context = *fixture.context;
    auto& live = context.state();
    const auto bank = d::prepare_shadow(context);
    auto& shadow = context.cpu[bank];
    check(d::dispatch_client(context, bank, fixture.player.slot) == a::Error::none,
          "typed callback selects its explicit shadow bank even when no build flag is set");
    const auto& original = checkpoint(context, context.live, fixture.player);
    auto& staged = checkpoint(context, bank, fixture.player);
    check(original.sounds[0].phase == d::SoundPhase::reserved && original.published_count == 1 &&
          live.clients[fixture.player.slot].next_raw_due == 0 &&
          live.logical[original.sounds[0].descriptor.slot].physical == a::no_slot,
          "speculative callback leaves live client clocks, events, phases and bindings untouched");
    check(staged.sounds[0].phase == d::SoundPhase::attack && staged.published_count == 0 &&
          staged.sounds[0].event_us == 6666 && staged.client_us == 6666 && staged.poll_us == 16000 &&
          shadow.clients[fixture.player.slot].next_raw_due == d::rounded_frames(6666, 32006),
          "one calendar chooses the pitch-scaled envelope event before the 16000-us poll");
    const auto physical = shadow.logical[staged.sounds[0].descriptor.slot].physical;
    check(physical != a::no_slot, "speculative start binds only the shadow descriptor");
    if (physical == a::no_slot) return;
    const auto perceived = (16384u * 127u) >> 7;
    const auto amplitude = static_cast<std::uint16_t>((perceived * perceived) >> 15);
    check(shadow.physical[physical].current.parameters.gain.value == amplitude,
          "player attack-volume shift and perceptual square precede linear CoreGain15");
    check(d::render(context, shadow, 208, 0, 208) == a::Error::none,
          "real shared renderer advances to the envelope callback DSP floor");
    check(!shadow.physical[physical].current.envelope_init, "render emitted the initial envelope setup");
    context.building = true;
    check(d::dispatch_client(context, bank, fixture.player.slot) == a::Error::none &&
          staged.sounds[0].phase == d::SoundPhase::sustain &&
          !shadow.physical[physical].current.envelope_init,
          "raw attack endpoint changes CPU phase without another DSP envelope INIT");
    context.building = false;
    check(shadow.clients[fixture.player.slot].next_raw_due ==
              d::rounded_frames(6666, 32006) + d::rounded_frames(9334, 32006) &&
          staged.client_us == 16000 && staged.poll_us == 16000,
          "next raw deadline adds the rounded microsecond delta rather than converting absolute time");
}

void control(d::SfxCheckpoint& player, s::SoundHandle handle, d::PlayerEventKind kind,
             s::Parameters parameters = {}) {
    player.published_events[player.published_count++] = {kind, handle, {}, parameters};
}

void next_callback(d::Context& context, a::PlayerId id) {
    auto& state = context.state();
    const auto due = state.clients[id.slot].next_raw_due & ~std::uint64_t{15};
    const auto frames = static_cast<std::uint32_t>(due - state.sample_clock);
    if (frames != 0)
        check(d::render(context, state, frames, 0, frames) == a::Error::none,
              "shared DSP commands advance physical state to the next callback floor");
    check(d::advance_parameters(context, state, {due}) == a::Error::none,
          "physical endpoint events become visible before callback allocation");
    check(d::dispatch_client(context, context.live, id.slot) == a::Error::none,
          "one typed player callback advances all of its logical descriptors");
}

void attack_dsp_time_is_not_pitch_scaled() {
    ClientFixture fixture(1001);
    if (!fixture.bank) return;
    fixture.start(0, {.volume = 32767, .pan = 0, .pitch = 0.5f});
    auto& context = *fixture.context;
    auto& state = context.state();
    check(d::dispatch_client(context, context.live, fixture.player.slot) == a::Error::none &&
          checkpoint(context, context.live, fixture.player).sounds[0].event_us == 2002,
          "half-pitch attack callback retains its pitch-scaled microsecond deadline");
    check(d::render(context, state, 16, 0, 16) == a::Error::none,
          "attack emits its actual ABI1 envelope setup");
    const auto rate = d::envelope_rate(1, 32255, 32, context.coefficients);
    bool found = false;
    for (auto address = a::AudioService::command_address; address < context.write_address; address += 8)
        if (context.memory[address / 4] == (0x09020000u | 32255u) &&
            context.memory[address / 4 + 1] == rate) found = true;
    check(found, "authored 1001-us attack emits a 32-frame DSP ramp, not its 64-frame callback duration");
}

void measured_posted_start_order() {
    ClientFixture fixture;
    if (!fixture.bank) return;
    auto& context = *fixture.context;
    for (std::uint16_t slot = 10; slot != 0; --slot) fixture.start(slot - 1);
    check(d::dispatch_client(context, context.live, fixture.player.slot) == a::Error::none,
          "one callback processes all ten posted logical starts");
    const auto& player = checkpoint(context, context.live, fixture.player);
    for (std::uint16_t slot = 0; slot < 10; ++slot)
        check(context.state().logical[player.sounds[slot].descriptor.slot].physical == 9 - slot,
              "equal-time starts allocate in posted order, not descriptor slot order");
    check(player.client_us == 16000 &&
          context.state().clients[fixture.player.slot].next_raw_due == d::rounded_frames(16000, 32006),
          "ten starts advance one player calendar, not ten per-voice calendars");
}

void sequential_controls_and_earliest_envelope() {
    ClientFixture ordered;
    if (!ordered.bank) return;
    const auto handle = ordered.start();
    auto& context = *ordered.context;
    auto& player = checkpoint(context, context.live, ordered.player);
    control(player, handle, d::PlayerEventKind::pitch, {.volume = 0, .pan = 0, .pitch = 0.5f});
    control(player, handle, d::PlayerEventKind::stop);
    control(player, handle, d::PlayerEventKind::pitch, {.volume = 0, .pan = 0, .pitch = 1.5f});
    check(d::dispatch_client(context, context.live, ordered.player.slot) == a::Error::none &&
          player.sounds[0].phase == d::SoundPhase::release && player.sounds[0].event_us == 12000 &&
          player.sounds[0].parameters.pitch == 1.5f,
          "queued controls execute sequentially: stop uses preceding pitch, not a coalesced final pitch");

    ClientFixture envelopes(10000);
    if (!envelopes.bank) return;
    envelopes.start(0, {.volume = 32767, .pan = 64, .pitch = 1.0f});
    envelopes.start(1, {.volume = 32767, .pan = 64, .pitch = 1.5f});
    auto& registered = checkpoint(*envelopes.context, envelopes.context->live, envelopes.player);
    check(d::dispatch_client(*envelopes.context, envelopes.context->live, envelopes.player.slot) == a::Error::none &&
          registered.client_us == 6666, "earliest of two sound envelopes owns the next single player deadline");
    next_callback(*envelopes.context, envelopes.player);
    check(registered.sounds[1].phase == d::SoundPhase::sustain &&
          registered.sounds[0].phase == d::SoundPhase::attack && registered.client_us == 10000 &&
          envelopes.context->state().clients[envelopes.player.slot].next_raw_due ==
              d::rounded_frames(6666, 32006) + d::rounded_frames(3334, 32006),
          "next envelope event preempts the periodic poll using per-delta rounded frames");
    next_callback(*envelopes.context, envelopes.player);
    check(registered.sounds[0].phase == d::SoundPhase::sustain && registered.client_us == 16000,
          "periodic 16000-us polling resumes after all earlier envelope events");
}

void unchanged_volume_restarts_the_live_envelope() {
    ClientFixture fixture(0, 1);
    if (!fixture.bank) return;
    const auto handle = fixture.start(0, {.volume = 21000, .pan = 127, .pitch = 1.5f});
    auto& context = *fixture.context;
    auto& state = context.state();
    check(d::dispatch_client(context, context.live, fixture.player.slot) == a::Error::none,
          "unchanged-volume test starts a live player source");
    next_callback(context, fixture.player);
    bool initial_from = false, initial_target = false;
    for (auto address = a::AudioService::command_address; address < context.write_address; address += 8) {
        const auto word = context.memory[address / 4];
        const auto argument = context.memory[address / 4 + 1];
        initial_from = initial_from || (word == 0x09060001u && argument == 0);
        initial_target = initial_target || (word == 0x09020000u && argument == 0);
    }
    check(initial_from && initial_target,
          "configured zero-attack startup evaluates its silent CPU origin instead of emitting an untouched raw zero");
    auto& player = checkpoint(context, context.live, fixture.player);
    control(player, handle, d::PlayerEventKind::volume, {.volume = 21000, .pan = 127, .pitch = 1.5f});
    next_callback(context, fixture.player);
    context.write_address = a::AudioService::command_address;
    check(d::render(context, state, 16, 0, 16) == a::Error::none,
          "live repeated volume renders its one-millisecond transition");
    bool from = false, target = false;
    unsigned initializations = 0;
    for (auto address = a::AudioService::command_address; address < context.write_address; address += 8) {
        const auto word = context.memory[address / 4];
        const auto argument = context.memory[address / 4 + 1];
        from = from || (word == 0x09060001u && argument == 0);
        target = target || (word == 0x09020000u && argument == 0x103e8);
        initializations += (word & 0xffff0000u) == 0x03090000u ? 1u : 0u;
    }
    check(from && target && initializations == 1,
          "unchanged player volume is not coalesced: it reinitializes ENV with its CPU-normalized silent channel");
}

void gain_retarget_and_stolen_controls() {
    ClientFixture fixture(0, 1);
    if (!fixture.bank) return;
    auto handle = fixture.start(0, {.volume = 32767, .pan = 64, .pitch = 1.0f});
    auto& context = *fixture.context;
    auto& state = context.state();
    check(d::dispatch_client(context, context.live, fixture.player.slot) == a::Error::none, "gain test starts authored voice");
    auto& player = checkpoint(context, context.live, fixture.player);
    auto& sound = player.sounds[0];
    const auto physical = state.logical[sound.descriptor.slot].physical;
    if (physical == a::no_slot) { check(false, "gain test binds physical voice"); return; }
    const auto perceived = (32767u * 127u) >> 7;
    check(state.physical[physical].current.parameters.gain.value == (perceived * perceived) >> 15,
          "explicit player volume bypasses sample volume and game-adapter gain conversion");
    control(player, handle, d::PlayerEventKind::volume, {.volume = 16000, .pan = 64, .pitch = 1.0f});
    control(player, handle, d::PlayerEventKind::pan, {.volume = 0, .pan = 127, .pitch = 1.0f});
    next_callback(context, fixture.player);
    const auto command_begin = context.write_address;
    next_callback(context, fixture.player);
    const auto old_amplitude = (perceived * perceived) >> 15;
    const auto new_perceived = (16000u * 127u) >> 7;
    const auto new_amplitude = (new_perceived * new_perceived) >> 15;
    const std::array<std::uint16_t, 2> from{
        static_cast<std::uint16_t>((old_amplitude * context.coefficients.pan[64]) >> 15),
        static_cast<std::uint16_t>((old_amplitude * context.coefficients.pan[63]) >> 15)};
    const std::array<std::uint16_t, 2> target{
        0, static_cast<std::uint16_t>((new_amplitude * context.coefficients.pan[0]) >> 15)};
    std::array<bool, 2> found{};
    for (auto address = command_begin; address < context.write_address; address += 8) {
        const auto command = context.memory[address / 4];
        const auto argument = context.memory[address / 4 + 1];
        for (std::size_t channel = 0; channel < found.size(); ++channel) {
            const auto side = channel == 0 ? 0x00020000u : 0u;
            const auto rate = d::envelope_rate(from[channel], target[channel],
                                              d::duration_frames(1000, 32006), context.coefficients);
            if (command == (0x09000000u | side | target[channel]) && argument == rate)
                found[channel] = true;
        }
    }
    check(found[0] && found[1],
          "live volume emits one-millisecond channel envelope rates, preserved by the following pan control");
    a::VoiceHandle replacement{};
    a::Allocation allocation{};
    const a::ParamTime time{state.sample_clock};
    check(d::create_voice(context, state, replacement) == a::Error::none &&
          d::allocate_voice(context, state, replacement, 65, time, allocation) == a::Error::none &&
          allocation.displaced.generation == sound.descriptor.generation,
          "higher priority physical allocation displaces the original logical owner");
    check(d::start_voice(context, state, replacement, {context.owner, 0}, {1.0f, {12345}, 32}, {0}, time) == a::Error::none,
          "replacement schedules its own source independently of stolen playback identity");
    control(player, handle, d::PlayerEventKind::pitch, {.volume = 0, .pan = 0, .pitch = 1.5f});
    control(player, handle, d::PlayerEventKind::pan, {.volume = 0, .pan = 127, .pitch = 1.0f});
    control(player, handle, d::PlayerEventKind::volume, {.volume = 21000, .pan = 0, .pitch = 1.0f});
    next_callback(context, fixture.player);
    check(sound.generation == handle.generation && sound.phase == d::SoundPhase::sustain &&
          sound.parameters.pitch == 1.5f && sound.parameters.pan == 127 && sound.parameters.volume == 21000 &&
          sound.error == a::Error::unbound && state.logical[sound.descriptor.slot].physical == a::no_slot,
          "stolen playback remains logically live and retains its accepted unbound controls");
    check(state.physical[physical].owner == replacement.slot &&
          state.physical[physical].current.parameters.pitch == 1.0f &&
          state.physical[physical].current.parameters.pan == 32 &&
          state.physical[physical].current.parameters.gain.value == 12345,
          "old playback controls never route to the replacement physical owner");
    const auto release_begin_us = player.client_us;
    control(player, handle, d::PlayerEventKind::stop);
    next_callback(context, fixture.player);
    check(sound.phase == d::SoundPhase::release && sound.event_us == release_begin_us + 4000,
          "unbound stop uses the updated pitch to scale the release microseconds");
    next_callback(context, fixture.player);
    check(sound.phase == d::SoundPhase::free && sound.generation == handle.generation &&
          state.physical[physical].owner == replacement.slot &&
          state.physical[physical].current.parameters.gain.value == 12345,
          "unbound release retires only the playback, never its replacement");
}

void measured_failed_allocation_lifecycle() {
    ClientFixture fixture(0, 1);
    if (!fixture.bank) return;
    auto& context = *fixture.context;
    auto& state = context.state();
    a::VoiceHandle blocker{};
    a::Allocation allocation{};
    check(d::create_voice(context, state, blocker) == a::Error::none &&
          d::allocate_voice(context, state, blocker, 100, {0}, allocation) == a::Error::none &&
          d::start_voice(context, state, blocker, {context.owner, 0}, {1.0f, {10000}, 64}, {0}, {0}) == a::Error::none,
          "higher-priority direct voice fills the physical pool");
    const auto handle = fixture.start();
    check(d::dispatch_client(context, context.live, fixture.player.slot) == a::Error::none,
          "failed physical start does not fail the player callback");
    auto& player = checkpoint(context, context.live, fixture.player);
    auto& sound = player.sounds[0];
    check(sound.phase == d::SoundPhase::sustain && sound.error == a::Error::unbound &&
          state.logical[sound.descriptor.slot].physical == a::no_slot,
          "measured failed allocation remains logically playing and unbound rather than dropping");
    control(player, handle, d::PlayerEventKind::volume, {.volume = 21000, .pan = 0, .pitch = 1.0f});
    control(player, handle, d::PlayerEventKind::pitch, {.volume = 0, .pan = 0, .pitch = 1.5f});
    control(player, handle, d::PlayerEventKind::pan, {.volume = 0, .pan = 127, .pitch = 1.0f});
    check(d::stop_voice(context, state, blocker, {0}) == a::Error::none &&
          d::free_voice(context, state, blocker, {0}) == a::Error::none, "blocked pool later becomes free");
    next_callback(context, fixture.player);
    check(sound.phase == d::SoundPhase::sustain && sound.parameters.volume == 21000 &&
          sound.parameters.pitch == 1.5f && sound.parameters.pan == 127 &&
          state.logical[sound.descriptor.slot].physical == a::no_slot,
          "failed allocation accepts logical controls but never retries when capacity becomes available");
    control(player, handle, d::PlayerEventKind::stop);
    next_callback(context, fixture.player);
    check(sound.phase == d::SoundPhase::release, "explicit stop begins release for failed allocation playback");
    next_callback(context, fixture.player);
    check(sound.phase == d::SoundPhase::free, "failed allocation playback reaches inactive only through explicit stop");
}

void failed_callback_shadow_isolation() {
    ClientFixture fixture;
    if (!fixture.bank) return;
    fixture.start();
    auto& context = *fixture.context;
    auto& live = context.state();
    const auto bank = d::prepare_shadow(context);
    auto& shadow = context.cpu[bank];
    shadow.parameter_count = shadow.parameters.size();
    for (auto& event : shadow.parameters) event.due = 4096;
    check(d::dispatch_client(context, bank, fixture.player.slot) == a::Error::event_capacity,
          "exhausted physical parameter heap reports transactional callback failure");
    const auto& player = checkpoint(context, context.live, fixture.player);
    check(player.published_count == 1 && live.clients[fixture.player.slot].next_raw_due == 0 &&
          player.sounds[0].phase == d::SoundPhase::reserved &&
          live.logical[player.sounds[0].descriptor.slot].physical == a::no_slot,
          "failed callback cannot consume live publication, clocks, phases or logical binding");
}

void configured_start_second_step_failure_is_shadow_local() {
    for (const bool full_heap : {true, false}) {
        ClientFixture fixture(0, 2);
        if (!fixture.bank) return;
        fixture.start(0);
        fixture.start(1);
        auto& context = *fixture.context;
        auto& live = context.state();
        check(d::dispatch_client(context, context.live, fixture.player.slot) == a::Error::none,
              "two live sources fill the configured physical pool");
        auto& player = checkpoint(context, context.live, fixture.player);
        const auto protected_voice = player.sounds[0].descriptor;
        check(d::priority_voice(context, live, protected_voice, 100) == a::Error::none &&
              d::render(context, live, 512, 0, 512) == a::Error::none,
              "protected source advances to the next client boundary");
        fixture.start(2);
        if (full_heap) {
            for (std::size_t i = 0; i != a::AudioService::parameter_capacity - 2; ++i)
                check(d::pitch_voice(context, live, protected_voice, 1.0f, {4096}) == a::Error::none,
                      "future controls fill the heap without targeting the source to be displaced");
        }
        const auto old_owner = live.physical[1].owner;
        const auto old_sequence = live.next_parameter_sequence;
        const auto old_due = live.clients[fixture.player.slot].next_raw_due;
        const auto old_count = live.parameter_count;
        const auto bank = d::prepare_shadow(context);
        auto& shadow = context.cpu[bank];
        if (!full_heap)
            shadow.next_parameter_sequence = std::numeric_limits<std::uint64_t>::max() - 3;
        const auto expected = full_heap ? a::Error::event_capacity : a::Error::identity_exhausted;
        check(d::dispatch_client(context, bank, fixture.player.slot) == expected,
              "deferred configured start reports failure of its second primitive");
        check(player.published_count == 1 && live.clients[fixture.player.slot].next_raw_due == old_due &&
              player.sounds[2].phase == d::SoundPhase::reserved &&
              live.logical[player.sounds[2].descriptor.slot].physical == a::no_slot &&
              live.physical[1].owner == old_owner && live.next_parameter_sequence == old_sequence &&
              live.parameter_count == old_count,
              "failed configured-start lowering commits no source transfer, publication, clock or event identity");
    }
}

void playback_reuse_keeps_descriptor_not_generation() {
    f::Fixture fixture;
    auto bank = fixture.decode();
    if (!bank) return;
    auto made = service(bank.asset, 1, 1);
    if (!made) { check(false, "playback reuse service creates"); return; }
    auto& audio = *made.service;
    a::SoundId sound{};
    audio.sound_id(f::selected(bank.asset), sound);
    s::Config config{};
    config.max_sounds = 1;
    config.first_due = a::RawSampleTime{0};
    a::PlayerId id{};
    audio.add_sound_player(config, id);
    auto player = audio.player(id);
    s::SoundHandle old{};
    check(player.stage_start(sound, {}, old) == a::Error::none && player.stage() == a::Error::none,
          "first playback reserves the only descriptor slot");
    a::TaskView task{};
    check(audio.build({1, 0, 16}, task) == a::Error::none, "first callback binds the initial playback");
    s::Status initial{};
    player.status(old, initial);
    check(player.stage_stop(old) == a::Error::none && player.stage() == a::Error::none,
          "stop stages while its initial command task remains leased");
    retire(audio, task);
    check(audio.build({2, 0, 512}, task) == a::Error::none && player.active(old),
          "stop callback preserves playback identity throughout release");
    retire(audio, task);
    check(audio.build({3, 0, 256}, task) == a::Error::none && !player.active(old),
          "raw release endpoint makes the logical sound slot inactive");
    retire(audio, task);
    s::SoundHandle fresh{};
    check(player.stage_start(sound, {}, fresh) == a::Error::none && fresh.slot == old.slot &&
          fresh.generation > old.generation, "inactive slot reuse creates a new playback generation");
    s::Status replacement{};
    check(player.status(fresh, replacement) == a::Error::none &&
          replacement.voice.slot == initial.voice.slot &&
          replacement.voice.generation == initial.voice.generation,
          "playback generation changes while its setup-reserved VoiceHandle remains stable");
    check(player.stage_volume(old, 0) == a::Error::stale_handle &&
          player.status(old, replacement) == a::Error::stale_handle && player.active(fresh),
          "old playback generation cannot inspect or control the reused logical descriptor");
}

void bounded_calendar_overflow() {
    ClientFixture fixture;
    if (!fixture.bank) return;
    auto& context = *fixture.context;
    auto& state = context.state();
    auto& player = checkpoint(context, context.live, fixture.player);
    player.client_us = player.poll_us = std::numeric_limits<std::uint64_t>::max() - 4;
    check(d::dispatch_client(context, context.live, fixture.player.slot) == a::Error::identity_exhausted,
          "periodic client microseconds cannot wrap at the calendar boundary");
    player.client_us = player.poll_us = 0;
    state.clients[fixture.player.slot].next_raw_due =
        state.sample_clock = std::numeric_limits<std::uint64_t>::max() & ~std::uint64_t{15};
    check(d::dispatch_client(context, context.live, fixture.player.slot) == a::Error::identity_exhausted,
          "per-delta rounded raw deadline cannot wrap at the sample-clock boundary");
}

void unsupported_envelopes_and_setup_rollback(bool zero_level = false) {
    f::Fixture fixture;
    author_envelope(fixture, 0, zero_level ? 12000 : 0, 6000, zero_level ? 0 : 127);
    auto bank = fixture.decode();
    if (!bank) return;
    auto made = service(bank.asset, 1, 1);
    if (!made) { check(false, "unsupported-envelope catalog remains identifiable"); return; }
    auto& audio = *made.service;
    a::PlayerId id{9, 9, 9};
    check(audio.add_sound_player({}, id) == a::Error::descriptor_capacity && id.generation == 0,
          "failed setup reserves no partial subset of ten descriptors");
    s::Config config{};
    config.max_sounds = 1;
    check(audio.add_sound_player(config, id) == a::Error::none,
          "smaller registration still fits after atomic setup failure");
    a::SoundId sound{};
    audio.sound_id(f::selected(bank.asset), sound);
    s::SoundHandle handle{9, 9, 9, 9};
    check(audio.player(id).stage_start(sound, {}, handle) == a::Error::unsupported_envelope && handle.generation == 0,
          "unobserved zero-time/zero-level finite decay stays unsupported instead of guessing SDK behavior");
}

// These are authored public B1 envelopes, not original sample payloads. The
// admitted32 lifecycles qualify the three archetypes and the literal us clocks.
// Public Syn volume is perceptual input to the native square, not DSP Gain15.
struct FiniteEnvelope {
    std::int32_t attack, decay, release;
    std::uint8_t decay_volume;
    std::uint16_t volume, attack_public, decay_public;
    std::array<std::uint32_t, 3> attack_end, decay_time, release_time;
};
constexpr std::array<float, 3> finite_pitches{0.5f, 1.0f, 1.5f};
constexpr std::array<FiniteEnvelope, 3> finite_envelopes{{
    {364000, 900000, 20000, 127, 30479, 30240, 30240,
     {728000, 364000, 242666}, {1800000, 900000, 600000}, {40000, 20000, 13333}},
    {0, 47120, 6000, 127, 25399, 25200, 25200,
     {0, 0, 0}, {94240, 47120, 31413}, {12000, 6000, 4000}},
    {0, 2000000, 20000, 30, 20319, 20160, 4762,
     {0, 0, 0}, {4000000, 2000000, 1333333}, {40000, 20000, 13333}}
}};

std::uint16_t squared_public_gain(std::uint16_t volume) {
    return static_cast<std::uint16_t>((static_cast<std::uint32_t>(volume) * volume) >> 15);
}

void finite_next_callback(ClientFixture& fixture) {
    auto& context = *fixture.context;
    const auto previous_us = checkpoint(context, context.live, fixture.player).client_us;
    const auto previous_raw = context.state().clients[fixture.player.slot].next_raw_due;
    context.write_address = a::AudioService::command_address;
    next_callback(context, fixture.player);
    const auto next_us = checkpoint(context, context.live, fixture.player).client_us;
    check(next_us > previous_us &&
          context.state().clients[fixture.player.slot].next_raw_due ==
              previous_raw + d::rounded_frames(static_cast<std::int32_t>(next_us - previous_us), 32006),
          "finite phase callbacks retain per-delta raw rounding, not absolute-time conversion or DSP flooring");
}

bool finite_before(ClientFixture& fixture, std::uint64_t at_us) {
    auto& context = *fixture.context;
    auto& player = checkpoint(context, context.live, fixture.player);
    for (unsigned count = 0; player.client_us < at_us && count < 1024; ++count)
        finite_next_callback(fixture);
    check(player.client_us == at_us, "measured finite phase/control deadline is an actual callback");
    return player.client_us == at_us;
}

void finite_gain_commands(ClientFixture& fixture, std::uint16_t public_volume, std::uint32_t time_us) {
    auto& context = *fixture.context;
    auto& state = context.state();
    const auto& player = checkpoint(context, context.live, fixture.player);
    const auto physical = state.logical[player.sounds[0].descriptor.slot].physical;
    if (physical == a::no_slot) { check(false, "finite gain has a bound source"); return; }
    const auto& source = state.physical[physical].current;
    const auto gain = squared_public_gain(public_volume);
    const auto frames = d::duration_frames(static_cast<std::int32_t>(time_us), 32006);
    check(source.parameters.gain.value == gain && source.gain_frames == frames,
          "finite phase squares its public target once and lowers its own pitched DSP duration");
    std::array<std::uint16_t, 2> targets{
        static_cast<std::uint16_t>((gain * context.coefficients.pan[source.parameters.pan]) >> 15),
        static_cast<std::uint16_t>((gain * context.coefficients.pan[127u - source.parameters.pan]) >> 15)};
    const auto from = source.retarget_current;
    context.write_address = a::AudioService::command_address;
    check(d::render(context, state, 16, 0, 16) == a::Error::none,
          "finite gain emits actual ABI1 commands through the shared renderer");
    std::array<bool, 2> found{};
    for (auto address = a::AudioService::command_address; address < context.write_address; address += 8)
        for (std::size_t side = 0; side != found.size(); ++side)
            if (context.memory[address / 4] == (0x09000000u | (side == 0 ? 0x00020000u : 0u) | targets[side]) &&
                context.memory[address / 4 + 1] ==
                    d::envelope_rate(from[side], targets[side], frames, context.coefficients))
                found[side] = true;
    check(found[0] && found[1], "both emitted finite envelope channels use the expected target and scaled rate");
}

void measured_finite_natural_lifecycles() {
    for (const auto& envelope : finite_envelopes) {
        for (std::size_t pitch = 0; pitch != finite_pitches.size(); ++pitch) {
            ClientFixture fixture(envelope.attack, 1, envelope.decay, envelope.release, envelope.decay_volume);
            if (!fixture.bank) return;
            const auto handle = fixture.start(0, {.volume = envelope.volume, .pan = 0, .pitch = finite_pitches[pitch]});
            auto& context = *fixture.context;
            auto& state = context.state();
            auto& player = checkpoint(context, context.live, fixture.player);
            auto& sound = player.sounds[0];
            allocation_attempts = 0;
            count_allocations = true;
            const auto started = d::dispatch_client(context, context.live, fixture.player.slot);
            count_allocations = false;
            check(started == a::Error::none && allocation_attempts == 0,
                  "finite startup uses the existing producer, fixed event heap and checkpoint storage");
            if (started != a::Error::none) return;
            const auto physical = state.logical[sound.descriptor.slot].physical;
            if (physical == a::no_slot) { check(false, "natural finite source binds"); return; }
            const auto source_epoch = state.physical[physical].current.epoch;
            if (envelope.attack != 0) {
                check(sound.phase == d::SoundPhase::attack && sound.event_us == envelope.attack_end[pitch],
                      "real383 attack callback scales with pitch while its DSP duration remains authored");
                finite_gain_commands(fixture, envelope.attack_public, static_cast<std::uint32_t>(envelope.attack));
                if (!finite_before(fixture, envelope.attack_end[pitch])) return;
                finite_next_callback(fixture);
            } else {
                check(state.next_parameter_sequence == 3,
                      "real1/22 zero attack publishes Start, explicit startup gain and decay in the SAME callback");
            }
            const auto release_at = envelope.attack_end[pitch] + envelope.decay_time[pitch];
            check(sound.phase == d::SoundPhase::decay && sound.event_us == release_at &&
                  state.physical[physical].current.epoch == source_epoch,
                  "finite decay owns its endpoint without a new source or resampler initialization");
            finite_gain_commands(fixture, envelope.decay_public, envelope.decay_time[pitch]);
            if (!finite_before(fixture, release_at)) return;
            finite_next_callback(fixture);
            const auto free_at = release_at + envelope.release_time[pitch];
            check(sound.phase == d::SoundPhase::release && sound.event_us == free_at &&
                  sound.generation == handle.generation && state.logical[sound.descriptor.slot].physical == physical,
                  "natural decay endpoint starts a real release and retains its playback/binding through the tail");
            finite_gain_commands(fixture, 0, envelope.release_time[pitch]);
            if (!finite_before(fixture, free_at)) return;
            finite_next_callback(fixture);
            check(sound.phase == d::SoundPhase::free && !sound.published && sound.event_us == 0 &&
                  sound.generation == handle.generation && state.logical[sound.descriptor.slot].physical == a::no_slot &&
                  !state.physical[physical].current.active && !state.physical[physical].allocated,
                  "natural release endpoint performs BOTH source stop and physical free, not just logical inactivity");
        }
    }
}

enum class FiniteControl { game_volume, direct_volume, pitch, fx, triple_fx_pan, stop };

void measured_finite_decay_controls() {
    for (const bool low_decay : {false, true}) {
        const auto& envelope = finite_envelopes[low_decay ? 2 : 0];
        for (std::size_t pitch = 0; pitch != finite_pitches.size(); ++pitch) {
            if (low_decay && pitch != 1) continue;
            for (const auto action : {FiniteControl::game_volume, FiniteControl::direct_volume, FiniteControl::pitch,
                                      FiniteControl::fx, FiniteControl::triple_fx_pan, FiniteControl::stop}) {
                if (low_decay && action == FiniteControl::triple_fx_pan) continue;
                ClientFixture fixture(envelope.attack, 1, envelope.decay, envelope.release, envelope.decay_volume);
                if (!fixture.bank) return;
                const auto handle = fixture.start(0, {.volume = envelope.volume, .pan = 0, .pitch = finite_pitches[pitch]});
                auto& context = *fixture.context;
                auto& state = context.state();
                auto& player = checkpoint(context, context.live, fixture.player);
                auto& sound = player.sounds[0];
                check(d::dispatch_client(context, context.live, fixture.player.slot) == a::Error::none,
                      "measured finite control lifecycle starts");
                std::uint64_t at_us = low_decay ? 992000u :
                    std::array<std::uint32_t, 3>{1632000, 816000, 544000}[pitch];
                if ((action == FiniteControl::fx && (low_decay || pitch != 1)) ||
                    (action == FiniteControl::stop && (low_decay || pitch == 2)))
                    at_us += 16000;
                if (!finite_before(fixture, at_us)) return;
                const auto physical = state.logical[sound.descriptor.slot].physical;
                if (physical == a::no_slot) { check(false, "finite control retains binding"); return; }
                auto& source = state.physical[physical].current;
                const auto source_epoch = source.epoch;
                const auto decay_end = sound.event_us;
                const auto authored_frames = source.gain_frames;
                const auto elapsed_at_callback = std::min(source.gain_frames, source.gain_elapsed +
                    static_cast<std::uint32_t>((state.clients[fixture.player.slot].next_raw_due &
                                               ~std::uint64_t{15}) - state.sample_clock));
                const auto sequence = state.next_parameter_sequence;
                const auto new_pitch = low_decay || pitch == 0 ? 1.5f : 0.5f;
                std::uint16_t target_public = envelope.decay_public;
                std::uint64_t expected_controls = 1;
                switch (action) {
                case FiniteControl::game_volume:
                    control(player, handle, d::PlayerEventKind::volume,
                            {.volume = static_cast<std::uint16_t>(low_decay ? 10160 : 15240)});
                    target_public = low_decay ? 2381 : 15120;
                    break;
                case FiniteControl::direct_volume:
                    control(player, handle, d::PlayerEventKind::volume, {.volume = 16000});
                    target_public = low_decay ? 3750 : 15875;
                    break;
                case FiniteControl::pitch:
                    control(player, handle, d::PlayerEventKind::pitch, {.pitch = new_pitch});
                    break;
                case FiniteControl::fx:
                    control(player, handle, d::PlayerEventKind::fx, {.fx_mix = 64});
                    break;
                case FiniteControl::triple_fx_pan:
                    for (unsigned repeat = 0; repeat != 3; ++repeat)
                        control(player, handle, d::PlayerEventKind::fx, {.fx_mix = 64});
                    control(player, handle, d::PlayerEventKind::pan, {.pan = 127});
                    expected_controls = 4;
                    break;
                case FiniteControl::stop:
                    control(player, handle, d::PlayerEventKind::stop);
                    break;
                }
                finite_next_callback(fixture);
                check(state.next_parameter_sequence == sequence + expected_controls && source.epoch == source_epoch,
                      "each measured normal/repeated control is forwarded once without recreating the source");
                const auto release_us = action == FiniteControl::pitch ?
                    (new_pitch == 0.5f ? 40000u : 13333u) : envelope.release_time[pitch];
                const auto release_at = action == FiniteControl::stop ? at_us : decay_end;
                if (action != FiniteControl::stop) {
                    check(sound.phase == d::SoundPhase::decay && sound.event_us == decay_end,
                          "Pitch, Volume and single/triple FX/Pan never move an existing finite decay endpoint");
                    if (action == FiniteControl::game_volume || action == FiniteControl::direct_volume) {
                        finite_gain_commands(fixture, target_public, 1000);
                    } else {
                        check(source.gain_frames == authored_frames && source.gain_elapsed == elapsed_at_callback &&
                              source.parameters.gain.value == squared_public_gain(target_public),
                              "Pitch and repeated FX/Pan preserve decay gain target, elapsed progress and full authored ramp");
                    }
                    if (!finite_before(fixture, release_at)) return;
                    finite_next_callback(fixture);
                }
                check(sound.phase == d::SoundPhase::release && sound.event_us == release_at + release_us,
                      "explicit/natural release uses the latest pitch, not a rewritten decay duration");
                finite_gain_commands(fixture, 0, release_us);
                if (!finite_before(fixture, release_at + release_us)) return;
                finite_next_callback(fixture);
                check(sound.phase == d::SoundPhase::free && !source.active &&
                      state.logical[sound.descriptor.slot].physical == a::no_slot,
                      "all23 measured during-decay control profiles reach source stop and logical deallocation");
                if (action == FiniteControl::stop) {
                    for (unsigned count = 0; player.client_us <= decay_end && count < 1024; ++count)
                        finite_next_callback(fixture);
                    check(sound.phase == d::SoundPhase::free && !source.active,
                          "an explicit stop cancels the former natural decay deadline through its idle tail");
                }
            }
        }
    }
}

void finite_public_retirement_reuse_and_reset() {
    f::Fixture fixture;
    author_envelope(fixture, 0, 47120, 6000);
    auto bank = fixture.decode();
    if (!bank) { check(false, "real1 authored public fixture hydrates"); return; }
    auto made = service(bank.asset, 1, 1);
    if (!made) { check(false, "finite public service creates"); return; }
    auto& audio = *made.service;
    a::SoundId sound{};
    check(audio.sound_id(f::selected(bank.asset), sound) == a::Error::none, "finite public resource resolves");
    s::Config config{};
    config.max_sounds = 1;
    config.first_due = a::RawSampleTime{7};
    a::PlayerId id{};
    check(audio.add_sound_player(config, id) == a::Error::none, "finite public player registers");
    auto player = audio.player(id);
    s::SoundHandle old{};
    check(player.stage_start(sound, {.volume = 25399, .pan = 0, .pitch = 1.5f}, old) == a::Error::none &&
          player.stage() == a::Error::none, "previously unsupported real1 finite sound publishes through unchanged API");
    s::Status initial{}, final{};
    bool saw_bound = false;
    for (std::uint64_t serial = 1; serial <= 256 && player.active(old); ++serial) {
        a::TaskView task{};
        allocation_attempts = 0;
        count_allocations = true;
        const auto error = audio.build({serial, 0, 16}, task);
        count_allocations = false;
        check(error == a::Error::none && allocation_attempts == 0,
              "finite public startup, phase changes and natural retirement build without allocation");
        if (error != a::Error::none) return;
        check(player.status(old, final) == a::Error::none, "finite status remains queryable through natural release");
        if (final.bound && !saw_bound) { initial = final; saw_bound = true; }
        retire(audio, task);
    }
    check(saw_bound && !final.active && !final.bound && final.error == a::Error::none &&
          !player.active(old) && player.stage_volume(old, 16000) == a::Error::stale_handle,
          "natural finite completion exposes inactive/unbound status and rejects controls without explicit stop");
    s::SoundHandle fresh{};
    check(player.stage_start(sound, {}, fresh) == a::Error::none && fresh.slot == old.slot &&
          fresh.generation > old.generation, "natural retirement returns the only playback slot with a fresh generation");
    s::Status reserved{};
    check(player.status(fresh, reserved) == a::Error::none && reserved.active && !reserved.bound &&
          reserved.voice.slot == initial.voice.slot && reserved.voice.generation == initial.voice.generation &&
          player.stage_pitch(old, 0.5f) == a::Error::stale_handle,
          "finite reuse keeps the setup descriptor but never revives old playback authority");
    check(audio.reset() == a::Error::none && player.stage_stop(fresh) == a::Error::stale_handle &&
          player.status(old, final) == a::Error::stale_handle,
          "reset invalidates both naturally completed and reserved finite playback identities");
}

void finite_unchanged_controls_and_release_volume() {
    ClientFixture fixture(0, 1, 2000000, 20000, 30);
    if (!fixture.bank) return;
    const auto handle = fixture.start(0, {.volume = 20319, .pan = 0});
    auto& context = *fixture.context;
    auto& state = context.state();
    auto& player = checkpoint(context, context.live, fixture.player);
    auto& sound = player.sounds[0];
    check(d::dispatch_client(context, context.live, fixture.player.slot) == a::Error::none,
          "finite unchanged-control source starts");
    const auto physical = state.logical[sound.descriptor.slot].physical;
    if (physical == a::no_slot) return;
    const auto sequence = state.next_parameter_sequence;
    control(player, handle, d::PlayerEventKind::pitch, {.pitch = 1.0f});
    control(player, handle, d::PlayerEventKind::pan, {.pan = 0});
    for (unsigned repeat = 0; repeat != 3; ++repeat)
        control(player, handle, d::PlayerEventKind::fx, {.fx_mix = 0});
    control(player, handle, d::PlayerEventKind::volume, {.volume = 20319});
    finite_next_callback(fixture);
    check(state.next_parameter_sequence == sequence + 6 && sound.event_us == 2000000 &&
          state.physical[physical].current.gain_elapsed == 0,
          "unchanged finite Pitch/Pan/FX/Volume are all forwarded; Volume restarts only its one-ms DSP ramp");
    finite_gain_commands(fixture, 4762, 1000);
    if (!finite_before(fixture, 2000000)) return;
    finite_next_callback(fixture);
    control(player, handle, d::PlayerEventKind::volume, {.volume = 16000});
    control(player, handle, d::PlayerEventKind::pitch, {.pitch = 0.5f});
    control(player, handle, d::PlayerEventKind::fx, {.fx_mix = 64});
    control(player, handle, d::PlayerEventKind::stop);
    finite_next_callback(fixture);
    check(sound.phase == d::SoundPhase::release && sound.event_us == 2020000,
          "controls and repeated stop during release cannot extend the existing natural retirement endpoint");
    finite_gain_commands(fixture, 0, 1000);
    if (!finite_before(fixture, 2020000)) return;
    finite_next_callback(fixture);
    check(sound.phase == d::SoundPhase::free && !state.physical[physical].current.active,
          "release-phase Volume stays zero rather than reviving the decay or attack target");
}

void finite_unbound_and_deferred_source_epochs() {
    {
        ClientFixture fixture(0, 1, 47120, 6000);
        if (!fixture.bank) return;
        auto& context = *fixture.context;
        auto& state = context.state();
        a::VoiceHandle blocker{};
        a::Allocation allocation{};
        check(d::create_voice(context, state, blocker) == a::Error::none &&
              d::allocate_voice(context, state, blocker, 100, {0}, allocation) == a::Error::none &&
              d::start_voice(context, state, blocker, {context.owner, 0}, {1.0f, {12345}, 0}, {0}, {0}) == a::Error::none,
              "finite failed-allocation fixture fills the physical pool");
        const auto handle = fixture.start();
        auto& player = checkpoint(context, context.live, fixture.player);
        auto& sound = player.sounds[0];
        check(d::dispatch_client(context, context.live, fixture.player.slot) == a::Error::none &&
              sound.phase == d::SoundPhase::decay && sound.event_us == 47120 && sound.error == a::Error::unbound,
              "unbound zero-attack finite start still enters its scheduled decay in the same callback");
        check(d::stop_voice(context, state, blocker, {0}) == a::Error::none &&
              d::free_voice(context, state, blocker, {0}) == a::Error::none, "blocked physical capacity later becomes free");
        control(player, handle, d::PlayerEventKind::volume, {.volume = 16000});
        control(player, handle, d::PlayerEventKind::pitch, {.pitch = 1.5f});
        finite_next_callback(fixture);
        check(sound.parameters.volume == 16000 && sound.parameters.pitch == 1.5f && sound.event_us == 47120 &&
              state.logical[sound.descriptor.slot].physical == a::no_slot,
              "unbound finite controls retain logical parameters but neither retry allocation nor retime decay");
        if (!finite_before(fixture, 47120)) return;
        finite_next_callback(fixture);
        check(sound.phase == d::SoundPhase::release && sound.event_us == 51120,
              "failed physical allocation still naturally releases using its latest accepted pitch");
        if (!finite_before(fixture, 51120)) return;
        finite_next_callback(fixture);
        check(sound.phase == d::SoundPhase::free && sound.error == a::Error::unbound &&
              state.logical[sound.descriptor.slot].physical == a::no_slot,
              "failed-allocation finite playback naturally retires without touching a physical source");
    }
    {
        ClientFixture fixture(0, 1, 47120, 6000);
        if (!fixture.bank) return;
        auto& context = *fixture.context;
        auto& state = context.state();
        fixture.start(0);
        auto& player = checkpoint(context, context.live, fixture.player);
        check(d::dispatch_client(context, context.live, fixture.player.slot) == a::Error::none,
              "finite source-epoch fixture starts its original owner");
        const auto original_epoch = state.physical[0].current.epoch;
        fixture.start(1, {.volume = 16000, .pan = 127});
        finite_next_callback(fixture);
        check(state.physical[0].pending.valid && state.physical[0].current.epoch == original_epoch,
              "stolen finite startup uses the existing deferred source restart rather than replacing the old tail early");
        finite_next_callback(fixture);
        const auto replacement_epoch = state.physical[0].current.epoch;
        check(replacement_epoch != original_epoch && !state.physical[0].pending.valid &&
              state.physical[0].current.parameters.gain.value == squared_public_gain(15875) &&
              state.physical[0].current.gain_frames == d::duration_frames(47120, 32006),
              "same-callback finite decay follows the pending replacement's source epoch and survives its actual restart");
        if (!finite_before(fixture, 47120)) return;
        finite_next_callback(fixture);
        if (!finite_before(fixture, 53120)) return;
        finite_next_callback(fixture);
        check(player.sounds[0].phase == d::SoundPhase::free && player.sounds[0].error == a::Error::unbound &&
              player.sounds[1].phase == d::SoundPhase::decay && state.physical[0].current.active &&
              state.physical[0].current.epoch == replacement_epoch &&
              state.physical[0].current.parameters.gain.value == squared_public_gain(15875),
              "stolen sound's natural release/stop/free cannot target its replacement binding or source epoch");
    }
}

void finite_phase_failures_are_transactional() {
    for (unsigned phase = 0; phase != 4; ++phase) {
        ClientFixture fixture(phase == 0 ? 0 : 10000, 1, 12000, 6000);
        if (!fixture.bank) return;
        fixture.start();
        auto& context = *fixture.context;
        auto& state = context.state();
        auto& player = checkpoint(context, context.live, fixture.player);
        if (phase != 0) {
            check(d::dispatch_client(context, context.live, fixture.player.slot) == a::Error::none,
                  "finite transactional fixture starts");
            const auto deadline = phase == 1 ? 10000u : phase == 2 ? 22000u : 28000u;
            if (!finite_before(fixture, deadline)) return;
            const auto due = state.clients[fixture.player.slot].next_raw_due & ~std::uint64_t{15};
            const auto frames = static_cast<std::uint32_t>(due - state.sample_clock);
            context.write_address = a::AudioService::command_address;
            if (frames != 0)
                check(d::render(context, state, frames, 0, frames) == a::Error::none,
                      "finite failure injection reaches its real DSP boundary");
            check(d::advance_parameters(context, state, {due}) == a::Error::none,
                  "finite failure injection applies preceding source events");
        }
        const auto saved_phase = player.sounds[0].phase;
        const auto saved_event = player.sounds[0].event_us;
        const auto saved_publication = player.published_count;
        const auto saved_binding = state.logical[player.sounds[0].descriptor.slot].physical;
        const auto saved_due = state.clients[fixture.player.slot].next_raw_due;
        const auto bank = d::prepare_shadow(context);
        context.cpu[bank].next_parameter_sequence =
            std::numeric_limits<std::uint64_t>::max() - (phase == 0 ? 2u : 0u);
        allocation_attempts = 0;
        count_allocations = true;
        const auto error = d::dispatch_client(context, bank, fixture.player.slot);
        count_allocations = false;
        check(error == a::Error::identity_exhausted && allocation_attempts == 0 &&
              player.sounds[0].phase == saved_phase && player.sounds[0].event_us == saved_event &&
              player.published_count == saved_publication &&
              state.logical[player.sounds[0].descriptor.slot].physical == saved_binding &&
              state.clients[fixture.player.slot].next_raw_due == saved_due,
              "failed finite startup/decay/release/retirement lowering commits no live phase, source, queue or clock");
        finite_next_callback(fixture);
        check(player.sounds[0].phase != saved_phase,
              "discarding a failed finite shadow permits a clean retry of the original published transition");
    }
}

void finite_duration_preflight_preserves_publication() {
    for (unsigned large_phase = 0; large_phase != 3; ++large_phase) {
        f::Fixture fixture;
        constexpr auto large = std::numeric_limits<std::int32_t>::max() - 255; // Exactly representable in float.
        author_envelope(fixture, large_phase == 0 ? large : 0,
                        large_phase == 1 ? large : 47120, large_phase == 2 ? large : 6000);
        auto bank = fixture.decode();
        if (!bank) { check(false, "finite duration boundary fixture hydrates"); return; }
        auto made = service(bank.asset, 1, 1);
        if (!made) { check(false, "finite duration boundary service creates"); return; }
        auto& audio = *made.service;
        a::SoundId sound{};
        audio.sound_id(f::selected(bank.asset), sound);
        s::Config config{};
        config.max_sounds = 1;
        a::PlayerId id{};
        check(audio.add_sound_player(config, id) == a::Error::none, "finite duration boundary player registers");
        auto player = audio.player(id);
        s::SoundHandle rejected{9, 9, 9, 9}, accepted{};
        check(player.stage_start(sound, {.pitch = 0.5f}, rejected) == a::Error::invalid_parameter &&
              rejected.generation == 0 && player.stage_start(sound, {}, accepted) == a::Error::none &&
              player.stage_pitch(accepted, 0.5f) == a::Error::invalid_parameter && player.active(accepted),
              "start and Pitch preflight every required scaled finite phase without consuming playback/event capacity");
        for (std::size_t entry = 1; entry != s::SoundPlayer::event_capacity; ++entry)
            check(player.stage_pan(accepted, 64) == a::Error::none,
                  "rejected scaled duration consumes no bounded inbox entry");
        check(player.stage() == a::Error::none && player.stage_pan(accepted, 0) == a::Error::none,
              "full valid publication still fits after rejected finite duration operations");
    }
}

void finite_deadline_overflow_is_shadow_local() {
    for (const bool releasing : {false, true}) {
        ClientFixture fixture(10000, 1, 12000, 6000);
        if (!fixture.bank) return;
        fixture.start();
        auto& context = *fixture.context;
        auto& state = context.state();
        auto& player = checkpoint(context, context.live, fixture.player);
        check(d::dispatch_client(context, context.live, fixture.player.slot) == a::Error::none,
              "finite deadline overflow fixture starts a bound source");
        auto& sound = player.sounds[0];
        sound.phase = releasing ? d::SoundPhase::decay : d::SoundPhase::attack;
        player.client_us = sound.event_us = std::numeric_limits<std::uint64_t>::max() - 4000;
        player.poll_us = std::numeric_limits<std::uint64_t>::max();
        state.clients[fixture.player.slot].next_raw_due = state.sample_clock;
        const auto physical = state.logical[sound.descriptor.slot].physical;
        const auto sequence = state.next_parameter_sequence;
        const auto bank = d::prepare_shadow(context);
        check(d::dispatch_client(context, bank, fixture.player.slot) == a::Error::identity_exhausted &&
              sound.phase == (releasing ? d::SoundPhase::decay : d::SoundPhase::attack) &&
              sound.event_us == player.client_us && state.next_parameter_sequence == sequence &&
              state.logical[sound.descriptor.slot].physical == physical &&
              state.physical[physical].current.active,
              "overflowing finite decay/release deadline publishes no wrapped clock, phase or gain change");
    }
}

void finite_attack_volume_retains_phase_and_endpoint() {
    ClientFixture fixture(364000, 1, 900000, 20000);
    if (!fixture.bank) return;
    const auto handle = fixture.start(0, {.volume = 30479, .pan = 0, .pitch = 0.5f});
    auto& context = *fixture.context;
    auto& player = checkpoint(context, context.live, fixture.player);
    check(d::dispatch_client(context, context.live, fixture.player.slot) == a::Error::none,
          "real383 long attack starts before a phase-specific volume control");
    control(player, handle, d::PlayerEventKind::volume, {.volume = 16000});
    finite_next_callback(fixture);
    check(player.sounds[0].phase == d::SoundPhase::attack && player.sounds[0].event_us == 728000,
          "Volume during attack uses its attack level without moving the original pitch-scaled callback");
    finite_gain_commands(fixture, 15875, 1000);
    if (!finite_before(fixture, 728000)) return;
    finite_next_callback(fixture);
    check(player.sounds[0].phase == d::SoundPhase::decay && player.sounds[0].event_us == 2528000,
          "finite decay still starts at the authored attack callback after an attack gain retarget");
    finite_gain_commands(fixture, 15875, 1800000);
}

} // namespace

int main() {
    registration_and_reservations();
    staged_publication_and_lease();
    registration_storage_failure_preserves_existing_client();
    multi_client_failure_retry_and_reset();
    bounded_publication();
    shadow_isolation_and_envelope_calendar();
    attack_dsp_time_is_not_pitch_scaled();
    measured_posted_start_order();
    gain_retarget_and_stolen_controls();
    unchanged_volume_restarts_the_live_envelope();
    measured_failed_allocation_lifecycle();
    failed_callback_shadow_isolation();
    configured_start_second_step_failure_is_shadow_local();
    sequential_controls_and_earliest_envelope();
    unsupported_envelopes_and_setup_rollback();
    unsupported_envelopes_and_setup_rollback(true);
    playback_reuse_keeps_descriptor_not_generation();
    bounded_calendar_overflow();
    measured_finite_natural_lifecycles();
    measured_finite_decay_controls();
    finite_public_retirement_reuse_and_reset();
    finite_unchanged_controls_and_release_volume();
    finite_unbound_and_deferred_source_epochs();
    finite_phase_failures_are_transactional();
    finite_duration_preflight_preserves_publication();
    finite_deadline_overflow_is_shadow_local();
    finite_attack_volume_retains_phase_and_endpoint();
    if (failures) return EXIT_FAILURE;
    std::cout << "SoundPlayer shared descriptors, finite/indefinite phases, staged publication, callback calendar and rollback checks passed\n";
    return EXIT_SUCCESS;
}
