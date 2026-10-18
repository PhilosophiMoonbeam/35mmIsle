#ifndef ISLE_POST_COURSE_REPLAY_HPP
#define ISLE_POST_COURSE_REPLAY_HPP
#include "application_photos.hpp"
#include "game_audio.hpp"
#include "host_progress.hpp"
#include "photo_replay.hpp"
#include "post_course_replay_source.h"
#include <memory>

namespace isle::post_course {
/* Mandatory actual owner service, never a readiness assertion/no-op fallback.
 * Called after the shared Progress poll. Services genuine display GPU/readback
 * terminals, presentation/image release and its actual FrameToken if needed,
 * plus genuinely joined native collection. No Replay token stands in for a
 * display terminal. Must NOT pump Runtime, tick OM/GTL, invoke Source callbacks
 * or waits, manufacture retraces, delete the current Source owner, or perform a
 * control transfer under this returning native frame. Runs during cancellation
 * even after Host stop/deadline. A sticky fault is not benign pending. */
struct ReplayNativeProgress {
    void* owner = nullptr;
    IsleGameResult (*service)(void*) noexcept = nullptr;
};
/* One scene-owned stable record over the SAME Game, application Photos, Replay,
 * Host Progress and Source audio Session. All dependencies and callback backing
 * outlive this context; Replay belongs exclusively to it through retirement.
 * The owner must bind its genuine native service (unbound is invalid), reserve
 * Replay RAM before display begins, and route private Source photo mutations
 * through Photos/GameSession. Existing writable C roll pointers cannot be
 * memory-locked: no course Player may bind/mutate the roll during this lease.
 * Original C processes/callbacks must join before destruction. retire() is a
 * bounded returning leaf, reports actual retirement independently of sticky
 * execution status, and must reach retired=true before destruction. Partial
 * failure retains all borrowed callback/operation storage until that fence.
 * No alternate scheduler, gameplay controller, cached score or thumbnail. */
class Replay final {
public:
    Replay(IsleGame*, application::Photos&, photo::Replay&, host::Progress&,
           game_audio::Session&, ReplayNativeProgress);
    ~Replay() noexcept;
    Replay(const Replay&) = delete;
    Replay& operator=(const Replay&) = delete;
    IslePostCourseReplay* source() noexcept;
    IsleGameResult retire(bool& retired) noexcept;
private:
    struct Impl;
    std::unique_ptr<Impl> p_;
};
}
#endif
