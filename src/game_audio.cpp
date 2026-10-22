#include "game_audio.hpp"
#include "audio_effects_internal.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <new>
#include <utility>

namespace isle::game_audio {
namespace {
std::uint16_t be16(std::span<const std::byte> b, std::size_t p) noexcept {
    return static_cast<std::uint16_t>((std::to_integer<unsigned>(b[p]) << 8) | std::to_integer<unsigned>(b[p + 1]));
}
std::uint32_t be32(std::span<const std::byte> b, std::size_t p) noexcept {
    return (static_cast<std::uint32_t>(be16(b, p)) << 16) | be16(b, p + 2);
}
a::ByteOwner copy(std::span<const std::byte> b, std::size_t first, std::size_t end) {
    return std::make_shared<a::Bytes>(b.begin() + first, b.begin() + end);
}
bool valid(Parameters p) noexcept {
    return p.volume <= 32767 && p.pan <= 127 && p.reverb <= 127 && std::isfinite(p.pitch) &&
        p.pitch >= 1.0f / 32768.0f;
}
}
ResourceResult hydrate(const rom::Snapshot& snapshot) noexcept {
    ResourceResult result;
    const auto bytes = snapshot.canonical();
    if (bytes.size() != rom::us_size) { result.error = a::Error::invalid_resource; return result; }
    try {
        auto sounds = a::hydrate_bank(copy(bytes, 0xba6c20, 0xbb6940), copy(bytes, 0xbb6940, 0xf53540));
        auto music = a::hydrate_bank(copy(bytes, 0xafeee0, 0xb04430), copy(bytes, 0xb04430, 0xba6c20));
        auto sequences = a::hydrate_sequences(copy(bytes, 0xaefc10, 0xafeee0));
        if (!sounds || !music || !sequences) { result.error = a::Error::invalid_resource; return result; }
        auto& r = result.resources;
        r.sounds = std::move(sounds.asset); r.music = std::move(music.asset);
        r.sequences = std::move(sequences.asset);
        for (std::size_t i = 0; i < r.coefficients.pan.size(); ++i) r.coefficients.pan[i] = be16(bytes, 0x436b0 + i * 2);
        for (std::size_t i = 0; i < r.coefficients.logarithm.size(); ++i) {
            const auto p = 0x437b0 + i * 8;
            const auto bits = (static_cast<std::uint64_t>(be32(bytes, p)) << 32) | be32(bytes, p + 4);
            r.coefficients.logarithm[i] = std::bit_cast<double>(bits);
        }
        r.priorities.resize(400);
        for (std::size_t i = 0; i < r.priorities.size(); ++i) r.priorities[i] = std::to_integer<std::uint8_t>(bytes[0x42e28 + i]);
        for (std::size_t i = 0; i < r.startup_effect_words.size(); ++i)
            r.startup_effect_words[i] = std::bit_cast<std::int32_t>(be32(bytes, 0x42b50 + i * 4));
        return result;
    } catch (const std::bad_alloc&) { return {a::Error::allocation_failure, {}}; }
    catch (...) { return {a::Error::invalid_resource, {}}; }
}
a::Error decode_startup_poles(std::span<const std::byte> wire,
                              std::array<a::effects::PoleFilter, 3>& destination) noexcept {
    if (wire.size() != 114) return a::Error::invalid_resource;
    std::array<a::effects::PoleFilter, 3> result{};
    for (std::size_t i = 0; i < result.size(); ++i) {
        const auto at = i * 38;
        if (be32(wire, at) != 0x5000u + i * 0x1000u) return a::Error::invalid_resource;
        result[i].gain = be16(wire, at + 4);
        for (std::size_t j = 0; j < result[i].coefficients.size(); ++j)
            result[i].coefficients[j] = std::bit_cast<std::int16_t>(be16(wire, at + 6 + j * 2));
    }
    destination = result;
    return a::Error::none;
}
a::Error startup_effects(const Resources& r, const std::array<a::effects::PoleFilter, 3>& poles,
                         std::array<a::effects::Section, 8>& destination, a::effects::Config& config) noexcept {
    config = {};
    if (r.startup_effect_words[0] != 8 || r.startup_effect_words[1] != 0x28a0) return a::Error::invalid_resource;
    std::array<a::effects::Section, 8> sections{};
    for (std::size_t i = 0; i < sections.size(); ++i) {
        const auto* w = r.startup_effect_words.data() + 2 + i * 8;
        if (w[0] < 0 || w[1] < 0 || w[5] != 0 || w[6] != 0 ||
            w[2] < -32768 || w[2] > 32767 || w[3] < -32768 || w[3] > 32767 ||
            w[4] < -32768 || w[4] > 32767) return a::Error::invalid_resource;
        auto& s = sections[i];
        s.input_samples = static_cast<std::uint32_t>(w[0]); s.output_samples = static_cast<std::uint32_t>(w[1]);
        s.feedback = static_cast<std::int16_t>(w[2]); s.feedforward = static_cast<std::int16_t>(w[3]);
        s.gain = static_cast<std::int16_t>(w[4]);
        if (w[7] != 0) {
            if (w[7] < 0x5000 || w[7] > 0x7000 || (w[7] & 0xfff) != 0) return a::Error::invalid_resource;
            s.pole = poles[static_cast<std::size_t>((w[7] - 0x5000) / 0x1000)];
        }
    }
    destination = sections;
    config = {a::effects::Routing::network, 0x28a0, destination};
    return a::Error::none;
}

a::Error native_startup_effects(const Resources& resources,
    std::array<a::effects::Section,8>& sections, a::effects::Config& config) noexcept {
    std::array<a::effects::PoleFilter,3> poles{};
    for (std::size_t i = 0; i < poles.size(); ++i) {
        const auto parameter = 0x5000u + static_cast<std::uint32_t>(i) * 0x1000u;
        const float a = static_cast<float>(parameter) / 32768.0f;
        auto& filter = poles[i];
        filter.gain = static_cast<std::uint16_t>((32768u-parameter)/2u);
        float coefficient = 16384.0f;
        for (std::size_t sample = 8; sample < filter.coefficients.size(); ++sample) {
            coefficient *= a;
            filter.coefficients[sample] = static_cast<std::int16_t>(coefficient);
        }
    }
    return startup_effects(resources,poles,sections,config);
}

namespace {
bool is_custom_program(const Resources& resources, a::effects::Config candidate) noexcept {
    std::array<a::effects::Section, 8> sections{};
    a::effects::Config expected;
    if (native_startup_effects(resources, sections, expected) != a::Error::none ||
        candidate.routing != expected.routing || candidate.ring_samples != expected.ring_samples ||
        candidate.sections.size() != expected.sections.size()) return false;
    for (std::size_t i = 0; i < sections.size(); ++i) {
        const auto& x = candidate.sections[i];
        const auto& y = sections[i];
        if (x.input_samples != y.input_samples || x.output_samples != y.output_samples ||
            x.feedback != y.feedback || x.feedforward != y.feedforward || x.gain != y.gain ||
            x.pole.has_value() != y.pole.has_value() ||
            x.chorus.has_value() != y.chorus.has_value()) return false;
        if (x.pole && (x.pole->gain != y.pole->gain ||
                       x.pole->coefficients != y.pole->coefficients)) return false;
        if (x.chorus && (x.chorus->rate != y.chorus->rate ||
                         x.chorus->depth != y.chorus->depth)) return false;
    }
    return true;
}
}

Session::Session(Resources resources, Output output, Executor executor) noexcept
    : resources_(std::move(resources)), output_(output), executor_(executor) {}
Session::~Session() {
    if (executor_started_) executor_.retire(executor_.owner);
}
CreateResult Session::create(Resources resources, a::effects::Config effects, Output output, Executor executor) noexcept {
    if (!output.owner || !output.inspect || !output.submit || !output.finish || !output.poll ||
        !executor.owner || !executor.initialize || !executor.ready || !executor.retire)
        return {a::Error::invalid_parameter, nullptr};
    const auto info = output.inspect(output.owner);
    if (info.state != Output::State::ready || info.rate != source_rate)
        return {a::Error::invalid_state, nullptr};
    if (!resources.sounds || !resources.music || !resources.sequences ||
        resources.sounds.banks().empty() || !resources.sounds.banks()[0] ||
        resources.sounds.banks()[0]->instruments.empty() || !resources.sounds.banks()[0]->instruments[0] ||
        resources.priorities.size() < resources.sounds.banks()[0]->instruments[0]->sounds.size() ||
        effects.routing != a::effects::Routing::network)
        return {a::Error::invalid_resource, nullptr};
    try {
        auto owner = std::unique_ptr<Session>(new Session(std::move(resources), output, executor));
        const std::array banks{owner->resources_.sounds, owner->resources_.music};
        auto service = a::AudioService::create(banks, {source_rate, 22, 42}, owner->resources_.coefficients, effects);
        if (!service) return {service.error, nullptr};
        owner->service_ = std::move(service.service);
        owner->custom_effects_ = is_custom_program(owner->resources_, effects);
        const auto error = owner->register_players();
        if (error != a::Error::none) return {error, nullptr};
        owner->executor_started_ = true;
        if (!executor.initialize(executor.owner, owner->service_->rdram_words()) ||
            !executor.ready(executor.owner, owner->service_->rdram_words()))
            return {a::Error::execution_failed, nullptr};
        // Recheck the checked endpoint after all resource/player admission. A
        // failure never emits a readiness acknowledgement to the session owner.
        const auto checked = output.inspect(output.owner);
        if (checked.state != Output::State::ready || checked.rate != source_rate)
            return {a::Error::invalid_state, nullptr};
        return {a::Error::none, std::move(owner)};
    } catch (const std::bad_alloc&) { return {a::Error::allocation_failure, nullptr}; }
    catch (...) { return {a::Error::invalid_resource, nullptr}; }
}
a::Error Session::register_players() noexcept {
    a::sfx::Config sound;
    sound.max_sounds = sound_slots;
    auto error = service_->add_sound_player(sound, sfx_);
    if (error != a::Error::none) return error;
    for (auto& id : csp_) {
        a::csp::Config music;
        music.bank = resources_.music; music.sequences = resources_.sequences; music.max_notes = 16;
        error = service_->add_sequence_player(music, id);
        if (error != a::Error::none) return error;
    }
    return a::Error::none;
}
a::Error Session::fail(a::Error error) noexcept {
    if (error_ == a::Error::none) error_ = error;
    state_ = State::failed;
    return error;
}
a::Error Session::control() const noexcept {
    return state_ == State::ready && !restarting_ ? a::Error::none : a::Error::invalid_state;
}
a::Error Session::publish(unsigned p, a::Error error) noexcept {
    if (error == a::Error::none) error = service_->sequence_player(csp_[p]).stage();
    return error == a::Error::none ? error : fail(error);
}
a::Error Session::play_song(unsigned p, std::uint32_t song) noexcept {
    if (p >= songs_.size() || song >= resources_.sequences.sequences().size()) return a::Error::invalid_parameter;
    if (control() != a::Error::none) return a::Error::invalid_state;
    songs_[p].requested = static_cast<int>(song); songs_[p].phase = 1;
    return a::Error::none;
}
a::Error Session::stop_song(unsigned p) noexcept {
    if (p >= songs_.size()) return a::Error::invalid_parameter;
    if (state_ == State::failed || state_ == State::draining || state_ == State::drained) return a::Error::invalid_state;
    songs_[p].requested = -1; songs_[p].phase = 1;
    return a::Error::none;
}
a::Error Session::stop_bgm() noexcept {
    for (unsigned p = 0; p < songs_.size(); ++p) { const auto e = stop_song(p); if (e != a::Error::none) return e; }
    return a::Error::none;
}
bool Session::bgm_playing(unsigned p) const noexcept { return p < songs_.size() && songs_[p].phase != 0 && songs_[p].requested >= 0; }
a::Error Session::bgm_volume(unsigned p, std::uint32_t value, std::uint32_t retraces) noexcept {
    if (p >= songs_.size()) return a::Error::invalid_parameter;
    if (control() != a::Error::none) return a::Error::invalid_state;
    auto& s = songs_[p]; value = std::min(value, 0x7f00u);
    if (retraces != 0) { s.volume_ticks = retraces; s.volume_step = (static_cast<float>(value) - s.volume) / static_cast<float>(retraces); return a::Error::none; }
    const auto error = publish(p, service_->sequence_player(csp_[p]).stage_volume(static_cast<std::uint16_t>(value)));
    if (error == a::Error::none) { s.volume = static_cast<float>(value); s.volume_ticks = 0; }
    return error;
}
a::Error Session::bgm_tempo(unsigned p, std::uint32_t value) noexcept {
    if (p >= songs_.size() || control() != a::Error::none) return a::Error::invalid_parameter;
    return publish(p, service_->sequence_player(csp_[p]).stage_tempo(value));
}
a::Error Session::get_bgm_tempo(unsigned p, std::uint32_t& value) const noexcept {
    if (p >= songs_.size()) return a::Error::invalid_parameter;
    return service_->sequence_player(csp_[p]).tempo(value);
}
a::Error Session::bgm_reverb(unsigned p, std::uint32_t value) noexcept {
    if (p >= songs_.size() || control() != a::Error::none) return a::Error::invalid_parameter;
    auto player = service_->sequence_player(csp_[p]);
    for (std::uint8_t channel = 0; channel < 16; ++channel) {
        const auto e = player.stage_channel_fx(channel, static_cast<std::uint8_t>(std::min(value, 127u)));
        if (e != a::Error::none) return fail(e);
    }
    return publish(p, a::Error::none);
}
a::Error Session::bgm_priority(unsigned p, std::uint8_t value) noexcept {
    if (p >= songs_.size() || control() != a::Error::none) return a::Error::invalid_parameter;
    global_priority_ = std::min<std::uint8_t>(value, 127);
    auto player = service_->sequence_player(csp_[p]);
    for (std::uint8_t channel = 0; channel < 16; ++channel) {
        const auto e = player.stage_channel_priority(channel, global_priority_);
        if (e != a::Error::none) return fail(e);
    }
    return publish(p, a::Error::none);
}
a::Error Session::bgm_extra_reverb(unsigned p, std::int8_t value) noexcept {
    if (p >= songs_.size() || control() != a::Error::none) return a::Error::invalid_parameter;
    return publish(p, service_->sequence_player(csp_[p]).stage_player_extra_fx(value));
}
a::Error Session::bgm_channel_volume(unsigned p, std::uint8_t channel, std::int8_t value) noexcept {
    if (p >= songs_.size() || control() != a::Error::none) return a::Error::invalid_parameter;
    return publish(p, service_->sequence_player(csp_[p]).stage_channel_extra_volume(channel, value));
}
bool Session::has_sound(std::uint32_t id) const noexcept {
    const auto& catalog = resources_.sounds.banks()[0]->instruments[0]->sounds;
    return id < catalog.size() && catalog[id] && id < resources_.priorities.size();
}
a::Error Session::play_sound(std::uint32_t id, Parameters parameters, int& slot) noexcept {
    slot = -1;
    if (control() != a::Error::none) return a::Error::invalid_state;
    if (!valid(parameters)) return a::Error::invalid_parameter;
    for (auto& s : sounds_) if (s.idle < 255) ++s.idle;
    // Absent GAME sound IDs are ordinary -1 results, not service failures.
    if (!has_sound(id)) return a::Error::none;
    const auto priority = resources_.priorities[id];
    int selected = -1;
    for (unsigned i = 0; i < sounds_.size(); ++i) {
        if (sounds_[i].handle.generation == 0 && sounds_[i].starting < 0) { selected = static_cast<int>(i); break; }
    }
    if (selected < 0) {
        unsigned best_priority = priority, oldest = 0;
        for (unsigned i = 0; i < sounds_.size(); ++i) {
            const auto& s = sounds_[i];
            if (s.priority < best_priority || (s.priority == best_priority && oldest < s.idle)) {
                selected = static_cast<int>(i); best_priority = s.priority; oldest = s.idle;
            }
        }
    }
    // GAME returns -1 when no slot can be stolen; not a failed service.
    if (selected < 0) return a::Error::none;
    auto& s = sounds_[static_cast<std::size_t>(selected)];
    if (s.handle.generation != 0 && service_->player(sfx_).active(s.handle)) {
        auto player = service_->player(sfx_);
        auto error = player.stage_stop(s.handle);
        if (error == a::Error::none) error = player.stage();
        if (error != a::Error::none) return fail(error);
    }
    s.playing = s.starting = static_cast<int>(id); s.priority = priority; s.idle = 0; s.parameters = parameters;
    slot = selected;
    return a::Error::none;
}
a::Error Session::sound_control(unsigned, a::Error error) noexcept {
    if (error == a::Error::none) error = service_->player(sfx_).stage();
    return error == a::Error::none ? error : fail(error);
}
a::Error Session::stop_sound(unsigned slot) noexcept {
    if (slot >= sounds_.size()) return a::Error::invalid_parameter;
    auto& s = sounds_[slot];
    if (state_ == State::failed || state_ == State::draining || state_ == State::drained) return a::Error::invalid_state;
    if (s.handle.generation != 0 && service_->player(sfx_).active(s.handle)) {
        const auto e = sound_control(slot, service_->player(sfx_).stage_stop(s.handle));
        if (e != a::Error::none) return e;
    }
    s.playing = s.starting = -1;
    return a::Error::none;
}
a::Error Session::stop_sounds() noexcept {
    for (unsigned i = 0; i < sounds_.size(); ++i) { const auto e = stop_sound(i); if (e != a::Error::none) return e; }
    return a::Error::none;
}
a::Error Session::sound_volume(unsigned slot, std::uint32_t value) noexcept {
    if (slot >= sounds_.size() || control() != a::Error::none) return a::Error::invalid_parameter;
    auto& s = sounds_[slot];
    // GAME ignores volume changes until this slot owns a sound descriptor.
    if (s.handle.generation == 0) return a::Error::none;
    s.parameters.volume = static_cast<std::uint16_t>(std::min(value, 32767u));
    if (!service_->player(sfx_).active(s.handle)) return a::Error::none;
    const auto* resource = resources_.sounds.banks()[0]->instruments[0]->sounds[static_cast<std::size_t>(s.bound_sound)];
    const auto volume = static_cast<std::uint16_t>((static_cast<std::uint32_t>(s.parameters.volume) * resource->volume * global_volume_) >> 14);
    return sound_control(slot, service_->player(sfx_).stage_volume(s.handle, volume));
}
a::Error Session::sound_pan(unsigned slot, std::uint32_t value) noexcept {
    if (slot >= sounds_.size() || control() != a::Error::none) return a::Error::invalid_parameter;
    auto& s = sounds_[slot];
    return s.handle.generation == 0 || !service_->player(sfx_).active(s.handle) ? a::Error::none : sound_control(slot, service_->player(sfx_).stage_pan(s.handle, static_cast<std::uint8_t>(std::min(value, 127u))));
}
a::Error Session::sound_pitch(unsigned slot, float value) noexcept {
    if (slot >= sounds_.size() || control() != a::Error::none || !std::isfinite(value) ||
        value < 1.0f / 32768.0f) return a::Error::invalid_parameter;
    auto& s = sounds_[slot];
    return s.handle.generation == 0 || !service_->player(sfx_).active(s.handle) ? a::Error::none : sound_control(slot, service_->player(sfx_).stage_pitch(s.handle, std::min(value, 2.0f)));
}
a::Error Session::sound_reverb(unsigned slot, std::uint32_t value) noexcept {
    if (slot >= sounds_.size() || control() != a::Error::none) return a::Error::invalid_parameter;
    auto& s = sounds_[slot];
    return s.handle.generation == 0 || !service_->player(sfx_).active(s.handle) ? a::Error::none : sound_control(slot, service_->player(sfx_).stage_fx_mix(s.handle, static_cast<std::uint8_t>(std::min(value, 127u))));
}
a::Error Session::sound_priority(unsigned slot, std::uint8_t value) noexcept {
    if (slot >= sounds_.size() || control() != a::Error::none) return a::Error::invalid_parameter;
    auto& s = sounds_[slot];
    if (s.handle.generation == 0) return a::Error::none;
    s.priority = value;
    return !service_->player(sfx_).active(s.handle) ? a::Error::none : service_->player(sfx_).set_priority(s.handle, value);
}
a::Error Session::global_sound_volume(std::uint8_t value, bool current_only) noexcept {
    if (control() != a::Error::none) return a::Error::invalid_state;
    if (!current_only) global_volume_ = std::min<std::uint8_t>(value, 127);
    auto player = service_->player(sfx_);
    for (auto& s : sounds_) {
        if (!player.active(s.handle)) continue;
        const auto* resource = resources_.sounds.banks()[0]->instruments[0]->sounds[static_cast<std::size_t>(s.bound_sound)];
        const auto volume = static_cast<std::uint16_t>((static_cast<std::uint32_t>(s.parameters.volume) * resource->volume *
            (current_only ? value : global_volume_)) >> 14);
        const auto e = player.stage_volume(s.handle, volume);
        if (e != a::Error::none) return fail(e);
    }
    return sound_control(0, a::Error::none);
}
a::Error Session::global_sound_reverb(std::uint8_t value, bool current_only) noexcept {
    if (control() != a::Error::none) return a::Error::invalid_state;
    value = std::min<std::uint8_t>(value, 127);
    auto player = service_->player(sfx_);
    for (auto& s : sounds_) {
        if (!player.active(s.handle)) continue;
        const auto mix = current_only ? value : static_cast<std::uint8_t>(std::min<unsigned>(s.parameters.reverb + value, 127));
        const auto e = player.stage_fx_mix(s.handle, mix);
        if (e != a::Error::none) return fail(e);
    }
    if (!current_only) global_reverb_ = value;
    return sound_control(0, a::Error::none);
}
int Session::playing_sound(unsigned slot) const noexcept { return slot < sounds_.size() ? sounds_[slot].playing : -1; }

a::Error Session::update() noexcept {
    auto player = service_->player(sfx_);
    std::uint16_t start_mask = 0;
    // Facade slots and native descriptors can diverge after out-of-order
    // retirement. Check every old generation before ANY start can reuse it.
    for (unsigned i = 0; i < sounds_.size(); ++i) {
        auto& s = sounds_[i];
        if (s.handle.generation == 0) {
            if (s.starting >= 0) start_mask |= static_cast<std::uint16_t>(1u << i);
            continue;
        }
        a::sfx::Status status;
        const auto e = player.status(s.handle, status);
        if (e != a::Error::none) return fail(e);
        if (!status.active) { s.handle = {}; if (s.starting < 0) s.playing = -1; }
        // A just-retired slot is deliberately absent from start_mask: GAME
        // starts its pending replacement on the next service iteration.
    }
    for (unsigned i = 0; i < sounds_.size(); ++i) {
        if ((start_mask & (1u << i)) == 0) continue;
        auto& s = sounds_[i];
        const auto* resource = resources_.sounds.banks()[0]->instruments[0]->sounds[static_cast<std::size_t>(s.starting)];
        a::SoundId id;
        auto e = service_->sound_id(*resource, id);
        a::sfx::Parameters parameters;
        parameters.volume = static_cast<std::uint16_t>((static_cast<std::uint32_t>(s.parameters.volume) * resource->volume * global_volume_) >> 14);
        parameters.pan = s.parameters.pan; parameters.pitch = s.parameters.pitch;
        parameters.fx_mix = static_cast<std::uint8_t>(std::min<unsigned>(s.parameters.reverb + global_reverb_, 127));
        if (e == a::Error::none) e = player.stage_start(id, parameters, s.handle);
        if (e == a::Error::none) e = player.set_priority(s.handle, s.priority);
        if (e != a::Error::none) return fail(e);
        s.bound_sound = s.starting;
        s.starting = -1;
    }
    auto error = player.stage();
    if (error != a::Error::none) return fail(error);
    for (unsigned i = 0; i < songs_.size(); ++i) {
        auto& song = songs_[i]; auto music = service_->sequence_player(csp_[i]);
        a::csp::Status status;
        error = music.status(status);
        if (error != a::Error::none) return fail(error);
        switch (song.phase) {
        case 1:
            // Native stopping is a real release/drain state, not the original
            // SDK's opaque state value. Re-posting stop here would restart its
            // fifty-ms release every service and prevent source retirement.
            if (status.playback == a::csp::Playback::playing) error = music.stage_stop();
            else if (status.playback == a::csp::Playback::stopped)
                song.phase = song.requested < 0 ? 0u : 2u;
            break;
        case 2:
            error = music.stage_sequence(static_cast<std::uint32_t>(song.requested));
            if (error == a::Error::none) error = music.stage_play();
            for (std::uint8_t channel = 0; error == a::Error::none && channel < 16; ++channel)
                error = music.stage_channel_priority(channel, global_priority_);
            if (error == a::Error::none) song.phase = 3;
            break;
        case 3:
            if (status.playback == a::csp::Playback::stopped) { song.phase = 0; song.requested = -1; }
            break;
        default: break;
        }
        if (error != a::Error::none) return fail(error);
        if (song.volume_ticks != 0) {
            --song.volume_ticks; song.volume = std::clamp(song.volume + song.volume_step, 0.0f, 32512.0f);
            error = music.stage_volume(static_cast<std::uint16_t>(song.volume));
        }
        error = publish(i, error);
        if (error != a::Error::none) return error;
    }
    return a::Error::none;
}
std::uint32_t Session::native_ai_frames() const noexcept { return ai_[0]; }
a::Error Session::retrace(std::uint64_t ordinal, std::uint64_t ns) noexcept {
    if (state_ == State::failed) return error_;
    if (output_.inspect(output_.owner).state == Output::State::failed ||
        !executor_.ready(executor_.owner, service_->rdram_words())) return fail(a::Error::execution_failed);
    if (ordinal == 0 || ordinal <= last_retrace_ || (clock_started_ && ns < now_ns_)) return fail(a::Error::invalid_service);
    if (retrace_notifications_ == std::numeric_limits<std::uint64_t>::max()) return fail(a::Error::identity_exhausted);
    if (clock_started_) {
        const auto elapsed = ns - now_ns_;
        // Split before multiplication to avoid overflow on a suspended host.
        const auto seconds = elapsed / 1000000000u;
        const auto remainder = elapsed % 1000000000u;
        const auto fraction = remainder * source_rate + fractional_;
        fractional_ = fraction % 1000000000u;
        std::uint64_t consumed = seconds > 1 ? std::numeric_limits<std::uint64_t>::max() : seconds * source_rate + fraction / 1000000000u;
        for (unsigned i = 0; i < ai_.size(); ++i) {
            const auto n = std::min<std::uint64_t>(consumed, ai_[i]);
            ai_[i] -= static_cast<std::uint32_t>(n); consumed -= n;
        }
        if (ai_[0] == 0) { ai_[0] = ai_[1]; ai_[1] = 0; }
    }
    clock_started_ = true; now_ns_ = ns; last_retrace_ = ordinal;
    ++retrace_notifications_;
    sampled_ai_ = native_ai_frames();
    tick_pending_ = true;
    // The first task can be built before its source tick arrives. Preserve its
    // deferred post-publication command update while that real task/PCM remains
    // owned; do not silently consume the initial queued tick.
    if (initial_update_pending_) {
        lagged_ai_ = sampled_ai_;
        const auto advanced = update();
        if (advanced != a::Error::none) return advanced;
        initial_update_pending_ = false;
        tick_pending_ = false;
    }
    return a::Error::none;
}
bool Session::retired() noexcept {
    for (const auto& s : sounds_) if (s.handle.generation != 0 || s.starting >= 0) return false;
    for (unsigned i = 0; i < songs_.size(); ++i) {
        a::csp::Status status;
        if (service_->sequence_player(csp_[i]).status(status) != a::Error::none || songs_[i].phase != 0 ||
            status.active_notes != 0 || status.bound_notes != 0 || status.playback != a::csp::Playback::stopped) return false;
    }
    return service_->voices_retired();
}
a::Error Session::build(a::TaskView& task) noexcept {
    task = {};
    if (state_ == State::failed) return error_;
    if (state_ == State::draining || state_ == State::drained || service_->has_lease()) return a::Error::none;
    if (!initial_build_ && !tick_pending_) return a::Error::none;
    if (output_.inspect(output_.owner).state != Output::State::ready ||
        !executor_.ready(executor_.owner, service_->rdram_words())) return fail(a::Error::execution_failed);
    if ((state_ == State::stopping || restarting_) && retired()) {
        if (restarting_) {
            const auto reset = reconfigure_ ? service_->reset(restart_effects_) : service_->reset();
            if (reset != a::Error::none) return fail(reset);
            if (reconfigure_) custom_effects_ = pending_custom_effects_;
            sounds_ = {}; songs_ = {}; lagged_ai_ = native_ai_frames();
            reconfigure_ = false;
            const auto registered = register_players();
            if (registered != a::Error::none) return fail(registered);
            restarting_ = false;
        } else {
            if (!output_.finish(output_.owner)) return fail(a::Error::execution_failed);
            state_ = State::draining;
            return a::Error::none;
        }
    }
    const auto wait_notifications = initial_build_ && !tick_pending_ ? 2u : 1u;
    if (service_serial_ == std::numeric_limits<std::uint64_t>::max() ||
        retrace_notifications_ > std::numeric_limits<std::uint64_t>::max() - wait_notifications)
        return fail(a::Error::identity_exhausted);
    // GAME samples its audio-frame time immediately before alAudioFrame.
    // Reuse this already-required producer identity as the native equality
    // epoch at that same boundary, after every no-work/admission return.
    const auto error = service_->build({++service_serial_, lagged_ai_, 0}, task_);
    if (error != a::Error::none) return fail(error);
    // sp70 is sampled at the GAME tick AFTER the task's quota/build. Retain it
    // only for the next task; the first build's zero comes from source init.
    const bool service_tick = tick_pending_;
    initial_update_pending_ = initial_build_ && !service_tick;
    output_after_notification_ = retrace_notifications_ + wait_notifications;
    if (service_tick) lagged_ai_ = sampled_ai_;
    initial_build_ = false; tick_pending_ = false; task = task_;
    // GAME advances sound/song requests and volume controllers after publishing
    // the built task. These controls can affect only future client callbacks.
    if (service_tick) {
        const auto advanced = update();
        if (advanced != a::Error::none) return advanced;
    }
    return a::Error::none;
}
a::Error Session::complete(a::TaskToken token, bool success) noexcept {
    if (token != task_.token || token.serial == 0 || pcm_.token.serial != 0) return a::Error::stale_task;
    const auto error = service_->complete(token, success, pcm_);
    if (error != a::Error::none) return fail(error);
    submitted_ = 0;
    auto copied = service_->copy_pcm(pcm_, scratch_);
    if (copied != a::Error::none) return fail(copied);
    if (!high_quality_) {
        for (std::uint32_t i = 0; i < pcm_.frame_count; ++i) {
            const auto mono = static_cast<std::int16_t>((static_cast<int>(scratch_[i * 2]) + scratch_[i * 2 + 1]) / 2);
            scratch_[i * 2] = scratch_[i * 2 + 1] = mono;
        }
    }
    return a::Error::none;
}
a::Error Session::submit() noexcept {
    if (state_ == State::failed) return error_;
    if (pcm_.token.serial == 0) return a::Error::none;
    if (retrace_notifications_ < output_after_notification_) return a::Error::none;
    // Native AI has one current and one pending buffer. Never turn a third
    // accepted host buffer into fabricated AI storage or discard its PCM.
    if (submitted_ == 0 && ai_[0] != 0 && ai_[1] != 0) return a::Error::none;
    const auto remaining = pcm_.frame_count - submitted_;
    const auto samples = std::span(scratch_).subspan(submitted_ * 2u, remaining * 2u);
    const auto accepted = output_.submit(output_.owner, samples);
    if (accepted.result != Output::Submit::accepted && accepted.result != Output::Submit::would_block &&
        accepted.result != Output::Submit::failed) return fail(a::Error::invalid_state);
    if (accepted.accepted_frames > remaining ||
        (accepted.result == Output::Submit::accepted && accepted.accepted_frames != remaining)) return fail(a::Error::invalid_state);
    if (accepted_frames_ > std::numeric_limits<std::uint64_t>::max() - accepted.accepted_frames) return fail(a::Error::identity_exhausted);
    submitted_ += accepted.accepted_frames; accepted_frames_ += accepted.accepted_frames;
    if (accepted.result == Output::Submit::failed) return fail(a::Error::execution_failed);
    if (submitted_ != pcm_.frame_count) return a::Error::none;
    if (ai_[0] == 0) ai_[0] = pcm_.frame_count;
    else if (ai_[1] == 0) ai_[1] = pcm_.frame_count;
    else return fail(a::Error::invalid_state);
    const auto error = service_->release(pcm_.token);
    if (error != a::Error::none) return fail(error);
    pcm_ = {}; task_ = {}; submitted_ = 0;
    return a::Error::none;
}
a::Error Session::stop() noexcept {
    if (state_ == State::failed) return error_;
    if (state_ == State::draining || state_ == State::drained) return a::Error::none;
    auto e = stop_bgm(); if (e == a::Error::none) e = stop_sounds();
    if (e != a::Error::none) return e;
    for (auto& song : songs_) song.volume_ticks = 0;
    state_ = State::stopping; restarting_ = false;
    return a::Error::none;
}
a::Error Session::restart() noexcept {
    if (control() != a::Error::none) return a::Error::invalid_state;
    auto e = stop_bgm(); if (e == a::Error::none) e = stop_sounds();
    if (e == a::Error::none) restarting_ = true;
    return e;
}
a::Error Session::restart(a::effects::Config effects) noexcept {
    if (control() != a::Error::none) return a::Error::invalid_state;
    const auto admission = a::detail::validate_effects_for_rate(effects, source_rate);
    if (admission != a::Error::none) return admission;
    const auto count = effects.sections.size();
    std::copy(effects.sections.begin(), effects.sections.end(), restart_sections_.begin());
    restart_effects_ = {effects.routing, effects.ring_samples, std::span(restart_sections_).first(count)};
    const auto requested = restart();
    if (requested == a::Error::none) {
        reconfigure_ = true;
        pending_custom_effects_ = is_custom_program(resources_, effects);
    }
    return requested;
}
a::Error Session::custom_reverb() noexcept {
    if (state_ != State::ready) return a::Error::invalid_state;
    if (restarting_ && (reconfigure_ ? pending_custom_effects_ : custom_effects_))
        return a::Error::none;
    if (!restarting_ && custom_effects_) return a::Error::none;
    std::array<a::effects::Section, 8> sections{};
    a::effects::Config effects;
    const auto decoded = native_startup_effects(resources_, sections, effects);
    if (decoded != a::Error::none) return decoded;
    if (restarting_) {
        // The real stop/client/PCM retirement is already in flight. Replace
        // only its retained target program before reset; never claim it has
        // completed, restart its executor, or reject a legitimate drain.
        std::copy(sections.begin(), sections.end(), restart_sections_.begin());
        restart_effects_ = {effects.routing, effects.ring_samples,
                           std::span(restart_sections_).first(sections.size())};
        pending_custom_effects_ = reconfigure_ = true;
        return a::Error::none;
    }
    return restart(effects);
}
a::Error Session::poll_drain() noexcept {
    if (state_ == State::failed) return error_;
    if (state_ != State::draining) return a::Error::invalid_state;
    const auto state = output_.poll(output_.owner);
    if (state == Output::State::failed) return fail(a::Error::execution_failed);
    if (state != Output::State::draining && state != Output::State::drained) return fail(a::Error::invalid_state);
    if (state == Output::State::drained) state_ = State::drained;
    return a::Error::none;
}
a::Error Session::abandon_pcm() noexcept {
    if (pcm_.token.serial == 0 && service_->has_lease()) return a::Error::task_busy;
    if (pcm_.token.serial != 0) {
        const auto released = service_->release(pcm_.token);
        if (released != a::Error::none) return fail(released);
        pcm_ = {}; task_ = {}; submitted_ = 0;
    }
    fail(a::Error::execution_failed);
    return a::Error::none;
}
} // namespace isle::game_audio
