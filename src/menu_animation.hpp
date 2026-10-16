#ifndef ISLE_MENU_ANIMATION_HPP
#define ISLE_MENU_ANIMATION_HPP

#include "menu_resources.hpp"

namespace isle::menu::animation {
enum class Error { none, malformed, unsupported, extent, allocation, retired, handle, occupied, capacity, exhausted };
struct Status {
    Error error = Error::none;
    std::uint32_t address = 0;
    explicit operator bool() const noexcept { return error == Error::none; }
};
enum class Phase { disabled, changed, running, finished };
struct InstanceHandle {
    std::uint64_t pool = 0, incarnation = 0;
    std::uint32_t slot = UINT32_MAX;
    explicit operator bool() const noexcept { return pool != 0 && incarnation != 0 && slot != UINT32_MAX; }
    bool operator==(const InstanceHandle&) const = default;
};
struct SlotCapacity { std::size_t nodes = 0, material_attachments = 0; };
// Total decoded instructions and callback events per step_model (including its
// materials), or per step_camera. Fixed at construction; never silently capped.
// {} selects resource-certified finite-only bounds and rejects cyclic input.
// Cyclic owners must specify their actual scheduling/skip envelope explicitly;
// a call exceeding either bound rejects before any state/event publication.
// Both fields must be defaulted, or both explicit. Explicit zero is valid.
struct ExecutionCapacity { std::size_t commands = no_index, events = no_index; };
SlotCapacity slot_capacity(const Model&) noexcept;
struct AcquireResult {
    Status status;
    InstanceHandle handle;
    explicit operator bool() const noexcept { return static_cast<bool>(status); }
};
struct Clock {
    Phase phase = Phase::disabled;
    float time_left = 0, time_passed = 0, speed = 0.5f;
};
struct Transform { Vec3 position{}, rotation{}, scale{1, 1, 1}; };
enum class ModelBindPose { preserve, restore };
struct NodeState {
    Transform transform;
    std::size_t parent = no_index; // Local model index; parents precede children.
    std::size_t first_material = 0, material_count = 0;
    std::uint32_t flags = 0;
    std::uint64_t revision = 1; // Local transform/flags; descendants still compose with parent world.
    bool callback_receiver = false;
    Clock clock;
};
struct MaterialState {
    std::size_t resource = no_index; // Catalog material index; state is per attachment.
    std::uint16_t image_index = 0, next_image_index = 0; // GAME stores truncated u16.
    // Animation-owned pre-render values. For Texture.flags&0x10 the renderer
    // derives effective u16 indices from trunc(s32(lod_level)) and that+1 at
    // each actual render (sys/render.c:989-993). Animation never reads those
    // render-side assignments back into its independent AObj tracks.
    float palette_index = 0, lod_level = 0;
    std::array<float, 9> parameters{}; // Same 14,18,1c,20,24,28,3c,40,44 order as Material.
    std::array<std::uint32_t, 10> color_and_trailing_words{}; // 50..74, including 54.
    std::uint32_t word_4c = 0;
    std::uint64_t revision = 1;
    Clock clock;
};
struct CameraState {
    Vec3 eye{}, at{};
    float roll = 0, fov_y = 0;
    // Source kind 8 uses fixed Y-up, NOT roll-derived up. Apply through the
    // existing camera setter: view_dirty changes eye/at/roll only; projection_dirty
    // changes FOV only. Preserve near=47, far/aspect, viewport, clips, and kind 8.
    bool view_dirty = true, projection_dirty = true;
    std::uint16_t written_channels = 0;
    Clock clock;
};
struct CallbackEvent {
    InstanceHandle instance;
    std::size_t node = 0;
    std::int32_t event_id = 0;
    float argument = 0;
    std::uint32_t command_address = 0;
    std::uint64_t source_order = 0;
};
struct ModelState {
    std::span<const NodeState> nodes;
    std::span<const MaterialState> materials;
    float animation_time = 0;
    std::uint32_t object_flags = 0; // Bit 2 advances tracks but suppresses model writes.
    InstanceHandle instance;
    std::size_t resource_model = no_index;
    Status status{Error::handle, 0};
    explicit operator bool() const noexcept { return static_cast<bool>(status); }
};
// Borrowed immutable input, also usable by synthetic behavior tests. Every span,
// nested vector and wire owner must outlive Runtime and all outstanding views.
// Production owners supply typed Snapshot-backed catalog views; this interface
// borrows them and does not confer admission on reconstructed private tables.
struct Resources {
    std::span<const Model> models;
    std::span<const Material> materials;
    std::span<const Animation> animations;
    std::size_t camera_animation = no_index;
    std::span<const AnimationPath> paths{};
};
// GAME 1F960.c uniform arc-length parameterization and native spline math.
Vec3 path_position(const AnimationPath&, float parameter);
Vec3 path_velocity(const AnimationPath&, float parameter);
struct CreateResult;
// Pure animator, not a native lifecycle coordinator. Destruction/whole-runtime
// retirement require the owner to join native callbacks and end every renderer,
// completed-memory and effect borrow first; neither operation manufactures a fence.
class Runtime final {
public:
    ~Runtime();
    Runtime(const Runtime&) = delete;
    Runtime& operator=(const Runtime&) = delete;
    Runtime(Runtime&&) = delete;
    Runtime& operator=(Runtime&&) = delete;
    // Construction compiles immutable programs and fixes every storage address.
    // Slots start free; capacity plans are not implicit resource assignments.
    static CreateResult create(const Catalog&, std::span<const SlotCapacity>, CameraState initial_camera,
                               ExecutionCapacity) noexcept;
    static CreateResult create(Resources, std::span<const SlotCapacity>, CameraState initial_camera,
                               ExecutionCapacity) noexcept;
    AcquireResult acquire(std::size_t slot, std::size_t resource_model, bool callback_enabled) noexcept;
    Status begin_retire(InstanceHandle) noexcept;
    // Owner-only precondition: actual native work is joined, graphics/completed
    // memory borrows ended and attached effects detached. Runtime cannot prove
    // these external facts; call only from the real fenced retirement coordinator.
    Status release_retired(InstanceHandle) noexcept;
    Status set_callback_enabled(InstanceHandle, bool) noexcept;
    // Call in the actual OM object/process order, not catalog order by default.
    // Each model call visits all nodes depth-first, model then attached materials.
    // First call after acquire/reset evaluates time 0 (CHANGED); subsequent
    // calls advance the instance's signed source speed. END applies once.
    // SET_ANIMATION14 follows a validated same-stream boundary and resets source
    // clocks only, retaining tracks/fields/speeds/receiver. Receiver callbacks
    // report -2 on model loops; material loops never emit model callbacks.
    Status step_model(InstanceHandle) noexcept;
    Status step_camera(std::uint32_t object_flags = 0) noexcept;
    // Storage addresses stay stable until destruction; active spans are borrowed
    // only until mutation/retirement and cannot authorize stale-handle access.
    // Dispatch every event in order before another mutation; bind its actual node
    // world transform (including native root parent), never a guest pointer.
    ModelState model(InstanceHandle) const noexcept;
    const CameraState& camera() const noexcept;
    std::span<const CallbackEvent> events() const noexcept;
    std::size_t slot_count() const noexcept;
    std::uint64_t generation() const noexcept;
    Status set_object_flags(InstanceHandle, std::uint32_t flags) noexcept;
    // Source animSet semantics: clear track kinds but retain allocated track
    // values/rates and current fields/flags; skip is source skipFrames.
    Status restart_model(InstanceHandle, float skip_frames = 0) noexcept;
    // Program indices refer to the immutable Resources::animations pool. Model
    // bindings have exactly one entry per DFS node. Non-null entries clear track
    // kinds only (retaining values/rates), restart at skip, and select the first
    // animated node as END receiver. no_index disables a node without clearing
    // its tracks, fields, speed or time_passed. restore also restores every node's
    // Catalog bind transform, as GAME anim_func_8000F8E4 with its initial tree.
    // Neither bind evaluates: the actor sets speeds, then calls step_model once
    // immediately and routes its borrowed events before any further mutation.
    // Header/array identity, force, pause and actor loop bookkeeping belong to actor.
    Status bind_model(InstanceHandle, std::span<const std::size_t> programs,
                      float skip_frames = 0, ModelBindPose pose = ModelBindPose::preserve) noexcept;
    // Flattened node/material attachment order. Empty span is a null outer
    // array; no_index is a null entry: both preserve existing bindings/tracks/
    // clocks/fields. Non-null entries use kind-only restart at skip. Like GAME's
    // texture tree setter, even an all-null array sets object animation_time.
    Status bind_materials(InstanceHandle, std::span<const std::size_t> programs,
                          float skip_frames = 0) noexcept;
    // Pokemon_SetTextures primitive: empty span restores every original attached
    // Catalog material; otherwise exactly one resource index per attachment,
    // no_index leaving that attachment untouched. Clears tracks/program, restores
    // texture fields and image indices, disables clock at time0/speed1, but keeps
    // palette_index (GAME Pokemon_SetTexture does not assign it). No evaluation.
    // All binding/reset arguments validate before any state/event publication.
    Status reset_materials(InstanceHandle, std::span<const std::size_t> resources = {}) noexcept;
    // Source speed setters permit signed deltas when movement seeks the road.
    Status set_model_speed(InstanceHandle, float speed) noexcept;
    Status set_texture_speed(InstanceHandle, float speed) noexcept;
    Status restart_camera(float skip_frames = 0) noexcept;
    // Fresh active scene reentry restores resource bind/material fields, zero/new
    // AObjs, receiver/camera/clocks. Keeps handles and per-instance callback policy.
    Status reset(float skip_frames = 0) noexcept;
    // Owner-only whole-runtime retirement, under the same external fences.
    void retire() noexcept;
private:
    struct Impl;
    explicit Runtime(std::unique_ptr<Impl>) noexcept;
    std::unique_ptr<Impl> impl_;
};
struct CreateResult {
    Status status;
    std::unique_ptr<Runtime> runtime;
    explicit operator bool() const noexcept { return static_cast<bool>(status); }
};
} // namespace isle::menu::animation
#endif
