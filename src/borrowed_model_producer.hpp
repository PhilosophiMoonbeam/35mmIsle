#ifndef ISLE_BORROWED_MODEL_PRODUCER_HPP
#define ISLE_BORROWED_MODEL_PRODUCER_HPP

#include "borrowed_model_kernel.hpp"
#include "scene_render.hpp"

namespace isle::menu::model {
struct PacketResources {
    std::span<const Model> models;
    std::span<const Material> materials;
    std::span<const rom::GuestView> installation_spans;
};
struct PacketBinding {
    BorrowedBinding model;
    scene::RenderMetadata render;
};
struct PacketLayout {
    RamRange payloads, lists, materials;
    std::array<RamRange,2> framebuffers;
};
// Raw complete source-callback command ranges; no EndDL inside these ranges.
// Hidden and pass-filtered active objects retain metadata and empty lanes.
// Composition borrows these ranges rather than re-encoding the models.
struct PacketObject {
    IsleGameObject* object = nullptr;
    scene::RenderMetadata render;
    std::array<ListRange,4> lanes{};
    // Absolute byte addresses of owned NOP tails, outside raw command counts.
    std::array<std::uint32_t,4> continuation_slots{};
};
struct PacketFrame {
    ListRange camera;
    std::array<ListRange,4> lanes;
    std::span<const RamRange> reservations;
    std::uint64_t revision = 0;
    std::span<const PacketObject> objects;
};
// Resource-agnostic source packet adapter over the sole BorrowedKernel. Admits
// ordinary B and the exact GAME B/D/I/J fogged and I no-fog callbacks
// (4CDB0.c131-163), not arbitrary callback identities. Model.render_type, never
// an owner family enum, selects hierarchy and exact opaque/transparent wrappers.
//
// Caller owns the one actual Game, camera, immutable resources, Runtime/native
// poses, basis, bindings and real GPU/readback lease. All must remain stable
// through completion; this adapter neither creates owners nor waits for them.
// Empty slots have null object/model/runtime and an empty instance. Every active
// binding is checked, even hidden or pass-filtered. Repeated resources and
// arbitrary physical slots are legal. No per-preparation allocation occurs.
// Returned object ranges expire at the next preparation or retirement.
class PacketProducer final {
public:
    // Complete disjoint scene/task/output union, excluding cold image/BSS,
    // framebuffers and these three owned ranges. Every range is checked against
    // all immutable installation spans, not just currently drawn resources.
    PacketProducer(IsleGame*, IsleGameCamera*, CameraBasisState&, PacketResources,
                   std::span<const animation::SlotCapacity>,
                   std::span<const PacketBinding>, PacketLayout,
                   std::span<const RamRange> other_reservations);
    ~PacketProducer();
    PacketProducer(const PacketProducer&) = delete;
    PacketProducer& operator=(const PacketProducer&) = delete;
    PacketFrame prepare(std::span<std::uint32_t>, std::uint64_t exclusive_epoch);
    PacketFrame prepare(std::span<std::uint32_t>, std::uint64_t, const scene::render::Pass&);
    static bool prepare_pass(void*, std::span<std::uint32_t>, std::uint64_t,
                             const scene::render::Pass&, scene::render::Prepared&);
    std::span<const RamRange> reservations() const noexcept;
    std::span<const WorldMatrix> world_nodes(animation::InstanceHandle) const noexcept;
    std::span<const IsleGameModelNode> native_nodes(animation::InstanceHandle) const noexcept;
    // Exclusive owner only, after native joins and genuine GPU/readback release.
    // Does not retire Runtime/native storage or manufacture a completion fence.
    void retire() noexcept;
private:
    struct Impl;
    PacketFrame prepare_filtered(std::span<std::uint32_t>, std::uint64_t,
                                 const scene::render::Pass*);
    std::unique_ptr<Impl> impl_;
};
} // namespace isle::menu::model
#endif
