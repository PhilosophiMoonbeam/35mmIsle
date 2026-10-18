#ifndef ISLE_PLAYER_ROOT_HPP
#define ISLE_PLAYER_ROOT_HPP
#include "player_root_source.h"
#include "item_owner_source.h"
#include "world_movement.hpp"
#include <array>
#include <optional>

namespace isle::scene { class Services; }
namespace isle::actor { class Owner; }
namespace isle::items { class Owner; }
namespace isle::player {
class Ui;
class UiResources;
class BumpOwner;
class Renderer;
namespace camera_animation { class NativeDriver; }
struct RootAccess;

// One fixed-address source owner. Domains and application callbacks outlive
// native continuations, then their independent graphics/photo/audio borrowers.
// Gameplay state is exclusively Source; this class owns native identities only.
class Root final {
    friend struct RootAccess;
public:
    struct Config {
        std::int32_t level_id;
        IslePlayerEventCallback end_level;
        void* end_level_user;
        IslePlayerEventCallback pause;
        void* pause_user;
        world::MovementCallbacks movement{};
        IsleGameSourceCallback update_items = nullptr;
        std::uint8_t update_items_kind = 0;
        IsleItemCollisionCallback collide_items = nullptr;
    };
    struct Domains {
        world::World& world;
        actor::Owner& actors;
        items::Owner& items;
        Ui& ui;
        BumpOwner& bump;
        Renderer& renderer;
        IslePlayerPhotoSource& photo;
        camera_animation::NativeDriver* camera_animation = nullptr;
    };
    Root(scene::Services&, const UiResources&, Config);
    ~Root();
    Root(const Root&) = delete;
    Root& operator=(const Root&) = delete;
    Root(Root&&) = delete;
    Root& operator=(Root&&) = delete;
    IslePlayerSource& source() noexcept { return source_; }
    const IslePlayerSource& source() const noexcept { return source_; }
    // Cold domain constructors borrow source() before any Root/model publication.
    IsleGameResult bind(Domains) noexcept;
    // Late, one-time association after original createMainCameras, for courses
    // whose cold Actor context must be bound before Source publishes the camera.
    IsleGameResult bind_camera_animation(camera_animation::NativeDriver&) noexcept;
    // Cold association only; the caller first validates this Root's canonical
    // Player/Game and status. Available before original initUI publishes Root.
    // Inline to avoid an ActorOwner -> PlayerRoot static-library dependency.
    actor::Owner* bound_actors() noexcept {
        return !deactivated_ && domains_ ? &domains_->actors : nullptr;
    }
    IsleGameResult initialize() noexcept;
    // Returning accounting after an enclosing original C setup has executed
    // initUI itself. Never replays Source or publishes another Player object.
    IsleGameResult complete_initialization(IsleGameObject*) noexcept;
    // Once per application OM update, after input publication and before ticking.
    // Original idle/pause C subsequently mutates these same canonical scalars.
    IsleGameResult publish_input() noexcept;
    // Outside OM and only after all graphics/readback/source consumers close.
    // Requests actual native deletion/join; incoming semantic credits may remain.
    IsleGameResult retire_objects() noexcept;
    // After dependent receiver retirement closes their FIFO/field credits.
    // Failure retains the joined identities and their canonical Source storage.
    IsleGameResult collect_joined() noexcept;
    // LAST: all domains/commands/renderer and actual callbacks have detached.
    IsleGameResult deactivate() noexcept;
private:
    struct Object {
        Root* owner = nullptr;
        IsleGameObject* identity = nullptr;
        bool registered = false;
        bool joined = false;
    };
    static void joined(IsleGameObject*, void*) noexcept;
    IsleGameResult object(std::uint32_t, IsleGameSourceCallback, std::uint8_t,
                          std::uint32_t, IsleGameObject**) noexcept;
    IsleGameResult prepare_owners(IsleGameObject*, IsleGameModel*) noexcept;
    IsleGameResult ready() const noexcept;
    IsleGameResult camera_ready() const noexcept;
    scene::Services& services_;
    Config config_;
    IslePlayerSource source_{};
    std::optional<Domains> domains_;
    std::optional<world::Movement> movement_;
    std::array<Object, 3> objects_{};
    std::array<IsleGameObject**, 1> fields_{{&source_.gPreviousPokemonInFocus}};
    bool initialized_ = false;
    bool deactivated_ = false;
};
}
#endif
