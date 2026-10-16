#ifndef ISLE_WINDOW_RESOURCES_HPP
#define ISLE_WINDOW_RESOURCES_HPP
#include "rom_snapshot.hpp"
#include "scene_resources.hpp"
#include "sprite_producer.hpp"
#include <array>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

namespace isle::window {
struct FontData {
    std::span<const std::uint8_t> characters_be, ascii_be;
    std::array<std::span<const std::uint8_t>, 2> widths, glyphs;
    void validate() const;
    std::size_t index(std::uint16_t character) const noexcept;
    std::uint16_t ascii(unsigned character) const noexcept;
    std::span<const std::uint8_t> glyph(unsigned charset, std::uint16_t character) const noexcept;
};
struct ResourceBank {
    enum class Group : unsigned { common = 0, name = 1, oak = 2 };
    sprite::BankOwner owner;
    std::uint32_t guest;
    std::size_t rom_begin, rom_end;
};
// Private, data-only copies of bounded admitted ROM spans. No protected tables
// or image bytes are compiled into the implementation or test executable.
class Resources final {
public:
    static std::shared_ptr<const Resources> load(const rom::Snapshot&);
    const FontData& font() const noexcept { return font_; }
    std::span<const ResourceBank> banks() const noexcept { return banks_; }
    std::span<const scene::SpriteResource> sprites() const noexcept { return sprites_; }
    // Overlay groups are separate residency domains: name and Oak reuse guest
    // addresses and MUST NOT be installed together. Add common to either one.
    std::span<const scene::SpriteResource> sprites(ResourceBank::Group) const noexcept;
    // Exact common source pre-render list and its immutable referenced data.
    // The address is physical, not KSEG; views borrow this Resources owner.
    std::uint32_t common_pre_render() const noexcept;
    std::span<const sprite::AssetSpan> common_render_assets() const noexcept;
    const scene::SpriteResource& table_variant(std::uint32_t table, unsigned index) const;
    std::string_view text(ResourceBank::Group group, std::uint32_t guest_address) const;
    const scene::SpriteResource& sprite(std::uint32_t identity) const;
    const scene::SpriteResource& frame(unsigned style, unsigned piece) const;
    std::array<std::uint8_t,4> frame_background(unsigned style) const;
    int frame_animation_frames() const;
    const scene::SpriteResource& focus(unsigned corner) const;
    const scene::SpriteResource& button(char name, unsigned phase) const;
    const scene::SpriteResource& panel_button(unsigned index) const;
    const scene::SpriteResource& header_arrow(unsigned index) const;
    std::int32_t button_interval(char name) const;
    std::string_view name_character(std::size_t index) const;
    std::string_view name_end_label(unsigned index) const;
private:
    std::span<const std::uint8_t> bytes(unsigned bank, std::uint32_t address, std::size_t count) const;
    std::uint32_t pointer(unsigned bank, std::uint32_t table, std::size_t index) const;
    std::string_view string(unsigned bank, std::uint32_t address) const;
    void root(unsigned bank, std::uint32_t address);
    void variant(unsigned bank, std::uint32_t prototype, std::uint32_t bitmap);
    std::array<ResourceBank, 3> banks_{};
    sprite::BankOwner common_render_owner_;
    std::array<sprite::AssetSpan, 2> common_render_assets_{};
    FontData font_{};
    std::vector<scene::SpriteResource> sprites_;
    std::size_t common_count_ = 0, name_count_ = 0;
};
} // namespace isle::window
#endif
