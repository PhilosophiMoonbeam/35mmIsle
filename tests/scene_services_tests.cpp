#include "scene_services.hpp"
#include "scene_storage.hpp"
#include "audio_fixture.hpp"

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
} // namespace
int main(int argc, char** argv) {
    try {
        if (argc > 2) throw std::runtime_error("usage: scene-services-tests [existing-owned-parent-directory]");
        services_preserve_state_and_source_metadata(argc == 2 ? fs::absolute(argv[1]) : fs::temp_directory_path());
    } catch (const std::exception& error) { std::cerr << "FAIL: " << error.what() << '\n'; return EXIT_FAILURE; }
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
