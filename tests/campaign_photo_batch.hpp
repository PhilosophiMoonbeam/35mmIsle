#ifndef ISLE_PRIVATE_CAMPAIGN_PHOTO_BATCH_HPP
#define ISLE_PRIVATE_CAMPAIGN_PHOTO_BATCH_HPP

#include "campaign_photo_selection.hpp"
#include "35mmisle/game_save.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

struct IslePhotoCheckSource;
namespace isle::private_course { class Probe; }
namespace isle::private_campaign {
struct PhotoBatchItem {
    PlannedPhoto planned{};
    std::uint64_t captured_digest=0,marked_revision=0,selected_revision=0,report_digest=0;
    // Presentation evidence belongs to the copied Frame, not a Source row.
    std::uint64_t mark_frame_floor=0;
    bool marked=false,mark_presented=false,selected=false,reported=false;
    // Assessment is separate from publishing the new image: retained means
    // exact immutable pre-trip Report bytes and score survived the assessment.
    bool assessed=false,retained=false,old_report_selected=false;
    std::int32_t prior_report_score=0;
};
class PhotoBatch final {
public:
    PhotoBatch()=default;
    PhotoBatch(const PhotoBatch&)=delete;
    PhotoBatch& operator=(const PhotoBatch&)=delete;
    // Camera rows are not retained. The canonical roll owner/values must remain
    // in this course epoch until reset(), before the next real roll reset.
    void freeze(const IsleCameraCheckSource&,std::span<const PlannedPhoto>,
                const private_course::Probe*,std::uint64_t actual_course_epoch);
    int observe_marks(const IsleCameraCheckSource&);
    void observe_selected(const IslePhotoCheckSource&);
    void observe_reports();
    void reset() noexcept;
    std::span<PhotoBatchItem> items() noexcept {return {items_.data(),count_};}
    std::span<const PhotoBatchItem> items() const noexcept {return {items_.data(),count_};}
    std::size_t count() const noexcept {return count_;}
    std::size_t marked_count() const noexcept {return marked_;}
    std::size_t selected_count() const noexcept {return selected_;}
    // New captured-image publications only; retained reassessments never add one.
    std::size_t reported_count() const noexcept {return reported_;}
    // Completion includes either exact publication or a real old-image decision
    // followed by unchanged immutable prior Report bytes and score.
    std::size_t assessed_count() const noexcept {return assessed_;}
    std::size_t retained_count() const noexcept {return retained_;}
    const save::GameSnapshot& prior_report_snapshot() const noexcept {return prior_reports_;}
    std::uint64_t epoch() const noexcept {return epoch_;}
private:
    std::size_t identify(const IslePlayerPhotoData*,IslePhotoRecord&) const;
    std::array<PhotoBatchItem,ISLE_PHOTO_ROLL_CAPACITY> items_{};
    std::array<std::int32_t,ISLE_PHOTO_ROLL_CAPACITY> by_roll_{};
    IsleApplicationPhotos* owner_=nullptr;
    std::size_t count_=0,marked_=0,selected_=0,reported_=0,assessed_=0,retained_=0;
    save::GameSnapshot prior_reports_{};
    std::uint64_t epoch_=0;
    bool queue_verified_=false;
};
} // namespace isle::private_campaign
#endif
