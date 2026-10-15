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

struct Completion {
    audio::sfx::TaskView task{};
    rsp::Result execution{};
    audio::sfx::PcmView pcm{};
};

// Private, exact-US, one-voice experiment, not a game/audio engine. Resource
// hydration and original main/BSS/boot preparation consume an admitted snapshot
// once; all consumers own their resources before the constructor returns.
class Session final {
public:
    Session(const rom::Snapshot& snapshot, std::uint32_t sound_index);
    ~Session();
    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

    const rom::Identity& identity() const noexcept { return identity_; }
    audio::sfx::Producer& producer() noexcept { return *producer_; }
    // Executes actual ROM aspMain. Returns only after SP=0x203 and no DPC.
    // Leaves the actual completed PCM lease owned by producer(): a downstream
    // sink must retain it on WouldBlock, release after copying, and never build
    // another task while leased. No SDL dependency or playback claim here.
    Completion execute(audio::sfx::Service service);

private:
    rom::Identity identity_{};
    std::array<std::byte, 0x100> boot_{};
    std::unique_ptr<audio::sfx::Producer> producer_;
    // Destroy the borrower before producer-owned RDRAM.
    std::unique_ptr<rsp::Machine> machine_;
};

void require(audio::sfx::Error error, const char* operation);
} // namespace isle::audio_probe
#endif
