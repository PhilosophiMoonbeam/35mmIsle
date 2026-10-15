#include "audio_dsp.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <numbers>

namespace isle::audio::detail {
namespace {
constexpr std::uint32_t outer_capacity = 160;
constexpr std::uint16_t section_input = 0, section_output = 0x140;
constexpr std::uint16_t wet_left = 0x6c0, wet_right = 0x800;
constexpr std::array<std::uint16_t, 4> buses{0x440, 0x580, wet_left, wet_right};
static_assert(AudioService::state_address + AudioService::max_physical_voices *
              AudioService::state_stride == effects_ring_address);
static_assert(effects_ring_address + effects_ring_bytes == effects_section_address);
static_assert(effects_section_address + effects::max_sections * effects_section_stride ==
              effects_staging_address);
static_assert(effects_book_bytes + effects_mutable_section_bytes <= effects_section_stride);
static_assert(effects_staging_address + effects_staging_bytes <= AudioService::resource_address);
static_assert((effects_ring_address | effects_section_address | effects_staging_address |
               effects_section_stride | effects_book_bytes | effects_history_bytes |
               effects_resample_bytes) % 8u == 0);

// Strict target FP options preserve these explicit float stages without FMA
// contraction or reassociation; no volatile spill/reload barriers are needed.
float chorus_amplitude(std::uint32_t depth) noexcept {
    constexpr double scale = std::numbers::ln2 / 120000.0;
    return static_cast<float>(static_cast<double>(depth) * scale);
}
float chorus_increment(std::uint32_t rate, std::uint32_t sample_rate) noexcept {
    const float scaled = static_cast<float>(static_cast<double>(rate) / 2000.0);
    return static_cast<float>(static_cast<double>(scaled) / sample_rate);
}

class WetRenderer final : public DspWriter {
public:
    explicit WetRenderer(Context& context) noexcept : DspWriter(context), program_(context.effects) {}
    void mix(std::int16_t gain, std::uint16_t input, std::uint16_t output) noexcept {
        context_.emit(0x0c000000u | std::bit_cast<std::uint16_t>(gain),
                      (static_cast<std::uint32_t>(input) << 16) | output);
    }
    std::uint32_t port(std::uint32_t cursor, std::uint32_t samples) const noexcept {
        const auto delay = samples * 2u;
        return cursor >= delay ? cursor - delay : cursor + program_.ring_bytes - delay;
    }
    // A complete operand is assembled before any graph processing. Split DMA is
    // a transport detail, never a split feedback/filter invocation. End-cursor
    // zero-byte transfers are deliberately retained.
    void ring_load(std::uint32_t offset, std::uint16_t dest, std::uint16_t bytes) noexcept {
        const auto first = static_cast<std::uint16_t>(std::min<std::uint32_t>(bytes, program_.ring_bytes - offset));
        load(effects_ring_address + offset, dest, first);
        if (first != bytes)
            load(effects_ring_address, static_cast<std::uint16_t>(dest + first),
                 static_cast<std::uint16_t>(bytes - first));
        buffers(section_input, section_input, bytes);
    }
    void ring_save(std::uint16_t source, std::uint32_t offset, std::uint16_t bytes) noexcept {
        const auto first = static_cast<std::uint16_t>(std::min<std::uint32_t>(bytes, program_.ring_bytes - offset));
        save(source, effects_ring_address + offset, first);
        if (first != bytes)
            save(static_cast<std::uint16_t>(source + first), effects_ring_address,
                 static_cast<std::uint16_t>(bytes - first));
        buffers(0, source, bytes);
    }
    void chorus_load(std::size_t index, std::uint32_t output, ChorusState& state,
                     std::uint32_t frames) noexcept {
        const auto& modulation = program_.chorus[index];
        const float advance = modulation.increment * static_cast<float>(frames);
        state.phase += advance;
        if (state.phase > 0.5f) state.phase -= 1.0f;
        const float triangle = 1.0f - 4.0f * std::abs(state.phase);
        const float delta = triangle * modulation.amplitude * 32768.0f;
        const auto word = static_cast<std::uint16_t>(32768 + static_cast<std::int32_t>(delta));
        const auto total = frames * word + state.carry;
        const auto pull = total >> 15;
        const auto tap = (output + state.drift) % program_.ring_bytes;
        const auto alignment = static_cast<std::uint16_t>(tap & 7u);
        const auto load_bytes = static_cast<std::uint16_t>(pull * 2u + alignment);
        // LOAD rounds nonzero counts up to eight bytes. Refuse an actual
        // scratch/bus overlap, not a fitted depth/rate admission threshold.
        if (((static_cast<std::uint32_t>(load_bytes) + 7u) & ~7u) > buses[0] - 0x280u) {
            context_.write_error = Error::address_range;
            return;
        }
        // The aligned start and every ring boundary are eight-byte aligned.
        // Retain LOAD count0: the real command emits no input DMA but RESAMPLE
        // still executes, including INIT/continuation history ownership.
        auto offset = tap & ~7u;
        std::uint16_t remaining = load_bytes, dest = 0x280;
        do {
            const auto bytes = static_cast<std::uint16_t>(
                std::min<std::uint32_t>(remaining, program_.ring_bytes - offset));
            load(effects_ring_address + offset, dest, bytes);
            remaining = static_cast<std::uint16_t>(remaining - bytes);
            dest = static_cast<std::uint16_t>(dest + bytes);
            offset = 0;
        } while (remaining != 0);
        buffers(static_cast<std::uint16_t>(0x280u + alignment), section_output,
                static_cast<std::uint16_t>(frames * 2u));
        if (address(effects_resample_address(index), effects_resample_bytes))
            context_.emit(0x05000000u | (state.initialized ? 0u : 0x00010000u) | word,
                          effects_resample_address(index));
        state.initialized = true;
        state.carry = static_cast<std::uint16_t>(total & 0x7fffu);
        // Bounded modulo arithmetic also handles negative cumulative drift and
        // arbitrarily many lifetimes without signed overflow.
        state.drift = (state.drift + program_.ring_bytes + pull * 2u - frames * 2u) % program_.ring_bytes;
    }
    void graph(EffectsState& state, std::uint32_t frames) noexcept {
        const auto bytes = static_cast<std::uint16_t>(frames * 2u);
        buffers(0, 0, bytes);
        // Literal signed, in-place downmix. Two positive products are not the
        // same DSP operation, including destination scaling and saturation.
        context_.emit(0x0c00da83u, 0x06c006c0u);
        context_.emit(0x0c005a82u, 0x080006c0u);
        ring_save(wet_left, state.cursor, bytes);
        clear(wet_right, bytes);
        for (std::size_t index = 0; index < program_.section_count; ++index) {
            const auto& section = program_.sections[index];
            const auto input = port(state.cursor, section.input_samples);
            const auto output = port(state.cursor, section.output_samples);
            const bool chorus = section.chorus && section.chorus->rate != 0;
            ring_load(input, section_input, bytes);
            if (chorus) chorus_load(index, output, state.chorus[index], frames);
            else ring_load(output, section_output, bytes);
            if (context_.write_error != Error::none) return;
            if (section.feedforward != 0) {
                mix(section.feedforward, section_input, section_output);
                // This pre-feedback commit is observable when ring windows
                // alias, even though a non-pole output is saved again below.
                if (!chorus && !section.pole) ring_save(section_output, output, bytes);
            }
            if (section.feedback != 0) {
                mix(section.feedback, section_output, section_input);
                // Sealed combined graph: only nonzero FB commits input.
                // FF-only, pole-only and pure chorus have no input SAVE.
                ring_save(section_input, input, bytes);
            }
            if (section.pole) {
                buffers(section_output, section_output, bytes);
                context_.emit(0x0b000000u | effects_book_bytes, effects_book_address(index));
                const auto bit = static_cast<std::uint16_t>(1u << index);
                const auto init = (state.initialized_poles & bit) == 0 ? 0x00010000u : 0u;
                context_.emit(0x0e000000u | init | section.pole->gain, effects_history_address(index));
                state.initialized_poles = static_cast<std::uint16_t>(state.initialized_poles | bit);
                // LOADADPCM shares the RSP coefficient table with voice books.
                // Renderer::visit reloads each voice book unconditionally; no
                // book cache survives this graph or a callback subdivision.
            }
            if (!chorus) ring_save(section_output, output, bytes);
            if (section.gain != 0) mix(section.gain, section_output, wet_right);
        }
        move(wet_right, wet_left, bytes);
        state.cursor += bytes;
        if (state.cursor > program_.ring_bytes) state.cursor -= program_.ring_bytes;
    }
private:
    const EffectsProgram& program_;
};
} // namespace

Error validate_effects(effects::Config config) noexcept {
    switch (config.routing) {
    case effects::Routing::direct:
        return config.ring_samples == 0 && config.sections.empty() ? Error::none : Error::invalid_parameter;
    case effects::Routing::network:
        break;
    default:
        return Error::invalid_parameter;
    }
    if (config.ring_samples < outer_capacity || config.ring_samples > effects::max_ring_samples ||
        (config.ring_samples & 3u) != 0 || config.sections.empty() ||
        config.sections.size() > effects::max_sections)
        return Error::invalid_parameter;
    const auto max_offset = config.ring_samples - outer_capacity;
    for (const auto& section : config.sections) {
        if ((section.input_samples & 3u) != 0 || (section.output_samples & 3u) != 0 ||
            section.input_samples > max_offset || section.output_samples > max_offset)
            return Error::invalid_parameter;
        if (section.chorus && section.chorus->rate != 0) {
            const auto amplitude = chorus_amplitude(section.chorus->depth);
            if (!(amplitude < 1.0f)) return Error::invalid_parameter;
            const auto maximum_word = 32768u + static_cast<std::uint32_t>(amplitude * 32768.0f);
            const auto maximum_pull = (outer_capacity * maximum_word + 32767u) >> 15;
            // Worst case: original160, carry32767, six alignment bytes, then
            // actual LOAD rounding to eight. No public lifetime may overwrite
            // the dry bus at440. This is geometry, not a corpus-depth cap;
            // depth50000 remains admitted, including its open .17.9 residual.
            if (((maximum_pull * 2u + 6u + 7u) & ~7u) > buses[0] - 0x280u)
                return Error::invalid_parameter;
        }
    }
    return Error::none;
}
Error build_effects(Context& context, effects::Config config) noexcept {
    const auto error = validate_effects(config);
    if (error != Error::none) return error;
    if (!context.memory) return Error::invalid_state;
    // The measured recurrence subtracts one period, not arbitrary modulo.
    // At most one period per largest outer preserves its centered phase.
    if (config.routing == effects::Routing::network) {
        if (context.config.synthesis_rate == 0) return Error::invalid_parameter;
        for (const auto& section : config.sections)
            if (section.chorus && section.chorus->rate != 0 &&
                chorus_increment(section.chorus->rate, context.config.synthesis_rate) *
                    static_cast<float>(outer_capacity) > 1.0f)
                return Error::invalid_parameter;
    }
    auto& program = context.effects;
    program.routing = config.routing;
    program.ring_bytes = config.ring_samples * 2u;
    program.section_count = config.sections.size();
    std::copy(config.sections.begin(), config.sections.end(), program.sections.begin());
    for (std::size_t index = 0; index < program.section_count; ++index) {
        const auto& chorus = program.sections[index].chorus;
        program.chorus[index] = chorus && chorus->rate != 0
            ? ChorusProgram{chorus_increment(chorus->rate, context.config.synthesis_rate),
                            chorus_amplitude(chorus->depth)}
            : ChorusProgram{};
    }
    // Only creation writes immutable books. There are no caller spans or CPU
    // pointers in the published program, checkpoint, or command stream.
    for (std::size_t index = 0; index < program.section_count; ++index) {
        if (!program.sections[index].pole) continue;
        const auto& coefficients = program.sections[index].pole->coefficients;
        for (std::size_t word = 0; word < coefficients.size(); ++word)
            context.put16(effects_book_address(index) + static_cast<std::uint32_t>(word) * 2u,
                          std::bit_cast<std::uint16_t>(coefficients[word]));
    }
    return Error::none;
}
void reset_effects(Context& context) noexcept {
    // Quiescent reset only. Leave immutable books and program/catalog untouched.
    std::fill_n(context.memory.get() + effects_ring_address / 4u, effects_ring_bytes / 4u, 0u);
    for (std::size_t index = 0; index < effects::max_sections; ++index)
        std::fill_n(context.memory.get() + effects_history_address(index) / 4u,
                    effects_mutable_section_bytes / 4u, 0u);
    std::fill_n(context.memory.get() + effects_staging_address / 4u, effects_staging_bytes / 4u, 0u);
    for (auto& state : context.cpu) state.effects = EffectsState{};
}
WetOutput render_effects(Context& context, EffectsState& state, std::uint32_t task_offset,
                         std::uint32_t frames, std::uint32_t task_frames) noexcept {
    if (context.write_error != Error::none || frames == 0) return {};
    if (context.effects.routing == effects::Routing::direct) return {task_offset, frames};
    const auto outer_start = task_offset - task_offset % outer_capacity;
    const auto fragment_offset = task_offset - outer_start;
    if (task_frames > AudioService::max_frames || task_offset >= task_frames ||
        (task_offset & 15u) != 0 || (frames & 15u) != 0 || (task_frames & 15u) != 0 ||
        state.cursor > context.effects.ring_bytes) {
        context.write_error = Error::invalid_state;
        return {};
    }
    const auto outer_frames = std::min(outer_capacity, task_frames - outer_start);
    if (frames > outer_frames - fragment_offset || state.staged_frames != fragment_offset ||
        (state.staged_frames != 0 && (state.outer_start != outer_start || state.outer_frames != outer_frames))) {
        context.write_error = Error::invalid_state;
        return {};
    }
    WetRenderer renderer(context);
    if (fragment_offset != 0 || frames != outer_frames) {
        if (state.staged_frames == 0) {
            state.outer_start = outer_start;
            state.outer_frames = static_cast<std::uint16_t>(outer_frames);
        }
        for (std::size_t plane = 0; plane < buses.size(); ++plane)
            renderer.save(buses[plane], effects_staging_address + static_cast<std::uint32_t>(plane) *
                          effects_plane_bytes + fragment_offset * 2u, static_cast<std::uint16_t>(frames * 2u));
        state.staged_frames = static_cast<std::uint16_t>(fragment_offset + frames);
        if (state.staged_frames != outer_frames) return {};
        for (std::size_t plane = 0; plane < buses.size(); ++plane)
            renderer.load(effects_staging_address + static_cast<std::uint32_t>(plane) * effects_plane_bytes,
                          buses[plane], static_cast<std::uint16_t>(outer_frames * 2u));
        state.staged_frames = state.outer_frames = 0;
        state.outer_start = 0;
    }
    renderer.graph(state, outer_frames);
    return context.write_error == Error::none ? WetOutput{outer_start, outer_frames} : WetOutput{};
}
} // namespace isle::audio::detail
