#include "35mmisle/game_input.hpp"

#include <algorithm>
#include <limits>

namespace isle::game_input {
namespace {
bool valid(const HostFrame& frame) noexcept {
    for (const auto& port : frame) {
        if (port.connection != Connection::present && port.connection != Connection::absent)
            return false;
        if ((port.connection == Connection::present) != (port.identity != 0)) return false;
    }
    return true;
}
bool healthy(const PortSample& port) noexcept {
    return port.connection == Connection::present && port.error == 0;
}
void indices(Snapshot& snapshot, const HostFrame& ports) noexcept {
    snapshot.controller_count = 0;
    snapshot.indices.fill(-1);
    for (std::size_t i = 0; i < port_count; ++i)
        if (healthy(ports[i]))
            snapshot.indices[snapshot.controller_count++] = static_cast<int>(i);
}
float axis(std::int8_t value) noexcept {
    // GAME stores a float after dividing by the unsuffixed double 80.0.
    return std::clamp(static_cast<float>(static_cast<double>(value) / 80.0), -1.0f, 1.0f);
}
std::uint32_t directions(float x, float y) noexcept {
    std::uint32_t bits = 0;
    // Unsuffixed source thresholds promote the stored float back to double.
    if (static_cast<double>(x) > 0.3) bits |= stick_slow_right;
    else if (static_cast<double>(x) < -0.3) bits |= stick_slow_left;
    if (static_cast<double>(y) > 0.3) bits |= stick_slow_up;
    else if (static_cast<double>(y) < -0.3) bits |= stick_slow_down;
    if (static_cast<double>(x) > 0.7) bits |= stick_right;
    else if (static_cast<double>(x) < -0.7) bits |= stick_left;
    if (static_cast<double>(y) > 0.7) bits |= stick_up;
    else if (static_cast<double>(y) < -0.7) bits |= stick_down;
    return bits;
}
} // namespace

Error Service::available() const noexcept {
    if (!running_) return Error::stopped;
    if (read_) return Error::read_in_flight;
    return Error::none;
}
Error Service::reserve(std::uint64_t count) const noexcept {
    return serial_ > std::numeric_limits<std::uint64_t>::max() - count
        ? Error::exhausted : Error::none;
}
RequestToken Service::ticket() noexcept { return {this, generation_, ++serial_}; }
ReadToken Service::read_ticket() noexcept { return {this, generation_, ++serial_}; }
Error Service::start(const HostFrame& initial) noexcept {
    if (running_) return Error::already_started;
    if (!valid(initial)) return Error::invalid_sample;
    if (generation_ == std::numeric_limits<std::uint64_t>::max()) return Error::exhausted;
    ++generation_;
    state_ = {};
    ports_ = initial;
    snapshot_ = {};
    snapshot_.ports = initial;
    indices(snapshot_, initial);
    menu_ = {};
    neutral_ = {};
    menu_delay_ = {};
    menu_interval_ = {};
    menu_update_ = 0;
    hold_delay_ = 30;
    hold_interval_ = 5;
    menu_hold_delay_ = 36;
    menu_hold_interval_ = 12;
    menu_mask_ = menu_scale_ = 0;
    pending_ = fresh_ = {};
    read_ = {};
    dirty_ = menu_cached_ = false;
    every_tick_ = running_ = true;
    return Error::none;
}
StopResult Service::stop() noexcept {
    if (!running_) return {Error::stopped, {}};
    StopResult result{Error::none, {pending_, fresh_}};
    pending_ = fresh_ = {};
    read_ = {};
    running_ = false;
    return result;
}
EventResult Service::retrace() noexcept {
    if (const auto error = available(); error != Error::none) return {error};
    if (!every_tick_) return {};
    if (const auto error = reserve(1); error != Error::none) return {error};
    read_ = read_ticket();
    return {Error::none, {}, read_, {}};
}
EventResult Service::update() noexcept {
    if (const auto error = available(); error != Error::none) return {error};
    if (pending_) return {Error::pending_update};
    if (const auto error = reserve(1); error != Error::none) return {error};
    const auto accepted = ticket();
    if (dirty_) {
        publish();
        return {Error::none, accepted, {}, accepted};
    }
    pending_ = accepted;
    return {Error::none, accepted, {}, {}};
}
EventResult Service::read_and_update() noexcept {
    if (const auto error = available(); error != Error::none) return {error};
    if (const auto error = reserve(2); error != Error::none) return {error};
    fresh_ = ticket();
    read_ = read_ticket();
    return {Error::none, fresh_, read_, {}};
}
EventResult Service::complete_read(ReadToken token, const HostFrame& frame) noexcept {
    // Check tokens before sample validation, including completions after stop.
    if (!token || token != read_ || token.owner != this || token.generation != generation_)
        return {Error::stale_token};
    if (!running_) return {Error::stopped};
    if (!valid(frame)) return {Error::invalid_sample}; // Retain read for correction/cancellation.
    ports_ = frame;
    for (std::size_t i = 0; i < port_count; ++i) {
        if (!healthy(frame[i])) continue; // Source preserves state AND accumulators on errors.
        auto& state = state_[i];
        auto& raw = state.raw;
        const auto changed = static_cast<std::uint16_t>(frame[i].buttons ^ raw.buttons);
        raw.pressed = static_cast<std::uint16_t>(changed & frame[i].buttons);
        raw.released = static_cast<std::uint16_t>(changed & raw.buttons);
        if (changed != 0) {
            raw.held = raw.pressed;
            state.counter = hold_delay_;
        } else {
            --state.counter;
            if (state.counter > 0) raw.held = 0;
            else {
                raw.held = frame[i].buttons;
                state.counter = hold_interval_;
            }
        }
        raw.buttons = frame[i].buttons;
        raw.stick_x = frame[i].stick_x;
        raw.stick_y = frame[i].stick_y;
        state.pressed |= raw.pressed;
        state.released |= raw.released;
        state.held |= raw.held;
    }
    dirty_ = true;
    read_ = {};
    RequestToken acknowledged{};
    if (fresh_) {
        // A fresh-read event does NOT also acknowledge a previously waiting update.
        publish();
        acknowledged = fresh_;
        fresh_ = {};
    } else if (pending_) {
        publish();
        acknowledged = pending_;
        pending_ = {};
    }
    return {Error::none, {}, {}, acknowledged};
}
void Service::publish() noexcept {
    for (std::size_t i = 0; i < port_count; ++i) {
        if (!healthy(ports_[i])) continue;
        auto& state = state_[i];
        snapshot_.pads[i] = state.raw;
        snapshot_.pads[i].pressed = state.pressed;
        snapshot_.pads[i].released = state.released;
        snapshot_.pads[i].held = state.held;
        state.pressed = state.released = state.held = 0;
    }
    snapshot_.ports = ports_;
    indices(snapshot_, ports_);
    dirty_ = false;
}
Error Service::set_update_every_tick(bool enabled) noexcept {
    if (const auto error = available(); error != Error::none) return error;
    every_tick_ = enabled;
    return Error::none;
}
Error Service::set_hold(std::int32_t delay, std::int32_t interval) noexcept {
    if (const auto error = available(); error != Error::none) return error;
    if (delay < 0 || interval < 0) return Error::invalid_parameter;
    hold_delay_ = delay;
    hold_interval_ = interval;
    // Source changes parameters, not the current countdown.
    return Error::none;
}
Error Service::require(Operation operation) const noexcept {
    return operation == Operation::controller_samples ? Error::none : Error::unsupported;
}
Error Service::begin_update() noexcept {
    if (!running_) return Error::stopped;
    menu_cached_ = false;
    return Error::none;
}
Error Service::reset_scene_menu() noexcept {
    if (!running_) return Error::stopped;
    menu_update_ = 0;
    menu_delay_ = {};
    return Error::none;
}
MenuResult Service::menu(int physical_port) noexcept {
    if (!running_) return {Error::stopped, nullptr};
    // Source invalid index returns cached port zero WITHOUT computing it.
    if (physical_port < 0 || physical_port >= static_cast<int>(port_count))
        return {Error::none, &menu_[0]};
    if (!menu_cached_) {
        if (menu_update_ == std::numeric_limits<std::uint64_t>::max())
            return {Error::exhausted, nullptr};
        for (std::size_t i = 0; i < port_count; ++i) {
            auto& output = menu_[i];
            const auto& pad = snapshot_.pads[i];
            output.stick_x = axis(pad.stick_x);
            output.stick_y = axis(pad.stick_y);
            output.unused_axis = 0;
            const auto buttons = static_cast<std::uint32_t>(pad.buttons) |
                                 directions(output.stick_x, output.stick_y);
            std::uint32_t current = 0;
            for (std::size_t bit = 0; bit < 24; ++bit) {
                const std::uint32_t mask = 1u << bit;
                if ((menu_mask_ & mask) == 0) current |= buttons & mask;
                else if ((buttons & mask) == 0) menu_delay_[i][bit] = 0;
                else if (menu_delay_[i][bit] < menu_hold_delay_) {
                    ++menu_delay_[i][bit];
                    menu_interval_[i][bit] = menu_hold_interval_;
                    current |= buttons & mask;
                } else if (menu_interval_[i][bit] >= menu_hold_interval_)
                    menu_interval_[i][bit] = 0;
                else {
                    ++menu_interval_[i][bit];
                    current |= mask;
                }
            }
            output.current = current;
            output.pressed = output.not_pressed & current;
            output.released = ~(output.not_pressed | current);
            output.not_pressed = ~current;
            output.update = menu_update_;
        }
        ++menu_update_;
        menu_cached_ = true;
    }
    return {Error::none, &menu_[static_cast<std::size_t>(physical_port)]};
}
MenuResult Service::neutral(int physical_port) noexcept {
    if (!running_) return {Error::stopped, nullptr};
    if (physical_port < 0 || physical_port >= static_cast<int>(port_count))
        return {Error::none, &neutral_[0]};
    // Source AA740 shares AA38C's cache gate, but neither sets it nor advances
    // its serial/repeat clocks. gNumControllers is a count, not a port mapping.
    if (!menu_cached_) {
        for (std::size_t i = 0; i < snapshot_.controller_count; ++i) {
            auto& output = neutral_[i];
            output.stick_x = output.stick_y = output.unused_axis = 0;
            output.current = output.pressed = output.released = 0;
            output.not_pressed = ~std::uint32_t{0};
            output.update = menu_update_;
        }
    }
    return {Error::none, &neutral_[static_cast<std::size_t>(physical_port)]};
}
Error Service::set_menu_repeat(std::int32_t delay, std::int32_t interval,
                                std::uint32_t mask) noexcept {
    if (!running_) return Error::stopped;
    if (delay < 0 || interval < 0 || mask > 0xffffffu) return Error::invalid_parameter;
    menu_hold_delay_ = delay;
    menu_hold_interval_ = interval;
    menu_mask_ = mask;
    return Error::none;
}
Error Service::set_menu_repeat(std::int32_t delay, std::int32_t interval) noexcept {
    if (!running_) return Error::stopped;
    if (delay < 0 || interval < 0) return Error::invalid_parameter;
    menu_hold_delay_ = delay;
    menu_hold_interval_ = interval;
    return Error::none;
}
Error Service::set_menu_mask(std::uint32_t mask) noexcept {
    if (!running_) return Error::stopped;
    if (mask > 0xffffffu) return Error::invalid_parameter;
    menu_mask_ = mask;
    return Error::none;
}
Error Service::set_menu_scale(std::uint32_t value) noexcept {
    if (!running_) return Error::stopped;
    if (value > static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max()))
        return Error::invalid_parameter;
    menu_scale_ = value / 8u;
    return Error::none;
}
Error HostTransfer::request_read(ReadToken token) noexcept {
    if (!token) return Error::stale_token;
    return requests_.push(token);
}
Error HostTransfer::acquire_request(ReadToken& token) noexcept { return requests_.pop(token); }
Error HostTransfer::submit_sample(const ReadCompletion& completion) noexcept {
    if (!completion.token) return Error::stale_token;
    return completions_.push(completion);
}
Error HostTransfer::acquire_sample(ReadCompletion& completion) noexcept {
    return completions_.pop(completion);
}
} // namespace isle::game_input
