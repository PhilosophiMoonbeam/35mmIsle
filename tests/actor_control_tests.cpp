#include "actor_control_fixture.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {
constexpr IsleActorInteraction terminal_row{58,nullptr,0,nullptr};
constexpr IsleActorInteractionGraph pending_graph{&terminal_row,1,0};
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
void ok(IsleGameResult result) { require(result == ISLE_GAME_OK, "native actor operation rejected"); }
class World {
public:
    World() : bytes(65536) {
        require(isle_arena_init(&arena, bytes.data(), bytes.size()) == ISLE_OK, "arena init");
        ok(isle_game_create(&arena, &game));
    }
    ~World() { if (game && isle_game_destroy(game) != ISLE_GAME_OK) std::abort(); }
    World(const World&) = delete;
    World& operator=(const World&) = delete;
    std::vector<unsigned char> bytes;
    isle_arena arena{};
    IsleGame* game = nullptr;
};
IsleGameObject* bind(World& world, ActorControlFixture& f, isle_rng& rng, bool update = false) {
    IsleGameObject* object = nullptr;
    ok(isle_game_add_object(world.game, 1, 0, 0, update ? actor_control_update : nullptr,
                           update ? &f : nullptr, &object));
    ok(isle_actor_source_bind(world.game, object, &f.actor, &rng, &f));
    return object;
}
IsleGameProcess* start(World& world, IsleGameObject* object, IsleGameSourceCallback callback,
                       std::uint32_t priority = 7, IsleGameProcessKind kind = ISLE_GAME_STACKFUL) {
    IsleGameProcess* process = nullptr;
    ok(isle_game_add_source_process(world.game, object, callback, kind, priority, &process));
    IsleGameSourceCallback identity = nullptr;
    ok(isle_game_process_source_function(world.game, process, &identity));
    require(identity == callback, "native source identity was replaced by an adapter");
    return process;
}
void driver(World& world, IsleGameCallback callback, ActorControlFixture& f) {
    IsleGameObject* object = nullptr;
    IsleGameProcess* process = nullptr;
    ok(isle_game_add_object(world.game, 2, 0, 0, nullptr, nullptr, &object));
    ok(isle_game_add_process(world.game, object, callback, &f, ISLE_GAME_SYNCHRONOUS, 8, &process));
}
IsleGameCounts counts(World& world) {
    IsleGameCounts result{};
    ok(isle_game_counts(world.game, &result));
    return result;
}
void trace(const ActorControlFixture& f, std::initializer_list<int> expected) {
    require(f.failures == 0 && f.after_end == 0 && f.old_resumed == 0,
            "real C control callback resumed a retired frame or violated native metadata");
    require(f.count == expected.size(), "source control event extent differs");
    std::size_t i = 0;
    for (int value : expected) require(f.events[i++] == value, "source control ordering differs");
}

void self_replacement_and_null_state() {
    ActorControlFixture f{}; isle_rng rng{1}; World world;
    auto* object = bind(world, f, rng);
    f.actor.transitionGraph = &pending_graph;
    f.actor.stateProc = start(world, object, actor_control_replace_self);
    ok(isle_game_tick(world.game));
    trace(f, {10, 11});
    require(f.actor.stateProc == nullptr && f.actor.transitionGraph == nullptr && counts(world).processes == 0,
            "new state was not published before self finish or NULL state retained a process");
    for (int i = 0; i < 3; ++i) ok(isle_game_tick(world.game));
    trace(f, {10, 11});
}

void cross_state_replacement_joins_old_stack() {
    ActorControlFixture f{}; isle_rng rng{1}; World world;
    auto* object = bind(world, f, rng);
    f.actor.stateProc = start(world, object, actor_control_sleep_state);
    ok(isle_game_tick(world.game));
    f.actor.transitionGraph = &pending_graph;
    driver(world, actor_control_cross_replace, f);
    ok(isle_game_tick(world.game));
    trace(f, {20, 21, 22, 11});
    require(f.actor.stateProc == nullptr && counts(world).processes == 0,
            "cross-state replacement retained the sleeping old source process");
    for (int i = 0; i < 51; ++i) ok(isle_game_tick(world.game));
    trace(f, {20, 21, 22, 11});
}

void auxiliary_priority_flags_and_null_path() {
    ActorControlFixture f{}; isle_rng rng{1}; World world;
    auto* object = bind(world, f, rng);
    f.actor.processFlags = 0x13a;
    f.actor.stateProc = start(world, object, actor_control_aux_path);
    ok(isle_game_tick(world.game));
    trace(f, {30, 31, 32, 33});
    require(f.actor.processFlags == 0x108 && counts(world).processes == 2,
            "aux self-stop did not set its flag while retaining state/path");
    ok(isle_game_tick(world.game));
    trace(f, {30, 31, 32, 33, 34});
    require(f.actor.processFlags == 0x13a && f.actor.pathProc == nullptr && counts(world).processes == 0,
            "NULL path changed flags or did not join its real old continuation");
}

void path_replacement_and_recycled_storage() {
    ActorControlFixture f{}; isle_rng rng{1}; World world;
    auto* object = bind(world, f, rng);
    f.actor.pathProc = start(world, object, actor_control_path_old, 5);
    ok(isle_game_tick(world.game));
    f.actor.processFlags = 0x13a;
    driver(world, actor_control_replace_path, f);
    ok(isle_game_tick(world.game));
    trace(f, {33, 36, 35});
    require(f.actor.processFlags == 0x108 && f.actor.pathProc != f.old_path,
            "replacement path failed to create/publish before joining old path");
    f.actor.processFlags = 0x13a;
    driver(world, actor_control_null_path, f);
    ok(isle_game_tick(world.game));
    trace(f, {33, 36, 35, 37});
    require(f.actor.processFlags == 0x13a && f.actor.pathProc == nullptr && counts(world).processes == 0,
            "NULL replacement must preserve all path flags");
    const auto cursor = world.arena.cursor;
    f.actor.pathProc = start(world, object, actor_control_path_old, 5);
    require(world.arena.cursor == cursor, "joined source process storage was not reusable");
    ok(isle_game_tick(world.game));
    ok(isle_game_delete_object(world.game, object));
    require(actor_control_joined(f.actor.pathProc), "actual actor deletion did not join reused path");
    for (int i = 0; i < 51; ++i) ok(isle_game_tick(world.game));
    trace(f, {33, 36, 35, 37, 33});
}

std::uint32_t seed_for_high16(std::uint32_t high) {
    // Independent modulo-2^32 inverse, not an invocation of the native RNG.
    std::uint32_t inverse = 1;
    for (int i = 0; i < 5; ++i) inverse *= 2u - 214013u * inverse;
    return inverse * ((high << 16) - 2531011u);
}
void random_boundaries_zero_weights_and_application_stream() {
    for (std::uint32_t high : {0u, 16383u, 16384u, 32768u, 65535u}) {
        std::array<ActorControlFixture, 2> f{};
        isle_rng rng{seed_for_high16(high)};
        const auto initial_seed = rng.seed;
        World world;
        IsleActorRandomState list[] = {
            {0, actor_control_forbidden_choice}, {1, actor_control_choice_a},
            {0, actor_control_forbidden_choice}, {3, actor_control_choice_b},
            {0, actor_control_forbidden_choice}, {0, nullptr}
        };
        for (auto& actor : f) {
            auto* object = bind(world, actor, rng);
            actor.random = list;
            actor.actor.transitionGraph = &pending_graph;
            actor.actor.stateProc = start(world, object, actor_control_random);
        }
        std::uint32_t reference = initial_seed;
        std::array<int, 2> expected{};
        for (auto& choice : expected) {
            reference = reference * 214013u + 2531011u;
            const std::uint32_t draw = ((reference >> 16) * 4u) / 65536u;
            choice = draw < 1u ? 0 : 1;
        }
        ok(isle_game_tick(world.game));
        require(rng.seed == reference && f[0].choice == expected[0] && f[1].choice == expected[1],
                "source weighted choice did not consume the shared application stream exactly once per actor");
        require(f[0].choice == (high < 16384u ? 0 : 1), "source <= boundary or leading zero weight changed");
        for (const auto& actor : f) {
            require(actor.failures == 0 && actor.after_end == 0 && actor.count == 1 &&
                    actor.actor.stateProc == nullptr && actor.actor.transitionGraph == nullptr,
                    "random replacement selected a zero entry or resumed its old stack");
        }
        require(counts(world).processes == 0, "random replacement leaked source processes");
    }
}

void waits_observe_any_mask_on_actual_native_updates() {
    for (std::uint32_t signal : {0x002u, 0x008u}) {
        ActorControlFixture f{}; isle_rng rng{1}; World world;
        auto* object = bind(world, f, rng, true);
        f.mask = 0x00a; f.signal = signal; f.signal_at = 3;
        f.actor.stateProc = start(world, object, actor_control_wait);
        ok(isle_game_tick(world.game));
        trace(f, {50});
        require(f.updates == 1 && counts(world).processes == 1, "wait did not suspend its real C stack");
        ok(isle_game_tick(world.game));
        trace(f, {50});
        ok(isle_game_tick(world.game));
        trace(f, {50, 51});
        require(f.wait_return_update == 3 && f.actor.processFlags == signal && counts(world).processes == 0,
                "multi-mask wait required all bits or resumed on the wrong tick");
        require(actor_control_update_context(object) == &f && f.actor.rng == &rng && f.actor.owner == &f,
                "native payload corrupted independent update context/application state");
    }
    ActorControlFixture f{}; isle_rng rng{1}; World world;
    auto* object = bind(world, f, rng, true);
    f.mask = 0; f.signal = 0xffffffffu; f.signal_at = 1;
    f.actor.stateProc = start(world, object, actor_control_wait);
    for (int i = 0; i < 5; ++i) ok(isle_game_tick(world.game));
    trace(f, {50});
    auto* waiting = f.actor.stateProc;
    ok(isle_game_delete_object(world.game, object));
    require(actor_control_joined(waiting), "zero-mask endless wait did not join at retirement");
    ok(isle_game_tick(world.game));
    trace(f, {50});
}

void binding_and_native_address_reuse() {
    ActorControlFixture old{}, next{}; isle_rng rng{1}; World world;
    auto* object = bind(world, old, rng);
    IsleActorSource* record = nullptr;
    ok(isle_actor_source_get(world.game, object, &record));
    require(record == &old.actor, "bridge did not borrow the actual semantic record");
    require(isle_actor_source_bind(world.game, object, &next.actor, &rng, &next) == ISLE_GAME_INVALID_STATE,
            "binding replaced a live GAME payload");
    old.mask = 0;
    old.actor.stateProc = start(world, object, actor_control_wait);
    ok(isle_game_tick(world.game));
    auto* waiting = old.actor.stateProc;
    ok(isle_game_delete_object(world.game, object));
    require(actor_control_joined(waiting), "retirement did not join bound actor callbacks");
    record = &old.actor;
    require(isle_actor_source_get(world.game, object, &record) == ISLE_GAME_INVALID_ARGUMENT && record == nullptr,
            "retired object exposed a stale semantic payload");
    IsleGameObject* reused = nullptr;
    ok(isle_game_add_object(world.game, 3, 0, 0, nullptr, nullptr, &reused));
    require(reused == object && actor_control_payload_empty(reused) && actor_control_update_context(reused) == nullptr,
            "native reuse retained actor payload or update context");
    require(isle_actor_source_get(world.game, reused, &record) == ISLE_GAME_INVALID_STATE && record == nullptr,
            "fresh native object inferred actor identity from a reused address");
    auto foreign=std::make_unique<int>(456);
    actor_control_foreign_payload(reused,foreign.get());
    require(isle_actor_source_get(world.game,reused,&record)==ISLE_GAME_INVALID_STATE && record==nullptr && *foreign==456,
            "reused native slot retained an actor-binding discriminator over foreign payload");
    actor_control_foreign_payload(reused,nullptr);
    require(isle_actor_source_bind(world.game, reused, &old.actor, &rng, &old) == ISLE_GAME_INVALID_STATE,
            "stale non-reset semantic record rebound after native address reuse");
    ok(isle_actor_source_bind(world.game, reused, &next.actor, &rng, &next));
    ok(isle_actor_source_get(world.game, reused, &record));
    require(record == &next.actor && old.count == 1, "new actor borrowed the retired actor's semantic state");
}

void nullable_lookup_distinguishes_absence_from_invalid_ownership() {
    ActorControlFixture f{}; isle_rng rng{1}; World world;
    auto* object = bind(world, f, rng);
    IsleActorSource* output = nullptr;
    ok(isle_actor_source_get_nullable(world.game, object, &output));
    require(output == &f.actor, "nullable lookup lost a live canonical actor");
    f.actor.owner = nullptr;
    require(isle_actor_source_get_nullable(world.game, object, &output) == ISLE_GAME_INVALID_STATE &&
            output == nullptr, "corrupt actor ownership was treated as normal absence");
    f.actor.owner = &f;
    IsleGameObject* empty = nullptr;
    ok(isle_game_add_object(world.game, 5, 0, 0, nullptr, nullptr, &empty));
    output = &f.actor;
    ok(isle_actor_source_get_nullable(world.game, empty, &output));
    require(output == nullptr, "missing semantic payload retained an old actor identity");
    require(isle_actor_source_get(world.game, empty, &output) == ISLE_GAME_INVALID_STATE &&
            output == nullptr, "nullable query weakened existing required-actor lookup");
    require(isle_actor_source_get_nullable(world.game, reinterpret_cast<IsleGameObject*>(&f), &output) == ISLE_GAME_INVALID_ARGUMENT &&
            output == nullptr, "foreign native identity was treated as normal actor absence");
}

void nullable_lookup_accepts_only_joined_identity_held_absence() {
    ActorControlFixture f{}; isle_rng rng{1}; World world;
    auto* object = bind(world, f, rng);
    ok(isle_game_object_retain_identity(world.game, object));
    const auto release = [game=world.game](IsleGameObject* held) {
        if (isle_game_object_release_identity(game, held) != ISLE_GAME_OK) std::abort();
    };
    std::unique_ptr<IsleGameObject,decltype(release)> held(object,release);
    f.mask = 0;
    f.actor.stateProc = start(world, object, actor_control_wait);
    ok(isle_game_tick(world.game));
    auto* waiting = f.actor.stateProc;
    ok(isle_game_delete_object(world.game, object));
    require(actor_control_joined(waiting), "retained Actor deletion did not join its real continuation");
    IsleActorSource* output = &f.actor;
    require(isle_actor_source_get_nullable(world.game, object, &output) == ISLE_GAME_OK &&
            output == nullptr, "joined identity-held Actor was not normal nullable absence");
    require(isle_actor_source_get(world.game, object, &output) == ISLE_GAME_INVALID_ARGUMENT &&
            output == nullptr, "required Actor lookup accepted joined identity-only storage");
    IsleGameObject* pressure = nullptr;
    ok(isle_game_add_object(world.game, 6, 0, 0, nullptr, nullptr, &pressure));
    require(pressure != object, "Photo-style identity hold allowed native object address reuse");
    held.reset();
    require(isle_actor_source_get_nullable(world.game, object, &output) == ISLE_GAME_INVALID_ARGUMENT &&
            output == nullptr, "unheld stale Actor identity was treated as nullable absence");
}

void foreign_payload_is_not_an_actor_record() {
    auto payload=std::make_unique<int>(123);
    World world;
    IsleGameObject* object=nullptr;
    ok(isle_game_add_object(world.game,4,0,0,nullptr,nullptr,&object));
    actor_control_foreign_payload(object,payload.get());
    IsleActorSource sentinel{},*output=&sentinel;
    require(isle_actor_source_get(world.game,object,&output)==ISLE_GAME_INVALID_STATE && output==nullptr && *payload==123,
            "unbound foreign native payload was interpreted as an actor source record");
    output = &sentinel;
    require(isle_actor_source_get_nullable(world.game,object,&output)==ISLE_GAME_INVALID_STATE &&
            output==nullptr && *payload==123,
            "nullable lookup swallowed a foreign non-Actor payload");
}

void creation_failure_exits_both_c_fences() {
    for (auto kind : {ISLE_GAME_STACKFUL, ISLE_GAME_SYNCHRONOUS}) {
        ActorControlFixture f{}; isle_rng rng{1}; World world;
        auto* object = bind(world, f, rng);
        f.actor.transitionGraph = &pending_graph;
        f.actor.stateProc = start(world, object, actor_control_creation_failure, 7, kind);
        auto* original = f.actor.stateProc;
        isle_span consumed{};
        require(isle_arena_allocate(&world.arena, world.arena.storage.size - world.arena.cursor, 1, &consumed) == ISLE_OK,
                "could not consume actual bounded native arena");
        require(isle_game_tick(world.game) == ISLE_GAME_EXECUTION_FAILURE,
                "real source process allocation failure did not close the native tick");
        trace(f, {60});
        require(f.actor.stateProc == original && f.actor.transitionGraph == nullptr && counts(world).processes == 1,
                "failed create published NULL/new state or silently ended old source process");
        IsleGameProcess* rejected = original;
        require(isle_game_add_source_process(world.game, object, actor_control_new_state,
                    ISLE_GAME_STACKFUL, 7, &rejected) == ISLE_GAME_EXECUTION_FAILURE && rejected == nullptr,
                "poisoned owner-side creation did not report status and clear output");
        require(isle_game_tick(world.game) == ISLE_GAME_EXECUTION_FAILURE,
                "faulted actor timeline restarted after a source creation failure");
        trace(f, {60});
    }
}

void synchronous_wait_failure_does_not_continue() {
    ActorControlFixture f{}; isle_rng rng{1}; World world;
    auto* object = bind(world, f, rng);
    f.mask = 2;
    f.actor.stateProc = start(world, object, actor_control_invalid_wait, 7, ISLE_GAME_SYNCHRONOUS);
    require(isle_game_tick(world.game) == ISLE_GAME_EXECUTION_FAILURE,
            "source flag wait escaped the existing synchronous C fault fence");
    trace(f, {61});
}
} // namespace

int main() {
    try {
        self_replacement_and_null_state();
        cross_state_replacement_joins_old_stack();
        auxiliary_priority_flags_and_null_path();
        path_replacement_and_recycled_storage();
        random_boundaries_zero_weights_and_application_stream();
        waits_observe_any_mask_on_actual_native_updates();
        binding_and_native_address_reuse();
        foreign_payload_is_not_an_actor_record();
        nullable_lookup_distinguishes_absence_from_invalid_ownership();
        nullable_lookup_accepts_only_joined_identity_held_absence();
        creation_failure_exits_both_c_fences();
        synchronous_wait_failure_does_not_continue();
        std::cout << "native source actor controls passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
