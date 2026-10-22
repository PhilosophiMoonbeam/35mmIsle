#include "35mmisle/game_math.h"

#include <array>
#include <bit>
#include <cfenv>
#include <cstdint>
#include <iostream>

namespace {
int failures = 0;

void check(bool condition, const char* description) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << description << '\n';
    }
}

void expect_bits(float actual, std::uint32_t expected, const char* description, const char* mode) {
    const auto bits = std::bit_cast<std::uint32_t>(actual);
    if (bits != expected) {
        ++failures;
        std::cerr << "FAIL: " << description << " (" << mode << ") got 0x" << std::hex << bits
                  << ", expected 0x" << expected << std::dec << '\n';
    }
}

struct Rounding {
    int value;
    const char* name;
};
constexpr std::array<Rounding, 4> rounding_modes{{
    {FE_TONEAREST, "nearest"},
    {FE_TOWARDZERO, "toward zero"},
    {FE_UPWARD, "upward"},
    {FE_DOWNWARD, "downward"},
}};

void axes_and_signed_zero(const Rounding& mode) {
    constexpr std::array<float, 2> zeros{0.0f, -0.0f};
    for (const float y : zeros) {
        expect_bits(isle_game_atanf(y), 0x00000000u, "atan normalizes signed zero", mode.name);
        expect_bits(isle_game_atan2f(y, 1.0f), 0x00000000u,
                    "positive x axis normalizes signed zero", mode.name);
        expect_bits(isle_game_atan2f(y, -1.0f), 0x40490fdbu,
                    "negative x axis is positive pi even for negative-zero y", mode.name);
        for (const float x : zeros) {
            expect_bits(isle_game_atan2f(y, x), 0x00000000u,
                        "every signed-zero origin returns positive zero", mode.name);
        }
    }
    for (const float x : zeros) {
        expect_bits(isle_game_atan2f(1.0f, x), 0x3fc90fdbu,
                    "positive y axis ignores x zero sign", mode.name);
        expect_bits(isle_game_atan2f(-1.0f, x), 0xbfc90fdbu,
                    "negative y axis ignores x zero sign", mode.name);
    }
}

void continued_fraction_and_quadrants(std::size_t mode_index) {
    // Fixed results derived with exact rational operations rounded to binary32
    // after each GAME operation, not host-libm results or approximate tolerances.
    // Reciprocal corrections exercise the explicitly qualified native PI_2
    // policy; these fixtures do not establish the original SDK macro's type.
    struct Case {
        std::uint32_t input;
        std::array<std::uint32_t, 4> output;
    };
    constexpr std::array<Case, 8> cases{{
        {0x3f000000u, {0x3eed6338u, 0x3eed6338u, 0x3eed6339u, 0x3eed6338u}},
        {0xbf000000u, {0xbeed6338u, 0xbeed6338u, 0xbeed6339u, 0xbeed6338u}},
        {0x3f7fffffu, {0x3f490fdau, 0x3f490fdau, 0x3f490fdbu, 0x3f490fdau}},
        {0x3f800000u, {0x3f490fdbu, 0x3f490fdau, 0x3f490fdbu, 0x3f490fdbu}},
        {0xbf800000u, {0xbf490fdbu, 0xbf490fdau, 0xbf490fdbu, 0xbf490fdbu}},
        {0x3f800001u, {0x3f490fddu, 0x3f490fddu, 0x3f490fdbu, 0x3f490fddu}},
        {0x40000000u, {0x3f8db70du, 0x3f8db70du, 0x3f8db70du, 0x3f8db70du}},
        {0xc0000000u, {0xbf8db70du, 0xbf8db70du, 0xbf8db70cu, 0xbf8db70du}},
    }};
    const auto& mode = rounding_modes[mode_index];
    for (const auto& fixture : cases) {
        const float input = std::bit_cast<float>(fixture.input);
        expect_bits(isle_game_atanf(input), fixture.output[mode_index],
                    "continued fraction and reciprocal boundary", mode.name);
        expect_bits(isle_game_atan2f(input, 1.0f), fixture.output[mode_index],
                    "positive-x slope follows GAME atan", mode.name);
    }

    constexpr std::array<std::uint32_t, 4> negative_x_angle{
        0x4016cbe4u, 0x4016cbe4u, 0x4016cbe5u, 0x4016cbe4u,
    };
    expect_bits(isle_game_atan2f(1.0f, -1.0f), negative_x_angle[mode_index],
                "second quadrant subtracts positive atan from binary32 pi", mode.name);
    expect_bits(isle_game_atan2f(-1.0f, -1.0f), negative_x_angle[mode_index] | 0x80000000u,
                "third quadrant applies sign after pi subtraction", mode.name);
}

unsigned pose_bin(float camera_angle) {
    // A scalar consumer with yaw zero and the GAME score's binary32 operation
    // grouping. No species tables, fitted thresholds or libm oracle are used.
    volatile float delta = 0.0f - camera_angle;
    const float magnitude = delta < 0.0f ? -delta : delta;
    volatile float degrees_numerator = magnitude * 180.0f;
    volatile float degrees = degrees_numerator / 0x1.921fb6p+1f;
    volatile float bin = degrees / 15.0f;
    return static_cast<unsigned>(bin);
}

void actual_pose_boundaries() {
    // Actual Original scalar-return pairs, frozen before math-body inspection
    // and independently admitted for bd_35mmIsle-v4u.7.4.3. Both host-libm
    // substitutions returned one ULP less and crossed a real pose-score bin.
    // These are FE_TONEAREST observations, not evidence for other modes or for
    // subnormal/Original flush-to-zero equivalence.
    struct Case {
        std::uint32_t y;
        std::uint32_t x;
        std::uint32_t angle;
        unsigned bin;
        const char* description;
    };
    constexpr std::array<Case, 2> cases{{
        {0x3e8930a2u, 0x3f7fffffu, 0x3e860a92u, 1, "Original scalar case 10168"},
        {0x3f13cd39u, 0x3f7fffffu, 0x3f060a92u, 2, "Original scalar case 10204"},
    }};
    for (const auto& fixture : cases) {
        const float angle = isle_game_atan2f(std::bit_cast<float>(fixture.y),
                                            std::bit_cast<float>(fixture.x));
        expect_bits(angle, fixture.angle, fixture.description, "nearest");
        check(pose_bin(angle) == fixture.bin, fixture.description);
        check(pose_bin(std::bit_cast<float>(fixture.angle - 1u)) == fixture.bin - 1u,
              "one-ULP predecessor crosses the observed pose boundary");
    }
}
}

int main() {
    const int saved_rounding = std::fegetround();
    check(saved_rounding != -1, "read caller rounding mode");
    for (std::size_t index = 0; index < rounding_modes.size(); ++index) {
        const auto& mode = rounding_modes[index];
        if (std::fesetround(mode.value) != 0) {
            check(false, "select supported IEEE rounding mode");
            continue;
        }
        axes_and_signed_zero(mode);
        continued_fraction_and_quadrants(index);
        if (mode.value == FE_TONEAREST) actual_pose_boundaries();
        check(std::fegetround() == mode.value, "scalar functions preserve caller rounding mode");
    }
    if (saved_rounding != -1) {
        check(std::fesetround(saved_rounding) == 0, "restore caller rounding mode");
    }
    if (failures) {
        std::cerr << failures << " GAME scalar math checks failed\n";
        return 1;
    }
    std::cout << "GAME scalar signed-zero, continued-fraction, quadrant and pose-boundary checks passed\n";
    return 0;
}
