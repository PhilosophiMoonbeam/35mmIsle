// Opt-in ROM-backed NativeHost error/retirement regression, not a Manager route
// or an earned-progress, course-completion, display/device or Windows proof.
#include "tunnel_scene.hpp"
#include "application_manager.hpp"
#include "application_photos.hpp"
#include "menu_resources.hpp"
#include "player_source_fixture.h"
#include "actual_audio_driver.hpp"
#include <chrono>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

namespace {
using namespace isle;
void require(bool condition,const char* message) {
    if (!condition) throw std::runtime_error(message);
}
void native(IsleGameResult result,const char* message) { require(result==ISLE_GAME_OK,message); }
void audio_ok(audio::Error result,const char* message) { require(result==audio::Error::none,message); }
void run(const std::filesystem::path& rom_path,const std::filesystem::path& profile) {
    require(std::filesystem::is_directory(profile) && !std::filesystem::is_symlink(profile) &&
        std::filesystem::is_empty(profile),"Host failure profile must be a fresh empty directory");
    const auto snapshot=rom::read_snapshot(rom_path);
    auto catalog=menu::hydrate(snapshot); require(bool(catalog),"Host failure catalog hydration failed");
    auto hydrated=game_audio::hydrate(snapshot); require(bool(hydrated),"Host failure audio hydration failed");
    std::array<audio::effects::Section,8> sections; audio::effects::Config effects;
    audio_ok(game_audio::native_startup_effects(hydrated.resources,sections,effects),"Host failure audio effects admission failed");
    private_audio::Capture pcm; private_audio::Execution execution{snapshot,nullptr,{}};
    auto made_audio=game_audio::Session::create(std::move(hydrated.resources),effects,pcm.output(),execution.executor());
    require(bool(made_audio),"Host failure actual Audio Session creation failed");
    auto& audio_session=*made_audio.session;
    game::State state(profile); require(state.initialize(1),"Host failure fresh State initialization failed");
    game_input::Service input; game_input::HostFrame ports{};
    ports[0].connection=game_input::Connection::present; ports[0].identity=1;
    require(input.start(ports)==game_input::Error::none,"Host failure software Input initialization failed");
    state.select_controller(input.snapshot());
    host::GtlClock clock({1,1,2,true,0});
    auto made_manager=application::Manager::create(snapshot,state,input,audio_session,clock,*catalog.catalog);
    require(bool(made_manager),made_manager.message.data());
    auto& manager=*made_manager.manager;
    application::Photos photos(snapshot,*state.save()); world::ModuleState module;
    auto made=tunnel::Lifecycle::create(snapshot,manager,state,input,audio_session,clock,photos,module,0,true,
        course::EntryNativeProfile::hosted_canonical_image);
    require(bool(made),made.message.data());
    auto& entry=*made.lifecycle;
    native(entry.resume(),entry.message());
    require(entry.pending_setup()!=nullptr,"Typed Tunnel entry did not request its real SceneSetup");
    native(entry.initialize_scene(),entry.message());
    auto* host=entry.native_host();
    require(entry.status()==course::LifecycleStatus::running && host && host->initialized() &&
        host->root() && host->root()->source().object,"Typed Host did not publish its actual initialized Player");
    const auto& source=host->root()->source();
    require(source.status==ISLE_GAME_OK && !source.failure[0] && host->error()==ISLE_GAME_OK,
        "Host fault fixture entered an already failed Source or component");
    IsleGameProcess* process=nullptr;
    native(isle_game_add_source_process(host->game(),source.object,player_source_host_failure,
        ISLE_GAME_SYNCHRONOUS,9,&process),"Actual Player checked-failure process registration failed");
    const auto updated=entry.update(1);
    const std::string diagnostic=host->message();
    require(updated==ISLE_GAME_EXECUTION_FAILURE && entry.error()==updated && host->error()==updated &&
        entry.status()==course::LifecycleStatus::failed && source.status==ISLE_GAME_INVALID_ARGUMENT &&
        isle_game_status(host->game())==ISLE_GAME_EXECUTION_FAILURE && !source.nonwaiting_call_frame &&
        diagnostic.find("player_source_host_failure")!=std::string::npos &&
        diagnostic.find("isle_game_object_metadata")!=std::string::npos && entry.message()==diagnostic &&
        !entry.result(),"Real typed update lost the checked C cause or fabricated a normal course result");
    // Production cancellation permits pending retirement. Service its actual
    // poll/join boundary; a single false service step is not a failed join.
    const auto cancelled=entry.cancel();
    require(cancelled==ISLE_GAME_OK || cancelled==ISLE_GAME_INVALID_STATE,
        "Failed typed Host rejected its cancellation request");
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
    while (!entry.scene_join_receipt().joined() && std::chrono::steady_clock::now()<deadline) {
        (void)entry.poll();
        if (entry.retire_scene()) break;
        std::this_thread::yield();
    }
    require(entry.entry_joined() && entry.scene_join_receipt().joined() && host->retired() &&
        !host->game() && !host->root() && !host->renderer() && host->error()==updated &&
        diagnostic==host->message() && diagnostic==entry.message() && !entry.result(),
        "Host retirement retained native storage or replaced its first diagnostic/result");
    native(entry.retire(),"Failed typed lifecycle retirement rejected");
    require(entry.status()==course::LifecycleStatus::retired && !entry.native_host() &&
        entry.entry_joined() && !entry.result(),"Retired typed lifecycle retained its Host or manufactured success");
    native(manager.retire(),"Independent Manager did not retire after its typed borrower");
    require(input.stop().error==game_input::Error::none,"Host failure Input did not stop");
    audio_ok(audio_session.stop(),"Host failure Audio stop request failed");
    std::uint64_t tick=0; unsigned tasks=0;
    for (unsigned n=0;n<240 && audio_session.state()!=game_audio::Session::State::draining;++n) {
        ++tick; audio_ok(audio_session.retrace(tick,tick*1000000000ull/60),"Host failure Audio retrace failed");
        audio_ok(audio_session.submit(),"Host failure owned PCM acceptance failed");
        audio::TaskView task; audio_ok(audio_session.build(task),"Host failure actual Audio task build failed");
        if (task.token.serial) { private_audio::execute(audio_session,execution,task); ++tasks; }
    }
    require(audio_session.state()==game_audio::Session::State::draining &&
        audio_session.service().voices_retired(),"Host failure Audio graph did not retire");
    audio_ok(audio_session.poll_drain(),"Host failure PCM first drain failed");
    audio_ok(audio_session.poll_drain(),"Host failure PCM final drain failed");
    require(audio_session.state()==game_audio::Session::State::drained &&
        !audio_session.service().has_lease() && audio_session.accepted_frames()*2==pcm.used,
        "Host failure Audio retirement lost its actual output ownership");
    std::cout<<"typed_Tunnel_NativeHost_checked_C_failure=passed joined_retirement=passed first_error="
        <<diagnostic<<" actual_audio_RSP_tasks="<<tasks
        <<" Manager_route_earned_progress_course_completion_display_Original_Windows11=not_claimed\n";
}
}
int main(int argc,char** argv) {
    if (argc!=3) { std::cerr<<"usage: native-host-failure-rom-smoke PRIVATE_ROM EMPTY_PRIVATE_PROFILE\n"; return 1; }
    try { run(argv[1],argv[2]); return 0; }
    catch (const std::exception& error) { std::cerr<<"native-host-failure-rom-smoke: "<<error.what()<<'\n'; return 1; }
}
