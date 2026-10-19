#ifndef ISLE_VALLEY_ACTOR_WINDOW_MECHANICS_HPP
#define ISLE_VALLEY_ACTOR_WINDOW_MECHANICS_HPP
#include "valley_actor_source.h"
#include "scene_resources.hpp"
#include <cstdint>
#include <span>
namespace isle::scene { class Services; }
namespace isle::player { class Ui; }
namespace isle::valley {
// Entry-owned, fixed-address native bindings only. Original Msg_* owns all text,
// printing/input state and its real C printer process. Dependencies survive all
// Source callbacks and graphics borrows; destruction follows actual scene joins.
class WindowMechanics final {
public:
    WindowMechanics(scene::Services&,player::Ui&,
        std::span<const scene::SpriteResource>,std::size_t producer_base) noexcept;
    WindowMechanics(const WindowMechanics&)=delete;
    WindowMechanics& operator=(const WindowMechanics&)=delete;
    IsleGameResult message_reset() noexcept;
    IsleGameResult message_interactive(bool) noexcept;
    IsleGameResult message_printed(bool*) noexcept;
    IsleGameResult message_show(IsleValleyActorSource*,const char*,std::int32_t,std::int32_t,
        std::uint8_t,std::uint8_t,std::uint8_t,std::uint8_t,std::int32_t,std::uint8_t) noexcept;
    IsleGameResult message_handler(IsleValleyActorSource*,IsleValleyMessageCallback) noexcept;
    IsleGameResult sprite_add(IsleGameObject*,const IsleSprite*,IsleGameSprite**) noexcept;
    IsleGameResult sprite_move(IsleSprite*,std::int32_t,std::int32_t) noexcept;
    IsleGameResult sprite_scale(IsleSprite*,float,float) noexcept;
    IsleGameResult sprite_color(IsleSprite*,std::uint8_t,std::uint8_t,std::uint8_t,std::uint8_t) noexcept;
    IsleGameResult sprite_link(IsleGameObject*,std::uint8_t,std::uint32_t,std::int32_t) noexcept;
    IsleGameResult player_name(const char**) noexcept;
private:
    IsleGameResult ready() const noexcept;
    IsleGameResult rebind(IsleSprite*,IsleGameSprite**) noexcept;
    std::size_t resource_index(const IsleSprite&) const noexcept;
    scene::Services& services_;
    player::Ui& ui_;
    std::span<const scene::SpriteResource> resources_;
    std::size_t base_;
    IsleValleyActorSource* handler_source_=nullptr;
    IsleValleyMessageCallback handler_=nullptr;
};
}
// Exact service-table signatures; no unchecked Source helper is called by C++.
IsleGameResult isle_valley_window_message_reset(IsleValleyActorSource*) noexcept;
IsleGameResult isle_valley_window_message_interactive(IsleValleyActorSource*) noexcept;
IsleGameResult isle_valley_window_message_noninteractive(IsleValleyActorSource*) noexcept;
IsleGameResult isle_valley_window_message_printed(IsleValleyActorSource*,bool*) noexcept;
IsleGameResult isle_valley_window_message_show(IsleValleyActorSource*,const char*,int32_t,int32_t,uint8_t,uint8_t,uint8_t,uint8_t,int32_t,uint8_t) noexcept;
IsleGameResult isle_valley_window_message_handler(IsleValleyActorSource*,IsleValleyMessageCallback) noexcept;
IsleGameResult isle_valley_window_sprite_add(IsleValleyActorSource*,IsleGameObject*,const IsleSprite*,IsleGameSprite**) noexcept;
IsleGameResult isle_valley_window_sprite_move(IsleValleyActorSource*,IsleSprite*,int32_t,int32_t) noexcept;
IsleGameResult isle_valley_window_sprite_x2_move(IsleValleyActorSource*,IsleSprite*,int32_t,int32_t) noexcept;
IsleGameResult isle_valley_window_sprite_scale(IsleValleyActorSource*,IsleSprite*,float,float) noexcept;
IsleGameResult isle_valley_window_sprite_color(IsleValleyActorSource*,IsleSprite*,uint8_t,uint8_t,uint8_t,uint8_t) noexcept;
IsleGameResult isle_valley_window_sprite_link(IsleValleyActorSource*,IsleGameObject*,uint8_t,uint32_t,int32_t) noexcept;
IsleGameResult isle_valley_window_player_name(IsleValleyActorSource*,const char**) noexcept;
IsleGameResult isle_valley_window_check_player_flag(IsleValleyActorSource*,int32_t,int32_t*) noexcept;
IsleGameResult isle_valley_window_set_player_flag(IsleValleyActorSource*,int32_t,int32_t) noexcept;
IsleGameResult isle_valley_window_application_notice(IsleValleyActorSource*,int32_t) noexcept;
IsleGameResult isle_valley_window_transition_begin(IsleValleyActorSource*,int32_t,int32_t) noexcept;
IsleGameResult isle_valley_window_transition_color(IsleValleyActorSource*,float,float,float,float) noexcept;
IsleGameResult isle_valley_window_transition_busy(IsleValleyActorSource*,int32_t*) noexcept;
#endif
