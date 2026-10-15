#include "35mmisle/save_image.hpp"
#include "../src/save_hash.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <span>
#include <string_view>
#include <type_traits>
#include <vector>

namespace s = isle::save;
namespace {

int failures = 0;
void check(bool condition, std::string_view meaning) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << meaning << '\n';
    }
}

bool equals_hex(std::span<const std::byte> bytes, std::string_view hex) {
    if (hex.size() != bytes.size() * 2) return false;
    constexpr std::string_view digits = "0123456789abcdef";
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        const auto value = std::to_integer<unsigned>(bytes[i]);
        if (hex[2 * i] != digits[value >> 4] || hex[2 * i + 1] != digits[value & 15]) return false;
    }
    return true;
}

void put32(std::span<std::byte> bytes, std::size_t offset, std::uint32_t value) {
    for (std::size_t i = 0; i < 4; ++i) bytes[offset + i] = std::byte((value >> (24 - 8 * i)) & 255);
}

void put64(std::span<std::byte> bytes, std::size_t offset, std::uint64_t value) {
    put32(bytes, offset, static_cast<std::uint32_t>(value >> 32));
    put32(bytes, offset + 4, static_cast<std::uint32_t>(value));
}

std::span<const std::byte> text(std::string_view value) {
    return std::as_bytes(std::span{value.data(), value.size()});
}

void expect_image_error(std::span<const std::byte> bytes, s::Error error, std::string_view meaning) {
    const auto result = s::inspect_image(bytes);
    check(!result && result.error == error && !result.image && result.image.bytes().empty(), meaning);
}

void expect_envelope_error(std::span<const std::byte> bytes, s::Error error, std::string_view meaning) {
    const auto result = s::inspect_envelope(bytes);
    check(!result && result.error == error && !result.image && result.image.bytes().empty() &&
          result.generation == 0, meaning);
}

void sha256_standard_vectors() {
    struct Vector { std::string_view input; std::string_view digest; };
    // FIPS 180-4 SHA-256, empty input, and the standard 56/112-byte examples.
    constexpr std::array vectors{
        Vector{"", "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"},
        Vector{"abc", "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"},
        Vector{"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
               "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"},
        Vector{"abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmno"
               "ijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu",
               "cf5b16a778af8380036ce59e7b0492370b249b11e8f07a51afac45037afee9d1"},
    };
    for (const auto& vector : vectors) {
        const auto bytes = text(vector.input);
        // Every two-part split covers partial/full-block update boundaries.
        for (std::size_t split = 0; split <= bytes.size(); ++split) {
            s::detail::Sha256 hash;
            hash.update(bytes.first(split));
            hash.update({});
            hash.update(bytes.subspan(split));
            check(equals_hex(hash.finish(), vector.digest), "SHA-256 standard vector, every update split");
        }
    }
    s::detail::Sha256 million;
    std::array<std::byte, 1000> block;
    block.fill(std::byte{0x61});
    for (int i = 0; i < 1000; ++i) million.update(block);
    check(equals_hex(million.finish(), "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0"),
          "SHA-256 one million a bytes, streamed without concatenation");
}

void observed_legacy_word_convention() {
    struct Vector { std::size_t size; std::string_view digest; std::string_view sha256; };
    // Facts-only actual GAME A/B/C/D observations from run-05-extended of
    // /tmp/save-authority-tv1t8c2k; only independently authored inputs below.
    // No GAME save bytes, executable or GAME/SDK implementation is embedded.
    constexpr std::array vectors{
        Vector{0, "e5b73b53f6526e86b3cc9a3b7fd70114", "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"},
        Vector{1, "99b949016ffac4d63e19237ee8ff79a9", "4bf5122f344554c53bde2ebb8cd2b7e3d1600ad631c385a5d7cce23c7785459a"},
        Vector{2, "70e0f77d043ad1292dc0baec06ca044b", "a12871fee210fb8619291eaea194581cbd2531e4b23759d225f6806923f63222"},
        Vector{3, "81312a4cf8e3cf919ab8a2275d7fecc7", "039058c6f2c0cb492c533b0a4d14ef77cc0f78abccced5287d84a1a2011cfb81"},
        Vector{4, "730b935bef9710b479579d162106927b", "9f64a747e1b97f131fabb6b447296c9b6f0201e79fb3c5356e6c77e89b6a806a"},
        Vector{7, "6ddebe5173bf8b367489daee51562f97", "f463fc91a8e4a2246a53d59cd2ad24331b7e107957e1bd63374674da8c51c14e"},
        Vector{8, "6a0b911e8edbd4fab812fa02b3a8ec0b", "cf00797b54084163f5a2df112db6481395186d47bd701f0f8b886a2e0a5b99b2"},
        Vector{15, "a0219709bf3a684225fe995f3e754c14", "b92edea21b13cd93cc9cfbd35b3ac4f289c9c163139c9c35ff51ecace884d3de"},
        Vector{16, "3996a59f5b143f88102e621e74b6ab7e", "074e0d61fda655126c7e4c1ef836dc48aa537a0a186f5457a768e98bfba87666"},
        Vector{31, "485157295a9fe956114e9801dd3ba8b9", "4f9fe65b8d9350eb609de5f88281cfeae02cb3ffee57f8d3ae410577615aa763"},
        Vector{32, "e6a5c976c939672807ec3264fa625483", "79a214779a8e50a9f9697accc9360b4172c60e98ec4fd5ff7ceeefb4a08978d0"},
        Vector{54, "04f322a5ba776d2cf6c97a2b5520a261", "afe6f896618352d65334934e673fa192ae63f85351c7db64f067bff5e50ea465"},
        Vector{55, "070bca8554a70c069d595d121deaadee", "9f04e5a9c80e5adb3419564bafa29b80fa3671c22e14b0dfa01cd88d86eae0e0"},
        Vector{56, "4a7db2493169b2bd7e32ac938b07961f", "a66ec984512af748ccb8995164464bb67b9250ee625be364db7bedcf9f098971"},
        Vector{57, "92a77624be11f19fdc424272e7a8e206", "eee08842e5ea0b91922e5e58f332ae97fd6f1d3d98e23e27c887d49304a45d90"},
        Vector{60, "b51e0425b9d2b565265234baf02d3c4a", "2547cbf6f22180e49ff3f76f37b61d53827893bdd42e2cb68047ea7637e38543"},
        Vector{63, "9cc5d22d856c4f1fe4297dd9f5d75215", "2bb910e2f12984e8b153bf2b1927e2c05eb9a7b3edbd72d81e5088bdfab122f8"},
        Vector{64, "79a63082957e732e4158ca75fec28d52", "aef9db732932e3242441e03e59a0ee3984569c154da236782c5c657e11be1bc9"},
        Vector{65, "9f2c0e47ebf5178b6a5a1c6d6593aec8", "e033b1dddbc61b2796b16f4d8559c7f340c059d9352bedb863f8bd82bfbea278"},
        Vector{119, "ca20faa69b1ee80d1639872da5211a91", "6468def2f068a1373609c2983142285f5c71a07984d86d5f48ab95b59d49b8db"},
        Vector{120, "ceb0b7028efdd12b73a1caaf97dbd716", "a217efa6802a6580e6ce9299761b8d9212d5d8ac2fbe498672e5172ef452c12b"},
        Vector{127, "5420cfe614a2f039d27148b6851c0961", "fcc659c60c6883d65f89fb59401f5a66b1edcf5dd58b2fef1707927024659220"},
        Vector{128, "ec6967fb69f69f0b2a484af69e6e006f", "ccba277de05e1237eb527f391f52f214c0ff196a14cf03be964d8921719eb9f2"},
        Vector{129, "000bfe19a63a85d3b86738c323e6b042", "ad17a60a7bd72aaa2e901cfc628414c8f92fa73068c792e1f1add9da7f0a127a"},
        Vector{127636, "e6aeeb6a59326af93c33974903fabf06", "fe8400d76e70053eb06990536bc48aeb541c63bc464e31e3c76affb07a9dc292"},
    };
    std::vector<std::byte> authored(127636);
    // byte offset i, not word index: uint32(0x01020304 + i * 0x10201), BE.
    for (std::size_t i = 0; i < authored.size(); i += 4) {
        put32(authored, i, 0x01020304u + static_cast<std::uint32_t>(i) * 0x10201u);
    }
    const auto before = authored;
    for (const auto& vector : vectors) {
        const auto input = std::span<const std::byte>{authored}.first(vector.size);
        check(equals_hex(s::detail::legacy_digest(input), vector.digest),
              "legacy padding/BE compression/BE output matches actual observed GAME words");
        s::detail::Sha256 hash;
        hash.update(input);
        check(equals_hex(hash.finish(), vector.sha256), "authored hash input has the observed independent SHA-256 identity");
        if (vector.size <= 129) {
            for (std::size_t split = 0; split <= input.size(); ++split) {
                s::detail::Sha256 streaming;
                streaming.update(input.first(split));
                streaming.update(input.subspan(split));
                check(equals_hex(streaming.finish(), vector.sha256), "SHA-256 partial blocks match independent boundary identities");
            }
        }
    }
    check(authored == before, "both hash implementations leave borrowed inputs untouched");
}

std::vector<std::byte> authored_image() {
    std::vector<std::byte> bytes(s::image_bytes);
    for (std::size_t i = 0; i < bytes.size(); ++i) bytes[i] = std::byte((i * 131 + i / 7 + 29) & 255);
    constexpr std::string_view version{"HAL_SNAP_V1.0-1\0", 16};
    std::copy(text(version).begin(), text(version).end(), bytes.begin() + 0x10);
    put32(bytes, 0x20, 0x6c);
    put32(bytes, 0x24, 0x180);
    put32(bytes, 0x28, 0xfba0);
    put32(bytes, 0x2c, 0x10a20);
    // Deliberate raw signed/float-like patterns and NONZERO final padding.
    // They are opaque bytes, never interpreted as safe scene/photo records.
    put32(bytes, 0x34, 0x80000000);
    put32(bytes, 0x38, 0x7fc12345);
    put32(bytes, 0x3c, 0xffffffff);
    put32(bytes, 0x40, 0x00000001);
    put32(bytes, bytes.size() - 4, 0xdeadbeef);
    return bytes;
}

void raw_rehash_image(std::span<std::byte> bytes) {
    const auto digest = s::detail::legacy_digest(bytes.subspan(0x10));
    std::copy(digest.begin(), digest.end(), bytes.begin());
}

void default_and_borrowed_results() {
    static_assert(!std::is_constructible_v<s::ImageView, std::span<const std::byte>>);
    static_assert(!std::is_convertible_v<s::ImageView, bool>);
    static_assert(noexcept(s::inspect_image({})));
    static_assert(noexcept(s::seal_image({}, 0)));
    static_assert(noexcept(s::inspect_envelope({})));
    s::ImageView empty;
    check(!empty && empty.bytes().empty(), "default image view is empty");
    const s::ImageResult default_image;
    const s::EnvelopeResult default_envelope;
    check(!default_image && default_image.error != s::Error::none, "default image result cannot masquerade as success");
    check(!default_envelope && default_envelope.error != s::Error::none, "default envelope result cannot masquerade as success");
    check(!s::ImageResult{s::Error::none, {}}, "none error alone does not mint a valid view");
    check(!s::EnvelopeResult{s::Error::none, {}, 1}, "generation alone does not mint an envelope view");

    auto bytes = authored_image();
    const auto before = bytes;
    const auto sealed = s::seal_image(bytes, 0x89abcdef);
    check(bool(sealed), "sealing authored structural bytes succeeds");
    if (!sealed) return;
    check(sealed.image.bytes().data() == bytes.data() && sealed.image.bytes().size() == bytes.size(),
          "seal result borrows the exact caller storage, no payload copy/owner");
    check(equals_hex(std::span<const std::byte>{bytes}.subspan(0x30, 4), "89abcdef"), "raw game time is exactly four BE bytes");
    bool unchanged = true;
    for (std::size_t i = 16; i < bytes.size(); ++i) {
        if ((i < 0x30 || i >= 0x34) && bytes[i] != before[i]) unchanged = false;
    }
    check(unchanged, "sealing preserves every other payload byte, including unknown/signed/float/final-padding bits");
    const auto after = bytes;
    const auto inspected = s::inspect_image(bytes);
    check(inspected && inspected.image.bytes().data() == bytes.data(), "inspection borrows the same immutable bytes");
    check(bytes == after, "inspection has no mutations");
    check(bool(s::seal_image(bytes, 0x89abcdef)) && bytes == after, "sealing the same raw time is byte-identical");
    check(bool(s::seal_image(bytes, 0)) && bool(s::inspect_image(bytes)), "zero raw game time is legal, not native generation");
    check(equals_hex(std::span<const std::byte>{bytes}.subspan(0x30, 4), "00000000"), "zero raw time has no wall-clock substitution");
}

void image_boundary_and_atomic_preflight() {
    auto storage = authored_image();
    storage.push_back(std::byte{0x91});
    constexpr std::array<std::size_t, 15> lengths{0, 1, 15, 16, 31, 32, 47, 48, 51, 52, 63, 64,
                                                s::image_bytes - 4, s::image_bytes - 1, s::image_bytes + 1};
    for (auto size : lengths) {
        const auto before = storage;
        const auto input = std::span<std::byte>{storage}.first(size);
        expect_image_error(input, s::Error::image_size, "every truncated/extra extent rejected before fixed-field reads");
        const auto sealed = s::seal_image(input, 0x12345678);
        check(!sealed && sealed.error == s::Error::image_size && !sealed.image && storage == before,
              "wrong-size seal is completely mutation-free");
    }
    for (std::size_t i = 0; i < 16; ++i) {
        auto bytes = authored_image();
        bytes[0x10 + i] ^= std::byte{1};
        raw_rehash_image(bytes);
        expect_image_error(bytes, s::Error::image_version, "all 16 version bytes matter even with a matching digest");
        const auto before = bytes;
        const auto sealed = s::seal_image(bytes, 0x12345678);
        check(!sealed && sealed.error == s::Error::image_version && !sealed.image && bytes == before,
              "bad version seal changes neither time nor digest nor other bytes");
    }
    constexpr std::array<std::uint32_t, 4> offsets{0x6c, 0x180, 0xfba0, 0x10a20};
    for (std::size_t field = 0; field < offsets.size(); ++field) {
        const std::array<std::uint32_t, 6> bad{0, 0xffffffff, 0x800c21b0, offsets[field] - 1,
                                               offsets[field] + 4, static_cast<std::uint32_t>(s::image_bytes)};
        for (auto value : bad) {
            auto bytes = authored_image();
            put32(bytes, 0x20 + field * 4, value);
            raw_rehash_image(bytes);
            expect_image_error(bytes, s::Error::image_offsets, "canonical relative offsets required even when legacy digest is valid");
            const auto before = bytes;
            const auto sealed = s::seal_image(bytes, 0xfedcba98);
            check(!sealed && sealed.error == s::Error::image_offsets && !sealed.image && bytes == before,
                  "malformed relative field seal is completely mutation-free");
        }
    }
    auto bytes = authored_image();
    check(bool(s::seal_image(bytes, 7)), "prepare aligned boundary fixture");
    std::vector<std::byte> unaligned(s::image_bytes + 2, std::byte{0x67});
    std::copy(bytes.begin(), bytes.end(), unaligned.begin() + 1);
    auto image = std::span<std::byte>{unaligned}.subspan(1, s::image_bytes);
    check(bool(s::inspect_image(image)), "structural inspect does not require host-word alignment");
    check(bool(s::seal_image(image, 0xffffffff)) && bool(s::inspect_image(image)), "unaligned image seal uses explicit byte order");
    check(unaligned.front() == std::byte{0x67} && unaligned.back() == std::byte{0x67}, "seal never writes outside the exact borrowed image");
}

void image_digest_and_tail() {
    auto good = authored_image();
    check(bool(s::seal_image(good, 0x76543210)), "prepare corruption fixture");
    for (std::size_t i = 0; i < 16; ++i) {
        auto bytes = good;
        bytes[i] ^= std::byte{0x80};
        expect_image_error(bytes, s::Error::image_digest, "every stored digest byte participates in verification");
    }
    constexpr std::array<std::size_t, 10> corruptions{0x30, 0x33, 0x34, 0x6c, 0x180, 0x10000,
                                                     s::image_bytes - 4, s::image_bytes - 3,
                                                     s::image_bytes - 2, s::image_bytes - 1};
    for (auto index : corruptions) {
        auto bytes = good;
        bytes[index] ^= std::byte{1};
        expect_image_error(bytes, s::Error::image_digest, "every selected payload boundary, including all final four bytes, is hashed");
    }
    auto repair = good;
    std::fill(repair.begin(), repair.begin() + 16, std::byte{0});
    expect_image_error(repair, s::Error::image_digest, "structural authored/unsealed data are not valid without a seal");
    check(bool(s::seal_image(repair, 0x76543210)) && repair == good, "seal repairs a digest without interpreting/reinitializing opaque payload bytes");
}

std::vector<std::byte> envelope_of(std::span<const std::byte> image, std::uint64_t generation) {
    const auto inspected = s::inspect_image(image);
    check(bool(inspected), "prepare envelope from a valid authored image");
    s::EnvelopeHeader header;
    const auto error = s::encode_envelope_header(inspected.image, generation, header);
    check(error == s::Error::none, "encode envelope header");
    std::vector<std::byte> bytes(s::envelope_file_bytes);
    std::copy(header.begin(), header.end(), bytes.begin());
    std::copy(image.begin(), image.end(), bytes.begin() + s::envelope_header_bytes);
    return bytes;
}

void raw_rehash_envelope(std::span<std::byte> file) {
    // Deliberately use one complete-file stream with the hash field actually
    // zeroed: independent segmentation from the production three-span path.
    std::fill(file.begin() + 0x60, file.begin() + 0x80, std::byte{0});
    s::detail::Sha256 hash;
    hash.update(file);
    const auto digest = hash.finish();
    std::copy(digest.begin(), digest.end(), file.begin() + 0x60);
}

void envelope_wire_and_preflight() {
    auto image = authored_image();
    check(bool(s::seal_image(image, 0x12345678)), "prepare envelope wire fixture");
    const auto before = image;
    const auto result = s::inspect_image(image);
    s::EnvelopeHeader header;
    header.fill(std::byte{0xa5});
    const auto unchanged = header;
    check(s::encode_envelope_header({}, 1, header) == s::Error::invalid_argument && header == unchanged,
          "empty borrowed view is rejected without touching output header");
    check(s::encode_envelope_header(result.image, 0, header) == s::Error::envelope_generation && header == unchanged,
          "generation zero is rejected before changing output header");
    check(s::encode_envelope_header(result.image, 0x0102030405060708, header) == s::Error::none,
          "nontrivial 64-bit generation encoded");
    check(image == before, "header encoding never mutates/canonicalizes image payload");
    check(equals_hex(std::span<const std::byte>{header}.first(0x60),
          "33354d5341564500000100800000000101020304050607080001f2a400000000"
          "a1d5d816db7f8557ee04c35a011326d058b2c1fbca76b57b352b1d705a1ec1cc"
          "df8815e75f3d79618466e01e0827c6165b93d11c000000000000000000000000"),
          "exact 128-byte envelope prefix layout, fixed ROM/source identities and zero reserved bytes");
    auto envelope = envelope_of(image, 0x0102030405060708);
    check(std::equal(header.begin(), header.end(), envelope.begin()), "header encoding is deterministic");
    const auto decoded = s::inspect_envelope(envelope);
    check(decoded && decoded.generation == 0x0102030405060708 &&
          decoded.image.bytes().data() == envelope.data() + s::envelope_header_bytes,
          "decoder publishes the exact borrowed payload and full BE generation");
    check(decoded && std::equal(image.begin(), image.end(), decoded.image.bytes().begin()),
          "opaque original payload survives the envelope byte-for-byte");
    const auto saved_digest = std::span<const std::byte>{envelope}.subspan(0x60, 32);
    const std::array<std::byte, 32> expected_digest = [&] {
        std::array<std::byte, 32> copy;
        std::copy(saved_digest.begin(), saved_digest.end(), copy.begin());
        return copy;
    }();
    raw_rehash_envelope(envelope);
    check(std::equal(expected_digest.begin(), expected_digest.end(), envelope.begin() + 0x60),
          "envelope hash covers the complete header with hash field zero, then the complete payload");
    for (auto generation : {std::uint64_t{1}, std::numeric_limits<std::uint64_t>::max()}) {
        const auto bytes = envelope_of(image, generation);
        const auto inspected = s::inspect_envelope(bytes);
        check(inspected && inspected.generation == generation, "codec accepts the full nonzero generation domain without truncation");
    }
    std::vector<std::byte> unaligned(envelope.size() + 2, std::byte{0x35});
    std::copy(envelope.begin(), envelope.end(), unaligned.begin() + 1);
    const auto borrowed = std::span<const std::byte>{unaligned}.subspan(1, envelope.size());
    const auto unaligned_result = s::inspect_envelope(borrowed);
    check(unaligned_result && unaligned_result.image.bytes().data() == borrowed.data() + 128,
          "unaligned envelope parse has no host integer/pointer layout dependency");
}

void envelope_boundary_and_metadata() {
    auto image = authored_image();
    check(bool(s::seal_image(image, 42)), "prepare envelope metadata fixture");
    const auto good = envelope_of(image, 5);
    auto extended = good;
    extended.push_back(std::byte{0});
    constexpr std::array<std::size_t, 13> lengths{0, 7, 8, 9, 15, 16, 31, 95, 127, 128,
                                                 s::envelope_file_bytes - 4, s::envelope_file_bytes - 1,
                                                 s::envelope_file_bytes + 1};
    for (auto size : lengths) {
        expect_envelope_error(std::span<const std::byte>{extended}.first(size), s::Error::envelope_size,
                              "envelope requires exact EOF, never a header-derived unchecked extent");
    }
    struct Field { std::size_t offset; std::size_t size; s::Error error; };
    constexpr std::array fields{
        Field{0, 8, s::Error::envelope_magic},
        Field{8, 2, s::Error::envelope_version},
        Field{10, 2, s::Error::envelope_header},
        Field{12, 4, s::Error::envelope_codec},
        Field{24, 4, s::Error::envelope_header},
        Field{28, 4, s::Error::envelope_header},
        Field{32, 32, s::Error::envelope_baseline},
        Field{64, 20, s::Error::envelope_baseline},
        Field{84, 12, s::Error::envelope_header},
    };
    for (const auto field : fields) {
        for (std::size_t i = 0; i < field.size; ++i) {
            auto bytes = good;
            bytes[field.offset + i] ^= std::byte{0x80};
            expect_envelope_error(bytes, field.error, "all fixed metadata bytes validated without publishing recovery-looking success");
        }
    }
    {
        auto bytes = good;
        put64(bytes, 0x10, 0);
        raw_rehash_envelope(bytes);
        expect_envelope_error(bytes, s::Error::envelope_generation, "zero generation rejected even with a matching envelope hash");
    }
    {
        auto bytes = good;
        bytes[9] = std::byte{2};
        raw_rehash_envelope(bytes);
        expect_envelope_error(bytes, s::Error::envelope_version, "future version is not silently migrated/reset");
    }
    {
        auto bytes = good;
        put32(bytes, 0x0c, 2);
        raw_rehash_envelope(bytes);
        expect_envelope_error(bytes, s::Error::envelope_codec, "future codec is not treated as original image bytes");
    }
    check(bool(s::inspect_envelope(good)), "all rejection cases leave the original immutable envelope usable");
}

void envelope_integrity_and_inner_rejection() {
    auto image = authored_image();
    check(bool(s::seal_image(image, 7)), "prepare layered-integrity fixture");
    const auto good = envelope_of(image, 0x0102030405060708);
    for (std::size_t i = 0; i < 8; ++i) {
        auto bytes = good;
        bytes[0x10 + i] ^= std::byte{0x10};
        expect_envelope_error(bytes, s::Error::envelope_digest, "every generation byte belongs to the envelope hash, not just the payload");
    }
    for (std::size_t i = 0; i < 32; ++i) {
        auto bytes = good;
        bytes[0x60 + i] ^= std::byte{1};
        expect_envelope_error(bytes, s::Error::envelope_digest, "every envelope digest byte is verified");
    }
    for (auto offset : {s::envelope_header_bytes, s::envelope_header_bytes + 0x1000,
                        s::envelope_file_bytes - 4, s::envelope_file_bytes - 1}) {
        auto bytes = good;
        bytes[offset] ^= std::byte{1};
        expect_envelope_error(bytes, s::Error::envelope_digest, "outer integrity covers original digest, body and final padding");
    }
    {
        auto bytes = good;
        auto payload = std::span<std::byte>{bytes}.subspan(s::envelope_header_bytes);
        payload[0x1000] ^= std::byte{1};
        check(bool(s::seal_image(payload, 8)), "reseal changed opaque payload for independent outer-hash test");
        expect_envelope_error(bytes, s::Error::envelope_digest, "valid resealed original image cannot bypass stale envelope integrity");
        raw_rehash_envelope(bytes);
        check(bool(s::inspect_envelope(bytes)), "both explicit seals admit a structurally valid changed image, no semantic photo claim");
    }
    {
        auto bytes = good;
        bytes[s::envelope_header_bytes + 0x1000] ^= std::byte{1};
        raw_rehash_envelope(bytes);
        expect_envelope_error(bytes, s::Error::image_digest, "matching envelope SHA-256 does not bypass original digest validation");
    }
    {
        auto bytes = good;
        auto payload = std::span<std::byte>{bytes}.subspan(s::envelope_header_bytes);
        payload[0x1f] ^= std::byte{1};
        raw_rehash_image(payload);
        raw_rehash_envelope(bytes);
        expect_envelope_error(bytes, s::Error::image_version, "two valid hashes cannot bypass the complete original version tag");
    }
    {
        auto bytes = good;
        auto payload = std::span<std::byte>{bytes}.subspan(s::envelope_header_bytes);
        put32(payload, 0x2c, 0x800d0000);
        raw_rehash_image(payload);
        raw_rehash_envelope(bytes);
        expect_envelope_error(bytes, s::Error::image_offsets, "two valid hashes cannot publish noncanonical absolute/relative offsets");
    }
}

} // namespace

int main() {
    sha256_standard_vectors();
    observed_legacy_word_convention();
    default_and_borrowed_results();
    image_boundary_and_atomic_preflight();
    image_digest_and_tail();
    envelope_wire_and_preflight();
    envelope_boundary_and_metadata();
    envelope_integrity_and_inner_rejection();
    if (failures != 0) return EXIT_FAILURE;
    std::cout << "Save image/envelope math, borrowing, boundaries and corruption checks passed using authored data only.\n";
    return EXIT_SUCCESS;
}
