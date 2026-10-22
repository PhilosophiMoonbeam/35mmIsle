#include "35mmisle/photo_score.hpp"
#include "35mmisle/game_math.h"

#include <bit>
#include <cfenv>
#include <cmath>

namespace isle::photo_score {
namespace {
constexpr std::uint32_t animation_sets = 0x800ad4b8;
constexpr std::uint32_t pokemon_indexes = 0x800ae4e4;
constexpr std::uint32_t special_bonuses = 0x800ae580;
constexpr std::uint32_t pose_bonuses = 0x800ae59c;
constexpr std::uint32_t angle_bonuses = 0x800ae6e4;
constexpr std::uint32_t angle_classes = 0x800ae744;
// Static observation of GAME score_CalculateScore: LWC1 constants 40490FDB /
// 40C90FDB and single-precision fold, MUL/DIV/DIV/TRUNC; no SDK inspection.
constexpr float pi = 0x1.921fb6p+1f;
constexpr float tau = 0x1.921fb6p+2f;
float add(float a, float b) noexcept { volatile float v = a + b; return v; }
float sub(float a, float b) noexcept { volatile float v = a - b; return v; }
float mul(float a, float b) noexcept { volatile float v = a * b; return v; }
float div(float a, float b) noexcept { volatile float v = a / b; return v; }
float interpolate(float pixels) noexcept {
    return add(div(mul(sub(1.0f, 0.2f), sub(pixels, 4.0f)), sub(768.0f, 4.0f)), 0.2f);
}
std::uint16_t tens(float value) noexcept {
    return static_cast<std::uint16_t>(div(add(value, 5.0f), 10.0f)) * 10;
}
bool egg(std::int16_t id) noexcept { return id >= 600 && id <= 602; }
bool sign(std::int16_t id) noexcept {
    return id == 1004 || id == 1010 || id == 1018 || id == 1022 || id == 1028 || id == 1035;
}
bool renderable(std::int16_t id) noexcept { return (id > 0 && id <= 151) || egg(id) || id == 603; }
float fraction(const photo_feedback::FinalCount& count) noexcept {
    const float value = div(static_cast<float>(count.visible), static_cast<float>(count.approximate_total));
    return value > 1.0f ? 1.0f : value;
}
}

bool RulesView::borrow(std::span<const std::byte> overlay, RulesView& output) noexcept {
    if (overlay.size() < minimum_bytes || overlay.size() > maximum_bytes) return false;
    output.bytes_ = overlay;
    return true;
}
bool RulesView::range(std::uint32_t guest, std::size_t bytes) const noexcept {
    return guest >= guest_begin && static_cast<std::size_t>(guest - guest_begin) <= bytes_.size() &&
           bytes <= bytes_.size() - static_cast<std::size_t>(guest - guest_begin);
}
std::uint8_t RulesView::u8(std::uint32_t guest) const noexcept {
    return std::to_integer<std::uint8_t>(bytes_[guest - guest_begin]);
}
std::uint16_t RulesView::u16(std::uint32_t guest) const noexcept {
    return static_cast<std::uint16_t>((static_cast<std::uint16_t>(u8(guest)) << 8) | u8(guest + 1));
}
std::uint32_t RulesView::u32(std::uint32_t guest) const noexcept {
    return (static_cast<std::uint32_t>(u16(guest)) << 16) | u16(guest + 2);
}
bool RulesView::eligibility(std::int16_t pokemon, std::uint8_t animation, float& output) const noexcept {
    for (std::uint32_t row = 0; row < 67; ++row) {
        const auto record = animation_sets + row * 12;
        if (u32(record) != static_cast<std::uint32_t>(pokemon)) continue;
        const auto pointer = u32(record + 4), count = u32(record + 8);
        if (animation >= 64 || animation >= count || count > 64 || (pointer & 3) != 0 ||
            !range(pointer, static_cast<std::size_t>(count) * 12)) return false;
        output = std::bit_cast<float>(u32(pointer + static_cast<std::uint32_t>(animation) * 12));
        return std::isfinite(output);
    }
    output = -1.0f;
    return true;
}

struct Evaluation {
    const RulesView& rules;
    const Photo& photo;
    const photo_feedback::FinalCounts& counts;
    std::span<const SubjectMap> mapping;
    Result result{};

    const photo_feedback::FinalCount& pixels(std::size_t rank) const noexcept {
        return counts[mapping[rank].feedback_slot];
    }
    bool eligible(const Subject& subject, float& value) const noexcept {
        return rules.eligibility(subject.pokemon_id, subject.animation_index, value);
    }
    void add_same(std::size_t index, std::int16_t focus) noexcept {
        const auto& entry = mapping[index];
        const auto id = photo.subjects[entry.photo_index].pokemon_id;
        if (focus == 90 || focus == 603) {
            if (id != 90 && id != 603) return;
        } else if (id != focus) return;
        const auto& count = counts[entry.feedback_slot];
        const auto visible = count.visible > 768 ? 768 : count.visible;
        if (visible < 4 || count.approximate_total == 0) return;
        const auto proximity = tens(mul(interpolate(static_cast<float>(visible)), 1000.0f));
        // Unlike the main score, bonus completeness is NOT quantized to 1/10000.
        const auto bonus = tens(mul(fraction(count), static_cast<float>(proximity)));
        result.same_species_bonus = static_cast<std::uint16_t>(result.same_species_bonus + bonus);
        result.total += bonus;
        ++result.same_species_number;
    }
    bool calculate(std::size_t index) noexcept {
        const auto& entry = mapping[index];
        const auto& subject = photo.subjects[entry.photo_index];
        const auto& count = counts[entry.feedback_slot];
        result.selected_photo_index = entry.photo_index;
        result.pokemon_in_focus = subject.pokemon_id == 603 ? 90 : subject.pokemon_id;
        float eligibility;
        if (!eligible(subject, eligibility)) return false;
        if (eligibility < 0) { result.pokemon_in_focus = 500; return true; }
        if (subject.special_id > 0) {
            if (subject.special_id >= 14) return false;
            result.special_id = subject.special_id;
            result.special_bonus = std::bit_cast<std::int16_t>(rules.u16(special_bonuses + subject.special_id * 2u));
            result.total += result.special_bonus;
        }
        const auto visible = count.visible > 768 ? 768 : count.visible;
        if (visible < 4) return true;
        const float proximity = interpolate(static_cast<float>(visible));
        result.proximity = tens(mul(proximity, 1000.0f));
        if (count.approximate_total == 0) return true;
        result.completeness = static_cast<std::uint16_t>(add(mul(fraction(count), 10000.0f), 0.5f));
        const float completeness = div(static_cast<float>(result.completeness), 10000.0f);
        result.total += tens(mul(completeness, static_cast<float>(result.proximity)));
        if (proximity < 0.245f || completeness < 0.6f) return true;
        result.pose_id = subject.pose_id;
        if (subject.pose_id > 0) {
            if (subject.pose_id >= 164) return false;
            result.pose_points = rules.u16(pose_bonuses + subject.pose_id * 2u);
        } else {
            if (!std::isfinite(subject.yaw) || !std::isfinite(photo.eye_x) || !std::isfinite(photo.eye_z) ||
                !std::isfinite(photo.at_x) || !std::isfinite(photo.at_z)) return false;
            const float x = sub(photo.eye_x, photo.at_x), z = sub(photo.eye_z, photo.at_z);
            float angle = sub(subject.yaw, isle_game_atan2f(x, z));
            if (!std::isfinite(angle)) return false;
            while (angle > pi) {
                const float next = sub(angle, tau);
                if (next == angle) return false; // No progress for unrepresentably large native input.
                angle = next;
            }
            while (angle < -pi) {
                const float next = add(angle, tau);
                if (next == angle) return false;
                angle = next;
            }
            if (angle < 0) angle = -angle;
            const auto id = subject.pokemon_id == 603 ? 90 : subject.pokemon_id;
            if (id <= 0 || id > 151) return false;
            const auto species = std::bit_cast<std::int8_t>(rules.u8(pokemon_indexes + static_cast<std::uint32_t>(id - 1)));
            if (species < 0 || species >= 64) return false;
            const auto category = rules.u16(angle_classes + static_cast<std::uint32_t>(species) * 2);
            const auto bin = static_cast<std::uint32_t>(div(div(mul(angle, 180.0f), pi), 15.0f));
            if (category >= 4 || bin > 12) return false;
            // Exactly pi produces column12. GAME spills into the next row,
            // including angle_classes[0] after row3; read the defined BE image
            // address, not an out-of-bounds native array or a clamped angle.
            result.pose_points = rules.u16(angle_bonuses + (category * 12u + bin) * 2u);
        }
        result.total += result.pose_points;
        if (result.pose_points < 200) return true;
        if (count.center == 0) return true;
        result.well_framed = true;
        result.total *= 2;
        for (std::size_t i = 0; i < mapping.size(); ++i)
            if (i != index) add_same(i, subject.pokemon_id);
        return true;
    }
    bool run() noexcept {
        if (!rules.range(RulesView::guest_begin, RulesView::minimum_bytes)) return false;
        result.photo_flags = photo.flags;
        std::int16_t preferred = -1;
        if (photo.level_id >= 0) {
            switch (photo.flags & 0xe0) {
                case 0x60: preferred = 1004; break;
                case 0x80: preferred = 1018; break;
                case 0xa0: preferred = 1022; break;
                case 0xe0: preferred = 1035; break;
                case 0x20: preferred = photo.subjects[0].pokemon_id; break;
                default: break;
            }
        }
        if (preferred > 0) {
            if (sign(preferred)) result.pokemon_in_focus = photo.has_dash_engine ? preferred : 500;
            else if (egg(preferred)) result.pokemon_in_focus = preferred;
            else {
                float value;
                if (!eligible(photo.subjects[0], value)) return false;
                if (value < 0) result.pokemon_in_focus = 500;
            }
            if (result.pokemon_in_focus > 0) return true;
        }
        if (preferred > 0 && !mapping.empty() && pixels(0).visible >= 12 &&
            pixels(0).approximate_total > 0 &&
            div(static_cast<float>(pixels(0).visible), static_cast<float>(pixels(0).approximate_total)) >= 0.05f)
            return calculate(0);
        std::array<int, 4> selected{-1, -1, -1, -1};
        std::array<std::uint32_t, 4> best{};
        for (std::size_t i = 0; i < mapping.size(); ++i) {
            const auto& count = pixels(i);
            if (count.visible < 12 || count.approximate_total == 0) continue;
            const auto ratio = div(static_cast<float>(count.visible), static_cast<float>(count.approximate_total));
            if (ratio < 0.2f) continue;
            // Deliberately raw i, not mapping[i].photo_index: original selector.
            const auto id = photo.subjects[i].pokemon_id;
            const bool weak = (id > 151 && id != 603) || ratio < 0.65f;
            const std::size_t bucket = count.center != 0 ? (weak ? 1u : 0u) : (weak ? 3u : 2u);
            if (count.visible > best[bucket]) {
                best[bucket] = count.visible;
                selected[bucket] = static_cast<int>(i);
            }
        }
        result.photo_flags = static_cast<std::uint8_t>(photo.flags & ~0x20u);
        for (const auto index : selected)
            if (index >= 0) return calculate(static_cast<std::size_t>(index));
        return true;
    }
};

bool evaluate(const RulesView& rules, const Photo& photo,
              const photo_feedback::FinalCounts& counts,
              std::span<const SubjectMap> mapping, Result& output) noexcept {
    if (std::fegetround() != FE_TONEAREST || mapping.size() > photo.subjects.size()) return false;
    Evaluation state{rules, photo, counts, mapping};
    std::size_t length = 0;
    while (length < photo.subjects.size() && photo.subjects[length].pokemon_id != -1) ++length;
    std::uint16_t used_photos = 0, used_slots = 0;
    for (const auto& entry : mapping) {
        if (entry.photo_index >= length || entry.feedback_slot >= counts.size() ||
            !renderable(photo.subjects[entry.photo_index].pokemon_id) ||
            (used_photos & (1u << entry.photo_index)) != 0 ||
            (used_slots & (1u << entry.feedback_slot)) != 0) return false;
        const auto& pixels = counts[entry.feedback_slot];
        if (pixels.visible > 3072 || pixels.center > 16 ||
            pixels.approximate_total > 3072u * 3072u) return false;
        used_photos = static_cast<std::uint16_t>(used_photos | (1u << entry.photo_index));
        used_slots = static_cast<std::uint16_t>(used_slots | (1u << entry.feedback_slot));
    }
    if (!state.run()) return false;
    output = state.result;
    return true;
}
} // namespace isle::photo_score
