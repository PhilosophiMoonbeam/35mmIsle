#ifndef ISLE_BEACH_WORLD_RESOURCES_HPP
#define ISLE_BEACH_WORLD_RESOURCES_HPP

#include "rom_snapshot.hpp"
#include "rom_view.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace isle::beach {
inline constexpr std::size_t rdram_size = 8 * 1024 * 1024;
struct RamReservation { std::size_t offset, size; }; // Logical physical RAM byte range, NOT host-swapped indices.
inline constexpr std::array<RamReservation, 3> producer_reservations{{
    {0x100000, 0x10000}, {0x200000, 0x10000}, {0x300000, 0x27000}
}};
struct Vec3 { float x, y, z; };
struct WorldMetadata {
    rom::GuestView wire, blocks_setup_wire, primary_table_wire, secondary_table_wire;
    std::uint32_t blocks_setup_guest, static_definitions_guest, collision_models_guest;
    std::int32_t unk08;
    float sky_animation_speed;
    std::uint16_t fog_min, fog_max;
    std::array<std::uint8_t, 3> fog_rgb, background_rgb;
};
// Metadata only for both null-terminated descriptor groups. Only primary[0]'s
// model is hydrated. No road, movement, static-object, actor or spawn data is
// installed, nor any other block's geometry/material/animation data.
struct BlockMetadata {
    rom::GuestView wire, gfx_wire; // Exact 0x24 descriptor and 0x20 fixed GFX header.
    Vec3 world_position;
    float yaw;
    bool reversed;
    std::uint32_t display_list_guest, materials_guest, texture_animation_guest, render_callback_guest;
    std::uint32_t road_guest, movement_animation_guest;
    std::int32_t control_line_count, movement_animation_duration; // Duration is s32, NOT binary32.
    std::uint32_t statics_guest, spawn_guest, static_objects_guest;
};
struct SkyMetadata {
    rom::GuestView wire;
    std::uint32_t display_list_guest, render_callback_guest, materials_guest, animation_guest;
    float animation_speed;
};
struct WorldDescriptors {
    WorldMetadata world;
    SkyMetadata sky;
    std::vector<BlockMetadata> primary, secondary;
};
// Metadata-only admission shared by the full native world and the independent
// first-block diagnostic. Does not scan/install any GBI or runtime records.
WorldDescriptors world_descriptors(const rom::Snapshot&);
struct Command { std::uint32_t w0, w1; };
struct DisplayList {
    rom::GuestView wire; // Through ENDDL or a terminating no-push G_DL, inclusive.
    std::size_t command_count() const noexcept { return wire.bytes.size() / 8; }
    Command command(std::size_t index) const;
};
struct VertexLoad {
    rom::GuestView wire;
    std::uint32_t command_guest;
    std::uint8_t first_slot, count;
};
enum class ImageLoadKind { texture_block, rgba16_palette };
struct ImageLoad {
    rom::GuestView wire; // Only bytes actually consumed, not padded texture declarations.
    std::uint32_t set_image_command_guest, load_command_guest;
    ImageLoadKind kind;
    std::uint8_t source_format, source_size_code;
    std::uint16_t source_width, unit_count; // Units are source texels, palette entries for TLUT.
};
struct ResourceCounts {
    std::size_t display_lists = 0, commands = 0, vertex_loads = 0, loaded_vertices = 0;
    std::size_t unique_vertices = 0, texture_loads = 0, palette_loads = 0;
    std::size_t texture_bytes = 0, palette_bytes = 0, installation_spans = 0, installed_bytes = 0;
    std::size_t excluded_blocks = 0, excluded_material_blocks = 0;
};

// Original Beach FIRST STATIC BLOCK + SKY ONLY, not an entire level or scene.
// Metadata is native/borrowed and is NOT installed into RAM. Installation is
// exactly the merged immutable GBI closure: lists, referenced vertices, image
// load bytes and TLUT bytes. Callback guest values are identity/provenance, never
// executable host functions. GAME requires renderModelTypeAFogged (0x800A1530)
// and translate-only matrices for the first block; yaw does not imply RTS.
// Sky callback 0x800E1CA4 remains a guest identity. StartProducer supplies its
// independently established GAME drawSkyBox1Cycle-equivalent behavior and the
// source translate-only follow-camera model; resources never execute callbacks.
//
// The Snapshot AND its storage must outlive this nonmovable object and all
// views. Do not move the Snapshot while borrowed; install detects that breach.
// Construction owns descriptor allocations; reads/install never allocate.
class StartResources final {
public:
    explicit StartResources(const rom::Snapshot& snapshot);
    StartResources(rom::Snapshot&&) = delete;
    StartResources(const StartResources&) = delete;
    StartResources& operator=(const StartResources&) = delete;
    StartResources(StartResources&&) = delete;
    StartResources& operator=(StartResources&&) = delete;

    const WorldMetadata& world() const noexcept { return world_; }
    std::span<const BlockMetadata> primary_blocks() const noexcept { return primary_; }
    std::span<const BlockMetadata> secondary_blocks() const noexcept { return secondary_; }
    const BlockMetadata& start_block() const noexcept { return primary_.front(); }
    const SkyMetadata& sky() const noexcept { return sky_; }
    std::span<const DisplayList> display_lists() const noexcept { return lists_; }
    std::span<const VertexLoad> vertex_loads() const noexcept { return vertices_; }
    std::span<const ImageLoad> image_loads() const noexcept { return images_; }
    const ResourceCounts& counts() const noexcept { return counts_; }
    std::span<const rom::GuestView> installation_spans() const noexcept { return install_; }

    // Caller-owned canonical-backend WORD-SWAPPED 8 MiB. Original guest pointers
    // stay unchanged: dst[(guest - 0x80000000 + i) XOR 3] = source[i].
    // Validates extent, live borrowing, all write spans and all reservations
    // BEFORE any write. Default producer/FIFO/planes reservations above always
    // apply; caller reservations are additional. Resource overlap, RAM/ROM
    // aliasing, invalid reservation extent and moved-from input throw. All other
    // bytes remain untouched, including bytes sharing a partial palette word.
    void install(std::span<std::byte> word_swapped_rdram,
                 std::span<const RamReservation> additional_reservations = {}) const;
private:
    const rom::Snapshot* snapshot_;
    const std::byte* source_data_;
    WorldMetadata world_{};
    std::vector<BlockMetadata> primary_, secondary_;
    SkyMetadata sky_{};
    std::vector<DisplayList> lists_;
    std::vector<VertexLoad> vertices_;
    std::vector<ImageLoad> images_;
    std::vector<rom::GuestView> install_;
    ResourceCounts counts_{};
};
} // namespace isle::beach
#endif
