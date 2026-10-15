// Original synthetic packets and bounded analytic checks, not SDK display lists.
// Packet/format facts checked against Themaister/parallel-rdp at
// 1cecd042b2619bc505c12bfdc713808386f2b54d: rdp_device.cpp, rdp_common.hpp,
// shaders/{coverage,span_setup,interpolation,z_encode,memory_interfacing}.*.
// No upstream implementation is copied here. The analytic checks intentionally
// cover only these constant-Z, quarter-pixel, slope-minus-one triangles. They
// are not a substitute for an independent hardware/raw-memory reference oracle.
#include "rdp_fixture.hpp"
#include "35mmisle/photo_feedback.hpp"

#include <initializer_list>
#include <optional>
#include <sstream>
#include <stdexcept>

#include <utility>
namespace isle::rdp_probe {
namespace {
constexpr std::uint16_t clear_z = 0xfffc;
// Primitive Z is 15-bit, expanded by eight before the 18-to-14-bit Z packing.
// A: 0x4000 -> 0x20000 -> exponent 1, mantissa 0 -> visible 0x2000.
// B: 0x2000 -> 0x10000 -> base visible 0x1000, plus compressed dZ high bits.
// A uses dZ=8: log2=3 -> visible low bits 0, hidden 3.
// B uses dZ=32: log2=5 -> visible low bits 1, hidden 1. This explicitly
// distinguishes packed-Z feedback from a host-depth conversion that zeros them.
constexpr std::uint16_t a_z = 0x2000;
constexpr std::uint16_t b_z = 0x1001;
constexpr unsigned a_depth_dz = 3;
constexpr unsigned b_depth_dz = 1;
constexpr std::uint32_t half_bytes = 0xc00;
static_assert(plane_bytes == 2 * half_bytes);
static_assert(region_end - color_base == 0x27000);

constexpr std::uint32_t depth(unsigned i) { return depth_base + i * plane_bytes; }
constexpr std::uint32_t before(unsigned i) { return before_base + i * plane_bytes; }
constexpr std::uint32_t xy(unsigned x_quarters, unsigned y_quarters) {
    return (x_quarters << 12) | y_quarters;
}

class Stream {
public:
    Fixture fixture;
    Stream() {
        fixture.words.reserve(1024);
        fixture.packets.reserve(256);
    }
    void packet(std::initializer_list<std::uint32_t> words) {
        fixture.packets.push_back({fixture.words.size(), static_cast<unsigned>(words.size())});
        fixture.words.insert(fixture.words.end(), words.begin(), words.end());
    }
    void begin(unsigned index, StageKind kind) {
        fixture.stages[index] = {kind, fixture.packets.size(), 0};
    }
    void end(unsigned index) {
        packet({0x29000000, 0}); // SyncFull; host still must idle()/wait the timeline.
        auto &stage = fixture.stages[index];
        stage.packet_count = fixture.packets.size() - stage.first_packet;
    }
    void scissor(unsigned rows) {
        packet({0x2d000000, xy(width * 4, rows * 4)});
    }
    void color_image(std::uint32_t address) {
        packet({0x3f100000 | (width - 1), address}); // RGBA16, width=64.
    }
    void fill(std::uint32_t address, std::uint32_t word) {
        packet({0x27000000, 0});
        color_image(address);
        packet({0x37000000, word});
        packet({0x36000000 | xy((width - 1) * 4, (height - 1) * 4), 0});
    }
    void fill_mode() { packet({0x2f300000, 0}); }
    void triangle_mode() {
        // One cycle, RGB/alpha dither off; strict-less transparent Z mode,
        // AA, image-read, Z compare/update and primitive Z. No blending/alpha test.
        packet({0x2f0000f0, 0x0000087c});
        // (0-0)*0 + primitive for RGB and alpha, in BOTH combiner cycles.
        packet({0x3c000000 | (8u << 20) | (16u << 15) | (7u << 12) |
                    (7u << 9) | (8u << 5) | 16u,
                (8u << 28) | (8u << 24) | (7u << 21) | (7u << 18) |
                    (3u << 15) | (7u << 12) | (3u << 9) | (3u << 6) | (7u << 3) | 3u});
    }
    void triangle(bool object_b, std::uint32_t rgba, unsigned primitive_z) {
        const unsigned left_quarters = object_b ? 81 : 33; // 20.25 / 8.25.
        const unsigned top = object_b ? 12 : 4;
        const unsigned leg = object_b ? 32 : 40;
        const unsigned bottom = top + leg;
        const std::uint32_t left = left_quarters << 14;
        const std::uint32_t right = (left_quarters + leg * 4) << 14;
        packet({0x27000000, 0});
        packet({0x3a000000, rgba});
        packet({0x2e000000, (primitive_z << 16) | (object_b ? 32u : 8u)});
        // Eight-word, untextured raw triangle. Major edge is vertical left;
        // upper minor edge has slope -1; YM=YL makes the lower edge empty.
        packet({0x08800000 | (bottom * 4), ((bottom * 4) << 16) | (top * 4),
                left, 0, left, 0, right, 0xffff0000});
    }
    void copy_plane(std::uint32_t source, std::uint32_t destination) {
        for (unsigned half = 0; half != 2; ++half) {
            const auto offset = half * half_bytes;
            packet({0x27000000, 0});
            color_image(destination + offset);
            // One 64x24 RGBA16 tile is 0xC00 bytes, below TMEM's 0x1000 limit.
            packet({0x3d100000 | (width - 1), source + offset});
            packet({0x28000000, 0});
            packet({0x35102000, 0x00080200}); // 128-byte line; clamp S/T, tile 0.
            packet({0x26000000, 0});
            packet({0x34000000, xy((width - 1) * 4, 23 * 4)});
            packet({0x27000000, 0});
            packet({0x32000000, xy((width - 1) * 4, 23 * 4)});
            // Copy cycle advances four texels per S step; dS/dX=4, dT/dY=1.
            packet({0x24000000 | xy((width - 1) * 4, 23 * 4), 0, 0, 0x10000400});
        }
    }
};

std::uint8_t byte(std::span<const std::byte> memory, std::uint32_t address) {
    return std::to_integer<std::uint8_t>(memory[address ^ 3u]);
}
std::uint16_t halfword(std::span<const std::byte> memory, std::uint32_t address) {
    return static_cast<std::uint16_t>((unsigned(byte(memory, address)) << 8) |
                                      byte(memory, address + 1));
}
[[noreturn]] void mismatch(StageKind stage, const char *meaning, std::uint32_t address,
                          unsigned expected, unsigned actual) {
    const char *name = stage == StageKind::clear ? "clear" :
                       stage == StageKind::geometry ? "geometry" : "depth_copy";
    std::ostringstream message;
    message << "RDP " << name << " " << meaning << " at RDRAM 0x" << std::hex
            << address << ": expected 0x" << expected << ", got 0x" << actual;
    throw std::runtime_error(message.str());
}
void expect(StageKind stage, const char *meaning, std::uint32_t address,
            unsigned expected, unsigned actual) {
    if (actual != expected) mismatch(stage, meaning, address, expected, actual);
}
void pixel(StageKind stage, std::span<const std::byte> ram,
           std::span<const std::byte> hidden, const char *meaning,
           std::uint32_t address, unsigned value, unsigned metadata) {
    expect(stage, meaning, address, value, halfword(ram, address));
    expect(stage, "hidden metadata (byte at RDRAM/2)", address, metadata,
           std::to_integer<unsigned>(hidden[address / 2]));
}
void clear_words(StageKind stage, std::span<const std::byte> ram,
                 std::span<const std::byte> hidden, std::uint32_t begin,
                 std::uint32_t end, std::uint32_t word, unsigned metadata) {
    for (auto address = begin; address != end; address += 4) {
        const std::uint32_t actual = (std::uint32_t(halfword(ram, address)) << 16) |
                                    halfword(ram, address + 2);
        expect(stage, "fill word", address, word, actual);
        for (unsigned offset : {0u, 2u})
            expect(stage, "fill hidden metadata", address + offset, metadata,
                   std::to_integer<unsigned>(hidden[(address + offset) / 2]));
    }
}

// Independent closed-form geometric check, NOT an RDP decoder/replay. In quarter
// pixel coordinates the eight hardware sample sites are (0,0),(2,0),(1,1),
// (3,1),(0,2),(2,2),(1,3),(3,3). Both right triangles obey X>=left and
// X+Y<left+top+leg, with top<=Y<bottom. No edge stepping or Z LUT is emulated.
unsigned coverage(bool object_b, unsigned x, unsigned y) {
    const unsigned top = object_b ? 12 : 4;
    if (y < top || y >= 44) return 0;
    const unsigned left = object_b ? 81 : 33;
    const unsigned diagonal = object_b ? 257 : 209;
    unsigned covered = 0;
    for (unsigned sy = 0; sy != 4; ++sy) {
        for (unsigned sample = 0; sample != 2; ++sample) {
            const unsigned sx = (sy & 1u) + 2 * sample;
            const unsigned sample_x = 4 * x + sx;
            const unsigned sample_y = 4 * y + sy;
            if (sample_x >= left && sample_x + sample_y < diagonal) ++covered;
        }
    }
    return covered;
}
unsigned object_depth(unsigned count, unsigned z) { return count ? z : clear_z; }
unsigned current_depth(unsigned a, unsigned b) { return b ? b_z : object_depth(a, a_z); }
} // namespace

Fixture make_fixture() {
    Stream stream;
    stream.begin(0, StageKind::clear);
    stream.scissor(height);
    stream.packet({0x3e000000, 0}); // Nonaliasing, unused Z target during fills.
    stream.fill_mode();
    stream.fill(color_base, 0x00010001);
    for (unsigned plane = 0; plane != 13; ++plane)
        stream.fill(depth(plane), 0xfffcfffc);
    for (unsigned plane = 0; plane != 12; ++plane)
        stream.fill(before(plane), 0xfffcfffc);
    stream.end(0);

    stream.begin(1, StageKind::geometry);
    stream.color_image(color_base);
    stream.triangle_mode();
    stream.packet({0x3e000000, depth(1)});
    stream.triangle(false, 0xff0000ff, 0x4000); // Object A alone.
    stream.packet({0x3e000000, depth(2)});
    stream.triangle(true, 0x00ff00ff, 0x2000); // Object B alone.
    stream.packet({0x27000000, 0});
    stream.fill_mode();
    stream.fill(color_base, 0x00010001); // Restore full background coverage.
    stream.packet({0x27000000, 0});
    stream.triangle_mode();
    stream.packet({0x3e000000, depth(0)});
    stream.triangle(false, 0xff0000ff, 0x4000);
    stream.triangle(true, 0x00ff00ff, 0x2000); // Near B must occlude A.
    stream.triangle(true, 0x0000ffff, 0x6000); // Far blue must be fully rejected.
    stream.end(1);

    stream.begin(2, StageKind::depth_copy);
    stream.packet({0x27000000, 0});
    stream.packet({0x2f200000, 0}); // Copy cycle, no alpha/Z tests or update.
    stream.packet({0x3e000000, 0});
    stream.scissor(24);
    stream.copy_plane(depth(12), before(0)); // Before A: untouched far depth.
    stream.copy_plane(depth(1), before(1));  // Before B: A's depth.
    stream.copy_plane(depth(0), depth(3));   // Capture complete current depth.
    stream.end(2);
    return std::move(stream.fixture);
}

void validate_stage(StageKind kind, std::span<const std::byte> rdram,
                    std::span<const std::byte> hidden) {
    if (rdram.size() < rdram_size || hidden.size() < hidden_size)
        throw std::runtime_error("RDP validation requires absolute-address 8 MiB RDRAM and 4 MiB hidden spans");
    if (kind != StageKind::clear && kind != StageKind::geometry && kind != StageKind::depth_copy)
        throw std::runtime_error("RDP validation received an unknown stage");
    // Nearby red zones were initialized by the caller, never by fixture packets.
    for (unsigned i = 0; i != 16; ++i) {
        for (const auto address : {color_base - 16 + i, region_end + i}) {
            expect(kind, "untouched boundary byte", address, 0xa5, byte(rdram, address));
            expect(kind, "untouched boundary hidden byte", address, 0,
                   std::to_integer<unsigned>(hidden[address / 2]));
        }
    }
    if (kind == StageKind::clear) {
        clear_words(kind, rdram, hidden, color_base, depth_base, 0x00010001, 3);
        // One contiguous range checks EVERY word, including depth[12]'s last.
        clear_words(kind, rdram, hidden, depth_base, before_base, 0xfffcfffc, 0);
        clear_words(kind, rdram, hidden, before_base, region_end, 0xfffcfffc, 0);
        return;
    }
    struct FeedbackCounts {
        photo_feedback::WideCounts wide{};
        photo_feedback::FinalCounts narrow{};
    };
    std::optional<FeedbackCounts> expected_feedback;
    if (kind == StageKind::depth_copy) expected_feedback.emplace();


    for (unsigned y = 0; y != height; ++y) {
        for (unsigned x = 0; x != width; ++x) {
            const auto offset = 2 * (y * width + x);
            const auto a = coverage(false, x, y);
            const auto b = coverage(true, x, y);
            const auto current = current_depth(a, b);
            const auto winning_coverage = b ? b : a;
            const auto color_coverage = winning_coverage ? winning_coverage - 1 : 7;
            const auto rgb = b ? 0x07c0u : a ? 0xf800u : 0u;
            pixel(kind, rdram, hidden, "primitive RGB/edge coverage/occlusion", color_base + offset,
                  rgb | (color_coverage >> 2), color_coverage & 3);
            for (unsigned plane = 0; plane != 13; ++plane) {
                unsigned expected = clear_z;
                unsigned metadata = 0;
                if (plane == 0) {
                    expected = current;
                    metadata = b ? b_depth_dz : a ? a_depth_dz : 0;
                } else if (plane == 1 || plane == 2) {
                    const auto count = plane == 1 ? a : b;
                    expected = object_depth(count, plane == 1 ? a_z : b_z);
                    metadata = count ? (plane == 1 ? a_depth_dz : b_depth_dz) : 0;
                } else if (plane == 3 && kind == StageKind::depth_copy) {
                    expected = current;
                    // Texture-copy hidden bits encode RGBA coverage, NOT Z dZ.
                    metadata = (expected & 1) ? 3 : 0;
                }
                pixel(kind, rdram, hidden, "object/current/copied/untouched depth", depth(plane) + offset,
                      expected, metadata);
            }
            for (unsigned plane = 0; plane != 12; ++plane) {
                const auto expected = plane == 1 && kind == StageKind::depth_copy ? object_depth(a, a_z) : clear_z;
                pixel(kind, rdram, hidden, "before snapshot", before(plane) + offset,
                      expected, (expected & 1) ? 3 : 0);
            }
            if (kind == StageKind::depth_copy) {
                // Exercise the original visibility predicate on real GPU buffers,
                // against the independently determined geometric winner.
                const auto gpu_current = halfword(rdram, depth(0) + offset);
                for (unsigned object = 0; object != 2; ++object) {
                    const auto object_z = halfword(rdram, depth(object + 1) + offset);
                    const auto before_z = halfword(rdram, before(object) + offset);
                    const bool visible = object_z != clear_z && object_z == gpu_current && before_z != gpu_current;
                    const bool expected = object == 0 ? a != 0 && b == 0 : b != 0;
                    expect(kind, object == 0 ? "A visibility predicate" : "B visibility predicate",
                           depth(object + 1) + offset, expected, visible);
                    auto& wide = expected_feedback->wide[object];
                    auto& narrow = expected_feedback->narrow[object];
                    if (object == 0 ? a != 0 : b != 0) {
                        ++wide.total; ++narrow.total;
                        if (x >= 17 && x <= 46 && y >= 13 && y <= 34) ++wide.center;
                        if (x >= 30 && x <= 33 && y >= 22 && y <= 25) ++narrow.center;
                        if (expected) {
                            ++narrow.visible;
                            if (x >= 31 && x <= 32 && y >= 23 && y <= 24) ++narrow.visible_center;
                        }
                    }
                }
                // This fixture also copies completed scene depth into A[2].
                // Its B[2] stays clear, so its winner is the union of A and B.
                if (a != 0 || b != 0) {
                    auto& wide = expected_feedback->wide[2];
                    auto& narrow = expected_feedback->narrow[2];
                    ++wide.total; ++narrow.total; ++narrow.visible;
                    if (x >= 17 && x <= 46 && y >= 13 && y <= 34) ++wide.center;
                    if (x >= 30 && x <= 33 && y >= 22 && y <= 25) ++narrow.center;
                    if (x >= 31 && x <= 32 && y >= 23 && y <= 24) ++narrow.visible_center;
                }
            }
        }
    }
    if (expected_feedback) {
        photo_feedback::PlanesView view;
        photo_feedback::WideCounts wide{};
        photo_feedback::FinalCounts narrow{};
        const auto bytes = std::span(reinterpret_cast<const std::uint8_t*>(rdram.data()), rdram.size());
        if (!photo_feedback::PlanesView::borrow(bytes, color_base, view) ||
            !photo_feedback::count_wide(view, wide) ||
            !photo_feedback::count_narrow(view, wide, narrow))
            throw std::runtime_error("Completed RDP planes rejected by native photo feedback reader");
        for (unsigned subject = 0; subject != photo_feedback::subject_count; ++subject) {
            const auto& expected_wide = expected_feedback->wide[subject];
            auto& expected_narrow = expected_feedback->narrow[subject];
            expected_narrow.approximate_total = expected_wide.center
                ? expected_wide.total * expected_narrow.total / expected_wide.center
                : expected_wide.total * 4;
            const auto address = depth(subject + 1);
            expect(kind, "native wide total", address, expected_wide.total, wide[subject].total);
            expect(kind, "native wide center", address, expected_wide.center, wide[subject].center);
            expect(kind, "native narrow total", address, expected_narrow.total, narrow[subject].total);
            expect(kind, "native narrow center", address, expected_narrow.center, narrow[subject].center);
            expect(kind, "native unobstructed total", address, expected_narrow.visible, narrow[subject].visible);
            expect(kind, "native unobstructed center", address,
                   expected_narrow.visible_center, narrow[subject].visible_center);
            expect(kind, "native approximate total", address,
                   expected_narrow.approximate_total, narrow[subject].approximate_total);
        }
    }
}
} // namespace isle::rdp_probe
