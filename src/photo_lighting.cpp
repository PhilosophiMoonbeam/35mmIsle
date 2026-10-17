#include "photo_lighting.hpp"

#include <bit>
#include <cfenv>
#include <cmath>
#include <limits>
#include <stdexcept>

#if defined(__FAST_MATH__) || (defined(__FINITE_MATH_ONLY__) && __FINITE_MATH_ONLY__ > 0)
#error "GAME photo lighting requires strict binary32 arithmetic, not fast-math"
#endif

namespace isle::photo {
namespace {
static_assert(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559);
constexpr std::uint32_t ram_base = 0x80000000;
constexpr std::uint32_t light_block_guest = 0x800aebd0;
constexpr std::uint32_t lighting_bit = 0x00020000;
struct Mapping { std::size_t rom_begin, rom_end; std::uint32_t guest_begin; };
// Exact pinned splat raw-data boundary, not a generated/dirty 5A580.c.
constexpr Mapping light_data{0x5a580, 0x5ad20, light_block_guest};
// splat photo_check: ROM 0x98c330 <-> guest 0x801dc8c0. Only this
// independently framed reference-free list is borrowed, not the overlay.
constexpr Mapping normal_data{0x994e10, 0x994e80, 0x801e53a0};

void require(bool condition) {
    if (!condition) throw std::runtime_error("Invalid admitted GAME photo lighting resources");
}
LightingSpan view(const rom::Snapshot& snapshot, Mapping mapping, std::uint32_t guest, std::size_t size) {
    require(guest >= mapping.guest_begin && guest % 8 == 0 && size != 0 && size % 8 == 0);
    const auto delta = static_cast<std::size_t>(guest - mapping.guest_begin);
    require(delta <= mapping.rom_end - mapping.rom_begin);
    const auto offset = mapping.rom_begin + delta;
    require(size <= mapping.rom_end - offset);
    const auto canonical = snapshot.canonical();
    require(offset <= canonical.size() && size <= canonical.size() - offset);
    return {guest, offset, canonical.subspan(offset, size)};
}
std::uint32_t u32(const LightingSpan& wire, std::size_t offset) {
    if (offset % 4 != 0 || offset > wire.bytes.size() || wire.bytes.size() - offset < 4)
        throw std::out_of_range("GAME photo lighting wire read");
    return (std::to_integer<std::uint32_t>(wire.bytes[offset]) << 24) |
           (std::to_integer<std::uint32_t>(wire.bytes[offset + 1]) << 16) |
           (std::to_integer<std::uint32_t>(wire.bytes[offset + 2]) << 8) |
            std::to_integer<std::uint32_t>(wire.bytes[offset + 3]);
}
LightingCommand command_at(const LightingSpan& wire, std::size_t index) {
    if (index >= wire.bytes.size() / 8) throw std::out_of_range("GAME photo lighting command index");
    return {u32(wire, index * 8), u32(wire, index * 8 + 4)};
}
struct HighMode { unsigned shift, length; };
HighMode high_mode(std::uint32_t word) {
    require((word & 0x00ff0000) == 0);
    const auto length = (word & 0xffu) + 1;
    const auto encoded_shift = (word >> 8) & 0xffu;
    require(length <= 32 && encoded_shift <= 32 - length);
    return {32 - length - encoded_shift, length};
}
void validate_normal_setup(const LightingSpan& wire) {
    unsigned pipes = 0, geometry = 0, modes = 0, clips = 0;
    for (std::size_t i = 0; i < wire.bytes.size() / 8; ++i) {
        const auto [w0, w1] = command_at(wire, i);
        switch (w0 >> 24) {
        case 0xe7:
            require(w0 == 0xe7000000 && w1 == 0 && pipes++ == 0);
            break;
        case 0xd9:
            require((w0 & 0x00ffffff) == 0x00ffffff && w1 == lighting_bit && geometry++ == 0);
            break;
        case 0xe3: {
            const auto [shift, length] = high_mode(w0);
            unsigned bit = 0;
            switch (shift) {
            case 23: bit = 1; require(length == 1 && w1 == 0); break; // NPRIMITIVE
            case 16: bit = 2; require(length == 1 && w1 == 0); break; // TILE
            case 17: bit = 4; require(length == 2 && w1 == 0); break; // CLAMP
            case 19: bit = 8; require(length == 1 && w1 == (1u << 19)); break; // PERSP
            case 14: bit = 16; require(length == 2 && w1 == 0); break; // TT_NONE
            case 12: bit = 32; require(length == 2 && w1 == (2u << 12)); break; // BILERP
            case 9: bit = 64; require(length == 3 && w1 == (6u << 9)); break; // FILT
            default: require(false);
            }
            require((modes & bit) == 0);
            modes |= bit;
            break;
        }
        case 0xdb: {
            require(((w0 >> 16) & 0xffu) == 4); // G_MW_CLIP
            const auto offset = w0 & 0xffffu;
            require(offset == 4 || offset == 12 || offset == 20 || offset == 28);
            const auto bit = 1u << ((offset - 4) / 8);
            require((clips & bit) == 0 && w1 == (offset < 20 ? 6u : 0xfffau));
            clips |= bit;
            break;
        }
        case 0xdf:
            require(w0 == 0xdf000000 && w1 == 0 && i + 1 == wire.bytes.size() / 8);
            break;
        default:
            // In particular, no G_DL, vertex/matrix/image DMA, or branches:
            // there are no missing nested resources to synthesize or ignore.
            require(false);
        }
    }
    require(wire.bytes.size() == 14 * 8 && pipes == 1 && geometry == 1 && modes == 127 && clips == 15);
    require(command_at(wire, 13).w0 == 0xdf000000);
}

// Explicit binary32 storage at each original GAME operation also prevents FMA
// contraction or reassociation of the source's left-associated squared sums.
float add(float a, float b) { volatile float result = a + b; return result; }
float subtract(float a, float b) { volatile float result = a - b; return result; }
float multiply(float a, float b) { volatile float result = a * b; return result; }
float divide(float a, float b) { volatile float result = a / b; return result; }
float root(float a) { volatile float result = std::sqrt(a); return result; }
LightDirection camera_direction(const IsleGameVec3& eye, const IsleGameVec3& at) {
    if (std::fegetround() != FE_TONEAREST || !std::isfinite(eye.x) || !std::isfinite(eye.z) ||
        !std::isfinite(at.x) || !std::isfinite(at.z))
        throw std::invalid_argument("GAME photo light requires finite horizontal camera inputs and nearest rounding");
    float x = subtract(eye.x, at.x);
    float z = subtract(eye.z, at.z);
    const float horizontal_squared = add(multiply(x, x), multiply(z, z));
    if (!(horizontal_squared > 0) || !std::isfinite(horizontal_squared))
        throw std::invalid_argument("GAME photo light has no finite nonzero horizontal direction");
    const float inverse_horizontal = divide(1.0f, root(horizontal_squared));
    float y = 1.0f;
    x = multiply(x, inverse_horizontal);
    z = multiply(z, inverse_horizontal);
    const float magnitude = root(add(add(multiply(x, x), multiply(y, y)), multiply(z, z)));
    const float inverse_magnitude = divide(1.0f, magnitude);
    x = multiply(x, inverse_magnitude);
    y = multiply(y, inverse_magnitude);
    z = multiply(z, inverse_magnitude);
    const std::array<float, 3> components{x, y, z};
    LightDirection direction{};
    for (std::size_t i = 0; i < components.size(); ++i) {
        const float scaled = multiply(components[i], 127.0f);
        if (!std::isfinite(scaled) || scaled < -128.0f || scaled >= 128.0f)
            throw std::invalid_argument("GAME photo light direction cannot be represented as signed bytes");
        // Conversion truncates toward zero; no clamp, rounding, or white-light fallback.
        direction[i] = static_cast<std::int8_t>(static_cast<std::int32_t>(scaled));
    }
    return direction;
}
} // namespace

LightingInputs::LightingInputs(const rom::Snapshot& snapshot) : snapshot_(&snapshot) {
    constexpr std::array<std::uint8_t, 20> sha1{
        0xed, 0xc7, 0xc4, 0x9c, 0xc5, 0x68, 0xc0, 0x45, 0xfe, 0x48,
        0xbe, 0x0d, 0x18, 0x01, 0x1c, 0x30, 0xf3, 0x93, 0xcb, 0xaf
    };
    require(snapshot.canonical().size() == rom::us_size && snapshot.identity().size == rom::us_size &&
            snapshot.identity().sha1 == sha1);
    pre_render_ = view(snapshot, light_data, 0x800aec18, 8 * 8);
    normal_setup_ = view(snapshot, normal_data, 0x801e53a0, 14 * 8);
    require(pre_render_.rom_offset == 0x5a5c8 && normal_setup_.rom_offset == 0x994e10);
    unsigned pipes = 0, geometry = 0, number = 0, references = 0, dithers = 0;
    for (std::size_t i = 0; i < command_count(); ++i) {
        const auto [w0, w1] = command(i);
        switch (w0 >> 24) {
        case 0xe7:
            require(w0 == 0xe7000000 && w1 == 0 && pipes++ == 0);
            break;
        case 0xd9:
            require((w0 & 0x00ffffff) == 0x00ffffff && w1 == lighting_bit && geometry++ == 0);
            break;
        case 0xdb:
            require((w0 & 0x00ffffff) == 0x00020000 && w1 == 24 && number++ == 0); // NUMLIGHTS_1
            break;
        case 0xdc: {
            require(references < lights_.size() && (w0 & 0x00ff00ff) == 0x0008000a);
            const auto size = (((w0 >> 19) & 31u) + 1) * 8;
            const auto offset = ((w0 >> 8) & 0xffu) * 8;
            require(size == 16 && offset == 48 + references * 24);
            const auto guest = references == 0 ? light_block_guest + 8 : light_block_guest;
            require(w1 == guest);
            lights_[references] = {view(snapshot, light_data, w1, size),
                                    static_cast<std::uint16_t>(offset),
                                    static_cast<std::uint8_t>(references + 1), references == 1};
            ++references;
            break;
        }
        case 0xe3: {
            const auto [shift, length] = high_mode(w0);
            require(length == 2 && w1 == 0 && (shift == 6 || shift == 4));
            const auto bit = shift == 6 ? 1u : 2u; // MAGICSQ, AD_PATTERN
            require((dithers & bit) == 0);
            dithers |= bit;
            break;
        }
        case 0xdf:
            require(w0 == 0xdf000000 && w1 == 0 && i + 1 == command_count());
            break;
        default:
            require(false); // Exact list has no nested DL or other reference kind.
        }
    }
    require(pipes == 1 && geometry == 1 && number == 1 && references == lights_.size() && dithers == 3);
    require(command(command_count() - 1).w0 == 0xdf000000);
    validate_normal_setup(normal_setup_);
    // Ambient's 16-byte DMA overlaps directional's first 8 bytes. Their exact
    // union is 24 bytes, including the source-owned mutable direction at +0x10.
    install_ = {view(snapshot, light_data, light_block_guest, 24), pre_render_, normal_setup_};
    for (std::size_t i = 0; i < install_.size(); ++i) {
        const auto& span = install_[i];
        require(span.guest_address >= ram_base);
        const auto offset = static_cast<std::size_t>(span.guest_address - ram_base);
        require(offset <= lighting_rdram_size && span.bytes.size() <= lighting_rdram_size - offset);
        for (std::size_t j = 0; j < i; ++j) {
            const auto& previous = install_[j];
            require(static_cast<std::uint64_t>(previous.guest_address) + previous.bytes.size() <= span.guest_address ||
                    static_cast<std::uint64_t>(span.guest_address) + span.bytes.size() <= previous.guest_address);
        }
    }
}
LightingCommand LightingInputs::command(std::size_t index) const { return command_at(pre_render_, index); }
LightingCommand LightingInputs::normal_setup_command(std::size_t index) const {
    return command_at(normal_setup_, index);
}
std::size_t LightingInputs::installed_byte_count() const noexcept {
    std::size_t count = 0;
    for (const auto& span : install_) count += span.bytes.size();
    return count;
}
LightDirection LightingInputs::initial_direction() const noexcept {
    LightDirection result{};
    const auto bytes = lights_[0].dma.bytes.subspan<8, 3>();
    for (std::size_t i = 0; i < result.size(); ++i)
        result[i] = std::bit_cast<std::int8_t>(std::to_integer<std::uint8_t>(bytes[i]));
    return result;
}
void LightingInputs::check_destination(std::span<std::byte> ram) const {
    if (ram.size() != lighting_rdram_size)
        throw std::invalid_argument("GAME photo lighting requires exclusive word-swapped 8 MiB RAM");
    const auto canonical = snapshot_->canonical();
    require(canonical.size() == rom::us_size && canonical.data() + pre_render_.rom_offset == pre_render_.bytes.data() &&
            canonical.data() + normal_setup_.rom_offset == normal_setup_.bytes.data());
    const auto source = reinterpret_cast<std::uintptr_t>(canonical.data());
    const auto destination = reinterpret_cast<std::uintptr_t>(ram.data());
    if (source <= destination ? destination - source < canonical.size() : source - destination < ram.size())
        throw std::invalid_argument("GAME photo lighting RAM must not alias its immutable Snapshot");
}
void LightingInputs::install(std::span<std::byte> ram) const {
    check_destination(ram);
    for (const auto& span : install_) {
        const auto offset = static_cast<std::size_t>(span.guest_address - ram_base);
        for (std::size_t i = 0; i < span.bytes.size(); ++i) ram[(offset + i) ^ 3u] = span.bytes[i];
    }
}
void LightingInputs::install_direction(std::span<std::byte> ram, const LightDirection& direction) const {
    check_destination(ram);
    const auto offset = static_cast<std::size_t>(direction_guest() - ram_base);
    for (std::size_t i = 0; i < direction.size(); ++i)
        ram[(offset + i) ^ 3u] = std::byte{std::bit_cast<std::uint8_t>(direction[i])};
}
LightDirection LightingInputs::update_direction(std::span<std::byte> ram,
                                                const IsleGameVec3& eye, const IsleGameVec3& at) const {
    check_destination(ram);
    const auto direction = camera_direction(eye, at);
    const auto offset = static_cast<std::size_t>(direction_guest() - ram_base);
    for (std::size_t i = 0; i < direction.size(); ++i)
        ram[(offset + i) ^ 3u] = std::byte{std::bit_cast<std::uint8_t>(direction[i])};
    return direction;
}
} // namespace isle::photo
