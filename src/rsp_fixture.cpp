// Original canonical-word stimulus and structural checks, not SDK implementation.
// Interface facts: Snap df8815e75f3d79618466e01e0827c6165b93d11c,
// splat.yaml, src/entry/entry.s, src/sys/{gtl,render}.c and PR/{gbi,sptask,rcp}.h.
// The private ROM is task input only. No microcode bytes are compiled into this file.
#include "rsp_fixture.hpp"

#include "35mmisle/rom.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>

namespace isle::rsp_probe {
namespace {
constexpr std::uint32_t projection_address = 0x101000;
constexpr std::uint32_t model_address = 0x101040;
constexpr std::uint32_t viewport_address = 0x102000;
constexpr std::uint32_t vertices_address = 0x103000;
constexpr std::uint32_t stack_address = 0x104000;
constexpr std::uint32_t yield_address = 0x105000;
constexpr std::array<std::uint32_t, 3> markers{0xa13c5eff, 0xb24d6fff, 0xc35e70ff};
static_assert(main_address(main_rom_end) == main_bss_begin);
static_assert(main_bss_begin + 0x55250 == main_bss_end);
static_assert(main_address(line_data_rom) + 0x3f0 == main_bss_begin);
static_assert(data_bytes - 0x3f0 == 0x410);
static_assert(graphics_data_rom + data_bytes == main_rom_end - 0x10);
static_assert(main_bss_end < commands_address);
static_assert(output_end < color_address && color_address + width * height * 2 < rdram_bytes);

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void range(std::uint32_t address, std::size_t size) {
    require(address <= rdram_bytes && size <= rdram_bytes - address,
            "canonical fixture address is outside owned RDRAM");
}

class Memory {
public:
    explicit Memory(std::span<std::uint32_t> words) : words_(words) {}
    void word(std::uint32_t address, std::uint32_t value) {
        range(address, 4);
        require((address & 3) == 0, "unaligned canonical fixture word");
        words_[address / 4] = value;
    }
    void half(std::uint32_t address, std::uint16_t value) {
        range(address, 2);
        require((address & 1) == 0, "unaligned canonical fixture halfword");
        const unsigned shift = (address & 2) ? 0 : 16;
        auto& destination = words_[address / 4];
        destination = (destination & ~(0xffffu << shift)) | (std::uint32_t(value) << shift);
    }
    void copy(std::uint32_t address, std::span<const std::byte> bytes) {
        range(address, bytes.size());
        require((address & 3) == 0 && (bytes.size() & 3) == 0,
                "unaligned canonical fixture copy");
        for (std::size_t offset = 0; offset < bytes.size(); offset += 4) {
            const auto source = bytes.subspan(offset, 4);
            word(address + static_cast<std::uint32_t>(offset),
                 (std::to_integer<std::uint32_t>(source[0]) << 24) |
                 (std::to_integer<std::uint32_t>(source[1]) << 16) |
                 (std::to_integer<std::uint32_t>(source[2]) << 8) |
                  std::to_integer<std::uint32_t>(source[3]));
        }
    }
    void matrix(std::uint32_t address, const std::array<std::int32_t, 16>& fixed) {
        range(address, 64);
        // Signed 16.16 values: all sixteen integer halves, then fractional halves.
        for (unsigned index = 0; index < fixed.size(); ++index) {
            const auto bits = static_cast<std::uint32_t>(fixed[index]);
            half(address + index * 2, static_cast<std::uint16_t>(bits >> 16));
            half(address + 32 + index * 2, static_cast<std::uint16_t>(bits));
        }
    }
private:
    std::span<std::uint32_t> words_;
};

class List {
public:
    explicit List(Memory& memory) : memory_(memory) {}
    void command(std::uint32_t first, std::uint32_t second) {
        require(next_ + 8 <= commands_address + 0x1000, "synthetic display list exceeds its owned region");
        memory_.word(next_, first);
        memory_.word(next_ + 4, second);
        next_ += 8;
    }
    void load_ucode(std::uint32_t text_rom, std::uint32_t data_rom) {
        command(0xe1000000, main_address(data_rom));
        command(0xdd000000 | (data_bytes - 1), main_address(text_rom));
    }
    void clear_target() {
        command(0xe7000000, 0);                         // PipeSync before fill state.
        command(0xff100000 | (width - 1), color_address);
        command(0xed000000, ((width * 4) << 12) | (height * 4));
        command(0xef300000, 0);                        // Fill cycle, no depth/blending.
        command(0xf7000000, 0x00010001);                // Opaque black RGBA5551 pair.
        command(0xf6000000 | (((width - 1) * 4) << 12) | ((height - 1) * 4), 0);
        // phase() issues PipeSync and restores one-cycle shade before drawing.
    }
    void phase(unsigned marker, bool partial, bool line) {
        command(0xe7000000, 0);                         // PipeSync.
        command(0xff100000 | (width - 1), color_address);
        command(0xed000000, ((width * 4) << 12) | (height * 4));
        command(0xef0000f0, 0);                        // One cycle; no Z/texture/blending.
        // Both combiner cycles select shade for RGB and alpha.
        command(0xfc000000 | (8u << 20) | (16u << 15) | (7u << 12) |
                    (7u << 9) | (8u << 5) | 16u,
                (8u << 28) | (8u << 24) | (7u << 21) | (7u << 18) |
                    (4u << 15) | (7u << 12) | (4u << 9) | (4u << 6) | (7u << 3) | 4u);
        command(0xfa000000, markers[marker]);           // Observable phase boundary.
        command(0xdb060000, 0);                         // Segment zero is physical RDRAM.
        command(0xd9000000, 0x00a00004);                 // Shade, smooth, clipping; no light/cull/Z.
        command(0xd7000000, 0);                         // Texture disabled.
        command(0xdc080008, viewport_address);          // Sixteen-byte viewport.
        command(0xda380007, projection_address);        // Projection load, no push.
        command(0xda380003, model_address);             // Modelview load, no push.
        command(0xd5000001, 0);                         // Exact game's SPECIAL_1 MVP recalc interface.
        if (partial) {
            // Replace ONLY combined XX/XY. Signed halves -1,-1 plus .75,.875
            // produce -.25,-.125; all other fractional matrix entries survive.
            command(0xdb000000, 0xffffffff);
            command(0xdb000020, 0xc000e000);
        }
        command(0x01003006, vertices_address);           // Load three vertices into slots 0..2.
        command(line ? 0x08000200 : 0x05000204, 0);       // Line 0->1 or triangle 0,1,2.
    }
private:
    Memory& memory_;
    std::uint32_t next_ = commands_address;
};

void serialize_task(Memory& memory, const rsp::Task& task) {
    const std::array<std::uint32_t, 16> fields{
        task.type, task.flags, task.boot_address, task.boot_size,
        task.text_address, task.text_size, task.data_address, task.data_size,
        task.stack_address, task.stack_size, task.output_address, task.output_end,
        task.commands_address, task.commands_size, task.yield_address, task.yield_size};
    for (unsigned index = 0; index < fields.size(); ++index)
        memory.word(task_record_address + index * 4, fields[index]);
}

int signed14(std::uint32_t bits) noexcept {
    const int value = static_cast<int>(bits & 0x3fff);
    return value >= 0x2000 ? value - 0x4000 : value;
}

void triangle(std::span<const std::uint32_t> words, bool line, bool partial) {
    require(words.size() == 24 && ((words[0] >> 24) & 0x3f) == 0x0c,
            "expected an untextured shaded triangle, with no Z coefficients");
    const int low = signed14(words[0]);
    const int middle = signed14(words[1] >> 16);
    const int high = signed14(words[1]);
    require(high <= middle && middle <= low && high < low,
            "RSP emitted unordered or degenerate triangle Y edges");
    require(high >= 0 && low <= static_cast<int>(height * 4),
            "RSP emitted geometry outside the synthetic viewport");
    if (!line) {
        // Row-vector clip transform, then framebuffer Y points DOWN:
        // screenY = height/2 * (1 - clipY/W). Do not use GL's upward axis.
        // This narrow analytic stimulus is not an independent frontend oracle.
        const std::array<int, 3> expected = partial ? std::array{48, 108, 132} : std::array{54, 114, 126};
        const std::array<int, 3> actual{high, middle, low};
        for (unsigned index = 0; index < actual.size(); ++index)
            require(actual[index] >= expected[index] - 2 && actual[index] <= expected[index] + 2,
                    "triangle Y geometry does not reflect fractional MVP/negative partial writes");
        // XH is the major edge's 16.16 origin at the highest vertex. Allow
        // two pixels for the microcode's subpixel edge-start adjustment.
        const std::int64_t x_high = static_cast<std::int32_t>(words[4]);
        const std::int64_t expected_x = std::int64_t(partial ? 38 : 18) * 65536;
        require(x_high >= expected_x - 2 * 65536 && x_high <= expected_x + 2 * 65536,
                "triangle X geometry does not reflect signed partial matrix writes");
    }
}
} // namespace

const char* name(Case kind) noexcept {
    return kind == Case::matrix ? "fractional-matrix" : "in-list-graphics-line-graphics";
}

void initialize_fixture_memory(std::span<std::uint32_t> rdram,
                               std::span<const std::byte> canonical_rom) {
    require(rdram.size() == rdram_bytes / 4, "fixture requires exactly 8 MiB of caller-owned RDRAM");
    require(canonical_rom.size() == rom::us_size, "fixture requires the admitted complete ROM snapshot");
    range(main_ram_begin, main_rom_end - main_rom_begin);
    range(main_bss_begin, main_bss_end - main_bss_begin);
    range(boot_address, boot_bytes);
    range(main_address(graphics_text_rom), text_bytes);
    range(main_address(graphics_data_rom), data_bytes);
    range(main_address(line_text_rom), text_bytes);
    range(main_address(line_data_rom), data_bytes);
    std::fill(rdram.begin(), rdram.end(), guard_word);
    Memory memory(rdram);
    // Reconstruct main's REAL loaded RAM image, then its entry-cleared BSS.
    // Graphics' 0x800-byte data DMA includes adjacent line data. Line's includes
    // 0x3f0 ROM-loaded bytes and 0x410 entry-cleared BSS bytes, NOT app_render ROM.
    memory.copy(main_ram_begin, canonical_rom.subspan(main_rom_begin, main_rom_end - main_rom_begin));
    std::fill(rdram.begin() + main_bss_begin / 4, rdram.begin() + main_bss_end / 4, 0);
    // gRspBootCode is loaded separately by main.c:114 from IPL ROM offset 0xb70;
    // it is not the different linked rspbootTextStart resource in main text.
    memory.copy(boot_address, canonical_rom.subspan(boot_rom_begin, boot_bytes));
}

Fixture::Fixture(std::span<std::uint32_t> memory_words, Case kind) : rdram(memory_words) {
    require(rdram.size() == rdram_bytes / 4, "fixture requires exactly 8 MiB of caller-owned RDRAM");
    // Validate each declared per-task workspace before any write. No task
    // preparation touches the loaded main/BSS, boot, color or hidden regions.
    range(commands_address, 0x1000);
    range(projection_address, 128);
    range(viewport_address, 16);
    range(vertices_address, 48);
    range(stack_address, 0x400);
    range(yield_address, 0xc00);
    range(task_record_address, 64);
    range(output_address - 64, output_end - output_address + 128);
    const auto guard = [&](std::uint32_t address, std::size_t size) {
        std::fill(rdram.begin() + address / 4, rdram.begin() + (address + size) / 4, guard_word);
    };
    guard(commands_address, 0x1000);
    guard(stack_address, 0x400);
    guard(yield_address, 0xc00);
    guard(output_address - 64, output_end - output_address + 128);
    Memory memory(rdram);
    memory.matrix(projection_address, {
        0x4000, 0, 0, 0, 0, 0x4000, 0, 0,
        0, 0, 0x4000, 0, 0, 0, 0, 0x10000});
    memory.matrix(model_address, {
        0x18000, -0x4000, 0, 0, 0x4000, 0x14000, 0, 0,
        0, 0, 0x10000, 0, -0x8000, 0x4000, 0, 0x10000});
    const std::array<std::uint16_t, 8> viewport{128, 96, 511, 0, 128, 96, 511, 0};
    for (unsigned index = 0; index < viewport.size(); ++index)
        memory.half(viewport_address + index * 2, viewport[index]);
    const std::array<std::array<std::int16_t, 3>, 3> positions{{{-1, -1, 0}, {1, -1, 0}, {-1, 1, 0}}};
    const std::array<std::uint32_t, 3> colors{0xff2000ff, 0x20ff00ff, 0x2020ffff};
    for (unsigned index = 0; index < positions.size(); ++index) {
        const auto address = vertices_address + index * 16;
        for (unsigned component = 0; component < 3; ++component)
            memory.half(address + component * 2, static_cast<std::uint16_t>(positions[index][component]));
        memory.half(address + 6, 0);
        memory.word(address + 8, 0);
        memory.word(address + 12, colors[index]);
    }
    List list(memory);
    list.clear_target();
    if (kind == Case::matrix) {
        list.phase(0, false, false);
        list.phase(1, true, false);
    } else {
        list.phase(0, true, false);
        list.load_ucode(line_text_rom, line_data_rom);
        list.phase(1, true, true);
        list.load_ucode(graphics_text_rom, graphics_data_rom);
        list.phase(2, true, false);
    }
    list.command(0xe9000000, 0);                         // Exactly one final FullSync.
    list.command(0xdf000000, 0);                         // EndDL, never a fabricated host completion.
    task = {1, 4, boot_address, boot_bytes,
            main_address(graphics_text_rom), text_bytes, main_address(graphics_data_rom), data_bytes,
            stack_address, 0x400, output_address, output_end, commands_address, 0, yield_address, 0xc00};
    serialize_task(memory, task);
}

Geometry validate(const Fixture& fixture, Case kind, const rsp::PacketBuffer& capture, const rsp::Result& result) {
    require(bool(result), "RSP execution failed; captured output is invalid and will not be published");
    require(!capture.rejected(), "bounded packet sink rejected the task");
    // HALT+BROKE+TASKDONE, with no pending DMA/IO, single-step, or yield signals.
    require((result.sp_status & 0x203) == 0x203 && (result.sp_status & 0x1bc) == 0,
            "RSP stopped without clean graphics task completion status");
    require(result.dma_reads != 0 && result.dma_writes != 0 && result.imem_writes != 0,
            "task did not perform actual DMA/IMEM/FIFO work");
    require(result.full_syncs == 1 && result.packets == capture.packets().size() &&
            result.packet_words == capture.words().size() && !capture.packets().empty(),
            "RSP packet/FullSync counters do not match captured output");
    for (std::uint32_t address = output_address - 64; address < output_address; address += 4)
        require(fixture.rdram[address / 4] == guard_word, "FIFO underflow overwrote its canonical guard");
    for (std::uint32_t address = output_end; address < output_end + 64; address += 4)
        require(fixture.rdram[address / 4] == guard_word, "FIFO overflow overwrote its canonical guard");
    Geometry geometry;
    std::array<std::span<const std::uint32_t>, 3> graphics_triangles{};
    std::array<unsigned, 3> phase_triangles{};
    unsigned phase = 3;
    bool saw_full_sync = false;
    bool line_scissor = false;
    bool saw_line_scissor = false;
    bool color_defined = false, scissor_defined = false;
    bool fill_mode = false, fill_color = false, cleared = false, shade_mode = false, combine = false;
    std::size_t next_packet = 0;
    for (const auto& packet : capture.packets()) {
        require(packet.offset == next_packet && packet.offset <= capture.words().size() &&
                packet.count >= 2 && packet.count <= capture.words().size() - packet.offset,
                "packet buffer contains an invalid or noncontiguous packet range");
        next_packet += packet.count;
        const auto words = capture.words().subspan(packet.offset, packet.count);
        const auto opcode = (words[0] >> 24) & 0x3f;
        require(!saw_full_sync, "unexpected RDP output after final FullSync");
        if (opcode >= 8 && opcode <= 15) {
            require(phase < geometry.markers && cleared && shade_mode && combine,
                    "geometry appeared before the authored clear and explicit shade state/marker");
            const bool line = kind == Case::reload && phase == 1;
            require(line == line_scissor,
                    "line narrowing must be active only for the line primitive and restored before graphics");
            triangle(words, line, kind == Case::reload || phase == 1);
            ++geometry.triangles;
            ++phase_triangles[phase];
            if (line) ++geometry.line_triangles;
            else graphics_triangles[phase] = words;
            continue;
        }
        require(words.size() == 2, "unexpected non-triangle RDP packet length");
        switch (opcode) {
        case 0x00: case 0x26: case 0x27: case 0x28: break;
        case 0x29:
            saw_full_sync = true;
            break;
        case 0x2d: {
            const auto ulx = (words[0] >> 12) & 0xfffu;
            const auto uly = words[0] & 0xfffu;
            const auto lrx = (words[1] >> 12) & 0xfffu;
            const auto lry = words[1] & 0xfffu;
            require(uly == 0 && lry == height * 4 && (words[1] & 0xff000000u) == 0,
                    "unexpected vertical scissor or interlace state");
            if (ulx == 0 && lrx == width * 4) {
                line_scissor = false;
                scissor_defined = true;
            } else {
                // Line microcode temporarily bounds X to the two transformed
                // endpoints: clip X=-.4375,.0625 -> framebuffer X=18,34.
                // The same two-quarter-pixel transform quantization bound used
                // for vertices applies; this is not an arbitrary new scissor.
                require(kind == Case::reload && phase == 1 &&
                        ulx >= 18 * 4 - 2 && ulx <= 18 * 4 + 2 &&
                        lrx >= 34 * 4 - 2 && lrx <= 34 * 4 + 2,
                        "line scissor does not enclose its analytic transformed endpoints");
                line_scissor = saw_line_scissor = true;
            }
            break;
        }
        case 0x2f:
            require(words[1] == 0, "unexpected depth, blend or texture state");
            if ((words[0] & 0xffffff) == 0x300000) {
                require(!fill_mode && !cleared && geometry.markers == 0,
                        "fill mode is allowed only for the single leading authored clear");
                fill_mode = true;
            } else {
                require((words[0] & 0xffffff) == 0xf0 && cleared,
                        "expected one-cycle shade after the authored target clear");
                fill_mode = false;
                shade_mode = true;
            }
            break;
        case 0x36:
            require(fill_mode && fill_color && color_defined && scissor_defined && !cleared &&
                    geometry.markers == 0 && (words[0] & 0xffffff) ==
                        ((((width - 1) * 4) << 12) | ((height - 1) * 4)) && words[1] == 0,
                    "expected exactly one full-target inclusive fill rectangle before all draw phases");
            cleared = true;
            break;
        case 0x37:
            require(fill_mode && !fill_color && !cleared && geometry.markers == 0 &&
                    (words[0] & 0xffffff) == 0 && words[1] == 0x00010001,
                    "expected the authored opaque-black RGBA5551 fill color");
            fill_color = true;
            break;
        case 0x3a:
            require(cleared && shade_mode && geometry.markers < (kind == Case::matrix ? 2u : 3u) &&
                    words[1] == markers[geometry.markers], "unexpected primitive phase marker/order");
            phase = static_cast<unsigned>(geometry.markers++);
            break;
        case 0x3c:
            require((words[0] & 0xffffff) == 0x887f10 && words[1] == 0x88fe793c,
                    "expected the authored shade-only combiner");
            combine = true;
            break;
        case 0x3f:
            require((words[0] & 0xffffff) == (0x100000 | (width - 1)) && words[1] == color_address,
                    "unexpected synthetic color image format/address");
            color_defined = true;
            break;
        default: throw std::runtime_error("unexpected RDP opcode in original geometry stimulus");
        }
    }
    require(next_packet == capture.words().size() && cleared && !fill_mode && saw_full_sync &&
            geometry.markers == (kind == Case::matrix ? 2u : 3u),
            "missing authored clear, final FullSync or primitive phase");
    require(!line_scissor && (kind != Case::reload || saw_line_scissor),
            "line microcode must narrow and restore its temporary scissor");
    require(phase_triangles[0] == 1 && phase_triangles[1] >= 1,
            "expected baseline/partial graphics or line geometry is missing");
    if (kind == Case::matrix) {
        require(phase_triangles[1] == 1 &&
                !std::equal(graphics_triangles[0].begin(), graphics_triangles[0].end(), graphics_triangles[1].begin()),
                "negative partial matrix writes did not change emitted geometry");
    } else {
        require(phase_triangles[2] == 1 && phase_triangles[1] <= 4 && result.imem_writes >= 3,
                "in-list graphics/line/graphics did not reload IMEM and emit each geometry phase");
        require(std::equal(graphics_triangles[0].begin(), graphics_triangles[0].end(), graphics_triangles[2].begin()),
                "restored graphics microcode did not reproduce the original triangle");
    }
    return geometry;
}

void require_repeat(const rsp::PacketBuffer& first, const rsp::Result& a,
                    const rsp::PacketBuffer& second, const rsp::Result& b) {
    require(first.words().size() == second.words().size() &&
            first.packets().size() == second.packets().size() &&
            std::equal(first.packets().begin(), first.packets().end(), second.packets().begin()),
            "cold/reset/reload repeat changed packet boundaries");
    for (const auto& packet : first.packets()) {
        const auto x = first.words().subspan(packet.offset, packet.count);
        const auto y = second.words().subspan(packet.offset, packet.count);
        const auto opcode = (x[0] >> 24) & 63u;
        require(opcode == ((y[0] >> 24) & 63u), "repeat changed an RDP opcode");
        // RDP no-op/sync commands have no operands. Their ignored words can
        // retain microcode scratch registers; do not pin incidental contents.
        if (opcode == 0 || (opcode >= 0x26 && opcode <= 0x29)) continue;
        require(std::equal(x.begin(), x.end(), y.begin()),
                "cold/reset/reload repeat changed a semantic RDP operand");
    }
    require(a.error == b.error && a.sp_status == b.sp_status && a.dma_reads == b.dma_reads &&
            a.dma_writes == b.dma_writes && a.imem_writes == b.imem_writes && a.packets == b.packets &&
            a.packet_words == b.packet_words && a.full_syncs == b.full_syncs,
            "cold/reset/reload repeat changed DMA, IMEM, packet or completion counters");
}
} // namespace isle::rsp_probe
