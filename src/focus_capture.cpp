#include "focus_capture.hpp"
#include <algorithm>
#include <stdexcept>

namespace isle::focus {
namespace {
constexpr std::size_t ram_bytes = 8 * 1024 * 1024;
// Independently authored public F3DEX2/RDP wire encodings of own GAME
// pokemon_detect.c:11-34,193-199. No SDK producer implementation is used.
constexpr std::array<std::uint32_t, RegionCopies::tile_words> tile{
    0xe7000000,0, 0xfd10013f,0x0f000000,
    0xf5100400,0x07000000, 0xe6000000,0,
    0xf42701d0,0x0728c1ec, 0xe7000000,0,
    0xf5100400,0, 0xf2000000,0x0001c01c, 0xdf000000,0
};
constexpr std::array<std::uint32_t, RegionCopies::copy_words> copy{
    0xe3000a01,0x00200000, // COPY cycle
    0xe200001c,0,          // NOOP render mode
    0xe3000c00,0,          // texture perspective off
    0xe2001e01,0,          // alpha compare off
    0xed000000,0x00080080, // 32 x 32 pixel scissor
    0xe401c01c,0, 0xe1000000,0, 0xf1000000,0x10000400,
    0xe7000000,0,
    0xe3000a01,0x00100000, // source two-cycle restore
    0xe200001c,0xc8112078, // FOG_SHADE_A | AA_ZB_OPA_SURF2
    0xe3000c00,0x00080000, // texture perspective on
    0xff10013f,0x0f000000, 0xdf000000,0
};
bool overlaps(RamRange a, RamRange b) noexcept {
    return a.offset < b.offset+b.size && b.offset < a.offset+a.size;
}
}
RegionCopies::RegionCopies(CaptureLayout layout)
    : ranges_{layout.tile_list,layout.copy_list,layout.regions} {
    constexpr std::array<std::size_t,3> sizes{tile_words*4,copy_words*4,row_count*row_bytes};
    constexpr std::array<std::size_t,3> alignments{8,8,64};
    for (std::size_t i=0;i<ranges_.size();++i) {
        const auto r=ranges_[i];
        if (r.offset >= ram_bytes || r.size > ram_bytes-r.offset ||
            r.size < sizes[i] || r.offset%alignments[i] != 0)
            throw std::invalid_argument("invalid focus capture RAM range");
    }
    for (std::size_t i=0;i<ranges_.size();++i)
        for (std::size_t j=i+1;j<ranges_.size();++j)
            if (overlaps(ranges_[i],ranges_[j]))
                throw std::invalid_argument("overlapping focus capture RAM ranges");
}
std::array<RamRange,2> RegionCopies::installation_ranges() const noexcept {
    return {{{ranges_[0].offset,tile_words*4},{ranges_[1].offset,copy_words*4}}};
}
std::uint32_t RegionCopies::region_offset(unsigned row) const {
    if (row >= row_count) throw std::out_of_range("focus capture row");
    return static_cast<std::uint32_t>(ranges_[2].offset+row*row_bytes);
}
CaptureStatus RegionCopies::install(std::span<std::uint32_t> words) const noexcept {
    // Validate the entire declared ownership extent before either list write.
    for (const auto r:ranges_)
        if ((r.offset+r.size+3)/4 > words.size()) return CaptureStatus::insufficient_storage;
    std::copy(tile.begin(),tile.end(),words.begin()+static_cast<std::ptrdiff_t>(ranges_[0].offset/4));
    std::copy(copy.begin(),copy.end(),words.begin()+static_cast<std::ptrdiff_t>(ranges_[1].offset/4));
    return CaptureStatus::ok;
}
CaptureResult RegionCopies::emit_region(std::span<std::uint32_t> words,
                                       unsigned row,Scissor s) const noexcept {
    std::size_t cursor=0;
    return append_region(words.size(),row,s,[&](std::uint32_t a,std::uint32_t b) {
        words[cursor++]=a; words[cursor++]=b;
    });
}
} // namespace isle::focus
