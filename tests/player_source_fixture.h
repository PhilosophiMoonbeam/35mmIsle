#ifndef ISLE_PLAYER_SOURCE_FIXTURE_H
#define ISLE_PLAYER_SOURCE_FIXTURE_H
#include "player_source.h"
#ifdef __cplusplus
extern "C" {
#endif
void player_source_helper_userdata(IsleGameObject*, void*);
void player_source_helper_wait(IsleGameObject*);
void player_source_cold_entry(IslePlayerSource*, void*);
void player_source_nested_failure(IslePlayerSource*, void*);
void player_source_forbidden_wait(IslePlayerSource*, void*);
void player_source_native_actor_failure(IslePlayerSource*, void*);
typedef struct PlayerSpriteBorrow {
    IsleGameObject* object;
    IsleGameSprite* sprite;
    IslePlayerSource* context;
    int view_only;
    int entered, completed;
} PlayerSpriteBorrow;
void player_source_sprite_borrow(IslePlayerSource*, void*);
void player_source_sprite_nested_borrow(IslePlayerSource*, void*);
void player_source_sprite_wait(IsleGameObject*);
void player_source_sprite_delete_self(IsleGameObject*);
void player_source_sprite_explicit_context(IsleGameObject*, void*);
typedef struct PlayerObjectControl {
    IsleGameObject* object;
    int paused;
    int32_t link;
    uint32_t id;
    IsleGameObject* found;
    int completed;
} PlayerObjectControl;
void player_source_pause_all(IslePlayerSource*, void*);
void player_source_find_object(IslePlayerSource*, void*);
void player_source_pause_current(IsleGameObject*);
#ifdef __cplusplus
}
#endif
#endif
