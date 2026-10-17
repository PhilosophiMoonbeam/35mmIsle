// Opt-in private-ROM CPU consumer; never default discovery/CTest. The five
// complete admitted C bodies own proximity and scheduling. The bounded C
// command4 interpreter is explicitly NOT the production Root Player FSM.
#include "player_bump_rom_fixture.h"
#include "player_bump.hpp"
#include "actual_actor_world_hooks.hpp"
#include "actual_player_source.hpp"
#include "actual_audio_driver.hpp"
#include "effect_bank_resources.hpp"
#include "scene_storage.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <source_location>
#include <stdexcept>
#include <string>
#include <thread>

namespace {
using namespace isle;
namespace a = menu::animation;
void require(bool condition, const char* detail) {
    if (!condition) throw std::runtime_error(detail);
}
void native(IsleGameResult result, std::source_location location = std::source_location::current()) {
    if (result != ISLE_GAME_OK)
        throw std::runtime_error(std::string("bump native failure at ") + location.file_name() + ":" +
            std::to_string(location.line()) + " status=" + std::to_string(static_cast<int>(result)));
}
void accepted(actor::OwnerStatus result) { require(bool(result), result.detail); }

// Explicit failure-safe dependency teardown: stop/join detector, retire ALL
// canonical Actor borrowers, collect Actors, collect detector, then Root and
// SourceCtx. No queue cancellation/field assignment/borrow count is simulated.
class Context final {
public:
    Context(scene::Services& s, actor::Owner& actors, environment_sound::Environment& e,
            IslePlayerSource& source, IsleGameCamera* camera)
        : services(s), owner(actors), environment(e), player(source) {
        root = services.object(0x1001, 0);
        require(root, "canonical Player Root allocation failed");
        native(isle_player_source_bind(services.game, root, &player));
        require(services.source_owner_register(root, {}), "canonical Root source registration failed");
        root_registered = true;
        IsleGameModelNodeInit node{};
        node.parent = UINT32_MAX; node.transform.scale = {1,1,1};
        node.matrix_kind = ISLE_GAME_MODEL_CONTEXTUAL_RPY;
        native(isle_game_attach_model(services.game, root, &node, 1, &player.model));
        player.camera = camera;
        const IsleGameModelTransform placement{{0,-9000,0},{0,0,0},{1,1,1}};
        native(isle_game_model_set_placement(services.game, player.model, ISLE_GAME_MODEL_PLACEMENT_RT, &placement));
        require(environment.set_listener(player.model,camera),"environment listener binding failed");
        bump = std::make_unique<isle::player::BumpOwner>(services, player);
        fixture.player = &player; fixture.bump = &bump->source();
        require(services.set_command_handler(root, player_bump_rom_receiver, &fixture) &&
                services.set_command_queued_observer(root, player_bump_rom_queued, &fixture),
                "bounded C Root receiver installation failed");
    }
    ~Context() {
        if (bump) (void)isle_player_bump_BumpDetector_Delete(&bump->source(), 0);
        (void)owner.reset();
        const auto actors = owner.collect_joined();
        const auto detector = bump ? bump->retire() : ISLE_GAME_OK;
        if (!actors || detector != ISLE_GAME_OK) std::terminate();
        bump.reset();
        if (!environment.set_listener(nullptr,nullptr)) std::terminate();
        if (root) services.erase(root);
        if (root_registered && !services.source_owner_release(root)) std::terminate();
    }
    void call(PlayerBumpRomOperation operation) {
        PlayerBumpRomCall call{operation};
        native(isle_player_source_call(&player, player_bump_rom_call, &call));
        native(player.status); native(bump->source().status);
        require(!player.nonwaiting_call_frame, "returning C guard retained a call frame");
        if (operation == PLAYER_BUMP_ROM_INIT) fixture.expected_sender = bump->source().BumpDetector_Object;
    }
    void tick() {
        const auto result = isle_game_tick(services.game);
        // Check only AFTER every C callback has returned/yielded through OM.
        native(result); native(fixture.callback_status); native(player.status);
        native(bump->source().status); require(!services.failed(), services.error());
    }
    void gate_root(bool skip) {
        IsleGameObjectMetadata metadata{}; native(isle_game_object_metadata(services.game, root, &metadata));
        native(isle_game_object_set_flags(services.game, root, skip ? metadata.flags | 0x40u : metadata.flags & ~0x40u));
    }
    scene::Services& services;
    actor::Owner& owner;
    environment_sound::Environment& environment;
    IslePlayerSource& player;
    IsleGameObject* root = nullptr;
    std::unique_ptr<isle::player::BumpOwner> bump;
    PlayerBumpRomFixture fixture{};
private:
    bool root_registered = false;
};

class Consumer final {
public:
    Consumer(Context& c, world::World& w, const beach::ActorResources& resources)
        : context(c), world(w) {
        for (std::size_t i = 0; i < resources.definitions().size(); ++i)
            if (resources.definitions()[i].actor_id == 129 &&
                resources.definitions()[i].factory_identity == 0x802cac48u) definition = i;
        require(definition != beach::actor_no_index, "admitted real Beach Magikarp factory absent");
    }
    actor::SpawnResult spawn(float gap) {
        world::Spawn descriptor{}; descriptor.id = 129; descriptor.scale = {1,1,1};
        descriptor.path = world::no_index;
        auto* block = &world.blocks()[0];
        const auto result = context.owner.spawn({next_id++, definition, block, block, &descriptor});
        accepted(result.status);
        auto* source = context.owner.source(result.instance);
        require(source && !source->transitionGraph && source->collisionRadius > 0 &&
                std::isfinite(source->collisionRadius), "actual factory radius/NULL graph not admitted");
        native(isle_actor_source_pause_object(source, 1));
        source->tangible = 1; source->flags |= 0x20u;
        place(result, gap);
        return result;
    }
    void place(const actor::SpawnResult& actor, float gap) {
        auto* source = context.owner.source(actor.instance);
        require(source, "actual Actor source lost before geometry publication");
        const IsleGameModelPlacement* old = nullptr;
        native(isle_game_model_placement(context.services.game, context.owner.native_model(actor.instance), &old));
        auto transform = old->transform;
        const float distance = source->collisionRadius + gap;
        transform.position = {distance,-9000,0};
        accepted(context.owner.set_placement(actor.instance, transform));
        // Actual owner distance leaf reads both published native placements and
        // writes canonical playerDist. No handwritten distance/clock is used.
        native(isle_actor_owner_distance(source));
        require(source->playerDist == distance && source->playerDist - source->collisionRadius == gap,
                "actual asset radius/owner publication cannot express exact requested gap");
    }
    void expect(int level, bool command) {
        const auto queued = context.fixture.queued_count;
        context.tick();
        require(context.bump->source().BumpDetector_ProximityLevel == level,
                "original C strict threshold/proximity update changed");
        require(context.fixture.queued_count == queued + (command ? 1u : 0u),
                "original Update emitted a missing/extra source command4");
        if (command) {
            const auto& observation = context.fixture.queued[queued];
            require(observation.command == 4 && observation.proximity == -1 &&
                    observation.receiver == context.root && observation.sender == context.fixture.expected_sender,
                    "actual C callback command4 sender/proximity observation differs");
        }
    }
    void thresholds_and_admission() {
        auto& bump = context.bump->source();
        require(!bump.BumpDetector_Object && !bump.BumpDetector_IsEnabled && !bump.BumpDetector_ProximityLevel &&
                bump.BumpDetector_Thresholds[0] == 90 && bump.BumpDetector_Thresholds[1] == 60 &&
                bump.BumpDetector_Thresholds[2] == 30 && bump.BumpDetector_Thresholds[3] == 5,
                "sole bound source defaults differ from original C globals");
        context.call(PLAYER_BUMP_ROM_INIT); native(player_bump_rom_inspect(&bump, 0));
        require(bump.BumpDetector_IsEnabled && bump.BumpDetector_ProximityLevel == 0, "original Init did not publish enabled/zero state");
        expect(0, false); // Actual empty native link3.
        actors[0] = spawn(100); actors[1] = spawn(150); actors[2] = spawn(200);
        context.fixture.borrower = context.owner.source(actors[2].instance);
        const float admitted_radius = context.owner.source(actors[0].instance)->collisionRadius;
        const std::array<float,12> gaps{90.25f,90,89.75f,60.25f,60,59.75f,30.25f,30,29.75f,5.25f,5,4.75f};
        const std::array<int,12> levels{0,1,1,1,2,2,2,3,3,3,-1,-1};
        for (std::size_t i = 0; i < gaps.size(); ++i) {
            place(actors[0], gaps[i]); expect(levels[i], levels[i] == -1);
            std::cout << "bump_source_gap=" << gaps[i] << " proximity=" << bump.BumpDetector_ProximityLevel
                      << " radius=" << admitted_radius << " playerDist="
                      << context.owner.source(actors[0].instance)->playerDist << '\n';
        }
        // Drain command4 through the actual default Root Update; move geometry
        // away first so this tick cannot create a replacement queue credit.
        place(actors[0], 100); expect(0, false);
        auto* first = context.owner.source(actors[0].instance);
        auto* second = context.owner.source(actors[1].instance);
        place(actors[0], 4); place(actors[1], 30);
        first->tangible = 0; expect(3, false);
        first->tangible = 1; first->flags &= ~0x20u; expect(3, false);
        first->flags |= 0x20u; expect(-1, true);
        first->tangible = 0; second->flags &= ~0x20u; expect(0, false);
        // Keep geometry fixed, vary the actual radius field only. These are
        // canonical source inputs on a real model, not a synthetic collision.
        first->tangible = 1; place(actors[0], 60.25f); expect(1, false);
        first->collisionRadius += 0.5f; expect(2, false);
        first->collisionRadius -= 0.5f;
        second->flags |= 0x20u; place(actors[1], 150); place(actors[0], 100); expect(0, false);
        std::cout << "bump_admission=actual_tangible_flag20_minimum_radius_owner_distance native_link=3\n";
    }
    void pause_and_default_dispatch() {
        auto& bump = context.bump->source();
        place(actors[0], 30); expect(3, false);
        context.call(PLAYER_BUMP_ROM_DISABLE); native(player_bump_rom_inspect(&bump, 1));
        place(actors[0], 100); const auto queued = context.fixture.queued_count;
        context.tick(); context.tick();
        require(!bump.BumpDetector_IsEnabled && bump.BumpDetector_ProximityLevel == 3 &&
                context.fixture.queued_count == queued, "Disable failed to preserve suspended original proximity");
        context.call(PLAYER_BUMP_ROM_ENABLE); native(player_bump_rom_inspect(&bump, 0)); expect(0, false);
        // BumpOwner must not replace Services' default dispatcher: genuine -2
        // and -3 commands gate/resume the real priority9 C continuation. They
        // intentionally do NOT modify the original IsEnabled scalar.
        require(context.services.send_command(bump.BumpDetector_Object, -2), "detector default -2 enqueue failed");
        place(actors[0], 30); context.tick(); native(player_bump_rom_inspect(&bump, 1));
        require(bump.BumpDetector_IsEnabled && bump.BumpDetector_ProximityLevel == 0,
                "default -2 did not suspend independently of source enabled flag");
        require(context.services.send_command(bump.BumpDetector_Object, -3), "detector default -3 enqueue failed");
        expect(3, false); native(player_bump_rom_inspect(&bump, 0));
    }
    void errors() {
        auto& source = context.bump->source();
        IslePlayerSource* wrong = &context.player;
        require(isle_player_source_get(context.services.game, actors[0].object, &wrong) == ISLE_GAME_INVALID_STATE && !wrong,
                "real Actor payload aliased canonical Player Root");
        wrong = &context.player;
        require(isle_player_source_get(context.services.game, source.BumpDetector_Object, &wrong) == ISLE_GAME_INVALID_STATE && !wrong,
                "actual NONE detector aliased canonical Player Root");
        require(context.bump->command(actors[0].object, 4, source.BumpDetector_Object) == ISLE_GAME_INVALID_STATE &&
                context.bump->command(context.root, 4, actors[0].object) == ISLE_GAME_INVALID_STATE &&
                context.bump->erase(actors[0].object) == ISLE_GAME_INVALID_ARGUMENT,
                "real foreign receiver/sender/deletion admission did not reject");
        IslePlayerSource inactive{};
        native(isle_player_source_prepare(context.services.game, &inactive, &context.services.rng, &inactive, 10));
        PlayerBumpRomCall call{PLAYER_BUMP_ROM_INIT};
        require(isle_player_source_activate(&inactive) == ISLE_GAME_INVALID_STATE &&
                isle_player_source_call(&inactive, player_bump_rom_call, &call) == ISLE_GAME_INVALID_STATE &&
                inactive.bump == nullptr, "inactive second context entered the sole active Player");
        IslePlayerBumpSource foreign_record{};
        IsleGameResult foreign_bind = ISLE_GAME_OK, foreign_get = ISLE_GAME_OK;
        std::thread foreign([&] {
            IslePlayerSource* out = &context.player;
            foreign_get = isle_player_source_get(context.services.game, context.root, &out);
            foreign_bind = isle_player_bump_source_bind(&context.player, &foreign_record, &foreign_record);
            if (out) foreign_get = ISLE_GAME_OK;
        });
        foreign.join();
        require(foreign_get == ISLE_GAME_WRONG_OWNER && foreign_bind == ISLE_GAME_WRONG_OWNER &&
                !foreign_record.owner && !foreign_record.player && context.player.bump == &source,
                "real foreign thread acquired/mutated Player/bump context");
        native(isle_game_status(context.services.game)); native(source.status); native(context.player.status);
    }
    void fifo_delete_and_reinit() {
        auto& bump = context.bump->source();
        // Close the earlier delivered-command field via real Actor retirement,
        // not a harness NULL store. Fresh actual actors distinguish the queued-
        // ONLY hold below from the stored-field-ONLY hold after delivery.
        context.call(PLAYER_BUMP_ROM_DISABLE);
        accepted(context.owner.reset()); accepted(context.owner.collect_joined());
        actors[0] = spawn(100); actors[1] = spawn(150); actors[2] = spawn(200);
        context.fixture.borrower = context.owner.source(actors[2].instance);
        const auto initial_borrows = context.services.source_owner_borrows(bump.BumpDetector_Object);
        require(!initial_borrows.queued && !initial_borrows.fields,
                "real earlier Actor retirement left detector borrowers outstanding");
        context.call(PLAYER_BUMP_ROM_ENABLE);
        place(actors[0], 4); context.gate_root(true);
        const auto delivered = context.fixture.delivered_count;
        const auto queued = context.fixture.queued_count;
        require(context.services.send_command(context.root, 100), "FIFO leading marker enqueue failed");
        expect(-1, true);
        require(context.services.send_command(context.root, 101), "FIFO trailing marker enqueue failed");
        expect(-1, true);
        require(context.fixture.delivered_count == delivered && context.fixture.queued_count == queued + 2,
                "held real receiver consumed or fabricated the FIFO before dispatch");
        auto* identity = bump.BumpDetector_Object;
        const auto queued_only = context.services.source_owner_borrows(identity);
        require(queued_only.queued == 2 && !queued_only.fields &&
                !context.fixture.borrower->interactionTarget,
                "queued-only source4 lifetime case already had a stored sender borrow");
        // Run original Delete on its own actual object, while Update is really
        // suspended. The C witness can observe the request before native join.
        require(context.services.process(identity, player_bump_rom_delete_self, &context.fixture,
                ISLE_GAME_SYNCHRONOUS, 10), "self-delete C witness allocation failed");
        context.tick();
        require(context.fixture.self_delete_returned && context.fixture.self_delete_handle_preserved &&
                context.fixture.self_delete_enabled && context.fixture.self_delete_proximity == -1 &&
                !bump.BumpDetector_Object && bump.BumpDetector_IsEnabled && bump.BumpDetector_ProximityLevel == -1 &&
                context.services.source_owner_joined(identity),
                "original Delete cleared early, reset source state or failed actual continuation join");
        require(context.bump->retire() == ISLE_GAME_INVALID_STATE && !context.services.source_owner_collectable(identity),
                "queued sender was collected before actual FIFO delivery");
        context.gate_root(false); context.tick();
        require(context.fixture.delivered_count == delivered + 4,
                "actual bounded C receiver failed to drain the exact FIFO");
        const std::array<int,4> expected{100,4,101,4};
        for (std::size_t i = 0; i < expected.size(); ++i) {
            const auto& event = context.fixture.delivered[delivered+i];
            require(event.command == expected[i] && event.receiver == context.root &&
                    (event.command != 4 || event.sender == identity), "real command4 FIFO/source sender changed");
        }
        require(context.fixture.borrower->interactionTarget == identity &&
                context.services.source_owner_borrows(identity).queued == 0 &&
                context.services.source_owner_borrows(identity).fields != 0 &&
                context.bump->retire() == ISLE_GAME_INVALID_STATE,
                "delivery fabricated closure of the actual canonical Actor sender field");
        // No NULL assignment and no fake release: retire all actual Actor
        // parties, then collect only after Services has closed their fields.
        accepted(context.owner.reset()); accepted(context.owner.collect_joined());
        context.fixture.borrower = nullptr;
        require(context.services.source_owner_collectable(identity), "real Actor retirement did not close stored sender ownership");
        native(context.bump->retire());
        require(!context.services.source_owner_registered(identity) && bump.BumpDetector_IsEnabled &&
                bump.BumpDetector_ProximityLevel == -1, "identity collection reset original retained source globals");
        context.call(PLAYER_BUMP_ROM_INIT); native(player_bump_rom_inspect(&bump, 0));
        require(context.player.bump == &bump && bump.BumpDetector_ProximityLevel == 0 && bump.BumpDetector_IsEnabled,
                "same canonical source record did not admit original reinitialization");
        expect(0, false);
        context.call(PLAYER_BUMP_ROM_DISABLE); context.call(PLAYER_BUMP_ROM_DELETE);
        require(!bump.BumpDetector_Object && !bump.BumpDetector_IsEnabled && bump.BumpDetector_ProximityLevel == 0,
                "disabled original Delete reset retained state");
        context.call(PLAYER_BUMP_ROM_DELETE); native(context.bump->retire());
        std::cout << "bump_FIFO=100,4,101,4 actual_command4_enqueues=" << context.fixture.queued_count
                  << " actual_C_deliveries=" << context.fixture.delivered_count
                  << " joined_delete_queued_then_canonical_Actor_field_retirement_same_record_reinit=passed\n";
    }
private:
    Context& context; world::World& world;
    std::size_t definition = beach::actor_no_index;
    std::uint32_t next_id = 0x7100;
    std::array<actor::SpawnResult,3> actors{};
};

void strict_link3(Context& context) {
    auto* foreign = context.services.object(0x7200, 3);
    require(foreign, "live nonActor link3 admission witness allocation failed");
    context.call(PLAYER_BUMP_ROM_INIT);
    const auto result = isle_game_tick(context.services.game);
    require(result == ISLE_GAME_EXECUTION_FAILURE &&
            isle_game_status(context.services.game) == ISLE_GAME_EXECUTION_FAILURE &&
            context.bump->source().status == ISLE_GAME_INVALID_STATE &&
            context.player.status == ISLE_GAME_INVALID_STATE && context.fixture.queued_count == 0,
            "original native link3 traversal skipped/reinterpreted a live NONE payload or lost exact source status");
    // Do not clear terminal native/source status. Context's actual retirement
    // path still joins all parties, closes queues and releases identities.
    std::cout << "bump_strict_live_link3_error source_status=" << static_cast<int>(context.player.status)
              << " native_status=" << static_cast<int>(result) << " diagnostic_not_used_as_status=true\n";
}

void run(rom::Snapshot& snapshot, const std::filesystem::path& output) {
    const auto profile = output / "profile";
    require(std::filesystem::create_directory(profile), "new private profile creation failed");
    const world::Resources world_resources(snapshot); const auto& setup = world_resources.setup();
    const beach::ActorResources resources(snapshot, {setup.fog_rgb,setup.fog_min,setup.fog_max});
    const environment_sound::BeachResources sounds(snapshot);
    const menu::EffectResources effect_data(snapshot);
    const std::array banks{&effect_data.bank(menu::EffectGroup::common), &effect_data.bank(menu::EffectGroup::beach)};
    auto hydrated = game_audio::hydrate(snapshot); require(bool(hydrated), "admitted audio hydration failed");
    std::array<audio::effects::Section,8> sections; audio::effects::Config effects;
    require(game_audio::native_startup_effects(hydrated.resources, sections, effects) == audio::Error::none,
            "admitted startup effects configuration failed");
    private_audio::Capture capture; private_audio::Execution execution{snapshot,nullptr,{}};
    capture.samples.resize(8192 * audio::AudioService::max_frames * 2);
    auto made = game_audio::Session::create(std::move(hydrated.resources), effects, capture.output(), execution.executor());
    require(bool(made), "actual shared audio Session creation failed"); auto& session = *made.session;
    std::uint64_t retrace = 0; unsigned tasks = 0;
    const auto pump = [&] {
        ++retrace;
        require(session.retrace(retrace,retrace*1000000000ull/60) == audio::Error::none, "actual audio retrace failed");
        require(session.submit() == audio::Error::none, "actual PCM acceptance failed");
        audio::TaskView task; require(session.build(task) == audio::Error::none, "actual audio task build failed");
        if (task.token.serial) {
            private_audio::execute(session,execution,task); ++tasks;
            require(session.submit() == audio::Error::none, "completed RSP PCM acceptance failed");
        }
    };
    game::State state(profile); require(state.initialize(1), "private profile initialization failed");
    game_input::Service input;
    a::SlotCapacity maximum{};
    for (const auto& model : resources.models()) {
        const auto needed = a::slot_capacity(model);
        maximum.nodes = std::max(maximum.nodes,needed.nodes);
        maximum.material_attachments = std::max(maximum.material_attachments,needed.material_attachments);
    }
    const std::array capacities{maximum,maximum,maximum};
    pump(); pump();
    // Separate sequential native games: never violate NativeGame's one-active
    // invariant to test foreign ownership or recover a terminal source fault.
    for (unsigned mode = 0; mode < 2; ++mode) {
        scene::Storage storage(32*1024*1024); native(storage.create());
        {
            scene::Services services(storage.game(),storage.arena(),{{4,6,0,1,2},{}},input,session,state);
            require(!services.failed(),services.error());
            private_player::Source player_source(services);
            auto* camera_object = services.object(1,0); require(camera_object, "native camera allocation failed");
            IsleGameCamera* camera = nullptr; native(isle_game_attach_camera(services.game,camera_object,&camera));
            const IsleGameLookAt view{{0,-9000,0},{1,-9000,0},{0,1,0}};
            native(isle_game_camera_set_look_at(services.game,camera,&view));
            environment_sound::Environment environment(services,storage.arena(),sounds);
            require(environment.initialize(nullptr,camera),"environment source initialization failed");
            focus::Detector detector; private_actor::WorldHooks hooks{services,camera};
            world::World world(services,world_resources,hooks.callbacks());
            {
                actor::Owner owner(services,resources,world,environment,detector,player_source.value,capacities,banks,{32768,4096});
                accepted(owner.publish_effects());
                hooks.owner = &owner; private_actor::WorldLifetime lifetime{world,hooks};
                player_source.value.D_80382D44_523154 = 1;
                require(!world.current(), "standalone source-default-ground context unexpectedly entered a course block");
                Context context(services,owner,environment,player_source.value,camera);
                if (!mode) {
                    Consumer consumer(context,world,resources);
                    consumer.thresholds_and_admission(); consumer.pause_and_default_dispatch();
                    consumer.errors(); consumer.fifo_delete_and_reinit();
                } else strict_link3(context);
            }
            require(world.retire(),"Player bump World retirement failed"); environment.cleanup();
        }
        IsleGameCounts counts{}; native(isle_game_counts(storage.game(),&counts));
        require(!counts.objects && !counts.processes && !counts.models && !counts.model_nodes &&
                !counts.cameras && !counts.matrices && !counts.sprites,
                "actual native shutdown retained an owner/process/model/camera borrower");
        native(storage.destroy());
    }
    require(session.stop() == audio::Error::none, "actual audio stop failed");
    unsigned stop_retraces = 0;
    for (; stop_retraces < 240 && session.state() != game_audio::Session::State::draining; ++stop_retraces) pump();
    require(stop_retraces && session.state() == game_audio::Session::State::draining && session.service().voices_retired(),
            "actual audio source graph did not retire within fixed240 retraces");
    require(session.poll_drain() == audio::Error::none && session.poll_drain() == audio::Error::none &&
            session.state() == game_audio::Session::State::drained && tasks && capture.used && capture.finishes == 1,
            "actual RSP/PCM acceptance and terminal audio drain failed");
    std::cout << "original_bump_5C_private_native_CPU=passed geometry=real_Beach_Magikarp_129_factory802cac48"
              << " receiver=bounded_C_harness_not_PlayerFSM audio_tasks=" << tasks
              << " Root106C_39callbacks_fullgame_GPU_Windows11=not_claimed\n";
}
} // namespace
int main(int argc, char** argv) {
    try {
        require(argc == 3, "usage: player-bump-rom-smoke PRIVATE_ROM NEW_OUTPUT_DIRECTORY");
        const std::filesystem::path output(argv[2]);
        require(output.is_absolute() && output.has_filename() && !std::filesystem::exists(output),
                "output must be an absolute nonexistent private directory");
        auto snapshot = isle::rom::read_snapshot(argv[1]);
        require(std::filesystem::create_directory(output), "new private output creation failed");
        run(snapshot,output); return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "player-bump-rom-smoke: " << error.what() << '\n'; return EXIT_FAILURE;
    }
}
