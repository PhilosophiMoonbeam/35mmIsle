#include "game_run_report.hpp"

#include <algorithm>
#include <cerrno>
#include <charconv>
#include <cstdio>
#include <cstring>
#include <new>
#include <string_view>
#include <utility>
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace isle::game_run_report {
namespace {
using application::EntryIntent;
constexpr std::size_t maximum_report_bytes = 256 * 1024 * 1024;

bool same_entry(const EntryIntent& a, const EntryIntent& b) noexcept {
    return a.serial == b.serial && a.operation == b.operation && a.argument == b.argument;
}
bool same_row(const FrameIdentity& a, const FrameIdentity& b) noexcept {
    return a.application_epoch == b.application_epoch && same_entry(a.entry, b.entry) && a.row == b.row;
}
bool valid_entry(const EntryIntent& entry) noexcept {
    return entry.serial && entry.operation >= ISLE_APPLICATION_ENTRY_BOOT &&
        entry.operation <= ISLE_APPLICATION_ENTRY_PRINTER_DIAGNOSTIC;
}
void remember(const char*& destination, const char* message) noexcept {
    if (!destination) destination = message;
}
void count(std::uint64_t& value, const char*& failure) noexcept {
    if (value == std::numeric_limits<std::uint64_t>::max())
        remember(failure, "evidence counter exhausted");
    else ++value;
}
const char* purpose_name(Purpose purpose) noexcept {
    switch (purpose) {
    case Purpose::campaign: return "campaign";
    case Purpose::cold_boot: return "cold-boot";
    default: return "disabled";
    }
}
const char* operation_name(IsleApplicationEntryOperation operation) noexcept {
    switch (operation) {
    case ISLE_APPLICATION_ENTRY_BOOT: return "Boot";
    case ISLE_APPLICATION_ENTRY_TITLE: return "Title";
    case ISLE_APPLICATION_ENTRY_NAME: return "Name";
    case ISLE_APPLICATION_ENTRY_BEACH: return "Beach";
    case ISLE_APPLICATION_ENTRY_TUNNEL: return "Tunnel";
    case ISLE_APPLICATION_ENTRY_VOLCANO: return "Volcano";
    case ISLE_APPLICATION_ENTRY_RIVER: return "River";
    case ISLE_APPLICATION_ENTRY_CAVE: return "Cave";
    case ISLE_APPLICATION_ENTRY_VALLEY: return "Valley";
    case ISLE_APPLICATION_ENTRY_RAINBOW: return "Rainbow";
    case ISLE_APPLICATION_ENTRY_CAMERA_CHECK: return "CameraCheck";
    case ISLE_APPLICATION_ENTRY_OAKS_LAB: return "OaksLab";
    case ISLE_APPLICATION_ENTRY_ALBUM: return "Album";
    case ISLE_APPLICATION_ENTRY_REPORT: return "Report";
    case ISLE_APPLICATION_ENTRY_PHOTO_CHECK: return "PhotoCheck";
    case ISLE_APPLICATION_ENTRY_GALLERY: return "Gallery";
    case ISLE_APPLICATION_ENTRY_END_LEVEL_8: return "EndLevel8";
    case ISLE_APPLICATION_ENTRY_CREDITS: return "Credits";
    case ISLE_APPLICATION_ENTRY_PRINTER_DIAGNOSTIC: return "PrinterDiagnostic";
    }
    return "invalid";
}
const char* load_name(save::GameLoadState state) noexcept {
    switch (state) {
    case save::GameLoadState::ready: return "ready";
    case save::GameLoadState::initialized_missing: return "initialized_missing";
    case save::GameLoadState::recovery_required: return "recovery_required";
    case save::GameLoadState::conflict: return "conflict";
    case save::GameLoadState::io_error: return "io_error";
    case save::GameLoadState::invalid_state: return "invalid_state";
    }
    return "invalid";
}
struct SaveFacts {
    bool available = false, fields_valid = false, mew = false;
    std::uint64_t generation = 0;
    unsigned gear_mask = 0, sign_mask = 0, ordinary_count = 0, non_mew_count = 0;
};
SaveFacts save_facts(const save::GameSnapshot& snapshot) noexcept {
    SaveFacts result;
    if (!snapshot) return result;
    result.available = true;
    result.generation = snapshot.generation();
    result.fields_valid = bool(snapshot.image());
    constexpr unsigned flags[] = {0, 1, 2, 5};
    for (unsigned i = 0; i != 4; ++i) {
        const auto value = snapshot.player_flag(flags[i]);
        result.fields_valid = result.fields_valid && bool(value);
        if (value && value.value) result.gear_mask |= 1u << i;
    }
    // Presence is the public canonical photo sentinel, not a score predicate.
    for (std::size_t slot = 0; slot != save::game_report_count; ++slot) {
        const auto value = snapshot.photo_present(save::PhotoSet::report, slot);
        result.fields_valid = result.fields_valid && bool(value);
        if (!value || !value.value) continue;
        if (slot < 63) ++result.ordinary_count;
        if (slot < 62) ++result.non_mew_count;
        if (slot == 62) result.mew = true;
        if (slot >= 63) result.sign_mask |= 1u << (slot - 63);
    }
    return result;
}
bool terminal(const SaveFacts& facts) noexcept {
    return facts.available && facts.fields_valid && facts.generation && facts.gear_mask == 15 &&
        facts.sign_mask == 63 && facts.mew;
}
struct BootFacts {
    bool available = false, fresh = false, ready_image = false;
    save::GameLoadState state = save::GameLoadState::io_error;
    save::LoadState store_state = save::LoadState::recovery_required;
    std::uint64_t generation = 0;
    std::size_t bytes = 0;
    bool orphan_temporary = false;
    save::StoreError error{};
    std::array<save::SlotResult, 2> slots{};
};
BootFacts boot_facts(const application::BootLoadPublication& publication) noexcept {
    BootFacts facts;
    facts.available = true;
    facts.state = publication.state;
    facts.store_state = publication.load.state;
    facts.orphan_temporary = publication.load.orphan_temporary;
    facts.error = publication.load.error;
    facts.slots = publication.load.slots;
    facts.generation = publication.load.snapshot.generation();
    facts.bytes = publication.load.snapshot.image().bytes().size();
    facts.fresh = publication.state == save::GameLoadState::initialized_missing &&
        publication.load.state == save::LoadState::missing && !publication.load.snapshot &&
        !facts.orphan_temporary && !facts.error;
    for (const auto& slot : facts.slots)
        facts.fresh = facts.fresh && slot.state == save::SlotState::missing &&
            !slot.generation && !slot.error;
    facts.ready_image = publication.state == save::GameLoadState::ready &&
        publication.load.state == save::LoadState::ready && bool(publication.load.snapshot) &&
        facts.generation && facts.bytes == save::image_bytes && !facts.error && !facts.orphan_temporary;
    return facts;
}

#if defined(_WIN32)
using Handle = HANDLE;
const Handle bad_handle = INVALID_HANDLE_VALUE;
#else
using Handle = int;
constexpr Handle bad_handle = -1;
#endif
void close_handle(Handle handle) noexcept {
    if (handle == bad_handle) return;
#if defined(_WIN32)
    CloseHandle(handle);
#else
    ::close(handle);
#endif
}
bool checked_close(Handle& handle) noexcept {
    const auto old = std::exchange(handle, bad_handle);
    if (old == bad_handle) return false;
#if defined(_WIN32)
    return CloseHandle(old) != 0;
#else
    return ::close(old) == 0;
#endif
}
bool write_bytes(Handle handle, const void* data, std::size_t size) noexcept {
    auto* bytes = static_cast<const std::byte*>(data);
    while (size) {
#if defined(_WIN32)
        DWORD written = 0;
        if (!WriteFile(handle, bytes, static_cast<DWORD>(size), &written, nullptr) || !written) return false;
#else
        const auto written = ::write(handle, bytes, size);
        if (written < 0) { if (errno == EINTR) continue; return false; }
        if (!written) return false;
#endif
        bytes += written;
        size -= written;
    }
    return true;
}
bool flush_file(Handle handle) noexcept {
#if defined(_WIN32)
    return FlushFileBuffers(handle) != 0;
#else
    return fsync(handle) == 0;
#endif
}
// Like Snap Station, pin the selected directory and exclusively create private
// regular outputs. This is local cooperative filesystem custody, not an OS seal.
struct Output {
    Handle directory = bad_handle;
    std::array<Handle, 3> files{bad_handle, bad_handle, bad_handle};
    std::filesystem::path path;
    ~Output() {
        for (auto file : files) close_handle(file);
        close_handle(directory);
    }
    bool acquire(const std::filesystem::path& requested) {
        path = requested;
        // A trailing separator makes POSIX follow a directory symlink before
        // O_NOFOLLOW checks the final component. Preserve roots, not separators.
        while (path.has_relative_path() && path.filename().empty())
            path = path.parent_path();
#if defined(_WIN32)
        directory = CreateFileW(path.c_str(), FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
        BY_HANDLE_FILE_INFORMATION info{};
        return directory != bad_handle && GetFileInformationByHandle(directory, &info) &&
            (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            !(info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT);
#else
        directory = open(path.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
        return directory != bad_handle;
#endif
    }
    bool absent(const char* name) const {
#if defined(_WIN32)
        const auto attributes = GetFileAttributesW((path / name).c_str());
        if (attributes != INVALID_FILE_ATTRIBUTES) return false;
        return GetLastError() == ERROR_FILE_NOT_FOUND;
#else
        struct stat info{};
        return fstatat(directory, name, &info, AT_SYMLINK_NOFOLLOW) != 0 && errno == ENOENT;
#endif
    }
    bool create(unsigned index, const char* name) {
#if defined(_WIN32)
        files[index] = CreateFileW((path / name).c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
        if (files[index] == bad_handle) return false;
        BY_HANDLE_FILE_INFORMATION info{};
        return GetFileType(files[index]) == FILE_TYPE_DISK &&
            GetFileInformationByHandle(files[index], &info) &&
            !(info.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT));
#else
        files[index] = openat(directory, name, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600);
        if (files[index] == bad_handle) return false;
        struct stat info{};
        return fstat(files[index], &info) == 0 && S_ISREG(info.st_mode);
#endif
    }
    bool finish(unsigned index) noexcept {
        return flush_file(files[index]) && checked_close(files[index]);
    }
    bool finish_directory() noexcept {
#if defined(_WIN32)
        return true; // Win32 file flush acknowledgement, not Linux directory fsync.
#else
        return fsync(directory) == 0;
#endif
    }
};
struct Json {
    Handle file;
    std::array<char, 8192> buffer{};
    std::size_t used = 0, total = 0;
    bool good = true;
    bool flush() noexcept {
        if (good && used) good = write_bytes(file, buffer.data(), used);
        used = 0;
        return good;
    }
    void text(std::string_view value) noexcept {
        if (!good || value.size() > maximum_report_bytes - total) { good = false; return; }
        total += value.size();
        while (good && !value.empty()) {
            const auto size = std::min(value.size(), buffer.size() - used);
            std::memcpy(buffer.data() + used, value.data(), size);
            used += size;
            value.remove_prefix(size);
            if (used == buffer.size()) flush();
        }
    }
    void number(std::uint64_t value) noexcept {
        std::array<char, 32> digits{};
        const auto result = std::to_chars(digits.data(), digits.data() + digits.size(), value);
        if (result.ec != std::errc{}) { good = false; return; }
        text({digits.data(), static_cast<std::size_t>(result.ptr - digits.data())});
    }
    void signed_number(std::int32_t value) noexcept {
        std::array<char, 32> digits{};
        const auto result = std::to_chars(digits.data(), digits.data() + digits.size(), value);
        if (result.ec != std::errc{}) { good = false; return; }
        text({digits.data(), static_cast<std::size_t>(result.ptr - digits.data())});
    }
    void boolean(bool value) noexcept { text(value ? "true" : "false"); }
    void string(const char* value) noexcept {
        if (!value) { text("null"); return; }
        text("\"");
        for (; *value; ++value) {
            if (*value == '"' || *value == '\\') text("\\");
            const char c = *value;
            if (static_cast<unsigned char>(c) < 32) { text("?"); continue; }
            text({&c, 1});
        }
        text("\"");
    }
    void field(const char* name, std::uint64_t value) noexcept { text(name); number(value); }
    void flag(const char* name, bool value) noexcept { text(name); boolean(value); }
};
void emit_save(Json& json, const SaveFacts& facts) noexcept {
    json.flag("{\"available\":", facts.available);
    json.flag(",\"fields_valid\":", facts.fields_valid);
    json.field(",\"generation\":", facts.generation);
    json.field(",\"gear_flags_0_1_2_5_mask\":", facts.gear_mask);
    json.field(",\"sign_slots_63_68_mask\":", facts.sign_mask);
    json.flag(",\"mew_slot_62\":", facts.mew);
    json.field(",\"ordinary_slots_0_62_count\":", facts.ordinary_count);
    json.field(",\"non_mew_slots_0_61_count\":", facts.non_mew_count);
    json.flag(",\"terminal_inventory\":", terminal(facts));
    json.text("}");
}
void emit_boot(Json& json, const BootFacts& facts) noexcept {
    json.flag("{\"available\":", facts.available);
    json.text(",\"game_load_state\":"); json.string(load_name(facts.state));
    json.field(",\"store_load_state\":", static_cast<unsigned>(facts.store_state));
    json.flag(",\"genuine_initialized_missing\":", facts.fresh);
    json.flag(",\"ready_with_own_canonical_image\":", facts.ready_image);
    json.field(",\"generation\":", facts.generation);
    json.field(",\"bytes\":", facts.bytes);
    json.flag(",\"orphan_temporary\":", facts.orphan_temporary);
    json.field(",\"store_error\":", static_cast<unsigned>(facts.error.code));
    json.field(",\"store_error_phase\":", static_cast<unsigned>(facts.error.phase));
    json.field(",\"native_error\":", facts.error.native_error);
    json.text(",\"slots\":[");
    for (std::size_t i = 0; i != facts.slots.size(); ++i) {
        if (i) json.text(",");
        const auto& slot = facts.slots[i];
        json.field("{\"state\":", static_cast<unsigned>(slot.state));
        json.field(",\"generation\":", slot.generation);
        json.field(",\"error\":", static_cast<unsigned>(slot.error.code));
        json.field(",\"error_phase\":", static_cast<unsigned>(slot.error.phase));
        json.field(",\"native_error\":", slot.error.native_error);
        json.text("}");
    }
    json.text("]}");
}
} // namespace

struct Recorder::Impl {
    struct GameRow {
        FrameIdentity identity{};
        std::uint64_t frames = 0, first_frame = 0, last_frame = 0;
        bool retired = false;
        std::int32_t result = 0;
        SaveFacts save{};
        BootFacts boot{};
    };
    struct MainRow {
        FrameIdentity identity{};
        bool used = false;
        std::uint64_t queued = 0, completed = 0, declined = 0, cancelled = 0;
        std::uint64_t replaced = 0, dropped = 0, failed = 0;
        std::uint64_t first_offer = 0, last_offer = 0, last_queued_frame = 0;
        std::uint64_t first_completed_frame = 0, last_completed_frame = 0;
    };
    struct GameEpoch {
        std::uint64_t id = 0;
        bool observed_retirement = false;
        EpochRetirement retirement{};
        std::uint64_t last_generation = 0, publications = 0;
    };
    struct MainEpoch {
        std::uint64_t id = 0;
        bool shutdown_observed = false, cancel_succeeded = false;
        game_host::Shutdown shutdown{};
    };
    struct GamePartition {
        std::array<GameRow, boundary_capacity> rows{};
        std::array<GameEpoch, boundary_capacity> epochs{};
        std::size_t row_count = 0, epoch_count = 0;
        std::uint64_t last_unrecorded_epoch = 0;
        const char* failure = nullptr;
        save::GameSnapshot latest_save{};
        std::uint32_t latest_save_row = no_row, latest_boot_row = no_row;
        application::BootLoadPublication latest_boot{};
        std::uint64_t publications = 0;
        std::uint32_t first_durable_mew_row = no_row;
        std::uint64_t first_durable_mew_generation = 0;
    } game;
    struct MainPartition {
        std::array<MainRow, boundary_capacity> rows{};
        std::array<MainEpoch, boundary_capacity> epochs{};
        std::size_t high_row = 0, epoch_count = 0;
        std::uint64_t overflow_epoch = 0;
        bool overflow_shutdown = false, overflow_missing_shutdown = false;
        const char* failure = nullptr;
        FrameIdentity pending{};
        bool has_pending = false, pending_credit_eligible = false;
        std::uint64_t pending_offer = 0, last_offer = 0;
        std::uint64_t idle = 0, uncorrelated_completed = 0, failed_cancel = 0;
    } main;
    Purpose purpose;
    bool worker_joined = false;
    explicit Impl(Purpose selected) noexcept : purpose(selected) {}

    GameEpoch* game_epoch(std::uint64_t id) noexcept {
        if (game.epoch_count && game.epochs[game.epoch_count - 1].id == id)
            return &game.epochs[game.epoch_count - 1];
        if (!id || (game.epoch_count && id <= game.epochs[game.epoch_count - 1].id)) {
            remember(game.failure, "GAME epoch order invalid"); return nullptr;
        }
        if (game.epoch_count == boundary_capacity) {
            game.last_unrecorded_epoch = std::max(game.last_unrecorded_epoch, id);
            remember(game.failure, "GAME epoch capacity exhausted"); return nullptr;
        }
        auto& epoch = game.epochs[game.epoch_count++]; epoch.id = id;
        // A new application epoch cannot inherit a previous profile's payload.
        game.latest_save = {};
        game.latest_boot = {};
        game.latest_save_row = no_row;
        game.latest_boot_row = no_row;
        return &epoch;
    }
    MainEpoch* main_epoch(std::uint64_t id) noexcept {
        if (main.epoch_count && main.epochs[main.epoch_count - 1].id == id)
            return &main.epochs[main.epoch_count - 1];
        if (!id || (main.epoch_count && id <= main.epochs[main.epoch_count - 1].id)) {
            remember(main.failure, "Host epoch order invalid"); return nullptr;
        }
        if (main.epoch_count == boundary_capacity) {
            if (id != main.overflow_epoch) {
                if (main.overflow_epoch && !main.overflow_shutdown)
                    main.overflow_missing_shutdown = true;
                if (id < main.overflow_epoch) main.overflow_missing_shutdown = true;
                main.overflow_epoch = std::max(main.overflow_epoch, id);
                main.overflow_shutdown = false;
            }
            remember(main.failure, "Host epoch capacity exhausted"); return nullptr;
        }
        auto& epoch = main.epochs[main.epoch_count++]; epoch.id = id;
        return &epoch;
    }
    GameRow* game_row(std::uint64_t epoch_id, const EntryIntent& entry) noexcept {
        auto* epoch = game_epoch(epoch_id);
        if (!epoch || !valid_entry(entry)) {
            remember(game.failure, "GAME entry identity invalid"); return nullptr;
        }
        if (epoch->observed_retirement) {
            remember(game.failure, "GAME entry after epoch retirement"); return nullptr;
        }
        if (game.row_count) {
            auto& previous = game.rows[game.row_count - 1];
            if (previous.identity.application_epoch == epoch_id) {
                if (same_entry(previous.identity.entry, entry)) return &previous;
                if (entry.serial <= previous.identity.entry.serial || !previous.retired) {
                    remember(game.failure, "GAME entry order or retirement missing");
                    // Still retain the new boundary: failed evidence never controls GAME.
                }
            }
        }
        if (game.row_count == boundary_capacity) {
            remember(game.failure, "GAME boundary capacity exhausted"); return nullptr;
        }
        const auto index = static_cast<std::uint32_t>(game.row_count++);
        auto& row = game.rows[index];
        row.identity = {epoch_id, entry, 0, index};
        return &row;
    }
    bool readable() const noexcept {
        if (!worker_joined) return false;
        // Exhaustion declines detail, not the real postclosure output barrier.
        // Multiple unclosed overflow epochs cannot be reconciled exactly and
        // therefore keep output unavailable rather than infer their closure.
        if (main.overflow_missing_shutdown || (main.overflow_epoch && !main.overflow_shutdown) ||
            (game.last_unrecorded_epoch && (main.overflow_epoch < game.last_unrecorded_epoch || !main.overflow_shutdown)))
            return false;
        for (std::size_t i = 0; i != main.epoch_count; ++i)
            if (!main.epochs[i].shutdown_observed) return false;
        std::size_t host = 0;
        for (std::size_t i = 0; i != game.epoch_count; ++i) {
            while (host < main.epoch_count && main.epochs[host].id < game.epochs[i].id) ++host;
            if (host == main.epoch_count || main.epochs[host].id != game.epochs[i].id ||
                !main.epochs[host].shutdown_observed) return false;
        }
        return true;
    }
    struct Summary {
        bool evidence_valid = true, component_closure = true, native = false;
        bool fresh_boot = false, loaded_boot = false, initial_title = false, name = false, oak = false;
        bool continued_oak = false, terminal_save = false, credits = false, active_final_title = false;
        unsigned course_mask = 0;
        std::uint64_t epoch = 0, final_title_completed = 0;
        std::uint32_t boot_row = no_row, credits_row = no_row, title_row = no_row;
        const char* failure = nullptr;
    };
    Summary analyse() const noexcept {
        Summary summary;
        summary.evidence_valid = !game.failure && !main.failure;
        summary.failure = game.failure ? game.failure : main.failure;
        auto invalidate = [&](const char* message) {
            summary.evidence_valid = false; remember(summary.failure, message);
        };
        if (!game.epoch_count || game.epoch_count != main.epoch_count) summary.component_closure = false;
        for (std::size_t i = 0; i != game.epoch_count; ++i) {
            const auto& epoch = game.epochs[i];
            const auto& end = epoch.retirement;
            if (!epoch.observed_retirement || !end.successful || !end.runtime_retired || !end.input_stopped ||
                !end.audio_drained || !end.driver_retired) summary.component_closure = false;
            if (i >= main.epoch_count || main.epochs[i].id != epoch.id) {
                summary.component_closure = false; continue;
            }
            const auto& host = main.epochs[i];
            if (!host.shutdown_observed || !host.cancel_succeeded || !host.shutdown.owners_released ||
                host.shutdown.failed || host.shutdown.audio != audio::SdlAudioOutput::CloseDisposition::stream_drained)
                summary.component_closure = false;
        }
        if (main.has_pending) summary.component_closure = false;
        for (std::size_t i = 0; i != main.high_row; ++i) {
            const auto& host = main.rows[i];
            if (!host.used) continue;
            if (i >= game.row_count || !same_row(host.identity, game.rows[i].identity)) {
                invalidate("GAME/Host row identity mismatch"); continue;
            }
            const auto& row = game.rows[i];
            if (!row.frames || host.last_queued_frame > row.last_frame ||
                host.identity.frame_ordinal < row.first_frame || host.queued > row.frames || host.completed > host.queued)
                invalidate("Host frame correlation outside GAME row");
        }
        // Qualification is local to the terminal epoch. Earlier/reset epochs
        // remain explicit diagnostics; their masks/results cannot fill its gaps.
        if (game.epoch_count) summary.epoch = game.epochs[game.epoch_count - 1].id;
        if (main.epoch_count) summary.epoch = std::max(summary.epoch, main.epochs[main.epoch_count - 1].id);
        summary.epoch = std::max(summary.epoch, std::max(game.last_unrecorded_epoch, main.overflow_epoch));
        if (game.row_count) {
            unsigned stage = 0;
            std::uint32_t terminal_row = no_row;
            for (std::size_t i = 0; i != game.row_count; ++i) {
                const auto& row = game.rows[i];
                if (row.identity.application_epoch != summary.epoch) continue;
                const auto& entry = row.identity.entry;
                const auto& host = main.rows[i];
                const bool shown = host.used && host.completed && same_row(host.identity, row.identity);
                const bool retired_shown = row.retired && shown;
                if (stage == 0) {
                    if (entry.operation == ISLE_APPLICATION_ENTRY_BOOT && retired_shown && row.result == 0) {
                        summary.fresh_boot = row.boot.fresh;
                        summary.loaded_boot = row.boot.ready_image;
                        summary.boot_row = static_cast<std::uint32_t>(i);
                        stage = 1;
                    } else stage = 99;
                } else if (stage == 1) {
                    if (entry.operation == ISLE_APPLICATION_ENTRY_TITLE && entry.argument == 8 && retired_shown &&
                        row.result == (purpose == Purpose::cold_boot ? 11 : 0)) {
                        summary.initial_title = true; stage = 2;
                    } else stage = 99;
                } else if (stage == 2) {
                    if (purpose == Purpose::cold_boot) {
                        if (entry.operation == ISLE_APPLICATION_ENTRY_OAKS_LAB && entry.argument == 11 && shown)
                            summary.continued_oak = true;
                        stage = 3;
                    } else if (entry.operation == ISLE_APPLICATION_ENTRY_NAME && entry.argument == 0 &&
                        retired_shown && row.result == 11) {
                        summary.name = true; stage = 3;
                    } else stage = 99;
                } else if (stage == 3 && purpose == Purpose::campaign) {
                    if (entry.operation == ISLE_APPLICATION_ENTRY_OAKS_LAB && entry.argument == 11 &&
                        retired_shown) {
                        summary.oak = true; stage = 4;
                    } else stage = 99;
                }
                if (stage != 4 && purpose == Purpose::campaign) continue;
                if (retired_shown && entry.operation >= ISLE_APPLICATION_ENTRY_BEACH &&
                    entry.operation <= ISLE_APPLICATION_ENTRY_RAINBOW) {
                    const auto course = static_cast<unsigned>(entry.operation - ISLE_APPLICATION_ENTRY_BEACH);
                    if (entry.argument == course && (row.result == 9 || row.result == 13))
                        summary.course_mask |= 1u << course;
                }
                if (row.save.available) {
                    summary.terminal_save = terminal(row.save);
                    terminal_row = static_cast<std::uint32_t>(i);
                }
                if (retired_shown && entry.operation == ISLE_APPLICATION_ENTRY_CREDITS && entry.argument == 18 &&
                    row.result == 8 && summary.terminal_save && terminal_row != no_row && terminal_row < i) {
                    summary.credits = true;
                    summary.credits_row = static_cast<std::uint32_t>(i);
                }
                if (i + 1 == game.row_count && entry.operation == ISLE_APPLICATION_ENTRY_TITLE && entry.argument == 8 &&
                    !row.retired && summary.credits && summary.credits_row < i) {
                    summary.active_final_title = true;
                    summary.title_row = static_cast<std::uint32_t>(i);
                    summary.final_title_completed = shown ? host.completed : 0;
                }
            }
        }
        bool terminal_epoch_closed = false;
        if (game.epoch_count && game.epochs[game.epoch_count - 1].id == summary.epoch) {
            const auto& end = game.epochs[game.epoch_count - 1].retirement;
            terminal_epoch_closed = !end.reset_requested && !end.disconnect_requested;
        }
        const bool save_matches_terminal_epoch = game.latest_save_row != no_row &&
            game.rows[game.latest_save_row].identity.application_epoch == summary.epoch &&
            terminal(save_facts(game.latest_save));
        const bool boot_matches_terminal_epoch = game.latest_boot_row != no_row &&
            game.rows[game.latest_boot_row].identity.application_epoch == summary.epoch;
        summary.native = summary.evidence_valid && summary.component_closure && terminal_epoch_closed &&
            (purpose == Purpose::campaign ?
                summary.fresh_boot && summary.initial_title && summary.name && summary.oak &&
                summary.course_mask == 127 && summary.terminal_save && save_matches_terminal_epoch && summary.credits &&
                summary.active_final_title && summary.final_title_completed >= 120 :
                purpose == Purpose::cold_boot && summary.loaded_boot && boot_matches_terminal_epoch &&
                summary.initial_title && summary.continued_oak);
        if (!summary.failure && !summary.component_closure) summary.failure = "actual component closure incomplete";
        if (!summary.failure && !summary.native) summary.failure = "requested native boundary predicate incomplete";
        return summary;
    }
};

Recorder::Recorder(Purpose purpose) {
    if (purpose != Purpose::disabled) impl_ = std::make_unique<Impl>(purpose);
}
Recorder::~Recorder() = default;
FrameIdentity Recorder::game_frame(std::uint64_t epoch, const EntryIntent& entry, std::uint64_t ordinal) noexcept {
    FrameIdentity identity{epoch, entry, ordinal, no_row};
    if (!impl_) return identity;
    auto& state = *impl_;
    auto* row = state.game_row(epoch, entry);
    if (!row) return identity;
    if (!ordinal || row->retired || (row->frames && ordinal <= row->last_frame)) {
        remember(state.game.failure, "GAME completed frame ordinal invalid"); return identity;
    }
    if (!row->frames) row->first_frame = ordinal;
    row->last_frame = ordinal;
    count(row->frames, state.game.failure);
    identity.row = row->identity.row;
    return identity;
}
void Recorder::game_retired(std::uint64_t epoch, const application::SceneRetirementView& view) noexcept {
    if (!impl_) return;
    auto& state = *impl_;
    if (!view.entry) { remember(state.game.failure, "retirement omitted entry identity"); return; }
    auto* row = state.game_row(epoch, *view.entry);
    if (!row) return;
    if (row->retired) { remember(state.game.failure, "duplicate entry retirement"); return; }
    row->retired = true;
    row->result = view.source_result;
    if (view.published_save) {
        row->save = save_facts(view.published_save);
        auto& evidence = state.game;
        auto& epoch_evidence = evidence.epochs[evidence.epoch_count - 1];
        const bool valid_publication = view.entry->operation == ISLE_APPLICATION_ENTRY_OAKS_LAB &&
            row->save.fields_valid && row->save.generation && row->save.generation > epoch_evidence.last_generation;
        if (!valid_publication)
            remember(evidence.failure, "retired Save publication generation or origin invalid");
        epoch_evidence.last_generation = row->save.generation;
        count(epoch_evidence.publications, evidence.failure);
        count(evidence.publications, evidence.failure);
        evidence.latest_save = view.published_save;
        evidence.latest_save_row = row->identity.row;
        if (valid_publication && row->save.mew && evidence.first_durable_mew_row == no_row) {
            evidence.first_durable_mew_row = row->identity.row;
            evidence.first_durable_mew_generation = row->save.generation;
        }
    }
    if (view.boot_load) {
        row->boot = boot_facts(*view.boot_load);
        if (view.entry->operation != ISLE_APPLICATION_ENTRY_BOOT)
            remember(state.game.failure, "Boot publication on non-Boot retirement");
        state.game.latest_boot = *view.boot_load;
        state.game.latest_boot_row = row->identity.row;
    }
}
void Recorder::game_epoch_retired(const EpochRetirement& retirement) noexcept {
    if (!impl_) return;
    auto* epoch = impl_->game_epoch(retirement.application_epoch);
    if (!epoch) return;
    if (epoch->observed_retirement) {
        remember(impl_->game.failure, "duplicate GAME epoch retirement"); return;
    }
    epoch->observed_retirement = true;
    epoch->retirement = retirement;
}
void Recorder::main_queued(const FrameIdentity& identity, std::uint64_t offer) noexcept {
    if (!impl_) return;
    auto& state = *impl_;
    auto& main = state.main;
    if (main.has_pending) {
        count(main.rows[main.pending.row].replaced, main.failure);
        main.has_pending = false;
    }
    if (!state.main_epoch(identity.application_epoch) || identity.row >= boundary_capacity ||
        !valid_entry(identity.entry) || !identity.frame_ordinal || !offer || offer <= main.last_offer) {
        remember(main.failure, "queued Host frame identity or offer order invalid"); return;
    }
    if (main.epochs[main.epoch_count - 1].shutdown_observed) {
        remember(main.failure, "queued frame after Host shutdown"); return;
    }
    main.last_offer = offer;
    auto& row = main.rows[identity.row];
    main.high_row = std::max(main.high_row, std::size_t(identity.row) + 1);
    if (!row.used) { row.used = true; row.identity = identity; row.first_offer = offer; }
    else if (!same_row(row.identity, identity) || identity.frame_ordinal <= row.last_queued_frame) {
        remember(main.failure, "Host row collision or repeated frame ordinal"); return;
    }
    row.last_queued_frame = identity.frame_ordinal;
    row.last_offer = offer;
    count(row.queued, main.failure);
    main.pending = identity;
    main.pending_offer = offer;
    main.pending_credit_eligible = true;
    main.has_pending = true;
}
void Recorder::main_presented(game_host::Present result, bool still_pending) noexcept {
    if (!impl_) return;
    auto& main = impl_->main;
    if (!main.has_pending) {
        if (result == game_host::Present::completed) {
            count(main.uncorrelated_completed, main.failure);
            remember(main.failure, "Host completion without queued identity");
        } else if (result == game_host::Present::failed)
            remember(main.failure, "Host presentation failed without queued identity");
        else if (result == game_host::Present::idle) count(main.idle, main.failure);
        if (still_pending) remember(main.failure, "Host pending without queued identity");
        return;
    }
    auto& row = main.rows[main.pending.row];
    switch (result) {
    case game_host::Present::completed:
        if (main.pending_credit_eligible) {
            count(row.completed, main.failure);
            if (!row.first_completed_frame) row.first_completed_frame = main.pending.frame_ordinal;
            row.last_completed_frame = main.pending.frame_ordinal;
        } else count(row.dropped, main.failure);
        main.has_pending = false;
        if (still_pending) remember(main.failure, "completed Host presentation still pending");
        return;
    case game_host::Present::declined:
        count(row.declined, main.failure); break;
    case game_host::Present::failed:
        count(row.failed, main.failure);
        main.pending_credit_eligible = false;
        remember(main.failure, "Host presentation failed"); break;
    case game_host::Present::idle:
        count(main.idle, main.failure); break;
    }
    if (!still_pending) {
        count(row.dropped, main.failure);
        main.has_pending = false;
    }
}
void Recorder::main_cancelled(bool succeeded) noexcept {
    if (!impl_) return;
    auto& main = impl_->main;
    if (!succeeded) {
        count(main.failed_cancel, main.failure);
        remember(main.failure, "Host presentation cancellation failed"); return;
    }
    if (main.has_pending) {
        count(main.rows[main.pending.row].cancelled, main.failure);
        main.has_pending = false;
    }
}
void Recorder::main_host_shutdown(std::uint64_t epoch_id, bool cancel_succeeded,
                                  const game_host::Shutdown& shutdown) noexcept {
    if (!impl_) return;
    main_cancelled(cancel_succeeded);
    auto* epoch = impl_->main_epoch(epoch_id);
    if (!epoch) {
        if (impl_->main.overflow_epoch == epoch_id) impl_->main.overflow_shutdown = true;
        return;
    }
    if (epoch->shutdown_observed) {
        remember(impl_->main.failure, "duplicate Host epoch shutdown"); return;
    }
    epoch->shutdown_observed = true;
    epoch->cancel_succeeded = cancel_succeeded;
    epoch->shutdown = shutdown;
}
void Recorder::joined() noexcept { if (impl_) impl_->worker_joined = true; }
bool Recorder::readable() const noexcept { return impl_ && impl_->readable(); }
bool Recorder::native_complete() const noexcept { return readable() && impl_->analyse().native; }
const char* Recorder::message() const noexcept {
    if (!impl_) return "terminal evidence disabled";
    if (!readable()) return "terminal evidence unavailable before GAME join and all Host shutdown observations";
    const auto summary = impl_->analyse();
    return summary.failure ? summary.failure : "native boundary complete; independent pixels and external custody still required";
}

bool Recorder::write(const std::filesystem::path& directory, std::array<char, 512>& error) const noexcept {
    error.fill(0);
    auto fail = [&](const char* message) {
        std::snprintf(error.data(), error.size(), "%s", message); return false;
    };
    if (!readable()) return fail(message());
    try {
        const auto& state = *impl_;
        const auto summary = state.analyse();
        Output output;
        if (!output.acquire(directory)) return fail("cannot acquire existing non-symlink terminal output directory");
        constexpr const char* names[] = {"report.json", "terminal-save.bin", "boot-load.bin"};
        // Refuse all reserved destinations, even if this run lacks their payload.
        for (const auto* name : names)
            if (!output.absent(name)) return fail("terminal output destination already exists or cannot be examined");
        const bool save_matches_epoch = state.game.latest_save_row != no_row &&
            state.game.rows[state.game.latest_save_row].identity.application_epoch == summary.epoch;
        const bool boot_matches_epoch = state.game.latest_boot_row != no_row &&
            state.game.rows[state.game.latest_boot_row].identity.application_epoch == summary.epoch;
        const auto save_bytes = save_matches_epoch ? state.game.latest_save.image().bytes() : std::span<const std::byte>{};
        const auto boot_bytes = boot_matches_epoch ? state.game.latest_boot.load.snapshot.image().bytes() : std::span<const std::byte>{};
        if ((!save_bytes.empty() && save_bytes.size() != save::image_bytes) ||
            (!boot_bytes.empty() && boot_bytes.size() != save::image_bytes))
            return fail("immutable canonical terminal payload extent invalid");
        // Reserve the report first; it remains empty on a binary-output failure.
        // Never remove partial evidence or overwrite an existing destination.
        if (!output.create(0, names[0])) return fail("cannot exclusively create terminal report");
        if (!save_bytes.empty() && (!output.create(1, names[1]) ||
            !write_bytes(output.files[1], save_bytes.data(), save_bytes.size()) || !output.finish(1)))
            return fail("cannot publish whole immutable terminal Save image");
        if (!boot_bytes.empty() && (!output.create(2, names[2]) ||
            !write_bytes(output.files[2], boot_bytes.data(), boot_bytes.size()) || !output.finish(2)))
            return fail("cannot publish whole immutable completed Boot load image");
        Json json{output.files[0]};
        json.text("{\n\"schema\":\"35mmisle.shipping-terminal.v1\",\n\"purpose\":");
        json.string(purpose_name(state.purpose));
        json.flag(",\n\"native_boundary_complete\":", summary.native);
        json.flag(",\n\"evidence_valid\":", summary.evidence_valid);
        json.text(",\n\"failure\":"); json.string(summary.failure);
        json.text(",\n\"provenance\":{\"path\":\"shipping GameOwner passive observation\","
            "\"scene_inputs\":\"copied EntryIntent and actual completed retirement result only\","
            "\"presentation_credit\":\"actual Host Present::completed on original pending epoch/entry/frame/offer\","
            "\"save_inputs\":\"owning immutable retired Oak publications; not an exhaustive every-ACK ledger\","
            "\"boot_inputs\":\"owning initialization output copied only at completed Boot retirement\","
            "\"controller_feedback\":false,\"live_profile_reads\":false,\"automatic_stop\":false},"
            "\n\"external_requirements\":{\"independent_public_pixels\":true,"
            "\"temporal_Mew_acquisition_and_40_to_61_ordinary_before_63\":\"not proved natively; public pixels required\","
            "\"first_durable_Mew_publication_is_acquisition\":false,"
            "\"cold_boot_visible_progress_full_profile_and_byte_equality\":\"external review required\","
            "\"OS_wait_ECHILD_EOF_sealed_FD_lease_whole_tree_and_Parent_closure\":\"not observed natively\"},");
        json.flag("\n\"closure\":{\"worker_join_observed\":", state.worker_joined);
        json.flag(",\"all_admitted_Host_shutdowns_observed\":", state.readable());
        json.flag(",\"actual_component_closure\":", summary.component_closure);
        json.flag(",\"pending_identity_remains\":", state.main.has_pending);
        json.text("},\n\"predicate\":{");
        json.field("\"terminal_epoch\":", summary.epoch);
        json.flag(",\"fresh_completed_Boot\":", summary.fresh_boot);
        json.flag(",\"completed_Boot_ready_with_own_image\":", summary.loaded_boot);
        json.flag(",\"matching_initial_Title_result\":", summary.initial_title);
        json.flag(",\"matching_Name0_result11\":", summary.name);
        json.flag(",\"matching_first_completed_Oak11\":", summary.oak);
        json.flag(",\"continued_Oak11_presented\":", summary.continued_oak);
        json.field(",\"naturally_retired_presented_course_mask\":", summary.course_mask);
        json.flag(",\"terminal_immutable_inventory\":", summary.terminal_save);
        json.flag(",\"presented_Credits18_retired_result8_after_terminal_Save\":", summary.credits);
        json.flag(",\"later_correlated_active_final_Title8\":", summary.active_final_title);
        json.field(",\"final_Title_actual_completed_presentations\":", summary.final_title_completed);
        json.field(",\"required_final_Title_presentations\":", 120);
        json.field(",\"boot_row\":", summary.boot_row);
        json.field(",\"credits_row\":", summary.credits_row);
        json.field(",\"final_Title_row\":", summary.title_row);
        json.text("},\n\"publications\":{");
        json.field("\"retired_Save_publications\":", state.game.publications);
        json.text(",\"generation_policy\":\"nonzero strictly increasing observed generations within each application epoch; gaps allowed\"");
        json.field(",\"latest_Save_row\":", state.game.latest_save_row);
        json.field(",\"terminal_Save_epoch\":", save_matches_epoch ? summary.epoch : 0);
        json.field(",\"terminal_Save_generation\":", save_matches_epoch ? state.game.latest_save.generation() : 0);
        json.field(",\"terminal_Save_bytes\":", save_bytes.size());
        json.field(",\"latest_Boot_row\":", state.game.latest_boot_row);
        json.field(",\"Boot_load_epoch\":", boot_matches_epoch ? summary.epoch : 0);
        json.field(",\"Boot_load_generation\":", boot_matches_epoch ? state.game.latest_boot.load.snapshot.generation() : 0);
        json.field(",\"Boot_load_bytes\":", boot_bytes.size());
        json.field(",\"first_durable_Mew_publication_row\":", state.game.first_durable_mew_row);
        json.field(",\"first_durable_Mew_publication_generation\":", state.game.first_durable_mew_generation);
        json.text("},\n\"bounds\":{");
        json.field("\"boundary_capacity\":", boundary_capacity);
        json.field(",\"maximum_report_bytes\":", maximum_report_bytes);
        json.field(",\"GAME_rows\":", state.game.row_count);
        json.field(",\"GAME_epochs\":", state.game.epoch_count);
        json.field(",\"Host_epochs\":", state.main.epoch_count);
        json.field(",\"last_unrecorded_GAME_epoch\":", state.game.last_unrecorded_epoch);
        json.field(",\"overflow_Host_epoch\":", state.main.overflow_epoch);
        json.flag(",\"overflow_Host_shutdown_observed\":", state.main.overflow_shutdown);
        json.flag(",\"unreconciled_overflow_Host_shutdowns\":", state.main.overflow_missing_shutdown);
        json.text(",\"GAME_failure\":"); json.string(state.game.failure);
        json.text(",\"Host_failure\":"); json.string(state.main.failure);
        json.text("},\n\"pending\":{");
        json.flag("\"retained\":", state.main.has_pending);
        json.flag(",\"credit_eligible\":", state.main.has_pending && state.main.pending_credit_eligible);
        json.field(",\"row\":", state.main.pending.row);
        json.field(",\"epoch\":", state.main.pending.application_epoch);
        json.field(",\"entry_serial\":", state.main.pending.entry.serial);
        json.field(",\"frame_ordinal\":", state.main.pending.frame_ordinal);
        json.field(",\"offer_serial\":", state.main.pending_offer);
        json.field(",\"idle_results\":", state.main.idle);
        json.field(",\"uncorrelated_completions\":", state.main.uncorrelated_completed);
        json.field(",\"failed_cancellations\":", state.main.failed_cancel);
        json.text("},\n\"epochs\":[");
        for (std::size_t i = 0; i != state.game.epoch_count; ++i) {
            if (i) json.text(",");
            const auto& epoch = state.game.epochs[i];
            const auto& end = epoch.retirement;
            json.field("\n{\"epoch\":", epoch.id);
            json.flag(",\"GAME_retirement_observed\":", epoch.observed_retirement);
            json.flag(",\"successful\":", end.successful);
            json.flag(",\"reset_requested\":", end.reset_requested);
            json.flag(",\"disconnect_requested\":", end.disconnect_requested);
            json.flag(",\"Runtime_retired\":", end.runtime_retired);
            json.flag(",\"Input_stopped\":", end.input_stopped);
            json.flag(",\"Audio_drained\":", end.audio_drained);
            json.flag(",\"Driver_retired\":", end.driver_retired);
            json.field(",\"retired_Save_publications\":", epoch.publications);
            json.field(",\"last_observed_generation\":", epoch.last_generation);
            json.text("}");
        }
        json.text("\n],\n\"Host_epochs\":[");
        for (std::size_t i = 0; i != state.main.epoch_count; ++i) {
            if (i) json.text(",");
            const auto& epoch = state.main.epochs[i];
            json.field("\n{\"epoch\":", epoch.id);
            json.flag(",\"shutdown_observed\":", epoch.shutdown_observed);
            json.flag(",\"cancel_present_succeeded\":", epoch.cancel_succeeded);
            json.flag(",\"owners_released\":", epoch.shutdown.owners_released);
            json.flag(",\"failed\":", epoch.shutdown.failed);
            json.field(",\"audio_close_disposition\":", static_cast<unsigned>(epoch.shutdown.audio));
            json.text("}");
        }
        json.text("\n],\n\"rows\":[");
        for (std::size_t i = 0; i != state.game.row_count; ++i) {
            if (i) json.text(",");
            const auto& row = state.game.rows[i];
            const auto& host = state.main.rows[i];
            json.field("\n{\"row\":", i);
            json.field(",\"epoch\":", row.identity.application_epoch);
            json.field(",\"entry_serial\":", row.identity.entry.serial);
            json.text(",\"operation\":"); json.string(operation_name(row.identity.entry.operation));
            json.field(",\"operation_code\":", static_cast<unsigned>(row.identity.entry.operation));
            json.field(",\"argument\":", row.identity.entry.argument);
            json.field(",\"GAME_completed_frames\":", row.frames);
            json.field(",\"first_frame_ordinal\":", row.first_frame);
            json.field(",\"last_frame_ordinal\":", row.last_frame);
            json.flag(",\"completed_retirement_observed\":", row.retired);
            json.text(",\"source_result\":");
            if (row.retired) json.signed_number(row.result); else json.text("null");
            json.text(",\"retired_Save_publication\":"); emit_save(json, row.save);
            json.text(",\"completed_Boot_publication\":"); emit_boot(json, row.boot);
            json.flag(",\"Host_row_observed\":", host.used);
            json.flag(",\"Host_row_identity_matches\":", !host.used || same_row(host.identity, row.identity));
            json.field(",\"queued\":", host.queued);
            json.field(",\"actual_completed_presentations\":", host.completed);
            json.field(",\"declined_attempts\":", host.declined);
            json.field(",\"cancelled_pending\":", host.cancelled);
            json.field(",\"replaced_pending\":", host.replaced);
            json.field(",\"dropped_without_completion\":", host.dropped);
            json.field(",\"failed_attempts\":", host.failed);
            json.field(",\"first_offer_serial\":", host.first_offer);
            json.field(",\"last_offer_serial\":", host.last_offer);
            json.field(",\"last_queued_frame_ordinal\":", host.last_queued_frame);
            json.field(",\"first_completed_frame_ordinal\":", host.first_completed_frame);
            json.field(",\"last_completed_frame_ordinal\":", host.last_completed_frame);
            json.text("}");
        }
        json.text("\n]\n}\n");
        if (!json.flush() || !output.finish(0) || !output.finish_directory())
            return fail("bounded terminal report write/flush/close failed; partial outputs retained");
        return true;
    } catch (...) {
        return fail("terminal output allocation/path acquisition failed; partial outputs retained");
    }
}
} // namespace isle::game_run_report
