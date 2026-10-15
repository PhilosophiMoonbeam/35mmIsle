#include "audio_probe_fixture.hpp"
#include "35mmisle/audio_transport.hpp"

#include <SDL3/SDL.h>

#include <array>
#include <bit>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

namespace {
namespace probe = isle::audio_probe;
namespace sfx = isle::audio::sfx;
using Output = isle::audio::SdlAudioOutput;
#ifdef _WIN32
using Argument = wchar_t;
#else
using Argument = char;
#endif
using ArgumentView = std::basic_string_view<Argument>;
constexpr std::uint32_t max_tasks = 10000;
constexpr std::size_t control_capacity = 128;
enum class Kind { pitch, pan, stop, replay };
struct Control {
    Kind kind{};
    std::uint32_t after_task = 0;
    float pitch = 1.0f;
    std::uint8_t pan = 64;
};
struct Options {
    std::filesystem::path rom, raw;
    std::uint32_t sound = 14, tasks = 120, quota = probe::default_quota;
    sfx::Parameters initial{};
    std::array<Control, control_capacity> controls{};
    std::size_t control_count = 0;
    bool play = false;
};

void usage(std::ostream& out) {
    out << "35mmisle-audio-probe --rom PRIVATE_ROM [--raw-out NEW_DIRECTORY]\n"
           "  [--sound INDEX] [--tasks COUNT] [--quota FRAMES] [--play]\n"
           "  [--volume 0..32767] [--pitch RATIO] [--pan 0..127]\n"
           "  [--pitch-at TASK RATIO] [--pan-at TASK PAN]\n"
           "  [--stop-at TASK] [--replay-at TASK] ...\n"
           "35mmisle-audio-probe --help\n"
           "Defaults: sound=14 tasks=120 explicit experimental quota=640 stereo frames,\n"
           "volume=32767 pitch=1 pan=64 global_volume=127 source_clock=32006 Hz.\n"
           "Limits: tasks=1..10000; quota=16..1024, multiple of 16; pitch=1/32768..65535/32768.\n"
           "At most 128 controls, globally chronological nondecreasing TASK order;\n"
           "TASK is 1-based, less than --tasks. Controls publish AFTER that completed\n"
           "task for subsequent sound-client deadlines, not sample-exact host times.\n"
           "No repeated control kind at one TASK; replay uses initial settings and\n"
           "requires the previous voice to have actually finished its release.\n"
           "Initial start publishes before task 1. No automatic stop/replay or dropped controls.\n"
           "Exact-US private z64/v64/n64 snapshot; bank 0 instrument 0 sound slot.\n"
           "One native dry voice + actual ROM aspMain RSP; no CPU decode/tone/replay.\n"
           "Quota is experimental, NOT measured/emulated AI occupancy.\n"
           "Optional private output: pcm.s16be, tasks.csv, metadata.txt (success marker last).\n"
           "--play selects checked SDL output; pumps events, retains blocked PCM, then flushes/drains.\n"
           "It opens one enumerated physical endpoint with no fallback. Stream drain is NOT DAC/hearing proof.\n"
           "No game CPU/full boot/BGM/effects/N64 hardware/physical playback qualification.\n"
           "Native core MIT; static GNU lightning LGPL-3.0-or-later; see project licenses.\n"
           "Output scope: private synthesized PCM and non-payload provenance only;\n"
           "no ROM, SDK implementation, bank, microcode, or RAM dump is published.\n";
}
std::string ascii(ArgumentView input) {
    std::string result;
    result.reserve(input.size());
    for (const auto character : input) {
        if (static_cast<unsigned long>(character) > 127)
            throw std::runtime_error("flags and numeric values must be ASCII (paths remain native)");
        result.push_back(static_cast<char>(character));
    }
    return result;
}
std::uint32_t integer(ArgumentView input, std::uint32_t low, std::uint32_t high) {
    const auto text = ascii(input);
    std::uint32_t value = 0;
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
    if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size() || value < low || value > high)
        throw std::runtime_error("integer argument is not a complete decimal value in its documented range");
    return value;
}
float pitch(ArgumentView input) {
    const auto text = ascii(input);
    float value = 0;
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value, std::chars_format::general);
    if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size() || !std::isfinite(value) ||
        value < 1.0f / 32768.0f || value > 65535.0f / 32768.0f)
        throw std::runtime_error("pitch must be finite and in [1/32768, 65535/32768]");
    return value;
}
Options parse(int argc, Argument* argv[]) {
    Options options;
    std::array<bool, 9> seen{};
    for (int i = 1; i < argc; ++i) {
        const auto flag = ascii(argv[i]);
        auto next = [&]() -> ArgumentView {
            if (i + 1 >= argc) throw std::runtime_error("incomplete audio probe argument");
            return argv[++i];
        };
        auto once = [&](std::size_t slot) {
            if (seen[slot]) throw std::runtime_error("repeated singleton audio probe argument");
            seen[slot] = true;
        };
        if (flag == "--play") {
            once(8); options.play = true;
        } else if (flag == "--rom" || flag == "--raw-out") {
            const bool rom = flag == "--rom";
            once(rom ? 0 : 1);
            const auto value = next();
            if (value.empty() || (value.size() >= 2 && value[0] == '-' && value[1] == '-'))
                throw std::runtime_error("path flag requires a nonempty native path, not another flag");
            (rom ? options.rom : options.raw) = std::filesystem::path(value);
        } else if (flag == "--sound") {
            once(2); options.sound = integer(next(), 0, 65535);
        } else if (flag == "--tasks") {
            once(3); options.tasks = integer(next(), 1, max_tasks);
        } else if (flag == "--quota") {
            once(4); options.quota = integer(next(), 16, sfx::Producer::max_frames);
            if ((options.quota & 15u) != 0) throw std::runtime_error("--quota must be a multiple of 16 frames");
        } else if (flag == "--volume") {
            once(5); options.initial.volume = static_cast<std::uint16_t>(integer(next(), 0, 32767));
        } else if (flag == "--pitch") {
            once(6); options.initial.pitch = pitch(next());
        } else if (flag == "--pan") {
            once(7); options.initial.pan = static_cast<std::uint8_t>(integer(next(), 0, 127));
        } else if (flag == "--pitch-at" || flag == "--pan-at" || flag == "--stop-at" || flag == "--replay-at") {
            if (options.control_count == options.controls.size())
                throw std::runtime_error("timed control capacity exceeded (128)");
            Control control;
            control.after_task = integer(next(), 1, max_tasks);
            if (flag == "--pitch-at") { control.kind = Kind::pitch; control.pitch = pitch(next()); }
            else if (flag == "--pan-at") { control.kind = Kind::pan; control.pan = static_cast<std::uint8_t>(integer(next(), 0, 127)); }
            else control.kind = flag == "--stop-at" ? Kind::stop : Kind::replay;
            if (options.control_count != 0 && control.after_task < options.controls[options.control_count - 1].after_task)
                throw std::runtime_error("timed controls must be globally chronological");
            std::size_t same_task = 1;
            for (std::size_t j = options.control_count; j != 0; --j) {
                const auto& previous = options.controls[j - 1];
                if (previous.after_task != control.after_task) break;
                ++same_task;
                if (previous.kind == control.kind) throw std::runtime_error("repeated timed control kind at the same task");
            }
            if (same_task > sfx::Producer::event_capacity)
                throw std::runtime_error("controls at one task exceed native event capacity");
            options.controls[options.control_count++] = control;
        } else {
            throw std::runtime_error("unknown or misplaced audio probe argument (use --help alone)");
        }
    }
    if (options.rom.empty()) throw std::runtime_error("--rom is required; no implicit ROM search is performed");
    for (std::size_t i = 0; i != options.control_count; ++i)
        if (options.controls[i].after_task >= options.tasks)
            throw std::runtime_error("timed control TASK must precede the final task");
    return options;
}
const char* name(Kind kind) noexcept {
    switch (kind) {
    case Kind::pitch: return "pitch";
    case Kind::pan: return "pan";
    case Kind::stop: return "stop";
    case Kind::replay: return "replay";
    }
    return "invalid";
}
const char* order(isle::rom::ByteOrder value) noexcept {
    switch (value) {
    case isle::rom::ByteOrder::z64: return "z64";
    case isle::rom::ByteOrder::v64: return "v64";
    case isle::rom::ByteOrder::n64: return "n64";
    }
    return "invalid";
}
void settings(std::ostream& out, const Options& options) {
    out << std::setprecision(std::numeric_limits<float>::max_digits10)
        << "sound=" << options.sound << " tasks=" << options.tasks << " experimental_quota_frames=" << options.quota
        << " volume=" << options.initial.volume << " pitch=" << options.initial.pitch
        << " pan=" << static_cast<unsigned>(options.initial.pan)
        << " global_volume=" << static_cast<unsigned>(options.initial.global_volume)
        << " source_clock_hz=" << probe::synthesis_rate << " controls=" << options.control_count
        << " checked_SDL_stream=" << options.play << '\n';
    for (std::size_t i = 0; i != options.control_count; ++i) {
        const auto& control = options.controls[i];
        out << "control=" << name(control.kind) << " after_completed_task=" << control.after_task;
        if (control.kind == Kind::pitch) out << " pitch=" << control.pitch;
        if (control.kind == Kind::pan) out << " pan=" << static_cast<unsigned>(control.pan);
        if (control.kind == Kind::replay) out << " parameters=initial";
        out << '\n';
    }
}
void apply(sfx::Producer& producer, const Control& control, const sfx::Parameters& initial, std::uint64_t& generation) {
    sfx::Error error = sfx::Error::invalid_parameter;
    switch (control.kind) {
    case Kind::pitch: error = producer.stage_pitch(generation, control.pitch); break;
    case Kind::pan: error = producer.stage_pan(generation, control.pan); break;
    case Kind::stop: error = producer.stage_stop(generation); break;
    case Kind::replay: error = producer.stage_start(initial, generation); break;
    }
    probe::require(error, "timed control rejected (voice must be live for changes/stops and idle for replay)");
}

class Events final {
public:
    explicit Events(bool enabled) : active(enabled) {
        if (active && !SDL_Init(SDL_INIT_EVENTS))
            throw std::runtime_error(std::string("SDL3 events unavailable: ") + SDL_GetError());
    }
    ~Events() { if (active) SDL_QuitSubSystem(SDL_INIT_EVENTS); }
    Events(const Events&) = delete;
    Events& operator=(const Events&) = delete;
private:
    bool active;
};
[[noreturn]] void audio_failure(Output& output, const char* operation) {
    const auto& error = output.error();
    throw std::runtime_error(std::string(operation) + ": " + error.operation.data() + ": " + error.message.data());
}
void submit(Output& output, std::span<const std::int16_t> samples) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    for (;;) {
        SDL_PumpEvents();
        const auto result = output.try_submit(samples);
        if (result == Output::Submit::accepted) return;
        if (result == Output::Submit::error) audio_failure(output, "checked PCM submission failed");
        if (std::chrono::steady_clock::now() >= deadline)
            throw std::runtime_error("audio backpressure deadline expired; PCM was not acknowledged");
        SDL_Delay(1);
    }
}
void finish(Output& output) {
    if (!output.finish_input()) audio_failure(output, "checked final flush failed");
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    for (;;) {
        SDL_PumpEvents();
        const auto result = output.poll_drain();
        if (result == Output::Drain::stream_drained) break;
        if (result == Output::Drain::error) audio_failure(output, "checked stream drain failed");
        if (std::chrono::steady_clock::now() >= deadline)
            throw std::runtime_error("audio stream drain deadline expired; no completion claimed");
        SDL_Delay(1);
    }
    const auto closed = output.close();
    if (closed.failed || closed.disposition != Output::CloseDisposition::stream_drained)
        audio_failure(output, "checked stream closure failed");
}

int run(int argc, Argument* argv[]) {
    if (argc == 2 && ascii(argv[1]) == "--help") {
        usage(std::cout);
        std::cout.flush();
        return std::cout ? 0 : 1;
    }
    const auto options = parse(argc, argv);
    Events events(options.play);
    // Native argv bytes / Windows UTF-16 reach filesystem directly. Never log
    // paths, reopen the ROM, or narrow Windows paths through a locale.
    probe::Session session(isle::rom::read_snapshot(options.rom), options.sound);
    auto& producer = session.producer();
    Output output;
    settings(std::cout, options);
    std::cout << "private_rom_admitted normalized_sha1=" << isle::rom::digest_hex(session.identity())
              << " size=" << session.identity().size << " input_order=" << order(session.identity().order)
              << " quota_policy=explicit_experiment ai_occupancy=not_claimed\n"
                 "scope=native_dry_one_voice_actual_RSP no_game_CPU_no_full_boot_no_hardware_or_hearing_qualification\n"
                 "licenses=native_MIT,static_GNU_lightning_LGPL-3.0-or-later output=private_PCM_and_nonpayload_provenance_only\n";
    std::ofstream pcm_file, tasks_file;
    if (!options.raw.empty()) {
        if (!std::filesystem::create_directory(options.raw))
            throw std::runtime_error("--raw-out directory already exists; refusing overwrite");
#ifndef _WIN32
        std::filesystem::permissions(options.raw, std::filesystem::perms::owner_all,
                                     std::filesystem::perm_options::replace);
#endif
        pcm_file.exceptions(std::ios::failbit | std::ios::badbit);
        tasks_file.exceptions(std::ios::failbit | std::ios::badbit);
        pcm_file.open(options.raw / "pcm.s16be", std::ios::binary);
        tasks_file.open(options.raw / "tasks.csv");
        tasks_file << "task,frames,source_frame_begin,task_id,generation,commands,sp_status,dma_reads,dma_writes,voice_active\n";
    }
    if (options.play) {
        if (!output.open({static_cast<int>(probe::synthesis_rate), 4096, 0}))
            audio_failure(output, "checked audio admission failed");
        const auto& info = output.info();
        std::cout << "checked_SDL_stream_open driver=" << info.driver.data()
                  << " physical_device=" << info.physical_device << " logical_device=" << info.logical_device
                  << " source_hz=" << info.source.freq << " device_hz=" << info.device.freq
                  << " device_format=" << info.device.format << " device_buffer_frames=" << info.device_buffer_frames
                  << " physical_playback=not_qualified\n";
    }
    std::uint64_t generation = 0, total_frames = 0, total_reads = 0, total_writes = 0;
    probe::require(producer.stage_start(options.initial, generation), "initial voice start failed");
    probe::require(producer.stage(), "initial controls publication failed");
    std::array<std::int16_t, sfx::Producer::max_frames * 2> scratch{};
    std::size_t next_control = 0;
    for (std::uint32_t task = 1; task <= options.tasks; ++task) {
        const auto completed = session.execute({task, 0, options.quota});
        const auto& pcm = completed.pcm;
        if (pcm.frame_count > sfx::Producer::max_frames || pcm.synthesis_rate != probe::synthesis_rate)
            throw std::runtime_error("completed PCM exceeds admitted format or fixed scratch capacity");
        if (options.play || pcm_file.is_open()) {
            probe::require(producer.copy_pcm(pcm, scratch), "owned completed PCM extraction failed");
            auto samples = std::span(scratch).first(pcm.frame_count * 2u);
            // SDL must own its native-endian copy before scratch is converted
            // in place for an optional canonical BE16 file. The lease remains
            // live on every WouldBlock; no task can recycle its RDRAM.
            if (options.play) submit(output, samples);
            if (pcm_file.is_open()) {
                for (auto& sample : samples) {
                    const auto value = std::bit_cast<std::uint16_t>(sample);
                    sample = std::bit_cast<std::int16_t>(
                        static_cast<std::uint16_t>((value << 8) | (value >> 8)));
                }
            }
        }
        if (pcm_file.is_open()) {
            pcm_file.write(reinterpret_cast<const char*>(scratch.data()), static_cast<std::streamsize>(pcm.frame_count * 4u));
            tasks_file << task << ',' << pcm.frame_count << ',' << total_frames << ',' << completed.task.task_id << ','
                       << completed.task.generation << ',' << completed.task.command_bytes / 8 << ','
                       << completed.execution.sp_status << ',' << completed.execution.dma_reads << ','
                       << completed.execution.dma_writes << ',' << producer.voice_active() << '\n';
        }
        total_frames += pcm.frame_count;
        total_reads += completed.execution.dma_reads;
        total_writes += completed.execution.dma_writes;
        probe::require(producer.release(pcm.task_id, pcm.generation), "copied PCM lease release failed");
        bool changed = false;
        while (next_control != options.control_count && options.controls[next_control].after_task == task) {
            apply(producer, options.controls[next_control++], options.initial, generation);
            changed = true;
        }
        if (changed) probe::require(producer.stage(), "timed controls publication failed");
    }
    if (next_control != options.control_count) throw std::runtime_error("scheduled controls were not all consumed");
    if (pcm_file.is_open()) { pcm_file.close(); tasks_file.close(); }
    if (options.play) {
        finish(output);
        std::cout << "checked_SDL_stream_drained accepted_copy_frames=" << total_frames
                  << " source_hz=" << probe::synthesis_rate << " physical_playback=not_qualified\n";
    }
    std::cout << "actual_native_audio=passed tasks=" << options.tasks << " stereo_frames=" << total_frames
              << " source_clock_hz=" << probe::synthesis_rate << " all_SP_status=0x203 all_DPC_packets=0"
              << " dma_reads=" << total_reads << " dma_writes=" << total_writes
              << " physical_playback=not_claimed independent_RSP_or_hardware_parity=not_claimed\n";
    std::cout.flush();
    if (!std::cout) throw std::runtime_error("cannot write private experiment report");
    // Last file is the success marker. Failed runs can leave partial PCM/CSV
    // in their exclusive directory, never a successful provenance document.
    if (!options.raw.empty()) {
        std::ofstream metadata;
        metadata.exceptions(std::ios::failbit | std::ios::badbit);
        metadata.open(options.raw / "metadata.txt");
        metadata << "experiment=35mmisle-native-dry-one-voice-actual-RSP\n"
                    "snapshot_source_pin=df8815e75f3d79618466e01e0827c6165b93d11c\n"
                    "parallel_rsp_pin=9ab776a2ce28899769c4f911403234cac62fc799\n"
                 << "normalized_sha1=" << isle::rom::digest_hex(session.identity()) << '\n'
                 << "input_order=" << order(session.identity().order) << '\n';
        settings(metadata, options);
        metadata << "stereo_frames=" << total_frames << "\npcm_format=stereo_signed_16_big_endian\n"
                 << "pcm_bytes=" << total_frames * 4 << "\nall_SP_status=0x203\nall_DPC_packets=0\n"
                 << "dma_reads=" << total_reads << "\ndma_writes=" << total_writes << '\n'
                 << "initial_start=before_task_1\ncontrols=after_completed_task_at_next_client_deadline\n"
                    "quota_policy=explicit_experiment_not_AI_occupancy\n"
                    "scope=no_game_CPU_full_boot_BGM_effects_hardware_or_physical_playback_qualification\n"
                    "independent_RSP_or_hardware_parity=not_claimed\n"
                    "licenses=native_core_MIT;static_GNU_lightning_LGPL-3.0-or-later;see_project_licenses\n"
                    "output_scope=private_synthesized_PCM_and_nonpayload_provenance_only\n"
                    "no_ROM_bank_microcode_SDK_implementation_or_RAM_dump\n"
                 << "audio_output=" << (options.play ? "checked_SDL_stream_drained_not_DAC_completion" : "not_requested") << '\n'
                 << "completed=actual_all_tasks_and_selected_stream_success\n";
        metadata.close();
    }
    return 0;
}
} // namespace
#ifdef _WIN32
int wmain(int argc, wchar_t* argv[]) {
#else
int main(int argc, char* argv[]) {
#endif
    try {
        return run(argc, argv);
    } catch (const std::filesystem::filesystem_error&) {
        std::cerr << "Private audio experiment failed: private filesystem operation failed.\n";
    } catch (const std::ios_base::failure&) {
        std::cerr << "Private audio experiment failed: private input or report I/O failed.\n";
    } catch (const std::runtime_error& error) {
        std::cerr << "Private audio experiment failed: " << error.what() << ".\n";
    } catch (...) {
        std::cerr << "Private audio experiment failed: private input, allocation or reporting operation failed.\n";
    }
    return 1;
}
