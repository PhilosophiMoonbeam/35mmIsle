#ifndef ISLE_AUDIO_CSP_HPP
#define ISLE_AUDIO_CSP_HPP

#include "35mmisle/audio_cseq.hpp"
#include "35mmisle/audio_synth.hpp"
#include <optional>

namespace isle::audio::csp {
struct Config {
    BankFile bank;
    std::uint32_t bank_index = 0;
    SequenceFile sequences;
    std::uint16_t max_notes = 16;
    std::optional<RawSampleTime> first_due = {};
    std::uint64_t first_client_us = 0;
};
enum class Playback { stopped, playing, stopping };
struct ChannelStatus {
    std::uint8_t program = 0, volume = 127, pan = 64, priority = 0, fx_mix = 0;
    std::uint8_t gain_scale = 120; // Independently processed MIDI CC21.
    std::int8_t extra_volume = -1;
    std::uint16_t bend = 8192;
    bool sustain = false, percussion = false;
};
struct Status {
    Playback playback = Playback::stopped;
    bool bound = false, ended = false; // Sequence binding/cursor, not DSP/FX readiness.
    std::uint32_t song = 0, tempo = 0;
    std::uint16_t active_notes = 0, bound_notes = 0;
    std::size_t scheduled_events = 0; // Note events only; excludes staged/published controls.
};
// Nonowning, serialized control view. Setters fill an inbox; stage() publishes
// it atomically. Getters describe processed CPU state, never pending controls.
class SequencePlayer final {
public:
    static constexpr std::uint16_t max_notes = 16;
    // Native accounting: live note events + inbox + published controls <=64.
    // One fixed reader lookahead and the poll deadline are separate fields;
    // this is not a claim about opaque original SDK queue-pool occupancy.
    static constexpr std::size_t event_capacity = 64;
    Error stage_sequence(std::uint32_t song) noexcept;
    Error stage_play() noexcept;
    Error stage_stop() noexcept;
    Error stage_volume(std::uint16_t) noexcept; // Direct Q15; game callers cap at 0x7f00.
    Error stage_tempo(std::uint32_t microseconds_per_quarter) noexcept;
    Error stage_channel_program(std::uint8_t channel, std::uint8_t) noexcept;
    Error stage_channel_pan(std::uint8_t channel, std::uint8_t) noexcept;
    Error stage_channel_volume(std::uint8_t channel, std::uint8_t) noexcept;
    Error stage_channel_priority(std::uint8_t channel, std::uint8_t) noexcept;
    Error stage_channel_fx(std::uint8_t channel, std::uint8_t) noexcept;
    Error stage_channel_extra_volume(std::uint8_t channel, std::int8_t) noexcept;
    // Applies at future starts/channel-FX edits; does not retarget existing notes.
    Error stage_player_extra_fx(std::int8_t) noexcept;
    Error stage() noexcept;
    Error status(Status&) const noexcept;
    Error channel_status(std::uint8_t channel, ChannelStatus&) const noexcept;
    Error tempo(std::uint32_t&) const noexcept;
private:
    SequencePlayer(detail::Context* context, PlayerId id) noexcept : context_(context), id_(id) {}
    detail::Context* context_;
    PlayerId id_;
    friend class isle::audio::AudioService;
};
} // namespace isle::audio::csp
#endif
