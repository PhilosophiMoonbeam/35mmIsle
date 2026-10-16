#ifndef ISLE_OAKS_LAB_SCENE_HPP
#define ISLE_OAKS_LAB_SCENE_HPP
#include "scene_render.hpp"
#include "window_resources.hpp"
#include "window_widgets.hpp"
#include <memory>

namespace isle::oaks_lab {
class Resources;
enum class Phase { dormant, opening, dialogue, page_wait, repeat_question, lab, courses, confirmation, save_prompt, saving, keep_going, save_failed, closing, complete, retired };
struct View {
    Phase phase = Phase::dormant;
    int selected_slot = 0, selected_course = 0;
    std::uint32_t page_address = 0;
    std::uint64_t printed_characters = 0, commit_attempts = 0;
    bool first_meet_consumed = false, exit_requested = false;
    std::int32_t source_result = 11;
    save::CommitResult commit{};
};
struct Config {
    int level_id = 0;
    std::uint64_t resource_identity = 1;
    std::array<std::uint32_t,2> raster_addresses{0x400000,0x410000};
    sprite::RamRange sprite_commands{0x640000,0x20000};
};
struct Navigation { int slot = 0; unsigned move_sounds = 0; bool back = false; };
Navigation source_navigation(std::span<const window::Button,8>,int slot,const game_input::MenuInput&) noexcept;
bool commit_acknowledged(const save::CommitResult&) noexcept;
// Services is constructed against Resources::view(), using the same persistent
// State/GameSession as title/name. Native updates run through the shared game;
// this owner never owns an input/audio pump or a second renderer. Retirement is
// outside traversal AFTER actual graphics/readback/producer borrowers cease.
class Scene final {
public:
    Scene(scene::Services&, std::shared_ptr<const Resources>, sprite::Producer&, Config = {});
    ~Scene();
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;
    bool start() noexcept;
    View view() const noexcept;
    scene::render::Input render_input();
    bool failed() const noexcept;
    const char* error() const noexcept;
    void retire() noexcept;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace isle::oaks_lab
#endif
