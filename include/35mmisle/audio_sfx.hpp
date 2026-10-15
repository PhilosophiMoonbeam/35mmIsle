#ifndef ISLE_AUDIO_SFX_HPP
#define ISLE_AUDIO_SFX_HPP

#include "35mmisle/audio_bank.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

namespace isle::audio::sfx {

// A deliberately bounded, dry, one-voice ABI1 producer. This is not the
// original game's multi-player/voice allocator, effects graph, or BGM API.
// Admission: ADPCM order 2, at most 8 predictors, 16-sample-aligned loops,
// attack >= 0, indefinite decay (-1), release >= 0. Other resources fail.
// Original envelope-rate and control-clock fidelity is qualified separately
// against private reference observations. Caller supplies original coefficient
// data; ADPCM/resampling execute on the consumer, not the native producer.
enum class Error {
    none, invalid_resource, foreign_sound, unsupported_wave, unsupported_book,
    unsupported_loop, unsupported_envelope, invalid_parameter, event_capacity,
    voice_busy, stale_generation, task_busy, stale_task, invalid_service,
    command_capacity, address_range, execution_failed, needs_reset,
    allocation_failure, identity_exhausted
};

using PanTable = std::array<std::uint16_t, 128>;
struct Coefficients {
    PanTable pan{};
    // Eight log2 midpoint coefficients for frexp mantissas in [0.5, 1).
    // Original resource data, not host log2 approximations, defines rounding.
    std::array<double, 8> logarithm{};
};

struct Config {
    // The rate actually returned by AI frequency setup, not the requested rate.
    // The rounded 16-ms client interval must remain positive. Client deadlines
    // and 16-frame-quantized DSP parameter times are distinct clock domains.
    std::uint32_t synthesis_rate = 0;
};
struct Parameters {
    std::uint16_t volume = 0x7fff;
    std::uint8_t pan = 64;
    std::uint8_t global_volume = 127;
    // Direct alSndp pitch domain; no keymap or bank-rate multiplication.
    // Q1.15 quantized downward, range [1/32768, 65535/32768].
    float pitch = 1.0f;
};
struct Service {
    std::uint64_t service_id = 0;
    // Lagged original-domain AI stereo frames, not host sink occupancy.
    std::uint32_t previous_ai_frames = 0;
    // Zero selects the original quota formula. Nonzero is an explicit,
    // 16-frame-aligned experimental quota, at most max_frames.
    std::uint32_t quota_frames = 0;
};
struct TaskView {
    std::uint32_t command_address = 0, command_bytes = 0;
    std::uint32_t pcm_address = 0, frame_count = 0;
    std::uint32_t synthesis_rate = 0;
    std::uint64_t task_id = 0, generation = 0, service_id = 0;
};
struct PcmView {
    // Stereo signed BE16 in canonical RDRAM, not a native int16_t span.
    std::uint32_t address = 0, frame_count = 0, synthesis_rate = 0;
    std::uint64_t task_id = 0, generation = 0;
};
struct CreateResult;

class Producer final {
public:
    static constexpr std::uint32_t rdram_bytes = 0x800000;
    static constexpr std::uint32_t max_frames = 1024;
    static constexpr std::size_t event_capacity = 32;
    static constexpr std::uint32_t command_address = 0x110000;
    static constexpr std::uint32_t command_limit = 0x130000;
    static constexpr std::uint32_t pcm_address = 0x130000;
    static constexpr std::uint32_t adpcm_state_address = 0x132000;
    static constexpr std::uint32_t resample_state_address = 0x132040;
    static constexpr std::uint32_t envelope_state_address = 0x132080;
    static constexpr std::uint32_t loop_state_address = 0x132100;
    static constexpr std::uint32_t book_address = 0x133000;
    static constexpr std::uint32_t sample_address = 0x140000;

    // The selected Sound must be reachable through owner. Rate must be the
    // actual returned synthesis rate; no assumed default. Coefficients are
    // copied once for ownership, not regenerated from floating sin/cos/log2.
    // Resources are materialized once; no SDK implementation is imported.
    static CreateResult create(BankFile owner, const Sound& selected,
                               Config config, const Coefficients& coefficients) noexcept;
    ~Producer();
    Producer(const Producer&) = delete;
    Producer& operator=(const Producer&) = delete;
    Producer(Producer&&) = delete;
    Producer& operator=(Producer&&) = delete;

    // Stable 8MiB, word-swapped canonical RDRAM: canonical byte a is host
    // byte[a ^ 3]. Caller may load the original main/task/boot below 0x110000;
    // it must not alter the producer-owned ranges or race task execution.
    std::span<std::uint32_t> rdram_words() noexcept;
    std::span<const std::uint32_t> rdram_words() const noexcept;

    // Posting does not synthesize or change a published task. stage() publishes
    // queued controls to the next sound-client callback: periodic 16-ms polls
    // plus envelope events. Its raw deadline and 16-frame DSP parameter time
    // differ. Build first, then stage after submission for original outer order.
    Error stage_start(Parameters parameters, std::uint64_t& generation) noexcept;
    Error stage_pitch(std::uint64_t generation, float pitch) noexcept;
    Error stage_pan(std::uint64_t generation, std::uint8_t pan) noexcept;
    Error stage_stop(std::uint64_t generation) noexcept;
    Error stage() noexcept;

    // One outstanding task OR completed PCM lease. Build failures publish
    // nothing and do not advance the voice/service clock. Idle builds emit
    // real clear/interleave/save commands. No per-service allocation occurs.
    Error build(Service service, TaskView& task) noexcept;
    // Both outcomes acknowledge actual terminal RSP execution (including a
    // fully stopped cancellation), never request cancellation. Not playback.
    // Failure poisons mutable DSP state until quiescent reset. A stale identity
    // never releases/recycles anything. A successful completion returns a PCM
    // lease that MUST be explicitly released before another build/reset.
    Error complete(std::uint64_t task_id, std::uint64_t generation,
                   bool success, PcmView& pcm) noexcept;
    Error release(std::uint64_t task_id, std::uint64_t generation) noexcept;
    Error reset() noexcept;
    bool voice_active() const noexcept;
    bool has_lease() const noexcept;

    // Read canonical PCM without an intermediate buffer or pointer casts.
    Error read_pcm(const PcmView& pcm, std::uint32_t frame,
                   std::int16_t& left, std::int16_t& right) const noexcept;
    // One lease check for a whole native-endian stereo block. No write on an
    // invalid lease or short destination; trailing destination samples untouched.
    Error copy_pcm(const PcmView& pcm, std::span<std::int16_t> destination) const noexcept;

private:
    struct Impl;
    explicit Producer(std::unique_ptr<Impl>) noexcept;
    std::unique_ptr<Impl> impl_;
};
struct CreateResult {
    Error error = Error::none;
    std::unique_ptr<Producer> producer;
    explicit operator bool() const noexcept {
        return error == Error::none && producer != nullptr;
    }
};

// Original service quota, checked before conversion to an unsigned frame count.
Error service_quota(Config config, Service service, std::uint32_t& frames) noexcept;

} // namespace isle::audio::sfx
#endif
