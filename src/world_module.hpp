#ifndef ISLE_WORLD_MODULE_HPP
#define ISLE_WORLD_MODULE_HPP
#include <array>
#include <cstdint>

namespace isle::world {
// Pinned WORLD overlay data/BSS, not disposable scene ownership. The actual
// overlay loader resets this record; asset switches and native collection do not.
struct ModuleState {
    struct RoadEvent { float animation_time = 0, visibility = 0, unused = 0; };
    float global_timer = 0, sky_animation_speed = 1;
    std::int32_t sky_max_angle = 10000;
    std::array<RoadEvent,14> road_events{};

    // Source block.c58-65: createWorld/destroyWorld explicitly reset road data.
    // This sentinel state differs from zeroed BSS immediately after overlay load.
    void reset_road_events() noexcept {
        for (auto& event:road_events) event = {-1,0,0};
    }
};
}
#endif
