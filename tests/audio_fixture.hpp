#ifndef ISLE_TEST_AUDIO_FIXTURE_HPP
#define ISLE_TEST_AUDIO_FIXTURE_HPP
#include "35mmisle/audio_synth.hpp"
#include <cmath>
namespace isle::audio_test {
inline void put16(audio::Bytes& bytes, std::size_t offset, std::uint16_t value) {
    bytes[offset] = std::byte(value >> 8); bytes[offset + 1] = std::byte(value & 255);
}
inline void put32(audio::Bytes& bytes, std::size_t offset, std::uint32_t value) {
    put16(bytes, offset, static_cast<std::uint16_t>(value >> 16));
    put16(bytes, offset + 2, static_cast<std::uint16_t>(value));
}
// Authored B1 wire graph, no original payload or SDK implementation.
struct Fixture {
    std::shared_ptr<audio::Bytes> bank = std::make_shared<audio::Bytes>(208);
    std::shared_ptr<audio::Bytes> table = std::make_shared<audio::Bytes>(18);
    Fixture() {
        auto& b = *bank;
        put16(b, 0, 0x4231); put16(b, 2, 1); put32(b, 4, 16);
        put16(b, 16, 1); put32(b, 20, 32000); put32(b, 28, 32);
        b[32] = std::byte{100}; b[33] = std::byte{64};
        put16(b, 44, 200); put16(b, 46, 1); put32(b, 48, 64);
        put32(b, 64, 80); put32(b, 68, 96); put32(b, 72, 104);
        b[76] = std::byte{64}; b[77] = std::byte{75};
        put32(b, 80, 0); put32(b, 84, 0xffffffff); put32(b, 88, 6000);
        b[92] = std::byte{127}; b[93] = std::byte{127};
        b[97] = std::byte{127}; b[99] = std::byte{127}; b[100] = std::byte{60};
        put32(b, 104, 0); put32(b, 108, 18);
        put32(b, 116, 124); put32(b, 120, 168);
        put32(b, 124, 0); put32(b, 128, 32); put32(b, 132, 0xffffffff);
        put16(b, 136, 0x8000); put16(b, 166, 0x7fff);
        put32(b, 168, 2); put32(b, 172, 1);
        put16(b, 176, 0x8000); put16(b, 206, 0x7fff);
    }
    audio::BankResult decode() const { return audio::hydrate_bank(bank, table); }
};
inline const audio::Sound& selected(const audio::BankFile& bank) {
    return *bank.banks()[0]->instruments[0]->sounds[0];
}
inline audio::Coefficients coefficients() {
    audio::Coefficients values{};
    for (std::uint32_t i = 0; i < values.pan.size(); ++i)
        values.pan[i] = static_cast<std::uint16_t>(((127u - i) * 32767u) / 127u);
    for (std::size_t i = 0; i < values.logarithm.size(); ++i)
        values.logarithm[i] = std::log2((8.5 + static_cast<double>(i)) / 16.0);
    return values;
}
} // namespace isle::audio_test
#endif
