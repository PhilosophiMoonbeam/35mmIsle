#include "campaign_saved_profile.hpp"
#include "../src/save_hash.hpp"

#include <algorithm>
#include <array>
#include <exception>
#include <fstream>
#include <limits>
#include <ostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace isle::private_campaign {
namespace {
namespace fs = std::filesystem;
constexpr std::array<const char*, 3> input_names{"save.a", "save.b", "save.lock"};
using Digest = std::array<std::byte, 32>;

void require(bool valid, const char* message) {
    if (!valid) throw std::runtime_error(message);
}

Digest digest(std::span<const std::byte> bytes) noexcept {
    save::detail::Sha256 hash;
    hash.update(bytes);
    return hash.finish();
}

void hex(std::ostream& out, std::span<const std::byte> bytes) {
    constexpr char digits[] = "0123456789abcdef";
    for (const auto byte : bytes) {
        const auto value = std::to_integer<unsigned>(byte);
        out.put(digits[value >> 4]);
        out.put(digits[value & 15]);
    }
}

fs::path checked_directory(const fs::path& path) {
    require(!path.empty() && path.native().find(fs::path::value_type{}) == fs::path::string_type::npos,
        "saved-profile directory path is empty or contains a null character");
    const auto absolute = fs::absolute(path);
    fs::path prefix;
    for (const auto& component : absolute) {
        require(component != "..", "saved-profile directory paths must not contain parent aliases");
        prefix /= component;
        require(!fs::is_symlink(fs::symlink_status(prefix)),
            "saved-profile directory paths must not traverse symlinks");
    }
    require(fs::is_directory(fs::symlink_status(absolute)),
        "saved-profile source and destination must be existing real directories");
    return fs::canonical(absolute);
}

bool contains(const fs::path& parent, const fs::path& child) {
    auto left = parent.begin();
    auto right = child.begin();
    for (; left != parent.end(); ++left, ++right)
        if (right == child.end() || *left != *right) return false;
    return true;
}

struct Record {
    bool present = false;
    fs::perms permissions{};
    fs::file_time_type modified{};
    std::uintmax_t size = 0;
    std::uintmax_t links = 0;
    std::vector<std::byte> bytes;
    Digest sha256{};
};

std::vector<std::byte> read_slot(const fs::path& path, std::uintmax_t size) {
    require(size <= std::numeric_limits<std::size_t>::max() &&
            size <= static_cast<std::uintmax_t>(std::numeric_limits<std::streamsize>::max()),
        "saved-profile slot exceeds host read limits");
    std::vector<std::byte> bytes(static_cast<std::size_t>(size));
    std::ifstream in(path, std::ios::binary);
    require(bool(in), "cannot read preserved saved-profile slot");
    if (!bytes.empty())
        in.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    require(bool(in) && in.peek() == std::char_traits<char>::eof() && !in.bad(),
        "preserved saved-profile slot changed extent or could not be read completely");
    return bytes;
}

struct Inventory {
    std::array<Record, input_names.size()> records;
    fs::perms permissions{};
    fs::file_time_type modified{};
};

Inventory inventory(const fs::path& directory) {
    require(checked_directory(directory) == directory,
        "preserved saved-profile directory identity changed");
    Inventory result;
    result.permissions = fs::symlink_status(directory).permissions();
    result.modified = fs::last_write_time(directory);
    for (const auto& entry : fs::directory_iterator(directory)) {
        const auto name = entry.path().filename();
        std::size_t index = 0;
        while (index < input_names.size() && name != input_names[index]) ++index;
        require(index < input_names.size(),
            "preserved saved-profile contains unsupported inventory or orphan recovery evidence");
        auto& record = result.records[index];
        require(!record.present, "preserved saved-profile has duplicate inventory");
        const auto status = fs::symlink_status(entry.path());
        require(fs::is_regular_file(status),
            "preserved saved-profile records must be regular files, not symlinks or special files");
        record.present = true;
        record.permissions = status.permissions();
        record.modified = fs::last_write_time(entry.path());
        record.size = fs::file_size(entry.path());
        record.links = fs::hard_link_count(entry.path());
        require(record.links == 1, "preserved saved-profile records must not have hard-link aliases");
        if (index == 2) {
            // Never open the source lock, even for a byte read. This private
            // qualification supports its observed regular zero-byte form only.
            require(record.size == 0, "nonempty preserved save.lock is unsupported and will not be discarded");
        } else {
            record.bytes = read_slot(entry.path(), record.size);
        }
        record.sha256 = digest(record.bytes);
    }
    return result;
}

bool same(const Inventory& left, const Inventory& right) {
    if (left.permissions != right.permissions || left.modified != right.modified) return false;
    for (std::size_t i = 0; i < left.records.size(); ++i) {
        const auto& a = left.records[i];
        const auto& b = right.records[i];
        if (a.present != b.present) return false;
        if (a.present && (a.permissions != b.permissions || a.modified != b.modified ||
            a.size != b.size || a.links != b.links || a.bytes != b.bytes)) return false;
    }
    return true;
}

const char* slot_state(save::SlotState state) noexcept {
    switch (state) {
    case save::SlotState::missing: return "missing";
    case save::SlotState::valid: return "valid";
    case save::SlotState::rejected: return "rejected";
    }
    return "unknown";
}

void store_error(std::ostream& out, const save::StoreError& error) {
    out << " code=" << static_cast<unsigned>(error.code)
        << " phase=" << static_cast<unsigned>(error.phase)
        << " native=" << error.native_error
        << " codec=" << static_cast<unsigned>(error.codec_error);
}

template<class T>
T observed(save::GameValue<T> value) {
    require(bool(value), "preserved saved-profile baseline observation unavailable");
    return value.value;
}
} // namespace

struct SavedProfileInput::Impl {
    fs::path source;
    fs::path destination;
    Inventory witness;
    save::GameSnapshot baseline;
    std::array<save::SlotResult, 2> slots;
    Digest image_sha256{};
    mutable std::size_t unchanged_checks = 0;
    mutable bool boot_exact = false;
};

SavedProfileInput::SavedProfileInput(std::unique_ptr<Impl> impl) noexcept : impl_(std::move(impl)) {}
SavedProfileInput::~SavedProfileInput() = default;
SavedProfileInput::SavedProfileInput(SavedProfileInput&&) noexcept = default;
SavedProfileInput& SavedProfileInput::operator=(SavedProfileInput&&) noexcept = default;

SavedProfileInput SavedProfileInput::prepare(const fs::path& source_directory,
                                             const fs::path& new_empty_destination_profile) {
    auto impl = std::make_unique<Impl>();
    impl->source = checked_directory(source_directory);
    impl->destination = checked_directory(new_empty_destination_profile);
    require(!fs::equivalent(impl->source, impl->destination) &&
            !contains(impl->source, impl->destination) && !contains(impl->destination, impl->source),
        "saved-profile source and destination must be independent, nonnested directories");
    require(fs::is_empty(impl->destination), "saved-profile destination must already exist and be empty");
    impl->witness = inventory(impl->source);
    SavedProfileInput input(std::move(impl));
    try {
        for (std::size_t i = 0; i < input_names.size(); ++i) {
            const auto& record = input.impl_->witness.records[i];
            if (!record.present) continue;
            const auto source = input.impl_->source / input_names[i];
            const auto destination = input.impl_->destination / input_names[i];
            if (i == 2) {
                require(!fs::exists(fs::symlink_status(destination)),
                    "copied save.lock destination unexpectedly exists");
                std::ofstream out(destination, std::ios::binary | std::ios::out);
                require(bool(out), "cannot create independent copied zero-byte save.lock");
                out.close();
                require(bool(out), "cannot close independent copied zero-byte save.lock");
            } else {
                require(fs::copy_file(source, destination, fs::copy_options::none),
                    "cannot independently copy preserved saved-profile slot");
            }
            require(fs::is_regular_file(fs::symlink_status(destination)) &&
                    fs::hard_link_count(destination) == 1 && !fs::equivalent(source, destination) &&
                    fs::file_size(destination) == record.size,
                "copied saved-profile record is not an independent exact-extent regular file");
            if (i != 2)
                require(read_slot(destination, record.size) == record.bytes,
                    "copied saved-profile slot differs from preserved input witness");
        }
        {
            auto opened = save::GameSession::open(input.impl_->destination, 0);
            require(opened.state == save::GameLoadState::ready && bool(opened.session) &&
                    opened.load.state == save::LoadState::ready && !opened.store_error &&
                    !opened.error && !opened.load.error && !opened.load.orphan_temporary,
                "copied preserved saved-profile preflight did not load explicitly ready; no repair or fresh fallback allowed");
            const auto snapshot = opened.session.snapshot();
            require(bool(snapshot) && bool(snapshot.value) && bool(snapshot.value.image()) &&
                    snapshot.value.generation() != 0,
                "copied preserved saved-profile preflight has no immutable existing snapshot");
            input.impl_->baseline = snapshot.value;
            input.impl_->slots = opened.load.slots;
            input.impl_->image_sha256 = digest(input.impl_->baseline.image().bytes());
        } // Destroy both copied loader/Store owners and their lock before Boot.
        input.verify_source_unchanged();
    } catch (const std::exception& failure) {
        try {
            input.verify_source_unchanged();
        } catch (const std::exception& changed) {
            throw std::runtime_error(std::string(failure.what()) +
                "; preserved source nonmutation check also failed: " + changed.what());
        }
        throw;
    } catch (...) {
        input.verify_source_unchanged();
        throw;
    }
    return input;
}

const save::GameSnapshot& SavedProfileInput::baseline() const noexcept { return impl_->baseline; }
const fs::path& SavedProfileInput::input_path() const noexcept { return impl_->source; }

void SavedProfileInput::require_boot_load(const save::GameOpenResult& opened,
                                         const save::GameSnapshot& actual) const {
    require(opened.state == save::GameLoadState::ready && opened.load.state == save::LoadState::ready &&
            !opened.store_error && !opened.error && !opened.load.error && !opened.load.orphan_temporary,
        "actual Boot did not explicitly load the existing preserved profile ready");
    require(bool(actual) && bool(actual.image()) && actual.generation() == impl_->baseline.generation(),
        "actual Boot preserved-profile generation differs from copied preflight baseline");
    const auto expected = impl_->baseline.image().bytes();
    const auto loaded = actual.image().bytes();
    require(std::equal(expected.begin(), expected.end(), loaded.begin(), loaded.end()),
        "actual Boot whole canonical image differs from immutable preserved-input baseline");
    impl_->boot_exact = true;
}

void SavedProfileInput::verify_source_unchanged() const {
    require(same(impl_->witness, inventory(impl_->source)),
        "preserved saved-profile source inventory, slot bytes or metadata changed");
    ++impl_->unchanged_checks;
}

void SavedProfileInput::write_evidence(std::ostream& out) const {
    out << "saved_profile_origin=LOADED_PRESERVED_INPUT\n"
        << "saved_profile_natural_origin=NOT_proven_by_copy_loader_or_hashes_external_receipts_required\n"
        << "saved_profile_input=" << impl_->source << '\n'
        << "saved_profile_independent_output=" << impl_->destination << '\n'
        << "saved_profile_supported_inventory=save.a,save.b,optional_regular_zero_byte_save.lock_only\n"
        << "saved_profile_source_witness=exact_inventory_slot_bytes_permissions_mtimes_sizes_link_counts\n"
        << "saved_profile_source_lock=metadata_only_NO_open_or_lock\n"
        << "saved_profile_source_unchanged_checks=" << impl_->unchanged_checks << '\n';
    for (std::size_t i = 0; i < input_names.size(); ++i) {
        const auto& record = impl_->witness.records[i];
        out << "saved_profile_input_" << input_names[i] << "=";
        if (!record.present) {
            out << "absent\n";
            continue;
        }
        out << "regular bytes=" << record.size << " links=" << record.links << " sha256=";
        hex(out, record.sha256);
        out << '\n';
    }
    out << "saved_profile_copied_preflight=ready_NO_repair_NO_fresh_fallback_session_and_lock_released\n";
    for (std::size_t i = 0; i < impl_->slots.size(); ++i) {
        const auto& slot = impl_->slots[i];
        out << "saved_profile_copied_" << input_names[i] << "=" << slot_state(slot.state)
            << " generation=" << slot.generation;
        store_error(out, slot.error);
        out << '\n';
    }
    const auto& snapshot = impl_->baseline;
    out << "saved_profile_inherited_generation=" << snapshot.generation()
        << " canonical_bytes=" << snapshot.image().bytes().size() << " canonical_sha256=";
    hex(out, impl_->image_sha256);
    out << "\nsaved_profile_actual_Boot_ready_generation_whole_image_exact=" << impl_->boot_exact
        << "\nsaved_profile_inherited_Name_source16_hex=";
    hex(out, snapshot.player_name());
    out << "\nsaved_profile_inherited_K=" << observed(snapshot.selected_course())
        << " U=" << observed(snapshot.unlocked_course()) << '\n';
    std::size_t reports = 0;
    std::int64_t score = 0;
    for (std::size_t i = 0; i < save::game_report_count; ++i) {
        if (!observed(snapshot.photo_present(save::PhotoSet::report, i))) continue;
        ++reports;
        score += observed(snapshot.report_score(i));
    }
    out << "saved_profile_inherited_Reports=" << reports << " present_Report_score_total=" << score
        << "\nsaved_profile_inherited_course_scores=";
    for (std::size_t i = 0; i < 7; ++i) {
        if (i) out << ',';
        out << observed(snapshot.course_score(i));
    }
    out << "\nsaved_profile_inherited_player_flags_source_id:value=";
    for (unsigned i = 0; i < 22; ++i) {
        if (i == 15) continue; // Source flag15 is transient RAM, not inherited disk progress.
        if (i) out << ',';
        out << i << ':' << observed(snapshot.player_flag(i));
    }
    out << "\nsaved_profile_inherited_progress_is_NOT_new_captures_Reports_trips_or_rewards=1\n";
}

} // namespace isle::private_campaign
