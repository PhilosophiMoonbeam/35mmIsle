#ifndef ISLE_ITEM_PRODUCER_HPP
#define ISLE_ITEM_PRODUCER_HPP

#include "item_render_binding.hpp"
#include "scene_render.hpp"

namespace isle::items {
struct ProducerLayout {
    menu::RamRange payloads, lists, materials;
    std::array<menu::RamRange,2> framebuffers;
};
// Raw complete source-callback command ranges; no EndDL inside these ranges.
// Hidden and pass-filtered active objects are present with empty lane ranges.
// Course composition uses these borrowed ranges, never re-encodes the models.
struct ProducerObject {
    IsleGameObject* object = nullptr;
    scene::RenderMetadata render;
    std::array<menu::model::ListRange,4> lanes{};
};
struct ProducerFrame {
    menu::model::ListRange camera;
    std::array<menu::model::ListRange,4> lanes;
    std::span<const menu::RamRange> reservations;
    std::uint64_t revision = 0;
    std::span<const ProducerObject> objects;
};
// Native Item/fume adapter over the sole BorrowedKernel. Own GAME items.c
// 293-315/1134-1186 selects fogged B for ball/apple and fogged D for fumes;
// app_render/4CDB0.c131-143 supplies their opaque/transparent fog wrappers.
// No Actor payload, callback policy, model owner, scheduler or GPU fence exists
// here. Runtime/source/native roots must already be synchronized by the Owner.
//
// Resource/native/Runtime/basis owners and the binding storage remain stable
// through actual GPU completion/readback. Empty slots have null object/model/
// runtime and an empty instance; every other binding is checked even hidden or
// filtered. Repeated resources and arbitrary physical slots are legal. Change
// poses/bindings/lifetimes only after actual graphics/preparation borrowers end.
// Returned object ranges expire at the next preparation or retirement.
class Producer final {
public:
    // Complete disjoint scene/task/output union, excluding cold image/BSS,
    // both framebuffers and this producer's three owned ranges. Every range is
    // validated against every immutable Item asset, not just uploaded assets.
    Producer(IsleGame*, IsleGameCamera*, menu::model::CameraBasisState&,
             const Resources&, std::span<const menu::animation::SlotCapacity>,
             std::span<const RenderBinding>, ProducerLayout,
             std::span<const menu::RamRange> other_reservations);
    ~Producer();
    Producer(const Producer&) = delete;
    Producer& operator=(const Producer&) = delete;
    ProducerFrame prepare(std::span<std::uint32_t>, std::uint64_t exclusive_epoch);
    ProducerFrame prepare(std::span<std::uint32_t>, std::uint64_t, const scene::render::Pass&);
    static bool prepare_pass(void*, std::span<std::uint32_t>, std::uint64_t,
                             const scene::render::Pass&, scene::render::Prepared&);
    std::span<const menu::RamRange> reservations() const noexcept;
    std::span<const menu::model::WorldMatrix> world_nodes(menu::animation::InstanceHandle) const noexcept;
    std::span<const IsleGameModelNode> native_nodes(menu::animation::InstanceHandle) const noexcept;
    // Exclusive owner only after native joins and real GPU/readback release.
    // Does not retire Runtime/native storage or manufacture a completion fence.
    void retire() noexcept;
private:
    struct Impl;
    ProducerFrame prepare_filtered(std::span<std::uint32_t>, std::uint64_t,
                                   const scene::render::Pass*);
    std::unique_ptr<Impl> impl_;
};
} // namespace isle::items
#endif
