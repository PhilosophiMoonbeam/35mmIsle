#ifndef ISLE_GAME_SAVE_HPP
#define ISLE_GAME_SAVE_HPP

#include "35mmisle/save_store.hpp"

#include <memory>

namespace isle::application { class Photos; }

namespace isle::save {

enum class GameErrc { none, unavailable, field_bounds, canonical_image, source_state };
struct GameError {
    GameErrc code = GameErrc::none;
    Error codec_error = Error::none;
    [[nodiscard]] explicit operator bool() const noexcept { return code != GameErrc::none; }
};
template<class T> struct GameValue {
    T value{};
    GameError error{};
    [[nodiscard]] explicit operator bool() const noexcept { return !error; }
};
struct ScreenOffsets { int horizontal = 0; int vertical = 0; };
enum class PhotoSet { report, temporary, album };
inline constexpr std::size_t game_photo_bytes = 0x3a0;
inline constexpr std::size_t game_report_count = 69;
inline constexpr std::size_t game_album_count = 60;
inline constexpr std::size_t game_temporary_count = 4;

// Owns immutable canonical bytes. Every returned span/view remains valid while
// this snapshot is retained, including after its session changes or is destroyed.
// Photo spans are wire-format records, NOT host structs or replay authorization.
class GameSnapshot {
public:
    GameSnapshot() noexcept = default;
    [[nodiscard]] explicit operator bool() const noexcept { return bool(data_); }
    [[nodiscard]] ImageView image() const noexcept;
    [[nodiscard]] std::uint64_t generation() const noexcept;
    [[nodiscard]] GameValue<bool> player_flag(unsigned source_id) const noexcept;
    [[nodiscard]] GameValue<ScreenOffsets> screen_offsets() const noexcept;
    [[nodiscard]] GameValue<unsigned> selected_course() const noexcept;
    [[nodiscard]] GameValue<int> unlocked_course() const noexcept;
    [[nodiscard]] GameValue<std::int32_t> course_score(std::size_t index) const noexcept;
    [[nodiscard]] GameValue<std::int32_t> report_score(std::size_t index) const noexcept;
    [[nodiscard]] std::span<const std::byte> player_name() const noexcept;
    [[nodiscard]] GameValue<std::span<const std::byte>> photo(PhotoSet set, std::size_t index) const noexcept;
    [[nodiscard]] GameValue<bool> photo_present(PhotoSet set, std::size_t index) const noexcept;
    [[nodiscard]] GameValue<std::span<const std::byte>> album_comment(std::size_t index) const noexcept;
private:
    struct Data;
    std::shared_ptr<const Data> data_;
    ImageView image_{};
    friend class GameSession;
};

enum class GameLoadState {
    ready, initialized_missing, recovery_required, conflict, io_error, invalid_state,
};
struct GameOpenResult;
// One serialized gameplay owner. No per-getter allocation or payload copying.
// Mutation copies only when needed to preserve an outstanding immutable snapshot.
// Standard allocation/path exceptions follow Store's C++ policy; C callers must
// catch at their service boundary. Never expose this object as a raw GAME struct.
class GameSession {
public:
    GameSession() noexcept;
    // An application Photos binding pins this object's address/lifetime.
    // Moving either operand or destroying the session while bound terminates;
    // destroy Photos first. Unbound sessions retain ordinary noexcept moves.
    ~GameSession();
    GameSession(GameSession&&) noexcept;
    GameSession& operator=(GameSession&&) noexcept;
    GameSession(const GameSession&) = delete;
    GameSession& operator=(const GameSession&) = delete;
    [[nodiscard]] explicit operator bool() const noexcept;
    // Existing absolute directory only. Initializes RAM only for genuine Missing;
    // no durable write until explicit commit. Rejected profiles stay locked and
    // read-only, with their original Store evidence in GameOpenResult::load.
    [[nodiscard]] static GameOpenResult open(const std::filesystem::path& directory,
                                             std::uint32_t raw_game_time);
    [[nodiscard]] GameValue<GameSnapshot> snapshot();
    // Explicit source func_800C042C reset: RAM-only defaults and native sealing.
    // Preserves the Store lock/generation, owning snapshots and both existing
    // startup notices; clears transient PFID15 and the course notice. Only a
    // writable session may reset. Persistence requires a separate source commit.
    [[nodiscard]] GameError reset(std::uint32_t raw_game_time);
    [[nodiscard]] GameError set_player_flag(unsigned source_id, bool value);
    // Screen offsets and unlock progress use the original source clamping policy.
    [[nodiscard]] GameError set_screen_offsets(int horizontal, int vertical);
    [[nodiscard]] GameError set_selected_course(unsigned course);
    [[nodiscard]] GameError set_unlocked_course(int course);
    [[nodiscard]] GameError set_course_score(std::size_t index, std::int32_t score);
    [[nodiscard]] GameError set_report_score(std::size_t index, std::int32_t score);
    [[nodiscard]] GameValue<std::int32_t> report_score(std::size_t index) const noexcept;
    // Source func_800C02EC / func_800C0314: signed index clamps to 0..2
    // (apple, pester ball, flute). RAM only; true raises the SAME course notice
    // when possession is false, without granting possession. Reset clears all.
    [[nodiscard]] GameValue<bool> pending_item_grant(int source_index) const noexcept;
    [[nodiscard]] GameError set_pending_item_grant(int source_index, bool value) noexcept;
    // Exactly 16 source encoding bytes, no host text conversion or implicit name.
    [[nodiscard]] GameError set_player_name(std::span<const std::byte> name);
    // Copies exactly one canonical wire record, not a native PhotoData layout.
    // Report/temporary/album bounds are 69/4/60. Independent of score/comment
    // writes and disk commit; preserves all outstanding immutable snapshots.
    [[nodiscard]] GameError set_photo(PhotoSet set, std::size_t index, std::span<const std::byte> photo);
    // Scoring mutates this source flag byte only; retain every other wire byte.
    [[nodiscard]] GameError set_photo_flags(PhotoSet set, std::size_t index, std::uint8_t flags);
    // Borrowed canonical views, valid only until the next session mutation.
    // Unlike snapshot(), these do not seal/hash the whole image for a read.
    [[nodiscard]] GameValue<std::span<const std::byte>> photo(PhotoSet set, std::size_t index) const noexcept;
    [[nodiscard]] GameValue<std::span<const std::byte>> album_comment(std::size_t index) const noexcept;
    [[nodiscard]] GameError remove_photo(PhotoSet set, std::size_t index);
    // Exactly 32 BE16 source characters. These are not host wchar_t values.
    [[nodiscard]] GameError set_album_comment(std::size_t index, std::span<const std::byte> comment);
    [[nodiscard]] CommitResult commit(std::uint32_t raw_game_time);
    // Source func_800C03D4 / func_800C0400: independent consume-once RAM notices.
    [[nodiscard]] bool consume_initialization_notice() noexcept;
    [[nodiscard]] bool consume_initialization_notice_secondary() noexcept;
    // Source func_800C027C, set by unlock progress beyond selected course.
    [[nodiscard]] bool consume_course_notice() noexcept;
private:
    // One application-owned representation cache. Notifications are nonwaiting,
    // nonthrowing native leaves, never source callbacks or checked transfers.
    // They must not mutate/reenter session mutation. Session and Photos keep
    // their existing addresses until the binding is released.
    struct PhotoProjection {
        void* owner = nullptr;
        void (*photo_changed)(void*, PhotoSet, std::size_t, std::span<const std::byte>, bool flags_only) noexcept = nullptr;
        void (*comment_changed)(void*, std::size_t, std::span<const std::byte>) noexcept = nullptr;
        void (*reset)(void*) noexcept = nullptr;
    };
    [[nodiscard]] GameError bind_photo_projection(PhotoProjection projection) noexcept;
    void unbind_photo_projection(void* owner) noexcept;
    // Photos holds one actual replay read across Source waits. Saved mutation
    // rejects only that slot; a roll borrow still fences whole-session reset.
    [[nodiscard]] GameError borrow_replay(const void* borrower, bool saved,
        PhotoSet set, std::size_t index) noexcept;
    [[nodiscard]] GameError release_replay(const void* borrower) noexcept;
    friend class ::isle::application::Photos;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
struct GameOpenResult {
    GameSession session{};
    GameLoadState state = GameLoadState::io_error;
    LoadResult load{};
    StoreError store_error{};
    GameError error{};
    [[nodiscard]] explicit operator bool() const noexcept {
        return bool(session) && (state == GameLoadState::ready || state == GameLoadState::initialized_missing);
    }
};

} // namespace isle::save

#endif
