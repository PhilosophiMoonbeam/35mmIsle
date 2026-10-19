#ifndef ISLE_PRIVATE_CAMPAIGN_PHOTO_SELECTION_HPP
#define ISLE_PRIVATE_CAMPAIGN_PHOTO_SELECTION_HPP

#include "application_photos_source.h"
#include "camera_check_source.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace isle::private_campaign {

enum class PhotoDisposition {
    publish_new_report,
    retain_existing_report,
};

struct PlannedPhoto {
    // The canonical captured roll handle, not a Camera row or PhotoData copy.
    // record.index is the original shutter slot; revision is the build-time
    // value and can change when real Source marking updates canonical flags.
    IslePhotoRecord record{};
    std::int32_t species = 0;
    std::int32_t score = 0;
    std::int32_t report_index = -1;
    PhotoDisposition disposition = PhotoDisposition::publish_new_report;
};

struct SelectionPlan {
    std::array<PlannedPhoto, ISLE_PHOTO_ROLL_CAPACITY> items{};
    std::size_t count = 0;

    // Call after the actual Camera has scored its complete roll. Reads only:
    // no Source callbacks, marking, selection queue writes or Report writes.
    // An empty useful roll succeeds. Only an explicitly allowed reassessment
    // can replace it with one non-improving ordinary photo retaining its Report.
    // Failure leaves count zero; Photos must outlive the plan.
    [[nodiscard]] IsleGameResult build(const IsleCameraCheckSource&,
                                      bool allow_reassessment = false) noexcept;
    // Explicit private qualification purpose, not the useful-plan fallback:
    // select one real positive ordinary row with a saved Report and <= its
    // saved score, even when other useful rows exist. Reads every scored row,
    // not just the best photograph of each species. Empty succeeds.
    [[nodiscard]] IsleGameResult build_retained_assessment(const IsleCameraCheckSource&) noexcept;
    [[nodiscard]] std::span<const PlannedPhoto> photos() const noexcept {
        return {items.data(), count};
    }
};

}

#endif
