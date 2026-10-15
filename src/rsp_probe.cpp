#include "rsp_fixture.hpp"
#ifdef ISLE_HAS_CANONICAL_RDP
#include "rsp_render.hpp"
#endif

#include "rom_snapshot.hpp"
#include "35mmisle/rsp.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace {
namespace probe = isle::rsp_probe;
constexpr std::string_view snap_pin = "df8815e75f3d79618466e01e0827c6165b93d11c";
constexpr std::string_view rsp_pin = "9ab776a2ce28899769c4f911403234cac62fc799";
#ifdef _WIN32
using Argument = wchar_t;
constexpr std::wstring_view rom_argument = L"--rom";
constexpr std::wstring_view raw_argument = L"--raw-out";
constexpr std::wstring_view render_argument = L"--render-out";
constexpr std::wstring_view cpu_argument = L"--cpu-device";
constexpr std::wstring_view help_argument = L"--help";
#else
using Argument = char;
constexpr std::string_view rom_argument = "--rom";
constexpr std::string_view raw_argument = "--raw-out";
constexpr std::string_view render_argument = "--render-out";
constexpr std::string_view cpu_argument = "--cpu-device";
constexpr std::string_view help_argument = "--help";
#endif

void usage(std::ostream& output) {
    output << "35mmisle-rsp-probe --rom PRIVATE_ROM [--raw-out NEW_DIRECTORY]\n"
              "  [--render-out NEW_DIRECTORY [--cpu-device]]\n"
              "35mmisle-rsp-probe --help\n"
              "Private exact-US-ROM coprocessor-only synthetic task experiment.\n"
              "Runs fractional/negative-partial matrix geometry and an in-list graphics/line/graphics switch.\n"
              "Optional canonical execution requires both ISLE_BUILD_RSP_PROBE and ISLE_BUILD_RDP_PROBE.\n"
              "No R4300/game CPU, full scene, VI, hardware/frontend parity or GAME scene qualification claim.\n"
              "Outputs contain only authored target snapshots, validated synthetic packets and metadata; no ROM dump.\n";
}

struct SinkContext {
    isle::rsp::PacketBuffer* capture = nullptr;
    static bool submit(void* context, std::span<const std::uint32_t> words) noexcept {
        const auto& sink = *static_cast<SinkContext*>(context);
        return sink.capture && isle::rsp::PacketBuffer::sink(sink.capture, words);
    }
};

const char* error_name(isle::rsp::Error error) noexcept {
    switch (error) {
    case isle::rsp::Error::none: return "none";
    case isle::rsp::Error::invalid_task: return "invalid_task";
    case isle::rsp::Error::address_out_of_range: return "address_out_of_range";
    case isle::rsp::Error::execution_budget: return "execution_budget";
    case isle::rsp::Error::malformed_packet: return "malformed_packet";
    case isle::rsp::Error::packet_capacity: return "packet_capacity";
    case isle::rsp::Error::sink_rejected: return "sink_rejected";
    case isle::rsp::Error::unexpected_stop: return "unexpected_stop";
    }
    return "unknown";
}

void report(const char* label, const isle::rsp::Result& result) {
    std::cout << "task=" << label << " error=" << error_name(result.error)
              << " sp_status=0x" << std::hex << result.sp_status << std::dec
              << " dma_reads=" << result.dma_reads << " dma_writes=" << result.dma_writes
              << " imem_writes=" << result.imem_writes << " packets=" << result.packets
              << " packet_words=" << result.packet_words << " full_syncs=" << result.full_syncs << '\n';
}

void write_words(const std::filesystem::path& path, const isle::rsp::PacketBuffer& capture) {
    std::ofstream output(path, std::ios::binary);
    output.exceptions(std::ios::badbit | std::ios::failbit);
    std::array<char, 4096> buffer{};
    const auto words = capture.words();
    for (std::size_t begin = 0; begin < words.size(); begin += buffer.size() / 4) {
        const auto count = std::min(buffer.size() / 4, words.size() - begin);
        for (std::size_t index = 0; index < count; ++index) {
            const auto value = words[begin + index];
            buffer[index * 4] = static_cast<char>(value >> 24);
            buffer[index * 4 + 1] = static_cast<char>(value >> 16);
            buffer[index * 4 + 2] = static_cast<char>(value >> 8);
            buffer[index * 4 + 3] = static_cast<char>(value);
        }
        output.write(buffer.data(), static_cast<std::streamsize>(count * 4));
    }
    output.close();
}

void write_task(std::ostream& output, const isle::rsp::Task& task) {
    output << '[' << task.type << ',' << task.flags << ',' << task.boot_address << ',' << task.boot_size
           << ',' << task.text_address << ',' << task.text_size << ',' << task.data_address << ',' << task.data_size
           << ',' << task.stack_address << ',' << task.stack_size << ',' << task.output_address << ',' << task.output_end
           << ',' << task.commands_address << ',' << task.commands_size << ',' << task.yield_address << ',' << task.yield_size << ']';
}

void write_capture(std::ostream& output, const char* file, const isle::rsp::PacketBuffer& capture) {
    output << "{\"file\":\"" << file << "\",\"word_count\":" << capture.words().size() << ",\"packets\":[";
    for (std::size_t index = 0; index < capture.packets().size(); ++index) {
        const auto& packet = capture.packets()[index];
        output << (index ? "," : "") << "{\"word_offset\":" << packet.offset
               << ",\"word_count\":" << packet.count << '}';
    }
    output << "]}";
}

void write_metadata(const std::filesystem::path& directory, const isle::rom::Snapshot& snapshot,
                    const probe::Fixture& fixture, const isle::rsp::PacketBuffer& matrix,
                    const isle::rsp::PacketBuffer& reload, const std::array<isle::rsp::Result, 5>& results) {
    std::ofstream output(directory / "metadata.json");
    output.exceptions(std::ios::badbit | std::ios::failbit);
    output << "{\n  \"schema_version\":1,\n"
              "  \"scope\":\"private exact-ROM RSP input, original synthetic geometry; no ROM payload, R4300, scene, RDP render or parity claim\",\n"
           << "  \"snap_source_commit\":\"" << snap_pin << "\",\n"
           << "  \"rsp_source_commit\":\"" << rsp_pin << "\",\n"
           << "  \"rom_normalized_sha1\":\"" << isle::rom::digest_hex(snapshot.identity()) << "\",\n"
              "  \"fingerprint_basis\":\"resource SHA1 constants are consequences of exact full-snapshot admission and pinned ranges; not independent frontend verification\",\n"
              "  \"layout_source\":\"pinned splat.yaml main segment and src/entry/entry.s clear_bss\",\n"
              "  \"task_source\":\"pinned src/sys/gtl.c:297-364; in-list switch interface 426-439\",\n"
              "  \"boot_source\":\"pinned src/sys/main.c:51,114: gRspBootCode[0x20] u64, ROM offset 0xb70 via PHYSICAL_TO_ROM; not linked rspbootTextStart\",\n"
              "  \"matrix_interface_source\":\"pinned src/sys/render.c:8-9,499-527 and PR/gbi.h canonical field facts\",\n"
           << "  \"main_mapping\":{\"rom_begin\":" << probe::main_rom_begin
           << ",\"rom_end\":" << probe::main_rom_end << ",\"ram_begin\":" << probe::main_ram_begin
           << ",\"ram_end\":" << probe::main_bss_begin << "},\n"
           << "  \"entry_cleared_main_bss\":{\"begin\":" << probe::main_bss_begin
           << ",\"end\":" << probe::main_bss_end << ",\"byte_value\":0},\n"
              "  \"resources\":[\n"
           << "    {\"name\":\"boot\",\"rom_begin\":" << probe::boot_rom_begin
           << ",\"task_dma_bytes\":256,\"sha1\":\"95f758ce017375682479347697d0904896b1e3c5\"},\n"
           << "    {\"name\":\"graphics_text\",\"rom_begin\":" << probe::graphics_text_rom
           << ",\"symbol_bytes\":5008,\"symbol_sha1\":\"1d7a077766a9f5408c01472649f9b8ea690337e3\",\"task_dma_bytes\":4096,\"task_dma_sha1\":\"56e161decdd96f9154fb0b9e3a512e0b64dd8757\"},\n"
           << "    {\"name\":\"graphics_data\",\"rom_begin\":" << probe::graphics_data_rom
           << ",\"symbol_bytes\":1056,\"symbol_sha1\":\"4c68ae62d1c06fae55e1a46f827dc32202c0a220\",\"task_dma_bytes\":2048,\"adjacent_line_data_bytes\":992,\"task_dma_sha1\":\"9d8d5066684c59e1dcea627fc8677d95502964da\"},\n"
           << "    {\"name\":\"line_text\",\"rom_begin\":" << probe::line_text_rom
           << ",\"symbol_bytes\":4496,\"symbol_sha1\":\"86d3b76fd0f5a068acbf5b7efd6de250024c7b04\",\"task_dma_bytes\":4096,\"task_dma_sha1\":\"06914932aecd0c97d5c2caed5ef479c601d88daa\"},\n"
           << "    {\"name\":\"line_data\",\"rom_begin\":" << probe::line_data_rom
           << ",\"symbol_bytes\":1008,\"symbol_sha1\":\"8fad4940c55d45c11e8f0c0df038db7f90d27499\",\"task_dma_bytes\":2048,\"entry_cleared_bss_bytes\":1040,\"task_dma_sha1\":\"4d9800a53f1ef785e91f5b0679b7364b7d1af1bc\"}\n  ],\n"
           << "  \"rdram_bytes\":" << probe::rdram_bytes << ",\n"
              "  \"rdram_layout\":\"word-swapped u32 storage: canonical big-endian numeric word a at u32[a/4]; byte a at host byte a^3\",\n"
              "  \"packet_encoding\":\"flat canonical big-endian u32 RDP words; packet boundaries below; no ROM/RDRAM/DMEM/IMEM dump\",\n"
           << "  \"task_record_address\":" << probe::task_record_address << ",\n  \"task_console_u32_fields\":";
    write_task(output, fixture.task);
    output << ",\n  \"width\":" << probe::width << ",\"height\":" << probe::height
           << ",\"color_address\":" << probe::color_address << ",\n"
           << "  \"limits\":{\"jit_block_entries\":1000000,\"packet_words\":" << probe::capture_words << "},\n"
              "  \"stimulus\":\"one authored opaque-black full-target RGBA5551 fill before all draw phases; nonidentity fractional projection/modelview; SPECIAL_1 recalc; combined XX/XY integer=-1,-1 and fraction=.75,.875; distinct shaded vertex colors; no texture/light/Z/cull; baseline versus partial geometry and graphics-line-graphics reload\",\n"
              "  \"repeat_checks\":\"two cold coprocessor runs per case and warm graphics repeat after switched task in one separately allocated persistent RSP-only RAM session; only explicit per-task workspace is prepared; compare packet boundaries, semantic operands and DMA/status counters; ignored NOOP/sync operands excluded\",\n"
              "  \"captures\":{\"fractional_matrix\":";
    write_capture(output, "fractional-matrix-rdp.bin", matrix);
    output << ",\"in_list_graphics_line_graphics\":";
    write_capture(output, "in-list-reload-rdp.bin", reload);
    output << "},\n  \"runs\":[";
    constexpr std::array labels{"matrix-cold-1", "matrix-cold-2", "reload-cold-1", "reload-cold-2", "matrix-after-reload-warm"};
    for (unsigned index = 0; index < results.size(); ++index) {
        const auto& result = results[index];
        output << (index ? "," : "") << "{\"name\":\"" << labels[index] << "\",\"error\":\"" << error_name(result.error)
               << "\",\"sp_status\":" << result.sp_status << ",\"dma_reads\":" << result.dma_reads
               << ",\"dma_writes\":" << result.dma_writes << ",\"imem_writes\":" << result.imem_writes
               << ",\"packets\":" << result.packets << ",\"packet_words\":" << result.packet_words
               << ",\"full_syncs\":" << result.full_syncs << '}';
    }
    output << "]\n}\n";
    output.close();
}

#ifdef ISLE_HAS_CANONICAL_RDP
void write_binary(const std::filesystem::path& path, std::span<const std::byte> bytes) {
    std::ofstream output(path, std::ios::binary);
    output.exceptions(std::ios::badbit | std::ios::failbit);
    output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    output.close();
}

// Diagnostic exports deliberately live in the CLI, not in the general backend.
// Only this authored color target, its guards/coverage, and original packet
// words may leave the real result lease. Never export main, boot, or whole RAM.
void write_rendered_fixture(const std::filesystem::path& directory,
                            const isle::rsp::CanonicalExecutor::MemoryLease& completed,
                            const char* label) {
    constexpr std::size_t color_bytes = probe::width * probe::height * 2;
    constexpr std::size_t guard_bytes = 64;
    constexpr std::size_t snapshot_begin = probe::color_address - guard_bytes;
    constexpr std::size_t snapshot_end = probe::color_address + color_bytes + guard_bytes;
    const auto memory = completed.rdram();
    const auto hidden = completed.hidden();
    if (!std::filesystem::is_directory(directory) || !std::filesystem::is_empty(directory) ||
        memory.size() != probe::rdram_bytes || hidden.size() != probe::rdram_bytes / 2)
        throw std::runtime_error("canonical fixture export requires an empty directory and actual completed mappings");
    for (std::size_t offset = 0; offset < guard_bytes; ++offset) {
        if (memory[snapshot_begin + offset] != std::byte{0xa5} ||
            memory[probe::color_address + color_bytes + offset] != std::byte{0xa5})
            throw std::runtime_error("canonical fixture changed a color-target guard byte");
    }
    for (std::size_t offset = 0; offset < guard_bytes / 2; ++offset) {
        if (hidden[snapshot_begin / 2 + offset] != std::byte{3} ||
            hidden[(probe::color_address + color_bytes) / 2 + offset] != std::byte{3})
            throw std::runtime_error("canonical fixture changed a hidden color-target guard byte");
    }
    write_binary(directory / "rdram.bin", memory.subspan(snapshot_begin, snapshot_end - snapshot_begin));
    write_binary(directory / "hidden.bin", hidden.subspan(snapshot_begin / 2, (snapshot_end - snapshot_begin) / 2));
    write_words(directory / "rdp.bin", completed.packets());
    {
        std::ofstream output(directory / "color.ppm", std::ios::binary);
        output.exceptions(std::ios::badbit | std::ios::failbit);
        output << "P6\n" << probe::width << ' ' << probe::height << "\n255\n";
        std::array<char, probe::width * 3> row;
        for (unsigned y = 0; y < probe::height; ++y) {
            for (unsigned x = 0; x < probe::width; ++x) {
                const auto address = probe::color_address + (y * probe::width + x) * 2;
                const unsigned rgba = (std::to_integer<unsigned>(memory[address ^ 3u]) << 8) |
                                      std::to_integer<unsigned>(memory[(address + 1) ^ 3u]);
                for (unsigned channel = 0; channel < 3; ++channel) {
                    const unsigned five = (rgba >> (11 - channel * 5)) & 31u;
                    row[x * 3 + channel] = static_cast<char>((five << 3) | (five >> 2));
                }
            }
            output.write(row.data(), static_cast<std::streamsize>(row.size()));
        }
        output.close();
    }
    // Written last while the real result remains leased. No context/processor
    // internals or successful completion metadata are exposed on backend failure.
    const auto& result = completed.rsp_result();
    const auto& packets = completed.packets();
    std::ofstream output(directory / "metadata.json");
    output.exceptions(std::ios::badbit | std::ios::failbit);
    output << "{\n  \"schema_version\":2,\n  \"label\":\"" << label << "\",\n"
              "  \"scope\":\"actual ROM-bound persistent RSP and RDP on the same processor-owned canonical RAM; authored synthetic target only; NOT GAME scene, photo, VI, hardware or frontend parity\",\n"
              "  \"renderer_commit\":\"1cecd042b2619bc505c12bfdc713808386f2b54d\",\n"
           << "  \"snap_source_commit\":\"" << snap_pin << "\",\n"
           << "  \"rsp_source_commit\":\"" << rsp_pin << "\",\n"
              "  \"completion\":\"actual Machine::execute, flush CPU and RSP writes, staged packet enqueue, CommandProcessor::idle and successful vkDeviceWaitIdle before CPU read mapping; FullSync is not host completion\",\n"
              "  \"timeline_complete\":true,\n  \"backend_errors\":0,\n"
              "  \"memory_layout\":\"little-endian word-swapped N64: canonical byte a at host byte[a^3], halfword a at host u16[(a/2)^1], word a at host u32[a/4]; hidden byte at a/2\",\n"
              "  \"memory_lifetime\":\"one persistent processor-owned allocation; hidden initialized once cold; main/BSS/boot admitted once; no per-pass RAM/hidden reset\",\n"
           << "  \"rdram_size\":" << probe::rdram_bytes << ",\n  \"hidden_size\":" << probe::rdram_bytes / 2
           << ",\n  \"initial_target_byte\":165,\n  \"initial_hidden_byte\":3,\n"
              "  \"authored_clear\":{\"count\":1,\"rgba5551\":1,\"fill_word\":65537,\"before_all_phases\":true},\n"
           << "  \"width\":" << probe::width << ",\n  \"height\":" << probe::height
           << ",\n  \"color_format\":\"RGBA5551\",\n  \"color_base\":" << probe::color_address
           << ",\n  \"plane_bytes\":" << color_bytes << ",\n  \"guard_bytes_each_side\":" << guard_bytes
           << ",\n  \"rdram_file\":\"rdram.bin\",\n  \"rdram_file_offset\":" << snapshot_begin
           << ",\n  \"rdram_file_bytes\":" << snapshot_end - snapshot_begin
           << ",\n  \"hidden_file\":\"hidden.bin\",\n  \"hidden_file_offset\":" << snapshot_begin / 2
           << ",\n  \"hidden_file_bytes\":" << (snapshot_end - snapshot_begin) / 2
           << ",\n  \"ppm\":\"direct RGBA5551 RGB bit-replication; alpha/coverage ignored, no VI filtering\",\n"
              "  \"rsp_result\":{\"error\":\"" << error_name(result.error)
           << "\",\"sp_status\":" << result.sp_status << ",\"dma_reads\":" << result.dma_reads
           << ",\"dma_writes\":" << result.dma_writes << ",\"imem_writes\":" << result.imem_writes
           << ",\"packets\":" << result.packets << ",\"packet_words\":" << result.packet_words
           << ",\"full_syncs\":" << result.full_syncs << "},\n"
              "  \"words_encoding\":\"canonical host-value RDP u32 integers, not raw RDRAM byte order; rdp.bin contains big-endian numeric words\",\n  \"words\":[";
    for (std::size_t index = 0; index < packets.words().size(); ++index)
        output << (index ? "," : "") << packets.words()[index];
    output << "],\n  \"packets\":[";
    for (std::size_t index = 0; index < packets.packets().size(); ++index) {
        const auto& packet = packets.packets()[index];
        output << (index ? "," : "") << "{\"word_offset\":" << packet.offset
               << ",\"word_count\":" << packet.count << '}';
    }
    output << "]\n}\n";
    output.close();
}
#endif

int run(int argc, Argument* argv[]) {
    if (argc == 2 && std::basic_string_view<Argument>(argv[1]) == help_argument) {
        usage(std::cout);
        std::cout.flush();
        return std::cout ? 0 : 1;
    }
    std::filesystem::path rom_path, raw_directory, render_directory;
    bool cpu_device = false;
    for (int index = 1; index < argc; ++index) {
        const std::basic_string_view<Argument> argument(argv[index]);
        if (argument == rom_argument && index + 1 < argc && rom_path.empty()) {
            rom_path = argv[++index];
            if (rom_path.empty()) throw std::runtime_error("--rom requires a native filesystem path");
        } else if (argument == raw_argument && index + 1 < argc && raw_directory.empty()) {
            raw_directory = argv[++index];
            if (raw_directory.empty()) throw std::runtime_error("--raw-out requires a nonempty new directory");
        } else if (argument == render_argument && index + 1 < argc && render_directory.empty()) {
            render_directory = argv[++index];
            if (render_directory.empty()) throw std::runtime_error("--render-out requires a nonempty new directory");
        } else if (argument == cpu_argument && !cpu_device) {
            cpu_device = true;
        } else {
            usage(std::cerr);
            throw std::runtime_error("unknown, repeated or incomplete private probe argument");
        }
    }
    if (rom_path.empty()) {
        usage(std::cerr);
        throw std::runtime_error("--rom is required; no implicit ROM search is performed");
    }
    if (cpu_device && render_directory.empty())
        throw std::runtime_error("--cpu-device requires --render-out");
#ifndef ISLE_HAS_CANONICAL_RDP
    if (!render_directory.empty())
        throw std::runtime_error("canonical RSP/RDP execution requires both probe build options");
#endif
    if constexpr (std::endian::native != std::endian::little)
        throw std::runtime_error("the word-swapped RSP backend requires a little-endian host");
    // std::filesystem::path consumes Linux argv bytes or Windows UTF-16 wmain
    // directly. No locale narrowing, path logging, mutation, or second ROM open.
    const auto snapshot = isle::rom::read_snapshot(rom_path);
    std::cout << "private_rom_admitted normalized_sha1=" << isle::rom::digest_hex(snapshot.identity())
              << " size=" << snapshot.identity().size << " scope=coprocessor-only\n";
    // This RSP-only experiment owns ONE honest, separate allocation. Do not
    // zero it before the explicit guard/main/BSS/boot cold initialization.
    auto memory_owner = std::make_unique_for_overwrite<std::uint32_t[]>(probe::rdram_bytes / 4);
    const std::span<std::uint32_t> memory(memory_owner.get(), probe::rdram_bytes / 4);
    probe::initialize_fixture_memory(memory, snapshot.canonical());
    probe::Fixture fixture(memory, probe::Case::matrix);
    isle::rsp::PacketBuffer matrix(probe::capture_words), reload(probe::capture_words),
                            repeat(probe::capture_words);
    SinkContext sink;
    isle::rsp::Machine machine(memory, SinkContext::submit, &sink);
    const auto boot = snapshot.canonical().subspan(probe::boot_rom_begin, probe::boot_bytes);
    const isle::rsp::Limits limits{1000000, probe::capture_words};
    auto execute = [&](probe::Case kind, const char* label, isle::rsp::PacketBuffer& capture, bool cold) {
        if (cold) machine.reset();
        capture.clear();
        sink.capture = &capture;
        const auto result = machine.execute(fixture.task, boot, limits);
        report(label, result);
        const auto geometry = probe::validate(fixture, kind, capture, result);
        std::cout << "geometry=" << probe::name(kind) << " shaded_triangles=" << geometry.triangles
                  << " line_triangles=" << geometry.line_triangles << " markers=" << geometry.markers << '\n';
        return result;
    };
    std::array<isle::rsp::Result, 5> results;
    results[0] = execute(probe::Case::matrix, "matrix-cold-1", matrix, true);
    fixture = probe::Fixture(memory, probe::Case::matrix);
    results[1] = execute(probe::Case::matrix, "matrix-cold-2", repeat, true);
    probe::require_repeat(matrix, results[0], repeat, results[1]);
    fixture = probe::Fixture(memory, probe::Case::reload);
    results[2] = execute(probe::Case::reload, "reload-cold-1", reload, true);
    fixture = probe::Fixture(memory, probe::Case::reload);
    results[3] = execute(probe::Case::reload, "reload-cold-2", repeat, true);
    probe::require_repeat(reload, results[2], repeat, results[3]);
    fixture = probe::Fixture(memory, probe::Case::matrix);
    results[4] = execute(probe::Case::matrix, "matrix-after-reload-warm", repeat, false);
    probe::require_repeat(matrix, results[0], repeat, results[4]);
    // Publish only after every real task, status, geometry, and repeat check passed.
    // Never dump the reconstructed RAM image: it contains private ROM payload.
    if (!raw_directory.empty()) {
        if (!std::filesystem::create_directory(raw_directory))
            throw std::runtime_error("--raw-out directory already exists; refusing to overwrite");
        write_words(raw_directory / "fractional-matrix-rdp.bin", matrix);
        write_words(raw_directory / "in-list-reload-rdp.bin", reload);
        write_metadata(raw_directory, snapshot, fixture, matrix, reload, results);
    }
    bool rendered = false;
#ifdef ISLE_HAS_CANONICAL_RDP
    if (!render_directory.empty()) {
        if (!std::filesystem::create_directory(render_directory))
            throw std::runtime_error("--render-out directory already exists; refusing to overwrite");
        const auto matrix_directory = render_directory / "fractional-matrix";
        const auto reload_directory = render_directory / "in-list-reload";
        if (!std::filesystem::create_directory(matrix_directory) ||
            !std::filesystem::create_directory(reload_directory))
            throw std::runtime_error("cannot create canonical fixture case directories");
        isle::rsp::CanonicalExecutor executor(cpu_device, limits);
        auto canonical_run = [&](probe::Case kind, const std::filesystem::path& directory,
                                 const isle::rsp::PacketBuffer& reference,
                                 const isle::rsp::Result& reference_result, bool initialize) {
            auto preparation = executor.begin_preparation();
            if (initialize)
                probe::initialize_fixture_memory(preparation.words(), snapshot.canonical());
            const probe::Fixture canonical_fixture(preparation.words(), kind);
            auto completed = executor.execute(std::move(preparation), canonical_fixture.task, boot);
            report(probe::name(kind), completed.rsp_result());
            probe::validate(canonical_fixture, kind, completed.packets(), completed.rsp_result());
            probe::require_repeat(reference, reference_result, completed.packets(), completed.rsp_result());
            write_rendered_fixture(directory, completed, probe::name(kind));
        };
        canonical_run(probe::Case::matrix, matrix_directory, matrix, results[0], true);
        canonical_run(probe::Case::reload, reload_directory, reload, results[2], false);
        rendered = true;
    }
#endif
    std::cout << "private_rsp_experiment=passed cold_repeats=2 warm_reload_repeat=1"
              << " rdp_rendered=" << (rendered ? "yes" : "no")
              << " independent_frontend_parity=not_claimed\n";
    std::cout.flush();
    if (!std::cout) throw std::runtime_error("cannot write private experiment report");
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
        std::cerr << "Private RSP experiment failed: private filesystem operation failed.\n";
        return 1;
    } catch (const std::ios_base::failure&) {
        std::cerr << "Private RSP experiment failed: private input or report I/O failed.\n";
        return 1;
    } catch (const std::runtime_error& error) {
        std::cerr << "Private RSP experiment failed: " << error.what() << ".\n";
        return 1;
    } catch (...) {
        // Filesystem/allocator diagnostics may expose a private native path.
        std::cerr << "Private RSP experiment failed: private input, allocation or reporting operation failed.\n";
        return 1;
    }
}
