#ifndef ISLE_END_LEVEL_8_SCENE_NATIVE_HPP
#define ISLE_END_LEVEL_8_SCENE_NATIVE_HPP
#include "end_level_8_source.h"
#include "post_course_resources.hpp"
#include <memory>

namespace isle::application { class Photos; }
namespace isle::post_course {
// Mechanical, address-stable bindings only. The seven Original C bodies own
// all timing, input selection, photo indices, speed and scene-result decisions.
struct EndLevel8SceneNative final {
    explicit EndLevel8SceneNative(application::Photos&) noexcept {}
    EndLevel8SceneNative(const EndLevel8SceneNative&) = delete;
    EndLevel8SceneNative& operator=(const EndLevel8SceneNative&) = delete;
    std::shared_ptr<const Resources> resources;
    IsleEndLevel8Source* source = nullptr;
    IsleGameObject* camera_object = nullptr;
    IsleEndLevel8Camera* camera = nullptr;
    bool initial_data_admitted = false;
    bool depth_admitted = false;
};
const IsleEndLevel8Native& end_level_8_scene_native_table() noexcept;
IsleGameResult end_level_8_scene_native_prepare(IsleEndLevel8Source*, EndLevel8SceneNative&) noexcept;
}
#endif
