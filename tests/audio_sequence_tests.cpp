#include "35mmisle/audio_sequence.hpp"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>

namespace a = isle::audio;
namespace {
int failures = 0;
void check(bool condition, std::string_view meaning) {
    if (!condition) { ++failures; std::cerr << "FAIL: " << meaning << '\n'; }
}
void put16(a::Bytes& bytes, std::size_t offset, std::uint16_t value) {
    bytes[offset] = std::byte(value >> 8); bytes[offset + 1] = std::byte(value & 255);
}
void put32(a::Bytes& bytes, std::size_t offset, std::uint32_t value) {
    put16(bytes, offset, static_cast<std::uint16_t>(value >> 16));
    put16(bytes, offset + 2, static_cast<std::uint16_t>(value));
}
void row(a::Bytes& bytes, std::size_t index, std::uint32_t offset, std::uint32_t length) {
    put32(bytes, 4 + index * 8, offset); put32(bytes, 8 + index * 8, length);
}
struct Input {
    std::shared_ptr<a::Bytes> bytes = std::make_shared<a::Bytes>(52, std::byte{0x7d});
    Input() {
        put16(*bytes, 0, 0x5331); put16(*bytes, 2, 4);
        row(*bytes, 0, 41, 3); row(*bytes, 1, 41, 3);
        row(*bytes, 2, 42, 4); row(*bytes, 3, 50, 2);
        (*bytes)[41] = std::byte{0x11}; (*bytes)[43] = std::byte{0x33};
        (*bytes)[44] = std::byte{0xee}; (*bytes)[51] = std::byte{0x99};
    }
    a::SequenceResult decode() const { return a::hydrate_sequences(bytes); }
};
void rejection(const Input& input, a::SequenceError error, std::uint32_t offset,
               std::string_view meaning) {
    const auto result = input.decode();
    check(!result && result.error == error && result.offset == offset &&
          !result.asset && result.asset.sequences().empty(), meaning);
    check(input.bytes.use_count() == 1, "failed hydration retains no input owner");
}
void ownership_aliases_and_extents() {
    Input input;
    auto result = input.decode();
    check(bool(result), "bounded nonpacked catalog admitted");
    if (!result) return;
    auto& asset = result.asset;
    const auto entries = asset.sequences();
    check(entries.size() == 4, "BE16 count determines descriptor count");
    if (entries.size() != 4) return;
    check(entries[0].offset == 41 && entries[0].encoded.data() == input.bytes->data() + 41 &&
          entries[0].encoded.data() == entries[1].encoded.data() &&
          entries[0].dma_bytes.data() == entries[1].dma_bytes.data(),
          "duplicate payload rows retain their original zero-copy alias");
    check(entries[2].encoded.data() == entries[0].encoded.data() + 1 &&
          entries[2].encoded.size() == 4 && entries[2].dma_bytes.size() == 4,
          "bounded partially overlapping payloads are not rejected or copied");
    check(entries[0].encoded.size() == 3 && entries[0].dma_bytes.size() == 4 &&
          entries[0].encoded.back() == std::byte{0x33} &&
          entries[0].dma_bytes.back() == std::byte{0xee} &&
          entries[0].encoded.data() == entries[0].dma_bytes.data(),
          "declared odd length excludes the retained even-DMA padding byte");
    check(entries[3].offset == 50 && entries[3].encoded.size() == 2 &&
          entries[3].dma_bytes.size() == 2 && entries[3].encoded.back() == std::byte{0x99},
          "gaps do not extend payloads and even length needs no padding");
    check((*input.bytes)[11] == std::byte{3}, "hydration leaves the raw declared length unchanged");
    std::weak_ptr<const a::Bytes> owner = input.bytes;
    input.bytes.reset();
    check(!owner.expired() && asset.sequences().data() == entries.data() &&
          entries[0].encoded.front() == std::byte{0x11} &&
          entries[3].encoded.back() == std::byte{0x99},
          "catalog retains descriptors and payloads after the caller releases its input");
    asset = {};
    check(owner.expired(), "last catalog releases its retained owner");
}
void malformed_boundaries_and_atomic_failure() {
    const auto missing = a::hydrate_sequences({});
    check(missing.error == a::SequenceError::invalid_owner && !missing.asset,
          "null input owner cannot publish an asset");
    { Input i; i.bytes->resize(3);
      rejection(i, a::SequenceError::truncated, 0, "incomplete header rejected"); }
    { Input i; put16(*i.bytes, 0, 0x4231);
      rejection(i, a::SequenceError::revision, 0, "other resource magic rejected"); }
    { Input i; i.bytes->resize(35);
      rejection(i, a::SequenceError::truncated, 4, "last incomplete table row rejected before payload access"); }
    { Input i; put16(*i.bytes, 2, 0xffff);
      rejection(i, a::SequenceError::truncated, 4, "large unsigned count cannot escape table bounds"); }
    { Input i; row(*i.bytes, 0, 35, 1);
      rejection(i, a::SequenceError::invalid_offset, 35, "payload cannot alias the descriptor table"); }
    { Input i; row(*i.bytes, 0, 0, 0);
      rejection(i, a::SequenceError::invalid_offset, 0, "zero length does not exempt an out-of-payload offset"); }
    { Input i; row(*i.bytes, 0, 53, 0);
      rejection(i, a::SequenceError::invalid_offset, 53, "empty entry still needs a bounded offset"); }
    { Input i; row(*i.bytes, 0, 0xfffffff0, 32);
      rejection(i, a::SequenceError::invalid_offset, 0xfffffff0, "32-bit offset plus length cannot wrap into owner"); }
    { Input i; row(*i.bytes, 0, 41, std::numeric_limits<std::uint32_t>::max());
      rejection(i, a::SequenceError::invalid_offset, 41, "maximum odd length cannot wrap when rounded for DMA"); }
    { Input i; row(*i.bytes, 0, 50, 4);
      rejection(i, a::SequenceError::invalid_offset, 50, "declared payload must fit owner"); }
    { Input i; row(*i.bytes, 3, 50, 1); i.bytes->resize(51);
      rejection(i, a::SequenceError::invalid_offset, 50,
                "late row with present raw byte but missing DMA padding publishes no earlier descriptors"); }
}
void empty_and_unsigned_count() {
    auto bytes = std::make_shared<a::Bytes>(4);
    put16(*bytes, 0, 0x5331); put16(*bytes, 2, 0);
    {
        const auto result = a::hydrate_sequences(bytes);
        check(result && result.asset.sequences().empty(), "zero-entry catalog is a valid retained resource");
    }
    bytes->resize(24);
    put16(*bytes, 2, 2); row(*bytes, 0, 20, 0); row(*bytes, 1, 24, 0);
    {
        const auto result = a::hydrate_sequences(bytes);
        check(result && result.asset.sequences().size() == 2, "bounded empty payloads admitted");
        if (result && result.asset.sequences().size() == 2) {
            const auto entries = result.asset.sequences();
            check(entries[0].encoded.empty() && entries[0].dma_bytes.empty() &&
                  entries[0].encoded.data() == bytes->data() + 20 &&
                  entries[1].encoded.empty() && entries[1].dma_bytes.empty() &&
                  entries[1].encoded.data() == bytes->data() + bytes->size(),
                  "empty payloads preserve table-end and owner-end offsets without padding");
        }
    }
    constexpr std::uint16_t count = 0x8000;
    constexpr std::uint32_t table_end = 4 + std::uint32_t(count) * 8;
    bytes->resize(table_end);
    put16(*bytes, 2, count);
    for (std::size_t i = 0; i < count; ++i) row(*bytes, i, table_end, 0);
    const auto result = a::hydrate_sequences(bytes);
    check(result && result.asset.sequences().size() == count &&
          result.asset.sequences().back().offset == table_end &&
          result.asset.sequences().back().encoded.empty(),
          "high count bit is unsigned count data, not a negative count or a 32-bit header");
}
} // namespace
int main() {
    ownership_aliases_and_extents(); malformed_boundaries_and_atomic_failure();
    empty_and_unsigned_count();
    if (failures) return EXIT_FAILURE;
    std::cout << "Audio sequence boundary, alias and ownership checks passed\n";
    return EXIT_SUCCESS;
}
