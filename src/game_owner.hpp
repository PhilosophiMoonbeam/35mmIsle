#ifndef ISLE_GAME_OWNER_HPP
#define ISLE_GAME_OWNER_HPP

#include "application_runtime.hpp"
#include "game_host.hpp"
#include <array>
#include <filesystem>
#include <memory>

namespace isle::game_owner {
struct EpochResult {
    bool successful = false;
    bool reset_requested = false;
    bool disconnect_requested = false;
    std::array<char,512> message{};
};

// Main owns this handle and the borrowed Host. One persistent ordinary thread
// constructs, runs and destroys every GAME owner, including across native reset.
// Immutable Snapshot/Catalog outlive this object. The current Host outlives its
// actual epoch retirement and finish(); main continues service() during native
// waits and shutdown. No Runtime, Input, Station or Session reference crosses
// this interface.
class Owner final {
public:
    Owner(const rom::Snapshot&, const menu::Catalog&, application::RuntimeConfig,
          std::filesystem::path station_directory);
    ~Owner();
    Owner(const Owner&) = delete;
    Owner& operator=(const Owner&) = delete;

    bool start_epoch(game_host::Host&, const std::filesystem::path& profile,
                     std::uint64_t application_epoch) noexcept;
    // Actual SDL/main operations and bounded requests; never dispatches GAME.
    void service() noexcept;
    void request_stop() noexcept;
    bool epoch_retired() const noexcept;
    EpochResult result() const noexcept;
    // Keeps main requests serviceable until actual GAME retirement, then wakes
    // and joins the persistent owner. Must run before destroying the last Host.
    void finish() noexcept;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace isle::game_owner
#endif
