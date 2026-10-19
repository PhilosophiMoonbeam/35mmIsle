#ifndef ISLE_VALLEY_ACTOR_NATIVE_HPP
#define ISLE_VALLEY_ACTOR_NATIVE_HPP
#include "valley_actor_source.h"
#include "valley_dialogue_resources.hpp"
#include "menu_effects.hpp"
#include "player_camera_animation_clip.h"
#include "valley_actor_window_mechanics.hpp"
#include <array>
#include <memory>
#include <optional>
#include <vector>
namespace isle::course { class NativeHost; }
namespace isle::world { struct Block; }
struct IsleValleyCamera { isle::course::NativeHost* owner = nullptr; IsleGameCamera* camera = nullptr; };
struct IsleValleyEffect { isle::course::NativeHost* owner = nullptr; isle::menu::EffectHandle handle{}; };
namespace isle::valley {
// Fixed-address entry-owned native identities. No Original mutable scalar is
// mirrored here. Resource and World views are cold, immutable admitted borrows.
class ActorNativeMechanics final {
public:
    ActorNativeMechanics() noexcept;
    ~ActorNativeMechanics();
    ActorNativeMechanics(const ActorNativeMechanics&) = delete;
    ActorNativeMechanics& operator=(const ActorNativeMechanics&) = delete;
    IsleGameResult prepare(const rom::Snapshot&, IsleValleyActorSource&) noexcept;
    IsleGameResult bind(course::NativeHost&) noexcept;
    void clear() noexcept; // Only after actual native/GPU/Photo/reference joins.
    IsleValleyWorldBlock* source_block(world::Block*) noexcept;
    world::Block* native_block(IsleValleyWorldBlock*) noexcept;
    const DialogueResources& resources() const noexcept { return *resources_; }
    WindowMechanics* window() noexcept { return window_ ? &*window_ : nullptr; }
    course::NativeHost* host() const noexcept { return host_; }
    IsleValleyCamera camera;
    std::array<IsleValleyEffect,10> effects{};
    std::array<IslePlayerCameraAnimationClip,3> clips{};
private:
    bool block_views_valid() const noexcept;
    IsleValleyActorSource* source_ = nullptr;
    course::NativeHost* host_ = nullptr;
    std::shared_ptr<const DialogueResources> resources_;
    std::optional<WindowMechanics> window_;
    std::vector<IsleValleyWorldBlockDescriptor> descriptors_;
    std::vector<IsleValleyWorldBlock> blocks_;
};
IsleGameResult actor_native_mechanics(IsleValleyActorSource*,ActorNativeMechanics**) noexcept;
}
#endif
