#ifndef ISLE_GAME_STATE_HPP
#define ISLE_GAME_STATE_HPP

#include "35mmisle/game_input.hpp"
#include "35mmisle/game_save.hpp"
#include "35mmisle/rng.h"
#include <array>
#include <cstdint>
#include <filesystem>

namespace isle::game {
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
    // BB40 byte flags and selected physical controller are not scene notices.
    void select_controller(const game_input::Snapshot&) noexcept;
    int controller = -1;
    std::uint8_t title_start_flags = 0;
    isle_rng rng{};
    // AAED0/AAEE8/AAF10: OR, test-any-and-clear-whole-mask, clear-all.
    void add_scene_notices(std::uint32_t flags) noexcept;
    bool consume_scene_notices(std::uint32_t flags) noexcept;
    void clear_scene_notices() noexcept;
    std::uint32_t scene_notices() const noexcept;
    // Independent source scene-manager latch; not a Beach scene-local end reason.
    void set_scene_latch(std::int32_t value) noexcept;
    std::int32_t scene_latch() const noexcept;
private:
    std::filesystem::path profile_;
    ::isle::save::GameOpenResult opened_{};
    bool attempted_ = false;
    std::uint32_t notices_ = 0;
    std::int32_t scene_latch_ = 0;
    std::array<char,512> error_{};
};
} // namespace isle::game
#endif
