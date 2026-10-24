#include "game_owner.hpp"
#include "game_host.hpp"
#include "menu_resources.hpp"
#include "rom_snapshot.hpp"
#include <filesystem>
#include <iostream>
#include <string_view>
#include <limits>
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
    // Host storage is declared before Owner: even exceptional unwinding keeps
    // the actual platform alive while Owner services retirement and joins.
    std::unique_ptr<isle::game_host::Host> platform;
    isle::game_owner::Owner owner(snapshot,*catalog.catalog,application_config,station_directory);
    std::filesystem::path profile;
    std::uint64_t application_epoch=1;
    for (;;) {
        auto made=isle::game_host::Host::create(snapshot,host_config);
        if (!made) { std::cerr<<made.message.data()<<'\n';return 1; }
        platform=std::move(made.host);
        auto& host=*platform;
        host_config.data_directory=host.data_directory();
        if (profile.empty()) {
            profile=host.data_directory()/"profile";
            std::filesystem::create_directory(profile);
        }
        if (!owner.start_epoch(host,profile,application_epoch)) {
            std::cerr<<"Actual GAME-owner epoch admission failed.\n";return 1;
        }
        while (!owner.epoch_retired()) owner.service();
        const auto result=owner.result();
        if (result.message[0]) std::cerr<<result.message.data()<<'\n';
        const bool reopen=(result.reset_requested||result.disconnect_requested)&&host.running();
        (void)host.cancel_present();
        const auto closed=host.shutdown();
        const bool successful=result.successful&&!closed.failed&&closed.owners_released&&
            closed.audio==isle::audio::SdlAudioOutput::CloseDisposition::stream_drained;
        if (host.error()!=isle::game_host::Error::none) std::cerr<<host.message()<<'\n';
        if (!successful||!reopen) {
            owner.finish();platform.reset();return successful?0:1;
        }
        platform.reset();
        if (application_epoch==std::numeric_limits<std::uint64_t>::max()) {
            std::cerr<<"Native application epoch exhausted.\n";return 1;
        }
        ++application_epoch;
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
