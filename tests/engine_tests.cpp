#include "engine_fixture.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cfenv>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <stdexcept>
#include <limits>
#include <thread>
#include <utility>
#include <vector>

namespace {
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
void ok(IsleGameResult result) { require(result == ISLE_GAME_OK, "native game operation rejected"); }

class World {
public:
    explicit World(std::size_t size = 65536) : bytes(size) {
        require(reinterpret_cast<std::uintptr_t>(bytes.data()) > UINT32_MAX,
                "test needs actual high-address backing storage");
        require(isle_arena_init(&arena, bytes.data(), bytes.size()) == ISLE_OK, "arena init");
        ok(isle_game_create(&arena, &game));
    }
    ~World() {
        if (game != nullptr && isle_game_destroy(game) != ISLE_GAME_OK) std::abort();
    }
    void reset() {
        ok(isle_game_destroy(game));
        game = nullptr;
        require(isle_arena_reset(&arena) == ISLE_OK, "quiescent arena reset");
        ok(isle_game_create(&arena, &game));
    }
    World(const World&) = delete;
    World& operator=(const World&) = delete;
    std::vector<unsigned char> bytes;
    isle_arena arena{};
    IsleGame* game = nullptr;
};

IsleGameObject* object(World& world, std::uint32_t id, std::uint8_t link = 0,
                       std::uint32_t priority = 0, IsleGameCallback update = nullptr,
                       void* user = nullptr) {
    IsleGameObject* result = nullptr;
    ok(isle_game_add_object(world.game, id, link, priority, update, user, &result));
    return result;
}
IsleGameProcess* process(World& world, IsleGameObject* owner, IsleGameCallback entry,
                         void* user, IsleGameProcessKind kind, std::uint32_t priority) {
    IsleGameProcess* result = nullptr;
    ok(isle_game_add_process(world.game, owner, entry, user, kind, priority, &result));
    return result;
}
IsleGameCounts counts(World& world) {
    IsleGameCounts result{};
    ok(isle_game_counts(world.game, &result));
    return result;
}
void trace(const EngineFixture& fixture, std::initializer_list<int> expected) {
    require(fixture.failures == 0, "compiled-C callback contract failed");
    require(fixture.count == expected.size(), "authoritative dispatch trace extent differs");
    std::size_t index = 0;
    for (int value : expected) require(fixture.events[index++] == value, "authoritative dispatch order differs");
}

void update_and_process_order() {
    World world;
    EngineFixture fixture{}; fixture.game = world.game;
    auto* a = object(world, 10, 0, 20, engine_record_object, &fixture);
    auto* b = object(world, 11, 0, 10, engine_record_object, &fixture);
    auto* c = object(world, 12, 1, UINT32_MAX, engine_record_object, &fixture);
    auto* d = object(world, 13, 0, 20, engine_record_object, &fixture);
    std::array<EngineRecord, 4> records{{{&fixture, 130}, {&fixture, 100}, {&fixture, 120}, {&fixture, 110}}};
    process(world, d, engine_record_process, &records[0], ISLE_GAME_SYNCHRONOUS, 5);
    process(world, a, engine_record_process, &records[1], ISLE_GAME_SYNCHRONOUS, 5);
    process(world, c, engine_record_process, &records[2], ISLE_GAME_SYNCHRONOUS, 11);
    process(world, b, engine_record_process, &records[3], ISLE_GAME_SYNCHRONOUS, 0);
    ok(isle_game_tick(world.game));
    ok(isle_game_tick(world.game));
    trace(fixture, {10, 13, 11, 12, 120, 100, 130, 110,
                    10, 13, 11, 12, 120, 100, 130, 110});
}

void pause_preserves_nested_waits() {
    World world;
    EngineFixture fixture{}; fixture.game = world.game;
    auto* a = object(world, 1);
    auto* p = process(world, a, engine_nested_process, &fixture, ISLE_GAME_STACKFUL, 5);
    ok(isle_game_tick(world.game)); trace(fixture, {200, 201});
    ok(isle_game_tick(world.game)); trace(fixture, {200, 201, 202, 300});
    ok(isle_game_pause_process(world.game, p, 1));
    ok(isle_game_tick(world.game)); ok(isle_game_tick(world.game));
    trace(fixture, {200, 201, 202, 300});
    ok(isle_game_pause_process(world.game, p, 0));
    ok(isle_game_tick(world.game)); trace(fixture, {200, 201, 202, 300});
    ok(isle_game_tick(world.game)); trace(fixture, {200, 201, 202, 300, 301, 203});
    require(fixture.local_result == UINT64_C(0x1234567887654332), "nested C stack lost native-width local");
    require(counts(world).processes == 0, "normal return did not retire C process");
}

void callback_lifecycle_and_same_tick_child() {
    World world;
    EngineFixture fixture{}; fixture.game = world.game;
    auto* a = object(world, 1);
    fixture.victim = process(world, a, engine_victim, &fixture, ISLE_GAME_STACKFUL, 8);
    process(world, a, engine_parent, &fixture, ISLE_GAME_STACKFUL, 5);
    ok(isle_game_tick(world.game)); trace(fixture, {700, 800, 801, 900});
    require(fixture.retired_continued == 0 && fixture.after_finish == 0,
            "retired C callback resumed after cancellation/self-end");
    ok(isle_game_tick(world.game)); trace(fixture, {700, 800, 801, 900, 901});
    require(counts(world).processes == 0, "callback-created child was not owner-dispatched and retired");
}

void synchronous_end_and_update_pause() {
    World world;
    EngineFixture fixture{}; fixture.game = world.game;
    auto* a = object(world, 1, 0, 0, engine_record_object, &fixture);
    EngineRecord survivor{&fixture, 22};
    process(world, a, engine_end_synchronous, &fixture, ISLE_GAME_SYNCHRONOUS, 5);
    process(world, a, engine_record_process, &survivor, ISLE_GAME_SYNCHRONOUS, 4);
    ok(isle_game_tick(world.game)); trace(fixture, {1, 20, 21, 22});
    ok(isle_game_pause_updates(world.game, a, 1));
    ok(isle_game_tick(world.game)); trace(fixture, {1, 20, 21, 22, 22});
    ok(isle_game_pause_updates(world.game, a, 0));
    ok(isle_game_tick(world.game)); trace(fixture, {1, 20, 21, 22, 22, 1, 22});
    require(counts(world).processes == 1, "kind-1 return/end semantics differ");
}

void deferred_object_deletion() {
    for (int mode : {0, 1, 2}) {
        World world;
        EngineFixture fixture{}; fixture.game = world.game; fixture.mode = mode;
        auto* a = object(world, 1, 0, 1, mode == 2 ? engine_delete_self : nullptr,
                         mode == 2 ? &fixture : nullptr);
        auto* b = object(world, 2);
        EngineRecord forbidden{&fixture, 32}, survivor{&fixture, 40};
        if (mode != 2) process(world, a, engine_delete_self, &fixture,
                              mode == 0 ? ISLE_GAME_STACKFUL : ISLE_GAME_SYNCHRONOUS, 5);
        process(world, a, engine_record_process, &forbidden, ISLE_GAME_SYNCHRONOUS, 5);
        process(world, b, engine_record_process, &survivor, ISLE_GAME_SYNCHRONOUS, 5);
        ok(isle_game_tick(world.game)); trace(fixture, {30, 31, 40});
        require(fixture.retired_continued == 0, "self-deleted stackful C object resumed");
        auto live = counts(world);
        require(live.objects == 1 && live.processes == 1, "deferred deletion missed sibling process/object");
        ok(isle_game_tick(world.game)); trace(fixture, {30, 31, 40, 40});
    }
}

void actual_intro_fade_and_cold_reset() {
    World world;
    EngineFixture retired{}; retired.game = world.game;
    auto* a = object(world, 1);
    IsleSprite initial{};
    initial.red = 200; initial.green = 100; initial.blue = 50; initial.alpha = 77;
    IsleGameSprite* sprite = nullptr;
    ok(isle_game_add_sprite(world.game, a, &initial, &sprite));
    IsleSprite* view = nullptr;
    ok(isle_game_sprite_view(world.game, sprite, &view));
    IsleGameProcess* fade = nullptr;
    ok(isle_game_add_intro_fade(world.game, a, sprite, 4, 5, &fade));
    for (std::uint8_t expected : {std::uint8_t{191}, std::uint8_t{127}, std::uint8_t{63}, std::uint8_t{0}, std::uint8_t{0}}) {
        ok(isle_game_tick(world.game));
        require(view->red == expected && view->green == expected && view->blue == expected && view->alpha == 77,
                "actual authoritative C fade or native sprite field layout differs");
    }
    require(counts(world).processes == 0, "completed intro helper retained a process");
    process(world, a, engine_victim, &retired, ISLE_GAME_STACKFUL, 5);
    ok(isle_game_tick(world.game)); trace(retired, {700});
    world.reset();
    EngineFixture fresh{}; fresh.game = world.game;
    auto* b = object(world, 9, 0, 0, engine_record_object, &fresh);
    IsleGameSprite* fresh_sprite = nullptr;
    ok(isle_game_add_sprite(world.game, b, &initial, &fresh_sprite));
    ok(isle_game_sprite_view(world.game, fresh_sprite, &view));
    require(view->red == 200 && view->green == 100 && view->blue == 50, "cold native state inherited mutated tint");
    ok(isle_game_tick(world.game)); trace(fresh, {9});
    require(retired.retired_continued == 0 && retired.count == 1,
            "old world callback accessed storage after quiescent arena reuse");
}

void bounded_pool_reuse_and_failure_fence() {
    {
        World world(4096);
        EngineFixture fixture{}; fixture.game = world.game;
        std::vector<IsleGameObject*> handles;
        IsleGameResult result = ISLE_GAME_OK;
        for (std::uint32_t id = 1; result == ISLE_GAME_OK && id < 512; ++id) {
            IsleGameObject* added = nullptr;
            result = isle_game_add_object(world.game, id, 0, 0, engine_record_object, &fixture, &added);
            if (result == ISLE_GAME_OK) handles.push_back(added);
            else require(added == nullptr, "failed allocation published an object");
        }
        require(result == ISLE_GAME_EXHAUSTED, "bounded arena did not reject exhausted pool");
        ok(isle_game_tick(world.game));
        require(fixture.failures == 0 && fixture.count == handles.size(), "allocation rejection corrupted live object chain");
        for (auto* handle : handles) ok(isle_game_delete_object(world.game, handle));
        fixture.count = 0;
        object(world, 499, 0, 0, engine_record_object, &fixture);
        ok(isle_game_tick(world.game)); trace(fixture, {499});
    }
    {
        World world;
        EngineFixture fixture{}; fixture.game = world.game;
        auto* a = object(world, 1);
        process(world, a, engine_invalid_legacy_wait, &fixture, ISLE_GAME_SYNCHRONOUS, 5);
        require(isle_game_tick(world.game) == ISLE_GAME_EXECUTION_FAILURE, "invalid legacy C wait reported tick success");
        trace(fixture, {60});
        require(fixture.retired_continued == 0, "invalid C callback continued after its failure fence");
        require(isle_game_tick(world.game) == ISLE_GAME_EXECUTION_FAILURE, "failed world resumed callbacks");
        trace(fixture, {60});
    }
}
IsleGameCamera* camera(World& world, IsleGameObject* owner, bool matrices = true) {
    IsleGameCamera* result = nullptr;
    ok(isle_game_attach_camera(world.game, owner, &result));
    if (matrices) {
        ok(isle_game_camera_add_matrix(world.game, result, ISLE_GAME_CAMERA_PERSPECTIVE_FAST));
        ok(isle_game_camera_add_matrix(world.game, result, ISLE_GAME_CAMERA_LOOK_AT));
    }
    return result;
}

const IsleGameCameraMatrix* matrix(World& world, IsleGameCamera* owner, std::size_t index) {
    const IsleGameCameraMatrix* result = nullptr;
    ok(isle_game_camera_matrix(world.game, owner, index, &result));
    return result;
}

std::array<float, 4> transform(const float m[4][4], std::array<float, 4> point) {
    std::array<float, 4> result{};
    for (std::size_t col = 0; col < 4; ++col)
        for (std::size_t row = 0; row < 4; ++row) result[col] += point[row] * m[row][col];
    return result;
}

void close_to(float value, float expected, float epsilon = 0.0001f) {
    require(std::isfinite(value) && std::abs(value - expected) <= epsilon, "camera geometry differs");
}

void camera_geometry_and_cache() {
    World world;
    auto* owner = object(world, 1);
    auto* cam = camera(world, owner);
    auto* projection = matrix(world, cam, 0);
    auto* view = matrix(world, cam, 1);
    const IsleGameCameraParameters* parameters = nullptr;
    ok(isle_game_camera_parameters(world.game, cam, &parameters));
    auto eye = transform(view->floating, {parameters->look_at.eye[0], parameters->look_at.eye[1],
                                         parameters->look_at.eye[2], 1});
    close_to(eye[0], 0); close_to(eye[1], 0); close_to(eye[2], 0); close_to(eye[3], 1);
    auto at = transform(view->floating, {0, 0, 0, 1});
    require(at[2] < 0, "default camera target is not in front of the eye");
    for (const auto [distance, expected] :
         {std::pair{parameters->perspective.near_plane, -1.0f},
          std::pair{parameters->perspective.far_plane, 1.0f}}) {
        const auto point = transform(projection->floating, {0, 0, -distance, 1});
        close_to(point[2] / point[3], expected);
    }
    const auto original_projection_revision = projection->revision, original_view_revision = view->revision;
    const auto arena_used = world.arena.cursor;
    auto p = parameters->perspective;
    p.near_plane = 10; p.far_plane = 25600;
    ok(isle_game_camera_set_perspective(world.game, cam, &p));
    projection = matrix(world, cam, 0);
    std::uint16_t normalization = 0;
    ok(isle_game_camera_perspective_normalization(world.game, cam, &normalization));
    require(normalization == static_cast<std::uint16_t>(131072.0f / (10.0f + 25600.0f)),
            "photo perspective normalization differs");
    const auto narrow = projection->floating[1][1];
    const auto photo_revision = projection->revision;
    for (const auto [distance, expected] : {std::pair{10.0f, -1.0f}, std::pair{25600.0f, 1.0f}}) {
        const auto point = transform(projection->floating, {0, 0, -distance, 1});
        close_to(point[2] / point[3], expected);
    }
    p.fovy = 60;
    ok(isle_game_camera_set_perspective(world.game, cam, &p));
    require(matrix(world, cam, 0) == projection && projection->revision == photo_revision + 1,
            "projection update changed stable address or missed cache invalidation");
    require(narrow > projection->floating[1][1] * 2.0f, "wide photo FOV did not widen frustum");
    const float edge = 100.0f * std::tan(p.fovy * 3.14159265358979323846f / 360.0f);
    const auto edge_point = transform(projection->floating, {edge * p.aspect, edge, -100, 1});
    close_to(edge_point[0] / edge_point[3], 1, 0.01f);
    close_to(edge_point[1] / edge_point[3], 1, 0.01f);
    require(view == matrix(world, cam, 1) && view->revision == original_view_revision,
            "perspective changes invalidated unrelated view matrix");
    const auto revision = projection->revision;
    ok(isle_game_camera_set_perspective(world.game, cam, &p));
    auto viewport = parameters->viewport;
    viewport.scale[0] = viewport.translate[0] = 128;
    viewport.scale[1] = viewport.translate[1] = 96;
    ok(isle_game_camera_set_viewport(world.game, cam, &viewport));
    require(matrix(world, cam, 0)->revision == revision && matrix(world, cam, 1)->revision == original_view_revision,
            "identical settings or viewport dirtied computed matrices");
    IsleGameLookAt pose{{10, 20, 30}, {10, 20, 29}, {0, 1, 0}};
    ok(isle_game_camera_set_look_at(world.game, cam, &pose));
    require(matrix(world, cam, 1) == view && view->revision == original_view_revision + 1,
            "look-at update lost stable storage or cached old pose");
    eye = transform(view->floating, {10, 20, 30, 1});
    at = transform(view->floating, {10, 20, 29, 1});
    close_to(eye[0], 0); close_to(eye[1], 0); close_to(eye[2], 0);
    close_to(at[0], 0); close_to(at[1], 0); close_to(at[2], -1);
    const auto up = transform(view->floating, {10, 21, 30, 1});
    close_to(up[1], 1);
    ok(isle_game_camera_set_look_at(world.game, cam, &pose));
    require(matrix(world, cam, 1)->revision == original_view_revision + 1 &&
            matrix(world, cam, 0)->revision == revision && world.arena.cursor == arena_used &&
            photo_revision == original_projection_revision + 1, "cache updates allocated or recomputed unchanged data");
    for (auto* computed : {projection, view}) {
        for (std::size_t row = 0; row < 4; ++row) {
            for (std::size_t col = 0; col < 4; ++col) {
                const auto pair = row * 2 + col / 2;
                const auto integral = computed->fixed.m[pair / 4][pair % 4];
                const auto fraction = computed->fixed.m[2 + pair / 4][pair % 4];
                const auto shift = col % 2 == 0 ? 16u : 0u;
                const auto bits = (((integral >> shift) & 0xffffu) << 16) | ((fraction >> shift) & 0xffffu);
                std::int32_t signed_value;
                std::memcpy(&signed_value, &bits, sizeof(bits));
                close_to(static_cast<float>(signed_value) / 65536.0f, computed->floating[row][col], 1.0f / 65536.0f);
            }
        }
    }
    pose = {{12, -8, 4}, {-2, 3, -5}, {0, 1, 0}};
    ok(isle_game_camera_set_look_at(world.game, cam, &pose));
    view = matrix(world, cam, 1);
    eye = transform(view->floating, {12, -8, 4, 1});
    at = transform(view->floating, {-2, 3, -5, 1});
    close_to(eye[0], 0); close_to(eye[1], 0); close_to(eye[2], 0);
    close_to(at[0], 0); close_to(at[1], 0);
    close_to(at[2], -std::sqrt(14.0f * 14.0f + 11.0f * 11.0f + 9.0f * 9.0f));
    for (std::size_t a = 0; a < 3; ++a) {
        for (std::size_t b = 0; b < 3; ++b) {
            float dot = 0;
            for (std::size_t i = 0; i < 3; ++i) dot += view->floating[i][a] * view->floating[i][b];
            close_to(dot, a == b ? 1.0f : 0.0f);
        }
    }
}

const IsleGameCameraReflection* reflection(World& world, IsleGameCamera* cam, std::size_t index) {
    const IsleGameCameraReflection* result = nullptr;
    ok(isle_game_camera_reflection(world.game, cam, index, &result));
    return result;
}

bool same_camera_values(const IsleGameCameraMatrix& a, const IsleGameCameraMatrix& b) {
    return a.kind == b.kind && a.revision == b.revision &&
        std::memcmp(a.floating, b.floating, sizeof(a.floating)) == 0 &&
        std::memcmp(a.fixed.m, b.fixed.m, sizeof(a.fixed.m)) == 0;
}
bool same_reflection_values(const IsleGameCameraReflection& a, const IsleGameCameraReflection& b) {
    return a.revision == b.revision &&
        std::memcmp(a.direction, b.direction, sizeof(a.direction)) == 0 &&
        std::memcmp(a.color, b.color, sizeof(a.color)) == 0 &&
        std::memcmp(a.color_copy, b.color_copy, sizeof(a.color_copy)) == 0;
}

void camera_reflection_roll_geometry_and_state() {
    World world;
    auto* owner = object(world, 1);
    auto* cam = camera(world, owner, false);
    ok(isle_game_camera_add_matrix(world.game, cam, ISLE_GAME_CAMERA_LOOK_AT));
    ok(isle_game_camera_add_matrix(world.game, cam, ISLE_GAME_CAMERA_LOOK_AT_REFLECT_ROLL));
    IsleGameLookAt pose{{10, 20, 30}, {10, 20, 29}, {0, 1, 0}};
    ok(isle_game_camera_set_look_at(world.game, cam, &pose));
    auto* basic = matrix(world, cam, 0);
    auto* rolled = matrix(world, cam, 1);
    auto* reflected = reflection(world, cam, 1);
    require(reflected->direction[0][0] == 127 && reflected->direction[1][1] == 127 &&
            reflected->direction[0][1] == 0 && reflected->direction[0][2] == 0 &&
            reflected->direction[1][0] == 0 && reflected->direction[1][2] == 0,
            "axis-aligned reflection directions are not actual signed 1.7 vectors");
    for (std::size_t axis = 0; axis < 2; ++axis)
        for (std::size_t component = 0; component < 3; ++component)
            require(reflected->color[axis][component] == (axis == 1 && component == 1 ? 128 : 0) &&
                    reflected->color_copy[axis][component] == reflected->color[axis][component],
                    "GAME reflection colors were fabricated or omitted");
    const auto initial = *rolled;
    const auto initial_reflection = *reflected;
    const auto basic_revision = basic->revision;
    const auto used = world.arena.cursor;
    constexpr float half_pi = 1.57079632679489661923f;
    for (const float roll : {half_pi, -half_pi, 0.47f, -2.3f, 0.0f}) {
        ok(isle_game_camera_set_roll(world.game, cam, roll));
        float stored = 99;
        ok(isle_game_camera_roll(world.game, cam, &stored));
        require(stored == roll, "roll property did not retain finite radians");
        const auto old_revision = rolled->revision;
        require(reflection(world, cam, 1) == reflected && matrix(world, cam, 1) == rolled &&
                rolled->revision == old_revision + 1 && reflected->revision == rolled->revision,
                "matrix/reflection borrows did not share one stable lazy cache revision");
        close_to(rolled->floating[0][0], std::cos(roll));
        close_to(rolled->floating[1][0], std::sin(roll));
        close_to(rolled->floating[0][1], -std::sin(roll));
        close_to(rolled->floating[1][1], std::cos(roll));
        const auto eye = transform(rolled->floating, {10, 20, 30, 1});
        const auto at = transform(rolled->floating, {10, 20, 29, 1});
        close_to(eye[0], 0); close_to(eye[1], 0); close_to(eye[2], 0);
        close_to(at[0], 0); close_to(at[1], 0); close_to(at[2], -1);
        for (std::size_t axis = 0; axis < 2; ++axis)
            for (std::size_t component = 0; component < 3; ++component) {
                const float scaled = rolled->floating[component][axis] * 128;
                const auto expected = static_cast<int>(std::min(scaled, 127.0f));
                require(reflected->direction[axis][component] == expected &&
                        expected >= -128 && expected <= 127,
                        "reflection quantization/sign is not derived from rolled camera basis");
            }
        if (roll == half_pi)
            require(reflected->direction[0][1] == 127 && reflected->direction[1][0] == -128,
                    "positive roll reflected directions have wrong handedness/endpoints");
        ok(isle_game_camera_set_roll(world.game, cam, roll));
        require(matrix(world, cam, 0)->revision == basic_revision &&
                matrix(world, cam, 1)->revision == old_revision + 1 && world.arena.cursor == used,
                "roll recomputed basic/unchanged matrices or allocated");
    }
    require(std::memcmp(initial.floating, rolled->floating, sizeof(initial.floating)) == 0 &&
            std::memcmp(&initial.fixed, &rolled->fixed, sizeof(initial.fixed)) == 0 &&
            std::memcmp(initial_reflection.direction, reflected->direction,
                        sizeof(initial_reflection.direction)) == 0,
            "restoring zero roll did not restore camera and reflection values");
    for (const IsleGameLookAt arbitrary : {
            IsleGameLookAt{{5, 0, 0}, {0, 0, 0}, {0, 1, 0}},
            IsleGameLookAt{{-5, 0, 0}, {0, 0, 0}, {0, 1, 0}},
            IsleGameLookAt{{12, -8, 4}, {-2, 3, -5}, {0.2f, 2, 0.4f}}}) {
        ok(isle_game_camera_set_look_at(world.game, cam, &arbitrary));
        const auto unrolled = *matrix(world, cam, 0);
        ok(isle_game_camera_set_roll(world.game, cam, 0.61f));
        rolled = matrix(world, cam, 1);
        for (std::size_t c = 0; c < 3; ++c) {
            close_to(rolled->floating[c][0], std::cos(0.61f) * unrolled.floating[c][0] +
                     std::sin(0.61f) * unrolled.floating[c][1]);
            close_to(rolled->floating[c][1], -std::sin(0.61f) * unrolled.floating[c][0] +
                     std::cos(0.61f) * unrolled.floating[c][1]);
            close_to(rolled->floating[c][2], unrolled.floating[c][2]);
        }
        require(reflection(world, cam, 1)->revision == rolled->revision,
                "arbitrary-axis pose change did not invalidate reflection cache");
    }
    ok(isle_game_camera_set_look_at(world.game, cam, &pose));
    std::fenv_t environment;
    require(std::fegetenv(&environment) == 0, "cannot save camera test floating environment");
    for (int rounding : {FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO, FE_TONEAREST}) {
        require(std::fesetround(rounding) == 0, "cannot select camera test rounding");
        std::feclearexcept(FE_ALL_EXCEPT);
        ok(isle_game_camera_set_roll(world.game, cam, static_cast<float>(rounding) * 0.0001f + 0.7f));
        const auto* checked = reflection(world, cam, 1);
        require(checked->revision == rolled->revision && std::fegetround() == rounding &&
                (std::fetestexcept(FE_INVALID | FE_DIVBYZERO | FE_OVERFLOW)) == 0,
                "reflection computation changed rounding or raised invalid/divide/overflow exceptions");
    }
    require(std::fesetenv(&environment) == 0, "cannot restore camera test floating environment");
    ok(isle_game_camera_set_roll(world.game, cam, 0));
    reflection(world, cam, 1);
    const auto revision = rolled->revision;
    const auto stable = *rolled;
    const auto stable_reflection = *reflected;
    std::feclearexcept(FE_ALL_EXCEPT);
    for (float invalid : {std::numeric_limits<float>::infinity(),
                          -std::numeric_limits<float>::infinity(),
                          std::numeric_limits<float>::quiet_NaN()})
        require(isle_game_camera_set_roll(world.game, cam, invalid) == ISLE_GAME_INVALID_ARGUMENT,
                "nonfinite roll mutated a camera");
    auto bad = pose;
    bad.at[2] = bad.eye[2];
    require(isle_game_camera_set_look_at(world.game, cam, &bad) == ISLE_GAME_INVALID_ARGUMENT,
            "reflection camera accepted zero-length look vector");
    require((std::fetestexcept(FE_INVALID | FE_DIVBYZERO)) == 0 &&
            matrix(world, cam, 1)->revision == revision,
            "rejected mutation performed invalid floating math or dirtied cache");
    auto far = pose; far.eye[0] = far.at[0] = 40000;
    ok(isle_game_camera_set_look_at(world.game, cam, &far));
    const IsleGameCameraReflection* failed = reflected;
    require(isle_game_camera_reflection(world.game, cam, 1, &failed) == ISLE_GAME_INVALID_ARGUMENT &&
            failed == nullptr && same_camera_values(stable, *rolled) &&
            same_reflection_values(stable_reflection, *reflected),
            "failed fixed computation exposed partial matrix/reflection cache contents");
    ok(isle_game_camera_set_look_at(world.game, cam, &pose));
    require(reflection(world, cam, 1)->revision == revision + 1,
            "pose restoration failed to refresh reflection cache");
    failed = reflected;
    require(isle_game_camera_reflection(world.game, cam, 0, &failed) == ISLE_GAME_INVALID_STATE && !failed,
            "basic camera fabricated reflection output");
    failed = reflected;
    require(isle_game_camera_reflection(world.game, cam, 2, &failed) == ISLE_GAME_INVALID_ARGUMENT && !failed,
            "reflection query accepted out-of-range matrix index");
    require(isle_game_camera_reflection(world.game, cam, 1, nullptr) == ISLE_GAME_INVALID_ARGUMENT &&
            isle_game_camera_roll(world.game, cam, nullptr) == ISLE_GAME_INVALID_ARGUMENT,
            "reflection/roll query accepted null output");
    bool rejected = false;
    std::thread foreign([&] {
        const IsleGameCameraReflection* output = reflected;
        float roll = 99;
        rejected = isle_game_camera_reflection(world.game, cam, 1, &output) == ISLE_GAME_WRONG_OWNER &&
            output == nullptr && isle_game_camera_set_roll(world.game, cam, 1) == ISLE_GAME_WRONG_OWNER &&
            isle_game_camera_roll(world.game, cam, &roll) == ISLE_GAME_WRONG_OWNER && roll == 0;
    });
    foreign.join();
    require(rejected, "reflection camera admitted foreign owner access");
    ok(isle_game_delete_object(world.game, owner));
    failed = reflected;
    require(isle_game_camera_reflection(world.game, cam, 1, &failed) == ISLE_GAME_INVALID_ARGUMENT &&
            !failed && counts(world).matrices == 0 && counts(world).cameras == 0,
            "deleted camera retained a live reflection borrow");
    auto* replacement = object(world, 2);
    cam = camera(world, replacement, false);
    ok(isle_game_camera_add_matrix(world.game, cam, ISLE_GAME_CAMERA_LOOK_AT_REFLECT_ROLL));
    float roll = 99;
    ok(isle_game_camera_roll(world.game, cam, &roll));
    require(roll == 0 && reflection(world, cam, 0)->revision == 1 && world.arena.cursor == used,
            "recycled reflection camera inherited roll/cache or failed actual-size arena reuse");
}

void camera_batch_failure_is_atomic() {
    World world;
    auto* cam = camera(world, object(world, 1), false);
    ok(isle_game_camera_add_matrix(world.game, cam, ISLE_GAME_CAMERA_PERSPECTIVE_FAST));
    ok(isle_game_camera_add_matrix(world.game, cam, ISLE_GAME_CAMERA_LOOK_AT_REFLECT_ROLL));
    IsleGamePerspective perspective{60, 4.0f / 3, 10, 25600, 1};
    const IsleGameLookAt pose{{0, 0, 0}, {0, 0, -1}, {0, 1, 0}};
    ok(isle_game_camera_set_perspective(world.game, cam, &perspective));
    ok(isle_game_camera_set_look_at(world.game, cam, &pose));
    ok(isle_game_camera_compute_all(world.game, cam));
    const auto* projection = matrix(world, cam, 0);
    const auto* view = matrix(world, cam, 1);
    const auto* directions = reflection(world, cam, 1);
    const auto previous_projection = *projection, previous_view = *view;
    const auto previous_directions = *directions;
    const auto used = world.arena.cursor;

    perspective.fovy = 45;
    ok(isle_game_camera_set_perspective(world.game, cam, &perspective));
    auto unrepresentable = pose;
    unrepresentable.eye[0] = unrepresentable.at[0] = 40000;
    ok(isle_game_camera_set_look_at(world.game, cam, &unrepresentable));
    require(isle_game_camera_compute_all(world.game, cam) == ISLE_GAME_INVALID_ARGUMENT &&
            same_camera_values(previous_projection, *projection) &&
            same_camera_values(previous_view, *view) &&
            same_reflection_values(previous_directions, *directions) && world.arena.cursor == used,
            "failed second camera computation published the first payload or changed stable ownership");

    ok(isle_game_camera_set_look_at(world.game, cam, &pose));
    ok(isle_game_camera_compute_all(world.game, cam));
    require(projection->revision == previous_projection.revision + 1 &&
            view->revision == previous_view.revision + 1 &&
            directions->revision == previous_directions.revision + 1 &&
            std::memcmp(projection->fixed.m, previous_projection.fixed.m, sizeof(projection->fixed.m)) != 0 &&
            std::memcmp(view->fixed.m, previous_view.fixed.m, sizeof(view->fixed.m)) == 0,
            "camera batch recovery lost dirty inputs, double-published, or failed to restore source view");
    const auto recovered_projection = *projection, recovered_view = *view;
    const auto recovered_directions = *directions;
    ok(isle_game_camera_compute_all(world.game, cam));
    require(same_camera_values(recovered_projection, *projection) &&
            same_camera_values(recovered_view, *view) &&
            same_reflection_values(recovered_directions, *directions) && world.arena.cursor == used,
            "unchanged camera batch recomputed, replaced or allocated cached output");
}


void camera_rejection_and_typed_cleanup() {
    World world;
    auto* owner = object(world, 1);
    auto* sprite_owner = object(world, 2);
    IsleGameSprite* sprite = nullptr;
    ok(isle_game_add_sprite(world.game, sprite_owner, nullptr, &sprite));
    auto* cam = camera(world, owner, false);
    std::size_t extent = 99;
    ok(isle_game_camera_matrix_count(world.game, cam, &extent));
    require(extent == 0, "new camera unexpectedly owns matrix nodes");
    const IsleGameCameraMatrix* borrowed = nullptr;
    require(isle_game_camera_matrix(world.game, cam, 0, &borrowed) == ISLE_GAME_INVALID_ARGUMENT && !borrowed,
            "missing matrix was borrowed");
    std::uint16_t norm = 99;
    require(isle_game_camera_perspective_normalization(world.game, cam, &norm) == ISLE_GAME_INVALID_STATE && norm == 0,
            "normalization fabricated without a projection");
    IsleGameCamera* rejected = cam;
    require(isle_game_attach_camera(world.game, owner, &rejected) == ISLE_GAME_INVALID_STATE && !rejected,
            "attach overwrote existing camera");
    rejected = cam;
    require(isle_game_attach_camera(world.game, sprite_owner, &rejected) == ISLE_GAME_INVALID_STATE && !rejected,
            "attach overwrote sprite payload");
    auto* rejected_sprite = sprite;
    require(isle_game_add_sprite(world.game, owner, nullptr, &rejected_sprite) == ISLE_GAME_INVALID_ARGUMENT && !rejected_sprite,
            "sprite attach interpreted camera as sprite list");
    ok(isle_game_camera_add_matrix(world.game, cam, ISLE_GAME_CAMERA_PERSPECTIVE_FAST));
    require(isle_game_camera_add_matrix(world.game, cam, static_cast<IsleGameCameraMatrixKind>(1)) ==
                ISLE_GAME_INVALID_ARGUMENT, "unsupported matrix kind was silently accepted");
    ok(isle_game_camera_add_matrix(world.game, cam, ISLE_GAME_CAMERA_LOOK_AT));
    require(isle_game_camera_add_matrix(world.game, cam, ISLE_GAME_CAMERA_LOOK_AT) == ISLE_GAME_EXHAUSTED &&
            counts(world).matrices == 2, "third camera matrix corrupted bounded ownership");
    auto* stable = matrix(world, cam, 0);
    borrowed = stable;
    require(isle_game_camera_matrix(world.game, cam, 2, &borrowed) == ISLE_GAME_INVALID_ARGUMENT && !borrowed,
            "out-of-range matrix query leaked pointer output");
    IsleGamePerspective invalid{30, 0, 10, 25600, 1};
    require(isle_game_camera_set_perspective(world.game, cam, &invalid) == ISLE_GAME_INVALID_ARGUMENT,
            "zero aspect accepted");
    invalid = {std::numeric_limits<float>::quiet_NaN(), 1, 10, 25600, 1};
    require(isle_game_camera_set_perspective(world.game, cam, &invalid) == ISLE_GAME_INVALID_ARGUMENT,
            "nonfinite FOV accepted");
    IsleGameLookAt degenerate{{1, 2, 3}, {1, 2, 3}, {0, 1, 0}};
    require(isle_game_camera_set_look_at(world.game, cam, &degenerate) == ISLE_GAME_INVALID_ARGUMENT,
            "coincident eye and target accepted");
    degenerate = {{0, 0, 1}, {0, 0, 0}, {0, 0, 2}};
    require(isle_game_camera_set_look_at(world.game, cam, &degenerate) == ISLE_GAME_INVALID_ARGUMENT,
            "parallel up/look accepted");
    const auto stable_revision = stable->revision;
    require(matrix(world, cam, 0)->revision == stable_revision, "rejected input dirtied projection");
    IsleGameLookAt too_far{{40000, 0, 0}, {0, 0, 0}, {0, 0, 1}};
    ok(isle_game_camera_set_look_at(world.game, cam, &too_far));
    borrowed = stable;
    require(isle_game_camera_matrix(world.game, cam, 1, &borrowed) == ISLE_GAME_INVALID_ARGUMENT && !borrowed,
            "unrepresentable fixed translation wrapped");
    bool wrong_owner = false;
    std::thread other([&] {
        const IsleGameCameraMatrix* matrix_output = stable;
        const IsleGameCameraParameters* parameter_output = nullptr;
        IsleGameCamera* camera_output = cam;
        wrong_owner = isle_game_camera_matrix(world.game, cam, 0, &matrix_output) == ISLE_GAME_WRONG_OWNER &&
            matrix_output == nullptr &&
            isle_game_camera_parameters(world.game, cam, &parameter_output) == ISLE_GAME_WRONG_OWNER &&
            parameter_output == nullptr &&
            isle_game_attach_camera(world.game, owner, &camera_output) == ISLE_GAME_WRONG_OWNER &&
            camera_output == nullptr && isle_game_remove_camera(world.game, cam) == ISLE_GAME_WRONG_OWNER;
    });
    other.join();
    require(wrong_owner, "camera ownership admitted foreign-thread access");
    ok(isle_game_delete_object(world.game, owner));
    auto live = counts(world);
    require(live.cameras == 0 && live.matrices == 0 && live.sprites == 1 && live.objects == 1,
            "typed camera deletion freed sprite or leaked matrices");
    IsleSprite* sprite_view = nullptr;
    ok(isle_game_sprite_view(world.game, sprite, &sprite_view));
    auto* replacement = object(world, 3);
    cam = camera(world, replacement);
    require(counts(world).cameras == 1 && counts(world).matrices == 2, "recycled camera counts differ");
    ok(isle_game_remove_camera(world.game, cam));
    const auto recycled_cursor = world.arena.cursor;
    cam = camera(world, replacement, false);
    ok(isle_game_camera_add_matrix(world.game, cam, ISLE_GAME_CAMERA_PERSPECTIVE_FAST));
    ok(isle_game_camera_add_matrix(world.game, cam, ISLE_GAME_CAMERA_PERSPECTIVE_FAST));
    require(isle_game_camera_add_matrix(world.game, cam, ISLE_GAME_CAMERA_LOOK_AT) == ISLE_GAME_EXHAUSTED &&
            counts(world).matrices == 2 && world.arena.cursor == recycled_cursor,
            "two same-kind GAME matrices failed bounded pool reuse");
    ok(isle_game_remove_camera(world.game, cam));
    ok(isle_game_add_sprite(world.game, replacement, nullptr, &rejected_sprite));
    ok(isle_game_delete_object(world.game, sprite_owner));
    require(counts(world).sprites == 1 && counts(world).cameras == 0, "sprite cleanup dispatched as camera");
}

void camera_exhaustion_and_reuse() {
    for (bool allocate_matrices : {false, true}) {
        World world(8192);
        auto* a = object(world, 1);
        auto* b = object(world, 2);
        auto* cam = camera(world, a, allocate_matrices);
        auto* first = allocate_matrices ? matrix(world, cam, 0) : nullptr;
        const auto used_before_exhaustion = world.arena.cursor;
        std::vector<IsleGameObject*> extras;
        for (;;) {
            IsleGameObject* extra = nullptr;
            const auto result = isle_game_add_object(world.game, 3, 0, 0, nullptr, nullptr, &extra);
            if (result == ISLE_GAME_EXHAUSTED) break;
            ok(result); extras.push_back(extra);
        }
        IsleGameCamera* failed = cam;
        require(isle_game_attach_camera(world.game, b, &failed) == ISLE_GAME_EXHAUSTED && !failed,
                "exhausted camera allocation published an owner");
        if (!allocate_matrices)
            require(isle_game_camera_add_matrix(world.game, cam, ISLE_GAME_CAMERA_PERSPECTIVE_FAST) == ISLE_GAME_EXHAUSTED &&
                    counts(world).matrices == 0, "failed matrix allocation published partial ownership");
        const auto full = world.arena.cursor;
        require(full >= used_before_exhaustion, "arena cursor regressed");
        ok(isle_game_remove_camera(world.game, cam));
        cam = camera(world, b, allocate_matrices);
        require(world.arena.cursor == full, "camera/matrix pool reuse allocated in exhausted arena");
        if (allocate_matrices) {
            auto* reused_projection = matrix(world, cam, 0);
            auto* reused_view = matrix(world, cam, 1);
            require(first == reused_projection || first == reused_view, "matrix free list did not reuse storage");
            require(reused_projection->revision == 1, "recycled projection inherited a computed cache");
        }
        ok(isle_game_delete_object(world.game, b));
        require(counts(world).cameras == 0 && counts(world).matrices == 0, "reused camera cleanup leaked nodes");
    }
}

IsleGameModelTransform identity_model_transform() {
    return {{0, 0, 0}, {0, 0, 0}, {1, 1, 1}};
}

std::int32_t fixed_lane(const IsleGameFixedMatrix& m, std::size_t row, std::size_t col) {
    const auto pair = row * 2 + col / 2;
    const auto shift = col % 2 == 0 ? 16u : 0u;
    const auto bits = (((m.m[pair / 4][pair % 4] >> shift) & 0xffffu) << 16) |
        ((m.m[2 + pair / 4][pair % 4] >> shift) & 0xffffu);
    std::int32_t value;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

void model_hierarchy_math_and_cache() {
    World world;
    auto* owner = object(world, 1);
    std::array<IsleGameModelNodeInit, 5> initial{};
    for (auto& node : initial) {
        node.transform = identity_model_transform();
        node.matrix_kind = ISLE_GAME_MODEL_ROTATE_RPY_TRANSLATE_SCALE;
    }
    initial[0].parent = UINT32_MAX;
    initial[1].parent = 0; initial[2].parent = 1;
    initial[3].parent = 0; initial[4].parent = 3;
    initial[2].transform.scale = {0.999999f, -0.2f, 1.25f};
    initial[2].transform.position = {1.5f, -2.25f, 3.125f};
    IsleGameModel* model = nullptr;
    ok(isle_game_attach_model(world.game, owner, initial.data(), initial.size(), &model));
    auto* cam_owner = object(world, 2);
    camera(world, cam_owner);
    const auto live = counts(world);
    require(live.models == 1 && live.model_nodes == 5 && live.matrices == 7,
            "model and camera matrix ownership was omitted or double counted");
    const IsleGameModelNode* nodes = nullptr;
    std::size_t count = 0;
    ok(isle_game_model_nodes(world.game, model, &nodes, &count));
    require(count == 5 && nodes[0].parent == UINT32_MAX && nodes[0].first_child == 1 &&
        nodes[1].next_sibling == 3 && nodes[1].first_child == 2 && nodes[3].first_child == 4 &&
        nodes[2].next_sibling == UINT32_MAX && nodes[4].first_child == UINT32_MAX,
        "parent-before-child hierarchy lost root, children or sibling order");
    require(fixed_lane(nodes[2].fixed, 0, 0) == 65280 &&
        fixed_lane(nodes[2].fixed, 1, 1) == -13056 &&
        fixed_lane(nodes[2].fixed, 2, 2) == 81920 &&
        fixed_lane(nodes[2].fixed, 3, 0) == 98304 &&
        fixed_lane(nodes[2].fixed, 3, 1) == -147456 &&
        fixed_lane(nodes[2].fixed, 3, 2) == 204800,
        "kind28 rounded scale to one or used float-to-fixed TRS instead of integer scales");
    const auto used = world.arena.cursor;
    const auto original = nodes[2].fixed;
    for (std::size_t i = 0; i < count; ++i) {
        ok(isle_game_model_set_transform(world.game, model, i, &initial[i].transform));
        ok(isle_game_model_set_flags(world.game, model, i, 3));
    }
    const IsleGameModelNode* again = nullptr;
    ok(isle_game_model_nodes(world.game, model, &again, &count));
    require(again == nodes && world.arena.cursor == used && nodes[2].revision == 1 &&
        std::memcmp(&nodes[2].fixed, &original, sizeof(original)) == 0,
        "unchanged transforms or flags allocated/recomputed/moved model storage");
    auto root = initial[0].transform;
    root.position.x = 9;
    ok(isle_game_model_set_transform(world.game, model, 0, &root));
    ok(isle_game_model_nodes(world.game, model, &again, &count));
    require(nodes[0].revision == 2 && nodes[1].revision == 1 && nodes[2].revision == 1,
        "root update recomputed child local matrices");
    auto turn = identity_model_transform();
    turn.rotation.y = 1.57079637f;
    turn.scale.x = 0.999999f;
    ok(isle_game_model_set_transform(world.game, model, 1, &turn));
    ok(isle_game_model_nodes(world.game, model, &again, &count));
    require(fixed_lane(nodes[1].fixed, 0, 2) == -65280 &&
        fixed_lane(nodes[1].fixed, 2, 0) == 65536 &&
        fixed_lane(nodes[1].fixed, 0, 0) == 0,
        "GAME table-angle quantization or signed scale arithmetic differs");
    turn = identity_model_transform();
    // ROM-observed binary32 MUL.S gives table index 14 here; evaluating the
    // unsuffixed mathematical PI ratio in double incorrectly gives index 15.
    turn.rotation.y = 0x1.78fdbap-6f;
    ok(isle_game_model_set_transform(world.game, model, 1, &turn));
    ok(isle_game_model_nodes(world.game, model, &again, &count));
    require(fixed_lane(nodes[1].fixed, 0, 2) == -1406 &&
        fixed_lane(nodes[1].fixed, 2, 0) == 1406,
        "kind28 angle quantization did not retain actual GAME binary32 multiplication");
    turn = identity_model_transform();
    turn.scale.x = 128; // GAME MULTU/MFLO + SRA retains the low product word.
    ok(isle_game_model_set_transform(world.game, model, 1, &turn));
    ok(isle_game_model_nodes(world.game, model, &again, &count));
    require(fixed_lane(nodes[1].fixed, 0, 0) == -8388608,
        "kind28 lost defined MIPS low-word multiplication semantics");
    require(world.arena.cursor == used, "model mutations allocated");
}

void model_translate_selection_and_cache() {
    World world;
    auto* owner = object(world, 1);
    std::array<IsleGameModelNodeInit, 2> initial{{
        {UINT32_MAX, identity_model_transform(), ISLE_GAME_MODEL_TRANSLATE},
        {0, identity_model_transform(), ISLE_GAME_MODEL_ROTATE_RPY_TRANSLATE_SCALE}
    }};
    initial[0].transform.position = {1.5f, -2.25f, 3.125f};
    const auto arena_before = world.arena.cursor;
    IsleGameModel* model = nullptr;
    auto rejected = initial;
    require(engine_attach_model_with_invalid_kind(world.game, owner, initial.data(), &model) ==
            ISLE_GAME_INVALID_ARGUMENT && !model && world.arena.cursor == arena_before &&
            counts(world).models == 0, "unknown matrix kind mutated model ownership/arena");
    for (unsigned channel = 0; channel < 3; ++channel) {
        rejected = initial;
        if (channel == 0) rejected[0].transform.rotation.y = 0.25f;
        if (channel == 1) rejected[0].transform.scale.x = 0.999f;
        if (channel == 2) rejected[0].transform.position.x = 32768;
        require(isle_game_attach_model(world.game, owner, rejected.data(), rejected.size(), &model) ==
                ISLE_GAME_INVALID_ARGUMENT && !model && world.arena.cursor == arena_before &&
                counts(world).models == 0, "invalid translate attachment changed ownership/arena");
    }
    ok(isle_game_attach_model(world.game, owner, initial.data(), initial.size(), &model));
    const IsleGameModelNode* nodes = nullptr;
    std::size_t extent = 0;
    ok(isle_game_model_nodes(world.game, model, &nodes, &extent));
    require(extent == 2 && nodes[0].matrix_kind == ISLE_GAME_MODEL_TRANSLATE &&
            nodes[1].matrix_kind == ISLE_GAME_MODEL_ROTATE_RPY_TRANSLATE_SCALE,
            "mixed native model matrix selections were lost");
    const auto* stable = nodes;
    const auto used = world.arena.cursor;
    const auto child_revision = nodes[1].revision;
    const std::array<std::int32_t, 3> expected_translation{98304, -147456, 204800};
    for (std::size_t row = 0; row < 4; ++row)
        for (std::size_t col = 0; col < 4; ++col)
            require(fixed_lane(nodes[0].fixed, row, col) ==
                    (row == 3 && col < 3 ? expected_translation[col] : (row == col ? 65536 : 0)),
                    "translate matrix is not exact identity plus source FTOFIX32 translation");
    const auto initial_fixed = nodes[0].fixed;
    const auto initial_revision = nodes[0].revision;
    ok(isle_game_model_set_transform(world.game, model, 0, &initial[0].transform));
    ok(isle_game_model_set_flags(world.game, model, 0, 1));
    ok(isle_game_model_nodes(world.game, model, &nodes, &extent));
    require(nodes == stable && nodes[0].revision == initial_revision && nodes[1].revision == child_revision &&
            world.arena.cursor == used, "unchanged translate/flags invalidated cache or replaced native borrow");
    for (unsigned channel = 0; channel < 5; ++channel) {
        auto invalid = initial[0].transform;
        if (channel == 0) invalid.rotation.x = 1;
        if (channel == 1) invalid.scale.z = 2;
        if (channel == 2) invalid.position.y = std::numeric_limits<float>::quiet_NaN();
        if (channel == 3) invalid.position.z = std::numeric_limits<float>::infinity();
        if (channel == 4) invalid.position.x = 32768;
        require(isle_game_model_set_transform(world.game, model, 0, &invalid) == ISLE_GAME_INVALID_ARGUMENT,
                "invalid translate mutation accepted");
        ok(isle_game_model_nodes(world.game, model, &nodes, &extent));
        require(nodes == stable && nodes[0].revision == initial_revision &&
                std::memcmp(&nodes[0].fixed, &initial_fixed, sizeof(initial_fixed)) == 0 &&
                nodes[0].transform.position.x == initial[0].transform.position.x && world.arena.cursor == used,
                "rejected translate mutation dirtied state/cache or allocated");
    }
    auto boundary = initial[0].transform;
    boundary.position = {-32768.0f, std::nextafter(32768.0f, 0.0f), -0.5f / 65536.0f};
    ok(isle_game_model_set_transform(world.game, model, 0, &boundary));
    ok(isle_game_model_nodes(world.game, model, &nodes, &extent));
    require(nodes == stable && nodes[0].revision == initial_revision + 1 &&
            nodes[1].revision == child_revision && world.arena.cursor == used &&
            fixed_lane(nodes[0].fixed, 3, 0) == std::numeric_limits<std::int32_t>::min() &&
            fixed_lane(nodes[0].fixed, 3, 1) == 2147483520 &&
            fixed_lane(nodes[0].fixed, 3, 2) == 0,
            "translate boundary/truncate/cache/selective invalidation semantics differ from source");
    ok(isle_game_model_set_transform(world.game, model, 0, &initial[0].transform));
    ok(isle_game_model_nodes(world.game, model, &nodes, &extent));
    require(nodes[0].revision == initial_revision + 2 && nodes[1].revision == child_revision &&
            std::memcmp(&nodes[0].fixed, &initial_fixed, sizeof(initial_fixed)) == 0,
            "restored translate input failed exact fixed repeat");
    ok(isle_game_delete_object(world.game, owner));
    require(counts(world).models == 0 && counts(world).model_nodes == 0 && counts(world).matrices == 0,
            "mixed-kind model ownership did not retire");
}

void model_photo_bake() {
    auto root = identity_model_transform();
    root.position = {10, 20, 30};
    root.rotation = {0.25f, 0.5f, -0.75f};
    root.scale = {0.999999f, 2, -3};
    const IsleGameVec3 world{100, 200, 300};
    IsleGameModelTransform baked{}, repeated{};
    ok(isle_game_model_bake_photo_transform(&root, &world, 1.57079637f, 2, &baked));
    close_to(baked.position.x, 106);
    close_to(baked.position.y, 204);
    close_to(baked.position.z, 298);
    close_to(baked.rotation.x, root.rotation.x);
    close_to(baked.rotation.y, root.rotation.y + 1.57079637f);
    close_to(baked.rotation.z, root.rotation.z);
    require(baked.scale.x == root.scale.x * 0.2f && baked.scale.y == 0.4f &&
        baked.scale.z == -0.6f, "photo Pidgey scale did not use 2*0.1 binary32 order");
    ok(isle_game_model_bake_photo_transform(&root, &world, 1.57079637f, 2, &repeated));
    require(std::memcmp(&baked, &repeated, sizeof(baked)) == 0,
        "repeated unbaked photo input accumulated the world transform");
    auto aliased = root;
    ok(isle_game_model_bake_photo_transform(&aliased, &world, 1.57079637f, 2, &aliased));
    require(std::memcmp(&baked, &aliased, sizeof(baked)) == 0, "photo output/root alias changed the result");
    auto invalid = root;
    invalid.scale.x = std::numeric_limits<float>::infinity();
    require(isle_game_model_bake_photo_transform(&invalid, &world, 0, 2, &repeated) ==
        ISLE_GAME_INVALID_ARGUMENT && repeated.scale.x == 0 && repeated.position.x == 0,
        "invalid photo root leaked output");
}

void model_rejection_atomicity_and_reuse() {
    World world(8192);
    auto* owner = object(world, 1);
    auto* other = object(world, 2);
    IsleGameModelNodeInit initial{UINT32_MAX, identity_model_transform(), ISLE_GAME_MODEL_ROTATE_RPY_TRANSLATE_SCALE};
    IsleGameModel* model = nullptr;
    const auto before = world.arena.cursor;
    auto invalid = initial;
    invalid.parent = 0;
    require(isle_game_attach_model(world.game, owner, &invalid, 1, &model) ==
        ISLE_GAME_INVALID_ARGUMENT && !model && world.arena.cursor == before,
        "invalid root attachment changed arena or published a partial model");
    std::array<IsleGameModelNodeInit, 2> bad_tree{initial, initial};
    require(isle_game_attach_model(world.game, owner, bad_tree.data(), bad_tree.size(), &model) ==
        ISLE_GAME_INVALID_ARGUMENT && !model && world.arena.cursor == before,
        "multiple roots accepted");
    bad_tree[1].parent = 2;
    require(isle_game_attach_model(world.game, owner, bad_tree.data(), bad_tree.size(), &model) ==
        ISLE_GAME_INVALID_ARGUMENT && !model, "forward/cyclic parent accepted");
    ok(isle_game_attach_model(world.game, owner, &initial, 1, &model));
    IsleGameCamera* rejected_camera = nullptr;
    require(isle_game_attach_camera(world.game, owner, &rejected_camera) == ISLE_GAME_INVALID_STATE &&
        !rejected_camera, "camera overwrote model payload");
    IsleGameSprite* rejected_sprite = nullptr;
    require(isle_game_add_sprite(world.game, owner, nullptr, &rejected_sprite) == ISLE_GAME_INVALID_ARGUMENT &&
        !rejected_sprite, "sprite interpreted native model as sprite list");
    IsleGameModel* rejected = model;
    require(isle_game_attach_model(world.game, owner, &initial, 1, &rejected) ==
        ISLE_GAME_INVALID_STATE && !rejected, "second model overwrote object");
    const IsleGameModelNode* nodes = nullptr;
    std::size_t extent = 0;
    ok(isle_game_model_nodes(world.game, model, &nodes, &extent));
    const auto original = nodes[0].fixed;
    for (auto value : {std::numeric_limits<float>::quiet_NaN(),
                       std::numeric_limits<float>::infinity(), 32768.0f}) {
        auto t = initial.transform;
        t.position.x = value;
        require(isle_game_model_set_transform(world.game, model, 0, &t) == ISLE_GAME_INVALID_ARGUMENT,
            "nonfinite or unrepresentable position reached fixed cast");
    }
    auto t = initial.transform;
    t.rotation.y = std::numeric_limits<float>::max();
    require(isle_game_model_set_transform(world.game, model, 0, &t) == ISLE_GAME_INVALID_ARGUMENT,
        "angle product outside signed word reached cast");
    t = initial.transform; t.scale.z = 8388608.0f;
    require(isle_game_model_set_transform(world.game, model, 0, &t) == ISLE_GAME_INVALID_ARGUMENT,
        "scale product outside signed word reached cast");
    t = initial.transform; t.position.x = -32768;
    ok(isle_game_model_set_transform(world.game, model, 0, &t));
    ok(isle_game_model_nodes(world.game, model, &nodes, &extent));
    require(fixed_lane(nodes[0].fixed, 3, 0) == INT32_MIN, "legal negative fixed boundary rejected");
    ok(isle_game_model_set_transform(world.game, model, 0, &initial.transform));
    ok(isle_game_model_nodes(world.game, model, &nodes, &extent));
    require(std::memcmp(&nodes[0].fixed, &original, sizeof(original)) == 0,
        "restoring original model input did not restore fixed bytes");
    bool wrong_owner = false;
    std::thread foreign([&] {
        const IsleGameModelNode* output = nodes;
        std::size_t count = 7;
        IsleGameModel* attached = model;
        wrong_owner = isle_game_model_nodes(world.game, model, &output, &count) == ISLE_GAME_WRONG_OWNER &&
            !output && count == 0 &&
            isle_game_attach_model(world.game, other, &initial, 1, &attached) == ISLE_GAME_WRONG_OWNER &&
            !attached && isle_game_remove_model(world.game, model) == ISLE_GAME_WRONG_OWNER &&
            isle_game_model_set_flags(world.game, model, 0, 1) == ISLE_GAME_WRONG_OWNER &&
            isle_game_model_set_transform(world.game, model, 0, &initial.transform) == ISLE_GAME_WRONG_OWNER;
    });
    foreign.join();
    require(wrong_owner, "native model admitted foreign creator-domain access");
    std::vector<IsleGameObject*> fillers;
    for (;;) {
        IsleGameObject* extra = nullptr;
        auto result = isle_game_add_object(world.game, 3, 0, 0, nullptr, nullptr, &extra);
        if (result == ISLE_GAME_EXHAUSTED) break;
        ok(result); fillers.push_back(extra);
    }
    const auto exhausted = world.arena.cursor;
    rejected = model;
    require(isle_game_attach_model(world.game, other, &initial, 1, &rejected) == ISLE_GAME_EXHAUSTED &&
        !rejected && counts(world).models == 1 && counts(world).model_nodes == 1 &&
        world.arena.cursor == exhausted, "exhausted attachment published state or consumed arena");
    ok(isle_game_remove_model(world.game, model));
    nodes = reinterpret_cast<const IsleGameModelNode*>(std::uintptr_t{1}); extent = 9;
    require(isle_game_model_nodes(world.game, model, &nodes, &extent) == ISLE_GAME_INVALID_ARGUMENT &&
        !nodes && extent == 0, "removed model remained borrowable");
    IsleGameModel* reused = nullptr;
    ok(isle_game_attach_model(world.game, other, &initial, 1, &reused));
    require(reused == model && world.arena.cursor == exhausted, "released model block was not reused");
    ok(isle_game_model_nodes(world.game, reused, &nodes, &extent));
    require(nodes[0].revision == 1 && nodes[0].flags == 0, "reused model inherited cache/flags");
    ok(isle_game_delete_object(world.game, other));
    require(counts(world).models == 0 && counts(world).model_nodes == 0 && counts(world).matrices == 0,
        "typed object deletion leaked model ownership");
}

void model_deferred_delete_and_destroy() {
    for (int mode : {0, 1, 2}) {
        World world;
        EngineFixture fixture{}; fixture.game = world.game; fixture.mode = mode;
        auto* owner = object(world, 1, 0, 0, mode == 2 ? engine_delete_self : nullptr,
                             mode == 2 ? &fixture : nullptr);
        IsleGameModelNodeInit initial{UINT32_MAX, identity_model_transform(), ISLE_GAME_MODEL_ROTATE_RPY_TRANSLATE_SCALE};
        IsleGameModel* model = nullptr;
        ok(isle_game_attach_model(world.game, owner, &initial, 1, &model));
        if (mode != 2) process(world, owner, engine_delete_self, &fixture,
                              mode == 0 ? ISLE_GAME_STACKFUL : ISLE_GAME_SYNCHRONOUS, 5);
        ok(isle_game_tick(world.game));
        auto live = counts(world);
        require(live.objects == 0 && live.processes == 0 && live.models == 0 &&
            live.model_nodes == 0 && live.matrices == 0 && fixture.retired_continued == 0,
            "deferred self-deletion failed to retire native type1 model");
    }
    World world;
    auto* owner = object(world, 1);
    IsleGameModelNodeInit initial{UINT32_MAX, identity_model_transform(), ISLE_GAME_MODEL_ROTATE_RPY_TRANSLATE_SCALE};
    IsleGameModel* model = nullptr;
    ok(isle_game_attach_model(world.game, owner, &initial, 1, &model));
    world.reset();
    auto live = counts(world);
    require(live.objects == 0 && live.models == 0 && live.model_nodes == 0 && live.matrices == 0,
        "destroy/cold reset retained native type1 payload");
}
} // namespace

int main() {
    try {
        update_and_process_order();
        pause_preserves_nested_waits();
        callback_lifecycle_and_same_tick_child();
        synchronous_end_and_update_pause();
        deferred_object_deletion();
        actual_intro_fade_and_cold_reset();
        bounded_pool_reuse_and_failure_fence();
        camera_geometry_and_cache();
        camera_reflection_roll_geometry_and_state();
        camera_batch_failure_is_atomic();
        camera_rejection_and_typed_cleanup();
        camera_exhaustion_and_reuse();
        model_hierarchy_math_and_cache();
        model_translate_selection_and_cache();
        model_photo_bake();
        model_rejection_atomicity_and_reuse();
        model_deferred_delete_and_destroy();
    } catch (const std::exception& error) {
        std::cerr << "native C engine checks failed: " << error.what() << '\n';
        return 1;
    }
    std::cout << "native C engine checks passed; actual dispatcher/fade, not full scene boot\n";
    return 0;
}
