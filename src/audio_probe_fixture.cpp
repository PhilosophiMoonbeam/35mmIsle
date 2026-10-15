#include "audio_probe_fixture.hpp"

#include <algorithm>
#include <bit>
#include <span>
#include <stdexcept>
#include <utility>
#include <string>

namespace isle::audio_probe {
namespace {
std::uint16_t be16(std::span<const std::byte> bytes, std::size_t offset) noexcept {
    return static_cast<std::uint16_t>((std::to_integer<unsigned>(bytes[offset]) << 8) |
                                     std::to_integer<unsigned>(bytes[offset + 1]));
}
std::uint64_t be64(std::span<const std::byte> bytes, std::size_t offset) noexcept {
    std::uint64_t value = 0;
    for (unsigned i = 0; i != 8; ++i)
        value = (value << 8) | std::to_integer<unsigned>(bytes[offset + i]);
    return value;
}
bool reject_dp(void*, std::span<const std::uint32_t>) noexcept { return false; }
} // namespace

void require(audio::sfx::Error error, const char* operation) {
    if (error != audio::sfx::Error::none)
        throw std::runtime_error(std::string(operation) + " (native producer error=" +
                                 std::to_string(static_cast<unsigned>(error)) + ")");
}

Session::Session(const rom::Snapshot& snapshot, std::uint32_t sound_index) {
    if constexpr (std::endian::native != std::endian::little)
        throw std::runtime_error("the word-swapped RSP backend requires a little-endian host");
    identity_ = snapshot.identity();
    const auto bytes = snapshot.canonical();
    if (bytes.size() != rom::us_size)
        throw std::runtime_error("audio session requires a live admitted ROM snapshot");
    auto bank_bytes = std::make_shared<audio::Bytes>(bytes.begin() + 0xba6c20, bytes.begin() + 0xbb6940);
    auto table_bytes = std::make_shared<audio::Bytes>(bytes.begin() + 0xbb6940, bytes.begin() + 0xf53540);
    auto bank = audio::hydrate_bank(std::move(bank_bytes), std::move(table_bytes));
    if (!bank)
        throw std::runtime_error("typed SFX bank admission failed (bank error=" +
                                 std::to_string(static_cast<unsigned>(bank.error)) + ")");
    const auto banks = bank.asset.banks();
    if (banks.empty() || banks[0] == nullptr || banks[0]->instruments.empty() ||
        banks[0]->instruments[0] == nullptr)
        throw std::runtime_error("SFX bank 0 instrument 0 is unavailable");
    const auto& sounds = banks[0]->instruments[0]->sounds;
    if (sound_index >= sounds.size() || sounds[sound_index] == nullptr)
        throw std::runtime_error("--sound does not select a populated SFX bank 0 instrument 0 slot");
    audio::sfx::Coefficients coefficients;
    for (std::size_t i = 0; i != coefficients.pan.size(); ++i)
        coefficients.pan[i] = be16(bytes, 0x436b0 + i * 2);
    for (std::size_t i = 0; i != coefficients.logarithm.size(); ++i)
        coefficients.logarithm[i] = std::bit_cast<double>(be64(bytes, 0x437b0 + i * 8));
    auto made = audio::sfx::Producer::create(bank.asset, *sounds[sound_index], {synthesis_rate}, coefficients);
    require(made.error, "selected SFX resource admission failed");
    if (!made.producer) throw std::runtime_error("native producer admission returned no owner");
    producer_ = std::move(made.producer);
    auto* host = reinterpret_cast<std::uint8_t*>(producer_->rdram_words().data());
    // Original loaded main, including aspMain text/data and adjacent DMA bytes.
    // Interface facts only: no original producer body/header is imported.
    for (std::uint32_t address = 0x400; address != 0x45670; ++address)
        host[address ^ 3u] = std::to_integer<std::uint8_t>(bytes[address - 0x400 + 0x1000]);
    // Fresh producer allocation already zeros original BSS45670..9a8c0.
    std::copy_n(bytes.begin() + 0xb70, boot_.size(), boot_.begin());
    for (std::uint32_t i = 0; i != boot_.size(); ++i)
        host[(0x107000u + i) ^ 3u] = std::to_integer<std::uint8_t>(boot_[i]);
    machine_ = std::make_unique<rsp::Machine>(producer_->rdram_words(), reject_dp, nullptr);
}
Session::~Session() = default;

Completion Session::execute(audio::sfx::Service service) {
    Completion completion;
    require(producer_->build(service, completion.task), "native task build failed");
    const rsp::Task task{2, 0, 0x107000, 0x100, 0x3d980, 0x1000, 0x44ba0, 0x800,
                         0, 0, 0, 0, completion.task.command_address, completion.task.command_bytes, 0, 0};
    try {
        completion.execution = machine_->execute(task, boot_, {10000000, 0});
    } catch (...) {
        audio::sfx::PcmView discarded;
        producer_->complete(completion.task.task_id, completion.task.generation, false, discarded);
        throw;
    }
    const auto& result = completion.execution;
    const bool success = bool(result) && result.sp_status == 0x203 && result.packets == 0 &&
                         result.packet_words == 0 && result.full_syncs == 0;
    const auto acknowledged = producer_->complete(completion.task.task_id, completion.task.generation,
                                                  success, completion.pcm);
    if (!success)
        throw std::runtime_error("actual audio RSP terminal rejected (RSP error=" +
                                 std::to_string(static_cast<unsigned>(result.error)) +
                                 ", SP=" + std::to_string(result.sp_status) +
                                 ", DPC packets=" + std::to_string(result.packets) + ")");
    require(acknowledged, "actual completed PCM lease publication failed");
    return completion;
}
} // namespace isle::audio_probe
