#ifndef ISLE_PLAYER_CART_HPP
#define ISLE_PLAYER_CART_HPP
#include "player_cart_source.h"
#include "player_cart_render_binding.hpp"

namespace isle::player::cart {
enum class OwnerError { none, capacity, admission, native, animation, services, lifetime };
struct OwnerStatus {
    OwnerError error = OwnerError::none;
    const char* detail = "";
    explicit operator bool() const noexcept { return error == OwnerError::none; }
};
struct InitResult {
    OwnerStatus status;
    IsleGameObject* object = nullptr;
    menu::animation::InstanceHandle instance;
    explicit operator bool() const noexcept { return bool(status) && object; }
};
struct Membership {
    bool live = false, retiring = false, joined = false, prepared = false;
    std::size_t caller_holds = 0;
};
// All dependencies, Source, borrowed views and caller records outlive this owner.
// prepare/refill and destruction occur outside OM. The caller provides physical
// capacity including held retired carts, not a guessed logical cart count.
class Owner final {
    friend struct SourceAccess;
public:
    // A real preparation/GPU/readback/effect exclusion borrow. Keep it through
    // actual completion; dropping it is not itself a GPU fence. Mutation rejects
    // while borrowed, and requested retirement defers native deletion until end.
    class Borrow final {
    public:
        Borrow() noexcept = default;
        ~Borrow();
        Borrow(Borrow&&) noexcept;
        Borrow& operator=(Borrow&&) noexcept;
        Borrow(const Borrow&) = delete;
        Borrow& operator=(const Borrow&) = delete;
        explicit operator bool() const noexcept { return owner_ != nullptr; }
    private:
        friend class Owner;
        explicit Borrow(Owner*) noexcept;
        Owner* owner_ = nullptr;
    };
    // Adopt this hold BEFORE publishing an actual caller record naming a cart;
    // release only after clearing/replacing that record. Retired identities can
    // remain named, but never authorize live model/Runtime access.
    class Reference final {
    public:
        Reference() noexcept = default;
        ~Reference();
        Reference(Reference&&) noexcept;
        Reference& operator=(Reference&&) noexcept;
        Reference(const Reference&) = delete;
        Reference& operator=(const Reference&) = delete;
        IsleGameObject* object() const noexcept { return object_; }
        explicit operator bool() const noexcept { return owner_ != nullptr; }
    private:
        friend class Owner;
        Reference(Owner*, IsleGameObject*) noexcept;
        Owner* owner_ = nullptr;
        IsleGameObject* object_ = nullptr;
    };
    Owner(scene::Services&, const CartResources&, IslePlayerSource&,
          std::span<const menu::animation::SlotCapacity>, menu::animation::ExecutionCapacity = {});
    ~Owner();
    Owner(const Owner&) = delete;
    Owner& operator=(const Owner&) = delete;
    IslePlayerCartSource* source() noexcept;
    const IslePlayerCartSource* source() const noexcept;
    OwnerStatus refill_prepared() noexcept;
    InitResult init() noexcept;
    OwnerStatus set_animation(const IslePlayerCartModelAnimation*, const IslePlayerCartMaterialAnimation*, float start, float speed) noexcept;
    OwnerStatus change_animation(const IslePlayerCartModelAnimation*, const IslePlayerCartMaterialAnimation*, float start) noexcept;
    OwnerStatus set_animation_speed(float) noexcept;
    OwnerStatus set_callback(IsleGameObject*, IslePlayerCartCallback, void*) noexcept;
    const IslePlayerCartModelAnimation* intro_model() const noexcept;
    const IslePlayerCartMaterialAnimation* intro_material() const noexcept;
    const IslePlayerCartModelAnimation* outro_model() const noexcept;
    const IslePlayerCartMaterialAnimation* outro_material() const noexcept;
    IsleGameObject* object(menu::animation::InstanceHandle) const noexcept;
    IsleGameModel* native_model(menu::animation::InstanceHandle) const noexcept;
    menu::animation::InstanceHandle instance(IsleGameObject*) const noexcept;
    Membership membership(menu::animation::InstanceHandle) const noexcept;
    const menu::animation::Runtime& animation() const noexcept;
    // Scratch covers the complete actual Services DL3 link, including unrelated
    // World objects. No allocation/sort or guessed global link capacity.
    std::span<const RenderBinding> render_bindings(std::span<scene::RenderOrderEntry>) noexcept;
    Borrow borrow() noexcept;
    Reference retain(IsleGameObject*) noexcept;
    // Individual retirement deliberately does NOT clear source()->cartGObj.
    OwnerStatus retire(IsleGameObject*) noexcept;
    OwnerStatus collect_joined() noexcept;
    // Explicit overlay/Owner teardown: closes admission, retires every cart and
    // releases current only after real borrowers end. Caller References remain.
    OwnerStatus retire() noexcept;
private:
    void end_borrow() noexcept;
    void release_reference(IsleGameObject*) noexcept;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace isle::player::cart
#endif
