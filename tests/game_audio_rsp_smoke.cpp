#include "game_audio.hpp"
#include "actual_audio_driver.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>
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

constexpr std::uint32_t silent_variant = 0x16;

struct FirstNote {
    a::SequenceEvent event{};
    unsigned program = 0, volume = 127;
};

void require_decoded(const a::SequenceReadResult& result) {
    if (!result)
        throw std::runtime_error("compact sequence decode error=" + std::to_string(static_cast<int>(result.error)) +
            " track=" + std::to_string(result.track) + " offset=" + std::to_string(result.offset));
}

FirstNote first_note(const a::SequenceEntry& entry, std::size_t& tracks) {
    a::SequenceReader reader;
    require_decoded(reader.start(entry));
    // start() has validated the 16 big-endian track offsets. Count present
    // tracks separately from positive Note Ons (EOT can exist without either).
    for (std::size_t i = 0; i < a::SequenceReader::track_count; ++i) {
        const auto offset = entry.encoded.subspan(i * 4, 4);
        tracks += std::any_of(offset.begin(), offset.end(), [](std::byte byte) { return byte != std::byte{}; });
    }
    require(tracks != 0, "empty authored track set");
    std::array<unsigned, 16> programs{}, volumes;
    volumes.fill(127);
    for (unsigned i = 0; i < 4096; ++i) {
        a::SequenceEvent event;
        a::SequenceReadResult decoded;
        const auto step = reader.next(event, decoded);
        require_decoded(decoded);
        if (step == a::SequenceStep::end || event.kind == a::SequenceEventKind::sequence_end)
            throw std::runtime_error("authored tracks ended without a positive Note On");
        if (event.kind != a::SequenceEventKind::midi) continue;
        const std::size_t channel = event.status & 0x0fu;
        switch (event.status & 0xf0) {
        case 0xc0: programs[channel] = event.data1; break;
        case 0xb0: if (event.data1 == 7) volumes[channel] = event.data2; break;
        case 0x90: if (event.data2 != 0) return {event, programs[channel], volumes[channel]}; break;
        }
    }
    throw std::runtime_error("no positive Note On in bounded authored event prefix");
}

struct SongWindow {
    std::size_t tracks = 0, starts = 0, nonzero = 0;
    std::uint64_t tasks = 0, frames = 0;
    unsigned peak = 0;
    FirstNote note{};
};

void song_window(const isle::rom::Snapshot& snapshot, const g::Resources& resources, a::effects::Config effects,
                 isle::private_audio::Capture& capture, std::uint32_t song, SongWindow& result) {
    constexpr unsigned quiet_ticks = 4, play_ticks = 120, retire_ticks = 60;
    const bool expect_silence = song == silent_variant;
    result.note = first_note(resources.sequences.sequences()[song], result.tracks);
    if (expect_silence) {
        // Pinned US GAME S1 entry 0x16 is not empty: its single track authors
        // this sparse looped note. CC7=10 and velocity=1 quantize its linear
        // gain to zero even at full GAME master volume.
        const auto& note = result.note.event;
        require(result.tracks == 1 && note.track == 0 && note.status == 0x90 && note.tick == 60 &&
                note.data1 == 60 && note.data2 == 1 && note.duration_ticks == 10 &&
                result.note.program == 75 && result.note.volume == 10, "authored silent variant identity changed");
    }

    capture.used = capture.finishes = capture.polls = 0;
    capture.state = g::Output::State::ready;
    isle::private_audio::Execution execution{snapshot, nullptr, {}};
    auto made = g::Session::create(resources, effects, capture.output(), execution.executor());
    require(bool(made), "fresh catalog Session/executor initialization failed");
    auto& session = *made.session;
    const auto* stable = session.service().rdram_words().data();
    require(session.has_song(song), "catalog song rejected by GAME");
    require(!session.bgm_playing(0) && !session.bgm_playing(1) && session.service().voices_retired(),
            "fresh graph was not quiet");
    const auto build = [&] {
        a::TaskView task;
        const auto error = session.build(task);
        if (error != a::Error::none) {
            if (task.token.serial != 0) session.complete(task.token, false);
            throw std::runtime_error("GAME task build error=" + std::to_string(static_cast<int>(error)));
        }
        if (task.token.serial == 0) return;
        require(task.token.serial == result.tasks + 1 && task.service_id == session.source_frame_epoch() &&
                task.synthesis_rate == g::source_rate && task.frame_count != 0 &&
                task.frame_count <= a::AudioService::max_frames, "invalid task/frame accounting");
        require(task.command_address == a::AudioService::command_address && task.command_bytes != 0 &&
                task.command_bytes % 8 == 0 &&
                task.command_bytes <= a::AudioService::command_limit - task.command_address,
                "invalid command extent");
        const auto words = session.service().rdram_words();
        for (auto at = task.command_address / 4; at < (task.command_address + task.command_bytes) / 4; at += 2) {
            const auto op = words[at] & 0xffff0000u;
            const auto state = words[at + 1];
            // A_INIT RESAMPLE to a physical voice's history, not an FX DMA or
            // just a requested/bound song. SFX and the other CSP stay idle.
            if (op == 0x05010000u && state >= a::AudioService::state_address &&
                state < a::AudioService::state_address + a::AudioService::max_physical_voices * a::AudioService::state_stride &&
                (state - a::AudioService::state_address) % a::AudioService::state_stride == 0x40)
                ++result.starts;
        }
        const auto actual = isle::private_audio::execute(session, execution, task);
        require(actual.dma_reads != 0 && actual.dma_writes != 0 && session.service().has_lease(),
                "real RSP task did not retain a completed DMA/PCM owner");
        ++result.tasks;
        result.frames += task.frame_count;
    };
    std::uint64_t tick = 0;
    const auto pump = [&] {
        ++tick;
        require(session.retrace(tick, tick * 1000000000ull / 60), "catalog source retrace failed");
        require(session.submit(), "catalog checked capture rejected PCM");
        build();
    };
    build();
    for (unsigned i = 0; i < quiet_ticks; ++i) pump();
    const auto written = [&] { return std::span<const std::int16_t>(capture.samples).first(capture.used); };
    require(capture.used != 0 && result.starts == 0 &&
            std::all_of(written().begin(), written().end(), [](auto value) { return value == 0; }),
            "quiet preroll produced a physical note or nonzero PCM");
    const auto quiet_samples = capture.used;
    require(session.bgm_volume(0, 0x7f00), "catalog main CSP volume failed");
    require(session.play_song(0, song), "catalog GAME song request failed");
    for (unsigned i = 0; i < play_ticks; ++i) pump();
    const auto playback = written().subspan(quiet_samples);
    require(!playback.empty(), "catalog playback window had no accepted PCM");
    // Only this cue was requested on the fresh graph, and its captured preroll was all zero.
    result.nonzero = static_cast<std::size_t>(std::count_if(playback.begin(), playback.end(), [](auto value) { return value != 0; }));
    const auto play_starts = result.starts;
    std::uint32_t tempo = 0;
    require(session.get_bgm_tempo(0, tempo), "catalog main CSP tempo query failed");
    require(!session.bgm_playing(1), "unexpected auxiliary CSP activity");

    require(session.stop(), "catalog GAME stop failed");
    for (unsigned i = 0; i < retire_ticks && session.state() == g::Session::State::stopping; ++i) pump();
    require(session.state() == g::Session::State::draining && session.service().voices_retired() &&
            !session.service().has_lease() && !session.bgm_playing(0) && !session.bgm_playing(1),
            "catalog stop did not retire song/voice/task/PCM owners within its bound");
    require(session.poll_drain(), "catalog first checked drain poll failed");
    require(session.state() == g::Session::State::draining, "catalog drain completed before consumer acknowledgment");
    require(session.poll_drain(), "catalog second checked drain poll failed");
    require(session.state() == g::Session::State::drained && capture.state == g::Output::State::drained &&
            capture.finishes == 1 && capture.polls == 2 && session.accepted_frames() == result.frames &&
            capture.used == result.frames * 2 && session.source_frame_epoch() == result.tasks &&
            session.service().rdram_words().data() == stable && execution.borrowed == stable,
            "catalog drain/capture/frame or stable-owner accounting differs");
    made.session.reset();
    require(execution.borrowed == nullptr && !execution.machine, "catalog Session did not release its RSP borrower");

    require(tempo != 0, "authored note exists but GAME never bound the song");
    require(play_starts != 0, "authored note exists but no physical note was scheduled in the playback window");
    if (expect_silence) {
        for (const auto value : written().subspan(quiet_samples))
            result.peak = std::max(result.peak, static_cast<unsigned>(std::abs(static_cast<int>(value))));
        // Allow one LSB of envelope/FX rounding residue in the captured output.
        require(result.peak <= 1, "authored silent variant produced audible output");
    } else {
        require(result.nonzero != 0, "scheduled notes made no nonzero PCM contribution in the playback window");
    }
}

int all_songs(const isle::rom::Snapshot& snapshot, const g::Resources& resources) {
    const auto count = resources.sequences.sequences().size();
    require(count == 37, "pinned US GAME song catalog did not admit all 37 entries");
    std::array<a::effects::Section, 8> sections;
    a::effects::Config effects;
    require(g::native_startup_effects(resources, sections, effects), "native GAME default effects configuration failed");
    isle::private_audio::Capture capture;
    unsigned attempted = 0, active = 0, silent = 0;
    std::uint64_t tasks = 0, frames = 0;
    for (std::uint32_t song = 0; song < count; ++song) {
        ++attempted;
        SongWindow result;
        try {
            song_window(snapshot, resources, effects, capture, song, result);
            if (song == silent_variant) ++silent;
            else ++active;
        } catch (const std::exception& failure) {
            std::cerr << "game-audio-rsp-smoke: song=0x" << std::hex << song << std::dec << " " << failure.what()
                << " authored_tracks=" << result.tracks << " tasks=" << result.tasks
                << " physical_starts=" << result.starts << " nonzero_samples=" << result.nonzero;
            const auto& note = result.note.event;
            if (note.data2 != 0)
                std::cerr << " first_note_tick=" << note.tick << " track=" << unsigned(note.track)
                    << " channel=" << (note.status & 15) << " program=" << result.note.program
                    << " key=" << unsigned(note.data1) << " velocity=" << unsigned(note.data2);
            if (song == silent_variant) std::cerr << " peak=" << result.peak;
            std::cerr << '\n';
        }
        tasks += result.tasks;
        frames += result.frames;
    }
    const bool passed = silent == 1 && active + silent == attempted;
    std::cout << "actual_all_songs_audio=" << (passed ? "passed" : "failed")
        << " attempted=" << attempted << " active=" << active << " authored_silent=" << silent
        << " authored_silent_song=0x" << std::hex << silent_variant << std::dec
        << " anomalies=" << attempted - active - silent << " tasks=" << tasks << " frames=" << frames
        << " capture=checked_owned parity=not_claimed\n";
    return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
}
#ifdef _WIN32
int wmain(int argc, wchar_t* argv[]) {
#else
int main(int argc, char* argv[]) {
#endif
    try {
        require(argc == 3, "usage: game-audio-rsp-smoke PRIVATE_ROM {PRIVATE_114_BYTE_STARTUP_POLE_INPUT|--all-songs}");
        const auto snapshot = isle::rom::read_snapshot(std::filesystem::path(argv[1]));
        auto hydrated = g::hydrate(snapshot); require(bool(hydrated), "actual ROM startup audio hydration failed");
        if (std::filesystem::path(argv[2]) == "--all-songs") return all_songs(snapshot, hydrated.resources);
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
