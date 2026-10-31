#include "game_run_report.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#if !defined(_WIN32)
#include <sys/stat.h>
#endif

namespace report = isle::game_run_report;
namespace app = isle::application;
namespace host = isle::game_host;
namespace save = isle::save;
namespace fs = std::filesystem;
namespace {
void require(bool condition, const char* meaning) {
    if (!condition) throw std::runtime_error(meaning);
}
struct PrivateDirectory {
    fs::path path;
    PrivateDirectory() {
        static unsigned sequence = 0;
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        for (unsigned attempt = 0; attempt != 100; ++attempt) {
            auto candidate = fs::temp_directory_path() /
                ("35mmisle-terminal-" + std::to_string(stamp) + "-" + std::to_string(++sequence));
            if (fs::create_directory(candidate)) { path = std::move(candidate); return; }
        }
        throw std::runtime_error("cannot acquire isolated terminal evidence test directory");
    }
    ~PrivateDirectory() { std::error_code ignored; fs::remove_all(path, ignored); }
};
std::vector<std::byte> bytes(const fs::path& path) {
    std::ifstream input(path, std::ios::binary);
    require(bool(input), "terminal payload cannot be opened");
    const std::vector<char> chars{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    std::vector<std::byte> result(chars.size());
    std::transform(chars.begin(), chars.end(), result.begin(), [](char c) { return std::byte(static_cast<unsigned char>(c)); });
    return result;
}
std::string text(const fs::path& path) {
    std::ifstream input(path, std::ios::binary);
    require(bool(input), "terminal report cannot be opened");
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}
std::string_view value(std::string_view object, const char* key) {
    const auto needle = "\"" + std::string(key) + "\":";
    const auto position = object.find(needle);
    require(position != std::string_view::npos, "consumer-visible report field missing");
    return object.substr(position + needle.size());
}
std::uint64_t number(std::string_view object, const char* key) {
    const auto input = value(object, key);
    std::uint64_t result = 0;
    const auto parsed = std::from_chars(input.data(), input.data() + input.size(), result);
    require(parsed.ec == std::errc{}, "consumer-visible report number invalid");
    return result;
}
bool flag(std::string_view object, const char* key) {
    const auto input = value(object, key);
    if (input.substr(0, 4) == "true") return true;
    require(input.substr(0, 5) == "false", "consumer-visible report boolean invalid");
    return false;
}
std::string_view row(std::string_view document, std::uint32_t index) {
    const auto needle = "\n{\"row\":" + std::to_string(index) + ",";
    const auto begin = document.find(needle);
    require(begin != std::string_view::npos, "consumer-visible correlated row missing");
    const auto end = document.find('\n', begin + 1);
    return document.substr(begin + 1, end - begin - 1);
}
std::string write_report(report::Recorder& recorder, const fs::path& directory) {
    std::array<char, 512> error{};
    require(recorder.write(directory, error), error.data());
    auto result = text(directory / "report.json");
    return result;
}
report::EpochRetirement clean_epoch(std::uint64_t epoch) {
    return {epoch, true, false, false, true, true, true, true};
}
host::Shutdown clean_host() {
    return {isle::audio::SdlAudioOutput::CloseDisposition::stream_drained, true, false};
}

// These are isolated recorder event traces, not Runtime/Host mocks and not
// permanent fixtures claiming a real campaign. Real shipping qualification must
// supply actual callbacks, presentations, pixels and independent OS custody.
struct Trace {
    report::Recorder recorder;
    std::uint64_t epoch = 1, serial = 0, ordinal = 0, offer = 0;
    app::EntryIntent current{};
    report::FrameIdentity last{};
    explicit Trace(report::Purpose purpose = report::Purpose::campaign) : recorder(purpose) {}
    report::FrameIdentity begin(IsleApplicationEntryOperation operation, unsigned argument) {
        current = {++serial, operation, argument};
        ordinal = 0;
        return frame();
    }
    report::FrameIdentity frame() {
        last = recorder.game_frame(epoch, current, ++ordinal);
        require(last.row != report::no_row, "isolated trace exhausted unexpectedly");
        return last;
    }
    void queue() { recorder.main_queued(last, ++offer); }
    void present() { queue(); recorder.main_presented(host::Present::completed, false); }
    void retire(std::int32_t result, save::GameSnapshot snapshot = {},
                std::optional<app::BootLoadPublication> boot = {}) {
        app::SceneRetirementView view{};
        view.entry = &current;
        view.source_result = result;
        view.published_save = std::move(snapshot);
        view.boot_load = std::move(boot);
        recorder.game_retired(epoch, view);
    }
    void entry(IsleApplicationEntryOperation operation, unsigned argument, std::int32_t result,
               save::GameSnapshot snapshot = {}, std::optional<app::BootLoadPublication> boot = {}) {
        begin(operation, argument); present(); retire(result, std::move(snapshot), std::move(boot));
    }
    void close(report::EpochRetirement retirement, host::Shutdown shutdown = clean_host(), bool cancelled = true) {
        recorder.game_epoch_retired(retirement);
        recorder.main_host_shutdown(epoch, cancelled, shutdown);
        recorder.joined();
    }
    void close() { close(clean_epoch(epoch)); }
};
struct Publications {
    PrivateDirectory profile;
    save::GameOpenResult opened;
    app::BootLoadPublication missing;
    Publications() : opened(save::GameSession::open(profile.path, 1)), missing{opened.state, opened.load} {
        require(bool(opened), "cannot create genuine missing canonical publication fixture");
        require(missing.state == save::GameLoadState::initialized_missing, "fixture was not genuinely missing");
    }
    save::GameSnapshot snapshot() {
        const auto result = opened.session.snapshot();
        require(bool(result), "cannot obtain isolated owning canonical snapshot");
        return result.value;
    }
    save::GameSnapshot commit(unsigned time) {
        require(opened.session.commit(time).outcome == save::CommitOutcome::published, "isolated canonical commit failed");
        return snapshot();
    }
    save::GameSnapshot terminal() {
        for (const unsigned id : {0u, 1u, 2u, 5u})
            require(!opened.session.set_player_flag(id, true), "cannot author isolated gear predicate input");
        std::array<std::byte, save::game_photo_bytes> photo{};
        for (const std::size_t slot : {0u, 61u, 62u, 63u, 64u, 65u, 66u, 67u, 68u})
            require(!opened.session.set_photo(save::PhotoSet::report, slot, photo), "cannot author isolated photo-presence predicate input");
        return commit(2);
    }
    app::BootLoadPublication reopen() {
        opened.session = {};
        opened = save::GameSession::open(profile.path, 100);
        require(bool(opened) && opened.state == save::GameLoadState::ready, "cannot obtain actual owning Boot-ready load output");
        return {opened.state, opened.load};
    }
};
void campaign_prefix(Trace& trace, const app::BootLoadPublication& boot, const save::GameSnapshot& terminal,
                     int bad_course = -1, bool omit_credit_retirement = false) {
    trace.entry(ISLE_APPLICATION_ENTRY_BOOT, 0, 0, {}, boot);
    trace.entry(ISLE_APPLICATION_ENTRY_TITLE, 8, 0);
    trace.entry(ISLE_APPLICATION_ENTRY_NAME, 0, 11);
    trace.entry(ISLE_APPLICATION_ENTRY_OAKS_LAB, 11, 0);
    for (unsigned course = 0; course != 7; ++course)
        trace.entry(static_cast<IsleApplicationEntryOperation>(ISLE_APPLICATION_ENTRY_BEACH + course), course,
            bad_course == static_cast<int>(course) ? 8 : course % 2 ? 13 : 9);
    trace.entry(ISLE_APPLICATION_ENTRY_OAKS_LAB, 13, 18, terminal);
    trace.begin(ISLE_APPLICATION_ENTRY_CREDITS, 18);
    trace.present();
    if (!omit_credit_retirement) trace.retire(8);
    trace.begin(ISLE_APPLICATION_ENTRY_TITLE, 8);
}
void finish_title(Trace& trace, unsigned presentations) {
    for (unsigned i = 0; i != presentations; ++i) {
        if (i) trace.frame();
        trace.present();
    }
}
void initial_oak_save_title_continue_detour_remains_eligible() {
    Publications publications;
    const auto first_save = publications.commit(1);
    const auto terminal = publications.terminal();
    Trace trace;
    trace.entry(ISLE_APPLICATION_ENTRY_BOOT, 0, 0, {}, publications.missing);
    trace.entry(ISLE_APPLICATION_ENTRY_TITLE, 8, 0);
    trace.entry(ISLE_APPLICATION_ENTRY_NAME, 0, 11);
    trace.entry(ISLE_APPLICATION_ENTRY_OAKS_LAB, 11, 8, first_save);
    trace.entry(ISLE_APPLICATION_ENTRY_TITLE, 8, 11);
    trace.entry(ISLE_APPLICATION_ENTRY_OAKS_LAB, 11, 0);
    for (unsigned course = 0; course != 7; ++course)
        trace.entry(static_cast<IsleApplicationEntryOperation>(ISLE_APPLICATION_ENTRY_BEACH + course),
            course, course % 2 ? 13 : 9);
    trace.entry(ISLE_APPLICATION_ENTRY_OAKS_LAB, 13, 18, terminal);
    trace.entry(ISLE_APPLICATION_ENTRY_CREDITS, 18, 8);
    trace.begin(ISLE_APPLICATION_ENTRY_TITLE, 8);
    finish_title(trace, 120);
    trace.close();
    require(trace.recorder.native_complete(), "legitimate first-Oak Save/Title/Continue detour rejected a complete campaign");
    PrivateDirectory output;
    const auto document = write_report(trace.recorder, output.path);
    const auto image = terminal.image().bytes();
    require(number(document, "terminal_epoch") == 1 &&
        number(document, "naturally_retired_presented_course_mask") == 127 &&
        number(document, "final_Title_actual_completed_presentations") == 120 &&
        number(document, "terminal_Save_generation") == terminal.generation() &&
        bytes(output.path / "terminal-save.bin") == std::vector<std::byte>(image.begin(), image.end()),
        "first-Oak detour lost final-epoch course, presentation or whole canonical Save provenance");
}
void late_completion_stays_with_original_entry() {
    Trace trace;
    const auto original = trace.begin(ISLE_APPLICATION_ENTRY_TITLE, 8);
    trace.queue();
    trace.retire(0);
    const auto later = trace.begin(ISLE_APPLICATION_ENTRY_NAME, 0);
    trace.recorder.main_presented(host::Present::declined, true);
    trace.recorder.main_presented(host::Present::completed, false);
    trace.close();
    PrivateDirectory output;
    const auto document = write_report(trace.recorder, output.path);
    require(number(row(document, original.row), "actual_completed_presentations") == 1,
        "late completed presentation lost its original entry credit");
    require(number(row(document, later.row), "actual_completed_presentations") == 0,
        "late completed presentation was falsely attributed to current GAME entry");
    require(number(row(document, original.row), "declined_attempts") == 1,
        "declined pending presentation identity was not retained");
    require(flag(document, "evidence_valid"), "ordinary cross-entry late completion invalidated evidence");
    require(!trace.recorder.native_complete(), "partial ingress became a native campaign proof");
}
void pending_decline_cancel_replace_and_failure_have_no_credit() {
    Trace trace;
    const auto identity = trace.begin(ISLE_APPLICATION_ENTRY_TITLE, 8);
    trace.queue();
    trace.recorder.main_presented(host::Present::declined, true);
    trace.frame(); trace.queue(); // Replaces the first pending copy, not a presentation.
    trace.recorder.main_presented(host::Present::declined, false);
    trace.frame(); trace.queue(); trace.recorder.main_cancelled(true);
    trace.frame(); trace.queue();
    trace.recorder.main_presented(host::Present::failed, true);
    trace.recorder.main_cancelled(true);
    trace.frame(); trace.queue();
    trace.recorder.main_presented(host::Present::declined, true);
    trace.recorder.main_presented(host::Present::idle, true);
    trace.recorder.main_presented(host::Present::completed, false);
    trace.close();
    PrivateDirectory output;
    const auto document = write_report(trace.recorder, output.path);
    const auto emitted = row(document, identity.row);
    require(number(emitted, "queued") == 5 && number(emitted, "actual_completed_presentations") == 1,
        "pending/decline/cancel/failed frames acquired presentation credit");
    require(number(emitted, "replaced_pending") == 1 && number(emitted, "cancelled_pending") == 2 &&
        number(emitted, "dropped_without_completion") == 1 && number(emitted, "failed_attempts") == 1,
        "zero-credit terminal dispositions were lost");
    require(number(emitted, "declined_attempts") == 3 && number(emitted, "last_completed_frame_ordinal") == 5,
        "declines altered exact eventual-completion frame attribution");
    require(!flag(document, "evidence_valid"), "Host presentation failure remained positive evidence");
}
void cancelled_completion_and_forged_row_are_not_proof() {
    {
        Trace trace;
        trace.begin(ISLE_APPLICATION_ENTRY_TITLE, 8); trace.queue();
        trace.recorder.main_cancelled(true);
        trace.recorder.main_presented(host::Present::completed, false);
        trace.close();
        PrivateDirectory output;
        const auto document = write_report(trace.recorder, output.path);
        require(number(row(document, 0), "actual_completed_presentations") == 0 &&
            number(document, "uncorrelated_completions") == 1 && !flag(document, "evidence_valid"),
            "completion after cancelled identity was accepted");
    }
    {
        Trace trace;
        trace.begin(ISLE_APPLICATION_ENTRY_TITLE, 8); trace.queue();
        trace.recorder.main_presented(host::Present::failed, true);
        trace.recorder.main_presented(host::Present::completed, false);
        trace.close();
        PrivateDirectory output;
        const auto document = write_report(trace.recorder, output.path);
        require(number(row(document, 0), "actual_completed_presentations") == 0 &&
            number(row(document, 0), "dropped_without_completion") == 1 && !flag(document, "evidence_valid"),
            "a failed pending frame later acquired completion credit");
    }
    {
        Trace trace;
        auto identity = trace.begin(ISLE_APPLICATION_ENTRY_TITLE, 8);
        ++identity.entry.serial;
        trace.recorder.main_queued(identity, 1);
        trace.recorder.main_presented(host::Present::completed, false);
        trace.close();
        PrivateDirectory output;
        const auto document = write_report(trace.recorder, output.path);
        require(!flag(document, "evidence_valid") && !flag(row(document, 0), "Host_row_identity_matches"),
            "foreign entry forged a matching presentation row");
    }
}
void epoch_reused_serial_and_ordinal_remain_distinct() {
    Trace trace;
    const auto first = trace.begin(ISLE_APPLICATION_ENTRY_TITLE, 8);
    trace.queue(); trace.retire(0);
    trace.recorder.game_epoch_retired(clean_epoch(1));
    trace.epoch = 2; trace.serial = 0;
    const auto second = trace.begin(ISLE_APPLICATION_ENTRY_TITLE, 8);
    trace.recorder.main_presented(host::Present::completed, false);
    trace.recorder.main_host_shutdown(1, true, clean_host());
    trace.present(); trace.close();
    PrivateDirectory output;
    const auto document = write_report(trace.recorder, output.path);
    require(first.row != second.row && first.entry.serial == second.entry.serial && first.frame_ordinal == second.frame_ordinal,
        "test did not exercise a real scalar serial/ordinal epoch collision");
    require(number(row(document, first.row), "epoch") == 1 && number(row(document, second.row), "epoch") == 2 &&
        number(row(document, first.row), "actual_completed_presentations") == 1 &&
        number(row(document, second.row), "actual_completed_presentations") == 1 && flag(document, "evidence_valid"),
        "epoch collision stole or merged actual original pending completion");
}
void threshold_and_external_scope_are_exact() {
    Publications publications;
    const auto terminal = publications.terminal();
    for (const unsigned count : {119u, 120u}) {
        Trace trace;
        campaign_prefix(trace, publications.missing, terminal);
        finish_title(trace, count);
        // One further frame remains declined/cancelled. It is not the 120th.
        trace.frame(); trace.queue();
        trace.recorder.main_presented(host::Present::declined, true);
        trace.close();
        require(trace.recorder.native_complete() == (count == 120), "final Title threshold was not actual 119/120 completions");
        PrivateDirectory output;
        const auto document = write_report(trace.recorder, output.path);
        require(flag(document, "native_boundary_complete") == (count == 120) &&
            number(document, "final_Title_actual_completed_presentations") == count,
            "serialized final Title threshold differs from recorder evidence");
        require(!flag(document, "first_durable_Mew_publication_is_acquisition"),
            "durable Mew publication was mislabeled acquisition evidence");
        const auto publication = value(row(document, 11), "retired_Save_publication");
        require(number(publication, "ordinary_slots_0_62_count") == 3 &&
            number(publication, "non_mew_slots_0_61_count") == 2 &&
            number(publication, "sign_slots_63_68_mask") == 63,
            "ordinary/non-Mew/sign count terminology conflated Mew or signs");
        require(bytes(output.path / "terminal-save.bin").size() == save::image_bytes,
            "terminal serializer truncated canonical Save output");
    }
}
void missing_faulted_or_cancelled_boundary_cannot_complete() {
    Publications publications;
    const auto terminal = publications.terminal();
    for (unsigned variant = 0; variant != 13; ++variant) {
        Trace trace;
        campaign_prefix(trace, publications.missing, terminal, variant == 0 ? 6 : -1, variant == 1);
        finish_title(trace, 120);
        auto end = clean_epoch(1);
        auto shutdown = clean_host();
        bool cancel = true;
        switch (variant) {
        case 2: end.successful = false; break;
        case 3: end.runtime_retired = false; break;
        case 4: end.input_stopped = false; break;
        case 5: end.audio_drained = false; break;
        case 6: end.driver_retired = false; break;
        case 7: shutdown.owners_released = false; break;
        case 8: shutdown.audio = isle::audio::SdlAudioOutput::CloseDisposition::aborted; break;
        case 9: shutdown.failed = true; break;
        case 10: cancel = false; break;
        case 11: end.reset_requested = true; break;
        case 12: end.disconnect_requested = true; break;
        default: break;
        }
        trace.close(end, shutdown, cancel);
        require(trace.recorder.readable() && !trace.recorder.native_complete(),
            "missing/faulted/cancelled native boundary became a positive proof");
    }
    Trace missing_epoch;
    campaign_prefix(missing_epoch, publications.missing, terminal);
    finish_title(missing_epoch, 120);
    missing_epoch.recorder.main_host_shutdown(1, true, clean_host());
    missing_epoch.recorder.joined();
    require(missing_epoch.recorder.readable() && !missing_epoch.recorder.native_complete(),
        "missing actual GAME epoch retirement was inferred from campaign success");
}
void prior_epoch_cannot_supply_final_native_credit_or_payload() {
    Publications publications;
    const auto terminal = publications.terminal();
    Trace trace;
    campaign_prefix(trace, publications.missing, terminal);
    finish_title(trace, 120);
    trace.recorder.game_epoch_retired(clean_epoch(1));
    trace.recorder.main_host_shutdown(1, true, clean_host());
    trace.epoch = 2; trace.serial = 0;
    trace.entry(ISLE_APPLICATION_ENTRY_BOOT, 0, 0); // No completed publication.
    trace.begin(ISLE_APPLICATION_ENTRY_TITLE, 8); trace.present();
    trace.close();
    PrivateDirectory output;
    const auto document = write_report(trace.recorder, output.path);
    require(!trace.recorder.native_complete() && number(document, "terminal_epoch") == 2 &&
        number(document, "naturally_retired_presented_course_mask") == 0 &&
        number(document, "terminal_Save_generation") == 0 &&
        !fs::exists(output.path / "terminal-save.bin"),
        "earlier complete epoch supplied native credit or stale latest Save to a later reset epoch");
}
void prior_closed_attempt_does_not_ban_final_fresh_epoch() {
    Publications publications;
    const auto terminal = publications.terminal();
    Trace trace;
    trace.begin(ISLE_APPLICATION_ENTRY_TITLE, 8); trace.present();
    auto first_end = clean_epoch(1); first_end.reset_requested = true;
    trace.recorder.game_epoch_retired(first_end);
    trace.recorder.main_host_shutdown(1, true, clean_host());
    trace.epoch = 2; trace.serial = 0;
    campaign_prefix(trace, publications.missing, terminal);
    finish_title(trace, 120); trace.close();
    require(trace.recorder.native_complete(), "an independently closed earlier reset banned a complete final genuine fresh epoch");
}
void publication_generations_have_gaps_not_invented_every_acks() {
    Publications publications;
    const auto zero = publications.snapshot();
    const auto first = publications.commit(2);
    const auto second = publications.commit(3);
    const auto third = publications.commit(4);
    for (unsigned variant = 0; variant != 4; ++variant) {
        Trace trace;
        trace.entry(ISLE_APPLICATION_ENTRY_OAKS_LAB, 13, 8, variant == 2 ? third : first);
        trace.entry(ISLE_APPLICATION_ENTRY_OAKS_LAB, 13, 8,
            variant == 0 ? third : variant == 1 ? first : variant == 2 ? second : zero);
        trace.close();
        PrivateDirectory output;
        const auto document = write_report(trace.recorder, output.path);
        require(flag(document, "evidence_valid") == (variant == 0),
            "nonzero strictly increasing observed publication policy accepted duplicate/reversed/zero or rejected gaps");
        require(number(document, "retired_Save_publications") == 2,
            "generation gaps invented unobserved ACK publication rows");
    }
    Trace trace;
    trace.entry(ISLE_APPLICATION_ENTRY_OAKS_LAB, 13, 8, third);
    trace.recorder.game_epoch_retired(clean_epoch(1));
    trace.recorder.main_host_shutdown(1, true, clean_host());
    trace.epoch = 2; trace.serial = 0;
    trace.entry(ISLE_APPLICATION_ENTRY_OAKS_LAB, 13, 8, first);
    trace.close();
    PrivateDirectory output;
    const auto document = write_report(trace.recorder, output.path);
    require(flag(document, "evidence_valid") && number(document, "terminal_Save_epoch") == 2 &&
        number(document, "terminal_Save_generation") == 1,
        "new application epoch illegally inherited prior profile generation ordering");
}
void publications_are_owning_and_whole_after_callback_expiry() {
    PrivateDirectory output;
    Trace trace;
    std::vector<std::byte> expected;
    {
        Publications publications;
        auto immutable = publications.terminal();
        expected.assign(immutable.image().bytes().begin(), immutable.image().bytes().end());
        trace.entry(ISLE_APPLICATION_ENTRY_OAKS_LAB, 13, 8, immutable);
        immutable = {};
        require(!publications.opened.session.set_player_flag(0, false), "cannot mutate source after retired publication");
        (void)publications.commit(5);
    } // View, entry temporary and profile/session are all gone before serialization.
    trace.close();
    const auto document = write_report(trace.recorder, output.path);
    require(bytes(output.path / "terminal-save.bin") == expected && number(document, "terminal_Save_generation") == 1 &&
        number(value(row(document, 0), "retired_Save_publication"), "gear_flags_0_1_2_5_mask") == 15,
        "retired publication borrowed callback/session bytes or reread mutable live profile state");
}
void boot_ready_requires_actual_owning_load_and_continue() {
    Publications publications;
    const auto terminal = publications.terminal();
    auto ready = publications.reopen();
    const auto original = ready.load.snapshot.image().bytes();
    const std::vector<std::byte> expected(original.begin(), original.end());
    require(!publications.opened.session.set_player_flag(0, false), "cannot mutate loaded session for immutable Boot test");
    (void)publications.commit(6);
    for (unsigned variant = 0; variant != 7; ++variant) {
        Trace trace(report::Purpose::cold_boot);
        auto boot = ready;
        if (variant == 1) boot.load.snapshot = {};
        if (variant == 2) boot.state = save::GameLoadState::initialized_missing;
        if (variant == 3) boot = publications.missing;
        if (variant == 4) boot.load.state = save::LoadState::recovery_required;
        trace.entry(ISLE_APPLICATION_ENTRY_BOOT, 0, 0, {}, boot);
        trace.entry(ISLE_APPLICATION_ENTRY_TITLE, 8, variant == 5 ? 0 : 11);
        trace.begin(ISLE_APPLICATION_ENTRY_OAKS_LAB, variant == 6 ? 13 : 11);
        trace.present(); trace.close();
        require(trace.recorder.native_complete() == (variant == 0),
            "Boot-ready state, missing image, fresh initialization or non-Continue Oak substituted for genuine cold-boot boundary");
        PrivateDirectory output;
        const auto document = write_report(trace.recorder, output.path);
        if (variant == 0) {
            require(bytes(output.path / "boot-load.bin") == expected && number(document, "Boot_load_generation") == 1 &&
                number(document, "Boot_load_epoch") == 1 && !fs::exists(output.path / "terminal-save.bin"),
                "completed Boot did not retain exact whole owning original load after mutable session changed");
        }
        if (variant == 1 || variant == 3)
            require(!fs::exists(output.path / "boot-load.bin"), "missing Boot image was fabricated or reread");
    }
    Trace not_fresh;
    campaign_prefix(not_fresh, ready, terminal);
    finish_title(not_fresh, 120); not_fresh.close();
    require(!not_fresh.recorder.native_complete(), "loaded Boot was accepted as genuine fresh campaign initialization");
}
void postjoin_all_host_receipts_gate_readability_and_output() {
    Trace trace;
    trace.begin(ISLE_APPLICATION_ENTRY_TITLE, 8); trace.queue();
    PrivateDirectory output;
    std::array<char, 512> error{};
    require(!trace.recorder.readable() && !trace.recorder.write(output.path, error) && fs::is_empty(output.path),
        "live evidence was serialized before actual worker join");
    trace.recorder.game_epoch_retired(clean_epoch(1));
    trace.recorder.joined();
    require(!trace.recorder.readable() && !trace.recorder.write(output.path, error) && fs::is_empty(output.path),
        "joined worker bypassed still-unobserved Host shutdown");
    trace.recorder.main_host_shutdown(1, true, clean_host());
    require(trace.recorder.readable(), "actual Host receipt after actual join did not unlock terminal readability");
    const auto document = write_report(trace.recorder, output.path);
    require(!flag(document, "pending_identity_remains") && number(row(document, 0), "actual_completed_presentations") == 0 &&
        number(row(document, 0), "cancelled_pending") == 1,
        "shutdown cancellation acquired completion credit or retained a cancelled identity");
}
void output_refuses_existing_fifo_and_symlink_destinations() {
    Trace trace;
    trace.begin(ISLE_APPLICATION_ENTRY_TITLE, 8); trace.present(); trace.close();
    for (const char* name : {"report.json", "terminal-save.bin", "boot-load.bin"}) {
        PrivateDirectory output;
        { std::ofstream existing(output.path / name, std::ios::binary); existing << "preserve"; }
        std::array<char, 512> error{};
        require(!trace.recorder.write(output.path, error) && text(output.path / name) == "preserve",
            "terminal serializer overwrote an existing reserved destination");
        if (std::string_view(name) != "report.json")
            require(!fs::exists(output.path / "report.json"), "reserved-destination refusal created misleading report");
    }
#if !defined(_WIN32)
    {
        PrivateDirectory output;
        require(mkfifo((output.path / "boot-load.bin").c_str(), 0600) == 0, "cannot create isolated FIFO refusal case");
        std::array<char, 512> error{};
        require(!trace.recorder.write(output.path, error) && !fs::exists(output.path / "report.json"),
            "terminal serializer accepted or blocked on reserved FIFO destination");
    }
    {
        PrivateDirectory output;
        PrivateDirectory target;
        { std::ofstream victim(target.path / "untouched"); victim << "preserve"; }
        fs::create_symlink(target.path / "untouched", output.path / "report.json");
        std::array<char, 512> error{};
        require(!trace.recorder.write(output.path, error) && text(target.path / "untouched") == "preserve",
            "terminal serializer followed symlink file destination");
    }
    for (const char* suffix : {"", "/", "//"}) {
        PrivateDirectory output;
        PrivateDirectory target;
        fs::create_directory_symlink(target.path, output.path / "linked");
        auto requested = output.path / "linked";
        requested += suffix;
        std::array<char, 512> error{};
        require(!trace.recorder.write(requested, error) && fs::is_empty(target.path),
            "terminal serializer followed symlink output directory");
    }
#endif
    {
        PrivateDirectory output;
        const auto first = write_report(trace.recorder, output.path);
        std::array<char, 512> error{};
        require(!trace.recorder.write(output.path, error) && text(output.path / "report.json") == first,
            "repeat terminal serialization overwrote its prior evidence");
    }
}
void epoch_exhaustion_cannot_bypass_actual_final_host_shutdown() {
    report::Recorder recorder(report::Purpose::campaign);
    for (std::uint64_t epoch = 1; epoch <= report::boundary_capacity; ++epoch) {
        recorder.game_epoch_retired(clean_epoch(epoch));
        recorder.main_host_shutdown(epoch, true, clean_host());
    }
    const auto extra_epoch = static_cast<std::uint64_t>(report::boundary_capacity) + 1;
    const app::EntryIntent entry{1, ISLE_APPLICATION_ENTRY_TITLE, 8};
    const auto identity = recorder.game_frame(extra_epoch, entry, 1);
    require(identity.row == report::no_row, "epoch exhaustion fabricated frame evidence");
    recorder.main_queued(identity, 1);
    recorder.game_epoch_retired(clean_epoch(extra_epoch));
    recorder.joined();
    require(!recorder.readable(), "exhausted epoch ledger bypassed actual final Host shutdown barrier");
    recorder.main_host_shutdown(extra_epoch, true, clean_host());
    require(recorder.readable() && !recorder.native_complete(),
        "actual overflow-epoch shutdown did not preserve readable negative evidence");
}

void bounded_exhaustion_stays_negative_without_changing_caller() {
    report::Recorder recorder(report::Purpose::campaign);
    for (std::size_t i = 0; i != report::boundary_capacity; ++i) {
        app::EntryIntent entry{static_cast<std::uint64_t>(i) + 1, ISLE_APPLICATION_ENTRY_TITLE, 8};
        app::SceneRetirementView view{};
        view.entry = &entry;
        recorder.game_retired(1, view);
    }
    app::EntryIntent extra{report::boundary_capacity + 1, ISLE_APPLICATION_ENTRY_TITLE, 8};
    require(recorder.game_frame(1, extra, 1).row == report::no_row,
        "bounded exhaustion fabricated a row instead of declining evidence only");
    recorder.game_epoch_retired(clean_epoch(1));
    recorder.main_host_shutdown(1, true, clean_host());
    recorder.joined();
    require(recorder.readable() && !recorder.native_complete(),
        "bounded exhaustion was hidden or prevented negative postclosure evidence");
}
} // namespace

int main() try {
    late_completion_stays_with_original_entry();
    pending_decline_cancel_replace_and_failure_have_no_credit();
    cancelled_completion_and_forged_row_are_not_proof();
    epoch_reused_serial_and_ordinal_remain_distinct();
    threshold_and_external_scope_are_exact();
    initial_oak_save_title_continue_detour_remains_eligible();
    missing_faulted_or_cancelled_boundary_cannot_complete();
    prior_epoch_cannot_supply_final_native_credit_or_payload();
    prior_closed_attempt_does_not_ban_final_fresh_epoch();
    publication_generations_have_gaps_not_invented_every_acks();
    publications_are_owning_and_whole_after_callback_expiry();
    boot_ready_requires_actual_owning_load_and_continue();
    postjoin_all_host_receipts_gate_readability_and_output();
    output_refuses_existing_fifo_and_symlink_destinations();
    bounded_exhaustion_stays_negative_without_changing_caller();
    epoch_exhaustion_cannot_bypass_actual_final_host_shutdown();
    std::cout << "isolated terminal recorder semantic regressions passed; not Runtime qualification\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
}
