#ifndef ISLE_AUDIO_HOST_PROBE_HPP
#define ISLE_AUDIO_HOST_PROBE_HPP

#include "audio_probe_fixture.hpp"
#include "35mmisle/audio_transport.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <optional>

// Bounded host experiment: actual native SFX14, independently queue-paced.
// Not the original game's AI/VI scheduler, cue placement, pool, BGM or effects.
// One SDL-main-thread owner; Session and output share no application callbacks.
class AudioHostProbe final {
public:
    explicit AudioHostProbe(const isle::rom::Snapshot& snapshot);
    ~AudioHostProbe();
    AudioHostProbe(const AudioHostProbe&) = delete;
    AudioHostProbe& operator=(const AudioHostProbe&) = delete;
    void Tick();
    void Finish();
    bool Abort() noexcept;
private:
    void Build();
    bool Submit();
    static void Wait(std::chrono::steady_clock::time_point deadline);
    isle::audio_probe::Session session_;
    isle::audio::SdlAudioOutput output_;
    std::array<std::int16_t, isle::audio::sfx::Producer::max_frames * 2> samples_{};
    std::optional<isle::audio_probe::Completion> pending_;
    std::uint64_t generation_ = 0, service_ = 0, produced_ = 0, accepted_ = 0;
    bool closed_ = false, cleanup_ok_ = true;
};
#endif
