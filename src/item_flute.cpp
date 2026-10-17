#include "item_flute.hpp"
#include <array>

namespace isle::items {
namespace {
constexpr unsigned main_player = 0;
constexpr std::array<std::uint32_t, 3> songs{1, 3, 2};
constexpr std::array<std::int32_t, 3> commands{5, 6, 7};
}

Flute::Flute(game_audio::Session& session, FluteState& state) noexcept
    : session_(session), state_(state) {}

audio::Error Flute::play() noexcept {
    if (state_.playing) {
        ++state_.song_index;
        if (state_.song_index >= 3) state_.song_index -= 3;
    }
    auto error = session_.stop_song(main_player);
    if (error != audio::Error::none) return error;
    error = session_.play_song(main_player, songs[state_.song_index]);
    if (error != audio::Error::none) return error;
    state_.playing = true;
    state_.song_start_epoch = session_.source_frame_epoch();
    return audio::Error::none;
}

audio::Error Flute::stop() noexcept {
    if (state_.playing) {
        const auto error = session_.stop_song(main_player);
        if (error != audio::Error::none) return error;
    }
    state_.playing = false;
    state_.song_start_epoch = session_.source_frame_epoch();
    return audio::Error::none;
}

std::int32_t Flute::command() const noexcept {
    return state_.playing ? commands[state_.song_index] : 0;
}

std::int32_t Flute::state() const noexcept {
    return state_.song_start_epoch == session_.source_frame_epoch()
        ? -1 : static_cast<std::int32_t>(session_.bgm_playing(main_player));
}
} // namespace isle::items
