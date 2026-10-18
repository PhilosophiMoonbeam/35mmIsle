#ifndef ISLE_GRAPHICS_TASK_HPP
#define ISLE_GRAPHICS_TASK_HPP

#include "35mmisle/engine.h"
#include "35mmisle/rsp.hpp"
#include "rom_snapshot.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace isle::graphics {
// Public numeric RCP wire words; never an SDK structure or a native pointer.
class Memory final {
public:
    explicit Memory(std::span<std::uint32_t> words);
    void word(std::uint32_t address, std::uint32_t value);
    void half(std::uint32_t address, std::uint16_t value);
    void byte(std::uint32_t address, std::uint8_t value);
    void copy(std::uint32_t address, std::span<const std::byte> bytes);
    void matrix(std::uint32_t address, const IsleGameFixedMatrix& matrix);
private:
    std::span<std::uint32_t> words_;
};
inline constexpr std::uint32_t main_begin = 0x400, main_rom_begin = 0x1000;
inline constexpr std::uint32_t main_rom_end = 0x46270, bss_begin = 0x45670, bss_end = 0x9a8c0;
inline constexpr std::uint32_t graphics_text = 0x3e7a0, graphics_data = 0x44e60;
inline constexpr std::uint32_t boot_rom = 0xb70, boot_bytes = 0x100;
struct PreparedTask { rsp::Task task; std::span<const std::byte> boot; };

// Original gtl.c IDs, not aliases for interchangeable native programs. Only
// f3dex2_non_fifo and l3dex2_fifo have non-NULL entries in the pinned task table.
enum class SourceProgram : std::uint32_t {
    f3dex2_fifo = 0, f3dex2_xbus = 1,
    f3dex2_non_fifo = 2, f3dex2_non_xbus = 3,
    f3dex2_rej_fifo = 4, f3dex2_rej_xbus = 5,
    f3dlx2_rej_fifo = 6, f3dlx2_rej_xbus = 7,
    l3dex2_fifo = 8, l3dex2_xbus = 9,
    s2dex2_fifo = 10, s2dex2_xbus = 11
};
struct SourceProgramSelection {
    // Preserve gtlInit's ID0 defaults; querying an unincluded task still fails.
    SourceProgram primary = SourceProgram::f3dex2_fifo;
    SourceProgram alternate = SourceProgram::f3dex2_fifo;
    bool alternate_active = false;
    bool disable_near_clipping = false;
};
SourceProgram source_line_program(SourceProgramSelection) noexcept;
SourceProgram source_task_program(SourceProgramSelection, bool line) noexcept;

struct ProgramRegion {
    std::uint32_t address;
    std::uint32_t rom_offset;
    // Full immutable program/overlay extent, not the initial task DMA length.
    std::span<const std::byte> source;
};
struct ProgramDescriptor {
    SourceProgram program;
    ProgramRegion text, data;
    std::uint32_t task_text_bytes, task_data_bytes, load_data_bytes;
    std::uint32_t minimum_fifo_bytes;
};
// Counts are 64-bit commands, unlike scene::render::List's 32-bit words.
struct CommandRange { std::uint32_t address; std::size_t commands; };
struct ProgramLoad {
    std::array<std::uint32_t, 6> words{};
    std::size_t word_count = 0;
};
// Nonowning immutable source authority. Snapshot must remain live and unmoved.
// Both programs are installed by the existing cold main-image/BSS transaction;
// keep that reservation and the exclusive RAM/epoch owner through GPU retirement.
class SourcePrograms final {
public:
    explicit SourcePrograms(const rom::Snapshot&);
    ProgramDescriptor descriptor(SourceProgram) const;
    // Exact gtlLoadUcode cases: 0..2 load2, 3..8 load8, 9 reset-only.
    // Task-table admission is deliberately separate; this is NOT an ID fallback.
    // The genuine bounded reset list must end with ENDDL and remains caller-owned.
    ProgramLoad load(SourceProgram, CommandRange reset) const;
private:
    void validate() const;
    const rom::Snapshot* snapshot_;
    const std::byte* source_;
};

struct ResetInputs {
    std::uint32_t framebuffer, depth, viewport, identity;
    std::uint32_t pre_render;
    std::uint32_t framebuffer_bytes, depth_bytes;
};
inline constexpr std::size_t reset_command_words = 60;
// Source rdpReset numeric wire body, INCLUDING its actual global pre-render
// call, but WITHOUT ENDDL. Append ENDDL in an owned slot for a callable reset.
// Caller reserves all referenced targets/payloads/lists before writing packets.
// Explicit target extents are their owned 16-bit allocations, not the global
// 320x240 scissor installed here; Source offscreen callbacks replace that scissor.
// depth/depth_bytes == 0/0 explicitly binds Source NULL without claiming an
// allocation at physical zero. Any non-NULL depth requires its positive extent.
std::array<std::uint32_t, reset_command_words> reset_commands(ResetInputs);

// Shared by the real GAME Pidgey and Beach producers. Call validation BEFORE
// any installation write; boot_address is a producer-owned aligned 0x100 span.
void validate_cold_inputs(std::span<std::uint32_t> words, const rom::Snapshot& snapshot,
                          const std::byte* original_source, std::uint32_t boot_address);
// Requires successful validation and all producer/resource preflight checks.
// Cold image/BSS only: never clears whole RDRAM or hidden memory.
void install_cold_inputs(std::span<std::uint32_t> words, const rom::Snapshot& snapshot,
                         std::uint32_t boot_address);
PreparedTask finish_task(std::span<std::uint32_t> words, const rom::Snapshot& snapshot,
                         std::uint32_t boot_address, std::uint32_t stack_address,
                         std::uint32_t yield_address, std::uint32_t task_address,
                         std::uint32_t output_address, std::uint32_t output_end,
                         std::uint32_t commands_address,
                         SourceProgram program = SourceProgram::f3dex2_non_fifo);
} // namespace isle::graphics
#endif
