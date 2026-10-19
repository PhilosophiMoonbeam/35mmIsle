#ifndef ISLE_PHOTO_WORLD_HPP
#define ISLE_PHOTO_WORLD_HPP
#include "world_runtime.hpp"
#include "world_module.hpp"
#include "world_movement_state.h"

namespace isle::photo {
struct WorldRecord {
    std::uint16_t flags = 0;
    float time = 0;
    world::Vec3 eye{};
};
// Cold preparation owns stable callback/animation storage. Explicit publish is
// legal beneath the original Source callback: only foreign native objects are
// created/deleted; current Source identity and live World's idle guards remain.
// Same-course requests call reconstruct, never recreate/publish or resume.
// ModuleState outlives this owner and all native callbacks. Owner must close all
// producer/GPU/storage/semantic borrows before request_clear and clear. Logical
// create/destroy reset roads once, but retain the module clock and speed.
// A failed publish/reconstruct/clear retains all completed progress; error is
// sticky. Destructor does not attempt to delete live callback storage.
class World final {
public:
    World(IsleGame*, const world::Resources&, world::ModuleState&,
          std::uint32_t object_link = 18, std::uint32_t dl_link = 19);
    ~World() noexcept;
    World(const World&) = delete;
    World& operator=(const World&) = delete;
    bool publish() noexcept;
    bool reconstruct(const WorldRecord&) noexcept;
    bool request_clear() noexcept;
    bool clear() noexcept;
    // Reuse admitted CPU resources after real native joins; never resets module.
    bool rearm() noexcept;
    bool available() const noexcept;
    bool cleared() const noexcept;
    const std::string& error() const noexcept;
    world::RenderView render_view() const noexcept;
    std::span<const world::Block> blocks() const noexcept;
    const IsleWorldMovementState& movement() const noexcept;
    // Source thumbnail pauses every actual process on update link18, then17.
    // Never resumes them. Caller invokes once at original thumbnail callback,
    // not for score draws or GPU completion. Includes other actual owners.
    bool pause_thumbnail_processes() noexcept;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace isle::photo
#endif
