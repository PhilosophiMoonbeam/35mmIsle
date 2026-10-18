#ifndef ISLE_ACTOR_PHOTO_SCENE_HPP
#define ISLE_ACTOR_PHOTO_SCENE_HPP

#include "actor_owner.hpp"
struct IslePlayerPhotoSource;

namespace isle::actor {
// Original Pokemons_Init collision publication, before Detector_Create.
class CollisionPass final {
public:
    CollisionPass(scene::Services&, Owner&, IsleGameObject* scene_root);
    ~CollisionPass();
    CollisionPass(const CollisionPass&)=delete;
    CollisionPass& operator=(const CollisionPass&)=delete;
    void retire() noexcept;
    bool active() const noexcept;
    IsleGameObject* root() const noexcept { return root_; }
private:
    static void update(IsleGameObject*,void*) noexcept;
    static void retiring(IsleGameObject*,void*) noexcept;
    static void joined(IsleGameObject*,void*) noexcept;
    scene::Services& services_;
    Owner& owner_;
    IsleGameObject* root_=nullptr;
    IsleGameProcess* process_=nullptr;
    bool observed_=false, retiring_=false;
};
// Late Detector prefix/CopyInfo publication. An external early collision pass
// outlives this object; otherwise the controller owns a collision pass itself.
class PhotoScene final {
public:
    PhotoScene(scene::Services&, Owner&, IsleGameObject* scene_root,
               IsleGameCamera*, DetectorBinding&, IslePlayerPhotoSource*, CollisionPass* = nullptr);
    ~PhotoScene();
    PhotoScene(const PhotoScene&) = delete;
    PhotoScene& operator=(const PhotoScene&) = delete;
    PhotoScene(PhotoScene&&) = delete;
    PhotoScene& operator=(PhotoScene&&) = delete;
    void retire() noexcept;
    bool active() const noexcept;
    IsleGameObject* prefix_object() const noexcept { return prefix_; }
    IsleGameObject* copy_info_object() const noexcept { return copy_info_; }
private:
    static IsleGameResult prepare_membership(void*, focus::Photo&) noexcept;
    std::unique_ptr<CollisionPass> owned_collisions_;
    CollisionPass* collision_pass_=nullptr;
    static void copy_info(IsleGameObject*, void*) noexcept;
    static void retiring(IsleGameObject*, void*) noexcept;
    static void joined(IsleGameObject*, void*) noexcept;
    void fail(const char*) noexcept;
    void clear_hook() noexcept;
    scene::Services& services_;
    Owner& owner_;
    IsleGameCamera* camera_;
    DetectorBinding& binding_;
    IslePlayerPhotoSource* photo_ = nullptr;
    bool snapshot_bound_ = false;
    IsleGameObject* root_ = nullptr;
    IsleGameObject* prefix_ = nullptr;
    IsleGameObject* copy_info_ = nullptr;
    bool prefix_observed_ = false, copy_observed_ = false;
    bool prefix_retiring_ = false, copy_retiring_ = false;
    bool hook_installed_ = false, active_ = false, retiring_ = false, retired_ = false;
};
} // namespace isle::actor
#endif
