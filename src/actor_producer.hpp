#ifndef ISLE_ACTOR_PRODUCER_HPP
#define ISLE_ACTOR_PRODUCER_HPP

#include "beach_actor_resources.hpp"
#include "borrowed_model_kernel.hpp"
#include "scene_render.hpp"

namespace isle::actor {
// No implicit Title/World RAM addresses. Framebuffers are complete physical
// reservations (including an unused alternate), not producer-owned uploads.
struct ProducerLayout {
    menu::RamRange payloads, lists, materials;
    std::array<menu::RamRange,2> framebuffers;
};
// Only the detector-disabled geometry branch is admitted. There is deliberately
// no enabled mode/no-op capture callback; invalid enum values reject at creation.
enum class DetectorPolicy { disabled };
struct ActorRenderBinding {
    menu::model::BorrowedBinding model;
    scene::RenderMetadata render;
    // Actual renderPokemonModelType* wrapper gate (pokemon_detect.c247-280).
    // The owner supplies Pokemon.flags bit100, not native GObj hidden flags or
    // an inferred cull. Native GObj/Runtime hidden flags are independently read.
    bool source_flag100 = false;
};
struct ProducerFrame {
    menu::model::ListRange camera;
    std::array<menu::model::ListRange,4> lanes;
    std::span<const menu::RamRange> reservations;
    std::uint64_t revision = 0;
};
// Beach main-camera graphics policy: GAME player.c2936-2975 kinds3/14,
// pokemon.c2101-2150 DL5/default or DL4/wrapper, pokemon_detect.c247-280
// render gates and app_render/4CDB0.c15-49,131-163 fog/hierarchy wrappers.
// Detector-region saving is NOT implemented: use only with the detector off.
// This owns no actor factory/FSM, scheduling, effect sampling or GPU fence.
//
// All resource/native/Runtime/basis owners and the ordered binding span remain
// stable through actual GPU completion/readback. An empty binding has null
// object/model/runtime and an empty instance; every other binding is validated,
// even hidden/filtered/geometryless. Slot assignments and resources are dynamic:
// capacities are NOT resource indices, and repeated resource models are legal.
// Change bindings, poses or native lifetimes only under exclusive native and
// actual GPU-quiescent owner control. Never mutate between preflight and upload.
class Producer final {
public:
    // other_reservations is the COMPLETE disjoint scene/task/output union,
    // excluding cold image/BSS, framebuffers and these three producer ranges.
    // It is validated against every actual actor asset, not just own uploads.
    Producer(IsleGame*, IsleGameCamera*, menu::model::CameraBasisState&,
             const beach::ActorResources&, std::span<const menu::animation::SlotCapacity>,
             std::span<const ActorRenderBinding> ordered_bindings, ProducerLayout,
             std::span<const menu::RamRange> other_reservations, DetectorPolicy);
    ~Producer();
    Producer(const Producer&) = delete;
    Producer& operator=(const Producer&) = delete;
    ProducerFrame prepare(std::span<std::uint32_t>, std::uint64_t exclusive_epoch);
    ProducerFrame prepare(std::span<std::uint32_t>, std::uint64_t, const scene::render::Pass&);
    static bool prepare_pass(void*, std::span<std::uint32_t>, std::uint64_t,
                             const scene::render::Pass&, scene::render::Prepared&);
    std::span<const menu::RamRange> reservations() const noexcept;
    // Mathematical local RTS x parent/placement only; NOT source54 FX sampling.
    std::span<const menu::model::WorldMatrix> world_nodes(menu::animation::InstanceHandle) const noexcept;
    std::span<const IsleGameModelNode> native_nodes(menu::animation::InstanceHandle) const noexcept;
    // Owner-only after real native joins and GPU/readback release. No fake fence,
    // native deletion or Runtime retirement is manufactured by this adapter.
    void retire() noexcept;
private:
    struct Impl;
    ProducerFrame prepare_filtered(std::span<std::uint32_t>, std::uint64_t, const scene::render::Pass*);
    std::unique_ptr<Impl> impl_;
};
} // namespace isle::actor
#endif
