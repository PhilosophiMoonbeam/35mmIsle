#ifndef ISLE_EFFECT_GRAPHICS_RESOURCES_HPP
#define ISLE_EFFECT_GRAPHICS_RESOURCES_HPP
#include "menu_effects.hpp"
#include "sprite_producer.hpp"
#include <vector>

namespace isle::menu {
// Sole cold-construction packer for bank-qualified canonical FX assets. Native
// address arrays and borrowed union spans remain fixed through GPU retirement.
// Snapshot and selected banks outlive this owner and every exported view.
class EffectGraphicsResources final {
public:
    EffectGraphicsResources(const rom::Snapshot&, std::span<const EffectBank* const>, RamRange arena);
    EffectGraphicsResources(rom::Snapshot&&, std::span<const EffectBank* const>, RamRange) = delete;
    EffectGraphicsResources(const EffectGraphicsResources&) = delete;
    EffectGraphicsResources& operator=(const EffectGraphicsResources&) = delete;
    EffectGraphicsResources(EffectGraphicsResources&&) = delete;
    EffectGraphicsResources& operator=(EffectGraphicsResources&&) = delete;
    std::span<const EffectBankBinding> bindings() const;
    std::span<const sprite::AssetSpan> installation_spans() const;
    std::span<const RamRange> reservations() const;
private:
    struct BankAssets {
        struct ImageAddresses { std::vector<std::uint32_t> frames, palettes; };
        std::vector<ImageAddresses> addresses;
        std::vector<EffectImageBinding> images;
    };
    const rom::Snapshot& snapshot_;
    const std::byte* source_;
    std::vector<BankAssets> bank_assets_;
    std::vector<EffectBankBinding> banks_;
    std::vector<sprite::AssetSpan> spans_;
    std::vector<RamRange> ranges_;
    void require_live() const;
};
} // namespace isle::menu
#endif
