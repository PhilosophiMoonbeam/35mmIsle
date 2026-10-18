#include "host_progress.hpp"
#include <algorithm>
#include <cstdio>
#include <thread>

namespace isle::host {
namespace {
game_host::Input host_acquisition(void* owner) noexcept {
    return static_cast<game_host::Host*>(owner)->acquire_input();
}
void retain(std::array<char,512>& destination, const char* message) noexcept {
    std::snprintf(destination.data(),destination.size(),"%s",message && message[0]?message:"actual Host progress failed");
}
}
Progress::Progress(game_host::Host& host, game_input::Service& input, InputAcquisition acquisition)
    : host_(host), input_(input), acquisition_(acquisition.read?acquisition:InputAcquisition{&host,host_acquisition}) {}

void Progress::bind(GtlClock& clock, InputAcknowledgement acknowledgement) noexcept {
    clock_=&clock;
    acknowledgement_=acknowledgement;
    offerable_=0;
}
void Progress::unbind() noexcept {
    clock_=nullptr;
    acknowledgement_={};
    offerable_=0;
    policy_={};
}
void Progress::fail(const char* message) noexcept {
    if (status_==ProgressStatus::failed) return;
    retain(message_,message);
    status_=ProgressStatus::failed;
}
void Progress::request_stop() noexcept {
    if (status_==ProgressStatus::running) status_=ProgressStatus::stopped;
}
std::uint64_t Progress::elapsed_ns() const noexcept {
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(Time::now()-epoch_).count());
}
std::uint32_t Progress::raw(const ProgressTick& tick) noexcept {
    return static_cast<std::uint32_t>(RetraceClock::guest_ticks(tick.ns));
}
void Progress::set_deadline(Time::time_point deadline, const char* diagnostic) noexcept {
    deadline_=deadline;
    retain(deadline_message_,diagnostic);
}
bool Progress::deadline_expired() const noexcept { return Time::now()>=deadline_; }
void Progress::observe_deadline() noexcept {
    if (deadline_expired()) fail(deadline_message_.data());
}
void Progress::acquire(ProgressTick& tick) noexcept {
    const auto pending=input_.pending_read();
    if (!pending) return;
    if (live_read_ && live_read_!=pending) {
        fail("outstanding Host acquisition identity changed without completion");return;
    }
    live_read_=pending;
    if (!acknowledgement_.acknowledge) {
        fail("actual Input completion has no bound acknowledgement owner");return;
    }
    const auto acquired=acquisition_.read(acquisition_.owner);
    if (!acquired.completed) return;
    tick.completion=input_.complete_read(pending,acquired.ports);
    if (tick.completion.error!=game_input::Error::none) {
        fail("actual Input read completion rejected");return;
    }
    live_read_={};
    tick.completed=true;
    tick.completion_accepted=acknowledgement_.acknowledge(acknowledgement_.owner,tick.completion);
    if (!tick.completion_accepted)
        fail("actual Runtime Input acknowledgement rejected");
}
ProgressTick Progress::complete_input() noexcept {
    ProgressTick tick;
    tick.ns=elapsed_ns();
    acquire(tick);
    if (host_.error()!=game_host::Error::none) fail(host_.message());
    observe_deadline();
    return tick;
}
ClockError Progress::offer_retraces(std::uint64_t count) noexcept {
    if (!count) return ClockError::none;
    if (!clock_ || count>offerable_) {
        fail("GTL offer lacks actual unoffered Host retraces");return ClockError::invalid;
    }
    offerable_-=count;
    if (clock_->stopping() || clock_->stopped()) return ClockError::closed;
    const auto result=clock_->offer_retraces(count);
    if (result!=ClockError::none && result!=ClockError::full)
        fail("actual Host retrace GTL admission failed");
    return result;
}
ProgressTick Progress::poll() noexcept {
    ProgressTick tick;
    tick.ns=elapsed_ns();
    observe_deadline();
    if (policy_.poll_events && !host_.pump_events()) {
        if (host_.error()!=game_host::Error::none) fail(host_.message());
        else request_stop();
    }
    tick.count=retraces_.advance(tick.ns);
    sampled_ns_=tick.ns;
    for (std::uint64_t n=0;n<tick.count;++n) {
        if (status_==ProgressStatus::running && clock_ && policy_.request_input && input_.running()) {
            if (const auto pending=input_.pending_read()) {
                if (live_read_ && live_read_!=pending) fail("outstanding Host acquisition identity changed without completion");
                live_read_=pending;
                ++tick.read_skips;
            } else {
                if (live_read_) { fail("outstanding Host acquisition disappeared without completion"); }
                else {
                    const auto wanted=input_.retrace();
                    if (wanted.error!=game_input::Error::none) fail("actual Input retrace read request rejected");
                    else if (wanted.read) {
                        if (wanted.read.owner!=&input_ || wanted.read!=input_.pending_read())
                            fail("actual Input retrace returned a different live read");
                        else { tick.requested=wanted.read;live_read_=wanted.read; }
                    }
                }
            }
        }
        if (host_.error()==game_host::Error::none && !host_.notify_retrace(++ordinal_,tick.ns))
            fail(host_.message());
    }
    // One acquisition AFTER the whole real coalesced batch. A pending provider
    // retains the exact read while independent events, VI and Audio continue.
    if (status_==ProgressStatus::running && clock_ && policy_.complete_input &&
        input_.running() && host_.error()==game_host::Error::none) acquire(tick);
    if (host_.error()!=game_host::Error::none) fail(host_.message());
    if (clock_ && policy_.scene_active && !clock_->stopping() && !clock_->stopped()) {
        offerable_+=tick.count;
        if (policy_.offer_retraces) (void)offer_retraces(offerable_);
    } else offerable_=0;
    // Even stopped/failed progress services independent actual Audio obligations
    // while the caller drains native owners. No new Input request is admitted.
    if (host_.error()==game_host::Error::none && !host_.pump_audio()) fail(host_.message());
    observe_deadline();
    return tick;
}
ProgressStatus Progress::idle() noexcept {
    observe_deadline();
    const auto now=Time::now();
    const auto since_sample=elapsed_ns()-sampled_ns_;
    const auto next=retraces_.nanoseconds_until_next();
    const auto until_retrace=since_sample<next?next-since_sample:0;
    auto delay=std::chrono::nanoseconds(std::min<std::uint64_t>(1000000,until_retrace));
    if (deadline_!=Time::time_point::max()) {
        const auto remaining=std::chrono::duration_cast<std::chrono::nanoseconds>(deadline_-now);
        delay=std::min(delay,std::max(remaining,std::chrono::nanoseconds::zero()));
    }
    if (delay>std::chrono::nanoseconds::zero()) std::this_thread::sleep_for(delay);
    observe_deadline();
    return status_;
}
} // namespace isle::host
