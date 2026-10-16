#include "game_audio.hpp"
#include "actual_audio_driver.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace g = isle::game_audio;
namespace a = isle::audio;
namespace {
void require(bool value, const char* text) { if (!value) throw std::runtime_error(text); }
void require(a::Error error, const char* text) { require(error == a::Error::none, text); }
std::array<a::effects::PoleFilter, 3> poles(const std::filesystem::path& path) {
    std::array<std::byte, 114> wire{};
    std::ifstream in(path, std::ios::binary);
    require(bool(in.read(reinterpret_cast<char*>(wire.data()), wire.size())), "private three-book numeric input is truncated");
    require(in.peek() == std::char_traits<char>::eof(), "private numeric input has trailing bytes");
    std::array<a::effects::PoleFilter, 3> result{};
    require(g::decode_startup_poles(wire, result), "private numeric input has wrong parameter identity/order");
    return result;
}
struct Run {
    std::vector<std::int16_t> samples;
    std::vector<std::uint32_t> quotas;
    std::uint64_t tasks = 0, reads = 0, writes = 0;
};
Run run(const isle::rom::Snapshot& snapshot, const g::Resources& resources, a::effects::Config effects,
        unsigned isolated_mute, bool controls) {
    isle::private_audio::Capture capture;
    isle::private_audio::Execution execution{snapshot, nullptr, {}};
    auto made = g::Session::create(resources, effects, capture.output(), execution.executor());
    require(bool(made), "startup resource/three-calendar/effects/output initialization failed");
    auto& session = *made.session;
    auto words = session.service().rdram_words();
    const auto* stable = words.data();
    Run result;
    result.quotas.reserve(512);
    int rain = -1;
    require(session.bgm_volume(0, isolated_mute == 1 ? 0 : 0x7f00), "intro player0 volume failed");
    require(session.bgm_volume(1, isolated_mute == 2 ? 0 : 0x1000), "intro player1 volume failed");
    require(session.play_song(0, 0x19), "actual intro song19 request failed");
    require(session.play_song(1, 0x14), "actual intro song14 request failed");
    require(session.play_sound(0x2c, {static_cast<std::uint16_t>(isolated_mute == 3 ? 0 : 0x2000), 64, 30, 1.0f}, rain), "actual intro SFX2C request failed");
    require(rain >= 0, "actual SFX2C has no logical sound slot");
    auto execute = [&](const a::TaskView& built) {
        const auto actual = isle::private_audio::execute(session,execution,built);
        ++result.tasks; result.reads += actual.dma_reads; result.writes += actual.dma_writes;
        result.quotas.push_back(built.frame_count);
        require(session.submit(), "owned actual PCM capture acceptance failed");
        require(session.service().has_lease(), "source output gate did not retain the actual completed PCM owner");
    };
    // Source builds once before waiting for its first queued scheduler tick.
    a::TaskView initial; require(session.build(initial), "initial source build failed");
    require(initial.token.serial != 0 && initial.frame_count == 640, "source-derived bootstrap quota was not640");
    execute(initial);
    require(capture.used == 0 && session.accepted_frames() == 0 && session.native_ai_frames() == 0,
            "source bootstrap invented accepted PCM or AI occupancy before first retrace");
    bool requested_stop = false;
    bool reset_requested = false, reset_complete = !controls;
    std::uint64_t stop_tick = controls ? 0 : 210;
    for (std::uint64_t tick = 1; tick <= 500; ++tick) {
        require(session.retrace(tick, tick * 1000000000ull / 60), "independent source scheduler retrace failed");
        require(session.submit(), "source-gated previous PCM capture acceptance failed");
        if (tick == 1) require(capture.used == 0 && session.native_ai_frames() == 0,
                               "first source retrace did not preserve zero-count bootstrap buffer0");
        if (tick == 2) require(capture.used == initial.frame_count * 2 && !session.service().has_lease(),
                               "first actual PCM was not accepted exactly at second source retrace");
        if (controls && tick == 50) {
            require(session.bgm_volume(1, 0x6400, 60), "source smooth player1 volume failed");
            require(session.sound_pan(static_cast<unsigned>(rain), 20), "source live SFX pan failed");
            require(session.sound_volume(static_cast<unsigned>(rain), 0x4800), "source live SFX volume failed");
            require(session.sound_priority(static_cast<unsigned>(rain), 120), "source live SFX priority failed");
        }
        if (tick == 120) {
            require(session.stop_bgm(), "intro BGM stop failed");
            require(session.stop_sound(static_cast<unsigned>(rain)), "intro SFX stop failed");
            require(session.bgm_volume(0, 0x7f00), "menu player0 volume failed");
            require(session.bgm_volume(1, 0x7f00), "menu player1 volume failed");
            require(session.play_song(0, 0x17), "actual menu song17 request failed");
            int click = -1;
            require(session.play_sound(0x42, {}, click), "actual menu SFX42 request failed");
            require(click >= 0, "actual menu SFX42 has no slot");
        }
        if (controls && tick == 150) {
            std::uint32_t tempo = 0;
            require(session.get_bgm_tempo(0, tempo), "actual menu processed tempo query failed");
            require(tempo != 0, "actual menu sequence never bound");
            require(session.bgm_tempo(0, tempo + tempo / 4), "actual menu tempo edit failed");
            require(session.bgm_reverb(0, 50), "actual menu reverb edit failed");
            require(session.bgm_channel_volume(0, 0, 70), "actual menu channel volume edit failed");
        }
        if (controls && tick == 170) {
            require(session.restart(effects), "actual GAME effects/player recreation request failed");
            require(!session.initialization_ready(), "pending recreation fabricated initialization ready");
            reset_requested = true;
        }
        if (stop_tick != 0 && tick == stop_tick) { require(session.stop(), "source audio stop failed"); requested_stop = true; }
        a::TaskView built;
        require(session.build(built), "source GAME audio service build failed");
        if (built.token.serial != 0) execute(built);
        if (reset_requested && !reset_complete && session.initialization_ready()) {
            require(session.service().rdram_words().data() == stable, "actual effects recreation replaced RSP-borrowed allocation");
            require(session.bgm_volume(0, 0x7f00), "recreated real player volume failed");
            require(session.play_song(0, 0x17), "recreated actual menu sequence failed");
            int sound = -1;
            require(session.play_sound(0x43, {0x6000, 110, 30, 0.75f}, sound), "recreated actual SFX player failed");
            require(sound >= 0, "recreated actual SFX player has no owned slot");
            reset_complete = true;
            stop_tick = tick + 60;
        }
        if (session.state() == g::Session::State::draining) {
            require(requested_stop && session.service().voices_retired(), "drain began before actual source clients/physical/task/PCM owners retired");
            require(session.poll_drain(), "first checked capture drain poll failed");
            require(session.state() == g::Session::State::draining, "pending checked consumer was fabricated complete");
            require(session.poll_drain(), "second checked capture drain poll failed");
            break;
        }
    }
    require(session.state() == g::Session::State::drained && capture.finishes == 1, "bounded actual audio stop did not cleanly retire and drain");
    require(reset_complete, "bounded actual effects/client recreation never retired its preceding graph");
    require(session.service().voices_retired() && !session.service().has_lease() && session.service().rdram_words().data() == stable,
        "actual audio graph leaked an owner or replaced borrowed stable RDRAM");
    require(session.accepted_frames() * 2 == capture.used, "checked output acceptance differs from retained PCM frame count");
    capture.samples.resize(capture.used); result.samples = std::move(capture.samples);
    return result;
}
std::size_t differences(const Run& first, const Run& second) {
    require(first.quotas == second.quotas && first.samples.size() == second.samples.size(), "controlled audio comparisons changed source quota/time domain");
    std::size_t count = 0;
    for (std::size_t i = 0; i < first.samples.size(); ++i) count += first.samples[i] != second.samples[i];
    return count;
}
std::size_t differences_before_stop(const Run& first, const Run& second) {
    constexpr std::size_t tasks = 160; // Before intentional real graph recreation.
    require(first.quotas.size() >= tasks && second.quotas.size() >= tasks &&
        std::equal(first.quotas.begin(), first.quotas.begin() + tasks, second.quotas.begin()),
        "live control comparisons changed pre-stop source quota/time domain");
    std::size_t samples = 0;
    for (std::size_t i = 0; i < tasks; ++i) samples += first.quotas[i] * 2u;
    require(first.samples.size() >= samples && second.samples.size() >= samples, "checked capture lost pre-stop PCM");
    std::size_t count = 0;
    for (std::size_t i = 0; i < samples; ++i) count += first.samples[i] != second.samples[i];
    return count;
}
}
#ifdef _WIN32
int wmain(int argc, wchar_t* argv[]) {
#else
int main(int argc, char* argv[]) {
#endif
    try {
        require(argc == 3, "usage: game-audio-rsp-smoke PRIVATE_ROM PRIVATE_114_BYTE_STARTUP_POLE_INPUT");
        const auto snapshot = isle::rom::read_snapshot(std::filesystem::path(argv[1]));
        auto hydrated = g::hydrate(snapshot); require(bool(hydrated), "actual ROM startup audio hydration failed");
        const auto numeric = poles(std::filesystem::path(argv[2]));
        std::array<a::effects::Section, 8> sections;
        a::effects::Config effects;
        require(g::native_startup_effects(hydrated.resources, sections, effects), "native GAME default effects configuration failed");
        std::array<bool,3> compared{};
        for (std::size_t i = 0; i < sections.size(); ++i) {
            if (!sections[i].pole) continue;
            const auto parameter = hydrated.resources.startup_effect_words[9+i*8];
            const auto index = static_cast<std::size_t>((parameter-0x5000)/0x1000);
            const auto& generated = *sections[i].pole;
            require(generated.gain == numeric[index].gain &&
                    generated.coefficients == numeric[index].coefficients,
                    "native mathematical pole expansion differs from independent numeric observation");
            compared[index] = true;
        }
        require(std::all_of(compared.begin(),compared.end(),[](bool value) { return value; }),
                "default graph did not compare every reached native pole input");
        const auto full = run(snapshot, hydrated.resources, effects, 0, false);
        const auto repeat = run(snapshot, hydrated.resources, effects, 0, false);
        require(differences(full, repeat) == 0, "restored actual startup/menu run is not deterministic");
        const auto no19 = run(snapshot, hydrated.resources, effects, 1, false);
        const auto no14 = run(snapshot, hydrated.resources, effects, 2, false);
        const auto no2c = run(snapshot, hydrated.resources, effects, 3, false);
        const auto changed = run(snapshot, hydrated.resources, effects, 0, true);
        const auto song19 = differences(full, no19), song14 = differences(full, no14), sfx2c = differences(full, no2c),
            controls = differences_before_stop(full, changed);
        require(song19 != 0 && song14 != 0 && sfx2c != 0 && controls != 0,
                "actual commands did not independently change both BGM contexts, SFX and control-produced audio");
        std::cout << "actual_startup_menu_audio=passed baseline_tasks=" << full.tasks << " runs=6 baseline_frames=" << full.samples.size() / 2
            << " changed_samples_song19=" << song19 << " song14=" << song14 << " SFX2C=" << sfx2c << " live_controls=" << controls
            << " restored_repeat=exact native_parameter_factory=private_observations_equal"
               " all_task_PCM_physical_owners=retired capture=checked_owned"
               " physical_device_probes=none W12_numerical_Original_or_hardware_parity=not_claimed\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& e) { std::cerr << "game-audio-rsp-smoke: " << e.what() << '\n'; return EXIT_FAILURE; }
}
