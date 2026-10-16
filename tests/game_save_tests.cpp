#include "35mmisle/game_save.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace s = isle::save;
namespace fs = std::filesystem;
namespace {
int failures = 0;
fs::path parent_directory;
void check(bool condition, std::string_view meaning) {
    if (!condition) { ++failures; std::cerr << "FAIL: " << meaning << '\n'; }
}
struct Profile {
    fs::path root;
    Profile() {
        static unsigned sequence = 0;
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        for (unsigned attempt = 0; attempt < 100; ++attempt) {
            auto candidate = parent_directory /
                ("35mmisle-game-save-" + std::to_string(stamp) + "-" + std::to_string(++sequence));
            if (fs::create_directory(candidate)) { root = std::move(candidate); return; }
        }
        throw std::runtime_error("cannot create private GAME-save test profile");
    }
    ~Profile() { std::error_code ignored; fs::remove_all(root, ignored); }
};
void put32(std::span<std::byte> bytes, std::size_t offset, std::uint32_t value) {
    for (unsigned i = 0; i < 4; ++i) bytes[offset + i] = std::byte(value >> (24 - i * 8));
}
std::uint32_t get32(std::span<const std::byte> bytes, std::size_t offset) {
    std::uint32_t result = 0;
    for (unsigned i = 0; i < 4; ++i) result = (result << 8) | std::to_integer<unsigned>(bytes[offset + i]);
    return result;
}
std::vector<std::byte> copy(s::ImageView image) { return {image.bytes().begin(), image.bytes().end()}; }
bool same(s::ImageView image, std::span<const std::byte> expected) {
    return image.bytes().size() == expected.size() &&
        std::equal(image.bytes().begin(), image.bytes().end(), expected.begin());
}
void write_file(const fs::path& path, std::span<const std::byte> bytes) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    out.close();
    if (!out) throw std::runtime_error("cannot write owned save fixture");
}
std::vector<std::byte> read_file(const fs::path& path) {
    std::vector<std::byte> bytes(static_cast<std::size_t>(fs::file_size(path)));
    std::ifstream in(path, std::ios::binary);
    in.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!in) throw std::runtime_error("cannot read owned save fixture");
    return bytes;
}
std::vector<std::byte> envelope(s::ImageView image, std::uint64_t generation) {
    s::EnvelopeHeader header;
    check(s::encode_envelope_header(image, generation, header) == s::Error::none, "encode real canonical image envelope");
    std::vector<std::byte> result(s::envelope_file_bytes);
    std::copy(header.begin(), header.end(), result.begin());
    std::copy(image.bytes().begin(), image.bytes().end(), result.begin() + s::envelope_header_bytes);
    return result;
}
void defaults_and_roundtrip() {
    Profile profile;
    auto opened = s::GameSession::open(profile.root, 0x12345678);
    check(opened && opened.state == s::GameLoadState::initialized_missing &&
          opened.load.state == s::LoadState::missing, "only genuinely missing profile initializes GAME state");
    if (!opened) return;
    check(!fs::exists(profile.root / "save.a") && !fs::exists(profile.root / "save.b"),
          "initialization is not a fabricated durable commit");
    const auto fresh_result = opened.session.snapshot();
    check(bool(fresh_result), "fresh canonical snapshot publishes");
    if (!fresh_result) return;
    const auto fresh = fresh_result.value;
    const auto bytes = fresh.image().bytes();
    check(bytes.size() == s::image_bytes && bool(s::inspect_image(bytes)) && fresh.generation() == 0,
          "fresh image is sealed and generation zero until publication");
    // Independently enumerated expected wire fields; every other byte must be zero.
    std::vector<std::byte> expected(s::image_bytes);
    constexpr unsigned char version[] = {0x48,0x41,0x4c,0x5f,0x53,0x4e,0x41,0x50,
                                         0x5f,0x56,0x31,0x2e,0x30,0x2d,0x31,0x00};
    for (std::size_t i = 0; i < sizeof(version); ++i) expected[0x10 + i] = std::byte(version[i]);
    put32(expected, 0x20, 0x6c); put32(expected, 0x24, 0x180);
    put32(expected, 0x28, 0xfba0); put32(expected, 0x2c, 0x10a20);
    for (std::size_t i = 0; i < 73; ++i) put32(expected, 0x184 + i * 0x3a0, 0xffffffffu);
    for (std::size_t i = 0; i < 60; ++i) put32(expected, 0x10a24 + i * 0x3e0, 0xffffffffu);
    put32(expected, 0x64, 0x13240020);
    check(bool(s::seal_image(expected, 0x12345678)), "independent default field oracle seals");
    check(same(fresh.image(), expected), "all default header/name/report/album/progress/settings/padding bytes match source facts");
    for (unsigned id = 0; id < 22; ++id)
        check(fresh.player_flag(id) && fresh.player_flag(id).value == (id == 10), "all source flag defaults including transient flag");
    check(fresh.screen_offsets().value.horizontal == 0 && fresh.screen_offsets().value.vertical == 0 &&
          fresh.selected_course().value == 0 && fresh.unlocked_course().value == 0, "source default settings and progress");
    for (std::size_t i = 0; i < 69; ++i) {
        check(fresh.report_score(i).value == 0 && !fresh.photo_present(s::PhotoSet::report, i).value,
              "every report score/photo initializes");
    }
    for (std::size_t i = 0; i < 60; ++i)
        check(!fresh.photo_present(s::PhotoSet::album, i).value &&
              std::all_of(fresh.album_comment(i).value.begin(), fresh.album_comment(i).value.end(),
                          [](std::byte b) { return b == std::byte{0}; }), "every album record/comment initializes");
    for (std::size_t i = 0; i < 4; ++i)
        check(!fresh.photo_present(s::PhotoSet::temporary, i).value, "all temporary photos initialize");
    check(opened.session.consume_initialization_notice() && !opened.session.consume_initialization_notice() &&
          opened.session.consume_initialization_notice_secondary() && !opened.session.consume_initialization_notice_secondary(),
          "independent initialization notices consume exactly once before durable publication");
    check(!opened.session.consume_course_notice(), "fresh course notification is clear");
    check(!opened.session.set_player_flag(9, true) && !opened.session.set_player_flag(12, true) &&
          !opened.session.set_player_flag(16, true) && !opened.session.set_player_flag(21, true) &&
          !opened.session.set_player_flag(15, true), "mutate distinct source flags including nonpersistent RAM flag");
    check(!opened.session.set_screen_offsets(1000, -1000) && !opened.session.set_selected_course(3) &&
          !opened.session.set_unlocked_course(100), "source settings/progress clamp and mutate");
    check(opened.session.consume_course_notice() && !opened.session.consume_course_notice(), "unlock notice consumes once");
    check(!opened.session.set_course_score(6, -1234567) && !opened.session.set_report_score(68, 0x12345678), "signed scores mutate at final valid indices");
    std::array<std::byte, 16> name{}; name[0] = std::byte{0x82}; name[1] = std::byte{0x60};
    std::array<std::byte, 64> comment{}; comment[0] = std::byte{0x12}; comment[1] = std::byte{0x34}; comment[63] = std::byte{0x56};
    check(!opened.session.set_player_name(name) && !opened.session.set_album_comment(59, comment), "source name and BE16 comments mutate without host conversion");
    auto changed = opened.session.snapshot().value;
    check(changed.screen_offsets().value.horizontal == 24 && changed.screen_offsets().value.vertical == -36 &&
          changed.selected_course().value == 3 && changed.unlocked_course().value == 6,
          "native settings decode original bitfield endianness/clamping");
    check(get32(changed.image().bytes(), 0x4c) == 0xffed2979u &&
          get32(changed.image().bytes(), 0x17c) == 0x12345678, "signed course and report words serialize big endian");
    check(same(fresh.image(), expected) && fresh.image().bytes().data() != changed.image().bytes().data(),
          "earlier owning snapshot remains immutable across mutation");
    const auto unchanged = copy(changed.image());
    check(opened.session.set_player_flag(22, true).code == s::GameErrc::field_bounds &&
          opened.session.set_selected_course(7).code == s::GameErrc::field_bounds &&
          opened.session.set_course_score(7, 1).code == s::GameErrc::field_bounds &&
          opened.session.set_report_score(69, 1).code == s::GameErrc::field_bounds &&
          opened.session.set_player_name(std::span(name).first(15)).code == s::GameErrc::field_bounds &&
          opened.session.set_album_comment(60, comment).code == s::GameErrc::field_bounds &&
          opened.session.set_album_comment(0, std::span(comment).first(63)).code == s::GameErrc::field_bounds &&
          opened.session.remove_photo(s::PhotoSet::report, 69).code == s::GameErrc::field_bounds,
          "all invalid mutable field boundaries report checked errors");
    check(!changed.player_flag(22) && !changed.course_score(7) && !changed.report_score(69) &&
          !changed.photo(s::PhotoSet::temporary, 4) && !changed.photo(static_cast<s::PhotoSet>(99), 0) &&
          !changed.album_comment(60), "read boundaries never publish an out-of-range view");
    check(same(opened.session.snapshot().value.image(), unchanged), "rejected mutations preserve canonical bytes");
    const auto committed = opened.session.commit(0xfedcba98);
    check(committed.outcome == s::CommitOutcome::published && committed.generation == 1 && !committed.error,
          "actual checked missing-profile commit acknowledges generation one");
    const auto published = opened.session.snapshot().value;
    const auto published_bytes = copy(published.image());
    check(published.generation() == 1 && get32(published.image().bytes(), 0x30) == 0xfedcba98 &&
          changed.generation() == 0 && same(changed.image(), unchanged), "commit seals time and preserves old snapshot/generation");
    opened.session = {};
    auto reloaded = s::GameSession::open(profile.root, 999);
    check(reloaded && reloaded.state == s::GameLoadState::ready && !reloaded.session.consume_initialization_notice() &&
          !reloaded.session.consume_initialization_notice_secondary(), "cold load does not rerun initializer or emit missing notices");
    const auto cold = reloaded.session.snapshot().value;
    check(cold.generation() == 1 && same(cold.image(), published_bytes) && cold.player_flag(9).value &&
          cold.player_flag(12).value && cold.player_flag(16).value && cold.player_flag(21).value &&
          !cold.player_flag(15).value && cold.course_score(6).value == -1234567 &&
          cold.report_score(68).value == 0x12345678 && cold.screen_offsets().value.horizontal == 24 &&
          cold.screen_offsets().value.vertical == -36 && std::equal(name.begin(), name.end(), cold.player_name().begin()),
          "cold native reload preserves canonical flags/settings/progress/name and resets only transient RAM flag");
    check(!reloaded.session.remove_photo(s::PhotoSet::album, 59), "album removal succeeds");
    const auto removed = reloaded.session.snapshot().value;
    check(removed.album_comment(59).value[0] == std::byte{0} && removed.album_comment(59).value[1] == std::byte{0} &&
          removed.album_comment(59).value[63] == std::byte{0x56} && cold.album_comment(59).value[0] == std::byte{0x12},
          "album removal clears only the source comment terminator and retains prior owning view");
    check(published && same(published.image(), published_bytes), "owning snapshot survives original session destruction");
}
void explicit_reset_and_cold_generations() {
    Profile profile;
    auto opened = s::GameSession::open(profile.root, 1);
    check(bool(opened), "explicit reset starts from real missing profile");
    if (!opened) return;
    auto& session = opened.session;
    const auto defaults = session.snapshot().value;
    auto expected_defaults = copy(defaults.image());
    check(session.consume_initialization_notice(), "consume only first startup notice before reset");
    for (unsigned id = 0; id < 22; ++id)
        check(!session.set_player_flag(id, true), "seed source flags before explicit reset");
    for (std::size_t i = 0; i < 7; ++i)
        check(!session.set_course_score(i, static_cast<std::int32_t>(100 + i)), "seed every course score before reset");
    for (std::size_t i = 0; i < 69; ++i)
        check(!session.set_report_score(i, static_cast<std::int32_t>(200 + i)), "seed every report score before reset");
    std::array<std::byte, 16> name; name.fill(std::byte{0x81});
    std::array<std::byte, 64> comment; comment.fill(std::byte{0x23});
    check(!session.set_player_name(name) && !session.set_album_comment(59, comment) &&
          !session.set_screen_offsets(-50, 14) && !session.set_unlocked_course(6) &&
          !session.set_selected_course(3), "seed name/comment/settings/progress before reset");
    const auto first_commit = session.commit(2);
    check(first_commit.outcome == s::CommitOutcome::published && first_commit.generation == 1,
          "publish changed source state as real first generation");
    const auto prior = session.snapshot().value;
    const auto prior_bytes = copy(prior.image());
    const auto prior_file = read_file(profile.root / "save.a");
    check(!session.reset(3), "explicit source reset succeeds on writable initialized session");
    const auto reset = session.snapshot().value;
    check(bool(s::seal_image(expected_defaults, 3)) && same(reset.image(), expected_defaults) &&
          reset.generation() == 1 && !reset.player_flag(15).value && !session.consume_course_notice(),
          "reset restores complete canonical defaults and clears transient state without changing generation");
    check(same(prior.image(), prior_bytes) && prior.generation() == 1 && prior.player_flag(15).value &&
          prior.player_flag(9).value && prior.selected_course().value == 3 &&
          defaults.generation() == 0 && defaults.course_score(6).value == 0,
          "retained pre-reset and pre-mutation snapshots remain immutable and independently owned");
    check(read_file(profile.root / "save.a") == prior_file && !fs::exists(profile.root / "save.b"),
          "source reset never implicitly publishes or overwrites durable state");
    check(!session.consume_initialization_notice() && session.consume_initialization_notice_secondary() &&
          !session.consume_initialization_notice_secondary(),
          "reset neither reasserts a consumed startup notice nor clears an independent pending one");
    auto competing = s::GameSession::open(profile.root, 99);
    check(!competing && competing.store_error.code == s::StoreErrc::lock_busy,
          "reset retains the same durable session lock");
    session = {};
    auto first_cold = s::GameSession::open(profile.root, 99);
    check(first_cold && first_cold.state == s::GameLoadState::ready &&
          same(first_cold.session.snapshot().value.image(), prior_bytes),
          "cold reopen before an explicit reset commit recovers the unchanged first generation");
    if (!first_cold) return;
    const auto cold_prior = first_cold.session.snapshot().value;
    check(!first_cold.session.reset(5) && !first_cold.session.consume_initialization_notice() &&
          !first_cold.session.consume_initialization_notice_secondary(),
          "reset on loaded profile never fabricates missing-profile startup notices");
    const auto second_commit = first_cold.session.commit(6);
    check(second_commit.outcome == s::CommitOutcome::published && second_commit.generation == 2 &&
          read_file(profile.root / "save.a") == prior_file && same(cold_prior.image(), prior_bytes) &&
          cold_prior.generation() == 1,
          "only explicit commit publishes reset as generation two while preserving prior slot and snapshot");
    first_cold.session = {};
    auto second_cold = s::GameSession::open(profile.root, 999);
    check(bool(second_cold), "cold reopen of explicitly committed reset succeeds");
    if (!second_cold) return;
    const auto cold_reset = second_cold.session.snapshot().value;
    check(bool(s::seal_image(expected_defaults, 6)) && cold_reset.generation() == 2 &&
          same(cold_reset.image(), expected_defaults) && !cold_reset.player_flag(15).value &&
          !second_cold.session.consume_initialization_notice() &&
          !second_cold.session.consume_initialization_notice_secondary(),
          "second cold generation contains exact sealed defaults, not stale source state or invented notices");
    s::GameSession absent;
    check(absent.reset(7).code == s::GameErrc::unavailable, "empty session cannot reset");
}

void preservation_and_errors() {
    Profile seed;
    auto initial = s::GameSession::open(seed.root, 1);
    const auto good = initial.session.snapshot().value;
    const auto a = envelope(good.image(), 7);
    initial.session = {};
    for (unsigned mode = 0; mode < 4; ++mode) {
        Profile profile;
        write_file(profile.root / "save.a", a);
        auto b = a;
        if (mode == 0) b.back() ^= std::byte{1};
        if (mode == 1) b[9] = std::byte{2}; // Unknown envelope version, never migration/new game.
        if (mode == 2) {
            auto other = copy(good.image());
            put32(other, 0x34, 23);
            const auto sealed = s::seal_image(other, 2);
            b = envelope(sealed.image, 7); // Equal generation, unequal valid payloads.
        }
        if (mode == 3) {
            auto invalid = copy(good.image());
            put32(invalid, 0x50, 7); // Structurally sealed but impossible source course.
            b = envelope(s::seal_image(invalid, 2).image, 8);
        }
        write_file(profile.root / "save.b", b);
        auto rejected = s::GameSession::open(profile.root, 99);
        const auto expected = mode == 2 ? s::GameLoadState::conflict :
                              mode == 3 ? s::GameLoadState::invalid_state : s::GameLoadState::recovery_required;
        check(!rejected && rejected.state == expected && !rejected.session.snapshot() &&
              !rejected.session.consume_initialization_notice() && !rejected.session.consume_initialization_notice_secondary(),
              "corrupt/future/conflicting/impossible source state never initializes a new game");
        check(rejected.session.commit(100).outcome == s::CommitOutcome::not_published &&
              rejected.session.set_player_flag(9, true).code == s::GameErrc::unavailable &&
              rejected.session.reset(101).code == s::GameErrc::unavailable &&
              read_file(profile.root / "save.a") == a && read_file(profile.root / "save.b") == b,
              "rejected startup cannot mutate either durable candidate");
        if (mode < 2) check(rejected.load.snapshot && rejected.load.snapshot.generation() == 7 &&
                           same(rejected.load.snapshot.image(), good.image().bytes()), "recovery retains last good immutable Store evidence");
        if (mode == 2) check(!rejected.load.snapshot, "conflict never selects a winner");
        if (mode == 3) check(rejected.error.code == s::GameErrc::source_state, "invalid source state has checked semantic error");
    }
    for (unsigned mode = 0; mode < 2; ++mode) {
        Profile profile;
        auto rejected_bytes = a;
        if (mode == 0) rejected_bytes.back() ^= std::byte{1};
        if (mode == 1) rejected_bytes[9] = std::byte{2};
        write_file(profile.root / "save.a", rejected_bytes);
        auto rejected = s::GameSession::open(profile.root, 99);
        check(!rejected && rejected.state == s::GameLoadState::recovery_required &&
              rejected.load.slots[1].state == s::SlotState::missing && !rejected.load.snapshot &&
              !rejected.session.consume_initialization_notice() &&
              rejected.session.commit(100).outcome == s::CommitOutcome::not_published &&
              rejected.session.reset(101).code == s::GameErrc::unavailable &&
              read_file(profile.root / "save.a") == rejected_bytes && !fs::exists(profile.root / "save.b"),
              "one rejected candidate plus missing peer is never a genuinely missing profile");
        if (mode == 1) check(rejected.load.slots[0].error.codec_error == s::Error::envelope_version,
                             "future envelope rejection retains exact codec evidence");
    }
    Profile broken;
    auto no_directory = s::GameSession::open(broken.root / "absent", 1);
    check(!no_directory && no_directory.state == s::GameLoadState::io_error &&
          no_directory.store_error.phase == s::StorePhase::open_directory &&
          !fs::exists(broken.root / "absent"), "missing directory is I/O failure, not missing profile or implicit mkdir");
    auto relative = s::GameSession::open("relative-profile", 1);
    check(!relative && relative.store_error.code == s::StoreErrc::invalid_argument, "ambient relative profile forbidden");
    Profile locked;
    auto owner = s::GameSession::open(locked.root, 1);
    auto contender = s::GameSession::open(locked.root, 1);
    check(!contender && contender.store_error.code == s::StoreErrc::lock_busy, "session retains Store's cooperative ownership lock");
    check(owner.session.commit(2).outcome == s::CommitOutcome::published, "publish real prior generation for failure preservation");
    const auto durable = read_file(locked.root / "save.a");
    const auto retained = owner.session.snapshot().value;
    fs::create_directory(locked.root / "save.b");
    check(!owner.session.set_player_flag(9, true), "edit before genuine install failure");
    const auto failed = owner.session.commit(3);
    check(failed.outcome == s::CommitOutcome::indeterminate && failed.generation == 0 &&
          read_file(locked.root / "save.a") == durable && retained.generation() == 1 &&
          !retained.player_flag(9).value && bool(s::inspect_image(retained.image().bytes())),
          "indeterminate native commit preserves last good bytes/generation and immutable snapshot");
    check(owner.session.consume_initialization_notice() && owner.session.consume_initialization_notice_secondary() &&
          owner.session.commit(4).outcome == s::CommitOutcome::not_published,
          "publication failure neither consumes independent source notices nor permits another write");
    check(owner.session.reset(5).code == s::GameErrc::unavailable &&
          read_file(locked.root / "save.a") == durable,
          "frozen indeterminate session cannot reset or overwrite the prior good generation");
    s::GameSnapshot empty;
    check(!empty && !empty.image() && !empty.player_flag(0) && empty.player_name().empty(), "empty snapshot never fabricates fields");
}
} // namespace
int main(int argc, char** argv) {
    try {
        if (argc > 2) throw std::runtime_error("usage: game-save-tests [existing-owned-parent-directory]");
        parent_directory = argc == 2 ? fs::absolute(argv[1]) : fs::temp_directory_path();
        defaults_and_roundtrip();
        explicit_reset_and_cold_generations();
        preservation_and_errors();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
    if (failures) return EXIT_FAILURE;
    std::cout << "GAME startup save defaults, ownership, durable cold reload and failure preservation passed\n";
    return EXIT_SUCCESS;
}
