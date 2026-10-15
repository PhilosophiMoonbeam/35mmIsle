#ifndef ISLE_PHOTO_LIGHTING_HPP
#define ISLE_PHOTO_LIGHTING_HPP

#include "35mmisle/engine.h"
#include "rom_snapshot.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace isle::photo {
inline constexpr std::size_t lighting_installation_span_count = 3;
inline constexpr std::size_t lighting_reference_count = 2;
inline constexpr std::size_t lighting_rdram_size = 8 * 1024 * 1024;

// Canonical big-endian bytes borrowed from the admitted private Snapshot.
// Addresses are original KSEG0 addresses, not host pointers or ROM offsets.
struct LightingSpan {
    std::uint32_t guest_address = 0;
    std::size_t rom_offset = 0;
    std::span<const std::byte> bytes{};
};
struct LightingCommand { std::uint32_t w0, w1; };
struct LightReference {
    // Both commands DMA 16 bytes. The ambient ABI uses only its first 8;
    // its DMA's last 8 overlap the directional record's color/duplicate bytes.
    LightingSpan dma;
    std::uint16_t destination_offset = 0; // Public G_MOVEMEM light-buffer offset.
    std::uint8_t slot = 0;               // 1 = directional, 2 = ambient here.
    bool ambient = false;

    std::span<const std::byte, 3> color() const noexcept { return dma.bytes.first<3>(); }
    std::span<const std::byte, 3> duplicate_color() const noexcept { return dma.bytes.subspan<4, 3>(); }
};
using LightDirection = std::array<std::int8_t, 3>;

// Complete immutable closure of GAME func_800A1A50's pre-render list and the
// actual photo_check normal-preview setup: two terminated lists, two light
// references, no nested lists or other resources. Not the score-pass setup.
// Native descriptors are fixed-size; no extracted payload is compiled in.
// The Snapshot AND its unmoved storage must outlive this object and its views.
class LightingInputs final {
public:
    explicit LightingInputs(const rom::Snapshot& snapshot);
    LightingInputs(rom::Snapshot&&) = delete;
    LightingInputs(const LightingInputs&) = delete;
    LightingInputs& operator=(const LightingInputs&) = delete;
    LightingInputs(LightingInputs&&) = delete;
    LightingInputs& operator=(LightingInputs&&) = delete;

    std::uint32_t pre_render_guest() const noexcept { return pre_render_.guest_address; }
    const LightingSpan& pre_render() const noexcept { return pre_render_; }
    std::size_t command_count() const noexcept { return pre_render_.bytes.size() / 8; }
    LightingCommand command(std::size_t index) const;
    std::uint32_t normal_setup_guest() const noexcept { return normal_setup_.guest_address; }
    const LightingSpan& normal_setup() const noexcept { return normal_setup_; }
    LightingCommand normal_setup_command(std::size_t index) const;
    // Original list order: directional slot 1, then ambient slot 2.
    const std::array<LightReference, lighting_reference_count>& lights() const noexcept { return lights_; }
    // Exact disjoint union of the two overlapping DMA extents, plus both lists.
    const std::array<LightingSpan, lighting_installation_span_count>& installation_spans() const noexcept {
        return install_;
    }
    std::size_t installed_byte_count() const noexcept;
    std::uint32_t direction_guest() const noexcept { return lights_[0].dma.guest_address + 8; }

    // Requires exclusive caller-owned word-swapped 8 MiB RDRAM, disjoint from
    // Snapshot storage, only during prepare after all prior RSP/RDP retirement.
    // Writes only installation_spans(), to (guest - 0x80000000 + i) XOR 3.
    // Reinstall restores the original direction; update after install for the
    // current camera and execute the pre-render list before any model vertices.
    void install(std::span<std::byte> word_swapped_rdram) const;

    // GAME 47380.c:1125-1132 -> vector.c:5-15 -> 4D400.c:98-101:
    // normalize eye-at in XZ, set Y=1, normalize XYZ, truncate each f32*127.
    // Only three signed direction bytes in caller RAM change. Colors, duplicate
    // bytes, padding, list, Snapshot and unrelated RAM are never modified.
    // Eye/at Y is unused, just as in GAME. Requires round-to-nearest binary32;
    // nonfinite/zero/overflowed horizontal calculations are rejected, not
    // clamped or replaced by a guessed direction. All rejection precedes writes.
    // Same exclusivity/lifetime/extent requirements as install(); RAM must have
    // been installed by the caller. No renderer ownership or fence is acquired.
    LightDirection update_direction(std::span<std::byte> word_swapped_rdram,
                                    const IsleGameVec3& eye, const IsleGameVec3& at) const;

private:
    void check_destination(std::span<std::byte> word_swapped_rdram) const;
    const rom::Snapshot* snapshot_;
    LightingSpan pre_render_, normal_setup_;
    std::array<LightReference, lighting_reference_count> lights_{};
    std::array<LightingSpan, lighting_installation_span_count> install_{};
};
} // namespace isle::photo
#endif
