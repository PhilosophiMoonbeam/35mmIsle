#include "actor_photo_eligibility.hpp"
#include "actor_owner_source.h"
#include <cmath>
#include <cstddef>
#include <cstdint>

// Reuse the already admitted own-GAME math definitions, not SDK producers.
extern "C" void hal_rotate_rpy_translate_f(float [4][4], float, float, float,
                                          float, float, float);
extern "C" void hal_look_at_f(float [4][4], float, float, float, float, float, float,
                             float, float, float);
extern "C" void hal_perspective_fast_f(float [4][4], std::uint16_t*,
                                       float, float, float, float, float);

namespace isle::actor::photo {
namespace {
constexpr std::uint16_t root_offset_flag = 0x001;
constexpr std::uint16_t visibility_bypass_flag = 0x040;
constexpr std::uint16_t invisible_flag = 0x100;
constexpr std::uint32_t hidden_flag = 1;

bool finite(IsleGameVec3 v) noexcept {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}
bool finite(const float (&m)[4][4]) noexcept {
    for (const auto& row : m)
        for (float value : row)
            if (!std::isfinite(value)) return false;
    return true;
}
IsleGameResult bound(const IsleActorSource& actor) noexcept {
    IsleActorSource* actual = nullptr;
    const auto result = isle_actor_source_get(actor.game, actor.object, &actual);
    if (result != ISLE_GAME_OK) return result;
    return actual == &actor ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE;
}
// Independently authored public row-vector transform convention. Only the
// requested column is consumed; depth and X rejection must not validate Y's
// later cast or the entirely unused view_projection matrix/fourth column.
bool component(const Camera& camera, IsleGameVec3 p, unsigned column, float& out) noexcept {
    for (unsigned row = 0; row < 4; ++row)
        if (!std::isfinite(camera.view[row][column])) return false;
    out = ((camera.view[0][column] * p.x + camera.view[1][column] * p.y) +
           camera.view[2][column] * p.z) + camera.view[3][column];
    return std::isfinite(out);
}
bool screen_integer(float coordinate, float depth, std::int32_t& out) noexcept {
    const float projected = (coordinate * 228.506134f) / depth;
    // The positive signed32 endpoint rounds to 2147483648 in binary32. Compare
    // against the exclusive upper bound before any C++ integer conversion.
    if (!std::isfinite(projected) || projected < -2147483648.0f ||
        projected >= 2147483648.0f) return false;
    out = static_cast<std::int32_t>(projected);
    return true;
}
// Bound-record admission is shared by public callers but never runs source
// waits/faults. Publish only after the last consumed arithmetic has succeeded.
IsleGameResult visibility(const IsleActorSource& actor, const Camera& camera,
                          IsleGameVec3 collision, bool& visible) noexcept {
    if (actor.flags & visibility_bypass_flag) {
        visible = true;
        return ISLE_GAME_OK;
    }
    if (!std::isfinite(actor.playerDist)) return ISLE_GAME_INVALID_ARGUMENT;
    if (10000.0f < actor.playerDist) {
        visible = false;
        return ISLE_GAME_OK;
    }
    return point_visible(camera, collision, visible);
}
void publish_visibility(IsleActorSource& actor, bool visible) noexcept {
    if (visible) actor.flags &= static_cast<std::uint16_t>(~invisible_flag);
    else actor.flags |= invisible_flag;
}
} // namespace

IsleGameResult prepare_camera(const IsleGameCameraParameters& parameters, Camera& output) noexcept {
    const auto& p = parameters.perspective;
    const auto& v = parameters.look_at;
    // Same native perspective domain; in particular bound GAME's fast-trig
    // signed integer conversion before calling it. No fixed16.16/render bounds.
    if (!std::isfinite(p.fovy) || !std::isfinite(p.aspect) ||
        !std::isfinite(p.near_plane) || !std::isfinite(p.far_plane) ||
        !std::isfinite(p.scale) || p.fovy <= 0 || p.fovy >= 180 || p.aspect <= 0 ||
        p.near_plane <= 0 || p.far_plane <= p.near_plane || p.scale <= 0)
        return ISLE_GAME_INVALID_ARGUMENT;
    for (unsigned axis = 0; axis < 3; ++axis)
        if (!std::isfinite(v.eye[axis]) || !std::isfinite(v.at[axis]) ||
            !std::isfinite(v.up[axis])) return ISLE_GAME_INVALID_ARGUMENT;
    Camera candidate;
    float projection[4][4];
    // GAME 5047F0.c19-26 prepares perspective first, then explicit look-at.
    hal_perspective_fast_f(projection, nullptr, p.fovy, p.aspect,
                            p.near_plane, p.far_plane, p.scale);
    if (!finite(projection)) return ISLE_GAME_INVALID_ARGUMENT;
    hal_look_at_f(candidate.view, v.eye[0], v.eye[1], v.eye[2],
                 v.at[0], v.at[1], v.at[2], v.up[0], v.up[1], v.up[2]);
    if (!finite(candidate.view)) return ISLE_GAME_INVALID_ARGUMENT;
    // Public row-matrix concatenation, independently authored, left-associated
    // binary32 four-term products; no imported guMtxCatF body or SDK layout.
    for (unsigned row = 0; row < 4; ++row)
        for (unsigned column = 0; column < 4; ++column) {
            const float value = ((candidate.view[row][0] * projection[0][column] +
                candidate.view[row][1] * projection[1][column]) +
                candidate.view[row][2] * projection[2][column]) +
                candidate.view[row][3] * projection[3][column];
            if (!std::isfinite(value)) return ISLE_GAME_INVALID_ARGUMENT;
            candidate.view_projection[row][column] = value;
        }
    output = candidate;
    return ISLE_GAME_OK;
}
IsleGameResult prepare_camera(IsleGame* game, IsleGameCamera* camera, Camera& output) noexcept {
    const IsleGameCameraParameters* parameters = nullptr;
    const auto result = isle_game_camera_parameters(game, camera, &parameters);
    if (result != ISLE_GAME_OK) return result;
    if (!parameters) return ISLE_GAME_INVALID_STATE;
    return prepare_camera(*parameters, output);
}

IsleGameResult collision_position(const IsleActorSource& actor, IsleGameModel* model,
                                  IsleGameVec3& output) noexcept {
    auto result = bound(actor);
    if (result != ISLE_GAME_OK) return result;
    result = isle_actor_source_validate_model(actor.game, actor.object, model);
    if (result != ISLE_GAME_OK) return result;
    const IsleGameModelPlacement* placement = nullptr;
    result = isle_game_model_placement(actor.game, model, &placement);
    if (result != ISLE_GAME_OK) return result;
    if (!placement || (placement->mode != ISLE_GAME_MODEL_PLACEMENT_RT &&
                       placement->mode != ISLE_GAME_MODEL_PLACEMENT_RTS))
        return ISLE_GAME_INVALID_STATE;
    const auto& auxiliary = placement->transform;
    if (!finite(auxiliary.position) || !finite(auxiliary.rotation) || !finite(actor.collisionOffset))
        return ISLE_GAME_INVALID_STATE;
    float rt[4][4];
    hal_rotate_rpy_translate_f(rt, auxiliary.position.x, auxiliary.position.y, auxiliary.position.z,
                              auxiliary.rotation.x, auxiliary.rotation.y, auxiliary.rotation.z);
    if (!finite(rt)) return ISLE_GAME_INVALID_STATE;
    IsleGameVec3 offset = actor.collisionOffset;
    if (actor.flags & root_offset_flag) {
        const IsleGameModelNode* nodes = nullptr;
        std::size_t count = 0;
        result = isle_game_model_peek_nodes(actor.game, model, &nodes, &count);
        if (result != ISLE_GAME_OK) return result;
        if (!nodes || !count || nodes[0].parent != UINT32_MAX ||
            !finite(nodes[0].transform.position) || !finite(auxiliary.scale))
            return ISLE_GAME_INVALID_STATE;
        const auto root = nodes[0].transform.position;
        offset.x += root.x * auxiliary.scale.x;
        offset.y += root.y * auxiliary.scale.y;
        offset.z += root.z * auxiliary.scale.z;
        if (!finite(offset)) return ISLE_GAME_INVALID_STATE;
    }
    // Only row3 of translate(offset) x AUX-RT is observed by the source. The
    // collisionOffset itself is NOT scaled; root rotation/scale never enter it.
    IsleGameVec3 candidate{
        ((offset.x * rt[0][0] + offset.y * rt[1][0]) + offset.z * rt[2][0]) + rt[3][0],
        ((offset.x * rt[0][1] + offset.y * rt[1][1]) + offset.z * rt[2][1]) + rt[3][1],
        ((offset.x * rt[0][2] + offset.y * rt[1][2]) + offset.z * rt[2][2]) + rt[3][2]};
    if (!finite(candidate)) return ISLE_GAME_INVALID_STATE;
    output = candidate;
    return ISLE_GAME_OK;
}

IsleGameResult point_visible(const Camera& camera, IsleGameVec3 point, bool& output) noexcept {
    if (!finite(point)) return ISLE_GAME_INVALID_ARGUMENT;
    float depth;
    if (!component(camera, point, 2, depth)) return ISLE_GAME_INVALID_ARGUMENT;
    if (depth > -1.0f) { output = false; return ISLE_GAME_OK; }
    if (depth < -10000.0f) { output = false; return ISLE_GAME_OK; }
    float coordinate;
    std::int32_t screen;
    if (!component(camera, point, 0, coordinate) || !screen_integer(coordinate, depth, screen))
        return ISLE_GAME_INVALID_ARGUMENT;
    if (screen < -240 || screen > 240) { output = false; return ISLE_GAME_OK; }
    if (!component(camera, point, 1, coordinate) || !screen_integer(coordinate, depth, screen))
        return ISLE_GAME_INVALID_ARGUMENT;
    if (screen < -180 || screen > 180) { output = false; return ISLE_GAME_OK; }
    output = true;
    return ISLE_GAME_OK;
}
IsleGameResult update_visibility(IsleActorSource& actor, const Camera& camera,
                                 IsleGameVec3 collision, bool& output) noexcept {
    const auto guard = bound(actor);
    if (guard != ISLE_GAME_OK) return guard;
    bool visible;
    const auto result = visibility(actor, camera, collision, visible);
    if (result != ISLE_GAME_OK) return result;
    publish_visibility(actor, visible);
    output = visible;
    return ISLE_GAME_OK;
}
bool species_allowed(int32_t species) noexcept {
    // GAME 47380.c476-503, including effect/controller/egg exceptions, not the
    // narrower report-index table or a guessed set of encountered species.
    switch (species) {
    case 600: case 601: case 602: case 603:
    case 1001: case 1002: case 1003: case 1006:
    case 1008: case 1009: case 1010: case 1012: case 1013:
    case 1028: case 1029: case 1030: case 1031:
    case 1033: case 1037: case 1038:
        return true;
    default:
        return species > 0 && species <= 151;
    }
}
IsleGameResult admitted(IsleActorSource& actor, const Camera& camera, IsleGameVec3 collision,
                         bool& output) noexcept {
    const auto guard = bound(actor);
    if (guard != ISLE_GAME_OK) return guard;
    const auto species = actor.id;
    IsleGameObjectMetadata metadata{};
    const auto result = isle_game_object_metadata(actor.game, actor.object, &metadata);
    if (result != ISLE_GAME_OK) return result;
    if ((metadata.flags & hidden_flag) || !actor.tangible) {
        output = false;
        return ISLE_GAME_OK;
    }
    bool visible;
    const auto status = visibility(actor, camera, collision, visible);
    if (status != ISLE_GAME_OK) return status;
    publish_visibility(actor, visible);
    output = visible && species_allowed(species);
    return ISLE_GAME_OK;
}
} // namespace isle::actor::photo
