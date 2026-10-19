#include "course_scene_native.hpp"
extern "C" IsleGameResult isle_course_scene_native_input_update(IsleGame* game,IsleGameObject* object) {
    if (!game || !object) return ISLE_GAME_INVALID_ARGUMENT;
    IslePlayerSource* player=nullptr;
    auto result=isle_player_source_context(game,&player);
    if (result!=ISLE_GAME_OK) return result;
    if (!player || !player->native_root || player->game!=game) return ISLE_GAME_INVALID_STATE;
    auto& root=*static_cast<isle::player::Root*>(player->native_root);
    auto* owner=root.bound_actors();
    if (&root.source()!=player || !owner || &owner->player_source()!=player) return ISLE_GAME_INVALID_STATE;
    isle::course::NativeHost* host=nullptr;
    try {
        const auto& context=owner->course_context();
        switch (context.course_id()) {
            case 1: host=context.tunnel().native_host; break;
            case 2: host=context.volcano().native_host; break;
            case 3: host=context.river().native_host; break;
            case 4: host=context.cave().native_host; break;
            case 5: host=context.valley().native_host; break;
            default: return ISLE_GAME_INVALID_STATE;
        }
    } catch (...) { return ISLE_GAME_INVALID_STATE; }
    if (!host || host->game()!=game || host->root()!=&root || host->actors()!=owner) return ISLE_GAME_INVALID_STATE;
    return host->invalidate_input(object);
}
