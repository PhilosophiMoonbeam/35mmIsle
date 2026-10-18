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
    // Quiescence concerns actual capabilities, not unbound Source FIFO IDs.
    bool quiescent() const noexcept {return !preparing_&&photo_.quiescent();}
    // Bracket synchronous Frame::begin (or an unsubmitted producer preparation).
    // Restores the real collector before any native OM callback can run.
    IsleGameResult begin_preparation(const scene::graphics::Frame&) noexcept;
    IsleGameResult end_preparation() noexcept;
    // Original nonblocking ProcessImage: enqueue CURRENT u16 ID, including
    // repeated/no-prefix contexts. EXHAUSTED means no message was enqueued.
    IsleGameResult request() noexcept;
    std::size_t queued_requests() const noexcept {return photo_.count_;}
    // Real preparation/submission/consumer binding; generation0 is not absence.
    bool has_pending(const scene::graphics::Frame&) const noexcept;
    // Read the actual preparation/GPU/consumer capability, never a FIFO record.
    // A callback can select a different context by consuming its head ID.
    IslePlayerPhotoTicket ticket(const scene::graphics::Frame&) const noexcept;
    // Immediately after successful Frame::begin, before any poll. A no-prefix
    // current-context GPU lease is admitted only after its real task exists;
    // FIFO head metadata remains separately resolved at the eligible callback.
    IsleGameResult submitted(const scene::graphics::Frame&,IslePlayerPhotoTicket&) noexcept;
    // Call from that Frame's actual completed-memory callback. registered=false
    // also represents Source GFX_END with no matching presentation task. Both
    // settle successful GPU holds later, without consuming or analyzing FIFO.
    // An eligible callback acquires/closes the exclusive write mapping.
    IsleGameResult completed_memory(const scene::graphics::Frame&,
        rsp::CanonicalExecutor::MemoryLease&,std::uint32_t framebuffer,bool registered=true);
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
