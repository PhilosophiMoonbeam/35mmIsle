#include "player_photo_frame.hpp"
#include "menu_scene_bridge.h"
#include <exception>
#include <stdexcept>

namespace isle::player {
bool Photo::capture_rows_available() const noexcept {
    for(const auto& pending:pending_) {
        if(pending.phase==Phase::submitted) return false;
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
IsleGameResult Photo::bind_submission(IslePlayerPhotoTicket ticket,const scene::graphics::Frame& frame) noexcept {
    if(ticket.context>=2 || !ticket.submission || frame.submission()!=ticket.submission ||
       frame.error()!=scene::graphics::Error::none || !frame.busy()) return ISLE_GAME_INVALID_STATE;
    auto& pending=pending_[ticket.context];
    if(pending.phase!=Phase::prepared || !pending.history_retained ||
       pending.ticket!=IslePlayerPhotoTicket{ticket.context,ticket.generation,0} ||
       pending.frame!=&frame) return ISLE_GAME_INVALID_STATE;
    pending.ticket=ticket;pending.phase=Phase::submitted;
    return ISLE_GAME_OK;
}
IsleGameResult Photo::complete(IslePlayerPhotoTicket ticket) noexcept {
    auto* pending=matching(ticket);
    if(!pending || !pending->frame || !pending->frame->quiescent() ||
       !pending->frame->completed() || pending->phase!=Phase::postprocessed)
        return ISLE_GAME_INVALID_STATE;
    pending->phase=Phase::complete;completed_=ticket.context;
    return ISLE_GAME_OK;
}
IsleGameResult Photo::retire_failed(IslePlayerPhotoTicket ticket,const scene::graphics::Frame& frame) noexcept {
    auto* pending=matching(ticket);
    if(!pending || pending->borrows || pending->frame!=&frame ||
       (pending->phase!=Phase::submitted && pending->phase!=Phase::postprocessed) ||
       !frame.failed_submission(ticket.submission)) return ISLE_GAME_INVALID_STATE;
    // A failed GPU is not a Source callback. Unconsumed FIFO IDs survive.
    return release_pending(*pending);
}
IsleGameResult Photo::bind_existing(std::uint16_t context,const scene::graphics::Frame& frame) noexcept {
    if(context>=pending_.size()) return ISLE_GAME_INVALID_STATE;
    auto& entry=pending_[context];
    if(entry.phase!=Phase::idle) {
        if(entry.frame!=&frame) return ISLE_GAME_INVALID_STATE;
        if(entry.phase==Phase::captured) return seal_history(context,entry.ticket.generation);
        return entry.phase==Phase::prepared||entry.phase==Phase::submitted?ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
    }
    const auto generation=detector_.history.generation(context);
    const auto* members=detector_.history.photo(context);
    const auto& captured=source_.PokemonDetector_Photo[context];
    if(!members || !detector_.history.context_available(context)) return ISLE_GAME_INVALID_STATE;
    for(std::size_t i=0;i<members->objects.size();++i)
        if(members->objects[i]!=reinterpret_cast<focus::Object>(captured.pokemonObjects[i]))
            return ISLE_GAME_INVALID_STATE;
    entry.ticket={context,generation,0};entry.phase=Phase::rejected;entry.frame=&frame;
    for(auto* object:captured.pokemonObjects) {
        // Source snapshot members are compared as identities, never dereferenced.
        // Real Actor cleanup clears Detector subjects; a retired/collected
        // snapshot member must not resurrect its old semantic/native owner.
        if(!object || !services_->source_owner_registered(object) ||
           services_->source_owner_joined(object)) continue;
        const auto result=hold(entry,object);if(result!=ISLE_GAME_OK) return result;
    }
    for(const auto& subject:detector_.history.history(context)) {
        const auto result=hold(entry,reinterpret_cast<IsleGameObject*>(subject.object));
        if(result!=ISLE_GAME_OK) return result;
    }
    if(detector_.history.retain_context(context,generation)!=focus::Error::none) return ISLE_GAME_INVALID_STATE;
    entry.history_retained=true;entry.phase=Phase::prepared;
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
    Photo::Pending* found=nullptr;
    for(auto& entry:photo_.pending_) if(entry.phase!=Photo::Phase::idle && entry.frame==&frame) {
        if(entry.phase==Photo::Phase::postprocessed || entry.phase==Photo::Phase::complete) return &entry;
        found=&entry;
    }
    return found;
}
bool Controller::has_pending(const scene::graphics::Frame& frame) const noexcept {
    return photo_.source_access(true)==ISLE_GAME_OK && pending(frame);
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
    const auto access=photo_.source_access();
    if(access!=ISLE_GAME_OK) return access;
    if(!preparing_ || preparing_->submission() || detector_.context>=2 ||
       detector_.prefix_user!=this || detector_.prepare_membership!=prefix) return ISLE_GAME_INVALID_STATE;
    // Only a real fresh capture needs a GPU/history lease before submission.
    // An ID-only/no-prefix request is unbound until an eligible callback.
    for(auto& entry:photo_.pending_) if(entry.frame==preparing_) {
        if(entry.phase==Photo::Phase::captured) {
            const auto result=photo_.seal_history(entry.ticket.context,entry.ticket.generation);
            if(result!=ISLE_GAME_OK) return result;
        } else if(entry.phase!=Photo::Phase::prepared) return ISLE_GAME_INVALID_STATE;
    }
    if(photo_.count_==photo_.queue_.size()) return ISLE_GAME_EXHAUSTED;
    photo_.queue_[(photo_.head_+photo_.count_)%photo_.queue_.size()]=detector_.context;
    ++photo_.count_;
    return ISLE_GAME_OK;
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
    if(preparing_ || !frame.submission() || !frame.busy() ||
       frame.error()!=scene::graphics::Error::none) return ISLE_GAME_INVALID_STATE;
    if(!pending(frame)) {
        if(!photo_.count_) return ISLE_GAME_INVALID_STATE;
        // Bind current metadata only to this actual GPU task, not to a FIFO ID
        // or the generation that happened to exist when that ID was enqueued.
        const auto result=photo_.bind_existing(detector_.context,frame);
        if(result!=ISLE_GAME_OK) return result;
    }
    for(auto& entry:photo_.pending_) if(entry.frame==&frame && entry.phase==Photo::Phase::prepared) {
        const IslePlayerPhotoTicket executing{entry.ticket.context,entry.ticket.generation,frame.submission()};
        const auto result=photo_.bind_submission(executing,frame);
        if(result!=ISLE_GAME_OK) return result;
    }
    output=ticket(frame);
    return output.submission?ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
}
IsleGameResult Controller::completed_memory(const scene::graphics::Frame& frame,
    rsp::CanonicalExecutor::MemoryLease& lease,std::uint32_t framebuffer,bool registered) {
    const auto access=photo_.source_access(true);
    if(access!=ISLE_GAME_OK) return access;
    if(!frame.submission() || frame.error()!=scene::graphics::Error::none || !frame.busy() ||
       !lease || lease.postprocess_active()) return ISLE_GAME_INVALID_STATE;
    for(const auto& entry:photo_.pending_)
        if(entry.frame==&frame && entry.phase==Photo::Phase::postprocessed) return ISLE_GAME_INVALID_STATE;
    if(!registered || !photo_.count_) return ISLE_GAME_OK;
    // Receive the ID at the actual eligible callback, not at poll completion.
    // Resolve its latest CPU metadata against THIS completed shared Region RAM.
    const auto context=photo_.queue_[photo_.head_];
    photo_.head_=(photo_.head_+1)%photo_.queue_.size();--photo_.count_;
    const auto bound=photo_.bind_existing(context,frame);
    if(bound!=ISLE_GAME_OK) return bound;
    auto& entry=photo_.pending_[context];
    if(entry.phase==Photo::Phase::prepared) {
        const auto result=photo_.bind_submission({context,entry.ticket.generation,frame.submission()},frame);
        if(result!=ISLE_GAME_OK) return result;
    }
    auto words=lease.postprocess_words();
    const auto result=photo_.postprocess(entry.ticket,words,detector_.copies.region_offset(0),framebuffer);
    lease.finish_postprocess();
    return result;
}
IsleGameResult Controller::completed(const scene::graphics::Frame& frame) noexcept {
    const auto access=photo_.source_access(true);
    if(access!=ISLE_GAME_OK) return access;
    if(!frame.quiescent() || !frame.completed() || preparing_==&frame) return ISLE_GAME_INVALID_STATE;
    for(auto& entry:photo_.pending_) if(entry.frame==&frame) {
        IsleGameResult result=ISLE_GAME_INVALID_STATE;
        if(entry.phase==Photo::Phase::postprocessed) result=photo_.complete(entry.ticket);
        else if(entry.phase==Photo::Phase::complete) result=ISLE_GAME_OK;
        else if(entry.phase==Photo::Phase::submitted) result=photo_.release_pending(entry);
        if(result!=ISLE_GAME_OK) return result;
    }
    return ISLE_GAME_OK;
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
    if(!pending(frame) || preparing_==&frame || !frame.quiescent() || frame.submission())
        return ISLE_GAME_INVALID_STATE;
    for(auto& entry:photo_.pending_) if(entry.frame==&frame) {
        if(entry.ticket.submission || (entry.phase!=Photo::Phase::rejected &&
           entry.phase!=Photo::Phase::captured && entry.phase!=Photo::Phase::prepared))
            return ISLE_GAME_INVALID_STATE;
        const auto result=photo_.retire(entry.ticket);if(result!=ISLE_GAME_OK) return result;
    }
    return ISLE_GAME_OK;
}
IsleGameResult Controller::retire_failed(const scene::graphics::Frame& frame) noexcept {
    const auto access=photo_.source_access(true);
    if(access!=ISLE_GAME_OK) return access;
    if(!pending(frame)) return ISLE_GAME_INVALID_STATE;
    for(auto& entry:photo_.pending_) if(entry.frame==&frame) {
        if(!entry.ticket.submission) {
            if(!frame.quiescent() || frame.error()==scene::graphics::Error::none) return ISLE_GAME_INVALID_STATE;
            const auto result=photo_.retire(entry.ticket);if(result!=ISLE_GAME_OK) return result;
        } else {
            const auto result=photo_.retire_failed(entry.ticket,frame);if(result!=ISLE_GAME_OK) return result;
        }
    }
    return ISLE_GAME_OK;
}
} // namespace isle::player
extern "C" IsleGameResult isle_player_photo_func_8009C8E4(IslePlayerPhotoSource* source,uint16_t context,uint64_t generation) {
    if(!source || !source->owner) return ISLE_GAME_INVALID_ARGUMENT;
    return static_cast<isle::player::Photo*>(source->owner)->capture_prefix(context,generation);
}
