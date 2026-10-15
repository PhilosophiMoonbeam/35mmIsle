#ifndef ISLE_AUDIO_EFFECTS_HPP
#define ISLE_AUDIO_EFFECTS_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace isle::audio::effects {
// Direct is the admitted raw0 stereo AUX return, not a failed-network fallback.
enum class Routing { direct, network };
inline constexpr std::size_t max_sections = 16;
inline constexpr std::uint32_t max_ring_samples = 19200;

// Explicit numeric DSP input: no SDK object or coefficient generator is implied.
// The native 32-byte book/eight-byte history reservation is measured for the
// admitted books/gains and16..160-sample static pulls. Supplying other explicit
// numeric books is not a claim of SDK coefficient-generation or filter parity.
struct PoleFilter {
    std::uint16_t gain = 0;
    std::array<std::int16_t, 16> coefficients{};
};
// Creation-owned modulation, in the measured integer rate/depth units. A missing
// chorus or rate zero retains the static section graph. Depth zero with positive
// rate still runs the persistent resampler; it is not a static bypass.
// The ordinary-range recurrence is qualified, not a universal extreme-depth
// arithmetic claim (tracked separately by bd_35mmIsle-v4u.9.17.9).
// Creation rejects a possible u16 pitch overflow or rounded aligned LOAD past
// scratch280..43f into dry440 (worst original160, carry32767, alignment6).
// Some extreme-depth structural probes are consequently not safe musical
// configurations. Rate must advance at most one phase period per160 samples
// at the supplied actual synthesis rate. Neither parameter is silently clamped.
struct Chorus {
    std::uint32_t rate = 0, depth = 0;
};
struct Section {
    // Backward offsets from the common ring cursor, in samples, on a four-sample
    // grid. Both complete windows are loaded before the ordered section writes.
    std::uint32_t input_samples = 0, output_samples = 0;
    std::int16_t feedback = 0, feedforward = 0, gain = 0;
    std::optional<PoleFilter> pole = std::nullopt;
    std::optional<Chorus> chorus = std::nullopt;
};
struct Config {
    Routing routing = Routing::direct;
    // Network: 160..19200 samples, four-sample aligned, with room for a complete
    // 160-sample window after every port offset. At least one section is required.
    std::uint32_t ring_samples = 0;
    // Borrowed only for AudioService::create; successful creation owns its copy.
    std::span<const Section> sections{};
};
} // namespace isle::audio::effects
#endif
