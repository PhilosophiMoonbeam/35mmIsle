#ifndef ISLE_RDP_FIXTURE_HPP
#define ISLE_RDP_FIXTURE_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace isle::rdp_probe {
inline constexpr std::size_t rdram_size = 8u << 20;
inline constexpr std::size_t hidden_size = 4u << 20;
inline constexpr std::uint32_t width = 64;
inline constexpr std::uint32_t height = 48;
inline constexpr std::uint32_t plane_bytes = width * height * 2;
inline constexpr std::uint32_t color_base = 0x100000;
inline constexpr std::uint32_t depth_base = color_base + plane_bytes;
inline constexpr std::uint32_t before_base = depth_base + 13 * plane_bytes;
inline constexpr std::uint32_t region_end = before_base + 12 * plane_bytes;

enum class StageKind { clear, geometry, depth_copy };
struct Packet { std::size_t offset; unsigned word_count; };
struct Stage { StageKind kind; std::size_t first_packet; std::size_t packet_count; };
struct Fixture {
    std::vector<std::uint32_t> words;
    std::vector<Packet> packets;
    std::array<Stage, 3> stages;
};

// Original synthetic raw-RDP workload; no native Gfx, pointers, ROM or VI state.
Fixture make_fixture();
// Memory uses the backend's word-swapped N64 representation. Call after real
// GPU/timeline completion. Throws on a concrete semantic mismatch. These checks
// do not by themselves establish parity with an independent hardware oracle.
void validate_stage(StageKind kind, std::span<const std::byte> rdram,
                    std::span<const std::byte> hidden);
} // namespace isle::rdp_probe
#endif
