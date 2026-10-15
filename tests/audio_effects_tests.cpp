#include "audio_fixture.hpp"
#include "35mmisle/audio_sfx.hpp"
#include "../src/audio_synth_internal.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <numbers>
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
e::Section modulated(std::uint32_t rate = 513, std::uint32_t depth = 2500,
                     std::uint32_t output = 256) {
    e::Section result{0, output, 0, 0, 0, {}};
    result.chorus = e::Chorus{rate, depth};
    return result;
}
bool same_chorus(const d::ChorusState& left, const d::ChorusState& right) {
    return left.phase == right.phase && left.carry == right.carry &&
        left.drift == right.drift && left.initialized == right.initialized;
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
              "authored effects program is admitted");
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
// Literal scalar command/carry facts from the sealed .17.4 training section13,
// heldout section0 and requested-30000 section0, generation1/tasks300..303.
// No ROM/PCM/coefficient fixture or CPU audio decoder is embedded here.
struct ChorusVector { std::uint16_t frames, word, carry, pull; };
void chorus_vectors(std::uint32_t rate, std::uint32_t depth, std::uint32_t frequency,
                    std::span<const ChorusVector> vectors) {
    Core core;
    core.context().config.synthesis_rate = frequency;
    const std::array sections{modulated(rate, depth)};
    core.program(sections);
    std::int32_t drift = 0;
    for (std::size_t i = 0; i < vectors.size(); ++i) {
        const auto& wanted = vectors[i];
        const auto cursor = core.state().cursor;
        const auto tap = static_cast<std::uint32_t>(
            (static_cast<std::int32_t>(cursor) - 512 + drift + 4096) % 2048);
        core.rewind_commands();
        check(d::render_effects(core.context(), core.state(), 0, wanted.frames, wanted.frames).frames == wanted.frames,
              "recorded chorus outer builds");
        const auto trace = commands(core);
        const auto rs = std::find_if(trace.begin(), trace.end(), [](const auto& command) { return command.opcode() == 5; });
        const auto load = std::find_if(trace.begin(), trace.end(), [](const auto& command) {
            return command.opcode() == 4 && command.input == 0x280;
        });
        check(rs != trace.end() && rs->first == (0x05000000u | (i == 0 ? 0x10000u : 0u) | wanted.word) &&
              rs->second == d::effects_resample_address(0) && rs->input == 0x280u + (tap & 7u) &&
              rs->output == 0x140 && rs->bytes == wanted.frames * 2u,
              "normal/heldout literal pitch, INIT, alignment and complete output count are exact");
        check(load != trace.end() && load->second == d::effects_ring_address + (tap & ~7u) &&
              load->bytes == std::min<std::uint32_t>(wanted.pull * 2u + (tap & 7u), 2048u - (tap & ~7u)),
              "pull count includes alignment and the prior drift rather than next drift");
        drift += 2 * (static_cast<std::int32_t>(wanted.pull) - wanted.frames);
        check(core.state().chorus[0].carry == wanted.carry &&
              core.state().chorus[0].drift == static_cast<std::uint32_t>((drift + 4096) % 2048) &&
              count(trace, 5) == 1 && count(trace, 6) <= 2,
              "recorded carry and modulo drift survive each outer, including carry underflow wrap");
    }
}
void normal_and_heldout_chorus_arithmetic() {
    const ChorusVector canonical[]{
        ChorusVector{160,32766,32448,159}, {160,32764,31808,160}, {160,32762,30848,160},
        {160,32760,29568,160}, {160,32758,27968,160}, {160,32756,26048,160},
        {160,32754,23808,160}, {48,32753,23088,48}, {160,32751,20368,160},
        {160,32749,17328,160}, {160,32747,13968,160}, {48,32746,12912,48},
        {160,32744,9072,160}, {160,32742,4912,160}, {160,32740,432,160}, {48,32739,31808,47}};
    const ChorusVector heldout[]{
        ChorusVector{160,32766,32448,159}, {160,32764,31808,160}, {160,32761,30688,160},
        {160,32759,29248,160}, {160,32756,27328,160}, {160,32754,25088,160},
        {160,32752,22528,160}, {48,32751,21712,48}, {160,32748,18512,160},
        {160,32746,14992,160}, {160,32744,11152,160}, {48,32743,9952,48},
        {160,32740,5472,160}, {160,32738,672,160}, {160,32735,28160,159}, {48,32735,26576,48}};
    const ChorusVector frequency_29994[]{
        ChorusVector{160,32766,32448,159}, {160,32763,31648,160}, {160,32761,30528,160},
        {128,32759,29376,128}, {160,32756,27456,160}, {160,32754,25216,160},
        {160,32751,22496,160}, {16,32751,22224,16}, {160,32748,19024,160},
        {160,32745,15344,160}, {160,32743,11344,160}, {16,32743,10944,16},
        {160,32740,6464,160}, {160,32737,1504,160}, {160,32735,28992,159}, {16,32735,28464,16}};
    chorus_vectors(1537, 733, 32006, canonical);
    chorus_vectors(513, 2500, 32006, heldout);
    chorus_vectors(512, 2500, 29994, frequency_29994);
}
void chorus_partition_wrap_and_zero_pull() {
    Core whole, fragmented, partitioned;
    const std::array sections{modulated()};
    for (auto* core : {&whole, &fragmented, &partitioned}) core->program(sections);
    const auto initial = fragmented.state().chorus[0];
    check(d::render_effects(fragmented.context(), fragmented.state(), 0, 64, 160).frames == 0 &&
          same_chorus(fragmented.state().chorus[0], initial) &&
          d::render_effects(fragmented.context(), fragmented.state(), 64, 32, 160).frames == 0 &&
          same_chorus(fragmented.state().chorus[0], initial),
          "callback fragments do not advance phase, carry, drift or INIT before all four planes exist");
    check(d::render_effects(fragmented.context(), fragmented.state(), 96, 64, 160).frames == 160 &&
          d::render_effects(whole.context(), whole.state(), 0, 160, 160).frames == 160 &&
          same_chorus(fragmented.state().chorus[0], whole.state().chorus[0]) &&
          count(commands(fragmented), 5) == 1,
          "one original160 advances once after four-plane staging regardless of callback partition");
    d::render_effects(partitioned.context(), partitioned.state(), 0, 80, 80);
    d::render_effects(partitioned.context(), partitioned.state(), 0, 80, 80);
    check(partitioned.state().cursor == whole.state().cursor &&
          partitioned.state().chorus[0].carry == 32528 && whole.state().chorus[0].carry == 32448,
          "two genuine80 outers differ from one160: endpoint pitch is not a stateless sample curve");
    for (const std::uint32_t frames : {16u, 32u, 48u, 64u, 80u}) {
        whole.rewind_commands(); fragmented.rewind_commands();
        d::render_effects(whole.context(), whole.state(), 0, frames, frames);
        if (frames > 16) {
            const auto before = fragmented.state().chorus[0];
            d::render_effects(fragmented.context(), fragmented.state(), 0, 16, frames);
            check(same_chorus(fragmented.state().chorus[0], before), "short outer prefix is state neutral");
            d::render_effects(fragmented.context(), fragmented.state(), 16, frames - 16, frames);
        } else d::render_effects(fragmented.context(), fragmented.state(), 0, frames, frames);
        check(same_chorus(fragmented.state().chorus[0], whole.state().chorus[0]) &&
              count(commands(fragmented), 5) == 1, "all admitted short outer counts preserve once-per-outer ownership");
    }
    Core wrapped;
    const std::array wrapped_sections{modulated(513, 2500, 4)};
    wrapped.program(wrapped_sections, 640);
    wrapped.state().cursor = 1184;
    wrapped.state().chorus[0].drift = 6;
    d::render_effects(wrapped.context(), wrapped.state(), 0, 160, 160);
    const auto wrap_trace = commands(wrapped);
    std::vector<Command> loads;
    for (const auto& command : wrap_trace)
        if (command.opcode() == 4 && command.input >= 0x280) loads.push_back(command);
    check(loads.size() == 2 && loads[0].second == d::effects_ring_address + 1176 &&
          loads[0].input == 0x280 && loads[0].bytes == 104 &&
          loads[1].second == d::effects_ring_address && loads[1].input == 0x2e8 && loads[1].bytes == 220 &&
          wrapped.state().chorus[0].drift == 4 && wrapped.state().cursor == 224 &&
          std::any_of(wrap_trace.begin(), wrap_trace.end(), [](const auto& command) {
              return command.opcode() == 5 && command.input == 0x286 && command.bytes == 320;
          }), "unaligned chorus tap wraps at ring end, assembles a whole input and restores full RS count");
    for (const bool continuing : {false, true}) {
        Core zero;
        zero.context().config.synthesis_rate = 29994;
        const std::array zero_sections{modulated(93731, 0, 200)};
        zero.program(zero_sections, 800);
        // Internal structural probe only: depth173100 is not a safe admitted
        // musical lifetime because its positive half-cycle overwrites dry440.
        // Preserve the Original zero-pull observation without claiming that
        // whole lifetime or correcting its separate extreme-depth residuals.
        zero.context().effects.sections[0].chorus->depth = 173100;
        zero.context().effects.chorus[0].amplitude =
            static_cast<float>(173100.0 * (std::numbers::ln2 / 120000.0));
        zero.state().chorus[0].initialized = continuing;
        zero.state().chorus[0].carry = continuing ? 800 : 0;
        const auto before = mutable_arena(zero);
        d::render_effects(zero.context(), zero.state(), 0, 160, 160);
        const auto trace = commands(zero);
        check(std::any_of(trace.begin(), trace.end(), [](const auto& command) {
                  return command.opcode() == 4 && command.input == 0x280 && command.bytes == 0;
              }) && std::any_of(trace.begin(), trace.end(), [continuing](const auto& command) {
                  return command.first == (0x05000005u | (continuing ? 0u : 0x10000u)) && command.bytes == 320;
              }) && zero.state().chorus[0].carry == (continuing ? 1600 : 800) &&
              zero.state().chorus[0].drift == 1280 && mutable_arena(zero) == before,
              "zero pull retains real LOAD0/RS INIT or continuation, no fabricated input, and full32B state binding");
    }
    // Synthetic exact boundary documents our measured-recurrence policy; the
    // Original collector did not independently force phase exactly +0.5.
    Core phase;
    phase.context().config.synthesis_rate = 32000;
    const std::array phase_sections{modulated(100000, 0)};
    phase.program(phase_sections);
    for (const float expected : {0.5f, -0.25f, 0.0f, 0.25f, 0.5f, -0.25f}) {
        phase.rewind_commands();
        d::render_effects(phase.context(), phase.state(), 0, 160, 160);
        check(phase.state().chorus[0].phase == expected && phase.state().chorus[0].carry == 0 &&
              phase.state().chorus[0].drift == 0 && count(commands(phase), 5) == 1,
              "centered phase wraps one period; positive rate with zero depth never bypasses RS");
    }
}
void combined_chorus_poles_and_save_branches() {
    Core core;
    std::array<e::Section, e::max_sections> sections{};
    for (std::size_t i = 0; i < sections.size(); ++i) {
        sections[i] = modulated(513, 2500, 256 + static_cast<std::uint32_t>(i) * 4u);
        if ((i & 1u) != 0) sections[i].pole = pole();
        if ((i & 2u) != 0) sections[i].feedback = 7100;
        if ((i & 4u) != 0) sections[i].feedforward = -8100;
    }
    core.program(sections);
    const auto before = mutable_arena(core);
    d::render_effects(core.context(), core.state(), 0, 160, 160);
    const auto trace = commands(core);
    check(count(trace, 5) == 16 && count(trace, 14) == 8 && count(trace, 6) == 9 &&
          core.state().initialized_poles == 0xaaaa && mutable_arena(core) == before,
          "every combination of FB/FF/pole and zero gain runs chorus, with input SAVE iff FB and no output SAVE");
    bool extents = d::effects_section_stride == 80 && d::effects_staging_address == 0x13d100 &&
        d::effects_staging_address + d::effects_staging_bytes == 0x13d600 &&
        a::AudioService::resource_address == 0x140000;
    for (std::size_t i = 0; i < sections.size(); ++i) {
        extents = extents && d::effects_history_address(i) == d::effects_book_address(i) + 32 &&
            d::effects_resample_address(i) == d::effects_book_address(i) + 40 &&
            d::effects_resample_address(i) + 32 <= d::effects_book_address(i) + 80;
        extents = extents && std::any_of(trace.begin(), trace.end(), [i](const auto& command) {
            return command.opcode() == 5 && command.second == d::effects_resample_address(i);
        });
    }
    check(extents, "all sixteen native books/pole8/RS32 extents are disjoint below unchanged catalog boundary");
    check(std::all_of(trace.begin(), trace.end(), [](const auto& command) {
        return command.opcode() != 6 || (command.bytes == 320 &&
            (command.output == 0x6c0 || command.output == 0));
    }), "feedback commits only nominal unmodulated input windows, never aligned/drifted output windows");
    Core static_core, rate_zero;
    auto static_section = filtered();
    auto rate_zero_section = static_section;
    rate_zero_section.chorus = e::Chorus{0, std::numeric_limits<std::uint32_t>::max()};
    static_core.program({&static_section, 1}); rate_zero.program({&rate_zero_section, 1});
    d::render_effects(static_core.context(), static_core.state(), 0, 160, 160);
    d::render_effects(rate_zero.context(), rate_zero.state(), 0, 160, 160);
    const auto expected = commands(static_core), actual = commands(rate_zero);
    check(expected.size() == actual.size() && std::equal(expected.begin(), expected.end(), actual.begin(),
          [](const auto& left, const auto& right) { return left.first == right.first && left.second == right.second; }),
          "optional rate-zero chorus retains the complete existing static command tape exactly");
}
void chorus_admission_shadow_rollback_and_reset() {
    Core core;
    auto section = filtered();
    section.chorus = e::Chorus{7600, 700};
    core.program({&section, 1});
    const auto before = mutable_arena(core);
    const auto initial = core.state().chorus[0];
    auto& context = core.context();
    const auto speculative = d::prepare_shadow(context);
    context.write_address = a::AudioService::command_limit - 15u * 8u;
    check(d::render_effects(context, context.cpu[speculative].effects, 0, 160, 160).frames == 0 &&
          context.write_error == a::Error::command_capacity &&
          !same_chorus(context.cpu[speculative].effects.chorus[0], initial) &&
          same_chorus(core.state().chorus[0], initial) && core.state().cursor == 0 &&
          mutable_arena(core) == before,
          "post-RS command failure discards speculative phase/carry/drift/INIT with no mutable DSP writes");
    core.rewind_commands();
    const auto retry = d::prepare_shadow(context);
    check(same_chorus(context.cpu[retry].effects.chorus[0], initial) &&
          d::render_effects(context, context.cpu[retry].effects, 0, 16, 16).frames == 16 &&
          context.cpu[retry].effects.chorus[0].initialized,
          "fresh shadow retry copies unadvanced live chorus rather than failed bank progress");
    for (auto& bank : context.cpu) {
        bank.effects.chorus[0] = {0.4f, 1000, 12345, true};
        bank.effects.cursor = 800;
        bank.effects.initialized_poles = 1;
    }
    for (std::size_t i = 0; i < e::max_sections; ++i)
        std::fill_n(context.memory.get() + d::effects_history_address(i) / 4u,
                    d::effects_mutable_section_bytes / 4u, 0xaabbccddu);
    const auto book = context.memory[d::effects_book_address(0) / 4u + 4u];
    d::reset_effects(context);
    bool reset = context.memory[d::effects_book_address(0) / 4u + 4u] == book;
    for (const auto& bank : context.cpu)
        for (const auto& chorus : bank.effects.chorus)
            reset = reset && same_chorus(chorus, initial);
    for (std::size_t i = 0; i < e::max_sections; ++i)
        reset = reset && std::all_of(context.memory.get() + d::effects_history_address(i) / 4u,
            context.memory.get() + (d::effects_history_address(i) + d::effects_mutable_section_bytes) / 4u,
            [](auto word) { return word == 0; });
    check(reset && context.cpu[0].effects.cursor == 0 && context.cpu[1].effects.cursor == 0 &&
          context.cpu[0].effects.initialized_poles == 0 && context.cpu[1].effects.initialized_poles == 0,
          "quiescent reset reinitializes both CPU banks and all pole8/RS32 histories without touching books");
    for (const auto depth : {50000u, 60000u}) {
        const auto admitted = modulated(513, depth);
        check(d::validate_effects({e::Routing::network, 1024, {&admitted, 1}}) == a::Error::none,
              "geometry admits safe large depths, including the open depth50000 arithmetic counterexample");
    }
    for (const auto depth : {75000u, 125000u, 150000u, 173100u, std::numeric_limits<std::uint32_t>::max()}) {
        const auto rejected = modulated(513, depth);
        check(d::build_effects(context, {e::Routing::network, 1024, {&rejected, 1}}) == a::Error::invalid_parameter &&
              context.effects.sections[0].chorus->depth == 700,
              "unsafe worst-case DMA/pitch config is rejected at creation without replacing immutable program");
    }
    const auto excessive_rate = modulated(std::numeric_limits<std::uint32_t>::max(), 0);
    check(d::build_effects(context, {e::Routing::network, 1024, {&excessive_rate, 1}}) == a::Error::invalid_parameter &&
          context.effects.sections[0].chorus->rate == 7600,
          "creation rejects rates needing more than the measured single phase-wrap operation");
    // Deliberately violate the immutable-program invariant internally to prove
    // the runtime geometry guard, not an admitted public extreme-depth lifetime.
    context.effects.chorus[0].amplitude = 0.9f;
    core.state().chorus[0].phase = 0.0f;
    const auto live = core.state().chorus[0];
    const auto guarded = d::prepare_shadow(context);
    core.rewind_commands();
    check(d::render_effects(context, context.cpu[guarded].effects, 0, 160, 160).frames == 0 &&
          context.write_error == a::Error::address_range && same_chorus(core.state().chorus[0], live),
          "actual rounded LOAD overflow is a transactional invariant error, never silent clamp or bus overwrite");
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
    auto section = filtered();
    section.chorus = e::Chorus{513, 2500};
    const std::array sections{section};
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
    check(count(trace, 14) == 2 && count(trace, 13) == 2 && core.state().cursor == 640 &&
          std::count_if(trace.begin(), trace.end(), [](const auto& command) {
              return command.opcode() == 5 && command.second == d::effects_resample_address(0);
          }) == 2, "multiple parameter fragments do not become multiple wet outers or output mixes");
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
    const auto chorus = core.state().chorus[0];
    check(d::stop_voice(c, state, voice, {320}) == a::Error::none &&
          d::free_voice(c, state, voice, {320}) == a::Error::none &&
          core.state().cursor == cursor && core.state().initialized_poles == initialized &&
          same_chorus(core.state().chorus[0], chorus),
          "individual stop/free never resets the common wet ring/pole/chorus tail");
    core.rewind_commands();
    check(d::render(c, state, 48, 0, 48) == a::Error::none && core.state().cursor == cursor + 96 &&
          count(commands(core), 14) == 1 && count(commands(core), 5) == 1 && count(commands(core), 1) == 0 &&
          core.state().chorus[0].phase != chorus.phase && core.state().chorus[0].initialized,
          "silent-source final partial still evolves global ring, pole and chorus history");
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
        sections[0].chorus = e::Chorus{1537, 733};
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
    check(service.build({1, 0, 208}, task) == a::Error::none, "real service callbacks fragment a chorus wet task");
    const auto trace = commands(service, task);
    check(count(trace, 14) == 2 && count(trace, 13) == 2 && count(trace, 5) == 2 &&
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
    auto section = filtered();
    section.chorus = e::Chorus{7600, 700};
    const std::array sections{section};
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
          std::any_of(trace.begin(), trace.end(), [](const auto& command) { return command.first == 0x0e011200u; }) &&
          std::any_of(trace.begin(), trace.end(), [](const auto& command) {
              return command.first == 0x05017fffu && command.second == d::effects_resample_address(0);
          }), "retry uses rolled-back cursor, phase, carry and fresh pole/RS INIT, not rejected shadow progress");
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
        std::fill_n(memory.begin() + d::effects_history_address(i) / 4u,
                    d::effects_mutable_section_bytes / 4u, 0x11223344u);
    std::fill_n(memory.begin() + d::effects_staging_address / 4u, d::effects_staging_bytes / 4u, 0x55667788u);
    check(service.reset() == a::Error::none && service.rdram_words().data() == stable,
          "quiescent reset clears poison without replacing stable RDRAM");
    bool cleared = std::all_of(memory.begin() + d::effects_ring_address / 4u,
        memory.begin() + (d::effects_ring_address + d::effects_ring_bytes) / 4u, [](auto value) { return value == 0; });
    for (std::size_t i = 0; i < e::max_sections; ++i)
        cleared = cleared && std::all_of(memory.begin() + d::effects_history_address(i) / 4u,
            memory.begin() + (d::effects_history_address(i) + d::effects_mutable_section_bytes) / 4u,
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
          std::any_of(trace.begin(), trace.end(), [](const auto& command) { return command.first == 0x05017fffu; }) &&
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
    normal_and_heldout_chorus_arithmetic();
    chorus_partition_wrap_and_zero_pull();
    combined_chorus_poles_and_save_branches();
    chorus_admission_shadow_rollback_and_reset();
    fragment_staging_and_task_relative_final_partial();
    parameter_fragments_voice_books_and_global_tail();
    borrowed_lifetime_and_callback_fragments();
    rollback_poison_reset_and_immutable_lifetime();
    direct_preserves_per_fragment_return();
    if (failures) return EXIT_FAILURE;
    std::cout << "Wet program ordering, chorus arithmetic, bounds, fragmentation and ownership checks passed\n";
    return EXIT_SUCCESS;
}
