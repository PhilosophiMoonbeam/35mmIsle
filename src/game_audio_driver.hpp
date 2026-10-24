#ifndef ISLE_GAME_AUDIO_DRIVER_HPP
#define ISLE_GAME_AUDIO_DRIVER_HPP

#include "game_audio.hpp"
#include <array>
#include <memory>

namespace isle::game_audio {
enum class DriverError { none, invalid, allocation, resources, execution, output, retired };
enum class Drain { pending, drained, failed };
struct DriverCreateResult;

// Serialized GAME service owner. Owns Session, producer RAM, decoded inputs and
// its actual audio RSP borrower. Output is borrowed and must outlive this driver;
// owner-affine endpoint callbacks must rendezvous with their actual owner.
// Snapshot is borrowed only during create. No renderer/presentation supplies
// retraces, output acceptance or audio task terminals.
class Driver final {
public:
    static DriverCreateResult create(const rom::Snapshot&, Output) noexcept;
    ~Driver();
    Driver(const Driver&) = delete;
    Driver& operator=(const Driver&) = delete;

    Session& session() noexcept;
    DriverError error() const noexcept;
    const char* message() const noexcept;
    bool notify_retrace(std::uint64_t ordinal, std::uint64_t monotonic_ns) noexcept;
    // At most one actual bounded RSP execution. Blocked/partial PCM acceptance
    // retains the exact Session lease; no queue admission is a task terminal.
    bool pump() noexcept;
    bool stop() noexcept;
    Drain poll_drain() noexcept;
    // Call after every GAME user has retired. A still-retained completed PCM
    // lease is honestly abandoned, never described as drained. RSP is destroyed
    // before producer RAM. Idempotent; destruction follows the same abort path.
    bool retire() noexcept;

private:
    struct Impl;
    explicit Driver(std::unique_ptr<Impl>) noexcept;
    std::unique_ptr<Impl> impl_;
};
struct DriverCreateResult {
    DriverError error = DriverError::none;
    std::unique_ptr<Driver> driver;
    std::array<char,512> message{};
    explicit operator bool() const noexcept { return error == DriverError::none && bool(driver); }
};
} // namespace isle::game_audio
#endif
