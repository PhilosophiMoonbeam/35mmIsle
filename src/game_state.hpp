#ifndef ISLE_GAME_STATE_HPP
#define ISLE_GAME_STATE_HPP

#include "application_source.h"
#include "35mmisle/game_input.hpp"
#include "35mmisle/game_save.hpp"
#include "35mmisle/rng.h"
#include <array>
#include <cstdint>
#include <filesystem>


namespace isle::game {
// Actual application-lifetime VI output, independent of profile RAM and menu
// editing BSS. Retain every endpoint even when native translation cannot draw it.
struct VideoOffsets {
    std::int32_t horizontal_start = 0, horizontal_end = 0;
    std::int32_t vertical_start = 0, vertical_end = 0;
    // Failure leaves the destination unchanged; never collapse unequal edges.
    bool translation(::isle::save::ScreenOffsets&) const noexcept;
};
// Application-lifetime owner. Construction never touches the profile. Logo's
// first actual wait must precede initialize(); the sole real open result and its
// Store evidence/lock survive all scene retirement, including rejected opens.
class State final {
public:
    explicit State(std::filesystem::path existing_profile, std::uint32_t random_seed = 1);
    ~State();
    State(const State&) = delete;
    State& operator=(const State&) = delete;
    State(State&&) = delete;
    State& operator=(State&&) = delete;
    bool initialize(std::uint32_t raw_game_time) noexcept;
    bool initialization_attempted() const noexcept;
    bool initialized() const noexcept;
    const char* error() const noexcept;
    const std::filesystem::path& profile_path() const noexcept;
    const ::isle::save::GameOpenResult* save_initialization() const noexcept;
    ::isle::save::GameSession* save() noexcept;
    const ::isle::save::GameSession* save() const noexcept;
    const VideoOffsets& screen_offsets() const noexcept;
    // Called at actual Source VI service boundaries, never by frame polling.
    void set_screen_offsets(VideoOffsets) noexcept;
    // BB40 byte flags and selected physical controller are not scene notices.
    void select_controller(const game_input::Snapshot&) noexcept;
    int controller = -1;
    std::uint8_t title_start_flags = 0;
    isle_rng rng{};
    // GAME4CDB0.c3-5/171-180, not WORLD overlay data or profile bytes.
    // Only actual createWorld/setBackgroundColor service boundaries change it.
    std::array<std::uint8_t,3> background_rgb{90,140,200};
    // Own GAME46270.c48–54 retains this application-selected script pointer.
    // Its immutable inputs outlive every course that adopts it.
    void set_idle_script(const IslePlayerIdleScript*) noexcept;
    const IslePlayerIdleScript* idle_script() const noexcept;
    // AAED0/AAEE8/AAF10: OR, test-any-and-clear-whole-mask, clear-all.
    void add_scene_notices(std::uint32_t flags) noexcept;
    bool consume_scene_notices(std::uint32_t flags) noexcept;
    void clear_scene_notices() noexcept;
    std::uint32_t scene_notices() const noexcept;
    // Independent source scene-manager latch; not a Beach scene-local end reason.
    void set_scene_latch(std::int32_t value) noexcept;
    std::int32_t scene_latch() const noexcept;
    // Original manager C and native scene views borrow this one retained record.
    IsleApplicationManagerState& manager_state() noexcept;
private:
    std::filesystem::path profile_;
    ::isle::save::GameOpenResult opened_{};
    bool attempted_ = false;
    std::uint32_t notices_ = 0;
    IsleApplicationManagerState manager_state_ = ISLE_APPLICATION_MANAGER_STATE_INITIALIZER;
    VideoOffsets screen_offsets_{};
    std::array<char,512> error_{};
};
} // namespace isle::game
#endif
