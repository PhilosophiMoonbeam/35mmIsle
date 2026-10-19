#ifndef ISLE_GALLERY_SCENE_NATIVE_HPP
#define ISLE_GALLERY_SCENE_NATIVE_HPP
#include "gallery_source.h"
#include "application_photos.hpp"
#include "35mmisle/game_input.hpp"
#include <array>
#include <cstdint>
#include <memory>

namespace isle::photo { class Presentation; }
namespace isle::scene::graphics { class Frame; class SourceControl; }
namespace isle::post_course {
class Resources;
struct GallerySceneNative final {
    explicit GallerySceneNative(application::Photos&) noexcept;
    GallerySceneNative(const GallerySceneNative&) = delete;
    GallerySceneNative& operator=(const GallerySceneNative&) = delete;
    GallerySceneNative(GallerySceneNative&&) = delete;
    GallerySceneNative& operator=(GallerySceneNative&&) = delete;

    // The four full-wire owners are constructed cold and never move. Original
    // C copies all four canonical records before its first temporary write.
    std::array<application::PhotoWorking,4> working;
    std::array<IslePhotoWorking*,4> working_handles{};
    std::shared_ptr<const Resources> resources;
    IsleGalleryInitialData initial{};
    std::array<IslePostCourseButton,7> absent_buttons{};
    std::array<IslePostCourseButton,8> present_buttons{};
    IsleGallerySource* source = nullptr;
    const void* native_allocation = nullptr;
    IsleGameObject* main_object = nullptr;
    IsleGameObject* photo_object = nullptr;
    IsleGameCamera* main_camera = nullptr;
    IsleGameCamera* photo_camera = nullptr;
    const application::Photos* photos = nullptr;
    bool prepared = false, screen_applied = false, cancel_requested = false;
    IsleGameResult failure = ISLE_GAME_OK;
    save::CommitResult last_commit{};
    game_input::Error printer_requirement = game_input::Error::none;

    // This token is an actual bounded native display cursor, not a guest Gfx*
    // or a successful command counter standing in for rendering. The returning
    // leaves emit into control, which exists only inside real Frame preparation.
    struct DisplayCursor {
        GallerySceneNative* state = nullptr;
        std::uint64_t dispatch = 0;
        std::size_t commands = 0;
    } display;
    scene::graphics::SourceControl* control = nullptr;
    void request_cancel() noexcept;
    // Parent has already joined Source callbacks and services/released actual
    // completed display ownership. False retains every working/resource owner.
    bool cancel_drain(scene::graphics::Frame&, photo::Presentation*) noexcept;
};
const IsleGalleryNative& gallery_scene_native_table() noexcept;
IsleGameResult gallery_scene_native_prepare(IsleGallerySource*, GallerySceneNative&) noexcept;
// Bind only for an actually admitted Original callback identity. Each hook
// invokes its precise plain-C callback under the existing nonwaiting fence.
// The callback's cursor/state is consumed by SourceControl's real command sink.
bool gallery_scene_native_display_control(void*, scene::graphics::SourceControl&);
bool gallery_scene_native_render_control(void*, scene::graphics::SourceControl&);
}
#endif
