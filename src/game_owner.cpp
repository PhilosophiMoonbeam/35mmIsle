#include "game_owner.hpp"
#include "game_audio_driver.hpp"
#include "game_state.hpp"
#include "host_progress.hpp"
#include "post_course_replay.hpp" // Complete ReplayNativeProgress at the bind boundary.
#include "snap_station.hpp"
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <limits>
#include <mutex>
#include <optional>
#include <thread>
#include <utility>
#ifdef __linux__
#include <pthread.h>
#endif

namespace isle::game_owner {
namespace {
void retain(std::array<char,512>& destination,const char* message) noexcept {
    if (!destination[0]) std::snprintf(destination.data(),destination.size(),"%s",
        message&&message[0]?message:"native GAME owner failed");
}
}
struct Owner::Impl {
    using Time=std::chrono::steady_clock;
    using MainCall=void (*)(Impl&,void*) noexcept;
    enum class CallState { idle, pending, executing, completed };
    enum class DisplayState { idle, offered, copying, acknowledged };
    struct DisplayOffer {
        std::uint64_t epoch=0,serial=0;
        std::span<const std::byte> rgba{};
        std::uint32_t width=0,height=0;
        save::ScreenOffsets offsets{};
    };
    const rom::Snapshot& snapshot;
    const menu::Catalog& catalog;
    const application::RuntimeConfig config;
    const std::filesystem::path station_directory;
    const std::thread::id main_thread=std::this_thread::get_id();
    mutable std::mutex mutex;
    std::condition_variable changed;
    game_input::HostTransfer transfer;
    std::thread worker;
    game_host::Host* host=nullptr; // Main only; retained through actual epoch retirement.
    game_audio::Output endpoint{}; // Main only; never invoked by GAME directly.
    std::filesystem::path profile;
    std::uint64_t epoch=0;
    bool ready=false,quitting=false,active=false,retired=true,stop_requested=false;
    bool platform_running=false;
    game_host::Error platform_error=game_host::Error::none;
    std::array<char,512> platform_message{};
    std::uint32_t station_actions=0;
    EpochResult outcome{};
    CallState call_state=CallState::idle;
    MainCall call=nullptr;
    void* call_argument=nullptr;
    DisplayState display_state=DisplayState::idle;
    DisplayOffer display{};
    std::uint64_t next_display=0;
    game_input::ReadToken main_read{},game_read{};
    std::optional<game_input::ReadCompletion> main_completion;

    Impl(const rom::Snapshot& s,const menu::Catalog& c,application::RuntimeConfig options,
         std::filesystem::path directory)
        :snapshot(s),catalog(c),config(options),station_directory(std::move(directory)) {
        worker=std::thread([this] { run(); });
        std::unique_lock lock(mutex);
        changed.wait(lock,[this] { return ready; });
    }
    void require_main() const noexcept {
        if (std::this_thread::get_id()!=main_thread) std::terminate();
    }
    bool main_call(MainCall function,void* argument) noexcept {
        std::unique_lock lock(mutex);
        changed.wait(lock,[this] { return call_state==CallState::idle||!active; });
        if (!active) return false;
        call=function;call_argument=argument;call_state=CallState::pending;
        changed.notify_all();
        // Stop never cancels accepted stack storage or pretends an operation
        // completed. Main remains serviceable through actual retirement.
        changed.wait(lock,[this] { return call_state==CallState::completed; });
        call=nullptr;call_argument=nullptr;call_state=CallState::idle;
        changed.notify_all();
        return true;
    }
    static game_audio::Output::Info inspect(void* owner) noexcept {
        auto& self=*static_cast<Impl*>(owner);
        game_audio::Output::Info result{};
        (void)self.main_call(+[](Impl& bridge,void* raw) noexcept {
            *static_cast<game_audio::Output::Info*>(raw)=bridge.endpoint.inspect(bridge.endpoint.owner);
        },&result);
        return result;
    }
    static game_audio::Output::Acceptance submit(void* owner,std::span<const std::int16_t> samples) noexcept {
        auto& self=*static_cast<Impl*>(owner);
        struct Request { std::span<const std::int16_t> samples;game_audio::Output::Acceptance result{}; } request{samples,{}};
        (void)self.main_call(+[](Impl& bridge,void* raw) noexcept {
            auto& value=*static_cast<Request*>(raw);
            value.result=bridge.endpoint.submit(bridge.endpoint.owner,value.samples);
        },&request);
        return request.result;
    }
    static bool finish_output(void* owner) noexcept {
        auto& self=*static_cast<Impl*>(owner);
        bool result=false;
        (void)self.main_call(+[](Impl& bridge,void* raw) noexcept {
            *static_cast<bool*>(raw)=bridge.endpoint.finish(bridge.endpoint.owner);
        },&result);
        return result;
    }
    static game_audio::Output::State poll_output(void* owner) noexcept {
        auto& self=*static_cast<Impl*>(owner);
        auto result=game_audio::Output::State::failed;
        (void)self.main_call(+[](Impl& bridge,void* raw) noexcept {
            *static_cast<game_audio::Output::State*>(raw)=bridge.endpoint.poll(bridge.endpoint.owner);
        },&result);
        return result;
    }
    game_audio::Output audio_output() noexcept { return {this,inspect,submit,finish_output,poll_output}; }
    static bool events(void* owner) noexcept {
        auto& self=*static_cast<Impl*>(owner);
        std::lock_guard lock(self.mutex);
        return self.platform_running&&!self.stop_requested;
    }
    static game_host::Error error(void* owner) noexcept {
        auto& self=*static_cast<Impl*>(owner);
        std::lock_guard lock(self.mutex);
        return self.platform_error;
    }
    static const char* message(void* owner) noexcept {
        // First-failure text is written before platform_error publication under
        // the mutex and remains immutable until the next quiescent epoch.
        return static_cast<Impl*>(owner)->platform_message.data();
    }
    void transfer_failure(const char* diagnostic) noexcept {
        std::lock_guard lock(mutex);
        if (platform_error==game_host::Error::none) {
            retain(platform_message,diagnostic);platform_error=game_host::Error::input;
        }
        stop_requested=true;changed.notify_all();
    }
    static game_host::Input read(void* owner,game_input::ReadToken token) noexcept {
        auto& self=*static_cast<Impl*>(owner);
        if (self.game_read&&self.game_read!=token) {
            self.transfer_failure("GAME Input transfer changed its outstanding read identity");return {};
        }
        if (!self.game_read) {
            const auto sent=self.transfer.request_read(token);
            if (sent==game_input::Error::transfer_full) return {};
            if (sent!=game_input::Error::none) { self.transfer_failure("GAME Input transfer rejected a real read");return {}; }
            self.game_read=token;self.changed.notify_all();
        }
        game_input::ReadCompletion completion{};
        const auto received=self.transfer.acquire_sample(completion);
        if (received==game_input::Error::transfer_empty) return {};
        if (received!=game_input::Error::none||completion.token!=token) {
            self.transfer_failure("GAME Input transfer returned a different read completion");return {};
        }
        self.game_read={};
        game_host::Input result{};result.ports=completion.frame;result.completed=true;
        return result;
    }
    host::PlatformProgress platform() noexcept { return {this,events,read,error,message}; }
    game_host::Input initial_input() noexcept {
        game_host::Input result{};
        (void)main_call(+[](Impl& bridge,void* raw) noexcept {
            *static_cast<game_host::Input*>(raw)=bridge.host->acquire_input();
        },&result);
        return result;
    }
    void feedback(const char* text) noexcept {
        (void)main_call(+[](Impl& bridge,void* raw) noexcept {
            (void)bridge.host->station_feedback(static_cast<const char*>(raw));
        },const_cast<char*>(text));
    }
    std::uint32_t take_station_actions() noexcept {
        std::lock_guard lock(mutex);
        return std::exchange(station_actions,0);
    }
    void cancel_input() noexcept {
        // Service::stop has already cancelled the actual read. Main withdraws
        // only its acquisition/retained completion; no success is synthesized.
        (void)main_call(+[](Impl& bridge,void*) noexcept {
            bridge.main_read={};bridge.main_completion.reset();
            game_input::ReadToken token{};
            while (bridge.transfer.acquire_request(token)==game_input::Error::none) {}
        },nullptr);
        game_input::ReadCompletion completion{};
        while (transfer.acquire_sample(completion)==game_input::Error::none) {}
        game_read={};
    }
    std::uint64_t offer(const scene::graphics::Completed& frame) noexcept {
        std::lock_guard lock(mutex);
        if (display_state!=DisplayState::idle||next_display==std::numeric_limits<std::uint64_t>::max()) return 0;
        display={epoch,++next_display,frame.rgba,frame.width,frame.height,frame.screen_offsets};
        display_state=DisplayState::offered;changed.notify_all();
        return display.serial;
    }
    bool acknowledged(std::uint64_t serial) noexcept {
        std::lock_guard lock(mutex);
        if (display_state!=DisplayState::acknowledged) return false;
        if (display.serial!=serial||display.epoch!=epoch) std::terminate();
        display={};display_state=DisplayState::idle;changed.notify_all();return true;
    }
    struct NativeDisplay {
        Impl& bridge;
        application::Runtime& application;
        bool& failed;
        std::uint64_t outstanding=0;
        static IsleGameResult service(void* raw) noexcept {
            auto& self=*static_cast<NativeDisplay*>(raw);
            if (self.outstanding&&self.bridge.acknowledged(self.outstanding)) {
                self.outstanding=0;
                const auto result=self.application.release_completed();
                if (result!=ISLE_GAME_OK) { self.failed=true;self.application.request_stop();return result; }
            }
            if (!self.outstanding) {
                if (const auto frame=self.application.completed_frame()) {
                    self.outstanding=self.bridge.offer(frame);
                    if (!self.outstanding) { self.failed=true;self.application.request_stop();return ISLE_GAME_INVALID_STATE; }
                }
            }
            if (error(&self.bridge)!=game_host::Error::none) {
                self.failed=true;self.application.request_stop();return ISLE_GAME_EXECUTION_FAILURE;
            }
            return ISLE_GAME_OK;
        }
    };
    void toggle_station(game_input::Service& input,snap_station::Device& station) noexcept {
        if (input.printer()) {
            input.bind_printer(nullptr);station.disconnect();feedback("disconnected (F9 to connect)");
        } else if (station_directory.empty()) feedback("no output directory configured");
        else if (station.connect(station_directory)) {
            input.bind_printer(&station);feedback("connected (F9 to disconnect)");
        } else feedback(station.message());
    }
    EpochResult run_epoch(game_input::Service& input,snap_station::Device& station,bool& completing_reset) noexcept {
        EpochResult result{};
        auto audio=game_audio::Driver::create(snapshot,audio_output());
        if (!audio) { retain(result.message,audio.message.data());cancel_input();return result; }
        auto& driver=*audio.driver;
        host::GtlClock clock({1,1,2,true,0});
        host::Progress progress(platform(),input,driver);
        bool initialized=false,platform_failed=false;
        std::unique_ptr<application::Runtime> runtime;
        std::optional<game::State> state;
        std::optional<NativeDisplay> native_display;
        try {
            feedback(station.available()?"connected (F9 to disconnect)":"disconnected (F9 to connect)");
            if (station.available()) input.bind_printer(&station);
            game_host::Input initial{};
            while (events(this)&&!initial.completed) {
                initial=initial_input();
                if (!initial.completed) {
                    std::unique_lock lock(mutex);
                    changed.wait_for(lock,std::chrono::milliseconds(1));
                }
            }
            if (initial.completed&&events(this)) {
                initialized=input.start(initial.ports)==game_input::Error::none;
                if (!initialized) retain(result.message,"actual controller initialization failed");
                else {
                    state.emplace(profile);
                    auto options=config;options.station=&station;options.application_epoch=epoch;
                    auto made=application::Runtime::create(snapshot,catalog,*state,input,driver.session(),clock,options);
                    if (!made) retain(result.message,made.message.data());
                    else {
                        runtime=std::move(made.runtime);
                        native_display.emplace(NativeDisplay{*this,*runtime,platform_failed,0});
                        progress.bind(clock,{runtime.get(),+[](void* owner,const game_input::EventResult& completion) noexcept {
                            return static_cast<application::Runtime*>(owner)->acknowledge_input(completion)==ISLE_GAME_OK;
                        }});
                        if (runtime->bind_post_course_progress(progress,{&*native_display,NativeDisplay::service})!=ISLE_GAME_OK)
                            runtime->request_stop();
                        if (completing_reset) {
                            completing_reset=false;
                            if (!station.complete_reset(epoch)) {
                                retain(result.message,station.message());platform_failed=true;runtime->request_stop();
                            }
                        }
                        auto native_actions=[&] {
                            // Only outer GAME code consumes reset/disconnect;
                            // Source/native leaves must already have unwound.
                            std::uint64_t ticket=0;
                            if (station.take_reset_request(ticket)) {
                                result.reset_requested=true;runtime->request_stop();progress.request_stop();
                                feedback("resetting after saved profile");
                            }
                            auto actions=take_station_actions();
                            while (actions--) {
                                const auto view=runtime->view();
                                const bool diagnostic=view.entry&&view.entry->operation==ISLE_APPLICATION_ENTRY_PRINTER_DIAGNOSTIC;
                                if (diagnostic||result.reset_requested) {
                                    result.disconnect_requested=true;runtime->request_stop();progress.request_stop();
                                    feedback("disconnecting after capture retirement");
                                } else toggle_station(input,station);
                            }
                            if (input.printer()&&station.error()!=snap_station::Error::none) {
                                retain(result.message,station.message());platform_failed=true;
                                runtime->request_stop();progress.request_stop();feedback(station.message());
                            }
                        };
                        while (!runtime->stopped()) {
                            progress.set_policy({true,true,runtime->scene_active(),true,true});
                            const auto tick=progress.poll();
                            if (progress.status()!=host::ProgressStatus::running) runtime->request_stop();
                            native_actions();
                            (void)runtime->pump(host::Progress::raw(tick));
                            native_actions();
                            if (runtime->error()!=ISLE_GAME_OK) progress.request_stop();
                            (void)NativeDisplay::service(&*native_display);
                            (void)progress.idle();
                        }
                        if (runtime->error()!=ISLE_GAME_OK) retain(result.message,runtime->message());
                        if (progress.status()==host::ProgressStatus::failed) retain(result.message,progress.message());
                        progress.unbind();
                        runtime.reset();native_display.reset();
                    }
                }
            }
        } catch (const std::exception& exception) { retain(result.message,exception.what()); }
        catch (...) { retain(result.message,"native GAME epoch failed"); }
        // Runtime methods are noexcept. Unexpected allocation/I/O failure after
        // admission still requests and services actual retirement, never unwinds
        // a live creator-affine Runtime or borrowed completed image.
        if (runtime) {
            runtime->request_stop();
            while (!runtime->stopped()) {
                progress.set_policy({false,true,false,false,true});
                const auto tick=progress.poll();
                (void)runtime->pump(host::Progress::raw(tick));
                (void)NativeDisplay::service(&*native_display);(void)progress.idle();
            }
            progress.unbind();runtime.reset();native_display.reset();
        }
        state.reset();
        if (initialized&&input.stop().error!=game_input::Error::none) retain(result.message,"actual Input stop failed");
        input.bind_printer(nullptr);cancel_input();
        if (result.disconnect_requested) station.disconnect();
        bool drained=false;
        if (error(this)==game_host::Error::none&&driver.stop()) {
            progress.set_deadline(Time::now()+std::chrono::seconds(30),"actual audio shutdown did not drain");
            while (!progress.deadline_expired()) {
                progress.set_policy({false,false,false,false,false});(void)progress.poll();
                if (error(this)!=game_host::Error::none||driver.error()!=game_audio::DriverError::none) break;
                const auto audio_state=driver.poll_drain();
                if (audio_state==game_audio::Drain::failed) break;
                if (audio_state==game_audio::Drain::drained) { drained=true;break; }
                (void)progress.idle();
            }
        }
        if (driver.error()!=game_audio::DriverError::none) retain(result.message,driver.message());
        if (error(this)!=game_host::Error::none) retain(result.message,message(this));
        if (progress.status()==host::ProgressStatus::failed) retain(result.message,progress.message());
        if (!drained) retain(result.message,"actual audio shutdown did not drain");
        if (!driver.retire()) retain(result.message,driver.message());
        result.successful=!result.message[0]&&!platform_failed&&drained;
        completing_reset=result.reset_requested&&!result.disconnect_requested;
        return result;
    }
    void run() noexcept {
#ifdef __linux__
        (void)pthread_setname_np(pthread_self(),"35mmIsle-GAME");
#endif
        game_input::Service input;
        snap_station::Device station;
        bool completing_reset=false;
        {
            std::lock_guard lock(mutex);ready=true;changed.notify_all();
        }
        for (;;) {
            {
                std::unique_lock lock(mutex);
                changed.wait(lock,[this] { return active||quitting; });
                if (quitting) break;
            }
            auto result=run_epoch(input,station,completing_reset);
            {
                std::lock_guard lock(mutex);
                if (call_state!=CallState::idle||display_state!=DisplayState::idle) std::terminate();
                outcome=result;active=false;retired=true;changed.notify_all();
            }
        }
        station.cancel();
    }
    void observe_platform() noexcept {
        const auto actual_error=host->error();
        const bool running=host->running();
        std::lock_guard lock(mutex);
        platform_running=running;
        if (platform_error==game_host::Error::none&&actual_error!=game_host::Error::none) {
            retain(platform_message,host->message());platform_error=actual_error;
        }
        if (!running||platform_error!=game_host::Error::none) stop_requested=true;
        changed.notify_all();
    }
    void service_input() noexcept {
        if (main_completion) {
            const auto published=transfer.submit_sample(*main_completion);
            if (published==game_input::Error::transfer_full) return;
            if (published!=game_input::Error::none) { transfer_failure("main Input transfer rejected its retained completion");return; }
            main_completion.reset();main_read={};changed.notify_all();
        }
        if (!main_read) {
            const auto received=transfer.acquire_request(main_read);
            if (received==game_input::Error::transfer_empty) return;
            if (received!=game_input::Error::none) { transfer_failure("main Input transfer rejected its actual request");return; }
        }
        if (host->error()!=game_host::Error::none||!host->running()) return;
        const auto actual=host->acquire_input();
        if (!actual.completed) return;
        main_completion=game_input::ReadCompletion{main_read,actual.ports};
        const auto published=transfer.submit_sample(*main_completion);
        if (published==game_input::Error::none) { main_completion.reset();main_read={};changed.notify_all(); }
        else if (published!=game_input::Error::transfer_full) transfer_failure("main Input transfer rejected an actual sample");
    }
    void service() noexcept {
        require_main();
        if (!host) return;
        (void)host->pump_events();observe_platform();
        while (host->take_station_action()) {
            std::lock_guard lock(mutex);
            if (station_actions==std::numeric_limits<std::uint32_t>::max()) {
                retain(platform_message,"native station action queue exhausted");platform_error=game_host::Error::input;stop_requested=true;
            } else ++station_actions;
        }
        service_input();
        MainCall function=nullptr;void* argument=nullptr;
        {
            std::lock_guard lock(mutex);
            if (call_state==CallState::pending) {
                function=call;argument=call_argument;call_state=CallState::executing;
            }
        }
        if (function) {
            function(*this,argument);
            std::lock_guard lock(mutex);call_state=CallState::completed;changed.notify_all();
        }
        DisplayOffer offered{};
        {
            std::lock_guard lock(mutex);
            if (display_state==DisplayState::offered) { offered=display;display_state=DisplayState::copying; }
        }
        if (offered.serial) {
            if (host->error()==game_host::Error::none) {
                if ((!host->presentation_pending()||host->cancel_present())&&
                    host->configure_present(offered.width,offered.height))
                    (void)host->queue_present(offered.rgba,offered.width,offered.height,offered.offsets);
            }
            // Copy or explicit decline has returned. This only ends main's
            // pixel access; GAME performs the real FrameToken/Station release.
            std::lock_guard lock(mutex);display_state=DisplayState::acknowledged;changed.notify_all();
        }
        if (host->error()==game_host::Error::none&&host->presentation_pending()) (void)host->try_present();
        observe_platform();
        std::unique_lock lock(mutex);
        changed.wait_for(lock,std::chrono::milliseconds(1),[this] {
            return call_state==CallState::pending||display_state==DisplayState::offered||retired;
        });
    }
};
Owner::Owner(const rom::Snapshot& snapshot,const menu::Catalog& catalog,application::RuntimeConfig config,
             std::filesystem::path directory)
    :impl_(std::make_unique<Impl>(snapshot,catalog,config,std::move(directory))) {}
Owner::~Owner() { finish(); }
bool Owner::start_epoch(game_host::Host& host,const std::filesystem::path& profile,std::uint64_t epoch) noexcept {
    auto& self=*impl_;self.require_main();
    std::lock_guard lock(self.mutex);
    if (self.active||self.quitting||!epoch||epoch<=self.epoch||
        self.call_state!=Impl::CallState::idle||self.display_state!=Impl::DisplayState::idle) return false;
    try { self.profile=profile; }
    catch (...) { retain(self.outcome.message,"native profile path allocation failed");return false; }
    self.host=&host;self.endpoint=host.audio_output();self.epoch=epoch;
    self.platform_error=host.error();self.platform_message={};
    if (self.platform_error!=game_host::Error::none) retain(self.platform_message,host.message());
    self.platform_running=host.running();self.stop_requested=!self.platform_running;
    self.station_actions=0;self.outcome={};self.retired=false;self.active=true;
    self.changed.notify_all();return true;
}
void Owner::service() noexcept { impl_->service(); }
void Owner::request_stop() noexcept {
    std::lock_guard lock(impl_->mutex);impl_->stop_requested=true;impl_->changed.notify_all();
}
bool Owner::epoch_retired() const noexcept {
    std::lock_guard lock(impl_->mutex);return impl_->retired;
}
EpochResult Owner::result() const noexcept {
    std::lock_guard lock(impl_->mutex);return impl_->outcome;
}
void Owner::finish() noexcept {
    if (!impl_) return;
    auto& self=*impl_;self.require_main();
    if (!self.worker.joinable()) return;
    request_stop();
    while (!epoch_retired()) service();
    {
        std::lock_guard lock(self.mutex);self.quitting=true;self.changed.notify_all();
    }
    self.worker.join();self.host=nullptr;
}
} // namespace isle::game_owner
