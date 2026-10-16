#include "actor_spatial.h"
#include "actor_owner_source.h"
#include <cmath>
#include <cstddef>
#include <limits>

namespace {
// Own GAME vector.c5-16,46-52. In particular, do not replace reciprocal
// multiplication with division, sqrt with hypot, or zero-magnitude vectors with
// a fabricated unit/zero direction: underflow leaves the input vector intact.
float normalize(IsleGameVec3& vector) noexcept {
    const float magnitude = std::sqrt((vector.x * vector.x + vector.y * vector.y) +
                                      vector.z * vector.z);
    if (magnitude == 0.0f) return 0.0f;
    const float inverse = 1.0f / magnitude;
    vector.x *= inverse;
    vector.y *= inverse;
    vector.z *= inverse;
    return magnitude;
}
float direction(IsleGameVec3& vector, IsleGameVec3 to, IsleGameVec3 from) noexcept {
    vector.x = to.x - from.x;
    vector.y = to.y - from.y;
    vector.z = to.z - from.z;
    return normalize(vector);
}

IsleGameResult position(IsleGame* game, IsleGameModel* model, bool require_auxiliary,
                        IsleGameVec3& output) noexcept {
    const IsleGameModelPlacement* placement = nullptr;
    auto result = isle_game_model_placement(game, model, &placement);
    if (result != ISLE_GAME_OK) return result;
    if (!placement) return ISLE_GAME_INVALID_STATE;
    switch (placement->mode) {
    case ISLE_GAME_MODEL_PLACEMENT_RT:
    case ISLE_GAME_MODEL_PLACEMENT_RTS:
        // Only source GET_TRANSFORM(model)->pos is consumed; never root local,
        // rendered/world/collision position, rotation or scale.
        output = placement->transform.position;
        return ISLE_GAME_OK;
    case ISLE_GAME_MODEL_PLACEMENT_DISABLED:
        break;
    default:
        return ISLE_GAME_INVALID_STATE;
    }
    if (require_auxiliary) return ISLE_GAME_INVALID_STATE;
    const IsleGameModelNode* nodes = nullptr;
    std::size_t count = 0;
    result = isle_game_model_peek_nodes(game, model, &nodes, &count);
    if (result != ISLE_GAME_OK) return result;
    if (!nodes || !count || nodes[0].parent != UINT32_MAX) return ISLE_GAME_INVALID_STATE;
    output = nodes[0].transform.position;
    return ISLE_GAME_OK;
}
} // namespace

extern "C" IsleGameResult isle_actor_spatial_distance(IsleGame* game,
    IsleGameObject* first, IsleGameObject* second, float* out_distance) {
    if (!out_distance) return ISLE_GAME_INVALID_ARGUMENT;
    IsleGameModel* model = nullptr;
    if (!first || !second) {
        const auto result = isle_actor_spatial_object_model(game, nullptr, &model);
        if (result != ISLE_GAME_OK) return result;
        *out_distance = std::numeric_limits<float>::max();
        return ISLE_GAME_OK;
    }
    auto result = isle_actor_spatial_object_model(game, first, &model);
    if (result != ISLE_GAME_OK) return result;
    if (!model) return ISLE_GAME_INVALID_STATE;
    IsleGameVec3 first_position;
    result = position(game, model, false, first_position);
    if (result != ISLE_GAME_OK) return result;
    result = isle_actor_spatial_object_model(game, second, &model);
    if (result != ISLE_GAME_OK) return result;
    if (!model) {
        *out_distance = std::numeric_limits<float>::max();
        return ISLE_GAME_OK;
    }
    IsleGameVec3 second_position;
    result = position(game, model, false, second_position);
    if (result != ISLE_GAME_OK) return result;
    IsleGameVec3 vector;
    *out_distance = direction(vector, first_position, second_position);
    return ISLE_GAME_OK;
}

extern "C" IsleGameResult isle_actor_spatial_neighbors(IsleActorSource* actor,
    IsleGameVec3* out_direction, float* out_magnitude) {
    if (!actor || !out_direction || !out_magnitude) return ISLE_GAME_INVALID_ARGUMENT;
    IsleActorSource* bound = nullptr;
    auto result = isle_actor_source_get(actor->game, actor->object, &bound);
    if (result != ISLE_GAME_OK) return result;
    if (bound != actor) return ISLE_GAME_INVALID_STATE;
    IsleGameModel* model = nullptr;
    result = isle_actor_spatial_object_model(actor->game, actor->object, &model);
    if (result != ISLE_GAME_OK) return result;
    if (!model) return ISLE_GAME_INVALID_STATE;
    IsleGameVec3 self_position;
    result = position(actor->game, model, true, self_position);
    if (result != ISLE_GAME_OK) return result;

    IsleGameVec3 sum{0.0f, 0.0f, 0.0f};
    bool has_neighbor = false;
    IsleGameObject* previous = nullptr;
    for (;;) {
        IsleGameObject* object = nullptr;
        IsleActorSource* source = nullptr;
        result = isle_actor_source_link3_next(actor->game, previous, &object, &source, &model);
        if (result != ISLE_GAME_OK) return result;
        if (!object) break;
        previous = object;
        if (object == actor->object) continue;
        IsleGameVec3 neighbor_position;
        result = position(actor->game, model, true, neighbor_position);
        if (result != ISLE_GAME_OK) return result;
        IsleGameVec3 vector;
        const float distance = direction(vector, self_position, neighbor_position);
        if (distance < 100.0f) {
            has_neighbor = true;
            sum.x += distance * vector.x;
            sum.y += distance * vector.y;
            sum.z += distance * vector.z;
        }
    }
    const float magnitude = has_neighbor ? normalize(sum) : 0.0f;
    *out_direction = sum;
    *out_magnitude = magnitude;
    return ISLE_GAME_OK;
}
