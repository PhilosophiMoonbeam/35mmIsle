#ifndef ISLE_ACTOR_OWNER_HPP
#define ISLE_ACTOR_OWNER_HPP
#include "actor_producer.hpp"
#include "actor_owner_source.h"
#include "actor_course_context.hpp"
#include "environment_sound.hpp"
#include "menu_effects.hpp"
#include "world_runtime.hpp"
#include "player_source.h"
#include <array>
#include <cstdint>
#include "beach_ending_source.h"


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
    std::size_t definition = actor::actor_no_index;
    world::Block* block = nullptr;
    world::Block* reference = nullptr;
    const world::Spawn* spawn = nullptr;
    // Source automatic definitions retain their real function/line origin;
    // definition still names the initialized resource family, never a fake wire.
    std::size_t automatic_definition = actor::actor_no_index;
};
struct SpawnResult {
    OwnerStatus status;
    menu::animation::InstanceHandle instance;
    IsleGameObject* object = nullptr;
    explicit operator bool() const noexcept { return bool(status); }
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
    const actor::ActorAnimationHeader*, float start, bool force,
    const actor::ActorInitialization&, const actor::Resources&, EventServices) noexcept;
// Sole source coordinator, not a second animator/scheduler/renderer. Services,
// World, immutable resources, borrowed typed course state, Environment and Detector outlive this owner.
// ALL mutation/tick/cleanup/collection/destruction requires the caller's real
// serialized native+command and GPU/preparation/readback/postprocess exclusion.
// In graphics scenes the actual CanonicalExecutor must be quiescent BEFORE the
// next native tick, not merely before collection; this class creates no fence.
// A Producer borrowing render_bindings() must retire before owner destruction.
class Owner final {
    friend struct SourceAccess;
public:
    Owner(scene::Services&, const actor::Resources&, CourseContext, world::World&,
          environment_sound::Environment&, focus::Detector&, IslePlayerSource&,
          std::span<const menu::animation::SlotCapacity>,
          std::span<const menu::EffectBank* const>,
          menu::animation::ExecutionCapacity = {});
    ~Owner();
    Owner(const Owner&) = delete;
    Owner& operator=(const Owner&) = delete;
    // Preparation creates no native FX helpers. Each original allocation is a
    // separate one-shot publication; real joined callbacks own handle retirement.
    // Renderer must adopt the corresponding canonical field before C stores it.
    OwnerStatus publish_particle_camera() noexcept;
    OwnerStatus publish_effect_emitters() noexcept;
    // Existing all-at-once consumers retain the same two-object operation.
    OwnerStatus publish_effects() noexcept;
    // Original Beach17 plus Pidgey/Scyther alternates and all five Rainbow
    // factories dispatch through compiled C; ordinary/ground/DL4 own placement.
    // Native admission retains exact setup/header tuples, including 1005's NULL
    // header and its real, paused priority3 animation process.
    SpawnResult spawn(const SpawnRequest&) noexcept;
    // Sole compiled factory/init/model/render/setup/header/state admission.
    const ActorInitialization* admitted_factory(std::uint32_t actor_id,
        std::uint32_t factory_identity) const noexcept;
    InteractionServices* bound_interactions() const noexcept;
    // Original503C10 registry configuration, before actor publication. Clears
    // the100 actual registry entries, not the source getFreePokemonId cursor.
    OwnerStatus configure_registry(std::int32_t minimum, std::int32_t maximum,
                                   std::int32_t link, std::int32_t arg3) noexcept;
    // Shared source getFreePokemonId cursor/search, including unrelated native
    // objects on the configured link and occupied inclusive-max fallback.
    // Allocation failures consume the cursor; reset never resets it.
    // Source AddAtGeo uses the decoded singleton definition, current World block
    // and canonical inherited path; its C caller retains behavior/position stores.
    SpawnResult add_one(std::size_t definition, world::Block*, world::Block* reference,
                        const world::Spawn&) noexcept;
    // Standalone Beach CBFF4 writes only transforms. Its complete129 behavior
    // closure never consumes copied behavior/path; do not invent their values.
    SpawnResult add_one_magikarp_unwritten(std::size_t definition, world::Block*,
        world::Block* reference, IsleGameVec3 translation, IsleGameVec3 euler,
        IsleGameVec3 scale) noexcept;
    // Five exact Source helpers: three automatic function/line origins and two
    // initialized singletons. Only id/transforms (mask0x3ff) are Source-written.
    SpawnResult add_one_rainbow(const IsleRainbowDefinition&,
        const IsleRainbowAutomaticSpawn&, world::Block*, world::Block* reference) noexcept;
    OwnerStatus block_members(const world::Block&,
        std::span<menu::animation::InstanceHandle>, std::size_t& written) const noexcept;
    OwnerStatus set_animation(menu::animation::InstanceHandle, const actor::ActorAnimationHeader*, float start = 0, bool force = false) noexcept;
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
    OwnerStatus change_block_ground(menu::animation::InstanceHandle, menu::Vec3 previous, menu::Vec3 current) noexcept;
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
    // Complete returning binding only, once before spawn. Ending scalars stay
    // in this owner; late services/context outlive all their actual callbacks.
    OwnerStatus bind_ending(IsleBeachEndingServices, IsleBeachMusicSource&, const rom::Snapshot&) noexcept;
    // Outside OM/commands/graphics after reset and the provider's independent
    // native/cart/camera joins, before dependent collection/destruction.
    OwnerStatus release_ending() noexcept;
    // Canonical original ending state; owner-phase borrow, no state snapshot.
    const IsleBeachEndingSource& ending_source() const noexcept;
    // Canonical nonowning association; Source outlives all native/graphics borrows.
    IslePlayerSource& player_source() noexcept;
    const IslePlayerSource& player_source() const noexcept;
    const CourseContext& course_context() const noexcept;
private:
    SpawnResult spawn_native(const SpawnRequest&) noexcept;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace isle::actor
#endif
