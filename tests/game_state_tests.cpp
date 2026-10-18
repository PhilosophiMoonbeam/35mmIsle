#include "game_state.hpp"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace fs = std::filesystem;
namespace s = isle::save;
namespace {
int failures = 0;
fs::path parent;
void check(bool condition, std::string_view meaning) {
    if (!condition) { ++failures; std::cerr << "FAIL: " << meaning << '\n'; }
}
struct Profile {
    fs::path path;
    Profile() {
        static unsigned sequence = 0;
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        for (unsigned i = 0; i < 100; ++i) {
            auto candidate = parent / ("35mmisle-state-" + std::to_string(stamp) + "-" + std::to_string(++sequence));
            if (fs::create_directory(candidate)) { path = std::move(candidate); return; }
        }
        throw std::runtime_error("cannot create owned State test profile");
    }
    ~Profile() { std::error_code error; fs::remove_all(path,error); }
};
void delayed_open_and_persistent_owner() {
    Profile profile;
    isle::game::State state(profile.path,0x12345678);
    check(!state.initialization_attempted() && !state.initialized() && !state.save_initialization() &&
          !state.save() && fs::is_empty(profile.path), "State construction never opens or locks the profile");
    { // Real competing Store proves construction has not acquired ownership.
        auto before = s::GameSession::open(profile.path,1);
        check(bool(before),"profile remains independently openable before deferred initialization");
    }
    check(state.initialize(0x10203040),"explicit deferred initialization opens the actual profile");
    if (!state.initialized()) return;
    auto* session = state.save();
    const auto* result = state.save_initialization();
    check(result && result->state == s::GameLoadState::initialized_missing && result->load.state == s::LoadState::missing,
          "State retains exact GameOpenResult rather than reconstructing a snapshot");
    check(!session->set_player_flag(16,true),"source RAM mutation succeeds without implicit commit");
    const auto first = session->snapshot().value;
    check(state.initialize(99) && state.save() == session && state.save_initialization() == result &&
          state.save()->snapshot().value.player_flag(16).value &&
          state.save()->snapshot().value.image().bytes().data() == first.image().bytes().data(),
          "repeated initialization does not reopen, reset, or snapshot-reconstruct the owner");
    auto contender = s::GameSession::open(profile.path,2);
    check(!contender && contender.store_error.code == s::StoreErrc::lock_busy && state.save() == session,
          "persistent application State retains the real Store lock");
    check(!fs::exists(profile.path / "save.a") && !fs::exists(profile.path / "save.b"),
          "deferred open and source RAM mutations never fabricate a durable commit");
}
void rejected_open_retains_evidence_and_lock() {
    Profile profile;
    const std::string original = "authored corrupt slot";
    { std::ofstream out(profile.path / "save.a",std::ios::binary); out << original; }
    {
        isle::game::State state(profile.path);
        check(!state.initialize(1) && state.initialization_attempted() && !state.save(),
              "rejected profile never becomes a writable session");
        const auto* evidence = state.save_initialization();
        check(evidence && evidence->state == s::GameLoadState::recovery_required &&
              evidence->load.state == s::LoadState::recovery_required && state.error()[0] != '\0',
              "actual checked rejection evidence remains available");
        check(!state.initialize(2) && state.save_initialization() == evidence,
              "rejected initialization is attempted once, never reopened or factory-reset");
        auto locked = s::GameSession::open(profile.path,3);
        check(!locked && locked.store_error.code == s::StoreErrc::lock_busy,
              "rejected State retains real Store lock while alive");
        std::ifstream in(profile.path / "save.a",std::ios::binary);
        const std::string retained{std::istreambuf_iterator<char>(in),std::istreambuf_iterator<char>()};
        check(retained == original && !fs::exists(profile.path / "save.b"),"rejection preserves original durable evidence");
    }
    auto released = s::GameSession::open(profile.path,4);
    check(!released && released.store_error.code != s::StoreErrc::lock_busy &&
          released.state == s::GameLoadState::recovery_required,"only application State destruction releases rejected profile ownership");
}
void source_notice_flags_controller_and_latches_are_distinct() {
    Profile profile;
    isle::game::State state(profile.path);
    state.title_start_flags = 0xa5;
    state.set_scene_latch(-7);
    state.add_scene_notices(1);
    state.add_scene_notices(0x80000004u);
    check(state.scene_notices() == 0x80000005u && state.consume_scene_notices(6) &&
          state.scene_notices() == 0x80000001u && !state.consume_scene_notices(6),
          "notice operation tests any bit then clears the complete supplied mask, preserving other bits");
    check(!state.consume_scene_notices(0) && state.scene_notices() == 0x80000001u,
          "zero-mask notice consumption preserves state");
    check(state.title_start_flags == 0xa5 && state.scene_latch() == -7,
          "32-bit notices never alias byte title flags or independent scene latch");
    state.clear_scene_notices();
    check(state.scene_notices() == 0 && state.title_start_flags == 0xa5 && state.scene_latch() == -7,
          "clear-all resets notices only");
    isle::game_input::Snapshot snapshot;
    snapshot.indices = {1,2,3,-1};
    state.select_controller(snapshot);
    check(state.controller == -1,"first healthy nonzero physical controller is not promoted to physical port zero");
    snapshot.indices = {0,1,2,3};
    state.select_controller(snapshot);
    check(state.controller == 0 && state.rng.seed == 1,"physical port zero selection and source default RNG seed");
    const auto random = isle_rng_next16(&state.rng);
    check(random == static_cast<std::uint16_t>((1u * 0x343fdu + 0x269ec3u) >> 16) &&
          state.rng.seed == 1u * 0x343fdu + 0x269ec3u,"application State owns the actual source RNG sequence");
}
void live_video_is_independent_of_profile_ram() {
    Profile profile;
    isle::game::State state(profile.path);
    const auto* const live = &state.screen_offsets();
    s::ScreenOffsets translation{99,99};
    check(live->horizontal_start == 0 && live->horizontal_end == 0 &&
          live->vertical_start == 0 && live->vertical_end == 0 &&
          live->translation(translation) && translation.horizontal == 0 && translation.vertical == 0,
          "actual initial VI is four zero endpoints before Boot opens the profile");
    check(state.initialize(1),"live video fixture opens the actual profile");
    if (!state.initialized()) return;
    check(!state.save()->set_screen_offsets(8,-6),"source settings fixture mutates RAM only");
    check(live->translation(translation) && translation.horizontal == 0 && translation.vertical == 0,
          "profile initialization/settings mutation never substitute for actual VI service publication");
    const auto saved = state.save()->snapshot().value;
    state.set_screen_offsets({8,8,-6,-6});
    const auto after_vi = state.save()->snapshot().value;
    check(&state.screen_offsets() == live && live->translation(translation) &&
          translation.horizontal == 8 && translation.vertical == -6 &&
          saved.image().bytes().data() == after_vi.image().bytes().data(),
          "Source VI publishes into one stable record without any settings mutation or snapshot copy");
    check(!state.save()->reset(2),"real new-game source reset succeeds");
    const auto reset = state.save()->snapshot().value.screen_offsets();
    check(reset && reset.value.horizontal == 0 && reset.value.vertical == 0 &&
          &state.screen_offsets() == live && live->translation(translation) &&
          translation.horizontal == 8 && translation.vertical == -6,
          "new-game RAM defaults do not reset live output at Name/Oak/Beach or later Title entries");
    state.set_screen_offsets({8,9,-6,-6});
    check(live->horizontal_start == 8 && live->horizontal_end == 9 &&
          !live->translation(translation) && translation.horizontal == 8 && translation.vertical == -6,
          "unequal horizontal endpoints remain canonical and fail translation without partial projection");
    state.set_screen_offsets({8,8,-6,-5});
    check(live->vertical_start == -6 && live->vertical_end == -5 &&
          !live->translation(translation) && translation.horizontal == 8 && translation.vertical == -6,
          "unequal vertical endpoints also fail explicitly instead of silently collapsing");
}
} // namespace
static_assert(!std::is_copy_constructible_v<isle::game::State> && !std::is_move_constructible_v<isle::game::State>);
int main(int argc, char** argv) {
    try {
        if (argc > 2) throw std::runtime_error("usage: game-state-tests [existing-owned-parent-directory]");
        parent = argc == 2 ? fs::absolute(argv[1]) : fs::temp_directory_path();
        delayed_open_and_persistent_owner();
        rejected_open_retains_evidence_and_lock();
        source_notice_flags_controller_and_latches_are_distinct();
        live_video_is_independent_of_profile_ram();
    } catch (const std::exception& error) { std::cerr << "FAIL: " << error.what() << '\n'; return EXIT_FAILURE; }
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
