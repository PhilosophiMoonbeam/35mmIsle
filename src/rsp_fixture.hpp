#ifndef ISLE_RSP_FIXTURE_HPP
#define ISLE_RSP_FIXTURE_HPP

#include "35mmisle/rsp.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace isle::rsp_probe {
inline constexpr std::size_t rdram_bytes = 8u << 20;
inline constexpr std::size_t capture_words = 16384;
inline constexpr std::uint32_t main_rom_begin = 0x1000;
inline constexpr std::uint32_t main_rom_end = 0x46270;
inline constexpr std::uint32_t main_ram_begin = 0x400;
inline constexpr std::uint32_t main_bss_begin = 0x45670;
inline constexpr std::uint32_t main_bss_end = 0x9a8c0;
inline constexpr std::uint32_t boot_rom_begin = 0xb70;
inline constexpr std::uint32_t boot_bytes = 0x100;
inline constexpr std::uint32_t graphics_text_rom = 0x3f3a0;
inline constexpr std::uint32_t graphics_data_rom = 0x45a60;
inline constexpr std::uint32_t line_text_rom = 0x40730;
inline constexpr std::uint32_t line_data_rom = 0x45e80;
inline constexpr std::uint32_t text_bytes = 0x1000;
inline constexpr std::uint32_t data_bytes = 0x800;
inline constexpr std::uint32_t commands_address = 0x100000;
inline constexpr std::uint32_t task_record_address = 0x106000;
inline constexpr std::uint32_t boot_address = 0x107000;
inline constexpr std::uint32_t output_address = 0x200000;
inline constexpr std::uint32_t output_end = 0x210000;
inline constexpr std::uint32_t color_address = 0x300000;
inline constexpr unsigned width = 64, height = 48;
inline constexpr std::uint32_t guard_word = 0xa5a5a5a5;

constexpr std::uint32_t main_address(std::uint32_t rom_address) noexcept {
    return rom_address - main_rom_begin + main_ram_begin;
}

enum class Case { matrix, reload };
const char* name(Case kind) noexcept;

struct Packet {
    std::size_t offset = 0, count = 0;
    bool operator==(const Packet&) const = default;
};

// Storage is allocated before execute; the noexcept JIT callback cannot allocate.
class Capture {
public:
    Capture();
    void clear() noexcept;
    static bool sink(void* context, std::span<const std::uint32_t> packet) noexcept;
    std::span<const std::uint32_t> words() const noexcept;
    std::span<const Packet> packets() const noexcept;
    bool rejected() const noexcept { return rejected_; }
private:
    std::vector<std::uint32_t> words_;
    std::vector<Packet> packets_;
    std::size_t word_count_ = 0, packet_count_ = 0;
    bool rejected_ = false;
};

struct Fixture {
    explicit Fixture(std::span<const std::byte> canonical_rom, Case kind);
    void load(std::span<const std::byte> canonical_rom, Case kind);
    std::vector<std::uint32_t> rdram;
    rsp::Task task{};
};

struct Geometry {
    std::size_t triangles = 0, line_triangles = 0, markers = 0;
};
Geometry validate(const Fixture& fixture, Case kind, const Capture& capture,
                  const rsp::Result& result);
void require_repeat(const Capture& first, const rsp::Result& first_result,
                    const Capture& second, const rsp::Result& second_result);
} // namespace isle::rsp_probe
#endif
