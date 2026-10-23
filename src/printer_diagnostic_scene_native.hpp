#ifndef ISLE_PRINTER_DIAGNOSTIC_SCENE_NATIVE_HPP
#define ISLE_PRINTER_DIAGNOSTIC_SCENE_NATIVE_HPP
#include "printer_diagnostic_source.h"
#include "diagnostic_resources.hpp"
#include <array>
namespace isle::application { class Photos; }
namespace isle::scene::graphics { struct Completed; }
namespace isle::post_course {
struct PrinterDiagnosticSceneNative final {
    explicit PrinterDiagnosticSceneNative(application::Photos&) noexcept {}
    PrinterDiagnosticSceneNative(const PrinterDiagnosticSceneNative&) = delete;
    PrinterDiagnosticSceneNative& operator=(const PrinterDiagnosticSceneNative&) = delete;
    std::shared_ptr<const DiagnosticResources> resources;
    IslePrinterDiagnosticSource* source = nullptr;
    IsleGameObject* camera_object = nullptr;
    IsleGameCamera* camera = nullptr;
    std::array<IslePhotoRecord,4> saved{};
    bool depth_admitted = false;
    bool postprocess = false;
    bool photo_ready = false;
    bool frame_staged = false;
    std::uint64_t first_ordinal = 0;
};
const IslePrinterDiagnosticNative& printer_diagnostic_scene_native_table() noexcept;
IsleGameResult printer_diagnostic_scene_native_prepare(IslePrinterDiagnosticSource*,PrinterDiagnosticSceneNative&) noexcept;
// Call under the actual completed Frame ownership, before releasing its RGBA.
IsleGameResult printer_diagnostic_scene_native_completed(IslePrinterDiagnosticSource*,const scene::graphics::Completed&) noexcept;
}
#endif
