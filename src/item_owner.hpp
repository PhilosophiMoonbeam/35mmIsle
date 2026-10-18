#ifndef ISLE_ITEM_OWNER_HPP
#define ISLE_ITEM_OWNER_HPP
#include "item_owner_source.h"
#include "borrowed_model_producer.hpp"
#include "item_resources.hpp"
#include "item_flute.hpp"
#include "actor_owner.hpp"
#include "beach_magikarp_scene.hpp"
#include <optional>

namespace isle::items {
enum class OwnerError { none, admission, capacity, native, animation, services, lifetime };
struct OwnerStatus {
    OwnerError error = OwnerError::none;
    const char* detail = "";
    explicit operator bool() const noexcept { return error == OwnerError::none; }
};
struct SpawnResult {
    OwnerStatus status;
    menu::animation::InstanceHandle instance;
    IsleGameObject* object = nullptr;
    explicit operator bool() const noexcept { return bool(status); }
};
struct PayloadStorage;
struct Globals {
    IsleItemGlobals source{};
    FluteState flute{};
    beach::MagikarpScene* beach_scene = nullptr; // Borrowed installed-hook lifetime.
    Globals();
    ~Globals();
    Globals(const Globals&) = delete;
    Globals& operator=(const Globals&) = delete;
    // Canonical cache records retain stable addresses across Owner wrappers.
    // Storage is provisioned before execution, never allocated by source ticks.
    std::unique_ptr<PayloadStorage> payload_storage;
};
struct Membership {
    bool logical = false, retiring = false, joined = false;
    std::size_t payload_cache_retained = 0;
};
// Every dependency and caller Globals/Source outlives this owner. ALL
// mutation, retirement and destruction requires real serialized native/command
// and graphics/preparation/readback exclusion. Native joins are not GPU fences.
// Physical capacities include held retired Items AND independent fume objects;
// neither logical20 nor source payload-cache20 is a physical capacity bound.
class Owner final {
    friend struct SourceAccess;
public:
    // Preparation only: canonical Source association precedes original Root
    // publication. Explicit init() remains the original late Items_Init stage.
    Owner(scene::Services&, const Resources&, world::World&, actor::Owner&,
          Globals&, IslePlayerSource&, std::span<const menu::animation::SlotCapacity>,
          menu::animation::ExecutionCapacity = {});
    ~Owner();
    Owner(const Owner&) = delete;
    Owner& operator=(const Owner&) = delete;
    OwnerStatus init() noexcept;
    // Explicit factories retain their original supplied origin/extra velocity;
    // original Item initialization reads the current Source.cam_target.
    SpawnResult spawn_ball(IsleGameVec3 position, IsleGameVec3 extra_velocity) noexcept;
    SpawnResult spawn_apple(IsleGameVec3 position, IsleGameVec3 extra_velocity) noexcept;
    OwnerStatus delete_item(IsleGameObject*) noexcept;
    bool exists(IsleGameObject*) const noexcept;
    void init_iterator() noexcept;
    IsleGameObject* next_valid() noexcept;
    IsleGameObject* next_still() noexcept;
    // Read immediately after a query: NULL with success is normal exhaustion;
    // a failed native canonical binding is an error, never an empty-item claim.
    OwnerStatus query_status() const noexcept;
    bool matches(scene::Services&, world::World&, actor::Owner&) const noexcept;
    IsleItemSource* source(menu::animation::InstanceHandle) noexcept;
    const IsleItemSource* source(menu::animation::InstanceHandle) const noexcept;
    IsleItemSource* source(IsleGameObject*) noexcept;
    IsleGameObject* object(menu::animation::InstanceHandle) const noexcept;
    IsleGameModel* native_model(menu::animation::InstanceHandle) const noexcept;
    menu::animation::InstanceHandle instance(IsleGameObject*) const noexcept;
    Membership membership(menu::animation::InstanceHandle) const noexcept;
    // Canonical live slot identity, including independent water/lava splashes.
    std::optional<Family> family(menu::animation::InstanceHandle) const noexcept;
    std::span<const menu::model::PacketBinding> render_bindings() noexcept;
    menu::animation::Runtime& animation() noexcept;
    const menu::animation::Runtime& animation() const noexcept;
    OwnerStatus set_custom_functions(IsleGameSourceCallback, std::uint8_t kind, IsleItemCollisionCallback) noexcept;
    // The binding Game must be this Owner's Game. Both-null clears, including
    // failed-scene cleanup; no half-null/contextless provider is accepted.
    // Serialize replacement with Item execution. Retain context through active
    // calls and process joins, then clear before context retirement. A new Owner
    // requires a fresh binding, never a prior wrapper's opaque context.
    OwnerStatus set_extra_collision(IsleGame* binding_game,
        IsleItemCollisionProvider, void* context) noexcept;
    // Installs the real decoded/shared-RNG chooser; NULL update reinstates
    // default movement. Passing NULL collision to the generic setter preserves
    // the prior hook, exactly as source. Scene object outlives installed hook.
    OwnerStatus set_beach_collision(beach::MagikarpScene&) noexcept;
    OwnerStatus pause() noexcept;
    OwnerStatus resume() noexcept;
    OwnerStatus remove_flying() noexcept;
    OwnerStatus displace(IsleGameVec3) noexcept;
    // Actual source Pokemon_EatApple / KickItem consumers; no extra physics.
    OwnerStatus eat(IsleGameObject*) noexcept;
    OwnerStatus kick(IsleGameObject* actor, float speed) noexcept;
    Flute flute() noexcept;
    // A failed scene disposes only retained logical/cache ownership, never live
    // Source callbacks or new processes. Completed disposal still reports the
    // terminal Services error; collection depends on actual joins/borrow release.
    // The caller supplies graphics exclusion for both phases and destruction.
    OwnerStatus retire() noexcept;
    OwnerStatus collect_joined() noexcept;
private:
    SpawnResult spawn(Family, IsleGameVec3, IsleGameVec3) noexcept;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace isle::items
#endif
