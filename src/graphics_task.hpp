#ifndef ISLE_GRAPHICS_TASK_HPP
#define ISLE_GRAPHICS_TASK_HPP

#include "35mmisle/engine.h"
#include "35mmisle/rsp.hpp"
#include "rom_snapshot.hpp"

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
                         std::uint32_t commands_address);
} // namespace isle::graphics
#endif
