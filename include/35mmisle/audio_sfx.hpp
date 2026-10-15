#ifndef ISLE_AUDIO_SFX_HPP
#define ISLE_AUDIO_SFX_HPP

#include "35mmisle/audio_synth.hpp"
#include <optional>

namespace isle::audio::sfx {
struct Config {
    std::uint16_t max_sounds = 10; // One player/calendar, not this many clients.
    std::int16_t priority = 5; // Measured original default SoundPlayer priority.
    // Omitted selects the service frontier; the initial callback has zero delay.
    std::optional<RawSampleTime> first_due;
    std::uint64_t first_client_us = 0;
};
struct Parameters {
    // Direct sound-player domain. Game sample/global conversion belongs to caller.
    std::uint16_t volume = 0x7fff;
    std::uint8_t pan = 64;
    std::uint8_t fx_mix = 0;
    float pitch = 1.0f; // Raw ratio; DSP truncation/capping does not rewrite it.
};
static_assert(sizeof(Parameters) == 8);
struct SoundHandle {
    std::uint64_t owner = 0, generation = 0;
    std::uint16_t player = no_slot, slot = no_slot;
};
struct Status {
    bool active = false, bound = false;
    VoiceHandle voice{};
    Error error = Error::none;
};

// Nonowning view of service-owned client state. A stolen binding does not
// invalidate playback identity. The service must outlive every retained view.
class SoundPlayer final {
public:
    static constexpr std::uint16_t max_sounds = 16;
    static constexpr std::size_t event_capacity = 128;
    // Supports indefinite decay (-1) and positive finite decay with nonzero
    // level. Finite sounds naturally release and retire; unmeasured zero-time
    // or zero-level finite decay is unsupported.
    // Attack DSP time stays authored; attack callbacks and both decay/release
    // durations use the raw pitch when that phase starts. Later controls never
    // retime an existing endpoint. A zero attack enters decay in the same callback.
    Error stage_start(SoundId, Parameters, SoundHandle&) noexcept;
    Error stage_pitch(SoundHandle, float) noexcept;
    Error stage_pan(SoundHandle, std::uint8_t) noexcept;
    Error stage_fx_mix(SoundHandle, std::uint8_t) noexcept;
    // One-ms gain retarget at the current attack/decay level, or zero in release.
    Error stage_volume(SoundHandle, std::uint16_t) noexcept;
    Error stage_stop(SoundHandle) noexcept;
    // Publish the bounded inbox for this player's next callback. Never modifies
    // published RDRAM or bypasses the shared raw-deadline calendar.
    Error stage() noexcept;
    Error status(SoundHandle, Status&) const noexcept;
    bool active(SoundHandle) const noexcept;
private:
    SoundPlayer(detail::Context* context, PlayerId id) noexcept : context_(context), id_(id) {}
    detail::Context* context_;
    PlayerId id_;
    friend class isle::audio::AudioService;
};
} // namespace isle::audio::sfx
#endif
