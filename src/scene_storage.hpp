#ifndef ISLE_SCENE_STORAGE_HPP
#define ISLE_SCENE_STORAGE_HPP

#include "35mmisle/engine.h"
#include <cstddef>
#include <memory>

namespace isle::scene {
// Bare source scene owner, also usable before deferred profile initialization.
// One stable backing allocation and arena; no Catalog/State/service/registry or
// rendering/audio policy. Every operation is owner-thread-only, outside native
// traversal and after the caller's actual GPU/readback/producer borrow fence.
class Storage final {
public:
    explicit Storage(std::size_t arena_bytes = 8 * 1024 * 1024);
    // Calls joined destroy. A failed join cannot release backing storage; callers
    // must retain the owner after an explicit destroy failure. Destruction under
    // an unresolved join/owner violation terminates rather than free live memory.
    ~Storage();
    Storage(const Storage&) = delete;
    Storage& operator=(const Storage&) = delete;
    Storage(Storage&&) = delete;
    Storage& operator=(Storage&&) = delete;
    IsleGame* game() const noexcept;
    isle_arena& arena() noexcept;
    std::size_t capacity() const noexcept;
    // create requires no live game; reset requires a successfully destroyed game.
    // destroy nulls its handle only after actual native process-domain joins.
    // reenter is exactly joined destroy -> arena reset -> native create; it never
    // reallocates backing storage, opens a profile, or creates concrete Services.
    IsleGameResult create() noexcept;
    IsleGameResult destroy() noexcept;
    IsleGameResult reset() noexcept;
    IsleGameResult reenter() noexcept;
private:
    std::unique_ptr<std::byte[]> bytes_;
    std::size_t capacity_;
    isle_arena arena_{};
    IsleGame* game_ = nullptr;
};
} // namespace isle::scene
#endif
