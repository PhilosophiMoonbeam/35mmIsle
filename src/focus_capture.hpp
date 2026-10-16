#ifndef ISLE_FOCUS_CAPTURE_HPP
#define ISLE_FOCUS_CAPTURE_HPP

#include "menu_resources.hpp"
#include <array>
#include <cstdint>
#include <span>

namespace isle::focus {
using RamRange = menu::RamRange;
struct CaptureLayout { RamRange tile_list, copy_list, regions; };
struct Scissor { unsigned left, top, right, bottom; };
enum class CaptureStatus { ok, invalid_row, invalid_scissor, insufficient_storage };
struct CaptureResult {
    CaptureStatus status;
    std::size_t written_words;
    explicit operator bool() const noexcept { return status == CaptureStatus::ok; }
};
// Standalone SaveRegion graphics primitive, not a detector/photo/GObj service.
// Caller owns the complete disjoint RAM/asset/framebuffer union and segment F.
// ONE shared twenty-row allocation serves both source CPU history contexts.
// GPU completion/readback AND CPU analysis must retire before overwriting a row;
// this object provides neither a fence nor permission to overlap either owner.
class RegionCopies final {
public:
    static constexpr unsigned row_count = 20;
    static constexpr std::size_t row_bytes = 128, tile_words = 18, copy_words = 28;
    static constexpr std::size_t command_words = 12;
    // Throws invalid_argument for malformed physical 8 MiB RDRAM reservations.
    explicit RegionCopies(CaptureLayout layout);
    const std::array<RamRange, 3>& reservations() const noexcept { return ranges_; }
    std::array<RamRange, 2> installation_ranges() const noexcept;
    // Invalid indices throw out_of_range; no pointer/epoch residency cache.
    std::uint32_t region_offset(unsigned row) const;
    // Canonical numeric words indexed by physical byte address / 4. Explicitly
    // reinstall after RAM cold/reset. Only static-list bytes are written.
    CaptureStatus install(std::span<std::uint32_t> canonical_words) const noexcept;
    // Append only: no EndDL/FullSync. Preflight failure leaves all words intact.
    // Scissor is the already-computed source mainCameraSetScissor pixel bounds.
    CaptureResult emit_region(std::span<std::uint32_t> command_storage,
                              unsigned row, Scissor scissor) const noexcept;
    // Direct command-pair sink for existing typed renderer command buffers.
    // Complete row/scissor/capacity preflight precedes the first sink invocation.
    template<class Emit>
    CaptureResult append_region(std::size_t capacity_words,unsigned row,Scissor s,Emit&& emit) const {
        if (row>=row_count) return {CaptureStatus::invalid_row,0};
        if (s.left>s.right || s.top>s.bottom || s.right>1023 || s.bottom>1023)
            return {CaptureStatus::invalid_scissor,0};
        if (capacity_words<command_words) return {CaptureStatus::insufficient_storage,0};
        emit(0xe7000000u,0u); emit(0xe3001001u,0u);
        emit(0xde000000u,static_cast<std::uint32_t>(ranges_[0].offset));
        emit(0xff100007u,static_cast<std::uint32_t>(ranges_[2].offset+row*row_bytes));
        emit(0xde000000u,static_cast<std::uint32_t>(ranges_[1].offset));
        emit(0xed000000u|(s.left<<14)|(s.top<<2),(s.right<<14)|(s.bottom<<2));
        return {CaptureStatus::ok,command_words};
    }
private:
    std::array<RamRange, 3> ranges_;
};
} // namespace isle::focus
#endif
