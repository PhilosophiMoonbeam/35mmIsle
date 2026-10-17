#include "player_photo.hpp"

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
       pending.ticket!=IslePlayerPhotoTicket{ticket.context,ticket.generation,0})
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
}
extern "C" IsleGameResult isle_player_photo_func_8009C8E4(IslePlayerPhotoSource* source,uint16_t context,uint64_t generation) {
    if(!source || !source->owner) return ISLE_GAME_INVALID_ARGUMENT;
    return static_cast<isle::player::Photo*>(source->owner)->capture_prefix(context,generation);
}
