#include "35mmisle/audio_transport.hpp"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_timer.h>

#include <array>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
using Output = isle::audio::SdlAudioOutput;
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
void opened(Output& output, SDL_AudioDeviceID physical = 0) {
    if (!output.open({32006, 256, physical})) {
        throw std::runtime_error(std::string(output.error().operation.data()) + ": " + output.error().message.data());
    }
    require(output.info().logical_device != 0 &&
            !SDL_IsAudioDevicePhysical(output.info().logical_device), "open must return a real logical SDL handle");
}
template<class Predicate>
void until(Predicate predicate, const char* message) {
    const Uint64 deadline = SDL_GetTicks() + 5000;
    do {
        SDL_PumpEvents();
        if (predicate()) return;
        SDL_Delay(2);
    } while (SDL_GetTicks() < deadline);
    throw std::runtime_error(message);
}
bool named(SDL_AudioDeviceID device, const std::string& expected) {
    const char* name = SDL_GetAudioDeviceName(device);
    return name && expected == name;
}
SDL_AudioDeviceID first_playback(bool multiple) {
    int count = 0;
    SDL_AudioDeviceID* devices = SDL_GetAudioPlaybackDevices(&count);
    const SDL_AudioDeviceID first = devices && count > 0 ? devices[0] : 0;
    SDL_free(devices);
    require(first != 0 && (!multiple || count >= 2), "test requires enumerated playback endpoints");
    return first;
}
void submit_and_consume(Output& output) {
    std::array<std::int16_t, 512> pcm{};
    for (std::size_t i = 0; i < pcm.size(); ++i) pcm[i] = (i % 32 < 16) ? 1000 : -1000;
    require(output.pause(), "pause actual logical consumer");
    require(output.try_submit(pcm) == Output::Submit::accepted, "consumer accepts a complete bounded PCM block");
    const auto queued = output.query();
    require(queued && queued.paused && queued.queued_source_frames == 256, "paused consumer retains source-frame occupancy");
    require(output.try_submit(pcm) == Output::Submit::would_block, "full queue must backpressure without copying another block");
    require(output.resume(), "resume actual logical consumer");
    // Flush is intentionally deferred: SDL resampling can retain a short tail.
}
void finish(Output& output, std::uint64_t accepted) {
    require(output.finish_input() && output.finish_input(), "flush is idempotent");
    until([&] { return output.poll_drain() == Output::Drain::stream_drained; }, "actual SDL stream did not drain within five seconds");
    require(output.query().accepted_source_frames == accepted, "backpressure preserves the exact accepted-frame counter");
    const auto closed = output.close();
    require(!closed.failed && closed.disposition == Output::CloseDisposition::stream_drained, "observed stream drain closes without abort");
    require(output.close().disposition == closed.disposition, "close is idempotent");
    require((SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO) != 0, "transport closes only its own SDL audio reference");
}
void dummy() {
    Output output;
    opened(output);
    const auto logical = output.info().logical_device;
    submit_and_consume(output);
    finish(output, 256);

    // SDL accepts logical IDs in its open API, but this transport's explicit
    // endpoint contract accepts only currently enumerated physical playback IDs.
    opened(output, first_playback(false));
    Output rejected;
    require(!rejected.open({32006, 256, output.info().logical_device}) &&
            rejected.error().kind == Output::ErrorKind::unavailable_device &&
            rejected.info().logical_device == 0, "explicit logical ID is rejected, never redirected to a default");
    rejected.close();
    require(!rejected.open({32006, 256, logical}) &&
            rejected.error().kind == Output::ErrorKind::unavailable_device,
            "closed/stale logical ID must fail without default fallback");
    rejected.close();
    require(!rejected.open({32006, 256, SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK}) &&
            rejected.error().kind == Output::ErrorKind::unavailable_device,
            "nonzero explicit endpoint field cannot smuggle in a default sentinel");
    rejected.close();
    require(!rejected.open({32006, 256, 0x12345678u}) &&
            rejected.error().kind == Output::ErrorKind::unavailable_device,
            "missing explicit endpoint fails closed despite an available default");
    rejected.close();
    submit_and_consume(output);
    finish(output, 256);
}
std::string command() {
    std::string line;
    require(bool(std::getline(std::cin, line)) && !line.empty(), "isolated routing controller closed before completing the protocol");
    return line;
}
void routing() {
    // An external private-daemon fixture owns all backend configuration. The
    // first endpoint is discovered, never assumed from PulseAudio list order.
    // No synthetic SDL events or substituted audio API are used in this mode.
    require(std::string_view(SDL_GetCurrentAudioDriver()) == "pulseaudio", "routing test requires the isolated PulseAudio backend");
    const auto physical = first_playback(true);
    const char* name = SDL_GetAudioDeviceName(physical);
    require(name != nullptr, "enumerated endpoint has a name");
    const std::string first_name(name);
    std::cout << "FIRST\t" << first_name << std::endl;
    const std::string target_name = command();
    require(target_name != first_name, "fixture must choose a default different from SDL's first enumerated endpoint");
    until([&] { return named(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, target_name); }, "fixture's non-first default did not reach SDL");

    Output automatic, pinned;
    opened(automatic);
    opened(pinned, physical);
    const auto automatic_id = automatic.info().logical_device;
    const auto pinned_id = pinned.info().logical_device;
    require(named(automatic_id, target_name) && named(pinned_id, first_name),
            "ordinary consumer must open the actual non-first default while explicit consumer stays pinned");
    submit_and_consume(automatic);
    std::cout << "OPEN\t" << automatic_id << '\t' << pinned_id << std::endl;

    // Controller observes actual backend sink-input routing, changes its private
    // default to FIRST, then sends MIGRATE. Logical handles must stay stable.
    require(command() == "MIGRATE", "expected private default-change phase");
    until([&] { return named(automatic_id, first_name); }, "default logical consumer did not migrate to the changed default");
    require(automatic.info().logical_device == automatic_id &&
            automatic.state() == Output::State::ready && named(pinned_id, first_name),
            "default migration preserves logical ownership and explicit pinning");
    std::cout << "MIGRATED" << std::endl;

    // Controller removes FIRST: PulseAudio returns to the surviving target.
    // Real SDL physical/logical removal events must fail only the pinned run.
    require(command() == "REMOVE", "expected private endpoint-removal phase");
    until([&] {
        return named(automatic_id, target_name) && pinned.state() == Output::State::failed;
    }, "default did not migrate off removed hardware or explicit consumer did not fail");
    require(automatic.state() == Output::State::ready &&
            pinned.error().kind == Output::ErrorKind::disconnected,
            "physical removal must not kill migratable default playback or redirect the explicit consumer");
    const auto rejected_pcm = std::array<std::int16_t, 2>{1, -1};
    require(pinned.try_submit(rejected_pcm) == Output::Submit::error, "removed explicit endpoint cannot silently accept redirected PCM");
    const auto closed = pinned.close();
    require(closed.failed && closed.disposition == Output::CloseDisposition::aborted, "removed explicit consumer retains failure through close");
    finish(automatic, 256);
    // Reopening the removed physical endpoint must still not choose the survivor.
    require(!pinned.open({32006, 256, physical}) &&
            pinned.error().kind == Output::ErrorKind::unavailable_device,
            "removed explicit endpoint cannot reopen through fallback");
    pinned.close();
    std::cout << "ROUTING_PASS" << std::endl;
}
struct SdlLifetime {
    ~SdlLifetime() { SDL_Quit(); }
};
} // namespace

int main(int argc, char** argv) {
    try {
        const bool route = argc == 2 && std::string_view(argv[1]) == "--routing";
        require(argc == 1 || route, "usage: audio-transport-tests [--routing]");
        if (!route) require(SDL_SetHintWithPriority(SDL_HINT_AUDIO_DRIVER, "dummy", SDL_HINT_OVERRIDE), "select isolated dummy backend");
        SdlLifetime lifetime;
        require(SDL_Init(SDL_INIT_AUDIO | SDL_INIT_EVENTS), SDL_GetError());
        if (route) routing(); else dummy();
        std::cout << "audio transport tests passed (software endpoints; no physical Windows qualification)\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "audio transport test failure: " << error.what() << '\n';
        return 1;
    }
}
