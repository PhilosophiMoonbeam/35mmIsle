#ifndef ISLE_COURSE_RESOURCES_HPP
#define ISLE_COURSE_RESOURCES_HPP

#include "beach_world_resources.hpp"
#include "model_asset_closure.hpp"

namespace isle::course {
// Original GAME scene/course IDs, not host display ordering.
inline constexpr std::int32_t beach_id = 0, tunnel_id = 1, volcano_id = 2, river_id = 3,
                              cave_id = 4, valley_id = 5, rainbow_id = 6;
struct HeightLayout {
    std::uint32_t root, patches, tree;
    std::size_t patch_count, node_count;
};
struct Layout {
    std::int32_t id;
    std::uint32_t world_setup;
    std::span<const model_assets::Mapping> mappings;
    std::uint32_t block_render_callback, sky_render_callback;
    HeightLayout ground, ceiling;
};
// Immutable source-framed initialized ROM mappings. Guest callback identities
// remain provenance only; this metadata never grants executable-code access.
const Layout& layout(std::int32_t id);
// One descriptor decoder serves the complete world and Beach's independent
// first-block diagnostic. Primary tables retain their NULL/repeated-first
// terminal entry; secondary tables retain their NULL terminal entry.
beach::WorldDescriptors world_descriptors(const rom::Snapshot&, std::int32_t id);
} // namespace isle::course
#endif
