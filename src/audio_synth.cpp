#include "audio_synth_internal.hpp"
#include "audio_dsp.hpp"

#include <algorithm>
#include <limits>

namespace isle::audio::detail {
namespace {
constexpr std::uint32_t chunk_frames = 160;
constexpr std::uint32_t decode_chunk = 112;
constexpr std::uint16_t stream = 0x008;
constexpr std::uint16_t decoded = 0x2a0;
constexpr std::uint16_t encoded = 0x3c0;
constexpr std::uint16_t mono = decoded;
constexpr std::uint16_t left_bus = 0x440;
constexpr std::uint16_t right_bus = 0x580;
constexpr std::uint16_t wet_left = 0x6c0;
constexpr std::uint16_t wet_right = 0x800;
constexpr std::uint16_t interleaved = 0;
static_assert(stream + chunk_frames * 4u <= decoded);
static_assert(decoded + 32u + decode_chunk * 2u <= encoded);
static_assert(encoded + 72u <= left_bus);
static_assert(mono + chunk_frames * 2u <= left_bus);
static_assert(wet_right + chunk_frames * 2u <= 0x940);
constexpr std::uint32_t align8(std::uint32_t n) noexcept { return (n + 7u) & ~7u; }
constexpr std::uint32_t align16(std::uint32_t n) noexcept { return (n + 15u) & ~15u; }

Error logical_voice(const Context& context, const CpuState& state, VoiceHandle handle,
                    const LogicalVoice*& voice) noexcept {
    if (handle.owner != context.owner || handle.owner == 0 ||
        handle.slot >= context.config.logical_voices) return Error::stale_handle;
    const auto& candidate = state.logical[handle.slot];
    if (!candidate.live || candidate.generation != handle.generation || handle.generation == 0)
        return Error::stale_handle;
    voice = &candidate;
    return Error::none;
}
Error bound_voice(const Context& context, const CpuState& state, VoiceHandle handle,
                  std::uint16_t& physical) noexcept {
    const LogicalVoice* logical = nullptr;
    const auto error = logical_voice(context, state, handle, logical);
    if (error != Error::none) return error;
    if (logical->physical == no_slot) return Error::unbound;
    physical = logical->physical;
    return Error::none;
}
bool valid_time(const CpuState& state, ParamTime time) noexcept {
    return (time.value & 15u) == 0 && time.value >= state.sample_clock;
}
bool valid_parameters(const DirectParameters& parameters) noexcept {
    std::uint16_t unused = 0;
    return parameters.gain.value <= 0x7fff && parameters.pan <= 127 && parameters.fx_mix <= 127 &&
        pitch_word(parameters.pitch, unused);
}
Error effective_time(const Context& context, const PhysicalVoice& physical,
                     std::uint64_t raw, std::uint64_t& effective) noexcept {
    const auto offset = physical.stealable ? 0u : context.config.steal.restart_frames;
    if (raw > std::numeric_limits<std::uint64_t>::max() - offset) return Error::identity_exhausted;
    effective = raw + offset;
    return Error::none;
}
void initialize_lists(const Context& context, CpuState& state) noexcept {
    for (std::uint16_t slot = 0; slot < context.config.physical_voices; ++slot) {
        auto& physical = state.physical[slot];
        physical.previous = slot == 0 ? no_slot : static_cast<std::uint16_t>(slot - 1);
        physical.next = slot + 1 == context.config.physical_voices ? no_slot : static_cast<std::uint16_t>(slot + 1);
    }
    state.free_head = 0;
    state.lists_initialized = true;
}
void unlink_voice(CpuState& state, std::uint16_t slot, std::uint16_t& head) noexcept {
    auto& physical = state.physical[slot];
    if (physical.previous == no_slot) head = physical.next;
    else state.physical[physical.previous].next = physical.next;
    if (physical.next != no_slot) state.physical[physical.next].previous = physical.previous;
    physical.next = physical.previous = no_slot;
}
void prepend_voice(CpuState& state, std::uint16_t slot, std::uint16_t& head) noexcept {
    auto& physical = state.physical[slot];
    physical.previous = no_slot;
    physical.next = head;
    if (head != no_slot) state.physical[head].previous = slot;
    head = slot;
}
void flush_retirements(const Context& context, CpuState& state) noexcept {
    // Effective frees are visited in fixed physical DSP order. Their list
    // movement is independent of parameter heap insertion order.
    for (auto slot = context.config.physical_voices; slot != 0; --slot) {
        auto& physical = state.physical[slot - 1];
        if (!physical.retire_ready) continue;
        physical.retire_ready = false;
        physical.allocated = false;
        unlink_voice(state, static_cast<std::uint16_t>(slot - 1), state.allocated_head);
        prepend_voice(state, static_cast<std::uint16_t>(slot - 1), state.retiring_head);
    }
}
Error reserve_events(const CpuState& state, std::size_t count) noexcept {
    if (count > state.parameters.size() - state.parameter_count) return Error::event_capacity;
    if (count > std::numeric_limits<std::uint64_t>::max() - state.next_parameter_sequence)
        return Error::identity_exhausted;
    return Error::none;
}
bool earlier(const ParameterEvent& left, const ParameterEvent& right) noexcept {
    return left.due < right.due || (left.due == right.due && left.sequence < right.sequence);
}
void push_event(CpuState& state, ParameterEvent event) noexcept {
    event.sequence = ++state.next_parameter_sequence;
    auto index = state.parameter_count++;
    while (index != 0) {
        const auto parent = (index - 1) / 2;
        if (!earlier(event, state.parameters[parent])) break;
        state.parameters[index] = state.parameters[parent];
        index = parent;
    }
    state.parameters[index] = event;
}
ParameterEvent pop_event(CpuState& state) noexcept {
    const auto result = state.parameters[0];
    const auto last = state.parameters[--state.parameter_count];
    if (state.parameter_count != 0) {
        std::size_t index = 0;
        while (index * 2 + 1 < state.parameter_count) {
            auto child = index * 2 + 1;
            if (child + 1 < state.parameter_count && earlier(state.parameters[child + 1], state.parameters[child]))
                ++child;
            if (!earlier(state.parameters[child], last)) break;
            state.parameters[index] = state.parameters[child];
            index = child;
        }
        state.parameters[index] = last;
    }
    return result;
}
std::array<std::uint16_t, 2> channel_gains(const Context& context, Gain15 gain, std::uint8_t pan) noexcept {
    const auto amplitude = gain.value;
    return {static_cast<std::uint16_t>((amplitude * context.coefficients.pan[pan]) >> 15),
            static_cast<std::uint16_t>((amplitude * context.coefficients.pan[127u - pan]) >> 15)};
}
std::array<std::uint16_t, 2> channel_gains(const Context& context, const RenderVoice& voice) noexcept {
    return channel_gains(context, voice.parameters.gain, voice.parameters.pan);
}
void retarget(const Context& context, RenderVoice& voice) noexcept {
    // Start/gain own the CPU clock. Pan rebases the channels without restarting
    // it, even across ENV emission; completion reads current parameter targets.
    if (voice.gain_elapsed >= voice.gain_frames) {
        voice.retarget_current = channel_gains(context, voice);
    } else if (voice.gain_elapsed == 0) {
        if (!voice.have_retarget_current) voice.retarget_current = voice.gain_from;
    } else {
        const auto& from = voice.have_retarget_current ? voice.retarget_current : voice.gain_from;
        for (std::size_t channel = 0; channel != 2; ++channel)
            voice.retarget_current[channel] = cpu_gain(from[channel], voice.gain_to[channel],
                voice.gain_rate[channel], voice.gain_elapsed, voice.gain_frames);
    }
    // CPU evaluation maps a wrapped zero to one, not signed negative values.
    // Untouched static source initialization is raw and may still emit zero.
    for (auto& value : voice.retarget_current) value = std::max<std::uint16_t>(value, 1);
    voice.have_retarget_current = true;
    voice.envelope_init = true;
}
void apply_event(Context& context, CpuState& state, const ParameterEvent& event) noexcept {
    auto& physical = state.physical[event.physical];
    // An already queued STOP belongs to its source, not a later logical owner.
    // This preserves stop(t);free(t);allocate;start(t) while a source epoch
    // prevents that stop from touching an already replaced source.
    if (!physical.allocated || physical.epoch != event.epoch) {
        if (event.kind == ParameterKind::stop && physical.current.epoch == event.source_epoch)
            physical.current.active = false;
        return;
    }
    if (event.kind == ParameterKind::retire) {
        if (physical.owner == no_slot) physical.retire_ready = true;
        return;
    }
    if (event.kind == ParameterKind::start) {
        if (!physical.pending.valid || physical.pending.epoch != event.source_epoch) return;
        const auto pending = physical.pending;
        physical.pending = PendingSource{};
        auto& voice = physical.current;
        const bool continue_chain = voice.active;
        const auto adpcm_flags = voice.adpcm_flags;
        const bool resample_init = voice.resample_init;
        const auto fraction = voice.fraction;
        voice = RenderVoice{};
        if (continue_chain) {
            // StartParams selects the new encoded source from offset zero,
            // but an unstopped live chain retains its actual DSP histories.
            voice.adpcm_flags = adpcm_flags == 1 ? 1 : 0;
            voice.resample_init = resample_init;
            voice.fraction = fraction;
        }
        voice.active = true;
        voice.epoch = pending.epoch;
        voice.binding_epoch = physical.epoch;
        voice.wave = pending.wave;
        voice.parameters = pending.parameters;
        pitch_word(voice.parameters.pitch, voice.pitch);
        const auto& wave = context.catalog.waves[voice.wave.index];
        voice.loops_remaining = wave.resource->loop != nullptr ? wave.resource->loop->count : 0;
        voice.gain_frames = pending.attack.value;
        if (pending.gain_retarget) {
            // A gain queued while replacement is deferred starts its ramp at
            // restart, from the original start amplitude at the final pan.
            if (pending.initial_attack) {
                voice.retarget_current = {1, 1};
            } else {
                voice.retarget_current = channel_gains(context, pending.initial_gain, pending.parameters.pan);
                for (auto& value : voice.retarget_current) value = std::max<std::uint16_t>(value, 1);
            }
        } else {
            voice.retarget_current = pending.attack.value == 0 ? channel_gains(context, voice) :
                std::array<std::uint16_t, 2>{1, 1};
        }
        voice.have_retarget_current = true;
        return;
    }
    if (physical.pending.valid && physical.pending.epoch == event.source_epoch) {
        auto& pending = physical.pending;
        switch (event.kind) {
        case ParameterKind::pitch: pending.parameters.pitch = event.parameters.pitch; break;
        case ParameterKind::pan: pending.parameters.pan = event.parameters.pan; break;
        case ParameterKind::fx: pending.parameters.fx_mix = event.parameters.fx_mix; break;
        case ParameterKind::gain:
            pending.parameters.gain = event.parameters.gain;
            pending.attack = event.ramp;
            pending.gain_retarget = true;
            break;
        case ParameterKind::stop: physical.pending = PendingSource{}; break;
        default: break;
        }
        return;
    }
    auto& voice = physical.current;
    if (voice.epoch != event.source_epoch) return;
    switch (event.kind) {
    case ParameterKind::pitch:
        voice.parameters.pitch = event.parameters.pitch;
        pitch_word(voice.parameters.pitch, voice.pitch);
        break;
    case ParameterKind::pan: {
        retarget(context, voice);
        voice.parameters.pan = event.parameters.pan;
        break;
    }
    case ParameterKind::fx:
        retarget(context, voice);
        voice.parameters.fx_mix = event.parameters.fx_mix;
        break;
    case ParameterKind::gain:
    case ParameterKind::kill:
        retarget(context, voice);
        voice.gain_frames = event.ramp.value;
        voice.gain_elapsed = 0;
        voice.parameters.gain = event.parameters.gain;
        break;
    case ParameterKind::stop: voice.active = false; break;
    default: break;
    }
}
Error apply_now(Context& context, CpuState& state) noexcept {
    return advance_parameters(context, state, ParamTime{state.sample_clock});
}
Error control_event(Context& context, CpuState& state, VoiceHandle handle, ParameterEvent event) noexcept {
    if (!valid_time(state, ParamTime{event.due})) return Error::invalid_parameter;
    std::uint16_t index = no_slot;
    auto error = bound_voice(context, state, handle, index);
    if (error != Error::none) return error;
    auto& physical = state.physical[index];
    const auto source = physical.pending.valid ? physical.pending.epoch :
        physical.current.binding_epoch == physical.epoch ? physical.current.epoch : 0;
    if (source == 0) return Error::invalid_state;
    error = effective_time(context, physical, event.due, event.due);
    if (error != Error::none) return error;
    error = reserve_events(state, 1);
    if (error != Error::none) return error;
    event.physical = index;
    event.epoch = physical.epoch;
    event.source_epoch = source;
    push_event(state, event);
    return apply_now(context, state);
}

// Only DSP command writers mutate command RAM. No start/control path resets
// persistent DSP memory: actual A_INIT execution owns that transition.
class Renderer : public DspWriter {
public:
    explicit Renderer(Context& context) noexcept : DspWriter(context) {}
    void envelope(RenderVoice& voice, std::uint32_t frames, std::uint32_t state_address) noexcept {
        if (voice.envelope_init) {
            const auto& current = voice.retarget_current;
            const auto target = channel_gains(context_, voice);
            for (std::size_t channel = 0; channel != 2; ++channel) {
                const std::uint32_t side = channel == 0 ? 0x00020000u : 0u;
                const auto from = current[channel];
                const auto to = target[channel];
                const auto rate = envelope_rate(from, to, voice.gain_frames, context_.coefficients);
                voice.gain_from[channel] = from;
                voice.gain_to[channel] = to;
                voice.gain_rate[channel] = rate;
                context_.emit(0x09040000u | side | from, 0);
                context_.emit(0x09000000u | side | to, rate);
            }
            const auto mix = voice.parameters.fx_mix;
            const auto dry = context_.coefficients.pan[mix];
            const auto wet = context_.coefficients.pan[127u - mix];
            context_.emit(0x09080000u | dry, static_cast<std::uint32_t>(wet));
            voice.have_retarget_current = false;
        }
        buffers(mono, left_bus, static_cast<std::uint16_t>(frames * 2u));
        context_.emit(0x08080000u | right_bus, (static_cast<std::uint32_t>(wet_left) << 16) | wet_right);
        if (address(state_address + 0x80, 80))
            context_.emit(0x03080000u | (voice.envelope_init ? 0x00010000u : 0u), state_address + 0x80);
        voice.envelope_init = false;
        voice.gain_elapsed += std::min(frames, voice.gain_frames - voice.gain_elapsed);
    }
    void source(RenderVoice& voice, const CatalogWave& wave, std::uint32_t state_address,
                std::uint32_t needed) noexcept {
        if (voice.source_finished) {
            if (needed != 0) clear(stream, static_cast<std::uint16_t>(needed * 2u));
            voice.cursor = wave.sample_frames;
            return;
        }
        std::uint32_t written = 0;
        const Loop* loop = wave.resource->loop;
        while (written < needed && context_.write_error == Error::none) {
            if (loop != nullptr && voice.loops_remaining != 0 && voice.cursor == loop->end) {
                if (voice.loops_remaining != 0xffffffffu) ++voice.deferred_loops;
                voice.cursor = loop->start;
                voice.decoded_to = (loop->start & ~15u) + 16u;
                voice.adpcm_flags = 2;
            }
            if (voice.cursor == wave.sample_frames) {
                const auto count = needed - written;
                clear(static_cast<std::uint16_t>(stream + written * 2u), static_cast<std::uint16_t>(count * 2u));
                break;
            }
            const auto boundary = loop != nullptr && voice.loops_remaining != 0 ? loop->end : wave.sample_frames;
            const auto take = std::min(needed - written, boundary - voice.cursor);
            const auto available = voice.decoded_to - voice.cursor;
            if (available != 0) {
                const auto copied = std::min(take, available);
                if (voice.adpcm_flags == 2) {
                    buffers(encoded, decoded, 0);
                    if (address(wave.loop_address, 32)) context_.emit(0x0f000000u, wave.loop_address);
                    if (address(state_address, 32)) context_.emit(0x01020000u, state_address);
                    voice.adpcm_flags = 0;
                } else {
                    load(state_address, decoded, 32);
                }
                move(static_cast<std::uint16_t>(decoded + 32u - available * 2u),
                     static_cast<std::uint16_t>(stream + written * 2u), static_cast<std::uint16_t>(copied * 2u));
                voice.cursor += copied;
                written += copied;
                continue;
            }
            const auto copied = std::min(take, decode_chunk);
            const auto decode_samples = align16(copied);
            const auto bytes = (decode_samples / 16u) * 9u;
            const auto source_address = wave.sample_address + (voice.cursor / 16u) * 9u;
            const auto prefix = source_address & 7u;
            load(source_address & ~7u, encoded, static_cast<std::uint16_t>(align8(prefix + bytes)));
            buffers(static_cast<std::uint16_t>(encoded + prefix), decoded, static_cast<std::uint16_t>(decode_samples * 2u));
            if (voice.adpcm_flags == 2 && address(wave.loop_address, 32))
                context_.emit(0x0f000000u, wave.loop_address);
            if (address(state_address, 32))
                context_.emit(0x01000000u | (static_cast<std::uint32_t>(voice.adpcm_flags) << 16), state_address);
            voice.adpcm_flags = 0;
            voice.decoded_to += decode_samples;
            move(static_cast<std::uint16_t>(decoded + 32u), static_cast<std::uint16_t>(stream + written * 2u),
                 static_cast<std::uint16_t>(copied * 2u));
            voice.cursor += copied;
            written += copied;
        }
    }
    void visit(RenderVoice& voice, std::uint16_t slot, std::uint32_t frames) noexcept {
        const auto& wave = context_.catalog.waves[voice.wave.index];
        const auto state_address = AudioService::state_address + slot * AudioService::state_stride;
        if (address(wave.book_address, wave.book_bytes))
            context_.emit(0x0b000000u | wave.book_bytes, wave.book_address);
        const auto total = static_cast<std::uint64_t>(voice.fraction) +
            static_cast<std::uint64_t>(voice.pitch) * 2u * frames;
        const auto needed = static_cast<std::uint32_t>(total >> 16);
        if (needed * 2u > decoded - stream) {
            context_.write_error = Error::address_range;
            return;
        }
        source(voice, wave, state_address, needed);
        buffers(stream, mono, static_cast<std::uint16_t>(frames * 2u));
        if (address(state_address + 0x40, 32))
            context_.emit(0x05000000u | (voice.resample_init ? 0x00010000u : 0u) | voice.pitch, state_address + 0x40);
        voice.resample_init = false;
        voice.fraction = static_cast<std::uint32_t>(total & 0xffffu);
        envelope(voice, frames, state_address);
    }
    void output(std::uint32_t offset, std::uint32_t frames) noexcept {
        buffers(0, interleaved, static_cast<std::uint16_t>(frames * 2u));
        // AUX return kernels scale the destination even when the wet send is0.
        context_.emit(0x0c007fffu, (static_cast<std::uint32_t>(wet_left) << 16) | left_bus);
        context_.emit(0x0c007fffu, (static_cast<std::uint32_t>(wet_right) << 16) | right_bus);
        context_.emit(0x0d000000u, (static_cast<std::uint32_t>(left_bus) << 16) | right_bus);
        save(interleaved, AudioService::pcm_address + offset * 4u, static_cast<std::uint16_t>(frames * 4u));
    }
};
void consume_loops(Context& context, RenderVoice& voice) noexcept {
    if (!voice.active) return;
    if (voice.loops_remaining != 0xffffffffu)
        voice.loops_remaining -= std::min(voice.loops_remaining, voice.deferred_loops);
    voice.deferred_loops = 0;
    voice.source_finished = voice.loops_remaining == 0 &&
        voice.decoded_to == context.catalog.waves[voice.wave.index].sample_frames;
}
bool suppress_active_start_prefix(const CpuState& state, std::uint16_t slot,
                                  std::uint64_t outer_end) noexcept {
    const auto& physical = state.physical[slot];
    if (!physical.current.active || !physical.pending.valid || !physical.stealable ||
        physical.current.binding_epoch == physical.epoch ||
        physical.pending.due <= state.sample_clock || physical.pending.due > outer_end)
        return false;
    // A stop before this start makes it a fresh chain, including the queued
    // stop for a stolen tail. Such a chain must retain its preceding pull.
    for (std::size_t index = 0; index < state.parameter_count; ++index) {
        const auto& event = state.parameters[index];
        if (event.kind == ParameterKind::stop && event.physical == slot &&
            event.source_epoch == physical.current.epoch && event.due <= physical.pending.due)
            return false;
    }
    // An unstopped free/reallocated chain discards the preceding portion of
    // the start-bearing task-relative160 pull, including its right endpoint.
    // Do not advance the old decoder, resampler or envelope through that gap.
    return true;
}
} // namespace

Error create_voice(Context& context, CpuState& state, VoiceHandle& result) noexcept {
    for (std::uint16_t index = 0; index < context.config.logical_voices; ++index) {
        auto& logical = state.logical[index];
        if (logical.live) continue;
        if (context.next_voice == std::numeric_limits<std::uint64_t>::max()) return Error::identity_exhausted;
        logical = LogicalVoice{};
        logical.live = true;
        logical.generation = ++context.next_voice;
        result = VoiceHandle{context.owner, logical.generation, index};
        return Error::none;
    }
    return Error::descriptor_capacity;
}
Error destroy_voice(Context& context, CpuState& state, VoiceHandle handle) noexcept {
    const LogicalVoice* logical = nullptr;
    const auto error = logical_voice(context, state, handle, logical);
    if (error != Error::none) return error;
    if (logical->physical != no_slot) return Error::invalid_state;
    state.logical[handle.slot] = LogicalVoice{};
    return Error::none;
}
Error allocate_voice(Context& context, CpuState& state, VoiceHandle handle, std::int16_t priority,
                     ParamTime time, Allocation& result) noexcept {
    const LogicalVoice* logical = nullptr;
    auto error = logical_voice(context, state, handle, logical);
    if (error != Error::none) return error;
    if (!valid_time(state, time)) return Error::invalid_parameter;
    if (logical->physical != no_slot) return Error::invalid_state;
    std::uint16_t selected = !state.lists_initialized ? 0 :
        state.retiring_head != no_slot ? state.retiring_head : state.free_head;
    const bool fresh = selected != no_slot;
    if (!fresh) {
        for (std::uint16_t index = 0; index < context.config.physical_voices; ++index) {
            const auto& candidate = state.physical[index];
            if (!candidate.stealable || candidate.owner == no_slot) continue;
            if (selected == no_slot || state.logical[candidate.owner].priority < state.logical[state.physical[selected].owner].priority ||
                (state.logical[candidate.owner].priority == state.logical[state.physical[selected].owner].priority &&
                 candidate.allocation_age < state.physical[selected].allocation_age)) selected = index;
        }
        if (selected == no_slot || priority < state.logical[state.physical[selected].owner].priority)
            return Error::voice_unavailable;
    }
    auto& physical = state.physical[selected];
    const bool delayed = !fresh && physical.current.active;
    if (state.next_binding_epoch == std::numeric_limits<std::uint64_t>::max() ||
        (fresh && state.next_allocation_age == std::numeric_limits<std::uint64_t>::max()) ||
        (delayed && time.value > std::numeric_limits<std::uint64_t>::max() - context.config.steal.restart_frames))
        return Error::identity_exhausted;
    error = reserve_events(state, delayed ? 2 : 0);
    if (error != Error::none) return error;
    Allocation allocation{};
    if (!fresh && physical.owner != no_slot) {
        auto& old = state.logical[physical.owner];
        allocation.displaced = VoiceHandle{context.owner, old.generation, physical.owner};
        old.physical = no_slot;
    }
    if (fresh) {
        if (!state.lists_initialized) initialize_lists(context, state);
        auto& source_head = state.retiring_head != no_slot ? state.retiring_head : state.free_head;
        unlink_voice(state, selected, source_head);
        prepend_voice(state, selected, state.allocated_head);
        // Ordinary free does not stop a live chain. A fresh allocation keeps
        // that tail until a source-qualified stop or the new source starts.
        physical.stealable = true;
        physical.allocated = true;
        physical.allocation_age = ++state.next_allocation_age;
    } else {
        // Displacement excludes the physical binding immediately, even if
        // its old source has never started. Only free/reclaim restores it.
        physical.stealable = false;
    }
    physical.owner = handle.slot;
    physical.epoch = ++state.next_binding_epoch;
    physical.pending = PendingSource{};
    physical.restart_due = delayed ? time.value + context.config.steal.restart_frames : 0;
    if (delayed) {
        ParameterEvent kill{};
        kill.kind = ParameterKind::kill;
        kill.physical = selected;
        kill.due = time.value;
        kill.epoch = physical.epoch;
        kill.source_epoch = physical.current.epoch;
        kill.ramp = RampFrames{context.config.steal.kill_frames};
        push_event(state, kill);
        kill.kind = ParameterKind::stop;
        kill.due = physical.restart_due;
        push_event(state, kill);
    }
    state.logical[handle.slot].physical = selected;
    state.logical[handle.slot].priority = priority;
    allocation.binding = BindingInfo{true, selected, physical.epoch};
    result = allocation;
    return apply_now(context, state);
}
Error priority_voice(Context& context, CpuState& state, VoiceHandle handle, std::int16_t priority) noexcept {
    const LogicalVoice* logical = nullptr;
    const auto error = logical_voice(context, state, handle, logical);
    if (error != Error::none) return error;
    state.logical[handle.slot].priority = priority;
    return Error::none;
}
Error start_voice(Context& context, CpuState& state, VoiceHandle handle, WaveId wave,
                  DirectParameters parameters, RampFrames attack, ParamTime time) noexcept {
    if (!valid_time(state, time) || (attack.value & 15u) != 0 || !valid_parameters(parameters))
        return Error::invalid_parameter;
    const CatalogWave* resource = nullptr;
    auto error = catalog_wave(context, wave, resource);
    if (error != Error::none) return error;
    if (resource->admission != Error::none) return resource->admission;
    std::uint16_t index = no_slot;
    error = bound_voice(context, state, handle, index);
    if (error != Error::none) return error;
    error = reserve_events(state, 1);
    if (error != Error::none) return error;
    auto& physical = state.physical[index];
    std::uint64_t due = 0;
    error = effective_time(context, physical, time.value, due);
    if (error != Error::none) return error;
    due = std::max(due, physical.restart_due);
    const auto source_epoch = state.next_parameter_sequence + 1;
    physical.pending = PendingSource{true, source_epoch, due, wave, parameters, attack,
                                    parameters.gain, attack.value != 0, false};
    ParameterEvent event{};
    event.kind = ParameterKind::start;
    event.physical = index;
    event.due = due;
    event.epoch = physical.epoch;
    event.source_epoch = source_epoch;
    push_event(state, event);
    return apply_now(context, state);
}
Error pitch_voice(Context& context, CpuState& state, VoiceHandle handle, float pitch, ParamTime time) noexcept {
    std::uint16_t unused = 0;
    if (!pitch_word(pitch, unused)) return Error::invalid_parameter;
    ParameterEvent event{};
    event.kind = ParameterKind::pitch; event.due = time.value; event.parameters.pitch = pitch;
    return control_event(context, state, handle, event);
}
Error pan_voice(Context& context, CpuState& state, VoiceHandle handle, std::uint8_t pan, ParamTime time) noexcept {
    if (pan > 127) return Error::invalid_parameter;
    ParameterEvent event{};
    event.kind = ParameterKind::pan; event.due = time.value; event.parameters.pan = pan;
    return control_event(context, state, handle, event);
}
Error fx_voice(Context& context, CpuState& state, VoiceHandle handle, std::uint8_t mix, ParamTime time) noexcept {
    if (mix > 127) return Error::invalid_parameter;
    ParameterEvent event{};
    event.kind = ParameterKind::fx; event.due = time.value; event.parameters.fx_mix = mix;
    return control_event(context, state, handle, event);
}
Error gain_voice(Context& context, CpuState& state, VoiceHandle handle, Gain15 gain, RampFrames ramp, ParamTime time) noexcept {
    if (gain.value > 0x7fff || (ramp.value & 15u) != 0) return Error::invalid_parameter;
    ParameterEvent event{};
    event.kind = ParameterKind::gain; event.due = time.value; event.parameters.gain = gain; event.ramp = ramp;
    return control_event(context, state, handle, event);
}
Error stop_voice(Context& context, CpuState& state, VoiceHandle handle, ParamTime time) noexcept {
    ParameterEvent event{};
    event.kind = ParameterKind::stop; event.due = time.value;
    return control_event(context, state, handle, event);
}
Error free_voice(Context& context, CpuState& state, VoiceHandle handle, ParamTime time) noexcept {
    if (!valid_time(state, time)) return Error::invalid_parameter;
    std::uint16_t index = no_slot;
    auto error = bound_voice(context, state, handle, index);
    if (error != Error::none) return error;
    auto& physical = state.physical[index];
    const bool deferred = !physical.stealable;
    if (deferred && time.value > std::numeric_limits<std::uint64_t>::max() - context.config.steal.restart_frames)
        return Error::identity_exhausted;
    error = reserve_events(state, deferred ? 1 : 0);
    if (error != Error::none) return error;
    state.logical[handle.slot].physical = no_slot;
    physical.owner = no_slot;
    physical.pending = PendingSource{};
    if (!deferred) {
        physical.allocated = false;
        unlink_voice(state, index, state.allocated_head);
        prepend_voice(state, index, state.retiring_head);
        physical.restart_due = 0;
        return Error::none;
    }
    ParameterEvent event{};
    event.kind = ParameterKind::retire;
    event.physical = index;
    event.epoch = physical.epoch;
    event.source_epoch = physical.current.epoch;
    event.due = time.value + context.config.steal.restart_frames;
    push_event(state, event);
    return apply_now(context, state);
}
Error voice_binding(const Context& context, const CpuState& state, VoiceHandle handle, BindingInfo& result) noexcept {
    const LogicalVoice* logical = nullptr;
    const auto error = logical_voice(context, state, handle, logical);
    if (error != Error::none) return error;
    result = logical->physical == no_slot ? BindingInfo{} :
        BindingInfo{true, logical->physical, state.physical[logical->physical].epoch};
    return Error::none;
}
Error advance_parameters(Context& context, CpuState& state, ParamTime boundary) noexcept {
    if (!valid_time(state, boundary)) return Error::invalid_parameter;
    while (state.parameter_count != 0 && state.parameters[0].due <= boundary.value)
        apply_event(context, state, pop_event(state));
    flush_retirements(context, state);
    return Error::none;
}
void collect_voices(CpuState& state) noexcept {
    while (state.retiring_head != no_slot) {
        const auto slot = state.retiring_head;
        unlink_voice(state, slot, state.retiring_head);
        prepend_voice(state, slot, state.free_head);
    }
}
Error render(Context& context, CpuState& state, std::uint32_t frames, std::uint32_t task_offset,
             std::uint32_t task_frames) noexcept {
    if ((frames & 15u) != 0 || (task_offset & 15u) != 0 || (task_frames & 15u) != 0 ||
        task_frames > AudioService::max_frames || task_offset > task_frames || frames > task_frames - task_offset)
        return Error::invalid_parameter;
    if (state.sample_clock > std::numeric_limits<std::uint64_t>::max() - (task_frames - task_offset))
        return Error::identity_exhausted;
    Renderer renderer(context);
    std::uint32_t offset = 0;
    while (offset < frames && context.write_error == Error::none) {
        auto error = advance_parameters(context, state, ParamTime{state.sample_clock});
        if (error != Error::none) return error;
        const auto output_offset = task_offset + offset;
        const auto outer_remaining = std::min(chunk_frames - (output_offset % chunk_frames),
                                              task_frames - output_offset);
        const auto outer_end = state.sample_clock + outer_remaining;
        auto count = std::min(outer_remaining, frames - offset);
        if (state.parameter_count != 0 && state.parameters[0].due < state.sample_clock + count)
            count = static_cast<std::uint32_t>(state.parameters[0].due - state.sample_clock);
        renderer.clear(left_bus, static_cast<std::uint16_t>(count * 2u));
        renderer.clear(right_bus, static_cast<std::uint16_t>(count * 2u));
        renderer.clear(wet_left, static_cast<std::uint16_t>(count * 2u));
        renderer.clear(wet_right, static_cast<std::uint16_t>(count * 2u));
        for (auto slot = context.config.physical_voices; slot != 0; --slot) {
            auto& voice = state.physical[slot - 1].current;
            if (voice.active && !suppress_active_start_prefix(state, static_cast<std::uint16_t>(slot - 1), outer_end))
                renderer.visit(voice, static_cast<std::uint16_t>(slot - 1), count);
        }
        const auto wet_output = render_effects(context, state.effects, task_offset + offset, count, task_frames);
        if (wet_output.frames != 0) renderer.output(wet_output.task_offset, wet_output.frames);
        offset += count;
        state.sample_clock += count;
        if ((task_offset + offset) % chunk_frames == 0 || task_offset + offset == task_frames)
            for (std::uint16_t slot = 0; slot < context.config.physical_voices; ++slot)
                consume_loops(context, state.physical[slot].current);
    }
    return context.write_error;
}
} // namespace isle::audio::detail

namespace isle::audio {
namespace {
Error control_access(const detail::Context* context) noexcept {
    if (context == nullptr) return Error::invalid_state;
    if (context->building) return Error::task_busy;
    if (context->poisoned) return Error::needs_reset;
    return Error::none;
}
}
Error Synth::create_voice(VoiceHandle& handle) noexcept {
    const auto error = control_access(context_);
    return error == Error::none ? detail::create_voice(*context_, context_->state(), handle) : error;
}
Error Synth::destroy_voice(VoiceHandle handle) noexcept {
    const auto error = control_access(context_);
    return error == Error::none ? detail::destroy_voice(*context_, context_->state(), handle) : error;
}
Error Synth::allocate(VoiceHandle handle, std::int16_t priority, ParamTime time, Allocation& allocation) noexcept {
    const auto error = control_access(context_);
    return error == Error::none ? detail::allocate_voice(*context_, context_->state(), handle, priority, time, allocation) : error;
}
Error Synth::set_priority(VoiceHandle handle, std::int16_t priority) noexcept {
    const auto error = control_access(context_);
    return error == Error::none ? detail::priority_voice(*context_, context_->state(), handle, priority) : error;
}
Error Synth::start(VoiceHandle handle, WaveId wave, DirectParameters parameters, RampFrames attack, ParamTime time) noexcept {
    const auto error = control_access(context_);
    return error == Error::none ? detail::start_voice(*context_, context_->state(), handle, wave, parameters, attack, time) : error;
}
Error Synth::set_pitch(VoiceHandle handle, float pitch, ParamTime time) noexcept {
    const auto error = control_access(context_);
    return error == Error::none ? detail::pitch_voice(*context_, context_->state(), handle, pitch, time) : error;
}
Error Synth::set_pan(VoiceHandle handle, std::uint8_t pan, ParamTime time) noexcept {
    const auto error = control_access(context_);
    return error == Error::none ? detail::pan_voice(*context_, context_->state(), handle, pan, time) : error;
}
Error Synth::set_fx_mix(VoiceHandle handle, std::uint8_t mix, ParamTime time) noexcept {
    const auto error = control_access(context_);
    return error == Error::none ? detail::fx_voice(*context_, context_->state(), handle, mix, time) : error;
}
Error Synth::set_gain(VoiceHandle handle, Gain15 gain, RampFrames ramp, ParamTime time) noexcept {
    const auto error = control_access(context_);
    return error == Error::none ? detail::gain_voice(*context_, context_->state(), handle, gain, ramp, time) : error;
}
Error Synth::stop(VoiceHandle handle, ParamTime time) noexcept {
    const auto error = control_access(context_);
    return error == Error::none ? detail::stop_voice(*context_, context_->state(), handle, time) : error;
}
Error Synth::free(VoiceHandle handle, ParamTime time) noexcept {
    const auto error = control_access(context_);
    return error == Error::none ? detail::free_voice(*context_, context_->state(), handle, time) : error;
}
Error Synth::binding(VoiceHandle handle, BindingInfo& binding) const noexcept {
    const auto error = control_access(context_);
    return error == Error::none ? detail::voice_binding(*context_, context_->state(), handle, binding) : error;
}
ParamTime Synth::control_frontier() const noexcept {
    return ParamTime{context_ == nullptr ? 0 : context_->state().sample_clock};
}
} // namespace isle::audio
