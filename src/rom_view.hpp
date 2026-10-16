#ifndef ISLE_ROM_VIEW_HPP
#define ISLE_ROM_VIEW_HPP

#include <cstddef>
#include <cstdint>
#include <span>

namespace isle::rom {
// Borrowed immutable canonical big-endian bytes. guest_address is a guest
// address, never a host pointer; rom_offset records its admitted provenance.
// The storage owner must outlive this view. Scalar reads check their relative
// alignment and extent; creating a view does not admit a mapping or ROM.
struct GuestView {
    std::uint32_t guest_address = 0;
    std::size_t rom_offset = 0;
    std::span<const std::byte> bytes{};

    std::uint8_t u8(std::size_t offset) const;
    std::uint16_t u16(std::size_t offset) const;
    std::uint32_t u32(std::size_t offset) const;
    float f32(std::size_t offset) const;
};
} // namespace isle::rom
#endif
