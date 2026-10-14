#ifndef ISLE_ROM_HPP
#define ISLE_ROM_HPP

#include <array>
#include <cstdint>
#include <iosfwd>
#include <string>

namespace isle::rom {

enum class ByteOrder { z64, v64, n64 };
enum class Error { none, io, truncated, unsupported_order, wrong_size, wrong_identity };

struct Identity {
    ByteOrder order;
    std::uint64_t size;
    std::array<std::uint8_t, 20> sha1;
};

struct Result {
    Identity identity;
    Error error;

    explicit operator bool() const noexcept { return error == Error::none; }
};

inline constexpr std::uint64_t us_size = 16777216;

// Consume from the current stream position without seeking or writing. The
// fingerprint covers every byte, normalized to z64 order. A recognized magic
// and a complete four-byte-aligned stream are required, not a full game header.
// On failure, size counts bytes reported by the stream; sha1 is not a digest.
// The working storage is bounded, independent of the input length.
Result inspect(std::istream& input);

// Identity-only admission of the exact US image; this does not load a game.
Result validate_us(std::istream& input);
std::string digest_hex(const Identity& identity);

} // namespace isle::rom

#endif
