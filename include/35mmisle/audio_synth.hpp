#ifndef ISLE_AUDIO_SYNTH_HPP
#define ISLE_AUDIO_SYNTH_HPP

#include "35mmisle/audio_bank.hpp"
#include "35mmisle/audio_effects.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

namespace isle::audio {
namespace detail { struct Context; }
namespace sfx { struct Config; class SoundPlayer; }
namespace csp { struct Config; class SequencePlayer; }

enum class Error {
    none, invalid_resource, foreign_resource, unsupported_wave, unsupported_book,
    unsupported_loop, unsupported_envelope, invalid_parameter, event_capacity,
    voice_unavailable, descriptor_capacity, client_capacity, invalid_state,
    stale_handle, unbound, task_busy, stale_task, invalid_service,
    command_capacity, address_range, execution_failed, needs_reset,
    allocation_failure, identity_exhausted
};
struct Coefficients {
    std::array<std::uint16_t, 128> pan{};
    // Original eight log2 midpoint coefficients, not host approximations.
    std::array<double, 8> logarithm{};
};
inline constexpr std::uint16_t no_slot = 0xffff;
struct SoundId { std::uint64_t owner = 0; std::uint32_t index = 0; };
struct WaveId { std::uint64_t owner = 0; std::uint32_t index = 0; };
struct VoiceHandle {
    std::uint64_t owner = 0, generation = 0;
    std::uint16_t slot = no_slot;
};
struct PlayerId {
    std::uint64_t owner = 0, generation = 0;
    std::uint16_t slot = no_slot;
};
struct TaskToken {
    std::uint64_t owner = 0, serial = 0;
    friend bool operator==(TaskToken, TaskToken) = default;
};
struct RawSampleTime { std::uint64_t value = 0; };
struct ParamTime { std::uint64_t value = 0; }; // Multiple of16; never a raw deadline.
struct RampFrames { std::uint32_t value = 0; }; // Multiple of16; not microseconds.
struct Gain15 { std::uint16_t value = 0; }; // Linear pre-pan DSP amplitude.
struct DirectParameters {
    float pitch = 1.0f; // Raw >=1/32768 retained; DSP Q1.15 caps at 0xfffe.
    Gain15 gain{};
    std::uint8_t pan = 64;
    std::uint8_t fx_mix = 0;
};
static_assert(sizeof(DirectParameters) == 8);
struct BindingInfo {
    bool bound = false;
    // Zero-based: index0 is P01; fixed DSP traversal visits highest index first.
    std::uint16_t physical_slot = no_slot;
    std::uint64_t epoch = 0;
};
struct Allocation {
    BindingInfo binding{};
    VoiceHandle displaced{}; // Zero owner means there was no previous owner.
};
struct StealTiming {
    // Measured at32006 Hz; an explicit frame-domain policy, not a hidden-us claim.
    std::uint32_t kill_frames = 448;
    std::uint32_t restart_frames = 512;
};
struct SynthConfig {
    std::uint32_t synthesis_rate = 0; // Actual returned source rate, required.
    std::uint16_t physical_voices = 22;
    std::uint16_t logical_voices = 42;
    StealTiming steal{};
};
struct Service {
    std::uint64_t service_id = 0;
    std::uint32_t previous_ai_frames = 0; // Original lagged AI domain, not host queue.
    std::uint32_t quota_frames = 0; // Zero selects original formula; otherwise16-aligned.
};
struct TaskView {
    TaskToken token{};
    std::uint64_t service_id = 0;
    std::uint32_t command_address = 0, command_bytes = 0;
    std::uint32_t pcm_address = 0, frame_count = 0, synthesis_rate = 0;
};
struct PcmView {
    TaskToken token{};
    // Stereo signed BE16 in canonical RDRAM; not a native int16_t span.
    std::uint32_t address = 0, frame_count = 0, synthesis_rate = 0;
};

// Nonowning control view. Calls are externally serialized with build/complete;
// RSP execution may run concurrently because controls never write published RAM.
class Synth final {
public:
    Error create_voice(VoiceHandle&) noexcept;
    Error destroy_voice(VoiceHandle) noexcept; // Requires an unbound descriptor.
    Error allocate(VoiceHandle, std::int16_t priority, ParamTime, Allocation&) noexcept;
    Error set_priority(VoiceHandle, std::int16_t priority) noexcept;
    Error start(VoiceHandle, WaveId, DirectParameters, RampFrames attack, ParamTime) noexcept;
    Error set_pitch(VoiceHandle, float pitch, ParamTime) noexcept;
    Error set_pan(VoiceHandle, std::uint8_t pan, ParamTime) noexcept;
    Error set_fx_mix(VoiceHandle, std::uint8_t mix, ParamTime) noexcept;
    Error set_gain(VoiceHandle, Gain15, RampFrames, ParamTime) noexcept;
    Error stop(VoiceHandle, ParamTime) noexcept; // Retains physical binding.
    Error free(VoiceHandle, ParamTime) noexcept; // Unbinds now; retirement may be deferred.
    Error binding(VoiceHandle, BindingInfo&) const noexcept;
    ParamTime control_frontier() const noexcept;
private:
    explicit Synth(detail::Context* context) noexcept : context_(context) {}
    detail::Context* context_;
    friend class AudioService;
};
struct ServiceResult;
class AudioService final {
public:
    static constexpr std::uint32_t rdram_bytes = 0x800000;
    static constexpr std::uint32_t max_frames = 1024;
    static constexpr std::uint16_t max_physical_voices = 22;
    static constexpr std::uint16_t max_logical_voices = 42;
    static constexpr std::uint16_t max_players = 3;
    static constexpr std::size_t parameter_capacity = 1024;
    static constexpr std::uint32_t command_address = 0x110000;
    static constexpr std::uint32_t command_limit = 0x130000;
    static constexpr std::uint32_t pcm_address = 0x130000;
    static constexpr std::uint32_t state_address = 0x132000;
    static constexpr std::uint32_t state_stride = 0x100;
    static constexpr std::uint32_t resource_address = 0x140000;

    // Retains all bank/table owners and catalog aliases. Unsupported wave types
    // remain identifiable but direct start rejects them; no alternate resource.
    // Effects data is validated and copied at creation; no caller span survives.
    static ServiceResult create(std::span<const BankFile> owners, SynthConfig,
                                const Coefficients&, effects::Config) noexcept;
    ~AudioService();
    AudioService(const AudioService&) = delete;
    AudioService& operator=(const AudioService&) = delete;
    AudioService(AudioService&&) = delete;
    AudioService& operator=(AudioService&&) = delete;
    Synth synth() noexcept { return Synth(context_.get()); }
    Error sound_id(const Sound&, SoundId&) const noexcept;
    Error wave_id(SoundId, WaveId&) const noexcept;
    Error sound(SoundId, const Sound*&) const noexcept;
    // Setup-only, quiescent registration; all clients share the same pool.
    Error add_sound_player(const sfx::Config&, PlayerId&) noexcept;
    sfx::SoundPlayer player(PlayerId) noexcept;
    Error add_sequence_player(const csp::Config&, PlayerId&) noexcept;
    csp::SequencePlayer sequence_player(PlayerId) noexcept;
    // Quiescent absolute raw-clock rephasing, matching an ALPlayer deadline
    // change. Does not alter the client's microsecond or envelope calendar.
    Error set_player_deadline(PlayerId, RawSampleTime) noexcept;

    // Successful build publishes the predicted CPU graph atomically. Completion
    // only changes the lease/poison state, never restores an older CPU snapshot.
    Error build(Service, TaskView&) noexcept;
    Error complete(TaskToken, bool terminal_execution_succeeded, PcmView&) noexcept;
    Error release(TaskToken) noexcept;
    Error reset() noexcept; // Quiescent; invalidates descriptors/registrations, not serials.
    bool has_lease() const noexcept;
    Error read_pcm(const PcmView&, std::uint32_t frame,
                   std::int16_t& left, std::int16_t& right) const noexcept;
    Error copy_pcm(const PcmView&, std::span<std::int16_t>) const noexcept;
    // Stable word-swapped storage; canonical byte a is host byte[a^3]. Callers
    // may prepare task/main/boot below command_address, never producer ranges.
    std::span<std::uint32_t> rdram_words() noexcept;
    std::span<const std::uint32_t> rdram_words() const noexcept;
private:
    explicit AudioService(std::unique_ptr<detail::Context>) noexcept;
    std::unique_ptr<detail::Context> context_;
};
struct ServiceResult {
    Error error = Error::none;
    std::unique_ptr<AudioService> service;
    explicit operator bool() const noexcept { return error == Error::none && service != nullptr; }
};
Error service_quota(SynthConfig, Service, std::uint32_t& frames) noexcept;
} // namespace isle::audio
#endif
