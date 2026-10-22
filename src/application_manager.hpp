#ifndef ISLE_APPLICATION_MANAGER_HPP
#define ISLE_APPLICATION_MANAGER_HPP

#include "application_source.h"
#include "game_audio.hpp"
#include "game_state.hpp"
#include "host_clock.hpp"
#include "menu_resources.hpp"
#include <array>
#include <cstdint>
#include <memory>

namespace isle::application {
struct EntryIntent {
    std::uint64_t serial = 0;
    IsleApplicationEntryOperation operation = ISLE_APPLICATION_ENTRY_BOOT;
    std::uint32_t argument = 0;
};
// Original app_render/4CDB0.c data initializers: D_800AEBC0 and
// sUnkColorR/G/B are all zero. These are not a scene-local exit/fade flag.
struct ApplicationEndState {
    std::int32_t reason = 0;
    std::array<std::uint8_t, 3> transition_color{};
};
enum class Status { ready, waiting, entry_pending, completed, cancelled, failed, retired };
class Manager;
struct CreateResult;

// A real lifetime borrow, not an assertion that GPU/photo/audio work drained.
// Keep it around the actual native scene and every downstream borrower. Destroy
// those owners and resolve their fences BEFORE releasing this borrow. Manager
// must outlive it; completion/retirement reject a still-live borrow.
class EntryBorrow final {
public:
    EntryBorrow() noexcept = default;
    ~EntryBorrow();
    EntryBorrow(EntryBorrow&&) noexcept;
    EntryBorrow& operator=(EntryBorrow&&) noexcept;
    EntryBorrow(const EntryBorrow&) = delete;
    EntryBorrow& operator=(const EntryBorrow&) = delete;
    explicit operator bool() const noexcept { return manager_ != nullptr; }
    const EntryIntent& intent() const noexcept { return intent_; }
private:
    EntryBorrow(Manager&, EntryIntent) noexcept;
    Manager* manager_ = nullptr;
    EntryIntent intent_{};
    friend class Manager;
};

// Immutable pinned data residency. Views are published only by the original C
// load operations, remain backed by the admitted Snapshot, and are never cast
// to executable functions. Native GAME code is compiled into the application;
// these images establish data/provenance, NOT implementation of a scene entry.
// VPK views borrow actual Catalog-owned decompression, never compressed bytes.
class Resources final {
public:
    Resources(const Resources&) = delete;
    Resources& operator=(const Resources&) = delete;
    rom::GuestView overlay(IsleApplicationOverlay) const noexcept;
    // Successful original overlay-load events, not cached-view residency.
    // Consumers reset mutable module data only when this generation changes.
    std::uint64_t overlay_generation(IsleApplicationOverlay) const noexcept;
    rom::GuestView rom(IsleApplicationRom) const noexcept;
    rom::GuestView vpk(IsleApplicationVpk) const noexcept;
    bool live() const noexcept;
private:
    Resources(const rom::Snapshot&, const menu::Catalog&) noexcept;
    IsleGameResult retain(IsleApplicationOverlay) noexcept;
    IsleGameResult retain(IsleApplicationRom) noexcept;
    IsleGameResult retain(IsleApplicationVpk) noexcept;
    const rom::Snapshot& snapshot_;
    const menu::Catalog& catalog_;
    const std::byte* binding_ = nullptr;
    std::array<rom::GuestView, 30> overlays_{};
    std::array<std::uint64_t, 30> overlay_generations_{};
    std::array<rom::GuestView, 19> roms_{};
    std::array<rom::GuestView, 2> vpks_{};
    std::array<sprite::BankOwner, 2> banks_{};
    friend class Manager;
    friend struct NativeAccess;
};

// Independent, nonmovable application continuation. All dependencies are real
// borrowed application-lifetime owners; no scene domain/storage is used here.
// Owner-thread API only. resume never constructs/drives a scene. After it
// returns, acquire a borrow for pending_entry(), create the native scene, drive
// it with the host's real scheduler, retire its owners/fences, release the
// borrow, then complete_entry with its actual signed Source result. Unsupported
// entry operations must fail at the host, never complete as a finished scene.
class Manager final {
public:
    static CreateResult create(const rom::Snapshot&, game::State&, game_input::Service&,
        game_audio::Session&, host::GtlClock&, const menu::Catalog&,
        std::int32_t manager_argument = 0) noexcept;
    ~Manager();
    Manager(const Manager&) = delete;
    Manager& operator=(const Manager&) = delete;
    Manager(Manager&&) = delete;
    Manager& operator=(Manager&&) = delete;
    IsleGameResult resume() noexcept;
    const EntryIntent* pending_entry() const noexcept;
    EntryBorrow borrow_entry(std::uint64_t serial) noexcept;
    // Admit only this manager's still-held borrow of the current real request.
    bool owns_entry(const EntryBorrow&, IsleApplicationEntryOperation operation,
        std::uint32_t argument) const noexcept;
    IsleGameResult complete_entry(std::uint64_t serial, std::int32_t source_result) noexcept;
    // cancel joins original C, but never cancels a native scene behind its
    // owner's back. Retained entry/resources survive until its borrow retires.
    IsleGameResult cancel() noexcept;
    IsleGameResult retire() noexcept;
    Status status() const noexcept;
    IsleGameResult error() const noexcept;
    const char* message() const noexcept;
    std::int32_t source_level() const noexcept;
    const game::VideoOffsets& screen_offsets() const noexcept;
    // Returning owner-thread service for original func_800A19D8. Safe inside
    // an existing scene's nonwaiting owner handoff; never drives a scene.
    IsleGameResult transition_end(std::int32_t reason) noexcept;
    const ApplicationEndState& end_state() const noexcept;
    const Resources& resources() const noexcept;
private:
    struct Impl;
    explicit Manager(std::unique_ptr<Impl>) noexcept;
    void release_entry(std::uint64_t serial) noexcept;
    std::unique_ptr<Impl> impl_;
    friend class EntryBorrow;
    friend struct NativeAccess;
};
struct CreateResult {
    IsleGameResult error = ISLE_GAME_OK;
    std::unique_ptr<Manager> manager;
    std::array<char, 512> message{};
    explicit operator bool() const noexcept { return error == ISLE_GAME_OK && bool(manager); }
};
} // namespace isle::application
#endif
