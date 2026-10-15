#include "35mmisle/photo_feedback.hpp"

#include <array>
#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <span>
#include <vector>

namespace {
namespace pf = isle::photo_feedback;
int failures = 0;

void check(bool condition, const char* description) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << description << '\n';
    }
}

struct Fixture {
    static constexpr std::uint32_t base = 0x104;
    std::vector<std::uint8_t> bytes = std::vector<std::uint8_t>(base + pf::buffer_bytes + 16, 0xa5);

    Fixture() { clear(); }

    // Independent pair packing: the second BE halfword precedes the first
    // within each reversed four-byte RDRAM word. No native-endian u16 stores.
    void word(std::size_t plane, unsigned x, unsigned y, std::uint16_t value) {
        const auto index = static_cast<std::size_t>(y) * 64 + x;
        const auto group = base + plane * 0x1800 + (index / 2) * 4;
        const auto high = group + (index % 2 == 0 ? 3 : 1);
        bytes[high] = static_cast<std::uint8_t>(value >> 8);
        bytes[high - 1] = static_cast<std::uint8_t>(value);
    }

    void clear() {
        for (std::size_t plane = 0; plane < 26; ++plane)
            for (unsigned y = 0; y < 48; ++y)
                for (unsigned x = 0; x < 64; ++x)
                    word(plane, x, y, 0xfffc);
    }

    void sample(std::size_t subject, unsigned x, unsigned y,
                std::uint16_t a, std::uint16_t c, std::uint16_t b) {
        word(2 + subject, x, y, a);
        word(1, x, y, c);
        word(14 + subject, x, y, b);
    }

    pf::PlanesView borrow() const {
        pf::PlanesView view;
        check(pf::PlanesView::borrow(bytes, base, view), "borrow completed synthetic planes");
        return view;
    }
};

void empty_and_dense_planes() {
    Fixture f;
    pf::WideCounts wide;
    wide.fill({123, 45});
    pf::FinalCounts final;
    final.fill({1, 2, 3, 4, 5});
    check(pf::count_wide(f.borrow(), wide), "empty wide reduction succeeds");
    check(wide == pf::WideCounts{}, "all twelve empty wide entries replaced");
    check(pf::count_narrow(f.borrow(), wide, final), "empty narrow reduction succeeds");
    check(final == pf::FinalCounts{}, "all twelve empty narrow entries replaced");

    // Maximum counts in every slot, including the last A/B planes.
    for (unsigned y = 0; y < 48; ++y) {
        for (unsigned x = 0; x < 64; ++x) {
            f.word(1, x, y, 0x4321);
            for (std::size_t i = 0; i < 12; ++i) f.word(2 + i, x, y, 0x4321);
        }
    }
    check(pf::count_wide(f.borrow(), wide), "dense wide reduction succeeds");
    for (const auto& entry : wide)
        check(entry == pf::WideCount{3072, 660}, "maximum wide counts and inclusive center area");
    check(pf::count_narrow(f.borrow(), wide, final), "dense narrow reduction succeeds");
    for (const auto& entry : final)
        check(entry == pf::FinalCount{3072, 16, 3072, 4, 14298},
              "maximum narrow counts and bounded integer product");
}

void packed_depth_and_coincident_occlusion() {
    Fixture f;
    // All packed low bits matter, including values adjacent to clear FFFC.
    f.sample(0, 31, 23, 0x1234, 0x1235, 0xfffc); // A/C differ only in dZ low bit.
    f.sample(0, 32, 23, 0x1235, 0x1235, 0x1234); // B/C low-bit difference is visible.
    f.sample(0, 31, 24, 0x1235, 0x1235, 0x1235); // Coincident before=current: occluded.
    f.sample(0, 32, 24, 0xfffc, 0xfffc, 0x1111); // Clear A must never count.
    f.sample(0, 0, 0, 0xfffd, 0xfffd, 0xfffc);   // Not clear after masking dZ.
    f.sample(0, 1, 0, 0xfffe, 0xfffe, 0xfffc);
    f.sample(0, 2, 0, 0xffff, 0xffff, 0xfffc);
    f.sample(0, 3, 0, 0x1236, 0x1235, 0xfffc); // No numeric depth tolerance.
    pf::WideCounts wide{};
    pf::FinalCounts final{};
    check(pf::count_wide(f.borrow(), wide), "packed wide succeeds");
    check(wide[0] == pf::WideCount{7, 3}, "only exact FFFC is clear");
    check(pf::count_narrow(f.borrow(), wide, final), "packed narrow succeeds");
    check(final[0] == pf::FinalCount{7, 3, 4, 1, 16},
          "strict packed equality and before-current occlusion, integer truncation");
    for (std::size_t i = 1; i < 12; ++i)
        check(final[i] == pf::FinalCount{}, "subject samples do not leak to empty planes");
}

struct Point { unsigned x, y; };

void inclusive_windows() {
    Fixture f;
    // Four corners included; each immediately adjacent outer edge excluded.
    constexpr std::array<Point, 12> wide_points{{
        {17, 13}, {46, 13}, {17, 34}, {46, 34},
        {16, 13}, {47, 13}, {16, 34}, {47, 34},
        {17, 12}, {46, 12}, {17, 35}, {46, 35}}};
    for (std::size_t i = 0; i < wide_points.size(); ++i)
        f.word(2 + i, wide_points[i].x, wide_points[i].y, 0x2233);
    pf::WideCounts wide{};
    check(pf::count_wide(f.borrow(), wide), "wide edge samples succeed");
    for (std::size_t i = 0; i < wide_points.size(); ++i)
        check(wide[i] == pf::WideCount{1, i < 4 ? 1u : 0u},
              "wide window includes corners and excludes each adjacent edge");

    constexpr std::array<Point, 12> narrow_points{{
        {30, 22}, {33, 22}, {30, 25}, {33, 25},
        {29, 22}, {34, 22}, {29, 25}, {34, 25},
        {30, 21}, {33, 21}, {30, 26}, {33, 26}}};
    f.clear(); // Actual same-buffer wide/narrow reuse; wide remains independent.
    for (std::size_t i = 0; i < narrow_points.size(); ++i)
        f.sample(i, narrow_points[i].x, narrow_points[i].y, 0x2233, 0x2233, 0xfffc);
    pf::FinalCounts final{};
    check(pf::count_narrow(f.borrow(), wide, final), "narrow edge samples succeed after reuse");
    for (std::size_t i = 0; i < narrow_points.size(); ++i)
        check(final[i] == pf::FinalCount{1, i < 4 ? 1u : 0u, 1, 0, i < 4 ? 1u : 4u},
              "narrow inclusive edges, adjacent exclusion and saved-wide fallback");

    constexpr std::array<Point, 12> visible_points{{
        {31, 23}, {32, 23}, {31, 24}, {32, 24},
        {30, 23}, {33, 23}, {30, 24}, {33, 24},
        {31, 22}, {32, 22}, {31, 25}, {32, 25}}};
    f.clear();
    for (std::size_t i = 0; i < visible_points.size(); ++i)
        f.sample(i, visible_points[i].x, visible_points[i].y, 0x5566, 0x5566, 0xfffc);
    check(pf::count_narrow(f.borrow(), wide, final), "visible center edge samples succeed");
    for (std::size_t i = 0; i < visible_points.size(); ++i)
        check(final[i] == pf::FinalCount{1, 1, 1, i < 4 ? 1u : 0u, i < 4 ? 1u : 4u},
              "visible center has independent inclusive two-by-two window");
}

void word_swap_and_subject_isolation() {
    Fixture f;
    // Manually pack the first A pair: canonical [FF FD FF FC] becomes
    // [FC FF FD FF]. Only the even-addressed halfword is nonclear.
    const auto first_a = Fixture::base + 2 * pf::plane_bytes;
    f.bytes[first_a] = 0xfc;
    f.bytes[first_a + 1] = 0xff;
    f.bytes[first_a + 2] = 0xfd;
    f.bytes[first_a + 3] = 0xff;
    // Last A plane's final pair: canonical [FF FC 24 69] -> [69 24 FC FF].
    const auto last_a_pair = Fixture::base + 14 * pf::plane_bytes - 4;
    f.bytes[last_a_pair] = 0x69;
    f.bytes[last_a_pair + 1] = 0x24;
    f.bytes[last_a_pair + 2] = 0xfc;
    f.bytes[last_a_pair + 3] = 0xff;
    f.word(1, 0, 0, 0xfffd);
    f.word(1, 63, 47, 0x2469);
    f.word(14, 0, 0, 0xfffd); // First subject occluded by its own before plane.
    // Last B remains clear. Color is unrelated and must not become scene Z.
    f.word(0, 63, 47, 0x1234);
    pf::WideCounts wide{};
    pf::FinalCounts final{};
    check(pf::count_wide(f.borrow(), wide), "pair-addressed wide succeeds");
    check(wide[0] == pf::WideCount{1, 0} && wide[11] == pf::WideCount{1, 0},
          "first and last A halfword pairs read in correct order");
    check(pf::count_narrow(f.borrow(), wide, final), "pair-addressed narrow succeeds");
    check(final[0] == pf::FinalCount{1, 0, 0, 0, 4}, "first subject uses B[0]");
    check(final[11] == pf::FinalCount{1, 0, 1, 0, 4}, "last subject uses B[11] and C, not color");
    for (std::size_t i = 1; i < 11; ++i)
        check(wide[i] == pf::WideCount{} && final[i] == pf::FinalCount{},
              "first/last subjects do not leak into intervening slots");
}

void invalid_extents_preserve_outputs() {
    Fixture f;
    auto view = f.borrow();
    // A successful existing borrow must survive every rejected replacement.
    check(!pf::PlanesView::borrow({}, 0, view), "empty extent rejected without read");
    check(!pf::PlanesView::borrow(std::span<const std::uint8_t>(f.bytes).first(
              Fixture::base + pf::buffer_bytes - 4), Fixture::base, view),
          "whole-word extent missing final word rejected before read");
    check(!pf::PlanesView::borrow(std::span<const std::uint8_t>(f.bytes).first(
              Fixture::base + pf::buffer_bytes - 1), Fixture::base, view),
          "partial-word extent rejected before read");
    check(!pf::PlanesView::borrow(f.bytes, Fixture::base + 2, view),
          "halfword-only physical alignment rejected");
    check(!pf::PlanesView::borrow(f.bytes, 0xfffffffcu, view), "physical address wrap rejected");
    check(!pf::PlanesView::borrow(f.bytes, static_cast<std::uint32_t>(f.bytes.size() + 4), view),
          "base beyond extent rejected without subtraction wrap");
    pf::WideCounts wide;
    wide.fill({123, 45});
    const auto saved_wide = wide;
    pf::FinalCounts final;
    final.fill({8, 7, 6, 5, 4});
    const auto saved_final = final;
    pf::PlanesView invalid;
    check(!pf::count_wide(invalid, wide) && wide == saved_wide,
          "invalid view leaves entire wide output unchanged");
    check(!pf::count_narrow(invalid, wide, final) && final == saved_final,
          "invalid view leaves entire narrow output unchanged");
    check(pf::count_wide(view, wide) && wide == pf::WideCounts{},
          "failed borrow leaves prior checked view intact");
    for (const pf::WideCount bad : {pf::WideCount{3073, 0}, pf::WideCount{3072, 661},
                                   pf::WideCount{1, 2}}) {
        wide = {};
        wide[11] = bad;
        check(!pf::count_narrow(view, wide, final) && final == saved_final,
              "all wide entries checked before any narrow output mutation");
    }
    // Exact endpoint is sufficient, and byte-only reading permits unaligned
    // host storage without pretending it is an aligned native halfword array.
    check(pf::PlanesView::borrow(std::span<const std::uint8_t>(f.bytes).first(
              Fixture::base + pf::buffer_bytes), Fixture::base, view),
          "exact complete extent accepted");
    std::vector<std::uint8_t> unaligned(pf::buffer_bytes + 1, 0);
    check(pf::PlanesView::borrow(std::span<const std::uint8_t>(unaligned).subspan(1), 0, view),
          "unaligned host byte span is valid word-swapped storage");
    check(pf::count_wide(view, wide), "unaligned host byte reduction succeeds");
    for (const auto& entry : wide)
        check(entry == pf::WideCount{3072, 660}, "zero packed depth is nonclear");
}
} // namespace

int main() {
    empty_and_dense_planes();
    packed_depth_and_coincident_occlusion();
    inclusive_windows();
    word_swap_and_subject_isolation();
    invalid_extents_preserve_outputs();
    if (failures != 0) {
        std::cerr << failures << " photo feedback checks failed\n";
        return 1;
    }
    std::cout << "photo feedback checks passed\n";
    return 0;
}
