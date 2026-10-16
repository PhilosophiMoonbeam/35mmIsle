#ifndef ISLE_RSP_RENDER_HPP
#define ISLE_RSP_RENDER_HPP

#include "35mmisle/rsp.hpp"
#include "rsp_packets.hpp"

#include <memory>
#include <span>

namespace isle::rsp {
// One processor-owned persistent RDRAM/hidden allocation and one actual RSP
// machine. Every operation, including lease access/move/destruction, belongs to
// the creating thread. Wrong-thread noexcept operations terminate.
class CanonicalExecutor {
    struct Impl;
public:
    class PreparationLease {
    public:
        PreparationLease() noexcept = default;
        ~PreparationLease();
        PreparationLease(const PreparationLease&) = delete;
        PreparationLease& operator=(const PreparationLease&) = delete;
        PreparationLease(PreparationLease&& other) noexcept;
        PreparationLease& operator=(PreparationLease&& other) noexcept;
        explicit operator bool() const noexcept;
        std::span<std::uint32_t> words() const noexcept;
        // All borrowed CPU writers MUST have ceased access before cancel,
        // destruction or execute. Abandonment poisons, never rolls RAM back.
        void cancel() noexcept;
    private:
        friend class CanonicalExecutor;
        explicit PreparationLease(Impl& owner) noexcept;
        Impl* owner_ = nullptr;
    };

    class MemoryLease {
    public:
        MemoryLease() noexcept = default;
        ~MemoryLease();
        MemoryLease(const MemoryLease&) = delete;
        MemoryLease& operator=(const MemoryLease&) = delete;
        MemoryLease(MemoryLease&& other) noexcept;
        MemoryLease& operator=(MemoryLease&& other) noexcept;
        explicit operator bool() const noexcept;
        // Actual completed mappings, not copies. RDRAM is word-swapped32;
        // hidden retains the backend coverage-byte layout. All spans/references
        // expire on release. A borrowing reader must cease access first.
        std::span<const std::byte> rdram() const noexcept;
        std::span<const std::byte> hidden() const noexcept;
        // Exclusive creator-thread CPU postprocess of this actually completed
        // RDRAM, not preparation or a copied image. Hidden remains read-only.
        // Cease writer access before finish_postprocess, which performs real
        // host writeback. Finish before release; abandonment poisons the owner.
        std::span<std::uint32_t> postprocess_words();
        void finish_postprocess();
        const Result& rsp_result() const;
        const PacketBuffer& packets() const;
        void release() noexcept;
    private:
        friend class CanonicalExecutor;
        MemoryLease(Impl& owner, std::span<const std::byte> rdram,
                    std::span<const std::byte> hidden) noexcept;
        Impl* owner_ = nullptr;
        std::span<const std::byte> rdram_;
        std::span<const std::byte> hidden_;
        bool postprocessing_ = false;
    };

    explicit CanonicalExecutor(bool require_cpu, Limits limits = {});
    // Preparation writers/result readers must cease access before destruction.
    // An outstanding GPU-only token is drained without publishing completion;
    // the worker joins before machine/processor/backing-storage destruction.
    ~CanonicalExecutor();
    CanonicalExecutor(const CanonicalExecutor&) = delete;
    CanonicalExecutor& operator=(const CanonicalExecutor&) = delete;
    CanonicalExecutor(CanonicalExecutor&&) = delete;
    CanonicalExecutor& operator=(CanonicalExecutor&&) = delete;

    // The caller initializes every byte its task uses. Hidden is initialized
    // once at cold creation; neither memory is implicitly reset between tasks.
    PreparationLease begin_preparation();
    // RSP execution remains synchronous on the owner thread (shared upstream
    // JIT globals are not qualified for concurrent audio/graphics execution).
    // Only the actual GPU submission/fence/read mapping runs on one persistent
    // worker. No native view or preparation writer survives begin().
    using Token = std::uint64_t;
    Token begin(PreparationLease&& preparation, const Task& task,
                std::span<const std::byte> boot);
    // Nonblocking actual GPU completion, not FullSync. A failure drains/poisons
    // and throws without publishing a result lease. Tokens are single-use.
    bool poll(Token token);
    MemoryLease completed(Token token);
    // Blocking shutdown/abandonment fence, not an authored frame completion.
    // On failure drains/poisons and throws; no result mapping escapes.
    void retire(Token token);
    // Consumes exclusive CPU preparation, runs the actual persistent RSP to a
    // bounded packet buffer, flushes CPU+RSP writes, then enqueues all RDP work.
    // Real backend/device completion precedes readback. FullSync is not a fence.
    // Any execution failure poisons the instance; no result lease is published.
    MemoryLease execute(PreparationLease&& preparation, const Task& task,
                        std::span<const std::byte> boot);
    bool quiescent() const noexcept;
private:
    std::unique_ptr<Impl> impl_;
};
} // namespace isle::rsp
#endif
