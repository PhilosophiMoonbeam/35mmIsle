#include "35mmisle/audio_bank.hpp"
#include <algorithm>
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
struct Input {
    std::shared_ptr<a::Bytes> bank = std::make_shared<a::Bytes>(208);
    std::shared_ptr<a::Bytes> table = std::make_shared<a::Bytes>(18, std::byte{0x31});
    Input() {
        auto& b = *bank;
        put16(b, 0, 0x4231); put16(b, 2, 1); put32(b, 4, 16);
        put16(b, 16, 1); put32(b, 20, 32000); put32(b, 28, 32);
        b[32] = std::byte{100}; b[33] = std::byte{64};
        put16(b, 44, 200); put16(b, 46, 2);
        put32(b, 48, 64); put32(b, 52, 64);
        put32(b, 64, 80); put32(b, 68, 96); put32(b, 72, 104);
        b[76] = std::byte{64}; b[77] = std::byte{75};
        put32(b, 80, 1000); put32(b, 84, 0xffffffff); put32(b, 88, 6000);
        b[92] = std::byte{127}; b[93] = std::byte{100};
        b[97] = std::byte{127}; b[99] = std::byte{127}; b[100] = std::byte{60};
        b[101] = std::byte{0xfe}; // signed detune -2
        put32(b, 104, 0); put32(b, 108, 18);
        put32(b, 116, 124); put32(b, 120, 168);
        put32(b, 124, 3); put32(b, 128, 31); put32(b, 132, 0xffffffff);
        put16(b, 136, 0x8000); put16(b, 166, 0x7fff);
        put32(b, 168, 2); put32(b, 172, 1);
        put16(b, 176, 0x8000); put16(b, 206, 0x7fff);
    }
    a::BankResult decode() const { return a::hydrate_bank(bank, table); }
};
void rejection(const Input& input, a::BankError expected, std::string_view meaning) {
    auto result = input.decode();
    check(result.error == expected && !result.asset && result.asset.banks().empty(), meaning);
}
void ownership_and_aliases() {
    Input input;
    auto decoded = input.decode();
    check(bool(decoded), "closed valid graph admitted");
    if (!decoded) return;
    auto asset = std::move(decoded.asset);
    const auto* inst = asset.banks()[0]->instruments[0];
    const auto* sound = inst->sounds[0];
    check(sound == inst->sounds[1], "one serialized sound has one native identity");
    check(sound->keymap->detune == -2 && sound->envelope->decay_us == -1,
          "signed detune and indefinite decay survive wire conversion");
    check(sound->wave->loop->start == 3 && sound->wave->loop->end == 31 &&
          sound->wave->loop->count == 0xffffffff &&
          sound->wave->loop->history.front() == std::numeric_limits<std::int16_t>::min() &&
          sound->wave->loop->history.back() == std::numeric_limits<std::int16_t>::max(),
          "non-frame loop endpoints, unsigned repeat count and signed history preserved");
    check(sound->wave->book->coefficients.front() == std::numeric_limits<std::int16_t>::min() &&
          sound->wave->book->coefficients.back() == std::numeric_limits<std::int16_t>::max(),
          "complete signed predictor extent, including final coefficient");
    std::weak_ptr<const a::Bytes> bank_owner = input.bank, table_owner = input.table;
    input.bank.reset(); input.table.reset();
    auto moved = std::move(asset);
    check(!bank_owner.expired() && !table_owner.expired() &&
          sound->wave->encoded.front() == std::byte{0x31} &&
          sound->wave->encoded.back() == std::byte{0x31},
          "moved asset retains bare native view and encoded sample owners");
    moved = {};
    check(bank_owner.expired() && table_owner.expired(), "last asset releases both resource owners");
}
void malformed_boundaries() {
    { Input i; i.bank->pop_back(); rejection(i, a::BankError::invalid_offset, "missing final coefficient fails unpublished"); }
    { Input i; i.table->pop_back(); rejection(i, a::BankError::invalid_sample, "encoded sample cannot exceed table"); }
    { Input i; put32(*i.bank, 128, 33); rejection(i, a::BankError::invalid_loop, "loop cannot address a nonexistent decoded sample"); }
    { Input i; put32(*i.bank, 124, 31); rejection(i, a::BankError::invalid_loop, "nonzero repeat count needs nonempty loop"); }
    { Input i; put32(*i.bank, 168, 0x7fffffff); put32(*i.bank, 172, 0x7fffffff);
      auto r = i.decode(); check(!r && !r.asset, "oversized coefficient dimensions reject without allocation publication"); }
    { Input i; put32(*i.bank, 68, 80); rejection(i, a::BankError::conflicting_record, "one serialized record cannot impersonate a different type"); }
    { Input i; put16(*i.bank, 46, 0xffff); rejection(i, a::BankError::invalid_count, "signed count cannot become a huge native vector"); }
    { Input i; (*i.bank)[113] = std::byte{1}; rejection(i, a::BankError::relocated_input, "already-relocated wire pointers are never native pointers"); }
    { Input i; put32(*i.bank, 104, 0xffffffff); rejection(i, a::BankError::invalid_sample, "sample offset arithmetic cannot wrap"); }
    { Input i; put32(*i.bank, 120, 0); rejection(i, a::BankError::invalid_book, "ADPCM has a required predictor book"); }
}
void optional_slots_raw_and_padding() {
    { Input i; put32(*i.bank, 52, 0); put32(*i.bank, 116, 0);
      const auto r = i.decode(); check(r && r.asset.banks()[0]->instruments[0]->sounds[1] == nullptr &&
          r.asset.banks()[0]->instruments[0]->sounds[0]->wave->loop == nullptr,
          "optional sound slot and loop remain absent"); }
    { Input i; i.table->push_back(std::byte{0x00}); put32(*i.bank, 108, 19);
      const auto r = i.decode(); check(r && r.asset.banks()[0]->instruments[0]->sounds[0]->wave->encoded.size() == 19,
          "one ADPCM alignment byte is retained, not treated as a complete frame"); }
    { Input i; (*i.bank)[112] = std::byte{1}; put32(*i.bank, 128, 9);
      const auto r = i.decode(); check(r && r.asset.banks()[0]->instruments[0]->sounds[0]->wave->book == nullptr &&
          !r.asset.banks()[0]->instruments[0]->sounds[0]->wave->loop->adpcm_history,
          "RAW16 loop consumes twelve bytes and no ADPCM book/history"); }
    { Input i; (*i.bank)[112] = std::byte{1}; put32(*i.bank, 108, 17);
      rejection(i, a::BankError::invalid_sample, "RAW16 has complete two-byte samples"); }
}
} // namespace
int main() {
    ownership_and_aliases(); malformed_boundaries(); optional_slots_raw_and_padding();
    if (failures) return EXIT_FAILURE;
    std::cout << "Audio bank boundary, alias and ownership checks passed\n";
    return EXIT_SUCCESS;
}
