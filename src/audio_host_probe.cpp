#include "audio_host_probe.hpp"

#include <SDL3/SDL.h>

#include <iostream>
#include <span>
#include <stdexcept>
#include <string>

namespace {
namespace probe = isle::audio_probe;
namespace sfx = isle::audio::sfx;
using Output = isle::audio::SdlAudioOutput;
[[noreturn]] void AudioFailure(Output& output, const char* operation) {
    const auto& error = output.error();
    throw std::runtime_error(std::string(operation) + ": " + error.operation.data() + ": " + error.message.data());
}
} // namespace

AudioHostProbe::AudioHostProbe(const isle::rom::Snapshot& snapshot) : session_(snapshot, 14) {
    if (!output_.open({static_cast<int>(probe::synthesis_rate), 4096, 0}))
        AudioFailure(output_, "checked audio admission failed");
    auto& producer = session_.producer();
    probe::require(producer.stage_start(session_.player_parameters({}), generation_), "host experimental SFX14 start failed");
    probe::require(producer.stage(), "host initial audio publication failed");
    const auto& info = output_.info();
    std::cout << "AUDIO_STREAM_OPEN: driver=" << info.driver.data()
              << " physical_device=" << info.physical_device << " logical_device=" << info.logical_device
              << " source_hz=" << info.source.freq << " source_channels=" << info.source.channels
              << " device_hz=" << info.device.freq << " device_channels=" << info.device.channels
              << " device_format=" << info.device.format << " device_buffer_frames=" << info.device_buffer_frames
              << "; actual native dry SFX14/RSP, explicit quota640; host queue is NOT guest AI occupancy; "
                 "independent audio/video experiment, not original cue synchronization or hearing proof.\n" << std::flush;
}
AudioHostProbe::~AudioHostProbe() { Abort(); }

void AudioHostProbe::Build() {
    // Explicit experimental quota: the unused occupancy field is not an
    // observation of zero AI occupancy and never comes from the SDL queue.
    pending_.emplace(session_.execute({++service_, 0, probe::default_quota}));
    probe::require(session_.producer().copy_pcm(pending_->pcm, samples_), "host owned PCM extraction failed");
    produced_ += pending_->pcm.frame_count;
}
bool AudioHostProbe::Submit() {
    if (!pending_) return true;
    const auto& pcm = pending_->pcm;
    const auto result = output_.try_submit(std::span<const std::int16_t>(samples_).first(pcm.frame_count * 2u));
    if (result == Output::Submit::would_block) return false;
    if (result != Output::Submit::accepted) AudioFailure(output_, "host audio submission failed");
    accepted_ += pcm.frame_count;
    // SDL owns a full copy. Only now may this exact producer lease retire.
    probe::require(session_.producer().release(pcm.task_id, pcm.generation), "host copied PCM release failed");
    pending_.reset();
    return true;
}
void AudioHostProbe::Tick() {
    if (closed_) throw std::runtime_error("host audio tick after closure");
    if (output_.state() != Output::State::ready) AudioFailure(output_, "host audio endpoint failed");
    if (!Submit()) return;
    Build();
    Submit(); // WouldBlock retains this same lease and scratch for the next tick.
}
void AudioHostProbe::Wait(std::chrono::steady_clock::time_point deadline) {
    SDL_PumpEvents();
    if (std::chrono::steady_clock::now() >= deadline)
        throw std::runtime_error("host audio stream deadline expired; no drain or playback completion claimed");
    SDL_Delay(1);
}
void AudioHostProbe::Finish() {
    if (closed_) throw std::runtime_error("host audio finish after closure");
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    auto& producer = session_.producer();
    if (producer.voice_active()) {
        probe::require(producer.stage_stop(generation_), "host SFX stop failed");
        probe::require(producer.stage(), "host SFX stop publication failed");
    }
    while (pending_ || producer.voice_active()) {
        if (!pending_) Build();
        if (!Submit()) Wait(deadline);
    }
    if (producer.has_lease() || produced_ != accepted_)
        throw std::runtime_error("host audio completion retained a lease or lost a whole source block");
    if (!output_.finish_input()) AudioFailure(output_, "host audio final flush failed");
    for (;;) {
        const auto drain = output_.poll_drain();
        if (drain == Output::Drain::stream_drained) break;
        if (drain == Output::Drain::error) AudioFailure(output_, "host audio stream drain failed");
        Wait(deadline);
    }
    const auto closed = output_.close();
    closed_ = true;
    cleanup_ok_ = !closed.failed && closed.disposition == Output::CloseDisposition::stream_drained;
    if (!cleanup_ok_) AudioFailure(output_, "host audio checked closure failed");
    std::cout << "AUDIO_STREAM_DRAINED: actual_tasks=" << service_ << " source_frames=" << produced_
              << " accepted_copy_frames=" << accepted_
              << "; each SP=0x203, no DPC; final flush and stream counters exhausted, not DAC/hearing proof.\n"
                 "AUDIO_STREAM_OWNERS_RELEASED: removal watch and stream destroyed before host SDL teardown.\n" << std::flush;
}
bool AudioHostProbe::Abort() noexcept {
    if (closed_) return cleanup_ok_;
    cleanup_ok_ = !output_.close().failed;
    if (pending_) {
        const auto& pcm = pending_->pcm;
        const auto result = session_.producer().release(pcm.task_id, pcm.generation);
        cleanup_ok_ = result == sfx::Error::none && cleanup_ok_;
        pending_.reset();
    }
    closed_ = true;
    return cleanup_ok_;
}
