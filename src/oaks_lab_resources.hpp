#ifndef ISLE_OAKS_LAB_RESOURCES_HPP
#define ISLE_OAKS_LAB_RESOURCES_HPP
#include "window_resources.hpp"
#include "window_widgets.hpp"
#include <utility>

namespace isle::oaks_lab {
// Admits common window + Oak initialized data only. Name overlay is never a
// sprite identity/residency source, despite overlapping guest addresses.
class Resources final {
public:
    static std::shared_ptr<const Resources> load(const rom::Snapshot&,std::shared_ptr<const window::Resources>);
    const std::shared_ptr<const window::Resources>& window() const noexcept { return window_; }
    scene::ResourceView view() const noexcept;
    // Canonical source mappings. The compositor caller must explicitly relocate
    // the Oak overlay bank away from native command storage before installation.
    std::span<const sprite::BankMapping> banks() const noexcept { return banks_; }
    std::span<const IsleSprite* const> roots() const noexcept { return roots_; }
    std::string_view text(std::uint32_t address) const;
    std::span<const window::Button> main_buttons(bool report_unlocked) const noexcept;
    std::span<const window::Button> course_buttons(unsigned unlocked) const;
    std::uint32_t word(std::uint32_t address) const;
    float real(std::uint32_t address) const;
    std::size_t sprite_index(std::uint32_t identity) const;
    int species_index(unsigned id) const noexcept;
private:
    std::shared_ptr<const window::Resources> window_;
    std::vector<scene::SpriteResource> sprites_;
    std::vector<const IsleSprite*> roots_;
    std::array<sprite::BankMapping,2> banks_;
    std::array<std::array<window::Button,9>,2> main_{};
    std::array<std::array<window::Button,9>,7> courses_{};
    std::vector<std::pair<std::uint32_t,std::size_t>> indices_;
    std::array<std::int8_t,151> species_{};
    std::span<const std::uint8_t> bytes(std::uint32_t address,std::size_t size) const;
    void buttons(std::uint32_t address,std::span<window::Button>);
};
} // namespace isle::oaks_lab
#endif
