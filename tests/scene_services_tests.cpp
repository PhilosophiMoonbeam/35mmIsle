#include "scene_services.hpp"
#include "actor_source.h"
#include "scene_storage.hpp"
#include "audio_fixture.hpp"
#include "engine_fixture.h"

#include <array>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace a = isle::audio;
namespace f = isle::audio_test;
namespace g = isle::game_audio;
namespace sc = isle::scene;
namespace fs = std::filesystem;
namespace {
int failures = 0;
void check(bool condition, std::string_view meaning) {
    if (!condition) { ++failures; std::cerr << "FAIL: " << meaning << '\n'; }
}
// Authored CPU-only audio contract fixture. These tests never build/complete an
// RSP task or submit PCM; no device, readiness or audio execution is qualified.
struct AudioOwner {
    const std::uint32_t* words = nullptr;
    unsigned retired = 0, submits = 0, finishes = 0;
    static bool initialize(void* p, std::span<std::uint32_t> words) noexcept {
        static_cast<AudioOwner*>(p)->words = words.data();
        return words.size_bytes() == a::AudioService::rdram_bytes;
    }
    static bool ready(void* p, std::span<std::uint32_t> words) noexcept {
        return static_cast<AudioOwner*>(p)->words == words.data();
    }
    static void retire(void* p) noexcept {
        auto& self = *static_cast<AudioOwner*>(p); self.words = nullptr; ++self.retired;
    }
    static g::Output::Info inspect(void*) noexcept { return {g::Output::State::ready,g::source_rate}; }
    static g::Output::Acceptance submit(void* p, std::span<const std::int16_t>) noexcept {
        ++static_cast<AudioOwner*>(p)->submits;
        return {}; // Unexpected PCM submission is a fixture failure, not a Null sink.
    }
    static bool finish(void* p) noexcept { ++static_cast<AudioOwner*>(p)->finishes; return false; }
    static g::Output::State poll(void*) noexcept { return g::Output::State::failed; }
    g::CreateResult create() {
        f::Fixture fixture;
        auto bank = fixture.decode();
        auto bytes = std::make_shared<a::Bytes>(92);
        (*bytes)[0] = std::byte{'S'}; (*bytes)[1] = std::byte{'1'};
        f::put16(*bytes,2,1); f::put32(*bytes,4,12); f::put32(*bytes,8,80);
        f::put32(*bytes,12,68); f::put32(*bytes,76,48);
        constexpr std::array<unsigned,12> track{0,0xc0,0,0,0x90,60,127,48,48,0xff,0x2f,0};
        for (std::size_t i = 0; i < track.size(); ++i) (*bytes)[80+i] = std::byte(track[i]);
        auto sequences = a::hydrate_sequences(bytes);
        check(bool(bank) && bool(sequences),"authored audio fixture descriptors hydrate");
        g::Resources resources;
        resources.sounds = bank.asset; resources.music = bank.asset; resources.sequences = sequences.asset;
        resources.coefficients = f::coefficients(); resources.priorities = {100};
        const std::array sections{a::effects::Section{0,160,1000,-1000,2000}};
        return g::Session::create(std::move(resources),{a::effects::Routing::network,512,sections},
            {this,inspect,submit,finish,poll},{this,initialize,ready,retire});
    }
};
struct Profile {
    fs::path path;
    explicit Profile(const fs::path& parent) {
        static unsigned sequence = 0;
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        for (unsigned i = 0; i < 100; ++i) {
            auto candidate = parent / ("35mmisle-scenes-"+std::to_string(stamp)+"-"+std::to_string(++sequence));
            if (fs::create_directory(candidate)) { path = std::move(candidate); return; }
        }
        throw std::runtime_error("cannot create owned scene service test profile");
    }
    ~Profile() { std::error_code error; fs::remove_all(path,error); }
};
struct Commands {
    sc::Services* services = nullptr;
    std::array<int,16> values{};
    unsigned count = 0;
    IsleGameObject* last_source = nullptr;
    IsleGameObject* replacement = nullptr;
};
void handler(IsleGameObject* object, std::int32_t command, IsleGameObject* source, void* opaque) {
    auto& state = *static_cast<Commands*>(opaque);
    state.values[state.count++] = command;
    state.last_source = source;
    if (command == 70) {
        state.services->erase(object);
        state.replacement = state.services->object(99,5);
        check(state.replacement != object,"self-delete cannot recycle native node or callback record inside active handler");
    }
}
void count(IsleGameObject*, void* opaque) { ++*static_cast<unsigned*>(opaque); }
void services_preserve_state_and_source_metadata(const fs::path& parent) {
    Profile profile(parent);
    isle::game::State state(profile.path);
    check(state.initialize(1),"initialize persistent profile once before concrete Services");
    if (!state.initialized()) return;
    auto* session = state.save();
    AudioOwner audio_owner;
    auto audio = audio_owner.create();
    check(bool(audio),"real native audio Session owns authored CPU-only fixture resources");
    if (!audio) return;
    isle::game_input::Service input;
    sc::Storage native(1024*1024);
    check(native.create() == ISLE_GAME_OK,"create actual native scene arena");
    {
        sc::Services services(native.game(),native.arena(),{{1,1,1,0,0},{}},input,*audio.session,state);
        check(!services.failed() && &services.save == session && &services.rng == &state.rng,
              "concrete Services borrows one application State without opening/reconstructing it");
        services.controller = 0; services.start_flags = 0x81;
        check(state.controller == 0 && state.title_start_flags == 0x81,"source controller and byte flags remain persistent references");
        auto* first = services.object(1,5,100);
        auto* second = services.object(2,5,100);
        auto* other = services.object(3,6,100);
        check(first && second && other,"native pools grow beyond source seeds");
        Commands first_commands{&services}, second_commands{&services}, other_commands{&services};
        services.set_command_handler(first,handler,&first_commands);
        services.set_command_handler(second,handler,&second_commands);
        services.set_command_handler(other,handler,&other_commands);
        unsigned calls = 0;
        services.process(first,count,&calls,ISLE_GAME_SYNCHRONOUS);
        check(services.send_command_to_link(5,41,other) && services.send_command(first,-2) &&
              isle_game_tick(native.game()) == ISLE_GAME_OK && first_commands.count == 1 &&
              second_commands.count == 1 && other_commands.count == 0 && calls == 0 &&
              first_commands.last_source == other,"link broadcast uses source update links and same builtin/custom FIFO");
        check(services.send_command(first,-3) && isle_game_tick(native.game()) == ISLE_GAME_OK && calls == 1,
              "builtin resume reaches native process rather than invented completion");
        IsleGameObjectMetadata metadata{};
        check(isle_game_object_metadata(native.game(),first,&metadata) == ISLE_GAME_OK && metadata.id == 1 &&
              metadata.update_link == 5 && metadata.priority == 100,"native source object/update metadata is explicit");
        IsleSprite primary{}; primary.width = 4; primary.height = 8; primary.x = 12;
        IsleSprite alternate = primary; alternate.x = 27;
        auto* s1 = services.append_sprite(first,10,primary,7);
        auto* s2 = services.append_sprite(second,11,primary,8);
        check(s1 && s2 && services.set_render(first,{30,6,0x40000000}) &&
              services.set_render(second,{30,6,0x40000000}),"explicit raster resources attach with independent source DL metadata");
        std::array<sc::SpriteDraw,2> draws{};
        std::size_t written = 0;
        check(services.sprite_draws(draws,written) && written == 2 && draws[0].object == first && draws[1].object == second &&
              draws[0].render.link == 30 && draws[0].render.camera_tag == 0x40000000 && draws[0].draw.generation == 7,
              "source after-equal insertion, DL link/camera tag and dynamic generation survive native composition");
        check(services.move_to_head(second) && services.sprite_draws(draws,written) && draws[0].object == second &&
              services.move_to_tail(second) && services.sprite_draws(draws,written) && draws[1].object == second,
              "head/tail ordering changes only the source render ordering");
        check(services.replace_sprite(s1,42,alternate,11) && services.sprite_draws(draws,written) &&
              draws[0].draw.resource == 42 && draws[0].draw.generation == 11 && draws[0].draw.view->x == 27,
              "full native source descriptor rebind selects explicit producer slot/generation");
        check(services.send_command(first,-4) && isle_game_tick(native.game()) == ISLE_GAME_OK && services.sprite_count() == 1 &&
              services.send_command(first,-5) && isle_game_tick(native.game()) == ISLE_GAME_OK && services.sprite_count() == 2,
              "builtin hidden state controls actual sprite admission");
        check(services.send_command(first,70) && services.send_command(first,71) && isle_game_tick(native.game()) == ISLE_GAME_OK &&
              first_commands.count == 2 && first_commands.values[1] == 70 && services.sprite_count() == 1 && !services.failed(),
              "custom self-deletion stops suffix dispatch and joins before queue/record recycling");
        check(!session->set_player_flag(16,true),"scene mutates the persistent real profile in RAM");
        draws[0].object = other;
        written = 99;
        check(!services.sprite_draws(std::span<sc::SpriteDraw>(draws).first(0),written) &&
              written == 0 && draws[0].object == other && services.failed(),
              "draw capacity rejects atomically before publication");
        const std::string first_error = services.error();
        services.fail("later source error");
        check(first_error == services.error() && !services.exit_requested(),
              "first diagnostic is retained and failure never masquerades as source scene completion");
    }
    IsleGameCounts counts{};
    check(isle_game_counts(native.game(),&counts) == ISLE_GAME_OK && counts.objects == 0 && counts.processes == 0 && counts.sprites == 0,
          "Services destruction retires all scene-native owners but not application services");
    check(state.save() == session && session->snapshot().value.player_flag(16).value &&
          audio_owner.submits == 0 && audio_owner.finishes == 0 && audio_owner.retired == 0,
          "scene destruction preserves real profile/RNG/audio owners without audio drain/pump");
    auto contender = isle::save::GameSession::open(profile.path,2);
    check(!contender && contender.store_error.code == isle::save::StoreErrc::lock_busy,
          "persistent profile lock remains held across actual Services destruction");
}
struct Lifecycle {
    sc::Services* services = nullptr;
    IsleGame* game = nullptr;
    IsleGameModel* model = nullptr;
    std::array<IsleGameProcess*,2> processes{};
    std::size_t process_count = 0;
    void* native_context = nullptr;
    void* update_context = nullptr;
    IsleGameObject* replacement = nullptr;
    IsleGameObject* source = nullptr;
    IsleGameObject* target = nullptr;
    std::array<int,16> events{};
    unsigned event_count = 0, retiring = 0, joined = 0, started = 0, continued = 0;
    unsigned custom = 0, after_join = 0, unexpected = 0;
    bool handler_active = false, joined_in_handler = false, direct_in_handler = false, reject_late = false;
    bool park_in_handler = false, fault_in_handler = false;
};
void lifecycle_event(Lifecycle& context, int event) noexcept {
    check(context.event_count < context.events.size(),"lifecycle trace capacity");
    if (context.event_count < context.events.size()) context.events[context.event_count++] = event;
}
void lifecycle_retiring(IsleGameObject* object, void* user) noexcept {
    auto& context = *static_cast<Lifecycle*>(user);
    check(context.retiring == 0 && context.joined == 0 &&
          engine_model_before_join(object,context.model,&context,context.update_context,-13),
          "Services retiring observes the actual intact model and stable native/actor callback contexts once");
    check(engine_object_process_count(object) == context.process_count,
          "Services retiring runs before any process is unlinked");
    for (std::size_t i = 0; i < context.process_count; ++i)
        check(engine_process_alive(context.processes[i]),"Services retiring runs before native process retirement");
    check(engine_process_waiting(context.processes[0]),
          "Services retiring observes the real parked continuation before its join");
    sc::RenderMetadata render{};
    check(!context.services->render_metadata(object,render),"actual pre-retirement closes Services render admission");
    ++context.retiring;
    lifecycle_event(context,30);
}
void lifecycle_joined(IsleGameObject* object, void* user) noexcept {
    auto& context = *static_cast<Lifecycle*>(user);
    check(context.retiring == 1 && context.joined == 0 &&
          engine_model_after_join(object,context.model,&context,context.update_context),
          "Services joined observes actual native payload removal before owner context recycling");
    for (std::size_t i = 0; i < context.process_count; ++i)
        check(engine_process_joined(context.processes[i]),
              "Services joined is after actual continuation destruction, not a requested-delete notification");
    context.joined_in_handler = context.handler_active;
    ++context.joined;
    lifecycle_event(context,40);
}
// These process frames retain only trivial state across the native C fence.
void lifecycle_sleep(IsleGameObject*, void* user) noexcept {
    auto* context = static_cast<Lifecycle*>(user);
    ++context->started;
    check(isle_game_wait(context->game,100) == ISLE_GAME_OK,"park real Services continuation");
    ++context->continued;
}
void lifecycle_fault(IsleGameObject*, void* user) noexcept {
    ohWait(1); // The genuine native synchronous-wait fault must not return.
    ++static_cast<Lifecycle*>(user)->continued;
}
void lifecycle_dispatch_process(IsleGameObject*, void* user) noexcept {
    auto* context = static_cast<Lifecycle*>(user);
    context->services->process_commands(context->target);
    ++context->continued;
}
void lifecycle_delete_update(IsleGameObject* object, void* user) noexcept {
    auto* context = static_cast<Lifecycle*>(user);
    context->services->erase(object);
    check(context->retiring == 0 && context->joined == 0 && engine_model_alive(context->model),
          "Services self-delete request retains the current callback/model until actual deferred teardown");
    if (context->reject_late)
        check(!context->services->set_lifecycle(object,{count,count,&context->unexpected}),
              "late lifecycle replacement rejects atomically while current deletion is deferred");
    lifecycle_event(*context,20);
}
void lifecycle_command(IsleGameObject* object, std::int32_t command, IsleGameObject* source, void* user) noexcept {
    auto* context = static_cast<Lifecycle*>(user);
    context->handler_active = true;
    check(source == context->source,"lifecycle command retains its actual queued source owner");
    ++context->custom;
    lifecycle_event(*context,command);
    if (context->fault_in_handler) {
        ohWait(1);
        ++context->continued;
    }
    if (context->park_in_handler) {
        check(isle_game_wait(context->game,100) == ISLE_GAME_OK,"park inside actual custom command dispatch");
        ++context->continued;
    }
    if (context->direct_in_handler) {
        engine_delete_object_direct(object);
        check(context->joined == 1 && context->joined_in_handler,
              "direct native deletion notifies joined during the still-live custom command dispatch");
        context->replacement = context->services->object(99);
        check(context->replacement == object && engine_object_update_context(context->replacement) != context->native_context,
              "native node may reuse only after join, but Services callback record cannot recycle inside dispatch");
        ++context->after_join;
        lifecycle_event(*context,50);
    }
    context->handler_active = false;
}
IsleGameObject* lifecycle_object(sc::Services& services, Lifecycle& context) {
    context.services = &services;
    context.game = services.game;
    auto* object = services.object(1);
    context.target = object;
    check(object != nullptr,"create lifecycle-owned native object");
    if (object == nullptr) throw std::runtime_error("cannot create lifecycle fixture object");
    engine_set_actor_payload(object,&context);
    context.native_context = engine_object_update_context(object);
    context.update_context = context.native_context;
    const IsleGameModelNodeInit node{UINT32_MAX,{{7,-13,11},{0,0,0},{1,1,1}},ISLE_GAME_MODEL_CONTEXTUAL_RPY};
    check(isle_game_attach_model(services.game,object,&node,1,&context.model) == ISLE_GAME_OK &&
          services.set_render(object,{4,9,0xffffffffu}) &&
          services.set_lifecycle(object,{lifecycle_retiring,lifecycle_joined,&context}),
          "compose lifecycle observers without replacing Services native ownership hooks");
    context.processes[0] = services.process(object,lifecycle_sleep,&context);
    context.process_count = 1;
    check(context.processes[0] && isle_game_tick(services.game) == ISLE_GAME_OK &&
          context.started == 1 && context.retiring == 0 && context.joined == 0,
          "real model and parked Services continuation stay active before retirement");
    return object;
}
struct AbaDispatch {
    sc::Services* services = nullptr;
    IsleGame* game = nullptr;
    IsleGameObject* target = nullptr;
    std::int32_t command = 0;
    unsigned entered = 0, returned = 0, finished = 0;
    bool release = false;
};
void aba_command(IsleGameObject*, std::int32_t command, IsleGameObject*, void* user) noexcept {
    auto* context = static_cast<AbaDispatch*>(user);
    check(command == context->command,"ABA dispatch retains its own queued command");
    ++context->entered;
    while (!context->release)
        check(isle_game_wait(context->game,1) == ISLE_GAME_OK,"park actual ABA command continuation");
    ++context->returned;
}
void aba_dispatch(IsleGameObject*, void* user) noexcept {
    auto* context = static_cast<AbaDispatch*>(user);
    context->services->process_commands(context->target);
    ++context->finished;
}
void services_dispatch_owner_native_address_aba(sc::Services& services) {
    auto* target_t = services.object(101);
    auto* caller_a = services.object(102);
    auto* target_v = services.object(103);
    check(target_t && caller_a && target_v,"create registered ABA targets and original native caller");
    if (!target_t || !caller_a || !target_v) throw std::runtime_error("cannot create ABA owners");
    void* const record_t = engine_object_update_context(target_t);
    void* const record_v = engine_object_update_context(target_v);
    check(services.set_update(target_t,nullptr,nullptr) && services.set_update(target_v,nullptr,nullptr),
          "only actual parked callers dispatch the ABA target queues");
    AbaDispatch a{&services,services.game,target_t,41};
    AbaDispatch u{&services,services.game,target_v,42};
    check(services.set_command_handler(target_t,aba_command,&a) &&
          services.set_command_handler(target_v,aba_command,&u),"install independent ABA command contexts");
    auto* process_a = services.process(caller_a,aba_dispatch,&a,ISLE_GAME_STACKFUL,3);
    check(process_a && services.send_command(target_t,41) && isle_game_tick(services.game) == ISLE_GAME_OK &&
          a.entered == 1 && a.returned == 0,"registered A parks inside T's real command dispatch");
    services.erase(target_t);
    IsleGameObject* caller_u = nullptr;
    check(isle_game_add_object(services.game,104,0,0,nullptr,nullptr,&caller_u) == ISLE_GAME_OK &&
          caller_u == target_t,"unregistered native U actually reuses retired T's native address");
    if (!caller_u) throw std::runtime_error("cannot create unregistered ABA caller");
    IsleGameProcess* process_u = nullptr;
    check(isle_game_add_process(services.game,caller_u,aba_dispatch,&u,ISLE_GAME_STACKFUL,2,&process_u) ==
              ISLE_GAME_OK && services.send_command(target_v,42) &&
          isle_game_tick(services.game) == ISLE_GAME_OK && u.entered == 1 && u.returned == 0,
          "unregistered U parks inside V while retained retired T still has U's native address");
    services.erase(target_v);
    services.erase(caller_a);
    check(engine_process_joined(process_a) && a.returned == 0 && a.finished == 0,
          "actual A join retires its own T dispatch without completing U's independent frame");
    auto* joined_a_record = services.object(105);
    auto* unrelated_r = services.object(106);
    check(joined_a_record && unrelated_r && engine_object_update_context(joined_a_record) != record_t &&
          engine_object_update_context(unrelated_r) == record_t,"registered unrelated R actually reuses retired T's Services record");
    // Consume the one command node released by A/T before retiring R. Keeping
    // it queued makes the failure-before path safe to finish: V's suspended
    // command node has no recycled free-list suffix to traverse after release.
    auto* held_queue = services.object(107);
    check(held_queue && services.set_update(held_queue,nullptr,nullptr) &&
          services.send_command(held_queue,43),"retain independent command storage throughout the ABA observation");
    services.erase(unrelated_r);
    check(engine_object_alive(caller_u) && engine_process_waiting(process_u) &&
          u.returned == 0 && u.finished == 0,"unregistered U remains genuinely parked across unrelated R's actual join");
    auto* probe_one = services.object(108);
    auto* probe_two = services.object(109);
    check(probe_one && probe_two && engine_object_update_context(probe_one) != record_v &&
          engine_object_update_context(probe_two) != record_v,
          "unrelated R join must not certify U or recycle V's pending command queue/Services record");
    if (probe_one) services.set_update(probe_one,nullptr,nullptr);
    if (probe_two) services.set_update(probe_two,nullptr,nullptr);
    // The unregistered caller returns normally; target retirement alone never
    // supplies its completion proof, even during final Services destruction.
    u.release = true;
    check(isle_game_tick(services.game) == ISLE_GAME_OK && u.entered == 1 && u.returned == 1 && u.finished == 1 &&
          engine_process_joined(process_u) && !services.failed(),
          "U's actual return ends its dispatch exactly once without inheriting another owner's queue");
    check(isle_game_delete_object(services.game,caller_u) == ISLE_GAME_OK,
          "retire the fixture-owned unregistered native caller after its real frame completes");
}

void services_lifecycle_composes_delete_paths(const fs::path& parent) {
    Profile profile(parent);
    isle::game::State state(profile.path);
    check(state.initialize(1),"initialize lifecycle profile");
    if (!state.initialized()) return;
    AudioOwner audio_owner;
    auto audio = audio_owner.create();
    check(bool(audio),"create lifecycle CPU-only audio resource owner");
    if (!audio) return;
    isle::game_input::Service input;
    for (int mode : {0,1,2,3,4,5,6}) {
        sc::Storage native(1024*1024);
        check(native.create() == ISLE_GAME_OK,"create lifecycle native arena");
        Lifecycle context{};
        {
            sc::Services services(native.game(),native.arena(),{{1,1,0,0,0},{}},input,*audio.session,state);
            auto* object = lifecycle_object(services,context);
            context.source = services.object(2,1);
            check(services.set_command_handler(object,lifecycle_command,&context),"install real custom command handler");
            if (mode == 0) {
                check(services.send_command(object,21,context.source) && services.send_command(object,-1,context.source) &&
                      services.send_command(object,22,context.source) && isle_game_tick(native.game()) == ISLE_GAME_OK &&
                      context.custom == 1 && context.retiring == 1 && context.joined == 1 &&
                      context.event_count == 3 && context.events[0] == 21 && context.events[1] == 30 &&
                      context.events[2] == 40 && !context.joined_in_handler && !services.failed(),
                      "standard source command -1 defers current deletion and stops the queued custom suffix");
            } else if (mode == 1 || mode == 5) {
                check(services.send_command(object,22,context.source),"queue custom command before direct retirement");
                if (mode == 5)
                    check(services.set_lifecycle(object,{}),"all-null clears external observers but not Services ownership");
                check(isle_game_delete_object(native.game(),object) == ISLE_GAME_OK && context.custom == 0,
                      "direct public native deletion bypasses no ownership hook and does not execute queued commands");
                auto* replacement = services.object(99);
                unsigned calls = 0;
                check(replacement == object && services.set_command_handler(replacement,
                    [](IsleGameObject*,std::int32_t command,IsleGameObject*,void* user) noexcept {
                        check(command == 23,"recycled Services queue contains only its new command");
                        ++*static_cast<unsigned*>(user);
                    },&calls) && services.send_command(replacement,23) &&
                    isle_game_tick(native.game()) == ISLE_GAME_OK && calls == 1,
                    "direct delete preserves Services queue cleanup and clean native/record reuse");
                services.erase(replacement);
                check(context.retiring == (mode == 5 ? 0u : 1u) && context.joined == (mode == 5 ? 0u : 1u),
                      "cleared or prior-incarnation lifecycle observers cannot leak into a reused owner");
            } else if (mode == 2) {
                context.direct_in_handler = true;
                check(services.send_command(object,21,context.source) && services.send_command(object,22,context.source) &&
                      services.process_commands(object) && context.custom == 1 && context.after_join == 1 &&
                      context.event_count == 4 && context.events[0] == 21 && context.events[1] == 30 &&
                      context.events[2] == 40 && context.events[3] == 50,
                      "actual direct GAME deletion inside a custom dispatch retains userdata and cancels the old queue");
                auto* after_dispatch = services.object(100);
                check(after_dispatch && engine_object_update_context(after_dispatch) == context.native_context,
                      "Services recycles the old callback record only after its joined observer and command dispatch return");
                unsigned calls = 0;
                check(services.set_command_handler(context.replacement,
                    [](IsleGameObject*,std::int32_t command,IsleGameObject*,void* user) noexcept {
                        check(command == 23,"replacement did not inherit the retiring queue suffix");
                        ++*static_cast<unsigned*>(user);
                    },&calls) && services.send_command(context.replacement,23) &&
                    isle_game_tick(native.game()) == ISLE_GAME_OK && calls == 1 && !services.failed(),
                    "replacement command processing remains independent while old observer userdata is still owned");
            } else if (mode == 3 || mode == 4) {
                context.reject_late = mode == 4;
                context.update_context = &context;
                check(services.set_update(object,lifecycle_delete_update,&context) &&
                      isle_game_tick(native.game()) == ISLE_GAME_OK &&
                      context.retiring == 1 && context.joined == 1 && context.unexpected == 0 &&
                      context.event_count == 3 && context.events[0] == 20 && context.events[1] == 30 &&
                      context.events[2] == 40 && services.failed() == (mode == 4),
                      "Services erase preserves deferred current lifetime and rejects late observer replacement atomically");
            } else {
                IsleGameObject* unregistered = nullptr;
                check(isle_game_add_object(native.game(),3,2,0,nullptr,nullptr,&unregistered) == ISLE_GAME_OK &&
                      !services.set_lifecycle(unregistered,{count,count,&context.unexpected}),
                      "Services refuses to install lifecycle observers on an unregistered native owner");
                services.erase(object);
                check(context.retiring == 1 && context.joined == 1 && context.unexpected == 0,
                      "invalid lifecycle installation cannot replace a registered owner's observers");
                check(isle_game_delete_object(native.game(),unregistered) == ISLE_GAME_OK,
                      "retire fixture-owned unregistered native object");
            }
        }
        IsleGameCounts counts{};
        check(isle_game_counts(native.game(),&counts) == ISLE_GAME_OK && counts.objects == 0 &&
              counts.processes == 0 && counts.models == 0 && context.continued == 0 && context.unexpected == 0,
              "all Services delete paths join parked work and retain neither queues, native owners nor external callbacks");
    }
    for (bool fault : {false,true}) {
        sc::Storage native(1024*1024);
        check(native.create() == ISLE_GAME_OK,"create destructor lifecycle native arena");
        Lifecycle context{};
        {
            sc::Services services(native.game(),native.arena(),{{1,1,0,0,0},{}},input,*audio.session,state);
            auto* object = lifecycle_object(services,context);
            if (fault) {
                context.processes[1] = services.process(object,lifecycle_fault,&context,ISLE_GAME_SYNCHRONOUS,0);
                context.process_count = 2;
                check(context.processes[1] && isle_game_tick(native.game()) == ISLE_GAME_EXECUTION_FAILURE &&
                      context.retiring == 0 && context.joined == 0 && engine_model_alive(context.model),
                      "native callback failure leaves Services owner/payload intact until actual fault destruction");
            }
            check(services.send_command(object,22),"retain queued userdata until real destructor retirement");
        }
        IsleGameCounts counts{};
        check(context.retiring == 1 && context.joined == 1 && context.custom == 0 && context.continued == 0 &&
              isle_game_counts(native.game(),&counts) == ISLE_GAME_OK && counts.objects == 0 &&
              counts.processes == 0 && counts.models == 0,
              "normal/fault Services destruction notifies real native pre/joined events and releases pending queues");
    }
    for (int mode : {0,1,2,3}) {
        sc::Storage native(1024*1024);
        check(native.create() == ISLE_GAME_OK,"create pending command lifecycle native arena");
        Lifecycle context{};
        {
            sc::Services services(native.game(),native.arena(),{{1,1,0,0,0},{}},input,*audio.session,state);
            auto* object = lifecycle_object(services,context);
            check(services.set_command_handler(object,lifecycle_command,&context),"install pending native command handler");
            if (mode == 1) {
                context.fault_in_handler = true;
                check(services.send_command(object,21) && services.send_command(object,22) &&
                      isle_game_tick(native.game()) == ISLE_GAME_EXECUTION_FAILURE &&
                      context.custom == 1 && context.retiring == 0 && context.joined == 0,
                      "real synchronous command fault abandons its command frame without early retirement");
            } else {
                context.park_in_handler = true;
                check(services.set_update(object,nullptr,nullptr),"dispatch queued commands only from the selected stackful frame");
                context.update_context = nullptr;
                auto* dispatch_owner = mode == 0 ? object : services.object(2,1);
                context.source = mode == 0 ? nullptr : dispatch_owner;
                auto* dispatch = services.process(dispatch_owner,lifecycle_dispatch_process,&context,ISLE_GAME_STACKFUL,2);
                if (mode == 0) {
                    context.processes[1] = dispatch;
                    context.process_count = 2;
                }
                check(dispatch && services.send_command(object,21,context.source) &&
                      services.send_command(object,22,context.source) && isle_game_tick(native.game()) == ISLE_GAME_OK &&
                      context.custom == 1 && context.handler_active && context.retiring == 0,
                      "actual custom command dispatch remains parked with its queue and observer context live");
                services.erase(object);
                check(context.retiring == 1 && context.joined == 1 && context.continued == 0,
                      "actual whole-object join notifies while never resuming the parked command body");
                auto* replacement = services.object(99);
                if (mode == 0) {
                    check(replacement && engine_object_update_context(replacement) == context.native_context,
                          "self-owned parked command frame is gone at actual join, permitting record reuse");
                    unsigned calls = 0;
                    check(services.set_command_handler(replacement,
                        [](IsleGameObject*,std::int32_t command,IsleGameObject*,void* user) noexcept {
                            check(command == 23,"joined parked dispatch did not retain its pending queue suffix");
                            ++*static_cast<unsigned*>(user);
                        },&calls) && services.send_command(replacement,23) &&
                        isle_game_tick(native.game()) == ISLE_GAME_OK && calls == 1,
                        "joined parked dispatch releases its processing queue before replacement command admission");
                    services.erase(replacement);
                } else {
                    check(replacement && engine_object_update_context(replacement) != context.native_context &&
                          engine_process_alive(dispatch) && engine_process_has_continuation(dispatch),
                          "different-object pending command retains its callback record until its own native caller joins");
                    if (mode == 3) {
                        services.erase(dispatch_owner);
                        check(engine_process_joined(dispatch),
                              "external dispatch caller actually joins before its retained target record can recycle");
                        auto* joined_caller = services.object(100);
                        auto* joined_target = services.object(101);
                        check(joined_caller && joined_target && engine_object_update_context(joined_target) == context.native_context &&
                              !services.failed(),
                              "actual external caller join reclaims the abandoned target queue/record without restarting live work");
                    }
                }
            }
        }
        IsleGameCounts counts{};
        check(context.retiring == 1 && context.joined == 1 && context.custom == 1 && context.continued == 0 &&
              isle_game_counts(native.game(),&counts) == ISLE_GAME_OK && counts.objects == 0 &&
              counts.processes == 0 && counts.models == 0,
              "command-fault and cross-object pending frames retain their owners through all real joins, then reclaim queues");
    }
    {
        sc::Storage native(1024*1024);
        check(native.create() == ISLE_GAME_OK,"create failed-scope observer-detach arena");
        Lifecycle context{};
        {
            sc::Services services(native.game(),native.arena(),{{1,1,0,0,0},{}},input,*audio.session,state);
            auto* object = lifecycle_object(services,context);
            services.fail("source failure before borrowed observer teardown");
            const std::string first_error = services.error();
            check(!services.set_lifecycle(object,{count,count,&context.unexpected}) &&
                  services.failed() && first_error == services.error(),
                  "failed scene cannot install new lifecycle work or replace its original failure");
            check(services.set_lifecycle(object,{}) && services.failed() && first_error == services.error(),
                  "validated all-null detach must release borrowed callbacks without healing scene failure");
            check(isle_game_delete_object(native.game(),object) == ISLE_GAME_OK &&
                  context.retiring == 0 && context.joined == 0 && context.unexpected == 0 &&
                  engine_process_joined(context.processes[0]) && context.continued == 0,
                  "detached failed-scope observer survives deletion or real parked continuation failed to join");
            IsleGameCounts counts{};
            check(isle_game_counts(native.game(),&counts) == ISLE_GAME_OK &&
                  counts.objects == 0 && counts.processes == 0 && counts.models == 0 && counts.model_nodes == 0,
                  "failed-scope detach removed native ownership hooks or leaked native model/process storage");
        }
        check(native.destroy() == ISLE_GAME_OK,"destroy joined failed-scope observer arena");
    }
    {
        sc::Storage native(1024*1024);
        check(native.create() == ISLE_GAME_OK,"create native/Services address-ABA arena");
        sc::Services services(native.game(),native.arena(),{{1,1,0,0,0},{}},input,*audio.session,state);
        services_dispatch_owner_native_address_aba(services);
    }
}

// These are real Services/GObj/canonical-payload owners, not queue callback
// echoes. Native identity reuse is checked independently of semantic fields.
struct SourceRecord {
    IsleActorSource source{};
    std::array<IsleGameObject**,2> fields{&source.apple,&source.interactionTarget};
    IsleGameObject* object = nullptr;
    IsleGameModel* model = nullptr;
};
void source_record(sc::Services& services, SourceRecord& record, unsigned id) {
    record.object = services.object(id);
    const IsleGameModelNodeInit node{UINT32_MAX,{{1,2,3},{0,0,0},{1,1,1}},ISLE_GAME_MODEL_CONTEXTUAL_RPY};
    check(record.object && isle_game_attach_model(services.game,record.object,&node,1,&record.model) == ISLE_GAME_OK &&
          isle_actor_source_bind(services.game,record.object,&record.source,&services.rng,&record) == ISLE_GAME_OK &&
          services.source_owner_register(record.object,record.fields),"register actual canonical source/model owner");
}
struct SourceDelivery {
    sc::Services* services = nullptr;
    SourceRecord* receiver = nullptr;
    IsleGameObject* sender = nullptr;
    unsigned calls = 0, returned = 0;
    bool park = false;
};
void source_delivery(IsleGameObject* object, std::int32_t command, IsleGameObject* sender, void* user) noexcept {
    auto* context = static_cast<SourceDelivery*>(user);
    auto& services = *context->services;
    ++context->calls;
    check(sender == context->sender && !services.source_owner_collectable(sender) &&
          !services.source_owner_release(sender),"in-flight retired sender cannot collect inside manual handler");
    if (context->park) {
        check(isle_game_wait(services.game,100) == ISLE_GAME_OK,"park actual in-flight source delivery");
        ++context->returned;
        return;
    }
    if (command == 73) {
        check(services.source_reference_assign(object,&context->receiver->source.interactionTarget,sender),
              "adopt actual canonical target before finishing delivered credit");
        check(services.source_command_before_transfer(object) && services.source_command_before_transfer(object) &&
              services.source_owner_borrows(sender).fields == 1 &&
              services.source_owner_borrows(sender).queued == 1,
              "explicit finish settles current once while preserving ordinary FIFO suffix");
    } else {
        check(command == 74 && services.source_owner_borrows(sender).queued == 1,
              "ordinary suffix arrives with its own in-flight credit");
    }
}
void source_dispatch(IsleGameObject*, void* user) noexcept {
    auto* context = static_cast<SourceDelivery*>(user);
    context->services->process_commands(context->receiver->object);
    ++context->returned;
}
void services_source_owners(const fs::path& parent) {
    Profile profile(parent);
    isle::game::State state(profile.path);
    check(state.initialize(1),"initialize source-owner profile");
    AudioOwner audio_owner;
    auto audio = audio_owner.create();
    if (!audio || !state.initialized()) throw std::runtime_error("source-owner fixture admission");
    isle::game_input::Service input;
    sc::Storage native(1024*1024);
    check(native.create() == ISLE_GAME_OK,"create source-owner native arena");
    sc::Services services(native.game(),native.arena(),{{2,2,0,0,0},{}},input,*audio.session,state);
    SourceRecord sender, receiver, alternate;
    source_record(services,sender,201); source_record(services,receiver,202); source_record(services,alternate,203);
    SourceDelivery delivery{&services,&receiver,sender.object};
    check(services.set_command_handler(receiver.object,source_delivery,&delivery) &&
          services.send_command(receiver.object,73,sender.object) &&
          services.send_command(receiver.object,74,sender.object) &&
          services.source_owner_borrows(sender.object).queued == 2,"all custom commands acquire actual successful-enqueue borrows");
    services.erase(sender.object);
    IsleActorSource* expired = nullptr;
    check(isle_actor_source_get(services.game,sender.object,&expired) != ISLE_GAME_OK && !expired &&
          !services.source_owner_collectable(sender.object),"held sender is identity-only after real native retirement");
    auto* pressure = services.object(204);
    check(pressure && pressure != sender.object,"native allocator cannot alias queued retired source");
    check(services.process_commands(receiver.object) && delivery.calls == 2 &&
          services.source_owner_borrows(sender.object).queued == 0 &&
          services.source_owner_borrows(sender.object).fields == 1 &&
          receiver.source.interactionTarget == sender.object &&
          !services.source_owner_collectable(sender.object),"delivery adopts persistent actual target without releasing semantic source");
    check(services.source_reference_assign(receiver.object,&receiver.source.apple,sender.object) &&
          services.source_owner_borrows(sender.object).fields == 2 &&
          services.source_reference_assign(receiver.object,&receiver.source.apple,sender.object) &&
          services.source_owner_borrows(sender.object).fields == 2,"same-field assignment does not double borrow");
    check(services.source_reference_assign(receiver.object,&receiver.source.interactionTarget,alternate.object) &&
          services.source_owner_borrows(sender.object).fields == 1 &&
          services.source_owner_borrows(alternate.object).fields == 1 &&
          services.source_reference_assign(receiver.object,&receiver.source.apple,nullptr) &&
          services.source_owner_collectable(sender.object) && services.source_owner_release(sender.object),
          "replacement and NULL release old borrows exactly once");
    auto* reused = services.object(205);
    check(reused == sender.object,"final joined source release returns actual address to native allocator");
    services.erase(receiver.object); // Closes its owned target, not other owners' fields.
    check(!receiver.source.interactionTarget && services.source_owner_borrows(alternate.object).fields == 0 &&
          services.source_owner_release(receiver.object),"receiver retirement closes actual canonical field");
    services.erase(alternate.object);
    check(services.source_owner_release(alternate.object),"release alternate joined semantic owner");
    services.erase(pressure); services.erase(reused);

    SourceRecord cycle_a, cycle_b;
    source_record(services,cycle_a,206); source_record(services,cycle_b,207);
    check(services.source_reference_assign(cycle_a.object,&cycle_a.source.apple,cycle_b.object) &&
          services.source_reference_assign(cycle_b.object,&cycle_b.source.interactionTarget,cycle_a.object) &&
          services.send_command(cycle_b.object,12345,cycle_a.object),"cross-owner cycle and arbitrary custom sender enqueue");
    services.erase(cycle_a.object);
    check(!cycle_a.source.apple && cycle_b.source.interactionTarget == cycle_a.object &&
          !services.source_owner_collectable(cycle_a.object) &&
          services.source_owner_borrows(cycle_a.object).queued == 1,
          "sender retirement does not eagerly clear other receiver's canonical fields or queued notifications");
    services.erase(cycle_b.object);
    check(!cycle_b.source.interactionTarget && services.source_owner_borrows(cycle_a.object).queued == 0 &&
          services.source_owner_release(cycle_a.object) && services.source_owner_release(cycle_b.object),
          "receiver retirement breaks field cycle and cancels each queued credit once");

    SourceRecord parked_sender, parked_receiver;
    source_record(services,parked_sender,208); source_record(services,parked_receiver,209);
    auto* caller = services.object(210);
    SourceDelivery parked{&services,&parked_receiver,parked_sender.object,0,0,true};
    check(services.set_update(parked_receiver.object,nullptr,nullptr) &&
          services.set_command_handler(parked_receiver.object,source_delivery,&parked) &&
          services.process(caller,source_dispatch,&parked) &&
          services.send_command(parked_receiver.object,73,parked_sender.object) &&
          isle_game_tick(services.game) == ISLE_GAME_OK && parked.calls == 1,"park real cross-object source command continuation");
    services.erase(parked_sender.object); services.erase(parked_receiver.object);
    check(services.source_owner_borrows(parked_sender.object).queued == 1 &&
          !services.source_owner_collectable(parked_sender.object) &&
          !services.source_owner_collectable(parked_receiver.object),
          "receiver join alone cannot close external caller's active dispatch token");
    services.erase(caller);
    check(!parked.returned && services.source_owner_borrows(parked_sender.object).queued == 0 &&
          services.source_owner_release(parked_sender.object) &&
          services.source_owner_release(parked_receiver.object),"actual caller join closes abandoned active borrow without unwinding");

    SourceRecord failed_sender, failed_receiver;
    source_record(services,failed_sender,211); source_record(services,failed_receiver,212);
    services.fail("authored source enqueue rejection");
    check(!services.send_command(failed_receiver.object,99,failed_sender.object) &&
          services.source_owner_borrows(failed_sender.object).queued == 0,"failed enqueue records no semantic borrow");
    services.erase(failed_sender.object);
    check(services.source_owner_release(failed_sender.object),"failed enqueue cannot pin an otherwise collectable source");
    services.erase(failed_receiver.object);
    check(services.source_owner_release(failed_receiver.object),"failed scene permits joined source ownership release");
}
void adopt_target(IsleGameObject* object, std::int32_t, IsleGameObject* sender, void* user) noexcept {
    auto& context=*static_cast<SourceDelivery*>(user);
    auto& receiver=*context.receiver;
    auto& services=*context.services;
    check(services.source_reference_assign(object,&receiver.source.interactionTarget,sender),
          "actual canonical target assignment failed");
}
void services_reject_unregistered_sender(const fs::path& parent) {
    Profile profile(parent);
    isle::game::State state(profile.path);
    check(state.initialize(1),"initialize source-admission profile");
    AudioOwner audio_owner; auto audio=audio_owner.create();
    if (!audio || !state.initialized()) throw std::runtime_error("source-admission fixture");
    isle::game_input::Service input;
    sc::Storage native(1024*1024);
    check(native.create()==ISLE_GAME_OK,"create source-admission native arena");
    sc::Services services(native.game(),native.arena(),{{2,2,0,0,0},{}},input,*audio.session,state);
    SourceRecord receiver;
    source_record(services,receiver,301);
    SourceDelivery delivery{&services,&receiver};
    check(services.set_command_handler(receiver.object,adopt_target,&delivery),"install real field-adopting receiver");
    auto* raw_sender=services.object(302);
    check(raw_sender!=nullptr,"create unregistered native sender");
    const bool enqueued=services.send_command(receiver.object,26,raw_sender);
    check(!enqueued && services.failed(),"unregistered semantic sender must reject before source FIFO publication");
    services.erase(raw_sender);
    IsleGameObject* replacement=nullptr;
    if (enqueued) {
        // Expose the actual ABA consequence if admission ever regresses again.
        replacement=services.object(303);
        check(replacement && services.source_owner_register(replacement,{}),"register allocator replacement");
        check(services.process_commands(receiver.object),"deliver unexpected unsafe source command");
        check(receiver.source.interactionTarget!=replacement,
              "queued retired unregistered sender rebound to replacement semantic identity");
    }
    check(!receiver.source.interactionTarget &&
          services.source_owner_borrows(receiver.object).queued==0 &&
          services.source_owner_borrows(receiver.object).fields==0,
          "rejected source admission changed canonical fields or semantic credits");
    services.erase(receiver.object);
    check(services.source_owner_release(receiver.object),"release actual receiver after rejected source request");
    if (replacement) {
        services.erase(replacement);
        check(services.source_owner_release(replacement),"release unexpected replacement after receiver cancellation");
    }
}

} // namespace
int main(int argc, char** argv) {
    try {
        if (argc > 2) throw std::runtime_error("usage: scene-services-tests [existing-owned-parent-directory]");
        services_preserve_state_and_source_metadata(argc == 2 ? fs::absolute(argv[1]) : fs::temp_directory_path());
        services_lifecycle_composes_delete_paths(argc == 2 ? fs::absolute(argv[1]) : fs::temp_directory_path());
        services_source_owners(argc == 2 ? fs::absolute(argv[1]) : fs::temp_directory_path());
        services_reject_unregistered_sender(argc == 2 ? fs::absolute(argv[1]) : fs::temp_directory_path());
    } catch (const std::exception& error) { std::cerr << "FAIL: " << error.what() << '\n'; return EXIT_FAILURE; }
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
