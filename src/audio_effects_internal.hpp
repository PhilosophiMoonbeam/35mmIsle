#ifndef ISLE_AUDIO_EFFECTS_INTERNAL_HPP
#define ISLE_AUDIO_EFFECTS_INTERNAL_HPP

#include "35mmisle/audio_effects.hpp"

namespace isle::audio {
enum class Error;
namespace detail {
struct Context;
inline constexpr std::uint32_t effects_ring_address = 0x133600;
inline constexpr std::uint32_t effects_ring_bytes = effects::max_ring_samples * 2u;
inline constexpr std::uint32_t effects_section_address = 0x13cc00;
inline constexpr std::uint32_t effects_section_stride = 80;
inline constexpr std::uint32_t effects_book_bytes = 32;
inline constexpr std::uint32_t effects_history_bytes = 8;
inline constexpr std::uint32_t effects_resample_bytes = 32;
inline constexpr std::uint32_t effects_mutable_section_bytes = effects_history_bytes + effects_resample_bytes;
inline constexpr std::uint32_t effects_staging_address = 0x13d100;
inline constexpr std::uint32_t effects_plane_bytes = 320;
inline constexpr std::uint32_t effects_staging_bytes = 4u * effects_plane_bytes;
constexpr std::uint32_t effects_book_address(std::size_t section) noexcept {
    return effects_section_address + static_cast<std::uint32_t>(section) * effects_section_stride;
}
constexpr std::uint32_t effects_history_address(std::size_t section) noexcept {
    return effects_book_address(section) + effects_book_bytes;
}
constexpr std::uint32_t effects_resample_address(std::size_t section) noexcept {
    return effects_history_address(section) + effects_history_bytes;
}
struct ChorusProgram {
    float increment = 0.0f, amplitude = 0.0f;
};
// Creation-owned and immutable after publication; never copied per task.
struct EffectsProgram {
    effects::Routing routing = effects::Routing::direct;
    std::uint32_t ring_bytes = 0;
    std::size_t section_count = 0;
    std::array<effects::Section, effects::max_sections> sections{};
    std::array<ChorusProgram, effects::max_sections> chorus{};
};
// Value-only state shared by all clients and included in both common CPU banks.
struct ChorusState {
    float phase = 0.25f;
    // Drift is a byte offset modulo the common ring, never an unbounded sum.
    std::uint32_t drift = 0;
    std::uint16_t carry = 0;
    bool initialized = false;
};
struct EffectsState {
    // Preserve the end-cursor representation; the next transfer emits its
    // observed zero-length end segment before wrapping back to the ring base.
    std::uint32_t cursor = 0;
    std::uint16_t initialized_poles = 0;
    std::uint16_t staged_frames = 0, outer_frames = 0;
    std::uint32_t outer_start = 0;
    std::array<ChorusState, effects::max_sections> chorus{};
};
struct WetOutput { std::uint32_t task_offset = 0, frames = 0; };
Error validate_effects(effects::Config) noexcept;
Error validate_effects_for_rate(effects::Config, std::uint32_t synthesis_rate) noexcept;
Error build_effects(Context&, effects::Config) noexcept;
void reset_effects(Context&) noexcept;
WetOutput render_effects(Context&, EffectsState&, std::uint32_t task_offset,
                         std::uint32_t frames, std::uint32_t task_frames) noexcept;
} // namespace detail
} // namespace isle::audio
#endif
