#include "rom_view.hpp"

#include <bit>
#include <limits>
#include <stdexcept>

namespace isle::rom {
namespace {
static_assert(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559);
void bounds(const GuestView& view, std::size_t offset, std::size_t size, std::size_t alignment) {
    if (offset % alignment != 0 || offset > view.bytes.size() || size > view.bytes.size() - offset)
        throw std::out_of_range("Guest resource read outside aligned big-endian wire extent");
}
} // namespace
std::uint8_t GuestView::u8(std::size_t offset) const {
    bounds(*this, offset, 1, 1);
    return std::to_integer<std::uint8_t>(bytes[offset]);
}
std::uint16_t GuestView::u16(std::size_t offset) const {
    bounds(*this, offset, 2, 2);
    return static_cast<std::uint16_t>((std::to_integer<std::uint16_t>(bytes[offset]) << 8) |
                                    std::to_integer<std::uint16_t>(bytes[offset + 1]));
}
std::uint32_t GuestView::u32(std::size_t offset) const {
    bounds(*this, offset, 4, 4);
    return (std::to_integer<std::uint32_t>(bytes[offset]) << 24) |
           (std::to_integer<std::uint32_t>(bytes[offset + 1]) << 16) |
           (std::to_integer<std::uint32_t>(bytes[offset + 2]) << 8) |
           std::to_integer<std::uint32_t>(bytes[offset + 3]);
}
float GuestView::f32(std::size_t offset) const { return std::bit_cast<float>(u32(offset)); }
} // namespace isle::rom
