#ifndef ISLE_PHOTO_FEEDBACK_HPP
#define ISLE_PHOTO_FEEDBACK_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace isle::photo_feedback {

inline constexpr std::size_t width = 64;
inline constexpr std::size_t height = 48;
inline constexpr std::size_t subject_count = 12;
inline constexpr std::size_t plane_bytes = 0x1800;
inline constexpr std::size_t buffer_bytes = 0x27000;
inline constexpr std::uint16_t clear_depth = 0xfffc;

struct WideCount {
    std::uint32_t total = 0;
    std::uint32_t center = 0; // Inclusive x=17..46, y=13..34.
    bool operator==(const WideCount&) const = default;
};
using WideCounts = std::array<WideCount, subject_count>;

struct FinalCount {
    std::uint32_t total = 0;
    std::uint32_t center = 0; // Inclusive x=30..33, y=22..25.
    std::uint32_t visible = 0;
    std::uint32_t visible_center = 0; // Inclusive x=31..32, y=23..24.
    std::uint32_t approximate_total = 0;
    bool operator==(const FinalCount&) const = default;
};
using FinalCounts = std::array<FinalCount, subject_count>;

class PlanesView {
public:
    PlanesView() noexcept = default;

    // Borrow completed word-swapped RDRAM, whose span starts at physical address
    // zero. physical_base is the word-aligned physical address of 26 contiguous
    // 64x48 halfword planes: F/color=0, C/scene Z=1, A[i]=2+i, B[i]=14+i.
    // Every canonical byte address is read at address XOR 3, not as native u16.
    // Requires a whole-word RDRAM extent containing the complete plane block.
    // Rejects misalignment, address overflow and short extents without reading
    // any bytes or changing output. No host-pointer alignment is required.
    //
    // Caller must have completed/retired all RSP/RDP writes and hold exclusive
    // completed-read access for the entire reduction. The storage must remain
    // alive, unmoved and unmodified throughout each call; this view neither
    // owns storage nor waits on a fence. Release the borrow before GPU reuse.
    // Save WideCounts by value before the narrow pass reuses the plane block,
    // then borrow the completed narrow block again. Do not retain a view as a
    // substitute for an executor fence or resource lifetime.
    static bool borrow(std::span<const std::uint8_t> rdram,
                       std::uint32_t physical_base, PlanesView& output) noexcept;

private:
    std::span<const std::uint8_t> rdram_;
    std::size_t base_ = 0;
    std::uint16_t halfword(std::size_t plane, std::size_t pixel) const noexcept;
    friend bool count_wide(const PlanesView&, WideCounts&) noexcept;
    friend bool count_narrow(const PlanesView&, const WideCounts&, FinalCounts&) noexcept;
};

// Exact CPU reductions from GAME score.c:266-318 at pin
// df8815e75f3d79618466e01e0827c6165b93d11c (plane layout:243-255).
// All 12 entries, including empty subjects, are replaced on success. No heap
// allocation, plane copies, hidden-bit reads, depth tolerance or Z/dZ masking.
// Output must not overlap the borrowed RDRAM. Invalid views leave output intact.
bool count_wide(const PlanesView& planes, WideCounts& output) noexcept;

// visible means A != FFFC && A == C && B != C, using all 16 packed Z/dZ bits.
// approximate_total = wide.total * total / wide.center (integer truncation),
// or wide.total * 4 when wide.center is zero. Wide counts must satisfy the
// 3072-pixel total / 660-pixel center bounds and center <= total; invalid counts
// reject before reading RDRAM or changing output. WideCounts is independent of
// the reused GPU storage. This component is not a renderer or an Oak scorer.
bool count_narrow(const PlanesView& planes, const WideCounts& wide,
                  FinalCounts& output) noexcept;

} // namespace isle::photo_feedback

#endif
