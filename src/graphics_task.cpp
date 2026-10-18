#include "graphics_task.hpp"

#include <algorithm>
#include <array>
#include <functional>
#include <stdexcept>

// Numeric GAME/public-wire facts only. Private program bytes remain borrowed
// from the admitted Snapshot; the copied CPU image stays inert.
namespace isle::graphics {
namespace {
constexpr std::size_t ram_size = 8 * 1024 * 1024;
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
constexpr std::array<std::uint8_t, 20> snapshot_sha1{
    0xed, 0xc7, 0xc4, 0x9c, 0xc5, 0x68, 0xc0, 0x45, 0xfe, 0x48,
    0xbe, 0x0d, 0x18, 0x01, 0x1c, 0x30, 0xf3, 0x93, 0xcb, 0xaf};
void validate_snapshot(const rom::Snapshot& snapshot, const std::byte* source) {
    const auto canonical = snapshot.canonical();
    require(canonical.size() == rom::us_size && canonical.data() == source &&
            snapshot.identity().size == rom::us_size && snapshot.identity().sha1 == snapshot_sha1,
            "Graphics source requires its live admitted Snapshot");
}
void validate_range(std::uint32_t address, std::size_t bytes, std::uint32_t alignment,
                    const char* message) {
    require((address & (alignment - 1)) == 0 && address <= ram_size &&
            bytes <= ram_size - address, message);
}
void validate_commands(CommandRange range) {
    require(range.commands != 0 && range.commands <= ram_size / 8,
            "Graphics source command range is empty or oversized");
    validate_range(range.address, range.commands * 8, 8, "Invalid graphics source command range");
}
constexpr std::uint32_t triangle_text_rom = 0x3f3a0, triangle_text_bytes = 0x1390;
constexpr std::uint32_t triangle_data_rom = 0x45a60, triangle_data_bytes = 0x420;
constexpr std::uint32_t line_text = 0x3fb30, line_data = 0x45280;
constexpr std::uint32_t line_text_rom = 0x40730, line_text_bytes = 0x1190;
constexpr std::uint32_t line_data_rom = 0x45e80, line_data_bytes = 0x3f0;
constexpr std::uint32_t task_text_bytes = 0x1000, task_data_bytes = 0x800;
static_assert(graphics_text + triangle_text_bytes == line_text);
static_assert(graphics_data + triangle_data_bytes == line_data);
static_assert(line_text_rom + line_text_bytes <= main_rom_end);
static_assert(line_data_rom + line_data_bytes == main_rom_end);
static_assert(line_data + line_data_bytes == bss_begin);
// The source's 0x800-byte line data DMA also reads the cold-zeroed BSS tail.
static_assert(line_data + task_data_bytes <= bss_end);
// Caller has already validated the immutable Snapshot. No per-task table scan.
ProgramDescriptor describe_program(std::span<const std::byte> canonical, SourceProgram program) {
    switch (program) {
    case SourceProgram::f3dex2_non_fifo:
        return {program,
                {graphics_text, triangle_text_rom,
                 canonical.subspan(triangle_text_rom, triangle_text_bytes)},
                {graphics_data, triangle_data_rom,
                 canonical.subspan(triangle_data_rom, triangle_data_bytes)},
                task_text_bytes, task_data_bytes, task_data_bytes, 0x410};
    case SourceProgram::l3dex2_fifo:
        return {program,
                {line_text, line_text_rom, canonical.subspan(line_text_rom, line_text_bytes)},
                {line_data, line_data_rom, canonical.subspan(line_data_rom, line_data_bytes)},
                task_text_bytes, task_data_bytes, task_data_bytes, 0x540};
    default:
        throw std::runtime_error("Graphics task program is not included by pinned GAME");
    }
}
}
Memory::Memory(std::span<std::uint32_t> words) : words_(words) {
    require(words.size_bytes() == ram_size, "Graphics producer requires exactly 8 MiB");
}
void Memory::word(std::uint32_t address, std::uint32_t value) { words_[address / 4] = value; }
void Memory::byte(std::uint32_t address, std::uint8_t value) {
    auto& w = words_[address / 4];
    const auto shift = (3 - (address & 3u)) * 8;
    w = (w & ~(255u << shift)) | (std::uint32_t(value) << shift);
}
void Memory::half(std::uint32_t address, std::uint16_t value) {
    auto& w = words_[address / 4];
    const auto shift = (address & 2u) ? 0u : 16u;
    w = (w & ~(0xffffu << shift)) | (std::uint32_t(value) << shift);
}
void Memory::copy(std::uint32_t address, std::span<const std::byte> bytes) {
    std::size_t i = 0;
    for (; i + 4 <= bytes.size(); i += 4)
        word(address + static_cast<std::uint32_t>(i),
             std::to_integer<std::uint32_t>(bytes[i]) << 24 |
             std::to_integer<std::uint32_t>(bytes[i + 1]) << 16 |
             std::to_integer<std::uint32_t>(bytes[i + 2]) << 8 |
             std::to_integer<std::uint32_t>(bytes[i + 3]));
    for (; i < bytes.size(); ++i)
        byte(address + static_cast<std::uint32_t>(i), std::to_integer<std::uint8_t>(bytes[i]));
}
void Memory::matrix(std::uint32_t address, const IsleGameFixedMatrix& m) {
    for (unsigned row = 0; row < 4; ++row)
        for (unsigned col = 0; col < 4; ++col) word(address + (row * 4 + col) * 4, m.m[row][col]);
}

SourceProgram source_line_program(SourceProgramSelection selection) noexcept {
    const auto program = selection.alternate_active ? selection.alternate : selection.primary;
    switch (program) {
    case SourceProgram::f3dex2_xbus:
    case SourceProgram::f3dex2_non_xbus:
    case SourceProgram::f3dex2_rej_xbus:
    case SourceProgram::f3dlx2_rej_xbus:
    case SourceProgram::l3dex2_xbus:
        return SourceProgram::l3dex2_xbus;
    default:
        return SourceProgram::l3dex2_fifo;
    }
}
SourceProgram source_task_program(SourceProgramSelection selection, bool line) noexcept {
    if (line) return source_line_program(selection);
    auto program = selection.primary;
    if (selection.disable_near_clipping) {
        switch (program) {
        case SourceProgram::f3dex2_fifo: program = SourceProgram::f3dex2_non_fifo; break;
        case SourceProgram::f3dex2_xbus: program = SourceProgram::f3dex2_non_xbus; break;
        default: break;
        }
    }
    return program;
}
SourcePrograms::SourcePrograms(const rom::Snapshot& snapshot)
    : snapshot_(&snapshot), source_(snapshot.canonical().data()) {
    validate();
    // Exact pinned gtlD_80040CFC table, not dirty generated standalone data.
    const auto bytes = snapshot.canonical();
    const auto word = [&](std::size_t offset) {
        return std::to_integer<std::uint32_t>(bytes[offset]) << 24 |
               std::to_integer<std::uint32_t>(bytes[offset + 1]) << 16 |
               std::to_integer<std::uint32_t>(bytes[offset + 2]) << 8 |
               std::to_integer<std::uint32_t>(bytes[offset + 3]);
    };
    for (std::uint32_t id = 0; id < 11; ++id) {
        const auto text = id == 2 ? graphics_text : id == 8 ? line_text : 0;
        const auto data = id == 2 ? graphics_data : id == 8 ? line_data : 0;
        require(word(0x418fc + id * 8) == (text ? 0x80000000u + text : 0) &&
                word(0x41900 + id * 8) == (data ? 0x80000000u + data : 0),
                "Graphics program table differs from pinned GAME");
    }
}
void SourcePrograms::validate() const { validate_snapshot(*snapshot_, source_); }
ProgramDescriptor SourcePrograms::descriptor(SourceProgram program) const {
    validate();
    return describe_program(snapshot_->canonical(), program);
}
ProgramLoad SourcePrograms::load(SourceProgram program, CommandRange reset) const {
    validate();
    validate_commands(reset);
    // gtlLoadUcode's switch is intentionally NOT the task-table lookup.
    SourceProgram loaded;
    switch (program) {
    case SourceProgram::f3dex2_fifo:
    case SourceProgram::f3dex2_xbus:
    case SourceProgram::f3dex2_non_fifo:
        loaded = SourceProgram::f3dex2_non_fifo;
        break;
    case SourceProgram::f3dex2_non_xbus:
    case SourceProgram::f3dex2_rej_fifo:
    case SourceProgram::f3dex2_rej_xbus:
    case SourceProgram::f3dlx2_rej_fifo:
    case SourceProgram::f3dlx2_rej_xbus:
    case SourceProgram::l3dex2_fifo:
        loaded = SourceProgram::l3dex2_fifo;
        break;
    case SourceProgram::l3dex2_xbus:
        return {{{0xde000000, reset.address, 0, 0, 0, 0}}, 2};
    default:
        throw std::runtime_error("Graphics source reload ID is outside admitted switch cases");
    }
    const auto selected = descriptor(loaded);
    return {{{0xe1000000, selected.data.address,
              0xdd000000u | (selected.load_data_bytes - 1), selected.text.address,
              0xde000000, reset.address}}, 6};
}
std::array<std::uint32_t, reset_command_words> reset_commands(ResetInputs inputs) {
    require(inputs.framebuffer_bytes && !(inputs.framebuffer_bytes & 1u),
            "Invalid source reset owned 16-bit framebuffer extent");
    require((!inputs.depth && !inputs.depth_bytes) ||
            (inputs.depth && inputs.depth_bytes && !(inputs.depth_bytes & 1u)),
            "Invalid source reset nullable 16-bit depth extent");
    validate_range(inputs.framebuffer, inputs.framebuffer_bytes, 8, "Invalid source reset framebuffer");
    if (inputs.depth_bytes)
        validate_range(inputs.depth, inputs.depth_bytes, 8, "Invalid source reset depth target");
    validate_range(inputs.viewport, 16, 8, "Invalid source reset viewport");
    validate_range(inputs.identity, 64, 8, "Invalid source reset identity matrix");
    validate_range(inputs.pre_render, 8, 8, "Invalid source reset global pre-render head");
    std::array<std::uint32_t, reset_command_words> words{};
    std::size_t cursor = 0;
    const auto command = [&](std::uint32_t a, std::uint32_t b) {
        words[cursor++] = a; words[cursor++] = b;
    };
    const auto mode = [&](bool high, unsigned shift, unsigned length, std::uint32_t value) {
        command((high ? 0xe3000000u : 0xe2000000u) |
                ((32 - shift - length) << 8) | (length - 1), value);
    };
    command(0xdb060000, 0); command(0xdb06003c, inputs.framebuffer);
    command(0xfe000000, inputs.depth);
    command(0xe7000000, 0); command(0xdc080008, inputs.viewport);
    constexpr std::uint32_t cleared = 1u | 4u | 0x600u | 0x10000u | 0x20000u |
                                      0x40000u | 0x80000u | 0x100000u | 0x200000u;
    command(0xd9000000u | ((~cleared) & 0xffffffu), 0);
    command(0xdb040004, 1); command(0xdb04000c, 1);
    command(0xdb040014, 0xffff); command(0xdb04001c, 0xffff);
    command(0xd7000000, 0); command(0xd9ffffff, 0x200405);
    command(0xda380007, inputs.identity); command(0xda380003, inputs.identity);
    mode(true, 20, 2, 0); mode(true, 23, 1, 0);
    command(0xfcffffff, 0xfffe793c);
    mode(true, 16, 1, 0); mode(true, 14, 2, 0); mode(true, 17, 2, 0);
    mode(true, 19, 1, 0x80000); mode(true, 12, 2, 0x2000);
    mode(true, 9, 3, 0xc00); mode(true, 8, 1, 0);
    mode(false, 0, 2, 0); mode(false, 3, 29, 0x0f0a4000);
    mode(true, 6, 2, 0); command(0xe7000000, 0);
    command(0xed000000, (320u * 4 << 12) | (240u * 4));
    command(0xde000000, inputs.pre_render);
    return words;
}
void validate_cold_inputs(std::span<std::uint32_t> words, const rom::Snapshot& snapshot,
                          const std::byte* source, std::uint32_t boot_address) {
    require(words.size_bytes() == ram_size, "Graphics installation requires exactly 8 MiB");
    validate_snapshot(snapshot, source);
    const auto canonical = snapshot.canonical();
    require((boot_address & 15u) == 0 && boot_address >= bss_end &&
            boot_address <= ram_size - boot_bytes, "Invalid graphics boot reservation");
    const auto ram = std::as_bytes(words);
    const std::less<const std::byte*> less;
    require(!less(ram.data(), canonical.data() + canonical.size()) ||
            !less(canonical.data(), ram.data() + ram.size()), "Graphics RAM aliases immutable ROM");
    static_assert(main_begin + main_rom_end - main_rom_begin == bss_begin);
    static_assert(main_rom_end <= rom::us_size && boot_rom + boot_bytes <= rom::us_size);
}
void install_cold_inputs(std::span<std::uint32_t> words, const rom::Snapshot& snapshot,
                         std::uint32_t boot_address) {
    validate_cold_inputs(words, snapshot, snapshot.canonical().data(), boot_address);
    Memory memory(words);
    const auto canonical = snapshot.canonical();
    memory.copy(main_begin, canonical.subspan(main_rom_begin, main_rom_end - main_rom_begin));
    std::fill(words.begin() + bss_begin / 4, words.begin() + bss_end / 4, 0);
    memory.copy(boot_address, canonical.subspan(boot_rom, boot_bytes));
}
PreparedTask finish_task(std::span<std::uint32_t> words, const rom::Snapshot& snapshot,
                         std::uint32_t boot_address, std::uint32_t stack_address,
                         std::uint32_t yield_address, std::uint32_t task_address,
                         std::uint32_t output_address, std::uint32_t output_end,
                         std::uint32_t commands_address, SourceProgram program) {
    validate_cold_inputs(words, snapshot, snapshot.canonical().data(), boot_address);
    const auto selected = describe_program(snapshot.canonical(), program);
    validate_range(stack_address, 0x400, 16, "Invalid graphics task stack");
    validate_range(yield_address, 0xc00, 16, "Invalid graphics task yield storage");
    validate_range(task_address, 64, 8, "Invalid graphics task descriptor storage");
    validate_range(commands_address, 8, 8, "Invalid graphics task command start");
    require(output_end >= output_address &&
            output_end - output_address >= selected.minimum_fifo_bytes,
            "Graphics FIFO output does not fit the selected source program");
    validate_range(output_address, output_end - output_address, 8, "Invalid graphics FIFO output");
    Memory memory(words);
    // GAME gtl.c: commands_size is zero; genuine ENDDL controls traversal.
    const rsp::Task task{1, 4, boot_address, boot_bytes, selected.text.address, selected.task_text_bytes,
                         selected.data.address, selected.task_data_bytes, stack_address, 0x400,
                         output_address, output_end, commands_address, 0, yield_address, 0xc00};
    const std::array<std::uint32_t, 16> fields{
        task.type, task.flags, task.boot_address, task.boot_size, task.text_address, task.text_size,
        task.data_address, task.data_size, task.stack_address, task.stack_size, task.output_address,
        task.output_end, task.commands_address, task.commands_size, task.yield_address, task.yield_size};
    for (std::size_t i = 0; i < fields.size(); ++i)
        memory.word(task_address + static_cast<std::uint32_t>(i * 4), fields[i]);
    return {task, snapshot.canonical().subspan(boot_rom, boot_bytes)};
}
} // namespace isle::graphics
