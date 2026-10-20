#include "campaign_photo_batch.hpp"
#include "application_photos.hpp"
#include "captured_photo_identity.hpp"
#include "native_course_probe.hpp"
#include "photo_check_source.h"
#include <algorithm>
#include <stdexcept>
#include <utility>

namespace isle::private_campaign {
namespace {
void require(bool value,const char* message) {
    if(!value)throw std::runtime_error(message);
}
}
void PhotoBatch::reset() noexcept {
    for(std::size_t i=0;i<count_;++i)items_[i]={};
    by_roll_.fill(-1);
    owner_=nullptr;count_=marked_=selected_=reported_=assessed_=retained_=0;
    prior_reports_={};epoch_=0;queue_verified_=false;
}
void PhotoBatch::freeze(const IsleCameraCheckSource& camera,std::span<const PlannedPhoto> plan,
                        const private_course::Probe* capture,std::uint64_t epoch) {
    reset();
    require(camera.common.status==ISLE_GAME_OK && camera.common.photos && epoch &&
        plan.size()<=items_.size(),"campaign batch requires a real Camera owner and course epoch");
    owner_=camera.common.photos;epoch_=epoch;
    std::array<bool,ISLE_PHOTO_REPORT_CAPACITY> reports{};
    for(const auto& planned:plan) {
        const auto& record=planned.record;
        require(record.owner==owner_ && record.photo && record.storage==ISLE_PHOTO_ROLL &&
            record.index>=0 && record.index<ISLE_PHOTO_ROLL_CAPACITY &&
            planned.report_index>=0 && planned.report_index<ISLE_PHOTO_REPORT_CAPACITY,
            "campaign plan contains a foreign captured roll or Report slot");
        const auto slot=static_cast<std::size_t>(record.index);
        const auto report=static_cast<std::size_t>(planned.report_index);
        require(by_roll_[slot]<0 && !reports[report],"campaign plan repeats a captured slot or Report species");
        std::array<std::byte,save::game_photo_bytes> bytes;
        const auto hash=private_course::photo_identity(*record.photo,bytes);
        require(!capture || capture->captured_photo(record.index,record.photo,hash),
            "campaign plan is not the same actual captured slot/address/wire digest after course detachment");
        auto& item=items_[count_];
        auto& session=reinterpret_cast<application::Photos*>(owner_)->save_session();
        const auto points=session.report_score(report);
        require(bool(points),"campaign prior canonical Report score is unavailable");
        item.prior_report_score=points.value;
        if(planned.disposition==PhotoDisposition::retain_existing_report) {
            if(!prior_reports_) {
                auto snapshot=session.snapshot();
                require(bool(snapshot),"campaign retained Report baseline snapshot is unavailable");
                prior_reports_=std::move(snapshot.value);
            }
            const auto present=prior_reports_.photo_present(save::PhotoSet::report,report);
            require(present && present.value && planned.species>0 &&
                planned.species<=151 && planned.score>0 && planned.score<=points.value,
                "campaign retained assessment requires a real non-improving ordinary Report");
        }
        item.planned=planned;item.captured_digest=hash;
        by_roll_[slot]=static_cast<std::int32_t>(count_);reports[report]=true;++count_;
    }
}
std::size_t PhotoBatch::identify(const IslePlayerPhotoData* photo,IslePhotoRecord& record) const {
    require(owner_ && photo && isle_application_photos_identify(owner_,photo,&record)==ISLE_GAME_OK &&
        record.owner==owner_ && record.photo==photo && record.storage==ISLE_PHOTO_ROLL &&
        record.index>=0 && record.index<ISLE_PHOTO_ROLL_CAPACITY,
        "campaign batch observation lost its canonical captured roll identity");
    const auto index=by_roll_[static_cast<std::size_t>(record.index)];
    require(index>=0 && static_cast<std::size_t>(index)<count_ &&
        items_[static_cast<std::size_t>(index)].planned.record.photo==photo,
        "Source selected or marked a photograph outside the frozen campaign batch");
    return static_cast<std::size_t>(index);
}
int PhotoBatch::observe_marks(const IsleCameraCheckSource& camera) {
    require(epoch_ && camera.common.photos==owner_,"campaign Camera observation crossed its roll epoch");
    int live=0;
    for(const auto& row:camera.D_camera_check_80249B30) {
        // Original87D1A0 uses separate ordinary/sign marks; 87FB20:736-754
        // merges both into the same real for-Oak selection queue.
        if(!row.unk_00 || (!row.unk_18_0x20000000 && !row.unk_18_0x10000000))continue;
        ++live;
        IslePhotoRecord record{};
        auto& item=items_[identify(row.unk_00,record)];
        require(row.unk_08==item.planned.species && row.unk_04==item.planned.score,
            "actual Camera marked species/score differs from its frozen Source plan");
        if(!item.marked) {
            std::array<std::byte,save::game_photo_bytes> bytes;
            require(private_course::photo_identity(*record.photo,bytes)==item.captured_digest,
                "actual Camera mark changed captured non-flag photo bytes");
            item.marked=true;item.marked_revision=record.revision;++marked_;
        }
    }
    return live;
}
void PhotoBatch::observe_selected(const IslePhotoCheckSource& photo) {
    require(epoch_ && photo.common.photos==owner_,"campaign Photo15 observation crossed its roll epoch");
    int count=0;
    require(isle_application_photos_selected_count(owner_,&count)==ISLE_GAME_OK &&
        count>=0 && static_cast<std::size_t>(count)==count_,
        "actual Photo15 queue count differs from the complete marked campaign batch");
    if(!queue_verified_) {
        std::array<bool,ISLE_PHOTO_ROLL_CAPACITY> seen{};
        for(int i=0;i<count;++i) {
            const IsleSelectedPhoto* selected=nullptr;
            require(isle_application_photos_selected(owner_,i,&selected)==ISLE_GAME_OK && selected && selected->for_oak,
                "actual campaign queue contains an absent or non-Oak selection");
            IslePhotoRecord record{};
            const auto index=identify(selected->photo,record);
            require(!seen[index],"actual Photo15 queue duplicates a campaign photograph");
            seen[index]=true;
        }
        queue_verified_=true;
    }
    // Original993F80:195-234 sorts scored rows independently of the canonical
    // selection queue. Match captured handles, never plan/queue/score row indices.
    for(std::size_t i=0;i<count_;++i) {
        const auto& scored=photo.D_802291A0_9D8C10[i];
        if(!scored.unk_0 || scored.pkmnID<=0)continue;
        // Source 98C330.c:1806-1810 clears the new Report-selection bit
        // only after A confirms its default old-image duplicate choice.
        if(!scored.unk_1A_14 && !scored.unk_1A_13) {
            IslePhotoRecord retained_record{};
            auto& retained_item=items_[identify(scored.unk_0,retained_record)];
            if(retained_item.selected && !retained_item.old_report_selected &&
                retained_item.planned.disposition==PhotoDisposition::retain_existing_report) {
                require(scored.pkmnID==retained_item.planned.species &&
                    scored.totalScore==static_cast<std::uint32_t>(retained_item.planned.score),
                    "actual retained decision changed the captured Source species/score");
                std::array<std::byte,save::game_photo_bytes> bytes;
                require(private_course::photo_identity(*retained_record.photo,bytes)==retained_item.captured_digest,
                    "actual retained decision changed captured non-flag photo bytes");
                require((photo.input.currentButtons & 0x8000u) && (photo.input.pressedButtons & 0x8000u),
                    "actual old-Report decision lacks Source-consumed A input");
                retained_item.old_report_confirmed_a=true;
                retained_item.old_report_selected=true;
            }
            continue;
        }
        IslePhotoRecord record{};
        auto& item=items_[identify(scored.unk_0,record)];
        if(item.selected)continue;
        require(item.marked && record.revision>=item.marked_revision &&
            scored.pkmnID==item.planned.species && item.planned.score>=0 &&
            scored.totalScore==static_cast<std::uint32_t>(item.planned.score),
            "actual Photo15 replay species/score differs from the same marked Source photograph");
        std::array<std::byte,save::game_photo_bytes> bytes;
        require(private_course::photo_identity(*record.photo,bytes)==item.captured_digest,
            "actual selected/scored photograph changed captured non-flag bytes");
        item.selected=true;item.selected_revision=record.revision;++selected_;
    }
}
void PhotoBatch::observe_reports() {
    if(assessed_==count_)return;
    for(auto& item:items()) {
        if(!item.selected || item.assessed)continue;
        IslePhotoRecord report{};
        int points=0;
        require(isle_application_photos_report(owner_,item.planned.species,&report)==ISLE_GAME_OK &&
            isle_application_photos_report_score(owner_,item.planned.species,&points)==ISLE_GAME_OK,
            "campaign Source Report publication could not be observed");
        if(item.planned.disposition==PhotoDisposition::retain_existing_report) {
            if(!item.old_report_selected)continue;
            const auto slot=static_cast<std::size_t>(item.planned.report_index);
            const auto baseline=prior_reports_.photo(save::PhotoSet::report,slot);
            const auto baseline_score=prior_reports_.report_score(slot);
            auto& session=reinterpret_cast<application::Photos*>(owner_)->save_session();
            const auto current=session.photo(save::PhotoSet::report,slot);
            require(baseline && current && baseline_score &&
                baseline.value.size()==save::game_photo_bytes &&
                current.value.size()==save::game_photo_bytes &&
                baseline_score.value==item.prior_report_score &&
                points==item.prior_report_score &&
                std::equal(baseline.value.begin(),baseline.value.end(),current.value.begin()),
                "actual retained assessment changed immutable prior Report bytes or score");
            require(report.photo && report.owner==owner_ && report.storage==ISLE_PHOTO_REPORT &&
                report.index==item.planned.report_index,
                "actual retained assessment lost its canonical prior Report slot");
            std::array<std::byte,save::game_photo_bytes> report_bytes,prior_bytes;
            const auto hash=private_course::photo_identity(*report.photo,report_bytes);
            std::copy(baseline.value.begin(),baseline.value.end(),prior_bytes.begin());
            prior_bytes[1]=std::byte{0};
            require(report_bytes==prior_bytes,
                "actual retained Report differs from the full prior normalized wire image");
            item.report_digest=hash;item.retained=true;item.assessed=true;
            ++retained_;++assessed_;
            continue;
        }
        if(!report.photo || points!=item.planned.score)continue;
        require(report.owner==owner_ && report.storage==ISLE_PHOTO_REPORT &&
            report.index==item.planned.report_index,"campaign Report published into a different canonical slot");
        std::array<std::byte,save::game_photo_bytes> report_bytes;
        const auto hash=private_course::photo_identity(*report.photo,report_bytes);
        if(hash!=item.captured_digest)continue; // An older Report may remain until real publication.
        std::array<std::byte,save::game_photo_bytes> captured_bytes;
        require(private_course::photo_identity(*item.planned.record.photo,captured_bytes)==item.captured_digest &&
            captured_bytes==report_bytes,"actual campaign Report differs from the full captured non-flag wire image");
        item.reported=true;item.assessed=true;item.report_digest=hash;++reported_;++assessed_;
    }
}
void PhotoBatch::validate_retained_save(const save::GameSnapshot& saved) const {
    for(const auto& item:items()) {
        if(item.planned.disposition!=PhotoDisposition::retain_existing_report)continue;
        require(item.selected && item.old_report_selected && item.old_report_confirmed_a &&
            item.retained && item.assessed && !item.reported && prior_reports_ && saved,
            "saved retained assessment lacks actual scored selection, consumed A or old-image disposition");
        const auto slot=static_cast<std::size_t>(item.planned.report_index);
        const auto baseline=prior_reports_.photo(save::PhotoSet::report,slot);
        const auto current=saved.photo(save::PhotoSet::report,slot);
        const auto present=saved.photo_present(save::PhotoSet::report,slot);
        const auto score=saved.report_score(slot);
        require(baseline && current && present && present.value && score &&
            baseline.value.size()==save::game_photo_bytes && current.value.size()==save::game_photo_bytes &&
            score.value==item.prior_report_score &&
            std::equal(baseline.value.begin(),baseline.value.end(),current.value.begin()),
            "acknowledged Save changed exact retained prior Report wire bytes or score");
    }
}
} // namespace isle::private_campaign
