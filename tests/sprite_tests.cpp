#include "35mmisle/sprite.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <memory>
#include <span>
#include <utility>
#include <vector>

namespace {

using isle::sprite::Bank;
using isle::sprite::BankOwner;
using isle::sprite::Error;
using isle::sprite::Sprite;

int failures = 0;

void check(bool condition, const char* description) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << description << '\n';
    }
}

// Independently authored synthetic banks. Unaligned structures and high-bit
// canonical addresses deliberately rule out host casts and signed relocation.
struct Fixture {
    static constexpr std::uint32_t base = 0xf3210000U;
    static constexpr std::size_t root = 17;
    static constexpr std::size_t descriptors = 128;
    static constexpr std::size_t texels = 512;
    static constexpr std::size_t palette = 768;

    Bank bytes = Bank(1024, 0);

    Fixture() {
        word(root, 0xfebf);       // x = -321
        word(root + 2, 1027);
        word(root + 4, 5);
        word(root + 6, 6);
        dword(root + 8, 0x3fc00000);  // 1.5, not native float serialization
        dword(root + 12, 0xbf400000); // -0.75
        word(root + 16, 0xfff9);
        word(root + 18, 9);
        word(root + 20, ISLE_SP_TRANSPARENT | ISLE_SP_SCALE);
        word(root + 22, 0x8001);
        bytes[root + 24] = 19;
        bytes[root + 25] = 71;
        bytes[root + 26] = 123;
        bytes[root + 27] = 177;
        word(root + 38, 1);
        word(root + 40, 1);
        word(root + 42, 32);
        word(root + 44, 1);
        word(root + 46, 2);
        bytes[root + 48] = 0;
        bytes[root + 49] = 2;
        dword(root + 52, address(descriptors));
        dword(root + 56, 0x12345678); // cache pointers: not bank references/code
        dword(root + 60, 0x9abcdef0);
        word(root + 64, 33);
        word(root + 66, 0xffef);
        bitmap(0, 3, 4, address(texels), 1);
        data(texels, {0xf8, 0x01, 0x07, 0xc1, 0x00, 0x3f, 0xff, 0xfe,
                      0x08, 0x43, 0x80, 0x21, 0x40, 0x10, 0x00, 0x00});
    }

    static std::uint32_t address(std::size_t offset) {
        return base + static_cast<std::uint32_t>(offset);
    }

    void word(std::size_t offset, unsigned value) {
        bytes[offset] = static_cast<std::uint8_t>(value >> 8);
        bytes[offset + 1] = static_cast<std::uint8_t>(value);
    }

    void dword(std::size_t offset, std::uint32_t value) {
        bytes[offset] = static_cast<std::uint8_t>(value >> 24);
        bytes[offset + 1] = static_cast<std::uint8_t>(value >> 16);
        bytes[offset + 2] = static_cast<std::uint8_t>(value >> 8);
        bytes[offset + 3] = static_cast<std::uint8_t>(value);
    }

    void data(std::size_t offset, std::initializer_list<std::uint8_t> values) {
        std::copy(values.begin(), values.end(), bytes.begin() + static_cast<std::ptrdiff_t>(offset));
    }

    void bitmap(std::size_t index, unsigned width, unsigned stored_width,
                std::uint32_t buffer, unsigned actual_height) {
        const auto offset = descriptors + index * 16;
        word(offset, width);
        word(offset + 2, stored_width);
        word(offset + 4, 0);
        word(offset + 6, 0);
        dword(offset + 8, buffer);
        word(offset + 12, actual_height);
        word(offset + 14, 0);
    }

    void plane(unsigned format, unsigned size, unsigned width, unsigned height) {
        bytes[root + 48] = static_cast<std::uint8_t>(format);
        bytes[root + 49] = static_cast<std::uint8_t>(size);
        word(root + 44, height);
        word(root + 46, height);
        bitmap(0, width, width, address(texels), height);
    }

    void lut(unsigned start, unsigned count) {
        word(root + 28, start);
        word(root + 30, count);
        dword(root + 32, address(palette));
        data(palette, {0xf8, 0x01, 0x07, 0xc1, 0x00, 0x3f});
    }

    BankOwner owner() const { return std::make_shared<const Bank>(bytes); }

    isle::sprite::HydrateResult hydrate() const {
        return isle::sprite::hydrate(owner(), base, address(root));
    }
};

constexpr std::array<std::uint8_t, 32> rgba16_expected{
    255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255, 255, 255, 255, 0,
    8, 8, 8, 255, 132, 0, 132, 255, 66, 0, 66, 0, 0, 0, 0, 0};

void expect_pixels(const IsleSprite& sprite, std::size_t index,
                   std::span<const std::uint8_t> expected,
                   std::size_t width, std::size_t height, const char* description) {
    std::vector<std::uint8_t> destination(expected.size() + 7, 0xcd);
    const auto result = isle::sprite::normalize_rgba8(
        sprite, index, std::span<std::uint8_t>(destination).subspan(1, expected.size() + 3));
    check(static_cast<bool>(result), description);
    check(result.error == Error::none, "normalization error is explicitly none");
    check(result.width == width && result.height == height, "exact stored normalization dimensions");
    check(result.bytes_written == expected.size(), "exact normalized byte count");
    check(std::equal(expected.begin(), expected.end(), destination.begin() + 1), "fixed RGBA8 texel expectation");
    check(destination.front() == 0xcd, "normalization leading guard unchanged");
    check(std::all_of(destination.begin() + static_cast<std::ptrdiff_t>(expected.size() + 1),
                      destination.end(), [](std::uint8_t byte) { return byte == 0xcd; }),
          "normalization unused capacity and trailing guards unchanged");
}

void expect_failure(const Fixture& fixture, Error error, const char* description) {
    const auto result = fixture.hydrate();
    check(result.error == error, description);
    check(!result && !result.asset && result.asset.view() == nullptr, "hydration failure publishes no native view");
}

void wire_scalars() {
    Fixture fixture;
    auto owner = fixture.owner();
    auto result = isle::sprite::hydrate(owner, Fixture::base, Fixture::address(Fixture::root));
    check(static_cast<bool>(result), "unaligned high-address RGBA16 graph hydrates");
    if (!result) {
        return;
    }
    const auto* view = result.asset.view();
    check(view->x == -321 && view->y == 1027 && view->width == 5 && view->height == 6,
          "BE signed placement and dimensions");
    check(view->scalex == 1.5F && view->scaley == -0.75F, "BE IEEE float scalars decoded by bits");
    check(view->expx == -7 && view->expy == 9 && view->zdepth == -32767, "signed explosion and depth scalars");
    check(view->attr == (ISLE_SP_TRANSPARENT | ISLE_SP_SCALE), "unsigned attributes preserved");
    check(view->red == 19 && view->green == 71 && view->blue == 123 && view->alpha == 177,
          "independent tint channels preserved");
    check(view->startTLUT == 0 && view->nTLUT == 0 && view->LUT == nullptr && view->LUT_bytes == 0,
          "empty lookup table owns no hidden pointer");
    check(view->istart == 0 && view->istep == 1 && view->nbitmaps == 1 && view->ndisplist == 32,
          "bitmap traversal and display list count scalars");
    check(view->bmheight == 1 && view->bmHreal == 2 && view->bmfmt == 0 && view->bmsiz == 2,
          "nominal grid dimensions and pixel format preserved");
    check(view->frac_s == 33 && view->frac_t == -17, "signed fractional scalars");
    const auto& bitmap = view->bitmap[0];
    check(bitmap.width == 3 && bitmap.width_img == 4 && bitmap.s == 0 && bitmap.t == 0 &&
          bitmap.actualHeight == 1 && bitmap.LUToffset == 0,
          "canonical bitmap scalar fields decode independently");
}

void short_planes_and_owned_lifetime() {
    Fixture tail;
    tail.word(Fixture::root + 44, 3);
    tail.word(Fixture::root + 46, 3);
    tail.word(Fixture::root + 20, ISLE_SP_TEXSHUF | ISLE_SP_TRANSPARENT);
    tail.bitmap(0, 3, 4, Fixture::address(Fixture::texels), 2);
    tail.data(Fixture::texels + 8, {0x40, 0x10, 0x00, 0x00, 0x08, 0x43, 0x80, 0x21});
    tail.bytes.resize(Fixture::texels + 16);
    auto owner = tail.owner();
    std::weak_ptr<const Bank> weak = owner;
    auto result = isle::sprite::hydrate(owner, Fixture::base, Fixture::address(Fixture::root));
    check(static_cast<bool>(result), "short final plane ending exactly at bank boundary hydrates");
    if (!result) return;
    check(result.asset.view()->bitmap[0].byte_count == 16,
          "positive actualHeight bounds stored bytes independently of nominal height");
    expect_pixels(*result.asset.view(), 0, rgba16_expected, 4, 2,
                  "short RGBA16 plane reverses its real odd row without invented padding");

    std::array<std::uint8_t, 40> output;
    output.fill(0xa7);
    const auto short_output = isle::sprite::normalize_rgba8(*result.asset.view(), 0,
                                                           std::span<std::uint8_t>(output).first(31));
    check(short_output.error == Error::output_too_small && short_output.width == 4 &&
          short_output.height == 2 && short_output.bytes_written == 0 &&
          std::all_of(output.begin(), output.end(), [](std::uint8_t value) { return value == 0xa7; }),
          "real short-plane output capacity rejects without writes");
    IsleSprite bare = *result.asset.view();
    IsleBitmap truncated = bare.bitmap[0];
    truncated.byte_count = 15;
    IsleSprite bad_extent = bare;
    bad_extent.bitmap = &truncated;
    const auto rejected = isle::sprite::normalize_rgba8(bad_extent, 0, output);
    check(rejected.error == Error::invalid_bitmap && rejected.bytes_written == 0 &&
          std::all_of(output.begin(), output.end(), [](std::uint8_t value) { return value == 0xa7; }),
          "missing final physical byte cannot become a normalized texel");
    bare.red = bare.green = bare.blue = 7;
    Sprite retained = result.asset;
    owner.reset();
    result = {};
    Sprite moved = std::move(retained);
    expect_pixels(bare, 0, rgba16_expected, 4, 2,
                  "borrowed C view still normalizes untinted pixels after resident owner moves");
    moved = {};
    check(weak.expired(), "final resident lease releases the canonical bank");
    tail.bytes.pop_back();
    expect_failure(tail, Error::out_of_bank, "one missing byte in a short final plane rejects");

    Fixture nominal;
    nominal.word(Fixture::descriptors + 12, 0);
    nominal.bytes.resize(Fixture::texels + 16);
    const auto fallback = nominal.hydrate();
    check(static_cast<bool>(fallback), "zero actualHeight retains the declared nominal plane");
    if (fallback) expect_pixels(*fallback.asset.view(), 0, rgba16_expected, 4, 2,
                               "zero actualHeight uses nominal storage rather than an empty image");
    Fixture one_row;
    one_row.bitmap(0, 3, 3, Fixture::address(Fixture::texels), 1);
    one_row.word(Fixture::root + 20, ISLE_SP_TEXSHUF);
    one_row.bytes.resize(Fixture::texels + 6);
    const auto single = one_row.hydrate();
    const std::array<std::uint8_t, 12> colors{255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255};
    check(static_cast<bool>(single), "one real row needs no nonexistent odd-row shuffle padding");
    if (single) expect_pixels(*single.asset.view(), 0, colors, 3, 1, "single-row shuffle preserves exact texels");
}

void aliased_buffers_keep_distinct_plane_extents() {
    Fixture aliases;
    aliases.plane(0, 2, 4, 2);
    aliases.word(Fixture::root + 40, 2);
    aliases.bitmap(1, 4, 4, Fixture::address(Fixture::texels), 1);
    aliases.bytes.resize(Fixture::texels + 16);
    const auto result = aliases.hydrate();
    check(static_cast<bool>(result), "one canonical buffer supports distinct bitmap extents");
    if (!result) return;
    const std::array<std::uint8_t, 16> first_row{
        255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255, 255, 255, 255, 0};
    expect_pixels(*result.asset.view(), 0, rgba16_expected, 4, 2,
                  "full alias retains both encoded rows");
    expect_pixels(*result.asset.view(), 1, first_row, 4, 1,
                  "short alias does not inherit another descriptor's nominal extent");
}



void bounds_and_scalars() {
    auto no_owner = isle::sprite::hydrate({}, Fixture::base, Fixture::address(Fixture::root));
    check(no_owner.error == Error::invalid_owner && !no_owner.asset, "NULL bank owner rejects without publishing");
    Fixture fixture;
    const auto overflow = isle::sprite::hydrate(fixture.owner(), 0xfffffff8U, 0xfffffff8U);
    check(overflow.error == Error::address_space_overflow && !overflow.asset, "canonical bank wrapping 32 bits rejects");
    const auto below = isle::sprite::hydrate(fixture.owner(), Fixture::base, Fixture::base - 1);
    check(below.error == Error::out_of_bank && below.address == Fixture::base - 1 && !below.asset,
          "root below base cannot underflow into bank");
    const auto outside = isle::sprite::hydrate(fixture.owner(), Fixture::base, Fixture::address(1024));
    check(outside.error == Error::out_of_bank && !outside.asset, "root exactly at bank end has no 68-byte extent");
    for (std::size_t length = 0; length < 68; ++length) {
        Fixture truncated;
        truncated.bytes.resize(Fixture::root + length);
        expect_failure(truncated, Error::out_of_bank, "every truncated Sprite wire length rejects");
    }
    Fixture exact;
    exact.word(Fixture::root + 40, 0);
    exact.word(Fixture::root + 44, 0);
    exact.word(Fixture::root + 46, 0);
    exact.bytes.resize(Fixture::root + 68);
    check(static_cast<bool>(exact.hydrate()), "root ending at exact bank boundary succeeds without descriptors");
    const auto limit = isle::sprite::hydrate(exact.owner(), 0xffffffabU, 0xffffffbcU);
    check(static_cast<bool>(limit), "bank ending exactly at unsigned 32-bit address limit succeeds");

    Fixture bad_descriptors;
    bad_descriptors.dword(Fixture::root + 52, Fixture::address(1024 - 15));
    expect_failure(bad_descriptors, Error::out_of_bank, "descriptor extent rejects missing final byte");
    bad_descriptors.dword(Fixture::root + 52, Fixture::base - 16);
    expect_failure(bad_descriptors, Error::out_of_bank, "descriptor canonical pointer before bank rejects");
    bad_descriptors.dword(Fixture::root + 52, 0);
    expect_failure(bad_descriptors, Error::invalid_bitmap, "nonempty descriptor array requires non-NULL pointer");

    Fixture bad_texels;
    bad_texels.dword(Fixture::descriptors + 8, Fixture::base - 1);
    expect_failure(bad_texels, Error::out_of_bank, "texture pointer underflow rejects");

    for (const auto offset : {4U, 6U, 36U, 40U, 42U, 44U, 46U}) {
        Fixture invalid;
        invalid.word(Fixture::root + offset, 0xffff);
        expect_failure(invalid, Error::invalid_scalar, "negative count or dimension rejects");
    }
    for (const auto bits : {0x7f800000U, 0xff800000U, 0x7fc00001U}) {
        Fixture invalid;
        invalid.dword(Fixture::root + 8, bits);
        expect_failure(invalid, Error::invalid_scalar, "nonfinite scale rejects");
        invalid.dword(Fixture::root + 8, 0x3f800000U);
        invalid.dword(Fixture::root + 12, bits);
        expect_failure(invalid, Error::invalid_scalar, "nonfinite second scale rejects");
    }
    Fixture invalid_start;
    invalid_start.word(Fixture::root + 36, 1);
    expect_failure(invalid_start, Error::invalid_scalar, "bitmap start is bounded by count");
    Fixture external;
    external.word(Fixture::root + 20, ISLE_SP_EXTERN);
    expect_failure(external, Error::unsupported_external_state, "external display state is not silently emulated");

    Fixture invalid_owner_retention;
    invalid_owner_retention.dword(Fixture::root + 52, 0);
    auto owner = invalid_owner_retention.owner();
    std::weak_ptr<const Bank> weak = owner;
    const auto failed = isle::sprite::hydrate(owner, Fixture::base, Fixture::address(Fixture::root));
    owner.reset();
    check(!failed.asset && weak.expired(), "failed factory publishes neither partial storage nor retained bank");
}

void bitmap_shapes_and_skips() {
    for (const auto pair : {std::pair{0U, 0xffffU}, std::pair{2U, 0U}, std::pair{2U, 0xffffU},
                            std::pair{2U, 2U}, std::pair{12U, 3U}}) {
        Fixture invalid;
        invalid.word(Fixture::descriptors + pair.first, pair.second);
        expect_failure(invalid, Error::invalid_bitmap, "active bitmap width, stored stride, or height error rejects");
    }
    Fixture zero_height;
    zero_height.word(Fixture::root + 46, 0);
    expect_failure(zero_height, Error::invalid_bitmap, "active bitmap needs real storage height");
    Fixture used_height;
    used_height.word(Fixture::root + 44, 3);
    expect_failure(used_height, Error::invalid_bitmap, "used bitmap height cannot exceed real height");
    Fixture visible_empty;
    visible_empty.word(Fixture::descriptors + 12, 0);
    check(static_cast<bool>(visible_empty.hydrate()), "zero visible height does not erase valid storage");
    Fixture load_only;
    load_only.word(Fixture::descriptors, 7);
    load_only.word(Fixture::descriptors + 4, 8);
    load_only.word(Fixture::descriptors + 6, 0xffff);
    check(static_cast<bool>(load_only.hydrate()), "load-only s and signed texture offsets are not drawing validation");

    Fixture null_buffer;
    null_buffer.bitmap(0, 9, 0xffff, 0, 0xffff);
    const auto skipped = null_buffer.hydrate();
    check(static_cast<bool>(skipped), "NULL buffer skip does not require valid unused texture geometry");
    if (skipped) {
        check(skipped.asset.view()->bitmap[0].buf == nullptr && skipped.asset.view()->bitmap[0].byte_count == 0,
              "NULL buffer has no hidden native texture reference");
        expect_pixels(*skipped.asset.view(), 0, {}, 0, 0, "NULL bitmap normalizes to an empty plane");
    }

    Fixture terminated;
    terminated.word(Fixture::root + 40, 3);
    terminated.bitmap(0, 0, 0xffff, 0x01020304, 0xffff);
    terminated.bitmap(1, 0xffff, 0xffff, 0xffffffff, 0xffff);
    terminated.bitmap(2, 1, 1, Fixture::base - 1, 1);
    const auto ended = terminated.hydrate();
    check(static_cast<bool>(ended), "width-zero terminator ignores all unused buffer references including suffix");
    if (ended) {
        for (std::size_t i = 0; i < 3; ++i) {
            check(ended.asset.view()->bitmap[i].buf == nullptr && ended.asset.view()->bitmap[i].byte_count == 0,
                  "terminated descriptors never publish external pointers");
            expect_pixels(*ended.asset.view(), i, {}, 0, 0, "terminated suffix normalizes empty");
        }
    }


    Fixture empty;
    empty.word(Fixture::root + 40, 0);
    empty.word(Fixture::root + 44, 0);
    empty.word(Fixture::root + 46, 0);
    empty.dword(Fixture::root + 52, 0x01020304);
    empty.dword(Fixture::root + 32, 0xffffffff);
    const auto empty_result = empty.hydrate();
    check(static_cast<bool>(empty_result), "zero counts do not resolve unused descriptor and LUT addresses");
    if (empty_result) {
        check(empty_result.asset.view()->bitmap == nullptr && empty_result.asset.view()->LUT == nullptr,
              "empty graph contains no borrowed references");
    }
}

void format_texels() {
    Fixture rgba32;
    rgba32.plane(0, 3, 4, 2);
    rgba32.data(Fixture::texels, {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
        25, 26, 27, 28, 29, 30, 31, 32, 17, 18, 19, 20, 21, 22, 23, 24});
    rgba32.word(Fixture::root + 20, ISLE_SP_TEXSHUF);
    const std::array<std::uint8_t, 32> rgba32_pixels{
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
        17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32};
    const auto rgba32_result = rgba32.hydrate();
    check(static_cast<bool>(rgba32_result), "RGBA32 shuffled storage hydrates");
    if (rgba32_result) {
        expect_pixels(*rgba32_result.asset.view(), 0, rgba32_pixels, 4, 2,
                      "RGBA32 reverses two-pixel halves, not adjacent-pixel swaps");
    }


    Fixture i4;
    i4.plane(4, 0, 3, 2);
    i4.data(Fixture::texels, {0x1a, 0xf0, 0x08, 0x50});
    const std::array<std::uint8_t, 24> i4_pixels{
        17, 17, 17, 17, 170, 170, 170, 170, 255, 255, 255, 255,
        0, 0, 0, 0, 136, 136, 136, 136, 85, 85, 85, 85};
    const auto i4_result = i4.hydrate();
    check(static_cast<bool>(i4_result), "odd-width nibble rows hydrate without guessed padding");
    if (i4_result) {
        check(i4_result.asset.view()->bitmap[0].byte_count == 4, "odd-width I4 storage has rounded bytes per row");
        expect_pixels(*i4_result.asset.view(), 0, i4_pixels, 3, 2, "I4 high nibble first and alpha equal intensity");
    }

    Fixture ia4;
    ia4.plane(3, 0, 3, 1);
    ia4.data(Fixture::texels, {0x1e, 0x70});
    const std::array<std::uint8_t, 12> ia4_pixels{0, 0, 0, 255, 255, 255, 255, 0, 109, 109, 109, 255};
    const auto ia4_result = ia4.hydrate();
    check(static_cast<bool>(ia4_result), "IA4 hydrates");
    if (ia4_result) {
        expect_pixels(*ia4_result.asset.view(), 0, ia4_pixels, 3, 1, "IA31 uses replicated three-bit intensity and one-bit alpha");
    }

    Fixture ia8;
    ia8.plane(3, 1, 2, 1);
    ia8.data(Fixture::texels, {0xf0, 0x2b});
    const std::array<std::uint8_t, 8> ia8_pixels{255, 255, 255, 0, 34, 34, 34, 187};
    const auto ia8_result = ia8.hydrate();
    check(static_cast<bool>(ia8_result), "IA8 hydrates");
    if (ia8_result) {
        expect_pixels(*ia8_result.asset.view(), 0, ia8_pixels, 2, 1, "IA44 channel expansion");
    }

    Fixture ia16;
    ia16.plane(3, 2, 2, 1);
    ia16.data(Fixture::texels, {0x12, 0x34, 0xfe, 0x80});
    const std::array<std::uint8_t, 8> ia16_pixels{18, 18, 18, 52, 254, 254, 254, 128};
    const auto ia16_result = ia16.hydrate();
    check(static_cast<bool>(ia16_result), "IA16 hydrates");
    if (ia16_result) {
        expect_pixels(*ia16_result.asset.view(), 0, ia16_pixels, 2, 1, "IA88 canonical channel order");
    }

    Fixture i8;
    i8.plane(4, 1, 2, 1);
    i8.data(Fixture::texels, {0x2a, 0x9f});
    const std::array<std::uint8_t, 8> i8_pixels{42, 42, 42, 42, 159, 159, 159, 159};
    const auto i8_result = i8.hydrate();
    check(static_cast<bool>(i8_result), "I8 hydrates");
    if (i8_result) {
        expect_pixels(*i8_result.asset.view(), 0, i8_pixels, 2, 1, "I8 alpha is intensity, not forced opaque");
    }

    for (const auto pair : {std::pair{0U, 0U}, std::pair{0U, 1U}, std::pair{1U, 2U},
                            std::pair{2U, 2U}, std::pair{3U, 3U}, std::pair{4U, 2U},
                            std::pair{5U, 2U}, std::pair{0U, 5U}, std::pair{255U, 255U}}) {
        Fixture unsupported;
        unsupported.bytes[Fixture::root + 48] = static_cast<std::uint8_t>(pair.first);
        unsupported.bytes[Fixture::root + 49] = static_cast<std::uint8_t>(pair.second);
        expect_failure(unsupported, Error::unsupported_format, "unsupported format/size pair never falls back to RGBA");
    }
}

void multi_bitmap_rgba16_closure() {
    Fixture fixture;
    fixture.word(Fixture::root + 4, 4);
    fixture.word(Fixture::root + 6, 16);
    fixture.word(Fixture::root + 40, 8);
    fixture.word(Fixture::root + 44, 2);
    fixture.word(Fixture::root + 20, ISLE_SP_TEXSHUF | ISLE_SP_TRANSPARENT);
    constexpr std::array<unsigned, 8> colors{
        0xf801, 0x07c1, 0x003f, 0xffff, 0x0001, 0xffc1, 0xf83f, 0x07ff};
    constexpr std::array<std::array<std::uint8_t, 4>, 8> pixels{{
        {255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}, {255, 255, 255, 255},
        {0, 0, 0, 255}, {255, 255, 0, 255}, {255, 0, 255, 255}, {0, 255, 255, 255}}};
    for (std::size_t i = 0; i < colors.size(); ++i) {
        const auto offset = Fixture::texels + i * 16;
        fixture.bitmap(i, 4, 4, Fixture::address(offset), 2);
        for (std::size_t texel = 0; texel < 8; ++texel) {
            fixture.word(offset + texel * 2, colors[i]);
        }
    }
    fixture.bytes.resize(Fixture::texels + colors.size() * 16);
    const auto result = fixture.hydrate();
    check(static_cast<bool>(result), "complete eight-piece RGBA16 graph hydrates at exact final texture boundary");
    if (!result) {
        return;
    }
    const auto* view = result.asset.view();
    check(view->width == 4 && view->height == 16 && view->nbitmaps == 8,
          "multi-piece root retains full placement dimensions and descriptor count");
    for (std::size_t i = 0; i < colors.size(); ++i) {
        std::array<std::uint8_t, 32> expected{};
        for (std::size_t texel = 0; texel < 8; ++texel) {
            std::copy(pixels[i].begin(), pixels[i].end(), expected.begin() + static_cast<std::ptrdiff_t>(texel * 4));
        }
        check(view->bitmap[i].byte_count == 16 && view->bitmap[i].actualHeight == 2,
              "every reachable RGBA16 piece has exact stored extent");
        expect_pixels(*view, i, expected, 4, 2, "every piece normalizes its independently fixed color");
    }
}

void palettes() {
    Fixture ci4;
    ci4.plane(2, 0, 4, 1);
    ci4.lut(48, 3);
    ci4.word(Fixture::descriptors + 14, 3);
    ci4.data(Fixture::texels, {0x02, 0x10});
    const std::array<std::uint8_t, 16> ci4_pixels{
        255, 0, 0, 255, 0, 0, 255, 255, 0, 255, 0, 255, 255, 0, 0, 255};
    const auto ci4_result = ci4.hydrate();
    check(static_cast<bool>(ci4_result), "CI4 bank selection and partial TLUT range hydrate");
    if (ci4_result) {
        check(ci4_result.asset.view()->LUT_bytes == 6, "CI LUT is two canonical bytes per entry");
        expect_pixels(*ci4_result.asset.view(), 0, ci4_pixels, 4, 1, "CI4 palette bank resolves BE RGBA5551 entries");
    }

    Fixture ci8;
    ci8.plane(2, 1, 3, 1);
    ci8.lut(7, 3);
    ci8.data(Fixture::texels, {9, 7, 8});
    ci8.word(Fixture::descriptors + 14, 0xffff); // CI8 ignores the CI4 selector.
    const std::array<std::uint8_t, 12> ci8_pixels{
        0, 0, 255, 255, 255, 0, 0, 255, 0, 255, 0, 255};
    const auto ci8_result = ci8.hydrate();
    check(static_cast<bool>(ci8_result), "CI8 indices resolve a nonzero TLUT start");
    if (ci8_result) {
        expect_pixels(*ci8_result.asset.view(), 0, ci8_pixels, 3, 1, "CI8 uses full indices, not CI4 selector");
    }

    Fixture missing = ci4;
    missing.dword(Fixture::root + 32, 0);
    expect_failure(missing, Error::invalid_palette, "nonempty NULL LUT rejects");
    missing = ci4;
    missing.dword(Fixture::root + 32, Fixture::address(1024 - 5));
    expect_failure(missing, Error::out_of_bank, "palette extent rejects missing final byte");
    missing = ci4;
    missing.dword(Fixture::root + 32, Fixture::address(1024 - 6));
    missing.data(1024 - 6, {0xf8, 0x01, 0x07, 0xc1, 0x00, 0x3f});
    check(static_cast<bool>(missing.hydrate()), "palette ending at exact bank boundary succeeds");
    missing = ci4;
    missing.word(Fixture::root + 30, 0);
    expect_failure(missing, Error::invalid_palette, "active CI texture cannot borrow undeclared external palette");
    for (const auto offset : {28U, 30U}) {
        missing = ci4;
        missing.word(Fixture::root + offset, 0xffff);
        expect_failure(missing, Error::invalid_palette, "negative palette start or count rejects");
    }
    missing = ci4;
    missing.word(Fixture::root + 28, 255);
    expect_failure(missing, Error::invalid_palette, "TLUT range cannot exceed 256 entries");
    for (const auto selector : {16U, 0xffffU}) {
        missing = ci4;
        missing.word(Fixture::descriptors + 14, selector);
        expect_failure(missing, Error::invalid_palette, "CI4 palette bank is bounded 0 through 15");
    }
    missing = ci4;
    missing.data(Fixture::texels, {0x03, 0x10});
    expect_failure(missing, Error::invalid_palette, "all CI4 indices must be inside loaded TLUT closure");
    missing = ci8;
    missing.data(Fixture::texels, {6, 7, 8});
    expect_failure(missing, Error::invalid_palette, "CI8 index below loaded start rejects");
    missing = ci8;
    missing.data(Fixture::texels, {9, 7, 10});
    expect_failure(missing, Error::invalid_palette, "last CI8 index beyond loaded range rejects before publishing");
}

void normalization_boundaries() {
    Fixture fixture;
    const auto hydrated = fixture.hydrate();
    check(static_cast<bool>(hydrated), "normalization boundary fixture hydrates");
    if (!hydrated) {
        return;
    }
    std::array<std::uint8_t, 40> output;
    output.fill(0xa7);
    const auto unchanged = [&output] {
        return std::all_of(output.begin(), output.end(), [](std::uint8_t value) { return value == 0xa7; });
    };
    const auto bad_index = isle::sprite::normalize_rgba8(*hydrated.asset.view(), 1, output);
    check(bad_index.error == Error::bitmap_index_out_of_range && bad_index.bytes_written == 0 && unchanged(),
          "bitmap index is checked before descriptor access");
    IsleSprite mutable_view = *hydrated.asset.view();
    mutable_view.bitmap = nullptr;
    const auto missing = isle::sprite::normalize_rgba8(mutable_view, 0, output);
    check(missing.error == Error::invalid_bitmap && unchanged(), "NULL native descriptor array rejects before reads");
    mutable_view = *hydrated.asset.view();
    mutable_view.attr |= ISLE_SP_EXTERN;
    const auto external = isle::sprite::normalize_rgba8(mutable_view, 0, output);
    check(external.error == Error::unsupported_external_state && unchanged(), "copied external-state view remains unsupported");

    Fixture bad_32_shuffle;
    bad_32_shuffle.plane(0, 3, 2, 2);
    bad_32_shuffle.word(Fixture::root + 20, ISLE_SP_TEXSHUF);
    expect_failure(bad_32_shuffle, Error::invalid_bitmap, "RGBA32 shuffle requires sixteen-byte row blocks");

    Fixture partial_ci;
    partial_ci.plane(2, 1, 3, 1);
    partial_ci.lut(7, 3);
    partial_ci.data(Fixture::texels, {7, 9, 8});
    const auto ci = partial_ci.hydrate();
    check(static_cast<bool>(ci), "CI partial-write contract fixture hydrates");
    if (ci) {
        mutable_view = *ci.asset.view();
        mutable_view.nTLUT = 1;
        mutable_view.LUT_bytes = 2;
        const auto invalid = isle::sprite::normalize_rgba8(mutable_view, 0, output);
        check(invalid.error == Error::invalid_palette && invalid.bytes_written == 4,
              "modified invalid CI range reports exact complete-texel prefix");
        const std::array<std::uint8_t, 4> first{255, 0, 0, 255};
        check(std::equal(first.begin(), first.end(), output.begin()), "CI prefix is real normalized palette data");
        check(std::all_of(output.begin() + 4, output.end(), [](std::uint8_t value) { return value == 0xa7; }),
              "invalid CI index never writes outside completed prefix");
    }
}

} // namespace

int main() {
    wire_scalars();
    short_planes_and_owned_lifetime();
    aliased_buffers_keep_distinct_plane_extents();
    bounds_and_scalars();
    bitmap_shapes_and_skips();
    format_texels();
    multi_bitmap_rgba16_closure();
    palettes();
    normalization_boundaries();
    if (failures != 0) {
        std::cerr << failures << " sprite checks failed\n";
        return 1;
    }
    std::cout << "sprite tests passed\n";
    return 0;
}
