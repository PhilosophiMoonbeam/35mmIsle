#ifndef ISLE_VPK0_HPP
#define ISLE_VPK0_HPP

#include <cstddef>
#include <span>

namespace isle::vpk0 {

enum class Error {
    none,
    bad_magic,
    truncated,
    output_too_small,
    invalid_tree,
    invalid_bit_width,
    invalid_backreference
};

struct Result {
    Error error;
    std::size_t bytes_consumed;
    std::size_t bytes_written;

    explicit operator bool() const noexcept { return error == Error::none; }
};

// Decode one lowercase vpk0 stream. Input and output must not overlap.
// The caller owns the output and must publish it only after success. Header and
// capacity failures write nothing; body failures may leave bytes_written bytes
// of partial output. No allocation is performed. Trailing input is permitted.
// bytes_consumed includes the header and rounds the actual bit position up to
// a byte (not any stream padding). A truncated read consumes the available bits;
// incomplete headers consume the available bytes, bad_magic consumes four, and
// output_too_small consumes the nine-byte header. Tree limits are 64 nodes total
// and 20 pending roots per tree. Nonempty copies exceeding the declared output,
// or using zero/forward/out-of-range distances, are invalid_backreference.
Result decode(std::span<const std::byte> input, std::span<std::byte> output) noexcept;

} // namespace isle::vpk0

#endif
