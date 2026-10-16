#ifndef ISLE_ACTUAL_AUDIO_DRIVER_HPP
#define ISLE_ACTUAL_AUDIO_DRIVER_HPP

#include "game_audio.hpp"
#include "graphics_task.hpp"
#include "35mmisle/rsp.hpp"
#include <algorithm>
#include <memory>
#include <stdexcept>
#include <vector>

namespace isle::private_audio {
namespace g = game_audio;
namespace a = audio;
struct Capture {
    // Checked owned PCM, not a physical device or discard/Null consumer.
    std::vector<std::int16_t> samples = std::vector<std::int16_t>(512*a::AudioService::max_frames*2);
    std::size_t used = 0;
    g::Output::State state = g::Output::State::ready;
    unsigned finishes = 0, polls = 0;
    static g::Output::Info inspect(void* p) noexcept { return {static_cast<Capture*>(p)->state,g::source_rate}; }
    static g::Output::Acceptance submit(void* p,std::span<const std::int16_t> block) noexcept {
        auto& c=*static_cast<Capture*>(p);
        if (c.state!=g::Output::State::ready || (block.size()&1u) || block.empty() || block.size()>c.samples.size()-c.used) return {};
        std::copy(block.begin(),block.end(),c.samples.begin()+static_cast<std::ptrdiff_t>(c.used));
        c.used+=block.size();
        return {g::Output::Submit::accepted,static_cast<std::uint32_t>(block.size()/2)};
    }
    static bool finish(void* p) noexcept {
        auto& c=*static_cast<Capture*>(p);
        if (c.state!=g::Output::State::ready || c.finishes!=0) return false;
        ++c.finishes; c.state=g::Output::State::draining; return true;
    }
    static g::Output::State poll(void* p) noexcept {
        auto& c=*static_cast<Capture*>(p);
        if (c.state==g::Output::State::draining && ++c.polls>=2) c.state=g::Output::State::drained;
        return c.state;
    }
    g::Output output() noexcept { return {this,inspect,submit,finish,poll}; }
};
inline bool reject_dp(void*,std::span<const std::uint32_t>) noexcept { return false; }
struct Execution {
    const rom::Snapshot& snapshot;
    const std::uint32_t* borrowed = nullptr;
    std::unique_ptr<rsp::Machine> machine;
    static bool initialize(void* p,std::span<std::uint32_t> words) noexcept {
        auto& e=*static_cast<Execution*>(p);
        try {
            graphics::validate_cold_inputs(words,e.snapshot,e.snapshot.canonical().data(),0x107000);
            graphics::install_cold_inputs(words,e.snapshot,0x107000);
            e.machine=std::make_unique<rsp::Machine>(words,reject_dp,nullptr);
            e.borrowed=words.data(); return true;
        } catch (...) { e.machine.reset(); e.borrowed=nullptr; return false; }
    }
    static bool ready(void* p,std::span<std::uint32_t> words) noexcept {
        const auto& e=*static_cast<Execution*>(p);
        return bool(e.machine) && e.borrowed==words.data();
    }
    static void retire(void* p) noexcept {
        auto& e=*static_cast<Execution*>(p); e.machine.reset(); e.borrowed=nullptr;
    }
    g::Executor executor() noexcept { return {this,initialize,ready,retire}; }
};
inline rsp::Result execute(g::Session& session,Execution& execution,const a::TaskView& built) {
    const rsp::Task task{2,0,0x107000,0x100,0x3d980,0x1000,0x44ba0,0x800,
        0,0,0,0,built.command_address,built.command_bytes,0,0};
    rsp::Result actual;
    try { actual=execution.machine->execute(task,execution.snapshot.canonical().subspan(0xb70,0x100),{10000000,0}); }
    catch (...) { session.complete(built.token,false); throw; }
    const bool success=bool(actual) && actual.sp_status==0x203 && actual.packets==0 && actual.packet_words==0 && actual.full_syncs==0;
    const auto completed=session.complete(built.token,success);
    if (!success) throw std::runtime_error("actual audio RSP did not reach checked SP203/no-DPC terminal");
    if (completed!=a::Error::none) throw std::runtime_error("actual RSP PCM lease completion failed");
    return actual;
}
} // namespace isle::private_audio
#endif
