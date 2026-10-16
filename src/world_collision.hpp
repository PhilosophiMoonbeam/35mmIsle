#ifndef ISLE_WORLD_COLLISION_HPP
#define ISLE_WORLD_COLLISION_HPP
#include "world_resources.hpp"

namespace isle::world::collision {
struct GroundResult {
    float height = -9000.0f;
    Vec3 normal{0, 1, 0};
    std::int32_t surface = -1;
};
const HeightPatch* height_patch_at(const HeightMap&, float x, float z) noexcept;
float patch_height_at(const HeightPatch*, float x, float z) noexcept;
bool ground_at_global(const HeightMap&, float x, float z, GroundResult&) noexcept;
bool ground_at(const HeightMap&, Vec3 block_world_position, float x, float z, GroundResult&) noexcept;
// Inputs/outputs are block-local game units (100 units per map unit).
// Object position is already translated into the current block's MAP units.
// Uses StaticObject_Collide's instance scale rule, root exclusion and first hit.
bool collision_one(const CollisionModel&, const StaticObject&, Vec3 position, Vec3 velocity,
                   Vec3& out_position, Vec3& out_velocity) noexcept;
// StaticObject_CollideOne variant: object position is in game units and the
// model's uniform scale is used unconditionally.
bool collision_model_one(const CollisionModel&, Vec3 object_position, Vec3 rotation,
                         Vec3 position, Vec3 velocity, Vec3& out_position, Vec3& out_velocity) noexcept;
bool collision_primitive(const Collider&, Vec3 position, Vec3 velocity,
                         Vec3& out_position, Vec3& out_velocity) noexcept;
// Borrows immutable resource models for its lifetime; builds collider matrices
// once, matching InitCollisionModels. Collision calls allocate no storage.
class PreparedModels final {
public:
    explicit PreparedModels(std::span<const CollisionModel>);
    bool collision_one(std::size_t model_index, const StaticObject&,
                       Vec3 position, Vec3 velocity,
                       Vec3& out_position, Vec3& out_velocity) const noexcept;
private:
    struct Transform {
        float local_to_global[4][4];
        float global_to_local[4][4];
    };
    struct Model {
        const CollisionModel* source;
        std::size_t first_transform, root;
    };
    std::vector<Model> models_;
    std::vector<Transform> transforms_;
};
}
#endif
