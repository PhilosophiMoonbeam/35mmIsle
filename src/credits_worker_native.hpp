#ifndef ISLE_CREDITS_WORKER_NATIVE_HPP
#define ISLE_CREDITS_WORKER_NATIVE_HPP
#include "credits_source.h"
#include <array>
#include <thread>

/* Construct once on the scene constructing thread, before its Original entry.
 * The independent continuation owns the actual pinned C worker stack. Only its
 * bounded mailbox and native exclusion/priority bookkeeping live in C++. */
struct IsleCreditsWorker final {
    explicit IsleCreditsWorker(IsleCreditsSource&);
    ~IsleCreditsWorker();
    IsleCreditsWorker(const IsleCreditsWorker&)=delete;
    IsleCreditsWorker& operator=(const IsleCreditsWorker&)=delete;
    IsleGameResult pump() noexcept;
    IsleGameResult request_cancel() noexcept;
    IsleGameResult join(int*) noexcept;
    bool main_excluded() const noexcept { return excluded; }
    bool joined() const noexcept { return retired; }
    // Earlier barrier: the real C worker stack has joined, but native
    // Replay/GPU/readback and queue-incarnation holds may still be draining.
    bool continuation_joined() const noexcept { return stack_joined; }
    IsleContinuationDomain* domain_handle() const noexcept { return domain; }
    IsleGameResult check_access() const noexcept;
    IsleGameResult send(IsleGameSprite*,int*) noexcept;
    IsleGameResult receive(IsleGameSprite**,int*) noexcept;
    IsleGameResult image(IsleGameSprite*,IsleCreditsDestination*,int*,int*) noexcept;
    IsleGameResult queue(size_t) noexcept;
    IsleGameResult create_thread(int32_t,IsleContinuationEntry,void*,int32_t) noexcept;
    IsleGameResult start() noexcept;
    IsleGameResult priority(int32_t) noexcept;
    IsleGameResult exclude_main(int) noexcept;
    IsleGameResult status() const noexcept { return failure; }
private:
    IsleCreditsSource& source;
    const std::thread::id creator=std::this_thread::get_id();
    IsleContinuationDomain* domain=nullptr;
    IsleContinuation* continuation=nullptr;
    std::array<IsleCreditsDestination,10> fifo{};
    IsleCreditsDestination inflight{};
    size_t head=0,count=0;
    int32_t current_priority=0x31;
    bool configured=false,created=false,started=false,blocked_receive=false;
    bool excluded=false,cancelling=false,stack_joined=false,retired=false;
    bool servicing=false,cancel_requested=false;
    IsleGameResult failure=ISLE_GAME_OK;
    IsleGameResult fail(IsleGameResult) noexcept;
    IsleGameResult release_inflight() noexcept;
};
#endif
