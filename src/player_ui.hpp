#ifndef ISLE_PLAYER_UI_HPP
#define ISLE_PLAYER_UI_HPP
#include "player_ui_resources.hpp"
#include "scene_services.hpp"
#include <array>
namespace isle::player {
class Ui final {
public:
    /* No OM publication. resource_base is the position of resources.roots() in
     * the caller's composed sprite Producer, including its raw glyph variants. */
    Ui(const UiResources&, scene::Services&, IslePlayerSource&, sprite::Producer&,
       std::size_t resource_base);
    ~Ui();
    Ui(const Ui&) = delete;
    Ui& operator=(const Ui&) = delete;
    IslePlayerUiSource& source() noexcept { return source_; }
    const UiResources& resources() const noexcept { return resources_; }
    sprite::Producer& producer() noexcept { return producer_; }
    std::size_t resource_base() const noexcept { return resource_base_; }
    /* Invoke once at original Msg camera draw position, lane0. No OM waits or
     * C failure transfers cross this returning call. Views remain stable until
     * next message_draw; caller holds the real preparation/GPU resource borrow.
     * Ordinary native Icons/film/Player sprites use Services::sprite_draws. */
    IsleGameResult message_draw() noexcept;
    std::span<const sprite::Draw> message_draws() const noexcept { return {draws_.data(),draw_count_}; }
    IsleGameResult object(std::uint32_t, IsleGameSourceCallback, IsleGameObject**) noexcept;
    IsleGameResult render(IsleGameObject*,std::uint8_t,std::uint32_t) noexcept;
    IsleGameResult append(IsleGameObject*,const IsleSprite&,IsleGameSprite**) noexcept;
    IsleGameResult digit(unsigned,int) noexcept;
    IsleGameResult glyph(const std::uint8_t*,int,int,int,int,int) noexcept;
    IsleGameResult message_camera(IsleGameObject**) noexcept;
    IsleGameResult erase(IsleGameObject*) noexcept;
    IsleGameResult copy_bitmap(IsleSprite*,const IsleSprite*) noexcept;
    /* Only after every caller's graphics/readback borrower closes. Performs
     * real Services retirement/join before retained source storage can die. */
    IsleGameResult retire() noexcept;
private:
    static void joined(IsleGameObject*,void*) noexcept;
    const UiResources& resources_;
    scene::Services& services_;
    IslePlayerUiSource source_{};
    std::size_t resource_base_;
    sprite::Producer& producer_;
    std::array<IsleGameObject*,3> objects_{};
    std::array<IsleSprite,514> glyph_views_{};
    std::array<sprite::Draw,514> draws_{};
    std::size_t draw_count_=0;
};
}
#endif
