#include "snap_station.hpp"

#include <array>
#include <atomic>
#include <cerrno>
#include <cstring>
#include <new>
#include <string>
#include <utility>
#if defined(_MSC_VER) && defined(_M_X64)
#include <intrin.h>
#elif defined(__x86_64__)
#include <cpuid.h>
#include <immintrin.h>
#endif
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

namespace isle::snap_station {
namespace {
constexpr std::size_t workspace_size = 0x400000;
constexpr std::size_t frame_size = 640 * 480 * 4;
constexpr std::uint32_t sheet_stride = 2560 * 3;
constexpr std::uint32_t sheet_size = 54 + sheet_stride * 1920;
std::atomic<std::uint64_t> sequence{0};
#if defined(_WIN32)
using Handle = HANDLE;
const Handle bad = INVALID_HANDLE_VALUE;
void close_handle(Handle h) noexcept { if (h != bad) CloseHandle(h); }
#else
using Handle = int;
constexpr Handle bad = -1;
void close_handle(Handle h) noexcept { if (h != bad) ::close(h); }
#endif
bool cache_line(std::size_t& size) noexcept {
#if defined(_MSC_VER) && defined(_M_X64)
    int regs[4]{};
    __cpuid(regs, 1);
    if (!(regs[3] & (1 << 19))) return false;
    size = ((static_cast<unsigned>(regs[1]) >> 8) & 255) * 8;
#elif defined(__x86_64__)
    unsigned a, b, c, d;
    if (!__get_cpuid(1, &a, &b, &c, &d) || !(d & (1u << 19))) return false;
    size = ((b >> 8) & 255) * 8;
#else
    (void)size;
    return false;
#endif
    return size && size <= 4096 && (size & (size - 1)) == 0;
}
bool write_at(Handle h, const void* bytes, std::size_t size, std::uint64_t offset) noexcept {
    auto p = static_cast<const std::byte*>(bytes);
    while (size) {
#if defined(_WIN32)
        LARGE_INTEGER pos{}; pos.QuadPart = static_cast<LONGLONG>(offset);
        DWORD n = 0;
        if (!SetFilePointerEx(h, pos, nullptr, FILE_BEGIN) ||
            !WriteFile(h, p, static_cast<DWORD>(size), &n, nullptr) || !n) return false;
#else
        const auto n = pwrite(h, p, size, static_cast<off_t>(offset));
        if (n < 0) { if (errno == EINTR) continue; return false; }
        if (!n) return false;
#endif
        p += n; size -= n; offset += n;
    }
    return true;
}
bool sync_file(Handle h) noexcept {
#if defined(_WIN32)
    return FlushFileBuffers(h) != 0;
#else
    return fsync(h) == 0;
#endif
}
bool checked_close(Handle& h) noexcept {
    auto old = std::exchange(h, bad);
#if defined(_WIN32)
    return CloseHandle(old) != 0;
#else
    return ::close(old) == 0;
#endif
}
void put32(std::byte* p, std::uint32_t n) noexcept {
    for (unsigned i = 0; i != 4; ++i) p[i] = std::byte((n >> (i * 8)) & 255);
}
}
struct Device::State {
    Handle selected = bad, directory = bad, file = bad, retained = bad;
    bool owns_partial = false;
    std::filesystem::path partial_path;
    std::filesystem::path path;
    std::byte* workspace = nullptr;
    std::size_t line = 0;
    bool ready = false, leased = false, capturing = false, published = false;
    bool reset = false, reset_taken = false, saving = false, saved = false;
    std::uint64_t ticket = 0, application = 0, entry = 0;
    const void* owner = nullptr;
    std::uint32_t pass = 0, count = 0, copies = 0;
    FrameIdentity staged{};
    bool has_frame = false;
    std::uint8_t acknowledgment = 0;
    ~State() {
        close_handle(file); close_handle(retained);
        if (owns_partial && directory != bad) {
#if defined(_WIN32)
            DeleteFileW(partial_path.c_str());
#else
            unlinkat(directory, "sheet.partial", 0);
#endif
        }
        close_handle(directory); close_handle(selected);
        if (workspace) ::operator delete(workspace, std::align_val_t(4096));
    }
    Handle create(const std::filesystem::path& name) {
#if defined(_WIN32)
        return CreateFileW((path / name).c_str(), GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_READ, nullptr, CREATE_NEW,
                           FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
#else
        return openat(directory, name.c_str(), O_RDWR | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600);
#endif
    }
};
Device::Device() noexcept = default;
Device::~Device() = default;
bool Device::fail(Error e, const char* m) noexcept {
    if (error_ == Error::none) { error_ = e; message_ = m; }
    if (state_ && !state_->ready) state_.reset();
    return false;
}
bool Device::connect(const std::filesystem::path& directory) noexcept {
    disconnect(); error_ = Error::none; message_ = "";
    try {
        state_ = std::make_unique<State>();
        auto& s = *state_;
        if (!cache_line(s.line)) return fail(Error::unsupported, "CLFLUSH is unavailable");
#if defined(_WIN32)
        s.selected = CreateFileW(directory.c_str(), FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
        BY_HANDLE_FILE_INFORMATION info{};
        if (s.selected == bad || !GetFileInformationByHandle(s.selected, &info) ||
            !(info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
            (info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
            return fail(Error::io, "Cannot acquire non-reparse output directory");
#else
        s.selected = open(directory.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
        if (s.selected == bad) return fail(Error::io, "Cannot acquire output directory");
#endif
        bool acquired = false;
        for (unsigned attempt = 0; attempt != 128; ++attempt) {
#if defined(_WIN32)
            const auto pid = GetCurrentProcessId();
#else
            const auto pid = getpid();
#endif
            const auto name = "snap-job-" + std::to_string(pid) + "-" + std::to_string(++sequence);
            s.path = directory / name;
#if defined(_WIN32)
            if (!CreateDirectoryW(s.path.c_str(), nullptr)) {
                if (GetLastError() == ERROR_ALREADY_EXISTS) continue;
                return fail(Error::io, "Cannot create owned job directory");
            }
            s.directory = CreateFileW(s.path.c_str(), FILE_LIST_DIRECTORY,
                FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
                FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
            if (s.directory != bad) {
                BY_HANDLE_FILE_INFORMATION job_info{};
                if (!GetFileInformationByHandle(s.directory, &job_info) ||
                    !(job_info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
                    (job_info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
                    close_handle(s.directory); s.directory = bad;
                }
            }
#else
            if (mkdirat(s.selected, name.c_str(), 0700) != 0) {
                if (errno == EEXIST) continue;
                return fail(Error::io, "Cannot create owned job directory");
            }
            s.directory = openat(s.selected, name.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
#endif
            acquired = s.directory != bad;
            break;
        }
        if (!acquired) return fail(Error::io, "Cannot acquire owned job directory");
        s.partial_path = s.path / "sheet.partial";
        s.file = s.create("sheet.partial");
        if (s.file == bad) return fail(Error::io, "Cannot acquire owned output file");
        s.owns_partial = true;
        s.workspace = static_cast<std::byte*>(::operator new(workspace_size, std::align_val_t(4096)));
        std::memset(s.workspace, 0, workspace_size);
        s.ready = true;
        return true;
    } catch (...) { return fail(Error::failed, "Station acquisition failed"); }
}
void Device::disconnect() noexcept { state_.reset(); }
void Device::cancel() noexcept { disconnect(); }
bool Device::available() const noexcept { return state_ && state_->ready && error_ == Error::none; }
Error Device::error() const noexcept { return error_; }
const char* Device::message() const noexcept { return message_; }
const std::filesystem::path& Device::output_directory() const noexcept {
    static const std::filesystem::path empty;
    return state_ ? state_->path : empty;
}
bool Device::memory(Memory& out) noexcept {
    if (!available() || state_->capturing || state_->published || state_->owner ||
        state_->has_frame || state_->reset)
        return fail(Error::invalid, "Memory is not available for an exclusive boot lease");
    state_->leased = true;
    out = {reinterpret_cast<std::uint32_t*>(state_->workspace), 0xffffc, workspace_size};
    return true;
}
bool Device::cache(Memory m, CacheOperation op) noexcept {
    if (!available() || !state_->leased || state_->capturing || state_->has_frame ||
        m.words != reinterpret_cast<std::uint32_t*>(state_->workspace) ||
        m.word_count != 0xffffc || m.byte_count != workspace_size ||
        (op != CacheOperation::writeback && op != CacheOperation::invalidate))
        return fail(Error::invalid, "Invalid exclusive memory descriptor");
#if defined(__x86_64__) || (defined(_MSC_VER) && defined(_M_X64))
    _mm_mfence();
    for (std::size_t offset = 0; offset != workspace_size; offset += state_->line)
        _mm_clflush(state_->workspace + offset);
    _mm_mfence();
    return true;
#else
    return fail(Error::unsupported, "CLFLUSH is unavailable");
#endif
}
bool Device::begin_pass(std::uint64_t application, std::uint64_t entry,
                        const void* owner, std::uint32_t pass) noexcept {
    if (!available() || state_->reset || !application || !entry || !owner || pass > 16)
        return fail(Error::invalid, "Invalid diagnostic pass provenance");
    auto& s = *state_;
    if ((s.application && s.application != application) ||
        (s.entry && entry <= s.entry) || (s.capturing && pass != s.count))
        return fail(Error::invalid, "Stale or out-of-order diagnostic entry");
    s.application = application; s.entry = entry; s.owner = owner; s.pass = pass;
    s.has_frame = false; s.staged = {}; s.leased = false;
    return true;
}
bool Device::stage_frame(FrameIdentity id, std::span<const std::byte> rgba,
                         std::uint32_t width, std::uint32_t height, std::size_t stride) noexcept {
    if (!available()) return false;
    auto& s = *state_;
    if (s.reset || !s.owner || id.application != s.application || id.entry != s.entry ||
        id.owner != s.owner || id.pass != s.pass || !id.ordinal || !id.submission ||
        width != 640 || height != 480 || stride != 2560 || rgba.size() != frame_size ||
        (s.staged.ordinal && (id.ordinal <= s.staged.ordinal || id.submission < s.staged.submission)))
        return fail(Error::invalid, "Ineligible completed diagnostic frame");
    std::memcpy(s.workspace, rgba.data(), frame_size);
    s.staged = id; s.has_frame = true; s.leased = false;
    return true;
}
bool Device::status(std::uint8_t& out) noexcept {
    if (!available()) return false;
    out = state_->reset ? 0x08 : state_->acknowledgment;
    return true;
}
bool Device::reset_pending() const noexcept { return state_ && state_->reset; }
bool Device::take_reset_request(std::uint64_t& ticket) noexcept {
    if (!available() || !state_->reset || state_->reset_taken) return false;
    state_->reset_taken = true; ticket = state_->ticket; return true;
}
bool Device::complete_reset(std::uint64_t application) noexcept {
    if (!available() || !state_->reset || !state_->reset_taken || !application ||
        application == state_->application)
        return fail(Error::invalid, "Reset was not retired and reopened");
    auto& s = *state_;
    s.application = application; s.entry = 0; s.owner = nullptr; s.has_frame = false;
    s.staged = {}; s.leased = false; s.reset = false; s.reset_taken = false;
    s.saved = false; s.acknowledgment = 0x5a;
    return true;
}
bool Device::command(std::uint8_t value) noexcept {
    if (!available()) return false;
    auto& s = *state_;
    if (s.reset) return fail(Error::invalid, "Host reset is still outstanding");
    if (value == 0xcc) {
        if (s.saving || s.capturing) return fail(Error::invalid, "Save bracket overlaps active work");
        s.saving = true; s.saved = false;
    } else if (value == 0x33) {
        if (!s.saving) return fail(Error::invalid, "Save end without save begin");
        s.saving = false; s.saved = true;
    } else if (value == 0x5a) {
        if (!s.saved || s.saving || s.capturing)
            return fail(Error::invalid, "Reset requires completed save bracket");
        s.reset = true; s.reset_taken = false; ++s.ticket;
        s.owner = nullptr; s.has_frame = false; s.leased = false;
    } else if (value == 1) {
        if (s.capturing || s.published || s.saving || !s.owner || s.pass != 0 || s.file == bad)
            return fail(Error::invalid, "Capture job cannot begin");
        std::array<std::byte, 54> header{};
        header[0] = std::byte{'B'}; header[1] = std::byte{'M'};
        put32(header.data() + 2, sheet_size); put32(header.data() + 10, 54);
        put32(header.data() + 14, 40); put32(header.data() + 18, 2560);
        put32(header.data() + 22, 1920); header[26] = std::byte{1}; header[28] = std::byte{24};
        put32(header.data() + 34, sheet_stride * 1920);
        if (!write_at(s.file, header.data(), header.size(), 0))
            return fail(Error::io, "Cannot write bitmap header");
        s.capturing = true; s.count = 0; s.leased = false;
    } else if (value == 2) {
        if (!s.capturing || s.count >= 16 || !s.has_frame || s.pass != s.count ||
            s.staged.pass != s.count || s.staged.owner != s.owner || s.staged.entry != s.entry)
            return fail(Error::invalid, "Capture requires completed current-pass frame");
        auto* row = s.workspace + frame_size;
        for (std::uint32_t y = 0; y != 480; ++y) {
            const auto* source = s.workspace + y * 2560;
            for (std::uint32_t x = 0; x != 640; ++x) {
                row[x * 3] = source[x * 4 + 2]; row[x * 3 + 1] = source[x * 4 + 1];
                row[x * 3 + 2] = source[x * 4];
            }
            const auto sheet_y = (s.count / 4) * 480 + y;
            const auto offset = 54ull + (1919 - sheet_y) * sheet_stride + (s.count % 4) * 640 * 3;
            if (!write_at(s.file, row, 640 * 3, offset))
                return fail(Error::io, "Cannot write captured bitmap region");
        }
        ++s.count; s.has_frame = false;
    } else if (value == 4) {
        if (s.count != 16 || s.pass != 16 || (!s.capturing && !s.published))
            return fail(Error::invalid, "Sheet requires sixteen successful captures and terminal pass");
        try {
            if (!s.published) {
                if (!sync_file(s.file)) return fail(Error::io, "Cannot flush completed bitmap");
                if (!checked_close(s.file)) return fail(Error::io, "Cannot close completed bitmap");
#if defined(_WIN32)
                if (!MoveFileExW((s.path / L"sheet.partial").c_str(), (s.path / L"sheet.bmp").c_str(), MOVEFILE_WRITE_THROUGH))
                    return fail(Error::io, "Cannot publish completed bitmap");
                s.published = true;
                s.owns_partial = false;
                s.retained = CreateFileW((s.path / L"sheet.bmp").c_str(), GENERIC_READ,
                    FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
#else
                // linkat is an atomic no-replace publication. Both names are owned.
                if (linkat(s.directory, "sheet.partial", s.directory, "sheet.bmp", 0) != 0)
                    return fail(Error::io, "Cannot publish completed bitmap");
                s.published = true;
                if (unlinkat(s.directory, "sheet.partial", 0) != 0 || fsync(s.directory) != 0)
                    return fail(Error::io, "Cannot synchronize bitmap publication");
                s.owns_partial = false;
                s.retained = openat(s.directory, "sheet.bmp", O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
#endif
                if (s.retained == bad) return fail(Error::io, "Cannot retain published bitmap");
                s.capturing = false;
            } else {
                const auto name = "sheet-copy-" + std::to_string(++s.copies) + ".bmp";
                // A copy remains unpublished until its complete bytes have been flushed.
                const auto temporary = "sheet.partial";
                s.file = s.create(temporary);
                if (s.file == bad) return fail(Error::io, "Cannot acquire repeat-copy file");
                s.owns_partial = true;
                std::uint64_t offset = 0;
                while (offset != sheet_size) {
                    const auto amount = static_cast<std::size_t>(sheet_size - offset < workspace_size ? sheet_size - offset : workspace_size);
                    std::size_t done = 0;
                    while (done != amount) {
#if defined(_WIN32)
                        LARGE_INTEGER pos{}; pos.QuadPart = static_cast<LONGLONG>(offset + done);
                        DWORD n = 0;
                        if (!SetFilePointerEx(s.retained, pos, nullptr, FILE_BEGIN) ||
                            !ReadFile(s.retained, s.workspace + done, static_cast<DWORD>(amount - done), &n, nullptr) || !n)
                            return fail(Error::io, "Cannot read retained sheet");
#else
                        const auto n = pread(s.retained, s.workspace + done, amount - done, static_cast<off_t>(offset + done));
                        if (n < 0) { if (errno == EINTR) continue; return fail(Error::io, "Cannot read retained sheet"); }
                        if (!n) return fail(Error::io, "Retained sheet is truncated");
#endif
                        done += n;
                    }
                    if (!write_at(s.file, s.workspace, amount, offset)) return fail(Error::io, "Cannot write repeat sheet");
                    offset += amount;
                }
                if (!sync_file(s.file) || !checked_close(s.file)) return fail(Error::io, "Cannot flush repeat sheet");
#if defined(_WIN32)
                if (!MoveFileExW((s.path / temporary).c_str(), (s.path / name).c_str(), MOVEFILE_WRITE_THROUGH))
                    return fail(Error::io, "Cannot publish repeat sheet");
#else
                if (linkat(s.directory, temporary, s.directory, name.c_str(), 0) != 0 ||
                    unlinkat(s.directory, temporary, 0) != 0 || fsync(s.directory) != 0)
                    return fail(Error::io, "Cannot publish repeat sheet");
#endif
                s.owns_partial = false;
            }
        } catch (...) { return fail(Error::failed, "Sheet publication allocation failed"); }
    } else {
        return fail(Error::unsupported, "Unknown station command has no defined operation");
    }
    s.acknowledgment = value;
    return true;
}
} // namespace isle::snap_station
