#include "rsp_fixture.hpp"
#ifdef ISLE_HAS_CANONICAL_RDP
#include "rsp_render.hpp"
#endif

#include "35mmisle/rom.hpp"
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
#include <span>
#include <stdexcept>
#include <streambuf>
#include <string>
#include <string_view>
#include <vector>

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
              "Optional after-SP RDP replay requires both ISLE_BUILD_RSP_PROBE and ISLE_BUILD_RDP_PROBE.\n"
              "No R4300/game CPU, full scene, VI, shared production memory or hardware/frontend parity claim.\n"
              "Outputs contain only validated synthetic packets, replay and metadata; no ROM memory dump.\n";
}

class SnapshotBuffer final : public std::streambuf {
public:
    explicit SnapshotBuffer(std::span<char> bytes) {
        setg(bytes.data(), bytes.data(), bytes.data() + bytes.size());
    }
};

struct Snapshot {
    std::vector<char> bytes;
    isle::rom::Identity identity{};
    std::span<const std::byte> canonical() const noexcept {
        return std::as_bytes(std::span<const char>(bytes));
    }
};

Snapshot read_snapshot(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input.is_open()) throw std::runtime_error("cannot open private ROM for reading");
    Snapshot snapshot{std::vector<char>(static_cast<std::size_t>(isle::rom::us_size)), {}};
    input.read(snapshot.bytes.data(), static_cast<std::streamsize>(snapshot.bytes.size()));
    if (input.gcount() != static_cast<std::streamsize>(snapshot.bytes.size()) || input.bad())
        throw std::runtime_error("private ROM snapshot is incomplete or unreadable");
    char extra = 0;
    if (input.get(extra)) throw std::runtime_error("private ROM exceeds the exact admitted image size");
    if (!input.eof() || input.bad()) throw std::runtime_error("private ROM snapshot read failed");
    input.close();
    // Admission and extraction consume the SAME owned snapshot, never a second
    // pathname open vulnerable to replacement between fingerprinting and use.
    SnapshotBuffer buffer(snapshot.bytes);
    std::istream memory_input(&buffer);
    const auto admitted = isle::rom::validate_us(memory_input);
    if (!admitted) throw std::runtime_error("private ROM snapshot failed existing isle-rom exact-US admission");
    snapshot.identity = admitted.identity;
    for (std::size_t offset = 0; offset < snapshot.bytes.size(); offset += 4) {
        if (admitted.identity.order == isle::rom::ByteOrder::v64) {
            std::swap(snapshot.bytes[offset], snapshot.bytes[offset + 1]);
            std::swap(snapshot.bytes[offset + 2], snapshot.bytes[offset + 3]);
        } else if (admitted.identity.order == isle::rom::ByteOrder::n64) {
            std::swap(snapshot.bytes[offset], snapshot.bytes[offset + 3]);
            std::swap(snapshot.bytes[offset + 1], snapshot.bytes[offset + 2]);
        }
    }
    return snapshot;
}

struct SinkContext {
    probe::Capture* capture = nullptr;
    static bool submit(void* context, std::span<const std::uint32_t> words) noexcept {
        const auto& sink = *static_cast<SinkContext*>(context);
        return sink.capture && probe::Capture::sink(sink.capture, words);
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

void write_words(const std::filesystem::path& path, const probe::Capture& capture) {
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

void write_capture(std::ostream& output, const char* file, const probe::Capture& capture) {
    output << "{\"file\":\"" << file << "\",\"word_count\":" << capture.words().size() << ",\"packets\":[";
    for (std::size_t index = 0; index < capture.packets().size(); ++index) {
        const auto& packet = capture.packets()[index];
        output << (index ? "," : "") << "{\"word_offset\":" << packet.offset
               << ",\"word_count\":" << packet.count << '}';
    }
    output << "]}";
}

void write_metadata(const std::filesystem::path& directory, const Snapshot& snapshot,
                    const probe::Fixture& fixture, const probe::Capture& matrix,
                    const probe::Capture& reload, const std::array<isle::rsp::Result, 5>& results) {
    std::ofstream output(directory / "metadata.json");
    output.exceptions(std::ios::badbit | std::ios::failbit);
    output << "{\n  \"schema_version\":1,\n"
              "  \"scope\":\"private exact-ROM RSP input, original synthetic geometry; no ROM payload, R4300, scene, RDP render or parity claim\",\n"
           << "  \"snap_source_commit\":\"" << snap_pin << "\",\n"
           << "  \"rsp_source_commit\":\"" << rsp_pin << "\",\n"
           << "  \"rom_normalized_sha1\":\"" << isle::rom::digest_hex(snapshot.identity) << "\",\n"
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
              "  \"stimulus\":\"nonidentity fractional projection/modelview; SPECIAL_1 recalc; combined XX/XY integer=-1,-1 and fraction=.75,.875; distinct shaded vertex colors; no texture/light/Z/cull; baseline versus partial geometry and graphics-line-graphics reload\",\n"
              "  \"repeat_checks\":\"two cold runs per case and warm graphics repeat after switched task; compare packet boundaries, semantic operands and DMA/status counters; ignored NOOP/sync operands excluded\",\n"
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
        throw std::runtime_error("RDP replay requires both probe build options");
#endif
    if constexpr (std::endian::native != std::endian::little)
        throw std::runtime_error("the word-swapped RSP backend requires a little-endian host");
    // std::filesystem::path consumes Linux argv bytes or Windows UTF-16 wmain
    // directly. No locale narrowing, path logging, mutation, or second ROM open.
    const auto snapshot = read_snapshot(rom_path);
    std::cout << "private_rom_admitted normalized_sha1=" << isle::rom::digest_hex(snapshot.identity)
              << " size=" << snapshot.identity.size << " scope=coprocessor-only\n";
    probe::Fixture fixture(snapshot.canonical(), probe::Case::matrix);
    probe::Capture matrix, reload, repeat;
    SinkContext sink;
    isle::rsp::Machine machine(fixture.rdram, SinkContext::submit, &sink);
    const auto boot = snapshot.canonical().subspan(probe::boot_rom_begin, probe::boot_bytes);
    const isle::rsp::Limits limits{1000000, probe::capture_words};
    auto execute = [&](probe::Case kind, const char* label, probe::Capture& capture, bool cold) {
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
    fixture.load(snapshot.canonical(), probe::Case::matrix);
    results[1] = execute(probe::Case::matrix, "matrix-cold-2", repeat, true);
    probe::require_repeat(matrix, results[0], repeat, results[1]);
    fixture.load(snapshot.canonical(), probe::Case::reload);
    results[2] = execute(probe::Case::reload, "reload-cold-1", reload, true);
    fixture.load(snapshot.canonical(), probe::Case::reload);
    results[3] = execute(probe::Case::reload, "reload-cold-2", repeat, true);
    probe::require_repeat(reload, results[2], repeat, results[3]);
    fixture.load(snapshot.canonical(), probe::Case::matrix);
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
            throw std::runtime_error("cannot create independent RDP case directories");
        isle::rsp::probe::render_capture(matrix, matrix_directory, "fractional-matrix", cpu_device);
        isle::rsp::probe::render_capture(reload, reload_directory, "in-list-reload", cpu_device);
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
