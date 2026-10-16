#ifndef ISLE_GAME_AUDIO_HPP
#define ISLE_GAME_AUDIO_HPP

#include "35mmisle/audio_csp.hpp"
#include "35mmisle/audio_sfx.hpp"
#include "rom_snapshot.hpp"
#include <array>
#include <memory>
#include <vector>

namespace isle::game_audio {
namespace a = audio;
inline constexpr std::uint32_t source_rate = 32006;
inline constexpr std::size_t sound_slots = 10;

// Immutable decoded GAME inputs; no original producer bodies or SDK layouts.
struct Resources {
    a::BankFile sounds, music;
    a::SequenceFile sequences;
    a::Coefficients coefficients{};
    std::vector<std::uint8_t> priorities;
    std::array<std::int32_t, 66> startup_effect_words{};
};
struct ResourceResult {
    a::Error error = a::Error::none;
    Resources resources;
    explicit operator bool() const noexcept { return error == a::Error::none; }
};
ResourceResult hydrate(const rom::Snapshot&) noexcept;
// Own explicit numeric-input wire: exactly three38-byte ascending records of
// BE32 GAME parameter, BE16 command gain and16 signed BE16 book entries.
// Shape/order checked atomically; provenance admission remains the caller's.
a::Error decode_startup_poles(std::span<const std::byte>,
                              std::array<a::effects::PoleFilter, 3>&) noexcept;
// The three books must come from admitted numeric observations or an original
// lawful generator for GAME pole parameters 0x5000,0x6000,0x7000 respectively.
// This decoder never invents them or silently substitutes a dry route.
a::Error startup_effects(const Resources&, const std::array<a::effects::PoleFilter, 3>&,
                         std::array<a::effects::Section, 8>&, a::effects::Config&) noexcept;
// Native DC-normalized one-pole expansion for the three reached startup Q15
// parameters. Q14 gain=(1-a), lag-two row=0, lag-one row=a^1..a^8. This derives
// numeric RSP inputs; it neither imports an SDK generator nor claims parity for
// other parameter domains. The explicit-input overload remains the independent
// observation-comparison boundary, not a required private end-user sidecar.
a::Error native_startup_effects(const Resources&, std::array<a::effects::Section, 8>&,
                                a::effects::Config&) noexcept;

// A checked consumer, not a Null sink. Every callback is required. The owner
// outlives Session. Calls are serialized with gameplay; inspect can originate
// from an active GAME continuation. Owner-affine host APIs must synchronously
// rendezvous with their constructing owner, not return cached readiness. submit,
// finish and poll run on the service owner. submit must
// synchronously own exactly accepted_frames of the prefix, even on would_block;
// failure may accept a prefix but is terminal, never retried. Native S16 stereo.
struct Output {
    enum class State { ready, draining, drained, failed };
    enum class Submit { accepted, would_block, failed };
    struct Acceptance { Submit result = Submit::failed; std::uint32_t accepted_frames = 0; };
    struct Info { State state = State::failed; std::uint32_t rate = 0; };
    void* owner = nullptr;
    Info (*inspect)(void*) noexcept = nullptr;
    Acceptance (*submit)(void*, std::span<const std::int16_t>) noexcept = nullptr;
    bool (*finish)(void*) noexcept = nullptr;
    State (*poll)(void*) noexcept = nullptr;
};
// Checked actual executor preparation over this service's stable allocation.
// initialize installs admitted main/BSS/boot and creates the real RSP borrower;
// ready checks that borrower, not a manually asserted flag. retire must destroy
// the borrower before returning. Callbacks never throw; owner outlives Session.
struct Executor {
    void* owner = nullptr;
    bool (*initialize)(void*, std::span<std::uint32_t>) noexcept = nullptr;
    bool (*ready)(void*, std::span<std::uint32_t>) noexcept = nullptr;
    void (*retire)(void*) noexcept = nullptr;
};
struct Parameters {
    std::uint16_t volume = 0x7fff;
    std::uint8_t pan = 64, reverb = 0;
    float pitch = 1.0f;
};
struct CreateResult;
// One GAME service owner. Commands may arrive while an RSP task/PCM lease is
// live, but only the independently supplied scheduler retraces advance the GAME
// command state machine. Rendering/presenting never calls retrace implicitly.
class Session final {
public:
    enum class State { ready, stopping, draining, drained, failed };
    static CreateResult create(Resources, a::effects::Config, Output, Executor) noexcept;
    ~Session();
    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;
    State state() const noexcept { return state_; }
    a::Error error() const noexcept { return error_; }
    bool initialization_ready() const noexcept { return state_ == State::ready && !restarting_; }
    a::AudioService& service() noexcept { return *service_; }

    a::Error play_song(unsigned player, std::uint32_t song) noexcept;
    a::Error stop_song(unsigned player) noexcept;
    a::Error stop_bgm() noexcept;
    bool bgm_playing(unsigned player) const noexcept;
    a::Error bgm_volume(unsigned player, std::uint32_t volume, std::uint32_t retraces = 0) noexcept;
    a::Error bgm_tempo(unsigned player, std::uint32_t us_per_quarter) noexcept;
    a::Error get_bgm_tempo(unsigned player, std::uint32_t& value) const noexcept;
    a::Error bgm_reverb(unsigned player, std::uint32_t) noexcept;
    a::Error bgm_priority(unsigned player, std::uint8_t) noexcept;
    a::Error bgm_extra_reverb(unsigned player, std::int8_t) noexcept;
    a::Error bgm_channel_volume(unsigned player, std::uint8_t, std::int8_t) noexcept;
    a::Error play_sound(std::uint32_t sound, Parameters, int& slot) noexcept;
    a::Error stop_sound(unsigned slot) noexcept;
    a::Error stop_sounds() noexcept;
    a::Error sound_volume(unsigned slot, std::uint32_t) noexcept;
    a::Error sound_pan(unsigned slot, std::uint32_t) noexcept;
    a::Error sound_pitch(unsigned slot, float) noexcept;
    a::Error sound_reverb(unsigned slot, std::uint32_t) noexcept;
    a::Error sound_priority(unsigned slot, std::uint8_t) noexcept;
    a::Error global_sound_volume(std::uint8_t, bool current_only = false) noexcept;
    a::Error global_sound_reverb(std::uint8_t, bool current_only = false) noexcept;
    int playing_sound(unsigned slot) const noexcept;
    void high_quality(bool value) noexcept { high_quality_ = value; }

    // Ordinals strictly increase, timestamps are monotonic. Native AI ledger:
    // 4 bytes per stereo frame, consumed at source_rate in monotonic ns, with
    // fractional frames carried. This emulates AI source time, NOT SDL bytes or
    // physical playback. One-message GAME tick queue coalesces occupied ticks.
    // The sampled head-buffer remainder is lagged into the NEXT build quota,
    // matching sp70 in audio.c; initial sp70=0 is not invented output credit.
    a::Error retrace(std::uint64_t ordinal, std::uint64_t monotonic_ns) noexcept;
    // No-work is Error::none plus token.serial==0. One task/PCM lease at a time.
    // The initial silent task may build before the first retrace, as in GAME.
    // A post-build command error may return a nonzero task token: the owner
    // must report a real failed/cancelled terminal via complete(token,false).
    a::Error build(a::TaskView&) noexcept;
    // First source tick submits zero-count bootstrap buffer0: no host PCM,
    // no invented occupancy. First actual PCM requires two session-local source
    // notifications, independent of the external scheduler's ordinal origin.
    a::Error complete(a::TaskToken, bool actual_terminal_success) noexcept;
    // Retains the lease and immutable scratch on blocked/partial acceptance.
    a::Error submit() noexcept;
    a::Error stop() noexcept;
    // Reset request stops actual clients, keeps executing/PCM owners alive and
    // waits for complete acceptance + native client retirement before resetting.
    a::Error restart() noexcept;
    a::Error restart(a::effects::Config) noexcept;
    a::Error poll_drain() noexcept;
    // Explicit abort/discard of already completed PCM, never a successful drain.
    // An executing task must first deliver its actual terminal/cancellation.
    a::Error abandon_pcm() noexcept;
    std::uint32_t native_ai_frames() const noexcept;
    std::uint64_t accepted_frames() const noexcept { return accepted_frames_; }
private:
    explicit Session(Resources, Output, Executor) noexcept;
    a::Error register_players() noexcept;
    a::Error update() noexcept;
    a::Error fail(a::Error) noexcept;
    a::Error control() const noexcept;
    a::Error publish(unsigned player, a::Error) noexcept;
    a::Error sound_control(unsigned slot, a::Error) noexcept;
    bool retired() noexcept;
    struct Sound {
        int playing = -1, starting = -1, bound_sound = -1;
        std::uint8_t priority = 0, idle = 255;
        Parameters parameters{};
        a::sfx::SoundHandle handle{};
    };
    struct Song {
        int requested = -1;
        unsigned phase = 0;
        float volume = 32512.0f, volume_step = 0;
        std::uint32_t volume_ticks = 0;
    };
    Resources resources_;
    Output output_;
    Executor executor_;
    std::unique_ptr<a::AudioService> service_;
    a::PlayerId sfx_{};
    std::array<a::PlayerId, 2> csp_{};
    std::array<Sound, sound_slots> sounds_{};
    std::array<Song, 2> songs_{};
    std::array<std::int16_t, a::AudioService::max_frames * 2> scratch_{};
    a::TaskView task_{};
    a::PcmView pcm_{};
    std::uint32_t submitted_ = 0;
    std::array<std::uint32_t, 2> ai_{};
    std::array<a::effects::Section, a::effects::max_sections> restart_sections_{};
    a::effects::Config restart_effects_{};
    std::uint64_t last_retrace_ = 0, now_ns_ = 0, fractional_ = 0;
    std::uint64_t service_serial_ = 0, accepted_frames_ = 0;
    std::uint64_t retrace_notifications_ = 0, output_after_notification_ = 0;
    std::uint32_t lagged_ai_ = 0, sampled_ai_ = 0;
    std::uint8_t global_priority_ = 100;
    std::uint8_t global_volume_ = 127, global_reverb_ = 0;
    bool clock_started_ = false, tick_pending_ = false, initial_build_ = true;
    bool initial_update_pending_ = false;
    bool executor_started_ = false;
    bool high_quality_ = true, restarting_ = false;
    bool reconfigure_ = false;
    State state_ = State::ready;
    a::Error error_ = a::Error::none;
};
struct CreateResult {
    a::Error error = a::Error::none;
    std::unique_ptr<Session> session;
    explicit operator bool() const noexcept { return error == a::Error::none && bool(session); }
};
} // namespace isle::game_audio
#endif
