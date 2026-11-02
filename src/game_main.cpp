#include "game_owner.hpp"
#include "game_host.hpp"
#include "menu_resources.hpp"
#include "rom_snapshot.hpp"
#include <filesystem>
#include <iostream>
#include <string_view>
#include <limits>
#include <utility>
#include <chrono>
#include <cstdio>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

namespace {
#ifdef _WIN32
using Argument=wchar_t;
constexpr std::wstring_view help_option=L"--help",data_option=L"--data-directory";
constexpr std::wstring_view cpu_option=L"--cpu-renderer",input_option=L"--disable-input";
constexpr std::wstring_view station_option=L"--snap-station-directory";
constexpr std::wstring_view resources_option=L"--renderer-resources";
constexpr std::wstring_view report_option=L"--terminal-report";
constexpr std::wstring_view diagnostics_option=L"--diagnostics";
constexpr std::wstring_view campaign_purpose=L"campaign",cold_boot_purpose=L"cold-boot";
#else
using Argument=char;
constexpr std::string_view help_option="--help",data_option="--data-directory";
constexpr std::string_view cpu_option="--cpu-renderer",input_option="--disable-input";
constexpr std::string_view station_option="--snap-station-directory";
constexpr std::string_view resources_option="--renderer-resources";
constexpr std::string_view report_option="--terminal-report";
constexpr std::string_view diagnostics_option="--diagnostics";
constexpr std::string_view campaign_purpose="campaign",cold_boot_purpose="cold-boot";
#endif
void usage(std::ostream& out) {
    out<<"Usage: 35mmisle <ROM> [--data-directory <directory>] [--cpu-renderer] [--disable-input]\n"
         "                  [--snap-station-directory <directory>]\n"
         "                  [--renderer-resources <directory>]\n"
         "                  [--diagnostics]\n"
         "                  [--terminal-report <campaign|cold-boot> <directory>]\n"
         "       35mmisle --help\n"
         "ROM must be the exact US Pokemon Snap image; z64/v64/n64 order is recognized from content.\n"
         "The ROM is read and normalized in memory, never modified or bundled.\n"
         "--cpu-renderer requires a CPU Vulkan device for canonical rendering.\n"
         "--disable-input disables controller acquisition for diagnostics; no controller is invented.\n"
         "--renderer-resources selects an explicit existing pinned shader-resource directory.\n"
         "--snap-station-directory configures an existing output directory; F9 connects/disconnects.\n"
         "--diagnostics enables a passive on-screen run/scene/frame/error label and private text report.\n"
         "After closure, last-diagnostic.txt in the data directory replaces the previous diagnostic report.\n"
         "--terminal-report writes passive native evidence only after normal GAME/Host closure.\n"
         "Its purpose selects evidence acceptance, not scenes, input, progress or automatic stopping.\n"
         "Incomplete evidence or output failure returns nonzero after cleanup; external acceptance is separate.\n";
}
// One bounded report per data directory; reserve our temporary file exclusively,
// then atomically replace the old report. Never follow a pre-existing temp link.
bool write_diagnostics(const std::filesystem::path& directory,const char* report,int status) noexcept {
    try {
        const auto temporary=directory/"last-diagnostic.txt.tmp";
        const auto destination=directory/"last-diagnostic.txt";
#ifdef _WIN32
        auto* file=_wfopen(temporary.c_str(),L"wbx");
#else
        const int descriptor=::open(temporary.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);
        if (descriptor<0) return false;
        auto* file=::fdopen(descriptor,"wb");
        if (!file) {
            ::close(descriptor);
            std::error_code ignored;
            std::filesystem::remove(temporary,ignored);
            return false;
        }
#endif
        if (!file) return false;
        bool written=std::fprintf(file,
            "35mmisle.private-diagnostic.v1\n%s\nExit status: %d\n"
            "Passive observation only; image identity is the latest offered native frame.\n"
            "Save generation is the boot-loaded durable generation, not current RAM or a Save ACK ledger.\n"
            "Audio fidelity is UNQUALIFIED. Playback and these labels cannot settle the known\n"
            "short-attack envelope-rate discrepancy (.9.8.7); .35 tracks missing reference evidence.\n"
            "No automatic audible-issue detector is provided.\n"
            "No telemetry, upload, ROM bytes, profile inventory or private SDK state is included.\n"
            "For a report, share the run ID, source, epoch/entry/frame or GTL update, what happened,\n"
            "and the expected result. Review error text for private filesystem paths before sharing.\n",
            report,status)>=0;
        if (std::fclose(file)!=0) written=false;
#ifdef _WIN32
        if (written) written=MoveFileExW(temporary.c_str(),destination.c_str(),
                                       MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
#else
        if (written) {
            std::error_code error;
            std::filesystem::rename(temporary,destination,error);
            written=!error;
        }
#endif
        if (!written) { std::error_code ignored;std::filesystem::remove(temporary,ignored); }
        return written;
    } catch (...) { return false; }
}
int run(int argc,Argument** argv) {
    if (argc==2&&std::basic_string_view<Argument>(argv[1])==help_option) { usage(std::cout);return std::cout?0:1; }
    if (argc<2) { usage(std::cerr);return 2; }
    isle::game_host::Config host_config;
    isle::application::RuntimeConfig application_config;
    std::filesystem::path station_directory;
    auto report_purpose=isle::game_run_report::Purpose::disabled;
    std::filesystem::path report_directory;
    bool diagnostics=false;
    for (int index=2;index<argc;++index) {
        const std::basic_string_view<Argument> option(argv[index]);
        if (option==cpu_option) application_config.require_cpu_renderer=true;
        else if (option==input_option) host_config.input=isle::game_host::InputMode::diagnostic_disabled;
        else if (option==diagnostics_option) diagnostics=true;
        else if (option==data_option&&index+1<argc) host_config.data_directory=std::filesystem::path(argv[++index]);
        else if (option==station_option&&index+1<argc) station_directory=std::filesystem::path(argv[++index]);
        else if (option==resources_option&&index+1<argc) {
            host_config.renderer_resources=std::filesystem::path(argv[++index]);
            if(host_config.renderer_resources.empty()) { usage(std::cerr);return 2; }
        }
        else if (option==report_option&&index+2<argc) {
            const std::basic_string_view<Argument> purpose(argv[++index]);
            if (purpose==campaign_purpose) report_purpose=isle::game_run_report::Purpose::campaign;
            else if (purpose==cold_boot_purpose) report_purpose=isle::game_run_report::Purpose::cold_boot;
            else { usage(std::cerr);return 2; }
            report_directory=std::filesystem::path(argv[++index]);
            if (report_directory.empty()) { usage(std::cerr);return 2; }
        }
        else { usage(std::cerr);return 2; }
    }
    const auto snapshot=isle::rom::read_snapshot(std::filesystem::path(argv[1]));
    auto catalog=isle::menu::hydrate(snapshot);
    if (!catalog) { std::cerr<<"Native application resources could not be admitted.\n";return 1; }
    // Host storage is declared before Owner: even exceptional unwinding keeps
    // the actual platform alive while Owner services retirement and joins.
    std::unique_ptr<isle::game_host::Host> platform;
    std::array<char,33> run_id{};
    if (diagnostics) {
        // Correlation only: never sample or seed the GAME RNG, or count GUI frames.
        const auto wall=std::chrono::system_clock::now().time_since_epoch().count();
        const auto monotonic=std::chrono::steady_clock::now().time_since_epoch().count();
        std::snprintf(run_id.data(),run_id.size(),"%016llx%016llx",
            static_cast<unsigned long long>(wall),static_cast<unsigned long long>(monotonic));
        std::cerr<<"Diagnostic run "<<run_id.data()<<" | configured source "<<ISLE_BUILD_REVISION<<'\n';
    }
    isle::game_owner::Owner owner(snapshot,*catalog.catalog,application_config,station_directory,report_purpose,
        diagnostics?isle::game_owner::Diagnostics{ISLE_BUILD_VERSION,ISLE_BUILD_REVISION,run_id.data()}:
                    isle::game_owner::Diagnostics{});
    std::filesystem::path profile;
    std::uint64_t application_epoch=1;
    bool epoch_admitted=false,host_closed=false;
    const auto close_host=[&] {
        const bool cancelled=platform->cancel_present();
        const auto closed=platform->shutdown();
        if (epoch_admitted) owner.observe_host_shutdown(application_epoch,cancelled,closed);
        epoch_admitted=false;host_closed=true;
        return closed;
    };
    const auto finish=[&](int status) {
        // Keep the platform serviceable through the actual worker join, even
        // when an exception interrupted an epoch before its normal shutdown.
        owner.finish();
        if (platform&&!host_closed) {
            const auto closed=close_host();
            if (closed.failed||!closed.owners_released||
                closed.audio!=isle::audio::SdlAudioOutput::CloseDisposition::stream_drained) status=1;
        }
        platform.reset();
        if (report_purpose!=isle::game_run_report::Purpose::disabled) {
            const auto* evidence=owner.terminal_evidence();
            if (!evidence) {
                owner.diagnostic_failure("Terminal native evidence is unavailable after closure");
                std::cerr<<"Terminal native evidence is unavailable after closure.\n";status=1;
            } else {
                std::array<char,512> error{};
                const bool written=evidence->write(report_directory,error);
                if (!written) {
                    owner.diagnostic_failure(error.data());
                    std::cerr<<"Terminal native report failed: "<<error.data()<<'\n';
                }
                const bool complete=evidence->native_complete();
                if (!complete) {
                    owner.diagnostic_failure(evidence->message());
                    std::cerr<<"Terminal native evidence is incomplete: "<<evidence->message()<<'\n';
                }
                if (!written||!complete) status=1;
            }
        }
        if (diagnostics) {
            const auto* report=owner.diagnostic_report();
            if (host_config.data_directory.empty()||
                !write_diagnostics(host_config.data_directory,report,status)) {
                owner.diagnostic_failure("Private diagnostic report could not be replaced");
                std::cerr<<owner.diagnostic_report()<<'\n'
                         <<"Private diagnostic report could not be replaced; retained label text is above.\n";
                status=1;
            } else std::cerr<<report<<'\n'<<"Private diagnostic report: "
                           <<host_config.data_directory/"last-diagnostic.txt"<<'\n';
        }
        return status;
    };
    try {
        for (;;) {
            auto made=isle::game_host::Host::create(snapshot,host_config);
            if (!made) {
                owner.diagnostic_failure(made.message.data());
                std::cerr<<made.message.data()<<'\n';return finish(1);
            }
            platform=std::move(made.host);host_closed=false;
            auto& host=*platform;
            host_config.data_directory=host.data_directory();
            if (profile.empty()) {
                profile=host.data_directory()/"profile";
                std::filesystem::create_directory(profile);
            }
            if (!owner.start_epoch(host,profile,application_epoch)) {
                owner.diagnostic_failure("Actual GAME-owner epoch admission failed");
                std::cerr<<"Actual GAME-owner epoch admission failed.\n";return finish(1);
            }
            epoch_admitted=true;
            while (!owner.epoch_retired()) owner.service();
            const auto result=owner.result();
            if (result.message[0]) std::cerr<<result.message.data()<<'\n';
            const bool reopen=(result.reset_requested||result.disconnect_requested)&&host.running();
            const auto closed=close_host();
            const bool successful=result.successful&&!closed.failed&&closed.owners_released&&
                closed.audio==isle::audio::SdlAudioOutput::CloseDisposition::stream_drained;
            if (host.error()!=isle::game_host::Error::none) std::cerr<<host.message()<<'\n';
            if (!successful||!reopen) return finish(successful?0:1);
            platform.reset();
            if (application_epoch==std::numeric_limits<std::uint64_t>::max()) {
                owner.diagnostic_failure("Native application epoch exhausted");
                std::cerr<<"Native application epoch exhausted.\n";return finish(1);
            }
            ++application_epoch;
        }
    } catch (const std::exception& error) {
        owner.diagnostic_failure(error.what());
        std::cerr<<"Native application startup failed: "<<error.what()<<'\n';return finish(1);
    } catch (...) {
        owner.diagnostic_failure("Native application startup failed");
        std::cerr<<"Native application startup failed.\n";return finish(1);
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
