#include "actor_photo_scene.hpp"
#include "actor_photo_membership.hpp"
#include "menu_scene_bridge.h"
#include "player_photo.hpp"

#include <exception>
#include <stdexcept>

// Own GAME df8815e75f3d79618466e01e0827c6165b93d11c:
// pokemon_detect.c121-178, pokemon.c2261-2265/2344, enums.h248-278.
namespace isle::actor {
namespace {
constexpr std::uint8_t source_player_link = 9; // Own enums.h LINK_PLAYER.
constexpr scene::RenderMetadata source_prefix{5,0x80000001u,UINT32_MAX};
void require(bool value, const char* detail) {
    if (!value) throw std::invalid_argument(detail);
}
}
PhotoScene::PhotoScene(scene::Services& services, Owner& owner, IsleGameObject* root,
                       IsleGameCamera* camera, DetectorBinding& binding, IslePlayerPhotoSource* photo)
    : services_(services), owner_(owner), camera_(camera), binding_(binding), photo_(photo) {
    require(!services.failed() && services.game && root && camera,
            "PhotoScene requires live same-scene owners");
    require(!photo_ || photo_->owner, "PhotoScene requires an active canonical photo transaction owner");
    require(!isle_menu_current_object(services.game), "PhotoScene construction inside native callback");
    require(!binding.prefix_user && !binding.prepare_membership,
            "PhotoScene membership hook already owned");
    require(binding.context<2, "PhotoScene detector context is invalid");
    IsleGameObjectMetadata metadata{};
    require(isle_game_object_metadata(services.game,root,&metadata)==ISLE_GAME_OK &&
            metadata.id==0 && metadata.update_link==0 && metadata.priority==0x80000000u,
            "PhotoScene requires the actual scene root0/link0/priority80000000");
    // The source finds root0 before creating the higher-priority CopyInfo GObj.
    auto* first=isle_menu_first_object(services.game,0);
    while (first) {
        std::uint32_t id=0;
        require(isle_game_object_id(services.game,first,&id)==ISLE_GAME_OK,
                "PhotoScene root search rejected native ownership");
        if (id==0) break;
        first=isle_menu_next_object(services.game,first);
    }
    require(first==root, "PhotoScene supplied root is not source link0's first id0");
    const auto& player=owner.player();
    const IsleGameCameraParameters* parameters=nullptr;
    require(player.camera==camera &&
            isle_game_model_validate_binding(services.game,player.object,player.model)==ISLE_GAME_OK &&
            isle_game_camera_parameters(services.game,camera,&parameters)==ISLE_GAME_OK,
            "PhotoScene requires Owner's actual canonical player/main camera");
    require(isle_game_object_metadata(services.game,owner.particle_object(),&metadata)==ISLE_GAME_OK &&
            isle_game_object_metadata(services.game,owner.effect_object(),&metadata)==ISLE_GAME_OK,
            "PhotoScene Owner is not live in the supplied native game");
    try {
        prefix_=services.object(0,source_player_link,0x80000000u);
        if (!prefix_) throw std::runtime_error("PhotoScene native prefix creation failed");
        if (!services.set_lifecycle(prefix_,{retiring,joined,this}))
            throw std::runtime_error("PhotoScene prefix lifecycle installation failed");
        prefix_observed_=true;
        if (!services.set_update(prefix_,nullptr,nullptr) || !services.set_render(prefix_,source_prefix))
            throw std::runtime_error("PhotoScene source prefix metadata installation failed");
        if (photo_) {
            if (static_cast<::isle::player::Photo*>(photo_->owner)->bind_snapshot_owner(services_,prefix_)!=ISLE_GAME_OK)
                throw std::runtime_error("PhotoScene canonical snapshot source ownership registration failed");
            snapshot_bound_=true;
        }
        copy_info_=services.object(0,0,0x80000001u);
        if (!copy_info_) throw std::runtime_error("PhotoScene native CopyInfo creation failed");
        if (!services.set_lifecycle(copy_info_,{retiring,joined,this}))
            throw std::runtime_error("PhotoScene CopyInfo lifecycle installation failed");
        copy_observed_=true;
        if (!services.set_update(copy_info_,copy_info,this))
            throw std::runtime_error("PhotoScene source CopyInfo update installation failed");
        collisions_=services.process(root,update_collisions,this,ISLE_GAME_SYNCHRONOUS,2);
        if (!collisions_) throw std::runtime_error("PhotoScene root priority2 process creation failed");
        // Last fallible installation. No failure after successful assignment
        // can escape construction with a borrowed root observer still installed.
        if (!services.set_lifecycle(root,{retiring,joined,this}))
            throw std::runtime_error("PhotoScene exclusive root lifecycle installation failed");
        root_=root; root_observed_=true;
        binding.prefix_user=this; binding.prepare_membership=prepare_membership;
        hook_installed_=true; active_=true;
    } catch (...) {
        retire();
        throw;
    }
}
PhotoScene::~PhotoScene() { retire(); }
bool PhotoScene::active() const noexcept {
    return active_ && !services_.failed() && !retired_ && root_ && prefix_ && copy_info_ &&
        binding_.prefix_user==this && binding_.prepare_membership==prepare_membership;
}
void PhotoScene::clear_hook() noexcept {
    if (!hook_installed_) return;
    if (binding_.prefix_user==this && binding_.prepare_membership==prepare_membership) {
        binding_.prepare_membership=nullptr;
        binding_.prefix_user=nullptr;
    } else services_.fail("PhotoScene lost exclusive membership hook ownership");
    hook_installed_=false;
}
void PhotoScene::fail(const char* detail) noexcept {
    active_=false;
    clear_hook();
    services_.fail(detail);
}
IsleGameResult PhotoScene::prepare_membership(void* user, focus::Photo& output) noexcept {
    if (!user) return ISLE_GAME_INVALID_ARGUMENT;
    auto& self=*static_cast<PhotoScene*>(user);
    if (!self.active() || self.owner_.player().camera!=self.camera_) {
        self.fail("PhotoScene prefix preparation outside its live canonical camera scope");
        return ISLE_GAME_INVALID_STATE;
    }
    // Native flags may have changed before a later visited-node error. The
    // collector preserves output on error; Producer must abort that draw.
    const auto result=photo::collect_membership(self.services_.game,self.camera_,output);
    if (result!=ISLE_GAME_OK) self.fail("PhotoScene native membership collection failed");
    return result;
}
void PhotoScene::update_collisions(IsleGameObject* object, void* user) noexcept {
    auto& self=*static_cast<PhotoScene*>(user);
    if (!self.active()) {
        if (self.active_) self.fail("PhotoScene priority2 process lost live scope");
        return;
    }
    if (object!=self.root_) { self.fail("PhotoScene collision process is not on root0"); return; }
    const auto result=self.owner_.update_collisions();
    if (!result) self.fail(result.detail);
}
void PhotoScene::copy_info(IsleGameObject* object, void* user) noexcept {
    auto& self=*static_cast<PhotoScene*>(user);
    if (!self.active()) {
        if (self.active_) self.fail("PhotoScene CopyInfo update lost live scope");
        return;
    }
    if (object!=self.copy_info_) { self.fail("PhotoScene CopyInfo native identity mismatch"); return; }
    if (self.photo_) {
        if (isle_player_photo_publish_completed(self.photo_)!=ISLE_GAME_OK)
            self.fail("PhotoScene CopyInfo rejected incomplete or mismatched photo readback");
    } else self.binding_.history.publish();
}
void PhotoScene::retiring(IsleGameObject* object, void* user) noexcept {
    auto& self=*static_cast<PhotoScene*>(user);
    if (object==self.root_) self.root_retiring_=true;
    else if (object==self.prefix_) self.prefix_retiring_=true;
    else if (object==self.copy_info_) self.copy_retiring_=true;
    else std::terminate();
    if (!self.retiring_) self.fail("PhotoScene native owner retired outside controller teardown");
}
void PhotoScene::joined(IsleGameObject* object, void* user) noexcept {
    auto& self=*static_cast<PhotoScene*>(user);
    if (object==self.root_) {
        self.root_=nullptr; self.collisions_=nullptr; self.root_observed_=false;
    } else if (object==self.prefix_) {
        self.prefix_=nullptr; self.prefix_observed_=false;
    } else if (object==self.copy_info_) {
        self.copy_info_=nullptr; self.copy_observed_=false;
    } else std::terminate();
}
void PhotoScene::retire() noexcept {
    if (retired_) return;
    if (isle_menu_current_object(services_.game)) std::terminate();
    if (snapshot_bound_ && !static_cast<::isle::player::Photo*>(photo_->owner)->quiescent()) std::terminate();
    retiring_=true; active_=false;
    clear_hook();
    // Only joined callbacks prove a previously retiring object cannot resume.
    if ((root_ && root_retiring_) || (prefix_ && prefix_retiring_) ||
        (copy_info_ && copy_retiring_)) std::terminate();
    if (root_observed_) {
        if (!services_.set_lifecycle(root_,{})) std::terminate();
        root_observed_=false;
    }
    if (collisions_) {
        if (isle_game_end_process(services_.game,collisions_)!=ISLE_GAME_OK) std::terminate();
        collisions_=nullptr;
    }
    const auto erase = [this](IsleGameObject*& object, bool observed) {
        if (!object) return;
        if (isle_game_delete_object(services_.game,object)!=ISLE_GAME_OK) std::terminate();
        if (observed && object) std::terminate();
        object=nullptr;
    };
    erase(copy_info_,copy_observed_);
    erase(prefix_,prefix_observed_);
    if (snapshot_bound_) {
        if (static_cast<::isle::player::Photo*>(photo_->owner)->detach_snapshot_owner()!=ISLE_GAME_OK)
            std::terminate();
        snapshot_bound_=false;
    }
    root_=nullptr;
    retired_=true;
}
} // namespace isle::actor
