#ifndef ISLE_WORLD_RUNTIME_HPP
#define ISLE_WORLD_RUNTIME_HPP

#include "world_resources.hpp"
#include "menu_animation.hpp"
#include <memory>
#include <string>

namespace isle::scene { class Services; }
namespace isle::world {
class World;
struct Block {
    std::size_t index = 0;
    const BlockResource* resource = nullptr;
    Block* previous = nullptr;
    Block* next = nullptr;
    IsleGameObject* object = nullptr;
    IsleGameModel* model = nullptr;
    IsleGameObject* road_object = nullptr;
    IsleGameModel* road_model = nullptr;
    bool control_points = false; // Source cpObjects slice: 0<numControlLines<4.
};
struct Callbacks {
    void* user = nullptr;
    // Source add/delete/change are nullable. Their userdata survives retirement.
    void (*add)(void*, Block&, Block& reference) noexcept = nullptr;
    void (*remove)(void*, Block&) noexcept = nullptr;
    void (*change)(void*, Block&, Block& reference) noexcept = nullptr;
    // Real bindCameraNextBlock / fx_translatePosAll consumers are REQUIRED.
    void (*shift_camera)(void*, Vec3 delta) noexcept = nullptr;
    void (*shift_effects)(void*, Vec3 delta) noexcept = nullptr;
    // Required by the explicit native-game constructor; production uses the
    // concrete Services constructor and its existing source command queue.
    void (*process_commands)(void*, IsleGameObject*) noexcept = nullptr;
};
struct ResourceView {
    const beach::WorldMetadata* setup = nullptr;
    const beach::SkyMetadata* sky = nullptr;
    std::span<const BlockResource> blocks;
    std::size_t primary_count = 0;
    std::span<const menu::Model> models;
    std::span<const menu::Material> materials;
    std::span<const menu::Animation> animations;
    std::span<const menu::AnimationPath> paths;
    std::span<const StaticDefinition> static_definitions;
    const HeightMap* height_map = nullptr;
    std::span<const CollisionModel> collision_models;
};
struct RenderObject {
    IsleGameObject* object = nullptr;
    IsleGameModel* model = nullptr;
    std::size_t resource_model = no_index;
    std::uint32_t display_list = 0, callback_identity = 0;
    std::uint8_t update_link = 9, render_link = 3;
    std::uint32_t priority = 0x80000000;
    bool sky = false;
};
struct MovementSample { Vec3 position{}, rotation{}; float speed = 0; };
struct GroundResult { float height = -9000; Vec3 normal{0, 1, 0}; std::int32_t surface = -1; };
// Application owns game, immutable resources, callback userdata and camera. They
// outlive World and every prepared renderer borrow. retire() is only legal at
// the owner's outside-traversal, graphics-completed scene boundary.
class World final {
public:
    World(scene::Services&, const Resources&, Callbacks);
    World(IsleGame*, ResourceView, Callbacks);
    ~World();
    World(const World&) = delete;
    World& operator=(const World&) = delete;
    std::span<Block> blocks() noexcept;
    std::span<const Block> blocks() const noexcept;
    Block* current() const noexcept;
    Block* enter_first(std::size_t);
    Block* enter_next();
    MovementSample sample(Block&, float move_time, float control_time);
    float next_speed(const Block&, float control_time) const;
    void reset_road(Block&, float skip_frames = 0);
    void set_visibility(std::uint32_t);
    void follow_camera(IsleGameCamera*);
    void set_sky_position(Vec3, float yaw, float animation_time);
    void set_sky_animation_speed(float);
    float global_time() const noexcept;
    std::span<const RenderObject> render_objects() const noexcept;
    menu::animation::ModelState animation_model(std::size_t) const;
    const ResourceView& resources() const noexcept;
    bool ground_at(float x, float z, GroundResult&) const;
    std::int32_t collide(Vec3 position, Vec3 velocity, Vec3& out_position, Vec3& out_velocity) const;
    void clear_block(Block&);
    void retire() noexcept;
    bool retired() const noexcept;
    const std::string& error() const noexcept;
private:
    struct Impl;
    World(IsleGame*, scene::Services*, ResourceView, Callbacks);
    std::unique_ptr<Impl> impl_;
};
} // namespace isle::world
#endif
