#ifndef ISLE_AUDIO_SYNTH_INTERNAL_HPP
#define ISLE_AUDIO_SYNTH_INTERNAL_HPP

#include "35mmisle/audio_synth.hpp"
#include "35mmisle/audio_sfx.hpp"
#include "35mmisle/audio_csp.hpp"
#include "audio_effects_internal.hpp"
#include <limits>
#include <vector>

namespace isle::audio::detail {
inline constexpr std::size_t max_callback_steps = 4096;
struct CatalogWave {
    const Wave* resource = nullptr;
    Error admission = Error::none;
    std::uint32_t sample_address = 0, sample_frames = 0;
    std::uint32_t book_address = 0, book_bytes = 0, loop_address = 0;
};
struct CatalogSound { const Sound* resource = nullptr; std::uint32_t wave_index = 0; };
struct Catalog {
    std::vector<BankFile> owners;
    std::vector<CatalogWave> waves;
    std::vector<CatalogSound> sounds;
    std::uint32_t resource_end = AudioService::resource_address;
};
struct LogicalVoice {
    bool live = false;
    std::uint64_t generation = 0;
    std::uint16_t physical = no_slot;
    std::int16_t priority = 0;
};
struct RenderVoice {
    bool active = false, resample_init = true, envelope_init = true;
    bool source_finished = false;
    std::uint8_t adpcm_flags = 1;
    WaveId wave{};
    std::uint64_t epoch = 0;
    std::uint64_t binding_epoch = 0;
    DirectParameters parameters{};
    std::uint16_t pitch = 0x8000;
    std::uint32_t cursor = 0, decoded_to = 0, loops_remaining = 0;
    std::uint32_t deferred_loops = 0, fraction = 0;
    // Event-authored CPU duration/progress; ENV emission never restarts it.
    std::uint32_t gain_frames = 0, gain_elapsed = 0;
    std::array<std::uint16_t, 2> gain_from{}, gain_to{}, retarget_current{};
    std::array<std::uint32_t, 2> gain_rate{};
    bool have_retarget_current = false;
};
struct PendingSource {
    bool valid = false;
    std::uint64_t epoch = 0, due = 0;
    WaveId wave{};
    DirectParameters parameters{};
    RampFrames attack{};
    Gain15 initial_gain{};
    bool initial_attack = false, gain_retarget = false;
};
struct PhysicalVoice {
    bool allocated = false, stealable = true;
    std::uint16_t owner = no_slot;
    std::uint16_t next = no_slot, previous = no_slot;
    bool retire_ready = false;
    std::uint64_t epoch = 0, allocation_age = 0, restart_due = 0;
    RenderVoice current{};
    PendingSource pending{};
};
enum class ParameterKind { start, pitch, pan, fx, gain, stop, kill, retire };
struct ParameterEvent {
    ParameterKind kind = ParameterKind::stop;
    std::uint16_t physical = no_slot;
    std::uint64_t due = 0, sequence = 0, epoch = 0, source_epoch = 0;
    WaveId wave{};
    DirectParameters parameters{};
    RampFrames ramp{};
};
enum class SoundPhase { free, reserved, attack, decay, sustain, release, failed };
struct PlayerSound {
    SoundPhase phase = SoundPhase::free;
    bool published = false, stop_requested = false;
    std::uint64_t generation = 0, event_us = 0;
    VoiceHandle descriptor{};
    SoundId sound{};
    sfx::Parameters parameters{}, desired{};
    std::int16_t priority = 0;
    Error error = Error::none;
};
enum class PlayerEventKind { start, pitch, pan, fx, volume, stop };
struct PlayerEvent {
    PlayerEventKind kind = PlayerEventKind::stop;
    sfx::SoundHandle handle{};
    SoundId sound{};
    sfx::Parameters parameters{};
};
enum class ClientKind { sfx, csp };
struct ClientCalendar {
    bool live = false;
    std::uint64_t generation = 0, registration_order = 0, next_raw_due = 0;
    ClientKind kind = ClientKind::sfx;
    std::uint16_t typed_index = no_slot;
};
struct SfxCheckpoint {
    std::uint16_t max_sounds = 0;
    std::int16_t priority = 0;
    std::uint64_t client_us = 0, poll_us = 0;
    std::array<PlayerSound, sfx::SoundPlayer::max_sounds> sounds{};
    std::array<PlayerEvent, sfx::SoundPlayer::event_capacity> inbox{}, published_events{};
    std::size_t inbox_count = 0, published_count = 0;
};
struct SfxCheckpointPair {
    std::array<SfxCheckpoint, 2> banks{};
};
struct CspChannel {
    csp::ChannelStatus controls{};
    const Instrument* instrument = nullptr;
};
enum class CspPhase { free, attack, decay, sustain, release };
struct CspNote {
    VoiceHandle descriptor{};
    SoundId sound{};
    std::uint64_t incarnation = 0;
    CspPhase phase = CspPhase::free;
    std::uint8_t channel = 0, key = 0, velocity = 0;
    bool sustained = false;
    float base_pitch = 1.0f, vibrato = 1.0f, tremolo = 127.0f;
    std::uint16_t tremolo_slot = no_slot, vibrato_slot = no_slot;
    float envelope_factor = 1.0f;
    std::uint64_t envelope_start = 0, envelope_end = 0;
    std::uint8_t envelope_from = 0, envelope_to = 0;
};
enum class CspControlKind : std::uint8_t { bind, play, stop, volume, tempo, program, pan,
                                         channel_volume, priority, fx, extra_volume, extra_fx };
struct CspControl {
    CspControlKind kind = CspControlKind::stop;
    std::uint8_t channel = 0;
    std::uint32_t value = 0;
};
static_assert(sizeof(CspControl) == 8);
enum class CspEventKind : std::uint8_t { off, envelope, tremolo, vibrato };
struct CspEvent {
    bool live = false;
    CspEventKind kind = CspEventKind::off;
    std::uint16_t note = no_slot;
    std::uint64_t cue = 0, incarnation = 0, due = 0, serial = 0;
};
struct CspCheckpoint {
    SequenceReader reader{};
    SequenceEvent next{};
    bool bound = false, have_next = false, ended = false;
    csp::Playback playback = csp::Playback::stopped;
    std::uint32_t song = 0, us_per_tick = 0;
    float quarter_per_tick = 0.0f;
    std::uint16_t volume = 0x7800; // Measured effective constructor master, not GAME bookkeeping.
    std::int8_t extra_fx = 0;
    std::uint64_t cue = 0, next_incarnation = 0, next_serial = 0;
    std::uint64_t client_us = 0, poll_us = 0, sequence_due = 0;
    std::uint64_t resume_delta_ticks = 0;
    std::array<CspChannel, 16> channels{};
    std::array<CspNote, csp::SequencePlayer::max_notes> notes{};
    std::array<CspEvent, csp::SequencePlayer::event_capacity> events{};
    std::array<CspControl, csp::SequencePlayer::event_capacity> inbox{}, published{};
    std::size_t inbox_count = 0, published_count = 0, event_count = 0;
};
struct CspSoundMapping { const Sound* resource = nullptr; SoundId id{}; WaveId wave{}; };
struct CspResources {
    csp::Config config{};
    const Bank* bank = nullptr;
    std::vector<CspSoundMapping> sounds;
};
struct CspCheckpointPair {
    CspResources resources{};
    std::array<CspCheckpoint, 2> banks{};
};
struct Oscillator {
    bool allocated = false, high = true;
    std::uint8_t type = 0, depth = 0;
    std::uint16_t count = 0, period = 0, next = no_slot;
    float depth_cents = 0.0f;
};
struct OscillatorPool {
    std::array<Oscillator, 44> slots{};
    std::uint16_t free_head = no_slot;
    bool initialized = false;
};
struct CpuState {
    std::array<LogicalVoice, AudioService::max_logical_voices> logical{};
    std::array<PhysicalVoice, AudioService::max_physical_voices> physical{};
    std::array<ClientCalendar, AudioService::max_players> clients{};
    OscillatorPool oscillators{};
    EffectsState effects{};
    std::uint16_t free_head = no_slot, allocated_head = no_slot, retiring_head = no_slot;
    bool lists_initialized = false;
    // Fixed min-heap by (due, sequence); no per-event erase/memmove or allocation.
    std::array<ParameterEvent, AudioService::parameter_capacity> parameters{};
    std::size_t parameter_count = 0;
    std::uint64_t next_parameter_sequence = 0, next_binding_epoch = 0, next_allocation_age = 0;
    std::uint64_t sample_clock = 0, last_service = 0;
};
enum class Lease { none, executing, pcm };
struct Context {
    SynthConfig config{};
    Coefficients coefficients{};
    Catalog catalog{};
    EffectsProgram effects{};
    std::unique_ptr<std::uint32_t[]> memory;
    std::array<CpuState, 2> cpu{};
    // Typed storage is acquired only at quiescent registration, never by a task.
    std::array<std::unique_ptr<SfxCheckpointPair>, AudioService::max_players> sfx{};
    std::array<std::unique_ptr<CspCheckpointPair>, AudioService::max_players> csp{};
    unsigned live = 0;
    std::uint64_t owner = 0, next_task = 0;
    // External descriptor/playback/registration reservations are monotonic across reset.
    std::uint64_t next_voice = 0, next_sound = 0, next_player = 0;
    Lease lease = Lease::none;
    bool poisoned = false, building = false;
    TaskView task{};
    PcmView pcm{};
    std::uint32_t write_address = AudioService::command_address;
    Error write_error = Error::none;
    CpuState& state() noexcept { return cpu[live]; }
    const CpuState& state() const noexcept { return cpu[live]; }
    SfxCheckpoint& sfx_state(unsigned bank, std::uint16_t typed_index) noexcept {
        return sfx[typed_index]->banks[bank];
    }
    const SfxCheckpoint& sfx_state(unsigned bank, std::uint16_t typed_index) const noexcept {
        return sfx[typed_index]->banks[bank];
    }
    CspCheckpoint& csp_state(unsigned bank, std::uint16_t index) noexcept {
        return csp[index]->banks[bank];
    }
    const CspCheckpoint& csp_state(unsigned bank, std::uint16_t index) const noexcept {
        return csp[index]->banks[bank];
    }
    void put8(std::uint32_t address, std::uint8_t value) noexcept {
        reinterpret_cast<std::uint8_t*>(memory.get())[address ^ 3u] = value;
    }
    std::uint8_t get8(std::uint32_t address) const noexcept {
        return reinterpret_cast<const std::uint8_t*>(memory.get())[address ^ 3u];
    }
    void put16(std::uint32_t address, std::uint16_t value) noexcept {
        put8(address, static_cast<std::uint8_t>(value >> 8));
        put8(address + 1, static_cast<std::uint8_t>(value));
    }
    void put32(std::uint32_t address, std::uint32_t value) noexcept {
        memory[address / 4] = value;
    }
    bool range(std::uint32_t address, std::uint32_t bytes) const noexcept {
        return address <= AudioService::rdram_bytes && bytes <= AudioService::rdram_bytes - address;
    }
    void emit(std::uint32_t first, std::uint32_t second) noexcept {
        if (write_error != Error::none) return;
        if (write_address > AudioService::command_limit - 8u) {
            write_error = Error::command_capacity; return;
        }
        put32(write_address, first); put32(write_address + 4, second); write_address += 8;
    }
};

// Qualified scalar routines; retained independently of game/player gain policy.
bool pitch_word(float, std::uint16_t&) noexcept;
float cents_ratio(std::int32_t) noexcept;
float integer_power(float, std::uint32_t) noexcept;
std::uint32_t rounded_frames(std::int32_t microseconds, std::uint32_t rate) noexcept;
std::uint32_t duration_frames(std::int32_t microseconds, std::uint32_t rate) noexcept;
bool scaled_duration_us(std::int32_t, float pitch, std::int32_t&) noexcept;
std::uint32_t envelope_rate(std::uint16_t from, std::uint16_t to, std::uint32_t frames,
                            const Coefficients&) noexcept;
std::uint16_t cpu_gain(std::uint16_t from, std::uint16_t to, std::uint32_t rate,
                       std::uint32_t elapsed, std::uint32_t duration) noexcept;

// Catalog writer owns these definitions; all packing is creation-time and atomic.
Error build_catalog(Context&, std::span<const BankFile>) noexcept;
Error find_sound(const Context&, const Sound&, SoundId&) noexcept;
Error catalog_wave(const Context&, WaveId, const CatalogWave*&) noexcept;
Error catalog_sound(const Context&, SoundId, const CatalogSound*&) noexcept;

// Core writer owns direct synthesis and fixed-order shared DSP emission.
Error create_voice(Context&, CpuState&, VoiceHandle&) noexcept;
Error destroy_voice(Context&, CpuState&, VoiceHandle) noexcept;
Error allocate_voice(Context&, CpuState&, VoiceHandle, std::int16_t, ParamTime, Allocation&) noexcept;
Error priority_voice(Context&, CpuState&, VoiceHandle, std::int16_t) noexcept;
Error start_voice(Context&, CpuState&, VoiceHandle, WaveId, DirectParameters, RampFrames, ParamTime) noexcept;
Error pitch_voice(Context&, CpuState&, VoiceHandle, float, ParamTime) noexcept;
Error pan_voice(Context&, CpuState&, VoiceHandle, std::uint8_t, ParamTime) noexcept;
Error gain_voice(Context&, CpuState&, VoiceHandle, Gain15, RampFrames, ParamTime) noexcept;
Error fx_voice(Context&, CpuState&, VoiceHandle, std::uint8_t, ParamTime) noexcept;
Error stop_voice(Context&, CpuState&, VoiceHandle, ParamTime) noexcept;
Error free_voice(Context&, CpuState&, VoiceHandle, ParamTime) noexcept;
Error voice_binding(const Context&, const CpuState&, VoiceHandle, BindingInfo&) noexcept;
// Applies pending events through this boundary before callbacks query allocation.
// Rendering calls this at its own span boundaries; it never acknowledges RSP work.
Error advance_parameters(Context&, CpuState&, ParamTime) noexcept;
// CPU AudioFrame collection reverses the retiring list onto the free head.
void collect_voices(CpuState&) noexcept;
Error render(Context&, CpuState&, std::uint32_t frames, std::uint32_t task_offset,
             std::uint32_t task_frames) noexcept;

// Copies common CPU state and every registered typed checkpoint into 1-live.
// Resources and ownership remain outside these value-only transaction banks.
unsigned prepare_shadow(Context&) noexcept;
Error dispatch_client(Context&, unsigned bank, std::uint16_t client) noexcept;

// Client writer owns setup, staging and explicitly banked callback dispatch.
Error add_player(Context&, const sfx::Config&, PlayerId&) noexcept;
Error dispatch_player(Context&, unsigned bank, std::uint16_t client) noexcept;
Error add_sequence_player(Context&, const csp::Config&, PlayerId&) noexcept;
Error dispatch_sequence_player(Context&, unsigned bank, std::uint16_t client) noexcept;
} // namespace isle::audio::detail
#endif
