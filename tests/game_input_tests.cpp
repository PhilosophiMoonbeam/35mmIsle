#include "35mmisle/game_input.hpp"

#include <array>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <thread>

namespace g = isle::game_input;
namespace {
int failures = 0;
void check(bool value, std::string_view message) {
    if (!value) { ++failures; std::cerr << "FAIL: " << message << '\n'; }
}
g::HostFrame connected() {
    g::HostFrame frame{};
    for (std::size_t i = 0; i < g::port_count; ++i) {
        frame[i].connection = g::Connection::present;
        frame[i].identity = i + 100;
    }
    return frame;
}
g::EventResult read(g::Service& service, const g::HostFrame& frame, bool fresh = false) {
    const auto request = fresh ? service.read_and_update() : service.retrace();
    check(request.error == g::Error::none && bool(request.read), "explicit read request issued");
    const auto completion = service.complete_read(request.read, frame);
    check(completion.error == g::Error::none, "matching read completed");
    if (fresh) check(completion.acknowledged == request.accepted, "fresh request really acknowledged");
    return completion;
}
void accumulation_and_stability() {
    g::Service service;
    auto frame = connected();
    check(service.start(frame) == g::Error::none, "four-port initialization");
    const auto* address = &service.snapshot();
    frame[0].buttons = 0x8000;
    frame[0].stick_x = -103;
    read(service, frame);
    check(service.snapshot().primary().buttons == 0 && service.needs_publication(),
          "host read cannot mutate stable gameplay values");
    frame[0].buttons = 0;
    frame[0].stick_x = 37;
    read(service, frame);
    const auto update = service.update();
    const auto& pad = service.snapshot().primary();
    check(update.accepted == update.acknowledged && bool(update.acknowledged),
          "already-read update publishes and acknowledges immediately");
    check(pad.buttons == 0 && pad.pressed == 0x8000 && pad.released == 0x8000 &&
          pad.held == 0x8000 && pad.stick_x == 37,
          "press and release between publications both survive with latest raw axes");
    check(&service.snapshot() == address && !service.needs_publication(), "fixed stable snapshot storage");
    read(service, frame, true);
    check(pad.pressed == 0 && pad.released == 0 && pad.held == 0,
          "only publication clears accumulated edges");
}
void hold_boundaries_and_changed_buttons() {
    g::Service service;
    auto frame = connected();
    check(service.start(frame) == g::Error::none, "hold service starts");
    frame[0].buttons = 1;
    read(service, frame, true);
    check(service.snapshot().primary().held == 1, "initial press counts as held");
    for (int i = 1; i < 30; ++i) {
        read(service, frame, true);
        check(service.snapshot().primary().held == 0, "no repeat before thirty unchanged reads");
    }
    read(service, frame, true);
    check(service.snapshot().primary().held == 1, "repeat on thirtieth unchanged read");
    for (int i = 1; i < 5; ++i) {
        read(service, frame, true);
        check(service.snapshot().primary().held == 0, "five-read repeat interval excludes early reads");
    }
    read(service, frame, true);
    check(service.snapshot().primary().held == 1, "repeat at five-read boundary");
    frame[0].buttons = 3;
    read(service, frame, true);
    check(service.snapshot().primary().pressed == 2 && service.snapshot().primary().held == 2,
          "changed button set resets delay and holds only newly pressed buttons");
    frame[0].buttons = 1;
    read(service, frame, true);
    check(service.snapshot().primary().released == 2 && service.snapshot().primary().held == 0,
          "release also resets delay without repeating surviving button");
    check(service.set_hold(2, 1) == g::Error::none, "hold parameters update");
    for (int i = 1; i < 30; ++i) {
        read(service, frame, true);
        check(service.snapshot().primary().held == 0, "parameter change does not reset current countdown");
    }
    read(service, frame, true);
    check(service.snapshot().primary().held == 1, "old countdown expires");
    read(service, frame, true);
    check(service.snapshot().primary().held == 1, "new interval applies after countdown expires");
    frame[0].buttons = 5;
    read(service, frame, true);
    read(service, frame, true);
    check(service.snapshot().primary().held == 0, "new delay first unchanged read");
    read(service, frame, true);
    check(service.snapshot().primary().held == 5, "new delay second unchanged read");
}
void ports_errors_and_reconnect() {
    g::Service service;
    g::HostFrame empty{};
    check(service.start(empty) == g::Error::none && service.snapshot().controller_count == 0,
          "no host controllers is actually empty, not an invented connected pad");
    auto frame = connected();
    for (std::size_t i = 0; i < g::port_count; ++i) {
        frame[i].buttons = static_cast<std::uint16_t>(1u << i);
        frame[i].stick_x = static_cast<std::int8_t>(-20 - static_cast<int>(i));
        frame[i].stick_y = static_cast<std::int8_t>(20 + static_cast<int>(i));
    }
    read(service, frame, true);
    for (std::size_t i = 0; i < g::port_count; ++i)
        check(service.snapshot().indices[i] == static_cast<int>(i) &&
              service.snapshot().pads[i].pressed == (1u << i) &&
              service.snapshot().pads[i].stick_y == frame[i].stick_y,
              "all four physical ports preserve separate buttons axes and ordered indices");
    frame[0].error = 7;
    frame[0].buttons = 0;
    frame[0].stick_x = 99;
    frame[2] = {};
    read(service, frame, true);
    const auto& snapshot = service.snapshot();
    check(snapshot.controller_count == 2 && snapshot.indices == std::array<int, 4>{1, 3, -1, -1},
          "errored and absent controllers excluded with trailing minus-one indices");
    check(snapshot.primary().buttons == 1 && snapshot.primary().stick_x == -20 &&
          snapshot.primary().pressed == 1 && snapshot.pads[2].buttons == 4,
          "source freezes even published edge fields on error or disconnect");
    check(snapshot.primary().buttons != snapshot.pads[1].buttons &&
          snapshot.ports[0].error == 7 && snapshot.ports[2].connection == g::Connection::absent,
          "port-zero scalar aliases are not remapped and actual failure metadata is exposed");
    frame[0].error = 0;
    frame[0].buttons = 1;
    frame[0].identity = 900; // Physical replacement still uses the source's retained port state.
    frame[2] = connected()[2];
    frame[2].buttons = 0;
    read(service, frame, true);
    check(snapshot.primary().pressed == 0 && snapshot.primary().released == 0 &&
          snapshot.ports[0].identity == 900 && snapshot.pads[2].released == 4 &&
          snapshot.controller_count == 4, "reconnect compares retained buttons, not synthetic zero state");

    frame[1].buttons = 0x100;
    read(service, frame); // Accumulation not yet published.
    frame[1].error = 3;
    read(service, frame);
    check(service.update().error == g::Error::none, "failed-port publication completes");
    frame[1].error = 0;
    frame[1].buttons = 0;
    read(service, frame, true);
    check(snapshot.pads[1].pressed == 0x100 && snapshot.pads[1].released == 0x102,
          "error does not discard unreported press or reconnect release accumulation");
}
void publication_protocol_and_stop() {
    g::Service service, foreign;
    auto frame = connected();
    check(service.update().error == g::Error::stopped, "stopped service rejects update");
    check(service.start(frame) == g::Error::none, "protocol service starts");
    const auto pending = service.update();
    check(bool(pending.accepted) && !pending.acknowledged && !pending.read &&
          service.pending_update() == pending.accepted, "update with no read stays pending, not fake-ready");
    check(service.update().error == g::Error::pending_update, "cannot overwrite accepted waiter");
    check(service.set_update_every_tick(false) == g::Error::none, "disable every-tick reads");
    const auto skipped = service.retrace();
    check(skipped.error == g::Error::none && !skipped.read && !skipped.acknowledged &&
          service.pending_update() == pending.accepted, "disabled retrace cannot fake pending acknowledgement");
    frame[0].buttons = 8;
    const auto fresh = service.read_and_update();
    check(bool(fresh.read) && bool(fresh.accepted) && !fresh.acknowledged,
          "fresh read requires real acquisition even while update is pending");
    check(service.retrace().error == g::Error::read_in_flight &&
          service.set_update_every_tick(true) == g::Error::read_in_flight,
          "serialized source read blocks later controller events");
    auto invalid = frame;
    invalid[0].identity = 0;
    check(service.complete_read(fresh.read, invalid).error == g::Error::invalid_sample &&
          service.pending_read() == fresh.read, "bad acquisition cannot consume read or acknowledge");
    check(foreign.complete_read(fresh.read, frame).error == g::Error::stale_token,
          "foreign service cannot accept another owner read");
    const auto completed = service.complete_read(fresh.read, frame);
    check(completed.acknowledged == fresh.accepted && service.pending_update() == pending.accepted &&
          !service.needs_publication(), "fresh completion publishes but does not acknowledge waiting contUpdate");
    check(service.complete_read(fresh.read, frame).error == g::Error::stale_token,
          "duplicate completion never republishes or acknowledges twice");
    check(service.set_update_every_tick(true) == g::Error::none, "enable sampling independently of presentation");
    const auto retrace = service.retrace();
    check(!retrace.acknowledged && service.pending_update() == pending.accepted,
          "retrace request alone is not a completion");
    const auto tick = service.complete_read(retrace.read, frame);
    check(tick.acknowledged == pending.accepted && !service.pending_update(),
          "actual next retrace read resolves original update exactly once");
    const auto cancelled = service.update();
    const auto cancelled_fresh = service.read_and_update();
    const auto stop = service.stop();
    check(stop.error == g::Error::none && stop.cancelled[0] == cancelled.accepted &&
          stop.cancelled[1] == cancelled_fresh.accepted,
          "stop cancels both accepted tickets explicitly without claiming publication");
    check(service.stop().error == g::Error::stopped &&
          !service.stop().cancelled[0], "repeated stop cannot resolve a ticket twice");
    check(service.complete_read(cancelled_fresh.read, frame).error == g::Error::stale_token,
          "completion after stop is stale");
    check(service.start(frame) == g::Error::none && service.snapshot().primary().buttons == 0 &&
          service.update_every_tick(), "reentry restores BSS and initialized flags");
    const auto reentry = service.read_and_update();
    check(reentry.read.generation != cancelled_fresh.read.generation &&
          reentry.accepted.serial > cancelled_fresh.accepted.serial &&
          service.complete_read(cancelled_fresh.read, frame).error == g::Error::stale_token,
          "generation fencing and non-wrapping serials reject old-session completions");
    check(service.complete_read(reentry.read, frame).acknowledged == reentry.accepted &&
          service.snapshot().primary().pressed == 8, "reentry begins with genuinely fresh button history");
    check(service.require(g::Operation::controller_samples) == g::Error::none &&
          service.require(g::Operation::rumble) == g::Error::unsupported &&
          service.require(g::Operation::controller_pak) == g::Error::unsupported &&
          service.require(g::Operation::transfer_pak) == g::Error::unsupported &&
          service.require(g::Operation::printer) == g::Error::unsupported,
          "unsupported accessories never report success or fabricated status");
}
void signed_axes_thresholds_cache_and_menu_repeat() {
    g::Service service;
    auto frame = connected();
    check(service.start(frame) == g::Error::none, "menu service starts");
    auto initial = service.menu(0);
    check(initial.error == g::Error::none && initial.input->pressed == 0 &&
          initial.input->released == 0xffffffffu, "first menu derivation preserves actual zeroed BSS complement state");
    struct Case { int x, y; std::uint32_t bits; };
    const std::array cases{
        Case{23, -23, 0}, Case{24, -24, g::stick_slow_right | g::stick_slow_down},
        Case{-24, 24, g::stick_slow_left | g::stick_slow_up},
        Case{56, -56, g::stick_slow_right | g::stick_slow_down},
        Case{57, -57, g::stick_slow_right | g::stick_slow_down | g::stick_right | g::stick_down},
        Case{-57, 57, g::stick_slow_left | g::stick_slow_up | g::stick_left | g::stick_up},
        Case{-128, 127, g::stick_slow_left | g::stick_slow_up | g::stick_left | g::stick_up}};
    for (const auto& value : cases) {
        frame[0].stick_x = static_cast<std::int8_t>(value.x);
        frame[0].stick_y = static_cast<std::int8_t>(value.y);
        read(service, frame, true);
        check(service.begin_update() == g::Error::none, "owner marks new OM update");
        const auto result = service.menu(0);
        check(result.error == g::Error::none && result.input->current == value.bits,
              "signed axis source float/double threshold ties and directions");
        if (value.x == -128)
            check(result.input->stick_x == -1.0f && result.input->stick_y == 1.0f,
                  "axis endpoints clamp after division by eighty");
    }
    const auto cached = *service.menu(0).input;
    frame[0].stick_x = frame[0].stick_y = 0;
    frame[0].buttons = 0x8000;
    frame[3].stick_x = 80;
    read(service, frame, true);
    check(service.menu(0).input->current == cached.current &&
          service.menu(3).input->current == 0 && service.menu(0).input->update == cached.update,
          "one query cached all four ports despite a newer publication");
    check(service.begin_update() == g::Error::none, "new update invalidates once");
    check(service.menu(-1).input->current == cached.current,
          "invalid physical index returns old port zero without populating cache");
    const auto next = *service.menu(3).input;
    check(next.current == (g::stick_slow_right | g::stick_right) &&
          service.menu(0).input->pressed == 0x8000 && service.menu(0).input->released == cached.current &&
          next.update == cached.update + 1, "next derivation computes all ports and true menu transitions");
    check(service.set_menu_scale(31) == g::Error::none && service.menu_scale() == 24,
          "source menu scale setter quantizes to multiples of eight");
    check(service.set_menu_repeat(2, 1, 0x8000) == g::Error::none, "menu filter parameters set");
    for (int update = 0; update < 5; ++update) {
        check(service.begin_update() == g::Error::none, "menu repeat advances at OM update boundary");
        const auto& value = *service.menu(0).input;
        const bool expected = update != 2 && update != 4;
        check((value.current == 0x8000) == expected,
              "source per-bit menu filter suppresses boundary update, not a pad hold substitute");
        check(service.menu(0).input->update == value.update,
              "same-update query cannot advance repeat countdown");
    }
    const auto old_menu = *service.menu(0).input;
    const auto waiting = service.update();
    const auto acquisition = service.retrace();
    check(waiting.error == g::Error::none && waiting.accepted && !waiting.acknowledged &&
          acquisition.error == g::Error::none && acquisition.read,
          "scene-entry reset fixture has a real waiting publication and independent read");
    check(service.reset_scene_menu() == g::Error::none &&
          service.pending_update() == waiting.accepted && service.pending_read() == acquisition.read &&
          service.menu(0).input->update == old_menu.update &&
          service.menu(0).input->current == old_menu.current,
          "scene entry preserves acquisition tickets and already-derived current-update menu");
    for (const auto expected : std::array{0x8000u,0x8000u,0u}) {
        check(service.begin_update() == g::Error::none,"scene repeat advances on real update boundary");
        const auto value = *service.menu(0).input;
        check(value.current == expected,"scene entry restarts delay without changing repeat settings");
        if (expected == 0x8000u && value.update == 0)
            check(value.pressed == 0x8000u,"scene entry preserves previous complement/edge history");
    }
    check(service.begin_update() == g::Error::none && service.menu(0).input->current == 0x8000u &&
          service.set_menu_repeat(0,1,0x8000) == g::Error::none &&
          service.reset_scene_menu() == g::Error::none &&
          service.begin_update() == g::Error::none,
          "zero-delay scene boundary starts with an existing repeat interval");
    const auto boundary = *service.menu(0).input;
    check(boundary.update == 0 && boundary.current == 0 && boundary.released == 0x8000u,
          "scene reset retains interval so zero-delay boundary releases rather than repeats");
    const auto completion = service.complete_read(acquisition.read,frame);
    check(completion.error == g::Error::none && completion.acknowledged == waiting.accepted,
          "only matching actual completion acknowledges pre-scene waiting publication");
    check(service.set_hold(-1, 5) == g::Error::invalid_parameter &&
          service.set_menu_repeat(1, -1, 0) == g::Error::invalid_parameter,
          "unrepresentable negative countdown settings rejected explicitly");
    check(service.stop().error == g::Error::none && service.reset_scene_menu() == g::Error::stopped,
          "scene-entry reset cannot revive stopped acquisition");
}
void presentation_independent_transfer() {
    g::Service service;
    g::HostTransfer transfer;
    auto frame = connected();
    check(service.start(frame) == g::Error::none, "transfer service starts");
    const auto pending = service.update();
    for (int tick = 0; tick < 3; ++tick) {
        // No renderer, presentation acceptance, or frame counter is involved.
        const auto request = service.retrace();
        check(transfer.request_read(request.read) == g::Error::none, "gameplay enqueues actual acquisition request");
        g::ReadToken acquired;
        check(transfer.acquire_request(acquired) == g::Error::none && acquired == request.read,
              "main receives exact read identity");
        frame[0].buttons = tick == 1 ? 0 : 0x1000;
        check(transfer.submit_sample({acquired, frame}) == g::Error::none, "main publishes typed four-port completion");
        g::ReadCompletion completion;
        check(transfer.acquire_sample(completion) == g::Error::none, "gameplay acquires synchronized completion");
        const auto result = service.complete_read(completion.token, completion.frame);
        check(result.error == g::Error::none &&
              (tick != 0 || result.acknowledged == pending.accepted), "sampling continues with every presentation declined");
    }
    check(service.update().error == g::Error::none && service.snapshot().primary().pressed == 0x1000 &&
          service.snapshot().primary().released == 0x1000,
          "declined presentation cannot coalesce away release and re-press");
    g::ReadToken token;
    check(transfer.acquire_request(token) == g::Error::transfer_empty, "empty request ring explicit");
    const auto request = service.retrace();
    for (std::size_t i = 0; i < g::HostTransfer::capacity; ++i)
        check(transfer.request_read(request.read) == g::Error::none, "bounded request storage accepts capacity");
    check(transfer.request_read(request.read) == g::Error::transfer_full,
          "full request queue applies backpressure rather than silently dropping");
    for (std::size_t i = 0; i < g::HostTransfer::capacity; ++i)
        check(transfer.acquire_request(token) == g::Error::none && token == request.read,
              "bounded transport preserves every queued value");
    for (std::size_t i = 0; i < g::HostTransfer::capacity; ++i)
        check(transfer.submit_sample({request.read, frame}) == g::Error::none, "completion storage accepts capacity");
    check(transfer.submit_sample({request.read, frame}) == g::Error::transfer_full,
          "completion backpressure is also explicit");
    check(service.stop().error == g::Error::none && service.start(frame) == g::Error::none,
          "session reentry does not reset transport beneath its owners");
    g::ReadCompletion completion;
    for (std::size_t i = 0; i < g::HostTransfer::capacity; ++i) {
        check(transfer.acquire_sample(completion) == g::Error::none, "old-session queue remains drainable");
        check(service.complete_read(completion.token, completion.frame).error == g::Error::stale_token,
              "every queued stale completion is fenced from the new session");
    }
    check(transfer.acquire_sample(completion) == g::Error::transfer_empty,
          "empty completion ring does not synthesize a sample");
    const auto next = service.retrace();
    check(transfer.request_read(next.read) == g::Error::none &&
          transfer.acquire_request(token) == g::Error::none && token == next.read,
          "ring wraps and resumes after bounded backpressure");
}
void synchronized_host_acquisition() {
    g::Service service;
    g::HostTransfer transfer;
    check(service.start(connected()) == g::Error::none, "concurrent acquisition starts");
    constexpr std::uint16_t count = 1024;
    std::atomic<bool> host_ok{true};
    std::thread host([&] {
        for (std::uint16_t tick = 0; tick < count; ++tick) {
            g::ReadToken token;
            g::Error error;
            while ((error = transfer.acquire_request(token)) == g::Error::transfer_empty)
                std::this_thread::yield();
            if (error != g::Error::none) host_ok.store(false, std::memory_order_relaxed);
            auto frame = connected();
            for (std::size_t port = 0; port < g::port_count; ++port) {
                frame[port].buttons = static_cast<std::uint16_t>(tick + port);
                frame[port].stick_x = static_cast<std::int8_t>(-static_cast<int>(port) - 1);
                frame[port].stick_y = static_cast<std::int8_t>(port + 1);
            }
            while ((error = transfer.submit_sample({token, frame})) == g::Error::transfer_full)
                std::this_thread::yield();
            if (error != g::Error::none) host_ok.store(false, std::memory_order_relaxed);
        }
    });
    for (std::uint16_t tick = 0; tick < count; ++tick) {
        const auto waiting = service.update();
        const auto request = service.retrace();
        check(waiting.error == g::Error::none && request.error == g::Error::none,
              "gameplay alone mutates service while acquisition is concurrent");
        check(transfer.request_read(request.read) == g::Error::none, "single outstanding read enqueued");
        g::ReadCompletion completion;
        g::Error error;
        while ((error = transfer.acquire_sample(completion)) == g::Error::transfer_empty)
            std::this_thread::yield();
        check(error == g::Error::none &&
              service.complete_read(completion.token, completion.frame).acknowledged == waiting.accepted,
              "synchronized completion publishes and acknowledges the exact waiting event");
        for (std::size_t port = 0; port < g::port_count; ++port)
            check(service.snapshot().pads[port].buttons == static_cast<std::uint16_t>(tick + port) &&
                  service.snapshot().pads[port].stick_x == -static_cast<int>(port) - 1,
                  "SPSC transfer preserves all port payloads across repeated ring wraps");
    }
    host.join();
    check(host_ok.load(std::memory_order_relaxed), "host only used supported bounded transfer operations");
}
} // namespace
int main() {
    accumulation_and_stability();
    hold_boundaries_and_changed_buttons();
    ports_errors_and_reconnect();
    publication_protocol_and_stop();
    signed_axes_thresholds_cache_and_menu_repeat();
    presentation_independent_transfer();
    synchronized_host_acquisition();
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
