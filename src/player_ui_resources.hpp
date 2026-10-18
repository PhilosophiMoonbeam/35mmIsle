#ifndef ISLE_PLAYER_UI_RESOURCES_HPP
#define ISLE_PLAYER_UI_RESOURCES_HPP
#include "rom_snapshot.hpp"
#include "scene_resources.hpp"
#include "sprite_producer.hpp"
#include "player_ui_source.h"
#include <array>
#include <vector>
namespace isle::player {
/* Initialized own app_level ROM data. One bank identity is shared by UI,
 * Player reticles/pause and the composed Producer; no generated PNG dependency. */
class UiResources final {
public:
    static constexpr std::uint32_t bank_base = 0x80366ba0;
    static constexpr std::size_t rom_begin = 0x506fb0, rom_end = 0x533c00;
    explicit UiResources(const rom::Snapshot&);
    UiResources(const UiResources&) = delete;
    UiResources& operator=(const UiResources&) = delete;
    const IsleSprite& sprite(std::uint32_t identity) const;
    const IsleSprite& digit(unsigned value) const;
    const IsleSprite& glyph(const std::uint8_t* pixels) const;
    std::size_t index(const IsleSprite&) const;
    std::size_t glyph_index(const std::uint8_t*) const;
    std::span<const scene::SpriteResource> resources() const noexcept { return resources_; }
    std::span<const IsleSprite* const> roots() const noexcept { return roots_; }
    std::span<const sprite::BankMapping> banks() const noexcept { return banks_; }
    void initialize(IslePlayerUiSource&) const noexcept;
    std::int32_t minimum_photo_interval() const noexcept { return minimum_photo_interval_; }
private:
    std::uint32_t word(std::uint32_t) const;
    sprite::BankOwner bank_;
    std::array<sprite::BankMapping,1> banks_;
    std::vector<scene::SpriteResource> resources_;
    std::vector<const IsleSprite*> roots_;
    std::array<IsleSprite,146> glyphs_{};
    std::array<IsleBitmap,146> glyph_bitmaps_{};
    std::array<IsleSprite,10> digit_sprites_{};
    std::array<IsleBitmap,10> digit_bitmaps_{};
    std::array<std::int16_t,38> wide_{};
    std::array<std::size_t,10> digits_{};
    std::int32_t minimum_photo_interval_ = 0;
};
}
#endif
