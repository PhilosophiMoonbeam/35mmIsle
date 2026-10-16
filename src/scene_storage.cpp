#include "scene_storage.hpp"

#include <exception>
#include <stdexcept>

namespace isle::scene {
Storage::Storage(std::size_t size) : capacity_(size) {
    if (size == 0) throw std::invalid_argument("scene Storage requires a nonempty arena");
    bytes_ = std::make_unique_for_overwrite<std::byte[]>(size);
    if (isle_arena_init(&arena_,bytes_.get(),size) != ISLE_OK)
        throw std::invalid_argument("scene Storage arena initialization failed");
}
Storage::~Storage() {
    // Never release backing memory under unjoined native callbacks. Callers can
    // inspect an explicit destroy() failure while this owner is still retained.
    if (destroy() != ISLE_GAME_OK) std::terminate();
}
IsleGame* Storage::game() const noexcept { return game_; }
isle_arena& Storage::arena() noexcept { return arena_; }
std::size_t Storage::capacity() const noexcept { return capacity_; }
IsleGameResult Storage::create() noexcept {
    if (game_ != nullptr) return ISLE_GAME_INVALID_STATE;
    return isle_game_create(&arena_,&game_);
}
IsleGameResult Storage::destroy() noexcept {
    if (game_ == nullptr) return ISLE_GAME_OK;
    const auto result = isle_game_destroy(game_);
    if (result == ISLE_GAME_OK) game_ = nullptr;
    return result;
}
IsleGameResult Storage::reset() noexcept {
    if (game_ != nullptr) return ISLE_GAME_INVALID_STATE;
    return isle_arena_reset(&arena_) == ISLE_OK ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE;
}
IsleGameResult Storage::reenter() noexcept {
    auto result = destroy();
    if (result != ISLE_GAME_OK) return result;
    result = reset();
    if (result != ISLE_GAME_OK) return result;
    return create();
}
} // namespace isle::scene
