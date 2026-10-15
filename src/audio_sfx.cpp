#include "35mmisle/audio_sfx.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <limits>
#include <new>
#include <utility>

namespace isle::audio::sfx {
namespace {
constexpr std::uint32_t chunk_frames = 160;
constexpr std::uint32_t decode_chunk = 112;
// ABI1 command DMEM operands are relative to its 0x5c0 sample area.
constexpr std::uint16_t stream = 0x008; // preceding eight bytes: resampler history
constexpr std::uint16_t decoded = 0x2a0;
constexpr std::uint16_t encoded = 0x3c0;
// Decoder and encoded scratch are dead when RESAMPLE writes the mono buffer.
constexpr std::uint16_t mono = decoded;
constexpr std::uint16_t left_bus = 0x440;
constexpr std::uint16_t right_bus = 0x580;
constexpr std::uint16_t wet_left = 0x6c0;
constexpr std::uint16_t wet_right = 0x800;
static_assert(stream + chunk_frames * 4u <= decoded);
static_assert(decoded + 32u + decode_chunk * 2u <= encoded);
static_assert(encoded + 72u <= left_bus);
static_assert(mono + chunk_frames * 2u <= left_bus);
static_assert(wet_right + chunk_frames * 2u <= 0x940);
// Source scratch is dead after RESAMPLE saves history. Reuse it for output;
// no overlap with live buses or the task/command area at the top of DMEM.
constexpr std::uint16_t interleaved = 0;

constexpr std::uint32_t align8(std::uint32_t n) noexcept { return (n + 7u) & ~7u; }
constexpr std::uint32_t align16(std::uint32_t n) noexcept { return (n + 15u) & ~15u; }
bool range(std::uint32_t address, std::uint32_t length) noexcept {
    return address <= Producer::rdram_bytes && length <= Producer::rdram_bytes - address;
}
bool pitch_word(float pitch, std::uint16_t& result) noexcept {
    if (!std::isfinite(pitch) || pitch < 1.0f / 32768.0f || pitch > 65535.0f / 32768.0f)
        return false;
    result = static_cast<std::uint16_t>(static_cast<double>(pitch) * 32768.0);
    return result != 0;
}
bool valid_parameters(const Parameters& p) noexcept {
    std::uint16_t unused = 0;
    return p.volume <= 0x7fff && p.pan <= 127 && pitch_word(p.pitch, unused);
}
std::uint32_t rounded_frames(std::int32_t us, std::uint32_t rate) noexcept {
    // Original conversion uses a single-precision product, double division
    // and rounding, then a single-precision result before integer truncation.
    const float product = static_cast<float>(us) * static_cast<float>(rate);
    const float rounded = static_cast<float>(static_cast<double>(product) / 1000000.0 + 0.5);
    return static_cast<std::uint32_t>(rounded);
}
std::uint32_t duration_frames(std::int32_t us, std::uint32_t rate) noexcept {
    // DSP parameter times, unlike client callback deadlines, discard low bits.
    return rounded_frames(us, rate) & ~15u;
}
bool scaled_duration_us(std::int32_t us, float pitch, std::int32_t& result) noexcept {
    const float scaled = static_cast<float>(us) / pitch;
    if (!std::isfinite(scaled) || static_cast<double>(scaled) > std::numeric_limits<std::int32_t>::max())
        return false;
    result = static_cast<std::int32_t>(scaled);
    return true;
}
std::uint32_t envelope_rate(std::uint16_t from, std::uint16_t to,
                            std::uint32_t samples, const Coefficients& coefficients) noexcept {
    if (samples == 0)
        return to < from ? 0u : 0x7fffffffu;
    const auto current = std::max<std::uint16_t>(from, 1);
    const auto target = std::max<std::uint16_t>(to, 1);
    int exponent = 0;
    const double mantissa = std::frexp(static_cast<double>(target) / current, &exponent);
    const auto bucket = static_cast<std::size_t>(mantissa * 16.0) - 8u;
    const double factor = std::exp2((exponent + coefficients.logarithm[bucket]) * 8.0 / samples);
    if (factor >= 32768.0) return 0x7fffffffu;
    const auto whole = static_cast<std::uint32_t>(factor);
    const auto fraction = static_cast<std::uint32_t>((factor - whole) * 65535.0);
    return (whole << 16) | fraction;
}
std::uint16_t cpu_gain(std::uint16_t from, std::uint16_t to, std::uint32_t rate,
                       std::uint32_t elapsed, std::uint32_t duration) noexcept {
    if (elapsed >= duration) return to;
    float power = 1.0f;
    float multiplier = static_cast<float>(rate) / 65536.0f;
    auto groups = elapsed / 8u;
    while (groups != 0) {
        if ((groups & 1u) != 0) power *= multiplier;
        groups >>= 1;
        if (groups != 0) multiplier *= multiplier;
    }
    // This is the original CPU estimate, not a scalar reconstructed from the
    // DSP's eight fractional lanes. The nominal endpoint snaps only CPU state.
    const float value = power * static_cast<float>(from);
    return static_cast<std::uint16_t>(std::clamp(value, 0.0f, 32767.0f));
}
bool contains(const BankFile& owner, const Sound* selected) noexcept {
    for (const Bank* bank : owner.banks()) {
        if (bank == nullptr) continue;
        if (bank->percussion != nullptr)
            for (const Sound* sound : bank->percussion->sounds)
                if (sound == selected) return true;
        for (const Instrument* instrument : bank->instruments) {
            if (instrument == nullptr) continue;
            for (const Sound* sound : instrument->sounds)
                if (sound == selected) return true;
        }
    }
    return false;
}
} // namespace

Error service_quota(Config config, Service service, std::uint32_t& frames) noexcept {
    frames = 0;
    if (config.synthesis_rate == 0 || config.synthesis_rate > 96000 || service.service_id == 0)
        return Error::invalid_service;
    if (service.quota_frames != 0) {
        if ((service.quota_frames & 15u) != 0 || service.quota_frames > Producer::max_frames)
            return Error::invalid_service;
        frames = service.quota_frames;
        return Error::none;
    }
    const std::int64_t nominal = align16(config.synthesis_rate / 60u + 1u);
    // Signed math matters: a large occupancy may make the variable term
    // negative, in which case the minimum quota wins (never unsigned wraps).
    const std::int64_t raw = 16 + nominal - static_cast<std::int64_t>(service.previous_ai_frames) + 80;
    const std::int64_t aligned = raw >= 0 ? (raw / 16) * 16 : -(((-raw + 15) / 16) * 16);
    const std::int64_t quota = std::max(nominal - 16, aligned);
    if (quota <= 0 || quota > Producer::max_frames) return Error::invalid_service;
    frames = static_cast<std::uint32_t>(quota);
    return Error::none;
}

struct Producer::Impl {
    enum class Phase { attack, sustain, release, done };
    enum class EventKind { start, pitch, pan, volume, stop };
    enum class Lease { none, executing, pcm };
    struct Event {
        EventKind kind = EventKind::stop;
        std::uint64_t generation = 0;
        Parameters parameters{};
    };
    struct Control {
        bool active = false, stop = false;
        std::uint64_t generation = 0;
        Parameters parameters{};
    };
    struct Voice {
        bool active = false, resample_init = true, envelope_init = true;
        bool source_finished = false;
        std::uint8_t adpcm_flags = 1;
        std::uint64_t generation = 0;
        Parameters parameters{};
        std::uint16_t pitch = 0x8000;
        std::uint32_t cursor = 0, decoded_to = 0, loops_remaining = 0;
        // Finite repeats retire after the enclosing outer160-frame pull,
        // not after decoder calls, callback subdivisions or individual wraps.
        std::uint32_t deferred_loops = 0;
        std::uint32_t fraction = 0;
        Phase phase = Phase::done;
        std::uint64_t event_us = 0;
        std::uint32_t pending_frames = 0, gain_frames = 0, gain_elapsed = 0;
        std::array<std::uint16_t, 2> gain_from{}, gain_to{}, retarget_current{};
        std::array<std::uint32_t, 2> gain_rate{};
        bool have_retarget_current = false;
    };

    BankFile owner;
    const Sound* sound;
    Config config;
    Coefficients coefficients;
    std::unique_ptr<std::uint32_t[]> memory;
    std::array<Event, Producer::event_capacity> events{};
    std::size_t event_count = 0;
    Control control{};
    Voice voice{}, pending_voice{};
    Lease lease = Lease::none;
    bool poisoned = false;
    std::uint64_t next_generation = 0, reserved_generation = 0;
    std::uint64_t next_task = 0, last_service = 0;
    std::uint64_t sample_clock = 0, next_client_sample = 0;
    std::uint64_t next_client_us = 0, next_poll_us = 0;
    const std::uint32_t max_client_step;
    TaskView task{};
    std::uint32_t sample_frames = 0, book_bytes = 0;
    std::uint32_t write_address = Producer::command_address;
    Error build_error = Error::none;

    Impl(BankFile retained, const Sound& selected, Config settings, const Coefficients& data)
        : owner(std::move(retained)), sound(&selected), config(settings), coefficients(data),
          memory(std::make_unique<std::uint32_t[]>(Producer::rdram_bytes / sizeof(std::uint32_t))),
          max_client_step(rounded_frames(16000, settings.synthesis_rate)) {}

    bool owns_pcm(const PcmView& pcm) const noexcept {
        return lease == Lease::pcm && pcm.task_id == task.task_id && pcm.generation == task.generation &&
            pcm.address == task.pcm_address && pcm.frame_count == task.frame_count && pcm.synthesis_rate == task.synthesis_rate;
    }
    void put8(std::uint32_t address, std::uint8_t value) noexcept {
        reinterpret_cast<std::uint8_t*>(memory.get())[address ^ 3u] = value;
    }
    void put16(std::uint32_t address, std::uint16_t value) noexcept {
        put8(address, static_cast<std::uint8_t>(value >> 8));
        put8(address + 1, static_cast<std::uint8_t>(value));
    }
    void put32(std::uint32_t address, std::uint32_t value) noexcept {
        put16(address, static_cast<std::uint16_t>(value >> 16));
        put16(address + 2, static_cast<std::uint16_t>(value));
    }
    void command(std::uint32_t a, std::uint32_t b) noexcept {
        if (build_error != Error::none) return;
        if (write_address > Producer::command_limit - 8u) {
            build_error = Error::command_capacity;
            return;
        }
        put32(write_address, a);
        put32(write_address + 4, b);
        write_address += 8;
    }
    bool address(std::uint32_t a, std::uint32_t bytes) noexcept {
        if (range(a, bytes)) return true;
        build_error = Error::address_range;
        return false;
    }
    void buffers(std::uint16_t input, std::uint16_t output, std::uint16_t bytes) noexcept {
        command(0x08000000u | input, (static_cast<std::uint32_t>(output) << 16) | bytes);
    }
    void clear(std::uint16_t output, std::uint16_t bytes) noexcept {
        if (bytes != 0) command(0x02000000u | output, bytes);
    }
    void move(std::uint16_t input, std::uint16_t output, std::uint16_t bytes) noexcept {
        if (bytes != 0) command(0x0a000000u | input, (static_cast<std::uint32_t>(output) << 16) | bytes);
    }
    void load(std::uint32_t source, std::uint16_t dest, std::uint16_t bytes) noexcept {
        if (!address(source, bytes)) return;
        buffers(dest, 0, bytes);
        command(0x04000000u, source);
    }
    void save(std::uint16_t source, std::uint32_t dest, std::uint16_t bytes) noexcept {
        if (!address(dest, bytes)) return;
        buffers(0, source, bytes);
        command(0x06000000u, dest);
    }

    std::array<std::uint16_t, 2> channel_volumes(const Voice& v) const noexcept {
        const auto perceived = (static_cast<std::uint32_t>(v.parameters.volume) *
                                sound->envelope->attack_volume) >> 7;
        const std::uint32_t amplitude = (perceived * perceived) >> 15;
        const auto pan = v.parameters.pan;
        return {static_cast<std::uint16_t>((amplitude * coefficients.pan[pan]) >> 15),
                static_cast<std::uint16_t>((amplitude * coefficients.pan[127u - pan]) >> 15)};
    }
    std::array<std::uint16_t, 2> current_gains(const Voice& v) const noexcept {
        if (v.have_retarget_current) return v.retarget_current;
        return {cpu_gain(v.gain_from[0], v.gain_to[0], v.gain_rate[0], v.gain_elapsed, v.gain_frames),
                cpu_gain(v.gain_from[1], v.gain_to[1], v.gain_rate[1], v.gain_elapsed, v.gain_frames)};
    }
    bool phase_duration(Voice& v, std::int32_t us, std::uint64_t now_us) noexcept {
        std::int32_t scaled = 0;
        if (!scaled_duration_us(us, v.parameters.pitch, scaled)) {
            build_error = Error::invalid_parameter;
            return false;
        }
        v.pending_frames = duration_frames(scaled, config.synthesis_rate);
        if (now_us > std::numeric_limits<std::uint64_t>::max() - static_cast<std::uint32_t>(scaled)) {
            build_error = Error::identity_exhausted;
            return false;
        }
        v.event_us = scaled == 0 ? 0 : now_us + static_cast<std::uint32_t>(scaled);
        return true;
    }
    void initialize(Voice& v, const Control& c, std::uint64_t now_us) noexcept {
        v = Voice{};
        v.active = true;
        v.generation = c.generation;
        v.parameters = c.parameters;
        pitch_word(c.parameters.pitch, v.pitch);
        const Loop* loop = sound->wave->loop;
        v.loops_remaining = loop != nullptr ? loop->count : 0;
        if (!phase_duration(v, sound->envelope->attack_us, now_us)) {
            v.active = false;
            return;
        }
        v.phase = v.event_us == 0 ? Phase::sustain : Phase::attack;
        v.retarget_current = v.phase == Phase::attack ? std::array<std::uint16_t, 2>{1, 1} : channel_volumes(v);
        v.have_retarget_current = true;
    }
    void begin_release(Voice& v, std::uint64_t now_us) noexcept {
        const auto previous = current_gains(v);
        if (!phase_duration(v, sound->envelope->release_us, now_us)) return;
        v.retarget_current = previous;
        v.have_retarget_current = true;
        v.envelope_init = true;
        v.phase = v.event_us == 0 ? Phase::done : Phase::release;
        if (v.phase == Phase::done) v.active = false;
    }
    void apply_control(Voice& v, std::uint64_t now_us) noexcept {
        if (!control.active) return;
        if (v.generation != control.generation) initialize(v, control, now_us);
        if (!v.active) return;
        const auto& desired = control.parameters;
        const bool volume_changed = v.parameters.volume != desired.volume;
        if (v.parameters.pan != desired.pan || volume_changed) {
            v.retarget_current = current_gains(v);
            v.pending_frames = volume_changed ? duration_frames(1000, config.synthesis_rate) :
                                               v.gain_frames - v.gain_elapsed;
            v.have_retarget_current = true;
            v.envelope_init = true;
        }
        v.parameters = desired;
        pitch_word(desired.pitch, v.pitch);
        if (control.stop && v.phase != Phase::release) begin_release(v, now_us);
    }
    void callback(Voice& v, std::uint64_t now_us, std::uint64_t& poll_us) noexcept {
        if (v.event_us != 0 && v.event_us == now_us) {
            v.event_us = 0;
            if (v.phase == Phase::attack) {
                // Indefinite decay adds a client deadline, not another DSP INIT.
                v.phase = Phase::sustain;
            } else {
                v.phase = Phase::done;
                v.active = false;
            }
        }
        apply_control(v, now_us);
        if (now_us == poll_us) {
            if (poll_us > std::numeric_limits<std::uint64_t>::max() - 16000u)
                build_error = Error::identity_exhausted;
            else
                poll_us += 16000u;
        }
    }
    void advance_gain(Voice& v, std::uint32_t frames) const noexcept {
        v.gain_elapsed += std::min(frames, v.gain_frames - v.gain_elapsed);
    }
    void envelope(Voice& v, std::uint32_t frames) noexcept {
        if (v.envelope_init) {
            const auto current = current_gains(v);
            const auto target = v.phase == Phase::release ? std::array<std::uint16_t, 2>{0, 0} : channel_volumes(v);
            for (std::size_t channel = 0; channel != 2; ++channel) {
                const std::uint32_t side = channel == 0 ? 0x00020000u : 0u;
                const auto from = std::max<std::uint16_t>(current[channel], 1);
                const auto to = target[channel];
                // The wire target remains zero during release. The rate
                // calculation alone substitutes one to keep logarithms finite.
                const std::uint32_t rate = envelope_rate(from, to, v.pending_frames, coefficients);
                v.gain_from[channel] = from;
                v.gain_to[channel] = to;
                v.gain_rate[channel] = rate;
                command(0x09040000u | side | from, 0);
                command(0x09000000u | side | to, rate);
            }
            command(0x09087fffu, 0); // unity dry, no auxiliary send
            v.gain_frames = v.pending_frames;
            v.gain_elapsed = 0;
            v.have_retarget_current = false;
        }
        buffers(mono, left_bus, static_cast<std::uint16_t>(frames * 2));
        command(0x08080000u | right_bus,
                (static_cast<std::uint32_t>(wet_left) << 16) | wet_right);
        if (address(Producer::envelope_state_address, 80))
            command(0x03080000u | (v.envelope_init ? 0x00010000u : 0u), Producer::envelope_state_address);
        v.envelope_init = false;
    }

    // Assemble source samples in DMEM. The decoder's 16-sample prefix carries
    // partial frames until the encoded extent finishes at an outer-pull fence;
    // no sample decoding or decoded-sample copies run on the CPU.
    void source(Voice& v, std::uint32_t needed) noexcept {
        if (v.source_finished) {
            // A completed encoded extent makes the next outer pull silent;
            // residual decoded frame history is not another source cache.
            if (needed != 0) clear(stream, static_cast<std::uint16_t>(needed * 2u));
            v.cursor = sample_frames;
            return;
        }
        std::uint32_t written = 0;
        const Loop* loop = sound->wave->loop;
        while (written < needed && build_error == Error::none) {
            if (loop != nullptr && v.loops_remaining != 0 && v.cursor == loop->end) {
                if (v.loops_remaining != 0xffffffffu) ++v.deferred_loops;
                v.cursor = loop->start;
                // History covers the aligned frame containing loop start.
                // A partial start skips its prefix; the next compressed frame
                // follows that restored frame, not sixteen logical samples.
                v.decoded_to = (loop->start & ~15u) + 16u;
                v.adpcm_flags = 2; // restore immutable loop history, not zeros
            }
            if (v.cursor == sample_frames) {
                const auto count = needed - written;
                clear(static_cast<std::uint16_t>(stream + written * 2), static_cast<std::uint16_t>(count * 2));
                break;
            }
            const std::uint32_t boundary = loop != nullptr && v.loops_remaining != 0 ? loop->end : sample_frames;
            const std::uint32_t take = std::min(needed - written, boundary - v.cursor);
            const std::uint32_t available = v.decoded_to - v.cursor;
            if (available != 0) {
                const std::uint32_t copied = std::min(take, available);
                if (v.adpcm_flags == 2) {
                    // A zero-count LOOP restores both the DMEM prefix and
                    // persistent history, even if this pull consumes no new
                    // compressed frame. Later decoding continues that state.
                    buffers(encoded, decoded, 0);
                    // LOOP context is shared microcode workspace, not a
                    // task-lifetime cache: intervening kernels may clobber it.
                    if (address(Producer::loop_state_address, 32))
                        command(0x0f000000u, Producer::loop_state_address);
                    if (address(Producer::adpcm_state_address, 32))
                        command(0x01020000u, Producer::adpcm_state_address);
                    v.adpcm_flags = 0;
                } else {
                    load(Producer::adpcm_state_address, decoded, 32);
                }
                move(static_cast<std::uint16_t>(decoded + 32u - available * 2u),
                     static_cast<std::uint16_t>(stream + written * 2u), static_cast<std::uint16_t>(copied * 2u));
                v.cursor += copied;
                written += copied;
                continue;
            }
            // Decode complete compressed frames in disjoint scratch, even
            // when the logical loop ends inside one. Copy only logical samples.
            const std::uint32_t copied = std::min(take, decode_chunk);
            const std::uint32_t decode_samples = align16(copied);
            const std::uint32_t bytes = (decode_samples / 16u) * 9u;
            const std::uint32_t source_address = Producer::sample_address + (v.cursor / 16u) * 9u;
            const std::uint32_t prefix = source_address & 7u;
            load(source_address & ~7u, encoded, static_cast<std::uint16_t>(align8(prefix + bytes)));
            buffers(static_cast<std::uint16_t>(encoded + prefix), decoded, static_cast<std::uint16_t>(decode_samples * 2u));
            if (address(Producer::adpcm_state_address, 32))
                command(0x01000000u | (static_cast<std::uint32_t>(v.adpcm_flags) << 16), Producer::adpcm_state_address);
            v.adpcm_flags = 0;
            v.decoded_to += decode_samples;
            move(static_cast<std::uint16_t>(decoded + 32u), static_cast<std::uint16_t>(stream + written * 2u),
                 static_cast<std::uint16_t>(copied * 2u));
            v.cursor += copied;
            written += copied;
        }
    }
    void consume_loops(Voice& v) const noexcept {
        if (v.loops_remaining != 0xffffffffu)
            v.loops_remaining -= std::min(v.loops_remaining, v.deferred_loops);
        v.deferred_loops = 0;
        v.source_finished = v.loops_remaining == 0 && v.decoded_to == sample_frames;
    }
    void output(std::uint32_t offset, std::uint32_t frames) noexcept {
        buffers(0, interleaved, static_cast<std::uint16_t>(frames * 2u));
        // Original AUX return mixing is observable even with a zero wet send:
        // the real kernel's fixed-point destination scaling is not an identity.
        command(0x0c007fffu, (static_cast<std::uint32_t>(wet_left) << 16) | left_bus);
        command(0x0c007fffu, (static_cast<std::uint32_t>(wet_right) << 16) | right_bus);
        command(0x0d000000u, (static_cast<std::uint32_t>(left_bus) << 16) | right_bus);
        save(interleaved, Producer::pcm_address + offset * 4u, static_cast<std::uint16_t>(frames * 4u));
    }
    void render(Voice& v, std::uint32_t frames, std::uint32_t output_offset, bool& prepared) noexcept {
        if (v.active && !prepared) {
            prepared = true;
            if (address(Producer::book_address, book_bytes))
                command(0x0b000000u | book_bytes, Producer::book_address);
        }
        std::uint32_t offset = 0;
        while (offset < frames && build_error == Error::none) {
            const std::uint32_t count = std::min(
                chunk_frames - ((output_offset + offset) % chunk_frames), frames - offset);
            clear(left_bus, static_cast<std::uint16_t>(count * 2));
            clear(right_bus, static_cast<std::uint16_t>(count * 2));
            clear(wet_left, static_cast<std::uint16_t>(count * 2u));
            clear(wet_right, static_cast<std::uint16_t>(count * 2u));
            if (v.active) {
                const std::uint64_t total = static_cast<std::uint64_t>(v.fraction) +
                    static_cast<std::uint64_t>(v.pitch) * 2u * count;
                const std::uint32_t needed = static_cast<std::uint32_t>(total >> 16);
                source(v, needed);
                buffers(stream, mono, static_cast<std::uint16_t>(count * 2u));
                if (address(Producer::resample_state_address, 32))
                    command(0x05000000u | (v.resample_init ? 0x00010000u : 0u) | v.pitch,
                            Producer::resample_state_address);
                v.resample_init = false;
                v.fraction = static_cast<std::uint32_t>(total & 0xffffu);
                envelope(v, count);
                advance_gain(v, count);
            }
            output(output_offset + offset, count);
            offset += count;
            if ((output_offset + offset) % chunk_frames == 0) consume_loops(v);
        }
    }
    bool accepts(std::uint64_t generation) const noexcept {
        return generation != 0 && ((control.active && control.generation == generation) || reserved_generation == generation);
    }
    Error post(Event event) noexcept {
        if (poisoned) return Error::needs_reset;
        if (!accepts(event.generation)) return Error::stale_generation;
        if (event_count == events.size()) return Error::event_capacity;
        events[event_count++] = event;
        return Error::none;
    }
};

Producer::Producer(std::unique_ptr<Impl> implementation) noexcept : impl_(std::move(implementation)) {}
Producer::~Producer() = default;

CreateResult Producer::create(BankFile owner, const Sound& selected, Config config,
                              const Coefficients& coefficients) noexcept {
    if (!owner || selected.envelope == nullptr || selected.wave == nullptr || selected.keymap == nullptr)
        return {Error::invalid_resource, nullptr};
    if (!contains(owner, &selected)) return {Error::foreign_sound, nullptr};
    if (config.synthesis_rate == 0 || config.synthesis_rate > 96000)
        return {Error::invalid_parameter, nullptr};
    const auto period = rounded_frames(16000, config.synthesis_rate);
    if (period == 0)
        return {Error::invalid_parameter, nullptr};
    const auto& pan = coefficients.pan;
    if (pan.front() != 0x7fff || pan.back() != 0 ||
        std::any_of(pan.begin(), pan.end(), [](auto value) { return value > 0x7fff; }) ||
        std::any_of(coefficients.logarithm.begin(), coefficients.logarithm.end(),
                    [](double value) { return !std::isfinite(value) || value < -1.0 || value > 0.0; }))
        return {Error::invalid_parameter, nullptr};
    const Wave& wave = *selected.wave;
    if (wave.type != WaveType::adpcm) return {Error::unsupported_wave, nullptr};
    if (wave.book == nullptr || wave.book->order != 2 || wave.book->predictors <= 0 || wave.book->predictors > 8 ||
        wave.book->coefficients.size() != static_cast<std::size_t>(wave.book->predictors) * 16u)
        return {Error::unsupported_book, nullptr};
    if (wave.encoded.empty() || (wave.encoded.size() % 9u != 0 && wave.encoded.size() % 9u != 1) ||
        wave.encoded.size() / 9u == 0 || wave.encoded.size() > rdram_bytes - sample_address - 8u)
        return {Error::invalid_resource, nullptr};
    const auto samples = static_cast<std::uint32_t>((wave.encoded.size() / 9u) * 16u);
    for (std::size_t i = 0; i < wave.encoded.size() / 9u; ++i)
        if ((std::to_integer<std::uint8_t>(wave.encoded[i * 9u]) & 15u) >= static_cast<std::uint32_t>(wave.book->predictors))
            return {Error::invalid_resource, nullptr};
    if (wave.loop != nullptr && wave.loop->count != 0 &&
        (!wave.loop->adpcm_history || wave.loop->start >= wave.loop->end || wave.loop->end > samples))
        return {Error::unsupported_loop, nullptr};
    const Envelope& envelope = *selected.envelope;
    if (envelope.attack_us < 0 || envelope.decay_us != -1 || envelope.release_us < 0 ||
        envelope.attack_volume > 127 || envelope.decay_volume > 127 || selected.volume > 127 || selected.pan > 127)
        return {Error::unsupported_envelope, nullptr};
    try {
        auto implementation = std::make_unique<Impl>(std::move(owner), selected, config, coefficients);
        implementation->sample_frames = samples;
        implementation->book_bytes = static_cast<std::uint32_t>(wave.book->coefficients.size() * 2u);
        for (std::size_t i = 0; i < wave.encoded.size(); ++i)
            implementation->put8(sample_address + static_cast<std::uint32_t>(i), std::to_integer<std::uint8_t>(wave.encoded[i]));
        for (std::size_t i = 0; i < wave.book->coefficients.size(); ++i)
            implementation->put16(book_address + static_cast<std::uint32_t>(i * 2u),
                                  static_cast<std::uint16_t>(wave.book->coefficients[i]));
        if (wave.loop != nullptr && wave.loop->count != 0)
            for (std::size_t i = 0; i < wave.loop->history.size(); ++i)
                implementation->put16(loop_state_address + static_cast<std::uint32_t>(i * 2u),
                                      static_cast<std::uint16_t>(wave.loop->history[i]));
        return {Error::none, std::unique_ptr<Producer>(new Producer(std::move(implementation)))};
    } catch (const std::bad_alloc&) {
        return {Error::allocation_failure, nullptr};
    }
}

std::span<std::uint32_t> Producer::rdram_words() noexcept { return {impl_->memory.get(), rdram_bytes / 4u}; }
std::span<const std::uint32_t> Producer::rdram_words() const noexcept { return {impl_->memory.get(), rdram_bytes / 4u}; }

Error Producer::stage_start(Parameters parameters, std::uint64_t& generation) noexcept {
    generation = 0;
    auto& s = *impl_;
    if (s.poisoned) return Error::needs_reset;
    if (!valid_parameters(parameters)) return Error::invalid_parameter;
    if (s.control.active || s.reserved_generation != 0) return Error::voice_busy;
    if (s.event_count == s.events.size()) return Error::event_capacity;
    if (s.next_generation == std::numeric_limits<std::uint64_t>::max()) return Error::identity_exhausted;
    generation = ++s.next_generation;
    s.reserved_generation = generation;
    s.events[s.event_count++] = {Impl::EventKind::start, generation, parameters};
    return Error::none;
}
Error Producer::stage_pitch(std::uint64_t generation, float pitch) noexcept {
    std::uint16_t unused = 0;
    if (!pitch_word(pitch, unused)) return Error::invalid_parameter;
    Impl::Event event{Impl::EventKind::pitch, generation, {}};
    event.parameters.pitch = pitch;
    return impl_->post(event);
}
Error Producer::stage_pan(std::uint64_t generation, std::uint8_t pan) noexcept {
    if (pan > 127) return Error::invalid_parameter;
    Impl::Event event{Impl::EventKind::pan, generation, {}};
    event.parameters.pan = pan;
    return impl_->post(event);
}
Error Producer::stage_volume(std::uint64_t generation, std::uint16_t volume) noexcept {
    if (volume > 0x7fff) return Error::invalid_parameter;
    Impl::Event event{Impl::EventKind::volume, generation, {}};
    event.parameters.volume = volume;
    return impl_->post(event);
}
Error Producer::stage_stop(std::uint64_t generation) noexcept {
    return impl_->post({Impl::EventKind::stop, generation, {}});
}
Error Producer::stage() noexcept {
    auto& s = *impl_;
    if (s.poisoned) return Error::needs_reset;
    for (std::size_t i = 0; i < s.event_count; ++i) {
        const auto& event = s.events[i];
        switch (event.kind) {
        case Impl::EventKind::start:
            s.control = {true, false, event.generation, event.parameters};
            s.reserved_generation = 0;
            break;
        case Impl::EventKind::pitch: s.control.parameters.pitch = event.parameters.pitch; break;
        case Impl::EventKind::pan: s.control.parameters.pan = event.parameters.pan; break;
        case Impl::EventKind::volume: s.control.parameters.volume = event.parameters.volume; break;
        case Impl::EventKind::stop: s.control.stop = true; break;
        }
    }
    s.event_count = 0;
    return Error::none;
}
Error Producer::build(Service service, TaskView& task) noexcept {
    task = {};
    auto& s = *impl_;
    if (s.poisoned) return Error::needs_reset;
    if (s.lease != Impl::Lease::none) return Error::task_busy;
    if (service.service_id <= s.last_service) return Error::invalid_service;
    std::uint32_t frames = 0;
    const Error quota_error = service_quota(s.config, service, frames);
    if (quota_error != Error::none) return quota_error;
    if (s.next_task == std::numeric_limits<std::uint64_t>::max() ||
        s.sample_clock > std::numeric_limits<std::uint64_t>::max() - frames - s.max_client_step)
        return Error::identity_exhausted;
    auto next = s.voice;
    auto clock = s.sample_clock;
    auto client = s.next_client_sample;
    auto client_us = s.next_client_us;
    auto poll_us = s.next_poll_us;
    const auto end = clock + frames;
    s.write_address = command_address;
    s.build_error = Error::none;
    s.command(0x07000000u, 0);
    bool prepared = false;
    std::uint32_t offset = 0;
    while (offset < frames && s.build_error == Error::none) {
        // Callbacks have unaligned raw deadlines. Parameter events produced by
        // each callback affect the enclosing 16-frame group, before it renders.
        while (client < end && (client & ~std::uint64_t{15}) == clock && s.build_error == Error::none) {
            s.callback(next, client_us, poll_us);
            if (s.build_error != Error::none) break;
            const auto due_us = next.event_us != 0 ? std::min(poll_us, next.event_us) : poll_us;
            const auto delta_us = static_cast<std::int32_t>(due_us - client_us);
            client += rounded_frames(delta_us, s.config.synthesis_rate);
            client_us = due_us;
        }
        if (s.build_error != Error::none) break;
        const auto count = static_cast<std::uint32_t>(
            std::min<std::uint64_t>(frames - offset, (client & ~std::uint64_t{15}) - clock));
        s.render(next, count, offset, prepared);
        clock += count;
        offset += count;
    }
    if (s.build_error != Error::none) return s.build_error;
    // The task's last short outer pull also retires its measured repeats.
    s.consume_loops(next);
    s.task = {command_address, s.write_address - command_address, pcm_address, frames,
              s.config.synthesis_rate, ++s.next_task, next.generation, service.service_id};
    s.pending_voice = next;
    s.sample_clock = clock;
    s.next_client_sample = client;
    s.next_client_us = client_us;
    s.next_poll_us = poll_us;
    s.last_service = service.service_id;
    s.lease = Impl::Lease::executing;
    task = s.task;
    return Error::none;
}
Error Producer::complete(std::uint64_t task_id, std::uint64_t generation, bool success, PcmView& pcm) noexcept {
    pcm = {};
    auto& s = *impl_;
    if (s.lease != Impl::Lease::executing || task_id != s.task.task_id || generation != s.task.generation)
        return Error::stale_task;
    if (!success) {
        s.lease = Impl::Lease::none;
        s.poisoned = true;
        return Error::execution_failed;
    }
    s.voice = s.pending_voice;
    if (!s.voice.active && s.control.generation == s.task.generation) {
        s.control.active = false;
        s.control.stop = false;
    }
    s.lease = Impl::Lease::pcm;
    pcm = {s.task.pcm_address, s.task.frame_count, s.task.synthesis_rate, task_id, generation};
    return Error::none;
}
Error Producer::release(std::uint64_t task_id, std::uint64_t generation) noexcept {
    auto& s = *impl_;
    if (s.lease != Impl::Lease::pcm || task_id != s.task.task_id || generation != s.task.generation)
        return Error::stale_task;
    s.lease = Impl::Lease::none;
    return Error::none;
}
Error Producer::reset() noexcept {
    auto& s = *impl_;
    if (s.lease != Impl::Lease::none) return Error::task_busy;
    s.event_count = 0;
    s.control = {};
    s.voice = {};
    s.pending_voice = {};
    s.reserved_generation = 0;
    s.last_service = 0;
    s.sample_clock = s.next_client_sample = 0;
    s.next_client_us = s.next_poll_us = 0;
    s.poisoned = false;
    // Resource/main image bytes are never recopied or erased by player reset.
    for (std::uint32_t i = adpcm_state_address; i < loop_state_address; ++i) s.put8(i, 0);
    return Error::none;
}
bool Producer::voice_active() const noexcept { return impl_->control.active || impl_->reserved_generation != 0; }
bool Producer::has_lease() const noexcept { return impl_->lease != Impl::Lease::none; }
Error Producer::read_pcm(const PcmView& pcm, std::uint32_t frame, std::int16_t& left, std::int16_t& right) const noexcept {
    const auto& s = *impl_;
    if (!s.owns_pcm(pcm)) return Error::stale_task;
    if (frame >= pcm.frame_count) return Error::invalid_parameter;
    const auto samples = s.memory[pcm.address / 4u + frame];
    left = std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(samples >> 16));
    right = std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(samples));
    return Error::none;
}
Error Producer::copy_pcm(const PcmView& pcm, std::span<std::int16_t> destination) const noexcept {
    const auto& s = *impl_;
    if (!s.owns_pcm(pcm)) return Error::stale_task;
    if (destination.size() < static_cast<std::size_t>(pcm.frame_count) * 2u) return Error::invalid_parameter;
    for (std::uint32_t frame = 0; frame < pcm.frame_count; ++frame) {
        const auto samples = s.memory[pcm.address / 4u + frame];
        destination[frame * 2u] = std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(samples >> 16));
        destination[frame * 2u + 1u] = std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(samples));
    }
    return Error::none;
}

} // namespace isle::audio::sfx
