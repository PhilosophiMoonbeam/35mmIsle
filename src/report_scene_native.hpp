#ifndef ISLE_REPORT_SCENE_NATIVE_HPP
#define ISLE_REPORT_SCENE_NATIVE_HPP
#include "report_source.h"
#include <array>
#include <cstddef>
#include <cstdint>

namespace isle::application { class Photos; }
namespace isle::scene::graphics { class SourceControl; }
namespace isle::post_course { struct ReportSceneNative; }

/* A native command cursor, not Gfx layout or a guest pointer. Only an actual
 * SourceControl dispatch lends control; its address never escapes that dispatch. */
struct IsleReportGraphics {
    isle::post_course::ReportSceneNative* allocation = nullptr;
    isle::scene::graphics::SourceControl* control = nullptr;
    std::uint64_t generation = 0;
};
namespace isle::post_course {
struct ReportSceneNative final {
    explicit ReportSceneNative(application::Photos&) noexcept;
    ReportSceneNative(const ReportSceneNative&) = delete;
    ReportSceneNative& operator=(const ReportSceneNative&) = delete;
    struct Destination {
        IsleGameSprite* sprite = nullptr;
        IslePostCourseThumbnail thumbnail{};
        std::uint32_t prototype = 0;
    };
    IsleReportData data{};
    std::array<std::array<const char*,3>,12> captions{};
    std::array<std::array<IslePostCourseButton,7>,8> buttons{};
    std::array<Destination,7> destinations{};
    IsleReportGraphics graphics{};
    IsleReportSource* source = nullptr;
    std::uint64_t generation = 0;
    bool admitted = false;
};
const IsleReportNative& report_scene_native_table() noexcept;
IsleGameResult report_scene_native_prepare(IsleReportSource*,ReportSceneNative&) noexcept;
/* Actual typed render dispatch only. userdata is the exact stable menu state,
 * retained by the Native owner through Source/GPU/Window retirement. These do
 * not install callbacks or infer an initialized render callback registration. */
bool report_scene_native_render_bind_control(void*,scene::graphics::SourceControl&) noexcept;
bool report_scene_native_render_state_control(void*,scene::graphics::SourceControl&) noexcept;
}
extern "C" {
/* Called by the pure-C provider after the returning table preview preparation.
 * The C driver, never this leaf, owns Replay waits and acknowledgment. */
IsleGameResult isle_report_native_preview_consume(IsleReportSource*,IsleGameSprite*,const IslePostCoursePreview*) noexcept;
IsleGameResult isle_report_native_sprite_rebind(IsleReportSource*,IsleGameSprite*,std::uint32_t) noexcept;
IsleGameResult isle_report_native_thumbnail_publish(IsleReportSource*,std::int32_t) noexcept;
IsleGameResult isle_report_native_preview_scale(IsleReportSource*,float*) noexcept;
}
#endif
