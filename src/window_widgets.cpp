#include "window_widgets.hpp"
#include "window_widget_math.hpp"
#include <algorithm>
#include <cmath>
namespace isle::window {
namespace {
scene::SpriteHandle make(scene::Services& s,std::uint32_t id) noexcept {
 scene::SpriteHandle h;h.object=s.object(1u<<6,6,6);if(h.object){h.sprite=s.append_sprite(h.object,id);s.set_render(h.object,{2,6,4});}return h;
}
void visibility(IsleSprite* v,bool show) noexcept { if(v){if(show)v->attr&=~ISLE_SP_HIDDEN;else v->attr|=ISLE_SP_HIDDEN;} }
}
Frame::Frame(scene::Services& s,const Resources& r):s_(s),r_(r){}
Frame::~Frame(){retire();}
bool Frame::create() noexcept {
 retire();x_=y_=60;style_=0;for(unsigned i=0;i<9;++i){sprites_[i]=make(s_,r_.frame(0,i).identity);if(!sprites_[i].sprite){retire();return false;}s_.view(sprites_[i].sprite)->attr|=ISLE_SP_SCALE|ISLE_SP_TRANSPARENT;}
 set_size(204,56);return true;
}
void Frame::retire() noexcept {for(auto& h:sprites_){if(h.object)s_.erase(h.object);h={};}}
void Frame::show(bool b) noexcept {for(auto h:sprites_)visibility(s_.view(h.sprite),b);}
void Frame::show_background(bool b) noexcept {visibility(s_.view(sprites_[6].sprite),b);}
void Frame::set_opacity(int a) noexcept {for(auto h:sprites_)if(auto v=s_.view(h.sprite))v->alpha=static_cast<std::uint8_t>(a);}
void Frame::set_style(int style) noexcept {
 style_=style;
 const auto palette=static_cast<unsigned>(std::clamp(style,0,2));
 for(unsigned i=0;i<9;++i){
  auto* v=s_.view(sprites_[i].sprite);
  if(!v)continue;
  const auto saved=*v;
  if(!s_.rebind_sprite(sprites_[i].sprite,r_.frame(palette,i).identity))return;
  const auto* bitmap=v->bitmap;
  *v=saved;
  v->bitmap=bitmap;
 }
}
void Frame::set_pos(int x,int y) noexcept {
 x_=x;y_=y;const auto geometry=detail::frame_geometry(x,y,width_,height_);
 for(unsigned i=0;i<9;++i)if(auto v=s_.view(sprites_[i].sprite)){v->x=geometry[i].x;v->y=geometry[i].y;v->scalex=geometry[i].sx;v->scaley=geometry[i].sy;}
}
void Frame::set_size(int w,int h) noexcept {
 width_=std::clamp(w,6,320);height_=std::clamp(h,6,240);
 set_pos(x_,y_);
}
Color Frame::background_color() const noexcept {auto v=s_.view(sprites_[6].sprite);if(!v||!v->bitmap||!v->bitmap->buf||v->bitmap->byte_count<2)return {};const auto p=v->bitmap->buf;unsigned n=(unsigned(p[0])<<8)|p[1];return {std::uint8_t((n>>8)&248),std::uint8_t((n>>3)&248),std::uint8_t((n<<2)&248),std::uint8_t((n&1)*255)};}
bool Frame::expand(int style,bool showing) noexcept {
 set_style(style);show_background(true);
 const int frames=r_.frame_animation_frames();
 for(int i=0;i<=frames;++i){const auto g=detail::frame_expansion(style,showing?i:frames-i,frames);
 set_pos(g.x,g.y);set_size(g.width,g.height);set_opacity(g.alpha);if(!s_.wait())return false;}
 if(showing){set_pos(96,165);set_size(208,60);}return true;
}
bool Frame::fade_in(int style) noexcept {show(true);if(style>=0&&style<=3&&!expand(style,true))return false;set_opacity(255);show_background(false);return true;}
bool Frame::fade_out() noexcept {show_background(true);if(style_>=0&&style_<=3&&!expand(style_,false))return false;show(false);return true;}
Focus::Focus(scene::Services& s,const Resources& r):s_(s),r_(r){}
Focus::~Focus(){retire();}
bool Focus::create() noexcept {retire();stopped_=false;for(unsigned i=0;i<4;++i){sprites_[i]=make(s_,r_.focus(i).identity);if(!sprites_[i].sprite){retire();return false;}auto v=s_.view(sprites_[i].sprite);v->x=v->y=80;}set_pos(22,29);set_size(62,13);if(!s_.process(sprites_[0].object,run,this,ISLE_GAME_STACKFUL,0)){retire();return false;}return true;}
void Focus::retire() noexcept {for(auto& h:sprites_){if(h.object)s_.erase(h.object);h={};}}
void Focus::show(bool b) noexcept {for(auto h:sprites_)visibility(s_.view(h.sprite),b);}
void Focus::move_front() noexcept {for(auto h:sprites_)s_.move_to_head(h.object);}
void Focus::set_pos(int x,int y) noexcept {x_=tx_=static_cast<float>(x);y_=ty_=static_cast<float>(y);}
void Focus::set_target_pos(int x,int y) noexcept {tx_=static_cast<float>(x);ty_=static_cast<float>(y);}
void Focus::set_size(int w,int h) noexcept {w_=tw_=static_cast<float>(w);h_=th_=static_cast<float>(h);}
void Focus::set_target_size(int w,int h) noexcept {tw_=static_cast<float>(w);th_=static_cast<float>(h);}
void Focus::stop_animation(bool b) noexcept {stopped_=!b;}
void Focus::run(IsleGameObject*,void* p) noexcept {
 auto& f=*static_cast<Focus*>(p);
 detail::FocusClock clock;
 while(true){
  clock.step(f.stopped_);
  f.x_=detail::focus_interpolate(f.x_,f.tx_);
  f.y_=detail::focus_interpolate(f.y_,f.ty_);
  f.w_=detail::focus_interpolate(f.w_,f.tw_);
  f.h_=detail::focus_interpolate(f.h_,f.th_);
  const auto corners=detail::focus_geometry(f.x_,f.y_,f.w_,f.h_,clock.offset);
  for(unsigned i=0;i<4;++i)if(auto* v=f.s_.view(f.sprites_[i].sprite)){
   v->x=corners[i].x;
   v->y=corners[i].y;
  }
  if(!f.s_.wait())return;
 }
}
} // namespace isle::window
