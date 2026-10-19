#include "campaign_photo_selection.hpp"
#include "application_photos.hpp"
#include <algorithm>

namespace isle::private_campaign {
namespace {

// Immutable Original df8815e75f3d79618466e01e0827c6165b93d11c:
// src/camera_check/87FB20.c:305-311 (801E2E5C), 329-343 (801E2EF4).
// include/enums.h:38,342,368 define Dash=5, POKEDEX_MAX=151, smoke=1028.
constexpr unsigned dash_flag = 5;
constexpr std::int32_t ordinary_species_max = 151;

bool is_sign(std::int32_t species) noexcept {
    switch (species) {
    case 1004:
    case 1010:
    case 1018:
    case 1022:
    case 1028:
    case 1035:
        return true;
    default:
        return false;
    }
}

}

IsleGameResult SelectionPlan::build(const IsleCameraCheckSource& camera,
                                   bool allow_reassessment) noexcept {
    count = 0;
    if (camera.common.status != ISLE_GAME_OK) return camera.common.status;
    if (!camera.common.photos || camera.common.scene != ISLE_POST_COURSE_CAMERA_CHECK)
        return ISLE_GAME_INVALID_STATE;

    // native_source() is the canonical Photos owner's opaque ABI identity.
    // The nonsealing flag read neither snapshots nor mutates canonical Save.
    auto& owner = *reinterpret_cast<application::Photos*>(camera.common.photos);
    const auto rows = camera.D_camera_check_80208964;
    if (rows < 0 || rows > ISLE_PHOTO_ROLL_CAPACITY || rows != owner.count())
        return ISLE_GAME_INVALID_STATE;
    const auto dash = owner.save_session().player_flag(dash_flag);
    if (!dash) return ISLE_GAME_INVALID_STATE;

    // Canonical Report mapping, not species-1: the admitted domain is 69 and
    // its aliases/sign ordering already have one owner in the typed API.
    std::array<std::int32_t, ISLE_PHOTO_REPORT_CAPACITY> best{};
    best.fill(-1);
    std::size_t candidates = 0;
    for (std::int32_t row = 0; row < rows; ++row) {
        const auto& source = camera.D_camera_check_80249B30[static_cast<std::size_t>(row)];
        const std::int32_t species = source.unk_08;
        const bool ordinary = species > 0 && species <= ordinary_species_max;
        const bool sign = is_sign(species);

        // 87D1A0.c:456-474 checks 801E2EF4 BEFORE dispatching to the
        // ordinary/sign mark helpers. Sign marking (229-248) requires Dash
        // but has NO positive-score test. Positive ordinary scores here are
        // the campaign's useful-assessment policy, not an Original mark gate.
        if ((!ordinary && !(sign && dash.value)) || (ordinary && source.unk_04 <= 0))
            continue;

        std::int32_t report_index = -1;
        auto result = isle_application_photos_report_index(camera.common.photos, species, &report_index);
        if (result != ISLE_GAME_OK) return result;
        if (report_index < 0) continue;
        if (report_index >= ISLE_PHOTO_REPORT_CAPACITY) return ISLE_GAME_INVALID_STATE;

        IslePhotoRecord record{};
        result = isle_application_photos_identify(camera.common.photos, source.unk_00, &record);
        if (result != ISLE_GAME_OK) return result;
        if (!record.photo || record.owner != camera.common.photos || record.storage != ISLE_PHOTO_ROLL ||
            record.index < 0 || record.index >= rows)
            return ISLE_GAME_INVALID_STATE;

        auto& selected = best[static_cast<std::size_t>(report_index)];
        if (selected < 0) {
            selected = static_cast<std::int32_t>(candidates);
            items[candidates++] = {record, species, source.unk_04, report_index};
        } else {
            auto& current = items[static_cast<std::size_t>(selected)];
            if (source.unk_04 > current.score ||
                (source.unk_04 == current.score && record.index < current.record.index))
                current = {record, species, source.unk_04, report_index};
        }
    }

    // Original 87FB20.c:402-416 defines newness by canonical Report presence,
    // not the current UI dim/selection flags. 438-464 changes those flags with
    // the view, so they are deliberately not used as immutable eligibility.
    // The campaign submits existing ordinary species only for a strict score
    // improvement; existing signs do not provide a missing progression entry.
    std::size_t useful = 0;
    PlannedPhoto reassessment{};
    bool has_reassessment = false;
    for (std::size_t i = 0; i < candidates; ++i) {
        const auto& candidate = items[i];
        int present = 0;
        auto result = isle_application_photos_report_present(camera.common.photos, candidate.species, &present);
        if (result != ISLE_GAME_OK) return result;
        if (present) {
            if (is_sign(candidate.species)) continue;
            std::int32_t previous_score = 0;
            result = isle_application_photos_report_score(camera.common.photos, candidate.species, &previous_score);
            if (result != ISLE_GAME_OK) return result;
            if (candidate.score <= previous_score) {
                if (allow_reassessment && (!has_reassessment ||
                    candidate.record.index < reassessment.record.index)) {
                    reassessment = candidate;
                    reassessment.disposition = PhotoDisposition::retain_existing_report;
                    has_reassessment = true;
                }
                continue;
            }
        }
        if (useful != i) items[useful] = candidate;
        ++useful;
    }
    if (useful == 0 && has_reassessment) items[useful++] = reassessment;

    // Camera sorts/filters its own rows (87FB20.c:632-734). Retain only the
    // stable canonical handles, ordered by their original roll slots instead.
    std::sort(items.begin(), items.begin() + static_cast<std::ptrdiff_t>(useful),
        [](const PlannedPhoto& lhs, const PlannedPhoto& rhs) noexcept {
            return lhs.record.index < rhs.record.index;
        });
    count = useful;
    return ISLE_GAME_OK;
}

}
