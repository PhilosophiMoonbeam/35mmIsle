#include "window_widgets.hpp"
#include "window_widget_math.hpp"
#include <algorithm>
namespace isle::window {
struct Layout::Impl {
 scene::Services& s;const Resources& r;
 std::array<scene::SpriteHandle,33> objects{};
 std::array<scene::SpriteHandle,4> furniture{};
 std::array<IsleGameSprite*,2> arrows{};
 std::array<float,33> phase{},base{};
 std::array<Button,9> slots{};
 std::span<const Button> active{};
 int state=2,frames=3,flags=0;bool instant=false,horizontal=false,panel=false;
 float panel_x=-180;std::array<float,3> header_y{-40,-40,-40};std::array<int,2> counter{};
 Impl(scene::Services& ss,const Resources& rr):s(ss),r(rr){}
 scene::SpriteHandle make(std::uint32_t id){scene::SpriteHandle h;h.object=s.object(1u<<6,6,6);if(h.object){h.sprite=s.append_sprite(h.object,id);s.set_render(h.object,{30,6,0x40000000});}return h;}
 static void animate(IsleGameObject*,void* data) noexcept {
 auto& p=*static_cast<Impl*>(data);int sliding=0;
 for(unsigned i=0;i<33;++i)if(auto v=p.s.view(p.objects[i].sprite))
 if(detail::animate_button(*v,p.phase[i],p.base[i],p.state,p.frames,p.instant,p.horizontal))++sliding;
 if(!sliding)p.state=2;
 }
 static void run(IsleGameObject* object,void* data) noexcept {auto& p=*static_cast<Impl*>(data);while(true){animate(object,data);if(!p.s.wait())return;}}
 static void update(IsleGameObject* object,void* data) noexcept {
 auto& p=*static_cast<Impl*>(data);
 if(object==p.furniture[0].object){
  p.panel_x=static_cast<float>(p.panel_x+((p.panel?0.0f:-180.0f)-p.panel_x)*0.15);
  if(auto* v=p.s.view(p.furniture[0].sprite))v->x=static_cast<std::int16_t>(p.panel_x);
  return;
 }
 for(unsigned i=0;i<3;++i)if(object==p.furniture[i+1].object){
  const float target=(p.flags&(i==0?1:i==1?2:4))?0.0f:-40.0f;
  p.header_y[i]=static_cast<float>(p.header_y[i]+(target-p.header_y[i])*0.2);
  const auto y=static_cast<std::int16_t>(p.header_y[i]);
  if(auto* v=p.s.view(p.furniture[i+1].sprite))v->y=y;
  if(i<2){
   const int c=++p.counter[i];
   auto* v=p.s.view(p.arrows[i]);
   if(c==0||c==-47){
    const auto saved=*v;
    if(!p.s.rebind_sprite(p.arrows[i],p.r.header_arrow(c<0?i+1:0).identity))return;
    const auto* bitmap=v->bitmap;
    *v=saved;
    v->bitmap=bitmap;
   }
   v->x=i?283:105;
   v->y=static_cast<std::int16_t>(y+22);
   if(c>48)p.counter[i]=-48;
  }
 }
 } // update
};
Layout::Layout(scene::Services& s,const Resources& r):p_(std::make_unique<Impl>(s,r)){}
Layout::~Layout(){retire();}
bool Layout::initialize() noexcept {
 retire();auto& p=*p_;p.panel_x=-180;p.header_y={-40,-40,-40};p.counter={};p.flags=0;p.panel=false;
 p.furniture[0]=p.make(0x803A1360);if(!p.furniture[0].sprite){retire();return false;}
 auto panel=p.s.view(p.furniture[0].sprite);panel->x=-180;panel->y=0;panel->attr|=ISLE_SP_SCALE|ISLE_SP_TRANSPARENT;panel->scalex=23;panel->scaley=60;panel->alpha=178;
 if(!p.s.process(p.furniture[0].object,Impl::update,&p,ISLE_GAME_SYNCHRONOUS,0)){retire();return false;}
 for(unsigned i=0;i<33;++i){p.objects[i]=p.make(p.r.panel_button(i).identity);if(!p.objects[i].sprite){retire();return false;}auto v=p.s.view(p.objects[i].sprite);v->x=0;v->y=static_cast<std::int16_t>(20+i*30);v->attr=(v->attr&~ISLE_SP_SCALE)|ISLE_SP_HIDDEN;v->scalex=v->scaley=1;p.phase[i]=.0001f;}
 if(!p.s.process(p.objects[0].object,Impl::run,&p,ISLE_GAME_STACKFUL,0)){retire();return false;}
 constexpr std::uint32_t ids[3]={0x803A1B10,0x803A22C0,0x803A51A0};
 for(unsigned i=0;i<3;++i){p.furniture[i+1]=p.make(ids[i]);if(!p.furniture[i+1].sprite){retire();return false;}auto v=p.s.view(p.furniture[i+1].sprite);v->x=i==0?96:i==1?280:124;v->y=-40;
 if(i<2){p.arrows[i]=p.s.append_sprite(p.furniture[i+1].object,0x803A6220);if(!p.arrows[i]){retire();return false;}auto a=p.s.view(p.arrows[i]);a->x=i?283:105;a->y=-18;}
 if(!p.s.process(p.furniture[i+1].object,Impl::update,&p,ISLE_GAME_SYNCHRONOUS,0)){retire();return false;}}
 return true;
}
void Layout::retire() noexcept {
 if(!p_)return;
 auto& p=*p_;
 // Whole-object erase joins each process before Impl or its aliases can die.
 for(auto& h:p.objects){if(h.object)p.s.erase(h.object);h={};}
 for(auto& h:p.furniture){if(h.object)p.s.erase(h.object);h={};}
 p.arrows={};
 p.slots.fill({});
 p.active={};
 p.state=2;
}
bool Layout::wait_transition() noexcept {while(p_->state!=2)if(!p_->s.wait())return false;return true;}
bool Layout::hide_buttons() noexcept {
 auto& p=*p_;
 if(p.active.empty())return true;
 if(!wait_transition())return false;
 float phase=static_cast<float>(p.frames+1)*detail::button_half_pi;
 const float stagger=static_cast<float>(p.frames*90.0/9.0)*detail::button_pi/180.0f;
 for(auto b:p.active){
  if(b.id==35)break;
  if(b.id<0||b.id>32)continue;
  auto* v=p.s.view(p.objects[b.id].sprite);
  v->attr|=ISLE_SP_SCALE;
  if(p.horizontal)v->scalex=1;else v->scaley=1;
  p.base[b.id]=p.horizontal?v->x:v->y;
  if(p.instant)p.phase[b.id]=0;
  else{p.phase[b.id]=phase;phase-=stagger;}
 }
 p.state=0;
 p.active={};
 return true;
}
bool Layout::create_buttons(std::span<const Button> list) noexcept {
 auto& p=*p_;if(!p.active.empty()){if(p.active.data()==list.data())return true;if(!hide_buttons()||!wait_transition())return false;}
 p.slots.fill({});int count=0,split=0,index=0;for(auto b:list){if(b.id==35)break;if(b.id>=0&&b.id<=32)++count;if(b.id==34)split=index;++index;}
 float phase=0;
 const float stagger=static_cast<float>(p.frames*90.0/9.0)*detail::button_pi/180.0f;
 index=0;
 for(auto b:list){
  if(b.id==35)break;
  if(b.id==34){index=split-count+8;continue;}
  if(b.id<0||b.id>32)continue;
  auto* v=p.s.view(p.objects[b.id].sprite);
  v->attr|=ISLE_SP_SCALE|ISLE_SP_HIDDEN;
  if(p.horizontal)v->scalex=1;else v->scaley=1;
  v->x=0;
  v->y=static_cast<std::int16_t>(25+index*24);
  p.base[b.id]=p.horizontal?v->x:v->y;
  if(p.instant){p.phase[b.id]=detail::button_half_pi;v->attr&=~ISLE_SP_HIDDEN;}
  else{p.phase[b.id]=phase;phase-=stagger;}
  if(index>=0&&index<8)p.slots[index].id=b.id;
  ++index;
 }
 p.state=1;p.active=list;return true;
}
void Layout::set_instant_transition(bool b) noexcept {p_->instant=b;}
void Layout::toggle_animation_direction() noexcept {p_->horizontal=!p_->horizontal;}
void Layout::toggle_animation_speed() noexcept {p_->frames=p_->frames?0:3;}
void Layout::disable_button(bool disabled,int id) noexcept {id=std::clamp(id,0,32);if(auto v=p_->s.view(p_->objects[id].sprite))v->red=v->green=v->blue=disabled?128:255;}
void Layout::disable_all_except(bool disabled,int id) noexcept {id=std::clamp(id,0,32);for(int i=0;i<33;++i)disable_button(disabled&&i!=id,i);}
void Layout::show_button(bool show,int id) noexcept {if(auto v=p_->s.view(p_->objects[std::clamp(id,0,32)].sprite)){v->alpha=show?255:0;if(show)v->attr&=~ISLE_SP_HIDDEN;else v->attr|=ISLE_SP_HIDDEN;}}
void Layout::set_button_scale(float scale,int id) noexcept {if(auto v=p_->s.view(p_->objects[std::clamp(id,0,32)].sprite)){v->scaley=scale;if(scale==1)v->attr&=~ISLE_SP_SCALE;else v->attr|=ISLE_SP_SCALE;}}
void Layout::set_header_flags(int flags) noexcept {p_->flags=flags;}
void Layout::show_header_element(int flags) noexcept {p_->flags|=flags;}
void Layout::hide_header_element(int flags) noexcept {p_->flags&=~flags;}
void Layout::show_panel(bool show) noexcept {p_->panel=show;}
std::span<const Button> Layout::buttons() const noexcept {return p_->slots;}
std::string_view Layout::button_text(std::span<const Button> list,int id) noexcept {for(auto b:list){if(b.id==35)break;if(b.id==id)return b.text;}return {};}
void Layout::set_button_text(std::span<Button> list,std::string_view text,int id) noexcept {for(auto& b:list){if(b.id==35)break;if(b.id==id)b.text=text;}}
}
