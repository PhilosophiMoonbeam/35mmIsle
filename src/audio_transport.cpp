#include "35mmisle/audio_transport.hpp"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_version.h>

#include <algorithm>
#include <cstring>
#include <limits>

namespace isle::audio {
namespace {
constexpr std::size_t source_frame_bytes = 2 * sizeof(std::int16_t);
static_assert(source_frame_bytes == 4);

template<std::size_t N>
void copy_text(std::array<char, N>& destination, const char* source) noexcept {
    const std::size_t length = source ? std::min(std::strlen(source), N - 1) : 0;
    if (length) std::memcpy(destination.data(), source, length);
    destination[length] = '\0';
}
} // namespace

SdlAudioOutput::~SdlAudioOutput() { close(); }

bool SdlAudioOutput::fail(ErrorKind kind, const char* operation, const char* message) noexcept {
    if (error_.kind == ErrorKind::none) {
        error_.kind = kind;
        copy_text(error_.operation, operation);
        copy_text(error_.message, message);
    }
    state_ = State::failed;
    return false;
}

bool SdlAudioOutput::fail_sdl(const char* operation) noexcept {
    // Copy on this thread before cleanup or any subsequent SDL call changes it.
    return fail(ErrorKind::sdl_failure, operation, SDL_GetError());
}

bool SDLCALL SdlAudioOutput::watch(void* userdata, SDL_Event* event) noexcept {
    if (event->type == SDL_EVENT_AUDIO_DEVICE_REMOVED && !event->adevice.recording) {
        auto& self = *static_cast<SdlAudioOutput*>(userdata);
        const auto physical = self.watched_physical_.load(std::memory_order_acquire);
        const auto logical = self.watched_logical_.load(std::memory_order_acquire);
        // Before both identities are published, any playback removal fails
        // closed. This covers enumeration/open/returned-logical-ID races.
        if (!physical || !logical || event->adevice.which == physical || event->adevice.which == logical) {
            self.removed_.store(true, std::memory_order_release);
        }
    }
    return true;
}

bool SdlAudioOutput::consume_removal() noexcept {
    if (removed_.load(std::memory_order_acquire)) {
        return fail(ErrorKind::disconnected, "SDL_EVENT_AUDIO_DEVICE_REMOVED",
                    "Selected playback endpoint/logical device was removed (or removal raced opening)");
    }
    return state_ != State::failed;
}

void SdlAudioOutput::cleanup() noexcept {
    if (watching_) {
        // SDL 3.4.2 SDL_eventwatch.c holds the same watch-list mutex throughout
        // dispatch and removal. On this non-callback owner, removal fences any
        // in-flight invocation before userdata can die. Keep this object alive
        // through removal; do not unregister/close from the callback itself.
        SDL_RemoveEventWatch(watch, this);
        watching_ = false;
        consume_removal();
    }
    // Unregister BEFORE destruction: self-close is not an endpoint failure.
    if (stream_) {
        SDL_DestroyAudioStream(stream_);
        stream_ = nullptr;
    }
    if (owns_audio_) {
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        owns_audio_ = false;
    }
}

bool SdlAudioOutput::open(Config config) noexcept {
    if (state_ != State::closed) {
        return fail(ErrorKind::invalid_state, "open", "Close the existing run before opening another");
    }
    error_ = {};
    info_ = {};
    close_result_ = {};
    accepted_source_frames_ = 0;
    paused_ = false;
    watched_physical_.store(0, std::memory_order_release);
    watched_logical_.store(0, std::memory_order_release);
    removed_.store(false, std::memory_order_release);
    if (config.source_rate <= 0 || !config.max_queued_source_frames ||
        config.max_queued_source_frames > static_cast<std::size_t>(std::numeric_limits<int>::max()) / source_frame_bytes) {
        return fail(ErrorKind::invalid_argument, "open", "Positive source rate and input bound <= INT_MAX / 4 frames required");
    }
    info_.sdl_version = SDL_GetVersion();
    info_.source = {SDL_AUDIO_S16, 2, config.source_rate};
    info_.max_queued_source_frames = config.max_queued_source_frames;
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) return fail_sdl("SDL_InitSubSystem(SDL_INIT_AUDIO)");
    owns_audio_ = true;
    if (!SDL_EventEnabled(SDL_EVENT_AUDIO_DEVICE_REMOVED) || SDL_GetEventFilter(nullptr, nullptr)) {
        fail(ErrorKind::invalid_state, "SDL event configuration", "Removal events must be enabled and no global event filter installed");
        cleanup();
        return false;
    }
    if (!SDL_AddEventWatch(watch, this)) {
        fail_sdl("SDL_AddEventWatch");
        cleanup();
        return false;
    }
    watching_ = true;
    int count = 0;
    SDL_AudioDeviceID* devices = SDL_GetAudioPlaybackDevices(&count);
    if (!devices) {
        fail_sdl("SDL_GetAudioPlaybackDevices");
        cleanup();
        return false;
    }
    SDL_AudioDeviceID selected = config.physical_device;
    if (!selected && count > 0) selected = devices[0];
    bool found = false;
    for (int i = 0; i < count; ++i) found = found || devices[i] == selected;
    SDL_free(devices);
    if (!selected || !found) {
        fail(ErrorKind::unavailable_device, "select physical playback device", "Requested physical endpoint is not enumerated; no fallback attempted");
        cleanup();
        return false;
    }
    info_.physical_device = selected;
    watched_physical_.store(selected, std::memory_order_release);
    const char* name = SDL_GetAudioDeviceName(selected);
    if (!name) {
        fail_sdl("SDL_GetAudioDeviceName");
        cleanup();
        return false;
    }
    copy_text(info_.device_name, name);
    const char* driver = SDL_GetCurrentAudioDriver();
    if (!driver) {
        fail(ErrorKind::invalid_state, "SDL_GetCurrentAudioDriver", "Audio subsystem has no selected driver");
        cleanup();
        return false;
    }
    copy_text(info_.driver, driver);
    if (!consume_removal()) {
        cleanup();
        return false;
    }
    // Physical identity is already watched; never open the migratable default.
    stream_ = SDL_OpenAudioDeviceStream(selected, &info_.source, nullptr, nullptr);
    if (!stream_) {
        fail_sdl("SDL_OpenAudioDeviceStream");
        cleanup();
        return false;
    }
    paused_ = true;
    info_.logical_device = SDL_GetAudioStreamDevice(stream_);
    if (!info_.logical_device) {
        fail_sdl("SDL_GetAudioStreamDevice");
        cleanup();
        return false;
    }
    watched_logical_.store(info_.logical_device, std::memory_order_release);
    if (!SDL_GetAudioDeviceFormat(info_.logical_device, &info_.device, &info_.device_buffer_frames)) {
        fail_sdl("SDL_GetAudioDeviceFormat");
        cleanup();
        return false;
    }
    SDL_AudioSpec source{};
    if (!SDL_GetAudioStreamFormat(stream_, &source, &info_.stream_output)) {
        fail_sdl("SDL_GetAudioStreamFormat");
        cleanup();
        return false;
    }
    if (source.format != SDL_AUDIO_S16 || source.channels != 2 || source.freq != config.source_rate ||
        info_.device.freq <= 0 || info_.device.channels <= 0 || !info_.device.format ||
        info_.stream_output.freq <= 0 || info_.stream_output.channels <= 0 || !info_.stream_output.format ||
        info_.device_buffer_frames <= 0) {
        fail(ErrorKind::invalid_state, "validate opening formats", "SDL returned an unexpected source or invalid device/output format");
        cleanup();
        return false;
    }
    if (!consume_removal()) {
        cleanup();
        return false;
    }
    if (!SDL_ResumeAudioStreamDevice(stream_)) {
        fail_sdl("SDL_ResumeAudioStreamDevice");
        cleanup();
        return false;
    }
    paused_ = false;
    if (!consume_removal()) {
        cleanup();
        return false;
    }
    state_ = State::ready;
    return true;
}

SdlAudioOutput::State SdlAudioOutput::state() noexcept {
    if (watching_) consume_removal();
    return state_;
}

const SdlAudioOutput::Error& SdlAudioOutput::error() noexcept {
    if (watching_) consume_removal();
    return error_;
}

bool SdlAudioOutput::require_stream(const char* operation) noexcept {
    if (!consume_removal()) return false;
    if (!stream_ || state_ == State::closed) {
        return fail(ErrorKind::invalid_state, operation, "An open nonfailed stream is required");
    }
    return true;
}

bool SdlAudioOutput::queued_frames(std::size_t& frames) noexcept {
    const int bytes = SDL_GetAudioStreamQueued(stream_);
    if (bytes < 0) return fail_sdl("SDL_GetAudioStreamQueued");
    const auto source_bytes = static_cast<std::size_t>(bytes);
    if (source_bytes % source_frame_bytes != 0) {
        return fail(ErrorKind::inconsistent_queue, "SDL_GetAudioStreamQueued", "Input byte count is not aligned to a four-byte source frame");
    }
    frames = source_bytes / source_frame_bytes;
    if (frames > info_.max_queued_source_frames) {
        return fail(ErrorKind::inconsistent_queue, "SDL_GetAudioStreamQueued", "Input occupancy exceeds the configured bound");
    }
    return true;
}

SdlAudioOutput::Queue SdlAudioOutput::query() noexcept {
    if (!require_stream("query")) return {};
    std::size_t queued = 0;
    if (!queued_frames(queued)) return {};
    const int available = SDL_GetAudioStreamAvailable(stream_);
    if (available < 0) {
        fail_sdl("SDL_GetAudioStreamAvailable");
        return {};
    }
    if (!consume_removal()) return {};
    return {true, queued, available, accepted_source_frames_, paused_};
}

SdlAudioOutput::Submit SdlAudioOutput::try_submit(std::span<const std::int16_t> samples) noexcept {
    if (!require_stream("try_submit")) return Submit::error;
    if (state_ != State::ready) {
        fail(ErrorKind::invalid_state, "try_submit", "Input was finished; no further puts are permitted");
        return Submit::error;
    }
    const std::size_t frames = samples.size() / 2;
    if (samples.empty() || samples.size() % 2 || frames > info_.max_queued_source_frames) {
        fail(ErrorKind::invalid_argument, "try_submit", "Nonempty whole stereo frames fitting the entire configured bound are required");
        return Submit::error;
    }
    if (frames > std::numeric_limits<std::uint64_t>::max() - accepted_source_frames_) {
        fail(ErrorKind::frame_counter_overflow, "try_submit", "Accepted source-frame counter would overflow");
        return Submit::error;
    }
    std::size_t queued = 0;
    if (!queued_frames(queued) || !consume_removal()) return Submit::error;
    if (frames > info_.max_queued_source_frames - queued) return Submit::would_block;
    // A single writer and asynchronous consumption make admission conservative.
    // The bound above also guarantees SDL's signed byte length cannot overflow.
    if (!SDL_PutAudioStreamData(stream_, samples.data(), static_cast<int>(frames * source_frame_bytes))) {
        fail_sdl("SDL_PutAudioStreamData");
        return Submit::error;
    }
    accepted_source_frames_ += frames;
    // Copy success remains success even if removal races afterward. The next
    // owner operation consumes the latched endpoint failure; it is not hearing.
    return Submit::accepted;
}

bool SdlAudioOutput::pause() noexcept {
    if (!require_stream("pause")) return false;
    if (!paused_) {
        if (!SDL_PauseAudioStreamDevice(stream_)) return fail_sdl("SDL_PauseAudioStreamDevice");
        paused_ = true;
    }
    return consume_removal();
}

bool SdlAudioOutput::resume() noexcept {
    if (!require_stream("resume")) return false;
    if (paused_) {
        if (!SDL_ResumeAudioStreamDevice(stream_)) return fail_sdl("SDL_ResumeAudioStreamDevice");
        paused_ = false;
    }
    return consume_removal();
}

bool SdlAudioOutput::finish_input() noexcept {
    if (!require_stream("finish_input")) return false;
    if (state_ == State::draining || state_ == State::stream_drained) return true;
    if (!SDL_FlushAudioStream(stream_)) return fail_sdl("SDL_FlushAudioStream");
    state_ = State::draining;
    return consume_removal();
}

SdlAudioOutput::Drain SdlAudioOutput::poll_drain() noexcept {
    if (!require_stream("poll_drain")) return Drain::error;
    if (state_ != State::draining && state_ != State::stream_drained) {
        fail(ErrorKind::invalid_state, "poll_drain", "Finish input before polling stream drain");
        return Drain::error;
    }
    const Queue queue = query();
    if (!queue) return Drain::error;
    if (queue.queued_source_frames || queue.available_output_bytes) return Drain::pending;
    state_ = State::stream_drained;
    return Drain::stream_drained;
}

SdlAudioOutput::CloseResult SdlAudioOutput::close() noexcept {
    if (state_ == State::closed) return close_result_;
    if (watching_) consume_removal();
    const bool had_stream = stream_ != nullptr;
    const bool drained = state_ == State::stream_drained;
    cleanup();
    close_result_.disposition = !had_stream ? CloseDisposition::not_open :
        drained && error_.kind == ErrorKind::none ? CloseDisposition::stream_drained : CloseDisposition::aborted;
    close_result_.failed = error_.kind != ErrorKind::none;
    state_ = State::closed;
    return close_result_;
}
} // namespace isle::audio
