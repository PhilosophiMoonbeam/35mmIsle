#ifndef ISLE_MODEL_ASSET_CLOSURE_HPP
#define ISLE_MODEL_ASSET_CLOSURE_HPP

#include "menu_resources.hpp"

namespace isle::model_assets {
struct Mapping {
    std::uint32_t guest_begin;
    std::size_t rom_begin, bytes;
};
// Bounded typed data access only; this reader grants no executable-code access.
// The caller owns admission of the Snapshot and the exact initialized mappings.
class Reader final {
public:
    Reader(const rom::Snapshot&, std::span<const Mapping>);
    Reader(rom::Snapshot&&, std::span<const Mapping>) = delete;
    // Authored-data seam uses precisely the same mapping and packet validation.
    Reader(std::span<const std::byte> canonical, std::span<const Mapping>);
    rom::GuestView view(std::uint32_t, std::size_t, std::size_t alignment = 4) const;
    // Finite initialized-data bound for source-framed sentinel/program readers.
    // Does not extend mapping authority or install the unconsumed suffix.
    std::size_t remaining_bytes(std::uint32_t) const;
private:
    std::span<const std::byte> bytes_;
    std::span<const Mapping> mappings_;
    friend struct Builder;
};
struct ModelPlan {
    const menu::Model* model;
    // Bit i admits DObj flags value i (low two bits). Empty means only flags0.
    // flags1 omits this payload; flags2/3 omit the entire subtree, not siblings.
    std::span<const std::uint8_t> visibility_domains{};
};
struct Limits {
    std::size_t commands = 1048576, evaluations = 4194304;
    std::size_t alternatives = 4096, call_depth = 32, obligations = 65536;
};
enum class Parameter { image, segment14, branch_target };
struct Obligation {
    Parameter parameter;
    std::size_t model_index; // menu::no_index denotes an extra root.
    std::uint32_t lane, command_identity, word0, word1;
    friend bool operator==(const Obligation&, const Obligation&) = default;
};
// Constructor-only relational analysis. Each model lane starts with independent
// symbolic incoming registers: other objects may interleave between lanes.
// Unknown incoming consumers remain explicit obligations, never guessed images.
// No complete installation view is available until every obligation is closed.
class Closure final {
public:
    Closure(const Reader&, std::span<const ModelPlan>, std::span<const menu::Material>,
            std::span<const std::uint32_t> extra_roots = {},
            std::span<const rom::GuestView> extra_installation = {}, Limits = {});
    std::span<const menu::DisplayList> display_lists() const noexcept { return lists_; }
    std::span<const menu::VertexLoad> vertex_loads() const noexcept { return vertices_; }
    std::span<const menu::ImageLoad> image_loads() const noexcept { return images_; }
    std::span<const menu::DynamicMaterialCall> dynamic_material_calls() const noexcept { return dynamic_calls_; }
    std::span<const Obligation> obligations() const noexcept { return obligations_; }
    void require_complete() const;
    std::span<const rom::GuestView> installation_spans() const;
private:
    std::vector<menu::DisplayList> lists_;
    std::vector<menu::VertexLoad> vertices_;
    std::vector<menu::ImageLoad> images_;
    std::vector<menu::DynamicMaterialCall> dynamic_calls_;
    std::vector<rom::GuestView> installation_;
    std::vector<Obligation> obligations_;
    friend struct Builder;
};
} // namespace isle::model_assets
#endif
