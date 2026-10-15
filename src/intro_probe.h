#ifndef ISLE_INTRO_PROBE_H
#define ISLE_INTRO_PROBE_H

#include <libultraship/libultra/gbi.h>

#include <cstdint>
#include <memory>

namespace isle::rom { class Snapshot; }

// One private-ROM sprite and selected authoritative C fade, not scene boot.
// Keep this owner alive through ProbeHost::Shutdown: display lists and texels
// are borrowed by LUS, independently of the C object's lifetime.
class IntroProbe final {
  public:
    explicit IntroProbe(const isle::rom::Snapshot& snapshot);
    ~IntroProbe();
    IntroProbe(const IntroProbe&) = delete;
    IntroProbe& operator=(const IntroProbe&) = delete;

    Gfx* Commands() noexcept;
    void Advance();
    std::uint32_t Ticks() const noexcept;
    void StopGame();

  private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

#endif
