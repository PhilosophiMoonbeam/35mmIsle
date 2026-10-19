#ifndef ISLE_PHOTO_SUBJECTS_HPP
#define ISLE_PHOTO_SUBJECTS_HPP

#include "borrowed_model_producer.hpp"
#include "model_asset_closure.hpp"
#include "player_photo_source.h"
#include "scene_render.hpp"
#include "world_resources.hpp"
#include <optional>

namespace isle::photo::subjects {
// Exact app_render table identities; values are data selectors, never executable
// guest callbacks. Table views borrow the admitted Snapshot.
struct Variant {
    rom::GuestView wire;
    float pose = 0;
    std::uint32_t model_animations = 0, material_animations = 0;
};
struct AnimationSet {
    rom::GuestView wire;
    std::int32_t id = 0;
    std::vector<Variant> variants;
};
struct PayloadIdentities {
    std::int32_t id = 0;
    // Each view is precisely the pointer word consumed at variant*12, not the
    // unused char[8] or a guessed source array extent.
    std::vector<rom::GuestView> variants;
};
struct Definition {
    rom::GuestView wire;
    std::int32_t id = 0;
    float scale = 0;
    std::uint32_t tree = 0, materials = 0, callback = 0;
    std::uint32_t model_animations = 0, material_animations = 0;
};
enum class Kind { subject, scenery };
struct Instance {
    Kind kind = Kind::subject;
    std::int32_t id = 0;
    std::size_t model = menu::no_index, record_index = menu::no_index;
    std::uint32_t callback = 0, dl_priority = 0x80000000u;
    float time = 0, scale = 0, yaw = 0;
    IsleGameVec3 position{};
    bool half_scale = false, hide_first_child = false;
};
struct VertexPatch {
    std::uint32_t guest = 0;
    // Canonical big endian vertex bytes from the original copy/interpolator.
    // Immutable after construction; installation_spans includes these owned
    // bytes in place of the overlapping original vertex spans.
    std::vector<std::byte> bytes;
};
// Cold resource/reconstruction plan for one canonical record. It decodes every
// normal/special/render/scenery lookup row but hydrates only the selected course,
// recorded variants and source-selected current/previous/next scenery blocks.
// The Snapshot and World resources outlive this nonmovable owner. Shared cold
// decoding admits exact source-consumed Texture image/palette domains.
class Resources final {
public:
    Resources(const rom::Snapshot&, const world::Resources&, const IslePlayerPhotoData&);
    Resources(const Resources&) = delete;
    Resources& operator=(const Resources&) = delete;
    Resources(Resources&&) = delete;
    Resources& operator=(Resources&&) = delete;
    void require_live() const;
    // Exclusive canonical-RAM installation; complete external reservations are
    // checked before any ordinary closure or egg-vertex byte is written.
    void install(std::span<std::uint32_t>, std::span<const menu::RamRange>) const;
    std::span<const Definition> subjects() const noexcept { return subjects_; }
    std::span<const Definition> scenery() const noexcept { return scenery_; }
    std::span<const AnimationSet> animation_sets() const noexcept { return animation_sets_; }
    std::span<const PayloadIdentities> payload_identities() const noexcept { return identities_; }
    std::span<const Instance> instances() const noexcept { return instances_; }
    std::span<const menu::Model> models() const noexcept { return models_; }
    std::span<const menu::Material> materials() const noexcept { return materials_; }
    std::span<const menu::Animation> animations() const noexcept { return animations_; }
    std::span<const menu::AnimationPath> paths() const noexcept { return paths_; }
    std::span<const menu::animation::SlotCapacity> capacities() const noexcept { return capacities_; }
    std::span<const rom::GuestView> installation_spans() const noexcept { return installation_; }
    std::span<const VertexPatch> vertex_patches() const noexcept { return patches_; }
    std::span<const menu::DisplayList> display_lists() const noexcept { return closure_->display_lists(); }
    std::span<const menu::VertexLoad> vertex_loads() const noexcept { return closure_->vertex_loads(); }
    std::span<const menu::ImageLoad> image_loads() const noexcept { return closure_->image_loads(); }
    std::span<const menu::DynamicMaterialCall> dynamic_material_calls() const noexcept { return closure_->dynamic_material_calls(); }
    menu::model::PacketResources packet_resources() const {
        require_live(); return {models_,materials_,installation_spans()};
    }
    const IslePlayerPhotoData& record() const noexcept { return record_; }
private:
    const rom::Snapshot* snapshot_;
    const std::byte* source_data_;
    const world::Resources* world_;
    IslePlayerPhotoData record_;
    std::vector<Definition> subjects_, scenery_;
    std::vector<AnimationSet> animation_sets_;
    std::array<PayloadIdentities,3> identities_;
    std::vector<Instance> instances_;
    std::vector<menu::Model> models_;
    std::vector<menu::Material> materials_;
    std::vector<menu::Animation> animations_;
    std::vector<menu::AnimationPath> paths_;
    std::vector<menu::animation::SlotCapacity> capacities_;
    std::vector<std::vector<std::uint8_t>> visibility_;
    std::vector<VertexPatch> patches_;
    std::vector<rom::GuestView> installation_;
    std::optional<model_assets::Closure> closure_;
};
struct LinkOwnership {
    std::uint8_t object_link = 0, dl_link = 0;
    std::uint32_t camera_tag = 0;
};
struct Object {
    menu::model::BorrowedBinding binding;
    scene::RenderMetadata render;
    std::size_t instance = menu::no_index, record_index = menu::no_index;
};
struct ScoredSlot {
    IsleGameObject* object = nullptr;
    std::uint8_t record_index = 0;
};
// No Game/Storage, callbacks, AI or OM/GTL dispatch is created here. Construct
// cold, then publish/clear only in the actual admitted Source OM context on the
// caller's active Game. Clear requires genuine GPU/readback/preparation release;
// the destructor does not delete native objects or manufacture a fence.
class Reconstruction final {
public:
    Reconstruction(IsleGame*, const Resources&, LinkOwnership);
    ~Reconstruction();
    Reconstruction(const Reconstruction&) = delete;
    Reconstruction& operator=(const Reconstruction&) = delete;
    IsleGameResult publish() noexcept;
    IsleGameResult clear() noexcept;
    // Reuse the cold record plan only after every native/runtime join.
    bool rearm() noexcept;
    // OK accepts logical clear; cleared(), not the return code, confirms every
    // actual native post-retire observer and Runtime release. Pending joins
    // retain all storage. Failure is sticky but cleanup remains available.
    bool clearing() const noexcept;
    bool cleared() const noexcept;
    bool failed() const noexcept;
    IsleGameResult error() const noexcept;
    bool active() const noexcept;
    std::span<const Object> objects() const noexcept;
    std::span<const menu::model::PacketBinding> bindings() const noexcept;
    std::span<const ScoredSlot> scored_slots() const noexcept;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
// Source's repeated binary32 subtraction by100, not fmod.
float frozen_time(float recorded_time);
} // namespace isle::photo::subjects
#endif
