#ifndef ISLE_ACTOR_RESOURCES_HPP
#define ISLE_ACTOR_RESOURCES_HPP

#include "model_asset_closure.hpp"
#include <optional>

namespace isle::actor {
inline constexpr std::size_t actor_no_index = menu::no_index;
// The caller supplies its actual World setup, never Title's default fog.
struct FogProfile {
    std::array<std::uint8_t, 3> rgb;
    std::uint16_t min, max;
};
struct ActorSetup {
    rom::GuestView wire;
    std::size_t initial_header = actor_no_index;
    std::uint32_t initial_state_identity = 0, unknown_0c_identity = 0,
                  transition_graph_identity = 0;
    std::int8_t process_kind = 0;
};
struct ActorInitialization {
    rom::GuestView wire;
    std::uint32_t actor_id = 0, render_callback_identity = 0;
    std::size_t model = actor_no_index;
    menu::Vec3 scale{}, collision_center{};
    float radius = 0;
    std::uint16_t flags = 0;
    std::array<std::uint8_t, 3> configured_matrix_kinds{};
    ActorSetup setup;
    bool animations_enabled = false;
    // Actual initialization textures outer pointer, not inferred attachments.
    bool textures_present = false;
};
struct ActorDefinition {
    rom::GuestView wire;
    std::uint32_t actor_id = 0, factory_identity = 0,
                  change_block_identity = 0, remove_identity = 0;
    std::size_t initialization = actor_no_index;
    bool singleton = false;
};
// An automatic C initializer has a Source origin, not an initialized guest
// address. These facts describe values only and grant no factory admission.
struct SourceDefinitionOrigin {
    std::uint32_t function_identity;
    std::uint32_t line;
};
struct AutomaticActorDefinition {
    SourceDefinitionOrigin origin;
    std::uint32_t actor_id, factory_identity, change_block_identity, remove_identity;
    std::size_t initialization;
};
struct ActorNodeBinding {
    std::size_t model_animation = actor_no_index;
    // A NULL per-node pointer preserves existing MObj animation; it is not a
    // zero-length terminated list. Non-NULL lists have exactly attached count.
    rom::GuestView material_animation_table;
    std::vector<std::size_t> material_animations;
};
struct ActorAnimationHeader {
    rom::GuestView wire, model_animation_table, material_animation_table, sound_id_table;
    std::size_t initialization = actor_no_index;
    float speed = 0, unknown_04 = 0; // Source field remains unqualified.
    std::vector<ActorNodeBinding> nodes;
    // Precomputed once in source DFS/attachment order. Model slots include
    // NULL entries; material slots include per-node/entry NULL preservation.
    // Only an outer NULL material table omits the flattened material bindings.
    std::vector<std::size_t> model_bindings, material_bindings;
    // Only root callbacks0..3 with positive arguments bound this prefix. A
    // zero argument stops a category without dereferencing ids; children are FX.
    std::vector<std::int32_t> sound_ids;
    // Entire-header NULL materials requests source texture reset (palette
    // retained) when applied; per-node NULL and NULL entries instead preserve.
    bool resets_materials() const noexcept { return material_animation_table.bytes.empty(); }
};
namespace detail { struct CourseCatalog; }
// Immutable descriptors borrow the exact admitted, unmoved Snapshot. No guest
// callback executes and no factory or ActorAI readiness is established. Only
// closed course 0/6 catalogs are selected; graphics obligations must all close.
class Resources final {
public:
    Resources(const rom::Snapshot&, std::int32_t course_id, FogProfile);
    Resources(rom::Snapshot&&, std::int32_t, FogProfile) = delete;
    Resources(const Resources&) = delete;
    Resources& operator=(const Resources&) = delete;
    Resources(Resources&&) = delete;
    Resources& operator=(Resources&&) = delete;
    std::int32_t course_id() const noexcept;
    std::span<const menu::Model> models() const;
    std::span<const menu::Material> materials() const;
    std::span<const menu::Animation> animations() const;
    // Retained once during decoding; immutable path indices/storage are shared
    // by every animator borrowing this owner, never copied for a frame.
    std::span<const menu::AnimationPath> paths() const;
    std::span<const ActorAnimationHeader> headers() const;
    std::span<const ActorInitialization> initializations() const;
    std::span<const ActorDefinition> definitions() const;
    std::span<const AutomaticActorDefinition> automatic_definitions() const;
    // Exact initialized identities only, with references stable for this owner.
    const ActorAnimationHeader& header(std::uint32_t identity) const;
    const ActorInitialization& initialization(std::uint32_t identity) const;
    const ActorDefinition& definition(std::uint32_t identity) const;
    // Each model node's bitset includes initial flags0 and every opcode15
    // choice from all headers, rather than an arbitrary independent hide policy.
    std::span<const std::uint8_t> visibility_domains(std::size_t model) const;
    std::span<const menu::DisplayList> display_lists() const;
    std::span<const menu::VertexLoad> vertex_loads() const;
    std::span<const menu::ImageLoad> image_loads() const;
    std::span<const menu::DynamicMaterialCall> dynamic_material_calls() const;
    std::span<const rom::GuestView> installation_spans() const;
    // Production callers supply the complete chosen scene/task/output RAM
    // reservation union. Empty means isolated asset-byte installation only.
    void install(std::span<std::byte>, std::span<const menu::RamRange> = {}) const;
    void require_live() const;
private:
    const rom::Snapshot* snapshot_;
    const std::byte* source_data_;
    const detail::CourseCatalog* catalog_;
    std::vector<menu::Model> models_;
    std::vector<menu::Material> materials_;
    std::vector<menu::Animation> animations_;
    std::vector<menu::AnimationPath> paths_;
    std::vector<ActorAnimationHeader> headers_;
    std::vector<ActorInitialization> initializations_;
    std::vector<ActorDefinition> definitions_;
    std::vector<std::vector<std::uint8_t>> visibility_;
    std::optional<model_assets::Closure> closure_;
    friend struct ActorResourceBuilder;
};
} // namespace isle::actor
#endif
