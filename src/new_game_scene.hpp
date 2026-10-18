#ifndef ISLE_NEW_GAME_SCENE_HPP
#define ISLE_NEW_GAME_SCENE_HPP

#include "scene_render.hpp"
#include "window_resources.hpp"
#include <array>
#include <memory>

namespace isle::new_game {
// Source byte editor, shared with the actual native control process. Selection
// comes from Resources::name_character(255 + row*5 + column), never host text.
struct EditEffect {
    std::array<unsigned,3> sounds{};
    unsigned sound_count = 0;
    bool redraw_name = false, refresh_preview = false, confirm = false;
};
class Editor final {
public:
    EditEffect press(std::uint32_t buttons, std::array<std::uint8_t,2> selection) noexcept;
    // Called only after the actual one-update End confirmation wait. Default
    // characters are the source font's ASCII mappings for T,o,d,d.
    bool confirm(std::array<std::uint16_t,4> default_characters) noexcept;
    std::span<const std::uint8_t,16> name() const noexcept { return name_; }
    unsigned byte_length() const noexcept { return length_; }
    int column() const noexcept { return x_; }
    int row() const noexcept { return y_; }
    bool confirming() const noexcept { return confirming_; }
    bool confirmed() const noexcept { return confirmed_; }
    std::array<int,4> focus() const noexcept;
private:
    void jump_end(EditEffect&) noexcept;
    void backspace(EditEffect&) noexcept;
    std::array<std::uint8_t,16> name_{};
    unsigned length_ = 0;
    int x_ = 0, y_ = 0, remembered_x_ = 0;
    bool confirming_ = false, confirmed_ = false;
};
struct Observation {
    int column = 0, row = 0;
    unsigned name_bytes = 0;
    std::array<std::uint8_t,16> name{};
    bool controls_ready = false, confirming = false, confirmed = false;
    bool exit_requested = false, completed = false;
};
struct Config {
    std::uint64_t resource_identity = 0x4e414d45;
    std::uint32_t raster_begin = 0x400000, raster_end = 0x580000;
    std::array<std::uint32_t,2> fade_addresses{0x580000,0x580008};
    std::array<sprite::RamRange,3> commands{{{0x640000,0x20000},{0x660000,0x20000},{0x680000,0x20000}}};
};
// Owns actual native continuations, source UI and dynamic rasters; borrows the
// one shared Services/Producer and initialized persistent State. Main owns the
// common Storage, OM traversal, input/audio calendars and actual graphics fence.
class Scene final {
public:
    Scene(scene::Services&, std::shared_ptr<const window::Resources>,
          sprite::Producer&, Config = {});
    ~Scene();
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;
    bool start() noexcept;
    // Preparation-only borrowed view: no ticking, publication or name edits.
    scene::render::Input render_input() noexcept;
    Observation observation() const noexcept;
    bool failed() const noexcept;
    const char* error() const noexcept;
    bool exit_requested() const noexcept;
    bool completed() const noexcept;
    // Owner-thread only, outside traversal AFTER the actual graphics fence.
    // Joins/retires this UI first, then source omSetupScene return boundary:
    // PFID16 and result11 exactly once. Services/State must still be alive.
    bool complete_scene() noexcept;
    bool take_result(std::int32_t&) noexcept;
    // Live State projection. Unequal endpoints fail the scene, never collapse.
    save::ScreenOffsets screen_offsets() const noexcept;
    // Same external fence prerequisite. Abort retirement does not set PFID16.
    void retire() noexcept;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace isle::new_game
#endif
