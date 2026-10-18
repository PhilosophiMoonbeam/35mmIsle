#include "beach_music.hpp"
#include "rom_snapshot.hpp"
#include "rom_view.hpp"
#include "scene_services.hpp"
#include <bit>
#include <exception>
#include <limits>
#include <stdexcept>

namespace isle::beach {
Music::Music(scene::Services& services, const rom::Snapshot& snapshot) : services_(services) {
    const auto bytes = snapshot.canonical();
    if (bytes.size() != rom::us_size || services.failed())
        throw std::invalid_argument("Beach music requires admitted ROM and actual native Services");
    // Pinned splat beach_code maps ROM55C110 to802C40A0 and names beach_$VRAM.
    // beach.h25–27 declares these signed .data words; they are not BSS defaults.
    const rom::GuestView data{0x802cc0d0, 0x564140, bytes.subspan(0x564140, 12)};
    source_.D_beach_802CC0D0 = std::bit_cast<int32_t>(data.u32(0));
    source_.D_beach_802CC0D4 = std::bit_cast<int32_t>(data.u32(4));
    source_.D_beach_802CC0D8 = std::bit_cast<int32_t>(data.u32(8));
    // Source55C870.c4–10 supplies the zero/BSS handle and volume state once.
    source_.owner = this;
}
Music::~Music() {
    if (source_.player) std::terminate();
}
IsleGameResult Music::bind(IslePlayerSource& player) noexcept {
    if (bound_once_ || source_.player || player.game != services_.game || !player.native_root)
        return ISLE_GAME_INVALID_STATE;
    IslePlayerSource* canonical = nullptr;
    const auto result = isle_player_source_context(services_.game, &canonical);
    if (result != ISLE_GAME_OK) return result;
    if (canonical != &player) return ISLE_GAME_INVALID_STATE;
    source_.player = &player;
    bound_once_ = true;
    return ISLE_GAME_OK;
}
IsleGameResult Music::update(std::size_t block, float part) noexcept {
    if (block > static_cast<std::size_t>(std::numeric_limits<int32_t>::max())) return ISLE_GAME_INVALID_ARGUMENT;
    return isle_beach_music_invoke(&source_, static_cast<int32_t>(block), part);
}
IsleGameResult Music::detach() noexcept {
    if (source_.player && source_.player->native_root) return ISLE_GAME_INVALID_STATE;
    source_.player = nullptr;
    return ISLE_GAME_OK;
}
struct MusicAccess {
    static Music* owner(IsleBeachMusicSource* source) noexcept {
        if (!source || !source->owner || !source->player) return nullptr;
        auto* self = static_cast<Music*>(source->owner);
        return &self->source_ == source && source->player->game == self->services_.game ? self : nullptr;
    }
    template<class F> static IsleGameResult invoke(IsleBeachMusicSource* source, F&& function) noexcept {
        auto* self = owner(source);
        if (!self) return ISLE_GAME_INVALID_ARGUMENT;
        IslePlayerSource* canonical = nullptr;
        const auto result = isle_player_source_context(self->services_.game, &canonical);
        if (result != ISLE_GAME_OK) return result;
        if (canonical != source->player || self->services_.failed()) return ISLE_GAME_INVALID_STATE;
        return self->services_.check(function(self->services_.audio), "original Beach music audio operation failed")
            ? ISLE_GAME_OK : ISLE_GAME_EXECUTION_FAILURE;
    }
};
}
extern "C" {
IsleGameResult isle_beach_music_native_play_sound(IsleBeachMusicSource* source, uint32_t sound,
    int32_t volume, int32_t pan, float pitch, int32_t reverb, int32_t* output) {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    *output = -1;
    return isle::beach::MusicAccess::invoke(source, [&](isle::game_audio::Session& audio) {
        int handle = -1;
        const auto result = audio.play_sound(sound,
            {static_cast<uint16_t>(volume), static_cast<uint8_t>(pan), static_cast<uint8_t>(reverb), pitch}, handle);
        *output = handle;
        return result;
    });
}
IsleGameResult isle_beach_music_native_volume(IsleBeachMusicSource* source, int32_t handle, uint32_t volume) {
    return isle::beach::MusicAccess::invoke(source, [&](isle::game_audio::Session& audio) {
        return audio.sound_volume(static_cast<unsigned>(handle), volume);
    });
}
IsleGameResult isle_beach_music_native_stop(IsleBeachMusicSource* source, uint32_t handle) {
    return isle::beach::MusicAccess::invoke(source, [&](isle::game_audio::Session& audio) { return audio.stop_sound(handle); });
}
}
