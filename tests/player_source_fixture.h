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
#ifdef __cplusplus
}
#endif
#endif
