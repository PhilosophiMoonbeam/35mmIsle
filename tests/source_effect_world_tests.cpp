#include "source_effect_world.hpp"
#include "source_effect_camera.hpp"
#include <array>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <new>
#include <stdexcept>
#include <vector>

namespace probe { bool enabled = false; std::size_t count = 0; }
void* operator new(std::size_t n) {
    if (probe::enabled) ++probe::count;
    if (void* p = std::malloc(n ? n : 1)) return p;
    throw std::bad_alloc();
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
namespace {
using namespace isle::menu;
constexpr float quarter = 1.57079632679489661923f;
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
void native(IsleGameResult r) { require(r == ISLE_GAME_OK,"native operation failed"); }
struct Fixture {
    std::vector<std::byte> storage = std::vector<std::byte>(1024 * 1024);
    isle_arena arena{};
    IsleGame* game = nullptr;
    IsleGameObject* object = nullptr;
    IsleGameModel* model = nullptr;
    std::array<IsleGameModelNodeInit,3> initial{};
    Fixture() {
        require(isle_arena_init(&arena,storage.data(),storage.size()) == ISLE_OK,"arena init");
        native(isle_game_create(&arena,&game));
        native(isle_game_add_object(game,1,0,0,nullptr,nullptr,&object));
        // All contextual: accepted floats, not quantized render/fixed matrices.
        initial[0] = {UINT32_MAX,{{7,-2,3},{quarter,0,0},{2,-3,4}},ISLE_GAME_MODEL_CONTEXTUAL_RPY};
        initial[1] = {0,{{-5,6,1},{0,quarter,0},{-1,2,0.5f}},ISLE_GAME_MODEL_CONTEXTUAL_RPY};
        initial[2] = {1,{{2,3,-4},{0,0,quarter},{3,0.25f,-2}},ISLE_GAME_MODEL_CONTEXTUAL_RPY};
        native(isle_game_attach_model(game,object,initial.data(),initial.size(),&model));
    }
    ~Fixture() { if (game && isle_game_destroy(game) != ISLE_GAME_OK) std::terminate(); }
};
struct Point { double x,y,z; };
// Independent point operations, quarter-turns are analytic permutations, not
// matrix composition, libm, or any production transform helper.
Point local(Point p, const IsleGameModelTransform& t) {
    p = {p.x*t.scale.x,p.y*t.scale.y,p.z*t.scale.z};
    if (t.rotation.x != 0) p = {p.x,-p.z,p.y};
    if (t.rotation.y != 0) p = {p.z,p.y,-p.x};
    if (t.rotation.z != 0) p = {-p.y,p.x,p.z};
    return {p.x+t.position.x,p.y+t.position.y,p.z+t.position.z};
}
Point transformed(const EffectWorld& world, Point p) {
    return {p.x*world.matrix[0][0]+p.y*world.matrix[1][0]+p.z*world.matrix[2][0]+world.matrix[3][0],
            p.x*world.matrix[0][1]+p.y*world.matrix[1][1]+p.z*world.matrix[2][1]+world.matrix[3][1],
            p.x*world.matrix[0][2]+p.y*world.matrix[1][2]+p.z*world.matrix[2][2]+world.matrix[3][2]};
}
void close(Point a, Point b) {
    require(std::abs(a.x-b.x)<0.002 && std::abs(a.y-b.y)<0.002 && std::abs(a.z-b.z)<0.002,
            "source chain disagrees with independent analytic point oracle");
}
void oracle(Fixture& f, unsigned leaf, IsleGameModelPlacementMode mode,
            IsleGameModelTransform auxiliary) {
    native(isle_game_model_set_placement(f.game,f.model,mode,&auxiliary));
    EffectWorld world;
    require(bool(model::source_effect_world(f.game,f.model,leaf,world)),"world rejected");
    if (mode == ISLE_GAME_MODEL_PLACEMENT_RT) auxiliary.scale = {1,1,1};
    const std::array<Point,5> points{{{0,0,0},{1,0,0},{0,1,0},{0,0,1},{2,-3,5}}};
    for (Point input : points) {
        Point expected = input;
        for (unsigned n = leaf;; --n) {
            expected = local(expected,f.initial[n].transform);
            if (!n) break;
        }
        if (mode != ISLE_GAME_MODEL_PLACEMENT_DISABLED) expected = local(expected,auxiliary);
        close(transformed(world,input),expected);
    }
}
void behavior() {
    Fixture f;
    IsleGameModelTransform auxiliary{{11,13,-17},{0,0,quarter},{-2,3,0.5f}};
    for (auto mode : {ISLE_GAME_MODEL_PLACEMENT_DISABLED,ISLE_GAME_MODEL_PLACEMENT_RT,ISLE_GAME_MODEL_PLACEMENT_RTS})
        for (unsigned leaf = 0; leaf < 3; ++leaf) oracle(f,leaf,mode,auxiliary);
    for (unsigned n = 0; n < 3; ++n) native(isle_game_model_set_flags(f.game,f.model,n,3));
    oracle(f,2,ISLE_GAME_MODEL_PLACEMENT_RTS,auxiliary);
    f.initial[1].transform.position = {19,-7,5};
    native(isle_game_model_set_transform(f.game,f.model,1,&f.initial[1].transform));
    oracle(f,2,ISLE_GAME_MODEL_PLACEMENT_RTS,auxiliary);
    // A zero source column is intentionally not normalized/rescued here.
    for (auto& n : f.initial) n.transform = {{},{},{1,1,1}};
    f.initial[0].transform.scale = {0,-3,2};
    for (unsigned n = 0; n < 3; ++n) native(isle_game_model_set_transform(f.game,f.model,n,&f.initial[n].transform));
    oracle(f,2,ISLE_GAME_MODEL_PLACEMENT_DISABLED,auxiliary);
    EffectWorld world;
    require(bool(model::source_effect_world(f.game,f.model,2,world)),"zero column rejected");
    require(world.matrix[0][0] == 0 && world.matrix[1][0] == 0 && world.matrix[2][0] == 0 &&
            world.matrix[1][1] == -3,"raw columns were normalized or repaired");
    const IsleGameModelNode* nodes = nullptr; std::size_t count = 0;
    native(isle_game_model_peek_nodes(f.game,f.model,&nodes,&count));
    std::array<IsleGameModelNode,3> before{nodes[0],nodes[1],nodes[2]};
    const auto cursor = f.arena.cursor;
    probe::count = 0; probe::enabled = true;
    bool success = true;
    for (unsigned n = 0; n < 256; ++n) success &= bool(model::source_effect_world(f.game,f.model,n%3,world));
    probe::enabled = false;
    require(success && probe::count == 0 && cursor == f.arena.cursor,"pure borrow allocated");
    require(std::memcmp(before.data(),nodes,sizeof(before)) == 0,"pure borrow evaluated native caches");
    auto unchanged = world;
    require(model::source_effect_world(f.game,f.model,3,world).error == EffectsError::extent &&
            world.matrix == unchanged.matrix,"index failure published output");
    require(model::source_effect_world(nullptr,f.model,0,world).error == EffectsError::attachment &&
            world.matrix == unchanged.matrix,"owner failure published output");
    require(model::source_effect_world(f.game,nullptr,0,world).error == EffectsError::attachment &&
            world.matrix == unchanged.matrix,"null model failure published output");
    auto huge = f.initial[0].transform;
    huge.scale = {std::numeric_limits<float>::max(),1,1};
    native(isle_game_model_set_transform(f.game,f.model,0,&huge));
    huge.scale = {2,1,1};
    native(isle_game_model_set_transform(f.game,f.model,1,&huge));
    require(model::source_effect_world(f.game,f.model,1,world).error == EffectsError::arithmetic &&
            world.matrix == unchanged.matrix,"nonfinite composition published output");
    huge.position.x = std::numeric_limits<float>::quiet_NaN();
    require(isle_game_model_set_transform(f.game,f.model,1,&huge) != ISLE_GAME_OK,
            "native accepted nonfinite pose");
    // Mixed cache kinds retain the same full source chain. In particular the
    // TRANSLATE root's noncommuting rotations and signed nonuniform scale must
    // affect descendant basis/arbitrary points despite its translation cache.
    native(isle_game_remove_model(f.game,f.model));
    f.initial[0] = {UINT32_MAX,{{3,4,5},{quarter,quarter,quarter},{-2,3,0.5f}},ISLE_GAME_MODEL_TRANSLATE};
    f.initial[1] = {0,{{2,-1,7},{0,quarter,0},{-2,3,1}},ISLE_GAME_MODEL_ROTATE_RPY_TRANSLATE_SCALE};
    f.initial[2] = {1,{{-4,3,2},{quarter,0,0},{1,-2,4}},ISLE_GAME_MODEL_CONTEXTUAL_RPY};
    native(isle_game_attach_model(f.game,f.object,f.initial.data(),f.initial.size(),&f.model));
    for (auto mode : {ISLE_GAME_MODEL_PLACEMENT_DISABLED,ISLE_GAME_MODEL_PLACEMENT_RT,ISLE_GAME_MODEL_PLACEMENT_RTS})
        for (unsigned leaf = 0; leaf < 3; ++leaf) oracle(f,leaf,mode,auxiliary);
}
Point screen(const EffectCamera& camera, Point point) {
    std::array<double,4> clip{};
    for (unsigned column=0;column<4;++column)
        clip[column]=point.x*camera.projection[0][column]+point.y*camera.projection[1][column]+
            point.z*camera.projection[2][column]+camera.projection[3][column];
    return {(clip[0]/clip[3]*camera.viewport_scale[0]+camera.viewport_translate[0])/4,
            (-clip[1]/clip[3]*camera.viewport_scale[1]+camera.viewport_translate[1])/4,0};
}
void close_pixel(Point actual, Point expected) {
    // GAME fast-trig projection is quantized. Compare its visible probe against
    // the continuous geometric oracle within half an RDP quarter-pixel.
    require(std::abs(actual.x-expected.x)<0.125&&std::abs(actual.y-expected.y)<0.125,
            "source FX projection exceeds the subpixel geometric oracle bound");
}
void camera_projection() {
    Fixture f;
    IsleGameObject* object=nullptr;
    IsleGameCamera* camera=nullptr;
    native(isle_game_add_object(f.game,2,0,0,nullptr,nullptr,&object));
    native(isle_game_attach_camera(f.game,object,&camera));
    native(isle_game_camera_add_matrix(f.game,camera,ISLE_GAME_CAMERA_PERSPECTIVE_FAST));
    native(isle_game_camera_add_matrix(f.game,camera,ISLE_GAME_CAMERA_LOOK_AT_REFLECT_ROLL));
    IsleGamePerspective perspective{90,1,1,100,1};
    // Renderer up intentionally differs: GAME FX independently uses fixed Y up.
    IsleGameLookAt look{{0,0,10},{0,0,0},{1,0,0}};
    IsleGameViewport viewport{{640,480,511,0},{640,480,511,0}};
    native(isle_game_camera_set_perspective(f.game,camera,&perspective));
    native(isle_game_camera_set_look_at(f.game,camera,&look));
    native(isle_game_camera_set_viewport(f.game,camera,&viewport));
    native(isle_game_camera_set_roll(f.game,camera,0));
    EffectCamera projected;
    require(source_effect_camera(f.game,camera,projected),"gameplay FX camera rejected");
    // 90-degree, unit-aspect frustum: (1,2) at distance10 is (0.1,0.2) NDC.
    close_pixel(screen(projected,{1,2,0}),{176,96,0});
    native(isle_game_camera_set_roll(f.game,camera,quarter));
    require(source_effect_camera(f.game,camera,projected),"rolled FX camera rejected");
    // Positive camera roll makes right=+Y and up=-X.
    close_pixel(screen(projected,{1,2,0}),{192,132,0});
    native(isle_game_camera_set_roll(f.game,camera,0));
    look.eye[0]=look.at[0]=1;
    native(isle_game_camera_set_look_at(f.game,camera,&look));
    require(source_effect_camera(f.game,camera,projected),"changed FX view rejected");
    close_pixel(screen(projected,{1,2,0}),{160,96,0});
    perspective.scale=2;
    native(isle_game_camera_set_perspective(f.game,camera,&perspective));
    require(source_effect_camera(f.game,camera,projected),"scaled FX camera rejected");
    close_pixel(screen(projected,{2,2,0}),{176,96,0});
    viewport.scale[0]=viewport.translate[0]=320;
    viewport.scale[1]=viewport.translate[1]=240;
    native(isle_game_camera_set_viewport(f.game,camera,&viewport));
    require(source_effect_camera(f.game,camera,projected),"changed FX viewport rejected");
    close_pixel(screen(projected,{2,2,0}),{88,48,0});
    const auto accepted=projected;
    require(!source_effect_camera(f.game,nullptr,projected)&&projected.projection==accepted.projection&&
            projected.viewport_scale==accepted.viewport_scale&&projected.viewport_translate==accepted.viewport_translate,
            "invalid native camera published partial FX projection");
}
}
int main() {
    try { behavior(); camera_projection(); std::cout << "source effect world/camera contracts passed\n"; return 0; }
    catch (const std::exception& e) { probe::enabled = false; std::cerr << e.what() << '\n'; return 1; }
}
