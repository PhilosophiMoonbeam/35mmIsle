#include "audio_synth_internal.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>

namespace isle::audio::detail {
float integer_power(float multiplier, std::uint32_t exponent) noexcept {
    float power = 1.0f;
    while (exponent != 0) {
        if ((exponent & 1u) != 0) power *= multiplier;
        exponent >>= 1;
        if (exponent != 0) multiplier *= multiplier;
    }
    return power;
}
float cents_ratio(std::int32_t cents) noexcept {
    // Correctly rounded binary32 unit-cent ratios; preserve a rounding step
    // after every multiply, rather than evaluating a full-exponent pow.
    constexpr float up = 0x1.0025dep0f;
    constexpr float down = 0x1.ffb45p-1f;
    const auto bits = static_cast<std::uint32_t>(cents);
    const auto magnitude = cents < 0 ? 0u - bits : bits;
    return integer_power(cents < 0 ? down : up, magnitude);
}
bool pitch_word(float pitch, std::uint16_t& result) noexcept {
    if (!std::isfinite(pitch) || pitch < 1.0f / 32768.0f) return false;
    // Preserve the caller's raw ratio; only the single-stage DSP word saturates.
    // Compare before narrowing so even large finite ratios cannot overflow it.
    result = pitch >= 65534.0f / 32768.0f ? std::uint16_t{65534} :
        static_cast<std::uint16_t>(static_cast<double>(pitch) * 32768.0);
    return true;
}
std::uint32_t rounded_frames(std::int32_t us, std::uint32_t rate) noexcept {
    // Original conversion uses a single-precision product, double division
    // and rounding, then a single-precision result before integer truncation.
    const float product = static_cast<float>(us) * static_cast<float>(rate);
    const float rounded = static_cast<float>(static_cast<double>(product) / 1000000.0 + 0.5);
    return static_cast<std::uint32_t>(rounded);
}
std::uint32_t duration_frames(std::int32_t us, std::uint32_t rate) noexcept {
    // DSP parameter times, unlike client callback deadlines, discard low bits.
    return rounded_frames(us, rate) & ~15u;
}
bool scaled_duration_us(std::int32_t us, float pitch, std::int32_t& result) noexcept {
    const float scaled = static_cast<float>(us) / pitch;
    if (!std::isfinite(scaled) || static_cast<double>(scaled) > std::numeric_limits<std::int32_t>::max())
        return false;
    result = static_cast<std::int32_t>(scaled);
    return true;
}
std::uint32_t envelope_rate(std::uint16_t from, std::uint16_t to,
                            std::uint32_t samples, const Coefficients& coefficients) noexcept {
    const auto signed_from = std::bit_cast<std::int16_t>(from);
    const auto signed_to = std::bit_cast<std::int16_t>(to);
    if (samples == 0)
        return signed_to < signed_from ? 0u : 0x7fffffffu;
    const auto current = std::max<std::int32_t>(signed_from, 1);
    const auto target = std::max<std::int32_t>(signed_to, 1);
    int exponent = 0;
    const double mantissa = std::frexp(static_cast<double>(target) / current, &exponent);
    const auto bucket = static_cast<std::size_t>(mantissa * 16.0) - 8u;
    const double factor = std::exp2((exponent + coefficients.logarithm[bucket]) * 8.0 / samples);
    if (factor >= 32768.0) return 0x7fffffffu;
    const auto whole = static_cast<std::uint32_t>(factor);
    const auto fraction = static_cast<std::uint32_t>((factor - whole) * 65535.0);
    return (whole << 16) | fraction;
}
std::uint16_t cpu_gain(std::uint16_t from, std::uint16_t to, std::uint32_t rate,
                       std::uint32_t elapsed, std::uint32_t duration) noexcept {
    if (elapsed >= duration) return to;
    const float power = integer_power(static_cast<float>(rate) / 65536.0f, elapsed / 8u);
    // This is the original CPU estimate, not a scalar reconstructed from the
    // DSP's eight fractional lanes. The nominal endpoint snaps only CPU state.
    const float value = power * static_cast<float>(std::bit_cast<std::int16_t>(from));
    // Generated positive-duration rates are paired with 16-aligned durations:
    // amplitudes 1..32767 and admitted log coefficients [-1,0] bound this
    // signed result below 1.18e9, including binary32 rounding. Integer
    // narrowing, unlike floating-to-uint16 conversion, preserves the low bits.
    return static_cast<std::uint16_t>(static_cast<std::int32_t>(value));
}
} // namespace isle::audio::detail
