#include "audio_dsp.hpp"

#include <algorithm>
#include <bit>

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
static_assert(effects_book_bytes + effects_history_bytes <= effects_section_stride);
static_assert(effects_staging_address + effects_staging_bytes <= AudioService::resource_address);
static_assert((effects_ring_address | effects_section_address | effects_staging_address |
               effects_section_stride | effects_book_bytes | effects_history_bytes) % 8u == 0);

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
            ring_load(input, section_input, bytes);
            ring_load(output, section_output, bytes);
            if (section.feedforward != 0) {
                mix(section.feedforward, section_input, section_output);
                // This pre-feedback commit is observable when ring windows
                // alias, even though a non-pole output is saved again below.
                if (!section.pole) ring_save(section_output, output, bytes);
            }
            if (section.feedback != 0) {
                mix(section.feedback, section_output, section_input);
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
            ring_save(section_output, output, bytes);
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
    }
    return Error::none;
}
Error build_effects(Context& context, effects::Config config) noexcept {
    const auto error = validate_effects(config);
    if (error != Error::none) return error;
    if (!context.memory) return Error::invalid_state;
    auto& program = context.effects;
    program.routing = config.routing;
    program.ring_bytes = config.ring_samples * 2u;
    program.section_count = config.sections.size();
    std::copy(config.sections.begin(), config.sections.end(), program.sections.begin());
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
        std::fill_n(context.memory.get() + effects_history_address(index) / 4u, effects_history_bytes / 4u, 0u);
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
