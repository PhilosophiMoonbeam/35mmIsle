#include "application_runtime.hpp"
#include "game_host.hpp"
#include "game_state.hpp"
#include "host_progress.hpp"
#include "menu_resources.hpp"
#include "rom_snapshot.hpp"
#include "post_course_replay.hpp"
#include "snap_station.hpp"
#include <chrono>
#include <filesystem>
#include <iostream>
#include <string_view>
#include <limits>
#include <thread>
#include <utility>

namespace {
#ifdef _WIN32
using Argument=wchar_t;
constexpr std::wstring_view help_option=L"--help",data_option=L"--data-directory";
constexpr std::wstring_view cpu_option=L"--cpu-renderer",input_option=L"--disable-input";
constexpr std::wstring_view station_option=L"--snap-station-directory";
constexpr std::wstring_view resources_option=L"--renderer-resources";
#else
using Argument=char;
constexpr std::string_view help_option="--help",data_option="--data-directory";
constexpr std::string_view cpu_option="--cpu-renderer",input_option="--disable-input";
constexpr std::string_view station_option="--snap-station-directory";
constexpr std::string_view resources_option="--renderer-resources";
#endif
void usage(std::ostream& out) {
    out<<"Usage: 35mmisle <ROM> [--data-directory <directory>] [--cpu-renderer] [--disable-input]\n"
         "                  [--snap-station-directory <directory>]\n"
         "                  [--renderer-resources <directory>]\n"
         "       35mmisle --help\n"
         "ROM must be the exact US Pokemon Snap image; z64/v64/n64 order is recognized from content.\n"
         "The ROM is read and normalized in memory, never modified or bundled.\n"
         "--cpu-renderer requires a CPU Vulkan device for canonical rendering.\n"
         "--disable-input disables controller acquisition for diagnostics; no controller is invented.\n"
         "--renderer-resources selects an explicit existing pinned shader-resource directory.\n"
         "--snap-station-directory configures an existing output directory; F9 connects/disconnects.\n";
}
void nap() { std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
struct NativeDisplay {
    isle::game_host::Host& host;
    isle::application::Runtime& application;
    bool& failed;
    static IsleGameResult service(void* raw) noexcept {
        auto& self=*static_cast<NativeDisplay*>(raw);
        if (const auto frame=self.application.completed_frame()) {
            if (self.host.error()==isle::game_host::Error::none) {
                // Retire an older optional display before the explicit cold
                // extent boundary. Frame capture does not borrow this copy.
                if ((self.host.presentation_pending()&&!self.host.cancel_present()) ||
                    !self.host.configure_present(frame.width,frame.height) ||
                    !self.host.queue_present(frame.rgba,frame.width,frame.height,frame.screen_offsets)) {
                    self.failed=true;self.application.request_stop();
                }
            }
            // Host copies or declines the optional image before the genuine
            // Runtime FrameToken terminal; no Source/OM/GTL advancement here.
            const auto released=self.application.release_completed();
            if (released!=ISLE_GAME_OK) { self.application.request_stop();return released; }
        }
        if (self.host.error()==isle::game_host::Error::none&&self.host.presentation_pending()&&
            self.host.try_present()==isle::game_host::Present::failed) {
            self.failed=true;self.application.request_stop();
        }
        return self.host.error()==isle::game_host::Error::none?ISLE_GAME_OK:ISLE_GAME_EXECUTION_FAILURE;
    }
};
void toggle_station(isle::game_host::Host& host,isle::game_input::Service& input,
                    isle::snap_station::Device& station,const std::filesystem::path& directory) {
    if (input.printer()) {
        input.bind_printer(nullptr);
        station.disconnect();
        (void)host.station_feedback("disconnected (F9 to connect)");
    } else if (directory.empty()) {
        (void)host.station_feedback("no output directory configured");
    } else if (station.connect(directory)) {
        input.bind_printer(&station);
        (void)host.station_feedback("connected (F9 to disconnect)");
    } else {
        // Failed acquisition never registers an endpoint with GAME.
        std::cerr<<"Snap Station connection failed: "<<station.message()<<'\n';
        (void)host.station_feedback(station.message());
    }
}
int run(int argc,Argument** argv) {
    if (argc==2&&std::basic_string_view<Argument>(argv[1])==help_option) { usage(std::cout);return std::cout?0:1; }
    if (argc<2) { usage(std::cerr);return 2; }
    isle::game_host::Config host_config;
    isle::application::RuntimeConfig application_config;
    std::filesystem::path station_directory;
    for (int index=2;index<argc;++index) {
        const std::basic_string_view<Argument> option(argv[index]);
        if (option==cpu_option) application_config.require_cpu_renderer=true;
        else if (option==input_option) host_config.input=isle::game_host::InputMode::diagnostic_disabled;
        else if (option==data_option&&index+1<argc) host_config.data_directory=std::filesystem::path(argv[++index]);
        else if (option==station_option&&index+1<argc) station_directory=std::filesystem::path(argv[++index]);
        else if (option==resources_option&&index+1<argc) {
            host_config.renderer_resources=std::filesystem::path(argv[++index]);
            if(host_config.renderer_resources.empty()) { usage(std::cerr);return 2; }
        }
        else { usage(std::cerr);return 2; }
    }
    const auto snapshot=isle::rom::read_snapshot(std::filesystem::path(argv[1]));
    auto catalog=isle::menu::hydrate(snapshot);
    if (!catalog) { std::cerr<<"Native application resources could not be admitted.\n";return 1; }
    // Device/job ownership is outside every real application/Host boot.
    isle::snap_station::Device station;
    std::filesystem::path profile;
    std::uint64_t application_epoch=1;
    bool completing_reset=false;
    for (;;) {
        auto platform=isle::game_host::Host::create(snapshot,host_config);
        if (!platform) { std::cerr<<platform.message.data()<<'\n';station.cancel();return 1; }
        auto& host=*platform.host;
        host_config.data_directory=host.data_directory();
        if (profile.empty()) {
            profile=host.data_directory()/"profile";
            std::filesystem::create_directory(profile);
        }
        (void)host.station_feedback(station.available()?"connected (F9 to disconnect)":"disconnected (F9 to connect)");
        isle::game_input::Service input;
        if (station.available()) input.bind_printer(&station);
        isle::host::GtlClock clock({1,1,2,true,0});
        isle::host::Progress progress(host,input);
        // Initialization consumes only an actual completed acquisition.
        isle::game_host::Input initial;
        do {
            if (!host.pump_events()) { station.cancel();return host.error()==isle::game_host::Error::none?0:1; }
            if (!completing_reset&&host.take_station_action())
                toggle_station(host,input,station,station_directory);
            initial=host.acquire_input();
            if (host.error()!=isle::game_host::Error::none) { std::cerr<<host.message()<<'\n';station.cancel();return 1; }
            if (!initial.completed) nap();
        } while (!initial.completed);
        if (input.start(initial.ports)!=isle::game_input::Error::none) {
            std::cerr<<"Actual controller initialization failed.\n";station.cancel();return 1;
        }
        bool platform_failed=false,reset_requested=false,disconnect_requested=false;
        IsleGameResult source_error=ISLE_GAME_OK;
        {
            // Reopening this same directory reacquires the real saved pair and
            // its lock. No Source entry/result is selected by native reset.
            isle::game::State state(profile);
            application_config.station=&station;
            application_config.application_epoch=application_epoch;
            auto made=isle::application::Runtime::create(snapshot,*catalog.catalog,state,input,*host.audio(),clock,application_config);
            if (!made) { std::cerr<<made.message.data()<<'\n';(void)input.stop();station.cancel();return 1; }
            {
                auto& application=*made.runtime;
                progress.bind(clock,{&application,[](void* owner,const isle::game_input::EventResult& completion) noexcept {
                    return static_cast<isle::application::Runtime*>(owner)->acknowledge_input(completion)==ISLE_GAME_OK;
                }});
                NativeDisplay display{host,application,platform_failed};
                if (application.bind_post_course_progress(progress,{&display,NativeDisplay::service})!=ISLE_GAME_OK)
                    application.request_stop();
                if (completing_reset) {
                    completing_reset=false;
                    if (!station.complete_reset(application_epoch)) {
                        std::cerr<<"Snap Station reset failed: "<<station.message()<<'\n';
                        platform_failed=true;application.request_stop();
                    }
                }
                auto native_actions=[&] {
                    // Only outer-owner code consumes 5A, after every returning
                    // Source/native leaf has unwound. Never reenter GAME here.
                    std::uint64_t ticket=0;
                    if (station.take_reset_request(ticket)) {
                        reset_requested=true;
                        application.request_stop();progress.request_stop();
                        (void)host.station_feedback("resetting after saved profile");
                    }
                    if (host.take_station_action()) {
                        const auto view=application.view();
                        const bool diagnostic=view.entry&&
                            view.entry->operation==ISLE_APPLICATION_ENTRY_PRINTER_DIAGNOSTIC;
                        if (diagnostic||reset_requested) {
                            // Even a failed capture retains its actual borrow.
                            // Disconnect only after full native retirement.
                            disconnect_requested=true;
                            application.request_stop();progress.request_stop();
                            (void)host.station_feedback("disconnecting after capture retirement");
                        } else toggle_station(host,input,station,station_directory);
                    }
                    if (input.printer()&&station.error()!=isle::snap_station::Error::none) {
                        std::cerr<<"Snap Station failed: "<<station.message()<<'\n';
                        (void)host.station_feedback(station.message());
                        platform_failed=true;
                        application.request_stop();progress.request_stop();
                    }
                };
                while (!application.stopped()) {
                    progress.set_policy({true,true,application.scene_active(),true,true});
                    const auto tick=progress.poll();
                    if (progress.status()!=isle::host::ProgressStatus::running) {
                        platform_failed|=progress.status()==isle::host::ProgressStatus::failed;
                        application.request_stop();
                    }
                    native_actions();
                    (void)application.pump(isle::host::Progress::raw(tick));
                    native_actions();
                    if (application.error()!=ISLE_GAME_OK) progress.request_stop();
                    (void)NativeDisplay::service(&display);
                    (void)progress.idle();
                }
                source_error=application.error();
                // These references must be dead before Runtime/Photos/State.
                progress.unbind();
                if (progress.status()==isle::host::ProgressStatus::failed && host.error()==isle::game_host::Error::none)
                    std::cerr<<progress.message()<<'\n';
                if (source_error!=ISLE_GAME_OK) std::cerr<<application.message()<<'\n';
            }
            made.runtime.reset();
        }
        (void)input.stop();
        input.bind_printer(nullptr);
        (void)host.cancel_present();
        const bool reopen=(reset_requested||disconnect_requested)&&host.running();
        if (disconnect_requested) station.disconnect();
        bool drained=false;
        if (host.error()==isle::game_host::Error::none&&host.stop_audio()) {
            // Accepted PCM receives its real stream terminal. Deadline failure
            // goes through the existing honest abort close, never fake drain.
            progress.set_deadline(std::chrono::steady_clock::now()+std::chrono::seconds(30),
                "actual audio shutdown did not drain");
            while (!progress.deadline_expired()) {
                progress.set_policy({false,false,false,false,false});
                (void)progress.poll();
                if (host.error()!=isle::game_host::Error::none) break;
                const auto audio_state=host.poll_audio_drain();
                if (audio_state==isle::game_host::Drain::failed) break;
                if (audio_state==isle::game_host::Drain::drained) { drained=true;break; }
                (void)progress.idle();
            }
        }
        if (host.error()!=isle::game_host::Error::none) std::cerr<<host.message()<<'\n';
        const auto closed=host.shutdown();
        const bool successful=source_error==ISLE_GAME_OK&&!platform_failed&&drained&&!closed.failed&&closed.owners_released&&
            closed.audio==isle::audio::SdlAudioOutput::CloseDisposition::stream_drained;
        // Host shutdown includes real Audio executor/PCM/window/cache retirement.
        platform.host.reset();
        if (!successful||!reopen) { station.cancel();return successful?0:1; }
        if (application_epoch==std::numeric_limits<std::uint64_t>::max()) {
            std::cerr<<"Native application epoch exhausted.\n";station.cancel();return 1;
        }
        ++application_epoch;
        completing_reset=reset_requested&&!disconnect_requested;
    }
}
} // namespace
#ifdef _WIN32
int wmain(int argc,wchar_t** argv) {
#else
int main(int argc,char** argv) {
#endif
    try { return run(argc,argv); }
    catch (const std::exception& error) { std::cerr<<"Native application startup failed: "<<error.what()<<'\n';return 1; }
    catch (...) { std::cerr<<"Native application startup failed.\n";return 1; }
}
