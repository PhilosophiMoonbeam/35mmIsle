#include "player_ui.hpp"
#include <exception>
namespace isle::player {
namespace {
IsleGameResult result(scene::Services& services,bool ok) noexcept {
 return ok&&!services.failed()?ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
}
}
Ui::Ui(const UiResources& resources,scene::Services& services,IslePlayerSource& player,
       sprite::Producer& producer,std::size_t resource_base)
 :resources_(resources),services_(services),resource_base_(resource_base),producer_(producer) {
 source_.player=&player;source_.owner=this;resources_.initialize(source_);
}
void Ui::joined(IsleGameObject* object,void* user) noexcept {
 auto& self=*static_cast<Ui*>(user);
 auto& source=self.source_;
 for(auto& entry:self.objects_)if(entry==object)entry=nullptr;
 if(source.Msg_CameraObject==object) {
  source.Msg_CameraObject=nullptr;source.Msg_Camera=nullptr;self.draw_count_=0;
 }
 if(source.Icons_MainObject==object) {
  source.Icons_MainObject=nullptr;
  for(auto& icon:source.Icons_IconObjects)icon.spriteObj=nullptr;
  for(auto& sprite:source.Icons_ButtonIcons)sprite=nullptr;
  for(auto& sprite:source.Icons_ButtonIconsCopy)sprite=nullptr;
 }
 if(source.D_803B09D8_550DE8==object) {
  source.D_803B09D8_550DE8=nullptr;
  source.D_803B0A14_550E24=nullptr;source.D_803B0A18_550E28=nullptr;source.D_803B0A1C_550E2C=nullptr;
 }
}
Ui::~Ui() {
 if(retire()!=ISLE_GAME_OK) std::terminate();
 if(source_.player->ui==&source_) source_.player->ui=nullptr;
}
IsleGameResult Ui::object(std::uint32_t id,IsleGameSourceCallback update,IsleGameObject** out) noexcept {
 if(!out) return ISLE_GAME_INVALID_ARGUMENT;
 *out=nullptr;
 auto slot=objects_.end();for(auto it=objects_.begin();it!=objects_.end();++it) if(!*it){slot=it;break;}
 if(slot==objects_.end()) return ISLE_GAME_INVALID_STATE;
 auto* object=services_.object(id,0,0x80000000u);
 if(!object) return ISLE_GAME_EXHAUSTED;
 if(!services_.set_lifecycle(object,{nullptr,joined,this})) {
  services_.erase(object);
  return ISLE_GAME_INVALID_STATE;
 }
 *slot=object;*out=object;
 (void)update; // Actual typed C update is installed by the returning C bridge.
 return result(services_,true);
}
IsleGameResult Ui::render(IsleGameObject* object,std::uint8_t link,std::uint32_t priority) noexcept {
 return result(services_,services_.set_render(object,{link,priority,0xffffffffu}));
}
IsleGameResult Ui::append(IsleGameObject* object,const IsleSprite& view,IsleGameSprite** out) noexcept {
 if(!out)return ISLE_GAME_INVALID_ARGUMENT;
 *out=nullptr;
 try {*out=services_.append_sprite(object,resource_base_+resources_.index(view),view);}
 catch(...){return ISLE_GAME_INVALID_ARGUMENT;}
 return result(services_,*out!=nullptr);
}
IsleGameResult Ui::digit(unsigned which,int value) noexcept {
 if(which>=3 || value<0 || value>9)return ISLE_GAME_INVALID_ARGUMENT;
 IsleGameSprite* handles[]={source_.D_803B0A14_550E24,source_.D_803B0A18_550E28,source_.D_803B0A1C_550E2C};
 auto* current=services_.view(handles[which]);if(!current)return ISLE_GAME_INVALID_STATE;
 try {
  IsleSprite replacement=resources_.digit(unsigned(value));
  replacement.x=current->x;replacement.y=current->y;replacement.scalex=current->scalex;replacement.scaley=current->scaley;
  replacement.attr=current->attr;replacement.red=current->red;replacement.green=current->green;
  replacement.blue=current->blue;replacement.alpha=current->alpha;
  return result(services_,services_.replace_sprite(handles[which],resource_base_+resources_.index(resources_.digit(unsigned(value))),replacement));
 } catch(...){return ISLE_GAME_INVALID_ARGUMENT;}
}
IsleGameResult Ui::glyph(const std::uint8_t* pixels,int left,int top,int right,int bottom,int kind) noexcept {
 if(draw_count_==draws_.size())return ISLE_GAME_EXHAUSTED;
 try {
  const auto index=resources_.glyph_index(pixels);
  auto& view=glyph_views_[draw_count_];view=*resources_.roots()[index];
  view.x=std::int16_t(left);view.y=std::int16_t(top);
  view.scalex=source_.Message_ScaleX;view.scaley=source_.Message_ScaleY;
  view.red=source_.draw_red;view.green=source_.draw_green;view.blue=source_.draw_blue;view.alpha=source_.draw_alpha;
  auto& draw=draws_[draw_count_++];draw={resource_base_+index,&view,0};
  draw.mode=kind?sprite::DrawMode::message_rgba:sprite::DrawMode::message_intensity;
  draw.right=right;draw.bottom=bottom;
  return ISLE_GAME_OK;
 }catch(...){return ISLE_GAME_INVALID_ARGUMENT;}
}
IsleGameResult Ui::message_camera(IsleGameObject** out) noexcept {
 auto r=object(29,nullptr,out);if(r!=ISLE_GAME_OK)return r;
 r=isle_game_attach_camera(services_.game,*out,&source_.Msg_Camera);if(r!=ISLE_GAME_OK)return r;
 // Source message camera has priority3 and tag0, unlike sprite DL entries.
 return result(services_,services_.set_render(*out,{0,3,0}));
}
IsleGameResult Ui::message_draw() noexcept {
 draw_count_=0;
 if(!source_.Msg_CameraObject)return ISLE_GAME_INVALID_STATE;
 return isle_player_ui_Msg_DrawMessage(&source_,source_.Msg_CameraObject);
}
IsleGameResult Ui::erase(IsleGameObject* object) noexcept {
 if(!object)return ISLE_GAME_INVALID_ARGUMENT;
 for(auto* entry:objects_)if(entry==object){services_.erase(object);return result(services_,true);}
 return ISLE_GAME_INVALID_ARGUMENT;
}
IsleGameResult Ui::copy_bitmap(IsleSprite* destination,const IsleSprite* source) noexcept {
 if(!destination||!source)return ISLE_GAME_INVALID_ARGUMENT;
 // Source rsp_dl is only a renderer cache. Native binding has no such cache;
 // replace its immutable bitmap authority in Services rather than patch bytes.
 try {
  auto* current=isle_player_ui_source_find_sprite(&source_,destination);
  if(current)return result(services_,services_.replace_sprite(current,resource_base_+resources_.index(*source),*destination));
 }catch(...){return ISLE_GAME_INVALID_ARGUMENT;}
 return ISLE_GAME_INVALID_ARGUMENT;
}
IsleGameResult Ui::retire() noexcept {
 IsleGameResult status=ISLE_GAME_OK;
 for(std::size_t i=0;i<objects_.size();++i)if(objects_[i]) {
  services_.erase(objects_[i]);
  if(services_.failed() || objects_[i])status=ISLE_GAME_INVALID_STATE;
 }
 return status;
}
}
namespace {
isle::player::Ui* owner(IslePlayerUiSource* source) noexcept {return source?static_cast<isle::player::Ui*>(source->owner):nullptr;}
}
extern "C" {
IsleGameResult isle_player_ui_native_object(IslePlayerUiSource* s,uint32_t id,IsleGameSourceCallback cb,IsleGameObject** out){auto* p=owner(s);return p?p->object(id,cb,out):ISLE_GAME_INVALID_ARGUMENT;}
IsleGameResult isle_player_ui_native_render(IslePlayerUiSource* s,IsleGameObject* o,uint8_t link,uint32_t priority){auto* p=owner(s);return p?p->render(o,link,priority):ISLE_GAME_INVALID_ARGUMENT;}
IsleGameResult isle_player_ui_native_sprite(IslePlayerUiSource* s,IsleGameObject* o,const IsleSprite* v,IsleGameSprite** out){auto* p=owner(s);return p&&v?p->append(o,*v,out):ISLE_GAME_INVALID_ARGUMENT;}
IsleGameResult isle_player_ui_native_digit(IslePlayerUiSource* s,unsigned n,int32_t value){auto* p=owner(s);return p?p->digit(n,value):ISLE_GAME_INVALID_ARGUMENT;}
IsleGameResult isle_player_ui_native_glyph(IslePlayerUiSource* s,const uint8_t* pixels,int32_t l,int32_t t,int32_t r,int32_t b,int kind){auto* p=owner(s);return p?p->glyph(pixels,l,t,r,b,kind):ISLE_GAME_INVALID_ARGUMENT;}
IsleGameResult isle_player_ui_native_message_camera(IslePlayerUiSource* s,IsleGameObject** out){auto* p=owner(s);return p?p->message_camera(out):ISLE_GAME_INVALID_ARGUMENT;}
IsleGameResult isle_player_ui_native_delete(IslePlayerUiSource* s,IsleGameObject* object){auto* p=owner(s);return p?p->erase(object):ISLE_GAME_INVALID_ARGUMENT;}
IsleGameResult isle_player_ui_native_copy_bitmap(IslePlayerUiSource* s,IsleSprite* dst,const IsleSprite* src){auto* p=owner(s);return p?p->copy_bitmap(dst,src):ISLE_GAME_INVALID_ARGUMENT;}
}
