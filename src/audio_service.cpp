#include "audio_synth_internal.hpp"
#include <algorithm>
#include <atomic>
#include <bit>
#include <cmath>
#include <limits>
#include <new>
#include <utility>

namespace isle::audio {
namespace {
std::atomic<std::uint64_t> next_owner{0};
std::uint64_t reserve_owner() noexcept {
    auto previous = next_owner.load(std::memory_order_relaxed);
    while (previous != std::numeric_limits<std::uint64_t>::max()) {
        if (next_owner.compare_exchange_weak(previous, previous + 1, std::memory_order_relaxed))
            return previous + 1;
    }
    return 0;
}
bool valid_pcm(const detail::Context& c, const PcmView& pcm) noexcept {
    return c.lease == detail::Lease::pcm && pcm.token == c.pcm.token &&
        pcm.address == c.pcm.address && pcm.frame_count == c.pcm.frame_count &&
        pcm.synthesis_rate == c.pcm.synthesis_rate;
}
std::int16_t sample(const detail::Context& c, std::uint32_t address) noexcept {
    const auto bits = static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(c.get8(address)) << 8) | c.get8(address + 1));
    return std::bit_cast<std::int16_t>(bits);
}
std::uint16_t next_client(const detail::CpuState& state, std::uint64_t end) noexcept {
    std::uint16_t selected = no_slot;
    for (std::uint16_t i = 0; i < AudioService::max_players; ++i) {
        const auto& client = state.clients[i];
        if (!client.live || client.next_raw_due >= end) continue;
        if (selected == no_slot || client.next_raw_due < state.clients[selected].next_raw_due ||
            (client.next_raw_due == state.clients[selected].next_raw_due &&
             client.registration_order > state.clients[selected].registration_order)) selected = i;
    }
    return selected;
}
}

namespace detail {
unsigned prepare_shadow(Context& context) noexcept {
    const auto bank = context.live ^ 1u;
    context.cpu[bank] = context.state();
    for (const auto& client : context.state().clients) {
        if (!client.live) continue;
        switch (client.kind) {
        case ClientKind::sfx:
            context.sfx_state(bank, client.typed_index) =
                context.sfx_state(context.live, client.typed_index);
            break;
        case ClientKind::csp:
            context.csp_state(bank, client.typed_index) =
                context.csp_state(context.live, client.typed_index);
            break;
        }
    }
    return bank;
}

Error dispatch_client(Context& context, unsigned bank, std::uint16_t slot) noexcept {
    if (bank >= context.cpu.size() || slot >= AudioService::max_players)
        return Error::stale_handle;
    const auto& client = context.cpu[bank].clients[slot];
    if (!client.live) return Error::stale_handle;
    switch (client.kind) {
    case ClientKind::sfx:
        return dispatch_player(context, bank, slot);
    case ClientKind::csp:
        return dispatch_sequence_player(context, bank, slot);
    }
    return Error::invalid_state;
}
} // namespace detail

Error service_quota(SynthConfig config, Service service, std::uint32_t& frames) noexcept {
    frames = 0;
    if (config.synthesis_rate == 0 || config.synthesis_rate > 96000 || service.service_id == 0)
        return Error::invalid_service;
    if (service.quota_frames != 0) {
        if ((service.quota_frames & 15u) != 0 || service.quota_frames > AudioService::max_frames)
            return Error::invalid_service;
        frames = service.quota_frames;
        return Error::none;
    }
    const std::int64_t nominal = (config.synthesis_rate / 60u + 16u) & ~15u;
    const std::int64_t raw = 16 + nominal - static_cast<std::int64_t>(service.previous_ai_frames) + 80;
    const std::int64_t aligned = raw >= 0 ? (raw / 16) * 16 : -(((-raw + 15) / 16) * 16);
    const std::int64_t quota = std::max(nominal - 16, aligned);
    if (quota <= 0 || quota > AudioService::max_frames) return Error::invalid_service;
    frames = static_cast<std::uint32_t>(quota);
    return Error::none;
}
AudioService::AudioService(std::unique_ptr<detail::Context> context) noexcept : context_(std::move(context)) {}
AudioService::~AudioService() = default;
ServiceResult AudioService::create(std::span<const BankFile> owners, SynthConfig config,
                                   const Coefficients& coefficients, effects::Config effects) noexcept {
    if (owners.empty() || std::any_of(owners.begin(), owners.end(), [](const auto& owner) { return !owner; }))
        return {Error::invalid_resource, nullptr};
    if (config.synthesis_rate == 0 || config.synthesis_rate > 96000 ||
        detail::rounded_frames(16000, config.synthesis_rate) == 0 ||
        config.physical_voices == 0 || config.physical_voices > max_physical_voices ||
        config.logical_voices == 0 || config.logical_voices > max_logical_voices ||
        (config.steal.kill_frames & 15u) != 0 || (config.steal.restart_frames & 15u) != 0 ||
        config.steal.restart_frames == 0 || config.steal.kill_frames > config.steal.restart_frames)
        return {Error::invalid_parameter, nullptr};
    if (coefficients.pan.front() != 0x7fff || coefficients.pan.back() != 0 ||
        std::any_of(coefficients.pan.begin(), coefficients.pan.end(), [](auto v) { return v > 0x7fff; }) ||
        std::any_of(coefficients.logarithm.begin(), coefficients.logarithm.end(),
                    [](double v) { return !std::isfinite(v) || v < -1.0 || v > 0.0; }))
        return {Error::invalid_parameter, nullptr};
    const auto effects_error = detail::validate_effects(effects);
    if (effects_error != Error::none) return {effects_error, nullptr};
    try {
        auto context = std::make_unique<detail::Context>();
        context->owner = reserve_owner();
        if (context->owner == 0) return {Error::identity_exhausted, nullptr};
        context->config = config;
        context->coefficients = coefficients;
        context->memory = std::make_unique<std::uint32_t[]>(rdram_bytes / 4u);
        const auto admitted = detail::build_catalog(*context, owners);
        if (admitted != Error::none) return {admitted, nullptr};
        const auto effects_admitted = detail::build_effects(*context, effects);
        if (effects_admitted != Error::none) return {effects_admitted, nullptr};
        return {Error::none, std::unique_ptr<AudioService>(new AudioService(std::move(context)))};
    } catch (const std::bad_alloc&) {
        return {Error::allocation_failure, nullptr};
    } catch (...) {
        return {Error::invalid_resource, nullptr};
    }
}
Error AudioService::sound_id(const Sound& sound, SoundId& id) const noexcept {
    return detail::find_sound(*context_, sound, id);
}
Error AudioService::wave_id(SoundId id, WaveId& wave) const noexcept {
    wave = {};
    const detail::CatalogSound* sound = nullptr;
    const auto error = detail::catalog_sound(*context_, id, sound);
    if (error == Error::none) wave = {context_->owner, sound->wave_index};
    return error;
}
Error AudioService::sound(SoundId id, const Sound*& resource) const noexcept {
    resource = nullptr;
    const detail::CatalogSound* sound = nullptr;
    const auto error = detail::catalog_sound(*context_, id, sound);
    if (error == Error::none) resource = sound->resource;
    return error;
}
Error AudioService::add_sound_player(const sfx::Config& config, PlayerId& id) noexcept {
    id = {};
    auto& c = *context_;
    if (c.poisoned) return Error::needs_reset;
    if (c.lease != detail::Lease::none || c.building) return Error::task_busy;
    return detail::add_player(c, config, id);
}
sfx::SoundPlayer AudioService::player(PlayerId id) noexcept { return sfx::SoundPlayer(context_.get(), id); }
Error AudioService::add_sequence_player(const csp::Config& config, PlayerId& id) noexcept {
    id = {};
    return detail::add_sequence_player(*context_, config, id);
}
csp::SequencePlayer AudioService::sequence_player(PlayerId id) noexcept {
    return csp::SequencePlayer(context_.get(), id);
}
Error AudioService::set_player_deadline(PlayerId id, RawSampleTime due) noexcept {
    auto& c = *context_;
    if (c.poisoned) return Error::needs_reset;
    if (c.lease != detail::Lease::none || c.building) return Error::task_busy;
    if (id.owner != c.owner || id.slot >= max_players) return Error::stale_handle;
    auto& state = c.state();
    auto& client = state.clients[id.slot];
    if (!client.live || client.generation != id.generation) return Error::stale_handle;
    if (due.value < state.sample_clock) return Error::invalid_parameter;
    client.next_raw_due = due.value;
    return Error::none;
}

Error AudioService::build(Service service, TaskView& task) noexcept {
    task = {};
    auto& c = *context_;
    if (c.poisoned) return Error::needs_reset;
    if (c.lease != detail::Lease::none || c.building) return Error::task_busy;
    std::uint32_t frames = 0;
    const auto quota_error = service_quota(c.config, service, frames);
    if (quota_error != Error::none) return quota_error;
    const auto& live = c.state();
    if (service.service_id <= live.last_service) return Error::invalid_service;
    if (c.next_task == std::numeric_limits<std::uint64_t>::max() ||
        live.sample_clock > std::numeric_limits<std::uint64_t>::max() - frames)
        return Error::identity_exhausted;
    const auto start = live.sample_clock;
    const auto end = start + frames;
    const auto shadow_bank = detail::prepare_shadow(c);
    auto& shadow = c.cpu[shadow_bank];
    c.building = true;
    c.write_address = command_address;
    c.write_error = Error::none;
    c.emit(0x07000000u, 0);
    Error error = Error::none;
    std::size_t callbacks = 0;
    for (;;) {
        const auto selected = next_client(shadow, end);
        if (selected == no_slot) break;
        if (++callbacks > detail::max_callback_steps) { error = Error::event_capacity; break; }
        const auto due = shadow.clients[selected].next_raw_due & ~std::uint64_t{15};
        if (due < shadow.sample_clock) { error = Error::invalid_state; break; }
        if (due > shadow.sample_clock) {
            const auto offset = static_cast<std::uint32_t>(shadow.sample_clock - start);
            error = detail::render(c, shadow, static_cast<std::uint32_t>(due - shadow.sample_clock), offset, frames);
            if (error != Error::none) break;
            if (shadow.sample_clock != due) { error = Error::invalid_state; break; }
        }
        error = detail::advance_parameters(c, shadow, {due});
        if (error != Error::none) break;
        error = detail::dispatch_client(c, shadow_bank, selected);
        if (error != Error::none) break;
    }
    if (error == Error::none && shadow.sample_clock < end) {
        const auto offset = static_cast<std::uint32_t>(shadow.sample_clock - start);
        error = detail::render(c, shadow, static_cast<std::uint32_t>(end - shadow.sample_clock), offset, frames);
    }
    if (error == Error::none && shadow.sample_clock != end) error = Error::invalid_state;
    if (error == Error::none && shadow.effects.staged_frames != 0) error = Error::invalid_state;
    // CPU-only events at the endpoint become visible to subsequent accepted
    // controls; raw client callbacks exactly there remain deferred to next task.
    if (error == Error::none) error = detail::advance_parameters(c, shadow, {end});
    if (error == Error::none) error = c.write_error;
    c.building = false;
    if (error != Error::none) return error;
    detail::collect_voices(shadow);
    shadow.last_service = service.service_id;
    c.live = shadow_bank;
    c.task = {{c.owner, ++c.next_task}, service.service_id, command_address,
              c.write_address - command_address, pcm_address, frames, c.config.synthesis_rate};
    c.lease = detail::Lease::executing;
    task = c.task;
    return Error::none;
}
Error AudioService::complete(TaskToken token, bool success, PcmView& pcm) noexcept {
    pcm = {};
    auto& c = *context_;
    if (c.lease != detail::Lease::executing || token != c.task.token) return Error::stale_task;
    if (!success) {
        c.lease = detail::Lease::none;
        c.poisoned = true;
        return Error::execution_failed;
    }
    c.pcm = {token, c.task.pcm_address, c.task.frame_count, c.task.synthesis_rate};
    c.lease = detail::Lease::pcm;
    pcm = c.pcm;
    return Error::none;
}
Error AudioService::release(TaskToken token) noexcept {
    auto& c = *context_;
    if (c.lease != detail::Lease::pcm || token != c.pcm.token) return Error::stale_task;
    c.lease = detail::Lease::none;
    c.pcm = {};
    return Error::none;
}
Error AudioService::reset() noexcept {
    auto& c = *context_;
    if (c.lease != detail::Lease::none || c.building) return Error::task_busy;
    const auto epoch = c.state().next_binding_epoch;
    const auto age = c.state().next_allocation_age;
    const auto sequence = c.state().next_parameter_sequence;
    c.state() = detail::CpuState{};
    c.state().next_binding_epoch = epoch;
    c.state().next_allocation_age = age;
    c.state().next_parameter_sequence = sequence;
    for (auto& checkpoints : c.sfx) checkpoints.reset();
    for (auto& checkpoints : c.csp) checkpoints.reset();
    detail::reset_effects(c);
    c.poisoned = false;
    c.task = {};
    c.pcm = {};
    // All next sources emit A_INIT. Mutable global FX storage is cleared above;
    // immutable program/books/catalog and the stable RAM allocation survive.
    return Error::none;
}
Error AudioService::reset(effects::Config effects) noexcept {
    auto& c = *context_;
    if (c.lease != detail::Lease::none || c.building) return Error::task_busy;
    const auto admitted = detail::validate_effects_for_rate(effects, c.config.synthesis_rate);
    if (admitted != Error::none) return admitted;
    // build_effects has no fallible operation after this same complete preflight.
    // Install first, retaining any caller span aliased to the old program until
    // copying is complete; reset() clears every mutable ring/history/staging byte.
    const auto configured = detail::build_effects(c, effects);
    if (configured != Error::none) return configured;
    return reset();
}
bool AudioService::has_lease() const noexcept { return context_->lease != detail::Lease::none; }
bool AudioService::voices_retired() const noexcept {
    const auto& c = *context_;
    const auto& state = c.state();
    if (c.lease != detail::Lease::none || c.building || state.parameter_count != 0 ||
        state.allocated_head != no_slot || state.retiring_head != no_slot) return false;
    return std::none_of(state.physical.begin(), state.physical.end(), [](const auto& voice) {
        return voice.allocated || voice.current.active || voice.pending.valid;
    });
}
Error AudioService::read_pcm(const PcmView& pcm, std::uint32_t frame,
                              std::int16_t& left, std::int16_t& right) const noexcept {
    if (!valid_pcm(*context_, pcm)) return Error::stale_task;
    if (frame >= pcm.frame_count) return Error::invalid_parameter;
    left = sample(*context_, pcm.address + frame * 4u);
    right = sample(*context_, pcm.address + frame * 4u + 2u);
    return Error::none;
}
Error AudioService::copy_pcm(const PcmView& pcm, std::span<std::int16_t> destination) const noexcept {
    if (!valid_pcm(*context_, pcm)) return Error::stale_task;
    const auto samples = static_cast<std::size_t>(pcm.frame_count) * 2u;
    if (destination.size() < samples) return Error::invalid_parameter;
    for (std::size_t i = 0; i < samples; ++i)
        destination[i] = sample(*context_, pcm.address + static_cast<std::uint32_t>(i) * 2u);
    return Error::none;
}
std::span<std::uint32_t> AudioService::rdram_words() noexcept {
    return {context_->memory.get(), rdram_bytes / 4u};
}
std::span<const std::uint32_t> AudioService::rdram_words() const noexcept {
    return {context_->memory.get(), rdram_bytes / 4u};
}
} // namespace isle::audio
