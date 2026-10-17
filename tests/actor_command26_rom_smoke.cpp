#include "actual_actor_world_hooks.hpp"
// Explicit opt-in actual-ROM CPU/private-native-API consumer; never CTest or
// default discovery. No graphics borrower/executor is created. This proves the
// command26 queue/stored-field identity and NULL-graph mechanism, not complete
// coverage, GPU lifetime, fullgame, Original or Windows11 execution.
// Authority: GAME df8815e75f3d79618466e01e0827c6165b93d11c,
// pokemon.c:1896-1910,1968-1995,2055-2088; common1003 4FE330.c:63-109.
// Fixtures reuse actor_owner_rom_smoke and the actual_audio_driver RSP/PCM path.
#include "actor_owner.hpp"
#include "actual_audio_driver.hpp"
#include "effect_bank_resources.hpp"
#include "scene_storage.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace {
using namespace isle;
namespace a = menu::animation;
void require(bool value,const char* text) { if (!value) throw std::runtime_error(text); }
void native(IsleGameResult value) { require(value==ISLE_GAME_OK,"native command26 consumer operation failed"); }
void accepted(actor::OwnerStatus value) { require(bool(value),value.detail); }
IsleGameCounts counts(scene::Services& services) {
    IsleGameCounts result{}; native(isle_game_counts(services.game,&result)); return result;
}
bool same_counts(IsleGameCounts left,IsleGameCounts right) {
    return left.objects==right.objects&&left.processes==right.processes&&left.sprites==right.sprites&&
        left.cameras==right.cameras&&left.matrices==right.matrices&&left.models==right.models&&
        left.model_nodes==right.model_nodes;
}
struct State {
    IsleGameProcess* state=nullptr;
    IsleGameProcess* path=nullptr;
    IsleGameSourceCallback function=nullptr;
    const IsleActorInteractionGraph* graph=nullptr;
    IsleGameObject* apple=nullptr;
    IsleGameObject* target=nullptr;
    float distance=0;
    int tangible=0,signal=0;
    State(scene::Services& services,const IsleActorSource& source)
        : state(source.stateProc),path(source.pathProc),graph(source.transitionGraph),apple(source.apple),
          target(source.interactionTarget),distance(source.interactionDist),tangible(source.tangible),
          signal(source.miscVars[0].field1) {
        require(state,"actual source state process absent");
        native(isle_game_process_source_function(services.game,state,&function));
        require(function,"state process lost its actual typed source function");
    }
    void unchanged(scene::Services& services,const IsleActorSource& source) const {
        IsleGameSourceCallback current=nullptr;
        native(isle_game_process_source_function(services.game,source.stateProc,&current));
        require(source.stateProc==state&&source.pathProc==path&&current==function&&
                source.transitionGraph==graph&&source.apple==apple&&source.interactionTarget==target&&
                source.interactionDist==distance&&source.tangible==tangible&&source.miscVars[0].field1==signal,
                "NULL-graph command26 changed actual state, target, apple or interaction data");
    }
};
struct Consumer {
    scene::Services& services;
    actor::Owner& owner;
    world::World& world;
    const beach::ActorResources& resources;
    const IsleGameCounts baseline;
    std::size_t definition=beach::actor_no_index;
    std::uint32_t next_id=10000;
    unsigned reuse_proofs=0;
    const IsleActorInteractionGraph invalid_graph{};
    Consumer(scene::Services& s,actor::Owner& o,world::World& w,const beach::ActorResources& r)
        : services(s),owner(o),world(w),resources(r),baseline(counts(s)) {
        const auto definitions=resources.definitions();
        for (std::size_t i=0;i<definitions.size();++i) if (definitions[i].actor_id==1003) definition=i;
        require(definition!=beach::actor_no_index,"actual admitted common1003 definition absent");
    }
    actor::SpawnResult attempt() {
        world::Spawn descriptor; descriptor.id=1003; descriptor.behavior=1;
        descriptor.position={12,17,-23};
        return owner.spawn({next_id++,definition,&world.blocks()[0],&world.blocks()[0],&descriptor});
    }
    actor::SpawnResult spawn() {
        const auto result=attempt(); accepted(result.status);
        require(result.object&&owner.registered(result.instance)&&owner.source(result.instance)&&
                !owner.source(result.instance)->transitionGraph,"real factory/source/NULL-graph publication absent");
        // Queue/lifetime cases isolate the genuine Update phase, not a manual
        // handler call. The progress case resumes via the actual FIFO -3.
        native(isle_actor_source_pause_object(owner.source(result.instance),1));
        return result;
    }
    void send(IsleGameObject* receiver,int command,IsleGameObject* sender) {
        require(services.send_command(receiver,command,sender),"actual Services FIFO enqueue failed");
    }
    void dispatch(IsleGameObject* receiver) {
        // Own GAME om.c1835-1842: native GObj flag40 suppresses fnUpdate only.
        // Preserve the old single-receiver lifetime scenarios using that real
        // gate, while a genuine whole-game tick owns current-object metadata.
        std::array<IsleGameObject*,4> skipped{};
        std::array<std::uint32_t,4> previous_flags{};
        std::size_t count=0;
        IsleGameObject* previous=nullptr;
        for (;;) {
            IsleGameObject* object=nullptr;
            IsleActorSource* source=nullptr;
            IsleGameModel* model=nullptr;
            native(isle_actor_source_link3_next(services.game,previous,&object,&source,&model));
            if (!object) break;
            previous=object;
            if (object==receiver) continue;
            require(count<skipped.size(),"actual command26 fixture exceeds its declared four native slots");
            IsleGameObjectMetadata metadata{};
            native(isle_game_object_metadata(services.game,object,&metadata));
            skipped[count]=object; previous_flags[count++]=metadata.flags;
            native(isle_game_object_set_flags(services.game,object,metadata.flags|0x40u));
        }
        tick();
        for (std::size_t i=0;i<count;++i)
            native(isle_game_object_set_flags(services.game,skipped[i],previous_flags[i]));
    }
    void tick() {
        // No renderer/preparation/readback borrower exists in this CPU consumer.
        // Deliberately do not collect here: held identities are the test oracle.
        native(isle_game_tick(services.game)); require(!services.failed(),services.error());
    }
    void native_delete(const actor::SpawnResult& actor,IsleGameResult expected=ISLE_GAME_OK) {
        // Do NOT use Owner::cleanup/request_cleanup: they can produce21/22 and
        // hide a missing26 credit. This is actual joined native deletion only.
        require(isle_game_delete_object(services.game,actor.object)==expected,
                "native deletion did not preserve its expected terminal scene status");
        const auto membership=owner.membership(actor.instance);
        require(membership.retiring&&membership.joined&&!membership.logical&&!membership.registered&&
                !owner.source(actor.instance)&&!owner.object(actor.instance)&&!owner.native_model(actor.instance),
                "native deletion did not join workers/remove source/model/live membership");
        IsleActorSource* expired=nullptr;
        require(isle_actor_source_get(services.game,actor.object,&expired)!=ISLE_GAME_OK&&!expired,
                "joined held sender still exposed a live source record");
    }
    void held(const actor::SpawnResult& sender) {
        const auto result=owner.collect_joined();
        require(!result&&result.error==actor::OwnerError::lifetime&&owner.membership(sender.instance).joined,
                "queued26 sender was collected before actual delivery/receiver cancellation");
    }
    void incarnation(const actor::SpawnResult& old,const actor::SpawnResult& replacement,const char* label) {
        require(replacement.instance.pool==old.instance.pool&&replacement.instance.slot==old.instance.slot&&
                replacement.instance.incarnation!=old.instance.incarnation&&replacement.instance!=old.instance&&
                !owner.source(old.instance)&&!owner.animation().model(old.instance)&&
                !owner.set_local(old.instance,0,{}),"released physical actor slot admitted its stale full incarnation");
        ++reuse_proofs;
        std::cout<<"command26_reuse="<<label<<" slot="<<old.instance.slot
                 <<" old_incarnation="<<old.instance.incarnation<<" new_incarnation="<<replacement.instance.incarnation
                 <<" old_address="<<static_cast<const void*>(old.object)
                 <<" new_address="<<static_cast<const void*>(replacement.object)<<'\n';
    }
    void reset() {
        accepted(owner.reset()); accepted(owner.collect_joined());
        require(same_counts(counts(services),baseline),"command26 case leaked native object/process/model/matrix ownership");
    }
    void delivery_and_allocator_pressure() {
        const auto sender=spawn(),receiver=spawn();
        send(receiver.object,26,sender.object);
        const auto before=counts(services);
        native_delete(sender);
        const auto after=counts(services);
        require(after.objects+1==before.objects&&after.processes+3==before.processes&&
                after.models+1==before.models&&after.model_nodes<before.model_nodes,
                "identity retention incorrectly retained live source workers/model counts");
        held(sender);
        const auto filler_a=spawn(),filler_b=spawn();
        require(filler_a.instance.slot!=sender.instance.slot&&filler_b.instance.slot!=sender.instance.slot&&
                filler_a.object!=sender.object&&filler_b.object!=sender.object,
                "Owner reused the queued sender's physical slot or native identity");
        const auto full=attempt();
        require(!full&&full.status.error==actor::OwnerError::capacity&&!services.failed(),
                "held26 sender did not occupy exactly its real caller slot capacity");
        std::array<IsleGameObject*,8> unrelated{};
        for (auto& object:unrelated) {
            object=services.object(next_id++);
            require(object&&object!=sender.object,"unrelated native allocator reused a held26 sender address");
        }
        // Adopt matching canonical identities AFTER actual sender retirement.
        //26 must not clear either or revive the sender's native payload.
        auto* record=owner.source(receiver.instance);
        require(services.source_reference_assign(receiver.object,&record->apple,sender.object)&&
                services.source_reference_assign(receiver.object,&record->interactionTarget,sender.object),"adopt held sender fields");
        record->interactionDist=37.0f;
        const State state(services,*record); const auto dispatched_counts=counts(services);
        dispatch(receiver.object); state.unchanged(services,*record);
        require(same_counts(counts(services),dispatched_counts),"NULL-graph26 changed native ownership on delivery");
        held(sender); // Delivered queue credit is gone; canonical fields still own it.
        require(services.source_reference_assign(receiver.object,&record->apple,nullptr)&&
                services.source_reference_assign(receiver.object,&record->interactionTarget,nullptr),"release held sender fields");
        accepted(owner.collect_joined());
        const auto replacement=spawn();
        incarnation(sender,replacement,"delivery");
        require(replacement.object==sender.object,"last delivered26 credit did not return native identity to its real free list");
        for (auto* object:unrelated) services.erase(object);
        reset();
    }
    void multiple_senders_and_receivers() {
        const auto sender_a=spawn(),sender_b=spawn(),receiver_a=spawn(),receiver_b=spawn();
        for (unsigned n=0;n<2;++n) {
            send(receiver_a.object,26,sender_a.object); send(receiver_a.object,26,sender_b.object);
            send(receiver_b.object,26,sender_a.object);
        }
        send(receiver_b.object,26,sender_b.object);
        native_delete(sender_a); native_delete(sender_b); held(sender_a);
        const auto before=counts(services);
        dispatch(receiver_b.object); // Releases two A credits and one B credit.
        require(same_counts(counts(services),before),"multi-receiver26 delivery changed native counts");
        held(sender_a); // Receiver A still owns two credits from EACH sender.
        const auto unavailable=attempt();
        require(!unavailable&&unavailable.status.error==actor::OwnerError::capacity,
                "partial receiver delivery released a still-borrowed sender slot");
        native_delete(receiver_a); // Cancels its four actual queued borrows, no fake dispatch.
        accepted(owner.collect_joined());
        const auto replacement_a=spawn(),replacement_b=spawn(),replacement_receiver=spawn();
        incarnation(sender_a,replacement_a,"multi-sender-A");
        incarnation(sender_b,replacement_b,"multi-sender-B");
        incarnation(receiver_a,replacement_receiver,"multi-receiver-cancel");
        const std::array reclaimed{replacement_a.object,replacement_b.object,replacement_receiver.object};
        for (auto* expected:{sender_a.object,sender_b.object,receiver_a.object})
            require(std::count(reclaimed.begin(),reclaimed.end(),expected)==1,
                    "delivery/cancellation did not reclaim each native sender/receiver identity exactly once");
        reset();
    }
    void fifo_delete_cancels_tail() {
        const auto sender=spawn(),receiver=spawn();
        // NonNULL is an explicit fail-closed graph, NOT a provider. If canceled
        //26 commands were fake-delivered, the real Owner handler would fail.
        owner.source(receiver.instance)->transitionGraph=&invalid_graph;
        send(receiver.object,-1,sender.object);
        send(receiver.object,26,sender.object); send(receiver.object,26,sender.object);
        native_delete(sender); held(sender);
        dispatch(receiver.object);
        require(owner.membership(receiver.instance).joined&&!services.failed(),
                "real FIFO -1 failed to discard its26 tail without invoking the unsupported graph");
        accepted(owner.collect_joined());
        const auto replacement_sender=spawn(),replacement_receiver=spawn();
        incarnation(sender,replacement_sender,"FIFO-delete-sender");
        incarnation(receiver,replacement_receiver,"FIFO-delete-receiver");
        require((replacement_sender.object==sender.object&&replacement_receiver.object==receiver.object)||
                (replacement_sender.object==receiver.object&&replacement_receiver.object==sender.object),
                "FIFO receiver cancellation failed actual native address reclamation");
        reset();
    }
    void live_consumption_and_legacy_clearing() {
        const auto sender=spawn(),receiver=spawn();
        auto* record=owner.source(receiver.instance);
        require(services.source_reference_assign(receiver.object,&record->apple,sender.object)&&
                services.source_reference_assign(receiver.object,&record->interactionTarget,sender.object),"adopt live sender fields");
        record->interactionDist=19.0f;
        const State state(services,*record); const auto before=counts(services);
        send(receiver.object,26,sender.object); dispatch(receiver.object);
        state.unchanged(services,*record);
        require(same_counts(counts(services),before),"live sender26 consumed native ownership");
        send(receiver.object,21,sender.object); send(receiver.object,26,sender.object);
        send(receiver.object,22,sender.object); dispatch(receiver.object);
        require(!record->apple&&!record->interactionTarget&&record->stateProc==state.state&&
                record->pathProc==state.path&&record->interactionDist==19.0f,
                "26 extension broke actual21/22 identity clearing or NULL-graph state invariance");
        native_delete(sender);
        accepted(owner.collect_joined()); // Every credit was consumed while sender was live.
        const auto replacement=spawn(); incarnation(sender,replacement,"live-consumed");
        require(replacement.object==sender.object,"live26 consumption left a phantom sender identity hold");
        reset();
    }
    void actual_fifo_pause_tail() {
        const auto sender=spawn(),receiver=spawn();
        auto* record=owner.source(receiver.instance);
        send(receiver.object,-3,sender.object);
        for (unsigned n=0;n<4&&record->tangible;++n) tick();
        require(!record->tangible&&!record->pathProc,"real behavior1 did not reach hidden source wait");
        accepted(owner.trigger_common1003(receiver.instance,{100,200,300})); tick();
        require(record->tangible&&record->pathProc&&!record->transitionGraph,
                "actual source trigger did not start a real priority5 path/NULL graph");
        send(receiver.object,-2,sender.object); tick();
        require(services.source_reference_assign(receiver.object,&record->apple,sender.object)&&
                services.source_reference_assign(receiver.object,&record->interactionTarget,sender.object),"adopt paused sender fields");
        record->counter=2; record->processFlags&=~std::uint32_t(POKEMON_PROCESS_WAIT_ENDED);
        const State state(services,*record); const auto before=counts(services);
        const auto time=owner.animation().model(receiver.instance).animation_time;
        const auto y=owner.animation().model(receiver.instance).nodes[0].transform.position.y;
        owner.course().execution_enabled=false;
        tick();
        require(record->counter==1&&owner.animation().model(receiver.instance).animation_time==time&&
                owner.animation().model(receiver.instance).nodes[0].transform.position.y==y,
                "execution predicate alone resumed/changed suspended native source processes");
        // -3 resumes, THEN26 must run the ordinary execution-disabled pause tail.
        // Enqueue does not execute any of this synchronously.
        send(receiver.object,-3,sender.object); send(receiver.object,26,sender.object);
        require(record->counter==1&&owner.animation().model(receiver.instance).animation_time==time,
                "26 enqueue ran source Update/processes before actual next-update dispatch");
        tick(); state.unchanged(services,*record);
        require(record->counter==0&&(record->processFlags&POKEMON_PROCESS_WAIT_ENDED)&&
                owner.animation().model(receiver.instance).animation_time==time&&
                owner.animation().model(receiver.instance).nodes[0].transform.position.y==y,
                "FIFO -3,26 skipped source Update bookkeeping or failed the ordinary26 pause tail");
        // Reverse the ACTUAL FIFO order:26 pauses, then -3 resumes. Both the
        // animation clock and the genuine C path must now progress that tick.
        send(receiver.object,26,sender.object); send(receiver.object,-3,sender.object);
        tick(); state.unchanged(services,*record);
        const auto progressed=owner.animation().model(receiver.instance).animation_time;
        const auto progressed_y=owner.animation().model(receiver.instance).nodes[0].transform.position.y;
        require(progressed>time&&progressed_y==y-0.2f&&same_counts(counts(services),before),
                "FIFO26,-3 did not resume actual animation3/path5 after26, or changed state/ownership");
        // With no ordinary command, execution_disabled itself must NOT pause.
        tick();
        require(owner.animation().model(receiver.instance).animation_time>progressed&&
                owner.animation().model(receiver.instance).nodes[0].transform.position.y==progressed_y-0.2f,
                "execution_disabled became a second scheduler rather than an ordinary command tail");
        send(receiver.object,26,sender.object); tick();
        const auto stopped=owner.animation().model(receiver.instance).animation_time;
        const auto stopped_y=owner.animation().model(receiver.instance).nodes[0].transform.position.y;
        require(stopped>time,"source process progress oracle never advanced before26 suspension");
        owner.course().execution_enabled=true; tick();
        require(owner.animation().model(receiver.instance).animation_time==stopped&&
                owner.animation().model(receiver.instance).nodes[0].transform.position.y==stopped_y,
                "predicate transition implicitly resumed source processes paused by26");
        send(receiver.object,-3,sender.object); tick();
        require(owner.animation().model(receiver.instance).animation_time>stopped&&
                owner.animation().model(receiver.instance).nodes[0].transform.position.y==stopped_y-0.2f,
                "real FIFO -3 failed to resume26-paused native source processes");
        std::cout<<"command26_FIFO_process_progress=observed animation_before="<<time
                 <<" animation_resumed="<<progressed<<" path_before="<<y<<" path_resumed="<<progressed_y<<'\n';
        reset();
    }
    void first_held_slot_preserves_progress() {
        const auto earlier=spawn(),held_sender=spawn(),later=spawn(),receiver=spawn();
        send(receiver.object,12345,held_sender.object);
        native_delete(earlier); native_delete(held_sender); native_delete(later);
        held(held_sender);
        const auto earlier_replacement=spawn();
        incarnation(earlier,earlier_replacement,"earlier-progress-before-first-held");
        require(!owner.membership(earlier.instance).joined&&owner.membership(later.instance).joined,
                "collection did not preserve earlier progress and stop at first held semantic slot");
        const auto unavailable=attempt();
        require(!unavailable&&unavailable.status.error==actor::OwnerError::capacity,
                "collection skipped first-held slot to reclaim a later retiring slot");
        dispatch(receiver.object);
        accepted(owner.collect_joined());
        const auto held_replacement=spawn(),later_replacement=spawn();
        incarnation(held_sender,held_replacement,"arbitrary-custom-command");
        incarnation(later,later_replacement,"after-first-held");
        reset();
    }
    void nonnull_graph_and_failed_enqueue() {
        const auto sender=spawn(),receiver=spawn();
        auto* record=owner.source(receiver.instance);
        record->transitionGraph=&invalid_graph;
        require(services.source_reference_assign(receiver.object,&record->apple,receiver.object)&&
                services.source_reference_assign(receiver.object,&record->interactionTarget,receiver.object),"adopt self fields");
        const State state(services,*record); const auto before=counts(services);
        send(receiver.object,26,sender.object);
        require(isle_game_tick(services.game)==ISLE_GAME_EXECUTION_FAILURE&&services.failed(),
                "invalid typed graph26 bypassed native C admission/fault boundary");
        state.unchanged(services,*record);
        require(same_counts(counts(services),before),"rejected graph26 ran native state/process ownership mutation");
        // Sticky failure rejects before successful FIFO enqueue/observer. Keep
        // the receiver alive: cancellation cannot conceal a phantom credit.
        require(!services.send_command(receiver.object,26,sender.object),"failed Services accepted a new26 enqueue");
        native_delete(sender,ISLE_GAME_EXECUTION_FAILURE); held(sender);
        require(owner.registered(receiver.instance),
                "pre-consumption graph rejection canceled its receiver or consumed its sender credit");
        native_delete(receiver,ISLE_GAME_EXECUTION_FAILURE); accepted(owner.collect_joined());
        require(same_counts(counts(services),baseline),"failed graph/enqueue retained a sender credit or native storage");
    }
};
void run(rom::Snapshot& snapshot,const std::filesystem::path& profile) {
    require(std::filesystem::is_directory(profile)&&std::filesystem::is_empty(profile),
            "explicit private profile directory must exist and be empty");
    const world::Resources world_resources(snapshot); const auto& setup=world_resources.setup();
    const beach::ActorResources resources(snapshot,{setup.fog_rgb,setup.fog_min,setup.fog_max});
    const environment_sound::BeachResources sound_resources(snapshot);
    const menu::EffectResources effect_resources(snapshot);
    const std::array banks{&effect_resources.bank(menu::EffectGroup::common),&effect_resources.bank(menu::EffectGroup::beach)};
    auto hydrated=game_audio::hydrate(snapshot); require(bool(hydrated),"actual shared audio hydration failed");
    std::array<audio::effects::Section,8> sections; audio::effects::Config effects;
    require(game_audio::native_startup_effects(hydrated.resources,sections,effects)==audio::Error::none,
            "actual command26 audio effects configuration failed");
    private_audio::Capture capture; private_audio::Execution execution{snapshot,nullptr,{}};
    capture.samples.resize(8192*audio::AudioService::max_frames*2);
    auto made=game_audio::Session::create(std::move(hydrated.resources),effects,capture.output(),execution.executor());
    require(bool(made),"actual shared audio Session creation failed"); auto& session=*made.session;
    std::uint64_t retrace=0; unsigned tasks=0;
    const auto pump=[&] {
        ++retrace;
        require(session.retrace(retrace,retrace*1000000000ull/60)==audio::Error::none,"actual audio retrace failed");
        require(session.submit()==audio::Error::none,"actual checked PCM submission failed");
        audio::TaskView task; require(session.build(task)==audio::Error::none,"actual audio task build failed");
        if (task.token.serial) {
            private_audio::execute(session,execution,task); ++tasks;
            require(session.submit()==audio::Error::none,"completed actual RSP PCM acceptance failed");
        }
    };
    unsigned reuse_proofs=0;
    {
        game::State state(profile); require(state.initialize(1),"owned private profile initialization failed");
        game_input::Service input; scene::Storage storage(32*1024*1024); native(storage.create());
        a::SlotCapacity capacity{};
        for (const auto& model:resources.models()) {
            const auto needed=a::slot_capacity(model);
            capacity.nodes=std::max(capacity.nodes,needed.nodes);
            capacity.material_attachments=std::max(capacity.material_attachments,needed.material_attachments);
        }
        const std::array capacities{capacity,capacity,capacity,capacity};
        const auto with_owner=[&](auto body) {
            {
                scene::Services services(storage.game(),storage.arena(),{{4,6,0,1,2},{}},input,session,state);
                require(!services.failed(),services.error());
                auto* camera_object=services.object(1,0); require(camera_object,"actual camera object creation failed");
                IsleGameCamera* camera=nullptr; native(isle_game_attach_camera(services.game,camera_object,&camera));
                const IsleGameLookAt view{{0,0,0},{1,0,0},{0,1,0}};
                native(isle_game_camera_set_look_at(services.game,camera,&view));
                environment_sound::Environment environment(services,storage.arena(),sound_resources,nullptr,camera);
                focus::Detector detector;
                private_actor::WorldHooks hooks{services,camera};
                world::World world(services,world_resources,hooks.callbacks());
                {
                    actor::Owner owner(services,resources,world,environment,detector,capacities,banks,{32768,4096});
                    hooks.owner=&owner;private_actor::WorldLifetime lifetime{world,hooks};
                    Consumer consumer(services,owner,world,resources); body(consumer); reuse_proofs+=consumer.reuse_proofs;
                }
                world.retire(); environment.cleanup();
            }
            IsleGameCounts empty{}; native(isle_game_counts(storage.game(),&empty));
            require(same_counts(empty,{}),"actual Owner/World/Environment/Services destruction left native borrowers");
        };
        pump(); pump();
        with_owner([&](Consumer& consumer) {
            consumer.delivery_and_allocator_pressure(); pump();
            consumer.multiple_senders_and_receivers(); pump();
            consumer.fifo_delete_cancels_tail(); pump();
            consumer.live_consumption_and_legacy_clearing(); pump();
            consumer.actual_fifo_pause_tail(); pump();
            consumer.first_held_slot_preserves_progress(); pump();
        });
        // Sticky graph rejection is isolated from the healthy actual fixtures.
        with_owner([&](Consumer& consumer) { consumer.nonnull_graph_and_failed_enqueue(); });
        native(storage.destroy());
    }
    // Even idle shared audio must traverse actual retraces/build/RSP/submit and
    // the fixed240-retrace stop-drain; stop() is not synchronous retirement.
    require(session.stop()==audio::Error::none,"actual shared audio stop failed");
    require(session.state()==game_audio::Session::State::stopping,"idle audio stop bypassed actual retirement");
    unsigned stop_retraces=0;
    for (;stop_retraces<240&&session.state()!=game_audio::Session::State::draining;++stop_retraces) pump();
    require(stop_retraces>0&&session.state()==game_audio::Session::State::draining&&session.service().voices_retired(),
            "actual audio voice graph failed fixed240-retrace retirement");
    require(session.poll_drain()==audio::Error::none&&session.poll_drain()==audio::Error::none&&
            session.state()==game_audio::Session::State::drained&&tasks>0&&capture.used>0&&capture.finishes==1,
            "actual RSP/PCM owner failed terminal acceptance/drain");
    std::cout<<"actor_command26_private_CPU_native_API=passed reuse_proofs="<<reuse_proofs
             <<" audio_tasks="<<tasks<<" checked_PCM_samples="<<capture.used<<" stop_retraces="<<stop_retraces
             <<" GPU_fullgame_Original_Windows11=not_claimed\n";
}
} // namespace
int main(int argc,char** argv) {
    try {
        require(argc==3,"usage: actor-command26-rom-smoke PRIVATE_ROM EXISTING_EMPTY_PRIVATE_PROFILE_DIRECTORY");
        auto snapshot=isle::rom::read_snapshot(argv[1]); run(snapshot,argv[2]); return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr<<"actor-command26-rom-smoke: "<<error.what()<<'\n'; return EXIT_FAILURE;
    }
}
