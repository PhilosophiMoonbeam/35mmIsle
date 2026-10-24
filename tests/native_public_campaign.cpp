#include "native_public_campaign.hpp"
#include "application_runtime.hpp"
#include "game_audio_driver.hpp"
#include "game_host.hpp"
#include "host_progress.hpp"
#include "post_course_scene.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace isle::native_public_campaign {
namespace {
namespace app = application;
namespace g = game_host;
namespace h = host;
namespace in = game_input;
using Time = std::chrono::steady_clock;
constexpr std::array courses{ISLE_APPLICATION_ENTRY_BEACH, ISLE_APPLICATION_ENTRY_TUNNEL,
    ISLE_APPLICATION_ENTRY_VOLCANO, ISLE_APPLICATION_ENTRY_RIVER, ISLE_APPLICATION_ENTRY_CAVE,
    ISLE_APPLICATION_ENTRY_VALLEY, ISLE_APPLICATION_ENTRY_RAINBOW};
constexpr std::size_t operation_count = ISLE_APPLICATION_ENTRY_PRINTER_DIAGNOSTIC + 1;
constexpr std::uint64_t maximum_boundary_rows = 65536;
constexpr std::uint64_t final_title_presentations = 120;
enum class Purpose { campaign, replay_stop };
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message && message[0] ? message : "actual fresh campaign failure");
}
void remember(std::string& failure, const char* message) {
    if (failure.empty()) {
        failure = message && message[0] ? message : "actual fresh campaign failure";
        std::cerr << "fresh public campaign: " << failure << '\n';
    }
}
// No optional scene pointer is inspected. Only actual Native entry/result
// scalars and the immutable, already durable publication cross this boundary.
struct Evidence {
    std::ofstream events;
    std::uint64_t rows = 0, last_serial = 0, last_retired = 0;
    app::EntryIntent current{}, displayed{};
    std::array<std::uint64_t,operation_count> entries{}, retirements{}, presentations{};
    std::uint64_t current_presentations = 0, frames = 0, presented = 0, declined = 0;
    std::uint64_t input_requests = 0, input_completions = 0, save_acks = 0;
    std::uint64_t credits18_serial = 0, credits18_retired = 0, final_title_serial = 0;
    std::uint64_t save_serial = 0, submissions = 0, terminals = 0, pcm = 0;
    std::uint32_t course_mask = 0;
    bool boot = false, title = false, name = false, oak = false;
    bool terminal_save = false, reached = false, runtime_retired = false, input_stopped = false;
    bool audio_drained = false, host_released = false, cold_equal = false;
    bool observer_failed = false;
    std::array<char,256> observer_message{};
    save::GameSnapshot published_save;
    app::EntryIntent preview_stop_entry{};
    bool preview_stop_requested = false, preview_stop_checked = false, preview_stop_result_emitted = false;

    void fail(const char* message) noexcept {
        observer_failed = true;
        if (!observer_message[0]) std::snprintf(observer_message.data(),observer_message.size(),"%s",message);
    }
    void row(const char* kind, const app::EntryIntent& entry, std::int32_t result = 0,
             std::uint64_t generation = 0) noexcept {
        if (rows >= maximum_boundary_rows) { fail("bounded Native boundary evidence exhausted"); return; }
        events << kind << '\t' << entry.serial << '\t' << static_cast<unsigned>(entry.operation)
               << '\t' << entry.argument << '\t' << result << '\t' << generation << '\n';
        events.flush();
        ++rows;
        if (!events) fail("Native boundary evidence write failed");
    }
    void entry(const app::EntryIntent* borrowed) noexcept {
        if (!borrowed || borrowed->serial == last_serial) return;
        const auto value = *borrowed;
        if (value.serial <= last_serial || static_cast<std::size_t>(value.operation) >= operation_count) {
            fail("actual Native entry serial/operation invalid"); return;
        }
        current = value;
        last_serial = value.serial;
        current_presentations = 0;
        ++entries[static_cast<std::size_t>(value.operation)];
        row("entry",value);
        if (value.operation == ISLE_APPLICATION_ENTRY_CREDITS && value.argument == 18)
            credits18_serial = value.serial;
        if (value.operation == ISLE_APPLICATION_ENTRY_TITLE && value.argument == 8 &&
            credits18_retired && value.serial > credits18_retired)
            final_title_serial = value.serial;
    }
    static bool terminal(const save::GameSnapshot& snapshot) noexcept {
        const auto unlocked = snapshot.unlocked_course();
        if (!unlocked || unlocked.value != 6) return false;
        for (const auto flag : {0u,1u,2u,5u}) {
            const auto value = snapshot.player_flag(flag);
            if (!value || !value.value) return false;
        }
        // Public canonical Report slots: Mew62 and all six signs63..68.
        for (std::size_t slot = 62; slot < save::game_report_count; ++slot) {
            const auto value = snapshot.photo_present(save::PhotoSet::report,slot);
            if (!value || !value.value) return false;
        }
        return true;
    }
    static void retired(void* owner, app::SceneRetirementView view) noexcept {
        auto& evidence = *static_cast<Evidence*>(owner);
        if (!view.entry) { evidence.fail("Native retirement omitted entry identity"); return; }
        evidence.entry(view.entry);
        const auto entry = *view.entry;
        if (evidence.preview_stop_requested && entry.serial == evidence.preview_stop_entry.serial) {
            evidence.preview_stop_result_emitted = true;
            evidence.fail("cancelled preview Entry emitted a successful retirement/result"); return;
        }
        if (entry.serial <= evidence.last_retired || entry.serial != evidence.current.serial) {
            evidence.fail("Native retirement serial does not match its actual entry"); return;
        }
        evidence.last_retired = entry.serial;
        const auto op = static_cast<std::size_t>(entry.operation);
        if (op >= operation_count) { evidence.fail("Native retirement operation invalid"); return; }
        ++evidence.retirements[op];
        evidence.row("retired",entry,view.source_result);
        const bool shown = evidence.current_presentations != 0;
        if (entry.operation == ISLE_APPLICATION_ENTRY_BOOT && view.source_result == 0 && shown)
            evidence.boot = true;
        if (entry.operation == ISLE_APPLICATION_ENTRY_TITLE && entry.argument == 8 && shown)
            evidence.title = true;
        if (entry.operation == ISLE_APPLICATION_ENTRY_NAME && entry.argument == 0 &&
            view.source_result == 11 && shown && evidence.boot && evidence.title)
            evidence.name = true;
        if (entry.operation == ISLE_APPLICATION_ENTRY_OAKS_LAB && shown && evidence.name)
            evidence.oak = true;
        for (std::size_t course = 0; course < courses.size(); ++course) {
            if (entry.operation == courses[course] && entry.argument == course && shown &&
                (view.source_result == 9 || view.source_result == 13))
                evidence.course_mask |= std::uint32_t{1} << course;
        }
        if (entry.operation == ISLE_APPLICATION_ENTRY_CREDITS && entry.argument == 18 &&
            entry.serial == evidence.credits18_serial && view.source_result == 8 && shown)
            evidence.credits18_retired = entry.serial;
        if (view.published_save) {
            if (entry.operation != ISLE_APPLICATION_ENTRY_OAKS_LAB || !view.published_save.generation() ||
                (evidence.published_save &&
                 view.published_save.generation() <= evidence.published_save.generation())) {
                evidence.fail("durable Save ACK identity/generation invalid"); return;
            }
            evidence.published_save = std::move(view.published_save);
            evidence.save_serial = entry.serial;
            ++evidence.save_acks;
            evidence.terminal_save = terminal(evidence.published_save);
            evidence.row("save_ack",entry,view.source_result,evidence.published_save.generation());
        }
    }
    bool complete() const noexcept {
        return !observer_failed && boot && title && name && oak && course_mask == 0x7fu &&
            terminal_save && credits18_retired && final_title_serial &&
            current.serial == final_title_serial && current_presentations >= final_title_presentations;
    }
};
// This adapter records only real read-token admission and actual ACK success.
// Every acquisition still calls the direct HostControlDeck platform endpoint;
// nested post-course servicing uses this same ledger, with no input override.
struct HostBoundary {
    h::PlatformProgress native;
    Evidence& evidence;
    in::ReadToken last_read{};
    app::Runtime* runtime = nullptr;
    h::PlatformProgress platform() noexcept {
        return {this,
            [](void* owner) noexcept {
                auto& boundary = *static_cast<HostBoundary*>(owner);
                return boundary.native.poll_events(boundary.native.owner);
            },
            [](void* owner, in::ReadToken token) noexcept {
                auto& boundary = *static_cast<HostBoundary*>(owner);
                if (token != boundary.last_read) {
                    boundary.last_read = token;
                    ++boundary.evidence.input_requests;
                }
                return boundary.native.read(boundary.native.owner,token);
            },
            [](void* owner) noexcept {
                auto& boundary = *static_cast<HostBoundary*>(owner);
                return boundary.native.error(boundary.native.owner);
            },
            [](void* owner) noexcept {
                auto& boundary = *static_cast<HostBoundary*>(owner);
                return boundary.native.message(boundary.native.owner);
            }};
    }
    static bool acknowledge(void* owner, const in::EventResult& completed) noexcept {
        auto& boundary = *static_cast<HostBoundary*>(owner);
        const bool accepted = boundary.runtime &&
            boundary.runtime->acknowledge_input(completed) == ISLE_GAME_OK;
        if (accepted) ++boundary.evidence.input_completions;
        return accepted;
    }
};
struct Display {
    g::Host& host;
    app::Runtime& runtime;
    Evidence& evidence;
    h::Progress& progress;
    Purpose purpose;
    IsleGameResult service(bool nested_replay_boundary) noexcept {
        // The public entry identity is metadata only, never input feedback.
        const auto view = runtime.view();
        evidence.entry(view.entry);
        if (nested_replay_boundary && purpose == Purpose::replay_stop && !evidence.preview_stop_requested &&
            progress.status() == h::ProgressStatus::running && view.entry && view.post &&
            ((view.entry->operation == ISLE_APPLICATION_ENTRY_CAMERA_CHECK && view.entry->argument == 9) ||
             (view.entry->operation == ISLE_APPLICATION_ENTRY_PHOTO_CHECK && view.entry->argument == 15)) &&
            view.post->replay_preview_submission_pending()) {
            evidence.preview_stop_entry = *view.entry;
            evidence.preview_stop_requested = true;
            evidence.row("preview_stop_requested",evidence.preview_stop_entry);
            // Ordinary Host stop at an actual submitted/unretired Native command.
            // The active C preview driver, not this observer, cancels and drains.
            progress.request_stop();
        }
        const auto frame = runtime.completed_frame();
        IsleGameResult failure = ISLE_GAME_OK;
        if (frame) {
            if (!frame.ordinal || frame.stride != frame.width * 4u ||
                frame.rgba.size() != std::size_t(frame.stride) * frame.height) {
                evidence.fail("actual Runtime completed frame extent invalid");
                failure = ISLE_GAME_EXECUTION_FAILURE;
            } else if (!host.presentation_pending()) {
                if (!host.queue_present(frame.rgba,frame.width,frame.height,frame.screen_offsets)) {
                    evidence.fail(host.message());
                    failure = ISLE_GAME_EXECUTION_FAILURE;
                } else evidence.displayed = evidence.current;
            } else ++evidence.declined;
            const auto released = runtime.release_completed();
            if (released != ISLE_GAME_OK) { evidence.fail(runtime.message()); failure = released; }
            else ++evidence.frames;
        }
        if (host.presentation_pending()) {
            const auto result = host.try_present();
            if (result == g::Present::failed) {
                evidence.fail(host.message()); failure = ISLE_GAME_EXECUTION_FAILURE;
            } else if (result == g::Present::completed) {
                ++evidence.presented;
                const auto op = static_cast<std::size_t>(evidence.displayed.operation);
                if (op < operation_count) ++evidence.presentations[op];
                if (evidence.displayed.serial == evidence.current.serial) {
                    if (!evidence.current_presentations++) evidence.row("present",evidence.displayed);
                }
            }
        }
        return failure;
    }
};
void report(const std::filesystem::path& root, const Evidence& evidence, Purpose purpose, const std::string& failure) {
    std::ofstream out(root/"proof"/"native-public-campaign-report.txt",std::ios::trunc);
    require(bool(out),"fresh campaign report open failed");
    out << "schema=native-public-campaign-1\nmode="
        << (purpose == Purpose::campaign ? "fresh-public-control-deck" : "public-control-deck-replay-stop")
        << "\ninput_acquisition=actual_HostControlDeck\ncontroller_policy=external_pixel_only_required\n"
        << "save_output=one_way_immutable_durable_ACK\nwall_limit_seconds=86400\n"
        << "native_boundary_qualified=" << (purpose == Purpose::campaign && failure.empty() && evidence.reached &&
            evidence.cold_equal && evidence.runtime_retired && evidence.input_stopped && evidence.audio_drained && evidence.host_released) << '\n'
        << "replay_stop_boundary_qualified=" << (purpose == Purpose::replay_stop && failure.empty() &&
            evidence.preview_stop_requested && evidence.preview_stop_checked && !evidence.preview_stop_result_emitted &&
            evidence.runtime_retired && evidence.input_stopped && evidence.audio_drained && evidence.host_released) << '\n'
        << "replay_stop_scope=submitted_unretired_preview_cancellation_NOT_campaign_or_hardware_busy\n"
        << "preview_stop_requested_in_nested_native_service=" << evidence.preview_stop_requested
        << "\npreview_stop_entry_serial=" << evidence.preview_stop_entry.serial
        << "\npreview_stop_entry_operation=" << static_cast<unsigned>(evidence.preview_stop_entry.operation)
        << "\npreview_stop_entry_argument=" << evidence.preview_stop_entry.argument
        << "\npreview_stop_runtime_stopped_without_fault=" << evidence.preview_stop_checked
        << "\npreview_stop_entry_result_emitted=" << evidence.preview_stop_result_emitted << '\n'
        << "qualification=independent_pixel_custody_review_required\n"
        << "failure=" << failure << "\nboot_retired=" << evidence.boot << "\ninitial_title_retired=" << evidence.title
        << "\nName_retired=" << evidence.name << "\nOak_retired=" << evidence.oak
        << "\ncompleted_course_mask=" << evidence.course_mask << "\nterminal_four_gear_six_signs_Mew=" << evidence.terminal_save
        << "\ncredits18_entry_serial=" << evidence.credits18_serial << "\ncredits18_joined_result8_serial=" << evidence.credits18_retired
        << "\nfinal_Title8_entry_serial=" << evidence.final_title_serial
        << "\nfinal_Title_presentations=" << (evidence.final_title_serial &&
            evidence.current.serial == evidence.final_title_serial ? evidence.current_presentations : 0)
        << "\nfinal_Title_visibility=independent_X11_pixel_evidence_required\n"
        << "save_ACKs=" << evidence.save_acks << "\nlast_save_serial=" << evidence.save_serial
        << "\nacknowledged_generation=" << evidence.published_save.generation()
        << "\ncold_GameSession_whole_canonical_equal=" << evidence.cold_equal
        << "\ncold_reopen_kind=GameSession_not_second_executable_Boot\n"
        << "Runtime_retired=" << evidence.runtime_retired << "\nInput_stopped=" << evidence.input_stopped
        << "\nAudio_drained=" << evidence.audio_drained << "\nHost_owners_released=" << evidence.host_released
        << "\nreleased_completed_frames=" << evidence.frames << "\npresentations=" << evidence.presented
        << "\noptional_present_declines=" << evidence.declined << "\nGTL_submissions=" << evidence.submissions
        << "\nGTL_terminals=" << evidence.terminals << "\ninput_requests=" << evidence.input_requests
        << "\ninput_completions=" << evidence.input_completions << "\naccepted_PCM_frames=" << evidence.pcm
        << "\nboundary_rows=" << evidence.rows << "\nnormal_exit0=" << failure.empty() << '\n';
    for (std::size_t op = 0; op < operation_count; ++op)
        out << "operation_" << op << "_entries=" << evidence.entries[op] << "\noperation_" << op
            << "_retirements=" << evidence.retirements[op] << "\noperation_" << op
            << "_presentations=" << evidence.presentations[op] << '\n';
    out.flush();
    require(bool(out),"fresh campaign report write failed");
}
} // namespace

static int run_owned(const std::filesystem::path& rom, const std::filesystem::path& root, Purpose purpose) {
    for (const auto* name : {"SDL_AUDIO_DRIVER","SDL_AUDIODRIVER"}) {
        const auto* value = std::getenv(name);
        require(value && std::string_view(value) == "dummy","fresh consumer requires both explicit SDL dummy audio variables");
    }
    require(root.is_absolute() && std::filesystem::is_directory(root) && !std::filesystem::is_symlink(root) &&
        std::filesystem::is_empty(root),"fresh campaign requires NEW empty absolute nonsymlink owned root");
#ifndef _WIN32
    require((std::filesystem::status(root).permissions() & std::filesystem::perms::mask) ==
        std::filesystem::perms::owner_all,"fresh campaign root must have mode 0700");
#endif
    for (const auto* child : {"host","profile","proof"}) {
        require(std::filesystem::create_directory(root/child),"fresh campaign child directory already exists");
        std::filesystem::permissions(root/child,std::filesystem::perms::owner_all,std::filesystem::perm_options::replace);
    }
    Evidence evidence;
    evidence.events.open(root/"proof"/"native-public-campaign-events.tsv");
    evidence.events << "kind\tserial\toperation\targument\tresult\tacknowledged_generation\n";
    evidence.events.flush();
    require(bool(evidence.events),"fresh Native boundary evidence open failed");
    std::string failure;
    try {
        const auto snapshot = isle::rom::read_snapshot(rom);
        auto hydrated = menu::hydrate(snapshot);
        require(bool(hydrated),"actual complete startup catalog hydration failed");
        g::Config options;
        options.data_directory = root/"host";
        options.input = g::InputMode::control_deck;
        auto platform = g::Host::create(snapshot,std::move(options));
        require(bool(platform),platform.message.data());
        auto& host = *platform.host;
        auto audio_owner = game_audio::Driver::create(snapshot,host.audio_output());
        if (!audio_owner) {
            remember(failure,audio_owner.message.data());
            (void)host.shutdown();
        } else {
            auto& audio = *audio_owner.driver;
            in::Service input;
            HostBoundary boundary{h::direct_platform(host),evidence};
            h::Progress progress(boundary.platform(),input,audio);
            {
                game::State state(root/"profile");
                h::GtlClock clock({1,1,2,true,0});
                std::unique_ptr<app::Runtime> runtime;
                std::optional<Display> display;
                try {
                    require(host.pump_events(),host.message());
                    const auto initial = host.acquire_input();
                    require(initial.completed,"actual HostControlDeck initial acquisition incomplete");
                    require(input.start(initial.ports) == in::Error::none,"actual HostControlDeck Input Service start failed");
                    auto made = app::Runtime::create(snapshot,*hydrated.catalog,state,input,audio.session(),clock,
                        {true,&evidence,Evidence::retired});
                    require(bool(made),made.message.data());
                    runtime = std::move(made.runtime);
                    boundary.runtime = runtime.get();
                    progress.bind(clock,{&boundary,HostBoundary::acknowledge});
                    display.emplace(Display{host,*runtime,evidence,progress,purpose});
                    require(runtime->bind_post_course_progress(progress,{&*display,[](void* owner) noexcept {
                        return static_cast<Display*>(owner)->service(true);
                    }}) == ISLE_GAME_OK,runtime->message());
                    progress.set_deadline(Time::now()+std::chrono::seconds(86400),
                        "actual public ControlDeck 86400-second wall deadline; incomplete");
                    for (;;) {
                        progress.set_policy({true,true,runtime->scene_active(),true,true});
                        const auto tick = progress.poll();
                        require(progress.status() != h::ProgressStatus::failed,progress.message());
                        if (progress.status() == h::ProgressStatus::stopped &&
                            !(purpose == Purpose::replay_stop && evidence.preview_stop_requested)) {
                            require(purpose == Purpose::campaign && evidence.complete(),
                                "WM close/EOF before complete campaign or submitted-preview stop");
                            evidence.reached = true;
                            break;
                        }
                        const bool pumped = runtime->pump(h::Progress::raw(tick));
                        require(progress.status() != h::ProgressStatus::failed,progress.message());
                        if (purpose == Purpose::replay_stop && evidence.preview_stop_requested && runtime->stopped()) {
                            require(runtime->error() == ISLE_GAME_OK,runtime->message());
                            require(!progress.deadline_expired(),progress.message());
                            require(!evidence.observer_failed,evidence.observer_message.data());
                            evidence.preview_stop_checked = true;
                            evidence.reached = true;
                            break;
                        }
                        require(pumped,runtime->message());
                        require(display->service(false) == ISLE_GAME_OK,evidence.observer_message.data());
                        require(!runtime->stopped(),"actual Runtime stopped before expected public consumer boundary");
                        require(!evidence.observer_failed,evidence.observer_message.data());
                        (void)progress.idle();
                        require(progress.status() != h::ProgressStatus::failed,progress.message());
                        // Stay live for independent final-Title pixel acquisition.
                        // Only ordinary Host close/EOF can accept the completed run.
                        if (progress.status() == h::ProgressStatus::stopped &&
                            !(purpose == Purpose::replay_stop && evidence.preview_stop_requested)) {
                            require(purpose == Purpose::campaign && evidence.complete(),
                                "WM close/EOF before complete campaign or submitted-preview stop");
                            evidence.reached = true;
                            break;
                        }
                    }
                } catch (const std::exception& error) { remember(failure,error.what()); }
                  catch (...) { remember(failure,"unknown actual fresh campaign Runtime failure"); }
                if (runtime) {
                    runtime->request_stop();
                    progress.set_policy({false,false,false,false,true});
                    progress.set_deadline(Time::now()+std::chrono::seconds(90),
                        "actual fresh Runtime shutdown deadline; Native/GPU owners remain live");
                    while (!runtime->stopped()) {
                        if (progress.deadline_expired()) {
                            remember(failure,"actual fresh Runtime shutdown deadline; Native/GPU owners remain live");
                            try { report(root,evidence,purpose,failure); } catch (...) {}
                            // A live Runtime cannot legally destruct, nor can its
                            // SDL/Audio dependencies be released underneath it.
                            std::cerr.flush(); std::cout.flush(); std::_Exit(EXIT_FAILURE);
                        }
                        const auto tick = progress.poll();
                        if (input.pending_read()) {
                            const auto completed = progress.complete_input();
                            if (!completed.completed || !completed.completion_accepted)
                                remember(failure,"actual input completion rejected during fresh Runtime cancellation");
                        }
                        (void)runtime->pump(h::Progress::raw(tick));
                        if (runtime->completed_frame()) {
                            if (runtime->release_completed() != ISLE_GAME_OK) remember(failure,runtime->message());
                            else ++evidence.frames;
                        }
                        (void)progress.idle();
                    }
                    if (runtime->error() != ISLE_GAME_OK) remember(failure,runtime->message());
                    evidence.runtime_retired = !runtime->scene_active() && !runtime->completed_frame() &&
                        !runtime->view().entry && clock.stopped() && !clock.failed();
                    evidence.submissions = clock.stats().submissions;
                    evidence.terminals = clock.stats().terminals;
                    progress.unbind();
                    boundary.runtime = nullptr;
                    runtime.reset();
                }
            } // All gameplay/session/Manager/scene owners end before cold reopen.
            if (input.running()) {
                const auto stopped = input.stop();
                evidence.input_stopped = stopped.error == in::Error::none && !input.running() &&
                    !input.pending_read() && !input.pending_update() &&
                    std::none_of(stopped.cancelled.begin(),stopped.cancelled.end(),[](const auto& token) { return bool(token); });
            }
            if (!host.cancel_present()) remember(failure,host.message());
            progress.set_policy({false,false,false,false,true});
            progress.set_deadline(Time::now()+std::chrono::seconds(90),"actual fresh Audio stop/drain deadline");
            if (host.error() == g::Error::none && audio.error() == game_audio::DriverError::none && audio.stop()) {
                while (!progress.deadline_expired()) {
                    (void)progress.poll();
                    const auto drain = audio.poll_drain();
                    if (drain == game_audio::Drain::drained) { evidence.audio_drained = true; break; }
                    if (drain == game_audio::Drain::failed || host.error() != g::Error::none) break;
                    (void)progress.idle();
                }
            }
            evidence.pcm = audio.session().accepted_frames();
            if (!audio.retire()) remember(failure,audio.message());
            const auto closed = host.shutdown();
            evidence.host_released = !closed.failed && closed.owners_released &&
                closed.audio == isle::audio::SdlAudioOutput::CloseDisposition::stream_drained;
            if (!evidence.runtime_retired || !evidence.input_stopped || !evidence.audio_drained || !evidence.host_released ||
                !evidence.pcm || evidence.submissions != evidence.terminals || evidence.frames != evidence.submissions ||
                evidence.input_requests != evidence.input_completions)
                remember(failure,"actual fresh Runtime/Input/Audio/Host/GPU ownership ledger did not retire/balance");
            if (failure.empty() && purpose == Purpose::campaign) {
                require(evidence.reached && bool(evidence.published_save),"fresh full campaign lacks acknowledged terminal Save");
                auto reopened = save::GameSession::open(root/"profile",0);
                require(reopened && reopened.state == save::GameLoadState::ready,
                    "cold GameSession reopen did not load existing ready acknowledged Save");
                const auto actual = reopened.session.snapshot();
                require(actual && actual.value.generation() == evidence.published_save.generation(),
                    "cold GameSession generation differs from durable Save ACK");
                const auto expected_bytes = evidence.published_save.image().bytes();
                const auto actual_bytes = actual.value.image().bytes();
                require(std::equal(expected_bytes.begin(),expected_bytes.end(),actual_bytes.begin(),actual_bytes.end()),
                    "cold GameSession whole canonical bytes differ from durable Save ACK");
                evidence.cold_equal = true;
            }
        }
    } catch (const std::exception& error) { remember(failure,error.what()); }
      catch (...) { remember(failure,"unknown fresh public campaign setup/retirement failure"); }
    if (evidence.observer_failed) remember(failure,evidence.observer_message.data());
    if (!evidence.reached) remember(failure,"actual public consumer boundary incomplete; never qualified partial exit0");
    if (purpose == Purpose::campaign && !evidence.cold_equal)
        remember(failure,"fresh full campaign incomplete; no exact cold equality");
    if (purpose == Purpose::replay_stop && (!evidence.preview_stop_requested || !evidence.preview_stop_checked ||
        evidence.preview_stop_result_emitted))
        remember(failure,"submitted-preview stop did not return through clean Runtime cancellation");
    report(root,evidence,purpose,failure);
    if (!failure.empty()) return 1;
    if (purpose == Purpose::campaign)
        std::cout << "fresh_public_campaign=passed whole_canonical_cold_GameSession=equal normal_exit0=true "
            "private_report=proof/native-public-campaign-report.txt independent_custody_pixel_qualification=required\n";
    else
        std::cout << "submitted_preview_stop=passed native_runtime_retired_without_fault=true campaign_qualified=false "
            "private_report=proof/native-public-campaign-report.txt independent_custody_review=required\n";
    return 0;
}
int run(const std::filesystem::path& rom, const std::filesystem::path& root) {
    return run_owned(rom,root,Purpose::campaign);
}
int run_preview_stop(const std::filesystem::path& rom, const std::filesystem::path& root) {
    return run_owned(rom,root,Purpose::replay_stop);
}
} // namespace isle::native_public_campaign
