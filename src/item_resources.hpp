#ifndef ISLE_ITEM_RESOURCES_HPP
#define ISLE_ITEM_RESOURCES_HPP

#include "model_asset_closure.hpp"
#include <optional>

namespace isle::items {
enum class Family { pester_ball, apple, water_splash, lava_splash };
struct Definition {
    Family family;
    std::size_t model;
    // Own om.c1429 initializes GObj fnAnimCallback=NULL; items.c never sets it.
    // Acquire Runtime instances with callbacks=false. Program validation
    // metadata is not the per-object callback installation policy.
    // Source om defaults give model/MObj speed1. Items change only texture
    // speed for ball/apple, but both clocks for the two splashes. Consumers
    // must apply this texture setter after acquiring the model instance.
    float texture_speed;
    // Source assignment overwrites the DObj root scale; it is not an extra
    // multiplicative parent transform or an asset-tree normalization.
    menu::Vec3 root_scale;
};
// Complete four-family items.c asset closure, not an item owner/FSM. Native
// descriptors and all payload views borrow the retained, unmoved US Snapshot.
// No second animation clock, source object, renderer or guest callback exists.
class Resources final {
public:
    Resources(const rom::Snapshot&, std::array<std::uint8_t,3> fog_rgb,
              std::uint16_t fog_min, std::uint16_t fog_max);
    Resources(rom::Snapshot&&, std::array<std::uint8_t,3>,std::uint16_t,std::uint16_t)=delete;
    Resources(const Resources&)=delete;
    Resources& operator=(const Resources&)=delete;
    Resources(Resources&&)=delete;
    Resources& operator=(Resources&&)=delete;
    std::span<const Definition> definitions() const;
    const Definition& definition(Family) const;
    std::span<const menu::Model> models() const;
    std::span<const menu::Material> materials() const;
    std::span<const menu::Animation> animations() const;
    std::span<const menu::DisplayList> display_lists() const;
    std::span<const menu::VertexLoad> vertex_loads() const;
    std::span<const menu::ImageLoad> image_loads() const;
    std::span<const menu::DynamicMaterialCall> dynamic_material_calls() const;
    std::span<const rom::GuestView> installation_spans() const;
    void require_live() const;
private:
    const rom::Snapshot* snapshot_;
    const std::byte* source_data_;
    std::array<Definition,4> definitions_;
    std::vector<menu::Model> models_;
    std::vector<menu::Material> materials_;
    std::vector<menu::Animation> animations_;
    std::optional<model_assets::Closure> closure_;
};
} // namespace isle::items
#endif
