#ifndef ISLE_AUDIO_TRANSPORT_HPP
#define ISLE_AUDIO_TRANSPORT_HPP

#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_events.h>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <span>

namespace isle::audio {
// Original SDL-only leaf, not a Ship::Audio backend or producer/PCM-lease owner.
// All application calls, destruction and SDL event pumping have one serialized
// owner on SDL's main thread. The host must keep SDL alive, pump events regularly,
// keep AUDIO_DEVICE_REMOVED enabled and install no global SDL event filter for
// this run. Other event consumers may freely remove queued events: the watch
// observes them first. No application callback, worker or second PCM queue exists.
//
// Accepted means SDL copied the whole block, not that a physical endpoint heard
// it. Queue counts are immutable-format source frames, never guest AI occupancy.
// Disconnect detection depends on SDL delivering its removal notification; SDL's
// zombie consumer, queue progress and stream drain cannot prove endpoint liveness.
class SdlAudioOutput final {
public:
    enum class State { closed, ready, draining, stream_drained, failed };
    enum class ErrorKind {
        none, invalid_argument, invalid_state, sdl_failure, unavailable_device,
        disconnected, inconsistent_queue, frame_counter_overflow
    };
    struct Error {
        ErrorKind kind = ErrorKind::none;
        std::array<char, 64> operation{};
        std::array<char, 512> message{};
    };
    struct Config {
        int source_rate = 0;
        std::size_t max_queued_source_frames = 0;
        // Must be a currently enumerated physical playback ID. Zero resolves
        // once to the FIRST enumerated physical endpoint, not the migratable
        // SDL default. No other device/driver is tried if opening it fails.
        SDL_AudioDeviceID physical_device = 0;
    };
    struct Info {
        int sdl_version = 0;
        std::array<char, 64> driver{};
        std::array<char, 256> device_name{};
        SDL_AudioDeviceID physical_device = 0, logical_device = 0;
        // Checked opening snapshots; source remains native-endian S16 stereo.
        // Stream output/device formats may differ: SDL owns conversion.
        SDL_AudioSpec source{}, stream_output{}, device{};
        int device_buffer_frames = 0;
        std::size_t max_queued_source_frames = 0;
    };
    struct Queue {
        // Counts are unavailable unless valid. Available is converted OUTPUT
        // bytes; it is not divided by the four-byte source frame size.
        bool valid = false;
        std::size_t queued_source_frames = 0;
        int available_output_bytes = 0;
        std::uint64_t accepted_source_frames = 0;
        bool paused = false;
        explicit operator bool() const noexcept { return valid; }
    };
    enum class Submit { accepted, would_block, error };
    enum class Drain { pending, stream_drained, error };
    enum class CloseDisposition { not_open, aborted, stream_drained };
    struct CloseResult {
        CloseDisposition disposition = CloseDisposition::not_open;
        bool failed = false;
    };

    SdlAudioOutput() = default;
    ~SdlAudioOutput();
    SdlAudioOutput(const SdlAudioOutput&) = delete;
    SdlAudioOutput& operator=(const SdlAudioOutput&) = delete;
    SdlAudioOutput(SdlAudioOutput&&) = delete;
    SdlAudioOutput& operator=(SdlAudioOutput&&) = delete;

    // A new run starts only after close; it resets that run's retained error.
    // Acquires/balances only its own SDL_INIT_AUDIO reference, never SDL_Quit.
    // Opens paused, verifies formats/identity, then checks resume before ready.
    bool open(Config config) noexcept;
    State state() noexcept;
    const Error& error() noexcept;
    const Info& info() const noexcept { return info_; }
    Queue query() noexcept;
    // Nonempty native-endian interleaved S16 stereo, whole block or WouldBlock.
    // An odd sample count or block larger than the entire configured bound is
    // an explicit terminal argument error, never an indefinitely blocked block.
    // Success copies synchronously in SDL; the caller may reuse its scratch.
    // No allocation or extra PCM copy is performed by this leaf per submission.
    Submit try_submit(std::span<const std::int16_t> samples) noexcept;
    bool pause() noexcept;
    bool resume() noexcept;
    // Checked flush exactly once after the last accepted input; no later puts.
    bool finish_input() noexcept;
    // Requires flush and BOTH input queued/output available == 0. Never steals
    // output with SDL_GetAudioStreamData. The owner supplies a monotonic deadline;
    // expiry is not completion. This is NOT downstream/hardware drain or hearing.
    Drain poll_drain() noexcept;
    // Idempotent quiescent cleanup. Anything but observed stream_drained is an
    // explicit abort/discard. DestroyAudioStream is void and auto-closes its
    // logical device; no fabricated checked close or second device close.
    // The first operational error survives cleanup until the next open run.
    CloseResult close() noexcept;

private:
    static bool SDLCALL watch(void* userdata, SDL_Event* event) noexcept;
    bool consume_removal() noexcept;
    bool require_stream(const char* operation) noexcept;
    bool queued_frames(std::size_t& frames) noexcept;
    bool fail(ErrorKind kind, const char* operation, const char* message) noexcept;
    bool fail_sdl(const char* operation) noexcept;
    void cleanup() noexcept;

    SDL_AudioStream* stream_ = nullptr;
    State state_ = State::closed;
    Error error_{};
    Info info_{};
    std::uint64_t accepted_source_frames_ = 0;
    bool owns_audio_ = false, watching_ = false, paused_ = false;
    CloseResult close_result_{};
    // The watch touches only these atomics, never owner state/stream/producer.
    std::atomic<SDL_AudioDeviceID> watched_physical_{0}, watched_logical_{0};
    std::atomic<bool> removed_{false};
};
} // namespace isle::audio
#endif
