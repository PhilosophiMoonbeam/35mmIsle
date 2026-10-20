#ifndef ISLE_PRIVATE_CAMPAIGN_SAVED_PROFILE_HPP
#define ISLE_PRIVATE_CAMPAIGN_SAVED_PROFILE_HPP

#include "35mmisle/game_save.hpp"

#include <filesystem>
#include <iosfwd>
#include <memory>

namespace isle::private_campaign {

// Private preserved-input capsule. Only the independent destination is opened
// by the game loader; no mutable session or profile lock survives preparation.
class SavedProfileInput final {
public:
    [[nodiscard]] static SavedProfileInput prepare(
        const std::filesystem::path& source_directory,
        const std::filesystem::path& new_empty_destination_profile);
    ~SavedProfileInput();
    SavedProfileInput(SavedProfileInput&&) noexcept;
    SavedProfileInput& operator=(SavedProfileInput&&) noexcept;
    SavedProfileInput(const SavedProfileInput&) = delete;
    SavedProfileInput& operator=(const SavedProfileInput&) = delete;

    [[nodiscard]] const save::GameSnapshot& baseline() const noexcept;
    [[nodiscard]] const std::filesystem::path& input_path() const noexcept;
    void require_boot_load(const save::GameOpenResult&, const save::GameSnapshot& actual) const;
    void verify_source_unchanged() const;
    void write_evidence(std::ostream&) const;

private:
    struct Impl;
    explicit SavedProfileInput(std::unique_ptr<Impl>) noexcept;
    std::unique_ptr<Impl> impl_;
};

} // namespace isle::private_campaign

#endif
