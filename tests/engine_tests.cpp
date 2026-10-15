#include "engine_fixture.h"

#include <array>
#include <cstdint>
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
        camera_rejection_and_typed_cleanup();
        camera_exhaustion_and_reuse();
    } catch (const std::exception& error) {
        std::cerr << "native C engine checks failed: " << error.what() << '\n';
        return 1;
    }
    std::cout << "native C engine checks passed; actual dispatcher/fade, not full scene boot\n";
    return 0;
}
