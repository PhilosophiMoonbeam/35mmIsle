#ifndef ISLE_SCENE_RENDER_HPP
#define ISLE_SCENE_RENDER_HPP

#include "scene_services.hpp"
#include <array>

namespace isle::scene::render {
struct List { std::uint32_t address = 0; std::size_t words = 0; };
struct Pass;
struct Prepared {
    List camera;
    std::array<List, 4> lanes{};
};
// Called synchronously under the one exclusive actual-RSP preparation lease.
// The compiled scene producer owns its native model/material/effect facts; it
// must filter objects by Pass::includes and return real EndDL sublists only.
using Prepare = bool (*)(void*, std::span<std::uint32_t>, std::uint64_t,
                        const Pass&, Prepared&);
enum class Kind { clear, sprites, compiled };
enum class TagMatch { intersection, equality };
struct Pass {
    Kind kind = Kind::compiled;
    IsleGameCamera* camera = nullptr;
    std::uint32_t priority = 0;
    unsigned flags = 0;
    std::uint32_t background_rgba = 0;
    std::uint64_t link_mask = ~std::uint64_t(0);
    std::uint32_t camera_tag = 0;
    TagMatch tag_match = TagMatch::intersection;
    // Scissor is intersected with the real native camera viewport.
    sprite::Bounds scissor{};
    std::span<const SpriteDraw> sprites;
    sprite::RamRange sprite_commands{};
    Prepare prepare = nullptr;
    void* userdata = nullptr;
    bool includes(RenderMetadata object) const noexcept {
        return object.link < 64 && ((link_mask >> object.link) & 1u) &&
               (tag_match == TagMatch::equality ? object.camera_tag == camera_tag :
                                                (object.camera_tag & camera_tag) != 0);
    }
};
struct Resources {
    // Owners/descriptors remain stable through actual GPU/readback retirement.
    // Stable identity changes only for actual source/RAM reuse, not each frame.
    std::uint64_t identity = 0;
    sprite::Producer* sprites = nullptr;
    std::span<const sprite::AssetSpan> immutable;
    // Every live scene producer's command/matrix/material/task reservation.
    std::span<const sprite::RamRange> reservations;
    std::array<std::uint32_t, 2> framebuffers{0x3b5000, 0x3da800};
    std::uint32_t pre_render = 0;
};
struct Input {
    IsleGame* game = nullptr;
    Resources resources;
    // Highest camera priority first; equal priorities preserve source order.
    // A camera pass joins real lanes in source order 0,2,1,3.
    std::span<const Pass> passes;
    save::ScreenOffsets screen_offsets{};
};
} // namespace isle::scene::render
#endif
