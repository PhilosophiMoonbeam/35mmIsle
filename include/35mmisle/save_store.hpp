#ifndef ISLE_SAVE_STORE_HPP
#define ISLE_SAVE_STORE_HPP

#include "35mmisle/save_image.hpp"

#include <array>
#include <filesystem>
#include <memory>

namespace isle::save {

enum class StoreErrc {
    none, invalid_argument, io, lock_busy, not_regular_file, codec,
    recovery_required, generation_conflict, expected_generation,
    generation_exhausted, orphan_temporary, session_frozen, verification_mismatch,
};
enum class StorePhase {
    none, argument, open_directory, open_lock, acquire_lock, scan_directory,
    load_a, load_b, preflight, create_temporary, write_temporary,
    verify_temporary, flush_temporary, close_temporary, install,
    reopen_installed, verify_installed, flush_installed, close_installed,
    acknowledge_directory, cleanup_temporary,
};
struct StoreError {
    StoreErrc code = StoreErrc::none;
    StorePhase phase = StorePhase::none;
    std::uint32_t native_error = 0; // errno on Linux; GetLastError() on Windows.
    Error codec_error = Error::none;
    [[nodiscard]] explicit operator bool() const noexcept { return code != StoreErrc::none; }
};

// Owned immutable canonical bytes, independent of the Store/lock lifetime.
// This is structural validity, not a claim of photo/scene replay safety.
class Snapshot {
public:
    Snapshot() noexcept = default;
    [[nodiscard]] explicit operator bool() const noexcept { return bool(data_); }
    [[nodiscard]] ImageView image() const noexcept;
    [[nodiscard]] std::uint64_t generation() const noexcept;
private:
    struct Data;
    std::shared_ptr<const Data> data_;
    friend class Store;
};

enum class SlotState { missing, valid, rejected };
struct SlotResult {
    SlotState state = SlotState::missing;
    std::uint64_t generation = 0;
    StoreError error{};
};
enum class LoadState { ready, missing, recovery_required, conflict };
struct LoadResult {
    LoadState state = LoadState::recovery_required;
    Snapshot snapshot{}; // Good candidate only; never chosen across a conflict.
    std::array<SlotResult, 2> slots{}; // save.a, save.b, including every rejection.
    bool orphan_temporary = false;
    StoreError error{StoreErrc::invalid_argument, StorePhase::argument};
    [[nodiscard]] bool writable() const noexcept {
        return state == LoadState::ready || state == LoadState::missing;
    }
};

enum class CommitOutcome { published, not_published, indeterminate };
enum class Durability { none, linux_file_and_directory_synced, win32_acknowledged };
struct CommitResult {
    CommitOutcome outcome = CommitOutcome::not_published;
    std::uint64_t generation = 0; // Nonzero only after publication is acknowledged.
    Durability durability = Durability::none;
    StoreError error{StoreErrc::invalid_argument, StorePhase::argument};
    // A failed cleanup of this operation's known-uninstalled temporary is separate
    // from the original error. Existing or ambiguous artifacts are never deleted.
    // Cleanup failure also freezes writes: its retained orphan needs reconciliation.
    StoreError cleanup_error{};
};

struct OpenResult;
// Single caller-serialized session; no background work. Only qualified local
// Linux/Windows filesystems are in scope, not network/cloud/power-loss promises.
// Session locks coordinate cooperative users, not arbitrary external modifiers.
class Store {
public:
    Store() noexcept;
    ~Store();
    Store(Store&&) noexcept;
    Store& operator=(Store&&) noexcept;
    Store(const Store&) = delete;
    Store& operator=(const Store&) = delete;
    [[nodiscard]] explicit operator bool() const noexcept;

    // Requires an existing absolute directory. Does not mkdir, initialize a game,
    // repair slots, or promote orphans. Acquires stable save.lock before loading;
    // holds it until this Store is destroyed/replaced, even in read-only recovery.
    // Allocation/path-construction exceptions follow the standard library.
    [[nodiscard]] static OpenResult open(const std::filesystem::path& directory);

    // expected_generation is zero only for a genuinely missing profile. The
    // caller keeps image bytes alive and immutable until return. No payload copy
    // or allocation is needed for a commit. An indeterminate result freezes this
    // session until close/reopen/reconciliation; it never means "unchanged".
    [[nodiscard]] CommitResult commit(ImageView image, std::uint64_t expected_generation) noexcept;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
struct OpenResult {
    Store store{};
    LoadResult load{};
    StoreError error{StoreErrc::invalid_argument, StorePhase::argument};
    // True means locked and examined, not necessarily writable. Inspect load.
    [[nodiscard]] explicit operator bool() const noexcept { return bool(store) && !error; }
};

} // namespace isle::save

#endif
