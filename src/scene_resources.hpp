#ifndef ISLE_SCENE_RESOURCES_HPP
#define ISLE_SCENE_RESOURCES_HPP

#include "35mmisle/sprite.hpp"
#include <cstddef>
#include <cstdint>
#include <span>

namespace isle::scene {
// Source SceneSetup seeds, not limits. Native pools grow from the scene arena
// using actual host sizeof/alignment and recycle only after joined deletion.
struct PoolCounts {
    std::size_t objects = 0, processes = 0, sprites = 0, cameras = 0, matrices = 0;
};
struct SpriteResource { std::uint32_t identity; sprite::Sprite sprite; };
// Borrowed descriptors and their pixels must outlive native scene borrowers and
// actual renderer completion. Scene-specific callbacks/models stay with owners.
struct ResourceView {
    PoolCounts pools;
    std::span<const SpriteResource> sprites;
};
} // namespace isle::scene
#endif
