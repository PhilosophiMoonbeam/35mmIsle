#include "player_photo_frame.hpp"
#include "menu_scene_bridge.h"
#include <exception>
#include <stdexcept>

namespace isle::player {
bool Photo::capture_rows_available() const noexcept {
    for(const auto& pending:pending_) {
        if(pending.phase==Phase::queued) return false;
        if(pending.phase==Phase::postprocessed &&
           (!pending.frame || !pending.frame->quiescent() || !pending.frame->completed()))
            return false;
    }
    return true;
}
IsleGameResult Photo::capture_prefix(std::uint16_t context,std::uint64_t generation) noexcept {
    auto result=source_access();
    if(result!=ISLE_GAME_OK) return result;
    if(context>=2 || detector_.context>=2 || context!=1-detector_.context ||
       !generation || pending_[context].phase!=Phase::idle ||
       !detector_.history.context_available(context) || !capture_rows_available())
        return ISLE_GAME_INVALID_STATE;
    const auto current=detector_.history.generation(context);
    if(current==UINT64_MAX || generation!=current+1) return ISLE_GAME_INVALID_ARGUMENT;
    auto& pending=pending_[context];
    pending.ticket={context,generation,0}; pending.phase=Phase::rejected;
    // Own pokemon_detect.c125-127 and 47380.c714-735: full original C capture
    // precedes history reset/baseline. No nonexistent history is leased here.
    result=isle_player_photo_capture_context(&source_,&source_.PokemonDetector_Photo[context]);
    if(result==ISLE_GAME_OK) pending.phase=Phase::captured;
    return remember(result);
}
IsleGameResult Photo::enqueue(IslePlayerPhotoTicket ticket,const scene::graphics::Frame& frame) noexcept {
    if(count_==2) return ISLE_GAME_EXHAUSTED;
    if(ticket.context>=2 || !ticket.submission) return ISLE_GAME_INVALID_STATE;
    if(frame.submission()!=ticket.submission ||
       frame.error()!=scene::graphics::Error::none || !frame.busy())
        return ISLE_GAME_INVALID_STATE;
    auto& pending=pending_[ticket.context];
    if(pending.phase!=Phase::prepared || !pending.history_retained ||
       pending.ticket!=IslePlayerPhotoTicket{ticket.context,ticket.generation,0} ||
       (pending.frame && pending.frame!=&frame))
        return ISLE_GAME_INVALID_STATE;
    pending.ticket=ticket; pending.phase=Phase::queued;
    pending.frame=&frame;
    queue_[(head_+count_)%2]=ticket.context; ++count_; return ISLE_GAME_OK;
}
IsleGameResult Photo::complete(IslePlayerPhotoTicket ticket) noexcept {
    auto* pending=matching(ticket);
    if(!pending || !pending->frame || !pending->frame->quiescent() ||
       !pending->frame->completed()) return ISLE_GAME_INVALID_STATE;
    if(pending->phase!=Phase::postprocessed || !count_ || queue_[head_]!=ticket.context)
        return ISLE_GAME_INVALID_STATE;
    pending->phase=Phase::complete; completed_=ticket.context;
    head_=(head_+1)%2; --count_; return ISLE_GAME_OK;
}
IsleGameResult Photo::retire_failed(IslePlayerPhotoTicket ticket,const scene::graphics::Frame& frame) noexcept {
    auto* pending=matching(ticket);
    if(!pending || pending->borrows || pending->frame!=&frame ||
       (pending->phase!=Phase::queued && pending->phase!=Phase::postprocessed) ||
       !frame.failed_submission(ticket.submission)) return ISLE_GAME_INVALID_STATE;
    std::size_t position=0;
    while(position<count_ && queue_[(head_+position)%2]!=ticket.context) ++position;
    if(position==count_) return ISLE_GAME_INVALID_STATE;
    const auto result=release_pending(*pending);
    if(result!=ISLE_GAME_OK) return result;
    for(std::size_t i=position;i+1<count_;++i)
        queue_[(head_+i)%2]=queue_[(head_+i+1)%2];
    --count_;
    return ISLE_GAME_OK;
}

Controller::Controller(Photo& photo,actor::DetectorBinding& detector)
    :photo_(photo),detector_(detector) {
    if(&photo.detector_!=&detector || photo.source_access()!=ISLE_GAME_OK)
        throw std::invalid_argument("Photo Controller requires its canonical live Detector");
}
Controller::~Controller() {
    if(preparing_ && end_preparation()!=ISLE_GAME_OK) std::terminate();
}
Photo::Pending* Controller::pending(const scene::graphics::Frame& frame) const noexcept {
    for(auto& entry:photo_.pending_)
        if(entry.phase!=Photo::Phase::idle && entry.frame==&frame) return &entry;
    return nullptr;
}
IsleGameResult Controller::begin_preparation(const scene::graphics::Frame& frame) noexcept {
    auto result=photo_.source_access();
    if(result!=ISLE_GAME_OK) return result;
    if(preparing_ || !frame.quiescent() || frame.error()!=scene::graphics::Error::none ||
       pending(frame) || !photo_.services_ || !photo_.snapshot_owner_ ||
       isle_menu_current_object(photo_.player_.game) ||
       !detector_.prefix_user || !detector_.prepare_membership ||
       detector_.prepare_membership==prefix)
        return ISLE_GAME_INVALID_STATE;
    preparing_=&frame;
    collector_user_=detector_.prefix_user; collector_=detector_.prepare_membership;
    detector_.prefix_user=this; detector_.prepare_membership=prefix;
    return ISLE_GAME_OK;
}
IsleGameResult Controller::end_preparation() noexcept {
    const auto access=photo_.source_access(true);
    if(access!=ISLE_GAME_OK) return access;
    if(!preparing_) return ISLE_GAME_INVALID_STATE;
    // Never detach a foreign/replaced hook or resurrect a failed PhotoScene.
    if(detector_.prefix_user!=this || detector_.prepare_membership!=prefix)
        return ISLE_GAME_INVALID_STATE;
    detector_.prefix_user=collector_user_; detector_.prepare_membership=collector_;
    preparing_=nullptr; collector_user_=nullptr; collector_=nullptr;
    return ISLE_GAME_OK;
}
IsleGameResult Controller::prefix(void* user,focus::Photo& output) noexcept {
    if(!user) return ISLE_GAME_INVALID_ARGUMENT;
    auto& self=*static_cast<Controller*>(user);
    if(!self.preparing_ || self.preparing_->submission() || !self.collector_ ||
       self.detector_.prefix_user!=&self || self.detector_.prepare_membership!=prefix ||
       self.detector_.context>=2)
        return ISLE_GAME_INVALID_STATE;
    const auto context=static_cast<std::uint16_t>(1-self.detector_.context);
    const auto before=self.detector_.history.generation(context);
    if(before==UINT64_MAX || self.photo_.pending_[context].phase!=Photo::Phase::idle)
        return ISLE_GAME_INVALID_STATE;
    auto result=self.photo_.capture_prefix(context,before+1);
    auto& entry=self.photo_.pending_[context];
    if(entry.phase!=Photo::Phase::idle &&
       entry.ticket==IslePlayerPhotoTicket{context,before+1,0} && !entry.frame)
        entry.frame=self.preparing_;
    if(result!=ISLE_GAME_OK) return result;
    // Original full C camera/Pokemon/Item/FX capture and semantic identity holds
    // precede the real collector and every history baseline/stage/commit.
    // PhotoScene checks its exact original userdata/hook while collecting.
    self.detector_.prefix_user=self.collector_user_;
    self.detector_.prepare_membership=self.collector_;
    result=self.collector_(self.collector_user_,output);
    if(self.detector_.prefix_user==self.collector_user_ &&
       self.detector_.prepare_membership==self.collector_) {
        self.detector_.prefix_user=&self; self.detector_.prepare_membership=prefix;
    } else {
        // The actual collector may revoke its own hook on failure. Preserve
        // that revocation; an outer failure must not restore a dead owner.
        self.preparing_=nullptr; self.collector_user_=nullptr; self.collector_=nullptr;
        if(result==ISLE_GAME_OK) result=ISLE_GAME_INVALID_STATE;
    }
    return result;
}
IsleGameResult Controller::request() noexcept {
    auto result=photo_.source_access();
    if(result!=ISLE_GAME_OK) return result;
    if(!preparing_ || preparing_->submission() ||
       detector_.prefix_user!=this || detector_.prepare_membership!=prefix)
        return ISLE_GAME_INVALID_STATE;
    auto* entry=pending(*preparing_);
    if(!entry || entry->phase!=Photo::Phase::captured || entry->ticket.submission)
        return ISLE_GAME_INVALID_STATE;
    return photo_.seal_history(entry->ticket.context,entry->ticket.generation);
}
IslePlayerPhotoTicket Controller::ticket(const scene::graphics::Frame& frame) const noexcept {
    if(photo_.source_access(true)!=ISLE_GAME_OK) return {};
    const auto* entry=pending(frame);
    return entry?entry->ticket:IslePlayerPhotoTicket{};
}
IsleGameResult Controller::submitted(const scene::graphics::Frame& frame,IslePlayerPhotoTicket& output) noexcept {
    output={};
    const auto access=photo_.source_access();
    if(access!=ISLE_GAME_OK) return access;
    if(preparing_) return ISLE_GAME_INVALID_STATE;
    const auto* entry=pending(frame);
    if(!entry || entry->phase!=Photo::Phase::prepared) return ISLE_GAME_INVALID_STATE;
    const IslePlayerPhotoTicket executing{entry->ticket.context,entry->ticket.generation,frame.submission()};
    const auto result=photo_.enqueue(executing,frame);
    if(result==ISLE_GAME_OK) output=executing;
    return result;
}
IsleGameResult Controller::completed_memory(const scene::graphics::Frame& frame,
    rsp::CanonicalExecutor::MemoryLease& lease,std::uint32_t framebuffer) {
    const auto access=photo_.source_access(true);
    if(access!=ISLE_GAME_OK) return access;
    const auto* entry=pending(frame);
    if(!entry || entry->phase!=Photo::Phase::queued ||
       !entry->ticket.submission || frame.submission()!=entry->ticket.submission ||
       frame.error()!=scene::graphics::Error::none || !frame.busy() || !lease ||
       lease.postprocess_active() || !photo_.count_ ||
       photo_.queue_[photo_.head_]!=entry->ticket.context)
        return ISLE_GAME_INVALID_STATE;
    auto words=lease.postprocess_words();
    const auto result=photo_.postprocess(entry->ticket,words,detector_.copies.region_offset(0),framebuffer);
    lease.finish_postprocess();
    return result;
}
IsleGameResult Controller::completed(const scene::graphics::Frame& frame) noexcept {
    const auto access=photo_.source_access(true);
    if(access!=ISLE_GAME_OK) return access;
    const auto* entry=pending(frame);
    return entry?photo_.complete(entry->ticket):ISLE_GAME_INVALID_STATE;
}
IsleGameResult Controller::retire(const scene::graphics::Frame& frame) noexcept {
    const auto access=photo_.source_access(true);
    if(access!=ISLE_GAME_OK) return access;
    const auto* entry=pending(frame);
    if(!entry || preparing_==&frame || !frame.quiescent() ||
       (!entry->ticket.submission && frame.submission()))
        return ISLE_GAME_INVALID_STATE;
    return photo_.retire(entry->ticket);
}
IsleGameResult Controller::retire_unsubmitted(const scene::graphics::Frame& frame) noexcept {
    const auto access=photo_.source_access(true);
    if(access!=ISLE_GAME_OK) return access;
    const auto* entry=pending(frame);
    if(!entry || preparing_==&frame || !frame.quiescent() ||
       entry->ticket.submission || frame.submission() ||
       (entry->phase!=Photo::Phase::rejected && entry->phase!=Photo::Phase::captured &&
        entry->phase!=Photo::Phase::prepared))
        return ISLE_GAME_INVALID_STATE;
    return photo_.retire(entry->ticket);
}
IsleGameResult Controller::retire_failed(const scene::graphics::Frame& frame) noexcept {
    const auto access=photo_.source_access(true);
    if(access!=ISLE_GAME_OK) return access;
    const auto* entry=pending(frame);
    return entry?photo_.retire_failed(entry->ticket,frame):ISLE_GAME_INVALID_STATE;
}
}
extern "C" IsleGameResult isle_player_photo_func_8009C8E4(IslePlayerPhotoSource* source,uint16_t context,uint64_t generation) {
    if(!source || !source->owner) return ISLE_GAME_INVALID_ARGUMENT;
    return static_cast<isle::player::Photo*>(source->owner)->capture_prefix(context,generation);
}
