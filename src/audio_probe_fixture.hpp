#ifndef ISLE_AUDIO_PROBE_FIXTURE_HPP
#define ISLE_AUDIO_PROBE_FIXTURE_HPP

#include "35mmisle/audio_sfx.hpp"
#include "rom_snapshot.hpp"
#include "35mmisle/rsp.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

namespace isle::audio_probe {
inline constexpr std::uint32_t synthesis_rate = 32006;
inline constexpr std::uint32_t default_quota = 640;

// Game auPlaySoundWithParams domain. The probe fixes game global volume at127;
// the producer receives the resulting explicit sound-player volume instead.
struct GameParameters {
    std::uint16_t volume = 0x7fff;
    std::uint8_t pan = 64;
    float pitch = 1.0f;
};

struct Completion {
    audio::TaskView task{};
    rsp::Result execution{};
    audio::PcmView pcm{};
};

// Private exact-US SoundPlayer consumer, not full game/audio integration.
// Immutable hydration/main/BSS/boot preparation consume one admitted snapshot;
// the shared service owns every resource before the constructor returns.
class Session final {
public:
    Session(const rom::Snapshot& snapshot, std::uint32_t sound_index);
    ~Session();
    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

    const rom::Identity& identity() const noexcept { return identity_; }
    audio::AudioService& service() noexcept { return *service_; }
    audio::sfx::SoundPlayer player() noexcept { return service_->player(player_); }
    audio::SoundId selected_sound() const noexcept { return selected_sound_; }
    // Convert prevalidated game parameters once, preserving game integer shifts.
    audio::sfx::Parameters player_parameters(GameParameters parameters) const noexcept;
    // Executes actual ROM aspMain. Returns only after SP=0x203 and no DPC.
    // Leaves the actual completed PCM lease owned by service(): a downstream
    // sink must retain it on WouldBlock, release after copying, and never build
    // another task while leased. No SDL dependency or playback claim here.
    Completion execute(audio::Service service);

private:
    rom::Identity identity_{};
    std::uint8_t sample_volume_ = 0;
    std::array<std::byte, 0x100> boot_{};
    std::unique_ptr<audio::AudioService> service_;
    audio::PlayerId player_{};
    audio::SoundId selected_sound_{};
    // Destroy the borrower before service-owned RDRAM.
    std::unique_ptr<rsp::Machine> machine_;
};

void require(audio::Error error, const char* operation);
} // namespace isle::audio_probe
#endif
