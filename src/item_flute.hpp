#ifndef ISLE_ITEM_FLUTE_HPP
#define ISLE_ITEM_FLUTE_HPP

#include "game_audio.hpp"
#include <cstdint>

namespace isle::items {
// Caller-retained BSS-equivalent state. Reconstructing the wrapper or invoking
// Items_Init does NOT reset this state. Initialize once for its actual source
// lifetime and mutate only through Flute; song_index remains in [0,2].
struct FluteState {
    std::int32_t song_index = 0;
    bool playing = false;
    std::uint64_t song_start_epoch = 0;
};
// The existing real AudioSession build serial is an equality-domain translation
// of the source audio-frame timestamp, NOT OSTime ticks or a second clock.
// No Item call, query, renderer, retrace, completion or no-work build advances it.
class Flute final {
public:
    Flute(game_audio::Session&, FluteState&) noexcept;
    audio::Error play() noexcept;
    audio::Error stop() noexcept;
    std::int32_t command() const noexcept;
    std::int32_t state() const noexcept;
private:
    game_audio::Session& session_;
    FluteState& state_;
};
} // namespace isle::items
#endif
