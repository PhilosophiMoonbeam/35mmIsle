#ifndef ISLE_ALBUM_SCENE_NATIVE_HPP
#define ISLE_ALBUM_SCENE_NATIVE_HPP
#include "album_source.h"
#include "application_photos.hpp"
#include "scene_graphics.hpp"
#include <array>
#include <cstdint>
#include <span>

namespace isle::post_course {
struct AlbumSceneNative final {
    explicit AlbumSceneNative(application::Photos& photos) noexcept
        : first_working(photos), second_working(photos) {}
    AlbumSceneNative(const AlbumSceneNative&) = delete;
    AlbumSceneNative& operator=(const AlbumSceneNative&) = delete;
    application::PhotoWorking first_working;
    application::PhotoWorking second_working;
    IsleAlbumSource* source = nullptr;
    IsleGameResult status = ISLE_GAME_OK;
    IsleGameObject* main_object = nullptr;
    IsleGameCamera* main_camera = nullptr;
    IsleGameObject* auxiliary_object = nullptr;
    IsleGameCamera* auxiliary_camera = nullptr;
    // Allocation ownership remains with Window even after Original clears its
    // semantic cover reference. This is not a fifth Source sprite-array slot.
    IsleGameObject* cover_allocation = nullptr;
    std::span<const std::uint8_t> character_page0;

    // The parent cold-admits two disjoint physical immutable list ranges via
    // bind_render(), merges assets into actual Frame::Input immutable, and uses
    // pre_render() as that Frame's actual pre-render address. No guest code.
    std::array<std::uint8_t,112> initialization_bytes{};
    std::array<std::uint8_t,16> pre_render_bytes{};
    std::array<sprite::AssetSpan,2> render_assets{};
    std::uint32_t initialization_address = 0;
    std::uint32_t pre_render_address = 0;
    bool render_bound = false;

    // Actual Source Gfx** semantics, not a cast to an SDK pointer. Parent binds
    // this exact state-owned cursor only inside Frame's exclusive RAM lease,
    // through begin_cursor(), and propagates end_cursor()'s returned address to
    // the corresponding real lane. The preincrement leaf leaves address at the
    // command it wrote; render_state postincrements by all five commands.
    struct DisplayCursor {
        std::span<std::uint32_t> ram;
        std::uint64_t epoch = 0;
        std::uint32_t address = 0, end = 0;
        bool active = false;
    } display_cursor;

    IsleGameResult bind_render(std::uint32_t initialization_physical,
        std::uint32_t pre_render_physical, std::uint32_t common_pre_render) noexcept;
    IsleGameResult apply_render(scene::render::Input&) noexcept;
    IsleGameResult begin_cursor(scene::graphics::SourceControl&,
        std::uint32_t address,std::uint32_t end) noexcept;
    IsleGameResult end_cursor(std::uint32_t* address) noexcept;
    std::uint32_t pre_render() const noexcept { return pre_render_address; }
};
const IsleAlbumNative& album_scene_native_table() noexcept;
IsleGameResult album_scene_native_prepare(IsleAlbumSource*,AlbumSceneNative&) noexcept;
}
#endif
