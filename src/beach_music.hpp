#ifndef ISLE_BEACH_MUSIC_HPP
#define ISLE_BEACH_MUSIC_HPP
#include "beach_music_source.h"
#include <cstddef>
namespace isle::scene { class Services; }
namespace isle::rom { class Snapshot; }
namespace isle::beach {
struct MusicAccess;
// Native Audio/Root borrower only. The complete original C callback owns all
// music-stage branches, handles and volume state in the one Source record.
class Music final {
    friend struct MusicAccess;
public:
    Music(scene::Services&, const rom::Snapshot&);
    ~Music();
    Music(const Music&) = delete;
    Music& operator=(const Music&) = delete;
    Music(Music&&) = delete;
    Music& operator=(Music&&) = delete;
    IsleGameResult bind(IslePlayerSource&) noexcept;
    IsleGameResult update(std::size_t block, float part) noexcept;
    // After actual Root deactivation, while its Source storage is still alive.
    IsleGameResult detach() noexcept;
    IsleBeachMusicSource& source() noexcept { return source_; }
    const IsleBeachMusicSource& source() const noexcept { return source_; }
private:
    scene::Services& services_;
    IsleBeachMusicSource source_{};
    bool bound_once_ = false;
};
}
#endif
