#ifndef ISLE_POST_COURSE_MODULES_HPP
#define ISLE_POST_COURSE_MODULES_HPP
#include "camera_check_source.h"
#include "world_module.hpp"
#include <cstdint>
#include <array>
#include <cstddef>
#include <string_view>

namespace isle::application { class Resources; }
namespace isle::rom { class Snapshot; }
namespace isle::post_course {
/* Original mutable module data, independent of disposable native scenes and
 * course asset residency. Observe actual successful Source overlay loads before
 * binding a new scene. Neither rendering nor photo begin/clear resets this.
 * Snapshot storage also outlives these retained source-byte name views. */
struct Modules final {
    float preview_scale = 0;
    IsleCameraCheckMusicState music{};
    ::isle::world::ModuleState world;
    std::array<std::string_view,7> course_names{};
    const std::byte* rom_binding = nullptr;
    std::uint64_t window_generation = 0, world_generation = 0;
    // No WORLD load yet is ordinary before the original manager reaches Oak.
    void observe_world(const application::Resources&);
    void observe_window(const rom::Snapshot&, const application::Resources&);
};
}
#endif
