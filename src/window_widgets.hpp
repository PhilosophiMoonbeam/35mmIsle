#ifndef ISLE_WINDOW_WIDGETS_HPP
#define ISLE_WINDOW_WIDGETS_HPP
#include "scene_services.hpp"
#include "window_resources.hpp"
#include "window_text.hpp"
#include <array>
#include <memory>
#include <span>
#include <string_view>
namespace isle::window {
// Stable scene-owned callback userdata. Retire outside native traversal and only
// after actual graphics borrowers cease; Services joins callbacks before erase.
class Frame final {
public:
 Frame(scene::Services&, const Resources&); ~Frame();
 Frame(const Frame&)=delete; Frame& operator=(const Frame&)=delete;
 bool create() noexcept; void retire() noexcept;
 void set_style(int) noexcept; void set_pos(int,int) noexcept; void set_size(int,int) noexcept;
 void show(bool) noexcept; void show_background(bool) noexcept; void set_opacity(int) noexcept;
 Color background_color() const noexcept;
 bool fade_in(int) noexcept; bool fade_out() noexcept;
private:
 bool expand(int,bool) noexcept;
 scene::Services& s_; const Resources& r_; std::array<scene::SpriteHandle,9> sprites_{};
 int x_=60,y_=60,width_=204,height_=56,style_=0;
};
class Focus final {
public:
 Focus(scene::Services&,const Resources&); ~Focus();
 Focus(const Focus&)=delete; Focus& operator=(const Focus&)=delete;
 bool create() noexcept; void retire() noexcept; void show(bool) noexcept; void move_front() noexcept;
 void set_pos(int,int) noexcept; void set_target_pos(int,int) noexcept;
 void set_size(int,int) noexcept; void set_target_size(int,int) noexcept;
 void stop_animation(bool) noexcept;
private:
 static void run(IsleGameObject*,void*) noexcept;
 scene::Services& s_; const Resources& r_; std::array<scene::SpriteHandle,4> sprites_{};
 float x_=22,y_=29,w_=62,h_=13,tx_=22,ty_=29,tw_=62,th_=13;
 bool stopped_=false;
};
struct Button { int id=35; std::string_view text{}; };
class Layout final {
public:
 Layout(scene::Services&,const Resources&); ~Layout();
 bool initialize() noexcept; void retire() noexcept;
 bool create_buttons(std::span<const Button>) noexcept; bool hide_buttons() noexcept;
 bool wait_transition() noexcept; void set_instant_transition(bool) noexcept;
 void toggle_animation_direction() noexcept; void toggle_animation_speed() noexcept;
 void disable_button(bool,int) noexcept; void disable_all_except(bool,int) noexcept;
 void show_button(bool,int) noexcept; void set_button_scale(float,int) noexcept;
 void set_header_flags(int) noexcept; void show_header_element(int) noexcept; void hide_header_element(int) noexcept;
 void show_panel(bool) noexcept;
 std::span<const Button> buttons() const noexcept;
 static std::string_view button_text(std::span<const Button>,int) noexcept;
 static void set_button_text(std::span<Button>,std::string_view,int) noexcept;
private:
 struct Impl; std::unique_ptr<Impl> p_;
};
}
#endif
