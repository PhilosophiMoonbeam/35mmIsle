#ifndef ISLE_FOCUS_DETECTOR_HPP
#define ISLE_FOCUS_DETECTOR_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace isle::focus {
using Object = std::uintptr_t;
inline constexpr std::size_t history_capacity = 20, photo_capacity = 12;
inline constexpr std::size_t tile_pixels = 64, tile_bytes = tile_pixels * 2;
struct Subject {
    Object object = 0;
    std::int32_t pokemon_id = 0;
    std::uint16_t flags = 0;
};
struct Photo {
    std::array<Object,photo_capacity> objects{};
    Object focus = 0;
};
struct Focus : Subject {
    bool present = false;
    std::uint16_t context = 0;
};
enum class Error { none, invalid_context, extent };

// CPU side of GAME's live color-history detector, not the photo depth scorer.
// The scene stages metadata at draw time and analyzes only after real GPU
// completion of the matching context's 8x8 RGBA16 captures. Source row0 is the
// baseline; rows1..19 follow actual per-object lane0 render order. The caller
// owns context reuse, GPU fences, capture commands and object-identity lifetime.
// A scene-local identity is compared only; no retired actor pointer is read.
class Detector final {
public:
    // Snapshot active metadata/photo fields, not color pixels. Rejection changes
    // nothing; unused source history tails remain inaccessible to analysis.
    Error stage(std::uint16_t context, std::span<const Subject>, const Photo&) noexcept;
    // Dense 128-byte regions in canonical word-swapped RDRAM; offset is physical
    // and 64-byte aligned, matching source capture allocations. No pixel copy,
    // allocation, FP arithmetic, GPU submission or manufactured completion.
    Error analyze(std::uint16_t context, std::span<const std::uint32_t> words,
                  std::uint32_t regions_offset) noexcept;
    const Focus& computed() const noexcept { return computed_; }
    const Focus& published() const noexcept { return published_; }
    const Photo* photo(std::uint16_t context) const noexcept;
    void publish() noexcept; // Source CopyInfo; analysis does not publish early.
    void cleanup(Object) noexcept; // All two-context history rows, then computed flag.
    // Source postprocess draws 21 red pixels in the 320x240 framebuffer only for
    // direction -1 and focused flag4. Canonical words are modified in place;
    // unrelated halfwords/hidden RDP coverage memory are untouched. If inactive,
    // the framebuffer is not accessed or validated, as in the source branch.
    Error mark_center(std::span<std::uint32_t> words, std::uint32_t framebuffer,
                      std::int32_t direction) const noexcept;
private:
    struct Context {
        std::array<Subject,history_capacity> subjects{};
        std::size_t count = 0;
        Photo photo{};
    };
    std::array<Context,2> contexts_{};
    Focus computed_{}, published_{};
};
} // namespace isle::focus
#endif
