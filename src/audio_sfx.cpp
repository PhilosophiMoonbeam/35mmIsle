#include "audio_synth_internal.hpp"

#include <algorithm>
#include <limits>
#include <new>
#include <utility>

namespace isle::audio {
namespace {
using detail::Context;
using detail::CpuState;
using detail::PlayerEvent;
using detail::PlayerEventKind;
using detail::PlayerSound;
using detail::SfxCheckpoint;
using detail::SoundPhase;

bool playing(const PlayerSound& sound) noexcept {
    return sound.phase != SoundPhase::free && sound.phase != SoundPhase::failed;
}

Error player_state(Context* context, PlayerId id, SfxCheckpoint*& player) noexcept {
    player = nullptr;
    if (context == nullptr || id.owner != context->owner || id.slot >= AudioService::max_players)
        return Error::stale_handle;
    const auto& client = context->state().clients[id.slot];
    if (!client.live || client.generation != id.generation || client.kind != detail::ClientKind::sfx ||
        client.typed_index >= context->sfx.size() || !context->sfx[client.typed_index])
        return Error::stale_handle;
    player = &context->sfx_state(context->live, client.typed_index);
    return Error::none;
}

Error mutable_player(Context* context, PlayerId id, SfxCheckpoint*& player) noexcept {
    const auto error = player_state(context, id, player);
    if (error != Error::none) return error;
    if (context->poisoned) return Error::needs_reset;
    if (context->building) return Error::task_busy;
    return Error::none;
}

Error playback(Context& context, PlayerId id, SfxCheckpoint& player,
               sfx::SoundHandle handle, PlayerSound*& sound) noexcept {
    sound = nullptr;
    if (handle.owner != context.owner || handle.player != id.slot || handle.slot >= player.max_sounds)
        return Error::stale_handle;
    auto& candidate = player.sounds[handle.slot];
    if (handle.generation == 0 || candidate.generation != handle.generation)
        return Error::stale_handle;
    sound = &candidate;
    return Error::none;
}

bool valid_parameters(const sfx::Parameters& parameters) noexcept {
    std::uint16_t pitch = 0;
    return parameters.volume <= 0x7fff && parameters.pan <= 127 && parameters.fx_mix <= 127 &&
           detail::pitch_word(parameters.pitch, pitch);
}

Error qualified_sound(const Context& context, SoundId id, float pitch,
                      const detail::CatalogSound*& selected) noexcept {
    auto error = detail::catalog_sound(context, id, selected);
    if (error != Error::none) return error;
    const auto& sound = *selected->resource;
    if (sound.envelope == nullptr || sound.wave == nullptr || sound.keymap == nullptr)
        return Error::invalid_resource;
    const auto& envelope = *sound.envelope;
    // Zero-time/zero-level finite decay has no admitted original lifecycle.
    if (envelope.attack_us < 0 || envelope.decay_us < -1 || envelope.decay_us == 0 ||
        (envelope.decay_us > 0 && envelope.decay_volume == 0) ||
        envelope.release_us < 0 || envelope.attack_volume > 127 ||
        envelope.decay_volume > 127 || sound.volume > 127 || sound.pan > 127)
        return Error::unsupported_envelope;
    const detail::CatalogWave* wave = nullptr;
    error = detail::catalog_wave(context, {context.owner, selected->wave_index}, wave);
    if (error != Error::none) return error;
    if (wave->admission != Error::none) return wave->admission;
    std::int32_t scaled = 0;
    if (!detail::scaled_duration_us(envelope.attack_us, pitch, scaled) ||
        (envelope.decay_us > 0 && !detail::scaled_duration_us(envelope.decay_us, pitch, scaled)) ||
        !detail::scaled_duration_us(envelope.release_us, pitch, scaled))
        return Error::invalid_parameter;
    return Error::none;
}

Gain15 player_gain(const PlayerSound& sound, const Sound& resource) noexcept {
    const auto level = sound.phase == SoundPhase::release ? 0u :
        sound.phase == SoundPhase::decay ? resource.envelope->decay_volume :
                                          resource.envelope->attack_volume;
    const auto perceived = (static_cast<std::uint32_t>(sound.parameters.volume) * level) >> 7;
    return {static_cast<std::uint16_t>((perceived * perceived) >> 15)};
}

Error post(Context* context, PlayerId id, PlayerEvent event) noexcept {
    SfxCheckpoint* player = nullptr;
    auto error = mutable_player(context, id, player);
    if (error != Error::none) return error;
    PlayerSound* sound = nullptr;
    error = playback(*context, id, *player, event.handle, sound);
    if (error != Error::none) return error;
    if (!playing(*sound)) return Error::stale_handle;
    if (event.kind == PlayerEventKind::pitch) {
        const detail::CatalogSound* selected = nullptr;
        error = qualified_sound(*context, sound->sound, event.parameters.pitch, selected);
        if (error != Error::none) return error;
    }
    if (player->inbox_count == player->inbox.size()) return Error::event_capacity;
    player->inbox[player->inbox_count++] = event;
    return Error::none;
}

Error control_result(PlayerSound& sound, Error error) noexcept {
    if (error == Error::unbound) {
        sound.error = Error::unbound;
        return Error::none;
    }
    return error;
}

Error schedule_phase(PlayerSound& sound, std::int32_t microseconds, std::uint64_t now) noexcept {
    std::int32_t scaled = 0;
    if (!detail::scaled_duration_us(microseconds, sound.parameters.pitch, scaled))
        return Error::invalid_parameter;
    if (now > std::numeric_limits<std::uint64_t>::max() - static_cast<std::uint32_t>(scaled))
        return Error::identity_exhausted;
    sound.event_us = scaled == 0 ? 0 : now + static_cast<std::uint32_t>(scaled);
    return Error::none;
}

Error finish_sound(Context& context, CpuState& state, PlayerSound& sound, ParamTime time) noexcept {
    auto error = control_result(sound, detail::stop_voice(context, state, sound.descriptor, time));
    if (error != Error::none) return error;
    error = control_result(sound, detail::free_voice(context, state, sound.descriptor, time));
    if (error != Error::none) return error;
    sound.phase = SoundPhase::free;
    sound.event_us = 0;
    sound.published = false;
    sound.stop_requested = false;
    return Error::none;
}

Error release_sound(Context& context, CpuState& state, SfxCheckpoint& player,
                    PlayerSound& sound, ParamTime time) noexcept {
    if (sound.phase == SoundPhase::release) return Error::none;
    const detail::CatalogSound* selected = nullptr;
    auto error = detail::catalog_sound(context, sound.sound, selected);
    if (error != Error::none) return error;
    error = schedule_phase(sound, selected->resource->envelope->release_us, player.client_us);
    if (error != Error::none) return error;
    sound.stop_requested = true;
    if (sound.event_us == 0) return finish_sound(context, state, sound, time);
    sound.phase = SoundPhase::release;
    // Release scales both its DSP ramp and its callback by the current pitch.
    const RampFrames release{detail::duration_frames(
        static_cast<std::int32_t>(sound.event_us - player.client_us), context.config.synthesis_rate)};
    return control_result(sound, detail::gain_voice(context, state, sound.descriptor, {0}, release, time));
}

Error decay_sound(Context& context, CpuState& state, SfxCheckpoint& player,
                  PlayerSound& sound, ParamTime time) noexcept {
    const detail::CatalogSound* selected = nullptr;
    auto error = detail::catalog_sound(context, sound.sound, selected);
    if (error != Error::none) return error;
    const auto decay_us = selected->resource->envelope->decay_us;
    if (decay_us == -1) {
        // An indefinite attack endpoint is only a CPU phase change, not a
        // second DSP gain event or envelope INIT.
        sound.phase = SoundPhase::sustain;
        sound.event_us = 0;
        return Error::none;
    }
    error = schedule_phase(sound, decay_us, player.client_us);
    if (error != Error::none) return error;
    sound.phase = SoundPhase::decay;
    const RampFrames decay{detail::duration_frames(
        static_cast<std::int32_t>(sound.event_us == 0 ? 0 : sound.event_us - player.client_us),
        context.config.synthesis_rate)};
    error = control_result(sound, detail::gain_voice(context, state, sound.descriptor,
        player_gain(sound, *selected->resource), decay, time));
    if (error != Error::none) return error;
    // A pitch-scaled duration can truncate to zero. Complete that bounded
    // phase transition here rather than inventing a later poll deadline.
    return sound.event_us == 0 ? release_sound(context, state, player, sound, time) : Error::none;
}

Error start_sound(Context& context, CpuState& state, SfxCheckpoint& player,
                  PlayerSound& sound, ParamTime time) noexcept {
    const detail::CatalogSound* selected = nullptr;
    auto error = qualified_sound(context, sound.sound, sound.parameters.pitch, selected);
    if (error != Error::none) return error;
    error = schedule_phase(sound, selected->resource->envelope->attack_us, player.client_us);
    if (error != Error::none) return error;
    // Attack DSP time is authored, while its client callback is pitch-scaled.
    const RampFrames attack{detail::duration_frames(selected->resource->envelope->attack_us,
                                                   context.config.synthesis_rate)};
    sound.phase = SoundPhase::attack;
    sound.published = true;
    Allocation allocation{};
    error = detail::allocate_voice(context, state, sound.descriptor, player.priority, time, allocation);
    if (error == Error::voice_unavailable) {
        // Original failed allocation remains logically playing and accepts
        // controls. It does not retry when a physical voice later becomes free.
        sound.error = Error::unbound;
    } else {
        if (error != Error::none) return error;
        const DirectParameters parameters{sound.parameters.pitch, player_gain(sound, *selected->resource),
                                          sound.parameters.pan, sound.parameters.fx_mix};
        error = detail::start_voice(context, state, sound.descriptor,
                                    {context.owner, selected->wave_index}, parameters, attack, time);
        if (error != Error::none) return error;
        // Configured startup explicitly evaluates gain at the source time,
        // including a pending replacement. Decay is a separate gain event.
        error = detail::gain_voice(context, state, sound.descriptor, parameters.gain, attack, time);
        if (error != Error::none) return error;
    }
    return sound.event_us == 0 ? decay_sound(context, state, player, sound, time) : Error::none;
}

Error apply_event(Context& context, CpuState& state, SfxCheckpoint& player,
                  const PlayerEvent& event, ParamTime time) noexcept {
    if (event.handle.owner != context.owner || event.handle.slot >= player.max_sounds)
        return Error::invalid_state;
    auto& sound = player.sounds[event.handle.slot];
    // A queued control can outlive its playback's release endpoint. Slot reuse
    // never forwards such a control to the next playback or physical owner.
    if (sound.generation != event.handle.generation || !playing(sound)) return Error::none;
    if (event.kind == PlayerEventKind::start) {
        if (sound.phase != SoundPhase::reserved || sound.published) return Error::none;
        sound.parameters = sound.desired = event.parameters;
        return start_sound(context, state, player, sound, time);
    }
    if (!sound.published) return Error::invalid_state;
    switch (event.kind) {
    case PlayerEventKind::pitch:
        sound.desired.pitch = event.parameters.pitch;
        sound.parameters.pitch = event.parameters.pitch;
        return control_result(sound, detail::pitch_voice(context, state, sound.descriptor,
                                                         sound.parameters.pitch, time));
    case PlayerEventKind::pan:
        sound.desired.pan = event.parameters.pan;
        sound.parameters.pan = event.parameters.pan;
        return control_result(sound, detail::pan_voice(context, state, sound.descriptor,
                                                       sound.parameters.pan, time));
    case PlayerEventKind::fx:
        sound.desired.fx_mix = event.parameters.fx_mix;
        sound.parameters.fx_mix = event.parameters.fx_mix;
        return control_result(sound, detail::fx_voice(context, state, sound.descriptor,
                                                      sound.parameters.fx_mix, time));
    case PlayerEventKind::volume: {
        sound.desired.volume = event.parameters.volume;
        sound.parameters.volume = event.parameters.volume;
        const detail::CatalogSound* selected = nullptr;
        const auto error = detail::catalog_sound(context, sound.sound, selected);
        if (error != Error::none) return error;
        const auto gain = player_gain(sound, *selected->resource);
        return control_result(sound, detail::gain_voice(context, state, sound.descriptor, gain,
            {detail::duration_frames(1000, context.config.synthesis_rate)}, time));
    }
    case PlayerEventKind::stop:
        return release_sound(context, state, player, sound, time);
    case PlayerEventKind::start:
        break;
    }
    return Error::invalid_state;
}
} // namespace

namespace sfx {
Error SoundPlayer::stage_start(SoundId id, Parameters parameters, SoundHandle& handle) noexcept {
    handle = {};
    SfxCheckpoint* player = nullptr;
    auto error = mutable_player(context_, id_, player);
    if (error != Error::none) return error;
    if (!valid_parameters(parameters)) return Error::invalid_parameter;
    const detail::CatalogSound* selected = nullptr;
    error = qualified_sound(*context_, id, parameters.pitch, selected);
    if (error != Error::none) return error;
    if (player->inbox_count == player->inbox.size()) return Error::event_capacity;
    std::uint16_t slot = 0;
    while (slot < player->max_sounds && playing(player->sounds[slot])) ++slot;
    if (slot == player->max_sounds) return Error::descriptor_capacity;
    if (context_->next_sound == std::numeric_limits<std::uint64_t>::max())
        return Error::identity_exhausted;
    auto& sound = player->sounds[slot];
    const auto descriptor = sound.descriptor;
    sound = {};
    sound.descriptor = descriptor;
    sound.phase = SoundPhase::reserved;
    sound.generation = ++context_->next_sound;
    sound.sound = id;
    sound.parameters = sound.desired = parameters;
    handle = {context_->owner, sound.generation, id_.slot, slot};
    player->inbox[player->inbox_count++] = {PlayerEventKind::start, handle, id, parameters};
    return Error::none;
}

Error SoundPlayer::stage_pitch(SoundHandle handle, float pitch) noexcept {
    std::uint16_t word = 0;
    if (!detail::pitch_word(pitch, word)) return Error::invalid_parameter;
    PlayerEvent event{PlayerEventKind::pitch, handle};
    event.parameters.pitch = pitch;
    return post(context_, id_, event);
}

Error SoundPlayer::stage_pan(SoundHandle handle, std::uint8_t pan) noexcept {
    if (pan > 127) return Error::invalid_parameter;
    PlayerEvent event{PlayerEventKind::pan, handle};
    event.parameters.pan = pan;
    return post(context_, id_, event);
}

Error SoundPlayer::stage_fx_mix(SoundHandle handle, std::uint8_t mix) noexcept {
    if (mix > 127) return Error::invalid_parameter;
    PlayerEvent event{PlayerEventKind::fx, handle};
    event.parameters.fx_mix = mix;
    return post(context_, id_, event);
}

Error SoundPlayer::stage_volume(SoundHandle handle, std::uint16_t volume) noexcept {
    if (volume > 0x7fff) return Error::invalid_parameter;
    PlayerEvent event{PlayerEventKind::volume, handle};
    event.parameters.volume = volume;
    return post(context_, id_, event);
}

Error SoundPlayer::stage_stop(SoundHandle handle) noexcept {
    return post(context_, id_, {PlayerEventKind::stop, handle});
}

Error SoundPlayer::stage() noexcept {
    SfxCheckpoint* player = nullptr;
    const auto error = mutable_player(context_, id_, player);
    if (error != Error::none) return error;
    if (player->inbox_count > player->published_events.size() - player->published_count)
        return Error::event_capacity;
    std::copy_n(player->inbox.begin(), player->inbox_count,
                player->published_events.begin() + player->published_count);
    player->published_count += player->inbox_count;
    player->inbox_count = 0;
    return Error::none;
}

Error SoundPlayer::status(SoundHandle handle, Status& status) const noexcept {
    status = {};
    SfxCheckpoint* player = nullptr;
    auto error = player_state(context_, id_, player);
    if (error != Error::none) return error;
    PlayerSound* sound = nullptr;
    error = playback(*context_, id_, *player, handle, sound);
    if (error != Error::none) return error;
    status.active = playing(*sound);
    status.voice = sound->descriptor;
    status.error = sound->error;
    BindingInfo binding{};
    error = detail::voice_binding(*context_, context_->state(), sound->descriptor, binding);
    if (error != Error::none) return error;
    status.bound = binding.bound;
    if (status.active && sound->phase != SoundPhase::reserved && !status.bound)
        status.error = Error::unbound;
    return Error::none;
}

bool SoundPlayer::active(SoundHandle handle) const noexcept {
    Status value{};
    return status(handle, value) == Error::none && value.active;
}
} // namespace sfx

namespace detail {
Error add_player(Context& context, const sfx::Config& config, PlayerId& id) noexcept {
    id = {};
    if (context.poisoned) return Error::needs_reset;
    if (context.building || context.lease != Lease::none) return Error::task_busy;
    const auto& live = context.state();
    if (live.sample_clock != 0 || live.last_service != 0) return Error::invalid_state;
    if (config.max_sounds == 0 || config.max_sounds > sfx::SoundPlayer::max_sounds ||
        rounded_frames(16000, context.config.synthesis_rate) == 0)
        return Error::invalid_parameter;
    std::uint16_t slot = 0;
    while (slot < AudioService::max_players && live.clients[slot].live) ++slot;
    if (slot == AudioService::max_players) return Error::client_capacity;
    std::uint16_t typed_index = 0;
    while (typed_index < context.sfx.size() && context.sfx[typed_index]) ++typed_index;
    if (typed_index == context.sfx.size()) return Error::client_capacity;
    std::size_t available = 0;
    for (std::uint16_t i = 0; i < context.config.logical_voices; ++i)
        if (!live.logical[i].live) ++available;
    if (available < config.max_sounds) return Error::descriptor_capacity;
    const auto limit = std::numeric_limits<std::uint64_t>::max();
    if (context.next_player == limit || context.next_voice > limit - config.max_sounds)
        return Error::identity_exhausted;

    // Acquire all throwing storage before touching descriptors or identities.
    std::unique_ptr<SfxCheckpointPair> checkpoints;
    try {
        checkpoints = std::make_unique<SfxCheckpointPair>();
    } catch (const std::bad_alloc&) {
        return Error::allocation_failure;
    }
    const auto bank = prepare_shadow(context);
    auto& state = context.cpu[bank];
    auto& player = checkpoints->banks[bank];
    player.max_sounds = config.max_sounds;
    player.priority = config.priority;
    player.client_us = player.poll_us = config.first_client_us;
    for (std::uint16_t i = 0; i < config.max_sounds; ++i) {
        const auto error = create_voice(context, state, player.sounds[i].descriptor);
        if (error != Error::none) return error;
    }
    checkpoints->banks[bank ^ 1u] = player;
    ClientCalendar client{};
    client.live = true;
    client.generation = client.registration_order = context.next_player + 1;
    client.next_raw_due = config.first_due ? config.first_due->value : state.sample_clock;
    client.kind = ClientKind::sfx;
    client.typed_index = typed_index;
    state.clients[slot] = client;
    context.sfx[typed_index] = std::move(checkpoints);
    context.next_player = client.generation;
    context.live = bank;
    id = {context.owner, client.generation, slot};
    return Error::none;
}

Error dispatch_player(Context& context, unsigned bank, std::uint16_t slot) noexcept {
    if (bank >= context.cpu.size() || slot >= AudioService::max_players)
        return Error::stale_handle;
    auto& state = context.cpu[bank];
    auto& client = state.clients[slot];
    if (!client.live || client.kind != ClientKind::sfx ||
        client.typed_index >= context.sfx.size() || !context.sfx[client.typed_index])
        return Error::stale_handle;
    auto& player = context.sfx_state(bank, client.typed_index);
    const ParamTime time{client.next_raw_due & ~std::uint64_t{15}};
    if (time.value != state.sample_clock || player.client_us > player.poll_us)
        return Error::invalid_state;
    for (std::uint16_t i = 0; i < player.max_sounds; ++i) {
        auto& sound = player.sounds[i];
        if (sound.event_us == 0) continue;
        if (sound.event_us < player.client_us) return Error::invalid_state;
        if (sound.event_us != player.client_us) continue;
        sound.event_us = 0;
        if (sound.phase == SoundPhase::attack) {
            const auto error = decay_sound(context, state, player, sound, time);
            if (error != Error::none) return error;
        } else if (sound.phase == SoundPhase::decay) {
            const auto error = release_sound(context, state, player, sound, time);
            if (error != Error::none) return error;
        } else if (sound.phase == SoundPhase::release) {
            const auto error = finish_sound(context, state, sound, time);
            if (error != Error::none) return error;
        } else {
            return Error::invalid_state;
        }
    }
    for (std::size_t i = 0; i < player.published_count; ++i) {
        const auto& event = player.published_events[i];
        if (event.handle.player != slot) return Error::invalid_state;
        const auto error = apply_event(context, state, player, event, time);
        if (error != Error::none) return error;
    }
    player.published_count = 0;
    if (player.client_us == player.poll_us) {
        if (player.poll_us > std::numeric_limits<std::uint64_t>::max() - 16000u)
            return Error::identity_exhausted;
        player.poll_us += 16000u;
    }
    auto due_us = player.poll_us;
    for (std::uint16_t i = 0; i < player.max_sounds; ++i) {
        const auto event_us = player.sounds[i].event_us;
        if (event_us != 0) due_us = std::min(due_us, event_us);
    }
    if (due_us <= player.client_us ||
        due_us - player.client_us > static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max()))
        return Error::identity_exhausted;
    const auto delta = rounded_frames(static_cast<std::int32_t>(due_us - player.client_us),
                                       context.config.synthesis_rate);
    if (client.next_raw_due > std::numeric_limits<std::uint64_t>::max() - delta)
        return Error::identity_exhausted;
    client.next_raw_due += delta;
    player.client_us = due_us;
    return Error::none;
}
} // namespace detail
} // namespace isle::audio
