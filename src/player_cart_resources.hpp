#ifndef ISLE_PLAYER_CART_RESOURCES_HPP
#define ISLE_PLAYER_CART_RESOURCES_HPP

#include "menu_animation.hpp"
#include "model_asset_closure.hpp"
#include <optional>

namespace isle::player {
struct CartFogProfile {
    std::array<std::uint8_t,3> rgb;
    std::uint16_t min, max;
};
struct CartNodeBinding {
    std::size_t model_animation = menu::no_index;
    rom::GuestView material_animation_table;
    std::vector<std::size_t> material_animations;
};
struct CartClip {
    rom::GuestView model_animation_table, material_animation_table;
    std::vector<CartNodeBinding> nodes;
    // Source DFS order, including every NULL model/attachment preservation slot.
    std::vector<std::size_t> model_bindings, material_bindings;
    std::size_t camera_animation = menu::no_index;
    std::uint32_t camera_identity = 0;
};
// DATA prerequisite only. Snapshot must outlive all descriptors, Runtime/Driver
// borrows and installation. No cart object/process/lifecycle is created here.
class CartResources final {
public:
    CartResources(const rom::Snapshot&, std::int32_t courseID, CartFogProfile);
    CartResources(rom::Snapshot&&, std::int32_t courseID, CartFogProfile) = delete;
    CartResources(const CartResources&) = delete;
    CartResources& operator=(const CartResources&) = delete;
    CartResources(CartResources&&) = delete;
    CartResources& operator=(CartResources&&) = delete;
    std::int32_t course_id() const noexcept { return course_id_; }
    menu::animation::Resources animation_resources() const;
    const CartClip& intro() const;
    const CartClip& outro() const;
    std::span<const std::uint8_t> visibility_domains() const;
    std::span<const menu::DisplayList> display_lists() const;
    std::span<const menu::VertexLoad> vertex_loads() const;
    std::span<const menu::ImageLoad> image_loads() const;
    std::span<const menu::DynamicMaterialCall> dynamic_material_calls() const;
    std::span<const rom::GuestView> installation_spans() const;
    // The production owner supplies its complete actual scene/task/output
    // reservation union. An empty union establishes isolated asset bytes only.
    void install(std::span<std::byte>, std::span<const menu::RamRange>) const;
    void require_live() const;
private:
    std::int32_t course_id_;
    const rom::Snapshot* snapshot_;
    const std::byte* source_data_;
    std::vector<menu::Model> models_;
    std::vector<menu::Material> materials_;
    std::vector<menu::Animation> animations_;
    std::vector<menu::AnimationPath> paths_;
    std::vector<std::uint8_t> visibility_;
    CartClip intro_, outro_;
    std::optional<model_assets::Closure> closure_;
};
} // namespace isle::player
#endif
