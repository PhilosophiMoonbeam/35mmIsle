#ifndef ISLE_WORLD_RUNTIME_HPP
#define ISLE_WORLD_RUNTIME_HPP

#include "world_resources.hpp"
#include "world_module.hpp"
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
    menu::animation::InstanceHandle instance, road_instance;
    bool control_points = false; // Source cpObjects slice: 0<numControlLines<4.
};
struct Callbacks {
    void* user = nullptr;
    // Source add/delete/change are nullable. Their userdata survives retirement.
    // False returns normally from the callback and stops dependent World work.
    // Original C callbacks must use their own nested C failure fence.
    bool (*add)(void*, Block&, Block& reference) noexcept = nullptr;
    bool (*remove)(void*, Block&) noexcept = nullptr;
    bool (*change)(void*, Block&, Block& reference) noexcept = nullptr;
    // Real bindCameraNextBlock / fx_translatePosAll consumers are REQUIRED.
    bool (*shift_camera)(void*, Vec3 delta) noexcept = nullptr;
    bool (*shift_effects)(void*, Vec3 delta) noexcept = nullptr;
    // Required by the explicit native-game constructor; production uses the
    // concrete Services constructor and its existing source command queue.
    bool (*process_commands)(void*, IsleGameObject*) noexcept = nullptr;
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
    // Optional source sCeilingMap, independent of the ground height map.
    const HeightMap* ceiling_map = nullptr;
    std::size_t sky_model_index = no_index;
};
struct RenderObject {
    IsleGameObject* object = nullptr;
    IsleGameModel* model = nullptr;
    std::size_t resource_model = no_index;
    std::uint32_t display_list = 0, callback_identity = 0;
    std::uint8_t update_link = 9, render_link = 3;
    std::uint32_t priority = 0x80000000;
    bool sky = false;
    menu::animation::InstanceHandle instance;
};
// Stable borrowed owner view. All pointers/spans and callback userdata outlive
// the producer; availability invalidates preparation before native/RAM mutation.
struct RenderView {
    const ResourceView* resources = nullptr;
    const menu::animation::Runtime* animation = nullptr;
    std::span<const RenderObject> objects;
    const void* owner = nullptr;
    bool (*available)(const void*) noexcept = nullptr;
    bool valid() const noexcept {
        return resources && animation && owner && available && available(owner);
    }
};
struct MovementSample { Vec3 position{}, rotation{}; float speed = 0; };
MovementSample sample_control_points(menu::animation::ModelState, float control_time,
    bool reversed, std::int32_t lines, std::span<const float> timestamps);
struct GroundResult { float height = -9000; Vec3 normal{0, 1, 0}; std::int32_t surface = -1; };
// Application owns game, immutable resources, module state, callback userdata and
// camera. They outlive World and every prepared renderer borrow. Logical
// retirement may be requested from a callback; physical retire() requires the
// owner's idle native, graphics-completed and semantic-consumer-released boundary.
class World final {
public:
    // {} retains resource-certified finite-only admission. Cyclic owners must
    // supply their per-call physical execution budget; Runtime preflights every
    // real step/seek and rejects overflow before animation state publication.
    World(scene::Services&, const Resources&, ModuleState&, Callbacks,
          menu::animation::ExecutionCapacity = {});
    // Cold preparation for original lifecycle setup ordering: admit/preallocate
    // resources and stable block links without publishing native World records.
    // With defer_publication=true, the owner must call publish() at createWorld.
    // Cold ownership never writes the module; publish resets only road events
    // before native creation and applies setup speed after block initialization.
    World(scene::Services&, const Resources&, ModuleState&, Callbacks, bool defer_publication,
          menu::animation::ExecutionCapacity = {});
    // One native publication attempt, outside traversal. Failure retains any
    // partial publication for explicit retire(); it cannot be retried/replayed.
    void publish();
    bool published() const noexcept;
    World(IsleGame*, ResourceView, ModuleState&, Callbacks, menu::animation::ExecutionCapacity = {});
    ~World() noexcept;
    World(const World&) = delete;
    World& operator=(const World&) = delete;
    std::span<Block> blocks() noexcept;
    std::span<const Block> blocks() const noexcept;
    Block* current() const noexcept;
    // Callback failure throws only after the callback has returned normally.
    // A null block means absent source index/next block, or logical retirement
    // requested by a completed callback; retired()/error() distinguish the latter.
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
    menu::animation::ModelState animation_model(menu::animation::InstanceHandle) const;
    const menu::animation::Runtime& animation_runtime() const;
    const ResourceView& resources() const noexcept;
    RenderView render_view() const noexcept;
    bool ground_at(float x, float z, GroundResult&) const;
    // GAME ground.c89-133: global absence defaults the result; local absence
    // leaves it untouched. A present map uses the same height-tree solver.
    bool ceiling_at_global(float x, float z, GroundResult&) const;
    bool ceiling_at(float x, float z, GroundResult&) const;
    std::int32_t collide(Vec3 position, Vec3 velocity, Vec3& out_position, Vec3& out_velocity) const;
    void clear_block(Block&);
    // Returning logical destroyWorld operation, legal during native traversal.
    // Road events reset once at logical destroy entry, before removal callbacks;
    // timer, speed and maximum retain their application-module lifetime.
    // Immediately makes retired() true, hides/pauses native World objects and
    // pauses their actual processes, then invokes original block removals in
    // order. No native model/object/process, animation or callback storage is
    // released here. Existing borrows must close before physical retire().
    // False preserves progress/storage; retry resumes the first incomplete
    // removal. A reentrant request returns false while its outer call finishes.
    // Completed removals never repeat and errors remain latched after cleanup.
    bool request_retire() noexcept;
    // Outside native traversal AND every World callback, finalize real native
    // deletion/process joins, then animation storage. Without a prior request,
    // preserves ordinary per-block remove/delete ordering and retry behavior.
    // This does not establish graphics/semantic-borrow fences: the owner must
    // finish those and retire dependent actors while userdata/blocks stay alive.
    // False preserves unjoined storage; destruction terminates if it persists.
    bool retire() noexcept;
    // Logical unavailability, including a pending explicit retirement request.
    // Movement's existing post-callback guards stop before another road step.
    bool retired() const noexcept;
    // Only this reports completed physical collection; retired() is not a join.
    bool retirement_complete() const noexcept;
    const std::string& error() const noexcept;
private:
    struct Impl;
    World(IsleGame*, scene::Services*, ResourceView, ModuleState&, Callbacks, bool defer_publication,
          menu::animation::ExecutionCapacity);
    std::unique_ptr<Impl> impl_;
};
} // namespace isle::world
#endif
