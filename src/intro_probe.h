#ifndef ISLE_INTRO_PROBE_H
#define ISLE_INTRO_PROBE_H

#include <libultraship/libultra/gbi.h>
#include "host_clock.hpp"
#ifdef ISLE_HAS_CANONICAL_RDP
#include "required_work.hpp"
#endif

#include <cstdint>
#include <memory>

namespace isle::rom { class Snapshot; }

// One private-ROM sprite and selected authoritative C fade, not scene boot.
// Keep this owner alive through ProbeHost::Shutdown: display lists and texels
// are borrowed by LUS, independently of the C object's lifetime.
class IntroProbe final {
  public:
    explicit IntroProbe(const isle::rom::Snapshot& snapshot, std::uint32_t feedback_cycles);
    ~IntroProbe();
    IntroProbe(const IntroProbe&) = delete;
    IntroProbe& operator=(const IntroProbe&) = delete;

    struct Frame { isle::host::FrameToken token{}; Gfx* commands = nullptr; };
    void Start();
    bool TakeFrame(Frame&);
    isle::host::ClockError CompleteFrame(isle::host::FrameToken, isle::host::Terminal);
    void RequestStop();
    isle::host::ClockError RequestPause();
    isle::host::ClockError Resume();
    bool Paused() const;
    isle::host::ClockError SetIntervals(std::uint16_t update, std::uint16_t draw);
    std::uint32_t Ticks() const;
    isle::host::ClockStats Stats() const;
    bool FeedbackDiagnostic() const noexcept;
    std::uint32_t CompletedFeedbackCycles() const;
#ifdef ISLE_HAS_CANONICAL_RDP
    bool TakeRequired(isle::host::RequiredRequest&);
    isle::host::ClockError PublishRequired(isle::host::RequiredToken, isle::host::RequiredMemoryView);
    isle::host::ClockError FailRequired(isle::host::RequiredToken);
    bool TakeRequiredRetirement(isle::host::RequiredRetirement&);
    isle::host::ClockError FinishRequiredRetirement(isle::host::RequiredToken);
    isle::host::RequiredStats RequiredStats() const;
    bool RequiredQuiescent() const;
#endif
    void Check() const;
    void StopGame();

  private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

#endif
