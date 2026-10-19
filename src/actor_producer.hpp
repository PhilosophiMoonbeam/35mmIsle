#ifndef ISLE_ACTOR_PRODUCER_HPP
#define ISLE_ACTOR_PRODUCER_HPP

#include "actor_resources.hpp"
#include "source_vertex_publication.hpp"
#include "borrowed_model_kernel.hpp"
#include "scene_render.hpp"
#include "focus_capture.hpp"
#include "35mmisle/focus_detector.hpp"

namespace isle::actor {
// No implicit Title/World RAM addresses. Framebuffers are complete physical
// reservations (including an unused alternate), not producer-owned uploads.
struct ProducerLayout {
    menu::RamRange payloads, lists, materials;
    std::array<menu::RamRange,2> framebuffers;
};
// Actual source detector state, not an optional capture callback. Null at
// Producer creation selects geometry-only composition without a detector owner.
// A bound owner always emits the source DL5 prefix; enabled controls per-actor
// captures only. Context starts at source1 and flips when the prefix renders.
struct DetectorBinding {
    focus::RegionCopies& copies;
    focus::Detector& history;
    focus::Photo photo{}; // Explicit membership when prepare_membership is null.
    focus::Scissor scissor{0,0,320,240};
    std::uint16_t context = 1;
    bool enabled = false;
    void* prefix_user = nullptr;
    IsleGameResult (*prepare_membership)(void*, focus::Photo&) noexcept = nullptr;
};
// Active objects carry the actual bound IsleActorSource payload. Rendering reads
// its authoritative Pokemon flags; no renderer-side flag100 replica is retained.
struct ActorRenderBinding {
    menu::model::BorrowedBinding model;
    scene::RenderMetadata render;
};
enum class FragmentKind { object, detector_prefix };
// Complete raw source callback ranges, including fog and GPU region copies,
// without lane EndDL. Hidden/filtered active objects have empty ranges.
// detector_prefix has no native object: it represents the existing source
// policy {5,0x80000001u,UINT32_MAX}, not a fabricated native registration.
struct ProducerFragment {
    FragmentKind kind = FragmentKind::object;
    IsleGameObject* object = nullptr;
    scene::RenderMetadata render;
    std::array<menu::model::ListRange,4> lanes{};
    // Absolute byte addresses of owned NOP tails, outside raw 64-bit counts.
    std::array<std::uint32_t,4> continuation_slots{};
};
struct ProducerFrame {
    menu::model::ListRange camera;
    std::array<menu::model::ListRange,4> lanes;
    std::span<const menu::RamRange> reservations;
    std::uint64_t revision = 0;
    // Borrowed through preparation/GPU consumption until the next successful
    // preparation or retirement. Rejection preserves published fragment records.
    std::span<const ProducerFragment> fragments;
};
// Beach main-camera graphics policy: GAME player.c2936-2975 kinds3/14,
// pokemon.c2101-2150 DL5/default or DL4/wrapper, pokemon_detect.c247-280
// render gates and app_render/4CDB0.c15-49,131-163 fog/hierarchy wrappers.
// A detector binding stages real GPU-copy commands and CPU history metadata.
// The caller installs static copies after RAM reset, completes actual GPU and
// readback before analyze, and analyzes before shared rows can be overwritten.
// Membership is collected at the included source prefix when a hook is bound;
// otherwise the explicit supplied photo remains supported. The hook may mutate
// canonical visibility flags before a later rejection; those source side effects
// are NOT rolled back. Photo/history/context/RAM publish only after success.
// Original full snapshot serialization/queue is outside this graphics policy.
// Native cleanup owns identity invalidation; this adapter owns no GPU fence.
//
// All resource/native/Runtime/basis owners and the ordered binding span remain
// stable through actual GPU completion/readback. An empty binding has null
// object/model/runtime and an empty instance; every other binding is validated,
// even hidden/filtered/geometryless. Slot assignments and resources are dynamic:
// capacities are NOT resource indices, and repeated resource models are legal.
// Change bindings, poses or native lifetimes only under exclusive native and
// actual GPU-quiescent owner control. Never mutate between preflight and upload.
// The optional hook/user and all native membership owners must remain stable
// through preparation. A hook must not mutate bindings, lifetimes or poses.
// Preprefix gates/subjects retain their old flags; postprefix gates read the
// collector's canonical flags. Filtered-out prefixes never invoke the hook.
struct SourceRender {
    void* user = nullptr;
    IsleGameResult (*invoke)(void*, IsleGameObject*, std::uint32_t) noexcept = nullptr;
};
class Producer final {
public:
    // other_reservations is the COMPLETE disjoint scene/task/output union,
    // excluding cold image/BSS, framebuffers and these three producer ranges.
    // It is validated against every actual actor asset, not just own uploads.
    Producer(IsleGame*, IsleGameCamera*, menu::model::CameraBasisState&,
             const actor::Resources&, std::span<const menu::animation::SlotCapacity>,
             std::span<const ActorRenderBinding> ordered_bindings, ProducerLayout,
             std::span<const menu::RamRange> other_reservations, DetectorBinding*, SourceRender = {},
             std::span<const source_geometry::VertexPublication> source_vertices = {});
    ~Producer();
    Producer(const Producer&) = delete;
    Producer& operator=(const Producer&) = delete;
    ProducerFrame prepare(std::span<std::uint32_t>, std::uint64_t exclusive_epoch);
    // Source composition projects the actual prefix object's native visibility
    // immediately before preparation. Hidden prefixes never collect/stage.
    ProducerFrame prepare(std::span<std::uint32_t>, std::uint64_t, const scene::render::Pass&,
                          bool prefix_visible = true);
    static bool prepare_pass(void*, std::span<std::uint32_t>, std::uint64_t,
                             const scene::render::Pass&, scene::render::Prepared&);
    std::span<const menu::RamRange> reservations() const noexcept;
    // Mathematical local RTS x parent/placement only; NOT source54 FX sampling.
    std::span<const menu::model::WorldMatrix> world_nodes(menu::animation::InstanceHandle) const noexcept;
    std::span<const IsleGameModelNode> native_nodes(menu::animation::InstanceHandle) const noexcept;
    // Only the currently executing, C-fenced custom Source callback may append.
    // These packets share the real bounded lane arrays and publication transaction.
    IsleGameResult source_packet(std::uint32_t lane, std::uint32_t first, std::uint32_t second) noexcept;
    IsleGameResult source_hierarchy(IsleGameObject*, menu::RenderType) noexcept;
    // Owner-only after real native joins and GPU/readback release. No fake fence,
    // native deletion or Runtime retirement is manufactured by this adapter.
    void retire() noexcept;
private:
    struct Impl;
    ProducerFrame prepare_filtered(std::span<std::uint32_t>, std::uint64_t, const scene::render::Pass*, bool);
    std::unique_ptr<Impl> impl_;
};
} // namespace isle::actor
#endif
