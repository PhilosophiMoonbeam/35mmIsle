#ifndef ISLE_APPLICATION_PHOTOS_HPP
#define ISLE_APPLICATION_PHOTOS_HPP
#include "player_photo.hpp"
#include "application_photos_source.h"
namespace isle::application {
class PhotoWorking;
// One application roll and its admitted tables, independent of scene Storage.
class Photos final {
public:
    // The session must outlive this owner and may not move while it is bound.
    Photos(const rom::Snapshot&, save::GameSession&);
    ~Photos();
    Photos(const Photos&) = delete;
    Photos& operator=(const Photos&) = delete;
    Photos(Photos&&) = delete;
    Photos& operator=(Photos&&) = delete;
    // Mutable source access is the exclusive course-owner interface, not a
    // replay writer. It must not bind/mutate the roll while replay is borrowed.
    IslePlayerPhotoSource& source() noexcept { return source_; }
    const IslePlayerPhotoSource& source() const noexcept { return source_; }
    const player::PhotoResources& resources() const noexcept { return resources_; }
    // Immutable admitted attract inputs share application lifetime because
    // canonical State retains the original setIdleScript pointer across scenes.
    const IslePlayerIdleScript& beach_idle_script() const noexcept { return beach_idle_; }
    const IslePlayerIdleScript& tunnel_idle_script() const noexcept { return tunnel_idle_; }
    IsleGameResult reset() noexcept;
    std::int32_t count() const noexcept { return source_.gPhotoCount; }
    IsleApplicationPhotos* native_source() noexcept {
        return reinterpret_cast<IsleApplicationPhotos*>(this);
    }
    save::GameSession& save_session() noexcept { return save_; }
    // func_800BFABC: NULL resets count; full queue ignores append. Only actual
    // active roll records may enter the handoff; all three flags are named.
    [[nodiscard]] save::GameError select(IslePlayerPhotoData*, bool for_oak,
        bool source_second_flag, bool for_album) noexcept;
    std::int32_t selected_count() const noexcept { return selected_count_; }
    const IsleSelectedPhoto* selected(std::int32_t index) const noexcept;
    // Stable read-only semantic records. Saved pointers are updated in place
    // by every canonical set/remove/reset, including direct GameSession calls.
    [[nodiscard]] save::GameValue<IslePhotoRecord> record(IslePhotoStorage, std::int32_t index) noexcept;
    // Resolves an original retained pointer to its canonical owner/slot. This is
    // identity lookup only, never a replay-completion or submission capability.
    [[nodiscard]] save::GameValue<IslePhotoRecord> identify(const IslePlayerPhotoData*) noexcept;
    [[nodiscard]] save::GameError apply_photo_flags(const IslePhotoRecord&, std::uint8_t flags);
    // Single canonical replay read lease. No source owner may be bound. Saved
    // writes to this slot and whole-session reset reject until release; other
    // scores/comments/player flags remain writable. Native source translations
    // must route roll mutations through Photos, never retained writable aliases.
    // Revision storage is owner-stable, not a temporary IslePhotoRecord member.
    [[nodiscard]] save::GameError acquire_replay(const IslePhotoRecord&, const void* borrower,
        const std::uint64_t** revision) noexcept;
    [[nodiscard]] save::GameError release_replay(const void* borrower) noexcept;
    std::int32_t report_index(std::int32_t source_id) noexcept;
    [[nodiscard]] save::GameValue<bool> report_present(std::int32_t source_id) noexcept;
    [[nodiscard]] save::GameError set_photo(save::PhotoSet, std::size_t index, const IslePlayerPhotoData&);
    [[nodiscard]] save::GameValue<const std::uint16_t*> album_comment(std::size_t index) noexcept;
    [[nodiscard]] save::GameError set_album_comment(std::size_t index, const std::uint16_t* comment);
    [[nodiscard]] save::GameError set_album_comment_char(std::size_t index,
        std::size_t character, std::uint16_t value);
    [[nodiscard]] save::GameError set_working(save::PhotoSet, std::size_t,
        const PhotoWorking&);
    // NULL operands are independent no-ops; comment is 32 numeric source chars.
    [[nodiscard]] save::GameError set_album(std::size_t index, const IslePlayerPhotoData*,
        const std::uint16_t* comment);
private:
    friend class PhotoWorking;
    [[nodiscard]] save::GameError acquire_working_replay(PhotoWorking&,const void*,
        IslePhotoWorkingRead&) noexcept;
    struct SavedPhoto {
        IslePlayerPhotoData photo{};
        std::uint64_t revision = 1;
        bool loaded = false;
    };
    struct AlbumComment {
        std::array<std::uint16_t, 32> text{};
        bool loaded = false;
    };
    SavedPhoto* saved_photo(save::PhotoSet, std::size_t index) noexcept;
    void reset_projection() noexcept;
    static void photo_changed(void*, save::PhotoSet, std::size_t,
        std::span<const std::byte>, bool flags_only) noexcept;
    static void comment_changed(void*, std::size_t, std::span<const std::byte>) noexcept;
    static void session_reset(void*) noexcept;
    save::GameSession& save_;
    player::PhotoResources resources_;
    IslePlayerPhotoSource source_{};
    std::vector<IslePlayerInput> beach_inputs_;
    IslePlayerIdleScript beach_idle_{};
    std::vector<IslePlayerInput> tunnel_inputs_;
    IslePlayerIdleScript tunnel_idle_{};
    std::array<IsleSelectedPhoto, 60> selected_{};
    std::int32_t selected_count_ = 0;
    std::array<std::uint64_t, 60> roll_revisions_{};
    std::array<SavedPhoto, save::game_report_count> report_{};
    std::array<SavedPhoto, save::game_temporary_count> temporary_{};
    std::array<SavedPhoto, save::game_album_count> album_{};
    std::array<AlbumComment, save::game_album_count> comments_{};
    const void* replay_borrower_ = nullptr;
    IslePhotoRecord replay_record_{};
    PhotoWorking* replay_working_ = nullptr;
    std::size_t working_owners_ = 0;
};
// Cold fixed storage for an Original PhotoData assignment. It is not a saved
// slot: saved inputs retain every wire byte, including reserved bytes, while
// the semantic view is separately decoded. Photos must outlive this owner.
class PhotoWorking final {
public:
    explicit PhotoWorking(Photos&) noexcept;
    ~PhotoWorking() noexcept;
    PhotoWorking(const PhotoWorking&) = delete;
    PhotoWorking& operator=(const PhotoWorking&) = delete;
    PhotoWorking(PhotoWorking&&) = delete;
    PhotoWorking& operator=(PhotoWorking&&) = delete;
    IslePhotoWorking* source() const noexcept { return handle_; }
    [[nodiscard]] save::GameError copy_record(const IslePhotoRecord&) noexcept;
    [[nodiscard]] save::GameError copy(const PhotoWorking&) noexcept;
    [[nodiscard]] save::GameError clear() noexcept;
    const IslePlayerPhotoData* photo() const noexcept { return &photo_; }
    IslePhotoWorkingRead describe() const noexcept;
    [[nodiscard]] save::GameError acquire(const void*, IslePhotoWorkingRead&) noexcept;
    bool validates(const IslePhotoWorkingRead&) const noexcept;
    [[nodiscard]] save::GameError release(IslePhotoWorkingRead&) noexcept;
private:
    friend class Photos;
    IslePhotoWorking* const handle_;
    bool mutable_now() const noexcept;
    Photos& photos_;
    std::array<std::byte, save::game_photo_bytes> wire_{};
    IslePlayerPhotoData photo_{};
    std::uint64_t revision_ = 1;
    const void* borrower_ = nullptr;
    bool valid_ = false;
};
}
#endif
