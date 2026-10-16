#ifndef ISLE_PHOTO_SCORE_HPP
#define ISLE_PHOTO_SCORE_HPP

#include "35mmisle/photo_feedback.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace isle::photo_score {

// Borrow the GAME app_render overlay in canonical big-endian order, beginning
// at guest 8009A8C0 (US ROM 46270). No executable or SDK layout is interpreted.
// The caller admits the source ROM and retains unmoved, immutable storage.
class RulesView {
public:
    static constexpr std::uint32_t guest_begin = 0x8009a8c0;
    static constexpr std::size_t minimum_bytes = 0x800ae7c4 - guest_begin;
    static constexpr std::size_t maximum_bytes = 0x5bf20 - 0x46270;
    static bool borrow(std::span<const std::byte> overlay, RulesView& output) noexcept;

private:
    std::span<const std::byte> bytes_{};
    bool range(std::uint32_t guest, std::size_t bytes) const noexcept;
    std::uint8_t u8(std::uint32_t guest) const noexcept;
    std::uint16_t u16(std::uint32_t guest) const noexcept;
    std::uint32_t u32(std::uint32_t guest) const noexcept;
    bool eligibility(std::int16_t pokemon, std::uint8_t animation, float& output) const noexcept;
    friend struct Evaluation;
};

struct Subject {
    std::int16_t pokemon_id = -1;
    std::uint8_t animation_index = 0; // GAME's six-bit animation selector.
    std::uint8_t pose_id = 0;
    std::uint8_t special_id = 0;
    float yaw = 0;
};

struct Photo {
    std::int8_t level_id = 0;
    std::uint8_t flags = 0;
    bool has_dash_engine = false;
    float eye_x = 0, eye_z = 0, at_x = 0, at_z = 0;
    std::array<Subject, photo_feedback::subject_count> subjects{};
};

struct SubjectMap {
    // The renderer's photo_PokemonIndexes mapping, not a reordered Photo.
    std::uint8_t photo_index = 0;
    std::uint8_t feedback_slot = 0;
};

struct Result {
    std::int32_t total = 0;
    std::uint16_t same_species_bonus = 0;
    std::uint8_t same_species_number = 0;
    bool well_framed = false;
    std::uint8_t pose_id = 0;
    std::int16_t pokemon_in_focus = 0;
    std::uint16_t proximity = 0, completeness = 0, pose_points = 0;
    std::int16_t special_bonus = 0;
    std::uint8_t special_id = 0;
    std::uint8_t photo_flags = 0; // Includes GAME's conditional clearing of 20h.
    std::uint8_t selected_photo_index = 0xff;
    bool operator==(const Result&) const = default;
};

// Pure CPU evaluation after genuine wide/narrow feedback has completed. Borrow
// the producer's FinalCounts directly; mapping contains active scored subjects
// in render order, including zero-visible subjects. Preserve sparse slot IDs,
// original Photo record order (including unscored records), and exact metadata
// from the same request. The selector reads the raw record at render rank,
// while calculation follows photo_index: preserve this GAME quirk.
// Framing uses counts[feedback_slot].center, not visible_center.
// No allocation, plane/table/count copies, GPU work or hidden/global state.
// Reject invalid views, out-of-bounds table operands, nonfinite used angles,
// duplicate/invalid mappings and impossible counter extents without changing
// output. All inputs must stay immutable; output must not alias them. Apply
// returned photo_flags at the owning state boundary after success.
// This is Oak's CPU scoring contract, not complete scene/photo reconstruction
// or an independent Original-machine floating-point parity claim.
bool evaluate(const RulesView& rules, const Photo& photo,
              const photo_feedback::FinalCounts& counts,
              std::span<const SubjectMap> mapping, Result& output) noexcept;

} // namespace isle::photo_score
#endif
