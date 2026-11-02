#ifndef ISLE_GAME_RUN_REPORT_HPP
#define ISLE_GAME_RUN_REPORT_HPP

#include "application_runtime.hpp"
#include "game_host.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <memory>

namespace isle::game_run_report {
// These purposes affect postclosure evidence acceptance only, never GAME input,
// scheduling, Source results, Save timing or automatic stopping.
enum class Purpose { disabled, campaign, cold_boot };
inline constexpr std::size_t boundary_capacity = 65536;
inline constexpr std::uint32_t no_row = std::numeric_limits<std::uint32_t>::max();
// Shared passive labels for the same actual entry/load facts in private reports.
const char* entry_name(IsleApplicationEntryOperation) noexcept;
const char* save_load_name(save::GameLoadState) noexcept;

// Scalar-only correlation. GAME constructs this while its completed Frame is
// borrowed. Main never inspects Runtime or retains a GAME/scene/pixel pointer.
struct FrameIdentity {
    std::uint64_t application_epoch = 0;
    application::EntryIntent entry{};
    std::uint64_t frame_ordinal = 0;
    std::uint32_t row = no_row;
};

struct EpochRetirement {
    std::uint64_t application_epoch = 0;
    bool successful = false;
    bool reset_requested = false;
    bool disconnect_requested = false;
    bool runtime_retired = false;
    bool input_stopped = false;
    bool audio_drained = false;
    bool driver_retired = false;
};

// Concrete opt-in recorder, not a callback registry or controller interface.
// Enabled storage is allocated once before GAME starts. No method below does
// per-frame allocation, output I/O, reentrant callbacks, waits or GAME control.
// GAME and main have independent single-writer partitions. Existing Owner
// handoff synchronization carries FrameIdentity; reconciliation is post-join.
class Recorder final {
public:
    explicit Recorder(Purpose);
    ~Recorder();
    Recorder(const Recorder&) = delete;
    Recorder& operator=(const Recorder&) = delete;
    Recorder(Recorder&&) = delete;
    Recorder& operator=(Recorder&&) = delete;

    // GAME-thread operations. Retain only copied entry/result scalars and the
    // explicitly owning Save/Boot publications. Borrowed scene pointers expire.
    FrameIdentity game_frame(std::uint64_t application_epoch,
                             const application::EntryIntent&,
                             std::uint64_t frame_ordinal) noexcept;
    void game_retired(std::uint64_t application_epoch,
                      const application::SceneRetirementView&) noexcept;
    void game_epoch_retired(const EpochRetirement&) noexcept;

    // Main-thread operations, outside Owner's mutex. Queue success establishes
    // one pending Host identity; a declined-but-still-pending presentation keeps
    // it. Only actual Present::completed grants completion credit. Cancellation,
    // replacement, copy acknowledgement and GAME Frame release grant none.
    void main_queued(const FrameIdentity&, std::uint64_t offer_serial) noexcept;
    void main_presented(game_host::Present, bool still_pending) noexcept;
    void main_cancelled(bool succeeded) noexcept;
    void main_host_shutdown(std::uint64_t application_epoch,
                            bool cancel_present_succeeded,
                            const game_host::Shutdown&) noexcept;

    // Main calls this only after the persistent GAME worker was actually joined.
    // No live summary/payload is exposed to the semantic controller. A Native
    // boundary result never replaces independent pixels or external custody.
    void joined() noexcept;
    bool readable() const noexcept;
    bool native_complete() const noexcept;
    const char* message() const noexcept;

    // Terminal-only, bounded private output: report.json and available complete
    // immutable terminal-save.bin / boot-load.bin canonical images. Refuse
    // existing destinations and preserve failure as incomplete evidence. Do not
    // claim first Mew acquisition from first durable Mew publication; temporal
    // acquisition/count proof remains independent public-pixel evidence.
    bool write(const std::filesystem::path& output_directory,
               std::array<char,512>& error) const noexcept;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace isle::game_run_report
#endif
