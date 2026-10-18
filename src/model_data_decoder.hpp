#ifndef ISLE_MODEL_DATA_DECODER_HPP
#define ISLE_MODEL_DATA_DECODER_HPP

#include "model_asset_closure.hpp"
#include "menu_animation.hpp"

namespace isle::model_assets {
// Bounded indexed image/palette domains and separately admitted trailing NULL
// slots. Source-proven image holes retain their original indices.
struct MaterialExtent {
    std::uint32_t material, table;
    std::size_t images, slots;
    std::uint32_t palette_table = 0;
    std::size_t palettes = 0, palette_slots = 0;
    std::span<const std::size_t> null_image_slots{};
};
// Shared cold decoder for bounded initialized GAME model resources. Reader owns
// mapping authority; callers retain all referenced storage. No installation,
// animation clock, model instance, callback execution or render cache is created.
class DataDecoder final {
public:
    DataDecoder(const Reader&, std::vector<menu::Material>&,
                std::vector<menu::Animation>&, std::span<const MaterialExtent>,
                std::vector<menu::AnimationPath>* paths = nullptr);
    menu::Model model(std::uint32_t tree, std::uint32_t materials,
                      std::size_t nodes, menu::RenderType);
    std::size_t material(std::uint32_t);
    std::size_t material(std::uint32_t, const MaterialExtent&);
    // Derive a universal renderer-consumed image/palette prefix, including
    // initial indices and LOD's adjacent image. Supply every selected texture
    // program for this identity; NULL programs (no_index) are ignored. No table
    // sentinel or mapping-size guess; timing-dependent domains need an explicit capability.
    MaterialExtent describe_material(std::uint32_t,
                                     std::span<const std::size_t> texture_streams = {}) const;
    // Source-owned first-update capability. Every later use of these stream
    // bindings must obey these exact samples (or restart intervals). This does
    // not authorize arbitrary live playback. Explicit MaterialExtent admission
    // and its separately proven trailing NULL slots remain unchanged.
    std::size_t sampled_material(std::uint32_t,
                                 std::span<const menu::animation::TextureSample>,
                                 std::span<const std::size_t> universal_streams = {});
    // Full-lifetime sequential playback proof, not a declared allocation or a
    // first-update sample. Owners must retain/forward these same capabilities
    // into Runtime resources; the material rejects omission during admission.
    std::size_t playback_material(std::uint32_t,
                                  std::span<const menu::animation::TexturePlayback>);
    std::size_t animation(std::uint32_t, menu::AnimationKind,
                          bool model_callback_installed = true);
    void bind_initial_animations(menu::Model&, std::uint32_t models,
                                 std::uint32_t materials, bool model_callback_installed);
    // Declared allocation facts validate independently of playback timing;
    // inferred extents require the matching sampled/universal/playback proof. Actual
    // per-pose image/palette bounds remain mandatory in the packet producer.
    void validate_material_animation(std::size_t stream, std::size_t material) const;
    std::size_t path(std::uint32_t);
private:
    const Reader& reader_;
    std::vector<menu::Material>& materials_;
    std::vector<menu::Animation>& animations_;
    std::span<const MaterialExtent> material_extents_;
    std::vector<menu::AnimationPath>* paths_;
    struct MaterialBinding { std::uint32_t material; std::size_t stream; };
    std::vector<MaterialBinding> sampled_bindings_, playback_bindings_;
};
} // namespace isle::model_assets
#endif
