#ifndef ISLE_ACTOR_OWNER_HPP
#define ISLE_ACTOR_OWNER_HPP
#include "actor_producer.hpp"
#include "actor_owner_source.h"
#include "environment_sound.hpp"
#include "menu_effects.hpp"
#include "world_runtime.hpp"

namespace isle::actor {
class InteractionServices;
enum class OwnerError { none, admission, capacity, native, animation, services, lifetime };
struct OwnerStatus {
    OwnerError error = OwnerError::none;
    const char* detail = "";
    explicit operator bool() const noexcept { return error == OwnerError::none; }
};
struct SpawnRequest {
    std::uint32_t object_id = 0;
    std::size_t definition = beach::actor_no_index;
    world::Block* block = nullptr;
    world::Block* reference = nullptr;
    const world::Spawn* spawn = nullptr;
};
struct SpawnResult {
    OwnerStatus status;
    menu::animation::InstanceHandle instance;
    IsleGameObject* object = nullptr;
    explicit operator bool() const noexcept { return bool(status); }
};
struct PlayerState {
    IsleGameObject* object = nullptr;
    IsleGameModel* model = nullptr;
    IsleGameCamera* camera = nullptr;
    menu::animation::InstanceHandle previous_focus;
    IsleGameObject* previous_focus_identity = nullptr;
    bool focus_latch = false;
    std::int32_t direction_index = 0;
    std::uint32_t focus_timer = 0;
};
// Source pause gates Update's counter only. Execution is checked at the ordinary
// command-handler tail; neither predicate transition resumes native processes.
// Actual FIFO commands -2/-3 own object-wide suspension/resumption.
struct CourseState {
    bool paused = false, execution_enabled = true, sound_enabled = true;
    // Actual Beach D_beach_802CC014. Scene chooser publishes true after its
    // successful spawn; the genuine Magikarp state clears it after animation.
    bool magikarp_active = false;
};
struct Membership {
    bool logical = false, registered = false, retiring = false, joined = false;
    std::size_t transform_cache_retained = 0, pokemon_cache_retained = 0;
    // Canonical record observation remains readable while retiring, until joined
    // collection. It is not a second advanced state/clock.
    std::uint32_t process_flags = 0;
    std::int32_t loop_count = 0;
};
// Shared source callback policy. This also admits a standalone actual
// animation/native model context without claiming that model's FSM is native.
// The sole guarded source record supplies current animHeader and tangible.
struct EventServices {
    scene::Services& services;
    environment_sound::Environment& sound;
    menu::EffectRuntime& effects;
    menu::animation::Runtime& animation;
    IsleGameModel* model = nullptr;
    bool sound_enabled = true;
    IsleGameObject* particle_owner = nullptr;
    IsleGameObject* effect_owner = nullptr;
};
OwnerStatus dispatch_event(IsleActorSource&, const menu::animation::CallbackEvent&, EventServices) noexcept;
OwnerStatus step_source_animation(IsleActorSource&, menu::animation::InstanceHandle, EventServices) noexcept;
OwnerStatus apply_source_animation(IsleActorSource&, menu::animation::InstanceHandle,
    const beach::ActorAnimationHeader*, float start, bool force,
    const beach::ActorInitialization&, const beach::ActorResources&, EventServices) noexcept;
// Sole source coordinator, not a second animator/scheduler/renderer. Services,
// World, immutable resources, Environment and Detector outlive this owner.
// ALL mutation/tick/cleanup/collection/destruction requires the caller's real
// serialized native+command and GPU/preparation/readback/postprocess exclusion.
// In graphics scenes the actual CanonicalExecutor must be quiescent BEFORE the
// next native tick, not merely before collection; this class creates no fence.
// A Producer borrowing render_bindings() must retire before owner destruction.
class Owner final {
    friend struct SourceAccess;
public:
    Owner(scene::Services&, const beach::ActorResources&, world::World&,
          environment_sound::Environment&, focus::Detector&,
          std::span<const menu::animation::SlotCapacity>,
          std::span<const menu::EffectBank* const>,
          menu::animation::ExecutionCapacity = {});
    ~Owner();
    Owner(const Owner&) = delete;
    Owner& operator=(const Owner&) = delete;
    SpawnResult spawn(const SpawnRequest&) noexcept;
    // Shared source getFreePokemonId cursor/search, including unrelated link3
    // objects and occupied inclusive-max fallback. Allocation failures consume
    // the cursor; reset does not reset it. No alternate scene allocator.
    SpawnResult add_one(std::size_t definition, world::Block*, world::Block* reference,
                        const world::Spawn&) noexcept;
    OwnerStatus set_animation(menu::animation::InstanceHandle, const beach::ActorAnimationHeader*, float start = 0, bool force = false) noexcept;
    OwnerStatus step_animation(menu::animation::InstanceHandle) noexcept;
    // Complete source logical-order position/flag80 + pair/command26 manager.
    // Caller owns the sole priority2 scene boundary and real graphics exclusion.
    OwnerStatus update_collisions() noexcept;
    // Success distinguishes normal source blocking from invalid/native failure.
    // On failure blocked is unchanged; earlier real currGround effects may remain.
    OwnerStatus step_walk(menu::animation::InstanceHandle, float velocity_x, float velocity_z,
                          std::uint32_t flags, bool& blocked) noexcept;
    OwnerStatus set_local(menu::animation::InstanceHandle, std::size_t node, const menu::animation::Transform&) noexcept;
    OwnerStatus set_placement(menu::animation::InstanceHandle, const IsleGameModelTransform&) noexcept;
    OwnerStatus trigger_common1003(menu::animation::InstanceHandle, menu::Vec3 position) noexcept;
    OwnerStatus cleanup(menu::animation::InstanceHandle) noexcept;
    // Source Pokemon_SendDieCommand: actual logical list only, matching target
    // identity. A registered Item sender needs no Actor payload/model.
    OwnerStatus notify_target_death(IsleGameObject* sender) noexcept;
    OwnerStatus request_cleanup(menu::animation::InstanceHandle) noexcept;
    OwnerStatus change_block(menu::animation::InstanceHandle, menu::Vec3 previous, menu::Vec3 current) noexcept;
    OwnerStatus remove_block(world::Block&) noexcept;
    OwnerStatus reset() noexcept;
    // Outside all OM/command traversal, after actual joins and external graphics
    // releases. Services retains native identities AND semantic slots through
    // all successful FIFO/in-flight and actual canonical apple/target borrows.
    // Stops at first held retiring slot, preserving earlier collection progress.
    OwnerStatus collect_joined() noexcept;
    IsleActorSource* source(menu::animation::InstanceHandle) noexcept;
    const IsleActorSource* source(menu::animation::InstanceHandle) const noexcept;
    IsleGameObject* object(menu::animation::InstanceHandle) const noexcept;
    IsleGameModel* native_model(menu::animation::InstanceHandle) const noexcept;
    bool registered(menu::animation::InstanceHandle) const noexcept;
    Membership membership(menu::animation::InstanceHandle) const noexcept;
    // Fixed-address physical storage, refreshed in source DL/priority/insertion
    // order. Entries carry true slot indices; never cache entry pointers across
    // a subsequent mutation/reordering. Span remains valid until destruction.
    std::span<const ActorRenderBinding> render_bindings() noexcept;
    menu::animation::Runtime& animation() noexcept;
    const menu::animation::Runtime& animation() const noexcept;
    menu::EffectRuntime& effects() noexcept;
    const environment_sound::Environment& environment() const noexcept;
    IsleGameObject* particle_object() const noexcept;
    IsleGameObject* effect_object() const noexcept;
    OwnerStatus update_particles() noexcept;
    OwnerStatus update_effects() noexcept;
    // Bind before spawning. The context stays alive through all source work,
    // actual joins and graphics release; successful reset ends this borrow.
    OwnerStatus bind_interactions(InteractionServices&) noexcept;
    OwnerStatus set_player(IsleGameObject*, IsleGameModel*, IsleGameCamera*) noexcept;
    OwnerStatus set_focus(menu::animation::InstanceHandle, bool latch, std::uint32_t timer) noexcept;
    void set_direction(std::int32_t) noexcept;
    const PlayerState& player() const noexcept;
    CourseState& course() noexcept;
    const CourseState& course() const noexcept;
private:
    SpawnResult spawn_native(const SpawnRequest&) noexcept;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace isle::actor
#endif
