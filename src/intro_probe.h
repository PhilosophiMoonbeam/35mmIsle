#ifndef ISLE_INTRO_PROBE_H
#define ISLE_INTRO_PROBE_H

#include <libultraship/libultra/gbi.h>

#include <cstdint>
#include <filesystem>
#include <memory>

// One private-ROM sprite and selected authoritative C fade, not scene boot.
// Keep this owner alive through ProbeHost::Shutdown: display lists and texels
// are borrowed by LUS, independently of the C object's lifetime.
class IntroProbe final {
  public:
    explicit IntroProbe(const std::filesystem::path& romPath);
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
