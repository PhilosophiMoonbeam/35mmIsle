#ifndef ISLE_SAVE_HASH_HPP
#define ISLE_SAVE_HASH_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

// Private implementation/test seam, not an installed save API. Implementations
// are independently authored from RFC 1320 mathematical sections 3.1-3.5 and
// FIPS 180-4 sections 4.1.2, 4.2.2, 5 and 6.2; no reference source was imported.
namespace isle::save::detail {

// RFC byte padding/LE bit length, but BE words for EVERY compression block
// (including padding), and BE state-word output. Not ordinary RFC MD4 bytes.
[[nodiscard]] std::array<std::byte, 16> legacy_digest(std::span<const std::byte> input) noexcept;

// Single-use streaming SHA-256. Call finish once, after the final update.
class Sha256 {
public:
    void update(std::span<const std::byte> input) noexcept;
    [[nodiscard]] std::array<std::byte, 32> finish() noexcept;

private:
    void compress(const std::byte* block) noexcept;
    std::array<std::uint32_t, 8> state_{
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19,
    };
    std::array<std::byte, 64> block_;
    std::uint64_t byte_count_ = 0;
    std::size_t used_ = 0;
};

} // namespace isle::save::detail

#endif
