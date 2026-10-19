#ifndef ISLE_NATIVE_SCENE_DRIVER_HPP
#define ISLE_NATIVE_SCENE_DRIVER_HPP

#include "game_host.hpp"
#include "host_clock.hpp"
#include "scene_graphics.hpp"
#include "scene_services.hpp"
#include <array>
#include <chrono>
#include <exception>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <thread>
#include <utility>

namespace isle::private_scene {
// Explicit private-ROM consumer, not a substitute renderer/input/audio backend.
// Each call publishes a real retrace-acquired owned software sample, performs
// exactly one OM update, and resolves its source-due graphics before returning.
template<class Scene>
class Driver final {
public:
    using Time = std::chrono::steady_clock::time_point;
    using Predicate = bool (*)(const Scene&) noexcept;
    using Capture = void (*)(const std::filesystem::path&);
    Driver(Scene& native_scene, scene::Services& services, game_input::Service& input,
           game_host::Host& platform, scene::graphics::Frame& graphics,
           host::GtlClock& clock, host::RetraceClock& retraces,
           std::uint64_t& retrace_ordinal, Time epoch, Time deadline,
           Predicate exited, Predicate capture_ready,
           std::filesystem::path capture_path, Capture capture)
        : scene_(native_scene), services_(services), input_(input), host_(platform),
          graphics_(graphics), clock_(clock), retraces_(retraces),
          retrace_ordinal_(retrace_ordinal), epoch_(epoch), deadline_(deadline),
          exited_(exited), capture_ready_(capture_ready),
          capture_path_(std::move(capture_path)), capture_(capture) {
        check(exited_ && capture_ready_ && capture_,"real scene driver callbacks are required");
        check(graphics_.quiescent() && clock_.stopped(),"scene entry preceded actual graphics/GTL retirement");
        checked(clock_.reset({1,2,2,true,0}));
    }
    ~Driver() {
        // Optional presentation owns pixels, not native scene resources. Failure
        // remains failure; only a real unresolved native/GPU owner is fatal here.
        fence();
        if (!graphics_.quiescent() || !clock_.stopped()) std::terminate();
    }
    Driver(const Driver&) = delete;
    Driver& operator=(const Driver&) = delete;

    template<class Result>
    bool advance(const game_input::HostFrame& sample, Result& result) noexcept {
        result = {};
        if (error_[0]) return false;
        sample_ = sample;
        try {
            check(!clock_.stopping() && graphics_.quiescent(),"update attempted after stop or before graphics retirement");
            // A new scripted sample cannot be attributed to an earlier SI read.
            // Wait for the next actual elapsed retrace; never manufacture one.
            const auto previous_read = reads_;
            while (reads_ == previous_read) { pump(); nap(); }
            host::ClockAction action;
            do {
                action = clock_.poll();
                if (action.kind == host::ActionKind::wait) { pump(); nap(); }
            } while (action.kind == host::ActionKind::wait);
            check(action.kind == host::ActionKind::update,"source scene scheduler did not offer an OM update");
            updating_ = true;
            const auto publication = input_.update();
            check(publication.error == game_input::Error::none && publication.accepted &&
                  publication.acknowledged == publication.accepted,
                  "source contUpdate did not acknowledge the newly acquired sample");
            services_.raw_game_time = static_cast<std::uint32_t>(host::RetraceClock::guest_ticks(elapsed_ns()));
            check(services_.tick() == ISLE_GAME_OK,"actual scene OM update failed");
            check(!scene_.failed(),scene_.error());
            if (exited_(scene_)) clock_.request_stop();
            checked(clock_.finish_update());
            updating_ = false;
            ++updates_;
            if (clock_.stopping()) {
                check(clock_.poll().kind == host::ActionKind::stopped,"source exit retained a real GTL borrower");
                return true;
            }
            // finish_update's public frame index determines the configured
            // source draw boundary. Do not poll/admit another OM update merely
            // to discover that this update has no draw.
            if (clock_.frame() % 2 != 0) return true;
            action = clock_.poll();
            check(action.kind == host::ActionKind::draw,"source due draw did not acquire a real context");
            offered_draw_ = action.token;
            check(graphics_.begin(scene_.render_input(),{action.token.context}),graphics_.message());
            checked(clock_.accept_draw(action.token));
            executing_ = action.token;
            offered_draw_.reset();
            while (executing_) { pump(); poll_graphics(); if (executing_) nap(); }
            const auto completed = graphics_.completed();
            check(bool(completed) && completed.ordinal != 0,"actual completed scene image/terminal missing");
            result.rgba = completed.rgba;
            result.actual_terminal_ordinal = completed.ordinal;
            return true;
        } catch (const std::exception& exception) {
            remember(exception.what());
        } catch (...) {
            remember("unknown actual native scene driver failure");
        }
        clock_.request_stop();
        if (updating_) {
            // isle_game_tick has returned: update accesses have actually ceased.
            clock_.abandon_update();
            updating_ = false;
        }
        if (offered_draw_ && graphics_.quiescent()) {
            clock_.abandon_draw(*offered_draw_);
            offered_draw_.reset();
        }
        return false;
    }

    bool fence() noexcept {
        if (graphics_.quiescent() && clock_.stopped() && !host_.presentation_pending()) return true;
        try {
            clock_.request_stop();
            if (updating_) { checked(clock_.abandon_update()); updating_ = false; }
            if (offered_draw_ && graphics_.quiescent()) {
                checked(clock_.abandon_draw(*offered_draw_)); offered_draw_.reset();
            }
            while (executing_ || graphics_.busy()) {
                pump(); poll_graphics(); if (executing_ || graphics_.busy()) nap();
            }
            check(graphics_.quiescent(),"actual graphics/readback/producer fence unresolved");
            check(clock_.poll().kind == host::ActionKind::stopped,"actual scene GTL retirement unresolved");
            while (host_.presentation_pending()) { pump(); if (host_.presentation_pending()) nap(); }
            return true; // Actual cessation, never reclassification of error().
        } catch (const std::exception& exception) { remember(exception.what()); }
        catch (...) { remember("unknown real scene retirement failure"); }
        return false;
    }
    const char* error() const noexcept { return error_.data(); }
    bool captured() const noexcept { return captured_; }
    std::uint64_t updates() const noexcept { return updates_; }
    std::uint64_t frames() const noexcept { return frames_; }
    std::uint64_t presentations() const noexcept { return presentations_; }
private:
    static void check(bool value,const char* reason) {
        if (!value) throw std::runtime_error(reason);
    }
    static void checked(host::ClockError value) {
        check(value == host::ClockError::none,"actual scene GTL ownership transition rejected");
    }
    static void nap() { std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
    void remember(const char* message) noexcept {
        if (error_[0]) return;
        std::size_t n = 0;
        while (n+1 < error_.size() && message && message[n]) { error_[n] = message[n]; ++n; }
        error_[n] = '\0';
    }
    std::uint64_t elapsed_ns() const {
        const auto now = std::chrono::steady_clock::now();
        check(now < deadline_,"actual native scene integration wall deadline expired");
        return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(now-epoch_).count());
    }
    void pump() {
        const auto ns = elapsed_ns();
        check(host_.pump_events(),host_.message());
        const auto ticks = retraces_.advance(ns);
        for (std::uint64_t n=0;n<ticks;++n) {
            const auto requested = input_.retrace();
            check(requested.error == game_input::Error::none && bool(requested.read),"actual scene retrace acquisition missing");
            const auto completed = input_.complete_read(requested.read,sample_);
            check(completed.error == game_input::Error::none,"actual owned scene sample completion rejected");
            ++reads_;
            check(host_.notify_retrace(++retrace_ordinal_,ns),host_.message());
        }
        if (ticks && !clock_.stopping() && !clock_.stopped()) {
            const auto offered = clock_.offer_retraces(ticks);
            check(offered == host::ClockError::none || offered == host::ClockError::full,
                  "actual source retrace admission failed");
        }
        check(host_.pump_audio(),host_.message());
        if (host_.presentation_pending()) {
            const auto presented = host_.try_present();
            check(presented != game_host::Present::failed,host_.message());
            if (presented == game_host::Present::completed) {
                ++presentations_;
                if (queued_capture_ready_ && !capture_attempted_) {
                    capture_attempted_ = true;
                    capture_(capture_path_);
                    captured_ = true;
                }
            }
        }
    }
    void poll_graphics() {
        if (!executing_) return;
        const auto status = graphics_.poll();
        if (status == scene::graphics::Poll::pending) return;
        check(status == scene::graphics::Poll::completed || status == scene::graphics::Poll::failed,
              "actual scene graphics lost its admitted task");
        check(graphics_.quiescent(),"graphics reported terminal before actual GPU/readback borrowers ceased");
        checked(clock_.offer_terminal(*executing_,status == scene::graphics::Poll::completed?
                                     host::Terminal::completed:host::Terminal::failed));
        executing_.reset();
        check(status == scene::graphics::Poll::completed,graphics_.message());
        ++frames_;
        if (!host_.presentation_pending()) {
            const auto completed = graphics_.completed();
            save::ScreenOffsets live;
            check(services_.state.screen_offsets().translation(live),
                  "actual scene output cannot collapse unequal live VI endpoints");
            check(completed.screen_offsets.horizontal == live.horizontal &&
                  completed.screen_offsets.vertical == live.vertical,
                  "actual scene graphics used profile/editor VI instead of canonical live output");
            check(host_.queue_present(completed.rgba,completed.screen_offsets),host_.message());
            queued_capture_ready_ = capture_ready_(scene_);
        }
    }
    Scene& scene_;
    scene::Services& services_;
    game_input::Service& input_;
    game_host::Host& host_;
    scene::graphics::Frame& graphics_;
    host::GtlClock& clock_;
    host::RetraceClock& retraces_;
    std::uint64_t& retrace_ordinal_;
    Time epoch_, deadline_;
    Predicate exited_, capture_ready_;
    std::filesystem::path capture_path_;
    Capture capture_;
    game_input::HostFrame sample_{};
    std::optional<host::FrameToken> executing_, offered_draw_;
    std::array<char,512> error_{};
    std::uint64_t reads_ = 0, updates_ = 0, frames_ = 0, presentations_ = 0;
    bool updating_ = false, queued_capture_ready_ = false;
    bool capture_attempted_ = false, captured_ = false;
};
} // namespace isle::private_scene
#endif
