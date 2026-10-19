#ifndef ISLE_SCREEN_TRANSITION_RESOURCES_HPP
#define ISLE_SCREEN_TRANSITION_RESOURCES_HPP
#include "rom_snapshot.hpp"
#include "scene_render.hpp"
#include "screen_transition.hpp"
namespace isle::screen_transition {
// Exact admitted main initialized GBI/vertex closure; Snapshot retains all bytes.
namespace render = ::isle::scene::render;
class Resources final {
public:
    explicit Resources(const rom::Snapshot&,sprite::RamRange memory=workspace);
    std::span<const sprite::AssetSpan> immutable() const noexcept { return assets_; }
    static constexpr sprite::RamRange workspace{0x1f0000,0x10000};
    sprite::RamRange memory() const noexcept { return workspace_; }
    // Source camera metadata and source rendering recipe, for the shared compositor.
    render::Pass pass(Owner&) noexcept;
private:
    std::array<sprite::AssetSpan,1> assets_{};
    sprite::RamRange workspace_;
    static bool prepare(void*,std::span<std::uint32_t>,std::uint64_t,const render::Pass&,render::Prepared&);
};
}
#endif
