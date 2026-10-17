#include "item_flute.hpp"
#include "actual_audio_driver.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace a = isle::audio;
namespace g = isle::game_audio;
namespace items = isle::items;
namespace {
void require(bool value, const char* text) { if (!value) throw std::runtime_error(text); }
void require(a::Error value, const char* text) { require(value == a::Error::none, text); }
struct Run {
    std::vector<std::int16_t> samples;
    std::vector<std::uint32_t> quotas;
    std::size_t first_stop = 0, restart_begin = 0, restart_end = 0;
    std::uint64_t tasks = 0, reads = 0, writes = 0;
};
// Each reference run directly requests one specified real song. It is not a
// second flute implementation: the production wrapper alone selects/cycles
// source state, while independently requested PCM proves its selected asset.
Run run(const isle::rom::Snapshot& snapshot, const g::Resources& resources,
        a::effects::Config effects, unsigned index, bool reference, bool muted) {
    constexpr std::array<std::uint32_t, 3> songs{1, 3, 2};
    constexpr std::array<std::int32_t, 3> commands{5, 6, 7};
    require(index < songs.size(), "private flute song index outside source domain");
    isle::private_audio::Capture capture;
    isle::private_audio::Execution execution{snapshot, nullptr, {}};
    auto made = g::Session::create(resources, effects, capture.output(), execution.executor());
    require(bool(made), "actual flute resources/executor/owned PCM consumer initialization failed");
    auto& session = *made.session;
    const auto* stable = session.service().rdram_words().data();
    items::FluteState retained;
    items::Flute flute(session, retained);
    require(session.source_frame_epoch() == 0, "fresh session fabricated an audio-frame epoch");
    require(session.bgm_volume(0, muted ? 0 : 0x7f00), "actual flute player volume admission failed");
    if (reference) {
        require(session.stop_song(0), "independent main0 stop request failed");
        require(session.play_song(0, songs[index]), "independent specified song request failed");
    } else {
        for (unsigned selection = 0; selection <= index; ++selection) require(flute.play(), "actual flute selection failed");
        require(retained.song_index == static_cast<std::int32_t>(index) && retained.playing &&
                flute.command() == commands[index] && flute.state() == -1 && session.bgm_playing(0),
                "source flute cycle/pending request/same-epoch contract failed before actual execution");
    }
    Run result;
    result.quotas.reserve(128);
    const auto execute = [&](const a::TaskView& built) {
        require(built.service_id == session.source_frame_epoch(), "actual task is not the published source-frame epoch");
        const auto epoch = session.source_frame_epoch();
        const auto actual = isle::private_audio::execute(session, execution, built);
        ++result.tasks; result.reads += actual.dma_reads; result.writes += actual.dma_writes;
        result.quotas.push_back(built.frame_count);
        require(session.source_frame_epoch() == epoch, "actual RSP completion advanced the source-frame epoch");
        require(session.submit(), "actual flute PCM acceptance failed");
        require(session.source_frame_epoch() == epoch && session.service().has_lease(),
                "completed actual PCM did not retain its source-gated lease/epoch");
        a::TaskView absent;
        require(session.build(absent), "leased actual audio no-work build failed");
        require(absent.token.serial == 0 && session.source_frame_epoch() == epoch,
                "no-work build fabricated a source epoch or recycled actual borrowed PCM");
    };
    a::TaskView initial;
    require(session.build(initial), "actual flute bootstrap build failed");
    require(initial.token.serial != 0 && session.source_frame_epoch() != 0, "actual bootstrap omitted its source epoch");
    execute(initial);
    require(capture.used == 0 && session.accepted_frames() == 0 && session.native_ai_frames() == 0,
            "actual flute bootstrap fabricated accepted PCM or AI occupancy");
    bool stop_requested = false;
    for (std::uint64_t tick = 1; tick <= 240; ++tick) {
        const auto before = session.source_frame_epoch();
        require(session.retrace(tick, tick * 1000000000ull / 60), "independent flute source retrace failed");
        require(session.submit(), "source-gated actual flute PCM transfer failed");
        require(session.source_frame_epoch() == before, "retrace/AI consumption/PCM acceptance sampled a new source epoch");
        if (tick == 1) require(capture.used == 0, "first actual source notification invented bootstrap output credit");
        if (tick == 2) require(capture.used == initial.frame_count * 2 && !session.service().has_lease(),
                               "second actual source notification did not transfer bootstrap PCM exactly once");
        if (tick == 40) {
            result.first_stop = capture.used;
            if (reference) require(session.stop_song(0), "independent selected-song stop failed");
            else {
                require(flute.stop(), "actual flute source stop failed");
                require(flute.command() == 0 && flute.state() == -1 && !retained.playing &&
                        retained.song_index == static_cast<std::int32_t>(index),
                        "actual flute stop changed retained index or omitted same-epoch uncertainty");
            }
            require(!session.bgm_playing(0), "stop request claimed main0 still requested");
        }
        if (tick == 48) {
            result.restart_begin = capture.used;
            if (reference) {
                require(session.stop_song(0), "independent restart stop failed");
                require(session.play_song(0, songs[index]), "independent same-song restart failed");
            } else {
                items::Flute reconstructed(session, retained);
                require(reconstructed.command() == 0 && !retained.playing &&
                        retained.song_index == static_cast<std::int32_t>(index),
                        "wrapper reconstruction reset caller-retained source state");
                require(reconstructed.play(), "actual retained-index flute restart failed");
                require(reconstructed.command() == commands[index] && reconstructed.state() == -1 &&
                        retained.song_index == static_cast<std::int32_t>(index),
                        "stopped/reconstructed flute advanced instead of restarting the selected real song");
            }
        }
        if (tick == 88) {
            result.restart_end = capture.used;
            if (!reference) require(flute.stop(), "final actual flute source stop failed");
            require(session.stop(), "actual shared flute voice graph stop failed");
            stop_requested = true;
        }
        if (!reference) {
            const auto expected = retained.song_start_epoch == session.source_frame_epoch()
                ? -1 : static_cast<std::int32_t>(session.bgm_playing(0));
            require(flute.state() == expected, "actual flute status diverged from source-epoch/real-request semantics");
            const auto held = session.source_frame_epoch();
            for (unsigned query = 0; query < 8; ++query) (void)flute.state();
            require(session.source_frame_epoch() == held, "gameplay status polling advanced actual source audio time");
        }
        a::TaskView built;
        require(session.build(built), "actual flute source audio build failed");
        require((built.token.serial != 0) == (session.source_frame_epoch() != before),
                "actual source epoch changed without a build or failed to change with one");
        if (built.token.serial != 0) execute(built);
        if (session.state() == g::Session::State::draining) {
            require(stop_requested && session.service().voices_retired() && !session.service().has_lease(),
                    "actual flute graph began drain before all native task/PCM/physical owners retired");
            require(session.poll_drain(), "first actual owned flute capture drain failed");
            require(session.state() == g::Session::State::draining, "actual pending consumer drain was fabricated complete");
            require(session.poll_drain(), "final actual owned flute capture drain failed");
            break;
        }
    }
    require(session.state() == g::Session::State::drained && capture.finishes == 1 &&
            session.accepted_frames() * 2 == capture.used && session.service().rdram_words().data() == stable,
            "actual flute source graph failed bounded owned drain or replaced borrowed audio storage");
    require(result.first_stop != 0 && result.restart_begin < result.restart_end && result.restart_end <= capture.used &&
            result.tasks != 0 && result.reads != 0 && result.writes != 0,
            "actual flute consumer did not execute and retain both real-song intervals");
    capture.samples.resize(capture.used);
    result.samples = std::move(capture.samples);
    made.session.reset();
    require(!execution.machine && execution.borrowed == nullptr, "actual flute executor borrower survived service retirement");
    return result;
}
void aligned(const Run& first, const Run& second) {
    require(first.quotas == second.quotas && first.samples.size() == second.samples.size() &&
            first.first_stop == second.first_stop && first.restart_begin == second.restart_begin &&
            first.restart_end == second.restart_end, "actual flute comparison changed source quotas or PCM interval ownership");
}
std::size_t differences(const Run& first, const Run& second, std::size_t begin, std::size_t end) {
    aligned(first, second);
    require(begin <= end && end <= first.samples.size(), "actual flute comparison has invalid PCM interval");
    std::size_t changed = 0;
    for (std::size_t i = begin; i < end; ++i) changed += first.samples[i] != second.samples[i];
    return changed;
}
} // namespace
#ifdef _WIN32
int wmain(int argc, wchar_t* argv[]) {
#else
int main(int argc, char* argv[]) {
#endif
    try {
        require(argc == 2, "usage: item-flute-rom-smoke PRIVATE_ROM");
        const auto snapshot = isle::rom::read_snapshot(std::filesystem::path(argv[1]));
        auto hydrated = g::hydrate(snapshot);
        require(bool(hydrated), "actual GAME flute audio resource hydration failed");
        std::array<a::effects::Section, 8> sections;
        a::effects::Config effects;
        require(g::native_startup_effects(hydrated.resources, sections, effects), "actual GAME native effects configuration failed");
        std::uint64_t tasks = 0;
        std::array<std::size_t, 3> first_changes{}, restart_changes{};
        std::array<std::vector<std::int16_t>, 3> selected_pcm;
        for (unsigned index = 0; index < 3; ++index) {
            const auto flute = run(snapshot, hydrated.resources, effects, index, false, false);
            const auto direct = run(snapshot, hydrated.resources, effects, index, true, false);
            const auto muted = run(snapshot, hydrated.resources, effects, index, false, true);
            require(differences(flute, direct, 0, flute.samples.size()) == 0,
                    "flute-produced actual PCM differs from independently requested real song1/3/2 and same-song restart");
            first_changes[index] = differences(flute, muted, 0, flute.first_stop);
            restart_changes[index] = differences(flute, muted, flute.restart_begin, flute.restart_end);
            require(first_changes[index] != 0 && restart_changes[index] != 0,
                    "actual selected flute song/restart did not independently contribute executor-produced accepted PCM");
            selected_pcm[index].assign(flute.samples.begin(), flute.samples.begin() + static_cast<std::ptrdiff_t>(flute.first_stop));
            tasks += flute.tasks + direct.tasks + muted.tasks;
        }
        require(selected_pcm[0] != selected_pcm[1] && selected_pcm[0] != selected_pcm[2] && selected_pcm[1] != selected_pcm[2],
                "three selected real flute songs produced indistinguishable first-interval PCM");
        std::cout << "item_flute_actual_RSP_PCM=passed songs=1/3/2 commands=5/6/7 runs=9 tasks=" << tasks
            << " direct_song_PCM=exact retained_index_restart=exact first_changed_samples="
            << first_changes[0] << '/' << first_changes[1] << '/' << first_changes[2]
            << " restart_changed_samples=" << restart_changes[0] << '/' << restart_changes[1] << '/' << restart_changes[2]
            << " source_epoch=actual_build_only task_PCM_physical_owners=retired capture=checked_owned"
               " physical_device_probes=none Original_hardware_numerical_parity_fullgame=not_claimed\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "item-flute-rom-smoke: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
