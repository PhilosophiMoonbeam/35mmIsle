#include "35mmisle/save_store.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <cerrno>
#include <csignal>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace s = isle::save;
namespace fs = std::filesystem;
namespace {
int failures = 0;
void check(bool condition, std::string_view meaning) {
    if (!condition) { ++failures; std::cerr << "FAIL: " << meaning << '\n'; }
}
struct Profile {
    fs::path root;
    Profile() {
        static unsigned sequence = 0;
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        for (unsigned attempt = 0; attempt != 100; ++attempt) {
            auto candidate = fs::temp_directory_path() /
                ("35mmisle-save-store-" + std::to_string(stamp) + "-" + std::to_string(++sequence));
            if (fs::create_directory(candidate)) { root = std::move(candidate); break; }
        }
        if (root.empty()) throw std::runtime_error("cannot create private filesystem test profile");
    }
    ~Profile() { std::error_code ignored; fs::remove_all(root, ignored); }
};
void put32(std::vector<std::byte>& bytes, std::size_t offset, std::uint32_t value) {
    for (unsigned i = 0; i < 4; ++i) bytes[offset + i] = std::byte(value >> (24 - i * 8));
}
// Authored structural fixtures, not copied GAME data or a new-game initializer.
std::vector<std::byte> image(std::uint32_t time, std::byte fill = std::byte{0x3d}) {
    std::vector<std::byte> bytes(s::image_bytes, fill);
    constexpr unsigned char version[] = {0x48,0x41,0x4c,0x5f,0x53,0x4e,0x41,0x50,
                                         0x5f,0x56,0x31,0x2e,0x30,0x2d,0x31,0x00};
    for (std::size_t i = 0; i < sizeof(version); ++i) bytes[0x10 + i] = std::byte(version[i]);
    put32(bytes, 0x20, 0x6c); put32(bytes, 0x24, 0x180);
    put32(bytes, 0x28, 0xfba0); put32(bytes, 0x2c, 0x10a20);
    check(bool(s::seal_image(bytes, time)), "authored fixture seals with the public codec");
    return bytes;
}
s::ImageView view(const std::vector<std::byte>& bytes) {
    const auto decoded = s::inspect_image(bytes);
    check(bool(decoded), "fixture image passes public structural inspection");
    return decoded.image;
}
std::vector<std::byte> envelope(const std::vector<std::byte>& bytes, std::uint64_t generation) {
    s::EnvelopeHeader header{};
    check(s::encode_envelope_header(view(bytes), generation, header) == s::Error::none,
          "authored envelope header encodes");
    std::vector<std::byte> file(s::envelope_file_bytes);
    std::copy(header.begin(), header.end(), file.begin());
    std::copy(bytes.begin(), bytes.end(), file.begin() + s::envelope_header_bytes);
    return file;
}
void write_file(const fs::path& path, const std::vector<std::byte>& bytes) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    out.close();
    if (!out) throw std::runtime_error("cannot author test file");
}
std::vector<std::byte> read_file(const fs::path& path) {
    std::vector<std::byte> bytes(static_cast<std::size_t>(fs::file_size(path)));
    std::ifstream in(path, std::ios::binary);
    in.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!in) throw std::runtime_error("cannot retain test file bytes");
    return bytes;
}
bool same_image(s::ImageView actual, const std::vector<std::byte>& expected) {
    return actual.bytes().size() == expected.size() &&
           std::equal(actual.bytes().begin(), actual.bytes().end(), expected.begin());
}

void arguments_and_missing() {
    Profile profile;
    check(!s::Store::open(fs::path("relative-profile")), "relative profile is rejected without creation");
    auto absent = s::Store::open(profile.root / "not-created");
    check(!absent && absent.error.phase == s::StorePhase::open_directory && absent.error.native_error != 0 &&
          !fs::exists(profile.root / "not-created"), "missing absolute directory is an error, never mkdir/new game");
    write_file(profile.root / "regular-file", {});
    auto not_directory = s::Store::open(profile.root / "regular-file");
    check(!not_directory && not_directory.error.phase == s::StorePhase::open_directory,
          "regular file cannot act as a profile directory");
    auto with_nul = profile.root.native();
    with_nul.push_back(fs::path::value_type{});
    with_nul += fs::path("ignored-suffix").native();
    auto nul = s::Store::open(fs::path(with_nul));
    check(!nul && nul.error.code == s::StoreErrc::invalid_argument, "embedded NUL path is not silently truncated");
    auto opened = s::Store::open(profile.root);
    check(opened && opened.load.state == s::LoadState::missing && opened.load.writable() &&
          !opened.load.snapshot && opened.load.slots[0].state == s::SlotState::missing &&
          opened.load.slots[1].state == s::SlotState::missing,
          "genuinely absent candidates load as Missing, with no manufactured snapshot");
    check(fs::exists(profile.root / "save.lock") && !fs::exists(profile.root / "save.a") &&
          !fs::exists(profile.root / "save.b"), "open creates only the stable lock, never published defaults");
    const auto invalid = opened.store.commit({}, 0);
    check(invalid.outcome == s::CommitOutcome::not_published && invalid.error.phase == s::StorePhase::preflight &&
          invalid.error.code == s::StoreErrc::codec && invalid.error.codec_error == s::Error::invalid_argument,
          "empty ImageView fails with the original codec error before creating a temp");
}

void generations_snapshots_and_moves() {
    Profile profile;
    // Includes non-ASCII and a non-BMP character on both platforms, without locale conversion.
    const auto directory = profile.root / fs::path(u8"profile-\u00e9-\u65e5-\U0001f4f7");
    fs::create_directory(directory);
    const auto first = image(7, std::byte{0x31});
    const auto second = image(8, std::byte{0x42});
    const auto third = image(9, std::byte{0x53});
    auto opened = s::Store::open(directory);
    check(bool(opened), "Unicode profile opens");
    if (!opened) return;
    auto competing = s::Store::open(directory);
    check(!competing && competing.error.code == s::StoreErrc::lock_busy &&
          competing.error.phase == s::StorePhase::acquire_lock && competing.error.native_error != 0,
          "second session is rejected by the stable lock before hydration");
    const auto stale_missing = opened.store.commit(view(first), 4);
    check(stale_missing.outcome == s::CommitOutcome::not_published &&
          stale_missing.error.code == s::StoreErrc::expected_generation && !fs::exists(directory / "save.a"),
          "missing-profile commit still requires expected generation zero");
    const auto committed = opened.store.commit(view(first), 0);
    check(committed.outcome == s::CommitOutcome::published && committed.generation == 1 && !committed.error,
          "first real commit publishes generation one only after OS acknowledgment");
#if defined(_WIN32)
    check(committed.durability == s::Durability::win32_acknowledged, "Windows success names only its Win32 boundary");
#else
    check(committed.durability == s::Durability::linux_file_and_directory_synced,
          "Linux success names file and directory fsync acknowledgment");
#endif
    const auto a1 = read_file(directory / "save.a");
    check(a1 == envelope(first, 1) && !fs::exists(directory / "save.b"), "first generation has exact envelope bytes in save.a");
    opened.store = {};
    auto loaded = s::Store::open(directory);
    check(loaded && loaded.load.state == s::LoadState::ready && loaded.load.snapshot.generation() == 1 &&
          same_image(loaded.load.snapshot.image(), first), "Unicode cold reopen recovers exact first payload");
    const auto retained = loaded.load.snapshot;
    s::Store moved = std::move(loaded.store);
    check(!loaded.store && moved, "Store move transfers the session, not its lock identity");
    check(!s::Store::open(directory), "moved session still excludes another opener");
    check(loaded.store.commit(view(first), 1).outcome == s::CommitOutcome::not_published,
          "moved-from Store cannot commit");
    const auto stale = moved.commit(view(second), 0);
    check(stale.outcome == s::CommitOutcome::not_published && stale.error.code == s::StoreErrc::expected_generation &&
          read_file(directory / "save.a") == a1, "stale expected generation cannot mutate the current slot");
    const auto two = moved.commit(view(second), 1);
    check(two.outcome == s::CommitOutcome::published && two.generation == 2 &&
          read_file(directory / "save.a") == a1 && read_file(directory / "save.b") == envelope(second, 2),
          "second commit targets absent inactive B and preserves current good A");
    const auto b2 = read_file(directory / "save.b");
    const auto three = moved.commit(view(third), 2);
    check(three.outcome == s::CommitOutcome::published && three.generation == 3 &&
          read_file(directory / "save.a") == envelope(third, 3) && read_file(directory / "save.b") == b2,
          "third commit replaces only older inactive A, preserving generation two");
    moved = {};
    auto newest = s::Store::open(directory);
    check(newest && newest.load.snapshot.generation() == 3 && same_image(newest.load.snapshot.image(), third),
          "cold reopen selects greatest valid generation and exact payload");
    check(retained.generation() == 1 && same_image(retained.image(), first),
          "immutable retained snapshot survives Store moves, close, and later publications");
    check(fs::exists(directory / "save.lock") && !fs::exists(directory / "HEAD"),
          "stable lock is retained; protocol does not invent a HEAD file");
}

void rejected_candidates_preserve_recovery() {
    const auto good_image = image(101);
    const auto good = envelope(good_image, 14);
    for (int variant = 0; variant != 5; ++variant) {
        Profile profile;
        auto bad = envelope(image(202), 15);
        s::Error expected = s::Error::envelope_digest;
        if (variant == 0) { bad.pop_back(); expected = s::Error::envelope_size; }
        if (variant == 1) { bad.push_back(std::byte{}); expected = s::Error::envelope_size; }
        if (variant == 2) { bad[9] = std::byte{2}; expected = s::Error::envelope_version; }
        if (variant == 3) { bad[0x20] ^= std::byte{1}; expected = s::Error::envelope_baseline; }
        if (variant == 4) bad.back() ^= std::byte{1};
        write_file(profile.root / "save.a", good);
        write_file(profile.root / "save.b", bad);
        auto opened = s::Store::open(profile.root);
        check(opened && opened.load.state == s::LoadState::recovery_required && !opened.load.writable() &&
              opened.load.snapshot.generation() == 14 && same_image(opened.load.snapshot.image(), good_image) &&
              opened.load.slots[1].state == s::SlotState::rejected &&
              opened.load.slots[1].error.phase == s::StorePhase::load_b &&
              opened.load.slots[1].error.codec_error == expected,
              "corrupt/trailing/future/wrong-baseline B exposes good A only as read-only recovery with exact codec error");
        const auto denied = opened.store.commit(view(good_image), 14);
        check(denied.outcome == s::CommitOutcome::not_published && denied.error.code == s::StoreErrc::recovery_required &&
              read_file(profile.root / "save.a") == good && read_file(profile.root / "save.b") == bad,
              "autosave cannot overwrite rejected candidate or good recovery bytes");
        opened.store = {};
        fs::remove(profile.root / "save.a");
        auto only_bad = s::Store::open(profile.root);
        check(only_bad && only_bad.load.state == s::LoadState::recovery_required && !only_bad.load.snapshot,
              "a rejected sole candidate never masquerades as Missing or a new game");
    }
}

void conflicts_duplicates_order_and_exhaustion() {
    Profile profile;
    const auto first = image(1), second = image(2);
    const auto a = envelope(first, 19), b = envelope(second, 19);
    write_file(profile.root / "save.a", a); write_file(profile.root / "save.b", b);
    auto conflicting = s::Store::open(profile.root);
    check(conflicting && conflicting.load.state == s::LoadState::conflict && !conflicting.load.snapshot &&
          conflicting.load.error.code == s::StoreErrc::generation_conflict,
          "equal generations with different bytes conflict, without arbitrary recovery winner");
    check(conflicting.store.commit(view(first), 19).outcome == s::CommitOutcome::not_published &&
          read_file(profile.root / "save.a") == a && read_file(profile.root / "save.b") == b,
          "conflict blocks all publication and preserves both candidates");
    conflicting.store = {};
    write_file(profile.root / "save.b", a);
    auto duplicate = s::Store::open(profile.root);
    check(duplicate && duplicate.load.state == s::LoadState::ready && duplicate.load.snapshot.generation() == 19,
          "identical duplicate generations are harmless and writable");
    const auto next = duplicate.store.commit(view(second), 19);
    check(next.outcome == s::CommitOutcome::published && next.generation == 20 &&
          read_file(profile.root / "save.a") == a, "duplicate selection retains an independently named current good slot");
    duplicate.store = {};
    write_file(profile.root / "save.a", envelope(first, 30));
    write_file(profile.root / "save.b", envelope(second, 4));
    fs::last_write_time(profile.root / "save.a", fs::file_time_type::clock::now() - std::chrono::hours(24));
    auto ordered = s::Store::open(profile.root);
    check(ordered && ordered.load.snapshot.generation() == 30 && same_image(ordered.load.snapshot.image(), first),
          "generation, never a newer modification time, determines the winner");
    ordered.store = {};
    const auto maximum = envelope(first, std::numeric_limits<std::uint64_t>::max());
    write_file(profile.root / "save.a", maximum);
    fs::remove(profile.root / "save.b");
    auto exhausted = s::Store::open(profile.root);
    const auto rejected = exhausted.store.commit(view(second), std::numeric_limits<std::uint64_t>::max());
    check(exhausted && exhausted.load.state == s::LoadState::ready &&
          rejected.outcome == s::CommitOutcome::not_published && rejected.error.code == s::StoreErrc::generation_exhausted &&
          read_file(profile.root / "save.a") == maximum && !fs::exists(profile.root / "save.b"),
          "generation exhaustion is explicit and cannot wrap to zero or damage the current slot");
}

void orphans_are_evidence_not_saves() {
    const auto payload = image(44);
    const auto temp = envelope(payload, 500);
    for (const bool has_good : {false, true}) {
        Profile profile;
        write_file(profile.root / "save.tmp.authored-orphan", temp);
        if (has_good) write_file(profile.root / "save.a", envelope(payload, 2));
        auto opened = s::Store::open(profile.root);
        check(opened && opened.load.state == s::LoadState::recovery_required && opened.load.orphan_temporary &&
              opened.load.error.code == s::StoreErrc::orphan_temporary &&
              bool(opened.load.snapshot) == has_good && opened.load.snapshot.generation() == (has_good ? 2 : 0),
              "even a valid higher-generation orphan is never promoted or treated as Missing");
        const auto refused = opened.store.commit(view(payload), has_good ? 2 : 0);
        check(refused.outcome == s::CommitOutcome::not_published &&
              read_file(profile.root / "save.tmp.authored-orphan") == temp && !fs::exists(profile.root / "save.b"),
              "orphan evidence survives open and refused autosave without implicit repair");
    }
}

void unreadable_and_nonregular_candidates() {
    Profile profile;
    const auto payload = image(3), good = envelope(payload, 1);
    write_file(profile.root / "save.a", good);
    fs::create_directory(profile.root / "save.b");
    auto directory_slot = s::Store::open(profile.root);
    check(directory_slot && directory_slot.load.state == s::LoadState::recovery_required &&
          directory_slot.load.snapshot.generation() == 1 && directory_slot.load.slots[1].state == s::SlotState::rejected,
          "directory occupying a published slot is rejected, not absent or overwritten");
    directory_slot.store = {};
    fs::remove(profile.root / "save.b");
#if defined(_WIN32)
    write_file(profile.root / "save.b", envelope(payload, 2));
    const auto held = CreateFileW((profile.root / "save.b").c_str(), GENERIC_READ | GENERIC_WRITE,
                                  0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    check(held != INVALID_HANDLE_VALUE, "create real Windows sharing-denial fixture");
    if (held != INVALID_HANDLE_VALUE) {
        auto denied = s::Store::open(profile.root);
        check(denied && denied.load.state == s::LoadState::recovery_required && denied.load.snapshot.generation() == 1 &&
              denied.load.slots[1].error.code == s::StoreErrc::io &&
              denied.load.slots[1].error.native_error == ERROR_SHARING_VIOLATION,
              "unreadable Windows slot preserves native sharing error and read-only good recovery");
        CloseHandle(held);
    }
#else
    fs::create_symlink(profile.root / "missing-target", profile.root / "save.b");
    auto link = s::Store::open(profile.root);
    check(link && link.load.state == s::LoadState::recovery_required && link.load.snapshot.generation() == 1 &&
          link.load.slots[1].error.native_error == ELOOP,
          "dangling slot symlink is present unreadable evidence, never genuinely absent");
    link.store = {};
    fs::remove(profile.root / "save.b");
    check(mkfifo((profile.root / "save.b").c_str(), 0600) == 0, "create real nonregular FIFO fixture");
    auto fifo = s::Store::open(profile.root);
    check(fifo && fifo.load.state == s::LoadState::recovery_required &&
          fifo.load.slots[1].error.code == s::StoreErrc::not_regular_file,
          "FIFO slot is rejected without blocking the loading thread");
    fifo.store = {};
    fs::remove(profile.root / "save.b");
    if (geteuid() != 0) {
        write_file(profile.root / "save.b", envelope(payload, 2));
        check(chmod((profile.root / "save.b").c_str(), 0) == 0, "deny real slot read access");
        auto denied = s::Store::open(profile.root);
        check(denied && denied.load.state == s::LoadState::recovery_required &&
              denied.load.slots[1].error.native_error == EACCES && denied.load.snapshot.generation() == 1,
              "permission-denied slot preserves EACCES and blocks writable recovery");
        check(chmod((profile.root / "save.b").c_str(), 0600) == 0, "restore owned fixture permissions for removal");
    }
#endif
}

void ambiguous_install_freezes_and_preserves() {
    Profile profile;
    const auto old_image = image(7), new_image = image(8);
    const auto old_file = envelope(old_image, 1);
    write_file(profile.root / "save.a", old_file);
    auto opened = s::Store::open(profile.root);
    check(opened && opened.load.state == s::LoadState::ready, "install-error fixture starts writable");
    // A real noncooperating filesystem change makes inactive-slot installation
    // fail. No test hook, mocked write, or synthetic acknowledgment is involved.
    fs::create_directory(profile.root / "save.b");
    const auto result = opened.store.commit(view(new_image), 1);
    check(result.outcome == s::CommitOutcome::indeterminate && result.generation == 0 &&
          result.durability == s::Durability::none && result.error.phase == s::StorePhase::install &&
          result.error.native_error != 0 && read_file(profile.root / "save.a") == old_file,
          "install-attempt error is Indeterminate, never acknowledged or misreported unchanged");
    const auto frozen = opened.store.commit(view(new_image), 1);
    check(frozen.outcome == s::CommitOutcome::not_published && frozen.error.code == s::StoreErrc::session_frozen,
          "ambiguous session refuses every further commit");
    std::vector<fs::path> artifacts;
    for (const auto& entry : fs::directory_iterator(profile.root))
        if (entry.path().filename().string().starts_with("save.tmp.")) artifacts.push_back(entry.path());
    check(artifacts.size() == 1, "ambiguous install preserves its one complete temporary as evidence");
    if (!artifacts.empty()) {
        const auto bytes = read_file(artifacts.front());
        const auto candidate = s::inspect_envelope(bytes);
        check(candidate && candidate.generation == 2 && same_image(candidate.image, new_image),
              "preserved temporary is a completely written and verified generation, not a fake success");
    }
    opened.store = {};
    auto reopened = s::Store::open(profile.root);
    check(reopened && reopened.load.state == s::LoadState::recovery_required && reopened.load.orphan_temporary &&
          reopened.load.snapshot.generation() == 1 && read_file(profile.root / "save.a") == old_file,
          "reopen reports evidence and offers only old good generation read-only, without orphan promotion");
}

#if defined(_WIN32)
int lock_child_if_requested() {
    std::wstring directory(32768, L'\0');
    const auto length = GetEnvironmentVariableW(L"ISLE_SAVE_STORE_LOCK_CHILD", directory.data(),
                                               static_cast<DWORD>(directory.size()));
    if (length == 0) return -1;
    if (length >= directory.size()) return EXIT_FAILURE;
    directory.resize(length);
    auto opened = s::Store::open(fs::path(directory));
    const char ready = opened ? 'R' : 'E';
    DWORD written = 0;
    if (!WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), &ready, 1, &written, nullptr) || written != 1 || !opened)
        return EXIT_FAILURE;
    Sleep(INFINITE);
    return EXIT_FAILURE;
}
#endif
void process_lifetime_lock() {
    Profile profile;
#if defined(_WIN32)
    SECURITY_ATTRIBUTES attributes{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
    HANDLE input = nullptr, output = nullptr;
    if (!CreatePipe(&input, &output, &attributes, 0)) { check(false, "create child lock handshake pipe"); return; }
    SetHandleInformation(input, HANDLE_FLAG_INHERIT, 0);
    std::wstring executable(32768, L'\0');
    const auto length = GetModuleFileNameW(nullptr, executable.data(), static_cast<DWORD>(executable.size()));
    executable.resize(length);
    std::wstring command = L"\"" + executable + L"\"";
    STARTUPINFOW startup{}; startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdOutput = output; startup.hStdError = output;
    PROCESS_INFORMATION process{};
    SetEnvironmentVariableW(L"ISLE_SAVE_STORE_LOCK_CHILD", profile.root.c_str());
    const bool created = CreateProcessW(executable.c_str(), command.data(), nullptr, nullptr, TRUE,
                                        CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process) != 0;
    SetEnvironmentVariableW(L"ISLE_SAVE_STORE_LOCK_CHILD", nullptr);
    CloseHandle(output);
    if (!created) { CloseHandle(input); check(false, "start real lock-holding child process"); return; }
    char ready = 0; DWORD received = 0;
    const bool signaled = ReadFile(input, &ready, 1, &received, nullptr) && received == 1 && ready == 'R';
    CloseHandle(input);
    check(signaled, "child acquires the real lock before signaling parent");
    if (signaled) {
        auto busy = s::Store::open(profile.root);
        check(!busy && busy.error.code == s::StoreErrc::lock_busy, "independent Windows process cannot hydrate while locked");
    }
    check(TerminateProcess(process.hProcess, 17) != 0, "terminate real lock holder without destructors");
    check(WaitForSingleObject(process.hProcess, 10000) == WAIT_OBJECT_0, "terminated lock holder exits");
    CloseHandle(process.hThread); CloseHandle(process.hProcess);
#else
    int handshake[2];
    if (pipe(handshake) != 0) { check(false, "create child lock handshake pipe"); return; }
    const auto child = fork();
    if (child == 0) {
        ::close(handshake[0]);
        auto opened = s::Store::open(profile.root);
        const char ready = opened ? 'R' : 'E';
        ssize_t sent;
        do { sent = ::write(handshake[1], &ready, 1); } while (sent < 0 && errno == EINTR);
        if (sent != 1) _exit(2);
        ::close(handshake[1]);
        if (!opened) _exit(1);
        for (;;) pause();
    }
    ::close(handshake[1]);
    if (child < 0) { ::close(handshake[0]); check(false, "fork real lock-holding child process"); return; }
    char ready = 0;
    ssize_t received;
    do { received = ::read(handshake[0], &ready, 1); } while (received < 0 && errno == EINTR);
    ::close(handshake[0]);
    const bool signaled = received == 1 && ready == 'R';
    check(signaled, "child acquires the real lock before signaling parent");
    if (signaled) {
        auto busy = s::Store::open(profile.root);
        check(!busy && busy.error.code == s::StoreErrc::lock_busy, "independent Linux process cannot hydrate while locked");
    }
    check(kill(child, SIGKILL) == 0, "kill real lock holder without destructors");
    int status = 0;
    pid_t waited;
    do { waited = waitpid(child, &status, 0); } while (waited < 0 && errno == EINTR);
    check(waited == child && WIFSIGNALED(status), "killed lock holder is reaped");
#endif
    auto released = s::Store::open(profile.root);
#if defined(_WIN32)
    // Microsoft documents that process-death lock release can be deferred by
    // available system resources; it does not promise instantaneous release.
    for (unsigned attempt = 0; !released && released.error.code == s::StoreErrc::lock_busy && attempt != 100; ++attempt) {
        Sleep(10);
        released = s::Store::open(profile.root);
    }
#endif
    check(released && released.load.state == s::LoadState::missing && fs::exists(profile.root / "save.lock"),
          "OS releases process-lifetime lock after termination without deleting the stable lock name");
}

#if defined(__linux__)
void real_short_write_and_retry() {
    Profile profile;
    const auto old_image = image(71), new_image = image(72);
    const auto old_file = envelope(old_image, 1);
    write_file(profile.root / "save.a", old_file);
    const auto child = fork();
    if (child == 0) {
        auto opened = s::Store::open(profile.root);
        if (!opened || !opened.load.writable()) _exit(10);
        struct rlimit original{};
        if (getrlimit(RLIMIT_FSIZE, &original) != 0 || original.rlim_max < 1024) _exit(11);
        auto limited = original;
        limited.rlim_cur = 1024;
        if (std::signal(SIGXFSZ, SIG_IGN) == SIG_ERR || setrlimit(RLIMIT_FSIZE, &limited) != 0) _exit(12);
        // The kernel accepts a partial payload write up to 1024 file bytes,
        // then returns EFBIG. This exercises real short-write/error handling.
        const auto failed = opened.store.commit(view(new_image), 1);
        if (setrlimit(RLIMIT_FSIZE, &original) != 0) _exit(13);
        bool okay = failed.outcome == s::CommitOutcome::not_published && failed.generation == 0 &&
            failed.error.code == s::StoreErrc::io && failed.error.phase == s::StorePhase::write_temporary &&
            failed.error.native_error == EFBIG && !failed.cleanup_error &&
            read_file(profile.root / "save.a") == old_file && !fs::exists(profile.root / "save.b");
        for (const auto& entry : fs::directory_iterator(profile.root))
            if (entry.path().filename().string().starts_with("save.tmp.")) okay = false;
        const auto retried = opened.store.commit(view(new_image), 1);
        okay = okay && retried.outcome == s::CommitOutcome::published && retried.generation == 2 &&
            read_file(profile.root / "save.a") == old_file;
        _exit(okay ? 0 : 14);
    }
    if (child < 0) { check(false, "fork actual short-write regression process"); return; }
    int status = 0;
    pid_t waited;
    do { waited = waitpid(child, &status, 0); } while (waited < 0 && errno == EINTR);
    check(waited == child && WIFEXITED(status) && WEXITSTATUS(status) == 0,
          "real partial write/EFBIG is NotPublished, cleans only its uninstalled temp, and allows a checked retry");
    auto reopened = s::Store::open(profile.root);
    check(reopened && reopened.load.state == s::LoadState::ready && reopened.load.snapshot.generation() == 2 &&
          same_image(reopened.load.snapshot.image(), new_image) && read_file(profile.root / "save.a") == old_file,
          "post-error retry cold-loads complete generation two with old current generation still intact");
}
#endif
} // namespace

int main() {
#if defined(_WIN32)
    if (const int child = lock_child_if_requested(); child != -1) return child;
#endif
    static_assert(!std::is_copy_constructible_v<s::Store>);
    static_assert(std::is_nothrow_move_constructible_v<s::Store>);
    static_assert(std::is_same_v<decltype(s::Snapshot{}.image().bytes()), std::span<const std::byte>>);
    try {
        arguments_and_missing();
        generations_snapshots_and_moves();
        rejected_candidates_preserve_recovery();
        conflicts_duplicates_order_and_exhaustion();
        orphans_are_evidence_not_saves();
        unreadable_and_nonregular_candidates();
        ambiguous_install_freezes_and_preserves();
        process_lifetime_lock();
#if defined(__linux__)
        real_short_write_and_retry();
#endif
    } catch (const std::exception& error) {
        ++failures; std::cerr << "FAIL: filesystem regression exception: " << error.what() << '\n';
    }
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
