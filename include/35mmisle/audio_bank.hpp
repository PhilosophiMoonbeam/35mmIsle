#ifndef ISLE_AUDIO_BANK_HPP
#define ISLE_AUDIO_BANK_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace isle::audio {
using Bytes = std::vector<std::byte>;
using ByteOwner = std::shared_ptr<const Bytes>;

struct Envelope {
    std::int32_t attack_us, decay_us, release_us;
    std::uint8_t attack_volume, decay_volume;
};
struct Keymap {
    std::uint8_t velocity_min, velocity_max, key_min, key_max, base_key;
    std::int8_t detune;
};
struct Book {
    std::int32_t order, predictors;
    std::vector<std::int16_t> coefficients;
};
struct Loop {
    std::uint32_t start, end, count;
    std::array<std::int16_t, 16> history{};
    bool adpcm_history = false;
};
enum class WaveType : std::uint8_t { adpcm = 0, raw16 = 1 };
struct Wave {
    WaveType type;
    std::uint32_t table_offset;
    std::span<const std::byte> encoded;
    const Loop* loop;
    const Book* book;
};
struct Sound {
    const Envelope* envelope;
    const Keymap* keymap;
    const Wave* wave;
    std::uint8_t pan, volume;
};
struct Instrument {
    std::uint8_t volume, pan, priority;
    std::uint8_t tremolo_type, tremolo_rate, tremolo_depth, tremolo_delay;
    std::uint8_t vibrato_type, vibrato_rate, vibrato_depth, vibrato_delay;
    std::int16_t bend_range;
    std::vector<const Sound*> sounds;
};
struct Bank {
    std::int32_t sample_rate;
    const Instrument* percussion;
    std::vector<const Instrument*> instruments;
};

enum class BankError {
    none, invalid_owner, truncated, revision, invalid_count, invalid_offset,
    relocated_input, invalid_scalar, unsupported_wave, invalid_sample,
    invalid_book, invalid_loop, conflicting_record, allocation_failure
};
struct BankResult;
class BankFile final {
public:
    BankFile() noexcept = default;
    std::span<const Bank* const> banks() const noexcept;
    explicit operator bool() const noexcept { return storage_ != nullptr; }
private:
    struct Storage;
    std::shared_ptr<const Storage> storage_;
    friend BankResult hydrate_bank(ByteOwner, ByteOwner) noexcept;
};
struct BankResult {
    BankError error = BankError::none;
    std::uint32_t offset = 0;
    BankFile asset;
    explicit operator bool() const noexcept { return error == BankError::none && bool(asset); }
};
// Original BE B1 graph decoder. All bank references are bank-file-relative;
// sample offsets are table-relative. Raw wire flags must be unrelocated (zero).
// Graph aliases are retained, not copied. Optional null bank/instrument/sound
// slots remain null. Required sound envelope/keymap/wave/book cannot be null.
// Positive book dimensions must fit the complete bounded coefficient record;
// downstream synthesizers separately admit supported orders/predictor counts.
// ADPCM sample tails may include one alignment byte; non-frame-aligned loops
// and the full unsigned loop count are preserved. No DSP/player state is made.
// Owners retain both immutable inputs and all descriptors. Bare pointers/spans
// require a live BankFile copy through all consumer/mixer completion. Inputs
// must not be mutated via another alias. No pointer-shaped wire relocation.
BankResult hydrate_bank(ByteOwner bank, ByteOwner table) noexcept;
} // namespace isle::audio
#endif
