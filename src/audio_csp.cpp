#include "audio_synth_internal.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <new>
#include <utility>

namespace isle::audio {
namespace {
using namespace detail;
constexpr auto limit = std::numeric_limits<std::uint64_t>::max();

Error access(Context* c, PlayerId id, CspCheckpoint*& p, const CspResources*& r,
             bool mutate = false) noexcept {
    p = nullptr; r = nullptr;
    if (!c || id.owner != c->owner || id.slot >= AudioService::max_players)
        return Error::stale_handle;
    const auto& client = c->state().clients[id.slot];
    if (!client.live || client.generation != id.generation || client.kind != ClientKind::csp ||
        client.typed_index >= c->csp.size() || !c->csp[client.typed_index]) return Error::stale_handle;
    if (mutate && c->poisoned) return Error::needs_reset;
    if (mutate && c->building) return Error::task_busy;
    p = &c->csp_state(c->live, client.typed_index);
    r = &c->csp[client.typed_index]->resources;
    return Error::none;
}
Error post(Context* c, PlayerId id, CspControl control) noexcept {
    CspCheckpoint* p; const CspResources* r;
    auto error = access(c, id, p, r, true);
    if (error != Error::none) return error;
    if (p->inbox_count + p->published_count + p->event_count >= csp::SequencePlayer::event_capacity)
        return Error::event_capacity;
    p->inbox[p->inbox_count++] = control;
    return Error::none;
}
Error add_us(std::uint64_t now, std::uint64_t delta, std::uint64_t& out) noexcept {
    if (delta > limit - now) return Error::identity_exhausted;
    out = now + delta;
    return Error::none;
}
Error tick_deadline(const CspCheckpoint& p, std::uint64_t ticks, std::uint64_t& due) noexcept {
    if (p.us_per_tick == 0 || ticks > (limit - p.client_us) / p.us_per_tick)
        return Error::identity_exhausted;
    due = p.client_us + ticks * p.us_per_tick;
    return Error::none;
}
Error schedule(CspCheckpoint& p, CspEventKind kind, std::uint16_t note, std::uint64_t due) noexcept {
    if (p.event_count + p.inbox_count + p.published_count >= p.events.size()) return Error::event_capacity;
    if (p.next_serial == limit) return Error::identity_exhausted;
    for (auto& e : p.events) {
        if (e.live) continue;
        e = {true, kind, note, p.cue, p.notes[note].incarnation, due, ++p.next_serial};
        ++p.event_count;
        return Error::none;
    }
    return Error::event_capacity;
}
void cancel(CspCheckpoint& p, std::uint16_t note, bool all = true) noexcept {
    for (auto& e : p.events) {
        if (!e.live || e.note != note || (!all && e.kind != CspEventKind::envelope)) continue;
        e.live = false; --p.event_count;
    }
}
std::size_t earliest(const CspCheckpoint& p) noexcept {
    std::size_t selected = p.events.size();
    for (std::size_t i = 0; i < p.events.size(); ++i) {
        const auto& e = p.events[i];
        if (!e.live) continue;
        if (selected == p.events.size() || e.due < p.events[selected].due ||
            (e.due == p.events[selected].due && e.serial < p.events[selected].serial)) selected = i;
    }
    return selected;
}
Error tolerate_unbound(Error e) noexcept { return e == Error::unbound ? Error::none : e; }

bool oscillator_type(std::uint8_t type) noexcept {
    return (type >= 1 && type <= 4) || (type >= 128 && type <= 131);
}
float oscillator_value(const Oscillator& o) noexcept {
    const auto type = o.type;
    const float position = static_cast<float>(o.count) / o.period;
    if (type == 1) {
        const float half = static_cast<float>(o.depth >> 1);
        return (127.0f - half) + std::sin(position * 6.2831853f) * half;
    }
    if (type == 2) return o.high ? 127.0f : static_cast<float>(static_cast<std::uint8_t>(127u - o.depth));
    if (type == 3) return 127.0f - position * o.depth;
    if (type == 4) return static_cast<float>(static_cast<std::uint8_t>(127u - o.depth)) + position * o.depth;
    if (type == 128) return cents_ratio(static_cast<std::int32_t>(
        std::sin(position * 6.2831853f) * o.depth_cents));
    const auto cents = static_cast<std::int32_t>(o.depth_cents);
    if (type == 129) return cents_ratio(o.high ? cents : -cents);
    const float range = position * static_cast<float>(2 * cents);
    return cents_ratio(static_cast<std::int32_t>(type == 130 ?
        static_cast<float>(cents) - range : -static_cast<float>(cents) + range));
}
std::uint16_t oscillator_start(OscillatorPool& pool, std::uint8_t type, std::uint8_t rate,
                               std::uint8_t depth, float& value) noexcept {
    if (type == 0 || !oscillator_type(type)) return no_slot;
    if (!pool.initialized) {
        for (std::size_t i = 0; i < pool.slots.size(); ++i)
            pool.slots[i].next = i + 1 < pool.slots.size() ? static_cast<std::uint16_t>(i + 1) : no_slot;
        pool.free_head = 0; pool.initialized = true;
    }
    if (pool.free_head == no_slot) return no_slot;
    const auto index = pool.free_head;
    auto& o = pool.slots[index];
    pool.free_head = o.next;
    o = {};
    o.allocated = true; o.type = type; o.depth = depth;
    o.period = static_cast<std::uint16_t>((type == 1 || type == 128 ? 259 : 256) - rate);
    o.count = type >= 130 ? o.period : 0;
    o.depth_cents = type >= 128 ? integer_power(1.03099303f, depth) : 0.0f;
    // Both vibrato saws initialize at their authored extrema, then wrap on
    // their first update. Their initial sample is not count/period evaluated.
    if (type == 130 || type == 131) {
        const auto cents = static_cast<std::int32_t>(o.depth_cents);
        value = cents_ratio(type == 130 ? cents : -cents);
    } else value = oscillator_value(o);
    return index;
}
std::uint32_t oscillator_update(Oscillator& o, float& value) noexcept {
    if (o.type == 2 || o.type == 129) {
        o.high = !o.high;
        value = oscillator_value(o);
        return 16000u * o.period;
    }
    ++o.count;
    const bool sine = o.type == 1 || o.type == 128;
    if (o.count > o.period || (sine && o.count == o.period)) o.count = 0;
    value = oscillator_value(o);
    return 16000;
}
void oscillator_stop(OscillatorPool& pool, std::uint16_t& slot) noexcept {
    if (slot == no_slot) return;
    auto& o = pool.slots[slot];
    o.allocated = false; o.next = pool.free_head; pool.free_head = slot;
    slot = no_slot;
}

const CspSoundMapping* select_sound(const CspResources& r, const Instrument* instrument,
                                  std::uint8_t key, std::uint8_t velocity) noexcept {
    if (!instrument) return nullptr;
    for (const auto* sound : instrument->sounds) {
        if (!sound || key < sound->keymap->key_min || key > sound->keymap->key_max ||
            velocity < sound->keymap->velocity_min || velocity > sound->keymap->velocity_max) continue;
        for (const auto& s : r.sounds) if (s.resource == sound) return &s;
    }
    return nullptr;
}
void program(CspChannel& channel, const Bank& bank, std::uint8_t index, bool percussion = false) noexcept {
    channel.controls.program = index;
    channel.controls.percussion = percussion;
    channel.instrument = percussion ? bank.percussion :
        index < bank.instruments.size() ? bank.instruments[index] : nullptr;
    if (channel.instrument) {
        channel.controls.volume = channel.instrument->volume;
        channel.controls.pan = channel.instrument->pan;
        channel.controls.priority = channel.instrument->priority;
    }
}
Error set_tempo(CspCheckpoint& p, std::uint32_t tempo) noexcept {
    const float product = static_cast<float>(tempo) * p.quarter_per_tick;
    if (!std::isfinite(product) || product < 1.0f ||
        static_cast<double>(product) > std::numeric_limits<std::uint32_t>::max()) return Error::invalid_parameter;
    const auto next = static_cast<std::uint32_t>(product);
    // Only duration offs are tick-retimed. A pending reader event keeps its
    // already posted deadline; the original public callback experiments reject
    // a fractional microsecond ratio for offs.
    for (auto& e : p.events) {
        if (!e.live || e.kind != CspEventKind::off || e.due < p.client_us) continue;
        const auto ticks = (e.due - p.client_us) / p.us_per_tick;
        if (ticks > (limit - p.client_us) / next) return Error::identity_exhausted;
    }
    for (auto& e : p.events)
        if (e.live && e.kind == CspEventKind::off && e.due >= p.client_us)
            e.due = p.client_us + ((e.due - p.client_us) / p.us_per_tick) * next;
    p.us_per_tick = next;
    return Error::none;
}
std::uint8_t note_pan(const CspChannel& channel, const Sound& sound) noexcept {
    return static_cast<std::uint8_t>(std::clamp<int>(channel.controls.pan + sound.pan - 64, 0, 127));
}
std::uint8_t note_fx(const CspCheckpoint& p, const CspChannel& channel) noexcept {
    const auto wrapped = static_cast<std::uint8_t>(channel.controls.fx_mix + p.extra_fx);
    return std::min<std::uint8_t>(wrapped, 127);
}
float note_pitch(const CspNote& n, const CspChannel& channel) noexcept {
    const auto range = channel.instrument ? channel.instrument->bend_range : 0;
    const auto cents = (static_cast<std::int32_t>(channel.controls.bend) - 8192) * range / 8192;
    return (n.base_pitch * cents_ratio(cents)) * n.vibrato;
}
Error note_gain(const CspCheckpoint& p, const CspNote& n, std::uint8_t level, Gain15& result) noexcept {
    result = {};
    // Original callbacks may produce a negative or oversized send for extreme
    // authored depths. That is outside public Synth volume's defined domain;
    // fail atomically rather than clamp it or perform an undefined float cast.
    if (!std::isfinite(n.tremolo)) return Error::invalid_parameter;
    const auto& ch = p.channels[n.channel].controls;
    // Two independently quantized Q15 factors: channel controls and note
    // performance. Public axis/fresh-source and independent heldout executions
    // distinguish this from multiplying all bytes in one rational expression.
    auto channel = (static_cast<std::uint32_t>(p.volume) * ch.volume * ch.gain_scale) >> 14;
    if (ch.extra_volume >= 0) channel = (channel * static_cast<std::uint8_t>(ch.extra_volume)) >> 7;
    const auto tremolo = static_cast<std::int32_t>(n.tremolo);
    const auto note = (static_cast<std::int32_t>(level) * n.velocity * tremolo) >> 6;
    const auto amplitude = (static_cast<std::int64_t>(note) * channel) >> 15;
    if (amplitude < 0 || amplitude > 0x7fff) return Error::invalid_parameter;
    // Public Synth volume is perceptual; Gain15 is the linear DSP amplitude.
    // Quantize controller/note composition first, then square once at this boundary.
    const auto volume = static_cast<std::uint32_t>(amplitude);
    result = {static_cast<std::uint16_t>((volume * volume) >> 15)};
    return Error::none;
}
float phase_factor(std::uint8_t from, std::uint8_t to, std::int32_t us) noexcept {
    // CSP control estimator, not the DSP ramp. Measured early rising-decay
    // controls retain their starting scalar; other phase boundaries need qualification.
    if (us == 0 || from <= to) return 1.0f;
    const auto ratio = static_cast<float>(std::max<std::uint8_t>(to, 1)) /
                       static_cast<float>(std::max<std::uint8_t>(from, 1));
    return std::pow(ratio, 1.0f / static_cast<float>(us));
}
std::uint8_t envelope_level(const CspNote& n, std::uint64_t at) noexcept {
    if (at >= n.envelope_end || n.envelope_end == n.envelope_start) return n.envelope_to;
    if (at <= n.envelope_start) return n.envelope_from;
    const auto elapsed = static_cast<std::uint32_t>(at - n.envelope_start);
    const auto value = std::max<float>(n.envelope_from, 1.0f) * integer_power(n.envelope_factor, elapsed);
    const auto low = static_cast<float>(std::min(n.envelope_from, n.envelope_to));
    const auto high = static_cast<float>(std::max(n.envelope_from, n.envelope_to));
    return static_cast<std::uint8_t>(std::clamp(value, low, high));
}
Error finish(Context& c, CpuState& state, CspCheckpoint& p, std::uint16_t slot, ParamTime time) noexcept {
    auto& n = p.notes[slot];
    auto error = tolerate_unbound(stop_voice(c, state, n.descriptor, time));
    if (error != Error::none) return error;
    error = tolerate_unbound(free_voice(c, state, n.descriptor, time));
    if (error != Error::none) return error;
    cancel(p, slot);
    oscillator_stop(state.oscillators, n.tremolo_slot);
    oscillator_stop(state.oscillators, n.vibrato_slot);
    const auto descriptor = n.descriptor;
    n = {}; n.descriptor = descriptor;
    return Error::none;
}
Error envelope(Context& c, CpuState& state, CspCheckpoint& p,
               std::uint16_t slot, CspPhase phase, std::uint8_t target, std::int32_t us,
               ParamTime time) noexcept {
    auto& n = p.notes[slot];
    if (us < 0) return Error::unsupported_envelope;
    const auto from = envelope_level(n, p.client_us);
    cancel(p, slot, false);
    n.phase = phase; n.envelope_from = from; n.envelope_to = target;
    n.envelope_factor = phase_factor(from, target, us);
    n.envelope_start = p.client_us;
    auto error = add_us(p.client_us, static_cast<std::uint32_t>(us), n.envelope_end);
    if (error != Error::none) return error;
    Gain15 gain{};
    error = note_gain(p, n, target, gain);
    if (error != Error::none) return error;
    error = tolerate_unbound(gain_voice(c, state, n.descriptor, gain,
        {duration_frames(us, c.config.synthesis_rate)}, time));
    if (error != Error::none) return error;
    if (phase == CspPhase::sustain) return Error::none;
    return schedule(p, CspEventKind::envelope, slot, n.envelope_end);
}
Error release(Context& c, CpuState& state, CspCheckpoint& p,
              std::uint16_t slot, ParamTime time, bool stop = false) noexcept {
    auto& n = p.notes[slot];
    if (n.phase == CspPhase::free || (n.phase == CspPhase::release && !stop)) return Error::none;
    if (!stop && p.channels[n.channel].controls.sustain) { n.sustained = true; return Error::none; }
    const CatalogSound* s = nullptr;
    auto error = catalog_sound(c, n.sound, s);
    if (error != Error::none) return error;
    error = priority_voice(c, state, n.descriptor, 0);
    if (error != Error::none) return error;
    return envelope(c, state, p, slot, CspPhase::release, 0,
                    stop ? 50000 : s->resource->envelope->release_us, time);
}
Error read_next(CspCheckpoint& p) noexcept {
    SequenceReadResult diagnostic{};
    const auto step = p.reader.next(p.next, diagnostic);
    if (step == SequenceStep::error) return Error::invalid_resource;
    p.have_next = step == SequenceStep::event;
    if (!p.have_next) { p.ended = true; return Error::none; }
    p.resume_delta_ticks = p.next.delta_ticks;
    return tick_deadline(p, p.next.delta_ticks, p.sequence_due);
}
Error start_note(Context& c, CpuState& state, CspCheckpoint& p, const CspResources& r,
                 const SequenceEvent& event, ParamTime time) noexcept {
    const auto channel = static_cast<std::uint8_t>(event.status & 15);
    auto& ch = p.channels[channel];
    const auto* s = select_sound(r, ch.instrument, event.data1, event.data2);
    if (!s) return Error::none;
    std::uint16_t slot = 0;
    while (slot < r.config.max_notes && p.notes[slot].phase != CspPhase::free) ++slot;
    if (slot == r.config.max_notes) return Error::none;
    if (p.next_incarnation == limit) return Error::identity_exhausted;
    auto& n = p.notes[slot];
    const auto descriptor = n.descriptor;
    n = {}; n.descriptor = descriptor;
    n.incarnation = ++p.next_incarnation; n.channel = channel; n.key = event.data1;
    n.velocity = event.data2; n.sound = s->id; n.phase = CspPhase::attack;
    n.base_pitch = cents_ratio((static_cast<std::int32_t>(n.key) - s->resource->keymap->base_key) * 100 +
                               s->resource->keymap->detune);
    Allocation allocation{};
    auto error = allocate_voice(c, state, n.descriptor, ch.controls.priority, time, allocation);
    if (error != Error::none && error != Error::voice_unavailable) return error;
    const bool bound = error == Error::none;
    const auto* instrument = ch.instrument;
    if (bound && instrument) {
        const auto init = [&](bool vibrato) noexcept -> Error {
            const auto type = vibrato ? instrument->vibrato_type : instrument->tremolo_type;
            const auto rate = vibrato ? instrument->vibrato_rate : instrument->tremolo_rate;
            const auto depth = vibrato ? instrument->vibrato_depth : instrument->tremolo_depth;
            const auto delay = vibrato ? instrument->vibrato_delay : instrument->tremolo_delay;
            auto& value = vibrato ? n.vibrato : n.tremolo;
            const auto osc = oscillator_start(state.oscillators, type, rate, depth, value);
            if (osc == no_slot || delay == 0) return Error::none;
            (vibrato ? n.vibrato_slot : n.tremolo_slot) = osc;
            std::uint64_t due;
            auto result = add_us(p.client_us, static_cast<std::uint32_t>(delay) * 16384u, due);
            return result == Error::none ? schedule(p, vibrato ? CspEventKind::vibrato : CspEventKind::tremolo, slot, due) : result;
        };
        error = init(false); if (error != Error::none) return error;
        error = init(true); if (error != Error::none) return error;
    }
    const auto& env = *s->resource->envelope;
    n.envelope_from = 0; n.envelope_to = env.attack_volume;
    n.envelope_factor = phase_factor(0, env.attack_volume, env.attack_us);
    n.envelope_start = p.client_us;
    error = add_us(p.client_us, static_cast<std::uint32_t>(env.attack_us), n.envelope_end);
    if (error != Error::none) return error;
    Gain15 gain{};
    error = note_gain(p, n, env.attack_volume, gain);
    if (error != Error::none) return error;
    if (bound) {
        DirectParameters parameters{note_pitch(n, ch), gain, note_pan(ch, *s->resource), note_fx(p, ch)};
        error = start_voice(c, state, n.descriptor, s->wave, parameters,
                            {duration_frames(env.attack_us, c.config.synthesis_rate)}, time);
        if (error != Error::none) return error;
    }
    error = schedule(p, CspEventKind::envelope, slot, n.envelope_end);
    if (error != Error::none) return error;
    if (event.duration_ticks != 0) {
        std::uint64_t due;
        error = tick_deadline(p, event.duration_ticks, due);
        if (error != Error::none) return error;
        error = schedule(p, CspEventKind::off, slot, due);
    }
    return error;
}
Error refresh(Context& c, CpuState& state, CspCheckpoint& p, const CspResources& r,
              std::uint8_t channel, CspControlKind kind, ParamTime time) noexcept {
    for (std::uint16_t slot = 0; slot < r.config.max_notes; ++slot) {
        auto& n = p.notes[slot];
        if (n.phase == CspPhase::free || (channel < p.channels.size() && n.channel != channel)) continue;
        Error error = Error::none;
        switch (kind) {
        case CspControlKind::pan: {
            const CatalogSound* s = nullptr;
            error = catalog_sound(c, n.sound, s);
            if (error != Error::none) return error;
            error = pan_voice(c, state, n.descriptor, note_pan(p.channels[n.channel], *s->resource), time);
            break;
        }
        case CspControlKind::fx:
            error = fx_voice(c, state, n.descriptor, note_fx(p, p.channels[n.channel]), time); break;
        case CspControlKind::priority:
            if (n.phase != CspPhase::release)
                error = priority_voice(c, state, n.descriptor, p.channels[n.channel].controls.priority);
            break;
        case CspControlKind::tempo:
            error = pitch_voice(c, state, n.descriptor, note_pitch(n, p.channels[n.channel]), time); break;
        default: {
            // Controls still update channel state during attack/release, but
            // do not replace those already-running DSP envelope ramps.
            if (n.phase == CspPhase::attack || n.phase == CspPhase::release) break;
            std::uint64_t resume;
            const auto clock_error = add_us(p.client_us, 1000, resume);
            if (clock_error != Error::none) return clock_error;
            const auto level = envelope_level(n, resume);
            Gain15 gain{};
            error = note_gain(p, n, level, gain);
            if (error != Error::none) return error;
            error = gain_voice(c, state, n.descriptor, gain,
                               {duration_frames(1000, c.config.synthesis_rate)}, time);
            // The adjustment targets its one-ms endpoint, then resumes the
            // authored envelope without moving that phase's original deadline.
            if (error == Error::none && n.envelope_end > resume) {
                cancel(p, slot, false);
                error = schedule(p, CspEventKind::envelope, slot, resume);
            }
            break;
        }
        }
        error = tolerate_unbound(error);
        if (error != Error::none) return error;
    }
    return Error::none;
}
Error apply_control(Context& c, CpuState& state, CspCheckpoint& p, const CspResources& r,
                    CspControl control, ParamTime time) noexcept {
    auto& ch = p.channels[control.channel];
    const auto value = static_cast<std::uint8_t>(control.value);
    switch (control.kind) {
    case CspControlKind::bind: {
        if (p.playback != csp::Playback::stopped || control.value >= r.config.sequences.sequences().size())
            return Error::invalid_state;
        for (const auto& n : p.notes) if (n.phase != CspPhase::free) return Error::invalid_state;
        if (p.cue == limit) return Error::identity_exhausted;
        const auto result = p.reader.start(r.config.sequences.sequences()[control.value]);
        if (!result) return Error::invalid_resource;
        ++p.cue; p.song = control.value; p.bound = true; p.ended = p.have_next = false;
        p.quarter_per_tick = 1.0f / static_cast<float>(p.reader.division());
        p.us_per_tick = static_cast<std::uint32_t>(500000.0f * p.quarter_per_tick);
        if (p.us_per_tick == 0) return Error::invalid_parameter;
        for (std::uint8_t i = 0; i < p.channels.size(); ++i) {
            const auto gain_scale = p.channels[i].controls.gain_scale;
            const auto extra_volume = p.channels[i].controls.extra_volume;
            p.channels[i] = {};
            p.channels[i].controls.gain_scale = gain_scale;
            p.channels[i].controls.extra_volume = extra_volume;
            program(p.channels[i], *r.bank, 0, i == 9 && r.bank->percussion);
        }
        return Error::none;
    }
    case CspControlKind::play:
        if (!p.bound || p.playback == csp::Playback::stopping) return Error::invalid_state;
        if (p.playback == csp::Playback::playing || p.ended) return Error::none;
        p.playback = csp::Playback::playing;
        return p.have_next ? tick_deadline(p, p.resume_delta_ticks, p.sequence_due) : read_next(p);
    case CspControlKind::stop:
        if (p.playback == csp::Playback::stopped) return Error::none;
        p.playback = csp::Playback::stopping;
        for (auto& e : p.events) if (e.live && e.kind == CspEventKind::off) { e.live = false; --p.event_count; }
        for (std::uint16_t i = 0; i < r.config.max_notes; ++i) {
            const auto error = release(c, state, p, i, time, true);
            if (error != Error::none) return error;
        }
        return Error::none;
    case CspControlKind::volume:
        p.volume = static_cast<std::uint16_t>(control.value); break;
    case CspControlKind::tempo:
        return p.bound ? set_tempo(p, control.value) : Error::invalid_state;
    case CspControlKind::program:
        program(ch, *r.bank, value); return Error::none;
    case CspControlKind::pan: ch.controls.pan = value; break;
    case CspControlKind::channel_volume: ch.controls.volume = value; break;
    case CspControlKind::priority: ch.controls.priority = value; break;
    case CspControlKind::fx: ch.controls.fx_mix = std::min<std::uint8_t>(value, 127); break;
    case CspControlKind::extra_volume: ch.controls.extra_volume = std::bit_cast<std::int8_t>(value); break;
    case CspControlKind::extra_fx:
        p.extra_fx = std::bit_cast<std::int8_t>(value); return Error::none;
    }
    // The channel-count sentinel selects all notes in one bounded pass.
    const auto channel = control.kind == CspControlKind::volume ?
                         static_cast<std::uint8_t>(p.channels.size()) : control.channel;
    return refresh(c, state, p, r, channel, control.kind, time);
}
Error midi(Context& c, CpuState& state, CspCheckpoint& p, const CspResources& r,
           const SequenceEvent& event, ParamTime time) noexcept {
    const auto channel = static_cast<std::uint8_t>(event.status & 15);
    auto& ch = p.channels[channel];
    const auto type = event.status & 0xf0;
    if (type == 0x90 && event.data2 != 0) return start_note(c, state, p, r, event, time);
    if (type == 0x80 || type == 0x90) {
        std::uint16_t first = no_slot;
        for (std::uint16_t i = 0; i < r.config.max_notes; ++i) {
            const auto& n = p.notes[i];
            if (n.phase == CspPhase::free || n.phase == CspPhase::release || n.sustained ||
                n.channel != channel || n.key != event.data1) continue;
            if (first == no_slot || n.incarnation < p.notes[first].incarnation) first = i;
        }
        return first == no_slot ? Error::none : release(c, state, p, first, time);
    }
    if (type == 0xc0) return apply_control(c, state, p, r, {CspControlKind::program, channel, event.data1}, time);
    if (type == 0xe0) {
        ch.controls.bend = static_cast<std::uint16_t>(event.data1 | (event.data2 << 7));
        return refresh(c, state, p, r, channel, CspControlKind::tempo, time);
    }
    if (type != 0xb0) return Error::none; // Pressure has no game-bank consumer.
    CspControlKind kind;
    switch (event.data1) {
    case 7: kind = CspControlKind::channel_volume; break;
    case 10: kind = CspControlKind::pan; break;
    case 16: kind = CspControlKind::priority; break;
    case 21:
        ch.controls.gain_scale = event.data2;
        return refresh(c, state, p, r, channel, CspControlKind::channel_volume, time);
    case 91: kind = CspControlKind::fx; break;
    case 64:
        ch.controls.sustain = event.data2 >= 64;
        if (!ch.controls.sustain) {
            for (std::uint16_t i = 0; i < r.config.max_notes; ++i) {
                auto& n = p.notes[i];
                if (n.phase == CspPhase::free || n.channel != channel || !n.sustained) continue;
                n.sustained = false;
                const auto error = release(c, state, p, i, time);
                if (error != Error::none) return error;
            }
        }
        return Error::none;
    case 120:
        for (std::uint16_t i = 0; i < r.config.max_notes; ++i)
            if (p.notes[i].phase != CspPhase::free && p.notes[i].channel == channel) {
                const auto error = finish(c, state, p, i, time);
                if (error != Error::none) return error;
            }
        return Error::none;
    case 123:
        for (std::uint16_t i = 0; i < r.config.max_notes; ++i)
            if (p.notes[i].phase != CspPhase::free && p.notes[i].channel == channel) {
                const auto error = release(c, state, p, i, time);
                if (error != Error::none) return error;
            }
        return Error::none;
    default: return Error::invalid_parameter;
    }
    return apply_control(c, state, p, r, {kind, channel, event.data2}, time);
}
Error event(Context& c, CpuState& state, CspCheckpoint& p, const CspResources& r,
            CspEvent e, ParamTime time) noexcept {
    if (e.cue != p.cue || e.note >= r.config.max_notes) return Error::none;
    auto& n = p.notes[e.note];
    if (n.phase == CspPhase::free || n.incarnation != e.incarnation) return Error::none;
    if (e.kind == CspEventKind::off) return release(c, state, p, e.note, time);
    if (e.kind == CspEventKind::envelope) {
        if (p.client_us < n.envelope_end) {
            const auto remaining = n.envelope_end - p.client_us;
            if (remaining > std::numeric_limits<std::int32_t>::max()) return Error::invalid_parameter;
            Gain15 gain{};
            auto error = note_gain(p, n, n.envelope_to, gain);
            if (error != Error::none) return error;
            error = tolerate_unbound(gain_voice(c, state, n.descriptor, gain,
                {duration_frames(static_cast<std::int32_t>(remaining), c.config.synthesis_rate)}, time));
            return error == Error::none ? schedule(p, CspEventKind::envelope, e.note, n.envelope_end) : error;
        }
        if (n.phase == CspPhase::release) return finish(c, state, p, e.note, time);
        if (n.phase == CspPhase::attack) {
            const CatalogSound* s = nullptr;
            const auto error = catalog_sound(c, n.sound, s);
            if (error != Error::none) return error;
            const auto& env = *s->resource->envelope;
            if (env.decay_us >= 0)
                return envelope(c, state, p, e.note, CspPhase::decay, env.decay_volume, env.decay_us, time);
        }
        n.phase = CspPhase::sustain;
        return Error::none;
    }
    const bool vibrato = e.kind == CspEventKind::vibrato;
    const auto index = vibrato ? n.vibrato_slot : n.tremolo_slot;
    if (index == no_slot) return Error::none;
    auto& value = vibrato ? n.vibrato : n.tremolo;
    const auto delay = oscillator_update(state.oscillators.slots[index], value);
    std::uint64_t due;
    auto error = add_us(p.client_us, delay, due);
    if (error != Error::none) return error;
    if (vibrato) {
        error = pitch_voice(c, state, n.descriptor, note_pitch(n, p.channels[n.channel]), time);
    } else {
        // Unlike channel gain's one-ms current-level adjustment, tremolo
        // retargets this phase's END gain over the unchanged remaining phase.
        const auto remaining = n.envelope_end > p.client_us ? n.envelope_end - p.client_us : 0;
        if (remaining > std::numeric_limits<std::int32_t>::max()) return Error::invalid_parameter;
        Gain15 gain{};
        error = note_gain(p, n, n.envelope_to, gain);
        if (error != Error::none) return error;
        error = gain_voice(c, state, n.descriptor, gain,
                           {duration_frames(static_cast<std::int32_t>(remaining), c.config.synthesis_rate)}, time);
    }
    error = tolerate_unbound(error);
    return error == Error::none ? schedule(p, e.kind, e.note, due) : error;
}
} // namespace

namespace csp {
Error SequencePlayer::stage_sequence(std::uint32_t song) noexcept {
    CspCheckpoint* p; const CspResources* r;
    auto error = access(context_, id_, p, r, true);
    if (error != Error::none) return error;
    if (p->playback != Playback::stopped) return Error::invalid_state;
    if (song >= r->config.sequences.sequences().size()) return Error::invalid_parameter;
    return post(context_, id_, {CspControlKind::bind, 0, song});
}
Error SequencePlayer::stage_play() noexcept { return post(context_, id_, {CspControlKind::play}); }
Error SequencePlayer::stage_stop() noexcept { return post(context_, id_, {CspControlKind::stop}); }
Error SequencePlayer::stage_volume(std::uint16_t value) noexcept {
    if (value > 0x7fff) return Error::invalid_parameter;
    return post(context_, id_, {CspControlKind::volume, 0, value});
}
Error SequencePlayer::stage_tempo(std::uint32_t value) noexcept {
    if (value == 0 || value > 0x7fffffff) return Error::invalid_parameter;
    return post(context_, id_, {CspControlKind::tempo, 0, value});
}
namespace {
Error channel_post(Context* c, PlayerId id, CspControlKind kind, std::uint8_t ch, std::uint8_t value,
                   bool seven = true) noexcept {
    if (ch >= 16 || (seven && value > 127)) return Error::invalid_parameter;
    return post(c, id, {kind, ch, value});
}
}
Error SequencePlayer::stage_channel_program(std::uint8_t ch, std::uint8_t value) noexcept {
    return channel_post(context_, id_, CspControlKind::program, ch, value);
}
Error SequencePlayer::stage_channel_pan(std::uint8_t ch, std::uint8_t value) noexcept {
    return channel_post(context_, id_, CspControlKind::pan, ch, value);
}
Error SequencePlayer::stage_channel_volume(std::uint8_t ch, std::uint8_t value) noexcept {
    return channel_post(context_, id_, CspControlKind::channel_volume, ch, value);
}
Error SequencePlayer::stage_channel_priority(std::uint8_t ch, std::uint8_t value) noexcept {
    return channel_post(context_, id_, CspControlKind::priority, ch, value, false);
}
Error SequencePlayer::stage_channel_fx(std::uint8_t ch, std::uint8_t value) noexcept {
    return channel_post(context_, id_, CspControlKind::fx, ch, value, false);
}
Error SequencePlayer::stage_channel_extra_volume(std::uint8_t ch, std::int8_t value) noexcept {
    return channel_post(context_, id_, CspControlKind::extra_volume, ch, std::bit_cast<std::uint8_t>(value), false);
}
Error SequencePlayer::stage_player_extra_fx(std::int8_t value) noexcept {
    return post(context_, id_, {CspControlKind::extra_fx, 0, std::bit_cast<std::uint8_t>(value)});
}
Error SequencePlayer::stage() noexcept {
    CspCheckpoint* p; const CspResources* r;
    const auto error = access(context_, id_, p, r, true);
    if (error != Error::none) return error;
    if (p->inbox_count > p->published.size() - p->published_count) return Error::event_capacity;
    std::copy_n(p->inbox.begin(), p->inbox_count, p->published.begin() + p->published_count);
    p->published_count += p->inbox_count; p->inbox_count = 0;
    return Error::none;
}
Error SequencePlayer::status(Status& result) const noexcept {
    result = {};
    CspCheckpoint* p; const CspResources* r;
    auto error = access(context_, id_, p, r);
    if (error != Error::none) return error;
    result.playback = p->playback; result.bound = p->bound; result.ended = p->ended;
    result.song = p->song; result.scheduled_events = p->event_count;
    error = tempo(result.tempo); if (error != Error::none) return error;
    for (std::uint16_t i = 0; i < r->config.max_notes; ++i) {
        const auto& n = p->notes[i];
        if (n.phase == CspPhase::free) continue;
        ++result.active_notes;
        BindingInfo binding{};
        error = voice_binding(*context_, context_->state(), n.descriptor, binding);
        if (error != Error::none) return error;
        if (binding.bound) ++result.bound_notes;
    }
    return Error::none;
}
Error SequencePlayer::channel_status(std::uint8_t ch, ChannelStatus& result) const noexcept {
    result = {};
    CspCheckpoint* p; const CspResources* r;
    const auto error = access(context_, id_, p, r);
    if (error != Error::none) return error;
    if (ch >= 16) return Error::invalid_parameter;
    result = p->channels[ch].controls;
    return Error::none;
}
Error SequencePlayer::tempo(std::uint32_t& result) const noexcept {
    result = 0;
    CspCheckpoint* p; const CspResources* r;
    const auto error = access(context_, id_, p, r);
    if (error != Error::none) return error;
    if (p->bound) result = static_cast<std::uint32_t>(static_cast<float>(p->us_per_tick) / p->quarter_per_tick);
    return Error::none;
}
} // namespace csp

namespace detail {
Error add_sequence_player(Context& c, const csp::Config& config, PlayerId& id) noexcept {
    id = {};
    if (c.poisoned) return Error::needs_reset;
    if (c.building || c.lease != Lease::none) return Error::task_busy;
    const auto& live = c.state();
    if (live.sample_clock != 0 || live.last_service != 0) return Error::invalid_state;
    if (!config.bank || !config.sequences || config.bank_index >= config.bank.banks().size() ||
        config.max_notes == 0 || config.max_notes > csp::SequencePlayer::max_notes ||
        config.first_client_us > limit - 16000) return Error::invalid_parameter;
    const auto* bank = config.bank.banks()[config.bank_index];
    if (!bank) return Error::invalid_resource;
    bool owned = false;
    for (const auto& owner : c.catalog.owners)
        for (const auto* candidate : owner.banks()) if (candidate == bank) owned = true;
    if (!owned) return Error::foreign_resource;
    std::uint16_t slot = 0, typed = 0;
    while (slot < AudioService::max_players && live.clients[slot].live) ++slot;
    while (typed < c.csp.size() && c.csp[typed]) ++typed;
    if (slot == AudioService::max_players || typed == c.csp.size()) return Error::client_capacity;
    std::size_t available = 0;
    for (std::uint16_t i = 0; i < c.config.logical_voices; ++i) if (!live.logical[i].live) ++available;
    if (available < config.max_notes) return Error::descriptor_capacity;
    if (c.next_player == limit || c.next_voice > limit - config.max_notes) return Error::identity_exhausted;
    if (config.first_due && config.first_due->value < live.sample_clock) return Error::invalid_parameter;
    std::unique_ptr<CspCheckpointPair> pair;
    try {
        pair = std::make_unique<CspCheckpointPair>();
        pair->resources.config = config; pair->resources.bank = bank;
        const auto prepare = [&](const Instrument* instrument) -> Error {
            if (!instrument) return Error::none;
            if ((instrument->tremolo_type && !oscillator_type(instrument->tremolo_type)) ||
                (instrument->vibrato_type && !oscillator_type(instrument->vibrato_type))) return Error::invalid_resource;
            for (const auto* sound : instrument->sounds) {
                if (!sound) continue;
                if (!sound->envelope || !sound->keymap || !sound->wave) return Error::invalid_resource;
                const auto& env = *sound->envelope;
                if (env.attack_us < 0 || env.decay_us < -1 || env.release_us < 0)
                    return Error::unsupported_envelope;
                if (std::any_of(pair->resources.sounds.begin(), pair->resources.sounds.end(),
                                [sound](const auto& value) { return value.resource == sound; })) continue;
                SoundId sound_id{};
                auto error = find_sound(c, *sound, sound_id);
                if (error != Error::none) return error;
                const CatalogSound* mapped = nullptr;
                error = catalog_sound(c, sound_id, mapped);
                if (error != Error::none) return error;
                const CatalogWave* wave = nullptr;
                error = catalog_wave(c, {c.owner, mapped->wave_index}, wave);
                if (error != Error::none) return error;
                if (wave->admission != Error::none) return wave->admission;
                pair->resources.sounds.push_back({sound, sound_id, {c.owner, mapped->wave_index}});
            }
            return Error::none;
        };
        auto error = prepare(bank->percussion); if (error != Error::none) return error;
        for (const auto* instrument : bank->instruments) {
            error = prepare(instrument); if (error != Error::none) return error;
        }
    } catch (const std::bad_alloc&) { return Error::allocation_failure; }
    const auto shadow = prepare_shadow(c);
    auto& state = c.cpu[shadow];
    auto& p = pair->banks[shadow];
    p.client_us = p.poll_us = config.first_client_us;
    for (std::uint16_t i = 0; i < config.max_notes; ++i) {
        const auto error = create_voice(c, state, p.notes[i].descriptor);
        if (error != Error::none) return error;
    }
    pair->banks[shadow ^ 1u] = p;
    ClientCalendar client{};
    client.live = true; client.kind = ClientKind::csp; client.typed_index = typed;
    client.generation = client.registration_order = c.next_player + 1;
    client.next_raw_due = config.first_due ? config.first_due->value : state.sample_clock;
    state.clients[slot] = client;
    c.csp[typed] = std::move(pair); c.next_player = client.generation; c.live = shadow;
    id = {c.owner, client.generation, slot};
    return Error::none;
}
Error dispatch_sequence_player(Context& c, unsigned bank, std::uint16_t slot) noexcept {
    if (bank >= c.cpu.size() || slot >= AudioService::max_players) return Error::stale_handle;
    auto& state = c.cpu[bank]; auto& client = state.clients[slot];
    if (!client.live || client.kind != ClientKind::csp || client.typed_index >= c.csp.size() ||
        !c.csp[client.typed_index]) return Error::stale_handle;
    auto& p = c.csp_state(bank, client.typed_index);
    const auto& resources = c.csp[client.typed_index]->resources;
    const ParamTime time{client.next_raw_due & ~std::uint64_t{15}};
    if (time.value != state.sample_clock) return Error::invalid_state;
    std::size_t steps = 0;
    const auto drain = [&]() noexcept -> Error {
        for (;;) {
            const auto index = earliest(p);
            if (index == p.events.size() || p.events[index].due > p.client_us) return Error::none;
            if (++steps > max_callback_steps) return Error::event_capacity;
            const auto e = p.events[index]; p.events[index].live = false; --p.event_count;
            const auto error = event(c, state, p, resources, e, time);
            if (error != Error::none) return error;
        }
    };
    auto error = drain(); if (error != Error::none) return error;
    const auto controls = p.published_count;
    // Remove each control reservation before executing it, so events posted by
    // that control consume precisely the same bounded internal capacity.
    for (std::size_t i = 0; i < controls; ++i) {
        --p.published_count;
        error = apply_control(c, state, p, resources, p.published[i], time);
        if (error != Error::none) return error;
    }
    while (p.playback == csp::Playback::playing && p.have_next && p.sequence_due <= p.client_us) {
        if (++steps > max_callback_steps) return Error::event_capacity;
        const auto next = p.next;
        p.have_next = false;
        if (next.kind == SequenceEventKind::midi) error = midi(c, state, p, resources, next, time);
        else if (next.kind == SequenceEventKind::tempo) error = set_tempo(p, next.tempo_us_per_quarter);
        else if (next.kind == SequenceEventKind::sequence_end) {
            p.ended = true;
            // EOT stops unreleased sources; an existing release keeps its
            // endpoint, events and oscillators. Native stopping means draining,
            // not an assertion about the original player's opaque state.
            for (std::uint16_t i = 0; i < resources.config.max_notes; ++i)
                if (p.notes[i].phase != CspPhase::free && p.notes[i].phase != CspPhase::release) {
                    error = finish(c, state, p, i, time);
                    if (error != Error::none) return error;
                }
            p.playback = csp::Playback::stopping;
            break;
        }
        if (error != Error::none) return error;
        error = drain(); if (error != Error::none) return error;
        error = read_next(p); if (error != Error::none) return error;
    }
    if (p.playback == csp::Playback::stopping) {
        bool active = false;
        for (const auto& n : p.notes) if (n.phase != CspPhase::free) active = true;
        if (!active) p.playback = csp::Playback::stopped;
    }
    if (p.poll_us <= p.client_us) {
        error = add_us(p.client_us, 16000, p.poll_us); if (error != Error::none) return error;
    }
    auto due = p.poll_us;
    const auto index = earliest(p);
    if (index != p.events.size()) due = std::min(due, p.events[index].due);
    if (p.playback == csp::Playback::playing && p.have_next) due = std::min(due, p.sequence_due);
    if (due < p.client_us || due - p.client_us > std::numeric_limits<std::int32_t>::max()) return Error::identity_exhausted;
    const auto delta = rounded_frames(static_cast<std::int32_t>(due - p.client_us), c.config.synthesis_rate);
    if (delta > limit - client.next_raw_due) return Error::identity_exhausted;
    client.next_raw_due += delta; p.client_us = due;
    return Error::none;
}
} // namespace detail
} // namespace isle::audio
