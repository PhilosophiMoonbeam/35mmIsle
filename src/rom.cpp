#include "35mmisle/rom.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstring>
#include <istream>
#include <limits>
#include <utility>

namespace isle::rom {
namespace {

// Original implementation of the SHA-1 arithmetic and padding specified in
// RFC 3174 sections 4-6. SHA-1 is used only for ROM identity, not security.
class Sha1 {
public:
    void append(const std::uint8_t* data, std::size_t size) noexcept {
        byte_count_ += static_cast<std::uint64_t>(size);
        if (used_ != 0) {
            const auto take = std::min(size, block_.size() - used_);
            std::memcpy(block_.data() + used_, data, take);
            used_ += take;
            data += take;
            size -= take;
            if (used_ == block_.size()) {
                compress(block_.data());
                used_ = 0;
            }
        }
        while (size >= block_.size()) {
            compress(data);
            data += block_.size();
            size -= block_.size();
        }
        if (size != 0) {
            std::memcpy(block_.data(), data, size);
            used_ = size;
        }
    }

    std::array<std::uint8_t, 20> finish() noexcept {
        const std::uint64_t bits = byte_count_ * 8;
        block_[used_++] = 0x80;
        if (used_ > 56) {
            std::fill(block_.begin() + static_cast<std::ptrdiff_t>(used_), block_.end(), 0);
            compress(block_.data());
            used_ = 0;
        }
        std::fill(block_.begin() + static_cast<std::ptrdiff_t>(used_), block_.begin() + 56, 0);
        for (std::size_t i = 0; i < 8; ++i) {
            block_[56 + i] = static_cast<std::uint8_t>(bits >> ((7 - i) * 8));
        }
        compress(block_.data());
        std::array<std::uint8_t, 20> digest{};
        for (std::size_t i = 0; i < state_.size(); ++i) {
            for (std::size_t j = 0; j < 4; ++j) {
                digest[i * 4 + j] = static_cast<std::uint8_t>(state_[i] >> ((3 - j) * 8));
            }
        }
        return digest;
    }

private:
    void compress(const std::uint8_t* data) noexcept {
        std::array<std::uint32_t, 80> words;
        for (std::size_t i = 0; i < 16; ++i) {
            words[i] = (static_cast<std::uint32_t>(data[i * 4]) << 24)
                | (static_cast<std::uint32_t>(data[i * 4 + 1]) << 16)
                | (static_cast<std::uint32_t>(data[i * 4 + 2]) << 8)
                | static_cast<std::uint32_t>(data[i * 4 + 3]);
        }
        for (std::size_t i = 16; i < words.size(); ++i) {
            words[i] = std::rotl(words[i - 3] ^ words[i - 8] ^ words[i - 14] ^ words[i - 16], 1);
        }
        auto a = state_[0];
        auto b = state_[1];
        auto c = state_[2];
        auto d = state_[3];
        auto e = state_[4];
        for (std::size_t round = 0; round < words.size(); ++round) {
            std::uint32_t mix;
            std::uint32_t constant;
            if (round < 20) {
                mix = (b & c) | (~b & d);
                constant = 0x5a827999U;
            } else if (round < 40) {
                mix = b ^ c ^ d;
                constant = 0x6ed9eba1U;
            } else if (round < 60) {
                mix = (b & c) | (b & d) | (c & d);
                constant = 0x8f1bbcdcU;
            } else {
                mix = b ^ c ^ d;
                constant = 0xca62c1d6U;
            }
            const auto next = std::rotl(a, 5) + mix + e + words[round] + constant;
            e = d;
            d = c;
            c = std::rotl(b, 30);
            b = a;
            a = next;
        }
        state_[0] += a;
        state_[1] += b;
        state_[2] += c;
        state_[3] += d;
        state_[4] += e;
    }

    std::array<std::uint32_t, 5> state_{
        0x67452301U, 0xefcdab89U, 0x98badcfeU, 0x10325476U, 0xc3d2e1f0U};
    std::array<std::uint8_t, 64> block_;
    std::size_t used_ = 0;
    std::uint64_t byte_count_ = 0;
};

constexpr std::array<std::uint8_t, 4> z64_magic{0x80, 0x37, 0x12, 0x40};
constexpr std::array<std::uint8_t, 4> v64_magic{0x37, 0x80, 0x40, 0x12};
constexpr std::array<std::uint8_t, 4> n64_magic{0x40, 0x12, 0x37, 0x80};
constexpr std::array<std::uint8_t, 20> us_sha1{
    0xed, 0xc7, 0xc4, 0x9c, 0xc5, 0x68, 0xc0, 0x45, 0xfe, 0x48,
    0xbe, 0x0d, 0x18, 0x01, 0x1c, 0x30, 0xf3, 0x93, 0xcb, 0xaf};

// An exception-enabled stream still reports ordinary EOF as EOF, not I/O
// failure. All other stream exceptions and bad/non-EOF fail states are errors.
bool read_chunk(std::istream& input, std::uint8_t* data, std::size_t capacity,
                std::size_t& count) {
    bool unexpected_exception = false;
    try {
        input.read(reinterpret_cast<char*>(data), static_cast<std::streamsize>(capacity));
    } catch (const std::ios_base::failure&) {
        unexpected_exception = input.bad() || !input.eof();
    } catch (...) {
        unexpected_exception = true;
    }
    const auto extracted = input.gcount();
    count = extracted > 0 ? static_cast<std::size_t>(extracted) : 0;
    return !unexpected_exception && !input.bad() && !(input.fail() && !input.eof())
        && extracted >= 0 && count <= capacity
        && (count == capacity || input.eof());
}

void normalize(std::uint8_t* data, std::size_t size, ByteOrder order) noexcept {
    if (order == ByteOrder::v64) {
        for (std::size_t i = 0; i < size; i += 2) {
            std::swap(data[i], data[i + 1]);
        }
    } else if (order == ByteOrder::n64) {
        for (std::size_t i = 0; i < size; i += 4) {
            std::swap(data[i], data[i + 3]);
            std::swap(data[i + 1], data[i + 2]);
        }
    }
}

} // namespace

Result inspect(std::istream& input) {
    Result result{{ByteOrder::z64, 0, {}}, Error::none};
    std::array<std::uint8_t, 4> header;
    std::size_t count = 0;
    const bool header_ok = read_chunk(input, header.data(), header.size(), count);
    result.identity.size = static_cast<std::uint64_t>(count);
    if (!header_ok) {
        result.error = Error::io;
        return result;
    }
    if (count != header.size()) {
        result.error = Error::truncated;
        return result;
    }
    if (header == z64_magic) {
        result.identity.order = ByteOrder::z64;
    } else if (header == v64_magic) {
        result.identity.order = ByteOrder::v64;
    } else if (header == n64_magic) {
        result.identity.order = ByteOrder::n64;
    } else {
        result.error = Error::unsupported_order;
        return result;
    }

    Sha1 hash;
    hash.append(z64_magic.data(), z64_magic.size());
    // One in-place normalization buffer, SHA-1's block and schedule: < 34 KiB.
    std::array<std::uint8_t, 32768> buffer;
    for (;;) {
        const bool chunk_ok = read_chunk(input, buffer.data(), buffer.size(), count);
        // SHA-1's message length is strictly less than 2^64 bits.
        constexpr auto max_bytes = std::numeric_limits<std::uint64_t>::max() / 8;
        if (count > max_bytes - result.identity.size) {
            result.error = Error::io;
            return result;
        }
        result.identity.size += static_cast<std::uint64_t>(count);
        if (!chunk_ok) {
            result.error = Error::io;
            return result;
        }
        if (count % 4 != 0) {
            result.error = Error::truncated;
            return result;
        }
        if (count != 0) {
            normalize(buffer.data(), count, result.identity.order);
            hash.append(buffer.data(), count);
        }
        if (input.eof()) {
            result.identity.sha1 = hash.finish();
            return result;
        }
    }
}

Result validate_us(std::istream& input) {
    auto result = inspect(input);
    if (!result) {
        return result;
    }
    if (result.identity.size != us_size) {
        result.error = Error::wrong_size;
    } else if (result.identity.sha1 != us_sha1) {
        result.error = Error::wrong_identity;
    }
    return result;
}

std::string digest_hex(const Identity& identity) {
    constexpr char digits[] = "0123456789abcdef";
    std::string text(identity.sha1.size() * 2, '0');
    for (std::size_t i = 0; i < identity.sha1.size(); ++i) {
        text[i * 2] = digits[identity.sha1[i] >> 4];
        text[i * 2 + 1] = digits[identity.sha1[i] & 0x0f];
    }
    return text;
}

} // namespace isle::rom
