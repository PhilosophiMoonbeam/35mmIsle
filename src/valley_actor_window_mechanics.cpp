#include "valley_actor_window_mechanics.hpp"
#include "valley_actor_window_fence.h"
#include "valley_actor_native.hpp"
#include "course_native_host.hpp"
#include "player_ui.hpp"
#include "scene_services.hpp"
#include "screen_transition_source.h"
#include <cmath>
#include <limits>

namespace isle::valley {
WindowMechanics::WindowMechanics(scene::Services& services,player::Ui& ui,
    std::span<const scene::SpriteResource> resources,std::size_t base) noexcept
    :services_(services),ui_(ui),resources_(resources),base_(base) {}
IsleGameResult WindowMechanics::ready() const noexcept {
    if (services_.failed()) return ISLE_GAME_INVALID_STATE;
    auto& ui=ui_.source();
    if (!ui.player || ui.player->game!=services_.game || ui.player->ui!=&ui)
        return ISLE_GAME_INVALID_STATE;
    return ui.status;
}
IsleGameResult WindowMechanics::message_reset() noexcept {
    auto result=ready();
    return result==ISLE_GAME_OK?isle_player_ui_Msg_Reset(&ui_.source()):result;
}
IsleGameResult WindowMechanics::message_interactive(bool enabled) noexcept {
    auto result=ready();
    if (result!=ISLE_GAME_OK) return result;
    return enabled?isle_player_ui_Msg_SetInteractive(&ui_.source()):
        isle_player_ui_Msg_SetNonInteractive(&ui_.source());
}
IsleGameResult WindowMechanics::message_printed(bool* output) noexcept {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    *output=false;
    const auto result=ready();
    if (result==ISLE_GAME_OK) *output=isle_player_ui_Msg_IsMessagePrinted(&ui_.source())!=0;
    return result;
}
IsleGameResult WindowMechanics::message_handler(IsleValleyActorSource* source,
    IsleValleyMessageCallback callback) noexcept {
    const auto result=ready();
    if (result!=ISLE_GAME_OK) return result;
    if (!source || source->base.nativeGame!=services_.game ||
        (callback && callback!=isle_valley_actor_message_event)) return ISLE_GAME_INVALID_ARGUMENT;
    const auto installed=isle_valley_window_install_handler(&ui_.source(),callback!=nullptr);
    if (installed==ISLE_GAME_OK) {
        handler_source_=callback?source:nullptr;
        handler_=callback;
    }
    return installed;
}
IsleGameResult WindowMechanics::message_show(IsleValleyActorSource* source,const char* text,
    std::int32_t x,std::int32_t y,std::uint8_t r,std::uint8_t g,std::uint8_t b,
    std::uint8_t a,std::int32_t delay,std::uint8_t alignment) noexcept {
    const auto result=ready();
    if (result!=ISLE_GAME_OK) return result;
    if (!text || !source || source->base.nativeGame!=services_.game ||
        (handler_ && handler_source_!=source)) return ISLE_GAME_INVALID_ARGUMENT;
    // The Original copies bytes to these real canonical 256-byte buffers, then
    // its Msg_UpdatePrinter process owns every wait, input skip and completion.
    std::size_t length=0;
    while (length<sizeof(ui_.source().Msg_MessageText) && text[length]) ++length;
    if (length==sizeof(ui_.source().Msg_MessageText)) return ISLE_GAME_INVALID_ARGUMENT;
    if (!ui_.source().Msg_CameraObject || !ui_.source().Msg_Camera)
        return ISLE_GAME_INVALID_STATE;
    return isle_valley_window_invoke_show(source,&ui_.source(),handler_,text,x,y,r,g,b,a,delay,alignment);
}
std::size_t WindowMechanics::resource_index(const IsleSprite& view) const noexcept {
    for (std::size_t i=0;i<resources_.size();++i) {
        const auto* root=resources_[i].sprite.view();
        if (root && view.bitmap==root->bitmap && view.LUT==root->LUT &&
            view.LUT_bytes==root->LUT_bytes && view.nbitmaps==root->nbitmaps &&
            view.width==root->width && view.height==root->height &&
            view.bmfmt==root->bmfmt && view.bmsiz==root->bmsiz &&
            view.bmheight==root->bmheight && view.bmHreal==root->bmHreal &&
            view.istart==root->istart && view.istep==root->istep &&
            view.startTLUT==root->startTLUT && view.nTLUT==root->nTLUT) return i;
    }
    return resources_.size();
}
IsleGameResult WindowMechanics::sprite_add(IsleGameObject* object,
    const IsleSprite* view,IsleGameSprite** output) noexcept {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    *output=nullptr;
    const auto result=ready();
    if (result!=ISLE_GAME_OK) return result;
    if (!object) return ISLE_GAME_INVALID_ARGUMENT;
    if (!view) *output=services_.append_empty_sprite(object);
    else {
        const auto index=resource_index(*view);
        if (index==resources_.size()) return ISLE_GAME_INVALID_ARGUMENT;
        *output=services_.append_sprite(object,base_+index,*view);
    }
    return *output&&!services_.failed()?ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
}
IsleGameResult WindowMechanics::rebind(IsleSprite* view,IsleGameSprite** output) noexcept {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    *output=nullptr;
    const auto result=ready();
    if (result!=ISLE_GAME_OK) return result;
    if (!view) return ISLE_GAME_INVALID_ARGUMENT;
    auto* sprite=services_.sprite_from_view(view);
    if (!sprite) return ISLE_GAME_INVALID_ARGUMENT;
    const auto index=resource_index(*view);
    if (index==resources_.size()) return ISLE_GAME_INVALID_ARGUMENT;
    // Original writes complete descriptors directly before spMove/spX2Move.
    // Preserve those bytes while replacing the actual native Producer binding.
    if (!services_.replace_sprite(sprite,base_+index,*view)) return ISLE_GAME_INVALID_STATE;
    *output=sprite;
    return ISLE_GAME_OK;
}
IsleGameResult WindowMechanics::sprite_move(IsleSprite* view,std::int32_t x,std::int32_t y) noexcept {
    IsleGameSprite* sprite=nullptr;
    const auto result=rebind(view,&sprite);
    if (result==ISLE_GAME_OK) isle_sprite_move(view,x,y);
    return result;
}
IsleGameResult WindowMechanics::sprite_scale(IsleSprite* view,float x,float y) noexcept {
    IsleGameSprite* sprite=nullptr;
    const auto result=rebind(view,&sprite);
    if (result==ISLE_GAME_OK) isle_sprite_scale(view,x,y);
    return result;
}
IsleGameResult WindowMechanics::sprite_color(IsleSprite* view,std::uint8_t r,
    std::uint8_t g,std::uint8_t b,std::uint8_t a) noexcept {
    IsleGameSprite* sprite=nullptr;
    const auto result=rebind(view,&sprite);
    if (result==ISLE_GAME_OK) isle_sprite_color(view,r,g,b,a);
    return result;
}
IsleGameResult WindowMechanics::sprite_link(IsleGameObject* object,std::uint8_t link,
    std::uint32_t priority,std::int32_t camera_tag) noexcept {
    const auto result=ready();
    if (result!=ISLE_GAME_OK) return result;
    return services_.set_render(object,{link,priority,static_cast<std::uint32_t>(camera_tag)})?
        ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
}
IsleGameResult WindowMechanics::player_name(const char** output) noexcept {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    *output=nullptr;
    const auto result=ready();
    if (result!=ISLE_GAME_OK) return result;
    // Borrow actual canonical Save RAM. Original immediately copies20 bytes
    // without waiting, and writes its own canonical BSS terminator.
    const auto bytes=services_.save.source_player_name_prefix();
    if (bytes.size()!=20) return ISLE_GAME_INVALID_STATE;
    *output=reinterpret_cast<const char*>(bytes.data());
    return ISLE_GAME_OK;
}
}
namespace {
using isle::valley::ActorNativeMechanics;
using isle::valley::WindowMechanics;
IsleGameResult mechanics(IsleValleyActorSource* source,WindowMechanics** output) noexcept {
    *output=nullptr;
    ActorNativeMechanics* owner=nullptr;
    const auto result=isle::valley::actor_native_mechanics(source,&owner);
    if (result!=ISLE_GAME_OK) return result;
    if (!owner || !owner->window()) return ISLE_GAME_INVALID_STATE;
    *output=owner->window();
    return ISLE_GAME_OK;
}
template<class F> IsleGameResult window(IsleValleyActorSource* source,F function) noexcept {
    WindowMechanics* value=nullptr;
    const auto result=mechanics(source,&value);
    return result==ISLE_GAME_OK?function(*value):result;
}
IsleGameResult host(IsleValleyActorSource* source,isle::course::NativeHost** output) noexcept {
    *output=nullptr;
    ActorNativeMechanics* owner=nullptr;
    const auto result=isle::valley::actor_native_mechanics(source,&owner);
    if (result!=ISLE_GAME_OK) return result;
    if (!owner || !owner->host()) return ISLE_GAME_INVALID_STATE;
    *output=owner->host();return ISLE_GAME_OK;
}
}
IsleGameResult isle_valley_window_message_reset(IsleValleyActorSource* s) noexcept {return window(s,[](auto& w){return w.message_reset();});}
IsleGameResult isle_valley_window_message_interactive(IsleValleyActorSource* s) noexcept {return window(s,[](auto& w){return w.message_interactive(true);});}
IsleGameResult isle_valley_window_message_noninteractive(IsleValleyActorSource* s) noexcept {return window(s,[](auto& w){return w.message_interactive(false);});}
IsleGameResult isle_valley_window_message_printed(IsleValleyActorSource* s,bool* out) noexcept {if(!out)return ISLE_GAME_INVALID_ARGUMENT;*out=false;return window(s,[&](auto& w){return w.message_printed(out);});}
IsleGameResult isle_valley_window_message_show(IsleValleyActorSource* s,const char* t,int32_t x,int32_t y,uint8_t r,uint8_t g,uint8_t b,uint8_t a,int32_t d,uint8_t align) noexcept {return window(s,[&](auto& w){return w.message_show(s,t,x,y,r,g,b,a,d,align);});}
IsleGameResult isle_valley_window_message_handler(IsleValleyActorSource* s,IsleValleyMessageCallback cb) noexcept {return window(s,[&](auto& w){return w.message_handler(s,cb);});}
IsleGameResult isle_valley_window_sprite_add(IsleValleyActorSource* s,IsleGameObject* o,const IsleSprite* v,IsleGameSprite** out) noexcept {if(!out)return ISLE_GAME_INVALID_ARGUMENT;*out=nullptr;return window(s,[&](auto& w){return w.sprite_add(o,v,out);});}
IsleGameResult isle_valley_window_sprite_move(IsleValleyActorSource* s,IsleSprite* v,int32_t x,int32_t y) noexcept {return window(s,[&](auto& w){return w.sprite_move(v,x,y);});}
// Pinned PR/sp.h F3DEX_GBI_2 aliases spMove to spX2Move; not 2x coordinates.
IsleGameResult isle_valley_window_sprite_x2_move(IsleValleyActorSource* s,IsleSprite* v,int32_t x,int32_t y) noexcept {return isle_valley_window_sprite_move(s,v,x,y);}
IsleGameResult isle_valley_window_sprite_scale(IsleValleyActorSource* s,IsleSprite* v,float x,float y) noexcept {return window(s,[&](auto& w){return w.sprite_scale(v,x,y);});}
IsleGameResult isle_valley_window_sprite_color(IsleValleyActorSource* s,IsleSprite* v,uint8_t r,uint8_t g,uint8_t b,uint8_t a) noexcept {return window(s,[&](auto& w){return w.sprite_color(v,r,g,b,a);});}
IsleGameResult isle_valley_window_sprite_link(IsleValleyActorSource* s,IsleGameObject* o,uint8_t l,uint32_t p,int32_t tag) noexcept {return window(s,[&](auto& w){return w.sprite_link(o,l,p,tag);});}
IsleGameResult isle_valley_window_player_name(IsleValleyActorSource* s,const char** out) noexcept {if(!out)return ISLE_GAME_INVALID_ARGUMENT;*out=nullptr;return window(s,[&](auto& w){return w.player_name(out);});}
IsleGameResult isle_valley_window_check_player_flag(IsleValleyActorSource* s,int32_t id,int32_t* out) noexcept {
    if(!out)return ISLE_GAME_INVALID_ARGUMENT;
    *out=0;
    isle::course::NativeHost* h=nullptr;const auto result=host(s,&h);
    if(result!=ISLE_GAME_OK)return result;
    if(id<0)return ISLE_GAME_INVALID_ARGUMENT;
    try {const auto value=h->services()->save.player_flag(static_cast<unsigned>(id));
        if(!value)return ISLE_GAME_INVALID_ARGUMENT;
        *out=value.value?1:0;return ISLE_GAME_OK;
    }catch(...){return ISLE_GAME_INVALID_STATE;}
}
IsleGameResult isle_valley_window_set_player_flag(IsleValleyActorSource* s,int32_t id,int32_t value) noexcept {
    isle::course::NativeHost* h=nullptr;const auto result=host(s,&h);
    if(result!=ISLE_GAME_OK)return result;
    if(id<0)return ISLE_GAME_INVALID_ARGUMENT;
    try{return h->services()->save.set_player_flag(static_cast<unsigned>(id),value!=0)?ISLE_GAME_INVALID_STATE:ISLE_GAME_OK;}
    catch(...){return ISLE_GAME_INVALID_STATE;}
}
IsleGameResult isle_valley_window_application_notice(IsleValleyActorSource* s,int32_t bits) noexcept {
    isle::course::NativeHost* h=nullptr;const auto result=host(s,&h);
    if(result!=ISLE_GAME_OK)return result;
    h->game_state().add_scene_notices(static_cast<std::uint32_t>(bits));return ISLE_GAME_OK;
}
IsleGameResult isle_valley_window_transition_begin(IsleValleyActorSource* s,int32_t link,int32_t mode) noexcept {
    isle::course::NativeHost* h=nullptr;const auto result=host(s,&h);
    return result==ISLE_GAME_OK?isle_screen_transition_func_800A7F68(h->services()->screen_transition_source(),link,mode):result;
}
IsleGameResult isle_valley_window_transition_color(IsleValleyActorSource* s,float x,float y,float kind,float duration) noexcept {
    isle::course::NativeHost* h=nullptr;const auto result=host(s,&h);
    if(result!=ISLE_GAME_OK)return result;
    // Source ABI's third argument is s32, despite the course veneer float POD.
    if(!std::isfinite(kind) || double(kind)<std::numeric_limits<std::int32_t>::min() ||
        double(kind)>std::numeric_limits<std::int32_t>::max()) return ISLE_GAME_INVALID_ARGUMENT;
    return isle_screen_transition_func_800A7F40(h->services()->screen_transition_source(),x,y,static_cast<std::int32_t>(kind),duration);
}
IsleGameResult isle_valley_window_transition_busy(IsleValleyActorSource* s,int32_t* out) noexcept {
    if(!out)return ISLE_GAME_INVALID_ARGUMENT;
    *out=0;
    isle::course::NativeHost* h=nullptr;const auto result=host(s,&h);
    if(result!=ISLE_GAME_OK)return result;
    auto* screen=h->services()->screen_transition_source();
    if(!screen || screen->status!=ISLE_GAME_OK)return screen?screen->status:ISLE_GAME_INVALID_STATE;
    *out=isle_screen_transition_func_800A7460(screen);return ISLE_GAME_OK;
}
