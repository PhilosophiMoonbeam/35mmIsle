#ifndef ISLE_ACTOR_PHOTO_SCENE_HPP
#define ISLE_ACTOR_PHOTO_SCENE_HPP

#include "actor_owner.hpp"

namespace isle::actor {
// Bounded source focus controller, not a full photo serializer or pair-physics
// manager. The caller proves the source flag200 pair branch inapplicable.
// Native actors own distance8/state7/animation3 processes. This class adds the
// synchronous priority2 collision-position first pass on the actual scene root0.
//
// Services, Owner, its canonical player/main camera and DetectorBinding (whose
// history MUST be the same Detector supplied to Owner) are stable same-scene
// owners. The caller grants EXCLUSIVE ownership of the root's Services client
// ObjectLifecycle slot: it must be empty on entry and must not be changed until
// retire. Engine lifecycle hooks remain Services-owned, never replaced/chained.
// Construction/destruction/retire run outside OM/command callbacks and AFTER
// genuine GPU/readback release; the controller must retire before Owner/Services.
// Services must permit validated all-null lifecycle detachment even after its
// sticky failure; no new observer is installed into failed Services. Unproven
// detachment/joins terminate instead of leaving dangling userdata. No history, photo,
// published focus, player latch or actor identities are eagerly reset here.
class PhotoScene final {
public:
    PhotoScene(scene::Services&, Owner&, IsleGameObject* scene_root,
               IsleGameCamera*, DetectorBinding&);
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
    static void refresh_positions(IsleGameObject*, void*) noexcept;
    static void copy_info(IsleGameObject*, void*) noexcept;
    static void retiring(IsleGameObject*, void*) noexcept;
    static void joined(IsleGameObject*, void*) noexcept;
    void fail(const char*) noexcept;
    void clear_hook() noexcept;
    scene::Services& services_;
    Owner& owner_;
    IsleGameCamera* camera_;
    DetectorBinding& binding_;
    IsleGameObject* root_ = nullptr;
    IsleGameObject* prefix_ = nullptr;
    IsleGameObject* copy_info_ = nullptr;
    IsleGameProcess* positions_ = nullptr;
    bool root_observed_ = false, prefix_observed_ = false, copy_observed_ = false;
    bool root_retiring_ = false, prefix_retiring_ = false, copy_retiring_ = false;
    bool hook_installed_ = false, active_ = false, retiring_ = false, retired_ = false;
};
} // namespace isle::actor
#endif
