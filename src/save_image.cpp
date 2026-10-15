#include "35mmisle/save_image.hpp"
#include "save_hash.hpp"

#include <algorithm>
#include <bit>

namespace isle::save {
namespace {

std::uint32_t read32(const std::byte* bytes) noexcept {
    return (std::to_integer<std::uint32_t>(bytes[0]) << 24) |
           (std::to_integer<std::uint32_t>(bytes[1]) << 16) |
           (std::to_integer<std::uint32_t>(bytes[2]) << 8) |
           std::to_integer<std::uint32_t>(bytes[3]);
}

std::uint16_t read16(const std::byte* bytes) noexcept {
    return static_cast<std::uint16_t>((std::to_integer<std::uint16_t>(bytes[0]) << 8) |
                                    std::to_integer<std::uint16_t>(bytes[1]));
}

std::uint64_t read64(const std::byte* bytes) noexcept {
    return (std::uint64_t{read32(bytes)} << 32) | read32(bytes + 4);
}

void write16(std::byte* bytes, std::uint16_t value) noexcept {
    bytes[0] = std::byte(value >> 8);
    bytes[1] = std::byte(value & 0xff);
}

void write32(std::byte* bytes, std::uint32_t value) noexcept {
    for (std::size_t i = 0; i < 4; ++i) {
        bytes[i] = std::byte((value >> (24 - 8 * i)) & 0xff);
    }
}

void write64(std::byte* bytes, std::uint64_t value) noexcept {
    write32(bytes, static_cast<std::uint32_t>(value >> 32));
    write32(bytes + 4, static_cast<std::uint32_t>(value));
}

constexpr std::array<std::byte, 16> image_version{
    std::byte{0x48}, std::byte{0x41}, std::byte{0x4c}, std::byte{0x5f},
    std::byte{0x53}, std::byte{0x4e}, std::byte{0x41}, std::byte{0x50},
    std::byte{0x5f}, std::byte{0x56}, std::byte{0x31}, std::byte{0x2e},
    std::byte{0x30}, std::byte{0x2d}, std::byte{0x31}, std::byte{0x00},
};
constexpr std::array<std::uint32_t, 4> image_offsets{0x6c, 0x180, 0xfba0, 0x10a20};
constexpr std::array<std::byte, 8> envelope_magic{
    std::byte{0x33}, std::byte{0x35}, std::byte{0x4d}, std::byte{0x53},
    std::byte{0x41}, std::byte{0x56}, std::byte{0x45}, std::byte{0x00},
};
constexpr std::array<std::byte, 32> baseline_rom{
    std::byte{0xa1}, std::byte{0xd5}, std::byte{0xd8}, std::byte{0x16},
    std::byte{0xdb}, std::byte{0x7f}, std::byte{0x85}, std::byte{0x57},
    std::byte{0xee}, std::byte{0x04}, std::byte{0xc3}, std::byte{0x5a},
    std::byte{0x01}, std::byte{0x13}, std::byte{0x26}, std::byte{0xd0},
    std::byte{0x58}, std::byte{0xb2}, std::byte{0xc1}, std::byte{0xfb},
    std::byte{0xca}, std::byte{0x76}, std::byte{0xb5}, std::byte{0x7b},
    std::byte{0x35}, std::byte{0x2b}, std::byte{0x1d}, std::byte{0x70},
    std::byte{0x5a}, std::byte{0x1e}, std::byte{0xc1}, std::byte{0xcc},
};
constexpr std::array<std::byte, 20> baseline_source{
    std::byte{0xdf}, std::byte{0x88}, std::byte{0x15}, std::byte{0xe7},
    std::byte{0x5f}, std::byte{0x3d}, std::byte{0x79}, std::byte{0x61},
    std::byte{0x84}, std::byte{0x66}, std::byte{0xe0}, std::byte{0x1e},
    std::byte{0x08}, std::byte{0x27}, std::byte{0xc6}, std::byte{0x16},
    std::byte{0x5b}, std::byte{0x93}, std::byte{0xd1}, std::byte{0x1c},
};

Error image_preflight(std::span<const std::byte> bytes) noexcept {
    // A single exact-extent gate bounds every subsequent fixed-field read.
    if (bytes.size() != image_bytes) return Error::image_size;
    if (!std::equal(image_version.begin(), image_version.end(), bytes.begin() + 0x10)) {
        return Error::image_version;
    }
    for (std::size_t i = 0; i < image_offsets.size(); ++i) {
        if (read32(bytes.data() + 0x20 + i * 4) != image_offsets[i]) return Error::image_offsets;
    }
    return Error::none;
}

std::array<std::byte, 32> envelope_digest(std::span<const std::byte> header,
                                         std::span<const std::byte> payload) noexcept {
    constexpr std::array<std::byte, 32> zero_hash{};
    detail::Sha256 hash;
    hash.update(header.first(0x60));
    hash.update(zero_hash);
    hash.update(payload);
    return hash.finish();
}

} // namespace

namespace detail {
namespace {

// Independently expressed RFC 1320 section 3.4 equations. The rolling working
// register order follows ABCD, DABC, CDAB, BCDA; no GAME/SDK implementation.
void legacy_compress(std::array<std::uint32_t, 4>& state, const std::byte* block) noexcept {
    std::array<std::uint32_t, 16> words;
    for (std::size_t i = 0; i < words.size(); ++i) words[i] = read32(block + i * 4);
    auto [a, b, c, d] = state;
    const auto step = [&](std::uint32_t function, std::size_t word,
                          std::uint32_t constant, int rotation) noexcept {
        const auto next = std::rotl(a + function + words[word] + constant, rotation);
        a = d;
        d = c;
        c = b;
        b = next;
    };
    constexpr std::array<int, 4> first_shifts{3, 7, 11, 19};
    constexpr std::array<int, 4> second_shifts{3, 5, 9, 13};
    constexpr std::array<int, 4> third_shifts{3, 9, 11, 15};
    constexpr std::array<std::size_t, 16> third_order{
        0, 8, 4, 12, 2, 10, 6, 14, 1, 9, 5, 13, 3, 11, 7, 15,
    };
    for (std::size_t i = 0; i < 16; ++i) {
        step((b & c) | (~b & d), i, 0, first_shifts[i % 4]);
    }
    for (std::size_t i = 0; i < 16; ++i) {
        step((b & c) | (b & d) | (c & d), (i % 4) * 4 + i / 4,
             0x5a827999, second_shifts[i % 4]);
    }
    for (std::size_t i = 0; i < 16; ++i) {
        step(b ^ c ^ d, third_order[i], 0x6ed9eba1, third_shifts[i % 4]);
    }
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
}

} // namespace

std::array<std::byte, 16> legacy_digest(std::span<const std::byte> input) noexcept {
    std::array<std::uint32_t, 4> state{0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476};
    const auto bit_length = static_cast<std::uint64_t>(input.size()) * 8;
    while (input.size() >= 64) {
        legacy_compress(state, input.data());
        input = input.subspan(64);
    }
    std::array<std::byte, 64> tail{};
    std::copy(input.begin(), input.end(), tail.begin());
    tail[input.size()] = std::byte{0x80};
    if (input.size() >= 56) {
        legacy_compress(state, tail.data());
        tail.fill(std::byte{0});
    }
    // RFC byte padding/LE length is applied BEFORE the observed BE word load.
    for (std::size_t i = 0; i < 8; ++i) {
        tail[56 + i] = std::byte((bit_length >> (8 * i)) & 0xff);
    }
    legacy_compress(state, tail.data());
    std::array<std::byte, 16> output;
    for (std::size_t i = 0; i < state.size(); ++i) write32(output.data() + i * 4, state[i]);
    return output;
}

void Sha256::compress(const std::byte* block) noexcept {
    // FIPS 180-4 4.2.2: fractional cube-root constants, not implementation code.
    constexpr std::array<std::uint32_t, 64> constants{
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
    };
    std::array<std::uint32_t, 64> words;
    for (std::size_t i = 0; i < 16; ++i) words[i] = read32(block + i * 4);
    for (std::size_t i = 16; i < words.size(); ++i) {
        const auto x = words[i - 15];
        const auto y = words[i - 2];
        const auto sigma0 = std::rotr(x, 7) ^ std::rotr(x, 18) ^ (x >> 3);
        const auto sigma1 = std::rotr(y, 17) ^ std::rotr(y, 19) ^ (y >> 10);
        words[i] = words[i - 16] + sigma0 + words[i - 7] + sigma1;
    }
    auto [a, b, c, d, e, f, g, h] = state_;
    for (std::size_t i = 0; i < words.size(); ++i) {
        const auto sum1 = std::rotr(e, 6) ^ std::rotr(e, 11) ^ std::rotr(e, 25);
        const auto choose = (e & f) ^ (~e & g);
        const auto first = h + sum1 + choose + constants[i] + words[i];
        const auto sum0 = std::rotr(a, 2) ^ std::rotr(a, 13) ^ std::rotr(a, 22);
        const auto majority = (a & b) ^ (a & c) ^ (b & c);
        const auto second = sum0 + majority;
        h = g;
        g = f;
        f = e;
        e = d + first;
        d = c;
        c = b;
        b = a;
        a = first + second;
    }
    state_[0] += a;
    state_[1] += b;
    state_[2] += c;
    state_[3] += d;
    state_[4] += e;
    state_[5] += f;
    state_[6] += g;
    state_[7] += h;
}

void Sha256::update(std::span<const std::byte> input) noexcept {
    byte_count_ += static_cast<std::uint64_t>(input.size());
    if (used_ != 0) {
        const auto count = std::min(input.size(), block_.size() - used_);
        std::copy_n(input.begin(), count, block_.begin() + used_);
        used_ += count;
        input = input.subspan(count);
        if (used_ != block_.size()) return;
        compress(block_.data());
        used_ = 0;
    }
    while (input.size() >= block_.size()) {
        compress(input.data());
        input = input.subspan(block_.size());
    }
    if (!input.empty()) {
        std::copy(input.begin(), input.end(), block_.begin());
        used_ = input.size();
    }
}

std::array<std::byte, 32> Sha256::finish() noexcept {
    block_[used_++] = std::byte{0x80};
    if (used_ > 56) {
        std::fill(block_.begin() + used_, block_.end(), std::byte{0});
        compress(block_.data());
        used_ = 0;
    }
    std::fill(block_.begin() + used_, block_.begin() + 56, std::byte{0});
    write64(block_.data() + 56, byte_count_ * 8);
    compress(block_.data());
    std::array<std::byte, 32> output;
    for (std::size_t i = 0; i < state_.size(); ++i) write32(output.data() + i * 4, state_[i]);
    return output;
}

} // namespace detail

ImageResult inspect_image(std::span<const std::byte> bytes) noexcept {
    const auto error = image_preflight(bytes);
    if (error != Error::none) return {error, {}};
    const auto digest = detail::legacy_digest(bytes.subspan(0x10));
    if (!std::equal(digest.begin(), digest.end(), bytes.begin())) return {Error::image_digest, {}};
    return {Error::none, ImageView{bytes}};
}

ImageResult seal_image(std::span<std::byte> bytes, std::uint32_t raw_game_time) noexcept {
    const auto error = image_preflight(bytes);
    if (error != Error::none) return {error, {}};
    // All fallible checks precede the first write; hashing is bounded/noexcept.
    write32(bytes.data() + 0x30, raw_game_time);
    const auto digest = detail::legacy_digest(bytes.subspan(0x10));
    std::copy(digest.begin(), digest.end(), bytes.begin());
    return {Error::none, ImageView{bytes}};
}

Error encode_envelope_header(ImageView image, std::uint64_t generation, EnvelopeHeader& output) noexcept {
    if (!image) return Error::invalid_argument;
    if (generation == 0) return Error::envelope_generation;
    // Preflight is complete; bounded/noexcept construction cannot fail now.
    std::copy(envelope_magic.begin(), envelope_magic.end(), output.begin());
    write16(output.data() + 0x08, 1);
    write16(output.data() + 0x0a, envelope_header_bytes);
    write32(output.data() + 0x0c, 1);
    write64(output.data() + 0x10, generation);
    write32(output.data() + 0x18, image_bytes);
    write32(output.data() + 0x1c, 0);
    std::copy(baseline_rom.begin(), baseline_rom.end(), output.begin() + 0x20);
    std::copy(baseline_source.begin(), baseline_source.end(), output.begin() + 0x40);
    std::fill(output.begin() + 0x54, output.begin() + 0x60, std::byte{0});
    const auto digest = envelope_digest(output, image.bytes());
    std::copy(digest.begin(), digest.end(), output.begin() + 0x60);
    return Error::none;
}

EnvelopeResult inspect_envelope(std::span<const std::byte> bytes) noexcept {
    // Exact EOF first: no field read occurs for any short or overlong input.
    if (bytes.size() != envelope_file_bytes) return {Error::envelope_size, {}, 0};
    const auto header = bytes.first(envelope_header_bytes);
    const auto* data = header.data();
    if (!std::equal(envelope_magic.begin(), envelope_magic.end(), header.begin())) {
        return {Error::envelope_magic, {}, 0};
    }
    if (read16(data + 0x08) != 1) return {Error::envelope_version, {}, 0};
    if (read16(data + 0x0a) != envelope_header_bytes || read32(data + 0x18) != image_bytes ||
        read32(data + 0x1c) != 0 ||
        !std::all_of(header.begin() + 0x54, header.begin() + 0x60,
                     [](std::byte value) { return value == std::byte{0}; })) {
        return {Error::envelope_header, {}, 0};
    }
    if (read32(data + 0x0c) != 1) return {Error::envelope_codec, {}, 0};
    if (!std::equal(baseline_rom.begin(), baseline_rom.end(), header.begin() + 0x20) ||
        !std::equal(baseline_source.begin(), baseline_source.end(), header.begin() + 0x40)) {
        return {Error::envelope_baseline, {}, 0};
    }
    const auto generation = read64(data + 0x10);
    if (generation == 0) return {Error::envelope_generation, {}, 0};
    const auto payload = bytes.subspan(envelope_header_bytes);
    const auto digest = envelope_digest(header, payload);
    if (!std::equal(digest.begin(), digest.end(), header.begin() + 0x60)) {
        return {Error::envelope_digest, {}, 0};
    }
    const auto image = inspect_image(payload);
    if (!image) return {image.error, {}, 0};
    return {Error::none, image.image, generation};
}

} // namespace isle::save
