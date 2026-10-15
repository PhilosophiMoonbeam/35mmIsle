#include "35mmisle/save_store.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <limits>
#include <string>
#include <string_view>
#include <utility>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(__linux__)
#include <cerrno>
#include <dirent.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#else
#error "The checked save store requires Linux or Windows native filesystem APIs"
#endif

namespace isle::save {
namespace {
StoreError io_error(StorePhase phase, std::uint32_t native) noexcept {
    return {StoreErrc::io, phase, native};
}
StoreError codec_error(StorePhase phase, Error error) noexcept {
    return {StoreErrc::codec, phase, 0, error};
}
#if defined(_WIN32)
using NativeHandle = HANDLE;
const NativeHandle invalid_handle = INVALID_HANDLE_VALUE;
std::uint32_t native_error() noexcept { return GetLastError(); }
#else
using NativeHandle = int;
constexpr NativeHandle invalid_handle = -1;
std::uint32_t native_error() noexcept { return static_cast<std::uint32_t>(errno); }
#endif
struct NativeFile {
    NativeHandle handle = invalid_handle;
    NativeFile() noexcept = default;
    NativeFile(const NativeFile&) = delete;
    NativeFile& operator=(const NativeFile&) = delete;
    ~NativeFile() { (void)close(StorePhase::none); }
    StoreError close(StorePhase phase) noexcept {
        if (handle == invalid_handle) return {};
        const auto old = std::exchange(handle, invalid_handle);
#if defined(_WIN32)
        if (!CloseHandle(old)) return io_error(phase, native_error());
#else
        // Do not retry close on EINTR: Linux has already released the descriptor.
        if (::close(old) != 0) return io_error(phase, native_error());
#endif
        return {};
    }
};
StoreError regular_file(NativeFile& file, StorePhase phase, std::uint64_t* size = nullptr) noexcept {
#if defined(_WIN32)
    BY_HANDLE_FILE_INFORMATION info{};
    if (!GetFileInformationByHandle(file.handle, &info)) return io_error(phase, native_error());
    SetLastError(NO_ERROR);
    const auto type = GetFileType(file.handle);
    if (type == FILE_TYPE_UNKNOWN && GetLastError() != NO_ERROR) return io_error(phase, native_error());
    if (type != FILE_TYPE_DISK ||
        (info.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)))
        return {StoreErrc::not_regular_file, phase};
    if (size) *size = (std::uint64_t(info.nFileSizeHigh) << 32) | info.nFileSizeLow;
#else
    struct stat info{};
    if (fstat(file.handle, &info) != 0) return io_error(phase, native_error());
    if (!S_ISREG(info.st_mode)) return {StoreErrc::not_regular_file, phase};
    if (size) *size = static_cast<std::uint64_t>(info.st_size);
#endif
    return {};
}
StoreError read_complete(NativeFile& file, std::span<std::byte> bytes, StorePhase phase) noexcept {
    std::uint64_t size = 0;
    if (auto error = regular_file(file, phase, &size)) return error;
    if (size != bytes.size()) return codec_error(phase, Error::envelope_size);
#if defined(_WIN32)
    LARGE_INTEGER start{};
    if (!SetFilePointerEx(file.handle, start, nullptr, FILE_BEGIN)) return io_error(phase, native_error());
#else
    if (lseek(file.handle, 0, SEEK_SET) < 0) return io_error(phase, native_error());
#endif
    std::size_t done = 0;
    while (done != bytes.size()) {
#if defined(_WIN32)
        DWORD count = 0;
        if (!ReadFile(file.handle, bytes.data() + done, static_cast<DWORD>(bytes.size() - done), &count, nullptr))
            return io_error(phase, native_error());
#else
        const auto count = ::read(file.handle, bytes.data() + done, bytes.size() - done);
        if (count < 0) { if (errno == EINTR) continue; return io_error(phase, native_error()); }
#endif
        if (count == 0) return codec_error(phase, Error::envelope_size);
        done += static_cast<std::size_t>(count);
    }
    std::byte extra{};
#if defined(_WIN32)
    DWORD count = 0;
    if (!ReadFile(file.handle, &extra, 1, &count, nullptr)) return io_error(phase, native_error());
#else
    ssize_t count;
    do { count = ::read(file.handle, &extra, 1); } while (count < 0 && errno == EINTR);
    if (count < 0) return io_error(phase, native_error());
#endif
    if (count != 0) return codec_error(phase, Error::envelope_size);
    return {};
}
StoreError write_complete(NativeFile& file, std::span<const std::byte> bytes) noexcept {
    std::size_t done = 0;
    while (done != bytes.size()) {
#if defined(_WIN32)
        DWORD count = 0;
        if (!WriteFile(file.handle, bytes.data() + done, static_cast<DWORD>(bytes.size() - done), &count, nullptr))
            return io_error(StorePhase::write_temporary, native_error());
        if (count == 0) return io_error(StorePhase::write_temporary, ERROR_WRITE_FAULT);
#else
        const auto count = ::write(file.handle, bytes.data() + done, bytes.size() - done);
        if (count < 0) { if (errno == EINTR) continue; return io_error(StorePhase::write_temporary, native_error()); }
        if (count == 0) return io_error(StorePhase::write_temporary, EIO);
#endif
        done += static_cast<std::size_t>(count);
    }
    return {};
}
StoreError flush_file(NativeFile& file, StorePhase phase) noexcept {
#if defined(_WIN32)
    if (!FlushFileBuffers(file.handle)) return io_error(phase, native_error());
#else
    int result;
    do { result = fsync(file.handle); } while (result != 0 && errno == EINTR);
    if (result != 0) return io_error(phase, native_error());
#endif
    return {};
}
StoreError verify_written(NativeFile& file, std::span<std::byte> scratch,
                          const EnvelopeHeader& header, ImageView image,
                          std::uint64_t generation, StorePhase phase) noexcept {
    if (auto error = read_complete(file, scratch, phase)) return error;
    const auto decoded = inspect_envelope(scratch);
    if (!decoded) return codec_error(phase, decoded.error);
    if (decoded.generation != generation ||
        !std::equal(header.begin(), header.end(), scratch.begin()) ||
        !std::equal(image.bytes().begin(), image.bytes().end(), decoded.image.bytes().begin()))
        return {StoreErrc::verification_mismatch, phase};
    return {};
}
} // namespace

struct Snapshot::Data {
    std::array<std::byte, envelope_file_bytes> bytes;
    ImageView image{};
    std::uint64_t generation = 0;
    Data() noexcept {} // Full read precedes publication; do not zero a second payload.
};
ImageView Snapshot::image() const noexcept { return data_ ? data_->image : ImageView{}; }
std::uint64_t Snapshot::generation() const noexcept { return data_ ? data_->generation : 0; }

struct Store::Impl {
    NativeFile directory;
    NativeFile lock;
    std::array<std::byte, envelope_file_bytes> scratch;
    bool writable = false;
    bool frozen = false;
    int current_slot = -1;
    std::uint64_t generation = 0;
    std::uint64_t temporary_sequence = 0;
    std::array<char, 80> temporary_name{};
#if defined(_WIN32)
    std::wstring prefix;
    std::array<std::wstring, 2> slots;
    std::wstring lock_path;
    std::wstring search_path;
    std::wstring temporary_path;
#endif
    Impl() noexcept {}

    StoreError initialize(const std::filesystem::path& path) {
#if defined(_WIN32)
        prefix = path.native();
        if (prefix.back() != L'\\' && prefix.back() != L'/') prefix.push_back(L'\\');
        slots = {prefix + L"save.a", prefix + L"save.b"};
        lock_path = prefix + L"save.lock";
        search_path = prefix + L"save.tmp.*";
        temporary_path.reserve(prefix.size() + temporary_name.size());
        directory.handle = CreateFileW(path.c_str(), FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES,
            FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
        if (directory.handle == invalid_handle) return io_error(StorePhase::open_directory, native_error());
        BY_HANDLE_FILE_INFORMATION info{};
        if (!GetFileInformationByHandle(directory.handle, &info)) return io_error(StorePhase::open_directory, native_error());
        if (!(info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || (info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
            return {StoreErrc::invalid_argument, StorePhase::open_directory};
        lock.handle = CreateFileW(lock_path.c_str(), GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS, FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
#else
        directory.handle = ::open(path.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
        if (directory.handle == invalid_handle) return io_error(StorePhase::open_directory, native_error());
        lock.handle = openat(directory.handle, "save.lock", O_RDWR | O_CREAT | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK, 0600);
#endif
        if (lock.handle == invalid_handle) return io_error(StorePhase::open_lock, native_error());
        if (auto error = regular_file(lock, StorePhase::open_lock)) return error;
#if defined(_WIN32)
        OVERLAPPED range{};
        if (!LockFileEx(lock.handle, LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY, 0, 1, 0, &range)) {
            const auto native = native_error();
            return {native == ERROR_LOCK_VIOLATION ? StoreErrc::lock_busy : StoreErrc::io,
                    StorePhase::acquire_lock, native};
        }
#else
        if (flock(lock.handle, LOCK_EX | LOCK_NB) != 0) {
            const auto native = native_error();
            return {native == EWOULDBLOCK || native == EAGAIN ? StoreErrc::lock_busy : StoreErrc::io,
                    StorePhase::acquire_lock, native};
        }
#endif
        return {};
    }

    StoreError scan_orphans(bool& found) noexcept {
#if defined(_WIN32)
        WIN32_FIND_DATAW entry{};
        const auto search = FindFirstFileW(search_path.c_str(), &entry);
        if (search == INVALID_HANDLE_VALUE) {
            const auto native = native_error();
            return native == ERROR_FILE_NOT_FOUND ? StoreError{} : io_error(StorePhase::scan_directory, native);
        }
        // Any object with our temporary prefix is evidence, regardless of type.
        found = true;
        if (!FindClose(search)) return io_error(StorePhase::scan_directory, native_error());
#else
        const int descriptor = openat(directory.handle, ".", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
        if (descriptor < 0) return io_error(StorePhase::scan_directory, native_error());
        DIR* entries = fdopendir(descriptor);
        if (!entries) {
            const auto native = native_error();
            (void)::close(descriptor);
            return io_error(StorePhase::scan_directory, native);
        }
        errno = 0;
        while (const auto* entry = readdir(entries)) {
            if (std::string_view(entry->d_name).starts_with("save.tmp.")) found = true;
            errno = 0;
        }
        const auto native = native_error();
        const int closed = closedir(entries);
        if (native != 0) return io_error(StorePhase::scan_directory, native);
        if (closed != 0) return io_error(StorePhase::scan_directory, native_error());
#endif
        return {};
    }

    std::shared_ptr<const Snapshot::Data> read_slot(int index, SlotResult& result) {
        const auto phase = index == 0 ? StorePhase::load_a : StorePhase::load_b;
        NativeFile file;
#if defined(_WIN32)
        file.handle = CreateFileW(slots[index].c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                                  OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
#else
        file.handle = openat(directory.handle, index == 0 ? "save.a" : "save.b",
                             O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
#endif
        if (file.handle == invalid_handle) {
            const auto native = native_error();
#if defined(_WIN32)
            const bool absent = native == ERROR_FILE_NOT_FOUND;
#else
            const bool absent = native == ENOENT;
#endif
            if (absent) return {};
            result.state = SlotState::rejected;
            result.error = io_error(phase, native);
            return {};
        }
        auto data = std::make_shared<Snapshot::Data>();
        result.error = read_complete(file, data->bytes, phase);
        if (!result.error) {
            const auto decoded = inspect_envelope(data->bytes);
            if (!decoded) result.error = codec_error(phase, decoded.error);
            else { data->image = decoded.image; data->generation = decoded.generation; }
        }
        const auto closed = file.close(phase);
        if (!result.error) result.error = closed;
        if (result.error) { result.state = SlotState::rejected; return {}; }
        result.state = SlotState::valid;
        result.generation = data->generation;
        return data;
    }

    LoadResult load() {
        LoadResult result;
        result.error = scan_orphans(result.orphan_temporary);
        const auto a = read_slot(0, result.slots[0]);
        const auto b = read_slot(1, result.slots[1]);
        if (a && b && a->generation == b->generation && a->bytes != b->bytes) {
            result.state = LoadState::conflict;
            if (!result.error) result.error = {StoreErrc::generation_conflict, StorePhase::preflight};
            return result;
        }
        const auto best = b && (!a || b->generation > a->generation) ? b : a;
        if (best) {
            result.snapshot.data_ = best;
            current_slot = best == a ? 0 : 1;
            generation = best->generation;
        }
        if (!result.error) {
            for (const auto& slot : result.slots) {
                if (slot.state == SlotState::rejected) { result.error = slot.error; break; }
            }
        }
        if (!result.error && result.orphan_temporary)
            result.error = {StoreErrc::orphan_temporary, StorePhase::scan_directory};
        if (result.error) { result.state = LoadState::recovery_required; return result; }
        result.state = best ? LoadState::ready : LoadState::missing;
        writable = true;
        return result;
    }

    StoreError create_temporary(NativeFile& file) noexcept {
        if (temporary_sequence == std::numeric_limits<std::uint64_t>::max())
            return {StoreErrc::generation_exhausted, StorePhase::create_temporary};
#if defined(_WIN32)
        const auto process = GetCurrentProcessId();
#else
        const auto process = getpid();
#endif
        const int length = std::snprintf(temporary_name.data(), temporary_name.size(), "save.tmp.%lu.%llu",
            static_cast<unsigned long>(process), static_cast<unsigned long long>(++temporary_sequence));
        if (length < 0 || static_cast<std::size_t>(length) >= temporary_name.size())
            return {StoreErrc::invalid_argument, StorePhase::create_temporary};
#if defined(_WIN32)
        temporary_path.assign(prefix);
        for (int i = 0; i < length; ++i) temporary_path.push_back(static_cast<wchar_t>(temporary_name[i]));
        file.handle = CreateFileW(temporary_path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                                  CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
#else
        file.handle = openat(directory.handle, temporary_name.data(),
                             O_RDWR | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
#endif
        if (file.handle == invalid_handle) return io_error(StorePhase::create_temporary, native_error());
        return {};
    }

    StoreError remove_uninstalled() noexcept {
#if defined(_WIN32)
        if (!DeleteFileW(temporary_path.c_str())) return io_error(StorePhase::cleanup_temporary, native_error());
#else
        if (unlinkat(directory.handle, temporary_name.data(), 0) != 0)
            return io_error(StorePhase::cleanup_temporary, native_error());
#endif
        return {};
    }

    CommitResult commit(ImageView image, std::uint64_t expected) noexcept {
        auto fail = [](StoreError error) {
            return CommitResult{CommitOutcome::not_published, 0, Durability::none, error, {}};
        };
        if (frozen) return fail({StoreErrc::session_frozen, StorePhase::preflight});
        if (!writable) return fail({StoreErrc::recovery_required, StorePhase::preflight});
        if (expected != generation) return fail({StoreErrc::expected_generation, StorePhase::preflight});
        if (generation == std::numeric_limits<std::uint64_t>::max())
            return fail({StoreErrc::generation_exhausted, StorePhase::preflight});
        const auto next = generation + 1;
        EnvelopeHeader header;
        if (const auto error = encode_envelope_header(image, next, header); error != Error::none)
            return fail(codec_error(StorePhase::preflight, error));
        NativeFile temporary;
        if (auto error = create_temporary(temporary)) return fail(error);
        auto before_install = [&](StoreError error) {
            const auto closed = temporary.close(StorePhase::close_temporary);
            auto result = fail(error ? error : closed);
            result.cleanup_error = remove_uninstalled();
            if (result.cleanup_error) frozen = true;
            return result;
        };
        if (auto error = write_complete(temporary, header)) return before_install(error);
        if (auto error = write_complete(temporary, image.bytes())) return before_install(error);
        if (auto error = verify_written(temporary, scratch, header, image, next, StorePhase::verify_temporary))
            return before_install(error);
        if (auto error = flush_file(temporary, StorePhase::flush_temporary)) return before_install(error);
        if (auto error = temporary.close(StorePhase::close_temporary)) return before_install(error);

        const int target = current_slot == 0 ? 1 : 0;
        // From the first install attempt onward, any error is ambiguous. Do not
        // delete, roll back, promote a temp, or acknowledge an unchanged old slot.
        auto ambiguous = [&](StoreError error) {
            frozen = true;
            return CommitResult{CommitOutcome::indeterminate, 0, Durability::none, error, {}};
        };
#if defined(_WIN32)
        if (!MoveFileExW(temporary_path.c_str(), slots[target].c_str(),
                         MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
            return ambiguous(io_error(StorePhase::install, native_error()));
        NativeFile installed;
        installed.handle = CreateFileW(slots[target].c_str(), GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
        if (installed.handle == invalid_handle) return ambiguous(io_error(StorePhase::reopen_installed, native_error()));
        if (auto error = verify_written(installed, scratch, header, image, next, StorePhase::verify_installed))
            return ambiguous(error);
        if (auto error = flush_file(installed, StorePhase::flush_installed)) return ambiguous(error);
        if (auto error = installed.close(StorePhase::close_installed)) return ambiguous(error);
        constexpr auto durability = Durability::win32_acknowledged;
#else
        if (renameat(directory.handle, temporary_name.data(), directory.handle, target == 0 ? "save.a" : "save.b") != 0)
            return ambiguous(io_error(StorePhase::install, native_error()));
        if (auto error = flush_file(directory, StorePhase::acknowledge_directory)) return ambiguous(error);
        constexpr auto durability = Durability::linux_file_and_directory_synced;
#endif
        generation = next;
        current_slot = target;
        return {CommitOutcome::published, next, durability, {}, {}};
    }
};

Store::Store() noexcept = default;
Store::~Store() = default;
Store::Store(Store&&) noexcept = default;
Store& Store::operator=(Store&&) noexcept = default;
Store::operator bool() const noexcept { return bool(impl_); }
OpenResult Store::open(const std::filesystem::path& directory) {
    OpenResult result;
    if (!directory.is_absolute() || directory.native().find(std::filesystem::path::value_type{}) !=
                                        std::filesystem::path::string_type::npos)
        return result;
    auto impl = std::make_unique<Impl>();
    result.error = impl->initialize(directory);
    if (result.error) return result;
    result.load = impl->load();
    result.store.impl_ = std::move(impl);
    return result;
}
CommitResult Store::commit(ImageView image, std::uint64_t expected_generation) noexcept {
    if (!impl_) return {};
    return impl_->commit(image, expected_generation);
}

} // namespace isle::save
