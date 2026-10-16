#include "actor_photo_eligibility.hpp"
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

namespace {
namespace photo = isle::actor::photo;
constexpr float quarter = 1.57079632679489661923f;
constexpr float nan = std::numeric_limits<float>::quiet_NaN();
constexpr float infinity = std::numeric_limits<float>::infinity();
constexpr float maximum = std::numeric_limits<float>::max();
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
void native(IsleGameResult result) { require(result == ISLE_GAME_OK, "native operation rejected"); }
void close(float actual, float expected, const char* message) {
    require(std::abs(actual - expected) < 0.002f, message);
}
void close(IsleGameVec3 actual, IsleGameVec3 expected) {
    close(actual.x, expected.x, "collision X disagrees with analytic oracle");
    close(actual.y, expected.y, "collision Y disagrees with analytic oracle");
    close(actual.z, expected.z, "collision Z disagrees with analytic oracle");
}
struct Fixture {
    std::vector<std::byte> bytes = std::vector<std::byte>(1024 * 1024);
    isle_arena arena{};
    IsleGame* game = nullptr;
    IsleGameObject* object = nullptr;
    IsleGameModel* model = nullptr;
    IsleActorSource actor{};
    isle_rng rng{};
    IsleGameModelTransform root{{2,3,-4},{quarter,quarter,quarter},{-7,11,13}};
    IsleGameModelTransform auxiliary{{100,200,300},{0,0,quarter},{2,-3,0.5f}};
    Fixture() {
        require(isle_arena_init(&arena, bytes.data(), bytes.size()) == ISLE_OK, "arena init");
        native(isle_game_create(&arena, &game));
        native(isle_game_add_object(game, 1, 0, 0, nullptr, nullptr, &object));
        native(isle_actor_source_bind(game, object, &actor, &rng, this));
        actor.id = 129;
        actor.tangible = 1;
        actor.collisionOffset = {5,7,11};
        IsleGameModelNodeInit initial{UINT32_MAX, root, ISLE_GAME_MODEL_CONTEXTUAL_RPY};
        native(isle_game_attach_model(game, object, &initial, 1, &model));
        native(isle_game_model_set_placement(game, model, ISLE_GAME_MODEL_PLACEMENT_RTS, &auxiliary));
    }
    ~Fixture() { if (game && isle_game_destroy(game) != ISLE_GAME_OK) std::abort(); }
    Fixture(const Fixture&) = delete;
    Fixture& operator=(const Fixture&) = delete;
};
photo::Camera identity_view() {
    photo::Camera camera;
    for (unsigned axis = 0; axis < 4; ++axis) camera.view[axis][axis] = 1;
    return camera;
}
IsleGameVec3 collision(Fixture& f) {
    IsleGameVec3 result{};
    native(photo::collision_position(f.actor, f.model, result));
    return result;
}
void collision_behavior() {
    Fixture f;
    f.actor.flags = 1;
    // offset=(5,7,11)+(2,3,-4)*(2,-3,.5)=(9,-2,9);
    // AUX Rz maps (x,y,z)->(-y,x,z), then adds (100,200,300).
    close(collision(f), {102,209,309});
    f.actor.flags = 0;
    close(collision(f), {93,205,311});
    f.actor.flags = 0xFFFF; // unrelated actor bits, including100/40, are ignored.
    close(collision(f), {102,209,309});
    native(isle_game_model_set_flags(f.game, f.model, 0, 3));
    f.root.rotation = {-quarter,0,quarter};
    f.root.scale = {0,-100,0.125f};
    native(isle_game_model_set_transform(f.game, f.model, 0, &f.root));
    close(collision(f), {102,209,309});
    // Three noncommuting quarter turns; root R/S still must not participate.
    // (9,-2,9) ->Rx(9,-9,-2) ->Ry(-2,-9,-9) ->Rz(9,-2,-9).
    f.auxiliary.rotation = {quarter,quarter,quarter};
    native(isle_game_model_set_placement(f.game, f.model, ISLE_GAME_MODEL_PLACEMENT_RTS, &f.auxiliary));
    close(collision(f), {109,198,291});
    f.auxiliary.rotation = {0,0,quarter};
    native(isle_game_model_set_placement(f.game, f.model, ISLE_GAME_MODEL_PLACEMENT_RT, &f.auxiliary));
    // Native RT retains canonical unit AUX scale, not the caller's unused scale.
    close(collision(f), {90,207,307});
    f.root.position = {-3,4,2};
    native(isle_game_model_set_transform(f.game, f.model, 0, &f.root));
    close(collision(f), {89,202,313});
    const IsleGameModelNode* nodes = nullptr;
    std::size_t count = 0;
    const IsleGameModelPlacement* placement = nullptr;
    native(isle_game_model_peek_nodes(f.game, f.model, &nodes, &count));
    native(isle_game_model_placement(f.game, f.model, &placement));
    require(count == 1, "fixture topology");
    const auto saved_node = nodes[0];
    const auto saved_placement = *placement;
    const auto cursor = f.arena.cursor;
    const auto flags = f.actor.flags;
    close(collision(f), {89,202,313});
    require(f.arena.cursor == cursor && f.actor.flags == flags &&
            std::memcmp(&saved_node, nodes, sizeof(saved_node)) == 0 &&
            std::memcmp(&saved_placement, placement, sizeof(saved_placement)) == 0,
            "collision calculation mutated native pose/cache/flags or arena");
}
void collision_errors() {
    Fixture f;
    const IsleGameVec3 sentinel{123,456,789};
    auto out = sentinel;
    native(isle_game_model_set_placement(f.game, f.model, ISLE_GAME_MODEL_PLACEMENT_DISABLED, nullptr));
    require(photo::collision_position(f.actor, f.model, out) == ISLE_GAME_INVALID_STATE,
            "absent canonical AUX became identity fallback");
    close(out, sentinel);
    native(isle_game_model_set_placement(f.game, f.model, ISLE_GAME_MODEL_PLACEMENT_RTS, &f.auxiliary));
    f.actor.flags = 1;
    f.root.position.x = maximum;
    native(isle_game_model_set_transform(f.game, f.model, 0, &f.root));
    require(photo::collision_position(f.actor, f.model, out) == ISLE_GAME_INVALID_STATE,
            "root times AUX scale overflow not reported");
    close(out, sentinel);
    f.actor.flags = 0; // Unused giant root position must not be multiplied/validated.
    close(collision(f), {93,205,311});
    f.actor.collisionOffset.x = nan;
    require(photo::collision_position(f.actor, f.model, out) == ISLE_GAME_INVALID_STATE,
            "nonfinite consumed collision offset accepted");
    close(out, sentinel);
    f.actor.collisionOffset = {5,7,11};
    auto copied = f.actor;
    require(photo::collision_position(copied, f.model, out) == ISLE_GAME_INVALID_STATE,
            "copied source record authorized canonical reads");
    require(photo::collision_position(f.actor, nullptr, out) != ISLE_GAME_OK,
            "null native model accepted");
    IsleGameObject* other = nullptr;
    IsleGameModel* wrong = nullptr;
    native(isle_game_add_object(f.game, 2, 0, 0, nullptr, nullptr, &other));
    IsleGameModelNodeInit root{UINT32_MAX, {{},{},{1,1,1}}, ISLE_GAME_MODEL_CONTEXTUAL_RPY};
    native(isle_game_attach_model(f.game, other, &root, 1, &wrong));
    require(photo::collision_position(f.actor, wrong, out) != ISLE_GAME_OK,
            "another live object's model accepted");
    close(out, sentinel);
}
IsleGameCameraParameters parameters() {
    IsleGameCameraParameters p{};
    p.perspective = {90,1,1,101,1};
    p.look_at = {{3,4,10},{3,4,0},{1,0,0}};
    return p;
}
float transformed(const float (&matrix)[4][4], IsleGameVec3 point, unsigned column) {
    return ((point.x * matrix[0][column] + point.y * matrix[1][column]) +
            point.z * matrix[2][column]) + matrix[3][column];
}
void camera_behavior() {
    Fixture f;
    auto p = parameters();
    photo::Camera camera;
    native(photo::prepare_camera(p, camera));
    const IsleGameVec3 probe{5,7,0};
    // Explicit X-up makes camera right=-Y, camera up=+X, camera back=+Z.
    close(transformed(camera.view, probe, 0), -3, "explicit camera up ignored");
    close(transformed(camera.view, probe, 1), 2, "explicit camera up ignored");
    close(transformed(camera.view, probe, 2), -10, "eye translation ignored");
    const float w = transformed(camera.view_projection, probe, 3);
    close(w, 10, "wrong view/projection composition order");
    close(transformed(camera.view_projection, probe, 0) / w, -0.3f, "projected X oracle");
    close(transformed(camera.view_projection, probe, 1) / w, 0.2f, "projected Y oracle");
    close(transformed(camera.view_projection, probe, 2) / w, 0.818f, "projected depth oracle");
    IsleGameObject* object = nullptr;
    IsleGameCamera* native_camera = nullptr;
    native(isle_game_add_object(f.game, 3, 0, 0, nullptr, nullptr, &object));
    native(isle_game_attach_camera(f.game, object, &native_camera));
    native(isle_game_camera_set_perspective(f.game, native_camera, &p.perspective));
    native(isle_game_camera_set_look_at(f.game, native_camera, &p.look_at));
    native(isle_game_camera_set_roll(f.game, native_camera, quarter));
    photo::Camera actual;
    native(photo::prepare_camera(f.game, native_camera, actual));
    require(std::memcmp(&actual, &camera, sizeof(camera)) == 0,
            "native photo preparation used roll instead of explicit up");
    native(isle_game_camera_set_roll(f.game, native_camera, -quarter));
    native(photo::prepare_camera(f.game, native_camera, actual));
    require(std::memcmp(&actual, &camera, sizeof(camera)) == 0, "photo camera consumed roll");
    p.look_at.up[0] = 0;
    p.look_at.up[1] = 1;
    native(photo::prepare_camera(p, actual));
    close(transformed(actual.view, probe, 0), 2, "changed up did not rotate view");
    close(transformed(actual.view, probe, 1), 3, "changed up did not rotate view");
    // These valid float coordinates exceed render fixed16.16 limits.
    p.look_at.eye[0] = p.look_at.at[0] = 10000000;
    native(photo::prepare_camera(p, actual));
    close(transformed(actual.view, {10000000,4,0}, 0), 0, "render bounds leaked into photo camera");
    // Point visibility does not consume prepared perspective or viewport.
    auto ordinary = identity_view();
    for (auto& row : ordinary.view_projection) for (float& value : row) value = nan;
    ordinary.view[0][3] = nan;
    bool visible = false;
    native(photo::point_visible(ordinary, {0,0,-10}, visible));
    require(visible, "visibility consumed unused projection/fourth column");
    const auto accepted = actual;
    auto reject = [&](IsleGameCameraParameters bad) {
        require(photo::prepare_camera(bad, actual) == ISLE_GAME_INVALID_ARGUMENT &&
                std::memcmp(&actual, &accepted, sizeof(actual)) == 0,
                "invalid camera published partial result");
    };
    auto bad = p; bad.perspective.fovy = nan; reject(bad);
    bad = p; bad.perspective.fovy = maximum; reject(bad);
    bad = p; bad.perspective.fovy = 0; reject(bad);
    bad = p; bad.perspective.fovy = 180; reject(bad);
    bad = p; bad.perspective.fovy = std::numeric_limits<float>::denorm_min(); reject(bad);
    bad = p; bad.perspective.aspect = 0; reject(bad);
    bad = p; bad.perspective.near_plane = bad.perspective.far_plane; reject(bad);
    bad = p; bad.perspective.scale = maximum; reject(bad);
    bad = p; bad.look_at.at[0] = nan; reject(bad);
    bad = p; bad.look_at.at[2] = bad.look_at.eye[2]; reject(bad);
    bad = p; bad.look_at.up[0] = bad.look_at.up[1] = bad.look_at.up[2] = 0; reject(bad);
    bad = p; bad.look_at.up[1] = 0; bad.look_at.up[2] = 1; reject(bad);
    require(photo::prepare_camera(f.game, nullptr, actual) != ISLE_GAME_OK &&
            std::memcmp(&actual, &accepted, sizeof(actual)) == 0,
            "null native camera published output");
}
void point_boundaries() {
    const auto camera = identity_view();
    auto expect = [&](IsleGameVec3 point, bool expected) {
        bool result = !expected;
        native(photo::point_visible(camera, point, result));
        require(result == expected, "depth/truncated screen visibility boundary");
    };
    expect({0,0,-1}, true);
    expect({0,0,std::nextafter(-1.0f, 0.0f)}, false);
    expect({0,0,-10000}, true);
    expect({0,0,std::nextafter(-10000.0f, -infinity)}, false);
    expect({0,0,0}, false);
    expect({0,0,1}, false);
    constexpr float focal = 228.506134f;
    for (float sign : {-1.0f,1.0f}) {
        expect({sign*240,0,-focal}, true);
        expect({sign*240.99f,0,-focal}, true);
        expect({sign*241.01f,0,-focal}, false);
        expect({0,sign*180,-focal}, true);
        expect({0,sign*180.99f,-focal}, true);
        expect({0,sign*181.01f,-focal}, false);
    }
    auto reject = [&](IsleGameVec3 point) {
        bool out = true;
        require(photo::point_visible(camera, point, out) == ISLE_GAME_INVALID_ARGUMENT && out,
                "unsafe screen arithmetic/cast failed to preserve output");
    };
    reject({nan,0,-10}); reject({0,infinity,-10}); reject({0,0,nan});
    reject({maximum,0,-1}); reject({0,maximum,-1});
    // With depth=-focal, power-of-two multiplication/division preserves the
    // exact endpoints. INT32_MIN is legal but invisible; +2^31 must not cast.
    constexpr float endpoint = 2147483648.0f;
    expect({endpoint,0,-focal}, false);
    expect({-std::nextafter(endpoint,0.0f),0,-focal}, false);
    reject({-endpoint,0,-focal});
    reject({std::nextafter(endpoint,infinity),0,-focal});
    reject({0,-endpoint,-focal});
    auto branch = camera;
    branch.view[0][1] = nan;
    bool out = true;
    native(photo::point_visible(branch, {300,0,-focal}, out));
    require(!out, "X rejection consumed later Y arithmetic");
    branch.view[0][0] = nan;
    out = true;
    native(photo::point_visible(branch, {0,0,-0.5f}, out));
    require(!out, "depth rejection consumed later screen arithmetic");
    out = true;
    require(photo::point_visible(branch, {0,0,-10}, out) == ISLE_GAME_INVALID_ARGUMENT && out,
            "consumed invalid X matrix accepted");
    branch = camera; branch.view[2][2] = maximum;
    require(photo::point_visible(branch, {0,0,-10}, out) == ISLE_GAME_INVALID_ARGUMENT && out,
            "view transform overflow accepted");
}
void visibility_policy() {
    Fixture f;
    const auto camera = identity_view();
    constexpr std::uint16_t unrelated = 0x8225;
    bool out = false;
    f.actor.flags = unrelated | 0x100;
    f.actor.playerDist = 10000;
    native(photo::update_visibility(f.actor, camera, {0,0,-10}, out));
    require(out && f.actor.flags == unrelated, "distance10000 must remain eligible and clear100");
    auto unusable = camera; unusable.view[2][2] = nan;
    f.actor.playerDist = std::nextafter(10000.0f, infinity);
    native(photo::update_visibility(f.actor, unusable, {nan,nan,nan}, out));
    require(!out && f.actor.flags == (unrelated | 0x100), "distance cutoff failed to set100 or used point");
    f.actor.flags = unrelated | 0x140;
    f.actor.playerDist = nan;
    native(photo::update_visibility(f.actor, unusable, {nan,nan,nan}, out));
    require(out && f.actor.flags == (unrelated | 0x40), "flag40 precedence/clear100/unused fields");
    f.actor.flags = unrelated;
    f.actor.playerDist = -1; // Source has no nonnegative-distance restriction.
    native(photo::update_visibility(f.actor, camera, {0,0,-0.5f}, out));
    require(!out && f.actor.flags == (unrelated | 0x100), "point failure must set100");
    native(photo::update_visibility(f.actor, camera, {0,0,-10}, out));
    require(out && f.actor.flags == unrelated, "point success must clear100");
    for (float bad : {nan,infinity,-infinity}) {
        f.actor.playerDist = bad;
        out = true;
        require(photo::update_visibility(f.actor, camera, {0,0,-10}, out) == ISLE_GAME_INVALID_ARGUMENT &&
                out && f.actor.flags == unrelated, "invalid consumed distance mutated visibility");
    }
    f.actor.playerDist = 0;
    out = false;
    require(photo::update_visibility(f.actor, camera, {-2147483648.0f,0,-228.506134f}, out) ==
            ISLE_GAME_INVALID_ARGUMENT && !out && f.actor.flags == unrelated,
            "out-of-int32 screen projection mutated flags/output");
    auto copy = f.actor;
    copy.flags = 0x140;
    require(photo::update_visibility(copy, camera, {0,0,-10}, out) == ISLE_GAME_INVALID_STATE &&
            copy.flags == 0x140 && !out, "unbound copy used flag40 to bypass ownership");
}
void admission_policy() {
    Fixture f;
    const auto camera = identity_view();
    auto unusable = camera; unusable.view[2][2] = nan;
    f.actor.flags = 0x100;
    f.actor.playerDist = nan;
    bool out = true;
    native(isle_game_object_set_flags(f.game, f.object, 1));
    native(photo::admitted(f.actor, unusable, {nan,nan,nan}, out));
    require(!out && f.actor.flags == 0x100, "hidden actor mutated visibility before rejection");
    native(isle_game_object_set_flags(f.game, f.object, 0));
    f.actor.tangible = 0;
    out = true;
    native(photo::admitted(f.actor, unusable, {nan,nan,nan}, out));
    require(!out && f.actor.flags == 0x100, "intangible actor mutated visibility before rejection");
    f.actor.tangible = -1; // Source truth, not tangible==1.
    f.actor.playerDist = 0;
    native(isle_game_object_set_flags(f.game, f.object, 2));
    native(photo::admitted(f.actor, camera, {0,0,-10}, out));
    require(out && f.actor.flags == 0, "unrelated GObj flags/tangible nonzero treated as hidden");
    const std::array<int,20> special{{600,601,602,603,1001,1002,1003,1006,1008,1009,
                                     1010,1012,1013,1028,1029,1030,1031,1033,1037,1038}};
    // Complete ordinary range, holes, controller/effect/egg exceptions, adjacent
    // values and rejected signs; independent set membership oracle.
    for (int species = -3; species <= 1100; ++species) {
        bool expected = species >= 1 && species <= 151;
        for (int value : special) if (species == value) expected = true;
        require(photo::species_allowed(species) == expected, "species whitelist boundary/hole");
        f.actor.id = species;
        f.actor.flags = 0x100;
        out = !expected;
        native(photo::admitted(f.actor, camera, {0,0,-10}, out));
        require(out == expected && f.actor.flags == 0,
                "admission whitelist or visibility-before-species mutation order");
    }
    require(!photo::species_allowed(INT32_MIN) && !photo::species_allowed(INT32_MAX),
            "out-of-range species overflow");
    f.actor.id = 129;
    f.actor.flags = 0;
    out = true;
    native(photo::admitted(f.actor, camera, {0,0,-0.5f}, out));
    require(!out && f.actor.flags == 0x100, "allowed species bypassed visibility");
    f.actor.id = 500;
    f.actor.flags = 0x140;
    f.actor.playerDist = nan;
    out = true;
    native(photo::admitted(f.actor, unusable, {nan,nan,nan}, out));
    require(!out && f.actor.flags == 0x40, "disallowed species must still receive flag40 clear100");
    f.actor.flags = 0;
    out = true;
    require(photo::admitted(f.actor, camera, {0,0,-10}, out) == ISLE_GAME_INVALID_ARGUMENT &&
            out && f.actor.flags == 0, "species rejection hid earlier consumed-distance error");
    auto copied = f.actor;
    copied.tangible = 0;
    require(photo::admitted(copied, camera, {0,0,-10}, out) == ISLE_GAME_INVALID_STATE && out,
            "intangible copied record bypassed native ownership");
}
} // namespace
int main() {
    try {
        collision_behavior(); collision_errors(); camera_behavior(); point_boundaries();
        visibility_policy(); admission_policy();
        std::cout << "actor photo collision/camera/eligibility contracts passed\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
