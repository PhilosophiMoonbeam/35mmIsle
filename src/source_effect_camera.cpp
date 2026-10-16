#include "source_effect_camera.hpp"
#include <cmath>
#include <cstdint>

// Exact own-GAME definitions already admitted by snap-engine-inputs.json.
extern "C" void hal_look_at_roll_f(float [4][4], float, float, float,
                                    float, float, float, float, float, float, float);
extern "C" void hal_perspective_fast_f(float [4][4], std::uint16_t*,
                                       float, float, float, float, float);

namespace isle::menu {
bool source_effect_camera(IsleGame* game, IsleGameCamera* camera, EffectCamera& output) noexcept {
    const IsleGameCameraParameters* parameters = nullptr;
    float roll = 0;
    if (isle_game_camera_parameters(game, camera, &parameters) != ISLE_GAME_OK || !parameters ||
        isle_game_camera_roll(game, camera, &roll) != ISLE_GAME_OK) return false;
    const auto& view = parameters->look_at;
    const auto& perspective = parameters->perspective;
    // Bound the exact GAME fast-trig integer argument before its evaluation.
    if (!std::isfinite(perspective.fovy) || perspective.fovy <= 0 || perspective.fovy >= 180) return false;
    float look_at[4][4], projection[4][4];
    hal_look_at_roll_f(look_at, view.eye[0], view.eye[1], view.eye[2],
        view.at[0], view.at[1], view.at[2], roll, 0, 1, 0);
    hal_perspective_fast_f(projection, nullptr, perspective.fovy, perspective.aspect,
        perspective.near_plane, perspective.far_plane, perspective.scale);
    EffectCamera candidate;
    for (unsigned row = 0; row < 4; ++row) for (unsigned column = 0; column < 4; ++column) {
        const float value = ((look_at[row][0] * projection[0][column] +
            look_at[row][1] * projection[1][column]) +
            look_at[row][2] * projection[2][column]) + look_at[row][3] * projection[3][column];
        if (!std::isfinite(value)) return false;
        candidate.projection[row][column] = value;
    }
    for (unsigned axis = 0; axis < 3; ++axis) {
        candidate.viewport_scale[axis] = parameters->viewport.scale[axis];
        candidate.viewport_translate[axis] = parameters->viewport.translate[axis];
    }
    output = candidate;
    return true;
}
}
