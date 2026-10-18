#include "beach_ending.hpp"
#include "player_root.hpp"
#include "player_root_source.h"
#include "player_ui.hpp"
#include "player_ui_source.h"
#include "item_owner.hpp"
#include "menu_scene_bridge.h"
#include <exception>
#include <stdexcept>
#include <utility>

namespace isle::beach {
Ending::Ending(scene::Services& services, actor::Owner& actors, Domains domains)
    : services_(services), actors_(actors), domains_(domains) {
    const auto& outro=domains_.cart_resources.outro();
    if (outro.camera_identity!=0x8013c530u ||
        outro.model_animation_table.guest_address!=0x8013b030u ||
        outro.material_animation_table.guest_address!=0x8013baa0u ||
        !domains_.camera.clip(outro.camera_animation,camera_clip_))
        throw std::invalid_argument("ending requires actual Cart outro resources8013B030/BAA0/C530");
    const IsleBeachEndingServices binding{this,validate,release_native,remove_items,
        reset_message,reset_camera,start_cutscene,camera,cart_init,cart_animation,
        cart_retain,cart_callback};
    if (!actors_.bind_ending(binding,domains_.music,domains_.snapshot))
        throw std::invalid_argument("ending requires complete canonical course domain binding before spawn");
}
Ending::~Ending() {
    if (!released_ && !release()) std::terminate();
}
actor::OwnerStatus Ending::release() noexcept {
    return released_?actor::OwnerStatus{}:actors_.release_ending();
}
IsleGameResult Ending::validate(void* context,const IsleBeachEndingSource* source) noexcept {
    if (!context || !source) return ISLE_GAME_INVALID_ARGUMENT;
    auto& self=*static_cast<Ending*>(context);
    auto& player=self.domains_.root.source();
    IslePlayerSource* canonical=nullptr;
    if (self.released_ || self.services_.failed() || source->player!=&player ||
        source->music!=&self.domains_.music || self.domains_.music.player!=&player ||
        &self.actors_.player_source()!=&player || player.game!=self.services_.game ||
        player.native_root!=&self.domains_.root ||
        self.domains_.carts.source()!=player.cart || &self.domains_.ui.source()!=player.ui ||
        isle_player_source_context(self.services_.game,&canonical)!=ISLE_GAME_OK || canonical!=&player)
        return ISLE_GAME_INVALID_STATE;
    return ISLE_GAME_OK;
}
IsleGameResult Ending::release_native(void* context) noexcept {
    auto& self=*static_cast<Ending*>(context);
    if (self.released_ || isle_menu_current_object(self.services_.game)) return ISLE_GAME_INVALID_STATE;
    // A native identity hold prevents camera-address reuse. Its real Services
    // record must still exist until this gate observes native process joins.
    if (self.camera_object_ && !self.services_.source_owner_joined(self.camera_object_))
        return ISLE_GAME_INVALID_STATE;
    if (self.cart_ && !self.services_.source_owner_joined(self.cart_.object()))
        return ISLE_GAME_INVALID_STATE;
    if (self.camera_object_) {
        const auto result=isle_game_object_release_identity(self.services_.game,self.camera_object_);
        if (result!=ISLE_GAME_OK) return result;
        self.camera_object_=nullptr;
    }
    self.cart_={};
    self.released_=true;
    return ISLE_GAME_OK;
}
IsleGameResult Ending::remove_items(void* context) noexcept {
    auto& self=*static_cast<Ending*>(context);
    return self.domains_.items.remove_flying()?ISLE_GAME_OK:ISLE_GAME_EXECUTION_FAILURE;
}
IsleGameResult Ending::reset_message(void* context) noexcept {
    auto& self=*static_cast<Ending*>(context);
    return isle_player_ui_Msg_Reset(&self.domains_.ui.source());
}
IsleGameResult Ending::reset_camera(void* context) noexcept {
    auto& self=*static_cast<Ending*>(context);
    return isle_player_root_invoke_reset_main_camera_settings(&self.domains_.root.source());
}
IsleGameResult Ending::start_cutscene(void* context,IsleGameObject* object) noexcept {
    auto& self=*static_cast<Ending*>(context);
    // Existing C nonwaiting fence catches every checked Source failure before
    // returning through this C++ leaf; no positive wait can cross this frame.
    return isle_player_root_invoke_start_cutscene(&self.domains_.root.source(),object,nullptr,0);
}
IsleGameResult Ending::camera(void* context) noexcept {
    auto& self=*static_cast<Ending*>(context);
    if (self.camera_object_) return ISLE_GAME_INVALID_STATE;
    auto& player=self.domains_.root.source();
    IsleGameObject* object=nullptr;
    auto result=isle_game_camera_object(self.services_.game,player.camera,&object);
    if (result!=ISLE_GAME_OK) return result;
    if (!self.services_.source_owner_registered(object)) return ISLE_GAME_INVALID_STATE;
    if (!self.domains_.camera.set_speed(0.5f) || !self.domains_.camera.set(&self.camera_clip_,0))
        return ISLE_GAME_EXECUTION_FAILURE;
    result=isle_game_object_retain_identity(self.services_.game,object);
    if (result!=ISLE_GAME_OK) return result;
    self.camera_object_=object;
    IsleGameProcess* process=nullptr;
    // SDK camera interpolation is a nonwaiting native service. Its real OM
    // process has source priority1/kind1; all waiting ending work stays C.
    return isle_game_add_process(self.services_.game,object,camera_tick,&self,
        ISLE_GAME_SYNCHRONOUS,1,&process);
}
void Ending::camera_tick(IsleGameObject* object,void* context) noexcept {
    auto& self=*static_cast<Ending*>(context);
    auto& player=self.domains_.root.source();
    IslePlayerSource* canonical=nullptr;
    IsleGameObject* actual=nullptr;
    IsleGameObjectMetadata metadata{};
    if (self.released_ || object!=self.camera_object_ ||
        isle_player_source_context(self.services_.game,&canonical)!=ISLE_GAME_OK || canonical!=&player ||
        player.native_root!=&self.domains_.root ||
        isle_game_camera_object(self.services_.game,player.camera,&actual)!=ISLE_GAME_OK || actual!=object ||
        isle_game_object_metadata(self.services_.game,object,&metadata)!=ISLE_GAME_OK ||
        !self.domains_.camera.update(metadata.flags))
        self.services_.fail("ending native camera update lost its canonical course domain");
}
IsleGameResult Ending::cart_init(void* context,IsleGameObject** output) noexcept {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    *output=nullptr;
    auto& self=*static_cast<Ending*>(context);
    const auto cart=self.domains_.carts.init();
    if (cart) { *output=cart.object; return ISLE_GAME_OK; }
    // Exact original PlayerModel_Init NULL capacity path, not a fallback.
    return cart.status.error==player::cart::OwnerError::capacity?ISLE_GAME_OK:ISLE_GAME_EXECUTION_FAILURE;
}
IsleGameResult Ending::cart_animation(void* context) noexcept {
    auto& carts=static_cast<Ending*>(context)->domains_.carts;
    return carts.set_animation(carts.outro_model(),carts.outro_material(),0,0.5f)?
        ISLE_GAME_OK:ISLE_GAME_EXECUTION_FAILURE;
}
IsleGameResult Ending::cart_retain(void* context,IsleGameObject* object) noexcept {
    auto& self=*static_cast<Ending*>(context);
    if (!object || self.cart_ || self.domains_.carts.source()->cartGObj!=object) return ISLE_GAME_INVALID_STATE;
    auto reference=self.domains_.carts.retain(object);
    if (!reference) return ISLE_GAME_INVALID_STATE;
    self.cart_=std::move(reference);
    return ISLE_GAME_OK;
}
IsleGameResult Ending::cart_callback(void* context,IsleGameObject* object,IsleBeachEndingSource* source) noexcept {
    auto& self=*static_cast<Ending*>(context);
    const auto result=validate(context,source);
    if (result!=ISLE_GAME_OK) return result;
    if (self.cart_.object()!=object) return ISLE_GAME_INVALID_STATE;
    return self.domains_.carts.set_callback(object,isle_beach_ending_cart_event,source)?
        ISLE_GAME_OK:ISLE_GAME_EXECUTION_FAILURE;
}
}
