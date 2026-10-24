#include "application_photos.hpp"
#include "game_host.hpp"
#include "game_state.hpp"
#include "host_progress.hpp"
#include "post_course_modules.hpp"
#include "post_course_scene.hpp"
#include "rom_snapshot.hpp"
#include <chrono>
#include <exception>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string_view>
#include <type_traits>

namespace {
using Component = isle::post_course::Component;
using Status = isle::post_course::SceneStatus;
using ClockError = isle::host::ClockError;
using Time = std::chrono::steady_clock;
static_assert(!std::is_convertible_v<Component*, isle::post_course::Scene*>);
static_assert(!std::is_convertible_v<Component&, isle::post_course::Scene&>);
#ifdef _WIN32
using Argument = wchar_t;
#define OPTION(value) L##value
#else
using Argument = char;
#define OPTION(value) value
#endif
using Text = std::basic_string_view<Argument>;
enum class Cancellation { none, before_update, completed_frame };
struct Options {
    isle::game_host::Config host;
    isle::post_course::ComponentEntry entry{};
    const char* entry_name = nullptr;
    Cancellation cancellation = Cancellation::none;
    unsigned deadline_seconds = 600;
    bool cpu = false;
};
void usage(std::ostream& out) {
    out << "Usage: post-course-component-rom-smoke <ROM> <existing-workdir>\n"
           "       <end8|credits17|credits18|credits19|credits20> [--cpu-renderer]\n"
           "       [--renderer-resources <directory>] [--deadline-seconds <1..86400>]\n"
           "       [--cancel-at <before-update|completed-frame>]\n"
           "The exact US ROM is read-only; workdir/profile must already contain the WHOLE\n"
           "unchanged canonical save pair earned through ordinary UI. No missing profile,\n"
           "photo, progress, gear, selector or RNG inputs are created or repaired here.\n"
           "Host configuration lives in workdir, not in the input ROM or profile pair.\n"
           "This explicitly selected standalone component is NOT a Root/Manager route.\n"
           "Default ordinary ControlDeck keyboard: W/A/S/D stick, X=A, C=B, Z=Z,\n"
           "Space=Start. Focus the real Host window; normal window close cancels.\n"
           "External window capture is permitted; this consumer reads no private Source.\n"
           "--cpu-renderer requires a CPU Vulkan device for canonical rendering.\n"
           "--renderer-resources selects the actual existing pinned shader directory.\n"
           "A deadline is FAILURE followed by actual retirement, never completion.\n"
           "Exit 0 requires an actual result or the requested reached cancellation,\n"
           "joined component/GTL owners and drained Host audio; no Root or hearing claim.\n";
}
bool seconds(Text text, unsigned& value) noexcept {
    if (text.empty()) return false;
    unsigned parsed = 0;
    for (const auto c : text) {
        if (c < Argument('0') || c > Argument('9')) return false;
        const auto digit = static_cast<unsigned>(c - Argument('0'));
        if (parsed > (86400u - digit) / 10u) return false;
        parsed = parsed * 10u + digit;
    }
    if (!parsed) return false;
    value = parsed;
    return true;
}
bool parse(int argc, Argument** argv, Options& options) {
    if (argc < 4) return false;
    options.host.data_directory = std::filesystem::absolute(std::filesystem::path(argv[2]));
    const Text entry(argv[3]);
    using Entry = isle::post_course::ComponentEntry;
    if (entry == OPTION("end8")) { options.entry = Entry::end_level_8; options.entry_name = "end8"; }
    else if (entry == OPTION("credits17")) { options.entry = Entry::credits17; options.entry_name = "credits17"; }
    else if (entry == OPTION("credits18")) { options.entry = Entry::credits18; options.entry_name = "credits18"; }
    else if (entry == OPTION("credits19")) { options.entry = Entry::credits19; options.entry_name = "credits19"; }
    else if (entry == OPTION("credits20")) { options.entry = Entry::credits20; options.entry_name = "credits20"; }
    else return false;
    for (int index = 4; index < argc; ++index) {
        const Text option(argv[index]);
        if (option == OPTION("--cpu-renderer")) options.cpu = true;
        else if (option == OPTION("--renderer-resources") && index + 1 < argc) {
            options.host.renderer_resources = std::filesystem::path(argv[++index]);
            if (options.host.renderer_resources.empty()) return false;
        } else if (option == OPTION("--deadline-seconds") && index + 1 < argc) {
            if (!seconds(Text(argv[++index]), options.deadline_seconds)) return false;
        } else if (option == OPTION("--cancel-at") && index + 1 < argc) {
            const Text milestone(argv[++index]);
            if (milestone == OPTION("before-update")) options.cancellation = Cancellation::before_update;
            else if (milestone == OPTION("completed-frame")) options.cancellation = Cancellation::completed_frame;
            else return false;
        } else return false;
    }
    return true;
}
const char* status_name(Status status) noexcept {
    switch (status) {
    case Status::ready: return "ready";
    case Status::setup_pending: return "setup_pending";
    case Status::running: return "running";
    case Status::retiring: return "retiring";
    case Status::completed: return "completed";
    case Status::cancelled: return "cancelled";
    case Status::failed: return "failed";
    case Status::retired: return "retired";
    }
    return "unknown";
}

// One outer consumer of the same canonical display executor used by Component.
// The nested service below only polls actual graphics, copies/releases images
// and resolves their exact GTL tokens. It never updates OM or resumes Source.
struct Consumer {
    isle::game_host::Host& host;
    isle::game_input::Service& input;
    isle::host::GtlClock& clock;
    isle::host::Progress& progress;
    Cancellation cancellation;
    Component* component = nullptr;
    std::optional<isle::host::FrameToken> executing{};
    isle::game_input::RequestToken publication{};
    bool publication_ready = false, updating = false, stopping = false;
    bool draw_accepted = false;
    bool failed = false, cancellation_reached = false, normal_result = false;
    std::uint64_t updates = 0, submissions = 0, releases = 0;
    std::uint64_t presentations = 0, native_services = 0;
    std::uint64_t retirement_attempts = 0, retirement_pending = 0;

    void fail(const char* message) noexcept {
        if (!failed) std::cerr << "COMPONENT_FAILURE " << message << '\n';
        failed = true;
        stopping = true;
        clock.request_stop();
        progress.request_stop();
    }
    void stop(const char* reason, bool requested_milestone = false) noexcept {
        if (!stopping) std::cout << "CANCEL_REQUEST reason=" << reason << '\n';
        stopping = true;
        cancellation_reached |= requested_milestone;
        clock.request_stop();
        progress.request_stop();
    }
    bool checked(IsleGameResult result, const char* message) noexcept {
        if (result == ISLE_GAME_OK) return true;
        if (result == ISLE_GAME_STOPPED) stop("public-stopped");
        else fail(message);
        return false;
    }
    bool checked(ClockError result, const char* message) noexcept {
        if (result == ClockError::none) return true;
        fail(message);
        return false;
    }
    void observe_host() noexcept {
        if (host.error() != isle::game_host::Error::none) fail(host.message());
        if (progress.status() == isle::host::ProgressStatus::failed) fail(progress.message());
        else if (progress.status() == isle::host::ProgressStatus::stopped) stop("host-stop");
    }
    static bool acknowledge(void* raw, const isle::game_input::EventResult& event) noexcept {
        auto& self = *static_cast<Consumer*>(raw);
        if (event.error != isle::game_input::Error::none) {
            self.fail("actual controller read completion failed");
            return false;
        }
        if (self.publication && event.acknowledged == self.publication) self.publication_ready = true;
        return true;
    }
    void terminal(isle::host::Terminal value) noexcept {
        if (!executing) { fail("graphics terminal lost its actual FrameToken"); return; }
        const auto ended = draw_accepted ? clock.offer_terminal(*executing, value) : clock.abandon_draw(*executing);
        if (checked(ended, "GTL rejected actual graphics terminal")) {
            executing.reset();
            draw_accepted = false;
        }
    }
    void service_display() noexcept {
        observe_host();
        if (component && executing) {
            const auto polled = component->poll();
            const auto frame = component->completed_frame();
            if (frame) {
                // Keep this real completed image and token live across a public
                // service boundary before cancellation's release/retirement.
                if (cancellation == Cancellation::completed_frame && !cancellation_reached && !stopping) {
                    std::cout << "HELD_COMPLETED_FRAME token=" << executing->serial
                              << " ordinal=" << frame.ordinal << " width=" << frame.width
                              << " height=" << frame.height << '\n';
                    stop("completed-frame", true);
                    return;
                }
                if (host.error() == isle::game_host::Error::none) {
                    if ((host.presentation_pending() && !host.cancel_present()) ||
                        !host.configure_present(frame.width, frame.height) ||
                        !host.queue_present(frame.rgba, frame.width, frame.height, frame.screen_offsets))
                        fail(host.message());
                }
                if (!checked(component->release_completed(), component->message())) return;
                ++releases;
                if (releases == 1)
                    std::cout << "FIRST_COMPLETED_FRAME token=" << executing->serial
                              << " ordinal=" << frame.ordinal << " width=" << frame.width
                              << " height=" << frame.height << " consumer_released=true\n";
                terminal(isle::host::Terminal::completed);
            } else if (polled == isle::scene::graphics::Poll::failed ||
                       (polled == isle::scene::graphics::Poll::idle && !component->graphics_busy())) {
                fail(component->message());
                // A failed poll is not a terminal until all real executor and
                // readback accesses have ceased. No timeout supplies this fence.
                if (!component->graphics_busy()) terminal(isle::host::Terminal::failed);
            }
        }
        if (host.error() == isle::game_host::Error::none && host.presentation_pending()) {
            const auto presented = host.try_present();
            if (presented == isle::game_host::Present::failed) fail(host.message());
            else if (presented == isle::game_host::Present::completed) {
                if (++presentations == 1) std::cout << "FIRST_HOST_PRESENTATION actual_window=true\n";
            }
        }
    }
    static IsleGameResult native_service(void* raw) noexcept {
        auto& self = *static_cast<Consumer*>(raw);
        ++self.native_services;
        self.service_display();
        // STOPPED requests the existing Source cancellation path. No lifecycle
        // destruction or OM/GTL update is reentered under this returning leaf.
        return self.failed ? ISLE_GAME_EXECUTION_FAILURE : self.stopping ? ISLE_GAME_STOPPED : ISLE_GAME_OK;
    }
    void abandon_update() noexcept {
        if (updating && checked(clock.abandon_update(), "actual update claim could not be abandoned")) {
            updating = false;
            publication = {};
            publication_ready = false;
        }
    }
    void run() noexcept {
        if (!checked(component->resume(), component->message())) stopping = true;
        if (!stopping && !component->setup_pending()) fail("Original component entry omitted public setup intent");
        if (!stopping) {
            std::cout << "COMPONENT_SETUP_PENDING entry_resumed=true\n";
            const auto initialized = component->initialize_scene();
            std::cout << "COMPONENT_INITIALIZE result=" << static_cast<int>(initialized)
                      << " status=" << status_name(component->status())
                      << " message=" << component->message() << '\n';
            (void)checked(initialized, component->message());
        }
        // Preserve an initialization failure (including the pre-fix End8
        // service-admission failure); never supply an update to work around it.
        if (!stopping && cancellation == Cancellation::before_update) {
            std::cout << "BEFORE_FIRST_UPDATE actual_updates=0\n";
            stop("before-update", true);
        }
        bool native_retired = false;
        while (component->status() != Status::retired) {
            const bool active = !stopping && component->status() == Status::running;
            progress.set_policy({!stopping, !stopping, active, true, true});
            const auto tick = progress.poll();
            observe_host();
            service_display();
            if (component->error() != ISLE_GAME_OK) fail(component->message());
            if (stopping) {
                clock.request_stop();
                abandon_update();
                // Complete only the genuine outstanding acquisition. This is
                // not an invented publication or a new cancellation-time read.
                if (input.pending_read()) (void)progress.complete_input();
            }
            const bool finishing = stopping || component->status() == Status::retiring;
            if (finishing) {
                clock.request_stop();
                if (!executing && !updating &&
                    (clock.stopped() || clock.poll().kind == isle::host::ActionKind::stopped)) {
                    if (stopping) (void)component->cancel();
                    ++retirement_attempts;
                    if (component->retire_scene()) {
                        if (!native_retired) {
                            native_retired = true;
                            std::cout << "NATIVE_SCENE_RETIRED status=" << status_name(component->status())
                                      << " graphics_busy=" << component->graphics_busy()
                                      << " completed_held=" << bool(component->completed_frame()) << '\n';
                        }
                        if (!stopping && checked(component->resume(), component->message())) {
                            normal_result = component->result().has_value();
                            if (!normal_result) fail("retired Original component omitted its public result");
                        }
                        (void)component->retire();
                    } else ++retirement_pending;
                }
            } else if (!executing) {
                if (!updating) {
                    const auto action = clock.poll();
                    if (action.kind == isle::host::ActionKind::update) updating = true;
                    else if (action.kind == isle::host::ActionKind::draw) {
                        const bool begun = component->draw({action.token.context});
                        if (!begun && !component->graphics_busy()) {
                            (void)checked(clock.abandon_draw(action.token), "unadmitted draw abandonment failed");
                            fail(component->message());
                        } else {
                            executing = action.token;
                            const auto accepted = clock.accept_draw(action.token);
                            draw_accepted = accepted == ClockError::none;
                            (void)checked(accepted, "actual draw lost its GTL context");
                            if (++submissions == 1)
                                std::cout << "FIRST_FRAME_SUBMITTED token=" << action.token.serial
                                          << " context=" << unsigned(action.token.context) << '\n';
                            if (!begun) fail(component->message());
                        }
                    }
                }
                if (updating && !stopping) {
                    if (!publication && !input.pending_read()) {
                        const auto published = input.update();
                        if (published.error != isle::game_input::Error::none || !published.accepted)
                            fail("actual controller publication request failed");
                        else {
                            publication = published.accepted;
                            publication_ready = published.acknowledged == publication;
                        }
                    }
                    if (publication_ready && !stopping) {
                        ++updates;
                        const auto updated = component->update(isle::host::Progress::raw(tick));
                        if (updates == 1)
                            std::cout << "FIRST_ACTUAL_UPDATE result=" << static_cast<int>(updated) << '\n';
                        (void)checked(updated, component->message());
                        if (stopping) abandon_update();
                        else if (checked(clock.finish_update(), "actual OM completion rejected by GTL")) {
                            updating = false;
                            publication = {};
                            publication_ready = false;
                        }
                    }
                }
            }
            (void)progress.idle();
        }
        observe_host();
        const auto result = component->result();
        std::cout << "COMPONENT_RETIRED status=" << status_name(component->status())
                  << " error=" << static_cast<int>(component->error()) << " result=";
        if (result) std::cout << *result;
        else std::cout << "none";
        const auto stats = clock.stats();
        std::cout << " actual_updates=" << updates << " submitted=" << submissions
                  << " consumer_releases=" << releases << " host_presentations=" << presentations
                  << " native_services=" << native_services
                  << " retirement_attempts=" << retirement_attempts << " retirement_pending=" << retirement_pending
                  << " gtl_submissions=" << stats.submissions
                  << " gtl_terminals=" << stats.terminals << " gtl_stopped=" << clock.stopped()
                  << " requested_cancel_reached=" << cancellation_reached
                  << " root_acknowledgments=0\n";
        if (!clock.stopped() || executing || updating || component->graphics_busy() || component->completed_frame())
            fail("component retired with an outstanding public owner");
        if (cancellation != Cancellation::none && !cancellation_reached)
            fail("requested cancellation milestone was not reached");
    }
};

bool shutdown(isle::game_host::Host& host, isle::game_input::Service& input,
              isle::host::Progress& progress) noexcept {
    progress.unbind();
    const auto stopped = input.stop();
    bool failed = stopped.error != isle::game_input::Error::none;
    if (!host.cancel_present()) failed = true;
    progress.clear_deadline();
    bool drained = false;
    if (host.error() == isle::game_host::Error::none && host.stop_audio()) {
        progress.set_deadline(Time::now() + std::chrono::seconds(30), "actual Host audio shutdown deadline");
        while (!progress.deadline_expired()) {
            progress.set_policy({false, false, false, false, true});
            (void)progress.poll();
            if (host.error() != isle::game_host::Error::none) break;
            const auto state = host.poll_audio_drain();
            if (state == isle::game_host::Drain::drained) { drained = true; break; }
            if (state == isle::game_host::Drain::failed) break;
            (void)progress.idle();
        }
    }
    const auto closed = host.shutdown();
    std::cout << "HOST_RETIRED owners_released=" << closed.owners_released
              << " audio_drained=" << drained << " audio_close=" << static_cast<int>(closed.audio)
              << " failed=" << closed.failed << " dummy_pcm_is_not_hearing_proof=true\n";
    if (host.error() != isle::game_host::Error::none) std::cerr << host.message() << '\n';
    return !failed && drained && !closed.failed && closed.owners_released &&
        closed.audio == isle::audio::SdlAudioOutput::CloseDisposition::stream_drained;
}
int run(int argc, Argument** argv) {
    std::cout << std::boolalpha << std::unitbuf;
    if (argc == 2 && Text(argv[1]) == OPTION("--help")) { usage(std::cout); return 0; }
    Options options;
    if (!parse(argc, argv, options)) { usage(std::cerr); return 2; }
    const auto profile = options.host.data_directory / "profile";
    if (!std::filesystem::is_directory(options.host.data_directory) || !std::filesystem::is_directory(profile)) {
        std::cerr << "Existing workdir and workdir/profile with an ordinary UI-earned whole save pair are required.\n";
        return 2;
    }
    const auto snapshot = isle::rom::read_snapshot(std::filesystem::path(argv[1]));
    auto platform = isle::game_host::Host::create(snapshot, options.host);
    if (!platform) { std::cerr << platform.message.data() << '\n'; return 1; }
    auto& host = *platform.host;
    isle::game_input::Service input;
    isle::host::GtlClock clock({1, 1, 2, true, 0});
    isle::host::Progress progress(host, input);
    Consumer consumer{host, input, clock, progress, options.cancellation};
    progress.bind(clock, {&consumer, Consumer::acknowledge});
    progress.set_deadline(Time::now() + std::chrono::seconds(options.deadline_seconds),
                          "standalone component deadline expired; not successful completion");
    progress.set_policy({false, false, false, false, true});
    std::cout << "COMPONENT_ONLY entry=" << options.entry_name
              << " root_route_claimed=false manager_entry_borrow=false input=ordinary-ControlDeck"
                 " private_source_reads=false canonical_pair_authored=false\n";
    const auto& audio = host.audio_info();
    std::cout << "AUDIO_OUTPUT driver=" << audio.driver.data()
              << " physical_device=" << audio.physical_device << " logical_device=" << audio.logical_device
              << " dummy_pcm_is_not_hearing_proof=true\n";
    bool initialized_input = false;
    while (!initialized_input && progress.status() == isle::host::ProgressStatus::running) {
        (void)progress.poll();
        if (progress.status() != isle::host::ProgressStatus::running) break;
        const auto initial = host.acquire_input();
        if (initial.completed) {
            initialized_input = input.start(initial.ports) == isle::game_input::Error::none;
            if (!initialized_input) consumer.fail("actual ControlDeck initialization failed");
        }
        if (!initialized_input) (void)progress.idle();
    }
    consumer.observe_host();
    bool success = false;
    if (!consumer.stopping && initialized_input && host.audio()) {
        isle::game::State state(profile);
        isle::host::ProgressTick opened_at{};
        opened_at.ns = progress.elapsed_ns();
        if (!state.initialize(isle::host::Progress::raw(opened_at))) consumer.fail(state.error());
        else if (!state.save_initialization() ||
                 state.save_initialization()->state != isle::save::GameLoadState::ready)
            consumer.fail("standalone component requires an existing ready canonical save pair, never Missing/defaults");
        else {
            state.select_controller(input.snapshot());
            isle::application::Photos photos(snapshot, *state.save());
            isle::post_course::ComponentModules modules(snapshot);
            std::cout << "PUBLIC_INPUT_READY controllers=" << input.snapshot().controller_count
                      << " PROFILE_READY existing_pair=true no_input_repair=true\n";
            auto made = Component::create(snapshot, state, input, *host.audio(), clock, photos, progress,
                {&consumer, Consumer::native_service}, modules, options.entry, options.cpu);
            if (!made) consumer.fail(made.message.data());
            else {
                consumer.component = made.component.get();
                consumer.run();
                success = !consumer.failed && (consumer.normal_result || consumer.cancellation_reached);
                // Callback storage, ROM, modules, Photos, State, Input, GTL and
                // Host all remain live through complete Component destruction.
                made.component.reset();
                consumer.component = nullptr;
            }
            // Even decoded/save/module owners remain live until actual audio,
            // native executors and Host callbacks have reached their terminals.
            return shutdown(host, input, progress) && success ? 0 : 1;
        }
    } else consumer.fail("actual Input/Host startup did not complete");
    (void)shutdown(host, input, progress);
    return 1;
}
} // namespace
#ifdef _WIN32
int wmain(int argc, wchar_t** argv) {
#else
int main(int argc, char** argv) {
#endif
    try { return run(argc, argv); }
    catch (const std::exception& error) { std::cerr << "Standalone component consumer failed: " << error.what() << '\n'; return 1; }
    catch (...) { std::cerr << "Standalone component consumer failed.\n"; return 1; }
}
