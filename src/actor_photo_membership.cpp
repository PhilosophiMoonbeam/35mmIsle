#include "actor_photo_membership.hpp"
#include "actor_owner_source.h"

namespace isle::actor::photo {
IsleGameResult collect_membership(IsleGame* game, IsleGameCamera* camera,
                                  focus::Photo& output) noexcept {
    Camera prepared;
    auto result = prepare_camera(game, camera, prepared);
    if (result != ISLE_GAME_OK) return result;
    focus::Photo candidate{};
    IsleGameObject* previous = nullptr;
    std::size_t count = 0;
    while (count < candidate.objects.size()) {
        IsleGameObject* object = nullptr;
        IsleActorSource* source = nullptr;
        IsleGameModel* model = nullptr;
        result = isle_actor_source_link3_next(game, previous, &object, &source, &model);
        if (result != ISLE_GAME_OK) return result;
        if (object == nullptr) break;
        bool accepted = false;
        result = admitted(*source, prepared, source->collPosition, accepted);
        if (result != ISLE_GAME_OK) return result;
        if (accepted) candidate.objects[count++] = reinterpret_cast<focus::Object>(object);
        previous = object;
    }
    output = candidate;
    return ISLE_GAME_OK;
}
} // namespace isle::actor::photo
