#ifndef ISLE_POST_COURSE_WINDOW_HPP
#define ISLE_POST_COURSE_WINDOW_HPP
#include "post_course_resources.hpp"
#include "post_course_window_source.h"
#include "window_element.hpp"
#include <memory>
namespace isle::scene::graphics { class Frame; }

namespace isle::post_course {
/* Each slot reserves a primary physical range and, when nonzero, a disjoint
 * alternate range. Source single-image Elements need no alternate reservation.
 * The caller holds real graphics retirement before mutation or teardown. */
struct WindowRasterSlot {
    std::array<std::uint32_t,2> physical{};
    std::size_t plane_capacity=0;
};
struct WindowCapacity {
    std::span<const WindowRasterSlot> rasters;
    std::size_t operations=8, button_tables=16, buttons_per_table=64, adopted_objects=64;
    // Separate fixed reservations for reached thumbnail children. Both planes
    // copy the original prototype; the primary is updated/published in place
    // only after the caller's real graphics retirement. No preview renderer.
    std::span<const WindowRasterSlot> thumbnails{};
};
struct WindowObservation {
    std::size_t prints=0,fades=0,layouts=0;
    bool printing=false,skip_enabled=false;
};
/* A single stable scene context. All borrowed dependencies outlive this object;
 * widgets/Text must be exclusively assigned to it. Producer uses Resources'
 * roots()/banks() order and Services uses Resources::view(), without rebasing.
 * No clock/renderer ownership. Construction allocates bounded bookkeeping once.
 * All external Source drivers/callbacks must join before retire/destruction;
 * their nonreturning scene exit first uses the C stop/cancel interface.
 * retire runs outside native traversal after actual graphics retirement and
 * before dependency destruction. It reports binding release, not game success;
 * sticky execution status remains available on source(). The destructor
 * terminates rather than free failed live bindings.
 * Native source byte strings remain borrowed, never transcoded/copied. Caller
 * button descriptors convert into bounded stable native storage, retaining
 * live text backing and source entries until scene retirement, including
 * cancelled partially-created Layout tables. */
class Window final {
public:
    Window(scene::Services&,window::Text&,window::Frame&,window::Focus&,
           window::Layout&,window::Buttons&,const Resources&,
           std::shared_ptr<window::Heap>,sprite::Producer&,WindowCapacity);
    ~Window();
    Window(const Window&)=delete;
    Window& operator=(const Window&)=delete;
    IslePostCourseWindow* source() noexcept;
    IsleGameResult retire() noexcept;
    // Cold binding to the actual Frame; it outlives this Window.
    IsleGameResult bind_graphics_frame(scene::graphics::Frame&) noexcept;
    // Read-only native borrows, never Source execution calls or failure latches.
    // The caller excludes Source, GPU and all mutation; views expire before the
    // next tick/attachment removal. Null or retired owned Elements return null;
    // foreign, wrong-kind and unissued handles reject without faulting either
    // owner. Raw object/sprite identities must not survive native slot reuse.
    static IsleGameResult observe_element(const IslePostCourseWindow*,
        IslePostCourseElement,const IsleSprite** output) noexcept;
    static IsleGameResult observe_sprite(const IslePostCourseWindow*,
        IsleGameSprite*,const IsleSprite** output) noexcept;
    static IsleGameResult observe_sprite_root(const IslePostCourseWindow*,
        IsleGameObject*,const IsleSprite** output) noexcept;
    // Zero-based current Layout::buttons() slot, including its empty sentinel.
    // An uninitialized Layout or invalid slot rejects without latching failure.
    static IsleGameResult observe_layout_slot_id(const IslePostCourseWindow*,
        std::size_t,int* output) noexcept;
    // Bounded inspection of existing operations/Text only; never steps them.
    static IsleGameResult observe_operations(const IslePostCourseWindow*,
        WindowObservation* output) noexcept;
    // Exact padded allocation derived from the admitted prototype's complete
    // bitmap pointer extents (small thumbnails or CameraCheck 0x80208900).
    // Use before context construction to reserve its two physical planes.
    static std::size_t thumbnail_plane_bytes(const Resources&,std::uint32_t identity);
private:
    struct Impl;
    std::unique_ptr<Impl> p_;
};
}
#endif
