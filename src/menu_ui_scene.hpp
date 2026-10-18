#ifndef ISLE_MENU_UI_SCENE_HPP
#define ISLE_MENU_UI_SCENE_HPP

#include "menu_title_scene.hpp"

namespace isle::menu::scene {
// GAME scene IDs, not an index into a host scene registry.
enum class SourceScene : std::int32_t { invalid = -1, beach = 0, oaks_lab = 11, attract = 21, gallery = 22 };
struct MenuAvailability { bool has_save = false, gallery = false; };
// Source menu ordering depends on PFID16 and >3 report Pokemon (excluding Mew).
unsigned menu_item_count(MenuAvailability) noexcept;
std::uint8_t menu_item_state(MenuAvailability, unsigned selection) noexcept;
SourceScene menu_exit_scene(std::uint8_t state) noexcept;
std::uint32_t menu_entry_flags(std::uint8_t state) noexcept;
struct MenuTransition { std::uint8_t state; unsigned selection; };
// Pure input/state boundary shared with the actual native continuation. Only
// source slow axes navigate. Input activity resets the idle deadline first.
MenuTransition menu_transition(MenuAvailability, std::uint8_t state, unsigned selection,
                               std::uint32_t pressed, std::uint32_t idle,
                               int controller, unsigned source_argument) noexcept;

// UI owns native control/cosmetic processes and mutable sprite copies only.
// Services, Title and their resource/service borrowers must outlive this owner.
// Parent owns GTL, global cameras and every GPU lease. State alone owns live VI;
// calibration writes it at Source service calls, independently of editing BSS.
class Ui final {
public:
    Ui(::isle::scene::Services&, Title&, const Catalog&);
    ~Ui();
    Ui(const Ui&) = delete;
    Ui& operator=(const Ui&) = delete;
    bool start() noexcept;
    std::uint8_t source_state() const noexcept;
    // Translation projection of State, not editable/saved offsets. Unsupported
    // unequal endpoints fail Services; callers must honor failed() on Services.
    save::ScreenOffsets screen_offsets() const noexcept;
    std::uint32_t entry_flags() const noexcept;
    void retire() noexcept;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace isle::menu::scene
#endif
