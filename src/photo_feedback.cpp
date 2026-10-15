#include "35mmisle/photo_feedback.hpp"

namespace isle::photo_feedback {

bool PlanesView::borrow(std::span<const std::uint8_t> rdram,
                        std::uint32_t physical_base, PlanesView& output) noexcept {
    // Whole words ensure XOR 3 cannot escape the checked contiguous block.
    const auto base = static_cast<std::size_t>(physical_base);
    if ((physical_base % 4) != 0 || (rdram.size() % 4) != 0 ||
        static_cast<std::uint64_t>(physical_base) + buffer_bytes > (std::uint64_t{1} << 32) ||
        base > rdram.size() || buffer_bytes > rdram.size() - base) {
        return false;
    }
    output.rdram_ = rdram;
    output.base_ = base;
    return true;
}

std::uint16_t PlanesView::halfword(std::size_t plane, std::size_t pixel) const noexcept {
    const auto address = base_ + plane * plane_bytes + pixel * 2;
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(rdram_[address ^ std::size_t{3}]) << 8) |
        rdram_[(address + 1) ^ std::size_t{3}]);
}

bool count_wide(const PlanesView& planes, WideCounts& output) noexcept {
    if (planes.rdram_.empty()) return false;
    for (std::size_t subject = 0; subject < subject_count; ++subject) {
        WideCount count;
        for (std::size_t y = 0; y < height; ++y) {
            for (std::size_t x = 0; x < width; ++x) {
                if (planes.halfword(2 + subject, y * width + x) != clear_depth) {
                    ++count.total;
                    if (x >= 17 && x <= 46 && y >= 13 && y <= 34) ++count.center;
                }
            }
        }
        output[subject] = count;
    }
    return true;
}

bool count_narrow(const PlanesView& planes, const WideCounts& wide,
                  FinalCounts& output) noexcept {
    if (planes.rdram_.empty()) return false;
    for (const auto& count : wide) {
        if (count.total > width * height || count.center > 30 * 22 ||
            count.center > count.total) return false;
    }
    for (std::size_t subject = 0; subject < subject_count; ++subject) {
        FinalCount count;
        for (std::size_t y = 0; y < height; ++y) {
            for (std::size_t x = 0; x < width; ++x) {
                const auto pixel = y * width + x;
                const auto a = planes.halfword(2 + subject, pixel);
                if (a == clear_depth) continue;
                ++count.total;
                if (x >= 30 && x <= 33 && y >= 22 && y <= 25) ++count.center;
                const auto c = planes.halfword(1, pixel);
                if (a == c && planes.halfword(14 + subject, pixel) != c) {
                    ++count.visible;
                    if (x >= 31 && x <= 32 && y >= 23 && y <= 24) ++count.visible_center;
                }
            }
        }
        const auto& previous = wide[subject];
        // 3072*3072 fits uint32_t; approximate_total need not fit 3072.
        count.approximate_total = previous.center != 0
            ? previous.total * count.total / previous.center
            : previous.total * 4;
        output[subject] = count;
    }
    return true;
}

} // namespace isle::photo_feedback
