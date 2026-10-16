#include "actor_spatial.h"
#include "actor_owner_source.h"
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <thread>
#include <vector>

namespace {
constexpr float maximum = std::numeric_limits<float>::max();
constexpr IsleGameVec3 sentinel{123,456,789};
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
void native(IsleGameResult result) { require(result == ISLE_GAME_OK, "native operation rejected"); }
bool same(float a, float b) {
    return std::bit_cast<std::uint32_t>(a) == std::bit_cast<std::uint32_t>(b);
}
bool same(IsleGameVec3 a, IsleGameVec3 b) {
    return same(a.x,b.x) && same(a.y,b.y) && same(a.z,b.z);
}
void close(float a, float b) { require(std::abs(a-b) < 0.00002f, "spatial analytic value differs"); }
void close(IsleGameVec3 a, IsleGameVec3 b) { close(a.x,b.x); close(a.y,b.y); close(a.z,b.z); }
struct Entity {
    IsleGameObject* object = nullptr;
    IsleGameModel* model = nullptr;
    IsleActorSource source{};
    isle_rng rng{1};
    IsleGameModelTransform root{{},{},{1,1,1}};
    IsleGameModelTransform auxiliary{{},{},{1,1,1}};
};
struct Fixture {
    std::vector<std::byte> bytes = std::vector<std::byte>(1024 * 1024);
    isle_arena arena{};
    IsleGame* game = nullptr;
    std::array<Entity,96> entities{};
    std::size_t count = 0;
    Fixture() {
        require(isle_arena_init(&arena, bytes.data(), bytes.size()) == ISLE_OK, "arena init");
        native(isle_game_create(&arena, &game));
    }
    ~Fixture() { if (game && isle_game_destroy(game) != ISLE_GAME_OK) std::abort(); }
    Fixture(const Fixture&) = delete;
    Fixture& operator=(const Fixture&) = delete;
    void bind(Entity& entity) {
        // Distinct real canonical owner identities, not an actor list supplied
        // to the query. Traversal must see every owner's native link3 nodes.
        native(isle_actor_source_bind(game, entity.object, &entity.source, &entity.rng, &entity));
        entity.source.id = 129;
        entity.source.tangible = 1;
        entity.source.dirToNeighbors = sentinel;
    }
    Entity& add(IsleGameVec3 position = {}, std::uint32_t priority = 0,
                std::uint8_t link = 3, bool bound = true, bool model = true,
                bool auxiliary = true) {
        require(count < entities.size(), "native fixture capacity");
        auto& entity = entities[count++];
        native(isle_game_add_object(game, static_cast<std::uint32_t>(count), link, priority,
                                    nullptr, nullptr, &entity.object));
        if (bound) bind(entity);
        if (model) {
            entity.root.position = auxiliary ? IsleGameVec3{30000,-20000,10000} : position;
            entity.root.rotation = {0.2f,-0.4f,0.6f};
            entity.root.scale = {-2,3,0};
            IsleGameModelNodeInit root{UINT32_MAX,entity.root,ISLE_GAME_MODEL_CONTEXTUAL_RPY};
            native(isle_game_attach_model(game, entity.object, &root, 1, &entity.model));
            if (auxiliary) set_auxiliary(entity, position);
        }
        return entity;
    }
    void set_auxiliary(Entity& entity, IsleGameVec3 position,
                       IsleGameModelPlacementMode mode = ISLE_GAME_MODEL_PLACEMENT_RTS) {
        entity.auxiliary.position = position;
        entity.auxiliary.rotation = {-0.3f,0.5f,-0.7f};
        entity.auxiliary.scale = {4,-5,0};
        native(isle_game_model_set_placement(game, entity.model, mode, &entity.auxiliary));
    }
    void set_root(Entity& entity, IsleGameVec3 position) {
        entity.root.position = position;
        native(isle_game_model_set_transform(game, entity.model, 0, &entity.root));
    }
};
float distance(Fixture& f, Entity& a, Entity& b) {
    float output = -1;
    native(isle_actor_spatial_distance(f.game, a.object, b.object, &output));
    return output;
}
struct NeighborResult {
    IsleGameVec3 direction = sentinel;
    float magnitude = -1;
};
NeighborResult neighbors(Entity& entity) {
    NeighborResult output;
    native(isle_actor_spatial_neighbors(&entity.source, &output.direction, &output.magnitude));
    return output;
}
// Independent source-expression oracle used only with explicit fixture order,
// never fed by a native cursor or the implementation's model/position helpers.
float source_normalize(IsleGameVec3& v) {
    const float length = std::sqrt((v.x*v.x + v.y*v.y) + v.z*v.z);
    if (length == 0) return 0;
    v.x *= 1.0f / length;
    v.y *= 1.0f / length;
    v.z *= 1.0f / length;
    return length;
}
NeighborResult source_neighbors(IsleGameVec3 self, std::initializer_list<IsleGameVec3> order) {
    NeighborResult result{{0,0,0},0};
    bool found = false;
    for (auto other : order) {
        IsleGameVec3 away{self.x-other.x,self.y-other.y,self.z-other.z};
        const float length = source_normalize(away);
        if (length < 100) {
            found = true;
            result.direction.x += length * away.x;
            result.direction.y += length * away.y;
            result.direction.z += length * away.z;
        }
    }
    if (found) result.magnitude = source_normalize(result.direction);
    return result;
}
void exact(NeighborResult actual, NeighborResult expected) {
    require(same(actual.direction,expected.direction) && same(actual.magnitude,expected.magnitude),
            "native neighbor result differs from ordered GAME binary32 expressions");
}
void expect_order(Fixture& f, std::initializer_list<const Entity*> expected) {
    IsleGameObject* previous = nullptr;
    for (auto* entity : expected) {
        IsleGameObject* object = nullptr;
        IsleActorSource* source = nullptr;
        IsleGameModel* model = nullptr;
        native(isle_actor_source_link3_next(f.game, previous, &object, &source, &model));
        require(object == entity->object && source == &entity->source && model == entity->model,
                "fixture did not exercise specified actual native object/model/record order");
        previous = object;
    }
    IsleGameObject* object = previous;
    IsleActorSource* source = nullptr;
    IsleGameModel* model = nullptr;
    native(isle_actor_source_link3_next(f.game, previous, &object, &source, &model));
    require(!object && !source && !model, "unexpected native link3 tail");
}
void distance_auxiliary_and_root() {
    Fixture f;
    // Unbound generic Item-style models are deliberately not link3 actors.
    auto& item = f.add({1,2,3},0,1,false,true,false);
    auto& actor = f.add({4,6,15},100);
    require(same(distance(f,item,actor),13) && same(distance(f,actor,item),13),
            "distance substituted source/collision/render origin for generic root or actor AUX");
    actor.source.collPosition = {-999,-999,-999};
    actor.source.collisionOffset = {888,888,888};
    actor.source.flags = 0xFFFF;
    actor.source.tangible = 0;
    native(isle_game_object_set_flags(f.game,actor.object,1));
    f.set_root(actor, {-12345,23456,-30000});
    require(same(distance(f,item,actor),13), "distance consumed root/flags with AUX present");
    f.set_auxiliary(item,{4,6,15},ISLE_GAME_MODEL_PLACEMENT_RT);
    require(same(distance(f,item,actor),0), "generic object's RT AUX did not override its root");
    native(isle_game_model_set_placement(f.game,item.model,ISLE_GAME_MODEL_PLACEMENT_DISABLED,nullptr));
    require(same(distance(f,item,actor),13), "disabled AUX did not restore ordinary root position");
    native(isle_game_model_set_placement(f.game,actor.model,ISLE_GAME_MODEL_PLACEMENT_DISABLED,nullptr));
    f.set_root(actor,{4,6,15});
    require(same(distance(f,item,actor),13), "actor without AUX did not use its actual root");
    f.set_root(item,{1,-10,3});
    require(same(distance(f,item,actor),std::sqrt(409.0f)), "distance omitted vertical component");
    require(same(distance(f,actor,actor),0), "self distance was not zero");
}
void distance_null_model_and_error_order() {
    Fixture f;
    auto& first = f.add({});
    auto& missing = f.add({},0,1,false,false);
    float output = -17;
    for (auto pair : {std::array<IsleGameObject*,2>{nullptr,first.object},
                      std::array<IsleGameObject*,2>{first.object,nullptr},
                      std::array<IsleGameObject*,2>{nullptr,nullptr}}) {
        native(isle_actor_spatial_distance(f.game,pair[0],pair[1],&output));
        require(same(output,maximum), "NULL distance did not return exact FLOAT_MAX");
    }
    require(same(distance(f,first,missing),maximum), "live NULL second model was not FLOAT_MAX");
    output = -17;
    require(isle_actor_spatial_distance(f.game,missing.object,first.object,&output) == ISLE_GAME_INVALID_STATE &&
            same(output,-17), "missing first model became default distance or changed error output");
    require(isle_actor_spatial_distance(f.game,missing.object,missing.object,&output) == ISLE_GAME_INVALID_STATE,
            "missing second model incorrectly preceded invalid first model dereference");
    native(isle_actor_spatial_distance(f.game,missing.object,nullptr,&output));
    require(same(output,maximum), "NULL argument did not precede missing first model");
    auto& camera_entity = f.add({},0,0,false,false);
    IsleGameCamera* camera = nullptr;
    native(isle_game_attach_camera(f.game,camera_entity.object,&camera));
    output = -17;
    require(isle_actor_spatial_distance(f.game,first.object,camera_entity.object,&output) == ISLE_GAME_INVALID_STATE &&
            same(output,-17), "non-model native payload was reinterpreted as a model/default distance");
    require(isle_actor_spatial_distance(nullptr,first.object,first.object,&output) == ISLE_GAME_INVALID_ARGUMENT &&
            same(output,-17), "invalid game published distance");
    require(isle_actor_spatial_distance(f.game,first.object,first.object,nullptr) == ISLE_GAME_INVALID_ARGUMENT,
            "NULL distance output accepted");
    IsleGameModel* model = first.model;
    native(isle_actor_spatial_object_model(f.game,missing.object,&model));
    require(!model, "native checked getter fabricated missing model");
    native(isle_game_remove_model(f.game,first.model));
    require(isle_actor_spatial_distance(f.game,first.object,missing.object,&output) == ISLE_GAME_INVALID_STATE,
            "removed first model remained queryable");
}
void source_float_distance_extremes() {
    Fixture f;
    auto& a = f.add({},0,0,false,true,false);
    auto& b = f.add({},0,0,false,true,false);
    f.set_root(a,{std::numeric_limits<float>::denorm_min(),-0.0f,0});
    require(same(distance(f,a,b),0), "source squared-subnormal underflow was replaced by hypot/epsilon");
    const float tiny = std::ldexp(1.0f,-74);
    f.set_root(a,{tiny,tiny,tiny});
    const float expected = std::sqrt((tiny*tiny + tiny*tiny) + tiny*tiny);
    require(same(distance(f,a,b),expected) && expected > 0,
            "nonzero subnormal sum-of-squares changed source distance");
    f.set_root(a,{maximum,0,0});
    require(std::isinf(distance(f,a,b)), "finite squared overflow was clamped/rejected instead of source infinity");
    f.set_root(b,{-maximum,0,0});
    require(std::isinf(distance(f,a,b)), "finite subtraction overflow gained an invented rejection/default");
}
void neighbors_three_dimensions_weights_and_exclusions() {
    Fixture f;
    auto& self = f.add({10,20,30},100);
    auto& x = f.add({7,20,30},90);
    auto& y = f.add({10,16,30},80);
    auto& z = f.add({10,20,18},70);
    auto& boundary = f.add({10,-80,30},60);
    auto& outside = f.add({10,20,131},50);
    // A root-only unbound other-link object must never enter the cohort.
    f.add({-10,20,30},UINT32_MAX,4,false,true,false);
    expect_order(f,{&self,&x,&y,&z,&boundary,&outside});
    require(self.source.owner != x.source.owner && x.source.owner != y.source.owner,
            "fixture lacks cross-owner native records");
    for (auto* entity : {&x,&y,&z}) {
        entity->source.tangible = 0;
        entity->source.flags = 0xFFFF;
        entity->source.id = -1;
        entity->source.collisionRadius = 50000;
        entity->source.collPosition = {9000,9000,9000};
        native(isle_game_object_set_flags(f.game,entity->object,1));
        native(isle_game_pause_updates(f.game,entity->object,1));
    }
    auto actual = neighbors(self);
    close(actual.direction,{3.0f/13,4.0f/13,12.0f/13});
    close(actual.magnitude,13);
    exact(actual,source_neighbors({10,20,30},{{7,20,30},{10,16,30},{10,20,18},{10,-80,30},{10,20,131}}));
    // This distinguishes distance weighting from a mean of normalized vectors,
    // full3D from XZ projection, strict100 from <=100, and away from attraction.
    require(same(self.source.dirToNeighbors,sentinel), "pure output leaf unexpectedly wrote canonical direction");
    float magnitude = -1;
    native(isle_actor_spatial_neighbors(&self.source,&self.source.dirToNeighbors,&magnitude));
    require(same(self.source.dirToNeighbors,actual.direction) && same(magnitude,actual.magnitude),
            "actual canonical direction storage was not a supported consumer output");
}
void never_dispatched(IsleGameObject*,void*) { std::abort(); }
void neighbors_boundary_coincident_and_paused() {
    Fixture f;
    auto& self = f.add({});
    exact(neighbors(self),{{0,0,0},0});
    auto& coincident = f.add({},40);
    auto& at100 = f.add({0,100,0},30);
    auto& outside = f.add({0,0,std::nextafter(100.0f,maximum)},20);
    exact(neighbors(self),{{0,0,0},0});
    auto& inside = f.add({0,0,std::nextafter(100.0f,0.0f)},10);
    inside.source.tangible = 0;
    native(isle_game_object_set_flags(f.game,inside.object,1));
    IsleGameProcess* process = nullptr;
    native(isle_game_add_process(f.game,inside.object,never_dispatched,nullptr,
                                 ISLE_GAME_SYNCHRONOUS,7,&process));
    native(isle_game_pause_process(f.game,process,1));
    expect_order(f,{&coincident,&at100,&outside,&inside,&self});
    auto expected = source_neighbors({},{{},{0,100,0},{0,0,outside.auxiliary.position.z},
                                         {0,0,inside.auxiliary.position.z}});
    exact(neighbors(self),expected);
    require(expected.direction.z < 0 && expected.magnitude > 99,
            "inside strict boundary or paused native fixture failed to contribute");
}
void ordered_ties_and_cancellation() {
    constexpr float small = 0x1p-20f;
    {
        Fixture f;
        auto& self = f.add({},100);
        auto& positive = f.add({-64,0,0},50);
        auto& tiny = f.add({-small,0,0},50);
        auto& negative = f.add({64,0,0},50);
        expect_order(f,{&self,&positive,&tiny,&negative});
        exact(neighbors(self),{{0,0,0},0});
        exact(neighbors(self),source_neighbors({},{{-64,0,0},{-small,0,0},{64,0,0}}));
    }
    {
        Fixture f;
        auto& self = f.add({},100);
        auto& positive = f.add({-64,0,0},50);
        auto& tiny = f.add({-small,0,0},49);
        auto& negative = f.add({64,0,0},50);
        expect_order(f,{&self,&positive,&negative,&tiny});
        exact(neighbors(self),{{1,0,0},small});
    }
    {
        Fixture f;
        auto& self = f.add({});
        f.add({-3,-4,0},10);
        f.add({3,4,0},9);
        exact(neighbors(self),{{0,0,0},0});
    }
}
void neighbors_subnormal_consumption() {
    Fixture f;
    auto& self = f.add({},100);
    const float least = std::numeric_limits<float>::denorm_min();
    auto& other = f.add({least,-least,-0.0f},90);
    // Direction's squared magnitude underflows to zero without clearing the
    // actual away-vector; weighting by its zero distance then contributes zero.
    exact(neighbors(self),source_neighbors({},{{least,-least,-0.0f}}));
    exact(neighbors(self),{{0,0,0},0});
    const float tiny = std::ldexp(1.0f,-74);
    f.set_auxiliary(other,{-tiny,-tiny,-tiny});
    const auto expected = source_neighbors({},{{-tiny,-tiny,-tiny}});
    require(expected.magnitude > 0 && expected.direction.x > 0,
            "source subnormal-square fixture lost its nonzero consumed sum");
    exact(neighbors(self),expected);
}
void reciprocal_rounding_subnormal_and_complete_cohort() {
    Fixture f;
    auto& self = f.add({},UINT32_MAX);
    const float tiny = std::ldexp(1.0f,-74);
    auto& a = f.add({-tiny,-tiny,-tiny},100);
    auto& b = f.add({-1,-2,-3},90);
    auto& c = f.add({4,7,-11},80);
    auto& d = f.add({-13,17,19},70);
    exact(neighbors(self),source_neighbors({},{{-tiny,-tiny,-tiny},{-1,-2,-3},{4,7,-11},{-13,17,19}}));
    expect_order(f,{&self,&a,&b,&c,&d});
    // More than either photo's twelve-member capacity or a small local cohort.
    // Every additional native owner contributes; no renderer/collision list is
    // registered at all. Pure axis contributions have an exact analytic sum.
    for (auto* entity : {&a,&b,&c,&d}) f.set_auxiliary(*entity,{});
    for (unsigned i = 0; i < 32; ++i) f.add({-1,0,0},static_cast<std::uint32_t>(60-i));
    exact(neighbors(self),{{1,0,0},32});
}
void nonmutating_actual_pose_and_records() {
    Fixture f;
    auto& self = f.add({1,2,3},100);
    auto& other = f.add({4,6,15},90);
    const IsleGameModelNode* nodes = nullptr;
    std::size_t count = 0;
    native(isle_game_model_peek_nodes(f.game,other.model,&nodes,&count));
    require(count == 1, "fixture topology");
    const auto original_node = nodes[0];
    const IsleGameModelPlacement* placement = nullptr;
    native(isle_game_model_placement(f.game,other.model,&placement));
    const auto original_placement = *placement;
    const auto original_self = self.source;
    const auto original_other = other.source;
    const auto used = f.arena.cursor;
    const auto seed_self = self.rng.seed;
    const auto seed_other = other.rng.seed;
    require(same(distance(f,self,other),13), "actual native distance before purity check");
    exact(neighbors(self),source_neighbors({1,2,3},{{4,6,15}}));
    require(f.arena.cursor == used && std::memcmp(&original_node,nodes,sizeof(original_node)) == 0 &&
            std::memcmp(&original_placement,placement,sizeof(original_placement)) == 0 &&
            std::memcmp(&original_self,&self.source,sizeof(original_self)) == 0 &&
            std::memcmp(&original_other,&other.source,sizeof(original_other)) == 0 &&
            self.rng.seed == seed_self && other.rng.seed == seed_other,
            "spatial query allocated or changed actual models/caches/canonical records/RNG");
}
void neighbor_admission_and_transactional_errors() {
    Fixture f;
    auto& self = f.add({},100);
    auto& good = f.add({-3,0,0},90);
    auto& unbound = f.add({-4,0,0},80,3,false);
    auto output = sentinel;
    float magnitude = -17;
    require(isle_actor_spatial_neighbors(&self.source,&output,&magnitude) == ISLE_GAME_INVALID_STATE &&
            same(output,sentinel) && same(magnitude,-17),
            "unbound native link3 node was skipped or published a partial sum");
    f.bind(unbound);
    native(isle_actor_spatial_neighbors(&self.source,&output,&magnitude));
    close(magnitude,7);
    native(isle_game_model_set_placement(f.game,unbound.model,ISLE_GAME_MODEL_PLACEMENT_DISABLED,nullptr));
    unbound.source.tangible = 0;
    native(isle_game_object_set_flags(f.game,unbound.object,1));
    output = sentinel;
    magnitude = -17;
    require(isle_actor_spatial_neighbors(&self.source,&output,&magnitude) == ISLE_GAME_INVALID_STATE &&
            same(output,sentinel) && same(magnitude,-17),
            "missing neighbor AUX used root/hidden/intangible fallback or partial output");
    f.set_auxiliary(unbound,{-4,0,0});
    native(isle_game_remove_model(f.game,unbound.model));
    require(isle_actor_spatial_neighbors(&self.source,&output,&magnitude) == ISLE_GAME_INVALID_STATE,
            "missing attached native link3 model was skipped");
    native(isle_game_delete_object(f.game,unbound.object));
    auto copied = self.source;
    require(isle_actor_spatial_neighbors(&copied,&output,&magnitude) == ISLE_GAME_INVALID_STATE,
            "copied canonical source authorized a spatial query");
    auto* actual_object = good.source.object;
    good.source.object = self.object;
    require(isle_actor_spatial_neighbors(&self.source,&output,&magnitude) == ISLE_GAME_INVALID_STATE,
            "foreign canonical record binding was accepted by traversal");
    good.source.object = actual_object;
    native(isle_game_model_set_placement(f.game,self.model,ISLE_GAME_MODEL_PLACEMENT_DISABLED,nullptr));
    require(isle_actor_spatial_neighbors(&self.source,&output,&magnitude) == ISLE_GAME_INVALID_STATE,
            "missing self AUX fell back to root");
    f.set_auxiliary(self,{});
    require(isle_actor_spatial_neighbors(nullptr,&output,&magnitude) == ISLE_GAME_INVALID_ARGUMENT &&
            isle_actor_spatial_neighbors(&self.source,nullptr,&magnitude) == ISLE_GAME_INVALID_ARGUMENT &&
            isle_actor_spatial_neighbors(&self.source,&output,nullptr) == ISLE_GAME_INVALID_ARGUMENT &&
            same(output,sentinel) && same(magnitude,-17), "invalid outputs accepted or mutated");
}
struct RetirementObservation {
    IsleGame* game = nullptr;
    Entity* self = nullptr;
    Entity* retiring = nullptr;
    IsleGameResult first = ISLE_GAME_OK, second = ISLE_GAME_OK, neighbors = ISLE_GAME_OK;
    float distance = -17, magnitude = -17;
    IsleGameVec3 direction = sentinel;
};
void observe_retiring(IsleGameObject*,void* user) {
    auto& observation = *static_cast<RetirementObservation*>(user);
    observation.first = isle_actor_spatial_distance(observation.game,observation.retiring->object,
                                                     observation.self->object,&observation.distance);
    observation.second = isle_actor_spatial_distance(observation.game,observation.self->object,
                                                      observation.retiring->object,&observation.distance);
    observation.neighbors = isle_actor_spatial_neighbors(&observation.self->source,
                                                         &observation.direction,&observation.magnitude);
}
void real_retirement_and_owner_access() {
    Fixture f;
    auto& self = f.add({},100);
    auto& other = f.add({-3,0,0},90);
    RetirementObservation observation;
    observation.game = f.game;
    observation.self = &self;
    observation.retiring = &other;
    native(isle_game_object_retain_identity(f.game,other.object));
    native(isle_game_object_set_pre_retire_callback(f.game,other.object,observe_retiring,&observation));
    native(isle_game_delete_object(f.game,other.object));
    require(observation.first == ISLE_GAME_INVALID_STATE && observation.second == ISLE_GAME_INVALID_STATE &&
            observation.neighbors == ISLE_GAME_INVALID_STATE && same(observation.distance,-17) &&
            same(observation.magnitude,-17) && same(observation.direction,sentinel),
            "actual retiring model/list entry was consumed before unlink/removal");
    float output = -17;
    require(isle_actor_spatial_distance(f.game,self.object,other.object,&output) == ISLE_GAME_INVALID_ARGUMENT &&
            same(output,-17), "held joined-dead second object became live NULL-model FLOAT_MAX");
    require(isle_actor_spatial_distance(f.game,other.object,self.object,&output) == ISLE_GAME_INVALID_ARGUMENT,
            "held joined-dead first object remained queryable");
    native(isle_actor_spatial_distance(f.game,other.object,nullptr,&output));
    require(same(output,maximum), "NULL source argument incorrectly consumed dead other handle");
    exact(neighbors(self),{{0,0,0},0});
    native(isle_game_object_release_identity(f.game,other.object));
    // old raw handle expires here; never query it after native address reuse.
    auto& replacement = f.add({-4,0,0},90,3,false);
    auto direction = sentinel;
    float magnitude = -17;
    require(isle_actor_spatial_neighbors(&self.source,&direction,&magnitude) == ISLE_GAME_INVALID_STATE,
            "recycled native object inherited retired canonical binding");
    f.bind(replacement);
    exact(neighbors(self),{{1,0,0},4});
    IsleGameResult distance_result = ISLE_GAME_OK, neighbor_result = ISLE_GAME_OK;
    output = -17;
    std::thread foreign([&] {
        distance_result = isle_actor_spatial_distance(f.game,self.object,replacement.object,&output);
        neighbor_result = isle_actor_spatial_neighbors(&self.source,&direction,&magnitude);
    });
    foreign.join();
    require(distance_result == ISLE_GAME_WRONG_OWNER && neighbor_result == ISLE_GAME_WRONG_OWNER &&
            same(output,-17) && same(direction,sentinel) && same(magnitude,-17),
            "unadmitted thread read actual native spatial state or published output");
}
} // namespace

int main() {
    try {
        distance_auxiliary_and_root(); distance_null_model_and_error_order();
        source_float_distance_extremes(); neighbors_three_dimensions_weights_and_exclusions();
        neighbors_boundary_coincident_and_paused(); ordered_ties_and_cancellation();
        neighbors_subnormal_consumption(); reciprocal_rounding_subnormal_and_complete_cohort();
        nonmutating_actual_pose_and_records();
        neighbor_admission_and_transactional_errors(); real_retirement_and_owner_access();
        std::cout << "native actor spatial contracts passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
