#ifndef ISLE_WORLD_RESOURCES_HPP
#define ISLE_WORLD_RESOURCES_HPP

#include "35mmisle/engine.h"
#include "beach_world_resources.hpp"
#include "menu_resources.hpp"
#include "model_asset_closure.hpp"
#include <optional>
#include <limits>

namespace isle::world {
inline constexpr std::size_t no_index = std::numeric_limits<std::size_t>::max();
using Vec3 = IsleGameVec3;
struct StaticObject {
    std::int32_t id = -1;
    Vec3 position{}, rotation{}, scale{1, 1, 1};
    rom::GuestView wire;
};
struct Spawn {
    std::int32_t id = -1, behavior = 0;
    Vec3 position{}, rotation{}, scale{1, 1, 1};
    std::size_t path = no_index;
    rom::GuestView wire;
};
struct BlockResource {
    beach::BlockMetadata metadata;
    std::size_t model = no_index, road_model = no_index;
    std::vector<float> control_timestamps;
    std::vector<StaticObject> decorations, collision_objects;
    std::vector<Spawn> spawns;
};
struct HeightPatch { float a, b, c, d; std::uint32_t surface; };
struct HeightNode {
    float a, b, c;
    std::size_t left_child = no_index, right_child = no_index;
    std::size_t left_patch = no_index, right_patch = no_index;
};
struct HeightMap { std::vector<HeightPatch> patches; std::vector<HeightNode> nodes; };
struct HitBox { std::uint8_t type = 0; Vec3 parameters{}; };
struct Collider {
    std::int32_t depth = 0;
    HitBox hitbox;
    bool has_hitbox = false;
    Vec3 position{}, rotation{}, scale{1, 1, 1};
};
struct CollisionModel { std::int32_t id = -1; float scale = 0; std::vector<Collider> colliders; };
struct StaticDefinition { std::int32_t id; std::uint32_t callback_identity, payload_identity; std::size_t model = no_index; };
// All immutable descriptors own native-size metadata and borrow only bounded ROM
// views. Neither a console object layout nor executable guest callbacks are used.
class Resources final {
public:
    explicit Resources(const rom::Snapshot&);
    Resources(rom::Snapshot&&) = delete;
    Resources(const Resources&) = delete;
    Resources& operator=(const Resources&) = delete;
    Resources(Resources&&) = delete;
    Resources& operator=(Resources&&) = delete;
    const beach::WorldMetadata& setup() const noexcept { return setup_; }
    const beach::SkyMetadata& sky() const noexcept { return sky_; }
    std::span<const BlockResource> blocks() const noexcept { return blocks_; }
    std::size_t primary_count() const noexcept { return primary_count_; }
    std::span<const menu::Model> models() const noexcept { return models_; }
    std::span<const menu::Material> materials() const noexcept { return materials_; }
    std::span<const menu::Animation> animations() const noexcept { return animations_; }
    std::span<const menu::AnimationPath> paths() const noexcept { return paths_; }
    const HeightMap& height_map() const noexcept { return height_map_; }
    std::span<const CollisionModel> collision_models() const noexcept { return collision_models_; }
    std::span<const StaticDefinition> static_definitions() const noexcept { return static_definitions_; }
    std::span<const rom::GuestView> installation_spans() const noexcept { return closure_->installation_spans(); }
    std::span<const menu::DisplayList> display_lists() const noexcept { return closure_->display_lists(); }
    std::span<const menu::VertexLoad> vertex_loads() const noexcept { return closure_->vertex_loads(); }
    std::span<const menu::ImageLoad> image_loads() const noexcept { return closure_->image_loads(); }
    std::span<const menu::DynamicMaterialCall> dynamic_material_calls() const noexcept { return closure_->dynamic_material_calls(); }
    void install(std::span<std::byte>, std::span<const beach::RamReservation> = {}) const;
    void require_live() const;
private:
    const rom::Snapshot* snapshot_;
    const std::byte* source_data_;
    beach::WorldMetadata setup_{};
    beach::SkyMetadata sky_{};
    std::vector<BlockResource> blocks_;
    std::size_t primary_count_ = 0;
    std::vector<menu::Model> models_;
    std::vector<menu::Material> materials_;
    std::vector<menu::Animation> animations_;
    std::vector<menu::AnimationPath> paths_;
    HeightMap height_map_;
    std::vector<CollisionModel> collision_models_;
    std::vector<StaticDefinition> static_definitions_;
    std::optional<model_assets::Closure> closure_;
    friend struct ResourceBuilder;
};
Vec3 path_position(const menu::AnimationPath&, float time);
Vec3 path_velocity(const menu::AnimationPath&, float time);
} // namespace isle::world
#endif
