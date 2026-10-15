// Original native decoder of public B1 wire-layout facts. No SDK implementation.
#include "35mmisle/audio_bank.hpp"

#include <bit>
#include <limits>
#include <map>
#include <new>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace isle::audio {
namespace {
struct Failure { BankError error; std::uint32_t offset; };
[[noreturn]] void fail(BankError error, std::uint32_t offset) { throw Failure{error, offset}; }
std::uint16_t u16(const std::byte* p) noexcept {
    return static_cast<std::uint16_t>((std::to_integer<unsigned>(p[0]) << 8) |
                                      std::to_integer<unsigned>(p[1]));
}
std::int16_t i16(const std::byte* p) noexcept { return std::bit_cast<std::int16_t>(u16(p)); }
std::uint32_t u32(const std::byte* p) noexcept {
    return (std::uint32_t(u16(p)) << 16) | u16(p + 2);
}
std::int32_t i32(const std::byte* p) noexcept { return std::bit_cast<std::int32_t>(u32(p)); }
std::uint8_t u8(const std::byte* p) noexcept { return std::to_integer<std::uint8_t>(*p); }
enum class Kind { file, bank, instrument, sound, envelope, keymap, wave, book, adpcm_loop, raw_loop };
template<class T> struct Nodes {
    std::vector<std::unique_ptr<T>> owned;
    std::unordered_map<std::uint32_t, T*> offsets;
    const T* find(std::uint32_t offset) const {
        const auto it = offsets.find(offset);
        return it == offsets.end() ? nullptr : it->second;
    }
    T& add(std::uint32_t offset) {
        auto value = std::make_unique<T>();
        auto* result = value.get();
        owned.push_back(std::move(value));
        offsets.emplace(offset, result);
        return *result;
    }
};
} // namespace

struct BankFile::Storage {
    ByteOwner bytes, table;
    std::vector<const Bank*> banks;
    Nodes<Bank> bank_nodes;
    Nodes<Instrument> instruments;
    Nodes<Sound> sounds;
    Nodes<Envelope> envelopes;
    Nodes<Keymap> keymaps;
    Nodes<Wave> waves;
    Nodes<Book> books;
    Nodes<Loop> loops;
    struct Record { std::size_t end; Kind kind; };
    std::map<std::uint32_t, Record> records;

    const std::byte* range(std::uint32_t offset, std::size_t size) const {
        if (offset > bytes->size() || size > bytes->size() - offset)
            fail(BankError::invalid_offset, offset);
        return bytes->data() + offset;
    }
    const std::byte* record(std::uint32_t offset, std::size_t size, Kind kind) {
        const auto* p = range(offset, size);
        auto next = records.lower_bound(offset);
        const auto end = static_cast<std::size_t>(offset) + size;
        if (next != records.end() && next->first == offset) {
            if (next->second.kind != kind || next->second.end != end)
                fail(BankError::conflicting_record, offset);
            return p;
        }
        if ((next != records.end() && next->first < end) ||
            (next != records.begin() && std::prev(next)->second.end > offset))
            fail(BankError::conflicting_record, offset);
        records.emplace_hint(next, offset, Record{end, kind});
        return p;
    }
    void flags(const std::byte* p, std::uint32_t offset) const {
        if (u8(p) != 0) fail(BankError::relocated_input, offset);
    }
    std::size_t count(const std::byte* p, std::uint32_t offset) const {
        const auto value = i16(p);
        if (value < 0) fail(BankError::invalid_count, offset);
        return static_cast<std::size_t>(value);
    }
    const Envelope* envelope(std::uint32_t offset) {
        if (offset == 0) fail(BankError::invalid_offset, offset);
        if (const auto* old = envelopes.find(offset)) return old;
        const auto* p = record(offset, 16, Kind::envelope);
        const auto attack = i32(p), decay = i32(p + 4), release = i32(p + 8);
        if (attack < 0 || decay < -1 || release < 0 || u8(p + 12) > 127 || u8(p + 13) > 127)
            fail(BankError::invalid_scalar, offset);
        auto& value = envelopes.add(offset);
        value = {attack, decay, release, u8(p + 12), u8(p + 13)};
        return &value;
    }
    const Keymap* keymap(std::uint32_t offset) {
        if (offset == 0) fail(BankError::invalid_offset, offset);
        if (const auto* old = keymaps.find(offset)) return old;
        const auto* p = record(offset, 6, Kind::keymap);
        if (u8(p) > u8(p + 1) || u8(p + 1) > 127 || u8(p + 2) > u8(p + 3) ||
            u8(p + 3) > 127 || u8(p + 4) > 127) fail(BankError::invalid_scalar, offset);
        auto& value = keymaps.add(offset);
        value = {u8(p), u8(p + 1), u8(p + 2), u8(p + 3), u8(p + 4),
                 std::bit_cast<std::int8_t>(u8(p + 5))};
        return &value;
    }
    const Book* book(std::uint32_t offset) {
        if (offset == 0) fail(BankError::invalid_book, offset);
        if (const auto* old = books.find(offset)) return old;
        const auto* p = range(offset, 8);
        const auto order = i32(p), predictors = i32(p + 4);
        if (order <= 0 || predictors <= 0) fail(BankError::invalid_book, offset);
        const auto a = static_cast<std::size_t>(order), b = static_cast<std::size_t>(predictors);
        if (a > (std::numeric_limits<std::size_t>::max() - 8) / 16 / b)
            fail(BankError::invalid_book, offset);
        const auto coefficients = a * b * 8;
        p = record(offset, 8 + coefficients * 2, Kind::book);
        auto& value = books.add(offset);
        value.order = order; value.predictors = predictors;
        value.coefficients.resize(coefficients);
        for (std::size_t i = 0; i < coefficients; ++i) value.coefficients[i] = i16(p + 8 + i * 2);
        return &value;
    }
    const Loop* loop(std::uint32_t offset, WaveType type, std::size_t sample_count) {
        if (offset == 0) return nullptr;
        const bool adpcm = type == WaveType::adpcm;
        const auto* p = record(offset, adpcm ? 44 : 12, adpcm ? Kind::adpcm_loop : Kind::raw_loop);
        if (u32(p) > u32(p + 4) || u32(p + 4) > sample_count)
            fail(BankError::invalid_loop, offset);
        if (u32(p + 8) != 0 && u32(p) == u32(p + 4)) fail(BankError::invalid_loop, offset);
        if (const auto* old = loops.find(offset)) return old;
        auto& value = loops.add(offset);
        value.start = u32(p); value.end = u32(p + 4); value.count = u32(p + 8);
        value.adpcm_history = adpcm;
        if (adpcm) for (std::size_t i = 0; i < value.history.size(); ++i) value.history[i] = i16(p + 12 + i * 2);
        return &value;
    }
    const Wave* wave(std::uint32_t offset) {
        if (offset == 0) fail(BankError::invalid_offset, offset);
        if (const auto* old = waves.find(offset)) return old;
        const auto* p = record(offset, 20, Kind::wave);
        flags(p + 9, offset + 9);
        if (u8(p + 8) > 1) fail(BankError::unsupported_wave, offset);
        const auto type = static_cast<WaveType>(u8(p + 8));
        const auto begin = u32(p);
        const auto length = i32(p + 4);
        if (length <= 0 || begin > table->size() || static_cast<std::size_t>(length) > table->size() - begin)
            fail(BankError::invalid_sample, offset);
        const auto size = static_cast<std::size_t>(length);
        if ((type == WaveType::adpcm && (size < 9 || size % 9 > 1)) ||
            (type == WaveType::raw16 && size % 2 != 0)) fail(BankError::invalid_sample, offset);
        const auto samples = type == WaveType::adpcm ? (size / 9) * 16 : size / 2;
        const auto* loop_value = loop(u32(p + 12), type, samples);
        const auto* book_value = type == WaveType::adpcm ? book(u32(p + 16)) : nullptr;
        auto& value = waves.add(offset);
        value = {type, begin, std::span<const std::byte>(*table).subspan(begin, size), loop_value, book_value};
        return &value;
    }
    const Sound* sound(std::uint32_t offset) {
        if (offset == 0) return nullptr;
        if (const auto* old = sounds.find(offset)) return old;
        const auto* p = record(offset, 16, Kind::sound);
        flags(p + 14, offset + 14);
        if (u8(p + 12) > 127 || u8(p + 13) > 127) fail(BankError::invalid_scalar, offset);
        const auto* e = envelope(u32(p)); const auto* k = keymap(u32(p + 4)); const auto* w = wave(u32(p + 8));
        auto& value = sounds.add(offset);
        value = {e, k, w, u8(p + 12), u8(p + 13)};
        return &value;
    }
    const Instrument* instrument(std::uint32_t offset) {
        if (offset == 0) return nullptr;
        if (const auto* old = instruments.find(offset)) return old;
        const auto* p = range(offset, 16);
        const auto length = count(p + 14, offset + 14);
        p = record(offset, 16 + length * 4, Kind::instrument);
        flags(p + 3, offset + 3);
        if (u8(p) > 127 || u8(p + 1) > 127) fail(BankError::invalid_scalar, offset);
        auto& value = instruments.add(offset);
        value.volume = u8(p); value.pan = u8(p + 1); value.priority = u8(p + 2);
        value.tremolo_type = u8(p + 4); value.tremolo_rate = u8(p + 5);
        value.tremolo_depth = u8(p + 6); value.tremolo_delay = u8(p + 7);
        value.vibrato_type = u8(p + 8); value.vibrato_rate = u8(p + 9);
        value.vibrato_depth = u8(p + 10); value.vibrato_delay = u8(p + 11);
        value.bend_range = i16(p + 12);
        value.sounds.reserve(length);
        for (std::size_t i = 0; i < length; ++i) value.sounds.push_back(sound(u32(p + 16 + i * 4)));
        return &value;
    }
    const Bank* bank(std::uint32_t offset) {
        if (offset == 0) return nullptr;
        if (const auto* old = bank_nodes.find(offset)) return old;
        const auto* p = range(offset, 12);
        const auto length = count(p, offset);
        p = record(offset, 12 + length * 4, Kind::bank);
        flags(p + 2, offset + 2);
        if (i32(p + 4) <= 0) fail(BankError::invalid_scalar, offset + 4);
        auto& value = bank_nodes.add(offset);
        value.sample_rate = i32(p + 4);
        value.percussion = instrument(u32(p + 8));
        value.instruments.reserve(length);
        for (std::size_t i = 0; i < length; ++i) value.instruments.push_back(instrument(u32(p + 12 + i * 4)));
        return &value;
    }
    void decode() {
        if (bytes->size() < 4) fail(BankError::truncated, 0);
        const auto* header = range(0, 4);
        if (u16(header) != 0x4231) fail(BankError::revision, 0);
        const auto length = count(header + 2, 2);
        header = record(0, 4 + length * 4, Kind::file);
        banks.reserve(length);
        for (std::size_t i = 0; i < length; ++i) banks.push_back(bank(u32(header + 4 + i * 4)));
    }
};

std::span<const Bank* const> BankFile::banks() const noexcept {
    return storage_ ? std::span<const Bank* const>(storage_->banks) : std::span<const Bank* const>{};
}
BankResult hydrate_bank(ByteOwner bank, ByteOwner table) noexcept {
    if (!bank || !table) return {BankError::invalid_owner, 0, {}};
    try {
        auto storage = std::make_shared<BankFile::Storage>();
        storage->bytes = std::move(bank); storage->table = std::move(table);
        storage->decode();
        BankFile asset; asset.storage_ = std::move(storage);
        return {BankError::none, 0, std::move(asset)};
    } catch (const Failure& failure) {
        return {failure.error, failure.offset, {}};
    } catch (const std::bad_alloc&) {
        return {BankError::allocation_failure, 0, {}};
    } catch (const std::length_error&) {
        return {BankError::allocation_failure, 0, {}};
    }
}
} // namespace isle::audio
