#ifndef ISLE_MODEL_DATA_DECODER_HPP
#define ISLE_MODEL_DATA_DECODER_HPP

#include "model_asset_closure.hpp"

namespace isle::model_assets {
// Exact source-consumed image entries, with any separately admitted NULL slots.
// This is bounded initialized GAME data, not a native Texture or console ABI.
struct MaterialExtent {
    std::uint32_t material, table;
    std::size_t images, slots;
};
// Shared cold decoder for the admitted Actor and Item model families. Reader
// owns mapping authority; callers retain all referenced storage. No installation,
// animation clock, model instance, callback execution or render cache is created.
class DataDecoder final {
public:
    DataDecoder(const Reader&, std::vector<menu::Material>&,
                std::vector<menu::Animation>&, std::span<const MaterialExtent>,
                std::vector<menu::AnimationPath>* paths = nullptr);
    menu::Model model(std::uint32_t tree, std::uint32_t materials,
                      std::size_t nodes, menu::RenderType);
    std::size_t material(std::uint32_t);
    std::size_t animation(std::uint32_t, menu::AnimationKind,
                          bool model_callback_installed = true);
    void bind_initial_animations(menu::Model&, std::uint32_t models,
                                 std::uint32_t materials, bool model_callback_installed);
    void validate_material_animation(std::size_t stream, std::size_t material) const;
    std::size_t path(std::uint32_t);
private:
    const Reader& reader_;
    std::vector<menu::Material>& materials_;
    std::vector<menu::Animation>& animations_;
    std::span<const MaterialExtent> material_extents_;
    std::vector<menu::AnimationPath>* paths_;
};
} // namespace isle::model_assets
#endif
