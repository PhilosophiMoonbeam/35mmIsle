#ifndef ISLE_SCENE_RENDER_HPP
#define ISLE_SCENE_RENDER_HPP

#include "scene_services.hpp"
#include <array>

#include "graphics_task.hpp"
namespace isle::scene::graphics { class SourceControl; }

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
// Runs before generic camera emission under the same exclusive preparation
// lease. A present control owns the whole dispatch: skip, emit source commands,
// or explicitly draw the current pass after resolving late visibility/views.
using Control = bool (*)(void*, graphics::SourceControl&);
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
    // Sprite passes naming identical full ranges share a bounded append arena
    // for each Frame::begin. Other sprite arenas must be disjoint (even when
    // controls skip drawing); partial overlap is rejected before callbacks.
    // Ownership is frozen at preflight, but actual draw-time views determine
    // consumption. Skipped controls consume nothing; empty draws emit a list.
    sprite::RamRange sprite_commands{};
    Prepare prepare = nullptr;
    void* userdata = nullptr;
    // Actual transition owner retained through preparation, GPU and readback.
    ::isle::screen_transition::Owner* transition_owner = nullptr;
    Control control = nullptr;
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
    // Actual included task program, independent of later in-list transitions.
    // Legacy callers retain their original F3DEX2_NoN_fifo task ID2.
    ::isle::graphics::SourceProgram program = ::isle::graphics::SourceProgram::f3dex2_non_fifo;
    // Optional final wire phase under the same real preparation lease, after
    // every camera callback. This is not an extra camera/pass or submission.
    Control finalize=nullptr;
    void* finalize_user=nullptr;
};
} // namespace isle::scene::render
#endif
