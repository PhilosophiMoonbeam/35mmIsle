#include "course_resources.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <stdexcept>

namespace isle::course {
namespace {
using model_assets::Mapping;
// ethteck/pokemonsnap df8815e75f3d79618466e01e0827c6165b93d11c,
// splat.yaml initialized segment boundaries; app_render/46270.c:64-122
// selects the course assets/HD/common-species groups. Course-code initialized
// data also contains the original collision/height descriptors. No text/BSS is
// admitted here and only the eventual graphics closure is installed.
constexpr Mapping world_data{0x800e6ad0, 0x64280, 0x731b0 - 0x64280};
constexpr Mapping magikarp_images{0x800f5d90, 0x731b0, 0x7d3b0 - 0x731b0};
constexpr Mapping pikachu_images{0x800fff90, 0x7d3b0, 0x98470 - 0x7d3b0};
constexpr Mapping zubat_images{0x8011b050, 0x98470, 0x99f70 - 0x98470};
constexpr Mapping bulbasaur_images{0x8011cb50, 0x99f70, 0xa74e0 - 0x99f70};
constexpr Mapping magikarp_hd{0x803b1f80, 0x82f8e0, 0x832960 - 0x82f8e0};
constexpr Mapping pikachu_hd{0x803ad580, 0x832960, 0x837360 - 0x832960};
constexpr Mapping zubat_hd{0x803aa700, 0x837360, 0x83a1e0 - 0x837360};
constexpr Mapping bulbasaur_hd{0x803a71b0, 0x83a1e0, 0x83d730 - 0x83a1e0};
constexpr std::array beach_mappings{
    Mapping{0x8011b060, 0xa74f0, 0x13c780 - 0xa74f0},
    Mapping{0x801b0310, 0x13c780, 0x162cb0 - 0x13c780},
    Mapping{0x802cbe10, 0x563e80, 0x5df5d0 - 0x563e80},
    world_data, magikarp_images, magikarp_hd, pikachu_images, pikachu_hd};
constexpr std::array tunnel_mappings{
    Mapping{0x8011cb60, 0x162cc0, 0x1d1d90 - 0x162cc0},
    Mapping{0x8018bc50, 0x1d1d90, 0x1f5e70 - 0x1d1d90},
    Mapping{0x802edf00, 0x5eafd0, 0x6401b0 - 0x5eafd0},
    world_data, magikarp_images, magikarp_hd, pikachu_images, pikachu_hd,
    zubat_images, zubat_hd};
constexpr std::array volcano_mappings{
    Mapping{0x800fffa0, 0x326c20, 0x3d0560 - 0x326c20},
    Mapping{0x801a9900, 0x3d0560, 0x3f63d0 - 0x3d0560},
    Mapping{0x802e0c80, 0x731e80, 0x79f1b0 - 0x731e80},
    world_data, magikarp_images, magikarp_hd};
constexpr std::array river_mappings{
    Mapping{0x8012a0d0, 0x29a1a0, 0x30af90 - 0x29a1a0},
    Mapping{0x8019aee0, 0x30af90, 0x326c10 - 0x30af90},
    Mapping{0x802e2620, 0x6ca400, 0x7272e0 - 0x6ca400},
    world_data, magikarp_images, magikarp_hd, pikachu_images, pikachu_hd,
    bulbasaur_images, bulbasaur_hd};
constexpr std::array cave_mappings{
    Mapping{0x8012a0d0, 0x1f5e80, 0x27ab80 - 0x1f5e80},
    Mapping{0x801aedf0, 0x27ab80, 0x29a190 - 0x27ab80},
    Mapping{0x802c6150, 0x648600, 0x6c05e0 - 0x648600},
    world_data, magikarp_images, magikarp_hd, pikachu_images, pikachu_hd,
    zubat_images, zubat_hd, bulbasaur_images, bulbasaur_hd};
constexpr std::array valley_mappings{
    Mapping{0x800fffa0, 0x3f63e0, 0x47cf30 - 0x3f63e0},
    Mapping{0x80186b10, 0x47cf30, 0x4a8160 - 0x47cf30},
    Mapping{0x802d2740, 0x7abcd0, 0x825e30 - 0x7abcd0},
    world_data, magikarp_images, magikarp_hd};
constexpr std::array rainbow_mappings{
    Mapping{0x800f5da0, 0x4a8170, 0x4ec000 - 0x4a8170},
    Mapping{0x80139c50, 0x4ec000, 0x4f0610 - 0x4ec000},
    Mapping{0x8034ab10, 0x82a280, 0x82f8e0 - 0x82a280}, world_data};
// WorldSetup roots follow the typed source declarations, not each assets
// segment's first datum (Tunnel starts with static definitions; Volcano with
// collision models). Height roots come from each course's main/init function.
// Counts are exact initialized indexed-array closures: every node and patch is
// reachable, source-ordered and bounded before the next initialized record.
// Tunnel and Cave additionally install the independently bounded ceiling map;
// Rainbow main.c never installs either map.
constexpr std::array<Layout, 7> layouts{{
    {beach_id, 0x8011b914, beach_mappings, 0x800a1530, 0x800e1ca4,
        {0x80318f00, 0x802ce1b0, 0x802ed498, 6386, 6385}, {}},
    {tunnel_id, 0x8011e6cc, tunnel_mappings, 0x800a1530, 0,
        {0x80326ee0, 0x802f0420, 0x80303d40, 4008, 4007},
        {0x80326ee8, 0x8031f390, 0x803226f8, 658, 657}},
    {volcano_id, 0x800fffb8, volcano_mappings, 0x800a1530, 0x800e1ca4,
        {0x8031d4d0, 0x802e3a00, 0x802fba88, 4922, 4921}, {}},
    {river_id, 0x8012ac90, river_mappings, 0x800a1530, 0x800e1ca4,
        {0x80321560, 0x802e4ca0, 0x802fe04c, 5167, 5166}, {}},
    {cave_id, 0x8012a0e8, cave_mappings, 0x800a1530, 0,
        {0x80317610, 0x802c8160, 0x802dfcd4, 4857, 4856},
        {0x80317618, 0x80301000, 0x8030a538, 1910, 1909}},
    {valley_id, 0x80100720, valley_mappings, 0x800a1530, 0x800e1ca4,
        {0x8031f9c0, 0x802ed1e0, 0x80302284, 4309, 4308}, {}},
    {rainbow_id, 0x800f5da0, rainbow_mappings, 0x800e1d80, 0x800e1d80, {}, {}}
}};
constexpr std::array<std::uint8_t, 20> admitted_sha1{
    0xed,0xc7,0xc4,0x9c,0xc5,0x68,0xc0,0x45,0xfe,0x48,
    0xbe,0x0d,0x18,0x01,0x1c,0x30,0xf3,0x93,0xcb,0xaf};
constexpr std::size_t max_blocks = 13; // world/world.h MAX_BLOCKS.
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
float finite(const rom::GuestView& wire, std::size_t offset) {
    const auto value = wire.f32(offset);
    require(std::isfinite(value), "Nonfinite GAME course descriptor scalar");
    return value;
}
std::array<std::uint8_t, 3> rgb(const rom::GuestView& wire, std::size_t offset) {
    return {wire.u8(offset), wire.u8(offset + 1), wire.u8(offset + 2)};
}
beach::BlockMetadata block(const model_assets::Reader& reader, std::uint32_t address) {
    beach::BlockMetadata result{};
    result.wire = reader.view(address, 0x24);
    const auto& w = result.wire;
    result.world_position = {finite(w, 4), finite(w, 8), finite(w, 12)};
    result.yaw = finite(w, 16);
    require(w.u32(20) <= 1, "Invalid GAME world block boolean");
    result.reversed = w.u32(20) != 0;
    result.statics_guest = w.u32(24);
    result.spawn_guest = w.u32(28);
    result.static_objects_guest = w.u32(32);
    if (const auto gfx = w.u32(0)) {
        result.gfx_wire = reader.view(gfx, 0x20);
        const auto& g = result.gfx_wire;
        result.display_list_guest = g.u32(0);
        result.materials_guest = g.u32(4);
        result.texture_animation_guest = g.u32(8);
        result.render_callback_guest = g.u32(12);
        result.road_guest = g.u32(16);
        result.control_line_count = std::bit_cast<std::int32_t>(g.u32(20));
        result.movement_animation_guest = g.u32(24);
        result.movement_animation_duration = std::bit_cast<std::int32_t>(g.u32(28));
    }
    return result;
}
rom::GuestView group(const model_assets::Reader& reader, std::uint32_t address,
                     std::vector<beach::BlockMetadata>& blocks, bool allow_loop) {
    if (!address) return {};
    for (std::size_t i = 0; i <= max_blocks; ++i) {
        const auto target = reader.view(address + static_cast<std::uint32_t>(i * 4), 4).u32(0);
        if (!target || (allow_loop && i && target == blocks.front().wire.guest_address))
            return reader.view(address, (i + 1) * 4);
        require(i < max_blocks, "GAME block table exceeds MAX_BLOCKS");
        require(std::none_of(blocks.begin(), blocks.end(), [target](const auto& previous) {
            return previous.wire.guest_address == target;
        }), "Unexpected repeated GAME block descriptor");
        blocks.push_back(block(reader, target));
    }
    throw std::runtime_error("Unterminated GAME block table");
}
} // namespace
const Layout& layout(std::int32_t id) {
    if (id < beach_id || id > rainbow_id) throw std::invalid_argument("Unknown original GAME course ID");
    return layouts[static_cast<std::size_t>(id)];
}
beach::WorldDescriptors world_descriptors(const rom::Snapshot& snapshot, std::int32_t id) {
    require(snapshot.canonical().size() == rom::us_size && snapshot.identity().size == rom::us_size &&
            snapshot.identity().sha1 == admitted_sha1, "Course descriptors require admitted US Snapshot");
    const auto& selected = layout(id);
    const model_assets::Reader reader(snapshot, selected.mappings);
    beach::WorldDescriptors result{};
    auto& world = result.world;
    world.wire = reader.view(selected.world_setup, 0x1e);
    const auto& w = world.wire;
    world.blocks_setup_guest = w.u32(0);
    world.static_definitions_guest = w.u32(4);
    world.unk08 = std::bit_cast<std::int32_t>(w.u32(8));
    world.collision_models_guest = w.u32(12);
    world.sky_animation_speed = finite(w, 16);
    world.fog_min = w.u16(20); world.fog_max = w.u16(22);
    world.fog_rgb = rgb(w, 24); world.background_rgb = rgb(w, 27);
    require(world.fog_min <= world.fog_max, "Invalid GAME fog interval");
    world.blocks_setup_wire = reader.view(world.blocks_setup_guest, 12);
    const auto& b = world.blocks_setup_wire;
    world.primary_table_wire = group(reader, b.u32(0), result.primary, true);
    world.secondary_table_wire = group(reader, b.u32(4), result.secondary, false);
    require(!result.primary.empty() && result.primary.size() + result.secondary.size() < max_blocks,
            "Invalid GAME block group extent");
    if (const auto sky = b.u32(8)) {
        result.sky.wire = reader.view(sky, 20);
        result.sky.display_list_guest = result.sky.wire.u32(0);
        result.sky.render_callback_guest = result.sky.wire.u32(4);
        result.sky.materials_guest = result.sky.wire.u32(8);
        result.sky.animation_guest = result.sky.wire.u32(12);
        result.sky.animation_speed = finite(result.sky.wire, 16);
    }
    return result;
}
} // namespace isle::course
