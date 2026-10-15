#ifndef ISLE_RSP_RENDER_HPP
#define ISLE_RSP_RENDER_HPP

#include <cstddef>
#include <filesystem>
#include <memory>
#include <span>
#include <string_view>

namespace isle::rsp_probe { class Capture; }

namespace isle::rsp::probe {
// Persistent after-SP synthetic replay in independently initialized RDP-owned
// memory, not shared production GAME RDRAM/coherency. All executor operations
// belong to the constructing thread; wrong-thread destruction is fatal.
class CanonicalCaptureExecutor {
    struct Impl;
public:
    // No copy/readback buffer: these are the actual completed backend mappings.
    // RDRAM is word-swapped32 (canonical byte a at host byte[a^3]); hidden keeps
    // the backend coverage-byte layout. Spans expire on release. The creating
    // thread retrieves and releases the lease; it may lend spans to a CPU owner
    // only if that owner's access ceases before release. No GPU call occurs on
    // that borrowing CPU thread.
    class MemoryLease {
    public:
        MemoryLease() noexcept = default;
        ~MemoryLease();
        MemoryLease(const MemoryLease&) = delete;
        MemoryLease& operator=(const MemoryLease&) = delete;
        MemoryLease(MemoryLease&& other) noexcept;
        MemoryLease& operator=(MemoryLease&& other) noexcept;
        explicit operator bool() const noexcept { return owner_ != nullptr; }
        std::span<const std::byte> rdram() const noexcept;
        std::span<const std::byte> hidden() const noexcept;
        void release() noexcept;
    private:
        friend class CanonicalCaptureExecutor;
        MemoryLease(Impl& owner, std::span<const std::byte> rdram,
                    std::span<const std::byte> hidden) noexcept;
        Impl* owner_ = nullptr;
        std::span<const std::byte> rdram_;
        std::span<const std::byte> hidden_;
    };

    explicit CanonicalCaptureExecutor(bool require_cpu_device);
    // Destroying while executing/leased terminates; borrowed storage is never
    // silently freed. Finish all CPU access and release on the creator first.
    ~CanonicalCaptureExecutor();
    CanonicalCaptureExecutor(const CanonicalCaptureExecutor&) = delete;
    CanonicalCaptureExecutor& operator=(const CanonicalCaptureExecutor&) = delete;
    CanonicalCaptureExecutor(CanonicalCaptureExecutor&&) = delete;
    CanonicalCaptureExecutor& operator=(CanonicalCaptureExecutor&&) = delete;

    // Validates the restricted memory-independent fixture before mutation.
    // Capture storage must remain immutable until this call returns or throws.
    // Real idle/device completion, backend diagnostics and guards precede the
    // lease. Backend failures poison this instance; create a new one to retry.
    MemoryLease execute(const ::isle::rsp_probe::Capture& capture);
    // No executing work or outstanding lease; not an executor-health query.
    bool quiescent() const noexcept;
private:
    friend void render_capture(CanonicalCaptureExecutor&, const ::isle::rsp_probe::Capture&,
                               const std::filesystem::path&, std::string_view);
    std::unique_ptr<Impl> impl_;
};

// The caller supplies an existing new empty directory. Uses the same real
// executor/lease; throws on unsupported packets, backend errors or output I/O.
// Guarded snapshots retain their existing format; metadata is written last.
void render_capture(CanonicalCaptureExecutor& executor,
                    const ::isle::rsp_probe::Capture& capture,
                    const std::filesystem::path& output_directory,
                    std::string_view label);
} // namespace isle::rsp::probe
#endif
