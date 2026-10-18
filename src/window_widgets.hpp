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
 // Records/owners remain address-stable until terminal or cancel. Only one
 // fade per Frame; begin rejects a live owner/record. Polling a pending wait
 // is inert; resume=true acknowledges exactly one completed source wait.
 // Cancel/retire retain partial visuals, never finish the fade implicitly.
 class FadeOperation final {
 public:
  FadeOperation() noexcept = default;
  FadeOperation(const FadeOperation&)=delete;
  FadeOperation& operator=(const FadeOperation&)=delete;
 private:
  friend class Frame;
  Frame* owner_=nullptr;
  StepResult result_{};
  enum class Phase : std::uint8_t { start, animate, finish };
  Phase phase_=Phase::start;
  int style_=0,frames_=0,index_=0;
  bool showing_=false;
 };
 Frame(scene::Services&, const Resources&); ~Frame();
 Frame(const Frame&)=delete; Frame& operator=(const Frame&)=delete;
 bool create() noexcept; void retire() noexcept;
 void set_style(int) noexcept; void set_pos(int,int) noexcept; void set_size(int,int) noexcept;
 void show(bool) noexcept; void show_background(bool) noexcept; void set_opacity(int) noexcept;
 Color background_color() const noexcept;
 bool fade_in(int) noexcept; bool fade_out() noexcept;
 StepResult begin_fade_in(FadeOperation&,int style) noexcept;
 StepResult begin_fade_out(FadeOperation&) noexcept;
 StepResult step_fade(FadeOperation&,bool resume=false) noexcept;
 void cancel_fade(FadeOperation&) noexcept;
private:
 StepResult begin_fade(FadeOperation&,int,bool) noexcept;
 StepResult finish_fade(FadeOperation&,StepError=StepError::none) noexcept;
 bool run_fade(FadeOperation&) noexcept;
 FadeOperation* active_fade_=nullptr;
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
 // The operation, owner, button table and every referenced string stay stable
 // across source waits. A created table remains borrowed by Layout until hide,
 // replacement or retire, not merely until create completes. No table copies.
 // One operation per Layout; no concurrent/reentrant wait/hide/create. Cancel
 // releases the operation only, retaining partial visuals and any active table.
 // Source callbacks may change text in stable backing, not destroy/reallocate it.
 class Operation final {
 public:
  Operation() noexcept = default;
  Operation(const Operation&)=delete;
  Operation& operator=(const Operation&)=delete;
 private:
  friend class Layout;
  Layout* owner_=nullptr;
  StepResult result_{};
  enum class Kind : std::uint8_t { wait, hide, create };
  enum class Phase : std::uint8_t { start, wait_previous, hide, wait_hidden, scan, build };
  Kind kind_=Kind::wait;
  Phase phase_=Phase::start;
  std::span<const Button> list_{};
  std::size_t cursor_=0;
  int count_=0,split_=0,index_=0;
  float phase_value_=0,stagger_=0;
 };
 bool initialize() noexcept; void retire() noexcept;
 bool create_buttons(std::span<const Button>) noexcept; bool hide_buttons() noexcept;
 bool wait_transition() noexcept; void set_instant_transition(bool) noexcept;
 StepResult begin_wait_transition(Operation&) noexcept;
 StepResult begin_hide_buttons(Operation&) noexcept;
 StepResult begin_create_buttons(Operation&,std::span<const Button>) noexcept;
 StepResult step(Operation&,bool resume=false) noexcept;
 void cancel(Operation&) noexcept;
 void toggle_animation_direction() noexcept; void toggle_animation_speed() noexcept;
 void disable_button(bool,int) noexcept; void disable_all_except(bool,int) noexcept;
 void show_button(bool,int) noexcept; void set_button_scale(float,int) noexcept;
 void set_header_flags(int) noexcept; void show_header_element(int) noexcept; void hide_header_element(int) noexcept;
 void show_panel(bool) noexcept;
 std::span<const Button> buttons() const noexcept;
 static std::string_view button_text(std::span<const Button>,int) noexcept;
 static void set_button_text(std::span<Button>,std::string_view,int) noexcept;
private:
 StepResult begin(Operation&,Operation::Kind,std::span<const Button>) noexcept;
 StepResult finish(Operation&,StepError=StepError::none) noexcept;
 bool run(Operation&) noexcept;
 struct Impl; std::unique_ptr<Impl> p_;
};
}
#endif
