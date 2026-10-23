#ifndef ISLE_POST_COURSE_SCENE_HPP
#define ISLE_POST_COURSE_SCENE_HPP
#include "application_manager.hpp"
#include "post_course_modules.hpp"
#include "post_course_replay.hpp"
#include "post_course_resources.hpp"
#include "scene_graphics.hpp"
#include "photo_check_source.h"
#include "album_source.h"
#include "report_source.h"
#include "gallery_source.h"
#include "end_level_8_source.h"
#include "credits_source.h"
#include "printer_diagnostic_source.h"
#include <memory>
#include <optional>

namespace isle::snap_station { class Device; }
namespace isle::post_course {
struct SceneCreateResult;
struct SceneAccess;
enum class SceneStatus { ready, setup_pending, running, retiring, completed, cancelled, failed, retired };
/* Shared actual original CameraCheck9, PhotoCheck15, Album12, Report14,
 * Gallery22/23, EndLevel8=24 and Credits17..20 C entry and scene owner. Entry
 * continuation/context survive disposable sceneStorage and return only after
 * joined graphics/native/Window/Replay retirement. Manager retains its real
 * EntryBorrow; this owner never selects the application's next entry. */
class Scene final {
    friend struct SceneAccess;
public:
    static SceneCreateResult create(const rom::Snapshot&, application::Manager&,
        const application::EntryBorrow&, game::State&, game_input::Service&, game_audio::Session&, host::GtlClock&,
        application::Photos&, host::Progress&, ReplayNativeProgress, Modules&,
        Resources::Kind, std::int32_t entry_argument, bool require_cpu_renderer=false,
        snap_station::Device* station=nullptr, std::uint64_t application_epoch=0,
        std::uint64_t entry_serial=0) noexcept;
    ~Scene();
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;
    IsleGameResult resume() noexcept;
    bool setup_pending() const noexcept;
    IsleGameResult initialize_scene() noexcept;
    // STOPPED cancels the current Source update after native Replay drains.
    // The caller must abandon its GTL update and join cancellation retirement;
    // error() stays OK unless an actual non-STOPPED fault is retained.
    IsleGameResult update(std::uint32_t raw_game_time) noexcept;
    bool draw(scene::graphics::FrameContext = {}) noexcept;
    bool graphics_busy() const noexcept;
    scene::graphics::Poll poll() noexcept;
    scene::graphics::Completed completed_frame() const noexcept;
    IsleGameResult release_completed() noexcept;
    bool retire_scene() noexcept;
    IsleGameResult cancel() noexcept;
    IsleGameResult retire() noexcept;
    SceneStatus status() const noexcept;
    IsleGameResult error() const noexcept;
    const char* message() const noexcept;
    std::optional<std::int32_t> result() const noexcept;
    const IslePostCourseSceneSource& source() const noexcept;
    const IsleCameraCheckSource* camera_source() const noexcept;
    const IslePhotoCheckSource* photo_source() const noexcept;
    const IsleAlbumSource* album_source() const noexcept;
    const IsleReportSource* report_source() const noexcept;
    const IsleGallerySource* gallery_source() const noexcept;
    const IsleEndLevel8Source* end_level_8_source() const noexcept;
    const IsleCreditsSource* credits_source() const noexcept;
    const IslePrinterDiagnosticSource* printer_diagnostic_source() const noexcept;
    // Last input actually returned to original CameraCheck, not a new
    // menu/neutral derivation. Null until Source reads it, or after retirement.
    // Read-only borrow; serialized native updates may replace its values.
    const IsleCameraCheckInput* camera_input_observation(std::size_t controller) const noexcept;
private:
    struct Impl;
    explicit Scene(std::unique_ptr<Impl>) noexcept;
    std::unique_ptr<Impl> p_;
};
struct SceneCreateResult {
    IsleGameResult error = ISLE_GAME_OK;
    std::unique_ptr<Scene> scene;
    std::array<char,512> message{};
    explicit operator bool() const noexcept { return error == ISLE_GAME_OK && bool(scene); }
};
}
#endif
