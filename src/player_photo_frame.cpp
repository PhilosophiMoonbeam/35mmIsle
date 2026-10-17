#include "player_photo.hpp"

namespace isle::player {
IsleGameResult Photo::enqueue(IslePlayerPhotoTicket ticket,const scene::graphics::Frame& frame) noexcept {
    if(count_==2) return ISLE_GAME_EXHAUSTED;
    if(ticket.context>=2 || !ticket.submission) return ISLE_GAME_INVALID_STATE;
    if(frame.submission()!=ticket.submission || frame.submission_context()!=ticket.context ||
       frame.error()!=scene::graphics::Error::none || !frame.busy())
        return ISLE_GAME_INVALID_STATE;
    auto& pending=pending_[ticket.context];
    if(pending.phase!=Phase::prepared || pending.ticket.generation!=ticket.generation) return ISLE_GAME_INVALID_STATE;
    pending.ticket=ticket; pending.phase=Phase::queued;
    pending.frame=&frame;
    queue_[(head_+count_)%2]=ticket.context; ++count_; return ISLE_GAME_OK;
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
