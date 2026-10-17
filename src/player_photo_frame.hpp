#ifndef ISLE_PLAYER_PHOTO_FRAME_HPP
#define ISLE_PLAYER_PHOTO_FRAME_HPP
#include "player_photo.hpp"

namespace isle::player {
// Nonowning production bridge over Photo's sole pending-ticket registry. The
// actual PhotoScene/Detector/Frames outlive every outstanding ticket. No native
// update, owner retirement or Frame reuse may cross GPU/readback ownership.
class Controller final {
public:
    Controller(Photo&,actor::DetectorBinding&);
    ~Controller();
    Controller(const Controller&)=delete;
    Controller& operator=(const Controller&)=delete;
    // The sole pending registry, including tickets bound to other real Frames.
    bool quiescent() const noexcept {return !preparing_&&photo_.quiescent();}
    // Bracket synchronous Frame::begin (or an unsubmitted producer preparation).
    // Restores the real collector before any native OM callback can run.
    IsleGameResult begin_preparation(const scene::graphics::Frame&) noexcept;
    IsleGameResult end_preparation() noexcept;
    // Original ProcessImage boundary: after exact producer history commit,
    // while preparation is still active and BEFORE an actual submission token.
    IsleGameResult request() noexcept;
    // Read the existing Photo capability; never creates or changes a token.
    IslePlayerPhotoTicket ticket(const scene::graphics::Frame&) const noexcept;
    // Immediately after successful Frame::begin, before any poll. Detector
    // context is independent of this Frame's physical submission context.
    IsleGameResult submitted(const scene::graphics::Frame&,IslePlayerPhotoTicket&) noexcept;
    // Call from that Frame's actual completed-memory callback. Acquires and
    // closes the exclusive write mapping; writeback exceptions escape to Frame.
    IsleGameResult completed_memory(const scene::graphics::Frame&,
        rsp::CanonicalExecutor::MemoryLease&,std::uint32_t framebuffer);
    // Only actual completed poll/readback evidence permits Photo completion.
    // Publication remains the later original native OM CopyInfo operation.
    IsleGameResult completed(const scene::graphics::Frame&) noexcept;
    IsleGameResult retire_unsubmitted(const scene::graphics::Frame&) noexcept;
    IsleGameResult retire(const scene::graphics::Frame&) noexcept;
    IsleGameResult retire_failed(const scene::graphics::Frame&) noexcept;
private:
    static IsleGameResult prefix(void*,focus::Photo&) noexcept;
    Photo::Pending* pending(const scene::graphics::Frame&) const noexcept;
    Photo& photo_;
    actor::DetectorBinding& detector_;
    const scene::graphics::Frame* preparing_=nullptr;
    void* collector_user_=nullptr;
    IsleGameResult (*collector_)(void*,focus::Photo&) noexcept=nullptr;
};
}
#endif
