#include "source_effect_world.hpp"
#include <cmath>
#include <cstdint>

extern "C" void hal_scale_f(float [4][4], float, float, float);
extern "C" void hal_rotate_rpy_translate_f(float [4][4], float, float, float,
                                          float, float, float);

namespace isle::menu::model {
namespace {
struct Matrix { float v[4][4]; };
Matrix identity() noexcept {
    Matrix m{};
    for (unsigned i = 0; i < 4; ++i) m.v[i][i] = 1;
    return m;
}
// Independently authored row product; do not combine local S/R/T into RTS:
// fx_getPosVelDObj concatenates each conditional operation separately.
bool append(Matrix& a, const Matrix& b) noexcept {
    Matrix result;
    for (unsigned i = 0; i < 4; ++i)
        for (unsigned j = 0; j < 4; ++j) {
            const float value = ((a.v[i][0] * b.v[0][j] + a.v[i][1] * b.v[1][j]) +
                                  a.v[i][2] * b.v[2][j]) + a.v[i][3] * b.v[3][j];
            if (!std::isfinite(value)) return false;
            result.v[i][j] = value;
        }
    a = result;
    return true;
}
bool finite(IsleGameVec3 v) noexcept {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}
bool apply(Matrix& world, const IsleGameModelTransform& t, bool scale) noexcept {
    if (!finite(t.position) || !finite(t.rotation) || (scale && !finite(t.scale))) return false;
    Matrix operation;
    if (scale && (t.scale.x != 1 || t.scale.y != 1 || t.scale.z != 1)) {
        hal_scale_f(operation.v,t.scale.x,t.scale.y,t.scale.z);
        if (!append(world,operation)) return false;
    }
    if (t.rotation.x != 0 || t.rotation.y != 0 || t.rotation.z != 0) {
        // matrix.c1229 calls the exact RPY formula and only replaces row3.
        hal_rotate_rpy_translate_f(operation.v,0,0,0,t.rotation.x,t.rotation.y,t.rotation.z);
        if (!append(world,operation)) return false;
    }
    if (t.position.x != 0 || t.position.y != 0 || t.position.z != 0) {
        operation = identity();
        operation.v[3][0] = t.position.x;
        operation.v[3][1] = t.position.y;
        operation.v[3][2] = t.position.z;
        if (!append(world,operation)) return false;
    }
    return true;
}
EffectsStatus failure(EffectsError error, std::size_t node) noexcept {
    return {error,no_index,node};
}
}
EffectsStatus source_effect_world(IsleGame* game, IsleGameModel* model,
                                 std::size_t node, menu::EffectWorld& output) noexcept {
    const IsleGameModelNode* nodes = nullptr;
    std::size_t count = 0;
    const IsleGameModelPlacement* placement = nullptr;
    if (isle_game_model_peek_nodes(game,model,&nodes,&count) != ISLE_GAME_OK ||
        isle_game_model_placement(game,model,&placement) != ISLE_GAME_OK || !placement)
        return failure(EffectsError::attachment,node);
    if (node >= count) return failure(EffectsError::extent,node);
    Matrix world = identity();
    for (std::size_t current = node;;) {
        if (!apply(world,nodes[current].transform,true)) return failure(EffectsError::arithmetic,current);
        const auto parent = nodes[current].parent;
        if (parent == UINT32_MAX) break;
        // Native topology guarantees earlier parents; retain a bounded fail-closed
        // guard rather than letting invalid topology become an unbounded walk.
        if (parent >= current) return failure(EffectsError::attachment,current);
        current = parent;
    }
    switch (placement->mode) {
    case ISLE_GAME_MODEL_PLACEMENT_DISABLED: break;
    case ISLE_GAME_MODEL_PLACEMENT_RT:
    case ISLE_GAME_MODEL_PLACEMENT_RTS:
        if (!apply(world,placement->transform,placement->mode == ISLE_GAME_MODEL_PLACEMENT_RTS))
            return failure(EffectsError::arithmetic,node);
        break;
    default: return failure(EffectsError::attachment,node);
    }
    for (unsigned i = 0; i < 4; ++i)
        for (unsigned j = 0; j < 4; ++j) output.matrix[i][j] = world.v[i][j];
    return {};
}
IsleGameResult source_effect_position_velocity(IsleGame* game,IsleGameModel* model,std::size_t node,
    IsleGameVec3* position,IsleGameVec3* velocity) noexcept {
    if (!position || !velocity) return ISLE_GAME_INVALID_ARGUMENT;
    menu::EffectWorld world{};
    if (!source_effect_world(game,model,node,world)) return ISLE_GAME_EXECUTION_FAILURE;
    auto& m=world.matrix;
    // effect.c1335-1360: normalize columns, never transform velocity as a
    // point or derive it from frame differences/renderer's quantized caches.
    for (unsigned col=0;col<3;++col) {
        if (m[0][col]==0 && m[1][col]==0 && m[2][col]==0) continue;
        const float n=std::sqrt(m[0][col]*m[0][col]+m[1][col]*m[1][col]+m[2][col]*m[2][col]);
        if (n==0 || !std::isfinite(n) || !std::isfinite(1.0f/n)) return ISLE_GAME_INVALID_ARGUMENT;
        const float inv=1.0f/n;
        m[0][col]*=inv; m[1][col]*=inv; m[2][col]*=inv;
    }
    const auto v=*velocity;
    const IsleGameVec3 result{m[0][0]*v.x+m[1][0]*v.y+m[2][0]*v.z,
        m[0][1]*v.x+m[1][1]*v.y+m[2][1]*v.z,m[0][2]*v.x+m[1][2]*v.y+m[2][2]*v.z};
    if (!std::isfinite(result.x) || !std::isfinite(result.y) || !std::isfinite(result.z)) return ISLE_GAME_INVALID_ARGUMENT;
    *position={m[3][0],m[3][1],m[3][2]}; *velocity=result;
    return ISLE_GAME_OK;
}
}
