#ifndef ISLE_BEACH_ENDING_HPP
#define ISLE_BEACH_ENDING_HPP
#include "actor_owner.hpp"
#include "player_cart.hpp"
#include "player_camera_animation.hpp"

namespace isle::player { class Root; class Ui; }
namespace isle::items { class Owner; }
namespace isle::beach {
/* Concrete late course capability provider. No original state, waits, routing
 * machine or animation clock lives here. All domains outlive this object. */
class Ending final {
public:
    struct Domains {
        player::Root& root;
        player::cart::Owner& carts;
        player::camera_animation::NativeDriver& camera;
        const player::CartResources& cart_resources;
        items::Owner& items;
        player::Ui& ui;
        IsleBeachMusicSource& music;
        const rom::Snapshot& snapshot;
    };
    // After Root.bind, before Root.initialize/spawns. Constructs no native
    // object/process; binds all required returning services to ActorOwner.
    Ending(scene::Services&, actor::Owner&, Domains);
    ~Ending();
    Ending(const Ending&) = delete;
    Ending& operator=(const Ending&) = delete;
    Ending(Ending&&) = delete;
    Ending& operator=(Ending&&) = delete;
    // Outside all native/command/graphics traversal, after actors.reset AND
    // actual outro Cart and main-camera deletion/joins. Before Cart/Renderer
    // source collection and domain destruction. A blocked release retains all
    // callback storage and native/Cart identity holds; it never fakes a join.
    actor::OwnerStatus release() noexcept;
private:
    static IsleGameResult validate(void*, const IsleBeachEndingSource*) noexcept;
    static IsleGameResult release_native(void*) noexcept;
    static IsleGameResult remove_items(void*) noexcept;
    static IsleGameResult reset_message(void*) noexcept;
    static IsleGameResult reset_camera(void*) noexcept;
    static IsleGameResult start_cutscene(void*, IsleGameObject*) noexcept;
    static IsleGameResult camera(void*) noexcept;
    static IsleGameResult cart_init(void*, IsleGameObject**) noexcept;
    static IsleGameResult cart_animation(void*) noexcept;
    static IsleGameResult cart_retain(void*, IsleGameObject*) noexcept;
    static IsleGameResult cart_callback(void*, IsleGameObject*, IsleBeachEndingSource*) noexcept;
    static void camera_tick(IsleGameObject*, void*) noexcept;
    scene::Services& services_;
    actor::Owner& actors_;
    Domains domains_;
    player::cart::Owner::Reference cart_;
    IslePlayerCameraAnimationClip camera_clip_{};
    IsleGameObject* camera_object_ = nullptr;
    bool released_ = false;
};
}
#endif
