#include "audio_fixture.hpp"
#include "35mmisle/audio_sfx.hpp"
#include "../src/audio_synth_internal.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>
#include <vector>

namespace a = isle::audio;
namespace d = isle::audio::detail;
namespace e = isle::audio::effects;
namespace f = isle::audio_test;
namespace {
int failures = 0;
void check(bool value, std::string_view message) {
    if (!value) { ++failures; std::cerr << "FAIL: " << message << '\n'; }
}
e::PoleFilter pole() {
    e::PoleFilter result;
    result.gain = 0x1200;
    // Authored numeric data, deliberately not a GAME profile/book fixture.
    for (std::size_t i = 8; i < result.coefficients.size(); ++i)
        result.coefficients[i] = static_cast<std::int16_t>(2400 - static_cast<int>(i) * 37);
    return result;
}
e::Section filtered(std::uint32_t input = 0, std::uint32_t output = 256) {
    return {input, output, 7100, -8100, 9200, pole()};
}
struct Core {
    std::unique_ptr<d::Context> storage = std::make_unique<d::Context>();
    a::BankFile bank;
    Core() {
        f::Fixture fixture;
        auto decoded = fixture.decode();
        check(bool(decoded), "authored bank hydrates");
        bank = decoded.asset;
        auto& c = context();
        c.owner = 43;
        c.config = {32006, 2, 4};
        c.coefficients = f::coefficients();
        c.memory = std::make_unique<std::uint32_t[]>(a::AudioService::rdram_bytes / 4u);
        const std::array owners{bank};
        check(d::build_catalog(c, owners) == a::Error::none, "authored immutable catalog packs");
    }
    d::Context& context() { return *storage; }
    d::EffectsState& state() { return context().state().effects; }
    void program(std::span<const e::Section> sections, std::uint32_t samples = 1024) {
        check(d::build_effects(context(), {e::Routing::network, samples, sections}) == a::Error::none,
              "authored static program is admitted");
    }
    void rewind_commands() {
        context().write_address = a::AudioService::command_address;
        context().write_error = a::Error::none;
    }
};
struct Command {
    std::uint32_t first = 0, second = 0;
    std::uint16_t input = 0, output = 0, bytes = 0;
    unsigned opcode() const { return first >> 24; }
};
// Decode command bindings only, never execute/HLE an audio kernel. These tests
// establish producer ordering/state/ownership, not actual-RSP PCM qualification.
std::vector<Command> commands(std::span<const std::uint32_t> memory, std::uint32_t end) {
    std::vector<Command> result;
    std::uint16_t input = 0, output = 0, bytes = 0;
    for (auto address = a::AudioService::command_address; address < end; address += 8u) {
        const auto first = memory[address / 4u], second = memory[address / 4u + 1u];
        if ((first & 0xffff0000u) == 0x08000000u) {
            input = static_cast<std::uint16_t>(first);
            output = static_cast<std::uint16_t>(second >> 16);
            bytes = static_cast<std::uint16_t>(second);
        }
        result.push_back({first, second, input, output, bytes});
    }
    return result;
}
std::vector<Command> commands(Core& core) {
    return commands({core.context().memory.get(), a::AudioService::rdram_bytes / 4u}, core.context().write_address);
}
std::vector<Command> commands(a::AudioService& service, const a::TaskView& task) {
    return commands(service.rdram_words(), task.command_address + task.command_bytes);
}
std::size_t count(const std::vector<Command>& trace, unsigned opcode) {
    return static_cast<std::size_t>(std::count_if(trace.begin(), trace.end(),
        [opcode](const auto& command) { return command.opcode() == opcode; }));
}
std::vector<std::uint32_t> mutable_arena(std::span<const std::uint32_t> memory) {
    return {memory.begin() + d::effects_ring_address / 4u,
            memory.begin() + (d::effects_staging_address + d::effects_staging_bytes) / 4u};
}
std::vector<std::uint32_t> mutable_arena(Core& core) {
    return mutable_arena({core.context().memory.get(), a::AudioService::rdram_bytes / 4u});
}
void validate_programs_and_copy() {
    const std::array sections{filtered(4, 8)};
    check(d::validate_effects({e::Routing::direct}) == a::Error::none,
          "explicit direct is admitted without a network");
    for (const auto config : std::array{
            e::Config{e::Routing::direct, 1024, {}},
            e::Config{e::Routing::direct, 0, sections},
            e::Config{static_cast<e::Routing>(9), 0, {}},
            e::Config{e::Routing::network, 0, sections},
            e::Config{e::Routing::network, 1024, {}},
            e::Config{e::Routing::network, 159, sections},
            e::Config{e::Routing::network, 1025, sections},
            e::Config{e::Routing::network, 19204, sections},
            e::Config{e::Routing::network, std::numeric_limits<std::uint32_t>::max(), sections}})
        check(d::validate_effects(config) == a::Error::invalid_parameter,
              "invalid routing, ring extent/grid and missing sections never fall back to direct");
    std::array<e::Section, 17> too_many{};
    check(d::validate_effects({e::Routing::network, 1024, too_many}) == a::Error::invalid_parameter,
          "seventeenth section exceeds explicit native capacity");
    auto section = filtered(4, 8);
    check(d::validate_effects({e::Routing::network, 168, {&section, 1}}) == a::Error::none,
          "four-sample offsets and non-eight-sample ring length remain valid");
    for (const auto offset : {2u, 12u, std::numeric_limits<std::uint32_t>::max()}) {
        section.input_samples = offset;
        check(d::validate_effects({e::Routing::network, 168, {&section, 1}}) == a::Error::invalid_parameter,
              "port arithmetic rejects misalignment, insufficient window space and overflow");
    }
    Core core;
    std::array<e::Section, e::max_sections> maximum{};
    maximum.back().pole = pole();
    core.program(maximum, e::max_ring_samples);
    check(core.context().effects.ring_bytes == 38400 && core.context().effects.section_count == 16,
          "one largest ring and all sixteen fixed native slots are usable");
    const auto before = mutable_arena(core);
    const auto old_count = core.context().effects.section_count;
    check(d::build_effects(core.context(), {e::Routing::network, 1, sections}) == a::Error::invalid_parameter &&
          core.context().effects.section_count == old_count && mutable_arena(core) == before,
          "bad configuration cannot partially replace program or coefficient storage");
}
void ordered_alias_commits_and_filter_init() {
    Core core;
    const std::array sections{
        e::Section{0, 256, 6100, -7200, 0, {}},
        filtered(256, 384)};
    core.program(sections);
    core.state().cursor = 800;
    const auto before = mutable_arena(core);
    const auto ready = d::render_effects(core.context(), core.state(), 0, 160, 160);
    const auto trace = commands(core);
    check(ready.task_offset == 0 && ready.frames == 160 && core.state().cursor == 1120,
          "complete outer advances one shared ring by actual byte count");
    check(mutable_arena(core) == before, "graph build writes no ring, history, staging or immutable-book bytes");
    struct Op { std::uint32_t first, second; };
    // Input/output windows of the second section overlap by64 bytes. The first
    // section output is the second input: this sequence is the alias contract.
    const auto base = d::effects_ring_address;
    const std::array expected{
        Op{0x0c00da83u, 0x06c006c0u}, Op{0x0c005a82u, 0x080006c0u},
        Op{0x06000000u, base + 800},
        Op{0x04000000u, base + 800}, Op{0x04000000u, base + 288},
        Op{0x0c00e3e0u, 0x00000140u}, // -7200 feedforward
        Op{0x06000000u, base + 288},
        Op{0x0c0017d4u, 0x01400000u}, //6100 feedback
        Op{0x06000000u, base + 800}, Op{0x06000000u, base + 288},
        Op{0x04000000u, base + 288}, Op{0x04000000u, base + 32},
        Op{0x0c00e05cu, 0x00000140u}, // -8100 feedforward
        Op{0x0c001bbcu, 0x01400000u}, //7100 feedback
        Op{0x06000000u, base + 288},
        Op{0x0b000020u, d::effects_book_address(1)},
        Op{0x0e011200u, d::effects_history_address(1)},
        Op{0x06000000u, base + 32}, Op{0x0c0023f0u, 0x01400800u}};
    std::vector<Command> relevant;
    for (const auto& command : trace)
        if (command.opcode() == 4 || command.opcode() == 6 || command.opcode() == 11 ||
            command.opcode() == 12 || command.opcode() == 14) relevant.push_back(command);
    bool same = relevant.size() == expected.size();
    for (std::size_t i = 0; same && i < expected.size(); ++i)
        same = relevant[i].first == expected[i].first && relevant[i].second == expected[i].second;
    check(same, "alias network preserves literal downmix, both complete loads and ordered prefeedback/input/pole/output commits");
    check(std::all_of(relevant.begin(), relevant.end(), [](const auto& command) {
        return command.bytes == 320 && (command.opcode() != 14 || (command.input == 0x140 && command.output == 0x140));
    }), "mixers, coefficient load, filter and every save retain full count and filter bindings");
    core.rewind_commands();
    d::render_effects(core.context(), core.state(), 0, 16, 16);
    const auto continuation = commands(core);
    check(std::any_of(continuation.begin(), continuation.end(), [](const auto& command) {
        return command.first == 0x0e001200u;
    }), "subsequent short outer continues the persistent filter without another INIT");
}
void wraps_end_cursor_and_zero_tap() {
    Core core;
    auto section = filtered(0, 4);
    section.gain = 0;
    const std::array sections{section};
    core.program(sections, 640);
    core.state().cursor = 1184; //96 bytes before ring end.
    const auto output = d::render_effects(core.context(), core.state(), 0, 160, 160);
    const auto trace = commands(core);
    check(output.frames == 160 && core.state().cursor == 224 && count(trace, 14) == 1,
          "wrapped zero-tap section still processes feedback and persistent filter");
    struct Transfer { unsigned opcode; std::uint32_t address; std::uint16_t dmem, bytes; };
    const auto base = d::effects_ring_address;
    const std::array expected{
        Transfer{6, base + 1184, 0x6c0, 96}, Transfer{6, base, 0x720, 224},
        Transfer{4, base + 1184, 0, 96}, Transfer{4, base, 96, 224},
        Transfer{4, base + 1176, 0x140, 104}, Transfer{4, base, 0x1a8, 216},
        Transfer{6, base + 1184, 0, 96}, Transfer{6, base, 96, 224},
        Transfer{6, base + 1176, 0x140, 104}, Transfer{6, base, 0x1a8, 216}};
    std::vector<Command> transfers;
    for (const auto& command : trace)
        if (command.opcode() == 4 || command.opcode() == 6) transfers.push_back(command);
    bool same = transfers.size() == expected.size();
    for (std::size_t i = 0; same && i < expected.size(); ++i) {
        const auto& actual = transfers[i]; const auto& wanted = expected[i];
        same = actual.opcode() == wanted.opcode && actual.second == wanted.address && actual.bytes == wanted.bytes &&
            (actual.opcode() == 4 ? actual.input : actual.output) == wanted.dmem &&
            actual.second >= base && actual.second + actual.bytes <= base + 1280;
    }
    check(same, "wrapped loads assemble whole operands and stores split only at logical ring boundary in original order");
    check(std::all_of(trace.begin(), trace.end(), [](const auto& command) {
        return (command.opcode() != 12 && command.opcode() != 14) || command.bytes == 320;
    }), "split transfer counts never leak into downmix, feedback, feedforward or pole");
    check(count(trace, 12) == 4 && core.state().initialized_poles == 1,
          "gain zero suppresses only tap, not either signed mix or pole initialization");
    core.state().cursor = 960;
    core.rewind_commands();
    d::render_effects(core.context(), core.state(), 0, 160, 160);
    check(core.state().cursor == 1280, "exact endpoint remains an end cursor, not prematurely normalized");
    core.rewind_commands();
    d::render_effects(core.context(), core.state(), 0, 16, 16);
    const auto endpoint = commands(core);
    std::vector<Command> endpoint_dma;
    for (const auto& command : endpoint)
        if (command.opcode() == 4 || command.opcode() == 6) endpoint_dma.push_back(command);
    check(endpoint_dma.size() >= 4 && endpoint_dma[0].opcode() == 6 && endpoint_dma[0].second == base + 1280 &&
          endpoint_dma[0].bytes == 0 && endpoint_dma[1].second == base && endpoint_dma[1].bytes == 32 &&
          endpoint_dma[2].opcode() == 4 && endpoint_dma[2].second == base + 1280 && endpoint_dma[2].bytes == 0 &&
          endpoint_dma[3].second == base && endpoint_dma[3].bytes == 32 && core.state().cursor == 32,
          "end cursor retains zero-byte save/load segments before next base transfers");
}
void fragment_staging_and_task_relative_final_partial() {
    Core core;
    const std::array sections{filtered()};
    core.program(sections);
    const auto before = mutable_arena(core);
    check(d::render(core.context(), core.context().state(), 64, 0, 208) == a::Error::none &&
          core.state().staged_frames == 64 && core.state().cursor == 0,
          "first callback prefix stages all buses without pulling graph");
    auto trace = commands(core);
    check(count(trace, 6) == 4 && count(trace, 14) == 0 && count(trace, 13) == 0,
          "incomplete outer emits four staging saves and no filter/final output");
    check(d::render(core.context(), core.context().state(), 32, 64, 208) == a::Error::none &&
          core.state().staged_frames == 96 && core.state().cursor == 0,
          "new local Renderer resumes common staged prefix rather than resetting it");
    check(d::render(core.context(), core.context().state(), 64, 96, 208) == a::Error::none &&
          core.state().staged_frames == 0 && core.state().cursor == 320,
          "last fragment restores four complete planes and runs graph exactly once");
    trace = commands(core);
    std::vector<Command> staging_saves, staging_loads;
    for (const auto& command : trace) {
        if (command.second >= d::effects_staging_address &&
            command.second < d::effects_staging_address + d::effects_staging_bytes) {
            if (command.opcode() == 6) staging_saves.push_back(command);
            if (command.opcode() == 4) staging_loads.push_back(command);
        }
    }
    bool planes = staging_saves.size() == 12 && staging_loads.size() == 4;
    const std::array<std::uint16_t, 4> buses{0x440, 0x580, 0x6c0, 0x800};
    for (std::size_t plane = 0; planes && plane < 4; ++plane) {
        const auto base = d::effects_staging_address + static_cast<std::uint32_t>(plane) * 320u;
        planes = staging_saves[plane].second == base && staging_saves[plane].output == buses[plane] &&
            staging_saves[plane].bytes == 128 && staging_saves[4 + plane].second == base + 128 &&
            staging_saves[4 + plane].bytes == 64 && staging_saves[8 + plane].second == base + 192 &&
            staging_saves[8 + plane].bytes == 128 && staging_loads[plane].second == base &&
            staging_loads[plane].input == buses[plane] && staging_loads[plane].bytes == 320;
    }
    check(planes && count(trace, 14) == 1 && count(trace, 13) == 1,
          "three fragment offsets occupy all four planes then restore the full original160 exactly once");
    check(d::render(core.context(), core.context().state(), 48, 160, 208) == a::Error::none,
          "task final48 is processed without padding or carry");
    trace = commands(core);
    std::vector<Command> poles, pcm;
    for (const auto& command : trace) {
        if (command.opcode() == 14) poles.push_back(command);
        if (command.opcode() == 6 && command.second >= a::AudioService::pcm_address &&
            command.second < a::AudioService::state_address) pcm.push_back(command);
    }
    check(poles.size() == 2 && poles[0].bytes == 320 && poles[1].bytes == 96 &&
          pcm.size() == 2 && pcm[0].second == a::AudioService::pcm_address && pcm[0].bytes == 640 &&
          pcm[1].second == a::AudioService::pcm_address + 640 && pcm[1].bytes == 192 &&
          core.state().cursor == 416 && core.state().staged_frames == 0,
          "160+48 final mixers/interleave/PCM saves correspond to original task offsets and counts");
    check(mutable_arena(core) == before, "staged rendering emits DMA only, never CPU bus/history copies");
    core.rewind_commands();
    check(d::render(core.context(), core.context().state(), 48, 0, 48) == a::Error::none &&
          core.state().cursor == 512 && count(commands(core), 14) == 1 && count(commands(core), 4) == 2,
          "next task begins a fresh48 outer despite global sample clock208 and needs no staging");
}
void parameter_fragments_voice_books_and_global_tail() {
    Core core;
    const std::array sections{filtered()};
    core.program(sections);
    a::VoiceHandle voice{}; a::Allocation allocation{};
    auto& c = core.context(); auto& state = c.state();
    check(d::create_voice(c, state, voice) == a::Error::none &&
          d::allocate_voice(c, state, voice, 64, {0}, allocation) == a::Error::none &&
          d::start_voice(c, state, voice, {c.owner, 0}, {1.0f, {21000}, 64, 100}, {0}, {0}) == a::Error::none &&
          d::fx_voice(c, state, voice, 90, {32}) == a::Error::none &&
          d::pan_voice(c, state, voice, 10, {96}) == a::Error::none,
          "source carries real wet sends and independent parameter boundaries");
    check(d::render(c, state, 320, 0, 320) == a::Error::none, "parameter-split voice task builds");
    const auto trace = commands(core);
    check(count(trace, 14) == 2 && count(trace, 13) == 2 && core.state().cursor == 640,
          "multiple parameter fragments do not become multiple wet outers or output mixes");
    auto first_pole = std::find_if(trace.begin(), trace.end(), [](const auto& command) { return command.opcode() == 14; });
    auto next_voice_book = first_pole == trace.end() ? trace.end() :
        std::find_if(first_pole + 1, trace.end(), [&c](const auto& command) {
            return command.opcode() == 11 && command.second == c.catalog.waves[0].book_address;
        });
    auto next_decode = next_voice_book == trace.end() ? trace.end() :
        std::find_if(next_voice_book + 1, trace.end(), [](const auto& command) { return command.opcode() == 1; });
    check(first_pole != trace.end() && next_voice_book != trace.end() && next_decode != trace.end() &&
          next_voice_book->first == (0x0b000000u | c.catalog.waves[0].book_bytes),
          "pole LOADADPCM cannot poison a cached voice book: next voice reloads its full book before decode");
    const auto cursor = core.state().cursor;
    const auto initialized = core.state().initialized_poles;
    check(d::stop_voice(c, state, voice, {320}) == a::Error::none &&
          d::free_voice(c, state, voice, {320}) == a::Error::none &&
          core.state().cursor == cursor && core.state().initialized_poles == initialized,
          "individual stop/free never resets the common wet tail");
    core.rewind_commands();
    check(d::render(c, state, 48, 0, 48) == a::Error::none && core.state().cursor == cursor + 96 &&
          count(commands(core), 14) == 1 && count(commands(core), 1) == 0,
          "silent-source final partial still evolves global ring and pole history");
}
a::ServiceResult make(const a::BankFile& bank, e::Config config, std::uint16_t voices = 2) {
    const std::array owners{bank};
    return a::AudioService::create(owners, {32006, voices, 4}, f::coefficients(), config);
}
// Ownership acknowledgements below do not execute DSP or establish PCM fidelity.
void acknowledge(a::AudioService& service, const a::TaskView& task) {
    a::PcmView pcm{};
    check(service.complete(task.token, true, pcm) == a::Error::none && service.release(pcm.token) == a::Error::none,
          "CPU test releases matching ownership token without an RSP claim");
}
void borrowed_lifetime_and_callback_fragments() {
    f::Fixture fixture;
    auto bank = fixture.decode();
    a::ServiceResult made;
    const auto expected_pole = pole();
    {
        std::array sections{filtered(4, 260)};
        e::Config config{e::Routing::network, 1024, sections};
        made = make(bank.asset, config);
        sections[0] = {};
        config = {e::Routing::direct};
    }
    check(bool(made), "creation copies borrowed stack program and explicit book data");
    if (!made) return;
    auto& service = *made.service;
    const auto book_word = service.rdram_words()[d::effects_book_address(0) / 4u + 4u];
    check(book_word == ((static_cast<std::uint32_t>(std::bit_cast<std::uint16_t>(expected_pole.coefficients[8])) << 16) |
                       std::bit_cast<std::uint16_t>(expected_pole.coefficients[9])),
          "coefficient RAM retains original owned signed numeric values after caller storage dies");
    for (const auto due : {64u, 96u}) {
        a::sfx::Config config;
        config.max_sounds = 1; config.first_due = {due};
        a::PlayerId id{};
        check(service.add_sound_player(config, id) == a::Error::none, "independent real callback calendar registers");
    }
    const auto before = mutable_arena(service.rdram_words());
    a::TaskView task{};
    check(service.build({1, 0, 208}, task) == a::Error::none, "real service callbacks fragment a static wet task");
    const auto trace = commands(service, task);
    check(count(trace, 14) == 2 && count(trace, 13) == 2 &&
          std::any_of(trace.begin(), trace.end(), [](const auto& command) {
              return command.opcode() == 4 && command.second == d::effects_ring_address + 2040;
          }) && mutable_arena(service.rdram_words()) == before,
          "copied nondefault offsets survive callbacks;160+48 graph executes once each with no CPU DSP mutation");
    acknowledge(service, task);
    auto invalid = make(bank.asset, {e::Routing::direct, 1024, {}});
    check(!invalid && invalid.error == a::Error::invalid_parameter && !invalid.service,
          "public creation rejects invalid network data instead of publishing a direct fallback");
}
void rollback_poison_reset_and_immutable_lifetime() {
    f::Fixture fixture;
    f::put32(*fixture.bank, 128, 1); // Authored short-loop capacity stress.
    auto bank = fixture.decode();
    const std::array sections{filtered()};
    auto made = make(bank.asset, {e::Routing::network, 1024, sections});
    check(bool(made), "rollback service creates");
    if (!made) return;
    auto& service = *made.service;
    auto synth = service.synth();
    a::SoundId sound{}; a::WaveId wave{};
    check(service.sound_id(f::selected(bank.asset), sound) == a::Error::none &&
          service.wave_id(sound, wave) == a::Error::none, "short-loop source resolves");
    for (unsigned i = 0; i < 2; ++i) {
        a::VoiceHandle voice{}; a::Allocation allocation{};
        check(synth.create_voice(voice) == a::Error::none && synth.allocate(voice, 64, {0}, allocation) == a::Error::none &&
              synth.start(voice, wave, {65535.0f / 32768.0f, {24000}, 64, 127}, {0}, {0}) == a::Error::none,
              "two authored short-loop voices stress speculative command capacity");
    }
    const auto before = mutable_arena(service.rdram_words());
    a::TaskView rejected{};
    check(service.build({1, 0, 1024}, rejected) == a::Error::command_capacity && !service.has_lease() &&
          rejected.token.owner == 0 && synth.control_frontier().value == 0 && mutable_arena(service.rdram_words()) == before,
          "failed build publishes neither live FX/voice clock, task token nor CPU ring/history/staging writes");
    a::TaskView retry{};
    check(service.build({1, 0, 16}, retry) == a::Error::none && retry.token.serial == 1,
          "same service id retries after capacity rollback without consuming task identity");
    auto trace = commands(service, retry);
    auto first_ring_save = std::find_if(trace.begin(), trace.end(), [](const auto& command) {
        return command.opcode() == 6 && command.second >= d::effects_ring_address &&
            command.second < d::effects_ring_address + d::effects_ring_bytes;
    });
    check(first_ring_save != trace.end() && first_ring_save->second == d::effects_ring_address &&
          std::any_of(trace.begin(), trace.end(), [](const auto& command) { return command.first == 0x0e011200u; }),
          "retry uses rolled-back ring cursor and INIT pole flags, not rejected shadow progress");
    check(service.reset() == a::Error::task_busy, "executing lease excludes FX reset");
    a::PcmView pcm{};
    check(service.complete(retry.token, true, pcm) == a::Error::none && service.reset() == a::Error::task_busy &&
          service.release(pcm.token) == a::Error::none, "PCM lease also excludes mutable FX reset");
    a::TaskView failed_execution{};
    check(service.build({2, 0, 16}, failed_execution) == a::Error::none &&
          service.complete(failed_execution.token, false, pcm) == a::Error::execution_failed &&
          service.build({3, 0, 16}, rejected) == a::Error::needs_reset,
          "terminal execution failure poisons all subsequent wet work until quiescent reset");
    auto memory = service.rdram_words();
    const auto* stable = memory.data();
    std::array<std::uint32_t, 8> book{};
    std::copy_n(memory.begin() + d::effects_book_address(0) / 4u, book.size(), book.begin());
    const auto resource_word = memory[a::AudioService::resource_address / 4u];
    std::fill_n(memory.begin() + d::effects_ring_address / 4u, d::effects_ring_bytes / 4u, 0xa1b2c3d4u);
    for (std::size_t i = 0; i < e::max_sections; ++i)
        std::fill_n(memory.begin() + d::effects_history_address(i) / 4u, d::effects_history_bytes / 4u, 0x11223344u);
    std::fill_n(memory.begin() + d::effects_staging_address / 4u, d::effects_staging_bytes / 4u, 0x55667788u);
    check(service.reset() == a::Error::none && service.rdram_words().data() == stable,
          "quiescent reset clears poison without replacing stable RDRAM");
    bool cleared = std::all_of(memory.begin() + d::effects_ring_address / 4u,
        memory.begin() + (d::effects_ring_address + d::effects_ring_bytes) / 4u, [](auto value) { return value == 0; });
    for (std::size_t i = 0; i < e::max_sections; ++i)
        cleared = cleared && std::all_of(memory.begin() + d::effects_history_address(i) / 4u,
            memory.begin() + (d::effects_history_address(i) + d::effects_history_bytes) / 4u,
            [](auto value) { return value == 0; });
    cleared = cleared && std::all_of(memory.begin() + d::effects_staging_address / 4u,
        memory.begin() + (d::effects_staging_address + d::effects_staging_bytes) / 4u,
        [](auto value) { return value == 0; });
    check(cleared && std::equal(book.begin(), book.end(), memory.begin() + d::effects_book_address(0) / 4u) &&
          memory[a::AudioService::resource_address / 4u] == resource_word,
          "reset zeros full mutable ring/all histories/four planes while preserving owned books and catalog");
    a::TaskView after_reset{};
    check(service.build({1, 0, 16}, after_reset) == a::Error::none && after_reset.token.owner == retry.token.owner &&
          after_reset.token.serial > failed_execution.token.serial, "reset preserves monotonic owner/task identities");
    trace = commands(service, after_reset);
    check(std::any_of(trace.begin(), trace.end(), [](const auto& command) { return command.first == 0x0e011200u; }) &&
          std::any_of(trace.begin(), trace.end(), [](const auto& command) {
              return command.opcode() == 6 && command.output == 0x6c0 && command.second == d::effects_ring_address;
          }), "immutable program remains active after reset with cursor zero and fresh pole initialization");
    acknowledge(service, after_reset);
}
void direct_preserves_per_fragment_return() {
    Core core;
    check(d::render(core.context(), core.context().state(), 64, 0, 160) == a::Error::none &&
          d::render(core.context(), core.context().state(), 96, 64, 160) == a::Error::none,
          "explicit direct uses original fragment rendering");
    const auto trace = commands(core);
    check(count(trace, 13) == 2 && count(trace, 12) == 4 && count(trace, 14) == 0 &&
          count(trace, 4) == 0 && core.state().cursor == 0 && core.state().staged_frames == 0,
          "direct returns stereo AUX per fragment with no ring, mono downmix or four-plane staging");
    check(std::all_of(trace.begin(), trace.end(), [](const auto& command) {
        return command.opcode() != 12 || (command.first == 0x0c007fffu &&
            (command.second == 0x06c00440u || command.second == 0x08000580u));
    }), "direct retains both proven final AUX destination-scaling kernels exactly");
}
} // namespace
int main() {
    validate_programs_and_copy();
    ordered_alias_commits_and_filter_init();
    wraps_end_cursor_and_zero_tap();
    fragment_staging_and_task_relative_final_partial();
    parameter_fragments_voice_books_and_global_tail();
    borrowed_lifetime_and_callback_fragments();
    rollback_poison_reset_and_immutable_lifetime();
    direct_preserves_per_fragment_return();
    if (failures) return EXIT_FAILURE;
    std::cout << "Static wet program ordering, bounds, fragmentation and ownership checks passed\n";
    return EXIT_SUCCESS;
}
