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
CollisionPass::CollisionPass(scene::Services& services, Owner& owner, IsleGameObject* root)
    :services_(services),owner_(owner),root_(root) {
    IsleGameObjectMetadata metadata{};
    require(!isle_menu_current_object(services.game) &&
        isle_game_object_metadata(services.game,root,&metadata)==ISLE_GAME_OK &&
        metadata.id==0 && metadata.update_link==0 && metadata.priority==0x80000000u,
        "collision pass requires genuine helper0 outside traversal");
    require(services.observe_lifecycle(root,{retiring,joined,this}),
        "collision helper secondary observer already owned");
    observed_=true;
    process_=services.process(root,update,this,ISLE_GAME_SYNCHRONOUS,2);
    if (!process_) {
        if (!services.unobserve_lifecycle(root,this)) std::terminate();
        observed_=false;
        throw std::runtime_error("original priority2 collision publication failed");
    }
}
CollisionPass::~CollisionPass() { retire(); }
bool CollisionPass::active() const noexcept { return root_ && process_ && !retiring_ && !services_.failed(); }
void CollisionPass::update(IsleGameObject* object,void* user) noexcept {
    auto& self=*static_cast<CollisionPass*>(user);
    if (!self.active() || object!=self.root_) {
        self.services_.fail("collision pass lost actual helper0 scope"); return;
    }
    const auto result=self.owner_.update_collisions();
    if (!result) self.services_.fail(result.detail);
}
void CollisionPass::retiring(IsleGameObject* object,void* user) noexcept {
    auto& self=*static_cast<CollisionPass*>(user);
    if (object!=self.root_) std::terminate();
    self.retiring_=true;
}
void CollisionPass::joined(IsleGameObject* object,void* user) noexcept {
    auto& self=*static_cast<CollisionPass*>(user);
    if (object!=self.root_) std::terminate();
    self.root_=nullptr; self.process_=nullptr; self.observed_=false;
}
void CollisionPass::retire() noexcept {
    if (isle_menu_current_object(services_.game)) std::terminate();
    if (observed_) {
        if (retiring_ || !services_.unobserve_lifecycle(root_,this)) std::terminate();
        observed_=false;
    }
    if (process_ && isle_game_end_process(services_.game,process_)!=ISLE_GAME_OK) std::terminate();
    process_=nullptr; root_=nullptr;
}
PhotoScene::PhotoScene(scene::Services& services, Owner& owner, IsleGameObject* root,
                       IsleGameCamera* camera, DetectorBinding& binding, IslePlayerPhotoSource* photo,
                       CollisionPass* collisions)
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
    const auto& player=owner.player_source();
    IslePlayerSource* active=nullptr;
    const IsleGameCameraParameters* parameters=nullptr;
    require(isle_player_source_context(services.game,&active)==ISLE_GAME_OK && active==&player &&
            player.camera==camera &&
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
        if (collisions) {
            require(collisions->active() && collisions->root()==root,
                "Detector requires the original early collision pass");
            collision_pass_=collisions;
        } else {
            owned_collisions_=std::make_unique<CollisionPass>(services,owner,root);
            collision_pass_=owned_collisions_.get();
        }
        root_=root;
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
        collision_pass_ && collision_pass_->active() &&
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
    IslePlayerSource* active=nullptr;
    if (!self.active() || isle_player_source_context(self.services_.game,&active)!=ISLE_GAME_OK ||
        active!=&self.owner_.player_source() || self.owner_.player_source().camera!=self.camera_) {
        self.fail("PhotoScene prefix preparation outside its live canonical camera scope");
        return ISLE_GAME_INVALID_STATE;
    }
    // Native flags may have changed before a later visited-node error. The
    // collector preserves output on error; Producer must abort that draw.
    const auto result=photo::collect_membership(self.services_.game,self.camera_,output);
    if (result!=ISLE_GAME_OK) self.fail("PhotoScene native membership collection failed");
    return result;
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
    if (object==self.prefix_) self.prefix_retiring_=true;
    else if (object==self.copy_info_) self.copy_retiring_=true;
    else std::terminate();
    if (!self.retiring_) self.fail("PhotoScene native owner retired outside controller teardown");
}
void PhotoScene::joined(IsleGameObject* object, void* user) noexcept {
    auto& self=*static_cast<PhotoScene*>(user);
    if (object==self.prefix_) {
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
    if ((prefix_ && prefix_retiring_) || (copy_info_ && copy_retiring_)) std::terminate();
    owned_collisions_.reset(); collision_pass_=nullptr;
    const auto erase = [this](IsleGameObject*& object, bool observed) {
        if (!object) return;
        auto* identity=object;
        const auto prior_fault=isle_game_status(services_.game);
        const auto result=isle_game_delete_object(services_.game,identity);
        // Deletion returns the retained Game fault even after a genuine join.
        // Never clear it or confuse it with proof that deletion was skipped.
        if (result!=ISLE_GAME_OK && result!=prior_fault) std::terminate();
        if (observed) {
            if (object) std::terminate();
        } else {
            IsleGameObjectMetadata metadata{};
            if (isle_game_object_metadata(services_.game,identity,&metadata)!=ISLE_GAME_INVALID_ARGUMENT)
                std::terminate();
        }
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
