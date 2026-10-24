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
// One SDL main-thread owner. Does not own a game scene, Service input cache,
// source clock, audio producer or RSP. Presentation and configuration bytes are
// owned here; GAME users and their Driver must retire before this endpoint.
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
    // Borrowed actual main-owned SDL endpoint. All callbacks require main;
    // direct main-owned continuation diagnostics can hand off inspection only.
    // A dedicated GAME owner must rendezvous every callback with actual main.
    game_audio::Output audio_output() noexcept;
    const audio::SdlAudioOutput::Info& audio_info() const noexcept;
    bool pump_events() noexcept;
    // Native F9 key-down edge, independent of controller/Source input. The
    // caller consumes it outside Source callbacks before touching the endpoint.
    bool station_action_pending() const noexcept;
    bool take_station_action() noexcept;
    bool station_feedback(const char* status) noexcept;
    Input acquire_input() noexcept;
    // Explicit scene-cold boundary; refuses pending presentation or failed/stopped
    // ownership. Supports only 320x240 and 640x480; fences old GPU/cache borrowers.
    bool configure_present(std::uint32_t surface_width, std::uint32_t surface_height) noexcept;
    // Copies one tightly packed RGBA8 surface at the configured extent. Declined
    // presentation keeps this copy; queueing never allocates or changes extent.
    // VI clocks/half-lines scale with extent; vertical rounds down to even first.
    bool queue_present(std::span<const std::byte>, save::ScreenOffsets = {}) noexcept;
    bool queue_present(std::span<const std::byte>, std::uint32_t surface_width,
                       std::uint32_t surface_height, save::ScreenOffsets = {}) noexcept;
    bool presentation_pending() const noexcept;
    Present try_present() noexcept;
    bool cancel_present() noexcept;
    // Call only after every GAME user and borrowed-output Driver has retired.
    // Host only closes its actual SDL stream; it cannot retire Source players or
    // their RSP. Pending output is honestly aborted, never described as drained.
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
