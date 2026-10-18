#ifndef ISLE_GAME_HOST_HPP
#define ISLE_GAME_HOST_HPP

#include "game_audio.hpp"
#include "35mmisle/audio_transport.hpp"
#include "35mmisle/game_input.hpp"
#include "35mmisle/game_save.hpp"
#include <filesystem>

namespace isle::game_host {
inline constexpr std::uint32_t width = 320, height = 240, stride = width * 4;
inline constexpr std::size_t rgba_bytes = stride * height;
enum class Error { none, invalid, allocation, platform, resources, input, audio, graphics, retired };
enum class InputMode { control_deck, diagnostic_disabled };
enum class Present { idle, declined, completed, failed };
enum class Drain { pending, drained, failed };
struct Config {
    // Empty paths select product application data and the pinned shader tree.
    std::filesystem::path data_directory, renderer_resources;
    InputMode input = InputMode::control_deck;
    std::size_t max_queued_audio_frames = 4096;
    SDL_AudioDeviceID physical_audio_device = 0;
};
struct Input {
    game_input::HostFrame ports{};
    std::uint8_t controller_bits = 0;
    // False means no read completion/publication/acknowledgement may be made.
    // Diagnostic-disabled is an intentional completed ABSENT acquisition.
    bool completed = false;
};
struct Shutdown {
    audio::SdlAudioOutput::CloseDisposition audio = audio::SdlAudioOutput::CloseDisposition::not_open;
    bool owners_released = false, failed = false;
};
struct CreateResult;
// One SDL main-thread owner. Does not own a game scene, Service input cache or
// source clock. Snapshot is borrowed ONLY during create; decoded audio, boot,
// presentation and configuration bytes are subsequently owned here.
class Host final {
public:
    static CreateResult create(const rom::Snapshot&, Config = {}) noexcept;
    ~Host();
    Host(const Host&) = delete;
    Host& operator=(const Host&) = delete;
    bool running() const noexcept;
    Error error() const noexcept;
    const char* message() const noexcept; // First failure, retained through shutdown.
    const std::filesystem::path& data_directory() const noexcept;
    // Commands from serialized GAME continuations inspect the actual endpoint
    // through the continuation owner-service fence; endpoint I/O stays on main.
    game_audio::Session* audio() noexcept;
    const audio::SdlAudioOutput::Info& audio_info() const noexcept;
    bool pump_events() noexcept;
    Input acquire_input() noexcept;
    // Independent scheduler input: never called by events or presentation.
    bool notify_retrace(std::uint64_t ordinal, std::uint64_t monotonic_ns) noexcept;
    // At most one actual bounded RSP execution; blocked PCM retains exact lease.
    bool pump_audio() noexcept;
    bool stop_audio() noexcept;
    Drain poll_audio_drain() noexcept;
    // Copies exactly one tightly packed 320x240 RGBA8 surface synchronously.
    // Declined presentation keeps this same copy; new frames cannot overwrite it.
    // Source VI offsets are horizontal clocks and vertical half-lines, not camera
    // pixels: x=horizontal/2, y=(vertical rounded down to even)/2 at 320x240.
    bool queue_present(std::span<const std::byte>, save::ScreenOffsets = {}) noexcept;
    bool presentation_pending() const noexcept;
    Present try_present() noexcept;
    bool cancel_present() noexcept;
    // Call only after all scene/graphics users of audio() have retired. A pending
    // stream is honestly aborted, never described as drained. Destructor does the
    // same nonblocking abort; graceful drain needs real retraces+pump_audio first.
    Shutdown shutdown() noexcept;
private:
    struct Impl;
    explicit Host(std::unique_ptr<Impl>) noexcept;
    std::unique_ptr<Impl> impl_;
};
struct CreateResult {
    Error error = Error::none;
    std::unique_ptr<Host> host;
    std::array<char, 512> message{};
    explicit operator bool() const noexcept { return error == Error::none && bool(host); }
};
} // namespace isle::game_host
#endif
