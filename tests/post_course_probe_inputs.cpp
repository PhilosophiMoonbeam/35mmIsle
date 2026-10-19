#include "post_course_probe_inputs.hpp"
#include "application_photos.hpp"
#include <array>
#include <cstdint>
#include <stdexcept>

namespace isle::private_post_course {
namespace {
void accepted(IsleGameResult result, const char* operation) {
    if (result != ISLE_GAME_OK) throw std::runtime_error(operation);
}
void accepted(save::GameError result, const char* operation) {
    if (result) throw std::runtime_error(operation);
}

IslePlayerPhotoData authored_photo(std::int32_t species) {
    IslePlayerPhotoData photo{};
    photo.levelID = 0;
    photo.unk_00_16 = 0x20; // Beach block zero, ordinary subject photo.
    photo.unk_08 = {0, 60, 300};
    photo.unk_14 = {0, 60, -250};
    for (auto& subject : photo.pokemons) subject.pokemonID = -1;
    for (auto& item : photo.items) item.itemType = -1;
    for (auto& effect : photo.effects) effect.textureID = -1;
    auto& subject = photo.pokemons[0];
    subject.pokemonID = species;
    subject.position = {0, 30, -250};
    // Original D_800AD4B8 admits variant zero for all four species (counts
    // 4/13/5/2 respectively). Seek its real model/material animation at zero;
    // no special scoring pose, SDK feedback, or captured course state is claimed.
    subject.unk_00_13 = 0;
    subject.animationTime = 0;
    return photo;
}
}

void seed(application::Photos& photos, game::State& state) {
    // Keep the actual ingress State and its flags untouched. These deliberately
    // authored photos, scores and comments are inputs to the real menu APIs.
    (void)state;
    auto& session = photos.save_session();
    for (std::size_t slot = 0; slot < save::game_report_count; ++slot) {
        accepted(session.remove_photo(save::PhotoSet::report, slot), "authored report clear rejected");
        accepted(session.set_report_score(slot, 0), "authored report score clear rejected");
    }
    for (std::size_t slot = 0; slot < save::game_temporary_count; ++slot)
        accepted(session.remove_photo(save::PhotoSet::temporary, slot), "authored temporary clear rejected");
    for (std::size_t slot = 0; slot < save::game_album_count; ++slot)
        accepted(session.remove_photo(save::PhotoSet::album, slot), "authored album clear rejected");

    constexpr std::array<std::int32_t, 4> species{16, 25, 84, 12};
    constexpr std::array<std::int32_t, 4> authored_scores{1200, 2300, 3400, 4500};
    // Native full-width source characters: BEACH 1 / BEACH 2, NUL padded to
    // exactly 32 numeric characters. These are not UTF-8 or host-endian blobs.
    std::array<std::uint16_t, 32> comment{0xa3c2, 0xa3c5, 0xa3c1, 0xa3c3, 0xa3c8, 0xa1a1, 0xa3b1};
    for (std::size_t slot = 0; slot < species.size(); ++slot) {
        const auto photo = authored_photo(species[slot]);
        accepted(isle_application_photos_set_report(photos.native_source(), species[slot], &photo),
                 "authored species report rejected");
        accepted(isle_application_photos_set_report_score(photos.native_source(), species[slot], authored_scores[slot]),
                 "authored species score rejected");
        accepted(photos.set_photo(save::PhotoSet::temporary, slot, photo), "authored temporary photo rejected");
        if (slot < 2) {
            comment[6] = static_cast<std::uint16_t>(0xa3b1 + slot);
            accepted(photos.set_album(slot, &photo, comment.data()), "authored album photo/comment rejected");
        }
    }
}
}
