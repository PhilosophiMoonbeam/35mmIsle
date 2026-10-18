#ifndef ISLE_ACTOR_RESOURCE_CATALOG_HPP
#define ISLE_ACTOR_RESOURCE_CATALOG_HPP

#include "actor_resources.hpp"
#include "model_data_decoder.hpp"

namespace isle::actor::detail {
// Private, pinned metadata; never a caller-supplied registry or executable API.
struct InitializationSpec {
    std::uint32_t actor_id, identity, model_identity, material_table_identity,
                  render_identity, setup_identity, initial_state_identity,
                  initial_header_identity;
    std::size_t nodes, header_begin, header_count;
    std::int8_t process_kind;
    std::uint32_t unknown_0c_identity, transition_graph_identity;
};
struct DefinitionSpec {
    std::uint32_t identity, actor_id, factory_identity, change_block_identity, remove_identity;
    std::size_t initialization;
    bool singleton;
};
struct DefinitionTableSpec {
    std::uint32_t identity, terminator_identity;
    std::size_t first_definition, live_rows;
};
struct DescriptorCounts {
    std::size_t models, materials, model_animations, material_animations, headers, definitions, paths;
};
// Source-owned sequential playback authority, never a pointer-array allocation.
struct MaterialPlaybackSpec {
    std::uint32_t material, program;
    std::span<const float> speeds;
};
struct CourseCatalog {
    std::int32_t course_id;
    std::uint32_t world_setup_identity;
    std::span<const model_assets::Mapping> mappings;
    std::span<const model_assets::MaterialExtent> material_extents;
    std::span<const InitializationSpec> initializations;
    std::span<const std::uint32_t> headers;
    std::span<const DefinitionSpec> definitions;
    std::span<const DefinitionTableSpec> tables;
    std::span<const AutomaticActorDefinition> automatic_definitions;
    DescriptorCounts expected;
    std::span<const MaterialPlaybackSpec> material_playbacks{};
};
// Exact maxima of the closed catalogs: fixed cold closure scratch, no temporary
// heap allocation and no aggregate overlay mappings.
inline constexpr std::size_t max_catalog_models = 23;
inline constexpr std::size_t max_catalog_paths = 5;
const CourseCatalog& beach_catalog() noexcept;
const CourseCatalog& tunnel_catalog() noexcept;
const CourseCatalog& volcano_catalog() noexcept;
const CourseCatalog& river_catalog() noexcept;
const CourseCatalog& cave_catalog() noexcept;
const CourseCatalog& valley_catalog() noexcept;
const CourseCatalog& rainbow_catalog() noexcept;
const CourseCatalog& course_catalog(std::int32_t course_id);
} // namespace isle::actor::detail
#endif
