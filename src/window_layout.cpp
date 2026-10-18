#include "window_widgets.hpp"
#include "window_widget_math.hpp"
#include <algorithm>
#include <limits>
namespace isle::window {
struct Layout::Impl {
 scene::Services& s;const Resources& r;
 std::array<scene::SpriteHandle,33> objects{};
 std::array<scene::SpriteHandle,4> furniture{};
 std::array<IsleGameSprite*,2> arrows{};
 std::array<float,33> phase{},base{};
 std::array<Button,9> slots{};
 std::span<const Button> active{};
 Operation* operation=nullptr;
 int state=2,frames=3,flags=0;bool instant=false,horizontal=false,panel=false,active_complete=false;
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
 if(p.operation)cancel(*p.operation);
 // Whole-object erase joins each process before Impl or its aliases can die.
 for(auto& h:p.objects){if(h.object)p.s.erase(h.object);h={};}
 for(auto& h:p.furniture){if(h.object)p.s.erase(h.object);h={};}
 p.arrows={};
 p.slots.fill({});
 p.active={};
 p.active_complete=false;
 p.state=2;
}
StepResult Layout::begin(Operation& op,Operation::Kind kind,std::span<const Button> list) noexcept {
 auto& p=*p_;
 if(p.operation || (op.owner_ && op.result_.status!=StepStatus::done && op.result_.status!=StepStatus::error))
  return {StepStatus::error,StepError::busy};
 if(!p.objects[0].sprite || list.size()>static_cast<std::size_t>(std::numeric_limits<int>::max()-8))
  return {StepStatus::error,StepError::invalid_operation};
 op.owner_=this;op.kind_=kind;op.phase_=Operation::Phase::start;op.list_=list;
 op.result_={StepStatus::more};op.cursor_=0;p.operation=&op;return op.result_;
}
StepResult Layout::begin_wait_transition(Operation& op) noexcept {return begin(op,Operation::Kind::wait,{});}
StepResult Layout::begin_hide_buttons(Operation& op) noexcept {return begin(op,Operation::Kind::hide,{});}
StepResult Layout::begin_create_buttons(Operation& op,std::span<const Button> list) noexcept {return begin(op,Operation::Kind::create,list);}
StepResult Layout::finish(Operation& op,StepError error) noexcept {
 op.result_={error==StepError::none?StepStatus::done:StepStatus::error,error};
 op.list_={};
 if(p_->operation==&op)p_->operation=nullptr;
 return op.result_;
}
void Layout::cancel(Operation& op) noexcept {
 if(op.owner_==this && p_->operation==&op)finish(op,StepError::cancelled);
}
StepResult Layout::step(Operation& op,bool resume) noexcept {
 auto& p=*p_;
 if(op.owner_!=this)return {StepStatus::error,StepError::invalid_operation};
 if(op.result_.status==StepStatus::done || op.result_.status==StepStatus::error)return op.result_;
 if(p.operation!=&op)return {StepStatus::error,StepError::invalid_operation};
 if(op.result_.status==StepStatus::needs_source_wait){
  if(!resume)return op.result_;
  op.result_={StepStatus::more};
 }else if(resume)return {StepStatus::error,StepError::invalid_operation};
 if(p.s.failed())return finish(op,StepError::native_failure);
 using Kind=Operation::Kind;using Phase=Operation::Phase;
 switch(op.phase_){
 case Phase::start:
  if(op.kind_==Kind::hide && p.active.empty())return finish(op);
  if(op.kind_==Kind::create){
   if(p.active_complete && p.active.data()==op.list_.data() && p.active.size()==op.list_.size())return finish(op);
   if(p.active.empty()){
    p.slots.fill({});op.count_=op.split_=op.index_=0;op.cursor_=0;op.phase_=Phase::scan;
    return op.result_;
   }
  }
  op.phase_=Phase::wait_previous;return op.result_;
 case Phase::wait_previous:
  if(p.state!=2){op.result_={StepStatus::needs_source_wait,StepError::none,1};return op.result_;}
  if(op.kind_==Kind::wait)return finish(op);
  op.phase_value_=static_cast<float>(p.frames+1)*detail::button_half_pi;
  p.active_complete=false;
  op.stagger_=static_cast<float>(p.frames*90.0/9.0)*detail::button_pi/180.0f;
  op.cursor_=0;op.phase_=Phase::hide;return op.result_;
 case Phase::hide: {
  if(op.cursor_>=p.active.size() || p.active[op.cursor_].id==35){
   p.state=0;p.active={};
   if(op.kind_==Kind::hide)return finish(op);
   op.phase_=Phase::wait_hidden;return op.result_;
  }
  const auto& b=p.active[op.cursor_++];
  if(b.id<0 || b.id>32)return op.result_;
  auto* v=p.s.view(p.objects[b.id].sprite);
  if(!v)return finish(op,StepError::native_failure);
  v->attr|=ISLE_SP_SCALE;
  if(p.horizontal)v->scalex=1;else v->scaley=1;
  p.base[b.id]=p.horizontal?v->x:v->y;
  if(p.instant)p.phase[b.id]=0;
  else{p.phase[b.id]=op.phase_value_;op.phase_value_-=op.stagger_;}
  return op.result_;
 }
 case Phase::wait_hidden:
  if(p.state!=2){op.result_={StepStatus::needs_source_wait,StepError::none,1};return op.result_;}
  p.slots.fill({});op.count_=op.split_=op.index_=0;op.cursor_=0;op.phase_=Phase::scan;
  return op.result_;
 case Phase::scan:
  if(op.cursor_>=op.list_.size() || op.list_[op.cursor_].id==35){
   op.phase_value_=0;
   op.stagger_=static_cast<float>(p.frames*90.0/9.0)*detail::button_pi/180.0f;
   op.index_=0;op.cursor_=0;op.phase_=Phase::build;return op.result_;
  }else{
   const auto& b=op.list_[op.cursor_++];
   if(b.id>=0 && b.id<=32)++op.count_;
   if(b.id==34)op.split_=op.index_;
   ++op.index_;return op.result_;
  }
 case Phase::build: {
  if(op.cursor_>=op.list_.size() || op.list_[op.cursor_].id==35){
   p.state=1;p.active=op.list_;p.active_complete=true;return finish(op);
  }
  const auto& b=op.list_[op.cursor_++];
  if(b.id==34){op.index_=op.split_-op.count_+8;return op.result_;}
  if(b.id<0 || b.id>32)return op.result_;
  auto* v=p.s.view(p.objects[b.id].sprite);
  if(!v)return finish(op,StepError::native_failure);
  v->attr|=ISLE_SP_SCALE|ISLE_SP_HIDDEN;
  if(p.horizontal)v->scalex=1;else v->scaley=1;
  v->x=0;
  v->y=static_cast<std::int16_t>(25+op.index_*24);
  p.base[b.id]=p.horizontal?v->x:v->y;
  if(p.instant){p.phase[b.id]=detail::button_half_pi;v->attr&=~ISLE_SP_HIDDEN;}
  else{p.phase[b.id]=op.phase_value_;op.phase_value_-=op.stagger_;}
  if(op.index_>=0 && op.index_<8)p.slots[op.index_].id=b.id;
  // Preserve ownership if the C caller cancels before the next build step.
  // An incomplete prefix is not an already-created table, even at equal size.
  p.active=op.list_.first(op.cursor_);p.active_complete=false;
  ++op.index_;return op.result_;
 }
 }
 return finish(op,StepError::invalid_operation);
}
bool Layout::run(Operation& op) noexcept {
 auto result=op.result_;
 for(;;){
  switch(result.status){
  case StepStatus::more:result=step(op);break;
  case StepStatus::needs_source_wait:
   if(!p_->s.wait(result.updates)){cancel(op);return false;}
   result=step(op,true);break;
  case StepStatus::done:return true;
  case StepStatus::event:case StepStatus::error:cancel(op);return false;
  }
 }
}
bool Layout::wait_transition() noexcept {
 Operation op;if(begin_wait_transition(op).status==StepStatus::error)return false;return run(op);
}
bool Layout::hide_buttons() noexcept {
 Operation op;if(begin_hide_buttons(op).status==StepStatus::error)return false;return run(op);
}
bool Layout::create_buttons(std::span<const Button> list) noexcept {
 Operation op;if(begin_create_buttons(op,list).status==StepStatus::error)return false;return run(op);
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
