// Explicit private-ROM integration scenario, never a ROM-free CTest or game
// executable. Actual native Logo/Title/UI, canonical RSP/RDP, LUS OpenGL and
// checked SDL dummy PCM. Input is an explicitly owned software acquisition,
// NOT a fabricated physical/LUS connection. External scene requests end this
// scenario before gameplay; the game scene manager must perform that entry.
#include "scene_graphics.hpp"
#include "scene_storage.hpp"
#include "game_state.hpp"
#include "game_host.hpp"
#include "host_clock.hpp"
#include "application_manager.hpp"
#include "new_game_oak_driver.hpp"
#include "canonical_frame_capture.hpp"

#include <SDL3/SDL.h>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <thread>

namespace {
namespace s = isle::startup;
namespace g = isle::game_host;
namespace h = isle::host;
namespace in = isle::game_input;
void require(bool value, const char* reason) { if (!value) throw std::runtime_error(reason); }
void checked(h::ClockError value) { require(value == h::ClockError::none,"GTL action/lease transition rejected"); }
void offer_ticks(h::GtlClock& clock, std::uint64_t ticks) {
    if (clock.stopping() || clock.stopped() || ticks == 0) return;
    // Source sends nonblocking to four entries; surplus is counted/dropped,
    // never replayed as catch-up GAME updates after a real GPU stall.
    const auto result = clock.offer_retraces(ticks);
    require(result == h::ClockError::none || result == h::ClockError::full,
            "source GTL retrace queue closed or exhausted");
}
struct SoftwareController {
    std::uint16_t buttons = 0;
    std::int8_t stick_x = 0, stick_y = 0;
    in::HostFrame sample() const noexcept {
        in::HostFrame frame{};
        frame[0] = {in::Connection::present,static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(this)),0,buttons,stick_x,stick_y};
        return frame;
    }
};
void complete_read(in::Service& input, in::EventResult event, const SoftwareController& controller) {
    require(event.error == in::Error::none && bool(event.read),"explicit acquisition read request missing");
    const auto completed = input.complete_read(event.read,controller.sample());
    require(completed.error == in::Error::none,"actual owned software acquisition completion rejected");
}
enum class Scenario { roundtrip, full_title, reset_live_offsets, name_oak };
void create_existing_profile(const std::filesystem::path& path) {
    auto fixture = isle::save::GameSession::open(path,0x13579u);
    require(bool(fixture),"real preexisting profile fixture could not acquire its Store");
    require(!fixture.session.set_player_flag(16,true) && !fixture.session.set_screen_offsets(8,-6),
            "real preexisting profile fixture mutation failed");
    const auto committed = fixture.session.commit(0x13579u);
    require(committed.outcome == isle::save::CommitOutcome::published && committed.generation == 1 &&
            committed.durability != isle::save::Durability::none && !committed.error && !committed.cleanup_error,
            "preexisting profile fixture lacks an actual acknowledged durable commit");
}
void check_existing_profile(const std::filesystem::path& path) {
    // Called only after run's application State/Store lock has actually retired.
    auto cold = isle::save::GameSession::open(path,0);
    require(bool(cold) && cold.state == isle::save::GameLoadState::ready,"cold existing profile was not retained");
    const auto data = cold.session.snapshot();
    require(bool(data),"cold profile snapshot unavailable");
    const auto flag = data.value.player_flag(16);
    const auto screen = data.value.screen_offsets();
    require(flag && flag.value && screen && screen.value.horizontal == 8 && screen.value.vertical == -6 &&
            data.value.generation() == 1,"RAM-only new-game reset implicitly committed over the existing disk profile");
}
int run(const std::filesystem::path& rom, const std::filesystem::path& root, Scenario scenario) {
    const bool full_title = scenario == Scenario::full_title;
    const bool reset_offsets = scenario == Scenario::reset_live_offsets || scenario == Scenario::name_oak;
    const bool name_oak = scenario == Scenario::name_oak;
    for (const auto* name : {"SDL_AUDIO_DRIVER","SDL_AUDIODRIVER"}) {
        const char* value = std::getenv(name);
        require(value && std::string_view(value) == "dummy","private integration requires explicit SDL dummy audio; no physical driver selection");
    }
    require(root.is_absolute() && std::filesystem::is_directory(root) && !std::filesystem::is_symlink(root),
            "private integration needs an existing absolute nonsymlink owned root");
    require(std::filesystem::create_directory(root/"profile") && std::filesystem::create_directory(root/"host") &&
            std::filesystem::create_directory(root/"proof"),"private scenario child roots must be newly owned, never reused");
    if (reset_offsets) create_existing_profile(root/"profile");
    const auto snapshot = isle::rom::read_snapshot(rom);
    auto catalog = isle::menu::hydrate(snapshot);
    require(bool(catalog),"complete private startup resource hydration rejected");
    // A valid owned persisted config deliberately disagrees with this adapter's
    // required backend. ConsoleVariable's reload must not erase the override.
    {
        std::ofstream config(root/"host"/"35mmIsle.json");
        config << "{\"Window\":{\"Backend\":{\"Id\":123456},\"Width\":640,\"Height\":480}}\n";
        require(bool(config),"owned renderer-precedence configuration write failed");
    }
    g::Config configuration;
    configuration.data_directory = root/"host";
    configuration.input = g::InputMode::diagnostic_disabled;
    auto platform = g::Host::create(snapshot,std::move(configuration));
    require(bool(platform),platform.message.data());
    auto& host = *platform.host;
    require(SDL_GL_GetCurrentContext() != nullptr,"actual required OpenGL override was lost during CVar reload");
    const auto disabled = host.acquire_input();
    require(disabled.completed && disabled.controller_bits == 0 &&
        std::all_of(disabled.ports.begin(),disabled.ports.end(),[](const auto& p) {
            return p.connection == in::Connection::absent && p.identity == 0;
        }),"diagnostic-disabled input invented physical controller metadata");
    SoftwareController controller;
    in::Service input;
    require(input.start(controller.sample()) == in::Error::none,"owned software input initialization failed");
    isle::game::State game_state(root/"profile");
    isle::scene::Storage scene_storage;
    h::GtlClock clock({1,1,2,true,0});
    auto application = isle::application::Manager::create(snapshot,game_state,input,*host.audio(),clock,*catalog.catalog);
    require(bool(application),application.message.data());
    auto& manager = *application.manager;
    require(&manager.screen_offsets() == &game_state.screen_offsets(),
            "Manager must expose the actual stable State-owned four-endpoint VI record");
    require(manager.resume()==ISLE_GAME_OK,manager.message());
    const auto* initial_entry=manager.pending_entry();
    require(initial_entry&&initial_entry->operation==ISLE_APPLICATION_ENTRY_BOOT,
        "original application manager did not request Boot first");
    auto entry_borrow=manager.borrow_entry(initial_entry->serial);
    require(bool(entry_borrow),"actual application Boot entry borrow failed");
    // Reverse destruction order is essential on every thrown failure: actual
    // graphics drain/join, then callbacks/native objects, then host audio/SDL.
    std::unique_ptr<s::Runtime> runtime;
    auto renderer = isle::scene::graphics::Frame::create(*catalog.catalog,snapshot,{true,{12000000,4*1024*1024}});
    require(bool(renderer),renderer.message.data());
    auto& graphics = *renderer.frame;
    auto started = s::Runtime::create_boot(*catalog.catalog,input,*host.audio(),game_state,scene_storage,0);
    require(bool(started),started.message.data());
    runtime = std::move(started.runtime);
    require(runtime->save_initialization() == nullptr,"save initialization ran before the actual first logo wait");
    h::RetraceClock retraces(2250000,37609);
    const auto epoch = std::chrono::steady_clock::now();
    const auto deadline = epoch+std::chrono::seconds(1800);
    std::uint64_t ordinal = 0, canonical_frames = 0, presentations = 0, first_menu_update = 0;
    std::uint64_t busy_audio_begin = 0, greatest_busy_audio_progress = 0;
    std::optional<h::FrameToken> executing;
    bool saw_title = false, saw_main_music = false, saw_aux_music = false, menu_capture = false;
    bool saw_save_after_wait = false, saw_logo = false, saw_menu = false, queued_menu_ready = false;
    bool logo_capture = false, title_capture = false;
    bool queued_logo_ready = false, queued_title_ready = false;
    bool calibration_exercised = false;
    std::uint8_t last_logo_rgb = 255;
    unsigned fade_changes = 0, stage = 0;
    std::uint64_t stable_since = 0;
    const auto set_pad = [&](std::uint16_t buttons,std::int8_t x=0,std::int8_t y=0) {
        controller.buttons=buttons; controller.stick_x=x; controller.stick_y=y;
        stable_since=runtime->updates();
    };
    const auto expect_live = [&](int horizontal,int vertical) {
        const auto& live = manager.screen_offsets();
        require(&live == &game_state.screen_offsets() && live.horizontal_start == horizontal &&
                live.horizontal_end == horizontal && live.vertical_start == vertical && live.vertical_end == vertical,
                "actual Source VI service did not publish all four endpoints into canonical State");
        if (runtime) {
            const auto view = runtime->view();
            require(view.screen_offsets.horizontal == horizontal && view.screen_offsets.vertical == vertical,
                    "Startup render projection did not use canonical live VI");
        }
    };
    const auto expect_saved_calibration = [&] {
        const auto saved = game_state.save()->snapshot();
        require(bool(saved),"calibration source settings snapshot unavailable");
        const auto screen = saved.value.screen_offsets();
        require(screen && screen.value.horizontal == 8 && screen.value.vertical == -6 &&
                saved.value.generation() == 1,
                "live calibration mutated saved settings before actual options exit");
    };
    for (;;) {
        const auto now = std::chrono::steady_clock::now();
        require(now < deadline,"actual startup integration wall deadline expired; no completion claimed");
        require(host.pump_events(),host.message());
        const auto ns = static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(now-epoch).count());
        const auto ticks = retraces.advance(ns);
        for (std::uint64_t n = 0; n < ticks; ++n) {
            complete_read(input,input.retrace(),controller);
            require(host.notify_retrace(++ordinal,ns),host.message());
        }
        offer_ticks(clock,ticks);
        require(host.pump_audio(),host.message());
        saw_main_music |= host.audio()->bgm_playing(0);
        saw_aux_music |= host.audio()->bgm_playing(1);
        if (executing) {
            const auto result = graphics.poll();
            require(result != isle::scene::graphics::Poll::failed,graphics.message());
            if (result == isle::scene::graphics::Poll::completed) {
                require(!graphics.busy() && graphics.quiescent(),"completed graphics retained actual RAM/native/GPU borrowers");
                greatest_busy_audio_progress = std::max(greatest_busy_audio_progress,host.audio()->accepted_frames()-busy_audio_begin);
                checked(clock.offer_terminal(*executing,h::Terminal::completed));
                executing.reset();
                const auto frame = graphics.completed();
                require(frame && frame.rgba.size() == 320u*240u*4u,"actual framebuffer readback has wrong complete extent");
                if (!host.presentation_pending()) {
                    const auto* ui = runtime->menu_ui();
                    queued_menu_ready = ui && ui->source_state() == 3 && runtime->updates()-stable_since >= 30;
                    queued_logo_ready = runtime->phase() == s::Phase::logo &&
                        runtime->logo_counter() >= 10 && runtime->logo_counter() < 60;
                    const auto prepared_view = runtime->view();
                    queued_title_ready = prepared_view.title && prepared_view.title->active() &&
                        prepared_view.title->source_time() >= 50;
                    if(!logo_capture && queued_logo_ready)
                        isle::private_capture::write_frame(root/"proof"/"canonical-logo-frame.ppm",frame.rgba);
                    if(!title_capture && queued_title_ready)
                        isle::private_capture::write_frame(root/"proof"/"canonical-title-frame.ppm",frame.rgba);
                    if(!menu_capture && queued_menu_ready)
                        isle::private_capture::write_frame(root/"proof"/"canonical-menu-frame.ppm",frame.rgba);
                    require(host.queue_present(frame.rgba,frame.screen_offsets),host.message());
                }
                ++canonical_frames;
            }
        }
        if (host.presentation_pending()) {
            const auto shown = host.try_present();
            require(shown != g::Present::failed,host.message());
            if (shown == g::Present::completed) {
                ++presentations;
                // This tag belongs to the retained copied presentation, not
                // whatever native UI state has advanced to during a decline.
                if (!logo_capture && queued_logo_ready) {
                    logo_capture = true;
                    std::cout<<"startup_surface_ready stage=logo"<<std::endl;
                }
                if (!title_capture && queued_title_ready) {
                    title_capture = true;
                    std::cout<<"startup_surface_ready stage=title"<<std::endl;
                }
                if (!menu_capture && queued_menu_ready) {
                    menu_capture = true;
                    std::cout<<"startup_surface_ready stage=menu"<<std::endl;
                }
            }
        }
        if (!executing) {
            const auto action = clock.poll();
            if (action.kind == h::ActionKind::update) {
                require(graphics.quiescent(),"native tick attempted before actual graphics retirement");
                const auto publication = input.update();
                require(publication.error == in::Error::none && publication.accepted &&
                        publication.acknowledged == publication.accepted,"contUpdate did not receive a real publication acknowledgement");
                const auto step = runtime->tick(static_cast<std::uint32_t>(h::RetraceClock::guest_ticks(ns)));
                require(step != s::Step::failed,runtime->message());
                const auto view = runtime->view();
                if (runtime->phase() == s::Phase::logo) {
                    saw_logo = true;
                    if (runtime->updates() == 1) require(runtime->save_initialization() == nullptr,"save initialization preceded first source wait");
                    if (runtime->updates() == 2) {
                        require(runtime->save_initialization() && *runtime->save_initialization(),"source logo did not initialize save after its first wait");
                        saw_save_after_wait = true;
                        expect_live(0,0);
                    }
                    IsleSprite* sprite = nullptr;
                    require(isle_game_sprite_view(view.game,view.logo_sprite,&sprite) == ISLE_GAME_OK,"live logo sprite state missing");
                    require(sprite->red == sprite->green && sprite->green == sprite->blue,"actual source logo RGB fade diverged between channels");
                    if (sprite->red != last_logo_rgb) {
                        require(runtime->logo_counter() >= 60,"actual logo faded before its concurrent minimum hold");
                        ++fade_changes; last_logo_rgb = sprite->red;
                    }
                }
                checked(clock.finish_update());
                if (step == s::Step::boot_complete) {
                    require(saw_save_after_wait && runtime->controller() == 0 && fade_changes == 20 && last_logo_rgb == 0,
                            "complete source logo/controller/save/20-step RGB transition was not observed");
                    clock.request_stop(); clock.poll();
                    require(clock.stopped(),"old logo GTL epoch did not actually quiesce");
                    require(runtime->retire(),runtime->message());
                    runtime.reset();
                    const auto boot_serial=entry_borrow.intent().serial;
                    entry_borrow={};
                    require(manager.complete_entry(boot_serial,0)==ISLE_GAME_OK,manager.message());
                    checked(clock.reset({1,1,2,true,0}));
                    require(manager.resume()==ISLE_GAME_OK,manager.message());
                    const auto* title_entry=manager.pending_entry();
                    require(title_entry&&title_entry->operation==ISLE_APPLICATION_ENTRY_TITLE&&title_entry->argument==8,
                        "original application manager did not select the real title entry after Boot");
                    entry_borrow=manager.borrow_entry(title_entry->serial);
                    require(bool(entry_borrow),"actual title entry lifetime borrow failed");
                    expect_live(reset_offsets?8:0,reset_offsets?-6:0);
                    auto title=s::Runtime::create_title(*catalog.catalog,input,*host.audio(),game_state,scene_storage,
                        static_cast<std::uint32_t>(h::RetraceClock::guest_ticks(ns)));
                    require(bool(title),title.message.data());
                    runtime=std::move(title.runtime);
                    first_menu_update=runtime->updates();
                    saw_menu=true;
                    continue;
                } else if (step == s::Step::scene_exit) {
                    require(stage == (reset_offsets?17u:9u) &&
                            runtime->exit().scene == isle::menu::scene::SourceScene::beach &&
                            runtime->exit().entry_flags == 1,
                            "actual menu did not produce source result0/new-game notice1 after confirmation");
                    if (reset_offsets) {
                        const auto data = game_state.save()->snapshot();
                        require(bool(data),"reset source snapshot missing");
                        const auto screen = data.value.screen_offsets();
                        const auto flag = data.value.player_flag(16);
                        require(screen && screen.value.horizontal == 0 && screen.value.vertical == 0 &&
                                flag && !flag.value && data.value.generation() == 1,
                                "new-game confirmation failed to reset RAM while preserving durable generation");
                    }
                    clock.request_stop(); clock.poll();
                    require(clock.stopped(),"menu GTL exit retained a context");
                    break;
                }
                const auto* ui = runtime->menu_ui();
                auto* title = view.title;
                if (title) saw_title |= title->active();
                if (ui) {
                    if (reset_offsets && !(stage >= 100 && stage <= 115))
                        require(ui->screen_offsets().horizontal == 8 && ui->screen_offsets().vertical == -6,
                                "RAM-only new-game reset incorrectly changed live VI offsets during the source exit wait");
                    const auto state = ui->source_state();
                    if (stage == 0 && saw_title) {
                        if (!full_title && title->active() && title->source_time() >= 100) { controller.buttons = 0x1000; stage = 1; }
                        else if (full_title && state == 2 && !title->active()) { stage = 2; stable_since = runtime->updates(); }
                    } else if (stage == 1 && state == 2 && !title->active()) { controller.buttons = 0; stage = 2; stable_since = runtime->updates(); }
                    else if (stage == 2 && runtime->updates()-stable_since >= 30) { controller.buttons = 0x8000; stage = 3; }
                    else if (stage == 3 && state == 3) { controller.buttons = 0; stage = 4; stable_since = runtime->updates(); }
                    else if (stage == 4 && runtime->updates()-stable_since >= 30) { controller.buttons = 0x4000; stage = 5; }
                    else if (stage == 5 && state == 2) { controller.buttons = 0; stage = 6; stable_since = runtime->updates(); }
                    else if (stage == 6 && runtime->updates()-stable_since >= 30) { controller.buttons = 0x1000; stage = 7; }
                    else if (stage == 7 && state == 3) { controller.buttons = 0; stage = 8; stable_since = runtime->updates(); }
                    else if (stage == 8 && runtime->updates()-stable_since >= 30 && menu_capture) {
                        if (scenario == Scenario::reset_live_offsets && !calibration_exercised) {
                            set_pad(0,0,-80); stage=100; // Continue -> Options.
                        } else
                        if (reset_offsets) { set_pad(0,0,80); stage=10; }
                        else { controller.buttons=0x8000; stage=9; }
                    } else if (stage==10 && runtime->updates()-stable_since>=4) { set_pad(0); stage=11; }
                    else if (stage==11 && runtime->updates()-stable_since>=30) { set_pad(0x8000); stage=12; }
                    else if (stage==12 && runtime->updates()-stable_since>=4) { set_pad(0); stage=13; }
                    else if (stage==13 && runtime->updates()-stable_since>=60) { set_pad(0,-80,0); stage=14; }
                    else if (stage==14 && runtime->updates()-stable_since>=4) { set_pad(0); stage=15; }
                    else if (stage==15 && runtime->updates()-stable_since>=30) { set_pad(0x8000); stage=16; }
                    else if (stage==16 && runtime->updates()-stable_since>=4) { set_pad(0); stage=17; }
                    else if (stage==100 && runtime->updates()-stable_since>=4) { set_pad(0); stage=101; }
                    else if (stage==101 && runtime->updates()-stable_since>=30) { set_pad(0x8000); stage=102; }
                    else if (stage==102 && state==8) { set_pad(0); stage=103; }
                    else if (stage==103 && runtime->updates()-stable_since>=80) { set_pad(0x8000); stage=104; }
                    else if (stage==104 && runtime->updates()-stable_since>=4) { set_pad(0); stage=105; }
                    else if (stage==105 && runtime->updates()-stable_since>=30) { set_pad(0,-80); stage=106; }
                    else if (stage==106 && runtime->updates()-stable_since>=4) { set_pad(0); stage=107; }
                    else if (stage==107 && runtime->updates()-stable_since>=4) {
                        expect_live(7,-6); expect_saved_calibration(); set_pad(0x4000); stage=108;
                    } else if (stage==108 && runtime->updates()-stable_since>=4) { set_pad(0); stage=109; }
                    else if (stage==109 && runtime->updates()-stable_since>=40) {
                        expect_live(8,-6); expect_saved_calibration(); set_pad(0x8000); stage=110;
                    } else if (stage==110 && runtime->updates()-stable_since>=4) { set_pad(0); stage=111; }
                    else if (stage==111 && runtime->updates()-stable_since>=30) { set_pad(0x1000); stage=112; }
                    else if (stage==112 && runtime->updates()-stable_since>=4) { set_pad(0); stage=113; }
                    else if (stage==113 && runtime->updates()-stable_since>=4) {
                        expect_live(0,0); expect_saved_calibration(); set_pad(0x4000); stage=114;
                    } else if (stage==114 && runtime->updates()-stable_since>=4) { set_pad(0); stage=115; }
                    else if (stage==115 && runtime->updates()-stable_since>=40) {
                        expect_live(8,-6); expect_saved_calibration(); set_pad(0x4000); stage=116;
                    } else if (stage==116 && runtime->updates()-stable_since>=4) { set_pad(0); stage=117; }
                    else if (stage==117 && state==2) {
                        expect_live(8,-6); calibration_exercised=true; set_pad(0); stage=6;
                    }
                }
            } else if (action.kind == h::ActionKind::draw) {
                require(graphics.begin(runtime->view(),{static_cast<unsigned>(canonical_frames&1)}),graphics.message());
                checked(clock.accept_draw(action.token));
                executing = action.token;
                busy_audio_begin = host.audio()->accepted_frames();
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    require(saw_logo && saw_menu && saw_title && logo_capture && title_capture && menu_capture &&
            saw_main_music && greatest_busy_audio_progress != 0,
            "real startup, visible logo/title/menu or independent audio-during-GPU behavior was not exercised");
    if (full_title) require(saw_aux_music,"natural title did not actually start the auxiliary music context");
    if (scenario == Scenario::reset_live_offsets)
        require(calibration_exercised,"actual calibration preview/B-cancel/Start reset source path was not exercised");
    require(graphics.quiescent(),graphics.message());
    std::uint64_t reentry_frames = 0;
    isle::private_scene::NameOakMetrics scene_metrics;
    if (name_oak) {
        const auto title_result=static_cast<std::int32_t>(runtime->exit().scene);
        require(runtime->retire(),runtime->message());
        runtime.reset();
        require(host.cancel_present(),host.message());
        const auto title_serial=entry_borrow.intent().serial;
        entry_borrow={};
        require(manager.complete_entry(title_serial,title_result)==ISLE_GAME_OK,manager.message());
        checked(clock.reset({1,1,2,true,0}));
        require(manager.resume()==ISLE_GAME_OK,manager.message());
        clock.request_stop();clock.poll();
        require(clock.stopped(),"source manager handoff retained a Name clock context");
        scene_metrics = isle::private_scene::exercise_name_to_oak(snapshot,game_state,scene_storage,
            input,host,graphics,clock,retraces,ordinal,epoch,deadline,root,manager);
    } else {
    auto* const persistent_save = game_state.save();
    auto* const initialization_evidence = game_state.save_initialization();
    auto* const native_backing = scene_storage.game();
    require(persistent_save && initialization_evidence && game_state.scene_notices() == 1,
            "actual source new-game notice/profile was not retained at the manager boundary");
    require(!persistent_save->set_player_flag(12,true),"source-owned reentry profile mutation failed");
    auto old_runtime = std::move(runtime);
    require(old_runtime->retire(),old_runtime->message());
    const auto reentry_ns = static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now()-epoch).count());
    auto reentered = s::Runtime::create_title(*catalog.catalog,input,*host.audio(),game_state,scene_storage,
        static_cast<std::uint32_t>(h::RetraceClock::guest_ticks(reentry_ns)));
    require(bool(reentered),reentered.message.data());
    runtime = std::move(reentered.runtime);
    require(scene_storage.game() == native_backing && runtime->phase() == s::Phase::title_menu &&
            runtime->logo_counter() == 0 && game_state.save() == persistent_save &&
            game_state.save_initialization() == initialization_evidence,
            "title reentry replayed boot, changed backing or replaced the persistent profile owner");
    if (reset_offsets) expect_live(8,-6);
    // The retired old wrapper must not destroy new content in borrowed Storage.
    old_runtime.reset();
    auto boot_again = s::Runtime::create_boot(*catalog.catalog,input,*host.audio(),game_state,scene_storage,0);
    require(!boot_again && scene_storage.game() == native_backing,
            "second boot reused an initialized profile or destroyed the live reentered native scene");
    controller.buttons = 0;
    require(host.cancel_present(),host.message());
    checked(clock.reset({1,1,2,true,0}));
    unsigned prompt_presentations = 0;
    bool queued_prompt = false;
    for (;;) {
        const auto now = std::chrono::steady_clock::now();
        require(now < deadline,"actual persistent-state title reentry deadline expired");
        require(host.pump_events(),host.message());
        const auto ns = static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(now-epoch).count());
        const auto ticks = retraces.advance(ns);
        for (std::uint64_t n=0;n<ticks;++n) {
            complete_read(input,input.retrace(),controller);
            require(host.notify_retrace(++ordinal,ns),host.message());
        }
        offer_ticks(clock,ticks);
        require(host.pump_audio(),host.message());
        if (executing) {
            const auto result = graphics.poll();
            require(result != isle::scene::graphics::Poll::failed,graphics.message());
            if (result == isle::scene::graphics::Poll::completed) {
                require(graphics.quiescent(),"reentry graphics retained native/readback borrowers");
                checked(clock.offer_terminal(*executing,h::Terminal::completed));
                executing.reset();
                const auto frame = graphics.completed();
                if (!host.presentation_pending()) {
                    queued_prompt = runtime->menu_ui() && runtime->menu_ui()->source_state() == 2;
                    if(queued_prompt && prompt_presentations==9)
                        isle::private_capture::write_frame(root/"proof"/"canonical-reentered-title-frame.ppm",frame.rgba);
                    require(host.queue_present(frame.rgba,frame.screen_offsets),host.message());
                }
                ++reentry_frames;
            }
        }
        if (host.presentation_pending()) {
            const auto shown = host.try_present();
            require(shown != g::Present::failed,host.message());
            if (shown == g::Present::completed && queued_prompt && prompt_presentations < 10) {
                if (++prompt_presentations == 10)
                    std::cout<<"startup_surface_ready stage=reentered_title"<<std::endl;
            }
        }
        if (prompt_presentations == 10) clock.request_stop();
        if (!executing) {
            const auto action = clock.poll();
            if (action.kind == h::ActionKind::stopped) break;
            if (action.kind == h::ActionKind::update) {
                const auto publication = input.update();
                require(publication.error == in::Error::none && publication.accepted &&
                        publication.acknowledged == publication.accepted,"reentry contUpdate lacked actual publication");
                require(runtime->tick(static_cast<std::uint32_t>(h::RetraceClock::guest_ticks(ns))) == s::Step::updated,
                        runtime->message());
                require(runtime->logo_counter() == 0 && !runtime->view().title->active(),
                        "source title-only reentry unexpectedly replayed logo/movie");
                checked(clock.finish_update());
            } else if (action.kind == h::ActionKind::draw) {
                require(graphics.begin(runtime->view(),{static_cast<unsigned>(reentry_frames&1)}),graphics.message());
                checked(clock.accept_draw(action.token));
                executing = action.token;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    const auto reentry_snapshot = persistent_save->snapshot();
    require(reentry_snapshot && reentry_snapshot.value.player_flag(12).value &&
            game_state.save() == persistent_save && game_state.scene_notices() == 1,
            "source reentry lost profile mutation/lock ownership or consumed Oak's notice");
    }
    require(graphics.quiescent() && graphics.retire(),graphics.message());
    if (runtime) {
        require(runtime->retire(),runtime->message());
        runtime.reset();
    }
    entry_borrow={};
    require(manager.cancel()==ISLE_GAME_OK&&manager.retire()==ISLE_GAME_OK,manager.message());
    const auto stopped_input = input.stop();
    require(stopped_input.error == in::Error::none && !input.running() &&
            std::none_of(stopped_input.cancelled.begin(),stopped_input.cancelled.end(),[](const auto& token) { return bool(token); }),
            "actual input shutdown retained an unacknowledged publication");
    require(host.cancel_present(),host.message());
    require(host.stop_audio(),host.message());
    for (;;) {
        const auto now = std::chrono::steady_clock::now();
        require(now < deadline,"actual SDL/native audio retirement deadline expired");
        require(host.pump_events(),host.message());
        const auto ns = static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(now-epoch).count());
        const auto ticks = retraces.advance(ns);
        for (std::uint64_t n = 0; n < ticks; ++n) require(host.notify_retrace(++ordinal,ns),host.message());
        require(host.pump_audio(),host.message());
        const auto drain = host.poll_audio_drain();
        require(drain != g::Drain::failed,host.message());
        if (drain == g::Drain::drained) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    const auto accepted_frames = host.audio()->accepted_frames();
    const auto closed = host.shutdown();
    require(!closed.failed && closed.owners_released && closed.audio == isle::audio::SdlAudioOutput::CloseDisposition::stream_drained,
            "actual platform/audio/GUI/worker ownership did not retire after observed stream drain");
    std::cout << "native_startup_runtime=passed title_mode=" << (full_title?"natural_end":"actual_Start_interrupt")
              << " canonical_frames=" << canonical_frames << " real_LUS_presentations=" << presentations
              << " persistent_profile_title_reentry=" << (name_oak?"not_requested":"passed")
              << " reentry_canonical_frames=" << reentry_frames
              << " name_to_Oak_source_dispatch=" << (name_oak?"passed":"not_requested")
              << " name_canonical_frames=" << scene_metrics.name_frames
              << " name_LUS_presentations=" << scene_metrics.name_presentations
              << " Oak_canonical_frames=" << scene_metrics.oak_frames
              << " Oak_LUS_presentations=" << scene_metrics.oak_presentations
              << " Oak_confirmed_course_request=" << (name_oak?"0_not_yet_gameplay":"not_requested")
              << " live_VI_reset_scenario=" << (reset_offsets?"passed":"not_requested")
              << " accepted_dummy_PCM_frames=" << accepted_frames << " max_audio_frames_during_GPU_wait=" << greatest_busy_audio_progress
              << " menu_first_update=" << first_menu_update << " gtl_queue_dropped_retraces=" << clock.stats().dropped_retraces
              << " logo_RGB_changes=20 menu=A_to_main_B_to_prompt_Start_to_main new_game_source_scene=0 entry_flags=1"
                 " canonical_RGBA_capture=private_PPM_NOT_window external_window_visual_inspection=required input=explicit_owned_software_NOT_LUS_or_physical"
                 " audio=actual_RSP_checked_SDL_dummy_drained gameplay_Original_numerical_hardware_parity=not_claimed\n";
    return 0;
}
} // namespace
#ifdef _WIN32
int wmain(int argc,wchar_t** argv) {
#else
int main(int argc,char** argv) {
#endif
    try {
        require(argc == 4,"expected PRIVATE_ROM NEW_EXISTING_OWNED_ROOT MODE(roundtrip|full-title|reset-live-offsets|name-oak)");
        const auto mode = std::filesystem::path(argv[3]).u8string();
        require(mode == u8"roundtrip" || mode == u8"full-title" || mode == u8"reset-live-offsets" || mode == u8"name-oak",
                "unknown source startup scenario");
        const auto scenario = mode == u8"full-title"?Scenario::full_title:
                              mode == u8"reset-live-offsets"?Scenario::reset_live_offsets:
                              mode == u8"name-oak"?Scenario::name_oak:Scenario::roundtrip;
        const std::filesystem::path root(argv[2]);
        const auto status = run(std::filesystem::path(argv[1]),root,scenario);
        if (scenario == Scenario::reset_live_offsets) check_existing_profile(root/"profile");
        return status;
    } catch (const std::exception& e) { std::cerr << "private actual startup integration: " << e.what() << '\n'; return 1; }
}
