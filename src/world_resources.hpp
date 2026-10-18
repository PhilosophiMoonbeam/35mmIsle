#ifndef ISLE_WORLD_RESOURCES_HPP
#define ISLE_WORLD_RESOURCES_HPP

#include "35mmisle/engine.h"
#include "beach_world_resources.hpp"
#include "course_resources.hpp"
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
struct HeightMap {
    rom::GuestView wire, patches_wire, nodes_wire;
    std::vector<HeightPatch> patches;
    std::vector<HeightNode> nodes;
};
struct HitBox { std::uint8_t type = 0; Vec3 parameters{}; };
struct Collider {
    std::int32_t depth = 0;
    HitBox hitbox;
    bool has_hitbox = false;
    Vec3 position{}, rotation{}, scale{1, 1, 1};
    rom::GuestView wire, hitbox_wire;
};
struct CollisionModel {
    std::int32_t id = -1;
    float scale = 0;
    std::vector<Collider> colliders;
    rom::GuestView wire, tree_wire;
    const rom::Snapshot* snapshot = nullptr;
    const std::byte* source_data = nullptr;
};
// Closed initialized Rainbow Gate record, not an arbitrary guest-root decoder.
// Snapshot must remain alive and unmoved until this and all prepared borrowers die.
// Legal US ROM: record [0x82a690,0x82a69c), tree [0x6b728,0x6b8b4).
// Eight nodes plus depth-18 sentinel: four transform-only nodes, three type-4
// boxes (half extents .5) and one excluded type-1 broadphase root (radius 1,
// scale 4). Box centers/scales: (2.23,2,0)/(1.1,4,1),
// (-2.23,2,0)/(1.1,4,1), (0,5,0)/(6,2,1). All rotations are zero.
class RainbowGateCollision final {
public:
    explicit RainbowGateCollision(const rom::Snapshot&);
    RainbowGateCollision(rom::Snapshot&&) = delete;
    RainbowGateCollision(const RainbowGateCollision&) = delete;
    RainbowGateCollision& operator=(const RainbowGateCollision&) = delete;
    RainbowGateCollision(RainbowGateCollision&&) = delete;
    RainbowGateCollision& operator=(RainbowGateCollision&&) = delete;
    const CollisionModel& model() const { require_live(); return model_; }
    std::span<const CollisionModel> models() const { require_live(); return {&model_, 1}; }
    void require_live() const;
private:
    CollisionModel model_;
};
struct StaticDefinition { std::int32_t id; std::uint32_t callback_identity, payload_identity; std::size_t model = no_index; };
// All immutable descriptors own native-size metadata and borrow only bounded ROM
// views. Neither a console object layout nor executable guest callbacks are used.
// The explicit original course ID selects immutable mappings and exact callback,
// height and ceiling authority. Missing source sky/maps remain absent, never a
// substitute course. Construction closes all model/material/GBI dependencies.
class Resources final {
public:
    Resources(const rom::Snapshot&, std::int32_t course_id);
    Resources(rom::Snapshot&&, std::int32_t) = delete;
    Resources(const Resources&) = delete;
    Resources& operator=(const Resources&) = delete;
    Resources(Resources&&) = delete;
    Resources& operator=(Resources&&) = delete;
    const beach::WorldMetadata& setup() const noexcept { return setup_; }
    const beach::SkyMetadata& sky() const noexcept { return sky_; }
    std::span<const BlockResource> blocks() const noexcept { return blocks_; }
    std::size_t primary_count() const noexcept { return primary_count_; }
    std::int32_t course_id() const noexcept { return layout_->id; }
    bool primary_loop() const noexcept { return primary_loop_; }
    // Appended after the stable block/road/static catalog; absent skies use no_index.
    std::size_t sky_model_index() const noexcept { return sky_model_index_; }
    std::span<const menu::Model> models() const noexcept { return models_; }
    std::span<const menu::Material> materials() const noexcept { return materials_; }
    std::span<const menu::Animation> animations() const noexcept { return animations_; }
    std::span<const menu::AnimationPath> paths() const noexcept { return paths_; }
    const HeightMap& height_map() const noexcept { return height_map_; }
    const HeightMap& ceiling_map() const noexcept { return ceiling_map_; }
    std::span<const CollisionModel> collision_models() const noexcept { return collision_models_; }
    std::span<const StaticDefinition> static_definitions() const noexcept { return static_definitions_; }
    std::span<const rom::GuestView> installation_spans() const noexcept { return closure_->installation_spans(); }
    std::span<const menu::DisplayList> display_lists() const noexcept { return closure_->display_lists(); }
    std::span<const menu::VertexLoad> vertex_loads() const noexcept { return closure_->vertex_loads(); }
    std::span<const menu::ImageLoad> image_loads() const noexcept { return closure_->image_loads(); }
    std::span<const menu::DynamicMaterialCall> dynamic_material_calls() const noexcept { return closure_->dynamic_material_calls(); }
    // Reservations are caller-owned runtime policy; no Beach diagnostic FIFO
    // ranges are implicitly imposed on another course's immutable graphics.
    void install(std::span<std::byte>, std::span<const beach::RamReservation> = {}) const;
    void require_live() const;
    void require_complete() const { require_live(); closure_->require_complete(); }
private:
    const rom::Snapshot* snapshot_;
    const std::byte* source_data_;
    const course::Layout* layout_;
    beach::WorldMetadata setup_{};
    beach::SkyMetadata sky_{};
    std::vector<BlockResource> blocks_;
    std::size_t primary_count_ = 0;
    bool primary_loop_ = false;
    std::size_t sky_model_index_ = no_index;
    std::vector<menu::Model> models_;
    std::vector<menu::Material> materials_;
    std::vector<menu::Animation> animations_;
    std::vector<menu::AnimationPath> paths_;
    HeightMap height_map_, ceiling_map_;
    std::vector<CollisionModel> collision_models_;
    std::vector<StaticDefinition> static_definitions_;
    std::optional<model_assets::Closure> closure_;
    friend struct ResourceBuilder;
};
Vec3 path_position(const menu::AnimationPath&, float time);
Vec3 path_velocity(const menu::AnimationPath&, float time);
} // namespace isle::world
#endif
