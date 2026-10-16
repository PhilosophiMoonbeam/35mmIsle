#include "actor_photo_membership.hpp"
#include "actor_owner_source.h"
#include <array>
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
namespace photo = isle::actor::photo;
namespace focus = isle::focus;
constexpr float nan = std::numeric_limits<float>::quiet_NaN();
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
void native(IsleGameResult result) { require(result == ISLE_GAME_OK, "native operation rejected"); }
struct Actor {
    IsleActorSource source{};
    isle_rng rng{1};
    IsleGameObject* object = nullptr;
    IsleGameModel* model = nullptr;
};
struct Fixture {
    std::vector<std::byte> bytes = std::vector<std::byte>(1024 * 1024);
    isle_arena arena{};
    IsleGame* game = nullptr;
    IsleGameObject* camera_object = nullptr;
    IsleGameCamera* camera = nullptr;
    std::array<Actor,64> actors{};
    std::size_t count = 0;
    Fixture() {
        require(isle_arena_init(&arena, bytes.data(), bytes.size()) == ISLE_OK, "arena init");
        native(isle_game_create(&arena, &game));
        native(isle_game_add_object(game, 9000, 0, 0, nullptr, nullptr, &camera_object));
        native(isle_game_attach_camera(game, camera_object, &camera));
        IsleGamePerspective perspective{60,4.0f/3.0f,1,20000,1};
        IsleGameLookAt look{{0,0,0},{0,0,-1},{0,1,0}};
        native(isle_game_camera_set_perspective(game, camera, &perspective));
        native(isle_game_camera_set_look_at(game, camera, &look));
    }
    ~Fixture() { if (game && isle_game_destroy(game) != ISLE_GAME_OK) std::abort(); }
    Fixture(const Fixture&) = delete;
    Fixture& operator=(const Fixture&) = delete;
    void bind(Actor& actor) {
        native(isle_actor_source_bind(game, actor.object, &actor.source, &actor.rng, &actor));
        actor.source.id = 129;
        actor.source.tangible = 1;
        actor.source.playerDist = 100;
        actor.source.collPosition = {0,0,-10};
    }
    Actor& add(std::uint32_t priority, std::uint8_t link = 3,
               bool bound = true, bool model = true) {
        require(count < actors.size(), "native fixture capacity");
        auto& actor = actors[count++];
        native(isle_game_add_object(game, static_cast<std::uint32_t>(count), link,
                                   priority, nullptr, nullptr, &actor.object));
        if (bound) bind(actor);
        if (model) {
            // A real contextual model, deliberately far from collPosition. No
            // AUX placement or rendered-origin collision surrogate is supplied.
            IsleGameModelNodeInit root{UINT32_MAX, {{50000,50000,50000},{},{1,1,1}},
                                       ISLE_GAME_MODEL_CONTEXTUAL_RPY};
            native(isle_game_attach_model(game, actor.object, &root, 1, &actor.model));
        }
        return actor;
    }
};
focus::Object identity(const Actor& actor) { return reinterpret_cast<focus::Object>(actor.object); }
focus::Photo sentinel() {
    focus::Photo result{};
    for (std::size_t i = 0; i < result.objects.size(); ++i) result.objects[i] = 0x123400 + i;
    result.focus = 0xABCDE;
    return result;
}
bool same(const focus::Photo& a, const focus::Photo& b) {
    return a.objects == b.objects && a.focus == b.focus;
}
void expect(const focus::Photo& actual, std::initializer_list<const Actor*> expected) {
    std::size_t i = 0;
    for (const auto* actor : expected)
        require(actual.objects[i++] == identity(*actor), "native membership identity/order differs");
    while (i < actual.objects.size())
        require(actual.objects[i++] == 0, "snapshot retained a stale membership tail");
    require(actual.focus == 0, "snapshot retained prior focus");
}
struct Cursor {
    IsleGameObject* object;
    IsleActorSource* source;
    IsleGameModel* model;
    explicit Cursor(Actor& seed) : object(seed.object), source(&seed.source), model(seed.model) {}
    IsleGameResult next(Fixture& f, IsleGameObject* previous) {
        return isle_actor_source_link3_next(f.game, previous, &object, &source, &model);
    }
    bool empty() const { return !object && !source && !model; }
};
void native_order_and_identity() {
    Fixture f;
    auto& low = f.add(10);
    auto& high_first = f.add(200);
    auto& high_second = f.add(200);
    auto& middle = f.add(80);
    auto& highest = f.add(UINT32_MAX);
    auto& other_link = f.add(UINT32_MAX, 4);
    for (auto& actor : f.actors) if (actor.object) actor.source.flags = 0x120;
    const auto used = f.arena.cursor;
    const IsleGameModelNode* nodes = nullptr;
    std::size_t node_count = 0;
    native(isle_game_model_peek_nodes(f.game, high_first.model, &nodes, &node_count));
    require(node_count == 1, "fixture model topology");
    const auto original_node = nodes[0];
    const IsleGameCameraParameters* parameters = nullptr;
    native(isle_game_camera_parameters(f.game, f.camera, &parameters));
    const auto original_camera = *parameters;
    Cursor cursor(low);
    IsleGameObject* previous = nullptr;
    for (const auto* actor : {&highest,&high_first,&high_second,&middle,&low}) {
        native(cursor.next(f, previous));
        require(cursor.object == actor->object && cursor.source == &actor->source &&
                cursor.model == actor->model && actor->source.flags == 0x120,
                "cursor copied/substituted the native record/model or mutated visibility");
        previous = cursor.object;
    }
    native(cursor.next(f, previous));
    require(cursor.empty(), "native end did not clear all cursor outputs");
    auto output = sentinel();
    native(photo::collect_membership(f.game, f.camera, output));
    expect(output, {&highest,&high_first,&high_second,&middle,&low});
    require(low.source.flags == 0x20 && high_first.source.flags == 0x20 &&
            high_second.source.flags == 0x20 && middle.source.flags == 0x20 &&
            highest.source.flags == 0x20 && other_link.source.flags == 0x120,
            "membership did not update only visited link3 flag100 bits");
    require(f.arena.cursor == used && std::memcmp(&original_node, nodes, sizeof(original_node)) == 0 &&
            std::memcmp(&original_camera, parameters, sizeof(original_camera)) == 0,
            "collection allocated or mutated native camera/model caches");
}
void twelve_admissions_not_twelve_visits() {
    Fixture f;
    std::array<Actor*,12> accepted{}, rejected{};
    for (std::size_t i = 0; i < accepted.size(); ++i) {
        rejected[i] = &f.add(static_cast<std::uint32_t>(1000 - 2*i));
        auto& reject = *rejected[i];
        reject.source.flags = 0x120;
        switch (i % 4) {
        case 0: native(isle_game_object_set_flags(f.game, reject.object, 1)); break;
        case 1: reject.source.tangible = 0; break;
        case 2: reject.source.id = 999; break;
        case 3: reject.source.collPosition = {1000,0,-10}; break;
        }
        accepted[i] = &f.add(static_cast<std::uint32_t>(999 - 2*i));
        accepted[i]->source.flags = 0x120;
    }
    auto& unbound_tail = f.add(100, 3, false);
    auto& visible_tail = f.add(90);
    visible_tail.source.flags = 0x120;
    auto output = sentinel();
    const auto used = f.arena.cursor;
    native(photo::collect_membership(f.game, f.camera, output));
    for (std::size_t i = 0; i < accepted.size(); ++i) {
        require(output.objects[i] == identity(*accepted[i]) && accepted[i]->source.flags == 0x20,
                "collector counted visited nodes instead of twelve admitted native objects");
        const std::uint16_t expected_flags = i % 4 == 2 ? 0x20 : 0x120;
        require(rejected[i]->source.flags == expected_flags,
                "interleaved source rejection changed flag100 branch ordering");
    }
    require(output.focus == 0 && visible_tail.source.flags == 0x120 && f.arena.cursor == used,
            "full snapshot visited/mutated its native tail or allocated");
    // The tail really is the next native node, and would fail if visited. This
    // distinguishes a hard capacity stop from silently skipping unbound nodes.
    Cursor cursor(*accepted.back());
    require(cursor.next(f, accepted.back()->object) == ISLE_GAME_INVALID_STATE && cursor.empty(),
            "fixture's unbound node was not the actual native tail boundary");
    const auto prior = output;
    accepted.back()->source.tangible = 0;
    require(photo::collect_membership(f.game, f.camera, output) == ISLE_GAME_INVALID_STATE &&
            same(output, prior) && visible_tail.source.flags == 0x120,
            "collector skipped a required unbound twelfth-admission successor");
    f.bind(unbound_tail);
    native(photo::collect_membership(f.game, f.camera, output));
    require(output.objects.back() == identity(unbound_tail) && visible_tail.source.flags == 0x120,
            "bound twelfth admission did not stop before its visible successor");
}
void source_admission_order_and_canonical_position() {
    Fixture f;
    auto& hidden = f.add(100);
    hidden.source.flags = 0x120;
    hidden.source.playerDist = nan;
    hidden.source.collPosition = {nan,nan,nan};
    native(isle_game_object_set_flags(f.game, hidden.object, 1));
    auto& intangible = f.add(90);
    intangible.source.flags = 0x120;
    intangible.source.tangible = 0;
    intangible.source.playerDist = nan;
    intangible.source.collPosition = {nan,nan,nan};
    auto& disallowed_visible = f.add(80);
    disallowed_visible.source.id = 999;
    disallowed_visible.source.flags = 0x120;
    auto& disallowed_far = f.add(70);
    disallowed_far.source.id = -1;
    disallowed_far.source.flags = 0x20;
    disallowed_far.source.playerDist = 10001;
    disallowed_far.source.collPosition = {nan,nan,nan};
    auto& bypass = f.add(60);
    bypass.source.flags = 0x160;
    bypass.source.playerDist = nan;
    bypass.source.collPosition = {nan,nan,nan};
    auto& canonical = f.add(50);
    canonical.source.flags = 0x120;
    canonical.source.collisionOffset = {nan,nan,nan};
    auto& exceptional_species = f.add(40);
    exceptional_species.source.id = 1038;
    exceptional_species.source.flags = 0x120;
    auto output = sentinel();
    native(photo::collect_membership(f.game, f.camera, output));
    expect(output, {&bypass,&canonical,&exceptional_species});
    require(hidden.source.flags == 0x120 && intangible.source.flags == 0x120,
            "hidden/intangible membership touched visibility or consumed invalid numeric fields");
    require(disallowed_visible.source.flags == 0x20 && disallowed_far.source.flags == 0x120 &&
            bypass.source.flags == 0x60 && canonical.source.flags == 0x20,
            "species preceded visibility, bypass consumed distance/point, or rendered origin replaced collPosition");
}
void whole_collection_visibility_boundaries_and_actual_camera() {
    Fixture f;
    auto& near = f.add(100); near.source.collPosition = {0,0,-1};
    auto& before_near = f.add(99); before_near.source.collPosition = {0,0,-0.999f};
    auto& far = f.add(98); far.source.collPosition = {0,0,-10000};
    auto& beyond_far = f.add(97); beyond_far.source.collPosition = {0,0,-10001};
    constexpr float depth = -228.506134f;
    auto& right = f.add(96); right.source.collPosition = {240.99f,0,depth};
    auto& outside_right = f.add(95); outside_right.source.collPosition = {241.01f,0,depth};
    auto& left = f.add(94); left.source.collPosition = {-240.99f,0,depth};
    auto& up = f.add(93); up.source.collPosition = {0,180.99f,depth};
    auto& outside_up = f.add(92); outside_up.source.collPosition = {0,181.01f,depth};
    auto& down = f.add(91); down.source.collPosition = {0,-180.99f,depth};
    auto& behind = f.add(90); behind.source.collPosition = {0,0,10};
    auto output = sentinel();
    native(photo::collect_membership(f.game, f.camera, output));
    expect(output, {&near,&far,&right,&left,&up,&down});
    require(before_near.source.flags == 0x100 && beyond_far.source.flags == 0x100 &&
            outside_right.source.flags == 0x100 && outside_up.source.flags == 0x100 &&
            behind.source.flags == 0x100, "rejected boundary rows lost canonical invisible flags");
    IsleGameLookAt reverse{{0,0,0},{0,0,1},{0,1,0}};
    native(isle_game_camera_set_look_at(f.game, f.camera, &reverse));
    native(photo::collect_membership(f.game, f.camera, output));
    expect(output, {&behind});
    require(behind.source.flags == 0 && near.source.flags == 0x100,
            "collector reused an old/surrogate camera instead of the actual attached camera");
}
void empty_and_short_snapshot_reset() {
    Fixture f;
    auto output = sentinel();
    native(photo::collect_membership(f.game, f.camera, output));
    expect(output, {});
    auto& actor = f.add(1);
    native(photo::collect_membership(f.game, f.camera, output));
    expect(output, {&actor});
    output.focus = identity(actor);
    actor.source.tangible = 0;
    native(photo::collect_membership(f.game, f.camera, output));
    expect(output, {});
}
void unbound_error_preserves_photo_not_prior_flags() {
    Fixture f;
    auto& visible = f.add(100); visible.source.flags = 0x120;
    auto& invisible = f.add(90); invisible.source.flags = 0x20;
    invisible.source.collPosition = {0,0,10};
    auto& unbound = f.add(80, 3, false);
    auto& later = f.add(70); later.source.flags = 0x120;
    const auto prior = sentinel();
    auto output = prior;
    require(photo::collect_membership(f.game, f.camera, output) == ISLE_GAME_INVALID_STATE &&
            same(output, prior), "unbound visited node was skipped or published a partial photo");
    require(visible.source.flags == 0x20 && invisible.source.flags == 0x120 && later.source.flags == 0x120,
            "error rolled back prior native visibility or continued into later nodes");
    Cursor cursor(later);
    require(cursor.next(f, invisible.object) == ISLE_GAME_INVALID_STATE && cursor.empty(),
            "unbound native cursor failure exposed partial outputs");
    f.bind(unbound);
    native(photo::collect_membership(f.game, f.camera, output));
    expect(output, {&visible,&unbound,&later});
}
void model_binding_and_consumed_numeric_errors() {
    Fixture f;
    auto& first = f.add(100); first.source.flags = 0x120;
    auto& second = f.add(90);
    auto& later = f.add(80); later.source.flags = 0x120;
    const auto prior = sentinel();
    auto output = prior;
    second.source.id = 999;
    second.source.collPosition.x = nan;
    require(photo::collect_membership(f.game, f.camera, output) == ISLE_GAME_INVALID_ARGUMENT &&
            same(output, prior) && first.source.flags == 0x20 && later.source.flags == 0x120,
            "later consumed numeric error was hidden by species or rolled back/published prior work");
    second.source.collPosition = {0,0,-10};
    auto* actual_object = second.source.object;
    second.source.object = first.object;
    Cursor cursor(later);
    require(cursor.next(f, first.object) == ISLE_GAME_INVALID_STATE && cursor.empty(),
            "mismatched canonical native binding was accepted");
    second.source.object = actual_object;
    native(isle_game_remove_model(f.game, second.model));
    // Even a hidden/intangible visited node must have a real attached model;
    // ownership validation is not bypassed by source admission branches.
    second.source.tangible = 0;
    native(isle_game_object_set_flags(f.game, second.object, 1));
    require(cursor.next(f, first.object) == ISLE_GAME_INVALID_STATE && cursor.empty(),
            "cursor fabricated/skipped a missing model for a rejected native object");
    require(photo::collect_membership(f.game, f.camera, output) == ISLE_GAME_INVALID_STATE &&
            same(output, prior), "missing attached native model published a partial photo");
}
void cursor_arguments_and_owner_access() {
    Fixture f;
    auto& actor = f.add(10);
    auto& wrong_link = f.add(20, 4);
    Cursor cursor(actor);
    require(cursor.next(f, wrong_link.object) == ISLE_GAME_INVALID_ARGUMENT && cursor.empty(),
            "cursor accepted a live object on a different native link");
    cursor = Cursor(actor);
    require(isle_actor_source_link3_next(nullptr, nullptr, &cursor.object, &cursor.source, &cursor.model) ==
                ISLE_GAME_INVALID_ARGUMENT && cursor.empty(), "null game exposed native cursor outputs");
    cursor = Cursor(actor);
    require(isle_actor_source_link3_next(f.game, nullptr, nullptr, &cursor.source, &cursor.model) ==
                ISLE_GAME_INVALID_ARGUMENT && !cursor.source && !cursor.model,
            "missing object output did not clear remaining cursor outputs");
    cursor = Cursor(actor);
    require(isle_actor_source_link3_next(f.game, nullptr, &cursor.object, nullptr, &cursor.model) ==
                ISLE_GAME_INVALID_ARGUMENT && !cursor.object && !cursor.model,
            "missing source output did not clear remaining cursor outputs");
    cursor = Cursor(actor);
    require(isle_actor_source_link3_next(f.game, nullptr, &cursor.object, &cursor.source, nullptr) ==
                ISLE_GAME_INVALID_ARGUMENT && !cursor.object && !cursor.source,
            "missing model output did not clear remaining cursor outputs");
    actor.source.flags = 0x120;
    auto output = sentinel();
    const auto prior = output;
    IsleGameResult cursor_result = ISLE_GAME_OK, membership_result = ISLE_GAME_OK;
    cursor = Cursor(actor);
    std::thread foreign([&] {
        cursor_result = cursor.next(f, nullptr);
        membership_result = photo::collect_membership(f.game, f.camera, output);
    });
    foreign.join();
    require(cursor_result == ISLE_GAME_WRONG_OWNER && membership_result == ISLE_GAME_WRONG_OWNER &&
            cursor.empty() && same(output, prior) && actor.source.flags == 0x120,
            "unadmitted native thread traversed/mutated membership or exposed outputs");
    require(photo::collect_membership(f.game, nullptr, output) == ISLE_GAME_INVALID_ARGUMENT &&
            same(output, prior) && actor.source.flags == 0x120,
            "camera failure occurred after native visibility mutation or changed output");
    native(isle_game_remove_camera(f.game, f.camera));
    require(photo::collect_membership(f.game, f.camera, output) == ISLE_GAME_INVALID_ARGUMENT &&
            same(output, prior) && actor.source.flags == 0x120,
            "detached camera was accepted or failed after native membership sideeffects");
}
struct RetirementObservation {
    IsleGame* game = nullptr;
    IsleGameObject* previous = nullptr;
    Cursor cursor;
    IsleGameResult result = ISLE_GAME_OK;
    explicit RetirementObservation(Actor& actor) : cursor(actor) {}
};
void observe_retiring(IsleGameObject*, void* user) {
    auto& observation = *static_cast<RetirementObservation*>(user);
    observation.result = isle_actor_source_link3_next(observation.game, observation.previous,
        &observation.cursor.object, &observation.cursor.source, &observation.cursor.model);
}
void actual_retirement_and_identity_reuse() {
    Fixture f;
    auto& old = f.add(100);
    old.source.flags = 0x120;
    auto output = sentinel();
    native(photo::collect_membership(f.game, f.camera, output));
    expect(output, {&old});
    const auto old_identity = identity(old);
    native(isle_game_object_retain_identity(f.game, old.object));
    RetirementObservation observation(old);
    observation.game = f.game;
    native(isle_game_object_set_pre_retire_callback(f.game, old.object, observe_retiring, &observation));
    native(isle_game_delete_object(f.game, old.object));
    require(observation.result == ISLE_GAME_INVALID_STATE && observation.cursor.empty(),
            "cursor exposed the actual retiring native head before model removal/unlink");
    Cursor cursor(old);
    require(cursor.next(f, old.object) == ISLE_GAME_INVALID_ARGUMENT && cursor.empty(),
            "dead held identity was accepted as a live traversal cursor");
    native(photo::collect_membership(f.game, f.camera, output));
    expect(output, {});
    auto& interim = f.add(10);
    require(identity(interim) != old_identity, "photo borrow allowed reuse of the held retired native identity");
    native(photo::collect_membership(f.game, f.camera, output));
    expect(output, {&interim});
    native(isle_game_object_release_identity(f.game, old.object));
    // Never call through old.object after this release: only its saved numeric
    // identity is compared with the newly returned live allocator handle.
    auto& reused = f.add(200, 3, false);
    require(identity(reused) == old_identity, "lifetime regression did not exercise actual native address reuse");
    const auto prior = output;
    require(photo::collect_membership(f.game, f.camera, output) == ISLE_GAME_INVALID_STATE &&
            same(output, prior), "recycled unbound native slot inherited retired source membership");
    require(isle_actor_source_bind(f.game, reused.object, &old.source, &old.rng, &old) == ISLE_GAME_INVALID_STATE,
            "retired nonreset canonical source record rebound to a recycled slot");
    f.bind(reused);
    native(photo::collect_membership(f.game, f.camera, output));
    expect(output, {&reused,&interim});
    require(reused.source.flags == 0 && old.source.flags == 0x20,
            "native address reuse copied the old record/visibility state");
}
} // namespace
int main() {
    try {
        native_order_and_identity(); twelve_admissions_not_twelve_visits();
        source_admission_order_and_canonical_position();
        whole_collection_visibility_boundaries_and_actual_camera();
        empty_and_short_snapshot_reset(); unbound_error_preserves_photo_not_prior_flags();
        model_binding_and_consumed_numeric_errors(); cursor_arguments_and_owner_access();
        actual_retirement_and_identity_reuse();
        std::cout << "native actor photo membership contracts passed\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
