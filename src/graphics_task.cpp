#include "graphics_task.hpp"

#include <algorithm>
#include <array>
#include <functional>
#include <stdexcept>

// Exact GAME splat/gtl.c image, boot and task provenance is shared with the
// Pidgey producer. Only RSP code executes; the copied CPU image stays inert.
namespace isle::graphics {
namespace {
constexpr std::size_t ram_size = 8 * 1024 * 1024;
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
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
void validate_cold_inputs(std::span<std::uint32_t> words, const rom::Snapshot& snapshot,
                          const std::byte* source, std::uint32_t boot_address) {
    require(words.size_bytes() == ram_size, "Graphics installation requires exactly 8 MiB");
    const auto canonical = snapshot.canonical();
    require(canonical.size() == rom::us_size && canonical.data() == source,
            "Graphics installation borrows a moved Snapshot");
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
                         std::uint32_t commands_address) {
    Memory memory(words);
    // GAME gtl.c: commands_size is zero; genuine ENDDL controls traversal.
    const rsp::Task task{1, 4, boot_address, boot_bytes, graphics_text, 0x1000,
                         graphics_data, 0x800, stack_address, 0x400,
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
